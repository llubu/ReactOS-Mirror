/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/aspace.c
 * PURPOSE:         Manages address spaces
 *
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include <internal/debug.h>

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, MmInitializeKernelAddressSpace)
#endif


/* GLOBALS ******************************************************************/

STATIC MADDRESS_SPACE KernelAddressSpace;

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
MmLockAddressSpace(PMADDRESS_SPACE AddressSpace)
{
   /*
    * Don't bother with locking if we are the first thread.
    */
   if (KeGetCurrentThread() == NULL)
   {
      return;
   }
   ExEnterCriticalRegionAndAcquireFastMutexUnsafe(&AddressSpace->Lock);
}

VOID
NTAPI
MmUnlockAddressSpace(PMADDRESS_SPACE AddressSpace)
{
   /*
    * Don't bother locking if we are the first thread.
    */
   if (KeGetCurrentThread() == NULL)
   {
      return;
   }
   ExReleaseFastMutexUnsafeAndLeaveCriticalRegion(&AddressSpace->Lock);
}

VOID
INIT_FUNCTION
NTAPI
MmInitializeKernelAddressSpace(VOID)
{
   MmInitializeAddressSpace(NULL, &KernelAddressSpace);
}

PMADDRESS_SPACE
NTAPI
MmGetCurrentAddressSpace(VOID)
{
   return(&((PROS_EPROCESS)PsGetCurrentProcess())->AddressSpace);
}

PMADDRESS_SPACE
NTAPI
MmGetKernelAddressSpace(VOID)
{
   return(&KernelAddressSpace);
}

NTSTATUS
NTAPI
MmInitializeAddressSpace(PROS_EPROCESS Process,
                         PMADDRESS_SPACE AddressSpace)
{
   AddressSpace->MemoryAreaRoot = NULL;
   ExInitializeFastMutex(&AddressSpace->Lock);
   if (Process != NULL)
   {
      AddressSpace->LowestAddress = MM_LOWEST_USER_ADDRESS;
   }
   else
   {
      AddressSpace->LowestAddress = MmSystemRangeStart;
   }
   AddressSpace->Process = Process;
   if (Process != NULL)
   {
      ULONG Count;
      Count = MiGetUserPageDirectoryCount();
      AddressSpace->PageTableRefCountTable =
         ExAllocatePoolWithTag(NonPagedPool, Count * sizeof(USHORT),
                               TAG_PTRC);
      RtlZeroMemory(AddressSpace->PageTableRefCountTable, Count * sizeof(USHORT));
      AddressSpace->PageTableRefCountTableSize = Count;
   }
   else
   {
      AddressSpace->PageTableRefCountTable = NULL;
      AddressSpace->PageTableRefCountTableSize = 0;
   }
   return(STATUS_SUCCESS);
}

NTSTATUS
NTAPI
MmDestroyAddressSpace(PMADDRESS_SPACE AddressSpace)
{
   if (AddressSpace->PageTableRefCountTable != NULL)
   {
      ExFreePool(AddressSpace->PageTableRefCountTable);
   }
   return(STATUS_SUCCESS);
}

/* EOF */
