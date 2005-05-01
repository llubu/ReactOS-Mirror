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
#include <disk.h>
#include <arch.h>
#include <rtl.h>
#include <ui.h>
#include <debug.h>


#undef  UNIMPLEMENTED
#define UNIMPLEMENTED   BugCheck((DPRINT_WARNING, "Unimplemented\n"));

static BOOL bReportError = TRUE;

/////////////////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS
/////////////////////////////////////////////////////////////////////////////////////////////

VOID DiskReportError (BOOL bError)
{
	bReportError = bError;
}

VOID DiskError(PCHAR ErrorString, ULONG ErrorCode)
{
	CHAR	ErrorCodeString[200];

	if (bReportError == FALSE)
		return;

	sprintf(ErrorCodeString, "%s\n\nError Code: 0x%x\nError: %s", ErrorString, ErrorCode, DiskGetErrorCodeString(ErrorCode));

	DbgPrint((DPRINT_DISK, "%s\n", ErrorCodeString));

	UiMessageBox(ErrorCodeString);
}

PCHAR DiskGetErrorCodeString(ULONG ErrorCode)
{
	switch (ErrorCode)
	{
	case 0x00:  return "no error";
	case 0x01:  return "bad command passed to driver";
	case 0x02:  return "address mark not found or bad sector";
	case 0x03:  return "diskette write protect error";
	case 0x04:  return "sector not found";
	case 0x05:  return "fixed disk reset failed";
	case 0x06:  return "diskette changed or removed";
	case 0x07:  return "bad fixed disk parameter table";
	case 0x08:  return "DMA overrun";
	case 0x09:  return "DMA access across 64k boundary";
	case 0x0A:  return "bad fixed disk sector flag";
	case 0x0B:  return "bad fixed disk cylinder";
	case 0x0C:  return "unsupported track/invalid media";
	case 0x0D:  return "invalid number of sectors on fixed disk format";
	case 0x0E:  return "fixed disk controlled data address mark detected";
	case 0x0F:  return "fixed disk DMA arbitration level out of range";
	case 0x10:  return "ECC/CRC error on disk read";
	case 0x11:  return "recoverable fixed disk data error, data fixed by ECC";
	case 0x20:  return "controller error (NEC for floppies)";
	case 0x40:  return "seek failure";
	case 0x80:  return "time out, drive not ready";
	case 0xAA:  return "fixed disk drive not ready";
	case 0xBB:  return "fixed disk undefined error";
	case 0xCC:  return "fixed disk write fault on selected drive";
	case 0xE0:  return "fixed disk status error/Error reg = 0";
	case 0xFF:  return "sense operation failed";

	default:  return "unknown error code";
	}
}

// This function is in arch/i386/i386disk.c
//BOOL DiskReadLogicalSectors(ULONG DriveNumber, U64 SectorNumber, ULONG SectorCount, PVOID Buffer)

BOOL DiskIsDriveRemovable(ULONG DriveNumber)
{
	// Hard disks use drive numbers >= 0x80
	// So if the drive number indicates a hard disk
	// then return FALSE
	if (DriveNumber >= 0x80)
	{
		return FALSE;
	}

	// Drive is a floppy diskette so return TRUE
	return TRUE;
}

// This function is in arch/i386/i386disk.c
//VOID DiskStopFloppyMotor(VOID)

// This function is in arch/i386/i386disk.c
//ULONG DiskGetCacheableBlockCount(ULONG DriveNumber)
