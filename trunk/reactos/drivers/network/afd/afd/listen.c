/* $Id$
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/net/afd/afd/listen.c
 * PURPOSE:          Ancillary functions driver
 * PROGRAMMER:       Art Yerkes (ayerkes@speakeasy.net)
 * UPDATE HISTORY:
 * 20040708 Created
 */
#include "afd.h"
#include "tdi_proto.h"
#include "tdiconn.h"
#include "debug.h"

static NTSTATUS SatisfyAccept( PAFD_DEVICE_EXTENSION DeviceExt,
                           PIRP Irp,
                           PFILE_OBJECT NewFileObject,
		                   PAFD_TDI_OBJECT_QELT Qelt ) {
    PAFD_FCB FCB = NewFileObject->FsContext;
    NTSTATUS Status;

    if( !SocketAcquireStateLock( FCB ) )
        return LostSocket( Irp );

    /* Transfer the connection to the new socket, launch the opening read */
    AFD_DbgPrint(MID_TRACE,("Completing a real accept (FCB %x)\n", FCB));

    FCB->Connection = Qelt->Object;

    if( FCB->RemoteAddress ) ExFreePool( FCB->RemoteAddress );
    FCB->RemoteAddress =
	TaCopyTransportAddress( Qelt->ConnInfo->RemoteAddress );

    if( !FCB->RemoteAddress ) 
	Status = STATUS_NO_MEMORY;
    else
	Status = MakeSocketIntoConnection( FCB );

    return UnlockAndMaybeComplete( FCB, Status, Irp, 0 );
}

static NTSTATUS SatisfyPreAccept( PIRP Irp, PAFD_TDI_OBJECT_QELT Qelt ) {
    PAFD_RECEIVED_ACCEPT_DATA ListenReceive =
	(PAFD_RECEIVED_ACCEPT_DATA)Irp->AssociatedIrp.SystemBuffer;
    PTA_IP_ADDRESS IPAddr;

    ListenReceive->SequenceNumber = Qelt->Seq;

    AFD_DbgPrint(MID_TRACE,("Giving SEQ %d to userland\n", Qelt->Seq));
    AFD_DbgPrint(MID_TRACE,("Socket Address (K) %x (U) %x\n",
                            &ListenReceive->Address,
                            Qelt->ConnInfo->RemoteAddress));

    TaCopyTransportAddressInPlace( &ListenReceive->Address,
				   Qelt->ConnInfo->RemoteAddress );

    IPAddr = (PTA_IP_ADDRESS)&ListenReceive->Address;

    if( !IPAddr ) {
	if( Irp->MdlAddress ) UnlockRequest( Irp, IoGetCurrentIrpStackLocation( Irp ) );
	Irp->IoStatus.Status = STATUS_NO_MEMORY;
	Irp->IoStatus.Information = 0;
        (void)IoSetCancelRoutine(Irp, NULL);
	IoCompleteRequest( Irp, IO_NETWORK_INCREMENT );
        return STATUS_NO_MEMORY;
    }

    AFD_DbgPrint(MID_TRACE,("IPAddr->TAAddressCount %d\n",
                            IPAddr->TAAddressCount));
    AFD_DbgPrint(MID_TRACE,("IPAddr->Address[0].AddressType %d\n",
                            IPAddr->Address[0].AddressType));
    AFD_DbgPrint(MID_TRACE,("IPAddr->Address[0].AddressLength %d\n",
                            IPAddr->Address[0].AddressLength));
    AFD_DbgPrint(MID_TRACE,("IPAddr->Address[0].Address[0].sin_port %x\n",
                            IPAddr->Address[0].Address[0].sin_port));
    AFD_DbgPrint(MID_TRACE,("IPAddr->Address[0].Address[0].sin_addr %x\n",
                            IPAddr->Address[0].Address[0].in_addr));

    if( Irp->MdlAddress ) UnlockRequest( Irp, IoGetCurrentIrpStackLocation( Irp ) );

    Irp->IoStatus.Information = ((PCHAR)&IPAddr[1]) - ((PCHAR)ListenReceive);
    Irp->IoStatus.Status = STATUS_SUCCESS;
    (void)IoSetCancelRoutine(Irp, NULL);
    IoCompleteRequest( Irp, IO_NETWORK_INCREMENT );
    return STATUS_SUCCESS;
}

