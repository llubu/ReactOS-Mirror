/*
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS project
 * FILE:                 ntoskrnl/ke/wait.c
 * PURPOSE:              Manages non-busy waiting
 * PROGRAMMER:           David Welch (welch@mcmail.com)
 * REVISION HISTORY:
 *           21/07/98: Created
 */

/* NOTES ********************************************************************
 * 
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <internal/ke.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS ******************************************************************/

static KSPIN_LOCK DispatcherDatabaseLock;
static BOOLEAN WaitSet = FALSE;
static KIRQL oldlvl = PASSIVE_LEVEL;
static PKTHREAD Owner = NULL; 

/* FUNCTIONS *****************************************************************/

VOID KeInitializeDispatcherHeader(DISPATCHER_HEADER* Header,
				  ULONG Type,
				  ULONG Size,
				  ULONG SignalState)
{
   Header->Type = Type;
   Header->Absolute = 0;
   Header->Inserted = 0;
   Header->Size = Size;
   Header->SignalState = SignalState;
   InitializeListHead(&(Header->WaitListHead));
}

VOID KeAcquireDispatcherDatabaseLock(BOOLEAN Wait)
/*
 * PURPOSE: Acquires the dispatcher database lock for the caller
 */
{
   DPRINT("KeAcquireDispatcherDatabaseLock(Wait %x)\n",Wait);
   if (WaitSet && Owner == KeGetCurrentThread())
     {
	return;
     }
   KeAcquireSpinLock(&DispatcherDatabaseLock,&oldlvl);
   WaitSet = Wait;
   Owner = KeGetCurrentThread();
}

VOID KeReleaseDispatcherDatabaseLock(BOOLEAN Wait)
{
   DPRINT("KeReleaseDispatcherDatabaseLock(Wait %x)\n",Wait);  
   assert(Wait==WaitSet);
   if (!Wait)
     {
	Owner = NULL;
	KeReleaseSpinLock(&DispatcherDatabaseLock, oldlvl);
     }
}

static BOOLEAN KeDispatcherObjectWakeAll(DISPATCHER_HEADER* hdr)
{
   PKWAIT_BLOCK current;
   PLIST_ENTRY current_entry;
   PKWAIT_BLOCK PrevBlock;

   DPRINT("KeDispatcherObjectWakeAll(hdr %x)\n",hdr);

   if (IsListEmpty(&hdr->WaitListHead))
     {
	return(FALSE);
     }
   
   while (!IsListEmpty(&(hdr->WaitListHead)))
     {
	current_entry = RemoveHeadList(&hdr->WaitListHead);
	current = CONTAINING_RECORD(current_entry,KWAIT_BLOCK,
					    WaitListEntry);
        DPRINT("Waking %x\n",current->Thread);

        if (current->WaitType == WaitAny)
          {
             DPRINT("WaitAny: Remove all wait blocks.\n");
             current->Thread->WaitBlockList = NULL;
          }
        else
          {
             DPRINT("WaitAll: Remove the current wait block only.\n");

             PrevBlock = current->Thread->WaitBlockList;
             if (PrevBlock)
               {
                  if (PrevBlock->NextWaitBlock == current)
                    {
                       DPRINT("WaitAll: Current block is list head.\n");
                       PrevBlock->NextWaitBlock = current->NextWaitBlock;
                    }
                  else
                    {
                       DPRINT("WaitAll: Current block is list head.\n");
                       while (PrevBlock &&
                              PrevBlock->NextWaitBlock != current)
                         {
                            PrevBlock = PrevBlock->NextWaitBlock;
                         }

                       if (PrevBlock)
                         {
                            PrevBlock->NextWaitBlock = current->NextWaitBlock;
                         }
                    }
               }
             else
               {
                  DPRINT("WaitAll: Wait Block List is empty!\n");
               }
          }

	PsResumeThread(CONTAINING_RECORD(current->Thread,ETHREAD,Tcb));
     };
   return(TRUE);
}

