/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Ancillary Function Driver DLL
 * FILE:        misc/dllmain.c
 * PURPOSE:     DLL entry point
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 *				Alex Ionescu (alex@relsoft.net)
 * REVISIONS:
 *   CSH 01/09-2000 Created
 *	 Alex 16/07/2004 - Complete Rewrite
 */
#include <string.h>
#include <msafd.h>
#include <helpers.h>
#include <rosrtl/string.h>

#ifdef DBG
DWORD DebugTraceLevel = DEBUG_ULTRA;
#endif /* DBG */

/* To make the linker happy */
VOID STDCALL KeBugCheck (ULONG	BugCheckCode) {}

HANDLE							GlobalHeap;
WSPUPCALLTABLE					Upcalls;
LPWPUCOMPLETEOVERLAPPEDREQUEST	lpWPUCompleteOverlappedRequest;
ULONG							SocketCount;
PSOCKET_INFORMATION 			*Sockets = NULL;
LIST_ENTRY						SockHelpersListHead = {NULL};

SOCKET 
WSPAPI 
WSPSocket(
	int AddressFamily, 
	int SocketType, 
	int Protocol, 
	LPWSAPROTOCOL_INFOW lpProtocolInfo, 
	GROUP g, 
	DWORD dwFlags, 
	LPINT lpErrno)  
