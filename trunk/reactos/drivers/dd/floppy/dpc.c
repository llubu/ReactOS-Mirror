/****************************************************************************
 * Defered procedure calls for floppy disk driver, reactos project, created *
 * by Phillip Susi on 2/25/2001.  This software is published under the GNU  *
 * general public license, see the README file for more details             *
 ***************************************************************************/

#include <ddk/ntddk.h>
#define NDEBUG
#include <debug.h>
#include "floppy.h"


VOID STDCALL
FloppyDpc (PKDPC Dpc,
	   PDEVICE_OBJECT DeviceObject,
	   PIRP Irp,
	   PVOID Context)
{
  PCONTROLLER_OBJECT Controller;
  PFLOPPY_CONTROLLER_EXTENSION ControllerExtension;

  Controller = (PCONTROLLER_OBJECT)Context;
  ControllerExtension = (PFLOPPY_CONTROLLER_EXTENSION)Controller->ControllerExtension;

  ControllerExtension->DpcState (Dpc,
				 DeviceObject,
				 Irp,
				 Context);
}


VOID STDCALL
FloppyDpcDetect (PKDPC Dpc,
		 PDEVICE_OBJECT DeviceObject,
		 PIRP Irp,
		 PVOID Context)
{
  PCONTROLLER_OBJECT Controller;
  PFLOPPY_CONTROLLER_EXTENSION ControllerExtension;

  Controller = (PCONTROLLER_OBJECT)Context;
  ControllerExtension = (PFLOPPY_CONTROLLER_EXTENSION)Controller->ControllerExtension;

  KeSetEvent (&ControllerExtension->Event,
	      0,
	      FALSE);
}


VOID STDCALL
FloppyDpcFailIrp (PKDPC Dpc,
		  PDEVICE_OBJECT DeviceObject,
		  PIRP Irp,
		  PVOID Context)
{
  Irp->IoStatus.Status = STATUS_DEVICE_NOT_READY;
  Irp->IoStatus.Information = 0;
  CHECKPOINT;
  IoCompleteRequest (Irp,
		     IO_NO_INCREMENT);
}


VOID STDCALL
FloppyMotorSpindownDpc (PKDPC Dpc,
			PVOID Context,
			PVOID Arg1,
			PVOID Arg2)
{
  PCONTROLLER_OBJECT Controller;
  PFLOPPY_CONTROLLER_EXTENSION ControllerExtension;

  Controller = (PCONTROLLER_OBJECT)Context;
  ControllerExtension = (PFLOPPY_CONTROLLER_EXTENSION)Controller->ControllerExtension;

  /* queue call to turn off motor */
  IoAllocateController (Controller,
			ControllerExtension->Device,
			FloppyExecuteSpindown,
			ControllerExtension);
}


VOID STDCALL
FloppyMotorSpinupDpc (PKDPC Dpc,
		      PVOID Context,
		      PVOID Arg1,
		      PVOID Arg2)
{
  PCONTROLLER_OBJECT Controller;
  PFLOPPY_CONTROLLER_EXTENSION ControllerExtension;
  PFLOPPY_DEVICE_EXTENSION DeviceExtension;
  LARGE_INTEGER Timeout;

  Controller = (PCONTROLLER_OBJECT)Context;
  ControllerExtension = (PFLOPPY_CONTROLLER_EXTENSION)Controller->ControllerExtension;
  DeviceExtension = (PFLOPPY_DEVICE_EXTENSION)ControllerExtension->Device->DeviceExtension;
  Timeout.QuadPart = FLOPPY_MOTOR_SPINDOWN_TIME;

  // Motor has had time to spin up, mark motor as spun up and restart IRP
  // don't forget to set the spindown timer
  KeSetTimer (&ControllerExtension->SpinupTimer,
	      Timeout,
	      &ControllerExtension->MotorSpindownDpc);
  DPRINT ("Motor spun up, retrying operation\n");

  ControllerExtension->MotorOn = DeviceExtension->DriveSelect;

  IoFreeController (Controller);
  IoAllocateController (Controller,
			ControllerExtension->Device,
			FloppyExecuteReadWrite,
			ControllerExtension->Irp);
}


