/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/iocomp.c
 * PURPOSE:         
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
		    Changed NtQueryIoCompletion 
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

#define IOC_TAG   TAG('I', 'O', 'C', 'T')   

POBJECT_TYPE ExIoCompletionType;

NPAGED_LOOKASIDE_LIST  IoCompletionPacketLookaside;

static GENERIC_MAPPING ExIoCompletionMapping = 
{
   STANDARD_RIGHTS_READ | IO_COMPLETION_QUERY_STATE,
   STANDARD_RIGHTS_WRITE | IO_COMPLETION_MODIFY_STATE,
   STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE | IO_COMPLETION_QUERY_STATE,
   IO_COMPLETION_ALL_ACCESS
};

/* FUNCTIONS *****************************************************************/

NTSTATUS 
STDCALL
NtpCreateIoCompletion(
   PVOID                ObjectBody,
   PVOID                Parent,
   PWSTR                RemainingPath,
   POBJECT_ATTRIBUTES   ObjectAttributes
   )
{
   DPRINT("NtpCreateIoCompletion(ObjectBody %x, Parent %x, RemainingPath %S)\n",
      ObjectBody, Parent, RemainingPath);

   if (RemainingPath != NULL && wcschr(RemainingPath+1, '\\') != NULL)
   {
      return STATUS_UNSUCCESSFUL;
   }

   return STATUS_SUCCESS;
}

VOID STDCALL
NtpDeleteIoCompletion(PVOID ObjectBody)
{
   PKQUEUE Queue = ObjectBody;

   DPRINT("NtpDeleteIoCompletion()\n");

   KeRundownQueue(Queue);
}


/*
 * @unimplemented
 */
NTSTATUS
STDCALL
IoSetCompletionRoutineEx(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PIO_COMPLETION_ROUTINE CompletionRoutine,
    IN PVOID Context,
    IN BOOLEAN InvokeOnSuccess,
    IN BOOLEAN InvokeOnError,
    IN BOOLEAN InvokeOnCancel
    )
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
NTSTATUS
STDCALL
IoSetIoCompletion (
	IN PVOID IoCompletion,
	IN PVOID KeyContext,
	IN PVOID ApcContext,
	IN NTSTATUS IoStatus,
	IN ULONG_PTR IoStatusInformation,
	IN BOOLEAN Quota
	)
{
   PKQUEUE Queue = (PKQUEUE) IoCompletion;
   PIO_COMPLETION_PACKET   Packet;

   Packet = ExAllocateFromNPagedLookasideList(&IoCompletionPacketLookaside);
   if (NULL == Packet)
   {
     return STATUS_NO_MEMORY;
   }

   Packet->Key = KeyContext;
   Packet->Context = ApcContext;
   Packet->IoStatus.Status = IoStatus;
   Packet->IoStatus.Information = IoStatusInformation;
   
   KeInsertQueue(Queue, &Packet->ListEntry);

   return STATUS_SUCCESS;
}

VOID
FASTCALL
IopInitIoCompletionImplementation(VOID)
{
   ExIoCompletionType = ExAllocatePool(NonPagedPool, sizeof(OBJECT_TYPE));
   
   RtlCreateUnicodeString(&ExIoCompletionType->TypeName, L"IoCompletion");
   
   ExIoCompletionType->Tag = IOC_TAG;
   ExIoCompletionType->MaxObjects = ULONG_MAX;
   ExIoCompletionType->MaxHandles = ULONG_MAX;
   ExIoCompletionType->TotalObjects = 0;
   ExIoCompletionType->TotalHandles = 0;
   ExIoCompletionType->PagedPoolCharge = 0;
   ExIoCompletionType->NonpagedPoolCharge = sizeof(KQUEUE);
   ExIoCompletionType->Mapping = &ExIoCompletionMapping;
   ExIoCompletionType->Dump = NULL;
   ExIoCompletionType->Open = NULL;
   ExIoCompletionType->Close = NULL;
   ExIoCompletionType->Delete = NtpDeleteIoCompletion;
   ExIoCompletionType->Parse = NULL;
   ExIoCompletionType->Security = NULL;
   ExIoCompletionType->QueryName = NULL;
   ExIoCompletionType->OkayToClose = NULL;
   ExIoCompletionType->Create = NtpCreateIoCompletion;
   ExIoCompletionType->DuplicationNotify = NULL;

   ExInitializeNPagedLookasideList(&IoCompletionPacketLookaside,
                                   NULL,
                                   NULL,
                                   0,
                                   sizeof(IO_COMPLETION_PACKET),
                                   IOC_TAG,
                                   0);
}


