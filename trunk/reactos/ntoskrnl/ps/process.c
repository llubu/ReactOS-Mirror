/* $Id$
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

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS ******************************************************************/

PEPROCESS EXPORTED PsInitialSystemProcess = NULL;
PEPROCESS PsIdleProcess = NULL;

POBJECT_TYPE EXPORTED PsProcessType = NULL;

LIST_ENTRY PsProcessListHead;
static KSPIN_LOCK PsProcessListLock;
static ULONG PiNextProcessUniqueId = 0; /* TODO */
static LARGE_INTEGER ShortPsLockDelay, PsLockTimeout;

static GENERIC_MAPPING PiProcessMapping = {STANDARD_RIGHTS_READ | PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
					   STANDARD_RIGHTS_WRITE | PROCESS_CREATE_PROCESS | PROCESS_CREATE_THREAD |
                       PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_DUP_HANDLE |
                       PROCESS_TERMINATE | PROCESS_SET_QUOTA | PROCESS_SET_INFORMATION | PROCESS_SET_PORT,
					   STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE,
					   PROCESS_ALL_ACCESS};

#define MAX_PROCESS_NOTIFY_ROUTINE_COUNT    8
#define MAX_LOAD_IMAGE_NOTIFY_ROUTINE_COUNT  8

static PCREATE_PROCESS_NOTIFY_ROUTINE
PiProcessNotifyRoutine[MAX_PROCESS_NOTIFY_ROUTINE_COUNT];
static PLOAD_IMAGE_NOTIFY_ROUTINE
PiLoadImageNotifyRoutine[MAX_LOAD_IMAGE_NOTIFY_ROUTINE_COUNT];


typedef struct
{
    WORK_QUEUE_ITEM WorkQueueItem;
    KEVENT Event;
    PEPROCESS Process;
    BOOLEAN IsWorkerQueue;
} DEL_CONTEXT, *PDEL_CONTEXT;

/* FUNCTIONS *****************************************************************/

VOID
STDCALL
PsExitSpecialApc(PKAPC Apc, 
		 PKNORMAL_ROUTINE *NormalRoutine,
		 PVOID *NormalContext,
		 PVOID *SystemArgument1,
		 PVOID *SystemArgument2)
{
}

PEPROCESS
PsGetNextProcess(PEPROCESS OldProcess)
{
   KIRQL oldIrql;
   PEPROCESS NextProcess;
   NTSTATUS Status;
   
   if (OldProcess == NULL)
     {
       Status = ObReferenceObjectByPointer(PsInitialSystemProcess,
				           PROCESS_ALL_ACCESS,
				           PsProcessType,
				           KernelMode);   
       if (!NT_SUCCESS(Status))
         {
	   CPRINT("PsGetNextProcess(): ObReferenceObjectByPointer failed for PsInitialSystemProcess\n");
	   KEBUGCHECK(0);
	 }
       return PsInitialSystemProcess;
     }
   
   KeAcquireSpinLock(&PsProcessListLock, &oldIrql);
   NextProcess = OldProcess;
   while (1)
     {
       if (NextProcess->ProcessListEntry.Blink == &PsProcessListHead)
         {
	   NextProcess = CONTAINING_RECORD(PsProcessListHead.Blink,
					   EPROCESS,
					   ProcessListEntry);
         }
       else
         {
	   NextProcess = CONTAINING_RECORD(NextProcess->ProcessListEntry.Blink,
					   EPROCESS,
					   ProcessListEntry);
         }
       Status = ObReferenceObjectByPointer(NextProcess,
				           PROCESS_ALL_ACCESS,
				           PsProcessType,
				           KernelMode);   
       if (NT_SUCCESS(Status))
         {
	   break;
	 }
       else if (Status == STATUS_PROCESS_IS_TERMINATING)
         {
	   continue;
	 }
       else if (!NT_SUCCESS(Status))
         {
	   CPRINT("PsGetNextProcess(): ObReferenceObjectByPointer failed\n");
	   KEBUGCHECK(0);
	 }
     }

   KeReleaseSpinLock(&PsProcessListLock, oldIrql);
   ObDereferenceObject(OldProcess);
   
   return(NextProcess);
}


/*
 * @implemented
 */
NTSTATUS STDCALL 
NtOpenProcessToken(IN	HANDLE		ProcessHandle,
		   IN	ACCESS_MASK	DesiredAccess,
		   OUT	PHANDLE		TokenHandle)
{
   PACCESS_TOKEN Token;
   HANDLE hToken;
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
			   &hToken);
   ObDereferenceObject(Token);
   
   if(NT_SUCCESS(Status))
     {
        Status = MmCopyToCaller(TokenHandle, &hToken, sizeof(HANDLE));
     }
   return(Status);
}


/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtOpenProcessTokenEx(
    IN HANDLE ProcessHandle,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG HandleAttributes,
    OUT PHANDLE TokenHandle
    )
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}


/*
 * @implemented
 */
PACCESS_TOKEN STDCALL
PsReferencePrimaryToken(PEPROCESS Process)
{
   ObReferenceObjectByPointer(Process->Token,
			      TOKEN_ALL_ACCESS,
			      SepTokenObjectType,
			      UserMode);
   return(Process->Token);
}


