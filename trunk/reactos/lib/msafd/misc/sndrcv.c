/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Ancillary Function Driver DLL
 * FILE:        misc/sndrcv.c
 * PURPOSE:     Send/receive routines
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 *				Alex Ionescu (alex@relsoft.net)
 * REVISIONS:
 *   CSH 01/09-2000 Created
 *	 Alex 16/07/2004 - Complete Rewrite
 */
#include <string.h>
#include <msafd.h>

INT
WSPAPI
WSPAsyncSelect(
    IN  SOCKET s, 
    IN  HWND hWnd, 
    IN  UINT wMsg, 
    IN  LONG lEvent, 
    OUT LPINT lpErrno)
{
  UNIMPLEMENTED

  return 0;
}


int 
WSPAPI
WSPRecv(
	SOCKET Handle, 
	LPWSABUF lpBuffers, 
	DWORD dwBufferCount, 
	LPDWORD lpNumberOfBytesRead, 
	LPDWORD ReceiveFlags, 
	LPWSAOVERLAPPED lpOverlapped, 
	LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine, 
	LPWSATHREADID lpThreadId, 
	LPINT lpErrno)
{
	PIO_STATUS_BLOCK			IOSB;
	IO_STATUS_BLOCK				DummyIOSB;
	AFD_RECV_INFO				RecvInfo;
	NTSTATUS					Status;
	PVOID						APCContext;
	PVOID						APCFunction;
	HANDLE						Event;

    /* Set up the Receive Structure */
    RecvInfo.BufferArray = (PAFD_WSABUF)lpBuffers;
    RecvInfo.BufferCount = dwBufferCount;
	RecvInfo.TdiFlags = 0;
	RecvInfo.AfdFlags = 0;

	/* Set the TDI Flags */
	if (*ReceiveFlags == 0) {
		RecvInfo.TdiFlags |= TDI_RECEIVE_NORMAL;
	
	} else {
	
		if (*ReceiveFlags & MSG_OOB) {
			RecvInfo.TdiFlags |= TDI_RECEIVE_EXPEDITED;
		} else {
			RecvInfo.TdiFlags |= TDI_RECEIVE_NORMAL;
		}

		if (*ReceiveFlags & MSG_PEEK) {
			RecvInfo.TdiFlags |= TDI_RECEIVE_PEEK;
		}

		if (*ReceiveFlags & MSG_PARTIAL) {
			RecvInfo.TdiFlags |= TDI_RECEIVE_NORMAL;
		}
	}

	/* Verifiy if we should use APC */

	if (lpOverlapped == NULL) {

		/* Not using Overlapped structure, so use normal blocking on event */
		APCContext = NULL;
		APCFunction = NULL;
		Event = SockEvent;
		IOSB = &DummyIOSB;

	} else {

		if (lpCompletionRoutine == NULL) {

			/* Using Overlapped Structure, but no Completition Routine, so no need for APC */
			APCContext = lpOverlapped;
			APCFunction = NULL;
			Event = lpOverlapped->hEvent;
		
		} else {

			/* Using Overlapped Structure and a Completition Routine, so use an APC */
			APCFunction = NULL; // should be a private io completition function inside us
			APCContext = lpCompletionRoutine;
			RecvInfo.AfdFlags = AFD_SKIP_FIO;
		}

		IOSB = (PIO_STATUS_BLOCK)&lpOverlapped->Internal;
		RecvInfo.AfdFlags |= AFD_OVERLAPPED;
	}

	IOSB->Status = STATUS_PENDING;

	/* Send IOCTL */
	Status = NtDeviceIoControlFile((HANDLE)Handle,
					SockEvent,
					APCFunction,
					APCContext,
					IOSB,
					IOCTL_AFD_RECV,
					&RecvInfo,
					sizeof(RecvInfo),
					NULL,
					0);

	/* Wait for completition of not overlapped */
	if (Status == STATUS_PENDING && lpOverlapped == NULL) {
		WaitForSingleObject(SockEvent, 0); // BUGBUG, shouldn wait infintely for receive...
		Status = IOSB->Status;
	}

	/* Return the Flags */
    	*ReceiveFlags = 0;
    switch (Status) {
        
		case STATUS_SUCCESS:
            	break;

        case STATUS_PENDING :
        	return WSA_IO_PENDING;

	case STATUS_BUFFER_OVERFLOW:
           	return WSAEMSGSIZE;

        case STATUS_RECEIVE_EXPEDITED:
            	*ReceiveFlags = MSG_OOB;
           	 break;

	case STATUS_RECEIVE_PARTIAL_EXPEDITED :
            	*ReceiveFlags = MSG_PARTIAL | MSG_OOB;
           	 break;

        case STATUS_RECEIVE_PARTIAL :
            	*ReceiveFlags = MSG_PARTIAL;
            	break;
	}

	/* Return Number of bytes Read */
    	*lpNumberOfBytesRead = (DWORD)IOSB->Information;

	/* Success */
	return STATUS_SUCCESS;
}