VOID STDCALL
FloppySeekDpc (PKDPC Dpc,
	       PDEVICE_OBJECT DeviceObject,
	       PIRP Irp,
	       PVOID Context)
{
  PFLOPPY_DEVICE_EXTENSION DeviceExtension;
  PFLOPPY_CONTROLLER_EXTENSION ControllerExtension;

  DeviceExtension = (PFLOPPY_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
  ControllerExtension = (PFLOPPY_CONTROLLER_EXTENSION)DeviceExtension->Controller->ControllerExtension;

  /* If the seek failed, fail the IRP */
  if (ControllerExtension->St0 & FLOPPY_ST0_GDMASK)
    {
      ControllerExtension->Irp->IoStatus.Status = STATUS_DISK_CORRUPT_ERROR;
      ControllerExtension->Irp->IoStatus.Information = 0;
      DPRINT ("Failing IRP: St0 = %2x, St1 = %2x, St2 = %2x\n",
	      ControllerExtension->St0,
	      ControllerExtension->St1,
	      ControllerExtension->St2);
      for (;;);
      IoCompleteRequest (ControllerExtension->Irp,
			 IO_NO_INCREMENT);
      IoFreeController (DeviceExtension->Controller);
      return;
    }

  KeStallExecutionProcessor (10000);

  DPRINT ("Seek completed, now on cyl %2x\n", DeviceExtension->Cyl);

  /* Now that we are on the right cyl, restart the read */
  if (FloppyExecuteReadWrite (DeviceObject,
			      ControllerExtension->Irp,
			      ControllerExtension->MapRegisterBase,
			      ControllerExtension->Irp) == DeallocateObject)
    {
      IoFreeController (DeviceExtension->Controller);
    }
}


VOID STDCALL
FloppyDpcReadWrite (PKDPC Dpc,
		    PDEVICE_OBJECT DeviceObject,
		    PIRP Irp,
		    PVOID Context)
{
  PCONTROLLER_OBJECT Controller;
  PFLOPPY_CONTROLLER_EXTENSION ControllerExtension;
  PFLOPPY_DEVICE_EXTENSION DeviceExtension;
  ULONG SectorSize;
  PIO_STACK_LOCATION Stack;
  BOOLEAN WriteToDevice;


  Controller = (PCONTROLLER_OBJECT)Context;
  ControllerExtension = (PFLOPPY_CONTROLLER_EXTENSION)Controller->ControllerExtension;
  DeviceExtension = (PFLOPPY_DEVICE_EXTENSION)ControllerExtension->Device->DeviceExtension;

  Irp = ControllerExtension->Irp;

  Stack = IoGetCurrentIrpStackLocation (Irp);

  SectorSize = 128 << ControllerExtension->SectorSizeCode;

  WriteToDevice = Stack->MajorFunction == IRP_MJ_WRITE ? TRUE : FALSE;

  /* If the IO failed, fail the IRP */
  if (ControllerExtension->St0 & FLOPPY_ST0_GDMASK)
    {
      Irp->IoStatus.Status = STATUS_DISK_CORRUPT_ERROR;
      Irp->IoStatus.Information = 0;
      DPRINT( "Failing IRP: St0 = %2x, St1 = %2x, St2 = %2x\n",
	      ControllerExtension->St0,
	      ControllerExtension->St1,
	      ControllerExtension->St2 );
      for(;;);
      IoCompleteRequest (Irp,
			 IO_NO_INCREMENT);
      IoFreeController (Controller);
      return;
    }

  /* Don't forget to flush the buffers */
  IoFlushAdapterBuffers( ControllerExtension->AdapterObject,
			 ControllerExtension->Irp->MdlAddress,
			 ControllerExtension->MapRegisterBase,
			 ControllerExtension->Irp->Tail.Overlay.DriverContext[0],
			 ControllerExtension->TransferLength,
			 WriteToDevice);
  DPRINT ("St0 = %2x  St1 = %2x  St2 = %2x\n",
	  ControllerExtension->St0,
	  ControllerExtension->St1,
	  ControllerExtension->St2);

  /* Update buffer info */
  Stack->Parameters.Read.ByteOffset.u.LowPart += ControllerExtension->TransferLength;
  Stack->Parameters.Read.Length -= ControllerExtension->TransferLength;

  /* drivercontext used for current va */
  (ULONG)ControllerExtension->Irp->Tail.Overlay.DriverContext[0] += ControllerExtension->TransferLength;
  DPRINT ("First ulong: %x\n", *((PULONG)ControllerExtension->MapRegisterBase))

  /* If there is more IO to be done, restart execute routine to issue next read */
  if (Stack->Parameters.Read.Length != 0)
    {
      if (FloppyExecuteReadWrite (DeviceObject,
				  Irp,
				  ControllerExtension->MapRegisterBase,
				  Irp ) == DeallocateObject)
	{
	  IoFreeController (Controller);
	}
    }
  else
    {
      /* Otherwise, complete the IRP */
      IoCompleteRequest (Irp,
			 IO_NO_INCREMENT);
      IoFreeController (Controller);
    }
}


VOID STDCALL
FloppyDpcDetectMedia (PKDPC Dpc,
		      PDEVICE_OBJECT DeviceObject,
		      PIRP Irp,
		      PVOID Context)
{
  PCONTROLLER_OBJECT Controller;
  PFLOPPY_CONTROLLER_EXTENSION ControllerExtension;

  Controller = (PCONTROLLER_OBJECT)Context;
  ControllerExtension = (PFLOPPY_CONTROLLER_EXTENSION)Controller->ControllerExtension;

  /* If the read ID failed, fail the IRP */
  if (ControllerExtension->St1 != 0)
    {
      DPRINT1 ("Read ID failed: ST1 = %2x\n", ControllerExtension->St1);
      IoFreeController (Controller);
      Irp->IoStatus.Information = 0;
      Irp->IoStatus.Status = STATUS_DEVICE_NOT_READY;
      IoCompleteRequest (Irp,
			 IO_NO_INCREMENT);
      return;
    }

  /* Set media type, and restart the IRP from the beginning */
  ((PFLOPPY_DEVICE_EXTENSION)ControllerExtension->Device->DeviceExtension)->MediaType = 0;
  DPRINT ("Media detected, restarting IRP\n");

  /* Don't forget to free the controller so that the now queued routine may execute */
  IoFreeController (Controller);

  IoAllocateController (Controller,
			DeviceObject,
			FloppyExecuteReadWrite,
			Irp);
}