static NTSTATUS NTAPI ListenComplete
( PDEVICE_OBJECT DeviceObject,
  PIRP Irp,
  PVOID Context ) {
    NTSTATUS Status = STATUS_SUCCESS;
    PAFD_FCB FCB = (PAFD_FCB)Context;
    PAFD_TDI_OBJECT_QELT Qelt;
    PLIST_ENTRY NextIrpEntry, QeltEntry;
    PIRP NextIrp;

    if( !SocketAcquireStateLock( FCB ) )
        return STATUS_FILE_CLOSED;

    FCB->ListenIrp.InFlightRequest = NULL;

    if( FCB->State == SOCKET_STATE_CLOSED ) {
        /* Cleanup our IRP queue because the FCB is being destroyed */
        while( !IsListEmpty( &FCB->PendingIrpList[FUNCTION_PREACCEPT] ) ) {
	       NextIrpEntry = RemoveHeadList(&FCB->PendingIrpList[FUNCTION_PREACCEPT]);
	       NextIrp = CONTAINING_RECORD(NextIrpEntry, IRP, Tail.Overlay.ListEntry);
	       NextIrp->IoStatus.Status = STATUS_FILE_CLOSED;
	       NextIrp->IoStatus.Information = 0;
	       if( NextIrp->MdlAddress ) UnlockRequest( NextIrp, IoGetCurrentIrpStackLocation( NextIrp ) );
               (void)IoSetCancelRoutine(NextIrp, NULL);
	       IoCompleteRequest( NextIrp, IO_NETWORK_INCREMENT );
        }

        /* Free all pending connections */
        while( !IsListEmpty( &FCB->PendingConnections ) ) {
               QeltEntry = RemoveHeadList(&FCB->PendingConnections);
               Qelt = CONTAINING_RECORD(QeltEntry, AFD_TDI_OBJECT_QELT, ListEntry);
               ExFreePool(Qelt);
        }

        /* Free ConnectionReturnInfo and ConnectionCallInfo */
        if (FCB->ListenIrp.ConnectionReturnInfo)
        {
            ExFreePool(FCB->ListenIrp.ConnectionReturnInfo);
            FCB->ListenIrp.ConnectionReturnInfo = NULL;
        }

        if (FCB->ListenIrp.ConnectionCallInfo)
        {
            ExFreePool(FCB->ListenIrp.ConnectionCallInfo);
            FCB->ListenIrp.ConnectionCallInfo = NULL;
        }

	SocketStateUnlock( FCB );
	return STATUS_FILE_CLOSED;
    }

    AFD_DbgPrint(MID_TRACE,("Completing listen request.\n"));
    AFD_DbgPrint(MID_TRACE,("IoStatus was %x\n", FCB->ListenIrp.Iosb.Status));

    Qelt = ExAllocatePool( NonPagedPool, sizeof(*Qelt) );
    if( !Qelt ) {
	Status = STATUS_NO_MEMORY;
    } else {
        UINT AddressType =
            FCB->LocalAddress->Address[0].AddressType;

	Qelt->Object = FCB->Connection;
	Qelt->Seq = FCB->ConnSeq++;
        AFD_DbgPrint(MID_TRACE,("Address Type: %d (RA %x)\n",
                                AddressType,
                                FCB->ListenIrp.
                                ConnectionReturnInfo->RemoteAddress));

        TdiBuildNullConnectionInfo( &Qelt->ConnInfo, AddressType );
        if( Qelt->ConnInfo ) {
            TaCopyTransportAddressInPlace
               ( Qelt->ConnInfo->RemoteAddress,
                 FCB->ListenIrp.ConnectionReturnInfo->RemoteAddress );
            InsertTailList( &FCB->PendingConnections, &Qelt->ListEntry );
        } else Status = STATUS_NO_MEMORY;
    }

    /* Satisfy a pre-accept request if one is available */
    if( !IsListEmpty( &FCB->PendingIrpList[FUNCTION_PREACCEPT] ) &&
	!IsListEmpty( &FCB->PendingConnections ) ) {
	PLIST_ENTRY PendingIrp  =
	    RemoveHeadList( &FCB->PendingIrpList[FUNCTION_PREACCEPT] );
	PLIST_ENTRY PendingConn = FCB->PendingConnections.Flink;
	SatisfyPreAccept
	    ( CONTAINING_RECORD( PendingIrp, IRP,
				 Tail.Overlay.ListEntry ),
	      CONTAINING_RECORD( PendingConn, AFD_TDI_OBJECT_QELT,
				 ListEntry ) );
    }

    if( FCB->ListenIrp.ConnectionCallInfo ) {
        ExFreePool( FCB->ListenIrp.ConnectionCallInfo );
        FCB->ListenIrp.ConnectionCallInfo = NULL;
    }

    if( FCB->ListenIrp.ConnectionReturnInfo ) {
        ExFreePool( FCB->ListenIrp.ConnectionReturnInfo );
        FCB->ListenIrp.ConnectionReturnInfo = NULL;
    }

    FCB->NeedsNewListen = TRUE;

    /* Trigger a select return if appropriate */
    if( !IsListEmpty( &FCB->PendingConnections ) ) {
	FCB->PollState |= AFD_EVENT_ACCEPT;
    } else {
	FCB->PollState &= ~AFD_EVENT_ACCEPT;
    }

    PollReeval( FCB->DeviceExt, FCB->FileObject );

    SocketStateUnlock( FCB );

    return Status;
}

