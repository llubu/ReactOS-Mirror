/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/po/power.c
 * PURPOSE:         Power Manager
 * PROGRAMMERS:     Casper S. Hornstrup (chorns@users.sourceforge.net)
 *                  Herv� Poussineau (hpoussin@reactos.com)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

extern ULONG ExpInitialiationPhase;

typedef struct _REQUEST_POWER_ITEM
{
    PREQUEST_POWER_COMPLETE CompletionRoutine;
    POWER_STATE PowerState;
    PVOID Context;
} REQUEST_POWER_ITEM, *PREQUEST_POWER_ITEM;

PDEVICE_NODE PopSystemPowerDeviceNode = NULL;
BOOLEAN PopAcpiPresent = FALSE;

/* PRIVATE FUNCTIONS *********************************************************/

static
NTSTATUS
NTAPI
PopRequestPowerIrpCompletion(IN PDEVICE_OBJECT DeviceObject,
                             IN PIRP Irp,
                             IN PVOID Context)
{
    PIO_STACK_LOCATION Stack;
    PREQUEST_POWER_ITEM RequestPowerItem;
  
    Stack = IoGetNextIrpStackLocation(Irp);
    RequestPowerItem = (PREQUEST_POWER_ITEM)Context;
  
    RequestPowerItem->CompletionRoutine(DeviceObject,
                                        Stack->MinorFunction,
                                        RequestPowerItem->PowerState,
                                        RequestPowerItem->Context,
                                        &Irp->IoStatus);
  
    ExFreePool(&Irp->IoStatus);
    ExFreePool(Context);

    return STATUS_SUCCESS;
}

VOID
NTAPI
PopCleanupPowerState(IN PPOWER_STATE PowerState)
{
    //UNIMPLEMENTED;
}

NTSTATUS
NTAPI
PopSetSystemPowerState(SYSTEM_POWER_STATE PowerState)
{
    IO_STATUS_BLOCK IoStatusBlock;
    PDEVICE_OBJECT DeviceObject;
    PIO_STACK_LOCATION IrpSp;
    PDEVICE_OBJECT Fdo;
    NTSTATUS Status;
    KEVENT Event;
    PIRP Irp;

    if (!PopAcpiPresent) return STATUS_NOT_IMPLEMENTED;

    Status = IopGetSystemPowerDeviceObject(&DeviceObject);
    if (!NT_SUCCESS(Status)) 
    {
        DPRINT1("No system power driver available\n");
        return STATUS_UNSUCCESSFUL;
    }

    Fdo = IoGetAttachedDeviceReference(DeviceObject);

    if (Fdo == DeviceObject)
    {
        DPRINT("An FDO was not attached\n");
        return STATUS_UNSUCCESSFUL;
    }

    KeInitializeEvent(&Event,
                      NotificationEvent,
                      FALSE);

    Irp = IoBuildSynchronousFsdRequest(IRP_MJ_POWER,
                                       Fdo,
                                       NULL,
                                       0,
                                       NULL,
                                       &Event,
                                       &IoStatusBlock);

    IrpSp = IoGetNextIrpStackLocation(Irp);
    IrpSp->MinorFunction = IRP_MN_SET_POWER;
    IrpSp->Parameters.Power.Type = SystemPowerState;
    IrpSp->Parameters.Power.State.SystemState = PowerState;

    DPRINT("Calling ACPI driver");
    Status = PoCallDriver(Fdo, Irp);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
      Status = IoStatusBlock.Status;
    }

    ObDereferenceObject(Fdo);

    return Status;
}

BOOLEAN
NTAPI
PoInitSystem(IN ULONG BootPhase,
             IN BOOLEAN HaveAcpiTable)
{
    PVOID NotificationEntry;
    PCHAR CommandLine;
    BOOLEAN ForceAcpiDisable = FALSE;

    /* Check if this is phase 1 init */
    if (BootPhase == 1)
    {
        /* Registry power button notification */
        IoRegisterPlugPlayNotification(EventCategoryDeviceInterfaceChange,
                                       0, /* The registry has not been initialized yet */
                                       (PVOID)&GUID_DEVICE_SYS_BUTTON,
                                       IopRootDeviceNode->
                                       PhysicalDeviceObject->DriverObject,
                                       PopAddRemoveSysCapsCallback,
                                       NULL,
                                       &NotificationEntry);
        return TRUE;
    }

    /* Get the Command Line */
    CommandLine = KeLoaderBlock->LoadOptions;

    /* Upcase it */
    _strupr(CommandLine);

    /* Check for ACPI disable */
    if (strstr(CommandLine, "NOACPI")) ForceAcpiDisable = TRUE;

    if (ForceAcpiDisable)
    {
        /* Set the ACPI State to False if it's been forced that way */
        PopAcpiPresent = FALSE;
    }
    else
    {
        /* Otherwise check the LoaderBlock's Flag */
        PopAcpiPresent = HaveAcpiTable;
    }

    return TRUE;
}

