/* $Id: console.c,v 1.75 2004/05/28 13:17:32 weiden Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/misc/console.c
 * PURPOSE:         Win32 server console functions
 * PROGRAMMER:      James Tabor 
 *			<jimtabor@adsl-64-217-116-74.dsl.hstntx.swbell.net>
 * UPDATE HISTORY:
 *	199901?? ??	Created
 *	19990204 EA	SetConsoleTitleA
 *      19990306 EA	Stubs
 */

/* INCLUDES ******************************************************************/

#include <k32.h>

#define NDEBUG
#include "../include/debug.h"

#define _NOACHS(__X) (sizeof(__X) / sizeof((__X)[0]))
extern BOOL WINAPI DefaultConsoleCtrlHandler(DWORD Event);
extern __declspec(noreturn) VOID CALLBACK ConsoleControlDispatcher(DWORD CodeAndFlag);
extern CRITICAL_SECTION ConsoleLock;
extern BOOL WINAPI IsDebuggerPresent(VOID);


/* GLOBALS *******************************************************************/

static BOOL IgnoreCtrlEvents = FALSE;

static PHANDLER_ROUTINE* CtrlHandlers = NULL;
static ULONG NrCtrlHandlers = 0;

/* Default Console Control Handler *******************************************/

BOOL WINAPI DefaultConsoleCtrlHandler(DWORD Event)
{
	switch(Event)
	{
	case CTRL_C_EVENT:
		DPRINT("Ctrl-C Event\n");
		ExitProcess(0);
		break;
		
	case CTRL_BREAK_EVENT:
		DPRINT("Ctrl-Break Event\n");
		ExitProcess(0);
		break;

	case CTRL_SHUTDOWN_EVENT:
		DPRINT("Ctrl Shutdown Event\n");
		break;

	case CTRL_CLOSE_EVENT:
		DPRINT("Ctrl Close Event\n");
		break;

	case CTRL_LOGOFF_EVENT:		
		DPRINT("Ctrl Logoff Event\n");
		break;
	}
//	ExitProcess((UINT)&ExitCode);
	return TRUE;
}


__declspec(noreturn) VOID CALLBACK ConsoleControlDispatcher(DWORD CodeAndFlag)
{
DWORD nExitCode = 0;
DWORD nCode = CodeAndFlag & MAXLONG;
UINT i;

SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

	switch(nCode)
	{
	case CTRL_C_EVENT:
	case CTRL_BREAK_EVENT:
	{
		if(IsDebuggerPresent())
		{
			EXCEPTION_RECORD erException;
			erException.ExceptionCode = 
			(nCode == CTRL_C_EVENT ? DBG_CONTROL_C : DBG_CONTROL_BREAK);
			erException.ExceptionFlags = 0;
			erException.ExceptionRecord = NULL;
			erException.ExceptionAddress = &DefaultConsoleCtrlHandler;
			erException.NumberParameters = 0;
			RtlRaiseException(&erException);
		}		
		RtlEnterCriticalSection(&ConsoleLock);

		if(!(nCode == CTRL_C_EVENT &&
			NtCurrentPeb()->ProcessParameters->ProcessGroup & 1))
		{
			for(i = NrCtrlHandlers; i > 0; -- i)
				if(CtrlHandlers[i - 1](nCode)) break;
		}
		RtlLeaveCriticalSection(&ConsoleLock);
		ExitThread(0);
	}
	case CTRL_CLOSE_EVENT:
	case CTRL_LOGOFF_EVENT:
	case CTRL_SHUTDOWN_EVENT:
		break;

	default: ExitThread(0);
	}

	RtlEnterCriticalSection(&ConsoleLock);

	if(!(nCode == CTRL_C_EVENT &&
		NtCurrentPeb()->ProcessParameters->ProcessGroup & 1))
	{
	i = NrCtrlHandlers;
	while(i > 0)
		{
		if (i == 1 && (CodeAndFlag & MINLONG) && 
			(nCode == CTRL_LOGOFF_EVENT || nCode == CTRL_SHUTDOWN_EVENT))
				break;

			if(CtrlHandlers[i - 1](nCode))
			{
				switch(nCode)
				{
					case CTRL_CLOSE_EVENT:
					case CTRL_LOGOFF_EVENT:
					case CTRL_SHUTDOWN_EVENT:
						nExitCode = CodeAndFlag;
				}
				break;
			}
			--i;
		}
	}
	RtlLeaveCriticalSection(&ConsoleLock);
	ExitThread(nExitCode);
}


/* FUNCTIONS *****************************************************************/

/*
 * @unimplemented
 */
BOOL STDCALL
AddConsoleAliasA (LPSTR Source,
		  LPSTR Target,
		  LPSTR ExeName)
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL STDCALL
AddConsoleAliasW (LPWSTR Source,
		  LPWSTR Target,
		  LPWSTR ExeName)
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL STDCALL
ConsoleMenuControl (HANDLE	hConsole,
		    DWORD	Unknown1,
		    DWORD	Unknown2)
     /*
      * Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}


/*
 * @implemented
 */
HANDLE STDCALL
DuplicateConsoleHandle (HANDLE	hConsole,
			DWORD   dwDesiredAccess,
			BOOL	bInheritHandle,
			DWORD	dwOptions)
{
  CSRSS_API_REQUEST Request;
  CSRSS_API_REPLY Reply;
  NTSTATUS Status;

  if (IsConsoleHandle (hConsole) == FALSE)
    {
      SetLastError (ERROR_INVALID_PARAMETER);
      return INVALID_HANDLE_VALUE;
    }
  
  Request.Type = CSRSS_DUPLICATE_HANDLE;
  Request.Data.DuplicateHandleRequest.Handle = hConsole;
  Request.Data.DuplicateHandleRequest.ProcessId = GetCurrentProcessId();
  Status = CsrClientCallServer(&Request,
			       &Reply,
			       sizeof(CSRSS_API_REQUEST),
			       sizeof(CSRSS_API_REPLY));
  if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status=Reply.Status))
    {
      SetLastErrorByStatus(Status);
      return INVALID_HANDLE_VALUE;
    }
  return Reply.Data.DuplicateHandleReply.Handle;
}


/*
 * @unimplemented
 */
DWORD STDCALL
ExpungeConsoleCommandHistoryW (DWORD	Unknown0)
     /*
      * Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}


/*
 * @unimplemented
 */
DWORD STDCALL
ExpungeConsoleCommandHistoryA (DWORD	Unknown0)
     /*
      * Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}


/*
 * @unimplemented
 */
DWORD STDCALL
GetConsoleAliasW (DWORD	Unknown0,
		  DWORD	Unknown1,
		  DWORD	Unknown2,
		  DWORD	Unknown3)
     /*
      * Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}


/*
 * @unimplemented
 */
DWORD STDCALL
GetConsoleAliasA (DWORD	Unknown0,
		  DWORD	Unknown1,
		  DWORD	Unknown2,
		  DWORD	Unknown3)
     /*
      * Undocumented
      */
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
DWORD STDCALL
GetConsoleAliasExesW (DWORD	Unknown0,
		      DWORD	Unknown1)
     /*
      * Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}


/*
 * @unimplemented
 */
DWORD STDCALL
GetConsoleAliasExesA (DWORD	Unknown0,
		      DWORD	Unknown1)
     /*
      * Undocumented
      */
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
DWORD STDCALL
GetConsoleAliasExesLengthA (VOID)
     /*
      * Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}


/*
 * @unimplemented
 */
DWORD STDCALL
GetConsoleAliasExesLengthW (VOID)
     /*
      * Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}


/*
 * @unimplemented
 */
DWORD STDCALL
GetConsoleAliasesW (DWORD	Unknown0,
		    DWORD	Unknown1,
		    DWORD	Unknown2)
     /*
      * Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}
 

/*
 * @unimplemented
 */
DWORD STDCALL
GetConsoleAliasesA (DWORD	Unknown0,
		    DWORD	Unknown1,
		    DWORD	Unknown2)
     /*
      * Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}


/*
 * @unimplemented
 */
DWORD STDCALL
GetConsoleAliasesLengthW (DWORD Unknown0)
     /*
      * Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}


/*
 * @unimplemented
 */
DWORD STDCALL
GetConsoleAliasesLengthA (DWORD Unknown0)
     /*
      * Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}


/*
 * @unimplemented
 */
DWORD STDCALL
GetConsoleCommandHistoryW (DWORD	Unknown0,
			   DWORD	Unknown1,
			   DWORD	Unknown2)
     /*
      * Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}


/*
 * @unimplemented
 */
DWORD STDCALL
GetConsoleCommandHistoryA (DWORD	Unknown0,
			   DWORD	Unknown1,
			   DWORD	Unknown2)
     /*
      * Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}


/*
 * @unimplemented
 */
DWORD STDCALL
GetConsoleCommandHistoryLengthW (DWORD	Unknown0)
     /*
      * Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}


/*
 * @unimplemented
 */
DWORD STDCALL
GetConsoleCommandHistoryLengthA (DWORD	Unknown0)
     /*
      * Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/*
 * @unimplemented
 */
DWORD STDCALL
GetConsoleDisplayMode (LPDWORD lpdwMode)
     /*
      * FUNCTION: Get the console display mode
      * ARGUMENTS:
      *      lpdwMode - Address of variable that receives the current value
      *                 of display mode
      * STATUS: Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}


/*
 * @unimplemented
 */
DWORD STDCALL
GetConsoleFontInfo (DWORD	Unknown0,
		    DWORD	Unknown1,
		    DWORD	Unknown2,
		    DWORD	Unknown3)
     /*
      * Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}


/*
 * @unimplemented
 */
DWORD STDCALL
GetConsoleFontSize(HANDLE hConsoleOutput,
		   DWORD nFont)
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}


/*
 * @implemented
 */
DWORD STDCALL
GetConsoleHardwareState (HANDLE	hConsole,
			 DWORD	Flags,
			 PDWORD	State)
     /*
      * Undocumented
      */
{
  CSRSS_API_REQUEST Request;
  CSRSS_API_REPLY   Reply;
  NTSTATUS          Status;

  Request.Type = CSRSS_SETGET_CONSOLE_HW_STATE;
  Request.Data.ConsoleHardwareStateRequest.ConsoleHandle = hConsole;
  Request.Data.ConsoleHardwareStateRequest.SetGet = CONSOLE_HARDWARE_STATE_GET;

  Status = CsrClientCallServer(& Request,
			       & Reply,
			       sizeof(CSRSS_API_REQUEST),
			       sizeof(CSRSS_API_REPLY));
  if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Reply.Status))
  {
    SetLastErrorByStatus(Status);
    return FALSE;
  }
  *State = Reply.Data.ConsoleHardwareStateReply.State;
  return TRUE;  
}


/*
 * @unimplemented
 */
DWORD STDCALL
GetConsoleInputWaitHandle (VOID)
     /*
      * Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}


/*
 * @unimplemented
 */
DWORD STDCALL
GetCurrentConsoleFont(HANDLE hConsoleOutput,
		      BOOL bMaximumWindow,
		      PCONSOLE_FONT_INFO lpConsoleCurrentFont)
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}


/*
 * @unimplemented
 */
ULONG STDCALL
GetNumberOfConsoleFonts (VOID)
     /*
      * Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 1; /* FIXME: call csrss.exe */
}


/*
 * @unimplemented
 */
DWORD STDCALL
InvalidateConsoleDIBits (DWORD	Unknown0,
			 DWORD	Unknown1)
     /*
      * Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}


/*
 * @unimplemented
 */