/*
 * FUNCTION: Creates a new socket
 * ARGUMENTS:
 *     af             = Address family
 *     type           = Socket type
 *     protocol       = Protocol type
 *     lpProtocolInfo = Pointer to protocol information
 *     g              = Reserved
 *     dwFlags        = Socket flags
 *     lpErrno        = Address of buffer for error information
 * RETURNS:
 *     Created socket, or INVALID_SOCKET if it could not be created
 */
{
	OBJECT_ATTRIBUTES			Object;
	IO_STATUS_BLOCK				IOSB;
	USHORT						SizeOfPacket;
	ULONG						SizeOfEA;
	PAFD_CREATE_PACKET			AfdPacket;
	HANDLE						Sock;
	PSOCKET_INFORMATION			Socket = NULL;
    PFILE_FULL_EA_INFORMATION	EABuffer = NULL;
	PHELPER_DATA				HelperData;
	PVOID						HelperDLLContext;
	DWORD						HelperEvents;
	DWORD						IOOptions;
	UNICODE_STRING				TransportName;
	UNICODE_STRING				DevName;
	LARGE_INTEGER				GroupData;
	INT                         Status;

	AFD_DbgPrint(MAX_TRACE, ("Creating Socket, getting TDI Name\n"));
	AFD_DbgPrint(MAX_TRACE, ("AddressFamily (%d)  SocketType (%d)  Protocol (%d).\n",
    AddressFamily, SocketType, Protocol));

	/* Get Helper Data and Transport */
	Status = SockGetTdiName (&AddressFamily,
	                         &SocketType,
	                         &Protocol,
	                         g,
	                         dwFlags,
	                         &TransportName,
	                         &HelperDLLContext,
	                         &HelperData,
	                         &HelperEvents);

	/* Check for error */
	if (Status != NO_ERROR) {
	    goto error;
	}

	/* AFD Device Name */
	RtlInitUnicodeString(&DevName, L"\\Device\\Afd\\Endpoint");

	/* Set Socket Data */
	Socket = HeapAlloc(GlobalHeap, 0, sizeof(*Socket));
	RtlZeroMemory(Socket, sizeof(*Socket));
	Socket->RefCount = 2;
	Socket->Handle = -1;
	Socket->SharedData.State = SocketOpen;
	Socket->SharedData.AddressFamily = AddressFamily;
	Socket->SharedData.SocketType = SocketType;
	Socket->SharedData.Protocol = Protocol;
	Socket->HelperContext = HelperDLLContext;
	Socket->HelperData = HelperData;
	Socket->HelperEvents = HelperEvents;
	Socket->LocalAddress = &Socket->WSLocalAddress;
	Socket->SharedData.SizeOfLocalAddress = HelperData->MaxWSAddressLength;
	Socket->RemoteAddress = &Socket->WSRemoteAddress;
	Socket->SharedData.SizeOfRemoteAddress = HelperData->MaxWSAddressLength;
	Socket->SharedData.UseDelayedAcceptance = HelperData->UseDelayedAcceptance;
	Socket->SharedData.CreateFlags = dwFlags;
	Socket->SharedData.CatalogEntryId = lpProtocolInfo->dwCatalogEntryId;
	Socket->SharedData.ServiceFlags1 = lpProtocolInfo->dwServiceFlags1;
	Socket->SharedData.ProviderFlags = lpProtocolInfo->dwProviderFlags;
	Socket->SharedData.GroupID = g;
	Socket->SharedData.GroupType = 0;
	Socket->SharedData.UseSAN = FALSE;
	Socket->SanData = NULL;

	/* Ask alex about this */
	if( Socket->SharedData.SocketType == SOCK_DGRAM )
	    Socket->SharedData.ServiceFlags1 |= XP1_CONNECTIONLESS;

	/* Packet Size */
	SizeOfPacket = TransportName.Length + sizeof(AFD_CREATE_PACKET) + sizeof(WCHAR);

	/* EA Size */
	SizeOfEA = SizeOfPacket + sizeof(FILE_FULL_EA_INFORMATION) + AFD_PACKET_COMMAND_LENGTH;

	/* Set up EA Buffer */
	EABuffer = HeapAlloc(GlobalHeap, 0, (SIZE_T)&SizeOfEA);
	EABuffer->NextEntryOffset = 0;
	EABuffer->Flags = 0;
	EABuffer->EaNameLength = AFD_PACKET_COMMAND_LENGTH;
	RtlCopyMemory (EABuffer->EaName, 
					AfdCommand, 
					AFD_PACKET_COMMAND_LENGTH + 1);
	EABuffer->EaValueLength = SizeOfPacket;
	
	/* Set up AFD Packet */
	AfdPacket = (PAFD_CREATE_PACKET)(EABuffer->EaName + EABuffer->EaNameLength + 1);
	AfdPacket->SizeOfTransportName = TransportName.Length;
	RtlCopyMemory (AfdPacket->TransportName,
					TransportName.Buffer, 
					TransportName.Length + sizeof(WCHAR));
	AfdPacket->GroupID = g;

	/* Set up Endpoint Flags */
	if ((Socket->SharedData.ServiceFlags1 & XP1_CONNECTIONLESS) != 0) {
		if ((SocketType != SOCK_DGRAM) && (SocketType != SOCK_RAW)) {
			goto error;			/* Only RAW or UDP can be Connectionless */
		}
		AfdPacket->EndpointFlags |= AFD_ENDPOINT_CONNECTIONLESS;
	}
	
	if ((Socket->SharedData.ServiceFlags1 & XP1_MESSAGE_ORIENTED) != 0) { 
		if (SocketType == SOCK_STREAM) {
			if ((Socket->SharedData.ServiceFlags1 & XP1_PSEUDO_STREAM) == 0) {
				goto error;		/* The Provider doesn't actually support Message Oriented Streams */
			}
		}
			AfdPacket->EndpointFlags |= AFD_ENDPOINT_MESSAGE_ORIENTED;
	}

	if (SocketType == SOCK_RAW) AfdPacket->EndpointFlags |= AFD_ENDPOINT_RAW;

	if (dwFlags & (WSA_FLAG_MULTIPOINT_C_ROOT | 
		WSA_FLAG_MULTIPOINT_C_LEAF | 
		WSA_FLAG_MULTIPOINT_D_ROOT | 
		WSA_FLAG_MULTIPOINT_D_LEAF)) {
		if ((Socket->SharedData.ServiceFlags1 & XP1_SUPPORT_MULTIPOINT) == 0) {
			goto error;			/* The Provider doesn't actually support Multipoint */
		}
		AfdPacket->EndpointFlags |= AFD_ENDPOINT_MULTIPOINT;
		if (dwFlags & WSA_FLAG_MULTIPOINT_C_ROOT) {
			if (((Socket->SharedData.ServiceFlags1 & XP1_MULTIPOINT_CONTROL_PLANE) == 0) 
				|| ((dwFlags & WSA_FLAG_MULTIPOINT_C_LEAF) != 0)) {
				goto error;		/* The Provider doesn't support Control Planes, or you already gave a leaf */
			}
			AfdPacket->EndpointFlags |= AFD_ENDPOINT_C_ROOT;
		}
		if (dwFlags & WSA_FLAG_MULTIPOINT_D_ROOT) {
			if (((Socket->SharedData.ServiceFlags1 & XP1_MULTIPOINT_DATA_PLANE) == 0) 
				|| ((dwFlags & WSA_FLAG_MULTIPOINT_D_LEAF) != 0)) {
				goto error;		/* The Provider doesn't support Data Planes, or you already gave a leaf */
			}
			AfdPacket->EndpointFlags |= AFD_ENDPOINT_D_ROOT;
		}
	}

    /* Set up Object Attributes */
    InitializeObjectAttributes (&Object,
								&DevName, 
								OBJ_CASE_INSENSITIVE | OBJ_INHERIT, 
								0, 
								0);

	/* Set IO Flag */
    if ((dwFlags & WSA_FLAG_OVERLAPPED) == 0) IOOptions = FILE_SYNCHRONOUS_IO_NONALERT;

	/* Create the Socket */
	ZwCreateFile(&Sock,
			GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
			&Object,
			&IOSB,
			NULL,
			0,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			FILE_OPEN_IF,
			IOOptions,
			EABuffer,
			SizeOfEA);

	/* Save Handle */
	Socket->Handle = (SOCKET)Sock;

	/* Save Group Info */
	if (g != 0) {
		GetSocketInformation(Socket, AFD_INFO_GROUP_ID_TYPE, 0, &GroupData);

		Socket->SharedData.GroupID = GroupData.u.LowPart;
		Socket->SharedData.GroupType = GroupData.u.HighPart;
	}

	/* Get Window Sizes and Save them */
	GetSocketInformation (Socket,
							AFD_INFO_SEND_WINDOW_SIZE, 
							&Socket->SharedData.SizeOfSendBuffer, 
							NULL);
	GetSocketInformation (Socket, 
							AFD_INFO_RECEIVE_WINDOW_SIZE, 
							&Socket->SharedData.SizeOfRecvBuffer, 
							NULL);

	/* Save in Process Sockets List */
	Sockets[SocketCount] = Socket;
	SocketCount ++;

	/* Create the Socket Context */
	CreateContext(Socket);

	/* Notify Winsock */
	Upcalls.lpWPUModifyIFSHandle(1, (SOCKET)Sock, lpErrno);

	/* Return Socket Handle */
	return (SOCKET)Sock;

error:
	AFD_DbgPrint(MID_TRACE,("Ending\n"));

	return 0;
}


INT
WSPAPI
WSPCloseSocket(
  IN  SOCKET s,
  OUT	LPINT lpErrno)
/*
 * FUNCTION: Closes an open socket
 * ARGUMENTS:
 *     s       = Socket descriptor
 *     lpErrno = Address of buffer for error information
 * RETURNS:
 *     NO_ERROR, or SOCKET_ERROR if the socket could not be closed
 */
{
	return 0;
}


