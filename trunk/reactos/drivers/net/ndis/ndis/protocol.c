/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NDIS library
 * FILE:        ndis/protocol.c
 * PURPOSE:     Routines used by NDIS protocol drivers
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */
#include <ndissys.h>
#include <miniport.h>
#include <protocol.h>
#include <buffer.h>


LIST_ENTRY ProtocolListHead;
KSPIN_LOCK ProtocolListLock;

/*
 * @implemented
 */
VOID
EXPORT
NdisCompleteBindAdapter(
    IN  NDIS_HANDLE BindAdapterContext,
    IN  NDIS_STATUS Status,
    IN  NDIS_STATUS OpenStatus)
/*
 * FUNCTION: Indicates a packet to bound protocols
 * ARGUMENTS:
 *     Adapter = Pointer to logical adapter
 *     Packet  = Pointer to packet to indicate
 * RETURNS:
 *     Status of operation
 */
{
    /* 
	  * XXX partially-implemented!
	  *
	  * need to handle error conditions, and i'm not sure this is even what this func should do.
	  * be sure to fix NdisRegisterProtocol before fixing this, though.
	  */

    PROTOCOL_BINDING *Protocol = (PROTOCOL_BINDING *)BindAdapterContext;

    /* Put protocol binding struct on global list */
    ExInterlockedInsertTailList(&ProtocolListHead, &Protocol->ListEntry, &ProtocolListLock);
}


NDIS_STATUS
ProIndicatePacket(
    PLOGICAL_ADAPTER Adapter,
    PNDIS_PACKET Packet)
/*
 * FUNCTION: Indicates a packet to bound protocols
 * ARGUMENTS:
 *     Adapter = Pointer to logical adapter
 *     Packet  = Pointer to packet to indicate
 * RETURNS:
 *     Status of operation
 */
{
    KIRQL OldIrql;
    UINT Length;
    UINT Total;

    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

#ifdef DBG
    MiniDisplayPacket(Packet);
#endif

    NdisQueryPacket(Packet, NULL, NULL, NULL, &Total);

    KeAcquireSpinLock(&Adapter->NdisMiniportBlock.Lock, &OldIrql);

    Adapter->LoopPacket = Packet;

    Length = CopyPacketToBuffer(
        Adapter->LookaheadBuffer,
        Packet,
        0,
        Adapter->CurLookaheadLength);

    KeReleaseSpinLock(&Adapter->NdisMiniportBlock.Lock, OldIrql);

    if (Length > Adapter->MediumHeaderSize) {
        MiniIndicateData(Adapter,
                         NULL,
                         Adapter->LookaheadBuffer,
                         Adapter->MediumHeaderSize,
                         &Adapter->LookaheadBuffer[Adapter->MediumHeaderSize],
                         Length - Adapter->MediumHeaderSize,
                         Total - Adapter->MediumHeaderSize);
    } else {
        MiniIndicateData(Adapter,
                         NULL,
                         Adapter->LookaheadBuffer,
                         Adapter->MediumHeaderSize,
                         NULL,
                         0,
                         0);
    }

    KeAcquireSpinLock(&Adapter->NdisMiniportBlock.Lock, &OldIrql);

    Adapter->LoopPacket = NULL;

    KeReleaseSpinLock(&Adapter->NdisMiniportBlock.Lock, OldIrql);

    return STATUS_SUCCESS;
}


NDIS_STATUS
ProRequest(
    IN  NDIS_HANDLE     MacBindingHandle,
    IN  PNDIS_REQUEST   NdisRequest)
