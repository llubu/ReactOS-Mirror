/*
 *  ReactOS kernel
 *  Copyright (C) 2002 ReactOS Team
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
/* $Id: partlist.c,v 1.4 2002/11/13 18:25:18 ekohl Exp $
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            subsys/system/usetup/partlist.c
 * PURPOSE:         Partition list functions
 * PROGRAMMER:      Eric Kohl
 */

#include <ddk/ntddk.h>
#include <ddk/ntddscsi.h>

#include <ntdll/rtl.h>

#include <ntos/minmax.h>

#include "usetup.h"
#include "console.h"
#include "partlist.h"
#include "drivesup.h"


/* FUNCTIONS ****************************************************************/

static VOID
AddPartitionList(ULONG DiskNumber,
		 PDISKENTRY DiskEntry,
		 DRIVE_LAYOUT_INFORMATION *LayoutBuffer)
{
  PPARTENTRY PartEntry;
  ULONG i;
  ULONG EntryCount;


  /*
   * FIXME:
   * Determine required number of partiton entries.
   * This must include entries for unused disk space.
   */

  /* Check for unpartitioned disk */
  if (LayoutBuffer->PartitionCount == 0)
    {
      EntryCount = 1;
    }
  else
    {

#if 0
  for (i = 0; i < LayoutBuffer->PartitionCount; i++)
    {

    }
#endif

      EntryCount = LayoutBuffer->PartitionCount;
    }


  DiskEntry->PartArray = (PPARTENTRY)RtlAllocateHeap(ProcessHeap,
						     0,
						     EntryCount * sizeof(PARTENTRY));
  DiskEntry->PartCount = EntryCount;

  RtlZeroMemory(DiskEntry->PartArray,
		EntryCount * sizeof(PARTENTRY));

  if (LayoutBuffer->PartitionCount == 0)
    {
      /* Initialize an 'Unpartitioned space' entry */
      PartEntry = &DiskEntry->PartArray[0];

      PartEntry->Unpartitioned = TRUE;
      PartEntry->PartSize = 0; /* ?? */

      PartEntry->Used = TRUE;
    }
  else
    {
      for (i = 0; i < LayoutBuffer->PartitionCount; i++)
	{
	  PartEntry = &DiskEntry->PartArray[i];

	  if ((LayoutBuffer->PartitionEntry[i].PartitionType != PARTITION_ENTRY_UNUSED) &&
	      (!IsContainerPartition(LayoutBuffer->PartitionEntry[i].PartitionType)))
	    {
	      PartEntry->PartSize = LayoutBuffer->PartitionEntry[i].PartitionLength.QuadPart;
	      PartEntry->PartNumber = LayoutBuffer->PartitionEntry[i].PartitionNumber,
	      PartEntry->PartType = LayoutBuffer->PartitionEntry[i].PartitionType;

	      PartEntry->DriveLetter = GetDriveLetter(DiskNumber,
						      LayoutBuffer->PartitionEntry[i].PartitionNumber);

	      PartEntry->Unpartitioned = FALSE;

	      PartEntry->Used = TRUE;
	    }
	  else
	    {
	      PartEntry->Used = FALSE;
	    }
	}
    }
}


