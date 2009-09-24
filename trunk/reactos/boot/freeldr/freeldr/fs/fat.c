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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <freeldr.h>

#define NDEBUG
#include <debug.h>

ULONG	FatDetermineFatType(PFAT_BOOTSECTOR FatBootSector, ULONG PartitionSectorCount);
PVOID	FatBufferDirectory(ULONG DirectoryStartCluster, ULONG* EntryCountPointer, BOOLEAN RootDirectory);
BOOLEAN	FatSearchDirectoryBufferForFile(PVOID DirectoryBuffer, ULONG EntryCount, PCHAR FileName, PFAT_FILE_INFO FatFileInfoPointer);
LONG FatLookupFile(PCSTR FileName, ULONG DeviceId, PFAT_FILE_INFO FatFileInfoPointer);
void	FatParseShortFileName(PCHAR Buffer, PDIRENTRY DirEntry);
BOOLEAN	FatGetFatEntry(ULONG Cluster, ULONG* ClusterPointer);
ULONG	FatCountClustersInChain(ULONG StartCluster);
ULONG*	FatGetClusterChainArray(ULONG StartCluster);
BOOLEAN	FatReadCluster(ULONG ClusterNumber, PVOID Buffer);
BOOLEAN	FatReadClusterChain(ULONG StartClusterNumber, ULONG NumberOfClusters, PVOID Buffer);
BOOLEAN	FatReadPartialCluster(ULONG ClusterNumber, ULONG StartingOffset, ULONG Length, PVOID Buffer);
BOOLEAN	FatReadFile(PFAT_FILE_INFO FatFileInfo, ULONG BytesToRead, ULONG* BytesRead, PVOID Buffer);
BOOLEAN	FatReadVolumeSectors(ULONG DriveNumber, ULONG SectorNumber, ULONG SectorCount, PVOID Buffer);

ULONG			BytesPerSector;			/* Number of bytes per sector */
ULONG			SectorsPerCluster;		/* Number of sectors per cluster */
ULONG			FatVolumeStartSector;		/* Absolute starting sector of the partition */
ULONG			FatSectorStart;			/* Starting sector of 1st FAT table */
ULONG			ActiveFatSectorStart;		/* Starting sector of active FAT table */
ULONG			NumberOfFats;			/* Number of FAT tables */
ULONG			SectorsPerFat;			/* Sectors per FAT table */
ULONG			RootDirSectorStart;		/* Starting sector of the root directory (non-fat32) */
ULONG			RootDirSectors;			/* Number of sectors of the root directory (non-fat32) */
ULONG			RootDirStartCluster;		/* Starting cluster number of the root directory (fat32 only) */
ULONG			DataSectorStart;		/* Starting sector of the data area */

ULONG			FatType = 0;			/* FAT12, FAT16, FAT32, FATX16 or FATX32 */
ULONG			FatDriveNumber = 0;

VOID FatSwapFatBootSector(PFAT_BOOTSECTOR Obj)
{
	SW(Obj, BytesPerSector);
	SW(Obj, ReservedSectors);
	SW(Obj, RootDirEntries);
	SW(Obj, TotalSectors);
	SW(Obj, SectorsPerFat);
	SW(Obj, SectorsPerTrack);
	SW(Obj, NumberOfHeads);
	SD(Obj, HiddenSectors);
	SD(Obj, TotalSectorsBig);
	SD(Obj, VolumeSerialNumber);
	SW(Obj, BootSectorMagic);
}

VOID FatSwapFat32BootSector(PFAT32_BOOTSECTOR Obj)
{
	SW(Obj, BytesPerSector);
	SW(Obj, ReservedSectors);
	SW(Obj, RootDirEntries);
	SW(Obj, TotalSectors);
	SW(Obj, SectorsPerFat);
	SW(Obj, NumberOfHeads);
	SD(Obj, HiddenSectors);
	SD(Obj, TotalSectorsBig);
	SD(Obj, SectorsPerFatBig);
	SW(Obj, ExtendedFlags);
	SW(Obj, FileSystemVersion);
	SD(Obj, RootDirStartCluster);
	SW(Obj, FsInfo);
	SW(Obj, BackupBootSector);
	SD(Obj, VolumeSerialNumber);
	SW(Obj, BootSectorMagic);
}

VOID FatSwapFatXBootSector(PFATX_BOOTSECTOR Obj)
{
	SD(Obj, VolumeSerialNumber);
	SD(Obj, SectorsPerCluster);
	SW(Obj, NumberOfFats);
}

VOID FatSwapDirEntry(PDIRENTRY Obj)
{
	SW(Obj, CreateTime);
	SW(Obj, CreateDate);
	SW(Obj, LastAccessDate);
	SW(Obj, ClusterHigh);
	SW(Obj, Time);
	SW(Obj, Date);
	SW(Obj, ClusterLow);
	SD(Obj, Size);
}

VOID FatSwapLFNDirEntry(PLFN_DIRENTRY Obj)
{
	int i;
	SW(Obj, StartCluster);
	for(i = 0; i < 5; i++)
		Obj->Name0_4[i] = SWAPW(Obj->Name0_4[i]);
	for(i = 0; i < 6; i++)
		Obj->Name5_10[i] = SWAPW(Obj->Name5_10[i]);
	for(i = 0; i < 2; i++)
		Obj->Name11_12[i] = SWAPW(Obj->Name11_12[i]);
}

VOID FatSwapFatXDirEntry(PFATX_DIRENTRY Obj)
{
	SD(Obj, StartCluster);
	SD(Obj, Size);
	SW(Obj, Time);
	SW(Obj, Date);
	SW(Obj, CreateTime);
	SW(Obj, CreateDate);
	SW(Obj, LastAccessTime);
	SW(Obj, LastAccessDate);
}

