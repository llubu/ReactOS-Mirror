/* $Id: create.c,v 1.75 2004/07/13 11:48:32 ekohl Exp $
 *
 * COPYRIGHT:              See COPYING in the top level directory
 * PROJECT:                ReactOS kernel
 * FILE:                   ntoskrnl/ps/thread.c
 * PURPOSE:                Thread managment
 * PROGRAMMER:             David Welch (welch@mcmail.com)
 * REVISION HISTORY: 
 *               23/06/98: Created
 *               12/10/99: Phillip Susi:  Thread priorities, and APC work
 *               09/08/03: Skywing:       ThreadEventPair support (delete)
 */

/*
 * NOTE:
 * 
 * All of the routines that manipulate the thread queue synchronize on
 * a single spinlock
 * 
 */

/* INCLUDES ****************************************************************/

#include <limits.h>

#define NTOS_MODE_KERNEL
#include <ntos.h>
#include <internal/ke.h>
#include <internal/ob.h>
#include <internal/ps.h>
#include <internal/se.h>
#include <internal/id.h>
#include <internal/dbg.h>
#include <internal/ldr.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBAL *******************************************************************/

static ULONG PiNextThreadUniqueId = 0;

extern KSPIN_LOCK PiThreadListLock;
extern ULONG PiNrThreads;

extern LIST_ENTRY PiThreadListHead;

#define MAX_THREAD_NOTIFY_ROUTINE_COUNT    8

static ULONG PiThreadNotifyRoutineCount = 0;
static PCREATE_THREAD_NOTIFY_ROUTINE
PiThreadNotifyRoutine[MAX_THREAD_NOTIFY_ROUTINE_COUNT];

/* FUNCTIONS ***************************************************************/

NTSTATUS STDCALL
PsAssignImpersonationToken(PETHREAD Thread,
			   HANDLE TokenHandle)
{
   PACCESS_TOKEN Token;
   SECURITY_IMPERSONATION_LEVEL ImpersonationLevel;
   NTSTATUS Status;

   if (TokenHandle != NULL)
     {
	Status = ObReferenceObjectByHandle(TokenHandle,
					   TOKEN_IMPERSONATE,
					   SepTokenObjectType,
					   KeGetPreviousMode(),
					   (PVOID*)&Token,
					   NULL);
	if (!NT_SUCCESS(Status))
	  {
	     return(Status);
	  }
	ImpersonationLevel = Token->ImpersonationLevel;
     }
   else
     {
	Token = NULL;
	ImpersonationLevel = 0;
     }

   PsImpersonateClient(Thread,
		       Token,
		       FALSE,
		       FALSE,
		       ImpersonationLevel);
   if (Token != NULL)
     {
	ObDereferenceObject(Token);
     }

   return(STATUS_SUCCESS);
}


/*
 * @implemented
 */
VOID STDCALL
PsRevertToSelf (VOID)
{
  PETHREAD Thread;

  Thread = PsGetCurrentThread ();

  if (Thread->ActiveImpersonationInfo == TRUE)
    {
      ObDereferenceObject (Thread->ImpersonationInfo->Token);
      Thread->ActiveImpersonationInfo = FALSE;
    }
}


/*
 * @implemented
 */
VOID STDCALL
PsImpersonateClient (IN PETHREAD Thread,
		     IN PACCESS_TOKEN Token,
		     IN BOOLEAN CopyOnOpen,
		     IN BOOLEAN EffectiveOnly,
		     IN SECURITY_IMPERSONATION_LEVEL ImpersonationLevel)
{
  if (Token == NULL)
    {
      if (Thread->ActiveImpersonationInfo == TRUE)
	{
	  Thread->ActiveImpersonationInfo = FALSE;
	  if (Thread->ImpersonationInfo->Token != NULL)
	    {
	      ObDereferenceObject (Thread->ImpersonationInfo->Token);
	    }
	}
      return;
    }

  if (Thread->ImpersonationInfo == NULL)
    {
      Thread->ImpersonationInfo = ExAllocatePool (NonPagedPool,
						  sizeof(PS_IMPERSONATION_INFO));
    }

  Thread->ImpersonationInfo->Level = ImpersonationLevel;
  Thread->ImpersonationInfo->CopyOnOpen = CopyOnOpen;
  Thread->ImpersonationInfo->EffectiveOnly = EffectiveOnly;
  Thread->ImpersonationInfo->Token = Token;
  ObReferenceObjectByPointer (Token,
			      0,
			      SepTokenObjectType,
			      KernelMode);
  Thread->ActiveImpersonationInfo = TRUE;
}


