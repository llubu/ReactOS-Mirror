/* $Id: stubs.c,v 1.4 2003/09/08 09:56:57 weiden Exp $
 *
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock DLL
 * FILE:        stubs.c
 * PURPOSE:     Stub functions
 * PROGRAMMERS: Ge van Geldorp (ge@gse.nl)
 * REVISIONS:
 */

#include <windows.h>
#include <stdlib.h>
#include <winsock2.h>

/*
 * @unimplemented
 */
BOOL
STDCALL
AcceptEx(SOCKET ListenSocket,
         SOCKET AcceptSocket,
         PVOID OutputBuffer,
         DWORD ReceiveDataLength,
         DWORD LocalAddressLength,
         DWORD RemoteAddressLength,
         LPDWORD BytesReceived,
         LPOVERLAPPED Overlapped)
{
  OutputDebugStringW(L"w32sock AcceptEx stub called\n");

  return FALSE;
}


/*
 * @unimplemented
 */
INT
STDCALL
EnumProtocolsA(LPINT ProtocolCount,
               LPVOID ProtocolBuffer,
               LPDWORD BufferLength)
{
  OutputDebugStringW(L"w32sock EnumProtocolsA stub called\n");

  return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
INT
STDCALL
EnumProtocolsW(LPINT ProtocolCount,
               LPVOID ProtocolBuffer,
               LPDWORD BufferLength)
{
  OutputDebugStringW(L"w32sock EnumProtocolsW stub called\n");

  return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
VOID
STDCALL
GetAcceptExSockaddrs(PVOID OutputBuffer,
                     DWORD ReceiveDataLength,
                     DWORD LocalAddressLength,
                     DWORD RemoteAddressLength,
                     LPSOCKADDR* LocalSockaddr,
                     LPINT LocalSockaddrLength,
                     LPSOCKADDR* RemoteSockaddr,
                     LPINT RemoteSockaddrLength)
{
  OutputDebugStringW(L"w32sock GetAcceptExSockaddrs stub called\n");
}


/*
 * @unimplemented
 */
INT
STDCALL
GetAddressByNameA(DWORD NameSpace,
                  LPGUID ServiceType,
                  LPSTR ServiceName,
                  LPINT Protocols,
                  DWORD Resolution,
                  LPSERVICE_ASYNC_INFO ServiceAsyncInfo,
                  LPVOID CsaddrBuffer,
                  LPDWORD BufferLength,
                  LPSTR AliasBuffer,
                  LPDWORD AliasBufferLength)
{
  OutputDebugStringW(L"w32sock GetAddressByNameA stub called\n");

  return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
INT
STDCALL
GetAddressByNameW(DWORD NameSpace,
                  LPGUID ServiceType,
                  LPWSTR ServiceName,
                  LPINT Protocols,
                  DWORD Resolution,
                  LPSERVICE_ASYNC_INFO ServiceAsyncInfo,
                  LPVOID CsaddrBuffer,
                  LPDWORD BufferLength,
                  LPWSTR AliasBuffer,
                  LPDWORD AliasBufferLength)
{
  OutputDebugStringW(L"w32sock GetAddressByNameW stub called\n");

  return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
INT
STDCALL
GetServiceA(DWORD NameSpace,
            LPGUID Guid,
            LPSTR ServiceName,
            DWORD Properties,
            LPVOID Buffer,
            LPDWORD BufferSize,
            LPSERVICE_ASYNC_INFO ServiceAsyncInfo)
{
  OutputDebugStringW(L"w32sock GetServiceA stub called\n");

  return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
INT
STDCALL
GetServiceW(DWORD NameSpace,
            LPGUID Guid,
            LPWSTR ServiceName,
            DWORD Properties,
            LPVOID Buffer,
            LPDWORD BufferSize,
            LPSERVICE_ASYNC_INFO ServiceAsyncInfo)
{
  OutputDebugStringW(L"w32sock GetServiceW stub called\n");

  return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
INT
STDCALL
GetTypeByNameA(LPSTR ServiceName,
               LPGUID ServiceType)
{
  OutputDebugStringW(L"w32sock GetTypeByNameA stub called\n");

  return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
INT
STDCALL
GetTypeByNameW(LPWSTR ServiceName,
               LPGUID ServiceType)
{
  OutputDebugStringW(L"w32sock GetTypeByNameW stub called\n");

  return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
INT
STDCALL
SetServiceA(DWORD NameSpace,
            DWORD Operation,
            DWORD Flags,
            LPSERVICE_INFOA ServiceInfo,
            LPSERVICE_ASYNC_INFO ServiceAsyncInfo,
            LPDWORD dwStatusFlags)
{
  OutputDebugStringW(L"w32sock SetServiceA stub called\n");

  return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
INT
STDCALL
SetServiceW(DWORD NameSpace,
            DWORD Operation,
            DWORD Flags,
            LPSERVICE_INFOW ServiceInfo,
            LPSERVICE_ASYNC_INFO ServiceAsyncInfo,
            LPDWORD dwStatusFlags)
{
  OutputDebugStringW(L"w32sock SetServiceW stub called\n");

  return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
TransmitFile(SOCKET Socket,
             HANDLE File,
             DWORD NumberOfBytesToWrite,
             DWORD NumberOfBytesPerSend,
             LPOVERLAPPED Overlapped,
             LPTRANSMIT_FILE_BUFFERS TransmitBuffers,
             DWORD Flags)
{
  OutputDebugStringW(L"w32sock TransmitFile stub called\n");

  return FALSE;
}


/*
 * @unimplemented
 */
HANDLE
STDCALL
WSAAsyncGetHostByAddr(HWND Wnd,
                      unsigned int Msg,
                      const char *Addr,
                      int Len,
                      int Type,
                      char *Buf,
                      int BufLen)
{
  OutputDebugStringW(L"w32sock WSAAsyncGetHostByAddr stub called\n");

  return NULL;
}


/*
 * @unimplemented
 */
HANDLE
STDCALL
WSAAsyncGetHostByName(HWND Wnd,
                      unsigned int Msg,
                      const char *Name,
                      char *Buf,
                      int BufLen)
{
  OutputDebugStringW(L"w32sock WSAAsyncGetHostByName stub called\n");

  return NULL;
}


/*
 * @unimplemented
 */
HANDLE
STDCALL
WSAAsyncGetProtoByName(HWND Wnd,
                       unsigned int Msg,
                       const char *Name,
                       char *Buf,
                       int Buflen)
{
  OutputDebugStringW(L"w32sock WSAAsyncGetProtoByName stub called\n");

  return NULL;
}


/*
 * @unimplemented
 */
HANDLE
STDCALL
WSAAsyncGetProtoByNumber(HWND Wnd,
                         unsigned int Msg,
                         int Number,
                         char *Buf,
                         int BufLen)
{
  OutputDebugStringW(L"w32sock WSAAsyncGetProtoByNumber stub called\n");

  return NULL;
}


/*
 * @unimplemented
 */
HANDLE
STDCALL
WSAAsyncGetServByName(HWND Wnd,
                      unsigned int Msg,
                      const char *Name,
                      const char *Proto,
                      char *Buf,
                      int BufLen)
{
  OutputDebugStringW(L"w32sock WSAAsyncGetServByName stub called\n");

  return NULL;
}


/*
 * @unimplemented
 */
HANDLE
STDCALL
WSAAsyncGetServByPort(HWND Wnd,
                      unsigned int Msg,
                      int Port,
                      const char *Proto,
                      char *Buf,
                      int BufLen)
{
  OutputDebugStringW(L"w32sock WSAAsyncGetServByPort stub called\n");

  return NULL;
}


/*
 * @unimplemented
 */
INT
STDCALL
WSAAsyncSelect(SOCKET Sock,
               HWND Wnd,
               UINT Msg,
               LONG Event)
{
  OutputDebugStringW(L"w32sock WSAAsyncSelect stub called\n");

  return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
int
STDCALL
WSACancelAsyncRequest(HANDLE AsyncTaskHandle)
{
  OutputDebugStringW(L"w32sock WSACancelAsyncRequest stub called\n");

  return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
int
STDCALL
WSACancelBlockingCall()
{
  OutputDebugStringW(L"w32sock WSACancelBlockingCall stub called\n");

  return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
int
STDCALL
WSACleanup()
{
  OutputDebugStringW(L"w32sock WSACleanup stub called\n");

  return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
int
STDCALL
WSAGetLastError(void)
{
  OutputDebugStringW(L"w32sock WSAGetLastError stub called\n");

  return WSANOTINITIALISED;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
WSAIsBlocking(VOID)
{
  OutputDebugStringW(L"w32sock WSAIsBlocking stub called\n");

  return FALSE;
}


/*
 * @unimplemented
 */
int
STDCALL
WSARecvEx(SOCKET Sock,
          char *Buf,
          int Len,
          int *Flags)
{
  OutputDebugStringW(L"w32sock WSARecvEx stub called\n");

  return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
FARPROC
STDCALL
WSASetBlockingHook(FARPROC BlockFunc)
{
  OutputDebugStringW(L"w32sock WSASetBlockingHook stub called\n");

  return NULL;
}


/*
 * @unimplemented
 */
void
STDCALL WSASetLastError(int Error)
{
  OutputDebugStringW(L"w32sock WSASetLastError stub called\n");
}


/*
 * @unimplemented
 */
int
STDCALL
WSAStartup(WORD VersionRequested,
           LPWSADATA WSAData)
{
  OutputDebugStringW(L"w32sock WSAStartup stub called\n");

  return WSASYSNOTREADY;
}


/*
 * @unimplemented
 */
int
STDCALL
WSAUnhookBlockingHook(void)
{
  OutputDebugStringW(L"w32sock WSAUnhookBlockingHook stub called\n");

  return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
int
STDCALL
WSApSetPostRoutine(LPVOID /* really LPWPUPOSTMESSAGE */ PostRoutine)
{
  OutputDebugStringW(L"w32sock WSApSetPostRoutine stub called\n");

  return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
int
STDCALL
__WSAFDIsSet(SOCKET Sock,
             fd_set *Set)
{
  OutputDebugStringW(L"w32sock __WSAFDIsSet stub called\n");

  return 0;
}


/*
 * @unimplemented
 */
SOCKET
STDCALL
accept(SOCKET Sock,
       struct sockaddr *Addr,
       int *AddrLen)
{
  OutputDebugStringW(L"w32sock accept stub called\n");

  return INVALID_SOCKET;
}


/*
 * @unimplemented
 */
INT
STDCALL
bind(SOCKET Sock,
     CONST LPSOCKADDR Name,
     INT NameLen)
{
  OutputDebugStringW(L"w32sock bind stub called\n");

  return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
int
STDCALL
closesocket(SOCKET Sock)
{
  OutputDebugStringW(L"w32sock closesocket stub called\n");

  return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
INT
STDCALL
connect(SOCKET Sock,
        CONST LPSOCKADDR Name,
        INT NameLen)
{
  OutputDebugStringW(L"w32sock connect stub called\n");

  return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
int
STDCALL
dn_expand(unsigned char *MessagePtr,
          unsigned char *EndofMesOrig,
          unsigned char *CompDomNam,
          unsigned char *ExpandDomNam,
          int Length)
{
  OutputDebugStringW(L"w32sock dn_expand stub called\n");

  return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
LPHOSTENT
STDCALL
gethostbyaddr(CONST CHAR *Addr,
              INT Len,
              INT Type)
{
  OutputDebugStringW(L"w32sock gethostbyaddr stub called\n");

  return NULL;
}


/*
 * @unimplemented
 */
struct hostent *
STDCALL
gethostbyname(const char *Name)
{
  OutputDebugStringW(L"w32sock gethostbyname stub called\n");

  return NULL;
}


/*
 * @unimplemented
 */
int
STDCALL
gethostname(char *Name,
            int NameLen)
{
  OutputDebugStringW(L"w32sock gethostname stub called\n");

  return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
struct netent *
STDCALL
getnetbyname(char *Name)
{
  OutputDebugStringW(L"w32sock getnetbyname stub called\n");

  return NULL;
}


/*
 * @unimplemented
 */
int
STDCALL
getpeername(SOCKET Sock,
            struct sockaddr *Name,
            int *NameLen)
{
  OutputDebugStringW(L"w32sock getpeername stub called\n");

  return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
LPPROTOENT
STDCALL
getprotobyname(CONST CHAR *Name)
{
  OutputDebugStringW(L"w32sock getprotobyname stub called\n");

  return NULL;
}


/*
 * @unimplemented
 */
LPPROTOENT
STDCALL
getprotobynumber(INT Number)
{
  OutputDebugStringW(L"w32sock getprotobynumber stub called\n");

  return NULL;
}


/*
 * @unimplemented
 */
struct servent *
STDCALL
getservbyname(const char *Name,
              const char *Proto)
{
  OutputDebugStringW(L"w32sock getservbyname stub called\n");

  return NULL;
}


/*
 * @unimplemented
 */
struct servent *
STDCALL
getservbyport(int Port,
              const char *Proto)
{
  OutputDebugStringW(L"w32sock getservbyport stub called\n");

  return NULL;
}


/*
 * @unimplemented
 */
int
STDCALL
getsockname(SOCKET Sock,
            struct sockaddr *Name,
            int *NameLen)
{
  OutputDebugStringW(L"w32sock getsockname stub called\n");

  return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
int
STDCALL
getsockopt(SOCKET Sock,
           int Level,
           int OptName,
           char *OptVal,
           int *OptLen)
{
  OutputDebugStringW(L"w32sock getsockopt stub called\n");

  return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
ULONG
STDCALL
htonl(ULONG HostLong)
{
  return (((HostLong << 24) & 0xff000000) |
          ((HostLong << 8) & 0x00ff0000) |
          ((HostLong >> 8) & 0x0000ff00) |
          ((HostLong >> 24) & 0x000000ff));
}


/*
 * @unimplemented
 */
USHORT
STDCALL
htons(USHORT HostShort)
{
  return (((HostShort << 8) & 0xff00) |
          ((HostShort >> 8) & 0x00ff));
}


/*
 * @unimplemented
 */
ULONG
STDCALL
inet_addr(CONST CHAR *cp)
{
  OutputDebugStringW(L"w32sock inet_addr stub called\n");

  return INADDR_NONE;
}


/*
 * @unimplemented
 */
unsigned long
STDCALL
inet_network(const char *cp)
{
  OutputDebugStringW(L"w32sock inet_network stub called\n");

  return INADDR_NONE;
}


/*
 * @unimplemented
 */
char *
STDCALL
inet_ntoa(struct in_addr in)
{
  OutputDebugStringW(L"w32sock inet_ntoa stub called\n");

  return NULL;
}


/*
 * @unimplemented
 */
INT
STDCALL
ioctlsocket(SOCKET Sock,
            LONG Cmd,
            ULONG *Argp)
{
  OutputDebugStringW(L"w32sock ioctlsocket stub called\n");

  return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
int
STDCALL
listen(SOCKET Sock,
       int BackLog)
{
  OutputDebugStringW(L"w32sock listen stub called\n");

  return SOCKET_ERROR;
}


/*
 * @implemented
 */
ULONG
STDCALL
ntohl(ULONG NetLong)
{
  return (((NetLong << 24) & 0xff000000) |
          ((NetLong << 8) & 0x00ff0000) |
          ((NetLong >> 8) & 0x0000ff00) |
          ((NetLong >> 24) & 0x000000ff));
}


/*
 * @implemented
 */
USHORT
STDCALL
ntohs(USHORT NetShort)
{
  return (((NetShort << 8) & 0xff00) |
          ((NetShort >> 8) & 0x00ff));
}


/*
 * @unimplemented
 */
SOCKET
STDCALL
rcmd(char **AHost,
     USHORT InPort,
     char *LocUser,
     char *RemUser,
     char *Cmd,
     int *Fd2p)
{
  OutputDebugStringW(L"w32sock rcmd stub called\n");

  return INVALID_SOCKET;
}


/*
 * @unimplemented
 */
int
STDCALL
recv(SOCKET Sock,
     char *Buf,
     int Len,
     int Flags)
{
  OutputDebugStringW(L"w32sock recv stub called\n");

  return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
int
STDCALL
recvfrom(SOCKET Sock,
         char *Buf,
         int Len,
         int Flags,
         struct sockaddr *From,
         int *FromLen)
{
  OutputDebugStringW(L"w32sock recvfrom stub called\n");

  return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
SOCKET
STDCALL
rexec(char **AHost,
      int InPort,
      char *User,
      char *Passwd,
      char *Cmd,
      int *Fd2p)
{
  OutputDebugStringW(L"w32sock rexec stub called\n");

  return INVALID_SOCKET;
}


/*
 * @unimplemented
 */
SOCKET
STDCALL
rresvport(int *port)
{
  OutputDebugStringW(L"w32sock rresvport stub called\n");

  return INVALID_SOCKET;
}


/*
 * @unimplemented
 */
void
STDCALL
s_perror(const char *str)
{
  OutputDebugStringW(L"w32sock s_perror stub called\n");
}


/*
 * @unimplemented
 */
INT
STDCALL
select(INT NumFds, 
       LPFD_SET ReadFds, 
       LPFD_SET WriteFds, 
       LPFD_SET ExceptFds, 
       CONST LPTIMEVAL TimeOut)
{
  OutputDebugStringW(L"w32sock select stub called\n");

  return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
int
STDCALL
send(SOCKET Sock,
     const char *Buf,
     int Len,
     int Flags)
{
  OutputDebugStringW(L"w32sock send stub called\n");

  return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
INT
STDCALL
sendto(SOCKET Sock,
       CONST CHAR *Buf,
       INT Len,
       INT Flags,
       CONST LPSOCKADDR To, 
       INT ToLen)
{
  OutputDebugStringW(L"w32sock sendto stub called\n");

  return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
int
STDCALL
sethostname(char *Name, int NameLen)
{
  OutputDebugStringW(L"w32sock sethostname stub called\n");

  return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
int
STDCALL
setsockopt(SOCKET Sock,
           int Level,
           int OptName,
           const char *OptVal,
           int OptLen)
{
  OutputDebugStringW(L"w32sock setsockopt stub called\n");

  return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
int
STDCALL
shutdown(SOCKET Sock,
         int How)
{
  OutputDebugStringW(L"w32sock shutdown stub called\n");

  return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
SOCKET
STDCALL
socket(int AF,
       int Type,
       int Protocol)
{
  OutputDebugStringW(L"w32sock socket stub called\n");

  return INVALID_SOCKET;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
DllMain(HINSTANCE InstDLL,
        DWORD Reason,
        LPVOID Reserved)
{
  return TRUE;
}

/*
 * @unimplemented
 */
INT
STDCALL
GetNameByTypeA(LPGUID lpServiceType,LPSTR lpServiceName,DWORD dwNameLength)
{
  OutputDebugStringW(L"w32sock GetNameByTypeA stub called\n");
  return TRUE;
}

/*
 * @unimplemented
 */
INT
STDCALL
GetNameByTypeW(LPGUID lpServiceType,LPWSTR lpServiceName,DWORD dwNameLength)
{
  OutputDebugStringW(L"w32sock GetNameByTypeW stub called\n");
  return TRUE;
}
