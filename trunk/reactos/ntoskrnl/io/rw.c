/* $Id: rw.c,v 1.42 2003/05/16 12:03:11 chorns Exp $
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
#include <internal/ob.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS ***************************************************************/


NTSTATUS STDCALL
IopReadWriteIoComplete(PDEVICE_OBJECT DeviceObject,
  PIRP Irp,
  PVOID Context)
{
  PIO_STACK_LOCATION IrpStack;

  DPRINT("IopReadWriteIoComplete(DeviceObject %p  Irp %p  Context %p) called\n",
	  DeviceObject, Irp, Context);

  IrpStack = IoGetCurrentIrpStackLocation(Irp);

  ObDereferenceObject(Irp->UserEvent);

  return STATUS_SUCCESS;
}

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
NTSTATUS STDCALL NtReadFile(HANDLE			FileHandle,
			    HANDLE			EventHandle,
			    PIO_APC_ROUTINE		ApcRoutine,
			    PVOID			ApcContext,
			    PIO_STATUS_BLOCK	        UserIoStatusBlock,
			    PVOID			Buffer,
			    ULONG			Length,
			    PLARGE_INTEGER		ByteOffset,
			    PULONG			Key)
{
  NTSTATUS Status;
  PFILE_OBJECT FileObject;
  PIRP Irp;
  PIO_STACK_LOCATION StackPtr;
  PKEVENT Event = NULL;
  IO_STATUS_BLOCK Iosb;
  PIO_STATUS_BLOCK IoStatusBlock;
  BOOLEAN SetIoCompletionRoutine;
  
  DPRINT("NtReadFile(FileHandle %x Buffer %x Length %x ByteOffset %x, "
	 "IoStatusBlock %x)\n", FileHandle, Buffer, Length, ByteOffset,
	 IoStatusBlock);
  
  Status = ObReferenceObjectByHandle(FileHandle,
				     FILE_READ_DATA,
				     IoFileObjectType,
				     UserMode,
				     (PVOID*)&FileObject,
				     NULL);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }
  
  if (ByteOffset == NULL)
    {
      ByteOffset = &FileObject->CurrentByteOffset;
    }
  
  if (EventHandle != NULL)
    {
      Status = ObReferenceObjectByHandle(EventHandle,
					 SYNCHRONIZE,
					 ExEventObjectType,
					 UserMode,
					 (PVOID*)&Event,
					 NULL);
      if (!NT_SUCCESS(Status))
	{
	  ObDereferenceObject(FileObject);
	  return(Status);
	}
      SetIoCompletionRoutine = TRUE;
    }
  else 
    {
      Event = &FileObject->Event;
      KeResetEvent(Event);
      SetIoCompletionRoutine = FALSE;
    }
  
  if (FileObject->Flags & FO_SYNCHRONOUS_IO)
    {
      IoStatusBlock = &Iosb;
    }
  else
    {
      IoStatusBlock = UserIoStatusBlock;
    }
  
  Irp = IoBuildSynchronousFsdRequest(IRP_MJ_READ,
				     FileObject->DeviceObject,
				     Buffer,
				     Length,
				     ByteOffset,
				     Event,
				     IoStatusBlock);
  
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
  
  if (SetIoCompletionRoutine)
    {
      /* Set completion routine */
      IoSetCompletionRoutine(Irp,
        IopReadWriteIoComplete,
        NULL,
        TRUE,
        TRUE,
        TRUE);
    }

  Status = IoCallDriver(FileObject->DeviceObject, Irp);
  if (Status == STATUS_PENDING && FileObject->Flags & FO_SYNCHRONOUS_IO)
     {
       BOOLEAN Alertable;
       
       if (FileObject->Flags & FO_ALERTABLE_IO)
	 {
	   Alertable = TRUE;
	 }
       else
	 {
	   Alertable = FALSE;
	 } 
       
       Status = KeWaitForSingleObject(Event,
				      Executive,
				      UserMode,
				      Alertable,
				      NULL);
       if (Status != STATUS_WAIT_0)
	 {
	   /* Wait failed. */
	   return(Status);
	 }

	Status = Iosb.Status;
	return(Status);
     }

  if (FileObject->Flags & FO_SYNCHRONOUS_IO)
    {
      *UserIoStatusBlock = Iosb;
    }
  return(Status);
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
NTSTATUS STDCALL NtWriteFile(HANDLE			FileHandle,
			     HANDLE			EventHandle,
			     PIO_APC_ROUTINE		ApcRoutine,
			     PVOID			ApcContext,
			     PIO_STATUS_BLOCK	        UserIoStatusBlock,
			     PVOID			Buffer,
			     ULONG			Length,
			     PLARGE_INTEGER		ByteOffset,
			     PULONG			Key)
{
  NTSTATUS Status;
  PFILE_OBJECT FileObject;
  PIRP Irp;
  PIO_STACK_LOCATION StackPtr;
  PKEVENT Event = NULL;
  IO_STATUS_BLOCK Iosb;
  PIO_STATUS_BLOCK IoStatusBlock;
  BOOLEAN SetIoCompletionRoutine;

  DPRINT("NtWriteFile(FileHandle %x Buffer %x Length %x ByteOffset %x, "
	 "IoStatusBlock %x)\n", FileHandle, Buffer, Length, ByteOffset,
	 IoStatusBlock);
  
  Status = ObReferenceObjectByHandle(FileHandle,
				     FILE_READ_DATA,
				     IoFileObjectType,
				     UserMode,
				     (PVOID*)&FileObject,
				     NULL);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }
  
  if (ByteOffset == NULL)
    {
      ByteOffset = &FileObject->CurrentByteOffset;
    }
  
  if (EventHandle != NULL)
    {
      Status = ObReferenceObjectByHandle(EventHandle,
					 SYNCHRONIZE,
					 ExEventObjectType,
					 UserMode,
					 (PVOID*)&Event,
					 NULL);
      if (!NT_SUCCESS(Status))
	{
	  ObDereferenceObject(FileObject);
	  return(Status);
	}
      SetIoCompletionRoutine = TRUE;
    }
  else 
    {
      Event = &FileObject->Event;
      KeResetEvent(Event);
      SetIoCompletionRoutine = FALSE;
    }
  
  if (FileObject->Flags & FO_SYNCHRONOUS_IO)
    {
      IoStatusBlock = &Iosb;
    }
  else
    {
      IoStatusBlock = UserIoStatusBlock;
    }
  
  Irp = IoBuildSynchronousFsdRequest(IRP_MJ_WRITE,
				     FileObject->DeviceObject,
				     Buffer,
				     Length,
				     ByteOffset,
				     Event,
				     IoStatusBlock);
  
  Irp->Overlay.AsynchronousParameters.UserApcRoutine = ApcRoutine;
  Irp->Overlay.AsynchronousParameters.UserApcContext = ApcContext;
  
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

  if (SetIoCompletionRoutine)
    {
      /* Set completion routine */
      IoSetCompletionRoutine(Irp,
        IopReadWriteIoComplete,
        NULL,
        TRUE,
        TRUE,
        TRUE);
    }

  Status = IoCallDriver(FileObject->DeviceObject, Irp);
  if (Status == STATUS_PENDING && FileObject->Flags & FO_SYNCHRONOUS_IO)
     {
       BOOLEAN Alertable;
       
       if (FileObject->Flags & FO_ALERTABLE_IO)
	 {
	   Alertable = TRUE;
	 }
       else
	 {
	   Alertable = FALSE;
	 } 
       
       Status = KeWaitForSingleObject(Event,
				      Executive,
				      UserMode,
				      Alertable,
				      NULL);
       if (Status != STATUS_WAIT_0)
	 {
	   /* Wait failed. */
	   return(Status);
	 }

	Status = Iosb.Status;
	return(Status);
     }

  if (FileObject->Flags & FO_SYNCHRONOUS_IO)
    {
      *UserIoStatusBlock = Iosb;
    }
  return(Status);
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
