/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/ksfilter/ks/filter.c
 * PURPOSE:         KS IKsFilter interface functions
 * PROGRAMMER:      Johannes Anderwald
 */


#include "priv.h"

typedef struct
{
    KSBASIC_HEADER Header;
    KSFILTER Filter;

    IKsFilterVtbl *lpVtbl;
    IKsControlVtbl *lpVtblKsControl;
    IKsFilterFactory * FilterFactory;
    LONG ref;

    PKSIOBJECT_HEADER ObjectHeader;
    KSTOPOLOGY Topology;
    KSPIN_DESCRIPTOR * PinDescriptors;
    ULONG PinDescriptorCount;
    PKSFILTERFACTORY Factory;
    PFILE_OBJECT FileObject;
    KMUTEX ProcessingMutex;


    PFNKSFILTERPOWER Sleep;
    PFNKSFILTERPOWER Wake;

    ULONG *PinInstanceCount;
    PKSPIN * FirstPin;
    KSPROCESSPIN_INDEXENTRY ProcessPinIndex;

}IKsFilterImpl;

const GUID IID_IKsControl = {0x28F54685L, 0x06FD, 0x11D2, {0xB2, 0x7A, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}};
const GUID IID_IKsFilter  = {0x3ef6ee44L, 0x0D41, 0x11d2, {0xbe, 0xDA, 0x00, 0xc0, 0x4f, 0x8e, 0xF4, 0x57}};
const GUID KSPROPSETID_Topology                = {0x720D4AC0L, 0x7533, 0x11D0, {0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00}};
const GUID KSPROPSETID_Pin                     = {0x8C134960L, 0x51AD, 0x11CF, {0x87, 0x8A, 0x94, 0xF8, 0x01, 0xC1, 0x00, 0x00}};


DEFINE_KSPROPERTY_TOPOLOGYSET(IKsFilterTopologySet, KspTopologyPropertyHandler);
DEFINE_KSPROPERTY_PINPROPOSEDATAFORMAT(IKsFilterPinSet, KspPinPropertyHandler, KspPinPropertyHandler, KspPinPropertyHandler);

KSPROPERTY_SET FilterPropertySet[] =
{
    {
        &KSPROPSETID_Topology,
        sizeof(IKsFilterTopologySet) / sizeof(KSPROPERTY_ITEM),
        (const KSPROPERTY_ITEM*)&IKsFilterTopologySet,
        0,
        NULL
    },
    {
        &KSPROPSETID_Pin,
        sizeof(IKsFilterPinSet) / sizeof(KSPROPERTY_ITEM),
        (const KSPROPERTY_ITEM*)&IKsFilterPinSet,
        0,
        NULL
    }
};

NTSTATUS
NTAPI
IKsControl_fnQueryInterface(
    IKsControl * iface,
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    IKsFilterImpl * This = (IKsFilterImpl*)CONTAINING_RECORD(iface, IKsFilterImpl, lpVtblKsControl);

    if (IsEqualGUIDAligned(refiid, &IID_IUnknown))
    {
        *Output = &This->lpVtbl;
        _InterlockedIncrement(&This->ref);
        return STATUS_SUCCESS;
    }
    return STATUS_UNSUCCESSFUL;
}

ULONG
NTAPI
IKsControl_fnAddRef(
    IKsControl * iface)
{
    IKsFilterImpl * This = (IKsFilterImpl*)CONTAINING_RECORD(iface, IKsFilterImpl, lpVtblKsControl);

    return InterlockedIncrement(&This->ref);
}

ULONG
NTAPI
IKsControl_fnRelease(
    IKsControl * iface)
{
    IKsFilterImpl * This = (IKsFilterImpl*)CONTAINING_RECORD(iface, IKsFilterImpl, lpVtblKsControl);

    InterlockedDecrement(&This->ref);

    /* Return new reference count */
    return This->ref;
}

NTSTATUS
NTAPI
IKsControl_fnKsProperty(
    IKsControl * iface,
    IN PKSPROPERTY Property,
    IN ULONG PropertyLength,
    IN OUT PVOID PropertyData,
    IN ULONG DataLength,
    OUT ULONG* BytesReturned)
{
    IKsFilterImpl * This = (IKsFilterImpl*)CONTAINING_RECORD(iface, IKsFilterImpl, lpVtblKsControl);

    return KsSynchronousIoControlDevice(This->FileObject, KernelMode, IOCTL_KS_PROPERTY, Property, PropertyLength, PropertyData, DataLength, BytesReturned);
}


NTSTATUS
NTAPI
IKsControl_fnKsMethod(
    IKsControl * iface,
    IN PKSMETHOD Method,
    IN ULONG MethodLength,
    IN OUT PVOID MethodData,
    IN ULONG DataLength,
    OUT ULONG* BytesReturned)
{
    IKsFilterImpl * This = (IKsFilterImpl*)CONTAINING_RECORD(iface, IKsFilterImpl, lpVtblKsControl);

    return KsSynchronousIoControlDevice(This->FileObject, KernelMode, IOCTL_KS_METHOD, Method, MethodLength, MethodData, DataLength, BytesReturned);
}


NTSTATUS
NTAPI
IKsControl_fnKsEvent(
    IKsControl * iface,
    IN PKSEVENT Event OPTIONAL,
    IN ULONG EventLength,
    IN OUT PVOID EventData,
    IN ULONG DataLength,
    OUT ULONG* BytesReturned)
{
    IKsFilterImpl * This = (IKsFilterImpl*)CONTAINING_RECORD(iface, IKsFilterImpl, lpVtblKsControl);

    if (Event)
    {
       return KsSynchronousIoControlDevice(This->FileObject, KernelMode, IOCTL_KS_ENABLE_EVENT, Event, EventLength, EventData, DataLength, BytesReturned);
    }
    else
    {
       return KsSynchronousIoControlDevice(This->FileObject, KernelMode, IOCTL_KS_DISABLE_EVENT, EventData, DataLength, NULL, 0, BytesReturned);
    }

}

static IKsControlVtbl vt_IKsControl =
{
    IKsControl_fnQueryInterface,
    IKsControl_fnAddRef,
    IKsControl_fnRelease,
    IKsControl_fnKsProperty,
    IKsControl_fnKsMethod,
    IKsControl_fnKsEvent
};