INT
WSPAPI
WSPBind(
	SOCKET Handle, 
	struct sockaddr *SocketAddress, 
	int SocketAddressLength, 
	LPINT lpErrno)
/*
 * FUNCTION: Associates a local address with a socket
 * ARGUMENTS:
 *     s       = Socket descriptor
 *     name    = Pointer to local address
 *     namelen = Length of name
 *     lpErrno = Address of buffer for error information
 * RETURNS:
 *     0, or SOCKET_ERROR if the socket could not be bound
 */
{
	IO_STATUS_BLOCK				IOSB;
	PAFD_BIND_DATA				BindData;
	PSOCKET_INFORMATION			Socket = NULL;
	NTSTATUS					Status;
	UCHAR						BindBuffer[0x1A];
	SOCKADDR_INFO				SocketInfo;
	HANDLE                                  SockEvent;

	Status = NtCreateEvent( &SockEvent, GENERIC_READ | GENERIC_WRITE,
				NULL, 1, FALSE );

	if( !NT_SUCCESS(Status) ) return -1;

	/* Get the Socket Structure associate to this Socket*/
	Socket = GetSocketStructure(Handle);

	/* Dynamic Structure...ugh */
	BindData = (PAFD_BIND_DATA)BindBuffer;

	/* Set up Address in TDI Format */
	BindData->Address.TAAddressCount = 1;
	BindData->Address.Address[0].AddressLength = SocketAddressLength - sizeof(SocketAddress->sa_family);
	RtlCopyMemory (&BindData->Address.Address[0].AddressType,
					SocketAddress, 
					SocketAddressLength);
	
	/* Get Address Information */
	Socket->HelperData->WSHGetSockaddrType ((PSOCKADDR)SocketAddress, 
											SocketAddressLength, 
											&SocketInfo);

	/* Set the Share Type */
	if (Socket->SharedData.ExclusiveAddressUse) {
		BindData->ShareType = AFD_SHARE_EXCLUSIVE;
	}
	else if (SocketInfo.EndpointInfo == SockaddrEndpointInfoWildcard) {
		BindData->ShareType = AFD_SHARE_WILDCARD;
	}
	else if (Socket->SharedData.ReuseAddresses) {
		BindData->ShareType = AFD_SHARE_REUSE;
	} else {
		BindData->ShareType = AFD_SHARE_UNIQUE;
	}

	/* Send IOCTL */
	Status = NtDeviceIoControlFile( (HANDLE)Socket->Handle,
					SockEvent,
					NULL,
					NULL,
					&IOSB,
					IOCTL_AFD_BIND,
					BindData,
					0xA + Socket->SharedData.SizeOfLocalAddress, /* Can't figure out a way to calculate this in C*/
					BindData,
					0xA + Socket->SharedData.SizeOfLocalAddress); /* Can't figure out a way to calculate this C */
	
	/* Set up Socket Data */
	Socket->SharedData.State = SocketBound;
	Socket->TdiAddressHandle = (HANDLE)IOSB.Information;

	NtClose( SockEvent );

	return 0;
}

int 
WSPAPI
WSPListen(
	SOCKET Handle, 
	int Backlog, 
	LPINT lpErrno)
{
	IO_STATUS_BLOCK				IOSB;
	AFD_LISTEN_DATA				ListenData;
	PSOCKET_INFORMATION			Socket = NULL;
	HANDLE                                  SockEvent;
	NTSTATUS				Status;

	Status = NtCreateEvent( &SockEvent, GENERIC_READ | GENERIC_WRITE,
				NULL, 1, FALSE );

	if( !NT_SUCCESS(Status) ) return -1;

	/* Get the Socket Structure associate to this Socket*/
	Socket = GetSocketStructure(Handle);

	/* Set Up Listen Structure */
	ListenData.UseSAN = FALSE;
	ListenData.UseDelayedAcceptance = Socket->SharedData.UseDelayedAcceptance;
	ListenData.Backlog = Backlog;

	/* Send IOCTL */
	Status = NtDeviceIoControlFile( (HANDLE)Socket->Handle,
					SockEvent,
					NULL,
					NULL,
					&IOSB,
					IOCTL_AFD_START_LISTEN,
					&ListenData,
					sizeof(ListenData),
					NULL,
					0);

	/* Set to Listening */
	Socket->SharedData.Listening = TRUE;

	NtClose( SockEvent );

	return 0;
}


