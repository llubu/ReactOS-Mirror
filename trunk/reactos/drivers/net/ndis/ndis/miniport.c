/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NDIS library
 * FILE:        ndis/miniport.c
 * PURPOSE:     Routines used by NDIS miniport drivers
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */
//#define DBG
#include <miniport.h>
#include <protocol.h>
#ifdef DBG
#include <buffer.h>
#endif /* DBG */

#ifdef DBG
/* See debug.h for debug/trace constants */
//ULONG DebugTraceLevel = MIN_TRACE;
//ULONG DebugTraceLevel = -1
#endif /* DBG */

#define SERVICES_ROOT L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\"
#define DEVICE_ROOT L"\\Device\\"
#define ROUTE_DATA_SIZE 256	/* adjust as necessary */


/* Number of media we know */
#define MEDIA_ARRAY_SIZE    15

static NDIS_MEDIUM MediaArray[MEDIA_ARRAY_SIZE] = {
    NdisMedium802_3,
    NdisMedium802_5,
    NdisMediumFddi,
    NdisMediumWan,
    NdisMediumLocalTalk,
    NdisMediumDix,
    NdisMediumArcnetRaw,
    NdisMediumArcnet878_2,
    NdisMediumAtm,
    NdisMediumWirelessWan,
    NdisMediumIrda,
    NdisMediumBpc,
    NdisMediumCoWan,
    NdisMedium1394,
    NdisMediumMax
};


LIST_ENTRY MiniportListHead;
KSPIN_LOCK MiniportListLock;
LIST_ENTRY AdapterListHead;
KSPIN_LOCK AdapterListLock;
LIST_ENTRY OrphanAdapterListHead;
KSPIN_LOCK OrphanAdapterListLock;

#ifdef DBG
VOID
MiniDisplayPacket(
    PNDIS_PACKET Packet)
{
#if 0
    ULONG i, Length;
    UCHAR Buffer[64];
    if ((DebugTraceLevel | DEBUG_PACKET) > 0) {
        Length = CopyPacketToBuffer(
            (PUCHAR)&Buffer,
            Packet,
            0,
            64);

        DbgPrint("*** PACKET START ***");

        for (i = 0; i < Length; i++) {
            if (i % 12 == 0)
                DbgPrint("\n%04X ", i);
            DbgPrint("%02X ", Buffer[i]);
        }

        DbgPrint("*** PACKET STOP ***\n");
    }
#endif
}
#endif /* DBG */


VOID
MiniIndicateData(
    PLOGICAL_ADAPTER    Adapter,
    NDIS_HANDLE         MacReceiveContext,
    PVOID               HeaderBuffer,
    UINT                HeaderBufferSize,
    PVOID               LookaheadBuffer,
    UINT                LookaheadBufferSize,
    UINT                PacketSize)
/*
 * FUNCTION: Indicate received data to bound protocols
 * ARGUMENTS:
 *     Adapter             = Pointer to logical adapter
 *     MacReceiveContext   = MAC receive context handle
 *     HeaderBuffer        = Pointer to header buffer
 *     HeaderBufferSize    = Size of header buffer
 *     LookaheadBuffer     = Pointer to lookahead buffer
 *     LookaheadBufferSize = Size of lookahead buffer
 *     PacketSize          = Total size of received packet
 */
{
    KIRQL OldIrql;
    PLIST_ENTRY CurrentEntry;
    PADAPTER_BINDING AdapterBinding;

    NDIS_DbgPrint(DEBUG_MINIPORT, ("Called. Adapter (0x%X)  HeaderBuffer (0x%X)  "
        "HeaderBufferSize (0x%X)  LookaheadBuffer (0x%X)  LookaheadBufferSize (0x%X).\n",
        Adapter, HeaderBuffer, HeaderBufferSize, LookaheadBuffer, LookaheadBufferSize));

#ifdef DBG
#if 0
    if ((DebugTraceLevel | DEBUG_PACKET) > 0) {
        ULONG i, Length;
        PUCHAR p;

        DbgPrint("*** RECEIVE PACKET START ***\n");
        DbgPrint("HEADER:");
        p = HeaderBuffer;
        for (i = 0; i < HeaderBufferSize; i++) {
            if (i % 16 == 0)
                DbgPrint("\n%04X ", i);
            DbgPrint("%02X ", *p);
            (ULONG_PTR)p += 1;
        }

        DbgPrint("\nFRAME:");

        p = LookaheadBuffer;
        Length = (LookaheadBufferSize < 64)? LookaheadBufferSize : 64;
        for (i = 0; i < Length; i++) {
            if (i % 16 == 0)
                DbgPrint("\n%04X ", i);
            DbgPrint("%02X ", *p);
            (ULONG_PTR)p += 1;
        }

        DbgPrint("\n*** RECEIVE PACKET STOP ***\n");
    }
#endif
#endif /* DBG */

    KeAcquireSpinLock(&Adapter->NdisMiniportBlock.Lock, &OldIrql);
    CurrentEntry = Adapter->ProtocolListHead.Flink;

    if (CurrentEntry == &Adapter->ProtocolListHead) {
        NDIS_DbgPrint(DEBUG_MINIPORT, ("WARNING: No upper protocol layer.\n"));
    }

    while (CurrentEntry != &Adapter->ProtocolListHead) {
	    AdapterBinding = CONTAINING_RECORD(CurrentEntry,
                                           ADAPTER_BINDING,
                                           AdapterListEntry);

        KeReleaseSpinLock(&Adapter->NdisMiniportBlock.Lock, OldIrql);

        (*AdapterBinding->ProtocolBinding->Chars.u4.ReceiveHandler)(
            AdapterBinding->NdisOpenBlock.ProtocolBindingContext,
            MacReceiveContext,
            HeaderBuffer,
            HeaderBufferSize,
            LookaheadBuffer,
            LookaheadBufferSize,
            PacketSize);

        KeAcquireSpinLock(&Adapter->NdisMiniportBlock.Lock, &OldIrql);

        CurrentEntry = CurrentEntry->Flink;
    }
    KeReleaseSpinLock(&Adapter->NdisMiniportBlock.Lock, OldIrql);
}


VOID
MiniEthReceiveComplete(
    IN  PETH_FILTER Filter)
/*
 * FUNCTION: Receive indication complete function for Ethernet devices
 * ARGUMENTS:
 *     Filter = Pointer to Ethernet filter
 */
{
    KIRQL OldIrql;
    PLIST_ENTRY CurrentEntry;
    PLOGICAL_ADAPTER Adapter;
    PADAPTER_BINDING AdapterBinding;

    NDIS_DbgPrint(DEBUG_MINIPORT, ("Called.\n"));

    Adapter = (PLOGICAL_ADAPTER)Filter->Miniport;

    KeAcquireSpinLock(&Adapter->NdisMiniportBlock.Lock, &OldIrql);
    CurrentEntry = Adapter->ProtocolListHead.Flink;
    while (CurrentEntry != &Adapter->ProtocolListHead) {
	    AdapterBinding = CONTAINING_RECORD(CurrentEntry,
                                           ADAPTER_BINDING,
                                           AdapterListEntry);

        KeReleaseSpinLock(&Adapter->NdisMiniportBlock.Lock, OldIrql);

        (*AdapterBinding->ProtocolBinding->Chars.ReceiveCompleteHandler)(
            AdapterBinding->NdisOpenBlock.ProtocolBindingContext);

        KeAcquireSpinLock(&Adapter->NdisMiniportBlock.Lock, &OldIrql);

        CurrentEntry = CurrentEntry->Flink;
    }
    KeReleaseSpinLock(&Adapter->NdisMiniportBlock.Lock, OldIrql);
}


