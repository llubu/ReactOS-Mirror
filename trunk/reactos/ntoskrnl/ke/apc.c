/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/apc.c
 * PURPOSE:         Possible implementation of APCs
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * PORTABILITY:     Unchecked
 * UPDATE HISTORY:
 *                  Created 22/05/98
 *                  12/11/99:  Phillip Susi: Reworked the APC code
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

KSPIN_LOCK PiApcLock;

VOID PsTerminateCurrentThread(NTSTATUS ExitStatus);

#define TAG_KAPC     TAG('K', 'A', 'P', 'C')

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
BOOLEAN
STDCALL
KeAreApcsDisabled(
	VOID
	)
{
 	return KeGetCurrentThread()->KernelApcDisable ? FALSE : TRUE;
}

VOID KiRundownThread(VOID)
/*
 * FUNCTION: 
 */
{
}

BOOLEAN KiTestAlert(VOID)
/*
 * FUNCTION: Tests whether there are any pending APCs for the current thread
 * and if so the APCs will be delivered on exit from kernel mode
 */
{
   KIRQL oldIrql; 
   
   KeAcquireSpinLock(&PiApcLock, &oldIrql);
   if (KeGetCurrentThread()->ApcState.UserApcPending == 0)
     {
	KeReleaseSpinLock(&PiApcLock, oldIrql);
	return(FALSE);
     }
   KeGetCurrentThread()->Alerted[0] = 1;
   KeReleaseSpinLock(&PiApcLock, oldIrql);
   return(TRUE);
}

VOID
KiDeliverNormalApc(VOID)
{
   PETHREAD Thread = PsGetCurrentThread();
   PLIST_ENTRY current;
   PKAPC Apc;
   KIRQL oldlvl;
   PKNORMAL_ROUTINE NormalRoutine;
   PVOID NormalContext;
   PVOID SystemArgument1;
   PVOID SystemArgument2;

   KeAcquireSpinLock(&PiApcLock, &oldlvl);
   while(!IsListEmpty(&(Thread->Tcb.ApcState.ApcListHead[0])))
     {
       current = RemoveTailList(&Thread->Tcb.ApcState.ApcListHead[0]);
       Apc = CONTAINING_RECORD(current, KAPC, ApcListEntry);
       if (Apc->NormalRoutine == NULL)
	 {
	   DbgPrint("Exiting kernel with kernel APCs pending.\n");
	   KEBUGCHECKEX(KERNEL_APC_PENDING_DURING_EXIT, (ULONG)Apc,
	                Thread->Tcb.KernelApcDisable, oldlvl, 0);
	 }
       Apc->Inserted = FALSE;
       Thread->Tcb.ApcState.KernelApcInProgress++;
       Thread->Tcb.ApcState.KernelApcPending--;
       
       KeReleaseSpinLock(&PiApcLock, oldlvl);
       
       NormalRoutine = Apc->NormalRoutine;
       NormalContext = Apc->NormalContext;
       SystemArgument1 = Apc->SystemArgument1;
       SystemArgument2 = Apc->SystemArgument2;
       Apc->KernelRoutine(Apc,
			  &NormalRoutine,
			  &NormalContext,
			  &SystemArgument1,
			  &SystemArgument2);
       NormalRoutine(NormalContext, SystemArgument1, SystemArgument2);
       
       KeAcquireSpinLock(&PiApcLock, &oldlvl);
       Thread->Tcb.ApcState.KernelApcInProgress--;
     }
   KeReleaseSpinLock(&PiApcLock, oldlvl);
}

