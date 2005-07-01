/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            subsys/csrss/win32csr/dllmain.c
 * PURPOSE:         Initialization
 */

/* INCLUDES ******************************************************************/

#include "w32csr.h"

#define NDEBUG
#include <debug.h>

/* Not defined in any header file */
extern VOID STDCALL PrivateCsrssManualGuiCheck(LONG Check);
extern VOID STDCALL PrivateCsrssInitialized();

/* GLOBALS *******************************************************************/

HANDLE Win32CsrApiHeap;
HINSTANCE Win32CsrDllHandle;
static CSRSS_EXPORTED_FUNCS CsrExports;

static CSRSS_API_DEFINITION Win32CsrApiDefinitions[] =
  {
    CSRSS_DEFINE_API(WRITE_CONSOLE,                CsrWriteConsole),
    CSRSS_DEFINE_API(READ_CONSOLE,                 CsrReadConsole),
    CSRSS_DEFINE_API(ALLOC_CONSOLE,                CsrAllocConsole),
    CSRSS_DEFINE_API(FREE_CONSOLE,                 CsrFreeConsole),
    CSRSS_DEFINE_API(SCREEN_BUFFER_INFO,           CsrGetScreenBufferInfo),
    CSRSS_DEFINE_API(SET_CURSOR,                   CsrSetCursor),
    CSRSS_DEFINE_API(FILL_OUTPUT,                  CsrFillOutputChar),
    CSRSS_DEFINE_API(READ_INPUT,                   CsrReadInputEvent),
    CSRSS_DEFINE_API(WRITE_CONSOLE_OUTPUT_CHAR,    CsrWriteConsoleOutputChar),
    CSRSS_DEFINE_API(WRITE_CONSOLE_OUTPUT_ATTRIB,  CsrWriteConsoleOutputAttrib),
    CSRSS_DEFINE_API(FILL_OUTPUT_ATTRIB,           CsrFillOutputAttrib),
    CSRSS_DEFINE_API(GET_CURSOR_INFO,              CsrGetCursorInfo),
    CSRSS_DEFINE_API(SET_CURSOR_INFO,              CsrSetCursorInfo),
    CSRSS_DEFINE_API(SET_ATTRIB,                   CsrSetTextAttrib),
    CSRSS_DEFINE_API(GET_CONSOLE_MODE,             CsrGetConsoleMode),
    CSRSS_DEFINE_API(SET_CONSOLE_MODE,             CsrSetConsoleMode),
    CSRSS_DEFINE_API(CREATE_SCREEN_BUFFER,         CsrCreateScreenBuffer),
    CSRSS_DEFINE_API(SET_SCREEN_BUFFER,            CsrSetScreenBuffer),
    CSRSS_DEFINE_API(SET_TITLE,                    CsrSetTitle),
    CSRSS_DEFINE_API(GET_TITLE,                    CsrGetTitle),
    CSRSS_DEFINE_API(WRITE_CONSOLE_OUTPUT,         CsrWriteConsoleOutput),
    CSRSS_DEFINE_API(FLUSH_INPUT_BUFFER,           CsrFlushInputBuffer),
    CSRSS_DEFINE_API(SCROLL_CONSOLE_SCREEN_BUFFER, CsrScrollConsoleScreenBuffer),
    CSRSS_DEFINE_API(READ_CONSOLE_OUTPUT_CHAR,     CsrReadConsoleOutputChar),
    CSRSS_DEFINE_API(READ_CONSOLE_OUTPUT_ATTRIB,   CsrReadConsoleOutputAttrib),
    CSRSS_DEFINE_API(GET_NUM_INPUT_EVENTS,         CsrGetNumberOfConsoleInputEvents),
    CSRSS_DEFINE_API(EXIT_REACTOS,                 CsrExitReactos),
    CSRSS_DEFINE_API(PEEK_CONSOLE_INPUT,           CsrPeekConsoleInput),
    CSRSS_DEFINE_API(READ_CONSOLE_OUTPUT,          CsrReadConsoleOutput),
    CSRSS_DEFINE_API(WRITE_CONSOLE_INPUT,          CsrWriteConsoleInput),
    CSRSS_DEFINE_API(SETGET_CONSOLE_HW_STATE,      CsrHardwareStateProperty),
    CSRSS_DEFINE_API(GET_CONSOLE_WINDOW,           CsrGetConsoleWindow),
    CSRSS_DEFINE_API(CREATE_DESKTOP,               CsrCreateDesktop),
    CSRSS_DEFINE_API(SHOW_DESKTOP,                 CsrShowDesktop),
    CSRSS_DEFINE_API(HIDE_DESKTOP,                 CsrHideDesktop),
    CSRSS_DEFINE_API(SET_CONSOLE_ICON,             CsrSetConsoleIcon),
    CSRSS_DEFINE_API(SET_LOGON_NOTIFY_WINDOW,      CsrSetLogonNotifyWindow),
    CSRSS_DEFINE_API(REGISTER_LOGON_PROCESS,       CsrRegisterLogonProcess),
    CSRSS_DEFINE_API(GET_CONSOLE_CP,               CsrGetConsoleCodePage),
    CSRSS_DEFINE_API(SET_CONSOLE_CP,               CsrSetConsoleCodePage),
    CSRSS_DEFINE_API(GET_CONSOLE_OUTPUT_CP,        CsrGetConsoleOutputCodePage),
    CSRSS_DEFINE_API(SET_CONSOLE_OUTPUT_CP,        CsrSetConsoleOutputCodePage),
    CSRSS_DEFINE_API(GET_PROCESS_LIST,             CsrGetProcessList),
    { 0, 0, NULL }
  };

