/* $Id: create.c,v 1.41 2001/08/07 14:11:41 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/process/create.c
 * PURPOSE:         Process functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/* INCLUDES ****************************************************************/

#include <ddk/ntddk.h>
#include <windows.h>
#include <kernel32/proc.h>
#include <kernel32/thread.h>
#include <wchar.h>
#include <string.h>
#include <napi/i386/segment.h>
#include <ntdll/ldr.h>
#include <napi/teb.h>
#include <ntdll/base.h>
#include <ntdll/rtl.h>
#include <csrss/csrss.h>
#include <ntdll/csr.h>

#define NDEBUG
#include <kernel32/kernel32.h>
#include <kernel32/error.h>

/* FUNCTIONS ****************************************************************/

WINBOOL STDCALL
CreateProcessA (LPCSTR			lpApplicationName,
		LPSTR			lpCommandLine,
		LPSECURITY_ATTRIBUTES	lpProcessAttributes,
		LPSECURITY_ATTRIBUTES	lpThreadAttributes,
		WINBOOL			bInheritHandles,
		DWORD			dwCreationFlags,
		LPVOID			lpEnvironment,
		LPCSTR			lpCurrentDirectory,
		LPSTARTUPINFOA		lpStartupInfo,
		LPPROCESS_INFORMATION	lpProcessInformation)
/*
 * FUNCTION: The CreateProcess function creates a new process and its
 * primary thread. The new process executes the specified executable file
 * ARGUMENTS:
 * 
 *     lpApplicationName = Pointer to name of executable module
 *     lpCommandLine = Pointer to command line string
 *     lpProcessAttributes = Process security attributes
 *     lpThreadAttributes = Thread security attributes
 *     bInheritHandles = Handle inheritance flag
 *     dwCreationFlags = Creation flags
 *     lpEnvironment = Pointer to new environment block
 *     lpCurrentDirectory = Pointer to current directory name
 *     lpStartupInfo = Pointer to startup info
 *     lpProcessInformation = Pointer to process information
 */
{
	UNICODE_STRING ApplicationNameU;
	UNICODE_STRING CurrentDirectoryU;
	UNICODE_STRING CommandLineU;
	ANSI_STRING ApplicationName;
	ANSI_STRING CurrentDirectory;
	ANSI_STRING CommandLine;
	WINBOOL Result;

	DPRINT("CreateProcessA\n");

	RtlInitAnsiString (&CommandLine,
	                   lpCommandLine);
	RtlInitAnsiString (&ApplicationName,
	                   (LPSTR)lpApplicationName);
	RtlInitAnsiString (&CurrentDirectory,
	                   (LPSTR)lpCurrentDirectory);

	/* convert ansi (or oem) strings to unicode */
	if (bIsFileApiAnsi)
	{
		RtlAnsiStringToUnicodeString (&CommandLineU,
		                              &CommandLine,
		                              TRUE);
		RtlAnsiStringToUnicodeString (&ApplicationNameU,
		                              &ApplicationName,
		                              TRUE);
		RtlAnsiStringToUnicodeString (&CurrentDirectoryU,
		                              &CurrentDirectory,
		                              TRUE);
	}
	else
	{
		RtlOemStringToUnicodeString (&CommandLineU,
		                             &CommandLine,
		                             TRUE);
		RtlOemStringToUnicodeString (&ApplicationNameU,
		                             &ApplicationName,
		                             TRUE);
		RtlOemStringToUnicodeString (&CurrentDirectoryU,
		                             &CurrentDirectory,
		                             TRUE);
	}

	Result = CreateProcessW (ApplicationNameU.Buffer,
	                         CommandLineU.Buffer,
	                         lpProcessAttributes,
	                         lpThreadAttributes,
	                         bInheritHandles,
	                         dwCreationFlags,
	                         lpEnvironment,
	                         (lpCurrentDirectory == NULL) ? NULL : CurrentDirectoryU.Buffer,
	                         (LPSTARTUPINFOW)lpStartupInfo,
	                         lpProcessInformation);

	RtlFreeUnicodeString (&ApplicationNameU);
	RtlFreeUnicodeString (&CommandLineU);
	RtlFreeUnicodeString (&CurrentDirectoryU);

	return Result;
}


