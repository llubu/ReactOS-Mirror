/* $Id: debugout.c,v 1.3 2004/02/10 16:22:55 navaraf Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/dd/debugout.c
 * PURPOSE:          Debug output device driver
 * PROGRAMMER:       Ge van Geldorp (ge@gse.nl)
 * UPDATE HISTORY:
 *              2003/05/22: Created
 * NOTES:
 * In your usermode application, do something like this:
 *
 *  DebugHandle = CreateFile("\\\\.\\DebugOut", 
 *                           GENERIC_WRITE, 
 *                           0, 
 *                           NULL,
 *                           OPEN_EXISTING, 
 *                           FILE_ATTRIBUTE_NORMAL, 
 *                           NULL);
 *
 * and write to your hearts content to DebugHandle.
 */

/* INCLUDES */
#include <ddk/ntddk.h>
#include <rosrtl/string.h>

/* FUNCTIONS */
NTSTATUS STDCALL
DebugOutDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
  PIO_STACK_LOCATION piosStack = IoGetCurrentIrpStackLocation(Irp);
  NTSTATUS nErrCode;
  char *Start;
  char Buf[513];
  unsigned Remaining;
  unsigned Length;
 
  nErrCode = STATUS_SUCCESS;

  switch(piosStack->MajorFunction)
    {
      /* opening and closing handles to the device */
      case IRP_MJ_CREATE:
      case IRP_MJ_CLOSE:
        break;

      /* write data */
      case IRP_MJ_WRITE:
	Remaining = piosStack->Parameters.Write.Length;
        Start = Irp->AssociatedIrp.SystemBuffer;
	while (0 < Remaining)
	  {
	    Length = Remaining;
	    if (sizeof(Buf) - 1 < Length)
	      {
		Length = sizeof(Buf) - 1;
	      }
	    RtlCopyMemory(Buf, Start, Length);
	    Buf[Length] = '\0';
	    DbgPrint("%s", Buf);
	    Remaining -= Length;
	    Start += Length;
	  }

        Irp->IoStatus.Information = piosStack->Parameters.Write.Length;
        break;

      /* read data */
      case IRP_MJ_READ:
        Irp->IoStatus.Information = 0;
        nErrCode = STATUS_END_OF_FILE;
        break;

      /* unsupported operations */
      default:
        nErrCode = STATUS_NOT_IMPLEMENTED;
    }

  Irp->IoStatus.Status = nErrCode;
  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return nErrCode;
}

VOID STDCALL
DebugOutUnload(PDRIVER_OBJECT DriverObject)
{
}

NTSTATUS STDCALL 
DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
  PDEVICE_OBJECT DebugOutDevice;
  UNICODE_STRING DeviceName;
  UNICODE_STRING DosName;
  NTSTATUS Status;

  /* register driver routines */
  DriverObject->MajorFunction[IRP_MJ_CLOSE] = DebugOutDispatch;
  DriverObject->MajorFunction[IRP_MJ_CREATE] = DebugOutDispatch;
  DriverObject->MajorFunction[IRP_MJ_WRITE] = DebugOutDispatch;
  DriverObject->MajorFunction[IRP_MJ_READ] = DebugOutDispatch;
  /* DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION] = DebugOutDispatch; */
  DriverObject->DriverUnload = DebugOutUnload;

  /* create device */
  RtlRosInitUnicodeStringFromLiteral(&DeviceName, L"\\Device\\DebugOut");

  Status = IoCreateDevice(DriverObject, 0, &DeviceName, FILE_DEVICE_NULL,
                            0, FALSE, &DebugOutDevice);
  if (! NT_SUCCESS(Status))
    {
      return Status;
    }

  RtlRosInitUnicodeStringFromLiteral(&DosName, L"\\DosDevices\\DebugOut");
  Status = IoCreateSymbolicLink(&DosName, &DeviceName);
  if (! NT_SUCCESS(Status))
    {
    IoDeleteDevice(DebugOutDevice);
    return Status;
    }

  DebugOutDevice->Flags |= DO_BUFFERED_IO;

  return Status;
}

/* EOF */
