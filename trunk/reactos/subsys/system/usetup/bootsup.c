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
/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            subsys/system/usetup/bootsup.c
 * PURPOSE:         Bootloader support functions
 * PROGRAMMER:      Eric Kohl
 */

#include <ddk/ntddk.h>
#include <ntdll/rtl.h>

#include "usetup.h"
#include "inicache.h"
#include "filesup.h"
#include "bootsup.h"

#define NDEBUG
#include <debug.h>

#define SECTORSIZE 512

/* FUNCTIONS ****************************************************************/


static VOID
CreateCommonFreeLoaderSections(PINICACHE IniCache)
{
  PINICACHESECTION IniSection;

  /* Create "FREELOADER" section */
  IniSection = IniCacheAppendSection(IniCache,
				     L"FREELOADER");

  /* DefaultOS=ReactOS */
  IniCacheInsertKey(IniSection,
		    NULL,
		    INSERT_LAST,
		    L"DefaultOS",
		    L"ReactOS");

#if 0
  /* Timeout=10 */
  IniCacheInsertKey(IniSection,
		    NULL,
		    INSERT_LAST,
		    L"TimeOut",
		    L"10");
#endif

  /* Create "Display" section */
  IniSection = IniCacheAppendSection(IniCache,
				     L"Display");

  /* TitleText=ReactOS Boot Manager */
  IniCacheInsertKey(IniSection,
		    NULL,
		    INSERT_LAST,
		    L"TitleText",
		    L"ReactOS Boot Manager");

  /* StatusBarColor=Cyan */
  IniCacheInsertKey(IniSection,
		    NULL,
		    INSERT_LAST,
		    L"StatusBarColor",
		    L"Cyan");

  /* StatusBarTextColor=Black */
  IniCacheInsertKey(IniSection,
		    NULL,
		    INSERT_LAST,
		    L"StatusBarTextColor",
		    L"Black");

  /* BackdropTextColor=White */
  IniCacheInsertKey(IniSection,
		    NULL,
		    INSERT_LAST,
		    L"BackdropTextColor",
		    L"White");

  /* BackdropColor=Blue */
  IniCacheInsertKey(IniSection,
		    NULL,
		    INSERT_LAST,
		    L"BackdropColor",
		    L"Blue");

  /* BackdropFillStyle=Medium */
  IniCacheInsertKey(IniSection,
		    NULL,
		    INSERT_LAST,
		    L"BackdropFillStyle",
		    L"Medium");

  /* TitleBoxTextColor=White */
  IniCacheInsertKey(IniSection,
		    NULL,
		    INSERT_LAST,
		    L"TitleBoxTextColor",
		    L"White");

  /* TitleBoxColor=Red */
  IniCacheInsertKey(IniSection,
		    NULL,
		    INSERT_LAST,
		    L"TitleBoxColor",
		    L"Red");

  /* MessageBoxTextColor=White */
  IniCacheInsertKey(IniSection,
		    NULL,
		    INSERT_LAST,
		    L"MessageBoxTextColor",
		    L"White");

  /* MessageBoxColor=Blue */
  IniCacheInsertKey(IniSection,
		    NULL,
		    INSERT_LAST,
		    L"MessageBoxColor",
		    L"Blue");

  /* MenuTextColor=White */
  IniCacheInsertKey(IniSection,
		    NULL,
		    INSERT_LAST,
		    L"MenuTextColor",
		    L"White");

  /* MenuColor=Blue */
  IniCacheInsertKey(IniSection,
		    NULL,
		    INSERT_LAST,
		    L"MenuColor",
		    L"Blue");

  /* TextColor=Yellow */
  IniCacheInsertKey(IniSection,
		    NULL,
		    INSERT_LAST,
		    L"TextColor",
		    L"Yellow");

  /* SelectedTextColor=Black */
  IniCacheInsertKey(IniSection,
		    NULL,
		    INSERT_LAST,
		    L"SelectedTextColor",
		    L"Black");

  /* SelectedColor=Gray */
  IniCacheInsertKey(IniSection,
		    NULL,
		    INSERT_LAST,
		    L"SelectedColor",
		    L"Gray");
}


NTSTATUS
CreateFreeLoaderIniForDos(PWCHAR IniPath,
			  PWCHAR ArcPath)
{
  PINICACHE IniCache;
  PINICACHESECTION IniSection;

  IniCache = IniCacheCreate();

  CreateCommonFreeLoaderSections(IniCache);

  /* Create "Operating Systems" section */
  IniSection = IniCacheAppendSection(IniCache,
				     L"Operating Systems");

  /* REACTOS=ReactOS */
  IniCacheInsertKey(IniSection,
		    NULL,
		    INSERT_LAST,
		    L"ReactOS",
		    L"\"ReactOS\"");

  /* ReactOS_Debug="ReactOS (Debug)" */
  IniCacheInsertKey(IniSection,
		    NULL,
		    INSERT_LAST,
		    L"ReactOS_Debug",
		    L"\"ReactOS (Debug)\"");

  /* DOS=Dos/Windows */
  IniCacheInsertKey(IniSection,
		    NULL,
		    INSERT_LAST,
		    L"DOS",
		    L"\"DOS/Windows\"");

  /* Create "ReactOS" section */
  IniSection = IniCacheAppendSection(IniCache,
				     L"ReactOS");

  /* BootType=ReactOS */
  IniCacheInsertKey(IniSection,
		    NULL,
		    INSERT_LAST,
		    L"BootType",
		    L"ReactOS");

  /* SystemPath=<ArcPath> */
  IniCacheInsertKey(IniSection,
		    NULL,
		    INSERT_LAST,
		    L"SystemPath",
		    ArcPath);

  /* Create "ReactOS_Debug" section */
  IniSection = IniCacheAppendSection(IniCache,
				     L"ReactOS_Debug");

  /* BootType=ReactOS */
  IniCacheInsertKey(IniSection,
		    NULL,
		    INSERT_LAST,
		    L"BootType",
		    L"ReactOS");

  /* SystemPath=<ArcPath> */
  IniCacheInsertKey(IniSection,
		    NULL,
		    INSERT_LAST,
		    L"SystemPath",
		    ArcPath);

  /* Options=/DEBUGPORT=SCREEN */
  IniCacheInsertKey(IniSection,
		    NULL,
		    INSERT_LAST,
		    L"Options",
		    L"/DEBUGPORT=SCREEN");

  /* Create "DOS" section */
  IniSection = IniCacheAppendSection(IniCache,
				     L"DOS");

  /* BootType=BootSector */
  IniCacheInsertKey(IniSection,
		    NULL,
		    INSERT_LAST,
		    L"BootType",
		    L"BootSector");

  /* BootDrive=hd0 */
  IniCacheInsertKey(IniSection,
		    NULL,
		    INSERT_LAST,
		    L"BootDrive",
		    L"hd0");

  /* BootPartition=1 */
  IniCacheInsertKey(IniSection,
		    NULL,
		    INSERT_LAST,
		    L"BootPartition",
		    L"1");

  /* BootSector=BOOTSECT.DOS */
  IniCacheInsertKey(IniSection,
		    NULL,
		    INSERT_LAST,
		    L"BootSectorFile",
		    L"BOOTSECT.DOS");

  IniCacheSave(IniCache, IniPath);
  IniCacheDestroy(IniCache);

  return(STATUS_SUCCESS);
}