NTSTATUS
PsOpenTokenOfProcess(HANDLE ProcessHandle,
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


VOID 
PiKillMostProcesses(VOID)
{
   KIRQL oldIrql;
   PLIST_ENTRY current_entry;
   PEPROCESS current;
   
   KeAcquireSpinLock(&PsProcessListLock, &oldIrql);
   
   current_entry = PsProcessListHead.Flink;
   while (current_entry != &PsProcessListHead)
     {
	current = CONTAINING_RECORD(current_entry, EPROCESS, 
				    ProcessListEntry);
	current_entry = current_entry->Flink;
	
	if (current->UniqueProcessId != PsInitialSystemProcess->UniqueProcessId &&
	    current->UniqueProcessId != (ULONG)PsGetCurrentProcessId())
	  {
	     PiTerminateProcessThreads(current, STATUS_SUCCESS);
	  }
     }
   
   KeReleaseSpinLock(&PsProcessListLock, oldIrql);
}


VOID INIT_FUNCTION
PsInitProcessManagment(VOID)
{
   PKPROCESS KProcess;
   KIRQL oldIrql;
   NTSTATUS Status;
   
   ShortPsLockDelay.QuadPart = -100LL;
   PsLockTimeout.QuadPart = -10000000LL; /* one second */
   /*
    * Register the process object type
    */
   
   PsProcessType = ExAllocatePool(NonPagedPool, sizeof(OBJECT_TYPE));

   PsProcessType->Tag = TAG('P', 'R', 'O', 'C');
   PsProcessType->TotalObjects = 0;
   PsProcessType->TotalHandles = 0;
   PsProcessType->MaxObjects = ULONG_MAX;
   PsProcessType->MaxHandles = ULONG_MAX;
   PsProcessType->PagedPoolCharge = 0;
   PsProcessType->NonpagedPoolCharge = sizeof(EPROCESS);
   PsProcessType->Mapping = &PiProcessMapping;
   PsProcessType->Dump = NULL;
   PsProcessType->Open = NULL;
   PsProcessType->Close = NULL;
   PsProcessType->Delete = PiDeleteProcess;
   PsProcessType->Parse = NULL;
   PsProcessType->Security = NULL;
   PsProcessType->QueryName = NULL;
   PsProcessType->OkayToClose = NULL;
   PsProcessType->Create = NULL;
   PsProcessType->DuplicationNotify = NULL;
   
   RtlRosInitUnicodeStringFromLiteral(&PsProcessType->TypeName, L"Process");
   
   ObpCreateTypeObject(PsProcessType);

   InitializeListHead(&PsProcessListHead);
   KeInitializeSpinLock(&PsProcessListLock);

   RtlZeroMemory(PiProcessNotifyRoutine, sizeof(PiProcessNotifyRoutine));
   RtlZeroMemory(PiLoadImageNotifyRoutine, sizeof(PiLoadImageNotifyRoutine));
   
   /*
    * Initialize the system process
    */
   Status = ObCreateObject(KernelMode,
			   PsProcessType,
			   NULL,
			   KernelMode,
			   NULL,
			   sizeof(EPROCESS),
			   0,
			   0,
			   (PVOID*)&PsInitialSystemProcess);
   if (!NT_SUCCESS(Status))
     {
	return;
     }
   
   /* System threads may run on any processor. */
   PsInitialSystemProcess->Pcb.Affinity = 0xFFFFFFFF;
   PsInitialSystemProcess->Pcb.IopmOffset = 0xffff;
   PsInitialSystemProcess->Pcb.LdtDescriptor[0] = 0;
   PsInitialSystemProcess->Pcb.LdtDescriptor[1] = 0;
   PsInitialSystemProcess->Pcb.BasePriority = PROCESS_PRIO_NORMAL;
   PsInitialSystemProcess->Pcb.ThreadQuantum = 6;
   InitializeListHead(&PsInitialSystemProcess->Pcb.ThreadListHead);
   KeInitializeDispatcherHeader(&PsInitialSystemProcess->Pcb.DispatcherHeader,
				InternalProcessType,
				sizeof(EPROCESS),
				FALSE);
   KProcess = &PsInitialSystemProcess->Pcb;
   
   MmInitializeAddressSpace(PsInitialSystemProcess,
			    &PsInitialSystemProcess->AddressSpace);
   ObCreateHandleTable(NULL,FALSE,PsInitialSystemProcess);
   
   KeInitializeEvent(&PsInitialSystemProcess->LockEvent, SynchronizationEvent, FALSE);
   PsInitialSystemProcess->LockCount = 0;
   PsInitialSystemProcess->LockOwner = NULL;

#if defined(__GNUC__)
   KProcess->DirectoryTableBase = 
     (LARGE_INTEGER)(LONGLONG)(ULONG)MmGetPageDirectory();
#else
   {
     LARGE_INTEGER dummy;
     dummy.QuadPart = (LONGLONG)(ULONG)MmGetPageDirectory();
     KProcess->DirectoryTableBase = dummy;
   }
#endif

   PsInitialSystemProcess->UniqueProcessId = 
     InterlockedIncrementUL(&PiNextProcessUniqueId); /* TODO */
   PsInitialSystemProcess->Win32WindowStation = (HANDLE)0;
   
   KeAcquireSpinLock(&PsProcessListLock, &oldIrql);
   InsertHeadList(&PsProcessListHead, 
		  &PsInitialSystemProcess->ProcessListEntry);
   InitializeListHead(&PsInitialSystemProcess->ThreadListHead);
   KeReleaseSpinLock(&PsProcessListLock, oldIrql);
   
   strcpy(PsInitialSystemProcess->ImageFileName, "SYSTEM");

   SepCreateSystemProcessToken(PsInitialSystemProcess);
}

VOID STDCALL
PiDeleteProcessWorker(PVOID pContext)
{
  KIRQL oldIrql;
  PDEL_CONTEXT Context;
  PEPROCESS CurrentProcess;
  PEPROCESS Process;

  Context = (PDEL_CONTEXT)pContext;
  Process = Context->Process;
  CurrentProcess = PsGetCurrentProcess();

  DPRINT("PiDeleteProcess(ObjectBody %x)\n",Process);

  if (CurrentProcess != Process)
    {
      KeAttachProcess(&Process->Pcb);
    }

  KeAcquireSpinLock(&PsProcessListLock, &oldIrql);
  RemoveEntryList(&Process->ProcessListEntry);
  KeReleaseSpinLock(&PsProcessListLock, oldIrql);

  /* KDB hook */
  KDB_DELETEPROCESS_HOOK(Process);

  ObDereferenceObject(Process->Token);
  ObDeleteHandleTable(Process);

  if (CurrentProcess != Process)
    {
      KeDetachProcess();
    }

  MmReleaseMmInfo(Process);
  if (Context->IsWorkerQueue)
    {
      KeSetEvent(&Context->Event, IO_NO_INCREMENT, FALSE);
    }
}

VOID STDCALL 
PiDeleteProcess(PVOID ObjectBody)
{
  DEL_CONTEXT Context;

  Context.Process = (PEPROCESS)ObjectBody;

  if (PsGetCurrentProcess() == Context.Process ||
      PsGetCurrentThread()->ThreadsProcess == Context.Process)
    {
       KEBUGCHECK(0);
    }

  if (PsGetCurrentThread()->ThreadsProcess == PsGetCurrentProcess())
    {
      Context.IsWorkerQueue = FALSE;
      PiDeleteProcessWorker(&Context);
    }
  else
    {
      Context.IsWorkerQueue = TRUE;
      KeInitializeEvent(&Context.Event, NotificationEvent, FALSE);
      ExInitializeWorkItem (&Context.WorkQueueItem, PiDeleteProcessWorker, &Context);
      ExQueueWorkItem(&Context.WorkQueueItem, HyperCriticalWorkQueue);
      if (KeReadStateEvent(&Context.Event) == 0)
        {
          KeWaitForSingleObject(&Context.Event, Executive, KernelMode, FALSE, NULL);
	}
    }

  if(((PEPROCESS)ObjectBody)->Win32Process != NULL)
  {
    /* delete the W32PROCESS structure if there's one associated */
    ExFreePool (((PEPROCESS)ObjectBody)->Win32Process);
  }
}

static NTSTATUS
PsCreatePeb(HANDLE ProcessHandle,
	    PEPROCESS Process,
	    PVOID ImageBase)
{
  ULONG AllocSize;
  ULONG PebSize;
  PPEB Peb;
  LARGE_INTEGER SectionOffset;
  ULONG ViewSize;
  PVOID TableBase;
  NTSTATUS Status;

  /* Allocate the Process Environment Block (PEB) */
  Process->TebBlock = (PVOID) MM_ROUND_DOWN(PEB_BASE, MM_VIRTMEM_GRANULARITY);
  AllocSize = MM_VIRTMEM_GRANULARITY;
  Status = NtAllocateVirtualMemory(ProcessHandle,
				   &Process->TebBlock,
				   0,
				   &AllocSize,
				   MEM_RESERVE,
				   PAGE_READWRITE);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("NtAllocateVirtualMemory() failed (Status %lx)\n", Status);
      return(Status);
    }
  ASSERT((ULONG_PTR) Process->TebBlock <= PEB_BASE &&
         PEB_BASE + PAGE_SIZE <= (ULONG_PTR) Process->TebBlock + AllocSize);
  Peb = (PPEB)PEB_BASE;
  PebSize = PAGE_SIZE;
  Status = NtAllocateVirtualMemory(ProcessHandle,
				   (PVOID*)&Peb,
				   0,
				   &PebSize,
				   MEM_COMMIT,
				   PAGE_READWRITE);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("NtAllocateVirtualMemory() failed (Status %lx)\n", Status);
      return(Status);
    }
  DPRINT("Peb %p  PebSize %lu\n", Peb, PebSize);
  ASSERT((PPEB) PEB_BASE == Peb && PAGE_SIZE <= PebSize);
  Process->TebLastAllocated = (PVOID) Peb;

  ViewSize = 0;
  SectionOffset.QuadPart = (ULONGLONG)0;
  TableBase = NULL;
  Status = MmMapViewOfSection(NlsSectionObject,
			      Process,
			      &TableBase,
			      0,
			      0,
			      &SectionOffset,
			      &ViewSize,
			      ViewShare,
			      MEM_TOP_DOWN,
			      PAGE_READONLY);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("MmMapViewOfSection() failed (Status %lx)\n", Status);
      return(Status);
    }
  DPRINT("TableBase %p  ViewSize %lx\n", TableBase, ViewSize);

  KeAttachProcess(&Process->Pcb);

  /* Initialize the PEB */
  RtlZeroMemory(Peb, sizeof(PEB));
  Peb->ImageBaseAddress = ImageBase;

  Peb->OSMajorVersion = 4;
  Peb->OSMinorVersion = 0;
  Peb->OSBuildNumber = 1381;
  Peb->OSPlatformId = 2; //VER_PLATFORM_WIN32_NT;
  Peb->SPMajorVersion = 6;

  Peb->AnsiCodePageData     = (char*)TableBase + NlsAnsiTableOffset;
  Peb->OemCodePageData      = (char*)TableBase + NlsOemTableOffset;
  Peb->UnicodeCaseTableData = (char*)TableBase + NlsUnicodeTableOffset;

  Process->Peb = Peb;
  KeDetachProcess();

  DPRINT("PsCreatePeb: Peb created at %p\n", Peb);

  return(STATUS_SUCCESS);
}


