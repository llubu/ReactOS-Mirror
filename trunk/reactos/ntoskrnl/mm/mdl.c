/* $Id: mdl.c,v 1.20 2000/07/02 17:32:51 ekohl Exp $
 *
 * COPYRIGHT:    See COPYING in the top level directory
 * PROJECT:      ReactOS kernel
 * FILE:         ntoskrnl/mm/mdl.c
 * PURPOSE:      Manipulates MDLs
 * PROGRAMMER:   David Welch (welch@cwcom.net)
 * UPDATE HISTORY: 
 *               27/05/98: Created
 */

/* INCLUDES ****************************************************************/

#include <ddk/ntddk.h>
#include <internal/mm.h>
#include <internal/mmhal.h>
#include <string.h>
#include <internal/string.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS ***************************************************************/

PVOID MmGetMdlPageAddress(PMDL Mdl, PVOID Offset)
{
   PULONG mdl_pages;
   
   mdl_pages = (PULONG)(Mdl + 1);
   
   return((PVOID)mdl_pages[((ULONG)Offset) / PAGESIZE]);
}

VOID STDCALL MmUnlockPages(PMDL MemoryDescriptorList)
/*
 * FUNCTION: Unlocks the physical pages described by a given MDL
 * ARGUMENTS:
 *      MemoryDescriptorList = MDL describing the buffer to be unlocked
 * NOTES: The memory described by the specified MDL must have been locked
 * previously by a call to MmProbeAndLockPages. As the pages unlocked, the
 * MDL is updated
 */
{
   /* It is harmless to leave this one as a stub */
}

PVOID STDCALL MmMapLockedPages(PMDL Mdl, KPROCESSOR_MODE AccessMode)
/*
 * FUNCTION: Maps the physical pages described by a given MDL
 * ARGUMENTS:
 *       Mdl = Points to an MDL updated by MmProbeAndLockPages
 *       AccessMode = Specifies the access mode in which to map the MDL
 * RETURNS: The base virtual address that maps the locked pages for the
 * range described by the MDL
 */
{
   PVOID base = NULL;
   unsigned int i;
   ULONG* mdl_pages=NULL;
   MEMORY_AREA* Result;
   
   DPRINT("MmMapLockedPages(Mdl %x, AccessMode %x)\n", Mdl, AccessMode);
   
   DPRINT("Mdl->ByteCount %x\n",Mdl->ByteCount);
   DPRINT("PAGE_ROUND_UP(Mdl->ByteCount)/PAGESIZE) %x\n",
	  PAGE_ROUND_UP(Mdl->ByteCount)/PAGESIZE);
   
   MmCreateMemoryArea(NULL,
		      MmGetKernelAddressSpace(),
		      MEMORY_AREA_MDL_MAPPING,
		      &base,
		      Mdl->ByteCount + Mdl->ByteOffset,
		      0,
		      &Result);
   CHECKPOINT;
   mdl_pages = (ULONG *)(Mdl + 1);
   for (i=0; i<(PAGE_ROUND_UP(Mdl->ByteCount+Mdl->ByteOffset)/PAGESIZE); i++)
     {
	DPRINT("Writing %x with physical address %x\n",
	       base+(i*PAGESIZE),mdl_pages[i]);
	MmSetPage(NULL,
		  (PVOID)((DWORD)base+(i*PAGESIZE)),
		  PAGE_READWRITE,
		  mdl_pages[i]);
     }
   DPRINT("base %x\n",base);
   Mdl->MdlFlags = Mdl->MdlFlags | MDL_MAPPED_TO_SYSTEM_VA;
   Mdl->MappedSystemVa = base + Mdl->ByteOffset;
   return(base + Mdl->ByteOffset);
}


VOID STDCALL MmUnmapLockedPages(PVOID BaseAddress, PMDL Mdl)
/*
 * FUNCTION: Releases a mapping set up by a preceding call to MmMapLockedPages
 * ARGUMENTS:
 *         BaseAddress = Base virtual address to which the pages were mapped
 *         MemoryDescriptorList = MDL describing the mapped pages
 */
{
   DPRINT("MmUnmapLockedPages(BaseAddress %x, Mdl %x)\n", Mdl, BaseAddress);
   (void)MmFreeMemoryArea(MmGetKernelAddressSpace(),
			  BaseAddress-Mdl->ByteOffset,
			  Mdl->ByteCount,
			  FALSE);
   Mdl->MdlFlags = Mdl->MdlFlags & ~MDL_MAPPED_TO_SYSTEM_VA;
   Mdl->MappedSystemVa = NULL;
   DPRINT("MmUnmapLockedPages() finished\n");
}


VOID MmBuildMdlFromPages(PMDL Mdl)
{
   ULONG i;
   PULONG mdl_pages;
   
   mdl_pages = (PULONG)(Mdl + 1);
   
   for (i=0;i<(PAGE_ROUND_UP(Mdl->ByteOffset+Mdl->ByteCount)/PAGESIZE);i++)
     {
        mdl_pages[i] = (ULONG)MmAllocPage();
	DPRINT("mdl_pages[i] %x\n",mdl_pages[i]);
     }
}


