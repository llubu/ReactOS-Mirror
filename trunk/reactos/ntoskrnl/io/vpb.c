/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/vpb.c
 * PURPOSE:         Volume Parameter Block managment
 * 
 * PROGRAMMERS:     David Welch (welch@mcmail.com)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

static KSPIN_LOCK IoVpbLock;

#define TAG_VPB    TAG('V', 'P', 'B', ' ')
#define TAG_SYSB   TAG('S', 'Y', 'S', 'B')

/* FUNCTIONS *****************************************************************/

VOID INIT_FUNCTION
IoInitVpbImplementation(VOID)
{
   KeInitializeSpinLock(&IoVpbLock);
}

NTSTATUS
IoAttachVpb(PDEVICE_OBJECT DeviceObject)
{
   PVPB Vpb;
   
   Vpb = ExAllocatePoolWithTag(NonPagedPool,
			       sizeof(VPB),
			       TAG_VPB);
   if (Vpb == NULL)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   
   Vpb->Type = 0;
   Vpb->Size = sizeof(VPB) / sizeof(DWORD);
   Vpb->Flags = 0;
   Vpb->VolumeLabelLength = 0;
   Vpb->DeviceObject = NULL;
   Vpb->RealDevice = DeviceObject;
   Vpb->SerialNumber = 0;
   Vpb->ReferenceCount = 0;
   RtlZeroMemory(Vpb->VolumeLabel,
		 sizeof(WCHAR) * MAXIMUM_VOLUME_LABEL_LENGTH);
   
   DeviceObject->Vpb = Vpb;
   
   return(STATUS_SUCCESS);
}


/*
 * FUNCTION: Queries the volume information
 * ARGUMENTS: 
 *	  FileHandle  = Handle to a file object on the target volume
 *	  ReturnLength = DataWritten
 *	  FsInformation = Caller should supply storage for the information 
 *	                  structure.
 *	  Length = Size of the information structure
 *	  FsInformationClass = Index to a information structure
 *
 *		FileFsVolumeInformation		FILE_FS_VOLUME_INFORMATION
 *		FileFsLabelInformation		FILE_FS_LABEL_INFORMATION
 *		FileFsSizeInformation		FILE_FS_SIZE_INFORMATION
 *		FileFsDeviceInformation		FILE_FS_DEVICE_INFORMATION
 *		FileFsAttributeInformation	FILE_FS_ATTRIBUTE_INFORMATION
 *		FileFsControlInformation	
 *		FileFsQuotaQueryInformation	--
 *		FileFsQuotaSetInformation	--
 *		FileFsMaximumInformation	
 *
 * RETURNS: Status
 *
 * @implemented
 */

