/* $Id: lock.c,v 1.9 2003/07/10 18:50:51 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/file.c
 * PURPOSE:         Directory functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 *		    GetTempFileName is modified from WINE [ Alexandre Juiliard ]
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/* FIXME: the large integer manipulations in this file dont handle overflow  */

/* INCLUDES ****************************************************************/

#include <k32.h>

#define NDEBUG
#include <kernel32/kernel32.h>

/* FUNCTIONS ****************************************************************/

/*
 * @implemented
 */
WINBOOL
STDCALL
LockFile(
	 HANDLE hFile,
	 DWORD dwFileOffsetLow,
	 DWORD dwFileOffsetHigh,
	 DWORD nNumberOfBytesToLockLow,
	 DWORD nNumberOfBytesToLockHigh
	 )
{	
	DWORD dwReserved;
	OVERLAPPED Overlapped;
   
	Overlapped.Offset = dwFileOffsetLow;
	Overlapped.OffsetHigh = dwFileOffsetHigh;
	dwReserved = 0;

  	return LockFileEx(hFile, LOCKFILE_FAIL_IMMEDIATELY|LOCKFILE_EXCLUSIVE_LOCK,dwReserved,nNumberOfBytesToLockLow, nNumberOfBytesToLockHigh, &Overlapped ) ;
 
}


/*
 * @implemented
 */
WINBOOL
STDCALL
LockFileEx(
	   HANDLE hFile,
	   DWORD dwFlags,
	   DWORD dwReserved,
	   DWORD nNumberOfBytesToLockLow,
	   DWORD nNumberOfBytesToLockHigh,
	   LPOVERLAPPED lpOverlapped
	   )
{
   LARGE_INTEGER BytesToLock;	
   BOOL LockImmediate;
   BOOL LockExclusive;
   NTSTATUS errCode;
   LARGE_INTEGER Offset;
   
   if(dwReserved != 0) 
     {      
	SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
     }
   
   lpOverlapped->Internal = STATUS_PENDING;  
   
   Offset.u.LowPart = lpOverlapped->Offset;
   Offset.u.HighPart = lpOverlapped->OffsetHigh;
   
   if ( (dwFlags & LOCKFILE_FAIL_IMMEDIATELY) == LOCKFILE_FAIL_IMMEDIATELY )
     LockImmediate = TRUE;
   else
     LockImmediate = FALSE;
   
   if ( (dwFlags & LOCKFILE_EXCLUSIVE_LOCK) == LOCKFILE_EXCLUSIVE_LOCK )
     LockExclusive = TRUE;
   else
     LockExclusive = FALSE;
   
   BytesToLock.u.LowPart = nNumberOfBytesToLockLow;
   BytesToLock.u.HighPart = nNumberOfBytesToLockHigh;
   
   errCode = NtLockFile(hFile,
			NULL,
			NULL,
			NULL,
			(PIO_STATUS_BLOCK)lpOverlapped,
			&Offset,
			&BytesToLock,
			NULL,
			LockImmediate,
			LockExclusive);
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
WINBOOL
STDCALL
UnlockFile(
	   HANDLE hFile,
	   DWORD dwFileOffsetLow,
	   DWORD dwFileOffsetHigh,
	   DWORD nNumberOfBytesToUnlockLow,
	   DWORD nNumberOfBytesToUnlockHigh
	   )
{
	DWORD dwReserved;
	OVERLAPPED Overlapped;
	Overlapped.Offset = dwFileOffsetLow;
	Overlapped.OffsetHigh = dwFileOffsetHigh;
	dwReserved = 0;
	return UnlockFileEx(hFile, dwReserved, nNumberOfBytesToUnlockLow, nNumberOfBytesToUnlockHigh, &Overlapped);

}


/*
 * @implemented
 */
WINBOOL 
STDCALL 
UnlockFileEx(
	HANDLE hFile,
	DWORD dwReserved,
	DWORD nNumberOfBytesToUnLockLow,
	DWORD nNumberOfBytesToUnLockHigh,
	LPOVERLAPPED lpOverlapped
	)
{
   LARGE_INTEGER BytesToUnLock;
   LARGE_INTEGER StartAddress;
   NTSTATUS errCode;
   
   if(dwReserved != 0) 
     {
	SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
     }
   if ( lpOverlapped == NULL ) 
     {
	SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
     }
   
   BytesToUnLock.u.LowPart = nNumberOfBytesToUnLockLow;
   BytesToUnLock.u.HighPart = nNumberOfBytesToUnLockHigh;
   
   StartAddress.u.LowPart = lpOverlapped->Offset;
   StartAddress.u.HighPart = lpOverlapped->OffsetHigh;
   
   errCode = NtUnlockFile(hFile,
			  (PIO_STATUS_BLOCK)lpOverlapped,
			  &StartAddress,
			  &BytesToUnLock,
			  NULL);
   if ( !NT_SUCCESS(errCode) ) {
      SetLastErrorByStatus (errCode);
      return FALSE;
   }
   
   return TRUE;
}

/* EOF */
