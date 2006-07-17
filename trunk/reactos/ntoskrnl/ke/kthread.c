/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/kthread.c
 * PURPOSE:         Microkernel thread support
 *
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 *                  David Welch (welch@cwcom.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

extern EX_WORK_QUEUE ExWorkerQueue[MaximumWorkQueue];

/*
 * PURPOSE: List of threads associated with each priority level
 */
LIST_ENTRY PriorityListHead[MAXIMUM_PRIORITY];
static ULONG PriorityListMask = 0;
ULONG IdleProcessorMask = 0;
extern LIST_ENTRY PspReaperListHead;

ULONG KiMask32Array[MAXIMUM_PRIORITY] =
{
    0x1,        0x2,       0x4,       0x8,       0x10,       0x20,
    0x40,       0x80,      0x100,     0x200,     0x4000,     0x800,
    0x1000,     0x2000,    0x4000,    0x8000,    0x10000,    0x20000,
    0x40000,    0x80000,   0x100000,  0x200000,  0x400000,   0x800000,
    0x1000000,  0x2000000, 0x4000000, 0x8000000, 0x10000000, 0x20000000,
    0x40000000, 0x80000000
};

/* FUNCTIONS *****************************************************************/

UCHAR
NTAPI
KeFindNextRightSetAffinity(IN UCHAR Number,
                           IN ULONG Set)
{
    ULONG Bit, Result;
    ASSERT(Set != 0);

    /* Calculate the mask */
    Bit = (AFFINITY_MASK(Number) - 1) & Set;

    /* If it's 0, use the one we got */
    if (!Bit) Bit = Set;

    /* Now find the right set and return it */
    BitScanReverse(&Result, Bit);
    return (UCHAR)Result;
}

STATIC
VOID
KiRequestReschedule(CCHAR Processor)
{
    PKPCR Pcr;

    Pcr = (PKPCR)(KPCR_BASE + Processor * PAGE_SIZE);
    Pcr->Prcb->QuantumEnd = TRUE;
    KiIpiSendRequest(1 << Processor, IPI_DPC);
}

STATIC
VOID
KiInsertIntoThreadList(KPRIORITY Priority,
                       PKTHREAD Thread)
{
    ASSERT(Ready == Thread->State);
    ASSERT(Thread->Priority == Priority);

    if (Priority >= MAXIMUM_PRIORITY || Priority < LOW_PRIORITY) {

        DPRINT1("Invalid thread priority (%d)\n", Priority);
        KEBUGCHECK(0);
    }

    InsertTailList(&PriorityListHead[Priority], &Thread->WaitListEntry);
    PriorityListMask |= (1 << Priority);
}

STATIC
VOID
KiRemoveFromThreadList(PKTHREAD Thread)
{
    ASSERT(Ready == Thread->State);
    RemoveEntryList(&Thread->WaitListEntry);
    if (IsListEmpty(&PriorityListHead[(ULONG)Thread->Priority])) {

        PriorityListMask &= ~(1 << Thread->Priority);
    }
}

STATIC
PKTHREAD
KiScanThreadList(KPRIORITY Priority,
                 KAFFINITY Affinity)
{
    PKTHREAD current;
    ULONG Mask;

    Mask = (1 << Priority);

    if (PriorityListMask & Mask) {

        LIST_FOR_EACH(current, &PriorityListHead[Priority], KTHREAD, WaitListEntry) {

            if (current->State != Ready) {

                DPRINT1("%p/%d\n", current, current->State);
            }

            ASSERT(current->State == Ready);

            if (current->Affinity & Affinity) {

                KiRemoveFromThreadList(current);
                return(current);
            }
        }
    }

    return(NULL);
}

VOID
STDCALL
KiDispatchThreadNoLock(ULONG NewThreadStatus)
{
    KPRIORITY CurrentPriority;
    PKTHREAD Candidate;
    ULONG Affinity;
    PKTHREAD CurrentThread = KeGetCurrentThread();

    DPRINT("KiDispatchThreadNoLock() %d/%d/%d/%d\n", KeGetCurrentProcessorNumber(),
            CurrentThread, NewThreadStatus, CurrentThread->State);

    CurrentThread->State = (UCHAR)NewThreadStatus;

    if (NewThreadStatus == Ready) {

        KiInsertIntoThreadList(CurrentThread->Priority,
                               CurrentThread);
    }

    Affinity = 1 << KeGetCurrentProcessorNumber();

    for (CurrentPriority = HIGH_PRIORITY; CurrentPriority >= LOW_PRIORITY; CurrentPriority--) {

        Candidate = KiScanThreadList(CurrentPriority, Affinity);

        if (Candidate == CurrentThread) {

            Candidate->State = Running;
            KeReleaseDispatcherDatabaseLockFromDpcLevel();
            return;
        }

        if (Candidate != NULL) {

            PKTHREAD OldThread;
            PKTHREAD IdleThread;

            DPRINT("Scheduling %x(%d)\n",Candidate, CurrentPriority);

            Candidate->State = Running;

            OldThread = CurrentThread;
            CurrentThread = Candidate;
            IdleThread = KeGetCurrentPrcb()->IdleThread;

            if (OldThread == IdleThread) {

                IdleProcessorMask &= ~Affinity;

            } else if (CurrentThread == IdleThread) {

                IdleProcessorMask |= Affinity;
            }

            MmUpdatePageDir((PEPROCESS)PsGetCurrentProcess(),((PETHREAD)CurrentThread)->ThreadsProcess, sizeof(EPROCESS));

            /* Special note for Filip: This will release the Dispatcher DB Lock ;-) -- Alex */
            DPRINT("You are : %x, swapping to: %x\n", OldThread, CurrentThread);
            KiArchContextSwitch(CurrentThread);
            DPRINT("You are : %x, swapped from: %x\n", OldThread, CurrentThread);
            return;
        }
    }

    DPRINT1("CRITICAL: No threads are ready (CPU%d)\n", KeGetCurrentProcessorNumber());
    KEBUGCHECK(0);
}

NTSTATUS
NTAPI
KiSwapThread(VOID)
{
    PKTHREAD CurrentThread = KeGetCurrentThread();

    /* Find a new thread to run */
    DPRINT("Dispatching Thread as blocked\n");
    KiDispatchThreadNoLock(Waiting);

    /* Lower IRQL back */
    DPRINT("Lowering IRQL \n");
    KfLowerIrql(CurrentThread->WaitIrql);

    /* Return the wait status */
    return CurrentThread->WaitStatus;
}

VOID
STDCALL
KiDispatchThread(ULONG NewThreadStatus)
{
    KIRQL OldIrql;

    if (KeGetCurrentPrcb()->IdleThread == NULL) {
        return;
    }

    OldIrql = KeAcquireDispatcherDatabaseLock();
    KiDispatchThreadNoLock(NewThreadStatus);
    KeLowerIrql(OldIrql);
}

VOID
NTAPI
KiReadyThread(IN PKTHREAD Thread)
{
    /* Makes a thread ready */
    Thread->State = Ready;
    KiInsertIntoThreadList(Thread->Priority, Thread);
}

VOID
STDCALL
KiAdjustQuantumThread(IN PKTHREAD Thread)
{
    KPRIORITY Priority;

    /* Don't adjust for RT threads */
    if ((Thread->Priority < LOW_REALTIME_PRIORITY) &&
        Thread->BasePriority < LOW_REALTIME_PRIORITY - 2)
    {
        /* Decrease Quantum by one and see if we've ran out */
        if (--Thread->Quantum <= 0)
        {
            /* Return quantum */
            Thread->Quantum = Thread->QuantumReset;

            /* Calculate new Priority */
            Priority = Thread->Priority - (Thread->PriorityDecrement + 1);

            /* Normalize it if we've gone too low */
            if (Priority < Thread->BasePriority) Priority = Thread->BasePriority;

            /* Reset the priority decrement, we've done it */
            Thread->PriorityDecrement = 0;

            /* Set the new priority, if needed */
            if (Priority != Thread->Priority)
            {
                /* 
                 * FIXME: This should be a call to KiSetPriorityThread but
                 * due to the current ""scheduler"" in ROS, it can't be done
                 * cleanly since it actualyl dispatches threads instead.
                 */
                Thread->Priority = Priority;
            }
            else
            {
                /* FIXME: Priority hasn't changed, find a new thread */
            }
        }
    }

    /* Nothing to do... */
    return;
}


VOID
STDCALL
KiSuspendThreadKernelRoutine(PKAPC Apc,
                             PKNORMAL_ROUTINE* NormalRoutine,
                             PVOID* NormalContext,
                             PVOID* SystemArgument1,
                             PVOID* SystemArguemnt2)
{
}

VOID
STDCALL
KiSuspendThreadNormalRoutine(PVOID NormalContext,
                             PVOID SystemArgument1,
                             PVOID SystemArgument2)
{
    PKTHREAD CurrentThread = KeGetCurrentThread();

    /* Non-alertable kernel-mode suspended wait */
    DPRINT("Waiting...\n");
    KeWaitForSingleObject(&CurrentThread->SuspendSemaphore,
                          Suspended,
                          KernelMode,
                          FALSE,
                          NULL);
    DPRINT("Done Waiting\n");
}

#ifdef KeGetCurrentThread
#undef KeGetCurrentThread
#endif
/*
 * @implemented
 */
PKTHREAD
STDCALL
KeGetCurrentThread(VOID)
{
#ifdef CONFIG_SMP
    ULONG Flags;
    PKTHREAD Thread;
    Ke386SaveFlags(Flags);
    Ke386DisableInterrupts();
    Thread = KeGetCurrentPrcb()->CurrentThread;
    Ke386RestoreFlags(Flags);
    return Thread;
#else
    return(KeGetCurrentPrcb()->CurrentThread);
#endif
}

VOID
STDCALL
KeSetPreviousMode(ULONG Mode)
{
    PsGetCurrentThread()->Tcb.PreviousMode = (UCHAR)Mode;
}

/*
 * @implemented
 */
KPROCESSOR_MODE
STDCALL
KeGetPreviousMode(VOID)
{
    return (ULONG)PsGetCurrentThread()->Tcb.PreviousMode;
}

BOOLEAN
STDCALL
KeDisableThreadApcQueueing(IN PKTHREAD Thread)
{
    KIRQL OldIrql;
    BOOLEAN PreviousState;

    /* Lock the Dispatcher Database */
    OldIrql = KeAcquireDispatcherDatabaseLock();

    /* Save old state */
    PreviousState = Thread->ApcQueueable;

    /* Disable it now */
    Thread->ApcQueueable = FALSE;

    /* Release the Lock */
    KeReleaseDispatcherDatabaseLock(OldIrql);

    /* Return old state */
    return PreviousState;
}

VOID
STDCALL
KeRundownThread(VOID)
{
    KIRQL OldIrql;
    PKTHREAD Thread = KeGetCurrentThread();
    PLIST_ENTRY NextEntry, ListHead;
    PKMUTANT Mutant;
    DPRINT("KeRundownThread: %x\n", Thread);

    /* Optimized path if nothing is on the list at the moment */
    if (IsListEmpty(&Thread->MutantListHead)) return;

    /* Lock the Dispatcher Database */
    OldIrql = KeAcquireDispatcherDatabaseLock();

    /* Get the List Pointers */
    ListHead = &Thread->MutantListHead;
    NextEntry = ListHead->Flink;
    while (NextEntry != ListHead)
    {
        /* Get the Mutant */
        Mutant = CONTAINING_RECORD(NextEntry, KMUTANT, MutantListEntry);
        DPRINT1("Mutant: %p. Type, Size %x %x\n",
                 Mutant,
                 Mutant->Header.Type,
                 Mutant->Header.Size);

        /* Make sure it's not terminating with APCs off */
        if (Mutant->ApcDisable)
        {
            /* Bugcheck the system */
            KEBUGCHECKEX(0,//THREAD_TERMINATE_HELD_MUTEX,
                         (ULONG_PTR)Thread,
                         (ULONG_PTR)Mutant,
                         0,
                         0);
        }

        /* Now we can remove it */
        RemoveEntryList(&Mutant->MutantListEntry);

        /* Unconditionally abandon it */
        DPRINT("Abandonning the Mutant\n");
        Mutant->Header.SignalState = 1;
        Mutant->Abandoned = TRUE;
        Mutant->OwnerThread = NULL;

        /* Check if the Wait List isn't empty */
        DPRINT("Checking whether to wake the Mutant\n");
        if (!IsListEmpty(&Mutant->Header.WaitListHead))
        {
            /* Wake the Mutant */
            DPRINT("Waking the Mutant\n");
            KiWaitTest(&Mutant->Header, MUTANT_INCREMENT);
        }

        /* Move on */
        NextEntry = NextEntry->Flink;
    }

    /* Release the Lock */
    KeReleaseDispatcherDatabaseLock(OldIrql);
}

ULONG
STDCALL
KeResumeThread(PKTHREAD Thread)
{
    ULONG PreviousCount;
    KIRQL OldIrql;

    DPRINT("KeResumeThread (Thread %p called). %x, %x\n", Thread,
            Thread->SuspendCount, Thread->FreezeCount);

    /* Lock the Dispatcher */
    OldIrql = KeAcquireDispatcherDatabaseLock();

    /* Save the Old Count */
    PreviousCount = Thread->SuspendCount;

    /* Check if it existed */
    if (PreviousCount) {

        Thread->SuspendCount--;

        /* Decrease the current Suspend Count and Check Freeze Count */
        if ((!Thread->SuspendCount) && (!Thread->FreezeCount)) {

            /* Signal the Suspend Semaphore */
            Thread->SuspendSemaphore.Header.SignalState++;
            KiWaitTest(&Thread->SuspendSemaphore.Header, IO_NO_INCREMENT);
        }
    }

    /* Release Lock and return the Old State */
    KeReleaseDispatcherDatabaseLock(OldIrql);
    return PreviousCount;
}

VOID
FASTCALL
KiInsertQueueApc(PKAPC Apc,
                 KPRIORITY PriorityBoost);

/*
 * Used by the debugging code to freeze all the process's threads
 * while the debugger is examining their state.
 */
VOID
STDCALL
KeFreezeAllThreads(PKPROCESS Process)
{
    KIRQL OldIrql;
    PKTHREAD Current;
    PKTHREAD CurrentThread = KeGetCurrentThread();

    /* Acquire Lock */
    OldIrql = KeAcquireDispatcherDatabaseLock();

    /* If someone is already trying to free us, try again */
    while (CurrentThread->FreezeCount)
    {
        /* Release and re-acquire the lock so the APC will go through */
        KeReleaseDispatcherDatabaseLock(OldIrql);
        OldIrql = KeAcquireDispatcherDatabaseLock();
    }

    /* Enter a critical region */
    KeEnterCriticalRegion();

    /* Loop the Process's Threads */
    LIST_FOR_EACH(Current, &Process->ThreadListHead, KTHREAD, ThreadListEntry)
    {
        /* Make sure it's not ours */
        if (Current != CurrentThread)
        {
            /* Should be bother inserting the APC? */
            if (Current->ApcQueueable)
            {
                /* Make sure it wasn't already frozen, and that it's not suspended */
                if (!(++Current->FreezeCount) && !(Current->SuspendCount))
                {
                    /* Did we already insert it? */
                    if (!Current->SuspendApc.Inserted)
                    {
                        /* Insert the APC */
                        Current->SuspendApc.Inserted = TRUE;
                        KiInsertQueueApc(&Current->SuspendApc, IO_NO_INCREMENT);
                    }
                    else
                    {
                        /* Unsignal the Semaphore, the APC already got inserted */
                        Current->SuspendSemaphore.Header.SignalState--;
                    }
                }
            }
        }
    }

    /* Release the lock */
    KeReleaseDispatcherDatabaseLock(OldIrql);
}

NTSTATUS
STDCALL
KeSuspendThread(PKTHREAD Thread)
{
    ULONG PreviousCount;
    KIRQL OldIrql;

    DPRINT("KeSuspendThread (Thread %p called). %x, %x\n", Thread, Thread->SuspendCount, Thread->FreezeCount);

    /* Lock the Dispatcher */
    OldIrql = KeAcquireDispatcherDatabaseLock();

    /* Save the Old Count */
    PreviousCount = Thread->SuspendCount;

    /* Handle the maximum */
    if (PreviousCount == MAXIMUM_SUSPEND_COUNT)
    {
        /* Raise an exception */
        KeReleaseDispatcherDatabaseLock(OldIrql);
        RtlRaiseStatus(STATUS_SUSPEND_COUNT_EXCEEDED);
    }

    /* Should we bother to queue at all? */
    if (Thread->ApcQueueable)
    {
        /* Increment the suspend count */
        Thread->SuspendCount++;

        /* Check if we should suspend it */
        if (!PreviousCount && !Thread->FreezeCount)
        {
            /* Is the APC already inserted? */
            if (!Thread->SuspendApc.Inserted)
            {
                /* Not inserted, insert it */
                Thread->SuspendApc.Inserted = TRUE;
                KiInsertQueueApc(&Thread->SuspendApc, IO_NO_INCREMENT);
            }
            else
            {
                /* Unsignal the Semaphore, the APC already got inserted */
                Thread->SuspendSemaphore.Header.SignalState--;
            }
        }
    }

    /* Release Lock and return the Old State */
    KeReleaseDispatcherDatabaseLock(OldIrql);
    return PreviousCount;
}

ULONG
STDCALL
KeForceResumeThread(IN PKTHREAD Thread)
{
    KIRQL OldIrql;
    ULONG PreviousCount;

    /* Lock the Dispatcher Database and the APC Queue */
    OldIrql = KeAcquireDispatcherDatabaseLock();

    /* Save the old Suspend Count */
    PreviousCount = Thread->SuspendCount + Thread->FreezeCount;

    /* If the thread is suspended, wake it up!!! */
    if (PreviousCount) {

        /* Unwait it completely */
        Thread->SuspendCount = 0;
        Thread->FreezeCount = 0;

        /* Signal and satisfy */
        Thread->SuspendSemaphore.Header.SignalState++;
        KiWaitTest(&Thread->SuspendSemaphore.Header, IO_NO_INCREMENT);
    }

    /* Release Lock and return the Old State */
    KeReleaseDispatcherDatabaseLock(OldIrql);
    return PreviousCount;
}

ULONG
STDCALL
KeAlertResumeThread(IN PKTHREAD Thread)
{
    ULONG PreviousCount;
    KIRQL OldIrql;

    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Lock the Dispatcher Database and the APC Queue */
    OldIrql = KeAcquireDispatcherDatabaseLock();
    KiAcquireSpinLock(&Thread->ApcQueueLock);

    /* Return if Thread is already alerted. */
    if (Thread->Alerted[KernelMode] == FALSE) {

        /* If it's Blocked, unblock if it we should */
        if (Thread->State == Waiting &&  Thread->Alertable) {

            DPRINT("Aborting Wait\n");
            KiAbortWaitThread(Thread, STATUS_ALERTED, THREAD_ALERT_INCREMENT);

        } else {

            /* If not, simply Alert it */
            Thread->Alerted[KernelMode] = TRUE;
        }
    }

    /* Save the old Suspend Count */
    PreviousCount = Thread->SuspendCount;

    /* If the thread is suspended, decrease one of the suspend counts */
    if (PreviousCount) {

        /* Decrease count. If we are now zero, unwait it completely */
        if (--Thread->SuspendCount) {

            /* Signal and satisfy */
            Thread->SuspendSemaphore.Header.SignalState++;
            KiWaitTest(&Thread->SuspendSemaphore.Header, IO_NO_INCREMENT);
        }
    }

    /* Release Locks and return the Old State */
    KiReleaseSpinLock(&Thread->ApcQueueLock);
    KeReleaseDispatcherDatabaseLock(OldIrql);
    return PreviousCount;
}

BOOLEAN
STDCALL
KeAlertThread(PKTHREAD Thread,
              KPROCESSOR_MODE AlertMode)
{
    KIRQL OldIrql;
    BOOLEAN PreviousState;

    /* Acquire the Dispatcher Database Lock */
    OldIrql = KeAcquireDispatcherDatabaseLock();

    /* Save the Previous State */
    PreviousState = Thread->Alerted[AlertMode];

    /* Return if Thread is already alerted. */
    if (PreviousState == FALSE) {

        /* If it's Blocked, unblock if it we should */
        if (Thread->State == Waiting &&
            (AlertMode == KernelMode || Thread->WaitMode == AlertMode) &&
            Thread->Alertable) {

            DPRINT("Aborting Wait\n");
            KiAbortWaitThread(Thread, STATUS_ALERTED, THREAD_ALERT_INCREMENT);

        } else {

            /* If not, simply Alert it */
            Thread->Alerted[AlertMode] = TRUE;
        }
    }

    /* Release the Dispatcher Lock */
    KeReleaseDispatcherDatabaseLock(OldIrql);

    /* Return the old state */
    return PreviousState;
}

/*
 * @unimplemented
 */
VOID
STDCALL
KeCapturePersistentThreadState(IN PVOID CurrentThread,
                               IN ULONG Setting1,
                               IN ULONG Setting2,
                               IN ULONG Setting3,
                               IN ULONG Setting4,
                               IN ULONG Setting5,
                               IN PVOID ThreadState)
{
    UNIMPLEMENTED;
}

NTSTATUS
NTAPI
KeInitThread(IN OUT PKTHREAD Thread,
             IN PVOID KernelStack,
             IN PKSYSTEM_ROUTINE SystemRoutine,
             IN PKSTART_ROUTINE StartRoutine,
             IN PVOID StartContext,
             IN PCONTEXT Context,
             IN PVOID Teb,
             IN PKPROCESS Process)
{
    BOOLEAN AllocatedStack = FALSE;
    ULONG i;
    PKWAIT_BLOCK TimerWaitBlock;
    PKTIMER Timer;
    NTSTATUS Status;

    /* Initalize the Dispatcher Header */
    KeInitializeDispatcherHeader(&Thread->DispatcherHeader,
                                 ThreadObject,
                                 sizeof(KTHREAD) / sizeof(LONG),
                                 FALSE);

    /* Initialize the Mutant List */
    InitializeListHead(&Thread->MutantListHead);

    /* Initialize the wait blocks */
    for (i = 0; i< (THREAD_WAIT_OBJECTS + 1); i++)
    {
        /* Put our pointer */
        Thread->WaitBlock[i].Thread = Thread;
    }

    /* Set swap settings */
    Thread->EnableStackSwap = FALSE;//TRUE;
    Thread->IdealProcessor = 1;
    Thread->SwapBusy = FALSE;
    Thread->AdjustReason = 0;

    /* Initialize the lock */
    KeInitializeSpinLock(&Thread->ThreadLock);

    /* Setup the Service Descriptor Table for Native Calls */
    Thread->ServiceTable = KeServiceDescriptorTable;

    /* Setup APC Fields */
    InitializeListHead(&Thread->ApcState.ApcListHead[0]);
    InitializeListHead(&Thread->ApcState.ApcListHead[1]);
    Thread->ApcState.Process = Process;
    Thread->ApcStatePointer[OriginalApcEnvironment] = &Thread->ApcState;
    Thread->ApcStatePointer[AttachedApcEnvironment] = &Thread->SavedApcState;
    Thread->ApcStateIndex = OriginalApcEnvironment;
    Thread->ApcQueueable = TRUE;
    KeInitializeSpinLock(&Thread->ApcQueueLock);

    /* Initialize the Suspend APC */
    KeInitializeApc(&Thread->SuspendApc,
                    Thread,
                    OriginalApcEnvironment,
                    KiSuspendThreadKernelRoutine,
                    NULL,
                    KiSuspendThreadNormalRoutine,
                    KernelMode,
                    NULL);

    /* Initialize the Suspend Semaphore */
    KeInitializeSemaphore(&Thread->SuspendSemaphore, 0, 2);

    /* Setup the timer */
    Timer = &Thread->Timer;
    KeInitializeTimer(Timer);
    TimerWaitBlock = &Thread->WaitBlock[TIMER_WAIT_BLOCK];
    TimerWaitBlock->Object = Timer;
    TimerWaitBlock->WaitKey = STATUS_TIMEOUT;
    TimerWaitBlock->WaitType = WaitAny;
    TimerWaitBlock->NextWaitBlock = NULL;

    /* Link the two wait lists together */
    TimerWaitBlock->WaitListEntry.Flink = &Timer->Header.WaitListHead;
    TimerWaitBlock->WaitListEntry.Blink = &Timer->Header.WaitListHead;

    /* Set the TEB */
    Thread->Teb = Teb;

    /* Check if we have a kernel stack */
    if (!KernelStack)
    {
        /* We don't, allocate one */
        KernelStack = (PVOID)((ULONG_PTR)MmCreateKernelStack(FALSE) +
                              KERNEL_STACK_SIZE);
        if (!KernelStack) return STATUS_INSUFFICIENT_RESOURCES;

        /* Remember for later */
        AllocatedStack = TRUE;
    }

    /* Set the Thread Stacks */
    Thread->InitialStack = (PCHAR)KernelStack;
    Thread->StackBase = (PCHAR)KernelStack;
    Thread->StackLimit = (ULONG_PTR)KernelStack - KERNEL_STACK_SIZE;
    Thread->KernelStackResident = TRUE;

    /* ROS Mm HACK */
    MmUpdatePageDir((PEPROCESS)Process,
                    (PVOID)Thread->StackLimit,
                    KERNEL_STACK_SIZE);
    MmUpdatePageDir((PEPROCESS)Process, (PVOID)Thread, sizeof(ETHREAD));

    /* Enter SEH to avoid crashes due to user mode */
    Status = STATUS_SUCCESS;
    _SEH_TRY
    {
        /* Initalize the Thread Context */
        KiArchInitThreadWithContext(Thread,
                                    SystemRoutine,
                                    StartRoutine,
                                    StartContext,
                                    Context);
    }
    _SEH_HANDLE
    {
        /* Set failure status */
        Status = STATUS_UNSUCCESSFUL;

        /* Check if a stack was allocated */
        if (AllocatedStack)
        {
            /* Delete the stack */
            MmDeleteKernelStack(Thread->StackBase, FALSE);
            Thread->InitialStack = NULL;
        }
    }
    _SEH_END;

    /* Set the Thread to initalized */
    Thread->State = Initialized;
    return Status;
}

VOID
NTAPI
KeStartThread(IN OUT PKTHREAD Thread)
{
    KIRQL OldIrql;
    PKPROCESS Process = Thread->ApcState.Process;
    PKNODE Node;
    PKPRCB NodePrcb;
    ULONG Set, Mask;
    UCHAR IdealProcessor;

    /* Setup static fields from parent */
    Thread->Iopl = Process->Iopl;
    Thread->Quantum = Process->QuantumReset;
    Thread->QuantumReset = Process->QuantumReset;
    Thread->SystemAffinityActive = FALSE;

    /* Lock the process */
    KeAcquireSpinLock(&Process->ProcessLock, &OldIrql);

    /* Setup volatile data */
    Thread->Priority = Process->BasePriority;
    Thread->BasePriority = Process->BasePriority;
    Thread->Affinity = Process->Affinity;
    Thread->UserAffinity = Process->Affinity;

    /* Get the KNODE and its PRCB */
    Node = KeNodeBlock[Process->IdealNode];
    NodePrcb = (PKPRCB)(KPCR_BASE + (Process->ThreadSeed * PAGE_SIZE));

    /* Calculate affinity mask */
    Set = ~NodePrcb->MultiThreadProcessorSet;
    Mask = (ULONG)(Node->ProcessorMask & Process->Affinity);
    Set &= Mask;
    if (Set) Mask = Set;

    /* Get the new thread seed */
    IdealProcessor = KeFindNextRightSetAffinity(Process->ThreadSeed, Mask);
    Process->ThreadSeed = IdealProcessor;

    /* Sanity check */
    ASSERT((Thread->UserAffinity & AFFINITY_MASK(IdealProcessor)));

    /* Set the Ideal Processor */
    Thread->IdealProcessor = IdealProcessor;
    Thread->UserIdealProcessor = IdealProcessor;

    /* Lock the Dispatcher Database */
    KeAcquireDispatcherDatabaseLockAtDpcLevel();

    /* Insert the thread into the process list */
    InsertTailList(&Process->ThreadListHead, &Thread->ThreadListEntry);

    /* Increase the stack count */
    ASSERT(Process->StackCount != MAXULONG_PTR);
    Process->StackCount++;

    /* Release locks and return */
    KeReleaseDispatcherDatabaseLockFromDpcLevel();
    KeReleaseSpinLock(&Process->ProcessLock, OldIrql);
}

VOID
NTAPI
KeInitializeThread(IN PKPROCESS Process,
                   IN OUT PKTHREAD Thread,
                   IN PKSYSTEM_ROUTINE SystemRoutine,
                   IN PKSTART_ROUTINE StartRoutine,
                   IN PVOID StartContext,
                   IN PCONTEXT Context,
                   IN PVOID Teb,
                   IN PVOID KernelStack)
{
    /* Initailize and start the thread on success */
    if (NT_SUCCESS(KeInitThread(Thread,
                                KernelStack,
                                SystemRoutine,
                                StartRoutine,
                                StartContext,
                                Context,
                                Teb,
                                Process)))
    {
        /* Start it */
        KeStartThread(Thread);
    }
}

/*
 * @implemented
 */
KPRIORITY
STDCALL
KeQueryPriorityThread (IN PKTHREAD Thread)
{
    return Thread->Priority;
}

/*
 * @implemented
 */
ULONG
STDCALL
KeQueryRuntimeThread(IN PKTHREAD Thread,
                     OUT PULONG UserTime)
{
    /* Return the User Time */
    *UserTime = Thread->UserTime;

    /* Return the Kernel Time */
    return Thread->KernelTime;
}

/*
 * @implemented
 */
BOOLEAN
STDCALL
KeSetKernelStackSwapEnable(IN BOOLEAN Enable)
{
    PKTHREAD Thread = KeGetCurrentThread();
    BOOLEAN PreviousState;
    KIRQL OldIrql;

     /* Lock the Dispatcher Database */
    OldIrql = KeAcquireDispatcherDatabaseLock();

    /* Save Old State */
    PreviousState = Thread->EnableStackSwap;

    /* Set New State */
    Thread->EnableStackSwap = Enable;

    /* No, Release Lock */
    KeReleaseDispatcherDatabaseLock(OldIrql);

    /* Return Old State */
    return PreviousState;
}

/*
 * @implemented
 */
VOID
STDCALL
KeRevertToUserAffinityThread(VOID)
{
    PKTHREAD CurrentThread = KeGetCurrentThread();
    KIRQL OldIrql;

    ASSERT(CurrentThread->SystemAffinityActive != FALSE);

    /* Lock the Dispatcher Database */
    OldIrql = KeAcquireDispatcherDatabaseLock();

    /* Return to User Affinity */
    CurrentThread->Affinity = CurrentThread->UserAffinity;

    /* Disable System Affinity */
    CurrentThread->SystemAffinityActive = FALSE;

    /* Check if we need to Dispatch a New thread */
    if (CurrentThread->Affinity & (1 << KeGetCurrentProcessorNumber())) {

        /* No, just release */
        KeReleaseDispatcherDatabaseLock(OldIrql);

    } else {

        /* We need to dispatch a new thread */
        CurrentThread->WaitIrql = OldIrql;
        KiDispatchThreadNoLock(Ready);
        KeLowerIrql(OldIrql);
    }
}

/*
 * @implemented
 */
CCHAR
STDCALL
KeSetIdealProcessorThread(IN PKTHREAD Thread,
                          IN CCHAR Processor)
{
    CCHAR PreviousIdealProcessor;
    KIRQL OldIrql;

    /* Lock the Dispatcher Database */
    OldIrql = KeAcquireDispatcherDatabaseLock();

    /* Save Old Ideal Processor */
    PreviousIdealProcessor = Thread->IdealProcessor;

    /* Set New Ideal Processor */
    Thread->IdealProcessor = Processor;

    /* Release Lock */
    KeReleaseDispatcherDatabaseLock(OldIrql);

    /* Return Old Ideal Processor */
    return PreviousIdealProcessor;
}

/*
 * @implemented
 */
VOID
STDCALL
KeSetSystemAffinityThread(IN KAFFINITY Affinity)
{
    PKTHREAD CurrentThread = KeGetCurrentThread();
    KIRQL OldIrql;

    ASSERT(Affinity & ((1 << KeNumberProcessors) - 1));

    /* Lock the Dispatcher Database */
    OldIrql = KeAcquireDispatcherDatabaseLock();

    /* Set the System Affinity Specified */
    CurrentThread->Affinity = Affinity;

    /* Enable System Affinity */
    CurrentThread->SystemAffinityActive = TRUE;

    /* Check if we need to Dispatch a New thread */
    if (Affinity & (1 << KeGetCurrentProcessorNumber())) {

        /* No, just release */
        KeReleaseDispatcherDatabaseLock(OldIrql);

    } else {

        /* We need to dispatch a new thread */
        CurrentThread->WaitIrql = OldIrql;
        KiDispatchThreadNoLock(Ready);
        KeLowerIrql(OldIrql);
    }
}

LONG
STDCALL
KeQueryBasePriorityThread(IN PKTHREAD Thread)
{
    LONG BasePriorityIncrement;
    KIRQL OldIrql;
    PKPROCESS Process;

    /* Lock the Dispatcher Database */
    OldIrql = KeAcquireDispatcherDatabaseLock();

    /* Get the Process */
    Process = Thread->ApcStatePointer[0]->Process;

    /* Calculate the BPI */
    BasePriorityIncrement = Thread->BasePriority - Process->BasePriority;

    /* If saturation occured, return the SI instead */
    if (Thread->Saturation) BasePriorityIncrement = (HIGH_PRIORITY + 1) / 2 *
                                                    Thread->Saturation;

    /* Release Lock */
    KeReleaseDispatcherDatabaseLock(OldIrql);

    /* Return Increment */
    return BasePriorityIncrement;
}

VOID
STDCALL
KiSetPriorityThread(PKTHREAD Thread,
                    KPRIORITY Priority,
                    PBOOLEAN Released)
{
    KPRIORITY OldPriority = Thread->Priority;
    ULONG Mask;
    int i;
    PKPCR Pcr;
    DPRINT("Changing prio to : %lx\n", Priority);

    /* Check if priority changed */
    if (OldPriority != Priority)
    {
        /* Set it */
        Thread->Priority = Priority;

        /* Choose action based on thread's state */
        if (Thread->State == Ready)
        {
            /* Remove it from the current queue */
            KiRemoveFromThreadList(Thread);
            
            /* Re-insert it at its current priority */
            KiInsertIntoThreadList(Priority, Thread);

            /* Check if the old priority was lower */
            if (KeGetCurrentThread()->Priority < Priority)
            {
                /* Dispatch it immediately */
                KiDispatchThreadNoLock(Ready);
                *Released = TRUE;
                return;
            }
        }
        else if (Thread->State == Running)
        {
            /* Check if the new priority is lower */
            if (Priority < OldPriority)
            {
                /* Check for threads with a higher priority */
                Mask = ~((1 << (Priority + 1)) - 1);
                if (PriorityListMask & Mask)
                {
                    /* Found a thread, is it us? */
                    if (Thread == KeGetCurrentThread())
                    {
                        /* Dispatch us */
                        KiDispatchThreadNoLock(Ready);
                        *Released = TRUE;
                        return;
                    } 
                    else
                    {
                        /* Loop every CPU */
                        for (i = 0; i < KeNumberProcessors; i++)
                        {
                            /* Get the PCR for this CPU */
                            Pcr = (PKPCR)(KPCR_BASE + i * PAGE_SIZE);

                            /* Reschedule if the new one is already on a CPU */
                            if (Pcr->Prcb->CurrentThread == Thread)
                            {
                                KeReleaseDispatcherDatabaseLockFromDpcLevel();
                                KiRequestReschedule(i);
                                *Released = TRUE;
                                return;
                            }
                        }
                    }
                }
            }
        }
    }

    /* Return to caller */
    return;
}

/*
 * @implemented
 */
LONG
STDCALL
KeSetBasePriorityThread(PKTHREAD Thread,
                        LONG Increment)
{
    KIRQL OldIrql;
    PKPROCESS Process;
    KPRIORITY Priority;
    KPRIORITY CurrentBasePriority;
    KPRIORITY BasePriority;
    BOOLEAN Released = FALSE;
    LONG CurrentIncrement;
       
    /* Lock the Dispatcher Database */
    OldIrql = KeAcquireDispatcherDatabaseLock();

    /* Get the process and calculate current BP and BPI */
    Process = Thread->ApcStatePointer[0]->Process;
    CurrentBasePriority = Thread->BasePriority;
    CurrentIncrement = CurrentBasePriority - Process->BasePriority;

    /* Change to use the SI if Saturation was used */
    if (Thread->Saturation) CurrentIncrement = (HIGH_PRIORITY + 1) / 2 *
                                               Thread->Saturation;

    /* Now check if saturation is being used for the new value */
    if (abs(Increment) >= ((HIGH_PRIORITY + 1) / 2))
    {
        /* Check if we need positive or negative saturation */
        Thread->Saturation = (Increment > 0) ? 1 : -1;
    }

    /* Normalize the Base Priority */
    BasePriority = Process->BasePriority + Increment;
    if (Process->BasePriority >= LOW_REALTIME_PRIORITY)
    {
        /* Check if it's too low */
        if (BasePriority < LOW_REALTIME_PRIORITY)
            BasePriority = LOW_REALTIME_PRIORITY;

        /* Check if it's too high */
        if (BasePriority > HIGH_PRIORITY) BasePriority = HIGH_PRIORITY;

        /* We are at RTP, so use the raw BP */
        Priority = BasePriority;
    }
    else
    {
        /* Check if it's entering RTP */
        if (BasePriority >= LOW_REALTIME_PRIORITY)
            BasePriority = LOW_REALTIME_PRIORITY - 1;

        /* Check if it's too low */
        if (BasePriority <= LOW_PRIORITY)
            BasePriority = 1;

        /* If Saturation is used, then use the raw BP */
        if (Thread->Saturation)
        {
            Priority = BasePriority;
        }
        else
        {
            /* Calculate the new priority */
            Priority = Thread->Priority + (BasePriority - CurrentBasePriority)-
                       Thread->PriorityDecrement;

            /* Make sure it won't enter RTP ranges */
            if (Priority >= LOW_REALTIME_PRIORITY)
                Priority = LOW_REALTIME_PRIORITY - 1;
        }
    }

    /* Finally set the new base priority */
    Thread->BasePriority = BasePriority;

    /* Reset the decrements */
    Thread->PriorityDecrement = 0;

    /* If the priority will change, reset quantum and change it for real */
    if (Priority != Thread->Priority)
    {
        Thread->Quantum = Thread->QuantumReset;
        KiSetPriorityThread(Thread, Priority, &Released);
    }

    /* Release Lock if needed */
    if (!Released)
    {
        KeReleaseDispatcherDatabaseLock(OldIrql);
    }
    else
    {
        KeLowerIrql(OldIrql);
    }

    /* Return the Old Increment */
    return CurrentIncrement;
}

/*
 * @implemented
 */
KPRIORITY
STDCALL
KeSetPriorityThread(PKTHREAD Thread,
                    KPRIORITY Priority)
{
    KPRIORITY OldPriority;
    BOOLEAN Released = FALSE;
    KIRQL OldIrql;

    /* Lock the Dispatcher Database */
    OldIrql = KeAcquireDispatcherDatabaseLock();

    /* Save the old Priority */
    OldPriority = Thread->Priority;

    /* Reset the Quantum and Decrements */
    Thread->Quantum = Thread->QuantumReset;
    Thread->PriorityDecrement = 0;

    /* Set the new Priority */
    KiSetPriorityThread(Thread, Priority, &Released);

    /* Release Lock if needed */
    if (!Released)
    {
        KeReleaseDispatcherDatabaseLock(OldIrql);
    }
    else
    {
        KeLowerIrql(OldIrql);
    }

    /* Return Old Priority */
    return OldPriority;
}

/*
 * @implemented
 *
 * Sets thread's affinity
 */
NTSTATUS
STDCALL
KeSetAffinityThread(PKTHREAD Thread,
                    KAFFINITY Affinity)
{
    KIRQL OldIrql;
    LONG i;
    PKPCR Pcr;
    KAFFINITY ProcessorMask;

    DPRINT("KeSetAffinityThread(Thread %x, Affinity %x)\n", Thread, Affinity);

    /* Verify correct affinity */
    if ((Affinity & Thread->ApcStatePointer[0]->Process->Affinity) !=
        Affinity || !Affinity)
    {
        KEBUGCHECK(INVALID_AFFINITY_SET);
    }

    OldIrql = KeAcquireDispatcherDatabaseLock();

    Thread->UserAffinity = Affinity;

    if (Thread->SystemAffinityActive == FALSE) {

        Thread->Affinity = Affinity;

        if (Thread->State == Running) {

            ProcessorMask = 1 << KeGetCurrentProcessorNumber();
            if (Thread == KeGetCurrentThread()) {

                if (!(Affinity & ProcessorMask)) {

                    KiDispatchThreadNoLock(Ready);
                    KeLowerIrql(OldIrql);
                    return STATUS_SUCCESS;
                }

            } else {

                for (i = 0; i < KeNumberProcessors; i++) {

                    Pcr = (PKPCR)(KPCR_BASE + i * PAGE_SIZE);
                    if (Pcr->Prcb->CurrentThread == Thread) {

                        if (!(Affinity & ProcessorMask)) {

                            KeReleaseDispatcherDatabaseLockFromDpcLevel();
                            KiRequestReschedule(i);
                            KeLowerIrql(OldIrql);
                            return STATUS_SUCCESS;
                        }

                        break;
                    }
                }

                ASSERT (i < KeNumberProcessors);
            }
        }
    }

    KeReleaseDispatcherDatabaseLock(OldIrql);
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
 /* The Increment Argument seems to be ignored by NT and always 0 when called */
VOID
STDCALL
KeTerminateThread(IN KPRIORITY Increment)
{
    KIRQL OldIrql;
    PKTHREAD Thread = KeGetCurrentThread();
    PKPROCESS Process = Thread->ApcState.Process;
    PLIST_ENTRY *ListHead;
    PETHREAD Entry, SavedEntry;
    PETHREAD *ThreadAddr;
    DPRINT("Terminating\n");

    /* Lock the Dispatcher Database and the APC Queue */
    ASSERT_IRQL(DISPATCH_LEVEL);
    OldIrql = KeAcquireDispatcherDatabaseLock();
    ASSERT(Thread->SwapBusy == FALSE);

    /* Make sure we won't get Swapped */
    Thread->SwapBusy = TRUE;

    /* Save the Kernel and User Times */
    Process->KernelTime += Thread->KernelTime;
    Process->UserTime += Thread->UserTime;

    /* Get the current entry and our Port */
    Entry = (PETHREAD)PspReaperListHead.Flink;
    ThreadAddr = &((PETHREAD)Thread)->ReaperLink;

    /* Add it to the reaper's list */
    do
    {
        /* Get the list head */
        ListHead = &PspReaperListHead.Flink;

        /* Link ourselves */
        *ThreadAddr = Entry;
        SavedEntry = Entry;

        /* Now try to do the exchange */
        Entry = InterlockedCompareExchangePointer(ListHead, ThreadAddr, Entry);

        /* Break out if the change was succesful */
    } while (Entry != SavedEntry);

    /* Check if the reaper wasn't active */
    if (!Entry)
    {
        /* Activate it as a work item, directly through its Queue */
        KiInsertQueue(&ExWorkerQueue[HyperCriticalWorkQueue].WorkerQueue,
                      &PspReaperWorkItem.List,
                      FALSE);
    }

    /* Handle Kernel Queues */
    if (Thread->Queue)
    {
        DPRINT("Waking Queue\n");
        RemoveEntryList(&Thread->QueueListEntry);
        KiWakeQueue(Thread->Queue);
    }

    /* Signal the thread */
    Thread->DispatcherHeader.SignalState = TRUE;
    if (IsListEmpty(&Thread->DispatcherHeader.WaitListHead) != TRUE)
    {
        /* Satisfy waits */
        KiWaitTest((PVOID)Thread, Increment);
    }

    /* Remove the thread from the list */
    RemoveEntryList(&Thread->ThreadListEntry);

    /* Set us as terminated, decrease the Process's stack count */
    Thread->State = Terminated;
    Process->StackCount--;

    /* Find a new Thread */
    KiDispatchThreadNoLock(Terminated);
}

/*
 * FUNCTION: Tests whether there are any pending APCs for the current thread
 * and if so the APCs will be delivered on exit from kernel mode
 */
BOOLEAN
STDCALL
KeTestAlertThread(IN KPROCESSOR_MODE AlertMode)
{
    KIRQL OldIrql;
    PKTHREAD Thread = KeGetCurrentThread();
    BOOLEAN OldState;

    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Lock the Dispatcher Database and the APC Queue */
    OldIrql = KeAcquireDispatcherDatabaseLock();
    KiAcquireSpinLock(&Thread->ApcQueueLock);

    /* Save the old State */
    OldState = Thread->Alerted[AlertMode];

    /* If the Thread is Alerted, Clear it */
    if (OldState) {

        Thread->Alerted[AlertMode] = FALSE;

    } else if ((AlertMode != KernelMode) && (!IsListEmpty(&Thread->ApcState.ApcListHead[UserMode]))) {

        /* If the mode is User and the Queue isn't empty, set Pending */
        Thread->ApcState.UserApcPending = TRUE;
    }

    /* Release Locks and return the Old State */
    KiReleaseSpinLock(&Thread->ApcQueueLock);
    KeReleaseDispatcherDatabaseLock(OldIrql);
    return OldState;
}

/*
 *
 * NOT EXPORTED
 */
NTSTATUS
STDCALL
NtAlertResumeThread(IN  HANDLE ThreadHandle,
                    OUT PULONG SuspendCount)
{
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    PETHREAD Thread;
    NTSTATUS Status;
    ULONG PreviousState;

    /* Check if parameters are valid */
    if(PreviousMode != KernelMode) {

        _SEH_TRY {

            ProbeForWriteUlong(SuspendCount);

        } _SEH_HANDLE {

            Status = _SEH_GetExceptionCode();

        } _SEH_END;
    }

    /* Reference the Object */
    Status = ObReferenceObjectByHandle(ThreadHandle,
                                       THREAD_SUSPEND_RESUME,
                                       PsThreadType,
                                       PreviousMode,
                                       (PVOID*)&Thread,
                                       NULL);

    /* Check for Success */
    if (NT_SUCCESS(Status)) {

        /* Call the Kernel Function */
        PreviousState = KeAlertResumeThread(&Thread->Tcb);

        /* Dereference Object */
        ObDereferenceObject(Thread);

        if (SuspendCount) {

            _SEH_TRY {

                *SuspendCount = PreviousState;

            } _SEH_HANDLE {

                Status = _SEH_GetExceptionCode();

            } _SEH_END;
        }
    }

    /* Return status */
    return Status;
}

/*
 * @implemented
 *
 * EXPORTED
 */
NTSTATUS
STDCALL
NtAlertThread (IN HANDLE ThreadHandle)
{
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    PETHREAD Thread;
    NTSTATUS Status;

    /* Reference the Object */
    Status = ObReferenceObjectByHandle(ThreadHandle,
                                       THREAD_SUSPEND_RESUME,
                                       PsThreadType,
                                       PreviousMode,
                                       (PVOID*)&Thread,
                                       NULL);

    /* Check for Success */
    if (NT_SUCCESS(Status)) {

        /*
         * Do an alert depending on the processor mode. If some kmode code wants to
         * enforce a umode alert it should call KeAlertThread() directly. If kmode
         * code wants to do a kmode alert it's sufficient to call it with Zw or just
         * use KeAlertThread() directly
         */
        KeAlertThread(&Thread->Tcb, PreviousMode);

        /* Dereference Object */
        ObDereferenceObject(Thread);
    }

    /* Return status */
    return Status;
}

NTSTATUS
STDCALL
NtDelayExecution(IN BOOLEAN Alertable,
                 IN PLARGE_INTEGER DelayInterval)
{
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    LARGE_INTEGER SafeInterval;
    NTSTATUS Status;

    /* Check if parameters are valid */
    if(PreviousMode != KernelMode) {

        Status = STATUS_SUCCESS;
        
        _SEH_TRY {

            /* make a copy on the kernel stack and let DelayInterval point to it so
               we don't need to wrap KeDelayExecutionThread in SEH! */
            SafeInterval = ProbeForReadLargeInteger(DelayInterval);
            DelayInterval = &SafeInterval;

        } _SEH_HANDLE {

            Status = _SEH_GetExceptionCode();
        } _SEH_END;
        
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }
   }

   /* Call the Kernel Function */
   Status = KeDelayExecutionThread(PreviousMode,
                                   Alertable,
                                   DelayInterval);

   /* Return Status */
   return Status;
}