/*
 * FUNCTION: Forwards a request to an NDIS miniport
 * ARGUMENTS:
 *     MacBindingHandle = Adapter binding handle
 *     NdisRequest      = Pointer to request to perform
 * RETURNS:
 *     Status of operation
 */
{
    KIRQL OldIrql;
    BOOLEAN Queue;
    NDIS_STATUS NdisStatus;
    PADAPTER_BINDING AdapterBinding = GET_ADAPTER_BINDING(MacBindingHandle);
    PLOGICAL_ADAPTER Adapter        = AdapterBinding->Adapter;

    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    KeAcquireSpinLock(&Adapter->NdisMiniportBlock.Lock, &OldIrql);
    Queue = Adapter->MiniportBusy;
    if (Queue) {
        MiniQueueWorkItem(Adapter,
                          NdisWorkItemRequest,
                          (PVOID)NdisRequest,
                          (NDIS_HANDLE)AdapterBinding);
    } else {
        Adapter->MiniportBusy = TRUE;
    }
    KeReleaseSpinLock(&Adapter->NdisMiniportBlock.Lock, OldIrql);

    if (!Queue) {
        KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
        NdisStatus = MiniDoRequest(Adapter, NdisRequest);
        KeAcquireSpinLockAtDpcLevel(&Adapter->NdisMiniportBlock.Lock);
        Adapter->MiniportBusy = FALSE;
        if (Adapter->WorkQueueHead)
            KeInsertQueueDpc(&Adapter->MiniportDpc, NULL, NULL);
        KeReleaseSpinLockFromDpcLevel(&Adapter->NdisMiniportBlock.Lock);
        KeLowerIrql(OldIrql);
    } else {
        NdisStatus = NDIS_STATUS_PENDING;
    }
    return NdisStatus;
}


NDIS_STATUS
ProReset(
    IN  NDIS_HANDLE MacBindingHandle)
{
    UNIMPLEMENTED

    return NDIS_STATUS_FAILURE;
}


NDIS_STATUS
ProSend(
    IN  NDIS_HANDLE     MacBindingHandle,
    IN  PNDIS_PACKET    Packet)
/*
 * FUNCTION: Forwards a request to send a packet to an NDIS miniport
 * ARGUMENTS:
 *     MacBindingHandle = Adapter binding handle
 *     Packet           = Pointer to NDIS packet descriptor
 */
{
    KIRQL OldIrql;
    BOOLEAN Queue;
    NDIS_STATUS NdisStatus;
    PADAPTER_BINDING AdapterBinding  = GET_ADAPTER_BINDING(MacBindingHandle);
    PLOGICAL_ADAPTER Adapter         = AdapterBinding->Adapter;

    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    /* FIXME: Should queue packet if miniport returns NDIS_STATUS_RESOURCES */

    Packet->Reserved[0] = (ULONG_PTR)MacBindingHandle;

    KeAcquireSpinLock(&Adapter->NdisMiniportBlock.Lock, &OldIrql);
    Queue = Adapter->MiniportBusy;

    /* We may have to loop this packet if miniport cannot */
    if (Adapter->NdisMiniportBlock.MacOptions & NDIS_MAC_OPTION_NO_LOOPBACK) {
        if (MiniAdapterHasAddress(Adapter, Packet)) {
            /* Do software loopback because miniport does not support it */

            NDIS_DbgPrint(MIN_TRACE, ("Looping packet.\n"));

            if (Queue) {

                /* FIXME: Packets should properbly be queued directly on the adapter instead */

                MiniQueueWorkItem(Adapter,
                                  NdisWorkItemSendLoopback,
                                  (PVOID)Packet,
                                  (NDIS_HANDLE)AdapterBinding);
            } else {
                Adapter->MiniportBusy = TRUE;
            }
            KeReleaseSpinLock(&Adapter->NdisMiniportBlock.Lock, OldIrql);

            if (!Queue) {
                KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
                NdisStatus = ProIndicatePacket(Adapter, Packet);
                KeAcquireSpinLockAtDpcLevel(&Adapter->NdisMiniportBlock.Lock);
                Adapter->MiniportBusy = FALSE;
                if (Adapter->WorkQueueHead)
                    KeInsertQueueDpc(&Adapter->MiniportDpc, NULL, NULL);
                KeReleaseSpinLockFromDpcLevel(&Adapter->NdisMiniportBlock.Lock);
                KeLowerIrql(OldIrql);
                return NdisStatus;
            } else {
                return NDIS_STATUS_PENDING;
            }
        }
    }

    if (Queue) {

        /* FIXME: Packets should properbly be queued directly on the adapter instead */

        MiniQueueWorkItem(Adapter,
                          NdisWorkItemSend,
                          (PVOID)Packet,
                          (NDIS_HANDLE)AdapterBinding);
    } else {
        Adapter->MiniportBusy = TRUE;
    }
    KeReleaseSpinLock(&Adapter->NdisMiniportBlock.Lock, OldIrql);

    if (!Queue) {
        KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
        NdisStatus = (*Adapter->Miniport->Chars.u1.SendHandler)(
            Adapter->NdisMiniportBlock.MiniportAdapterContext,
            Packet,
            0);
        KeAcquireSpinLockAtDpcLevel(&Adapter->NdisMiniportBlock.Lock);
        Adapter->MiniportBusy = FALSE;
        if (Adapter->WorkQueueHead)
            KeInsertQueueDpc(&Adapter->MiniportDpc, NULL, NULL);
        KeReleaseSpinLockFromDpcLevel(&Adapter->NdisMiniportBlock.Lock);
        KeLowerIrql(OldIrql);
    } else {
        NdisStatus = NDIS_STATUS_PENDING;
    }
    return NdisStatus;
}


