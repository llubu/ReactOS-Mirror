﻿/*
 * PROJECT:         ReactOS Universal Serial Bus Hub Driver
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            drivers/usb/usbhub/fdo.c
 * PURPOSE:         Handle FDO
 * PROGRAMMERS:
 *                  Michael Martin (michael.martin@reactos.org)
 *                  Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#define INITGUID
#include "usbhub.h"

NTSTATUS
QueryStatusChangeEndpoint(
    IN PDEVICE_OBJECT DeviceObject);

NTSTATUS
CreateUsbChildDeviceObject(
    IN PDEVICE_OBJECT UsbHubDeviceObject,
    IN LONG PortId,
    OUT PDEVICE_OBJECT *UsbChildDeviceObject,
    IN ULONG PortStatus);

NTSTATUS
DestroyUsbChildDeviceObject(
    IN PDEVICE_OBJECT UsbHubDeviceObject,
    IN LONG PortId);

NTSTATUS
SubmitRequestToRootHub(
    IN PDEVICE_OBJECT RootHubDeviceObject,
    IN ULONG IoControlCode,
    OUT PVOID OutParameter1,
    OUT PVOID OutParameter2)
{
    KEVENT Event;
    PIRP Irp;
    IO_STATUS_BLOCK IoStatus;
    NTSTATUS Status;
    PIO_STACK_LOCATION Stack = NULL;

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    //
    // Build Control Request
    //
    Irp = IoBuildDeviceIoControlRequest(IoControlCode,
                                        RootHubDeviceObject,
                                        NULL, 0,
                                        NULL, 0,
                                        TRUE,
                                        &Event,
                                        &IoStatus);

    if (Irp == NULL)
    {
        DPRINT("Usbhub: IoBuildDeviceIoControlRequest() failed\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Initialize the status block before sending the IRP
    //
    IoStatus.Status = STATUS_NOT_SUPPORTED;
    IoStatus.Information = 0;

    //
    // Get Next Stack Location and Initialize it
    //
    Stack = IoGetNextIrpStackLocation(Irp);
    Stack->Parameters.Others.Argument1 = OutParameter1;
    Stack->Parameters.Others.Argument2 = OutParameter2;

    //
    // Call RootHub
    //
    Status = IoCallDriver(RootHubDeviceObject, Irp);

    //
    // Its ok to block here as this function is called in an nonarbitrary thread
    //
    if    (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Suspended, KernelMode, FALSE, NULL);
        Status = IoStatus.Status;
    }

    //
    // The IO Manager will free the IRP
    //

    return Status;
}

NTSTATUS
GetPortStatusAndChange(
    IN PDEVICE_OBJECT RootHubDeviceObject,
    IN ULONG PortId,
    OUT PPORT_STATUS_CHANGE StatusChange)
{
    NTSTATUS Status;
    PURB Urb;

    //
    // Allocate URB
    //
    Urb = ExAllocatePoolWithTag(NonPagedPool, sizeof(URB), USB_HUB_TAG);
    if (!Urb)
    {
        DPRINT1("Failed to allocate memory for URB!\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Zero it
    //
    RtlZeroMemory(Urb, sizeof(URB));

    //
    // Initialize URB for getting Port Status
    //
    UsbBuildVendorRequest(Urb,
                          URB_FUNCTION_CLASS_OTHER,
                          sizeof(Urb->UrbControlVendorClassRequest),
                          USBD_TRANSFER_DIRECTION_OUT,
                          0,
                          USB_REQUEST_GET_STATUS,
                          0,
                          PortId,
                          StatusChange,
                          0,
                          sizeof(PORT_STATUS_CHANGE),
                          0);

    //
    // Query the Root Hub
    //
    Status = SubmitRequestToRootHub(RootHubDeviceObject, IOCTL_INTERNAL_USB_SUBMIT_URB, Urb, NULL);

    //
    // Free URB
    //
    ExFreePool(Urb);

    return Status;
}

NTSTATUS
SetPortFeature(
    IN PDEVICE_OBJECT RootHubDeviceObject,
    IN ULONG PortId,
    IN ULONG Feature)
{
    NTSTATUS Status;
    PURB Urb;

    //
    // Allocate URB
    //
    Urb = ExAllocatePoolWithTag(NonPagedPool, sizeof(URB), USB_HUB_TAG);
    if (!Urb)
    {
        DPRINT1("Failed to allocate memory for URB!\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Zero it
    //
    RtlZeroMemory(Urb, sizeof(URB));

    //
    // Initialize URB for Clearing Port Reset
    //
    UsbBuildVendorRequest(Urb,
                          URB_FUNCTION_CLASS_OTHER,
                          sizeof(Urb->UrbControlVendorClassRequest),
                          USBD_TRANSFER_DIRECTION_IN,
                          0,
                          USB_REQUEST_SET_FEATURE,
                          Feature,
                          PortId,
                          NULL,
                          0,
                          0,
                          0);
    //
    // Query the Root Hub
    //
    Status = SubmitRequestToRootHub(RootHubDeviceObject, IOCTL_INTERNAL_USB_SUBMIT_URB, Urb, NULL);

    //
    // Free URB
    //
    ExFreePool(Urb);

    return Status;
}

NTSTATUS
ClearPortFeature(
    IN PDEVICE_OBJECT RootHubDeviceObject,
    IN ULONG PortId,
    IN ULONG Feature)
{
    NTSTATUS Status;
    PURB Urb;

    //
    // Allocate a URB
    //
    Urb = ExAllocatePoolWithTag(NonPagedPool, sizeof(URB), USB_HUB_TAG);
    if (!Urb)
    {
        DPRINT1("Failed to allocate memory for URB!\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Zero it
    //
    RtlZeroMemory(Urb, sizeof(URB));

    //
    // Initialize URB for Clearing Port Reset
    //
    UsbBuildVendorRequest(Urb,
                          URB_FUNCTION_CLASS_OTHER,
                          sizeof(Urb->UrbControlVendorClassRequest),
                          USBD_TRANSFER_DIRECTION_IN,
                          0,
                          USB_REQUEST_CLEAR_FEATURE,
                          Feature,
                          PortId,
                          NULL,
                          0,
                          0,
                          0);
    //
    // Query the Root Hub
    //
    Status = SubmitRequestToRootHub(RootHubDeviceObject, IOCTL_INTERNAL_USB_SUBMIT_URB, Urb, NULL);

    //
    // Free URB
    //
    ExFreePool(Urb);

    return Status;
}

VOID NTAPI
DeviceStatusChangeThread(
    IN PVOID Context)
{
    NTSTATUS Status;
    PDEVICE_OBJECT DeviceObject, RootHubDeviceObject;
    PHUB_DEVICE_EXTENSION HubDeviceExtension;
    PWORK_ITEM_DATA WorkItemData;
    PORT_STATUS_CHANGE PortStatus;
    LONG PortId;
    BOOLEAN SignalResetComplete = FALSE;

    DPRINT("Entered DeviceStatusChangeThread, Context %x\n", Context);

    WorkItemData = (PWORK_ITEM_DATA)Context;
    DeviceObject = (PDEVICE_OBJECT)WorkItemData->Context;
    HubDeviceExtension = (PHUB_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    RootHubDeviceObject = HubDeviceExtension->RootHubPhysicalDeviceObject;
    //
    // Loop all ports
    //
    for (PortId = 1; PortId <= HubDeviceExtension->UsbExtHubInfo.NumberOfPorts; PortId++)
    {
        //
        // Get Port Status
        //
        Status = GetPortStatusAndChange(RootHubDeviceObject, PortId, &PortStatus);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to get port status for port %d, Status %x\n", PortId, Status);
            // FIXME: Do we really want to halt further SCE requests?
            return;
        }

        DPRINT1("Port %d Status %x\n", PortId, PortStatus.Status);
        DPRINT1("Port %d Change %x\n", PortId, PortStatus.Change);


        //
        // Check for new device connection
        //
        if (PortStatus.Change & USB_PORT_STATUS_CONNECT)
        {
            //
            // Clear Port Connect 
            //
            Status = ClearPortFeature(RootHubDeviceObject, PortId, C_PORT_CONNECTION);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Failed to clear connection change for port %d\n", PortId);
                continue;
            }

            //
            // Is this a connect or disconnect?
            //
            if (!(PortStatus.Status & USB_PORT_STATUS_CONNECT))
            {
                DPRINT1("Device disconnected from port %d\n", PortId);

                Status = DestroyUsbChildDeviceObject(DeviceObject, PortId);
                if (!NT_SUCCESS(Status))
                {
                    DPRINT1("Failed to delete child device object after disconnect\n");
                    continue;
                }
            }
            else
            {
                DPRINT1("Device connected from port %d\n", PortId);

                // No SCE completion done for clearing C_PORT_CONNECT

                //
                // Reset Port
                //
                Status = SetPortFeature(RootHubDeviceObject, PortId, PORT_RESET);
                if (!NT_SUCCESS(Status))
                {
                    DPRINT1("Failed to reset port %d\n", PortId);
                    continue;
                }
            }
        }
        else if (PortStatus.Change & USB_PORT_STATUS_ENABLE)
        {
            //
            // Clear Enable
            //
            Status = ClearPortFeature(RootHubDeviceObject, PortId, C_PORT_ENABLE);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Failed to clear enable change on port %d\n", PortId);
                continue;
            }
        }
        else if (PortStatus.Change & USB_PORT_STATUS_RESET)
        {
            //
            // Clear Reset
            //
            Status = ClearPortFeature(RootHubDeviceObject, PortId, C_PORT_RESET);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Failed to clear reset change on port %d\n", PortId);
                continue;
            }

            //
            // Get Port Status
            //
            Status = GetPortStatusAndChange(RootHubDeviceObject, PortId, &PortStatus);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Failed to get port status for port %d, Status %x\n", PortId, Status);
                // FIXME: Do we really want to halt further SCE requests?
                return;
            }

            DPRINT1("Port %d Status %x\n", PortId, PortStatus.Status);
            DPRINT1("Port %d Change %x\n", PortId, PortStatus.Change);

            //
            // Check that reset was cleared
            //
            if(PortStatus.Change & USB_PORT_STATUS_RESET)
            {
                DPRINT1("Port did not clear reset! Possible Hardware problem!\n");
                continue;
            }

            //
            // Check if the device is still connected
            //
            if (!(PortStatus.Status & USB_PORT_STATUS_CONNECT))
            {
                DPRINT1("Device has been disconnected\n");
                continue;
            }

            //
            // Make sure its Connected and Enabled
            //
            if (!(PortStatus.Status & (USB_PORT_STATUS_CONNECT | USB_PORT_STATUS_ENABLE)))
            {
                DPRINT1("Usb Device is not connected and enabled!\n");
                //
                // Attempt another reset
                //
                Status = SetPortFeature(RootHubDeviceObject, PortId, PORT_RESET);
                if (!NT_SUCCESS(Status))
                {
                    DPRINT1("Failed to reset port %d\n", PortId);
                }
                continue;
            }

            //
            // This is a new device
            //
            Status = CreateUsbChildDeviceObject(DeviceObject, PortId, NULL, PortStatus.Status);

            //
            // Request event signalling later
            //
            SignalResetComplete = TRUE;
        }
    }

    ExFreePool(WorkItemData);

    //
    // Send another SCE Request
    //
    DPRINT("Sending another SCE!\n");
    QueryStatusChangeEndpoint(DeviceObject);

    //
    // Check if a reset event was satisfied
    //
    if (SignalResetComplete)
    {
        //
        // Signal anyone waiting on it
        //
        KeSetEvent(&HubDeviceExtension->ResetComplete, IO_NO_INCREMENT, FALSE);
    }
}

NTSTATUS
NTAPI
StatusChangeEndpointCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context)
{
    PDEVICE_OBJECT RealDeviceObject;
    PHUB_DEVICE_EXTENSION HubDeviceExtension;
    PWORK_ITEM_DATA WorkItemData;

    RealDeviceObject = (PDEVICE_OBJECT)Context;
    HubDeviceExtension = (PHUB_DEVICE_EXTENSION)RealDeviceObject->DeviceExtension;

    //
    // NOTE: USBPORT frees this IRP
    //
    DPRINT("Received Irp %x, HubDeviceExtension->PendingSCEIrp %x\n", Irp, HubDeviceExtension->PendingSCEIrp);
    //IoFreeIrp(Irp);

    //
    // Create and initialize work item data
    //
    WorkItemData = ExAllocatePoolWithTag(NonPagedPool, sizeof(WORK_ITEM_DATA), USB_HUB_TAG);
    if (!WorkItemData)
    {
        DPRINT1("Failed to allocate memory!n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    WorkItemData->Context = RealDeviceObject;

    DPRINT("Queuing work item\n");

    //
    // Queue the work item to handle initializing the device
    //
    ExInitializeWorkItem(&WorkItemData->WorkItem, DeviceStatusChangeThread, (PVOID)WorkItemData);
    ExQueueWorkItem(&WorkItemData->WorkItem, DelayedWorkQueue);

    //
    // Return more processing required so the IO Manger doesn’t try to mess with IRP just freed
    //
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
QueryStatusChangeEndpoint(
    IN PDEVICE_OBJECT DeviceObject)
{
    NTSTATUS Status;
    PDEVICE_OBJECT RootHubDeviceObject;
    PIO_STACK_LOCATION Stack;
    PHUB_DEVICE_EXTENSION HubDeviceExtension;
    PURB PendingSCEUrb;

    HubDeviceExtension = (PHUB_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    RootHubDeviceObject = HubDeviceExtension->RootHubPhysicalDeviceObject;

    //
    // Allocate a URB
    //
    PendingSCEUrb = ExAllocatePoolWithTag(NonPagedPool, sizeof(URB), USB_HUB_TAG);

    //
    // Initialize URB for Status Change Endpoint request
    //
    UsbBuildInterruptOrBulkTransferRequest(PendingSCEUrb,
                                           sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER),
                                           HubDeviceExtension->PipeHandle,
                                           HubDeviceExtension->PortStatusChange,
                                           NULL,
                                           sizeof(USHORT) * 2 * HubDeviceExtension->UsbExtHubInfo.NumberOfPorts,
                                           USBD_TRANSFER_DIRECTION_IN | USBD_SHORT_TRANSFER_OK,
                                           NULL);

    //
    // Set the device handle to null for roothub
    //
    PendingSCEUrb->UrbHeader.UsbdDeviceHandle = NULL;//HubDeviceExtension->RootHubHandle;

    //
    // Allocate an Irp
    //

    HubDeviceExtension->PendingSCEIrp = ExAllocatePoolWithTag(NonPagedPool,
                                                              IoSizeOfIrp(RootHubDeviceObject->StackSize),
                                                              USB_HUB_TAG);
/*
    HubDeviceExtension->PendingSCEIrp = IoAllocateIrp(RootHubDeviceObject->StackSize,
                                  FALSE);
*/
    DPRINT("Allocated IRP %x\n", HubDeviceExtension->PendingSCEIrp);

    if (!HubDeviceExtension->PendingSCEIrp)
    {
        DPRINT1("USBHUB: Failed to allocate IRP for SCE request!\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Initialize the IRP
    //
    IoInitializeIrp(HubDeviceExtension->PendingSCEIrp,
                    IoSizeOfIrp(RootHubDeviceObject->StackSize),
                    RootHubDeviceObject->StackSize);

    HubDeviceExtension->PendingSCEIrp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    HubDeviceExtension->PendingSCEIrp->IoStatus.Information = 0;
    HubDeviceExtension->PendingSCEIrp->Flags = 0;
    HubDeviceExtension->PendingSCEIrp->UserBuffer = NULL;

    //
    // Get the Next Stack Location and Initialize it
    //
    Stack = IoGetNextIrpStackLocation(HubDeviceExtension->PendingSCEIrp);
    Stack->DeviceObject = DeviceObject;
    Stack->Parameters.Others.Argument1 = PendingSCEUrb;
    Stack->Parameters.Others.Argument2 = NULL;
    Stack->MajorFunction =  IRP_MJ_INTERNAL_DEVICE_CONTROL;
    Stack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;

    //
    // Set the completion routine for when device is connected to root hub
    //
    IoSetCompletionRoutine(HubDeviceExtension->PendingSCEIrp,
                           StatusChangeEndpointCompletion,
                           DeviceObject,
                           TRUE,
                           TRUE,
                           TRUE);

    //
    // Send to RootHub
    //
    DPRINT("DeviceObject is %x\n", DeviceObject);
    DPRINT("Iocalldriver %x with irp %x\n", RootHubDeviceObject, HubDeviceExtension->PendingSCEIrp);
    Status = IoCallDriver(RootHubDeviceObject, HubDeviceExtension->PendingSCEIrp);

    return STATUS_PENDING;
}

NTSTATUS
QueryInterface(
    IN PDEVICE_OBJECT DeviceObject,
    IN CONST GUID InterfaceType,
    IN LONG Size,
    IN LONG Version,
    OUT PVOID Interface)
{
    KEVENT Event;
    PIRP Irp;
    IO_STATUS_BLOCK IoStatus;
    NTSTATUS Status;
    PIO_STACK_LOCATION Stack = NULL;

    //
    // Initialize the Event used to wait for Irp completion
    //
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    //
    // Build Control Request
    //
    Irp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP,
                                       DeviceObject,
                                       NULL,
                                       0,
                                       NULL,
                                       &Event,
                                       &IoStatus);

    //
    // Get Next Stack Location and Initialize it.
    //
    Stack = IoGetNextIrpStackLocation(Irp);
    Stack->MinorFunction = IRP_MN_QUERY_INTERFACE;
    Stack->Parameters.QueryInterface.InterfaceType= &InterfaceType;//USB_BUS_INTERFACE_HUB_GUID;
    Stack->Parameters.QueryInterface.Size = Size;
    Stack->Parameters.QueryInterface.Version = Version;
    Stack->Parameters.QueryInterface.Interface = Interface;
    Stack->Parameters.QueryInterface.InterfaceSpecificData = NULL;

    Status = IoCallDriver(DeviceObject, Irp);

    if (Status == STATUS_PENDING)
    {
        DPRINT("Operation pending\n");
        KeWaitForSingleObject(&Event, Suspended, KernelMode, FALSE, NULL);
        Status = IoStatus.Status;
    }

    return Status;
}

