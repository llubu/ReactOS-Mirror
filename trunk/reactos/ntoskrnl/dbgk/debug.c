/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/dbgk/dbgkobj.c
 * PURPOSE:         User-Mode Debugging Support, Debug Object Management.
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

POBJECT_TYPE DbgkDebugObjectType;
FAST_MUTEX DbgkpProcessDebugPortMutex;

GENERIC_MAPPING DbgkDebugObjectMapping =
{
    STANDARD_RIGHTS_READ    | DEBUG_OBJECT_WAIT_STATE_CHANGE,
    STANDARD_RIGHTS_WRITE   | DEBUG_OBJECT_ADD_REMOVE_PROCESS,
    STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE,
    DEBUG_OBJECT_ALL_ACCESS
};

static const INFORMATION_CLASS_INFO DbgkpDebugObjectInfoClass[] =
{
    /* DebugObjectUnusedInformation */
    ICI_SQ_SAME(sizeof(ULONG), sizeof(ULONG), 0),
    /* DebugObjectKillProcessOnExitInformation */
    ICI_SQ_SAME(sizeof(DEBUG_OBJECT_KILL_PROCESS_ON_EXIT_INFORMATION), sizeof(ULONG), ICIF_SET),
};

/* PRIVATE FUNCTIONS *********************************************************/

NTSTATUS
NTAPI
DbgkpQueueMessage(IN PEPROCESS Process,
                  IN PETHREAD Thread,
                  IN PDBGKM_MSG Message,
                  IN ULONG Flags,
                  IN PDEBUG_OBJECT TargetObject OPTIONAL)
{
    PDEBUG_EVENT DebugEvent;
    DEBUG_EVENT LocalDebugEvent;
    PDEBUG_OBJECT DebugObject;
    NTSTATUS Status;
    BOOLEAN NewEvent;
    PAGED_CODE();

    /* Check if we have to allocate a debug event */
    NewEvent = (Flags & 2) ? TRUE : FALSE;
    if (NewEvent)
    {
        /* Allocate it */
        DebugEvent = ExAllocatePoolWithTag(NonPagedPool,
                                           sizeof(DEBUG_EVENT),
                                           TAG('D', 'b', 'g', 'E'));
        if (!DebugEvent) return STATUS_INSUFFICIENT_RESOURCES;

        /* Set flags */
        DebugEvent->Flags = Flags | 4;

        /* Reference the thread and process */
        ObReferenceObject(Thread);
        ObReferenceObject(Process);

        /* Set the current thread */
        DebugEvent->BackoutThread = PsGetCurrentThread();

        /* Set the debug object */
        DebugObject = TargetObject;
    }
    else
    {
        /* Use the debug event on the stack */
        DebugEvent = &LocalDebugEvent;
        DebugEvent->Flags = Flags;

        /* Acquire the port lock */
        ExAcquireFastMutex(&DbgkpProcessDebugPortMutex);

        /* Get the debug object */
        DebugObject = Process->DebugPort;

        /* Check what kind of API message this is */
        switch (Message->ApiNumber)
        {
            /* Process or thread creation */
            case DbgKmCreateThreadApi:
            case DbgKmCreateProcessApi:

                /* Make sure we're not skipping creation messages */
                if (Thread->SkipCreationMsg) DebugObject = NULL;
                break;

            /* Process or thread exit */
            case DbgKmExitThreadApi:
            case DbgKmExitProcessApi:

                /* Make sure we're not skipping exit messages */
                if (Thread->SkipTerminationMsg) DebugObject = NULL;

            /* No special handling for other messages */
            default:
                break;
        }
    }

    /* Setup the Debug Event */
    KeInitializeEvent(&DebugEvent->ContinueEvent, SynchronizationEvent, FALSE);
    DebugEvent->Process = Process;
    DebugEvent->Thread = Thread;
    RtlCopyMemory(&DebugEvent->ApiMsg, Message, sizeof(DBGKM_MSG));
    DebugEvent->ClientId = Thread->Cid;

    /* Check if we have a port object */
    if (!DebugObject)
    {
        /* Fail */
        Status = STATUS_PORT_NOT_SET;
    }
    else
    {
        /* Acquire the debug object mutex */
        ExAcquireFastMutex(&DebugObject->Mutex);

        /* Check if a debugger is active */
        if (!DebugObject->DebuggerInactive)
        {
            /* Add the event into the object's list */
            InsertTailList(&DebugObject->EventList, &DebugEvent->EventList);

            /* Check if we have to signal it */
            if (!NewEvent)
            {
                /* Signal it */
                KeSetEvent(&DebugObject->EventsPresent,
                           IO_NO_INCREMENT,
                           FALSE);
            }

            /* Set success */
            Status = STATUS_SUCCESS;
        }
        else
        {
            /* No debugger */
            Status = STATUS_DEBUGGER_INACTIVE;
        }

        /* Release the object lock */
        ExReleaseFastMutex(&DebugObject->Mutex);
    }

    /* Check if we had acquired the port lock */
    if (!NewEvent)
    {
        /* Release it */
        ExReleaseFastMutex(&DbgkpProcessDebugPortMutex);

        /* Check if we got here through success */
        if (NT_SUCCESS(Status))
        {
            /* Wait on the continue event */
            KeWaitForSingleObject(&DebugEvent->ContinueEvent,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);

            /* Copy API Message back */
            RtlCopyMemory(Message, &DebugEvent->ApiMsg, sizeof(DBGKM_MSG));

            /* Set return status */
            Status = DebugEvent->Status;
        }
    }
    else
    {
        /* Check if we failed */
        if (!NT_SUCCESS(Status))
        {
            /* Dereference the process and thread */
            ObDereferenceObject(Thread);
            ObDereferenceObject(Process);

            /* Free the debug event */
            ExFreePool(DebugEvent);
        }
    }

    /* Return status */
    return Status;
}

NTSTATUS
NTAPI
DbgkpSendApiMessageLpc(IN OUT PDBGKM_MSG Message,
                       IN PVOID Port,
                       IN BOOLEAN SuspendProcess)
{
    NTSTATUS Status;
    UCHAR Buffer[PORT_MAXIMUM_MESSAGE_LENGTH];
    PAGED_CODE();

    /* Suspend process if required */
    if (SuspendProcess) DbgkpSuspendProcess();

    /* Set return status */
    Message->ReturnedStatus = STATUS_PENDING;

    /* Set create process reported state */
    PsGetCurrentProcess()->CreateReported = TRUE;

    /* Send the LPC command */
#if 0
    Status = LpcRequestWaitReplyPort(Port,
                                     (PPORT_MESSAGE)Message,
                                     (PPORT_MESSAGE)&Buffer[0]);
#else
    Status = STATUS_UNSUCCESSFUL;
#endif

    /* Flush the instruction cache */
    ZwFlushInstructionCache(NtCurrentProcess(), NULL, 0);

    /* Copy the buffer back */
    if (NT_SUCCESS(Status)) RtlCopyMemory(Message, Buffer, sizeof(DBGKM_MSG));

    /* Resume the process if it was suspended */
    if (SuspendProcess) DbgkpResumeProcess();
    return Status;
}