static BOOLEAN KeDispatcherObjectWakeOne(DISPATCHER_HEADER* hdr)
{
   PKWAIT_BLOCK current;
   PLIST_ENTRY current_entry;
   PKWAIT_BLOCK PrevBlock;

   DPRINT("KeDispatcherObjectWakeOn(hdr %x)\n",hdr);
   DPRINT("hdr->WaitListHead.Flink %x hdr->WaitListHead.Blink %x\n",
	  hdr->WaitListHead.Flink,hdr->WaitListHead.Blink);
   if (IsListEmpty(&(hdr->WaitListHead)))
     {
	return(FALSE);
     }
   current_entry = RemoveHeadList(&(hdr->WaitListHead));
   current = CONTAINING_RECORD(current_entry,KWAIT_BLOCK,
			       WaitListEntry);
   DPRINT("current_entry %x current %x\n",current_entry,current);


   if (current->WaitType == WaitAny)
     {
        DPRINT("WaitAny: Remove all wait blocks.\n");
        current->Thread->WaitBlockList = NULL;
     }
   else
     {
        DPRINT("WaitAll: Remove the current wait block only.\n");

        PrevBlock = current->Thread->WaitBlockList;
        if (PrevBlock)
          {
             if (PrevBlock->NextWaitBlock == current)
               {
                  DPRINT("WaitAll: Current block is list head.\n");
                  PrevBlock->NextWaitBlock = current->NextWaitBlock;
               }
             else
               {
                  DPRINT("WaitAll: Current block is list head.\n");
                  while (PrevBlock && PrevBlock->NextWaitBlock != current)
                    {
                       PrevBlock = PrevBlock->NextWaitBlock;
                    }

                  if (PrevBlock)
                    {
                       PrevBlock->NextWaitBlock = current->NextWaitBlock;
                    }
               }
          }
         else
           {
              DPRINT("WaitAll: Wait Block List is empty!\n");
           }
     }

   DPRINT("Waking %x\n",current->Thread);
   
   if (hdr->Type == SemaphoreType)
      hdr->SignalState--;
   
   PsResumeThread(CONTAINING_RECORD(current->Thread,ETHREAD,Tcb));
   return(TRUE);
}

BOOLEAN KeDispatcherObjectWake(DISPATCHER_HEADER* hdr)
/*
 * FUNCTION: Wake threads waiting on a dispatcher object
 * NOTE: The exact semantics of waking are dependant on the type of object
 */
{
   BOOL Ret;

   DPRINT("Entering KeDispatcherObjectWake(hdr %x)\n",hdr);
//   DPRINT("hdr->WaitListHead %x hdr->WaitListHead.Flink %x\n",
//	  &hdr->WaitListHead,hdr->WaitListHead.Flink);
   DPRINT("hdr->Type %x\n",hdr->Type);
   switch (hdr->Type)
     {
      case NotificationEvent:
	return(KeDispatcherObjectWakeAll(hdr));

      case SynchronizationEvent:
	Ret = KeDispatcherObjectWakeOne(hdr);
	if (Ret)
	  {
	     hdr->SignalState = FALSE;
	  }
	return(Ret);

      case SemaphoreType:
	if(hdr->SignalState>0)
	{
          do
          {
     	    Ret = KeDispatcherObjectWakeOne(hdr);
          } while(hdr->SignalState > 0 &&  Ret) ;
	  return(Ret);
	}
	else return FALSE;
	
      case ID_PROCESS_OBJECT:
	return(KeDispatcherObjectWakeAll(hdr));

      case ID_THREAD_OBJECT:
	return(KeDispatcherObjectWakeAll(hdr));
     }
   DPRINT("Dispatcher object %x has unknown type\n",hdr);
   KeBugCheck(0);
   return(FALSE);
}


NTSTATUS KeWaitForSingleObject(PVOID Object,
			       KWAIT_REASON WaitReason,
			       KPROCESSOR_MODE WaitMode,
			       BOOLEAN Alertable,
			       PLARGE_INTEGER Timeout)
