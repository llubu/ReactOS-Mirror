/* $Id: thread.c,v 1.30 2002/10/01 19:27:20 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/thread/thread.c
 * PURPOSE:         Thread functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 *			Tls functions are modified from WINE
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <windows.h>
#include <kernel32/thread.h>
#include <ntdll/ldr.h>
#include <string.h>
#include <napi/i386/segment.h>

#define NDEBUG
#include <kernel32/kernel32.h>
#include <kernel32/error.h>


static VOID ThreadAttachDlls (VOID);

/* FUNCTIONS *****************************************************************/

static VOID STDCALL
ThreadStartup (LPTHREAD_START_ROUTINE lpStartAddress,
               LPVOID lpParameter)
{
   UINT uExitCode;

   /* FIXME: notify csrss of thread creation ?? */

   uExitCode = (lpStartAddress)(lpParameter);

   ExitThread(uExitCode);
}

HANDLE STDCALL CreateThread(LPSECURITY_ATTRIBUTES lpThreadAttributes,
			    DWORD dwStackSize,
			    LPTHREAD_START_ROUTINE lpStartAddress,
			    LPVOID lpParameter,
			    DWORD dwCreationFlags,
			    LPDWORD lpThreadId)
{
   return(CreateRemoteThread(NtCurrentProcess(),
			     lpThreadAttributes,
			     dwStackSize,
			     lpStartAddress,
			     lpParameter,
			     dwCreationFlags,
			     lpThreadId));
}

HANDLE STDCALL CreateRemoteThread(HANDLE hProcess,
				  LPSECURITY_ATTRIBUTES lpThreadAttributes,
				  DWORD dwStackSize,
				  LPTHREAD_START_ROUTINE lpStartAddress,
				  LPVOID lpParameter,
				  DWORD dwCreationFlags,
				  LPDWORD lpThreadId)
{
   HANDLE ThreadHandle;
   OBJECT_ATTRIBUTES ObjectAttributes;
   CLIENT_ID ClientId;
   CONTEXT ThreadContext;
   INITIAL_TEB InitialTeb;
   BOOLEAN CreateSuspended = FALSE;
   PVOID BaseAddress;
   ULONG OldPageProtection;
   NTSTATUS Status;
   
   ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
   ObjectAttributes.RootDirectory = NULL;
   ObjectAttributes.ObjectName = NULL;
   ObjectAttributes.Attributes = 0;
   if (lpThreadAttributes != NULL) 
     {
	if (lpThreadAttributes->bInheritHandle)
	  ObjectAttributes.Attributes = OBJ_INHERIT;
	ObjectAttributes.SecurityDescriptor = 
	  lpThreadAttributes->lpSecurityDescriptor;
     }
   ObjectAttributes.SecurityQualityOfService = NULL;
   
   if ((dwCreationFlags & CREATE_SUSPENDED) == CREATE_SUSPENDED)
     CreateSuspended = TRUE;
   else
     CreateSuspended = FALSE;

  InitialTeb.StackReserve = 0x100000; /* 1MByte */
  /* FIXME: use correct commit size */
#if 0
  InitialTeb.StackCommit = (dwStackSize == 0) ? PAGE_SIZE : dwStackSize;
#endif
  InitialTeb.StackCommit = InitialTeb.StackReserve - PAGE_SIZE;

  /* size of guard page */
  InitialTeb.StackCommit += PAGE_SIZE;

  /* Reserve stack */
  InitialTeb.StackAllocate = NULL;
  Status = NtAllocateVirtualMemory(hProcess,
				   &InitialTeb.StackAllocate,
				   0,
				   &InitialTeb.StackReserve,
				   MEM_RESERVE,
				   PAGE_READWRITE);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("Error reserving stack space!\n");
      SetLastErrorByStatus(Status);
      return(NULL);
    }

  DPRINT("StackDeallocation: %p ReserveSize: 0x%lX\n",
	 InitialTeb.StackDeallocation, InitialTeb.StackReserve);

  InitialTeb.StackBase = (PVOID)((ULONG)InitialTeb.StackAllocate + InitialTeb.StackReserve);
  InitialTeb.StackLimit = (PVOID)((ULONG)InitialTeb.StackBase - InitialTeb.StackCommit);

  DPRINT("StackBase: %p\nStackCommit: 0x%lX\n",
	 InitialTeb.StackBase,
	 InitialTeb.StackCommit);

  /* Commit stack pages */
  Status = NtAllocateVirtualMemory(hProcess,
				   &InitialTeb.StackLimit,
				   0,
				   &InitialTeb.StackCommit,
				   MEM_COMMIT,
				   PAGE_READWRITE);
  if (!NT_SUCCESS(Status))
    {
      /* release the stack space */
      NtFreeVirtualMemory(hProcess,
			  InitialTeb.StackAllocate,
			  &InitialTeb.StackReserve,
			  MEM_RELEASE);

      DPRINT("Error comitting stack page(s)!\n");
      SetLastErrorByStatus(Status);
      return(NULL);
    }

  DPRINT("StackLimit: %p\n",
	 InitialTeb.StackLimit);

  /* Protect guard page */
  Status = NtProtectVirtualMemory(hProcess,
				  InitialTeb.StackLimit,
				  PAGE_SIZE,
				  PAGE_GUARD | PAGE_READWRITE,
				  &OldPageProtection);
  if (!NT_SUCCESS(Status))
    {
      /* release the stack space */
      NtFreeVirtualMemory(hProcess,
			  InitialTeb.StackAllocate,
			  &InitialTeb.StackReserve,
			  MEM_RELEASE);

      DPRINT("Error comitting guard page!\n");
      SetLastErrorByStatus(Status);
      return(NULL);
    }

  memset(&ThreadContext,0,sizeof(CONTEXT));
  ThreadContext.Eip = (LONG)ThreadStartup;
  ThreadContext.SegGs = USER_DS;
  ThreadContext.SegFs = TEB_SELECTOR;
  ThreadContext.SegEs = USER_DS;
  ThreadContext.SegDs = USER_DS;
  ThreadContext.SegCs = USER_CS;
  ThreadContext.SegSs = USER_DS;
  ThreadContext.Esp = (ULONG)InitialTeb.StackBase - 12;
  ThreadContext.EFlags = (1<<1) + (1<<9);

  /* initialize call stack */
  *((PULONG)((ULONG)InitialTeb.StackBase - 4)) = (ULONG)lpParameter;
  *((PULONG)((ULONG)InitialTeb.StackBase - 8)) = (ULONG)lpStartAddress;
  *((PULONG)((ULONG)InitialTeb.StackBase - 12)) = 0xdeadbeef;

  DPRINT("Esp: %p\n", ThreadContext.Esp);
  DPRINT("Eip: %p\n", ThreadContext.Eip);

  Status = NtCreateThread(&ThreadHandle,
			  THREAD_ALL_ACCESS,
			  &ObjectAttributes,
			  hProcess,
			  &ClientId,
			  &ThreadContext,
			  &InitialTeb,
			  CreateSuspended);
  if (!NT_SUCCESS(Status))
    {
      NtFreeVirtualMemory(hProcess,
			  InitialTeb.StackAllocate,
			  &InitialTeb.StackReserve,
			  MEM_RELEASE);

      DPRINT("NtCreateThread() failed!\n");
      SetLastErrorByStatus(Status);
      return(NULL);
    }

  if (lpThreadId != NULL)
    memcpy(lpThreadId, &ClientId.UniqueThread,sizeof(ULONG));

  return(ThreadHandle);
}

