/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/marea.c
 * PURPOSE:         Implements memory areas
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/mm.h>
#include <internal/mmhal.h>
#include <internal/ps.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

VOID MmDumpMemoryAreas(PLIST_ENTRY ListHead)
{
   PLIST_ENTRY current_entry;
   MEMORY_AREA* current;
   
   DbgPrint("MmDumpMemoryAreas()\n");
   
   current_entry = ListHead->Flink;
   while (current_entry!=ListHead)
     {
	current = CONTAINING_RECORD(current_entry,MEMORY_AREA,Entry);
	DbgPrint("Base %x Length %x End %x Attributes %x Flink %x\n",
	       current->BaseAddress,current->Length,
	       current->BaseAddress+current->Length,current->Attributes,
	       current->Entry.Flink);
	current_entry = current_entry->Flink;
     }
   DbgPrint("Finished MmDumpMemoryAreas()\n");
}

MEMORY_AREA* MmOpenMemoryAreaByAddress(PMADDRESS_SPACE AddressSpace,
				       PVOID Address)
{
   PLIST_ENTRY current_entry;
   MEMORY_AREA* current;
   PLIST_ENTRY previous_entry;

   DPRINT("MmOpenMemoryAreaByAddress(AddressSpace %x, Address %x)\n",
	   AddressSpace, Address);
   
//   MmDumpMemoryAreas(&AddressSpace->MAreaListHead);
   
   previous_entry = &AddressSpace->MAreaListHead;
   current_entry = AddressSpace->MAreaListHead.Flink;
   while (current_entry != &AddressSpace->MAreaListHead)
     {
	current = CONTAINING_RECORD(current_entry,
				    MEMORY_AREA,
				    Entry);
	DPRINT("Scanning %x BaseAddress %x Length %x\n",
		current, current->BaseAddress, current->Length);
	assert(current_entry->Blink->Flink == current_entry);
	if (current_entry->Flink->Blink != current_entry)
	  {
	     DPRINT("BaseAddress %x\n", current->BaseAddress);
	     DPRINT("current_entry->Flink %x ", current_entry->Flink);
	     DPRINT("&current_entry->Flink %x\n",
		     &current_entry->Flink);
	     DPRINT("current_entry->Flink->Blink %x\n",
		     current_entry->Flink->Blink);
	     DPRINT("&current_entry->Flink->Blink %x\n",
		     &current_entry->Flink->Blink);
	     DPRINT("&current_entry->Flink %x\n",
		     &current_entry->Flink);
	  }
	assert(current_entry->Flink->Blink == current_entry);
	assert(previous_entry->Flink == current_entry);
	if (current->BaseAddress <= Address &&
	    (current->BaseAddress + current->Length) > Address)
	  {
	     DPRINT("%s() = %x\n",__FUNCTION__,current);
	     return(current);
	  }
	if (current->BaseAddress > Address)
	  {
	     DPRINT("%s() = NULL\n",__FUNCTION__);
	     return(NULL);
	  }
	previous_entry = current_entry;
	current_entry = current_entry->Flink;
     }
   DPRINT("%s() = NULL\n",__FUNCTION__);
   return(NULL);
}

