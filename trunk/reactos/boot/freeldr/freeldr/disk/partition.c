/*
 *  FreeLoader
 *  Copyright (C) 1998-2003  Brian Palmer  <brianp@sginet.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef _M_ARM
#include <freeldr.h>
#include <debug.h>

BOOLEAN DiskGetActivePartitionEntry(ULONG DriveNumber,
                                 PPARTITION_TABLE_ENTRY PartitionTableEntry,
                                 ULONG *ActivePartition)
{
	ULONG			BootablePartitionCount = 0;
	MASTER_BOOT_RECORD	MasterBootRecord;

	*ActivePartition = 0;
	// Read master boot record
	if (!DiskReadBootRecord(DriveNumber, 0, &MasterBootRecord))
	{
		return FALSE;
	}

	// Count the bootable partitions
	if (MasterBootRecord.PartitionTable[0].BootIndicator == 0x80)
	{
		BootablePartitionCount++;
		*ActivePartition = 1;
	}
	if (MasterBootRecord.PartitionTable[1].BootIndicator == 0x80)
	{
		BootablePartitionCount++;
		*ActivePartition = 2;
	}
	if (MasterBootRecord.PartitionTable[2].BootIndicator == 0x80)
	{
		BootablePartitionCount++;
		*ActivePartition = 3;
	}
	if (MasterBootRecord.PartitionTable[3].BootIndicator == 0x80)
	{
		BootablePartitionCount++;
		*ActivePartition = 4;
	}

	// Make sure there was only one bootable partition
	if (BootablePartitionCount == 0)
	{
		DPRINTM(DPRINT_DISK, "No bootable (active) partitions found.\n");
		return FALSE;
	}
	else if (BootablePartitionCount != 1)
	{
		DPRINTM(DPRINT_DISK, "Too many bootable (active) partitions found.\n");
		return FALSE;
	}

	// Copy the partition table entry
	RtlCopyMemory(PartitionTableEntry,
	              &MasterBootRecord.PartitionTable[*ActivePartition - 1],
	              sizeof(PARTITION_TABLE_ENTRY));

	return TRUE;
}

BOOLEAN DiskGetPartitionEntry(ULONG DriveNumber, ULONG PartitionNumber, PPARTITION_TABLE_ENTRY PartitionTableEntry)
{
	MASTER_BOOT_RECORD		MasterBootRecord;
	PARTITION_TABLE_ENTRY	ExtendedPartitionTableEntry;
	ULONG						ExtendedPartitionNumber;
	ULONG						ExtendedPartitionOffset;
	ULONG						Index;

	// Read master boot record
	if (!DiskReadBootRecord(DriveNumber, 0, &MasterBootRecord))
	{
		return FALSE;
	}

	// If they are asking for a primary
	// partition then things are easy
	if (PartitionNumber < 5)
	{
		// PartitionNumber is one-based and we need it zero-based
		PartitionNumber--;

		// Copy the partition table entry
		RtlCopyMemory(PartitionTableEntry, &MasterBootRecord.PartitionTable[PartitionNumber], sizeof(PARTITION_TABLE_ENTRY));

		return TRUE;
	}
	else
	{
		// They want an extended partition entry so we will need
		// to loop through all the extended partitions on the disk
		// and return the one they want.

		ExtendedPartitionNumber = PartitionNumber - 5;

		// Set the initial relative starting sector to 0
		// This is because extended partition starting
		// sectors a numbered relative to their parent
		ExtendedPartitionOffset = 0;

		for (Index=0; Index<=ExtendedPartitionNumber; Index++)
		{
			// Get the extended partition table entry
			if (!DiskGetFirstExtendedPartitionEntry(&MasterBootRecord, &ExtendedPartitionTableEntry))
			{
				return FALSE;
			}

			// Adjust the relative starting sector of the partition
			ExtendedPartitionTableEntry.SectorCountBeforePartition += ExtendedPartitionOffset;
			if (ExtendedPartitionOffset == 0)
			{
				// Set the start of the parrent extended partition
				ExtendedPartitionOffset = ExtendedPartitionTableEntry.SectorCountBeforePartition;
			}
			// Read the partition boot record
    			if (!DiskReadBootRecord(DriveNumber, ExtendedPartitionTableEntry.SectorCountBeforePartition, &MasterBootRecord))
			{
				return FALSE;
			}

			// Get the first real partition table entry
			if (!DiskGetFirstPartitionEntry(&MasterBootRecord, PartitionTableEntry))
			{
				return FALSE;
			}

			// Now correct the start sector of the partition
			PartitionTableEntry->SectorCountBeforePartition += ExtendedPartitionTableEntry.SectorCountBeforePartition;
		}

		// When we get here we should have the correct entry
		// already stored in PartitionTableEntry
		// so just return TRUE
		return TRUE;
	}

}

BOOLEAN DiskGetFirstPartitionEntry(PMASTER_BOOT_RECORD MasterBootRecord, PPARTITION_TABLE_ENTRY PartitionTableEntry)
{
	ULONG		Index;

	for (Index=0; Index<4; Index++)
	{
		// Check the system indicator
		// If it's not an extended or unused partition
		// then we're done
		if ((MasterBootRecord->PartitionTable[Index].SystemIndicator != PARTITION_ENTRY_UNUSED) &&
			(MasterBootRecord->PartitionTable[Index].SystemIndicator != PARTITION_EXTENDED) &&
			(MasterBootRecord->PartitionTable[Index].SystemIndicator != PARTITION_XINT13_EXTENDED))
		{
			RtlCopyMemory(PartitionTableEntry, &MasterBootRecord->PartitionTable[Index], sizeof(PARTITION_TABLE_ENTRY));
			return TRUE;
		}
	}

	return FALSE;
}

BOOLEAN DiskGetFirstExtendedPartitionEntry(PMASTER_BOOT_RECORD MasterBootRecord, PPARTITION_TABLE_ENTRY PartitionTableEntry)
{
	ULONG		Index;

	for (Index=0; Index<4; Index++)
	{
		// Check the system indicator
		// If it an extended partition then we're done
		if ((MasterBootRecord->PartitionTable[Index].SystemIndicator == PARTITION_EXTENDED) ||
			(MasterBootRecord->PartitionTable[Index].SystemIndicator == PARTITION_XINT13_EXTENDED))
		{
			RtlCopyMemory(PartitionTableEntry, &MasterBootRecord->PartitionTable[Index], sizeof(PARTITION_TABLE_ENTRY));
			return TRUE;
		}
	}

	return FALSE;
}

BOOLEAN DiskReadBootRecord(ULONG DriveNumber, ULONGLONG LogicalSectorNumber, PMASTER_BOOT_RECORD BootRecord)
{
	char		ErrMsg[64];
	ULONG		Index;

	// Read master boot record
	if (!MachDiskReadLogicalSectors(DriveNumber, LogicalSectorNumber, 1, (PVOID)DISKREADBUFFER))
	{
		return FALSE;
	}
	RtlCopyMemory(BootRecord, (PVOID)DISKREADBUFFER, sizeof(MASTER_BOOT_RECORD));


	DPRINTM(DPRINT_DISK, "Dumping partition table for drive 0x%x:\n", DriveNumber);
	DPRINTM(DPRINT_DISK, "Boot record logical start sector = %d\n", LogicalSectorNumber);
	DPRINTM(DPRINT_DISK, "sizeof(MASTER_BOOT_RECORD) = 0x%x.\n", sizeof(MASTER_BOOT_RECORD));

	for (Index=0; Index<4; Index++)
	{
		DPRINTM(DPRINT_DISK, "-------------------------------------------\n");
		DPRINTM(DPRINT_DISK, "Partition %d\n", (Index + 1));
		DPRINTM(DPRINT_DISK, "BootIndicator: 0x%x\n", BootRecord->PartitionTable[Index].BootIndicator);
		DPRINTM(DPRINT_DISK, "StartHead: 0x%x\n", BootRecord->PartitionTable[Index].StartHead);
		DPRINTM(DPRINT_DISK, "StartSector (Plus 2 cylinder bits): 0x%x\n", BootRecord->PartitionTable[Index].StartSector);
		DPRINTM(DPRINT_DISK, "StartCylinder: 0x%x\n", BootRecord->PartitionTable[Index].StartCylinder);
		DPRINTM(DPRINT_DISK, "SystemIndicator: 0x%x\n", BootRecord->PartitionTable[Index].SystemIndicator);
		DPRINTM(DPRINT_DISK, "EndHead: 0x%x\n", BootRecord->PartitionTable[Index].EndHead);
		DPRINTM(DPRINT_DISK, "EndSector (Plus 2 cylinder bits): 0x%x\n", BootRecord->PartitionTable[Index].EndSector);
		DPRINTM(DPRINT_DISK, "EndCylinder: 0x%x\n", BootRecord->PartitionTable[Index].EndCylinder);
		DPRINTM(DPRINT_DISK, "SectorCountBeforePartition: 0x%x\n", BootRecord->PartitionTable[Index].SectorCountBeforePartition);
		DPRINTM(DPRINT_DISK, "PartitionSectorCount: 0x%x\n", BootRecord->PartitionTable[Index].PartitionSectorCount);
	}

	// Check the partition table magic value
	if (BootRecord->MasterBootRecordMagic != 0xaa55)
	{
		sprintf(ErrMsg, "Invalid partition table magic 0x%x found on drive 0x%lx",
		        BootRecord->MasterBootRecordMagic, DriveNumber);
		DiskError(ErrMsg, 0);
		return FALSE;
	}

	return TRUE;
}

#endif