NTSTATUS AfdListenSocket(PDEVICE_OBJECT DeviceObject, PIRP Irp,
			 PIO_STACK_LOCATION IrpSp) {
    NTSTATUS Status = STATUS_SUCCESS;
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    PAFD_FCB FCB = FileObject->FsContext;
    PAFD_LISTEN_DATA ListenReq;

    AFD_DbgPrint(MID_TRACE,("Called on %x\n", FCB));

    if( !SocketAcquireStateLock( FCB ) ) return LostSocket( Irp );

    if( !(ListenReq = LockRequest( Irp, IrpSp )) )
	return UnlockAndMaybeComplete( FCB, STATUS_NO_MEMORY, Irp,
				       0 );

    if( FCB->State != SOCKET_STATE_BOUND ) {
	Status = STATUS_INVALID_PARAMETER;
	AFD_DbgPrint(MID_TRACE,("Could not listen an unbound socket\n"));
	return UnlockAndMaybeComplete( FCB, Status, Irp, 0 );
    }

    FCB->DelayedAccept = ListenReq->UseDelayedAcceptance;

    AFD_DbgPrint(MID_TRACE,("ADDRESSFILE: %x\n", FCB->AddressFile.Handle));

    Status = WarmSocketForConnection( FCB );

    AFD_DbgPrint(MID_TRACE,("Status from warmsocket %x\n", Status));

    if( !NT_SUCCESS(Status) ) return UnlockAndMaybeComplete( FCB, Status, Irp, 0 );

    TdiBuildNullConnectionInfo
	( &FCB->ListenIrp.ConnectionCallInfo,
	  FCB->LocalAddress->Address[0].AddressType );
    TdiBuildNullConnectionInfo
	( &FCB->ListenIrp.ConnectionReturnInfo,
	  FCB->LocalAddress->Address[0].AddressType );

    if( !FCB->ListenIrp.ConnectionReturnInfo || !FCB->ListenIrp.ConnectionCallInfo )
    {
        if (FCB->ListenIrp.ConnectionReturnInfo)
        {
            ExFreePool(FCB->ListenIrp.ConnectionReturnInfo);
            FCB->ListenIrp.ConnectionReturnInfo = NULL;
        }

        if (FCB->ListenIrp.ConnectionCallInfo)
        {
            ExFreePool(FCB->ListenIrp.ConnectionCallInfo);
            FCB->ListenIrp.ConnectionCallInfo = NULL;
        }

	return UnlockAndMaybeComplete( FCB, STATUS_NO_MEMORY, Irp, 0 );
    }

    FCB->State = SOCKET_STATE_LISTENING;

    Status = TdiListen( &FCB->ListenIrp.InFlightRequest,
			FCB->Connection.Object,
			&FCB->ListenIrp.ConnectionCallInfo,
			&FCB->ListenIrp.ConnectionReturnInfo,
			&FCB->ListenIrp.Iosb,
			ListenComplete,
			FCB );

    if( Status == STATUS_PENDING )
	Status = STATUS_SUCCESS;

    if (NT_SUCCESS(Status))
        FCB->NeedsNewListen = FALSE;

    AFD_DbgPrint(MID_TRACE,("Returning %x\n", Status));
    return UnlockAndMaybeComplete( FCB, Status, Irp, 0 );
}