int
WSPAPI 
WSPSelect(
	int nfds, 
	fd_set *readfds,
	fd_set *writefds, 
	fd_set *exceptfds, 
	struct timeval *timeout, 
	LPINT lpErrno)
{
	IO_STATUS_BLOCK			IOSB;
	PAFD_POLL_INFO			PollInfo;
	NTSTATUS				Status;
	ULONG					HandleCount;
	ULONG					PollBufferSize;
	LARGE_INTEGER			uSec;
	PVOID					PollBuffer;
	ULONG					i, j = 0;
	HANDLE                                  SockEvent;

	Status = NtCreateEvent( &SockEvent, GENERIC_READ | GENERIC_WRITE,
				NULL, 1, FALSE );

	if( !NT_SUCCESS(Status) ) return -1;

	/* Find out how many sockets we have, and how large the buffer needs to be */
	HandleCount = ( readfds ? readfds->fd_count : 0 ) + 
					( writefds ? writefds->fd_count : 0 ) + 
					( exceptfds ? exceptfds->fd_count : 0 );
	PollBufferSize = sizeof(*PollInfo) + (HandleCount * sizeof(AFD_HANDLE));

	/* Allocate */
	PollBuffer = HeapAlloc(GlobalHeap, 0, PollBufferSize);
	PollInfo = (PAFD_POLL_INFO)PollBuffer;

	/* Convert Timeout to NT Format */
	if (timeout == NULL) {
		PollInfo->Timeout.u.LowPart = -1;
		PollInfo->Timeout.u.HighPart = 0x7FFFFFFF;
	} else {
		PollInfo->Timeout = RtlEnlargedIntegerMultiply(timeout->tv_sec, -10000000);
		uSec = RtlEnlargedIntegerMultiply(timeout->tv_usec, -10);
		PollInfo->Timeout.QuadPart += uSec.QuadPart;
	}
	
	/* Number of handles for AFD to Check */
	PollInfo->HandleCount = HandleCount;
	PollInfo->Unknown = 0;

	if (readfds != NULL) {
		for (i = 0; i < readfds->fd_count; i++, j++) {
			PollInfo->Handles[j].Handle = readfds->fd_array[i];
			PollInfo->Handles[j].Events = AFD_EVENT_RECEIVE | AFD_EVENT_DISCONNECT | AFD_EVENT_ABORT;
		} 
	} else if (writefds != NULL) {
		for (i = 0; i < writefds->fd_count; i++, j++) {
			PollInfo->Handles[j].Handle = writefds->fd_array[i];
			PollInfo->Handles[j].Events = AFD_EVENT_SEND;
		}

	} else if (exceptfds != NULL) {
		for (i = 0; i < exceptfds->fd_count; i++, j++) {
			PollInfo->Handles[j].Handle = exceptfds->fd_array[i];
			PollInfo->Handles[j].Events = AFD_EVENT_OOB_RECEIVE | AFD_EVENT_CONNECT_FAIL;
		}
	}

	/* Send IOCTL */
	Status = NtDeviceIoControlFile( (HANDLE)Sockets[0]->Handle,
					SockEvent,
					NULL,
					NULL,
					&IOSB,
					IOCTL_AFD_SELECT,
					PollInfo,
					PollBufferSize,
					PollInfo,
					PollBufferSize);

	/* Wait for Completition */
	if (Status == STATUS_PENDING) {
		WaitForSingleObject(SockEvent, 0);
	}

	/* Clear the Structures */
	readfds ? FD_ZERO(readfds) : 0;
	writefds ? FD_ZERO(writefds) : 0;
	exceptfds ? FD_ZERO(exceptfds) : 0;

	/* Loop through return structure */
	HandleCount = PollInfo->HandleCount;

	/* Return in FDSET Format */
	for (i = 0; i < HandleCount; i++) {
		switch (PollInfo->Handles[i].Events) {

			case AFD_EVENT_RECEIVE: 
			case AFD_EVENT_DISCONNECT: 
			case AFD_EVENT_ABORT: 
			case AFD_EVENT_ACCEPT: 
			case AFD_EVENT_CLOSE:
				FD_SET(PollInfo->Handles[i].Handle, readfds);
				break;

			case AFD_EVENT_SEND: case AFD_EVENT_CONNECT:
				FD_SET(PollInfo->Handles[i].Handle, writefds);
				break;

			case AFD_EVENT_OOB_RECEIVE: case AFD_EVENT_CONNECT_FAIL:
				FD_SET(PollInfo->Handles[i].Handle, exceptfds);
				break;
		}
	}

	NtClose( SockEvent );

	return 0;
}

