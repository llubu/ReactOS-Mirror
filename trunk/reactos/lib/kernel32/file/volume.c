/* $Id: volume.c,v 1.41 2004/08/24 17:15:42 navaraf Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/volume.c
 * PURPOSE:         File volume functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 *                  Erik Bos, Alexandre Julliard :
 *                      GetLogicalDriveStringsA,
 *                      GetLogicalDriveStringsW, GetLogicalDrives
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */
//WINE copyright notice:
/*
 * DOS drives handling functions 
 *
 * Copyright 1993 Erik Bos
 * Copyright 1996 Alexandre Julliard
 */

#include <k32.h>

#define NDEBUG
#include "../include/debug.h"


#define MAX_DOS_DRIVES 26


static HANDLE
InternalOpenDirW(LPCWSTR DirName,
		 BOOLEAN Write)
{
  UNICODE_STRING NtPathU;
  OBJECT_ATTRIBUTES ObjectAttributes;
  NTSTATUS errCode;
  IO_STATUS_BLOCK IoStatusBlock;
  HANDLE hFile;

  if (!RtlDosPathNameToNtPathName_U((LPWSTR)DirName,
				    &NtPathU,
				    NULL,
				    NULL))
    {
	DPRINT("Invalid path\n");
	SetLastError(ERROR_BAD_PATHNAME);
	return INVALID_HANDLE_VALUE;
    }

    InitializeObjectAttributes(&ObjectAttributes,
	                       &NtPathU,
			       Write ? FILE_WRITE_ATTRIBUTES : FILE_READ_ATTRIBUTES,
			       NULL,
			       NULL);

    errCode = NtCreateFile (&hFile,
	                    Write ? FILE_GENERIC_WRITE : FILE_GENERIC_READ,
			    &ObjectAttributes,
			    &IoStatusBlock,
			    NULL,
			    0,
			    FILE_SHARE_READ|FILE_SHARE_WRITE,
			    FILE_OPEN,
			    0,
			    NULL,
			    0);

    RtlFreeUnicodeString(&NtPathU);

    if (!NT_SUCCESS(errCode))
    {
	SetLastErrorByStatus (errCode);
	return INVALID_HANDLE_VALUE;
    }
    return hFile;
}


/*
 * @implemented
 */
DWORD STDCALL
GetLogicalDriveStringsA(DWORD nBufferLength,
			LPSTR lpBuffer)
{
   DWORD drive, count;
   DWORD dwDriveMap;

   dwDriveMap = SharedUserData->DosDeviceMap;

   for (drive = count = 0; drive < MAX_DOS_DRIVES; drive++)
     {
	if (dwDriveMap & (1<<drive))
	   count++;
     }


   if (count * 4 * sizeof(char) <= nBufferLength)
     {
	LPSTR p = lpBuffer;

	for (drive = 0; drive < MAX_DOS_DRIVES; drive++)
	  if (dwDriveMap & (1<<drive))
	  {
	     *p++ = 'A' + drive;
	     *p++ = ':';
	     *p++ = '\\';
	     *p++ = '\0';
	  }
	*p = '\0';
     }
    return (count * 4 * sizeof(char));
}


/*
 * @implemented
 */
DWORD STDCALL
GetLogicalDriveStringsW(DWORD nBufferLength,
			LPWSTR lpBuffer)
{
   DWORD drive, count;
   DWORD dwDriveMap;

   dwDriveMap = SharedUserData->DosDeviceMap;

   for (drive = count = 0; drive < MAX_DOS_DRIVES; drive++)
     {
	if (dwDriveMap & (1<<drive))
	   count++;
     }

    if (count * 4 * sizeof(WCHAR) <=  nBufferLength)
    {
        LPWSTR p = lpBuffer;
        for (drive = 0; drive < MAX_DOS_DRIVES; drive++)
            if (dwDriveMap & (1<<drive))
            {
                *p++ = (WCHAR)('A' + drive);
                *p++ = (WCHAR)':';
                *p++ = (WCHAR)'\\';
                *p++ = (WCHAR)'\0';
            }
        *p = (WCHAR)'\0';
    }
    return (count * 4 * sizeof(WCHAR));
}


/*
 * @implemented
 */
DWORD STDCALL
GetLogicalDrives(VOID)
{
  return(SharedUserData->DosDeviceMap);
}


/*
 * @implemented
 */