int 
WSPAPI 
WSPRecvFrom(
	SOCKET Handle, 
	LPWSABUF lpBuffers, 
	DWORD dwBufferCount, 
	LPDWORD lpNumberOfBytesRead, 
	LPDWORD ReceiveFlags, 
	struct sockaddr *SocketAddress, 
	int *SocketAddressLength, 
	LPWSAOVERLAPPED lpOverlapped, 
	LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine, 
	LPWSATHREADID lpThreadId, 
	LPINT lpErrno)
{
	PIO_STATUS_BLOCK			IOSB;
	IO_STATUS_BLOCK				DummyIOSB;
	AFD_RECV_INFO_UDP			RecvInfo;
	NTSTATUS					Status;
	PVOID						APCContext;
	PVOID						APCFunction;
	HANDLE						Event;

    /* Set up the Receive Structure */
    RecvInfo.BufferArray = (PAFD_WSABUF)lpBuffers;
    RecvInfo.BufferCount = dwBufferCount;
	RecvInfo.TdiFlags = 0;
	RecvInfo.AfdFlags = 0;
	RecvInfo.AddressLength = SocketAddressLength;
	RecvInfo.Address = SocketAddress;

	/* Set the TDI Flags */
	if (*ReceiveFlags == 0) {
		RecvInfo.TdiFlags |= TDI_RECEIVE_NORMAL;
	
	} else {
	
		if (*ReceiveFlags & MSG_OOB) {
			RecvInfo.TdiFlags |= TDI_RECEIVE_EXPEDITED;
		} else {
			RecvInfo.TdiFlags |= TDI_RECEIVE_NORMAL;
		}

		if (*ReceiveFlags & MSG_PEEK) {
			RecvInfo.TdiFlags |= TDI_RECEIVE_PEEK;
		}

		if (*ReceiveFlags & MSG_PARTIAL) {
			RecvInfo.TdiFlags |= TDI_RECEIVE_NORMAL;
		}
	}

	/* Verifiy if we should use APC */

	if (lpOverlapped == NULL) {

		/* Not using Overlapped structure, so use normal blocking on event */
		APCContext = NULL;
		APCFunction = NULL;
		Event = SockEvent;
		IOSB = &DummyIOSB;

	} else {

		if (lpCompletionRoutine == NULL) {

			/* Using Overlapped Structure, but no Completition Routine, so no need for APC */
			APCContext = lpOverlapped;
			APCFunction = NULL;
			Event = lpOverlapped->hEvent;
		
		} else {

			/* Using Overlapped Structure and a Completition Routine, so use an APC */
			APCFunction = NULL; // should be a private io completition function inside us
			APCContext = lpCompletionRoutine;
			RecvInfo.AfdFlags = AFD_SKIP_FIO;
		}

		IOSB = (PIO_STATUS_BLOCK)&lpOverlapped->Internal;
		RecvInfo.AfdFlags |= AFD_OVERLAPPED;
	}

	IOSB->Status = STATUS_PENDING;

	/* Send IOCTL */
	Status = NtDeviceIoControlFile((HANDLE)Handle,
					SockEvent,
					APCFunction,
					APCContext,
					IOSB,
					IOCTL_AFD_RECV_DATAGRAM,
					&RecvInfo,
					sizeof(RecvInfo),
					NULL,
					0);

	/* Wait for completition of not overlapped */
	if (Status == STATUS_PENDING && lpOverlapped == NULL) {
		WaitForSingleObject(SockEvent, 0); // BUGBUG, shouldn wait infintely for receive...
		Status = IOSB->Status;
	}

	/* Return the Flags */
    	*ReceiveFlags = 0;
    switch (Status) {
        
		case STATUS_SUCCESS:
            	break;

        case STATUS_PENDING :
        	return WSA_IO_PENDING;

		case STATUS_BUFFER_OVERFLOW:
           	return WSAEMSGSIZE;

        case STATUS_RECEIVE_EXPEDITED:
            	*ReceiveFlags = MSG_OOB;
           	 break;

		case STATUS_RECEIVE_PARTIAL_EXPEDITED :
            	*ReceiveFlags = MSG_PARTIAL | MSG_OOB;
           	 break;

        case STATUS_RECEIVE_PARTIAL :
            	*ReceiveFlags = MSG_PARTIAL;
            	break;
	}

	/* Return Number of bytes Read */
    *lpNumberOfBytesRead = (DWORD)IOSB->Information;

	/* Success */
	return STATUS_SUCCESS;
}