SOCKET
WSPAPI 
WSPAccept(
	SOCKET Handle, 
	struct sockaddr *SocketAddress, 
	int *SocketAddressLength, 
	LPCONDITIONPROC lpfnCondition, 
	DWORD_PTR dwCallbackData, 
	LPINT lpErrno)
{
	IO_STATUS_BLOCK				IOSB;
	PAFD_RECEIVED_ACCEPT_DATA	ListenReceiveData;
	AFD_ACCEPT_DATA				AcceptData;
	AFD_DEFER_ACCEPT_DATA		DeferData;
	AFD_PENDING_ACCEPT_DATA		PendingAcceptData;
	PSOCKET_INFORMATION			Socket = NULL;
	NTSTATUS					Status;
	struct fd_set				ReadSet;
	struct timeval				Timeout;
	PVOID						PendingData;
	ULONG						PendingDataLength;
	PVOID						CalleeDataBuffer;
	WSABUF						CallerData, CalleeID, CallerID, CalleeData;
	PSOCKADDR					RemoteAddress =  NULL;
	GROUP						GroupID = 0;
	ULONG						CallBack;
	WSAPROTOCOL_INFOW			ProtocolInfo;
	SOCKET						AcceptSocket;
	UCHAR						ReceiveBuffer[0x1A];
	HANDLE                                  SockEvent;

	Status = NtCreateEvent( &SockEvent, GENERIC_READ | GENERIC_WRITE,
				NULL, 1, FALSE );

	if( !NT_SUCCESS(Status) ) return -1;

	/* Dynamic Structure...ugh */
	ListenReceiveData = (PAFD_RECEIVED_ACCEPT_DATA)ReceiveBuffer;

	/* Get the Socket Structure associate to this Socket*/
	Socket = GetSocketStructure(Handle);

	/* If this is non-blocking, make sure there's something for us to accept */
	FD_ZERO(&ReadSet);
	FD_SET(Socket->Handle, &ReadSet);
	Timeout.tv_sec=0;
	Timeout.tv_usec=0;
	WSPSelect(0, &ReadSet, NULL, NULL, &Timeout, NULL);
	if (ReadSet.fd_array[0] != Socket->Handle) return 0;

	/* Send IOCTL */
	Status = NtDeviceIoControlFile( (HANDLE)Socket->Handle,
					SockEvent,
					NULL,
					NULL,
					&IOSB,
					IOCTL_AFD_WAIT_FOR_LISTEN,
					NULL,
					0,
					ListenReceiveData,
					0xA + sizeof(*ListenReceiveData));
	
	if (lpfnCondition != NULL) {
		if ((Socket->SharedData.ServiceFlags1 & XP1_CONNECT_DATA) != 0) {

			/* Find out how much data is pending */
			PendingAcceptData.SequenceNumber = ListenReceiveData->SequenceNumber;
			PendingAcceptData.ReturnSize = TRUE;

			/* Send IOCTL */
			Status = NtDeviceIoControlFile( (HANDLE)Socket->Handle,
							SockEvent,
							NULL,
							NULL,
							&IOSB,
							IOCTL_AFD_GET_PENDING_CONNECT_DATA,
							&PendingAcceptData,
							sizeof(PendingAcceptData),
							&PendingAcceptData,
							sizeof(PendingAcceptData));

			/* How much data to allocate */
			PendingDataLength = IOSB.Information;

			if (PendingDataLength) {

				/* Allocate needed space */
				PendingData = HeapAlloc(GlobalHeap, 0, PendingDataLength);

				/* We want the data now */
				PendingAcceptData.ReturnSize = FALSE;

				/* Send IOCTL */
				Status = NtDeviceIoControlFile( (HANDLE)Socket->Handle,
								SockEvent,
								NULL,
								NULL,
								&IOSB,
								IOCTL_AFD_GET_PENDING_CONNECT_DATA,
								&PendingAcceptData,
								sizeof(PendingAcceptData),
								PendingData,
								PendingDataLength);
			}
		}

		
		if ((Socket->SharedData.ServiceFlags1 & XP1_QOS_SUPPORTED) != 0) {
			/* I don't support this yet */
		}
		
		/* Build Callee ID */
		CalleeID.buf = (PVOID)Socket->LocalAddress;
		CalleeID.len = Socket->SharedData.SizeOfLocalAddress;

		/* Set up Address in SOCKADDR Format */
		RtlCopyMemory (RemoteAddress, 
						&ListenReceiveData->Address.Address[0].AddressType, 
						sizeof(RemoteAddress));

		/* Build Caller ID */
		CallerID.buf = (PVOID)RemoteAddress;
		CallerID.len = sizeof(RemoteAddress);

		/* Build Caller Data */
		CallerData.buf = PendingData;
		CallerData.len = PendingDataLength;

		/* Check if socket supports Conditional Accept */
		if (Socket->SharedData.UseDelayedAcceptance != 0) {
			
			/* Allocate Buffer for Callee Data */
			CalleeDataBuffer = HeapAlloc(GlobalHeap, 0, 4096);
			CalleeData.buf = CalleeDataBuffer;
			CalleeData.len = 4096;

		} else {

			/* Nothing */
			CalleeData.buf = 0;
			CalleeData.len = 0;
		}
	
		/* Call the Condition Function */
		CallBack = (lpfnCondition)( &CallerID,
						CallerData.buf == NULL
						? NULL
						: & CallerData,
						NULL,
						NULL,
						&CalleeID,
						CalleeData.buf == NULL
						? NULL
						: & CalleeData,
						&GroupID,
						dwCallbackData);

		if (((CallBack == CF_ACCEPT) && GroupID) != 0) {
			/* TBD: Check for Validity */
		}

		if (CallBack == CF_ACCEPT) {

			if ((Socket->SharedData.ServiceFlags1 & XP1_QOS_SUPPORTED) != 0) {
				/* I don't support this yet */
			}

			if (CalleeData.buf) {
				// SockSetConnectData Sockets(SocketID), IOCTL_AFD_SET_CONNECT_DATA, CalleeData.Buffer, CalleeData.BuffSize, 0
			}
		
		} else {
			/* Callback rejected. Build Defer Structure */
			DeferData.SequenceNumber = ListenReceiveData->SequenceNumber;
			DeferData.RejectConnection = (CallBack == CF_REJECT);

			/* Send IOCTL */
			Status = NtDeviceIoControlFile( (HANDLE)Socket->Handle,
							SockEvent,
							NULL,
							NULL,
							&IOSB,
							IOCTL_AFD_DEFER_ACCEPT,
							&DeferData,
							sizeof(DeferData),
							NULL,
							0);

			NtClose( SockEvent );

			if (CallBack == CF_REJECT ) {
				return WSAECONNREFUSED;
			} else {
				return WSATRY_AGAIN;
			}
		}
	}
	
	/* Create a new Socket */
	ProtocolInfo.dwCatalogEntryId = Socket->SharedData.CatalogEntryId;
	ProtocolInfo.dwServiceFlags1 = Socket->SharedData.ServiceFlags1;
	ProtocolInfo.dwProviderFlags = Socket->SharedData.ProviderFlags;
	AcceptSocket = WSPSocket (Socket->SharedData.AddressFamily,
								Socket->SharedData.SocketType, 
								Socket->SharedData.Protocol, 
								&ProtocolInfo,
								GroupID, 
								Socket->SharedData.CreateFlags, 
								NULL);

	/* Set up the Accept Structure */
   	AcceptData.ListenHandle = AcceptSocket;
	AcceptData.SequenceNumber = ListenReceiveData->SequenceNumber;
    
	/* Send IOCTL to Accept */
	Status = NtDeviceIoControlFile( (HANDLE)Socket->Handle,
					SockEvent,
					NULL,
					NULL,
					&IOSB,
					IOCTL_AFD_ACCEPT,
					&AcceptData,
					sizeof(AcceptData),
					NULL,
					0);
	
	/* Return Address in SOCKADDR FORMAT */
	RtlCopyMemory (SocketAddress, 
					&ListenReceiveData->Address.Address[0].AddressType, 
					sizeof(RemoteAddress));

	NtClose( SockEvent );

	/* Return Socket */
	return AcceptSocket;
}