HANDLE STDCALL
OpenConsoleW (LPWSTR  wsName,
	      DWORD   dwDesiredAccess,
	      BOOL    bInheritHandle,
	      DWORD   dwCreationDistribution)
     /*
      * Undocumented
      */
{
  CSRSS_API_REQUEST Request;
  CSRSS_API_REPLY   Reply;
  PHANDLE           phConsole = NULL;
  NTSTATUS          Status = STATUS_SUCCESS;
  
  
  if(0 == _wcsicmp(wsName, L"CONIN$"))
  {
    Request.Type = CSRSS_GET_INPUT_HANDLE;
    phConsole = & Reply.Data.GetInputHandleReply.InputHandle;
  }
  else if (0 == _wcsicmp(wsName, L"CONOUT$"))
  {
    Request.Type = CSRSS_GET_OUTPUT_HANDLE;
    phConsole = & Reply.Data.GetOutputHandleReply.OutputHandle;
  }
  else
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return(INVALID_HANDLE_VALUE);
  }
  if ((GENERIC_READ|GENERIC_WRITE) != dwDesiredAccess)
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return(INVALID_HANDLE_VALUE);
  }
  if (OPEN_EXISTING != dwCreationDistribution)
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return(INVALID_HANDLE_VALUE);
  }
  Status = CsrClientCallServer(& Request,
			       & Reply,
			       sizeof(CSRSS_API_REQUEST),
			       sizeof(CSRSS_API_REPLY));
  if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Reply.Status))
  {
    SetLastErrorByStatus(Status);
    return INVALID_HANDLE_VALUE;
  }
  return(*phConsole);
}


/*
 * @unimplemented
 */
BOOL STDCALL
SetConsoleCommandHistoryMode (DWORD	dwMode)
     /*
      * Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL STDCALL
SetConsoleCursor (DWORD	Unknown0,
		  DWORD	Unknown1)
     /*
      * Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL STDCALL
SetConsoleDisplayMode (HANDLE hOut,
		       DWORD dwNewMode,
		       LPDWORD lpdwOldMode)
     /*
      * FUNCTION: Set the console display mode.
      * ARGUMENTS:
      *       hOut - Standard output handle.
      *       dwNewMode - New mode.
      *       lpdwOldMode - Address of a variable that receives the old mode.
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL STDCALL
SetConsoleFont (DWORD	Unknown0,
		DWORD	Unknown1)
     /*
      * Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}


/*
 * @implemented
 */
BOOL STDCALL
SetConsoleHardwareState (HANDLE	hConsole,
			 DWORD	Flags,
			 DWORD	State)
     /*
      * Undocumented
      */
{
  CSRSS_API_REQUEST Request;
  CSRSS_API_REPLY   Reply;
  NTSTATUS          Status;

  Request.Type = CSRSS_SETGET_CONSOLE_HW_STATE;
  Request.Data.ConsoleHardwareStateRequest.ConsoleHandle = hConsole;
  Request.Data.ConsoleHardwareStateRequest.SetGet = CONSOLE_HARDWARE_STATE_SET;
  Request.Data.ConsoleHardwareStateRequest.State = State;

  Status = CsrClientCallServer(& Request,
			       & Reply,
			       sizeof(CSRSS_API_REQUEST),
			       sizeof(CSRSS_API_REPLY));
  if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Reply.Status))
  {
    SetLastErrorByStatus(Status);
    return FALSE;
  }
  return TRUE;  
}


/*
 * @unimplemented
 */
BOOL STDCALL
SetConsoleKeyShortcuts (DWORD	Unknown0,
			DWORD	Unknown1,
			DWORD	Unknown2,
			DWORD	Unknown3)
     /*
      * Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL STDCALL
SetConsoleMaximumWindowSize (DWORD	Unknown0,
			     DWORD	Unknown1)
     /*
      * Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL STDCALL
SetConsoleMenuClose (DWORD	Unknown0)
     /*
      * Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL STDCALL
SetConsoleNumberOfCommandsA (DWORD	Unknown0,
			     DWORD	Unknown1)
     /*
      * Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL STDCALL
SetConsoleNumberOfCommandsW (DWORD	Unknown0,
			     DWORD	Unknown1)
     /*
      * Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL STDCALL
SetConsolePalette (DWORD	Unknown0,
		   DWORD	Unknown1,
		   DWORD	Unknown2)
     /*
      * Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL STDCALL
SetLastConsoleEventActive (VOID)
     /*
      * Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}


/*
 * @unimplemented
 */
DWORD STDCALL
ShowConsoleCursor (DWORD	Unknown0,
		   DWORD	Unknown1)
     /*
      * Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}


/*
 * FUNCTION: Checks whether the given handle is a valid console handle.
 * ARGUMENTS:
 *      Handle - Handle to be checked
 * RETURNS:
 *      TRUE: Handle is a valid console handle
 *      FALSE: Handle is not a valid console handle.
 * STATUS: Officially undocumented
 *
 * @implemented
 */
BOOL STDCALL
VerifyConsoleIoHandle(HANDLE Handle)
{
  CSRSS_API_REQUEST Request;
  CSRSS_API_REPLY Reply;
  NTSTATUS Status;

  Request.Type = CSRSS_VERIFY_HANDLE;
  Request.Data.VerifyHandleRequest.Handle = Handle;
  Status = CsrClientCallServer(&Request,
			       &Reply,
			       sizeof(CSRSS_API_REQUEST),
			       sizeof(CSRSS_API_REPLY));
  if (!NT_SUCCESS(Status))
    {
      SetLastErrorByStatus(Status);
      return FALSE;
    }

  return (BOOL)NT_SUCCESS(Reply.Status);
}


/*
 * @unimplemented
 */
DWORD STDCALL
WriteConsoleInputVDMA (DWORD	Unknown0,
		       DWORD	Unknown1,
		       DWORD	Unknown2,
		       DWORD	Unknown3)
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}


/*
 * @unimplemented
 */
DWORD STDCALL
WriteConsoleInputVDMW (DWORD	Unknown0,
		       DWORD	Unknown1,
		       DWORD	Unknown2,
		       DWORD	Unknown3)
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}


/*
 * @implemented
 */
BOOL STDCALL
CloseConsoleHandle(HANDLE Handle)
     /*
      * Undocumented
      */
{
  CSRSS_API_REQUEST Request;
  CSRSS_API_REPLY Reply;
  NTSTATUS Status;

  if (IsConsoleHandle (Handle) == FALSE)
    {
      SetLastError (ERROR_INVALID_PARAMETER);
      return FALSE;
    }

  Request.Type = CSRSS_CLOSE_HANDLE;
  Request.Data.CloseHandleRequest.Handle = Handle;
  Status = CsrClientCallServer(&Request,
			       &Reply,
			       sizeof(CSRSS_API_REQUEST),
			       sizeof(CSRSS_API_REPLY));
  if (!NT_SUCCESS(Status))
    {
       SetLastErrorByStatus(Status);
       return FALSE;
    }

  return TRUE;
}


/*
 * internal function
 */
BOOL STDCALL
IsConsoleHandle(HANDLE Handle)
{
  if ((((ULONG)Handle) & 0x10000003) == 0x3)
    {
      return(TRUE);
    }
  return(FALSE);
}


/*
 * @implemented
 */
HANDLE STDCALL
GetStdHandle(DWORD nStdHandle)
     /*
      * FUNCTION: Get a handle for the standard input, standard output
      * and a standard error device.
      * ARGUMENTS:
      *       nStdHandle - Specifies the device for which to return the handle.
      * RETURNS: If the function succeeds, the return value is the handle
      * of the specified device. Otherwise the value is INVALID_HANDLE_VALUE.
      */
{
  PRTL_USER_PROCESS_PARAMETERS Ppb;

  Ppb = NtCurrentPeb()->ProcessParameters;
  switch (nStdHandle)
    {
      case STD_INPUT_HANDLE:
	return Ppb->hStdInput;

      case STD_OUTPUT_HANDLE:
	return Ppb->hStdOutput;

      case STD_ERROR_HANDLE:
	return Ppb->hStdError;
    }

  SetLastError (ERROR_INVALID_PARAMETER);
  return INVALID_HANDLE_VALUE;
}


/*
 * @implemented
 */
WINBASEAPI BOOL WINAPI
SetStdHandle(DWORD nStdHandle,
	     HANDLE hHandle)
     /*
      * FUNCTION: Set the handle for the standard input, standard output or
      * the standard error device.
      * ARGUMENTS:
      *        nStdHandle - Specifies the handle to be set.
      *        hHandle - The handle to set.
      * RETURNS: TRUE if the function succeeds, FALSE otherwise.
      */
{
  PRTL_USER_PROCESS_PARAMETERS Ppb;

  /* no need to check if hHandle == INVALID_HANDLE_VALUE */

  Ppb = NtCurrentPeb()->ProcessParameters;

  switch (nStdHandle)
    {
      case STD_INPUT_HANDLE:
	Ppb->hStdInput = hHandle;
	return TRUE;

      case STD_OUTPUT_HANDLE:
	Ppb->hStdOutput = hHandle;
	return TRUE;

      case STD_ERROR_HANDLE:
	Ppb->hStdError = hHandle;
	return TRUE;
    }

  /* windows for whatever reason sets the last error to ERROR_INVALID_HANDLE here */
  SetLastError (ERROR_INVALID_HANDLE);
  return FALSE;
}


/*--------------------------------------------------------------
 *	WriteConsoleA
 *
 * @implemented
 */
BOOL STDCALL 
WriteConsoleA(HANDLE hConsoleOutput,
	      CONST VOID *lpBuffer,
	      DWORD nNumberOfCharsToWrite,
	      LPDWORD lpNumberOfCharsWritten,
	      LPVOID lpReserved)
{
  PCSRSS_API_REQUEST Request;
  CSRSS_API_REPLY Reply;
  NTSTATUS Status;
  USHORT Size;
  ULONG MessageSize;

  Request = RtlAllocateHeap(GetProcessHeap(),
			    HEAP_ZERO_MEMORY,
			    sizeof(CSRSS_API_REQUEST) + 
			    CSRSS_MAX_WRITE_CONSOLE_REQUEST);
  if (Request == NULL)
    {
      SetLastError(ERROR_OUTOFMEMORY);
      return(FALSE);
    }

  Request->Type = CSRSS_WRITE_CONSOLE;
  Request->Data.WriteConsoleRequest.ConsoleHandle = hConsoleOutput;
  if (lpNumberOfCharsWritten != NULL)
    *lpNumberOfCharsWritten = nNumberOfCharsToWrite;
  while (nNumberOfCharsToWrite)
    {
      if (nNumberOfCharsToWrite > CSRSS_MAX_WRITE_CONSOLE_REQUEST)
	{
	  Size = CSRSS_MAX_WRITE_CONSOLE_REQUEST;
	}
      else
	{
	  Size = nNumberOfCharsToWrite;
	}
      Request->Data.WriteConsoleRequest.NrCharactersToWrite = Size;

      memcpy(Request->Data.WriteConsoleRequest.Buffer, lpBuffer, Size);

      MessageSize = CSRSS_REQUEST_HEADER_SIZE + 
	sizeof(CSRSS_WRITE_CONSOLE_REQUEST) + Size;
      Status = CsrClientCallServer(Request,
				   &Reply,
				   MessageSize,
				   sizeof(CSRSS_API_REPLY));

      if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Reply.Status))
	{
	  RtlFreeHeap(GetProcessHeap(), 0, Request);
	  SetLastErrorByStatus(Status);
	  return(FALSE);
	}
      nNumberOfCharsToWrite -= Size;
      lpBuffer += Size;
    }

  RtlFreeHeap(GetProcessHeap(), 0, Request);

  return TRUE;
}


/*--------------------------------------------------------------
 *	ReadConsoleA
 *
 * @implemented
 */