PKPROCESS
KeGetCurrentProcess(VOID)
/*
 * FUNCTION: Returns a pointer to the current process
 */
{
  return(&(PsGetCurrentProcess()->Pcb));
}

/*
 * Warning: Even though it returns HANDLE, it's not a real HANDLE but really a
 * ULONG ProcessId! (Skywing)
 */
/*
 * @implemented
 */
HANDLE STDCALL
PsGetCurrentProcessId(VOID)
{
  return((HANDLE)PsGetCurrentProcess()->UniqueProcessId);
}

/*
 * @unimplemented
 */
ULONG
STDCALL
PsGetCurrentProcessSessionId (
    	VOID
	)
{
	return PsGetCurrentProcess()->SessionId;
}

/*
 * FUNCTION: Returns a pointer to the current process
 *
 * @implemented
 */
PEPROCESS STDCALL
IoGetCurrentProcess(VOID)
{
   if (PsGetCurrentThread() == NULL || 
       PsGetCurrentThread()->Tcb.ApcState.Process == NULL)
     {
	return(PsInitialSystemProcess);
     }
   else
     {
	return(PEPROCESS)(PsGetCurrentThread()->Tcb.ApcState.Process);
     }
}

/*
 * @implemented
 */
NTSTATUS STDCALL
PsCreateSystemProcess(PHANDLE ProcessHandle,
		      ACCESS_MASK DesiredAccess,
		      POBJECT_ATTRIBUTES ObjectAttributes)
{
   HANDLE SystemProcessHandle;
   NTSTATUS Status;
   
   /* FIXME - what about security? should there be any privilege checks or something
              security related? */
   
   Status = ObCreateHandle(PsGetCurrentProcess(),
                           PsInitialSystemProcess,
                           PROCESS_CREATE_PROCESS | PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION,
                           FALSE,
                           &SystemProcessHandle);
   if(!NT_SUCCESS(Status))
   {
      DPRINT1("Failed to create a handle for the system process!\n");
      return Status;
   }
   
   Status = NtCreateProcess(ProcessHandle,
			    DesiredAccess,
			    ObjectAttributes,
			    SystemProcessHandle,
			    FALSE,
			    NULL,
			    NULL,
			    NULL);

   NtClose(SystemProcessHandle);
   
   return Status;
}

NTSTATUS STDCALL
NtCreateProcess(OUT PHANDLE ProcessHandle,
		IN ACCESS_MASK DesiredAccess,
		IN POBJECT_ATTRIBUTES ObjectAttributes  OPTIONAL,
		IN HANDLE ParentProcess,
		IN BOOLEAN InheritObjectTable,
		IN HANDLE SectionHandle  OPTIONAL,
		IN HANDLE DebugPort  OPTIONAL,
		IN HANDLE ExceptionPort  OPTIONAL)
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
   PEPROCESS pParentProcess;
   PKPROCESS KProcess;
   NTSTATUS Status;
   KIRQL oldIrql;
   PVOID LdrStartupAddr;
   PVOID ImageBase;
   PEPORT pDebugPort;
   PEPORT pExceptionPort;
   PVOID BaseAddress;
   PMEMORY_AREA MemoryArea;
   PHYSICAL_ADDRESS BoundaryAddressMultiple;

   DPRINT("NtCreateProcess(ObjectAttributes %x)\n",ObjectAttributes);

   BoundaryAddressMultiple.QuadPart = 0;
   
   Status = ObReferenceObjectByHandle(ParentProcess,
				      PROCESS_CREATE_PROCESS,
				      PsProcessType,
				      ExGetPreviousMode(),
				      (PVOID*)&pParentProcess,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	DPRINT("NtCreateProcess() = %x\n",Status);
	return(Status);
     }

   Status = ObCreateObject(ExGetPreviousMode(),
			   PsProcessType,
			   ObjectAttributes,
			   ExGetPreviousMode(),
			   NULL,
			   sizeof(EPROCESS),
			   0,
			   0,
			   (PVOID*)&Process);
   if (!NT_SUCCESS(Status))
     {
	ObDereferenceObject(pParentProcess);
	DPRINT("ObCreateObject() = %x\n",Status);
	return(Status);
     }

  Status = ObInsertObject ((PVOID)Process,
			   NULL,
			   DesiredAccess,
			   0,
			   NULL,
			   ProcessHandle);
  if (!NT_SUCCESS(Status))
    {
      ObDereferenceObject (Process);
      ObDereferenceObject (pParentProcess);
      DPRINT("ObInsertObject() = %x\n",Status);
      return Status;
    }

   KeInitializeDispatcherHeader(&Process->Pcb.DispatcherHeader,
				InternalProcessType,
				sizeof(EPROCESS),
				FALSE);
   KProcess = &Process->Pcb;
   /* Inherit parent process's affinity. */
   KProcess->Affinity = pParentProcess->Pcb.Affinity;
   KProcess->BasePriority = PROCESS_PRIO_NORMAL;
   KProcess->IopmOffset = 0xffff;
   KProcess->LdtDescriptor[0] = 0;
   KProcess->LdtDescriptor[1] = 0;
   InitializeListHead(&KProcess->ThreadListHead);
   KProcess->ThreadQuantum = 6;
   KProcess->AutoAlignment = 0;
   MmInitializeAddressSpace(Process,
			    &Process->AddressSpace);
   Process->UniqueProcessId = InterlockedIncrementUL(&PiNextProcessUniqueId); /* TODO */
   Process->InheritedFromUniqueProcessId = 
     (HANDLE)pParentProcess->UniqueProcessId;
   ObCreateHandleTable(pParentProcess,
		       InheritObjectTable,
		       Process);
   MmCopyMmInfo(ParentProcess, Process);
   
   KeInitializeEvent(&Process->LockEvent, SynchronizationEvent, FALSE);
   Process->LockCount = 0;
   Process->LockOwner = NULL;
   
   Process->Win32WindowStation = (HANDLE)0;

   KeAcquireSpinLock(&PsProcessListLock, &oldIrql);
   InsertHeadList(&PsProcessListHead, &Process->ProcessListEntry);
   InitializeListHead(&Process->ThreadListHead);
   KeReleaseSpinLock(&PsProcessListLock, oldIrql);

   ExInitializeFastMutex(&Process->TebLock);
   Process->Pcb.State = PROCESS_STATE_ACTIVE;
   
   /*
    * Add the debug port
    */
   if (DebugPort != NULL)
     {
	Status = ObReferenceObjectByHandle(DebugPort,
					   PORT_ALL_ACCESS,
					   LpcPortObjectType,
					   UserMode,
					   (PVOID*)&pDebugPort,
					   NULL);   
	if (!NT_SUCCESS(Status))
	  {
	     ObDereferenceObject(Process);
	     ObDereferenceObject(pParentProcess);
	     ZwClose(*ProcessHandle);
	     *ProcessHandle = NULL;
	     return(Status);
	  }
	Process->DebugPort = pDebugPort;
     }
	
   /*
    * Add the exception port
    */
   if (ExceptionPort != NULL)
     {
	Status = ObReferenceObjectByHandle(ExceptionPort,
					   PORT_ALL_ACCESS,
					   LpcPortObjectType,
					   UserMode,
					   (PVOID*)&pExceptionPort,
					   NULL);   
	if (!NT_SUCCESS(Status))
	  {
	     ObDereferenceObject(Process);
	     ObDereferenceObject(pParentProcess);
	     ZwClose(*ProcessHandle);
	     *ProcessHandle = NULL;
	     return(Status);
	  }
	Process->ExceptionPort = pExceptionPort;
     }
   
   /*
    * Now we have created the process proper
    */

   MmLockAddressSpace(&Process->AddressSpace);

   /* Protect the highest 64KB of the process address space */
   BaseAddress = (PVOID)MmUserProbeAddress;
   Status = MmCreateMemoryArea(Process,
			       &Process->AddressSpace,
			       MEMORY_AREA_NO_ACCESS,
			       &BaseAddress,
			       0x10000,
			       PAGE_NOACCESS,
			       &MemoryArea,
			       FALSE,
			       FALSE,
			       BoundaryAddressMultiple);
   if (!NT_SUCCESS(Status))
     {
	MmUnlockAddressSpace(&Process->AddressSpace);
	DPRINT1("Failed to protect the highest 64KB of the process address space\n");
	KEBUGCHECK(0);
     }

   /* Protect the lowest 64KB of the process address space */