BOOL STDCALL
GetDiskFreeSpaceA (
	LPCSTR	lpRootPathName,
	LPDWORD	lpSectorsPerCluster,
	LPDWORD	lpBytesPerSector,
	LPDWORD	lpNumberOfFreeClusters,
	LPDWORD	lpTotalNumberOfClusters
	)
{
	UNICODE_STRING RootPathNameU;
	ANSI_STRING RootPathName;
	BOOL Result;

	RtlInitAnsiString (&RootPathName,
	                   (LPSTR)lpRootPathName);

	RtlInitUnicodeString (&RootPathNameU,
	                      NULL);

	if (lpRootPathName)
	{
		/* convert ansi (or oem) string to unicode */
		if (bIsFileApiAnsi)
			RtlAnsiStringToUnicodeString (&RootPathNameU,
			                              &RootPathName,
			                              TRUE);
		else
			RtlOemStringToUnicodeString (&RootPathNameU,
			                             &RootPathName,
			                             TRUE);
	}

	Result = GetDiskFreeSpaceW (RootPathNameU.Buffer,
	                            lpSectorsPerCluster,
	                            lpBytesPerSector,
	                            lpNumberOfFreeClusters,
	                            lpTotalNumberOfClusters);

	if (lpRootPathName)
	{
		RtlFreeHeap (RtlGetProcessHeap (),
		             0,
		             RootPathNameU.Buffer);
	}

	return Result;
}


/*
 * @implemented
 */
BOOL STDCALL
GetDiskFreeSpaceW(
    LPCWSTR lpRootPathName,
    LPDWORD lpSectorsPerCluster,
    LPDWORD lpBytesPerSector,
    LPDWORD lpNumberOfFreeClusters,
    LPDWORD lpTotalNumberOfClusters
    )
{
    FILE_FS_SIZE_INFORMATION FileFsSize;
    IO_STATUS_BLOCK IoStatusBlock;
    WCHAR RootPathName[MAX_PATH];
    HANDLE hFile;
    NTSTATUS errCode;

    if (lpRootPathName)
    {
        wcsncpy (RootPathName, lpRootPathName, 3);
    }
    else
    {
        GetCurrentDirectoryW (MAX_PATH, RootPathName);
    }
    RootPathName[3] = 0;

  hFile = InternalOpenDirW(RootPathName, FALSE);
  if (INVALID_HANDLE_VALUE == hFile)
    {
      SetLastError(ERROR_PATH_NOT_FOUND);
      return FALSE;
    }

    errCode = NtQueryVolumeInformationFile(hFile,
                                           &IoStatusBlock,
                                           &FileFsSize,
                                           sizeof(FILE_FS_SIZE_INFORMATION),
                                           FileFsSizeInformation);
    if (!NT_SUCCESS(errCode))
    {
        CloseHandle(hFile);
        SetLastErrorByStatus (errCode);
        return FALSE;
    }

    *lpBytesPerSector = FileFsSize.BytesPerSector;
    *lpSectorsPerCluster = FileFsSize.SectorsPerAllocationUnit;
    *lpNumberOfFreeClusters = FileFsSize.AvailableAllocationUnits.u.LowPart;
    *lpTotalNumberOfClusters = FileFsSize.TotalAllocationUnits.u.LowPart;
    CloseHandle(hFile);

    return TRUE;
}


/*
 * @implemented
 */
BOOL STDCALL
GetDiskFreeSpaceExA (
	LPCSTR		lpDirectoryName,
	PULARGE_INTEGER	lpFreeBytesAvailableToCaller,
	PULARGE_INTEGER	lpTotalNumberOfBytes,
	PULARGE_INTEGER	lpTotalNumberOfFreeBytes
	)
{
	UNICODE_STRING DirectoryNameU;
	ANSI_STRING DirectoryName;
	BOOL Result;

	RtlInitAnsiString (&DirectoryName,
	                   (LPSTR)lpDirectoryName);

	RtlInitUnicodeString (&DirectoryNameU,
	                      NULL);

	if (lpDirectoryName)
	{
		/* convert ansi (or oem) string to unicode */
		if (bIsFileApiAnsi)
			RtlAnsiStringToUnicodeString (&DirectoryNameU,
			                              &DirectoryName,
			                              TRUE);
		else
			RtlOemStringToUnicodeString (&DirectoryNameU,
			                             &DirectoryName,
			                             TRUE);
	}

	Result = GetDiskFreeSpaceExW (DirectoryNameU.Buffer,
	                              lpFreeBytesAvailableToCaller,
	                              lpTotalNumberOfBytes,
	                              lpTotalNumberOfFreeBytes);

	if (lpDirectoryName)
	{
		RtlFreeHeap (RtlGetProcessHeap (),
		             0,
		             DirectoryNameU.Buffer);
	}

	return Result;
}


