/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/iocompl.c
 * PURPOSE:         Io Completion functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

#include <k32.h>
#define NDEBUG
#include <debug.h>

#define NANOS_TO_100NS(nanos) (((LONGLONG)(nanos)) / 100)
#define MICROS_TO_100NS(micros) (((LONGLONG)(micros)) * NANOS_TO_100NS(1000))
#define MILLIS_TO_100NS(milli) (((LONGLONG)(milli)) * MICROS_TO_100NS(1000))

/*
 * @implemented
 */
HANDLE
WINAPI
CreateIoCompletionPort(
    HANDLE FileHandle,
    HANDLE ExistingCompletionPort,
    ULONG_PTR CompletionKey,
    DWORD NumberOfConcurrentThreads
    )
{
   HANDLE CompletionPort = NULL;
   NTSTATUS errCode;
   FILE_COMPLETION_INFORMATION CompletionInformation;
   IO_STATUS_BLOCK IoStatusBlock;

   if ( FileHandle == INVALID_HANDLE_VALUE && ExistingCompletionPort != NULL )
   {
      SetLastError(ERROR_INVALID_PARAMETER);
      return FALSE;
   }

   if ( ExistingCompletionPort != NULL )
   {
      CompletionPort = ExistingCompletionPort;
   }
   else
   {

      errCode = NtCreateIoCompletion(&CompletionPort,
                                     IO_COMPLETION_ALL_ACCESS,
                                     NULL,//ObjectAttributes
                                     NumberOfConcurrentThreads);

      if (!NT_SUCCESS(errCode) )
      {
         SetLastErrorByStatus (errCode);
         return FALSE;
      }

   }

   if ( FileHandle != INVALID_HANDLE_VALUE )
   {
      CompletionInformation.Port = CompletionPort;
      CompletionInformation.Key  = (PVOID)CompletionKey;

      errCode = NtSetInformationFile(FileHandle,
                                     &IoStatusBlock,
                                     &CompletionInformation,
                                     sizeof(FILE_COMPLETION_INFORMATION),
                                     FileCompletionInformation);

      if ( !NT_SUCCESS(errCode) )
      {
         if ( ExistingCompletionPort == NULL )
         {
            NtClose(CompletionPort);
         }

         SetLastErrorByStatus (errCode);
         return FALSE;
      }
   }

   return CompletionPort;
}


/*
 * @implemented
 */
BOOL
WINAPI
GetQueuedCompletionStatus(
   HANDLE CompletionHandle,
   LPDWORD lpNumberOfBytesTransferred,
   PULONG_PTR lpCompletionKey,
   LPOVERLAPPED *lpOverlapped,
   DWORD dwMilliseconds
   )
{
   NTSTATUS errCode;
   IO_STATUS_BLOCK IoStatus;
   ULONG_PTR CompletionKey;
   LARGE_INTEGER Interval;

   if (!lpNumberOfBytesTransferred || !lpCompletionKey || !lpOverlapped)
   {
      SetLastError(ERROR_INVALID_PARAMETER);
      return FALSE;
   }

   if (dwMilliseconds != INFINITE)
   {
      Interval.QuadPart = (-(MILLIS_TO_100NS(dwMilliseconds)));
   }

   errCode = NtRemoveIoCompletion(CompletionHandle,
                                  (PVOID*)&CompletionKey,
                                  (PVOID*)lpOverlapped,
                                  &IoStatus,
                                  dwMilliseconds == INFINITE ? NULL : &Interval);

   if (!NT_SUCCESS(errCode) || errCode == STATUS_TIMEOUT) {
      *lpOverlapped = NULL;
      SetLastErrorByStatus(errCode);
      return FALSE;
   }

   *lpCompletionKey = CompletionKey;
   *lpNumberOfBytesTransferred = IoStatus.Information;

   if (!NT_SUCCESS(IoStatus.Status)){
      //failed io operation
      SetLastErrorByStatus(IoStatus.Status);
      return FALSE;
   }

   return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
PostQueuedCompletionStatus(
   HANDLE CompletionHandle,
   DWORD dwNumberOfBytesTransferred,
   ULONG_PTR dwCompletionKey,
   LPOVERLAPPED lpOverlapped
   )
{
   NTSTATUS errCode;

   errCode = NtSetIoCompletion(CompletionHandle,
                               (PVOID)dwCompletionKey,      // KeyContext
                               (PVOID)lpOverlapped,         // ApcContext
                               STATUS_SUCCESS,              // IoStatusBlock->Status
                               dwNumberOfBytesTransferred); // IoStatusBlock->Information

   if ( !NT_SUCCESS(errCode) )
   {
      SetLastErrorByStatus (errCode);
      return FALSE;
   }
   return TRUE;
}


/*
 * @implemented
 */
BOOL WINAPI
CancelIo(HANDLE hFile)
{
  IO_STATUS_BLOCK IoStatusBlock;
  NTSTATUS Status;

  Status = NtCancelIoFile(hFile,
			  &IoStatusBlock);
  if (!NT_SUCCESS(Status))
    {
      SetLastErrorByStatus(Status);
      return(FALSE);
    }

  return(TRUE);
}


/*
 * @unimplemented
 */
BOOL
WINAPI
CancelIoEx(IN HANDLE hFile,
           IN LPOVERLAPPED lpOverlapped)
{
    UNIMPLEMENTED;
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
CancelSynchronousIo(IN HANDLE hThread)
{
    UNIMPLEMENTED;
    return FALSE;
}

/* EOF */
