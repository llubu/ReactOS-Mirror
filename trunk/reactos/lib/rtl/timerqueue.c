/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Timer Queue implementation
 * FILE:              lib/rtl/timerqueue.c
 * PROGRAMMER:        
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ***************************************************************/

typedef VOID (CALLBACK *WAITORTIMERCALLBACKFUNC) (PVOID, BOOLEAN );

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
RtlCreateTimer(HANDLE TimerQueue,
               PHANDLE phNewTimer,
	       WAITORTIMERCALLBACKFUNC Callback,
	       PVOID Parameter,
	       DWORD DueTime,
	       DWORD Period,
	       ULONG Flags)
{
  DPRINT1("RtlCreateTimer: stub\n");
  return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
NTSTATUS
STDCALL
RtlCreateTimerQueue(PHANDLE TimerQueue)
{
  DPRINT1("RtlCreateTimerQueue: stub\n");
  return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
NTSTATUS
STDCALL
RtlDeleteTimer(HANDLE TimerQueue,
               HANDLE Timer,
	       HANDLE CompletionEvent)
{
  DPRINT1("RtlDeleteTimer: stub\n");
  return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
NTSTATUS
STDCALL
RtlDeleteTimerQueue(HANDLE TimerQueue)
{
  DPRINT1("RtlDeleteTimerQueue: stub\n");
  return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
NTSTATUS
STDCALL
RtlDeleteTimerQueueEx(HANDLE TimerQueue,
                      HANDLE CompletionEvent)
{
  DPRINT1("RtlDeleteTimerQueueEx: stub\n");
  return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
NTSTATUS
STDCALL
RtlUpdateTimer(HANDLE TimerQueue,
               HANDLE Timer,
	       ULONG DueTime,
	       ULONG Period)
{
  DPRINT1("RtlUpdateTimer: stub\n");
  return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
