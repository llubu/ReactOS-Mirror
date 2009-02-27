/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/sysaudio/deviface.c
 * PURPOSE:         System Audio graph builder
 * PROGRAMMER:      Johannes Anderwald
 */

#include <ntifs.h>
#include <ntddk.h>
#include <portcls.h>
#include <ks.h>
#include <ksmedia.h>
#include <math.h>
#define YDEBUG
#include <debug.h>
#include "sysaudio.h"

const GUID GUID_DEVICE_INTERFACE_ARRIVAL       = {0xCB3A4004L, 0x46F0, 0x11D0, {0xB0, 0x8F, 0x00, 0x60, 0x97, 0x13, 0x05, 0x3F}};
const GUID GUID_DEVICE_INTERFACE_REMOVAL       = {0xCB3A4005L, 0x46F0, 0x11D0, {0xB0, 0x8F, 0x00, 0x60, 0x97, 0x13, 0x05, 0x3F}};
const GUID KS_CATEGORY_AUDIO                   = {0x6994AD04L, 0x93EF, 0x11D0, {0xA3, 0xCC, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}};
const GUID DMOCATEGORY_ACOUSTIC_ECHO_CANCEL    = {0xBF963D80L, 0xC559, 0x11D0, {0x8A, 0x2B, 0x00, 0xA0, 0xC9, 0x25, 0x5A, 0xC1}};