BOOL STDCALL ReadConsoleA(HANDLE hConsoleInput,
			     LPVOID lpBuffer,
			     DWORD nNumberOfCharsToRead,
			     LPDWORD lpNumberOfCharsRead,
			     LPVOID lpReserved)
{
   CSRSS_API_REQUEST Request;
   PCSRSS_API_REPLY Reply;
   NTSTATUS Status;
   ULONG CharsRead = 0;
   
   Reply = RtlAllocateHeap(GetProcessHeap(),
		     HEAP_ZERO_MEMORY,
		     sizeof(CSRSS_API_REPLY) + nNumberOfCharsToRead);
   if (Reply == NULL)
     {
	SetLastError(ERROR_OUTOFMEMORY);
	return(FALSE);
     }
   
   Request.Type = CSRSS_READ_CONSOLE;
   Request.Data.ReadConsoleRequest.ConsoleHandle = hConsoleInput;
   Request.Data.ReadConsoleRequest.NrCharactersToRead = nNumberOfCharsToRead > CSRSS_MAX_READ_CONSOLE_REQUEST ? CSRSS_MAX_READ_CONSOLE_REQUEST : nNumberOfCharsToRead;
   Request.Data.ReadConsoleRequest.nCharsCanBeDeleted = 0;
   Status = CsrClientCallServer(&Request, 
				Reply,
				sizeof(CSRSS_API_REQUEST),
				sizeof(CSRSS_API_REPLY) + 
				Request.Data.ReadConsoleRequest.NrCharactersToRead);
   if (!NT_SUCCESS(Status) || !NT_SUCCESS( Status = Reply->Status ))
     {
	DbgPrint( "CSR returned error in ReadConsole\n" );
	SetLastErrorByStatus ( Status );
	RtlFreeHeap( GetProcessHeap(), 0, Reply );
	return(FALSE);
     }
   if( Reply->Status == STATUS_NOTIFY_CLEANUP )
      Reply->Status = STATUS_PENDING;     // ignore backspace because we have no chars to backspace
   /* There may not be any chars or lines to read yet, so wait */
   while( Reply->Status == STATUS_PENDING )
     {
       /* some chars may have been returned, but not a whole line yet, so recompute buffer and try again */
       nNumberOfCharsToRead -= Reply->Data.ReadConsoleReply.NrCharactersRead;
       /* don't overflow caller's buffer, even if you still don't have a complete line */
       if( !nNumberOfCharsToRead )
	 break;
       Request.Data.ReadConsoleRequest.NrCharactersToRead = nNumberOfCharsToRead > CSRSS_MAX_READ_CONSOLE_REQUEST ? CSRSS_MAX_READ_CONSOLE_REQUEST : nNumberOfCharsToRead;
       /* copy any chars already read to buffer */
       memcpy( lpBuffer + CharsRead, Reply->Data.ReadConsoleReply.Buffer, Reply->Data.ReadConsoleReply.NrCharactersRead );
       CharsRead += Reply->Data.ReadConsoleReply.NrCharactersRead;
       /* wait for csrss to signal there is more data to read, but not if we got STATUS_NOTIFY_CLEANUP for backspace */
       Status = NtWaitForSingleObject( Reply->Data.ReadConsoleReply.EventHandle, FALSE, 0 );
       if( !NT_SUCCESS( Status ) )
	  {
	     DbgPrint( "Wait for console input failed!\n" );
	     RtlFreeHeap( GetProcessHeap(), 0, Reply );
	     return FALSE;
	  }
       Request.Data.ReadConsoleRequest.nCharsCanBeDeleted = CharsRead;
       Status = CsrClientCallServer( &Request, Reply, sizeof( CSRSS_API_REQUEST ), sizeof( CSRSS_API_REPLY ) + Request.Data.ReadConsoleRequest.NrCharactersToRead );
       if( !NT_SUCCESS( Status ) || !NT_SUCCESS( Status = Reply->Status ) )
	 {
	   SetLastErrorByStatus ( Status );
	   RtlFreeHeap( GetProcessHeap(), 0, Reply );
	   return FALSE;
	 }
       if( Reply->Status == STATUS_NOTIFY_CLEANUP )
	  {
	     // delete last char
	     if( CharsRead )
		{
		   CharsRead--;
		   nNumberOfCharsToRead++;
		}
	     Reply->Status = STATUS_PENDING;  // retry
	  }
     }
   /* copy data to buffer, count total returned, and return */
   memcpy( lpBuffer + CharsRead, Reply->Data.ReadConsoleReply.Buffer, Reply->Data.ReadConsoleReply.NrCharactersRead );
   CharsRead += Reply->Data.ReadConsoleReply.NrCharactersRead;
   if (lpNumberOfCharsRead != NULL)
     *lpNumberOfCharsRead = CharsRead;
   RtlFreeHeap(GetProcessHeap(),
	    0,
	    Reply);
   
   return(TRUE);
}


/*--------------------------------------------------------------
 *	AllocConsole
 *
 * @implemented
 */
BOOL STDCALL AllocConsole(VOID)
{
   CSRSS_API_REQUEST Request;
   CSRSS_API_REPLY Reply;
   NTSTATUS Status;
   HANDLE hStdError;

   if(NtCurrentPeb()->ProcessParameters->hConsole)
   {
	DPRINT("AllocConsole: Allocate duplicate console to the same Process\n");
	SetLastErrorByStatus (STATUS_OBJECT_EXISTS); 
	return FALSE;	 
   }

   Request.Data.AllocConsoleRequest.CtrlDispatcher = (PCONTROLDISPATCHER) &ConsoleControlDispatcher;

   Request.Type = CSRSS_ALLOC_CONSOLE;
   Status = CsrClientCallServer( &Request, &Reply, sizeof( CSRSS_API_REQUEST ), sizeof( CSRSS_API_REPLY ) );
   if( !NT_SUCCESS( Status ) || !NT_SUCCESS( Status = Reply.Status ) )
      {
	 SetLastErrorByStatus ( Status );
	 return FALSE;
      }
   NtCurrentPeb()->ProcessParameters->hConsole = Reply.Data.AllocConsoleReply.Console;
   SetStdHandle( STD_INPUT_HANDLE, Reply.Data.AllocConsoleReply.InputHandle );
   SetStdHandle( STD_OUTPUT_HANDLE, Reply.Data.AllocConsoleReply.OutputHandle );
   hStdError = DuplicateConsoleHandle(Reply.Data.AllocConsoleReply.OutputHandle,
                                      0,
				      TRUE,
				      DUPLICATE_SAME_ACCESS);
   SetStdHandle( STD_ERROR_HANDLE, hStdError );
   return TRUE;
}


/*--------------------------------------------------------------
 *	FreeConsole
 *
 * @implemented
 */
BOOL STDCALL FreeConsole(VOID)
{
    // AG: I'm not sure if this is correct (what happens to std handles?)
    // but I just tried to reverse what AllocConsole() does...

   CSRSS_API_REQUEST Request;
   CSRSS_API_REPLY Reply;
   NTSTATUS Status;

   Request.Type = CSRSS_FREE_CONSOLE;
   Status = CsrClientCallServer( &Request, &Reply, sizeof( CSRSS_API_REQUEST ), sizeof( CSRSS_API_REPLY ) );
   if( !NT_SUCCESS( Status ) || !NT_SUCCESS( Status = Reply.Status ) )
      {
	 SetLastErrorByStatus ( Status );
	 return FALSE;
      }

   return TRUE;
}


/*--------------------------------------------------------------
 *	GetConsoleScreenBufferInfo
 *
 * @implemented
 */
BOOL
STDCALL
GetConsoleScreenBufferInfo(
    HANDLE hConsoleOutput,
    PCONSOLE_SCREEN_BUFFER_INFO lpConsoleScreenBufferInfo
    )
{
   CSRSS_API_REQUEST Request;
   CSRSS_API_REPLY Reply;
   NTSTATUS Status;

   Request.Type = CSRSS_SCREEN_BUFFER_INFO;
   Request.Data.ScreenBufferInfoRequest.ConsoleHandle = hConsoleOutput;
   Status = CsrClientCallServer( &Request, &Reply, sizeof( CSRSS_API_REQUEST ), sizeof( CSRSS_API_REPLY ) );
   if( !NT_SUCCESS( Status ) || !NT_SUCCESS( Status = Reply.Status ) )
      {
	 SetLastErrorByStatus ( Status );
	 return FALSE;
      }
   *lpConsoleScreenBufferInfo = Reply.Data.ScreenBufferInfoReply.Info;
   return TRUE;
}


/*--------------------------------------------------------------
 *	SetConsoleCursorPosition
 *
 * @implemented
 */
BOOL
STDCALL
SetConsoleCursorPosition(
    HANDLE hConsoleOutput,
    COORD dwCursorPosition
    )
{
   CSRSS_API_REQUEST Request;
   CSRSS_API_REPLY Reply;
   NTSTATUS Status;

   Request.Type = CSRSS_SET_CURSOR;
   Request.Data.SetCursorRequest.ConsoleHandle = hConsoleOutput;
   Request.Data.SetCursorRequest.Position = dwCursorPosition;
   Status = CsrClientCallServer( &Request, &Reply, sizeof( CSRSS_API_REQUEST ), sizeof( CSRSS_API_REPLY ) );
   if( !NT_SUCCESS( Status ) || !NT_SUCCESS( Status = Reply.Status ) )
      {
	 SetLastErrorByStatus ( Status );
	 return FALSE;
      }
   return TRUE;
}


/*--------------------------------------------------------------
 *	FillConsoleOutputCharacterA
 *
 * @implemented
 */
BOOL STDCALL
FillConsoleOutputCharacterA(
	HANDLE		hConsoleOutput,
	CHAR		cCharacter,
	DWORD		nLength,
	COORD		dwWriteCoord,
	LPDWORD		lpNumberOfCharsWritten
	)
{
  CSRSS_API_REQUEST Request;
  CSRSS_API_REPLY Reply;
  NTSTATUS Status;

  Request.Type = CSRSS_FILL_OUTPUT;
  Request.Data.FillOutputRequest.ConsoleHandle = hConsoleOutput;
  Request.Data.FillOutputRequest.Char = cCharacter;
  Request.Data.FillOutputRequest.Position = dwWriteCoord;
  Request.Data.FillOutputRequest.Length = nLength;
  Status = CsrClientCallServer( &Request, &Reply, sizeof( CSRSS_API_REQUEST ), sizeof( CSRSS_API_REPLY ) );
  if ( !NT_SUCCESS( Status ) || !NT_SUCCESS( Status = Reply.Status ) )
    {
      SetLastErrorByStatus(Status);
      return(FALSE);
    }
  if (lpNumberOfCharsWritten != NULL)
     *lpNumberOfCharsWritten = nLength;
  return(TRUE);
}


/*--------------------------------------------------------------
 *	FillConsoleOutputCharacterW
 *
 * @unimplemented
 */
BOOL
STDCALL
FillConsoleOutputCharacterW(
	HANDLE		hConsoleOutput,
	WCHAR		cCharacter,
	DWORD		nLength,
	COORD		dwWriteCoord,
	LPDWORD		lpNumberOfCharsWritten
	)
{
/* TO DO */
	return FALSE;
}


/*--------------------------------------------------------------
 * 	PeekConsoleInputA
 *
 * @implemented
 */