PPARTLIST
CreatePartitionList(SHORT Left,
		    SHORT Top,
		    SHORT Right,
		    SHORT Bottom)
{
  PPARTLIST List;
  OBJECT_ATTRIBUTES ObjectAttributes;
  SYSTEM_DEVICE_INFORMATION Sdi;
  DISK_GEOMETRY DiskGeometry;
  IO_STATUS_BLOCK Iosb;
  ULONG ReturnSize;
  NTSTATUS Status;
  ULONG DiskNumber;
  WCHAR Buffer[MAX_PATH];
  UNICODE_STRING Name;
  HANDLE FileHandle;
  DRIVE_LAYOUT_INFORMATION *LayoutBuffer;
  SCSI_ADDRESS ScsiAddress;

  List = (PPARTLIST)RtlAllocateHeap(ProcessHeap, 0, sizeof(PARTLIST));
  if (List == NULL)
    return(NULL);

  List->Left = Left;
  List->Top = Top;
  List->Right = Right;
  List->Bottom = Bottom;

  List->Line = 0;

  List->TopDisk = (ULONG)-1;
  List->TopPartition = (ULONG)-1;

  List->CurrentDisk = (ULONG)-1;
  List->CurrentPartition = (ULONG)-1;

  List->DiskCount = 0;
  List->DiskArray = NULL;


  Status = NtQuerySystemInformation(SystemDeviceInformation,
				    &Sdi,
				    sizeof(SYSTEM_DEVICE_INFORMATION),
				    &ReturnSize);
  if (!NT_SUCCESS(Status))
    {
      RtlFreeHeap(ProcessHeap, 0, List);
      return(NULL);
    }

  List->DiskArray = (PDISKENTRY)RtlAllocateHeap(ProcessHeap,
						0,
						Sdi.NumberOfDisks * sizeof(DISKENTRY));
  List->DiskCount = Sdi.NumberOfDisks;

  for (DiskNumber = 0; DiskNumber < Sdi.NumberOfDisks; DiskNumber++)
    {
      swprintf(Buffer,
	       L"\\Device\\Harddisk%d\\Partition0",
	       DiskNumber);
      RtlInitUnicodeString(&Name,
			   Buffer);

      InitializeObjectAttributes(&ObjectAttributes,
				 &Name,
				 0,
				 NULL,
				 NULL);

      Status = NtOpenFile(&FileHandle,
			  0x10001,
			  &ObjectAttributes,
			  &Iosb,
			  1,
			  FILE_SYNCHRONOUS_IO_NONALERT);
      if (NT_SUCCESS(Status))
	{
	  Status = NtDeviceIoControlFile(FileHandle,
					 NULL,
					 NULL,
					 NULL,
					 &Iosb,
					 IOCTL_DISK_GET_DRIVE_GEOMETRY,
					 NULL,
					 0,
					 &DiskGeometry,
					 sizeof(DISK_GEOMETRY));
	  if (NT_SUCCESS(Status))
	    {
	      if (DiskGeometry.MediaType == FixedMedia)
		{
		  Status = NtDeviceIoControlFile(FileHandle,
						 NULL,
						 NULL,
						 NULL,
						 &Iosb,
						 IOCTL_SCSI_GET_ADDRESS,
						 NULL,
						 0,
						 &ScsiAddress,
						 sizeof(SCSI_ADDRESS));


		  List->DiskArray[DiskNumber].DiskSize = 
		    DiskGeometry.Cylinders.QuadPart *
		    (ULONGLONG)DiskGeometry.TracksPerCylinder *
		    (ULONGLONG)DiskGeometry.SectorsPerTrack *
		    (ULONGLONG)DiskGeometry.BytesPerSector;
		  List->DiskArray[DiskNumber].DiskNumber = DiskNumber;
		  List->DiskArray[DiskNumber].Port = ScsiAddress.PortNumber;
		  List->DiskArray[DiskNumber].Bus = ScsiAddress.PathId;
		  List->DiskArray[DiskNumber].Id = ScsiAddress.TargetId;

		  List->DiskArray[DiskNumber].FixedDisk = TRUE;


		  LayoutBuffer = (DRIVE_LAYOUT_INFORMATION*)RtlAllocateHeap(ProcessHeap, 0, 8192);

		  Status = NtDeviceIoControlFile(FileHandle,
						 NULL,
						 NULL,
						 NULL,
						 &Iosb,
						 IOCTL_DISK_GET_DRIVE_LAYOUT,
						 NULL,
						 0,
						 LayoutBuffer,
						 8192);
		  if (NT_SUCCESS(Status))
		    {
		      AddPartitionList(DiskNumber,
				       &List->DiskArray[DiskNumber],
				       LayoutBuffer);
		    }

		  RtlFreeHeap(ProcessHeap, 0, LayoutBuffer);
		}
	      else
		{
		  /* mark removable disk entry */
		  List->DiskArray[DiskNumber].FixedDisk = FALSE;
		  List->DiskArray[DiskNumber].PartCount = 0;
		  List->DiskArray[DiskNumber].PartArray = NULL;
		}
	    }

	  NtClose(FileHandle);
	}
    }

  List->TopDisk = 0;
  List->TopPartition = 0;

  /* FIXME: search for first usable disk and partition */
  List->CurrentDisk = 0;
  List->CurrentPartition = 0;

  DrawPartitionList(List);

  return(List);
}


