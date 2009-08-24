#ifndef KSFUNC_H__
#define KSFUNC_H__

#include "ksiface.h"
#include "kstypes.h"

#define TAG(A, B, C, D) (ULONG)(((A)<<0) + ((B)<<8) + ((C)<<16) + ((D)<<24))
#define TAG_KSDEVICE TAG('K', 'S', 'E', 'D')
#define TAG_KSOBJECT_TAG TAG('K', 'S', 'O', 'H')

NTSTATUS
NTAPI
KspCreateObjectType(
    IN HANDLE ParentHandle,
    IN LPWSTR ObjectType,
    PVOID CreateParameters,
    UINT CreateParametersSize,
    IN  ACCESS_MASK DesiredAccess,
    OUT PHANDLE NodeHandle);

NTSTATUS
NTAPI
KspCreateFilterFactory(
    IN PDEVICE_OBJECT  DeviceObject,
    IN const KSFILTER_DESCRIPTOR  *Descriptor,
    IN PWSTR  RefString OPTIONAL,
    IN PSECURITY_DESCRIPTOR  SecurityDescriptor OPTIONAL,
    IN ULONG  CreateItemFlags,
    IN PFNKSFILTERFACTORYPOWER  SleepCallback OPTIONAL,
    IN PFNKSFILTERFACTORYPOWER  WakeCallback OPTIONAL,
    OUT PKSFILTERFACTORY *FilterFactory OPTIONAL);

NTSTATUS
NTAPI
IKsDevice_Create(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP Irp);

NTSTATUS
NTAPI
IKsDevice_Pnp(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP Irp);

NTSTATUS
NTAPI
IKsDevice_Power(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP Irp);

NTSTATUS
NTAPI
KspCreateFilter(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN IKsFilterFactory *iface);

NTSTATUS
KspSetDeviceInterfacesState(
    IN PLIST_ENTRY ListHead,
    IN BOOL Enable);

NTSTATUS
KspFreeDeviceInterfaces(
    IN PLIST_ENTRY ListHead);

NTSTATUS
KspRegisterDeviceInterfaces(
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN ULONG CategoriesCount,
    IN GUID const*Categories,
    IN PUNICODE_STRING ReferenceString,
    OUT PLIST_ENTRY SymbolicLinkList);

NTSTATUS
KspForwardIrpSynchronous(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);

PVOID
AllocateItem(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes);

VOID
FreeItem(
    IN PVOID Item);

NTSTATUS
NTAPI
KspTopologyPropertyHandler(
    IN PIRP Irp,
    IN PKSIDENTIFIER  Request,
    IN OUT PVOID  Data);

NTSTATUS
NTAPI
KspPinPropertyHandler(
    IN PIRP Irp,
    IN PKSIDENTIFIER  Request,
    IN OUT PVOID  Data);

NTSTATUS
FindMatchingCreateItem(
    PLIST_ENTRY ListHead,
    ULONG BufferSize,
    LPWSTR Buffer,
    OUT PCREATE_ITEM_ENTRY *OutCreateItem);

NTSTATUS
KspCopyCreateRequest(
    IN PIRP Irp,
    IN LPWSTR ObjectClass,
    IN OUT PULONG Size,
    OUT PVOID * Result);

NTSTATUS
KspCreatePin(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PKSDEVICE KsDevice,
    IN IKsFilterFactory * FilterFactory,
    IN IKsFilter* Filter,
    IN PKSPIN_CONNECT Connect,
    IN KSPIN_DESCRIPTOR_EX* Descriptor);

NTSTATUS
IKsFilter_AddPin(
    IKsFilter * Filter,
    PKSPIN Pin);

NTSTATUS
KspAddCreateItemToList(
    OUT PLIST_ENTRY ListHead,
    IN ULONG ItemsCount,
    IN  PKSOBJECT_CREATE_ITEM ItemsList);

VOID
KspFreeCreateItems(
    IN PLIST_ENTRY ListHead);

NTSTATUS
KspPropertyHandler(
    IN PIRP Irp,
    IN  ULONG PropertySetsCount,
    IN  const KSPROPERTY_SET* PropertySet,
    IN  PFNKSALLOCATOR Allocator OPTIONAL,
    IN  ULONG PropertyItemSize OPTIONAL);


#endif
