/* $Id: cnotify.c,v 1.11 2004/10/08 21:29:40 weiden Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/find.c
 * PURPOSE:         Find functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

#include <k32.h>

#define NDEBUG
#include "../include/debug.h"

/*
 * @implemented
 */
BOOL STDCALL
FindCloseChangeNotification (HANDLE hChangeHandle)
{
   NTSTATUS Status = NtClose(hChangeHandle);
   if(!NT_SUCCESS(Status))
   {
     SetLastErrorByStatus(Status);
     return FALSE;
   }
   
   return TRUE;
}


/*
 * @implemented
 */
HANDLE
STDCALL
FindFirstChangeNotificationA (
	LPCSTR	lpPathName,
	BOOL	bWatchSubtree,
	DWORD	dwNotifyFilter
	)
{
	UNICODE_STRING PathNameU;
	ANSI_STRING PathName;
	HANDLE hNotify;

	RtlInitAnsiString (&PathName,
	                   (LPSTR)lpPathName);

	/* convert ansi (or oem) string to unicode */
	if (bIsFileApiAnsi)
   {
		RtlAnsiStringToUnicodeString (&PathNameU,
		                              &PathName,
		                              TRUE);
   }
	else
   {
		RtlOemStringToUnicodeString (&PathNameU,
		                             &PathName,
		                             TRUE);
   }

   hNotify = FindFirstChangeNotificationW (PathNameU.Buffer,
	                                        bWatchSubtree,
	                                        dwNotifyFilter);

   RtlFreeUnicodeString(&PathNameU);

   return hNotify;
}


/*
 * @implemented
 */
HANDLE
STDCALL
FindFirstChangeNotificationW (
	LPCWSTR	lpPathName,
	BOOL	bWatchSubtree,
	DWORD	dwNotifyFilter
	)
{
   NTSTATUS Status;
   UNICODE_STRING NtPathU;
   IO_STATUS_BLOCK IoStatus;
   OBJECT_ATTRIBUTES ObjectAttributes;
   HANDLE hDir;

   /*
   RtlDosPathNameToNtPathName takes a fully qualified file name "C:\Projects\LoadLibrary\Debug\TestDll.dll" 
   and returns something like "\??\C:\Projects\LoadLibrary\Debug\TestDll.dll."
   If the file name cannot be interpreted, then the routine returns STATUS_OBJECT_PATH_SYNTAX_BAD and 
   ends execution.
   */

   if (!RtlDosPathNameToNtPathName_U ((LPWSTR)lpPathName,
                                          &NtPathU,
                                          NULL,
                                          NULL))
   {
      SetLastErrorByStatus(STATUS_OBJECT_PATH_SYNTAX_BAD);
      return INVALID_HANDLE_VALUE;
   }

   InitializeObjectAttributes (&ObjectAttributes,
                               &NtPathU,
                               0,
                               NULL,
                               NULL);

   Status = NtOpenFile (&hDir,
                        SYNCHRONIZE|FILE_LIST_DIRECTORY,
                        &ObjectAttributes,
                        &IoStatus,
                        FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                        FILE_DIRECTORY_FILE);

   if (!NT_SUCCESS(Status))
   {
      SetLastErrorByStatus(Status);
      return INVALID_HANDLE_VALUE;
   }

   Status = NtNotifyChangeDirectoryFile(hDir,
                                        NULL,
                                        NULL,
                                        NULL,
                                        &IoStatus,
                                        NULL,//Buffer,
                                        0,//BufferLength,
                                        dwNotifyFilter,
                                        bWatchSubtree);
   if (!NT_SUCCESS(Status))
   {
      SetLastErrorByStatus(Status);
      return INVALID_HANDLE_VALUE;
   }

   return hDir;
}


/*
 * @implemented
 */
BOOL
STDCALL
FindNextChangeNotification (
	HANDLE	hChangeHandle
	)
{
   IO_STATUS_BLOCK IoStatus;
   NTSTATUS Status;

   Status = NtNotifyChangeDirectoryFile(hChangeHandle,
                                        NULL,
                                        NULL,
                                        NULL,
                                        &IoStatus,
                                        NULL,//Buffer,
                                        0,//BufferLength,
                                        FILE_NOTIFY_CHANGE_SECURITY,//meaningless for subsequent calls, but must contain a valid flag(s)
                                        0//meaningless for subsequent calls 
                                        );
   if (!NT_SUCCESS(Status))
   {
      SetLastErrorByStatus(Status);
      return FALSE;
   }

   return TRUE;
}

/* EOF */
