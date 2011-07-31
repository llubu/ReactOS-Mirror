/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        transport/tcp/tcp.c
 * PURPOSE:     Transmission Control Protocol
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 *              Art Yerkes (arty@users.sf.net)
 * REVISIONS:
 *   CSH 01/08-2000  Created
 *   arty 12/21/2004 Added accept
 */

#include "precomp.h"

LONG TCP_IPIdentification = 0;
static BOOLEAN TCPInitialized = FALSE;
static NPAGED_LOOKASIDE_LIST TCPSegmentList;
PORT_SET TCPPorts;
CLIENT_DATA ClientInfo;

VOID
CompleteBucketWorker(PVOID Context)
{
    PTDI_BUCKET Bucket = Context;
    PCONNECTION_ENDPOINT Connection;
    PTCP_COMPLETION_ROUTINE Complete;
    
    ASSERT(Bucket);
    
    Connection = Bucket->AssociatedEndpoint;
    Complete = Bucket->Request.RequestNotifyObject;
    
    Complete(Bucket->Request.RequestContext, Bucket->Status, Bucket->Information);
    
    ExFreePoolWithTag(Bucket, TDI_BUCKET_TAG);
    
    DereferenceObject(Connection);
}

VOID
CompleteBucket(PCONNECTION_ENDPOINT Connection, PTDI_BUCKET Bucket)
{
    Bucket->AssociatedEndpoint = Connection;
    ReferenceObject(Connection);
    ChewCreate(CompleteBucketWorker, Bucket);
}