NTSTATUS
NTAPI
IKsFilter_fnQueryInterface(
    IKsFilter * iface,
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    IKsFilterImpl * This = (IKsFilterImpl*)CONTAINING_RECORD(iface, IKsFilterImpl, lpVtbl);

    if (IsEqualGUIDAligned(refiid, &IID_IUnknown) ||
        IsEqualGUIDAligned(refiid, &IID_IKsFilter))
    {
        *Output = &This->lpVtbl;
        _InterlockedIncrement(&This->ref);
        return STATUS_SUCCESS;
    }
    else if (IsEqualGUIDAligned(refiid, &IID_IKsControl))
    {
        *Output = &This->lpVtblKsControl;
        _InterlockedIncrement(&This->ref);
        return STATUS_SUCCESS;
    }

    return STATUS_UNSUCCESSFUL;
}

ULONG
NTAPI
IKsFilter_fnAddRef(
    IKsFilter * iface)
{
    IKsFilterImpl * This = (IKsFilterImpl*)CONTAINING_RECORD(iface, IKsFilterImpl, lpVtbl);

    return InterlockedIncrement(&This->ref);
}

ULONG
NTAPI
IKsFilter_fnRelease(
    IKsFilter * iface)
{
    IKsFilterImpl * This = (IKsFilterImpl*)CONTAINING_RECORD(iface, IKsFilterImpl, lpVtbl);

    InterlockedDecrement(&This->ref);

    if (This->ref == 0)
    {
        FreeItem(This);
        return 0;
    }
    /* Return new reference count */
    return This->ref;

}

PKSFILTER
NTAPI
IKsFilter_fnGetStruct(
    IKsFilter * iface)
{
    IKsFilterImpl * This = (IKsFilterImpl*)CONTAINING_RECORD(iface, IKsFilterImpl, lpVtbl);

    return &This->Filter;
}

BOOL
NTAPI
IKsFilter_fnDoAllNecessaryPinsExist(
    IKsFilter * iface)
{
    UNIMPLEMENTED
    return FALSE;
}

NTSTATUS
NTAPI
IKsFilter_fnCreateNode(
    IKsFilter * iface,
    IN PIRP Irp,
    IN IKsPin * Pin,
    IN PLIST_ENTRY ListEntry)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
IKsFilter_fnBindProcessPinsToPipeSection(
    IKsFilter * iface,
    IN struct KSPROCESSPIPESECTION *Section,
    IN PVOID Create,
    IN PKSPIN KsPin,
    OUT IKsPin **Pin,
    OUT PKSGATE *OutGate)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
IKsFilter_fnUnbindProcessPinsFromPipeSection(
    IKsFilter * iface,
    IN struct KSPROCESSPIPESECTION *Section)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
IKsFilter_fnAddProcessPin(
    IKsFilter * iface,
    IN PKSPROCESSPIN ProcessPin)
{
    PKSPROCESSPIN *Pins;
    IKsFilterImpl * This = (IKsFilterImpl*)CONTAINING_RECORD(iface, IKsFilterImpl, lpVtbl);

    /* first acquire processing mutex */
    KeWaitForSingleObject(&This->ProcessingMutex, Executive, KernelMode, FALSE, NULL);

    /* allocate new pins array */
    Pins = AllocateItem(NonPagedPool, sizeof(PKSPROCESSPIN) * (This->ProcessPinIndex.Count + 1));

    /* check if allocation succeeded */
    if (Pins)
    {
        if (This->ProcessPinIndex.Count)
        {
            /* copy old pin index */
            RtlMoveMemory(Pins, This->ProcessPinIndex.Pins, sizeof(PKSPROCESSPIN) * This->ProcessPinIndex.Count);
        }

        /* add new process pin */
        Pins[This->ProcessPinIndex.Count] = ProcessPin;

        /* free old process pin */
        FreeItem(This->ProcessPinIndex.Pins);

        /* store new process pin index */
        This->ProcessPinIndex.Pins = Pins;
        This->ProcessPinIndex.Count++;
    }

    /* release process mutex */
    KeReleaseMutex(&This->ProcessingMutex, FALSE);

    if (Pins)
        return STATUS_SUCCESS;
    else
        return STATUS_INSUFFICIENT_RESOURCES;

}

NTSTATUS
NTAPI
IKsFilter_fnRemoveProcessPin(
    IKsFilter * iface,
    IN PKSPROCESSPIN ProcessPin)
{
    ULONG Index;
    IKsFilterImpl * This = (IKsFilterImpl*)CONTAINING_RECORD(iface, IKsFilterImpl, lpVtbl);

    /* first acquire processing mutex */
    KeWaitForSingleObject(&This->ProcessingMutex, Executive, KernelMode, FALSE, NULL);

    /* iterate through process pin index array and search for the process pin to be removed */
    for(Index = 0; Index < This->ProcessPinIndex.Count; Index++)
    {
        if (This->ProcessPinIndex.Pins[Index] == ProcessPin)
        {
            /* found process pin */
            if (Index + 1 < This->ProcessPinIndex.Count)
            {
                /* erase entry */
                RtlMoveMemory(&This->ProcessPinIndex.Pins[Index], &This->ProcessPinIndex.Pins[Index+1], This->ProcessPinIndex.Count - Index - 1);
            }
            /* decrement process pin count */
            This->ProcessPinIndex.Count--;
        }
    }

    /* release process mutex */
    KeReleaseMutex(&This->ProcessingMutex, FALSE);

    /* done */
    return STATUS_SUCCESS;
}

BOOL
NTAPI
IKsFilter_fnReprepareProcessPipeSection(
    IKsFilter * iface,
    IN struct KSPROCESSPIPESECTION *PipeSection,
    IN PULONG Data)
{
    UNIMPLEMENTED
    return FALSE;
}

VOID
NTAPI
IKsFilter_fnDeliverResetState(
    IKsFilter * iface,
    IN struct KSPROCESSPIPESECTION *PipeSection,
    IN KSRESET ResetState)
{
    UNIMPLEMENTED
}

BOOL
NTAPI
IKsFilter_fnIsFrameHolding(
    IKsFilter * iface)
{
    UNIMPLEMENTED
    return FALSE;
}

VOID
NTAPI
IKsFilter_fnRegisterForCopyCallbacks(
    IKsFilter * iface,
    IKsQueue *Queue,
    BOOL Register)
{
    UNIMPLEMENTED
}

PKSPROCESSPIN_INDEXENTRY
NTAPI
IKsFilter_fnGetProcessDispatch(
    IKsFilter * iface)
{
    UNIMPLEMENTED
    return NULL;
}

