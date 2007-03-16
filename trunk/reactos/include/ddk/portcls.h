/*
    ReactOS Kernel Streaming
    Port Class

    Andrew Greenwood

    NOTES:
    Does not support PC_OLD_NAMES (which is required for backwards-compatibility
    with older code)

    Obsolete macros are not implemented. For more info:
    http://www.osronline.com/ddkx/stream/audpc-struct_167n.htm


    == EXPORTS ==
    DRM (new in XP):
    * PcAddContentHandlers 
    * PcCreateContentMixed
    * PcDestroyContent
    * PcForwardContentToDeviceObject 
    * PcForwardContentToFileObject
    * PcForwardContentToInterface
    * PcGetContentRights

    IRP HANDLING:
    * PcCompleteIrp 
    * PcDispatchIrp 
    * PcForwardIrpSynchronous

    ADAPTER:
    * PcAddAdapterDevice 
    * PcInitializeAdapterDriver 

    FACTORIES:
    * PcNewDmaChannel 
    * PcNewInterruptSync 
    * PcNewMiniport 
    * PcNewPort 
    * PcNewRegistryKey 
    * PcNewResourceList 
    * PcNewResourceSublist 
    * PcNewServiceGroup 

    POWER MANAGEMENT:
    * PcRegisterAdapterPowerManagement 
    * PcRequestNewPowerState

    PROPERTIES:
    * PcCompletePendingPropertyRequest 
    * PcGetDeviceProperty 

    IO TIMEOUTS:
    * PcRegisterIoTimeout
    * PcUnregisterIoTimeout

    PHYSICAL CONNECTIONS:
    * PcRegisterPhysicalConnection 
    * PcRegisterPhysicalConnectionFromExternal 
    * PcRegisterPhysicalConnectionToExternal 

    MISC:
    * PcGetTimeInterval 
    * PcRegisterSubdevice 


    == AUDIO HELPER OBJECT INTERFACES ==
    IDmaChannel 
    IDmaChannelSlave 
    IDmaOperations 
    IDrmPort                        (XP)
    IDrmPort2                       (XP)
    IInterruptSync 
    IMasterClock 
    IPortClsVersion                 (XP)
    IPortEvents 
    IPreFetchOffset                 (XP)
    IRegistryKey
    IResourceList 
    IServiceGroup 
    IServiceSink
    IUnregisterPhysicalConnection   (Vista)
    IUnregisterSubdevice            (Vista)

    == AUDIO PORT OBJECT INTERFACES ==
    IPort 
    IPortDMus 
    IPortMidi 
    IPortTopology 
    IPortWaveCyclic 
    IPortWavePci

    == AUDIO MINIPORT OBJECT INTERFACES ==
    IMiniport 
    IMiniportDMus 
    IMiniportMidi 
    IMiniportTopology 
    IMiniportWaveCyclic 
    IMiniportWavePci

    == AUDIO MINIPORT AUXILIARY INTERFACES ==
    IMusicTechnology                (XP)
    IPinCount                       (XP)

    == AUDIO STREAM OBJECT INTERFACES ==
    IAllocatorMXF 
    IDrmAudioStream                 (XP)
    IMiniportMidiStream 
    IMiniportWaveCyclicStream 
    IMiniportWavePciStream 
    IMXF 
    IPortWavePciStream 
    ISynthSinkDMus

    == DIRECTMUSIC USERMODE SYNTH AND SYNTH SINK INTERFACES ==
    IDirectMusicSynth 
    IDirectMusicSynthSink

    == AUDIO POWER MANAGEMENT INTERFACES ==
    IAdapterPowerManagement
    IPowerNotify
*/

#ifndef PORTCLS_H
#define PORTCLS_H

/*#include <windef.h>*/
#include <ks.h>
#include <drmk.h>

/* TODO */
#define PORTCLASSAPI EXTERN_C

#define PCFILTER_NODE ((ULONG) -1)

/* HACK */
typedef PVOID CM_RESOURCE_TYPE;

#define PORT_CLASS_DEVICE_EXTENSION_SIZE (64 * sizeof(ULONG_PTR))

/* ===============================================================
    Class IDs - TODO
*/
//#define CLSID_PortDMus    /* dmusicks.h */
DEFINE_GUID(CLSID_PortMidi,0xb4c90a43L, 0x5791, 0x11d0, 0x86, 0xf9, 0x00, 0xa0, 0xc9, 0x11, 0xb5, 0x44);
#define CLSID_PortTopology
#define CLSID_PortWaveCyclic
#define CLSID_PortWavePci

/* first 2 are dmusicks.h */
#define CLSID_MiniportDriverDMusUART
#define CLSID_MiniportDriverDMusUARTCapture
#define CLSID_MiniportDriverFmSynth
#define CLSID_MiniportDriverFmSynthWithVol
DEFINE_GUID(CLSID_MiniportDriverUart,0xb4c90ae1L, 0x5791, 0x11d0, 0x86, 0xf9, 0x00, 0xa0, 0xc9, 0x11, 0xb5, 0x44);


/* ===============================================================
    Event Item Flags - TODO
*/
#define PCEVENT_ITEM_FLAG_ENABLE            KSEVENT_TYPE_ENABLE
#define PCEVENT_ITEM_FLAG_ONESHOT           KSEVENT_TYPE_ONESHOT
#define PCEVENT_ITEM_FLAG_BASICSUPPORT      KSEVENT_TYPE_BASICSUPPORT