VOID HandleSignalledConnection(PCONNECTION_ENDPOINT Connection)
{
        PTDI_BUCKET Bucket;
        PLIST_ENTRY Entry;
        NTSTATUS Status;
        PIRP Irp;
        PMDL Mdl;

        if (ClientInfo.Unlocked)
            LockObjectAtDpcLevel(Connection);

        TI_DbgPrint(MID_TRACE,("Handling signalled state on %x\n",
                               Connection));

        /* Things that can happen when we try the initial connection */
        if( Connection->SignalState & (SEL_CONNECT | SEL_FIN | SEL_ERROR) ) {
            while (!IsListEmpty(&Connection->ConnectRequest)) {
               Entry = RemoveHeadList( &Connection->ConnectRequest );

               Bucket = CONTAINING_RECORD( Entry, TDI_BUCKET, Entry );

               if (Connection->SignalState & SEL_ERROR)
               {
                   Bucket->Status = TCPTranslateError(OskitTCPGetSocketError(Connection));
               }
               else if (Connection->SignalState & SEL_FIN)
               {
                   Bucket->Status = STATUS_CANCELLED;
               }
               else
               {
                   Bucket->Status = STATUS_SUCCESS;
               }
               Bucket->Information = 0;

               CompleteBucket(Connection, Bucket);
           }
       }

       if( Connection->SignalState & (SEL_ACCEPT | SEL_FIN | SEL_ERROR) ) {
           /* Handle readable on a listening socket --
            * TODO: Implement filtering
            */
           TI_DbgPrint(DEBUG_TCP,("Accepting new connection on %x (Queue: %s)\n",
                                  Connection,
                                  IsListEmpty(&Connection->ListenRequest) ?
                                  "empty" : "nonempty"));

           while (!IsListEmpty(&Connection->ListenRequest)) {
               PIO_STACK_LOCATION IrpSp;

               Entry = RemoveHeadList( &Connection->ListenRequest );

               Bucket = CONTAINING_RECORD( Entry, TDI_BUCKET, Entry );

               Irp = Bucket->Request.RequestContext;
               IrpSp = IoGetCurrentIrpStackLocation( Irp );

               TI_DbgPrint(DEBUG_TCP,("Getting the socket\n"));

               if (Connection->SignalState & SEL_ERROR)
               {
                   Status = TCPTranslateError(OskitTCPGetSocketError(Connection));
               }
               else if (Connection->SignalState & SEL_FIN)
               {
                   Status = STATUS_CANCELLED;
               }
               else
               {
                   Status = TCPServiceListeningSocket(Connection->AddressFile->Listener,
                                                      Bucket->AssociatedEndpoint,
                                                      (PTDI_REQUEST_KERNEL)&IrpSp->Parameters);
               }

               TI_DbgPrint(DEBUG_TCP,("Socket: Status: %x\n"));

               if( Status == STATUS_PENDING ) {
                   InsertHeadList( &Connection->ListenRequest, &Bucket->Entry );
                   break;
               } else {
                   Bucket->Status = Status;
                   Bucket->Information = 0;
                   DereferenceObject(Bucket->AssociatedEndpoint);

                   CompleteBucket(Connection, Bucket);
               }
          }
      }

      /* Things that happen after we're connected */
      if( Connection->SignalState & (SEL_READ | SEL_FIN | SEL_ERROR) ) {
          TI_DbgPrint(DEBUG_TCP,("Readable: irp list %s\n",
                                 IsListEmpty(&Connection->ReceiveRequest) ?
                                 "empty" : "nonempty"));

           while (!IsListEmpty(&Connection->ReceiveRequest)) {
               OSK_UINT RecvLen = 0, Received = 0;
               PVOID RecvBuffer = 0;

               Entry = RemoveHeadList( &Connection->ReceiveRequest );

               Bucket = CONTAINING_RECORD( Entry, TDI_BUCKET, Entry );

               Irp = Bucket->Request.RequestContext;
               Mdl = Irp->MdlAddress;

               TI_DbgPrint(DEBUG_TCP,
                           ("Getting the user buffer from %x\n", Mdl));

               NdisQueryBuffer( Mdl, &RecvBuffer, &RecvLen );

               TI_DbgPrint(DEBUG_TCP,
                           ("Reading %d bytes to %x\n", RecvLen, RecvBuffer));

               TI_DbgPrint(DEBUG_TCP, ("Connection: %x\n", Connection));
               TI_DbgPrint(DEBUG_TCP, ("RecvBuffer: %x\n", RecvBuffer));

               if (Connection->SignalState & SEL_ERROR)
               {
                   Status = TCPTranslateError(OskitTCPGetSocketError(Connection));
               }
               else if (Connection->SignalState & SEL_FIN)
               {
                   /* We got here because of a SEL_FIN event */
                   Status = STATUS_CANCELLED;
               }
               else
               {
                   Status = TCPTranslateError(OskitTCPRecv(Connection,
                                                           RecvBuffer,
                                                           RecvLen,
                                                           &Received,
                                                           0));
               }

               TI_DbgPrint(DEBUG_TCP,("TCP Bytes: %d\n", Received));

               if( Status == STATUS_PENDING ) {
                   InsertHeadList( &Connection->ReceiveRequest, &Bucket->Entry );
                   break;
               } else {
                   TI_DbgPrint(DEBUG_TCP,
                               ("Completing Receive request: %x %x\n",
                                Bucket->Request, Status));

                   Bucket->Status = Status;
                   Bucket->Information = (Bucket->Status == STATUS_SUCCESS) ? Received : 0;

                   CompleteBucket(Connection, Bucket);
               }
           }
       }
       if( Connection->SignalState & (SEL_WRITE | SEL_FIN | SEL_ERROR) ) {
           TI_DbgPrint(DEBUG_TCP,("Writeable: irp list %s\n",
                                  IsListEmpty(&Connection->SendRequest) ?
                                  "empty" : "nonempty"));

           while (!IsListEmpty(&Connection->SendRequest)) {
               OSK_UINT SendLen = 0, Sent = 0;
               PVOID SendBuffer = 0;

               Entry = RemoveHeadList( &Connection->SendRequest );

               Bucket = CONTAINING_RECORD( Entry, TDI_BUCKET, Entry );

               Irp = Bucket->Request.RequestContext;
               Mdl = Irp->MdlAddress;

               TI_DbgPrint(DEBUG_TCP,
                           ("Getting the user buffer from %x\n", Mdl));

               NdisQueryBuffer( Mdl, &SendBuffer, &SendLen );

               TI_DbgPrint(DEBUG_TCP,
                           ("Writing %d bytes to %x\n", SendLen, SendBuffer));

               TI_DbgPrint(DEBUG_TCP, ("Connection: %x\n", Connection));

               if (Connection->SignalState & SEL_ERROR)
               {
                   Status = TCPTranslateError(OskitTCPGetSocketError(Connection));
               }
               else if (Connection->SignalState & SEL_FIN)
               {
                   /* We got here because of a SEL_FIN event */
                   Status = STATUS_CANCELLED;
               }
               else
               {
                   Status = TCPTranslateError(OskitTCPSend(Connection,
                                                           SendBuffer,
                                                           SendLen,
                                                           &Sent,
                                                           0));
               }

               TI_DbgPrint(DEBUG_TCP,("TCP Bytes: %d\n", Sent));

               if( Status == STATUS_PENDING ) {
                   InsertHeadList( &Connection->SendRequest, &Bucket->Entry );
                   break;
               } else {
                   TI_DbgPrint(DEBUG_TCP,
                               ("Completing Send request: %x %x\n",
                               Bucket->Request, Status));

                   Bucket->Status = Status;
                   Bucket->Information = (Bucket->Status == STATUS_SUCCESS) ? Sent : 0;

                   CompleteBucket(Connection, Bucket);
               }
           }
       }
       if( Connection->SignalState & (SEL_WRITE | SEL_FIN | SEL_ERROR) ) {
          while (!IsListEmpty(&Connection->ShutdownRequest)) {
            Entry = RemoveHeadList( &Connection->ShutdownRequest );
            
            Bucket = CONTAINING_RECORD( Entry, TDI_BUCKET, Entry );

            if (Connection->SignalState & SEL_ERROR)
            {
                Status = TCPTranslateError(OskitTCPGetSocketError(Connection));
            }
            else if (Connection->SignalState & SEL_FIN)
            {
                /* We were cancelled by a FIN */
                Status = STATUS_CANCELLED;
            }
            else
            {
                /* See if we can satisfy this after the events */
                if (IsListEmpty(&Connection->SendRequest))
                {
                    /* Send queue is empty so we're good to go */
                    Status = TCPTranslateError(OskitTCPShutdown(Connection, FWRITE));
                }
                else
                {
                    /* We still have to wait */
                    Status = STATUS_PENDING;
                }
            }
              
            if( Status == STATUS_PENDING ) {
                InsertHeadList( &Connection->ShutdownRequest, &Bucket->Entry );
                break;
            } else {
                TI_DbgPrint(DEBUG_TCP,
                            ("Completing shutdown request: %x %x\n",
                             Bucket->Request, Status));
                
                if (KeCancelTimer(&Connection->DisconnectTimer))
                {
                    DereferenceObject(Connection);
                }

                Bucket->Status = Status;
                Bucket->Information = 0;
                
                CompleteBucket(Connection, Bucket);
            }
          }
       }
    
       if (Connection->SignalState & SEL_FIN)
       {
           Connection->SocketContext = NULL;
           DereferenceObject(Connection);
       }

       if (ClientInfo.Unlocked)
           UnlockObjectFromDpcLevel(Connection);
}

