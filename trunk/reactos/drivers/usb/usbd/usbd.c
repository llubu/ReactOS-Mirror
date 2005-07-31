/*
 * Universal Serial Bus Driver/Helper Library
 *
 * Written by Filip Navara <xnavara@volny.cz>
 *
 * Notes:
 *    This driver was obsoleted in Windows XP and most functions
 *    became pure stubs. But some of them were retained for backward
 *    compatibilty with existing drivers.
 *
 *    Preserved functions:
 *
 *    USBD_Debug_GetHeap (implemented)
 *    USBD_Debug_RetHeap (implemented)
 *    USBD_CalculateUsbBandwidth (implemented, tested)
 *    USBD_CreateConfigurationRequestEx (implemented)
 *    USBD_CreateConfigurationRequest
 *    USBD_GetInterfaceLength (implemented)
 *    USBD_ParseConfigurationDescriptorEx
 *    USBD_ParseDescriptors
 *    USBD_GetPdoRegistryParameters (implemented)
 */

#include <windows.h>
#include <ddk/usbdi.h>
#ifndef PLUGPLAY_REGKEY_DRIVER
#define PLUGPLAY_REGKEY_DRIVER              2
#endif
typedef struct _USBD_INTERFACE_LIST_ENTRY {
    PUSB_INTERFACE_DESCRIPTOR InterfaceDescriptor;
    PUSBD_INTERFACE_INFORMATION Interface;
} USBD_INTERFACE_LIST_ENTRY, *PUSBD_INTERFACE_LIST_ENTRY;

