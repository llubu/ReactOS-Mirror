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
/* $Id$
 *
 * COPYRIGHT:   See COPYING in the top directory
 * PROJECT:     ReactOS kernel 
 * FILE:        ntoskrnl/mm/rmap.c
 * PURPOSE:     kernel memory managment functions
 * PROGRAMMER:  David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *              Created 27/12/01
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* TYPES ********************************************************************/

typedef struct _MM_RMAP_ENTRY
{
   struct _MM_RMAP_ENTRY* Next;
   PEPROCESS Process;
   PVOID Address;
}
MM_RMAP_ENTRY, *PMM_RMAP_ENTRY;

#define TAG_RMAP    TAG('R', 'M', 'A', 'P')

/* GLOBALS ******************************************************************/

static FAST_MUTEX RmapListLock;
static NPAGED_LOOKASIDE_LIST RmapLookasideList;

/* FUNCTIONS ****************************************************************/

VOID INIT_FUNCTION
MmInitializeRmapList(VOID)
{
   ExInitializeFastMutex(&RmapListLock);
   ExInitializeNPagedLookasideList (&RmapLookasideList,
                                    NULL,
                                    NULL,
                                    0,
                                    sizeof(MM_RMAP_ENTRY),
                                    TAG_RMAP,
                                    50);
}

NTSTATUS
MmWritePagePhysicalAddress(PFN_TYPE Page)
{
   PMM_RMAP_ENTRY entry;
   PMEMORY_AREA MemoryArea;
   PMADDRESS_SPACE AddressSpace;
   ULONG Type;
   PVOID Address;
   PEPROCESS Process;
   PMM_PAGEOP PageOp;
   ULONG Offset;
   NTSTATUS Status = STATUS_SUCCESS;

   /*
    * Check that the address still has a valid rmap; then reference the
    * process so it isn't freed while we are working.
    */
   ExAcquireFastMutex(&RmapListLock);
   entry = MmGetRmapListHeadPage(Page);
   if (entry == NULL)
   {
      ExReleaseFastMutex(&RmapListLock);
      return(STATUS_UNSUCCESSFUL);
   }
   Process = entry->Process;
   Address = entry->Address;
   if ((((ULONG_PTR)Address) & 0xFFF) != 0)
   {
      KEBUGCHECK(0);
   }
   if (Address < (PVOID)KERNEL_BASE)
   {
      Status = ObReferenceObjectByPointer(Process, PROCESS_ALL_ACCESS, NULL, KernelMode);
      ExReleaseFastMutex(&RmapListLock);
      if (!NT_SUCCESS(Status))
      {
         return Status;
      }
      AddressSpace = &Process->AddressSpace;
   }
   else
   {
      ExReleaseFastMutex(&RmapListLock);
      AddressSpace = MmGetKernelAddressSpace();
   }

   /*
    * Lock the address space; then check that the address we are using
    * still corresponds to a valid memory area (the page might have been
    * freed or paged out after we read the rmap entry.) 
    */
   MmLockAddressSpace(AddressSpace);
   MemoryArea = MmOpenMemoryAreaByAddress(AddressSpace, Address);
   if (MemoryArea == NULL || MemoryArea->DeleteInProgress)
   {
      MmUnlockAddressSpace(AddressSpace);
      if (Address < (PVOID)KERNEL_BASE)
      {
         ObDereferenceObject(Process);
      }
      return(STATUS_UNSUCCESSFUL);
   }

   Type = MemoryArea->Type;
   if (Type == MEMORY_AREA_SECTION_VIEW)
   {
      Offset = (ULONG_PTR)Address - (ULONG_PTR)MemoryArea->StartingAddress;

      /*
       * Get or create a pageop
       */
      PageOp = MmGetPageOp(MemoryArea, 0, 0,
                           MemoryArea->Data.SectionData.Segment,
                           Offset, MM_PAGEOP_PAGEOUT, TRUE);

      if (PageOp == NULL)
      {
         MmUnlockAddressSpace(AddressSpace);
         if (Address < (PVOID)KERNEL_BASE)
         {
            ObDereferenceObject(Process);
         }
         return(STATUS_UNSUCCESSFUL);
      }

      /*
       * Release locks now we have a page op.
       */
      MmUnlockAddressSpace(AddressSpace);

      /*
       * Do the actual page out work.
       */
      Status = MmWritePageSectionView(AddressSpace, MemoryArea,
                                      Address, PageOp);
   }
   else if (Type == MEMORY_AREA_VIRTUAL_MEMORY)
   {
      PageOp = MmGetPageOp(MemoryArea, Address < (PVOID)KERNEL_BASE ? Process->UniqueProcessId : 0,
                           Address, NULL, 0, MM_PAGEOP_PAGEOUT, TRUE);

      if (PageOp == NULL)
      {
         MmUnlockAddressSpace(AddressSpace);
         if (Address < (PVOID)KERNEL_BASE)
         {
            ObDereferenceObject(Process);
         }
         return(STATUS_UNSUCCESSFUL);
      }

      /*
       * Release locks now we have a page op.
       */
      MmUnlockAddressSpace(AddressSpace);

      /*
       * Do the actual page out work.
       */
      Status = MmWritePageVirtualMemory(AddressSpace, MemoryArea,
                                        Address, PageOp);
   }
   else
   {
      KEBUGCHECK(0);
   }
   if (Address < (PVOID)KERNEL_BASE)
   {
      ObDereferenceObject(Process);
   }
   return(Status);
}