NTSTATUS
STDCALL
NtCreateIoCompletion(
   OUT PHANDLE             IoCompletionHandle,
   IN  ACCESS_MASK         DesiredAccess,
   IN  POBJECT_ATTRIBUTES  ObjectAttributes,
   IN  ULONG               NumberOfConcurrentThreads
   )
{
   PKQUEUE     Queue;
   NTSTATUS    Status;

   Status = ObCreateObject(ExGetPreviousMode(),
                           ExIoCompletionType,
                           ObjectAttributes,
                           ExGetPreviousMode(),
                           NULL,
                           sizeof(KQUEUE),
                           0,
                           0,
                           (PVOID*)&Queue);
   if (!NT_SUCCESS(Status))
   {
     return Status;
   }

   Status = ObInsertObject ((PVOID)Queue,
			    NULL,
			    DesiredAccess,
			    0,
			    NULL,
			    IoCompletionHandle);
   if (!NT_SUCCESS(Status))
   {
     ObDereferenceObject(Queue);
     return Status;
   }

   KeInitializeQueue(Queue, NumberOfConcurrentThreads);
   ObDereferenceObject(Queue);

   return STATUS_SUCCESS;
   /*

  CompletionPort = NULL OR ExistingCompletionPort

  */
 

}

/*
DesiredAccess:
ZERO
IO_COMPLETION_QUERY_STATE Query access
IO_COMPLETION_MODIFY_STATE Modify access
IO_COMPLETION_ALL_ACCESS All of the preceding + STANDARD_RIGHTS_ALL

ObjectAttributes
OBJ_OPENLINK and OBJ_PERMANENT are not valid attributes

Return Value
STATUS_SUCCESS or an error status, such as STATUS_ACCESS_DENIED or
STATUS_OBJECT_NAME_NOT_FOUND.
*/
NTSTATUS
STDCALL
NtOpenIoCompletion(
   OUT PHANDLE             IoCompletionHandle,
   IN  ACCESS_MASK         DesiredAccess,
   IN  POBJECT_ATTRIBUTES  ObjectAttributes
   )
{
   NTSTATUS Status;
   
   Status = ObOpenObjectByName(ObjectAttributes,
                               ExIoCompletionType,
                               NULL,
                               UserMode,
                               DesiredAccess,
                               NULL,
                               IoCompletionHandle);  //<- ???
   
   return Status;
}


NTSTATUS
STDCALL
NtQueryIoCompletion(
   IN  HANDLE                          IoCompletionHandle,
   IN  IO_COMPLETION_INFORMATION_CLASS IoCompletionInformationClass,
   OUT PVOID                           IoCompletionInformation,
   IN  ULONG                           IoCompletionInformationLength,
   OUT PULONG                          ResultLength OPTIONAL
   )
{
   PKQUEUE  Queue;
   NTSTATUS Status;

   if (IoCompletionInformationClass != IoCompletionBasicInformation)
   {
      return STATUS_INVALID_INFO_CLASS;
   }
   if (IoCompletionInformationLength < sizeof(IO_COMPLETION_BASIC_INFORMATION))
   {
      return STATUS_INFO_LENGTH_MISMATCH;
   }

   Status = ObReferenceObjectByHandle( IoCompletionHandle,
                                       IO_COMPLETION_QUERY_STATE,
                                       ExIoCompletionType,
                                       UserMode,
                                       (PVOID*)&Queue,
                                       NULL);
   if (NT_SUCCESS(Status))
   {
      ((PIO_COMPLETION_BASIC_INFORMATION)IoCompletionInformation)->Depth = 
         Queue->Header.SignalState;

      ObDereferenceObject(Queue);

      if (ResultLength) *ResultLength = sizeof(IO_COMPLETION_BASIC_INFORMATION);
   }

   return Status;
}