WINBASEAPI
BOOL
WINAPI
PeekConsoleInputA(
	HANDLE			hConsoleInput,
	PINPUT_RECORD		lpBuffer,
	DWORD			nLength,
	LPDWORD			lpNumberOfEventsRead
	)
{
  PCSRSS_API_REQUEST Request;
  CSRSS_API_REPLY Reply;
  NTSTATUS Status;
  PVOID BufferBase;
  PVOID BufferTargetBase;
  DWORD Size;
  
  if(lpBuffer == NULL)
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
  }
  
  Size = nLength * sizeof(INPUT_RECORD);

  Status = CsrCaptureParameterBuffer((PVOID)lpBuffer, Size, &BufferBase, &BufferTargetBase);
  if(!NT_SUCCESS(Status))
  {
    SetLastErrorByStatus(Status);
    return FALSE;
  }
  
  Request = RtlAllocateHeap(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(CSRSS_API_REQUEST));
  if(Request == NULL)
  {
    CsrReleaseParameterBuffer(BufferBase);
    SetLastError(ERROR_OUTOFMEMORY);
    return FALSE;
  }
  
  Request->Type = CSRSS_PEEK_CONSOLE_INPUT;
  Request->Data.PeekConsoleInputRequest.ConsoleHandle = hConsoleInput;
  Request->Data.PeekConsoleInputRequest.Length = nLength;
  Request->Data.PeekConsoleInputRequest.InputRecord = (INPUT_RECORD*)BufferTargetBase;
  
  Status = CsrClientCallServer(Request, &Reply, sizeof(CSRSS_API_REQUEST), sizeof(CSRSS_API_REPLY));
  
  if(!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Reply.Status))
  {
    RtlFreeHeap(GetProcessHeap(), 0, Request);
    CsrReleaseParameterBuffer(BufferBase);
    return FALSE;
  }

  memcpy(lpBuffer, BufferBase, sizeof(INPUT_RECORD) * Reply.Data.PeekConsoleInputReply.Length);

  if(lpNumberOfEventsRead != NULL)
    *lpNumberOfEventsRead = Reply.Data.PeekConsoleInputReply.Length;

  RtlFreeHeap(GetProcessHeap(), 0, Request);
  CsrReleaseParameterBuffer(BufferBase);  
  
	return TRUE;
}


/*--------------------------------------------------------------
 * 	PeekConsoleInputW
 *
 * @unimplemented
 */
WINBASEAPI
BOOL
WINAPI
PeekConsoleInputW(
	HANDLE			hConsoleInput,
	PINPUT_RECORD		lpBuffer,
	DWORD			nLength,
	LPDWORD			lpNumberOfEventsRead
	)    
{
/* TO DO */
	return FALSE;
}


/*--------------------------------------------------------------
 * 	ReadConsoleInputA
 *
 * @implemented
 */
WINBASEAPI BOOL WINAPI
ReadConsoleInputA(HANDLE hConsoleInput,
		  PINPUT_RECORD	lpBuffer,
		  DWORD	nLength,
		  LPDWORD lpNumberOfEventsRead)
{
  CSRSS_API_REQUEST Request;
  CSRSS_API_REPLY Reply;
  DWORD NumEventsRead;
  NTSTATUS Status;

  Request.Type = CSRSS_READ_INPUT;
  Request.Data.ReadInputRequest.ConsoleHandle = hConsoleInput;
  Status = CsrClientCallServer(&Request, &Reply, sizeof(CSRSS_API_REQUEST),
				sizeof(CSRSS_API_REPLY));
  if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Reply.Status))
    {
      SetLastErrorByStatus(Status);
      return(FALSE);
    }
  
  while (Status == STATUS_PENDING)
    {
      Status = NtWaitForSingleObject(Reply.Data.ReadInputReply.Event, FALSE, 
				     0);
      if(!NT_SUCCESS(Status))
	{
	  SetLastErrorByStatus(Status);
	  return FALSE;
	}
      
      Request.Type = CSRSS_READ_INPUT;
      Request.Data.ReadInputRequest.ConsoleHandle = hConsoleInput;
      Status = CsrClientCallServer(&Request, &Reply, sizeof(CSRSS_API_REQUEST),
				   sizeof(CSRSS_API_REPLY));
      if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Reply.Status))
	{
	  SetLastErrorByStatus(Status);
	  return(FALSE);
	}
    }
  
  NumEventsRead = 0;
  *lpBuffer = Reply.Data.ReadInputReply.Input;
  lpBuffer++;
  NumEventsRead++;
  
  while ((NumEventsRead < nLength) && (Reply.Data.ReadInputReply.MoreEvents))
    {
      Status = CsrClientCallServer(&Request, &Reply, sizeof(CSRSS_API_REQUEST),
				   sizeof(CSRSS_API_REPLY));
      if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Reply.Status))
	{
	  SetLastErrorByStatus(Status);
	  return(FALSE);
	}
      
      if (Status == STATUS_PENDING)
	{
	  break;
	}
      
      *lpBuffer = Reply.Data.ReadInputReply.Input;
      lpBuffer++;
      NumEventsRead++;
      
    }
  *lpNumberOfEventsRead = NumEventsRead;
  
  return TRUE;
}


/*--------------------------------------------------------------
 * 	ReadConsoleInputW
 *
 * @unimplemented
 */
WINBASEAPI
BOOL
WINAPI
ReadConsoleInputW(
	HANDLE			hConsoleInput,
	PINPUT_RECORD		lpBuffer,
	DWORD			nLength,
	LPDWORD			lpNumberOfEventsRead
	)
{
/* TO DO */
	return FALSE;
}


/*--------------------------------------------------------------
 * 	WriteConsoleInputA
 *
 * @implemented
 */
WINBASEAPI
BOOL
WINAPI
WriteConsoleInputA(
	HANDLE			 hConsoleInput,
	CONST INPUT_RECORD	*lpBuffer,
	DWORD			 nLength,
	LPDWORD			 lpNumberOfEventsWritten
	)
{
  PCSRSS_API_REQUEST Request;
  CSRSS_API_REPLY Reply;
  PVOID BufferBase, BufferTargetBase;
  NTSTATUS Status;
  DWORD Size;
  
  if(lpBuffer == NULL)
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
  }
  
  Size = nLength * sizeof(INPUT_RECORD);

  Status = CsrCaptureParameterBuffer((PVOID)lpBuffer, Size, &BufferBase, &BufferTargetBase);
  if(!NT_SUCCESS(Status))
  {
    SetLastErrorByStatus(Status);
    return FALSE;
  }
  
  Request = RtlAllocateHeap(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(CSRSS_API_REQUEST));
  if(Request == NULL)
  {
    SetLastError(ERROR_OUTOFMEMORY);
    CsrReleaseParameterBuffer(BufferBase);
    return FALSE;
  }

  Request->Type = CSRSS_WRITE_CONSOLE_INPUT;
  Request->Data.WriteConsoleInputRequest.ConsoleHandle = hConsoleInput;
  Request->Data.WriteConsoleInputRequest.Length = nLength;
  Request->Data.WriteConsoleInputRequest.InputRecord = (PINPUT_RECORD)BufferTargetBase;
  
  Status = CsrClientCallServer(Request, &Reply, sizeof(CSRSS_API_REQUEST), sizeof(CSRSS_API_REPLY));
  if(!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Reply.Status))
  {
    RtlFreeHeap(GetProcessHeap(), 0, Request);
    CsrReleaseParameterBuffer(BufferBase);
    return FALSE;
  }
  
  if(lpNumberOfEventsWritten != NULL)
    *lpNumberOfEventsWritten = Reply.Data.WriteConsoleInputReply.Length;
  
  RtlFreeHeap(GetProcessHeap(), 0, Request);
  CsrReleaseParameterBuffer(BufferBase);
  
  return TRUE;
}


/*--------------------------------------------------------------
 * 	WriteConsoleInputW
 *
 * @unimplemented
 */
WINBASEAPI
BOOL
WINAPI
WriteConsoleInputW(
	HANDLE			 hConsoleInput,
	CONST INPUT_RECORD	*lpBuffer,
	DWORD			 nLength,
	LPDWORD			 lpNumberOfEventsWritten
	)
{
/* TO DO */
	return FALSE;
}


/*--------------------------------------------------------------
 * 	ReadConsoleOutputA
 *
 * @implemented
 */
WINBASEAPI
BOOL
WINAPI
ReadConsoleOutputA(
	HANDLE		hConsoleOutput,
	PCHAR_INFO	lpBuffer,
	COORD		dwBufferSize,
	COORD		dwBufferCoord,
	PSMALL_RECT	lpReadRegion
	)
{
  PCSRSS_API_REQUEST Request;
  CSRSS_API_REPLY Reply;
  PVOID BufferBase;
  PVOID BufferTargetBase;
  NTSTATUS Status;
  DWORD Size, SizeX, SizeY;
  
  if(lpBuffer == NULL)
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
  }
  
  Size = dwBufferSize.X * dwBufferSize.Y * sizeof(CHAR_INFO);

  Status = CsrCaptureParameterBuffer((PVOID)lpBuffer, Size, &BufferBase, &BufferTargetBase);
  if(!NT_SUCCESS(Status))
  {
    SetLastErrorByStatus(Status);
    return FALSE;
  }
  
  Request = RtlAllocateHeap(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(CSRSS_API_REQUEST));
  if(Request == NULL)
  {
    SetLastError(ERROR_OUTOFMEMORY);
    CsrReleaseParameterBuffer(BufferBase);
    return FALSE;
  }
   
  Request->Type = CSRSS_READ_CONSOLE_OUTPUT;
  Request->Data.ReadConsoleOutputRequest.ConsoleHandle = hConsoleOutput;
  Request->Data.ReadConsoleOutputRequest.BufferSize = dwBufferSize;
  Request->Data.ReadConsoleOutputRequest.BufferCoord = dwBufferCoord;
  Request->Data.ReadConsoleOutputRequest.ReadRegion = *lpReadRegion;
  Request->Data.ReadConsoleOutputRequest.CharInfo = (PCHAR_INFO)BufferTargetBase;
  
  Status = CsrClientCallServer(Request, &Reply, sizeof(CSRSS_API_REQUEST), sizeof(CSRSS_API_REPLY));
  if(!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Reply.Status))
  {
    SetLastErrorByStatus(Status);
    RtlFreeHeap(GetProcessHeap(), 0, Request);
    CsrReleaseParameterBuffer(BufferBase);
    return FALSE;
  }
  
  SizeX = Reply.Data.ReadConsoleOutputReply.ReadRegion.Right - Reply.Data.ReadConsoleOutputReply.ReadRegion.Left + 1;
  SizeY = Reply.Data.ReadConsoleOutputReply.ReadRegion.Bottom - Reply.Data.ReadConsoleOutputReply.ReadRegion.Top + 1;
  
  memcpy(lpBuffer, BufferBase, sizeof(CHAR_INFO) * SizeX * SizeY);
  *lpReadRegion = Reply.Data.ReadConsoleOutputReply.ReadRegion;
  
  RtlFreeHeap(GetProcessHeap(), 0, Request);
  CsrReleaseParameterBuffer(BufferBase);
  
  return TRUE;
}


/*--------------------------------------------------------------
 * 	ReadConsoleOutputW
 *
 * @unimplemented
 */
WINBASEAPI
BOOL
WINAPI
ReadConsoleOutputW(
	HANDLE		hConsoleOutput,
	PCHAR_INFO	lpBuffer,
	COORD		dwBufferSize,
	COORD		dwBufferCoord,
	PSMALL_RECT	lpReadRegion
	)
{
/* TO DO */
	return FALSE;
}

/*--------------------------------------------------------------
 * 	WriteConsoleOutputA
 *
 * @implemented
 */
