/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 DLL
 * FILE:        misc/ns.c
 * PURPOSE:     Namespace APIs
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/09-2000 Created
 */
#include <ws2_32.h>

/* Name resolution APIs */

/*
 * @unimplemented
 */
INT
EXPORT
WSAAddressToStringA(
    IN      LPSOCKADDR lpsaAddress,
    IN      DWORD dwAddressLength,
    IN      LPWSAPROTOCOL_INFOA lpProtocolInfo,
    OUT     LPSTR lpszAddressString,
    IN OUT  LPDWORD lpdwAddressStringLength)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSAAddressToStringW(
    IN      LPSOCKADDR lpsaAddress,
    IN      DWORD dwAddressLength,
    IN      LPWSAPROTOCOL_INFOW lpProtocolInfo,
    OUT     LPWSTR lpszAddressString,
    IN OUT  LPDWORD lpdwAddressStringLength)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSAEnumNameSpaceProvidersA(
    IN OUT  LPDWORD lpdwBufferLength,
    OUT     LPWSANAMESPACE_INFOA lpnspBuffer)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSAEnumNameSpaceProvidersW(
    IN OUT  LPDWORD lpdwBufferLength,
    OUT     LPWSANAMESPACE_INFOW lpnspBuffer)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSAGetServiceClassInfoA(
    IN      LPGUID lpProviderId,
    IN      LPGUID lpServiceClassId,
    IN OUT  LPDWORD lpdwBufferLength,
    OUT     LPWSASERVICECLASSINFOA lpServiceClassInfo)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSAGetServiceClassInfoW(
    IN      LPGUID lpProviderId,
    IN      LPGUID lpServiceClassId,
    IN OUT  LPDWORD lpdwBufferLength,
    OUT     LPWSASERVICECLASSINFOW lpServiceClassInfo)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSAGetServiceClassNameByClassIdA(
    IN      LPGUID lpServiceClassId,
    OUT     LPSTR lpszServiceClassName,
    IN OUT  LPDWORD lpdwBufferLength)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSAGetServiceClassNameByClassIdW(
    IN      LPGUID lpServiceClassId,
    OUT     LPWSTR lpszServiceClassName,
    IN OUT  LPDWORD lpdwBufferLength)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSAInstallServiceClassA(
    IN  LPWSASERVICECLASSINFOA lpServiceClassInfo)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSAInstallServiceClassW(
    IN  LPWSASERVICECLASSINFOW lpServiceClassInfo)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSALookupServiceBeginA(
    IN  LPWSAQUERYSETA lpqsRestrictions,
    IN  DWORD dwControlFlags,
    OUT LPHANDLE lphLookup)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSALookupServiceBeginW(
    IN  LPWSAQUERYSETW lpqsRestrictions,
    IN  DWORD dwControlFlags,
    OUT LPHANDLE lphLookup)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSALookupServiceEnd(
    IN  HANDLE hLookup)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSALookupServiceNextA(
    IN      HANDLE hLookup,
    IN      DWORD dwControlFlags,
    IN OUT  LPDWORD lpdwBufferLength,
    OUT     LPWSAQUERYSETA lpqsResults)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSALookupServiceNextW(
    IN      HANDLE hLookup,
    IN      DWORD dwControlFlags,
    IN OUT  LPDWORD lpdwBufferLength,
    OUT     LPWSAQUERYSETW lpqsResults)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSARemoveServiceClass(
    IN  LPGUID lpServiceClassId)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSASetServiceA(
    IN  LPWSAQUERYSETA lpqsRegInfo,
    IN  WSAESETSERVICEOP essOperation,
    IN  DWORD dwControlFlags)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSASetServiceW(
    IN  LPWSAQUERYSETW lpqsRegInfo,
    IN  WSAESETSERVICEOP essOperation,
    IN  DWORD dwControlFlags)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSAStringToAddressA(
    IN      LPSTR AddressString,
    IN      INT AddressFamily,
    IN      LPWSAPROTOCOL_INFOA lpProtocolInfo,
    OUT     LPSOCKADDR lpAddress,
    IN OUT  LPINT lpAddressLength)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSAStringToAddressW(
    IN      LPWSTR AddressString,
    IN      INT AddressFamily,
    IN      LPWSAPROTOCOL_INFOW lpProtocolInfo,
    OUT     LPSOCKADDR lpAddress,
    IN OUT  LPINT lpAddressLength)
{
    UNIMPLEMENTED

    return 0;
}


/* WinSock 1.1 compatible name resolution APIs */

/*
 * @unimplemented
 */
LPHOSTENT
EXPORT
gethostbyaddr(
    IN  CONST CHAR FAR* addr,
    IN  INT len,
    IN  INT type)
{
    UNIMPLEMENTED

    return (LPHOSTENT)NULL;
}

/*
 * @unimplemented
 */
LPHOSTENT
EXPORT
gethostbyname(
    IN  CONST CHAR FAR* name)
{
    UNIMPLEMENTED

    return (LPHOSTENT)NULL;
}


/*
 * @unimplemented
 */
INT
EXPORT
gethostname(
    OUT CHAR FAR* name,
    IN  INT namelen)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * XXX arty -- Partial implementation pending a better one.  This one will
 * do for normal purposes.
 *
 * Return the address of a static LPPROTOENT corresponding to the named
 * protocol.  These structs aren't very interesting, so I'm not too ashamed
 * to have this function work on builtins for now.
 *
 * @unimplemented
 */