BOOLEAN 
KiDeliverUserApc(PKTRAP_FRAME TrapFrame)
/*
 * FUNCTION: Tests whether there are any pending APCs for the current thread
 * and if so the APCs will be delivered on exit from kernel mode.
 * ARGUMENTS:
 *        Thread = Thread to test for alerts
 *        UserContext = The user context saved on entry to kernel mode
 */
{
   PLIST_ENTRY current_entry;
   PKAPC Apc;
   PULONG Esp;
   PCONTEXT Context;
   KIRQL oldlvl;
   PKTHREAD Thread;

   DPRINT("KiDeliverUserApc(TrapFrame %x/%x)\n", TrapFrame,
	  KeGetCurrentThread()->TrapFrame);
   Thread = KeGetCurrentThread();

   /*
    * Check for thread termination
    */

   KeAcquireSpinLock(&PiApcLock, &oldlvl);

   current_entry = Thread->ApcState.ApcListHead[1].Flink;
   
   /*
    * Shouldn't happen but check anyway.
    */
   if (current_entry == &Thread->ApcState.ApcListHead[1])
     {
	KeReleaseSpinLock(&PiApcLock, oldlvl);
	DbgPrint("KiDeliverUserApc called but no APC was pending\n");
	return(FALSE);
     }

   while (!IsListEmpty(&Thread->ApcState.ApcListHead[1]))
     {
       current_entry = RemoveHeadList(&Thread->ApcState.ApcListHead[1]);
       Apc = CONTAINING_RECORD(current_entry, KAPC, ApcListEntry);
       Apc->Inserted = FALSE;

       /*
	* We've dealt with one pending user-mode APC
	*/
       Thread->ApcState.UserApcPending--;
       KeReleaseSpinLock(&PiApcLock, oldlvl);       
       
       /*
	* Save the thread's current context (in other words the registers
	* that will be restored when it returns to user mode) so the
	* APC dispatcher can restore them later
	*/
       Context = (PCONTEXT)(((PUCHAR)TrapFrame->Esp) - sizeof(CONTEXT));
       memset(Context, 0, sizeof(CONTEXT));
       Context->ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER | 
	 CONTEXT_SEGMENTS | CONTEXT_i386;
       Context->SegGs = TrapFrame->Gs;
       Context->SegFs = TrapFrame->Fs;
       Context->SegEs = TrapFrame->Es;
       Context->SegDs = TrapFrame->Ds;
       Context->Edi = TrapFrame->Edi;
       Context->Esi = TrapFrame->Esi;
       Context->Ebx = TrapFrame->Ebx;
       Context->Edx = TrapFrame->Edx;
       Context->Ecx = TrapFrame->Ecx;
       Context->Eax = TrapFrame->Eax;
       Context->Ebp = TrapFrame->Ebp;
       Context->Eip = TrapFrame->Eip;
       Context->SegCs = TrapFrame->Cs;
       Context->EFlags = TrapFrame->Eflags;
       Context->Esp = TrapFrame->Esp;
       Context->SegSs = TrapFrame->Ss;
       
       /*
	* Setup the trap frame so the thread will start executing at the
	* APC Dispatcher when it returns to user-mode
	*/
       Esp = (PULONG)(((PUCHAR)TrapFrame->Esp) - 
		      (sizeof(CONTEXT) + (6 * sizeof(ULONG))));
       
       Esp[0] = 0xdeadbeef;
       Esp[1] = (ULONG)Apc->NormalRoutine;
       Esp[2] = (ULONG)Apc->NormalContext;
       Esp[3] = (ULONG)Apc->SystemArgument1;
       Esp[4] = (ULONG)Apc->SystemArgument2;
       Esp[5] = (ULONG)Context;
       TrapFrame->Eip = (ULONG)LdrpGetSystemDllApcDispatcher();
       TrapFrame->Esp = (ULONG)Esp;     
       
       
       /*
	* Now call for the kernel routine for the APC, which will free
	* the APC data structure, we can't do this ourselves because
	* the APC may be embedded in some larger structure e.g. an IRP
	* We also give the kernel routine a last chance to modify the 
	* arguments to the user APC routine.
	*/
       Apc->KernelRoutine(Apc,
			  (PKNORMAL_ROUTINE*)&Esp[1],
			  (PVOID*)&Esp[2],
			  (PVOID*)&Esp[3],
			  (PVOID*)&Esp[4]);

       KeAcquireSpinLock(&PiApcLock, &oldlvl);
     }       
   Thread->Alerted[0] = 0;
   KeReleaseSpinLock(&PiApcLock, oldlvl);

   return(TRUE);
}

/*
 * @implemented
 */
