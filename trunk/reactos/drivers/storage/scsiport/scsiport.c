/*
 *  ReactOS kernel
 *  Copyright (C) 2001, 2002 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: scsiport.c,v 1.5 2002/02/03 20:21:59 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            services/storage/scsiport/scsiport.c
 * PURPOSE:         SCSI port driver
 * PROGRAMMER:      Eric Kohl (ekohl@rz-online.de)
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include "../include/srb.h"
#include "../include/scsi.h"
#include "../include/ntddscsi.h"

#define NDEBUG
#include <debug.h>


#define VERSION "0.0.1"


/* TYPES *********************************************************************/

/*
 * SCSI_PORT_DEVICE_EXTENSION
 *
 * DESCRIPTION
 *	First part of the port objects device extension. The second
 *	part is the miniport-specific device extension.
 */

typedef struct _SCSI_PORT_DEVICE_EXTENSION
{
  ULONG Length;
  ULONG MiniPortExtensionSize;
  PORT_CONFIGURATION_INFORMATION PortConfig;
  
  ULONG PortBusInfoSize;
  PSCSI_ADAPTER_BUS_INFO PortBusInfo;
  
  PDEVICE_OBJECT DeviceObject;
  PCONTROLLER_OBJECT ControllerObject;
  
  PHW_STARTIO HwStartIo;
  PHW_INTERRUPT HwInterrupt;
  
  UCHAR MiniPortDeviceExtension[1]; /* must be the last entry */
} SCSI_PORT_DEVICE_EXTENSION, *PSCSI_PORT_DEVICE_EXTENSION;


/*
 * SCSI_PORT_TIMER_STATES
 *
 * DESCRIPTION
 *	An enumeration containing the states in the timer DFA
 */

typedef enum _SCSI_PORT_TIMER_STATES
{
  IDETimerIdle,
  IDETimerCmdWait,
  IDETimerResetWaitForBusyNegate,
  IDETimerResetWaitForDrdyAssert
} SCSI_PORT_TIMER_STATES;


/*
 * SCSI_PORT_CONTROLLER_EXTENSION
 *
 * DESCRIPTION
 *	Driver-defined structure used to hold miscellaneous controller information.
 *
 * ACCESS
 *	Allocated from NON-PAGED POOL
 *	Available at any IRQL
 */

typedef struct _SCSI_PORT_CONTROLLER_EXTENSION
{
  KSPIN_LOCK             SpinLock;
  ULONG                  Number;
  ULONG                  Vector;
  BOOLEAN                DMASupported;
  BOOLEAN                ControllerInterruptBug;
  PKINTERRUPT            Interrupt;

  BOOLEAN                OperationInProgress;
  UCHAR                  DeviceStatus;
  PSCSI_PORT_DEVICE_EXTENSION DeviceForOperation;
  PIRP                   CurrentIrp;
  ULONG                  Retries;

  SCSI_PORT_TIMER_STATES TimerState;
  LONG                   TimerCount;

  PDEVICE_OBJECT         PortDeviceObject;
  PCONTROLLER_OBJECT ControllerObject;

} SCSI_PORT_CONTROLLER_EXTENSION, *PSCSI_PORT_CONTROLLER_EXTENSION;


/* GLOBALS *******************************************************************/


static NTSTATUS STDCALL
ScsiPortCreateClose(IN PDEVICE_OBJECT DeviceObject,
		    IN PIRP Irp);

static NTSTATUS STDCALL
ScsiPortDispatchScsi(IN PDEVICE_OBJECT DeviceObject,
		     IN PIRP Irp);

static NTSTATUS STDCALL
ScsiPortDeviceControl(IN PDEVICE_OBJECT DeviceObject,
		      IN PIRP Irp);

static NTSTATUS STDCALL
ScsiPortReadWrite(IN PDEVICE_OBJECT DeviceObject,
		  IN PIRP Irp);

static VOID STDCALL
ScsiPortStartIo(IN PDEVICE_OBJECT DeviceObject,
		IN PIRP Irp);

static IO_ALLOCATION_ACTION STDCALL
ScsiPortAllocateController(IN PDEVICE_OBJECT DeviceObject,
			   IN PIRP Irp,
			   IN PVOID MapRegisterBase,
			   IN PVOID Context);

static BOOLEAN STDCALL
ScsiPortStartController(IN OUT PVOID Context);

static NTSTATUS
ScsiPortCreatePortDevice(IN PDRIVER_OBJECT DriverObject,
			 IN PSCSI_PORT_DEVICE_EXTENSION PseudoDeviceExtension,
			 IN ULONG PortCount);

static BOOLEAN STDCALL
ScsiPortIsr(IN PKINTERRUPT Interrupt,
	    IN PVOID ServiceContext);

static VOID STDCALL
ScsiPortDpcForIsr(IN PKDPC Dpc,
		  IN PDEVICE_OBJECT DpcDeviceObject,
		  IN PIRP DpcIrp,
		  IN PVOID DpcContext);

static VOID STDCALL
ScsiPortIoTimer(PDEVICE_OBJECT DeviceObject,
		PVOID Context);

static VOID
ScsiPortFinishOperation(PSCSI_PORT_CONTROLLER_EXTENSION ControllerExtension);


/* FUNCTIONS *****************************************************************/

/**********************************************************************
 * NAME							EXPORTED
 *	DriverEntry
 *
 * DESCRIPTION
 *	This function initializes the driver.
 *
 * RUN LEVEL
 *	PASSIVE_LEVEL
 *
 * ARGUMENTS
 *	DriverObject
 *		System allocated Driver Object for this driver.
 *
 *	RegistryPath
 *		Name of registry driver service key.
 *
 * RETURN VALUE
 * 	Status.
 */

NTSTATUS STDCALL
DriverEntry(IN PDRIVER_OBJECT DriverObject,
	    IN PUNICODE_STRING RegistryPath)
{
  DbgPrint("ScsiPort Driver %s\n", VERSION);
  return(STATUS_SUCCESS);
}