PACCESS_TOKEN
PsReferenceEffectiveToken(PETHREAD Thread,
			  PTOKEN_TYPE TokenType,
			  PBOOLEAN EffectiveOnly,
			  PSECURITY_IMPERSONATION_LEVEL Level)
{
   PEPROCESS Process;
   PACCESS_TOKEN Token;
   
   if (Thread->ActiveImpersonationInfo == FALSE)
     {
	Process = Thread->ThreadsProcess;
	*TokenType = TokenPrimary;
	*EffectiveOnly = FALSE;
	Token = Process->Token;
     }
   else
     {
	Token = Thread->ImpersonationInfo->Token;
	*TokenType = TokenImpersonation;
	*EffectiveOnly = Thread->ImpersonationInfo->EffectiveOnly;
	*Level = Thread->ImpersonationInfo->Level;
     }
   return(Token);
}


NTSTATUS STDCALL
NtImpersonateThread(IN HANDLE ThreadHandle,
		    IN HANDLE ThreadToImpersonateHandle,
		    IN PSECURITY_QUALITY_OF_SERVICE SecurityQualityOfService)
{
  SECURITY_CLIENT_CONTEXT ClientContext;
  PETHREAD Thread;
  PETHREAD ThreadToImpersonate;
  NTSTATUS Status;

  Status = ObReferenceObjectByHandle (ThreadHandle,
				      0,
				      PsThreadType,
				      UserMode,
				      (PVOID*)&Thread,
				      NULL);
  if (!NT_SUCCESS (Status))
    {
      return Status;
    }

  Status = ObReferenceObjectByHandle (ThreadToImpersonateHandle,
				      0,
				      PsThreadType,
				      UserMode,
				      (PVOID*)&ThreadToImpersonate,
				      NULL);
  if (!NT_SUCCESS(Status))
    {
      ObDereferenceObject (Thread);
      return Status;
    }

  Status = SeCreateClientSecurity (ThreadToImpersonate,
				   SecurityQualityOfService,
				   0,
				   &ClientContext);
  if (!NT_SUCCESS(Status))
    {
      ObDereferenceObject (ThreadToImpersonate);
      ObDereferenceObject (Thread);
      return Status;
     }

  SeImpersonateClient (&ClientContext,
		       Thread);
  if (ClientContext.Token != NULL)
    {
      ObDereferenceObject (ClientContext.Token);
    }

  ObDereferenceObject (ThreadToImpersonate);
  ObDereferenceObject (Thread);

  return STATUS_SUCCESS;
}


NTSTATUS STDCALL
NtOpenThreadToken (IN HANDLE ThreadHandle,
		   IN ACCESS_MASK DesiredAccess,
		   IN BOOLEAN OpenAsSelf,
		   OUT PHANDLE TokenHandle)
{
  PACCESS_TOKEN Token;
  PETHREAD Thread;
  NTSTATUS Status;

  Status = ObReferenceObjectByHandle (ThreadHandle,
				      0,
				      PsThreadType,
				      UserMode,
				      (PVOID*)&Thread,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }

  if (OpenAsSelf)
    {
      Token = Thread->ThreadsProcess->Token;
    }
  else
    {
      if (Thread->ActiveImpersonationInfo == FALSE)
	{
	  ObDereferenceObject (Thread);
	  return STATUS_NO_TOKEN;
	}

      Token = Thread->ImpersonationInfo->Token;
    }

  if (Token == NULL)
    {
      ObDereferenceObject (Thread);
      return STATUS_NO_TOKEN;
    }

  Status = ObCreateHandle (PsGetCurrentProcess(),
			   Token,
			   DesiredAccess,
			   FALSE,
			   TokenHandle);

  ObDereferenceObject (Thread);

  return Status;
}