NTSTATUS
NTAPI
DeviceInterfaceChangeCallback(
    IN PVOID NotificationStructure,
    IN PVOID Context)
{
    DEVICE_INTERFACE_CHANGE_NOTIFICATION * Event;
    SYSAUDIODEVEXT *DeviceExtension = (SYSAUDIODEVEXT*)Context;
    NTSTATUS Status = STATUS_SUCCESS;

    Event = (DEVICE_INTERFACE_CHANGE_NOTIFICATION*)NotificationStructure;

    if (IsEqualGUIDAligned(&Event->Event,
                           &GUID_DEVICE_INTERFACE_ARRIVAL))
    {
        /* a new device has arrived */

        PFILE_OBJECT FileObject = NULL;
        PKSAUDIO_DEVICE_ENTRY DeviceEntry;
        HANDLE NodeHandle;
        IO_STATUS_BLOCK IoStatusBlock;
        OBJECT_ATTRIBUTES ObjectAttributes;


        DeviceEntry = ExAllocatePool(NonPagedPool, sizeof(KSAUDIO_DEVICE_ENTRY));
        if (!DeviceEntry)
        {
            DPRINT1("No Mem\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlZeroMemory(DeviceEntry, sizeof(KSAUDIO_DEVICE_ENTRY));
        DeviceEntry->DeviceName.Length = 0;
        DeviceEntry->DeviceName.MaximumLength = Event->SymbolicLinkName->Length + 5 * sizeof(WCHAR);
        DeviceEntry->DeviceName.Buffer = ExAllocatePool(NonPagedPool, DeviceEntry->DeviceName.MaximumLength);
        if (!DeviceEntry->DeviceName.Buffer)
        {
            DPRINT1("No Mem\n");
            ExFreePool(DeviceEntry);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        if (!NT_SUCCESS(RtlAppendUnicodeToString(&DeviceEntry->DeviceName, L"\\??\\")))
        {
            DPRINT1("No Mem\n");
            ExFreePool(DeviceEntry->DeviceName.Buffer);
            ExFreePool(DeviceEntry);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        if (!NT_SUCCESS(RtlAppendUnicodeStringToString(&DeviceEntry->DeviceName, Event->SymbolicLinkName)))
        {
            DPRINT1("No Mem\n");
            ExFreePool(DeviceEntry->DeviceName.Buffer);
            ExFreePool(DeviceEntry);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        DPRINT1("Sym %wZ\n", &DeviceEntry->DeviceName);

        InitializeObjectAttributes(&ObjectAttributes, &DeviceEntry->DeviceName, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);

        Status = ZwCreateFile(&NodeHandle,
                              GENERIC_READ | GENERIC_WRITE,
                              &ObjectAttributes,
                              &IoStatusBlock,
                              NULL,
                              0,
                              0,
                              FILE_OPEN,
                              FILE_SYNCHRONOUS_IO_NONALERT,
                              NULL,
                              0);


        if (!NT_SUCCESS(Status))
        {
            DPRINT1("ZwCreateFile failed with %x\n", Status);
            ExFreePool(DeviceEntry);
            return Status;
        }

        Status = ObReferenceObjectByHandle(NodeHandle, GENERIC_READ | GENERIC_WRITE, IoFileObjectType, KernelMode, (PVOID*)&FileObject, NULL);
        if (!NT_SUCCESS(Status))
        {
            ZwClose(NodeHandle);
            ExFreePool(DeviceEntry);
            DPRINT1("ObReferenceObjectByHandle failed with %x\n", Status);
            return Status;
        }

        DeviceEntry->Handle = NodeHandle;
        DeviceEntry->FileObject = FileObject;

        InsertTailList(&DeviceExtension->KsAudioDeviceList, &DeviceEntry->Entry);
        DeviceExtension->NumberOfKsAudioDevices++;

        DPRINT1("Successfully opened audio device handle %p file object %p device object %p\n", NodeHandle, FileObject, FileObject->DeviceObject);
        return Status;
    }
    else if (IsEqualGUIDAligned(&Event->Event,
                                &GUID_DEVICE_INTERFACE_REMOVAL))
    {
        DPRINT1("Remove interface to audio device!\n");
        ///FIXME
        ///
        return STATUS_SUCCESS;
    }
    else
    {
        UNICODE_STRING EventName, InterfaceGuid;

        RtlStringFromGUID(&Event->Event, &EventName);
        RtlStringFromGUID(&Event->InterfaceClassGuid, &InterfaceGuid);
        DPRINT1("Unknown event: Event %wZ GUID %wZ\n", &EventName, &InterfaceGuid);
        return STATUS_SUCCESS;
    }

}

NTSTATUS
SysAudioRegisterNotifications(
    IN  PDRIVER_OBJECT  DriverObject,
    SYSAUDIODEVEXT *DeviceExtension)
{
    NTSTATUS Status;


    Status = IoRegisterPlugPlayNotification(EventCategoryDeviceInterfaceChange,
                                            PNPNOTIFY_DEVICE_INTERFACE_INCLUDE_EXISTING_INTERFACES,
                                            (PVOID)&KS_CATEGORY_AUDIO,
                                            DriverObject,
                                            DeviceInterfaceChangeCallback,
                                            (PVOID)DeviceExtension,
                                            (PVOID*)&DeviceExtension->KsAudioNotificationEntry);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IoRegisterPlugPlayNotification failed with %x\n", Status);
    }

    Status = IoRegisterPlugPlayNotification(EventCategoryDeviceInterfaceChange,
                                            PNPNOTIFY_DEVICE_INTERFACE_INCLUDE_EXISTING_INTERFACES,
                                            (PVOID)&DMOCATEGORY_ACOUSTIC_ECHO_CANCEL,
                                            DriverObject,
                                            DeviceInterfaceChangeCallback,
                                            (PVOID)DeviceExtension,
                                            (PVOID*)&DeviceExtension->EchoCancelNotificationEntry);

    if (!NT_SUCCESS(Status))
    {
        /* ignore failure for now */
        DPRINT1("IoRegisterPlugPlayNotification failed for DMOCATEGORY_ACOUSTIC_ECHO_CANCEL\n", Status);
    }

    return STATUS_SUCCESS;
}



NTSTATUS
SysAudioRegisterDeviceInterfaces(
    IN PDEVICE_OBJECT DeviceObject)
{
    NTSTATUS Status;
    UNICODE_STRING SymbolicLink;

    Status = IoRegisterDeviceInterface(DeviceObject, &KSCATEGORY_PREFERRED_MIDIOUT_DEVICE, NULL, &SymbolicLink);
    if (NT_SUCCESS(Status))
    {
        IoSetDeviceInterfaceState(&SymbolicLink, TRUE);
        RtlFreeUnicodeString(&SymbolicLink);
    }
    else
    {
        DPRINT1("Failed to register KSCATEGORY_PREFERRED_MIDIOUT_DEVICE interface Status %x\n", Status);
        return Status;
    }

    Status = IoRegisterDeviceInterface(DeviceObject, &KSCATEGORY_PREFERRED_WAVEIN_DEVICE, NULL, &SymbolicLink);
    if (NT_SUCCESS(Status))
    {
        IoSetDeviceInterfaceState(&SymbolicLink, TRUE);
        RtlFreeUnicodeString(&SymbolicLink);
    }
    else
    {
        DPRINT1("Failed to register KSCATEGORY_PREFERRED_WAVEIN_DEVICE interface Status %x\n", Status);
        return Status;
    }

    Status = IoRegisterDeviceInterface(DeviceObject, &KSCATEGORY_PREFERRED_WAVEOUT_DEVICE, NULL, &SymbolicLink);
    if (NT_SUCCESS(Status))
    {
        IoSetDeviceInterfaceState(&SymbolicLink, TRUE);
        RtlFreeUnicodeString(&SymbolicLink);
    }
    else
    {
        DPRINT1("Failed to register KSCATEGORY_PREFERRED_WAVEOUT_DEVICE interface Status %x\n", Status);
    }

    Status = IoRegisterDeviceInterface(DeviceObject, &KSCATEGORY_SYSAUDIO, NULL, &SymbolicLink);
    if (NT_SUCCESS(Status))
    {
        IoSetDeviceInterfaceState(&SymbolicLink, TRUE);
        RtlFreeUnicodeString(&SymbolicLink);
    }
    else
    {
        DPRINT1("Failed to register KSCATEGORY_SYSAUDIO interface Status %x\n", Status);
    }

    return Status;
}