int
WSPAPI 
WSPConnect(
	SOCKET Handle, 
	struct sockaddr * SocketAddress, 
	int SocketAddressLength, 
	LPWSABUF lpCallerData, 
	LPWSABUF lpCalleeData, 
	LPQOS lpSQOS, 
	LPQOS lpGQOS, 
	LPINT lpErrno)
{
	IO_STATUS_BLOCK				IOSB;
	PAFD_CONNECT_INFO			ConnectInfo;
	PSOCKET_INFORMATION			Socket = NULL;
	NTSTATUS					Status;
	UCHAR						ConnectBuffer[0x22];
	ULONG						ConnectDataLength;
	ULONG						InConnectDataLength;
	UINT						BindAddressLength;
	PSOCKADDR					BindAddress;
	HANDLE                                  SockEvent;

	Status = NtCreateEvent( &SockEvent, GENERIC_READ | GENERIC_WRITE,
				NULL, 1, FALSE );

	if( !NT_SUCCESS(Status) ) return -1;

	AFD_DbgPrint(MID_TRACE,("Called\n"));

	/* Get the Socket Structure associate to this Socket*/
	Socket = GetSocketStructure(Handle);

	/* Bind us First */
	if (Socket->SharedData.State == SocketOpen) {
		
		/* Get the Wildcard Address */
		BindAddressLength = Socket->HelperData->MaxWSAddressLength;
		BindAddress = HeapAlloc(GetProcessHeap(), 0, BindAddressLength);
		Socket->HelperData->WSHGetWildcardSockaddr (Socket->HelperContext, 
													BindAddress, 
													&BindAddressLength);

		/* Bind it */
		WSPBind(Handle, BindAddress, BindAddressLength, NULL);
	}

	/* Set the Connect Data */
	if (lpCallerData != NULL) {
		ConnectDataLength = lpCallerData->len;
		Status = NtDeviceIoControlFile((HANDLE)Handle,
										SockEvent,
										NULL,
										NULL,
										&IOSB,
										IOCTL_AFD_SET_CONNECT_DATA,
										lpCallerData->buf,
										ConnectDataLength,
										NULL,
										0);
	}

	/* Dynamic Structure...ugh */
	ConnectInfo = (PAFD_CONNECT_INFO)ConnectBuffer;

	/* Set up Address in TDI Format */
	ConnectInfo->RemoteAddress.TAAddressCount = 1;
	ConnectInfo->RemoteAddress.Address[0].AddressLength = SocketAddressLength - sizeof(SocketAddress->sa_family);
	RtlCopyMemory (&ConnectInfo->RemoteAddress.Address[0].AddressType, 
					SocketAddress, 
					SocketAddressLength);

	/* Tell AFD that we want Connection Data back, have it allocate a buffer */
	if (lpCalleeData != NULL) {
		InConnectDataLength = lpCalleeData->len;
		Status = NtDeviceIoControlFile((HANDLE)Handle,
										SockEvent,
										NULL,
										NULL,
										&IOSB,
										IOCTL_AFD_SET_CONNECT_DATA_SIZE,
										&InConnectDataLength,
										sizeof(InConnectDataLength),
										NULL,
										0);
	}

	/* AFD doesn't seem to care if these are invalid, but let's 0 them anyways */
	ConnectInfo->Root = 0;
	ConnectInfo->UseSAN = FALSE;
	ConnectInfo->Unknown = 0;

	/* Send IOCTL */
	Status = NtDeviceIoControlFile((HANDLE)Handle,
									SockEvent,
									NULL,
									NULL,
									&IOSB,
									IOCTL_AFD_CONNECT,
									ConnectInfo,
									0x22,
									NULL,
									0);

	/* Get any pending connect data */
	 if (lpCalleeData != NULL) {
		Status = NtDeviceIoControlFile((HANDLE)Handle,
										SockEvent,
										NULL,
										NULL,
										&IOSB,
										IOCTL_AFD_GET_CONNECT_DATA,
										NULL,
										0,
										lpCalleeData->buf,
										lpCalleeData->len);
	 }

	AFD_DbgPrint(MID_TRACE,("Ending\n"));

	NtClose( SockEvent );

	return Status;
}
int
WSPAPI 
WSPShutdown(
	SOCKET Handle, 
	int HowTo, 
	LPINT lpErrno)

{
	IO_STATUS_BLOCK				IOSB;
	AFD_DISCONNECT_INFO			DisconnectInfo;
	PSOCKET_INFORMATION			Socket = NULL;
	NTSTATUS					Status;
	HANDLE                                  SockEvent;

	Status = NtCreateEvent( &SockEvent, GENERIC_READ | GENERIC_WRITE,
				NULL, 1, FALSE );

	if( !NT_SUCCESS(Status) ) return -1;

	AFD_DbgPrint(MID_TRACE,("Called\n"));

	/* Get the Socket Structure associate to this Socket*/
	Socket = GetSocketStructure(Handle);

	/* Set AFD Disconnect Type */
    switch (HowTo) {

		case SD_RECEIVE:
			DisconnectInfo.DisconnectType = AFD_DISCONNECT_RECV;
			Socket->SharedData.ReceiveShutdown = TRUE;
			break;
		
		case SD_SEND:
	        DisconnectInfo.DisconnectType= AFD_DISCONNECT_SEND;
			Socket->SharedData.SendShutdown = TRUE;
			break;

		case SD_BOTH:
			DisconnectInfo.DisconnectType = AFD_DISCONNECT_RECV | AFD_DISCONNECT_SEND;
			Socket->SharedData.ReceiveShutdown = TRUE;
			Socket->SharedData.SendShutdown = TRUE;
			break;
    }

    DisconnectInfo.Timeout = RtlConvertLongToLargeInteger(-1);

	/* Send IOCTL */
	Status = NtDeviceIoControlFile((HANDLE)Handle,
									SockEvent,
									NULL,
									NULL,
									&IOSB,
									IOCTL_AFD_DISCONNECT,
									&DisconnectInfo,
									sizeof(DisconnectInfo),
									NULL,
									0);

	/* Wait for return */
	if (Status == STATUS_PENDING) {
		WaitForSingleObject(SockEvent, 0);
	}

	AFD_DbgPrint(MID_TRACE,("Ending\n"));

	NtClose( SockEvent );

	return 0;
}