/*
 * @implemented
 */
PACCESS_TOKEN STDCALL
PsReferenceImpersonationToken(IN PETHREAD Thread,
			      OUT PBOOLEAN CopyOnOpen,
			      OUT PBOOLEAN EffectiveOnly,
			      OUT PSECURITY_IMPERSONATION_LEVEL ImpersonationLevel)
{
  if (Thread->ActiveImpersonationInfo == FALSE)
    {
      return NULL;
    }

  *ImpersonationLevel = Thread->ImpersonationInfo->Level;
  *CopyOnOpen = Thread->ImpersonationInfo->CopyOnOpen;
  *EffectiveOnly = Thread->ImpersonationInfo->EffectiveOnly;
  ObReferenceObjectByPointer (Thread->ImpersonationInfo->Token,
			      TOKEN_ALL_ACCESS,
			      SepTokenObjectType,
			      KernelMode);

  return Thread->ImpersonationInfo->Token;
}


VOID
PiBeforeBeginThread(CONTEXT c)
{
   KeLowerIrql(PASSIVE_LEVEL);
}


VOID STDCALL
PiDeleteThread(PVOID ObjectBody)
{
  KIRQL oldIrql;
  PETHREAD Thread;

  Thread = (PETHREAD)ObjectBody;

  DPRINT("PiDeleteThread(ObjectBody %x)\n",ObjectBody);

  ObDereferenceObject(Thread->ThreadsProcess);
  Thread->ThreadsProcess = NULL;

  KeAcquireSpinLock(&PiThreadListLock, &oldIrql);
  PiNrThreads--;
  RemoveEntryList(&Thread->Tcb.ThreadListEntry);
  KeReleaseSpinLock(&PiThreadListLock, oldIrql);

  KeReleaseThread(Thread);
  DPRINT("PiDeleteThread() finished\n");
}


