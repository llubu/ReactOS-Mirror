/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/flush.c
 * PURPOSE:         Flushing file buffer
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/ob.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/


NTSTATUS
STDCALL
NtFlushWriteBuffer(VOID)
{
  KeFlushWriteBuffer();
  return STATUS_SUCCESS;
}

NTSTATUS
STDCALL
NtFlushBuffersFile (
	IN	HANDLE			FileHandle,
	OUT	PIO_STATUS_BLOCK	IoStatusBlock
	)
/*
 * FUNCTION: Flushes cached file data to disk
 * ARGUMENTS:
 *       FileHandle = Points to the file
 *	 IoStatusBlock = Caller must supply storage to receive the result of 
 *                       the flush buffers operation. The information field is
 *                       set to number of bytes flushed to disk.
 * RETURNS: Status 
 * REMARKS: This function maps to the win32 FlushFileBuffers
 */
{
   PFILE_OBJECT FileObject = NULL;
   PIRP Irp;
   PIO_STACK_LOCATION StackPtr;
   NTSTATUS Status;
   KPROCESSOR_MODE PreviousMode;

   PreviousMode = ExGetPreviousMode();

   Status = ObReferenceObjectByHandle(FileHandle,
				      FILE_WRITE_DATA,
				      NULL,
				      PreviousMode,
				      (PVOID*)&FileObject,
				      NULL);
   if (Status != STATUS_SUCCESS)
     {
	return(Status);
     }
   KeResetEvent( &FileObject->Event );
   Irp = IoBuildSynchronousFsdRequest(IRP_MJ_FLUSH_BUFFERS,
				      FileObject->DeviceObject,
				      NULL,
				      0,
				      NULL,
				      &FileObject->Event,
				      IoStatusBlock);

   /* Trigger FileObject/Event dereferencing */
   Irp->Tail.Overlay.OriginalFileObject = FileObject;

   Irp->RequestorMode = PreviousMode;

   StackPtr = IoGetNextIrpStackLocation(Irp);
   StackPtr->FileObject = FileObject;

   Status = IoCallDriver(FileObject->DeviceObject,Irp);
   if (Status == STATUS_PENDING)
     {
	KeWaitForSingleObject(&FileObject->Event,
			      Executive,
			      PreviousMode,
			      FileObject->Flags & FO_ALERTABLE_IO,
			      NULL);
	Status = IoStatusBlock->Status;
     }

   return(Status);
}