/**********************************************************************
 * NAME							EXPORTED
 *	ScsiDebugPrint
 *
 * DESCRIPTION
 *	Prints debugging messages.
 *
 * RUN LEVEL
 *	PASSIVE_LEVEL
 *
 * ARGUMENTS
 *	DebugPrintLevel
 *		Debug level of the given message.
 *
 *	DebugMessage
 *		Pointer to printf()-compatible format string.
 *
 *	...
  		Additional output data (see printf()).
 *
 * RETURN VALUE
 * 	None.
 */

VOID
ScsiDebugPrint(IN ULONG DebugPrintLevel,
	       IN PCHAR DebugMessage,
	       ...)
{
  char Buffer[256];
  va_list ap;

#if 0
  if (DebugPrintLevel > InternalDebugLevel)
    return;
#endif

  va_start(ap, DebugMessage);
  vsprintf(Buffer, DebugMessage, ap);
  va_end(ap);

  DbgPrint(Buffer);
}


VOID STDCALL
ScsiPortCompleteRequest(IN PVOID HwDeviceExtension,
			IN UCHAR PathId,
			IN UCHAR TargetId,
			IN UCHAR Lun,
			IN UCHAR SrbStatus)
{
  UNIMPLEMENTED;
}


ULONG STDCALL
ScsiPortConvertPhysicalAddressToUlong(IN SCSI_PHYSICAL_ADDRESS Address)
{
  return Address.u.LowPart;
}


VOID STDCALL
ScsiPortFlushDma(IN PVOID HwDeviceExtension)
{
  UNIMPLEMENTED;
}


VOID STDCALL
ScsiPortFreeDeviceBase(IN PVOID HwDeviceExtension,
		       IN PVOID MappedAddress)
{
  UNIMPLEMENTED;
}


ULONG STDCALL
ScsiPortGetBusData(IN PVOID DeviceExtension,
		   IN ULONG BusDataType,
		   IN ULONG SystemIoBusNumber,
		   IN ULONG SlotNumber,
		   IN PVOID Buffer,
		   IN ULONG Length)
{
  return(HalGetBusData(BusDataType,
		       SystemIoBusNumber,
		       SlotNumber,
		       Buffer,
		       Length));
}


PVOID STDCALL
ScsiPortGetDeviceBase(IN PVOID HwDeviceExtension,
		      IN INTERFACE_TYPE BusType,
		      IN ULONG SystemIoBusNumber,
		      IN SCSI_PHYSICAL_ADDRESS IoAddress,
		      IN ULONG NumberOfBytes,
		      IN BOOLEAN InIoSpace)
{
  ULONG AddressSpace;
  PHYSICAL_ADDRESS TranslatedAddress;
  PVOID VirtualAddress;
  PVOID Buffer;
  BOOLEAN rc;

  AddressSpace = (ULONG)InIoSpace;

  if (!HalTranslateBusAddress(BusType,
			      SystemIoBusNumber,
			      IoAddress,
			      &AddressSpace,
			      &TranslatedAddress))
    return NULL;

  /* i/o space */
  if (AddressSpace != 0)
    return (PVOID)TranslatedAddress.u.LowPart;

  VirtualAddress = MmMapIoSpace(TranslatedAddress,
				NumberOfBytes,
				FALSE);

  Buffer = ExAllocatePool(NonPagedPool,0x20);
  if (Buffer == NULL)
    return VirtualAddress;

  return NULL;  /* ?? */
}


PVOID STDCALL
ScsiPortGetLogicalUnit(IN PVOID HwDeviceExtension,
		       IN UCHAR PathId,
		       IN UCHAR TargetId,
		       IN UCHAR Lun)
{
  UNIMPLEMENTED;
}


SCSI_PHYSICAL_ADDRESS STDCALL
ScsiPortGetPhysicalAddress(IN PVOID HwDeviceExtension,
			   IN PSCSI_REQUEST_BLOCK Srb OPTIONAL,
			   IN PVOID VirtualAddress,
			   OUT ULONG *Length)
{
  UNIMPLEMENTED;
}


PSCSI_REQUEST_BLOCK STDCALL
ScsiPortGetSrb(IN PVOID DeviceExtension,
	       IN UCHAR PathId,
	       IN UCHAR TargetId,
	       IN UCHAR Lun,
	       IN LONG QueueTag)
{
  UNIMPLEMENTED;
}


PVOID STDCALL
ScsiPortGetUncachedExtension(IN PVOID HwDeviceExtension,
			     IN PPORT_CONFIGURATION_INFORMATION ConfigInfo,
			     IN ULONG NumberOfBytes)
{
  UNIMPLEMENTED;
}


PVOID STDCALL
ScsiPortGetVirtualAddress(IN PVOID HwDeviceExtension,
			  IN SCSI_PHYSICAL_ADDRESS PhysicalAddress)
{
  UNIMPLEMENTED;
}


/**********************************************************************
 * NAME							EXPORTED
 *	ScsiPortInitialize
 *
 * DESCRIPTION
 *	Initializes SCSI port driver specific data.
 *
 * RUN LEVEL
 *	PASSIVE_LEVEL
 *
 * ARGUMENTS
 *	Argument1
 *		Pointer to the miniport driver's driver object.
 *
 *	Argument2
 *		Pointer to the miniport driver's registry path.
 *
 *	HwInitializationData
 *		Pointer to port driver specific configuration data.
 *
 *	HwContext
  		Miniport driver specific context.
 *
 * RETURN VALUE
 * 	Status.
 */