VOID
MiniEthReceiveIndication(
    IN  PETH_FILTER Filter,
    IN  NDIS_HANDLE MacReceiveContext,
    IN  PCHAR       Address,
    IN  PVOID       HeaderBuffer,
    IN  UINT        HeaderBufferSize,
    IN  PVOID       LookaheadBuffer,
    IN  UINT        LookaheadBufferSize,
    IN  UINT        PacketSize)
/*
 * FUNCTION: Receive indication function for Ethernet devices
 * ARGUMENTS:
 *     Filter              = Pointer to Ethernet filter
 *     MacReceiveContext   = MAC receive context handle
 *     Address             = Pointer to destination Ethernet address
 *     HeaderBuffer        = Pointer to Ethernet header buffer
 *     HeaderBufferSize    = Size of Ethernet header buffer
 *     LookaheadBuffer     = Pointer to lookahead buffer
 *     LookaheadBufferSize = Size of lookahead buffer
 *     PacketSize          = Total size of received packet
 */
{
    MiniIndicateData((PLOGICAL_ADAPTER)Filter->Miniport,
                     MacReceiveContext,
                     HeaderBuffer,
                     HeaderBufferSize,
                     LookaheadBuffer,
                     LookaheadBufferSize,
                     PacketSize);
}


VOID
MiniResetComplete(
    IN  NDIS_HANDLE MiniportAdapterHandle,
    IN  NDIS_STATUS Status,
    IN  BOOLEAN     AddressingReset)
{
    UNIMPLEMENTED
}


VOID
MiniSendComplete(
    IN  NDIS_HANDLE     MiniportAdapterHandle,
    IN  PNDIS_PACKET    Packet,
    IN  NDIS_STATUS     Status)
/*
 * FUNCTION: Forwards a message to the initiating protocol saying
 *           that a packet was handled
 * ARGUMENTS:
 *     NdisAdapterHandle = Handle input to MiniportInitialize
 *     Packet            = Pointer to NDIS packet that was sent
 *     Status            = Status of send operation
 */
{
    PADAPTER_BINDING AdapterBinding;

    NDIS_DbgPrint(DEBUG_MINIPORT, ("Called.\n"));

    AdapterBinding = (PADAPTER_BINDING)Packet->Reserved[0];

    (*AdapterBinding->ProtocolBinding->Chars.u2.SendCompleteHandler)(
        AdapterBinding->NdisOpenBlock.ProtocolBindingContext,
        Packet,
        Status);
}


VOID
MiniSendResourcesAvailable(
    IN  NDIS_HANDLE MiniportAdapterHandle)
{
    UNIMPLEMENTED
}


VOID
MiniTransferDataComplete(
    IN  NDIS_HANDLE     MiniportAdapterHandle,
    IN  PNDIS_PACKET    Packet,
    IN  NDIS_STATUS     Status,
    IN  UINT            BytesTransferred)
{
    PLOGICAL_ADAPTER Adapter        = (PLOGICAL_ADAPTER)MiniportAdapterHandle;
    PADAPTER_BINDING AdapterBinding = Adapter->MiniportAdapterBinding;

    NDIS_DbgPrint(DEBUG_MINIPORT, ("Called.\n"));

    (*AdapterBinding->ProtocolBinding->Chars.u3.TransferDataCompleteHandler)(
        AdapterBinding->NdisOpenBlock.ProtocolBindingContext,
        Packet,
        Status,
        BytesTransferred);
}


BOOLEAN
MiniAdapterHasAddress(
    PLOGICAL_ADAPTER Adapter,
    PNDIS_PACKET Packet)
/*
 * FUNCTION: Determines wether a packet has the same destination address as an adapter
 * ARGUMENTS:
 *     Adapter = Pointer to logical adapter object
 *     Packet  = Pointer to NDIS packet
 * RETURNS:
 *     TRUE if the destination address is that of the adapter, FALSE if not
 */
{
    UINT Length;
    PUCHAR Start1;
    PUCHAR Start2;
    PNDIS_BUFFER NdisBuffer;
    UINT BufferLength;

    Start1 = (PUCHAR)&Adapter->Address;
    NdisQueryPacket(Packet, NULL, NULL, &NdisBuffer, NULL);
    if (!NdisBuffer) {
        NDIS_DbgPrint(MID_TRACE, ("Packet contains no buffers.\n"));
        return FALSE;
    }

    NdisQueryBuffer(NdisBuffer, (PVOID)&Start2, &BufferLength);

    /* FIXME: Should handle fragmented packets */

    switch (Adapter->NdisMiniportBlock.MediaType) {
    case NdisMedium802_3:
        Length = ETH_LENGTH_OF_ADDRESS;
        /* Destination address is the first field */
        break;

    default:
        NDIS_DbgPrint(MIN_TRACE, ("Adapter has unsupported media type (0x%X).\n",
            Adapter->NdisMiniportBlock.MediaType));
        return FALSE;
    }

    if (BufferLength < Length) {
        NDIS_DbgPrint(MID_TRACE, ("Buffer is too small.\n"));
        return FALSE;
    }

    return (RtlCompareMemory((PVOID)Start1, (PVOID)Start2, Length) == Length);
}


PLOGICAL_ADAPTER
MiniLocateDevice(
    PNDIS_STRING AdapterName)
/*
 * FUNCTION: Returns the logical adapter object for a specific adapter
 * ARGUMENTS:
 *     AdapterName = Pointer to name of adapter
 * RETURNS:
 *     Pointer to logical adapter object, or NULL if none was found.
 *     If found, the adapter is referenced for the caller. The caller
 *     is responsible for dereferencing after use
 */
{
    KIRQL OldIrql;
    PLIST_ENTRY CurrentEntry;
    PLOGICAL_ADAPTER Adapter;

    NDIS_DbgPrint(DEBUG_MINIPORT, ("Called.\n"));

    KeAcquireSpinLock(&AdapterListLock, &OldIrql);
    CurrentEntry = AdapterListHead.Flink;
    while (CurrentEntry != &AdapterListHead) {
	    Adapter = CONTAINING_RECORD(CurrentEntry, LOGICAL_ADAPTER, ListEntry);

        if (RtlCompareUnicodeString(AdapterName, &Adapter->DeviceName, TRUE) == 0) {
            ReferenceObject(Adapter);
            KeReleaseSpinLock(&AdapterListLock, OldIrql);

            NDIS_DbgPrint(DEBUG_MINIPORT, ("Leaving. Adapter found at (0x%X).\n", Adapter));

            return Adapter;
        }

        CurrentEntry = CurrentEntry->Flink;
    }
    KeReleaseSpinLock(&AdapterListLock, OldIrql);

    NDIS_DbgPrint(DEBUG_MINIPORT, ("Leaving (adapter not found).\n"));

    return NULL;
}


NDIS_STATUS
MiniQueryInformation(
    PLOGICAL_ADAPTER    Adapter,
    NDIS_OID            Oid,
    ULONG               Size,
    PULONG              BytesWritten)