VOID
DestroyPartitionList(PPARTLIST List)
{
  ULONG i;
#if 0
  COORD coPos;
  USHORT Width;

  /* clear occupied screen area */
  coPos.X = List->Left;
  Width = List->Right - List->Left + 1;
  for (coPos.Y = List->Top; coPos.Y <= List->Bottom; coPos.Y++)
    {
      FillConsoleOutputAttribute(0x17,
				 Width,
				 coPos,
				 &i);

      FillConsoleOutputCharacter(' ',
				 Width,
				 coPos,
				 &i);
    }
#endif

  /* free disk and partition info */
  if (List->DiskArray != NULL)
    {
      /* free partition arrays */
      for (i = 0; i < List->DiskCount; i++)
	{
	  if (List->DiskArray[i].PartArray != NULL)
	    {
	      RtlFreeHeap(ProcessHeap, 0, List->DiskArray[i].PartArray);
	      List->DiskArray[i].PartCount = 0;
	      List->DiskArray[i].PartArray = NULL;
	    }
	}

      /* free disk array */
      RtlFreeHeap(ProcessHeap, 0, List->DiskArray);
      List->DiskCount = 0;
      List->DiskArray = NULL;
    }

  /* free list head */
  RtlFreeHeap(ProcessHeap, 0, List);
}


static VOID
PrintEmptyLine(PPARTLIST List)
{
  COORD coPos;
  ULONG Written;
  USHORT Width;
  USHORT Height;

  Width = List->Right - List->Left - 1;
  Height = List->Bottom - List->Top - 1;

  if (List->Line < 0 || List->Line > Height)
    return;

  coPos.X = List->Left + 1;
  coPos.Y = List->Top + 1 + List->Line;

  FillConsoleOutputAttribute(0x17,
			     Width,
			     coPos,
			     &Written);

  FillConsoleOutputCharacter(' ',
			     Width,
			     coPos,
			     &Written);

  List->Line++;
}


static VOID
PrintPartitionData(PPARTLIST List,
		   SHORT DiskIndex,
		   SHORT PartIndex)
{
  PPARTENTRY PartEntry;
  CHAR LineBuffer[128];
  COORD coPos;
  ULONG Written;
  USHORT Width;
  USHORT Height;

  ULONGLONG PartSize;
  PCHAR Unit;
  UCHAR Attribute;
  PCHAR PartType;

  Width = List->Right - List->Left - 1;
  Height = List->Bottom - List->Top - 1;

  if (List->Line < 0 || List->Line > Height)
    return;

  coPos.X = List->Left + 1;
  coPos.Y = List->Top + 1 + List->Line;

  PartEntry = &List->DiskArray[DiskIndex].PartArray[PartIndex];

  /* Determine partition type */
  PartType = NULL;
  if (PartEntry->Unpartitioned == FALSE)
    {
      if ((PartEntry->PartType == PARTITION_FAT_12) ||
	  (PartEntry->PartType == PARTITION_FAT_16) ||
	  (PartEntry->PartType == PARTITION_HUGE) ||
	  (PartEntry->PartType == PARTITION_XINT13))
	{
	  PartType = "FAT";
	}
      else if ((PartEntry->PartType == PARTITION_FAT32) ||
	       (PartEntry->PartType == PARTITION_FAT32_XINT13))
	{
	  PartType = "FAT32";
	}
     else if (PartEntry->PartType == PARTITION_IFS)
	{
	  PartType = "NTFS"; /* FIXME: Not quite correct! */
	}
    }


#if 0
  if (PartEntry->PartSize >= 0x280000000ULL) /* 10 GB */
    {
      PartSize = (PartEntry->PartSize + (1 << 29)) >> 30;
      Unit = "GB";
    }
  else
#endif
  if (PartEntry->PartSize >= 0xA00000ULL) /* 10 MB */
    {
      PartSize = (PartEntry->PartSize + (1 << 19)) >> 20;
      Unit = "MB";
    }
  else
    {
      PartSize = (PartEntry->PartSize + (1 << 9)) >> 10;
      Unit = "KB";
    }


  if (PartEntry->Unpartitioned == TRUE)
    {
      sprintf(LineBuffer,
	      "    Unpartitioned space              %I64u %s",
	      PartSize,
	      Unit);
    }
  else if (PartEntry->DriveLetter != (CHAR)0)
    {
      if (PartType == NULL)
	{
	  sprintf(LineBuffer,
		  "%c:  Type %-3lu                        %I64u %s",
		  PartEntry->DriveLetter,
		  PartEntry->PartType,
		  PartSize,
		  Unit);
	}
      else
	{
	  sprintf(LineBuffer,
		  "%c:  %s                         %I64u %s",
		  PartEntry->DriveLetter,
		  PartType,
		  PartSize,
		  Unit);
	}

#if 0
      sprintf(LineBuffer,
	      "%c:  %s  (%d: nr: %d type: %x)  %I64u %s",
	      PartEntry->DriveLetter,
	      PartType,
	      PartIndex,
	      PartEntry->PartNumber,
	      PartEntry->PartType,
	      PartSize,
	      Unit);
#endif
    }
  else
    {
      sprintf(LineBuffer,
	      "--  %s  (%d: nr: %d type: %x)  %I64u %s",
	      PartEntry->FileSystemName,
	      PartIndex,
	      PartEntry->PartNumber,
	      PartEntry->PartType,
	      PartSize,
	      Unit);
    }

  Attribute = (List->CurrentDisk == DiskIndex &&
	       List->CurrentPartition == PartIndex) ? 0x71 : 0x17;

  FillConsoleOutputCharacter(' ',
			     Width,
			     coPos,
			     &Written);

  coPos.X += 4;
  Width -= 8;
  FillConsoleOutputAttribute(Attribute,
			     Width,
			     coPos,
			     &Written);

  coPos.X++;
  Width -= 2;
  WriteConsoleOutputCharacters(LineBuffer,
			       min(strlen(LineBuffer), Width),
			       coPos);

  List->Line++;
}


