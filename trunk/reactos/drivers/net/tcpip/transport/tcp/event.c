/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        transport/tcp/event.c
 * PURPOSE:     Transmission Control Protocol -- Events from oskittcp
 * PROGRAMMERS: Art Yerkes
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */

#include "precomp.h"

extern ULONG TCP_IPIdentification;
extern LIST_ENTRY SleepingThreadsList;
extern FAST_MUTEX SleepingThreadsLock;

int TCPSocketState(void *ClientData,
		   void *WhichSocket, 
		   void *WhichConnection,
		   OSK_UINT NewState ) {
    PCONNECTION_ENDPOINT Connection = WhichConnection;
    PTCP_COMPLETION_ROUTINE Complete;
    PTDI_BUCKET Bucket;
    PLIST_ENTRY Entry;

    TI_DbgPrint(MID_TRACE,("Called: NewState %x\n", NewState));

    if( !Connection ) {
	TI_DbgPrint(MID_TRACE,("Socket closing.\n"));
	return 0;
    }

    if( (NewState & SEL_CONNECT) && 
	!(Connection->State & SEL_CONNECT) ) {
	while( !IsListEmpty( &Connection->ConnectRequest ) ) {
	    Connection->State |= SEL_CONNECT;
	    Entry = RemoveHeadList( &Connection->ConnectRequest );
	    Bucket = CONTAINING_RECORD( Entry, TDI_BUCKET, Entry );
	    Complete = Bucket->Request.RequestNotifyObject;
	    TI_DbgPrint(MID_TRACE,
			("Completing Connect Request %x\n", Bucket->Request));
	    Complete( Bucket->Request.RequestContext, STATUS_SUCCESS, 0 );
	    /* Frees the bucket allocated in TCPConnect */
	    ExFreePool( Bucket );
	}
    } else if( (NewState & SEL_READ) || (NewState & SEL_FIN) ) {
	while( !IsListEmpty( &Connection->ReceiveRequest ) ) {
	    PIRP Irp;
	    OSK_UINT RecvLen = 0, Received = 0;
	    OSK_PCHAR RecvBuffer = 0;
	    PMDL Mdl;
	    NTSTATUS Status;

	    Entry = RemoveHeadList( &Connection->ReceiveRequest );
	    Bucket = CONTAINING_RECORD( Entry, TDI_BUCKET, Entry );
	    Complete = Bucket->Request.RequestNotifyObject;

	    TI_DbgPrint(MID_TRACE,
			("Readable, Completing read request %x\n", 
			 Bucket->Request));

	    Irp = Bucket->Request.RequestContext;
	    Mdl = Irp->MdlAddress;

	    TI_DbgPrint(MID_TRACE,
			("Getting the user buffer from %x\n", Mdl));

	    NdisQueryBuffer( Mdl, &RecvBuffer, &RecvLen );

	    TI_DbgPrint(MID_TRACE,
			("Reading %d bytes to %x\n", RecvLen, RecvBuffer));

	    if( NewState & SEL_FIN ) {
		Status = STATUS_END_OF_FILE;
		Received = 0;
	    } else 
		Status = TCPTranslateError
		    ( OskitTCPRecv( Connection->SocketContext,
				    RecvBuffer,
				    RecvLen,
				    &Received,
				    0 ) );

	    TI_DbgPrint(MID_TRACE,("TCP Bytes: %d\n", Received));

	    if( Status == STATUS_SUCCESS && Received != 0 ) {
		TI_DbgPrint(MID_TRACE,("Received %d bytes with status %x\n",
				       Received, Status));
		
		TI_DbgPrint(MID_TRACE,
			    ("Completing Receive Request: %x\n", 
			     Bucket->Request));
		
		Complete( Bucket->Request.RequestContext, 
			  STATUS_SUCCESS, 
			  Received );
	    } else {
		InsertHeadList( &Connection->ReceiveRequest,
				&Bucket->Entry );
	    }
	}
    } 

    return 0;
}

void TCPPacketSendComplete( PVOID Context,
			    PNDIS_PACKET NdisPacket,
			    NDIS_STATUS NdisStatus ) {
    TI_DbgPrint(MID_TRACE,("called\n"));
    /* FreeNdisPacket( NdisPacket ); */
}

#define STRINGIFY(x) #x

