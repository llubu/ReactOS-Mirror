/* $Id: virtual.c,v 1.38 2001/02/10 22:51:10 dwelch Exp $
 *
 * COPYRIGHT:   See COPYING in the top directory
 * PROJECT:     ReactOS kernel
 * FILE:        ntoskrnl/mm/virtual.c
 * PURPOSE:     implementing the Virtualxxx section of the win32 api
 * PROGRAMMER:  David Welch
 * UPDATE HISTORY:
 *              09/4/98: Created
 *              10/6/98: Corrections from Fatahi (i_fatahi@hotmail.com)
 *              30/9/98: Implemented ZwxxxVirtualMemory functions
 */
 
/* INCLUDE *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/mm.h>
#include <internal/mmhal.h>
#include <internal/ob.h>
#include <internal/io.h>
#include <internal/ps.h>

#define NDEBUG
#include <internal/debug.h>

/* TYPES *********************************************************************/

typedef struct _MM_SEGMENT
{
   ULONG Type;
   ULONG Protect;
   ULONG Length;
   LIST_ENTRY SegmentListEntry;
} MM_SEGMENT, *PMM_SEGMENT;

/* FUNCTIONS *****************************************************************/

PMM_SEGMENT 
MmGetSegmentForAddress(PMEMORY_AREA MArea,
		       PVOID Address,
		       PVOID* PCurrentAddress)
/*
 * FUNCTION: Get the segment corresponding to a particular memory area and
 * address. 
 * ARGUMENTS:
 *          MArea (IN) = The memory area
 *          Address (IN) = The address to get the segment for
 *          PCurrentAddress (OUT) = The start of the segment
 * RETURNS:
 *          The corresponding segment or NULL if an error occurred
 */
{
   PVOID CurrentAddress;
   PMM_SEGMENT CurrentSegment;
   PLIST_ENTRY Current;
   
   if (Address < MArea->BaseAddress ||
       Address >= (MArea->BaseAddress + MArea->Length))
     {
	KeBugCheck(0);
	*PCurrentAddress = NULL;
	return(NULL);
     }
   
   Current = MArea->Data.VirtualMemoryData.SegmentListHead.Flink;
   CurrentAddress = MArea->BaseAddress;
   while (Current != &MArea->Data.VirtualMemoryData.SegmentListHead)
     {
	CurrentSegment = CONTAINING_RECORD(Current, 
					   MM_SEGMENT,
					   SegmentListEntry);
	if (Address >= CurrentAddress &&
	    Address < (CurrentAddress + CurrentSegment->Length))
	  {
	     *PCurrentAddress = CurrentAddress;
	     return(CurrentSegment);
	  }
	CurrentAddress = CurrentAddress + CurrentSegment->Length;
	Current = Current->Flink;
     }
   KeBugCheck(0);
   return(NULL);
}

NTSTATUS 
MmWritePageVirtualMemory(PMADDRESS_SPACE AddressSpace,
			 PMEMORY_AREA MArea,
			 PVOID Address)
{
   SWAPENTRY se;
   ULONG Flags;
   PHYSICAL_ADDRESS PhysicalAddress;
   PMDL Mdl;
   NTSTATUS Status;
   
   /*
    * FIXME: What should we do if an i/o operation is pending on
    * this page
    */
   
   /*
    * If the memory area is readonly then there is nothing to do
    */
   if (MArea->Attributes & PAGE_READONLY ||
       MArea->Attributes & PAGE_EXECUTE_READ)
     {
	return(STATUS_SUCCESS);
     }
   /*
    * Set the page to readonly. This ensures the current contents aren't
    * modified while we are writing it to swap.
    */
   MmSetPageProtect(AddressSpace->Process,
		    Address,
		    PAGE_READONLY);
   /*
    * If the page isn't dirty then there is nothing to do.
    */
   if (!MmIsPageDirty(AddressSpace->Process, Address))
     {
	MmSetPageProtect(AddressSpace->Process,
			 Address,
			 MArea->Attributes);
	return(STATUS_SUCCESS);	
     }
   PhysicalAddress = MmGetPhysicalAddress(Address);
   /*
    * If we haven't already allocated a swap entry for this page
    * then allocate one
    */
   if ((se = MmGetSavedSwapEntryPage((PVOID)PhysicalAddress.u.LowPart)) != 0)
     {
	se = MmAllocSwapPage();
	if (se == 0)
	  {
	     MmSetPageProtect(AddressSpace->Process,
			      Address,
			      MArea->Attributes);
	     return(STATUS_UNSUCCESSFUL);
	  }
	MmSetSavedSwapEntryPage((PVOID)PhysicalAddress.u.LowPart, se);
     }
   /*
    * Set the flags so other threads will know what we are doing
    */
   Flags = MmGetFlagsPage((PVOID)PhysicalAddress.u.LowPart);
   Flags = Flags | MM_PHYSICAL_PAGE_MPW_PENDING;
   MmSetFlagsPage((PVOID)PhysicalAddress.u.LowPart, Flags);
   /*
    * Build an mdl to hold the page for writeout
    */
   Mdl = MmCreateMdl(NULL, NULL, PAGESIZE);
   MmBuildMdlFromPages(Mdl, (PULONG)&PhysicalAddress.u.LowPart);
   /*
    * Unlock the address space and write out the page to swap.
    */
   MmUnlockAddressSpace(AddressSpace);
   Status = MmWriteToSwapPage(se, Mdl);
   /*
    * Cleanup 
    */
   MmLockAddressSpace(AddressSpace);
   Flags = MmGetFlagsPage((PVOID)PhysicalAddress.u.LowPart);
   Flags = Flags & (~MM_PHYSICAL_PAGE_MPW_PENDING);
   MmSetFlagsPage((PVOID)PhysicalAddress.u.LowPart,Flags);
   /*
    * If we successfully wrote the page then reset the dirty bit
    */
   if (NT_SUCCESS(Status))
     {
	MmSetCleanPage(AddressSpace->Process, Address);
     }
   return(Status);
}