INT
WSPAPI
WSPGetSockName(
    IN      SOCKET s,
    OUT     LPSOCKADDR name,
    IN OUT  LPINT namelen,
    OUT     LPINT lpErrno)
{
  return 0;
}


INT
WSPAPI
WSPGetPeerName(
    IN      SOCKET s, 
    OUT     LPSOCKADDR name, 
    IN OUT  LPINT namelen, 
    OUT     LPINT lpErrno)
{
 
  return 0;
}

INT
WSPAPI
WSPStartup(
  IN  WORD wVersionRequested,
  OUT LPWSPDATA lpWSPData,
  IN  LPWSAPROTOCOL_INFOW lpProtocolInfo,
  IN  WSPUPCALLTABLE UpcallTable,
  OUT LPWSPPROC_TABLE lpProcTable)
/*
 * FUNCTION: Initialize service provider for a client
 * ARGUMENTS:
 *     wVersionRequested = Highest WinSock SPI version that the caller can use
 *     lpWSPData         = Address of WSPDATA structure to initialize
 *     lpProtocolInfo    = Pointer to structure that defines the desired protocol
 *     UpcallTable       = Pointer to upcall table of the WinSock DLL
 *     lpProcTable       = Address of procedure table to initialize
 * RETURNS:
 *     Status of operation
 */
{
  NTSTATUS Status;

  AFD_DbgPrint(MAX_TRACE, ("wVersionRequested (0x%X) \n", wVersionRequested));

  Status = NO_ERROR;

  Upcalls = UpcallTable;

  if (Status == NO_ERROR) {
    lpProcTable->lpWSPAccept = WSPAccept;
    lpProcTable->lpWSPAddressToString = WSPAddressToString;
    lpProcTable->lpWSPAsyncSelect = WSPAsyncSelect;
    lpProcTable->lpWSPBind = (LPWSPBIND)WSPBind;
    lpProcTable->lpWSPCancelBlockingCall = WSPCancelBlockingCall;
    lpProcTable->lpWSPCleanup = WSPCleanup;
    lpProcTable->lpWSPCloseSocket = WSPCloseSocket;
    lpProcTable->lpWSPConnect = (LPWSPCONNECT)WSPConnect;
    lpProcTable->lpWSPDuplicateSocket = WSPDuplicateSocket;
    lpProcTable->lpWSPEnumNetworkEvents = WSPEnumNetworkEvents;
    lpProcTable->lpWSPEventSelect = WSPEventSelect;
    lpProcTable->lpWSPGetOverlappedResult = WSPGetOverlappedResult;
    lpProcTable->lpWSPGetPeerName = WSPGetPeerName;
    lpProcTable->lpWSPGetSockName = WSPGetSockName;
    lpProcTable->lpWSPGetSockOpt = WSPGetSockOpt;
    lpProcTable->lpWSPGetQOSByName = WSPGetQOSByName;
    lpProcTable->lpWSPIoctl = WSPIoctl;
    lpProcTable->lpWSPJoinLeaf = (LPWSPJOINLEAF)WSPJoinLeaf;
    lpProcTable->lpWSPListen = WSPListen;
    lpProcTable->lpWSPRecv = WSPRecv;
    lpProcTable->lpWSPRecvDisconnect = WSPRecvDisconnect;
    lpProcTable->lpWSPRecvFrom = WSPRecvFrom;
    lpProcTable->lpWSPSelect = WSPSelect;
    lpProcTable->lpWSPSend = WSPSend;
    lpProcTable->lpWSPSendDisconnect = WSPSendDisconnect;
    lpProcTable->lpWSPSendTo = (LPWSPSENDTO)WSPSendTo;
    lpProcTable->lpWSPSetSockOpt = WSPSetSockOpt;
    lpProcTable->lpWSPShutdown = WSPShutdown;
    lpProcTable->lpWSPSocket = WSPSocket;
    lpProcTable->lpWSPStringToAddress = WSPStringToAddress;

    lpWSPData->wVersion     = MAKEWORD(2, 2);
    lpWSPData->wHighVersion = MAKEWORD(2, 2);
  }

  AFD_DbgPrint(MAX_TRACE, ("Status (%d).\n", Status));

  return Status;
}


INT
WSPAPI
WSPCleanup(
  OUT LPINT lpErrno)
/*
 * FUNCTION: Cleans up service provider for a client
 * ARGUMENTS:
 *     lpErrno = Address of buffer for error information
 * RETURNS:
 *     0 if successful, or SOCKET_ERROR if not
 */
{
  AFD_DbgPrint(MAX_TRACE, ("\n"));

 
  AFD_DbgPrint(MAX_TRACE, ("Leaving.\n"));

  *lpErrno = NO_ERROR;

  return 0;
}