#if 0
   BaseAddress = (PVOID)0x00000000;
   Status = MmCreateMemoryArea(Process,
			       &Process->AddressSpace,
			       MEMORY_AREA_NO_ACCESS,
			       &BaseAddress,
			       0x10000,
			       PAGE_NOACCESS,
			       &MemoryArea,
			       FALSE,
			       FALSE,
			       BoundaryAddressMultiple);
   if (!NT_SUCCESS(Status))
     {
	MmUnlockAddressSpace(&Process->AddressSpace);
	DPRINT1("Failed to protect the lowest 64KB of the process address space\n");
	KEBUGCHECK(0);
     }
#endif

   /* Protect the 60KB above the shared user page */
   BaseAddress = (char*)USER_SHARED_DATA + PAGE_SIZE;
   Status = MmCreateMemoryArea(Process,
			       &Process->AddressSpace,
			       MEMORY_AREA_NO_ACCESS,
			       &BaseAddress,
			       0x10000 - PAGE_SIZE,
			       PAGE_NOACCESS,
			       &MemoryArea,
			       FALSE,
			       FALSE,
			       BoundaryAddressMultiple);
   if (!NT_SUCCESS(Status))
     {
	MmUnlockAddressSpace(&Process->AddressSpace);
	DPRINT1("Failed to protect the memory above the shared user page\n");
	KEBUGCHECK(0);
     }

   /* Create the shared data page */
   BaseAddress = (PVOID)USER_SHARED_DATA;
   Status = MmCreateMemoryArea(Process,
			       &Process->AddressSpace,
			       MEMORY_AREA_SHARED_DATA,
			       &BaseAddress,
			       PAGE_SIZE,
			       PAGE_READONLY,
			       &MemoryArea,
			       FALSE,
			       FALSE,
			       BoundaryAddressMultiple);
   MmUnlockAddressSpace(&Process->AddressSpace);
   if (!NT_SUCCESS(Status))
     {
	DPRINT1("Failed to create shared data page\n");
	KEBUGCHECK(0);
     }

   if (SectionHandle != NULL)
     {
        PSECTION_OBJECT SectionObject;
        UNICODE_STRING FileName;
        PWCHAR szSrc;
        PCHAR szDest;
        USHORT lnFName = 0;
        
        /*
         * Determine the image file name and save it to the EPROCESS structure
         */
        Status = ObReferenceObjectByHandle(SectionHandle,
                                           0,
                                           MmSectionObjectType,
                                           UserMode,
                                           (PVOID*)&SectionObject,
                                           NULL);
	if (!NT_SUCCESS(Status))
	  {
	     DbgPrint("Failed to reference section object\n", Status);
	     ObDereferenceObject(Process);
	     ObDereferenceObject(pParentProcess);
	     return(Status);
	  }

        FileName = SectionObject->FileObject->FileName;
        szSrc = (PWCHAR)(FileName.Buffer + (FileName.Length / sizeof(WCHAR)) - 1);
        while(szSrc >= FileName.Buffer)
        {
          if(*szSrc == L'\\')
          {
            szSrc++;
            break;
          }
          else
          {
            szSrc--;
            lnFName++;
          }
        }
        
        /* copy the image file name to the process and truncate it to 15 characters
           if necessary */
        szDest = Process->ImageFileName;
        lnFName = min(lnFName, sizeof(Process->ImageFileName) - 1);
        while(lnFName-- > 0)
        {
          *(szDest++) = (UCHAR)*(szSrc++);
        }
        *szDest = '\0';
        

        ObDereferenceObject(SectionObject);
     }
   else
     {
        Process->ImageFileName[0] = '\0';
     }

   /*
    * Map ntdll
    */
   Status = LdrpMapSystemDll(*ProcessHandle,
			     &LdrStartupAddr);
   if (!NT_SUCCESS(Status))
     {
	DbgPrint("LdrpMapSystemDll failed (Status %x)\n", Status);
	ObDereferenceObject(Process);
	ObDereferenceObject(pParentProcess);
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
	     ObDereferenceObject(pParentProcess);
	     return(Status);
	  }
     }
   else
     {
	ImageBase = NULL;
     }
   
  /*
   * Duplicate the token
   */
  Status = SepInitializeNewProcess(Process, pParentProcess);
  if (!NT_SUCCESS(Status))
    {
       DbgPrint("SepInitializeNewProcess failed (Status %x)\n", Status);
       ObDereferenceObject(Process);
       ObDereferenceObject(pParentProcess);
       return(Status);
    }

   /*
    * 
    */
   DPRINT("Creating PEB\n");
   Status = PsCreatePeb(*ProcessHandle,
			Process,
			ImageBase);
   if (!NT_SUCCESS(Status))
     {
        DbgPrint("NtCreateProcess() Peb creation failed: Status %x\n",Status);
	ObDereferenceObject(Process);
	ObDereferenceObject(pParentProcess);
	ZwClose(*ProcessHandle);
	*ProcessHandle = NULL;
	return(Status);
     }
   
   /*
    * Maybe send a message to the creator process's debugger
    */
#if 0
   if (pParentProcess->DebugPort != NULL)
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
	  sizeof(LPC_MESSAGE);
	Message.Type = DBG_EVENT_CREATE_PROCESS;
	Message.Data.CreateProcess.FileHandle = FileHandle;
	Message.Data.CreateProcess.Base = ImageBase;
	Message.Data.CreateProcess.EntryPoint = NULL; //
	
	Status = LpcSendDebugMessagePort(pParentProcess->DebugPort,
					 &Message);
     }
#endif

   PspRunCreateProcessNotifyRoutines(Process, TRUE);
   
   ObDereferenceObject(Process);
   ObDereferenceObject(pParentProcess);
   return(STATUS_SUCCESS);
}


/*
 * @unimplemented
 */