VOID STDCALL 
KiDeliverApc(ULONG Unknown1,
	     ULONG Unknown2,
	     ULONG Unknown3)
/*
 * FUNCTION: Deliver an APC to the current thread.
 * NOTES: This is called from the IRQL switching code if the current thread
 * is returning from an IRQL greater than or equal to APC_LEVEL to 
 * PASSIVE_LEVEL and there are kernel-mode APCs pending. This means any
 * pending APCs will be delivered after a thread gets a new quantum and
 * after it wakes from a wait. 
 */
{
   PETHREAD Thread = PsGetCurrentThread();
   PLIST_ENTRY current_entry;
   PKAPC Apc;
   KIRQL oldlvl;

   DPRINT("KiDeliverApc()\n");
   KeAcquireSpinLock(&PiApcLock, &oldlvl);
   current_entry = Thread->Tcb.ApcState.ApcListHead[0].Flink;
   while(current_entry != &Thread->Tcb.ApcState.ApcListHead[0])
     {
       Apc = CONTAINING_RECORD(current_entry, KAPC, ApcListEntry);
       if (Apc->NormalRoutine == NULL)
	 {
	   Apc->Inserted = FALSE;
	   RemoveEntryList(&Apc->ApcListEntry);
	   Thread->Tcb.ApcState.KernelApcInProgress++;
	   Thread->Tcb.ApcState.KernelApcPending--;
	   
	   KeReleaseSpinLock(&PiApcLock, oldlvl);
	   
	   Apc->KernelRoutine(Apc,
			      &Apc->NormalRoutine,
			      &Apc->NormalContext,
			      &Apc->SystemArgument1,
			      &Apc->SystemArgument2);
	   
	   KeAcquireSpinLock(&PiApcLock, &oldlvl);
	   Thread->Tcb.ApcState.KernelApcInProgress--;
	   current_entry = Thread->Tcb.ApcState.ApcListHead[0].Flink;
	 }
       else
	 {
	   current_entry = current_entry->Flink;
	 }
     }
   KeReleaseSpinLock(&PiApcLock, oldlvl);
}

/*
 * @implemented
 */
BOOLEAN STDCALL
KeInsertQueueApc (PKAPC	Apc,
		  PVOID	SystemArgument1,
		  PVOID	SystemArgument2,
		  KPRIORITY PriorityBoost)