NTSTATUS
CreateFreeLoaderIniForReactos(PWCHAR IniPath,
			      PWCHAR ArcPath)
{
  PINICACHE IniCache;
  PINICACHESECTION IniSection;

  IniCache = IniCacheCreate();

  CreateCommonFreeLoaderSections(IniCache);

  /* Create "Operating Systems" section */
  IniSection = IniCacheAppendSection(IniCache,
				     L"Operating Systems");

  /* ReactOS="ReactOS" */
  IniCacheInsertKey(IniSection,
		    NULL,
		    INSERT_LAST,
		    L"ReactOS",
		    L"\"ReactOS\"");

  /* ReactOS_Debug="ReactOS (Debug)" */
  IniCacheInsertKey(IniSection,
		    NULL,
		    INSERT_LAST,
		    L"ReactOS_Debug",
		    L"\"ReactOS (Debug)\"");

  /* Create "ReactOS" section */
  IniSection = IniCacheAppendSection(IniCache,
				     L"ReactOS");

  /* BootType=ReactOS */
  IniCacheInsertKey(IniSection,
		    NULL,
		    INSERT_LAST,
		    L"BootType",
		    L"ReactOS");

  /* SystemPath=<ArcPath> */
  IniCacheInsertKey(IniSection,
		    NULL,
		    INSERT_LAST,
		    L"SystemPath",
		    ArcPath);

  /* Create "ReactOS_Debug" section */
  IniSection = IniCacheAppendSection(IniCache,
				     L"ReactOS_Debug");

  /* BootType=ReactOS */
  IniCacheInsertKey(IniSection,
		    NULL,
		    INSERT_LAST,
		    L"BootType",
		    L"ReactOS");

  /* SystemPath=<ArcPath> */
  IniCacheInsertKey(IniSection,
		    NULL,
		    INSERT_LAST,
		    L"SystemPath",
		    ArcPath);

  /* Options=/DEBUGPORT=SCREEN */
  IniCacheInsertKey(IniSection,
		    NULL,
		    INSERT_LAST,
		    L"Options",
		    L"/DEBUGPORT=SCREEN");

  /* Save the ini file */
  IniCacheSave(IniCache, IniPath);
  IniCacheDestroy(IniCache);

  return(STATUS_SUCCESS);
}


NTSTATUS
UpdateFreeLoaderIni(PWCHAR IniPath,
		    PWCHAR ArcPath)
{
  UNICODE_STRING Name;
  PINICACHE IniCache;
  PINICACHESECTION IniSection;
  PINICACHESECTION OsIniSection;
  WCHAR SectionName[80];
  WCHAR OsName[80];
  WCHAR SystemPath[200];
  WCHAR SectionName2[200];
  PWCHAR KeyData;
  ULONG i,j;
  NTSTATUS Status;

  RtlInitUnicodeString(&Name,
		       IniPath);

  Status = IniCacheLoad(&IniCache,
			&Name,
			FALSE);
  if (!NT_SUCCESS(Status))
    return(Status);

  /* Get "Operating Systems" section */
  IniSection = IniCacheGetSection(IniCache,
				  L"Operating Systems");
  if (IniSection == NULL)
  {
    IniCacheDestroy(IniCache);
    return(STATUS_UNSUCCESSFUL);
  }

  /* Find an existing usable or an unused section name */
  i = 1;
  wcscpy(SectionName, L"ReactOS");
  wcscpy(OsName, L"\"ReactOS\"");
  while(TRUE)
  {
    Status = IniCacheGetKey(IniSection,
			    SectionName,
			    &KeyData);
    if (!NT_SUCCESS(Status))
      break;

    /* Get operation system section */
    if (KeyData[0] == '"')
    {
      wcscpy(SectionName2, &KeyData[1]);
      j = wcslen(SectionName2);
      if (j > 0)
      {
        SectionName2[j-1] = 0;
      }
    }
    else
    {
      wcscpy(SectionName2, KeyData);
    }

    OsIniSection = IniCacheGetSection(IniCache,
      SectionName2);
    if (OsIniSection != NULL)
    {
      BOOLEAN UseExistingEntry = TRUE;

      /* Check BootType */
      Status = IniCacheGetKey(OsIniSection,
        L"BootType",
        &KeyData);
      if (NT_SUCCESS(Status))
      {
        if (KeyData == NULL
          || (_wcsicmp(KeyData, L"ReactOS") != 0
          && _wcsicmp(KeyData, L"\"ReactOS\"") != 0))
        {
          /* This is not a ReactOS entry */
          UseExistingEntry = FALSE;
        }
      }
      else
      {
        UseExistingEntry = FALSE;
      }

      if (UseExistingEntry)
      {
        /* BootType is ReactOS. Now check SystemPath */
        Status = IniCacheGetKey(OsIniSection,
          L"SystemPath",
          &KeyData);
        if (NT_SUCCESS(Status))
        {
          swprintf(SystemPath, L"\"%S\"", ArcPath);
          if (KeyData == NULL
            || (_wcsicmp(KeyData, ArcPath) != 0
            && _wcsicmp(KeyData, SystemPath) != 0))
          {
            /* This entry is a ReactOS entry, but the SystemRoot does not
               match the one we are looking for */
            UseExistingEntry = FALSE;
          }
        }
        else
        {
          UseExistingEntry = FALSE;
        }
      }

      if (UseExistingEntry)
      {
        IniCacheDestroy(IniCache);
        return(STATUS_SUCCESS);
      }
    }

    swprintf(SectionName, L"ReactOS_%lu", i);
    swprintf(OsName, L"\"ReactOS %lu\"", i);
    i++;
  }

  /* <SectionName>=<OsName> */
  IniCacheInsertKey(IniSection,
		    NULL,
		    INSERT_LAST,
		    SectionName,
		    OsName);

  /* Create <SectionName> section */
  IniSection = IniCacheAppendSection(IniCache,
				     SectionName);

  /* BootType=ReactOS */
  IniCacheInsertKey(IniSection,
		    NULL,
		    INSERT_LAST,
		    L"BootType",
		    L"ReactOS");

  /* SystemPath=<ArcPath> */
  IniCacheInsertKey(IniSection,
		    NULL,
		    INSERT_LAST,
		    L"SystemPath",
		    ArcPath);

  IniCacheSave(IniCache, IniPath);
  IniCacheDestroy(IniCache);

  return(STATUS_SUCCESS);
}


NTSTATUS
SaveCurrentBootSector(PWSTR RootPath,
		      PWSTR DstPath)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  IO_STATUS_BLOCK IoStatusBlock;
  UNICODE_STRING Name;
  HANDLE FileHandle;
  NTSTATUS Status;
  PUCHAR BootSector;

  /* Allocate buffer for bootsector */
  BootSector = (PUCHAR)RtlAllocateHeap(ProcessHeap,
				       0,
				       SECTORSIZE);
  if (BootSector == NULL)
    return(STATUS_INSUFFICIENT_RESOURCES);

  /* Read current boot sector into buffer */
  RtlInitUnicodeString(&Name,
		       RootPath);

  InitializeObjectAttributes(&ObjectAttributes,
			     &Name,
			     OBJ_CASE_INSENSITIVE,
			     NULL,
			     NULL);

  Status = NtOpenFile(&FileHandle,
		      FILE_READ_ACCESS,
		      &ObjectAttributes,
		      &IoStatusBlock,
		      0,
		      FILE_SYNCHRONOUS_IO_NONALERT);
  if (!NT_SUCCESS(Status))
  {
    RtlFreeHeap(ProcessHeap, 0, BootSector);
    return(Status);
  }

  Status = NtReadFile(FileHandle,
		      NULL,
		      NULL,
		      NULL,
		      &IoStatusBlock,
		      BootSector,
		      SECTORSIZE,
		      NULL,
		      NULL);
  NtClose(FileHandle);
  if (!NT_SUCCESS(Status))
  {
    RtlFreeHeap(ProcessHeap, 0, BootSector);
    return(Status);
  }

  /* Write bootsector to DstPath */
  RtlInitUnicodeString(&Name,
		       DstPath);

  InitializeObjectAttributes(&ObjectAttributes,
			     &Name,
			     0,
			     NULL,
			     NULL);

  Status = NtCreateFile(&FileHandle,
			FILE_WRITE_ACCESS,
			&ObjectAttributes,
			&IoStatusBlock,
			NULL,
			FILE_ATTRIBUTE_NORMAL,
			0,
			FILE_SUPERSEDE,
			FILE_SYNCHRONOUS_IO_NONALERT | FILE_SEQUENTIAL_ONLY,
			NULL,
			0);
  if (!NT_SUCCESS(Status))
  {
    RtlFreeHeap(ProcessHeap, 0, BootSector);
    return(Status);
  }

  Status = NtWriteFile(FileHandle,
		       NULL,
		       NULL,
		       NULL,
		       &IoStatusBlock,
		       BootSector,
		       SECTORSIZE,
		       NULL,
		       NULL);
  NtClose(FileHandle);

  /* Free the new boot sector */
  RtlFreeHeap(ProcessHeap, 0, BootSector);

  return(Status);
}


