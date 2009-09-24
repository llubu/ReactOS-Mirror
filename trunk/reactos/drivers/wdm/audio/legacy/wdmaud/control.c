/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/legacy/wdmaud/deviface.c
 * PURPOSE:         System Audio graph builder
 * PROGRAMMER:      Andrew Greenwood
 *                  Johannes Anderwald
 */
#include "wdmaud.h"

const GUID KSPROPSETID_Pin                     = {0x8C134960L, 0x51AD, 0x11CF, {0x87, 0x8A, 0x94, 0xF8, 0x01, 0xC1, 0x00, 0x00}};
const GUID KSPROPSETID_Connection               = {0x1D58C920L, 0xAC9B, 0x11CF, {0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00}};
const GUID KSPROPSETID_Sysaudio                 = {0xCBE3FAA0L, 0xCC75, 0x11D0, {0xB4, 0x65, 0x00, 0x00, 0x1A, 0x18, 0x18, 0xE6}};
const GUID KSPROPSETID_General                  = {0x1464EDA5L, 0x6A8F, 0x11D1, {0x9A, 0xA7, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}};
const GUID KSINTERFACESETID_Standard            = {0x1A8766A0L, 0x62CE, 0x11CF, {0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00}};
const GUID KSMEDIUMSETID_Standard               = {0x4747B320L, 0x62CE, 0x11CF, {0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00}};
const GUID KSDATAFORMAT_TYPE_AUDIO              = {0x73647561L, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};
const GUID KSDATAFORMAT_SUBTYPE_PCM             = {0x00000001L, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};
const GUID KSDATAFORMAT_SPECIFIER_WAVEFORMATEX  = {0x05589f81L, 0xc356, 0x11ce, {0xbf, 0x01, 0x00, 0xaa, 0x00, 0x55, 0x59, 0x5a}};
const GUID KSPROPSETID_Topology                 = {0x720D4AC0L, 0x7533, 0x11D0, {0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00}};

NTSTATUS
SetIrpIoStatus(
    IN PIRP Irp,
    IN NTSTATUS Status,
    IN ULONG Length)
{
    Irp->IoStatus.Information = Length;
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;

}