/*
 * FUNCTION: Queues an APC for execution
 * ARGUMENTS:
 *         Apc = APC to be queued
 *         SystemArgument[1-2] = TBD
 *         Mode = TBD
 */
{
   //FIXME: return FALSE if APC can't be queued to target thread (thread has ended)
   KIRQL oldlvl;
   PKTHREAD TargetThread;
   
   DPRINT("KeInsertQueueApc(Apc %x, SystemArgument1 %x, "
	  "SystemArgument2 %x)\n",Apc,SystemArgument1,
	  SystemArgument2);

   Apc->SystemArgument1 = SystemArgument1;
   Apc->SystemArgument2 = SystemArgument2;

   if (Apc->Inserted)
     {
#if 0
       /* TMN: This code is in error */
	DbgPrint("KeInsertQueueApc(): multiple APC insertations\n");
	KEBUGCHECK(0);
#else
	return FALSE;
#endif
     }

   KeAcquireSpinLock(&PiApcLock, &oldlvl);

   TargetThread = Apc->Thread;

   if (TargetThread->State == THREAD_STATE_TERMINATED_1 ||
       TargetThread->State == THREAD_STATE_TERMINATED_2)
     {
       KeReleaseSpinLock(&PiApcLock, oldlvl);
       return(FALSE);
     }

   if (Apc->ApcMode == KernelMode)
     {
	InsertTailList(&TargetThread->ApcStatePointer[(int) Apc->ApcStateIndex]->ApcListHead[0], 
		       &Apc->ApcListEntry);
	TargetThread->ApcStatePointer[(int) Apc->ApcStateIndex]->KernelApcPending++;
     }
   else
     {
	InsertTailList(&TargetThread->ApcStatePointer[(int) Apc->ApcStateIndex]->ApcListHead[1],
		       &Apc->ApcListEntry);
	TargetThread->ApcStatePointer[(int) Apc->ApcStateIndex]->UserApcPending++;
     }
   Apc->Inserted = TRUE;

   /*
    * If this is a kernel-mode APC for the current thread and we are not
    * inside a critical section or at APC level then call it, in fact we
    * rely on the side effects of dropping the IRQL level when we release
    * the spinlock
    */
   if (Apc->ApcMode == KernelMode && TargetThread == KeGetCurrentThread() && 
       Apc->NormalRoutine == NULL)
     {
       KeReleaseSpinLock(&PiApcLock, oldlvl);
       return TRUE;
     }

   /*
    * If this is a kernel-mode APC and it is waiting at PASSIVE_LEVEL and
    * not inside a critical section then wake it up. Otherwise it will
    * execute the APC as soon as it returns to PASSIVE_LEVEL.
    * FIXME: If the thread is running on another processor then send an
    * IPI.
    * FIXME: Check if the thread is terminating.
    */
   if (Apc->ApcMode == KernelMode && TargetThread->WaitIrql < APC_LEVEL && 
       Apc->NormalRoutine == NULL)
     {
	KeRemoveAllWaitsThread(CONTAINING_RECORD(TargetThread, ETHREAD, Tcb),
			       STATUS_KERNEL_APC, TRUE);
     }

   /*
    * If this is a 'funny' user-mode APC then mark the thread as
    * alerted so it will execute the APC on exit from kernel mode. If it
    * is waiting alertably then wake it up so it can return to user mode.
    */
   if (Apc->ApcMode == KernelMode && Apc->NormalRoutine != NULL)
     {
       TargetThread->Alerted[1] = 1;
       if (TargetThread->Alertable == TRUE &&
	   TargetThread->WaitMode == UserMode)
	 {
	   PETHREAD Thread;

	   Thread = CONTAINING_RECORD(TargetThread, ETHREAD, Tcb);
	   KeRemoveAllWaitsThread(Thread, STATUS_USER_APC, TRUE);
	 }
     }

   /*
    * If the thread is waiting alertably then wake it up and it will
    * return to to user-mode executing the APC in the process. Otherwise the
    * thread will execute the APC next time it enters an alertable wait.
    */
   if (Apc->ApcMode == UserMode && TargetThread->Alertable == TRUE &&
       TargetThread->WaitMode == UserMode)
     {
	NTSTATUS Status;
	
	DPRINT("Resuming thread for user APC\n");
	
	Status = STATUS_USER_APC;
	TargetThread->Alerted[0] = 1;
	KeRemoveAllWaitsThread(CONTAINING_RECORD(TargetThread, ETHREAD, Tcb),
			       STATUS_USER_APC, TRUE);
     }
   KeReleaseSpinLock(&PiApcLock, oldlvl);
   return TRUE;
}

BOOLEAN STDCALL
KeRemoveQueueApc (PKAPC Apc)
/*
 * FUNCTION: Removes APC object from the apc queue
 * ARGUMENTS:
 *          Apc = APC to remove
 * RETURNS: TRUE if the APC was in the queue
 *          FALSE otherwise
 * NOTE: This function is not exported.
 */
{
   KIRQL oldIrql;
   PKTHREAD TargetThread;

   KeRaiseIrql(HIGH_LEVEL, &oldIrql);
   KiAcquireSpinLock(&PiApcLock);
   if (Apc->Inserted == FALSE)
     {
        KiReleaseSpinLock(&PiApcLock);
        KeLowerIrql(oldIrql);
        return FALSE;
     }

   TargetThread = Apc->Thread;
   RemoveEntryList(&Apc->ApcListEntry);
   if (Apc->ApcMode == KernelMode)
     {
	TargetThread->ApcStatePointer[(int) Apc->ApcStateIndex]->KernelApcPending--;
     }
   else
     {
	TargetThread->ApcStatePointer[(int) Apc->ApcStateIndex]->UserApcPending--;
     }
   Apc->Inserted = FALSE;

   KiReleaseSpinLock(&PiApcLock);
   KeLowerIrql(oldIrql);   
   return(TRUE);
}


/*
 * @implemented
 */