NTSTATUS
InstallFat16BootCodeToFile(PWSTR SrcPath,
			   PWSTR DstPath,
			   PWSTR RootPath)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  IO_STATUS_BLOCK IoStatusBlock;
  UNICODE_STRING Name;
  HANDLE FileHandle;
  NTSTATUS Status;
  PUCHAR OrigBootSector;
  PUCHAR NewBootSector;

  /* Allocate buffer for original bootsector */
  OrigBootSector = (PUCHAR)RtlAllocateHeap(ProcessHeap,
					   0,
					   SECTORSIZE);
  if (OrigBootSector == NULL)
    return(STATUS_INSUFFICIENT_RESOURCES);

  /* Read current boot sector into buffer */
  RtlInitUnicodeString(&Name,
		       RootPath);

  InitializeObjectAttributes(&ObjectAttributes,
			     &Name,
			     OBJ_CASE_INSENSITIVE,
			     NULL,
			     NULL);

  Status = NtOpenFile(&FileHandle,
		      FILE_READ_ACCESS,
		      &ObjectAttributes,
		      &IoStatusBlock,
		      0,
		      FILE_SYNCHRONOUS_IO_NONALERT);
  if (!NT_SUCCESS(Status))
  {
    RtlFreeHeap(ProcessHeap, 0, OrigBootSector);
    return(Status);
  }

  Status = NtReadFile(FileHandle,
		      NULL,
		      NULL,
		      NULL,
		      &IoStatusBlock,
		      OrigBootSector,
		      SECTORSIZE,
		      NULL,
		      NULL);
  NtClose(FileHandle);
  if (!NT_SUCCESS(Status))
  {
    RtlFreeHeap(ProcessHeap, 0, OrigBootSector);
    return(Status);
  }


  /* Allocate buffer for new bootsector */
  NewBootSector = (PUCHAR)RtlAllocateHeap(ProcessHeap,
					  0,
					  SECTORSIZE);
  if (NewBootSector == NULL)
  {
    RtlFreeHeap(ProcessHeap, 0, OrigBootSector);
    return(STATUS_INSUFFICIENT_RESOURCES);
  }

  /* Read new bootsector from SrcPath */
  RtlInitUnicodeString(&Name,
		       SrcPath);

  InitializeObjectAttributes(&ObjectAttributes,
			     &Name,
			     OBJ_CASE_INSENSITIVE,
			     NULL,
			     NULL);

  Status = NtOpenFile(&FileHandle,
		      FILE_READ_ACCESS,
		      &ObjectAttributes,
		      &IoStatusBlock,
		      0,
		      FILE_SYNCHRONOUS_IO_NONALERT);
  if (!NT_SUCCESS(Status))
  {
    RtlFreeHeap(ProcessHeap, 0, OrigBootSector);
    RtlFreeHeap(ProcessHeap, 0, NewBootSector);
    return(Status);
  }

  Status = NtReadFile(FileHandle,
		      NULL,
		      NULL,
		      NULL,
		      &IoStatusBlock,
		      NewBootSector,
		      SECTORSIZE,
		      NULL,
		      NULL);
  NtClose(FileHandle);
  if (!NT_SUCCESS(Status))
  {
    RtlFreeHeap(ProcessHeap, 0, OrigBootSector);
    RtlFreeHeap(ProcessHeap, 0, NewBootSector);
    return(Status);
  }

  /* Adjust bootsector (copy a part of the FAT BPB) */
  memcpy((NewBootSector + 11), (OrigBootSector + 11), 51 /*fat BPB length*/);

  /* Free the original boot sector */
  RtlFreeHeap(ProcessHeap, 0, OrigBootSector);

  /* Write new bootsector to DstPath */
  RtlInitUnicodeString(&Name,
		       DstPath);

  InitializeObjectAttributes(&ObjectAttributes,
			     &Name,
			     0,
			     NULL,
			     NULL);

  Status = NtCreateFile(&FileHandle,
			FILE_WRITE_ACCESS,
			&ObjectAttributes,
			&IoStatusBlock,
			NULL,
			FILE_ATTRIBUTE_NORMAL,
			0,
			FILE_OVERWRITE_IF,
			FILE_SYNCHRONOUS_IO_NONALERT | FILE_SEQUENTIAL_ONLY,
			NULL,
			0);
  if (!NT_SUCCESS(Status))
  {
    RtlFreeHeap(ProcessHeap, 0, NewBootSector);
    return(Status);
  }

#if 0
  FilePosition.QuadPart = 0;
#endif
  Status = NtWriteFile(FileHandle,
		       NULL,
		       NULL,
		       NULL,
		       &IoStatusBlock,
		       NewBootSector,
		       SECTORSIZE,
		       NULL,
		       NULL);
  NtClose(FileHandle);

  /* Free the new boot sector */
  RtlFreeHeap(ProcessHeap, 0, NewBootSector);

  return(Status);
}


