/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/event.c
 * PURPOSE:         Implements events
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS ****************************************************************/

/*
 * @implemented
 */
VOID STDCALL KeClearEvent (PKEVENT Event)
{
   DPRINT("KeClearEvent(Event %x)\n", Event);
   Event->Header.SignalState = FALSE;
}

/*
 * @implemented
 */
VOID STDCALL KeInitializeEvent (PKEVENT		Event,
				EVENT_TYPE	Type,
				BOOLEAN		State)
{
   ULONG IType;
   
   if (Type == NotificationEvent)
     {
	IType = InternalNotificationEvent;
     }
   else if (Type == SynchronizationEvent)
     {
	IType = InternalSynchronizationEvent;
     }
   else
     {
	assert(FALSE);
	return;
     }
   
   KeInitializeDispatcherHeader(&(Event->Header),
				IType,
				sizeof(Event)/sizeof(ULONG),State);
   InitializeListHead(&(Event->Header.WaitListHead));
}

/*
 * @implemented
 */
LONG STDCALL KeReadStateEvent (PKEVENT Event)
{
   return(Event->Header.SignalState);
}

/*
 * @implemented
 */
LONG STDCALL KeResetEvent (PKEVENT Event)
{
   return(InterlockedExchange(&(Event->Header.SignalState),0));
}

/*
 * @implemented
 */
LONG STDCALL KeSetEvent (PKEVENT		Event,
			 KPRIORITY	Increment,
			 BOOLEAN		Wait)
{
  KIRQL OldIrql;
  int ret;

  DPRINT("KeSetEvent(Event %x, Wait %x)\n",Event,Wait);

  OldIrql = KeAcquireDispatcherDatabaseLock();

  ret = InterlockedExchange(&(Event->Header.SignalState),1);

  KeDispatcherObjectWake((DISPATCHER_HEADER *)Event);

  if (Wait == FALSE)
    {
      KeReleaseDispatcherDatabaseLock(OldIrql);
    }
  else
    {
      KTHREAD *Thread = KeGetCurrentThread();
      Thread->WaitNext = Wait;
      Thread->WaitIrql = OldIrql;
    }

  return(ret);
}

/*
 * @implemented
 */
NTSTATUS STDCALL KePulseEvent (PKEVENT		Event,
			       KPRIORITY	Increment,
			       BOOLEAN		Wait)
{
   KIRQL OldIrql;
   int ret;

   DPRINT("KePulseEvent(Event %x, Wait %x)\n",Event,Wait);
   OldIrql = KeAcquireDispatcherDatabaseLock();
   ret = InterlockedExchange(&(Event->Header.SignalState),1);
   KeDispatcherObjectWake((DISPATCHER_HEADER *)Event);
   InterlockedExchange(&(Event->Header.SignalState),0);

  if (Wait == FALSE)
    {
      KeReleaseDispatcherDatabaseLock(OldIrql);
    }
  else
    {
      KTHREAD *Thread = KeGetCurrentThread();
      Thread->WaitNext = Wait;
      Thread->WaitIrql = OldIrql;
    }

   return ((NTSTATUS)ret);
}

/*
 * @unimplemented
 */
STDCALL
VOID
KeSetEventBoostPriority(
	IN PKEVENT Event,
	IN PKTHREAD *Thread OPTIONAL
)
{
	UNIMPLEMENTED;
}

/* EOF */
