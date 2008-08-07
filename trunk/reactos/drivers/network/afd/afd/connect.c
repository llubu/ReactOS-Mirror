/* $Id$
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/net/afd/afd/connect.c
 * PURPOSE:          Ancillary functions driver
 * PROGRAMMER:       Art Yerkes (ayerkes@speakeasy.net)
 * UPDATE HISTORY:
 * 20040708 Created
 */
#include "afd.h"
#include "tdi_proto.h"
#include "tdiconn.h"
#include "debug.h"

NTSTATUS WarmSocketForConnection( PAFD_FCB FCB ) {
    NTSTATUS Status;

    if( !FCB->TdiDeviceName.Length || !FCB->TdiDeviceName.Buffer ) {
        AFD_DbgPrint(MID_TRACE,("Null Device\n"));
        return STATUS_NO_SUCH_DEVICE;
    }

    Status = TdiOpenConnectionEndpointFile(&FCB->TdiDeviceName,
                                           &FCB->Connection.Handle,
                                           &FCB->Connection.Object );

    if( NT_SUCCESS(Status) ) {
        Status = TdiAssociateAddressFile( FCB->AddressFile.Handle,
                                          FCB->Connection.Object );
    }

    return Status;
}

NTSTATUS MakeSocketIntoConnection( PAFD_FCB FCB ) {
    NTSTATUS Status = STATUS_NO_MEMORY;

    /* Allocate the receive area and start receiving */
    FCB->Recv.Window =
	ExAllocatePool( NonPagedPool, FCB->Recv.Size );
    FCB->Send.Window =
	ExAllocatePool( NonPagedPool, FCB->Send.Size );

    FCB->State = SOCKET_STATE_CONNECTED;

    if( FCB->Recv.Window ) {
	Status = TdiReceive( &FCB->ReceiveIrp.InFlightRequest,
			     FCB->Connection.Object,
			     TDI_RECEIVE_NORMAL,
			     FCB->Recv.Window,
			     FCB->Recv.Size,
			     &FCB->ReceiveIrp.Iosb,
			     ReceiveComplete,
			     FCB );
    }

    return Status;
}

static NTSTATUS NTAPI StreamSocketConnectComplete
( PDEVICE_OBJECT DeviceObject,
  PIRP Irp,
  PVOID Context ) {
    NTSTATUS Status = Irp->IoStatus.Status;
    PAFD_FCB FCB = (PAFD_FCB)Context;
    PLIST_ENTRY NextIrpEntry;
    PIRP NextIrp;

    AFD_DbgPrint(MID_TRACE,("Called: FCB %x, FO %x\n",
			    Context, FCB->FileObject));

    /* I was wrong about this before as we can have pending writes to a not
     * yet connected socket */
    if( !SocketAcquireStateLock( FCB ) ) return LostSocket( Irp );

    AFD_DbgPrint(MID_TRACE,("Irp->IoStatus.Status = %x\n",
			    Irp->IoStatus.Status));

    if( NT_SUCCESS(Irp->IoStatus.Status) ) {
	FCB->PollState |= AFD_EVENT_CONNECT | AFD_EVENT_SEND;
	FCB->State = SOCKET_STATE_CONNECTED;
	AFD_DbgPrint(MID_TRACE,("Going to connected state %d\n", FCB->State));
	PollReeval( FCB->DeviceExt, FCB->FileObject );
    } else {
	FCB->PollState |= AFD_EVENT_CONNECT_FAIL | AFD_EVENT_RECEIVE;
	AFD_DbgPrint(MID_TRACE,("Going to bound state\n"));
	FCB->State = SOCKET_STATE_BOUND;
	PollReeval( FCB->DeviceExt, FCB->FileObject );
    }

    /* Succeed pending irps on the FUNCTION_CONNECT list */
    while( !IsListEmpty( &FCB->PendingIrpList[FUNCTION_CONNECT] ) ) {
	NextIrpEntry = RemoveHeadList(&FCB->PendingIrpList[FUNCTION_CONNECT]);
	NextIrp = CONTAINING_RECORD(NextIrpEntry, IRP, Tail.Overlay.ListEntry);
	AFD_DbgPrint(MID_TRACE,("Completing connect %x\n", NextIrp));
	NextIrp->IoStatus.Status = Status;
	NextIrp->IoStatus.Information = 0;
	if( NextIrp->MdlAddress ) UnlockRequest( NextIrp, IoGetCurrentIrpStackLocation( NextIrp ) );
	IoCompleteRequest( NextIrp, IO_NETWORK_INCREMENT );
    }

    if( NT_SUCCESS(Status) ) {
	Status = MakeSocketIntoConnection( FCB );

	if( FCB->Send.Window &&
	    !IsListEmpty( &FCB->PendingIrpList[FUNCTION_SEND] ) ) {
	    NextIrpEntry = RemoveHeadList(&FCB->PendingIrpList[FUNCTION_SEND]);
	    NextIrp = CONTAINING_RECORD(NextIrpEntry, IRP,
					Tail.Overlay.ListEntry);
	    AFD_DbgPrint(MID_TRACE,("Launching send request %x\n", NextIrp));
	    Status = AfdConnectedSocketWriteData
		( DeviceObject,
		  NextIrp,
		  IoGetCurrentIrpStackLocation( NextIrp ),
		  FALSE );
	}

	if( Status == STATUS_PENDING )
	    Status = STATUS_SUCCESS;
    }

    SocketStateUnlock( FCB );

    AFD_DbgPrint(MID_TRACE,("Returning %x\n", Status));

    return Status;
}

