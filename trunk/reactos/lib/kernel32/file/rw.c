/* $Id: rw.c,v 1.22 2003/07/31 18:24:43 royce Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/rw.c
 * PURPOSE:         Read/write functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/* INCLUDES ****************************************************************/

#include <k32.h>

#define NDEBUG
#include <kernel32/kernel32.h>


/* FUNCTIONS ****************************************************************/

/*
 * @implemented
 */
WINBOOL STDCALL
WriteFile(HANDLE hFile,
			  LPCVOID lpBuffer,	
			  DWORD nNumberOfBytesToWrite,
			  LPDWORD lpNumberOfBytesWritten,	
			  LPOVERLAPPED lpOverLapped)
{
   HANDLE hEvent = NULL;
   LARGE_INTEGER Offset;
   NTSTATUS errCode;
   IO_STATUS_BLOCK IIosb;
   PIO_STATUS_BLOCK IoStatusBlock;
   PLARGE_INTEGER ptrOffset;

   DPRINT("WriteFile(hFile %x)\n",hFile);
   
   if (IsConsoleHandle(hFile))
     {
	return(WriteConsoleA(hFile,
			     lpBuffer,
			     nNumberOfBytesToWrite,
			     lpNumberOfBytesWritten,
			     NULL));
     }
      
   if (lpOverLapped != NULL) 
     {
        Offset.u.LowPart = lpOverLapped->Offset;
        Offset.u.HighPart = lpOverLapped->OffsetHigh;
	lpOverLapped->Internal = STATUS_PENDING;
	hEvent = lpOverLapped->hEvent;
	IoStatusBlock = (PIO_STATUS_BLOCK)lpOverLapped;
	ptrOffset = &Offset;
     }
   else 
     {
	ptrOffset = NULL;
	IoStatusBlock = &IIosb;
        Offset.QuadPart = 0;
     }

   errCode = NtWriteFile(hFile,
			 hEvent,
			 NULL,
			 NULL,
			 IoStatusBlock,
			 (PVOID)lpBuffer, 
			 nNumberOfBytesToWrite,
			 ptrOffset,
			 NULL);
   if (!NT_SUCCESS(errCode))
     {
	SetLastErrorByStatus (errCode);
	DPRINT("WriteFile() failed\n");
	return FALSE;
     }
   if (lpNumberOfBytesWritten != NULL )
     {
	*lpNumberOfBytesWritten = IoStatusBlock->Information;
     }
   DPRINT("WriteFile() succeeded\n");
   return(TRUE);
}


/*
 * @implemented
 */
WINBOOL STDCALL
ReadFile(HANDLE hFile,
			 LPVOID lpBuffer,
			 DWORD nNumberOfBytesToRead,
			 LPDWORD lpNumberOfBytesRead,
			 LPOVERLAPPED lpOverLapped)
{
   HANDLE hEvent = NULL;
   LARGE_INTEGER Offset;
   NTSTATUS errCode;
   IO_STATUS_BLOCK IIosb;
   PIO_STATUS_BLOCK IoStatusBlock;
   PLARGE_INTEGER ptrOffset;
   
   if (IsConsoleHandle(hFile))
     {
	return(ReadConsoleA(hFile,
			    lpBuffer,
			    nNumberOfBytesToRead,
			    lpNumberOfBytesRead,
			    NULL));
     }
   
   if (lpOverLapped != NULL) 
     {
        Offset.u.LowPart = lpOverLapped->Offset;
        Offset.u.HighPart = lpOverLapped->OffsetHigh;
	lpOverLapped->Internal = STATUS_PENDING;
	hEvent = lpOverLapped->hEvent;
	IoStatusBlock = (PIO_STATUS_BLOCK)lpOverLapped;
	ptrOffset = &Offset;
     }
   else 
     {
	ptrOffset = NULL;
	IoStatusBlock = &IIosb;
     }
   
   errCode = NtReadFile(hFile,
			hEvent,
			NULL,
			NULL,
			IoStatusBlock,
			lpBuffer,
			nNumberOfBytesToRead,
			ptrOffset,
			NULL);
   
   if (errCode != STATUS_PENDING && lpNumberOfBytesRead != NULL)
     {
	*lpNumberOfBytesRead = IoStatusBlock->Information;
     }
   
   if (!NT_SUCCESS(errCode) && errCode != STATUS_END_OF_FILE)  
     {
	SetLastErrorByStatus (errCode);
	return(FALSE);
     }
   return(TRUE);
}