/* ===============================================================
    Event Verbs - TODO
*/
#define PCEVENT_VERB_NONE       0
#define PCEVENT_VERB_ADD        1
#define PCEVENT_VERB_REMOVE     2
#define PCEVENT_VERB_SUPPORT    4


/* ===============================================================
    Method Item Flags - TODO
*/
#define PCMETHOD_ITEM_FLAG_NONE             KSMETHOD_TYPE_NONE
#define PCMETHOD_ITEM_FLAG_READ             KSMETHOD_TYPE_READ
#define PCMETHOD_ITEM_FLAG_WRITE            KSMETHOD_TYPE_WRITE
#define PCMETHOD_ITEM_FLAG_MODIFY           KSMETHOD_TYPE_MODIFY
#define PCMETHOD_ITEM_FLAG_SOURCE           KSMETHOD_TYPE_SOURCE


/* ===============================================================
    Method Verbs - TODO
*/
#define PCMETHOD_ITEM_FLAG_BASICSUPPORT     KSMETHOD_TYPE_BASICSUPPORT
#define PCMETHOD_ITEM_FLAG_SEND
#define PCMETHOD_ITEM_FLAG_SETSUPPORT


/* ===============================================================
    Versions
    IoIsWdmVersionAvailable may also be used by older drivers.
*/

enum
{
    kVersionInvalid = -1,

    kVersionWin98,
    kVersionWin98SE,
    kVersionWin2K,
    kVersionWin98SE_QFE2,
    kVersionWin2K_SP2,
    kVersionWinME,
    kVersionWin98SE_QFE3,
    kVersionWinME_QFE1,
    kVersionWinXP,
    kVersionWinXPSP1,
    kVersionWinServer2003,
    kVersionWin2K_UAAQFE,           /* These support IUnregister* interface */
    kVersionWinXP_UAAQFE,
    kVersionWinServer2003_UAAQFE
};

/* ===============================================================
    Properties
*/

struct _PCPROPERTY_REQUEST;

typedef NTSTATUS (*PCPFNPROPERTY_HANDLER)(
    IN  struct _PCPROPERTY_REQUEST* PropertyRequest);

typedef struct _PCPROPERTY_ITEM
{
    const GUID* Set;
    ULONG Id;
    ULONG Flags;
    PCPFNPROPERTY_HANDLER Handler;
} PCPROPERTY_ITEM, *PPCPROPERTY_ITEM;

typedef struct _PCPROPERTY_REQUEST
{
    PUNKNOWN MajorTarget;
    PUNKNOWN MinorTarget;
    ULONG Node;
    const PCPROPERTY_ITEM* PropertyItem;
    ULONG Verb;
    ULONG InstanceSize;
    PVOID Instance;
    ULONG ValueSize;
    PVOID Value;
    PIRP Irp;
} PCPROPERTY_REQUEST, *PPCPROPERTY_REQUEST;

#define PCPROPERTY_ITEM_FLAG_DEFAULTVALUES KSPROPERTY_TYPE_DEFAULTVALUES
#define PCPROPERTY_ITEM_FLAG_GET            KSPROPERTY_TYPE_GET
#define PCPROPERTY_ITEM_FLAG_SET            KSPROPERTY_TYPE_SET
#define PCPROPERTY_ITEM_FLAG_BASICSUPPORT   KSPROPERTY_TYPE_BASICSUPPORT
#define PCPROPERTY_ITEM_FLAG_SERIALIZESIZE  KSPROPERTY_TYPE_SERIALIZESIZE
#define PCPROPERTY_ITEM_FLAG_SERIALIZERAW   KSPROPERTY_TYPE_SERIALIZERAW
#define PCPROPERTY_ITEM_FLAG_UNSERIALIZERAW KSPROPERTY_TYPE_UNSERIALIZERAW
#define PCPROPERTY_ITEM_FLAG_SERIALIZE      ( PCPROPERTY_ITEM_FLAG_SERIALIZERAW \
                                            | PCPROPERTY_ITEM_FLAG_UNSERIALIZERAW \
                                            | PCPROPERTY_ITEM_FLAG_SERIALIZESIZE)


struct _PCEVENT_REQUEST;

typedef NTSTATUS (*PCPFNEVENT_HANDLER)(
    IN  struct _PCEVENT_REQUEST* EventRequest);

typedef struct _PCEVENT_ITEM
{
    const GUID* Set;
    ULONG Id;
    ULONG Flags;
    PCPFNEVENT_HANDLER Handler;
} PCEVENT_ITEM, *PPCEVENT_ITEM;

typedef struct _PCEVENT_REQUEST
{
    PUNKNOWN MajorTarget;
    PUNKNOWN MinorTarget;
    ULONG Node;
    const PCEVENT_ITEM* EventItem;
    PKSEVENT_ENTRY EventEntry;
    ULONG Verb;
    PIRP Irp;
} PCEVENT_REQUEST, *PPCEVENT_REQUEST;



struct _PCMETHOD_REQUEST;

typedef NTSTATUS (*PCPFNMETHOD_HANDLER)(
    IN  struct _PCMETHOD_REQUEST* MethodRequest);

typedef struct _PCMETHOD_ITEM
{
    const GUID* Set;
    ULONG Id;
    ULONG Flags;
    PCPFNMETHOD_HANDLER Handler;
} PCMETHOD_ITEM, *PPCMETHOD_ITEM;

