#ifndef __INCLUDE_NAPI_DBG_H
#define __INCLUDE_NAPI_DBG_H

#define DBG_EVENT_EXCEPTION         (1)
#define DBG_EVENT_CREATE_THREAD     (2)
#define DBG_EVENT_CREATE_PROCESS    (3)
#define DBG_EVENT_EXIT_THREAD       (4)
#define DBG_EVENT_EXIT_PROCESS      (5)
#define DBG_EVENT_LOAD_DLL          (6)
#define DBG_EVENT_UNLOAD_DLL        (7)

typedef struct _LPC_DBG_MESSAGE
{
   LPC_MESSAGE Header;
   ULONG Type;
   ULONG Status;
   union
     {
	struct
	  {
	     EXCEPTION_RECORD ExceptionRecord;
	     ULONG FirstChance;
	  } Exception;
	struct
	  {
	     ULONG Reserved;
	     PVOID StartAddress;
	  } CreateThread;
	struct
	  {
	     ULONG Reserved;
	     HANDLE FileHandle;
	     PVOID Base;
	     ULONG PointerToSymbolTable;
	     ULONG NumberOfSymbols;
	     ULONG Reserved2;
	     PVOID EntryPoint;
	  } CreateProcess;
	struct
	  {
	     ULONG ExitCode;
	  } ExitThread;
	struct
	  {
	     ULONG ExitCode;
	  } ExitProcess;
	struct
	  {
	     HANDLE FileHandle;
	     PVOID Base;
	     ULONG PointerToSymbolTable;
	     ULONG NumberOfSymbols;
	  } LoadDll;
	struct
	  {
	     PVOID Base;
	  } UnloadDll;
     } Data;
} LPC_DBG_MESSAGE, *PLPC_DBG_MESSAGE;


typedef struct _LPC_TERMINATION_MESSAGE
{
   LPC_MESSAGE Header;
   LARGE_INTEGER CreationTime;
} LPC_TERMINATION_MESSAGE, *PLPC_TERMINATION_MESSAGE;

#endif /* __INCLUDE_NAPI_DBG_H */
