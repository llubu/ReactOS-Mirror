/* $Id: proc.c,v 1.67 2004/09/21 21:53:45 weiden Exp $
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
#include "../include/debug.h"


/* GLOBALS *******************************************************************/

WaitForInputIdleType  lpfnGlobalRegisterWaitForInputIdle;

LPSTARTUPINFOA lpLocalStartupInfo = NULL;

VOID STDCALL
RegisterWaitForInputIdle(WaitForInputIdleType lpfnRegisterWaitForInputIdle);

BOOL STDCALL
InternalGetProcessId (HANDLE hProcess, LPDWORD lpProcessId);


/* FUNCTIONS ****************************************************************/

/*
 * @implemented
 */
BOOL STDCALL
GetProcessAffinityMask (HANDLE hProcess,
			LPDWORD lpProcessAffinityMask,
			LPDWORD lpSystemAffinityMask)
{
  PROCESS_BASIC_INFORMATION ProcessInfo;
  ULONG BytesWritten;
  NTSTATUS Status;

  Status = NtQueryInformationProcess (hProcess,
				      ProcessBasicInformation,
				      (PVOID)&ProcessInfo,
				      sizeof(PROCESS_BASIC_INFORMATION),
				      &BytesWritten);
  if (!NT_SUCCESS(Status))
    {
      SetLastErrorByStatus (Status);
      return FALSE;
    }

  *lpProcessAffinityMask = (DWORD)ProcessInfo.AffinityMask;

  /* FIXME */
  *lpSystemAffinityMask  = 0x00000001;

  return TRUE;
}


/*
 * @implemented
 */
BOOL STDCALL
SetProcessAffinityMask (HANDLE hProcess,
			DWORD dwProcessAffinityMask)
{
  NTSTATUS Status;

  Status = NtSetInformationProcess (hProcess,
				    ProcessAffinityMask,
				    (PVOID)&dwProcessAffinityMask,
				    sizeof(DWORD));
  if (!NT_SUCCESS(Status))
    {
      SetLastErrorByStatus (Status);
      return FALSE;
    }

  return TRUE;
}


/*
 * @implemented
 */
BOOL STDCALL
GetProcessShutdownParameters (LPDWORD lpdwLevel,
			      LPDWORD lpdwFlags)
{
  CSRSS_API_REQUEST CsrRequest;
  CSRSS_API_REPLY CsrReply;
  NTSTATUS Status;

  CsrRequest.Type = CSRSS_GET_SHUTDOWN_PARAMETERS;
  Status = CsrClientCallServer(&CsrRequest,
			       &CsrReply,
			       sizeof(CSRSS_API_REQUEST),
			       sizeof(CSRSS_API_REPLY));
  if (!NT_SUCCESS(Status) || !NT_SUCCESS(CsrReply.Status))
    {
      SetLastErrorByStatus (Status);
      return(FALSE);
    }

  *lpdwLevel = CsrReply.Data.GetShutdownParametersReply.Level;
  *lpdwFlags = CsrReply.Data.GetShutdownParametersReply.Flags;

  return(TRUE);
}


/*
 * @implemented
 */
BOOL STDCALL
SetProcessShutdownParameters (DWORD dwLevel,
			      DWORD dwFlags)
{
  CSRSS_API_REQUEST CsrRequest;
  CSRSS_API_REPLY CsrReply;
  NTSTATUS Status;

  CsrRequest.Data.SetShutdownParametersRequest.Level = dwLevel;
  CsrRequest.Data.SetShutdownParametersRequest.Flags = dwFlags;

  CsrRequest.Type = CSRSS_SET_SHUTDOWN_PARAMETERS;
  Status = CsrClientCallServer(&CsrRequest,
			       &CsrReply,
			       sizeof(CSRSS_API_REQUEST),
			       sizeof(CSRSS_API_REPLY));
  if (!NT_SUCCESS(Status) || !NT_SUCCESS(CsrReply.Status))
    {
      SetLastErrorByStatus (Status);
      return(FALSE);
    }

  return(TRUE);
}