ULONG STDCALL
ScsiPortInitialize(IN PVOID Argument1,
		   IN PVOID Argument2,
		   IN struct _HW_INITIALIZATION_DATA *HwInitializationData,
		   IN PVOID HwContext)
{
  PDRIVER_OBJECT DriverObject = (PDRIVER_OBJECT)Argument1;
  PUNICODE_STRING RegistryPath = (PUNICODE_STRING)Argument2;
  PSCSI_PORT_DEVICE_EXTENSION PseudoDeviceExtension;
  PCONFIGURATION_INFORMATION SystemConfig;
  PPORT_CONFIGURATION_INFORMATION PortConfig;
  BOOLEAN Again;
  ULONG i;
  ULONG Result;
  NTSTATUS Status;
  ULONG MaxBus;
  PACCESS_RANGE AccessRanges;
  ULONG ExtensionSize;

  DPRINT1("ScsiPortInitialize() called!\n");

  if ((HwInitializationData->HwInitialize == NULL) ||
      (HwInitializationData->HwStartIo == NULL) ||
      (HwInitializationData->HwInterrupt == NULL) ||
      (HwInitializationData->HwFindAdapter == NULL) ||
      (HwInitializationData->HwResetBus == NULL))
    return(STATUS_INVALID_PARAMETER);

  DriverObject->DriverStartIo = ScsiPortStartIo;
  DriverObject->MajorFunction[IRP_MJ_CREATE] = ScsiPortCreateClose;
  DriverObject->MajorFunction[IRP_MJ_CLOSE] = ScsiPortCreateClose;
  DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ScsiPortDeviceControl;
  DriverObject->MajorFunction[IRP_MJ_SCSI] = ScsiPortDispatchScsi;
//  DriverObject->MajorFunction[IRP_MJ_READ] = ScsiPortReadWrite;
//  DriverObject->MajorFunction[IRP_MJ_WRITE] = ScsiPortReadWrite;

//  DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION] = IDEDispatchQueryInformation;
//  DriverObject->MajorFunction[IRP_MJ_SET_INFORMATION] = IDEDispatchSetInformation;




  SystemConfig = IoGetConfigurationInformation();

  ExtensionSize = sizeof(SCSI_PORT_DEVICE_EXTENSION) +
		  HwInitializationData->DeviceExtensionSize;
  PseudoDeviceExtension = ExAllocatePool(PagedPool,
					 ExtensionSize);
  RtlZeroMemory(PseudoDeviceExtension,
		ExtensionSize);
  PseudoDeviceExtension->Length = ExtensionSize;
  PseudoDeviceExtension->MiniPortExtensionSize = HwInitializationData->DeviceExtensionSize;
  PseudoDeviceExtension->HwStartIo = HwInitializationData->HwStartIo;
  PseudoDeviceExtension->HwInterrupt = HwInitializationData->HwInterrupt;

  PortConfig = &PseudoDeviceExtension->PortConfig;

  PortConfig->Length = sizeof(PORT_CONFIGURATION_INFORMATION);
  PortConfig->AdapterInterfaceType = HwInitializationData->AdapterInterfaceType;
  PortConfig->InterruptMode =
   (PortConfig->AdapterInterfaceType == PCIBus) ? LevelSensitive : Latched;
  PortConfig->AtdiskPrimaryClaimed = SystemConfig->AtDiskPrimaryAddressClaimed;
  PortConfig->AtdiskSecondaryClaimed = SystemConfig->AtDiskSecondaryAddressClaimed;
  PortConfig->NumberOfAccessRanges = HwInitializationData->NumberOfAccessRanges;

  PortConfig->AccessRanges =
    ExAllocatePool(PagedPool,
		   sizeof(ACCESS_RANGE) * PortConfig->NumberOfAccessRanges);


  PortConfig->SystemIoBusNumber = 0;
  PortConfig->SlotNumber = 0;

  MaxBus = (PortConfig->AdapterInterfaceType == PCIBus) ? 8 : 1;

  DPRINT("MaxBus: %lu\n", MaxBus);

  while (TRUE)
    {
      DPRINT("Calling HwFindAdapter() for Bus %lu\n", PortConfig->SystemIoBusNumber);

//      RtlZeroMemory(AccessRanges,
//		    sizeof(ACCESS_RANGE) * PortConfig->NumberOfAccessRanges);

      /* Note: HwFindAdapter is called once for each bus */
      Result = (HwInitializationData->HwFindAdapter)(&PseudoDeviceExtension->MiniPortDeviceExtension,
						     HwContext,
						     NULL,	/* BusInformation */
						     NULL,	/* ArgumentString */
						     &PseudoDeviceExtension->PortConfig,
						     &Again);
      DPRINT("HwFindAdapter() result: %lu\n", Result);

      if (Result == SP_RETURN_FOUND)
	{
	  DPRINT("ScsiPortInitialize(): Found HBA!\n");

	  Status = ScsiPortCreatePortDevice(DriverObject,
					    PseudoDeviceExtension,
					    SystemConfig->ScsiPortCount);

	  if (!NT_SUCCESS(Status))
	    {
	      DbgPrint("ScsiPortCreatePortDevice() failed! (Status 0x%lX)\n", Status);

	      ExFreePool(PortConfig->AccessRanges);
	      ExFreePool(PseudoDeviceExtension);

	      return(Status);
	    }

	  /* Update the configuration info */
	  SystemConfig->AtDiskPrimaryAddressClaimed = PortConfig->AtdiskPrimaryClaimed;
	  SystemConfig->AtDiskSecondaryAddressClaimed = PortConfig->AtdiskSecondaryClaimed;
	  SystemConfig->ScsiPortCount++;
	}

      if (Again == FALSE)
	{
	  PortConfig->SystemIoBusNumber++;
	  PortConfig->SlotNumber = 0;
	}

      DPRINT("Bus: %lu  MaxBus: %lu\n", PortConfig->SystemIoBusNumber, MaxBus);
      if (PortConfig->SystemIoBusNumber >= MaxBus)
	{
	  DPRINT1("Scanned all buses!\n");
	  break;
	}
    }

  ExFreePool(PortConfig->AccessRanges);
  ExFreePool(PseudoDeviceExtension);

  DPRINT1("ScsiPortInitialize() done!\n");

  return(STATUS_SUCCESS);
}


VOID STDCALL
ScsiPortIoMapTransfer(IN PVOID HwDeviceExtension,
		      IN PSCSI_REQUEST_BLOCK Srb,
		      IN ULONG LogicalAddress,
		      IN ULONG Length)
{
  UNIMPLEMENTED;
}