NTSTATUS
MmPageOutPhysicalAddress(PFN_TYPE Page)
{
   PMM_RMAP_ENTRY entry;
   PMEMORY_AREA MemoryArea;
   PMADDRESS_SPACE AddressSpace;
   ULONG Type;
   PVOID Address;
   PEPROCESS Process;
   PMM_PAGEOP PageOp;
   ULONG Offset;
   NTSTATUS Status = STATUS_SUCCESS;

   ExAcquireFastMutex(&RmapListLock);
   entry = MmGetRmapListHeadPage(Page);
   if (entry == NULL || MmGetLockCountPage(Page) != 0)
   {
      ExReleaseFastMutex(&RmapListLock);
      return(STATUS_UNSUCCESSFUL);
   }
   Process = entry->Process;
   Address = entry->Address;
   if ((((ULONG_PTR)Address) & 0xFFF) != 0)
   {
      KEBUGCHECK(0);
   }

   if (Address < (PVOID)KERNEL_BASE)
   {
      Status = ObReferenceObjectByPointer(Process, PROCESS_ALL_ACCESS, NULL, KernelMode);
      ExReleaseFastMutex(&RmapListLock);
      if (!NT_SUCCESS(Status))
      {
         return Status;
      }
      AddressSpace = &Process->AddressSpace;
   }
   else
   {
      ExReleaseFastMutex(&RmapListLock);
      AddressSpace = MmGetKernelAddressSpace();
   }

   MmLockAddressSpace(AddressSpace);
   MemoryArea = MmOpenMemoryAreaByAddress(AddressSpace, Address);
   if (MemoryArea == NULL || MemoryArea->DeleteInProgress)
   {
      MmUnlockAddressSpace(AddressSpace);
      if (Address < (PVOID)KERNEL_BASE)
      {
         ObDereferenceObject(Process);
      }
      return(STATUS_UNSUCCESSFUL);
   }
   Type = MemoryArea->Type;
   if (Type == MEMORY_AREA_SECTION_VIEW)
   {
      Offset = (ULONG_PTR)Address - (ULONG_PTR)MemoryArea->StartingAddress;

      /*
       * Get or create a pageop
       */
      PageOp = MmGetPageOp(MemoryArea, 0, 0,
                           MemoryArea->Data.SectionData.Segment,
                           Offset, MM_PAGEOP_PAGEOUT, TRUE);
      if (PageOp == NULL)
      {
         MmUnlockAddressSpace(AddressSpace);
         if (Address < (PVOID)KERNEL_BASE)
         {
            ObDereferenceObject(Process);
         }
         return(STATUS_UNSUCCESSFUL);
      }

      /*
       * Release locks now we have a page op.
       */
      MmUnlockAddressSpace(AddressSpace);

      /*
       * Do the actual page out work.
       */
      Status = MmPageOutSectionView(AddressSpace, MemoryArea,
                                    Address, PageOp);
   }
   else if (Type == MEMORY_AREA_VIRTUAL_MEMORY)
   {
      PageOp = MmGetPageOp(MemoryArea, Address < (PVOID)KERNEL_BASE ? Process->UniqueProcessId : 0,
                           Address, NULL, 0, MM_PAGEOP_PAGEOUT, TRUE);
      if (PageOp == NULL)
      {
         MmUnlockAddressSpace(AddressSpace);
         if (Address < (PVOID)KERNEL_BASE)
         {
            ObDereferenceObject(Process);
         }
         return(STATUS_UNSUCCESSFUL);
      }

      /*
       * Release locks now we have a page op.
       */
      MmUnlockAddressSpace(AddressSpace);

      /*
       * Do the actual page out work.
       */
      Status = MmPageOutVirtualMemory(AddressSpace, MemoryArea,
                                      Address, PageOp);
   }
   else
   {
      KEBUGCHECK(0);
   }
   if (Address < (PVOID)KERNEL_BASE)
   {
      ObDereferenceObject(Process);
   }
   return(Status);
}