/*
 * @implemented
 */
BOOL STDCALL
GetProcessWorkingSetSize (HANDLE hProcess,
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
      return(FALSE);
    }

  *lpMinimumWorkingSetSize = QuotaLimits.MinimumWorkingSetSize;
  *lpMaximumWorkingSetSize = QuotaLimits.MaximumWorkingSetSize;

  return(TRUE);
}


/*
 * @implemented
 */
BOOL STDCALL
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
      return(FALSE);
    }

  return(TRUE);
}


/*
 * @implemented
 */
BOOL STDCALL
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
      return(FALSE);
    }

  lpCreationTime->dwLowDateTime = Kut.CreateTime.u.LowPart;
  lpCreationTime->dwHighDateTime = Kut.CreateTime.u.HighPart;

  lpExitTime->dwLowDateTime = Kut.ExitTime.u.LowPart;
  lpExitTime->dwHighDateTime = Kut.ExitTime.u.HighPart;

  lpKernelTime->dwLowDateTime = Kut.KernelTime.u.LowPart;
  lpKernelTime->dwHighDateTime = Kut.KernelTime.u.HighPart;

  lpUserTime->dwLowDateTime = Kut.UserTime.u.LowPart;
  lpUserTime->dwHighDateTime = Kut.UserTime.u.HighPart;

  return(TRUE);
}


/*
 * @implemented
 */
HANDLE STDCALL
GetCurrentProcess(VOID)
{
  return((HANDLE)NtCurrentProcess());
}


/*
 * @implemented
 */
HANDLE STDCALL
GetCurrentThread(VOID)
{
  return((HANDLE)NtCurrentThread());
}


/*
 * @implemented
 */
DWORD STDCALL
GetCurrentProcessId(VOID)
{
  return((DWORD)GetTeb()->Cid.UniqueProcess);
}


/*
 * @implemented
 */
BOOL STDCALL
GetExitCodeProcess(HANDLE hProcess,
		   LPDWORD lpExitCode)
{
  PROCESS_BASIC_INFORMATION ProcessBasic;
  ULONG BytesWritten;
  NTSTATUS Status;

  Status = NtQueryInformationProcess(hProcess,
				     ProcessBasicInformation,
				     &ProcessBasic,
				     sizeof(PROCESS_BASIC_INFORMATION),
				     &BytesWritten);
  if (!NT_SUCCESS(Status))
    {
      SetLastErrorByStatus(Status);
      return(FALSE);
     }

  memcpy(lpExitCode, &ProcessBasic.ExitStatus, sizeof(DWORD));

  return(TRUE);
}


/*
 * @implemented
 */
BOOL STDCALL
InternalGetProcessId(HANDLE hProcess,
	     LPDWORD lpProcessId)
{
  PROCESS_BASIC_INFORMATION ProcessBasic;
  ULONG BytesWritten;
  NTSTATUS Status;

  Status = NtQueryInformationProcess(hProcess,
				     ProcessBasicInformation,
				     &ProcessBasic,
				     sizeof(PROCESS_BASIC_INFORMATION),
				     &BytesWritten);
  if (!NT_SUCCESS(Status))
    {
      SetLastErrorByStatus(Status);
      return(FALSE);
    }

  memcpy(lpProcessId, &ProcessBasic.UniqueProcessId, sizeof(DWORD));

  return(TRUE);
}


/*
 * @implemented
 */
DWORD
STDCALL
GetProcessId(HANDLE Process)
{
  DWORD ProcessId;
  
  if(!InternalGetProcessId(Process, &ProcessId))
  {
    return 0;
  }
  return ProcessId;
}


/*
 * @implemented
 */
