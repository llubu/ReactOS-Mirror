/* $Id$
 *
 * PROJECT:         ReactOS ACPI bus driver
 * FILE:            acpi/ospm/pdo.c
 * PURPOSE:         Child device object dispatch routines
 * PROGRAMMERS:     Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      08-08-2001  CSH  Created
 */
#include <acpisys.h>
#include <bm.h>
#include <bn.h>

#define NDEBUG
#include <debug.h>

/*** PRIVATE *****************************************************************/

static NTSTATUS
AcpiDuplicateUnicodeString(
  PUNICODE_STRING Destination,
  PUNICODE_STRING Source,
  POOL_TYPE PoolType)
{
  if (Source == NULL)
  {
    RtlInitUnicodeString(Destination, NULL);
    return STATUS_SUCCESS;
  }

  Destination->Buffer = ExAllocatePool(PoolType, Source->MaximumLength);
  if (Destination->Buffer == NULL)
  {
    return STATUS_INSUFFICIENT_RESOURCES;
  }

  Destination->MaximumLength = Source->MaximumLength;
  Destination->Length = Source->Length;
  RtlCopyMemory(Destination->Buffer, Source->Buffer, Source->MaximumLength);

  return STATUS_SUCCESS;
}


static NTSTATUS
PdoQueryId(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp,
  PIO_STACK_LOCATION IrpSp)
{
  PPDO_DEVICE_EXTENSION DeviceExtension;
  UNICODE_STRING String;
  NTSTATUS Status;

  DPRINT("Called\n");

  DeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

  RtlInitUnicodeString(&String, NULL);

//  Irp->IoStatus.Information = 0;

  switch (IrpSp->Parameters.QueryId.IdType)
  {
    case BusQueryDeviceID:
      DPRINT("BusQueryDeviceID\n");
      Status = AcpiDuplicateUnicodeString(&String,
                                          &DeviceExtension->DeviceID,
                                          PagedPool);
      DPRINT("DeviceID: %S\n", String.Buffer);
      Irp->IoStatus.Information = (ULONG_PTR)String.Buffer;
      break;

    case BusQueryHardwareIDs:
      DPRINT("BusQueryHardwareIDs\n");
      Status = AcpiDuplicateUnicodeString(&String,
                                          &DeviceExtension->HardwareIDs,
                                          PagedPool);
      Irp->IoStatus.Information = (ULONG_PTR)String.Buffer;
      break;

    case BusQueryCompatibleIDs:
      DPRINT("BusQueryCompatibleIDs\n");
      Status = STATUS_NOT_IMPLEMENTED;
      break;

    case BusQueryInstanceID:
      DPRINT("BusQueryInstanceID\n");
      Status = AcpiDuplicateUnicodeString(&String,
                                          &DeviceExtension->InstanceID,
                                          PagedPool);
      DPRINT("InstanceID: %S\n", String.Buffer);
      Irp->IoStatus.Information = (ULONG_PTR)String.Buffer;
      break;

    case BusQueryDeviceSerialNumber:
      DPRINT("BusQueryDeviceSerialNumber\n");
      Status = STATUS_NOT_IMPLEMENTED;
      break;

    default:
      DPRINT("Unknown id type: %lx\n", IrpSp->Parameters.QueryId.IdType);
      Status = STATUS_NOT_IMPLEMENTED;
  }

  return Status;
}


static NTSTATUS
PdoSetPower(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp,
  PIO_STACK_LOCATION IrpSp)
{
  PPDO_DEVICE_EXTENSION DeviceExtension;
  NTSTATUS Status;

  DPRINT("Called\n");

  DeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

  if (IrpSp->Parameters.Power.Type == DevicePowerState) {
    Status = STATUS_SUCCESS;
    switch (IrpSp->Parameters.Power.State.SystemState) {
    default:
      Status = STATUS_UNSUCCESSFUL;
    }
  } else {
    Status = STATUS_UNSUCCESSFUL;
  }

  return Status;
}


/*** PUBLIC ******************************************************************/

NTSTATUS
STDCALL
PdoPnpControl(
  PDEVICE_OBJECT DeviceObject,
  PIRP Irp)
/*
 * FUNCTION: Handle Plug and Play IRPs for the child device
 * ARGUMENTS:
 *     DeviceObject = Pointer to physical device object of the child device
 *     Irp          = Pointer to IRP that should be handled
 * RETURNS:
 *     Status
 */
{
  PIO_STACK_LOCATION IrpSp;
  NTSTATUS Status;

  DPRINT("Called\n");

  Status = Irp->IoStatus.Status;

  IrpSp = IoGetCurrentIrpStackLocation(Irp);

  switch (IrpSp->MinorFunction) {
  case IRP_MN_CANCEL_REMOVE_DEVICE:
    break;

  case IRP_MN_CANCEL_STOP_DEVICE:
    break;

  case IRP_MN_DEVICE_USAGE_NOTIFICATION:
    break;

  case IRP_MN_EJECT:
    break;

  case IRP_MN_QUERY_BUS_INFORMATION:
    break;

  case IRP_MN_QUERY_CAPABILITIES:
    break;

  case IRP_MN_QUERY_DEVICE_RELATIONS:
    /* FIXME: Possibly handle for RemovalRelations */
    break;

  case IRP_MN_QUERY_DEVICE_TEXT:
    break;

  case IRP_MN_QUERY_ID:
    Status = PdoQueryId(DeviceObject,
                        Irp,
                        IrpSp);
    break;

  case IRP_MN_QUERY_PNP_DEVICE_STATE:
    break;

  case IRP_MN_QUERY_REMOVE_DEVICE:
    break;

  case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
    break;

  case IRP_MN_QUERY_RESOURCES:
    break;

  case IRP_MN_QUERY_STOP_DEVICE:
    break;

  case IRP_MN_REMOVE_DEVICE:
    break;

  case IRP_MN_SET_LOCK:
    break;

  case IRP_MN_START_DEVICE:
    break;

  case IRP_MN_STOP_DEVICE:
    break;

  case IRP_MN_SURPRISE_REMOVAL:
    break;

  default:
    DPRINT("Unknown IOCTL 0x%X\n", IrpSp->MinorFunction);
    break;
  }

  if (Status != STATUS_PENDING) {
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
  }

  DPRINT("Leaving. Status 0x%X\n", Status);

  return Status;
}

NTSTATUS
STDCALL
PdoPowerControl(
  PDEVICE_OBJECT DeviceObject,
  PIRP Irp)
/*
 * FUNCTION: Handle power management IRPs for the child device
 * ARGUMENTS:
 *     DeviceObject = Pointer to physical device object of the child device
 *     Irp          = Pointer to IRP that should be handled
 * RETURNS:
 *     Status
 */
{
  PIO_STACK_LOCATION IrpSp;
  NTSTATUS Status;

  DPRINT("Called\n");

  IrpSp = IoGetCurrentIrpStackLocation(Irp);

  switch (IrpSp->MinorFunction) {
  case IRP_MN_SET_POWER:
    Status = PdoSetPower(DeviceObject, Irp, IrpSp);
    break;

  default:
    DPRINT("Unknown IOCTL 0x%X\n", IrpSp->MinorFunction);
    Status = STATUS_NOT_IMPLEMENTED;
    break;
  }

  if (Status != STATUS_PENDING) {
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
  }

  DPRINT("Leaving. Status 0x%X\n", Status);

  return Status;
}

/* EOF */