VOID NTAPI
DisconnectTimeoutDpc(PKDPC Dpc,
                     PVOID DeferredContext,
                     PVOID SystemArgument1,
                     PVOID SystemArgument2)
{
    PCONNECTION_ENDPOINT Connection = DeferredContext;
    PLIST_ENTRY Entry;
    PTDI_BUCKET Bucket;
    
    LockObjectAtDpcLevel(Connection);
    
    /* We timed out waiting for pending sends so force it to shutdown */
    OskitTCPShutdown(Connection, FWRITE);
    
    while (!IsListEmpty(&Connection->SendRequest))
    {
        Entry = RemoveHeadList(&Connection->SendRequest);
        
        Bucket = CONTAINING_RECORD(Entry, TDI_BUCKET, Entry);
        
        Bucket->Information = 0;
        Bucket->Status = STATUS_FILE_CLOSED;
        
        CompleteBucket(Connection, Bucket);
    }
    
    while (!IsListEmpty(&Connection->ShutdownRequest)) {
        Entry = RemoveHeadList( &Connection->ShutdownRequest );
        
        Bucket = CONTAINING_RECORD( Entry, TDI_BUCKET, Entry );
        
        Bucket->Status = STATUS_TIMEOUT;
        Bucket->Information = 0;
        
        CompleteBucket(Connection, Bucket);
    }
    
    UnlockObjectFromDpcLevel(Connection);
    
    DereferenceObject(Connection);
}

VOID ConnectionFree(PVOID Object) {
    PCONNECTION_ENDPOINT Connection = Object;
    KIRQL OldIrql;

    TI_DbgPrint(DEBUG_TCP, ("Freeing TCP Endpoint\n"));

    TcpipAcquireSpinLock(&ConnectionEndpointListLock, &OldIrql);
    RemoveEntryList(&Connection->ListEntry);
    TcpipReleaseSpinLock(&ConnectionEndpointListLock, OldIrql);

    ExFreePoolWithTag( Connection, CONN_ENDPT_TAG );
}

PCONNECTION_ENDPOINT TCPAllocateConnectionEndpoint( PVOID ClientContext ) {
    PCONNECTION_ENDPOINT Connection =
        ExAllocatePoolWithTag(NonPagedPool, sizeof(CONNECTION_ENDPOINT),
                              CONN_ENDPT_TAG);
    if (!Connection)
        return Connection;

    TI_DbgPrint(DEBUG_CPOINT, ("Connection point file object allocated at (0x%X).\n", Connection));

    RtlZeroMemory(Connection, sizeof(CONNECTION_ENDPOINT));

    /* Initialize spin lock that protects the connection endpoint file object */
    KeInitializeSpinLock(&Connection->Lock);
    InitializeListHead(&Connection->ConnectRequest);
    InitializeListHead(&Connection->ListenRequest);
    InitializeListHead(&Connection->ReceiveRequest);
    InitializeListHead(&Connection->SendRequest);
    InitializeListHead(&Connection->ShutdownRequest);
    
    KeInitializeTimer(&Connection->DisconnectTimer);
    KeInitializeDpc(&Connection->DisconnectDpc, DisconnectTimeoutDpc, Connection);

    /* Save client context pointer */
    Connection->ClientContext = ClientContext;

    Connection->RefCount = 2;
    Connection->Free = ConnectionFree;

    /* Add connection endpoint to global list */
    ExInterlockedInsertTailList(&ConnectionEndpointListHead,
                                &Connection->ListEntry,
                                &ConnectionEndpointListLock);

    return Connection;
}

NTSTATUS TCPSocket( PCONNECTION_ENDPOINT Connection,
                    UINT Family, UINT Type, UINT Proto ) {
    NTSTATUS Status;
    KIRQL OldIrql;

    LockObject(Connection, &OldIrql);

    TI_DbgPrint(DEBUG_TCP,("Called: Connection %x, Family %d, Type %d, "
                           "Proto %d\n",
                           Connection, Family, Type, Proto));

    Status = TCPTranslateError( OskitTCPSocket( Connection,
                                                &Connection->SocketContext,
                                                Family,
                                                Type,
                                                Proto ) );

    ASSERT_KM_POINTER(Connection->SocketContext);

    UnlockObject(Connection, OldIrql);

    return Status;
}