NTSTATUS
GetUsbDeviceDescriptor(
    IN PDEVICE_OBJECT ChildDeviceObject,
    IN UCHAR DescriptorType,
    IN UCHAR Index,
    IN USHORT LangId,
    OUT PVOID TransferBuffer,
    IN ULONG TransferBufferLength)
{
    NTSTATUS Status;
    PDEVICE_OBJECT RootHubDeviceObject;
    PURB Urb;
    PHUB_DEVICE_EXTENSION HubDeviceExtension;
    PHUB_CHILDDEVICE_EXTENSION ChildDeviceExtension;

    //
    // Get the Hubs Device Extension
    //
    ChildDeviceExtension = (PHUB_CHILDDEVICE_EXTENSION)ChildDeviceObject->DeviceExtension;
    HubDeviceExtension = (PHUB_DEVICE_EXTENSION) ChildDeviceExtension->ParentDeviceObject->DeviceExtension;
    RootHubDeviceObject = HubDeviceExtension->RootHubPhysicalDeviceObject;

    //
    // Allocate a URB
    //
    Urb = ExAllocatePoolWithTag(NonPagedPool, sizeof(URB), USB_HUB_TAG);
    if (!Urb)
    {
        DPRINT1("Failed to allocate memory for URB!\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Zero it
    //
    RtlZeroMemory(Urb, sizeof(URB));

    //
    // Initialize URB for getting device descriptor
    //
    UsbBuildGetDescriptorRequest(Urb,
                                 sizeof(Urb->UrbControlDescriptorRequest),
                                 DescriptorType,
                                 Index,
                                 LangId,
                                 TransferBuffer,
                                 NULL,
                                 TransferBufferLength,
                                 NULL);

    //
    // Set the device handle
    //
    Urb->UrbHeader.UsbdDeviceHandle = (PVOID)ChildDeviceExtension->UsbDeviceHandle;

    //
    // Query the Root Hub
    //
    Status = SubmitRequestToRootHub(RootHubDeviceObject,
                                    IOCTL_INTERNAL_USB_SUBMIT_URB,
                                    Urb,
                                    NULL);

    return Status;
}

NTSTATUS
GetUsbStringDescriptor(
    IN PDEVICE_OBJECT ChildDeviceObject,
    IN UCHAR Index,
    IN USHORT LangId,
    OUT PVOID *TransferBuffer,
    OUT USHORT *Size)
{
    NTSTATUS Status;
    PUSB_STRING_DESCRIPTOR StringDesc = NULL;
    ULONG SizeNeeded;
    LPWSTR Buffer;

    StringDesc = ExAllocatePoolWithTag(NonPagedPool,
                                       sizeof(USB_STRING_DESCRIPTOR),
                                       USB_HUB_TAG);
    if (!StringDesc)
    {
        DPRINT1("Failed to allocate buffer for string!\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Get the index string descriptor length
    // FIXME: Implement LangIds
    //
    Status = GetUsbDeviceDescriptor(ChildDeviceObject,
                                    USB_STRING_DESCRIPTOR_TYPE,
                                    Index,
                                    0x0409,
                                    StringDesc,
                                    sizeof(USB_STRING_DESCRIPTOR));
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("GetUsbDeviceDescriptor failed with status %x\n", Status);
        ExFreePool(StringDesc);
        return Status;
    }
    DPRINT1("StringDesc->bLength %d\n", StringDesc->bLength);

    //
    // Did we get something more than the length of the first two fields of structure?
    //
    if (StringDesc->bLength == 2)
    {
        DPRINT1("USB Device Error!\n");
        ExFreePool(StringDesc);
        return STATUS_DEVICE_DATA_ERROR;
    }
    SizeNeeded = StringDesc->bLength + sizeof(WCHAR);

    //
    // Free String
    //
    ExFreePool(StringDesc);

    //
    // Recreate with appropriate size
    //
    StringDesc = ExAllocatePoolWithTag(NonPagedPool,
                                       SizeNeeded,
                                       USB_HUB_TAG);
    if (!StringDesc)
    {
        DPRINT1("Failed to allocate buffer for string!\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(StringDesc, SizeNeeded);

    //
    // Get the string
    //
    Status = GetUsbDeviceDescriptor(ChildDeviceObject,
                                    USB_STRING_DESCRIPTOR_TYPE,
                                    Index,
                                    0x0409,
                                    StringDesc,
                                    SizeNeeded);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("GetUsbDeviceDescriptor failed with status %x\n", Status);
        ExFreePool(StringDesc);
        return Status;
    }

    //
    // Allocate Buffer to return
    //
    Buffer = ExAllocatePoolWithTag(NonPagedPool,
                                   SizeNeeded,
                                   USB_HUB_TAG);
    if (!Buffer)
    {
        DPRINT1("Failed to allocate buffer for string!\n");
        ExFreePool(StringDesc);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
	DPRINT("Buffer %p\n", Buffer);
    RtlZeroMemory(Buffer, SizeNeeded);

	DPRINT("SizeNeeded %lu\n", SizeNeeded);
	DPRINT("Offset %lu\n", FIELD_OFFSET(USB_STRING_DESCRIPTOR, bLength));
    DPRINT("Length %lu\n", SizeNeeded - FIELD_OFFSET(USB_STRING_DESCRIPTOR, bLength));

    //
    // Copy the string to destination
    //
    RtlCopyMemory(Buffer, StringDesc->bString, SizeNeeded - FIELD_OFFSET(USB_STRING_DESCRIPTOR, bString));
    *Size = SizeNeeded;
    *TransferBuffer = Buffer;

    ExFreePool(StringDesc);

    return STATUS_SUCCESS;
}

ULONG
IsCompositeDevice(
    IN PUSB_DEVICE_DESCRIPTOR DeviceDescriptor,
    IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor)
{
    if (DeviceDescriptor->bNumConfigurations != 1)
    {
         //
         // composite device must have only one configuration
         //
         DPRINT1("IsCompositeDevice bNumConfigurations %x\n", DeviceDescriptor->bNumConfigurations);
         return FALSE;
    }

    if (ConfigurationDescriptor->bNumInterfaces < 2)
    {
        //
        // composite device must have multiple interfaces
        //
        DPRINT1("IsCompositeDevice bNumInterfaces %x\n", ConfigurationDescriptor->bNumInterfaces);
        return FALSE;
    }

    if (DeviceDescriptor->bDeviceClass == 0)
    {
        //
        // composite device
        //
        ASSERT(DeviceDescriptor->bDeviceSubClass == 0);
        ASSERT(DeviceDescriptor->bDeviceProtocol == 0);
        DPRINT1("IsCompositeDevice: TRUE\n");
        return TRUE;
    }

    if (DeviceDescriptor->bDeviceClass == 0xEF && 
        DeviceDescriptor->bDeviceSubClass == 0x02 &&
        DeviceDescriptor->bDeviceProtocol == 0x01)
    {
        //
        // USB-IF association descriptor
        //
        DPRINT1("IsCompositeDevice: TRUE\n");
        return TRUE;
    }

    DPRINT1("DeviceDescriptor bDeviceClass %x bDeviceSubClass %x bDeviceProtocol %x\n", DeviceDescriptor->bDeviceClass, DeviceDescriptor->bDeviceSubClass, DeviceDescriptor->bDeviceProtocol);

    //
    // not a composite device
    //
    return FALSE;
}

NTSTATUS
CreateDeviceIds(
    PDEVICE_OBJECT UsbChildDeviceObject)
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Index = 0;
    LPWSTR DeviceString;
    WCHAR Buffer[200];
    PHUB_CHILDDEVICE_EXTENSION UsbChildExtension;
    PUSB_DEVICE_DESCRIPTOR DeviceDescriptor;
    PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor;
    PUSB_INTERFACE_DESCRIPTOR InterfaceDescriptor;

    //
    // get child device extension
    //
    UsbChildExtension = (PHUB_CHILDDEVICE_EXTENSION)UsbChildDeviceObject->DeviceExtension;

    //
    // get device descriptor
    //
    DeviceDescriptor = &UsbChildExtension->DeviceDesc;

    //
    // get configuration descriptor
    //
    ConfigurationDescriptor = UsbChildExtension->FullConfigDesc;

    //
    // use first interface descriptor available
    //
    InterfaceDescriptor = USBD_ParseConfigurationDescriptorEx(ConfigurationDescriptor, ConfigurationDescriptor, 0, -1, -1, -1, -1);
    ASSERT(InterfaceDescriptor);

    //
    // Construct the CompatibleIds
    //
    if (IsCompositeDevice(DeviceDescriptor, ConfigurationDescriptor))
    {
        //
        // sanity checks
        //
        ASSERT(DeviceDescriptor->bNumConfigurations == 1);
        ASSERT(ConfigurationDescriptor->bNumInterfaces > 1);
        Index += swprintf(&Buffer[Index],
                          L"USB\\DevClass_%02x&SubClass_%02x&Prot_%02x",
                          DeviceDescriptor->bDeviceClass, DeviceDescriptor->bDeviceSubClass, DeviceDescriptor->bDeviceProtocol) + 1;
        Index += swprintf(&Buffer[Index],
                          L"USB\\DevClass_%02x&SubClass_%02x",
                          DeviceDescriptor->bDeviceClass, DeviceDescriptor->bDeviceSubClass) + 1;
        Index += swprintf(&Buffer[Index],
                          L"USB\\DevClass_%02x",
                          DeviceDescriptor->bDeviceClass) + 1;
        Index += swprintf(&Buffer[Index],
                          L"USB\\COMPOSITE") + 1;
    }
    else
    {
        //
        // sanity checks for simple usb device
        //
        ASSERT(ConfigurationDescriptor->bNumInterfaces == 1);

        //
        // FIXME: support multiple configurations
        //
        ASSERT(DeviceDescriptor->bNumConfigurations == 1);

        if (DeviceDescriptor->bDeviceClass == 0)
        {
            Index += swprintf(&Buffer[Index], 
                          L"USB\\Class_%02x&SubClass_%02x&Prot_%02x",
                          InterfaceDescriptor->bInterfaceClass, InterfaceDescriptor->bInterfaceSubClass, InterfaceDescriptor->bInterfaceProtocol) + 1;
            Index += swprintf(&Buffer[Index],
                          L"USB\\Class_%02x&SubClass_%02x",
                          InterfaceDescriptor->bInterfaceClass, InterfaceDescriptor->bInterfaceSubClass) + 1;
            Index += swprintf(&Buffer[Index],
                          L"USB\\Class_%02x",
                          InterfaceDescriptor->bInterfaceClass) + 1;
        }
        else
        {
            Index += swprintf(&Buffer[Index], 
                          L"USB\\Class_%02x&SubClass_%02x&Prot_%02x",
                          DeviceDescriptor->bDeviceClass, DeviceDescriptor->bDeviceSubClass, DeviceDescriptor->bDeviceProtocol) + 1;
            Index += swprintf(&Buffer[Index],
                          L"USB\\Class_%02x&SubClass_%02x",
                          DeviceDescriptor->bDeviceClass, DeviceDescriptor->bDeviceSubClass) + 1;
            Index += swprintf(&Buffer[Index],
                          L"USB\\Class_%02x",
                          DeviceDescriptor->bDeviceClass) + 1;
        }
    }

    //
    // now allocate the buffer
    //
    DeviceString = ExAllocatePool(NonPagedPool, (Index + 1) * sizeof(WCHAR));
    if (!DeviceString)
    {
        //
        // no memory
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // copy buffer
    //
    RtlCopyMemory(DeviceString, Buffer, Index * sizeof(WCHAR));
    DeviceString[Index] = UNICODE_NULL;
    UsbChildExtension->usCompatibleIds.Buffer = DeviceString;
    UsbChildExtension->usCompatibleIds.Length = Index * sizeof(WCHAR);
    UsbChildExtension->usCompatibleIds.MaximumLength = (Index + 1) * sizeof(WCHAR);
    DPRINT("usCompatibleIds %wZ\n", &UsbChildExtension->usCompatibleIds);

    //
    // Construct DeviceId string
    //
    Index = swprintf(Buffer, L"USB\\Vid_%04x&Pid_%04x", UsbChildExtension->DeviceDesc.idVendor, UsbChildExtension->DeviceDesc.idProduct) + 1;

    //
    // now allocate the buffer
    //
    DeviceString = ExAllocatePool(NonPagedPool, Index * sizeof(WCHAR));
    if (!DeviceString)
    {
        //
        // no memory
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // copy buffer
    //
    RtlCopyMemory(DeviceString, Buffer, Index * sizeof(WCHAR));
    UsbChildExtension->usDeviceId.Buffer = DeviceString;
    UsbChildExtension->usDeviceId.Length = (Index-1) * sizeof(WCHAR);
    UsbChildExtension->usDeviceId.MaximumLength = Index * sizeof(WCHAR);
    DPRINT("usDeviceId %wZ\n", &UsbChildExtension->usDeviceId);

    //
    // Construct HardwareIds
    //
    Index = 0;
    Index += swprintf(&Buffer[Index], 
                      L"USB\\Vid_%04x&Pid_%04x&Rev_%04x",
                      UsbChildExtension->DeviceDesc.idVendor, UsbChildExtension->DeviceDesc.idProduct, UsbChildExtension->DeviceDesc.bcdDevice) + 1;
    Index += swprintf(&Buffer[Index],
                      L"USB\\Vid_%04x&Pid_%04x",
                      UsbChildExtension->DeviceDesc.idVendor, UsbChildExtension->DeviceDesc.idProduct) + 1;

    //
    // now allocate the buffer
    //
    DeviceString = ExAllocatePool(NonPagedPool, (Index + 1) * sizeof(WCHAR));
    if (!DeviceString)
    {
        //
        // no memory
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // copy buffer
    //
    RtlCopyMemory(DeviceString, Buffer, Index * sizeof(WCHAR));
    DeviceString[Index] = UNICODE_NULL;
    UsbChildExtension->usHardwareIds.Buffer = DeviceString;
    UsbChildExtension->usHardwareIds.Length = (Index + 1) * sizeof(WCHAR);
    UsbChildExtension->usHardwareIds.MaximumLength = (Index + 1) * sizeof(WCHAR);
    DPRINT("usHardWareIds %wZ\n", &UsbChildExtension->usHardwareIds);

    //
    // FIXME: Handle Lang ids
    //

    //
    // Get the product string if obe provided
    //
    if (UsbChildExtension->DeviceDesc.iProduct)
    {
        Status = GetUsbStringDescriptor(UsbChildDeviceObject,
                                        UsbChildExtension->DeviceDesc.iProduct,
                                        0,
                                        (PVOID*)&UsbChildExtension->usTextDescription.Buffer,
                                        &UsbChildExtension->usTextDescription.Length);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("USBHUB: GetUsbStringDescriptor failed with status %x\n", Status);
            RtlInitUnicodeString(&UsbChildExtension->usTextDescription, L"");
        }
        else
        {
            UsbChildExtension->usTextDescription.MaximumLength = UsbChildExtension->usTextDescription.Length;
            DPRINT("Usb TextDescription %wZ\n", &UsbChildExtension->usTextDescription);
        }
    }

    //
    // Get the Serial Number string if obe provided
    //
    if (UsbChildExtension->DeviceDesc.iSerialNumber)
    {
        Status = GetUsbStringDescriptor(UsbChildDeviceObject,
                                        UsbChildExtension->DeviceDesc.iSerialNumber,
                                        0,
                                        (PVOID*)&UsbChildExtension->usInstanceId.Buffer,
                                        &UsbChildExtension->usInstanceId.Length);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("USBHUB: GetUsbStringDescriptor failed with status %x\n", Status);
            return Status;
        }

        UsbChildExtension->usInstanceId.MaximumLength = UsbChildExtension->usInstanceId.Length;
        DPRINT("Usb InstanceId %wZ\n", &UsbChildExtension->usInstanceId);
    }
    else
    {
       //
       // the device did not provide a serial number, lets create a pseudo instance id
       //
       Index = swprintf(Buffer, L"0&%04d", UsbChildExtension->PortNumber) + 1;
       UsbChildExtension->usInstanceId.Buffer = (LPWSTR)ExAllocatePool(NonPagedPool, Index * sizeof(WCHAR));
       if (UsbChildExtension->usInstanceId.Buffer == NULL)
       {
           DPRINT1("Error: failed to allocate %lu bytes\n", Index * sizeof(WCHAR));
           Status = STATUS_INSUFFICIENT_RESOURCES;
           return Status;
       }

       //
       // copy instance id
       //
       RtlCopyMemory(UsbChildExtension->usInstanceId.Buffer, Buffer, Index * sizeof(WCHAR));
       UsbChildExtension->usInstanceId.Length = UsbChildExtension->usInstanceId.MaximumLength = Index * sizeof(WCHAR);

       DPRINT("usDeviceId %wZ\n", &UsbChildExtension->usInstanceId);
    }

    return Status;
}

NTSTATUS
DestroyUsbChildDeviceObject(
    IN PDEVICE_OBJECT UsbHubDeviceObject,
    IN LONG PortId)
{
    PHUB_DEVICE_EXTENSION HubDeviceExtension = (PHUB_DEVICE_EXTENSION)UsbHubDeviceObject->DeviceExtension;
    PHUB_CHILDDEVICE_EXTENSION UsbChildExtension = NULL;
    PDEVICE_OBJECT ChildDeviceObject = NULL;
    ULONG Index = 0;

    DPRINT("Removing device on port %d (Child index: %d)\n", PortId, Index);

    for (Index = 0; Index < USB_MAXCHILDREN; Index++)
    {
        if (HubDeviceExtension->ChildDeviceObject[Index])
        {
            UsbChildExtension = (PHUB_CHILDDEVICE_EXTENSION)HubDeviceExtension->ChildDeviceObject[Index]->DeviceExtension;

            /* Check if it matches the port ID */
            if (UsbChildExtension->PortNumber == PortId)
            {
                /* We found it */
                ChildDeviceObject = HubDeviceExtension->ChildDeviceObject[Index];
                break;
            }
        }
    }

    /* Fail the request if the device doesn't exist */
    if (!ChildDeviceObject)
    {
        DPRINT1("Removal request for non-existant device!\n");
        return STATUS_UNSUCCESSFUL;
    }

    /* Remove the device from the table */
    HubDeviceExtension->ChildDeviceObject[Index] = NULL;

    /* Invalidate device relations for the root hub */
    IoInvalidateDeviceRelations(HubDeviceExtension->RootHubPhysicalDeviceObject, BusRelations);

    /* The rest of the removal process takes place in IRP_MN_REMOVE_DEVICE handling for the PDO */
    return STATUS_SUCCESS;
}

NTSTATUS
CreateUsbChildDeviceObject(
    IN PDEVICE_OBJECT UsbHubDeviceObject,
    IN LONG PortId,
    OUT PDEVICE_OBJECT *UsbChildDeviceObject,
    IN ULONG PortStatus)
{
    NTSTATUS Status;
    PDEVICE_OBJECT RootHubDeviceObject, NewChildDeviceObject;
    PHUB_DEVICE_EXTENSION HubDeviceExtension;
    PHUB_CHILDDEVICE_EXTENSION UsbChildExtension;
    PUSB_BUS_INTERFACE_HUB_V5 HubInterface;
    ULONG ChildDeviceCount, UsbDeviceNumber = 0;
    WCHAR CharDeviceName[64];
    UNICODE_STRING DeviceName;
    ULONG ConfigDescSize, DeviceDescSize;
    PVOID HubInterfaceBusContext;
    USB_CONFIGURATION_DESCRIPTOR ConfigDesc;

    HubDeviceExtension = (PHUB_DEVICE_EXTENSION) UsbHubDeviceObject->DeviceExtension;
    HubInterface = &HubDeviceExtension->HubInterface;
    RootHubDeviceObject = HubDeviceExtension->RootHubPhysicalDeviceObject;
    HubInterfaceBusContext = HubDeviceExtension->UsbDInterface.BusContext;
    //
    // Find an empty slot in the child device array 
    //
    for (ChildDeviceCount = 0; ChildDeviceCount < USB_MAXCHILDREN; ChildDeviceCount++)
    {
        if (HubDeviceExtension->ChildDeviceObject[ChildDeviceCount] == NULL)
        {
        DPRINT("Found unused entry at %d\n", ChildDeviceCount);
            break;
        }
    }

    //
    // Check if the limit has been reached for maximum usb devices
    //
    if (ChildDeviceCount == USB_MAXCHILDREN)
    {
        DPRINT1("USBHUB: Too many child devices!\n");
        return STATUS_UNSUCCESSFUL;
    }

    while (TRUE)
    {
        //
        // Create a Device Name
        //
        swprintf(CharDeviceName, L"\\Device\\USBPDO-%d", UsbDeviceNumber);

        //
        // Initialize UnicodeString
        //
        RtlInitUnicodeString(&DeviceName, CharDeviceName);

        //
        // Create a DeviceObject
        //
        Status = IoCreateDevice(UsbHubDeviceObject->DriverObject,
                                sizeof(HUB_CHILDDEVICE_EXTENSION),
                                NULL,
                                FILE_DEVICE_CONTROLLER,
                                FILE_AUTOGENERATED_DEVICE_NAME,
                                FALSE,
                                &NewChildDeviceObject);

        //
        // Check if the name is already in use
        //
        if ((Status == STATUS_OBJECT_NAME_EXISTS) || (Status == STATUS_OBJECT_NAME_COLLISION))
        {
            //
            // Try next name
            //
            UsbDeviceNumber++;
            continue;
        }

        //
        // Check for other errors
        //
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("USBHUB: IoCreateDevice failed with status %x\n", Status);
            return Status;
        }

        DPRINT("USBHUB: Created Device %x\n", NewChildDeviceObject);
        break;
    }

    NewChildDeviceObject->Flags |= DO_BUS_ENUMERATED_DEVICE;

    //
    // Assign the device extensions
    //
    UsbChildExtension = (PHUB_CHILDDEVICE_EXTENSION)NewChildDeviceObject->DeviceExtension;
    RtlZeroMemory(UsbChildExtension, sizeof(HUB_CHILDDEVICE_EXTENSION));
    UsbChildExtension->ParentDeviceObject = UsbHubDeviceObject;
    UsbChildExtension->PortNumber = PortId;

    //
    // Create the UsbDeviceObject
    //
    Status = HubInterface->CreateUsbDevice(HubInterfaceBusContext,
                                           (PVOID)&UsbChildExtension->UsbDeviceHandle,
                                           HubDeviceExtension->RootHubHandle,
                                           PortStatus,
                                           PortId);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("USBHUB: CreateUsbDevice failed with status %x\n", Status);
        goto Cleanup;
    }

    //
    // Initialize UsbDevice
    //
    Status = HubInterface->InitializeUsbDevice(HubInterfaceBusContext, UsbChildExtension->UsbDeviceHandle);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("USBHUB: InitializeUsbDevice failed with status %x\n", Status);
        goto Cleanup;
    }

    DPRINT("Usb Device Handle %x\n", UsbChildExtension->UsbDeviceHandle);

    ConfigDescSize = sizeof(USB_CONFIGURATION_DESCRIPTOR);
    DeviceDescSize = sizeof(USB_DEVICE_DESCRIPTOR);

    //
    // Get the descriptors
    //
    Status = HubInterface->GetUsbDescriptors(HubInterfaceBusContext,
                                             UsbChildExtension->UsbDeviceHandle,
                                             (PUCHAR)&UsbChildExtension->DeviceDesc,
                                             &DeviceDescSize,
                                             (PUCHAR)&ConfigDesc,
                                             &ConfigDescSize);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("USBHUB: GetUsbDescriptors failed with status %x\n", Status);
        goto Cleanup;
    }

    DumpDeviceDescriptor(&UsbChildExtension->DeviceDesc);
    DumpConfigurationDescriptor(&ConfigDesc);

    //
    // FIXME: Support more than one configuration and one interface?
    //
    if (UsbChildExtension->DeviceDesc.bNumConfigurations > 1)
    {
        DPRINT1("Warning: Device has more than one configuration. Only one configuration (the first) is supported!\n");
    }

    if (ConfigDesc.bNumInterfaces > 1)
    {
        DPRINT1("Warning: Device has more that one interface. Only one interface (the first) is currently supported\n");
    }

    ConfigDescSize = ConfigDesc.wTotalLength;

    //
    // Allocate memory for the first full descriptor, including interfaces and endpoints.
    //
    UsbChildExtension->FullConfigDesc = ExAllocatePoolWithTag(PagedPool, ConfigDescSize, USB_HUB_TAG);

    //
    // Retrieve the full configuration descriptor
    //
    Status = GetUsbDeviceDescriptor(NewChildDeviceObject,
                                    USB_CONFIGURATION_DESCRIPTOR_TYPE,
                                    0,
                                    0,
                                    UsbChildExtension->FullConfigDesc,
                                    ConfigDescSize);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("USBHUB: GetUsbDeviceDescriptor failed with status %x\n", Status);
        goto Cleanup;
    }

    //DumpFullConfigurationDescriptor(UsbChildExtension->FullConfigDesc);

    //
    // Construct all the strings that will described the device to PNP
    //
    Status = CreateDeviceIds(NewChildDeviceObject);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create strings needed to describe device to PNP.\n");
        goto Cleanup;
    }

    HubDeviceExtension->ChildDeviceObject[ChildDeviceCount] = NewChildDeviceObject;

    IoInvalidateDeviceRelations(RootHubDeviceObject, BusRelations);
    return STATUS_SUCCESS;