MEMORY_AREA* MmOpenMemoryAreaByRegion(PMADDRESS_SPACE AddressSpace, 
				      PVOID Address,
				      ULONG Length)
{
   PLIST_ENTRY current_entry;
   MEMORY_AREA* current;
   ULONG Extent;
   
   DPRINT("MmOpenMemoryByRegion(AddressSpace %x, Address %x, Length %x)\n",
	  AddressSpace, Address, Length);
   
   current_entry = AddressSpace->MAreaListHead.Flink;
   while (current_entry != &AddressSpace->MAreaListHead)
     {
	current = CONTAINING_RECORD(current_entry,
				    MEMORY_AREA,
				    Entry);
	DPRINT("current->BaseAddress %x current->Length %x\n",
	       current->BaseAddress,current->Length);
	if (current->BaseAddress >= Address &&
	    current->BaseAddress < (Address+Length))
	  {
	     DPRINT("Finished MmOpenMemoryAreaByRegion() = %x\n",
		    current);
	     return(current);
	  }
	Extent = (ULONG)current->BaseAddress + current->Length;
	if (Extent > (ULONG)Address &&
	    Extent < (ULONG)(Address+Length))
	  {
	     DPRINT("Finished MmOpenMemoryAreaByRegion() = %x\n",
		    current);
	     return(current);
	  }
	if (current->BaseAddress <= Address &&
	    Extent >= (ULONG)(Address+Length))
	  {
	     DPRINT("Finished MmOpenMemoryAreaByRegion() = %x\n",
		    current);
	     return(current);
	  }
	if (current->BaseAddress >= (Address+Length))
	  {
	     DPRINT("Finished MmOpenMemoryAreaByRegion()= NULL\n",0);
	     return(NULL);
	  }
	current_entry = current_entry->Flink;
     }
   DPRINT("Finished MmOpenMemoryAreaByRegion() = NULL\n",0);
   return(NULL);
}

static VOID MmInsertMemoryArea(PMADDRESS_SPACE AddressSpace,
			       MEMORY_AREA* marea)
{
   PLIST_ENTRY ListHead;
   PLIST_ENTRY current_entry;
   PLIST_ENTRY inserted_entry = &marea->Entry;
   MEMORY_AREA* current;
   MEMORY_AREA* next;   
   
   DPRINT("MmInsertMemoryArea(marea %x)\n", marea);
   DPRINT("marea->BaseAddress %x\n", marea->BaseAddress);
   DPRINT("marea->Length %x\n", marea->Length);
   
   ListHead = &AddressSpace->MAreaListHead;
   
   current_entry = ListHead->Flink;
   CHECKPOINT;
   if (IsListEmpty(ListHead))
     {
	CHECKPOINT;
	InsertHeadList(ListHead,&marea->Entry);
	DPRINT("Inserting at list head\n");
	CHECKPOINT;
	return;
     }
   CHECKPOINT;
   current = CONTAINING_RECORD(current_entry,MEMORY_AREA,Entry);
   CHECKPOINT;
   if (current->BaseAddress > marea->BaseAddress)
     {
	CHECKPOINT;
	InsertHeadList(ListHead,&marea->Entry);
	DPRINT("Inserting at list head\n");
	CHECKPOINT;
	return;
     }
   CHECKPOINT;
   while (current_entry->Flink!=ListHead)
     {
//	CHECKPOINT;
	current = CONTAINING_RECORD(current_entry,MEMORY_AREA,Entry);
	next = CONTAINING_RECORD(current_entry->Flink,MEMORY_AREA,Entry);
//	assert(current->BaseAddress != marea->BaseAddress);	
//	assert(next->BaseAddress != marea->BaseAddress);
	if (current->BaseAddress < marea->BaseAddress &&
	    current->Entry.Flink==ListHead)
	  {
	     DPRINT("Insert after %x\n", current_entry);
	     current_entry->Flink = inserted_entry;
	     inserted_entry->Flink=ListHead;
	     inserted_entry->Blink=current_entry;
	     ListHead->Blink = inserted_entry;	    	     
	     return;
	  }
	if (current->BaseAddress < marea->BaseAddress &&
	    next->BaseAddress > marea->BaseAddress)
	  {	     
	     DPRINT("Inserting before %x\n", current_entry);
	     inserted_entry->Flink = current_entry->Flink;
	     inserted_entry->Blink = current_entry;
	     inserted_entry->Flink->Blink = inserted_entry;
	     current_entry->Flink=inserted_entry;
	     return;
	  }
	current_entry = current_entry->Flink;
     }
   CHECKPOINT;
   DPRINT("Inserting at list tail\n");
   InsertTailList(ListHead,inserted_entry);
}