VOID TCPReceive(PIP_INTERFACE Interface, PIP_PACKET IPPacket)
/*
 * FUNCTION: Receives and queues TCP data
 * ARGUMENTS:
 *     IPPacket = Pointer to an IP packet that was received
 * NOTES:
 *     This is the low level interface for receiving TCP data
 */
{
    KIRQL OldIrql;

    TI_DbgPrint(DEBUG_TCP,("Sending packet %d (%d) to oskit\n",
                           IPPacket->TotalSize,
                           IPPacket->HeaderSize));

    KeAcquireSpinLock(&ClientInfo.Lock, &OldIrql);
    ClientInfo.Unlocked = TRUE;
    ClientInfo.OldIrql = OldIrql;

    OskitTCPReceiveDatagram( IPPacket->Header,
                             IPPacket->TotalSize,
                             IPPacket->HeaderSize );

    ClientInfo.Unlocked = FALSE;
    KeReleaseSpinLock(&ClientInfo.Lock, OldIrql);
}

/* event.c */
int TCPSocketState( void *ClientData,
                    void *WhichSocket,
                    void *WhichConnection,
                    OSK_UINT NewState );

int TCPPacketSend( void *ClientData,
                   OSK_PCHAR Data,
                   OSK_UINT Len );

POSK_IFADDR TCPFindInterface( void *ClientData,
                              OSK_UINT AddrType,
                              OSK_UINT FindType,
                              OSK_SOCKADDR *ReqAddr );

NTSTATUS TCPMemStartup( void );
void *TCPMalloc( void *ClientData,
                 OSK_UINT bytes, OSK_PCHAR file, OSK_UINT line );
void TCPFree( void *ClientData,
              void *data, OSK_PCHAR file, OSK_UINT line );
void TCPMemShutdown( void );

OSKITTCP_EVENT_HANDLERS EventHandlers = {
    NULL,             /* Client Data */
    TCPSocketState,   /* SocketState */
    TCPPacketSend,    /* PacketSend */
    TCPFindInterface, /* FindInterface */
    TCPMalloc,        /* Malloc */
    TCPFree,          /* Free */
    NULL,             /* Sleep */
    NULL,             /* Wakeup */
};

static KEVENT TimerLoopEvent;
static HANDLE TimerThreadHandle;

/*
 * We are running 2 timers here, one with a 200ms interval (fast) and the other
 * with a 500ms interval (slow). So we need to time out at 200, 400, 500, 600,
 * 800, 1000 and process the "fast" events at 200, 400, 600, 800, 1000 and the
 * "slow" events at 500 and 1000.
 */
static VOID NTAPI
TimerThread(PVOID Context)
{
    LARGE_INTEGER Timeout;
    NTSTATUS Status;
    unsigned Current, NextFast, NextSlow, Next;

    Current = 0;
    Next = 0;
    NextFast = 0;
    NextSlow = 0;
    while ( 1 ) {
        if (Next == NextFast) {
            NextFast += 2;
       }
        if (Next == NextSlow) {
            NextSlow += 5;
        }
        Next = min(NextFast, NextSlow);
        Timeout.QuadPart = (LONGLONG) (Next - Current) * -1000000; /* 100 ms */
        Status = KeWaitForSingleObject(&TimerLoopEvent, Executive, KernelMode,
                                       FALSE, &Timeout);
        if (Status != STATUS_TIMEOUT) {
            PsTerminateSystemThread(Status);
        }

        TimerOskitTCP( Next == NextFast, Next == NextSlow );

        Current = Next;
        if (10 <= Current) {
            Current = 0;
            Next = 0;
            NextFast = 0;
            NextSlow = 0;
        }
    }
}

static VOID
StartTimer(VOID)
{
    KeInitializeEvent(&TimerLoopEvent, NotificationEvent, FALSE);
    PsCreateSystemThread(&TimerThreadHandle, THREAD_ALL_ACCESS, 0, 0, 0,
                         TimerThread, NULL);
}

NTSTATUS TCPStartup(VOID)
/*
 * FUNCTION: Initializes the TCP subsystem
 * RETURNS:
 *     Status of operation
 */
{
    NTSTATUS Status;

    Status = TCPMemStartup();
    if ( ! NT_SUCCESS(Status) ) {
        return Status;
    }

    Status = PortsStartup( &TCPPorts, 1, 0xfffe );
    if( !NT_SUCCESS(Status) ) {
        TCPMemShutdown();
        return Status;
    }

    KeInitializeSpinLock(&ClientInfo.Lock);
    ClientInfo.Unlocked = FALSE;

    RegisterOskitTCPEventHandlers( &EventHandlers );
    InitOskitTCP();

    /* Register this protocol with IP layer */
    IPRegisterProtocol(IPPROTO_TCP, TCPReceive);

    ExInitializeNPagedLookasideList(
        &TCPSegmentList,                /* Lookaside list */
        NULL,                           /* Allocate routine */
        NULL,                           /* Free routine */
        0,                              /* Flags */
        sizeof(TCP_SEGMENT),            /* Size of each entry */
        'SPCT',                         /* Tag */
        0);                             /* Depth */

    StartTimer();

    TCPInitialized = TRUE;

    return STATUS_SUCCESS;
}