NTSTATUS
InstallFat32BootCodeToFile(PWSTR SrcPath,
			   PWSTR DstPath,
			   PWSTR RootPath)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  IO_STATUS_BLOCK IoStatusBlock;
  UNICODE_STRING Name;
  HANDLE FileHandle;
  NTSTATUS Status;
  PUCHAR OrigBootSector;
  PUCHAR NewBootSector;
  LARGE_INTEGER FileOffset;

  /* Allocate buffer for original bootsector */
  OrigBootSector = (PUCHAR)RtlAllocateHeap(ProcessHeap,
					   0,
					   SECTORSIZE);
  if (OrigBootSector == NULL)
    return(STATUS_INSUFFICIENT_RESOURCES);

  /* Read current boot sector into buffer */
  RtlInitUnicodeString(&Name,
		       RootPath);

  InitializeObjectAttributes(&ObjectAttributes,
			     &Name,
			     OBJ_CASE_INSENSITIVE,
			     NULL,
			     NULL);

  Status = NtOpenFile(&FileHandle,
		      FILE_READ_ACCESS,
		      &ObjectAttributes,
		      &IoStatusBlock,
		      0,
		      FILE_SYNCHRONOUS_IO_NONALERT);
  if (!NT_SUCCESS(Status))
  {
    RtlFreeHeap(ProcessHeap, 0, OrigBootSector);
    return(Status);
  }

  Status = NtReadFile(FileHandle,
		      NULL,
		      NULL,
		      NULL,
		      &IoStatusBlock,
		      OrigBootSector,
		      SECTORSIZE,
		      NULL,
		      NULL);
  NtClose(FileHandle);
  if (!NT_SUCCESS(Status))
  {
CHECKPOINT1;
    RtlFreeHeap(ProcessHeap, 0, OrigBootSector);
    return(Status);
  }

  /* Allocate buffer for new bootsector (2 sectors) */
  NewBootSector = (PUCHAR)RtlAllocateHeap(ProcessHeap,
					  0,
					  2 * SECTORSIZE);
  if (NewBootSector == NULL)
  {
    RtlFreeHeap(ProcessHeap, 0, OrigBootSector);
    return(STATUS_INSUFFICIENT_RESOURCES);
  }

  /* Read new bootsector from SrcPath */
  RtlInitUnicodeString(&Name,
		       SrcPath);

  InitializeObjectAttributes(&ObjectAttributes,
			     &Name,
			     OBJ_CASE_INSENSITIVE,
			     NULL,
			     NULL);

  Status = NtOpenFile(&FileHandle,
		      FILE_READ_ACCESS,
		      &ObjectAttributes,
		      &IoStatusBlock,
		      0,
		      FILE_SYNCHRONOUS_IO_NONALERT);
  if (!NT_SUCCESS(Status))
  {
    RtlFreeHeap(ProcessHeap, 0, OrigBootSector);
    RtlFreeHeap(ProcessHeap, 0, NewBootSector);
    return(Status);
  }

  Status = NtReadFile(FileHandle,
		      NULL,
		      NULL,
		      NULL,
		      &IoStatusBlock,
		      NewBootSector,
		      2 * SECTORSIZE,
		      NULL,
		      NULL);
  NtClose(FileHandle);
  if (!NT_SUCCESS(Status))
  {
    RtlFreeHeap(ProcessHeap, 0, OrigBootSector);
    RtlFreeHeap(ProcessHeap, 0, NewBootSector);
    return(Status);
  }

  /* Adjust bootsector (copy a part of the FAT32 BPB) */
  memcpy((NewBootSector + 3),
	 (OrigBootSector + 3),
	 87); /* FAT32 BPB length */

  /* Disable the backup boot sector */
  NewBootSector[0x32] = 0x00;
  NewBootSector[0x33] = 0x00;

  /* Free the original boot sector */
  RtlFreeHeap(ProcessHeap, 0, OrigBootSector);

  /* Write the first sector of the new bootcode to DstPath */
  RtlInitUnicodeString(&Name,
		       DstPath);

  InitializeObjectAttributes(&ObjectAttributes,
			     &Name,
			     0,
			     NULL,
			     NULL);

  Status = NtCreateFile(&FileHandle,
			FILE_WRITE_ACCESS,
			&ObjectAttributes,
			&IoStatusBlock,
			NULL,
			FILE_ATTRIBUTE_NORMAL,
			0,
			FILE_SUPERSEDE,
			FILE_SYNCHRONOUS_IO_NONALERT | FILE_SEQUENTIAL_ONLY,
			NULL,
			0);
  if (!NT_SUCCESS(Status))
  {
    RtlFreeHeap(ProcessHeap, 0, NewBootSector);
    return(Status);
  }

  Status = NtWriteFile(FileHandle,
		       NULL,
		       NULL,
		       NULL,
		       &IoStatusBlock,
		       NewBootSector,
		       SECTORSIZE,
		       NULL,
		       NULL);
  NtClose(FileHandle);
  if (!NT_SUCCESS(Status))
  {
    RtlFreeHeap(ProcessHeap, 0, NewBootSector);
    return(Status);
  }

  /* Write the second sector of the new bootcode to boot disk sector 14 */
  RtlInitUnicodeString(&Name,
		       RootPath);

  InitializeObjectAttributes(&ObjectAttributes,
			     &Name,
			     0,
			     NULL,
			     NULL);

  Status = NtOpenFile(&FileHandle,
		      FILE_WRITE_ACCESS,
		      &ObjectAttributes,
		      &IoStatusBlock,
		      0,
		      FILE_SYNCHRONOUS_IO_NONALERT);
  if (!NT_SUCCESS(Status))
  {
    RtlFreeHeap(ProcessHeap, 0, NewBootSector);
    return(Status);
  }

  FileOffset.QuadPart = (ULONGLONG)(14 * SECTORSIZE);
  Status = NtWriteFile(FileHandle,
		       NULL,
		       NULL,
		       NULL,
		       &IoStatusBlock,
		       (NewBootSector + SECTORSIZE),
		       SECTORSIZE,
		       &FileOffset,
		       NULL);
  if (!NT_SUCCESS(Status))
  {
  }
  NtClose(FileHandle);

  /* Free the new boot sector */
  RtlFreeHeap(ProcessHeap, 0, NewBootSector);

  return(Status);
}


NTSTATUS
InstallMbrBootCodeToDisk (PWSTR SrcPath,
			  PWSTR RootPath)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  IO_STATUS_BLOCK IoStatusBlock;
  UNICODE_STRING Name;
  HANDLE FileHandle;
  NTSTATUS Status;
  PUCHAR OrigBootSector;
  PUCHAR NewBootSector;

  /* Allocate buffer for original bootsector */
  OrigBootSector = (PUCHAR)RtlAllocateHeap(ProcessHeap,
					   0,
					   SECTORSIZE);
  if (OrigBootSector == NULL)
    return(STATUS_INSUFFICIENT_RESOURCES);

  /* Read current boot sector into buffer */
  RtlInitUnicodeString(&Name,
		       RootPath);

  InitializeObjectAttributes(&ObjectAttributes,
			     &Name,
			     OBJ_CASE_INSENSITIVE,
			     NULL,
			     NULL);

  Status = NtOpenFile(&FileHandle,
		      FILE_READ_ACCESS,
		      &ObjectAttributes,
		      &IoStatusBlock,
		      0,
		      FILE_SYNCHRONOUS_IO_NONALERT);
  if (!NT_SUCCESS(Status))
  {
    RtlFreeHeap(ProcessHeap, 0, OrigBootSector);
    return(Status);
  }

  Status = NtReadFile(FileHandle,
		      NULL,
		      NULL,
		      NULL,
		      &IoStatusBlock,
		      OrigBootSector,
		      SECTORSIZE,
		      NULL,
		      NULL);
  NtClose(FileHandle);
  if (!NT_SUCCESS(Status))
  {
    RtlFreeHeap(ProcessHeap, 0, OrigBootSector);
    return(Status);
  }


  /* Allocate buffer for new bootsector */
  NewBootSector = (PUCHAR)RtlAllocateHeap(ProcessHeap,
					  0,
					  SECTORSIZE);
  if (NewBootSector == NULL)
  {
    RtlFreeHeap(ProcessHeap, 0, OrigBootSector);
    return(STATUS_INSUFFICIENT_RESOURCES);
  }

  /* Read new bootsector from SrcPath */
  RtlInitUnicodeString(&Name,
		       SrcPath);

  InitializeObjectAttributes(&ObjectAttributes,
			     &Name,
			     OBJ_CASE_INSENSITIVE,
			     NULL,
			     NULL);

  Status = NtOpenFile(&FileHandle,
		      FILE_READ_ACCESS,
		      &ObjectAttributes,
		      &IoStatusBlock,
		      0,
		      FILE_SYNCHRONOUS_IO_NONALERT);
  if (!NT_SUCCESS(Status))
  {
    RtlFreeHeap(ProcessHeap, 0, OrigBootSector);
    RtlFreeHeap(ProcessHeap, 0, NewBootSector);
    return(Status);
  }

  Status = NtReadFile(FileHandle,
		      NULL,
		      NULL,
		      NULL,
		      &IoStatusBlock,
		      NewBootSector,
		      SECTORSIZE,
		      NULL,
		      NULL);
  NtClose(FileHandle);
  if (!NT_SUCCESS(Status))
  {
    RtlFreeHeap(ProcessHeap, 0, OrigBootSector);
    RtlFreeHeap(ProcessHeap, 0, NewBootSector);
    return(Status);
  }

  /* Copy partition table from old MBR to new */
  RtlCopyMemory ((NewBootSector + 446),
		 (OrigBootSector + 446),
		 4*16 /* Length of partition table */);

  /* Free the original boot sector */
  RtlFreeHeap(ProcessHeap, 0, OrigBootSector);

  /* Write new bootsector to RootPath */
  RtlInitUnicodeString(&Name,
		       RootPath);

  InitializeObjectAttributes(&ObjectAttributes,
			     &Name,
			     0,
			     NULL,
			     NULL);

  Status = NtCreateFile(&FileHandle,
			FILE_WRITE_ACCESS,
			&ObjectAttributes,
			&IoStatusBlock,
			NULL,
			FILE_ATTRIBUTE_NORMAL,
			0,
			FILE_OVERWRITE_IF,
			FILE_SYNCHRONOUS_IO_NONALERT | FILE_SEQUENTIAL_ONLY,
			NULL,
			0);
  if (!NT_SUCCESS(Status))
  {
    RtlFreeHeap(ProcessHeap, 0, NewBootSector);
    return(Status);
  }

  Status = NtWriteFile(FileHandle,
		       NULL,
		       NULL,
		       NULL,
		       &IoStatusBlock,
		       NewBootSector,
		       SECTORSIZE,
		       NULL,
		       NULL);
  NtClose(FileHandle);

  /* Free the new boot sector */
  RtlFreeHeap(ProcessHeap, 0, NewBootSector);

  return(Status);
}