static VOID
PrintDiskData(PPARTLIST List,
	      SHORT DiskIndex)
{
  PDISKENTRY DiskEntry;
  CHAR LineBuffer[128];
  COORD coPos;
  ULONG Written;
  USHORT Width;
  USHORT Height;
  ULONGLONG DiskSize;
  PCHAR Unit;
  SHORT PartIndex;

  DiskEntry = &List->DiskArray[DiskIndex];

  Width = List->Right - List->Left - 1;
  Height = List->Bottom - List->Top - 1;

  if (List->Line < 0 || List->Line > Height)
    return;

  coPos.X = List->Left + 1;
  coPos.Y = List->Top + 1 + List->Line;

#if 0
  if (DiskEntry->DiskSize >= 0x280000000ULL) /* 10 GB */
    {
      DiskSize = (DiskEntry->DiskSize + (1 << 29)) >> 30;
      Unit = "GB";
    }
  else
#endif
    {
      DiskSize = (DiskEntry->DiskSize + (1 << 19)) >> 20;
      if (DiskSize == 0)
	DiskSize = 1;
      Unit = "MB";
    }

  sprintf(LineBuffer,
	  "%I64u %s  Harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu)",
	  DiskSize,
	  Unit,
	  DiskEntry->DiskNumber,
	  DiskEntry->Port,
	  DiskEntry->Bus,
	  DiskEntry->Id);

  FillConsoleOutputAttribute(0x17,
			     Width,
			     coPos,
			     &Written);

  FillConsoleOutputCharacter(' ',
			     Width,
			     coPos,
			     &Written);

  coPos.X++;
  WriteConsoleOutputCharacters(LineBuffer,
			       min(strlen(LineBuffer), Width - 2),
			       coPos);

  List->Line++;

  /* Print separator line */
  PrintEmptyLine(List);


  /* Print partition lines*/
  for (PartIndex = 0; PartIndex < DiskEntry->PartCount; PartIndex++)
    {
      if (DiskEntry->PartArray[PartIndex].Used == TRUE)
	{
	  PrintPartitionData(List,
			     DiskIndex,
			     PartIndex);
	}
    }

  /* Print separator line */
  PrintEmptyLine(List);
}


VOID
DrawPartitionList(PPARTLIST List)
{
  CHAR LineBuffer[128];
  COORD coPos;
  ULONG Written;
  SHORT i;
  SHORT DiskIndex;

  /* draw upper left corner */
  coPos.X = List->Left;
  coPos.Y = List->Top;
  FillConsoleOutputCharacter(0xDA, // '+',
			     1,
			     coPos,
			     &Written);

  /* draw upper edge */
  coPos.X = List->Left + 1;
  coPos.Y = List->Top;
  FillConsoleOutputCharacter(0xC4, // '-',
			     List->Right - List->Left - 1,
			     coPos,
			     &Written);

  /* draw upper right corner */
  coPos.X = List->Right;
  coPos.Y = List->Top;
  FillConsoleOutputCharacter(0xBF, // '+',
			     1,
			     coPos,
			     &Written);

  /* draw left and right edge */
  for (i = List->Top + 1; i < List->Bottom; i++)
    {
      coPos.X = List->Left;
      coPos.Y = i;
      FillConsoleOutputCharacter(0xB3, // '|',
				 1,
				 coPos,
				 &Written);

      coPos.X = List->Right;
      FillConsoleOutputCharacter(0xB3, //'|',
				 1,
				 coPos,
				 &Written);
    }

  /* draw lower left corner */
  coPos.X = List->Left;
  coPos.Y = List->Bottom;
  FillConsoleOutputCharacter(0xC0, // '+',
			     1,
			     coPos,
			     &Written);

  /* draw lower edge */
  coPos.X = List->Left + 1;
  coPos.Y = List->Bottom;
  FillConsoleOutputCharacter(0xC4, // '-',
			     List->Right - List->Left - 1,
			     coPos,
			     &Written);

  /* draw lower right corner */
  coPos.X = List->Right;
  coPos.Y = List->Bottom;
  FillConsoleOutputCharacter(0xD9, // '+',
			     1,
			     coPos,
			     &Written);

  /* print list entries */
  List->Line = 0;
  for (DiskIndex = 0; DiskIndex < List->DiskCount; DiskIndex++)
    {
      if (List->DiskArray[DiskIndex].FixedDisk == TRUE)
	{
	  /* Print disk entry */
	  PrintDiskData(List,
			DiskIndex);
	}
    }
}