/*
 * FUNCTION: Puts the current thread into a wait state until the
 * given dispatcher object is set to signalled 
 * ARGUMENTS:
 *         Object = Object to wait on
 *         WaitReason = Reason for the wait (debugging aid)
 *         WaitMode = Can be KernelMode or UserMode, if UserMode then
 *                    user-mode APCs can be delivered and the thread's
 *                    stack can be paged out
 *         Altertable = Specifies if the wait is a alertable
 *         Timeout = Optional timeout value
 * RETURNS: Status
 */
{
   DISPATCHER_HEADER* hdr = (DISPATCHER_HEADER *)Object;
   KWAIT_BLOCK blk;
   PKTHREAD CurrentThread;

   DPRINT("Entering KeWaitForSingleObject(Object %x) "
	  "PsGetCurrentThread() %x\n",Object,PsGetCurrentThread());

   CurrentThread = KeGetCurrentThread();

   KeAcquireDispatcherDatabaseLock(FALSE);

   DPRINT("hdr->SignalState %d\n", hdr->SignalState);

   if (hdr->SignalState > 0)
   {
      switch (hdr->Type)
	{
	 case SynchronizationEvent:
	   hdr->SignalState = FALSE;
	   break;
	   
	 case SemaphoreType:
	   break;
	   
	 case ID_PROCESS_OBJECT:
	   break;

	 case ID_THREAD_OBJECT:
	   break;
	   
	 case NotificationEvent:
	   break;
	   
	 default:
	   DbgPrint("(%s:%d) Dispatcher object %x has unknown type\n",
		    __FILE__,__LINE__,hdr);
	   KeBugCheck(0);
	   
	}
      KeReleaseDispatcherDatabaseLock(FALSE);
      return(STATUS_WAIT_0);
   }

   if (Timeout != NULL)
     {
        KeAddThreadTimeout(CurrentThread,Timeout);
     }

   /* Append wait block to the KTHREAD wait block list */
   CurrentThread->WaitBlockList = &blk;

   blk.Object = Object;
   blk.Thread = CurrentThread;
   blk.WaitKey = 0;
   blk.WaitType = WaitAny;
   blk.NextWaitBlock = NULL;
   InsertTailList(&(hdr->WaitListHead),&(blk.WaitListEntry));
//   DPRINT("hdr->WaitListHead.Flink %x hdr->WaitListHead.Blink %x\n",
//          hdr->WaitListHead.Flink,hdr->WaitListHead.Blink);
   KeReleaseDispatcherDatabaseLock(FALSE);
   DPRINT("Waiting at %s:%d with irql %d\n", __FILE__, __LINE__, 
	  KeGetCurrentIrql());
   PsSuspendThread(PsGetCurrentThread());
   
   if (Timeout != NULL)
     {
	KeCancelTimer(&KeGetCurrentThread()->Timer);
        if (KeReadStateTimer(&KeGetCurrentThread()->Timer))
            return(STATUS_TIMEOUT);
     }
   DPRINT("Returning from KeWaitForSingleObject()\n");
   return(STATUS_WAIT_0);
}

NTSTATUS KeWaitForMultipleObjects(ULONG Count,
				  PVOID Object[],
				  WAIT_TYPE WaitType,
				  KWAIT_REASON WaitReason,
				  KPROCESSOR_MODE WaitMode,
				  BOOLEAN Alertable,
				  PLARGE_INTEGER Timeout,
				  PKWAIT_BLOCK WaitBlockArray)
{
    DISPATCHER_HEADER* hdr;
    PKWAIT_BLOCK blk;
    PKTHREAD CurrentThread;
    ULONG CountSignaled;
    ULONG i;

    DPRINT("Entering KeWaitForMultipleObjects(Count %lu Object[] %p) "
    "PsGetCurrentThread() %x\n",Count,Object,PsGetCurrentThread());

    CountSignaled = 0;
    CurrentThread = KeGetCurrentThread();

    if (WaitBlockArray == NULL)
    {
        if (Count > 3)
        {
            DbgPrint("(%s:%d) Too many objects!\n",
                     __FILE__,__LINE__);
            return STATUS_UNSUCCESSFUL;
        }
        blk = &CurrentThread->WaitBlock[1];
    }
    else
    {
        if (Count > 64)
        {
            DbgPrint("(%s:%d) Too many objects!\n",
                     __FILE__,__LINE__);
            return STATUS_UNSUCCESSFUL;
        }
        blk = WaitBlockArray;
    }

    KeAcquireDispatcherDatabaseLock(FALSE);

    for (i = 0; i < Count; i++)
    {
        hdr = (DISPATCHER_HEADER *)Object[i];

        DPRINT("hdr->SignalState %d\n", hdr->SignalState);

        if (hdr->SignalState > 0)
        {
            CountSignaled++;

            switch (hdr->Type)
            {
                case SynchronizationEvent:
                    hdr->SignalState = FALSE;
                    break;

                case SemaphoreType:
                    break;

                case ID_PROCESS_OBJECT:
                    break;

                case ID_THREAD_OBJECT:
                    break;

                case NotificationEvent:
                    break;

                default:
                    DbgPrint("(%s:%d) Dispatcher object %x has unknown type\n",
                             __FILE__,__LINE__,hdr);
                    KeBugCheck(0);
            }

            if (WaitType == WaitAny)
            {
                KeReleaseDispatcherDatabaseLock(FALSE);
                DPRINT("One object is already signaled!\n");
                return(STATUS_WAIT_0 + i);
            }
        }
    }

    if ((WaitType == WaitAll) && (CountSignaled == Count))
    {
        KeReleaseDispatcherDatabaseLock(FALSE);
        DPRINT("All objects are already signaled!\n");
        return(STATUS_WAIT_0);
    }

    if (Timeout != NULL)
    {
        KeAddThreadTimeout(CurrentThread,Timeout);
    }

    /* Append wait block to the KTHREAD wait block list */
    CurrentThread->WaitBlockList = blk;

    for (i = 0; i < Count; i++)
    {
        hdr = (DISPATCHER_HEADER *)Object[i];

        DPRINT("hdr->SignalState %d\n", hdr->SignalState);

        blk->Object = Object[i];
        blk->Thread = CurrentThread;
        blk->WaitKey = i;
        blk->WaitType = WaitType;
        if (i == Count - 1)
            blk->NextWaitBlock = NULL;
        else
            blk->NextWaitBlock = (PVOID)((ULONG)blk+sizeof(KWAIT_BLOCK));
        DPRINT("blk %p blk->NextWaitBlock %p\n",
               blk, blk->NextWaitBlock);

        InsertTailList(&(hdr->WaitListHead),&(blk->WaitListEntry));
//        DPRINT("hdr->WaitListHead.Flink %x hdr->WaitListHead.Blink %x\n",
//               hdr->WaitListHead.Flink,hdr->WaitListHead.Blink);

        blk = blk->NextWaitBlock;
    }

    KeReleaseDispatcherDatabaseLock(FALSE);

    DPRINT("Waiting at %s:%d with irql %d\n", __FILE__, __LINE__, 
           KeGetCurrentIrql());
    PsSuspendThread(PsGetCurrentThread());
   
    if (Timeout != NULL)
    {
	KeCancelTimer(&KeGetCurrentThread()->Timer);
        if (KeReadStateTimer(&KeGetCurrentThread()->Timer))
            return(STATUS_TIMEOUT);
    }

    DPRINT("Returning from KeWaitForMultipleObjects()\n");

    if (WaitType == WaitAny)
    {
        for (i = 0; i < Count; i++)
        {
            if (((DISPATCHER_HEADER *)Object[i])->SignalState)
                return(STATUS_WAIT_0+i);
        }
    }

    return(STATUS_WAIT_0);
}