NTSTATUS
InstallFat16BootCodeToDisk(PWSTR SrcPath,
			   PWSTR RootPath)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  IO_STATUS_BLOCK IoStatusBlock;
  UNICODE_STRING Name;
  HANDLE FileHandle;
  NTSTATUS Status;
  PUCHAR OrigBootSector;
  PUCHAR NewBootSector;

  /* Allocate buffer for original bootsector */
  OrigBootSector = (PUCHAR)RtlAllocateHeap(ProcessHeap,
					   0,
					   SECTORSIZE);
  if (OrigBootSector == NULL)
    return(STATUS_INSUFFICIENT_RESOURCES);

  /* Read current boot sector into buffer */
  RtlInitUnicodeString(&Name,
		       RootPath);

  InitializeObjectAttributes(&ObjectAttributes,
			     &Name,
			     OBJ_CASE_INSENSITIVE,
			     NULL,
			     NULL);

  Status = NtOpenFile(&FileHandle,
		      FILE_READ_ACCESS,
		      &ObjectAttributes,
		      &IoStatusBlock,
		      0,
		      FILE_SYNCHRONOUS_IO_NONALERT);
  if (!NT_SUCCESS(Status))
  {
    RtlFreeHeap(ProcessHeap, 0, OrigBootSector);
    return(Status);
  }

  Status = NtReadFile(FileHandle,
		      NULL,
		      NULL,
		      NULL,
		      &IoStatusBlock,
		      OrigBootSector,
		      SECTORSIZE,
		      NULL,
		      NULL);
  NtClose(FileHandle);
  if (!NT_SUCCESS(Status))
  {
    RtlFreeHeap(ProcessHeap, 0, OrigBootSector);
    return(Status);
  }


  /* Allocate buffer for new bootsector */
  NewBootSector = (PUCHAR)RtlAllocateHeap(ProcessHeap,
					  0,
					  SECTORSIZE);
  if (NewBootSector == NULL)
  {
    RtlFreeHeap(ProcessHeap, 0, OrigBootSector);
    return(STATUS_INSUFFICIENT_RESOURCES);
  }

  /* Read new bootsector from SrcPath */
  RtlInitUnicodeString(&Name,
		       SrcPath);

  InitializeObjectAttributes(&ObjectAttributes,
			     &Name,
			     OBJ_CASE_INSENSITIVE,
			     NULL,
			     NULL);

  Status = NtOpenFile(&FileHandle,
		      FILE_READ_ACCESS,
		      &ObjectAttributes,
		      &IoStatusBlock,
		      0,
		      FILE_SYNCHRONOUS_IO_NONALERT);
  if (!NT_SUCCESS(Status))
  {
    RtlFreeHeap(ProcessHeap, 0, OrigBootSector);
    RtlFreeHeap(ProcessHeap, 0, NewBootSector);
    return(Status);
  }

  Status = NtReadFile(FileHandle,
		      NULL,
		      NULL,
		      NULL,
		      &IoStatusBlock,
		      NewBootSector,
		      SECTORSIZE,
		      NULL,
		      NULL);
  NtClose(FileHandle);
  if (!NT_SUCCESS(Status))
  {
    RtlFreeHeap(ProcessHeap, 0, OrigBootSector);
    RtlFreeHeap(ProcessHeap, 0, NewBootSector);
    return(Status);
  }

  /* Adjust bootsector (copy a part of the FAT16 BPB) */
  memcpy((NewBootSector + 3),
	 (OrigBootSector + 3),
	 59);  /* FAT16 BPB length*/

  /* Free the original boot sector */
  RtlFreeHeap(ProcessHeap, 0, OrigBootSector);

  /* Write new bootsector to RootPath */
  RtlInitUnicodeString(&Name,
		       RootPath);

  InitializeObjectAttributes(&ObjectAttributes,
			     &Name,
			     0,
			     NULL,
			     NULL);

  Status = NtCreateFile(&FileHandle,
			FILE_WRITE_ACCESS,
			&ObjectAttributes,
			&IoStatusBlock,
			NULL,
			FILE_ATTRIBUTE_NORMAL,
			0,
			FILE_OVERWRITE_IF,
			FILE_SYNCHRONOUS_IO_NONALERT | FILE_SEQUENTIAL_ONLY,
			NULL,
			0);
  if (!NT_SUCCESS(Status))
  {
    RtlFreeHeap(ProcessHeap, 0, NewBootSector);
    return(Status);
  }

#if 0
  FilePosition.QuadPart = 0;
#endif
  Status = NtWriteFile(FileHandle,
		       NULL,
		       NULL,
		       NULL,
		       &IoStatusBlock,
		       NewBootSector,
		       SECTORSIZE,
		       NULL,
		       NULL);
  NtClose(FileHandle);

  /* Free the new boot sector */
  RtlFreeHeap(ProcessHeap, 0, NewBootSector);

  return(Status);
}


