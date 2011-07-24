/*++ NDK Version: 0098

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    pofuncs.h

Abstract:

    Function definitions for the Power Subsystem.

Author:

    Alex Ionescu (alexi@tinykrnl.org) - Updated - 27-Feb-2006

--*/

#ifndef _POFUNCS_H
#define _POFUNCS_H

//
// Dependencies
//
#include <umtypes.h>

//
// Native Calls
//
NTSYSCALLAPI
NTSTATUS
NTAPI
NtInitiatePowerAction(
    POWER_ACTION SystemAction,
    SYSTEM_POWER_STATE MinSystemState,
    ULONG Flags,
    BOOLEAN Asynchronous
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtPowerInformation(
    POWER_INFORMATION_LEVEL PowerInformationLevel,
    PVOID InputBuffer,
    ULONG InputBufferLength,
    PVOID OutputBuffer,
    ULONG OutputBufferLength
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetSystemPowerState(
    IN POWER_ACTION SystemAction,
    IN SYSTEM_POWER_STATE MinSystemState,
    IN ULONG Flags
);

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

NTSYSAPI
NTSTATUS
NTAPI
ZwInitiatePowerAction(
    POWER_ACTION SystemAction,
    SYSTEM_POWER_STATE MinSystemState,
    ULONG Flags,
    BOOLEAN Asynchronous
);

NTSYSAPI
NTSTATUS
NTAPI
ZwPowerInformation(
    POWER_INFORMATION_LEVEL PowerInformationLevel,
    PVOID InputBuffer,
    ULONG InputBufferLength,
    PVOID OutputBuffer,
    ULONG OutputBufferLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetSystemPowerState(
    IN POWER_ACTION SystemAction,
    IN SYSTEM_POWER_STATE MinSystemState,
    IN ULONG Flags
);
#endif