NTSTATUS TCPShutdown(VOID)
/*
 * FUNCTION: Shuts down the TCP subsystem
 * RETURNS:
 *     Status of operation
 */
{
    LARGE_INTEGER WaitForThread;

    if (!TCPInitialized)
        return STATUS_SUCCESS;

    WaitForThread.QuadPart = -2500000; /* 250 ms */
    KeSetEvent(&TimerLoopEvent, IO_NO_INCREMENT, FALSE);
    ZwWaitForSingleObject(TimerThreadHandle, FALSE, &WaitForThread);

    /* Deregister this protocol with IP layer */
    IPRegisterProtocol(IPPROTO_TCP, NULL);

    ExDeleteNPagedLookasideList(&TCPSegmentList);

    TCPInitialized = FALSE;

    DeinitOskitTCP();

    PortsShutdown( &TCPPorts );

    TCPMemShutdown();

    return STATUS_SUCCESS;
}

NTSTATUS TCPTranslateError( int OskitError ) {
    NTSTATUS Status;

    switch( OskitError ) {
    case 0: Status = STATUS_SUCCESS; break;
    case OSK_EADDRNOTAVAIL:
        Status = STATUS_INVALID_ADDRESS;
        DbgPrint("OskitTCP: EADDRNOTAVAIL\n");
        break;
    case OSK_EADDRINUSE:
        Status = STATUS_ADDRESS_ALREADY_EXISTS;
        DbgPrint("OskitTCP: EADDRINUSE\n");
        break;
    case OSK_EAFNOSUPPORT:
        Status = STATUS_INVALID_CONNECTION;
        DbgPrint("OskitTCP: EAFNOSUPPORT\n");
        break;
    case OSK_ECONNREFUSED:
        Status = STATUS_REMOTE_NOT_LISTENING;
        DbgPrint("OskitTCP: ECONNREFUSED\n");
        break;
    case OSK_ECONNRESET:
        Status = STATUS_REMOTE_DISCONNECT;
        DbgPrint("OskitTCP: ECONNRESET\n");
        break;
    case OSK_ECONNABORTED:
        Status = STATUS_LOCAL_DISCONNECT;
        DbgPrint("OskitTCP: ECONNABORTED\n");
        break;
    case OSK_EWOULDBLOCK:
    case OSK_EINPROGRESS: Status = STATUS_PENDING; break;
    case OSK_EINVAL:
        Status = STATUS_INVALID_PARAMETER;
        DbgPrint("OskitTCP: EINVAL\n");
        break;
    case OSK_ENOMEM:
    case OSK_ENOBUFS:
        Status = STATUS_INSUFFICIENT_RESOURCES;
        DbgPrint("OskitTCP: ENOMEM/ENOBUFS\n");
        break;
    case OSK_EPIPE:
    case OSK_ESHUTDOWN:
        Status = STATUS_FILE_CLOSED;
        DbgPrint("OskitTCP: ESHUTDOWN/EPIPE\n");
        break;
    case OSK_EMSGSIZE:
        Status = STATUS_BUFFER_TOO_SMALL;
        DbgPrint("OskitTCP: EMSGSIZE\n");
        break;
    case OSK_ETIMEDOUT:
        Status = STATUS_TIMEOUT;
        DbgPrint("OskitTCP: ETIMEDOUT\n");
        break;
    case OSK_ENETUNREACH:
        Status = STATUS_NETWORK_UNREACHABLE;
        DbgPrint("OskitTCP: ENETUNREACH\n");
        break;
    case OSK_EFAULT:
        Status = STATUS_ACCESS_VIOLATION;
        DbgPrint("OskitTCP: EFAULT\n");
        break;
    default:
       DbgPrint("OskitTCP returned unhandled error code: %d\n", OskitError);
       Status = STATUS_INVALID_CONNECTION;
       break;
    }

    TI_DbgPrint(DEBUG_TCP,("Error %d -> %x\n", OskitError, Status));
    return Status;
}