NTSTATUS
InstallFat32BootCodeToDisk(PWSTR SrcPath,
			   PWSTR RootPath)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  IO_STATUS_BLOCK IoStatusBlock;
  UNICODE_STRING Name;
  HANDLE FileHandle;
  NTSTATUS Status;
  PUCHAR OrigBootSector;
  PUCHAR NewBootSector;
  LARGE_INTEGER FileOffset;
  USHORT BackupBootSector;

  /* Allocate buffer for original bootsector */
  OrigBootSector = (PUCHAR)RtlAllocateHeap(ProcessHeap,
					   0,
					   SECTORSIZE);
  if (OrigBootSector == NULL)
    return(STATUS_INSUFFICIENT_RESOURCES);

  /* Read current boot sector into buffer */
  RtlInitUnicodeString(&Name,
		       RootPath);

  InitializeObjectAttributes(&ObjectAttributes,
			     &Name,
			     OBJ_CASE_INSENSITIVE,
			     NULL,
			     NULL);

  Status = NtOpenFile(&FileHandle,
		      FILE_READ_ACCESS,
		      &ObjectAttributes,
		      &IoStatusBlock,
		      0,
		      FILE_SYNCHRONOUS_IO_NONALERT);
  if (!NT_SUCCESS(Status))
  {
    RtlFreeHeap(ProcessHeap, 0, OrigBootSector);
    return(Status);
  }

  Status = NtReadFile(FileHandle,
		      NULL,
		      NULL,
		      NULL,
		      &IoStatusBlock,
		      OrigBootSector,
		      SECTORSIZE,
		      NULL,
		      NULL);
  NtClose(FileHandle);
  if (!NT_SUCCESS(Status))
  {
    RtlFreeHeap(ProcessHeap, 0, OrigBootSector);
    return(Status);
  }


  /* Allocate buffer for new bootsector (2 sectors) */
  NewBootSector = (PUCHAR)RtlAllocateHeap(ProcessHeap,
					  0,
					  2 * SECTORSIZE);
  if (NewBootSector == NULL)
  {
    RtlFreeHeap(ProcessHeap, 0, OrigBootSector);
    return(STATUS_INSUFFICIENT_RESOURCES);
  }

  /* Read new bootsector from SrcPath */
  RtlInitUnicodeString(&Name,
		       SrcPath);

  InitializeObjectAttributes(&ObjectAttributes,
			     &Name,
			     OBJ_CASE_INSENSITIVE,
			     NULL,
			     NULL);

  Status = NtOpenFile(&FileHandle,
		      FILE_READ_ACCESS,
		      &ObjectAttributes,
		      &IoStatusBlock,
		      0,
		      FILE_SYNCHRONOUS_IO_NONALERT);
  if (!NT_SUCCESS(Status))
  {
    RtlFreeHeap(ProcessHeap, 0, OrigBootSector);
    RtlFreeHeap(ProcessHeap, 0, NewBootSector);
    return(Status);
  }

  Status = NtReadFile(FileHandle,
		      NULL,
		      NULL,
		      NULL,
		      &IoStatusBlock,
		      NewBootSector,
		      2 * SECTORSIZE,
		      NULL,
		      NULL);
  NtClose(FileHandle);
  if (!NT_SUCCESS(Status))
  {
    RtlFreeHeap(ProcessHeap, 0, OrigBootSector);
    RtlFreeHeap(ProcessHeap, 0, NewBootSector);
    return(Status);
  }

  /* Adjust bootsector (copy a part of the FAT32 BPB) */
  memcpy((NewBootSector + 3),
	 (OrigBootSector + 3),
	 87); /* FAT32 BPB length */

  /* Get the location of the backup boot sector */
  BackupBootSector = (OrigBootSector[0x33] << 8) + OrigBootSector[0x32];

  /* Free the original boot sector */
  RtlFreeHeap(ProcessHeap, 0, OrigBootSector);

  /* Write the first sector of the new bootcode to DstPath */
  RtlInitUnicodeString(&Name,
		       RootPath);

  InitializeObjectAttributes(&ObjectAttributes,
			     &Name,
			     0,
			     NULL,
			     NULL);

  Status = NtOpenFile(&FileHandle,
		      FILE_WRITE_ACCESS | FILE_WRITE_ATTRIBUTES,
		      &ObjectAttributes,
		      &IoStatusBlock,
		      0,
		      FILE_SYNCHRONOUS_IO_NONALERT | FILE_SEQUENTIAL_ONLY);
  if (!NT_SUCCESS(Status))
  {
    DPRINT1("NtOpenFile() failed (Status %lx)\n", Status);
    RtlFreeHeap(ProcessHeap, 0, NewBootSector);
    return(Status);
  }

  /* Write sector 0 */
  FileOffset.QuadPart = 0ULL;
  Status = NtWriteFile(FileHandle,
		       NULL,
		       NULL,
		       NULL,
		       &IoStatusBlock,
		       NewBootSector,
		       SECTORSIZE,
		       &FileOffset,
		       NULL);
  if (!NT_SUCCESS(Status))
  {
    DPRINT1("NtWriteFile() failed (Status %lx)\n", Status);
    NtClose(FileHandle);
    RtlFreeHeap(ProcessHeap, 0, NewBootSector);
    return(Status);
  }

  /* Write backup boot sector */
  if ((BackupBootSector != 0x0000) && (BackupBootSector != 0xFFFF))
  {
    FileOffset.QuadPart = (ULONGLONG)((ULONG)BackupBootSector * SECTORSIZE);
    Status = NtWriteFile(FileHandle,
			 NULL,
			 NULL,
			 NULL,
			 &IoStatusBlock,
			 NewBootSector,
			 SECTORSIZE,
			 &FileOffset,
			 NULL);
    if (!NT_SUCCESS(Status))
    {
      DPRINT1("NtWriteFile() failed (Status %lx)\n", Status);
      NtClose(FileHandle);
      RtlFreeHeap(ProcessHeap, 0, NewBootSector);
      return(Status);
    }
  }

  /* Write sector 14 */
  FileOffset.QuadPart = (ULONGLONG)(14 * SECTORSIZE);
  Status = NtWriteFile(FileHandle,
		       NULL,
		       NULL,
		       NULL,
		       &IoStatusBlock,
		       (NewBootSector + SECTORSIZE),
		       SECTORSIZE,
		       &FileOffset,
		       NULL);
  if (!NT_SUCCESS(Status))
  {
    DPRINT1("NtWriteFile() failed (Status %lx)\n", Status);
  }
  NtClose(FileHandle);

  /* Free the new boot sector */
  RtlFreeHeap(ProcessHeap, 0, NewBootSector);

  return(Status);
}


static NTSTATUS
UnprotectBootIni(PWSTR FileName,
		 PULONG Attributes)
{
  UNICODE_STRING Name;
  OBJECT_ATTRIBUTES ObjectAttributes;
  IO_STATUS_BLOCK IoStatusBlock;
  FILE_BASIC_INFORMATION FileInfo;
  HANDLE FileHandle;
  NTSTATUS Status;

  RtlInitUnicodeString(&Name,
		       FileName);

  InitializeObjectAttributes(&ObjectAttributes,
			     &Name,
			     OBJ_CASE_INSENSITIVE,
			     NULL,
			     NULL);

  Status = NtOpenFile(&FileHandle,
		      FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES,
		      &ObjectAttributes,
		      &IoStatusBlock,
		      0,
		      FILE_SYNCHRONOUS_IO_NONALERT);
  if (Status == STATUS_NO_SUCH_FILE)
  {
    DPRINT1("NtOpenFile() failed (Status %lx)\n", Status);
    *Attributes = 0;
    return(STATUS_SUCCESS);
  }
  if (!NT_SUCCESS(Status))
  {
    DPRINT1("NtOpenFile() failed (Status %lx)\n", Status);
    return(Status);
  }

  Status = NtQueryInformationFile(FileHandle,
				  &IoStatusBlock,
				  &FileInfo,
				  sizeof(FILE_BASIC_INFORMATION),
				  FileBasicInformation);
  if (!NT_SUCCESS(Status))
  {
    DPRINT1("NtQueryInformationFile() failed (Status %lx)\n", Status);
    NtClose(FileHandle);
    return(Status);
  }

  *Attributes = FileInfo.FileAttributes;

  /* Delete attributes SYSTEM, HIDDEN and READONLY */
  FileInfo.FileAttributes = FileInfo.FileAttributes &
    ~(FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_READONLY);

  Status = NtSetInformationFile(FileHandle,
				&IoStatusBlock,
				&FileInfo,
				sizeof(FILE_BASIC_INFORMATION),
				FileBasicInformation);
  if (!NT_SUCCESS(Status))
  {
    DPRINT1("NtSetInformationFile() failed (Status %lx)\n", Status);
  }

  NtClose(FileHandle);
  return(Status);
}


static NTSTATUS
ProtectBootIni(PWSTR FileName,
	       ULONG Attributes)
{
  UNICODE_STRING Name;
  OBJECT_ATTRIBUTES ObjectAttributes;
  IO_STATUS_BLOCK IoStatusBlock;
  FILE_BASIC_INFORMATION FileInfo;
  HANDLE FileHandle;
  NTSTATUS Status;

  RtlInitUnicodeString(&Name,
		       FileName);

  InitializeObjectAttributes(&ObjectAttributes,
			     &Name,
			     OBJ_CASE_INSENSITIVE,
			     NULL,
			     NULL);

  Status = NtOpenFile(&FileHandle,
		      FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES,
		      &ObjectAttributes,
		      &IoStatusBlock,
		      0,
		      FILE_SYNCHRONOUS_IO_NONALERT);
  if (!NT_SUCCESS(Status))
  {
    DPRINT1("NtOpenFile() failed (Status %lx)\n", Status);
    return(Status);
  }

  Status = NtQueryInformationFile(FileHandle,
				  &IoStatusBlock,
				  &FileInfo,
				  sizeof(FILE_BASIC_INFORMATION),
				  FileBasicInformation);
  if (!NT_SUCCESS(Status))
  {
    DPRINT1("NtQueryInformationFile() failed (Status %lx)\n", Status);
    NtClose(FileHandle);
    return(Status);
  }

  FileInfo.FileAttributes = FileInfo.FileAttributes | Attributes;

  Status = NtSetInformationFile(FileHandle,
				&IoStatusBlock,
				&FileInfo,
				sizeof(FILE_BASIC_INFORMATION),
				FileBasicInformation);
  if (!NT_SUCCESS(Status))
  {
    DPRINT1("NtSetInformationFile() failed (Status %lx)\n", Status);
  }

  NtClose(FileHandle);
  return(Status);
}