static CSRSS_OBJECT_DEFINITION Win32CsrObjectDefinitions[] =
  {
    { CONIO_CONSOLE_MAGIC,       ConioDeleteConsole },
    { CONIO_SCREEN_BUFFER_MAGIC, ConioDeleteScreenBuffer },
    { 0,                         NULL }
  };

/* FUNCTIONS *****************************************************************/

BOOL STDCALL
DllMain(HANDLE hDll,
	DWORD dwReason,
	LPVOID lpReserved)
{
  if (DLL_PROCESS_ATTACH == dwReason)
    {
      Win32CsrDllHandle = hDll;
    }

  return TRUE;
}

NTSTATUS FASTCALL
Win32CsrInsertObject(PCSRSS_PROCESS_DATA ProcessData,
                     PHANDLE Handle,
                     Object_t *Object)
{
  InitializeCriticalSection(&(Object->Lock));

  return (CsrExports.CsrInsertObjectProc)(ProcessData, Handle, Object);
}

NTSTATUS FASTCALL
Win32CsrGetObject(PCSRSS_PROCESS_DATA ProcessData,
                 HANDLE Handle,
                 Object_t **Object)
{
  return (CsrExports.CsrGetObjectProc)(ProcessData, Handle, Object);
}

NTSTATUS FASTCALL
Win32CsrLockObject(PCSRSS_PROCESS_DATA ProcessData,
                   HANDLE Handle,
                   Object_t **Object,
                   LONG Type)
{
  NTSTATUS Status;

  Status = (CsrExports.CsrGetObjectProc)(ProcessData, Handle, Object);
  if (! NT_SUCCESS(Status))
    {
      return Status;
    }

  if ((*Object)->Type != Type)
    {
      return STATUS_INVALID_HANDLE;
    }

  EnterCriticalSection(&((*Object)->Lock));

  return STATUS_SUCCESS;
}

VOID FASTCALL
Win32CsrUnlockObject(Object_t *Object)
{
  LeaveCriticalSection(&(Object->Lock));
}

NTSTATUS FASTCALL
Win32CsrReleaseObject(PCSRSS_PROCESS_DATA ProcessData,
                     HANDLE Object)
{
  return (CsrExports.CsrReleaseObjectProc)(ProcessData, Object);
}

static BOOL STDCALL
Win32CsrInitComplete(void)
{
  PrivateCsrssInitialized();

  return TRUE;
}

BOOL STDCALL
Win32CsrInitialization(PCSRSS_API_DEFINITION *ApiDefinitions,
                       PCSRSS_OBJECT_DEFINITION *ObjectDefinitions,
                       CSRPLUGIN_INIT_COMPLETE_PROC *InitComplete,
                       PCSRSS_EXPORTED_FUNCS Exports,
                       HANDLE CsrssApiHeap)
{
  HANDLE ThreadHandle;

  CsrExports = *Exports;
  Win32CsrApiHeap = CsrssApiHeap;

  PrivateCsrssManualGuiCheck(0);
  CsrInitConsoleSupport();
  ThreadHandle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) Console_Api, NULL, 0, NULL);
  if (NULL == ThreadHandle)
    {
      DPRINT1("CSR: Unable to create console thread\n");
      return FALSE;
    }
  CloseHandle(ThreadHandle);

  *ApiDefinitions = Win32CsrApiDefinitions;
  *ObjectDefinitions = Win32CsrObjectDefinitions;
  *InitComplete = Win32CsrInitComplete;

  return TRUE;
}

/* EOF */
