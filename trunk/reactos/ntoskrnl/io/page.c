/* $Id: page.c,v 1.22 2004/08/15 16:39:03 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/page.c
 * PURPOSE:         
 * PROGRAMMER:      
 * UPDATE HISTORY:
 *                  
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL 
IoPageWrite(PFILE_OBJECT FileObject,
	    PMDL Mdl,
	    PLARGE_INTEGER Offset,
	    PKEVENT Event,
	    PIO_STATUS_BLOCK StatusBlock)
{
   PIRP Irp;
   PIO_STACK_LOCATION StackPtr;
   NTSTATUS Status;
   
   DPRINT("IoPageWrite(FileObject %x, Mdl %x)\n",
	  FileObject, Mdl);
   
   Irp = IoBuildSynchronousFsdRequestWithMdl(IRP_MJ_WRITE,
					     FileObject->DeviceObject,
					     Mdl,
					     Offset,
					     Event,
					     StatusBlock,
					     TRUE);
   if (Irp == NULL)
   {
      return (STATUS_INSUFFICIENT_RESOURCES);
   }
   Irp->Flags = IRP_NOCACHE|IRP_PAGING_IO;
   StackPtr = IoGetNextIrpStackLocation(Irp);
   StackPtr->FileObject = FileObject;
   DPRINT("Before IoCallDriver\n");
   Status = IofCallDriver(FileObject->DeviceObject,Irp);
   DPRINT("Status %d STATUS_PENDING %d\n",Status,STATUS_PENDING);
   return(Status);
}


/*
 * @implemented
 */
NTSTATUS STDCALL 
IoPageRead(PFILE_OBJECT FileObject,
	   PMDL Mdl,
	   PLARGE_INTEGER Offset,
	   PKEVENT Event,
	   PIO_STATUS_BLOCK StatusBlock)
{
   PIRP Irp;
   PIO_STACK_LOCATION StackPtr;
   NTSTATUS Status;
   
   DPRINT("IoPageRead(FileObject %x, Mdl %x)\n",
	  FileObject, Mdl);
   
   Irp = IoBuildSynchronousFsdRequestWithMdl(IRP_MJ_READ,
					     FileObject->DeviceObject,
					     Mdl,
					     Offset,
					     Event,
					     StatusBlock,
					     TRUE);
   if (Irp == NULL)
   {
      return (STATUS_INSUFFICIENT_RESOURCES);
   }
   Irp->Flags = IRP_NOCACHE|IRP_PAGING_IO;
   StackPtr = IoGetNextIrpStackLocation(Irp);
   StackPtr->FileObject = FileObject;
   DPRINT("Before IoCallDriver\n");
   Status = IofCallDriver(FileObject->DeviceObject, Irp);
   DPRINT("Status %d STATUS_PENDING %d\n",Status,STATUS_PENDING);

   return(Status);
}


/*
 * @implemented
 */
NTSTATUS STDCALL 
IoSynchronousPageWrite (PFILE_OBJECT FileObject,
			PMDL Mdl,
			PLARGE_INTEGER Offset,
			PKEVENT Event,
			PIO_STATUS_BLOCK StatusBlock)
{
   PIRP Irp;
   PIO_STACK_LOCATION StackPtr;
   NTSTATUS Status;
   
   DPRINT("IoSynchronousPageWrite(FileObject %x, Mdl %x)\n",
	  FileObject, Mdl);
   
   Irp = IoBuildSynchronousFsdRequestWithMdl(IRP_MJ_WRITE,
					     FileObject->DeviceObject,
					     Mdl,
					     Offset,
					     Event,
					     StatusBlock,
					     TRUE);
   if (Irp == NULL)
   {
      return (STATUS_INSUFFICIENT_RESOURCES);
   }
   Irp->Flags = IRP_NOCACHE|IRP_PAGING_IO|IRP_SYNCHRONOUS_PAGING_IO;
   StackPtr = IoGetNextIrpStackLocation(Irp);
   StackPtr->FileObject = FileObject;
   DPRINT("Before IoCallDriver\n");
   Status = IofCallDriver(FileObject->DeviceObject,Irp);
   DPRINT("Status %d STATUS_PENDING %d\n",Status,STATUS_PENDING);
   return(Status);
}


/* EOF */