NTSTATUS
UpdateBootIni(PWSTR BootIniPath,
	      PWSTR EntryName,
	      PWSTR EntryValue)
{
  UNICODE_STRING Name;
  PINICACHE Cache = NULL;
  PINICACHESECTION Section = NULL;
  NTSTATUS Status;
  ULONG FileAttribute;

  RtlInitUnicodeString(&Name,
		       BootIniPath);

  Status = IniCacheLoad(&Cache,
			&Name,
			FALSE);
  if (!NT_SUCCESS(Status))
  {
    return(Status);
  }

  Section = IniCacheGetSection(Cache,
			       L"operating systems");
  if (Section == NULL)
  {
    IniCacheDestroy(Cache);
    return(STATUS_UNSUCCESSFUL);
  }

  IniCacheInsertKey(Section,
		    NULL,
		    INSERT_LAST,
		    EntryName,
		    EntryValue);

  Status = UnprotectBootIni(BootIniPath,
			    &FileAttribute);
  if (!NT_SUCCESS(Status))
  {
    IniCacheDestroy(Cache);
    return(Status);
  }

  Status = IniCacheSave(Cache,
			BootIniPath);
  if (!NT_SUCCESS(Status))
  {
    IniCacheDestroy(Cache);
    return(Status);
  }

  FileAttribute |= (FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_READONLY);
  Status = ProtectBootIni(BootIniPath,
			  FileAttribute);

  IniCacheDestroy(Cache);

  return(Status);
}


BOOLEAN
CheckInstallFatBootcodeToPartition(PUNICODE_STRING SystemRootPath)
{
  if (DoesFileExist(SystemRootPath->Buffer, L"ntldr") ||
      DoesFileExist(SystemRootPath->Buffer, L"boot.ini"))
    {
      return TRUE;
    }
  else if (DoesFileExist(SystemRootPath->Buffer, L"io.sys") ||
	   DoesFileExist(SystemRootPath->Buffer, L"msdos.sys"))
    {
      return TRUE;
    }

  return FALSE;
}


NTSTATUS
InstallFatBootcodeToPartition(PUNICODE_STRING SystemRootPath,
			      PUNICODE_STRING SourceRootPath,
			      PUNICODE_STRING DestinationArcPath,
			      UCHAR PartitionType)
{
  WCHAR SrcPath[MAX_PATH];
  WCHAR DstPath[MAX_PATH];
  NTSTATUS Status;

  /* FAT or FAT32 partition */
  DPRINT1("System path: '%wZ'\n", SystemRootPath);

  if (DoesFileExist(SystemRootPath->Buffer, L"ntldr") == TRUE ||
      DoesFileExist(SystemRootPath->Buffer, L"boot.ini") == TRUE)
    {
      /* Search root directory for 'ntldr' and 'boot.ini'. */
      DPRINT("Found Microsoft Windows NT/2000/XP boot loader\n");

      /* Copy FreeLoader to the boot partition */
      wcscpy(SrcPath, SourceRootPath->Buffer);
      wcscat(SrcPath, L"\\loader\\freeldr.sys");
      wcscpy(DstPath, SystemRootPath->Buffer);
      wcscat(DstPath, L"\\freeldr.sys");

      DPRINT("Copy: %S ==> %S\n", SrcPath, DstPath);
      Status = SetupCopyFile(SrcPath, DstPath);
      if (!NT_SUCCESS(Status))
      {
	DPRINT1("SetupCopyFile() failed (Status %lx)\n", Status);
	return Status;
      }

      /* Create or update freeldr.ini */
      if (DoesFileExist(SystemRootPath->Buffer, L"freeldr.ini") == FALSE)
      {
	/* Create new 'freeldr.ini' */
	DPRINT1("Create new 'freeldr.ini'\n");
	wcscpy(DstPath, SystemRootPath->Buffer);
	wcscat(DstPath, L"\\freeldr.ini");

	Status = CreateFreeLoaderIniForReactos(DstPath,
					       DestinationArcPath->Buffer);
	if (!NT_SUCCESS(Status))
	{
	  DPRINT1("CreateFreeLoaderIniForReactos() failed (Status %lx)\n", Status);
	  return Status;
	}

	/* Install new bootcode */
	if (PartitionType == PARTITION_FAT32 ||
	    PartitionType == PARTITION_FAT32_XINT13)
	{
	  /* Install FAT32 bootcode */
	  wcscpy(SrcPath, SourceRootPath->Buffer);
	  wcscat(SrcPath, L"\\loader\\fat32.bin");
	  wcscpy(DstPath, SystemRootPath->Buffer);
	  wcscat(DstPath, L"\\bootsect.ros");

	  DPRINT1("Install FAT32 bootcode: %S ==> %S\n", SrcPath, DstPath);
	  Status = InstallFat32BootCodeToFile(SrcPath,
					      DstPath,
					      SystemRootPath->Buffer);
	  if (!NT_SUCCESS(Status))
	  {
	    DPRINT1("InstallFat32BootCodeToFile() failed (Status %lx)\n", Status);
	    return Status;
	  }
	}
	else
	{
	  /* Install FAT16 bootcode */
	  wcscpy(SrcPath, SourceRootPath->Buffer);
	  wcscat(SrcPath, L"\\loader\\fat.bin");
	  wcscpy(DstPath, SystemRootPath->Buffer);
	  wcscat(DstPath, L"\\bootsect.ros");

	  DPRINT1("Install FAT bootcode: %S ==> %S\n", SrcPath, DstPath);
	  Status = InstallFat16BootCodeToFile(SrcPath,
					      DstPath,
					      SystemRootPath->Buffer);
	  if (!NT_SUCCESS(Status))
	  {
	    DPRINT1("InstallFat16BootCodeToFile() failed (Status %lx)\n", Status);
	    return Status;
	  }
	}

	/* Update 'boot.ini' */
	wcscpy(DstPath, SystemRootPath->Buffer);
	wcscat(DstPath, L"\\boot.ini");

	DPRINT1("Update 'boot.ini': %S\n", DstPath);
	Status = UpdateBootIni(DstPath,
			       L"C:\\bootsect.ros",
			       L"\"ReactOS\"");
	if (!NT_SUCCESS(Status))
	{
	  DPRINT1("UpdateBootIni() failed (Status %lx)\n", Status);
	  return Status;
	}
      }
      else
      {
	/* Update existing 'freeldr.ini' */
	DPRINT1("Update existing 'freeldr.ini'\n");
	wcscpy(DstPath, SystemRootPath->Buffer);
	wcscat(DstPath, L"\\freeldr.ini");

	Status = UpdateFreeLoaderIni(DstPath,
				     DestinationArcPath->Buffer);
	if (!NT_SUCCESS(Status))
	{
	  DPRINT1("UpdateFreeLoaderIni() failed (Status %lx)\n", Status);
	  return Status;
	}
      }
    }
    else if (DoesFileExist(SystemRootPath->Buffer, L"io.sys") == TRUE ||
	     DoesFileExist(SystemRootPath->Buffer, L"msdos.sys") == TRUE)
    {
      /* Search for root directory for 'io.sys' and 'msdos.sys'. */
      DPRINT1("Found Microsoft DOS or Windows 9x boot loader\n");

      /* Copy FreeLoader to the boot partition */
      wcscpy(SrcPath, SourceRootPath->Buffer);
      wcscat(SrcPath, L"\\loader\\freeldr.sys");
      wcscpy(DstPath, SystemRootPath->Buffer);
      wcscat(DstPath, L"\\freeldr.sys");

      DPRINT("Copy: %S ==> %S\n", SrcPath, DstPath);
      Status = SetupCopyFile(SrcPath, DstPath);
      if (!NT_SUCCESS(Status))
      {
	DPRINT1("SetupCopyFile() failed (Status %lx)\n", Status);
	return Status;
      }

      /* Create or update 'freeldr.ini' */
      if (DoesFileExist(SystemRootPath->Buffer, L"freeldr.ini") == FALSE)
      {
	/* Create new 'freeldr.ini' */
	DPRINT1("Create new 'freeldr.ini'\n");
	wcscpy(DstPath, SystemRootPath->Buffer);
	wcscat(DstPath, L"\\freeldr.ini");

	Status = CreateFreeLoaderIniForDos(DstPath,
					   DestinationArcPath->Buffer);
	if (!NT_SUCCESS(Status))
	{
	  DPRINT1("CreateFreeLoaderIniForDos() failed (Status %lx)\n", Status);
	  return Status;
	}

	/* Save current bootsector as 'BOOTSECT.DOS' */
	wcscpy(SrcPath, SystemRootPath->Buffer);
	wcscpy(DstPath, SystemRootPath->Buffer);
	wcscat(DstPath, L"\\bootsect.dos");

	DPRINT1("Save bootsector: %S ==> %S\n", SrcPath, DstPath);
	Status = SaveCurrentBootSector(SrcPath,
				       DstPath);
	if (!NT_SUCCESS(Status))
	{
	  DPRINT1("SaveCurrentBootSector() failed (Status %lx)\n", Status);
	  return Status;
	}

	/* Install new bootsector */
	if (PartitionType == PARTITION_FAT32 ||
	    PartitionType == PARTITION_FAT32_XINT13)
	{
	  wcscpy(SrcPath, SourceRootPath->Buffer);
	  wcscat(SrcPath, L"\\loader\\fat32.bin");

	  DPRINT1("Install FAT32 bootcode: %S ==> %S\n", SrcPath, SystemRootPath->Buffer);
	  Status = InstallFat32BootCodeToDisk(SrcPath,
					      SystemRootPath->Buffer);
	  if (!NT_SUCCESS(Status))
	  {
	    DPRINT1("InstallFat32BootCodeToDisk() failed (Status %lx)\n", Status);
	    return Status;
	  }
	}
	else
	{
	  wcscpy(SrcPath, SourceRootPath->Buffer);
	  wcscat(SrcPath, L"\\loader\\fat.bin");

	  DPRINT1("Install FAT bootcode: %S ==> %S\n", SrcPath, SystemRootPath->Buffer);
	  Status = InstallFat16BootCodeToDisk(SrcPath,
					      SystemRootPath->Buffer);
	  if (!NT_SUCCESS(Status))
	  {
	    DPRINT1("InstallFat16BootCodeToDisk() failed (Status %lx)\n", Status);
	    return Status;
	  }
	}
      }
      else
      {
	/* Update existing 'freeldr.ini' */
	wcscpy(DstPath, SystemRootPath->Buffer);
	wcscat(DstPath, L"\\freeldr.ini");

	Status = UpdateFreeLoaderIni(DstPath,
				     DestinationArcPath->Buffer);
	if (!NT_SUCCESS(Status))
	{
	  DPRINT1("UpdateFreeLoaderIni() failed (Status %lx)\n", Status);
	  return Status;
	}
      }
    }
    else
    {
      /* No or unknown boot loader */
      DPRINT1("No or unknown boot loader found\n");

      /* Copy FreeLoader to the boot partition */
      wcscpy(SrcPath, SourceRootPath->Buffer);
      wcscat(SrcPath, L"\\loader\\freeldr.sys");
      wcscpy(DstPath, SystemRootPath->Buffer);
      wcscat(DstPath, L"\\freeldr.sys");

      DPRINT1("Copy: %S ==> %S\n", SrcPath, DstPath);
      Status = SetupCopyFile(SrcPath, DstPath);
      if (!NT_SUCCESS(Status))
      {
	DPRINT1("SetupCopyFile() failed (Status %lx)\n", Status);
	return Status;
      }

      /* Create or update 'freeldr.ini' */
      if (DoesFileExist(SystemRootPath->Buffer, L"freeldr.ini") == FALSE)
      {
	/* Create new freeldr.ini */
	wcscpy(DstPath, SystemRootPath->Buffer);
	wcscat(DstPath, L"\\freeldr.ini");

	DPRINT1("Copy: %S ==> %S\n", SrcPath, DstPath);
	Status = CreateFreeLoaderIniForReactos(DstPath,
					       DestinationArcPath->Buffer);
	if (!NT_SUCCESS(Status))
	{
	  DPRINT1("CreateFreeLoaderIniForReactos() failed (Status %lx)\n", Status);
	  return Status;
	}

	/* Save current bootsector as 'BOOTSECT.OLD' */
	wcscpy(SrcPath, SystemRootPath->Buffer);
	wcscpy(DstPath, SystemRootPath->Buffer);
	wcscat(DstPath, L"\\bootsect.old");

	DPRINT1("Save bootsector: %S ==> %S\n", SrcPath, DstPath);
	Status = SaveCurrentBootSector(SrcPath,
				       DstPath);
	if (!NT_SUCCESS(Status))
	{
	  DPRINT1("SaveCurrentBootSector() failed (Status %lx)\n", Status);
	  return Status;
	}

	/* Install new bootsector */
	if (PartitionType == PARTITION_FAT32 ||
	    PartitionType == PARTITION_FAT32_XINT13)
	{
	  wcscpy(SrcPath, SourceRootPath->Buffer);
	  wcscat(SrcPath, L"\\loader\\fat32.bin");

	  DPRINT("Install FAT32 bootcode: %S ==> %S\n", SrcPath, SystemRootPath->Buffer);
	  Status = InstallFat32BootCodeToDisk(SrcPath,
					      SystemRootPath->Buffer);
	  if (!NT_SUCCESS(Status))
	  {
	    DPRINT1("InstallFat32BootCodeToDisk() failed (Status %lx)\n", Status);
	    return Status;
	  }
	}
	else
	{
	  wcscpy(SrcPath, SourceRootPath->Buffer);
	  wcscat(SrcPath, L"\\loader\\fat.bin");

	  DPRINT("Install FAT bootcode: %S ==> %S\n", SrcPath, SystemRootPath->Buffer);
	  Status = InstallFat16BootCodeToDisk(SrcPath,
					      SystemRootPath->Buffer);
	  if (!NT_SUCCESS(Status))
	  {
	    DPRINT1("InstallFat16BootCodeToDisk() failed (Status %lx)\n", Status);
	    return Status;
	  }
	}
      }
      else
      {
	/* Update existing 'freeldr.ini' */
	wcscpy(DstPath, SystemRootPath->Buffer);
	wcscat(DstPath, L"\\freeldr.ini");

	Status = UpdateFreeLoaderIni(DstPath,
				     DestinationArcPath->Buffer);
	if (!NT_SUCCESS(Status))
	{
	  DPRINT1("UpdateFreeLoaderIni() failed (Status %lx)\n", Status);
	  return Status;
	}
      }
    }

  return STATUS_SUCCESS;
}


