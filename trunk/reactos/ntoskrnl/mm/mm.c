/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: mm.c,v 1.74 2004/06/01 10:16:26 gdalsnes Exp $
 *
 * COPYRIGHT:   See COPYING in the top directory
 * PROJECT:     ReactOS kernel 
 * FILE:        ntoskrnl/mm/mm.c
 * PURPOSE:     kernel memory managment functions
 * PROGRAMMER:  David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *              Created 9/4/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/i386/segment.h>
#include <internal/mm.h>
#include <internal/ntoskrnl.h>
#include <internal/io.h>
#include <internal/ps.h>
#include <reactos/bugcodes.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *****************************************************************/

PVOID EXPORTED MmUserProbeAddress = NULL;
PVOID EXPORTED MmHighestUserAddress = NULL;

PVOID MmSystemRangeStart = NULL;
MM_STATS MmStats;

/* FUNCTIONS ****************************************************************/


NTSTATUS STDCALL
MmCopyToCaller(PVOID Dest, const VOID *Src, ULONG NumberOfBytes)
{
  NTSTATUS Status;

  if (ExGetPreviousMode() == UserMode)
    {
      if ((ULONG)Dest >= KERNEL_BASE)
   {
     return(STATUS_ACCESS_VIOLATION);
   }
      Status = MmSafeCopyToUser(Dest, Src, NumberOfBytes);
      return(Status);
    }
  else
    {
      memcpy(Dest, Src, NumberOfBytes);
      return(STATUS_SUCCESS);
    }
}

NTSTATUS STDCALL
MmCopyFromCaller(PVOID Dest, const VOID *Src, ULONG NumberOfBytes)
{
  NTSTATUS Status;

  if (ExGetPreviousMode() == UserMode)
    {
      if ((ULONG)Src >= KERNEL_BASE)
   {
     return(STATUS_ACCESS_VIOLATION);
   }
      Status = MmSafeCopyFromUser(Dest, Src, NumberOfBytes);
      return(Status);
    }
  else
    {
      memcpy(Dest, Src, NumberOfBytes);
      return(STATUS_SUCCESS);
    }
}



NTSTATUS MmReleaseMemoryArea(PEPROCESS Process, PMEMORY_AREA Marea)
{
   NTSTATUS Status;

   DPRINT("MmReleaseMemoryArea(Process %x, Marea %x)\n",Process,Marea);

   DPRINT("Releasing %x between %x %x (type %d)\n",
          Marea, Marea->BaseAddress, (char*)Marea->BaseAddress + Marea->Length,
          Marea->Type);

   switch (Marea->Type)
   {
      case MEMORY_AREA_SECTION_VIEW:
         Status = MmUnmapViewOfSection(Process, Marea->BaseAddress);
         assert(Status == STATUS_SUCCESS);
         return(STATUS_SUCCESS);

      case MEMORY_AREA_VIRTUAL_MEMORY:
         MmFreeVirtualMemory(Process, Marea);
         break;

      case MEMORY_AREA_SHARED_DATA:
      case MEMORY_AREA_NO_ACCESS:
         Status = MmFreeMemoryArea(&Process->AddressSpace,
                                   Marea->BaseAddress,
                                   0,
                                   NULL,
                                   NULL);
         break;

      case MEMORY_AREA_MDL_MAPPING:
         KEBUGCHECK(PROCESS_HAS_LOCKED_PAGES);
         break;

      default:
         KEBUGCHECK(0);
   }

   return(STATUS_SUCCESS);
}

NTSTATUS MmReleaseMmInfo(PEPROCESS Process)
{
   PLIST_ENTRY CurrentEntry;
   PMEMORY_AREA Current;

   DPRINT("MmReleaseMmInfo(Process %x (%s))\n", Process,
          Process->ImageFileName);

   MmLockAddressSpace(&Process->AddressSpace);

   while(!IsListEmpty(&Process->AddressSpace.MAreaListHead))
   {
      CurrentEntry = Process->AddressSpace.MAreaListHead.Flink;
      Current = CONTAINING_RECORD(CurrentEntry, MEMORY_AREA, Entry);
      MmReleaseMemoryArea(Process, Current);
   }

   Mmi386ReleaseMmInfo(Process);

   MmUnlockAddressSpace(&Process->AddressSpace);
   MmDestroyAddressSpace(&Process->AddressSpace);

   DPRINT("Finished MmReleaseMmInfo()\n");
   return(STATUS_SUCCESS);
}

/*
 * @implemented
 */
BOOLEAN STDCALL MmIsNonPagedSystemAddressValid(PVOID VirtualAddress)
{
   return MmIsAddressValid(VirtualAddress);
}

/*
 * @implemented
 */