NTSTATUS AfdWaitForListen( PDEVICE_OBJECT DeviceObject, PIRP Irp,
			   PIO_STACK_LOCATION IrpSp ) {
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    PAFD_FCB FCB = FileObject->FsContext;
    NTSTATUS Status;

    AFD_DbgPrint(MID_TRACE,("Called\n"));

    if( !SocketAcquireStateLock( FCB ) ) return LostSocket( Irp );

    if( !IsListEmpty( &FCB->PendingConnections ) ) {
	PLIST_ENTRY PendingConn = FCB->PendingConnections.Flink;

	/* We have a pending connection ... complete this irp right away */
	Status = SatisfyPreAccept
	    ( Irp,
	      CONTAINING_RECORD
	      ( PendingConn, AFD_TDI_OBJECT_QELT, ListEntry ) );

	AFD_DbgPrint(MID_TRACE,("Completed a wait for accept\n"));

        if ( IsListEmpty( &FCB->PendingConnections ) )
             FCB->PollState &= ~AFD_EVENT_ACCEPT;
        else
             FCB->PollState |= AFD_EVENT_ACCEPT;

        PollReeval( FCB->DeviceExt, FCB->FileObject );

	SocketStateUnlock( FCB );
	return Status;
    } else {
	AFD_DbgPrint(MID_TRACE,("Holding\n"));

	return LeaveIrpUntilLater( FCB, Irp, FUNCTION_PREACCEPT );
    }
}