NTSTATUS TCPConnect
( PCONNECTION_ENDPOINT Connection,
  PTDI_CONNECTION_INFORMATION ConnInfo,
  PTDI_CONNECTION_INFORMATION ReturnInfo,
  PTCP_COMPLETION_ROUTINE Complete,
  PVOID Context ) {
    NTSTATUS Status;
    SOCKADDR_IN AddressToConnect = { 0 }, AddressToBind = { 0 };
    IP_ADDRESS RemoteAddress;
    USHORT RemotePort;
    TA_IP_ADDRESS LocalAddress;
    PTDI_BUCKET Bucket;
    PNEIGHBOR_CACHE_ENTRY NCE = NULL;
    KIRQL OldIrql;

    TI_DbgPrint(DEBUG_TCP,("TCPConnect: Called\n"));

    Status = AddrBuildAddress
        ((PTRANSPORT_ADDRESS)ConnInfo->RemoteAddress,
         &RemoteAddress,
         &RemotePort);

    if (!NT_SUCCESS(Status)) {
        TI_DbgPrint(DEBUG_TCP, ("Could not AddrBuildAddress in TCPConnect\n"));
        return Status;
    }

    /* Freed in TCPSocketState */
    TI_DbgPrint(DEBUG_TCP,
                ("Connecting to address %x:%x\n",
                 RemoteAddress.Address.IPv4Address,
                 RemotePort));

    AddressToConnect.sin_family = AF_INET;
    AddressToBind = AddressToConnect;

    LockObject(Connection, &OldIrql);

    if (!Connection->AddressFile)
    {
        UnlockObject(Connection, OldIrql);
        return STATUS_INVALID_PARAMETER;
    }

    if (AddrIsUnspecified(&Connection->AddressFile->Address))
    {
        if (!(NCE = RouteGetRouteToDestination(&RemoteAddress)))
        {
            UnlockObject(Connection, OldIrql);
            return STATUS_NETWORK_UNREACHABLE;
        }
    }

    if (Connection->AddressFile->Port)
    {
        /* See if we had an unspecified bind address */
        if (NCE)
        {
            /* We did, so use the interface unicast address associated with the route */
            AddressToBind.sin_addr.s_addr = NCE->Interface->Unicast.Address.IPv4Address;
        }
        else
        {
            /* Bind address was explicit so use it */
            AddressToBind.sin_addr.s_addr = Connection->AddressFile->Address.Address.IPv4Address;
        }
        
        AddressToBind.sin_port = Connection->AddressFile->Port;
        
        /* Perform an explicit bind */
        Status = TCPTranslateError(OskitTCPBind(Connection,
                                                &AddressToBind,
                                                sizeof(AddressToBind)));
        
        DbgPrint("Connect - Explicit bind on port %d returned 0x%x\n", Connection->AddressFile->Port, Status);
    }
    else
    {
        /* An implicit bind will be performed */
        Status = STATUS_SUCCESS;
    }

    if (NT_SUCCESS(Status)) {        
        if (NT_SUCCESS(Status))
        {
            memcpy( &AddressToConnect.sin_addr,
                   &RemoteAddress.Address.IPv4Address,
                   sizeof(AddressToConnect.sin_addr) );
            AddressToConnect.sin_port = RemotePort;
            
            Status = TCPTranslateError
            ( OskitTCPConnect( Connection,
                              &AddressToConnect,
                              sizeof(AddressToConnect) ) );

            if (NT_SUCCESS(Status))
            {
                /* Check if we had an unspecified port */
                if (!Connection->AddressFile->Port)
                {
                    /* We did, so we need to copy back the port */
                    if (NT_SUCCESS(TCPGetSockAddress(Connection, (PTRANSPORT_ADDRESS)&LocalAddress, FALSE)))
                    {
                        /* Allocate the port in the port bitmap */
                        Connection->AddressFile->Port = TCPAllocatePort(LocalAddress.Address[0].Address[0].sin_port);
                        DbgPrint("Connect - Implicit bind on port %d\n", Connection->AddressFile->Port);

                        /* This should never fail */
                        ASSERT(Connection->AddressFile->Port != 0xFFFF);
                    }
                }
                
                /* Check if the address was unspecified */
                if (AddrIsUnspecified(&Connection->AddressFile->Address))
                {
                    /* It is, so store the address of the outgoing NIC */
                    Connection->AddressFile->Address = NCE->Interface->Unicast;
                }
            }

            if (Status == STATUS_PENDING)
            {
                Bucket = ExAllocatePoolWithTag( NonPagedPool, sizeof(*Bucket), TDI_BUCKET_TAG );
                if( !Bucket )
                {
                    UnlockObject(Connection, OldIrql);
                    return STATUS_NO_MEMORY;
                }
                
                Bucket->Request.RequestNotifyObject = (PVOID)Complete;
                Bucket->Request.RequestContext = Context;
                
                InsertTailList( &Connection->ConnectRequest, &Bucket->Entry );
            }
        }
    }

    UnlockObject(Connection, OldIrql);

    return Status;
}