BOOLEAN STDCALL MmIsAddressValid(PVOID VirtualAddress)
/*
 * FUNCTION: Checks whether the given address is valid for a read or write
 * ARGUMENTS:
 *          VirtualAddress = address to check
 * RETURNS: True if the access would be valid
 *          False if the access would cause a page fault
 * NOTES: This function checks whether a byte access to the page would
 *        succeed. Is this realistic for RISC processors which don't
 *        allow byte granular access?
 */
{
   MEMORY_AREA* MemoryArea;
   PMADDRESS_SPACE AddressSpace;

   if ((ULONG)VirtualAddress >= KERNEL_BASE)
   {
      AddressSpace = MmGetKernelAddressSpace();
   }
   else
   {
      AddressSpace = &PsGetCurrentProcess()->AddressSpace;
   }

   MmLockAddressSpace(AddressSpace);
   MemoryArea = MmOpenMemoryAreaByAddress(AddressSpace,
                                          VirtualAddress);

   if (MemoryArea == NULL || MemoryArea->DeleteInProgress)
   {
      MmUnlockAddressSpace(AddressSpace);
      return(FALSE);
   }
   MmUnlockAddressSpace(AddressSpace);
   return(TRUE);
}

NTSTATUS MmAccessFault(KPROCESSOR_MODE Mode,
                       ULONG Address,
                       BOOLEAN FromMdl)
{
   PMADDRESS_SPACE AddressSpace;
   MEMORY_AREA* MemoryArea;
   NTSTATUS Status;
   BOOLEAN Locked = FromMdl;

   DPRINT("MmAccessFault(Mode %d, Address %x)\n", Mode, Address);

   if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
   {
      DbgPrint("Page fault at high IRQL was %d\n", KeGetCurrentIrql());
      return(STATUS_UNSUCCESSFUL);
   }
   if (PsGetCurrentProcess() == NULL)
   {
      DbgPrint("No current process\n");
      return(STATUS_UNSUCCESSFUL);
   }

   /*
    * Find the memory area for the faulting address
    */
   if (Address >= KERNEL_BASE)
   {
      /*
       * Check permissions
       */
      if (Mode != KernelMode)
      {
         DbgPrint("%s:%d\n",__FILE__,__LINE__);
         return(STATUS_UNSUCCESSFUL);
      }
      AddressSpace = MmGetKernelAddressSpace();
   }
   else
   {
      AddressSpace = &PsGetCurrentProcess()->AddressSpace;
   }

   if (!FromMdl)
   {
      MmLockAddressSpace(AddressSpace);
   }
   do
   {
      MemoryArea = MmOpenMemoryAreaByAddress(AddressSpace, (PVOID)Address);
      if (MemoryArea == NULL || MemoryArea->DeleteInProgress)
      {
         if (!FromMdl)
         {
            MmUnlockAddressSpace(AddressSpace);
         }
         return (STATUS_UNSUCCESSFUL);
      }

      switch (MemoryArea->Type)
      {
         case MEMORY_AREA_SYSTEM:
            Status = STATUS_UNSUCCESSFUL;
            break;

         case MEMORY_AREA_PAGED_POOL:
            Status = STATUS_SUCCESS;
            break;

         case MEMORY_AREA_SECTION_VIEW:
            Status = MmAccessFaultSectionView(AddressSpace,
                                              MemoryArea,
                                              (PVOID)Address,
                                              Locked);
            break;

         case MEMORY_AREA_VIRTUAL_MEMORY:
            Status = STATUS_UNSUCCESSFUL;
            break;

         case MEMORY_AREA_SHARED_DATA:
            Status = STATUS_UNSUCCESSFUL;
            break;

         default:
            Status = STATUS_UNSUCCESSFUL;
            break;
      }
   }
   while (Status == STATUS_MM_RESTART_OPERATION);

   DPRINT("Completed page fault handling\n");
   if (!FromMdl)
   {
      MmUnlockAddressSpace(AddressSpace);
   }
   return(Status);
}

NTSTATUS MmCommitPagedPoolAddress(PVOID Address, BOOLEAN Locked)
{
   NTSTATUS Status;
   PHYSICAL_ADDRESS AllocatedPage;
   Status = MmRequestPageMemoryConsumer(MC_PPOOL, FALSE, &AllocatedPage);
   if (!NT_SUCCESS(Status))
   {
      MmUnlockAddressSpace(MmGetKernelAddressSpace());
      Status = MmRequestPageMemoryConsumer(MC_PPOOL, TRUE, &AllocatedPage);
      MmLockAddressSpace(MmGetKernelAddressSpace());
   }
   Status =
      MmCreateVirtualMapping(NULL,
                             (PVOID)PAGE_ROUND_DOWN(Address),
                             PAGE_READWRITE,
                             AllocatedPage,
                             FALSE);
   if (!NT_SUCCESS(Status))
   {
      MmUnlockAddressSpace(MmGetKernelAddressSpace());
      Status =
         MmCreateVirtualMapping(NULL,
                                (PVOID)PAGE_ROUND_DOWN(Address),
                                PAGE_READWRITE,
                                AllocatedPage,
                                FALSE);
      MmLockAddressSpace(MmGetKernelAddressSpace());
   }
   if (Locked)
   {
      MmLockPage(AllocatedPage);
   }
   return(Status);
}

