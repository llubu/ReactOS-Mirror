/*
 * PROJECT:         ReactOS Windows-Compatible Session Manager
 * LICENSE:         BSD 2-Clause License
 * FILE:            base/system/smss/smss.h
 * PURPOSE:         Main SMSS Header
 * PROGRAMMERS:     Alex Ionescu
 */

/* DEPENDENCIES ***************************************************************/
#ifndef _SM_
#define _SM_

//
// Native Headers
//
#define WIN32_NO_STATUS
#include <windows.h> // Should just be using ntdef.h I think
#define RTL_NUMBER_OF_V1(A) (sizeof(A)/sizeof((A)[0]))
#define RTL_NUMBER_OF_V2(A) RTL_NUMBER_OF_V1(A)
#ifdef ENABLE_RTL_NUMBER_OF_V2
#define RTL_NUMBER_OF(A) RTL_NUMBER_OF_V2(A)
#else
#define RTL_NUMBER_OF(A) RTL_NUMBER_OF_V1(A)
#endif
#define NTOS_MODE_USER
#include <ndk/ntndk.h>

//
// SM Protocol Header
//
#include "sm/smmsg.h"

/* DEFINES ********************************************************************/

#define SMP_DEBUG_FLAG      0x01
#define SMP_ASYNC_FLAG      0x02
#define SMP_AUTOCHK_FLAG    0x04
#define SMP_SUBSYSTEM_FLAG  0x08
#define SMP_INVALID_PATH    0x10
#define SMP_DEFERRED_FLAG   0x20

/* STRUCTURES *****************************************************************/

typedef struct _SMP_REGISTRY_VALUE
{
    LIST_ENTRY Entry;
    UNICODE_STRING Name;
    UNICODE_STRING Value;
    PCHAR AnsiValue;
} SMP_REGISTRY_VALUE, *PSMP_REGISTRY_VALUE;

/* EXTERNALS ******************************************************************/

extern RTL_CRITICAL_SECTION SmpKnownSubSysLock;
extern LIST_ENTRY SmpKnownSubSysHead;
extern RTL_CRITICAL_SECTION SmpSessionListLock;
extern LIST_ENTRY SmpSessionListHead;
extern ULONG SmpNextSessionId;
extern ULONG SmpNextSessionIdScanMode;
extern BOOLEAN SmpDbgSsLoaded;
extern HANDLE SmpWindowsSubSysProcess;
extern HANDLE SmpSessionsObjectDirectory;
extern HANDLE SmpWindowsSubSysProcessId;
extern BOOLEAN RegPosixSingleInstance;
extern UNICODE_STRING SmpDebugKeyword, SmpASyncKeyword, SmpAutoChkKeyword;
extern PVOID SmpHeap;
extern ULONG SmBaseTag;
extern UNICODE_STRING SmpSystemRoot;
extern PWCHAR SmpDefaultEnvironment;
extern UNICODE_STRING SmpDefaultLibPath;
extern LIST_ENTRY SmpSetupExecuteList;
extern LIST_ENTRY SmpSubSystemsToLoad;
extern LIST_ENTRY SmpExecuteList;
extern LIST_ENTRY SmpSubSystemList;
extern ULONG AttachedSessionId;

/* FUNCTIONS ******************************************************************/

NTSTATUS
NTAPI
SmpTerminate(
    IN PULONG_PTR Parameters,
    IN ULONG ParameterMask,
    IN ULONG ParameterCount
);

NTSTATUS
NTAPI
SmpCreateSecurityDescriptors(
    IN BOOLEAN InitialCall
);

NTSTATUS
NTAPI
SmpInit(
    IN PUNICODE_STRING InitialCommand,
    OUT PHANDLE ProcessHandle
);

NTSTATUS
NTAPI
SmpAcquirePrivilege(
    IN ULONG Privilege,
    OUT PVOID *PrivilegeStat
);

VOID
NTAPI
SmpReleasePrivilege(
    IN PVOID State
);

ULONG
NTAPI
SmpApiLoop(
    IN PVOID Parameter
);

NTSTATUS
NTAPI
SmpExecuteCommand(
    IN PUNICODE_STRING CommandLine,
    IN ULONG MuSessionId,
    OUT PHANDLE ProcessId,
    IN ULONG Flags
);

NTSTATUS
NTAPI
SmpLoadSubSystemsForMuSession(
    IN PULONG MuSessionId,
    OUT PHANDLE ProcessId,
    IN PUNICODE_STRING InitialCommand
);

VOID
NTAPI
SmpPagingFileInitialize(
    VOID
);

NTSTATUS
NTAPI
SmpCreatePagingFileDescriptor(
    IN PUNICODE_STRING PageFileToken
);

NTSTATUS
NTAPI
SmpCreatePagingFiles(
    VOID
);

NTSTATUS
NTAPI
SmpParseCommandLine(
    IN PUNICODE_STRING CommandLine,
    OUT PULONG Flags,
    OUT PUNICODE_STRING FileName,
    OUT PUNICODE_STRING Directory,
    OUT PUNICODE_STRING Arguments
);

NTSTATUS
NTAPI
SmpLoadSubSystem(
    IN PUNICODE_STRING FileName,
    IN PUNICODE_STRING Directory,
    IN PUNICODE_STRING CommandLine,
    IN ULONG MuSessionId,
    OUT PHANDLE ProcessId
);

NTSTATUS
NTAPI
SmpSetProcessMuSessionId(
    IN HANDLE ProcessHandle,
    IN ULONG SessionId
);

BOOLEAN
NTAPI
SmpQueryRegistrySosOption(
    VOID
);

BOOLEAN
NTAPI
SmpSaveAndClearBootStatusData(
    OUT PBOOLEAN BootOkay,
    OUT PBOOLEAN ShutdownOkay
);

VOID
NTAPI
SmpRestoreBootStatusData(
    IN BOOLEAN BootOkay,
    IN BOOLEAN ShutdownOkay
);

BOOLEAN
NTAPI
SmpCheckForCrashDump(
    IN PUNICODE_STRING FileName
);

VOID
NTAPI
SmpTranslateSystemPartitionInformation(
    VOID
);
#endif