NTSTATUS STDCALL
NtOpenProcess(OUT PHANDLE	    ProcessHandle,
	      IN  ACCESS_MASK	    DesiredAccess,
	      IN  POBJECT_ATTRIBUTES  ObjectAttributes,
	      IN  PCLIENT_ID	    ClientId)
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
					 ProcessListEntry);
	     if (current->UniqueProcessId == (ULONG)ClientId->UniqueProcess)
	       {
	          if (current->Pcb.State == PROCESS_STATE_TERMINATED)
		    {
		      Status = STATUS_PROCESS_IS_TERMINATING;
		    }
		  else
		    {
		      Status = ObReferenceObjectByPointer(current,
					                  DesiredAccess,
					                  PsProcessType,
					                  UserMode);
		    }
		  KeReleaseSpinLock(&PsProcessListLock, oldIrql);
		  if (NT_SUCCESS(Status))
		    {
		      Status = ObCreateHandle(PsGetCurrentProcess(),
					      current,
					      DesiredAccess,
					      FALSE,
					      ProcessHandle);
		      ObDereferenceObject(current);
		      DPRINT("*ProcessHandle %x\n", ProcessHandle);
		      DPRINT("NtOpenProcess() = %x\n", Status);
		    }
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


/*
 * @unimplemented
 */
NTSTATUS STDCALL
NtQueryInformationProcess(IN  HANDLE ProcessHandle,
			  IN  PROCESSINFOCLASS ProcessInformationClass,
			  OUT PVOID ProcessInformation,
			  IN  ULONG ProcessInformationLength,
			  OUT PULONG ReturnLength  OPTIONAL)
{
   PEPROCESS Process;
   NTSTATUS Status;

   /*
    * TODO: Here we should probably check that ProcessInformationLength
    * bytes indeed are writable at address ProcessInformation.
    */

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
	if (ProcessInformationLength != sizeof(PROCESS_BASIC_INFORMATION))
	{
	  Status = STATUS_INFO_LENGTH_MISMATCH;
	}
	else
	{
	  PPROCESS_BASIC_INFORMATION ProcessBasicInformationP =
	    (PPROCESS_BASIC_INFORMATION)ProcessInformation;
	  ProcessBasicInformationP->ExitStatus = Process->ExitStatus;
	  ProcessBasicInformationP->PebBaseAddress = Process->Peb;
	  ProcessBasicInformationP->AffinityMask = Process->Pcb.Affinity;
	  ProcessBasicInformationP->UniqueProcessId =
	    Process->UniqueProcessId;
	  ProcessBasicInformationP->InheritedFromUniqueProcessId =
	    (ULONG)Process->InheritedFromUniqueProcessId;
	  ProcessBasicInformationP->BasePriority =
	    Process->Pcb.BasePriority;
	  
	  if (ReturnLength)
	  {
	    *ReturnLength = sizeof(PROCESS_BASIC_INFORMATION);
	  }
	}
	break;

      case ProcessQuotaLimits:
      case ProcessIoCounters:
	Status = STATUS_NOT_IMPLEMENTED;
	break;

      case ProcessTimes:
	if (ProcessInformationLength != sizeof(KERNEL_USER_TIMES))
	{
	  Status = STATUS_INFO_LENGTH_MISMATCH;
	}
	else
	{
	   PKERNEL_USER_TIMES ProcessTimeP =
	                     (PKERNEL_USER_TIMES)ProcessInformation;

	   ProcessTimeP->CreateTime = Process->CreateTime;
           ProcessTimeP->UserTime.QuadPart = Process->Pcb.UserTime * 100000LL;
           ProcessTimeP->KernelTime.QuadPart = Process->Pcb.KernelTime * 100000LL;
	   ProcessTimeP->ExitTime = Process->ExitTime;

	  if (ReturnLength)
	  {
	    *ReturnLength = sizeof(KERNEL_USER_TIMES);
	  }
	}
	break;

      case ProcessDebugPort:
      case ProcessLdtInformation:
      case ProcessWorkingSetWatch:
      case ProcessWx86Information:
	Status = STATUS_NOT_IMPLEMENTED;
	break;

      case ProcessHandleCount:
      	if (ProcessInformationLength != sizeof(ULONG))
	{
	  Status = STATUS_INFO_LENGTH_MISMATCH;
	}
	else
	{
	  PULONG HandleCount = (PULONG)ProcessInformation;
	  *HandleCount = ObpGetHandleCountByHandleTable(&Process->HandleTable);
	  if (ReturnLength)
	  {
	    *ReturnLength = sizeof(ULONG);
	  }
	}
	break;

      case ProcessSessionInformation:
      case ProcessWow64Information:
	Status = STATUS_NOT_IMPLEMENTED;
	break;

      case ProcessVmCounters:
	if (ProcessInformationLength != sizeof(VM_COUNTERS))
	{
	  Status = STATUS_INFO_LENGTH_MISMATCH;
	}
	else
	{
	  PVM_COUNTERS pOut = (PVM_COUNTERS)ProcessInformation;
	  pOut->PeakVirtualSize            = Process->PeakVirtualSize;
	  /*
	   * Here we should probably use VirtualSize.LowPart, but due to
	   * incompatibilities in current headers (no unnamed union),
	   * I opted for cast.
	   */
	  pOut->VirtualSize                = (ULONG)Process->VirtualSize.QuadPart;
	  pOut->PageFaultCount             = Process->Vm.PageFaultCount;
	  pOut->PeakWorkingSetSize         = Process->Vm.PeakWorkingSetSize;
	  pOut->WorkingSetSize             = Process->Vm.WorkingSetSize;
	  pOut->QuotaPeakPagedPoolUsage    = Process->QuotaPeakPoolUsage[0]; // TODO: Verify!
	  pOut->QuotaPagedPoolUsage        = Process->QuotaPoolUsage[0];     // TODO: Verify!
	  pOut->QuotaPeakNonPagedPoolUsage = Process->QuotaPeakPoolUsage[1]; // TODO: Verify!
	  pOut->QuotaNonPagedPoolUsage     = Process->QuotaPoolUsage[1];     // TODO: Verify!
	  pOut->PagefileUsage              = Process->PagefileUsage;
	  pOut->PeakPagefileUsage          = Process->PeakPagefileUsage;

	  if (ReturnLength)
	  {
	    *ReturnLength = sizeof(VM_COUNTERS);
	  }
	}
	break;

      case ProcessDefaultHardErrorMode:
	if (ProcessInformationLength != sizeof(ULONG))
	{
	  Status = STATUS_INFO_LENGTH_MISMATCH;
	}
	else
	{
	  PULONG HardErrMode = (PULONG)ProcessInformation;
	  *HardErrMode = Process->DefaultHardErrorProcessing;

	  if (ReturnLength)
	  {
	    *ReturnLength = sizeof(ULONG);
	  }
	}
	break;

      case ProcessPriorityBoost:
	if (ProcessInformationLength != sizeof(ULONG))
	{
	  Status = STATUS_INFO_LENGTH_MISMATCH;
	}
	else
	{
	  PULONG BoostEnabled = (PULONG)ProcessInformation;
	  *BoostEnabled = Process->Pcb.DisableBoost ? FALSE : TRUE;

	  if (ReturnLength)
	  {
	    *ReturnLength = sizeof(ULONG);
	  }
	}
	break;

      case ProcessDeviceMap:
	if (ProcessInformationLength != sizeof(PROCESS_DEVICEMAP_INFORMATION))
        {
	  Status = STATUS_INFO_LENGTH_MISMATCH;
	}
        else
        {
	  ObQueryDeviceMapInformation(Process, (PPROCESS_DEVICEMAP_INFORMATION)ProcessInformation);
	  if (ReturnLength)
          {
	    *ReturnLength = sizeof(PROCESS_DEVICEMAP_INFORMATION);
	  }
	}
	break;

      case ProcessPriorityClass:
	if (ProcessInformationLength != sizeof(USHORT))
	{
	  Status = STATUS_INFO_LENGTH_MISMATCH;
	}
	else
	{
	  PUSHORT Priority = (PUSHORT)ProcessInformation;
	  *Priority = Process->PriorityClass;

	  if (ReturnLength)
	  {
	    *ReturnLength = sizeof(USHORT);
	  }
	}
	break;

      case ProcessImageFileName:
      {
        /*
         * We DO NOT return the file name stored in the EPROCESS structure.
         * Propably if we can't find a PEB or ProcessParameters structure for the
         * process!
         */
        PRTL_USER_PROCESS_PARAMETERS ProcParams;
        ASSERT(Process->Peb);
        ASSERT(Process->Peb->ProcessParameters);
        ProcParams = Process->Peb->ProcessParameters;
        if(ProcessInformationLength < sizeof(UNICODE_STRING) + ProcParams->ImagePathName.Length + sizeof(WCHAR))
        {
          Status = STATUS_INFO_LENGTH_MISMATCH;
        }
        else
        {
          PUNICODE_STRING DstPath = (PUNICODE_STRING)ProcessInformation;
          DstPath->Length = ProcParams->ImagePathName.Length;
          DstPath->MaximumLength = DstPath->Length + sizeof(WCHAR);
          DstPath->Buffer = (PWSTR)(DstPath + 1);
          
          RtlCopyMemory(DstPath->Buffer, ProcParams->ImagePathName.Buffer, ProcParams->ImagePathName.Length);
          DstPath->Buffer[DstPath->Length / sizeof(WCHAR)] = L'\0';
        }
        break;
      }

      /*
       * Note: The following 10 information classes are verified to not be
       * implemented on NT, and do indeed return STATUS_INVALID_INFO_CLASS;
       */
      case ProcessBasePriority:
      case ProcessRaisePriority:
      case ProcessExceptionPort:
      case ProcessAccessToken:
      case ProcessLdtSize:
      case ProcessIoPortHandlers:
      case ProcessUserModeIOPL:
      case ProcessEnableAlignmentFaultFixup:
      case ProcessAffinityMask:
      case ProcessForegroundInformation:
      default:
	Status = STATUS_INVALID_INFO_CLASS;
     }
   ObDereferenceObject(Process);
   return(Status);
}