NTSTATUS
GetFilterIdAndPinId(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PWDMAUD_DEVICE_INFO DeviceInfo,
    IN  PWDMAUD_CLIENT ClientInfo,
    IN PULONG FilterId,
    IN PULONG PinId)
{
    KSP_PIN Pin;
    ULONG Count, BytesReturned, Index, SubIndex, Result, NumPins;
    NTSTATUS Status;
    KSPIN_COMMUNICATION Communication;
    KSPIN_DATAFLOW DataFlow;
    PWDMAUD_DEVICE_EXTENSION DeviceExtension;

    if (DeviceInfo->DeviceType != WAVE_OUT_DEVICE_TYPE && DeviceInfo->DeviceType != WAVE_IN_DEVICE_TYPE)
    {
        DPRINT1("FIXME: Unsupported device type %x\n", DeviceInfo->DeviceType);
        return STATUS_UNSUCCESSFUL;
    }

    Pin.Property.Set = KSPROPSETID_Sysaudio;
    Pin.Property.Id = KSPROPERTY_SYSAUDIO_DEVICE_COUNT;
    Pin.Property.Flags = KSPROPERTY_TYPE_GET;

    DeviceExtension = (PWDMAUD_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    Status = KsSynchronousIoControlDevice(DeviceExtension->FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&Pin, sizeof(KSPROPERTY), (PVOID)&Count, sizeof(ULONG), &BytesReturned);
    if (!NT_SUCCESS(Status))
        return STATUS_UNSUCCESSFUL;

    Result = 0;
    for(Index = 0; Index < Count; Index++)
    {
        /* query number of pins */
        Pin.Reserved = Index; // see sysaudio
        Pin.Property.Flags = KSPROPERTY_TYPE_GET;
        Pin.Property.Set = KSPROPSETID_Pin;
        Pin.Property.Id = KSPROPERTY_PIN_CTYPES;
        Pin.PinId = 0;

        Status = KsSynchronousIoControlDevice(DeviceExtension->FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&Pin, sizeof(KSP_PIN), (PVOID)&NumPins, sizeof(ULONG), &BytesReturned);
        if (NT_SUCCESS(Status))
        {
            /* enumerate now all pins */
            for(SubIndex = 0; SubIndex < NumPins; SubIndex++)
            {
                Pin.PinId = SubIndex;
                Pin.Property.Id = KSPROPERTY_PIN_COMMUNICATION;
                Communication = KSPIN_COMMUNICATION_NONE;

                /* get pin communication type */
                KsSynchronousIoControlDevice(DeviceExtension->FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&Pin, sizeof(KSP_PIN), (PVOID)&Communication, sizeof(KSPIN_COMMUNICATION), &BytesReturned);

                Pin.Property.Id = KSPROPERTY_PIN_DATAFLOW;
                DataFlow = 0;

                /* get pin dataflow type */
                KsSynchronousIoControlDevice(DeviceExtension->FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&Pin, sizeof(KSP_PIN), (PVOID)&DataFlow, sizeof(KSPIN_DATAFLOW), &BytesReturned);

                if (DeviceInfo->DeviceType == WAVE_OUT_DEVICE_TYPE)
                {
                    if (Communication == KSPIN_COMMUNICATION_SINK && DataFlow == KSPIN_DATAFLOW_IN)
                    {
                        if(DeviceInfo->DeviceIndex == Result)
                        {
                            /* found the index */
                            *FilterId = Index;
                            *PinId = SubIndex;
                            return STATUS_SUCCESS;
                        }

                        Result++;
                    }
                }
                else if (DeviceInfo->DeviceType == WAVE_IN_DEVICE_TYPE)
                {
                    if (Communication == KSPIN_COMMUNICATION_SINK && DataFlow == KSPIN_DATAFLOW_OUT)
                    {
                        if(DeviceInfo->DeviceIndex == Result)
                        {
                            /* found the index */
                            *FilterId = Index;
                            *PinId = SubIndex;
                            return STATUS_SUCCESS;
                        }
                        Result++;
                    }
                }
            }
        }
    }

    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
WdmAudControlOpen(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp,
    IN  PWDMAUD_DEVICE_INFO DeviceInfo,
    IN  PWDMAUD_CLIENT ClientInfo)
{
    SYSAUDIO_INSTANCE_INFO InstanceInfo;
    PWDMAUD_DEVICE_EXTENSION DeviceExtension;
    ULONG BytesReturned;
    NTSTATUS Status;
    ACCESS_MASK DesiredAccess = 0;
    HANDLE PinHandle;
    KSPIN_CONNECT * PinConnect;
    ULONG Length, Index;
    KSDATAFORMAT_WAVEFORMATEX * DataFormat;
    ULONG FilterId;
    ULONG PinId;
    ULONG FreeIndex;

    if (DeviceInfo->DeviceType == MIXER_DEVICE_TYPE)
    {
        return WdmAudControlOpenMixer(DeviceObject, Irp, DeviceInfo, ClientInfo);
    }

    if (DeviceInfo->DeviceType != WAVE_OUT_DEVICE_TYPE && DeviceInfo->DeviceType != WAVE_IN_DEVICE_TYPE)
    {
        DPRINT1("FIXME: only waveout / wavein devices are supported\n");
        return SetIrpIoStatus(Irp, STATUS_UNSUCCESSFUL, 0);
    }

    if (DeviceInfo->u.WaveFormatEx.wFormatTag != WAVE_FORMAT_PCM)
    {
        DPRINT("FIXME: Only WAVE_FORMAT_PCM is supported RequestFormat %x\n", DeviceInfo->u.WaveFormatEx.wFormatTag);
        return SetIrpIoStatus(Irp, STATUS_UNSUCCESSFUL, 0);
    }

    Status = GetFilterIdAndPinId(DeviceObject, DeviceInfo, ClientInfo, &FilterId, &PinId);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Invalid device index %u\n", DeviceInfo->DeviceIndex);
        return SetIrpIoStatus(Irp, STATUS_UNSUCCESSFUL, 0);
    }

    /* close pin handle which uses same virtual audio device id and pin id */
    FreeIndex = (ULONG)-1;
    for(Index = 0; Index < ClientInfo->NumPins; Index++)
    {
        if (ClientInfo->hPins[Index].FilterId == FilterId && ClientInfo->hPins[Index].PinId == PinId && ClientInfo->hPins[Index].Handle && ClientInfo->hPins[Index].Type == DeviceInfo->DeviceType)
        {
            ZwClose(ClientInfo->hPins[Index].Handle);
            ClientInfo->hPins[Index].Handle = NULL;
            FreeIndex = Index;
        }
    }


    Length = sizeof(KSDATAFORMAT_WAVEFORMATEX) + sizeof(KSPIN_CONNECT);
    PinConnect = ExAllocatePool(NonPagedPool, Length);
    if (!PinConnect)
    {
        /* no memory */
        return SetIrpIoStatus(Irp, STATUS_NO_MEMORY, 0);
    }

    DeviceExtension = (PWDMAUD_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    if (DeviceInfo->DeviceType == WAVE_IN_DEVICE_TYPE ||
        DeviceInfo->DeviceType == MIDI_IN_DEVICE_TYPE ||
        DeviceInfo->DeviceType == MIXER_DEVICE_TYPE)
    {
        DesiredAccess |= GENERIC_READ;
    }

    if (DeviceInfo->DeviceType == WAVE_OUT_DEVICE_TYPE ||
        DeviceInfo->DeviceType == MIDI_OUT_DEVICE_TYPE ||
        DeviceInfo->DeviceType == AUX_DEVICE_TYPE ||
        DeviceInfo->DeviceType == MIXER_DEVICE_TYPE)
    {
        DesiredAccess |= GENERIC_WRITE;
    }

    PinConnect->Interface.Set = KSINTERFACESETID_Standard;
    PinConnect->Interface.Id = KSINTERFACE_STANDARD_STREAMING;
    PinConnect->Interface.Flags = 0;
    PinConnect->Medium.Set = KSMEDIUMSETID_Standard;
    PinConnect->Medium.Id = KSMEDIUM_TYPE_ANYINSTANCE;
    PinConnect->Medium.Flags = 0;
    PinConnect->PinToHandle = NULL;
    PinConnect->PinId = PinId;
    PinConnect->Priority.PriorityClass = KSPRIORITY_NORMAL;
    PinConnect->Priority.PrioritySubClass = 1;


    DataFormat = (KSDATAFORMAT_WAVEFORMATEX*) (PinConnect + 1);
    DataFormat->WaveFormatEx.wFormatTag = DeviceInfo->u.WaveFormatEx.wFormatTag;
    DataFormat->WaveFormatEx.nChannels = DeviceInfo->u.WaveFormatEx.nChannels;
    DataFormat->WaveFormatEx.nSamplesPerSec = DeviceInfo->u.WaveFormatEx.nSamplesPerSec;
    DataFormat->WaveFormatEx.nBlockAlign = DeviceInfo->u.WaveFormatEx.nBlockAlign;
    DataFormat->WaveFormatEx.nAvgBytesPerSec = DeviceInfo->u.WaveFormatEx.nAvgBytesPerSec;
    DataFormat->WaveFormatEx.wBitsPerSample = DeviceInfo->u.WaveFormatEx.wBitsPerSample;
    DataFormat->WaveFormatEx.cbSize = 0;
    DataFormat->DataFormat.FormatSize = sizeof(KSDATAFORMAT) + sizeof(WAVEFORMATEX);
    DataFormat->DataFormat.Flags = 0;
    DataFormat->DataFormat.Reserved = 0;
    DataFormat->DataFormat.MajorFormat = KSDATAFORMAT_TYPE_AUDIO;

    DataFormat->DataFormat.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
    DataFormat->DataFormat.Specifier = KSDATAFORMAT_SPECIFIER_WAVEFORMATEX;
    DataFormat->DataFormat.SampleSize = 4;

    /* setup property request */
    InstanceInfo.Property.Set = KSPROPSETID_Sysaudio;
    InstanceInfo.Property.Id = KSPROPERTY_SYSAUDIO_INSTANCE_INFO;
    InstanceInfo.Property.Flags = KSPROPERTY_TYPE_SET;
    InstanceInfo.Flags = 0;
    InstanceInfo.DeviceNumber = FilterId;

    /* first open the virtual device */
    Status = KsSynchronousIoControlDevice(DeviceExtension->FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&InstanceInfo, sizeof(SYSAUDIO_INSTANCE_INFO), NULL, 0, &BytesReturned);

    if (!NT_SUCCESS(Status))
    {
        /* failed */
        ExFreePool(PinConnect);
        return SetIrpIoStatus(Irp, Status, sizeof(WDMAUD_DEVICE_INFO));
    }

    /* now create the pin */
    Status = KsCreatePin(DeviceExtension->hSysAudio, PinConnect, DesiredAccess, &PinHandle);

    /* free create info */
    ExFreePool(PinConnect);

    if (NT_SUCCESS(Status))
    {
        PWDMAUD_HANDLE Handels;

        if (FreeIndex != (ULONG)-1)
        {
            /* re-use a free index */
            ClientInfo->hPins[Index].Handle = PinHandle;
            ClientInfo->hPins[Index].FilterId = FilterId;
            ClientInfo->hPins[Index].PinId = PinId;
            ClientInfo->hPins[Index].Type = DeviceInfo->DeviceType;

            DeviceInfo->hDevice = PinHandle;
            return SetIrpIoStatus(Irp, Status, sizeof(WDMAUD_DEVICE_INFO));
        }

        Handels = ExAllocatePool(NonPagedPool, sizeof(WDMAUD_HANDLE) * (ClientInfo->NumPins+1));

        if (Handels)
        {
            if (ClientInfo->NumPins)
            {
                RtlMoveMemory(Handels, ClientInfo->hPins, sizeof(WDMAUD_HANDLE) * ClientInfo->NumPins);
                ExFreePool(ClientInfo->hPins);
            }

            ClientInfo->hPins = Handels;
            ClientInfo->hPins[ClientInfo->NumPins].Handle = PinHandle;
            ClientInfo->hPins[ClientInfo->NumPins].Type = DeviceInfo->DeviceType;
            ClientInfo->hPins[ClientInfo->NumPins].FilterId = FilterId;
            ClientInfo->hPins[ClientInfo->NumPins].PinId = PinId;
            ClientInfo->NumPins++;
        }
        DeviceInfo->hDevice = PinHandle;
    }
    else
    {
        DeviceInfo->hDevice = NULL;
    }

    return SetIrpIoStatus(Irp, Status, sizeof(WDMAUD_DEVICE_INFO));
}

NTSTATUS
WdmAudControlDeviceType(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp,
    IN  PWDMAUD_DEVICE_INFO DeviceInfo,
    IN  PWDMAUD_CLIENT ClientInfo)
{
    KSP_PIN Pin;
    ULONG Count, BytesReturned, Index, SubIndex, Result, NumPins;
    NTSTATUS Status;
    KSPIN_COMMUNICATION Communication;
    KSPIN_DATAFLOW DataFlow;
    PWDMAUD_DEVICE_EXTENSION DeviceExtension;

    if (DeviceInfo->DeviceType == MIXER_DEVICE_TYPE)
    {
        DeviceInfo->DeviceCount = GetNumOfMixerDevices(DeviceObject);
        return SetIrpIoStatus(Irp, STATUS_SUCCESS, sizeof(WDMAUD_DEVICE_INFO));
    }

    if (DeviceInfo->DeviceType != WAVE_OUT_DEVICE_TYPE && DeviceInfo->DeviceType != WAVE_IN_DEVICE_TYPE)
    {
        DPRINT("FIXME: Unsupported device type %x\n", DeviceInfo->DeviceType);
        DeviceInfo->DeviceCount = 0;
        return SetIrpIoStatus(Irp, STATUS_SUCCESS, sizeof(WDMAUD_DEVICE_INFO));
    }

    Pin.Property.Set = KSPROPSETID_Sysaudio;
    Pin.Property.Id = KSPROPERTY_SYSAUDIO_DEVICE_COUNT;
    Pin.Property.Flags = KSPROPERTY_TYPE_GET;

    DeviceExtension = (PWDMAUD_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    Status = KsSynchronousIoControlDevice(DeviceExtension->FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&Pin, sizeof(KSPROPERTY), (PVOID)&Count, sizeof(ULONG), &BytesReturned);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("KSPROPERTY_SYSAUDIO_DEVICE_COUNT failed with %x\n", Status);
        return SetIrpIoStatus(Irp, Status, sizeof(WDMAUD_DEVICE_INFO));
    }
    Result = 0;
    /* now enumerate all available filters */
    for(Index = 0; Index < Count; Index++)
    {
        /* query number of pins */
        Pin.Reserved = Index; // see sysaudio
        Pin.Property.Flags = KSPROPERTY_TYPE_GET;
        Pin.Property.Set = KSPROPSETID_Pin;
        Pin.Property.Id = KSPROPERTY_PIN_CTYPES;
        Pin.PinId = 0;

        Status = KsSynchronousIoControlDevice(DeviceExtension->FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&Pin, sizeof(KSP_PIN), (PVOID)&NumPins, sizeof(ULONG), &BytesReturned);
        if (NT_SUCCESS(Status))
        {
            /* enumerate now all pins */
            for(SubIndex = 0; SubIndex < NumPins; SubIndex++)
            {
                Pin.PinId = SubIndex;
                Pin.Property.Id = KSPROPERTY_PIN_COMMUNICATION;
                Communication = KSPIN_COMMUNICATION_NONE;

                /* get pin communication type */
                Status = KsSynchronousIoControlDevice(DeviceExtension->FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&Pin, sizeof(KSP_PIN), (PVOID)&Communication, sizeof(KSPIN_COMMUNICATION), &BytesReturned);
                if (!NT_SUCCESS(Status))
                    continue;

                Pin.Property.Id = KSPROPERTY_PIN_DATAFLOW;
                DataFlow = 0;

                /* get pin dataflow type */
                Status = KsSynchronousIoControlDevice(DeviceExtension->FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&Pin, sizeof(KSP_PIN), (PVOID)&DataFlow, sizeof(KSPIN_DATAFLOW), &BytesReturned);
                if (!NT_SUCCESS(Status))
                    continue;

                if (DeviceInfo->DeviceType == WAVE_OUT_DEVICE_TYPE)
                {
                    if (Communication == KSPIN_COMMUNICATION_SINK && DataFlow == KSPIN_DATAFLOW_IN)
                        Result++;
                }
                else if (DeviceInfo->DeviceType == WAVE_IN_DEVICE_TYPE)
                {
                    if (Communication == KSPIN_COMMUNICATION_SINK && DataFlow == KSPIN_DATAFLOW_OUT)
                        Result++;
                }
            }
        }
    }

    /* store result count */
    DeviceInfo->DeviceCount = Result;

    DPRINT("WdmAudControlDeviceType Status %x Devices %u\n", Status, DeviceInfo->DeviceCount);
    return SetIrpIoStatus(Irp, STATUS_SUCCESS, sizeof(WDMAUD_DEVICE_INFO));
}

NTSTATUS
WdmAudControlDeviceState(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp,
    IN  PWDMAUD_DEVICE_INFO DeviceInfo,
    IN  PWDMAUD_CLIENT ClientInfo)
{
    KSPROPERTY Property;
    KSSTATE State;
    NTSTATUS Status;
    ULONG BytesReturned;
    PFILE_OBJECT FileObject;

    //DPRINT1("WdmAudControlDeviceState\n");

    Status = ObReferenceObjectByHandle(DeviceInfo->hDevice, GENERIC_READ | GENERIC_WRITE, IoFileObjectType, KernelMode, (PVOID*)&FileObject, NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Error: invalid device handle provided %p Type %x\n", DeviceInfo->hDevice, DeviceInfo->DeviceType);
        return SetIrpIoStatus(Irp, STATUS_UNSUCCESSFUL, 0);
    }

    Property.Set = KSPROPSETID_Connection;
    Property.Id = KSPROPERTY_CONNECTION_STATE;
    Property.Flags = KSPROPERTY_TYPE_SET;

    State = DeviceInfo->u.State;

    Status = KsSynchronousIoControlDevice(FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSPROPERTY), (PVOID)&State, sizeof(KSSTATE), &BytesReturned);

    ObDereferenceObject(FileObject);

    //DPRINT1("WdmAudControlDeviceState Status %x\n", Status);
    return SetIrpIoStatus(Irp, Status, sizeof(WDMAUD_DEVICE_INFO));
}

ULONG
CheckFormatSupport(
    IN PKSDATARANGE_AUDIO DataRangeAudio,
    ULONG SampleFrequency,
    ULONG Mono8Bit,
    ULONG Stereo8Bit,
    ULONG Mono16Bit,
    ULONG Stereo16Bit)
{
    ULONG Result = 0;

    if (DataRangeAudio->MinimumSampleFrequency <= SampleFrequency && DataRangeAudio->MaximumSampleFrequency >= SampleFrequency)
    {
        if (DataRangeAudio->MinimumBitsPerSample <= 8 && DataRangeAudio->MaximumBitsPerSample >= 8)
        {
            Result |= Mono8Bit;
            if (DataRangeAudio->MaximumChannels >= 2)
            {
                Result |= Stereo8Bit;
            }
        }

        if (DataRangeAudio->MinimumBitsPerSample <= 16 && DataRangeAudio->MaximumBitsPerSample >= 16)
        {
            Result |= Mono16Bit;
            if (DataRangeAudio->MaximumChannels >= 2)
            {
                Result |= Stereo8Bit;
            }
        }
    }
    return Result;

}

PKEY_VALUE_PARTIAL_INFORMATION
ReadKeyValue(
    IN HANDLE hSubKey,
    IN PUNICODE_STRING KeyName)
{
    NTSTATUS Status;
    ULONG Length;
    PKEY_VALUE_PARTIAL_INFORMATION PartialInformation;

    /* now query MatchingDeviceId key */
    Status = ZwQueryValueKey(hSubKey, KeyName, KeyValuePartialInformation, NULL, 0, &Length);

    /* check for success */
    if (Status != STATUS_BUFFER_TOO_SMALL)
        return NULL;

    /* allocate a buffer for key data */
    PartialInformation = ExAllocatePool(NonPagedPool, Length);

    if (!PartialInformation)
        return NULL;


    /* now query MatchingDeviceId key */
    Status = ZwQueryValueKey(hSubKey, KeyName, KeyValuePartialInformation, PartialInformation, Length, &Length);

    /* check for success */
    if (!NT_SUCCESS(Status))
    {
        ExFreePool(PartialInformation);
        return NULL;
    }

    if (PartialInformation->Type != REG_SZ)
    {
        /* invalid key type */
        ExFreePool(PartialInformation);
        return NULL;
    }

    return PartialInformation;
}


NTSTATUS
CompareProductName(
    IN HANDLE hSubKey,
    IN LPWSTR PnpName,
    IN ULONG ProductNameSize,
    OUT LPWSTR ProductName)
{
    PKEY_VALUE_PARTIAL_INFORMATION PartialInformation;
    UNICODE_STRING DriverDescName = RTL_CONSTANT_STRING(L"DriverDesc");
    UNICODE_STRING MatchingDeviceIdName = RTL_CONSTANT_STRING(L"MatchingDeviceId");
    ULONG Length;
    LPWSTR DeviceName;

    /* read MatchingDeviceId value */
    PartialInformation = ReadKeyValue(hSubKey, &MatchingDeviceIdName);

    if (!PartialInformation)
        return STATUS_UNSUCCESSFUL;


    /* extract last '&' */
    DeviceName = wcsrchr((LPWSTR)PartialInformation->Data, L'&');
    ASSERT(DeviceName);
    /* terminate it */
    DeviceName[0] = L'\0';

    Length = wcslen((LPWSTR)PartialInformation->Data);

    DPRINT("DeviceName %S PnpName %S Length %u\n", (LPWSTR)PartialInformation->Data, PnpName, Length);

    if (_wcsnicmp((LPWSTR)PartialInformation->Data, &PnpName[4], Length))
    {
        ExFreePool(PartialInformation);
        return STATUS_NO_MATCH;
    }

    /* free buffer */
    ExFreePool(PartialInformation);

    /* read DriverDescName value */
    PartialInformation = ReadKeyValue(hSubKey, &DriverDescName);

    if (!PartialInformation)
    {
        /* failed to read driver desc key */
        return STATUS_UNSUCCESSFUL;
    }

    /* copy key name */
    Length = min(ProductNameSize * sizeof(WCHAR), PartialInformation->DataLength);
    RtlMoveMemory(ProductName, (PVOID)PartialInformation->Data, Length);

    /* zero terminate it */
    ProductName[ProductNameSize-1] = L'\0';

    /* free buffer */
    ExFreePool(PartialInformation);

    return STATUS_SUCCESS;
}



NTSTATUS
FindProductName(
    IN LPWSTR PnpName,
    IN ULONG ProductNameSize,
    OUT LPWSTR ProductName)
{
    UNICODE_STRING KeyName = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Class\\{4D36E96C-E325-11CE-BFC1-08002BE10318}");

    UNICODE_STRING SubKeyName;
    WCHAR SubKey[20];
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE hKey, hSubKey;
    NTSTATUS Status;
    ULONG Length, Index;
    PKEY_FULL_INFORMATION KeyInformation;

    for(Index = 0; Index < wcslen(PnpName); Index++)
    {
        if (PnpName[Index] == '#')
            PnpName[Index] = L'\\';
    }


    /* initialize key attributes */
    InitializeObjectAttributes(&ObjectAttributes, &KeyName, OBJ_CASE_INSENSITIVE | OBJ_OPENIF, NULL, NULL);

    /* open the key */
    Status = ZwOpenKey(&hKey, GENERIC_READ, &ObjectAttributes);

    /* check for success */
    if (!NT_SUCCESS(Status))
        return Status;

    /* query num of subkeys */
    Status = ZwQueryKey(hKey, KeyFullInformation, NULL, 0, &Length);

    if (Status != STATUS_BUFFER_TOO_SMALL)
    {
        DPRINT1("ZwQueryKey failed with %x\n", Status);
        /* failed */
        ZwClose(hKey);
        return Status;
    }

    /* allocate key information struct */
    KeyInformation = ExAllocatePool(NonPagedPool, Length);
    if (!KeyInformation)
    {
        /* no memory */
        ZwClose(hKey);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* query num of subkeys */
    Status = ZwQueryKey(hKey, KeyFullInformation, (PVOID)KeyInformation, Length, &Length);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ZwQueryKey failed with %x\n", Status);
        ExFreePool(KeyInformation);
        ZwClose(hKey);
        return Status;
    }

    /* now iterate through all subkeys */
    for(Index = 0; Index < KeyInformation->SubKeys; Index++)
    {
        /* subkeys are always in the format 0000-XXXX */
        swprintf(SubKey, L"%04u", Index);

        /* initialize subkey name */
        RtlInitUnicodeString(&SubKeyName, SubKey);

        /* initialize key attributes */
        InitializeObjectAttributes(&ObjectAttributes, &SubKeyName, OBJ_CASE_INSENSITIVE | OBJ_OPENIF, hKey, NULL);

        /* open the sub key */
        Status = ZwOpenKey(&hSubKey, GENERIC_READ, &ObjectAttributes);

        /* check for success */
        if (NT_SUCCESS(Status))
        {
            /* compare product name */
            Status = CompareProductName(hSubKey, PnpName, ProductNameSize, ProductName);

            /* close subkey */
            ZwClose(hSubKey);

            if (NT_SUCCESS(Status))
                break;
        }
    }

    /* free buffer */
    ExFreePool(KeyInformation);

    /* close key */
    ZwClose(hKey);

    /* no matching key found */
    return Status;
}



NTSTATUS
WdmAudCapabilities(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp,
    IN  PWDMAUD_DEVICE_INFO DeviceInfo,
    IN  PWDMAUD_CLIENT ClientInfo)
{
    PWDMAUD_DEVICE_EXTENSION DeviceExtension;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    KSP_PIN PinProperty;
    KSCOMPONENTID ComponentId;
    KSMULTIPLE_ITEM * MultipleItem;
    ULONG BytesReturned;
    PKSDATARANGE_AUDIO DataRangeAudio;
    PKSDATARANGE DataRange;
    ULONG Index;
    ULONG wChannels = 0;
    ULONG dwFormats = 0;
    ULONG dwSupport = 0;
    ULONG FilterId;
    ULONG PinId;
    WCHAR DeviceName[MAX_PATH];

    DPRINT("WdmAudCapabilities entered\n");

    if (DeviceInfo->DeviceType == MIXER_DEVICE_TYPE)
    {
        Status = WdmAudMixerCapabilities(DeviceObject, DeviceInfo, ClientInfo);
        return SetIrpIoStatus(Irp, Status, sizeof(WDMAUD_DEVICE_INFO));
    }


    Status = GetFilterIdAndPinId(DeviceObject, DeviceInfo, ClientInfo, &FilterId, &PinId);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Invalid device index provided %u\n", DeviceInfo->DeviceIndex);
        return SetIrpIoStatus(Irp, STATUS_INVALID_PARAMETER, 0);
    }

    PinProperty.PinId = FilterId;
    PinProperty.Property.Set = KSPROPSETID_Sysaudio;
    PinProperty.Property.Id = KSPROPERTY_SYSAUDIO_COMPONENT_ID;
    PinProperty.Property.Flags = KSPROPERTY_TYPE_GET;

    RtlZeroMemory(&ComponentId, sizeof(KSCOMPONENTID));

    DeviceExtension = (PWDMAUD_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    Status = KsSynchronousIoControlDevice(DeviceExtension->FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&PinProperty, sizeof(KSP_PIN), (PVOID)&ComponentId, sizeof(KSCOMPONENTID), &BytesReturned);
    if (NT_SUCCESS(Status))
    {
        DeviceInfo->u.WaveOutCaps.wMid = ComponentId.Manufacturer.Data1 - 0xd5a47fa7;
        DeviceInfo->u.WaveOutCaps.vDriverVersion = MAKELONG(ComponentId.Version, ComponentId.Revision);
    }

    /* retrieve pnp base name */
    PinProperty.PinId = FilterId;
    PinProperty.Property.Set = KSPROPSETID_Sysaudio;
    PinProperty.Property.Id = KSPROPERTY_SYSAUDIO_DEVICE_INTERFACE_NAME;
    PinProperty.Property.Flags = KSPROPERTY_TYPE_GET;

    Status = KsSynchronousIoControlDevice(DeviceExtension->FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&PinProperty, sizeof(KSP_PIN), (PVOID)DeviceName, sizeof(DeviceName), &BytesReturned);
    if (NT_SUCCESS(Status))
    {
        /* find product name */
        Status = FindProductName(DeviceName, MAXPNAMELEN, DeviceInfo->u.WaveOutCaps.szPname);

        /* check for success */
        if (!NT_SUCCESS(Status))
        {
            DeviceInfo->u.WaveOutCaps.szPname[0] = L'\0';
        }
    }

    PinProperty.Reserved = DeviceInfo->DeviceIndex;
    PinProperty.PinId = PinId;
    PinProperty.Property.Set = KSPROPSETID_Pin;
    PinProperty.Property.Id = KSPROPERTY_PIN_DATARANGES;
    PinProperty.Property.Flags = KSPROPERTY_TYPE_GET;

    BytesReturned = 0;
    Status = KsSynchronousIoControlDevice(DeviceExtension->FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&PinProperty, sizeof(KSP_PIN), (PVOID)NULL, 0, &BytesReturned);
    if (Status != STATUS_BUFFER_TOO_SMALL)
    {
        return SetIrpIoStatus(Irp, Status, 0);
    }

    MultipleItem = ExAllocatePool(NonPagedPool, BytesReturned);
    if (!MultipleItem)
    {
        /* no memory */
        return SetIrpIoStatus(Irp, STATUS_NO_MEMORY, 0);
    }

    Status = KsSynchronousIoControlDevice(DeviceExtension->FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&PinProperty, sizeof(KSP_PIN), (PVOID)MultipleItem, BytesReturned, &BytesReturned);
    if (!NT_SUCCESS(Status))
    {
        ExFreePool(MultipleItem);
        return SetIrpIoStatus(Irp, Status, 0);
    }

    DataRange = (PKSDATARANGE) (MultipleItem + 1);
    for(Index = 0; Index < MultipleItem->Count; Index++)
    {
        if (DeviceInfo->DeviceType == WAVE_OUT_DEVICE_TYPE || DeviceInfo->DeviceType == WAVE_IN_DEVICE_TYPE)
        {
            if (DataRange->FormatSize == sizeof(KSDATARANGE_AUDIO))
            {
                DataRangeAudio = (PKSDATARANGE_AUDIO)DataRange;
                
                if (IsEqualGUIDAligned(&DataRangeAudio->DataRange.MajorFormat, &KSDATAFORMAT_TYPE_AUDIO) &&
                    IsEqualGUIDAligned(&DataRangeAudio->DataRange.SubFormat, &KSDATAFORMAT_SUBTYPE_PCM) &&
                    IsEqualGUIDAligned(&DataRangeAudio->DataRange.Specifier, &KSDATAFORMAT_SPECIFIER_WAVEFORMATEX))
                {
                    DPRINT("Min Sample %u Max Sample %u Min Bits %u Max Bits %u Max Channel %u\n", DataRangeAudio->MinimumSampleFrequency, DataRangeAudio->MaximumSampleFrequency,
                                                             DataRangeAudio->MinimumBitsPerSample, DataRangeAudio->MaximumBitsPerSample, DataRangeAudio->MaximumChannels);

                    dwFormats |= CheckFormatSupport(DataRangeAudio, 11025, WAVE_FORMAT_1M08, WAVE_FORMAT_1S08, WAVE_FORMAT_1M16, WAVE_FORMAT_1S16);
                    dwFormats |= CheckFormatSupport(DataRangeAudio, 22050, WAVE_FORMAT_2M08, WAVE_FORMAT_2S08, WAVE_FORMAT_2M16, WAVE_FORMAT_2S16);
                    dwFormats |= CheckFormatSupport(DataRangeAudio, 44100, WAVE_FORMAT_4M08, WAVE_FORMAT_4S08, WAVE_FORMAT_4M16, WAVE_FORMAT_4S16);
                    dwFormats |= CheckFormatSupport(DataRangeAudio, 48000, WAVE_FORMAT_48M08, WAVE_FORMAT_48S08, WAVE_FORMAT_48M16, WAVE_FORMAT_48S16);
                    dwFormats |= CheckFormatSupport(DataRangeAudio, 96000, WAVE_FORMAT_96M08, WAVE_FORMAT_96S08, WAVE_FORMAT_96M16, WAVE_FORMAT_96S16);


                    wChannels = DataRangeAudio->MaximumChannels;
                    dwSupport = WAVECAPS_VOLUME; //FIXME get info from nodes
                }
            }
        }
        DataRange = (PKSDATARANGE)((PUCHAR)DataRange + DataRange->FormatSize);
    }

    DeviceInfo->u.WaveOutCaps.dwFormats = dwFormats;
    DeviceInfo->u.WaveOutCaps.dwSupport = dwSupport;
    DeviceInfo->u.WaveOutCaps.wChannels = wChannels;

    ExFreePool(MultipleItem);

    return SetIrpIoStatus(Irp, Status, sizeof(WDMAUD_DEVICE_INFO));
}

NTSTATUS
NTAPI
WdmAudIoctlClose(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp,
    IN  PWDMAUD_DEVICE_INFO DeviceInfo,
    IN  PWDMAUD_CLIENT ClientInfo)
{
    ULONG Index;

    for(Index = 0; Index < ClientInfo->NumPins; Index++)
    {
        if (ClientInfo->hPins[Index].Handle == DeviceInfo->hDevice && ClientInfo->hPins[Index].Type != MIXER_DEVICE_TYPE)
        {
            DPRINT1("Closing device %p\n", DeviceInfo->hDevice);
            ZwClose(DeviceInfo->hDevice);
            ClientInfo->hPins[Index].Handle = NULL;
            SetIrpIoStatus(Irp, STATUS_SUCCESS, sizeof(WDMAUD_DEVICE_INFO));
            return STATUS_SUCCESS;
        }
    }
    SetIrpIoStatus(Irp, STATUS_INVALID_PARAMETER, sizeof(WDMAUD_DEVICE_INFO));
    return STATUS_INVALID_PARAMETER;
}

NTSTATUS
NTAPI
WdmAudFrameSize(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp,
    IN  PWDMAUD_DEVICE_INFO DeviceInfo,
    IN  PWDMAUD_CLIENT ClientInfo)
{
    PFILE_OBJECT FileObject;
    KSPROPERTY Property;
    ULONG BytesReturned;
    KSALLOCATOR_FRAMING Framing;
    NTSTATUS Status;

    /* Get sysaudio pin file object */
    Status = ObReferenceObjectByHandle(DeviceInfo->hDevice, GENERIC_WRITE, IoFileObjectType, KernelMode, (PVOID*)&FileObject, NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Invalid buffer handle %x\n", DeviceInfo->hDevice);
        return SetIrpIoStatus(Irp, Status, 0);
    }

    /* Setup get framing request */
    Property.Id = KSPROPERTY_CONNECTION_ALLOCATORFRAMING;
    Property.Flags = KSPROPERTY_TYPE_GET;
    Property.Set = KSPROPSETID_Connection;

    Status = KsSynchronousIoControlDevice(FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSPROPERTY), (PVOID)&Framing, sizeof(KSALLOCATOR_FRAMING), &BytesReturned);
    /* Did we succeed */
    if (NT_SUCCESS(Status))
    {
        /* Store framesize */
        DeviceInfo->u.FrameSize = Framing.FrameSize;
    }

    /* Release file object */
    ObDereferenceObject(FileObject);

    return SetIrpIoStatus(Irp, Status, sizeof(WDMAUD_DEVICE_INFO));

}


