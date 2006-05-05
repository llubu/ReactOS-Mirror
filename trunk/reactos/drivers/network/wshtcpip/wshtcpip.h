/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock Helper DLL for TCP/IP
 * FILE:        include/wshtcpip.h
 * PURPOSE:     WinSock Helper DLL for TCP/IP header
 */
#ifndef __WSHTCPIP_H
#define __WSHTCPIP_H

#define WIN32_NO_STATUS
#include <wsahelp.h>
#include <rtlfuncs.h>

#define EXPORT STDCALL

#define DD_TCP_DEVICE_NAME      L"\\Device\\Tcp"
#define DD_UDP_DEVICE_NAME      L"\\Device\\Udp"
#define DD_RAW_IP_DEVICE_NAME   L"\\Device\\RawIp"


typedef struct _SOCKET_CONTEXT {
    INT AddressFamily;
    INT SocketType;
    INT Protocol;
    DWORD Flags;
} SOCKET_CONTEXT, *PSOCKET_CONTEXT;

#endif /* __WSHTCPIP_H */

/* EOF */
