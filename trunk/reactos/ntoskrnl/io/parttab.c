/* $Id: parttab.c,v 1.8 2004/08/15 16:39:03 chorns Exp $
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

#include <ntoskrnl.h>
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
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
