/* $Id: bind.c,v 1.3 2004/07/18 22:53:59 arty Exp $
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/net/afd/afd/bind.c
 * PURPOSE:          Ancillary functions driver
 * PROGRAMMER:       Art Yerkes (ayerkes@speakeasy.net)
 * UPDATE HISTORY:
 * 20040708 Created
 */

#include "afd.h"
#include "tdi_proto.h"
#include "tdiconn.h"
#include "debug.h"

NTSTATUS WarmSocketForBind( PAFD_FCB FCB ) {
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    AFD_DbgPrint(MID_TRACE,("Called\n"));

    if( FCB->LocalAddress ) {
	Status = TdiOpenAddressFile
	    ( &FCB->TdiDeviceName,
	      FCB->LocalAddress,
	      &FCB->AddressFile.Handle, 
	      &FCB->AddressFile.Object );
    }

    if( !NT_SUCCESS(Status) ) {
	TdiCloseDevice( &FCB->AddressFile.Handle,
			FCB->AddressFile.Object );
	RtlZeroMemory( &FCB->AddressFile, sizeof( FCB->AddressFile ) );
    }

    AFD_DbgPrint(MID_TRACE,("Returning %x\n", Status));

    return Status;
}

NTSTATUS STDCALL
AfdBindSocket(PDEVICE_OBJECT DeviceObject, PIRP Irp,
	      PIO_STACK_LOCATION IrpSp) {
    NTSTATUS Status = STATUS_SUCCESS;
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    PAFD_FCB FCB = FileObject->FsContext;
    PAFD_BIND_DATA BindReq;
    
    AFD_DbgPrint(MID_TRACE,("Called\n"));
    
    if( !SocketAcquireStateLock( FCB ) ) return LostSocket( Irp, FALSE );
    if( !(BindReq = LockRequest( Irp, IrpSp )) ) 
	return UnlockAndMaybeComplete( FCB, STATUS_NO_MEMORY, 
				       Irp, 0, NULL, FALSE );
    
    FCB->LocalAddress = TaCopyTransportAddress( &BindReq->Address );

    if( FCB->LocalAddress ) 
	Status = WarmSocketForBind( FCB );
    else Status = STATUS_NO_MEMORY;

    if( NT_SUCCESS(Status) ) 
	FCB->State = SOCKET_STATE_BOUND;

    if( FCB->Flags & SGID_CONNECTIONLESS ) {
	/* This will be the from address for subsequent recvfrom calls */
	TdiBuildConnectionInfo( &FCB->AddressFrom,
				&FCB->LocalAddress->Address[0] );
	/* Allocate our backup buffer */
	FCB->Recv.Window = ExAllocatePool( NonPagedPool, FCB->Recv.Size );
	FCB->PollState |= AFD_EVENT_SEND; 
	/* A datagram socket is always sendable */
	
	Status = TdiReceiveDatagram
	    ( &FCB->ReceiveIrp.InFlightRequest,
	      FCB->AddressFile.Object,
	      0,
	      FCB->Recv.Window,
	      FCB->Recv.Size,
	      FCB->AddressFrom,
	      &FCB->ReceiveIrp.Iosb,
	      PacketSocketRecvComplete,
	      FCB );
    }

    return UnlockAndMaybeComplete( FCB, Status, Irp, 0, NULL, TRUE );
}