VOID
ProSendPackets(
    IN  NDIS_HANDLE     NdisBindingHandle,
    IN  PPNDIS_PACKET   PacketArray,
    IN  UINT            NumberOfPackets)
{
    UNIMPLEMENTED
}


NDIS_STATUS
ProTransferData(
    IN  NDIS_HANDLE         MacBindingHandle,
    IN  NDIS_HANDLE         MacReceiveContext,
    IN  UINT                ByteOffset,
    IN  UINT                BytesToTransfer,
    IN  OUT	PNDIS_PACKET    Packet,
    OUT PUINT               BytesTransferred)
/*
 * FUNCTION: Forwards a request to copy received data into a protocol-supplied packet
 * ARGUMENTS:
 *     MacBindingHandle  = Adapter binding handle
 *     MacReceiveContext = MAC receive context
 *     ByteOffset        = Offset in packet to place data
 *     BytesToTransfer   = Number of bytes to copy into packet
 *     Packet            = Pointer to NDIS packet descriptor
 *     BytesTransferred  = Address of buffer to place number of bytes copied
 */
{
    PADAPTER_BINDING AdapterBinding = GET_ADAPTER_BINDING(MacBindingHandle);
    PLOGICAL_ADAPTER Adapter        = AdapterBinding->Adapter;

    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    /* FIXME: Interrupts must be disabled for adapter */

    if (Packet == Adapter->LoopPacket) {
        /* NDIS is responsible for looping this packet */
        NdisCopyFromPacketToPacket(Packet,
                                   ByteOffset,
                                   BytesToTransfer,
                                   Adapter->LoopPacket,
                                   0,
                                   BytesTransferred);
        return NDIS_STATUS_SUCCESS;
    }

    return (*Adapter->Miniport->Chars.u2.TransferDataHandler)(
        Packet,
        BytesTransferred,
        Adapter->NdisMiniportBlock.MiniportAdapterContext,
        MacReceiveContext,
        ByteOffset,
        BytesToTransfer);
}



/*
 * @implemented
 */
VOID
EXPORT
NdisCloseAdapter(
    OUT PNDIS_STATUS    Status,
    IN  NDIS_HANDLE     NdisBindingHandle)