VOID STDCALL
ScsiPortLogError(IN PVOID HwDeviceExtension,
		 IN PSCSI_REQUEST_BLOCK Srb OPTIONAL,
		 IN UCHAR PathId,
		 IN UCHAR TargetId,
		 IN UCHAR Lun,
		 IN ULONG ErrorCode,
		 IN ULONG UniqueId)
{
  UNIMPLEMENTED;
}


VOID STDCALL
ScsiPortMoveMemory(OUT PVOID Destination,
		   IN PVOID Source,
		   IN ULONG Length)
{
  RtlMoveMemory(Destination,
		Source,
		Length);
}


VOID
ScsiPortNotification(IN SCSI_NOTIFICATION_TYPE NotificationType,
		     IN PVOID HwDeviceExtension,
		     ...)
{
  PSCSI_PORT_DEVICE_EXTENSION DevExt;
  PSCSI_PORT_CONTROLLER_EXTENSION ControllerExt;

  DPRINT1("ScsiPortNotification() called\n");

  DevExt = (PSCSI_PORT_DEVICE_EXTENSION)((PUCHAR)HwDeviceExtension - sizeof(SCSI_PORT_DEVICE_EXTENSION));
  ControllerExt = (PSCSI_PORT_CONTROLLER_EXTENSION)DevExt->ControllerObject->ControllerExtension;

  switch (NotificationType)
    {
      case RequestComplete:
	IoCompleteRequest(ControllerExt->CurrentIrp, IO_NO_INCREMENT);
	break;

      case NextRequest:
	IoStartNextPacket(DevExt->DeviceObject, FALSE);
	break;

      default:
	break;
    }
}


ULONG STDCALL
ScsiPortSetBusDataByOffset(IN PVOID DeviceExtension,
			   IN ULONG BusDataType,
			   IN ULONG SystemIoBusNumber,
			   IN ULONG SlotNumber,
			   IN PVOID Buffer,
			   IN ULONG Offset,
			   IN ULONG Length)
{
  return(HalSetBusDataByOffset(BusDataType,
			       SystemIoBusNumber,
			       SlotNumber,
			       Buffer,
			       Offset,
			       Length));
}


BOOLEAN STDCALL
ScsiPortValidateRange(IN PVOID HwDeviceExtension,
		      IN INTERFACE_TYPE BusType,
		      IN ULONG SystemIoBusNumber,
		      IN SCSI_PHYSICAL_ADDRESS IoAddress,
		      IN ULONG NumberOfBytes,
		      IN BOOLEAN InIoSpace)
{
  return(TRUE);
}


/* INTERNAL FUNCTIONS ********************************************************/

/**********************************************************************
 * NAME							INTERNAL
 *	ScsiPortCreateClose
 *
 * DESCRIPTION
 *	Answer requests for Create/Close calls: a null operation.
 *
 * RUN LEVEL
 *	PASSIVE_LEVEL
 *
 * ARGUMENTS
 *	DeviceObject
 *		Pointer to a device object.
 *
 *	Irp
 *		Pointer to an IRP.
 *
 *	...
  		Additional output data (see printf()).
 *
 * RETURN VALUE
 * 	Status.
 */

static NTSTATUS STDCALL
ScsiPortCreateClose(IN PDEVICE_OBJECT DeviceObject,
		    IN PIRP Irp)
{
  DPRINT("ScsiPortCreateClose()\n");

  Irp->IoStatus.Status = STATUS_SUCCESS;
  Irp->IoStatus.Information = FILE_OPENED;

  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return(STATUS_SUCCESS);
}


/**********************************************************************
 * NAME							INTERNAL
 *	ScsiPortDispatchScsi
 *
 * DESCRIPTION
 *	Answer requests for SCSI calls
 *
 * RUN LEVEL
 *	PASSIVE_LEVEL
 *
 * ARGUMENTS
 *	Standard dispatch arguments
 *
 * RETURNS
 *	NTSTATUS
 */

