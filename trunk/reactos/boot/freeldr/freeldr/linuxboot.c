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
#include <arch.h>
#include <disk.h>
#include <miscboot.h>
#include <rtl.h>
#include <fs.h>
#include <ui.h>
#include <linux.h>
#include <debug.h>
#include <mm.h>
#include <inifile.h>
#include <oslist.h> // For RemoveQuotes()
#include <video.h>
#include <drivemap.h>



#define	LINUX_READ_CHUNK_SIZE	0x20000			// Read 128k at a time


PLINUX_BOOTSECTOR	LinuxBootSector = NULL;
PLINUX_SETUPSECTOR	LinuxSetupSector = NULL;
U32					SetupSectorSize = 0;
BOOL				NewStyleLinuxKernel = FALSE;
U32					LinuxKernelSize = 0;
U32					LinuxInitrdSize = 0;
UCHAR				LinuxKernelName[260];
UCHAR				LinuxInitrdName[260];
BOOL				LinuxHasInitrd = FALSE;
UCHAR				LinuxCommandLine[260] = "";
U32					LinuxCommandLineSize = 0;
PVOID				LinuxKernelLoadAddress = NULL;
PVOID				LinuxInitrdLoadAddress = NULL;
UCHAR				LinuxBootDescription[80];

VOID LoadAndBootLinux(PUCHAR OperatingSystemName, PUCHAR Description)
{
	PFILE	LinuxKernel = NULL;
	PFILE	LinuxInitrdFile = NULL;
	UCHAR	TempString[260];

	UiDrawBackdrop();

	if (Description)
	{
		sprintf(LinuxBootDescription, "Loading %s...", Description);
	}
	else
	{
		strcpy(LinuxBootDescription, "Loading Linux...");
	}

	UiDrawStatusText(LinuxBootDescription);
	UiDrawProgressBarCenter(0, 100, LinuxBootDescription);

	// Parse the .ini file section
	if (!LinuxParseIniSection(OperatingSystemName))
	{
		goto LinuxBootFailed;
	}

	// Open the boot volume
	if (!FsOpenVolume(BootDrive, BootPartition))
	{
		UiMessageBox("Failed to open boot drive.");
		goto LinuxBootFailed;
	}

	// Open the kernel
	LinuxKernel = FsOpenFile(LinuxKernelName);
	if (LinuxKernel == NULL)
	{
		sprintf(TempString, "Linux kernel \'%s\' not found.", LinuxKernelName);
		UiMessageBox(TempString);
		goto LinuxBootFailed;
	}

	// Open the initrd file image (if necessary)
	if (LinuxHasInitrd)
	{
		LinuxInitrdFile = FsOpenFile(LinuxInitrdName);
		if (LinuxInitrdFile == NULL)
		{
			sprintf(TempString, "Linux initrd image \'%s\' not found.", LinuxInitrdName);
			UiMessageBox(TempString);
			goto LinuxBootFailed;
		}
	}

	// Read the boot sector
	if (!LinuxReadBootSector(LinuxKernel))
	{
		goto LinuxBootFailed;
	}

	// Read the setup sector
	if (!LinuxReadSetupSector(LinuxKernel))
	{
		goto LinuxBootFailed;
	}

	// Calc kernel size
	LinuxKernelSize = FsGetFileSize(LinuxKernel) - (512 + SetupSectorSize);

	// Get the file size
	LinuxInitrdSize = FsGetFileSize(LinuxInitrdFile);

	// Read the kernel
	if (!LinuxReadKernel(LinuxKernel))
	{
		goto LinuxBootFailed;
	}

	// Read the initrd (if necessary)
	if (LinuxHasInitrd)
	{
		if (!LinuxReadInitrd(LinuxInitrdFile))
		{
			goto LinuxBootFailed;
		}
	}

	// If the default root device is set to FLOPPY (0000h), change to /dev/fd0 (0200h)
	if (LinuxBootSector->RootDevice == 0x0000)
	{
		LinuxBootSector->RootDevice = 0x0200;
	}

	if (LinuxSetupSector->Version >= 0x0202)
	{
		LinuxSetupSector->CommandLinePointer = 0x99000;
	}
	else
	{
		LinuxBootSector->CommandLineMagic = LINUX_COMMAND_LINE_MAGIC;
		LinuxBootSector->CommandLineOffset = 0x9000;
	}

	if (NewStyleLinuxKernel)
	{
		LinuxSetupSector->TypeOfLoader = LINUX_LOADER_TYPE_FREELOADER;
	}
	else
	{
		LinuxSetupSector->LoadFlags = 0;
	}

	RtlCopyMemory((PVOID)0x90000, LinuxBootSector, 512);
	RtlCopyMemory((PVOID)0x90200, LinuxSetupSector, SetupSectorSize);
	RtlCopyMemory((PVOID)0x99000, LinuxCommandLine, LinuxCommandLineSize);

	UiUnInitialize("Booting Linux...");

	DiskStopFloppyMotor();

	if (LinuxSetupSector->LoadFlags & LINUX_FLAG_LOAD_HIGH)
	{
		BootNewLinuxKernel();
	}
	else
	{
		BootOldLinuxKernel(LinuxKernelSize);
	}


LinuxBootFailed:

	if (LinuxKernel != NULL)
	{
		FsCloseFile(LinuxKernel);
	}
	if (LinuxInitrdFile != NULL)
	{
		FsCloseFile(LinuxInitrdFile);
	}

	if (LinuxBootSector != NULL)
	{
		MmFreeMemory(LinuxBootSector);
	}
	if (LinuxSetupSector != NULL)
	{
		MmFreeMemory(LinuxSetupSector);
	}
	if (LinuxKernelLoadAddress != NULL)
	{
		MmFreeMemory(LinuxKernelLoadAddress);
	}
	if (LinuxInitrdLoadAddress != NULL)
	{
		MmFreeMemory(LinuxInitrdLoadAddress);
	}

	LinuxBootSector = NULL;
	LinuxSetupSector = NULL;
	LinuxKernelLoadAddress = NULL;
	LinuxInitrdLoadAddress = NULL;
	SetupSectorSize = 0;
	NewStyleLinuxKernel = FALSE;
	LinuxKernelSize = 0;
	LinuxHasInitrd = FALSE;
	strcpy(LinuxCommandLine, "");
	LinuxCommandLineSize = 0;
}

