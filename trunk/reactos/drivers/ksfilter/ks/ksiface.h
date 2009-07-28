#ifndef KSIFACE_H__
#define KSIFACE_H__

#include <ntddk.h>
#include <ks.h>

#if !defined(DEFINE_ABSTRACT_UNKNOWN)

#define DEFINE_ABSTRACT_UNKNOWN()                               \
    STDMETHOD_(NTSTATUS, QueryInterface)(THIS_                  \
        REFIID InterfaceId,                                     \
        PVOID* Interface)PURE;                                  \
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;                        \
    STDMETHOD_(ULONG,Release)(THIS) PURE;
#endif

typedef struct
{
    LIST_ENTRY Entry;
    LIST_ENTRY ObjectList;
    PRKMUTEX BagMutex;
    PVOID DeviceHeader;
}KSIOBJECT_BAG, *PKSIOBJECT_BAG;


/*****************************************************************************
 * IKsAllocator
 *****************************************************************************
 */

#undef INTERFACE
#define INTERFACE IKsAllocator

DECLARE_INTERFACE_(IKsAllocator, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()

    STDMETHOD_(NTSTATUS, DispatchDeviceIoControl)(THIS_
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp) PURE;

    STDMETHOD_(NTSTATUS, Close)(THIS) PURE;

    STDMETHOD_(NTSTATUS, AllocateFrame)(THIS_
        IN PVOID * OutFrame) PURE;

    STDMETHOD_(VOID, FreeFrame)(THIS_
        IN PVOID OutFrame) PURE;
};


/*****************************************************************************
 * IKsPin
 *****************************************************************************
 */

#undef INTERFACE
#define INTERFACE IKsClock

DECLARE_INTERFACE_(IKsClock, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()
};

/*****************************************************************************
 * IKsPin
 *****************************************************************************
 */

#undef INTERFACE
#define INTERFACE IKsPin

DECLARE_INTERFACE_(IKsPin, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()
};

/*****************************************************************************
 * IKsQueue
 *****************************************************************************
 */

#undef INTERFACE
#define INTERFACE IKsQueue

DECLARE_INTERFACE_(IKsQueue, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()

};

/*****************************************************************************
 * IKsFilterFactory
 *****************************************************************************
 */

#undef INTERFACE
#define INTERFACE IKsFilter

struct KSPROCESSPIPESECTION;


DECLARE_INTERFACE_(IKsFilter, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()

    STDMETHOD_(PKSFILTER, GetStruct)(THIS) PURE;

    STDMETHOD_(BOOL, DoAllNecessaryPinsExist)(THIS) PURE;

    STDMETHOD_(NTSTATUS, CreateNode)(THIS_
        IN PIRP Irp,
        IN IKsPin * Pin,
        IN PLIST_ENTRY ListEntry) PURE;

    STDMETHOD_(NTSTATUS, BindProcessPinsToPipeSection)(THIS_
        IN struct KSPROCESSPIPESECTION *Section,
        IN PVOID Create,
        IN PKSPIN KsPin,
        OUT IKsPin **Pin,
        OUT PKSGATE *OutGate) PURE;

    STDMETHOD_(NTSTATUS, UnbindProcessPinsFromPipeSection)(THIS_
        IN struct KSPROCESSPIPESECTION *Section) PURE;

    STDMETHOD_(NTSTATUS, AddProcessPin)(THIS_
        IN PKSPROCESSPIN ProcessPin) PURE;

    STDMETHOD_(NTSTATUS, RemoveProcessPin)(THIS_
        IN PKSPROCESSPIN ProcessPin) PURE;

    STDMETHOD_(BOOL, ReprepareProcessPipeSection)(THIS_
        IN struct KSPROCESSPIPESECTION *PipeSection,
        IN PULONG Data) PURE;

    STDMETHOD_(VOID, DeliverResetState)(THIS_
        IN struct KSPROCESSPIPESECTION *PipeSection,
        IN KSRESET ResetState) PURE;

    STDMETHOD_(BOOL, IsFrameHolding)(THIS);

    STDMETHOD_(VOID, RegisterForCopyCallbacks)(THIS_
        IKsQueue * Queue,
        IN BOOL Enable) PURE;

    STDMETHOD_(PKSPROCESSPIN_INDEXENTRY, GetProcessDispatch)(THIS);
};

