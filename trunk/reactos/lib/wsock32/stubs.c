/* $Id: stubs.c,v 1.6 2003/11/19 13:16:22 navaraf Exp $
 *
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock DLL
 * FILE:        stubs.c
 * PURPOSE:     Stub functions
 * PROGRAMMERS: Ge van Geldorp (ge@gse.nl)
 * REVISIONS:
 */

#include <windows.h>
#include <winsock2.h>
//#include <stdlib.h>

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
BOOL
STDCALL
DllMain(HINSTANCE InstDLL,
        DWORD Reason,
        LPVOID Reserved)
{
  return TRUE;
}