WINBASEAPI BOOL WINAPI
WriteConsoleOutputA(HANDLE		 hConsoleOutput,
		    CONST CHAR_INFO	*lpBuffer,
		    COORD		 dwBufferSize,
		    COORD		 dwBufferCoord,
		    PSMALL_RECT	 lpWriteRegion)
{
  PCSRSS_API_REQUEST Request;
  CSRSS_API_REPLY Reply;
  NTSTATUS Status;
  ULONG Size;
  PVOID BufferBase;
  PVOID BufferTargetBase;

  Size = dwBufferSize.Y * dwBufferSize.X * sizeof(CHAR_INFO);

  Status = CsrCaptureParameterBuffer((PVOID)lpBuffer,
				     Size,
				     &BufferBase,
				     &BufferTargetBase);
  if (!NT_SUCCESS(Status))
    {
      SetLastErrorByStatus(Status);
      return(FALSE);
    }
  
  Request = RtlAllocateHeap(GetProcessHeap(), HEAP_ZERO_MEMORY, 
			    sizeof(CSRSS_API_REQUEST));
  if (Request == NULL)
    {
      CsrReleaseParameterBuffer(BufferBase);
      SetLastError(ERROR_OUTOFMEMORY);
      return FALSE;
    }
  Request->Type = CSRSS_WRITE_CONSOLE_OUTPUT;
  Request->Data.WriteConsoleOutputRequest.ConsoleHandle = hConsoleOutput;
  Request->Data.WriteConsoleOutputRequest.BufferSize = dwBufferSize;
  Request->Data.WriteConsoleOutputRequest.BufferCoord = dwBufferCoord;
  Request->Data.WriteConsoleOutputRequest.WriteRegion = *lpWriteRegion;
  Request->Data.WriteConsoleOutputRequest.CharInfo = 
    (CHAR_INFO*)BufferTargetBase;
  
  Status = CsrClientCallServer(Request, &Reply, 
			       sizeof(CSRSS_API_REQUEST), 
			       sizeof(CSRSS_API_REPLY));
  if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Reply.Status))
    {
      CsrReleaseParameterBuffer(BufferBase);
      RtlFreeHeap(GetProcessHeap(), 0, Request);
      SetLastErrorByStatus(Status);
      return FALSE;
    }
      
  *lpWriteRegion = Reply.Data.WriteConsoleOutputReply.WriteRegion;
  RtlFreeHeap(GetProcessHeap(), 0, Request);
  CsrReleaseParameterBuffer(BufferBase);
  return(TRUE);
}


/*--------------------------------------------------------------
 * 	WriteConsoleOutputW
 *
 * @unimplemented
 */
WINBASEAPI
BOOL
WINAPI
WriteConsoleOutputW(
	HANDLE		 hConsoleOutput,
	CONST CHAR_INFO	*lpBuffer,
	COORD		 dwBufferSize,
	COORD		 dwBufferCoord,
	PSMALL_RECT	 lpWriteRegion
	)
{
/* TO DO */
	return FALSE;
}


/*--------------------------------------------------------------
 * 	ReadConsoleOutputCharacterA
 *
 * @implemented
 */
WINBASEAPI
BOOL
WINAPI
ReadConsoleOutputCharacterA(
	HANDLE		hConsoleOutput,
	LPSTR		lpCharacter,
	DWORD		nLength,
	COORD		dwReadCoord,
	LPDWORD		lpNumberOfCharsRead
	)
{
  CSRSS_API_REQUEST Request;
  PCSRSS_API_REPLY Reply;
  NTSTATUS Status;
  DWORD Size;

  Reply = RtlAllocateHeap(GetProcessHeap(),
			  HEAP_ZERO_MEMORY,
			  sizeof(CSRSS_API_REPLY) + CSRSS_MAX_READ_CONSOLE_OUTPUT_CHAR);
  if (Reply == NULL)
    {
      SetLastError(ERROR_OUTOFMEMORY);
      return(FALSE);
    }

  if (lpNumberOfCharsRead != NULL)
    *lpNumberOfCharsRead = nLength;

  Request.Type = CSRSS_READ_CONSOLE_OUTPUT_CHAR;
  Request.Data.ReadConsoleOutputCharRequest.ConsoleHandle = hConsoleOutput;
  Request.Data.ReadConsoleOutputCharRequest.ReadCoord = dwReadCoord;

  while (nLength != 0)
    {
      if (nLength > CSRSS_MAX_READ_CONSOLE_OUTPUT_CHAR)
	Size = CSRSS_MAX_READ_CONSOLE_OUTPUT_CHAR;
      else
	Size = nLength;

      Request.Data.ReadConsoleOutputCharRequest.NumCharsToRead = Size;

      Status = CsrClientCallServer(&Request,
				   Reply,
				   sizeof(CSRSS_API_REQUEST),
				   sizeof(CSRSS_API_REPLY) + Size);
      if (!NT_SUCCESS(Status) || !NT_SUCCESS(Reply->Status))
	{
	  RtlFreeHeap(GetProcessHeap(), 0, Reply);
	  SetLastErrorByStatus(Status);
	  return(FALSE);
	}

      memcpy(lpCharacter, &Reply->Data.ReadConsoleOutputCharReply.String[0], Size);
      lpCharacter += Size;
      nLength -= Size;
      Request.Data.ReadConsoleOutputCharRequest.ReadCoord = Reply->Data.ReadConsoleOutputCharReply.EndCoord;
    }

  RtlFreeHeap(GetProcessHeap(), 0, Reply);

  return(TRUE);
}


/*--------------------------------------------------------------
 *      ReadConsoleOutputCharacterW
 *
 * @unimplemented
 */
WINBASEAPI
BOOL
WINAPI
ReadConsoleOutputCharacterW(
	HANDLE		hConsoleOutput,
	LPWSTR		lpCharacter,
	DWORD		nLength,
	COORD		dwReadCoord,
	LPDWORD		lpNumberOfCharsRead
	)
{
/* TO DO */
	return FALSE;
}


/*--------------------------------------------------------------
 * 	ReadConsoleOutputAttribute
 *
 * @implemented
 */
WINBASEAPI
BOOL
WINAPI
ReadConsoleOutputAttribute(
	HANDLE		hConsoleOutput,
	LPWORD		lpAttribute,
	DWORD		nLength,
	COORD		dwReadCoord,
	LPDWORD		lpNumberOfAttrsRead
	)
{
  CSRSS_API_REQUEST Request;
  PCSRSS_API_REPLY Reply;
  NTSTATUS Status;
  DWORD Size, i;
  
  Reply = RtlAllocateHeap(GetProcessHeap(),
			  HEAP_ZERO_MEMORY,
			  sizeof(CSRSS_API_REPLY) + CSRSS_MAX_READ_CONSOLE_OUTPUT_ATTRIB);
  if (Reply == NULL)
    {
      SetLastError(ERROR_OUTOFMEMORY);
      return(FALSE);
    }

  if (lpNumberOfAttrsRead != NULL)
    *lpNumberOfAttrsRead = nLength;

  Request.Type = CSRSS_READ_CONSOLE_OUTPUT_ATTRIB;
  Request.Data.ReadConsoleOutputAttribRequest.ConsoleHandle = hConsoleOutput;
  Request.Data.ReadConsoleOutputAttribRequest.ReadCoord = dwReadCoord;

  while (nLength != 0)
    {
      if (nLength > CSRSS_MAX_READ_CONSOLE_OUTPUT_ATTRIB)
	Size = CSRSS_MAX_READ_CONSOLE_OUTPUT_ATTRIB;
      else
	Size = nLength;

      Request.Data.ReadConsoleOutputAttribRequest.NumAttrsToRead = Size;

      Status = CsrClientCallServer(&Request,
				   Reply,
				   sizeof(CSRSS_API_REQUEST),
				   sizeof(CSRSS_API_REPLY) + Size);
      if (!NT_SUCCESS(Status) || !NT_SUCCESS(Reply->Status))
	{
	  RtlFreeHeap(GetProcessHeap(), 0, Reply);
	  SetLastErrorByStatus(Status);
	  return(FALSE);
	}

      // Convert CHARs to WORDs
      for(i = 0; i < Size; ++i)
        *lpAttribute++ = Reply->Data.ReadConsoleOutputAttribReply.String[i];
      
      nLength -= Size;
      Request.Data.ReadConsoleOutputAttribRequest.ReadCoord = Reply->Data.ReadConsoleOutputAttribReply.EndCoord;
    }

  RtlFreeHeap(GetProcessHeap(), 0, Reply);

  return(TRUE);
}


/*--------------------------------------------------------------
 * 	WriteConsoleOutputCharacterA
 *
 * @implemented
 */
WINBASEAPI BOOL WINAPI
WriteConsoleOutputCharacterA(HANDLE		hConsoleOutput,
			     LPCSTR		lpCharacter,
			     DWORD		nLength,
			     COORD		dwWriteCoord,
			     LPDWORD		lpNumberOfCharsWritten)
{
  PCSRSS_API_REQUEST Request;
  CSRSS_API_REPLY Reply;
  NTSTATUS Status;
  WORD Size;
  
  Request = RtlAllocateHeap(GetProcessHeap(),
			    HEAP_ZERO_MEMORY,
			    sizeof(CSRSS_API_REQUEST) + CSRSS_MAX_WRITE_CONSOLE_OUTPUT_CHAR);
  if( !Request )
    {
      SetLastError( ERROR_OUTOFMEMORY );
      return FALSE;
    }
  Request->Type = CSRSS_WRITE_CONSOLE_OUTPUT_CHAR;
  Request->Data.WriteConsoleOutputCharRequest.ConsoleHandle = hConsoleOutput;
  Request->Data.WriteConsoleOutputCharRequest.Coord = dwWriteCoord;
  if( lpNumberOfCharsWritten )
    *lpNumberOfCharsWritten = nLength;
  while( nLength )
    {
      Size = nLength > CSRSS_MAX_WRITE_CONSOLE_OUTPUT_CHAR ? CSRSS_MAX_WRITE_CONSOLE_OUTPUT_CHAR : nLength;
      Request->Data.WriteConsoleOutputCharRequest.Length = Size;
      memcpy( &Request->Data.WriteConsoleOutputCharRequest.String[0],
	      lpCharacter,
	      Size );
      Status = CsrClientCallServer( Request, &Reply, sizeof( CSRSS_API_REQUEST ) + Size, sizeof( CSRSS_API_REPLY ) );
      if( !NT_SUCCESS( Status ) || !NT_SUCCESS( Status = Reply.Status ) )
	{
	  RtlFreeHeap( GetProcessHeap(), 0, Request );
	  SetLastErrorByStatus ( Status );
	  return FALSE;
	}
      nLength -= Size;
      lpCharacter += Size;
      Request->Data.WriteConsoleOutputCharRequest.Coord = Reply.Data.WriteConsoleOutputCharReply.EndCoord;
    }
  
  RtlFreeHeap( GetProcessHeap(), 0, Request );	
  return TRUE;
}


/*--------------------------------------------------------------
 * 	WriteConsoleOutputCharacterW
 *
 * @implemented
 */
WINBASEAPI BOOL WINAPI
WriteConsoleOutputCharacterW(HANDLE		hConsoleOutput,
			     LPCWSTR		lpCharacter,
			     DWORD		nLength,
			     COORD		dwWriteCoord,
			     LPDWORD		lpNumberOfCharsWritten)
{
  PCSRSS_API_REQUEST Request;
  CSRSS_API_REPLY Reply;
  NTSTATUS Status;
  WORD Size;
  
  Request = RtlAllocateHeap(GetProcessHeap(),
			    HEAP_ZERO_MEMORY,
			    sizeof(CSRSS_API_REQUEST) + CSRSS_MAX_WRITE_CONSOLE_OUTPUT_CHAR);
  if( !Request )
    {
      SetLastError( ERROR_OUTOFMEMORY );
      return FALSE;
    }
  Request->Type = CSRSS_WRITE_CONSOLE_OUTPUT_CHAR;
  Request->Data.WriteConsoleOutputCharRequest.ConsoleHandle = hConsoleOutput;
  Request->Data.WriteConsoleOutputCharRequest.Coord = dwWriteCoord;
  if( lpNumberOfCharsWritten )
    *lpNumberOfCharsWritten = nLength;
  while( nLength )
    {
      Size = nLength > CSRSS_MAX_WRITE_CONSOLE_OUTPUT_CHAR ? CSRSS_MAX_WRITE_CONSOLE_OUTPUT_CHAR : nLength;
      Request->Data.WriteConsoleOutputCharRequest.Length = Size;
      Status = RtlUnicodeToOemN (&Request->Data.WriteConsoleOutputCharRequest.String[0],
				 Size,
				 NULL,
				 (PWCHAR)lpCharacter,
				 Size * sizeof(WCHAR));
      if (!NT_SUCCESS(Status))
	{
	  RtlFreeHeap (GetProcessHeap(), 0, Request);
	  SetLastErrorByStatus (Status);
	  return FALSE;
	}

      Status = CsrClientCallServer( Request, &Reply, sizeof( CSRSS_API_REQUEST ) + Size, sizeof( CSRSS_API_REPLY ) );
      if( !NT_SUCCESS( Status ) || !NT_SUCCESS( Status = Reply.Status ) )
	{
	  RtlFreeHeap( GetProcessHeap(), 0, Request );
	  SetLastErrorByStatus ( Status );
	  return FALSE;
	}
      nLength -= Size;
      lpCharacter += Size;
      Request->Data.WriteConsoleOutputCharRequest.Coord = Reply.Data.WriteConsoleOutputCharReply.EndCoord;
    }
  
  RtlFreeHeap( GetProcessHeap(), 0, Request );
  return TRUE;
}