VOID
MmSetCleanAllRmaps(PFN_TYPE Page)
{
   PMM_RMAP_ENTRY current_entry;

   ExAcquireFastMutex(&RmapListLock);
   current_entry = MmGetRmapListHeadPage(Page);
   if (current_entry == NULL)
   {
      DPRINT1("MmIsDirtyRmap: No rmaps.\n");
      KEBUGCHECK(0);
   }
   while (current_entry != NULL)
   {
      MmSetCleanPage(current_entry->Process, current_entry->Address);
      current_entry = current_entry->Next;
   }
   ExReleaseFastMutex(&RmapListLock);
}

VOID
MmSetDirtyAllRmaps(PFN_TYPE Page)
{
   PMM_RMAP_ENTRY current_entry;

   ExAcquireFastMutex(&RmapListLock);
   current_entry = MmGetRmapListHeadPage(Page);
   if (current_entry == NULL)
   {
      DPRINT1("MmIsDirtyRmap: No rmaps.\n");
      KEBUGCHECK(0);
   }
   while (current_entry != NULL)
   {
      MmSetDirtyPage(current_entry->Process, current_entry->Address);
      current_entry = current_entry->Next;
   }
   ExReleaseFastMutex(&RmapListLock);
}

BOOL
MmIsDirtyPageRmap(PFN_TYPE Page)
{
   PMM_RMAP_ENTRY current_entry;

   ExAcquireFastMutex(&RmapListLock);
   current_entry = MmGetRmapListHeadPage(Page);
   if (current_entry == NULL)
   {
      ExReleaseFastMutex(&RmapListLock);
      return(FALSE);
   }
   while (current_entry != NULL)
   {
      if (MmIsDirtyPage(current_entry->Process, current_entry->Address))
      {
         ExReleaseFastMutex(&RmapListLock);
         return(TRUE);
      }
      current_entry = current_entry->Next;
   }
   ExReleaseFastMutex(&RmapListLock);
   return(FALSE);
}