NTSTATUS AfdAccept( PDEVICE_OBJECT DeviceObject, PIRP Irp,
		    PIO_STACK_LOCATION IrpSp ) {
    NTSTATUS Status = STATUS_SUCCESS;
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    PAFD_DEVICE_EXTENSION DeviceExt =
        (PAFD_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    PAFD_FCB FCB = FileObject->FsContext;
    PAFD_ACCEPT_DATA AcceptData = Irp->AssociatedIrp.SystemBuffer;
    PLIST_ENTRY PendingConn;

    AFD_DbgPrint(MID_TRACE,("Called\n"));

    if( !SocketAcquireStateLock( FCB ) ) return LostSocket( Irp );

    if( FCB->NeedsNewListen ) {
	AFD_DbgPrint(MID_TRACE,("ADDRESSFILE: %x\n", FCB->AddressFile.Handle));

	/* Launch new accept socket */
	Status = WarmSocketForConnection( FCB );

	if( Status == STATUS_SUCCESS ) {
            TdiBuildNullConnectionInfo
	      ( &FCB->ListenIrp.ConnectionCallInfo,
	        FCB->LocalAddress->Address[0].AddressType );
            TdiBuildNullConnectionInfo
	      ( &FCB->ListenIrp.ConnectionReturnInfo,
                FCB->LocalAddress->Address[0].AddressType );

            if( !FCB->ListenIrp.ConnectionReturnInfo || !FCB->ListenIrp.ConnectionCallInfo )
            {
                if (FCB->ListenIrp.ConnectionReturnInfo)
                {
                    ExFreePool(FCB->ListenIrp.ConnectionReturnInfo);
                    FCB->ListenIrp.ConnectionReturnInfo = NULL;
                }

                if (FCB->ListenIrp.ConnectionCallInfo)
                {
                    ExFreePool(FCB->ListenIrp.ConnectionCallInfo);
                    FCB->ListenIrp.ConnectionCallInfo = NULL;
                }

	        return UnlockAndMaybeComplete( FCB, STATUS_NO_MEMORY, Irp, 0 );
            }

	    Status = TdiListen( &FCB->ListenIrp.InFlightRequest,
				FCB->Connection.Object,
				&FCB->ListenIrp.ConnectionCallInfo,
				&FCB->ListenIrp.ConnectionReturnInfo,
				&FCB->ListenIrp.Iosb,
				ListenComplete,
				FCB );

            if( Status == STATUS_PENDING )
                Status = STATUS_SUCCESS;

            if( !NT_SUCCESS(Status) )
                return UnlockAndMaybeComplete( FCB, Status, Irp, 0 );

	    FCB->NeedsNewListen = FALSE;
	} else return UnlockAndMaybeComplete( FCB, Status, Irp, 0 );
    }

    for( PendingConn = FCB->PendingConnections.Flink;
	 PendingConn != &FCB->PendingConnections;
	 PendingConn = PendingConn->Flink ) {
	PAFD_TDI_OBJECT_QELT PendingConnObj =
	    CONTAINING_RECORD( PendingConn, AFD_TDI_OBJECT_QELT, ListEntry );

	AFD_DbgPrint(MID_TRACE,("Comparing Seq %d to Q %d\n",
				AcceptData->SequenceNumber,
				PendingConnObj->Seq));

	if( PendingConnObj->Seq == AcceptData->SequenceNumber ) {
	    PFILE_OBJECT NewFileObject = NULL;

	    RemoveEntryList( PendingConn );

	    Status = ObReferenceObjectByHandle
		( (HANDLE)AcceptData->ListenHandle,
		  FILE_ALL_ACCESS,
		  NULL,
		  KernelMode,
		  (PVOID *)&NewFileObject,
		  NULL );

            if( !NT_SUCCESS(Status) ) return UnlockAndMaybeComplete( FCB, Status, Irp, 0 );

            ASSERT(NewFileObject != FileObject);
            ASSERT(NewFileObject->FsContext != FCB);

	    /* We have a pending connection ... complete this irp right away */
	    Status = SatisfyAccept( DeviceExt, Irp, NewFileObject, PendingConnObj );

	    ObDereferenceObject( NewFileObject );

	    AFD_DbgPrint(MID_TRACE,("Completed a wait for accept\n"));

	    ExFreePool( PendingConnObj );

	    if( IsListEmpty( &FCB->PendingConnections ) ) {
		FCB->PollState &= ~AFD_EVENT_ACCEPT;
	    } else {
		FCB->PollState |= AFD_EVENT_ACCEPT;
            }

	    PollReeval( FCB->DeviceExt, FCB->FileObject );

	    SocketStateUnlock( FCB );
	    return Status;
	}
    }

    return UnlockAndMaybeComplete( FCB, STATUS_UNSUCCESSFUL, Irp, 0 );
}