int
WSPAPI 
WSPSend(
	SOCKET Handle, 
	LPWSABUF lpBuffers, 
	DWORD dwBufferCount, 
	LPDWORD lpNumberOfBytesSent, 
	DWORD iFlags, 
	LPWSAOVERLAPPED lpOverlapped, 
	LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine, 
	LPWSATHREADID lpThreadId, 
	LPINT lpErrno)
{
	PIO_STATUS_BLOCK			IOSB;
	IO_STATUS_BLOCK				DummyIOSB;
	AFD_SEND_INFO				SendInfo;
	NTSTATUS					Status;
	PVOID						APCContext;
	PVOID						APCFunction;
	HANDLE						Event;

	AFD_DbgPrint(MID_TRACE,("Called\n"));

    /* Set up the Send Structure */
    SendInfo.BufferArray = (PAFD_WSABUF)lpBuffers;
    SendInfo.BufferCount = dwBufferCount;
	SendInfo.TdiFlags = 0;
	SendInfo.AfdFlags = 0;

	/* Set the TDI Flags */
	if (iFlags) {
		if (iFlags & MSG_OOB) {
			SendInfo.TdiFlags |= TDI_SEND_EXPEDITED;
        }
        if (iFlags & MSG_PARTIAL) {
            SendInfo.TdiFlags |= TDI_SEND_PARTIAL;
        }
	}

	/* Verifiy if we should use APC */
	if (lpOverlapped == NULL) {

		/* Not using Overlapped structure, so use normal blocking on event */
		APCContext = NULL;
		APCFunction = NULL;
		Event = SockEvent;
		IOSB = &DummyIOSB;

	} else {

		if (lpCompletionRoutine == NULL) {

			/* Using Overlapped Structure, but no Completition Routine, so no need for APC */
			APCContext = lpOverlapped;
			APCFunction = NULL;
			Event = lpOverlapped->hEvent;
		
		} else {

			/* Using Overlapped Structure and a Completition Routine, so use an APC */
			APCFunction = NULL; // should be a private io completition function inside us
			APCContext = lpCompletionRoutine;
			SendInfo.AfdFlags = AFD_SKIP_FIO;
		}

		IOSB = (PIO_STATUS_BLOCK)&lpOverlapped->Internal;
		SendInfo.AfdFlags |= AFD_OVERLAPPED;
	}

	IOSB->Status = STATUS_PENDING;

	/* Send IOCTL */
	Status = NtDeviceIoControlFile((HANDLE)Handle,
					SockEvent,
					APCFunction,
					APCContext,
					IOSB,
					IOCTL_AFD_SEND,
					&SendInfo,
					sizeof(SendInfo),
					NULL,
					0);

	/* Wait for completition of not overlapped */
	if (Status == STATUS_PENDING && lpOverlapped == NULL) {
		WaitForSingleObject(SockEvent, 0); // BUGBUG, shouldn wait infintely for send...
		Status = IOSB->Status;
	}

	if (Status == STATUS_PENDING) {
	    AFD_DbgPrint(MID_TRACE,("Leaving (Pending)\n"));
	    return WSA_IO_PENDING;
	}

	/* Return Number of bytes Sent */
	*lpNumberOfBytesSent = (DWORD)IOSB->Information;

	AFD_DbgPrint(MID_TRACE,("Leaving (Success, %d)\n", IOSB->Information));

	/* Success */
	return STATUS_SUCCESS;
}