VOID
ScrollDownPartitionList(PPARTLIST List)
{
  ULONG i;
  ULONG j;

  /* check for available disks */
  if (List->DiskCount == 0)
    return;

  /* check for next usable entry on current disk */
  for (i = List->CurrentPartition + 1; i < List->DiskArray[List->CurrentDisk].PartCount; i++)
    {
      if (List->DiskArray[List->CurrentDisk].PartArray[i].Used == TRUE)
	{
	  List->CurrentPartition = i;
	  DrawPartitionList(List);
	  return;
	}
    }

  /* check for first usable entry on next disk */
  for (j = List->CurrentDisk + 1; j < List->DiskCount; j++)
    {
      for (i = 0; i < List->DiskArray[j].PartCount; i++)
	{
	  if (List->DiskArray[j].PartArray[i].Used == TRUE)
	    {
	      List->CurrentDisk = j;
	      List->CurrentPartition = i;
	      DrawPartitionList(List);
	      return;
	    }
	}
    }
}


VOID
ScrollUpPartitionList(PPARTLIST List)
{
  ULONG i;
  ULONG j;

  /* check for available disks */
  if (List->DiskCount == 0)
    return;

  /* check for previous usable entry on current disk */
  for (i = List->CurrentPartition - 1; i != (ULONG)-1; i--)
    {
      if (List->DiskArray[List->CurrentDisk].PartArray[i].Used == TRUE)
	{
	  List->CurrentPartition = i;
	  DrawPartitionList(List);
	  return;
	}
    }

  /* check for last usable entry on previous disk */
  for (j = List->CurrentDisk - 1; j != (ULONG)-1; j--)
    {
      for (i = List->DiskArray[j].PartCount - 1; i != (ULONG)-1; i--)
	{
	  if (List->DiskArray[j].PartArray[i].Used == TRUE)
	    {
	      List->CurrentDisk = j;
	      List->CurrentPartition = i;
	      DrawPartitionList(List);
	      return;
	    }
	}
    }
}


BOOL
GetPartitionData(PPARTLIST List,
		 PPARTDATA Data)
{
  if (List->CurrentDisk >= List->DiskCount)
    return(FALSE);

  if (List->DiskArray[List->CurrentDisk].FixedDisk == FALSE)
    return(FALSE);

  if (List->CurrentPartition >= List->DiskArray[List->CurrentDisk].PartCount)
    return(FALSE);

  if (List->DiskArray[List->CurrentDisk].PartArray[List->CurrentPartition].Used == FALSE)
    return(FALSE);

  Data->DiskSize = List->DiskArray[List->CurrentDisk].DiskSize;
  Data->DiskNumber = List->DiskArray[List->CurrentDisk].DiskNumber;
  Data->Port = List->DiskArray[List->CurrentDisk].Port;
  Data->Bus = List->DiskArray[List->CurrentDisk].Bus;
  Data->Id = List->DiskArray[List->CurrentDisk].Id;

  Data->PartSize = List->DiskArray[List->CurrentDisk].PartArray[List->CurrentPartition].PartSize;
  Data->PartNumber = List->DiskArray[List->CurrentDisk].PartArray[List->CurrentPartition].PartNumber;
  Data->PartType = List->DiskArray[List->CurrentDisk].PartArray[List->CurrentPartition].PartType;

  Data->DriveLetter = List->DiskArray[List->CurrentDisk].PartArray[List->CurrentPartition].DriveLetter;

  return(TRUE);
}

/* EOF */
