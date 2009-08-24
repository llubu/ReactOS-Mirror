/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/backpln/portcls/filter_wavecyclic.c
 * PURPOSE:         portcls wave cyclic filter
 * PROGRAMMER:      Johannes Anderwald
 */

#include "private.h"

typedef struct
{
    IPortFilterWaveCyclicVtbl *lpVtbl;

    LONG ref;

    IPortWaveCyclic* Port;
    IPortPinWaveCyclic ** Pins;
    SUBDEVICE_DESCRIPTOR * Descriptor;
    ISubdevice * SubDevice;

}IPortFilterWaveCyclicImpl;

/*
 * @implemented
 */
NTSTATUS
NTAPI
IPortFilterWaveCyclic_fnQueryInterface(
    IPortFilterWaveCyclic* iface,
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    IPortFilterWaveCyclicImpl * This = (IPortFilterWaveCyclicImpl*)iface;

    if (IsEqualGUIDAligned(refiid, &IID_IIrpTarget) || 
        IsEqualGUIDAligned(refiid, &IID_IUnknown))
    {
        *Output = &This->lpVtbl;
        InterlockedIncrement(&This->ref);
        return STATUS_SUCCESS;
    }
    else if (IsEqualGUIDAligned(refiid, &IID_IPort))
    {
        *Output = This->Port;
        This->Port->lpVtbl->AddRef(This->Port);
        return STATUS_SUCCESS;
    }


    return STATUS_UNSUCCESSFUL;
}

/*
 * @implemented
 */
ULONG
NTAPI
IPortFilterWaveCyclic_fnAddRef(
    IPortFilterWaveCyclic* iface)
{
    IPortFilterWaveCyclicImpl * This = (IPortFilterWaveCyclicImpl*)iface;

    return InterlockedIncrement(&This->ref);
}

/*
 * @implemented
 */
