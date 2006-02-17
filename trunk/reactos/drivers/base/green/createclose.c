/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS VT100 emulator
 * FILE:            drivers/dd/green/createclose.c
 * PURPOSE:         IRP_MJ_CREATE, IRP_MJ_CLOSE and IRP_MJ_CLEANUP operations
 *
 * PROGRAMMERS:     Herv� Poussineau (hpoussin@reactos.org)
 */

//#define NDEBUG
#include <debug.h>

#include "green.h"

NTSTATUS NTAPI
GreenCreate(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	DPRINT("Green: IRP_MJ_CREATE\n");

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS NTAPI
GreenClose(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	DPRINT("Green: IRP_MJ_CLOSE\n");

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}
