/* $Id: buildirp.c,v 1.40 2004/06/19 08:53:35 vizzini Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/buildirp.c
 * PURPOSE:         Building various types of irp
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 *                  Fixed IO method handling 04/03/99
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/pool.h>
#include <internal/io.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS ******************************************************************/

#define TAG_SYS_BUF  TAG('S', 'B', 'U', 'F')

/* FUNCTIONS *****************************************************************/

NTSTATUS IoPrepareIrpBuffer(PIRP Irp,
			    PDEVICE_OBJECT DeviceObject,
			    PVOID Buffer,
			    ULONG Length,
			    ULONG MajorFunction)
/*
 * FUNCTION: Prepares the buffer to be used for an IRP
 */
{
   Irp->UserBuffer = Buffer;
   if (DeviceObject->Flags & DO_BUFFERED_IO)
     {
	DPRINT("Doing buffer i/o\n");
	Irp->AssociatedIrp.SystemBuffer = 
	  (PVOID)ExAllocatePoolWithTag(NonPagedPool,Length, TAG_SYS_BUF);
	if (Irp->AssociatedIrp.SystemBuffer==NULL)
	  {
	     IoFreeIrp(Irp);
	     return(STATUS_NOT_IMPLEMENTED);
	  }
	/* FIXME: should copy buffer in on other ops */
	if (MajorFunction == IRP_MJ_WRITE)
	  {
	     RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer, Buffer, Length);
	  }
     }
   if (DeviceObject->Flags & DO_DIRECT_IO)
     {
	DPRINT("Doing direct i/o\n");
	
	Irp->MdlAddress = MmCreateMdl(NULL,Buffer,Length);
	if(Irp->MdlAddress == NULL) {
		DPRINT("MmCreateMdl: Out of memory!");
		return(STATUS_NO_MEMORY);
	}	
	if (MajorFunction == IRP_MJ_READ)
	  {
	     MmProbeAndLockPages(Irp->MdlAddress,UserMode,IoWriteAccess);
	  }
	else
	  {
	     MmProbeAndLockPages(Irp->MdlAddress,UserMode,IoReadAccess);
	  }
	Irp->UserBuffer = NULL;
	Irp->AssociatedIrp.SystemBuffer = NULL;
     }
   return(STATUS_SUCCESS);
}


/*
 * @implemented
 */
PIRP STDCALL
IoBuildAsynchronousFsdRequest(ULONG MajorFunction,
			      PDEVICE_OBJECT DeviceObject,
			      PVOID Buffer,
			      ULONG Length,
			      PLARGE_INTEGER StartingOffset,
			      PIO_STATUS_BLOCK IoStatusBlock)
/*
 * FUNCTION: Allocates and sets up an IRP to be sent to lower level drivers
 * ARGUMENTS:
 *         MajorFunction = One of IRP_MJ_READ, IRP_MJ_WRITE, 
 *                         IRP_MJ_FLUSH_BUFFERS or IRP_MJ_SHUTDOWN
 *         DeviceObject = Device object to send the irp to
 *         Buffer = Buffer into which data will be read or written
 *         Length = Length in bytes of the irp to be allocated
 *         StartingOffset = Starting offset on the device
 *         IoStatusBlock (OUT) = Storage for the result of the operation
 * RETURNS: The IRP allocated on success, or
 *          NULL on failure
 */
{
   PIRP Irp;
   PIO_STACK_LOCATION StackPtr;

   DPRINT("IoBuildAsynchronousFsdRequest(MajorFunction %x, DeviceObject %x, "
	  "Buffer %x, Length %x, StartingOffset %x, "
	  "IoStatusBlock %x\n",MajorFunction,DeviceObject,Buffer,Length,
	  StartingOffset,IoStatusBlock);

   Irp = IoAllocateIrp(DeviceObject->StackSize,TRUE);
   if (Irp==NULL)
     {
	return(NULL);
     }

   Irp->UserIosb = IoStatusBlock;
   DPRINT("Irp->UserIosb %x\n", Irp->UserIosb);
   Irp->Tail.Overlay.Thread = PsGetCurrentThread();

   StackPtr = IoGetNextIrpStackLocation(Irp);
   StackPtr->MajorFunction = (UCHAR)MajorFunction;
   StackPtr->MinorFunction = 0;
   StackPtr->Flags = 0;
   StackPtr->Control = 0;
   StackPtr->DeviceObject = DeviceObject;
   StackPtr->FileObject = NULL;
   StackPtr->CompletionRoutine = NULL;

   if (Buffer != NULL)
     {
	IoPrepareIrpBuffer(Irp,
			   DeviceObject,
			   Buffer,
			   Length,
			   MajorFunction);
     }

   if (MajorFunction == IRP_MJ_READ)
     {
	StackPtr->Parameters.Read.Length = Length;
	if (StartingOffset!=NULL)
	  {
	     StackPtr->Parameters.Read.ByteOffset = *StartingOffset;
	  }
	else
	  {
	     StackPtr->Parameters.Read.ByteOffset.QuadPart = 0;
	  }
     }
   else if (MajorFunction == IRP_MJ_WRITE)
     {
	StackPtr->Parameters.Write.Length = Length;
	if (StartingOffset!=NULL)
	  {
	     StackPtr->Parameters.Write.ByteOffset = *StartingOffset;
	  }
	else
	  {
	    StackPtr->Parameters.Write.ByteOffset.QuadPart = 0;
	  }
     }

   Irp->UserIosb = IoStatusBlock;

   return(Irp);
}


