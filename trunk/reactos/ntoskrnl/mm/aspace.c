/* $Id: aspace.c,v 1.3 2000/07/04 08:52:42 dwelch Exp $
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/aspace.c
 * PURPOSE:         Manages address spaces
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/mm.h>
#include <internal/ps.h>

#include <internal/debug.h>

/* GLOBALS ******************************************************************/

static MADDRESS_SPACE KernelAddressSpace;

/* FUNCTIONS *****************************************************************/

VOID MmLockAddressSpace(PMADDRESS_SPACE AddressSpace)
{
   (VOID)KeWaitForMutexObject(&AddressSpace->Lock,
			      0,
			      KernelMode,
			      FALSE,
			      NULL);   
}

VOID MmUnlockAddressSpace(PMADDRESS_SPACE AddressSpace)
{
   KeReleaseMutex(&AddressSpace->Lock, FALSE);
}

VOID MmInitializeKernelAddressSpace(VOID)
{
   MmInitializeAddressSpace(NULL, &KernelAddressSpace);
   KernelAddressSpace.LowestAddress = KERNEL_BASE;
}

PMADDRESS_SPACE MmGetCurrentAddressSpace(VOID)
{
   return(&PsGetCurrentProcess()->AddressSpace);
}

PMADDRESS_SPACE MmGetKernelAddressSpace(VOID)
{
   return(&KernelAddressSpace);
}

NTSTATUS MmInitializeAddressSpace(PEPROCESS Process,
				  PMADDRESS_SPACE AddressSpace)
{
   InitializeListHead(&AddressSpace->MAreaListHead);
   KeInitializeMutex(&AddressSpace->Lock, 1);
   AddressSpace->LowestAddress = MM_LOWEST_USER_ADDRESS;
   AddressSpace->Process = Process;
   if (Process != NULL)
     {
	MmInitializeWorkingSet(Process, AddressSpace);
     }
   return(STATUS_SUCCESS);
}

NTSTATUS MmDestroyAddressSpace(PMADDRESS_SPACE AddressSpace)
{
   return(STATUS_SUCCESS);
}