BOOLEAN FatOpenVolume(UCHAR DriveNumber, ULONGLONG VolumeStartSector, ULONGLONG PartitionSectorCount)
{
	char ErrMsg[80];
	ULONG FatSize;
	PFAT_BOOTSECTOR	FatVolumeBootSector;
	PFAT32_BOOTSECTOR Fat32VolumeBootSector;
	PFATX_BOOTSECTOR FatXVolumeBootSector;

	DPRINTM(DPRINT_FILESYSTEM, "FatOpenVolume() DriveNumber = 0x%x VolumeStartSector = %d\n", DriveNumber, VolumeStartSector);

	// Store the drive number
	FatDriveNumber = DriveNumber;

	//
	// Allocate the memory to hold the boot sector
	//
	FatVolumeBootSector = (PFAT_BOOTSECTOR) MmHeapAlloc(512);
	Fat32VolumeBootSector = (PFAT32_BOOTSECTOR) FatVolumeBootSector;
	FatXVolumeBootSector = (PFATX_BOOTSECTOR) FatVolumeBootSector;

	//
	// Make sure we got the memory
	//
	if (FatVolumeBootSector == NULL)
	{
		FileSystemError("Out of memory.");
		return FALSE;
	}

	// Now try to read the boot sector
	// If this fails then abort
	if (!MachDiskReadLogicalSectors(DriveNumber, VolumeStartSector, 1, (PVOID)DISKREADBUFFER))
	{
		MmHeapFree(FatVolumeBootSector);
		return FALSE;
	}
	RtlCopyMemory(FatVolumeBootSector, (PVOID)DISKREADBUFFER, 512);

	// Get the FAT type
	FatType = FatDetermineFatType(FatVolumeBootSector, PartitionSectorCount);

	// Dump boot sector (and swap it for big endian systems)
	DPRINTM(DPRINT_FILESYSTEM, "Dumping boot sector:\n");
	if (ISFATX(FatType))
	{
		FatSwapFatXBootSector(FatXVolumeBootSector);
		DPRINTM(DPRINT_FILESYSTEM, "sizeof(FATX_BOOTSECTOR) = 0x%x.\n", sizeof(FATX_BOOTSECTOR));

		DPRINTM(DPRINT_FILESYSTEM, "FileSystemType: %c%c%c%c.\n", FatXVolumeBootSector->FileSystemType[0], FatXVolumeBootSector->FileSystemType[1], FatXVolumeBootSector->FileSystemType[2], FatXVolumeBootSector->FileSystemType[3]);
		DPRINTM(DPRINT_FILESYSTEM, "VolumeSerialNumber: 0x%x\n", FatXVolumeBootSector->VolumeSerialNumber);
		DPRINTM(DPRINT_FILESYSTEM, "SectorsPerCluster: %d\n", FatXVolumeBootSector->SectorsPerCluster);
		DPRINTM(DPRINT_FILESYSTEM, "NumberOfFats: %d\n", FatXVolumeBootSector->NumberOfFats);
		DPRINTM(DPRINT_FILESYSTEM, "Unknown: 0x%x\n", FatXVolumeBootSector->Unknown);

		DPRINTM(DPRINT_FILESYSTEM, "FatType %s\n", FatType == FATX16 ? "FATX16" : "FATX32");

	}
	else if (FatType == FAT32)
	{
		FatSwapFat32BootSector(Fat32VolumeBootSector);
		DPRINTM(DPRINT_FILESYSTEM, "sizeof(FAT32_BOOTSECTOR) = 0x%x.\n", sizeof(FAT32_BOOTSECTOR));

		DPRINTM(DPRINT_FILESYSTEM, "JumpBoot: 0x%x 0x%x 0x%x\n", Fat32VolumeBootSector->JumpBoot[0], Fat32VolumeBootSector->JumpBoot[1], Fat32VolumeBootSector->JumpBoot[2]);
		DPRINTM(DPRINT_FILESYSTEM, "OemName: %c%c%c%c%c%c%c%c\n", Fat32VolumeBootSector->OemName[0], Fat32VolumeBootSector->OemName[1], Fat32VolumeBootSector->OemName[2], Fat32VolumeBootSector->OemName[3], Fat32VolumeBootSector->OemName[4], Fat32VolumeBootSector->OemName[5], Fat32VolumeBootSector->OemName[6], Fat32VolumeBootSector->OemName[7]);
		DPRINTM(DPRINT_FILESYSTEM, "BytesPerSector: %d\n", Fat32VolumeBootSector->BytesPerSector);
		DPRINTM(DPRINT_FILESYSTEM, "SectorsPerCluster: %d\n", Fat32VolumeBootSector->SectorsPerCluster);
		DPRINTM(DPRINT_FILESYSTEM, "ReservedSectors: %d\n", Fat32VolumeBootSector->ReservedSectors);
		DPRINTM(DPRINT_FILESYSTEM, "NumberOfFats: %d\n", Fat32VolumeBootSector->NumberOfFats);
		DPRINTM(DPRINT_FILESYSTEM, "RootDirEntries: %d\n", Fat32VolumeBootSector->RootDirEntries);
		DPRINTM(DPRINT_FILESYSTEM, "TotalSectors: %d\n", Fat32VolumeBootSector->TotalSectors);
		DPRINTM(DPRINT_FILESYSTEM, "MediaDescriptor: 0x%x\n", Fat32VolumeBootSector->MediaDescriptor);
		DPRINTM(DPRINT_FILESYSTEM, "SectorsPerFat: %d\n", Fat32VolumeBootSector->SectorsPerFat);
		DPRINTM(DPRINT_FILESYSTEM, "SectorsPerTrack: %d\n", Fat32VolumeBootSector->SectorsPerTrack);
		DPRINTM(DPRINT_FILESYSTEM, "NumberOfHeads: %d\n", Fat32VolumeBootSector->NumberOfHeads);
		DPRINTM(DPRINT_FILESYSTEM, "HiddenSectors: %d\n", Fat32VolumeBootSector->HiddenSectors);
		DPRINTM(DPRINT_FILESYSTEM, "TotalSectorsBig: %d\n", Fat32VolumeBootSector->TotalSectorsBig);
		DPRINTM(DPRINT_FILESYSTEM, "SectorsPerFatBig: %d\n", Fat32VolumeBootSector->SectorsPerFatBig);
		DPRINTM(DPRINT_FILESYSTEM, "ExtendedFlags: 0x%x\n", Fat32VolumeBootSector->ExtendedFlags);
		DPRINTM(DPRINT_FILESYSTEM, "FileSystemVersion: 0x%x\n", Fat32VolumeBootSector->FileSystemVersion);
		DPRINTM(DPRINT_FILESYSTEM, "RootDirStartCluster: %d\n", Fat32VolumeBootSector->RootDirStartCluster);
		DPRINTM(DPRINT_FILESYSTEM, "FsInfo: %d\n", Fat32VolumeBootSector->FsInfo);
		DPRINTM(DPRINT_FILESYSTEM, "BackupBootSector: %d\n", Fat32VolumeBootSector->BackupBootSector);
		DPRINTM(DPRINT_FILESYSTEM, "Reserved: 0x%x\n", Fat32VolumeBootSector->Reserved);
		DPRINTM(DPRINT_FILESYSTEM, "DriveNumber: 0x%x\n", Fat32VolumeBootSector->DriveNumber);
		DPRINTM(DPRINT_FILESYSTEM, "Reserved1: 0x%x\n", Fat32VolumeBootSector->Reserved1);
		DPRINTM(DPRINT_FILESYSTEM, "BootSignature: 0x%x\n", Fat32VolumeBootSector->BootSignature);
		DPRINTM(DPRINT_FILESYSTEM, "VolumeSerialNumber: 0x%x\n", Fat32VolumeBootSector->VolumeSerialNumber);
		DPRINTM(DPRINT_FILESYSTEM, "VolumeLabel: %c%c%c%c%c%c%c%c%c%c%c\n", Fat32VolumeBootSector->VolumeLabel[0], Fat32VolumeBootSector->VolumeLabel[1], Fat32VolumeBootSector->VolumeLabel[2], Fat32VolumeBootSector->VolumeLabel[3], Fat32VolumeBootSector->VolumeLabel[4], Fat32VolumeBootSector->VolumeLabel[5], Fat32VolumeBootSector->VolumeLabel[6], Fat32VolumeBootSector->VolumeLabel[7], Fat32VolumeBootSector->VolumeLabel[8], Fat32VolumeBootSector->VolumeLabel[9], Fat32VolumeBootSector->VolumeLabel[10]);
		DPRINTM(DPRINT_FILESYSTEM, "FileSystemType: %c%c%c%c%c%c%c%c\n", Fat32VolumeBootSector->FileSystemType[0], Fat32VolumeBootSector->FileSystemType[1], Fat32VolumeBootSector->FileSystemType[2], Fat32VolumeBootSector->FileSystemType[3], Fat32VolumeBootSector->FileSystemType[4], Fat32VolumeBootSector->FileSystemType[5], Fat32VolumeBootSector->FileSystemType[6], Fat32VolumeBootSector->FileSystemType[7]);
		DPRINTM(DPRINT_FILESYSTEM, "BootSectorMagic: 0x%x\n", Fat32VolumeBootSector->BootSectorMagic);
	}
	else
	{
		FatSwapFatBootSector(FatVolumeBootSector);
		DPRINTM(DPRINT_FILESYSTEM, "sizeof(FAT_BOOTSECTOR) = 0x%x.\n", sizeof(FAT_BOOTSECTOR));

		DPRINTM(DPRINT_FILESYSTEM, "JumpBoot: 0x%x 0x%x 0x%x\n", FatVolumeBootSector->JumpBoot[0], FatVolumeBootSector->JumpBoot[1], FatVolumeBootSector->JumpBoot[2]);
		DPRINTM(DPRINT_FILESYSTEM, "OemName: %c%c%c%c%c%c%c%c\n", FatVolumeBootSector->OemName[0], FatVolumeBootSector->OemName[1], FatVolumeBootSector->OemName[2], FatVolumeBootSector->OemName[3], FatVolumeBootSector->OemName[4], FatVolumeBootSector->OemName[5], FatVolumeBootSector->OemName[6], FatVolumeBootSector->OemName[7]);
		DPRINTM(DPRINT_FILESYSTEM, "BytesPerSector: %d\n", FatVolumeBootSector->BytesPerSector);
		DPRINTM(DPRINT_FILESYSTEM, "SectorsPerCluster: %d\n", FatVolumeBootSector->SectorsPerCluster);
		DPRINTM(DPRINT_FILESYSTEM, "ReservedSectors: %d\n", FatVolumeBootSector->ReservedSectors);
		DPRINTM(DPRINT_FILESYSTEM, "NumberOfFats: %d\n", FatVolumeBootSector->NumberOfFats);
		DPRINTM(DPRINT_FILESYSTEM, "RootDirEntries: %d\n", FatVolumeBootSector->RootDirEntries);
		DPRINTM(DPRINT_FILESYSTEM, "TotalSectors: %d\n", FatVolumeBootSector->TotalSectors);
		DPRINTM(DPRINT_FILESYSTEM, "MediaDescriptor: 0x%x\n", FatVolumeBootSector->MediaDescriptor);
		DPRINTM(DPRINT_FILESYSTEM, "SectorsPerFat: %d\n", FatVolumeBootSector->SectorsPerFat);
		DPRINTM(DPRINT_FILESYSTEM, "SectorsPerTrack: %d\n", FatVolumeBootSector->SectorsPerTrack);
		DPRINTM(DPRINT_FILESYSTEM, "NumberOfHeads: %d\n", FatVolumeBootSector->NumberOfHeads);
		DPRINTM(DPRINT_FILESYSTEM, "HiddenSectors: %d\n", FatVolumeBootSector->HiddenSectors);
		DPRINTM(DPRINT_FILESYSTEM, "TotalSectorsBig: %d\n", FatVolumeBootSector->TotalSectorsBig);
		DPRINTM(DPRINT_FILESYSTEM, "DriveNumber: 0x%x\n", FatVolumeBootSector->DriveNumber);
		DPRINTM(DPRINT_FILESYSTEM, "Reserved1: 0x%x\n", FatVolumeBootSector->Reserved1);
		DPRINTM(DPRINT_FILESYSTEM, "BootSignature: 0x%x\n", FatVolumeBootSector->BootSignature);
		DPRINTM(DPRINT_FILESYSTEM, "VolumeSerialNumber: 0x%x\n", FatVolumeBootSector->VolumeSerialNumber);
		DPRINTM(DPRINT_FILESYSTEM, "VolumeLabel: %c%c%c%c%c%c%c%c%c%c%c\n", FatVolumeBootSector->VolumeLabel[0], FatVolumeBootSector->VolumeLabel[1], FatVolumeBootSector->VolumeLabel[2], FatVolumeBootSector->VolumeLabel[3], FatVolumeBootSector->VolumeLabel[4], FatVolumeBootSector->VolumeLabel[5], FatVolumeBootSector->VolumeLabel[6], FatVolumeBootSector->VolumeLabel[7], FatVolumeBootSector->VolumeLabel[8], FatVolumeBootSector->VolumeLabel[9], FatVolumeBootSector->VolumeLabel[10]);
		DPRINTM(DPRINT_FILESYSTEM, "FileSystemType: %c%c%c%c%c%c%c%c\n", FatVolumeBootSector->FileSystemType[0], FatVolumeBootSector->FileSystemType[1], FatVolumeBootSector->FileSystemType[2], FatVolumeBootSector->FileSystemType[3], FatVolumeBootSector->FileSystemType[4], FatVolumeBootSector->FileSystemType[5], FatVolumeBootSector->FileSystemType[6], FatVolumeBootSector->FileSystemType[7]);
		DPRINTM(DPRINT_FILESYSTEM, "BootSectorMagic: 0x%x\n", FatVolumeBootSector->BootSectorMagic);
	}

	//
	// Set the correct partition offset
	//
	FatVolumeStartSector = VolumeStartSector;

	//
	// Check the boot sector magic
	//
	if (! ISFATX(FatType) && FatVolumeBootSector->BootSectorMagic != 0xaa55)
	{
		sprintf(ErrMsg, "Invalid boot sector magic on drive 0x%x (expected 0xaa55 found 0x%x)",
		        DriveNumber, FatVolumeBootSector->BootSectorMagic);
		FileSystemError(ErrMsg);
		MmHeapFree(FatVolumeBootSector);
		return FALSE;
	}

	//
	// Check the FAT cluster size
	// We do not support clusters bigger than 64k
	//
	if ((ISFATX(FatType) && 64 * 1024 < FatXVolumeBootSector->SectorsPerCluster * 512) ||
	   (! ISFATX(FatType) && 64 * 1024 < FatVolumeBootSector->SectorsPerCluster * FatVolumeBootSector->BytesPerSector))
	{
		FileSystemError("This file system has cluster sizes bigger than 64k.\nFreeLoader does not support this.");
		MmHeapFree(FatVolumeBootSector);
		return FALSE;
	}

	//
	// Clear our variables
	//
	FatSectorStart = 0;
	ActiveFatSectorStart = 0;
	NumberOfFats = 0;
	RootDirSectorStart = 0;
	DataSectorStart = 0;
	SectorsPerFat = 0;
	RootDirSectors = 0;

	//
	// Get the sectors per FAT,
	// root directory starting sector,
	// and data sector start
	//
	if (ISFATX(FatType))
	{
		BytesPerSector = 512;
		SectorsPerCluster = SWAPD(FatXVolumeBootSector->SectorsPerCluster);
		FatSectorStart = (4096 / BytesPerSector);
		ActiveFatSectorStart = FatSectorStart;
		NumberOfFats = 1;
		FatSize = PartitionSectorCount / SectorsPerCluster *
		          (FATX16 == FatType ? 2 : 4);
		SectorsPerFat = (((FatSize + 4095) / 4096) * 4096) / BytesPerSector;

		RootDirSectorStart = FatSectorStart + NumberOfFats * SectorsPerFat;
		RootDirSectors = FatXVolumeBootSector->SectorsPerCluster;

		DataSectorStart = RootDirSectorStart + RootDirSectors;
	}
	else if (FatType != FAT32)
	{
		BytesPerSector = FatVolumeBootSector->BytesPerSector;
		SectorsPerCluster = FatVolumeBootSector->SectorsPerCluster;
		FatSectorStart = FatVolumeBootSector->ReservedSectors;
		ActiveFatSectorStart = FatSectorStart;
		NumberOfFats = FatVolumeBootSector->NumberOfFats;
		SectorsPerFat = FatVolumeBootSector->SectorsPerFat;

		RootDirSectorStart = FatSectorStart + NumberOfFats * SectorsPerFat;
		RootDirSectors = ((FatVolumeBootSector->RootDirEntries * 32) + (BytesPerSector - 1)) / BytesPerSector;

		DataSectorStart = RootDirSectorStart + RootDirSectors;
	}
	else
	{
		BytesPerSector = Fat32VolumeBootSector->BytesPerSector;
		SectorsPerCluster = Fat32VolumeBootSector->SectorsPerCluster;
		FatSectorStart = Fat32VolumeBootSector->ReservedSectors;
		ActiveFatSectorStart = FatSectorStart +
		                       ((Fat32VolumeBootSector->ExtendedFlags & 0x80) ? ((Fat32VolumeBootSector->ExtendedFlags & 0x0f) * Fat32VolumeBootSector->SectorsPerFatBig) : 0);
		NumberOfFats = Fat32VolumeBootSector->NumberOfFats;
		SectorsPerFat = Fat32VolumeBootSector->SectorsPerFatBig;

		RootDirStartCluster = Fat32VolumeBootSector->RootDirStartCluster;
		DataSectorStart = FatSectorStart + NumberOfFats * SectorsPerFat;

		//
		// Check version
		// we only work with version 0
		//
		if (Fat32VolumeBootSector->FileSystemVersion != 0)
		{
			FileSystemError("FreeLoader is too old to work with this FAT32 filesystem.\nPlease update FreeLoader.");
			return FALSE;
		}
	}
	MmHeapFree(FatVolumeBootSector);

	{
		GEOMETRY DriveGeometry;
		ULONG BlockSize;

		// Initialize drive by getting its geometry
		if (!MachDiskGetDriveGeometry(DriveNumber, &DriveGeometry))
		{
			return FALSE;
		}

		BlockSize = MachDiskGetCacheableBlockCount(DriveNumber);
	}

	return TRUE;
}

