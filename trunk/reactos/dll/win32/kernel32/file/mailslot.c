/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/mailslot.c
 * PURPOSE:         Mailslot functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 */

/* INCLUDES *****************************************************************/

#include <k32.h>

#define NDEBUG
#include "../include/debug.h"

/* FUNCTIONS ****************************************************************/

/*
 * @implemented
 */
HANDLE STDCALL
CreateMailslotA(LPCSTR lpName,
		DWORD nMaxMessageSize,
		DWORD lReadTimeout,
		LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
   HANDLE MailslotHandle;
   UNICODE_STRING NameU;
   ANSI_STRING NameA;

   RtlInitAnsiString(&NameA, (LPSTR)lpName);
   RtlAnsiStringToUnicodeString(&NameU, &NameA, TRUE);

   MailslotHandle = CreateMailslotW(NameU.Buffer,
				    nMaxMessageSize,
				    lReadTimeout,
				    lpSecurityAttributes);

   RtlFreeUnicodeString(&NameU);

   return(MailslotHandle);
}


/*
 * @implemented
 */
HANDLE STDCALL
CreateMailslotW(LPCWSTR lpName,
		DWORD nMaxMessageSize,
		DWORD lReadTimeout,
		LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
   OBJECT_ATTRIBUTES ObjectAttributes;
   UNICODE_STRING MailslotName;
   HANDLE MailslotHandle;
   NTSTATUS Status;
   BOOLEAN Result;
   LARGE_INTEGER DefaultTimeOut;
   IO_STATUS_BLOCK Iosb;
   ULONG Attributes = OBJ_CASE_INSENSITIVE;
   PSECURITY_DESCRIPTOR SecurityDescriptor = NULL;

   Result = RtlDosPathNameToNtPathName_U(lpName,
					 &MailslotName,
					 NULL,
					 NULL);
   if (!Result)
     {
	SetLastError(ERROR_PATH_NOT_FOUND);
	return(INVALID_HANDLE_VALUE);
     }

   DPRINT("Mailslot name: %wZ\n", &MailslotName);

   if(lpSecurityAttributes)
     {
       SecurityDescriptor = lpSecurityAttributes->lpSecurityDescriptor;
       if(lpSecurityAttributes->bInheritHandle)
          Attributes |= OBJ_INHERIT;
     }

   InitializeObjectAttributes(&ObjectAttributes,
			      &MailslotName,
			      Attributes,
			      NULL,
			      SecurityDescriptor);

   DefaultTimeOut.QuadPart = lReadTimeout * 10000;

   Status = NtCreateMailslotFile(&MailslotHandle,
				 GENERIC_READ | SYNCHRONIZE | WRITE_DAC,
				 &ObjectAttributes,
				 &Iosb,
				 FILE_WRITE_THROUGH,
				 0,
				 nMaxMessageSize,
				 &DefaultTimeOut);

   RtlFreeHeap(RtlGetProcessHeap(),
               0,
               MailslotName.Buffer);

   if (!NT_SUCCESS(Status))
     {
	DPRINT("NtCreateMailslot failed (Status %x)!\n", Status);
	SetLastErrorByStatus (Status);
	return(INVALID_HANDLE_VALUE);
     }

   return(MailslotHandle);
}


/*
 * @implemented
 */
BOOL STDCALL
GetMailslotInfo(HANDLE hMailslot,
		LPDWORD lpMaxMessageSize,
		LPDWORD lpNextSize,
		LPDWORD lpMessageCount,
		LPDWORD lpReadTimeout)
{
   FILE_MAILSLOT_QUERY_INFORMATION Buffer;
   IO_STATUS_BLOCK Iosb;
   NTSTATUS Status;

   Status = NtQueryInformationFile(hMailslot,
				   &Iosb,
				   &Buffer,
				   sizeof(FILE_MAILSLOT_QUERY_INFORMATION),
				   FileMailslotQueryInformation);
   if (!NT_SUCCESS(Status))
     {
	DPRINT("NtQueryInformationFile failed (Status %x)!\n", Status);
	SetLastErrorByStatus (Status);
	return(FALSE);
     }

   if (lpMaxMessageSize != NULL)
     {
	*lpMaxMessageSize = Buffer.MaximumMessageSize;
     }
   if (lpNextSize != NULL)
     {
	*lpNextSize = Buffer.NextMessageSize;
     }
   if (lpMessageCount != NULL)
     {
	*lpMessageCount = Buffer.MessagesAvailable;
     }
   if (lpReadTimeout != NULL)
     {
	*lpReadTimeout = (DWORD)(Buffer.ReadTimeout.QuadPart / -10000);
     }

   return(TRUE);
}


/*
 * @implemented
 */
BOOL STDCALL
SetMailslotInfo(HANDLE hMailslot,
		DWORD lReadTimeout)
{
   FILE_MAILSLOT_SET_INFORMATION Buffer;
   LARGE_INTEGER Timeout;
   IO_STATUS_BLOCK Iosb;
   NTSTATUS Status;

   Timeout.QuadPart = lReadTimeout * -10000;
   Buffer.ReadTimeout = &Timeout;

   Status = NtSetInformationFile(hMailslot,
				 &Iosb,
				 &Buffer,
				 sizeof(FILE_MAILSLOT_SET_INFORMATION),
				 FileMailslotSetInformation);
   if (!NT_SUCCESS(Status))
     {
	DPRINT("NtSetInformationFile failed (Status %x)!\n", Status);
	SetLastErrorByStatus (Status);
	return(FALSE);
     }

   return(TRUE);
}

/* EOF */
