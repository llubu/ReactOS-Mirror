/* $Id: vpb.c,v 1.11 2001/03/07 16:48:42 dwelch Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/vpb.c
 * PURPOSE:         Volume Parameter Block managment
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/io.h>
#include <internal/pool.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

static KSPIN_LOCK IoVpbLock;

#define TAG_VPB    TAG('V', 'P', 'B', ' ')

/* FUNCTIONS *****************************************************************/

VOID
IoInitVpbImplementation (
	VOID
	)
{
	KeInitializeSpinLock(&IoVpbLock);
}

NTSTATUS IoAttachVpb(PDEVICE_OBJECT DeviceObject)
{
   PVPB Vpb;
   
   Vpb = ExAllocatePoolWithTag(NonPagedPool, sizeof(VPB), TAG_VPB);
   if (Vpb==NULL)
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
   RtlZeroMemory(Vpb->VolumeLabel,sizeof(WCHAR)*MAXIMUM_VOLUME_LABEL_LENGTH);
   
   DeviceObject->Vpb = Vpb;
   
   return(STATUS_SUCCESS);
}

PIRP IoBuildVolumeInformationIrp(ULONG MajorFunction,
				 PFILE_OBJECT FileObject,
				 PVOID FSInformation,
				 ULONG Length,
				 CINT FSInformationClass,
				 PIO_STATUS_BLOCK IoStatusBlock,
				 PKEVENT Event)
{
   PIRP Irp;
   PIO_STACK_LOCATION StackPtr;
   PDEVICE_OBJECT DeviceObject;
   
   DeviceObject = FileObject->DeviceObject;
   
   Irp = IoAllocateIrp(DeviceObject->StackSize,TRUE);
   if (Irp==NULL)
     {
	return(NULL);
     }
   
//   Irp->AssociatedIrp.SystemBuffer = FSInformation;
   Irp->UserBuffer = FSInformation;
   
   StackPtr = IoGetNextIrpStackLocation(Irp);
   StackPtr->MajorFunction = MajorFunction;
   StackPtr->MinorFunction = 0;
   StackPtr->Flags = 0;
   StackPtr->Control = 0;
   StackPtr->DeviceObject = DeviceObject;
   StackPtr->FileObject = FileObject;
   Irp->UserEvent = Event;
   Irp->UserIosb = IoStatusBlock;
   
   switch (MajorFunction)
   {
      case IRP_MJ_SET_VOLUME_INFORMATION:
         StackPtr->Parameters.SetVolume.Length = Length;
         StackPtr->Parameters.SetVolume.FileInformationClass =
                     FSInformationClass;
         break;

      case IRP_MJ_QUERY_VOLUME_INFORMATION:
         StackPtr->Parameters.QueryVolume.Length = Length;
         StackPtr->Parameters.QueryVolume.FileInformationClass =
                     FSInformationClass;
         break;
   }
   return(Irp);
}


NTSTATUS
STDCALL
NtQueryVolumeInformationFile (

	IN	HANDLE			FileHandle,
	OUT	PIO_STATUS_BLOCK	IoStatusBlock,
	OUT	PVOID			FSInformation,
	IN	ULONG			Length,
	IN	FS_INFORMATION_CLASS	FSInformationClass
	)

/*
 * FUNCTION: Queries the volume information
 * ARGUMENTS: 
 *        FileHandle  = Handle to a file object on the target volume
 *	  ReturnLength = DataWritten
 *	  FSInformation = Caller should supply storage for the information 
 *                        structure.
 *	  Length = Size of the information structure
 *	  FSInformationClass = Index to a information structure
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
 */
{
   PFILE_OBJECT FileObject;
   PDEVICE_OBJECT DeviceObject;
   PIRP Irp;
   KEVENT Event;
   NTSTATUS Status;

   DPRINT("FSInformation %p\n", FSInformation);

   Status = ObReferenceObjectByHandle(FileHandle,
				      FILE_READ_ATTRIBUTES,
				      NULL,
				      UserMode,
				      (PVOID*)&FileObject,
				      NULL);   
   if (Status != STATUS_SUCCESS)
     {
	return(Status);
     }
   
   DeviceObject = FileObject->DeviceObject;
   
   KeInitializeEvent(&Event,NotificationEvent,FALSE);
   
   Irp = IoBuildVolumeInformationIrp(IRP_MJ_QUERY_VOLUME_INFORMATION,
				     FileObject,
				     FSInformation,
				     Length,
				     FSInformationClass,
				     IoStatusBlock,
				     &Event);
   Status = IoCallDriver(DeviceObject,Irp);
   if (Status == STATUS_PENDING)
     {
	KeWaitForSingleObject(&Event,UserRequest,KernelMode,FALSE,NULL);
	Status = IoStatusBlock->Status;
     }
   return(Status);
}


NTSTATUS
STDCALL
IoQueryVolumeInformation (
	IN	PFILE_OBJECT		FileObject,
	IN	FS_INFORMATION_CLASS	FsInformationClass,
	IN	ULONG			Length,
	OUT	PVOID			FsInformation,
	OUT	PULONG			ReturnedLength
	)
{
	UNIMPLEMENTED;
	return (STATUS_NOT_IMPLEMENTED);
}


NTSTATUS
STDCALL
NtSetVolumeInformationFile (
	IN	HANDLE	FileHandle,
	IN	CINT	VolumeInformationClass,
		PVOID	VolumeInformation,
		ULONG	Length
	)
{
   PFILE_OBJECT FileObject;
   PDEVICE_OBJECT DeviceObject;
   PIRP Irp;
   KEVENT Event;
   NTSTATUS Status;

   Status = ObReferenceObjectByHandle(FileHandle,
				      FILE_WRITE_ATTRIBUTES,
				      NULL,
				      UserMode,
				      (PVOID*)&FileObject,
				      NULL);   
   if (Status != STATUS_SUCCESS)
     {
	return(Status);
     }

   DeviceObject = FileObject->DeviceObject;
   
   KeInitializeEvent(&Event,NotificationEvent,FALSE);

   Irp = IoBuildVolumeInformationIrp(IRP_MJ_SET_VOLUME_INFORMATION,
				     FileObject,
				     VolumeInformation,
				     Length,
				     VolumeInformationClass,
				     NULL,
				     &Event);
   Status = IoCallDriver(DeviceObject,Irp);
   if (Status == STATUS_PENDING)
     {
	KeWaitForSingleObject(&Event,UserRequest,KernelMode,FALSE,NULL);
     }
   return(Status);
}


VOID
STDCALL
IoAcquireVpbSpinLock (
	OUT	PKIRQL	Irql
	)
{
	KeAcquireSpinLock (&IoVpbLock,
	                   Irql);
}


VOID
STDCALL
IoReleaseVpbSpinLock (
	IN	KIRQL	Irql
	)
{
	KeReleaseSpinLock (&IoVpbLock,
	                   Irql);
}


NTSTATUS
STDCALL
IoVerifyVolume (
	IN	PDEVICE_OBJECT	DeviceObject,
	IN	BOOLEAN		AllowRawMount
	)
{
	UNIMPLEMENTED;
	return (STATUS_NOT_IMPLEMENTED);
}


/* EOF */