ULONG
NTAPI
IPortFilterWaveCyclic_fnRelease(
    IPortFilterWaveCyclic* iface)
{
    IPortFilterWaveCyclicImpl * This = (IPortFilterWaveCyclicImpl*)iface;

    InterlockedDecrement(&This->ref);

    if (This->ref == 0)
    {
        FreeItem(This, TAG_PORTCLASS);
        return 0;
    }
    return This->ref;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IPortFilterWaveCyclic_fnNewIrpTarget(
    IN IPortFilterWaveCyclic* iface,
    OUT struct IIrpTarget **OutTarget,
    IN WCHAR * Name,
    IN PUNKNOWN Unknown,
    IN POOL_TYPE PoolType,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN KSOBJECT_CREATE *CreateObject)
{
    NTSTATUS Status;
    IPortPinWaveCyclic * Pin;
    PKSPIN_CONNECT ConnectDetails;
    IPortFilterWaveCyclicImpl * This = (IPortFilterWaveCyclicImpl *)iface;

    ASSERT(This->Port);
    ASSERT(This->Descriptor);
    ASSERT(This->Pins);

    DPRINT("IPortFilterWaveCyclic_fnNewIrpTarget entered\n");

    /* let's verify the connection request */
    Status = PcValidateConnectRequest(Irp, &This->Descriptor->Factory, &ConnectDetails);
    if (!NT_SUCCESS(Status))
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (This->Pins[ConnectDetails->PinId] && 
        (This->Descriptor->Factory.Instances[ConnectDetails->PinId].CurrentPinInstanceCount == This->Descriptor->Factory.Instances[ConnectDetails->PinId].MaxFilterInstanceCount))
    {
        /* release existing instance */
        return STATUS_UNSUCCESSFUL;
    }

    /* now create the pin */
    Status = NewPortPinWaveCyclic(&Pin);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* initialize the pin */
    Status = Pin->lpVtbl->Init(Pin, This->Port, iface, ConnectDetails, &This->Descriptor->Factory.KsPinDescriptor[ConnectDetails->PinId]);
    if (!NT_SUCCESS(Status))
    {
        Pin->lpVtbl->Release(Pin);
        return Status;
    }

    /* store pin */
    This->Pins[ConnectDetails->PinId] = Pin;

    /* store result */
    *OutTarget = (IIrpTarget*)Pin;

    /* increment current instance count */
    This->Descriptor->Factory.Instances[ConnectDetails->PinId].CurrentPinInstanceCount++;

    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IPortFilterWaveCyclic_fnDeviceIoControl(
    IN IPortFilterWaveCyclic* iface,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    IPortFilterWaveCyclicImpl * This = (IPortFilterWaveCyclicImpl *)iface;

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    if (IoStack->Parameters.DeviceIoControl.IoControlCode != IOCTL_KS_PROPERTY)
    {
        DPRINT1("Unhandled function %lx Length %x\n", IoStack->Parameters.DeviceIoControl.IoControlCode, IoStack->Parameters.DeviceIoControl.InputBufferLength);
        
        Irp->IoStatus.Status = STATUS_SUCCESS;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_SUCCESS;
    }


    ASSERT(IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_KS_PROPERTY);

    return PcPropertyHandler(Irp, This->Descriptor);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IPortFilterWaveCyclic_fnRead(
    IN IPortFilterWaveCyclic* iface,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    return KsDispatchInvalidDeviceRequest(DeviceObject, Irp);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IPortFilterWaveCyclic_fnWrite(
    IN IPortFilterWaveCyclic* iface,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    return KsDispatchInvalidDeviceRequest(DeviceObject, Irp);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IPortFilterWaveCyclic_fnFlush(
    IN IPortFilterWaveCyclic* iface,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    return KsDispatchInvalidDeviceRequest(DeviceObject, Irp);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IPortFilterWaveCyclic_fnClose(
    IN IPortFilterWaveCyclic* iface,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    ULONG Index;
    NTSTATUS Status = STATUS_SUCCESS;
    IPortFilterWaveCyclicImpl * This = (IPortFilterWaveCyclicImpl *)iface;

    DPRINT("IPortFilterWaveCyclic_fnClose ref %u\n", This->ref);

    if (This->ref == 1)
    {
        for(Index = 0; Index < This->Descriptor->Factory.PinDescriptorCount; Index++)
        {
            /* all pins should have been closed by now */
            ASSERT(This->Pins[Index] == NULL);
        }

        /* release reference to port */
        This->SubDevice->lpVtbl->Release(This->SubDevice);

        /* time to shutdown the audio system */
        Status = This->SubDevice->lpVtbl->ReleaseChildren(This->SubDevice);
    }

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IPortFilterWaveCyclic_fnQuerySecurity(
    IN IPortFilterWaveCyclic* iface,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    return KsDispatchInvalidDeviceRequest(DeviceObject, Irp);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IPortFilterWaveCyclic_fnSetSecurity(
    IN IPortFilterWaveCyclic* iface,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    return KsDispatchInvalidDeviceRequest(DeviceObject, Irp);
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
IPortFilterWaveCyclic_fnFastDeviceIoControl(
    IN IPortFilterWaveCyclic* iface,
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer,
    IN ULONG OutputBufferLength,
    IN ULONG IoControlCode,
    OUT PIO_STATUS_BLOCK StatusBlock,
    IN PDEVICE_OBJECT DeviceObject)
{
    UNIMPLEMENTED
    return FALSE;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
IPortFilterWaveCyclic_fnFastRead(
    IN IPortFilterWaveCyclic* iface,
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN PVOID Buffer,
    OUT PIO_STATUS_BLOCK StatusBlock,
    IN PDEVICE_OBJECT DeviceObject)
{
    UNIMPLEMENTED
    return FALSE;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
IPortFilterWaveCyclic_fnFastWrite(
    IN IPortFilterWaveCyclic* iface,
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN PVOID Buffer,
    OUT PIO_STATUS_BLOCK StatusBlock,
    IN PDEVICE_OBJECT DeviceObject)
{
    UNIMPLEMENTED
    return FALSE;
}

/*
 * @implemented
 */
static
NTSTATUS
NTAPI
IPortFilterWaveCyclic_fnInit(
    IN IPortFilterWaveCyclic* iface,
    IN IPortWaveCyclic* Port)
{
    ISubdevice * ISubDevice;
    SUBDEVICE_DESCRIPTOR * Descriptor;
    NTSTATUS Status;
    IPortFilterWaveCyclicImpl * This = (IPortFilterWaveCyclicImpl*)iface;

    /* get our private interface */
    Status = Port->lpVtbl->QueryInterface(Port, &IID_ISubdevice, (PVOID*)&ISubDevice);
    if (!NT_SUCCESS(Status))
        return STATUS_UNSUCCESSFUL;

    /* get the subdevice descriptor */
    Status = ISubDevice->lpVtbl->GetDescriptor(ISubDevice, &Descriptor);

    /* store subdevice interface */
    This->SubDevice = ISubDevice;

    if (!NT_SUCCESS(Status))
        return STATUS_UNSUCCESSFUL;

    /* save descriptor */
    This->Descriptor = Descriptor;

    /* allocate pin array */
    This->Pins = AllocateItem(NonPagedPool, Descriptor->Factory.PinDescriptorCount * sizeof(IPortPinWaveCyclic*), TAG_PORTCLASS);

    if (!This->Pins)
        return STATUS_UNSUCCESSFUL;

    /* store port driver */
    This->Port = Port;

    return STATUS_SUCCESS;
}


static
NTSTATUS
NTAPI
IPortFilterWaveCyclic_fnFreePin(
    IN IPortFilterWaveCyclic* iface,
    IN struct IPortPinWaveCyclic* Pin)
{
    ULONG Index;
    IPortFilterWaveCyclicImpl * This = (IPortFilterWaveCyclicImpl*)iface;

    for(Index = 0; Index < This->Descriptor->Factory.PinDescriptorCount; Index++)
    {
        if (This->Pins[Index] == Pin)
        {
            This->Descriptor->Factory.Instances[Index].CurrentPinInstanceCount--;
            This->Pins[Index]->lpVtbl->Release(This->Pins[Index]);
            This->Pins[Index] = NULL;
            return STATUS_SUCCESS;
        }
    }
    return STATUS_UNSUCCESSFUL;
}

static IPortFilterWaveCyclicVtbl vt_IPortFilterWaveCyclic =
{
    IPortFilterWaveCyclic_fnQueryInterface,
    IPortFilterWaveCyclic_fnAddRef,
    IPortFilterWaveCyclic_fnRelease,
    IPortFilterWaveCyclic_fnNewIrpTarget,
    IPortFilterWaveCyclic_fnDeviceIoControl,
    IPortFilterWaveCyclic_fnRead,
    IPortFilterWaveCyclic_fnWrite,
    IPortFilterWaveCyclic_fnFlush,
    IPortFilterWaveCyclic_fnClose,
    IPortFilterWaveCyclic_fnQuerySecurity,
    IPortFilterWaveCyclic_fnSetSecurity,
    IPortFilterWaveCyclic_fnFastDeviceIoControl,
    IPortFilterWaveCyclic_fnFastRead,
    IPortFilterWaveCyclic_fnFastWrite,
    IPortFilterWaveCyclic_fnInit,
    IPortFilterWaveCyclic_fnFreePin
};

NTSTATUS 
NewPortFilterWaveCyclic(
    OUT IPortFilterWaveCyclic ** OutFilter)
{
    IPortFilterWaveCyclicImpl * This;

    This = AllocateItem(NonPagedPool, sizeof(IPortFilterWaveCyclicImpl), TAG_PORTCLASS);
    if (!This)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* initialize IPortFilterWaveCyclic */
    This->ref = 1;
    This->lpVtbl = &vt_IPortFilterWaveCyclic;

    /* return result */
    *OutFilter = (IPortFilterWaveCyclic*)&This->lpVtbl;

    return STATUS_SUCCESS;
}