VOID
NTAPI
PopPerfIdle(PPROCESSOR_POWER_STATE PowerState)
{
    DPRINT1("PerfIdle function: %p\n", PowerState);
}

VOID
NTAPI
PopPerfIdleDpc(IN PKDPC Dpc,
               IN PVOID DeferredContext,
               IN PVOID SystemArgument1,
               IN PVOID SystemArgument2)
{
    /* Call the Perf Idle function */
    PopPerfIdle(&((PKPRCB)DeferredContext)->PowerState);
}

VOID
FASTCALL
PopIdle0(IN PPROCESSOR_POWER_STATE PowerState)
{
    /* FIXME: Extremly naive implementation */
    HalProcessorIdle();
}

VOID
NTAPI
PoInitializePrcb(IN PKPRCB Prcb)
{
    /* Initialize the Power State */
    RtlZeroMemory(&Prcb->PowerState, sizeof(Prcb->PowerState));
    Prcb->PowerState.Idle0KernelTimeLimit = 0xFFFFFFFF;
    Prcb->PowerState.CurrentThrottle = 100;
    Prcb->PowerState.CurrentThrottleIndex = 0;
    Prcb->PowerState.IdleFunction = PopIdle0;

    /* Initialize the Perf DPC and Timer */
    KeInitializeDpc(&Prcb->PowerState.PerfDpc, PopPerfIdleDpc, Prcb);
    KeSetTargetProcessorDpc(&Prcb->PowerState.PerfDpc, Prcb->Number);
    KeInitializeTimerEx(&Prcb->PowerState.PerfTimer, SynchronizationTimer);
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
PoCancelDeviceNotify(IN PVOID NotifyBlock)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
PoRegisterDeviceNotify(OUT PVOID Unknown0,
                       IN ULONG Unknown1,
                       IN ULONG Unknown2,
                       IN ULONG Unknown3,
                       IN PVOID Unknown4,
                       IN PVOID Unknown5)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
VOID
NTAPI
PoShutdownBugCheck(IN BOOLEAN LogError,
                   IN ULONG BugCheckCode,
                   IN ULONG_PTR BugCheckParameter1,
                   IN ULONG_PTR BugCheckParameter2,
                   IN ULONG_PTR BugCheckParameter3,
                   IN ULONG_PTR BugCheckParameter4)
{
    DPRINT1("PoShutdownBugCheck called\n");

    /* FIXME: Log error if requested */
    /* FIXME: Initiate a shutdown */

    /* Bugcheck the system */
    KeBugCheckEx(BugCheckCode,
                 BugCheckParameter1,
                 BugCheckParameter2,
                 BugCheckParameter3,
                 BugCheckParameter4);
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
PoRequestShutdownEvent(OUT PVOID *Event)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
VOID
NTAPI
PoSetHiberRange(IN PVOID HiberContext,
                IN ULONG Flags,
                IN OUT PVOID StartPage,
                IN ULONG Length,
                IN ULONG PageTag)
{
    UNIMPLEMENTED;
    return;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
PoCallDriver(IN PDEVICE_OBJECT DeviceObject,
             IN OUT PIRP Irp)
{
    NTSTATUS Status;

    /* Forward to Io -- FIXME! */
    Status = IoCallDriver(DeviceObject, Irp);

    /* Return status */
    return Status;
}

/*
 * @unimplemented
 */
PULONG
NTAPI
PoRegisterDeviceForIdleDetection(IN PDEVICE_OBJECT DeviceObject,
                                 IN ULONG ConservationIdleTime,
                                 IN ULONG PerformanceIdleTime,
                                 IN DEVICE_POWER_STATE State)
{
    UNIMPLEMENTED;
    return NULL;
}

/*
 * @unimplemented
 */
PVOID
NTAPI
PoRegisterSystemState(IN PVOID StateHandle,
                      IN EXECUTION_STATE Flags)
{
    UNIMPLEMENTED;
    return NULL;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
PoRequestPowerIrp(IN PDEVICE_OBJECT DeviceObject,
                  IN UCHAR MinorFunction,
                  IN POWER_STATE PowerState,
                  IN PREQUEST_POWER_COMPLETE CompletionFunction,
                  IN PVOID Context,
                  OUT PIRP *pIrp OPTIONAL)
{
    PDEVICE_OBJECT TopDeviceObject;
    PIO_STACK_LOCATION Stack;
    PIRP Irp;
    PIO_STATUS_BLOCK IoStatusBlock;
    PREQUEST_POWER_ITEM RequestPowerItem;
    NTSTATUS Status;
  
    if (MinorFunction != IRP_MN_QUERY_POWER
        && MinorFunction != IRP_MN_SET_POWER
        && MinorFunction != IRP_MN_WAIT_WAKE)
        return STATUS_INVALID_PARAMETER_2;
  
    RequestPowerItem = ExAllocatePool(NonPagedPool, sizeof(REQUEST_POWER_ITEM));
    if (!RequestPowerItem)
        return STATUS_INSUFFICIENT_RESOURCES;
    IoStatusBlock = ExAllocatePool(NonPagedPool, sizeof(IO_STATUS_BLOCK));
    if (!IoStatusBlock)
    {
        ExFreePool(RequestPowerItem);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
  
    /* Always call the top of the device stack */
    TopDeviceObject = IoGetAttachedDeviceReference(DeviceObject);
  
    Irp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP,
                                       TopDeviceObject,
                                       NULL,
                                       0,
                                       NULL,
                                       NULL,
                                       IoStatusBlock);
    if (!Irp)
    {
        ExFreePool(RequestPowerItem);
        ExFreePool(IoStatusBlock);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
  
    /* POWER IRPs are always initialized with a status code of
       STATUS_NOT_IMPLEMENTED */
    Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
    Irp->IoStatus.Information = 0;
  
    Stack = IoGetNextIrpStackLocation(Irp);
    Stack->MinorFunction = MinorFunction;
    if (MinorFunction == IRP_MN_WAIT_WAKE)
        Stack->Parameters.WaitWake.PowerState = PowerState.SystemState;
    else
        Stack->Parameters.WaitWake.PowerState = PowerState.DeviceState;
  
    RequestPowerItem->CompletionRoutine = CompletionFunction;
    RequestPowerItem->PowerState = PowerState;
    RequestPowerItem->Context = Context;
  
    if (pIrp != NULL)
        *pIrp = Irp;
  
    IoSetCompletionRoutine(Irp, PopRequestPowerIrpCompletion, RequestPowerItem, TRUE, TRUE, TRUE);
    Status = IoCallDriver(TopDeviceObject, Irp);
  
    /* Always return STATUS_PENDING. The completion routine
     * will call CompletionFunction and complete the Irp.
     */
    return STATUS_PENDING;
}

/*
 * @unimplemented
 */
POWER_STATE
NTAPI
PoSetPowerState(IN PDEVICE_OBJECT DeviceObject,
                IN POWER_STATE_TYPE Type,
                IN POWER_STATE State)
{
    POWER_STATE ps;

    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    ps.SystemState = PowerSystemWorking;  // Fully on
    ps.DeviceState = PowerDeviceD0;       // Fully on

    return ps;
}

/*
 * @unimplemented
 */
VOID
NTAPI
PoSetSystemState(IN EXECUTION_STATE Flags)
{
    UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
VOID
NTAPI
PoStartNextPowerIrp(IN PIRP Irp)
{
    UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
VOID
NTAPI
PoUnregisterSystemState(IN PVOID StateHandle)
{
    UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
PoQueueShutdownWorkItem(IN PWORK_QUEUE_ITEM WorkItem)
{
    PAGED_CODE();

    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
NtInitiatePowerAction (IN POWER_ACTION SystemAction,
                       IN SYSTEM_POWER_STATE MinSystemState,
                       IN ULONG Flags,
                       IN BOOLEAN Asynchronous)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
NtPowerInformation(IN POWER_INFORMATION_LEVEL PowerInformationLevel,
                   IN PVOID InputBuffer  OPTIONAL,
                   IN ULONG InputBufferLength,
                   OUT PVOID OutputBuffer  OPTIONAL,
                   IN ULONG OutputBufferLength)
{
    NTSTATUS Status;

    PAGED_CODE();

    DPRINT("NtPowerInformation(PowerInformationLevel 0x%x, InputBuffer 0x%x, "
           "InputBufferLength 0x%x, OutputBuffer 0x%x, OutputBufferLength 0x%x)\n",
           PowerInformationLevel,
           InputBuffer, InputBufferLength,
           OutputBuffer, OutputBufferLength);
    
    switch (PowerInformationLevel)
    {
        case SystemBatteryState:
        {
            PSYSTEM_BATTERY_STATE BatteryState = (PSYSTEM_BATTERY_STATE)OutputBuffer;

            if (InputBuffer != NULL)
                return STATUS_INVALID_PARAMETER;
            if (OutputBufferLength < sizeof(SYSTEM_BATTERY_STATE))
                return STATUS_BUFFER_TOO_SMALL;

            /* Just zero the struct (and thus set BatteryState->BatteryPresent = FALSE) */
            RtlZeroMemory(BatteryState, sizeof(SYSTEM_BATTERY_STATE));
            BatteryState->EstimatedTime = MAXULONG;

            Status = STATUS_SUCCESS;
            break;
        }
		case SystemPowerCapabilities:
        {
            PSYSTEM_POWER_CAPABILITIES PowerCapabilities = (PSYSTEM_POWER_CAPABILITIES)OutputBuffer;

            if (InputBuffer != NULL)
                return STATUS_INVALID_PARAMETER;
            if (OutputBufferLength < sizeof(SYSTEM_POWER_CAPABILITIES))
                return STATUS_BUFFER_TOO_SMALL;

            /* Just zero the struct (and thus set BatteryState->BatteryPresent = FALSE) */
            RtlZeroMemory(PowerCapabilities, sizeof(SYSTEM_POWER_CAPABILITIES));
            //PowerCapabilities->SystemBatteriesPresent = 0;

            Status = STATUS_SUCCESS;
            break;
        }

        default:
            Status = STATUS_NOT_IMPLEMENTED;
            DPRINT1("PowerInformationLevel 0x%x is UNIMPLEMENTED! Have a nice day.\n",
                    PowerInformationLevel);
            break;
    }

    return Status;
}

NTSTATUS
NTAPI
NtGetDevicePowerState(IN HANDLE Device,
                      IN PDEVICE_POWER_STATE PowerState)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

BOOLEAN
NTAPI
NtIsSystemResumeAutomatic(VOID)
{
    UNIMPLEMENTED;
    return FALSE;
}

NTSTATUS
NTAPI
NtRequestWakeupLatency(IN LATENCY_TIME Latency)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtSetThreadExecutionState(IN EXECUTION_STATE esFlags,
                          OUT EXECUTION_STATE *PreviousFlags)
{
    PKTHREAD Thread = KeGetCurrentThread();
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    EXECUTION_STATE PreviousState;
    PAGED_CODE();

    /* Validate flags */
    if (esFlags & ~(ES_CONTINUOUS | ES_USER_PRESENT))
    {
        /* Fail the request */
        return STATUS_INVALID_PARAMETER;
    }

    /* Check for user parameters */
    if (PreviousMode != KernelMode)
    {
        /* Protect the probes */
        _SEH2_TRY
        {
            /* Check if the pointer is valid */
            ProbeForWriteUlong(PreviousFlags);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* It isn't -- fail */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }

    /* Save the previous state, always masking in the continous flag */
    PreviousState = Thread->PowerState | ES_CONTINUOUS;

    /* Check if we need to update the power state */
    if (esFlags & ES_CONTINUOUS) Thread->PowerState = esFlags;

    /* Protect the write back to user mode */
    _SEH2_TRY
    {
        /* Return the previous flags */
        *PreviousFlags = PreviousState;
    }
    _SEH2_EXCEPT(ExSystemExceptionFilter())
    {
        /* Something's wrong, fail */
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    /* All is good */
    return STATUS_SUCCESS;
}