NTSTATUS
PsInitializeThread(HANDLE ProcessHandle,
		   PETHREAD* ThreadPtr,
		   PHANDLE ThreadHandle,
		   ACCESS_MASK	DesiredAccess,
		   POBJECT_ATTRIBUTES ThreadAttributes,
		   BOOLEAN First)
{
   PETHREAD Thread;
   NTSTATUS Status;
   KIRQL oldIrql;
   PEPROCESS Process;

   /*
    * Reference process
    */
   if (ProcessHandle != NULL)
     {
	Status = ObReferenceObjectByHandle(ProcessHandle,
					   PROCESS_CREATE_THREAD,
					   PsProcessType,
					   UserMode,
					   (PVOID*)&Process,
					   NULL);
	if (Status != STATUS_SUCCESS)
	  {
	     DPRINT("Failed at %s:%d\n",__FILE__,__LINE__);
	     return(Status);
	  }
	DPRINT( "Creating thread in process %x\n", Process );
     }
   else
     {
	Process = PsInitialSystemProcess;
	ObReferenceObjectByPointer(Process,
				   PROCESS_CREATE_THREAD,
				   PsProcessType,
				   UserMode);
     }
   
   /*
    * Create and initialize thread
    */
   Status = ObCreateObject(UserMode,
			   PsThreadType,
			   ThreadAttributes,
			   UserMode,
			   NULL,
			   sizeof(ETHREAD),
			   0,
			   0,
			   (PVOID*)&Thread);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }

  Status = ObInsertObject ((PVOID)Thread,
			   NULL,
			   DesiredAccess,
			   0,
			   NULL,
			   ThreadHandle);
  if (!NT_SUCCESS(Status))
    {
      ObDereferenceObject (Thread);
      return Status;
    }

   DPRINT("Thread = %x\n",Thread);
   
   PiNrThreads++;
   
   KeInitializeThread(&Process->Pcb, &Thread->Tcb, First);
   Thread->ThreadsProcess = Process;
   /*
    * FIXME: What lock protects this?
    */
   InsertTailList(&Thread->ThreadsProcess->ThreadListHead, 
		  &Thread->Tcb.ProcessThreadListEntry);
   InitializeListHead(&Thread->TerminationPortList);
   KeInitializeSpinLock(&Thread->ActiveTimerListLock);
   InitializeListHead(&Thread->IrpList);
   Thread->Cid.UniqueThread = (HANDLE)InterlockedIncrement(
					      (LONG *)&PiNextThreadUniqueId);
   Thread->Cid.UniqueProcess = (HANDLE)Thread->ThreadsProcess->UniqueProcessId;
   Thread->DeadThread = 0;
   Thread->Win32Thread = 0;
   DPRINT("Thread->Cid.UniqueThread %d\n",Thread->Cid.UniqueThread);
   
   *ThreadPtr = Thread;
   
   KeAcquireSpinLock(&PiThreadListLock, &oldIrql);
   InsertTailList(&PiThreadListHead, &Thread->Tcb.ThreadListEntry);
   KeReleaseSpinLock(&PiThreadListLock, oldIrql);

   Thread->Tcb.BasePriority = (CHAR)Thread->ThreadsProcess->Pcb.BasePriority;
   Thread->Tcb.Priority = Thread->Tcb.BasePriority;

   /*
    * Local Procedure Call facility (LPC)
    */
   KeInitializeSemaphore  (& Thread->LpcReplySemaphore, 0, LONG_MAX);
   Thread->LpcReplyMessage = NULL;
   Thread->LpcReplyMessageId = 0; /* not valid */
   /* Thread->LpcReceiveMessageId = 0; */
   Thread->LpcExitThreadCalled = FALSE;
   Thread->LpcReceivedMsgIdValid = FALSE;

   return(STATUS_SUCCESS);
}