HANDLE STDCALL
OpenProcess(DWORD dwDesiredAccess,
	    BOOL bInheritHandle,
	    DWORD dwProcessId)
{
   NTSTATUS errCode;
   HANDLE ProcessHandle;
   OBJECT_ATTRIBUTES ObjectAttributes;
   CLIENT_ID ClientId ;
   
   ClientId.UniqueProcess = (HANDLE)dwProcessId;
   ClientId.UniqueThread = INVALID_HANDLE_VALUE;
   
   ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
   ObjectAttributes.RootDirectory = (HANDLE)NULL;
   ObjectAttributes.SecurityDescriptor = NULL;
   ObjectAttributes.SecurityQualityOfService = NULL;
   ObjectAttributes.ObjectName = NULL;
   
   if (bInheritHandle == TRUE)
     ObjectAttributes.Attributes = OBJ_INHERIT;
   else
     ObjectAttributes.Attributes = 0;
   
   errCode = NtOpenProcess(&ProcessHandle,
			   dwDesiredAccess,
			   &ObjectAttributes,
			   &ClientId);
   if (!NT_SUCCESS(errCode))
     {
	SetLastErrorByStatus (errCode);
	return NULL;
     }
   return ProcessHandle;
}


/*
 * @implemented
 */
UINT STDCALL
WinExec(LPCSTR lpCmdLine,
	UINT uCmdShow)
{
   STARTUPINFOA StartupInfo;
   PROCESS_INFORMATION  ProcessInformation;
   DWORD dosErr;

   RtlZeroMemory(&StartupInfo, sizeof(StartupInfo));
   StartupInfo.cb = sizeof(STARTUPINFOA);
   StartupInfo.wShowWindow = uCmdShow;
   StartupInfo.dwFlags = 0;

   if (! CreateProcessA(NULL,
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
       lpfnGlobalRegisterWaitForInputIdle (
	  ProcessInformation.hProcess,
          10000
	);
     }
   NtClose(ProcessInformation.hProcess);
   NtClose(ProcessInformation.hThread);

   return 33; /* Something bigger than 31 means success. */
}


/*
 * @implemented
 */
VOID STDCALL
RegisterWaitForInputIdle (
	WaitForInputIdleType	lpfnRegisterWaitForInputIdle
	)
{
	lpfnGlobalRegisterWaitForInputIdle = lpfnRegisterWaitForInputIdle;
	return;
}


/*
 * @unimplemented
 */
DWORD STDCALL
WaitForInputIdle (
	HANDLE	hProcess,
	DWORD	dwMilliseconds
	)
{
	return 0;
}


/*
 * @implemented
 */
VOID STDCALL
Sleep(DWORD dwMilliseconds)
{
  SleepEx(dwMilliseconds, FALSE);
  return;
}


/*
 * @implemented
 */
DWORD STDCALL
SleepEx(DWORD dwMilliseconds,
	BOOL bAlertable)
{
  TIME Interval;
  NTSTATUS errCode;
  
  if (dwMilliseconds != INFINITE)
    {
      /*
       * System time units are 100 nanoseconds (a nanosecond is a billionth of
       * a second).
       */
      Interval.QuadPart = -((ULONGLONG)dwMilliseconds * 10000);
    }  
  else
    {
      /* Approximately 292000 years hence */
      Interval.QuadPart = -0x7FFFFFFFFFFFFFFFLL;
    }

  errCode = NtDelayExecution (bAlertable, &Interval);
  if (!NT_SUCCESS(errCode))
    {
      SetLastErrorByStatus (errCode);
      return -1;
    }
  return 0;
}


/*
 * @implemented
 */
VOID STDCALL
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
  lpStartupInfo->dwX = Params->dwX;
  lpStartupInfo->dwY = Params->dwY;
  lpStartupInfo->dwXSize = Params->dwXSize;
  lpStartupInfo->dwYSize = Params->dwYSize;
  lpStartupInfo->dwXCountChars = Params->dwXCountChars;
  lpStartupInfo->dwYCountChars = Params->dwYCountChars;
  lpStartupInfo->dwFillAttribute = Params->dwFillAttribute;
  lpStartupInfo->dwFlags = Params->dwFlags;
  lpStartupInfo->wShowWindow = Params->wShowWindow;
  lpStartupInfo->lpReserved = Params->ShellInfo.Buffer;
  lpStartupInfo->cbReserved2 = Params->RuntimeInfo.Length;
  lpStartupInfo->lpReserved2 = (LPBYTE)Params->RuntimeInfo.Buffer;

  lpStartupInfo->hStdInput = Params->hStdInput;
  lpStartupInfo->hStdOutput = Params->hStdOutput;
  lpStartupInfo->hStdError = Params->hStdError;
}