/*
 * Dequeues an I/O completion message from an I/O completion object
 */
NTSTATUS
STDCALL
NtRemoveIoCompletion(
   IN  HANDLE           IoCompletionHandle,
   OUT PVOID            *CompletionKey,
   OUT PVOID            *CompletionContext,
   OUT PIO_STATUS_BLOCK IoStatusBlock,
   IN  PLARGE_INTEGER   Timeout OPTIONAL
   )
{
   PKQUEUE  Queue;
   NTSTATUS Status;
   PIO_COMPLETION_PACKET   Packet;
   PLIST_ENTRY             ListEntry;

   Status = ObReferenceObjectByHandle( IoCompletionHandle,
                                       IO_COMPLETION_MODIFY_STATE,
                                       ExIoCompletionType,
                                       UserMode,
                                       (PVOID*)&Queue,
                                       NULL);
   if (!NT_SUCCESS(Status))
   {
      return Status;
   }

   /*
   Try 2 remove packet from queue. Wait (optionaly) if
   no packet in queue or max num of threads allready running.
   */
      
   do {
      
      ListEntry = KeRemoveQueue(Queue, UserMode, Timeout );

      /* Nebbets book says nothing about NtRemoveIoCompletion returning STATUS_USER_APC,
      and the umode equivalent GetQueuedCompletionStatus says nothing about this either,
      so my guess it we should restart the operation. Need further investigation. -Gunnar
      */

   } while((NTSTATUS)ListEntry == STATUS_USER_APC);

   ObDereferenceObject(Queue);
   
   if ((NTSTATUS)ListEntry == STATUS_TIMEOUT)
   {
      return STATUS_TIMEOUT;
   }
   
   ASSERT(ListEntry);
   
   Packet = CONTAINING_RECORD(ListEntry, IO_COMPLETION_PACKET, ListEntry);

   if (CompletionKey) *CompletionKey = Packet->Key;
   if (CompletionContext) *CompletionContext = Packet->Context;
   if (IoStatusBlock) *IoStatusBlock = Packet->IoStatus;

   ExFreeToNPagedLookasideList(&IoCompletionPacketLookaside, Packet);

   return STATUS_SUCCESS;
}


/*
ASSOSIERT MED FOB's IoCompletionContext

typedef struct _IO_COMPLETION_CONTEXT {
        PVOID Port; 
        ULONG Key; 
} IO_COMPLETION_CONTEXT, *PIO_COMPLETION_CONTEXT;

*/


/*
 * Queues an I/O completion message to an I/O completion object
 */
NTSTATUS
STDCALL
NtSetIoCompletion(
   IN HANDLE   IoCompletionPortHandle,
   IN PVOID    CompletionKey,
   IN PVOID    CompletionContext,
   IN NTSTATUS CompletionStatus,
   IN ULONG    CompletionInformation
   )
{
   NTSTATUS                Status;
   PKQUEUE                 Queue;

   Status = ObReferenceObjectByHandle( IoCompletionPortHandle,
                                       IO_COMPLETION_MODIFY_STATE,
                                       ExIoCompletionType,
                                       UserMode,
                                       (PVOID*)&Queue,
                                       NULL);
   if (NT_SUCCESS(Status))
   {
      Status = IoSetIoCompletion(Queue, CompletionKey, CompletionContext,
                                 CompletionStatus, CompletionInformation, TRUE);
      ObDereferenceObject(Queue);
   }

   return Status;
}
