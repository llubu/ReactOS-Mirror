/* $Id: critical.c,v 1.8 2000/04/20 05:28:31 phreak Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/sync/critical.c
 * PURPOSE:         Critical regions
 * UPDATE HISTORY:
 *                  Created 30/09/98
 */

/* INCLUDES ******************************************************************/

#include <windows.h>

#include <kernel32/kernel32.h>

/* FUNCTIONS *****************************************************************/


VOID
STDCALL
DeleteCriticalSection(LPCRITICAL_SECTION lpCriticalSection)
{
   CloseHandle(lpCriticalSection->LockSemaphore);
   lpCriticalSection->Reserved = -1;
}


VOID
STDCALL
EnterCriticalSection(LPCRITICAL_SECTION lpCriticalSection)
{
   if( InterlockedIncrement(&(lpCriticalSection->LockCount) )  != 1 ) {
      if (lpCriticalSection->OwningThread != (HANDLE)GetCurrentThreadId() ) {
	 WaitForSingleObject(lpCriticalSection->LockSemaphore,100000);
	 // WAIT_TIMEOUT should give message if DEBUG
	 lpCriticalSection->OwningThread = (HANDLE)GetCurrentThreadId();
      }
   }
   else
      lpCriticalSection->OwningThread = (HANDLE)GetCurrentThreadId();
   
   lpCriticalSection->RecursionCount++;
}


VOID
STDCALL
InitializeCriticalSection(LPCRITICAL_SECTION pcritical)
{
   pcritical->LockCount = -1;
   pcritical->RecursionCount = 0;
   pcritical->LockSemaphore = CreateSemaphoreW(NULL,0,100,NULL);
   pcritical->OwningThread = (HANDLE)-1; // critical section has no owner yet
   pcritical->Reserved = 0;
}


VOID
STDCALL
LeaveCriticalSection(LPCRITICAL_SECTION lpCriticalSection)
{
   lpCriticalSection->RecursionCount--;
   if ( lpCriticalSection->RecursionCount == 0 ) {
      lpCriticalSection->OwningThread = (HANDLE)-1;
      // if LockCount > 0 and RecursionCount == 0 there
      // is a waiting thread 
      // ReleaseSemaphore will fire up a waiting thread
      if ( InterlockedDecrement( &lpCriticalSection->LockCount ) > 0 )
	 ReleaseSemaphore( lpCriticalSection->LockSemaphore,1,NULL);
   }
   else InterlockedDecrement( &lpCriticalSection->LockCount );
}


WINBOOL
STDCALL
TryEnterCriticalSection(LPCRITICAL_SECTION lpCriticalSection)
{
   if( InterlockedCompareExchange( (PVOID *)&lpCriticalSection->LockCount, (PVOID)1, (PVOID)0 ) == 0 )
      {
	 lpCriticalSection->OwningThread = (HANDLE)GetCurrentThreadId();
	 lpCriticalSection->RecursionCount++;
	 return TRUE;
      }
   if( lpCriticalSection->OwningThread == (HANDLE)GetCurrentThreadId() )
      {
	 lpCriticalSection->RecursionCount++;
	 return TRUE;
      }
   return FALSE;
}


/* EOF */