BOOL LinuxParseIniSection(PUCHAR OperatingSystemName)
{
	UCHAR	SettingName[260];
	UCHAR	SettingValue[260];
	U32		SectionId;

	// Find all the message box settings and run them
	UiShowMessageBoxesInSection(OperatingSystemName);

	// Try to open the operating system section in the .ini file
	if (!IniOpenSection(OperatingSystemName, &SectionId))
	{
		sprintf(SettingName, "Section [%s] not found in freeldr.ini.\n", OperatingSystemName);
		UiMessageBox(SettingName);
		return FALSE;
	}

	if (!IniReadSettingByName(SectionId, "BootDrive", SettingValue, 260))
	{
		UiMessageBox("Boot drive not specified for selected OS!");
		return FALSE;
	}

	BootDrive = DriveMapGetBiosDriveNumber(SettingValue);

	BootPartition = 0;
	if (IniReadSettingByName(SectionId, "BootPartition", SettingValue, 260))
	{
		BootPartition = atoi(SettingValue);
	}

	// Get the kernel name
	if (!IniReadSettingByName(SectionId, "Kernel", LinuxKernelName, 260))
	{
		UiMessageBox("Linux kernel filename not specified for selected OS!");
		return FALSE;
	}

	// Get the initrd name
	if (IniReadSettingByName(SectionId, "Initrd", LinuxInitrdName, 260))
	{
		LinuxHasInitrd = TRUE;
	}

	// Get the command line
	if (IniReadSettingByName(SectionId, "CommandLine", LinuxCommandLine, 260))
	{
		RemoveQuotes(LinuxCommandLine);
		LinuxCommandLineSize = strlen(LinuxCommandLine) + 1;
	}

	return TRUE;
}

BOOL LinuxReadBootSector(PFILE LinuxKernelFile)
{
	// Allocate memory for boot sector
	LinuxBootSector = (PLINUX_BOOTSECTOR)MmAllocateMemory(512);
	if (LinuxBootSector == NULL)
	{
		return FALSE;
	}

	// Read linux boot sector
	FsSetFilePointer(LinuxKernelFile, 0);
	if (!FsReadFile(LinuxKernelFile, 512, NULL, LinuxBootSector))
	{
		return FALSE;
	}

	// Check for validity
	if (LinuxBootSector->BootFlag != LINUX_BOOT_SECTOR_MAGIC)
	{
		UiMessageBox("Invalid boot sector magic (0xaa55)");
		return FALSE;
	}

	DbgDumpBuffer(DPRINT_LINUX, LinuxBootSector, 512);

	DbgPrint((DPRINT_LINUX, "SetupSectors: %d\n", LinuxBootSector->SetupSectors));
	DbgPrint((DPRINT_LINUX, "RootFlags: 0x%x\n", LinuxBootSector->RootFlags));
	DbgPrint((DPRINT_LINUX, "SystemSize: 0x%x\n", LinuxBootSector->SystemSize));
	DbgPrint((DPRINT_LINUX, "SwapDevice: 0x%x\n", LinuxBootSector->SwapDevice));
	DbgPrint((DPRINT_LINUX, "RamSize: 0x%x\n", LinuxBootSector->RamSize));
	DbgPrint((DPRINT_LINUX, "VideoMode: 0x%x\n", LinuxBootSector->VideoMode));
	DbgPrint((DPRINT_LINUX, "RootDevice: 0x%x\n", LinuxBootSector->RootDevice));
	DbgPrint((DPRINT_LINUX, "BootFlag: 0x%x\n", LinuxBootSector->BootFlag));

	return TRUE;
}