/*
 * @implemented
 */
VOID STDCALL
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

  if (lpLocalStartupInfo == NULL)
    {
	/* create new local startup info (ansi) */
	lpLocalStartupInfo = RtlAllocateHeap (RtlGetProcessHeap (),
	                                      0,
	                                      sizeof(STARTUPINFOA));

	lpLocalStartupInfo->cb = sizeof(STARTUPINFOA);

	/* copy window title string */
	RtlUnicodeStringToAnsiString (&AnsiString,
	                              &Params->WindowTitle,
	                              TRUE);
	lpLocalStartupInfo->lpTitle = AnsiString.Buffer;

	/* copy desktop info string */
	RtlUnicodeStringToAnsiString (&AnsiString,
	                              &Params->DesktopInfo,
	                              TRUE);
	lpLocalStartupInfo->lpDesktop = AnsiString.Buffer;

	/* copy shell info string */
	RtlUnicodeStringToAnsiString (&AnsiString,
	                              &Params->ShellInfo,
	                              TRUE);
	lpLocalStartupInfo->lpReserved = AnsiString.Buffer;

	lpLocalStartupInfo->dwX = Params->dwX;
	lpLocalStartupInfo->dwY = Params->dwY;
	lpLocalStartupInfo->dwXSize = Params->dwXSize;
	lpLocalStartupInfo->dwYSize = Params->dwYSize;
	lpLocalStartupInfo->dwXCountChars = Params->dwXCountChars;
	lpLocalStartupInfo->dwYCountChars = Params->dwYCountChars;
	lpLocalStartupInfo->dwFillAttribute = Params->dwFillAttribute;
	lpLocalStartupInfo->dwFlags = Params->dwFlags;
	lpLocalStartupInfo->wShowWindow = Params->wShowWindow;
	lpLocalStartupInfo->cbReserved2 = Params->RuntimeInfo.Length;
	lpLocalStartupInfo->lpReserved2 = (LPBYTE)Params->RuntimeInfo.Buffer;

	lpLocalStartupInfo->hStdInput = Params->hStdInput;
	lpLocalStartupInfo->hStdOutput = Params->hStdOutput;
	lpLocalStartupInfo->hStdError = Params->hStdError;
     }

   RtlReleasePebLock ();

   /* copy local startup info data to external startup info */
   memcpy (lpStartupInfo,
           lpLocalStartupInfo,
           sizeof(STARTUPINFOA));
}


/*
 * @implemented
 */
BOOL STDCALL
FlushInstructionCache (HANDLE	hProcess,
		       LPCVOID	lpBaseAddress,
		       DWORD	dwSize)
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
VOID STDCALL
ExitProcess(UINT uExitCode)
{
  CSRSS_API_REQUEST CsrRequest;
  CSRSS_API_REPLY CsrReply;
  NTSTATUS Status;
  
  /* unload all dll's */
  LdrShutdownProcess ();

  /* notify csrss of process termination */
  CsrRequest.Type = CSRSS_TERMINATE_PROCESS;
  Status = CsrClientCallServer(&CsrRequest, 
			       &CsrReply,
			       sizeof(CSRSS_API_REQUEST),
			       sizeof(CSRSS_API_REPLY));
  if (!NT_SUCCESS(Status) || !NT_SUCCESS(CsrReply.Status))
    {
      DPRINT("Failed to tell csrss about terminating process\n");
    }
  
  
  NtTerminateProcess (NtCurrentProcess (),
		      uExitCode);

  /* should never get here */
  assert(0);
  while(1);
}


/*
 * @implemented
 */
BOOL STDCALL
TerminateProcess (HANDLE	hProcess,
		  UINT	uExitCode)
{
  NTSTATUS Status;

  Status = NtTerminateProcess (hProcess, uExitCode);
  if (NT_SUCCESS(Status))
    {
      return TRUE;
    }
  SetLastErrorByStatus (Status);
  return FALSE;
}