static IKsFilterVtbl vt_IKsFilter =
{
    IKsFilter_fnQueryInterface,
    IKsFilter_fnAddRef,
    IKsFilter_fnRelease,
    IKsFilter_fnGetStruct,
    IKsFilter_fnDoAllNecessaryPinsExist,
    IKsFilter_fnCreateNode,
    IKsFilter_fnBindProcessPinsToPipeSection,
    IKsFilter_fnUnbindProcessPinsFromPipeSection,
    IKsFilter_fnAddProcessPin,
    IKsFilter_fnRemoveProcessPin,
    IKsFilter_fnReprepareProcessPipeSection,
    IKsFilter_fnDeliverResetState,
    IKsFilter_fnIsFrameHolding,
    IKsFilter_fnRegisterForCopyCallbacks,
    IKsFilter_fnGetProcessDispatch
};

NTSTATUS
IKsFilter_GetFilterFromIrp(
    IN PIRP Irp,
    OUT IKsFilter **Filter)
{
    PIO_STACK_LOCATION IoStack;
    PKSIOBJECT_HEADER ObjectHeader;
    NTSTATUS Status;

    /* get current irp stack */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* santiy check */
    ASSERT(IoStack->FileObject != NULL);

    ObjectHeader = (PKSIOBJECT_HEADER)IoStack->FileObject->FsContext;

    /* sanity is important */
    ASSERT(ObjectHeader != NULL);
    ASSERT(ObjectHeader->Type == KsObjectTypeFilter);
    ASSERT(ObjectHeader->Unknown != NULL);

    /* get our private interface */
    Status = ObjectHeader->Unknown->lpVtbl->QueryInterface(ObjectHeader->Unknown, &IID_IKsFilter, (PVOID*)Filter);

    if (!NT_SUCCESS(Status))
    {
        /* something is wrong here */
        DPRINT1("KS: Misbehaving filter %p\n", ObjectHeader->Unknown);
        Irp->IoStatus.Status = Status;

        /* complete and forget irp */
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return Status;
    }
    return Status;
}


NTSTATUS
NTAPI
IKsFilter_DispatchClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    IKsFilter * Filter;
    IKsFilterImpl * This;
    NTSTATUS Status;

    /* obtain filter from object header */
    Status = IKsFilter_GetFilterFromIrp(Irp, &Filter);
    if (!NT_SUCCESS(Status))
        return Status;

    /* get our real implementation */
    This = (IKsFilterImpl*)CONTAINING_RECORD(Filter, IKsFilterImpl, lpVtbl);

    /* does the driver support notifications */
    if (This->Factory->FilterDescriptor && This->Factory->FilterDescriptor->Dispatch && This->Factory->FilterDescriptor->Dispatch->Close)
    {
        /* call driver's filter close function */
        Status = This->Factory->FilterDescriptor->Dispatch->Close(&This->Filter, Irp);
    }

    if (NT_SUCCESS(Status) && Status != STATUS_PENDING)
    {
        /* save the result */
        Irp->IoStatus.Status = Status;
        /* complete irp */
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        /* FIXME remove our instance from the filter factory */
        ASSERT(0);

        /* free object header */
        KsFreeObjectHeader(This->ObjectHeader);
    }
    else
    {
        /* complete and forget */
        Irp->IoStatus.Status = Status;
        /* complete irp */
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    /* done */
    return Status;
}

NTSTATUS
KspHandlePropertyInstances(
    IN PIO_STATUS_BLOCK IoStatus,
    IN PKSIDENTIFIER  Request,
    IN OUT PVOID  Data,
    IN IKsFilterImpl * This,
    IN BOOL Global)
{
    KSPIN_CINSTANCES * Instances;
    KSP_PIN * Pin = (KSP_PIN*)Request;

    if (!This->Factory->FilterDescriptor || !This->Factory->FilterDescriptor->PinDescriptorsCount)
    {
        /* no filter / pin descriptor */
        IoStatus->Status = STATUS_NOT_IMPLEMENTED;
        return STATUS_NOT_IMPLEMENTED;
    }

    /* ignore custom structs for now */
    ASSERT(This->Factory->FilterDescriptor->PinDescriptorSize == sizeof(KSPIN_DESCRIPTOR_EX)); 
    ASSERT(This->Factory->FilterDescriptor->PinDescriptorsCount > Pin->PinId);

    Instances = (KSPIN_CINSTANCES*)Data;
    /* max instance count */
    Instances->PossibleCount = This->Factory->FilterDescriptor->PinDescriptors[Pin->PinId].InstancesPossible;
    /* current instance count */
    Instances->CurrentCount = This->PinInstanceCount[Pin->PinId];

    IoStatus->Information = sizeof(KSPIN_CINSTANCES);
    IoStatus->Status = STATUS_SUCCESS;
    return STATUS_SUCCESS;
}

NTSTATUS
KspHandleNecessaryPropertyInstances(
    IN PIO_STATUS_BLOCK IoStatus,
    IN PKSIDENTIFIER  Request,
    IN OUT PVOID  Data,
    IN IKsFilterImpl * This)
{
    PULONG Result;
    KSP_PIN * Pin = (KSP_PIN*)Request;

    if (!This->Factory->FilterDescriptor || !This->Factory->FilterDescriptor->PinDescriptorsCount)
    {
        /* no filter / pin descriptor */
        IoStatus->Status = STATUS_NOT_IMPLEMENTED;
        return STATUS_NOT_IMPLEMENTED;
    }

    /* ignore custom structs for now */
    ASSERT(This->Factory->FilterDescriptor->PinDescriptorSize == sizeof(KSPIN_DESCRIPTOR_EX)); 
    ASSERT(This->Factory->FilterDescriptor->PinDescriptorsCount > Pin->PinId);

    Result = (PULONG)Data;
    *Result = This->Factory->FilterDescriptor->PinDescriptors[Pin->PinId].InstancesNecessary;

    IoStatus->Information = sizeof(ULONG);
    IoStatus->Status = STATUS_SUCCESS;
    return STATUS_SUCCESS;
}

