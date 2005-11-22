/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ps/thread.c
 * PURPOSE:         Thread managment
 *
 * PROGRAMMERS:     David Welch (welch@mcmail.com)
 *                  Phillip Susi
 */

/* INCLUDES ****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS ******************************************************************/

extern LIST_ENTRY PsActiveProcessHead;
extern PEPROCESS PsIdleProcess;
extern PVOID PspSystemDllEntryPoint;
extern PHANDLE_TABLE PspCidTable;

POBJECT_TYPE PsThreadType = NULL;

/* FUNCTIONS ***************************************************************/

VOID
STDCALL
PspThreadSpecialApc(PKAPC Apc,
                    PKNORMAL_ROUTINE* NormalRoutine,
                    PVOID* NormalContext,
                    PVOID* SystemArgument1,
                    PVOID* SystemArgument2)
{
    ExFreePool(Apc);
}

VOID
STDCALL
PspUserThreadStartup(PKSTART_ROUTINE StartRoutine,
                     PVOID StartContext)
{
    PKAPC ThreadApc;
    PETHREAD Thread = PsGetCurrentThread();

    DPRINT("I am a new USER thread. This is my start routine: %p. This my context: %p."
           "This is my IRQL: %d. This is my Thread Pointer: %x.\n", StartRoutine,
            StartContext, KeGetCurrentIrql(), Thread);

    if (!Thread->Terminated) {

        /* Allocate the APC */
        ThreadApc = ExAllocatePoolWithTag(NonPagedPool, sizeof(KAPC), TAG('T', 'h', 'r','d'));

        /* Initialize it */
        KeInitializeApc(ThreadApc,
                        &Thread->Tcb,
                        OriginalApcEnvironment,
                        PspThreadSpecialApc,
                        NULL,
                        PspSystemDllEntryPoint,
                        UserMode,
                        NULL);

        /* Insert it into the queue */
        KeInsertQueueApc(ThreadApc, NULL, NULL, IO_NO_INCREMENT);
        Thread->Tcb.ApcState.UserApcPending = TRUE;
    }

    /* Go to Passive Level and notify debugger */
    KeLowerIrql(PASSIVE_LEVEL);
    DbgkCreateThread(StartContext);
}

VOID
STDCALL
PspSystemThreadStartup(PKSTART_ROUTINE StartRoutine,
                       PVOID StartContext)
{
    PETHREAD Thread = PsGetCurrentThread();

    /* Unlock the dispatcher Database */
    KeLowerIrql(PASSIVE_LEVEL);

    /* Make sure it's not terminated by now */
    if (!Thread->Terminated) {

        /* Call it */
        (StartRoutine)(StartContext);
    }

    /* Exit the thread */
    PspExitThread(STATUS_SUCCESS);
}