/*
 * FUNCTION: Closes an adapter opened with NdisOpenAdapter
 * ARGUMENTS:
 *     Status            = Address of buffer for status information
 *     NdisBindingHandle = Handle returned by NdisOpenAdapter
 */
{
    KIRQL OldIrql;
    PADAPTER_BINDING AdapterBinding = GET_ADAPTER_BINDING(NdisBindingHandle);

    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    /* Remove from protocol's bound adapters list */
    KeAcquireSpinLock(&AdapterBinding->ProtocolBinding->Lock, &OldIrql);
    RemoveEntryList(&AdapterBinding->ProtocolListEntry);
    KeReleaseSpinLock(&AdapterBinding->ProtocolBinding->Lock, OldIrql);

    /* Remove protocol from adapter's bound protocols list */
    KeAcquireSpinLock(&AdapterBinding->Adapter->NdisMiniportBlock.Lock, &OldIrql);
    RemoveEntryList(&AdapterBinding->AdapterListEntry);
    KeReleaseSpinLock(&AdapterBinding->Adapter->NdisMiniportBlock.Lock, OldIrql);

    ExFreePool(AdapterBinding);

    *Status = NDIS_STATUS_SUCCESS;
}


/*
 * @implemented
 */
VOID
EXPORT
NdisDeregisterProtocol(
    OUT PNDIS_STATUS    Status,
    IN NDIS_HANDLE      NdisProtocolHandle)
/*
 * FUNCTION: Releases the resources allocated by NdisRegisterProtocol
 * ARGUMENTS:
 *     Status             = Address of buffer for status information
 *     NdisProtocolHandle = Handle returned by NdisRegisterProtocol
 */
{
    KIRQL OldIrql;
    PPROTOCOL_BINDING Protocol = GET_PROTOCOL_BINDING(NdisProtocolHandle);

    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    /* FIXME: Make sure no adapter bindings exist */

    /* Remove protocol from global list */
    KeAcquireSpinLock(&ProtocolListLock, &OldIrql);
    RemoveEntryList(&Protocol->ListEntry);
    KeReleaseSpinLock(&ProtocolListLock, OldIrql);

    ExFreePool(Protocol);

    *Status = NDIS_STATUS_SUCCESS;
}


/*
 * @implemented
 */
VOID
EXPORT
NdisOpenAdapter(
    OUT PNDIS_STATUS    Status,
    OUT PNDIS_STATUS    OpenErrorStatus,
    OUT PNDIS_HANDLE    NdisBindingHandle,
    OUT PUINT           SelectedMediumIndex,
    IN  PNDIS_MEDIUM    MediumArray,
    IN  UINT            MediumArraySize,
    IN  NDIS_HANDLE     NdisProtocolHandle,
    IN  NDIS_HANDLE     ProtocolBindingContext,
    IN  PNDIS_STRING    AdapterName,
    IN  UINT            OpenOptions,
    IN  PSTRING         AddressingInformation   OPTIONAL)
