/* $Id: create.c,v 1.19 2000/07/04 08:52:46 dwelch Exp $
 *
 * COPYRIGHT:              See COPYING in the top level directory
 * PROJECT:                ReactOS kernel
 * FILE:                   ntoskrnl/ps/thread.c
 * PURPOSE:                Thread managment
 * PROGRAMMER:             David Welch (welch@mcmail.com)
 * REVISION HISTORY: 
 *               23/06/98: Created
 *               12/10/99: Phillip Susi:  Thread priorities, and APC work
 */

/*
 * NOTE:
 * 
 * All of the routines that manipulate the thread queue synchronize on
 * a single spinlock
 * 
 */

/* INCLUDES ****************************************************************/

#include <ddk/ntddk.h>
#include <internal/ke.h>
#include <internal/ob.h>
#include <string.h>
#include <internal/string.h>
#include <internal/hal.h>
#include <internal/ps.h>
#include <internal/ob.h>
#include <internal/id.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBAL *******************************************************************/

static ULONG PiNextThreadUniqueId = 0;

extern KSPIN_LOCK PiThreadListLock;
extern ULONG PiNrThreads;

extern LIST_ENTRY PiThreadListHead;

/* FUNCTIONS ***************************************************************/

NTSTATUS
STDCALL
PsAssignImpersonationToken(PETHREAD Thread,
			   HANDLE TokenHandle)
{
   PACCESS_TOKEN Token;
   SECURITY_IMPERSONATION_LEVEL ImpersonationLevel;
   NTSTATUS Status;
   
   if (TokenHandle != NULL)
     {
	Status = ObReferenceObjectByHandle(TokenHandle,
					   0,
					   SeTokenType,
					   UserMode,
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
		       0,
		       0,
		       ImpersonationLevel);
   if (Token != NULL)
     {
	ObDereferenceObject(Token);
     }
   return(STATUS_SUCCESS);
}

VOID STDCALL PsRevertToSelf(PETHREAD Thread)
{
   if (Thread->ActiveImpersonationInfo != 0)
     {
	Thread->ActiveImpersonationInfo = 0;
	ObDereferenceObject(Thread->ImpersonationInfo->Token);
     }
}

VOID STDCALL PsImpersonateClient(PETHREAD Thread,
				 PACCESS_TOKEN Token,
				 UCHAR b,
				 UCHAR c,
				 SECURITY_IMPERSONATION_LEVEL Level)
{
   if (Token == 0)
     {
	if (Thread->ActiveImpersonationInfo != 0)
	  {
	     Thread->ActiveImpersonationInfo = 0;
	     if (Thread->ImpersonationInfo->Token != NULL)
	       {
		  ObDereferenceObject(Thread->ImpersonationInfo->Token);
	       }
	  }
	return;
     }
   if (Thread->ActiveImpersonationInfo == 0 ||
       Thread->ImpersonationInfo == NULL)
     {
	Thread->ImpersonationInfo = ExAllocatePool(NonPagedPool,
      					   sizeof(PS_IMPERSONATION_INFO));	
     }
   Thread->ImpersonationInfo->Level = Level;
   Thread->ImpersonationInfo->Unknown2 = c;
   Thread->ImpersonationInfo->Unknown1 = b;
   Thread->ImpersonationInfo->Token = Token;
   ObReferenceObjectByPointer(Token,
			      0,
			      SeTokenType,
			      KernelMode);
   Thread->ActiveImpersonationInfo = 1;
}

PACCESS_TOKEN PsReferenceEffectiveToken(PETHREAD Thread,
					PTOKEN_TYPE TokenType,
					PUCHAR b,
					PSECURITY_IMPERSONATION_LEVEL Level)
{
   PEPROCESS Process;
   PACCESS_TOKEN Token;
   
   if (Thread->ActiveImpersonationInfo == 0)
     {
	Process = Thread->ThreadsProcess;
	*TokenType = TokenPrimary;
	*b = 0;
	Token = Process->Token;
     }
   else
     {
	Token = Thread->ImpersonationInfo->Token;
	*TokenType = TokenImpersonation;
	*b = Thread->ImpersonationInfo->Unknown2;
	*Level = Thread->ImpersonationInfo->Level;	
     }
   return(Token);
}

NTSTATUS STDCALL NtImpersonateThread (IN HANDLE ThreadHandle,
				      IN HANDLE ThreadToImpersonateHandle,
				      IN PSECURITY_QUALITY_OF_SERVICE	
				      SecurityQualityOfService)
{
   PETHREAD Thread;
   PETHREAD ThreadToImpersonate;
   NTSTATUS Status;
   SE_SOME_STRUCT2 b;
   
   Status = ObReferenceObjectByHandle(ThreadHandle,
				      0,
				      PsThreadType,
				      UserMode,
				      (PVOID*)&Thread,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   Status = ObReferenceObjectByHandle(ThreadToImpersonateHandle,
				      0,
				      PsThreadType,
				      UserMode,
				      (PVOID*)&ThreadToImpersonate,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	ObDereferenceObject(Thread);
	return(Status);
     }
   
   Status = SeCreateClientSecurity(ThreadToImpersonate,
				   SecurityQualityOfService,
				   0,
				   &b);
   if (!NT_SUCCESS(Status))
     {
	ObDereferenceObject(Thread);
	ObDereferenceObject(ThreadToImpersonate);
	return(Status);
     }
   
   SeImpersonateClient(&b, Thread);
   if (b.Token != NULL)
     {
	ObDereferenceObject(b.Token);
     }
   return(STATUS_SUCCESS);
}

NTSTATUS STDCALL NtOpenThreadToken(IN	HANDLE		ThreadHandle,  
				   IN	ACCESS_MASK	DesiredAccess,  
				   IN	BOOLEAN		OpenAsSelf,     
				   OUT	PHANDLE		TokenHandle)
{
#if 0
   PETHREAD Thread;
   NTSTATUS Status;
   PACCESS_TOKEN Token;
   
   Status = ObReferenceObjectByHandle(ThreadHandle,
				      0,
				      PsThreadType,
				      UserMode,
				      (PVOID*)&Thread,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   Token = PsReferencePrimaryToken(Thread->ThreadsProcess);
   SepCreateImpersonationTokenDacl(Token);
#endif
   return(STATUS_UNSUCCESSFUL);
}

PACCESS_TOKEN STDCALL PsReferenceImpersonationToken(PETHREAD Thread,
						    PULONG Unknown1,
						    PULONG Unknown2,
						    SECURITY_IMPERSONATION_LEVEL* 
						    Level)
{
   if (Thread->ActiveImpersonationInfo == 0)
     {
	return(NULL);
     }
   
   *Level = Thread->ImpersonationInfo->Level;
   *Unknown1 = Thread->ImpersonationInfo->Unknown1;
   *Unknown2 = Thread->ImpersonationInfo->Unknown2;
   ObReferenceObjectByPointer(Thread->ImpersonationInfo->Token,
			      GENERIC_ALL,
			      SeTokenType,
			      KernelMode);
   return(Thread->ImpersonationInfo->Token);
}

static VOID PiTimeoutThread(struct _KDPC *dpc, 
			    PVOID Context, 
			    PVOID arg1, 
			    PVOID arg2 )
{
   // wake up the thread, and tell it it timed out
   NTSTATUS Status = STATUS_TIMEOUT;
   
   DPRINT("PiTimeoutThread()\n");
   
   KeRemoveAllWaitsThread((PETHREAD)Context, Status);
}

VOID PiBeforeBeginThread(CONTEXT c)
{
   DPRINT("PiBeforeBeginThread(Eip %x)\n", c.Eip);
   //KeReleaseSpinLock(&PiThreadListLock, PASSIVE_LEVEL);
   KeLowerIrql(PASSIVE_LEVEL);
}

VOID PsBeginThread(PKSTART_ROUTINE StartRoutine, PVOID StartContext)
{
   NTSTATUS Ret;
   
//   KeReleaseSpinLock(&PiThreadListLock,PASSIVE_LEVEL);
   KeLowerIrql(PASSIVE_LEVEL);
   Ret = StartRoutine(StartContext);
   PsTerminateSystemThread(Ret);
   KeBugCheck(0);
}

VOID PiDeleteThread(PVOID ObjectBody)
{
   KIRQL oldIrql;
   
   DPRINT("PiDeleteThread(ObjectBody %x)\n",ObjectBody);
   
   KeAcquireSpinLock(&PiThreadListLock, &oldIrql);
   DPRINT("Process %x(%d)\n", ((PETHREAD)ObjectBody)->ThreadsProcess,
	   ObGetReferenceCount(((PETHREAD)ObjectBody)->ThreadsProcess));
   ObDereferenceObject(((PETHREAD)ObjectBody)->ThreadsProcess);
   ((PETHREAD)ObjectBody)->ThreadsProcess = NULL;
   PiNrThreads--;
   RemoveEntryList(&((PETHREAD)ObjectBody)->Tcb.ThreadListEntry);
   HalReleaseTask((PETHREAD)ObjectBody);
   KeReleaseSpinLock(&PiThreadListLock, oldIrql);
   DPRINT("PiDeleteThread() finished\n");
}

VOID PiCloseThread(PVOID ObjectBody, ULONG HandleCount)
{
   DPRINT("PiCloseThread(ObjectBody %x)\n", ObjectBody);
   DPRINT("ObGetReferenceCount(ObjectBody) %d "
	   "ObGetHandleCount(ObjectBody) %d\n",
	   ObGetReferenceCount(ObjectBody),
	   ObGetHandleCount(ObjectBody));
}

NTSTATUS PsInitializeThread(HANDLE ProcessHandle, 
			    PETHREAD* ThreadPtr,
			    PHANDLE ThreadHandle,
			    ACCESS_MASK	DesiredAccess,
			    POBJECT_ATTRIBUTES ThreadAttributes)
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
   Thread = ObCreateObject(ThreadHandle,
			   DesiredAccess,
			   ThreadAttributes,
			   PsThreadType);
   DPRINT("Thread = %x\n",Thread);
   
   PiNrThreads++;
   
   Thread->Tcb.State = THREAD_STATE_SUSPENDED;
   Thread->Tcb.SuspendCount = 0;
   Thread->Tcb.FreezeCount = 1;
   InitializeListHead(&Thread->Tcb.ApcState.ApcListHead[0]);
   InitializeListHead(&Thread->Tcb.ApcState.ApcListHead[1]);
   Thread->Tcb.KernelApcDisable = 1;
   Thread->Tcb.WaitIrql = PASSIVE_LEVEL;
   Thread->ThreadsProcess = Process;
   KeInitializeDpc(&Thread->Tcb.TimerDpc, PiTimeoutThread, Thread);
   Thread->Tcb.WaitBlockList = NULL;
   InsertTailList(&Thread->ThreadsProcess->ThreadListHead, 
		  &Thread->Tcb.ProcessThreadListEntry);
   InitializeListHead(&Thread->TerminationPortList);
   KeInitializeDispatcherHeader(&Thread->Tcb.DispatcherHeader,
                                InternalThreadType,
                                sizeof(ETHREAD),
                                FALSE);
   KeInitializeSpinLock(&Thread->ActiveTimerListLock);
   InitializeListHead(&Thread->IrpList);
   Thread->Cid.UniqueThread = (HANDLE)InterlockedIncrement(
					      &PiNextThreadUniqueId);
   Thread->Cid.UniqueProcess = (HANDLE)Thread->ThreadsProcess->UniqueProcessId;
   DPRINT("Thread->Cid.UniqueThread %d\n",Thread->Cid.UniqueThread);
   
   *ThreadPtr = Thread;
   
   KeAcquireSpinLock(&PiThreadListLock, &oldIrql);
   InsertTailList(&PiThreadListHead, &Thread->Tcb.ThreadListEntry);
   KeReleaseSpinLock(&PiThreadListLock, oldIrql);

   Thread->Tcb.BasePriority = Thread->ThreadsProcess->Pcb.BasePriority;
   Thread->Tcb.Priority = Thread->Tcb.BasePriority;
   
   return(STATUS_SUCCESS);
}


static NTSTATUS PsCreateTeb (HANDLE ProcessHandle,
			     PNT_TEB *TebPtr,
			     PETHREAD Thread,
			     PINITIAL_TEB InitialTeb)
{
//   MEMORY_BASIC_INFORMATION Info;
   NTSTATUS Status;
   ULONG ByteCount;
   ULONG RegionSize;
   ULONG TebSize;
   PVOID TebBase;
   NT_TEB Teb;

   TebBase = (PVOID)0x7FFDE000;
   TebSize = PAGESIZE;

   while (TRUE)
     {
        /* The TEB must reside in user space */
        Status = NtAllocateVirtualMemory(ProcessHandle,
                                         &TebBase,
                                         0,
                                         &TebSize,
                                         MEM_COMMIT,
                                         PAGE_READWRITE);
        if (NT_SUCCESS(Status))
          {
//             DPRINT1 ("TEB allocated at %x\n", TebBase);
             break;
          }
        else
          {
             DPRINT ("TEB allocation failed! Status %x\n",Status);
	     for(;;);
	     return(Status);
          }

//        TebBase = Info.BaseAddress - TebSize;
	TebBase = TebBase - TebSize;
   }

   DPRINT ("TebBase %p TebSize %lu\n", TebBase, TebSize);

   /* set all pointers to and from the TEB */
   Teb.Tib.Self = TebBase;
   if (Thread->ThreadsProcess)
     {
        Teb.Peb = Thread->ThreadsProcess->Peb; /* No PEB yet!! */
     }
//   DPRINT1("Teb.Peb %x\n", Teb.Peb);
   
   /* store stack information from InitialTeb */
   if (InitialTeb != NULL)
     {
        Teb.Tib.StackBase = InitialTeb->StackBase;
        Teb.Tib.StackLimit = InitialTeb->StackLimit;

        /*
         * I don't know if this is really stored in a WNT-TEB,
         * but it's needed to free the thread stack. (Eric Kohl)
         */
        Teb.StackCommit = InitialTeb->StackCommit;
        Teb.StackCommitMax = InitialTeb->StackCommitMax;
        Teb.StackReserved = InitialTeb->StackReserved;
     }


   /* more initialization */
   Teb.Cid.UniqueThread = Thread->Cid.UniqueThread;
   Teb.Cid.UniqueProcess = Thread->Cid.UniqueProcess;
   
   DPRINT("sizeof(NT_TEB) %x\n", sizeof(NT_TEB));
   
   /* write TEB data into teb page */
   Status = NtWriteVirtualMemory(ProcessHandle,
                                 TebBase,
                                 &Teb,
                                 sizeof(NT_TEB),
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
        *TebPtr = (PNT_TEB)TebBase;
     }

//   DPRINT1 ("TEB allocated at %p\n", TebBase);

   return Status;
}


NTSTATUS STDCALL NtCreateThread (PHANDLE			ThreadHandle,
				 ACCESS_MASK		DesiredAccess,
				 POBJECT_ATTRIBUTES	ObjectAttributes,
				 HANDLE			ProcessHandle,
				 PCLIENT_ID		Client,
				 PCONTEXT		ThreadContext,
				 PINITIAL_TEB		InitialTeb,
				 BOOLEAN CreateSuspended)
{
   PETHREAD Thread;
   PNT_TEB  TebBase;
   NTSTATUS Status;
   
   DPRINT("NtCreateThread(ThreadHandle %x, PCONTEXT %x)\n",
	  ThreadHandle,ThreadContext);
   
   Status = PsInitializeThread(ProcessHandle,&Thread,ThreadHandle,
			       DesiredAccess,ObjectAttributes);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   Status = HalInitTaskWithContext(Thread,ThreadContext);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }

   Status = PsCreateTeb (ProcessHandle,
                         &TebBase,
                         Thread,
                         InitialTeb);
   if (!NT_SUCCESS(Status))
     {
        return(Status);
     }

   /* Attention: TebBase is in user memory space */
   Thread->Tcb.Teb = TebBase;

   Thread->StartAddress=NULL;

   if (Client!=NULL)
     {
	*Client=Thread->Cid;
     }  
   
   /*
    * Maybe send a message to the process's debugger
    */
#if 0
   if (ParentProcess->DebugPort != NULL)
     {
	LPC_DBG_MESSAGE Message;
	PEPROCESS Process;
	
	
	Message.Header.MessageSize = sizeof(LPC_DBG_MESSAGE);
	Message.Header.DataSize = sizeof(LPC_DBG_MESSAGE) -
	  sizeof(LPC_MESSAGE_HEADER);
	Message.EventCode = DBG_EVENT_CREATE_THREAD;
	Message.Data.CreateThread.StartAddress =
	  ;
	Message.Data.CreateProcess.Base = ImageBase;
	Message.Data.CreateProcess.EntryPoint = NULL; //
	
	Status = LpcSendDebugMessagePort(ParentProcess->DebugPort,
					 &Message);
     }
#endif
   
   if (!CreateSuspended)
     {
        DPRINT("Not creating suspended\n");
	PsUnfreezeThread(Thread, NULL);
     }
   DPRINT("Thread %x\n", Thread);
   DPRINT("ObGetReferenceCount(Thread) %d ObGetHandleCount(Thread) %x\n",
	  ObGetReferenceCount(Thread), ObGetHandleCount(Thread));
   DPRINT("Finished PsCreateThread()\n");
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL PsCreateSystemThread(PHANDLE ThreadHandle,
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
   
   Status = PsInitializeThread(ProcessHandle,&Thread,ThreadHandle,
			       DesiredAccess,ObjectAttributes);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   Thread->StartAddress=StartRoutine;
   Status = HalInitTask(Thread,StartRoutine,StartContext);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }

   if (ClientId!=NULL)
     {
	*ClientId=Thread->Cid;
     }  

   PsUnfreezeThread(Thread, NULL);
   
   return(STATUS_SUCCESS);
}