/*
 * @implemented
 */
PIRP STDCALL
IoBuildDeviceIoControlRequest(ULONG IoControlCode,
			      PDEVICE_OBJECT DeviceObject,
			      PVOID InputBuffer,
			      ULONG InputBufferLength,
			      PVOID OutputBuffer,
			      ULONG OutputBufferLength,
			      BOOLEAN InternalDeviceIoControl,
			      PKEVENT Event,
			      PIO_STATUS_BLOCK IoStatusBlock)
/*
 * FUNCTION: Allocates and sets up an IRP to be sent to drivers
 * ARGUMENTS:
 *         IoControlCode = Device io control code
 *         DeviceObject = Device object to send the irp to
 *         InputBuffer = Buffer from which data will be read by the driver
 *         InputBufferLength = Length in bytes of the input buffer
 *         OutputBuffer = Buffer into which data will be written by the driver
 *         OutputBufferLength = Length in bytes of the output buffer
 *         InternalDeviceIoControl = Determines weather
 *                                   IRP_MJ_INTERNAL_DEVICE_CONTROL or
 *                                   IRP_MJ_DEVICE_CONTROL will be used
 *         Event = Event used to notify the caller of completion
 *         IoStatusBlock (OUT) = Storage for the result of the operation
 * RETURNS: The IRP allocated on success, or
 *          NULL on failure
 */
{
   PIRP Irp;
   PIO_STACK_LOCATION StackPtr;
   ULONG BufferLength;

   DPRINT("IoBuildDeviceIoRequest(IoControlCode %x, DeviceObject %x, "
	  "InputBuffer %x, InputBufferLength %x, OutputBuffer %x, "
	  "OutputBufferLength %x, InternalDeviceIoControl %x "
	  "Event %x, IoStatusBlock %x\n",IoControlCode,DeviceObject,
	  InputBuffer,InputBufferLength,OutputBuffer,OutputBufferLength,
	  InternalDeviceIoControl,Event,IoStatusBlock);
   
   Irp = IoAllocateIrp(DeviceObject->StackSize,TRUE);
   if (Irp==NULL)
     {
	return(NULL);
     }
   
   Irp->UserEvent = Event;
   Irp->UserIosb = IoStatusBlock;
   DPRINT("Irp->UserIosb %x\n", Irp->UserIosb);
   Irp->Tail.Overlay.Thread = PsGetCurrentThread();

   StackPtr = IoGetNextIrpStackLocation(Irp);
   StackPtr->MajorFunction = InternalDeviceIoControl ? 
     IRP_MJ_INTERNAL_DEVICE_CONTROL : IRP_MJ_DEVICE_CONTROL;
   StackPtr->MinorFunction = 0;
   StackPtr->Flags = 0;
   StackPtr->Control = 0;
   StackPtr->DeviceObject = DeviceObject;
   StackPtr->FileObject = NULL;
   StackPtr->CompletionRoutine = NULL;
   StackPtr->Parameters.DeviceIoControl.IoControlCode = IoControlCode;
   StackPtr->Parameters.DeviceIoControl.InputBufferLength = InputBufferLength;
   StackPtr->Parameters.DeviceIoControl.OutputBufferLength = 
     OutputBufferLength;

   switch (IO_METHOD_FROM_CTL_CODE(IoControlCode))
     {
      case METHOD_BUFFERED:
	DPRINT("Using METHOD_BUFFERED!\n");
      
	if (InputBufferLength > OutputBufferLength)
	  {
	     BufferLength = InputBufferLength;
	  }
	else
	  {
	     BufferLength = OutputBufferLength;
	  }
	if (BufferLength)
	  {
	     Irp->AssociatedIrp.SystemBuffer = (PVOID)
	       ExAllocatePoolWithTag(NonPagedPool,BufferLength, TAG_SYS_BUF);
	    
	     if (Irp->AssociatedIrp.SystemBuffer == NULL)
	       {
		  IoFreeIrp(Irp);
		  return(NULL);
	       }
	  }
      
	if (InputBuffer && InputBufferLength)
	  {
	     RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer,
			   InputBuffer,
			   InputBufferLength);
	     RtlZeroMemory(Irp->AssociatedIrp.SystemBuffer + InputBufferLength,
			   BufferLength - InputBufferLength);
	  }
	else
	  {
	     RtlZeroMemory(Irp->AssociatedIrp.SystemBuffer,
			   BufferLength);
	  }
	Irp->UserBuffer = OutputBuffer;
	break;
	
      case METHOD_IN_DIRECT:
	DPRINT("Using METHOD_IN_DIRECT!\n");
	
	/* build output buffer (control buffer) */
	if (OutputBuffer && OutputBufferLength)
	  {
	     Irp->AssociatedIrp.SystemBuffer = (PVOID)
               ExAllocatePoolWithTag(NonPagedPool,OutputBufferLength, 
				     TAG_SYS_BUF);

	     if (Irp->AssociatedIrp.SystemBuffer == NULL)
	       {
		  IoFreeIrp(Irp);
		  return(NULL);
	       }

	     RtlZeroMemory(Irp->AssociatedIrp.SystemBuffer, OutputBufferLength);
	     Irp->UserBuffer = OutputBuffer;
	  }
	
         /* build input buffer (data transfer buffer) */
	if (InputBuffer && InputBufferLength)
	  {
	     Irp->MdlAddress = IoAllocateMdl(InputBuffer,
					     InputBufferLength,
					     FALSE,
					     FALSE,
					     Irp);
	     if(Irp->MdlAddress == NULL) {
		IoFreeIrp(Irp);
		return(NULL);
	     }
	     MmProbeAndLockPages (Irp->MdlAddress,UserMode,IoReadAccess);
	  }
	break;
	
      case METHOD_OUT_DIRECT:
	DPRINT("Using METHOD_OUT_DIRECT!\n");
	
	/* build input buffer (control buffer) */
	if (InputBuffer && InputBufferLength)
	  {
            Irp->AssociatedIrp.SystemBuffer = (PVOID)
               ExAllocatePoolWithTag(NonPagedPool,InputBufferLength, 
				     TAG_SYS_BUF);
	     
	     if (Irp->AssociatedIrp.SystemBuffer==NULL)
	       {
		  IoFreeIrp(Irp);
		  return(NULL);
	       }
	     
            RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer,
			  InputBuffer,
			  InputBufferLength);
	  }
	
	/* build output buffer (data transfer buffer) */
         if (OutputBuffer && OutputBufferLength)
	  {
	     Irp->MdlAddress = IoAllocateMdl(OutputBuffer,
					     OutputBufferLength,
					     FALSE,
					     FALSE,
					    Irp);
	     MmProbeAndLockPages(Irp->MdlAddress,UserMode,IoWriteAccess);
	  }
	break;
	
      case METHOD_NEITHER:
	DPRINT("Using METHOD_NEITHER!\n");
	
	Irp->UserBuffer = OutputBuffer;
	StackPtr->Parameters.DeviceIoControl.Type3InputBuffer = InputBuffer;
	break;
     }

   /* synchronous irp's are queued to requestor thread's irp cancel/cleanup list */
   IoQueueThreadIrp(Irp);
   return(Irp);
}