typedef struct _PCMETHOD_REQUEST
{
    PUNKNOWN MajorTarget;
    PUNKNOWN MinorTarget;
    ULONG Node;
    const PCMETHOD_ITEM* MethodItem;
    ULONG Verb;
} PCMETHOD_REQUEST, *PPCMETHOD_REQUEST;


/* ===============================================================
    Structures (unsorted)
*/

typedef struct
{
    ULONG PropertyItemSize;
    ULONG PropertyCount;
    const PCPROPERTY_ITEM* Properties;
    ULONG MethodItemSize;
    ULONG MethodCount;
    const PCMETHOD_ITEM* Methods;
    ULONG EventItemSize;
    ULONG EventCount;
    const PCEVENT_ITEM* Events;
    ULONG Reserved;
} PCAUTOMATION_TABLE, *PPCAUTOMATION_TABLE;

typedef struct
{
    ULONG FromNode;
    ULONG FromNodePin;
    ULONG ToNode;
    ULONG ToNodePin;
} PCCONNECTION_DESCRIPTOR, *PPCCONNECTIONDESCRIPTOR;

typedef struct
{
    ULONG MaxGlobalInstanceCount;
    ULONG MaxFilterInstanceCount;
    ULONG MinFilterInstanceCount;
    const PCAUTOMATION_TABLE* AutomationTable;
    KSPIN_DESCRIPTOR KsPinDescriptor;
} PCPIN_DESCRIPTOR, *PPCPIN_DESCRIPTOR;

typedef struct
{
    ULONG Flags;
    const PCAUTOMATION_TABLE* AutomationTable;
    const GUID* Type;
    const GUID* Name;
} PCNODE_DESCRIPTOR, *PPCNODE_DESCRIPTOR;

typedef struct
{
    ULONG Version;
    const PCAUTOMATION_TABLE* AutomationTable;
    ULONG PinSize;
    ULONG PinCount;
    const PCPIN_DESCRIPTOR* Pins;
    ULONG NodeSize;
    ULONG NodeCount;
    const PCNODE_DESCRIPTOR* Nodes;
    ULONG ConnectionCount;
    const PCCONNECTION_DESCRIPTOR* Connections;
    ULONG CategoryCount;
    const GUID* Categories;
} PCFILTER_DESCRIPTOR, *PPCFILTER_DESCRIPTOR;


/* ===============================================================
    IResourceList Interface
*/

DECLARE_INTERFACE_(IResourceList, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()

    STDMETHOD_(ULONG, NumberOfEntries)( THIS ) PURE;

    STDMETHOD_(ULONG, NumberOfEntriesOfType)( THIS_
        IN  CM_RESOURCE_TYPE Type) PURE;

    STDMETHOD_(PCM_PARTIAL_RESOURCE_DESCRIPTOR, FindTranslatedEntry)( THIS_
        IN  CM_RESOURCE_TYPE Type,
        IN  ULONG Index) PURE;

    STDMETHOD_(PCM_PARTIAL_RESOURCE_DESCRIPTOR, FindUntranslatedEntry)( THIS_
        IN  CM_RESOURCE_TYPE Type,
        IN  ULONG Index) PURE;

    STDMETHOD_(NTSTATUS, AddEntry)( THIS_
        IN  PCM_PARTIAL_RESOURCE_DESCRIPTOR Translated,
        IN  PCM_PARTIAL_RESOURCE_DESCRIPTOR Untranslated) PURE;

    STDMETHOD_(NTSTATUS, AddEntryFromParent)( THIS_
        IN  struct IResourceList* Parent,
        IN  CM_RESOURCE_TYPE Type,
        IN  ULONG Index) PURE;

    STDMETHOD_(PCM_RESOURCE_LIST, TranslatedList)( THIS ) PURE;
    STDMETHOD_(PCM_RESOURCE_LIST, UntranslatedList)( THIS ) PURE;
};

#define IMP_IResourceList \
    STDMETHODIMP_(ULONG) NumberOfEntries(void); \
\
    STDMETHODIMP_(ULONG) NumberOfEntriesOfType( \
        IN  CM_RESOURCE_TYPE Type); \
\
    STDMETHODIMP_(PCM_PARTIAL_RESOURCE_DESCRIPTOR) FindTranslatedEntry( \
        IN  CM_RESOURCE_TYPE Type, \
        IN  ULONG Index); \
\
    STDMETHODIMP_(PCM_PARTIAL_RESOURCE_DESCRIPTOR) FindUntranslatedEntry( \
        IN  CM_RESOURCE_TYPE Type, \
        IN  ULONG Index); \
\
    STDMETHODIMP_(NTSTATUS) AddEntry( \
        IN  PCM_PARTIAL_RESOURCE_DESCRIPTOR Translated, \
        IN  PCM_PARTIAL_RESOURCE_DESCRIPTOR Untranslated); \
\
    STDMETHODIMP_(NTSTATUS) AddEntryFromParent( \
        IN  struct IResourceList* Parent, \
        IN  CM_RESOURCE_TYPE Type, \
        IN  ULONG Index); \
\
    STDMETHODIMP_(PCM_RESOURCE_LIST) TranslatedList(void); \
    STDMETHODIMP_(PCM_RESOURCE_LIST) UntranslatedList(void);

typedef IResourceList *PRESOURCELIST;

#define NumberOfPorts() \
    NumberOfEntriesOfType(CmResourceTypePort)

#define FindTranslatedPort(n) \
    FindTranslatedEntry(CmResourceTypePort, (n))

#define FindUntranslatedPort(n) \
    FindUntranslatedEntry(CmResourceTypePort, (n))

