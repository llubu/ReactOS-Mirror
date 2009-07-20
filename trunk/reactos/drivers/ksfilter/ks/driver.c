/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/ksfilter/ks/driver.c
 * PURPOSE:         KS Driver functions
 * PROGRAMMER:      Johannes Anderwald
 */

#include "priv.h"

#include "ksfunc.h"


NTSTATUS
NTAPI
DriverEntry(
    IN PDRIVER_OBJECT Driver,
    IN PUNICODE_STRING Registry_path
)
{
    DPRINT1("ks.sys loaded\n");
    return STATUS_SUCCESS;
}

/*
    @implemented
*/
KSDDKAPI
PKSDEVICE
NTAPI
KsGetDeviceForDeviceObject(
    IN PDEVICE_OBJECT FunctionalDeviceObject)
{
    PDEVICE_EXTENSION DeviceExtension;

    /* get device extension */
    DeviceExtension = (PDEVICE_EXTENSION)FunctionalDeviceObject->DeviceExtension;

    return &DeviceExtension->DeviceHeader->KsDevice;
}

/*
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsCreateDevice(
    IN  PDRIVER_OBJECT DriverObject,
    IN  PDEVICE_OBJECT PhysicalDeviceObject,
    IN  const KSDEVICE_DESCRIPTOR* Descriptor OPTIONAL,
    IN  ULONG ExtensionSize OPTIONAL,
    OUT PKSDEVICE* Device OPTIONAL)
{
    NTSTATUS Status = STATUS_DEVICE_REMOVED;
    PDEVICE_OBJECT FunctionalDeviceObject= NULL;
    PDEVICE_OBJECT OldHighestDeviceObject;
    if (!ExtensionSize)
        ExtensionSize = sizeof(KSDEVICE_HEADER);

    Status = IoCreateDevice(DriverObject, ExtensionSize, NULL, FILE_DEVICE_KS, FILE_DEVICE_SECURE_OPEN, FALSE, &FunctionalDeviceObject);
    if (!NT_SUCCESS(Status))
        return Status;

    OldHighestDeviceObject = IoAttachDeviceToDeviceStack(FunctionalDeviceObject, PhysicalDeviceObject);
    if (OldHighestDeviceObject)
    {
        Status = KsInitializeDevice(FunctionalDeviceObject, PhysicalDeviceObject, OldHighestDeviceObject, Descriptor);
    }
    else
    {
        Status = STATUS_DEVICE_REMOVED;
    }

    /* check if all succeeded */
    if (!NT_SUCCESS(Status))
    {
        if (OldHighestDeviceObject)
            IoDetachDevice(OldHighestDeviceObject);

        IoDeleteDevice(FunctionalDeviceObject);
        return Status;
    }

    /* set device flags */
    FunctionalDeviceObject->Flags |= DO_DIRECT_IO | DO_POWER_PAGABLE;
    FunctionalDeviceObject->Flags &= ~ DO_DEVICE_INITIALIZING;

    if (Device)
    {
        /* get PKSDEVICE struct */
        *Device = KsGetDeviceForDeviceObject(FunctionalDeviceObject);

        if (ExtensionSize > sizeof(KSDEVICE_HEADER))
        {
            /* caller needs a device extension */
            (*Device)->Context = (PVOID)((ULONG_PTR)FunctionalDeviceObject->DeviceExtension + sizeof(KSDEVICE_HEADER));
        }
    }

    return Status;
}

/*
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsAddDevice(
    IN  PDRIVER_OBJECT DriverObject,
    IN  PDEVICE_OBJECT PhysicalDeviceObject)
{
    PKSDEVICE_DESCRIPTOR *DriverObjectExtension;
    PKSDEVICE_DESCRIPTOR Descriptor = NULL;

    DriverObjectExtension = IoGetDriverObjectExtension(DriverObject, (PVOID)KsAddDevice);
    if (DriverObjectExtension)
    {
        Descriptor = *DriverObjectExtension;
    }

    return KsCreateDevice(DriverObject, PhysicalDeviceObject, Descriptor, 0, NULL);
}

/*
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsInitializeDriver(
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING  RegistryPath,
    IN const KSDEVICE_DESCRIPTOR  *Descriptor OPTIONAL
)
{
    PKSDEVICE_DESCRIPTOR *DriverObjectExtension;
    NTSTATUS Status;

    if (Descriptor)
    {
        Status = IoAllocateDriverObjectExtension(DriverObject, (PVOID)KsAddDevice, sizeof(PKSDEVICE_DESCRIPTOR), (PVOID*)&DriverObjectExtension);
        if (NT_SUCCESS(Status))
        {
            *DriverObjectExtension = (KSDEVICE_DESCRIPTOR*)Descriptor;
        }
    }
    /* Setting our IRP handlers */
    DriverObject->MajorFunction[IRP_MJ_CREATE] = IKsDevice_Create;
    DriverObject->MajorFunction[IRP_MJ_PNP] = IKsDevice_Pnp;
    DriverObject->MajorFunction[IRP_MJ_POWER] = IKsDevice_Power;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = KsDefaultForwardIrp;

    /* The driver unload routine */
    DriverObject->DriverUnload = KsNullDriverUnload;

    /* The driver-supplied AddDevice */
    DriverObject->DriverExtension->AddDevice = KsAddDevice;

    /* KS handles these */
    DPRINT1("Setting KS function handlers\n");
    KsSetMajorFunctionHandler(DriverObject, IRP_MJ_CLOSE);
    KsSetMajorFunctionHandler(DriverObject, IRP_MJ_DEVICE_CONTROL);


    return STATUS_SUCCESS;
}
