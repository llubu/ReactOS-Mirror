#ifndef _SMSS_H_INCLUDED_
#define _SMSS_H_INCLUDED_

#define NTOS_MODE_USER
#include <ntos.h>
#include <sm/api.h>
#include <sm/helper.h>

/* smss.c */
extern ULONG SmSsProcessId;

/* init.c */
NTSTATUS InitSessionManager(VOID);

/* initheap.c */
extern HANDLE SmpHeap;
NTSTATUS SmCreateHeap(VOID);

/* initenv.c */
extern PWSTR SmSystemEnvironment;
NTSTATUS SmCreateEnvironment(VOID);
NTSTATUS SmSetEnvironmentVariables(VOID);
NTSTATUS SmUpdateEnvironment(VOID);

/* initobdir.c */
NTSTATUS SmCreateObjectDirectories(VOID);

/* initdosdev.c */
NTSTATUS SmInitDosDevices(VOID);

/* initrun.c */
extern HANDLE Children[2];
NTSTATUS SmRunBootApplications(VOID);

/* initmv.c */
NTSTATUS SmProcessFileRenameList(VOID);

/* initwkdll.c */
NTSTATUS SmLoadKnownDlls(VOID);

/* initpage.c */
NTSTATUS SmCreatePagingFiles(VOID);

/* initreg.c */
NTSTATUS SmInitializeRegistry(VOID);

/* initss.c */
NTSTATUS SmLoadSubsystems(VOID);

/* smapi.c */
#define SMAPI(n) \
NTSTATUS FASTCALL n (PSM_PORT_MESSAGE Request)
PSM_CONNECT_DATA FASTCALL SmpGetConnectData (PSM_PORT_MESSAGE);
NTSTATUS SmCreateApiPort(VOID);
VOID STDCALL SmpApiThread(PVOID);


/* smapiexec.c */
NTSTATUS STDCALL SmCreateUserProcess(LPWSTR ImagePath,
				     LPWSTR CommandLine,
				     BOOLEAN WaitForIt,
				     PLARGE_INTEGER Timeout OPTIONAL,
				     PRTL_PROCESS_INFO UserProcessInfo OPTIONAL);
NTSTATUS FASTCALL SmExecPgm(PSM_PORT_MESSAGE);

/* smapicomp.c */
NTSTATUS FASTCALL SmCompSes(PSM_PORT_MESSAGE);

/* smapiquery.c */
NTSTATUS FASTCALL SmQryInfo(PSM_PORT_MESSAGE);

/* client.c */
#define SM_CLIENT_FLAG_CANDIDATE   0x8000
#define SM_CLIENT_FLAG_INITIALIZED 0x0001
#define SM_CLIENT_FLAG_REQUIRED    0x0002
typedef struct _SM_CLIENT_DATA
{
  RTL_CRITICAL_SECTION  Lock;
  WCHAR                 ProgramName [SM_SB_NAME_MAX_LENGTH];
  USHORT                SubsystemId;
  WORD                  Flags;
  WORD                  Unused;
  ULONG                 ServerProcessId;
  HANDLE	        ServerProcess;
  HANDLE	        ApiPort;
  HANDLE	        ApiPortThread;
  HANDLE	        SbApiPort;
  WCHAR	                SbApiPortName [SM_SB_NAME_MAX_LENGTH];
  struct _SM_CLIENT_DATA * Next;

} SM_CLIENT_DATA, *PSM_CLIENT_DATA;
NTSTATUS SmInitializeClientManagement (VOID);
NTSTATUS STDCALL SmCreateClient (PRTL_PROCESS_INFO,PWSTR);
NTSTATUS STDCALL SmDestroyClient (ULONG);
NTSTATUS STDCALL SmBeginClientInitialization (PSM_PORT_MESSAGE,PSM_CLIENT_DATA*);
NTSTATUS STDCALL SmCompleteClientInitialization (ULONG);
NTSTATUS FASTCALL SmGetClientBasicInformation (PSM_BASIC_INFORMATION);
NTSTATUS FASTCALL SmGetSubSystemInformation (PSM_SUBSYSTEM_INFORMATION);

/* debug.c */
extern HANDLE DbgSsApiPort;
extern HANDLE DbgUiApiPort;
NTSTATUS SmInitializeDbgSs(VOID);

/* print.c */
VOID STDCALL DisplayString(LPCWSTR lpwString);
VOID STDCALL PrintString (char* fmt, ...);

#endif /* _SMSS_H_INCLUDED_ */

/* EOF */