ULONG FatDetermineFatType(PFAT_BOOTSECTOR FatBootSector, ULONG PartitionSectorCount)
{
	ULONG			RootDirSectors;
	ULONG			DataSectorCount;
	ULONG			SectorsPerFat;
	ULONG			TotalSectors;
	ULONG			CountOfClusters;
	PFAT32_BOOTSECTOR	Fat32BootSector = (PFAT32_BOOTSECTOR)FatBootSector;
	PFATX_BOOTSECTOR	FatXBootSector = (PFATX_BOOTSECTOR)FatBootSector;

	if (0 == strncmp(FatXBootSector->FileSystemType, "FATX", 4))
	{
		CountOfClusters = PartitionSectorCount / FatXBootSector->SectorsPerCluster;
		if (CountOfClusters < 65525)
		{
			/* Volume is FATX16 */
			return FATX16;
		}
		else
		{
			/* Volume is FAT32 */
			return FATX32;
		}
	}
	else
	{
		RootDirSectors = ((SWAPW(FatBootSector->RootDirEntries) * 32) + (SWAPW(FatBootSector->BytesPerSector) - 1)) / SWAPW(FatBootSector->BytesPerSector);
		SectorsPerFat = SWAPW(FatBootSector->SectorsPerFat) ? SWAPW(FatBootSector->SectorsPerFat) : SWAPD(Fat32BootSector->SectorsPerFatBig);
		TotalSectors = SWAPW(FatBootSector->TotalSectors) ? SWAPW(FatBootSector->TotalSectors) : SWAPD(FatBootSector->TotalSectorsBig);
		DataSectorCount = TotalSectors - (SWAPW(FatBootSector->ReservedSectors) + (FatBootSector->NumberOfFats * SectorsPerFat) + RootDirSectors);

//mjl
		if (FatBootSector->SectorsPerCluster == 0)
			CountOfClusters = 0;
		else
			CountOfClusters = DataSectorCount / FatBootSector->SectorsPerCluster;

		if (CountOfClusters < 4085)
		{
			/* Volume is FAT12 */
			return FAT12;
		}
		else if (CountOfClusters < 65525)
		{
			/* Volume is FAT16 */
			return FAT16;
		}
		else
		{
			/* Volume is FAT32 */
			return FAT32;
		}
	}
}

