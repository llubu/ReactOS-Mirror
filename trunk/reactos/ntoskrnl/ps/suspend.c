/* $Id: suspend.c,v 1.2 2001/01/21 14:54:30 dwelch Exp $
 *
 * COPYRIGHT:              See COPYING in the top level directory
 * PROJECT:                ReactOS kernel
 * FILE:                   ntoskrnl/ps/suspend.c
 * PURPOSE:                Thread managment
 * PROGRAMMER:             David Welch (welch@mcmail.com)
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <internal/ke.h>
#include <internal/ob.h>
#include <internal/hal.h>
#include <internal/ps.h>
#include <internal/ob.h>

#define NDEBUG
#include <internal/debug.h>

/* NOTES **********************************************************************
 *
 */

/* FUNCTIONS *****************************************************************/

VOID
PiSuspendThreadRundownRoutine(PKAPC Apc)
{
   ExFreePool(Apc);
}

VOID
PiSuspendThreadKernelRoutine(PKAPC Apc,
			     PKNORMAL_ROUTINE* NormalRoutine,
			     PVOID* NormalContext,
			     PVOID* SystemArgument1,
			     PVOID* SystemArguemnt2)
{
   KeWaitForSingleObject(&PsGetCurrentThread()->Tcb.SuspendSemaphore,
			 0,
			 UserMode,
			 TRUE,
			 NULL);
   ExFreePool(Apc);
}

NTSTATUS
PsResumeThread(PETHREAD Thread, PULONG SuspendCount)
{
  KeReleaseSemaphore(&Thread->Tcb.SuspendSemaphore, IO_NO_INCREMENT, 1, FALSE);
  return(STATUS_SUCCESS);
}

NTSTATUS 
PsSuspendThread(PETHREAD Thread, PULONG PreviousSuspendCount)
{
   PKAPC Apc;
   NTSTATUS Status;

   /*
    * If we are suspending ourselves then we can cut out the work in
    * sending an APC
    */
   if (Thread == PsGetCurrentThread())
     {
       Status = KeWaitForSingleObject(&Thread->Tcb.SuspendSemaphore,
				      0,
				      UserMode,
				      FALSE,
				      NULL);
       if (!NT_SUCCESS(Status))
	 {
	   return(Status);
	 }
       return(Status);
     }

   Apc = ExAllocatePool(NonPagedPool, sizeof(KAPC));
   if (Apc == NULL)
     {
	return(STATUS_NO_MEMORY);
     }
   
   KeInitializeApc(Apc,
		   &Thread->Tcb,
		   0,
		   PiSuspendThreadKernelRoutine,
		   PiSuspendThreadRundownRoutine,
		   NULL,
		   KernelMode,
		   NULL);
   KeInsertQueueApc(Apc,
		    NULL,
		    NULL,
		    0);
   return(STATUS_SUCCESS);
}

NTSTATUS STDCALL 
NtResumeThread (IN	HANDLE	ThreadHandle,
		IN	PULONG	SuspendCount)
/*
 * FUNCTION: Decrements a thread's resume count
 * ARGUMENTS: 
 *        ThreadHandle = Handle to the thread that should be resumed
 *        ResumeCount =  The resulting resume count.
 * RETURNS: Status
 */
{
   PETHREAD Thread;
   NTSTATUS Status;
   ULONG Count;

   Status = ObReferenceObjectByHandle(ThreadHandle,
				      THREAD_SUSPEND_RESUME,
				      PsThreadType,
				      UserMode,
				      (PVOID*)&Thread,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }

   Status = PsResumeThread(Thread, &Count);
   if (SuspendCount != NULL)
     {
	*SuspendCount = Count;
     }

   ObDereferenceObject((PVOID)Thread);

   return STATUS_SUCCESS;
}


NTSTATUS STDCALL 
NtSuspendThread (IN HANDLE ThreadHandle,
		 IN PULONG PreviousSuspendCount)
/*
 * FUNCTION: Increments a thread's suspend count
 * ARGUMENTS: 
 *        ThreadHandle = Handle to the thread that should be resumed
 *        PreviousSuspendCount =  The resulting/previous suspend count.
 * REMARK:
 *        A thread will be suspended if its suspend count is greater than 0. 
 *        This procedure maps to the win32 SuspendThread function. ( 
 *        documentation about the the suspend count can be found here aswell )
 *        The suspend count is not increased if it is greater than 
 *        MAXIMUM_SUSPEND_COUNT.
 * RETURNS: Status
 */ 
{
   PETHREAD Thread;
   NTSTATUS Status;
   ULONG Count;

   Status = ObReferenceObjectByHandle(ThreadHandle,
				      THREAD_SUSPEND_RESUME,
				      PsThreadType,
				      UserMode,
				      (PVOID*)&Thread,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }

   Status = PsSuspendThread(Thread, &Count);
   if (PreviousSuspendCount != NULL)
     {
	*PreviousSuspendCount = Count;
     }

   ObDereferenceObject((PVOID)Thread);

   return STATUS_SUCCESS;
}