/*
 * FUNCTION: Opens an adapter for communication
 * ARGUMENTS:
 *     Status                 = Address of buffer for status information
 *     OpenErrorStatus        = Address of buffer for secondary error code
 *     NdisBindingHandle      = Address of buffer for adapter binding handle
 *     SelectedMediumIndex    = Address of buffer for selected medium
 *     MediumArray            = Pointer to an array of NDIS_MEDIUMs called can support
 *     MediumArraySize        = Number of elements in MediumArray
 *     NdisProtocolHandle     = Handle returned by NdisRegisterProtocol
 *     ProtocolBindingContext = Pointer to caller suplied context area
 *     AdapterName            = Pointer to buffer with name of adapter
 *     OpenOptions            = Bitmask with flags passed to next-lower driver
 *     AddressingInformation  = Optional pointer to buffer with NIC specific information
 */
{
    UINT i;
    BOOLEAN Found;
    PLOGICAL_ADAPTER Adapter;
    PADAPTER_BINDING AdapterBinding;
    PPROTOCOL_BINDING Protocol = GET_PROTOCOL_BINDING(NdisProtocolHandle);

    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    Adapter = MiniLocateDevice(AdapterName);
    if (!Adapter) {
        NDIS_DbgPrint(MIN_TRACE, ("Adapter not found.\n"));
        *Status = NDIS_STATUS_ADAPTER_NOT_FOUND;
        return;
    }

    /* Find the media type in the list provided by the protocol driver */
    Found = FALSE;
    for (i = 0; i < MediumArraySize; i++) {
        if (Adapter->NdisMiniportBlock.MediaType == MediumArray[i]) {
            *SelectedMediumIndex = i;
            Found = TRUE;
            break;
        }
    }

    if (!Found) {
        NDIS_DbgPrint(MIN_TRACE, ("Medium is not supported.\n"));
        *Status = NDIS_STATUS_UNSUPPORTED_MEDIA;
        return;
    }

    AdapterBinding = ExAllocatePool(NonPagedPool, sizeof(ADAPTER_BINDING));
    if (!AdapterBinding) {
        NDIS_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
        *Status = NDIS_STATUS_RESOURCES;
        return;
    }

    RtlZeroMemory(AdapterBinding, sizeof(ADAPTER_BINDING));

    AdapterBinding->ProtocolBinding        = Protocol;
    AdapterBinding->Adapter                = Adapter;
    AdapterBinding->NdisOpenBlock.ProtocolBindingContext = ProtocolBindingContext;

    /* Set fields required by some NDIS macros */
    AdapterBinding->NdisOpenBlock.MacBindingHandle = (NDIS_HANDLE)AdapterBinding;
    
    /* Set handlers (some NDIS macros require these) */

    AdapterBinding->NdisOpenBlock.RequestHandler      = ProRequest;
    AdapterBinding->NdisOpenBlock.ResetHandler        = ProReset;
    AdapterBinding->NdisOpenBlock.u1.SendHandler      = ProSend;
    AdapterBinding->NdisOpenBlock.SendPacketsHandler  = ProSendPackets;
    AdapterBinding->NdisOpenBlock.TransferDataHandler = ProTransferData;

#if 0
	 /* XXX this looks fishy */
	 /* OK, this really *is* fishy - it bugchecks */
    /* Put on protocol's bound adapters list */
    ExInterlockedInsertTailList(&Protocol->AdapterListHead,
                                &AdapterBinding->ProtocolListEntry,
                                &Protocol->Lock);
#endif
	 /* XXX so does this */
    /* Put protocol on adapter's bound protocols list */
    ExInterlockedInsertTailList(&Adapter->ProtocolListHead,
                                &AdapterBinding->AdapterListEntry,
                                &Adapter->NdisMiniportBlock.Lock);

    *NdisBindingHandle = (NDIS_HANDLE)AdapterBinding;

    *Status = NDIS_STATUS_SUCCESS;
}


/*
 * @implemented
 */
VOID
EXPORT
NdisRegisterProtocol(
    OUT PNDIS_STATUS                    Status,
    OUT PNDIS_HANDLE                    NdisProtocolHandle,
    IN  PNDIS_PROTOCOL_CHARACTERISTICS  ProtocolCharacteristics,
    IN  UINT                            CharacteristicsLength)