/*
 * @implemented
 */
BOOL STDCALL
GetDiskFreeSpaceExW(
    LPCWSTR lpDirectoryName,
    PULARGE_INTEGER lpFreeBytesAvailableToCaller,
    PULARGE_INTEGER lpTotalNumberOfBytes,
    PULARGE_INTEGER lpTotalNumberOfFreeBytes
    )
{
    FILE_FS_SIZE_INFORMATION FileFsSize;
    IO_STATUS_BLOCK IoStatusBlock;
    ULARGE_INTEGER BytesPerCluster;
    WCHAR RootPathName[MAX_PATH];
    HANDLE hFile;
    NTSTATUS errCode;

    if (lpDirectoryName)
    {
        wcsncpy (RootPathName, lpDirectoryName, 3);
    }
    else
    {
        GetCurrentDirectoryW (MAX_PATH, RootPathName);
    }
    RootPathName[3] = 0;

    hFile = InternalOpenDirW(RootPathName, FALSE);
    if (INVALID_HANDLE_VALUE == hFile)
    {
        return FALSE;
    }
   
    errCode = NtQueryVolumeInformationFile(hFile,
                                           &IoStatusBlock,
                                           &FileFsSize,
                                           sizeof(FILE_FS_SIZE_INFORMATION),
                                           FileFsSizeInformation);
    if (!NT_SUCCESS(errCode))
    {
        CloseHandle(hFile);
        SetLastErrorByStatus (errCode);
        return FALSE;
    }

    BytesPerCluster.QuadPart =
        FileFsSize.BytesPerSector * FileFsSize.SectorsPerAllocationUnit;

    // FIXME: Use quota information
	if (lpFreeBytesAvailableToCaller)
        lpFreeBytesAvailableToCaller->QuadPart =
            BytesPerCluster.QuadPart * FileFsSize.AvailableAllocationUnits.QuadPart;
	
	if (lpTotalNumberOfBytes)
        lpTotalNumberOfBytes->QuadPart =
            BytesPerCluster.QuadPart * FileFsSize.TotalAllocationUnits.QuadPart;
	if (lpTotalNumberOfFreeBytes)
        lpTotalNumberOfFreeBytes->QuadPart =
            BytesPerCluster.QuadPart * FileFsSize.AvailableAllocationUnits.QuadPart;

    CloseHandle(hFile);

    return TRUE;
}


/*
 * @implemented
 */
UINT STDCALL
GetDriveTypeA(LPCSTR lpRootPathName)
{
	UNICODE_STRING RootPathNameU;
	ANSI_STRING RootPathName;
	UINT Result;

	RtlInitAnsiString (&RootPathName,
	                   (LPSTR)lpRootPathName);

	/* convert ansi (or oem) string to unicode */
	if (bIsFileApiAnsi)
		RtlAnsiStringToUnicodeString (&RootPathNameU,
		                              &RootPathName,
		                              TRUE);
	else
		RtlOemStringToUnicodeString (&RootPathNameU,
		                             &RootPathName,
		                             TRUE);

	Result = GetDriveTypeW (RootPathNameU.Buffer);

	RtlFreeHeap (RtlGetProcessHeap (),
	             0,
	             RootPathNameU.Buffer);

	return Result;
}


/*
 * @implemented
 */
UINT STDCALL
GetDriveTypeW(LPCWSTR lpRootPathName)
{
	FILE_FS_DEVICE_INFORMATION FileFsDevice;
	IO_STATUS_BLOCK IoStatusBlock;

	HANDLE hFile;
	NTSTATUS errCode;

	hFile = InternalOpenDirW(lpRootPathName, FALSE);
	if (hFile == INVALID_HANDLE_VALUE)
	{
	    return DRIVE_NO_ROOT_DIR;	/* According to WINE regression tests */
	}

	errCode = NtQueryVolumeInformationFile (hFile,
	                                        &IoStatusBlock,
	                                        &FileFsDevice,
	                                        sizeof(FILE_FS_DEVICE_INFORMATION),
	                                        FileFsDeviceInformation);
	if (!NT_SUCCESS(errCode))
	{
		CloseHandle(hFile);
		SetLastErrorByStatus (errCode);
		return 0;	
	}
	CloseHandle(hFile);

        switch (FileFsDevice.DeviceType)
        {
		case FILE_DEVICE_CD_ROM_FILE_SYSTEM:
			return DRIVE_CDROM;
	        case FILE_DEVICE_VIRTUAL_DISK:
	        	return DRIVE_RAMDISK;
	        case FILE_DEVICE_NETWORK_FILE_SYSTEM:
	        	return DRIVE_REMOTE;
	        case FILE_DEVICE_DISK:
	        case FILE_DEVICE_DISK_FILE_SYSTEM:
			if (FileFsDevice.Characteristics & FILE_REMOTE_DEVICE)
				return DRIVE_REMOTE;
			if (FileFsDevice.Characteristics & FILE_REMOVABLE_MEDIA)
				return DRIVE_REMOVABLE;
			return DRIVE_FIXED;
        }

	return DRIVE_UNKNOWN;
}


