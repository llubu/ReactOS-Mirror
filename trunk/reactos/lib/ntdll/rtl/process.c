/* $Id: process.c,v 1.5 1999/12/06 00:22:43 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/ntdll/rtl/process.c
 * PURPOSE:         Process functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/* INCLUDES ****************************************************************/

#define WIN32_NO_PEHDR
#include <ddk/ntddk.h>
#include <wchar.h>
#include <string.h>
#include <pe.h>
#include <internal/i386/segment.h>
#include <ntdll/ldr.h>
#include <internal/teb.h>
#include <ntdll/base.h>
#include <ntdll/rtl.h>

#define NDEBUG
#include <ntdll/ntdll.h>

/* FUNCTIONS ****************************************************************/


#define STACK_TOP (0xb0000000)

static HANDLE STDCALL
RtlpCreateFirstThread(HANDLE ProcessHandle,
                      PSECURITY_DESCRIPTOR SecurityDescriptor,
				 DWORD dwStackSize,
				 LPTHREAD_START_ROUTINE lpStartAddress,
				 LPVOID lpParameter,
				 DWORD dwCreationFlags,
				 LPDWORD lpThreadId,
				 PWSTR lpCommandLine,
				 HANDLE NTDllSectionHandle,
				 HANDLE SectionHandle,
				 PVOID ImageBase)
{	
   NTSTATUS Status;
   HANDLE ThreadHandle;
   OBJECT_ATTRIBUTES ObjectAttributes;
   CLIENT_ID ClientId;
   CONTEXT ThreadContext;
   INITIAL_TEB InitialTeb;
   BOOLEAN CreateSuspended = FALSE;
   PVOID BaseAddress;
   ULONG BytesWritten;
   HANDLE DupNTDllSectionHandle, DupSectionHandle;

   ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
   ObjectAttributes.RootDirectory = NULL;
   ObjectAttributes.ObjectName = NULL;
   ObjectAttributes.Attributes = 0;
//   ObjectAttributes.Attributes = OBJ_INHERIT;
   ObjectAttributes.SecurityDescriptor = SecurityDescriptor;
   ObjectAttributes.SecurityQualityOfService = NULL;

   if ((dwCreationFlags & CREATE_SUSPENDED) == CREATE_SUSPENDED)
     CreateSuspended = TRUE;
   else
     CreateSuspended = FALSE;

   BaseAddress = (PVOID)(STACK_TOP - dwStackSize);
   Status = NtAllocateVirtualMemory(ProcessHandle,
				    &BaseAddress,
				    0,
				    (PULONG)&dwStackSize,
				    MEM_COMMIT,
				    PAGE_READWRITE);
   if (!NT_SUCCESS(Status))
     {
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
   ThreadContext.Esp = STACK_TOP - 16;
   ThreadContext.EFlags = (1<<1) + (1<<9);

   DPRINT("ThreadContext.Eip %x\n",ThreadContext.Eip);

   NtDuplicateObject(NtCurrentProcess(),
		     &SectionHandle,
		     ProcessHandle,
		     &DupSectionHandle,
		     0,
		     FALSE,
		     DUPLICATE_SAME_ACCESS);
   NtDuplicateObject(NtCurrentProcess(),
		     &NTDllSectionHandle,
		     ProcessHandle,
		     &DupNTDllSectionHandle,
		     0,
		     FALSE,
		     DUPLICATE_SAME_ACCESS);

   NtWriteVirtualMemory(ProcessHandle,
			(PVOID)(STACK_TOP - 4),
			&DupNTDllSectionHandle,
			sizeof(DupNTDllSectionHandle),
			&BytesWritten);
   NtWriteVirtualMemory(ProcessHandle,
			(PVOID)(STACK_TOP - 8),
			&ImageBase,
			sizeof(ImageBase),
			&BytesWritten);
   NtWriteVirtualMemory(ProcessHandle,
			(PVOID)(STACK_TOP - 12),
			&DupSectionHandle,
			sizeof(DupSectionHandle),
			&BytesWritten);

   Status = NtCreateThread(&ThreadHandle,
                           THREAD_ALL_ACCESS,
                           &ObjectAttributes,
                           ProcessHandle,
                           &ClientId,
                           &ThreadContext,
                           &InitialTeb,
                           CreateSuspended);
   if ( lpThreadId != NULL )
     memcpy(lpThreadId, &ClientId.UniqueThread,sizeof(ULONG));

   return ThreadHandle;
}


static NTSTATUS
RtlpMapFile(PUNICODE_STRING ApplicationName,
            PIMAGE_NT_HEADERS Headers,
            PIMAGE_DOS_HEADER DosHeader,
            PHANDLE Section)
{
    HANDLE hFile;
    IO_STATUS_BLOCK IoStatusBlock;
    LARGE_INTEGER FileOffset;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PSECURITY_DESCRIPTOR SecurityDescriptor = NULL;
    NTSTATUS Status;

    hFile = NULL;
    *Section = NULL;


    DPRINT("ApplicationName %w\n", ApplicationName->Buffer);

    InitializeObjectAttributes(&ObjectAttributes,
                               ApplicationName,
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
    if (!NT_SUCCESS(Status))
        return Status;

    Status = NtReadFile(hFile,
                        NULL,
                        NULL,
                        NULL,
                        &IoStatusBlock,
                        DosHeader,
                        sizeof(IMAGE_DOS_HEADER),
                        NULL,
                        NULL);
    if (!NT_SUCCESS(Status))
        return Status;

    FileOffset.u.LowPart = DosHeader->e_lfanew;
    FileOffset.u.HighPart = 0;

    Status = NtReadFile(hFile,
		       NULL,
		       NULL,
		       NULL,
		       &IoStatusBlock,
		       Headers,
		       sizeof(IMAGE_NT_HEADERS),
		       &FileOffset,
		       NULL);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = NtCreateSection(Section,
                             SECTION_ALL_ACCESS,
                             NULL,
                             NULL,
                             PAGE_EXECUTE,
                             SEC_IMAGE,
                             hFile);
    NtClose(hFile);

    if (!NT_SUCCESS(Status))
        return Status;

    return STATUS_SUCCESS;
}


static NTSTATUS
RtlpCreatePeb (
	HANDLE	ProcessHandle,
	PPPB	Ppb)
{
    NTSTATUS Status;
    ULONG BytesWritten;
    PVOID PebBase;
    ULONG PebSize;
    PEB Peb;
    PVOID PpbBase;
    ULONG PpbSize;

    PebBase = (PVOID)PEB_BASE;
    PebSize = 0x1000;

	Status = NtAllocateVirtualMemory (
		ProcessHandle,
		&PebBase,
		0,
		&PebSize,
		MEM_COMMIT,
		PAGE_READWRITE);

	memset(&Peb, 0, sizeof(Peb));
	Peb.Ppb = (PPPB)PEB_STARTUPINFO;

    NtWriteVirtualMemory(ProcessHandle,
                         (PVOID)PEB_BASE,
                         &Peb,
                         sizeof(Peb),
                         &BytesWritten);

    PpbBase = (PVOID)PEB_STARTUPINFO;
    PpbSize = Ppb->TotalSize;
    Status = NtAllocateVirtualMemory(ProcessHandle,
                                     &PpbBase,
                                     0,
                                     &PpbSize,
                                     MEM_COMMIT,
                                     PAGE_READWRITE);
    if (!NT_SUCCESS(Status))
	return(Status);

	DPRINT("Ppb size %x\n", PpbSize);
	NtWriteVirtualMemory (
		ProcessHandle,
		(PVOID)PEB_STARTUPINFO,
		Ppb,
		Ppb->TotalSize,
		&BytesWritten);

	return STATUS_SUCCESS;
}


NTSTATUS
STDCALL
RtlCreateUserProcess (
	PUNICODE_STRING		CommandLine,
	ULONG			Unknown1,
	PPPB			Ppb,
	PSECURITY_DESCRIPTOR ProcessSd,
	PSECURITY_DESCRIPTOR ThreadSd,
	WINBOOL bInheritHandles,
	DWORD dwCreationFlags,
	PCLIENT_ID ClientId,
	PHANDLE ProcessHandle,
	PHANDLE ThreadHandle)
{
   HANDLE hSection, hProcess, hThread;
   NTSTATUS Status;
   LPTHREAD_START_ROUTINE  lpStartAddress = NULL;
   LPVOID  lpParameter = NULL;
   WCHAR TempCommandLine[256];
   PVOID BaseAddress;
   LARGE_INTEGER SectionOffset;
   IMAGE_NT_HEADERS Headers;
   IMAGE_DOS_HEADER DosHeader;
   HANDLE NTDllSection;
   ULONG InitialViewSize;
   PROCESS_BASIC_INFORMATION ProcessBasicInfo;
   CLIENT_ID LocalClientId;
   ULONG retlen;

	DPRINT ("RtlCreateUserProcess(CommandLine '%w')\n",
		CommandLine->Buffer);

    Status = RtlpMapFile(CommandLine,
                         &Headers,
                         &DosHeader,
                         &hSection);

    Status = NtCreateProcess(&hProcess,
                             PROCESS_ALL_ACCESS,
                             NULL,
                             NtCurrentProcess(),
                             bInheritHandles,
                             NULL,
                             NULL,
                             NULL);

    NtQueryInformationProcess(hProcess,
                              ProcessBasicInformation,
                              &ProcessBasicInfo,
                              sizeof(ProcessBasicInfo),
                              &retlen);
    DPRINT("ProcessBasicInfo.UniqueProcessId %d\n",
           ProcessBasicInfo.UniqueProcessId);
    LocalClientId.UniqueProcess = ProcessBasicInfo.UniqueProcessId;

    /*
     * Map NT DLL into the process
     */
    Status = LdrMapNTDllForProcess(hProcess,
                                   &NTDllSection);

   InitialViewSize = DosHeader.e_lfanew + sizeof(IMAGE_NT_HEADERS) 
     + sizeof(IMAGE_SECTION_HEADER) * Headers.FileHeader.NumberOfSections;

   BaseAddress = (PVOID)Headers.OptionalHeader.ImageBase;
   SectionOffset.QuadPart = 0;
   Status = NtMapViewOfSection(hSection,
			       hProcess,
			       &BaseAddress,
			       0,
			       InitialViewSize,
			       &SectionOffset,
			       &InitialViewSize,
			       0,
			       MEM_COMMIT,
			       PAGE_READWRITE);
   if (!NT_SUCCESS(Status))
       return Status;

   /*
    * 
    */
   DPRINT("Creating peb\n");
   RtlpCreatePeb (hProcess, Ppb);

   DPRINT("Creating thread for process\n");
   lpStartAddress = (LPTHREAD_START_ROUTINE)
     ((PIMAGE_OPTIONAL_HEADER)OPTHDROFFSET(NTDLL_BASE))->
     AddressOfEntryPoint + 
     ((PIMAGE_OPTIONAL_HEADER)OPTHDROFFSET(NTDLL_BASE))->ImageBase;
	hThread = RtlpCreateFirstThread (
		hProcess,
		ThreadSd,
		Headers.OptionalHeader.SizeOfStackReserve,
		lpStartAddress,
		lpParameter,
		dwCreationFlags,
		&LocalClientId.UniqueThread,
		TempCommandLine,
		NTDllSection,
		hSection,
		(PVOID)Headers.OptionalHeader.ImageBase);

    if ( hThread == NULL )
        return Status;

    if (ClientId)
    {
        ClientId->UniqueProcess = LocalClientId.UniqueProcess;
        ClientId->UniqueThread = LocalClientId.UniqueThread;
    }

    if (ProcessHandle)
        *ProcessHandle = hProcess;

    if (ThreadHandle)
        *ThreadHandle = hThread;

    return STATUS_SUCCESS;
}


VOID
STDCALL
RtlAcquirePebLock (VOID)
{

}


VOID
STDCALL
RtlReleasePebLock (VOID)
{

}

NTSTATUS
STDCALL
RtlCreateProcessParameters (
	PPPB		*Ppb,
	PUNICODE_STRING	CommandLine,
	PUNICODE_STRING	LibraryPath,
	PUNICODE_STRING	CurrentDirectory,
	PUNICODE_STRING	ImageName,
	PVOID		Environment,
	PUNICODE_STRING	Title,
	PUNICODE_STRING	Desktop,
	PUNICODE_STRING	Reserved,
	PVOID		Reserved2
	)
{
	NTSTATUS Status = STATUS_SUCCESS;
	PPPB Param = NULL;
	ULONG RegionSize = 0;
	ULONG DataSize = 0;
	PWCHAR Dest;

	DPRINT ("RtlCreateProcessParameters\n");

	RtlAcquirePebLock ();

	/* size of process parameter block */
	DataSize = sizeof (PPB);

	/* size of (reserved) buffer */
	DataSize += (256 * sizeof(WCHAR));

	/* size of current directory buffer */
	DataSize += (MAX_PATH * sizeof(WCHAR));

	/* add string lengths */
	if (LibraryPath != NULL)
		DataSize += (LibraryPath->Length + sizeof(WCHAR));

	if (CommandLine != NULL)
		DataSize += (CommandLine->Length + sizeof(WCHAR));

	if (ImageName != NULL)
		DataSize += (ImageName->Length + sizeof(WCHAR));

	if (Title != NULL)
		DataSize += (Title->Length + sizeof(WCHAR));

	if (Desktop != NULL)
		DataSize += (Desktop->Length + sizeof(WCHAR));

	if (Reserved != NULL)
		DataSize += (Reserved->Length + sizeof(WCHAR));

	/* Calculate the required block size */
	RegionSize = DataSize;

	Status = NtAllocateVirtualMemory (
		NtCurrentProcess (),
		(PVOID*)&Param,
		0,
		&RegionSize,
		MEM_COMMIT,
		PAGE_READWRITE);
	if (!NT_SUCCESS(Status))
	{
		RtlReleasePebLock ();
		return Status;
	}

	DPRINT ("Ppb allocated\n");

	Param->TotalSize = RegionSize;
	Param->DataSize = DataSize;
	Param->Normalized = TRUE;
	Param->Environment = Environment;
//	Param->Unknown1 =
//	Param->Unknown2 =
//	Param->Unknown3 =
//	Param->Unknown4 =

	/* copy current directory */
	Dest = (PWCHAR)(((PBYTE)Param) + sizeof(PPB) + (256 * sizeof(WCHAR)));

	Param->CurrentDirectory.Buffer = Dest;
	if (CurrentDirectory != NULL)
	{
		Param->CurrentDirectory.Length = CurrentDirectory->Length;
		Param->CurrentDirectory.MaximumLength = CurrentDirectory->Length + sizeof(WCHAR);
		memcpy (Dest,
		        CurrentDirectory->Buffer,
		        CurrentDirectory->Length);
		Dest = (PWCHAR)(((PBYTE)Dest) + CurrentDirectory->Length);
	}
	*Dest = 0;

	Dest = (PWCHAR)(((PBYTE)Param) + sizeof(PPB) +
			(256 * sizeof(WCHAR)) + (MAX_PATH * sizeof(WCHAR)));

	/* copy library path */
	Param->LibraryPath.Buffer = Dest;
	if (LibraryPath != NULL)
	{
		Param->LibraryPath.Length = LibraryPath->Length;
		memcpy (Dest,
		        LibraryPath->Buffer,
		        LibraryPath->Length);
		Dest = (PWCHAR)(((PBYTE)Dest) + LibraryPath->Length);
	}
	Param->LibraryPath.MaximumLength = Param->LibraryPath.Length + sizeof(WCHAR);
	*Dest = 0;
	Dest++;

	/* copy command line */
	Param->CommandLine.Buffer = Dest;
	if (CommandLine != NULL)
	{
		Param->CommandLine.Length = CommandLine->Length;
		memcpy (Dest,
		        CommandLine->Buffer,
		        CommandLine->Length);
		Dest = (PWCHAR)(((PBYTE)Dest) + CommandLine->Length);
	}
	Param->CommandLine.MaximumLength = Param->CommandLine.Length + sizeof(WCHAR);
	*Dest = 0;
	Dest++;

	/* copy image name */
	Param->ImageName.Buffer = Dest;
	if (ImageName != NULL)
	{
		Param->ImageName.Length = ImageName->Length;
		memcpy (Dest,
		        ImageName->Buffer,
		        ImageName->Length);
		Dest = (PWCHAR)(((PBYTE)Dest) + ImageName->Length);
	}
	Param->ImageName.MaximumLength = Param->ImageName.Length + sizeof(WCHAR);
	*Dest = 0;
	Dest++;

	/* copy title */
	Param->Title.Buffer = Dest;
	if (Title != NULL)
	{
		Param->Title.Length = Title->Length;
		memcpy (Dest,
		        Title->Buffer,
		        Title->Length);
		Dest = (PWCHAR)(((PBYTE)Dest) + Title->Length);
	}
	Param->Title.MaximumLength = Param->Title.Length + sizeof(WCHAR);
	*Dest = 0;
	Dest++;

	/* copy desktop */
	Param->Desktop.Buffer = Dest;
	if (Desktop != NULL)
	{
		Param->Desktop.Length = Desktop->Length;
		memcpy (Dest,
		        Desktop->Buffer,
		        Desktop->Length);
		Dest = (PWCHAR)(((PBYTE)Dest) + Desktop->Length);
	}
	Param->Desktop.MaximumLength = Param->Desktop.Length + sizeof(WCHAR);
	*Dest = 0;
	Dest++;

	/* copy reserved */
	Param->Reserved.Buffer = Dest;
	if (Reserved != NULL)
	{
		Param->Reserved.Length = Reserved->Length;
		memcpy (Dest,
		        Reserved->Buffer,
		        Reserved->Length);
		Dest = (PWCHAR)(((PBYTE)Dest) + Reserved->Length);
	}
	Param->Reserved.MaximumLength = Param->Reserved.Length + sizeof(WCHAR);
	*Dest = 0;
	Dest++;

	/* set reserved2 */
	Param->Reserved2.Length = 0;
	Param->Reserved2.MaximumLength = 0;
	Param->Reserved2.Buffer = NULL;

	RtlDeNormalizeProcessParams (Param);
	*Ppb = Param;
	RtlReleasePebLock ();

	return Status;
}

VOID
STDCALL
RtlDestroyProcessParameters (
	PPPB	Ppb
	)
{
	ULONG RegionSize = 0;

	NtFreeVirtualMemory (NtCurrentProcess (),
	                     (PVOID)Ppb,
	                     &RegionSize,
	                     MEM_RELEASE);
}

/*
 * denormalize process parameters (Pointer-->Offset)
 */
VOID
STDCALL
RtlDeNormalizeProcessParams (
	PPPB	Ppb
	)
{
	if (Ppb == NULL)
		return;

	if (Ppb->Normalized == FALSE)
		return;

	if (Ppb->CurrentDirectory.Buffer != NULL)
	{
		Ppb->CurrentDirectory.Buffer =
			(PWSTR)((ULONG)Ppb->CurrentDirectory.Buffer -
				(ULONG)Ppb);
	}

	if (Ppb->LibraryPath.Buffer != NULL)
	{
		Ppb->LibraryPath.Buffer =
			(PWSTR)((ULONG)Ppb->LibraryPath.Buffer -
				(ULONG)Ppb);
	}

	if (Ppb->CommandLine.Buffer != NULL)
	{
		Ppb->CommandLine.Buffer =
			(PWSTR)((ULONG)Ppb->CommandLine.Buffer -
				(ULONG)Ppb);
	}

	if (Ppb->ImageName.Buffer != NULL)
	{
		Ppb->ImageName.Buffer =
			(PWSTR)((ULONG)Ppb->ImageName.Buffer -
				(ULONG)Ppb);
	}

	if (Ppb->Title.Buffer != NULL)
	{
		Ppb->Title.Buffer =
			(PWSTR)((ULONG)Ppb->Title.Buffer -
				(ULONG)Ppb);
	}

	if (Ppb->Desktop.Buffer != NULL)
	{
		Ppb->Desktop.Buffer =
			(PWSTR)((ULONG)Ppb->Desktop.Buffer -
				(ULONG)Ppb);
	}

	if (Ppb->Reserved.Buffer != NULL)
	{
		Ppb->Reserved.Buffer =
			(PWSTR)((ULONG)Ppb->Reserved.Buffer -
				(ULONG)Ppb);
	}

	Ppb->Normalized = FALSE;
}

/*
 * normalize process parameters (Offset-->Pointer)
 */
VOID
STDCALL
RtlNormalizeProcessParams (
	PPPB	Ppb
	)
{
	if (Ppb == NULL)
		return;

	if (Ppb->Normalized == TRUE)
		return;

	if (Ppb->CurrentDirectory.Buffer != NULL)
	{
		Ppb->CurrentDirectory.Buffer =
			(PWSTR)((ULONG)Ppb->CurrentDirectory.Buffer +
				(ULONG)Ppb);
	}

	if (Ppb->LibraryPath.Buffer != NULL)
	{
		Ppb->LibraryPath.Buffer =
			(PWSTR)((ULONG)Ppb->LibraryPath.Buffer +
				(ULONG)Ppb);
	}

	if (Ppb->CommandLine.Buffer != NULL)
	{
		Ppb->CommandLine.Buffer =
			(PWSTR)((ULONG)Ppb->CommandLine.Buffer +
				(ULONG)Ppb);
	}

	if (Ppb->ImageName.Buffer != NULL)
	{
		Ppb->ImageName.Buffer =
			(PWSTR)((ULONG)Ppb->ImageName.Buffer +
				(ULONG)Ppb);
	}

	if (Ppb->Title.Buffer != NULL)
	{
		Ppb->Title.Buffer =
			(PWSTR)((ULONG)Ppb->Title.Buffer +
				(ULONG)Ppb);
	}

	if (Ppb->Desktop.Buffer != NULL)
	{
		Ppb->Desktop.Buffer =
			(PWSTR)((ULONG)Ppb->Desktop.Buffer +
				(ULONG)Ppb);
	}

	if (Ppb->Reserved.Buffer != NULL)
	{
		Ppb->Reserved.Buffer =
			(PWSTR)((ULONG)Ppb->Reserved.Buffer +
				(ULONG)Ppb);
	}

	Ppb->Normalized = TRUE;
}

/* EOF */