/*
 * FUNCTION: Registers an NDIS driver's ProtocolXxx entry points
 * ARGUMENTS:
 *     Status                  = Address of buffer for status information
 *     NdisProtocolHandle      = Address of buffer for handle used to identify the driver
 *     ProtocolCharacteristics = Pointer to NDIS_PROTOCOL_CHARACTERISTICS structure
 *     CharacteristicsLength   = Size of structure which ProtocolCharacteristics targets
 * TODO:
 *     break this function up
 *     make this thing able to handle >1 protocol
 */
{
    PPROTOCOL_BINDING Protocol;
    NTSTATUS NtStatus;
    UINT MinSize;
    HANDLE DriverKeyHandle = NULL;
    PKEY_VALUE_PARTIAL_INFORMATION KeyInformation = NULL;
	 UINT DataOffset = 0;

    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    switch (ProtocolCharacteristics->MajorNdisVersion) {
    case 0x03:	/* we don't really want to support ndis3 drivers - so we complainf or now */
		  NDIS_DbgPrint(MID_TRACE, ("Ndis 3 protocol attempting to register\n"));
        MinSize = sizeof(NDIS30_PROTOCOL_CHARACTERISTICS_S);
        break;

    case 0x04:
        MinSize = sizeof(NDIS40_PROTOCOL_CHARACTERISTICS_S);
        break;

    case 0x05:
        MinSize = sizeof(NDIS50_PROTOCOL_CHARACTERISTICS_S);
        break;

    default:
        *Status = NDIS_STATUS_BAD_VERSION;
		  NDIS_DbgPrint(MIN_TRACE, ("Incorrect characteristics size\n"));
        return;
    }

    if (CharacteristicsLength < MinSize) {
        NDIS_DbgPrint(DEBUG_PROTOCOL, ("Bad protocol characteristics.\n"));
        *Status = NDIS_STATUS_BAD_CHARACTERISTICS;
        return;
    }

    Protocol = ExAllocatePool(NonPagedPool, sizeof(PROTOCOL_BINDING));
    if (!Protocol) {
        NDIS_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
        *Status = NDIS_STATUS_RESOURCES;
        return;
    }

    RtlZeroMemory(Protocol, sizeof(PROTOCOL_BINDING));
    RtlCopyMemory(&Protocol->Chars, ProtocolCharacteristics, MinSize);

    NtStatus = RtlUpcaseUnicodeString(&Protocol->Chars.Name,
                                      &ProtocolCharacteristics->Name,
                                      TRUE);
    if (!NT_SUCCESS(NtStatus)) {
        NDIS_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
        ExFreePool(Protocol);
        *Status = NDIS_STATUS_RESOURCES;
        return;
    }

    KeInitializeSpinLock(&Protocol->Lock);

    Protocol->RefCount = 1;

    InitializeListHead(&Protocol->AdapterListHead);

	 /* 
	  * bind the adapter to all of its miniports 
	  *
	  * open registry path
	  * get list of devices from Bind key
	  * call BindAdapterHandler for each
	  */
	{
		OBJECT_ATTRIBUTES ObjectAttributes;
		UNICODE_STRING RegistryPath;
		WCHAR *RegistryPathStr;

#define SERVICES_KEY L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\"
#define LINKAGE_KEY  L"\\Linkage"

		RegistryPathStr = ExAllocatePool(PagedPool, 
			sizeof(SERVICES_KEY) + ProtocolCharacteristics->Name.Length + sizeof(LINKAGE_KEY));
		if(!RegistryPathStr)
		{
			NDIS_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
			ExFreePool(Protocol);
			*Status = NDIS_STATUS_RESOURCES;
			return;
		}

		wcscpy(RegistryPathStr, SERVICES_KEY);
		wcsncat(RegistryPathStr, ((WCHAR *)ProtocolCharacteristics->Name.Buffer),
				ProtocolCharacteristics->Name.Length / sizeof(WCHAR));
		RegistryPathStr[wcslen(SERVICES_KEY)+ProtocolCharacteristics->Name.Length/sizeof(WCHAR)] = 0;
		wcscat(RegistryPathStr, LINKAGE_KEY);

		RtlInitUnicodeString(&RegistryPath, RegistryPathStr);
		NDIS_DbgPrint(MAX_TRACE, ("Opening configuration key: %wZ\n", &RegistryPath));

		InitializeObjectAttributes(&ObjectAttributes, &RegistryPath, OBJ_CASE_INSENSITIVE, NULL, NULL);
		NtStatus = ZwOpenKey(&DriverKeyHandle, KEY_READ, &ObjectAttributes);

		ExFreePool(RegistryPathStr);

		if(!NT_SUCCESS(NtStatus))
		{
			NDIS_DbgPrint(MID_TRACE, ("Unable to open protocol configuration\n"));
			ExFreePool(Protocol);
			*Status = NDIS_STATUS_FAILURE;
			return;
		}
	}

	NDIS_DbgPrint(MAX_TRACE, ("Successfully opened the registry configuration\n"));

	{
		UNICODE_STRING ValueName;
		ULONG ResultLength;

		RtlInitUnicodeString(&ValueName, L"Bind");

		NtStatus = ZwQueryValueKey(DriverKeyHandle, &ValueName, KeyValuePartialInformation, NULL, 0, &ResultLength);
		if(NtStatus != STATUS_BUFFER_OVERFLOW && NtStatus != STATUS_BUFFER_TOO_SMALL)
		{
			NDIS_DbgPrint(MID_TRACE, ("Unable to query the Bind value for size\n"));
			ZwClose(DriverKeyHandle);
			ExFreePool(Protocol);
			*Status = NDIS_STATUS_FAILURE;
			return;
		}

		KeyInformation = ExAllocatePool(PagedPool, sizeof(KEY_VALUE_PARTIAL_INFORMATION) + ResultLength);
		if(!KeyInformation)
		{
			NDIS_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
			ZwClose(DriverKeyHandle);
			ExFreePool(Protocol);
			*Status = NDIS_STATUS_FAILURE;
			return;
		}

		NtStatus = ZwQueryValueKey(DriverKeyHandle, &ValueName, KeyValuePartialInformation, KeyInformation,
				sizeof(KEY_VALUE_PARTIAL_INFORMATION) + ResultLength, &ResultLength);

		if(!NT_SUCCESS(NtStatus))
		{
			NDIS_DbgPrint(MIN_TRACE, ("Unable to query the Bind value\n"));
			ZwClose(DriverKeyHandle);
			ExFreePool(KeyInformation);
			ExFreePool(Protocol);
			*Status = NDIS_STATUS_FAILURE;
			return;
		}
	}

	DataOffset = 0;
	while((KeyInformation->Data)[DataOffset])
	{
		/* BindContext is for tracking pending binding operations */
		VOID *BindContext = 0;
		NDIS_STRING DeviceName;
		NDIS_STRING RegistryPath;
		WCHAR *RegistryPathStr = NULL;
		ULONG PathLength = 0;

		RtlInitUnicodeString(&DeviceName, (WCHAR *)KeyInformation->Data);	/* we know this is 0-term */

		/*
		 * RegistryPath should be:
		 *     \Registry\Machine\System\CurrentControlSet\Services\Nic1\Parameters\Tcpip
		 *
		 *  This is constructed as follows:
		 *      SERVICES_KEY + extracted device name + Protocol name from characteristics 
		 */
#define PARAMETERS_KEY L"\\Parameters\\"
		
		PathLength = sizeof(SERVICES_KEY) +                              /* \Registry\Machine\System\CurrentControlSet\Services\ */
			wcslen( ((WCHAR *)KeyInformation->Data)+8 ) * sizeof(WCHAR) + /* Adapter1  (extracted from \Device\Adapter1)          */
			sizeof(PARAMETERS_KEY) +                                      /* \Parameters\                                         */
			ProtocolCharacteristics->Name.Length;                         /* Tcpip                                                */

		RegistryPathStr = ExAllocatePool(PagedPool, PathLength);
		if(!RegistryPathStr)
		{
			NDIS_DbgPrint(MIN_TRACE, ("insufficient resources.\n"));
			ExFreePool(KeyInformation);
			ExFreePool(Protocol);
			*Status = NDIS_STATUS_RESOURCES;
			return;
		}

		wcscpy(RegistryPathStr, SERVICES_KEY);
		wcscat(RegistryPathStr, (((WCHAR *)(KeyInformation->Data)) +8 ));
		wcscat(RegistryPathStr, PARAMETERS_KEY);
		wcsncat(RegistryPathStr, ProtocolCharacteristics->Name.Buffer, ProtocolCharacteristics->Name.Length / sizeof(WCHAR) );

		RegistryPathStr[PathLength/sizeof(WCHAR) - 1] = 0;

		RtlInitUnicodeString(&RegistryPath, RegistryPathStr);

		NDIS_DbgPrint(MAX_TRACE, ("Calling protocol's BindAdapter handler with DeviceName %wZ and RegistryPath %wZ\n",
					&DeviceName, &RegistryPath));

		/* XXX SD must do something with bind context */
		{
			BIND_HANDLER BindHandler = ProtocolCharacteristics->BindAdapterHandler;
			if(BindHandler)
				BindHandler(Status, BindContext, &DeviceName, &RegistryPath, 0);
			else
				NDIS_DbgPrint(MID_TRACE, ("No protocol bind handler specified\n"));
		}
		/*
      (*(Protocol->Chars.BindAdapterHandler))(Status, BindContext, &DeviceName, &RegistryPath, 0);
		*/

		if(*Status == NDIS_STATUS_SUCCESS)
		{
			 /* Put protocol binding struct on global list */
			 ExInterlockedInsertTailList(&ProtocolListHead, &Protocol->ListEntry, &ProtocolListLock);
		}
		/*
		else if(*Status != NDIS_STATUS_PENDING)
		{
			// what to do here?
		}
		*/

		DataOffset += wcslen((WCHAR *)KeyInformation->Data);
	}

    *NdisProtocolHandle = Protocol;
    *Status             = NDIS_STATUS_SUCCESS;
}


