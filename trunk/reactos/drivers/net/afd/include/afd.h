/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Ancillary Function Driver
 * FILE:        include/afd.h
 * PURPOSE:     Main driver header
 */
#ifndef __AFD_H
#define __AFD_H

#include <winsock2.h>
#include <ddk/ntddk.h>
#include <net/tdikrnl.h>
#include <net/tdiinfo.h>
#include <afd/shared.h>
#include <debug.h>

/* Forward declarations */
struct _AFDFCB;

typedef struct _DEVICE_EXTENSION {
    PDEVICE_OBJECT StorageDevice;
    KSPIN_LOCK FCBListLock;
    LIST_ENTRY FCBListHead;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

/* Context Control Block structure */
typedef struct _AFDCCB {
	struct _AFDFCB *FCB;
	LIST_ENTRY      ListEntry;
	PFILE_OBJECT    FileObject;
	ULONG           Flags;
	LARGE_INTEGER   CurrentByteOffset;
} AFDCCB, *PAFDCCB;

/* Flags for CCB structure */
#define	CCB_CLEANED     0x00000001

/* Borrowed from http://www.acc.umu.se/~bosse/ntifs.h by Bo Branten */
typedef struct _FSRTL_COMMON_FCB_HEADER {
    CSHORT          NodeTypeCode;
    CSHORT          NodeByteSize;
    UCHAR           Flags;
    UCHAR           IsFastIoPossible;
    UCHAR           Flags2;
    UCHAR           Reserved;
    PERESOURCE      Resource;
    PERESOURCE      PagingIoResource;
    LARGE_INTEGER   AllocationSize;
    LARGE_INTEGER   FileSize;
    LARGE_INTEGER   ValidDataLength;
} FSRTL_COMMON_FCB_HEADER, *PFSRTL_COMMON_FCB_HEADER;

typedef struct _FsdNTRequiredFCB {
    FSRTL_COMMON_FCB_HEADER CommonFCBHeader;
    SECTION_OBJECT_POINTERS SectionObject;
    ERESOURCE               MainResource;
    ERESOURCE               PagingIoResource;
} FsdNTRequiredFCB, *PFsdNTRequiredFCB;

typedef struct _AFDFCB {
    FsdNTRequiredFCB    NTRequiredFCB;
    LIST_ENTRY          ListEntry;
    PDEVICE_EXTENSION   DeviceExt;
    SHARE_ACCESS        ShareAccess;
    ULONG               ReferenceCount;
    ULONG               OpenHandleCount;
    HANDLE              TdiAddressObjectHandle;
    PFILE_OBJECT        TdiAddressObject;
    HANDLE              TdiConnectionObjectHandle;
    PFILE_OBJECT        TdiConnectionObject;
    LIST_ENTRY          CCBListHead;
    INT                 AddressFamily;
    INT                 SocketType;
    INT                 Protocol;
    PVOID               HelperContext;
    DWORD               NotificationEvents;
    UNICODE_STRING      TdiDeviceName;
    DWORD               State;
} AFDFCB, *PAFDFCB;

/* Socket states */
#define SOCKET_STATE_CREATED    0
#define SOCKET_STATE_BOUND      1
#define SOCKET_STATE_LISTENING  2

typedef struct IPSNMP_INFO {
	ULONG Forwarding;
	ULONG DefaultTTL;
	ULONG InReceives;
	ULONG InHdrErrors;
	ULONG InAddrErrors;
	ULONG ForwDatagrams;
	ULONG InUnknownProtos;
	ULONG InDiscards;
	ULONG InDelivers;
	ULONG OutRequests;
	ULONG RoutingDiscards;
	ULONG OutDiscards;
	ULONG OutNoRoutes;
	ULONG ReasmTimeout;
	ULONG ReasmReqds;
	ULONG ReasmOks;
	ULONG ReasmFails;
	ULONG FragOks;
	ULONG FragFails;
	ULONG FragCreates;
	ULONG NumIf;
	ULONG NumAddr;
	ULONG NumRoutes;
} IPSNMP_INFO, *PIPSNMP_INFO;

typedef struct IPADDR_ENTRY {
	ULONG  Addr;
	ULONG  Index;
	ULONG  Mask;
	ULONG  BcastAddr;
	ULONG  ReasmSize;
	USHORT Context;
	USHORT Pad;
} IPADDR_ENTRY, *PIPADDR_ENTRY;


#define TL_INSTANCE 0

#define IP_MIB_STATS_ID             0x1
#define IP_MIB_ADDRTABLE_ENTRY_ID   0x102


/* IOCTL codes */

#define IOCTL_TCP_QUERY_INFORMATION_EX \
	    CTL_CODE(FILE_DEVICE_NETWORK, 0, METHOD_NEITHER, FILE_ANY_ACCESS)

#define IOCTL_TCP_SET_INFORMATION_EX  \
	    CTL_CODE(FILE_DEVICE_NETWORK, 1, METHOD_BUFFERED, FILE_WRITE_ACCESS)


#ifdef i386

/* DWORD network to host byte order conversion for i386 */
#define DN2H(dw) \
    ((((dw) & 0xFF000000L) >> 24) | \
	 (((dw) & 0x00FF0000L) >> 8) | \
	 (((dw) & 0x0000FF00L) << 8) | \
	 (((dw) & 0x000000FFL) << 24))

/* DWORD host to network byte order conversion for i386 */
#define DH2N(dw) \
	((((dw) & 0xFF000000L) >> 24) | \
	 (((dw) & 0x00FF0000L) >> 8) | \
	 (((dw) & 0x0000FF00L) << 8) | \
	 (((dw) & 0x000000FFL) << 24))

/* WORD network to host order conversion for i386 */
#define WN2H(w) \
	((((w) & 0xFF00) >> 8) | \
	 (((w) & 0x00FF) << 8))

/* WORD host to network byte order conversion for i386 */
#define WH2N(w) \
	((((w) & 0xFF00) >> 8) | \
	 (((w) & 0x00FF) << 8))

#else /* i386 */

/* DWORD network to host byte order conversion for other architectures */
#define DN2H(dw) \
    (dw)

/* DWORD host to network byte order conversion for other architectures */
#define DH2N(dw) \
    (dw)

/* WORD network to host order conversion for other architectures */
#define WN2H(w) \
    (w)

/* WORD host to network byte order conversion for other architectures */
#define WH2N(w) \
    (w)

#endif /* i386 */


/* Prototypes from dispatch.c */

NTSTATUS AfdDispBind(
    PIRP Irp,
    PIO_STACK_LOCATION IrpSp);

NTSTATUS AfdDispListen(
    PIRP Irp,
    PIO_STACK_LOCATION IrpSp);

NTSTATUS AfdDispSendTo(
    PIRP Irp,
    PIO_STACK_LOCATION IrpSp);

NTSTATUS AfdDispRecvFrom(
    PIRP Irp,
    PIO_STACK_LOCATION IrpSp);

/* Prototypes from event.c */

NTSTATUS AfdRegisterEventHandlers(
    PAFDFCB FCB);

NTSTATUS AfdDeregisterEventHandlers(
    PAFDFCB FCB);

/* Prototypes from opnclose.c */

NTSTATUS AfdCreate(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp);

NTSTATUS AfdCreateNamedPipe(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp);

NTSTATUS AfdClose(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp);

/* Prototypes from rdwr.c */

NTSTATUS AfdEventReceiveDatagramHandler(
    IN PVOID TdiEventContext,
    IN LONG SourceAddressLength,
    IN PVOID SourceAddress,
    IN LONG OptionsLength,
    IN PVOID Options,
    IN ULONG ReceiveDatagramFlags,
    IN ULONG BytesIndicated,
    IN ULONG BytesAvailable,
    OUT ULONG * BytesTaken,
    IN PVOID Tsdu,
    OUT PIRP * IoRequestPacket);

NTSTATUS AfdRead(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp);

NTSTATUS AfdWrite(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp);

/* Prototypes from tdi.c */

NTSTATUS TdiCloseDevice(
    HANDLE Handle,
    PFILE_OBJECT FileObject);

NTSTATUS TdiOpenAddressFileIPv4(
    PUNICODE_STRING DeviceName,
    LPSOCKADDR Name,
    PHANDLE AddressHandle,
    PFILE_OBJECT *AddressObject);

NTSTATUS TdiSetEventHandler(
    PFILE_OBJECT FileObject,
    LONG EventType,
    PVOID Handler,
    PVOID Context);

NTSTATUS TdiQueryDeviceControl(
    PFILE_OBJECT FileObject,
    ULONG IoControlCode,
    PVOID InputBuffer,
    ULONG InputBufferLength,
    PVOID OutputBuffer,
    ULONG OutputBufferLength,
    PULONG Return);

NTSTATUS TdiQueryInformationEx(
    PFILE_OBJECT FileObject,
    ULONG Entity,
    ULONG Instance,
    ULONG Class,
    ULONG Type,
    ULONG Id,
    PVOID OutputBuffer,
    PULONG OutputLength);

NTSTATUS TdiQueryAddress(
    PFILE_OBJECT FileObject,
    PULONG Address);

NTSTATUS TdiSend(
    PFILE_OBJECT TransportObject,
    PFILE_REQUEST_SENDTO Request);

NTSTATUS TdiSendDatagram(
    PFILE_OBJECT TransportObject,
    LPSOCKADDR Address,
    PVOID Buffer,
    ULONG BufferSize);

#endif /*__AFD_H */

/* EOF */
