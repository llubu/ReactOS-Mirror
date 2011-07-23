/*
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/kernel32/misc/power.c
 * PURPOSE:         Power Management Functions
 * PROGRAMMER:      Dmitry Chapyshev <dmitry@reactos.org>
 *
 * UPDATE HISTORY:
 *                  01/15/2009 Created
 */

#include <k32.h>

#define NDEBUG
#include <debug.h>


NTSYSAPI
NTSTATUS
NTAPI
NtGetDevicePowerState(
    IN HANDLE Device,
    IN PDEVICE_POWER_STATE PowerState
);

NTSYSAPI
NTSTATUS
NTAPI
NtRequestWakeupLatency(
    IN LATENCY_TIME latency
);

NTSYSAPI
BOOLEAN
NTAPI
NtIsSystemResumeAutomatic(VOID);

NTSYSAPI
NTSTATUS
NTAPI
NtSetThreadExecutionState(
    IN EXECUTION_STATE esFlags,
    OUT EXECUTION_STATE *PreviousFlags
);

NTSYSAPI
NTSTATUS
NTAPI
NtInitiatePowerAction(
    IN POWER_ACTION SystemAction,
    IN SYSTEM_POWER_STATE MinSystemState,
    IN ULONG Flags,
    IN BOOLEAN Asynchronous
);

NTSYSAPI
NTSTATUS
NTAPI
NtRequestDeviceWakeup(
    IN HANDLE Device
);

NTSYSAPI
NTSTATUS
NTAPI
NtCancelDeviceWakeupRequest(
    IN HANDLE Device
);

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @implemented
 */
BOOL
WINAPI
GetSystemPowerStatus(LPSYSTEM_POWER_STATUS PowerStatus)
{
    NTSTATUS Status;
    SYSTEM_BATTERY_STATE SysBatState;

    Status = NtPowerInformation(SystemBatteryState,
                                NULL,
                                0,
                                &SysBatState,
                                sizeof(SYSTEM_BATTERY_STATE));

    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    RtlZeroMemory(PowerStatus, sizeof(LPSYSTEM_POWER_STATUS));

    PowerStatus->BatteryLifeTime = BATTERY_LIFE_UNKNOWN;
    PowerStatus->BatteryFullLifeTime = BATTERY_LIFE_UNKNOWN;

    PowerStatus->BatteryLifePercent = BATTERY_PERCENTAGE_UNKNOWN;
    if (SysBatState.MaxCapacity)
    {
        if (SysBatState.MaxCapacity >= SysBatState.RemainingCapacity)
            PowerStatus->BatteryLifePercent = (SysBatState.RemainingCapacity / SysBatState.MaxCapacity) * 100;
        else
            PowerStatus->BatteryLifePercent = 100; /* 100% */

        if (PowerStatus->BatteryLifePercent <= 32)
            PowerStatus->BatteryFlag |= BATTERY_FLAG_LOW;

        if (PowerStatus->BatteryLifePercent >= 67)
            PowerStatus->BatteryFlag |= BATTERY_FLAG_HIGH;
    }

    if (!SysBatState.BatteryPresent)
        PowerStatus->BatteryFlag |= BATTERY_FLAG_NO_BATTERY;

    if (SysBatState.Charging)
        PowerStatus->BatteryFlag |= BATTERY_FLAG_CHARGING;

    if (!SysBatState.AcOnLine && SysBatState.BatteryPresent)
        PowerStatus->ACLineStatus = AC_LINE_OFFLINE;
    else
        PowerStatus->ACLineStatus = AC_LINE_ONLINE;

    if (SysBatState.EstimatedTime)
        PowerStatus->BatteryLifeTime = SysBatState.EstimatedTime;

    return TRUE;
}

/*
 * @implemented
 */
BOOL WINAPI
SetSystemPowerState(BOOL fSuspend, BOOL fForce)
{
    SYSTEM_POWER_STATE MinSystemState = (!fSuspend ? PowerSystemHibernate : PowerSystemSleeping1);
    ULONG Flags = (!fForce ? POWER_ACTION_QUERY_ALLOWED : 0);
    NTSTATUS Status;

    Status = NtInitiatePowerAction(PowerActionSleep,
                                   MinSystemState,
                                   Flags,
                                   FALSE);

    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
GetDevicePowerState(HANDLE hDevice, BOOL *pfOn)
{
    DEVICE_POWER_STATE DevicePowerState;
    NTSTATUS Status;

    Status = NtGetDevicePowerState(hDevice, &DevicePowerState);

    if (NT_SUCCESS(Status))
    {
        if ((DevicePowerState != PowerDeviceUnspecified) &&
            (DevicePowerState != PowerDeviceD0))
            *pfOn = FALSE;
        else
            *pfOn = TRUE;

        return TRUE;
    }

    BaseSetLastNTError(Status);
    return FALSE;
}

/*
 * @implemented
 */
BOOL
WINAPI
RequestDeviceWakeup(HANDLE hDevice)
{
    NTSTATUS Status;

    Status = NtRequestDeviceWakeup(hDevice);

    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
RequestWakeupLatency(LATENCY_TIME latency)
{
    NTSTATUS Status;

    Status = NtRequestWakeupLatency(latency);

    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
CancelDeviceWakeupRequest(HANDLE hDevice)
{
    NTSTATUS Status;

    Status = NtCancelDeviceWakeupRequest(hDevice);

    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
IsSystemResumeAutomatic(VOID)
{
    return NtIsSystemResumeAutomatic();
}

/*
 * @implemented
 */
BOOL
WINAPI
SetMessageWaitingIndicator(IN HANDLE hMsgIndicator,
                           IN ULONG ulMsgCount)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @implemented
 */
EXECUTION_STATE
WINAPI
SetThreadExecutionState(EXECUTION_STATE esFlags)
{
    EXECUTION_STATE OldFlags;
    NTSTATUS Status;

    Status = NtSetThreadExecutionState(esFlags, &OldFlags);

    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return 0;
    }

    return OldFlags;
}
