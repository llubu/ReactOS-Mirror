/* $Id: delete.c,v 1.17 2004/08/18 02:13:27 navaraf Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/delete.c
 * PURPOSE:         Deleting files
 * PROGRAMMER:      Ariadne (ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/* INCLUDES ****************************************************************/

#include <k32.h>

#define NDEBUG
#include "../include/debug.h"


/* FUNCTIONS ****************************************************************/

/*
 * @implemented
 */
BOOL
STDCALL
DeleteFileA (
	LPCSTR	lpFileName
	)
{
	UNICODE_STRING FileNameU;
	ANSI_STRING FileName;
	BOOL Result;

	RtlInitAnsiString (&FileName,
	                   (LPSTR)lpFileName);

	/* convert ansi (or oem) string to unicode */
	if (bIsFileApiAnsi)
		RtlAnsiStringToUnicodeString (&FileNameU,
		                              &FileName,
		                              TRUE);
	else
		RtlOemStringToUnicodeString (&FileNameU,
		                             &FileName,
		                             TRUE);

	Result = DeleteFileW (FileNameU.Buffer);

	RtlFreeHeap (RtlGetProcessHeap (),
	             0,
	             FileNameU.Buffer);

	return Result;
}


/*
 * @implemented
 */
BOOL
STDCALL
DeleteFileW (
	LPCWSTR	lpFileName
	)
{
	FILE_DISPOSITION_INFORMATION FileDispInfo;
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK IoStatusBlock;
	UNICODE_STRING NtPathU;
	HANDLE FileHandle;
	NTSTATUS Status;

	DPRINT("DeleteFileW (lpFileName %S)\n",lpFileName);

	if (!RtlDosPathNameToNtPathName_U ((LPWSTR)lpFileName,
	                                   &NtPathU,
	                                   NULL,
	                                   NULL))
		return FALSE;

	DPRINT("NtPathU \'%wZ\'\n", &NtPathU);

	ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
	ObjectAttributes.RootDirectory = NULL;
	ObjectAttributes.ObjectName = &NtPathU;
	ObjectAttributes.Attributes = OBJ_CASE_INSENSITIVE| OBJ_INHERIT;
	ObjectAttributes.SecurityDescriptor = NULL;
	ObjectAttributes.SecurityQualityOfService = NULL;

	Status = NtCreateFile (&FileHandle,
	                       FILE_WRITE_ATTRIBUTES,
	                       &ObjectAttributes,
	                       &IoStatusBlock,
	                       NULL,
	                       FILE_ATTRIBUTE_NORMAL,
	                       0,
	                       FILE_OPEN,
                               FILE_NON_DIRECTORY_FILE,
	                       NULL,
	                       0);

	RtlFreeUnicodeString(&NtPathU);

	if (!NT_SUCCESS(Status))
	{
		CHECKPOINT;
		SetLastErrorByStatus (Status);
		return FALSE;
	}

	FileDispInfo.DeleteFile = TRUE;

	Status = NtSetInformationFile (FileHandle,
	                               &IoStatusBlock,
	                               &FileDispInfo,
	                               sizeof(FILE_DISPOSITION_INFORMATION),
	                               FileDispositionInformation);
	if (!NT_SUCCESS(Status))
	{
		CHECKPOINT;
		NtClose (FileHandle);
		SetLastErrorByStatus (Status);
		return FALSE;
	}

	Status = NtClose (FileHandle);
	if (!NT_SUCCESS (Status))
	{
		CHECKPOINT;
		SetLastErrorByStatus (Status);
		return FALSE;
	}

	return TRUE;
}

/* EOF */
