/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Enhanced Host Controller Interface
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/usbohci/usb_device.cpp
 * PURPOSE:     USB OHCI device driver.
 * PROGRAMMERS:
 *              Michael Martin (michael.martin@reactos.org)
 *              Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#define INITGUID
#include "usbuhci.h"

class CUSBDevice : public IUSBDevice
{
public:
    STDMETHODIMP QueryInterface( REFIID InterfaceId, PVOID* Interface);

    STDMETHODIMP_(ULONG) AddRef()
    {
        InterlockedIncrement(&m_Ref);
        return m_Ref;
    }
    STDMETHODIMP_(ULONG) Release()
    {
        InterlockedDecrement(&m_Ref);

        if (!m_Ref)
        {
            delete this;
            return 0;
        }
        return m_Ref;
    }

    // IUSBDevice interface functions
    virtual NTSTATUS Initialize(IN PHUBCONTROLLER HubController, IN PUSBHARDWAREDEVICE Device, IN PVOID Parent, IN ULONG Port, IN ULONG PortStatus);
    virtual BOOLEAN IsHub();
    virtual NTSTATUS GetParent(PVOID * Parent);
    virtual UCHAR GetDeviceAddress();
    virtual ULONG GetPort();
    virtual USB_DEVICE_SPEED GetSpeed();
    virtual USB_DEVICE_TYPE GetType();
    virtual ULONG GetState();
    virtual void SetDeviceHandleData(PVOID Data);
    virtual NTSTATUS SetDeviceAddress(UCHAR DeviceAddress);
    virtual void GetDeviceDescriptor(PUSB_DEVICE_DESCRIPTOR DeviceDescriptor);
    virtual UCHAR GetConfigurationValue();
    virtual NTSTATUS SubmitIrp(PIRP Irp);
    virtual VOID GetConfigurationDescriptors(IN PUSB_CONFIGURATION_DESCRIPTOR ConfigDescriptorBuffer, IN ULONG BufferLength, OUT PULONG OutBufferLength);
    virtual ULONG GetConfigurationDescriptorsLength();
    virtual NTSTATUS SubmitSetupPacket(IN PUSB_DEFAULT_PIPE_SETUP_PACKET SetupPacket, OUT ULONG BufferLength, OUT PVOID Buffer);
    virtual NTSTATUS SelectConfiguration(IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor, IN PUSBD_INTERFACE_INFORMATION Interface, OUT USBD_CONFIGURATION_HANDLE *ConfigurationHandle);
    virtual NTSTATUS SelectInterface(IN USBD_CONFIGURATION_HANDLE ConfigurationHandle, IN OUT PUSBD_INTERFACE_INFORMATION Interface);

    // local function
    virtual NTSTATUS CommitIrp(PIRP Irp);
    virtual NTSTATUS CommitSetupPacket(PUSB_DEFAULT_PIPE_SETUP_PACKET Packet, IN OPTIONAL PUSB_ENDPOINT_DESCRIPTOR EndpointDescriptor, IN ULONG BufferLength, IN OUT PMDL Mdl);
    virtual NTSTATUS CreateConfigurationDescriptor(ULONG ConfigurationIndex);
    virtual NTSTATUS CreateDeviceDescriptor();
    virtual VOID DumpDeviceDescriptor(PUSB_DEVICE_DESCRIPTOR DeviceDescriptor);
    virtual VOID DumpConfigurationDescriptor(PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor);

    // constructor / destructor
    CUSBDevice(IUnknown *OuterUnknown){}
    virtual ~CUSBDevice(){}

protected:
    LONG m_Ref;
    PHUBCONTROLLER m_HubController;
    PUSBHARDWAREDEVICE m_Device;
    PVOID m_Parent; 
    ULONG m_Port;
    UCHAR m_DeviceAddress;
    PVOID m_Data;
    UCHAR m_ConfigurationIndex;
    KSPIN_LOCK m_Lock;
    USB_DEVICE_DESCRIPTOR m_DeviceDescriptor;
    ULONG m_PortStatus;
    PUSBQUEUE m_Queue;
    PDMAMEMORYMANAGER m_DmaManager;
    PUSB_CONFIGURATION_DESCRIPTOR *m_ConfigurationDescriptors;
};

//----------------------------------------------------------------------------------------
NTSTATUS
STDMETHODCALLTYPE
CUSBDevice::QueryInterface(
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    return STATUS_UNSUCCESSFUL;
}