NTSTATUS
NTAPI
WdmAudDeviceControl(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PWDMAUD_DEVICE_INFO DeviceInfo;
    PWDMAUD_CLIENT ClientInfo;

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    DPRINT("WdmAudDeviceControl entered\n");

    if (IoStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(WDMAUD_DEVICE_INFO))
    {
        /* invalid parameter */
        DPRINT1("Input buffer too small size %u expected %u\n", IoStack->Parameters.DeviceIoControl.InputBufferLength, sizeof(WDMAUD_DEVICE_INFO));
        return SetIrpIoStatus(Irp, STATUS_INVALID_PARAMETER, 0);
    }

    DeviceInfo = (PWDMAUD_DEVICE_INFO)Irp->AssociatedIrp.SystemBuffer;

    if (DeviceInfo->DeviceType < MIN_SOUND_DEVICE_TYPE || DeviceInfo->DeviceType > MAX_SOUND_DEVICE_TYPE)
    {
        /* invalid parameter */
        DPRINT1("Error: device type not set\n");
        return SetIrpIoStatus(Irp, STATUS_INVALID_PARAMETER, 0);
    }

    if (!IoStack->FileObject)
    {
        /* file object parameter */
        DPRINT1("Error: file object is not attached\n");
        return SetIrpIoStatus(Irp, STATUS_UNSUCCESSFUL, 0);
    }
    ClientInfo = (PWDMAUD_CLIENT)IoStack->FileObject->FsContext;

    DPRINT("WdmAudDeviceControl entered\n");

    switch(IoStack->Parameters.DeviceIoControl.IoControlCode)
    {
        case IOCTL_OPEN_WDMAUD:
            return WdmAudControlOpen(DeviceObject, Irp, DeviceInfo, ClientInfo);
        case IOCTL_GETNUMDEVS_TYPE:
            return WdmAudControlDeviceType(DeviceObject, Irp, DeviceInfo, ClientInfo);
        case IOCTL_SETDEVICE_STATE:
            return WdmAudControlDeviceState(DeviceObject, Irp, DeviceInfo, ClientInfo);
        case IOCTL_GETCAPABILITIES:
            return WdmAudCapabilities(DeviceObject, Irp, DeviceInfo, ClientInfo);
        case IOCTL_CLOSE_WDMAUD:
            return WdmAudIoctlClose(DeviceObject, Irp, DeviceInfo, ClientInfo);
        case IOCTL_GETFRAMESIZE:
            return WdmAudFrameSize(DeviceObject, Irp, DeviceInfo, ClientInfo);
        case IOCTL_GETPOS:
        case IOCTL_GETDEVID:
        case IOCTL_GETVOLUME:
        case IOCTL_SETVOLUME:

           DPRINT1("Unhandeled %x\n", IoStack->Parameters.DeviceIoControl.IoControlCode);
           break;
    }

    return SetIrpIoStatus(Irp, STATUS_NOT_IMPLEMENTED, 0);
}

