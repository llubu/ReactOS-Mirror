/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS UHCI controller driver (Cromwell type)
 * FILE:            drivers/usb/cromwell/uhci/pdo.c
 * PURPOSE:         IRP_MJ_PNP/IRP_MJ_DEVICE_CONTROL operations for PDOs
 *
 * PROGRAMMERS:     Herv� Poussineau (hpoussin@reactos.com),
 *                  James Tabor (jimtabor@adsl-64-217-116-74.dsl.hstntx.swbell.net)
 */

#define NDEBUG
#include "uhci.h"

extern struct usb_driver hub_driver;

#define IO_METHOD_FROM_CTL_CODE(ctlCode) (ctlCode&0x00000003)

NTSTATUS
UhciDeviceControlPdo(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	PIO_STACK_LOCATION Stack;
	ULONG_PTR Information = 0;
	NTSTATUS Status;
	
	DPRINT("UHCI: UhciDeviceControlPdo() called\n");
	
	Stack = IoGetCurrentIrpStackLocation(Irp);
	Status = Irp->IoStatus.Status;
	
	switch (Stack->Parameters.DeviceIoControl.IoControlCode)
	{
		case IOCTL_INTERNAL_USB_GET_ROOT_USB_DEVICE:
		{
			POHCI_DEVICE_EXTENSION DeviceExtension;
			
			DPRINT("UHCI: IOCTL_INTERNAL_USB_GET_ROOT_USB_DEVICE\n");
			if (Irp->AssociatedIrp.SystemBuffer == NULL
				|| Stack->Parameters.DeviceIoControl.OutputBufferLength != sizeof(PVOID))
			{
				Status = STATUS_INVALID_PARAMETER;
			}
			else
			{
				PVOID* pRootHubPointer;
				DeviceExtension = (POHCI_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
				DeviceExtension = (POHCI_DEVICE_EXTENSION)DeviceExtension->FunctionalDeviceObject->DeviceExtension;
				
				pRootHubPointer = (PVOID*)Irp->AssociatedIrp.SystemBuffer;
				*pRootHubPointer = (PVOID)DeviceExtension->pdev->bus; /* struct usb_device* */
				Information = sizeof(PVOID);
				Status = STATUS_SUCCESS;
			}
			break;
		}
		default:
		{
			DPRINT1("UHCI: Unknown IOCTL code 0x%lx\n", Stack->Parameters.DeviceIoControl.IoControlCode);
			Information = Irp->IoStatus.Information;
			Status = Irp->IoStatus.Status;
		}
	}
	
	Irp->IoStatus.Information = Information;
	Irp->IoStatus.Status = Status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return Status;
}

static NTSTATUS
UhciPdoQueryId(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp,
	OUT ULONG_PTR* Information)
{
	POHCI_DEVICE_EXTENSION DeviceExtension;
	ULONG IdType;
	UNICODE_STRING SourceString;
	UNICODE_STRING String;
	NTSTATUS Status;

	IdType = IoGetCurrentIrpStackLocation(Irp)->Parameters.QueryId.IdType;
	DeviceExtension = (POHCI_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	RtlInitUnicodeString(&String, NULL);

	switch (IdType)
	{
		case BusQueryDeviceID:
		{
			DPRINT("UHCI: IRP_MJ_PNP / IRP_MN_QUERY_ID / BusQueryDeviceID\n");
			RtlInitUnicodeString(&SourceString, L"USB\\ROOT_HUB");
			break;
		}
		case BusQueryHardwareIDs:
		{
			DPRINT("UHCI: IRP_MJ_PNP / IRP_MN_QUERY_ID / BusQueryHardwareIDs\n");
			/* FIXME: Should return
				USB\ROOT_HUB&VID????&PID????&REV????
				USB\ROOT_HUB&VID????&PID????
				USB\ROOT_HUB
			*/
			UhciInitMultiSzString(&SourceString, "USB\\ROOT_HUB", NULL);
			break;
		}
		case BusQueryCompatibleIDs:
			DPRINT("UHCI: IRP_MJ_PNP / IRP_MN_QUERY_ID / BusQueryCompatibleIDs\n");
			/* No compatible ID */
			*Information = 0;
			return STATUS_NOT_SUPPORTED;
		case BusQueryInstanceID:
		{
			DPRINT("UHCI: IRP_MJ_PNP / IRP_MN_QUERY_ID / BusQueryInstanceID\n");
			RtlInitUnicodeString(&SourceString, L"0000"); /* FIXME */
			break;
		}
		default:
			DPRINT1("UHCI: IRP_MJ_PNP / IRP_MN_QUERY_ID / unknown query id type 0x%lx\n", IdType);
			return STATUS_NOT_SUPPORTED;
	}

	Status = UhciDuplicateUnicodeString(
		&String,
		&SourceString,
		PagedPool);
	*Information = (ULONG_PTR)String.Buffer;
	return Status;
}

static NTSTATUS
UhciPnpStartDevice(
	IN PDEVICE_OBJECT DeviceObject)
{
	POHCI_DEVICE_EXTENSION DeviceExtension;
	NTSTATUS Status;
	
	DeviceExtension = (POHCI_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	
	/* Register device interface for root hub */
	Status = IoRegisterDeviceInterface(
		DeviceObject,
		&GUID_DEVINTERFACE_USB_HUB,
		NULL,
		&DeviceExtension->HcdInterfaceName);
	if (!NT_SUCCESS(Status))
	{
		DPRINT("UHCI: IoRegisterDeviceInterface() failed with status 0x%08lx\n", Status);
		return Status;
	}
	
	return Status;
}

NTSTATUS STDCALL
UhciPnpPdo(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	ULONG MinorFunction;
	PIO_STACK_LOCATION Stack;
	ULONG_PTR Information = 0;
	NTSTATUS Status;
	
	Stack = IoGetCurrentIrpStackLocation(Irp);
	MinorFunction = Stack->MinorFunction;

	switch (MinorFunction)
	{
		case IRP_MN_START_DEVICE: /* 0x00 */
		{
			DPRINT("UHCI: IRP_MJ_PNP/IRP_MN_START_DEVICE\n");
			Status = UhciPnpStartDevice(DeviceObject);
			break;
		}
		case IRP_MN_QUERY_ID: /* 0x13 */
		{
			Status = UhciPdoQueryId(DeviceObject, Irp, &Information);
			break;
		}
		default:
		{
			/* We can't forward request to the lower driver, because
			 * we are a Pdo, so we don't have lower driver...
			 */
			DPRINT1("UHCI: IRP_MJ_PNP / unknown minor function 0x%lx\n", MinorFunction);
			Information = Irp->IoStatus.Information;
			Status = Irp->IoStatus.Status;
		}
	}

	Irp->IoStatus.Information = Information;
	Irp->IoStatus.Status = Status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return Status;
}