ULONG MmPageOutVirtualMemory(PMADDRESS_SPACE AddressSpace,
			     PMEMORY_AREA MemoryArea,
			     PVOID Address,
			     PBOOLEAN Ul)
{
   PHYSICAL_ADDRESS PhysicalAddress;
   
   if ((MemoryArea->Attributes & PAGE_READONLY) ||
       (MemoryArea->Attributes & PAGE_EXECUTE_READ) ||
       !MmIsPageDirty(PsGetCurrentProcess(), Address))
     {
	PhysicalAddress = MmGetPhysicalAddress(Address);
	
	MmRemovePageFromWorkingSet(AddressSpace->Process,
				   Address);
	MmDeleteVirtualMapping(PsGetCurrentProcess(), Address, FALSE);
	MmDereferencePage((PVOID)PhysicalAddress.u.LowPart);
	*Ul = TRUE;
	return(1);
     }
   *Ul = FALSE;
   return(0);     
}

NTSTATUS
MmNotPresentFaultVirtualMemory(PMADDRESS_SPACE AddressSpace,
			       MEMORY_AREA* MemoryArea, 
			       PVOID Address)
/*
 * FUNCTION: Move data into memory to satisfy a page not present fault
 * ARGUMENTS:
 *      AddressSpace = Address space within which the fault occurred
 *      MemoryArea = The memory area within which the fault occurred
 *      Address = The absolute address of fault
 * RETURNS: Status
 * NOTES: This function is called with the address space lock held.
 */
{
   PVOID Page;
   NTSTATUS Status;
   PMM_SEGMENT Segment;
   PVOID CurrentAddress;
   
   /*
    * Get the segment corresponding to the virtual address
    */
   Segment = MmGetSegmentForAddress(MemoryArea, Address, &CurrentAddress);
   if (Segment == NULL)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   if (Segment->Type == MEM_RESERVE)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   
   if (MmIsPagePresent(NULL, Address))
     {	
	return(STATUS_SUCCESS);
     }
   
   /*
    * Try to allocate a page
    */
   Page = MmAllocPage(0);
   while (Page == NULL)
     {
	MmUnlockAddressSpace(AddressSpace);
	MmWaitForFreePages();
	MmLockAddressSpace(AddressSpace);
	if (MmIsPagePresent(NULL, Address))
	  {
	     return(STATUS_SUCCESS);
	  }
	Page = MmAllocPage(0);
     }
   
   /*
    * Add the page to the process's working set
    */
   MmAddPageToWorkingSet(PsGetCurrentProcess(), Address);
   
   /*
    * Set the page. If we fail because we are out of memory then
    * try again
    */
   Status = MmCreateVirtualMapping(PsGetCurrentProcess(),		      
				   Address,
				   MemoryArea->Attributes,
				   (ULONG)Page);
   while (Status == STATUS_NO_MEMORY)
     {
	MmUnlockAddressSpace(AddressSpace);
	MmWaitForFreePages();
	MmLockAddressSpace(AddressSpace);
	if (MmIsPagePresent(NULL, Address))
	  {
	     MmDereferencePage(Page);
	     return(STATUS_SUCCESS);
	  }
	Status = MmCreateVirtualMapping(PsGetCurrentProcess(),
					Address,
					MemoryArea->Attributes,
					(ULONG)Page);
     }
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   return(STATUS_SUCCESS);
}

VOID STATIC
MmModifyAttributes(PMADDRESS_SPACE AddressSpace,
		   PVOID BaseAddress,
		   ULONG RegionSize,
		   ULONG OldType,
		   ULONG OldProtect,
		   ULONG NewType,
		   ULONG NewProtect)
/*
 * FUNCTION: Modify the attributes of a memory region
 */
{      
  /*
   * If we are switching a previously committed region to reserved then
   * free any allocated pages within the region
   */
  if (NewType == MEM_RESERVE && OldType == MEM_COMMIT)
    {
      ULONG i;
      
      for (i=0; i <= (RegionSize/PAGESIZE); i++)
	{
	  LARGE_INTEGER PhysicalAddr;
	  
	  PhysicalAddr = MmGetPhysicalAddress(BaseAddress + (i*PAGESIZE));
	  if (PhysicalAddr.u.LowPart != 0)
	    {
	      MmRemovePageFromWorkingSet(AddressSpace->Process,
					 BaseAddress + (i*PAGESIZE));
	      MmDereferencePage((PVOID)(ULONG)(PhysicalAddr.u.LowPart));
	    }
	  MmDeleteVirtualMapping(AddressSpace->Process,
				 BaseAddress + (i*PAGESIZE),
				 FALSE);
	}
    }

  /*
   * If we are changing the protection attributes of a committed region then
   * alter the attributes for any allocated pages within the region
   */
  if (NewType == MEM_COMMIT && OldType == MEM_COMMIT && 
      OldProtect != NewProtect)
    {
      ULONG i;
   
      for (i=0; i <= (RegionSize/PAGESIZE); i++)
	{
	  if (MmIsPagePresent(AddressSpace->Process, 
			      BaseAddress + (i*PAGESIZE)))
	    {
	      MmSetPageProtect(AddressSpace->Process,
			       BaseAddress + (i*PAGESIZE), 
			       NewProtect);
	    }
	}
    }
}