Cleanup:

    //
    // Remove the usb device if it was created
    //
    if (UsbChildExtension->UsbDeviceHandle)
        HubInterface->RemoveUsbDevice(HubInterfaceBusContext, UsbChildExtension->UsbDeviceHandle, 0);

    //
    // Free full configuration descriptor if one was allocated
    //
    if (UsbChildExtension->FullConfigDesc)
        ExFreePool(UsbChildExtension->FullConfigDesc);

    //
    // Delete the device object
    //
    IoDeleteDevice(NewChildDeviceObject);
    return Status;
}

NTSTATUS
USBHUB_FdoQueryBusRelations(
    IN PDEVICE_OBJECT DeviceObject,
    OUT PDEVICE_RELATIONS* pDeviceRelations)
{
    PHUB_DEVICE_EXTENSION HubDeviceExtension;
    PDEVICE_RELATIONS DeviceRelations;
    ULONG i;
    ULONG Children = 0;
    ULONG NeededSize;

    HubDeviceExtension = (PHUB_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    //
    // Count the number of children
    //
    for (i = 0; i < USB_MAXCHILDREN; i++)
    {

        if (HubDeviceExtension->ChildDeviceObject[i] == NULL)
        {
            continue;
        }
        Children++;
    }

    NeededSize = sizeof(DEVICE_RELATIONS);
    if (Children > 1)
        NeededSize += (Children - 1) * sizeof(PDEVICE_OBJECT);

    //
    // Allocate DeviceRelations
    //
    DeviceRelations = (PDEVICE_RELATIONS)ExAllocatePool(PagedPool,
                                                        NeededSize);

    if (!DeviceRelations)
        return STATUS_INSUFFICIENT_RESOURCES;
    DeviceRelations->Count = Children;
    Children = 0;

    //
    // Fill in return structure
    //
    for (i = 0; i < USB_MAXCHILDREN; i++)
    {
        if (HubDeviceExtension->ChildDeviceObject[i])
        {
            ObReferenceObject(HubDeviceExtension->ChildDeviceObject[i]);
            HubDeviceExtension->ChildDeviceObject[i]->Flags &= ~DO_DEVICE_INITIALIZING;
            DeviceRelations->Objects[Children++] = HubDeviceExtension->ChildDeviceObject[i];
        }
    }

    ASSERT(Children == DeviceRelations->Count);
    *pDeviceRelations = DeviceRelations;

    return STATUS_SUCCESS;
}

VOID
NTAPI
RootHubInitCallbackFunction(
    PVOID Context)
{
    PDEVICE_OBJECT DeviceObject = (PDEVICE_OBJECT)Context;
    NTSTATUS Status;
    ULONG PortId;
    PHUB_DEVICE_EXTENSION HubDeviceExtension;
    PORT_STATUS_CHANGE StatusChange;

    HubDeviceExtension = (PHUB_DEVICE_EXTENSION) DeviceObject->DeviceExtension;

    DPRINT("RootHubInitCallbackFunction Sending the initial SCE Request %x\n", DeviceObject);

    //
    // Send the first SCE Request
    //
    QueryStatusChangeEndpoint(DeviceObject);

    for (PortId = 1; PortId <= HubDeviceExtension->HubDescriptor.bNumberOfPorts; PortId++)
    {
        //
        // get port status
        //
        Status = GetPortStatusAndChange(HubDeviceExtension->RootHubPhysicalDeviceObject, PortId, &StatusChange);
        if (NT_SUCCESS(Status))
        {
            //
            // is there a device connected
            //
            if (StatusChange.Status & USB_PORT_STATUS_CONNECT)
            {
                //
                // reset port
                //
                Status = SetPortFeature(HubDeviceExtension->RootHubPhysicalDeviceObject, PortId, PORT_RESET);
                if (!NT_SUCCESS(Status))
                {
                    DPRINT1("Failed to reset on port %d\n", PortId);
                }
                else
                {
                    //
                    // wait for the reset to be handled since we want to enumerate synchronously
                    //
                    KeWaitForSingleObject(&HubDeviceExtension->ResetComplete,
                                          Executive,
                                          KernelMode,
                                          FALSE,
                                          NULL);
                    KeClearEvent(&HubDeviceExtension->ResetComplete);
                }
            }
        }
    }
}

NTSTATUS
USBHUB_FdoHandlePnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION Stack;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG_PTR Information = 0;
    PHUB_DEVICE_EXTENSION HubDeviceExtension;
    PDEVICE_OBJECT RootHubDeviceObject;
    PVOID HubInterfaceBusContext , UsbDInterfaceBusContext;
    PORT_STATUS_CHANGE StatusChange;

    HubDeviceExtension = (PHUB_DEVICE_EXTENSION) DeviceObject->DeviceExtension;

    Stack = IoGetCurrentIrpStackLocation(Irp);

    switch (Stack->MinorFunction)
    {
        case IRP_MN_START_DEVICE:
        {
            PURB Urb;
            PUSB_INTERFACE_DESCRIPTOR Pid;
            ULONG Result = 0, PortId;
            USBD_INTERFACE_LIST_ENTRY InterfaceList[2] = {{NULL, NULL}, {NULL, NULL}};
            PURB ConfigUrb = NULL;
            ULONG HubStatus;

            DPRINT("IRP_MJ_PNP / IRP_MN_START_DEVICE\n");

            //
            // Allocated size including the sizeof USBD_INTERFACE_LIST_ENTRY
            //
            Urb = ExAllocatePoolWithTag(NonPagedPool, sizeof(URB) + sizeof(USBD_INTERFACE_LIST_ENTRY), USB_HUB_TAG);
            RtlZeroMemory(Urb, sizeof(URB) + sizeof(USBD_INTERFACE_LIST_ENTRY));

            //
            // Get the Root Hub Pdo
            //
            SubmitRequestToRootHub(HubDeviceExtension->LowerDeviceObject,
                                   IOCTL_INTERNAL_USB_GET_ROOTHUB_PDO,
                                   &HubDeviceExtension->RootHubPhysicalDeviceObject,
                                   &HubDeviceExtension->RootHubFunctionalDeviceObject);

            RootHubDeviceObject = HubDeviceExtension->RootHubPhysicalDeviceObject;
            ASSERT(HubDeviceExtension->RootHubPhysicalDeviceObject);
            ASSERT(HubDeviceExtension->RootHubFunctionalDeviceObject);
            DPRINT("RootPdo %x, RootFdo %x\n",
                    HubDeviceExtension->RootHubPhysicalDeviceObject,
                    HubDeviceExtension->RootHubFunctionalDeviceObject);

            //
            // Send the StartDevice to RootHub
            //
            Status = ForwardIrpAndWait(RootHubDeviceObject, Irp);

            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Failed to start the RootHub PDO\n");
                ASSERT(FALSE);
            }

            //
            // Get the current number of hubs
            //
            Status = SubmitRequestToRootHub(RootHubDeviceObject,
                                            IOCTL_INTERNAL_USB_GET_HUB_COUNT,
                                            &HubDeviceExtension->NumberOfHubs, NULL);

            //
            // Get the Hub Interface
            //
            Status = QueryInterface(RootHubDeviceObject,
                                    USB_BUS_INTERFACE_HUB_GUID,
                                    sizeof(USB_BUS_INTERFACE_HUB_V5),
                                    USB_BUSIF_HUB_VERSION_5,
                                    (PVOID)&HubDeviceExtension->HubInterface);

            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Failed to get HUB_GUID interface with status 0x%08lx\n", Status);
                return STATUS_UNSUCCESSFUL;
            }

            HubInterfaceBusContext = HubDeviceExtension->HubInterface.BusContext;

            //
            // Get the USBDI Interface
            //
            Status = QueryInterface(RootHubDeviceObject,
                                    USB_BUS_INTERFACE_USBDI_GUID,
                                    sizeof(USB_BUS_INTERFACE_USBDI_V2),
                                    USB_BUSIF_USBDI_VERSION_2,
                                    (PVOID)&HubDeviceExtension->UsbDInterface);

            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Failed to get USBDI_GUID interface with status 0x%08lx\n", Status);
                return Status;
            }

            UsbDInterfaceBusContext = HubDeviceExtension->UsbDInterface.BusContext;

            //
            // Get Root Hub Device Handle
            //
            Status = SubmitRequestToRootHub(RootHubDeviceObject,
                                            IOCTL_INTERNAL_USB_GET_DEVICE_HANDLE,
                                            &HubDeviceExtension->RootHubHandle,
                                            NULL);

            if (!NT_SUCCESS(Status))
            {
                DPRINT1("GetRootHubDeviceHandle failed with status 0x%08lx\n", Status);
                return Status;
            }

            //
            // Get Hub Device Information
            //
            Status = HubDeviceExtension->HubInterface.QueryDeviceInformation(HubInterfaceBusContext,
                                                                             HubDeviceExtension->RootHubHandle,
                                                                             &HubDeviceExtension->DeviceInformation,
                                                                             sizeof(USB_DEVICE_INFORMATION_0),
                                                                             &Result);

            DPRINT1("Status %x, Result 0x%08lx\n", Status, Result);
            DPRINT1("InformationLevel %x\n", HubDeviceExtension->DeviceInformation.InformationLevel);
            DPRINT1("ActualLength %x\n", HubDeviceExtension->DeviceInformation.ActualLength);
            DPRINT1("PortNumber %x\n", HubDeviceExtension->DeviceInformation.PortNumber);
            DPRINT1("DeviceDescriptor %x\n", HubDeviceExtension->DeviceInformation.DeviceDescriptor);
            DPRINT1("HubAddress %x\n", HubDeviceExtension->DeviceInformation.HubAddress);
            DPRINT1("NumberofPipes %x\n", HubDeviceExtension->DeviceInformation.NumberOfOpenPipes);

            //
            // Get Root Hubs Device Descriptor
            //
            UsbBuildGetDescriptorRequest(Urb,
                                         sizeof(Urb->UrbControlDescriptorRequest),
                                         USB_DEVICE_DESCRIPTOR_TYPE,
                                         0,
                                         0,
                                         &HubDeviceExtension->HubDeviceDescriptor,
                                         NULL,
                                         sizeof(USB_DEVICE_DESCRIPTOR),
                                         NULL);

            Urb->UrbHeader.UsbdDeviceHandle = NULL;//HubDeviceExtension->RootHubHandle;

            Status = SubmitRequestToRootHub(RootHubDeviceObject,
                                            IOCTL_INTERNAL_USB_SUBMIT_URB,
                                            Urb,
                                            NULL);

            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Failed to get HubDeviceDescriptor!\n");
            }

            DumpDeviceDescriptor(&HubDeviceExtension->HubDeviceDescriptor);

            //
            // Get Root Hubs Configuration Descriptor
            //
            UsbBuildGetDescriptorRequest(Urb,
                                         sizeof(Urb->UrbControlDescriptorRequest),
                                         USB_CONFIGURATION_DESCRIPTOR_TYPE,
                                         0,
                                         0,
                                         &HubDeviceExtension->HubConfigDescriptor,
                                         NULL,
                                         sizeof(USB_CONFIGURATION_DESCRIPTOR) + sizeof(USB_INTERFACE_DESCRIPTOR) + sizeof(USB_ENDPOINT_DESCRIPTOR),
                                         NULL);

            DPRINT("RootHub Handle %x\n", HubDeviceExtension->RootHubHandle);
            Urb->UrbHeader.UsbdDeviceHandle = NULL;//HubDeviceExtension->RootHubHandle;

            Status = SubmitRequestToRootHub(RootHubDeviceObject,
                                            IOCTL_INTERNAL_USB_SUBMIT_URB,
                                            Urb,
                                            NULL);

            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Failed to get RootHub Configuration with status %x\n", Status);
                ASSERT(FALSE);
            }
            ASSERT(HubDeviceExtension->HubConfigDescriptor.wTotalLength);

            DumpConfigurationDescriptor(&HubDeviceExtension->HubConfigDescriptor);

            Status = HubDeviceExtension->HubInterface.GetExtendedHubInformation(HubInterfaceBusContext,
                                                                                RootHubDeviceObject,
                                                                                &HubDeviceExtension->UsbExtHubInfo,
                                                                                sizeof(USB_EXTHUB_INFORMATION_0),
                                                                                &Result);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Failed to extended hub information. Unable to determine the number of ports!\n");
                ASSERT(FALSE);
            }

            DPRINT1("HubDeviceExtension->UsbExtHubInfo.NumberOfPorts %x\n", HubDeviceExtension->UsbExtHubInfo.NumberOfPorts);

            //
            // Get the Hub Descriptor
            //
            UsbBuildVendorRequest(Urb,
                                  URB_FUNCTION_CLASS_DEVICE,
                                  sizeof(Urb->UrbControlVendorClassRequest),
                                  USBD_TRANSFER_DIRECTION_IN | USBD_SHORT_TRANSFER_OK,
                                  0,
                                  USB_REQUEST_GET_DESCRIPTOR,
                                  USB_DEVICE_CLASS_RESERVED,
                                  0,
                                  &HubDeviceExtension->HubDescriptor,
                                  NULL,
                                  sizeof(USB_HUB_DESCRIPTOR),
                                  NULL);

            Urb->UrbHeader.UsbdDeviceHandle = NULL;//HubDeviceExtension->RootHubHandle;

            Status = SubmitRequestToRootHub(RootHubDeviceObject,
                                            IOCTL_INTERNAL_USB_SUBMIT_URB,
                                            Urb,
                                            NULL);

            DPRINT1("bDescriptorType %x\n", HubDeviceExtension->HubDescriptor.bDescriptorType);

            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Failed to get Hub Descriptor!\n");
                ExFreePool(Urb);
                return STATUS_UNSUCCESSFUL;
            }

            HubStatus = 0;
            UsbBuildGetStatusRequest(Urb,
                                     URB_FUNCTION_GET_STATUS_FROM_DEVICE,
                                     0,
                                     &HubStatus,
                                     0,
                                     NULL);
            Urb->UrbHeader.UsbdDeviceHandle = NULL;//HubDeviceExtension->RootHubHandle;

            Status = SubmitRequestToRootHub(RootHubDeviceObject,
                                            IOCTL_INTERNAL_USB_SUBMIT_URB,
                                            Urb,
                                            NULL);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Failed to get Hub Status!\n");
                ExFreePool(Urb);
                return STATUS_UNSUCCESSFUL;
            }

            DPRINT1("HubStatus %x\n", HubStatus);

            //
            // Allocate memory for PortStatusChange to hold 2 USHORTs for each port on hub
            //
            HubDeviceExtension->PortStatusChange = ExAllocatePoolWithTag(NonPagedPool,
                                                                         sizeof(ULONG) * HubDeviceExtension->UsbExtHubInfo.NumberOfPorts,
                                                                         USB_HUB_TAG);

            //
            // Get the first Configuration Descriptor
            //
            Pid = USBD_ParseConfigurationDescriptorEx(&HubDeviceExtension->HubConfigDescriptor,
                                                      &HubDeviceExtension->HubConfigDescriptor,
                                                     -1, -1, -1, -1, -1);

            ASSERT(Pid != NULL);

            InterfaceList[0].InterfaceDescriptor = Pid;
            ConfigUrb = USBD_CreateConfigurationRequestEx(&HubDeviceExtension->HubConfigDescriptor,
                                                          (PUSBD_INTERFACE_LIST_ENTRY)&InterfaceList);
            ASSERT(ConfigUrb != NULL);

            Status = SubmitRequestToRootHub(RootHubDeviceObject,
                                            IOCTL_INTERNAL_USB_SUBMIT_URB,
                                            ConfigUrb,
                                            NULL);

            HubDeviceExtension->ConfigurationHandle = ConfigUrb->UrbSelectConfiguration.ConfigurationHandle;
            HubDeviceExtension->PipeHandle = ConfigUrb->UrbSelectConfiguration.Interface.Pipes[0].PipeHandle;
            DPRINT("Configuration Handle %x\n", HubDeviceExtension->ConfigurationHandle);

            //
            // check if function is available
            //
            if (HubDeviceExtension->UsbDInterface.IsDeviceHighSpeed)
            {
                //
                // is it high speed bus
                //
                if (HubDeviceExtension->UsbDInterface.IsDeviceHighSpeed(HubInterfaceBusContext))
                {
                    //
                    // initialize usb 2.0 hub
                    //
                    Status = HubDeviceExtension->HubInterface.Initialize20Hub(HubInterfaceBusContext,
                                                                              HubDeviceExtension->RootHubHandle, 1);
                    DPRINT("Status %x\n", Status);

                    //
                    // FIXME handle error
                    //
                    ASSERT(Status == STATUS_SUCCESS);
                }
            }

            ExFreePool(ConfigUrb);

            //
            // Enable power on all ports
            //

            DPRINT("Enabling PortPower on all ports!\n");

            for (PortId = 1; PortId <= HubDeviceExtension->HubDescriptor.bNumberOfPorts; PortId++)
            {
                Status = SetPortFeature(RootHubDeviceObject, PortId, PORT_POWER);
                if (!NT_SUCCESS(Status))
                    DPRINT1("Failed to power on port %d\n", PortId);

                Status = ClearPortFeature(RootHubDeviceObject, PortId, C_PORT_CONNECTION);
                if (!NT_SUCCESS(Status))
                    DPRINT1("Failed to power on port %d\n", PortId);
            }

            DPRINT("RootHubInitNotification %x\n", HubDeviceExtension->HubInterface.RootHubInitNotification);

            //
            // init root hub notification
            //
            if (HubDeviceExtension->HubInterface.RootHubInitNotification)
            {
                Status = HubDeviceExtension->HubInterface.RootHubInitNotification(HubInterfaceBusContext,
                                                                                  DeviceObject,
                                                                                  RootHubInitCallbackFunction);
                if (!NT_SUCCESS(Status))
                {
                    DPRINT1("Failed to set callback\n");
                }
            }
            else
            {
                //
                // Send the first SCE Request
                //
                QueryStatusChangeEndpoint(DeviceObject);

                //
                // reset ports
                //
                for (PortId = 1; PortId <= HubDeviceExtension->HubDescriptor.bNumberOfPorts; PortId++)
                {
                    //
                    // get port status
                    //
                    Status = GetPortStatusAndChange(HubDeviceExtension->RootHubPhysicalDeviceObject, PortId, &StatusChange);
                    if (NT_SUCCESS(Status))
                    {
                        //
                        // is there a device connected
                        //
                        if (StatusChange.Status & USB_PORT_STATUS_CONNECT)
                        {
                            //
                            // reset port
                            //
                            Status = SetPortFeature(HubDeviceExtension->RootHubPhysicalDeviceObject, PortId, PORT_RESET);
                            if (!NT_SUCCESS(Status))
                            {
                                DPRINT1("Failed to reset on port %d\n", PortId);
                            }
                            else
                            {
                                //
                                // wait for the reset to be handled since we want to enumerate synchronously
                                //
                                KeWaitForSingleObject(&HubDeviceExtension->ResetComplete,
                                                      Executive,
                                                      KernelMode,
                                                      FALSE,
                                                      NULL);
                                KeClearEvent(&HubDeviceExtension->ResetComplete);
                            }
                        }
                    }
                }
            }

            ExFreePool(Urb);
            break;
        }

        case IRP_MN_QUERY_DEVICE_RELATIONS:
        {
            switch (Stack->Parameters.QueryDeviceRelations.Type)
            {
                case BusRelations:
                {
                    PDEVICE_RELATIONS DeviceRelations = NULL;
                    DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_RELATIONS / BusRelations\n");

                    Status = USBHUB_FdoQueryBusRelations(DeviceObject, &DeviceRelations);

                    Information = (ULONG_PTR)DeviceRelations;
                    break;
                }
                case RemovalRelations:
                {
                    DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_RELATIONS / RemovalRelations\n");
                    return ForwardIrpAndForget(DeviceObject, Irp);
                }
                default:
                    DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_RELATIONS / Unknown type 0x%lx\n",
                            Stack->Parameters.QueryDeviceRelations.Type);
                    return ForwardIrpAndForget(DeviceObject, Irp);
            }
            break;
        }
        case IRP_MN_QUERY_REMOVE_DEVICE:
        case IRP_MN_QUERY_STOP_DEVICE:
        {
            Irp->IoStatus.Status = STATUS_SUCCESS;
            return ForwardIrpAndForget(DeviceObject, Irp);
        }
        case IRP_MN_REMOVE_DEVICE:
        {
            Irp->IoStatus.Status = STATUS_SUCCESS;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);

            IoDetachDevice(HubDeviceExtension->LowerDeviceObject);
            IoDeleteDevice(DeviceObject);

            return STATUS_SUCCESS;
        }
        case IRP_MN_QUERY_BUS_INFORMATION:
        {
            DPRINT("IRP_MN_QUERY_BUS_INFORMATION\n");
            break;
        }
        case IRP_MN_QUERY_ID:
        {
            DPRINT("IRP_MN_QUERY_ID\n");
            break;
        }
        case IRP_MN_QUERY_CAPABILITIES:
        {
            DPRINT("IRP_MN_QUERY_CAPABILITIES\n");
            break;
        }
        default:
        {
            DPRINT(" IRP_MJ_PNP / unknown minor function 0x%lx\n", Stack->MinorFunction);
            return ForwardIrpAndForget(DeviceObject, Irp);
        }
    }

    Irp->IoStatus.Information = Information;
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

NTSTATUS
USBHUB_FdoHandleDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    DPRINT1("FdoHandleDeviceControl\n");
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