/*
 * @implemented
 */
PIRP STDCALL
IoBuildSynchronousFsdRequest(ULONG MajorFunction,
				  PDEVICE_OBJECT DeviceObject,
				  PVOID Buffer,
				  ULONG Length,
				  PLARGE_INTEGER StartingOffset,
				  PKEVENT Event,
				  PIO_STATUS_BLOCK IoStatusBlock)
/*
 * FUNCTION: Allocates and builds an IRP to be sent synchronously to lower
 * level driver(s)
 * ARGUMENTS:
 *        MajorFunction = Major function code, one of IRP_MJ_READ,
 *                        IRP_MJ_WRITE, IRP_MJ_FLUSH_BUFFERS, IRP_MJ_SHUTDOWN
 *        DeviceObject = Target device object
 *        Buffer = Buffer containing data for a read or write
 *        Length = Length in bytes of the information to be transferred
 *        StartingOffset = Offset to begin the read/write from
 *        Event (OUT) = Will be set when the operation is complete
 *        IoStatusBlock (OUT) = Set to the status of the operation
 * RETURNS: The IRP allocated on success, or
 *          NULL on failure
 */
{
   PIRP Irp;
   
   DPRINT("IoBuildSynchronousFsdRequest(MajorFunction %x, DeviceObject %x, "
	  "Buffer %x, Length %x, StartingOffset %x, Event %x, "
	  "IoStatusBlock %x\n",MajorFunction,DeviceObject,Buffer,Length,
	  StartingOffset,Event,IoStatusBlock);
   
   Irp = IoBuildAsynchronousFsdRequest(MajorFunction,
                                       DeviceObject,
                                       Buffer,
                                       Length,
                                       StartingOffset,
                                       IoStatusBlock );
   if (Irp==NULL)
     {
	return(NULL);
     }
   
   Irp->UserEvent = Event;

   /* synchronous irp's are queued to requestor thread's irp cancel/cleanup list */
   IoQueueThreadIrp(Irp);
   return(Irp);
}