/*
 * @implemented
 */
VOID
EXPORT
NdisRequest(
    OUT PNDIS_STATUS    Status,
    IN  NDIS_HANDLE     NdisBindingHandle,
    IN  PNDIS_REQUEST   NdisRequest)
/*
 * FUNCTION: Forwards a request to an NDIS driver
 * ARGUMENTS:
 *     Status            = Address of buffer for status information
 *     NdisBindingHandle = Adapter binding handle
 *     NdisRequest       = Pointer to request to perform
 */
{
    *Status = ProRequest(NdisBindingHandle, NdisRequest);
}


/*
 * @implemented
 */
VOID
EXPORT
NdisReset(
    OUT PNDIS_STATUS    Status,
    IN  NDIS_HANDLE     NdisBindingHandle)
{
    *Status = ProReset(NdisBindingHandle);
}


/*
 * @implemented
 */
VOID
EXPORT
NdisSend(
    OUT PNDIS_STATUS    Status,
    IN  NDIS_HANDLE     NdisBindingHandle,
    IN  PNDIS_PACKET    Packet)
/*
 * FUNCTION: Forwards a request to send a packet
 * ARGUMENTS:
 *     Status             = Address of buffer for status information
 *     NdisBindingHandle  = Adapter binding handle
 *     Packet             = Pointer to NDIS packet descriptor
 */
{
    *Status = ProSend(NdisBindingHandle, Packet);
}