NTSTATUS
NTAPI
WdmAudWriteCompletion(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP LowerIrp,
    IN PVOID  Context)
{
    //PIRP Irp;
    ASSERT(LowerIrp->PendingReturned == FALSE);
    /* get original irp */
    //Irp = (PIRP)Context;

    /* save status */
    //Irp->IoStatus.Status = LowerIrp->IoStatus.Status;
    //Irp->IoStatus.Information = LowerIrp->IoStatus.Information;
    /* complete request */
    //IoCompleteRequest(Irp, IO_SOUND_INCREMENT);
    /* return success to free irp */
    return STATUS_SUCCESS;
}


NTSTATUS
NTAPI
WdmAudWrite(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PWDMAUD_DEVICE_INFO DeviceInfo;
    PWDMAUD_CLIENT ClientInfo;
    NTSTATUS Status = STATUS_SUCCESS;
    PUCHAR Buffer;
    PFILE_OBJECT FileObject;
    PMDL Mdl;
    //PIRP LowerIrp;
    PCONTEXT_WRITE Packet;
    PVOID SystemBuffer;
    //LARGE_INTEGER Offset;
    IO_STATUS_BLOCK IoStatusBlock;

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    //DPRINT("WdmAudWrite entered\n");

    if (IoStack->Parameters.Write.Length < sizeof(WDMAUD_DEVICE_INFO))
    {
        /* invalid parameter */
        DPRINT1("Input buffer too small size %u expected %u\n", IoStack->Parameters.Write.Length, sizeof(WDMAUD_DEVICE_INFO));
        return SetIrpIoStatus(Irp, STATUS_INVALID_PARAMETER, 0);
    }

    DeviceInfo = (PWDMAUD_DEVICE_INFO)MmGetMdlVirtualAddress(Irp->MdlAddress);


    Status = ObReferenceObjectByHandle(DeviceInfo->hDevice, GENERIC_WRITE, IoFileObjectType, KernelMode, (PVOID*)&FileObject, NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Invalid buffer handle %x\n", DeviceInfo->hDevice);
        return SetIrpIoStatus(Irp, Status, 0);
    }


    //DPRINT("DeviceInfo %p %p %p\n", DeviceInfo, Irp->MdlAddress->StartVa, Irp->MdlAddress->MappedSystemVa);
    if (DeviceInfo->DeviceType < MIN_SOUND_DEVICE_TYPE || DeviceInfo->DeviceType > MAX_SOUND_DEVICE_TYPE)
    {
        /* invalid parameter */
        DPRINT1("Error: device type not set\n");
        ObDereferenceObject(FileObject);
        return SetIrpIoStatus(Irp, STATUS_INVALID_PARAMETER, 0);
    }

    if (!IoStack->FileObject)
    {
        /* file object parameter */
        DPRINT1("Error: file object is not attached\n");
        ObDereferenceObject(FileObject);
        return SetIrpIoStatus(Irp, STATUS_UNSUCCESSFUL, 0);
    }
    ClientInfo = (PWDMAUD_CLIENT)IoStack->FileObject->FsContext;


    /* setup stream context */
    Packet = (PCONTEXT_WRITE)ExAllocatePool(NonPagedPool, sizeof(CONTEXT_WRITE));
    if (!Packet)
    {
        /* no memory */
        return SetIrpIoStatus(Irp, STATUS_NO_MEMORY, 0);
    }

    Packet->Header.FrameExtent = DeviceInfo->Header.FrameExtent;
    Packet->Header.DataUsed = DeviceInfo->Header.DataUsed;
    Packet->Header.Size = sizeof(KSSTREAM_HEADER);
    Packet->Header.PresentationTime.Numerator = 1;
    Packet->Header.PresentationTime.Denominator = 1;
    Packet->Irp = Irp;

    Buffer = ExAllocatePool(NonPagedPool, DeviceInfo->Header.DataUsed);
    if (!Buffer)
    {
        /* no memory */
        ExFreePool(Packet);
        ObDereferenceObject(FileObject);
        return SetIrpIoStatus(Irp, STATUS_NO_MEMORY, 0);
    }
    Packet->Header.Data = Buffer;

    Mdl = IoAllocateMdl(DeviceInfo->Header.Data, DeviceInfo->Header.DataUsed, FALSE, FALSE, FALSE);
    if (!Mdl)
    {
        /* no memory */
        ExFreePool(Packet);
        ObDereferenceObject(FileObject);
        ExFreePool(Buffer);
        return SetIrpIoStatus(Irp, STATUS_NO_MEMORY, 0);
    }

    _SEH2_TRY
    {
        MmProbeAndLockPages(Mdl, UserMode, IoReadAccess);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Exception, get the error code */
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Invalid buffer supplied\n");
        ExFreePool(Buffer);
        ExFreePool(Packet);
        IoFreeMdl(Mdl);
        ObDereferenceObject(FileObject);
        return SetIrpIoStatus(Irp, Status, 0);
    }

    SystemBuffer = MmGetSystemAddressForMdlSafe(Mdl, NormalPagePriority );
    if (!SystemBuffer)
    {
        DPRINT1("Invalid buffer supplied\n");
        ExFreePool(Buffer);
        ExFreePool(Packet);
        IoFreeMdl(Mdl);
        ObDereferenceObject(FileObject);
        return SetIrpIoStatus(Irp, STATUS_INSUFFICIENT_RESOURCES, 0);
    }

    RtlMoveMemory(Buffer, SystemBuffer, DeviceInfo->Header.DataUsed);
    MmUnlockPages(Mdl);
    IoFreeMdl(Mdl);

#if 1
    KsStreamIo(FileObject, NULL, NULL, NULL, NULL, 0, &IoStatusBlock, Packet, sizeof(CONTEXT_WRITE), KSSTREAM_WRITE, UserMode);
    /* dereference file object */
    ObDereferenceObject(FileObject);
    return IoStatusBlock.Status;
#else
    Offset.QuadPart = 0L;

    /* now build the irp */
    LowerIrp = IoBuildAsynchronousFsdRequest (IRP_MJ_WRITE,
                                              IoGetRelatedDeviceObject(FileObject),
                                              Packet,
                                              sizeof(KSSTREAM_HEADER),
                                              &Offset,
                                              NULL);

    if (!LowerIrp)
    {
        /* failed to create an associated irp */
        ExFreePool(Buffer);
        ExFreePool(Packet);
        ObDereferenceObject(FileObject);

        return SetIrpIoStatus(Irp, STATUS_INSUFFICIENT_RESOURCES, 0);
    }

    /* get next stack location */
    IoStack = IoGetNextIrpStackLocation(LowerIrp);

    /* attach file object */
    IoStack->FileObject = FileObject;

    /* set a completion routine */
    IoSetCompletionRoutine(LowerIrp, WdmAudWriteCompletion, (PVOID)Irp, TRUE, TRUE, TRUE);

    /* mark irp as pending */
    //IoMarkIrpPending(Irp);
    Irp->IoStatus.Information = DeviceInfo->BufferSize;
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    DPRINT1("Wrote %u\n", DeviceInfo->BufferSize);
    /* call the driver */
    Status = IoCallDriver(IoGetRelatedDeviceObject(FileObject), LowerIrp);

    /* dereference file object */
    ObDereferenceObject(FileObject);

    return STATUS_SUCCESS;
#endif
}