//----------------------------------------------------------------------------------------
NTSTATUS
CUSBDevice::Initialize(
    IN PHUBCONTROLLER HubController, 
    IN PUSBHARDWAREDEVICE Device, 
    IN PVOID Parent, 
    IN ULONG Port,
    IN ULONG PortStatus)
{
    NTSTATUS Status;

    //
    // initialize members
    //
    m_HubController = HubController;
    m_Device = Device;
    m_Parent = Parent;
    m_Port = Port;
    m_PortStatus = PortStatus;

    //
    // initialize device lock
    //
    KeInitializeSpinLock(&m_Lock);

    //
    // no device address has been set yet
    //
    m_DeviceAddress = 0;

    //
    // get usb request queue
    //
    Status = m_Device->GetUSBQueue(&m_Queue);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to get usb queue
        //
        DPRINT1("CUSBDevice::Initialize GetUsbQueue failed with %x\n", Status);
        return Status;
    }

    //
    // get dma manager
    //
    Status = m_Device->GetDMA(&m_DmaManager);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to get dma manager
        //
        DPRINT1("CUSBDevice::Initialize GetDMA failed with %x\n", Status);
        return Status;
    }

    //
    // sanity check
    //
    PC_ASSERT(m_DmaManager);

    //
    // get device descriptor
    //
    Status = CreateDeviceDescriptor();
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to get device descriptor
        //
        DPRINT1("CUSBDevice::Initialize Failed to get device descriptor with %x\n", Status);
        return Status;
    }

    //
    // done
    //
    return Status;
}

//----------------------------------------------------------------------------------------
BOOLEAN
CUSBDevice::IsHub()
{
    //
    // USB Standard Device Class see http://www.usb.org/developers/defined_class/#BaseClass09h
    // for details
    //
    return (m_DeviceDescriptor.bDeviceClass == 0x09 && m_DeviceDescriptor.bDeviceSubClass == 0x00);
}

//----------------------------------------------------------------------------------------
NTSTATUS
CUSBDevice::GetParent(
    PVOID * Parent)
{
    //
    // returns parent
    //
    *Parent = m_Parent;

    //
    // done
    //
    return STATUS_SUCCESS;
}

//----------------------------------------------------------------------------------------
UCHAR
CUSBDevice::GetDeviceAddress()
{
    //
    // get device address
    //
    return m_DeviceAddress;
}

//----------------------------------------------------------------------------------------
ULONG
CUSBDevice::GetPort()
{
    //
    // get port to which this device is connected to
    //
    return m_Port;
}

//----------------------------------------------------------------------------------------
USB_DEVICE_SPEED
CUSBDevice::GetSpeed()
{
    if (m_PortStatus & USB_PORT_STATUS_LOW_SPEED)
    {
        //
        // low speed device
        //
        return UsbLowSpeed;
    }
    else if (m_PortStatus & USB_PORT_STATUS_HIGH_SPEED)
    {
        //
        // high speed device
        //
        return UsbHighSpeed;
    }

    //
    // default to full speed
    //
    return UsbFullSpeed;
}

//----------------------------------------------------------------------------------------
USB_DEVICE_TYPE
CUSBDevice::GetType()
{
    //
    // device is encoded into bcdUSB
    //
    if (m_DeviceDescriptor.bcdUSB == 0x110)
    {
        //
        // USB 1.1 device
        //
        return Usb11Device;
    }
    else if (m_DeviceDescriptor.bcdUSB == 0x200)
    {
        //
        // USB 2.0 device
        //
        return Usb20Device;
    }

    DPRINT1("CUSBDevice::GetType Unknown bcdUSB Type %x\n", m_DeviceDescriptor.bcdUSB);
    PC_ASSERT(FALSE);

    return Usb11Device;
}

//----------------------------------------------------------------------------------------
ULONG
CUSBDevice::GetState()
{
    UNIMPLEMENTED
    return FALSE;
}

//----------------------------------------------------------------------------------------
void
CUSBDevice::SetDeviceHandleData(
    PVOID Data)
{
    //
    // set device data, for debugging issues
    //
    m_Data = Data;
}

