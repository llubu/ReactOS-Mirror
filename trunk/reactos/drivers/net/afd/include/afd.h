/* $Id: afd.h,v 1.19 2004/07/18 22:49:17 arty Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/net/afd/include/afd.h
 * PURPOSE:          Ancillary functions driver -- constants and structures
 * PROGRAMMER:       Art Yerkes (ayerkes@speakeasy.net)
 * UPDATE HISTORY:
 * 20040630 Created
 */

#ifndef _AFD_H
#define _AFD_H

#include <ntddk.h>
#include <tdi.h>
#include <tdikrnl.h>
#include <tdiinfo.h>
#include <string.h>
#include <ndis.h>
#include <shared.h>

#ifndef _MSC_VER
#include <rosrtl/string.h>
#include <winsock2.h>
#include <ddk/tdi.h>
#include <ddk/ndis.h>
#include <tcpmisc.h>
#include <tcpioctl.h>
#else
#include <ntdef.h>
#define STDCALL
#endif

#ifndef MIN
#define MIN(x,y) (((x)<(y))?(x):(y))
#endif

#define SOCKET_STATE_INVALID_TRANSITION ((DWORD)-1)
#define SOCKET_STATE_CREATED            0
#define SOCKET_STATE_BOUND              1
#define SOCKET_STATE_CONNECTING         2
#define SOCKET_STATE_CONNECTED          3
#define SOCKET_STATE_LISTENING          4
#define SOCKET_STATE_MASK               0x0000ffff
#define SOCKET_STATE_LOCKED             0x40000000
#define SOCKET_STATE_NEW                0x80000000
#define SOCKET_STATE_CLOSED             0x00000100

#define FUNCTION_CONNECT                0
#define FUNCTION_RECV                   1
#define FUNCTION_RECV_DATAGRAM          2
#define FUNCTION_SEND                   3
#define FUNCTION_CLOSE                  4
#define MAX_FUNCTIONS                   5

#define IN_FLIGHT_REQUESTS              3

#define DEFAULT_SEND_WINDOW_SIZE        16384
#define DEFAULT_RECEIVE_WINDOW_SIZE     16384

#define SGID_CONNECTIONLESS             1 /* XXX Find this flag */

typedef struct _AFD_MAPBUF {
    PVOID BufferAddress;
    PMDL  Mdl;
} AFD_MAPBUF, *PAFD_MAPBUF;

typedef struct _AFD_DEVICE_EXTENSION {
    PDEVICE_OBJECT DeviceObject;
    LIST_ENTRY Polls;
    KSPIN_LOCK Lock;
} AFD_DEVICE_EXTENSION, *PAFD_DEVICE_EXTENSION;

typedef struct _AFD_ACTIVE_POLL {
    LIST_ENTRY ListEntry;
    PIRP Irp;
    PAFD_DEVICE_EXTENSION DeviceExt;
    KDPC TimeoutDpc;
    KTIMER Timer;
} AFD_ACTIVE_POLL, *PAFD_ACTIVE_POLL;

typedef struct _IRP_LIST {
    LIST_ENTRY ListEntry;
    PIRP Irp;
} IRP_LIST, *PIRP_LIST;

typedef struct _AFD_TDI_OBJECT {
    PFILE_OBJECT Object;
    HANDLE Handle;
} AFD_TDI_OBJECT, *PAFD_TDI_OBJECT;

typedef struct _AFD_IN_FLIGHT_REQUEST {
    PIRP InFlightRequest;
    IO_STATUS_BLOCK Iosb;    
    PTDI_CONNECTION_INFORMATION ConnectionInfo;
} AFD_IN_FLIGHT_REQUEST, *PAFD_IN_FLIGHT_REQUEST;

typedef struct _AFD_DATA_WINDOW {
    PCHAR Window;
    UINT BytesUsed, Size, Content;
} AFD_DATA_WINDOW, *PAFD_DATA_WINDOW;