int 
WSPAPI
WSPSendTo(
	SOCKET Handle, 
	LPWSABUF lpBuffers, 
	DWORD dwBufferCount, 
	LPDWORD lpNumberOfBytesSent, 
	DWORD iFlags, 
	struct sockaddr *SocketAddress, 
	int SocketAddressLength, 
	LPWSAOVERLAPPED lpOverlapped, 
	LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine, 
	LPWSATHREADID lpThreadId, 
	LPINT lpErrno)
{
	PIO_STATUS_BLOCK			IOSB;
	IO_STATUS_BLOCK				DummyIOSB;
	AFD_SEND_INFO_UDP			SendInfo;
	PSOCKET_INFORMATION			Socket;
	NTSTATUS					Status;
	PVOID						APCContext;
	PVOID						APCFunction;
	HANDLE						Event;
	PTRANSPORT_ADDRESS			RemoteAddress;
	UCHAR						TdiBuffer[0x16];
	PSOCKADDR					BindAddress;
	INT							BindAddressLength;

	/* Get the Socket Structure associate to this Socket*/
	Socket = GetSocketStructure(Handle);

	/* Bind us First */
	if (Socket->SharedData.State == SocketOpen) {
		
		/* Get the Wildcard Address */
		BindAddressLength = Socket->HelperData->MaxWSAddressLength;
		BindAddress = HeapAlloc(GlobalHeap, 0, BindAddressLength);
		Socket->HelperData->WSHGetWildcardSockaddr (Socket->HelperContext, 
													BindAddress, 
													&BindAddressLength);

		/* Bind it */
		WSPBind(Handle, BindAddress, BindAddressLength, NULL);
	}

	/* Set up Address in TDI Format */
	RemoteAddress = (PTRANSPORT_ADDRESS)TdiBuffer;
	RemoteAddress->TAAddressCount = 1;
	RemoteAddress->Address[0].AddressLength = SocketAddressLength - sizeof(SocketAddress->sa_family);
	RtlCopyMemory(&RemoteAddress->Address[0].AddressType, SocketAddress, SocketAddressLength);

	/* Set up Structure */
	SendInfo.BufferArray = (PAFD_WSABUF)lpBuffers;
	SendInfo.AfdFlags = 0;
	SendInfo.BufferCount = dwBufferCount;
	SendInfo.RemoteAddress = RemoteAddress;
	SendInfo.SizeOfRemoteAddress = Socket->HelperData->MaxTDIAddressLength;

	/* Verifiy if we should use APC */
	if (lpOverlapped == NULL) {

		/* Not using Overlapped structure, so use normal blocking on event */
		APCContext = NULL;
		APCFunction = NULL;
		Event = SockEvent;
		IOSB = &DummyIOSB;

	} else {

		if (lpCompletionRoutine == NULL) {

			/* Using Overlapped Structure, but no Completition Routine, so no need for APC */
			APCContext = lpOverlapped;
			APCFunction = NULL;
			Event = lpOverlapped->hEvent;
		
		} else {

			/* Using Overlapped Structure and a Completition Routine, so use an APC */
			APCFunction = NULL; // should be a private io completition function inside us
			APCContext = lpCompletionRoutine;
			SendInfo.AfdFlags = AFD_SKIP_FIO;
		}

		IOSB = (PIO_STATUS_BLOCK)&lpOverlapped->Internal;
		SendInfo.AfdFlags |= AFD_OVERLAPPED;
	}

   	/* Send IOCTL */
	Status = NtDeviceIoControlFile((HANDLE)Handle,
					SockEvent,
					APCFunction,
					APCContext,
					IOSB,
					IOCTL_AFD_SEND_DATAGRAM,
					&SendInfo,
					sizeof(SendInfo),
					NULL,
					0);

	/* Wait for completition of not overlapped */
	if (Status == STATUS_PENDING && lpOverlapped == NULL) {
		WaitForSingleObject(SockEvent, 0); // BUGBUG, shouldn wait infintely for send...
		Status = IOSB->Status;
	}

	if (Status == STATUS_PENDING) {
        return WSA_IO_PENDING;
	}

	/* Return Number of bytes Sent */
    *lpNumberOfBytesSent = (DWORD)IOSB->Information;

	/* Success */
	return STATUS_SUCCESS;
}
INT
WSPAPI
WSPRecvDisconnect(
    IN  SOCKET s,
    OUT LPWSABUF lpInboundDisconnectData,
    OUT LPINT lpErrno)
{
  UNIMPLEMENTED

  return 0;
}



INT
WSPAPI
WSPSendDisconnect(
    IN  SOCKET s,
    IN  LPWSABUF lpOutboundDisconnectData,
    OUT LPINT lpErrno)
{
  UNIMPLEMENTED

  return 0;
}

/* EOF */