NTSTATUS
PspAssignPrimaryToken(PEPROCESS Process,
		      HANDLE TokenHandle)
{
   PACCESS_TOKEN Token;
   PACCESS_TOKEN OldToken;
   NTSTATUS Status;
   
   Status = ObReferenceObjectByHandle(TokenHandle,
				      0,
				      SepTokenObjectType,
				      UserMode,
				      (PVOID*)&Token,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   Status = SeExchangePrimaryToken(Process, Token, &OldToken);
   if (NT_SUCCESS(Status))
     {
	ObDereferenceObject(OldToken);
     }
   ObDereferenceObject(Token);
   return(Status);
}

/*
 * @unimplemented
 */
NTSTATUS STDCALL
NtSetInformationProcess(IN HANDLE ProcessHandle,
			IN PROCESSINFOCLASS ProcessInformationClass,
			IN PVOID ProcessInformation,
			IN ULONG ProcessInformationLength)
{
   PEPROCESS Process;
   NTSTATUS Status;
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
      case ProcessQuotaLimits:
      case ProcessBasePriority:
      case ProcessRaisePriority:
      case ProcessDebugPort:
      case ProcessExceptionPort:
	Status = STATUS_NOT_IMPLEMENTED;
	break;

      case ProcessAccessToken:
	ProcessAccessTokenP = (PHANDLE)ProcessInformation;
	Status = PspAssignPrimaryToken(Process, *ProcessAccessTokenP);
	break;
	
      case ProcessLdtInformation:
      case ProcessLdtSize:
      case ProcessDefaultHardErrorMode:
      case ProcessIoPortHandlers:
      case ProcessWorkingSetWatch:
      case ProcessUserModeIOPL:
      case ProcessEnableAlignmentFaultFixup:
      case ProcessPriorityClass:
      case ProcessAffinityMask:
	Status = STATUS_NOT_IMPLEMENTED;
	break;

      case ProcessBasicInformation:
      case ProcessIoCounters:
      case ProcessTimes:
      case ProcessPooledUsageAndLimits:
      case ProcessWx86Information:
      case ProcessHandleCount:
      case ProcessWow64Information:
      default:
	Status = STATUS_INVALID_INFO_CLASS;
     }
   ObDereferenceObject(Process);
   return(Status);
}


/**********************************************************************
 * NAME							INTERNAL
 * 	PiQuerySystemProcessInformation
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
PiQuerySystemProcessInformation(PVOID Buffer,
				ULONG Size,
				PULONG ReqSize)
{
   return STATUS_NOT_IMPLEMENTED;

#if 0
	KIRQL		OldIrql;
	PLIST_ENTRY	CurrentEntryP;
	PEPROCESS	CurrentP;
	PLIST_ENTRY	CurrentEntryT;
	PETHREAD	CurrentT;
	
	ULONG		RequiredSize = 0L;
	BOOLEAN		SizeOnly = FALSE;

	ULONG		SpiSize = 0L;
	
	PSYSTEM_PROCESS_INFORMATION	pInfoP = (PSYSTEM_PROCESS_INFORMATION) SnapshotBuffer;
	PSYSTEM_PROCESS_INFORMATION	pInfoPLast = NULL;
	PSYSTEM_THREAD_INFO		pInfoT = NULL;
	

   /* Lock the process list. */
   KeAcquireSpinLock(&PsProcessListLock,
		     &OldIrql);

	/*
	 * Scan the process list. Since the
	 * list is circular, the guard is false
	 * after the last process.
	 */
	for (	CurrentEntryP = PsProcessListHead.Flink;
		(CurrentEntryP != & PsProcessListHead);
		CurrentEntryP = CurrentEntryP->Flink
		)
	{
		/*
		 * Compute how much space is
		 * occupied in the snapshot
		 * by adding this process info.
		 * (at least one thread).
		 */
		SpiSizeCurrent = sizeof (SYSTEM_PROCESS_INFORMATION);
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
		 * Get a reference to the 
		 * process descriptor we are
		 * handling.
		 */
		CurrentP = CONTAINING_RECORD(
				CurrentEntryP,
				EPROCESS, 
				ProcessListEntry
				);
		/*
		 * Write process data in the buffer.
		 */
		RtlZeroMemory (pInfoP, sizeof (SYSTEM_PROCESS_INFORMATION));
		/* PROCESS */
		pInfoP->ThreadCount = 0L;
		pInfoP->ProcessId = CurrentP->UniqueProcessId;
		RtlInitUnicodeString (
			& pInfoP->Name,
			CurrentP->ImageFileName
			);
		/* THREAD */
		for (	pInfoT = & CurrentP->ThreadSysInfo [0],
			CurrentEntryT = CurrentP->ThreadListHead.Flink;
			
			(CurrentEntryT != & CurrentP->ThreadListHead);
			
			pInfoT = & CurrentP->ThreadSysInfo [pInfoP->ThreadCount],
			CurrentEntryT = CurrentEntryT->Flink
			)
		{
			/*
			 * Recalculate the size of the
			 * information block.
			 */
			if (0 < pInfoP->ThreadCount)
			{
				RequiredSize += sizeof (SYSTEM_THREAD_INFORMATION);
			}
			/*
			 * Do not write thread data in the
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
			 * Get a reference to the 
			 * thread descriptor we are
			 * handling.
			 */
			CurrentT = CONTAINING_RECORD(
					CurrentEntryT,
					KTHREAD, 
					ThreadListEntry
					);
			/*
			 * Write thread data.
			 */
			RtlZeroMemory (
				pInfoT,
				sizeof (SYSTEM_THREAD_INFORMATION)
				);
			pInfoT->KernelTime	= CurrentT-> ;	/* TIME */
			pInfoT->UserTime	= CurrentT-> ;	/* TIME */
			pInfoT->CreateTime	= CurrentT-> ;	/* TIME */
			pInfoT->TickCount	= CurrentT-> ;	/* ULONG */
			pInfoT->StartEIP	= CurrentT-> ;	/* ULONG */
			pInfoT->ClientId	= CurrentT-> ;	/* CLIENT_ID */
			pInfoT->ClientId	= CurrentT-> ;	/* CLIENT_ID */
			pInfoT->DynamicPriority	= CurrentT-> ;	/* ULONG */
			pInfoT->BasePriority	= CurrentT-> ;	/* ULONG */
			pInfoT->nSwitches	= CurrentT-> ;	/* ULONG */
			pInfoT->State		= CurrentT-> ;	/* DWORD */
			pInfoT->WaitReason	= CurrentT-> ;	/* KWAIT_REASON */
			/*
			 * Count the number of threads 
			 * this process has.
			 */
			++ pInfoP->ThreadCount;
		}
		/*
		 * Save the size of information
		 * stored in the buffer for the
		 * current process.
		 */
		pInfoP->RelativeOffset = SpiSize;
		/*
		 * Save a reference to the last
		 * valid information block.
		 */
		pInfoPLast = pInfoP;
		/*
		 * Compute the offset of the 
		 * SYSTEM_PROCESS_INFORMATION
		 * descriptor in the snapshot 
		 * buffer for the next process.
		 */
		(ULONG) pInfoP += SpiSize;
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
		if (NULL != RequiredSize)
		{
			*pRequiredSize = RequiredSize;
		}
		return STATUS_INFO_LENGTH_MISMATCH;
	}
	/*
	 * Mark the end of the snapshot.
	 */
	pInfoP->RelativeOffset = 0L;
	/* OK */	
	return STATUS_SUCCESS;