VOID STDCALL
KeInitializeApc(
  IN PKAPC  Apc,
	IN PKTHREAD  Thread,
  IN KAPC_ENVIRONMENT  TargetEnvironment,
	IN PKKERNEL_ROUTINE  KernelRoutine,
	IN PKRUNDOWN_ROUTINE  RundownRoutine OPTIONAL,
	IN PKNORMAL_ROUTINE  NormalRoutine,
	IN KPROCESSOR_MODE  Mode,
	IN PVOID  Context)
/*
 * FUNCTION: Initialize an APC object
 * ARGUMENTS:
 *       Apc = Pointer to the APC object to initialized
 *       Thread = Thread the APC is to be delivered to
 *       StateIndex = KAPC_ENVIRONMENT
 *       KernelRoutine = Routine to be called for a kernel-mode APC
 *       RundownRoutine = Routine to be called if the thread has exited with
 *                        the APC being executed
 *       NormalRoutine = Routine to be called for a user-mode APC
 *       Mode = APC mode
 *       Context = Parameter to be passed to the APC routine
 */
{
   DPRINT("KeInitializeApc(Apc %x, Thread %x, Environment %d, "
	  "KernelRoutine %x, RundownRoutine %x, NormalRoutine %x, Mode %d, "
    "Context %x)\n",Apc,Thread,TargetEnvironment,KernelRoutine,RundownRoutine,
	  NormalRoutine,Mode,Context);

   memset(Apc, 0, sizeof(KAPC));
   Apc->Thread = Thread;
   Apc->ApcListEntry.Flink = NULL;
   Apc->ApcListEntry.Blink = NULL;
   Apc->KernelRoutine = KernelRoutine;
   Apc->RundownRoutine = RundownRoutine;
   Apc->NormalRoutine = NormalRoutine;
   Apc->NormalContext = Context;
   Apc->Inserted = FALSE;

   if (TargetEnvironment == CurrentApcEnvironment)
   {
      Apc->ApcStateIndex = Thread->ApcStateIndex;
   }
   else
   {
      Apc->ApcStateIndex = TargetEnvironment;   
   }

   if (Apc->NormalRoutine != NULL)
     {
	Apc->ApcMode = Mode;
     }
   else
     {
	Apc->ApcMode = KernelMode;
     }
}

VOID STDCALL
NtQueueApcRundownRoutine(PKAPC Apc)
{
   ExFreePool(Apc);
}

VOID STDCALL
NtQueueApcKernelRoutine(PKAPC Apc,
			PKNORMAL_ROUTINE* NormalRoutine,
			PVOID* NormalContext,
			PVOID* SystemArgument1,
			PVOID* SystemArgument2)
{
   ExFreePool(Apc);
}