//----------------------------------------------------------------------------------------
NTSTATUS
CUSBDevice::SetDeviceAddress(
    UCHAR DeviceAddress)
{
    PUSB_DEFAULT_PIPE_SETUP_PACKET CtrlSetup;
    NTSTATUS Status;
    UCHAR OldAddress;
    ULONG Index;

    DPRINT1("CUSBDevice::SetDeviceAddress Address %d\n", DeviceAddress);

    CtrlSetup = (PUSB_DEFAULT_PIPE_SETUP_PACKET)ExAllocatePoolWithTag(NonPagedPool, sizeof(USB_DEFAULT_PIPE_SETUP_PACKET), TAG_USBOHCI);
    if (!CtrlSetup)
        return STATUS_INSUFFICIENT_RESOURCES;

    //
    // zero request
    //
    RtlZeroMemory(CtrlSetup, sizeof(USB_DEFAULT_PIPE_SETUP_PACKET));

    //
    // initialize request
    //
    CtrlSetup->bRequest = USB_REQUEST_SET_ADDRESS;
    CtrlSetup->wValue.W = (USHORT)DeviceAddress;

    //
    // set device address
    //
    Status = CommitSetupPacket(CtrlSetup, 0, 0, 0);

    //
    // free setup packet
    //
    ExFreePoolWithTag(CtrlSetup, TAG_USBOHCI);

    //
    // check for success
    //
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to set device address
        //
        DPRINT1("CUSBDevice::SetDeviceAddress> failed to set device address with %x Address %x\n", Status, DeviceAddress);
        return Status;
    }

    //
    // lets have a short nap
    //
    KeStallExecutionProcessor(300);

    //
    // back up old address
    //
    OldAddress = m_DeviceAddress;

    //
    // store new device address
    //
    m_DeviceAddress = DeviceAddress;

    //
    // check that setting device address succeeded by retrieving the device descriptor
    //
    Status = CreateDeviceDescriptor();
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to retrieve device descriptor
        //
        DPRINT1("CUSBbDevice::SetDeviceAddress> failed to retrieve device descriptor with device address set Error %x\n", Status);
        m_DeviceAddress = OldAddress;

        //
        // return error status
        //
        return Status;
    }

    //
    // sanity checks
    //
    PC_ASSERT(m_DeviceDescriptor.bNumConfigurations);

    //
    // allocate configuration descriptor
    //
    m_ConfigurationDescriptors = (PUSB_CONFIGURATION_DESCRIPTOR*) ExAllocatePoolWithTag(NonPagedPool, sizeof(PUSB_CONFIGURATION_DESCRIPTOR) * m_DeviceDescriptor.bNumConfigurations, TAG_USBOHCI);

    //
    // zero configuration descriptor
    //
    RtlZeroMemory(m_ConfigurationDescriptors, sizeof(PUSB_CONFIGURATION_DESCRIPTOR) * m_DeviceDescriptor.bNumConfigurations);

    //
    // retrieve the configuration descriptors
    //
    for(Index = 0; Index < m_DeviceDescriptor.bNumConfigurations; Index++)
    {
        //
        // retrieve configuration descriptors from device
        //
        Status = CreateConfigurationDescriptor(Index);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("CUSBDevice::SetDeviceAddress> failed to retrieve configuration %lu\n", Index);
            break;
        }
    }

    //
    // done
    //
    return Status;

}

//----------------------------------------------------------------------------------------
void
CUSBDevice::GetDeviceDescriptor(
    PUSB_DEVICE_DESCRIPTOR DeviceDescriptor)
{
    RtlMoveMemory(DeviceDescriptor, &m_DeviceDescriptor, sizeof(USB_DEVICE_DESCRIPTOR));
}

//----------------------------------------------------------------------------------------
UCHAR
CUSBDevice::GetConfigurationValue()
{
    //
    // return configuration index
    //
    return m_ConfigurationIndex;
}

//----------------------------------------------------------------------------------------
NTSTATUS
CUSBDevice::CommitIrp(
    PIRP Irp)
{
    NTSTATUS Status;
    PUSBREQUEST Request;

    if (!m_Queue || !m_DmaManager)
    {
        //
        // no queue, wtf?
        //
        DPRINT1("CUSBDevice::CommitUrb> no queue / dma !!!\n");
        return STATUS_UNSUCCESSFUL;
    }

    //
    // build usb request
    //
    Status = m_Queue->CreateUSBRequest(&Request);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to build request
        //
        DPRINT1("CUSBDevice::CommitSetupPacket> CreateUSBRequest failed with %x\n", Status);
        return Status;
    }

    //
    // initialize request
    //
    Status = Request->InitializeWithIrp(m_DmaManager, Irp);

    //
    // mark irp as pending
    //
    IoMarkIrpPending(Irp);

    //
    // now add the request
    //
    Status = m_Queue->AddUSBRequest(Request);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to add request
        //
        DPRINT1("CUSBDevice::CommitSetupPacket> failed add request to queue with %x\n", Status);
        Request->Release();
        return Status;
    }

    //
    // done
    //
    return STATUS_PENDING;
}