static NTSTATUS
PsCreateTeb(HANDLE ProcessHandle,
	    PTEB *TebPtr,
	    PETHREAD Thread,
	    PUSER_STACK UserStack)
{
   MEMORY_BASIC_INFORMATION Info;
   NTSTATUS Status;
   ULONG ByteCount;
   ULONG RegionSize;
   ULONG TebSize;
   PVOID TebBase;
   TEB Teb;
   ULONG ResultLength;

   TebBase = (PVOID)0x7FFDE000;
   TebSize = PAGE_SIZE;

   while (TRUE)
     {
	Status = ZwQueryVirtualMemory(ProcessHandle,
				      TebBase,
				      MemoryBasicInformation,
				      &Info,
				      sizeof(Info),
				      &ResultLength);
	if (!NT_SUCCESS(Status))
	  {
	     CPRINT("ZwQueryVirtualMemory (Status %x)\n", Status);
	     KEBUGCHECK(0);
	  }
	/* FIXME: Race between this and the above check */
	if (Info.State == MEM_FREE)
	  {
	     /* The TEB must reside in user space */
	     Status = ZwAllocateVirtualMemory(ProcessHandle,
					      &TebBase,
					      0,
					      &TebSize,
					      MEM_RESERVE | MEM_COMMIT,
					      PAGE_READWRITE);
	     if (NT_SUCCESS(Status))
	       {
		  break;
	       }
	  }
	     
	TebBase = (char*)TebBase - TebSize;
     }

   DPRINT ("TebBase %p TebSize %lu\n", TebBase, TebSize);

   RtlZeroMemory(&Teb, sizeof(TEB));
   /* set all pointers to and from the TEB */
   Teb.Tib.Self = TebBase;
   if (Thread->ThreadsProcess)
     {
        Teb.Peb = Thread->ThreadsProcess->Peb; /* No PEB yet!! */
     }
   DPRINT("Teb.Peb %x\n", Teb.Peb);
   
   /* store stack information from UserStack */
   if(UserStack != NULL)
   {
    /* fixed-size stack */
    if(UserStack->FixedStackBase && UserStack->FixedStackLimit)
    {
     Teb.Tib.StackBase = UserStack->FixedStackBase;
     Teb.Tib.StackLimit = UserStack->FixedStackLimit;
     Teb.DeallocationStack = UserStack->FixedStackLimit;
    }
    /* expandable stack */
    else
    {
     Teb.Tib.StackBase = UserStack->ExpandableStackBase;
     Teb.Tib.StackLimit = UserStack->ExpandableStackLimit;
     Teb.DeallocationStack = UserStack->ExpandableStackBottom;
    }
   }

   /* more initialization */
   Teb.Cid.UniqueThread = Thread->Cid.UniqueThread;
   Teb.Cid.UniqueProcess = Thread->Cid.UniqueProcess;
   Teb.CurrentLocale = PsDefaultThreadLocaleId;

   /* Terminate the exception handler list */
   Teb.Tib.ExceptionList = (PVOID)-1;
   
   DPRINT("sizeof(TEB) %x\n", sizeof(TEB));
   
   /* write TEB data into teb page */
   Status = NtWriteVirtualMemory(ProcessHandle,
                                 TebBase,
                                 &Teb,
                                 sizeof(TEB),
                                 &ByteCount);

   if (!NT_SUCCESS(Status))
     {
        /* free TEB */
        DPRINT1 ("Writing TEB failed!\n");

        RegionSize = 0;
        NtFreeVirtualMemory(ProcessHandle,
                            TebBase,
                            &RegionSize,
                            MEM_RELEASE);

        return Status;
     }

   if (TebPtr != NULL)
     {
        *TebPtr = (PTEB)TebBase;
     }

   DPRINT("TEB allocated at %p\n", TebBase);

   return Status;
}


VOID STDCALL
LdrInitApcRundownRoutine(PKAPC Apc)
{
   ExFreePool(Apc);
}


VOID STDCALL
LdrInitApcKernelRoutine(PKAPC Apc,
			PKNORMAL_ROUTINE* NormalRoutine,
			PVOID* NormalContext,
			PVOID* SystemArgument1,
			PVOID* SystemArgument2)
{
  ExFreePool(Apc);
}


NTSTATUS STDCALL
NtCreateThread(PHANDLE ThreadHandle,
	       ACCESS_MASK DesiredAccess,
	       POBJECT_ATTRIBUTES ObjectAttributes,
	       HANDLE ProcessHandle,
	       PCLIENT_ID Client,
	       PCONTEXT ThreadContext,
	       PUSER_STACK UserStack,
	       BOOLEAN CreateSuspended)
{
  PETHREAD Thread;
  PTEB TebBase;
  NTSTATUS Status;
  PKAPC LdrInitApc;

  DPRINT("NtCreateThread(ThreadHandle %x, PCONTEXT %x)\n",
	 ThreadHandle,ThreadContext);

  Status = PsInitializeThread(ProcessHandle,
			      &Thread,
			      ThreadHandle,
			      DesiredAccess,
			      ObjectAttributes,
			      FALSE);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }

  Status = KiArchInitThreadWithContext(&Thread->Tcb, ThreadContext);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }

  Status = PsCreateTeb(ProcessHandle,
		       &TebBase,
		       Thread,
		       UserStack);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }
  Thread->Tcb.Teb = TebBase;

  Thread->StartAddress = NULL;

  if (Client != NULL)
    {
      *Client = Thread->Cid;
    }

  /*
   * Maybe send a message to the process's debugger
   */
  DbgkCreateThread((PVOID)ThreadContext->Eip);

  /*
   * First, force the thread to be non-alertable for user-mode alerts.
   */
  Thread->Tcb.Alertable = FALSE;

  /*
   * If the thread is to be created suspended then queue an APC to
   * do the suspend before we run any userspace code.
   */
  if (CreateSuspended)
    {
      PsSuspendThread(Thread, NULL);
    }

  /*
   * Queue an APC to the thread that will execute the ntdll startup
   * routine.
   */
  LdrInitApc = ExAllocatePool(NonPagedPool, sizeof(KAPC));
  KeInitializeApc(LdrInitApc, &Thread->Tcb, OriginalApcEnvironment, LdrInitApcKernelRoutine,
		  LdrInitApcRundownRoutine, LdrpGetSystemDllEntryPoint(), 
		  UserMode, NULL);
  KeInsertQueueApc(LdrInitApc, NULL, NULL, IO_NO_INCREMENT);

  /*
   * Start the thread running and force it to execute the APC(s) we just
   * queued before it runs anything else in user-mode.
   */
  Thread->Tcb.Alertable = TRUE;
  Thread->Tcb.Alerted[0] = 1;
  PsUnblockThread(Thread, NULL);

  return(STATUS_SUCCESS);
}