static NTSTATUS STDCALL
ScsiPortDispatchScsi(IN PDEVICE_OBJECT DeviceObject,
		     IN PIRP Irp)
{
  PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;
  PIO_STACK_LOCATION Stack;
  PSCSI_REQUEST_BLOCK Srb;
  NTSTATUS Status = STATUS_SUCCESS;
  ULONG DataSize = 0;


  DPRINT("ScsiPortDispatchScsi()\n");

  DeviceExtension = DeviceObject->DeviceExtension;
  Stack = IoGetCurrentIrpStackLocation(Irp);

  switch(Stack->Parameters.DeviceIoControl.IoControlCode)
    {
      case IOCTL_SCSI_EXECUTE_IN:
	{
	  DPRINT("  IOCTL_SCSI_EXECUTE_IN\n");
	}
	break;

      case IOCTL_SCSI_EXECUTE_OUT:
	{
	  DPRINT("  IOCTL_SCSI_EXECUTE_OUT\n");
	}
	break;

      case IOCTL_SCSI_EXECUTE_NONE:
	{
	  DPRINT("  IOCTL_SCSI_EXECUTE_NONE\n");
	}
	break;
    }

  Srb = Stack->Parameters.Scsi.Srb;
  if (Srb == NULL)
    {

      Status = STATUS_UNSUCCESSFUL;


      Irp->IoStatus.Status = Status;
      Irp->IoStatus.Information = 0;

      IoCompleteRequest(Irp, IO_NO_INCREMENT);

      return(Status);
    }

  DPRINT("Srb: %p\n", Srb);
  DPRINT("Srb->Function: %lu\n", Srb->Function);

  switch (Srb->Function)
    {
      case SRB_FUNCTION_EXECUTE_SCSI:
	DPRINT1("  SRB_FUNCTION_EXECUTE_SCSI\n");

	switch (Srb->Cdb[0])
	  {
	    case SCSIOP_INQUIRY:
	    case SCSIOP_READ_CAPACITY:
	      Status = DeviceExtension->HwStartIo(&DeviceExtension->MiniPortDeviceExtension,
						  Srb);
	      break;


	    default:
	      IoStartPacket(DeviceObject, Irp, NULL, NULL);
	      DPRINT1("Returning STATUS_PENDING\n");
	      return(STATUS_PENDING);
	  }
	break;

      case SRB_FUNCTION_CLAIM_DEVICE:
	{
	  PSCSI_ADAPTER_BUS_INFO AdapterInfo;
	  PSCSI_INQUIRY_DATA UnitInfo;
	  PINQUIRYDATA InquiryData;

	  DPRINT("  SRB_FUNCTION_CLAIM_DEVICE\n");

	  if (DeviceExtension->PortBusInfo != NULL)
	    {
	      AdapterInfo = (PSCSI_ADAPTER_BUS_INFO)DeviceExtension->PortBusInfo;

	      UnitInfo = (PSCSI_INQUIRY_DATA)((PUCHAR)AdapterInfo +
		AdapterInfo->BusData[Srb->PathId].InquiryDataOffset);

	      while (AdapterInfo->BusData[Srb->PathId].InquiryDataOffset)
		{
		  InquiryData = (PINQUIRYDATA)UnitInfo->InquiryData;

		  if ((UnitInfo->TargetId == Srb->TargetId) &&
		      (UnitInfo->Lun == Srb->Lun) &&
		      (UnitInfo->DeviceClaimed == FALSE))
		    {
		      UnitInfo->DeviceClaimed = TRUE;
		      DPRINT("Claimed device!\n");
		      break;
		    }

		  if (UnitInfo->NextInquiryDataOffset == 0)
		    break;

		  UnitInfo = (PSCSI_INQUIRY_DATA)((PUCHAR)AdapterInfo + UnitInfo->NextInquiryDataOffset);
		}
	    }

	  /* FIXME: Hack!!!!! */
	  Srb->DataBuffer = DeviceObject;
	}
	break;

      case SRB_FUNCTION_RELEASE_DEVICE:
	{
	  PSCSI_ADAPTER_BUS_INFO AdapterInfo;
	  PSCSI_INQUIRY_DATA UnitInfo;
	  PINQUIRYDATA InquiryData;

	  DPRINT("  SRB_FUNCTION_RELEASE_DEVICE\n");

	  if (DeviceExtension->PortBusInfo != NULL)
	    {
	      AdapterInfo = (PSCSI_ADAPTER_BUS_INFO)DeviceExtension->PortBusInfo;

	      UnitInfo = (PSCSI_INQUIRY_DATA)((PUCHAR)AdapterInfo +
		AdapterInfo->BusData[Srb->PathId].InquiryDataOffset);


	      while (AdapterInfo->BusData[Srb->PathId].InquiryDataOffset)
		{
		  InquiryData = (PINQUIRYDATA)UnitInfo->InquiryData;

		  if ((UnitInfo->TargetId == Srb->TargetId) &&
		      (UnitInfo->Lun == Srb->Lun) &&
		      (UnitInfo->DeviceClaimed == TRUE))
		    {
		      UnitInfo->DeviceClaimed = FALSE;
		      DPRINT("Released device!\n");
		      break;
		    }

		  if (UnitInfo->NextInquiryDataOffset == 0)
		    break;

		  UnitInfo = (PSCSI_INQUIRY_DATA)((PUCHAR)AdapterInfo + UnitInfo->NextInquiryDataOffset);
		}
	    }
	}
	break;
    }

  Irp->IoStatus.Status = Status;
  Irp->IoStatus.Information = DataSize;

  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return(Status);
}


/**********************************************************************
 * NAME							INTERNAL
 *	ScsiPortDeviceControl
 *
 * DESCRIPTION
 *	Answer requests for device control calls
 *
 * RUN LEVEL
 *	PASSIVE_LEVEL
 *
 * ARGUMENTS
 *	Standard dispatch arguments
 *
 * RETURNS
 *	NTSTATUS
 */