PIRP 
IoBuildSynchronousFsdRequestWithMdl(ULONG MajorFunction,
				    PDEVICE_OBJECT DeviceObject,
				    PMDL Mdl,
				    PLARGE_INTEGER StartingOffset,
				    PKEVENT Event,
				    PIO_STATUS_BLOCK IoStatusBlock,
				    BOOLEAN PagingIo)
/*
 * FUNCTION: Allocates and builds an IRP to be sent synchronously to lower
 * level driver(s)
 * ARGUMENTS:
 *        MajorFunction = Major function code, one of IRP_MJ_READ,
 *                        IRP_MJ_WRITE, IRP_MJ_FLUSH_BUFFERS, IRP_MJ_SHUTDOWN
 *        DeviceObject = Target device object
 *        Buffer = Buffer containing data for a read or write
 *        Length = Length in bytes of the information to be transferred
 *        StartingOffset = Offset to begin the read/write from
 *        Event (OUT) = Will be set when the operation is complete
 *        IoStatusBlock (OUT) = Set to the status of the operation
 * RETURNS: The IRP allocated on success, or
 *          NULL on failure
 */
{
   PIRP Irp;
   PIO_STACK_LOCATION StackPtr;
   
   DPRINT("IoBuildSynchronousFsdRequestWithMdl(MajorFunction %x, "
	  "DeviceObject %x, "
	  "Mdl %x, StartingOffset %x, Event %x, "
	  "IoStatusBlock %x\n",MajorFunction,DeviceObject,Mdl,
	  StartingOffset,Event,IoStatusBlock);
   
   Irp = IoAllocateIrp(DeviceObject->StackSize,TRUE);
   if (Irp==NULL)
     {
	return(NULL);
     }
   
   Irp->UserEvent = Event;
   Irp->UserIosb = IoStatusBlock;
   DPRINT("Irp->UserIosb %x\n", Irp->UserIosb);
   Irp->Tail.Overlay.Thread = PsGetCurrentThread();
   if (PagingIo)
     {
       Irp->Flags = IRP_PAGING_IO;
     }
   else
     {
       Irp->Flags = 0;
     }
   
   StackPtr = IoGetNextIrpStackLocation(Irp);
   StackPtr->MajorFunction = (UCHAR)MajorFunction;
   StackPtr->MinorFunction = 0;
   StackPtr->Flags = 0;
   StackPtr->Control = 0;
   StackPtr->DeviceObject = DeviceObject;
   StackPtr->FileObject = NULL;
   StackPtr->CompletionRoutine = NULL;
   
   Irp->MdlAddress = Mdl;
   Irp->UserBuffer = NULL;
   Irp->AssociatedIrp.SystemBuffer = NULL;
      
   if (MajorFunction == IRP_MJ_READ)
     {
       if (StartingOffset != NULL)
	 {
	    StackPtr->Parameters.Read.ByteOffset = *StartingOffset;
	 }
       else
	 {
            StackPtr->Parameters.Read.ByteOffset.QuadPart = 0;
	 }
	StackPtr->Parameters.Read.Length = MmGetMdlByteCount(Mdl);
     }
   else
     {
       if (StartingOffset!=NULL)
	 {
	    StackPtr->Parameters.Write.ByteOffset = *StartingOffset;
	 }
       else
	 {
            StackPtr->Parameters.Write.ByteOffset.QuadPart = 0;
	 }
	StackPtr->Parameters.Write.Length = MmGetMdlByteCount(Mdl);
     }

   return(Irp);
}

/* EOF */