/*--------------------------------------------------------------
 * 	WriteConsoleOutputAttribute
 *
 * @implemented
 */
WINBASEAPI
BOOL
WINAPI
WriteConsoleOutputAttribute(
	HANDLE		 hConsoleOutput,
	CONST WORD	*lpAttribute,
	DWORD		 nLength,
	COORD		 dwWriteCoord,
	LPDWORD		 lpNumberOfAttrsWritten
	)
{
   PCSRSS_API_REQUEST Request;
   CSRSS_API_REPLY Reply;
   NTSTATUS Status;
   WORD Size;
   int c;

   Request = RtlAllocateHeap(GetProcessHeap(),
		       HEAP_ZERO_MEMORY,
		       sizeof(CSRSS_API_REQUEST) + CSRSS_MAX_WRITE_CONSOLE_OUTPUT_ATTRIB);
   if( !Request )
     {
       SetLastError( ERROR_OUTOFMEMORY );
       return FALSE;
     }
   Request->Type = CSRSS_WRITE_CONSOLE_OUTPUT_ATTRIB;
   Request->Data.WriteConsoleOutputAttribRequest.ConsoleHandle = hConsoleOutput;
   Request->Data.WriteConsoleOutputAttribRequest.Coord = dwWriteCoord;
   if( lpNumberOfAttrsWritten )
      *lpNumberOfAttrsWritten = nLength;
   while( nLength )
      {
	 Size = nLength > CSRSS_MAX_WRITE_CONSOLE_OUTPUT_ATTRIB ? CSRSS_MAX_WRITE_CONSOLE_OUTPUT_ATTRIB : nLength;
	 Request->Data.WriteConsoleOutputAttribRequest.Length = Size;
	 for( c = 0; c < ( Size * 2 ); c++ )
	   Request->Data.WriteConsoleOutputAttribRequest.String[c] = (char)lpAttribute[c];
	 Status = CsrClientCallServer( Request, &Reply, sizeof( CSRSS_API_REQUEST ) + (Size * 2), sizeof( CSRSS_API_REPLY ) );
	 if( !NT_SUCCESS( Status ) || !NT_SUCCESS( Status = Reply.Status ) )
	    {
	       RtlFreeHeap( GetProcessHeap(), 0, Request );
	       SetLastErrorByStatus ( Status );
	       return FALSE;
	    }
	 nLength -= Size;
	 lpAttribute += Size;
	 Request->Data.WriteConsoleOutputAttribRequest.Coord = Reply.Data.WriteConsoleOutputAttribReply.EndCoord;
      }
   
   RtlFreeHeap( GetProcessHeap(), 0, Request );
   return TRUE;
}


/*--------------------------------------------------------------
 * 	FillConsoleOutputAttribute
 *
 * @implemented
 */
WINBASEAPI
BOOL
WINAPI
FillConsoleOutputAttribute(
	HANDLE		hConsoleOutput,
	WORD		wAttribute,
	DWORD		nLength,
	COORD		dwWriteCoord,
	LPDWORD		lpNumberOfAttrsWritten
	)
{
   CSRSS_API_REQUEST Request;
   CSRSS_API_REPLY Reply;
   NTSTATUS Status;

   Request.Type = CSRSS_FILL_OUTPUT_ATTRIB;
   Request.Data.FillOutputAttribRequest.ConsoleHandle = hConsoleOutput;
   Request.Data.FillOutputAttribRequest.Attribute = wAttribute;
   Request.Data.FillOutputAttribRequest.Coord = dwWriteCoord;
   Request.Data.FillOutputAttribRequest.Length = nLength;
   Status = CsrClientCallServer( &Request, &Reply, sizeof( CSRSS_API_REQUEST ), sizeof( CSRSS_API_REPLY ) );
   if( !NT_SUCCESS( Status ) || !NT_SUCCESS( Status = Reply.Status ) )
      {
	 SetLastErrorByStatus ( Status );
	 return FALSE;
      }
   if( lpNumberOfAttrsWritten )
      *lpNumberOfAttrsWritten = nLength;
   return TRUE;
}


/*--------------------------------------------------------------
 * 	GetConsoleMode
 *
 * @implemented
 */
WINBASEAPI
BOOL
WINAPI
GetConsoleMode(
	HANDLE		hConsoleHandle,
	LPDWORD		lpMode
	)
{
  CSRSS_API_REQUEST Request;
  CSRSS_API_REPLY Reply;
  NTSTATUS Status;
  
  Request.Type = CSRSS_GET_CONSOLE_MODE;
  Request.Data.GetConsoleModeRequest.ConsoleHandle = hConsoleHandle;
  Status = CsrClientCallServer( &Request, &Reply, sizeof( CSRSS_API_REQUEST ), sizeof( CSRSS_API_REPLY ) );
  if( !NT_SUCCESS( Status ) || !NT_SUCCESS( Status = Reply.Status ) )
      {
	SetLastErrorByStatus ( Status );
	return FALSE;
      }
  *lpMode = Reply.Data.GetConsoleModeReply.ConsoleMode;
  return TRUE;
}


/*--------------------------------------------------------------
 * 	GetNumberOfConsoleInputEvents
 *
 * @implemented
 */
WINBASEAPI
BOOL
WINAPI
GetNumberOfConsoleInputEvents(
	HANDLE		hConsoleInput,
	LPDWORD		lpNumberOfEvents
	)
{
   CSRSS_API_REQUEST Request;
   CSRSS_API_REPLY Reply;
   NTSTATUS Status;
 
   if(lpNumberOfEvents == NULL)
   {
      SetLastError(ERROR_INVALID_PARAMETER);
      return FALSE;
   }
   
   Request.Type = CSRSS_GET_NUM_INPUT_EVENTS;
   Request.Data.GetNumInputEventsRequest.ConsoleHandle = hConsoleInput;
   Status = CsrClientCallServer(&Request, &Reply, sizeof(CSRSS_API_REQUEST), sizeof(CSRSS_API_REPLY));
   if(!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Reply.Status))
   {
      SetLastErrorByStatus(Status);
      return FALSE;
   }
   
   *lpNumberOfEvents = Reply.Data.GetNumInputEventsReply.NumInputEvents;
   
	return TRUE;
}


/*--------------------------------------------------------------
 * 	GetLargestConsoleWindowSize
 *
 * @unimplemented
 */
WINBASEAPI
COORD
WINAPI
GetLargestConsoleWindowSize(
	HANDLE		hConsoleOutput
	)
{
#if 1	/* FIXME: */
	COORD Coord = {80,25};

/* TO DO */
	return Coord;
#endif
}


/*--------------------------------------------------------------
 *	GetConsoleCursorInfo
 *
 * @implemented
 */
WINBASEAPI
BOOL
WINAPI
GetConsoleCursorInfo(
	HANDLE			hConsoleOutput,
	PCONSOLE_CURSOR_INFO	lpConsoleCursorInfo
	)
{
   CSRSS_API_REQUEST Request;
   CSRSS_API_REPLY Reply;
   NTSTATUS Status;

   Request.Type = CSRSS_GET_CURSOR_INFO;
   Request.Data.GetCursorInfoRequest.ConsoleHandle = hConsoleOutput;
   Status = CsrClientCallServer( &Request, &Reply, sizeof( CSRSS_API_REQUEST ), sizeof( CSRSS_API_REPLY ) );

   if( !NT_SUCCESS( Status ) || !NT_SUCCESS( Status = Reply.Status ) )
      {
	 SetLastErrorByStatus ( Status );
	 return FALSE;
      }
   *lpConsoleCursorInfo = Reply.Data.GetCursorInfoReply.Info;
   return TRUE;
}


/*--------------------------------------------------------------
 * 	GetNumberOfConsoleMouseButtons
 *
 * @unimplemented
 */
WINBASEAPI
BOOL
WINAPI
GetNumberOfConsoleMouseButtons(
	LPDWORD		lpNumberOfMouseButtons
	)
{
/* TO DO */
	return FALSE;
}


/*--------------------------------------------------------------
 * 	SetConsoleMode
 *
 * @implemented
 */
WINBASEAPI
BOOL
WINAPI
SetConsoleMode(
	HANDLE		hConsoleHandle,
	DWORD		dwMode
	)
{
  CSRSS_API_REQUEST Request;
  CSRSS_API_REPLY Reply;
  NTSTATUS Status;
  
  Request.Type = CSRSS_SET_CONSOLE_MODE;
  Request.Data.SetConsoleModeRequest.ConsoleHandle = hConsoleHandle;
  Request.Data.SetConsoleModeRequest.Mode = dwMode;
  Status = CsrClientCallServer( &Request, &Reply, sizeof( CSRSS_API_REQUEST ), sizeof( CSRSS_API_REPLY ) );
  if( !NT_SUCCESS( Status ) || !NT_SUCCESS( Status = Reply.Status ) )
      {
	SetLastErrorByStatus ( Status );
	return FALSE;
      }
  return TRUE;
}


/*--------------------------------------------------------------
 * 	SetConsoleActiveScreenBuffer
 *
 * @implemented
 */
WINBASEAPI
BOOL
WINAPI
SetConsoleActiveScreenBuffer(
	HANDLE		hConsoleOutput
	)
{
   CSRSS_API_REQUEST Request;
   CSRSS_API_REPLY Reply;
   NTSTATUS Status;

   Request.Type = CSRSS_SET_SCREEN_BUFFER;
   Request.Data.SetScreenBufferRequest.OutputHandle = hConsoleOutput;
   Status = CsrClientCallServer( &Request, &Reply, sizeof( CSRSS_API_REQUEST ), sizeof( CSRSS_API_REPLY ) );
   if( !NT_SUCCESS( Status ) || !NT_SUCCESS( Status = Reply.Status ) )
      {
	 SetLastErrorByStatus ( Status );
	 return FALSE;
      }
   return TRUE;
}


/*--------------------------------------------------------------
 * 	FlushConsoleInputBuffer
 *
 * @implemented
 */
WINBASEAPI
BOOL
WINAPI
FlushConsoleInputBuffer(
	HANDLE		hConsoleInput
	)
{
   CSRSS_API_REQUEST Request;
   CSRSS_API_REPLY Reply;
   NTSTATUS Status;

   Request.Type = CSRSS_FLUSH_INPUT_BUFFER;
   Request.Data.FlushInputBufferRequest.ConsoleInput = hConsoleInput;
   Status = CsrClientCallServer( &Request, &Reply, sizeof( CSRSS_API_REQUEST ), sizeof( CSRSS_API_REPLY ) );
   if( !NT_SUCCESS( Status ) || !NT_SUCCESS( Status = Reply.Status ) )
      {
	 SetLastErrorByStatus ( Status );
	 return FALSE;
      }
   return TRUE;
}