static NTSTATUS STDCALL
ScsiPortDeviceControl(IN PDEVICE_OBJECT DeviceObject,
		      IN PIRP Irp)
{
  PIO_STACK_LOCATION Stack;
  PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;

  DPRINT("ScsiPortDeviceControl()\n");

  Irp->IoStatus.Status = STATUS_SUCCESS;
  Irp->IoStatus.Information = 0;


  Stack = IoGetCurrentIrpStackLocation(Irp);
  DeviceExtension = DeviceObject->DeviceExtension;

  switch (Stack->Parameters.DeviceIoControl.IoControlCode)
    {

      case IOCTL_SCSI_GET_CAPABILITIES:
	{
	  PIO_SCSI_CAPABILITIES Capabilities;

	  DPRINT("  IOCTL_SCSI_GET_CAPABILITIES\n");

	  Capabilities = (PIO_SCSI_CAPABILITIES)Irp->AssociatedIrp.SystemBuffer;
	  Capabilities->Length = sizeof(IO_SCSI_CAPABILITIES);
	  Capabilities->MaximumTransferLength =
	    DeviceExtension->PortConfig.MaximumTransferLength;
	  Capabilities->MaximumPhysicalPages = 1;
	  Capabilities->SupportedAsynchronousEvents = 0;
	  Capabilities->AlignmentMask =
	    DeviceExtension->PortConfig.AlignmentMask;
	  Capabilities->TaggedQueuing =
	    DeviceExtension->PortConfig.TaggedQueuing;
	  Capabilities->AdapterScansDown =
	    DeviceExtension->PortConfig.AdapterScansDown;
	  Capabilities->AdapterUsesPio = TRUE;

	  Irp->IoStatus.Information = sizeof(IO_SCSI_CAPABILITIES);
	}
	break;

      case IOCTL_SCSI_GET_INQUIRY_DATA:
	{
	  PSCSI_ADAPTER_BUS_INFO AdapterInfo;
	  PSCSI_INQUIRY_DATA UnitInfo, PrevUnit;
	  SCSI_REQUEST_BLOCK Srb;
	  ULONG Bus;
	  ULONG Target;
	  ULONG UnitCount;
	  ULONG DataSize;
	  BOOLEAN Result;

	  DPRINT("  IOCTL_SCSI_GET_INQUIRY_DATA\n");

	  /* Copy inquiry data to the port device extension */
	  if (DeviceExtension->PortBusInfo == NULL)
	    {
	      AdapterInfo =(PSCSI_ADAPTER_BUS_INFO)Irp->AssociatedIrp.SystemBuffer;
	      AdapterInfo->NumberOfBuses = DeviceExtension->PortConfig.NumberOfBuses;

	      UnitInfo = (PSCSI_INQUIRY_DATA)
		((PUCHAR)AdapterInfo + sizeof(SCSI_ADAPTER_BUS_INFO) +
		 (sizeof(SCSI_BUS_DATA) * (AdapterInfo->NumberOfBuses - 1)));

	      Srb.DataBuffer = ExAllocatePool(NonPagedPool, 256);
	      RtlZeroMemory(&Srb,
			    sizeof(SCSI_REQUEST_BLOCK));
	      Srb.Function = SRB_FUNCTION_EXECUTE_SCSI;
	      Srb.DataTransferLength = 256;
	      Srb.Cdb[0] = SCSIOP_INQUIRY;

	      for (Bus = 0; Bus < AdapterInfo->NumberOfBuses; Bus++)
		{
		  Srb.PathId = Bus;

		  AdapterInfo->BusData[Bus].InitiatorBusId = 0;	/* ? */
		  AdapterInfo->BusData[Bus].InquiryDataOffset =
		    (ULONG)((PUCHAR)UnitInfo - (PUCHAR)AdapterInfo);

		  PrevUnit = NULL;
		  UnitCount = 0;

		  for (Target = 0; Target < DeviceExtension->PortConfig.MaximumNumberOfTargets; Target++)
		    {
		      Srb.TargetId = Target;
		      Srb.Lun = 0;

		      Result = DeviceExtension->HwStartIo(&DeviceExtension->MiniPortDeviceExtension,
							  &Srb);
		      if (Result == TRUE)
			{
			  UnitInfo->PathId = Bus;
			  UnitInfo->TargetId = Target;
			  UnitInfo->Lun = 0;
			  UnitInfo->InquiryDataLength = INQUIRYDATABUFFERSIZE;
			  memcpy(&UnitInfo->InquiryData,
				 Srb.DataBuffer,
				 INQUIRYDATABUFFERSIZE);
			  if (PrevUnit != NULL)
			    PrevUnit->NextInquiryDataOffset = (ULONG)((PUCHAR)UnitInfo-(PUCHAR)AdapterInfo);
			  PrevUnit = UnitInfo;
			  UnitInfo = (PSCSI_INQUIRY_DATA)((PUCHAR)UnitInfo + sizeof(SCSI_INQUIRY_DATA)+INQUIRYDATABUFFERSIZE-1);
			  UnitCount++;
			}
		    }
		  AdapterInfo->BusData[Bus].NumberOfLogicalUnits = UnitCount;
		}
	      DataSize = (ULONG)((PUCHAR)UnitInfo-(PUCHAR)AdapterInfo);

	      ExFreePool(Srb.DataBuffer);

	      /* copy inquiry data to the port driver's device extension */
	      DeviceExtension->PortBusInfoSize = DataSize;
	      DeviceExtension->PortBusInfo = ExAllocatePool(NonPagedPool,
							    DataSize);
	      memcpy(DeviceExtension->PortBusInfo,
		     AdapterInfo,
		     DataSize);
	    }
	  else
	    {
	      memcpy(Irp->AssociatedIrp.SystemBuffer,
		     DeviceExtension->PortBusInfo,
		     DeviceExtension->PortBusInfoSize);
	    }

	  DPRINT("BufferSize: %lu\n", DeviceExtension->PortBusInfoSize);
	  Irp->IoStatus.Information = DeviceExtension->PortBusInfoSize;
	}
	break;

      default:
	DPRINT1("  unknown ioctl code: 0x%lX\n",
	       Stack->Parameters.DeviceIoControl.IoControlCode);
	break;
    }

  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return(STATUS_SUCCESS);
}


static NTSTATUS STDCALL
ScsiPortReadWrite(IN PDEVICE_OBJECT DeviceObject,
		  IN PIRP Irp)
{
  NTSTATUS Status;

  DPRINT("ScsiPortReadWrite() called!\n");

  Status = STATUS_SUCCESS;

  Irp->IoStatus.Status = Status;
  Irp->IoStatus.Information = 0;

  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return(Status);
}




static VOID STDCALL
ScsiPortStartIo(IN PDEVICE_OBJECT DeviceObject,
		IN PIRP Irp)
{
  PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;
  PIO_STACK_LOCATION IrpStack;
  KIRQL OldIrql;

  DPRINT("ScsiPortStartIo() called!\n");

  DeviceExtension = DeviceObject->DeviceExtension;
  IrpStack = IoGetCurrentIrpStackLocation(Irp);

  // FIXME: implement the supported functions

  switch (IrpStack->MajorFunction)
    {
      case IRP_MJ_SCSI:
	{
	  BOOLEAN Result;
	  PSCSI_REQUEST_BLOCK Srb;

	  DPRINT("IRP_MJ_SCSI\n");

	  Srb = IrpStack->Parameters.Scsi.Srb;

	  DPRINT("DeviceExtension %p\n", DeviceExtension);
	  DPRINT("DeviceExtension->ControllerObject %p\n", DeviceExtension->ControllerObject);

	  Irp->IoStatus.Status = STATUS_SUCCESS;
	  Irp->IoStatus.Information = Srb->DataTransferLength;
	  IoMarkIrpPending(Irp);

	  KeRaiseIrql(DISPATCH_LEVEL,
		      &OldIrql);
	  IoAllocateController(DeviceExtension->ControllerObject,
			       DeviceObject,
			       ScsiPortAllocateController,
			       Irp);
	  KeLowerIrql(OldIrql);
	}
	break;

      default:
	Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
	Irp->IoStatus.Information = 0;
	IoStartNextPacket(DeviceObject,
			  FALSE);
	IoCompleteRequest(Irp,
			  IO_NO_INCREMENT);
	break;
    }
  DPRINT("ScsiPortStartIo() done\n");
}