NTSTATUS STDCALL
NtQueryVolumeInformationFile(IN HANDLE FileHandle,
			     OUT PIO_STATUS_BLOCK IoStatusBlock,
			     OUT PVOID FsInformation,
			     IN ULONG Length,
			     IN FS_INFORMATION_CLASS FsInformationClass)
{
   PFILE_OBJECT FileObject;
   PDEVICE_OBJECT DeviceObject;
   PIRP Irp;
   NTSTATUS Status;
   PIO_STACK_LOCATION StackPtr;
   PVOID SystemBuffer;
   KPROCESSOR_MODE PreviousMode;
   
   ASSERT(IoStatusBlock != NULL);
   ASSERT(FsInformation != NULL);
   
   DPRINT("FsInformation %p\n", FsInformation);

   PreviousMode = ExGetPreviousMode();

   Status = ObReferenceObjectByHandle(FileHandle,
				      0, /* FIXME - depends on the information class! */
				      IoFileObjectType,
				      PreviousMode,
				      (PVOID*)&FileObject,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   DeviceObject = FileObject->DeviceObject;
   
   Irp = IoAllocateIrp(DeviceObject->StackSize,
		       TRUE);
   if (Irp == NULL)
     {
	ObDereferenceObject(FileObject);
	return(STATUS_INSUFFICIENT_RESOURCES);
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
   KeResetEvent( &FileObject->Event );
   Irp->UserEvent = &FileObject->Event;
   Irp->UserIosb = IoStatusBlock;
   Irp->Tail.Overlay.Thread = PsGetCurrentThread();
   
   StackPtr = IoGetNextIrpStackLocation(Irp);
   StackPtr->MajorFunction = IRP_MJ_QUERY_VOLUME_INFORMATION;
   StackPtr->MinorFunction = 0;
   StackPtr->Flags = 0;
   StackPtr->Control = 0;
   StackPtr->DeviceObject = DeviceObject;
   StackPtr->FileObject = FileObject;
   StackPtr->Parameters.QueryVolume.Length = Length;
   StackPtr->Parameters.QueryVolume.FsInformationClass =
	FsInformationClass;
   
   Status = IoCallDriver(DeviceObject,
			 Irp);
   if (Status == STATUS_PENDING)
     {
	KeWaitForSingleObject(&FileObject->Event,
			      UserRequest,
			      PreviousMode,
			      FALSE,
			      NULL);
	Status = IoStatusBlock->Status;
     }
   DPRINT("Status %x\n", Status);
   
   if (NT_SUCCESS(Status))
     {
	DPRINT("Information %lu\n", IoStatusBlock->Information);
	MmSafeCopyToUser(FsInformation,
			 SystemBuffer,
			 IoStatusBlock->Information);
     }

   ExFreePool(SystemBuffer);
   
   return(Status);
}


/*
 * @implemented
 */
NTSTATUS STDCALL
IoQueryVolumeInformation(IN PFILE_OBJECT FileObject,
			 IN FS_INFORMATION_CLASS FsInformationClass,
			 IN ULONG Length,
			 OUT PVOID FsInformation,
			 OUT PULONG ReturnedLength)
{
   IO_STATUS_BLOCK IoStatusBlock;
   PIO_STACK_LOCATION StackPtr;
   PDEVICE_OBJECT DeviceObject;
   PIRP Irp;
   NTSTATUS Status;
   
   ASSERT(FsInformation != NULL);
   
   DPRINT("FsInformation %p\n", FsInformation);
   
   Status = ObReferenceObjectByPointer(FileObject,
				       FILE_READ_ATTRIBUTES,
				       IoFileObjectType,
				       KernelMode);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   DeviceObject = FileObject->DeviceObject;
   
   Irp = IoAllocateIrp(DeviceObject->StackSize,
		       TRUE);
   if (Irp == NULL)
     {
	ObDereferenceObject(FileObject);
	return(STATUS_INSUFFICIENT_RESOURCES);
     }

   /* Trigger FileObject/Event dereferencing */
   Irp->Tail.Overlay.OriginalFileObject = FileObject;
   Irp->RequestorMode = KernelMode;
   Irp->AssociatedIrp.SystemBuffer = FsInformation;
   KeResetEvent( &FileObject->Event );
   Irp->UserEvent = &FileObject->Event;
   Irp->UserIosb = &IoStatusBlock;
   Irp->Tail.Overlay.Thread = PsGetCurrentThread();
   
   StackPtr = IoGetNextIrpStackLocation(Irp);
   StackPtr->MajorFunction = IRP_MJ_QUERY_VOLUME_INFORMATION;
   StackPtr->MinorFunction = 0;
   StackPtr->Flags = 0;
   StackPtr->Control = 0;
   StackPtr->DeviceObject = DeviceObject;
   StackPtr->FileObject = FileObject;
   StackPtr->Parameters.QueryVolume.Length = Length;
   StackPtr->Parameters.QueryVolume.FsInformationClass =
	FsInformationClass;
   
   Status = IoCallDriver(DeviceObject,
			 Irp);
   if (Status == STATUS_PENDING)
     {
	KeWaitForSingleObject(&FileObject->Event,
			      UserRequest,
			      KernelMode,
			      FALSE,
			      NULL);
	Status = IoStatusBlock.Status;
     }
   DPRINT("Status %x\n", Status);
   
   if (ReturnedLength != NULL)
     {
	*ReturnedLength = IoStatusBlock.Information;
     }
   
   return(Status);
}


/*
 * @implemented
 */
NTSTATUS STDCALL
NtSetVolumeInformationFile(IN HANDLE FileHandle,
			   OUT PIO_STATUS_BLOCK IoStatusBlock,
			   IN PVOID FsInformation,
			   IN ULONG Length,
			   IN FS_INFORMATION_CLASS FsInformationClass)
{
   PFILE_OBJECT FileObject;
   PDEVICE_OBJECT DeviceObject;
   PIRP Irp;
   NTSTATUS Status;
   PIO_STACK_LOCATION StackPtr;
   PVOID SystemBuffer;
   KPROCESSOR_MODE PreviousMode;

   ASSERT(IoStatusBlock != NULL);
   ASSERT(FsInformation != NULL);

   PreviousMode = ExGetPreviousMode();

   Status = ObReferenceObjectByHandle(FileHandle,
				      FILE_WRITE_ATTRIBUTES,
				      NULL,
				      PreviousMode,
				      (PVOID*)&FileObject,
				      NULL);
   if (Status != STATUS_SUCCESS)
     {
	return(Status);
     }
   
   DeviceObject = FileObject->DeviceObject;
   
   Irp = IoAllocateIrp(DeviceObject->StackSize,TRUE);
   if (Irp == NULL)
     {
	ObDereferenceObject(FileObject);
	return(STATUS_INSUFFICIENT_RESOURCES);
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
		      FsInformation,
		      Length);
   
   /* Trigger FileObject/Event dereferencing */
   Irp->Tail.Overlay.OriginalFileObject = FileObject;
   Irp->RequestorMode = PreviousMode;
   Irp->AssociatedIrp.SystemBuffer = SystemBuffer;
   KeResetEvent( &FileObject->Event );
   Irp->UserEvent = &FileObject->Event;
   Irp->UserIosb = IoStatusBlock;
   Irp->Tail.Overlay.Thread = PsGetCurrentThread();
   
   StackPtr = IoGetNextIrpStackLocation(Irp);
   StackPtr->MajorFunction = IRP_MJ_SET_VOLUME_INFORMATION;
   StackPtr->MinorFunction = 0;
   StackPtr->Flags = 0;
   StackPtr->Control = 0;
   StackPtr->DeviceObject = DeviceObject;
   StackPtr->FileObject = FileObject;
   StackPtr->Parameters.SetVolume.Length = Length;
   StackPtr->Parameters.SetVolume.FsInformationClass =
	FsInformationClass;
   
   Status = IoCallDriver(DeviceObject,Irp);
   if (Status == STATUS_PENDING)
     {
	KeWaitForSingleObject(&FileObject->Event,
			      UserRequest,
			      PreviousMode,
			      FALSE,
			      NULL);
	Status = IoStatusBlock->Status;
     }

   ExFreePool(SystemBuffer);
   
   return(Status);
}


/*
 * @implemented
 */
VOID STDCALL
IoAcquireVpbSpinLock(OUT PKIRQL Irql)
{
   KeAcquireSpinLock(&IoVpbLock,
		     Irql);
}


/*
 * @implemented
 */
VOID STDCALL
IoReleaseVpbSpinLock(IN KIRQL Irql)
{
   KeReleaseSpinLock(&IoVpbLock,
		     Irql);
}

/* EOF */