/*--------------------------------------------------------------
 * 	SetConsoleScreenBufferSize
 *
 * @unimplemented
 */
WINBASEAPI
BOOL
WINAPI
SetConsoleScreenBufferSize(
	HANDLE		hConsoleOutput,
	COORD		dwSize
	)
{
/* TO DO */
	return FALSE;
}

/*--------------------------------------------------------------
 * 	SetConsoleCursorInfo
 *
 * @implemented
 */
WINBASEAPI
BOOL
WINAPI
SetConsoleCursorInfo(
	HANDLE				 hConsoleOutput,
	CONST CONSOLE_CURSOR_INFO	*lpConsoleCursorInfo
	)
{
   CSRSS_API_REQUEST Request;
   CSRSS_API_REPLY Reply;
   NTSTATUS Status;

   Request.Type = CSRSS_SET_CURSOR_INFO;
   Request.Data.SetCursorInfoRequest.ConsoleHandle = hConsoleOutput;
   Request.Data.SetCursorInfoRequest.Info = *lpConsoleCursorInfo;
   Status = CsrClientCallServer( &Request, &Reply, sizeof( CSRSS_API_REQUEST ), sizeof( CSRSS_API_REPLY ) );

   if( !NT_SUCCESS( Status ) || !NT_SUCCESS( Status = Reply.Status ) )
      {
	 SetLastErrorByStatus ( Status );
	 return FALSE;
      }
   return TRUE;
}


/*--------------------------------------------------------------
 *	ScrollConsoleScreenBufferA
 *
 * @implemented
 */
WINBASEAPI
BOOL
WINAPI
ScrollConsoleScreenBufferA(
	HANDLE			 hConsoleOutput,
	CONST SMALL_RECT	*lpScrollRectangle,
	CONST SMALL_RECT	*lpClipRectangle,
	COORD			 dwDestinationOrigin,
	CONST CHAR_INFO		*lpFill
	)
{
  CSRSS_API_REQUEST Request;
  CSRSS_API_REPLY Reply;
  NTSTATUS Status;

  Request.Type = CSRSS_SCROLL_CONSOLE_SCREEN_BUFFER;
  Request.Data.ScrollConsoleScreenBufferRequest.ConsoleHandle = hConsoleOutput;
  Request.Data.ScrollConsoleScreenBufferRequest.ScrollRectangle = *lpScrollRectangle;

  if (lpClipRectangle != NULL)
    {
  Request.Data.ScrollConsoleScreenBufferRequest.UseClipRectangle = TRUE;
  Request.Data.ScrollConsoleScreenBufferRequest.ClipRectangle = *lpClipRectangle;
    }
  else
    {
  Request.Data.ScrollConsoleScreenBufferRequest.UseClipRectangle = FALSE;
    }

  Request.Data.ScrollConsoleScreenBufferRequest.DestinationOrigin = dwDestinationOrigin;
  Request.Data.ScrollConsoleScreenBufferRequest.Fill = *lpFill;
  Status = CsrClientCallServer( &Request, &Reply, sizeof( CSRSS_API_REQUEST ), sizeof( CSRSS_API_REPLY ) );

  if( !NT_SUCCESS( Status ) || !NT_SUCCESS( Status = Reply.Status ) )
    {
      SetLastErrorByStatus ( Status );
      return FALSE;
    }
  return TRUE;
}


/*--------------------------------------------------------------
 * 	ScrollConsoleScreenBufferW
 *
 * @unimplemented
 */
WINBASEAPI
BOOL
WINAPI
ScrollConsoleScreenBufferW(
	HANDLE			 hConsoleOutput,
	CONST SMALL_RECT	*lpScrollRectangle,
	CONST SMALL_RECT	*lpClipRectangle,
	COORD			 dwDestinationOrigin,
	CONST CHAR_INFO		*lpFill
	)
{
/* TO DO */
	return FALSE;
}


/*--------------------------------------------------------------
 * 	SetConsoleWindowInfo
 *
 * @unimplemented
 */
WINBASEAPI
BOOL
WINAPI
SetConsoleWindowInfo(
	HANDLE			 hConsoleOutput,
	BOOL			 bAbsolute,
	CONST SMALL_RECT	*lpConsoleWindow
	)
{
/* TO DO */
	return FALSE;
}


/*--------------------------------------------------------------
 *      SetConsoleTextAttribute
 *
 * @implemented
 */
WINBASEAPI
BOOL
WINAPI
SetConsoleTextAttribute(
        HANDLE		hConsoleOutput,
        WORD            wAttributes
        )
{
   CSRSS_API_REQUEST Request;
   CSRSS_API_REPLY Reply;
   NTSTATUS Status;

   Request.Type = CSRSS_SET_ATTRIB;
   Request.Data.SetAttribRequest.ConsoleHandle = hConsoleOutput;
   Request.Data.SetAttribRequest.Attrib = wAttributes;
   Status = CsrClientCallServer( &Request, &Reply, sizeof( CSRSS_API_REQUEST ), sizeof( CSRSS_API_REPLY ) );
   if( !NT_SUCCESS( Status ) || !NT_SUCCESS( Status = Reply.Status ) )
      {
	 SetLastErrorByStatus ( Status );
	 return FALSE;
      }
   return TRUE;
}


BOOL STATIC
AddConsoleCtrlHandler(PHANDLER_ROUTINE HandlerRoutine)
{
  if (HandlerRoutine == NULL)
    {
      IgnoreCtrlEvents = TRUE;
      return(TRUE);
    }
  else
    {
      NrCtrlHandlers++;
      CtrlHandlers = 
	RtlReAllocateHeap(RtlGetProcessHeap(),
			   HEAP_ZERO_MEMORY,
			   (PVOID)CtrlHandlers,
			   NrCtrlHandlers * sizeof(PHANDLER_ROUTINE)); 
      if (CtrlHandlers == NULL)
	{
	  NrCtrlHandlers = 0;
	  SetLastError(ERROR_NOT_ENOUGH_MEMORY);
	  return(FALSE);
	}
      CtrlHandlers[NrCtrlHandlers - 1] = HandlerRoutine;
      return(TRUE);
    }
}


BOOL STATIC
RemoveConsoleCtrlHandler(PHANDLER_ROUTINE HandlerRoutine)
{
  ULONG i;

  if (HandlerRoutine == NULL)
    {
      IgnoreCtrlEvents = FALSE;
      return(TRUE);
    }
  else
    {
      for (i = 0; i < NrCtrlHandlers; i++)
	{
	  if ( ((void*)(CtrlHandlers[i])) == (void*)HandlerRoutine)
	    {
	      NrCtrlHandlers--;
	      memmove(CtrlHandlers + i, CtrlHandlers + i + 1, 
		      (NrCtrlHandlers - i) * sizeof(PHANDLER_ROUTINE));
	      CtrlHandlers = 
		RtlReAllocateHeap(RtlGetProcessHeap(),
				  HEAP_ZERO_MEMORY,
				  (PVOID)CtrlHandlers,
				  NrCtrlHandlers * sizeof(PHANDLER_ROUTINE));
	      return(TRUE);
	    }
	}
    }
  return(FALSE);
}


/*
 * @implemented
 */
WINBASEAPI BOOL WINAPI
SetConsoleCtrlHandler(PHANDLER_ROUTINE HandlerRoutine,
		      BOOL Add)
{
  BOOLEAN Ret;

  RtlEnterCriticalSection(&DllLock);
  if (Add)
    {
      Ret = AddConsoleCtrlHandler(HandlerRoutine);
    }
  else
    {
      Ret = RemoveConsoleCtrlHandler(HandlerRoutine);
    }
  RtlLeaveCriticalSection(&DllLock);
  return(Ret);
}


/*--------------------------------------------------------------
 * 	GenerateConsoleCtrlEvent
 *
 * @unimplemented
 */
WINBASEAPI BOOL WINAPI
GenerateConsoleCtrlEvent(
	DWORD		dwCtrlEvent,
	DWORD		dwProcessGroupId
	)
{
  /* TO DO */
  return FALSE;
}


/*--------------------------------------------------------------
 *	GetConsoleTitleW
 *
 * @implemented
 */
