/* $Id: npipe.c,v 1.5 2001/05/10 23:37:06 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/npipe.c
 * PURPOSE:         Directory functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <ntdll/rtl.h>
#include <windows.h>
//#include <wchar.h>
//#include <string.h>

#include <kernel32/kernel32.h>
#include <kernel32/error.h>

/* FUNCTIONS ****************************************************************/

HANDLE STDCALL
CreateNamedPipeA(LPCSTR lpName,
		 DWORD dwOpenMode,
		 DWORD dwPipeMode,
		 DWORD nMaxInstances,
		 DWORD nOutBufferSize,
		 DWORD nInBufferSize,
		 DWORD nDefaultTimeOut,
		 LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
   HANDLE NamedPipeHandle;
   UNICODE_STRING NameU;
   ANSI_STRING NameA;
   
   RtlInitAnsiString(&NameA, (LPSTR)lpName);
   RtlAnsiStringToUnicodeString(&NameU, &NameA, TRUE);
   
   NamedPipeHandle = CreateNamedPipeW(NameU.Buffer,
				      dwOpenMode,
				      dwPipeMode,
				      nMaxInstances,
				      nOutBufferSize,
				      nInBufferSize,
				      nDefaultTimeOut,
				      lpSecurityAttributes);
   
   RtlFreeUnicodeString(&NameU);
   
   return(NamedPipeHandle);
}

HANDLE STDCALL
CreateNamedPipeW(LPCWSTR lpName,
		 DWORD dwOpenMode,
		 DWORD dwPipeMode,
		 DWORD nMaxInstances,
		 DWORD nOutBufferSize,
		 DWORD nInBufferSize,
		 DWORD nDefaultTimeOut,
		 LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
   UNICODE_STRING NamedPipeName;
   BOOL Result;
   NTSTATUS Status;
   OBJECT_ATTRIBUTES ObjectAttributes;
   HANDLE PipeHandle;
   ACCESS_MASK DesiredAccess;
   ULONG CreateOptions;
   ULONG CreateDisposition;
   BOOLEAN WriteModeMessage;
   BOOLEAN ReadModeMessage;
   BOOLEAN NonBlocking;
   IO_STATUS_BLOCK Iosb;
   ULONG ShareAccess;
   LARGE_INTEGER DefaultTimeOut;
   
   Result = RtlDosPathNameToNtPathName_U((LPWSTR)lpName,
					 &NamedPipeName,
					 NULL,
					 NULL);
   if (!Result)
     {
	SetLastError(ERROR_PATH_NOT_FOUND);
	return(INVALID_HANDLE_VALUE);
     }
   
   DPRINT("Pipe name: %wZ\n", &NamedPipeName);
   
   InitializeObjectAttributes(&ObjectAttributes,
			      &NamedPipeName,
			      OBJ_CASE_INSENSITIVE,
			      NULL,
			      NULL);
   
   DesiredAccess = 0;
   
   ShareAccess = 0;
   
   CreateDisposition = FILE_OPEN_IF;
   
   CreateOptions = 0;
   if (dwOpenMode & FILE_FLAG_WRITE_THROUGH)
     {
	CreateOptions = CreateOptions | FILE_WRITE_THROUGH;
     }
   if (dwOpenMode & FILE_FLAG_OVERLAPPED)
     {
	CreateOptions = CreateOptions | FILE_SYNCHRONOUS_IO_ALERT;
     }
   
   if (dwPipeMode & PIPE_TYPE_BYTE)
     {
	WriteModeMessage = FALSE;
     }
   else if (dwPipeMode & PIPE_TYPE_MESSAGE)
     {
	WriteModeMessage = TRUE;
     }
   else
     {
	WriteModeMessage = FALSE;
     }
   
   if (dwPipeMode & PIPE_READMODE_BYTE)
     {
	ReadModeMessage = FALSE;
     }
   else if (dwPipeMode & PIPE_READMODE_MESSAGE)
     {
	ReadModeMessage = TRUE;
     }
   else
     {
	ReadModeMessage = FALSE;
     }
   
   if (dwPipeMode & PIPE_WAIT)
     {
	NonBlocking = FALSE;
     }
   else if (dwPipeMode & PIPE_NOWAIT)
     {
	NonBlocking = TRUE;
     }
   else
     {
	NonBlocking = FALSE;
     }
   
   DefaultTimeOut.QuadPart = nDefaultTimeOut * 10000;
   
   Status = NtCreateNamedPipeFile(&PipeHandle,
				  DesiredAccess,
				  &ObjectAttributes,
				  &Iosb,
				  ShareAccess,
				  CreateDisposition,
				  CreateOptions,
				  WriteModeMessage,
				  ReadModeMessage,
				  NonBlocking,
				  nMaxInstances,
				  nInBufferSize,
				  nOutBufferSize,
				  &DefaultTimeOut);
   
   RtlFreeUnicodeString(&NamedPipeName);
   
   if (!NT_SUCCESS(Status))
     {
	DPRINT("NtCreateNamedPipe failed (Status %x)!\n", Status);
	SetLastErrorByStatus (Status);
	return(INVALID_HANDLE_VALUE);
     }
   
   return(PipeHandle);
}