PVOID FatBufferDirectory(ULONG DirectoryStartCluster, ULONG *DirectorySize, BOOLEAN RootDirectory)
{
	PVOID	DirectoryBuffer;

	DPRINTM(DPRINT_FILESYSTEM, "FatBufferDirectory() DirectoryStartCluster = %d RootDirectory = %s\n", DirectoryStartCluster, (RootDirectory ? "TRUE" : "FALSE"));

	/*
	 * For FAT32, the root directory is nothing special. We can treat it the same
	 * as a subdirectory.
	 */
	if (RootDirectory && FAT32 == FatType)
	{
		DirectoryStartCluster = RootDirStartCluster;
		RootDirectory = FALSE;
	}

	//
	// Calculate the size of the directory
	//
	if (RootDirectory)
	{
		*DirectorySize = RootDirSectors * BytesPerSector;
	}
	else
	{
		*DirectorySize = FatCountClustersInChain(DirectoryStartCluster) * SectorsPerCluster * BytesPerSector;
	}

	//
	// Attempt to allocate memory for directory buffer
	//
	DPRINTM(DPRINT_FILESYSTEM, "Trying to allocate (DirectorySize) %d bytes.\n", *DirectorySize);
	DirectoryBuffer = MmHeapAlloc(*DirectorySize);

	if (DirectoryBuffer == NULL)
	{
		return NULL;
	}

	//
	// Now read directory contents into DirectoryBuffer
	//
	if (RootDirectory)
	{
		if (!FatReadVolumeSectors(FatDriveNumber, RootDirSectorStart, RootDirSectors, DirectoryBuffer))
		{
			MmHeapFree(DirectoryBuffer);
			return NULL;
		}
	}
	else
	{
		if (!FatReadClusterChain(DirectoryStartCluster, 0xFFFFFFFF, DirectoryBuffer))
		{
			MmHeapFree(DirectoryBuffer);
			return NULL;
		}
	}

	return DirectoryBuffer;
}