NTSTATUS
KspHandleDataIntersection(
    IN PIRP Irp,
    IN PIO_STATUS_BLOCK IoStatus,
    IN PKSIDENTIFIER Request,
    IN OUT PVOID  Data,
    IN ULONG DataLength,
    IN IKsFilterImpl * This)
{
    PKSMULTIPLE_ITEM MultipleItem;
    PKSDATARANGE DataRange;
    NTSTATUS Status = STATUS_NO_MATCH;
    ULONG Index, Length;
    KSP_PIN * Pin = (KSP_PIN*)Request;

    /* Access parameters */
    MultipleItem = (PKSMULTIPLE_ITEM)(Pin + 1);
    DataRange = (PKSDATARANGE)(MultipleItem + 1);

    if (!This->Factory->FilterDescriptor || !This->Factory->FilterDescriptor->PinDescriptorsCount)
    {
        /* no filter / pin descriptor */
        IoStatus->Status = STATUS_NOT_IMPLEMENTED;
        return STATUS_NOT_IMPLEMENTED;
    }

    /* ignore custom structs for now */
    ASSERT(This->Factory->FilterDescriptor->PinDescriptorSize == sizeof(KSPIN_DESCRIPTOR_EX)); 
    ASSERT(This->Factory->FilterDescriptor->PinDescriptorsCount > Pin->PinId);

    if (This->Factory->FilterDescriptor->PinDescriptors[Pin->PinId].IntersectHandler == NULL ||
        This->Factory->FilterDescriptor->PinDescriptors[Pin->PinId].PinDescriptor.DataRanges == NULL ||
        This->Factory->FilterDescriptor->PinDescriptors[Pin->PinId].PinDescriptor.DataRangesCount == 0)
    {
        /* no driver supported intersect handler / no provided data ranges */
        IoStatus->Status = STATUS_NOT_IMPLEMENTED;
        return STATUS_NOT_IMPLEMENTED;
    }

    for(Index = 0; Index < MultipleItem->Count; Index++)
    {
        /* Call miniport's properitary handler */
        Status = This->Factory->FilterDescriptor->PinDescriptors[Pin->PinId].IntersectHandler(NULL, /* context */
                                                                                              Irp,
                                                                                              Pin,
                                                                                              DataRange,
                                                                                              (PKSDATAFORMAT)This->Factory->FilterDescriptor->PinDescriptors[Pin->PinId].PinDescriptor.DataRanges,
                                                                                              DataLength,
                                                                                              Data,
                                                                                              &Length);

        if (Status == STATUS_SUCCESS)
        {
            IoStatus->Information = Length;
            break;
        }
        DataRange =  UlongToPtr(PtrToUlong(DataRange) + DataRange->FormatSize);
    }

    IoStatus->Status = Status;
    return Status;
}

NTSTATUS
NTAPI
KspPinPropertyHandler(
    IN PIRP Irp,
    IN PKSIDENTIFIER  Request,
    IN OUT PVOID  Data)
{
    PIO_STACK_LOCATION IoStack;
    IKsFilterImpl * This;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    /* get filter implementation */
    This = (IKsFilterImpl*)KSPROPERTY_ITEM_IRP_STORAGE(Irp);

    /* get current stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    switch(Request->Id)
    {
        case KSPROPERTY_PIN_CTYPES:
        case KSPROPERTY_PIN_DATAFLOW:
        case KSPROPERTY_PIN_DATARANGES:
        case KSPROPERTY_PIN_INTERFACES:
        case KSPROPERTY_PIN_MEDIUMS:
        case KSPROPERTY_PIN_COMMUNICATION:
        case KSPROPERTY_PIN_CATEGORY:
        case KSPROPERTY_PIN_NAME:
        case KSPROPERTY_PIN_PROPOSEDATAFORMAT:
            Status = KsPinPropertyHandler(Irp, Request, Data, This->PinDescriptorCount, This->PinDescriptors);
            break;
        case KSPROPERTY_PIN_GLOBALCINSTANCES:
            Status = KspHandlePropertyInstances(&Irp->IoStatus, Request, Data, This, TRUE);
            break;
        case KSPROPERTY_PIN_CINSTANCES:
            Status = KspHandlePropertyInstances(&Irp->IoStatus, Request, Data, This, FALSE);
            break;
        case KSPROPERTY_PIN_NECESSARYINSTANCES:
            Status = KspHandleNecessaryPropertyInstances(&Irp->IoStatus, Request, Data, This);
            break;

        case KSPROPERTY_PIN_DATAINTERSECTION:
            Status = KspHandleDataIntersection(Irp, &Irp->IoStatus, Request, Data, IoStack->Parameters.DeviceIoControl.OutputBufferLength, This);
            break;
        case KSPROPERTY_PIN_PHYSICALCONNECTION:
        case KSPROPERTY_PIN_CONSTRAINEDDATARANGES:
            UNIMPLEMENTED
            Status = STATUS_NOT_IMPLEMENTED;
            break;
        default:
            UNIMPLEMENTED
            Status = STATUS_UNSUCCESSFUL;
    }

    return Status;
}

NTSTATUS
NTAPI
IKsFilter_DispatchDeviceIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    IKsFilter * Filter;
    IKsFilterImpl * This;
    NTSTATUS Status;
    PKSFILTER FilterInstance;

    /* obtain filter from object header */
    Status = IKsFilter_GetFilterFromIrp(Irp, &Filter);
    if (!NT_SUCCESS(Status))
        return Status;

    /* get our real implementation */
    This = (IKsFilterImpl*)CONTAINING_RECORD(Filter, IKsFilterImpl, lpVtbl);

    /* current irp stack */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    if (IoStack->Parameters.DeviceIoControl.IoControlCode != IOCTL_KS_PROPERTY)
    {
        UNIMPLEMENTED;

        /* release filter interface */
        Filter->lpVtbl->Release(Filter);

        /* complete and forget irp */
        Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_NOT_IMPLEMENTED;
    }

    /* call property handler supported by ks */
    Status = KspPropertyHandler(Irp, 2, FilterPropertySet, NULL, sizeof(KSPROPERTY_ITEM));

    if (Status == STATUS_NOT_FOUND)
    {
        /* get filter instance */
        FilterInstance = Filter->lpVtbl->GetStruct(Filter);

        /* check if the driver supports property sets */
        if (FilterInstance->Descriptor->AutomationTable && FilterInstance->Descriptor->AutomationTable->PropertySetsCount)
        {
            /* call driver's filter property handler */
            Status = KspPropertyHandler(Irp, 
                                        FilterInstance->Descriptor->AutomationTable->PropertySetsCount,
                                        FilterInstance->Descriptor->AutomationTable->PropertySets, 
                                        NULL,
                                        FilterInstance->Descriptor->AutomationTable->PropertyItemSize);
        }
    }

    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    /* done */
    return Status;
}

static KSDISPATCH_TABLE DispatchTable =
{
    IKsFilter_DispatchDeviceIoControl,
    KsDispatchInvalidDeviceRequest,
    KsDispatchInvalidDeviceRequest,
    KsDispatchInvalidDeviceRequest,
    IKsFilter_DispatchClose,
    KsDispatchQuerySecurity,
    KsDispatchSetSecurity,
    KsDispatchFastIoDeviceControlFailure,
    KsDispatchFastReadFailure,
    KsDispatchFastReadFailure,
};


