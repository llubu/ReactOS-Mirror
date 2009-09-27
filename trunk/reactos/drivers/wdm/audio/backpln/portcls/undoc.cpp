/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/backpln/portcls/api.cpp
 * PURPOSE:         Port api functions
 * PROGRAMMER:      Johannes Anderwald
 */

#include "private.hpp"

NTSTATUS
NTAPI
KsoDispatchCreateWithGenericFactory(
    LONG Unknown,
    PIRP Irp)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

IIrpTarget *
NTAPI
KsoGetIrpTargetFromFileObject(
    PFILE_OBJECT FileObject)
{
    PC_ASSERT(FileObject);

    // IrpTarget is stored in FsContext
    return (IIrpTarget*)FileObject->FsContext;
}

IIrpTarget *
NTAPI
KsoGetIrpTargetFromIrp(
    PIRP Irp)
{
    PKSOBJECT_CREATE_ITEM CreateItem;

    // access the create item
    CreateItem = KSCREATE_ITEM_IRP_STORAGE(Irp);

    // IIrpTarget is stored in Context member
    return (IIrpTarget*)CreateItem->Context;
}

VOID
NTAPI
PcAcquireFormatResources(
    LONG Unknown,
    LONG Unknown2,
    LONG Unknown3,
    LONG Unknown4)
{
    UNIMPLEMENTED;
}

NTSTATUS
PcAddToEventTable(
    PVOID Ptr,
    LONG Unknown2,
    ULONG Length,
    LONG Unknown3,
    LONG Unknown4,
    LONG Unknown5,
    LONG Unknown6,
    LONG Unknown7)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
PcAddToPropertyTable(
    PVOID Ptr,
    LONG Unknown,
    LONG Unknown2,
    LONG Unknown3,
    CHAR Unknown4)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
PcCaptureFormat(
    LONG Unknown,
    LONG Unknown2,
    LONG Unknown3,
    LONG Unknown4)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