BOOLEAN FatSearchDirectoryBufferForFile(PVOID DirectoryBuffer, ULONG DirectorySize, PCHAR FileName, PFAT_FILE_INFO FatFileInfoPointer)
{
	ULONG		EntryCount;
	ULONG		CurrentEntry;
	CHAR		LfnNameBuffer[265];
	CHAR		ShortNameBuffer[20];
	ULONG		StartCluster;
	DIRENTRY        OurDirEntry;
	LFN_DIRENTRY    OurLfnDirEntry;
	PDIRENTRY	DirEntry = &OurDirEntry;
	PLFN_DIRENTRY	LfnDirEntry = &OurLfnDirEntry;

	EntryCount = DirectorySize / sizeof(DIRENTRY);

	DPRINTM(DPRINT_FILESYSTEM, "FatSearchDirectoryBufferForFile() DirectoryBuffer = 0x%x EntryCount = %d FileName = %s\n", DirectoryBuffer, EntryCount, FileName);

	memset(ShortNameBuffer, 0, 13 * sizeof(CHAR));
	memset(LfnNameBuffer, 0, 261 * sizeof(CHAR));

	for (CurrentEntry=0; CurrentEntry<EntryCount; CurrentEntry++, DirectoryBuffer = ((PDIRENTRY)DirectoryBuffer)+1)
	{
		OurLfnDirEntry = *((PLFN_DIRENTRY) DirectoryBuffer);
		FatSwapLFNDirEntry(LfnDirEntry);
		OurDirEntry = *((PDIRENTRY) DirectoryBuffer);
		FatSwapDirEntry(DirEntry);

		//DPRINTM(DPRINT_FILESYSTEM, "Dumping directory entry %d:\n", CurrentEntry);
		//DbgDumpBuffer(DPRINT_FILESYSTEM, DirEntry, sizeof(DIRENTRY));

		//
		// Check if this is the last file in the directory
		// If DirEntry[0] == 0x00 then that means all the
		// entries after this one are unused. If this is the
		// last entry then we didn't find the file in this directory.
		//
		if (DirEntry->FileName[0] == '\0')
		{
			return FALSE;
		}

		//
		// Check if this is a deleted entry or not
		//
		if (DirEntry->FileName[0] == '\xE5')
		{
			memset(ShortNameBuffer, 0, 13 * sizeof(CHAR));
			memset(LfnNameBuffer, 0, 261 * sizeof(CHAR));
			continue;
		}

		//
		// Check if this is a LFN entry
		// If so it needs special handling
		//
		if (DirEntry->Attr == ATTR_LONG_NAME)
		{
			//
			// Check to see if this is a deleted LFN entry, if so continue
			//
			if (LfnDirEntry->SequenceNumber & 0x80)
			{
				continue;
			}

			//
			// Mask off high two bits of sequence number
			// and make the sequence number zero-based
			//
			LfnDirEntry->SequenceNumber &= 0x3F;
			LfnDirEntry->SequenceNumber--;

			//
			// Get all 13 LFN entry characters
			//
			if (LfnDirEntry->Name0_4[0] != 0xFFFF)
			{
				LfnNameBuffer[0 + (LfnDirEntry->SequenceNumber * 13)] = (UCHAR)LfnDirEntry->Name0_4[0];
			}
			if (LfnDirEntry->Name0_4[1] != 0xFFFF)
			{
				LfnNameBuffer[1 + (LfnDirEntry->SequenceNumber * 13)] = (UCHAR)LfnDirEntry->Name0_4[1];
			}
			if (LfnDirEntry->Name0_4[2] != 0xFFFF)
			{
				LfnNameBuffer[2 + (LfnDirEntry->SequenceNumber * 13)] = (UCHAR)LfnDirEntry->Name0_4[2];
			}
			if (LfnDirEntry->Name0_4[3] != 0xFFFF)
			{
				LfnNameBuffer[3 + (LfnDirEntry->SequenceNumber * 13)] = (UCHAR)LfnDirEntry->Name0_4[3];
			}
			if (LfnDirEntry->Name0_4[4] != 0xFFFF)
			{
				LfnNameBuffer[4 + (LfnDirEntry->SequenceNumber * 13)] = (UCHAR)LfnDirEntry->Name0_4[4];
			}
			if (LfnDirEntry->Name5_10[0] != 0xFFFF)
			{
				LfnNameBuffer[5 + (LfnDirEntry->SequenceNumber * 13)] = (UCHAR)LfnDirEntry->Name5_10[0];
			}
			if (LfnDirEntry->Name5_10[1] != 0xFFFF)
			{
				LfnNameBuffer[6 + (LfnDirEntry->SequenceNumber * 13)] = (UCHAR)LfnDirEntry->Name5_10[1];
			}
			if (LfnDirEntry->Name5_10[2] != 0xFFFF)
			{
				LfnNameBuffer[7 + (LfnDirEntry->SequenceNumber * 13)] = (UCHAR)LfnDirEntry->Name5_10[2];
			}
			if (LfnDirEntry->Name5_10[3] != 0xFFFF)
			{
				LfnNameBuffer[8 + (LfnDirEntry->SequenceNumber * 13)] = (UCHAR)LfnDirEntry->Name5_10[3];
			}
			if (LfnDirEntry->Name5_10[4] != 0xFFFF)
			{
				LfnNameBuffer[9 + (LfnDirEntry->SequenceNumber * 13)] = (UCHAR)LfnDirEntry->Name5_10[4];
			}
			if (LfnDirEntry->Name5_10[5] != 0xFFFF)
			{
				LfnNameBuffer[10 + (LfnDirEntry->SequenceNumber * 13)] = (UCHAR)LfnDirEntry->Name5_10[5];
			}
			if (LfnDirEntry->Name11_12[0] != 0xFFFF)
			{
				LfnNameBuffer[11 + (LfnDirEntry->SequenceNumber * 13)] = (UCHAR)LfnDirEntry->Name11_12[0];
			}
			if (LfnDirEntry->Name11_12[1] != 0xFFFF)
			{
				LfnNameBuffer[12 + (LfnDirEntry->SequenceNumber * 13)] = (UCHAR)LfnDirEntry->Name11_12[1];
			}

			//DPRINTM(DPRINT_FILESYSTEM, "Dumping long name buffer:\n");
			//DbgDumpBuffer(DPRINT_FILESYSTEM, LfnNameBuffer, 260);

			continue;
		}

		//
		// Check for the volume label attribute
		// and skip over this entry if found
		//
		if (DirEntry->Attr & ATTR_VOLUMENAME)
		{
			memset(ShortNameBuffer, 0, 13 * sizeof(UCHAR));
			memset(LfnNameBuffer, 0, 261 * sizeof(UCHAR));
			continue;
		}

		//
		// If we get here then we've found a short file name
		// entry and LfnNameBuffer contains the long file
		// name or zeroes. All we have to do now is see if the
		// file name matches either the short or long file name
		// and fill in the FAT_FILE_INFO structure if it does
		// or zero our buffers and continue looking.
		//

		//
		// Get short file name
		//
		FatParseShortFileName(ShortNameBuffer, DirEntry);

		DPRINTM(DPRINT_FILESYSTEM, "Entry: %d LFN = %s\n", CurrentEntry, LfnNameBuffer);
		DPRINTM(DPRINT_FILESYSTEM, "Entry: %d DOS name = %s\n", CurrentEntry, ShortNameBuffer);

		//
		// See if the file name matches either the short or long name
		//
		if (((strlen(FileName) == strlen(LfnNameBuffer)) && (_stricmp(FileName, LfnNameBuffer) == 0)) ||
			((strlen(FileName) == strlen(ShortNameBuffer)) && (_stricmp(FileName, ShortNameBuffer) == 0)))		{
			//
			// We found the entry, now fill in the FAT_FILE_INFO struct
			//
			FatFileInfoPointer->FileSize = DirEntry->Size;
			FatFileInfoPointer->FilePointer = 0;

			DPRINTM(DPRINT_FILESYSTEM, "MSDOS Directory Entry:\n");
			DPRINTM(DPRINT_FILESYSTEM, "FileName[11] = %c%c%c%c%c%c%c%c%c%c%c\n", DirEntry->FileName[0], DirEntry->FileName[1], DirEntry->FileName[2], DirEntry->FileName[3], DirEntry->FileName[4], DirEntry->FileName[5], DirEntry->FileName[6], DirEntry->FileName[7], DirEntry->FileName[8], DirEntry->FileName[9], DirEntry->FileName[10]);
			DPRINTM(DPRINT_FILESYSTEM, "Attr = 0x%x\n", DirEntry->Attr);
			DPRINTM(DPRINT_FILESYSTEM, "ReservedNT = 0x%x\n", DirEntry->ReservedNT);
			DPRINTM(DPRINT_FILESYSTEM, "TimeInTenths = %d\n", DirEntry->TimeInTenths);
			DPRINTM(DPRINT_FILESYSTEM, "CreateTime = %d\n", DirEntry->CreateTime);
			DPRINTM(DPRINT_FILESYSTEM, "CreateDate = %d\n", DirEntry->CreateDate);
			DPRINTM(DPRINT_FILESYSTEM, "LastAccessDate = %d\n", DirEntry->LastAccessDate);
			DPRINTM(DPRINT_FILESYSTEM, "ClusterHigh = 0x%x\n", DirEntry->ClusterHigh);
			DPRINTM(DPRINT_FILESYSTEM, "Time = %d\n", DirEntry->Time);
			DPRINTM(DPRINT_FILESYSTEM, "Date = %d\n", DirEntry->Date);
			DPRINTM(DPRINT_FILESYSTEM, "ClusterLow = 0x%x\n", DirEntry->ClusterLow);
			DPRINTM(DPRINT_FILESYSTEM, "Size = %d\n", DirEntry->Size);

			//
			// Get the cluster chain
			//
			StartCluster = ((ULONG)DirEntry->ClusterHigh << 16) + DirEntry->ClusterLow;
			DPRINTM(DPRINT_FILESYSTEM, "StartCluster = 0x%x\n", StartCluster);
			FatFileInfoPointer->FileFatChain = FatGetClusterChainArray(StartCluster);

			//
			// See if memory allocation failed
			//
			if (FatFileInfoPointer->FileFatChain == NULL)
			{
				return FALSE;
			}

			return TRUE;
		}

		//
		// Nope, no match - zero buffers and continue looking
		//
		memset(ShortNameBuffer, 0, 13 * sizeof(UCHAR));
		memset(LfnNameBuffer, 0, 261 * sizeof(UCHAR));
		continue;
	}

	return FALSE;
}

static BOOLEAN FatXSearchDirectoryBufferForFile(PVOID DirectoryBuffer, ULONG DirectorySize, PCHAR FileName, PFAT_FILE_INFO FatFileInfoPointer)
{
	ULONG		EntryCount;
	ULONG		CurrentEntry;
	ULONG		FileNameLen;
	FATX_DIRENTRY	OurDirEntry;
	PFATX_DIRENTRY	DirEntry = &OurDirEntry;

	EntryCount = DirectorySize / sizeof(FATX_DIRENTRY);

	DPRINTM(DPRINT_FILESYSTEM, "FatXSearchDirectoryBufferForFile() DirectoryBuffer = 0x%x EntryCount = %d FileName = %s\n", DirectoryBuffer, EntryCount, FileName);

	FileNameLen = strlen(FileName);

	for (CurrentEntry = 0; CurrentEntry < EntryCount; CurrentEntry++, DirectoryBuffer = ((PFATX_DIRENTRY)DirectoryBuffer)+1)
	{
		OurDirEntry = *(PFATX_DIRENTRY) DirectoryBuffer;
		FatSwapFatXDirEntry(&OurDirEntry);
		if (0xff == DirEntry->FileNameSize)
		{
			break;
		}
		if (0xe5 == DirEntry->FileNameSize)
		{
			continue;
		}
		if (FileNameLen == DirEntry->FileNameSize &&
		    0 == _strnicmp(FileName, DirEntry->FileName, FileNameLen))
		{
			/*
			 * We found the entry, now fill in the FAT_FILE_INFO struct
			 */
			FatFileInfoPointer->FileSize = DirEntry->Size;
			FatFileInfoPointer->FilePointer = 0;

			DPRINTM(DPRINT_FILESYSTEM, "FATX Directory Entry:\n");
			DPRINTM(DPRINT_FILESYSTEM, "FileNameSize = %d\n", DirEntry->FileNameSize);
			DPRINTM(DPRINT_FILESYSTEM, "Attr = 0x%x\n", DirEntry->Attr);
			DPRINTM(DPRINT_FILESYSTEM, "StartCluster = 0x%x\n", DirEntry->StartCluster);
			DPRINTM(DPRINT_FILESYSTEM, "Size = %d\n", DirEntry->Size);
			DPRINTM(DPRINT_FILESYSTEM, "Time = %d\n", DirEntry->Time);
			DPRINTM(DPRINT_FILESYSTEM, "Date = %d\n", DirEntry->Date);
			DPRINTM(DPRINT_FILESYSTEM, "CreateTime = %d\n", DirEntry->CreateTime);
			DPRINTM(DPRINT_FILESYSTEM, "CreateDate = %d\n", DirEntry->CreateDate);
			DPRINTM(DPRINT_FILESYSTEM, "LastAccessTime = %d\n", DirEntry->LastAccessTime);
			DPRINTM(DPRINT_FILESYSTEM, "LastAccessDate = %d\n", DirEntry->LastAccessDate);

			/*
			 * Get the cluster chain
			 */
			FatFileInfoPointer->FileFatChain = FatGetClusterChainArray(DirEntry->StartCluster);

			/*
			 * See if memory allocation failed
			 */
			if (NULL == FatFileInfoPointer->FileFatChain)
			{
				return FALSE;
			}

			return TRUE;
		}
	}

	return FALSE;
}