/*
 * @implemented
 */
BOOL STDCALL
GetVolumeInformationA(
	LPCSTR	lpRootPathName,
	LPSTR	lpVolumeNameBuffer,
	DWORD	nVolumeNameSize,
	LPDWORD	lpVolumeSerialNumber,
	LPDWORD	lpMaximumComponentLength,
	LPDWORD	lpFileSystemFlags,
	LPSTR	lpFileSystemNameBuffer,
	DWORD	nFileSystemNameSize
	)
{
  UNICODE_STRING RootPathNameU;
  UNICODE_STRING FileSystemNameU;
  UNICODE_STRING VolumeNameU;
  ANSI_STRING RootPathName;
  ANSI_STRING VolumeName;
  ANSI_STRING FileSystemName;
  BOOL Result;

  RtlInitAnsiString (&RootPathName,
	             (LPSTR)lpRootPathName);

  /* convert ansi (or oem) string to unicode */
  if (bIsFileApiAnsi)
    RtlAnsiStringToUnicodeString (&RootPathNameU,
		                  &RootPathName,
		                  TRUE);
  else
    RtlOemStringToUnicodeString (&RootPathNameU,
		                 &RootPathName,
		                 TRUE);

  if (lpVolumeNameBuffer)
    {
      VolumeNameU.Length = 0;
      VolumeNameU.MaximumLength = nVolumeNameSize * sizeof(WCHAR);
      VolumeNameU.Buffer = RtlAllocateHeap (RtlGetProcessHeap (),
	                                    0,
	                                    VolumeNameU.MaximumLength);
    }

  if (lpFileSystemNameBuffer)
    {
      FileSystemNameU.Length = 0;
      FileSystemNameU.MaximumLength = nFileSystemNameSize * sizeof(WCHAR);
      FileSystemNameU.Buffer = RtlAllocateHeap (RtlGetProcessHeap (),
	                                        0,
	                                        FileSystemNameU.MaximumLength);
    }

  Result = GetVolumeInformationW (RootPathNameU.Buffer,
	                          lpVolumeNameBuffer ? VolumeNameU.Buffer : NULL,
	                          nVolumeNameSize,
	                          lpVolumeSerialNumber,
	                          lpMaximumComponentLength,
	                          lpFileSystemFlags,
				  lpFileSystemNameBuffer ? FileSystemNameU.Buffer : NULL,
	                          nFileSystemNameSize);

  if (Result)
    {
      if (lpVolumeNameBuffer)
        {
          VolumeNameU.Length = wcslen(VolumeNameU.Buffer) * sizeof(WCHAR);
	  VolumeName.Length = 0;
	  VolumeName.MaximumLength = nVolumeNameSize;
	  VolumeName.Buffer = lpVolumeNameBuffer;
	}

      if (lpFileSystemNameBuffer)
	{
	  FileSystemNameU.Length = wcslen(FileSystemNameU.Buffer) * sizeof(WCHAR);
	  FileSystemName.Length = 0;
	  FileSystemName.MaximumLength = nFileSystemNameSize;
	  FileSystemName.Buffer = lpFileSystemNameBuffer;
	}

      /* convert unicode strings to ansi (or oem) */
      if (bIsFileApiAnsi)
        {
	  if (lpVolumeNameBuffer)
	    {
	      RtlUnicodeStringToAnsiString (&VolumeName,
			                    &VolumeNameU,
			                    FALSE);
	    }
	  if (lpFileSystemNameBuffer)
	    {
	      RtlUnicodeStringToAnsiString (&FileSystemName,
			                    &FileSystemNameU,
			                    FALSE);
	    }
	}
      else
        {
	  if (lpVolumeNameBuffer)
	    {
	      RtlUnicodeStringToOemString (&VolumeName,
			                   &VolumeNameU,
			                   FALSE);
	    }
          if (lpFileSystemNameBuffer)
	    {
	      RtlUnicodeStringToOemString (&FileSystemName,
			                   &FileSystemNameU,
			                   FALSE);
	    }
	}
    }

  RtlFreeHeap (RtlGetProcessHeap (),
	       0,
	       RootPathNameU.Buffer);
  if (lpVolumeNameBuffer)
    {
      RtlFreeHeap (RtlGetProcessHeap (),
	           0,
	           VolumeNameU.Buffer);
    }
  if (lpFileSystemNameBuffer)
    {
      RtlFreeHeap (RtlGetProcessHeap (),
	           0,
	           FileSystemNameU.Buffer);
    }

  return Result;
}