NTSTATUS
NTAPI
DbgkpSendApiMessage(IN OUT PDBGKM_MSG ApiMsg,
                    IN ULONG Flags)
{
    NTSTATUS Status;
    PAGED_CODE();

    /* Suspend process if required */
    if (Flags) DbgkpSuspendProcess();

    /* Set return status */
    ApiMsg->ReturnedStatus = STATUS_PENDING;

    /* Set create process reported state */
    PsGetCurrentProcess()->CreateReported = TRUE;

    /* Send the LPC command */
    Status = DbgkpQueueMessage(PsGetCurrentProcess(),
                               PsGetCurrentThread(),
                               ApiMsg,
                               0,
                               NULL);

    /* Flush the instruction cache */
    ZwFlushInstructionCache(NtCurrentProcess(), NULL, 0);

    /* Resume the process if it was suspended */
    if (Flags) DbgkpResumeProcess();
    return Status;
}

VOID
NTAPI
DbgkCopyProcessDebugPort(IN PEPROCESS Process,
                         IN PEPROCESS Parent)
{
    PDEBUG_OBJECT DebugObject;
    PAGED_CODE();

    /* Clear this process's port */
    Process->DebugPort = NULL;

    /* Check if the parent has one */
    if (!Parent->DebugPort) return;

    /* It does, acquire the mutex */
    ExAcquireFastMutex(&DbgkpProcessDebugPortMutex);

    /* Make sure it still has one, and that we should inherit */
    DebugObject = Parent->DebugPort;
    if ((DebugObject) && !(Process->NoDebugInherit))
    {
        /* Acquire the debug object's lock */
        ExAcquireFastMutex(&DebugObject->Mutex);

        /* Make sure the debugger is active */
        if (!DebugObject->DebuggerInactive)
        {
            /* Reference the object and set it */
            ObReferenceObject(DebugObject);
            Process->DebugPort = DebugObject;
        }

        /* Release the debug object */
        ExReleaseFastMutex(&DebugObject->Mutex);
    }

    /* Release the port mutex */
    ExReleaseFastMutex(&DbgkpProcessDebugPortMutex);
}

BOOLEAN
NTAPI
DbgkForwardException(IN PEXCEPTION_RECORD ExceptionRecord,
                     IN BOOLEAN DebugPort,
                     IN BOOLEAN SecondChance)
{
    DBGKM_MSG ApiMessage;
    PDBGKM_EXCEPTION DbgKmException = &ApiMessage.Exception;
    NTSTATUS Status;
    PEPROCESS Process = PsGetCurrentProcess();
    PVOID Port;
    BOOLEAN UseLpc = FALSE;
    PAGED_CODE();

    /* Setup the API Message */
    ApiMessage.h.u1.Length = sizeof(DBGKM_MSG) << 16 |
                             (8 + sizeof(DBGKM_EXCEPTION));
    ApiMessage.h.u2.ZeroInit = LPC_DEBUG_EVENT;
    ApiMessage.ApiNumber = DbgKmExceptionApi;

    /* Check if this is to be sent on the debug port */
    if (DebugPort)
    {
        /* Use the debug port, onless the thread is being hidden */
        Port = PsGetCurrentThread()->HideFromDebugger ?
               NULL : Process->DebugPort;
    }
    else
    {
        /* Otherwise, use the exception port */
        Port = Process->ExceptionPort;
        ApiMessage.h.u2.ZeroInit = LPC_EXCEPTION;
        UseLpc = TRUE;
    }

    /* Break out if there's no port */
    if (!Port) return FALSE;

    /* Fill out the exception information */
    DbgKmException->ExceptionRecord = *ExceptionRecord;
    DbgKmException->FirstChance = !SecondChance;

    /* Check if we should use LPC */
    if (UseLpc)
    {
        /* Send the message on the LPC Port */
        Status = DbgkpSendApiMessageLpc(&ApiMessage, Port, DebugPort);
    }
    else
    {
        /* Use native debug object */
        Status = DbgkpSendApiMessage(&ApiMessage, DebugPort);
    }

    /* Check if we failed, and for a debug port, also check the return status */
    if (!(NT_SUCCESS(Status)) ||
        ((DebugPort) &&
         (!(NT_SUCCESS(ApiMessage.ReturnedStatus)) ||
           (ApiMessage.ReturnedStatus == DBG_EXCEPTION_NOT_HANDLED))))
    {
        /* Fail */
        return FALSE;
    }

    /* Otherwise, we're ok */
    return TRUE;
}

VOID
NTAPI
DbgkpFreeDebugEvent(IN PDEBUG_EVENT DebugEvent)
{
    PHANDLE Handle = NULL;
    PAGED_CODE();

    /* Check if this event had a file handle */
    switch (DebugEvent->ApiMsg.ApiNumber)
    {
        /* Create process has a handle */
        case DbgKmCreateProcessApi:

            /* Get the pointer */
            Handle = &DebugEvent->ApiMsg.CreateProcess.FileHandle;

        /* As does DLL load */
        case DbgKmLoadDllApi:

            /* Get the pointer */
            Handle = &DebugEvent->ApiMsg.LoadDll.FileHandle;

        default:
            break;
    }

    /* Close the handle if it exsts */
    if ((Handle) && (*Handle)) ObCloseHandle(*Handle, KernelMode);

    /* Dereference process and thread and free the event */
    ObDereferenceObject(DebugEvent->Process);
    ObDereferenceObject(DebugEvent->Thread);
    ExFreePool(DebugEvent);
}

VOID
NTAPI
DbgkpWakeTarget(IN PDEBUG_EVENT DebugEvent)
{
    PETHREAD Thread = DebugEvent->Thread;
    PAGED_CODE();

    /* Check if we have to wake the thread */
    if (DebugEvent->Flags & 20) PsResumeThread(Thread, NULL);

    /* Check if we had locked the thread */
    if (DebugEvent->Flags & 8)
    {
        /* Unlock it */
        ExReleaseRundownProtection(&Thread->RundownProtect);
    }

    /* Check if we have to wake up the event */
    if (DebugEvent->Flags & 2)
    {
        /* Signal the continue event */
        KeSetEvent(&DebugEvent->ContinueEvent, IO_NO_INCREMENT, FALSE);
    }
    else
    {
        /* Otherwise, free the debug event */
        DbgkpFreeDebugEvent(DebugEvent);
    }
}