NTSTATUS
STDCALL
PspCreateThread(OUT PHANDLE ThreadHandle,
                IN ACCESS_MASK DesiredAccess,
                IN POBJECT_ATTRIBUTES ObjectAttributes  OPTIONAL,
                IN HANDLE ProcessHandle,
                IN PEPROCESS TargetProcess,
                OUT PCLIENT_ID ClientId,
                IN PCONTEXT ThreadContext,
                IN PINITIAL_TEB InitialTeb,
                IN BOOLEAN CreateSuspended,
                IN PKSTART_ROUTINE StartRoutine OPTIONAL,
                IN PVOID StartContext OPTIONAL)
{
    HANDLE hThread;
    PEPROCESS Process;
    PETHREAD Thread;
    PTEB TebBase;
    KIRQL OldIrql;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status;
    HANDLE_TABLE_ENTRY CidEntry;
    PVOID KernelStack;

    /* Reference the Process by handle or pointer, depending on what we got */
    DPRINT("PspCreateThread: %x, %x, %x\n", ProcessHandle, TargetProcess, ThreadContext);
    if (ProcessHandle) {

        /* Normal thread or System Thread */
        DPRINT("Referencing Parent Process\n");
        Status = ObReferenceObjectByHandle(ProcessHandle,
                                           PROCESS_CREATE_THREAD,
                                           PsProcessType,
                                           PreviousMode,
                                           (PVOID*)&Process,
                                           NULL);
    } else {

        /* System thread inside System Process, or Normal Thread with a bug */
        if (StartRoutine) {

            /* Reference the Process by Pointer */
            DPRINT("Referencing Parent System Process\n");
            ObReferenceObject(TargetProcess);
            Process = TargetProcess;
            Status = STATUS_SUCCESS;

        } else {

            /* Fake ObReference returning this */
            Status = STATUS_INVALID_HANDLE;
        }
    }

    /* Check for success */
    if(!NT_SUCCESS(Status)) {

        DPRINT1("Invalid Process Handle, or no handle given\n");
        return(Status);
    }

    /* Create Thread Object */
    DPRINT("Creating Thread Object\n");
    Status = ObCreateObject(PreviousMode,
                            PsThreadType,
                            ObjectAttributes,
                            KernelMode,
                            NULL,
                            sizeof(ETHREAD),
                            0,
                            0,
                            (PVOID*)&Thread);

    /* Check for success */
    if (!NT_SUCCESS(Status)) {

        /* Dereference the Process */
        DPRINT1("Failed to Create Thread Object\n");
        ObDereferenceObject(Process);
        return(Status);
    }

    /* Zero the Object entirely */
    DPRINT("Cleaning Thread Object\n");
    RtlZeroMemory(Thread, sizeof(ETHREAD));

    /* Create Cid Handle */
    DPRINT("Creating Thread Handle (CID)\n");
    CidEntry.u1.Object = Thread;
    CidEntry.u2.GrantedAccess = 0;
    Thread->Cid.UniqueThread = ExCreateHandle(PspCidTable, &CidEntry);
    if (!Thread->Cid.UniqueThread) {

        DPRINT1("Failed to create Thread Handle (CID)\n");
        ObDereferenceObject(Process);
        ObDereferenceObject(Thread);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Initialize Lists */
    DPRINT("Initialliazing Thread Lists and Locks\n");
    InitializeListHead(&Thread->LpcReplyChain);
    InitializeListHead(&Thread->IrpList);
    InitializeListHead(&Thread->ActiveTimerListHead);
    KeInitializeSpinLock(&Thread->ActiveTimerListLock);

    /* Initialize LPC */
    DPRINT("Initialliazing Thread Semaphore\n");
    KeInitializeSemaphore(&Thread->LpcReplySemaphore, 0, MAXLONG);

    /* Allocate Stack for non-GUI Thread */
    DPRINT("Initialliazing Thread Stack\n");
    KernelStack = MmCreateKernelStack(FALSE);

    /* Set the Process CID */
    DPRINT("Initialliazing Thread PID and Parent Process\n");
    Thread->Cid.UniqueProcess = Process->UniqueProcessId;
    Thread->ThreadsProcess = Process;

    /* Now let the kernel initialize the context */
    if (ThreadContext) {

        /* User-mode Thread */

        /* Create Teb */
        DPRINT("Initialliazing Thread PEB\n");
        TebBase = MmCreateTeb(Process, &Thread->Cid, InitialTeb);

        /* Set the Start Addresses */
        DPRINT("Initialliazing Thread Start Addresses :%x, %x\n", ThreadContext->Eip, ThreadContext->Eax);
        Thread->StartAddress = (PVOID)ThreadContext->Eip;
        Thread->Win32StartAddress = (PVOID)ThreadContext->Eax;

        /* Let the kernel intialize the Thread */
        DPRINT("Initialliazing Kernel Thread\n");
        KeInitializeThread(&Process->Pcb,
                           &Thread->Tcb,
                           PspUserThreadStartup,
                           NULL,
                           NULL,
                           ThreadContext,
                           TebBase,
                           KernelStack);

    } else {

        /* System Thread */
        DPRINT("Initialliazing Thread Start Address :%x\n", StartRoutine);
        Thread->StartAddress = StartRoutine;
        Thread->SystemThread = TRUE;

        /* Let the kernel intialize the Thread */
        DPRINT("Initialliazing Kernel Thread\n");
        KeInitializeThread(&Process->Pcb,
                           &Thread->Tcb,
                           PspSystemThreadStartup,
                           StartRoutine,
                           StartContext,
                           NULL,
                           NULL,
                           KernelStack);
    }

    /*
     * Insert the Thread into the Process's Thread List
     * Note, this is the ETHREAD Thread List. It is removed in
     * ps/kill.c!PspExitThread.
     */
    DPRINT("Inserting into Process Thread List \n");
    InsertTailList(&Process->ThreadListHead, &Thread->ThreadListEntry);

    /* Notify Thread Creation */
    DPRINT("Running Thread Notify \n");
    PspRunCreateThreadNotifyRoutines(Thread, TRUE);

    /* FIXME: Use Lock */
    DPRINT("Apcs Queueable: %d \n", Thread->Tcb.ApcQueueable);
    Thread->Tcb.ApcQueueable = TRUE;

    /* Suspend the Thread if we have to */
    if (CreateSuspended) {

        DPRINT("Suspending Thread\n");
        KeSuspendThread(&Thread->Tcb);
    }

    /* Reference ourselves as a keep-alive */
    ObReferenceObject(Thread);

    /* Insert the Thread into the Object Manager */
    DPRINT("Inserting Thread\n");
    Status = ObInsertObject((PVOID)Thread,
                            NULL,
                            DesiredAccess,
                            0,
                            NULL,
                            &hThread);

    /* Return Cid and Handle */
    DPRINT("All worked great!\n");
    if(NT_SUCCESS(Status)) {

        _SEH_TRY {

            if(ClientId != NULL) {

                *ClientId = Thread->Cid;
            }
            *ThreadHandle = hThread;

        } _SEH_HANDLE {

            Status = _SEH_GetExceptionCode();

        } _SEH_END;
    }

    /* FIXME: SECURITY */

    /* Dispatch thread */
    DPRINT("About to dispatch the thread: %x!\n", &Thread->Tcb);
    OldIrql = KeAcquireDispatcherDatabaseLock ();
    KiUnblockThread(&Thread->Tcb, NULL, 0);
    ObDereferenceObject(Thread);
    KeReleaseDispatcherDatabaseLock(OldIrql);

    /* Return */
    DPRINT("Returning\n");
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
STDCALL
PsCreateSystemThread(PHANDLE ThreadHandle,
                     ACCESS_MASK DesiredAccess,
                     POBJECT_ATTRIBUTES ObjectAttributes,
                     HANDLE ProcessHandle,
                     PCLIENT_ID ClientId,
                     PKSTART_ROUTINE StartRoutine,
                     PVOID StartContext)
{
    PEPROCESS TargetProcess = NULL;
    HANDLE Handle = ProcessHandle;

    /* Check if we have a handle. If not, use the System Process */
    if (!ProcessHandle) {

        Handle = NULL;
        TargetProcess = PsInitialSystemProcess;
    }

    /* Call the shared function */
    return PspCreateThread(ThreadHandle,
                           DesiredAccess,
                           ObjectAttributes,
                           Handle,
                           TargetProcess,
                           ClientId,
                           NULL,
                           NULL,
                           FALSE,
                           StartRoutine,
                           StartContext);
}

/*
 * @implemented
 */
NTSTATUS
STDCALL
PsLookupThreadByThreadId(IN HANDLE ThreadId,
                         OUT PETHREAD *Thread)
{
    PHANDLE_TABLE_ENTRY CidEntry;
    PETHREAD FoundThread;
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    PAGED_CODE();
    
    KeEnterCriticalRegion();

    /* Get the CID Handle Entry */
    if ((CidEntry = ExMapHandleToPointer(PspCidTable,
                                         ThreadId)))
    {
        /* Get the Process */
        FoundThread = CidEntry->u1.Object;

        /* Make sure it's really a process */
        if (FoundThread->Tcb.DispatcherHeader.Type == ThreadObject)
        {
            /* Reference and return it */
            ObReferenceObject(FoundThread);
            *Thread = FoundThread;
            Status = STATUS_SUCCESS;
        }

        /* Unlock the Entry */
        ExUnlockHandleTableEntry(PspCidTable, CidEntry);
    }
    
    KeLeaveCriticalRegion();

    /* Return to caller */
    return Status;
}

/*
 * @implemented
 */
HANDLE
STDCALL
PsGetCurrentThreadId(VOID)
{
    return(PsGetCurrentThread()->Cid.UniqueThread);
}

/*
 * @implemented
 */
ULONG
STDCALL
PsGetThreadFreezeCount(PETHREAD Thread)
{
    return Thread->Tcb.FreezeCount;
}

/*
 * @implemented
 */
BOOLEAN
STDCALL
PsGetThreadHardErrorsAreDisabled(PETHREAD Thread)
{
    return Thread->HardErrorsAreDisabled;
}

/*
 * @implemented
 */
HANDLE
STDCALL
PsGetThreadId(PETHREAD Thread)
{
    return Thread->Cid.UniqueThread;
}

/*
 * @implemented
 */
PEPROCESS
STDCALL
PsGetThreadProcess(PETHREAD Thread)
{
    return Thread->ThreadsProcess;
}

/*
 * @implemented
 */
HANDLE
STDCALL
PsGetThreadProcessId(PETHREAD Thread)
{
    return Thread->Cid.UniqueProcess;
}

/*
 * @implemented
 */
HANDLE
STDCALL
PsGetThreadSessionId(PETHREAD Thread)
{
    return (HANDLE)Thread->ThreadsProcess->Session;
}

/*
 * @implemented
 */
PTEB
STDCALL
PsGetThreadTeb(PETHREAD Thread)
{
    return Thread->Tcb.Teb;
}

/*
 * @implemented
 */
PVOID
STDCALL
PsGetThreadWin32Thread(PETHREAD Thread)
{
    return Thread->Tcb.Win32Thread;
}

/*
 * @implemented
 */
KPROCESSOR_MODE
STDCALL
PsGetCurrentThreadPreviousMode(VOID)
{
    return (KPROCESSOR_MODE)PsGetCurrentThread()->Tcb.PreviousMode;
}

/*
 * @implemented
 */
PVOID
STDCALL
PsGetCurrentThreadStackBase(VOID)
{
    return PsGetCurrentThread()->Tcb.StackBase;
}

/*
 * @implemented
 */
PVOID
STDCALL
PsGetCurrentThreadStackLimit(VOID)
{
    return (PVOID)PsGetCurrentThread()->Tcb.StackLimit;
}

/*
 * @implemented
 */
BOOLEAN
STDCALL
PsIsThreadTerminating(IN PETHREAD Thread)
{
    return (Thread->Terminated ? TRUE : FALSE);
}

/*
 * @implemented
 */
BOOLEAN
STDCALL
PsIsSystemThread(PETHREAD Thread)
{
    return (Thread->SystemThread ? TRUE: FALSE);
}

/*
 * @implemented
 */
BOOLEAN
STDCALL
PsIsThreadImpersonating(PETHREAD Thread)
{
    return Thread->ActiveImpersonationInfo;
}

/*
 * @implemented
 */
VOID
STDCALL
PsSetThreadHardErrorsAreDisabled(PETHREAD Thread,
                                 BOOLEAN HardErrorsAreDisabled)
{
    Thread->HardErrorsAreDisabled = HardErrorsAreDisabled;
}

/*
 * @implemented
 */
VOID
STDCALL
PsSetThreadWin32Thread(PETHREAD Thread,
                       PVOID Win32Thread)
{
    Thread->Tcb.Win32Thread = Win32Thread;
}

NTSTATUS
STDCALL
NtCreateThread(OUT PHANDLE ThreadHandle,
               IN ACCESS_MASK DesiredAccess,
               IN POBJECT_ATTRIBUTES ObjectAttributes  OPTIONAL,
               IN HANDLE ProcessHandle,
               OUT PCLIENT_ID ClientId,
               IN PCONTEXT ThreadContext,
               IN PINITIAL_TEB InitialTeb,
               IN BOOLEAN CreateSuspended)
{
    INITIAL_TEB SafeInitialTeb;
    CONTEXT SafeContext;
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    DPRINT("NtCreateThread(ThreadHandle %x, PCONTEXT %x)\n",
            ThreadHandle,ThreadContext);

    if(KeGetPreviousMode() != KernelMode) {

        if (ThreadContext == NULL) {
            DPRINT1("No context for User-Mode Thread!!\n");
            return STATUS_INVALID_PARAMETER;
        }

        _SEH_TRY {

            ProbeForWriteHandle(ThreadHandle);

            if(ClientId != NULL) {

                ProbeForWrite(ClientId,
                              sizeof(CLIENT_ID),
                              sizeof(ULONG));
            }

            if(ThreadContext != NULL) {

                ProbeForRead(ThreadContext,
                             sizeof(CONTEXT),
                             sizeof(ULONG));
                SafeContext = *ThreadContext;
                ThreadContext = &SafeContext;
            }

            ProbeForRead(InitialTeb,
                         sizeof(INITIAL_TEB),
                         sizeof(ULONG));
            SafeInitialTeb = *InitialTeb;
            InitialTeb = &SafeInitialTeb;

        } _SEH_HANDLE {

            Status = _SEH_GetExceptionCode();

        } _SEH_END;

        if (!NT_SUCCESS(Status)) return Status;
    }

    /* Call the shared function */
    return PspCreateThread(ThreadHandle,
                           DesiredAccess,
                           ObjectAttributes,
                           ProcessHandle,
                           NULL,
                           ClientId,
                           ThreadContext,
                           InitialTeb,
                           CreateSuspended,
                           NULL,
                           NULL);
}

/*
 * @implemented
 */
NTSTATUS
STDCALL
NtOpenThread(OUT PHANDLE ThreadHandle,
             IN ACCESS_MASK DesiredAccess,
             IN POBJECT_ATTRIBUTES ObjectAttributes,
             IN PCLIENT_ID ClientId  OPTIONAL)
{
    KPROCESSOR_MODE PreviousMode;
    CLIENT_ID SafeClientId;
    ULONG Attributes = 0;
    HANDLE hThread = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    PETHREAD Thread;
    BOOLEAN HasObjectName = FALSE;

    PAGED_CODE();

    PreviousMode = KeGetPreviousMode();

    /* Probe the paraemeters */
    if(PreviousMode != KernelMode)
    {
        _SEH_TRY
        {
            ProbeForWriteHandle(ThreadHandle);

            if(ClientId != NULL)
            {
                ProbeForRead(ClientId,
                             sizeof(CLIENT_ID),
                             sizeof(ULONG));

                SafeClientId = *ClientId;
                ClientId = &SafeClientId;
            }

            /* just probe the object attributes structure, don't capture it
               completely. This is done later if necessary */
            ProbeForRead(ObjectAttributes,
                         sizeof(OBJECT_ATTRIBUTES),
                         sizeof(ULONG));
            HasObjectName = (ObjectAttributes->ObjectName != NULL);
            Attributes = ObjectAttributes->Attributes;
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        if(!NT_SUCCESS(Status)) return Status;
    }
    else
    {
        HasObjectName = (ObjectAttributes->ObjectName != NULL);
        Attributes = ObjectAttributes->Attributes;
    }
    
    if (HasObjectName && ClientId != NULL)
    {
        /* can't pass both, n object name and a client id */
        return STATUS_INVALID_PARAMETER_MIX;
    }

    /* Open by name if one was given */
    if (HasObjectName)
    {
        /* Open it */
        Status = ObOpenObjectByName(ObjectAttributes,
                                    PsThreadType,
                                    NULL,
                                    PreviousMode,
                                    DesiredAccess,
                                    NULL,
                                    &hThread);

        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Could not open object by name\n");
        }
    }
    else if (ClientId != NULL)
    {
        /* Open by Thread ID */
        if (ClientId->UniqueProcess)
        {
            /* Get the Process */
            DPRINT("Opening by Process ID: %x\n", ClientId->UniqueProcess);
            Status = PsLookupProcessThreadByCid(ClientId,
                                                NULL,
                                                &Thread);
        }
        else
        {
            /* Get the Process */
            DPRINT("Opening by Thread ID: %x\n", ClientId->UniqueThread);
            Status = PsLookupThreadByThreadId(ClientId->UniqueThread,
                                              &Thread);
        }

        if(!NT_SUCCESS(Status))
        {
            DPRINT1("Failure to find Thread\n");
            return Status;
        }

        /* Open the Thread Object */
        Status = ObOpenObjectByPointer(Thread,
                                       Attributes,
                                       NULL,
                                       DesiredAccess,
                                       PsThreadType,
                                       PreviousMode,
                                       &hThread);
        if(!NT_SUCCESS(Status))
        {
            DPRINT1("Failure to open Thread\n");
        }

        /* Dereference the thread */
        ObDereferenceObject(Thread);
    }
    else
    {
        /* neither an object name nor a client id was passed */
        return STATUS_INVALID_PARAMETER_MIX;
    }

    /* Write back the handle */
    if(NT_SUCCESS(Status))
    {
        _SEH_TRY
        {
            *ThreadHandle = hThread;
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
    }

    /* Return status */
    return Status;
}

NTSTATUS
STDCALL
NtYieldExecution(VOID)
{
    KiDispatchThread(Ready);
    return(STATUS_SUCCESS);
}

NTSTATUS
STDCALL
NtTestAlert(VOID)
{
    /* Check and Alert Thread if needed */
    return KeTestAlertThread(ExGetPreviousMode()) ? STATUS_ALERTED : STATUS_SUCCESS;
}

/*
 * @implemented
 */
KPROCESSOR_MODE
STDCALL
ExGetPreviousMode (VOID)
{
    return (KPROCESSOR_MODE)PsGetCurrentThread()->Tcb.PreviousMode;
}

/* EOF */