#endif
}

/*
 * @implemented
 */
LARGE_INTEGER STDCALL
PsGetProcessExitTime(VOID)
{
  LARGE_INTEGER Li;
  Li.QuadPart = PsGetCurrentProcess()->ExitTime.QuadPart;
  return Li;
}

/*
 * @implemented
 */
LONGLONG
STDCALL
PsGetProcessCreateTimeQuadPart(
    PEPROCESS	Process
	)
{
	return Process->CreateTime.QuadPart;
}

/*
 * @implemented
 */
PVOID
STDCALL
PsGetProcessDebugPort(
    PEPROCESS	Process
	)
{
	return Process->DebugPort;
}

/*
 * @implemented
 */
BOOLEAN
STDCALL
PsGetProcessExitProcessCalled(
    PEPROCESS	Process
	)
{
	return Process->ExitProcessCalled;	
}

/*
 * @implemented
 */
NTSTATUS
STDCALL
PsGetProcessExitStatus(
	PEPROCESS Process
	)
{
	return Process->ExitStatus;
}

/*
 * @implemented
 */
HANDLE
STDCALL
PsGetProcessId(
   	PEPROCESS	Process
	)
{
	return (HANDLE)Process->UniqueProcessId;
}

/*
 * @implemented
 */
LPSTR
STDCALL
PsGetProcessImageFileName(
    PEPROCESS	Process
	)
{
	return (LPSTR)Process->ImageFileName;
}

/*
 * @implemented
 */
HANDLE
STDCALL
PsGetProcessInheritedFromUniqueProcessId(
    PEPROCESS	Process
	)
{
	return Process->InheritedFromUniqueProcessId;
}

/*
 * @implemented
 */
PEJOB
STDCALL
PsGetProcessJob(
	PEPROCESS Process
	)
{
	return Process->Job;
}

/*
 * @implemented
 */
PPEB
STDCALL
PsGetProcessPeb(
    PEPROCESS	Process
	)
{
	return Process->Peb;	
}

/*
 * @implemented
 */
ULONG
STDCALL
PsGetProcessPriorityClass(
    PEPROCESS	Process
	)
{
	return Process->PriorityClass;
}

/*
 * @implemented
 */
PVOID
STDCALL
PsGetProcessSectionBaseAddress(
    PEPROCESS	Process
	)
{
	return Process->SectionBaseAddress;
}

/*
 * @implemented
 */
PVOID
STDCALL
PsGetProcessSecurityPort(
	PEPROCESS Process
	)
{
	return Process->SecurityPort;
}

/*
 * @implemented
 */
HANDLE
STDCALL
PsGetProcessSessionId(
    PEPROCESS	Process
	)
{
	return (HANDLE)Process->SessionId;
}

/*
 * @implemented
 */
PVOID
STDCALL
PsGetProcessWin32Process(
	PEPROCESS Process
	)
{
	return Process->Win32Process;
}

/*
 * @implemented
 */
PVOID
STDCALL
PsGetProcessWin32WindowStation(
    PEPROCESS	Process
	)
{
	return Process->Win32WindowStation;
}

/*
 * @implemented
 */
BOOLEAN
STDCALL
PsIsProcessBeingDebugged(
    PEPROCESS	Process
	)
{
	return FALSE/*Process->IsProcessBeingDebugged*/;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
PsLookupProcessByProcessId(IN PVOID ProcessId,
			   OUT PEPROCESS *Process)
{
  KIRQL oldIrql;
  PLIST_ENTRY current_entry;
  PEPROCESS current;

  KeAcquireSpinLock(&PsProcessListLock, &oldIrql);

  current_entry = PsProcessListHead.Flink;
  while (current_entry != &PsProcessListHead)
    {
      current = CONTAINING_RECORD(current_entry,
				  EPROCESS,
				  ProcessListEntry);
      if (current->UniqueProcessId == (ULONG)ProcessId)
	{
	  *Process = current;
          ObReferenceObject(current);
	  KeReleaseSpinLock(&PsProcessListLock, oldIrql);
	  return(STATUS_SUCCESS);
	}
      current_entry = current_entry->Flink;
    }

  KeReleaseSpinLock(&PsProcessListLock, oldIrql);

  return(STATUS_INVALID_PARAMETER);
}

VOID
STDCALL
PspRunCreateProcessNotifyRoutines
(
 PEPROCESS CurrentProcess,
 BOOLEAN Create
)
{
 ULONG i;
 HANDLE ProcessId = (HANDLE)CurrentProcess->UniqueProcessId;
 HANDLE ParentId = CurrentProcess->InheritedFromUniqueProcessId;
 
 for(i = 0; i < MAX_PROCESS_NOTIFY_ROUTINE_COUNT; ++ i)
  if(PiProcessNotifyRoutine[i])
   PiProcessNotifyRoutine[i](ParentId, ProcessId, Create);
}

/*
 * @implemented
 */
NTSTATUS STDCALL
PsSetCreateProcessNotifyRoutine(IN PCREATE_PROCESS_NOTIFY_ROUTINE NotifyRoutine,
				IN BOOLEAN Remove)
{
  ULONG i;

  if (Remove)
  {
     for(i=0;i<MAX_PROCESS_NOTIFY_ROUTINE_COUNT;i++)
     {
        if ((PVOID)PiProcessNotifyRoutine[i] == (PVOID)NotifyRoutine)
        {
           PiProcessNotifyRoutine[i] = NULL;
           break;
        }
     }

     return(STATUS_SUCCESS);
  }

  /*insert*/
  for(i=0;i<MAX_PROCESS_NOTIFY_ROUTINE_COUNT;i++)
  {
     if (PiProcessNotifyRoutine[i] == NULL)
     {
        PiProcessNotifyRoutine[i] = NotifyRoutine;
        break;
     }
  }

  if (i == MAX_PROCESS_NOTIFY_ROUTINE_COUNT)
  {
     return STATUS_INSUFFICIENT_RESOURCES;
  }

  return STATUS_SUCCESS;
}

VOID STDCALL
PspRunLoadImageNotifyRoutines(
   PUNICODE_STRING FullImageName,
   HANDLE ProcessId,
   PIMAGE_INFO ImageInfo)
{
   ULONG i;
 
   for (i = 0; i < MAX_PROCESS_NOTIFY_ROUTINE_COUNT; ++ i)
      if (PiLoadImageNotifyRoutine[i])
         PiLoadImageNotifyRoutine[i](FullImageName, ProcessId, ImageInfo);
}

/*
 * @unimplemented
 */                       
NTSTATUS
STDCALL
PsRemoveLoadImageNotifyRoutine(
    IN PLOAD_IMAGE_NOTIFY_ROUTINE NotifyRoutine
    )
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;	
}

