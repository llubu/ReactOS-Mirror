/* $Id: psfuncs.h,v 1.17 2002/01/03 14:01:16 ekohl Exp $
 */
#ifndef _INCLUDE_DDK_PSFUNCS_H
#define _INCLUDE_DDK_PSFUNCS_H

NTSTATUS STDCALL PsAssignImpersonationToken (struct _ETHREAD* Thread,
					     HANDLE TokenHandle);

HANDLE STDCALL PsGetCurrentProcessId(VOID);
HANDLE STDCALL PsGetCurrentThreadId(VOID);

/*
 * FUNCTION: Creates a thread which executes in kernel mode
 * ARGUMENTS:
 *       ThreadHandle (OUT) = Caller supplied storage for the returned thread 
 *                            handle
 *       DesiredAccess = Requested access to the thread
 *       ObjectAttributes = Object attributes (optional)
 *       ProcessHandle = Handle of process thread will run in
 *                       NULL to use system process
 *       ClientId (OUT) = Caller supplied storage for the returned client id
 *                        of the thread (optional)
 *       StartRoutine = Entry point for the thread
 *       StartContext = Argument supplied to the thread when it begins
 *                     execution
 * RETURNS: Success or failure status
 */
NTSTATUS STDCALL PsCreateSystemThread(PHANDLE ThreadHandle,
				      ACCESS_MASK DesiredAccess,
				      POBJECT_ATTRIBUTES ObjectAttributes,
				      HANDLE ProcessHandle,
				      PCLIENT_ID ClientId,
				      PKSTART_ROUTINE StartRoutine,
				      PVOID StartContext);
NTSTATUS STDCALL PsTerminateSystemThread(NTSTATUS ExitStatus);

NTSTATUS STDCALL PsCreateSystemProcess(PHANDLE ProcessHandle,
				       ACCESS_MASK DesiredAccess,
				       POBJECT_ATTRIBUTES ObjectAttributes);

NTSTATUS STDCALL PsCreateWin32Process(PEPROCESS Process);

VOID STDCALL PsEstablishWin32Callouts(PVOID Param1,
				      PVOID Param2,
				      PVOID Param3,
				      PVOID Param4,
				      PVOID Param5,
				      ULONG W32ProcessSize);

struct _ETHREAD* STDCALL PsGetCurrentThread(VOID);
struct _EPROCESS* STDCALL PsGetCurrentProcess(VOID);
PACCESS_TOKEN STDCALL PsReferenceImpersonationToken(struct _ETHREAD* Thread,
						    PULONG Unknown1,
						    PULONG Unknown2,
						    SECURITY_IMPERSONATION_LEVEL* 
						    Level);
PACCESS_TOKEN STDCALL PsReferencePrimaryToken(struct _EPROCESS* Process);
NTSTATUS STDCALL PsAssignImpersonationToken(struct _ETHREAD* Thread,
					    HANDLE TokenHandle);

VOID STDCALL PsImpersonateClient(struct _ETHREAD* Thread,
				 PACCESS_TOKEN Token,
				 UCHAR b,
				 UCHAR c,
				 SECURITY_IMPERSONATION_LEVEL Level);
VOID STDCALL PsRevertToSelf(VOID);

BOOLEAN STDCALL PsGetVersion (PULONG		MajorVersion	OPTIONAL,
			      PULONG		MinorVersion	OPTIONAL,
			      PULONG		BuildNumber	OPTIONAL,
			      PUNICODE_STRING	CSDVersion	OPTIONAL);

VOID STDCALL PsDispatchThread(ULONG NewThreadStatus);

LARGE_INTEGER STDCALL PsGetProcessExitTime(VOID);
BOOLEAN STDCALL PsIsThreadTerminating(struct _ETHREAD* Thread);

NTSTATUS STDCALL PsLookupProcessByProcessId(IN PVOID ProcessId,
					    OUT PEPROCESS *Process);

NTSTATUS STDCALL PsLookupProcessThreadByCid(IN PCLIENT_ID Cid,
					    OUT PEPROCESS *Process OPTIONAL,
					    OUT struct _ETHREAD **Thread);
//					    OUT PETHREAD *Thread);

NTSTATUS STDCALL PsLookupThreadByThreadId(IN PVOID ThreadId,
					  OUT struct _ETHREAD **Thread);
//					  OUT PETHREAD *Thread);

#endif

/* EOF */
