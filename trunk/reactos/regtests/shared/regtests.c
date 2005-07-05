/*
 * PROJECT:         ReactOS kernel
 * FILE:            regtests/shared/regtests.c
 * PURPOSE:         Regression testing framework
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      06-07-2003  CSH  Created
 */
#include <windows.h>
#define NTOS_MODE_USER
#include <ndk/ntndk.h>
#include <pseh/pseh.h>
#include "regtests.h"

#define NDEBUG
#include <debug.h>

typedef struct _PERFORM_TEST_ARGS
{
  TestOutputRoutine OutputRoutine;
  _PTEST Test;
  LPSTR TestName;
  DWORD Result;
  char Buffer[5000];
  DWORD Time;
} PERFORM_TEST_ARGS;

int _Result;
char *_Buffer;

static LIST_ENTRY AllTests;

VOID
InitializeTests()
{
  InitializeListHead(&AllTests);
}

char*
FormatExecutionTime(char *buffer, ULONG milliseconds)
{
  sprintf(buffer,
	  "%ldms",
	  milliseconds);
  return buffer;
}

DWORD WINAPI
PerformTest(PVOID _arg)
{
  PERFORM_TEST_ARGS *Args = (PERFORM_TEST_ARGS *)_arg;
  _PTEST Test = Args->Test;

  _SetThreadPriority(_GetCurrentThread(), THREAD_PRIORITY_IDLE);

  memset(Args->Buffer, 0, sizeof(Args->Buffer));

  _SEH_TRY {
    _Result = TS_OK;
    _Buffer = Args->Buffer;
    (Test->Routine)(TESTCMD_RUN);
    Args->Result = _Result;
  } _SEH_HANDLE {
    Args->Result = TS_FAILED;
    sprintf(Args->Buffer, "due to exception 0x%lx", _SEH_GetExceptionCode());
  } _SEH_END;
  return 1;
}

BOOL
IsContextChanged(LPCONTEXT context1, LPCONTEXT context2)
{
  return memcmp(context1, context2, sizeof(CONTEXT)) != 0;
}

VOID
ControlNormalTest(HANDLE hThread,
            PERFORM_TEST_ARGS *Args,
            DWORD TimeOut)
{
  FILETIME time;
  FILETIME executionTime;
  DWORD status;

  status = _WaitForSingleObject(hThread, TimeOut);
  if (status == WAIT_TIMEOUT)
  {
    _TerminateThread(hThread, 0);
    Args->Result = TS_TIMEDOUT;
  }
  status = _GetThreadTimes(hThread,
                          &time,
                          &time,
                          &time,
                          &executionTime);
  Args->Time = executionTime.dwLowDateTime / 10000;
}

VOID
ControlPerformanceTest(HANDLE hThread,
            PERFORM_TEST_ARGS *Args,
            DWORD TimeOut)
{
  DWORD status;
  CONTEXT lastContext;
  CONTEXT currentContext;

  ZeroMemory(&lastContext, sizeof(CONTEXT));
  lastContext.ContextFlags = CONTEXT_FULL;
  ZeroMemory(&currentContext, sizeof(CONTEXT));
  currentContext.ContextFlags = CONTEXT_FULL;

  do {
    _Sleep(1);

    if (_SuspendThread(hThread) == (DWORD)-1)
      break;

    if (_GetThreadContext(hThread, &currentContext) == 0)
      break;

   if (IsContextChanged(&currentContext, &lastContext))
     Args->Time++;

    if (_ResumeThread(hThread) == (DWORD)-1)
      break;

    if (Args->Time >= TimeOut)
      {
        _TerminateThread(hThread, 0);
        Args->Result = TS_TIMEDOUT;
        break;
      }

    status = _WaitForSingleObject(hThread, 0);
    if (status == WAIT_OBJECT_0 || status == WAIT_FAILED)
      break;

    lastContext = currentContext;
  } while (TRUE);
}