NTSTATUS
IKsFilter_CreateDescriptors(
    IKsFilterImpl * This,
    KSFILTER_DESCRIPTOR* FilterDescriptor)
{
    ULONG Index = 0;

    /* initialize pin descriptors */
    if (FilterDescriptor->PinDescriptorsCount)
    {
        /* allocate pin instance count array */
        This->PinInstanceCount = AllocateItem(NonPagedPool, sizeof(ULONG) * FilterDescriptor->PinDescriptorsCount);
        if(!This->PinDescriptors)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        /* allocate first pin array */
        This->FirstPin = AllocateItem(NonPagedPool, sizeof(PKSPIN) * FilterDescriptor->PinDescriptorsCount);
        if(!This->FirstPin)
        {
            FreeItem(This->PinDescriptors);
            return STATUS_INSUFFICIENT_RESOURCES;
        }


        /* allocate pin descriptor array */
        This->PinDescriptors = AllocateItem(NonPagedPool, sizeof(KSPIN_DESCRIPTOR) * FilterDescriptor->PinDescriptorsCount);
        if(!This->PinDescriptors)
        {
            FreeItem(This->PinInstanceCount);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        /* set pin count */
        This->PinDescriptorCount = FilterDescriptor->PinDescriptorsCount;
        /* now copy those pin descriptors over */
        for(Index = 0; Index < FilterDescriptor->PinDescriptorsCount; Index++)
        {
            /* copy one pin per time */
            RtlMoveMemory(&This->PinDescriptors[Index], &FilterDescriptor->PinDescriptors[Index].PinDescriptor, sizeof(KSPIN_DESCRIPTOR));
        }
    }

    /* initialize topology descriptor */
    This->Topology.CategoriesCount = FilterDescriptor->CategoriesCount;
    This->Topology.Categories = FilterDescriptor->Categories;
    This->Topology.TopologyNodesCount = FilterDescriptor->NodeDescriptorsCount;
    This->Topology.TopologyConnectionsCount = FilterDescriptor->ConnectionsCount;
    This->Topology.TopologyConnections = FilterDescriptor->Connections;

    if (This->Topology.TopologyNodesCount > 0)
    {
        This->Topology.TopologyNodes = AllocateItem(NonPagedPool, sizeof(GUID) * This->Topology.TopologyNodesCount);
        /* allocate topology node types array */
        if (!This->Topology.TopologyNodes)
            return STATUS_INSUFFICIENT_RESOURCES;

        This->Topology.TopologyNodesNames = AllocateItem(NonPagedPool, sizeof(GUID) * This->Topology.TopologyNodesCount);
        /* allocate topology names array */
        if (!This->Topology.TopologyNodesNames)
        {
            FreeItem((PVOID)This->Topology.TopologyNodes);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        for(Index = 0; Index < This->Topology.TopologyNodesCount; Index++)
        {
            /* copy topology type */
            RtlMoveMemory((PVOID)&This->Topology.TopologyNodes[Index], FilterDescriptor->NodeDescriptors[Index].Type, sizeof(GUID));
            /* copy topology name */
            RtlMoveMemory((PVOID)&This->Topology.TopologyNodesNames[Index], FilterDescriptor->NodeDescriptors[Index].Name, sizeof(GUID));
        }
    }

    /* done! */
    return STATUS_SUCCESS;
}

NTSTATUS
IKsFilter_CopyFilterDescriptor(
    IKsFilterImpl * This,
    const KSFILTER_DESCRIPTOR* FilterDescriptor)
{
    This->Filter.Descriptor = (const KSFILTER_DESCRIPTOR*)AllocateItem(NonPagedPool, sizeof(KSFILTER_DESCRIPTOR));
    if (!This->Filter.Descriptor)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* copy all fields */
    RtlMoveMemory((PVOID)This->Filter.Descriptor, FilterDescriptor, sizeof(KSFILTER_DESCRIPTOR));


    /* perform deep copy of pin descriptors */
    if (FilterDescriptor->PinDescriptorsCount)
    {
        KSPIN_DESCRIPTOR_EX * PinDescriptors = (KSPIN_DESCRIPTOR_EX *)AllocateItem(NonPagedPool, FilterDescriptor->PinDescriptorSize * FilterDescriptor->PinDescriptorsCount);


        if (!PinDescriptors)
        {
            FreeItem((PVOID)This->Filter.Descriptor);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        RtlMoveMemory((PVOID)PinDescriptors, FilterDescriptor->PinDescriptors, FilterDescriptor->PinDescriptorSize * FilterDescriptor->PinDescriptorsCount);

        /* brain-dead gcc hack */
        RtlMoveMemory((PVOID)&This->Filter.Descriptor->PinDescriptors, PinDescriptors, sizeof(PKSPIN_DESCRIPTOR_EX));

    }

    /* perform deep copy of node descriptors */
    if (FilterDescriptor->NodeDescriptorsCount)
    {
        KSNODE_DESCRIPTOR* NodeDescriptor = AllocateItem(NonPagedPool, FilterDescriptor->NodeDescriptorsCount * FilterDescriptor->NodeDescriptorSize);
        if (!NodeDescriptor)
        {
            if (This->Filter.Descriptor->PinDescriptors)
                FreeItem((PVOID)This->Filter.Descriptor->PinDescriptors);
            FreeItem((PVOID)This->Filter.Descriptor);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        RtlMoveMemory((PVOID)NodeDescriptor, FilterDescriptor->NodeDescriptors, FilterDescriptor->NodeDescriptorsCount * FilterDescriptor->NodeDescriptorSize);

        /* brain-dead gcc hack */
        RtlMoveMemory((PVOID)&This->Filter.Descriptor->NodeDescriptors, NodeDescriptor, sizeof(PKSNODE_DESCRIPTOR));
    }

    /* perform deep copy of connections descriptors */
    if (FilterDescriptor->NodeDescriptorsCount)
    {
        KSTOPOLOGY_CONNECTION* Connections = AllocateItem(NonPagedPool, sizeof(KSTOPOLOGY_CONNECTION) * FilterDescriptor->ConnectionsCount);
        if (!Connections)
        {
            if (This->Filter.Descriptor->PinDescriptors)
                FreeItem((PVOID)This->Filter.Descriptor->PinDescriptors);

            if (This->Filter.Descriptor->NodeDescriptors)
                FreeItem((PVOID)This->Filter.Descriptor->PinDescriptors);

            FreeItem((PVOID)This->Filter.Descriptor);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlMoveMemory((PVOID)Connections, FilterDescriptor->Connections, sizeof(KSTOPOLOGY_CONNECTION) * FilterDescriptor->ConnectionsCount);

        /* brain-dead gcc hack */
        RtlMoveMemory((PVOID)&This->Filter.Descriptor->Connections, Connections, sizeof(PKSTOPOLOGY_CONNECTION));
    }

    return STATUS_SUCCESS;
}


NTSTATUS
IKsFilter_AddPin(
    IKsFilter * Filter,
    PKSPIN Pin)
{
    PKSPIN NextPin, CurPin;
    PKSBASIC_HEADER BasicHeader;
    IKsFilterImpl * This = (IKsFilterImpl*)Filter;

    /* sanity check */
    ASSERT(Pin->Id < This->PinDescriptorCount);

    if (This->FirstPin[Pin->Id] == NULL)
    {
        /* welcome first pin */
        This->FirstPin[Pin->Id] = Pin;
        return STATUS_SUCCESS;
    }

    /* get first pin */
    CurPin = This->FirstPin[Pin->Id];

    do
    {
        /* get next instantiated pin */
        NextPin = KsPinGetNextSiblingPin(CurPin);
        if (!NextPin)
            break;

        NextPin = CurPin;

    }while(NextPin != NULL);

    /* get basic header */
    BasicHeader = (PKSBASIC_HEADER)((ULONG_PTR)CurPin - sizeof(KSBASIC_HEADER));

    /* store pin */
    BasicHeader->Next.Pin = Pin;

    return STATUS_SUCCESS;
}


NTSTATUS
NTAPI
IKsFilter_DispatchCreatePin(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    IKsFilterImpl * This;
    PKSOBJECT_CREATE_ITEM CreateItem;
    PKSPIN_CONNECT Connect;
    NTSTATUS Status;

    /* get the create item */
    CreateItem = KSCREATE_ITEM_IRP_STORAGE(Irp);

    /* get the filter object */
    This = (IKsFilterImpl*)CreateItem->Context;

    /* acquire control mutex */
    KeWaitForSingleObject(&This->Header.ControlMutex, Executive, KernelMode, FALSE, NULL);

    /* now validate the connect request */
    Status = KsValidateConnectRequest(Irp, This->PinDescriptorCount, This->PinDescriptors, &Connect);

    if (NT_SUCCESS(Status))
    {
        if (This->PinInstanceCount[Connect->PinId] < This->Filter.Descriptor->PinDescriptors[Connect->PinId].InstancesPossible)
        {
            /* create the pin */
            Status = KspCreatePin(DeviceObject, Irp, This->Header.KsDevice, This->FilterFactory, (IKsFilter*)&This->lpVtbl, Connect, (KSPIN_DESCRIPTOR_EX*)&This->Filter.Descriptor->PinDescriptors[Connect->PinId]);

            if (NT_SUCCESS(Status))
            {
                /* successfully created pin, increment pin instance count */
                This->PinInstanceCount[Connect->PinId]++;
            }
        }
        else
        {
            /* maximum instance count reached, bye-bye */
            Status = STATUS_UNSUCCESSFUL;
        }
    }

    /* release control mutex */
    KeReleaseMutex(&This->Header.ControlMutex, FALSE);

    if (Status != STATUS_PENDING)
    {
        /* complete request */
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }
    /* done */
    return Status;
}

NTSTATUS
NTAPI
IKsFilter_DispatchCreateNode(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    UNIMPLEMENTED
    Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_UNSUCCESSFUL;
}


VOID
IKsFilter_AttachFilterToFilterFactory(
    IKsFilterImpl * This,
    PKSFILTERFACTORY FilterFactory)
{
    PKSBASIC_HEADER BasicHeader;
    PKSFILTER Filter;


    /* get filter factory basic header */
    BasicHeader = (PKSBASIC_HEADER)((ULONG_PTR)FilterFactory - sizeof(KSBASIC_HEADER));

    /* sanity check */
    ASSERT(BasicHeader->Type == KsObjectTypeFilterFactory);

    if (BasicHeader->FirstChild.FilterFactory == NULL)
    {
        /* welcome first instantiated filter */
        BasicHeader->FirstChild.Filter = &This->Filter;
        return;
    }

    /* set to first entry */
    Filter = BasicHeader->FirstChild.Filter;

    do
    {
        /* get basic header */
        BasicHeader = (PKSBASIC_HEADER)((ULONG_PTR)Filter - sizeof(KSBASIC_HEADER));
        /* sanity check */
        ASSERT(BasicHeader->Type == KsObjectTypeFilter);

        if (BasicHeader->Next.Filter)
        {
            /* iterate to next filter factory */
            Filter = BasicHeader->Next.Filter;
        }
        else
        {
            /* found last entry */
            break;
        }
    }while(FilterFactory);

    /* attach filter factory */
    BasicHeader->Next.Filter = &This->Filter;
}

NTSTATUS
NTAPI
KspCreateFilter(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN IKsFilterFactory *iface)
{
    IKsFilterImpl * This;
    IKsDevice *KsDevice;
    PKSFILTERFACTORY Factory;
    PIO_STACK_LOCATION IoStack;
    PDEVICE_EXTENSION DeviceExtension;
    NTSTATUS Status;
    PKSOBJECT_CREATE_ITEM CreateItem;

    /* get device extension */
    DeviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;

    /* get the filter factory */
    Factory = iface->lpVtbl->GetStruct(iface);

    if (!Factory || !Factory->FilterDescriptor || !Factory->FilterDescriptor->Dispatch || !Factory->FilterDescriptor->Dispatch->Create)
    {
        /* Sorry it just will not work */
        return STATUS_UNSUCCESSFUL;
    }

    /* allocate filter instance */
    This = AllocateItem(NonPagedPool, sizeof(IKsFilterFactory));
    if (!This)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* copy filter descriptor */
    Status = IKsFilter_CopyFilterDescriptor(This, Factory->FilterDescriptor);
    if (!NT_SUCCESS(Status))
    {
        /* not enough memory */
        FreeItem(This);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* get current irp stack */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* initialize object bag */
    This->Filter.Bag = AllocateItem(NonPagedPool, sizeof(KSIOBJECT_BAG));
    if (!This->Filter.Bag)
    {
        /* no memory */
        FreeItem(This);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* allocate create items */
    CreateItem = AllocateItem(NonPagedPool, sizeof(KSOBJECT_CREATE_ITEM) * 2);
    if (!CreateItem)
    {
        /* no memory */
        FreeItem(This);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* initialize pin create item */
    CreateItem[0].Create = IKsFilter_DispatchCreatePin;
    CreateItem[0].Context = (PVOID)This;
    CreateItem[0].Flags = KSCREATE_ITEM_FREEONSTOP;
    RtlInitUnicodeString(&CreateItem[0].ObjectClass, KSSTRING_Pin);
    /* initialize node create item */
    CreateItem[1].Create = IKsFilter_DispatchCreateNode;
    CreateItem[1].Context = (PVOID)This;
    CreateItem[1].Flags = KSCREATE_ITEM_FREEONSTOP;
    RtlInitUnicodeString(&CreateItem[1].ObjectClass, KSSTRING_TopologyNode);


    KsDevice = (IKsDevice*)&DeviceExtension->DeviceHeader->lpVtblIKsDevice;
    KsDevice->lpVtbl->InitializeObjectBag(KsDevice, (PKSIOBJECT_BAG)This->Filter.Bag, NULL);

    /* initialize filter instance */
    This->ref = 1;
    This->lpVtbl = &vt_IKsFilter;
    This->lpVtblKsControl = &vt_IKsControl;

    This->Filter.Descriptor = Factory->FilterDescriptor;
    This->Factory = Factory;
    This->FilterFactory = iface;
    This->FileObject = IoStack->FileObject;
    KeInitializeMutex(&This->ProcessingMutex, 0);
    /* initialize basic header */
    This->Header.KsDevice = &DeviceExtension->DeviceHeader->KsDevice;
    This->Header.Parent.KsFilterFactory = iface->lpVtbl->GetStruct(iface);
    This->Header.Type = KsObjectTypeFilter;
    KeInitializeMutex(&This->Header.ControlMutex, 0);
    InitializeListHead(&This->Header.EventList);
    KeInitializeSpinLock(&This->Header.EventListLock);

    /* allocate the stream descriptors */
    Status = IKsFilter_CreateDescriptors(This, (PKSFILTER_DESCRIPTOR)Factory->FilterDescriptor);
    if (!NT_SUCCESS(Status))
    {
        /* what can go wrong, goes wrong */
        FreeItem(This);
        FreeItem(CreateItem);
        return Status;
    }

    /* does the filter have a filter dispatch */
    if (Factory->FilterDescriptor->Dispatch)
    {
        /* does it have a create routine */
        if (Factory->FilterDescriptor->Dispatch->Create)
        {
            /* now let driver initialize the filter instance */
            Status = Factory->FilterDescriptor->Dispatch->Create(&This->Filter, Irp);

            if (!NT_SUCCESS(Status) && Status != STATUS_PENDING)
            {
                /* driver failed to initialize */
                DPRINT1("Driver: Status %x\n", Status);

                /* free filter instance */
                FreeItem(This);
                FreeItem(CreateItem);
                return Status;
            }
        }
    }

    /* now allocate the object header */
    Status = KsAllocateObjectHeader((PVOID*)&This->ObjectHeader, 2, CreateItem, Irp, &DispatchTable);
    if (!NT_SUCCESS(Status))
    {
        /* failed to allocate object header */
        DPRINT1("Failed to allocate object header %x\n", Status);

        return Status;
    }

    /* initialize object header extra fields */
    This->ObjectHeader->Type = KsObjectTypeFilter;
    This->ObjectHeader->Unknown = (PUNKNOWN)&This->lpVtbl;
    This->ObjectHeader->ObjectType = (PVOID)&This->Filter;

    /* attach filter to filter factory */
    IKsFilter_AttachFilterToFilterFactory(This, This->Header.Parent.KsFilterFactory);

    /* completed initialization */
    return Status;
}

/*
    @implemented
*/
KSDDKAPI
VOID
NTAPI
KsFilterAcquireProcessingMutex(
    IN PKSFILTER Filter)
{
    IKsFilterImpl * This = (IKsFilterImpl*)CONTAINING_RECORD(Filter, IKsFilterImpl, Filter);

    KeWaitForSingleObject(&This->ProcessingMutex, Executive, KernelMode, FALSE, NULL);
}

/*
    @implemented
*/
KSDDKAPI
VOID
NTAPI
KsFilterReleaseProcessingMutex(
    IN PKSFILTER Filter)
{
    IKsFilterImpl * This = (IKsFilterImpl*)CONTAINING_RECORD(Filter, IKsFilterImpl, Filter);

    KeReleaseMutex(&This->ProcessingMutex, FALSE);
}

/*
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsFilterAddTopologyConnections (
    IN PKSFILTER Filter,
    IN ULONG NewConnectionsCount,
    IN const KSTOPOLOGY_CONNECTION *const NewTopologyConnections)
{
    ULONG Count;
    KSTOPOLOGY_CONNECTION * Connections;
    IKsFilterImpl * This = (IKsFilterImpl*)CONTAINING_RECORD(Filter, IKsFilterImpl, Filter);

    Count = This->Filter.Descriptor->ConnectionsCount + NewConnectionsCount;

    /* allocate array */
    Connections = AllocateItem(NonPagedPool, Count * sizeof(KSTOPOLOGY_CONNECTION));
    if (!Connections)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* FIXME verify connections */

    if (This->Filter.Descriptor->ConnectionsCount)
    {
        /* copy old connections */
        RtlMoveMemory(Connections, This->Filter.Descriptor->Connections, sizeof(KSTOPOLOGY_CONNECTION) * This->Filter.Descriptor->ConnectionsCount);
    }

    /* add new connections */
    RtlMoveMemory((PVOID)(Connections + This->Filter.Descriptor->ConnectionsCount), NewTopologyConnections, NewConnectionsCount);

    /* add the new connections */
    RtlMoveMemory((PVOID)&This->Filter.Descriptor->ConnectionsCount, &Count, sizeof(ULONG)); /* brain-dead gcc hack */

    /* free old connections array */
    if (This->Filter.Descriptor->ConnectionsCount)
    {
        FreeItem((PVOID)This->Filter.Descriptor->Connections);
    }

    /* brain-dead gcc hack */
    RtlMoveMemory((PVOID)&This->Filter.Descriptor->Connections, Connections, sizeof(KSTOPOLOGY_CONNECTION*));

    return STATUS_SUCCESS;
}

/*
    @unimplemented
*/
KSDDKAPI
VOID
NTAPI
KsFilterAttemptProcessing(
    IN PKSFILTER Filter,
    IN BOOLEAN Asynchronous)
{
    UNIMPLEMENTED
}

/*
    @unimplemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsFilterCreateNode (
    IN PKSFILTER Filter,
    IN const KSNODE_DESCRIPTOR *const NodeDescriptor,
    OUT PULONG NodeID)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

/*
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsFilterCreatePinFactory (
    IN PKSFILTER Filter,
    IN const KSPIN_DESCRIPTOR_EX *const InPinDescriptor,
    OUT PULONG PinID)
{
    ULONG Count;
    ULONG *PinInstanceCount;
    KSPIN_DESCRIPTOR_EX * PinDescriptorsEx;
    KSPIN_DESCRIPTOR * PinDescriptors;
    PKSPIN *FirstPin;
    IKsFilterImpl * This = (IKsFilterImpl*)CONTAINING_RECORD(Filter, IKsFilterImpl, Filter);

    /* calculate existing count */
    Count = This->PinDescriptorCount + 1;

    /* allocate pin descriptors array */
    PinDescriptorsEx = AllocateItem(NonPagedPool, max(This->Filter.Descriptor->PinDescriptorSize, sizeof(KSPIN_DESCRIPTOR_EX)) * Count);
    if (!PinDescriptorsEx)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* allocate pin instance count array */
    PinInstanceCount = AllocateItem(NonPagedPool, sizeof(ULONG) * Count);
    if (!PinInstanceCount)
    {
        /* not enough memory */
        FreeItem(PinDescriptorsEx);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* allocate pin descriptor array for pin property handling */
    PinDescriptors = AllocateItem(NonPagedPool, sizeof(KSPIN_DESCRIPTOR) * Count);
    if (!PinDescriptors)
    {
        /* not enough memory */
        FreeItem(PinDescriptorsEx);
        FreeItem(PinInstanceCount);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* allocate first pin array */
    FirstPin = AllocateItem(NonPagedPool, sizeof(PKSPIN) * Count);
    if (!FirstPin)
    {
        /* not enough memory */
        FreeItem(PinDescriptorsEx);
        FreeItem(PinInstanceCount);
        FreeItem(PinDescriptors);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* now copy all fields */
    if (Count > 1)
    {
        /* copy old descriptors */
        RtlMoveMemory(PinDescriptorsEx, This->Filter.Descriptor->PinDescriptors, max(This->Filter.Descriptor->PinDescriptorSize, sizeof(KSPIN_DESCRIPTOR_EX)) * This->PinDescriptorCount);
        RtlMoveMemory(PinInstanceCount, This->PinInstanceCount, This->PinDescriptorCount * sizeof(ULONG));
        RtlMoveMemory(PinDescriptors, This->PinDescriptors, sizeof(KSPIN_DESCRIPTOR) * This->PinDescriptorCount);
        RtlMoveMemory(FirstPin, This->FirstPin, sizeof(PKSPIN) * This->PinDescriptorCount);

        /* now free old descriptors */
        FreeItem(This->PinInstanceCount);
        FreeItem((PVOID)This->Filter.Descriptor->PinDescriptors);
        FreeItem(This->PinDescriptors);
        FreeItem(This->FirstPin);
    }

    /* add new pin factory */
    RtlMoveMemory((PVOID)((ULONG_PTR)PinDescriptorsEx + max(This->Filter.Descriptor->PinDescriptorSize, sizeof(KSPIN_DESCRIPTOR_EX)) * This->PinDescriptorCount), InPinDescriptor, sizeof(KSPIN_DESCRIPTOR));
    RtlMoveMemory((PVOID)(PinDescriptors + This->PinDescriptorCount), &InPinDescriptor->PinDescriptor, sizeof(KSPIN_DESCRIPTOR));

    /* replace old descriptor by using a gcc-compliant hack */
    RtlMoveMemory((PVOID)&This->Filter.Descriptor->PinDescriptors, PinDescriptorsEx, sizeof(KSPIN_DESCRIPTOR_EX*));
    RtlMoveMemory((PVOID)&This->Filter.Descriptor->PinDescriptorsCount, &Count, sizeof(ULONG));

    This->PinDescriptors = PinDescriptors;
    This->PinInstanceCount = PinInstanceCount;
    This->FirstPin = FirstPin;

    /* store new pin id */
    *PinID = This->PinDescriptorCount;

    /* increment pin descriptor count */
    This->PinDescriptorCount++;

    return STATUS_SUCCESS;

}

/*
    @unimplemented
*/
KSDDKAPI
PKSGATE
NTAPI
KsFilterGetAndGate(
    IN PKSFILTER Filter)
{
    UNIMPLEMENTED
    return NULL;
}

/*
    @implemented
*/
KSDDKAPI
ULONG
NTAPI
KsFilterGetChildPinCount(
    IN PKSFILTER Filter,
    IN ULONG PinId)
{
    IKsFilterImpl * This = (IKsFilterImpl*)CONTAINING_RECORD(Filter, IKsFilterImpl, Filter);

    if (PinId >= This->PinDescriptorCount)
    {
        /* index is out of bounds */
        return 0;
    }
    /* return pin instance count */
    return This->PinInstanceCount[PinId];
}

/*
    @implemented
*/
KSDDKAPI
PKSPIN
NTAPI
KsFilterGetFirstChildPin(
    IN PKSFILTER Filter,
    IN ULONG PinId)
{
    IKsFilterImpl * This = (IKsFilterImpl*)CONTAINING_RECORD(Filter, IKsFilterImpl, Filter);

    if (PinId >= This->PinDescriptorCount)
    {
        /* index is out of bounds */
        return NULL;
    }

    /* return first pin index */
    return This->FirstPin[PinId];
}

/*
    @implemented
*/
KSDDKAPI
VOID
NTAPI
KsFilterRegisterPowerCallbacks(
    IN PKSFILTER Filter,
    IN PFNKSFILTERPOWER Sleep OPTIONAL,
    IN PFNKSFILTERPOWER Wake OPTIONAL)
{
    IKsFilterImpl * This = (IKsFilterImpl*)CONTAINING_RECORD(Filter, IKsFilterImpl, Filter);

    This->Sleep = Sleep;
    This->Wake = Wake;
}

/*
    @implemented
*/
KSDDKAPI
PKSFILTER
NTAPI
KsGetFilterFromIrp(
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PKSIOBJECT_HEADER ObjectHeader;

    /* get current irp stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* sanity check */
    ASSERT(IoStack->FileObject);

    /* get object header */
    ObjectHeader = (PKSIOBJECT_HEADER)IoStack->FileObject->FsContext;

    if (ObjectHeader->Type == KsObjectTypeFilter)
    {
        /* irp is targeted at the filter */
        return (PKSFILTER)ObjectHeader->ObjectType;
    }
    else if (ObjectHeader->Type == KsObjectTypePin)
    {
        /* irp is for a pin */
        return KsPinGetParentFilter((PKSPIN)ObjectHeader->ObjectType);
    }
    else
    {
        /* irp is unappropiate to retrieve a filter */
        return NULL;
    }
}