/*
 * @implemented
 */
VOID
EXPORT
NdisSendPackets(
    IN  NDIS_HANDLE     NdisBindingHandle,
    IN  PPNDIS_PACKET   PacketArray,
    IN  UINT            NumberOfPackets)
{
    ProSendPackets(NdisBindingHandle, PacketArray, NumberOfPackets);
}


/*
 * @implemented
 */
VOID
EXPORT
NdisTransferData(
    OUT     PNDIS_STATUS    Status,
    IN      NDIS_HANDLE     NdisBindingHandle,
    IN      NDIS_HANDLE     MacReceiveContext,
    IN      UINT            ByteOffset,
    IN      UINT            BytesToTransfer,
    IN OUT	PNDIS_PACKET    Packet,
    OUT     PUINT           BytesTransferred)
/*
 * FUNCTION: Forwards a request to copy received data into a protocol-supplied packet
 * ARGUMENTS:
 *     Status            = Address of buffer for status information
 *     NdisBindingHandle = Adapter binding handle
 *     MacReceiveContext = MAC receive context
 *     ByteOffset        = Offset in packet to place data
 *     BytesToTransfer   = Number of bytes to copy into packet
 *     Packet            = Pointer to NDIS packet descriptor
 *     BytesTransferred  = Address of buffer to place number of bytes copied
 */
{
    *Status = ProTransferData(NdisBindingHandle,
                              MacReceiveContext,
                              ByteOffset,
                              BytesToTransfer,
                              Packet,
                              BytesTransferred);
}

/* EOF */