NTSTATUS
NTAPI
DbgkpPostFakeModuleMessages(IN PEPROCESS Process,
                            IN PETHREAD Thread,
                            IN PDEBUG_OBJECT DebugObject)
{
    PPEB Peb = Process->Peb;
    PPEB_LDR_DATA LdrData;
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    PLIST_ENTRY ListHead, NextEntry;
    DBGKM_MSG ApiMessage;
    PDBGKM_LOAD_DLL LoadDll = &ApiMessage.LoadDll;
    ULONG i;
    PIMAGE_NT_HEADERS NtHeader;
    UNICODE_STRING ModuleName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;
    PAGED_CODE();

    /* Quit if there's no PEB */
    if (!Peb) return STATUS_SUCCESS;

    /* Get the Loader Data List */
    LdrData = Peb->Ldr;
    ListHead = &LdrData->InLoadOrderModuleList;
    NextEntry = ListHead->Flink;

    /* Loop the modules */
    i = 0;
    while ((NextEntry != ListHead) && (i < 500))
    {
        /* Get the entry */
        LdrEntry = CONTAINING_RECORD(NextEntry,
                                     LDR_DATA_TABLE_ENTRY,
                                     InLoadOrderLinks);

        /* Setup the API Message */
        RtlZeroMemory(&ApiMessage, sizeof(DBGKM_MSG));
        ApiMessage.ApiNumber = DbgKmLoadDllApi;

        /* Set base and clear the name */
        LoadDll->BaseOfDll = LdrEntry->DllBase;
        LoadDll->NamePointer = NULL;

        /* Get the NT Headers */
        NtHeader = RtlImageNtHeader(LoadDll->BaseOfDll);
        if (NtHeader)
        {
            /* Save debug data */
            LoadDll->DebugInfoFileOffset = NtHeader->FileHeader.
                                           PointerToSymbolTable;
            LoadDll->DebugInfoSize = NtHeader->FileHeader.NumberOfSymbols;
        }

        /* Get the name of the DLL */
        Status = MmGetFileNameForAddress(NtHeader, &ModuleName);
        if (NT_SUCCESS(Status))
        {
            /* Setup the object attributes */
            InitializeObjectAttributes(&ObjectAttributes,
                                       &ModuleName,
                                       OBJ_FORCE_ACCESS_CHECK |
                                       OBJ_KERNEL_HANDLE |
                                       OBJ_CASE_INSENSITIVE,
                                       NULL,
                                       NULL);

            /* Open the file to get a handle to it */
            Status = ZwOpenFile(&LoadDll->FileHandle,
                                GENERIC_READ | SYNCHRONIZE,
                                &ObjectAttributes,
                                &IoStatusBlock,
                                FILE_SHARE_READ |
                                FILE_SHARE_WRITE |
                                FILE_SHARE_DELETE,
                                FILE_SYNCHRONOUS_IO_NONALERT);
            if (!NT_SUCCESS(Status)) LoadDll->FileHandle = NULL;

            /* Free the name now */
            ExFreePool(ModuleName.Buffer);
        }

        /* Send the fake module load message */
        Status = DbgkpQueueMessage(Process,
                                   Thread,
                                   &ApiMessage,
                                   2,
                                   DebugObject);
        if (!NT_SUCCESS(Status))
        {
            /* Message send failed, close the file handle if we had one */
            if (LoadDll->FileHandle) ObCloseHandle(LoadDll->FileHandle,
                                                   KernelMode);
        }

        /* Go to the next module */
        NextEntry = NextEntry->Flink;
        i++;
    }

    /* Return success */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
DbgkpPostFakeThreadMessages(IN PEPROCESS Process,
                            IN PDEBUG_OBJECT DebugObject,
                            IN PETHREAD StartThread,
                            OUT PETHREAD *FirstThread,
                            OUT PETHREAD *LastThread)
{
    PETHREAD pFirstThread = NULL, ThisThread, OldThread = NULL, pLastThread;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    BOOLEAN IsFirstThread;
    ULONG Flags;
    DBGKM_MSG ApiMessage;
    PDBGKM_CREATE_THREAD CreateThread = &ApiMessage.CreateThread;
    PDBGKM_CREATE_PROCESS CreateProcess = &ApiMessage.CreateProcess;
    BOOLEAN First;
    PIMAGE_NT_HEADERS NtHeader;
    PAGED_CODE();

    /* Check if we have a start thread */
    if (StartThread)
    {
        /* Then the one we'll find won't be the first one */
        IsFirstThread = FALSE;
        pFirstThread = StartThread;
        ThisThread = StartThread;

        /* Reference it */
        ObReferenceObject(StartThread);
    }
    else
    {
        /* Get the first thread ourselves */
        ThisThread = PsGetNextProcessThread(Process, OldThread);
        IsFirstThread = TRUE;
    }

    /* Start thread loop */
    do
    {
        /* Dereference the previous thread if we had one */
        if (OldThread) ObDereferenceObject(OldThread);

        /* Set this as the last thread and lock it */
        pLastThread = ThisThread;
        ObReferenceObject(ThisThread);
        if (ExAcquireRundownProtection(&ThisThread->RundownProtect))
        {
            /* Acquire worked, set flags */
            Flags = 0x8 | 0x2;

            /* Check if this is a user thread */
            if (!ThisThread->SystemThread)
            {
                /* Suspend it */
                if (NT_SUCCESS(PsSuspendThread(ThisThread, NULL)))
                {
                    /* Remember this */
                    Flags = 0x8 | 0x2 | 0x20;
                }
            }
        }
        else
        {
            /* Couldn't acquire rundown */
            Flags = 0x10 | 0x2;
        }

        /* Clear the API Message */
        RtlZeroMemory(&ApiMessage, sizeof(ApiMessage));

        /* Check if this is the first thread */
        if ((IsFirstThread) &&
            !(Flags & 0x10) &&
            !(ThisThread->SystemThread) &&
            (ThisThread->GrantedAccess))
        {
            /* It is, save the flag */
            First = TRUE;
        }
        else
        {
            /* It isn't, save the flag */
            First = FALSE;
        }

        /* Check if this is the first */
        if (First)
        {
            /* So we'll start with the create process message */
            ApiMessage.ApiNumber = DbgKmCreateProcessApi;

            /* Get the file handle */
            if (Process->SectionObject)
            {
                /* Use the section object */
                CreateProcess->FileHandle =
                    DbgkpSectionToFileHandle(Process->SectionObject);
            }
            else
            {
                /* Don't return any handle */
                CreateProcess->FileHandle = NULL;
            }

            /* Set the base address */
            CreateProcess->BaseOfImage = Process->SectionBaseAddress;

            /* Get the NT Header */
            NtHeader = RtlImageNtHeader(Process->SectionBaseAddress);
            if (NtHeader)
            {
                /* Fill out data from the header */
                CreateProcess->DebugInfoFileOffset = NtHeader->FileHeader.
                                                     PointerToSymbolTable;
                CreateProcess->DebugInfoSize = NtHeader->FileHeader.
                                               NumberOfSymbols;
            }
        }
        else
        {
            /* Otherwise it's a thread message */
            ApiMessage.ApiNumber = DbgKmCreateThreadApi;
            CreateThread->StartAddress = ThisThread->StartAddress;
        }

        /* Queue the message */
        Status = DbgkpQueueMessage(Process,
                                   ThisThread,
                                   &ApiMessage,
                                   Flags,
                                   DebugObject);
        if (!NT_SUCCESS(Status))
        {
            /* We failed. FIXME: Handle this */
            DPRINT1("Unhandled Dbgk codepath!\n");
            KEBUGCHECK(0);
        }

        /* Check if this was the first message */
        if (First)
        {
            /* It isn't the first thread anymore */
            IsFirstThread = FALSE;

            /* Reference this thread and set it as first */
            ObDereferenceObject(ThisThread);
            pFirstThread = ThisThread;
        }

        /* Get the next thread */
        ThisThread = PsGetNextProcessThread(Process, ThisThread);
        OldThread = pLastThread;
    } while(ThisThread);

    /* Check the API status */
    if (!NT_SUCCESS(Status))
    {
        /* We failed. FIXME: Handle this */
        DPRINT1("Unhandled Dbgk codepath!\n");
        KEBUGCHECK(0);
    }

    /* Make sure we have a first thread */
    if (!pFirstThread) return STATUS_UNSUCCESSFUL;

    /* Return thread pointers */
    *FirstThread = pFirstThread;
    *LastThread = pLastThread;
    return Status;
}

NTSTATUS
NTAPI
DbgkpPostFakeProcessCreateMessages(IN PEPROCESS Process,
                                   IN PDEBUG_OBJECT DebugObject,
                                   OUT PETHREAD *LastThread)
{
    KAPC_STATE ApcState;
    PETHREAD FirstThread, FinalThread;
    PETHREAD ReturnThread = NULL;
    NTSTATUS Status;
    PAGED_CODE();

    /* Attach to the process */
    KeStackAttachProcess(&Process->Pcb, &ApcState);

    /* Post the fake thread messages */
    Status = DbgkpPostFakeThreadMessages(Process,
                                         DebugObject,
                                         NULL,
                                         &FirstThread,
                                         &FinalThread);
    if (NT_SUCCESS(Status))
    {
        /* Send the fake module messages too */
        Status = DbgkpPostFakeModuleMessages(Process,
                                             FirstThread,
                                             DebugObject);
        if (!NT_SUCCESS(Status))
        {
            /* We failed, dereference the final thread */
            ObDereferenceObject(FinalThread);
        }
        else
        {
            /* Set the final thread */
            ReturnThread = FinalThread;
        }

        /* Dereference the first thread */
        ObDereferenceObject(FirstThread);
    }

    /* Detach from the process */
    KeUnstackDetachProcess(&ApcState);

    /* Return the last thread */
    *LastThread = ReturnThread;
    return Status;
}

VOID
NTAPI
DbgkpConvertKernelToUserStateChange(IN PDBGUI_WAIT_STATE_CHANGE WaitStateChange,
                                    IN PDEBUG_EVENT DebugEvent)
{
    /* Start by copying the client ID */
    WaitStateChange->AppClientId = DebugEvent->ClientId;

    /* Now check which kind of event this was */
    switch (DebugEvent->ApiMsg.ApiNumber)
    {
        /* New process */
        case DbgKmCreateProcessApi:

            /* Set the right native code */
            WaitStateChange->NewState = DbgCreateProcessStateChange;

            /* Copy the information */
            WaitStateChange->StateInfo.CreateProcessInfo.NewProcess =
                DebugEvent->ApiMsg.CreateProcess;

            /* Clear the file handle for us */
            DebugEvent->ApiMsg.CreateProcess.FileHandle = NULL;
            break;

        /* New thread */
        case DbgKmCreateThreadApi:

            /* Set the right native code */
            WaitStateChange->NewState = DbgCreateThreadStateChange;

            /* Copy information */
            WaitStateChange->StateInfo.CreateThread.NewThread.StartAddress =
                DebugEvent->ApiMsg.CreateThread.StartAddress;
            WaitStateChange->StateInfo.CreateThread.NewThread.SubSystemKey =
                DebugEvent->ApiMsg.CreateThread.SubSystemKey;
            break;

        /* Exception (or breakpoint/step) */
        case DbgKmExceptionApi:

            /* Look at the exception code */
            if (DebugEvent->ApiMsg.Exception.ExceptionRecord.ExceptionCode ==
                STATUS_BREAKPOINT)
            {
                /* Update this as a breakpoint exception */
                WaitStateChange->NewState = DbgBreakpointStateChange;
            }
            else if (DebugEvent->ApiMsg.Exception.ExceptionRecord.ExceptionCode ==
                     STATUS_SINGLE_STEP)
            {
                /* Update this as a single step exception */
                WaitStateChange->NewState = DbgSingleStepStateChange;
            }
            else
            {
                /* Otherwise, set default exception */
                WaitStateChange->NewState = DbgExceptionStateChange;
            }

            /* Copy the exception record */
            WaitStateChange->StateInfo.Exception.ExceptionRecord =
                DebugEvent->ApiMsg.Exception.ExceptionRecord;
            break;

        /* Process exited */
        case DbgKmExitProcessApi:

            /* Set the right native code and copy the exit code */
            WaitStateChange->NewState = DbgExitProcessStateChange;
            WaitStateChange->StateInfo.ExitProcess.ExitStatus =
                DebugEvent->ApiMsg.ExitProcess.ExitStatus;
            break;

        /* Thread exited */
        case DbgKmExitThreadApi:

            /* Set the right native code */
            WaitStateChange->NewState = DbgExitThreadStateChange;
            WaitStateChange->StateInfo.ExitThread.ExitStatus =
                DebugEvent->ApiMsg.ExitThread.ExitStatus;
            break;

        /* DLL Load */
        case DbgKmLoadDllApi:

            /* Set the native code */
            WaitStateChange->NewState = DbgLoadDllStateChange;

            /* Copy the data */
            WaitStateChange->StateInfo.LoadDll = DebugEvent->ApiMsg.LoadDll;

            /* Clear the file handle for us */
            DebugEvent->ApiMsg.LoadDll.FileHandle = NULL;
            break;

        /* DLL Unload */
        case DbgKmUnloadDllApi:

            /* Set the native code and copy the address */
            WaitStateChange->NewState = DbgUnloadDllStateChange;
            WaitStateChange->StateInfo.UnloadDll.BaseAddress =
                DebugEvent->ApiMsg.UnloadDll.BaseAddress;
            break;

        default:

            /* Shouldn't happen */
            ASSERT(FALSE);
    }
}

VOID
NTAPI
DbgkpMarkProcessPeb(IN PEPROCESS Process)
{
    KAPC_STATE ApcState;
    PAGED_CODE();

    /* Acquire process rundown */
    if (!ExAcquireRundownProtection(&Process->RundownProtect)) return;

    /* Make sure we have a PEB */
    if (Process->Peb)
    {
        /* Attach to the process */
        KeStackAttachProcess(&Process->Pcb, &ApcState);

        /* Acquire the debug port mutex */
        ExAcquireFastMutex(&DbgkpProcessDebugPortMutex);

        /* Set the IsBeingDebugged member of the PEB */
        Process->Peb->BeingDebugged = (Process->DebugPort) ? TRUE: FALSE;

        /* Release lock */
        ExReleaseFastMutex(&DbgkpProcessDebugPortMutex);

        /* Detach from the process */
        KeUnstackDetachProcess(&ApcState);
    }

    /* Release rundown protection */
    ExReleaseRundownProtection(&Process->RundownProtect);
}

VOID
NTAPI
DbgkpOpenHandles(IN PDBGUI_WAIT_STATE_CHANGE WaitStateChange,
                 IN PEPROCESS Process,
                 IN PETHREAD Thread)
{
    NTSTATUS Status;
    HANDLE Handle;
    PHANDLE DupHandle;
    PAGED_CODE();

    /* Check which state this is */
    switch (WaitStateChange->NewState)
    {
        /* New thread */
        case DbgCreateThreadStateChange:

            /* Get handle to thread */
            Status = ObOpenObjectByPointer(Thread,
                                           0,
                                           NULL,
                                           THREAD_ALL_ACCESS,
                                           PsThreadType,
                                           KernelMode,
                                           &Handle);
            if (NT_SUCCESS(Status))
            {
                /* Save the thread handle */
                WaitStateChange->
                    StateInfo.CreateThread.HandleToThread = Handle;
            }
            return;

        /* New process */
        case DbgCreateProcessStateChange:

            /* Get handle to thread */
            Status = ObOpenObjectByPointer(Thread,
                                           0,
                                           NULL,
                                           THREAD_ALL_ACCESS,
                                           PsThreadType,
                                           KernelMode,
                                           &Handle);
            if (NT_SUCCESS(Status))
            {
                /* Save the thread handle */
                WaitStateChange->
                    StateInfo.CreateProcessInfo.HandleToThread = Handle;
            }

            /* Get handle to process */
            Status = ObOpenObjectByPointer(Process,
                                           0,
                                           NULL,
                                           PROCESS_ALL_ACCESS,
                                           PsProcessType,
                                           KernelMode,
                                           &Handle);
            if (NT_SUCCESS(Status))
            {
                /* Save the process handle */
                WaitStateChange->
                    StateInfo.CreateProcessInfo.HandleToProcess = Handle;
            }

            /* Fall through to duplicate file handle */
            DupHandle = &WaitStateChange->
                            StateInfo.CreateProcessInfo.NewProcess.FileHandle;
            break;

        /* DLL Load */
        case DbgLoadDllStateChange:

            /* Fall through to duplicate file handle */
            DupHandle = &WaitStateChange->StateInfo.LoadDll.FileHandle;

        /* Anything else has no handles */
        default:
            return;
    }

    /* If we got here, then we have to duplicate a handle, possibly */
    Handle = *DupHandle;
    if (Handle)
    {
        /* Duplicate it */
        Status = ObDuplicateObject(PsGetCurrentProcess(),
                                   Handle,
                                   PsGetCurrentProcess(),
                                   DupHandle,
                                   0,
                                   0,
                                   DUPLICATE_SAME_ACCESS,
                                   KernelMode);
        if (NT_SUCCESS(Status)) *DupHandle = NULL;

        /* Close the original handle */
        ObCloseHandle(Handle, KernelMode);
    }
}

VOID
NTAPI
DbgkpDeleteObject(IN PVOID DebugObject)
{
    PAGED_CODE();

    /* Sanity check */
    ASSERT(IsListEmpty(&((PDEBUG_OBJECT)DebugObject)->EventList));
}

VOID
NTAPI
DbgkpCloseObject(IN PEPROCESS OwnerProcess OPTIONAL,
                 IN PVOID ObjectBody,
                 IN ACCESS_MASK GrantedAccess,
                 IN ULONG HandleCount,
                 IN ULONG SystemHandleCount)
{
    PDEBUG_OBJECT DebugObject = ObjectBody;
    PEPROCESS Process = NULL;
    BOOLEAN DebugPortCleared = FALSE;
    PLIST_ENTRY DebugEventList;
    PDEBUG_EVENT DebugEvent;

    /* If this isn't the last handle, do nothing */
    if (HandleCount > 1) return;

    /* Otherwise, lock the debug object */
    ExAcquireFastMutex(&DebugObject->Mutex);

    /* Set it as inactive */
    DebugObject->DebuggerInactive = TRUE;

    /* Remove it from the debug event list */
    DebugEventList = DebugObject->EventList.Flink;
    InitializeListHead(&DebugObject->EventList);

    /* Release the lock */
    ExReleaseFastMutex(&DebugObject->Mutex);

    /* Signal the wait event */
    KeSetEvent(&DebugObject->EventsPresent, IO_NO_INCREMENT, FALSE);

    /* Start looping each process */
    while ((Process = PsGetNextProcess(Process)))
    {
        /* Check if the process has us as their debug port */
        if (Process->DebugPort == DebugObject)
        {
            /* Acquire the process debug port lock */
            ExAcquireFastMutex(&DbgkpProcessDebugPortMutex);

            /* Check if it's still us */
            if (Process->DebugPort == DebugObject)
            {
                /* Clear it and remember */
                Process->DebugPort = NULL;
                DebugPortCleared = TRUE;
            }

            /* Release the port lock */
            ExReleaseFastMutex(&DbgkpProcessDebugPortMutex);

            /* Check if we cleared the debug port */
            if (DebugPortCleared)
            {
                /* Mark this in the PEB */
                DbgkpMarkProcessPeb(OwnerProcess);

                /* Check if we terminate on exit */
                if (DebugObject->KillProcessOnExit)
                {
                    /* Terminate the process */
                    PsTerminateProcess(OwnerProcess, STATUS_DEBUGGER_INACTIVE);
                }

                /* Dereference the debug object */
                ObDereferenceObject(DebugObject);
            }
        }
    }

    /* Loop debug events */
    while (DebugEventList != &DebugObject->EventList)
    {
        /* Get the debug event */
        DebugEvent = CONTAINING_RECORD(DebugEventList, DEBUG_EVENT, EventList);

        /* Go to the next entry */
        DebugEventList = DebugEventList->Flink;

        /* Wake it up */
        DebugEvent->Status = STATUS_DEBUGGER_INACTIVE;
        DbgkpWakeTarget(DebugEvent);
    }
}

NTSTATUS
NTAPI
DbgkpSetProcessDebugObject(IN PEPROCESS Process,
                           IN PDEBUG_OBJECT DebugObject,
                           IN NTSTATUS MsgStatus,
                           IN PETHREAD LastThread)
{
    NTSTATUS Status;
    LIST_ENTRY TempList;
    BOOLEAN GlobalHeld = FALSE;
    PETHREAD ThisThread, FirstThread;
    PLIST_ENTRY NextEntry;
    PAGED_CODE();

    /* Initialize the temporary list */
    InitializeListHead(&TempList);

    /* Check if we have a success message */
    if (NT_SUCCESS(MsgStatus))
    {
        /* Then default to STATUS_SUCCESS */
        Status = STATUS_SUCCESS;
    }
    else
    {
        /* No last thread, and set the failure code */
        LastThread = NULL;
        Status = MsgStatus;
    }

    /* Now check what status we have here */
    if (NT_SUCCESS(Status))
    {
        /* Acquire the global lock */
        GlobalHeld = TRUE;
        ExAcquireFastMutex(&DbgkpProcessDebugPortMutex);

        /* Check if we already have a port */
        if (Process->DebugPort)
        {
            /* Set failure */
            Status = STATUS_PORT_ALREADY_SET;
        }
        else
        {
ThreadScan:
            /* Otherwise, set the port and reference the thread */
            Process->DebugPort = DebugObject;
            ObReferenceObject(LastThread);

            /* Get the next thread */
            ThisThread  = PsGetNextProcessThread(Process, LastThread);
            if (ThisThread)
            {
                /* Clear the debug port and release the lock */
                Process->DebugPort = NULL;
                ExReleaseFastMutex(&DbgkpProcessDebugPortMutex);
                GlobalHeld = FALSE;

                /* Dereference the thread */
                ObDereferenceObject(LastThread);

                /* Post fake messages */
                Status = DbgkpPostFakeThreadMessages(Process,
                                                     DebugObject,
                                                     ThisThread,
                                                     &FirstThread,
                                                     &LastThread);
                if (!NT_SUCCESS(Status))
                {
                    /* Clear the last thread */
                    LastThread = NULL;
                }
                else
                {
                    /* Dereference the first thread and re-acquire the lock */
                    ObDereferenceObject(FirstThread);
                    GlobalHeld = TRUE;
                    ExAcquireFastMutex(&DbgkpProcessDebugPortMutex);

                    /* Check if we should loop again */
                    if (!Process->DebugPort) goto ThreadScan;

                    /* Otherwise, we already have a port */
                    Status = STATUS_PORT_ALREADY_SET;
                }
            }
        }
    }

    /* Acquire the debug object's lock */
    ExAcquireFastMutex(&DebugObject->Mutex);

    /* Check our status here */
    if (NT_SUCCESS(Status))
    {
        /* Check if we're disconnected */
        if (DebugObject->DebuggerInactive)
        {
            /* Set status */
            Process->DebugPort = NULL;
            Status = STATUS_DEBUGGER_INACTIVE;
        }
        else
        {
            /* Set the process flags */
            InterlockedOr((PLONG)&Process->Flags,
                          PSF_NO_DEBUG_INHERIT_BIT |
                          PSF_CREATE_REPORTED_BIT);

            /* Reference the debug object */
            ObDereferenceObject(DebugObject);
        }
    }

    /* Loop the events list */
    NextEntry = DebugObject->EventList.Flink;
    while (NextEntry != &DebugObject->EventList)
    {
        /* FIXME: TODO */
        KEBUGCHECK(0);
    }

    /* Release the debug object */
    ExReleaseFastMutex(&DebugObject->Mutex);

    /* Release the global lock if acquired */
    if (GlobalHeld) ExReleaseFastMutex(&DbgkpProcessDebugPortMutex);

    /* Check if there's a thread to dereference */
    if (LastThread) ObDereferenceObject(LastThread);

    /* Loop our temporary list */
    NextEntry = TempList.Flink;
    while (NextEntry != &TempList)
    {
        /* FIXME: TODO */
        KEBUGCHECK(0);
    }

    /* Check if we got here through success and mark the PEB, then return */
    if (NT_SUCCESS(Status)) DbgkpMarkProcessPeb(Process);
    return Status;
}

NTSTATUS
NTAPI
DbgkClearProcessDebugObject(IN PEPROCESS Process,
                            IN PDEBUG_OBJECT SourceDebugObject)
{
    /* FIXME: TODO */
    return STATUS_UNSUCCESSFUL;
}

VOID
INIT_FUNCTION
NTAPI
DbgkInitialize(VOID)
{
    OBJECT_TYPE_INITIALIZER ObjectTypeInitializer;
    UNICODE_STRING Name;
    PAGED_CODE();

    /* Initialize the process debug port mutex */
    ExInitializeFastMutex(&DbgkpProcessDebugPortMutex);

    /* Create the Debug Object Type */
    RtlZeroMemory(&ObjectTypeInitializer, sizeof(ObjectTypeInitializer));
    RtlInitUnicodeString(&Name, L"DebugObject");
    ObjectTypeInitializer.Length = sizeof(ObjectTypeInitializer);
    ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof(DEBUG_OBJECT);
    ObjectTypeInitializer.GenericMapping = DbgkDebugObjectMapping;
    ObjectTypeInitializer.PoolType = NonPagedPool;
    ObjectTypeInitializer.ValidAccessMask = DEBUG_OBJECT_WAIT_STATE_CHANGE;
    ObjectTypeInitializer.UseDefaultObject = TRUE;
    ObjectTypeInitializer.CloseProcedure = DbgkpCloseObject;
    ObjectTypeInitializer.DeleteProcedure = DbgkpDeleteObject;
    ObCreateObjectType(&Name,
                       &ObjectTypeInitializer,
                       NULL,
                       &DbgkDebugObjectType);
}

/* PUBLIC FUNCTIONS **********************************************************/

NTSTATUS
NTAPI
NtCreateDebugObject(OUT PHANDLE DebugHandle,
                    IN ACCESS_MASK DesiredAccess,
                    IN POBJECT_ATTRIBUTES ObjectAttributes,
                    IN BOOLEAN KillProcessOnExit)
{
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    PDEBUG_OBJECT DebugObject;
    HANDLE hDebug;
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();

    /* Check if we were called from user mode*/
    if (PreviousMode != KernelMode)
    {
        /* Enter SEH for probing */
        _SEH_TRY
        {
            /* Probe the handle */
            ProbeForWriteHandle(DebugHandle);
        }
        _SEH_HANDLE
        {
            /* Get exception error */
            Status = _SEH_GetExceptionCode();
        } _SEH_END;
        if (!NT_SUCCESS(Status)) return Status;
    }

    /* Create the Object */
    Status = ObCreateObject(PreviousMode,
                            DbgkDebugObjectType,
                            ObjectAttributes,
                            PreviousMode,
                            NULL,
                            sizeof(PDEBUG_OBJECT),
                            0,
                            0,
                            (PVOID*)&DebugObject);
    if (NT_SUCCESS(Status))
    {
        /* Initialize the Debug Object's Fast Mutex */
        ExInitializeFastMutex(&DebugObject->Mutex);

        /* Initialize the State Event List */
        InitializeListHead(&DebugObject->EventList);

        /* Initialize the Debug Object's Wait Event */
        KeInitializeEvent(&DebugObject->EventsPresent, NotificationEvent, 0);

        /* Set the Flags */
        DebugObject->KillProcessOnExit = KillProcessOnExit;

        /* Insert it */
        Status = ObInsertObject((PVOID)DebugObject,
                                 NULL,
                                 DesiredAccess,
                                 0,
                                 NULL,
                                 &hDebug);
        ObDereferenceObject(DebugObject);

        /* Check for success and return handle */
        if (NT_SUCCESS(Status))
        {
            _SEH_TRY
            {
                *DebugHandle = hDebug;
            }
            _SEH_HANDLE
            {
                Status = _SEH_GetExceptionCode();
            } _SEH_END;
        }
    }

    /* Return Status */
    return Status;
}

NTSTATUS
NTAPI
NtDebugContinue(IN HANDLE DebugHandle,
                IN PCLIENT_ID AppClientId,
                IN NTSTATUS ContinueStatus)
{
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    PDEBUG_OBJECT DebugObject;
    NTSTATUS Status = STATUS_SUCCESS;
    PDEBUG_EVENT DebugEvent = NULL, DebugEventToWake = NULL;
    PLIST_ENTRY ListHead, NextEntry;
    BOOLEAN NeedsWake = FALSE;
    CLIENT_ID ClientId;
    PAGED_CODE();

    /* Check if we were called from user mode*/
    if (PreviousMode != KernelMode)
    {
        /* Enter SEH for probing */
        _SEH_TRY
        {
            /* Probe the handle */
            ProbeForRead(AppClientId, sizeof(CLIENT_ID), sizeof(ULONG));
            ClientId = *AppClientId;
            AppClientId = &ClientId;
        }
        _SEH_HANDLE
        {
            /* Get exception error */
            Status = _SEH_GetExceptionCode();
        } _SEH_END;
        if (!NT_SUCCESS(Status)) return Status;
    }

    /* Make sure that the status is valid */
    if ((ContinueStatus != DBG_EXCEPTION_NOT_HANDLED) &&
        (ContinueStatus != DBG_REPLY_LATER) &&
        (ContinueStatus != DBG_UNABLE_TO_PROVIDE_HANDLE) &&
        (ContinueStatus != DBG_TERMINATE_THREAD) &&
        (ContinueStatus != DBG_TERMINATE_PROCESS))
    {
        /* Invalid status */
        Status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        /* Get the debug object */
        Status = ObReferenceObjectByHandle(DebugHandle,
                                           DEBUG_OBJECT_WAIT_STATE_CHANGE,
                                           DbgkDebugObjectType,
                                           PreviousMode,
                                           (PVOID*)&DebugObject,
                                           NULL);
        if (NT_SUCCESS(Status))
        {
            /* Acquire the mutex */
            ExAcquireFastMutex(&DebugObject->Mutex);

            /* Loop the state list */
            ListHead = &DebugObject->EventList;
            NextEntry = ListHead->Flink;
            while (ListHead != NextEntry)
            {
                /* Get the current debug event */
                DebugEvent = CONTAINING_RECORD(NextEntry,
                                               DEBUG_EVENT,
                                               EventList);

                /* Compare process ID */
                if (DebugEvent->ClientId.UniqueProcess ==
                    AppClientId->UniqueProcess)
                {
                    /* Check if we already found a match */
                    if (NeedsWake)
                    {
                        /* Wake it up and break out */
                        DebugEvent->Flags &= ~4;
                        KeSetEvent(&DebugEvent->ContinueEvent,
                                   IO_NO_INCREMENT,
                                   FALSE);
                        break;
                    }

                    /* Compare thread ID and flag */
                    if ((DebugEvent->ClientId.UniqueThread ==
                        AppClientId->UniqueThread) && (DebugEvent->Flags & 1))
                    {
                        /* Remove the event from the list */
                        RemoveEntryList(NextEntry);

                        /* Remember who to wake */
                        NeedsWake = TRUE;
                        DebugEventToWake = DebugEvent;
                    }
                }

                /* Go to the next entry */
                NextEntry = NextEntry->Flink;
            }

            /* Release the mutex */
            ExReleaseFastMutex(&DebugObject->Mutex);

            /* Dereference the object */
            ObDereferenceObject(DebugObject);

            /* Check if need a wait */
            if (NeedsWake)
            {
                /* Set the continue status */
                DebugEvent->ApiMsg.ReturnedStatus = Status;
                DebugEvent->Status = STATUS_SUCCESS;

                /* Wake the target */
                DbgkpWakeTarget(DebugEvent);
            }
            else
            {
                /* Fail */
                Status = STATUS_INVALID_PARAMETER;
            }
        }
    }

    /* Return status */
    return Status;
}

NTSTATUS
NTAPI
NtDebugActiveProcess(IN HANDLE ProcessHandle,
                     IN HANDLE DebugHandle)
{
    PEPROCESS Process;
    PDEBUG_OBJECT DebugObject;
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    PETHREAD LastThread;
    NTSTATUS Status;

    /* Reference the process */
    Status = ObReferenceObjectByHandle(ProcessHandle,
                                       PROCESS_SUSPEND_RESUME,
                                       PsProcessType,
                                       PreviousMode,
                                       (PVOID*)&Process,
                                       NULL);
    if (!NT_SUCCESS(Status)) return Status;

    /* Don't allow debugging the initial system process */
    if (Process == PsInitialSystemProcess) return STATUS_ACCESS_DENIED;

    /* Reference the debug object */
    Status = ObReferenceObjectByHandle(DebugHandle,
                                       DEBUG_OBJECT_ADD_REMOVE_PROCESS,
                                       DbgkDebugObjectType,
                                       PreviousMode,
                                       (PVOID*)&DebugObject,
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        /* Dereference the process and exit */
        ObDereferenceObject(Process);
        return Status;
    }

    /* Acquire process rundown protection */
    if (!ExAcquireRundownProtection(&Process->RundownProtect))
    {
        /* Dereference the process and debug object and exit */
        ObDereferenceObject(Process);
        ObDereferenceObject(DebugObject);
        return STATUS_PROCESS_IS_TERMINATING;
    }

    /* Send fake create messages for debuggers to have a consistent state */
    Status = DbgkpPostFakeProcessCreateMessages(Process,
                                                DebugObject,
                                                &LastThread);
    Status = DbgkpSetProcessDebugObject(Process,
                                        DebugObject,
                                        Status,
                                        LastThread);

    /* Release rundown protection */
    ExReleaseRundownProtection(&Process->RundownProtect);

    /* Dereference the process and debug object and return status */
    ObDereferenceObject(Process);
    ObDereferenceObject(DebugObject);
    return Status;
}

NTSTATUS
NTAPI
NtRemoveProcessDebug(IN HANDLE ProcessHandle,
                     IN HANDLE DebugHandle)
{
    PEPROCESS Process;
    PDEBUG_OBJECT DebugObject;
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    NTSTATUS Status;

    /* Reference the process */
    Status = ObReferenceObjectByHandle(ProcessHandle,
                                       PROCESS_SUSPEND_RESUME,
                                       PsProcessType,
                                       PreviousMode,
                                       (PVOID*)&Process,
                                       NULL);
    if (!NT_SUCCESS(Status)) return Status;

    /* Reference the debug object */
    Status = ObReferenceObjectByHandle(DebugHandle,
                                       DEBUG_OBJECT_ADD_REMOVE_PROCESS,
                                       DbgkDebugObjectType,
                                       PreviousMode,
                                       (PVOID*)&DebugObject,
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        /* Dereference the process and exit */
        ObDereferenceObject(Process);
        return Status;
    }

    /* Remove the debug object */
    Status = DbgkClearProcessDebugObject(Process, DebugObject);

    /* Dereference the process and debug object and return status */
    ObDereferenceObject(Process);
    ObDereferenceObject(DebugObject);
    return Status;
}

NTSTATUS
NTAPI
NtSetInformationDebugObject(IN HANDLE DebugHandle,
                            IN DEBUGOBJECTINFOCLASS DebugObjectInformationClass,
                            IN PVOID DebugInformation,
                            IN ULONG DebugInformationLength,
                            OUT PULONG ReturnLength OPTIONAL)
{
    PDEBUG_OBJECT DebugObject;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status = STATUS_SUCCESS;
    PDEBUG_OBJECT_KILL_PROCESS_ON_EXIT_INFORMATION DebugInfo = DebugInformation;
    PAGED_CODE();

    /* Check buffers and parameters */
    Status = DefaultSetInfoBufferCheck(DebugObjectInformationClass,
                                       DbgkpDebugObjectInfoClass,
                                       sizeof(DbgkpDebugObjectInfoClass) /
                                       sizeof(DbgkpDebugObjectInfoClass[0]),
                                       DebugInformation,
                                       DebugInformationLength,
                                       PreviousMode);

    /* Check if the caller wanted the return length */
    if (ReturnLength)
    {
        /* Enter SEH for probe */
        _SEH_TRY
        {
            /* Return required length to user-mode */
            ProbeForWriteUlong(ReturnLength);
            *ReturnLength = sizeof(*DebugInfo);
        }
        _SEH_EXCEPT(_SEH_ExSystemExceptionFilter)
        {
            /* Get SEH Exception code */
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
    }
    if (!NT_SUCCESS(Status)) return Status;

    /* Open the Object */
    Status = ObReferenceObjectByHandle(DebugHandle,
                                       DEBUG_OBJECT_WAIT_STATE_CHANGE,
                                       DbgkDebugObjectType,
                                       PreviousMode,
                                       (PVOID*)&DebugObject,
                                       NULL);
    if (NT_SUCCESS(Status))
    {
        /* Acquire the object */
        ExAcquireFastMutex(&DebugObject->Mutex);

        /* Set the proper flag */
        if (DebugInfo->KillProcessOnExit)
        {
            /* Enable killing the process */
            DebugObject->KillProcessOnExit = TRUE;
        }
        else
        {
            /* Disable */
            DebugObject->KillProcessOnExit = FALSE;
        }

        /* Release the mutex */
        ExReleaseFastMutex(&DebugObject->Mutex);

        /* Release the Object */
        ObDereferenceObject(DebugObject);
    }

    /* Return Status */
    return Status;
}

NTSTATUS
NTAPI
NtWaitForDebugEvent(IN HANDLE DebugHandle,
                    IN BOOLEAN Alertable,
                    IN PLARGE_INTEGER Timeout OPTIONAL,
                    OUT PDBGUI_WAIT_STATE_CHANGE StateChange)
{
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    LARGE_INTEGER SafeTimeOut;
    PEPROCESS Process;
    LARGE_INTEGER StartTime;
    PETHREAD Thread;
    BOOLEAN GotEvent;
    LARGE_INTEGER NewTime;
    PDEBUG_OBJECT DebugObject;
    DBGUI_WAIT_STATE_CHANGE WaitStateChange;
    NTSTATUS Status = STATUS_SUCCESS;
    PDEBUG_EVENT DebugEvent, DebugEvent2;
    PLIST_ENTRY ListHead, NextEntry;

    /* Clear the initial wait state change structure */
    RtlZeroMemory(&WaitStateChange, sizeof(WaitStateChange));

    /* Check if the call was from user mode */
    if (PreviousMode != KernelMode)
    {
        /* Protect probe in SEH */
        _SEH_TRY
        {
            /* Check if we came with a timeout */
            if (Timeout)
            {
                /* Make a copy on the stack */
                SafeTimeOut = ProbeForReadLargeInteger(Timeout);
                Timeout = &SafeTimeOut;
            }

            /* Probe the state change structure */
            ProbeForWrite(StateChange, sizeof(*StateChange), sizeof(ULONG));
        }
        _SEH_HANDLE
        {
            /* Get the exception code */
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
        if (!NT_SUCCESS(Status)) return Status;

        /* Query the current time */
        KeQuerySystemTime(&StartTime);
    }

    /* Get the debug object */
    Status = ObReferenceObjectByHandle(DebugHandle,
                                       DEBUG_OBJECT_WAIT_STATE_CHANGE,
                                       DbgkDebugObjectType,
                                       PreviousMode,
                                       (PVOID*)&DebugObject,
                                       NULL);
    if (!NT_SUCCESS(Status)) return Status;

    /* Clear process and thread */
    Process = NULL;
    Thread = NULL;

    /* Wait on the debug object given to us */
    Status = KeWaitForSingleObject(DebugObject,
                                   Executive,
                                   PreviousMode,
                                   Alertable,
                                   Timeout);

    /* Start the wait loop */
    while (TRUE)
    {
        if (!NT_SUCCESS(Status) ||
            (Status == STATUS_TIMEOUT) ||
            (Status == STATUS_ALERTED) ||
            (Status == STATUS_USER_APC))
        {
            /* Break out the wait */
            break;
        }

        /* Lock the object */
        GotEvent = FALSE;
        ExAcquireFastMutex(&DebugObject->Mutex);

        /* Check if a debugger is connected */
        if (DebugObject->DebuggerInactive)
        {
            /* Not connected */
            Status = STATUS_DEBUGGER_INACTIVE;
        }
        else
        {
            /* Loop the events */
            ListHead = &DebugObject->EventList;
            NextEntry =  ListHead->Flink;
            while (ListHead != NextEntry)
            {
                /* Get the debug event */
                DebugEvent = CONTAINING_RECORD(NextEntry,
                                               DEBUG_EVENT,
                                               EventList);

                /* Check flags */
                if (!(DebugEvent->Flags & (4 | 1)))
                {
                    /* We got an event */
                    GotEvent = TRUE;

                    /* Loop the list internally */
                    while (&DebugEvent->EventList != NextEntry)
                    {
                        /* Get the debug event */
                        DebugEvent2 = CONTAINING_RECORD(NextEntry,
                                                        DEBUG_EVENT,
                                                        EventList);

                        /* Try to match process IDs */
                        if (DebugEvent2->ClientId.UniqueProcess ==
                            DebugEvent->ClientId.UniqueProcess)
                        {
                            /* Found it, break out */
                            DebugEvent->Flags |= 4;
                            DebugEvent->BackoutThread = NULL;
                            GotEvent = FALSE;
                            break;
                        }

                        /* Move to the next entry */
                        NextEntry = NextEntry->Flink;
                    }

                    /* Check if we still have a valid event */
                    if (GotEvent) break;
                }

                /* Move to the next entry */
                NextEntry = NextEntry->Flink;
            }

            /* Check if we have an event */
            if (GotEvent)
            {
                /* Save and reference the process and thread */
                Process = DebugEvent->Process;
                Thread = DebugEvent->Thread;
                ObReferenceObject(Process);
                ObReferenceObject(Thread);

                /* Convert to user-mode structure */
                DbgkpConvertKernelToUserStateChange(&WaitStateChange,
                                                    DebugEvent);

                /* Set flag */
                DebugEvent->Flags |= 1;
                Status = STATUS_SUCCESS;
            }
            else
            {
                /* Unsignal the event */
                DebugObject->EventsPresent.Header.SignalState = 0;
                Status = STATUS_SUCCESS;
            }
        }

        /* Release the mutex */
        ExReleaseFastMutex(&DebugObject->Mutex);
        if (!NT_SUCCESS(Status)) break;

        /* Check if we got an event */
        if (GotEvent)
        {
            /* Check if we can wait again */
            if (!SafeTimeOut.QuadPart)
            {
                /* Query the new time */
                KeQuerySystemTime(&NewTime);

                /* Substract times */
                /* FIXME: TODO */
            }
        }
        else
        {
            /* Open the handles and dereference the objects */
            DbgkpOpenHandles(&WaitStateChange, Process, Thread);
            ObDereferenceObject(Process);
            ObDereferenceObject(Thread);
        }
    }

    /* We're, dereference the object */
    ObDereferenceObject(DebugObject);

    /* Protect write with SEH */
    _SEH_TRY
    {
        /* Return our wait state change structure */
        RtlCopyMemory(StateChange,
                      &WaitStateChange,
                      sizeof(DBGUI_WAIT_STATE_CHANGE));
    }
    _SEH_EXCEPT(_SEH_ExSystemExceptionFilter)
    {
        /* Get SEH Exception code */
        Status = _SEH_GetExceptionCode();
    }
    _SEH_END;

    /* Return status */
    return Status;
}

/* EOF */