NTSTATUS
InstallFatBootcodeToFloppy(PUNICODE_STRING SourceRootPath,
			   PUNICODE_STRING DestinationArcPath)
{
  WCHAR SrcPath[MAX_PATH];
  WCHAR DstPath[MAX_PATH];
  NTSTATUS Status;

  /* Copy FreeLoader to the boot partition */
  wcscpy(SrcPath, SourceRootPath->Buffer);
  wcscat(SrcPath, L"\\loader\\freeldr.sys");

  wcscat(DstPath, L"\\Device\\Floppy0\\freeldr.sys");

  DPRINT("Copy: %S ==> %S\n", SrcPath, DstPath);
  Status = SetupCopyFile(SrcPath, DstPath);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("SetupCopyFile() failed (Status %lx)\n", Status);
      return Status;
    }

  /* Create new 'freeldr.ini' */
  wcscat(DstPath, L"\\Device\\Floppy0\\freeldr.ini");

  DPRINT("Create new 'freeldr.ini'\n");
  Status = CreateFreeLoaderIniForReactos(DstPath,
					 DestinationArcPath->Buffer);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("CreateFreeLoaderIniForReactos() failed (Status %lx)\n", Status);
      return Status;
    }

  /* Install FAT12/16 boosector */
  wcscpy(SrcPath, SourceRootPath->Buffer);
  wcscat(SrcPath, L"\\loader\\fat.bin");

  wcscat(DstPath, L"\\Device\\Floppy0");

  DPRINT("Install FAT bootcode: %S ==> %S\n", SrcPath, DstPath);
  Status = InstallFat16BootCodeToDisk(SrcPath,
				      DstPath);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("InstallFat16BootCodeToDisk() failed (Status %lx)\n", Status);
      return Status;
    }

  return STATUS_SUCCESS;
}

/* EOF */