//----------------------------------------------------------------------------------------
NTSTATUS
CUSBDevice::SubmitIrp(
    PIRP Irp)
{
    KIRQL OldLevel;
    NTSTATUS Status;

    //
    // acquire device lock
    //
    KeAcquireSpinLock(&m_Lock, &OldLevel);

    //
    // commit urb
    //
    Status = CommitIrp(Irp);

    //
    // release lock
    //
    KeReleaseSpinLock(&m_Lock, OldLevel);

    return Status;
}
//----------------------------------------------------------------------------------------
NTSTATUS
CUSBDevice::CommitSetupPacket(
    IN PUSB_DEFAULT_PIPE_SETUP_PACKET Packet,
    IN OPTIONAL PUSB_ENDPOINT_DESCRIPTOR EndpointDescriptor,
    IN ULONG BufferLength, 
    IN OUT PMDL Mdl)
{
    NTSTATUS Status;
    PUSBREQUEST Request;

    if (!m_Queue)
    {
        //
        // no queue, wtf?
        //
        DPRINT1("CUSBDevice::CommitSetupPacket> no queue!!!\n");
        return STATUS_UNSUCCESSFUL;
    }

    //
    // build usb request
    //
    Status = m_Queue->CreateUSBRequest(&Request);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to build request
        //
        DPRINT1("CUSBDevice::CommitSetupPacket> CreateUSBRequest failed with %x\n", Status);
        return Status;
    }

    //
    // initialize request
    //
    Status = Request->InitializeWithSetupPacket(m_DmaManager, Packet, m_DeviceAddress, EndpointDescriptor, BufferLength, Mdl);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to initialize request
        //
        DPRINT1("CUSBDevice::CommitSetupPacket> failed to initialize  usb request with %x\n", Status);
        Request->Release();
        return Status;
    }

    //
    // now add the request
    //
    Status = m_Queue->AddUSBRequest(Request);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to add request
        //
        DPRINT1("CUSBDevice::CommitSetupPacket> failed add request to queue with %x\n", Status);
        Request->Release();
        return Status;
    }

    //
    // get the result code when the operation has been finished
    //
    Request->GetResultStatus(&Status, NULL);

    //
    // release request
    //
    Request->Release();

    //
    // done
    //
    return Status;
}