#define AddPortFromParent(p, n) \
    AddEntryFromParent((p), CmResourceTypePort, (n))

/* TODO ... */


/* ===============================================================
    IServiceSink Interface
*/

DECLARE_INTERFACE_(IServiceSink, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()
    STDMETHOD_(void, RequestService)( THIS ) PURE;
};

#define IMP_IServiceSink \
    STDMETHODIMP_(void) RequestService(void);

typedef IServiceSink *PSERVICESINK;


/* ===============================================================
    IServiceGroup Interface
*/

DECLARE_INTERFACE_(IServiceGroup, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()

    STDMETHOD_(void, RequestService)( THIS ) PURE;  /* IServiceSink */

    STDMETHOD_(NTSTATUS, AddMember)( THIS_
        IN  PSERVICESINK pServiceSink) PURE;

    STDMETHOD_(void, RemoveMember)( THIS_
        IN  PSERVICESINK pServiceSink) PURE;

    STDMETHOD_(void, SupportDelayedService)( THIS ) PURE;

    STDMETHOD_(void, RequestDelayedService)( THIS_
        IN  ULONGLONG ullDelay) PURE;

    STDMETHOD_(void, CancelDelayedService)( THIS ) PURE;
};

#define IMP_IServiceGroup \
    IMP_IServiceSink; \
\
    STDMETHODIMP_(NTSTATUS) AddMember( \
        IN  PSERVICESINK pServiceSink); \
\
    STDMETHODIMP_(void) RemoveMember( \
        IN  PSERVICESINK pServiceSink); \
\
    STDMETHODIMP_(void) SupportDelayedService(void); \
\
    STDMETHODIMP_(void) RequestDelayedService( \
        IN  ULONGLONG ullDelay); \
\
    STDMETHODIMP_(void) CancelDelayedService(void);

typedef IServiceGroup *PSERVICEGROUP;


/* ===============================================================
    IRegistryKey Interface
*/

DECLARE_INTERFACE_(IRegistryKey, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()

    STDMETHOD_(NTSTATUS, QueryKey)( THIS_
        IN  KEY_INFORMATION_CLASS KeyInformationClass,
        OUT PVOID KeyInformation,
        IN  ULONG Length,
        OUT PULONG ResultLength) PURE;

    STDMETHOD_(NTSTATUS, EnumerateKey)( THIS_
        IN  ULONG Index,
        IN  KEY_INFORMATION_CLASS KeyInformationClass,
        OUT PVOID KeyInformation,
        IN  ULONG Length,
        OUT PULONG ResultLength) PURE;

    STDMETHOD_(NTSTATUS, QueryValueKey)( THIS_
        IN  PUNICODE_STRING ValueName,
        IN  KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
        OUT PVOID KeyValueInformation,
        IN  ULONG Length,
        OUT PULONG ResultLength) PURE;

    STDMETHOD_(NTSTATUS, EnumerateValueKey)( THIS_
        IN  ULONG Index,
        IN  KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
        OUT PVOID KeyValueInformation,
        IN  ULONG Length,
        OUT PULONG ResultLength) PURE;

    STDMETHOD_(NTSTATUS, SetValueKey)( THIS_
        IN  PUNICODE_STRING ValueName OPTIONAL,
        IN  ULONG Type,
        IN  PVOID Data,
        IN  ULONG DataSize) PURE;

    STDMETHOD_(NTSTATUS, QueryRegistryValues)( THIS_
        IN  PRTL_QUERY_REGISTRY_TABLE QueryTable,
        IN  PVOID Context OPTIONAL) PURE;

    STDMETHOD_(NTSTATUS, NewSubKey)( THIS_
        OUT IRegistryKey** RegistrySubKey,
        IN  PUNKNOWN OuterUnknown,
        IN  ACCESS_MASK DesiredAccess,
        IN  PUNICODE_STRING SubKeyName,
        IN  ULONG CreateOptions,
        OUT PULONG Disposition OPTIONAL) PURE;

    STDMETHOD_(NTSTATUS, DeleteKey)( THIS ) PURE;
};

#define IMP_IRegistryKey \
    STDMETHODIMP_(NTSTATUS) QueryKey( \
        IN  KEY_INFORMATION_CLASS KeyInformationClass, \
        OUT PVOID KeyInformation, \
        IN  ULONG Length, \
        OUT PULONG ResultLength); \
\
    STDMETHODIMP_(NTSTATUS) EnumerateKey( \
        IN  ULONG Index, \
        IN  KEY_INFORMATION_CLASS KeyInformationClass, \
        OUT PVOID KeyInformation, \
        IN  ULONG Length, \
        OUT PULONG ResultLength); \
\
    STDMETHODIMP_(NTSTATUS) QueryValueKey( \
        IN  PUNICODE_STRING ValueName, \
        IN  KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass, \
        OUT PVOID KeyValueInformation, \
        IN  ULONG Length, \
        OUT PULONG ResultLength); \
\
    STDMETHODIMP_(NTSTATUS) EnumerateValueKey( \
        IN  ULONG Index, \
        IN  KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass, \
        OUT PVOID KeyValueInformation, \
        IN  ULONG Length, \
        OUT PULONG ResultLength); \
\
    STDMETHODIMP_(NTSTATUS) SetValueKey( \
        IN  PUNICODE_STRING ValueName OPTIONAL, \
        IN  ULONG Type, \
        IN  PVOID Data, \
        IN  ULONG DataSize); \