VOID STATIC 
InsertAfterEntry(PLIST_ENTRY Previous,
		 PLIST_ENTRY Entry)
/*
 * FUNCTION: Insert a list entry after another entry in the list
 */
{
   Previous->Flink->Blink = Entry;
   
   Entry->Flink = Previous->Flink;
   Entry->Blink = Previous;
   
   Previous->Flink = Entry;
}

#if 0
VOID STATIC
MmDumpSegmentsMemoryArea(PMEMORY_AREA MemoryArea)
{
   PVOID CurrentAddress;
   PLIST_ENTRY CurrentEntry;
   PMM_SEGMENT CurrentSegment;
   PLIST_ENTRY ListHead;
   
   CurrentEntry = MemoryArea->Data.VirtualMemoryData.SegmentListHead.Flink;
   ListHead = &MemoryArea->Data.VirtualMemoryData.SegmentListHead;
   
   CurrentAddress = MemoryArea->BaseAddress;
   while (CurrentEntry != ListHead)
     {
	CurrentSegment = CONTAINING_RECORD(CurrentEntry,
					   MM_SEGMENT,
					   SegmentListEntry);
	
	DbgPrint("0x%x 0x%x %d %d\n", 
		 CurrentAddress,
		 CurrentSegment->Length,
		 CurrentSegment->Type,
		 CurrentSegment->Protect);
	
	CurrentAddress = CurrentAddress + CurrentSegment->Length;
	CurrentEntry = CurrentEntry->Flink;
     }
}
#endif

NTSTATUS 
MmSplitSegment(PMADDRESS_SPACE AddressSpace,
	       PMEMORY_AREA MemoryArea,
	       PVOID RegionAddress,
	       ULONG RegionLength,
	       ULONG Type,
	       ULONG Protect,
	       PMM_SEGMENT FirstSegment,
	       PVOID FirstAddress)
/*
 * FUNCTION: Split a memory segment internally 
 */
{
   PMM_SEGMENT NewTopSegment;
   PMM_SEGMENT RegionSegment;
   ULONG OldType;
   ULONG OldProtect;
   ULONG OldLength;
   
   /*
    * Save the type and protection and length of the current segment
    */
   OldType = FirstSegment->Type;
   OldProtect = FirstSegment->Protect;
   OldLength = FirstSegment->Length;

   /*
    * If the segment is already of the right type and protection then
    * there is nothing to do.
    */
   if (FirstSegment->Type == Type && FirstSegment->Protect == Protect)
     {       
       return(STATUS_SUCCESS);
     }
   
   /*
    * Allocate the segment we might need here because if the allocation
    * fails below it will be difficult to undo what we've done already.
    */
   NewTopSegment = ExAllocatePool(NonPagedPool, sizeof(MM_SEGMENT));
   if (NewTopSegment == NULL)
     {
	return(STATUS_NO_MEMORY);
     }
   
   if (FirstAddress < RegionAddress)
     {
	/*
	 * If the region to be affected starts at a higher address than
	 * the current segment then create a new segment for the
	 * affected portion
	 */	
	RegionSegment = ExAllocatePool(NonPagedPool, sizeof(MM_SEGMENT));
	if (RegionSegment == NULL)
	  {
	     ExFreePool(NewTopSegment);
	     return(STATUS_NO_MEMORY);
	  }
	
	RegionSegment->Type = Type;
	RegionSegment->Protect = Protect;
	RegionSegment->Length = RegionLength;
	
	FirstSegment->Length = RegionAddress - FirstAddress;

	InsertAfterEntry(&FirstSegment->SegmentListEntry,
			 &RegionSegment->SegmentListEntry);
     }
   else
     {
	/*
	 * Otherwise just set its type and protection and length
	 */
	
	FirstSegment->Type = Type;
	FirstSegment->Protect = Protect;
	FirstSegment->Length = RegionLength;
	
        RegionSegment = FirstSegment;
     }
   
   if ((FirstAddress + OldLength) > (RegionAddress + RegionLength))
     {
	/*
	 * If the top of the current segment extends after the affected
	 * region then create a segment for the unaffected portion
	 */
	
	NewTopSegment->Type = OldType;
	NewTopSegment->Protect = OldProtect;
	NewTopSegment->Length = (FirstAddress + OldLength) -
	  (RegionAddress + RegionLength);
	
	InsertAfterEntry(&RegionSegment->SegmentListEntry,
			 &NewTopSegment->SegmentListEntry);
     }
   else
     {
	ExFreePool(NewTopSegment);
	NewTopSegment = NULL;
     }
   
   /*
    * Actually set the type and protection of the affected region
    */
   MmModifyAttributes(AddressSpace, 
		      RegionAddress, 
		      RegionLength, 
		      OldType, 
		      OldProtect, 
		      Type, 
		      Protect);
   return(STATUS_SUCCESS);
}

NTSTATUS MmGatherSegment(PMADDRESS_SPACE AddressSpace,
			 PMEMORY_AREA MemoryArea,
			 PVOID RegionAddress,
			 ULONG RegionLength,
			 ULONG Type,
			 ULONG Protect,
			 PMM_SEGMENT FirstSegment,
			 PVOID FirstAddress)