AddToPropertyTable(
    IN OUT SUBDEVICE_DESCRIPTOR * Descriptor,
    IN KSPROPERTY_SET * FilterProperty)
{
    if (Descriptor->FilterPropertySet.FreeKsPropertySetOffset >= Descriptor->FilterPropertySet.MaxKsPropertySetCount)
    {
        DPRINT1("FIXME\n");
        return STATUS_UNSUCCESSFUL;
    }

    RtlMoveMemory(&Descriptor->FilterPropertySet.Properties[Descriptor->FilterPropertySet.FreeKsPropertySetOffset],
                  FilterProperty,
                  sizeof(KSPROPERTY_SET));

    if (FilterProperty->PropertiesCount)
    {
        Descriptor->FilterPropertySet.Properties[Descriptor->FilterPropertySet.FreeKsPropertySetOffset].PropertyItem = (const KSPROPERTY_ITEM*)AllocateItem(NonPagedPool,
                                                                                                                                    sizeof(KSPROPERTY_ITEM) * FilterProperty->PropertiesCount,
                                                                                                                                   TAG_PORTCLASS);

        if (!Descriptor->FilterPropertySet.Properties[Descriptor->FilterPropertySet.FreeKsPropertySetOffset].PropertyItem)
        {
            Descriptor->FilterPropertySet.Properties[Descriptor->FilterPropertySet.FreeKsPropertySetOffset].PropertiesCount = 0;
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        RtlMoveMemory((PVOID)Descriptor->FilterPropertySet.Properties[Descriptor->FilterPropertySet.FreeKsPropertySetOffset].PropertyItem,
                      FilterProperty->PropertyItem,
                      sizeof(KSPROPERTY_ITEM) * FilterProperty->PropertiesCount);

    }
    Descriptor->FilterPropertySet.Properties[Descriptor->FilterPropertySet.FreeKsPropertySetOffset].Set = (const GUID *)AllocateItem(NonPagedPool, sizeof(GUID), TAG_PORTCLASS);
    if (!Descriptor->FilterPropertySet.Properties[Descriptor->FilterPropertySet.FreeKsPropertySetOffset].Set)
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlCopyMemory((PVOID)Descriptor->FilterPropertySet.Properties[Descriptor->FilterPropertySet.FreeKsPropertySetOffset].Set, FilterProperty->Set, sizeof(GUID));

    // ignore fast io table for now
    Descriptor->FilterPropertySet.Properties[Descriptor->FilterPropertySet.FreeKsPropertySetOffset].FastIoCount = 0;
    Descriptor->FilterPropertySet.Properties[Descriptor->FilterPropertySet.FreeKsPropertySetOffset].FastIoTable = NULL;

    Descriptor->FilterPropertySet.FreeKsPropertySetOffset++;

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
PcCreateSubdeviceDescriptor(
    OUT SUBDEVICE_DESCRIPTOR ** OutSubdeviceDescriptor,
    IN ULONG InterfaceCount,
    IN GUID * InterfaceGuids,
    IN ULONG IdentifierCount,
    IN KSIDENTIFIER *Identifier,
    IN ULONG FilterPropertiesCount,
    IN KSPROPERTY_SET * FilterProperties,
    IN ULONG Unknown1,
    IN ULONG Unknown2,
    IN ULONG PinPropertiesCount,
    IN KSPROPERTY_SET * PinProperties,
    IN ULONG EventSetCount,
    IN KSEVENT_SET * EventSet,
    IN PPCFILTER_DESCRIPTOR FilterDescription)
{
    SUBDEVICE_DESCRIPTOR * Descriptor;
    ULONG Index, SubIndex;
    PKSDATARANGE DataRange;
    NTSTATUS Status = STATUS_INSUFFICIENT_RESOURCES;

    Descriptor = (PSUBDEVICE_DESCRIPTOR)AllocateItem(NonPagedPool, sizeof(SUBDEVICE_DESCRIPTOR), TAG_PORTCLASS);
    if (!Descriptor)
        return STATUS_INSUFFICIENT_RESOURCES;

    // initialize physical / symbolic link connection list 
    InitializeListHead(&Descriptor->SymbolicLinkList);
    InitializeListHead(&Descriptor->PhysicalConnectionList);

    Descriptor->Interfaces = (GUID*)AllocateItem(NonPagedPool, sizeof(GUID) * InterfaceCount, TAG_PORTCLASS);
    if (!Descriptor->Interfaces)
        goto cleanup;

    // copy interface guids
    RtlCopyMemory(Descriptor->Interfaces, InterfaceGuids, sizeof(GUID) * InterfaceCount);
    Descriptor->InterfaceCount = InterfaceCount;

    if (FilterPropertiesCount)
    {
       /// FIXME
       /// handle driver properties
       Descriptor->FilterPropertySet.Properties = (PKSPROPERTY_SET)AllocateItem(NonPagedPool, sizeof(KSPROPERTY_SET) * FilterPropertiesCount, TAG_PORTCLASS);
       if (! Descriptor->FilterPropertySet.Properties)
           goto cleanup;

       Descriptor->FilterPropertySet.MaxKsPropertySetCount = FilterPropertiesCount;
       for(Index = 0; Index < FilterPropertiesCount; Index++)
       {
           Status = AddToPropertyTable(Descriptor, &FilterProperties[Index]);
           if (!NT_SUCCESS(Status))
               goto cleanup;
       }
    }

    Descriptor->Topology = (PKSTOPOLOGY)AllocateItem(NonPagedPool, sizeof(KSTOPOLOGY), TAG_PORTCLASS);
    if (!Descriptor->Topology)
        goto cleanup;

    if (FilterDescription->ConnectionCount)
    {
        Descriptor->Topology->TopologyConnections = (PKSTOPOLOGY_CONNECTION)AllocateItem(NonPagedPool, sizeof(KSTOPOLOGY_CONNECTION) * FilterDescription->ConnectionCount, TAG_PORTCLASS);
        if (!Descriptor->Topology->TopologyConnections)
            goto cleanup;

        RtlMoveMemory((PVOID)Descriptor->Topology->TopologyConnections, FilterDescription->Connections, FilterDescription->ConnectionCount * sizeof(PCCONNECTION_DESCRIPTOR));
        Descriptor->Topology->TopologyConnectionsCount = FilterDescription->ConnectionCount;
    }

    if (FilterDescription->NodeCount)
    {
        Descriptor->Topology->TopologyNodes = (const GUID *)AllocateItem(NonPagedPool, sizeof(GUID) * FilterDescription->NodeCount, TAG_PORTCLASS);
        if (!Descriptor->Topology->TopologyNodes)
            goto cleanup;

        Descriptor->Topology->TopologyNodesNames = (const GUID *)AllocateItem(NonPagedPool, sizeof(GUID) * FilterDescription->NodeCount, TAG_PORTCLASS);
        if (!Descriptor->Topology->TopologyNodesNames)
            goto cleanup;

        for(Index = 0; Index < FilterDescription->NodeCount; Index++)
        {
            if (FilterDescription->Nodes[Index].Type)
            {
                RtlMoveMemory((PVOID)&Descriptor->Topology->TopologyNodes[Index], FilterDescription->Nodes[Index].Type, sizeof(GUID));
            }
            if (FilterDescription->Nodes[Index].Name)
            {
                RtlMoveMemory((PVOID)&Descriptor->Topology->TopologyNodesNames[Index], FilterDescription->Nodes[Index].Name, sizeof(GUID));
            }
        }
        Descriptor->Topology->TopologyNodesCount = FilterDescription->NodeCount;
    }

    if (FilterDescription->PinCount)
    {
        Descriptor->Factory.KsPinDescriptor = (PKSPIN_DESCRIPTOR)AllocateItem(NonPagedPool, FilterDescription->PinSize * FilterDescription->PinCount, TAG_PORTCLASS);
        if (!Descriptor->Factory.KsPinDescriptor)
            goto cleanup;

        Descriptor->Factory.Instances = (PPIN_INSTANCE_INFO)AllocateItem(NonPagedPool, FilterDescription->PinCount * sizeof(PIN_INSTANCE_INFO), TAG_PORTCLASS);
        if (!Descriptor->Factory.Instances)
            goto cleanup;

        Descriptor->Factory.PinDescriptorCount = FilterDescription->PinCount;
        Descriptor->Factory.PinDescriptorSize = FilterDescription->PinSize;

        // copy pin factories
        for(Index = 0; Index < FilterDescription->PinCount; Index++)
        {
            RtlMoveMemory(&Descriptor->Factory.KsPinDescriptor[Index], &FilterDescription->Pins[Index].KsPinDescriptor, FilterDescription->PinSize);

            if (FilterDescription->Pins[Index].KsPinDescriptor.DataRangesCount)
            {
                Descriptor->Factory.KsPinDescriptor[Index].DataRanges = (const PKSDATARANGE*)AllocateItem(NonPagedPool, FilterDescription->Pins[Index].KsPinDescriptor.DataRangesCount * sizeof(PKSDATARANGE), TAG_PORTCLASS);
                if(!Descriptor->Factory.KsPinDescriptor[Index].DataRanges)
                    goto cleanup;

                for (SubIndex = 0; SubIndex < FilterDescription->Pins[Index].KsPinDescriptor.DataRangesCount; SubIndex++)
                {
                    DataRange = (PKSDATARANGE)AllocateItem(NonPagedPool, FilterDescription->Pins[Index].KsPinDescriptor.DataRanges[SubIndex]->FormatSize, TAG_PORTCLASS);
                    if (!DataRange)
                        goto cleanup;

                    RtlMoveMemory(DataRange,
                                  FilterDescription->Pins[Index].KsPinDescriptor.DataRanges[SubIndex],
                                  FilterDescription->Pins[Index].KsPinDescriptor.DataRanges[SubIndex]->FormatSize);

                    ((PKSDATAFORMAT*)Descriptor->Factory.KsPinDescriptor[Index].DataRanges)[SubIndex] = DataRange;

                }

                Descriptor->Factory.KsPinDescriptor[Index].DataRangesCount = FilterDescription->Pins[Index].KsPinDescriptor.DataRangesCount;
            }

            Descriptor->Factory.Instances[Index].CurrentPinInstanceCount = 0;
            Descriptor->Factory.Instances[Index].MaxFilterInstanceCount = FilterDescription->Pins[Index].MaxFilterInstanceCount;
            Descriptor->Factory.Instances[Index].MaxGlobalInstanceCount = FilterDescription->Pins[Index].MaxGlobalInstanceCount;
            Descriptor->Factory.Instances[Index].MinFilterInstanceCount = FilterDescription->Pins[Index].MinFilterInstanceCount;
        }
    }
    Descriptor->DeviceDescriptor = FilterDescription;
    *OutSubdeviceDescriptor = Descriptor;
    return STATUS_SUCCESS;

cleanup:
    if (Descriptor)
    {
        if (Descriptor->Interfaces)
            FreeItem(Descriptor->Interfaces, TAG_PORTCLASS);

        if (Descriptor->Factory.KsPinDescriptor)
            FreeItem(Descriptor->Factory.KsPinDescriptor, TAG_PORTCLASS);

        FreeItem(Descriptor, TAG_PORTCLASS);
    }
    return Status;
}

NTSTATUS
NTAPI
PcValidateConnectRequest(
    IN PIRP Irp,
    IN KSPIN_FACTORY * Factory,
    OUT PKSPIN_CONNECT * Connect)
{
    return KsValidateConnectRequest(Irp, Factory->PinDescriptorCount, Factory->KsPinDescriptor, Connect);
}