/*
 * @unimplemented
 */
VOID STDCALL
FatalAppExitA (UINT	uAction,
	       LPCSTR	lpMessageText)
{
  UNICODE_STRING MessageTextU;
  ANSI_STRING MessageText;
  
  RtlInitAnsiString (&MessageText, (LPSTR) lpMessageText);

  RtlAnsiStringToUnicodeString (&MessageTextU,
				&MessageText,
				TRUE);

  FatalAppExitW (uAction, MessageTextU.Buffer);

  RtlFreeUnicodeString (&MessageTextU);
}


/*
 * @unimplemented
 */
VOID STDCALL
FatalAppExitW(UINT uAction,
	      LPCWSTR lpMessageText)
{
  return;
}


/*
 * @implemented
 */
VOID STDCALL
FatalExit (int ExitCode)
{
  ExitProcess(ExitCode);
}


/*
 * @implemented
 */
DWORD STDCALL
GetPriorityClass (HANDLE hProcess)
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
BOOL STDCALL
SetPriorityClass (HANDLE hProcess,
		  DWORD	dwPriorityClass)
{
  NTSTATUS Status;
  PROCESS_PRIORITY_CLASS PriorityClass;
  
  switch(dwPriorityClass)
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

  if(!NT_SUCCESS(Status))
  {
    SetLastErrorByStatus(Status);
    return FALSE;
  }
  
  return TRUE;
}


/*
 * @implemented
 */
DWORD STDCALL
GetProcessVersion (DWORD ProcessId)
{
  DWORD			Version = 0;
  PIMAGE_NT_HEADERS	NtHeader = NULL;
  PVOID			BaseAddress = NULL;

  /* Caller's */
  if (0 == ProcessId || GetCurrentProcessId() == ProcessId)
    {
      BaseAddress = (PVOID) NtCurrentPeb()->ImageBaseAddress;
      NtHeader = RtlImageNtHeader (BaseAddress);
      if (NULL != NtHeader)
	{
	  Version =
	    (NtHeader->OptionalHeader.MajorOperatingSystemVersion << 16) | 
	    (NtHeader->OptionalHeader.MinorOperatingSystemVersion);
	}
    }
  else /* other process */
    {
      /* FIXME: open the other process */
      SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    }
  return (Version);
}


/*
 * @implemented
 */
BOOL
STDCALL
GetProcessIoCounters(
  HANDLE hProcess,
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
      return(FALSE);
    }
  
  return TRUE;
}


/*
 * @implemented
 */
BOOL
STDCALL
GetProcessPriorityBoost(HANDLE hProcess,
                        PBOOL pDisablePriorityBoost)
{
  NTSTATUS Status;
  BOOL PriorityBoost;

  Status = NtQueryInformationProcess(hProcess,
				     ProcessPriorityBoost,
				     &PriorityBoost,
				     sizeof(BOOL),
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
STDCALL
SetProcessPriorityBoost(HANDLE hProcess,
                        BOOL bDisablePriorityBoost)
{
  NTSTATUS Status;
  BOOL PriorityBoost = (bDisablePriorityBoost ? TRUE : FALSE); /* prevent setting values other than 1 and 0 */

  Status = NtSetInformationProcess(hProcess,
				   ProcessPriorityBoost,
				   &PriorityBoost,
				   sizeof(BOOL));
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
STDCALL
GetProcessHandleCount(HANDLE hProcess,
                      PDWORD pdwHandleCount)
{
    ULONG phc, BytesWritten;
    NTSTATUS Status;

    Status = NtQueryInformationProcess(hProcess,
                                       ProcessHandleCount,
                                       &phc,
                                       sizeof(ULONG),
                                       &BytesWritten);
    if(NT_SUCCESS(Status))
    {
      *pdwHandleCount = ProcessHandleCount;
      return TRUE;
    }

    SetLastErrorByStatus(Status);
    return FALSE;
}

/* EOF */
