/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Ancillary Function Driver DLL
 * FILE:        misc/stubs.c
 * PURPOSE:     Stubs
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/09-2000 Created
 */
#include <msafd.h>


INT
WSPAPI
WSPAddressToString(
    IN      LPSOCKADDR lpsaAddress,
    IN      DWORD dwAddressLength,
    IN      LPWSAPROTOCOL_INFOW lpProtocolInfo,
    OUT     LPWSTR lpszAddressString,
    IN OUT  LPDWORD lpdwAddressStringLength,
    OUT     LPINT lpErrno)
{
    UNIMPLEMENTED

    return 0;
}


INT
WSPAPI
WSPCancelBlockingCall(
    OUT LPINT lpErrno)
{
    UNIMPLEMENTED

    return 0;
}


INT
WSPAPI
WSPDuplicateSocket(
    IN  SOCKET s,
    IN  DWORD dwProcessId,
    OUT LPWSAPROTOCOL_INFOW lpProtocolInfo,
    OUT LPINT lpErrno)
{
    UNIMPLEMENTED

    return 0;
}


BOOL
WSPAPI
WSPGetOverlappedResult(
    IN  SOCKET s,
    IN  LPWSAOVERLAPPED lpOverlapped,
    OUT LPDWORD lpcbTransfer,
    IN  BOOL fWait,
    OUT LPDWORD lpdwFlags,
    OUT LPINT lpErrno)
{
    UNIMPLEMENTED

    return FALSE;
}


BOOL
WSPAPI
WSPGetQOSByName(
    IN      SOCKET s, 
    IN OUT  LPWSABUF lpQOSName, 
    OUT     LPQOS lpQOS, 
    OUT     LPINT lpErrno)
{
    UNIMPLEMENTED

    return FALSE;
}


INT
WSPAPI
WSPGetSockOpt(
    IN      SOCKET s,
    IN      INT level,
    IN      INT optname,
    OUT	    CHAR FAR* optval,
    IN OUT  LPINT optlen,
    OUT     LPINT lpErrno)
{
    UNIMPLEMENTED

    return 0;
}


INT
WSPAPI
WSPIoctl(
    IN  SOCKET s,
    IN  DWORD dwIoControlCode,
    IN  LPVOID lpvInBuffer,
    IN  DWORD cbInBuffer,
    OUT LPVOID lpvOutBuffer,
    IN  DWORD cbOutBuffer,
    OUT LPDWORD lpcbBytesReturned,
    IN  LPWSAOVERLAPPED lpOverlapped,
    IN  LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
    IN  LPWSATHREADID lpThreadId,
    OUT LPINT lpErrno)
{
    UNIMPLEMENTED

    return 0;
}


SOCKET
WSPAPI
WSPJoinLeaf(
    IN  SOCKET s,
    IN  CONST LPSOCKADDR name,
    IN  INT namelen,
    IN  LPWSABUF lpCallerData,
    OUT LPWSABUF lpCalleeData,
    IN  LPQOS lpSQOS,
    IN  LPQOS lpGQOS,
    IN  DWORD dwFlags,
    OUT LPINT lpErrno)
{
    UNIMPLEMENTED

    return (SOCKET)0;
}


INT
WSPAPI
WSPSetSockOpt(
    IN  SOCKET s,
    IN  INT level,
    IN  INT optname,
    IN  CONST CHAR FAR* optval,
    IN  INT optlen,
    OUT LPINT lpErrno)
{
    UNIMPLEMENTED

    return 0;
}


INT
WSPAPI
WSPShutdown(
    IN  SOCKET s,
    IN  INT how,
    OUT LPINT lpErrno)
{
    UNIMPLEMENTED

    return 0;
}


INT
WSPAPI
WSPStringToAddress(
    IN      LPWSTR AddressString,
    IN      INT AddressFamily,
    IN      LPWSAPROTOCOL_INFOW lpProtocolInfo,
    OUT     LPSOCKADDR lpAddress,
    IN OUT  LPINT lpAddressLength,
    OUT     LPINT lpErrno)
{
    UNIMPLEMENTED

    return 0;
}

/* EOF */
