/* $Id: critical.c,v 1.9 2001/01/21 00:07:51 phreak Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/ntdll/rtl/critical.c
 * PURPOSE:         Critical sections
 * UPDATE HISTORY:
 *                  Created 30/09/98
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <ntdll/rtl.h>

#include <ntdll/ntdll.h>

/* FUNCTIONS *****************************************************************/

VOID STDCALL
RtlDeleteCriticalSection(LPCRITICAL_SECTION lpCriticalSection)
{
   NtClose(lpCriticalSection->LockSemaphore);
   lpCriticalSection->Reserved = -1;
}

VOID STDCALL
RtlEnterCriticalSection(LPCRITICAL_SECTION lpCriticalSection)
{
   HANDLE Thread = (HANDLE)NtCurrentTeb()->Cid.UniqueThread;
   ULONG ret;
 
   if (InterlockedIncrement(&lpCriticalSection->LockCount))
     {
	NTSTATUS Status;
	
	if (lpCriticalSection->OwningThread == Thread)
	  {
	     lpCriticalSection->RecursionCount++;
	     return;
	  }
	
//	DbgPrint("Entering wait for critical section\n");
	Status = NtWaitForSingleObject(lpCriticalSection->LockSemaphore, 
				       0, FALSE);
	if (!NT_SUCCESS(Status))
	  {
	     DbgPrint("RtlEnterCriticalSection: Failed to wait (Status %x)\n", 
		      Status);
	  }
//	DbgPrint("Left wait for critical section\n");
     }
   lpCriticalSection->OwningThread = Thread;
   lpCriticalSection->RecursionCount = 1;
   
#if 0
   if ((ret = InterlockedIncrement(&(lpCriticalSection->LockCount) )) != 1)
     {
	if (lpCriticalSection->OwningThread != Thread) 
	  {
	     NtWaitForSingleObject(lpCriticalSection->LockSemaphore, 
				   0, 
				   FALSE);
	     lpCriticalSection->OwningThread = Thread;
	  }
     }
   else
     {
	lpCriticalSection->OwningThread = Thread;
     }
   
   lpCriticalSection->RecursionCount++;
#endif
}

VOID STDCALL
RtlInitializeCriticalSection(LPCRITICAL_SECTION pcritical)
{
   NTSTATUS Status;
   
   pcritical->LockCount = -1;
   pcritical->RecursionCount = 0;
   Status = NtCreateSemaphore(&pcritical->LockSemaphore, 
			      STANDARD_RIGHTS_ALL, 
			      NULL, 
			      0, 
			      1);
   if (!NT_SUCCESS(Status))
     {
	DbgPrint("Failed to create semaphore (Status %x)\n", Status);
	/* FIXME: Throw exception */	
     }
   pcritical->OwningThread = (HANDLE)-1; // critical section has no owner yet
   pcritical->Reserved = 0;
}

VOID STDCALL
RtlLeaveCriticalSection(LPCRITICAL_SECTION lpCriticalSection)
{
   HANDLE Thread = (HANDLE)NtCurrentTeb()->Cid.UniqueThread;
   
   if (lpCriticalSection->OwningThread != Thread)
     {
	DbgPrint("Freeing critical section not owned\n");
	return;
     }
   
   lpCriticalSection->RecursionCount--;
   if (lpCriticalSection->RecursionCount > 0)
     {
	InterlockedDecrement(&lpCriticalSection->LockCount);
	return;
     }
   lpCriticalSection->OwningThread = 0;
   if (InterlockedIncrement(&lpCriticalSection->LockCount) >= 0)
     {
	NTSTATUS Status;
	
	Status = NtReleaseSemaphore(lpCriticalSection->LockSemaphore, 1, NULL);
	if (!NT_SUCCESS(Status))
	  {
	     DbgPrint("Failed to release semaphore (Status %x)\n",
		      Status);
	  }
     }
   
#if 0
   lpCriticalSection->RecursionCount--;
   if (lpCriticalSection->RecursionCount == 0)
     {
	lpCriticalSection->OwningThread = (HANDLE)-1;
	// if LockCount > 0 and RecursionCount == 0 there
	// is a waiting thread 
	// ReleaseSemaphore will fire up a waiting thread
	if (InterlockedDecrement(&lpCriticalSection->LockCount) > 0)
	  {
	     NtReleaseSemaphore( lpCriticalSection->LockSemaphore,1,NULL);
	  }
     }
   else InterlockedDecrement( &lpCriticalSection->LockCount );
#endif
}

BOOLEAN STDCALL
RtlTryEnterCriticalSection(LPCRITICAL_SECTION lpCriticalSection)
{
   if (InterlockedCompareExchange((PVOID*)&lpCriticalSection->LockCount, 
				  (PVOID)1, (PVOID)0 ) == 0)
     {
	lpCriticalSection->OwningThread = 
	  (HANDLE) NtCurrentTeb()->Cid.UniqueThread;
	lpCriticalSection->RecursionCount++;
	return TRUE;
     }
   if (lpCriticalSection->OwningThread == 
       (HANDLE)NtCurrentTeb()->Cid.UniqueThread)
     {
	lpCriticalSection->RecursionCount++;
	return TRUE;
     }
   return FALSE;
}


/* EOF */