BOOL LinuxReadSetupSector(PFILE LinuxKernelFile)
{
	U8	TempLinuxSetupSector[512];

	LinuxSetupSector = (PLINUX_SETUPSECTOR)TempLinuxSetupSector;

	// Read first linux setup sector
	FsSetFilePointer(LinuxKernelFile, 512);
	if (!FsReadFile(LinuxKernelFile, 512, NULL, TempLinuxSetupSector))
	{
		return FALSE;
	}

	// Check the kernel version
	if (!LinuxCheckKernelVersion())
	{
		return FALSE;
	}

	if (NewStyleLinuxKernel)
	{
		SetupSectorSize = 512 * LinuxBootSector->SetupSectors;
	}
	else
	{
		SetupSectorSize = 4 * 512; // Always 4 setup sectors
	}

	// Allocate memory for setup sectors
	LinuxSetupSector = (PLINUX_SETUPSECTOR)MmAllocateMemory(SetupSectorSize);
	if (LinuxSetupSector == NULL)
	{
		return FALSE;
	}

	// Copy over first setup sector
	RtlCopyMemory(LinuxSetupSector, TempLinuxSetupSector, 512);

	// Read in the rest of the linux setup sectors
	FsSetFilePointer(LinuxKernelFile, 1024);
	if (!FsReadFile(LinuxKernelFile, SetupSectorSize - 512, NULL, ((PVOID)LinuxSetupSector) + 512))
	{
		return FALSE;
	}

	DbgDumpBuffer(DPRINT_LINUX, LinuxSetupSector, SetupSectorSize);

	DbgPrint((DPRINT_LINUX, "SetupHeaderSignature: 0x%x (HdrS)\n", LinuxSetupSector->SetupHeaderSignature));
	DbgPrint((DPRINT_LINUX, "Version: 0x%x\n", LinuxSetupSector->Version));
	DbgPrint((DPRINT_LINUX, "RealModeSwitch: 0x%x\n", LinuxSetupSector->RealModeSwitch));
	DbgPrint((DPRINT_LINUX, "SetupSeg: 0x%x\n", LinuxSetupSector->SetupSeg));
	DbgPrint((DPRINT_LINUX, "StartSystemSeg: 0x%x\n", LinuxSetupSector->StartSystemSeg));
	DbgPrint((DPRINT_LINUX, "KernelVersion: 0x%x\n", LinuxSetupSector->KernelVersion));
	DbgPrint((DPRINT_LINUX, "TypeOfLoader: 0x%x\n", LinuxSetupSector->TypeOfLoader));
	DbgPrint((DPRINT_LINUX, "LoadFlags: 0x%x\n", LinuxSetupSector->LoadFlags));
	DbgPrint((DPRINT_LINUX, "SetupMoveSize: 0x%x\n", LinuxSetupSector->SetupMoveSize));
	DbgPrint((DPRINT_LINUX, "Code32Start: 0x%x\n", LinuxSetupSector->Code32Start));
	DbgPrint((DPRINT_LINUX, "RamdiskAddress: 0x%x\n", LinuxSetupSector->RamdiskAddress));
	DbgPrint((DPRINT_LINUX, "RamdiskSize: 0x%x\n", LinuxSetupSector->RamdiskSize));
	DbgPrint((DPRINT_LINUX, "BootSectKludgeOffset: 0x%x\n", LinuxSetupSector->BootSectKludgeOffset));
	DbgPrint((DPRINT_LINUX, "BootSectKludgeSegment: 0x%x\n", LinuxSetupSector->BootSectKludgeSegment));
	DbgPrint((DPRINT_LINUX, "HeapEnd: 0x%x\n", LinuxSetupSector->HeapEnd));

	return TRUE;
}

