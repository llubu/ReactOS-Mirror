/* $Id: process.c,v 1.42 2000/05/14 09:34:14 ea Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * FILE:              ntoskrnl/ps/process.c
 * PURPOSE:           Process managment
 * PROGRAMMER:        David Welch (welch@cwcom.net)
 * REVISION HISTORY:
 *              21/07/98: Created
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <internal/ob.h>
#include <internal/mm.h>
#include <internal/ke.h>
#include <internal/ps.h>
#include <string.h>
#include <internal/string.h>
#include <internal/id.h>
#include <internal/teb.h>
#include <internal/ldr.h>
#include <internal/port.h>
#include <napi/dbg.h>
#include <internal/dbg.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS ******************************************************************/

PEPROCESS SystemProcess = NULL;
HANDLE SystemProcessHandle = NULL;

POBJECT_TYPE PsProcessType = NULL;

static LIST_ENTRY PsProcessListHead;
static KSPIN_LOCK PsProcessListLock;
static ULONG PiNextProcessUniqueId = 0;

/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL NtOpenProcessToken(IN	HANDLE		ProcessHandle,
				    IN	ACCESS_MASK	DesiredAccess,  
				    OUT	PHANDLE		TokenHandle)
{
   PACCESS_TOKEN Token;
   NTSTATUS Status;
   
   Status = PsOpenTokenOfProcess(ProcessHandle,
				 &Token);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   Status = ObCreateHandle(PsGetCurrentProcess(),
			   Token,
			   DesiredAccess,
			   FALSE,
			   ProcessHandle);
   ObDereferenceObject(Token);
   return(Status);
}

PACCESS_TOKEN PsReferencePrimaryToken(PEPROCESS Process)
{
   ObReferenceObjectByPointer(Process->Token,
			      GENERIC_ALL,
			      SeTokenType,
			      UserMode);
   return(Process->Token);
}