\
    STDMETHODIMP_(NTSTATUS) QueryRegistryValues( \
        IN  PRTL_QUERY_REGISTRY_TABLE QueryTable, \
        IN  PVOID Context OPTIONAL); \
\
    STDMETHODIMP_(NTSTATUS) NewSubKey( \
        OUT IRegistryKey** RegistrySubKey, \
        IN  PUNKNOWN OuterUnknown, \
        IN  ACCESS_MASK DesiredAccess, \
        IN  PUNICODE_STRING SubKeyName, \
        IN  ULONG CreateOptions, \
        OUT PULONG Disposition OPTIONAL); \
\
    STDMETHODIMP_(NTSTATUS) DeleteKey(void);

typedef IRegistryKey *PREGISTRYKEY;


/* ===============================================================
    IMusicTechnology Interface
*/

DECLARE_INTERFACE_(IMusicTechnology, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()

    STDMETHOD_(NTSTATUS, SetTechnology)( THIS_
        IN  const GUID* Technology) PURE;
};

#define IMP_IMusicTechnology \
    STDMETHODIMP_(NTSTATUS) SetTechnology( \
        IN  const GUID* Technology);

typedef IMusicTechnology *PMUSICTECHNOLOGY;


/* ===============================================================
    IPort Interface
*/

#define DEFINE_ABSTRACT_PORT() \
    STDMETHOD_(NTSTATUS, Init)( THIS_ \
        IN  PDEVICE_OBJECT DeviceObject, \
        IN  PIRP Irp, \
        IN  PUNKNOWN UnknownMiniport, \
        IN  PUNKNOWN UnknownAdapter OPTIONAL, \
        IN  PRESOURCELIST ResourceList) PURE; \
\
    STDMETHOD_(NTSTATUS, GetDeviceProperty)( THIS_ \
        IN  DEVICE_REGISTRY_PROPERTY DeviceProperty, \
        IN  ULONG BufferLength, \
        OUT PVOID PropertyBuffer, \
        OUT PULONG ResultLength) PURE; \
\
    STDMETHOD_(NTSTATUS, NewRegistryKey)( THIS_ \
        OUT PREGISTRYKEY* OutRegistryKey, \
        IN  PUNKNOWN OuterUnknown OPTIONAL, \
        IN  ULONG RegistryKeyType, \
        IN  ACCESS_MASK DesiredAccess, \
        IN  POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL, \
        IN  ULONG CreateOptiona OPTIONAL, \
        OUT PULONG Disposition OPTIONAL) PURE;

#define IMP_IPort() \
    STDMETHODIMP_(NTSTATUS) Init( \
        IN  PDEVICE_OBJECT DeviceObject, \
        IN  PIRP Irp, \
        IN  PUNKNOWN UnknownMiniport, \
        IN  PUNKNOWN UnknownAdapter OPTIONAL, \
        IN  PRESOURCELIST ResourceList); \
\
    STDMETHODIMP_(NTSTATUS) GetDeviceProperty( \
        IN  DEVICE_REGISTRY_PROPERTY DeviceProperty, \
        IN  ULONG BufferLength, \
        OUT PVOID PropertyBuffer, \
        OUT PULONG ResultLength); \
\
    STDMETHOD_(NTSTATUS) NewRegistryKey( \
        OUT PREGISTRYKEY* OutRegistryKey, \
        IN  PUNKNOWN OuterUnknown OPTIONAL, \
        IN  ULONG RegistryKeyType, \
        IN  ACCESS_MASK DesiredAccess, \
        IN  POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL, \
        IN  ULONG CreateOptiona OPTIONAL, \
        OUT PULONG Disposition OPTIONAL);

DECLARE_INTERFACE_(IPort, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()
    DEFINE_ABSTRACT_PORT()
};

typedef IPort *PPORT;


/* ===============================================================
    IMiniPort Interface
*/

#define DEFINE_ABSTRACT_MINIPORT() \
    STDMETHOD_(NTSTATUS, GetDescription)( THIS_ \
        OUT  PPCFILTER_DESCRIPTOR* Description) PURE; \
\
    STDMETHOD_(NTSTATUS, DataRangeIntersection)( THIS_ \
        IN  ULONG PinId, \
        IN  PKSDATARANGE DataRange, \
        IN  PKSDATARANGE MatchingDataRange, \
        IN  ULONG OutputBufferLength, \
        OUT PVOID ResultantFormat OPTIONAL, \
        OUT PULONG ResultantFormatLength) PURE;

#define IMP_IMiniport \
    STDMETHODIMP_(NTSTATUS) GetDescription( \
        OUT  PPCFILTER_DESCRIPTOR* Description); \
\
    STDMETHODIMP_(NTSTATUS) DataRangeIntersection( \
        IN  ULONG PinId, \
        IN  PKSDATARANGE DataRange, \
        IN  PKSDATARANGE MatchingDataRange, \
        IN  ULONG OutputBufferLength, \
        OUT PVOID ResultantFormat OPTIONAL, \
        OUT PULONG ResultantFormatLength);

DECLARE_INTERFACE_(IMiniport, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()
    DEFINE_ABSTRACT_MINIPORT()
};

typedef IMiniport *PMINIPORT;


/* ===============================================================
    IDmaChannel Interface
*/

#define DEFINE_ABSTRACT_DMACHANNEL() \
    STDMETHOD_(NTSTATUS, AllocateBuffer)( THIS_ \
        IN  ULONG BufferSize, \
        IN  PPHYSICAL_ADDRESS PhysicalAddressConstraint OPTIONAL) PURE; \