BOOL STDCALL
WaitNamedPipeA(LPCSTR lpNamedPipeName,
	       DWORD nTimeOut)
{
   BOOL r;
   UNICODE_STRING NameU;
   ANSI_STRING NameA;
   
   RtlInitAnsiString(&NameA, (LPSTR)lpNamedPipeName);
   RtlAnsiStringToUnicodeString(&NameU, &NameA, TRUE);
   
   r = WaitNamedPipeW(NameU.Buffer, nTimeOut);
   
   RtlFreeUnicodeString(&NameU);
   
   return(r);
}

BOOL STDCALL
WaitNamedPipeW(LPCWSTR lpNamedPipeName,
	       DWORD nTimeOut)
{
   UNICODE_STRING NamedPipeName;
   BOOL r;
   NTSTATUS Status;
   OBJECT_ATTRIBUTES ObjectAttributes;
   NPFS_WAIT_PIPE WaitPipe;
   HANDLE FileHandle;
   IO_STATUS_BLOCK Iosb;
   
   r = RtlDosPathNameToNtPathName_U((LPWSTR)lpNamedPipeName,
				    &NamedPipeName,
				    NULL,
				    NULL);
   
   if (!r)
     {
	return(FALSE);
     }
   
   InitializeObjectAttributes(&ObjectAttributes,
			      &NamedPipeName,
			      0,
			      NULL,
			      NULL);
   Status = NtOpenFile(&FileHandle,
		       FILE_GENERIC_READ,
		       &ObjectAttributes,
		       &Iosb,
		       0,
		       FILE_SYNCHRONOUS_IO_ALERT);
   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus (Status);
	return(FALSE);
     }
   
   WaitPipe.Timeout.QuadPart = nTimeOut * 10000;
   
#if 0
   Status = NtFsControlFile(FileHandle,
			    NULL,
			    NULL,
			    NULL,
			    &Iosb,
			    FSCTL_WAIT_PIPE,
			    &WaitPipe,
			    sizeof(WaitPipe),
			    NULL,
			    0);
   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus (Status);
	return(FALSE);
     }
#endif
   
   NtClose(FileHandle);
   return(TRUE);
}

BOOL STDCALL
ConnectNamedPipe(HANDLE hNamedPipe,
		 LPOVERLAPPED lpOverlapped)
{
   IO_STATUS_BLOCK Iosb;
   HANDLE hEvent;
   PIO_STATUS_BLOCK IoStatusBlock;
   NTSTATUS Status;
   
   if (lpOverlapped != NULL)
     {
	lpOverlapped->Internal = STATUS_PENDING;
	hEvent = lpOverlapped->hEvent;
	IoStatusBlock = (PIO_STATUS_BLOCK)lpOverlapped;
     }
   else
     {
	IoStatusBlock = &Iosb;
	hEvent = NULL;
     }
   
   Status = NtFsControlFile(hNamedPipe,
			    hEvent,
			    NULL,
			    NULL,
			    IoStatusBlock,
			    FSCTL_PIPE_LISTEN,
			    NULL,
			    0,
			    NULL,
			    0);
   if ((lpOverlapped == NULL) && (Status == STATUS_PENDING))
     {
	Status = NtWaitForSingleObject(hNamedPipe,
				       FALSE,
				       NULL);
	if (!NT_SUCCESS(Status))
	  {
	     SetLastErrorByStatus(Status);
	     return(FALSE);
	  }
	Status = Iosb.Status;
     }
   if (!NT_SUCCESS(Status) || (Status == STATUS_PENDING))
     {
	SetLastErrorByStatus (Status);
	return(FALSE);
     }
   return(TRUE);
}

BOOL STDCALL
SetNamedPipeHandleState(HANDLE hNamedPipe,
			LPDWORD lpMode,
			LPDWORD lpMaxCollectionCount,
			LPDWORD lpCollectDataTimeout)
{
   NPFS_GET_STATE GetState;
   NPFS_SET_STATE SetState;
   IO_STATUS_BLOCK Iosb;
   NTSTATUS Status;
   
#if 0
   Status = NtFsControlFile(hNamedPipe,
			    NULL,
			    NULL,
			    NULL,
			    &Iosb,
			    FSCTL_GET_STATE,
			    NULL,
			    0,
			    &GetState,
			    sizeof(GetState));
   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus (Status);
	return(FALSE);
     }