/*
 * FatLookupFile()
 * This function searches the file system for the
 * specified filename and fills in an FAT_FILE_INFO structure
 * with info describing the file, etc. returns ARC error code
 */
LONG FatLookupFile(PCSTR FileName, ULONG DeviceId, PFAT_FILE_INFO FatFileInfoPointer)
{
	UINT32		i;
	ULONG		NumberOfPathParts;
	CHAR		PathPart[261];
	PVOID		DirectoryBuffer;
	ULONG		DirectoryStartCluster = 0;
	ULONG		DirectorySize;
	FAT_FILE_INFO	FatFileInfo;

	DPRINTM(DPRINT_FILESYSTEM, "FatLookupFile() FileName = %s\n", FileName);

	memset(FatFileInfoPointer, 0, sizeof(FAT_FILE_INFO));

	//
	// Figure out how many sub-directories we are nested in
	//
	NumberOfPathParts = FsGetNumPathParts(FileName);

	//
	// Loop once for each part
	//
	for (i=0; i<NumberOfPathParts; i++)
	{
		//
		// Get first path part
		//
		FsGetFirstNameFromPath(PathPart, FileName);

		//
		// Advance to the next part of the path
		//
		for (; (*FileName != '\\') && (*FileName != '/') && (*FileName != '\0'); FileName++)
		{
		}
		FileName++;

		//
		// Buffer the directory contents
		//
		DirectoryBuffer = FatBufferDirectory(DirectoryStartCluster, &DirectorySize, (i == 0) );
		if (DirectoryBuffer == NULL)
		{
			return ENOMEM;
		}

		//
		// Search for file name in directory
		//
		if (ISFATX(FatType))
		{
			if (!FatXSearchDirectoryBufferForFile(DirectoryBuffer, DirectorySize, PathPart, &FatFileInfo))
			{
				MmHeapFree(DirectoryBuffer);
				return ENOENT;
			}
		}
		else
		{
			if (!FatSearchDirectoryBufferForFile(DirectoryBuffer, DirectorySize, PathPart, &FatFileInfo))
			{
				MmHeapFree(DirectoryBuffer);
				return ENOENT;
			}
		}

		MmHeapFree(DirectoryBuffer);

		//
		// If we have another sub-directory to go then
		// grab the start cluster and free the fat chain array
		//
		if ((i+1) < NumberOfPathParts)
		{
			DirectoryStartCluster = FatFileInfo.FileFatChain[0];
			MmHeapFree(FatFileInfo.FileFatChain);
		}
	}

	memcpy(FatFileInfoPointer, &FatFileInfo, sizeof(FAT_FILE_INFO));

	return ESUCCESS;
}

/*
 * FatParseFileName()
 * This function parses a directory entry name which
 * is in the form of "FILE   EXT" and puts it in Buffer
 * in the form of "file.ext"
 */
void FatParseShortFileName(PCHAR Buffer, PDIRENTRY DirEntry)
{
	ULONG		Idx;

	Idx = 0;
	RtlZeroMemory(Buffer, 13);

	//
	// Fixup first character
	//
	if (DirEntry->FileName[0] == 0x05)
	{
		DirEntry->FileName[0] = 0xE5;
	}

	//
	// Get the file name
	//
	while (Idx < 8)
	{
		if (DirEntry->FileName[Idx] == ' ')
		{
			break;
		}

		Buffer[Idx] = DirEntry->FileName[Idx];
		Idx++;
	}

	//
	// Get extension
	//
	if ((DirEntry->FileName[8] != ' '))
	{
		Buffer[Idx++] = '.';
		Buffer[Idx++] = (DirEntry->FileName[8] == ' ') ? '\0' : DirEntry->FileName[8];
		Buffer[Idx++] = (DirEntry->FileName[9] == ' ') ? '\0' : DirEntry->FileName[9];
		Buffer[Idx++] = (DirEntry->FileName[10] == ' ') ? '\0' : DirEntry->FileName[10];
	}

	DPRINTM(DPRINT_FILESYSTEM, "FatParseShortFileName() ShortName = %s\n", Buffer);
}

/*
 * FatGetFatEntry()
 * returns the Fat entry for a given cluster number
 */
BOOLEAN FatGetFatEntry(ULONG Cluster, ULONG* ClusterPointer)
{
	ULONG		fat = 0;
	UINT32		FatOffset;
	UINT32		ThisFatSecNum;
	UINT32		ThisFatEntOffset;

	DPRINTM(DPRINT_FILESYSTEM, "FatGetFatEntry() Retrieving FAT entry for cluster %d.\n", Cluster);

	switch(FatType)
	{
	case FAT12:

		FatOffset = Cluster + (Cluster / 2);
		ThisFatSecNum = ActiveFatSectorStart + (FatOffset / BytesPerSector);
		ThisFatEntOffset = (FatOffset % BytesPerSector);

		DPRINTM(DPRINT_FILESYSTEM, "FatOffset: %d\n", FatOffset);
		DPRINTM(DPRINT_FILESYSTEM, "ThisFatSecNum: %d\n", ThisFatSecNum);
		DPRINTM(DPRINT_FILESYSTEM, "ThisFatEntOffset: %d\n", ThisFatEntOffset);

		if (ThisFatEntOffset == (BytesPerSector - 1))
		{
			if (!FatReadVolumeSectors(FatDriveNumber, ThisFatSecNum, 2, (PVOID)FILESYSBUFFER))
			{
				return FALSE;
			}
		}
		else
		{
			if (!FatReadVolumeSectors(FatDriveNumber, ThisFatSecNum, 1, (PVOID)FILESYSBUFFER))
			{
				return FALSE;
			}
		}

		fat = *((USHORT *) ((ULONG_PTR)FILESYSBUFFER + ThisFatEntOffset));
		fat = SWAPW(fat);
		if (Cluster & 0x0001)
			fat = fat >> 4;	/* Cluster number is ODD */
		else
			fat = fat & 0x0FFF;	/* Cluster number is EVEN */

		break;

	case FAT16:
	case FATX16:

		FatOffset = (Cluster * 2);
		ThisFatSecNum = ActiveFatSectorStart + (FatOffset / BytesPerSector);
		ThisFatEntOffset = (FatOffset % BytesPerSector);

		if (!FatReadVolumeSectors(FatDriveNumber, ThisFatSecNum, 1, (PVOID)FILESYSBUFFER))
		{
			return FALSE;
		}

		fat = *((USHORT *) ((ULONG_PTR)FILESYSBUFFER + ThisFatEntOffset));
		fat = SWAPW(fat);

		break;

	case FAT32:
	case FATX32:

		FatOffset = (Cluster * 4);
		ThisFatSecNum = ActiveFatSectorStart + (FatOffset / BytesPerSector);
		ThisFatEntOffset = (FatOffset % BytesPerSector);

		if (!FatReadVolumeSectors(FatDriveNumber, ThisFatSecNum, 1, (PVOID)FILESYSBUFFER))
		{
			return FALSE;
		}

		// Get the fat entry
		fat = (*((ULONG *) ((ULONG_PTR)FILESYSBUFFER + ThisFatEntOffset))) & 0x0FFFFFFF;
		fat = SWAPD(fat);

		break;

	}

	DPRINTM(DPRINT_FILESYSTEM, "FAT entry is 0x%x.\n", fat);

	*ClusterPointer = fat;

	return TRUE;
}

ULONG FatCountClustersInChain(ULONG StartCluster)
{
	ULONG	ClusterCount = 0;

	DPRINTM(DPRINT_FILESYSTEM, "FatCountClustersInChain() StartCluster = %d\n", StartCluster);

	while (1)
	{
		//
		// If end of chain then break out of our cluster counting loop
		//
		if (((FatType == FAT12) && (StartCluster >= 0xff8)) ||
			((FatType == FAT16 || FatType == FATX16) && (StartCluster >= 0xfff8)) ||
			((FatType == FAT32 || FatType == FATX32) && (StartCluster >= 0x0ffffff8)))
		{
			break;
		}

		//
		// Increment count
		//
		ClusterCount++;

		//
		// Get next cluster
		//
		if (!FatGetFatEntry(StartCluster, &StartCluster))
		{
			return 0;
		}
	}

	DPRINTM(DPRINT_FILESYSTEM, "FatCountClustersInChain() ClusterCount = %d\n", ClusterCount);

	return ClusterCount;
}