/*
 * FUNCTION: Queries a logical adapter for properties
 * ARGUMENTS:
 *     Adapter      = Pointer to the logical adapter object to query
 *     Oid          = Specifies the Object ID to query for
 *     Size         = If non-zero overrides the length in the adapter object
 *     BytesWritten = Address of buffer to place number of bytes written
 * NOTES:
 *     If the specified buffer is too small, a new buffer is allocated,
 *     and the query is attempted again
 * RETURNS:
 *     Status of operation
 */
{
    NDIS_STATUS NdisStatus;
    ULONG BytesNeeded;

    if (Adapter->QueryBufferLength == 0) {
        Adapter->QueryBuffer = ExAllocatePool(NonPagedPool, (Size == 0)? 32 : Size);

        if (!Adapter->QueryBuffer) {
            NDIS_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
            return NDIS_STATUS_RESOURCES;
        }

        Adapter->QueryBufferLength = (Size == 0)? 32 : Size;
    }

    BytesNeeded = (Size == 0)? Adapter->QueryBufferLength : Size;

    NdisStatus = (*Adapter->Miniport->Chars.QueryInformationHandler)(
        Adapter->NdisMiniportBlock.MiniportAdapterContext,
        Oid,
        Adapter->QueryBuffer,
        BytesNeeded,
        BytesWritten,
        &BytesNeeded);

    if ((NT_SUCCESS(NdisStatus)) || (NdisStatus == NDIS_STATUS_PENDING)) {
        NDIS_DbgPrint(DEBUG_MINIPORT, ("Miniport returned status (0x%X).\n", NdisStatus));
        return NdisStatus;
    }

    if (NdisStatus == NDIS_STATUS_INVALID_LENGTH) {
        ExFreePool(Adapter->QueryBuffer);

        Adapter->QueryBufferLength += BytesNeeded;
        Adapter->QueryBuffer = ExAllocatePool(NonPagedPool,
                                              Adapter->QueryBufferLength);

        if (!Adapter->QueryBuffer) {
            NDIS_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
            return NDIS_STATUS_RESOURCES;
        }

        NdisStatus = (*Adapter->Miniport->Chars.QueryInformationHandler)(
            Adapter->NdisMiniportBlock.MiniportAdapterContext,
            Oid,
            Adapter->QueryBuffer,
            Size,
            BytesWritten,
            &BytesNeeded);
    }

    return NdisStatus;
}


NDIS_STATUS
FASTCALL
MiniQueueWorkItem(
    PLOGICAL_ADAPTER    Adapter,
    NDIS_WORK_ITEM_TYPE WorkItemType,
    PVOID               WorkItemContext,
    NDIS_HANDLE         Initiator)
/*
 * FUNCTION: Queues a work item for execution at a later time
 * ARGUMENTS:
 *     Adapter         = Pointer to the logical adapter object to queue work item on
 *     WorkItemType    = Type of work item to queue
 *     WorkItemContext = Pointer to context information for work item
 *     Initiator       = Pointer to ADAPTER_BINDING structure of initiating protocol
 * NOTES:
 *     Adapter lock must be held when called
 * RETURNS:
 *     Status of operation
 */
{
    PNDIS_MINIPORT_WORK_ITEM Item;

    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    if (Adapter->WorkQueueLevel < NDIS_MINIPORT_WORK_QUEUE_SIZE - 1) {
        Item = &Adapter->WorkQueue[Adapter->WorkQueueLevel];
        Adapter->WorkQueueLevel++;
    } else {
        Item = ExAllocatePool(NonPagedPool, sizeof(NDIS_MINIPORT_WORK_ITEM));
        if (Item) {
            /* Set flag so we know that the buffer should be freed
               when work item is dequeued */
            Item->Allocated = TRUE;
        } else {
            NDIS_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
            return NDIS_STATUS_RESOURCES;
        }
    }

    Item->WorkItemType    = WorkItemType;
    Item->WorkItemContext = WorkItemContext;
    Item->Initiator       = Initiator;

    Item->Link.Next = NULL;
    if (!Adapter->WorkQueueHead) {
        Adapter->WorkQueueHead = Item;
    } else {
        Adapter->WorkQueueTail->Link.Next = (PSINGLE_LIST_ENTRY)Item;
    }

    KeInsertQueueDpc(&Adapter->MiniportDpc, NULL, NULL);

    return NDIS_STATUS_SUCCESS;
}


NDIS_STATUS
FASTCALL
MiniDequeueWorkItem(
    PLOGICAL_ADAPTER    Adapter,
    NDIS_WORK_ITEM_TYPE *WorkItemType,
    PVOID               *WorkItemContext,
    NDIS_HANDLE         *Initiator)
/*
 * FUNCTION: Dequeues a work item from the work queue of a logical adapter
 * ARGUMENTS:
 *     Adapter         = Pointer to the logical adapter object to dequeue work item from
 *     WorkItemType    = Address of buffer for work item type
 *     WorkItemContext = Address of buffer for pointer to context information
 *     Initiator       = Address of buffer for initiator of the work (ADAPTER_BINDING)
 * NOTES:
 *     Adapter lock must be held when called
 * RETURNS:
 *     Status of operation
 */
{
    PNDIS_MINIPORT_WORK_ITEM Item;

    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    Item = Adapter->WorkQueueHead;
    if (Item) {
        Adapter->WorkQueueHead = (PNDIS_MINIPORT_WORK_ITEM)Item->Link.Next;
        if (Item == Adapter->WorkQueueTail)
            Adapter->WorkQueueTail = NULL;

        *WorkItemType    = Item->WorkItemType;
        *WorkItemContext = Item->WorkItemContext;
        *Initiator       = Item->Initiator;

        if (Item->Allocated) {
            ExFreePool(Item);
        } else {
            Adapter->WorkQueueLevel--;
#ifdef DBG
            if (Adapter->WorkQueueLevel < 0) {
                NDIS_DbgPrint(MIN_TRACE, ("Adapter->WorkQueueLevel is < 0 (should be >= 0).\n"));
            }
#endif
        }

        return NDIS_STATUS_SUCCESS;
    }

    return NDIS_STATUS_FAILURE;
}


NDIS_STATUS
MiniDoRequest(
    PLOGICAL_ADAPTER Adapter,
    PNDIS_REQUEST NdisRequest)
