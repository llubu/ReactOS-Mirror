/* $Id: rw.c,v 1.26 1999/08/29 06:59:06 ea Exp $
 *
 * COPYRIGHT:      See COPYING in the top level directory
 * PROJECT:        ReactOS kernel
 * FILE:           ntoskrnl/io/rw.c
 * PURPOSE:        Implements read/write APIs
 * PROGRAMMER:     David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                 30/05/98: Created
 */

/* INCLUDES ****************************************************************/

#include <ddk/ntddk.h>
#include <internal/io.h>
#include <string.h>
#include <internal/string.h>
#include <internal/ob.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS ***************************************************************/


/**********************************************************************
 * NAME							EXPORTED
 *	NtReadFile
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 *
 */
NTSTATUS
STDCALL
NtReadFile (
	HANDLE			FileHandle,
	HANDLE			EventHandle,
	PIO_APC_ROUTINE		ApcRoutine,
	PVOID			ApcContext,
	PIO_STATUS_BLOCK	IoStatusBlock,
	PVOID			Buffer,
	ULONG			Length,
	PLARGE_INTEGER		ByteOffset,
	PULONG			Key
	)
{
	NTSTATUS		Status;
	PFILE_OBJECT		FileObject;
	PIRP			Irp;
	PIO_STACK_LOCATION	StackPtr;
	PKEVENT			ptrEvent = NULL;
	KEVENT			Event;
   
	DPRINT(
		"NtReadFile(FileHandle %x Buffer %x Length %x ByteOffset %x, "
		"IoStatusBlock %x)\n",
		FileHandle,
		Buffer,
		Length,
		ByteOffset,
		IoStatusBlock
		);

	assert_irql(PASSIVE_LEVEL);
   
	Status = ObReferenceObjectByHandle(
			FileHandle,
			FILE_READ_DATA,
			IoFileType,
			UserMode,
			(PVOID *) & FileObject,
			NULL
			);
	if (!NT_SUCCESS(Status))
	{
		DPRINT("NtReadFile() = %x\n",Status);
		return Status;
	}
   
	DPRINT(
		"ByteOffset %x FileObject->CurrentByteOffset %d\n",
		ByteOffset,
		FileObject->CurrentByteOffset.LowPart
		);
	if (ByteOffset == NULL)
	{
		ByteOffset = & (FileObject->CurrentByteOffset);
	}
   
	if (EventHandle != NULL)
	{
		Status = ObReferenceObjectByHandle(
				EventHandle,
				SYNCHRONIZE,
				ExEventType,
				UserMode,
				(PVOID *) ptrEvent,
				NULL
				);
		if (!NT_SUCCESS(Status))
		{
			return Status;
		}
	}
	else
	{
		KeInitializeEvent(
			& Event,
			NotificationEvent,
			FALSE
			);
		ptrEvent = & Event;
	}
					   
	DPRINT("FileObject %x\n",FileObject);
	
	Irp = IoBuildSynchronousFsdRequest(
			IRP_MJ_READ,
			FileObject->DeviceObject,
			Buffer,
			Length,
			ByteOffset,
			ptrEvent,
			IoStatusBlock
			);
   
	Irp->Overlay.AsynchronousParameters.UserApcRoutine = ApcRoutine;
	Irp->Overlay.AsynchronousParameters.UserApcContext = ApcContext;
   
	StackPtr = IoGetNextIrpStackLocation(Irp);
	StackPtr->FileObject = FileObject;
	if (Key != NULL)
	{
		StackPtr->Parameters.Read.Key = *Key;
	}
	else
	{
		StackPtr->Parameters.Read.Key = 0;
	}
   
	Status = IoCallDriver(
			FileObject->DeviceObject,
			Irp
			);
	if (
		(Status == STATUS_PENDING)
		&& (FileObject->Flags & FO_SYNCHRONOUS_IO)
		)
	{
		KeWaitForSingleObject(
			& Event,
			Executive,
			KernelMode,
			FALSE,
			NULL
			);
		Status = IoStatusBlock->Status;
	}
	
	DPRINT("NtReadFile() = %x\n",Status);
	
	assert_irql(PASSIVE_LEVEL);
	
	return Status;
}