/*
 * FUNCTION: Do a virtual memory operation that will effect several
 * memory segments. 
 * ARGUMENTS:
 *          AddressSpace (IN) = Address space to affect
 *          MemoryArea (IN) = Memory area to affect
 *          BaseAddress (IN) = Base address of the region to affect
 *          RegionSize (IN) = Size of the region to affect
 *          Type (IN) = New type of the region
 *          Protect (IN) = New protection of the region
 *          CurrentSegment (IN) = First segment intersecting with the region
 *          CurrentAddress (IN) = Start address of the first segment
 *                                interesting with the region
 * RETURNS: Status
 */
{
   PMM_SEGMENT RegionSegment;
   PVOID CurrentAddress;
   ULONG RemainingLength;
   PLIST_ENTRY CurrentEntry;
   PLIST_ENTRY ListHead;
   PMM_SEGMENT CurrentSegment;
   
   if (FirstAddress < RegionAddress)
     {
	/*
	 * If a portion of the first segment is not covered by the region then
	 * we need to split it into two segments
	 */
	
	RegionSegment = ExAllocatePool(NonPagedPool, sizeof(MM_SEGMENT));
	if (RegionSegment == NULL)
	  {
	     return(STATUS_NO_MEMORY);
	  }
	
	RegionSegment->Type = Type;
	RegionSegment->Protect = Protect;
	RegionSegment->Length = (FirstAddress + FirstSegment->Length) -
	  RegionAddress;
	
	FirstSegment->Length = RegionAddress - FirstAddress;

	InsertAfterEntry(&FirstSegment->SegmentListEntry,
			 &RegionSegment->SegmentListEntry);
	
	MmModifyAttributes(AddressSpace, 
			   RegionAddress, 
			   RegionSegment->Length,
			   FirstSegment->Type, 
			   FirstSegment->Protect, 
			   Type, 
			   Protect);
	
	CurrentAddress = FirstAddress + FirstSegment->Length +
	  RegionSegment->Length;
     }
   else
     {
	/*
	 *  Otherwise just change the attributes of the segment
	 */
	
	ULONG OldType;
	ULONG OldProtect;		
	
	OldType = FirstSegment->Type;
	OldProtect = FirstSegment->Protect;
	
	FirstSegment->Type = Type;
	FirstSegment->Protect = Protect;
	
	RegionSegment = FirstSegment;
	
	MmModifyAttributes(AddressSpace, 
			   RegionAddress, 
			   FirstSegment->Length,
			   OldType, 
			   OldProtect, 
			   Type, 
			   Protect);
	
	CurrentAddress = FirstAddress + RegionSegment->Length;
     }
  
   /*
    * Change the attributes of all the complete segments lying inside the
    * affected region
    */   
   RemainingLength = RegionLength - RegionSegment->Length;
   CurrentEntry = RegionSegment->SegmentListEntry.Flink;
   CurrentSegment = CONTAINING_RECORD(CurrentEntry,
				      MM_SEGMENT,
				      SegmentListEntry);
   ListHead = &MemoryArea->Data.VirtualMemoryData.SegmentListHead;
   
   while (CurrentEntry != ListHead && RemainingLength > 0)
     {
	ULONG OldType;
	ULONG OldProtect;
	ULONG OldLength;
			
	/*
	 * If this segment will not be completely covered by the
	 * affected region then break
	 */
	if (CurrentSegment->Length > RemainingLength)
	  {
	     break;
	  }
	
	OldType = CurrentSegment->Type;
	OldProtect = CurrentSegment->Protect;
	OldLength = CurrentSegment->Length;
	
	/*
	 * Extend the length of the previous segment to cover this one
	 */
	RegionSegment->Length = RegionSegment->Length + OldLength;
	RemainingLength = RemainingLength - OldLength;
	CurrentAddress = CurrentAddress + OldLength;
	CurrentEntry = CurrentEntry->Flink;
	
	/*
	 * Remove the current segment from the list
	 */
	RemoveEntryList(&CurrentSegment->SegmentListEntry);
	ExFreePool(CurrentSegment);
	
	MmModifyAttributes(AddressSpace, 
			   CurrentAddress, 
			   OldLength,
			   OldType, 
			   OldProtect, 
			   Type, 
			   Protect);
	
	CurrentSegment = CONTAINING_RECORD(CurrentEntry,
					   MM_SEGMENT,
					   SegmentListEntry);
     }
   
   /*
    * If we've run off the top of the memory area then bug check
    */
   if (CurrentEntry == ListHead && RemainingLength > 0)
     {
	KeBugCheck(0);
     }
   
   /*
    * We've only affected a portion of a segment then split it in two
    */
   if (RemainingLength > 0)
     {
	CurrentSegment->Length = CurrentSegment->Length - RemainingLength;

	RegionSegment->Length = RegionSegment->Length + RemainingLength;
	
	MmModifyAttributes(AddressSpace, 
			   CurrentAddress, 
			   RemainingLength,
			   CurrentSegment->Type, 
			   CurrentSegment->Protect, 
			   Type, 
			   Protect);
     }
   
   return(STATUS_SUCCESS);
}