/*
 * @implemented
 */
NTSTATUS STDCALL
PsCreateSystemThread(PHANDLE ThreadHandle,
		     ACCESS_MASK DesiredAccess,
		     POBJECT_ATTRIBUTES ObjectAttributes,
		     HANDLE ProcessHandle,
		     PCLIENT_ID ClientId,
		     PKSTART_ROUTINE StartRoutine,
		     PVOID StartContext)
/*
 * FUNCTION: Creates a thread which executes in kernel mode
 * ARGUMENTS:
 *       ThreadHandle (OUT) = Caller supplied storage for the returned thread 
 *                            handle
 *       DesiredAccess = Requested access to the thread
 *       ObjectAttributes = Object attributes (optional)
 *       ProcessHandle = Handle of process thread will run in
 *                       NULL to use system process
 *       ClientId (OUT) = Caller supplied storage for the returned client id
 *                        of the thread (optional)
 *       StartRoutine = Entry point for the thread
 *       StartContext = Argument supplied to the thread when it begins
 *                     execution
 * RETURNS: Success or failure status
 */
{
   PETHREAD Thread;
   NTSTATUS Status;
   
   DPRINT("PsCreateSystemThread(ThreadHandle %x, ProcessHandle %x)\n",
	    ThreadHandle,ProcessHandle);
   
   Status = PsInitializeThread(ProcessHandle,
			       &Thread,
			       ThreadHandle,
			       DesiredAccess,
			       ObjectAttributes,
			       FALSE);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   Thread->StartAddress = StartRoutine;
   Status = KiArchInitThread(&Thread->Tcb, StartRoutine, StartContext);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }

   if (ClientId != NULL)
     {
	*ClientId=Thread->Cid;
     }

   PsUnblockThread(Thread, NULL);
   
   return(STATUS_SUCCESS);
}


VOID STDCALL
PspRunCreateThreadNotifyRoutines(PETHREAD CurrentThread,
				 BOOLEAN Create)
{
  ULONG i;
  CLIENT_ID Cid = CurrentThread->Cid;

  for (i = 0; i < PiThreadNotifyRoutineCount; i++)
    {
      PiThreadNotifyRoutine[i](Cid.UniqueProcess, Cid.UniqueThread, Create);
    }
}


/*
 * @implemented
 */
NTSTATUS STDCALL
PsSetCreateThreadNotifyRoutine(IN PCREATE_THREAD_NOTIFY_ROUTINE NotifyRoutine)
{
  if (PiThreadNotifyRoutineCount >= MAX_THREAD_NOTIFY_ROUTINE_COUNT)
    {
      return(STATUS_INSUFFICIENT_RESOURCES);
    }

  PiThreadNotifyRoutine[PiThreadNotifyRoutineCount] = NotifyRoutine;
  PiThreadNotifyRoutineCount++;

  return(STATUS_SUCCESS);
}

/* EOF */