PTEB
GetTeb(VOID)
{
  return(NtCurrentTeb());
}

WINBOOL STDCALL
SwitchToThread(VOID)
{
   NTSTATUS errCode;
   errCode = NtYieldExecution();
   return TRUE;
}

DWORD STDCALL
GetCurrentThreadId()
{
   return((DWORD)(NtCurrentTeb()->Cid).UniqueThread);
}

VOID STDCALL
ExitThread(DWORD uExitCode)
{
   NTSTATUS errCode;
   BOOLEAN LastThread;
   NTSTATUS Status;

   /*
    * Terminate process if this is the last thread
    * of the current process
    */
   Status = NtQueryInformationThread(NtCurrentThread(),
				     ThreadAmILastThread,
				     &LastThread,
				     sizeof(BOOLEAN),
				     NULL);
   if (NT_SUCCESS(Status) && LastThread == TRUE)
     {
	ExitProcess (uExitCode);
     }

   /* FIXME: notify csrss of thread termination */

   LdrShutdownThread();

   errCode = NtTerminateThread(NtCurrentThread(),
			       uExitCode);
   if (!NT_SUCCESS(errCode))
     {
	SetLastErrorByStatus(errCode);
     }
}

WINBOOL STDCALL GetThreadTimes(HANDLE hThread,
			       LPFILETIME lpCreationTime,
			       LPFILETIME lpExitTime,
			       LPFILETIME lpKernelTime,
			       LPFILETIME lpUserTime)
{
   NTSTATUS errCode;
   KERNEL_USER_TIMES KernelUserTimes;
   ULONG ReturnLength;
   
   errCode = NtQueryInformationThread(hThread,
				      ThreadTimes,
				      &KernelUserTimes,
				      sizeof(KERNEL_USER_TIMES),
				      &ReturnLength);
   if (!NT_SUCCESS(errCode))
     {
	SetLastErrorByStatus(errCode);
	return FALSE;
     }
   memcpy(lpCreationTime, &KernelUserTimes.CreateTime, sizeof(FILETIME));
   memcpy(lpExitTime, &KernelUserTimes.ExitTime, sizeof(FILETIME));
   memcpy(lpKernelTime, &KernelUserTimes.KernelTime, sizeof(FILETIME));
   memcpy(lpUserTime, &KernelUserTimes.UserTime, sizeof(FILETIME));
   return TRUE;
}