//----------------------------------------------------------------------------------------
NTSTATUS
CUSBDevice::CreateDeviceDescriptor()
{
    USB_DEFAULT_PIPE_SETUP_PACKET CtrlSetup;
    PMDL Mdl;
    NTSTATUS Status;

    //
    // zero descriptor
    //
    RtlZeroMemory(&m_DeviceDescriptor, sizeof(USB_DEVICE_DESCRIPTOR));
    RtlZeroMemory(&CtrlSetup, sizeof(USB_DEFAULT_PIPE_SETUP_PACKET));

    //
    // setup request
    //
    CtrlSetup.bRequest = USB_REQUEST_GET_DESCRIPTOR;
    CtrlSetup.wValue.HiByte = USB_DEVICE_DESCRIPTOR_TYPE;
    CtrlSetup.wLength = sizeof(USB_DEVICE_DESCRIPTOR);
    CtrlSetup.bmRequestType.B = 0x80;

    //
    // allocate mdl describing the device descriptor
    //
    Mdl = IoAllocateMdl(&m_DeviceDescriptor, sizeof(USB_DEVICE_DESCRIPTOR), FALSE, FALSE, 0);
    if (!Mdl)
    {
        //
        // failed to allocate mdl
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // build mdl for non paged pool
    //
    MmBuildMdlForNonPagedPool(Mdl);

    //
    // commit setup packet
    //
    Status = CommitSetupPacket(&CtrlSetup, 0, sizeof(USB_DEVICE_DESCRIPTOR), Mdl);

    //
    // now free the mdl
    //
    IoFreeMdl(Mdl);

    if (NT_SUCCESS(Status))
    {
        //
        // informal dbg print
        //
        DumpDeviceDescriptor(&m_DeviceDescriptor);
    }

    //
    // done
    //
    return Status;

}

//----------------------------------------------------------------------------------------
NTSTATUS
CUSBDevice::CreateConfigurationDescriptor(
    ULONG Index)
{
    PVOID Buffer;
    USB_DEFAULT_PIPE_SETUP_PACKET CtrlSetup;
    NTSTATUS Status;
    PMDL Mdl;
    PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor;


    //
    // sanity checks
    //
    PC_ASSERT(m_ConfigurationDescriptors);

    //
    // first allocate a buffer which should be enough to store all different interfaces and endpoints
    //
    Buffer = ExAllocatePoolWithTag(NonPagedPool, PAGE_SIZE, TAG_USBOHCI);
    if (!Buffer)
    {
        //
        // failed to allocate buffer
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // zero buffer
    //
    RtlZeroMemory(Buffer, PAGE_SIZE);

    //
    // build setup packet
    //
    CtrlSetup.bmRequestType._BM.Recipient = BMREQUEST_TO_DEVICE;
    CtrlSetup.bmRequestType._BM.Type = BMREQUEST_STANDARD;
    CtrlSetup.bmRequestType._BM.Reserved = 0;
    CtrlSetup.bmRequestType._BM.Dir = BMREQUEST_DEVICE_TO_HOST;
    CtrlSetup.bRequest = USB_REQUEST_GET_DESCRIPTOR;
    CtrlSetup.wValue.LowByte = 0;
    CtrlSetup.wValue.HiByte = USB_CONFIGURATION_DESCRIPTOR_TYPE;
    CtrlSetup.wIndex.W = 0;
    CtrlSetup.wLength = PAGE_SIZE;

    //
    // FIXME: where put configuration index?
    //

    //
    // now build MDL describing the buffer
    //
    Mdl = IoAllocateMdl(Buffer, PAGE_SIZE, FALSE, FALSE, 0);
    if (!Mdl)
    {
        //
        // failed to allocate mdl
        //
        ExFreePoolWithTag(Buffer, TAG_USBOHCI);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // build mdl for non paged pool
    //
    MmBuildMdlForNonPagedPool(Mdl);

    //
    // commit packet
    //
    Status = CommitSetupPacket(&CtrlSetup, 0, PAGE_SIZE, Mdl);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to issue request, cleanup
        //
        IoFreeMdl(Mdl);
        ExFreePool(Buffer);
        return Status;
    }

    //
    // now free the mdl
    //
    IoFreeMdl(Mdl);

    //
    // get configuration descriptor
    //
    ConfigurationDescriptor = (PUSB_CONFIGURATION_DESCRIPTOR)Buffer;

    //
    // informal debug print
    //
    DumpConfigurationDescriptor(ConfigurationDescriptor);

    //
    // sanity check
    //
    PC_ASSERT(ConfigurationDescriptor->bLength == sizeof(USB_CONFIGURATION_DESCRIPTOR));
    PC_ASSERT(ConfigurationDescriptor->wTotalLength <= PAGE_SIZE);
    PC_ASSERT(ConfigurationDescriptor->bNumInterfaces);

    //
    // store configuration descriptor
    //
    m_ConfigurationDescriptors[Index] = ConfigurationDescriptor;

    //
    // done
    //
    return Status;
}
//----------------------------------------------------------------------------------------
VOID
CUSBDevice::GetConfigurationDescriptors(
    IN PUSB_CONFIGURATION_DESCRIPTOR ConfigDescriptorBuffer,
    IN ULONG BufferLength,
    OUT PULONG OutBufferLength)
{
    //
    // sanity check
    //
    PC_ASSERT(BufferLength >= sizeof(USB_CONFIGURATION_DESCRIPTOR));
    PC_ASSERT(ConfigDescriptorBuffer);
    PC_ASSERT(OutBufferLength);

    //
    // reset copied length
    //
    *OutBufferLength = 0;

    //
    // FIXME: support multiple configurations
    //
    PC_ASSERT(m_DeviceDescriptor.bNumConfigurations == 1);

    //
    // copy configuration descriptor
    //
    RtlCopyMemory(ConfigDescriptorBuffer, m_ConfigurationDescriptors[0], min(m_ConfigurationDescriptors[0]->wTotalLength, BufferLength));
    *OutBufferLength = m_ConfigurationDescriptors[0]->wTotalLength;
}

//----------------------------------------------------------------------------------------
ULONG
CUSBDevice::GetConfigurationDescriptorsLength()
{
    //
    // FIXME: support multiple configurations
    //
    PC_ASSERT(m_DeviceDescriptor.bNumConfigurations == 1);

    ASSERT(m_ConfigurationDescriptors[0]);
    ASSERT(m_ConfigurationDescriptors[0]->wTotalLength);

    return m_ConfigurationDescriptors[0]->wTotalLength;
}
//----------------------------------------------------------------------------------------
VOID
CUSBDevice::DumpDeviceDescriptor(PUSB_DEVICE_DESCRIPTOR DeviceDescriptor)
{
    DPRINT1("Dumping Device Descriptor %x\n", DeviceDescriptor);
    DPRINT1("bLength %x\n", DeviceDescriptor->bLength);
    DPRINT1("bDescriptorType %x\n", DeviceDescriptor->bDescriptorType);
    DPRINT1("bcdUSB %x\n", DeviceDescriptor->bcdUSB);
    DPRINT1("bDeviceClass %x\n", DeviceDescriptor->bDeviceClass);
    DPRINT1("bDeviceSubClass %x\n", DeviceDescriptor->bDeviceSubClass);
    DPRINT1("bDeviceProtocol %x\n", DeviceDescriptor->bDeviceProtocol);
    DPRINT1("bMaxPacketSize0 %x\n", DeviceDescriptor->bMaxPacketSize0);
    DPRINT1("idVendor %x\n", DeviceDescriptor->idVendor);
    DPRINT1("idProduct %x\n", DeviceDescriptor->idProduct);
    DPRINT1("bcdDevice %x\n", DeviceDescriptor->bcdDevice);
    DPRINT1("iManufacturer %x\n", DeviceDescriptor->iManufacturer);
    DPRINT1("iProduct %x\n", DeviceDescriptor->iProduct);
    DPRINT1("iSerialNumber %x\n", DeviceDescriptor->iSerialNumber);
    DPRINT1("bNumConfigurations %x\n", DeviceDescriptor->bNumConfigurations);
}

//----------------------------------------------------------------------------------------
VOID
CUSBDevice::DumpConfigurationDescriptor(PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor)
{
    DPRINT1("Dumping ConfigurationDescriptor %x\n", ConfigurationDescriptor);
    DPRINT1("bLength %x\n", ConfigurationDescriptor->bLength);
    DPRINT1("bDescriptorType %x\n", ConfigurationDescriptor->bDescriptorType);
    DPRINT1("wTotalLength %x\n", ConfigurationDescriptor->wTotalLength);
    DPRINT1("bNumInterfaces %x\n", ConfigurationDescriptor->bNumInterfaces);
    DPRINT1("bConfigurationValue %x\n", ConfigurationDescriptor->bConfigurationValue);
    DPRINT1("iConfiguration %x\n", ConfigurationDescriptor->iConfiguration);
    DPRINT1("bmAttributes %x\n", ConfigurationDescriptor->bmAttributes);
    DPRINT1("MaxPower %x\n", ConfigurationDescriptor->MaxPower);
}
//----------------------------------------------------------------------------------------
NTSTATUS
CUSBDevice::SubmitSetupPacket(
    IN PUSB_DEFAULT_PIPE_SETUP_PACKET SetupPacket, 
    IN OUT ULONG BufferLength, 
    OUT PVOID Buffer)
{
    NTSTATUS Status;
    PMDL Mdl;

    //
    // allocate mdl
    //
    Mdl = IoAllocateMdl(Buffer, BufferLength, FALSE, FALSE, 0);

    //
    // HACK HACK HACK: assume the buffer is build from non paged pool
    //
    MmBuildMdlForNonPagedPool(Mdl);

    //
    // commit setup packet
    //
    Status = CommitSetupPacket(SetupPacket, 0, BufferLength, Mdl);

    //
    // free mdl
    //
    IoFreeMdl(Mdl);

    //
    // done
    //
    return Status;
}

//----------------------------------------------------------------------------------------
NTSTATUS
CUSBDevice::SelectConfiguration(
    IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
    IN PUSBD_INTERFACE_INFORMATION InterfaceInfo,
    OUT USBD_CONFIGURATION_HANDLE *ConfigurationHandle)
{
    ULONG InterfaceIndex, PipeIndex;
    USB_DEFAULT_PIPE_SETUP_PACKET CtrlSetup;
    NTSTATUS Status;
    PUSB_CONFIGURATION_DESCRIPTOR CurrentConfigurationDescriptor;
    PUSB_INTERFACE_DESCRIPTOR CurrentInterfaceDescriptor;
    PUSB_ENDPOINT_DESCRIPTOR CurrentEndpointDescriptor;
    PVOID StartPosition;

    //
    // FIXME: support multiple configurations
    //
    ASSERT(m_DeviceDescriptor.bNumConfigurations == 1);
    ASSERT(m_ConfigurationDescriptors[0]);
    CurrentConfigurationDescriptor = m_ConfigurationDescriptors[0];

    //
    // sanity check
    //
    PC_ASSERT(ConfigurationDescriptor->iConfiguration == CurrentConfigurationDescriptor->iConfiguration);
    PC_ASSERT(ConfigurationDescriptor->bNumInterfaces <= CurrentConfigurationDescriptor->bNumInterfaces);
    DPRINT1("CUSBDevice::SelectConfiguration NumInterfaces %lu\n", ConfigurationDescriptor->bNumInterfaces);


    //
    // copy interface info and pipe info
    //
    for(InterfaceIndex = 0; InterfaceIndex < ConfigurationDescriptor->bNumInterfaces; InterfaceIndex++)
    {
        //
        // find interface descriptor
        //
        CurrentInterfaceDescriptor = USBD_ParseConfigurationDescriptor(CurrentConfigurationDescriptor, InterfaceInfo->InterfaceNumber, InterfaceInfo->AlternateSetting);

        //
        // sanity check
        //
        ASSERT(CurrentInterfaceDescriptor);
        ASSERT(CurrentInterfaceDescriptor->bLength != 0);
        ASSERT(InterfaceInfo->NumberOfPipes == CurrentInterfaceDescriptor->bNumEndpoints);
        ASSERT(InterfaceInfo->Length != 0);
#ifdef _MSC_VER
        PC_ASSERT(InterfaceInfo->Length == FIELD_OFFSET(USBD_INTERFACE_INFORMATION, Pipes[InterfaceInfo->NumberOfPipes]));
#endif

        DPRINT1("CUSBDevice::SelectConfiguration InterfaceNumber %lu AlternativeSetting %lu bNumEndpoints %lu\n", InterfaceInfo->InterfaceNumber, InterfaceInfo->AlternateSetting, CurrentInterfaceDescriptor->bNumEndpoints);

        //
        // copy interface info
        //
        InterfaceInfo->InterfaceHandle = (USBD_INTERFACE_HANDLE)CurrentInterfaceDescriptor;
        InterfaceInfo->Class = CurrentInterfaceDescriptor->bInterfaceClass;
        InterfaceInfo->SubClass = CurrentInterfaceDescriptor->bInterfaceSubClass;
        InterfaceInfo->Protocol = CurrentInterfaceDescriptor->bInterfaceProtocol;
        InterfaceInfo->Reserved = 0;

        //
        // copy endpoint info
        //
        StartPosition = CurrentInterfaceDescriptor;
        for(PipeIndex = 0; PipeIndex < InterfaceInfo->NumberOfPipes; PipeIndex++)
        {
            //
            // find corresponding endpoint descriptor
            //
            CurrentEndpointDescriptor = (PUSB_ENDPOINT_DESCRIPTOR)USBD_ParseDescriptors(CurrentConfigurationDescriptor, CurrentConfigurationDescriptor->wTotalLength, StartPosition, USB_ENDPOINT_DESCRIPTOR_TYPE);

            //
            // sanity checks
            //
            ASSERT(CurrentEndpointDescriptor);
            ASSERT(CurrentEndpointDescriptor->bDescriptorType == USB_ENDPOINT_DESCRIPTOR_TYPE);

            //
            // copy pipe info
            //
            InterfaceInfo->Pipes[PipeIndex].MaximumPacketSize = CurrentEndpointDescriptor->wMaxPacketSize;
            InterfaceInfo->Pipes[PipeIndex].EndpointAddress = CurrentEndpointDescriptor->bEndpointAddress;
            InterfaceInfo->Pipes[PipeIndex].Interval = CurrentEndpointDescriptor->bInterval;
            InterfaceInfo->Pipes[PipeIndex].PipeType = (USBD_PIPE_TYPE)CurrentEndpointDescriptor->bmAttributes;
            InterfaceInfo->Pipes[PipeIndex].PipeHandle = (PVOID)CurrentEndpointDescriptor;

            //
            // move start position beyond the current endpoint descriptor
            //
            StartPosition = (PVOID)(CurrentEndpointDescriptor + 1);
        }

        //
        // move offset
        //
        InterfaceInfo = (PUSBD_INTERFACE_INFORMATION)((ULONG_PTR)PtrToUlong(InterfaceInfo) + InterfaceInfo->Length);
    }

    //
    // now build setup packet
    //
    RtlZeroMemory(&CtrlSetup, sizeof(USB_DEFAULT_PIPE_SETUP_PACKET));
    CtrlSetup.bRequest = USB_REQUEST_SET_CONFIGURATION;
    CtrlSetup.wValue.W = ConfigurationDescriptor->bConfigurationValue;

    //
    // select configuration
    //
    Status = CommitSetupPacket(&CtrlSetup, 0, 0, 0);

    //
    // informal debug print
    //
    DPRINT1("CUsbDevice::SelectConfiguration New Configuration %x Old Configuration %x Result %x\n", ConfigurationDescriptor->iConfiguration, m_ConfigurationIndex, Status);

    if (NT_SUCCESS(Status))
    {
        //
        // store configuration device index
        //
        m_ConfigurationIndex = ConfigurationDescriptor->iConfiguration;

        //
        // store configuration handle
        //
        *ConfigurationHandle = m_ConfigurationDescriptors[0];
    }

    //
    // done
    //
    return Status;
}

//----------------------------------------------------------------------------------------
NTSTATUS
CUSBDevice::SelectInterface(
    IN USBD_CONFIGURATION_HANDLE ConfigurationHandle,
    IN OUT PUSBD_INTERFACE_INFORMATION InterfaceInfo)
{
    PUSB_CONFIGURATION_DESCRIPTOR Configuration;
    ULONG PipeIndex;
    USB_DEFAULT_PIPE_SETUP_PACKET CtrlSetup;
    NTSTATUS Status;
    PUSB_INTERFACE_DESCRIPTOR InterfaceDescriptor;
    PUSB_ENDPOINT_DESCRIPTOR CurrentEndpointDescriptor;
    PVOID StartPosition;

    //
    // FIXME support multiple configurations
    //
    PC_ASSERT(m_ConfigurationDescriptors[0] == (PUSB_CONFIGURATION_DESCRIPTOR)ConfigurationHandle);

    //
    // get configuration struct
    //
    Configuration = (PUSB_CONFIGURATION_DESCRIPTOR)ConfigurationHandle;

    //
    // sanity checks
    //
    PC_ASSERT(Configuration->bNumInterfaces > InterfaceInfo->InterfaceNumber);
#ifdef _MSC_VER
    //PC_ASSERT(InterfaceInfo->Length == FIELD_OFFSET(USBD_INTERFACE_INFORMATION, Pipes[InterfaceInfo->NumberOfPipes]));
#endif

    //
    // FIXME: check bandwidth
    //

    //
    // find interface number
    //
    InterfaceDescriptor = USBD_ParseConfigurationDescriptor(Configuration, InterfaceInfo->InterfaceNumber, InterfaceInfo->AlternateSetting);
    ASSERT(InterfaceDescriptor);

    //
    // initialize setup packet
    //
    RtlZeroMemory(&CtrlSetup, sizeof(USB_DEFAULT_PIPE_SETUP_PACKET));
    CtrlSetup.bRequest = USB_REQUEST_SET_INTERFACE;
    CtrlSetup.wValue.W = InterfaceDescriptor->bAlternateSetting;
    CtrlSetup.wIndex.W = InterfaceDescriptor->bInterfaceNumber;
    CtrlSetup.bmRequestType.B = 0x01;

    //
    // issue request
    //
    Status = CommitSetupPacket(&CtrlSetup, 0, 0, 0);

    //
    // informal debug print
    //
    DPRINT1("CUSBDevice::SelectInterface AlternateSetting %x InterfaceNumber %x Status %x\n", InterfaceInfo->AlternateSetting, InterfaceInfo->InterfaceNumber, Status);
    DPRINT1("CUSBDevice::SelectInterface bInterfaceNumber %u bAlternateSetting %u NumberOfPipes %u Length %lu\n", 
            InterfaceDescriptor->bInterfaceNumber, InterfaceDescriptor->bAlternateSetting, InterfaceInfo->NumberOfPipes, InterfaceInfo->Length);
    InterfaceInfo->InterfaceHandle =  InterfaceDescriptor;
    InterfaceInfo->NumberOfPipes = InterfaceDescriptor->bNumEndpoints;

    //
    // are there end points
    //
    if (InterfaceDescriptor->bNumEndpoints)
    {
        //
        // sanity check
        //
        ASSERT(InterfaceInfo->Length == sizeof(USBD_INTERFACE_INFORMATION) + (InterfaceDescriptor->bNumEndpoints > 1 ? sizeof(USBD_PIPE_INFORMATION) * (InterfaceDescriptor->bNumEndpoints - 1) : 0));

        //
        // store number of pipes
        //
        InterfaceInfo->NumberOfPipes = InterfaceDescriptor->bNumEndpoints;

        StartPosition = InterfaceDescriptor;
        for(PipeIndex = 0; PipeIndex < InterfaceInfo->NumberOfPipes; PipeIndex++)
        {
            //
            // find corresponding endpoint descriptor
            //
            CurrentEndpointDescriptor = (PUSB_ENDPOINT_DESCRIPTOR)USBD_ParseDescriptors(Configuration, Configuration->wTotalLength, StartPosition, USB_ENDPOINT_DESCRIPTOR_TYPE);

            //
            // sanity checks
            //
            ASSERT(CurrentEndpointDescriptor);
            ASSERT(CurrentEndpointDescriptor->bDescriptorType == USB_ENDPOINT_DESCRIPTOR_TYPE);

            //
            // copy pipe info
            //
            InterfaceInfo->Pipes[PipeIndex].MaximumPacketSize = CurrentEndpointDescriptor->wMaxPacketSize;
            InterfaceInfo->Pipes[PipeIndex].EndpointAddress = CurrentEndpointDescriptor->bEndpointAddress;
            InterfaceInfo->Pipes[PipeIndex].Interval = CurrentEndpointDescriptor->bInterval;
            InterfaceInfo->Pipes[PipeIndex].PipeType = (USBD_PIPE_TYPE)CurrentEndpointDescriptor->bmAttributes;
            InterfaceInfo->Pipes[PipeIndex].PipeHandle = (PVOID)CurrentEndpointDescriptor;

            //
            // move start position beyond the current endpoint descriptor
            //
            StartPosition = (PVOID)(CurrentEndpointDescriptor + 1);
        }
    }
    //
    // done
    //
    return Status;
}

//----------------------------------------------------------------------------------------
NTSTATUS
CreateUSBDevice(
    PUSBDEVICE *OutDevice)
{
    CUSBDevice * This;

    //
    // allocate controller
    //
    This = new(NonPagedPool, TAG_USBOHCI) CUSBDevice(0);
    if (!This)
    {
        //
        // failed to allocate
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // add reference count
    //
    This->AddRef();

    //
    // return result
    //
    *OutDevice = (PUSBDEVICE)This;

    //
    // done
    //
    return STATUS_SUCCESS;
}