/**********************************************************************
 * NAME							EXPORTED
 *	NtWriteFile
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 *
 */
NTSTATUS
STDCALL
NtWriteFile (
	HANDLE			FileHandle,
	HANDLE			EventHandle,
	PIO_APC_ROUTINE		ApcRoutine,
	PVOID			ApcContext,
	PIO_STATUS_BLOCK	IoStatusBlock,
	PVOID			Buffer,
	ULONG			Length,
	PLARGE_INTEGER		ByteOffset,
	PULONG			Key
	)
{
	NTSTATUS		Status;
	PFILE_OBJECT		FileObject;
	PIRP			Irp;
	PIO_STACK_LOCATION	StackPtr;
	KEVENT			Event;
   
	DPRINT(
		"NtWriteFile(FileHandle %x, Buffer %x, Length %d)\n",
		FileHandle,
		Buffer,
		Length
		);
   
	Status = ObReferenceObjectByHandle(
			FileHandle,
			FILE_WRITE_DATA,
			IoFileType,
			UserMode,
			(PVOID *) & FileObject,
			NULL
			);
	if (!NT_SUCCESS(Status))
	{
		return Status;
	}
	if (ByteOffset == NULL)
	{
		ByteOffset = & (FileObject->CurrentByteOffset);
	}
   
	KeInitializeEvent(
		& Event,
		NotificationEvent,
		FALSE
		);
	Irp = IoBuildSynchronousFsdRequest(
			IRP_MJ_WRITE,
			FileObject->DeviceObject,
			Buffer,
			Length,
			ByteOffset,
			& Event,
			IoStatusBlock
			);
   
	Irp->Overlay.AsynchronousParameters.UserApcRoutine = ApcRoutine;
	Irp->Overlay.AsynchronousParameters.UserApcContext = ApcContext;
   
	DPRINT("FileObject->DeviceObject %x\n",FileObject->DeviceObject);
	
	StackPtr = IoGetNextIrpStackLocation(Irp);
	StackPtr->FileObject = FileObject;
	if (Key != NULL)
	{
		StackPtr->Parameters.Write.Key = *Key;
	}
	else
	{
		StackPtr->Parameters.Write.Key = 0;
	}
	Status = IoCallDriver(
			FileObject->DeviceObject,
			Irp
			);
	if (
		(Status == STATUS_PENDING)
		&& (FileObject->Flags & FO_SYNCHRONOUS_IO)
		)
	{
		KeWaitForSingleObject(
			& Event,
			Executive,
			KernelMode,
			FALSE,
			NULL
			);
		Status = Irp->IoStatus.Status;
	}
	return Status;
}


/**********************************************************************
 * NAME							EXPORTED
 *	NtReadFileScatter
 *	
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 *
 */
NTSTATUS
STDCALL
NtReadFileScatter (
	IN	HANDLE			FileHandle, 
	IN	HANDLE			Event			OPTIONAL, 
	IN	PIO_APC_ROUTINE		UserApcRoutine		OPTIONAL, 
	IN	PVOID			UserApcContext		OPTIONAL, 
	OUT	PIO_STATUS_BLOCK	UserIoStatusBlock, 
	IN	FILE_SEGMENT_ELEMENT	BufferDescription [], 
	IN	ULONG			BufferLength, 
	IN	PLARGE_INTEGER		ByteOffset, 
	IN	PULONG			Key			OPTIONAL
	)
{
	UNIMPLEMENTED;
}


/**********************************************************************
 * NAME							EXPORTED
 *	NtWriteFileGather
 *	
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 *
 */
NTSTATUS
STDCALL
NtWriteFileGather (
	IN	HANDLE			FileHandle, 
	IN	HANDLE			Event OPTIONAL, 
	IN	PIO_APC_ROUTINE		ApcRoutine		OPTIONAL, 
	IN	PVOID			ApcContext		OPTIONAL, 
	OUT	PIO_STATUS_BLOCK	IoStatusBlock,
	IN	FILE_SEGMENT_ELEMENT	BufferDescription [],
	IN	ULONG			BufferLength, 
	IN	PLARGE_INTEGER		ByteOffset, 
	IN	PULONG			Key			OPTIONAL
	)
{
	UNIMPLEMENTED;
}


/* EOF */
