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
/* $Id: rmap.c,v 1.20 2003/07/15 19:31:27 hbirr Exp $
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

#include <ddk/ntddk.h>
#include <internal/mm.h>
#include <internal/ps.h>

#define NDEBUG
#include <internal/debug.h>

/* TYPES ********************************************************************/

typedef struct _MM_RMAP_ENTRY
{
  struct _MM_RMAP_ENTRY* Next;
  PEPROCESS Process;
  PVOID Address;
} MM_RMAP_ENTRY, *PMM_RMAP_ENTRY;

#define TAG_RMAP    TAG('R', 'M', 'A', 'P')

/* GLOBALS ******************************************************************/

static FAST_MUTEX RmapListLock;
static NPAGED_LOOKASIDE_LIST RmapLookasideList;

/* FUNCTIONS ****************************************************************/

VOID
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
MmWritePagePhysicalAddress(PHYSICAL_ADDRESS PhysicalAddress)
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
  entry = MmGetRmapListHeadPage(PhysicalAddress);
  if (entry == NULL)
    {
      ExReleaseFastMutex(&RmapListLock);
      return(STATUS_UNSUCCESSFUL);
    }
  Process = entry->Process;
  Address = entry->Address;
  if ((((ULONG)Address) & 0xFFF) != 0)
    {
      KeBugCheck(0);
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
      Offset = (ULONG)(Address - (ULONG)MemoryArea->BaseAddress);

      /*
       * Get or create a pageop
       */
      PageOp = MmGetPageOp(MemoryArea, 0, 0, 
			   MemoryArea->Data.SectionData.Segment, 
			   Offset, MM_PAGEOP_PAGEOUT);
      if (PageOp == NULL)
	{
	  DPRINT1("MmGetPageOp failed\n");
	  KeBugCheck(0);
	}


      if (PageOp->Thread != PsGetCurrentThread())
	{
	  MmUnlockAddressSpace(AddressSpace);
          Status = KeWaitForSingleObject(&PageOp->CompletionEvent,
				         0,
				         KernelMode,
				         FALSE,
				         NULL);
          KeSetEvent(&PageOp->CompletionEvent, IO_NO_INCREMENT, FALSE);
	  MmReleasePageOp(PageOp);
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
			   Address, NULL, 0, MM_PAGEOP_PAGEOUT);
      
      if (PageOp->Thread != PsGetCurrentThread())
	{
	  MmReleasePageOp(PageOp);
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
      KeBugCheck(0);
    }  
  if (Address < (PVOID)KERNEL_BASE)
    {
      ObDereferenceObject(Process);
    }
  return(Status);
}