/*
 * FUNCTION: Sends a request to a miniport
 * ARGUMENTS:
 *     Adapter     = Pointer to logical adapter object
 *     NdisRequest = Pointer to NDIS request structure describing request
 * RETURNS:
 *     Status of operation
 */
{
    Adapter->NdisMiniportBlock.MediaRequest = NdisRequest;

    switch (NdisRequest->RequestType) {
    case NdisRequestQueryInformation:
        return (*Adapter->Miniport->Chars.QueryInformationHandler)(
            Adapter->NdisMiniportBlock.MiniportAdapterContext,
            NdisRequest->DATA.QUERY_INFORMATION.Oid,
            NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer,
            NdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength,
            (PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesWritten,
            (PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesNeeded);
        break;

    case NdisRequestSetInformation:
        return (*Adapter->Miniport->Chars.SetInformationHandler)(
            Adapter->NdisMiniportBlock.MiniportAdapterContext,
            NdisRequest->DATA.SET_INFORMATION.Oid,
            NdisRequest->DATA.SET_INFORMATION.InformationBuffer,
            NdisRequest->DATA.SET_INFORMATION.InformationBufferLength,
            (PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesRead,
            (PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesNeeded);
        break;

    default:
        return NDIS_STATUS_FAILURE;
    }
}


VOID STDCALL MiniportDpc(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2)
/*
 * FUNCTION: Deferred routine to handle serialization
 * ARGUMENTS:
 *     Dpc             = Pointer to DPC object
 *     DeferredContext = Pointer to context information (LOGICAL_ADAPTER)
 *     SystemArgument1 = Unused
 *     SystemArgument2 = Unused
 */
{
    NDIS_STATUS NdisStatus;
    PVOID WorkItemContext;
    NDIS_WORK_ITEM_TYPE WorkItemType;
    PADAPTER_BINDING AdapterBinding;
    PLOGICAL_ADAPTER Adapter = GET_LOGICAL_ADAPTER(DeferredContext);

    NDIS_DbgPrint(DEBUG_MINIPORT, ("Called.\n"));

    NdisStatus = MiniDequeueWorkItem(Adapter,
                                     &WorkItemType,
                                     &WorkItemContext,
                                     (PNDIS_HANDLE)&AdapterBinding);
    if (NdisStatus == NDIS_STATUS_SUCCESS) {
        Adapter->MiniportAdapterBinding = AdapterBinding;
        switch (WorkItemType) {
        case NdisWorkItemSend:
#ifdef DBG
            MiniDisplayPacket((PNDIS_PACKET)WorkItemContext);
#endif
            NdisStatus = (*Adapter->Miniport->Chars.u1.SendHandler)(
                Adapter->NdisMiniportBlock.MiniportAdapterContext,
                (PNDIS_PACKET)WorkItemContext,
                0);
            if (NdisStatus != NDIS_STATUS_PENDING) {
                MiniSendComplete((NDIS_HANDLE)Adapter,
                                 (PNDIS_PACKET)WorkItemContext,
                                 NdisStatus);
            }
            break;

        case NdisWorkItemSendLoopback:
            NdisStatus = ProIndicatePacket(Adapter,
                                           (PNDIS_PACKET)WorkItemContext);
            MiniSendComplete((NDIS_HANDLE)Adapter,
                             (PNDIS_PACKET)WorkItemContext,
                             NdisStatus);
            break;

        case NdisWorkItemReturnPackets:
            break;

        case NdisWorkItemResetRequested:
            break;

        case NdisWorkItemResetInProgress:
            break;

        case NdisWorkItemHalt:
            break;

        case NdisWorkItemMiniportCallback:
            break;

        case NdisWorkItemRequest:
            NdisStatus = MiniDoRequest(Adapter, (PNDIS_REQUEST)WorkItemContext);

            if (NdisStatus == NDIS_STATUS_PENDING)
                break;

            switch (((PNDIS_REQUEST)WorkItemContext)->RequestType) {
            case NdisRequestQueryInformation:
                NdisMQueryInformationComplete((NDIS_HANDLE)Adapter, NdisStatus);
                break;

            case NdisRequestSetInformation:
                NdisMSetInformationComplete((NDIS_HANDLE)Adapter, NdisStatus);
                break;

            default:
                NDIS_DbgPrint(MIN_TRACE, ("Unknown NDIS request type.\n"));
                break;
            }
            break;

        default:
            NDIS_DbgPrint(MIN_TRACE, ("Unknown NDIS work item type (%d).\n", WorkItemType));
            break;
        }
    }
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisMCloseLog(
    IN  NDIS_HANDLE LogHandle)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
NDIS_STATUS
EXPORT
NdisMCreateLog(
    IN  NDIS_HANDLE     MiniportAdapterHandle,
    IN  UINT            Size,
    OUT PNDIS_HANDLE    LogHandle)
{
    UNIMPLEMENTED

	return NDIS_STATUS_FAILURE;
}


/*
 * @implemented
 */
VOID
EXPORT
NdisMDeregisterAdapterShutdownHandler(
    IN  NDIS_HANDLE MiniportHandle)
{
	PLOGICAL_ADAPTER  Adapter = (PLOGICAL_ADAPTER)MiniportHandle;

	if(Adapter->BugcheckContext->ShutdownHandler)
		KeDeregisterBugCheckCallback(Adapter->BugcheckContext->CallbackRecord);
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisMFlushLog(
    IN  NDIS_HANDLE LogHandle)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisMIndicateStatus(
    IN  NDIS_HANDLE MiniportAdapterHandle,
    IN  NDIS_STATUS GeneralStatus,
    IN  PVOID       StatusBuffer,
    IN  UINT        StatusBufferSize)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisMIndicateStatusComplete(
    IN  NDIS_HANDLE MiniportAdapterHandle)
{
    UNIMPLEMENTED
}


/*
 * @implemented
 */
VOID
EXPORT
NdisInitializeWrapper(
    OUT PNDIS_HANDLE    NdisWrapperHandle,
    IN  PVOID           SystemSpecific1,
    IN  PVOID           SystemSpecific2,
    IN  PVOID           SystemSpecific3)
/*
 * FUNCTION: Notifies the NDIS library that a new miniport is initializing
 * ARGUMENTS:
 *     NdisWrapperHandle = Address of buffer to place NDIS wrapper handle
 *     SystemSpecific1   = Pointer to the driver's driver object
 *     SystemSpecific2   = Pointer to the driver's registry path
 *     SystemSpecific3   = Always NULL
 * NOTES:
 *     - SystemSpecific2 goes invalid so we copy it
 *     - Registry key is 0-terminated for safety
 */
{
    PMINIPORT_DRIVER Miniport;
    PUNICODE_STRING RegistryPath;
    WCHAR *RegistryBuffer;

    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    Miniport = ExAllocatePool(NonPagedPool, sizeof(MINIPORT_DRIVER));
    if (!Miniport) {
        NDIS_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
        *NdisWrapperHandle = NULL;
        return;
    }

    RtlZeroMemory(Miniport, sizeof(MINIPORT_DRIVER));

    KeInitializeSpinLock(&Miniport->Lock);

    Miniport->RefCount = 1;

    Miniport->DriverObject = (PDRIVER_OBJECT)SystemSpecific1;

    /* set the miniport's driver registry path */
    RegistryPath = ExAllocatePool(PagedPool, sizeof(UNICODE_STRING));
    if(!RegistryPath)
    {
       NDIS_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
       return;
    }

    RegistryPath->Length = ((PUNICODE_STRING)SystemSpecific2)->Length;
    RegistryPath->MaximumLength = RegistryPath->Length + 1;	/* room for 0-term */

    RegistryBuffer = ExAllocatePool(PagedPool, RegistryPath->Length + sizeof(WCHAR));
    if(!RegistryBuffer)
    {
       NDIS_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
       return;
    }

    RtlCopyMemory(RegistryBuffer, ((PUNICODE_STRING)SystemSpecific2)->Buffer, RegistryPath->Length);
    RegistryBuffer[RegistryPath->Length] = 0;

    RegistryPath->Buffer = RegistryBuffer;
    Miniport->RegistryPath = RegistryPath;

    InitializeListHead(&Miniport->AdapterListHead);

    /* Put miniport in global miniport list */
    ExInterlockedInsertTailList(&MiniportListHead,
                                &Miniport->ListEntry,
                                &MiniportListLock);

    *NdisWrapperHandle = Miniport;
}


/*
 * @implemented
 */
VOID
EXPORT
NdisMQueryInformationComplete(
    IN  NDIS_HANDLE MiniportAdapterHandle,
    IN  NDIS_STATUS Status)
{
    PLOGICAL_ADAPTER Adapter        = GET_LOGICAL_ADAPTER(MiniportAdapterHandle);
    PADAPTER_BINDING AdapterBinding = (PADAPTER_BINDING)Adapter->MiniportAdapterBinding;

    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    (*AdapterBinding->ProtocolBinding->Chars.RequestCompleteHandler)(
        AdapterBinding->NdisOpenBlock.ProtocolBindingContext,
        Adapter->NdisMiniportBlock.MediaRequest,
        Status);
}

VOID NdisIBugcheckCallback(
    IN PVOID   Buffer,
    IN ULONG   Length)
{
	PMINIPORT_BUGCHECK_CONTEXT Context = (PMINIPORT_BUGCHECK_CONTEXT)Buffer;
	ADAPTER_SHUTDOWN_HANDLER sh = (ADAPTER_SHUTDOWN_HANDLER)Context->ShutdownHandler;

	if(sh)
		sh(Context->DriverContext);
} 

/*
 * @implemented
 */
VOID
EXPORT
NdisMRegisterAdapterShutdownHandler(
    IN  NDIS_HANDLE                 MiniportHandle,
    IN  PVOID                       ShutdownContext,
    IN  ADAPTER_SHUTDOWN_HANDLER    ShutdownHandler)
{
	PLOGICAL_ADAPTER            Adapter = (PLOGICAL_ADAPTER)MiniportHandle;
	PMINIPORT_BUGCHECK_CONTEXT  BugcheckContext = Adapter->BugcheckContext;

	if(BugcheckContext->ShutdownHandler)
		return;

	BugcheckContext->ShutdownHandler = ShutdownHandler;
	BugcheckContext->DriverContext = ShutdownContext;

	/* not sure if this needs to be initialized or not... oh well, it's a leak. */
	BugcheckContext->CallbackRecord = ExAllocatePool(NonPagedPool, sizeof(KBUGCHECK_CALLBACK_RECORD));

	KeRegisterBugCheckCallback(BugcheckContext->CallbackRecord, NdisIBugcheckCallback, 
		BugcheckContext, sizeof(BugcheckContext), "Ndis Miniport");
}


NDIS_STATUS
DoQueries(
    PLOGICAL_ADAPTER Adapter,
    NDIS_OID AddressOID)
/*
 * FUNCTION: Queries miniport for information
 * ARGUMENTS:
 *     Adapter    = Pointer to logical adapter
 *     AddressOID = OID to use to query for current address
 * RETURNS:
 *     Status of operation
 */
{
    ULONG BytesWritten;
    NDIS_STATUS NdisStatus;

    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    /* Get MAC options for adapter */
    NdisStatus = MiniQueryInformation(Adapter,
                                      OID_GEN_MAC_OPTIONS,
                                      0,
                                      &BytesWritten);
    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        NDIS_DbgPrint(MIN_TRACE, ("OID_GEN_MAC_OPTIONS failed. NdisStatus (0x%X).\n", NdisStatus));
        return NdisStatus;
    }

    RtlCopyMemory(&Adapter->NdisMiniportBlock.MacOptions, Adapter->QueryBuffer, sizeof(UINT));

    NDIS_DbgPrint(DEBUG_MINIPORT, ("MacOptions (0x%X).\n", Adapter->NdisMiniportBlock.MacOptions));

    /* Get current hardware address of adapter */
    NdisStatus = MiniQueryInformation(Adapter,
                                      AddressOID,
                                      0,
                                      &BytesWritten);
    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        NDIS_DbgPrint(MIN_TRACE, ("Address OID (0x%X) failed. NdisStatus (0x%X).\n",
            AddressOID, NdisStatus));
        return NdisStatus;
    }

    RtlCopyMemory(&Adapter->Address, Adapter->QueryBuffer, Adapter->AddressLength);
#ifdef DBG
    {
        /* 802.3 only */

        PUCHAR A = (PUCHAR)&Adapter->Address.Type.Medium802_3;

        NDIS_DbgPrint(MAX_TRACE, ("Adapter address is (%02X %02X %02X %02X %02X %02X).\n",
            A[0], A[1], A[2], A[3], A[4], A[5]));
    }
#endif /* DBG */

    /* Get maximum lookahead buffer size of adapter */
    NdisStatus = MiniQueryInformation(Adapter,
                                      OID_GEN_MAXIMUM_LOOKAHEAD,
                                      0,
                                      &BytesWritten);
    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        NDIS_DbgPrint(MIN_TRACE, ("OID_GEN_MAXIMUM_LOOKAHEAD failed. NdisStatus (0x%X).\n", NdisStatus));
        return NdisStatus;
    }

    Adapter->MaxLookaheadLength = *((PULONG)Adapter->QueryBuffer);

    NDIS_DbgPrint(DEBUG_MINIPORT, ("MaxLookaheadLength (0x%X).\n", Adapter->MaxLookaheadLength));

    /* Get current lookahead buffer size of adapter */
    NdisStatus = MiniQueryInformation(Adapter,
                                      OID_GEN_CURRENT_LOOKAHEAD,
                                      0,
                                      &BytesWritten);
    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        NDIS_DbgPrint(MIN_TRACE, ("OID_GEN_CURRENT_LOOKAHEAD failed. NdisStatus (0x%X).\n", NdisStatus));
        return NdisStatus;
    }

    Adapter->CurLookaheadLength = *((PULONG)Adapter->QueryBuffer);

    NDIS_DbgPrint(DEBUG_MINIPORT, ("CurLookaheadLength (0x%X).\n", Adapter->CurLookaheadLength));

    if (Adapter->MaxLookaheadLength != 0) {
        Adapter->LookaheadLength = Adapter->MaxLookaheadLength +
                                   Adapter->MediumHeaderSize;
        Adapter->LookaheadBuffer = ExAllocatePool(NonPagedPool,
                                                  Adapter->LookaheadLength);
        if (!Adapter->LookaheadBuffer)
            return NDIS_STATUS_RESOURCES;
    }

    return STATUS_SUCCESS;
}


/*
 * @implemented
 */
NDIS_STATUS
EXPORT
NdisMRegisterMiniport(
    IN  NDIS_HANDLE                     NdisWrapperHandle,
    IN  PNDIS_MINIPORT_CHARACTERISTICS  MiniportCharacteristics,
    IN  UINT                            CharacteristicsLength)
/*
 * FUNCTION: Registers a miniport's MiniportXxx entry points with the NDIS library
 * ARGUMENTS:
 *     NdisWrapperHandle       = Pointer to handle returned by NdisMInitializeWrapper
 *     MiniportCharacteristics = Pointer to a buffer with miniport characteristics
 *     CharacteristicsLength   = Number of bytes in characteristics buffer
 * RETURNS:
 *     Status of operation	 
 * NOTES:
 *     - To create device objects for the miniport, the Route value under Linkage is
 *       parsed.  I don't know if this is the way Microsoft does it or not.
 * TODO: 
 *     verify this algorithm by playing with nt
 *     break this function up
 */
{
    UINT MinSize;
    KIRQL OldIrql;
    NTSTATUS Status;
    NDIS_STATUS NdisStatus;
    NDIS_STATUS OpenErrorStatus;
    UINT SelectedMediumIndex;
    PLOGICAL_ADAPTER Adapter;
    NDIS_OID AddressOID;
    BOOLEAN MemError          = FALSE;
    PMINIPORT_DRIVER Miniport = GET_MINIPORT_DRIVER(NdisWrapperHandle);
    OBJECT_ATTRIBUTES DeviceKeyAttributes;
    OBJECT_ATTRIBUTES LinkageKeyAttributes;
    HANDLE DeviceKeyHandle;
    HANDLE LinkageKeyHandle;
    UNICODE_STRING RouteVal;
    UNICODE_STRING LinkageKeyName;
    KEY_VALUE_PARTIAL_INFORMATION *RouteData;
    ULONG RouteDataLength;
    UINT NextRouteOffset = 0;

    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    switch (MiniportCharacteristics->MajorNdisVersion) {
    case 0x03:
        MinSize = sizeof(NDIS30_MINIPORT_CHARACTERISTICS_S);
        break;

    case 0x04:
        MinSize = sizeof(NDIS40_MINIPORT_CHARACTERISTICS_S);
        break;

    case 0x05:
        MinSize = sizeof(NDIS50_MINIPORT_CHARACTERISTICS_S);
        break;

    default:
        NDIS_DbgPrint(DEBUG_MINIPORT, ("Bad miniport characteristics version.\n"));
        return NDIS_STATUS_BAD_VERSION;
    }

    if (CharacteristicsLength < MinSize) {
        NDIS_DbgPrint(DEBUG_MINIPORT, ("Bad miniport characteristics.\n"));
        return NDIS_STATUS_BAD_CHARACTERISTICS;
    }

    /* Check if mandatory MiniportXxx functions are specified */
    if ((!MiniportCharacteristics->HaltHandler) ||
        (!MiniportCharacteristics->InitializeHandler)||
        (!MiniportCharacteristics->QueryInformationHandler) ||
        (!MiniportCharacteristics->ResetHandler) ||
        (!MiniportCharacteristics->SetInformationHandler)) {
        NDIS_DbgPrint(DEBUG_MINIPORT, ("Bad miniport characteristics.\n"));
        return NDIS_STATUS_BAD_CHARACTERISTICS;
    }

    if (MiniportCharacteristics->MajorNdisVersion == 0x03) {
        if (!MiniportCharacteristics->u1.SendHandler) {
            NDIS_DbgPrint(DEBUG_MINIPORT, ("Bad miniport characteristics.\n"));
            return NDIS_STATUS_BAD_CHARACTERISTICS;
        }
    } else if (MiniportCharacteristics->MajorNdisVersion >= 0x04) {
        /* NDIS 4.0+ */
        if ((!MiniportCharacteristics->u1.SendHandler) &&
            (!MiniportCharacteristics->SendPacketsHandler)) {
            NDIS_DbgPrint(DEBUG_MINIPORT, ("Bad miniport characteristics.\n"));
            return NDIS_STATUS_BAD_CHARACTERISTICS;
        }
    }

    RtlCopyMemory(&Miniport->Chars, MiniportCharacteristics, MinSize);

    /* Read the miniport config from the registry */
    InitializeObjectAttributes(&DeviceKeyAttributes, Miniport->RegistryPath, OBJ_CASE_INSENSITIVE, NULL, NULL);

    Status = ZwOpenKey(&DeviceKeyHandle, KEY_READ, &DeviceKeyAttributes);
    if(!NT_SUCCESS(Status))
    {
        NDIS_DbgPrint(MIN_TRACE,("Failed to open driver key: 0x%x\n", Status));
        return NDIS_STATUS_FAILURE;
    }

    RtlInitUnicodeString(&LinkageKeyName, L"Linkage");
    InitializeObjectAttributes(&LinkageKeyAttributes, &LinkageKeyName, OBJ_CASE_INSENSITIVE, DeviceKeyHandle, NULL);

    Status = ZwOpenKey(&LinkageKeyHandle, KEY_READ, &LinkageKeyAttributes);
    if(!NT_SUCCESS(Status))
    {
        NDIS_DbgPrint(MIN_TRACE,("Failed to open Linkage key: 0x%x\n", Status));
        ZwClose(DeviceKeyHandle);
        return NDIS_STATUS_FAILURE;
    }

    RouteData = ExAllocatePool(PagedPool, ROUTE_DATA_SIZE);
    if(!RouteData)
    {
        NDIS_DbgPrint(MIN_TRACE,("Insufficient resources\n"));
        ZwClose(LinkageKeyHandle);
        ZwClose(DeviceKeyHandle);
        return NDIS_STATUS_RESOURCES;
    }

    RtlInitUnicodeString(&RouteVal, L"Route");

    Status = ZwQueryValueKey(LinkageKeyHandle, &RouteVal, KeyValuePartialInformation, RouteData, ROUTE_DATA_SIZE, &RouteDataLength);
    if(!NT_SUCCESS(Status))
    {
        NDIS_DbgPrint(MIN_TRACE,("Failed to query Route value\n"));
        ZwClose(LinkageKeyHandle);
        ZwClose(DeviceKeyHandle);
        ExFreePool(RouteData);
        return NDIS_STATUS_FAILURE;
    }

    ZwClose(LinkageKeyHandle);
    ZwClose(DeviceKeyHandle);

    /* route is a REG_MULTI_SZ with each nic object created by NDI - create an adapter for each */
    while((RouteData->Data)[NextRouteOffset])
    {
        WCHAR *DeviceName;
        HANDLE RegKeyHandle;
        WCHAR *RegKeyPath;
        UNICODE_STRING RegKeyPathU;
        OBJECT_ATTRIBUTES RegKeyAttributes;

        Adapter = ExAllocatePool(NonPagedPool, sizeof(LOGICAL_ADAPTER));
        if (!Adapter) {
            NDIS_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
            ExFreePool(RouteData);
            return NDIS_STATUS_RESOURCES;
        }

        /* This is very important */
        RtlZeroMemory(Adapter, sizeof(LOGICAL_ADAPTER));

        DeviceName = ExAllocatePool(NonPagedPool, sizeof(DEVICE_ROOT) + RouteData->DataLength);
        if(!DeviceName)
        {
            NDIS_DbgPrint(MIN_TRACE,("Insufficient memory\n"));
            ExFreePool(Adapter);
            ExFreePool(RouteData);
            return NDIS_STATUS_RESOURCES;
        }

        wcscpy(DeviceName, DEVICE_ROOT);
        wcsncat(DeviceName, (WCHAR *)RouteData->Data, RouteData->DataLength); /* reg_sz is 0-term by def */
        RtlInitUnicodeString(&Adapter->DeviceName, DeviceName);

        NDIS_DbgPrint(MAX_TRACE, ("creating device %ws\n", DeviceName));

        Status = IoCreateDevice(Miniport->DriverObject, 0, &Adapter->DeviceName, FILE_DEVICE_PHYSICAL_NETCARD,
            0, FALSE, &Adapter->NdisMiniportBlock.DeviceObject);
        if (!NT_SUCCESS(Status)) {
            NDIS_DbgPrint(MIN_TRACE, ("Could not create device object.\n"));
            ExFreePool(Adapter);
            ExFreePool(RouteData);
            return NDIS_STATUS_FAILURE;
        }

        RegKeyPath = ExAllocatePool(PagedPool, sizeof(SERVICES_ROOT)+RouteData->DataLength);    /* includes room for a 0-term */
        if(!RegKeyPath)
        {
            NDIS_DbgPrint(MIN_TRACE,("Insufficient resources\n"));
            ExFreePool(Adapter);
            ExFreePool(RouteData);
            return NDIS_STATUS_FAILURE;
        }

        wcscpy(RegKeyPath, SERVICES_ROOT);
        wcsncat(RegKeyPath, (WCHAR *)RouteData->Data, RouteData->DataLength);
        RegKeyPath[sizeof(SERVICES_ROOT)+RouteData->DataLength-1] = 0;

        RtlInitUnicodeString(&RegKeyPathU, RegKeyPath);
        InitializeObjectAttributes(&RegKeyAttributes, &RegKeyPathU, OBJ_CASE_INSENSITIVE, NULL, NULL);

        Status = ZwOpenKey(&RegKeyHandle, KEY_ALL_ACCESS, &RegKeyAttributes);
        if(Status != STATUS_SUCCESS)
        {
            NDIS_DbgPrint(MIN_TRACE,("ndis!NdisMRegisterMiniport: failed to open adapter-specific reg key %ws\n", RegKeyPath));
            ExFreePool(Adapter);
            ExFreePool(RouteData);
            return NDIS_STATUS_FAILURE;
        }

        KeInitializeSpinLock(&Adapter->NdisMiniportBlock.Lock);
        InitializeListHead(&Adapter->ProtocolListHead);
        Adapter->RefCount = 1;
        Adapter->Miniport = Miniport;

        /* Set handlers (some NDIS macros require these) */

        Adapter->NdisMiniportBlock.EthRxCompleteHandler = MiniEthReceiveComplete;
        Adapter->NdisMiniportBlock.EthRxIndicateHandler = MiniEthReceiveIndication;
        Adapter->NdisMiniportBlock.SendCompleteHandler  = MiniSendComplete;
        Adapter->NdisMiniportBlock.SendResourcesHandler = MiniSendResourcesAvailable;
        Adapter->NdisMiniportBlock.ResetCompleteHandler = MiniResetComplete;
        Adapter->NdisMiniportBlock.TDCompleteHandler    = MiniTransferDataComplete;

        KeInitializeDpc(&Adapter->MiniportDpc, MiniportDpc, (PVOID)Adapter);

        /* Put adapter in adapter list for this miniport */
        ExInterlockedInsertTailList(&Miniport->AdapterListHead, &Adapter->MiniportListEntry, &Miniport->Lock);

        /* Put adapter in global adapter list */
        ExInterlockedInsertTailList(&AdapterListHead, &Adapter->ListEntry, &AdapterListLock);

        /* Call MiniportInitialize */
        NDIS_DbgPrint(MID_TRACE, ("calling MiniportInitialize\n"));
        NdisStatus = (*Miniport->Chars.InitializeHandler)( &OpenErrorStatus, &SelectedMediumIndex, &MediaArray[0],
            MEDIA_ARRAY_SIZE, Adapter, RegKeyHandle);

        ZwClose(RegKeyHandle);

        if ((NdisStatus == NDIS_STATUS_SUCCESS) && (SelectedMediumIndex < MEDIA_ARRAY_SIZE)) {

            NDIS_DbgPrint(MID_TRACE,("successful return from MiniportInitialize\n"));

            Adapter->NdisMiniportBlock.MediaType = MediaArray[SelectedMediumIndex];

            switch (Adapter->NdisMiniportBlock.MediaType) {
            case NdisMedium802_3:
                Adapter->MediumHeaderSize = 14;
                AddressOID = OID_802_3_CURRENT_ADDRESS;
                Adapter->AddressLength = ETH_LENGTH_OF_ADDRESS;

                Adapter->NdisMiniportBlock.FilterDbs.u.EthDB = ExAllocatePool(NonPagedPool, sizeof(ETH_FILTER));
                if (Adapter->NdisMiniportBlock.FilterDbs.u.EthDB) {
                    RtlZeroMemory(Adapter->NdisMiniportBlock.FilterDbs.u.EthDB, sizeof(ETH_FILTER));
                    Adapter->NdisMiniportBlock.FilterDbs.u.EthDB->Miniport = (PNDIS_MINIPORT_BLOCK)Adapter;
                } else
                    MemError = TRUE;
                break;

            default:
                /* FIXME: Support other types of media */
                ExFreePool(Adapter);
                ExFreePool(RouteData);
                ASSERT(FALSE);
                return NDIS_STATUS_FAILURE;
            }

            NdisStatus = DoQueries(Adapter, AddressOID);
        }

        if ((MemError) || (NdisStatus != NDIS_STATUS_SUCCESS) || (SelectedMediumIndex >= MEDIA_ARRAY_SIZE)) {

            /* Remove adapter from adapter list for this miniport */
            KeAcquireSpinLock(&Miniport->Lock, &OldIrql);
            RemoveEntryList(&Adapter->MiniportListEntry);
            KeReleaseSpinLock(&Miniport->Lock, OldIrql);

            /* Remove adapter from global adapter list */
            KeAcquireSpinLock(&AdapterListLock, &OldIrql);
            RemoveEntryList(&Adapter->ListEntry);
            KeReleaseSpinLock(&AdapterListLock, OldIrql);

            if (Adapter->LookaheadBuffer)
            ExFreePool(Adapter->LookaheadBuffer);

            IoDeleteDevice(Adapter->NdisMiniportBlock.DeviceObject);
            ExFreePool(Adapter);
            NDIS_DbgPrint(MIN_TRACE, ("MiniportInitialize() failed for an adapter.\n"));
        }

        NextRouteOffset += wcslen((WCHAR *)RouteData->Data);
    }

    ExFreePool(RouteData);
    return NDIS_STATUS_SUCCESS;
}


/*
 * @implemented
 */
VOID
EXPORT
NdisMResetComplete(
    IN NDIS_HANDLE MiniportAdapterHandle,
    IN NDIS_STATUS Status,
    IN BOOLEAN     AddressingReset)
{
    MiniResetComplete(MiniportAdapterHandle,
                      Status,
                      AddressingReset);
}


/*
 * @implemented
 */
VOID
EXPORT
NdisMSendComplete(
    IN  NDIS_HANDLE     MiniportAdapterHandle,
    IN  PNDIS_PACKET    Packet,
    IN  NDIS_STATUS     Status)
/*
 * FUNCTION: Forwards a message to the initiating protocol saying
 *           that a packet was handled
 * ARGUMENTS:
 *     NdisAdapterHandle = Handle input to MiniportInitialize
 *     Packet            = Pointer to NDIS packet that was sent
 *     Status            = Status of send operation
 */
{
    MiniSendComplete(MiniportAdapterHandle,
                     Packet,
                     Status);
}


/*
 * @implemented
 */
VOID
EXPORT
NdisMSendResourcesAvailable(
    IN  NDIS_HANDLE MiniportAdapterHandle)
{
    MiniSendResourcesAvailable(MiniportAdapterHandle);
}


/*
 * @implemented
 */
VOID
EXPORT
NdisMTransferDataComplete(
    IN  NDIS_HANDLE     MiniportAdapterHandle,
    IN  PNDIS_PACKET    Packet,
    IN  NDIS_STATUS     Status,
    IN  UINT            BytesTransferred)
{
    MiniTransferDataComplete(MiniportAdapterHandle,
                             Packet,
                             Status,
                             BytesTransferred);
}


/*
 * @implemented
 */
VOID
EXPORT
NdisMSetInformationComplete(
    IN  NDIS_HANDLE MiniportAdapterHandle,
    IN  NDIS_STATUS Status)
{
    PLOGICAL_ADAPTER Adapter        = GET_LOGICAL_ADAPTER(MiniportAdapterHandle);
    PADAPTER_BINDING AdapterBinding = (PADAPTER_BINDING)Adapter->MiniportAdapterBinding;

    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    (*AdapterBinding->ProtocolBinding->Chars.RequestCompleteHandler)(
        AdapterBinding->NdisOpenBlock.ProtocolBindingContext,
        Adapter->NdisMiniportBlock.MediaRequest,
        Status);
}


/*
 * @implemented
 */
VOID
EXPORT
NdisMSetAttributes(
    IN  NDIS_HANDLE         MiniportAdapterHandle,
    IN  NDIS_HANDLE         MiniportAdapterContext,
    IN  BOOLEAN             BusMaster,
    IN  NDIS_INTERFACE_TYPE AdapterType)
/*
 * FUNCTION: Informs the NDIS library of significant features of the caller's NIC
 * ARGUMENTS:
 *     MiniportAdapterHandle  = Handle input to MiniportInitialize
 *     MiniportAdapterContext = Pointer to context information
 *     BusMaster              = Specifies TRUE if the caller's NIC is a busmaster DMA device
 *     AdapterType            = Specifies the I/O bus interface of the caller's NIC
 */
{
    PLOGICAL_ADAPTER Adapter = GET_LOGICAL_ADAPTER(MiniportAdapterHandle);

    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    Adapter->NdisMiniportBlock.MiniportAdapterContext = MiniportAdapterContext;
    Adapter->Attributes    = BusMaster? NDIS_ATTRIBUTE_BUS_MASTER : 0;
    Adapter->NdisMiniportBlock.AdapterType   = AdapterType;
    Adapter->AttributesSet = TRUE;
}


/*
 * @implemented
 */
VOID
EXPORT
NdisMSetAttributesEx(
    IN  NDIS_HANDLE         MiniportAdapterHandle,
    IN  NDIS_HANDLE         MiniportAdapterContext,
    IN  UINT                CheckForHangTimeInSeconds   OPTIONAL,
    IN  ULONG               AttributeFlags,
    IN  NDIS_INTERFACE_TYPE	AdapterType)
/*
 * FUNCTION: Informs the NDIS library of significant features of the caller's NIC
 * ARGUMENTS:
 *     MiniportAdapterHandle     = Handle input to MiniportInitialize
 *     MiniportAdapterContext    = Pointer to context information
 *     CheckForHangTimeInSeconds = Specifies interval in seconds at which
 *                                 MiniportCheckForHang should be called
 *     AttributeFlags            = Bitmask that indicates specific attributes
 *     AdapterType               = Specifies the I/O bus interface of the caller's NIC
 */
{
	// Currently just like NdisMSetAttributesEx
	// TODO: Take CheckForHandTimeInSeconds into account!
	PLOGICAL_ADAPTER Adapter = GET_LOGICAL_ADAPTER(MiniportAdapterHandle);

    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));
    NDIS_DbgPrint(MIN_TRACE, ("NdisMSetAttributesEx() is partly-implemented."));

    Adapter->NdisMiniportBlock.MiniportAdapterContext = MiniportAdapterContext;
	Adapter->Attributes = AttributeFlags & NDIS_ATTRIBUTE_BUS_MASTER;
	Adapter->NdisMiniportBlock.AdapterType   = AdapterType;
    Adapter->AttributesSet = TRUE;
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisMSleep(
    IN  ULONG   MicrosecondsToSleep)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
BOOLEAN
EXPORT
NdisMSynchronizeWithInterrupt(
    IN  PNDIS_MINIPORT_INTERRUPT    Interrupt,
    IN  PVOID                       SynchronizeFunction,
    IN  PVOID                       SynchronizeContext)
{
    UNIMPLEMENTED

    return FALSE;
}


/*
 * @unimplemented
 */
NDIS_STATUS
EXPORT
NdisMWriteLogData(
    IN  NDIS_HANDLE LogHandle,
    IN  PVOID       LogBuffer,
    IN  UINT        LogBufferSize)
{
    UNIMPLEMENTED

	return NDIS_STATUS_FAILURE;
}


/*
 * @implemented
 */
VOID
EXPORT
NdisTerminateWrapper(
    IN  NDIS_HANDLE NdisWrapperHandle,
    IN  PVOID       SystemSpecific)
/*
 * FUNCTION: Releases resources allocated by a call to NdisInitializeWrapper
 * ARGUMENTS:
 *     NdisWrapperHandle = Handle returned by NdisInitializeWrapper (MINIPORT_DRIVER)
 *     SystemSpecific    = Always NULL
 */
{
    PMINIPORT_DRIVER Miniport = GET_MINIPORT_DRIVER(NdisWrapperHandle);

    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

	 ExFreePool(Miniport->RegistryPath->Buffer);
	 ExFreePool(Miniport->RegistryPath);
    ExFreePool(Miniport);
}

/* EOF */
/* enum test */
	 /*
    {
	ULONG KeyInformationSize;
	KEY_BASIC_INFORMATION *KeyInformation;
	int i;

	KeyInformation = ExAllocatePool(PagedPool, 1024);
	ASSERT(KeyInformation);

	RtlInitUnicodeString(&LinkageKeyName, L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\NE2000");
	InitializeObjectAttributes(&LinkageKeyAttributes, &LinkageKeyName, OBJ_CASE_INSENSITIVE|OBJ_KERNEL_HANDLE, NULL, NULL);

	Status = ZwOpenKey(&LinkageKeyHandle, KEY_READ, &LinkageKeyAttributes);
	if(!NT_SUCCESS(Status))
	{
		DbgPrint("ndis!NdisMRegisterMiniport: Failed to open Linkage key: 0x%x\n", Status);
		ASSERT(0);	
		KeBugCheck(0);
	}

	for(i=0;i<5;i++)
	{
	    Status = ZwEnumerateKey(LinkageKeyHandle, i, KeyBasicInformation, KeyInformation, 1024, &KeyInformationSize);
            if(!NT_SUCCESS(Status))
	    {
                DbgPrint("ndis!NdisMRegisterMiniport: Failed to enumerate: 0x%x\n", Status);
		break;
	    }
	   
	    KeyInformation->Name[KeyInformation->NameLength] = 0;
            DbgPrint("ndis!NdisMRegisterMiniport: enumerated key %ws\n", KeyInformation->Name);
	}

	ExFreePool(KeyInformation);
	KeBugCheck(0);
    }
	 */
