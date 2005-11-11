/*
 *  ReactOS Services
 *  Copyright (C) 2005 ReactOS Team
 *
 * LICENCE:     GPL - See COPYING in the top level directory
 * PROJECT:     ReactOS simple TCP/IP services
 * FILE:        apps/utils/net/tcpsvcs/skelserver.c
 * PURPOSE:     Provide CharGen, Daytime, Discard, Echo, and Qotd services
 * PROGRAMMERS: Ged Murphy (gedmurphy@gmail.com)
 * REVISIONS:
 *   GM 04/10/05 Created
 *
 */

#include "tcpsvcs.h"

extern BOOL bShutDownFlag;
extern BOOL bPauseFlag;

DWORD WINAPI StartServer(LPVOID lpParam)
{
	SOCKET ListeningSocket;
	PSERVICES pServices;
    const TCHAR* HostIP = "127.0.0.1";
    TCHAR temp[512];

    pServices = (PSERVICES)lpParam;
	
//DebugBreak();
    ListeningSocket = SetUpListener(HostIP, htons(pServices->Port));
    if (ListeningSocket == INVALID_SOCKET)
    {
		LogEvent("Socket error when setting up listener", 0, TRUE);
        return 3;
    }

    _stprintf(temp, _T("%s is waiting for connections on port %d...\n"),
        pServices->Name, pServices->Port);
    LogEvent(temp, 0, FALSE);

	AcceptConnections(ListeningSocket, pServices->Service, pServices->Name);

    LogEvent(_T("Detaching Winsock2...\n"), 0, FALSE);
    WSACleanup();
    return 0;
}


SOCKET SetUpListener(const char* ServAddr, int Port)
{
    SOCKET Sock;
    SOCKADDR_IN Server;

    Sock = socket(AF_INET, SOCK_STREAM, 0);
    if (Sock != INVALID_SOCKET)
    {
        Server.sin_family = AF_INET;
        Server.sin_addr.s_addr = htonl(INADDR_ANY);
        Server.sin_port = Port;
        if (bind(Sock, (SOCKADDR*)&Server, sizeof(SOCKADDR_IN)) != SOCKET_ERROR)
        {
            listen(Sock, SOMAXCONN);
            return Sock;
        }
        else
            LogEvent(_T("bind() failed\n"), 0, TRUE);

    }
    return INVALID_SOCKET;
}

/* note: consider allowing a maximum number of connections
 * A number of threads can be allocated and worker threads will
 * only be spawned if a free space is available
 
typedef struct _WORKER_THREAD {
    DWORD num;
    BOOL available;
    HANDLE hThread;
} WORKER_THREAD;

*/

VOID AcceptConnections(SOCKET ListeningSocket,
    LPTHREAD_START_ROUTINE Service, TCHAR *Name)
{
    SOCKADDR_IN Client;
    SOCKET Sock;
    HANDLE hThread;
    TIMEVAL TimeVal;
    FD_SET ReadFDS;
    INT nAddrSize = sizeof(Client);
    DWORD ThreadID;
    TCHAR temp[512];
    INT TimeOut = 2000; // 2 seconds

//DebugBreak();
    /* monitor for incomming connections */
    FD_ZERO(&ReadFDS);

    /* set timeout values */
    TimeVal.tv_sec  = TimeOut / 1000;
    TimeVal.tv_usec = TimeOut % 1000;

    while (! bShutDownFlag) // (i<MAX_CLIENTS && !bShutDownFlag)
    {
		FD_SET(ListeningSocket, &ReadFDS);
        if (select(0, &ReadFDS, NULL, NULL, &TimeVal) == SOCKET_ERROR)
        {
            LogEvent(_T("select failed\n"), 0, TRUE);
            return;
        }
        
        if (FD_ISSET(ListeningSocket, &ReadFDS))
        {
            Sock = accept(ListeningSocket, (SOCKADDR*)&Client, &nAddrSize);
            if (Sock != INVALID_SOCKET)
            {
                _stprintf(temp, _T("Accepted connection to %s server from %s:%d\n"),
                    Name, inet_ntoa(Client.sin_addr), ntohs(Client.sin_port));
                LogEvent(temp, 0, FALSE);
                _stprintf(temp, _T("Creating new thread for %s\n"), Name);
                LogEvent(temp, 0, FALSE);

                hThread = CreateThread(0, 0, Service, (void*)Sock, 0, &ThreadID);

                /* Check the return value for success. */ 
                if (hThread == NULL)
                {
                    _stprintf(temp, _T("Failed to start worker thread for "
                        "the %s server....\n"), Name);
                    LogEvent(temp, 0, TRUE);
                }

                //Do we need to wait, or just kill it?
                WaitForSingleObject(hThread, INFINITE);

            }
            else
            {
                LogEvent(_T("accept failed\n"), 0, TRUE);
                return;
            }
        }
    }
}

BOOL ShutdownConnection(SOCKET Sock, BOOL bRec)
{
    TCHAR temp[512];

    /* Disallow any further data sends.  This will tell the other side
       that we want to go away now.  If we skip this step, we don't
       shut the connection down nicely. */
    if (shutdown(Sock, SD_SEND) == SOCKET_ERROR)
    {
        LogEvent(_T("Error in shutdown()\n"), 0, TRUE);
        return FALSE;
    }

      /* Receive any extra data still sitting on the socket.  After all
         data is received, this call will block until the remote host
         acknowledges the TCP control packet sent by the shutdown above.
         Then we'll get a 0 back from recv, signalling that the remote
         host has closed its side of the connection. */
    if (bRec)
    {
        char ReadBuffer[BUF];
        int NewBytes = recv(Sock, ReadBuffer, BUF, 0);
        if (NewBytes == SOCKET_ERROR)
            return FALSE;
        else if (NewBytes != 0)
        {
            _stprintf(temp, _T("FYI, received %d unexpected bytes during shutdown\n"), NewBytes);
            LogEvent(temp, 0, FALSE);
        }
    }

    /* Close the socket. */
    if (closesocket(Sock) == SOCKET_ERROR)
        return FALSE;

    return TRUE;
}
