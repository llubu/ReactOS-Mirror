/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            kernel/base/bug.c
 * PURPOSE:         Graceful system shutdown if a bug is detected
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <internal/kernel.h>
#include <internal/linkage.h>
#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

VOID RtlStoreUlong(PULONG Address,
		   ULONG Value)
{
   *Address=Value;
}

VOID RtlStoreUshort(PUSHORT Address,
		    USHORT Value)
{
   *Address=Value;
}

VOID RtlRetrieveUlong(PULONG DestinationAddress,
		      PULONG SourceAddress)
{
   *DestinationAddress = *SourceAddress;
}

VOID RtlRetrieveUshort(PUSHORT DestinationAddress,
		       PUSHORT SourceAddress)
{
   *DestinationAddress = *SourceAddress;
}