\
    STDMETHOD_(void, FreeBuffer)( THIS ) PURE; \
    STDMETHOD_(ULONG, TransferCount)( THIS ) PURE; \
    STDMETHOD_(ULONG, MaximumBufferSize)( THIS ) PURE; \
    STDMETHOD_(ULONG, AllocatedBufferSize)( THIS ) PURE; \
    STDMETHOD_(ULONG, BufferSize)( THIS ) PURE; \
\
    STDMETHOD_(void, SetBufferSize)( THIS_ \
        IN  ULONG BufferSize) PURE; \
\
    STDMETHOD_(PVOID, SystemAddress)( THIS ) PURE; \
    STDMETHOD_(PHYSICAL_ADDRESS, PhysicalAddress)( THIS ) PURE; \
    STDMETHOD_(PADAPTER_OBJECT, GetAdapterObject)( THIS ) PURE; \
\
    STDMETHOD_(void, CopyTo)( THIS_ \
        IN  PVOID Destination, \
        IN  PVOID Source, \
        IN  ULONG ByteCount) PURE; \
\
    STDMETHOD_(void, CopyFrom)( THIS_ \
        IN  PVOID Destination, \
        IN  PVOID Source, \
        IN  ULONG ByteCount) PURE;

#define IMP_IDmaChannel() \
    STDMETHODIMP_(NTSTATUS) AllocateBuffer( \
        IN  ULONG BufferSize, \
        IN  PPHYSICAL_ADDRESS PhysicalAddressConstraint OPTIONAL); \
\
    STDMETHODIMP_(void) FreeBuffer(void); \
    STDMETHODIMP_(ULONG) TransferCount(void); \
    STDMETHODIMP_(ULONG) MaximumBufferSize(void); \
    STDMETHODIMP_(ULONG) AllocatedBufferSize(void); \
    STDMETHODIMP_(ULONG) BufferSize(void); \
\
    STDMETHODIMP_(void) SetBufferSize)( \
        IN  ULONG BufferSize); \
\
    STDMETHODIMP_(PVOID) SystemAddress(void); \
    STDMETHODIMP_(PHYSICAL_ADDRESS) PhysicalAddress(void); \
    STDMETHODIMP_(PADAPTER_OBJECT) GetAdapterObject(void); \
\
    STDMETHODIMP_(void) CopyTo( \
        IN  PVOID Destination, \
        IN  PVOID Source, \
        IN  ULONG ByteCount); \
\
    STDMETHODIMP_(void) CopyFrom( \
        IN  PVOID Destination, \
        IN  PVOID Source, \
        IN  ULONG ByteCount);

DECLARE_INTERFACE_(IDmaChannel, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()
    DEFINE_ABSTRACT_DMACHANNEL()
};

typedef IDmaChannel *PDMACHANNEL;


/* ===============================================================
    IDmaChannelSlave Interface
*/

#define DEFINE_ABSTRACT_DMACHANNELSLAVE() \
    STDMETHOD_(NTSTATUS, Start)( THIS_ \
        IN  ULONG MapSize, \
        IN  BOOLEAN WriteToDevice) PURE; \
\
    STDMETHOD_(NTSTATUS, Stop)( THIS ) PURE; \
    STDMETHOD_(NTSTATUS, ReadCounter)( THIS ) PURE; \
\
    STDMETHOD_(NTSTATUS, WaitForTC)( THIS_ \
        ULONG Timeout) PURE;

#define IMP_IDmaChannelSlave \
    STDMETHODIMP_(NTSTATUS) Start( \
        IN  ULONG MapSize, \
        IN  BOOLEAN WriteToDevice); \
\
    STDMETHODIMP_(NTSTATUS) Stop(void); \
    STDMETHODIMP_(NTSTATUS) ReadCounter)(void); \
\
    STDMETHODIMP_(NTSTATUS, WaitForTC)( \
        ULONG Timeout);

DECLARE_INTERFACE_(IDmaChannelSlave, IDmaChannel)
{
    DEFINE_ABSTRACT_UNKNOWN()
    DEFINE_ABSTRACT_DMACHANNEL()
    DEFINE_ABSTRACT_DMACHANNELSLAVE()
};

typedef IDmaChannelSlave *PDMACHANNELSLAVE;


/* ===============================================================
    IInterruptSync Interface
*/

typedef enum
{
    InterruptSyncModeNormal = 1,
    InterruptSyncModeAll,
    InterruptSyncModeRepeat
} INTERRUPTSYNCMODE;

struct IInterruptSync;

typedef NTSTATUS (*PINTERRUPTSYNCROUTINE)(
    IN  struct IInterruptSync* InterruptSync,
    IN  PVOID DynamicContext);

DECLARE_INTERFACE_(IInterruptSync, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()

    STDMETHOD_(NTSTATUS, CallSynchronizedRoutine)( THIS_
        IN  PINTERRUPTSYNCROUTINE Routine,
        IN  PVOID DynamicContext) PURE;

    STDMETHOD_(PKINTERRUPT, GetKInterrupt)( THIS ) PURE;
    STDMETHOD_(NTSTATUS, Connect)( THIS ) PURE;
    STDMETHOD_(void, Disconnect)( THIS ) PURE;

    STDMETHOD_(NTSTATUS, RegisterServiceRoutine)( THIS_
        IN  PINTERRUPTSYNCROUTINE Routine,
        IN  PVOID DynamicContext,
        IN  BOOLEAN First) PURE;
};