#endif
   
   if (lpMode != NULL)
     {
	if ((*lpMode) & PIPE_READMODE_MESSAGE)
	  {
	     SetState.ReadModeMessage = TRUE;
	  }
	else
	  {
	     SetState.ReadModeMessage = FALSE;
	  }
	if ((*lpMode) & PIPE_NOWAIT)
	  {
	     SetState.NonBlocking = TRUE;
	  }
	else
	  {
	     SetState.NonBlocking = FALSE;
	  }
	SetState.WriteModeMessage = GetState.WriteModeMessage;
     }
   else
     {
	SetState.ReadModeMessage = GetState.ReadModeMessage;
	SetState.WriteModeMessage = GetState.WriteModeMessage;
	SetState.NonBlocking = SetState.NonBlocking;
     }
   
   if (lpMaxCollectionCount != NULL)
     {
	SetState.InBufferSize = *lpMaxCollectionCount;
     }
   else
     {
	SetState.InBufferSize = GetState.InBufferSize;
     }
   
   SetState.OutBufferSize = GetState.OutBufferSize;
   
   if (lpCollectDataTimeout != NULL)
     {
	SetState.Timeout.QuadPart = (*lpCollectDataTimeout) * 1000 * 1000;
     }
   else
     {
	SetState.Timeout = GetState.Timeout;
     }
   
#if 0
   Status = NtFsControlFile(hNamedPipe,
			    NULL,
			    NULL,
			    NULL,
			    &Iosb,
			    FSCTL_SET_STATE,
			    &SetState,
			    sizeof(SetState),
			    NULL,
			    0);
   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus (Status);
	return(FALSE);
     }
#endif
   return(TRUE);
}

WINBOOL
STDCALL
CallNamedPipeA (
	LPCSTR	lpNamedPipeName,
	LPVOID	lpInBuffer,
	DWORD	nInBufferSize,
	LPVOID	lpOutBuffer,
	DWORD	nOutBufferSize,
	LPDWORD	lpBytesRead,
	DWORD	nTimeOut
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL STDCALL
CallNamedPipeW (
	LPCWSTR	lpNamedPipeName,
	LPVOID	lpInBuffer,
	DWORD	nInBufferSize,
	LPVOID	lpOutBuffer,
	DWORD	nOutBufferSize,
	LPDWORD	lpBytesRead,
	DWORD	nTimeOut
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL STDCALL
DisconnectNamedPipe(HANDLE hNamedPipe)
{
   IO_STATUS_BLOCK Iosb;
   NTSTATUS Status;

   Status = NtFsControlFile(hNamedPipe,
			    NULL,
			    NULL,
			    NULL,
			    &Iosb,
			    FSCTL_PIPE_DISCONNECT,
			    NULL,
			    0,
			    NULL,
			    0);
   if (Status == STATUS_PENDING)
     {
	Status = NtWaitForSingleObject(hNamedPipe,
				       FALSE,
				       NULL);
	if (!NT_SUCCESS(Status))
	  {
	     SetLastErrorByStatus(Status);
	     return(FALSE);
	  }
     }

   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus(Status);
	return(FALSE);
     }
   return(TRUE);
}


WINBOOL STDCALL
GetNamedPipeHandleStateW (
	HANDLE	hNamedPipe,
	LPDWORD	lpState,
	LPDWORD	lpCurInstances,
	LPDWORD	lpMaxCollectionCount,
	LPDWORD	lpCollectDataTimeout,
	LPWSTR	lpUserName,
	DWORD	nMaxUserNameSize
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL STDCALL
GetNamedPipeHandleStateA (
	HANDLE	hNamedPipe,
	LPDWORD	lpState,
	LPDWORD	lpCurInstances,
	LPDWORD	lpMaxCollectionCount,
	LPDWORD	lpCollectDataTimeout,
	LPSTR	lpUserName,
	DWORD	nMaxUserNameSize
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL STDCALL
GetNamedPipeInfo(HANDLE hNamedPipe,
		 LPDWORD lpFlags,
		 LPDWORD lpOutBufferSize,
		 LPDWORD lpInBufferSize,
		 LPDWORD lpMaxInstances)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL STDCALL
PeekNamedPipe(HANDLE hNamedPipe,
	      LPVOID lpBuffer,
	      DWORD nBufferSize,
	      LPDWORD lpBytesRead,
	      LPDWORD lpTotalBytesAvail,
	      LPDWORD lpBytesLeftThisMessage)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL STDCALL
TransactNamedPipe(HANDLE hNamedPipe,
		  LPVOID lpInBuffer,
		  DWORD nInBufferSize,
		  LPVOID lpOutBuffer,
		  DWORD nOutBufferSize,
		  LPDWORD lpBytesRead,
		  LPOVERLAPPED lpOverlapped)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

/* EOF */