VOID KeInitializeDispatcher(VOID)
{
   KeInitializeSpinLock(&DispatcherDatabaseLock);
}

NTSTATUS STDCALL NtWaitForMultipleObjects(IN ULONG Count,
					  IN HANDLE Object [],
					  IN CINT WaitType,
					  IN BOOLEAN Alertable,
					  IN PLARGE_INTEGER Time)
{
   KWAIT_BLOCK WaitBlockArray[64]; /* FIXME: use MAXIMUM_WAIT_OBJECTS instead */
   PVOID ObjectPtrArray[64];       /* FIXME: use MAXIMUM_WAIT_OBJECTS instead */
   NTSTATUS Status;
   ULONG i, j;

   DPRINT("NtWaitForMultipleObjects(Count %lu Object[] %x, Alertable %d, Time %x)\n",
          Count,Object,Alertable,Time);

   if (Count > 64)  /* FIXME: use MAXIMUM_WAIT_OBJECTS instead */
        return STATUS_UNSUCCESSFUL;

   /* reference all objects */
   for (i = 0; i < Count; i++)
     {
        Status = ObReferenceObjectByHandle(Object[i],
                                           SYNCHRONIZE,
                                           NULL,
                                           UserMode,
                                           &ObjectPtrArray[i],
                                           NULL);
        if (Status != STATUS_SUCCESS)
          {
             /* dereference all referenced objects */
             for (j = 0; j < i; i++)
               {
                  ObDereferenceObject(ObjectPtrArray[j]);
               }

             return(Status);
          }
     }

   Status = KeWaitForMultipleObjects(Count,
                                     ObjectPtrArray,
                                     WaitType,
                                     UserRequest,
                                     UserMode,
                                     Alertable,
                                     Time,
                                     WaitBlockArray);

   /* dereference all objects */
   for (i = 0; i < Count; i++)
     {
        ObDereferenceObject(ObjectPtrArray[i]);
     }

   return(Status);
}


NTSTATUS
STDCALL
NtWaitForSingleObject (
	IN	HANDLE		Object,
	IN	BOOLEAN		Alertable,
	IN	PLARGE_INTEGER	Time
	)
{
   PVOID ObjectPtr;
   NTSTATUS Status;
   
   DPRINT("NtWaitForSingleObject(Object %x, Alertable %d, Time %x)\n",
	  Object,Alertable,Time);
   
   Status = ObReferenceObjectByHandle(Object,
				      SYNCHRONIZE,
				      NULL,
				      UserMode,
				      &ObjectPtr,
				      NULL);
   if (Status != STATUS_SUCCESS)
     {
	return(Status);
     }
   
   Status = KeWaitForSingleObject(ObjectPtr,
				  UserMode,
				  UserMode,
				  Alertable,
				  Time);
   
   ObDereferenceObject(ObjectPtr);
   
   return(Status);
}


NTSTATUS
STDCALL
NtSignalAndWaitForSingleObject (
	IN	HANDLE		EventHandle,
	IN	BOOLEAN		Alertable,
	IN	PLARGE_INTEGER	Time,
		PULONG		NumberOfWaitingThreads	OPTIONAL
	)
{
	UNIMPLEMENTED;
}