NTSTATUS STDCALL
NtQueueApcThread(HANDLE			ThreadHandle,
		 PKNORMAL_ROUTINE	ApcRoutine,
		 PVOID			NormalContext,
		 PVOID			SystemArgument1,
		 PVOID			SystemArgument2)
{
   PKAPC Apc;
   PETHREAD Thread;
   NTSTATUS Status;
   PVOID ThreadVar;

   ThreadVar = &Thread;
   
   Status = ObReferenceObjectByHandle(ThreadHandle,
				      THREAD_ALL_ACCESS, /* FIXME */
				      PsThreadType,
				      UserMode,
				      &ThreadVar,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   Apc = ExAllocatePoolWithTag(NonPagedPool, sizeof(KAPC), TAG_KAPC);
   if (Apc == NULL)
     {
	ObDereferenceObject(Thread);
	return(STATUS_NO_MEMORY);
     }
   
   KeInitializeApc(Apc,
		   &Thread->Tcb,
         OriginalApcEnvironment,
		   NtQueueApcKernelRoutine,
		   NtQueueApcRundownRoutine,
		   ApcRoutine,
		   UserMode,
		   NormalContext);
   KeInsertQueueApc(Apc,
		    SystemArgument1,
		    SystemArgument2,
		    IO_NO_INCREMENT);
   
   ObDereferenceObject(Thread);
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL NtTestAlert(VOID)
{
   KiTestAlert();
   return(STATUS_SUCCESS);
}

VOID INIT_FUNCTION
PiInitApcManagement(VOID)
{
   KeInitializeSpinLock(&PiApcLock);
}

static VOID FASTCALL
RepairList(PLIST_ENTRY Original, PLIST_ENTRY Copy, int Mode)
{
  if (IsListEmpty(&Original[Mode]))
    {
      InitializeListHead(&Copy[Mode]);
    }
  else
    {
      Copy[Mode].Flink->Blink = &Copy[Mode];
      Copy[Mode].Blink->Flink = &Copy[Mode];
    }
}


VOID FASTCALL
KiSwapApcEnvironment(
  PKTHREAD Thread,
  PKPROCESS NewProcess)
{
  /* FIXME: Grab the process apc lock or the PiApcLock? And why does both exist? */

  if (Thread->ApcStateIndex == AttachedApcEnvironment)
  {
    /* NewProcess must be the same as in the original-environment */
    ASSERT(NewProcess == Thread->ApcStatePointer[OriginalApcEnvironment]->Process);

    /*    
    FIXME: Deliver any pending apc's queued to the attached environment before 
    switching back to the original environment.
    This is not crucial thou, since i don't think we'll ever target apc's
    to attached environments (or?)...   
    Remove the following asserts if implementing this.
    -Gunnar
    */

    /* we don't support targeting apc's at attached-environments (yet)... */
    ASSERT(IsListEmpty(&Thread->ApcState.ApcListHead[KernelMode]));
    ASSERT(IsListEmpty(&Thread->ApcState.ApcListHead[UserMode]));
    ASSERT(Thread->ApcState.KernelApcInProgress == FALSE);
    ASSERT(Thread->ApcState.KernelApcPending == FALSE);
    ASSERT(Thread->ApcState.UserApcPending == FALSE);
    
    /* restore backup of original environment */
    Thread->ApcState = Thread->SavedApcState;
    /* repair lists */
    RepairList(Thread->SavedApcState.ApcListHead, Thread->ApcState.ApcListHead,
               KernelMode);
    RepairList(Thread->SavedApcState.ApcListHead, Thread->ApcState.ApcListHead,
               UserMode);

    /* update environment pointers */
    Thread->ApcStatePointer[OriginalApcEnvironment] = &Thread->ApcState;
    Thread->ApcStatePointer[AttachedApcEnvironment] = &Thread->SavedApcState;

    /* update current-environment index */
    Thread->ApcStateIndex = OriginalApcEnvironment;
  }
  else if (Thread->ApcStateIndex == OriginalApcEnvironment)
  {
    /* backup original environment */
    Thread->SavedApcState = Thread->ApcState;
    /* repair lists */
    RepairList(Thread->ApcState.ApcListHead, Thread->SavedApcState.ApcListHead,
               KernelMode);
    RepairList(Thread->ApcState.ApcListHead, Thread->SavedApcState.ApcListHead,
               UserMode);

    /*
    FIXME: Is it possible to target an apc to an attached environment even if the 
    thread is not currently attached???? If so, then this is bougus since it 
    reinitializes the attached apc environment then located in SavedApcState.
    -Gunnar
    */

    /* setup a fresh new attached environment */
    InitializeListHead(&Thread->ApcState.ApcListHead[KernelMode]);
    InitializeListHead(&Thread->ApcState.ApcListHead[UserMode]);
    Thread->ApcState.Process = NewProcess;
    Thread->ApcState.KernelApcInProgress = FALSE;
    Thread->ApcState.KernelApcPending = FALSE;
    Thread->ApcState.UserApcPending = FALSE;

    /* update environment pointers */
    Thread->ApcStatePointer[OriginalApcEnvironment] = &Thread->SavedApcState;
    Thread->ApcStatePointer[AttachedApcEnvironment] = &Thread->ApcState;

    /* update current-environment index */
    Thread->ApcStateIndex = AttachedApcEnvironment;
  }
  else
  {
    /* FIXME: Is this the correct bug code? */
    KEBUGCHECK(APC_INDEX_MISMATCH);
  }
}