VOID
DisplayResult(PERFORM_TEST_ARGS* Args,
              LPSTR OutputBuffer)
{
  char Buffer[5000];
  char Format[100];

  if (Args->Result == TS_OK)
    {
      sprintf(OutputBuffer,
              "[%s] Success [%s]\n",
              Args->TestName,
              FormatExecutionTime(Format,
                                  Args->Time));
    }
  else if (Args->Result == TS_TIMEDOUT)
    {
      sprintf(OutputBuffer,
              "[%s] Timed out [%s]\n",
              Args->TestName,
              FormatExecutionTime(Format,
                                  Args->Time));
    }
  else
      sprintf(OutputBuffer, "[%s] Failed (%s)\n", Args->TestName, Buffer);

  if (Args->OutputRoutine != NULL)
    (*Args->OutputRoutine)(OutputBuffer);
  else
    DbgPrint(OutputBuffer);
}

VOID
ControlTest(HANDLE hThread,
            PERFORM_TEST_ARGS *Args,
            DWORD TestType,
            DWORD TimeOut)
{
  switch (TestType)
  {
    case TT_NORMAL:
      ControlNormalTest(hThread, Args, TimeOut);
      break;
    case TT_PERFORMANCE:
      ControlPerformanceTest(hThread, Args, TimeOut);
      break;
    default:
      printf("Unknown test type %ld\n", TestType);
      break;
  }
}

VOID
PerformTests(TestOutputRoutine OutputRoutine, LPSTR TestName)
{
  PLIST_ENTRY CurrentEntry;
  PLIST_ENTRY NextEntry;
  _PTEST Current;
  PERFORM_TEST_ARGS Args;
  HANDLE hThread;
  char OutputBuffer[1024];
  char Name[200];
  DWORD TestType;
  DWORD TimeOut;

  Args.OutputRoutine = OutputRoutine;
  Args.TestName = Name;
  Args.Time = 0;

  CurrentEntry = AllTests.Flink;
  for (; CurrentEntry != &AllTests; CurrentEntry = NextEntry)
    {
      NextEntry = CurrentEntry->Flink;
      Current = CONTAINING_RECORD(CurrentEntry, _TEST, ListEntry);
      Args.Test = Current;

      /* Get name of test */
      memset(Name, 0, sizeof(Name));

      _Result = TS_OK;
      _Buffer = Name;
      (Current->Routine)(TESTCMD_TESTNAME);
      if (_Result != TS_OK)
        {
          if (TestName != NULL)
            continue;
          strcpy(Name, "Unnamed");
        }

      if ((TestName != NULL) && (_stricmp(Name, TestName) != 0))
        continue;

      TestType = TT_NORMAL;
      _Result = TS_OK;
      _Buffer = (char *)&TestType;
      (Current->Routine)(TESTCMD_TESTTYPE);
      if (_Result != TS_OK)
        TestType = TT_NORMAL;

      /* Get timeout for test */
      TimeOut = 0;
      _Result = TS_OK;
      _Buffer = (char *)&TimeOut;
      (Current->Routine)(TESTCMD_TIMEOUT);
      if (_Result != TS_OK || TimeOut == INFINITE)
        TimeOut = 5000;

      /* Run test in a separate thread */
      hThread = _CreateThread(NULL, 0, PerformTest, (PVOID)&Args, 0, NULL);
      if (hThread == NULL)
        {
          printf("[%s] Failed (CreateThread() failed: %ld)\n",
                 Name,
                 _GetLastError());
          Args.Result = TS_FAILED;
        }
      else
        ControlTest(hThread, &Args, TestType, TimeOut);

      DisplayResult(&Args, OutputBuffer);
    }
}

VOID
AddTest(TestRoutine Routine)
{
  _PTEST Test;

  Test = (_PTEST) malloc(sizeof(_TEST));
  if (Test == NULL)
    {
      DbgPrint("Out of memory");
      return;
    }

  Test->Routine = Routine;

  InsertTailList(&AllTests, &Test->ListEntry);
}

PVOID STDCALL
FrameworkGetHook(ULONG index)
{
  return FrameworkGetHookInternal(index);
}