#define FS_VOLUME_BUFFER_SIZE (MAX_PATH * sizeof(WCHAR) + sizeof(FILE_FS_VOLUME_INFORMATION))

#define FS_ATTRIBUTE_BUFFER_SIZE (MAX_PATH * sizeof(WCHAR) + sizeof(FILE_FS_ATTRIBUTE_INFORMATION))

/*
 * @implemented
 */
BOOL STDCALL
GetVolumeInformationW(
    LPCWSTR lpRootPathName,
    LPWSTR lpVolumeNameBuffer,
    DWORD nVolumeNameSize,
    LPDWORD lpVolumeSerialNumber,
    LPDWORD lpMaximumComponentLength,
    LPDWORD lpFileSystemFlags,
    LPWSTR lpFileSystemNameBuffer,
    DWORD nFileSystemNameSize
    )
{
  PFILE_FS_VOLUME_INFORMATION FileFsVolume;
  PFILE_FS_ATTRIBUTE_INFORMATION FileFsAttribute;
  IO_STATUS_BLOCK IoStatusBlock;
  WCHAR RootPathName[MAX_PATH];
  UCHAR Buffer[max(FS_VOLUME_BUFFER_SIZE, FS_ATTRIBUTE_BUFFER_SIZE)];

  HANDLE hFile;
  NTSTATUS errCode;

  FileFsVolume = (PFILE_FS_VOLUME_INFORMATION)Buffer;
  FileFsAttribute = (PFILE_FS_ATTRIBUTE_INFORMATION)Buffer;

  DPRINT("FileFsVolume %p\n", FileFsVolume);
  DPRINT("FileFsAttribute %p\n", FileFsAttribute);

  if (!lpRootPathName || !wcscmp(lpRootPathName, L""))
  {
      GetCurrentDirectoryW (MAX_PATH, RootPathName);
  }
  else
  {
      wcsncpy (RootPathName, lpRootPathName, 3);
  }
  RootPathName[3] = 0;

  hFile = InternalOpenDirW(RootPathName, FALSE);
  if (hFile == INVALID_HANDLE_VALUE)
    {
      return FALSE;
    }

  DPRINT("hFile: %x\n", hFile);
  errCode = NtQueryVolumeInformationFile(hFile,
                                         &IoStatusBlock,
                                         FileFsVolume,
                                         FS_VOLUME_BUFFER_SIZE,
                                         FileFsVolumeInformation);
  if ( !NT_SUCCESS(errCode) ) 
    {
      DPRINT("Status: %x\n", errCode);
      CloseHandle(hFile);
      SetLastErrorByStatus (errCode);
      return FALSE;
    }

  if (lpVolumeSerialNumber)
    *lpVolumeSerialNumber = FileFsVolume->VolumeSerialNumber;

  if (lpVolumeNameBuffer)
    {
      if (nVolumeNameSize * sizeof(WCHAR) >= FileFsVolume->VolumeLabelLength + sizeof(WCHAR))
        {
	  memcpy(lpVolumeNameBuffer, 
		 FileFsVolume->VolumeLabel, 
		 FileFsVolume->VolumeLabelLength);
	  lpVolumeNameBuffer[FileFsVolume->VolumeLabelLength / sizeof(WCHAR)] = 0;
	}
      else
        {
	  CloseHandle(hFile);
	  SetLastError(ERROR_MORE_DATA);
	  return FALSE;
	}
    }

  errCode = NtQueryVolumeInformationFile (hFile,
	                                  &IoStatusBlock,
	                                  FileFsAttribute,
	                                  FS_ATTRIBUTE_BUFFER_SIZE,
	                                  FileFsAttributeInformation);
  CloseHandle(hFile);
  if (!NT_SUCCESS(errCode))
    {
      DPRINT("Status: %x\n", errCode);
      SetLastErrorByStatus (errCode);
      return FALSE;
    }

  if (lpFileSystemFlags)
    *lpFileSystemFlags = FileFsAttribute->FileSystemAttributes;
  if (lpMaximumComponentLength)
    *lpMaximumComponentLength = FileFsAttribute->MaximumComponentNameLength;
  if (lpFileSystemNameBuffer)
    {
      if (nFileSystemNameSize * sizeof(WCHAR) >= FileFsAttribute->FileSystemNameLength + sizeof(WCHAR))
        {
	  memcpy(lpFileSystemNameBuffer, 
		 FileFsAttribute->FileSystemName, 
		 FileFsAttribute->FileSystemNameLength);
	  lpFileSystemNameBuffer[FileFsAttribute->FileSystemNameLength / sizeof(WCHAR)] = 0;
	}
      else
        {
	  SetLastError(ERROR_MORE_DATA);
	  return FALSE;
	}
    }
  return TRUE;
}