HANDLE STDCALL 
KlCreateFirstThread(HANDLE ProcessHandle,
		    LPSECURITY_ATTRIBUTES lpThreadAttributes,
		    ULONG StackReserve,
		    ULONG StackCommit,
		    LPTHREAD_START_ROUTINE lpStartAddress,
		    DWORD dwCreationFlags,
		    LPDWORD lpThreadId)
{
  NTSTATUS Status;
  HANDLE ThreadHandle;
  OBJECT_ATTRIBUTES ObjectAttributes;
  CLIENT_ID ClientId;
  CONTEXT ThreadContext;
  INITIAL_TEB InitialTeb;
  BOOLEAN CreateSuspended = FALSE;
  ULONG OldPageProtection;

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

  InitialTeb.StackReserve = (StackReserve < 0x100000) ? 0x100000 : StackReserve;
  /* FIXME: use correct commit size */
#if 0
  InitialTeb.StackCommit = (StackCommit < PAGESIZE) ? PAGESIZE : StackCommit;
#endif
  InitialTeb.StackCommit = InitialTeb.StackReserve - PAGESIZE;

  /* size of guard page */
  InitialTeb.StackCommit += PAGESIZE;

  /* Reserve stack */
  InitialTeb.StackAllocate = NULL;
  Status = NtAllocateVirtualMemory(ProcessHandle,
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

  DPRINT("StackBase: %p\nStackCommit: %p\n",
	 InitialTeb.StackBase, InitialTeb.StackCommit);

  /* Commit stack page(s) */
  Status = NtAllocateVirtualMemory(ProcessHandle,
				   &InitialTeb.StackLimit,
				   0,
				   &InitialTeb.StackCommit,
				   MEM_COMMIT,
				   PAGE_READWRITE);
  if (!NT_SUCCESS(Status))
    {
      /* release the stack space */
      NtFreeVirtualMemory(ProcessHandle,
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
  Status = NtProtectVirtualMemory(ProcessHandle,
				  InitialTeb.StackLimit,
				  PAGESIZE,
				  PAGE_GUARD | PAGE_READWRITE,
				  &OldPageProtection);
  if (!NT_SUCCESS(Status))
    {
      /* release the stack space */
      NtFreeVirtualMemory(ProcessHandle,
			  InitialTeb.StackAllocate,
			  &InitialTeb.StackReserve,
			  MEM_RELEASE);

      DPRINT("Error comitting guard page!\n");
      SetLastErrorByStatus(Status);
      return(NULL);
    }

  memset(&ThreadContext,0,sizeof(CONTEXT));
  ThreadContext.Eip = (ULONG)lpStartAddress;
  ThreadContext.SegGs = USER_DS;
  ThreadContext.SegFs = USER_DS;
  ThreadContext.SegEs = USER_DS;
  ThreadContext.SegDs = USER_DS;
  ThreadContext.SegCs = USER_CS;
  ThreadContext.SegSs = USER_DS;
  ThreadContext.Esp = (ULONG)InitialTeb.StackBase - 20;
  ThreadContext.EFlags = (1<<1) + (1<<9);

  DPRINT("ThreadContext.Eip %x\n",ThreadContext.Eip);

  Status = NtCreateThread(&ThreadHandle,
			  THREAD_ALL_ACCESS,
			  &ObjectAttributes,
			  ProcessHandle,
			  &ClientId,
			  &ThreadContext,
			  &InitialTeb,
			  CreateSuspended);
  if (!NT_SUCCESS(Status))
    {
      NtFreeVirtualMemory(ProcessHandle,
			  InitialTeb.StackAllocate,
			  &InitialTeb.StackReserve,
			  MEM_RELEASE);
      SetLastErrorByStatus(Status);
      return(NULL);
    }

  if (lpThreadId != NULL)
    {
      memcpy(lpThreadId, &ClientId.UniqueThread,sizeof(ULONG));
    }

  return(ThreadHandle);
}

HANDLE 
KlMapFile(LPCWSTR lpApplicationName,
	  LPCWSTR lpCommandLine)
{
   HANDLE hFile;
   IO_STATUS_BLOCK IoStatusBlock;
   UNICODE_STRING ApplicationNameString;
   OBJECT_ATTRIBUTES ObjectAttributes;
   PSECURITY_DESCRIPTOR SecurityDescriptor = NULL;
   NTSTATUS Status;
   HANDLE hSection;

   hFile = NULL;

   /*
    * Find the application name
    */

   if (!RtlDosPathNameToNtPathName_U ((LPWSTR)lpApplicationName,
                                      &ApplicationNameString,
                                      NULL,
                                      NULL))
	return NULL;

   DPRINT("ApplicationName %S\n",ApplicationNameString.Buffer);

   InitializeObjectAttributes(&ObjectAttributes,
			      &ApplicationNameString,
			      OBJ_CASE_INSENSITIVE,
			      NULL,
			      SecurityDescriptor);

   /*
    * Try to open the executable
    */

   Status = NtOpenFile(&hFile,
			SYNCHRONIZE|FILE_EXECUTE|FILE_READ_DATA,
			&ObjectAttributes,
			&IoStatusBlock,
			FILE_SHARE_DELETE|FILE_SHARE_READ,
			FILE_SYNCHRONOUS_IO_NONALERT|FILE_NON_DIRECTORY_FILE);

   RtlFreeUnicodeString (&ApplicationNameString);

   if (!NT_SUCCESS(Status))
     {
	DPRINT("Failed to open file\n");
	SetLastErrorByStatus (Status);
	return(NULL);
     }

   Status = NtCreateSection(&hSection,
			    SECTION_ALL_ACCESS,
			    NULL,
			    NULL,
			    PAGE_EXECUTE,
			    SEC_IMAGE,
			    hFile);
   NtClose(hFile);

   if (!NT_SUCCESS(Status))
     {
	DPRINT("Failed to create section\n");
	SetLastErrorByStatus (Status);
	return(NULL);
     }

   return(hSection);
}

static NTSTATUS 
KlInitPeb (HANDLE ProcessHandle,
	   PRTL_USER_PROCESS_PARAMETERS	Ppb)
{
   NTSTATUS Status;
   PVOID PpbBase;
   ULONG PpbSize;
   ULONG BytesWritten;
   ULONG Offset;
   PVOID ParentEnv = NULL;
   PVOID EnvPtr = NULL;
   ULONG EnvSize = 0;

   /* create the Environment */
   if (Ppb->Environment != NULL)
	ParentEnv = Ppb->Environment;
   else if (NtCurrentPeb()->ProcessParameters->Environment != NULL)
	ParentEnv = NtCurrentPeb()->ProcessParameters->Environment;

   if (ParentEnv != NULL)
     {
	MEMORY_BASIC_INFORMATION MemInfo;

	Status = NtQueryVirtualMemory (NtCurrentProcess (),
	                               ParentEnv,
	                               MemoryBasicInformation,
	                               &MemInfo,
	                               sizeof(MEMORY_BASIC_INFORMATION),
	                               NULL);
	if (!NT_SUCCESS(Status))
	  {
	     return Status;
	  }
	EnvSize = MemInfo.RegionSize;
     }
   DPRINT("EnvironmentSize %ld\n", EnvSize);

   /* allocate and initialize new environment block */
   if (EnvSize != 0)
     {
	Status = NtAllocateVirtualMemory(ProcessHandle,
					 &EnvPtr,
					 0,
					 &EnvSize,
					 MEM_RESERVE | MEM_COMMIT,
					 PAGE_READWRITE);
	if (!NT_SUCCESS(Status))
	  {
	     return(Status);
	  }

	NtWriteVirtualMemory(ProcessHandle,
			     EnvPtr,
			     ParentEnv,
			     EnvSize,
			     &BytesWritten);
     }

   /* create the PPB */
   PpbBase = NULL;
   PpbSize = Ppb->MaximumLength;
   Status = NtAllocateVirtualMemory(ProcessHandle,
				    &PpbBase,
				    0,
				    &PpbSize,
				    MEM_RESERVE | MEM_COMMIT,
				    PAGE_READWRITE);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }

   DPRINT("Ppb->MaximumLength %x\n", Ppb->MaximumLength);
   NtWriteVirtualMemory(ProcessHandle,
			PpbBase,
			Ppb,
			Ppb->MaximumLength,
			&BytesWritten);

   /* write pointer to environment */
   Offset = FIELD_OFFSET(RTL_USER_PROCESS_PARAMETERS, Environment);
   NtWriteVirtualMemory(ProcessHandle,
			(PVOID)(PpbBase + Offset),
			&EnvPtr,
			sizeof(EnvPtr),
			&BytesWritten);

   /* write pointer to process parameter block */
   Offset = FIELD_OFFSET(PEB, ProcessParameters);
   NtWriteVirtualMemory(ProcessHandle,
			(PVOID)(PEB_BASE + Offset),
			&PpbBase,
			sizeof(PpbBase),
			&BytesWritten);

   return(STATUS_SUCCESS);
}


WINBOOL STDCALL 
CreateProcessW(LPCWSTR lpApplicationName,
	       LPWSTR lpCommandLine,
	       LPSECURITY_ATTRIBUTES lpProcessAttributes,
	       LPSECURITY_ATTRIBUTES lpThreadAttributes,
	       WINBOOL bInheritHandles,
	       DWORD dwCreationFlags,
	       LPVOID lpEnvironment,
	       LPCWSTR lpCurrentDirectory,
	       LPSTARTUPINFOW lpStartupInfo,
	       LPPROCESS_INFORMATION lpProcessInformation)
{
   HANDLE hSection, hProcess, hThread;
   NTSTATUS Status;
   LPTHREAD_START_ROUTINE  lpStartAddress = NULL;
   WCHAR TempCommandLine[256];
   WCHAR ImagePathName[256];
   UNICODE_STRING ImagePathName_U;
   PROCESS_BASIC_INFORMATION ProcessBasicInfo;
   ULONG retlen;
   PRTL_USER_PROCESS_PARAMETERS Ppb;
   UNICODE_STRING CommandLine_U;
   CSRSS_API_REQUEST CsrRequest;
   CSRSS_API_REPLY CsrReply;
   CHAR ImageFileName[8];
   PWCHAR s;
   PWCHAR e;
   ULONG i;
   ANSI_STRING ProcedureName;
   UNICODE_STRING CurrentDirectoryW;
   SECTION_IMAGE_INFORMATION Sii;
   
   DPRINT("CreateProcessW(lpApplicationName '%S', lpCommandLine '%S')\n",
	   lpApplicationName,lpCommandLine);

   /*
    * Store the image file name for the process
    */
   s = wcsrchr(lpApplicationName, '\\');
   if (s == NULL)
     {
	s = (PWCHAR)lpApplicationName;
     }
   else
     {
	s++;
     }
   e = wcschr(s, '.');
   if (e != NULL)
     {
	*e = 0;
     }
   for (i = 0; i < 8; i++)
     {
	ImageFileName[i] = (CHAR)(s[i]);
     }
   if (e != NULL)
     {
	*e = '.';
     }
   
   /*
    * Process the application name and command line
    */

   RtlGetFullPathName_U ((LPWSTR)lpApplicationName,
                         256 * sizeof(WCHAR),
                         TempCommandLine,
                         NULL);
   wcscpy(ImagePathName, TempCommandLine);
   RtlInitUnicodeString(&ImagePathName_U, ImagePathName);

   if (lpCommandLine != NULL)
     {
	wcscat(TempCommandLine, L" ");
	wcscat(TempCommandLine, lpCommandLine);
     }

   /* Initialize the current directory string */
   RtlInitUnicodeString(&CurrentDirectoryW,
			lpCurrentDirectory);

   /*
    * Create the PPB
    */
   
   RtlInitUnicodeString(&CommandLine_U, lpCommandLine);

   DPRINT("CommandLine_U %S\n", CommandLine_U.Buffer);

   RtlCreateProcessParameters(&Ppb,
			      &ImagePathName_U,
			      NULL,
			      (lpCurrentDirectory == NULL) ? NULL : &CurrentDirectoryW,
			      &CommandLine_U,
			      lpEnvironment,
			      NULL,
			      NULL,
			      NULL,
			      NULL);
   
   /*
    * Create a section for the executable
    */
   
   hSection = KlMapFile (lpApplicationName, lpCommandLine);
   if (hSection == NULL)
   {
	RtlDestroyProcessParameters (Ppb);
	return FALSE;
   }
   
   /*
    * Create a new process
    */
   Status = NtCreateProcess(&hProcess,
			    PROCESS_ALL_ACCESS,
			    NULL,
			    NtCurrentProcess(),
			    bInheritHandles,
			    hSection,
			    NULL,
			    NULL);

   /*
    * Get some information about the executable
    */
   Status = ZwQuerySection(hSection,
			   SectionImageInformation,
			   &Sii,
			   sizeof(Sii),
			   &i);

   /*
    * Close the section
    */
   NtClose(hSection);

   /*
    * Get some information about the process
    */
   ZwQueryInformationProcess(hProcess,
			     ProcessBasicInformation,
			     &ProcessBasicInfo,
			     sizeof(ProcessBasicInfo),
			     &retlen);
   DPRINT("ProcessBasicInfo.UniqueProcessId %d\n",
	  ProcessBasicInfo.UniqueProcessId);
   lpProcessInformation->dwProcessId = ProcessBasicInfo.UniqueProcessId;
   
   /*
    * Tell the csrss server we are creating a new process
    */
   CsrRequest.Type = CSRSS_CREATE_PROCESS;
   CsrRequest.Data.CreateProcessRequest.NewProcessId = 
     ProcessBasicInfo.UniqueProcessId;
   CsrRequest.Data.CreateProcessRequest.Flags = dwCreationFlags;
   Status = CsrClientCallServer(&CsrRequest, 
				&CsrReply,
				sizeof(CSRSS_API_REQUEST),
				sizeof(CSRSS_API_REPLY));
   if (!NT_SUCCESS(Status) || !NT_SUCCESS(CsrReply.Status))
     {
	DbgPrint("Failed to tell csrss about new process. Expect trouble.\n");
     }
   
   /*
    * Create Process Environment Block
    */
   DPRINT("Creating peb\n");
   
   Ppb->InputHandle = CsrReply.Data.CreateProcessReply.InputHandle;
   Ppb->OutputHandle = CsrReply.Data.CreateProcessReply.OutputHandle;;
   Ppb->ErrorHandle = Ppb->OutputHandle;
   KlInitPeb(hProcess, Ppb);

   RtlDestroyProcessParameters (Ppb);

   Status = NtSetInformationProcess(hProcess,
				    ProcessImageFileName,
				    ImageFileName,
				    8);
   /*
    * Retrieve the start address
    */
   DPRINT("Retrieving entry point address\n");
   RtlInitAnsiString (&ProcedureName, "LdrInitializeThunk");
   Status = LdrGetProcedureAddress ((PVOID)NTDLL_BASE,
				    &ProcedureName,
				    0,
				    (PVOID*)&lpStartAddress);
   if (!NT_SUCCESS(Status))
     {
	DbgPrint ("LdrGetProcedureAddress failed (Status %x)\n", Status);
	return (Status);
     }
   DPRINT("lpStartAddress 0x%08lx\n", (ULONG)lpStartAddress);

   /*
    * Create the thread for the kernel
    */
   DPRINT("Creating thread for process\n");
   hThread =  KlCreateFirstThread(hProcess,
				  lpThreadAttributes,
				  //Sii.StackReserve,
				  0x200000,
				  //Sii.StackCommit,
				  0x1000,
				  lpStartAddress,
				  dwCreationFlags,
				  &lpProcessInformation->dwThreadId);
   if (hThread == NULL)
     {
	return FALSE;
     }

   lpProcessInformation->hProcess = hProcess;
   lpProcessInformation->hThread = hThread;

   return TRUE;
}

/* EOF */