#define IMP_IInterruptSync \
    STDMETHODIMP_(NTSTATUS) CallSynchronizedRoutine)( \
        IN  PINTERRUPTSYNCROUTINE Routine, \
        IN  PVOID DynamicContext); \
\
    STDMETHODIMP_(PKINTERRUPT, GetKInterrupt)(void); \
    STDMETHODIMP_(NTSTATUS, Connect)(void); \
    STDMETHODIMP_(void, Disconnect)(void); \
\
    STDMETHODIMP_(NTSTATUS, RegisterServiceRoutine)( \
        IN  PINTERRUPTSYNCROUTINE Routine, \
        IN  PVOID DynamicContext, \
        IN  BOOLEAN First);

typedef IInterruptSync *PINTERRUPTSYNC;


/* ===============================================================
    IPortMidi Interface
*/

/* ===============================================================
    IMiniportMidiStream Interface
*/

/* ===============================================================
    IMiniportMidi Interface
*/

/* ===============================================================
    IPortTopology Interface
*/

/* ===============================================================
    IMiniportTopology Interface
*/

/* ===============================================================
    IPortWaveCyclic Interface
*/

/* ===============================================================
    IMiniportWaveCyclicStream Interface
*/

/* ===============================================================
    IMiniportWaveCyclic Interface
*/

/* ===============================================================
    IPortWavePci Interface
*/

/* ===============================================================
    IPortWavePciStream Interface
*/

/* ===============================================================
    IMiniportWavePciStream Interface
*/

/* ===============================================================
    IMiniportWavePci Interface
*/

/* ===============================================================
    IAdapterPowerManagement Interface
*/

/* ===============================================================
    IPowerNotify Interface
*/

/* ===============================================================
    IPinCount Interface
*/

/* ===============================================================
    IPortEvents Interface
*/

/* ===============================================================
    IDrmPort / IDrmPort2 Interfaces
    These are almost identical, except for the addition of two extra methods.
*/

/* ===============================================================
    IPortClsVersion Interface
*/

DECLARE_INTERFACE_(IPortClsVersion, IUnknown)
{
    STDMETHOD_(DWORD, GetVersion)(THIS) PURE;
};

#define IMP_IPortClsVersion \
    STDMETHODIMP_(DWORD) GetVersion(void);

typedef IPortClsVersion *PPORTCLSVERSION;


/* ===============================================================
    IDmaOperations Interface
*/

/* ===============================================================
    IPreFetchOffset Interface
*/



/* ===============================================================
    PortCls API Functions
*/

typedef NTSTATUS (*PCPFNSTARTDEVICE)(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp,
    IN  PRESOURCELIST ResourceList);

/* This is in NTDDK.H */
/*
typedef NTSTATUS (*PDRIVER_ADD_DEVICE)(
    IN struct _DRIVER_OBJECT* DriverObject,
    IN struct _DEVICE_OBJECT* PhysicalDeviceObject);
*/

PORTCLASSAPI NTSTATUS NTAPI
PcAddAdapterDevice(
    IN  PDRIVER_OBJECT DriverObject,
    IN  PDEVICE_OBJECT PhysicalDeviceObject,
    IN  PCPFNSTARTDEVICE StartDevice,
    IN  ULONG MaxObjects,
    IN  ULONG DeviceExtensionSize);

PORTCLASSAPI NTSTATUS NTAPI
PcInitializeAdapterDriver(
    IN  PDRIVER_OBJECT DriverObject,
    IN  PUNICODE_STRING RegistryPathName,
    IN  PDRIVER_ADD_DEVICE AddDevice);


/* ===============================================================
    Factories (TODO: Move elsewhere)
*/

PORTCLASSAPI NTSTATUS NTAPI
PcNewDmaChannel(
    OUT PDMACHANNEL* OutDmaChannel,
    IN  PUNKNOWN OuterUnknown OPTIONAL,
    IN  POOL_TYPE PoolType,
    IN  PDEVICE_DESCRIPTION DeviceDescription,
    IN  PDEVICE_OBJECT DeviceObject);

PORTCLASSAPI NTSTATUS NTAPI
PcNewInterruptSync(
    OUT PINTERRUPTSYNC* OUtInterruptSync,
    IN  PUNKNOWN OuterUnknown OPTIONAL,
    IN  PRESOURCELIST ResourceList,
    IN  ULONG ResourceIndex,
    IN  INTERRUPTSYNCMODE Mode);

PORTCLASSAPI NTSTATUS NTAPI
PcNewMiniport(
    OUT PMINIPORT* OutMiniport,
    IN  REFCLSID ClassId);

PORTCLASSAPI NTSTATUS NTAPI
PcNewPort(
    OUT PPORT* OutPort,
    IN  REFCLSID ClassId);

PORTCLASSAPI NTSTATUS NTAPI
PcNewRegistryKey(
    OUT PREGISTRYKEY* OutRegistryKey,
    IN  PUNKNOWN OuterUnknown OPTIONAL,
    IN  ULONG RegistryKeyType,
    IN  ACCESS_MASK DesiredAccess,
    IN  PVOID DeviceObject OPTIONAL,
    IN  PVOID SubDevice OPTIONAL,
    IN  POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN  ULONG CreateOptions OPTIONAL,
    OUT PULONG Disposition OPTIONAL);