WINBOOL STDCALL GetThreadContext(HANDLE hThread,
				 LPCONTEXT lpContext)
{
   NTSTATUS errCode;
   
   errCode = NtGetContextThread(hThread,
				lpContext);
   if (!NT_SUCCESS(errCode))
     {
	SetLastErrorByStatus(errCode);
	return FALSE;
     }
   return TRUE;
}

WINBOOL STDCALL SetThreadContext(HANDLE hThread,
				 CONST CONTEXT *lpContext)
{
   NTSTATUS errCode;
   
   errCode = NtSetContextThread(hThread,
				(void *)lpContext);
   if (!NT_SUCCESS(errCode))
     {
	SetLastErrorByStatus(errCode);
	return FALSE;
     }
   return TRUE;
}

WINBOOL STDCALL GetExitCodeThread(HANDLE hThread,
				  LPDWORD lpExitCode)
{
   NTSTATUS errCode;
   THREAD_BASIC_INFORMATION ThreadBasic;
   ULONG DataWritten;
   
   errCode = NtQueryInformationThread(hThread,
				      ThreadBasicInformation,
				      &ThreadBasic,
				      sizeof(THREAD_BASIC_INFORMATION),
				      &DataWritten);
   if (!NT_SUCCESS(errCode))
     {
	SetLastErrorByStatus(errCode);
	return FALSE;
     }
   memcpy(lpExitCode, &ThreadBasic.ExitStatus, sizeof(DWORD));
   return TRUE;
}

DWORD STDCALL ResumeThread(HANDLE hThread)
{
   NTSTATUS errCode;
   ULONG PreviousResumeCount;
   
   errCode = NtResumeThread(hThread,
			    &PreviousResumeCount);
   if (!NT_SUCCESS(errCode))
     {
	SetLastErrorByStatus(errCode);
	return  -1;
     }
   return PreviousResumeCount;
}


WINBOOL STDCALL
TerminateThread (HANDLE	hThread,
		 DWORD	dwExitCode)
{
    if (0 == hThread)
    {
        SetLastError (ERROR_INVALID_HANDLE);
    }
    else
    {
        NTSTATUS Status = NtTerminateThread (hThread, dwExitCode);
	
        if (NT_SUCCESS(Status))
	{
            return TRUE;
        }
        SetLastErrorByStatus (Status);
    }
    return FALSE;
}


DWORD STDCALL SuspendThread(HANDLE hThread)
{
   NTSTATUS errCode;
   ULONG PreviousSuspendCount;
   
   errCode = NtSuspendThread(hThread,
			     &PreviousSuspendCount);
   if (!NT_SUCCESS(errCode))
     {
	SetLastErrorByStatus(errCode);
	return -1;
     }
   return PreviousSuspendCount;
}

DWORD STDCALL SetThreadAffinityMask(HANDLE hThread,
				    DWORD dwThreadAffinityMask)
{
   return 0;
}

WINBOOL STDCALL SetThreadPriority(HANDLE hThread,
				  int nPriority)
{
   NTSTATUS errCode;
   THREAD_BASIC_INFORMATION ThreadBasic;
   ULONG DataWritten;
   
   errCode = NtQueryInformationThread(hThread,
				      ThreadBasicInformation,
				      &ThreadBasic,
				      sizeof(THREAD_BASIC_INFORMATION),
				      &DataWritten);
   if (!NT_SUCCESS(errCode))
     {
	SetLastErrorByStatus(errCode);
	return FALSE;
     }
   ThreadBasic.BasePriority = nPriority;
   errCode = NtSetInformationThread(hThread,
				    ThreadBasicInformation,
				    &ThreadBasic,
				    sizeof(THREAD_BASIC_INFORMATION));
   if (!NT_SUCCESS(errCode))
     {
	SetLastErrorByStatus(errCode);
	return FALSE;
     }
   return TRUE;
}

int STDCALL GetThreadPriority(HANDLE hThread)
{
   NTSTATUS errCode;
   THREAD_BASIC_INFORMATION ThreadBasic;
   ULONG DataWritten;
   
   errCode = NtQueryInformationThread(hThread,
				      ThreadBasicInformation,
				      &ThreadBasic,
				      sizeof(THREAD_BASIC_INFORMATION),
				      &DataWritten);
   if (!NT_SUCCESS(errCode))
     {
	SetLastErrorByStatus(errCode);
	return THREAD_PRIORITY_ERROR_RETURN;
     }
   return ThreadBasic.BasePriority;
}

/* EOF */
