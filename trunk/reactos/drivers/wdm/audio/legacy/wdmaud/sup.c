/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/legacy/wdmaud/sup.c
 * PURPOSE:         System Audio graph builder
 * PROGRAMMER:      Andrew Greenwood
 *                  Johannes Anderwald
 */
#include "wdmaud.h"

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
ClosePin(
    IN  PWDMAUD_CLIENT ClientInfo,
    IN  ULONG FilterId,
    IN  ULONG PinId,
    IN  SOUND_DEVICE_TYPE DeviceType)
{
    ULONG Index;

    for(Index = 0; Index < ClientInfo->NumPins; Index++)
    {
        if (ClientInfo->hPins[Index].FilterId == FilterId && ClientInfo->hPins[Index].PinId == PinId && ClientInfo->hPins[Index].Handle && ClientInfo->hPins[Index].Type == DeviceType)
        {
            if (ClientInfo->hPins[Index].Type != MIXER_DEVICE_TYPE)
            {
                ZwClose(ClientInfo->hPins[Index].Handle);
            }
            ClientInfo->hPins[Index].Handle = NULL;
            return Index;
        }
    }
    return MAXULONG;
}

NTSTATUS
InsertPinHandle(
    IN  PWDMAUD_CLIENT ClientInfo,
    IN  ULONG FilterId,
    IN  ULONG PinId,
    IN  SOUND_DEVICE_TYPE DeviceType,
    IN  HANDLE PinHandle,
    IN  ULONG FreeIndex)
{
    PWDMAUD_HANDLE Handles;

    if (FreeIndex != MAXULONG)
    {
        /* re-use a free index */
        ClientInfo->hPins[FreeIndex].Handle = PinHandle;
        ClientInfo->hPins[FreeIndex].FilterId = FilterId;
        ClientInfo->hPins[FreeIndex].PinId = PinId;
        ClientInfo->hPins[FreeIndex].Type = DeviceType;

        return STATUS_SUCCESS;
    }

    Handles = ExAllocatePool(NonPagedPool, sizeof(WDMAUD_HANDLE) * (ClientInfo->NumPins+1));

    if (!Handles)
        return STATUS_INSUFFICIENT_RESOURCES;

    if (ClientInfo->NumPins)
    {
        RtlMoveMemory(Handles, ClientInfo->hPins, sizeof(WDMAUD_HANDLE) * ClientInfo->NumPins);
        ExFreePool(ClientInfo->hPins);
    }

    ClientInfo->hPins = Handles;
    ClientInfo->hPins[ClientInfo->NumPins].Handle = PinHandle;
    ClientInfo->hPins[ClientInfo->NumPins].Type = DeviceType;
    ClientInfo->hPins[ClientInfo->NumPins].FilterId = FilterId;
    ClientInfo->hPins[ClientInfo->NumPins].PinId = PinId;
    ClientInfo->NumPins++;

    return STATUS_SUCCESS;
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