NTSTATUS MmNotPresentFault(KPROCESSOR_MODE Mode,
                           ULONG Address,
                           BOOLEAN FromMdl)
{
   PMADDRESS_SPACE AddressSpace;
   MEMORY_AREA* MemoryArea;
   NTSTATUS Status;
   BOOLEAN Locked = FromMdl;

   DPRINT("MmNotPresentFault(Mode %d, Address %x)\n", Mode, Address);

   if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
   {
      DbgPrint("Page fault at high IRQL was %d\n", KeGetCurrentIrql());
      return(STATUS_UNSUCCESSFUL);
   }
   if (PsGetCurrentProcess() == NULL)
   {
      DbgPrint("No current process\n");
      return(STATUS_UNSUCCESSFUL);
   }

   /*
    * Find the memory area for the faulting address
    */
   if (Address >= KERNEL_BASE)
   {
      /*
       * Check permissions
       */
      if (Mode != KernelMode)
      {
         DbgPrint("%s:%d\n",__FILE__,__LINE__);
         return(STATUS_UNSUCCESSFUL);
      }
      AddressSpace = MmGetKernelAddressSpace();
   }
   else
   {
      AddressSpace = &PsGetCurrentProcess()->AddressSpace;
   }

   if (!FromMdl)
   {
      MmLockAddressSpace(AddressSpace);
   }

   /*
    * Call the memory area specific fault handler
    */
   do
   {
      MemoryArea = MmOpenMemoryAreaByAddress(AddressSpace, (PVOID)Address);
      if (MemoryArea == NULL || MemoryArea->DeleteInProgress)
      {
         if (!FromMdl)
         {
            MmUnlockAddressSpace(AddressSpace);
         }
         return (STATUS_UNSUCCESSFUL);
      }

      switch (MemoryArea->Type)
      {
         case MEMORY_AREA_PAGED_POOL:
            {
               Status = MmCommitPagedPoolAddress((PVOID)Address, Locked);
               break;
            }

         case MEMORY_AREA_SYSTEM:
            Status = STATUS_UNSUCCESSFUL;
            break;

         case MEMORY_AREA_SECTION_VIEW:
            Status = MmNotPresentFaultSectionView(AddressSpace,
                                                  MemoryArea,
                                                  (PVOID)Address,
                                                  Locked);
            break;

         case MEMORY_AREA_VIRTUAL_MEMORY:
            Status = MmNotPresentFaultVirtualMemory(AddressSpace,
                                                    MemoryArea,
                                                    (PVOID)Address,
                                                    Locked);
            break;

         case MEMORY_AREA_SHARED_DATA:
            Status =
               MmCreateVirtualMapping(PsGetCurrentProcess(),
                                      (PVOID)PAGE_ROUND_DOWN(Address),
                                      PAGE_READONLY,
                                      MmSharedDataPagePhysicalAddress,
                                      FALSE);
            if (!NT_SUCCESS(Status))
            {
               MmUnlockAddressSpace(&PsGetCurrentProcess()->AddressSpace);
               Status =
                  MmCreateVirtualMapping(PsGetCurrentProcess(),
                                         (PVOID)PAGE_ROUND_DOWN(Address),
                                         PAGE_READONLY,
                                         MmSharedDataPagePhysicalAddress,
                                         TRUE);
               MmLockAddressSpace(&PsGetCurrentProcess()->AddressSpace);
            }
            break;

         default:
            Status = STATUS_UNSUCCESSFUL;
            break;
      }
   }
   while (Status == STATUS_MM_RESTART_OPERATION);

   DPRINT("Completed page fault handling\n");
   if (!FromMdl)
   {
      MmUnlockAddressSpace(AddressSpace);
   }
   return(Status);
}

/* Miscellanea functions: they may fit somewhere else */

/*
 * @unimplemented
 */
DWORD STDCALL
MmAdjustWorkingSetSize (DWORD Unknown0,
                        DWORD Unknown1,
                        DWORD Unknown2)
{
   UNIMPLEMENTED;
   return (0);
}


DWORD
STDCALL
MmDbgTranslatePhysicalAddress (
   DWORD Unknown0,
   DWORD Unknown1
)
{
   UNIMPLEMENTED;
   return (0);
}


/*
 * @unimplemented
 */
NTSTATUS
STDCALL
MmGrowKernelStack (
   DWORD Unknown0
)
{
   UNIMPLEMENTED;
   return (STATUS_NOT_IMPLEMENTED);
}


/*
 * @unimplemented
 */
BOOLEAN
STDCALL
MmSetAddressRangeModified (
   DWORD Unknown0,
   DWORD Unknown1
)
{
   UNIMPLEMENTED;
   return (FALSE);
}

/* EOF */