NTSTATUS STDCALL
DriverEntry(PDRIVER_OBJECT DriverObject,
            PUNICODE_STRING RegistryPath)
{
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
DWORD STDCALL
DllInitialize(DWORD Unknown)
{
    return 0;
}

/*
 * @implemented
 */
DWORD STDCALL
DllUnload(VOID)
{
    return 0;
}

/*
 * @implemented
 */
PVOID STDCALL
USBD_Debug_GetHeap(DWORD Unknown1, POOL_TYPE PoolType, ULONG NumberOfBytes,
	ULONG Tag)
{
    return ExAllocatePoolWithTag(PoolType, NumberOfBytes, Tag);
}

/*
 * @implemented
 */
VOID STDCALL
USBD_Debug_RetHeap(PVOID Heap, DWORD Unknown2, DWORD Unknown3)
{
    ExFreePool(Heap);
}

/*
 * @implemented
 */
VOID STDCALL
USBD_Debug_LogEntry(PCHAR Name, ULONG_PTR Info1, ULONG_PTR Info2,
    ULONG_PTR Info3)
{
}

/*
 * @implemented
 */
PVOID STDCALL
USBD_AllocateDeviceName(DWORD Unknown)
{
    return NULL;
}

/*
 * @implemented
 */
DWORD STDCALL
USBD_CalculateUsbBandwidth(
    ULONG MaxPacketSize,
    UCHAR EndpointType,
    BOOLEAN LowSpeed
    )
{
    DWORD OverheadTable[] = {
            0x00, /* UsbdPipeTypeControl */
            0x09, /* UsbdPipeTypeIsochronous */
            0x00, /* UsbdPipeTypeBulk */
            0x0d  /* UsbdPipeTypeInterrupt */
        };
    DWORD Result;
    
    if (OverheadTable[EndpointType] != 0)
    {
        Result = ((MaxPacketSize + OverheadTable[EndpointType]) * 8 * 7) / 6;
        if (LowSpeed)
           return Result << 3;
        return Result;
    }
    return 0;
}

/*
 * @implemented
 */
DWORD STDCALL
USBD_Dispatch(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
    return 1;
}

/*
 * @implemented
 */
VOID STDCALL
USBD_FreeDeviceMutex(PVOID Unknown)
{
}

/*
 * @implemented
 */
VOID STDCALL
USBD_FreeDeviceName(PVOID Unknown)
{
}

/*
 * @implemented
 */
VOID STDCALL
USBD_WaitDeviceMutex(PVOID Unknown)
{
}

/*
 * @implemented
 */
DWORD STDCALL
USBD_GetSuspendPowerState(DWORD Unknown1)
{
    return 0;
}

/*
 * @implemented
 */
NTSTATUS STDCALL
USBD_InitializeDevice(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3,
    DWORD Unknown4, DWORD Unknown5, DWORD Unknown6)
{
    return STATUS_NOT_SUPPORTED;
}

/*
 * @implemented
 */
NTSTATUS STDCALL
USBD_RegisterHostController(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3,
    DWORD Unknown4, DWORD Unknown5, DWORD Unknown6, DWORD Unknown7,
    DWORD Unknown8, DWORD Unknown9, DWORD Unknown10)
{
    return STATUS_NOT_SUPPORTED;
}

/*
 * @implemented
 */
NTSTATUS STDCALL
USBD_GetDeviceInformation(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
    return STATUS_NOT_SUPPORTED;
}

/*
 * @implemented
 */
NTSTATUS STDCALL
USBD_CreateDevice(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3,
    DWORD Unknown4, DWORD Unknown5)
{
    return STATUS_NOT_SUPPORTED;
}

/*
 * @implemented
 */
NTSTATUS STDCALL
USBD_RemoveDevice(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
    return STATUS_NOT_SUPPORTED;
}

/*
 * @implemented
 */
VOID STDCALL
USBD_CompleteRequest(DWORD Unknown1, DWORD Unknown2)
{
}

/*
 * @implemented
 */
VOID STDCALL
USBD_RegisterHcFilter(
    PDEVICE_OBJECT DeviceObject, 
    PDEVICE_OBJECT FilterDeviceObject
    )
{
}

/*
 * @implemented
 */
VOID STDCALL
USBD_SetSuspendPowerState(DWORD Unknown1, DWORD Unknown2)
{
}

/*
 * @implemented
 */
NTSTATUS STDCALL
USBD_MakePdoName(DWORD Unknown1, DWORD Unknown2)
{
    return STATUS_NOT_SUPPORTED;
}

/*
 * @implemented
 */
NTSTATUS STDCALL
USBD_QueryBusTime(
    PDEVICE_OBJECT RootHubPdo,
    PULONG CurrentFrame
    )
{
    return STATUS_NOT_SUPPORTED;
}

/*
 * @implemented
 */
VOID STDCALL
USBD_GetUSBDIVersion(
    PUSBD_VERSION_INFORMATION Version
    )
{
    if (Version != NULL)
    {
        Version->USBDI_Version = USBDI_VERSION;
        Version->Supported_USB_Version = 0x100;
    }
}

/*
 * @implemented
 */
NTSTATUS STDCALL
USBD_RestoreDevice(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
    return STATUS_NOT_SUPPORTED;
}

/*
 * @implemented
 */
VOID STDCALL
USBD_RegisterHcDeviceCapabilities(DWORD Unknown1, DWORD Unknown2,
    DWORD Unknown3)
{
}

/*
 * @implemented
 * FIXME: Test
 */
PURB
STDCALL
USBD_CreateConfigurationRequestEx(
    PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
    PUSBD_INTERFACE_LIST_ENTRY InterfaceList
    )
{
    PURB Urb;
    DWORD UrbSize;
    DWORD InterfaceCount;

    for (InterfaceCount = 0;
         InterfaceList[InterfaceCount].InterfaceDescriptor != NULL;
         ++InterfaceCount)
       ;
    /* Include the NULL entry */
    ++InterfaceCount;

    UrbSize = sizeof(Urb->UrbSelectConfiguration) + 
       (InterfaceCount * sizeof(PUSBD_INTERFACE_LIST_ENTRY));
    Urb = ExAllocatePool(NonPagedPool, UrbSize);
    Urb->UrbSelectConfiguration.Hdr.Function =
        URB_FUNCTION_SELECT_CONFIGURATION;        
    Urb->UrbSelectConfiguration.Hdr.Length =
        sizeof(Urb->UrbSelectConfiguration);
    Urb->UrbSelectConfiguration.ConfigurationDescriptor = 
       ConfigurationDescriptor;
    memcpy((PVOID)&Urb->UrbSelectConfiguration.Interface, (PVOID)InterfaceList,
       InterfaceCount * sizeof(PUSBD_INTERFACE_LIST_ENTRY));

    return Urb;
}

/*
 * @unimplemented
 */
PURB STDCALL
USBD_CreateConfigurationRequest(
    PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
    PUSHORT Size
    )
{
    return NULL;
}

/*
 * @unimplemented
 */
ULONG STDCALL
USBD_GetInterfaceLength(
    PUSB_INTERFACE_DESCRIPTOR InterfaceDescriptor,
    PUCHAR BufferEnd
    )
{
    ULONG_PTR Current;
    PUSB_INTERFACE_DESCRIPTOR CurrentDescriptor = InterfaceDescriptor;
    DWORD Length = CurrentDescriptor->bLength;

    // USB_ENDPOINT_DESCRIPTOR_TYPE
    if (CurrentDescriptor->bDescriptorType == USB_INTERFACE_DESCRIPTOR_TYPE)
    {
        for (Current = (ULONG_PTR)CurrentDescriptor;
             Current < (ULONG_PTR)BufferEnd;
             Current += CurrentDescriptor->bLength)
            CurrentDescriptor = (PUSB_INTERFACE_DESCRIPTOR)Current;
            Length += CurrentDescriptor->bLength;

    }
    return Length;
}

/*
 * @unimplemented
 */
PUSB_INTERFACE_DESCRIPTOR STDCALL
USBD_ParseConfigurationDescriptorEx(
    PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
    PVOID StartPosition,
    LONG InterfaceNumber,
    LONG AlternateSetting,
    LONG InterfaceClass,
    LONG InterfaceSubClass,
    LONG InterfaceProtocol
    )
{
    return NULL;
}

/*
 * @implemented
 */
PUSB_INTERFACE_DESCRIPTOR STDCALL
USBD_ParseConfigurationDescriptor(
    PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
    UCHAR InterfaceNumber,
    UCHAR AlternateSetting
    )
{
    return USBD_ParseConfigurationDescriptorEx(ConfigurationDescriptor,
        (PVOID)ConfigurationDescriptor, InterfaceNumber, AlternateSetting,
        -1, -1, -1);
}

/*
 * @unimplemented
 */
PUSB_COMMON_DESCRIPTOR STDCALL
USBD_ParseDescriptors(
    PVOID  DescriptorBuffer,
    ULONG  TotalLength,
    PVOID  StartPosition,
    LONG  DescriptorType
    )
{
    return NULL; 
}

/*
 * @implemented
 */
DWORD STDCALL
USBD_GetPdoRegistryParameter(
    PDEVICE_OBJECT PhysicalDeviceObject,
    PVOID Parameter,
    ULONG ParameterLength,
    PWCHAR KeyName,
    ULONG KeyNameLength
    )
{
    NTSTATUS Status;
    HANDLE DevInstRegKey;

    Status = IoOpenDeviceRegistryKey(PhysicalDeviceObject,
        PLUGPLAY_REGKEY_DRIVER, STANDARD_RIGHTS_ALL, &DevInstRegKey);
    if (NT_SUCCESS(Status))
    {
        PKEY_VALUE_FULL_INFORMATION FullInfo;
        UNICODE_STRING ValueName;
        ULONG Length;

        RtlInitUnicodeString(&ValueName, KeyName);
        Length = ParameterLength + KeyNameLength + sizeof(KEY_VALUE_FULL_INFORMATION);
        FullInfo = ExAllocatePool(PagedPool, Length);
        if (FullInfo)
        {
            Status = ZwQueryValueKey(DevInstRegKey, &ValueName,
                KeyValueFullInformation, FullInfo, Length, &Length);
            if (NT_SUCCESS(Status))
            {
                RtlCopyMemory(Parameter,
                    ((PUCHAR)FullInfo) + FullInfo->DataOffset,
                    ParameterLength /*FullInfo->DataLength*/);
            }
            ExFreePool(FullInfo);
        } else
            Status = STATUS_NO_MEMORY;
        ZwClose(DevInstRegKey);
    }
    return Status;
}
