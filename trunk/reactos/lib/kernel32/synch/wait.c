/* $Id: wait.c,v 1.32 2004/12/04 19:31:26 navaraf Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/synch/wait.c
 * PURPOSE:         Wait functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/* INCLUDES *****************************************************************/

#include <k32.h>

#define NDEBUG
#include "../include/debug.h"


/* FUNCTIONS ****************************************************************/

DWORD STDCALL
GetConsoleInputWaitHandle (VOID);

/*
 * @implemented
 */
DWORD STDCALL
WaitForSingleObject(HANDLE hHandle,
		    DWORD dwMilliseconds)
{
   return WaitForSingleObjectEx(hHandle,
				dwMilliseconds,
				FALSE);
}


/*
 * @implemented
 */
DWORD STDCALL
WaitForSingleObjectEx(HANDLE hHandle,
                      DWORD  dwMilliseconds,
                      BOOL   bAlertable)
{
  PLARGE_INTEGER TimePtr;
  LARGE_INTEGER Time;
  NTSTATUS Status;

  /* Get real handle */
  switch ((ULONG)hHandle)
    {
      case STD_INPUT_HANDLE:
	hHandle = NtCurrentPeb()->ProcessParameters->hStdInput;
	break;

      case STD_OUTPUT_HANDLE:
	hHandle = NtCurrentPeb()->ProcessParameters->hStdOutput;
	break;

      case STD_ERROR_HANDLE:
	hHandle = NtCurrentPeb()->ProcessParameters->hStdError;
	break;
    }

  /* Check for console handle */
  if (IsConsoleHandle(hHandle))
    {
      if (!VerifyConsoleIoHandle(hHandle))
	{
	  SetLastError (ERROR_INVALID_HANDLE);
	  return WAIT_FAILED;
        }
	  
      hHandle = (HANDLE)GetConsoleInputWaitHandle();
      if (hHandle == NULL || hHandle == INVALID_HANDLE_VALUE)
        {
	  SetLastError (ERROR_INVALID_HANDLE);
	  return WAIT_FAILED;

	}
    }

  if (dwMilliseconds == INFINITE)
    {
      TimePtr = NULL;
    }
  else
    {
      Time.QuadPart = -10000 * (LONGLONG)dwMilliseconds;
      TimePtr = &Time;
    }

  Status = NtWaitForSingleObject(hHandle,
				 (BOOLEAN) bAlertable,
				 TimePtr);

  if (HIWORD(Status))
    {
      SetLastErrorByStatus (Status);
      return WAIT_FAILED;
    }

  return Status;
}


/*
 * @implemented
 */
DWORD STDCALL
WaitForMultipleObjects(DWORD nCount,
		       CONST HANDLE *lpHandles,
		       BOOL  bWaitAll,
		       DWORD dwMilliseconds)
{
  return WaitForMultipleObjectsEx(nCount,
				  lpHandles,
				  bWaitAll,
				  dwMilliseconds,
				  FALSE);
}


/*
 * @implemented
 */
