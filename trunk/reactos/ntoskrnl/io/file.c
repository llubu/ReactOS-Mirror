/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/file.c
 * PURPOSE:         Graceful system shutdown if a bug is detected
 * 
 * PROGRAMMERS:     David Welch (welch@mcmail.com)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>


/* GLOBALS *******************************************************************/

#define TAG_SYSB   TAG('S', 'Y', 'S', 'B')


/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
NTSTATUS STDCALL
NtQueryInformationFile(HANDLE FileHandle,
		       PIO_STATUS_BLOCK IoStatusBlock,
		       PVOID FileInformation,
		       ULONG Length,
		       FILE_INFORMATION_CLASS FileInformationClass)
{
   PFILE_OBJECT FileObject;
   NTSTATUS Status;
   PIRP Irp;
   PDEVICE_OBJECT DeviceObject;
   PIO_STACK_LOCATION StackPtr;
   PVOID SystemBuffer;
   KPROCESSOR_MODE PreviousMode;
   
   ASSERT(IoStatusBlock != NULL);
   ASSERT(FileInformation != NULL);
   
   DPRINT("NtQueryInformationFile(Handle %x StatBlk %x FileInfo %x Length %d "
	  "Class %d)\n", FileHandle, IoStatusBlock, FileInformation,
	  Length, FileInformationClass);

   PreviousMode = ExGetPreviousMode();

   Status = ObReferenceObjectByHandle(FileHandle,
				      0, /* FIXME - access depends on the information class! */
				      IoFileObjectType,
				      PreviousMode,
				      (PVOID *)&FileObject,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   DPRINT("FileObject %x\n", FileObject);
   
   DeviceObject = FileObject->DeviceObject;
   
   Irp = IoAllocateIrp(DeviceObject->StackSize,
		       TRUE);
   if (Irp == NULL)
     {
	ObDereferenceObject(FileObject);
	return STATUS_INSUFFICIENT_RESOURCES;
     }
   
   SystemBuffer = ExAllocatePoolWithTag(NonPagedPool,
					Length,
					TAG_SYSB);
   if (SystemBuffer == NULL)
     {
	IoFreeIrp(Irp);
	ObDereferenceObject(FileObject);
	return(STATUS_INSUFFICIENT_RESOURCES);
     }
   
   /* Trigger FileObject/Event dereferencing */
   Irp->Tail.Overlay.OriginalFileObject = FileObject;
   Irp->RequestorMode = PreviousMode;
   Irp->AssociatedIrp.SystemBuffer = SystemBuffer;
   Irp->UserIosb = IoStatusBlock;
   Irp->UserEvent = &FileObject->Event;
   Irp->Tail.Overlay.Thread = PsGetCurrentThread();
   KeResetEvent( &FileObject->Event );
   
   StackPtr = IoGetNextIrpStackLocation(Irp);
   StackPtr->MajorFunction = IRP_MJ_QUERY_INFORMATION;
   StackPtr->MinorFunction = 0;
   StackPtr->Flags = 0;
   StackPtr->Control = 0;
   StackPtr->DeviceObject = DeviceObject;
   StackPtr->FileObject = FileObject;
   
   StackPtr->Parameters.QueryFile.FileInformationClass =
     FileInformationClass;
   StackPtr->Parameters.QueryFile.Length = Length;
   
   Status = IoCallDriver(FileObject->DeviceObject,
			 Irp);
   if (Status == STATUS_PENDING && (FileObject->Flags & FO_SYNCHRONOUS_IO))
     {
	KeWaitForSingleObject(&FileObject->Event,
			      Executive,
			      PreviousMode,
			      FileObject->Flags & FO_ALERTABLE_IO,
			      NULL);
	Status = IoStatusBlock->Status;
     }

  if (NT_SUCCESS(Status))
    {
      DPRINT("Information %lu\n", IoStatusBlock->Information);
      MmSafeCopyToUser(FileInformation,
		       SystemBuffer,
		       IoStatusBlock->Information);
    }

   ExFreePool(SystemBuffer);

   return Status;
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtQueryQuotaInformationFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID Buffer,
    IN ULONG Length,
    IN BOOLEAN ReturnSingleEntry,
    IN PVOID SidList OPTIONAL,
    IN ULONG SidListLength,
    IN PSID StartSid OPTIONAL,
    IN BOOLEAN RestartScan
    )
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS 
STDCALL
NtSetQuotaInformationFile(
	HANDLE FileHandle,
    	PIO_STATUS_BLOCK IoStatusBlock,
	PFILE_USER_QUOTA_INFORMATION Buffer,
    	ULONG BufferLength)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
NTSTATUS
STDCALL
IoCheckQuerySetFileInformation(
    IN FILE_INFORMATION_CLASS FileInformationClass,
    IN ULONG Length,
    IN BOOLEAN SetOperation
    )
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
IoCheckQuerySetVolumeInformation(
    IN FS_INFORMATION_CLASS FsInformationClass,
    IN ULONG Length,
    IN BOOLEAN SetOperation
    )
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
IoCheckQuotaBufferValidity(
    IN PFILE_QUOTA_INFORMATION QuotaBuffer,
    IN ULONG QuotaLength,
    OUT PULONG ErrorOffset
    )
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
IoCreateFileSpecifyDeviceObjectHint(
    OUT PHANDLE FileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PLARGE_INTEGER AllocationSize OPTIONAL,
    IN ULONG FileAttributes,
    IN ULONG ShareAccess,
    IN ULONG Disposition,
    IN ULONG CreateOptions,
    IN PVOID EaBuffer OPTIONAL,
    IN ULONG EaLength,
    IN CREATE_FILE_TYPE CreateFileType,
    IN PVOID ExtraCreateParameters OPTIONAL,
    IN ULONG Options,
    IN PVOID DeviceObject
    )
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
PFILE_OBJECT
STDCALL
IoCreateStreamFileObjectEx(
    IN PFILE_OBJECT FileObject OPTIONAL,
    IN PDEVICE_OBJECT DeviceObject OPTIONAL,
    OUT PHANDLE FileObjectHandle OPTIONAL
    )
{
	UNIMPLEMENTED;
	return 0;
}
/*
 * @unimplemented
 */
PFILE_OBJECT
STDCALL
IoCreateStreamFileObjectLite(
    IN PFILE_OBJECT FileObject OPTIONAL,
    IN PDEVICE_OBJECT DeviceObject OPTIONAL
    )
{
	UNIMPLEMENTED;
	return 0;
}

/*
 * @implemented
 */
BOOLEAN
STDCALL
IoIsFileOriginRemote(IN PFILE_OBJECT FileObject)
{
    /* Return the flag status */
    return (FileObject->Flags & FO_REMOTE_ORIGIN);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
IoQueryFileDosDeviceName(
    IN PFILE_OBJECT FileObject,
    OUT POBJECT_NAME_INFORMATION *ObjectNameInformation
    )
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
NTSTATUS STDCALL
IoQueryFileInformation(IN PFILE_OBJECT FileObject,
		       IN FILE_INFORMATION_CLASS FileInformationClass,
		       IN ULONG Length,
		       OUT PVOID FileInformation,
		       OUT PULONG ReturnedLength)
{
   IO_STATUS_BLOCK IoStatusBlock;
   PIRP Irp;
   PDEVICE_OBJECT DeviceObject;
   PIO_STACK_LOCATION StackPtr;
   NTSTATUS Status;
   
   ASSERT(FileInformation != NULL);
   
   Status = ObReferenceObjectByPointer(FileObject,
				       FILE_READ_ATTRIBUTES,
				       IoFileObjectType,
				       KernelMode);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   DPRINT("FileObject %x\n", FileObject);
   
   DeviceObject = FileObject->DeviceObject;
   
   Irp = IoAllocateIrp(DeviceObject->StackSize,
		       TRUE);
   if (Irp == NULL)
     {
	ObDereferenceObject(FileObject);
	return STATUS_INSUFFICIENT_RESOURCES;
     }

   /* Trigger FileObject/Event dereferencing */
   Irp->Tail.Overlay.OriginalFileObject = FileObject;
   Irp->RequestorMode = KernelMode;
   Irp->AssociatedIrp.SystemBuffer = FileInformation;
   Irp->UserIosb = &IoStatusBlock;
   Irp->UserEvent = &FileObject->Event;
   Irp->Tail.Overlay.Thread = PsGetCurrentThread();
   KeResetEvent( &FileObject->Event );
   
   StackPtr = IoGetNextIrpStackLocation(Irp);
   StackPtr->MajorFunction = IRP_MJ_QUERY_INFORMATION;
   StackPtr->MinorFunction = 0;
   StackPtr->Flags = 0;
   StackPtr->Control = 0;
   StackPtr->DeviceObject = DeviceObject;
   StackPtr->FileObject = FileObject;
   
   StackPtr->Parameters.QueryFile.FileInformationClass =
     FileInformationClass;
   StackPtr->Parameters.QueryFile.Length = Length;
   
   Status = IoCallDriver(FileObject->DeviceObject,
			 Irp);
   if (Status==STATUS_PENDING && (FileObject->Flags & FO_SYNCHRONOUS_IO))
     {
	KeWaitForSingleObject(&FileObject->Event,
			      Executive,
			      KernelMode,
			      FileObject->Flags & FO_ALERTABLE_IO,
			      NULL);
	Status = IoStatusBlock.Status;
     }
   
   if (ReturnedLength != NULL)
     {
	*ReturnedLength = IoStatusBlock.Information;
     }
   
   
   return Status;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
NtSetInformationFile(HANDLE FileHandle,
		     PIO_STATUS_BLOCK IoStatusBlock,
		     PVOID FileInformation,
		     ULONG Length,
		     FILE_INFORMATION_CLASS FileInformationClass)
{
   PIO_STACK_LOCATION StackPtr;
   PFILE_OBJECT FileObject;
   PDEVICE_OBJECT DeviceObject;
   PIRP Irp;
   NTSTATUS Status;
   PVOID SystemBuffer;
   KPROCESSOR_MODE PreviousMode;
   
   ASSERT(IoStatusBlock != NULL);
   ASSERT(FileInformation != NULL);
   
   DPRINT("NtSetInformationFile(Handle %x StatBlk %x FileInfo %x Length %d "
	  "Class %d)\n", FileHandle, IoStatusBlock, FileInformation,
	  Length, FileInformationClass);

   PreviousMode = ExGetPreviousMode();

   /*  Get the file object from the file handle  */
   Status = ObReferenceObjectByHandle(FileHandle,
				      0, /* FIXME - depends on the information class */
				      IoFileObjectType,
				      PreviousMode,
				      (PVOID *)&FileObject,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return Status;
     }
   
   DPRINT("FileObject %x\n", FileObject);

   /* io completion port? */
   if (FileInformationClass == FileCompletionInformation)
   {
      PKQUEUE Queue;

      if (Length < sizeof(FILE_COMPLETION_INFORMATION))
      {
         Status = STATUS_INFO_LENGTH_MISMATCH;
      }
      else
      {
         Status = ObReferenceObjectByHandle(((PFILE_COMPLETION_INFORMATION)FileInformation)->IoCompletionHandle,
                                            IO_COMPLETION_MODIFY_STATE,//???
                                            ExIoCompletionType,
                                            PreviousMode,
                                            (PVOID*)&Queue,
                                            NULL);
         if (NT_SUCCESS(Status))
         {   
            /* FIXME: maybe use lookaside list */
            FileObject->CompletionContext = ExAllocatePool(NonPagedPool, sizeof(IO_COMPLETION_CONTEXT));
            FileObject->CompletionContext->Key = ((PFILE_COMPLETION_INFORMATION)FileInformation)->CompletionKey;
            FileObject->CompletionContext->Port = Queue;

            ObDereferenceObject(Queue);
         }
      }

      ObDereferenceObject(FileObject);
      return Status;
   }

   DeviceObject = FileObject->DeviceObject;
   
   Irp = IoAllocateIrp(DeviceObject->StackSize,
		       TRUE);
   if (Irp == NULL)
     {
	ObDereferenceObject(FileObject);
	return STATUS_INSUFFICIENT_RESOURCES;
     }
   
   SystemBuffer = ExAllocatePoolWithTag(NonPagedPool,
					Length,
					TAG_SYSB);
   if (SystemBuffer == NULL)
     {
	IoFreeIrp(Irp);
	ObDereferenceObject(FileObject);
	return(STATUS_INSUFFICIENT_RESOURCES);
     }
   
   MmSafeCopyFromUser(SystemBuffer,
		      FileInformation,
		      Length);
   
   /* Trigger FileObject/Event dereferencing */
   Irp->Tail.Overlay.OriginalFileObject = FileObject;
   Irp->RequestorMode = PreviousMode;
   Irp->AssociatedIrp.SystemBuffer = SystemBuffer;
   Irp->UserIosb = IoStatusBlock;
   Irp->UserEvent = &FileObject->Event;
   KeResetEvent( &FileObject->Event );
   Irp->Tail.Overlay.Thread = PsGetCurrentThread();
   
   StackPtr = IoGetNextIrpStackLocation(Irp);
   StackPtr->MajorFunction = IRP_MJ_SET_INFORMATION;
   StackPtr->MinorFunction = 0;
   StackPtr->Flags = 0;
   StackPtr->Control = 0;
   StackPtr->DeviceObject = DeviceObject;
   StackPtr->FileObject = FileObject;
   
   StackPtr->Parameters.SetFile.FileInformationClass =
     FileInformationClass;
   StackPtr->Parameters.SetFile.Length = Length;
   
   /*
    * Pass the IRP to the FSD (and wait for
    * it if required)
    */
   DPRINT("FileObject->DeviceObject %x\n", FileObject->DeviceObject);
   Status = IoCallDriver(FileObject->DeviceObject,
			 Irp);
   if (Status == STATUS_PENDING && (FileObject->Flags & FO_SYNCHRONOUS_IO))
     {
	KeWaitForSingleObject(&FileObject->Event,
			      Executive,
			      PreviousMode,
			      FileObject->Flags & FO_ALERTABLE_IO,
			      NULL);
	Status = IoStatusBlock->Status;
     }

   ExFreePool(SystemBuffer);

   return Status;
}


/*
 * @unimplemented
 */
NTSTATUS
STDCALL
IoSetFileOrigin(
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Remote
    )
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS STDCALL
NtQueryAttributesFile(IN POBJECT_ATTRIBUTES ObjectAttributes,
		      OUT PFILE_BASIC_INFORMATION FileInformation)
{
  IO_STATUS_BLOCK IoStatusBlock;
  HANDLE FileHandle;
  NTSTATUS Status;

  /* Open the file */
  Status = ZwOpenFile (&FileHandle,
		       SYNCHRONIZE | FILE_READ_ATTRIBUTES,
		       ObjectAttributes,
		       &IoStatusBlock,
		       FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		       FILE_SYNCHRONOUS_IO_NONALERT);
  if (!NT_SUCCESS (Status))
    {
      DPRINT ("ZwOpenFile() failed (Status %lx)\n", Status);
      return Status;
    }

  /* Get file attributes */
  Status = ZwQueryInformationFile (FileHandle,
				   &IoStatusBlock,
				   FileInformation,
				   sizeof(FILE_BASIC_INFORMATION),
				   FileBasicInformation);
  ZwClose (FileHandle);
  if (!NT_SUCCESS (Status))
    {
      DPRINT ("ZwQueryInformationFile() failed (Status %lx)\n", Status);
    }

  return Status;
}


NTSTATUS STDCALL
NtQueryFullAttributesFile(IN POBJECT_ATTRIBUTES ObjectAttributes,
			  OUT PFILE_NETWORK_OPEN_INFORMATION FileInformation)
{
  IO_STATUS_BLOCK IoStatusBlock;
  HANDLE FileHandle;
  NTSTATUS Status;

  /* Open the file */
  Status = ZwOpenFile (&FileHandle,
		       SYNCHRONIZE | FILE_READ_ATTRIBUTES,
		       ObjectAttributes,
		       &IoStatusBlock,
		       FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		       FILE_SYNCHRONOUS_IO_NONALERT);
  if (!NT_SUCCESS (Status))
    {
      DPRINT ("ZwOpenFile() failed (Status %lx)\n", Status);
      return Status;
    }

  /* Get file attributes */
  Status = ZwQueryInformationFile (FileHandle,
				   &IoStatusBlock,
				   FileInformation,
				   sizeof(FILE_NETWORK_OPEN_INFORMATION),
				   FileNetworkOpenInformation);
  ZwClose (FileHandle);
  if (!NT_SUCCESS (Status))
    {
      DPRINT ("ZwQueryInformationFile() failed (Status %lx)\n", Status);
    }

  return Status;
}


/*
 * @unimplemented
 */
NTSTATUS STDCALL
NtQueryEaFile(IN HANDLE FileHandle,
	      OUT PIO_STATUS_BLOCK IoStatusBlock,
	      OUT PVOID Buffer,
	      IN ULONG Length,
	      IN BOOLEAN ReturnSingleEntry,
	      IN PVOID EaList OPTIONAL,
	      IN ULONG EaListLength,
	      IN PULONG EaIndex OPTIONAL,
	      IN BOOLEAN RestartScan)
{
   UNIMPLEMENTED;
   return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
NTSTATUS STDCALL
NtSetEaFile(IN HANDLE FileHandle,
	    IN PIO_STATUS_BLOCK	IoStatusBlock,
	    IN PVOID EaBuffer,
	    IN ULONG EaBufferSize)
{
   UNIMPLEMENTED;
   return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