NTSTATUS TCPDisconnect
( PCONNECTION_ENDPOINT Connection,
  UINT Flags,
  PLARGE_INTEGER Timeout,
  PTDI_CONNECTION_INFORMATION ConnInfo,
  PTDI_CONNECTION_INFORMATION ReturnInfo,
  PTCP_COMPLETION_ROUTINE Complete,
  PVOID Context ) {
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    PTDI_BUCKET Bucket;
    KIRQL OldIrql;
    PLIST_ENTRY Entry;
    LARGE_INTEGER ActualTimeout;

    TI_DbgPrint(DEBUG_TCP,("started\n"));

    LockObject(Connection, &OldIrql);

    if (Flags & TDI_DISCONNECT_RELEASE)
    {
        /* See if we can satisfy this right now */
        if (IsListEmpty(&Connection->SendRequest))
        {
            /* Send queue is empty so we're good to go */
            Status = TCPTranslateError(OskitTCPShutdown(Connection, FWRITE));
            
            UnlockObject(Connection, OldIrql);
            
            return Status;
        }
        
        /* Check if the timeout was 0 */
        if (Timeout && Timeout->QuadPart == 0)
        {
            OskitTCPShutdown(Connection, FWRITE);
            
            while (!IsListEmpty(&Connection->SendRequest))
            {
                Entry = RemoveHeadList(&Connection->SendRequest);
                
                Bucket = CONTAINING_RECORD(Entry, TDI_BUCKET, Entry);
                
                Bucket->Information = 0;
                Bucket->Status = STATUS_FILE_CLOSED;
                
                CompleteBucket(Connection, Bucket);
            }
            
            UnlockObject(Connection, OldIrql);
            
            return STATUS_TIMEOUT;
        }
        
        /* Otherwise we wait for the send queue to be empty */
    }

    if ((Flags & TDI_DISCONNECT_ABORT) || !Flags)
    {
        /* This request overrides any pending graceful disconnects */
        while (!IsListEmpty(&Connection->ShutdownRequest))
        {
            Entry = RemoveHeadList(&Connection->ShutdownRequest);
            
            Bucket = CONTAINING_RECORD(Entry, TDI_BUCKET, Entry);
            
            Bucket->Information = 0;
            Bucket->Status = STATUS_FILE_CLOSED;
            
            CompleteBucket(Connection, Bucket);
        }
        
        /* Also kill any pending reads and writes */
        while (!IsListEmpty(&Connection->SendRequest))
        {
            Entry = RemoveHeadList(&Connection->SendRequest);
            
            Bucket = CONTAINING_RECORD(Entry, TDI_BUCKET, Entry);
            
            Bucket->Information = 0;
            Bucket->Status = STATUS_FILE_CLOSED;
            
            CompleteBucket(Connection, Bucket);
        }
        
        while (!IsListEmpty(&Connection->ReceiveRequest))
        {
            Entry = RemoveHeadList(&Connection->ReceiveRequest);
            
            Bucket = CONTAINING_RECORD(Entry, TDI_BUCKET, Entry);
            
            Bucket->Information = 0;
            Bucket->Status = STATUS_FILE_CLOSED;
            
            CompleteBucket(Connection, Bucket);
        }
        
        /* An abort never pends; we just drop everything and complete */
        Status = TCPTranslateError(OskitTCPShutdown(Connection, FWRITE | FREAD));
            
        UnlockObject(Connection, OldIrql);
            
        return Status;
    }
    
    /* We couldn't complete the request now because we need to wait for outstanding I/O */
    Bucket = ExAllocatePoolWithTag(NonPagedPool, sizeof(*Bucket), TDI_BUCKET_TAG);
    if (!Bucket)
    {
        UnlockObject(Connection, OldIrql);
        return STATUS_NO_MEMORY;
    }
    
    Bucket->Request.RequestNotifyObject = (PVOID)Complete;
    Bucket->Request.RequestContext = Context;

    InsertTailList(&Connection->ShutdownRequest, &Bucket->Entry);
    
    /* Use the timeout specified or 1 second if none was specified */
    if (Timeout)
    {
        ActualTimeout = *Timeout;
    }
    else
    {
        ActualTimeout.QuadPart = -1000000;
    }
    
    ReferenceObject(Connection);
    KeSetTimer(&Connection->DisconnectTimer, ActualTimeout, &Connection->DisconnectDpc);

    UnlockObject(Connection, OldIrql);

    TI_DbgPrint(DEBUG_TCP,("finished %x\n", Status));

    return STATUS_PENDING;
}

NTSTATUS TCPClose
( PCONNECTION_ENDPOINT Connection )
{
    KIRQL OldIrql;

    LockObject(Connection, &OldIrql);

    /* We should not be associated to an address file at this point */
    ASSERT(!Connection->AddressFile);

    OskitTCPClose(Connection);

    Connection->SocketContext = NULL;

    UnlockObject(Connection, OldIrql);

    DereferenceObject(Connection);

    return STATUS_SUCCESS;
}

NTSTATUS TCPReceiveData
( PCONNECTION_ENDPOINT Connection,
  PNDIS_BUFFER Buffer,
  ULONG ReceiveLength,
  PULONG BytesReceived,
  ULONG ReceiveFlags,
  PTCP_COMPLETION_ROUTINE Complete,
  PVOID Context ) {
    PVOID DataBuffer;
    UINT DataLen, Received = 0;
    NTSTATUS Status;
    PTDI_BUCKET Bucket;
    KIRQL OldIrql;

    NdisQueryBuffer( Buffer, &DataBuffer, &DataLen );

    TI_DbgPrint(DEBUG_TCP,("TCP>|< Got an MDL %x (%x:%d)\n", Buffer, DataBuffer, DataLen));

    LockObject(Connection, &OldIrql);

    Status = TCPTranslateError
        ( OskitTCPRecv
          ( Connection,
            DataBuffer,
            DataLen,
            &Received,
            ReceiveFlags ) );

    TI_DbgPrint(DEBUG_TCP,("OskitTCPReceive: %x, %d\n", Status, Received));

    /* Keep this request around ... there was no data yet */
    if( Status == STATUS_PENDING ) {
        /* Freed in TCPSocketState */
        Bucket = ExAllocatePoolWithTag( NonPagedPool, sizeof(*Bucket), TDI_BUCKET_TAG );
        if( !Bucket ) {
            TI_DbgPrint(DEBUG_TCP,("Failed to allocate bucket\n"));
            UnlockObject(Connection, OldIrql);
            return STATUS_NO_MEMORY;
        }

        Bucket->Request.RequestNotifyObject = Complete;
        Bucket->Request.RequestContext = Context;
        *BytesReceived = 0;

        InsertTailList( &Connection->ReceiveRequest, &Bucket->Entry );
        TI_DbgPrint(DEBUG_TCP,("Queued read irp\n"));
    } else {
        TI_DbgPrint(DEBUG_TCP,("Got status %x, bytes %d\n", Status, Received));
        *BytesReceived = Received;
    }

    UnlockObject(Connection, OldIrql);

    TI_DbgPrint(DEBUG_TCP,("Status %x\n", Status));

    return Status;
}