BOOL LinuxReadKernel(PFILE LinuxKernelFile)
{
	U32		BytesLoaded;
	UCHAR	StatusText[260];
	PVOID	LoadAddress;

	sprintf(StatusText, "Loading %s", LinuxKernelName);
	UiDrawStatusText(StatusText);

	// Allocate memory for Linux kernel
	LinuxKernelLoadAddress = MmAllocateMemoryAtAddress(LinuxKernelSize, (PVOID)LINUX_KERNEL_LOAD_ADDRESS);
	if (LinuxKernelLoadAddress != (PVOID)LINUX_KERNEL_LOAD_ADDRESS)
	{
		return FALSE;
	}

	LoadAddress = LinuxKernelLoadAddress;

	// Read linux kernel to 0x100000 (1mb)
	FsSetFilePointer(LinuxKernelFile, 512 + SetupSectorSize);
	for (BytesLoaded=0; BytesLoaded<LinuxKernelSize; )
	{
		if (!FsReadFile(LinuxKernelFile, LINUX_READ_CHUNK_SIZE, NULL, LoadAddress))
		{
			return FALSE;
		}

		BytesLoaded += LINUX_READ_CHUNK_SIZE;
		LoadAddress += LINUX_READ_CHUNK_SIZE;

		UiDrawProgressBarCenter(BytesLoaded, LinuxKernelSize + LinuxInitrdSize, LinuxBootDescription);
	}

	return TRUE;
}

BOOL LinuxCheckKernelVersion(VOID)
{
	// Just assume old kernel until we find otherwise
	NewStyleLinuxKernel = FALSE;

	// Check for new style setup header
	if (LinuxSetupSector->SetupHeaderSignature != LINUX_SETUP_HEADER_ID)
	{
		NewStyleLinuxKernel = FALSE;
	}
	// Check for version below 2.0
	else if (LinuxSetupSector->Version < 0x0200)
	{
		NewStyleLinuxKernel = FALSE;
	}
	// Check for version 2.0
	else if (LinuxSetupSector->Version == 0x0200)
	{
		NewStyleLinuxKernel = TRUE;
	}
	// Check for version 2.01+
	else if (LinuxSetupSector->Version >= 0x0201)
	{
		NewStyleLinuxKernel = TRUE;
		LinuxSetupSector->HeapEnd = 0x9000;
		LinuxSetupSector->LoadFlags |= LINUX_FLAG_CAN_USE_HEAP;
	}

	if ((NewStyleLinuxKernel == FALSE) && (LinuxHasInitrd == TRUE))
	{
		UiMessageBox("Error: Cannot load a ramdisk (initrd) with an old kernel image.");
		return FALSE;
	}

	return TRUE;
}

BOOL LinuxReadInitrd(PFILE LinuxInitrdFile)
{
	U32		BytesLoaded;
	UCHAR	StatusText[260];

	sprintf(StatusText, "Loading %s", LinuxInitrdName);
	UiDrawStatusText(StatusText);

	// Allocate memory for the ramdisk
	//LinuxInitrdLoadAddress = MmAllocateMemory(LinuxInitrdSize);
	// Try to align it at the next MB boundary after the kernel
	//LinuxInitrdLoadAddress = MmAllocateMemoryAtAddress(LinuxInitrdSize, (PVOID)ROUND_UP((LINUX_KERNEL_LOAD_ADDRESS + LinuxKernelSize), 0x100000));
	if (LinuxSetupSector->Version <= 0x0202)
	{
		LinuxInitrdLoadAddress = MmAllocateHighestMemoryBelowAddress(LinuxInitrdSize, (PVOID)LINUX_MAX_INITRD_ADDRESS);
	}
	else
	{
		LinuxInitrdLoadAddress = MmAllocateHighestMemoryBelowAddress(LinuxInitrdSize, (PVOID)LinuxSetupSector->InitrdAddressMax);
	}
	if (LinuxInitrdLoadAddress == NULL)
	{
		return FALSE;
	}

	// Set the information in the setup struct
	LinuxSetupSector->RamdiskAddress = (U32)LinuxInitrdLoadAddress;
	LinuxSetupSector->RamdiskSize = LinuxInitrdSize;

	DbgPrint((DPRINT_LINUX, "RamdiskAddress: 0x%x\n", LinuxSetupSector->RamdiskAddress));
	DbgPrint((DPRINT_LINUX, "RamdiskSize: 0x%x\n", LinuxSetupSector->RamdiskSize));

	if (LinuxSetupSector->Version >= 0x0203)
	{
		DbgPrint((DPRINT_LINUX, "InitrdAddressMax: 0x%x\n", LinuxSetupSector->InitrdAddressMax));
	}

	// Read in the ramdisk
	for (BytesLoaded=0; BytesLoaded<LinuxInitrdSize; )
	{
		if (!FsReadFile(LinuxInitrdFile, LINUX_READ_CHUNK_SIZE, NULL, (PVOID)LinuxInitrdLoadAddress))
		{
			return FALSE;
		}

		BytesLoaded += LINUX_READ_CHUNK_SIZE;
		LinuxInitrdLoadAddress += LINUX_READ_CHUNK_SIZE;

		UiDrawProgressBarCenter(BytesLoaded + LinuxKernelSize, LinuxInitrdSize + LinuxKernelSize, LinuxBootDescription);
	}

	return TRUE;
}