WINBASEAPI
DWORD
WINAPI
GetConsoleTitleW(
	LPWSTR		lpConsoleTitle,
	DWORD		nSize
	)
{
   CSRSS_API_REQUEST Request;
   PCSRSS_API_REPLY Reply;
   NTSTATUS Status;
   HANDLE hConsole;

   hConsole = CreateFileW(L"CONIN$", GENERIC_READ, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
   if (hConsole == INVALID_HANDLE_VALUE)
   {
      return 0;
   }

   Reply = RtlAllocateHeap(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(CSRSS_API_REPLY) + CSRSS_MAX_TITLE_LENGTH * sizeof(WCHAR));
   if(Reply == NULL)
   {
      CloseHandle(hConsole);   
      SetLastError(ERROR_OUTOFMEMORY);
      return 0;
   }

   Request.Type = CSRSS_GET_TITLE;
   Request.Data.GetTitleRequest.ConsoleHandle = hConsole;
   
   Status = CsrClientCallServer(&Request, Reply, sizeof(CSRSS_API_REQUEST), sizeof(CSRSS_API_REPLY) + CSRSS_MAX_TITLE_LENGTH * sizeof(WCHAR));
   CloseHandle(hConsole);
   if(!NT_SUCCESS(Status) || !(NT_SUCCESS(Status = Reply->Status)))
   {
      SetLastErrorByStatus(Status);
      RtlFreeHeap(GetProcessHeap(), 0, Reply);
      return 0;
   }
   
   if(nSize * sizeof(WCHAR) < Reply->Data.GetTitleReply.Length)
   {
      wcsncpy(lpConsoleTitle, Reply->Data.GetTitleReply.Title, nSize - 1);
      lpConsoleTitle[nSize--] = L'\0';
   }
   else
   {  
      nSize = Reply->Data.GetTitleReply.Length / sizeof (WCHAR);
      wcscpy(lpConsoleTitle, Reply->Data.GetTitleReply.Title);
      lpConsoleTitle[nSize] = L'\0';
   }
   
   RtlFreeHeap(GetProcessHeap(), 0, Reply);
   return nSize;
}


/*--------------------------------------------------------------
 * 	GetConsoleTitleA
 *
 * 	19990306 EA
 *
 * @implemented
 */
WINBASEAPI
DWORD
WINAPI
GetConsoleTitleA(
	LPSTR		lpConsoleTitle,
	DWORD		nSize
	)
{
	wchar_t	WideTitle [CSRSS_MAX_TITLE_LENGTH];
	DWORD	nWideTitle = sizeof WideTitle;
	DWORD	nWritten;
	
	if (!lpConsoleTitle || !nSize) return 0;
	nWideTitle = GetConsoleTitleW( (LPWSTR) WideTitle, nWideTitle );
	if (!nWideTitle) return 0;

	if ( (nWritten = WideCharToMultiByte(
    		CP_ACP,			// ANSI code page 
		0,			// performance and mapping flags 
		(LPWSTR) WideTitle,	// address of wide-character string 
		nWideTitle,		// number of characters in string 
		lpConsoleTitle,		// address of buffer for new string 
		nSize,			// size of buffer 
		NULL,			// FAST
		NULL	 		// FAST
		)))
	{
		lpConsoleTitle[nWritten] = '\0';
		return nWritten;
	}

	return 0;
}


/*--------------------------------------------------------------
 *	SetConsoleTitleW
 *
 * @implemented
 */
WINBASEAPI
BOOL
WINAPI
SetConsoleTitleW(
	LPCWSTR		lpConsoleTitle
	)
{
  PCSRSS_API_REQUEST Request;
  CSRSS_API_REPLY Reply;
  NTSTATUS Status;
  unsigned int c;
  HANDLE hConsole;

  hConsole = CreateFileW(L"CONIN$", GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  if (hConsole == INVALID_HANDLE_VALUE)
  {
     return FALSE;
  }
  
  Request = RtlAllocateHeap(GetProcessHeap(),
			    HEAP_ZERO_MEMORY,
			    sizeof(CSRSS_API_REQUEST) + CSRSS_MAX_SET_TITLE_REQUEST);
  if (Request == NULL)
    {
      CloseHandle(hConsole);
      SetLastError(ERROR_OUTOFMEMORY);
      return(FALSE);
    }
  
  Request->Type = CSRSS_SET_TITLE;
  Request->Data.SetTitleRequest.Console = hConsole;
  
  for( c = 0; lpConsoleTitle[c] && c < CSRSS_MAX_TITLE_LENGTH; c++ )
    Request->Data.SetTitleRequest.Title[c] = lpConsoleTitle[c];
  // add null
  Request->Data.SetTitleRequest.Title[c] = 0;
  Request->Data.SetTitleRequest.Length = c;  
  Status = CsrClientCallServer(Request,
			       &Reply,
			       sizeof(CSRSS_API_REQUEST) + 
			       c * sizeof(WCHAR),
			       sizeof(CSRSS_API_REPLY));
  CloseHandle(hConsole);
  if (!NT_SUCCESS(Status) || !NT_SUCCESS( Status = Reply.Status ) )
    {
      RtlFreeHeap( GetProcessHeap(), 0, Request );
      SetLastErrorByStatus (Status);
      return(FALSE);
    }
  RtlFreeHeap( GetProcessHeap(), 0, Request );
  return TRUE;
}


/*--------------------------------------------------------------
 *	SetConsoleTitleA
 *	
 * 	19990204 EA	Added
 *
 * @implemented
 */
WINBASEAPI
BOOL
WINAPI
SetConsoleTitleA(
	LPCSTR		lpConsoleTitle
	)
{
  PCSRSS_API_REQUEST Request;
  CSRSS_API_REPLY Reply;
  NTSTATUS Status;
  unsigned int c;
  HANDLE hConsole;

  hConsole = CreateFileW(L"CONIN$", GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  if (hConsole == INVALID_HANDLE_VALUE)
  {
     return FALSE;
  }
  
  Request = RtlAllocateHeap(GetProcessHeap(),
			    HEAP_ZERO_MEMORY,
			    sizeof(CSRSS_API_REQUEST) + CSRSS_MAX_SET_TITLE_REQUEST);
  if (Request == NULL)
    {
      CloseHandle(hConsole);
      SetLastError(ERROR_OUTOFMEMORY);
      return(FALSE);
    }
  
  Request->Type = CSRSS_SET_TITLE;
  Request->Data.SetTitleRequest.Console = hConsole;
  
  for( c = 0; lpConsoleTitle[c] && c < CSRSS_MAX_TITLE_LENGTH; c++ )
    Request->Data.SetTitleRequest.Title[c] = lpConsoleTitle[c];
  // add null
  Request->Data.SetTitleRequest.Title[c] = 0;
  Request->Data.SetTitleRequest.Length = c;
  Status = CsrClientCallServer(Request,
			       &Reply,
			       sizeof(CSRSS_API_REQUEST) + 
			       c * sizeof(WCHAR),
			       sizeof(CSRSS_API_REPLY));
  CloseHandle(hConsole);
  if (!NT_SUCCESS(Status) || !NT_SUCCESS( Status = Reply.Status ) )
    {
      RtlFreeHeap( GetProcessHeap(), 0, Request );
      SetLastErrorByStatus (Status);
      return(FALSE);
    }
  RtlFreeHeap( GetProcessHeap(), 0, Request );
  return TRUE;
}


/*--------------------------------------------------------------
 *	ReadConsoleW
 *
 * @unimplemented
 */
WINBASEAPI
BOOL
WINAPI
ReadConsoleW(
	HANDLE		hConsoleInput,
	LPVOID		lpBuffer,
	DWORD		nNumberOfCharsToRead,
	LPDWORD 	lpNumberOfCharsRead,
	LPVOID		lpReserved
	)
{
/* --- TO DO --- */
	return FALSE;
}


/*--------------------------------------------------------------
 *	WriteConsoleW
 *
 * @unimplemented
 */
WINBASEAPI
BOOL
WINAPI
WriteConsoleW(
	HANDLE		 hConsoleOutput,
	CONST VOID	*lpBuffer,
	DWORD		 nNumberOfCharsToWrite,
	LPDWORD		 lpNumberOfCharsWritten,
	LPVOID		 lpReserved
	)
{
#if 0
  PCSRSS_API_REQUEST Request;
  CSRSS_API_REPLY Reply;
  NTSTATUS Status;

  Request = RtlAllocateHeap(GetProcessHeap(),
			    HEAP_ZERO_MEMORY,
			    sizeof(CSRSS_API_REQUEST) + nNumberOfCharsToWrite * sizeof(WCHAR));
  if (Request == NULL)
    {
      SetLastError(ERROR_OUTOFMEMORY);
      return(FALSE);
    }

  Request->Type = CSRSS_WRITE_CONSOLE;
  Request->Data.WriteConsoleRequest.ConsoleHandle = hConsoleOutput;
  Request->Data.WriteConsoleRequest.NrCharactersToWrite =
    nNumberOfCharsToWrite;
//  DbgPrint("nNumberOfCharsToWrite %d\n", nNumberOfCharsToWrite);
//  DbgPrint("Buffer %s\n", Request->Data.WriteConsoleRequest.Buffer);
  memcpy(Request->Data.WriteConsoleRequest.Buffer,
	 lpBuffer,
	 nNumberOfCharsToWrite * sizeof(WCHAR));

  Status = CsrClientCallServer(Request,
			       &Reply,
			       sizeof(CSRSS_API_REQUEST) + nNumberOfCharsToWrite,
			       sizeof(CSRSS_API_REPLY));

  RtlFreeHeap(GetProcessHeap(),
	      0,
	      Request);

  if (!NT_SUCCESS(Status))
    {
      return(FALSE);
    }

  if (lpNumberOfCharsWritten != NULL)
    {
      *lpNumberOfCharsWritten = 
	  Reply.Data.WriteConsoleReply.NrCharactersWritten;
    }

  return(TRUE);
#endif
  return(FALSE);
}


/*--------------------------------------------------------------
 *	CreateConsoleScreenBuffer
 *
 * @implemented
 */
WINBASEAPI
HANDLE
WINAPI
CreateConsoleScreenBuffer(
	DWORD				 dwDesiredAccess,
	DWORD				 dwShareMode,
	CONST SECURITY_ATTRIBUTES	*lpSecurityAttributes,
	DWORD				 dwFlags,
	LPVOID				 lpScreenBufferData
	)
{
   // FIXME: don't ignore access, share mode, and security
   CSRSS_API_REQUEST Request;
   CSRSS_API_REPLY Reply;
   NTSTATUS Status;

   Request.Type = CSRSS_CREATE_SCREEN_BUFFER;
   Status = CsrClientCallServer( &Request, &Reply, sizeof( CSRSS_API_REQUEST ), sizeof( CSRSS_API_REPLY ) );
   if( !NT_SUCCESS( Status ) || !NT_SUCCESS( Status = Reply.Status ) )
      {
	 SetLastErrorByStatus ( Status );
	 return FALSE;
      }
   return Reply.Data.CreateScreenBufferReply.OutputHandle;
}


/*--------------------------------------------------------------
 *	GetConsoleCP
 *
 * @unimplemented
 */
WINBASEAPI
UINT
WINAPI
GetConsoleCP( VOID )
{
/* --- TO DO --- */
	return CP_OEMCP; /* FIXME */
}


/*--------------------------------------------------------------
 *	SetConsoleCP
 *
 * @unimplemented
 */
WINBASEAPI
BOOL
WINAPI
SetConsoleCP(
	UINT		wCodePageID
	)
{
/* --- TO DO --- */
	return FALSE;
}


/*--------------------------------------------------------------
 *	GetConsoleOutputCP
 *
 * @unimplemented
 */
WINBASEAPI
UINT
WINAPI
GetConsoleOutputCP( VOID )
{
/* --- TO DO --- */
	return 0; /* FIXME */
}


/*--------------------------------------------------------------
 *	SetConsoleOutputCP
 *
 * @unimplemented
 */
WINBASEAPI
BOOL
WINAPI
SetConsoleOutputCP(
	UINT		wCodePageID
	)
{
/* --- TO DO --- */
	return FALSE;
}


/*--------------------------------------------------------------
 * 	GetConsoleProcessList
 *
 * @unimplemented
 */
DWORD STDCALL
GetConsoleProcessList(LPDWORD lpdwProcessList,
                  DWORD dwProcessCount)
{
   SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
   return 0;
}



/*--------------------------------------------------------------
 * 	GetConsoleSelectionInfo
 *
 * @unimplemented
 */
BOOL STDCALL
GetConsoleSelectionInfo(PCONSOLE_SELECTION_INFO lpConsoleSelectionInfo)
{
   SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
   return FALSE;
}



/*--------------------------------------------------------------
 * 	AttachConsole
 *
 * @unimplemented
 */
BOOL STDCALL 
AttachConsole(DWORD dwProcessId)
{
   SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
   return FALSE;
}

/*--------------------------------------------------------------
 * 	GetConsoleWindow
 *
 * @implemented
 */
HWND STDCALL
GetConsoleWindow (VOID)
{
  CSRSS_API_REQUEST Request;
  CSRSS_API_REPLY   Reply;
  NTSTATUS          Status;
   
  Request.Data.ConsoleWindowRequest.ConsoleHandle =
    OpenConsoleW (L"CONOUT$", (GENERIC_READ|GENERIC_WRITE), FALSE, OPEN_EXISTING);
  if (INVALID_HANDLE_VALUE == Request.Data.ConsoleWindowRequest.ConsoleHandle)
  {
    return (HWND) NULL;
  }
  Request.Type = CSRSS_GET_CONSOLE_WINDOW;
  Status = CsrClientCallServer( &Request, &Reply, sizeof( CSRSS_API_REQUEST ), sizeof( CSRSS_API_REPLY ) );
  if (!NT_SUCCESS(Status ) || !NT_SUCCESS(Status = Reply.Status))
  {
    SetLastErrorByStatus (Status);
    return (HWND) NULL;
  }
  return Reply.Data.ConsoleWindowReply.WindowHandle;
}


/*--------------------------------------------------------------
 * 	GetConsoleWindow
 * @implemented
 */
BOOL STDCALL SetConsoleIcon(HICON hicon)
{
  CSRSS_API_REQUEST Request;
  CSRSS_API_REPLY   Reply;
  NTSTATUS          Status;
  
  Request.Data.ConsoleSetWindowIconRequest.ConsoleHandle =
    OpenConsoleW (L"CONOUT$", (GENERIC_READ|GENERIC_WRITE), FALSE, OPEN_EXISTING);
  if (INVALID_HANDLE_VALUE == Request.Data.ConsoleSetWindowIconRequest.ConsoleHandle)
  {
    return FALSE;
  }
  Request.Type = CSRSS_SET_CONSOLE_ICON;
  Request.Data.ConsoleSetWindowIconRequest.WindowIcon = hicon;
  Status = CsrClientCallServer( &Request, &Reply, sizeof( CSRSS_API_REQUEST ), sizeof( CSRSS_API_REPLY ) );
  if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Reply.Status))
  {
    SetLastErrorByStatus (Status);
    return FALSE;
  }
  return NT_SUCCESS(Status);
}

/* EOF */