NTSTATUS
MmPageOutPhysicalAddress(PHYSICAL_ADDRESS PhysicalAddress)
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
  entry = MmGetRmapListHeadPage(PhysicalAddress);
  if (entry == NULL)
    {
      ExReleaseFastMutex(&RmapListLock);
      return(STATUS_UNSUCCESSFUL);
    }
  Process = entry->Process;
  Address = entry->Address;
  if ((((ULONG)Address) & 0xFFF) != 0)
    {
      KeBugCheck(0);
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
      Offset = (ULONG)(Address - (ULONG)MemoryArea->BaseAddress);

      /*
       * Get or create a pageop
       */
      PageOp = MmGetPageOp(MemoryArea, 0, 0, 
			   MemoryArea->Data.SectionData.Segment, 
			   Offset, MM_PAGEOP_PAGEOUT);
      if (PageOp == NULL)
	{
	  DPRINT1("MmGetPageOp failed\n");
	  KeBugCheck(0);
	}

      if (PageOp->Thread != PsGetCurrentThread())
	{
	  MmReleasePageOp(PageOp);
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
  else if (Type == MEMORY_AREA_VIRTUAL_MEMORY || 
           Type == MEMORY_AREA_CACHE_SEGMENT)
    {
      PageOp = MmGetPageOp(MemoryArea, Address < (PVOID)KERNEL_BASE ? Process->UniqueProcessId : 0,
			   Address, NULL, 0, MM_PAGEOP_PAGEOUT);
      if (PageOp->Thread != PsGetCurrentThread())
	{
	  MmUnlockAddressSpace(AddressSpace);
          Status = KeWaitForSingleObject(&PageOp->CompletionEvent,
				         0,
				         KernelMode,
				         FALSE,
				         NULL);
          KeSetEvent(&PageOp->CompletionEvent, IO_NO_INCREMENT, FALSE);
	  MmReleasePageOp(PageOp);
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
      KeBugCheck(0);
    }
  if (Address < (PVOID)KERNEL_BASE)
    {
      ObDereferenceObject(Process);
    }
  return(Status);
}

VOID
MmSetCleanAllRmaps(PHYSICAL_ADDRESS PhysicalAddress)
{
  PMM_RMAP_ENTRY current_entry;

  ExAcquireFastMutex(&RmapListLock);
  current_entry = MmGetRmapListHeadPage(PhysicalAddress);
  if (current_entry == NULL)
    {
      DPRINT1("MmIsDirtyRmap: No rmaps.\n");
      KeBugCheck(0);
    }
  while (current_entry != NULL)
    {      
      MmSetCleanPage(current_entry->Process, current_entry->Address);
      current_entry = current_entry->Next;
    }
  ExReleaseFastMutex(&RmapListLock);
}

VOID
MmSetDirtyAllRmaps(PHYSICAL_ADDRESS PhysicalAddress)
{
  PMM_RMAP_ENTRY current_entry;

  ExAcquireFastMutex(&RmapListLock);
  current_entry = MmGetRmapListHeadPage(PhysicalAddress);
  if (current_entry == NULL)
    {
      DPRINT1("MmIsDirtyRmap: No rmaps.\n");
      KeBugCheck(0);
    }
  while (current_entry != NULL)
    {      
      MmSetDirtyPage(current_entry->Process, current_entry->Address);
      current_entry = current_entry->Next;
    }
  ExReleaseFastMutex(&RmapListLock);
}

BOOL
MmIsDirtyPageRmap(PHYSICAL_ADDRESS PhysicalAddress)
{
  PMM_RMAP_ENTRY current_entry;

  ExAcquireFastMutex(&RmapListLock);
  current_entry = MmGetRmapListHeadPage(PhysicalAddress);
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
MmInsertRmap(PHYSICAL_ADDRESS PhysicalAddress, PEPROCESS Process, 
	     PVOID Address)
{
  PMM_RMAP_ENTRY current_entry;
  PMM_RMAP_ENTRY new_entry;

  Address = (PVOID)PAGE_ROUND_DOWN(Address);

  new_entry = ExAllocateFromNPagedLookasideList(&RmapLookasideList);
  if (new_entry == NULL)
    {
      KeBugCheck(0);
    }
  new_entry->Address = Address;
  new_entry->Process = Process;

  if (MmGetPhysicalAddressForProcess(Process, Address).QuadPart != 
      PhysicalAddress.QuadPart)
    {
      DPRINT1("Insert rmap (%d, 0x%.8X) 0x%.8X which doesn't match physical "
	      "address 0x%.8X\n", Process->UniqueProcessId, Address, 
	      MmGetPhysicalAddressForProcess(Process, Address), 
	      PhysicalAddress)
      KeBugCheck(0);
    }

  ExAcquireFastMutex(&RmapListLock);
  current_entry = MmGetRmapListHeadPage(PhysicalAddress);
  new_entry->Next = current_entry;
  MmSetRmapListHeadPage(PhysicalAddress, new_entry);
  ExReleaseFastMutex(&RmapListLock);
}

VOID
MmDeleteAllRmaps(PHYSICAL_ADDRESS PhysicalAddress, PVOID Context, 
		 VOID (*DeleteMapping)(PVOID Context, PEPROCESS Process, 
				       PVOID Address))
{
  PMM_RMAP_ENTRY current_entry;
  PMM_RMAP_ENTRY previous_entry;

  ExAcquireFastMutex(&RmapListLock);
  current_entry = MmGetRmapListHeadPage(PhysicalAddress);
  if (current_entry == NULL)
    {
      DPRINT1("MmDeleteAllRmaps: No rmaps.\n");
      KeBugCheck(0);
    }
  MmSetRmapListHeadPage(PhysicalAddress, NULL);
  while (current_entry != NULL)
    {
      previous_entry = current_entry;
      current_entry = current_entry->Next;
      if (DeleteMapping)
	{
	  DeleteMapping(Context, previous_entry->Process, 
			previous_entry->Address);
	}
      ExFreeToNPagedLookasideList(&RmapLookasideList, previous_entry);
    }
  ExReleaseFastMutex(&RmapListLock);
}

VOID
MmDeleteRmap(PHYSICAL_ADDRESS PhysicalAddress, PEPROCESS Process, 
	     PVOID Address)
{
  PMM_RMAP_ENTRY current_entry, previous_entry;

  ExAcquireFastMutex(&RmapListLock);
  previous_entry = NULL;
  current_entry = MmGetRmapListHeadPage(PhysicalAddress);
  while (current_entry != NULL)
    {
      if (current_entry->Process == Process && 
	  current_entry->Address == Address)
	{
	  if (previous_entry == NULL)
	    {
	      MmSetRmapListHeadPage(PhysicalAddress, current_entry->Next);
	    }
	  else
	    {
	      previous_entry->Next = current_entry->Next;
	    }
	  ExReleaseFastMutex(&RmapListLock);
	  ExFreeToNPagedLookasideList(&RmapLookasideList, current_entry);
	  return;
	}
      previous_entry = current_entry;
      current_entry = current_entry->Next;
    }
  KeBugCheck(0);
}
