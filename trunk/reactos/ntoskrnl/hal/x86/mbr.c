/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/hal/x86/mbr.c
 * PURPOSE:         Functions for reading the master boot record (MBR)
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

VOID HalExamineMBR(PDEVICE_OBJECT DeviceObject,
		   ULONG SectorSize,
		   ULONG MBRTypeIdentifier,
		   PVOID Buffer)
{
   UNIMPLEMENTED;
}
