/* $Id: stubs.c,v 1.89 2004/09/22 10:58:06 weiden Exp $
 *
 * KERNEL32.DLL stubs (STUB functions)
 * Remove from this file, if you implement them.
 */

#include <k32.h>

#define NDEBUG
#include "../include/debug.h"


#define STUB \
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED); \
  DPRINT1("%s() is UNIMPLEMENTED!\n", __FUNCTION__)

/*
 * @unimplemented
 */
BOOL
STDCALL
BaseAttachCompleteThunk (VOID)
{
    STUB;
    return FALSE;
}

/*
 * @unimplemented
 */
VOID STDCALL
BaseDumpAppcompatCache(VOID)
{
    STUB;
}

/*
 * @unimplemented
 */
VOID STDCALL
BaseFlushAppcompatCache(VOID)
{
    STUB;
}

/*
 * @unimplemented
 */
VOID STDCALL
BaseCheckAppcompatCache(ULONG Unknown1, ULONG Unknown2, ULONG Unknown3, ULONG Unknown4)
{
    STUB;
}

/*
 * @unimplemented
 */
VOID STDCALL
BaseUpdateAppcompatCache(ULONG Unknown1, ULONG Unknown2, ULONG Unknown3)
{
    STUB;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
CmdBatNotification (
    DWORD   Unknown
    )
{
    STUB;
    return FALSE;
}


/*
 * @unimplemented
 */
int
STDCALL
CompareStringA (
    LCID    Locale,
    DWORD   dwCmpFlags,
    LPCSTR  lpString1,
    int cchCount1,
    LPCSTR  lpString2,
    int cchCount2
    )
{
    STUB;
    return 0;
}


/*
 * @unimplemented
 */
int
STDCALL
CompareStringW (
    LCID    Locale,
    DWORD   dwCmpFlags,
    LPCWSTR lpString1,
    int cchCount1,
    LPCWSTR lpString2,
    int cchCount2
    )
{
    INT Result;
    UNICODE_STRING String1, String2;

    if (!lpString1 || !lpString2)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (dwCmpFlags & ~(NORM_IGNORECASE | NORM_IGNORENONSPACE |
        NORM_IGNORESYMBOLS | SORT_STRINGSORT | NORM_IGNOREKANATYPE |
        NORM_IGNOREWIDTH | 0x10000000))
    {
        SetLastError(ERROR_INVALID_FLAGS);
        return 0;
    }

    if (dwCmpFlags & ~NORM_IGNORECASE)
    {
        DPRINT1("CompareString: STUB flags - 0x%x\n",
           dwCmpFlags & ~NORM_IGNORECASE);
    }

    if (cchCount1 < 0) cchCount1 = lstrlenW(lpString1);
    if (cchCount2 < 0) cchCount2 = lstrlenW(lpString2);

    String1.Length = String1.MaximumLength = cchCount1 * sizeof(WCHAR);
    String1.Buffer = (LPWSTR)lpString1;
    String2.Length = String2.MaximumLength = cchCount2 * sizeof(WCHAR);
    String2.Buffer = (LPWSTR)lpString2;

    Result = RtlCompareUnicodeString(
       &String1, &String2, dwCmpFlags & NORM_IGNORECASE);

    if (Result) /* need to translate result */
        return (Result < 0) ? CSTR_LESS_THAN : CSTR_GREATER_THAN;

    return CSTR_EQUAL;
}


/*
 * @unimplemented
 */
DWORD
STDCALL
CreateVirtualBuffer (
    DWORD   Unknown0,
    DWORD   Unknown1,
    DWORD   Unknown2
    )
{
    STUB;
    return 0;
}


/*
 * @unimplemented
 */
DWORD
STDCALL
ExitVDM (
    DWORD   Unknown0,
    DWORD   Unknown1
    )
{
    STUB;
    return 0;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
ExtendVirtualBuffer (
    DWORD   Unknown0,
    DWORD   Unknown1
    )
{
    STUB;
    return FALSE;
}


/*
 * @unimplemented
 */
int
STDCALL
FoldStringW (
    DWORD   dwMapFlags,
    LPCWSTR lpSrcStr,
    int cchSrc,
    LPWSTR  lpDestStr,
    int cchDest
    )
{
    STUB;
    return 0;
}


/*
 * @unimplemented
 */
int
STDCALL
FoldStringA (
    DWORD   dwMapFlags,
    LPCSTR  lpSrcStr,
    int cchSrc,
    LPSTR   lpDestStr,
    int cchDest
    )
{
    STUB;
    return 0;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
FreeVirtualBuffer (
    HANDLE  hVirtualBuffer
    )
{
    STUB;
    return FALSE;
}


/*
 * @unimplemented
 */
DWORD
STDCALL
GetNextVDMCommand (
    DWORD   Unknown0
    )
{
    STUB;
    return 0;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
GetStringTypeExW (
    LCID    Locale,
    DWORD   dwInfoType,
    LPCWSTR lpSrcStr,
    int cchSrc,
    LPWORD  lpCharType
    )
{
    STUB;
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
GetStringTypeExA (
    LCID    Locale,
    DWORD   dwInfoType,
    LPCSTR  lpSrcStr,
    int cchSrc,
    LPWORD  lpCharType
    )
{
    STUB;
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
GetStringTypeW (
    DWORD   dwInfoType,
    LPCWSTR lpSrcStr,
    int cchSrc,
    LPWORD  lpCharType
    )
{
    STUB;
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
GetStringTypeA (
    LCID    Locale,
    DWORD   dwInfoType,
    LPCSTR  lpSrcStr,
    int cchSrc,
    LPWORD  lpCharType
    )
{
    STUB;
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
GetSystemPowerStatus (
    LPSYSTEM_POWER_STATUS PowerStatus
    )
{
    STUB;
    return 0;
}


/*
 * @unimplemented
 */
DWORD
STDCALL
GetVDMCurrentDirectories (
    DWORD   Unknown0,
    DWORD   Unknown1
    )
{
    STUB;
    return 0;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
RegisterConsoleVDM (
    DWORD   Unknown0,
    DWORD   Unknown1,
    DWORD   Unknown2,
    DWORD   Unknown3,
    DWORD   Unknown4,
    DWORD   Unknown5,
    DWORD   Unknown6,
    DWORD   Unknown7,
    DWORD   Unknown8,
    DWORD   Unknown9,
    DWORD   Unknown10
    )
{
    STUB;
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
RegisterWowBaseHandlers (
    DWORD   Unknown0
    )
{
    STUB;
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
RegisterWowExec (
    DWORD   Unknown0
    )
{
    STUB;
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL STDCALL
SetSystemPowerState (
    BOOL fSuspend,
    BOOL fForce
    )
{
    STUB;
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
SetVDMCurrentDirectories (
    DWORD   Unknown0,
    DWORD   Unknown1
    )
{
    STUB;
    return FALSE;
}


/*
 * @unimplemented
 */
DWORD
STDCALL
TrimVirtualBuffer (
    DWORD   Unknown0
    )
{
    STUB;
    return 0;
}


/*
 * @unimplemented
 */
DWORD
STDCALL
VDMConsoleOperation (
    DWORD   Unknown0,
    DWORD   Unknown1
    )
{
    STUB;
    return 0;
}


/*
 * @unimplemented
 */
DWORD
STDCALL
VDMOperationStarted (
    DWORD   Unknown0
    )
{
    STUB;
    return 0;
}


/*
 * @unimplemented
 */
DWORD
STDCALL
VerLanguageNameA (
    DWORD   wLang,
    LPSTR   szLang,
    DWORD   nSize
    )
{
    STUB;
    return 0;
}


/*
 * @unimplemented
 */
DWORD
STDCALL
VerLanguageNameW (
    DWORD   wLang,
    LPWSTR  szLang,
    DWORD   nSize
    )
{
    STUB;
    return 0;
}


/*
 * @unimplemented
 */
DWORD
STDCALL
VirtualBufferExceptionHandler (
    DWORD   Unknown0,
    DWORD   Unknown1,
    DWORD   Unknown2
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
ActivateActCtx(
    HANDLE hActCtx,
    ULONG_PTR *lpCookie
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
VOID
STDCALL
AddRefActCtx(
    HANDLE hActCtx
    )
{
    STUB;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
AllocateUserPhysicalPages(
    HANDLE hProcess,
    PULONG_PTR NumberOfPages,
    PULONG_PTR PageArray
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
AssignProcessToJobObject(
    HANDLE hJob,
    HANDLE hProcess
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
BindIoCompletionCallback (
    HANDLE FileHandle,
    LPOVERLAPPED_COMPLETION_ROUTINE Function,
    ULONG Flags
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
CancelDeviceWakeupRequest(
    HANDLE hDevice
    )
{
    STUB;
    return 0;
}


/*
 * @unimplemented
 */
HANDLE
STDCALL
CreateActCtxA(
    PCACTCTXA pActCtx
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
HANDLE
STDCALL
CreateActCtxW(
    PCACTCTXW pActCtx
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
CreateJobSet (
    ULONG NumJob,
    PJOB_SET_ARRAY UserJobSet,
    ULONG Flags)
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
DeactivateActCtx(
    DWORD dwFlags,
    ULONG_PTR ulCookie
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
FindActCtxSectionGuid(
    DWORD dwFlags,
    const GUID *lpExtensionGuid,
    ULONG ulSectionId,
    const GUID *lpGuidToFind,
    PACTCTX_SECTION_KEYED_DATA ReturnedData
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
FindVolumeClose(
    HANDLE hFindVolume
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
FindVolumeMountPointClose(
    HANDLE hFindVolumeMountPoint
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
FreeUserPhysicalPages(
    HANDLE hProcess,
    PULONG_PTR NumberOfPages,
    PULONG_PTR PageArray
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GetCurrentActCtx(
    HANDLE *lphActCtx)
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GetDevicePowerState(
    HANDLE hDevice,
    BOOL *pfOn
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
VOID
STDCALL
GetNativeSystemInfo(
    LPSYSTEM_INFO lpSystemInfo
    )
{
    STUB;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GetNumaHighestNodeNumber(
    PULONG HighestNodeNumber
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GetNumaNodeProcessorMask(
    UCHAR Node,
    PULONGLONG ProcessorMask
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GetNumaProcessorNode(
    UCHAR Processor,
    PUCHAR NodeNumber
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GetThreadIOPendingFlag(
    HANDLE hThread,
    PBOOL lpIOIsPending
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
UINT
STDCALL
GetWriteWatch(
    DWORD  dwFlags,
    PVOID  lpBaseAddress,
    SIZE_T dwRegionSize,
    PVOID *lpAddresses,
    PULONG_PTR lpdwCount,
    PULONG lpdwGranularity
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
HeapQueryInformation (
    HANDLE HeapHandle, 
    HEAP_INFORMATION_CLASS HeapInformationClass,
    PVOID HeapInformation OPTIONAL,
    SIZE_T HeapInformationLength OPTIONAL,
    PSIZE_T ReturnLength OPTIONAL
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
HeapSetInformation (
    HANDLE HeapHandle, 
    HEAP_INFORMATION_CLASS HeapInformationClass,
    PVOID HeapInformation OPTIONAL,
    SIZE_T HeapInformationLength OPTIONAL
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
IsProcessInJob (
    HANDLE ProcessHandle,
    HANDLE JobHandle,
    PBOOL Result
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
IsSystemResumeAutomatic(
    VOID
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
IsWow64Process(
    HANDLE hProcess,
    PBOOL Wow64Process
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
MapUserPhysicalPages(
    PVOID VirtualAddress,
    ULONG_PTR NumberOfPages,
    PULONG_PTR PageArray OPTIONAL
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
MapUserPhysicalPagesScatter(
    PVOID *VirtualAddresses,
    ULONG_PTR NumberOfPages,
    PULONG_PTR PageArray OPTIONAL
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
QueryActCtxW(
    DWORD dwFlags,
    HANDLE hActCtx,
    PVOID pvSubInstance,
    ULONG ulInfoClass,
    PVOID pvBuffer,
    SIZE_T cbBuffer OPTIONAL,
    SIZE_T *pcbWrittenOrRequired OPTIONAL
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
QueryInformationJobObject(
    HANDLE hJob,
    JOBOBJECTINFOCLASS JobObjectInformationClass,
    LPVOID lpJobObjectInformation,
    DWORD cbJobObjectInformationLength,
    LPDWORD lpReturnLength
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
DWORD
STDCALL
QueueUserAPC(
    PAPCFUNC pfnAPC,
    HANDLE hThread,
    ULONG_PTR dwData
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
QueueUserWorkItem(
    LPTHREAD_START_ROUTINE Function,
    PVOID Context,
    ULONG Flags
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
ReadDirectoryChangesW(
    HANDLE hDirectory,
    LPVOID lpBuffer,
    DWORD nBufferLength,
    BOOL bWatchSubtree,
    DWORD dwNotifyFilter,
    LPDWORD lpBytesReturned,
    LPOVERLAPPED lpOverlapped,
    LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
ReadFileScatter(
    HANDLE hFile,
    FILE_SEGMENT_ELEMENT aSegmentArray[],
    DWORD nNumberOfBytesToRead,
    LPDWORD lpReserved,
    LPOVERLAPPED lpOverlapped
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
RegisterWaitForSingleObject(
    PHANDLE phNewWaitObject,
    HANDLE hObject,
    WAITORTIMERCALLBACK Callback,
    PVOID Context,
    ULONG dwMilliseconds,
    ULONG dwFlags
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
HANDLE
STDCALL
RegisterWaitForSingleObjectEx(
    HANDLE hObject,
    WAITORTIMERCALLBACK Callback,
    PVOID Context,
    ULONG dwMilliseconds,
    ULONG dwFlags
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
VOID
STDCALL
ReleaseActCtx(
    HANDLE hActCtx
    )
{
    STUB;
}

/*
 * @unimplemented
 */
ULONG
STDCALL
RemoveVectoredExceptionHandler(
    PVOID VectoredHandlerHandle
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
RequestDeviceWakeup(
    HANDLE hDevice
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
RequestWakeupLatency(
    LATENCY_TIME latency
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
UINT
STDCALL
ResetWriteWatch(
    LPVOID lpBaseAddress,
    SIZE_T dwRegionSize
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
VOID
STDCALL
RestoreLastError(
    DWORD dwErrCode
    )
{
    STUB;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
SetInformationJobObject(
    HANDLE hJob,
    JOBOBJECTINFOCLASS JobObjectInformationClass,
    LPVOID lpJobObjectInformation,
    DWORD cbJobObjectInformationLength
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
SetMessageWaitingIndicator(
    HANDLE hMsgIndicator,
    ULONG ulMsgCount
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
EXECUTION_STATE
STDCALL
SetThreadExecutionState(
    EXECUTION_STATE esFlags
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
TerminateJobObject(
    HANDLE hJob,
    UINT uExitCode
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
TzSpecificLocalTimeToSystemTime(
    LPTIME_ZONE_INFORMATION lpTimeZoneInformation,
    LPSYSTEMTIME lpLocalTime,
    LPSYSTEMTIME lpUniversalTime
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
UnregisterWait(
    HANDLE WaitHandle
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
UnregisterWaitEx(
    HANDLE WaitHandle,
    HANDLE CompletionEvent
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
WriteFileGather(
    HANDLE hFile,
    FILE_SEGMENT_ELEMENT aSegmentArray[],
    DWORD nNumberOfBytesToWrite,
    LPDWORD lpReserved,
    LPOVERLAPPED lpOverlapped
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
DWORD
STDCALL
WTSGetActiveConsoleSessionId(VOID)
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
ZombifyActCtx(
    HANDLE hActCtx
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
HANDLE
STDCALL
CreateJobObjectW(
    LPSECURITY_ATTRIBUTES lpJobAttributes,
    LPCWSTR lpName
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
DeleteVolumeMountPointW(
    LPCWSTR lpszVolumeMountPoint
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
DnsHostnameToComputerNameW (
    LPCWSTR Hostname,
    LPWSTR ComputerName,
    LPDWORD nSize
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
FindActCtxSectionStringW(
    DWORD dwFlags,
    const GUID *lpExtensionGuid,
    ULONG ulSectionId,
    LPCWSTR lpStringToFind,
    PACTCTX_SECTION_KEYED_DATA ReturnedData
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
HANDLE
STDCALL
FindFirstVolumeW(
    LPCWSTR lpszVolumeName,
    DWORD cchBufferLength
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
HANDLE
STDCALL
FindFirstVolumeMountPointW(
    LPWSTR lpszRootPathName,
    LPWSTR lpszVolumeMountPoint,
    DWORD cchBufferLength
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
FindNextVolumeW(
    HANDLE hFindVolume,
    LPWSTR lpszVolumeName,
    DWORD cchBufferLength
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
FindNextVolumeMountPointW(
    HANDLE hFindVolumeMountPoint,
    LPWSTR lpszVolumeMountPoint,
    DWORD cchBufferLength
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
DWORD
STDCALL
GetDllDirectoryW(
    DWORD nBufferLength,
    LPWSTR lpBuffer
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
DWORD
STDCALL
GetFirmwareEnvironmentVariableW(
    LPCWSTR lpName,
    LPCWSTR lpGuid,
    PVOID   pBuffer,
    DWORD    nSize
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
DWORD
STDCALL
GetLongPathNameW(
    LPCWSTR lpszShortPath,
    LPWSTR  lpszLongPath,
    DWORD    cchBuffer
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GetModuleHandleExW(
    DWORD        dwFlags,
    LPCWSTR     lpModuleName,
    HMODULE*    phModule
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
UINT
STDCALL
GetSystemWow64DirectoryW(
    LPWSTR lpBuffer,
    UINT uSize
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GetVolumeNameForVolumeMountPointW(
    LPCWSTR lpszVolumeMountPoint,
    LPWSTR lpszVolumeName,
    DWORD cchBufferLength
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GetVolumePathNameW(
    LPCWSTR lpszFileName,
    LPWSTR lpszVolumePathName,
    DWORD cchBufferLength
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GetVolumePathNamesForVolumeNameW(
    LPCWSTR lpszVolumeName,
    LPWSTR lpszVolumePathNames,
    DWORD cchBufferLength,
    PDWORD lpcchReturnLength
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
HANDLE
STDCALL
OpenJobObjectW(
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    LPCWSTR lpName
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
ReplaceFileW(
    LPCWSTR lpReplacedFileName,
    LPCWSTR lpReplacementFileName,
    LPCWSTR lpBackupFileName,
    DWORD   dwReplaceFlags,
    LPVOID  lpExclude,
    LPVOID  lpReserved
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
SetComputerNameExW (
    COMPUTER_NAME_FORMAT NameType,
    LPCWSTR lpBuffer
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
SetDllDirectoryW(
    LPCWSTR lpPathName
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
SetFirmwareEnvironmentVariableW(
    LPCWSTR lpName,
    LPCWSTR lpGuid,
    PVOID    pValue,
    DWORD    nSize
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
SetVolumeMountPointW(
    LPCWSTR lpszVolumeMountPoint,
    LPCWSTR lpszVolumeName
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
VerifyVersionInfoW(
    LPOSVERSIONINFOEXW lpVersionInformation,
    DWORD dwTypeMask,
    DWORDLONG dwlConditionMask
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
HANDLE
STDCALL
CreateJobObjectA(
    LPSECURITY_ATTRIBUTES lpJobAttributes,
    LPCSTR lpName
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
DeleteVolumeMountPointA(
    LPCSTR lpszVolumeMountPoint
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
DnsHostnameToComputerNameA (
    LPCSTR Hostname,
    LPSTR ComputerName,
    LPDWORD nSize
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
FindActCtxSectionStringA(
    DWORD dwFlags,
    const GUID *lpExtensionGuid,
    ULONG ulSectionId,
    LPCSTR lpStringToFind,
    PACTCTX_SECTION_KEYED_DATA ReturnedData
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
HANDLE
STDCALL
FindFirstVolumeA(
    LPCSTR lpszVolumeName,
    DWORD cchBufferLength
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
HANDLE
STDCALL
FindFirstVolumeMountPointA(
    LPSTR lpszRootPathName,
    LPSTR lpszVolumeMountPoint,
    DWORD cchBufferLength
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
FindNextVolumeA(
    HANDLE hFindVolume,
    LPCSTR lpszVolumeName,
    DWORD cchBufferLength
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
FindNextVolumeMountPointA(
    HANDLE hFindVolumeMountPoint,
    LPSTR lpszVolumeMountPoint,
    DWORD cchBufferLength
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
DWORD
STDCALL
GetDllDirectoryA(
    DWORD nBufferLength,
    LPSTR lpBuffer
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
DWORD
STDCALL
GetFirmwareEnvironmentVariableA(
    LPCSTR lpName,
    LPCSTR lpGuid,
    PVOID   pBuffer,
    DWORD    nSize
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
DWORD
STDCALL
GetLongPathNameA(
    LPCSTR lpszShortPath,
    LPSTR  lpszLongPath,
    DWORD    cchBuffer
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GetModuleHandleExA(
    DWORD        dwFlags,
    LPCSTR     lpModuleName,
    HMODULE*    phModule
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
UINT
STDCALL
GetSystemWow64DirectoryA(
    LPSTR lpBuffer,
    UINT uSize
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GetVolumeNameForVolumeMountPointA(
    LPCSTR lpszVolumeMountPoint,
    LPSTR lpszVolumeName,
    DWORD cchBufferLength
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GetVolumePathNameA(
    LPCSTR lpszFileName,
    LPSTR lpszVolumePathName,
    DWORD cchBufferLength
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GetVolumePathNamesForVolumeNameA(
    LPCSTR lpszVolumeName,
    LPSTR lpszVolumePathNames,
    DWORD cchBufferLength,
    PDWORD lpcchReturnLength
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
HANDLE
STDCALL
OpenJobObjectA(
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    LPCSTR lpName
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
ReplaceFileA(
    LPCSTR  lpReplacedFileName,
    LPCSTR  lpReplacementFileName,
    LPCSTR  lpBackupFileName,
    DWORD   dwReplaceFlags,
    LPVOID  lpExclude,
    LPVOID  lpReserved
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
SetComputerNameExA (
    COMPUTER_NAME_FORMAT NameType,
    LPCSTR lpBuffer
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
SetDllDirectoryA(
    LPCSTR lpPathName
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
SetFirmwareEnvironmentVariableA(
    LPCSTR lpName,
    LPCSTR lpGuid,
    PVOID    pValue,
    DWORD    nSize
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
SetVolumeMountPointA(
    LPCSTR lpszVolumeMountPoint,
    LPCSTR lpszVolumeName
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
VerifyVersionInfoA(
    LPOSVERSIONINFOEXA lpVersionInformation,
    DWORD dwTypeMask,
    DWORDLONG dwlConditionMask
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
EnumSystemLanguageGroupsW(
    LANGUAGEGROUP_ENUMPROCW lpLanguageGroupEnumProc,
    DWORD                   dwFlags,
    LONG_PTR                lParam)
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
ULONGLONG
STDCALL
VerSetConditionMask(
        ULONGLONG   ConditionMask,
        DWORD   TypeMask,
        BYTE    Condition
        )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL GetConsoleKeyboardLayoutNameA(LPSTR name)
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL GetConsoleKeyboardLayoutNameW(LPWSTR name)
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
DWORD STDCALL GetHandleContext(HANDLE hnd)
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
HANDLE STDCALL CreateSocketHandle(VOID)
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL SetHandleContext(HANDLE hnd,DWORD context)
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL SetConsoleInputExeNameA(LPCSTR name)
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL SetConsoleInputExeNameW(LPCWSTR name)
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL UTRegister( HMODULE hModule, LPSTR lpsz16BITDLL,
                        LPSTR lpszInitName, LPSTR lpszProcName,
                        FARPROC *ppfn32Thunk, FARPROC pfnUT32CallBack,
                        LPVOID lpBuff )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
VOID STDCALL UTUnRegister( HMODULE hModule )
{
    STUB;
}

/*
 * @unimplemented
 */
#if 0
FARPROC STDCALL DelayLoadFailureHook(unsigned int dliNotify, PDelayLoadInfo pdli)
#else
FARPROC STDCALL DelayLoadFailureHook(unsigned int dliNotify, PVOID pdli)
#endif
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
NTSTATUS STDCALL CreateNlsSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,ULONG Size,ULONG AccessMask)
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL GetConsoleInputExeNameA(ULONG length,LPCSTR name)
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL GetConsoleInputExeNameW(ULONG length,LPCWSTR name)
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL IsValidUILanguage(LANGID langid)
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
VOID STDCALL NlsConvertIntegerToString(ULONG Value,ULONG Base,ULONG strsize, LPWSTR str, ULONG strsize2)
{
    STUB;
}

/*
 * @unimplemented
 */
UINT STDCALL SetCPGlobal(UINT CodePage)
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
SetClientTimeZoneInformation(
		       CONST TIME_ZONE_INFORMATION *lpTimeZoneInformation
		       )
{
    STUB;
    return 0;
}
