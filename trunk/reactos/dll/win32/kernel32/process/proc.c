/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/proc/proc.c
 * PURPOSE:         Process functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/* INCLUDES ****************************************************************/

#include <k32.h>

#define NDEBUG
#include <debug.h>


typedef INT (WINAPI *MessageBoxW_Proc) (HWND, LPCWSTR, LPCWSTR, UINT);

/* GLOBALS *******************************************************************/

WaitForInputIdleType  lpfnGlobalRegisterWaitForInputIdle;

LPSTARTUPINFOA lpLocalStartupInfo = NULL;

VOID WINAPI
RegisterWaitForInputIdle(WaitForInputIdleType lpfnRegisterWaitForInputIdle);

/* FUNCTIONS ****************************************************************/

/*
 * @implemented
 */
BOOL
WINAPI
GetProcessAffinityMask(HANDLE hProcess,
                       PDWORD_PTR lpProcessAffinityMask,
                       PDWORD_PTR lpSystemAffinityMask)
{
    PROCESS_BASIC_INFORMATION ProcessInfo;
    SYSTEM_BASIC_INFORMATION SystemInfo;
    NTSTATUS Status;

    Status = NtQuerySystemInformation(SystemBasicInformation,
                                      &SystemInfo,
                                      sizeof(SystemInfo),
                                      NULL);
    if (!NT_SUCCESS(Status))
    {
        SetLastErrorByStatus(Status);
        return FALSE;
    }

    Status = NtQueryInformationProcess(hProcess,
                                       ProcessBasicInformation,
                                       (PVOID)&ProcessInfo,
                                       sizeof(PROCESS_BASIC_INFORMATION),
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        SetLastErrorByStatus(Status);
        return FALSE;
    }

    *lpProcessAffinityMask = (DWORD)ProcessInfo.AffinityMask;
    *lpSystemAffinityMask = (DWORD)SystemInfo.ActiveProcessorsAffinityMask;

    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
SetProcessAffinityMask(HANDLE hProcess,
                       DWORD_PTR dwProcessAffinityMask)
{
    NTSTATUS Status;

    Status = NtSetInformationProcess(hProcess,
                                     ProcessAffinityMask,
                                     (PVOID)&dwProcessAffinityMask,
                                     sizeof(DWORD));
    if (!NT_SUCCESS(Status))
    {
        SetLastErrorByStatus(Status);
        return FALSE;
    }

    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
GetProcessShutdownParameters(LPDWORD lpdwLevel,
                             LPDWORD lpdwFlags)
{
    CSR_API_MESSAGE CsrRequest;
    ULONG Request;
    NTSTATUS Status;

    Request = GET_SHUTDOWN_PARAMETERS;
    Status = CsrClientCallServer(&CsrRequest,
                                 NULL,
                                 MAKE_CSR_API(Request, CSR_NATIVE),
                                 sizeof(CSR_API_MESSAGE));
    if (!NT_SUCCESS(Status) || !NT_SUCCESS(CsrRequest.Status))
    {
        SetLastErrorByStatus(Status);
        return FALSE;
    }

    *lpdwLevel = CsrRequest.Data.GetShutdownParametersRequest.Level;
    *lpdwFlags = CsrRequest.Data.GetShutdownParametersRequest.Flags;

    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
SetProcessShutdownParameters(DWORD dwLevel,
                             DWORD dwFlags)
{
    CSR_API_MESSAGE CsrRequest;
    ULONG Request;
    NTSTATUS Status;

    CsrRequest.Data.SetShutdownParametersRequest.Level = dwLevel;
    CsrRequest.Data.SetShutdownParametersRequest.Flags = dwFlags;

    Request = SET_SHUTDOWN_PARAMETERS;
    Status = CsrClientCallServer(&CsrRequest,
                                 NULL,
                                 MAKE_CSR_API(Request, CSR_NATIVE),
                                 sizeof(CSR_API_MESSAGE));
    if (!NT_SUCCESS(Status) || !NT_SUCCESS(CsrRequest.Status))
    {
        SetLastErrorByStatus(Status);
        return FALSE;
    }

    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
GetProcessWorkingSetSize(HANDLE hProcess,
                         PSIZE_T lpMinimumWorkingSetSize,
                         PSIZE_T lpMaximumWorkingSetSize)
{
    QUOTA_LIMITS QuotaLimits;
    NTSTATUS Status;

    Status = NtQueryInformationProcess(hProcess,
                                       ProcessQuotaLimits,
                                       &QuotaLimits,
                                       sizeof(QUOTA_LIMITS),
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        SetLastErrorByStatus(Status);
        return FALSE;
    }

    *lpMinimumWorkingSetSize = QuotaLimits.MinimumWorkingSetSize;
    *lpMaximumWorkingSetSize = QuotaLimits.MaximumWorkingSetSize;

    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
SetProcessWorkingSetSize(HANDLE hProcess,
                         SIZE_T dwMinimumWorkingSetSize,
                         SIZE_T dwMaximumWorkingSetSize)
{
    QUOTA_LIMITS QuotaLimits;
    NTSTATUS Status;

    QuotaLimits.MinimumWorkingSetSize = dwMinimumWorkingSetSize;
    QuotaLimits.MaximumWorkingSetSize = dwMaximumWorkingSetSize;

    Status = NtSetInformationProcess(hProcess,
                                     ProcessQuotaLimits,
                                     &QuotaLimits,
                                     sizeof(QUOTA_LIMITS));
    if (!NT_SUCCESS(Status))
    {
        SetLastErrorByStatus(Status);
        return FALSE;
    }

    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
GetProcessTimes(HANDLE hProcess,
                LPFILETIME lpCreationTime,
                LPFILETIME lpExitTime,
                LPFILETIME lpKernelTime,
                LPFILETIME lpUserTime)
{
    KERNEL_USER_TIMES Kut;
    NTSTATUS Status;

    Status = NtQueryInformationProcess(hProcess,
                                       ProcessTimes,
                                       &Kut,
                                       sizeof(Kut),
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        SetLastErrorByStatus(Status);
        return FALSE;
    }

    lpCreationTime->dwLowDateTime = Kut.CreateTime.u.LowPart;
    lpCreationTime->dwHighDateTime = Kut.CreateTime.u.HighPart;

    lpExitTime->dwLowDateTime = Kut.ExitTime.u.LowPart;
    lpExitTime->dwHighDateTime = Kut.ExitTime.u.HighPart;

    lpKernelTime->dwLowDateTime = Kut.KernelTime.u.LowPart;
    lpKernelTime->dwHighDateTime = Kut.KernelTime.u.HighPart;

    lpUserTime->dwLowDateTime = Kut.UserTime.u.LowPart;
    lpUserTime->dwHighDateTime = Kut.UserTime.u.HighPart;

    return TRUE;
}


/*
 * @implemented
 */
HANDLE
WINAPI
GetCurrentProcess(VOID)
{
    return (HANDLE)NtCurrentProcess();
}


/*
 * @implemented
 */
HANDLE
WINAPI
GetCurrentThread(VOID)
{
    return (HANDLE)NtCurrentThread();
}


/*
 * @implemented
 */
DWORD
WINAPI
GetCurrentProcessId(VOID)
{
    return HandleToUlong(GetTeb()->ClientId.UniqueProcess);
}


/*
 * @implemented
 */
BOOL
WINAPI
GetExitCodeProcess(HANDLE hProcess,
                   LPDWORD lpExitCode)
{
    PROCESS_BASIC_INFORMATION ProcessBasic;
    NTSTATUS Status;

    Status = NtQueryInformationProcess(hProcess,
                                       ProcessBasicInformation,
                                       &ProcessBasic,
                                       sizeof(PROCESS_BASIC_INFORMATION),
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        SetLastErrorByStatus(Status);
        return FALSE;
    }

    *lpExitCode = (DWORD)ProcessBasic.ExitStatus;

    return TRUE;
}


/*
 * @implemented
 */
DWORD
WINAPI
GetProcessId(HANDLE Process)
{
    PROCESS_BASIC_INFORMATION ProcessBasic;
    NTSTATUS Status;

    Status = NtQueryInformationProcess(Process,
                                       ProcessBasicInformation,
                                       &ProcessBasic,
                                       sizeof(PROCESS_BASIC_INFORMATION),
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        SetLastErrorByStatus(Status);
        return 0;
    }

    return (DWORD)ProcessBasic.UniqueProcessId;
}


/*
 * @implemented
 */
HANDLE
WINAPI
OpenProcess(DWORD dwDesiredAccess,
            BOOL bInheritHandle,
            DWORD dwProcessId)
{
    NTSTATUS errCode;
    HANDLE ProcessHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    CLIENT_ID ClientId;

    ClientId.UniqueProcess = UlongToHandle(dwProcessId);
    ClientId.UniqueThread = 0;

    InitializeObjectAttributes(&ObjectAttributes,
                               NULL,
                               (bInheritHandle ? OBJ_INHERIT : 0),
                               NULL,
                               NULL);

    errCode = NtOpenProcess(&ProcessHandle,
                            dwDesiredAccess,
                            &ObjectAttributes,
                            &ClientId);
    if (!NT_SUCCESS(errCode))
    {
        SetLastErrorByStatus(errCode);
        return NULL;
    }

    return ProcessHandle;
}


/*
 * @implemented
 */
UINT
WINAPI
WinExec(LPCSTR lpCmdLine,
        UINT uCmdShow)
{
    STARTUPINFOA StartupInfo;
    PROCESS_INFORMATION  ProcessInformation;
    DWORD dosErr;

    RtlZeroMemory(&StartupInfo, sizeof(StartupInfo));
    StartupInfo.cb = sizeof(STARTUPINFOA);
    StartupInfo.wShowWindow = (WORD)uCmdShow;
    StartupInfo.dwFlags = 0;

    if (!CreateProcessA(NULL,
                        (PVOID)lpCmdLine,
                        NULL,
                        NULL,
                        FALSE,
                        0,
                        NULL,
                        NULL,
                        &StartupInfo,
                        &ProcessInformation))
    {
        dosErr = GetLastError();
        return dosErr < 32 ? dosErr : ERROR_BAD_FORMAT;
    }

    if (NULL != lpfnGlobalRegisterWaitForInputIdle)
    {
        lpfnGlobalRegisterWaitForInputIdle(ProcessInformation.hProcess,
                                           10000);
    }

    NtClose(ProcessInformation.hProcess);
    NtClose(ProcessInformation.hThread);

    return 33; /* Something bigger than 31 means success. */
}


/*
 * @implemented
 */
VOID
WINAPI
RegisterWaitForInputIdle(WaitForInputIdleType lpfnRegisterWaitForInputIdle)
{
    lpfnGlobalRegisterWaitForInputIdle = lpfnRegisterWaitForInputIdle;
    return;
}

/*
 * @implemented
 */
VOID
WINAPI
GetStartupInfoW(LPSTARTUPINFOW lpStartupInfo)
{
    PRTL_USER_PROCESS_PARAMETERS Params;

    if (lpStartupInfo == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return;
    }

    Params = NtCurrentPeb()->ProcessParameters;

    lpStartupInfo->cb = sizeof(STARTUPINFOW);
    lpStartupInfo->lpDesktop = Params->DesktopInfo.Buffer;
    lpStartupInfo->lpTitle = Params->WindowTitle.Buffer;
    lpStartupInfo->dwX = Params->StartingX;
    lpStartupInfo->dwY = Params->StartingY;
    lpStartupInfo->dwXSize = Params->CountX;
    lpStartupInfo->dwYSize = Params->CountY;
    lpStartupInfo->dwXCountChars = Params->CountCharsX;
    lpStartupInfo->dwYCountChars = Params->CountCharsY;
    lpStartupInfo->dwFillAttribute = Params->FillAttribute;
    lpStartupInfo->dwFlags = Params->WindowFlags;
    lpStartupInfo->wShowWindow = (WORD)Params->ShowWindowFlags;
    lpStartupInfo->cbReserved2 = Params->RuntimeData.Length;
    lpStartupInfo->lpReserved2 = (LPBYTE)Params->RuntimeData.Buffer;

    lpStartupInfo->hStdInput = Params->StandardInput;
    lpStartupInfo->hStdOutput = Params->StandardOutput;
    lpStartupInfo->hStdError = Params->StandardError;
}


/*
 * @implemented
 */
VOID
WINAPI
GetStartupInfoA(LPSTARTUPINFOA lpStartupInfo)
{
    PRTL_USER_PROCESS_PARAMETERS Params;
    ANSI_STRING AnsiString;

    if (lpStartupInfo == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return;
    }

    Params = NtCurrentPeb ()->ProcessParameters;

    RtlAcquirePebLock ();

    /* FIXME - not thread-safe */
    if (lpLocalStartupInfo == NULL)
    {
        /* create new local startup info (ansi) */
        lpLocalStartupInfo = RtlAllocateHeap(RtlGetProcessHeap(),
                                             0,
                                             sizeof(STARTUPINFOA));
        if (lpLocalStartupInfo == NULL)
        {
            RtlReleasePebLock();
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return;
        }

        lpLocalStartupInfo->cb = sizeof(STARTUPINFOA);

        /* copy window title string */
        RtlUnicodeStringToAnsiString(&AnsiString,
                                     &Params->WindowTitle,
                                     TRUE);
        lpLocalStartupInfo->lpTitle = AnsiString.Buffer;

        /* copy desktop info string */
        RtlUnicodeStringToAnsiString(&AnsiString,
                                     &Params->DesktopInfo,
                                     TRUE);
        lpLocalStartupInfo->lpDesktop = AnsiString.Buffer;

        /* copy shell info string */
        RtlUnicodeStringToAnsiString(&AnsiString,
                                     &Params->ShellInfo,
                                     TRUE);
        lpLocalStartupInfo->lpReserved = AnsiString.Buffer;

        lpLocalStartupInfo->dwX = Params->StartingX;
        lpLocalStartupInfo->dwY = Params->StartingY;
        lpLocalStartupInfo->dwXSize = Params->CountX;
        lpLocalStartupInfo->dwYSize = Params->CountY;
        lpLocalStartupInfo->dwXCountChars = Params->CountCharsX;
        lpLocalStartupInfo->dwYCountChars = Params->CountCharsY;
        lpLocalStartupInfo->dwFillAttribute = Params->FillAttribute;
        lpLocalStartupInfo->dwFlags = Params->WindowFlags;
        lpLocalStartupInfo->wShowWindow = (WORD)Params->ShowWindowFlags;
        lpLocalStartupInfo->cbReserved2 = Params->RuntimeData.Length;
        lpLocalStartupInfo->lpReserved2 = (LPBYTE)Params->RuntimeData.Buffer;

        lpLocalStartupInfo->hStdInput = Params->StandardInput;
        lpLocalStartupInfo->hStdOutput = Params->StandardOutput;
        lpLocalStartupInfo->hStdError = Params->StandardError;
    }

    RtlReleasePebLock();

    /* copy local startup info data to external startup info */
    memcpy(lpStartupInfo,
           lpLocalStartupInfo,
           sizeof(STARTUPINFOA));
}


/*
 * @implemented
 */
BOOL
WINAPI
FlushInstructionCache(HANDLE hProcess,
                      LPCVOID lpBaseAddress,
                      SIZE_T dwSize)
{
    NTSTATUS Status;

    Status = NtFlushInstructionCache(hProcess,
                                     (PVOID)lpBaseAddress,
                                     dwSize);
    if (!NT_SUCCESS(Status))
    {
        SetLastErrorByStatus(Status);
        return FALSE;
    }

    return TRUE;
}


/*
 * @implemented
 */
VOID
WINAPI
ExitProcess(UINT uExitCode)
{
    CSR_API_MESSAGE CsrRequest;
    ULONG Request;
    NTSTATUS Status;

    /* kill sibling threads ... we want to be alone at this point */
    NtTerminateProcess(NULL, 0);

    /* unload all dll's */
    LdrShutdownProcess();

    /* notify csrss of process termination */
    Request = TERMINATE_PROCESS;
    Status = CsrClientCallServer(&CsrRequest,
                                 NULL,
                                 MAKE_CSR_API(Request, CSR_NATIVE),
                                 sizeof(CSR_API_MESSAGE));
    if (!NT_SUCCESS(Status) || !NT_SUCCESS(CsrRequest.Status))
    {
        DPRINT("Failed to tell csrss about terminating process\n");
    }

    NtTerminateProcess(NtCurrentProcess (),
                       uExitCode);

    /* should never get here */
    ASSERT(0);
    while(1);
}


/*
 * @implemented
 */
BOOL
WINAPI
TerminateProcess(HANDLE hProcess,
                 UINT uExitCode)
{
    NTSTATUS Status;

    if (hProcess == NULL)
    {
      return FALSE;
    }

    Status = NtTerminateProcess(hProcess, uExitCode);
    if (NT_SUCCESS(Status))
    {
        return TRUE;
    }

    SetLastErrorByStatus(Status);
    return FALSE;
}


/*
 * @unimplemented
 */
VOID
WINAPI
FatalAppExitA(UINT uAction,
              LPCSTR lpMessageText)
{
    UNICODE_STRING MessageTextU;
    ANSI_STRING MessageText;

    RtlInitAnsiString(&MessageText, (LPSTR)lpMessageText);

    RtlAnsiStringToUnicodeString(&MessageTextU,
                                 &MessageText,
                                 TRUE);

    FatalAppExitW(uAction, MessageTextU.Buffer);

    RtlFreeUnicodeString(&MessageTextU);
}


/*
 * @unimplemented
 */
VOID
WINAPI
FatalAppExitW(UINT uAction,
              LPCWSTR lpMessageText)
{
    static const WCHAR szUser32[] = L"user32.dll\0";

    HMODULE hModule = GetModuleHandleW(szUser32);
    MessageBoxW_Proc pMessageBoxW = NULL;

    DPRINT1("AppExit\n");

    if (hModule)
        pMessageBoxW = (MessageBoxW_Proc)GetProcAddress(hModule, "MessageBoxW");

    if (pMessageBoxW)
        pMessageBoxW(0, lpMessageText, NULL, MB_SYSTEMMODAL | MB_OK);
    else
        DPRINT1("%s\n", lpMessageText);

    ExitProcess(0);
}


/*
 * @implemented
 */
VOID
WINAPI
FatalExit(int ExitCode)
{
    ExitProcess(ExitCode);
}


/*
 * @implemented
 */
DWORD
WINAPI
GetPriorityClass(HANDLE hProcess)
{
  NTSTATUS Status;
  PROCESS_PRIORITY_CLASS PriorityClass;

  Status = NtQueryInformationProcess(hProcess,
                                     ProcessPriorityClass,
                                     &PriorityClass,
                                     sizeof(PROCESS_PRIORITY_CLASS),
                                     NULL);
  if(NT_SUCCESS(Status))
  {
    switch(PriorityClass.PriorityClass)
    {
      case PROCESS_PRIORITY_CLASS_IDLE:
        return IDLE_PRIORITY_CLASS;

      case PROCESS_PRIORITY_CLASS_BELOW_NORMAL:
        return BELOW_NORMAL_PRIORITY_CLASS;

      case PROCESS_PRIORITY_CLASS_NORMAL:
        return NORMAL_PRIORITY_CLASS;

      case PROCESS_PRIORITY_CLASS_ABOVE_NORMAL:
        return ABOVE_NORMAL_PRIORITY_CLASS;

      case PROCESS_PRIORITY_CLASS_HIGH:
        return HIGH_PRIORITY_CLASS;

      case PROCESS_PRIORITY_CLASS_REALTIME:
        return REALTIME_PRIORITY_CLASS;

      default:
        return NORMAL_PRIORITY_CLASS;
    }
  }

  SetLastErrorByStatus(Status);
  return FALSE;
}


/*
 * @implemented
 */
BOOL
WINAPI
SetPriorityClass(HANDLE hProcess,
                 DWORD dwPriorityClass)
{
    NTSTATUS Status;
    PROCESS_PRIORITY_CLASS PriorityClass;

    switch (dwPriorityClass)
    {
        case IDLE_PRIORITY_CLASS:
            PriorityClass.PriorityClass = PROCESS_PRIORITY_CLASS_IDLE;
            break;

        case BELOW_NORMAL_PRIORITY_CLASS:
            PriorityClass.PriorityClass = PROCESS_PRIORITY_CLASS_BELOW_NORMAL;
            break;

        case NORMAL_PRIORITY_CLASS:
            PriorityClass.PriorityClass = PROCESS_PRIORITY_CLASS_NORMAL;
            break;

        case ABOVE_NORMAL_PRIORITY_CLASS:
            PriorityClass.PriorityClass = PROCESS_PRIORITY_CLASS_ABOVE_NORMAL;
            break;

        case HIGH_PRIORITY_CLASS:
            PriorityClass.PriorityClass = PROCESS_PRIORITY_CLASS_HIGH;
            break;

        case REALTIME_PRIORITY_CLASS:
            PriorityClass.PriorityClass = PROCESS_PRIORITY_CLASS_REALTIME;
            break;

        default:
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
    }

    PriorityClass.Foreground = FALSE;

    Status = NtSetInformationProcess(hProcess,
                                     ProcessPriorityClass,
                                     &PriorityClass,
                                     sizeof(PROCESS_PRIORITY_CLASS));
    if (!NT_SUCCESS(Status))
    {
        SetLastErrorByStatus(Status);
        return FALSE;
    }

    return TRUE;
}


/*
 * @implemented
 */
DWORD
WINAPI
GetProcessVersion(DWORD ProcessId)
{
    DWORD Version = 0;
    PIMAGE_NT_HEADERS NtHeader = NULL;
    IMAGE_NT_HEADERS NtHeaders;
    IMAGE_DOS_HEADER DosHeader;
    PROCESS_BASIC_INFORMATION ProcessBasicInfo;
    PVOID BaseAddress = NULL;
    HANDLE ProcessHandle = NULL;
    NTSTATUS Status;
    SIZE_T Count;
    PEB Peb;

    _SEH2_TRY
    {
        if (0 == ProcessId || GetCurrentProcessId() == ProcessId)
        {
            /* Caller's */
            BaseAddress = (PVOID) NtCurrentPeb()->ImageBaseAddress;
            NtHeader = RtlImageNtHeader(BaseAddress);

            Version = (NtHeader->OptionalHeader.MajorOperatingSystemVersion << 16) |
                      (NtHeader->OptionalHeader.MinorOperatingSystemVersion);
        }
        else
        {
            /* Other process */
            ProcessHandle = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION,
                                        FALSE,
                                        ProcessId);

            if (!ProcessHandle) return 0;

            Status = NtQueryInformationProcess(ProcessHandle,
                                               ProcessBasicInformation,
                                               &ProcessBasicInfo,
                                               sizeof(ProcessBasicInfo),
                                               NULL);

            if (!NT_SUCCESS(Status)) goto Error;

            Status = NtReadVirtualMemory(ProcessHandle,
                                         ProcessBasicInfo.PebBaseAddress,
                                         &Peb,
                                         sizeof(Peb),
                                         &Count);

            if (!NT_SUCCESS(Status) || Count != sizeof(Peb)) goto Error;

            memset(&DosHeader, 0, sizeof(DosHeader));
            Status = NtReadVirtualMemory(ProcessHandle,
                                         Peb.ImageBaseAddress,
                                         &DosHeader,
                                         sizeof(DosHeader),
                                         &Count);

            if (!NT_SUCCESS(Status) || Count != sizeof(DosHeader)) goto Error;
            if (DosHeader.e_magic != IMAGE_DOS_SIGNATURE) goto Error;

            memset(&NtHeaders, 0, sizeof(NtHeaders));
            Status = NtReadVirtualMemory(ProcessHandle,
                                         (char *)Peb.ImageBaseAddress + DosHeader.e_lfanew,
                                         &NtHeaders,
                                         sizeof(NtHeaders),
                                         &Count);

            if (!NT_SUCCESS(Status) || Count != sizeof(NtHeaders)) goto Error;
            if (NtHeaders.Signature != IMAGE_NT_SIGNATURE) goto Error;

            Version = MAKELONG(NtHeaders.OptionalHeader.MinorSubsystemVersion,
                               NtHeaders.OptionalHeader.MajorSubsystemVersion);

Error:
            if (!NT_SUCCESS(Status))
            {
                SetLastErrorByStatus(Status);
            }
        }
    }
    _SEH2_FINALLY
    {
        if (ProcessHandle) CloseHandle(ProcessHandle);
    }
    _SEH2_END;

    return Version;
}


/*
 * @implemented
 */
BOOL
WINAPI
GetProcessIoCounters(HANDLE hProcess,
                     PIO_COUNTERS lpIoCounters)
{
    NTSTATUS Status;

    Status = NtQueryInformationProcess(hProcess,
                                       ProcessIoCounters,
                                       lpIoCounters,
                                       sizeof(IO_COUNTERS),
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        SetLastErrorByStatus(Status);
        return FALSE;
    }

    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
GetProcessPriorityBoost(HANDLE hProcess,
                        PBOOL pDisablePriorityBoost)
{
    NTSTATUS Status;
    ULONG PriorityBoost;

    Status = NtQueryInformationProcess(hProcess,
                                       ProcessPriorityBoost,
                                       &PriorityBoost,
                                       sizeof(ULONG),
                                       NULL);
    if (NT_SUCCESS(Status))
    {
        *pDisablePriorityBoost = PriorityBoost;
        return TRUE;
    }

    SetLastErrorByStatus(Status);
    return FALSE;
}


/*
 * @implemented
 */
BOOL
WINAPI
SetProcessPriorityBoost(HANDLE hProcess,
                        BOOL bDisablePriorityBoost)
{
    NTSTATUS Status;
    ULONG PriorityBoost = (bDisablePriorityBoost ? TRUE : FALSE); /* prevent setting values other than 1 and 0 */

    Status = NtSetInformationProcess(hProcess,
                                     ProcessPriorityBoost,
                                     &PriorityBoost,
                                     sizeof(ULONG));
    if (!NT_SUCCESS(Status))
    {
        SetLastErrorByStatus(Status);
        return FALSE;
    }

    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
GetProcessHandleCount(HANDLE hProcess,
                      PDWORD pdwHandleCount)
{
    ULONG phc;
    NTSTATUS Status;

    Status = NtQueryInformationProcess(hProcess,
                                       ProcessHandleCount,
                                       &phc,
                                       sizeof(ULONG),
                                       NULL);
    if(NT_SUCCESS(Status))
    {
      *pdwHandleCount = phc;
      return TRUE;
    }

    SetLastErrorByStatus(Status);
    return FALSE;
}


/*
 * @implemented
 */
BOOL
WINAPI
IsWow64Process(HANDLE hProcess,
               PBOOL Wow64Process)
{
    ULONG pbi;
    NTSTATUS Status;

    Status = NtQueryInformationProcess(hProcess,
                                       ProcessWow64Information,
                                       &pbi,
                                       sizeof(pbi),
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    *Wow64Process = (pbi != 0);

    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
QueryFullProcessImageNameW(HANDLE hProcess,
                           DWORD dwFlags,
                           LPWSTR lpExeName,
                           PDWORD pdwSize)
{
    BYTE Buffer[sizeof(UNICODE_STRING) + MAX_PATH * sizeof(WCHAR)];
    UNICODE_STRING *DynamicBuffer = NULL;
    UNICODE_STRING *Result = NULL;
    NTSTATUS Status;
    DWORD Needed;

    Status = NtQueryInformationProcess(hProcess,
                                       ProcessImageFileName,
                                       Buffer,
                                       sizeof(Buffer) - sizeof(WCHAR),
                                       &Needed);
    if (Status == STATUS_INFO_LENGTH_MISMATCH)
    {
        DynamicBuffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, Needed + sizeof(WCHAR));
        if (!DynamicBuffer)
        {
            SetLastErrorByStatus(STATUS_NO_MEMORY);
            return FALSE;
        }

        Status = NtQueryInformationProcess(hProcess,
                                           ProcessImageFileName,
                                           (LPBYTE)DynamicBuffer,
                                           Needed,
                                           &Needed);
        Result = DynamicBuffer;
    }
    else Result = (PUNICODE_STRING)Buffer;

    if (!NT_SUCCESS(Status)) goto Cleanup;

    if (Result->Length / sizeof(WCHAR) + 1 > *pdwSize)
    {
        Status = STATUS_BUFFER_TOO_SMALL;
        goto Cleanup;
    }

    *pdwSize = Result->Length / sizeof(WCHAR);
    memcpy(lpExeName, Result->Buffer, Result->Length);
    lpExeName[*pdwSize] = 0;

Cleanup:
    RtlFreeHeap(RtlGetProcessHeap(), 0, DynamicBuffer);

    if (!NT_SUCCESS(Status))
    {
        SetLastErrorByStatus(Status);
    }

    return !Status;
}


/*
 * @implemented
 */
BOOL
WINAPI
QueryFullProcessImageNameA(HANDLE hProcess,
                           DWORD dwFlags,
                           LPSTR lpExeName,
                           PDWORD pdwSize)
{
    DWORD pdwSizeW = *pdwSize;
    BOOL Result;
    LPWSTR lpExeNameW;

    lpExeNameW = RtlAllocateHeap(RtlGetProcessHeap(),
                                 HEAP_ZERO_MEMORY,
                                 *pdwSize * sizeof(WCHAR));
    if (!lpExeNameW)
    {
        SetLastErrorByStatus(STATUS_NO_MEMORY);
        return FALSE;
    }

    Result = QueryFullProcessImageNameW(hProcess, dwFlags, lpExeNameW, &pdwSizeW);

    if (Result)
        Result = (0 != WideCharToMultiByte(CP_ACP, 0,
                                           lpExeNameW,
                                           -1,
                                           lpExeName,
                                           *pdwSize,
                                           NULL, NULL));

    if (Result)
        *pdwSize = strlen(lpExeName);

    RtlFreeHeap(RtlGetProcessHeap(), 0, lpExeNameW);
    return Result;
}

/* EOF */