PORTCLASSAPI NTSTATUS NTAPI
PcNewResourceList(
    OUT PRESOURCELIST* OutResourceList,
    IN  PUNKNOWN OuterUnknown OPTIONAL,
    IN  POOL_TYPE PoolType,
    IN  PCM_RESOURCE_LIST TranslatedResources,
    IN  PCM_RESOURCE_LIST UntranslatedResources);

PORTCLASSAPI NTSTATUS NTAPI
PcNewResourceSublist(
    OUT PRESOURCELIST* OutResourceList,
    IN  PUNKNOWN OuterUnknown OPTIONAL,
    IN  POOL_TYPE PoolType,
    IN  PRESOURCELIST ParentList,
    IN  ULONG MaximumEntries);

PORTCLASSAPI NTSTATUS NTAPI
PcNewServiceGroup(
    OUT PSERVICEGROUP* OutServiceGroup,
    IN  PUNKNOWN OuterUnknown OPTIONAL);


/* ===============================================================
    IRP Handling
*/

PORTCLASSAPI NTSTATUS NTAPI
PcDispatchIrp(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp);

PORTCLASSAPI NTSTATUS NTAPI
PcCompleteIrp(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp,
    IN  NTSTATUS Status);

PORTCLASSAPI NTSTATUS NTAPI
PcForwardIrpSynchronous(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp);


/* ===============================================================
    Power Management
*/

PORTCLASSAPI NTSTATUS NTAPI
PcRegisterAdapterPowerManagement(
    IN  PUNKNOWN pUnknown,
    IN  PVOID pvContext1);

PORTCLASSAPI NTSTATUS NTAPI
PcRequestNewPowerState(
    IN  PDEVICE_OBJECT pDeviceObject,
    IN  DEVICE_POWER_STATE RequestedNewState);


/* ===============================================================
    Properties
*/

PORTCLASSAPI NTSTATUS NTAPI
PcGetDeviceProperty(
    IN  PVOID DeviceObject,
    IN  DEVICE_REGISTRY_PROPERTY DeviceProperty,
    IN  ULONG BufferLength,
    OUT PVOID PropertyBuffer,
    OUT PULONG ResultLength);

PORTCLASSAPI NTSTATUS NTAPI
PcCompletePendingPropertyRequest(
    IN  PPCPROPERTY_REQUEST PropertyRequest,
    IN  NTSTATUS NtStatus);


/* ===============================================================
    I/O Timeouts
*/

PORTCLASSAPI NTSTATUS NTAPI
PcRegisterIoTimeout(
    IN  PDEVICE_OBJECT pDeviceObject,
    IN  PIO_TIMER_ROUTINE pTimerRoutine,
    IN  PVOID pContext);

PORTCLASSAPI NTSTATUS NTAPI
PcUnregisterIoTimeout(
    IN  PDEVICE_OBJECT pDeviceObject,
    IN  PIO_TIMER_ROUTINE pTimerRoutine,
    IN  PVOID pContext);


/* ===============================================================
    Physical Connections
*/

PORTCLASSAPI NTSTATUS NTAPI
PcRegisterPhysicalConnection(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PUNKNOWN FromUnknown,
    IN  ULONG FromPin,
    IN  PUNKNOWN ToUnknown,
    IN  ULONG ToPin);

PORTCLASSAPI NTSTATUS NTAPI
PcRegisterPhysicalConnectionFromExternal(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PUNICODE_STRING FromString,
    IN  ULONG FromPin,
    IN  PUNKNOWN ToUnknown,
    IN  ULONG ToPin);

PORTCLASSAPI NTSTATUS NTAPI
PcRegisterPhysicalConnectionToExternal(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PUNKNOWN FromUnknown,
    IN  ULONG FromPin,
    IN  PUNICODE_STRING ToString,
    IN  ULONG ToPin);


/* ===============================================================
    Misc
*/

PORTCLASSAPI ULONGLONG NTAPI
PcGetTimeInterval(
    IN  ULONGLONG Since);

PORTCLASSAPI NTSTATUS NTAPI
PcRegisterSubdevice(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PWCHAR Name,
    IN  PUNKNOWN Unknown);


/* ===============================================================
    Digital Rights Management Functions
    Implemented in XP and above
*/

PORTCLASSAPI NTSTATUS NTAPI
PcAddContentHandlers(
    IN  ULONG ContentId,
    IN  PVOID *paHandlers,
    IN  ULONG NumHandlers);

PORTCLASSAPI NTSTATUS NTAPI
PcCreateContentMixed(
    IN  PULONG paContentId,
    IN  ULONG cContentId,
    OUT PULONG pMixedContentId);

PORTCLASSAPI NTSTATUS NTAPI
PcDestroyContent(
    IN  ULONG ContentId);

PORTCLASSAPI NTSTATUS NTAPI
PcForwardContentToDeviceObject(
    IN  ULONG ContentId,
    IN  PVOID Reserved,
    IN  PCDRMFORWARD DrmForward);

PORTCLASSAPI NTSTATUS NTAPI
PcForwardContentToFileObject(
    IN  ULONG ContentId,
    IN  PFILE_OBJECT FileObject);

PORTCLASSAPI NTSTATUS NTAPI
PcForwardContentToInterface(
    IN  ULONG ContentId,
    IN  PUNKNOWN pUnknown,
    IN  ULONG NumMethods);

PORTCLASSAPI NTSTATUS NTAPI
PcGetContentRights(
    IN  ULONG ContentId,
    OUT PDRMRIGHTS DrmRights);


#endif