NTSTATUS MmComplexVirtualMemoryOperation(PMADDRESS_SPACE AddressSpace,
					 PMEMORY_AREA MemoryArea,
					 PVOID BaseAddress,
					 ULONG RegionSize,
					 ULONG Type,
					 ULONG Protect)
{
   PMM_SEGMENT CurrentSegment;
   PVOID CurrentAddress;
   
   CurrentSegment = MmGetSegmentForAddress(MemoryArea, 
					   BaseAddress, 
					   &CurrentAddress);
   if (CurrentSegment == NULL)
     {
	KeBugCheck(0);
     }
   
   if (BaseAddress >= CurrentAddress &&
       (BaseAddress + RegionSize) <= (CurrentAddress + CurrentSegment->Length))
     {
	return((MmSplitSegment(AddressSpace,
			       MemoryArea,
			       BaseAddress,
			       RegionSize,
			       Type,
			       Protect,
			       CurrentSegment,
			       CurrentAddress)));
     }
   else
     {
	return((MmGatherSegment(AddressSpace,
				MemoryArea,
				BaseAddress,
				RegionSize,
				Type,
				Protect,
				CurrentSegment,
				CurrentAddress)));
     }
}


NTSTATUS STDCALL 
NtAllocateVirtualMemory(IN	HANDLE	ProcessHandle,
			IN OUT	PVOID*  UBaseAddress,
			IN	ULONG	ZeroBits,
			IN OUT	PULONG	URegionSize,
			IN	ULONG	AllocationType, 
			IN	ULONG	Protect)