/*
 * @implemented
 */
NTSTATUS STDCALL
PsSetLoadImageNotifyRoutine(IN PLOAD_IMAGE_NOTIFY_ROUTINE NotifyRoutine)
{
   ULONG i;

   for (i = 0; i < MAX_LOAD_IMAGE_NOTIFY_ROUTINE_COUNT; i++)
   {
      if (PiLoadImageNotifyRoutine[i] == NULL)
      {
         PiLoadImageNotifyRoutine[i] = NotifyRoutine;
         break;
      }
   }

   if (i == MAX_PROCESS_NOTIFY_ROUTINE_COUNT)
   {
      return STATUS_INSUFFICIENT_RESOURCES;
   }

   return STATUS_SUCCESS;
}

/*
 * @implemented
 */                       
VOID
STDCALL
PsSetProcessPriorityClass(
    PEPROCESS	Process,
    ULONG	PriorityClass	
	)
{
	Process->PriorityClass = PriorityClass;
}

/*
 * @implemented
 */                       
VOID
STDCALL
PsSetProcessSecurityPort(
    PEPROCESS	Process,
    PVOID	SecurityPort	
	)
{
	Process->SecurityPort = SecurityPort;
}

/*
 * @implemented
 */
VOID
STDCALL
PsSetProcessWin32Process(
    PEPROCESS	Process,
    PVOID	Win32Process
	)
{
	Process->Win32Process = Win32Process;
}

/*
 * @implemented
 */
VOID
STDCALL
PsSetProcessWin32WindowStation(
    PEPROCESS	Process,
    PVOID	WindowStation
	)
{
	Process->Win32WindowStation = WindowStation;
}

/* Pool Quotas */
/*
 * @implemented
 */
VOID
STDCALL
PsChargePoolQuota(
    IN PEPROCESS Process,
    IN POOL_TYPE PoolType,
    IN ULONG_PTR Amount
    )
{
    NTSTATUS Status;

    /* Charge the usage */
    Status = PsChargeProcessPoolQuota(Process, PoolType, Amount);

    /* Raise Exception */
    if (!NT_SUCCESS(Status)) {
        ExRaiseStatus(Status);
    }
}

/*
 * @implemented
 */
NTSTATUS
STDCALL
PsChargeProcessNonPagedPoolQuota (
    IN PEPROCESS Process,
    IN ULONG_PTR Amount
    )
{
    /* Call the general function */
    return PsChargeProcessPoolQuota(Process, NonPagedPool, Amount);
}

/*
 * @implemented
 */
NTSTATUS
STDCALL
PsChargeProcessPagedPoolQuota (
    IN PEPROCESS Process,
    IN ULONG_PTR Amount
    )
{
    /* Call the general function */
    return PsChargeProcessPoolQuota(Process, PagedPool, Amount);
}

/*
 * @implemented
 */
NTSTATUS
STDCALL
PsChargeProcessPoolQuota(
    IN PEPROCESS Process,
    IN POOL_TYPE PoolType,
    IN ULONG_PTR Amount
    )
{
    PEPROCESS_QUOTA_BLOCK QuotaBlock;
    ULONG NewUsageSize;
    ULONG NewMaxQuota;

    /* Get current Quota Block */
    QuotaBlock = Process->QuotaBlock;

    /* Quota Operations are not to be done on the SYSTEM Process */
    if (Process == PsInitialSystemProcess) return STATUS_SUCCESS;

    /* New Size in use */
    NewUsageSize = QuotaBlock->QuotaEntry[PoolType].Usage + Amount;

    /* Does this size respect the quota? */
    if (NewUsageSize > QuotaBlock->QuotaEntry[PoolType].Limit) {

        /* It doesn't, so keep raising the Quota */
        while (MiRaisePoolQuota(PoolType, QuotaBlock->QuotaEntry[PoolType].Limit, &NewMaxQuota)) {
            /* Save new Maximum Quota */
            QuotaBlock->QuotaEntry[PoolType].Limit = NewMaxQuota;

            /* See if the new Maximum Quota fulfills our need */
            if (NewUsageSize <= NewMaxQuota) goto QuotaChanged;
        }

        return STATUS_QUOTA_EXCEEDED;
    }

QuotaChanged:
    /* Save new Usage */
    QuotaBlock->QuotaEntry[PoolType].Usage = NewUsageSize;

    /* Is this a new peak? */
    if (NewUsageSize > QuotaBlock->QuotaEntry[PoolType].Peak) {
        QuotaBlock->QuotaEntry[PoolType].Peak = NewUsageSize;
    }

    /* All went well */
    return STATUS_SUCCESS;
}

/*
 * @unimplemented
 */                       
VOID
STDCALL
PsReturnPoolQuota(
    IN PEPROCESS Process,
    IN POOL_TYPE PoolType,
    IN ULONG_PTR Amount
    )
{
	UNIMPLEMENTED;
} 

/*
 * @unimplemented
 */                       
VOID
STDCALL
PsReturnProcessNonPagedPoolQuota(
    IN PEPROCESS Process,
    IN ULONG_PTR Amount
    )
{
	UNIMPLEMENTED;
} 

/*
 * @unimplemented
 */                       
VOID
STDCALL
PsReturnProcessPagedPoolQuota(
    IN PEPROCESS Process,
    IN ULONG_PTR Amount
    )
{
	UNIMPLEMENTED;
}

NTSTATUS
PsLockProcess(PEPROCESS Process, BOOL Timeout)
{
  ULONG Attempts = 0;
  PKTHREAD PrevLockOwner;
  NTSTATUS Status = STATUS_UNSUCCESSFUL;
  PLARGE_INTEGER Delay = (Timeout ? &PsLockTimeout : NULL);
  PKTHREAD CallingThread = KeGetCurrentThread();
  
  KeEnterCriticalRegion();
  
  for(;;)
  {
    if(Process->Pcb.State == PROCESS_STATE_TERMINATED)
    {
      KeLeaveCriticalRegion();
      return STATUS_PROCESS_IS_TERMINATING;
    }

    PrevLockOwner = (PKTHREAD)InterlockedCompareExchangePointer(
      &Process->LockOwner, CallingThread, NULL);
    if(PrevLockOwner == NULL || PrevLockOwner == CallingThread)
    {
      /* we got the lock or already locked it */
      if(InterlockedIncrementUL(&Process->LockCount) == 1)
      {
        KeClearEvent(&Process->LockEvent);
      }

      return STATUS_SUCCESS;
    }
    else
    {
      if(++Attempts > 2)
      {
        Status = KeWaitForSingleObject(&Process->LockEvent,
                                       Executive,
                                       KernelMode,
                                       FALSE,
                                       Delay);
        if(Status == STATUS_TIMEOUT)
        {
          KeLeaveCriticalRegion();
          break;
        }
      }
      else
      {
        KeDelayExecutionThread(KernelMode, FALSE, &ShortPsLockDelay);
      }
    }
  }
  
  return Status;
}

VOID
PsUnlockProcess(PEPROCESS Process)
{
  ASSERT(Process->LockOwner == KeGetCurrentThread());
  
  if(InterlockedDecrementUL(&Process->LockCount) == 0)
  {
    InterlockedExchangePointer(&Process->LockOwner, NULL);
    KeSetEvent(&Process->LockEvent, IO_NO_INCREMENT, FALSE);
  }
  
  KeLeaveCriticalRegion();
}

/* EOF */