DWORD STDCALL
WaitForMultipleObjectsEx(DWORD nCount,
                         CONST HANDLE *lpHandles,
                         BOOL  bWaitAll,
                         DWORD dwMilliseconds,
                         BOOL  bAlertable)
{
  PLARGE_INTEGER TimePtr;
  LARGE_INTEGER Time;
  PHANDLE HandleBuffer;
  HANDLE Handle[3];
  DWORD i;
  NTSTATUS Status;

  DPRINT("nCount %lu\n", nCount);

  if (nCount > 3)
    {
      HandleBuffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, nCount * sizeof(HANDLE));
      if (HandleBuffer == NULL)
        {
          SetLastError(ERROR_NOT_ENOUGH_MEMORY);
          return WAIT_FAILED;
        }
    }
  else
    {
      HandleBuffer = Handle;
    }
  for (i = 0; i < nCount; i++)
    {
      switch ((DWORD)lpHandles[i])
	{
	  case STD_INPUT_HANDLE:
	    HandleBuffer[i] = NtCurrentPeb()->ProcessParameters->hStdInput;
	    break;

	  case STD_OUTPUT_HANDLE:
	    HandleBuffer[i] = NtCurrentPeb()->ProcessParameters->hStdOutput;
	    break;

	  case STD_ERROR_HANDLE:
	    HandleBuffer[i] = NtCurrentPeb()->ProcessParameters->hStdError;
	    break;

	  default:
	    HandleBuffer[i] = lpHandles[i];
	    break;
	}

      /* Check for console handle */
      if (IsConsoleHandle(HandleBuffer[i]))
	{
	  if (!VerifyConsoleIoHandle(HandleBuffer[i]))
	    {
	      if (HandleBuffer != Handle)
	        {
	          RtlFreeHeap(GetProcessHeap(),0,HandleBuffer);
	        }
	      SetLastError (ERROR_INVALID_HANDLE);
	      return WAIT_FAILED;
	    }
	  HandleBuffer[i] = (HANDLE)GetConsoleInputWaitHandle();
	  if (HandleBuffer[i] == NULL || HandleBuffer[i] == INVALID_HANDLE_VALUE)
	    {
	      if (HandleBuffer != Handle)
	        {
	          RtlFreeHeap(GetProcessHeap(),0,HandleBuffer);
	        }
	      SetLastError (ERROR_INVALID_HANDLE);
	      return WAIT_FAILED;
	    }
	}
    }

  if (dwMilliseconds == INFINITE)
    {
      TimePtr = NULL;
    }
  else
    {
      Time.QuadPart = -10000 * (LONGLONG)dwMilliseconds;
      TimePtr = &Time;
    }

  Status = NtWaitForMultipleObjects (nCount,
				     HandleBuffer,
				     bWaitAll  ? WaitAll : WaitAny,
				     (BOOLEAN)bAlertable,
				     TimePtr);
  if (HandleBuffer != Handle)
    {
      RtlFreeHeap(RtlGetProcessHeap(), 0, HandleBuffer);
    }

  if (HIWORD(Status))
    {
      SetLastErrorByStatus (Status);
      return WAIT_FAILED;
    }

  return Status;
}


/*
 * @implemented
 */
DWORD STDCALL
SignalObjectAndWait(HANDLE hObjectToSignal,
		    HANDLE hObjectToWaitOn,
		    DWORD dwMilliseconds,
		    BOOL bAlertable)
{
  PLARGE_INTEGER TimePtr;
  LARGE_INTEGER Time;
  NTSTATUS Status;

  /* Get real handle */
  switch ((ULONG)hObjectToWaitOn)
    {
      case STD_INPUT_HANDLE:
	hObjectToWaitOn = NtCurrentPeb()->ProcessParameters->hStdInput;
	break;

      case STD_OUTPUT_HANDLE:
	hObjectToWaitOn = NtCurrentPeb()->ProcessParameters->hStdOutput;
	break;

      case STD_ERROR_HANDLE:
	hObjectToWaitOn = NtCurrentPeb()->ProcessParameters->hStdError;
	break;
    }

  /* Check for console handle */
  if (IsConsoleHandle(hObjectToWaitOn))
    {
      if (VerifyConsoleIoHandle(hObjectToWaitOn))
	{
	  DPRINT1("Console handles are not supported yet!\n");
	  SetLastError(ERROR_INVALID_HANDLE);
	  return FALSE;
	}
    }

  if (dwMilliseconds == INFINITE)
    {
      TimePtr = NULL;
    }
  else
    {
      Time.QuadPart = -10000 * (LONGLONG)dwMilliseconds;
      TimePtr = &Time;
    }

  Status = NtSignalAndWaitForSingleObject (hObjectToSignal,
					   hObjectToWaitOn,
					   (BOOLEAN)bAlertable,
					   TimePtr);
  if (!NT_SUCCESS(Status))
    {
      SetLastErrorByStatus (Status);
      return FALSE;
    }

  return TRUE;
}

/* EOF */