int 
GetSocketInformation(
	PSOCKET_INFORMATION Socket, 
	ULONG AfdInformationClass, 
	PULONG Ulong OPTIONAL, 
	PLARGE_INTEGER LargeInteger OPTIONAL)
{
	IO_STATUS_BLOCK				IOSB;
	AFD_INFO					InfoData;
	NTSTATUS					Status;
	HANDLE                                  SockEvent;

	Status = NtCreateEvent( &SockEvent, GENERIC_READ | GENERIC_WRITE,
				NULL, 1, FALSE );

	if( !NT_SUCCESS(Status) ) return -1;

	/* Set Info Class */
	InfoData.InformationClass = AfdInformationClass;

	/* Send IOCTL */
	Status = NtDeviceIoControlFile( (HANDLE)Socket->Handle,
					SockEvent,
					NULL,
					NULL,
					&IOSB,
					IOCTL_AFD_GET_INFO,
					&InfoData,
					sizeof(InfoData),
					&InfoData,
					sizeof(InfoData));

	/* Wait for return */
	if (Status == STATUS_PENDING) {
		WaitForSingleObject(SockEvent, 0);
	}

	/* Return Information */
	*Ulong = InfoData.Information.Ulong;
	if (LargeInteger != NULL) {
		*LargeInteger = InfoData.Information.LargeInteger;
	}

	NtClose( SockEvent );

	return 0;

}


int 
SetSocketInformation(
	PSOCKET_INFORMATION Socket, 
	ULONG AfdInformationClass, 
	PULONG Ulong OPTIONAL, 
	PLARGE_INTEGER LargeInteger OPTIONAL)
{
	IO_STATUS_BLOCK				IOSB;
	AFD_INFO					InfoData;
	NTSTATUS					Status;
	HANDLE                                  SockEvent;

	Status = NtCreateEvent( &SockEvent, GENERIC_READ | GENERIC_WRITE,
				NULL, 1, FALSE );

	if( !NT_SUCCESS(Status) ) return -1;

	/* Set Info Class */
	InfoData.InformationClass = AfdInformationClass;

	/* Set Information */
	InfoData.Information.Ulong = *Ulong;
	if (LargeInteger != NULL) {
		InfoData.Information.LargeInteger = *LargeInteger;
	}

	/* Send IOCTL */
	Status = NtDeviceIoControlFile( (HANDLE)Socket->Handle,
					SockEvent,
					NULL,
					NULL,
					&IOSB,
					IOCTL_AFD_GET_INFO,
					&InfoData,
					sizeof(InfoData),
					NULL,
					0);

	/* Wait for return */
	if (Status == STATUS_PENDING) {
		WaitForSingleObject(SockEvent, 0);
	}

	NtClose( SockEvent );

	return 0;

}

PSOCKET_INFORMATION 
GetSocketStructure(
	SOCKET Handle)
{
	ULONG	i;

	for (i=0; i<SocketCount; i++) {
		if (Sockets[i]->Handle == Handle) {
			return Sockets[i];
		}
	}
	return 0;
}

int CreateContext(PSOCKET_INFORMATION Socket)
{
	IO_STATUS_BLOCK				IOSB;
	SOCKET_CONTEXT				ContextData;
	NTSTATUS				Status;
	HANDLE                                  SockEvent;

	Status = NtCreateEvent( &SockEvent, GENERIC_READ | GENERIC_WRITE,
				NULL, 1, FALSE );

	if( !NT_SUCCESS(Status) ) return -1;

	/* Create Context */
	ContextData.SharedData = Socket->SharedData;
	ContextData.SizeOfHelperData = 0;
	RtlCopyMemory (&ContextData.LocalAddress, 
					Socket->LocalAddress, 
					Socket->SharedData.SizeOfLocalAddress);
	RtlCopyMemory (&ContextData.RemoteAddress, 
					Socket->RemoteAddress, 
					Socket->SharedData.SizeOfRemoteAddress);

	/* Send IOCTL */
	Status = NtDeviceIoControlFile( (HANDLE)Socket->Handle,
					SockEvent,
					NULL,
					NULL,
					&IOSB,
					IOCTL_AFD_SET_CONTEXT,
					&ContextData,
					sizeof(ContextData),
					NULL,
					0);

	/* Wait for Completition */
	if (Status == STATUS_PENDING) {
		WaitForSingleObject(SockEvent, 0);
	}
	
	NtClose( SockEvent );

	return 0;
}

BOOL
STDCALL
DllMain(HANDLE hInstDll,
        ULONG dwReason,
        PVOID Reserved)
{
  
    switch (dwReason) {
    case DLL_PROCESS_ATTACH:

		AFD_DbgPrint(MAX_TRACE, ("Loading MSAFD.DLL \n"));

        /* Don't need thread attach notifications
           so disable them to improve performance */
        DisableThreadLibraryCalls(hInstDll);

		/* List of DLL Helpers */
        InitializeListHead(&SockHelpersListHead);

		/* Heap to use when allocating */
        GlobalHeap = GetProcessHeap();

		/* Allocate Heap for 1024 Sockets, can be expanded later */
		Sockets = HeapAlloc(GetProcessHeap(), 0, sizeof(PSOCKET_INFORMATION) * 1024);

		AFD_DbgPrint(MAX_TRACE, ("MSAFD.DLL has been loaded\n"));

        break;

    case DLL_THREAD_ATTACH:
        break;

    case DLL_THREAD_DETACH:
        break;

    case DLL_PROCESS_DETACH:
        break;
    }

    AFD_DbgPrint(MAX_TRACE, ("DllMain of msafd.dll (leaving)\n"));

    return TRUE;
}

/* EOF */