static PVOID MmFindGap(PMADDRESS_SPACE AddressSpace,
		       ULONG Length)
{
   PLIST_ENTRY ListHead;
   PLIST_ENTRY current_entry;
   MEMORY_AREA* current;
   MEMORY_AREA* next;
   ULONG Gap;
   
   DPRINT("MmFindGap(Length %x)\n",Length);
   
   ListHead = &AddressSpace->MAreaListHead;
     
   current_entry = ListHead->Flink;
   while (current_entry->Flink!=ListHead)
     {
	current = CONTAINING_RECORD(current_entry,MEMORY_AREA,Entry);
	next = CONTAINING_RECORD(current_entry->Flink,MEMORY_AREA,Entry);
	DPRINT("current %x current->BaseAddress %x ",current,
	       current->BaseAddress);
	DPRINT("current->Length %x\n",current->Length);
	DPRINT("next %x next->BaseAddress %x ",next,next->BaseAddress);
	Gap = (next->BaseAddress ) -(current->BaseAddress + current->Length);
	DPRINT("Base %x Gap %x\n",current->BaseAddress,Gap);
	if (Gap >= Length)
	  {
	     return(current->BaseAddress + PAGE_ROUND_UP(current->Length));
	  }
	current_entry = current_entry->Flink;
     }
   
   if (current_entry == ListHead)
     {
	return((PVOID)AddressSpace->LowestAddress);
     }
   
   current = CONTAINING_RECORD(current_entry,MEMORY_AREA,Entry);
   //DbgPrint("current %x returning %x\n",current,current->BaseAddress+
//	    current->Length);
   return(current->BaseAddress + PAGE_ROUND_UP(current->Length));
}

NTSTATUS MmInitMemoryAreas(VOID)
/*
 * FUNCTION: Initialize the memory area list
 */
{
   DPRINT("MmInitMemoryAreas()\n",0);
   return(STATUS_SUCCESS);
}

NTSTATUS 
MmFreeMemoryArea(PMADDRESS_SPACE AddressSpace,
		 PVOID BaseAddress,
		 ULONG Length,
		 VOID (*FreePage)(PVOID Context, PVOID Address),
		 PVOID FreePageContext)
{
   MEMORY_AREA* MemoryArea;
   ULONG i;
   
   DPRINT("MmFreeMemoryArea(AddressSpace %x, BaseAddress %x, Length %x,"
	   "FreePages %d)\n",AddressSpace,BaseAddress,Length,FreePages);

   MemoryArea = MmOpenMemoryAreaByAddress(AddressSpace,
					  BaseAddress);
   if (MemoryArea == NULL)
     {
	KeBugCheck(0);
	return(STATUS_UNSUCCESSFUL);
     }
   if (FreePage != NULL)
     {
	for (i=0;i<=(MemoryArea->Length/PAGESIZE);i++)
	  {
	    FreePage(FreePageContext, 
		     MemoryArea->BaseAddress + (i * PAGESIZE));
	  }
     }
   for (i=0; i<=(MemoryArea->Length/PAGESIZE); i++)
     {
	if (AddressSpace->Process != NULL)
	  {
	     DPRINT("Freeing %x in %d\n", 
		     MemoryArea->BaseAddress + (i*PAGESIZE),
		     AddressSpace->Process->UniqueProcessId);
	  }
	else
	  {
//	     DPRINT("Freeing %x in kernel address space\n");
	  }
	MmDeleteVirtualMapping(AddressSpace->Process,
			       MemoryArea->BaseAddress + (i*PAGESIZE),
			       FALSE);
     }
   
   RemoveEntryList(&(MemoryArea->Entry));
   ExFreePool(MemoryArea);
   
   DPRINT("MmFreeMemoryArea() succeeded\n");
   
   return(STATUS_SUCCESS);
}