/*
 * FUNCTION: Allocates a block of virtual memory in the process address space
 * ARGUMENTS:
 *      ProcessHandle = The handle of the process which owns the virtual memory
 *      BaseAddress   = A pointer to the virtual memory allocated. If you 
 *                      supply a non zero value the system will try to 
 *                      allocate the memory at the address supplied. It round 
 *                      it down to a multiple  of the page size.
 *      ZeroBits  = (OPTIONAL) You can specify the number of high order bits 
 *                      that must be zero, ensuring that the memory will be 
 *                      allocated at a address below a certain value.
 *      RegionSize = The number of bytes to allocate
 *      AllocationType = Indicates the type of virtual memory you like to 
 *                       allocated, can be one of the values : MEM_COMMIT, 
 *                       MEM_RESERVE, MEM_RESET, MEM_TOP_DOWN
 *      Protect = Indicates the protection type of the pages allocated, can be
 *                a combination of PAGE_READONLY, PAGE_READWRITE, 
 *                PAGE_EXECUTE_READ, PAGE_EXECUTE_READWRITE, PAGE_GUARD, 
 *                PAGE_NOACCESS
 * REMARKS:
 *       This function maps to the win32 VirtualAllocEx. Virtual memory is 
 *       process based so the  protocol starts with a ProcessHandle. I 
 *       splitted the functionality of obtaining the actual address and 
 *       specifying the start address in two parameters ( BaseAddress and 
 *       StartAddress ) The NumberOfBytesAllocated specify the range and the 
 *       AllocationType and ProctectionType map to the other two parameters.
 * RETURNS: Status
 */
{
   PEPROCESS Process;
   MEMORY_AREA* MemoryArea;
   ULONG Type;
   NTSTATUS Status;
   PMADDRESS_SPACE AddressSpace;
   PMM_SEGMENT Segment;
   PVOID BaseAddress;
   ULONG RegionSize;
   PVOID PBaseAddress;
   ULONG PRegionSize;

   DPRINT("NtAllocateVirtualMemory(ProcessHandle %x, *BaseAddress %x, "
	  "ZeroBits %d, *RegionSize %x, AllocationType %x, Protect %x)\n",
	  ProcessHandle,*BaseAddress,ZeroBits,*RegionSize,AllocationType,
	  Protect);
   
   /*
    * Check the validity of the parameters
    */
   if ((Protect & PAGE_FLAGS_VALID_FROM_USER_MODE) != Protect)
     {
       return(STATUS_INVALID_PAGE_PROTECTION);
     }
   PBaseAddress = *UBaseAddress;
   PRegionSize = *URegionSize;
   

   BaseAddress = (PVOID)PAGE_ROUND_DOWN(PBaseAddress);
   RegionSize = PAGE_ROUND_UP(PBaseAddress + PRegionSize) -
     PAGE_ROUND_DOWN(PBaseAddress);
   
   Status = ObReferenceObjectByHandle(ProcessHandle,
				      PROCESS_VM_OPERATION,
				      NULL,
				      UserMode,
				      (PVOID*)(&Process),
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	DPRINT("NtAllocateVirtualMemory() = %x\n",Status);
	return(Status);
     }
   
   if (AllocationType & MEM_RESERVE)
     {
	Type = MEM_RESERVE;
     }
   else
     {
	Type = MEM_COMMIT;
     }
   
   AddressSpace = &Process->AddressSpace;
   MmLockAddressSpace(AddressSpace);
   
   if (BaseAddress != 0)
     {
	MemoryArea = MmOpenMemoryAreaByAddress(&Process->AddressSpace,
					       BaseAddress);
	
	if (MemoryArea != NULL &&
	    MemoryArea->Type == MEMORY_AREA_VIRTUAL_MEMORY &&
	    MemoryArea->Length >= RegionSize)
	  {
	     Status = MmComplexVirtualMemoryOperation(AddressSpace,
						      MemoryArea,
						      BaseAddress,
						      RegionSize,
						      Type,
						      Protect);
	     /* FIXME: Reserve/dereserve swap pages */
	     MmUnlockAddressSpace(AddressSpace);
	     ObDereferenceObject(Process);
	     return(Status);
	  }
	else if (MemoryArea != NULL)
	  {
	     MmUnlockAddressSpace(AddressSpace);
	     ObDereferenceObject(Process);
	     return(STATUS_UNSUCCESSFUL);
	  }
     }

   Segment = ExAllocatePool(NonPagedPool, sizeof(MM_SEGMENT));
   if (Segment == NULL)
     {
	MmUnlockAddressSpace(AddressSpace);
	ObDereferenceObject(Process);
	return(STATUS_UNSUCCESSFUL);
     }
   
   Status = MmCreateMemoryArea(Process,
			       &Process->AddressSpace,
			       MEMORY_AREA_VIRTUAL_MEMORY,
			       &BaseAddress,
			       RegionSize,
			       Protect,
			       &MemoryArea);
   
   if (!NT_SUCCESS(Status))
     {
	DPRINT("NtAllocateVirtualMemory() = %x\n",Status);
	MmUnlockAddressSpace(AddressSpace);	
	ObDereferenceObject(Process);
	return(Status);
     }
   
   InitializeListHead(&MemoryArea->Data.VirtualMemoryData.SegmentListHead);
   
   Segment->Type = Type;
   Segment->Protect = Protect;
   Segment->Length = RegionSize;
   InsertTailList(&MemoryArea->Data.VirtualMemoryData.SegmentListHead,
		  &Segment->SegmentListEntry);
   
   DPRINT("*BaseAddress %x\n",*BaseAddress);
   if ((AllocationType & MEM_COMMIT) &&
       ((Protect & PAGE_READWRITE) ||
	(Protect & PAGE_EXECUTE_READWRITE)))
     {
	MmReserveSwapPages(RegionSize);
     }
      
   *UBaseAddress = BaseAddress;
   *URegionSize = RegionSize;
   
   MmUnlockAddressSpace(AddressSpace);
   ObDereferenceObject(Process);
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL 
NtFlushVirtualMemory(IN	HANDLE	ProcessHandle,
		     IN	PVOID	BaseAddress,
		     IN	ULONG	NumberOfBytesToFlush,
		     OUT PULONG NumberOfBytesFlushed OPTIONAL)
/*
 * FUNCTION: Flushes virtual memory to file
 * ARGUMENTS:
 *        ProcessHandle = Points to the process that allocated the virtual 
 *                        memory
 *        BaseAddress = Points to the memory address
 *        NumberOfBytesToFlush = Limits the range to flush,
 *        NumberOfBytesFlushed = Actual number of bytes flushed
 * RETURNS: Status 
 */
{
	UNIMPLEMENTED;
}


NTSTATUS STDCALL 
NtFreeVirtualMemory(IN	HANDLE	ProcessHandle,
		    IN	PVOID*  PBaseAddress,	
		    IN	PULONG	PRegionSize,	
		    IN	ULONG	FreeType)
/*
 * FUNCTION: Frees a range of virtual memory
 * ARGUMENTS:
 *        ProcessHandle = Points to the process that allocated the virtual 
 *                        memory
 *        BaseAddress = Points to the memory address, rounded down to a 
 *                      multiple of the pagesize
 *        RegionSize = Limits the range to free, rounded up to a multiple of 
 *                     the paging size
 *        FreeType = Can be one of the values:  MEM_DECOMMIT, or MEM_RELEASE
 * RETURNS: Status 
 */
{
   MEMORY_AREA* MemoryArea;
   NTSTATUS Status;
   PEPROCESS Process;
   PMADDRESS_SPACE AddressSpace;
   ULONG i;
   PVOID BaseAddress;
   ULONG RegionSize;
   
   DPRINT("NtFreeVirtualMemory(ProcessHandle %x, *BaseAddress %x, "
	  "*RegionSize %x, FreeType %x)\n",ProcessHandle,*BaseAddress,
	  *RegionSize,FreeType);
				 
   BaseAddress = (PVOID)PAGE_ROUND_DOWN((*PBaseAddress));
   RegionSize = PAGE_ROUND_UP((*PBaseAddress) + (*PRegionSize)) -
     PAGE_ROUND_DOWN((*PBaseAddress));
   
   Status = ObReferenceObjectByHandle(ProcessHandle,
				      PROCESS_VM_OPERATION,
				      PsProcessType,
				      UserMode,
				      (PVOID*)(&Process),
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   AddressSpace = &Process->AddressSpace;
   
   MmLockAddressSpace(AddressSpace);
   MemoryArea = MmOpenMemoryAreaByAddress(AddressSpace,
					  BaseAddress);
   if (MemoryArea == NULL)
     {
	MmUnlockAddressSpace(AddressSpace);
	ObDereferenceObject(Process);
	return(STATUS_UNSUCCESSFUL);
     }
   
   switch (FreeType)
     {
      case MEM_RELEASE:
	if (MemoryArea->BaseAddress != BaseAddress)
	  {
	     MmUnlockAddressSpace(AddressSpace);
	     ObDereferenceObject(Process);
	     return(STATUS_UNSUCCESSFUL);
	  }
#if 0	
	if ((MemoryArea->Type == MEMORY_AREA_COMMIT) &&
	    ((MemoryArea->Attributes & PAGE_READWRITE) ||
	     (MemoryArea->Attributes & PAGE_EXECUTE_READWRITE)))
	  {
	     MmDereserveSwapPages(PAGE_ROUND_UP(MemoryArea->Length));
	  }
#endif
	
	for (i=0; i<=(MemoryArea->Length/PAGESIZE); i++)
	  {
	     ULONG PhysicalAddr;
	     
	     PhysicalAddr = 
	       MmGetPhysicalAddressForProcess(Process, 
					      MemoryArea->BaseAddress + 
					      (i*PAGESIZE));
	     if (PhysicalAddr != 0)
	       {
		  MmRemovePageFromWorkingSet(AddressSpace->Process,
					     MemoryArea->BaseAddress +
					     (i*PAGESIZE));
		  MmDereferencePage((PVOID)(ULONG)(PhysicalAddr));
	       }
	  }
	
	MmFreeMemoryArea(&Process->AddressSpace,
			 BaseAddress,
			 0,
			 NULL,
			 NULL);
	MmUnlockAddressSpace(AddressSpace);
	ObDereferenceObject(Process);
	return(STATUS_SUCCESS);
	
      case MEM_DECOMMIT:	
	Status = MmComplexVirtualMemoryOperation(AddressSpace,
						 MemoryArea,
						 BaseAddress,
						 RegionSize,
						 MEM_RESERVE,
						 PAGE_NOACCESS);
	MmUnlockAddressSpace(AddressSpace);	
	ObDereferenceObject(Process);
	return(Status);
     }
   MmUnlockAddressSpace(AddressSpace);
   ObDereferenceObject(Process);
   return(STATUS_NOT_IMPLEMENTED);
}


NTSTATUS STDCALL 
NtLockVirtualMemory(HANDLE	ProcessHandle,
		    PVOID	BaseAddress,
		    ULONG	NumberOfBytesToLock,
		    PULONG	NumberOfBytesLocked)
{
  UNIMPLEMENTED;
}


VOID 
MmChangeAreaProtection(PEPROCESS Process,
		       PVOID BaseAddress,
		       ULONG Length,
		       ULONG Protect)
{
   ULONG i;
   
   for (i=0; i<(Length/PAGESIZE); i++)
     {
	if (MmIsPagePresent(Process, BaseAddress + (i*PAGESIZE)))
	  {
	     MmSetPageProtect(Process,
			      BaseAddress + (i*PAGESIZE), 
			      Protect);
	  }
     }
}


NTSTATUS STDCALL NtProtectVirtualMemory(IN	HANDLE	ProcessHandle,
					IN	PVOID	BaseAddress,
					IN	ULONG	NumberOfBytesToProtect,
					IN	ULONG	NewAccessProtection,
					OUT	PULONG	OldAccessProtection)
{
   PMEMORY_AREA MemoryArea;
   PEPROCESS Process;
   NTSTATUS Status;
   PMADDRESS_SPACE AddressSpace;
   
   Status = ObReferenceObjectByHandle(ProcessHandle,
				      PROCESS_VM_OPERATION,
				      PsProcessType,
				      UserMode,
				      (PVOID*)(&Process),
				      NULL);
   if (Status != STATUS_SUCCESS)
     {
	DPRINT("NtProtectVirtualMemory() = %x\n",Status);
	return(Status);
     }

   AddressSpace = &Process->AddressSpace;
   
   MmLockAddressSpace(AddressSpace);
   MemoryArea = MmOpenMemoryAreaByAddress(AddressSpace,
					  BaseAddress);
   if (MemoryArea == NULL)
     {
	DPRINT("NtProtectVirtualMemory() = %x\n",STATUS_UNSUCCESSFUL);
	MmUnlockAddressSpace(AddressSpace);
	ObDereferenceObject(Process);
	return(STATUS_UNSUCCESSFUL);
     }

   *OldAccessProtection = MemoryArea->Attributes;

   if (MemoryArea->BaseAddress == BaseAddress &&
       MemoryArea->Length == NumberOfBytesToProtect)
     {
	MemoryArea->Attributes = NewAccessProtection;	
     }
   else
     {
	MemoryArea = MmSplitMemoryArea(Process,
				       &Process->AddressSpace,
				       MemoryArea,
				       BaseAddress,
				       NumberOfBytesToProtect,
				       MemoryArea->Type,
				       NewAccessProtection);
     }
   MmChangeAreaProtection(Process,
			  BaseAddress,
			  NumberOfBytesToProtect,
			  NewAccessProtection);
   MmUnlockAddressSpace(AddressSpace);
   ObDereferenceObject(Process);
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL NtQueryVirtualMemory (IN HANDLE ProcessHandle,
				       IN PVOID Address,
				       IN CINT VirtualMemoryInformationClass,
				       OUT PVOID VirtualMemoryInformation,
				       IN ULONG Length,
				       OUT PULONG ResultLength)
{
   NTSTATUS Status;
   PEPROCESS Process;
   MEMORY_AREA* MemoryArea;

   DPRINT("NtQueryVirtualMemory(ProcessHandle %x, Address %x, "
          "VirtualMemoryInformationClass %d, VirtualMemoryInformation %x, "
          "Length %lu ResultLength %x)\n",ProcessHandle,Address,
          VirtualMemoryInformationClass,VirtualMemoryInformation,
          Length,ResultLength);

   switch(VirtualMemoryInformationClass)
     {
        case MemoryBasicInformation:
          {
             PMEMORY_BASIC_INFORMATION Info =
                (PMEMORY_BASIC_INFORMATION)VirtualMemoryInformation;
	     PMADDRESS_SPACE AddressSpace;
	     
             if (Length < sizeof(MEMORY_BASIC_INFORMATION))
               {
                  ObDereferenceObject(Process);
                  return STATUS_INFO_LENGTH_MISMATCH;
               }

             if (ResultLength)
               {
                  *ResultLength = sizeof(MEMORY_BASIC_INFORMATION);
               }

             Status = ObReferenceObjectByHandle(ProcessHandle,
                                                PROCESS_QUERY_INFORMATION,
                                                NULL,
                                                UserMode,
                                                (PVOID*)(&Process),
                                                NULL);

             if (!NT_SUCCESS(Status))
               {
                  DPRINT("NtQueryVirtualMemory() = %x\n",Status);
                  return(Status);
               }

	     AddressSpace = &Process->AddressSpace;
	     MmLockAddressSpace(AddressSpace);
             MemoryArea = MmOpenMemoryAreaByAddress(AddressSpace,
                                                    Address);

             if (MemoryArea == NULL)
               {
                  Info->State = MEM_FREE;
                  DPRINT("Virtual memory at %p is free.\n", Address);
		  MmUnlockAddressSpace(AddressSpace);
                  ObDereferenceObject(Process);
                  return (STATUS_SUCCESS);
               }

#if 0	     
             if (MemoryArea->Type == MEMORY_AREA_VIRTUAL_MEMORY)
               {
                  Info->State = MEM_COMMIT;
               }
             else
               {
                  Info->State = MEM_RESERVE;
               }
#endif
	     
             Info->BaseAddress = MemoryArea->BaseAddress;
             Info->RegionSize  = MemoryArea->Length;

             DPRINT("BaseAddress %p, RegionSize %x State %x\n",
                    Info->BaseAddress, Info->RegionSize, Info->State);
	     
	     MmUnlockAddressSpace(AddressSpace);
             ObDereferenceObject(Process);
             return STATUS_SUCCESS;
          }
          break;
     }

   return STATUS_INVALID_INFO_CLASS;
}


NTSTATUS STDCALL 
NtReadVirtualMemory(IN	HANDLE	ProcessHandle,
		    IN	PVOID	BaseAddress,
		    OUT	PVOID	Buffer,
		    IN	ULONG	NumberOfBytesToRead,
		    OUT	PULONG	NumberOfBytesRead)
{
   NTSTATUS Status;
   PMDL Mdl;
   PVOID SystemAddress;
   PEPROCESS Process;
   
   DPRINT("NtReadVirtualMemory(ProcessHandle %x, BaseAddress %x, "
	    "Buffer %x, NumberOfBytesToRead %d)\n",ProcessHandle,BaseAddress,
	    Buffer,NumberOfBytesToRead);
   
   Status = ObReferenceObjectByHandle(ProcessHandle,
				      PROCESS_VM_WRITE,
				      NULL,
				      UserMode,
				      (PVOID*)(&Process),
				      NULL);
   if (Status != STATUS_SUCCESS)
     {
	return(Status);
     }
   
   Mdl = MmCreateMdl(NULL, 
		     Buffer,
		     NumberOfBytesToRead);
   MmProbeAndLockPages(Mdl,
		       UserMode,
		       IoWriteAccess);
   
   KeAttachProcess(Process);
   
   SystemAddress = MmGetSystemAddressForMdl(Mdl);
   memcpy(SystemAddress, BaseAddress, NumberOfBytesToRead);
   
   KeDetachProcess();
   
   ObDereferenceObject(Process);
   
   *NumberOfBytesRead = NumberOfBytesToRead;
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL 
NtUnlockVirtualMemory(HANDLE	ProcessHandle,
		      PVOID	BaseAddress,
		      ULONG	NumberOfBytesToUnlock,
		      PULONG NumberOfBytesUnlocked OPTIONAL)
{
	UNIMPLEMENTED;
}


NTSTATUS STDCALL 
NtWriteVirtualMemory(IN	HANDLE	ProcessHandle,
		     IN	PVOID	BaseAddress,
		     IN	PVOID	Buffer,
		     IN	ULONG	NumberOfBytesToWrite,
		     OUT	PULONG	NumberOfBytesWritten)
{
   NTSTATUS Status;
   PMDL Mdl;
   PVOID SystemAddress;
   PEPROCESS Process;
   
   DPRINT("NtWriteVirtualMemory(ProcessHandle %x, BaseAddress %x, "
	    "Buffer %x, NumberOfBytesToWrite %d)\n",ProcessHandle,BaseAddress,
	    Buffer,NumberOfBytesToWrite);
   
   Status = ObReferenceObjectByHandle(ProcessHandle,
				      PROCESS_VM_WRITE,
				      NULL,
				      UserMode,
				      (PVOID*)(&Process),
				      NULL);
   if (Status != STATUS_SUCCESS)
     {
	return(Status);
     }
   
   Mdl = MmCreateMdl(NULL, 
		     Buffer,
		     NumberOfBytesToWrite);
   MmProbeAndLockPages(Mdl,
		       UserMode,
		       IoReadAccess);
   
   KeAttachProcess(Process);
   
   DPRINT("Attached to process copying memory\n");
   
   SystemAddress = MmGetSystemAddressForMdl(Mdl);
   memcpy(BaseAddress, SystemAddress, NumberOfBytesToWrite);
   
   DPRINT("Done copy\n");
   
   KeDetachProcess();
   
   ObDereferenceObject(Process);
   
   *NumberOfBytesWritten = NumberOfBytesToWrite;
   
   DPRINT("Finished NtWriteVirtualMemory()\n");
   
   return(STATUS_SUCCESS);
}


DWORD
STDCALL
MmSecureVirtualMemory (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2
	)
{
	UNIMPLEMENTED;
	return 0;
}


VOID
STDCALL
MmUnsecureVirtualMemory (
	DWORD	Unknown0
	)
{
	UNIMPLEMENTED;
}

/* EOF */