NTSTATUS TCPSendData
( PCONNECTION_ENDPOINT Connection,
  PCHAR BufferData,
  ULONG SendLength,
  PULONG BytesSent,
  ULONG Flags,
  PTCP_COMPLETION_ROUTINE Complete,
  PVOID Context ) {
    UINT Sent = 0;
    NTSTATUS Status;
    PTDI_BUCKET Bucket;
    KIRQL OldIrql;

    LockObject(Connection, &OldIrql);

    TI_DbgPrint(DEBUG_TCP,("Connection = %x\n", Connection));

    Status = TCPTranslateError
        ( OskitTCPSend( Connection,
                        (OSK_PCHAR)BufferData, SendLength,
                        &Sent, 0 ) );

    TI_DbgPrint(DEBUG_TCP,("OskitTCPSend: %x, %d\n", Status, Sent));

    /* Keep this request around ... there was no data yet */
    if( Status == STATUS_PENDING ) {
        /* Freed in TCPSocketState */
        Bucket = ExAllocatePoolWithTag( NonPagedPool, sizeof(*Bucket), TDI_BUCKET_TAG );
        if( !Bucket ) {
            UnlockObject(Connection, OldIrql);
            TI_DbgPrint(DEBUG_TCP,("Failed to allocate bucket\n"));
            return STATUS_NO_MEMORY;
        }
        
        Bucket->Request.RequestNotifyObject = Complete;
        Bucket->Request.RequestContext = Context;
        *BytesSent = 0;
        
        InsertTailList( &Connection->SendRequest, &Bucket->Entry );
        TI_DbgPrint(DEBUG_TCP,("Queued write irp\n"));
    } else {
        TI_DbgPrint(DEBUG_TCP,("Got status %x, bytes %d\n", Status, Sent));
        *BytesSent = Sent;
    }

    UnlockObject(Connection, OldIrql);

    TI_DbgPrint(DEBUG_TCP,("Status %x\n", Status));

    return Status;
}

UINT TCPAllocatePort( UINT HintPort ) {
    if( HintPort ) {
        if( AllocatePort( &TCPPorts, HintPort ) ) return HintPort;
        else {
            TI_DbgPrint
                (MID_TRACE,("We got a hint port but couldn't allocate it\n"));
            return (UINT)-1;
        }
    } else return AllocatePortFromRange( &TCPPorts, 1024, 5000 );
}

VOID TCPFreePort( UINT Port ) {
    DeallocatePort( &TCPPorts, Port );
}

NTSTATUS TCPGetSockAddress
( PCONNECTION_ENDPOINT Connection,
  PTRANSPORT_ADDRESS Address,
  BOOLEAN GetRemote ) {
    OSK_UINT LocalAddress, RemoteAddress;
    OSK_UI16 LocalPort, RemotePort;
    PTA_IP_ADDRESS AddressIP = (PTA_IP_ADDRESS)Address;
    NTSTATUS Status;
    KIRQL OldIrql;

    LockObject(Connection, &OldIrql);

    Status = TCPTranslateError(OskitTCPGetAddress(Connection,
                                                  &LocalAddress, &LocalPort,
                                                  &RemoteAddress, &RemotePort));

    UnlockObject(Connection, OldIrql);

    if (!NT_SUCCESS(Status))
        return Status;

    AddressIP->TAAddressCount = 1;
    AddressIP->Address[0].AddressLength = TDI_ADDRESS_LENGTH_IP;
    AddressIP->Address[0].AddressType = TDI_ADDRESS_TYPE_IP;
    AddressIP->Address[0].Address[0].sin_port = GetRemote ? RemotePort : LocalPort;
    AddressIP->Address[0].Address[0].in_addr = GetRemote ? RemoteAddress : LocalAddress;

    return Status;
}

BOOLEAN TCPRemoveIRP( PCONNECTION_ENDPOINT Endpoint, PIRP Irp ) {
    PLIST_ENTRY Entry;
    PLIST_ENTRY ListHead[5];
    KIRQL OldIrql;
    PTDI_BUCKET Bucket;
    UINT i = 0;
    BOOLEAN Found = FALSE;

    ListHead[0] = &Endpoint->SendRequest;
    ListHead[1] = &Endpoint->ReceiveRequest;
    ListHead[2] = &Endpoint->ConnectRequest;
    ListHead[3] = &Endpoint->ListenRequest;
    ListHead[4] = &Endpoint->ShutdownRequest;

    LockObject(Endpoint, &OldIrql);

    for( i = 0; i < 5; i++ )
    {
        for( Entry = ListHead[i]->Flink;
             Entry != ListHead[i];
             Entry = Entry->Flink )
        {
            Bucket = CONTAINING_RECORD( Entry, TDI_BUCKET, Entry );
            if( Bucket->Request.RequestContext == Irp )
            {
                RemoveEntryList( &Bucket->Entry );
                ExFreePoolWithTag( Bucket, TDI_BUCKET_TAG );
                Found = TRUE;
                break;
            }
        }
    }

    UnlockObject(Endpoint, OldIrql);

    return Found;
}

/* EOF */