PMEMORY_AREA MmSplitMemoryArea(PEPROCESS Process,
			       PMADDRESS_SPACE AddressSpace,
			       PMEMORY_AREA OriginalMemoryArea,
			       PVOID BaseAddress,
			       ULONG Length,
			       ULONG NewType,
			       ULONG NewAttributes)
{
   PMEMORY_AREA Result;
   PMEMORY_AREA Split;
   
   Result = ExAllocatePool(NonPagedPool,sizeof(MEMORY_AREA));
   RtlZeroMemory(Result,sizeof(MEMORY_AREA));
   Result->Type = NewType;
   Result->BaseAddress = BaseAddress;
   Result->Length = Length;
   Result->Attributes = NewAttributes;
   Result->LockCount = 0;
   Result->Process = Process;
   
   if (BaseAddress == OriginalMemoryArea->BaseAddress)
     {
	OriginalMemoryArea->BaseAddress = BaseAddress + Length;
	OriginalMemoryArea->Length = OriginalMemoryArea->Length - Length;
	MmInsertMemoryArea(AddressSpace, Result);
	return(Result);
     }
   if ((BaseAddress + Length) == 
       (OriginalMemoryArea->BaseAddress + OriginalMemoryArea->Length))
     {
	OriginalMemoryArea->Length = OriginalMemoryArea->Length - Length; 
	MmInsertMemoryArea(AddressSpace, Result);

	return(Result);
     }
      
   Split = ExAllocatePool(NonPagedPool,sizeof(MEMORY_AREA));
   RtlCopyMemory(Split,OriginalMemoryArea,sizeof(MEMORY_AREA));
   Split->BaseAddress = BaseAddress + Length;
   Split->Length = OriginalMemoryArea->Length - (((ULONG)BaseAddress) 
						 + Length);
   
   OriginalMemoryArea->Length = BaseAddress - OriginalMemoryArea->BaseAddress;
      
   return(Split);
}

NTSTATUS MmCreateMemoryArea(PEPROCESS Process,
			    PMADDRESS_SPACE AddressSpace,
			    ULONG Type,
			    PVOID* BaseAddress,
			    ULONG Length,
			    ULONG Attributes,
			    MEMORY_AREA** Result)
/*
 * FUNCTION: Create a memory area
 * ARGUMENTS:
 *     AddressSpace = Address space to create the area in
 *     Type = Type of the address space
 *     BaseAddress = 
 *     Length = Length to allocate
 *     Attributes = Protection attributes for the memory area
 *     Result = Receives a pointer to the memory area on exit
 * RETURNS: Status
 * NOTES: Lock the address space before calling this function
 */
{
   DPRINT("MmCreateMemoryArea(Type %d, BaseAddress %x,"
	   "*BaseAddress %x, Length %x, Attributes %x, Result %x)\n",
	   Type,BaseAddress,*BaseAddress,Length,Attributes,Result);

   if ((*BaseAddress)==0)
     {
	*BaseAddress = MmFindGap(AddressSpace,
				 PAGE_ROUND_UP(Length) +(PAGESIZE*2));
	if ((*BaseAddress)==0)
	  {
	     DPRINT("No suitable gap\n");
	     return(STATUS_UNSUCCESSFUL);
	  }
	(*BaseAddress)=(*BaseAddress)+PAGESIZE;
     }
   else
     {
	(*BaseAddress) = (PVOID)PAGE_ROUND_DOWN((*BaseAddress));
	if (MmOpenMemoryAreaByRegion(AddressSpace,
				     *BaseAddress,
				     Length)!=NULL)
	  {
	     DPRINT("Memory area already occupied\n");
	     return(STATUS_UNSUCCESSFUL);
	  }
     }
   
   *Result = ExAllocatePool(NonPagedPool,sizeof(MEMORY_AREA));
   RtlZeroMemory(*Result,sizeof(MEMORY_AREA));
   (*Result)->Type = Type;
   (*Result)->BaseAddress = *BaseAddress;
   (*Result)->Length = Length;
   (*Result)->Attributes = Attributes;
   (*Result)->LockCount = 0;
   (*Result)->Process = Process;
   
   MmInsertMemoryArea(AddressSpace, *Result);
   
   DPRINT("MmCreateMemoryArea() succeeded\n");
   return(STATUS_SUCCESS);
}
