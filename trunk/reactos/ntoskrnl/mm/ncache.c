/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/cont.c
 * PURPOSE:         Manages non-cached memory
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/


/**********************************************************************
 * NAME       EXPORTED
 *  MmAllocateNonCachedMemory@4
 *
 * DESCRIPTION
 *  Allocates a virtual address range of noncached and cache
 * aligned memory.
 * 
 * ARGUMENTS
 * NumberOfBytes
 *  Size of region to allocate.
 *  
 * RETURN VALUE
 * The base address of the range on success;
 * NULL on failure.
 *
 * NOTE
 *  Description taken from include/ddk/mmfuncs.h.
 *  Code taken from ntoskrnl/mm/special.c.
 *
 * REVISIONS
 *
 * @implemented
 */
PVOID STDCALL
MmAllocateNonCachedMemory(IN ULONG NumberOfBytes)
{
   PVOID Result;
   MEMORY_AREA* marea;
   NTSTATUS Status;
   ULONG i;
   ULONG Attributes;
   PHYSICAL_ADDRESS BoundaryAddressMultiple;

   BoundaryAddressMultiple.QuadPart = 0;
   MmLockAddressSpace(MmGetKernelAddressSpace());
   Result = NULL;
   Status = MmCreateMemoryArea (NULL,
                                MmGetKernelAddressSpace(),
                                MEMORY_AREA_NO_CACHE,
                                &Result,
                                NumberOfBytes,
                                0,
                                &marea,
                                FALSE,
                                FALSE,
                                BoundaryAddressMultiple);
   MmUnlockAddressSpace(MmGetKernelAddressSpace());

   if (!NT_SUCCESS(Status))
   {
      return (NULL);
   }
   Attributes = PAGE_READWRITE | PAGE_SYSTEM | PAGE_NOCACHE |
                PAGE_WRITETHROUGH;
   for (i = 0; i < (PAGE_ROUND_UP(NumberOfBytes) / PAGE_SIZE); i++)
   {
      PFN_TYPE NPage;

      Status = MmRequestPageMemoryConsumer(MC_NPPOOL, TRUE, &NPage);
      MmCreateVirtualMapping (NULL,
                              (char*)Result + (i * PAGE_SIZE),
                              Attributes,
                              &NPage,
                              1);
   }
   return ((PVOID)Result);
}

VOID STATIC
MmFreeNonCachedPage(PVOID Context, MEMORY_AREA* MemoryArea, PVOID Address,
                    PFN_TYPE Page, SWAPENTRY SwapEntry,
                    BOOLEAN Dirty)
{
   ASSERT(SwapEntry == 0);
   if (Page != 0)
   {
      MmReleasePageMemoryConsumer(MC_NPPOOL, Page);
   }
}

/**********************************************************************
 * NAME       EXPORTED
 * MmFreeNonCachedMemory@8
 *
 * DESCRIPTION
 * Releases a range of noncached memory allocated with 
 * MmAllocateNonCachedMemory.
 * 
 * ARGUMENTS
 * BaseAddress
 *  Virtual address to be freed;
 *  
 * NumberOfBytes
 *  Size of the region to be freed.
 *  
 * RETURN VALUE
 *  None.
 *
 * NOTE
 *  Description taken from include/ddk/mmfuncs.h.
 *  Code taken from ntoskrnl/mm/special.c.
 *
 * REVISIONS
 *
 * @implemented
 */
VOID STDCALL MmFreeNonCachedMemory (IN PVOID BaseAddress,
                                    IN ULONG NumberOfBytes)
{
   MmLockAddressSpace(MmGetKernelAddressSpace());
   MmFreeMemoryAreaByPtr(MmGetKernelAddressSpace(),
                         BaseAddress,
                         MmFreeNonCachedPage,
                         NULL);
   MmUnlockAddressSpace(MmGetKernelAddressSpace());
}

/* EOF */