ULONG* FatGetClusterChainArray(ULONG StartCluster)
{
	ULONG	ClusterCount;
	ULONG		ArraySize;
	ULONG*	ArrayPointer;
	ULONG		Idx;

	DPRINTM(DPRINT_FILESYSTEM, "FatGetClusterChainArray() StartCluster = %d\n", StartCluster);

	ClusterCount = FatCountClustersInChain(StartCluster) + 1; // Lets get the 0x0ffffff8 on the end of the array
	ArraySize = ClusterCount * sizeof(ULONG);

	//
	// Allocate array memory
	//
	ArrayPointer = MmHeapAlloc(ArraySize);

	if (ArrayPointer == NULL)
	{
		return NULL;
	}

	//
	// Loop through and set array values
	//
	for (Idx=0; Idx<ClusterCount; Idx++)
	{
		//
		// Set current cluster
		//
		ArrayPointer[Idx] = StartCluster;

		//
		// Don't try to get next cluster for last cluster
		//
		if (((FatType == FAT12) && (StartCluster >= 0xff8)) ||
			((FatType == FAT16 || FatType == FATX16) && (StartCluster >= 0xfff8)) ||
			((FatType == FAT32 || FatType == FATX32) && (StartCluster >= 0x0ffffff8)))
		{
			Idx++;
			break;
		}

		//
		// Get next cluster
		//
		if (!FatGetFatEntry(StartCluster, &StartCluster))
		{
			MmHeapFree(ArrayPointer);
			return NULL;
		}
	}

	return ArrayPointer;
}

/*
 * FatReadCluster()
 * Reads the specified cluster into memory
 */
BOOLEAN FatReadCluster(ULONG ClusterNumber, PVOID Buffer)
{
	ULONG		ClusterStartSector;

	ClusterStartSector = ((ClusterNumber - 2) * SectorsPerCluster) + DataSectorStart;

	DPRINTM(DPRINT_FILESYSTEM, "FatReadCluster() ClusterNumber = %d Buffer = 0x%x ClusterStartSector = %d\n", ClusterNumber, Buffer, ClusterStartSector);

	if (!FatReadVolumeSectors(FatDriveNumber, ClusterStartSector, SectorsPerCluster, (PVOID)FILESYSBUFFER))
	{
		return FALSE;
	}

	memcpy(Buffer, (PVOID)FILESYSBUFFER, SectorsPerCluster * BytesPerSector);

	return TRUE;
}

/*
 * FatReadClusterChain()
 * Reads the specified clusters into memory
 */
BOOLEAN FatReadClusterChain(ULONG StartClusterNumber, ULONG NumberOfClusters, PVOID Buffer)
{
	ULONG		ClusterStartSector;

	DPRINTM(DPRINT_FILESYSTEM, "FatReadClusterChain() StartClusterNumber = %d NumberOfClusters = %d Buffer = 0x%x\n", StartClusterNumber, NumberOfClusters, Buffer);

	while (NumberOfClusters > 0)
	{

		DPRINTM(DPRINT_FILESYSTEM, "FatReadClusterChain() StartClusterNumber = %d NumberOfClusters = %d Buffer = 0x%x\n", StartClusterNumber, NumberOfClusters, Buffer);
		//
		// Calculate starting sector for cluster
		//
		ClusterStartSector = ((StartClusterNumber - 2) * SectorsPerCluster) + DataSectorStart;

		//
		// Read cluster into memory
		//
		if (!FatReadVolumeSectors(FatDriveNumber, ClusterStartSector, SectorsPerCluster, (PVOID)FILESYSBUFFER))
		{
			return FALSE;
		}

		memcpy(Buffer, (PVOID)FILESYSBUFFER, SectorsPerCluster * BytesPerSector);

		//
		// Decrement count of clusters left to read
		//
		NumberOfClusters--;

		//
		// Increment buffer address by cluster size
		//
		Buffer = (PVOID)((ULONG_PTR)Buffer + (SectorsPerCluster * BytesPerSector));

		//
		// Get next cluster
		//
		if (!FatGetFatEntry(StartClusterNumber, &StartClusterNumber))
		{
			return FALSE;
		}

		//
		// If end of chain then break out of our cluster reading loop
		//
		if (((FatType == FAT12) && (StartClusterNumber >= 0xff8)) ||
			((FatType == FAT16 || FatType == FATX16) && (StartClusterNumber >= 0xfff8)) ||
			((FatType == FAT32 || FatType == FATX32) && (StartClusterNumber >= 0x0ffffff8)))
		{
			break;
		}
	}

	return TRUE;
}

/*
 * FatReadPartialCluster()
 * Reads part of a cluster into memory
 */
BOOLEAN FatReadPartialCluster(ULONG ClusterNumber, ULONG StartingOffset, ULONG Length, PVOID Buffer)
{
	ULONG		ClusterStartSector;

	DPRINTM(DPRINT_FILESYSTEM, "FatReadPartialCluster() ClusterNumber = %d StartingOffset = %d Length = %d Buffer = 0x%x\n", ClusterNumber, StartingOffset, Length, Buffer);

	ClusterStartSector = ((ClusterNumber - 2) * SectorsPerCluster) + DataSectorStart;

	if (!FatReadVolumeSectors(FatDriveNumber, ClusterStartSector, SectorsPerCluster, (PVOID)FILESYSBUFFER))
	{
		return FALSE;
	}

	memcpy(Buffer, (PVOID)((ULONG_PTR)FILESYSBUFFER + StartingOffset), Length);

	return TRUE;
}

/*
 * FatReadFile()
 * Reads BytesToRead from open file and
 * returns the number of bytes read in BytesRead
 */