int TCPPacketSend(void *ClientData, OSK_PCHAR data, OSK_UINT len ) {
    NTSTATUS Status;
    KIRQL OldIrql;
    NDIS_STATUS NdisStatus;
    ROUTE_CACHE_NODE *RCN;
    IP_PACKET Packet = { 0 };
    IP_ADDRESS RemoteAddress, LocalAddress;
    PIPv4_HEADER Header;

    TI_DbgPrint(MID_TRACE,("TCP OUTPUT (%x:%d):\n", data, len));
    OskitDumpBuffer( data, len );

    if( *data == 0x45 ) { /* IPv4 */
	Header = (PIPv4_HEADER)data;
	LocalAddress.Type = IP_ADDRESS_V4;
	LocalAddress.Address.IPv4Address = Header->SrcAddr;
	RemoteAddress.Type = IP_ADDRESS_V4;
	RemoteAddress.Address.IPv4Address = Header->DstAddr;
    } else {
	DbgPrint("Don't currently handle IPv6\n");
	KeBugCheck(4);
    }

    RemoteAddress.Type = LocalAddress.Type = IP_ADDRESS_V4;

    DbgPrint("OSKIT SENDING PACKET *** %x -> %x\n",
	     LocalAddress.Address.IPv4Address,
	     RemoteAddress.Address.IPv4Address);
    
    ASSERT( (LocalAddress.Address.IPv4Address & 0xc0000000) != 0xc0000000 );
	

    Status = RouteGetRouteToDestination( &RemoteAddress,
					 NULL,
					 &RCN );
    
    if( !NT_SUCCESS(Status) || !RCN ) return OSK_EADDRNOTAVAIL;

    KeRaiseIrql( DISPATCH_LEVEL, &OldIrql );

    NdisStatus = 
	AllocatePacketWithBuffer( &Packet.NdisPacket, data, len );
    
    if (NdisStatus != NDIS_STATUS_SUCCESS) {
	TI_DbgPrint(MAX_TRACE, ("Error from NDIS: %08x\n", NdisStatus));
	goto end;
    }

    AdjustPacket( Packet.NdisPacket, 0, MaxLLHeaderSize );
    GetDataPtr( Packet.NdisPacket, 0, (PCHAR *)&Packet.Header, &Packet.ContigSize );
    TI_DbgPrint(MAX_TRACE,("PC(Packet.NdisPacket) is %s (%x)\n", STRINGIFY(PC(Packet.NdisPacket)), PC(Packet.NdisPacket)));
    PC(Packet.NdisPacket)->Complete = TCPPacketSendComplete;
    OskitDumpBuffer((PCHAR)(PC(Packet.NdisPacket)),sizeof(*(PC(Packet.NdisPacket))));

    Packet.HeaderSize = sizeof(IPv4_HEADER);
    Packet.TotalSize = len;
    Packet.SrcAddr = LocalAddress;
    Packet.DstAddr = RemoteAddress;

    IPSendFragment( Packet.NdisPacket, RCN->NCE );

end:
    KeLowerIrql( OldIrql );

    if( !NT_SUCCESS(NdisStatus) ) return OSK_EINVAL;
    else return 0;
}

void *TCPMalloc( void *ClientData,
		 OSK_UINT Bytes, OSK_PCHAR File, OSK_UINT Line ) {
    void *v = ExAllocatePool( NonPagedPool, Bytes );
    if( v ) TrackWithTag( FOURCC('f','b','s','d'), v, File, Line );
    return v;
}

void TCPFree( void *ClientData,
	      void *data, OSK_PCHAR File, OSK_UINT Line ) {
    UntrackFL( File, Line, data );
    ExFreePool( data );
}

int TCPSleep( void *ClientData, void *token, int priority, char *msg,
	      int tmio ) {
    PSLEEPING_THREAD SleepingThread;
    
    TI_DbgPrint(MID_TRACE,
		("Called TSLEEP: tok = %x, pri = %d, wmesg = %s, tmio = %x\n",
		 token, priority, msg, tmio));

    SleepingThread = ExAllocatePool( NonPagedPool, sizeof( *SleepingThread ) );
    if( SleepingThread ) {
	KeInitializeEvent( &SleepingThread->Event, NotificationEvent, FALSE );
	SleepingThread->SleepToken = token;

	ExAcquireFastMutex( &SleepingThreadsLock );
	InsertTailList( &SleepingThreadsList, &SleepingThread->Entry );
	ExReleaseFastMutex( &SleepingThreadsLock );

	TI_DbgPrint(MID_TRACE,("Waiting on %x\n", token));
	KeWaitForSingleObject( &SleepingThread->Event,
			       WrSuspended,
			       KernelMode,
			       TRUE,
			       NULL );

	ExAcquireFastMutex( &SleepingThreadsLock );
	RemoveEntryList( &SleepingThread->Entry );
	ExReleaseFastMutex( &SleepingThreadsLock );

	ExFreePool( SleepingThread );
    }
    TI_DbgPrint(MID_TRACE,("Waiting finished: %x\n", token));
    return 0;
}

void TCPWakeup( void *ClientData, void *token ) {
    PLIST_ENTRY Entry;
    PSLEEPING_THREAD SleepingThread;

    ExAcquireFastMutex( &SleepingThreadsLock );
    Entry = SleepingThreadsList.Flink;
    while( Entry != &SleepingThreadsList ) {
	SleepingThread = CONTAINING_RECORD(Entry, SLEEPING_THREAD, Entry);
	TI_DbgPrint(MID_TRACE,("Sleeper @ %x\n", SleepingThread));
	if( SleepingThread->SleepToken == token ) {
	    TI_DbgPrint(MID_TRACE,("Setting event to wake %x\n", token));
	    KeSetEvent( &SleepingThread->Event, IO_NETWORK_INCREMENT, FALSE );
	}
	Entry = Entry->Flink;
    }
    ExReleaseFastMutex( &SleepingThreadsLock );
}