/*
 * @implemented
 */
BOOL
STDCALL
SetVolumeLabelA (
	LPCSTR	lpRootPathName,
	LPCSTR	lpVolumeName
	)
{
	UNICODE_STRING RootPathNameU;
	ANSI_STRING RootPathName;
	UNICODE_STRING VolumeNameU;
	ANSI_STRING VolumeName;
	BOOL Result;

	RtlInitAnsiString (&RootPathName,
	                   (LPSTR)lpRootPathName);
	RtlInitAnsiString (&VolumeName,
	                   (LPSTR)lpVolumeName);

	/* convert ansi (or oem) strings to unicode */
	if (bIsFileApiAnsi)
	{
		RtlAnsiStringToUnicodeString (&RootPathNameU,
		                              &RootPathName,
		                              TRUE);
		RtlAnsiStringToUnicodeString (&VolumeNameU,
		                              &VolumeName,
		                              TRUE);
	}
	else
	{
		RtlOemStringToUnicodeString (&RootPathNameU,
		                             &RootPathName,
		                             TRUE);
		RtlOemStringToUnicodeString (&VolumeNameU,
		                             &VolumeName,
		                             TRUE);
	}

	Result = SetVolumeLabelW (RootPathNameU.Buffer,
	                          VolumeNameU.Buffer);

	RtlFreeHeap (RtlGetProcessHeap (),
	             0,
	             RootPathNameU.Buffer);
	RtlFreeHeap (RtlGetProcessHeap (),
	             0,
	             VolumeNameU.Buffer);

	return Result;
}


/*
 * @implemented
 */
BOOL STDCALL
SetVolumeLabelW(LPCWSTR lpRootPathName,
		LPCWSTR lpVolumeName)
{
   PFILE_FS_LABEL_INFORMATION LabelInfo;
   IO_STATUS_BLOCK IoStatusBlock;
   ULONG LabelLength;
   HANDLE hFile;
   NTSTATUS Status;
   
   LabelLength = wcslen(lpVolumeName) * sizeof(WCHAR);
   LabelInfo = RtlAllocateHeap(RtlGetProcessHeap(),
			       0,
			       sizeof(FILE_FS_LABEL_INFORMATION) +
			       LabelLength);
   LabelInfo->VolumeLabelLength = LabelLength;
   memcpy(LabelInfo->VolumeLabel,
	  lpVolumeName,
	  LabelLength);

   hFile = InternalOpenDirW(lpRootPathName, TRUE);
   if (INVALID_HANDLE_VALUE == hFile)
   {
        RtlFreeHeap(RtlGetProcessHeap(),
	            0,
	            LabelInfo);
        return FALSE;
   }
   
   Status = NtSetVolumeInformationFile(hFile,
				       &IoStatusBlock,
				       LabelInfo,
				       sizeof(FILE_FS_LABEL_INFORMATION) +
				       LabelLength,
				       FileFsLabelInformation);

   RtlFreeHeap(RtlGetProcessHeap(),
	       0,
	       LabelInfo);

   if (!NT_SUCCESS(Status))
     {
	DPRINT("Status: %x\n", Status);
	CloseHandle(hFile);
	SetLastErrorByStatus(Status);
	return FALSE;
     }

   CloseHandle(hFile);
   return TRUE;
}

/* EOF */