NTSTATUS PsOpenTokenOfProcess(HANDLE ProcessHandle,
			      PACCESS_TOKEN* Token)
{
   PEPROCESS Process;
   NTSTATUS Status;
   
   Status = ObReferenceObjectByHandle(ProcessHandle,
				      PROCESS_QUERY_INFORMATION,
				      PsProcessType,
				      UserMode,
				      (PVOID*)&Process,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   *Token = PsReferencePrimaryToken(Process);
   ObDereferenceObject(Process);
   return(STATUS_SUCCESS);
}

VOID PiKillMostProcesses(VOID)
{
   KIRQL oldIrql;
   PLIST_ENTRY current_entry;
   PEPROCESS current;
   
   KeAcquireSpinLock(&PsProcessListLock, &oldIrql);
   
   current_entry = PsProcessListHead.Flink;
   while (current_entry != &PsProcessListHead)
     {
	current = CONTAINING_RECORD(current_entry, EPROCESS, 
				    Pcb.ProcessListEntry);
	current_entry = current_entry->Flink;
	
	if (current->UniqueProcessId != SystemProcess->UniqueProcessId &&
	    current->UniqueProcessId != (ULONG)PsGetCurrentProcessId())
	  {
	     PiTerminateProcess(current, STATUS_SUCCESS);
	  }
     }
   
   KeReleaseSpinLock(&PsProcessListLock, oldIrql);
}

VOID PsInitProcessManagment(VOID)
{
   ANSI_STRING AnsiString;
   PKPROCESS KProcess;
   KIRQL oldIrql;
   
   /*
    * Register the process object type
    */   
   
   PsProcessType = ExAllocatePool(NonPagedPool, sizeof(OBJECT_TYPE));
   
   PsProcessType->TotalObjects = 0;
   PsProcessType->TotalHandles = 0;
   PsProcessType->MaxObjects = ULONG_MAX;
   PsProcessType->MaxHandles = ULONG_MAX;
   PsProcessType->PagedPoolCharge = 0;
   PsProcessType->NonpagedPoolCharge = sizeof(EPROCESS);
   PsProcessType->Dump = NULL;
   PsProcessType->Open = NULL;
   PsProcessType->Close = NULL;
   PsProcessType->Delete = PiDeleteProcess;
   PsProcessType->Parse = NULL;
   PsProcessType->Security = NULL;
   PsProcessType->QueryName = NULL;
   PsProcessType->OkayToClose = NULL;
   PsProcessType->Create = NULL;
   
   RtlInitAnsiString(&AnsiString,"Process");
   RtlAnsiStringToUnicodeString(&PsProcessType->TypeName,&AnsiString,TRUE);
   
   InitializeListHead(&PsProcessListHead);
   KeInitializeSpinLock(&PsProcessListLock);
   
   /*
    * Initialize the system process
    */
   SystemProcess = ObCreateObject(NULL,
				  PROCESS_ALL_ACCESS,
				  NULL,
				  PsProcessType);
   SystemProcess->Pcb.BasePriority = PROCESS_PRIO_NORMAL;
   KeInitializeDispatcherHeader(&SystemProcess->Pcb.DispatcherHeader,
				InternalProcessType,
				sizeof(EPROCESS),
				FALSE);
   KProcess = &SystemProcess->Pcb;  
   
   MmInitializeAddressSpace(SystemProcess, 
			    &SystemProcess->Pcb.AddressSpace);
   ObCreateHandleTable(NULL,FALSE,SystemProcess);
   KProcess->PageTableDirectory = get_page_directory();
   SystemProcess->UniqueProcessId = 
     InterlockedIncrement(&PiNextProcessUniqueId);
   
   KeAcquireSpinLock(&PsProcessListLock, &oldIrql);
   InsertHeadList(&PsProcessListHead, &KProcess->ProcessListEntry);
   InitializeListHead( &KProcess->ThreadListHead );
   KeReleaseSpinLock(&PsProcessListLock, oldIrql);
   
   strcpy(SystemProcess->ImageFileName, "SYSTEM");
   
   ObCreateHandle(SystemProcess,
		  SystemProcess,
		  PROCESS_ALL_ACCESS,
		  FALSE,
		  &SystemProcessHandle);
}

VOID PiDeleteProcess(PVOID ObjectBody)
{
   KIRQL oldIrql;
   
   DPRINT1("PiDeleteProcess(ObjectBody %x)\n",ObjectBody);
   
   KeAcquireSpinLock(&PsProcessListLock, &oldIrql);
   RemoveEntryList(&((PEPROCESS)ObjectBody)->Pcb.ProcessListEntry);
   KeReleaseSpinLock(&PsProcessListLock, oldIrql);
   (VOID)MmReleaseMmInfo((PEPROCESS)ObjectBody);
   ObDeleteHandleTable((PEPROCESS)ObjectBody);
}


static NTSTATUS PsCreatePeb(HANDLE ProcessHandle,
			    PVOID ImageBase,
			    PVOID* RPeb)
{
   NTSTATUS Status;
   PVOID PebBase;
   ULONG PebSize;
   PEB Peb;
   ULONG BytesWritten;
   
   memset(&Peb, 0, sizeof(Peb));
   Peb.ImageBaseAddress = ImageBase;
   
   PebBase = (PVOID)PEB_BASE;
   PebSize = 0x1000;
   Status = NtAllocateVirtualMemory(ProcessHandle,
				    &PebBase,
				    0,
				    &PebSize,
				    MEM_COMMIT,
				    PAGE_READWRITE);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   ZwWriteVirtualMemory(ProcessHandle,
			(PVOID)PEB_BASE,
			&Peb,
			sizeof(Peb),
			&BytesWritten);

   DPRINT("PsCreatePeb: Peb created at %x\n", PebBase);
   
   *RPeb = PebBase;
   
   return(STATUS_SUCCESS);
}


PKPROCESS KeGetCurrentProcess(VOID)
/*
 * FUNCTION: Returns a pointer to the current process
 */
{
   return(&(PsGetCurrentProcess()->Pcb));
}

HANDLE PsGetCurrentProcessId(VOID)
{
   return((HANDLE)PsGetCurrentProcess()->UniqueProcessId);
}

struct _EPROCESS* PsGetCurrentProcess(VOID)
/*
 * FUNCTION: Returns a pointer to the current process
 */
{
   if (PsGetCurrentThread() == NULL || 
       PsGetCurrentThread()->ThreadsProcess == NULL)
     {
	return(SystemProcess);
     }
   else
     {
	return(PsGetCurrentThread()->ThreadsProcess);
     }
}

NTSTATUS STDCALL NtCreateProcess (OUT PHANDLE ProcessHandle,
				  IN ACCESS_MASK DesiredAccess,
				  IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
				  IN HANDLE ParentProcessHandle,
				  IN BOOLEAN InheritObjectTable,
				  IN HANDLE SectionHandle OPTIONAL,
				  IN HANDLE DebugPortHandle OPTIONAL,
				  IN HANDLE ExceptionPortHandle OPTIONAL)
/*
 * FUNCTION: Creates a process.
 * ARGUMENTS:
 *        ProcessHandle (OUT) = Caller supplied storage for the resulting 
 *                              handle
 *        DesiredAccess = Specifies the allowed or desired access to the 
 *                        process can be a combination of 
 *                        STANDARD_RIGHTS_REQUIRED| ..  
 *        ObjectAttribute = Initialized attributes for the object, contains 
 *                          the rootdirectory and the filename
 *        ParentProcess = Handle to the parent process.
 *        InheritObjectTable = Specifies to inherit the objects of the parent 
 *                             process if true.
 *        SectionHandle = Handle to a section object to back the image file
 *        DebugPort = Handle to a DebugPort if NULL the system default debug 
 *                    port will be used.
 *        ExceptionPort = Handle to a exception port. 
 * REMARKS:
 *        This function maps to the win32 CreateProcess. 
 * RETURNS: Status
 */
{
   PEPROCESS Process;
   PEPROCESS ParentProcess;
   PKPROCESS KProcess;
   NTSTATUS Status;
   KIRQL oldIrql;
   PVOID LdrStartupAddr;
   PVOID ImageBase;
   PVOID Peb;
   PEPORT DebugPort;
   PEPORT ExceptionPort;
   
   DPRINT("NtCreateProcess(ObjectAttributes %x)\n",ObjectAttributes);

   Status = ObReferenceObjectByHandle(ParentProcessHandle,
				      PROCESS_CREATE_PROCESS,
				      PsProcessType,
				      UserMode,
				      (PVOID*)&ParentProcess,
				      NULL);

   if (Status != STATUS_SUCCESS)
     {
	DPRINT("NtCreateProcess() = %x\n",Status);
	return(Status);
     }

   Process = ObCreateObject(ProcessHandle,
			    DesiredAccess,
			    ObjectAttributes,
			    PsProcessType);
   KeInitializeDispatcherHeader(&Process->Pcb.DispatcherHeader,
				InternalProcessType,
				sizeof(EPROCESS),
				FALSE);
   KProcess = &Process->Pcb;
   
   KProcess->BasePriority = PROCESS_PRIO_NORMAL;
   MmInitializeAddressSpace(Process,
			    &KProcess->AddressSpace);
   Process->UniqueProcessId = InterlockedIncrement(&PiNextProcessUniqueId);
   Process->InheritedFromUniqueProcessId = ParentProcess->UniqueProcessId;
   ObCreateHandleTable(ParentProcess,
		       InheritObjectTable,
		       Process);
   MmCopyMmInfo(ParentProcess, Process);
   
   KeAcquireSpinLock(&PsProcessListLock, &oldIrql);
   InsertHeadList(&PsProcessListHead, &KProcess->ProcessListEntry);
   InitializeListHead( &KProcess->ThreadListHead );
   KeReleaseSpinLock(&PsProcessListLock, oldIrql);
   
   Process->Pcb.ProcessState = PROCESS_STATE_ACTIVE;
   
   /*
    * Add the debug port
    */
   if (DebugPortHandle != NULL)
     {
	Status = ObReferenceObjectByHandle(DebugPortHandle,
					   PORT_ALL_ACCESS,
					   ExPortType,
					   UserMode,
					   (PVOID*)&DebugPort,
					   NULL);   
	if (!NT_SUCCESS(Status))
	  {
	     ObDereferenceObject(Process);
	     ObDereferenceObject(ParentProcess);
	     ZwClose(*ProcessHandle);
	     *ProcessHandle = NULL;
	     return(Status);
	  }
	Process->DebugPort = DebugPort;
     }
	
   /*
    * Add the exception port
    */
   if (ExceptionPortHandle != NULL)
     {
	Status = ObReferenceObjectByHandle(ExceptionPortHandle,
					   PORT_ALL_ACCESS,
					   ExPortType,
					   UserMode,
					   (PVOID*)&ExceptionPort,
					   NULL);   
	if (!NT_SUCCESS(Status))
	  {
	     ObDereferenceObject(Process);
	     ObDereferenceObject(ParentProcess);
	     ZwClose(*ProcessHandle);
	     *ProcessHandle = NULL;
	     return(Status);
	  }
	Process->ExceptionPort = ExceptionPort;
     }
   
   /*
    * Now we have created the process proper
    */
      
   /*
    * Map ntdll
    */
   Status = LdrpMapSystemDll(*ProcessHandle,
			     &LdrStartupAddr);
   if (!NT_SUCCESS(Status))
     {
	DbgPrint("LdrpMapSystemDll failed (Status %x)\n", Status);
	ObDereferenceObject(Process);
	ObDereferenceObject(ParentProcess);
	return(Status);
     }
   
   /*
    * Map the process image
    */
   if (SectionHandle != NULL)
     {
	DPRINT("Mapping process image\n");
	Status = LdrpMapImage(*ProcessHandle,
			      SectionHandle,
			      &ImageBase);
	if (!NT_SUCCESS(Status))
	  {
	     DbgPrint("LdrpMapImage failed (Status %x)\n", Status);
	     ObDereferenceObject(Process);
	     ObDereferenceObject(ParentProcess);
	     return(Status);
	  }
     }
   else
     {
	ImageBase = NULL;
     }
   
   /*
    * 
    */
   DPRINT("Creating PEB\n");
   Status = PsCreatePeb(*ProcessHandle,
			ImageBase,
			&Peb);
   if (!NT_SUCCESS(Status))
     {
        DbgPrint("NtCreateProcess() Peb creation failed: Status %x\n",Status);
	ObDereferenceObject(Process);
	ObDereferenceObject(ParentProcess);
	ZwClose(*ProcessHandle);
	*ProcessHandle = NULL;       
	return(Status);
     }
   Process->Peb = Peb;
   
   /*
    * Maybe send a message to the creator process's debugger
    */
   if (ParentProcess->DebugPort != NULL)
     {
	LPC_DBG_MESSAGE Message;
	HANDLE FileHandle;
	
	ObCreateHandle(NULL, // Debugger Process
		       NULL, // SectionHandle
		       FILE_ALL_ACCESS,
		       FALSE,
		       &FileHandle);
	
	Message.Header.MessageSize = sizeof(LPC_DBG_MESSAGE);
	Message.Header.DataSize = sizeof(LPC_DBG_MESSAGE) -
	  sizeof(LPC_MESSAGE_HEADER);
	Message.Type = DBG_EVENT_CREATE_PROCESS;
	Message.Data.CreateProcess.FileHandle = FileHandle;
	Message.Data.CreateProcess.Base = ImageBase;
	Message.Data.CreateProcess.EntryPoint = NULL; //
	
	Status = LpcSendDebugMessagePort(ParentProcess->DebugPort,
					 &Message);
     }
   
   ObDereferenceObject(Process);
   ObDereferenceObject(ParentProcess);
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL NtOpenProcess (OUT	PHANDLE		    ProcessHandle,
				IN	ACCESS_MASK	    DesiredAccess,
				IN	POBJECT_ATTRIBUTES  ObjectAttributes,
				IN	PCLIENT_ID	    ClientId)
{
   DPRINT("NtOpenProcess(ProcessHandle %x, DesiredAccess %x, "
	  "ObjectAttributes %x, ClientId %x { UniP %d, UniT %d })\n",
	  ProcessHandle, DesiredAccess, ObjectAttributes, ClientId,
	  ClientId->UniqueProcess, ClientId->UniqueThread);
	  
   
   /*
    * Not sure of the exact semantics 
    */
   if (ObjectAttributes != NULL && ObjectAttributes->ObjectName != NULL &&
       ObjectAttributes->ObjectName->Buffer != NULL)
     {
	NTSTATUS Status;
	PEPROCESS Process;
		
	Status = ObReferenceObjectByName(ObjectAttributes->ObjectName,
					 ObjectAttributes->Attributes,
					 NULL,
					 DesiredAccess,
					 PsProcessType,
					 UserMode,
					 NULL,
					 (PVOID*)&Process);
	if (Status != STATUS_SUCCESS)
	  {
	     return(Status);
	  }
	
	Status = ObCreateHandle(PsGetCurrentProcess(),
				Process,
				DesiredAccess,
				FALSE,
				ProcessHandle);
	ObDereferenceObject(Process);
   
	return(Status);
     }
   else
     {
	KIRQL oldIrql;
	PLIST_ENTRY current_entry;
	PEPROCESS current;
	NTSTATUS Status;
	
	KeAcquireSpinLock(&PsProcessListLock, &oldIrql);
	current_entry = PsProcessListHead.Flink;
	while (current_entry != &PsProcessListHead)
	  {
	     current = CONTAINING_RECORD(current_entry, EPROCESS, 
					 Pcb.ProcessListEntry);
	     if (current->UniqueProcessId == (ULONG)ClientId->UniqueProcess)
	       {
		  ObReferenceObjectByPointer(current,
					     DesiredAccess,
					     PsProcessType,
					     UserMode);
		  KeReleaseSpinLock(&PsProcessListLock, oldIrql);
		  Status = ObCreateHandle(PsGetCurrentProcess(),
					  current,
					  DesiredAccess,
					  FALSE,
					  ProcessHandle);
		  ObDereferenceObject(current);
		  DPRINT("*ProcessHandle %x\n", ProcessHandle);
		  DPRINT("NtOpenProcess() = %x\n", Status);
		  return(Status);			  
	       }
	     current_entry = current_entry->Flink;
	  }
	KeReleaseSpinLock(&PsProcessListLock, oldIrql);
	DPRINT("NtOpenProcess() = STATUS_UNSUCCESSFUL\n");
	return(STATUS_UNSUCCESSFUL);
     }
   return(STATUS_UNSUCCESSFUL);
}


NTSTATUS STDCALL NtQueryInformationProcess (IN	HANDLE ProcessHandle,
					    IN	CINT ProcessInformationClass,
					    OUT	PVOID ProcessInformation,
					    IN	ULONG ProcessInformationLength,
					    OUT	PULONG ReturnLength)
{
   PEPROCESS Process;
   NTSTATUS Status;
   PPROCESS_BASIC_INFORMATION ProcessBasicInformationP;
   
   Status = ObReferenceObjectByHandle(ProcessHandle,
				      PROCESS_SET_INFORMATION,
				      PsProcessType,
				      UserMode,
				      (PVOID*)&Process,
				      NULL);
   if (Status != STATUS_SUCCESS)
     {
	return(Status);
     }
   
   switch (ProcessInformationClass)
     {
      case ProcessBasicInformation:
	ProcessBasicInformationP = (PPROCESS_BASIC_INFORMATION)
	  ProcessInformation;
	ProcessBasicInformationP->ExitStatus = Process->ExitStatus;
	ProcessBasicInformationP->PebBaseAddress = Process->Peb;
	ProcessBasicInformationP->AffinityMask = Process->Pcb.Affinity;
        ProcessBasicInformationP->UniqueProcessId =
          Process->UniqueProcessId;
        ProcessBasicInformationP->InheritedFromUniqueProcessId =
          Process->InheritedFromUniqueProcessId;
	Status = STATUS_SUCCESS;
	break;
	
      case ProcessQuotaLimits:
      case ProcessIoCounters:
      case ProcessVmCounters:
      case ProcessTimes:
      case ProcessBasePriority:
      case ProcessRaisePriority:
      case ProcessDebugPort:
      case ProcessExceptionPort:
      case ProcessAccessToken:
      case ProcessLdtInformation:
      case ProcessLdtSize:
	Status = STATUS_NOT_IMPLEMENTED;
	break;
	
      case ProcessDefaultHardErrorMode:
	*((PULONG)ProcessInformation) = Process->DefaultHardErrorProcessing;
	break;
	
      case ProcessIoPortHandlers:
      case ProcessWorkingSetWatch:
      case ProcessUserModeIOPL:
      case ProcessEnableAlignmentFaultFixup:
      case ProcessPriorityClass:
      case ProcessWx86Information:
      case ProcessHandleCount:
      case ProcessAffinityMask:
      default:
	Status = STATUS_NOT_IMPLEMENTED;
     }
   ObDereferenceObject(Process);
   return(Status);
}

NTSTATUS PspAssignPrimaryToken(PEPROCESS Process,
			       HANDLE TokenHandle)
{
   PACCESS_TOKEN Token;
   PACCESS_TOKEN OldToken;
   NTSTATUS Status;
   
   Status = ObReferenceObjectByHandle(TokenHandle,
				      0,
				      SeTokenType,
				      UserMode,
				      (PVOID*)&Token,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   Status = SeExchangePrimaryToken(Process, Token, &OldToken);
   if (!NT_SUCCESS(Status))
     {
	ObDereferenceObject(OldToken);
     }
   ObDereferenceObject(Token);
   return(Status);
}

NTSTATUS STDCALL NtSetInformationProcess(IN HANDLE ProcessHandle,
					 IN CINT ProcessInformationClass,
					 IN PVOID ProcessInformation,
					 IN ULONG ProcessInformationLength)
{
   PEPROCESS Process;
   NTSTATUS Status;
   PPROCESS_BASIC_INFORMATION ProcessBasicInformationP;
   PHANDLE ProcessAccessTokenP;
   
   Status = ObReferenceObjectByHandle(ProcessHandle,
				      PROCESS_SET_INFORMATION,
				      PsProcessType,
				      UserMode,
				      (PVOID*)&Process,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   switch (ProcessInformationClass)
     {
      case ProcessBasicInformation:
	ProcessBasicInformationP = (PPROCESS_BASIC_INFORMATION)
	  ProcessInformation;
	memset(ProcessBasicInformationP, 0, sizeof(PROCESS_BASIC_INFORMATION));
	Process->Pcb.Affinity = ProcessBasicInformationP->AffinityMask;
	Status = STATUS_SUCCESS;
	break;
	
      case ProcessQuotaLimits:
      case ProcessIoCounters:
      case ProcessVmCounters:
      case ProcessTimes:
      case ProcessBasePriority:
      case ProcessRaisePriority:
      case ProcessDebugPort:
      case ProcessExceptionPort:
      case ProcessAccessToken:
	ProcessAccessTokenP = (PHANDLE)ProcessInformation;
	Status = PspAssignPrimaryToken(Process, *ProcessAccessTokenP);
	break;
	
      case ProcessImageFileName:
	memcpy(Process->ImageFileName, ProcessInformation, 8);
//	DPRINT1("Process->ImageFileName %.8s\n", Process->ImageFileName);
	Status = STATUS_SUCCESS;
	break;
	  
      case ProcessLdtInformation:
      case ProcessLdtSize:
      case ProcessDefaultHardErrorMode:
      case ProcessIoPortHandlers:
      case ProcessWorkingSetWatch:
      case ProcessUserModeIOPL:
      case ProcessEnableAlignmentFaultFixup:
      case ProcessPriorityClass:
      case ProcessWx86Information:
      case ProcessHandleCount:
      case ProcessAffinityMask:
      default:
	Status = STATUS_NOT_IMPLEMENTED;
     }
   ObDereferenceObject(Process);
   return(Status);
}


#if 0
/**********************************************************************
 * NAME							INTERNAL
 * 	PiSnapshotProcessTable
 *
 * DESCRIPTION
 * 	Compute the size of a process+thread snapshot as 
 * 	expected by NtQuerySystemInformation.
 *
 * RETURN VALUE
 * 	0 on error; otherwise the size, in bytes of the buffer
 * 	required to write a full snapshot.
 *
 * NOTE
 * 	We assume (sizeof (PVOID) == sizeof (ULONG)) holds.
 */
NTSTATUS
STDCALL
PiSnapshotProcessTable (
	IN	PVOID	SnapshotBuffer,
	IN	ULONG	Size,
	IN	PULONG	pRequiredSize
	)
{
	KIRQL		OldIrql;
	PLIST_ENTRY	CurrentEntry;
	PEPROCESS	Current;
	
	ULONG		RequiredSize = 0L;
	BOOLEAN		SizeOnly = FALSE;

	ULONG		SpiSizeLast = 0L;
	ULONG		SpiSizeCurrent = 0L;
	
	PSYSTEM_PROCESS_INFORMATION	pInfoP = (PSYSTEM_PROCESS_INFORMATION) SnapshotBuffer;
	PSYSTEM_THREAD_INFO		pInfoT = NULL;
	

	/*
	 * Lock the process list.
	 */
	KeAcquireSpinLock (
		& PsProcessListLock,
		& OldIrql
		);
	/*
	 * Scan the process list. Since the
	 * list is circular, the guard is false
	 * after the last process.
	 */
	for (	CurrentEntry = PsProcessListHead.Flink;
		(CurrentEntry != & PsProcessListHead);
		CurrentEntry = CurrentEntry->Flink
		)
	{
		/* 
		 * Get a reference to the 
		 * process object we are
		 * handling.
		 */
		Current = CONTAINING_RECORD(
				CurrentEntry,
				EPROCESS, 
				Pcb.ProcessListEntry
				);
		/* FIXME: assert (NULL != Current) */
		/*
		 * Compute how much space is
		 * occupied in the snapshot
		 * by adding this process info.
		 */
		SpiSizeCurrent = 
			sizeof (SYSTEM_PROCESS_INFORMATION)
			+ (
				(Current->ThreadCount - 1)
				* sizeof (SYSTEM_THREAD_INFORMATION)
				);
		RequiredSize += SpiSizeCurrent;
		/*
		 * Do not write process data in the
		 * buffer if it is too small.
		 */
		if (TRUE == SizeOnly) continue;
		/*
		 * Check if the buffer can contain
		 * the full snapshot.
		 */
		if (Size < RequiredSize)
		{
			SizeOnly = TRUE;
			continue;
		}
		/*
		 * Compute the offset of the 
		 * SYSTEM_PROCESS_INFORMATION
		 * descriptor in the snapshot 
		 * buffer for this process.
		 */
		if (0L != SpiSizeLast)
		{
			(ULONG) pInfoP += SpiSizeLast;
			/* Save current process SpiSize */
			SpiSizeLast = SpiSizeCurrent;
		}
		/*
		 * Write process data in the buffer.
		 */
		pInfoP->RelativeOffset = SpiSizeCurrent;
		/* PROCESS */
		pInfoP->ThreadCount =
		pInfoP->ProcessId = Current->UniqueProcessId;
		RtlInitUnicodeString (
			& pInfoP->Name,
			Current->ImageFileName
			);
		/* THREAD */
		for (	ThreadIndex = 0;
			(ThreadIndex < Current->ThreadCount);
			ThreadIndex ++
			)
		{
		}
	}
	/*
	 * Unlock the process list.
	 */
	KeReleaseSpinLock (
		& PsProcessListLock,
		OldIrql
		);
	/*
	 * Return the proper error status code,
	 * if the buffer was too small.
	 */
	if (TRUE == SizeOnly)
	{
		*pRequiredSize = RequiredSize;
		return STATUS_INFO_LENGTH_MISMATCH;
	}
	/*
	 * Mark the end of the snapshot.
	 */
	pInfoP->RelativeOffset = 0L;
	/* OK */	
	return STATUS_SUCCESS;
}
#endif

/* EOF */