VOID
STDCALL
MmProbeAndLockPages (
	PMDL		Mdl,
	KPROCESSOR_MODE	AccessMode,
	LOCK_OPERATION	Operation
	)
/*
 * FUNCTION: Probes the specified pages, makes them resident and locks them
 * ARGUMENTS:
 *          Mdl = MDL to probe
 *          AccessMode = Access at which to probe the buffer
 *          Operation = Operation to probe for
 */
{
   ULONG* mdl_pages=NULL;
   int i;
   MEMORY_AREA* marea;
   PVOID Address;
   PMADDRESS_SPACE AddressSpace;
   
   DPRINT("MmProbeAndLockPages(Mdl %x)\n",Mdl);
   DPRINT("StartVa %x\n",Mdl->StartVa);
   
   if (Mdl->StartVa > (PVOID)KERNEL_BASE)
     {
	AddressSpace = MmGetKernelAddressSpace();
     }
   else
     {
	AddressSpace = &Mdl->Process->Pcb.AddressSpace;
     }
   MmLockAddressSpace(AddressSpace);
   marea = MmOpenMemoryAreaByAddress(AddressSpace,
				     Mdl->StartVa);
   DPRINT("marea %x\n",marea);

   /*
    * Check the area is valid
    */
   if (marea==NULL )
     {
	DbgPrint("(%s:%d) Area is invalid\n",__FILE__,__LINE__);
	MmUnlockAddressSpace(AddressSpace);
	ExRaiseStatus(STATUS_INVALID_PARAMETER);
     }

   /*
    * Lock the memory area
    * (We can't allow it to be freed while an I/O operation to it is
    * ongoing)
    */
   
   /*
    * Lock the pages
    */
   mdl_pages = (ULONG *)(Mdl + 1);
   
   for (i=0;i<(PAGE_ROUND_UP(Mdl->ByteOffset+Mdl->ByteCount)/PAGESIZE);i++)
     {
	Address = Mdl->StartVa + (i*PAGESIZE);
	mdl_pages[i] = (MmGetPhysicalAddress(Address)).u.LowPart;
	DPRINT("mdl_pages[i] %x\n",mdl_pages[i]);
     }
   MmUnlockAddressSpace(AddressSpace);
}


ULONG
STDCALL
MmSizeOfMdl (
	PVOID	Base,
	ULONG	Length
	)
/*
 * FUNCTION: Returns the number of bytes to allocate for an MDL describing
 * the given address range
 * ARGUMENTS:
 *         Base = base virtual address
 *         Length = number of bytes to map
 */
{
   unsigned int len=ADDRESS_AND_SIZE_TO_SPAN_PAGES(Base,Length);
   
   DPRINT("MmSizeOfMdl() %x\n",sizeof(MDL)+(len*sizeof(ULONG)));
   return(sizeof(MDL)+(len*sizeof(ULONG)));
}


VOID
STDCALL
MmBuildMdlForNonPagedPool (
	PMDL	Mdl
	)
/*
 * FUNCTION: Fills in the corresponding physical page array of a given 
 * MDL for a buffer in nonpaged system space
 * ARGUMENTS:
 *        Mdl = Points to an MDL that supplies a virtual address, 
 *              byte offset and length
 */
{
   int va;
   Mdl->MdlFlags = Mdl->MdlFlags | MDL_SOURCE_IS_NONPAGED_POOL;
   for (va=0; va<Mdl->Size; va++)
     {
        ((PULONG)(Mdl + 1))[va] =
            (MmGetPhysicalAddress(Mdl->StartVa + (va * PAGESIZE))).u.LowPart;
     }
   Mdl->MappedSystemVa = Mdl->StartVa;
}


PMDL
STDCALL
MmCreateMdl (
	PMDL	MemoryDescriptorList,
	PVOID	Base,
	ULONG	Length
	)
/*
 * FUNCTION: Allocates and initalizes an MDL
 * ARGUMENTS:
 *          MemoryDescriptorList = Points to MDL to initalize. If this is
 *                                 NULL then one is allocated
 *          Base = Base virtual address of the buffer
 *          Length = Length in bytes of the buffer
 * RETURNS: A pointer to initalized MDL
 */
{
   if (MemoryDescriptorList == NULL)
     {
	ULONG Size;
	
	Size = MmSizeOfMdl(Base,Length);
	MemoryDescriptorList = (PMDL)ExAllocatePool(NonPagedPool,Size);
	if (MemoryDescriptorList==NULL)
	  {
	     return(NULL);
	  }
     }
   
   MmInitializeMdl(MemoryDescriptorList,Base,Length);
   
   return(MemoryDescriptorList);
}


VOID
STDCALL
MmMapMemoryDumpMdl (
	PVOID	Unknown0
	)
{
	UNIMPLEMENTED;
}

/* EOF */