BOOLEAN FatReadFile(PFAT_FILE_INFO FatFileInfo, ULONG BytesToRead, ULONG* BytesRead, PVOID Buffer)
{
	ULONG			ClusterNumber;
	ULONG			OffsetInCluster;
	ULONG			LengthInCluster;
	ULONG			NumberOfClusters;
	ULONG			BytesPerCluster;

	DPRINTM(DPRINT_FILESYSTEM, "FatReadFile() BytesToRead = %d Buffer = 0x%x\n", BytesToRead, Buffer);

	if (BytesRead != NULL)
	{
		*BytesRead = 0;
	}

	//
	// If they are trying to read past the
	// end of the file then return success
	// with BytesRead == 0
	//
	if (FatFileInfo->FilePointer >= FatFileInfo->FileSize)
	{
		return TRUE;
	}

	//
	// If they are trying to read more than there is to read
	// then adjust the amount to read
	//
	if ((FatFileInfo->FilePointer + BytesToRead) > FatFileInfo->FileSize)
	{
		BytesToRead = (FatFileInfo->FileSize - FatFileInfo->FilePointer);
	}

	//
	// Ok, now we have to perform at most 3 calculations
	// I'll draw you a picture (using nifty ASCII art):
	//
	// CurrentFilePointer -+
	//                     |
	//    +----------------+
	//    |
	// +-----------+-----------+-----------+-----------+
	// | Cluster 1 | Cluster 2 | Cluster 3 | Cluster 4 |
	// +-----------+-----------+-----------+-----------+
	//    |                                    |
	//    +---------------+--------------------+
	//                    |
	// BytesToRead -------+
	//
	// 1 - The first calculation (and read) will align
	//     the file pointer with the next cluster.
	//     boundary (if we are supposed to read that much)
	// 2 - The next calculation (and read) will read
	//     in all the full clusters that the requested
	//     amount of data would cover (in this case
	//     clusters 2 & 3).
	// 3 - The last calculation (and read) would read
	//     in the remainder of the data requested out of
	//     the last cluster.
	//

	BytesPerCluster = SectorsPerCluster * BytesPerSector;

	//
	// Only do the first read if we
	// aren't aligned on a cluster boundary
	//
	if (FatFileInfo->FilePointer % BytesPerCluster)
	{
		//
		// Do the math for our first read
		//
		ClusterNumber = (FatFileInfo->FilePointer / BytesPerCluster);
		ClusterNumber = FatFileInfo->FileFatChain[ClusterNumber];
		OffsetInCluster = (FatFileInfo->FilePointer % BytesPerCluster);
		LengthInCluster = (BytesToRead > (BytesPerCluster - OffsetInCluster)) ? (BytesPerCluster - OffsetInCluster) : BytesToRead;

		//
		// Now do the read and update BytesRead, BytesToRead, FilePointer, & Buffer
		//
		if (!FatReadPartialCluster(ClusterNumber, OffsetInCluster, LengthInCluster, Buffer))
		{
			return FALSE;
		}
		if (BytesRead != NULL)
		{
			*BytesRead += LengthInCluster;
		}
		BytesToRead -= LengthInCluster;
		FatFileInfo->FilePointer += LengthInCluster;
		Buffer = (PVOID)((ULONG_PTR)Buffer + LengthInCluster);
	}

	//
	// Do the math for our second read (if any data left)
	//
	if (BytesToRead > 0)
	{
		//
		// Determine how many full clusters we need to read
		//
		NumberOfClusters = (BytesToRead / BytesPerCluster);

		if (NumberOfClusters > 0)
		{
			ClusterNumber = (FatFileInfo->FilePointer / BytesPerCluster);
			ClusterNumber = FatFileInfo->FileFatChain[ClusterNumber];

			//
			// Now do the read and update BytesRead, BytesToRead, FilePointer, & Buffer
			//
			if (!FatReadClusterChain(ClusterNumber, NumberOfClusters, Buffer))
			{
				return FALSE;
			}
			if (BytesRead != NULL)
			{
				*BytesRead += (NumberOfClusters * BytesPerCluster);
			}
			BytesToRead -= (NumberOfClusters * BytesPerCluster);
			FatFileInfo->FilePointer += (NumberOfClusters * BytesPerCluster);
			Buffer = (PVOID)((ULONG_PTR)Buffer + (NumberOfClusters * BytesPerCluster));
		}
	}

	//
	// Do the math for our third read (if any data left)
	//
	if (BytesToRead > 0)
	{
		ClusterNumber = (FatFileInfo->FilePointer / BytesPerCluster);
		ClusterNumber = FatFileInfo->FileFatChain[ClusterNumber];

		//
		// Now do the read and update BytesRead, BytesToRead, FilePointer, & Buffer
		//
		if (!FatReadPartialCluster(ClusterNumber, 0, BytesToRead, Buffer))
		{
			return FALSE;
		}
		if (BytesRead != NULL)
		{
			*BytesRead += BytesToRead;
		}
		FatFileInfo->FilePointer += BytesToRead;
		BytesToRead -= BytesToRead;
		Buffer = (PVOID)((ULONG_PTR)Buffer + BytesToRead);
	}

	return TRUE;
}

BOOLEAN FatReadVolumeSectors(ULONG DriveNumber, ULONG SectorNumber, ULONG SectorCount, PVOID Buffer)
{
	// Now try to read in the block
	if (!MachDiskReadLogicalSectors(DriveNumber, SectorNumber + FatVolumeStartSector, SectorCount, (PVOID)DISKREADBUFFER))
	{
		return FALSE;
	}

	// Copy data to the caller
	RtlCopyMemory(Buffer, (PVOID)DISKREADBUFFER, SectorCount * BytesPerSector);

	// Return success
	return TRUE;
}

LONG FatClose(ULONG FileId)
{
	PFAT_FILE_INFO FileHandle = FsGetDeviceSpecific(FileId);

	MmHeapFree(FileHandle);

	return ESUCCESS;
}

LONG FatGetFileInformation(ULONG FileId, FILEINFORMATION* Information)
{
	PFAT_FILE_INFO FileHandle = FsGetDeviceSpecific(FileId);

	RtlZeroMemory(Information, sizeof(FILEINFORMATION));
	Information->EndingAddress.LowPart = FileHandle->FileSize;
	Information->CurrentAddress.LowPart = FileHandle->FilePointer;

	DPRINTM(DPRINT_FILESYSTEM, "FatGetFileInformation() FileSize = %d\n",
	    Information->EndingAddress.LowPart);
	DPRINTM(DPRINT_FILESYSTEM, "FatGetFileInformation() FilePointer = %d\n",
	    Information->CurrentAddress.LowPart);

	return ESUCCESS;
}

LONG FatOpen(CHAR* Path, OPENMODE OpenMode, ULONG* FileId)
{
	FAT_FILE_INFO TempFileInfo;
	PFAT_FILE_INFO FileHandle;
	ULONG DeviceId;
	LONG ret;

	if (OpenMode != OpenReadOnly)
		return EACCES;

	DeviceId = FsGetDeviceId(*FileId);

	DPRINTM(DPRINT_FILESYSTEM, "FatOpen() FileName = %s\n", Path);

	RtlZeroMemory(&TempFileInfo, sizeof(TempFileInfo));
	ret = FatLookupFile(Path, DeviceId, &TempFileInfo);
	if (ret != ESUCCESS)
		return ENOENT;

	FileHandle = MmHeapAlloc(sizeof(FAT_FILE_INFO));
	if (!FileHandle)
		return ENOMEM;

	RtlCopyMemory(FileHandle, &TempFileInfo, sizeof(FAT_FILE_INFO));

	FsSetDeviceSpecific(*FileId, FileHandle);
	return ESUCCESS;
}

LONG FatRead(ULONG FileId, VOID* Buffer, ULONG N, ULONG* Count)
{
	PFAT_FILE_INFO FileHandle = FsGetDeviceSpecific(FileId);
	BOOLEAN ret;

	//
	// Call old read method
	//
	ret = FatReadFile(FileHandle, N, Count, Buffer);

	//
	// Check for success
	//
	if (ret)
		return ESUCCESS;
	else
		return EIO;
}

LONG FatSeek(ULONG FileId, LARGE_INTEGER* Position, SEEKMODE SeekMode)
{
	PFAT_FILE_INFO FileHandle = FsGetDeviceSpecific(FileId);

	DPRINTM(DPRINT_FILESYSTEM, "FatSeek() NewFilePointer = %lu\n", Position->LowPart);

	if (SeekMode != SeekAbsolute)
		return EINVAL;
	if (Position->HighPart != 0)
		return EINVAL;
	if (Position->LowPart >= FileHandle->FileSize)
		return EINVAL;

	FileHandle->FilePointer = Position->LowPart;
	return ESUCCESS;
}

const DEVVTBL FatFuncTable =
{
	FatClose,
	FatGetFileInformation,
	FatOpen,
	FatRead,
	FatSeek,
	L"fastfat",
};

const DEVVTBL* FatMount(ULONG DeviceId)
{
	UCHAR Buffer[512];
	PFAT_BOOTSECTOR BootSector = (PFAT_BOOTSECTOR)Buffer;
	PFAT32_BOOTSECTOR BootSector32 = (PFAT32_BOOTSECTOR)Buffer;
	PFATX_BOOTSECTOR BootSectorX = (PFATX_BOOTSECTOR)Buffer;
	LARGE_INTEGER Position;
	ULONG Count;
	LONG ret;

	//
	// Read the BootSector
	//
	Position.HighPart = 0;
	Position.LowPart = 0;
	ret = ArcSeek(DeviceId, &Position, SeekAbsolute);
	if (ret != ESUCCESS)
		return NULL;
	ret = ArcRead(DeviceId, Buffer, sizeof(Buffer), &Count);
	if (ret != ESUCCESS || Count != sizeof(Buffer))
		return NULL;

	//
	// Check if BootSector is valid. If yes, return FAT function table
	//
	if (RtlEqualMemory(BootSector->FileSystemType, "FAT12   ", 8) ||
	    RtlEqualMemory(BootSector->FileSystemType, "FAT16   ", 8) ||
	    RtlEqualMemory(BootSector32->FileSystemType, "FAT32   ", 8) ||
	    RtlEqualMemory(BootSectorX->FileSystemType, "FATX", 4))
	{
		//
		// Compatibility hack as long as FS is not using underlying device DeviceId
		//
		ULONG DriveNumber;
		ULONGLONG StartSector;
		ULONGLONG SectorCount;
		int Type;
		if (!DiskGetBootVolume(&DriveNumber, &StartSector, &SectorCount, &Type))
			return NULL;
		FatOpenVolume(DriveNumber, StartSector, SectorCount);
		return &FatFuncTable;
	}
	else
		return NULL;
}