typedef struct _AFD_STORED_DATAGRAM {
    LIST_ENTRY ListEntry;
    UINT Len;
    PTA_ADDRESS Address;
    CHAR Buffer[1];
} AFD_STORED_DATAGRAM, *PAFD_STORED_DATAGRAM;

typedef struct _AFD_FCB {
    BOOLEAN Locked;
    UINT State, Flags;
    KIRQL OldIrql;
    UINT LockCount;
    PVOID CurrentThread;
    KSPIN_LOCK SpinLock;
    PFILE_OBJECT FileObject;
    PAFD_DEVICE_EXTENSION DeviceExt;
    BOOLEAN DelayedAccept;
    PTRANSPORT_ADDRESS LocalAddress, RemoteAddress;
    PTDI_CONNECTION_INFORMATION AddressFrom;
    AFD_TDI_OBJECT AddressFile, Connection;
    AFD_IN_FLIGHT_REQUEST ListenIrp, ReceiveIrp, SendIrp;
    AFD_DATA_WINDOW Send, Recv;
    FAST_MUTEX Mutex;
    KEVENT StateLockedEvent;
    UNICODE_STRING TdiDeviceName;
    PVOID Context;
    DWORD PollState;
    UINT ContextSize;
    PIRP PendingTdiIrp;
    LIST_ENTRY PendingIrpList[MAX_FUNCTIONS];
    LIST_ENTRY DatagramList;
} AFD_FCB, *PAFD_FCB;

/* bind.c */

NTSTATUS WarmSocketForBind( PAFD_FCB FCB );
NTSTATUS STDCALL
AfdBindSocket(PDEVICE_OBJECT DeviceObject, PIRP Irp,
	      PIO_STACK_LOCATION IrpSp);

/* connect.c */

NTSTATUS WarmSocketForConnection( PAFD_FCB FCB );
NTSTATUS STDCALL
AfdStreamSocketConnect(PDEVICE_OBJECT DeviceObject, PIRP Irp,
		       PIO_STACK_LOCATION IrpSp);

/* context.c */

NTSTATUS STDCALL
AfdGetContext( PDEVICE_OBJECT DeviceObject, PIRP Irp, 
	       PIO_STACK_LOCATION IrpSp );
NTSTATUS STDCALL
AfdSetContext( PDEVICE_OBJECT DeviceObject, PIRP Irp, 
	       PIO_STACK_LOCATION IrpSp );

/* info.c */

NTSTATUS STDCALL
AfdGetInfo( PDEVICE_OBJECT DeviceObject, PIRP Irp, 
	    PIO_STACK_LOCATION IrpSp );

/* listen.c */

NTSTATUS AfdListenSocket(PDEVICE_OBJECT DeviceObject, PIRP Irp,
			 PIO_STACK_LOCATION IrpSp);

/* lock.c */

PAFD_WSABUF LockBuffers( PAFD_WSABUF Buf, UINT Count, BOOLEAN Write );
VOID UnlockBuffers( PAFD_WSABUF Buf, UINT Count );
UINT SocketAcquireStateLock( PAFD_FCB FCB );
NTSTATUS DDKAPI UnlockAndMaybeComplete
( PAFD_FCB FCB, NTSTATUS Status, PIRP Irp,
  UINT Information, 
  PIO_COMPLETION_ROUTINE Completion,
  BOOL ShouldUnlockIrp );
VOID SocketStateUnlock( PAFD_FCB FCB );
NTSTATUS LostSocket( PIRP Irp, BOOL ShouldUnlockIrp );
PVOID LockRequest( PIRP Irp, PIO_STACK_LOCATION IrpSp );
VOID UnlockRequest( PIRP Irp, PIO_STACK_LOCATION IrpSp );

/* main.c */

VOID OskitDumpBuffer( PCHAR Buffer, UINT Len );
NTSTATUS LeaveIrpUntilLater( PAFD_FCB FCB, PIRP Irp, UINT Function );