VOID
MmInsertRmap(PFN_TYPE Page, PEPROCESS Process,
             PVOID Address)
{
   PMM_RMAP_ENTRY current_entry;
   PMM_RMAP_ENTRY new_entry;
   ULONG PrevSize;

   Address = (PVOID)PAGE_ROUND_DOWN(Address);

   new_entry = ExAllocateFromNPagedLookasideList(&RmapLookasideList);
   if (new_entry == NULL)
   {
      KEBUGCHECK(0);
   }
   new_entry->Address = Address;
   new_entry->Process = Process;

   if (MmGetPfnForProcess(Process, Address) != Page)
   {
      DPRINT1("Insert rmap (%d, 0x%.8X) 0x%.8X which doesn't match physical "
              "address 0x%.8X\n", Process->UniqueProcessId, Address,
              MmGetPfnForProcess(Process, Address) << PAGE_SHIFT,
              Page << PAGE_SHIFT);
      KEBUGCHECK(0);
   }

   ExAcquireFastMutex(&RmapListLock);
   current_entry = MmGetRmapListHeadPage(Page);
   new_entry->Next = current_entry;
   MmSetRmapListHeadPage(Page, new_entry);
   ExReleaseFastMutex(&RmapListLock);
   if (Process == NULL)
   {
      Process = PsInitialSystemProcess;
   }
   if (Process)
   {
      PrevSize = InterlockedExchangeAddUL(&Process->Vm.WorkingSetSize, PAGE_SIZE);
      if (PrevSize >= Process->Vm.PeakWorkingSetSize)
      {
         Process->Vm.PeakWorkingSetSize = PrevSize + PAGE_SIZE;
      }
   }
}

VOID
MmDeleteAllRmaps(PFN_TYPE Page, PVOID Context,
                 VOID (*DeleteMapping)(PVOID Context, PEPROCESS Process,
                                       PVOID Address))
{
   PMM_RMAP_ENTRY current_entry;
   PMM_RMAP_ENTRY previous_entry;
   PEPROCESS Process;

   ExAcquireFastMutex(&RmapListLock);
   current_entry = MmGetRmapListHeadPage(Page);
   if (current_entry == NULL)
   {
      DPRINT1("MmDeleteAllRmaps: No rmaps.\n");
      KEBUGCHECK(0);
   }
   MmSetRmapListHeadPage(Page, NULL);
   while (current_entry != NULL)
   {
      previous_entry = current_entry;
      current_entry = current_entry->Next;
      if (DeleteMapping)
      {
         DeleteMapping(Context, previous_entry->Process,
                       previous_entry->Address);
      }
      Process = previous_entry->Process;
      ExFreeToNPagedLookasideList(&RmapLookasideList, previous_entry);
      if (Process == NULL)
      {
         Process = PsInitialSystemProcess;
      }
      if (Process)
      {
         InterlockedExchangeAddUL(&Process->Vm.WorkingSetSize, -PAGE_SIZE);
      }
   }
   ExReleaseFastMutex(&RmapListLock);
}

VOID
MmDeleteRmap(PFN_TYPE Page, PEPROCESS Process,
             PVOID Address)
{
   PMM_RMAP_ENTRY current_entry, previous_entry;

   ExAcquireFastMutex(&RmapListLock);
   previous_entry = NULL;
   current_entry = MmGetRmapListHeadPage(Page);
   while (current_entry != NULL)
   {
      if (current_entry->Process == Process &&
            current_entry->Address == Address)
      {
         if (previous_entry == NULL)
         {
            MmSetRmapListHeadPage(Page, current_entry->Next);
         }
         else
         {
            previous_entry->Next = current_entry->Next;
         }
         ExReleaseFastMutex(&RmapListLock);
         ExFreeToNPagedLookasideList(&RmapLookasideList, current_entry);
	 if (Process == NULL)
	 {
	    Process = PsInitialSystemProcess;
	 }
	 if (Process)
	 {
            InterlockedExchangeAddUL(&Process->Vm.WorkingSetSize, -PAGE_SIZE);
	 }
         return;
      }
      previous_entry = current_entry;
      current_entry = current_entry->Next;
   }
   KEBUGCHECK(0);
}