VOID STDCALL
ApcRoutine(PVOID ApcContext, 
		struct _IO_STATUS_BLOCK* IoStatusBlock, 
		ULONG NumberOfBytesTransfered)
{
   DWORD dwErrorCode;
   LPOVERLAPPED_COMPLETION_ROUTINE  lpCompletionRoutine = 
     (LPOVERLAPPED_COMPLETION_ROUTINE)ApcContext;
   
   dwErrorCode = RtlNtStatusToDosError(IoStatusBlock->Status);
   lpCompletionRoutine(dwErrorCode, 
		       NumberOfBytesTransfered, 
		       (LPOVERLAPPED)IoStatusBlock);
}


/*
 * @implemented
 */
WINBOOL STDCALL 
WriteFileEx (HANDLE				hFile,
	     LPCVOID				lpBuffer,
	     DWORD				nNumberOfBytesToWrite,
	     LPOVERLAPPED			lpOverLapped,
	     LPOVERLAPPED_COMPLETION_ROUTINE	lpCompletionRoutine)
{

   LARGE_INTEGER Offset;
   NTSTATUS errCode;
   PIO_STATUS_BLOCK IoStatusBlock;
   PLARGE_INTEGER ptrOffset;
   
   DPRINT("WriteFileEx(hFile %x)\n",hFile);
   
   if (lpOverLapped == NULL) 
	return FALSE;

   Offset.u.LowPart = lpOverLapped->Offset;
   Offset.u.HighPart = lpOverLapped->OffsetHigh;
   lpOverLapped->Internal = STATUS_PENDING;
   IoStatusBlock = (PIO_STATUS_BLOCK)lpOverLapped;
   ptrOffset = &Offset;

   errCode = NtWriteFile(hFile,
			 NULL,
			 ApcRoutine,
			 lpCompletionRoutine,
			 IoStatusBlock,
			 (PVOID)lpBuffer, 
			 nNumberOfBytesToWrite,
			 ptrOffset,
			 NULL);
   if (!NT_SUCCESS(errCode))
     {
	SetLastErrorByStatus (errCode);
	DPRINT("WriteFileEx() failed\n");
	return FALSE;
     }
  
   DPRINT("WriteFileEx() succeeded\n");
   return(TRUE);
}


/*
 * @implemented
 */
WINBOOL STDCALL
ReadFileEx(HANDLE hFile,
			   LPVOID lpBuffer,
			   DWORD nNumberOfBytesToRead,
			   LPOVERLAPPED lpOverLapped,
			   LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
   LARGE_INTEGER Offset;
   NTSTATUS errCode;
   PIO_STATUS_BLOCK IoStatusBlock;
   PLARGE_INTEGER ptrOffset;
   
   if (lpOverLapped == NULL) 
	return FALSE;

   Offset.u.LowPart = lpOverLapped->Offset;
   Offset.u.HighPart = lpOverLapped->OffsetHigh;
   lpOverLapped->Internal = STATUS_PENDING;
   IoStatusBlock = (PIO_STATUS_BLOCK)lpOverLapped;
   ptrOffset = &Offset;

   errCode = NtReadFile(hFile,
			NULL,
			ApcRoutine,
			lpCompletionRoutine,
			IoStatusBlock,
			lpBuffer,
			nNumberOfBytesToRead,
			ptrOffset,
			NULL);

   if (!NT_SUCCESS(errCode))  
     {
	SetLastErrorByStatus (errCode);
	return(FALSE);
     }
   return(TRUE);
}

/* EOF */