LPPROTOENT
EXPORT
getprotobyname(
    IN  CONST CHAR FAR* name)
{
    static CHAR *udp_aliases = 0;
    static PROTOENT udp = { "udp", &udp_aliases, 17 };
    static CHAR *tcp_aliases = 0;
    static PROTOENT tcp = { "tcp", &tcp_aliases, 6 };
    if( !_stricmp( name, "udp" ) ) {
	return &udp;
    } else if( !_stricmp( name, "tcp" ) ) {
	return &tcp;
    }
    
    return 0;
}

/*
 * @unimplemented
 */
LPPROTOENT
EXPORT
getprotobynumber(
    IN  INT number)
{
    UNIMPLEMENTED

    return (LPPROTOENT)NULL;
}


/*
 * @unimplemented
 */
LPSERVENT
EXPORT
getservbyname(
    IN  CONST CHAR FAR* name, 
    IN  CONST CHAR FAR* proto)
{
    UNIMPLEMENTED

    return (LPSERVENT)NULL;
}


/*
 * @unimplemented
 */
LPSERVENT
EXPORT
getservbyport(
    IN  INT port, 
    IN  CONST CHAR FAR* proto)
{
    UNIMPLEMENTED

    return (LPSERVENT)NULL;
}


/*
 * @implemented
 */
ULONG
EXPORT
inet_addr(
    IN  CONST CHAR FAR* cp)
/*
 * FUNCTION: Converts a string containing an IPv4 address to an unsigned long
 * ARGUMENTS:
 *     cp = Pointer to string with address to convert
 * RETURNS:
 *     Binary representation of IPv4 address, or INADDR_NONE
 */
{
    UINT i;
    PCHAR p;
    ULONG u = 0;

    p = (PCHAR)cp;

    if (strlen(p) == 0)
        return INADDR_NONE;

    if (strcmp(p, " ") == 0)
        return 0;

    for (i = 0; i <= 3; i++) {
        u += (strtoul(p, &p, 0) << (i * 8));

        if (strlen(p) == 0)
            return u;

        if (p[0] != '.')
            return INADDR_NONE;

        p++;
    }

    return u;
}


/*
 * @implemented
 */
CHAR FAR*
EXPORT
inet_ntoa(
    IN  IN_ADDR in)
{
    CHAR b[10];
    PCHAR p;

    p = ((PWINSOCK_THREAD_BLOCK)NtCurrentTeb()->WinSockData)->Intoa;
    _itoa(in.S_un.S_addr & 0xFF, b, 10);
    strcpy(p, b);
    _itoa((in.S_un.S_addr >> 8) & 0xFF, b, 10);
    strcat(p, ".");
    strcat(p, b);
    _itoa((in.S_un.S_addr >> 16) & 0xFF, b, 10);
    strcat(p, ".");
    strcat(p, b);
    _itoa((in.S_un.S_addr >> 24) & 0xFF, b, 10);
    strcat(p, ".");
    strcat(p, b);
    return (CHAR FAR*)p;
}


/*
 * @unimplemented
 */
HANDLE
EXPORT
WSAAsyncGetHostByAddr(
    IN  HWND hWnd,
    IN  UINT wMsg,
    IN  CONST CHAR FAR* addr, 
    IN  INT len,
    IN  INT type, 
    OUT CHAR FAR* buf, 
    IN  INT buflen)
{
    UNIMPLEMENTED

    return (HANDLE)0;
}


/*
 * @unimplemented
 */
HANDLE
EXPORT
WSAAsyncGetHostByName(
    IN  HWND hWnd, 
    IN  UINT wMsg,  
    IN  CONST CHAR FAR* name, 
    OUT CHAR FAR* buf, 
    IN  INT buflen)
{
    UNIMPLEMENTED

    return (HANDLE)0;
}


/*
 * @unimplemented
 */
HANDLE
EXPORT
WSAAsyncGetProtoByName(
    IN  HWND hWnd,
    IN  UINT wMsg,
    IN  CONST CHAR FAR* name,
    OUT CHAR FAR* buf,
    IN  INT buflen)
{
    UNIMPLEMENTED

    return (HANDLE)0;
}


/*
 * @unimplemented
 */
HANDLE
EXPORT
WSAAsyncGetProtoByNumber(
    IN  HWND hWnd,
    IN  UINT wMsg,
    IN  INT number,
    OUT CHAR FAR* buf,
    IN  INT buflen)
{
    UNIMPLEMENTED

    return (HANDLE)0;
}


/*
 * @unimplemented
 */
HANDLE
EXPORT
WSAAsyncGetServByName(
    IN  HWND hWnd,
    IN  UINT wMsg,
    IN  CONST CHAR FAR* name,
    IN  CONST CHAR FAR* proto,
    OUT CHAR FAR* buf,
    IN  INT buflen)
{
    UNIMPLEMENTED

    return (HANDLE)0;
}


/*
 * @unimplemented
 */
HANDLE
EXPORT
WSAAsyncGetServByPort(
    IN  HWND hWnd,
    IN  UINT wMsg,
    IN  INT port,
    IN  CONST CHAR FAR* proto,
    OUT CHAR FAR* buf,
    IN  INT buflen)
{
    UNIMPLEMENTED

    return (HANDLE)0;
}

/* EOF */