/* read.c */

NTSTATUS DDKAPI ReceiveComplete
( PDEVICE_OBJECT DeviceObject,
  PIRP Irp,
  PVOID Context );

NTSTATUS DDKAPI PacketSocketRecvComplete
( PDEVICE_OBJECT DeviceObject,
  PIRP Irp,
  PVOID Context );

NTSTATUS STDCALL
AfdConnectedSocketReadData(PDEVICE_OBJECT DeviceObject, PIRP Irp, 
			   PIO_STACK_LOCATION IrpSp, BOOLEAN Short);
NTSTATUS STDCALL
AfdPacketSocketReadData(PDEVICE_OBJECT DeviceObject, PIRP Irp, 
			PIO_STACK_LOCATION IrpSp );

/* select.c */

NTSTATUS STDCALL
AfdSelect( PDEVICE_OBJECT DeviceObject, PIRP Irp, 
	   PIO_STACK_LOCATION IrpSp );
VOID PollReeval( PAFD_DEVICE_EXTENSION DeviceObject, PFILE_OBJECT FileObject );

/* tdi.c */

NTSTATUS TdiOpenAddressFile(
    PUNICODE_STRING DeviceName,
    PTRANSPORT_ADDRESS Name,
    PHANDLE AddressHandle,
    PFILE_OBJECT *AddressObject);

NTSTATUS TdiAssociateAddressFile(
  HANDLE AddressHandle,
  PFILE_OBJECT ConnectionObject);

NTSTATUS TdiListen
( PIRP *Irp,
  PFILE_OBJECT ConnectionObject,
  PTDI_CONNECTION_INFORMATION *RequestConnectionInfo,
  PIO_STATUS_BLOCK Iosb,
  PIO_COMPLETION_ROUTINE  CompletionRoutine,
  PVOID CompletionContext);

NTSTATUS TdiReceive
( PIRP *Irp,
  PFILE_OBJECT ConnectionObject,
  USHORT Flags,
  PCHAR Buffer,
  UINT BufferLength,
  PIO_STATUS_BLOCK Iosb,
  PIO_COMPLETION_ROUTINE  CompletionRoutine,
  PVOID CompletionContext);

NTSTATUS TdiSend
( PIRP *Irp,
  PFILE_OBJECT ConnectionObject,
  USHORT Flags,
  PCHAR Buffer,
  UINT BufferLength,
  PIO_STATUS_BLOCK Iosb,
  PIO_COMPLETION_ROUTINE  CompletionRoutine,
  PVOID CompletionContext);

NTSTATUS TdiReceiveDatagram(
    PIRP *Irp,
    PFILE_OBJECT TransportObject,
    USHORT Flags,
    PCHAR Buffer,
    UINT BufferLength,
    PTDI_CONNECTION_INFORMATION From,
    PIO_STATUS_BLOCK Iosb,
    PIO_COMPLETION_ROUTINE CompletionRoutine,
    PVOID CompletionContext);

NTSTATUS TdiSendDatagram(
    PIRP *Irp,
    PFILE_OBJECT TransportObject,
    PCHAR Buffer,
    UINT BufferLength,
    PTDI_CONNECTION_INFORMATION To,
    PIO_STATUS_BLOCK Iosb,
    PIO_COMPLETION_ROUTINE CompletionRoutine,
    PVOID CompletionContext);

/* write.c */

NTSTATUS STDCALL
AfdConnectedSocketWriteData(PDEVICE_OBJECT DeviceObject, PIRP Irp, 
			    PIO_STACK_LOCATION IrpSp, BOOLEAN Short);
NTSTATUS STDCALL
AfdPacketSocketWriteData(PDEVICE_OBJECT DeviceObject, PIRP Irp, 
			 PIO_STACK_LOCATION IrpSp);

#endif/*_AFD_H*/
