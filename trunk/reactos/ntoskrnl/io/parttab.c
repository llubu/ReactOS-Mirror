/* $Id: parttab.c,v 1.4 2003/02/26 14:12:43 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/parttab.c
 * PURPOSE:         Handling fixed disks (partition table functions)
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 * 2000-03-25 (ea)
 * 	Moved here from ntoskrnl/io/fdisk.c
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL
IoReadPartitionTable(PDEVICE_OBJECT DeviceObject,
		     ULONG SectorSize,
		     BOOLEAN ReturnRecognizedPartitions,
		     PDRIVE_LAYOUT_INFORMATION *PartitionBuffer)
{
  return(HalIoReadPartitionTable(DeviceObject,
				 SectorSize,
				 ReturnRecognizedPartitions,
				 PartitionBuffer));
}


NTSTATUS STDCALL
IoSetPartitionInformation(PDEVICE_OBJECT DeviceObject,
			  ULONG SectorSize,
			  ULONG PartitionNumber,
			  ULONG PartitionType)
{
  return(HalIoSetPartitionInformation(DeviceObject,
				      SectorSize,
				      PartitionNumber,
				      PartitionType));
}


NTSTATUS STDCALL
IoWritePartitionTable(PDEVICE_OBJECT DeviceObject,
		      ULONG SectorSize,
		      ULONG SectorsPerTrack,
		      ULONG NumberOfHeads,
		      PDRIVE_LAYOUT_INFORMATION PartitionBuffer)
{
  return(HalIoWritePartitionTable(DeviceObject,
				  SectorSize,
				  SectorsPerTrack,
				  NumberOfHeads,
				  PartitionBuffer));
}

/* EOF */