/* Return the socket object for ths request only if it is a connected or
   stream type. */
NTSTATUS STDCALL
AfdStreamSocketConnect(PDEVICE_OBJECT DeviceObject, PIRP Irp,
		       PIO_STACK_LOCATION IrpSp) {
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    PTDI_CONNECTION_INFORMATION TargetAddress;
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    PAFD_FCB FCB = FileObject->FsContext;
    PAFD_CONNECT_INFO ConnectReq;
    AFD_DbgPrint(MID_TRACE,("Called on %x\n", FCB));

    if( !SocketAcquireStateLock( FCB ) ) return LostSocket( Irp );
    if( !(ConnectReq = LockRequest( Irp, IrpSp )) )
	return UnlockAndMaybeComplete( FCB, STATUS_NO_MEMORY, Irp,
				       0, NULL );

    AFD_DbgPrint(MID_TRACE,("Connect request:\n"));
#if 0
    OskitDumpBuffer
	( (PCHAR)ConnectReq,
	  IrpSp->Parameters.DeviceIoControl.InputBufferLength );
#endif

    switch( FCB->State ) {
    case SOCKET_STATE_CONNECTED:
	Status = STATUS_SUCCESS;
	break;

    case SOCKET_STATE_CONNECTING:
	return LeaveIrpUntilLater( FCB, Irp, FUNCTION_CONNECT );

    case SOCKET_STATE_CREATED: {
	FCB->LocalAddress =
	    TaCopyTransportAddress( &ConnectReq->RemoteAddress );

	if( FCB->LocalAddress ) {
	    RtlZeroMemory( FCB->LocalAddress,
			   TaLengthOfTransportAddress
			   ( &ConnectReq->RemoteAddress ) );

	    FCB->LocalAddress->TAAddressCount = 1;
	    FCB->LocalAddress->Address[0].AddressType =
		ConnectReq->RemoteAddress.Address[0].AddressType;
	    FCB->LocalAddress->Address[0].AddressLength =
		ConnectReq->RemoteAddress.Address[0].AddressLength;

	    Status = WarmSocketForBind( FCB );

	    if( NT_SUCCESS(Status) )
		FCB->State = SOCKET_STATE_BOUND;
	    else
		return UnlockAndMaybeComplete( FCB, Status, Irp, 0, NULL );
	} else
	    return UnlockAndMaybeComplete
		( FCB, STATUS_NO_MEMORY, Irp, 0, NULL );
    } /* Drop through to SOCKET_STATE_BOUND */

    case SOCKET_STATE_BOUND:
	FCB->RemoteAddress =
	    TaCopyTransportAddress( &ConnectReq->RemoteAddress );

	if( FCB->Flags & AFD_ENDPOINT_CONNECTIONLESS )
	{
	    Status = STATUS_SUCCESS;
	    break;
	}

	Status = WarmSocketForConnection( FCB );

	if( !NT_SUCCESS(Status) )
	    break;

	FCB->State = SOCKET_STATE_CONNECTING;

	TdiBuildConnectionInfo
	    ( &TargetAddress,
	      &ConnectReq->RemoteAddress );

	if( TargetAddress ) {
	    Status = TdiConnect( &FCB->ConnectIrp.InFlightRequest,
				 FCB->Connection.Object,
				 TargetAddress,
				 &FCB->ConnectIrp.Iosb,
				 StreamSocketConnectComplete,
				 FCB );

	    ExFreePool( TargetAddress );

	    AFD_DbgPrint(MID_TRACE,("Queueing IRP %x\n", Irp));

	    if( Status == STATUS_PENDING )
		return LeaveIrpUntilLater( FCB, Irp, FUNCTION_CONNECT );
	} else Status = STATUS_NO_MEMORY;
	break;

    default:
	AFD_DbgPrint(MID_TRACE,("Inappropriate socket state %d for connect\n",
				FCB->State));
	break;
    }

    return UnlockAndMaybeComplete( FCB, Status, Irp, 0, NULL );
}