static IO_ALLOCATION_ACTION STDCALL
ScsiPortAllocateController(IN PDEVICE_OBJECT DeviceObject,
			   IN PIRP Irp,
			   IN PVOID MapRegisterBase,
			   IN PVOID Context)
{
  PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;
  PSCSI_PORT_CONTROLLER_EXTENSION ControllerExtension;

  DPRINT("ScsiPortAllocateController() called\n");

  DeviceExtension = (PSCSI_PORT_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
  ControllerExtension = (PSCSI_PORT_CONTROLLER_EXTENSION)
    DeviceExtension->ControllerObject->ControllerExtension;

  ControllerExtension->CurrentIrp = Irp;
  ControllerExtension->Retries = 0;
  return(KeSynchronizeExecution(ControllerExtension->Interrupt,
                                ScsiPortStartController,
                                DeviceExtension) ? KeepObject : 
                                  DeallocateObject);
}


static BOOLEAN STDCALL
ScsiPortStartController(IN OUT PVOID Context)
{
  PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;
  PSCSI_PORT_CONTROLLER_EXTENSION ControllerExtension;
  PIO_STACK_LOCATION IrpStack;
  PSCSI_REQUEST_BLOCK Srb;

  DPRINT("ScsiPortStartController() called\n");

  DeviceExtension = (PSCSI_PORT_DEVICE_EXTENSION) Context;
  ControllerExtension = (PSCSI_PORT_CONTROLLER_EXTENSION)
      DeviceExtension->ControllerObject->ControllerExtension;
  ControllerExtension->OperationInProgress = TRUE;
  ControllerExtension->DeviceForOperation = DeviceExtension;

  IrpStack = IoGetCurrentIrpStackLocation(ControllerExtension->CurrentIrp);
  Srb = IrpStack->Parameters.Scsi.Srb;

  return(DeviceExtension->HwStartIo(&DeviceExtension->MiniPortDeviceExtension,
				    Srb));
}


/**********************************************************************
 * NAME							INTERNAL
 *	ScsiPortCreatePortDevice
 *
 * DESCRIPTION
 *	Creates and initializes a SCSI port device object.
 *
 * RUN LEVEL
 *	PASSIVE_LEVEL
 *
 * ARGUMENTS
 *	DriverObject
 *		...
 *
 *	PseudoDeviceExtension
 *		...
 *
 *	PortNumber
 *		...
 *
 * RETURNS
 *	NTSTATUS
 */

static NTSTATUS
ScsiPortCreatePortDevice(IN PDRIVER_OBJECT DriverObject,
			 IN PSCSI_PORT_DEVICE_EXTENSION PseudoDeviceExtension,
			 IN ULONG PortNumber)
{
  PSCSI_PORT_CONTROLLER_EXTENSION ControllerExtension;
  PCONTROLLER_OBJECT ControllerObject;
  PSCSI_PORT_DEVICE_EXTENSION PortDeviceExtension;
  PDEVICE_OBJECT PortDeviceObject;
  WCHAR NameBuffer[80];
  UNICODE_STRING DeviceName;
  WCHAR DosNameBuffer[80];
  UNICODE_STRING DosDeviceName;
  NTSTATUS Status;
  ULONG AccessRangeSize;

#if 0
  ULONG MappedIrq;
  KIRQL Dirql;
  KAFFINITY Affinity;
#endif

  DPRINT("ScsiPortCreatePortDevice() called\n");

#if 0
  MappedIrq = HalGetInterruptVector(PseudoDeviceExtension->PortConfig.AdapterInterfaceType,
				    PseudoDeviceExtension->PortConfig.SystemIoBusNumber,
				    0,
				    PseudoDeviceExtension->PortConfig.BusInterruptLevel,
				    &Dirql,
				    &Affinity);
#endif

  ControllerObject = IoCreateController(sizeof(SCSI_PORT_CONTROLLER_EXTENSION));
  if (ControllerObject == NULL)
    {
      DbgPrint("Could not create controller object for port %d\n",
	       PortNumber);
      return(STATUS_NO_SUCH_DEVICE);
    }

  /* Fill out Controller extension data */
  ControllerExtension = (PSCSI_PORT_CONTROLLER_EXTENSION)
      ControllerObject->ControllerExtension;
  ControllerExtension->ControllerObject = ControllerObject;
  ControllerExtension->Number = PortNumber;
  ControllerExtension->Vector = PseudoDeviceExtension->PortConfig.BusInterruptVector;
  ControllerExtension->DMASupported = FALSE;
  ControllerExtension->ControllerInterruptBug = FALSE;
  ControllerExtension->OperationInProgress = FALSE;

  /* Initialize the spin lock in the controller extension */
  KeInitializeSpinLock(&ControllerExtension->SpinLock);

  /* Register an interrupt handler for this controller */
  Status = IoConnectInterrupt(&ControllerExtension->Interrupt,
			      ScsiPortIsr,
			      ControllerExtension,
			      &ControllerExtension->SpinLock,
			      PseudoDeviceExtension->PortConfig.BusInterruptVector, // MappedIrq,
			      PseudoDeviceExtension->PortConfig.BusInterruptLevel, // Dirql,
			      15, //Dirql,
			      PseudoDeviceExtension->PortConfig.InterruptMode,
			      FALSE,
			      0xFFFF, //Affinity,
			      FALSE);
  if (!NT_SUCCESS(Status))
    {
      DbgPrint("Could not Connect Interrupt %d\n",
	       ControllerExtension->Vector);
      IoDeleteController (ControllerObject);
      return(Status);
    }

  /* Create a unicode device name */
  swprintf(NameBuffer,
	   L"\\Device\\ScsiPort%lu",
	   PortNumber);
  RtlInitUnicodeString(&DeviceName,
		       NameBuffer);

  DPRINT("Creating device: %wZ\n", &DeviceName);

  /* Create the port device */
  Status = IoCreateDevice(DriverObject,
			  PseudoDeviceExtension->Length,
			  &DeviceName,
			  FILE_DEVICE_CONTROLLER,
			  0,
			  FALSE,
			  &PortDeviceObject);
  if (!NT_SUCCESS(Status))
    {
      DbgPrint("IoCreateDevice call failed! (Status 0x%lX)\n", Status);
      IoDisconnectInterrupt(ControllerExtension->Interrupt);
      IoDeleteController(ControllerObject);
      return(Status);
    }

  DPRINT("Created device: %wZ\n", &DeviceName);

  /* Set the buffering strategy here... */
  PortDeviceObject->Flags |= DO_DIRECT_IO;
  PortDeviceObject->AlignmentRequirement = FILE_WORD_ALIGNMENT;

  PortDeviceExtension = PortDeviceObject->DeviceExtension;

  /* Copy pseudo device extension into the real device extension */
  memcpy(PortDeviceExtension,
	 PseudoDeviceExtension,
	 PseudoDeviceExtension->Length);

  /* Copy access ranges */
  AccessRangeSize =
    sizeof(ACCESS_RANGE) * PseudoDeviceExtension->PortConfig.NumberOfAccessRanges;
  PortDeviceExtension->PortConfig.AccessRanges = ExAllocatePool(NonPagedPool,
								AccessRangeSize);
  memcpy(PortDeviceExtension->PortConfig.AccessRanges,
	 PseudoDeviceExtension->PortConfig.AccessRanges,
	 AccessRangeSize);

  /* Store pointers to controller object and port device object */
  PortDeviceExtension->ControllerObject = ControllerObject;
  PortDeviceExtension->DeviceObject = PortDeviceObject;
  ControllerExtension->PortDeviceObject = PortDeviceObject;

  /* Initialize the DPC object here */
  IoInitializeDpcRequest(ControllerExtension->PortDeviceObject,
			 ScsiPortDpcForIsr);

  /*
   * Initialize the controller timer here
   * (since it has to be tied to a device)
   */
  ControllerExtension->TimerState = IDETimerIdle;
  ControllerExtension->TimerCount = 0;
  IoInitializeTimer(ControllerExtension->PortDeviceObject,
		    ScsiPortIoTimer,
		    ControllerExtension);

  /* Initialize inquiry data */
  PortDeviceExtension->PortBusInfoSize = 0;
  PortDeviceExtension->PortBusInfo = NULL;

  /* FIXME: Copy more configuration data? */

  /* Create the dos device */
  swprintf(DosNameBuffer,
	   L"\\??\\Scsi%lu:",
	   PortNumber);
  RtlInitUnicodeString(&DosDeviceName,
		       DosNameBuffer);

  IoCreateSymbolicLink(&DosDeviceName,
		       &DeviceName);

  return(STATUS_SUCCESS);
}


static BOOLEAN STDCALL
ScsiPortIsr(IN PKINTERRUPT Interrupt,
	    IN PVOID ServiceContext)
{
  PSCSI_PORT_CONTROLLER_EXTENSION ControllerExtension;
  PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;
  BOOLEAN Result;

  DPRINT1("ScsiPortIsr() called!\n");

  ControllerExtension = (PSCSI_PORT_CONTROLLER_EXTENSION)ServiceContext;
  DeviceExtension = ControllerExtension->DeviceForOperation;

  Result = DeviceExtension->HwInterrupt(&DeviceExtension->MiniPortDeviceExtension);
  if (Result == FALSE)
    {
      return(FALSE);
    }

  IoRequestDpc(ControllerExtension->PortDeviceObject,
	       ControllerExtension->CurrentIrp,
	       ControllerExtension);

  return(TRUE);
}


//    ScsiPortDpcForIsr
//  DESCRIPTION:
//
//  RUN LEVEL:
//
//  ARGUMENTS:
//    IN PKDPC          Dpc
//    IN PDEVICE_OBJECT DpcDeviceObject
//    IN PIRP           DpcIrp
//    IN PVOID          DpcContext
//
static VOID STDCALL
ScsiPortDpcForIsr(IN PKDPC Dpc,
		  IN PDEVICE_OBJECT DpcDeviceObject,
		  IN PIRP DpcIrp,
		  IN PVOID DpcContext)
{
  DPRINT1("ScsiPortDpcForIsr()\n");
  ScsiPortFinishOperation((PSCSI_PORT_CONTROLLER_EXTENSION) DpcContext);
}


//    ScsiPortIoTimer
//  DESCRIPTION:
//    This function handles timeouts and other time delayed processing
//
//  RUN LEVEL:
//
//  ARGUMENTS:
//    IN  PDEVICE_OBJECT  DeviceObject  Device object registered with timer
//    IN  PVOID           Context       the Controller extension for the
//                                      controller the device is on
//
static VOID STDCALL
ScsiPortIoTimer(PDEVICE_OBJECT DeviceObject,
		PVOID Context)
{
  DPRINT("ScsiPortIoTimer()\n");
}



static VOID
ScsiPortFinishOperation(PSCSI_PORT_CONTROLLER_EXTENSION ControllerExtension)
{
  PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;
  PIRP Irp;
  ULONG Operation;

  DPRINT1("ScsiPortFinishOperation()\n");

  DeviceExtension = ControllerExtension->DeviceForOperation;
  Irp = ControllerExtension->CurrentIrp;
//  Operation = DeviceExtension->Operation;
  ControllerExtension->OperationInProgress = FALSE;
  ControllerExtension->DeviceForOperation = NULL;
  ControllerExtension->CurrentIrp = NULL;

  /* Deallocate the controller */
  IoFreeController(DeviceExtension->ControllerObject);

  /* Issue completion of the current packet */
  IoCompleteRequest(Irp,
		    IO_DISK_INCREMENT);

  /* Start the next packet */
  IoStartNextPacket(DeviceExtension->DeviceObject,
		    FALSE);

#if 0
  /* Flush cache if necessary */
  if (Operation == IRP_MJ_READ)
    {
      KeFlushIoBuffers(Irp->MdlAddress,
		       TRUE,
		       FALSE);
    }
#endif
}

/* EOF */