/*****************************************************************************
 * IKsFilterFactory
 *****************************************************************************
 */

#undef INTERFACE
#define INTERFACE IKsFilterFactory

DECLARE_INTERFACE_(IKsFilterFactory, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()

    STDMETHOD_(KSFILTERFACTORY*, GetStruct)(THIS) PURE;

    STDMETHOD_(NTSTATUS, SetDeviceClassesState)(THIS_
        IN BOOLEAN Enable)PURE;

    STDMETHOD_(NTSTATUS, Initialize)(THIS_
        IN PDEVICE_OBJECT  DeviceObject,
        IN const KSFILTER_DESCRIPTOR  *Descriptor,
        IN PWSTR  RefString OPTIONAL,
        IN PSECURITY_DESCRIPTOR  SecurityDescriptor OPTIONAL,
        IN ULONG  CreateItemFlags,
        IN PFNKSFILTERFACTORYPOWER  SleepCallback OPTIONAL,
        IN PFNKSFILTERFACTORYPOWER  WakeCallback OPTIONAL,
        OUT PKSFILTERFACTORY  *FilterFactory OPTIONAL)PURE;

    STDMETHOD_(NTSTATUS, AddFilterInstance)(THIS_
        IN IKsFilter *Filter)PURE;

    STDMETHOD_(NTSTATUS, RemoveFilterInstance)(THIS_
        IN IKsFilter *Filter)PURE;
};


/*****************************************************************************
 * IKsPowerNotify
 *****************************************************************************
 */

#undef INTERFACE
#define INTERFACE IKsPowerNotify

DECLARE_INTERFACE_(IKsPowerNotify, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()

    STDMETHOD_(VOID,Sleep)(THIS_
        IN DEVICE_POWER_STATE State) PURE;

    STDMETHOD_(VOID,Wake)(THIS) PURE;
};


/*****************************************************************************
 * IKsDevice
 *****************************************************************************
 */

#undef INTERFACE
#define INTERFACE IKsDevice

struct KSPOWER_ENTRY;

DECLARE_INTERFACE_(IKsDevice, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()

    STDMETHOD_(KSDEVICE*,GetStruct)(THIS) PURE;

    STDMETHOD_(NTSTATUS, InitializeObjectBag)(THIS_
        IN PKSIOBJECT_BAG Bag,
        IN PRKMUTEX Mutex) PURE;

    STDMETHOD_(NTSTATUS,AcquireDevice)(THIS) PURE;
    STDMETHOD_(NTSTATUS,ReleaseDevice)(THIS) PURE;

    STDMETHOD_(NTSTATUS, GetAdapterObject)(THIS_
        IN PADAPTER_OBJECT Object,
        IN PULONG Unknown1,
        IN PULONG Unknown2) PURE;

    STDMETHOD_(NTSTATUS, AddPowerEntry)(THIS_
        IN struct KSPOWER_ENTRY * Entry,
        IN IKsPowerNotify* Notify)PURE;

    STDMETHOD_(NTSTATUS, RemovePowerEntry)(THIS_
        IN struct KSPOWER_ENTRY * Entry)PURE;

    STDMETHOD_(NTSTATUS, PinStateChange)(THIS_
        IN KSPIN Pin,
        IN PIRP Irp,
        IN KSSTATE OldState,
        IN KSSTATE NewState)PURE;

    STDMETHOD_(NTSTATUS, ArbitrateAdapterChannel)(THIS_
        IN ULONG ControlCode,
        IN IO_ALLOCATION_ACTION Action,
        IN PVOID Context)PURE;

    STDMETHOD_(NTSTATUS, CheckIoCapability)(THIS_
        IN ULONG Unknown)PURE;
};

#endif
