/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/filters/kmixer/filter.c
 * PURPOSE:         Filter File Context Header header
 * PROGRAMMER:      Johannes Anderwald
 */

#include "kmixer.h"

NTSTATUS
NTAPI
Dispatch_fnDeviceIoControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    UNIMPLEMENTED

    Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
Dispatch_fnRead(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    UNIMPLEMENTED

    Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
Dispatch_fnWrite(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    UNIMPLEMENTED

    Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
Dispatch_fnFlush(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    UNIMPLEMENTED
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
Dispatch_fnClose(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    UNIMPLEMENTED

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
Dispatch_fnQuerySecurity(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    UNIMPLEMENTED

    Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
Dispatch_fnSetSecurity(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{

    UNIMPLEMENTED

    Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_UNSUCCESSFUL;
}

BOOLEAN
NTAPI
Dispatch_fnFastDeviceIoControl(
    PFILE_OBJECT FileObject,
    BOOLEAN Wait,
    PVOID InputBuffer,
    ULONG InputBufferLength,
    PVOID OutputBuffer,
    ULONG OutputBufferLength,
    ULONG IoControlCode,
    PIO_STATUS_BLOCK IoStatus,
    PDEVICE_OBJECT DeviceObject)
{
    UNIMPLEMENTED


    return FALSE;
}


BOOLEAN
NTAPI
Dispatch_fnFastRead(
    PFILE_OBJECT FileObject,
    PLARGE_INTEGER FileOffset,
    ULONG Length,
    BOOLEAN Wait,
    ULONG LockKey,
    PVOID Buffer,
    PIO_STATUS_BLOCK IoStatus,
    PDEVICE_OBJECT DeviceObject)
{
    UNIMPLEMENTED

    return FALSE;

}

BOOLEAN
NTAPI
Dispatch_fnFastWrite(
    PFILE_OBJECT FileObject,
    PLARGE_INTEGER FileOffset,
    ULONG Length,
    BOOLEAN Wait,
    ULONG LockKey,
    PVOID Buffer,
    PIO_STATUS_BLOCK IoStatus,
    PDEVICE_OBJECT DeviceObject)
{
    UNIMPLEMENTED

    return FALSE;
}

static KSDISPATCH_TABLE DispatchTable =
{
    Dispatch_fnDeviceIoControl,
    Dispatch_fnRead,
    Dispatch_fnWrite,
    Dispatch_fnFlush,
    Dispatch_fnClose,
    Dispatch_fnQuerySecurity,
    Dispatch_fnSetSecurity,
    Dispatch_fnFastDeviceIoControl,
    Dispatch_fnFastRead,
    Dispatch_fnFastWrite,
};

NTSTATUS
NTAPI
DispatchCreateKMixPin(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    NTSTATUS Status;

    DPRINT("DispatchCreateKMix entered\n");

    /* create the pin */
    Status = CreatePin(Irp);

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

NTSTATUS
NTAPI
DispatchCreateKMix(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    NTSTATUS Status;
    KSOBJECT_HEADER ObjectHeader;
    PKSOBJECT_CREATE_ITEM CreateItem;

    static LPWSTR KS_NAME_PIN = L"{146F1A80-4791-11D0-A5D6-28DB04C10000}";

    DPRINT("DispatchCreateKMix entered\n");

   /* allocate create item */
    CreateItem = ExAllocatePool(NonPagedPool, sizeof(KSOBJECT_CREATE_ITEM));
    if (!CreateItem)
    {
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* zero create struct */
    RtlZeroMemory(CreateItem, sizeof(KSOBJECT_CREATE_ITEM));

    /* initialize pin create item */
    CreateItem->Create = DispatchCreateKMixPin;
    RtlInitUnicodeString(&CreateItem->ObjectClass, KS_NAME_PIN);


    /* allocate object header */
    Status = KsAllocateObjectHeader(&ObjectHeader, 1, CreateItem, Irp, &DispatchTable);

    DPRINT("KsAllocateObjectHeader result %x\n", Status);
    /* complete the irp */
    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

NTSTATUS
NTAPI
KMixAllocateDeviceHeader(
    IN PKMIXER_DEVICE_EXT DeviceExtension)
{
    NTSTATUS Status;
    PKSOBJECT_CREATE_ITEM CreateItem;

    /* allocate create item */
    CreateItem = ExAllocatePool(NonPagedPool, sizeof(KSOBJECT_CREATE_ITEM));
    if (!CreateItem)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* initialize create item struct */
    RtlZeroMemory(CreateItem, sizeof(KSOBJECT_CREATE_ITEM));
    CreateItem->Create = DispatchCreateKMix;
    CreateItem->Flags = KSCREATE_ITEM_WILDCARD;
    RtlInitUnicodeString(&CreateItem->ObjectClass, L"KMixer");

    Status = KsAllocateDeviceHeader(&DeviceExtension->KsDeviceHeader,
                                    1,
                                    CreateItem);
    return Status;
}

