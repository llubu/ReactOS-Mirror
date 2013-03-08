/*
 * PROJECT:     ReactOS Service Control Manager
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/system/services/services.c
 * PURPOSE:     Main SCM controller
 * COPYRIGHT:   Copyright 2001-2005 Eric Kohl
 *              Copyright 2007 Ged Murphy <gedmurphy@reactos.org>
 *
 */

/* INCLUDES *****************************************************************/

#include "services.h"

#define NDEBUG
#include <debug.h>

int WINAPI RegisterServicesProcess(DWORD ServicesProcessId);

/* GLOBALS ******************************************************************/

#define PIPE_BUFSIZE 1024
#define PIPE_TIMEOUT 1000

BOOL ScmShutdown = FALSE;
static HANDLE hScmShutdownEvent = NULL;


/* FUNCTIONS *****************************************************************/

VOID
PrintString(LPCSTR fmt, ...)
{
#if DBG
    CHAR buffer[512];
    va_list ap;

    va_start(ap, fmt);
    vsprintf(buffer, fmt, ap);
    va_end(ap);

    OutputDebugStringA(buffer);
#endif
}


VOID
ScmLogError(DWORD dwEventId,
            WORD wStrings,
            LPCWSTR *lpStrings)
{
    HANDLE hLog;

    hLog = RegisterEventSourceW(NULL,
                                L"Service Control Manager");
    if (hLog == NULL)
    {
        DPRINT1("ScmLogEvent: RegisterEventSourceW failed %lu\n", GetLastError());
        return;
    }

    if (!ReportEventW(hLog,
                      EVENTLOG_ERROR_TYPE,
                      0,
                      dwEventId,
                      NULL, // Sid,
                      wStrings,
                      0,
                      lpStrings,
                      NULL))
    {
        DPRINT1("ScmLogEvent: ReportEventW failed %lu\n", GetLastError());
    }

    DeregisterEventSource(hLog);
}


BOOL
ScmCreateStartEvent(PHANDLE StartEvent)
{
    HANDLE hEvent;

    hEvent = CreateEventW(NULL,
                          TRUE,
                          FALSE,
                          L"SvcctrlStartEvent_A3752DX");
    if (hEvent == NULL)
    {
        if (GetLastError() == ERROR_ALREADY_EXISTS)
        {
            hEvent = OpenEventW(EVENT_ALL_ACCESS,
                                FALSE,
                                L"SvcctrlStartEvent_A3752DX");
            if (hEvent == NULL)
            {
                return FALSE;
            }
        }
        else
        {
            return FALSE;
        }
    }

    *StartEvent = hEvent;

    return TRUE;
}


VOID
ScmWaitForLsa(VOID)
{
    HANDLE hEvent;
    DWORD dwError;

    hEvent = CreateEventW(NULL,
                          TRUE,
                          FALSE,
                          L"LSA_RPC_SERVER_ACTIVE");
    if (hEvent == NULL)
    {
        dwError = GetLastError();
        DPRINT1("Failed to create the notication event (Error %lu)\n", dwError);

        if (dwError == ERROR_ALREADY_EXISTS)
        {
            hEvent = OpenEventW(SYNCHRONIZE,
                                FALSE,
                                L"LSA_RPC_SERVER_ACTIVE");
            if (hEvent == NULL)
            {
               DPRINT1("Could not open the notification event (Error %lu)\n", GetLastError());
               return;
            }
        }
    }

    DPRINT("Wait for the LSA server!\n");
    WaitForSingleObject(hEvent, INFINITE);
    DPRINT("LSA server running!\n");

    CloseHandle(hEvent);

    DPRINT("ScmWaitForLsa() done\n");
}


BOOL
ScmNamedPipeHandleRequest(PVOID Request,
                          DWORD RequestSize,
                          PVOID Reply,
                          LPDWORD ReplySize)
{
    DbgPrint("SCM READ: %p\n", Request);

    *ReplySize = 0;
    return FALSE;
}


DWORD WINAPI
ScmNamedPipeThread(LPVOID Context)
{
    CHAR chRequest[PIPE_BUFSIZE];
    CHAR chReply[PIPE_BUFSIZE];
    DWORD cbReplyBytes;
    DWORD cbBytesRead;
    DWORD cbWritten;
    BOOL bSuccess;
    HANDLE hPipe;

    hPipe = (HANDLE)Context;

    DPRINT("ScmNamedPipeThread(%lu) - Accepting SCM commands through named pipe\n", hPipe);

    for (;;)
    {
        bSuccess = ReadFile(hPipe,
                            &chRequest,
                            PIPE_BUFSIZE,
                            &cbBytesRead,
                            NULL);
        if (!bSuccess || cbBytesRead == 0)
        {
            break;
        }

        if (ScmNamedPipeHandleRequest(&chRequest, cbBytesRead, &chReply, &cbReplyBytes))
        {
            bSuccess = WriteFile(hPipe,
                                 &chReply,
                                 cbReplyBytes,
                                 &cbWritten,
                                 NULL);
            if (!bSuccess || cbReplyBytes != cbWritten)
            {
                break;
            }
        }
    }

    DPRINT("ScmNamedPipeThread(%lu) - Disconnecting named pipe connection\n", hPipe);

    FlushFileBuffers(hPipe);
    DisconnectNamedPipe(hPipe);
    CloseHandle(hPipe);

    DPRINT("ScmNamedPipeThread(%lu) - Done.\n", hPipe);

    return ERROR_SUCCESS;
}


BOOL
ScmCreateNamedPipe(VOID)
{
    DWORD dwThreadId;
    BOOL bConnected;
    HANDLE hThread;
    HANDLE hPipe;

    DPRINT("ScmCreateNamedPipe() - CreateNamedPipe(\"\\\\.\\pipe\\Ntsvcs\")\n");

    hPipe = CreateNamedPipeW(L"\\\\.\\pipe\\Ntsvcs",
              PIPE_ACCESS_DUPLEX,
              PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
              PIPE_UNLIMITED_INSTANCES,
              PIPE_BUFSIZE,
              PIPE_BUFSIZE,
              PIPE_TIMEOUT,
              NULL);
    if (hPipe == INVALID_HANDLE_VALUE)
    {
        DPRINT("CreateNamedPipe() failed (%lu)\n", GetLastError());
        return FALSE;
    }

    DPRINT("CreateNamedPipe() - calling ConnectNamedPipe(%x)\n", hPipe);
    bConnected = ConnectNamedPipe(hPipe,
                                  NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
    DPRINT("CreateNamedPipe() - ConnectNamedPipe() returned %d\n", bConnected);

    if (bConnected)
    {
        DPRINT("Pipe connected\n");
        hThread = CreateThread(NULL,
                               0,
                               ScmNamedPipeThread,
                               (LPVOID)hPipe,
                               0,
                               &dwThreadId);
        if (!hThread)
        {
            DPRINT("Could not create thread (%lu)\n", GetLastError());
            DisconnectNamedPipe(hPipe);
            CloseHandle(hPipe);
            DPRINT("CreateNamedPipe() - returning FALSE\n");
            return FALSE;
        }

        CloseHandle(hThread);
    }
    else
    {
        DPRINT("Pipe not connected\n");
        CloseHandle(hPipe);
        DPRINT("CreateNamedPipe() - returning FALSE\n");
        return FALSE;
    }
    DPRINT("CreateNamedPipe() - returning TRUE\n");
    return TRUE;
}


DWORD WINAPI
ScmNamedPipeListenerThread(LPVOID Context)
{
//    HANDLE hPipe;
    DPRINT("ScmNamedPipeListenerThread(%p) - aka SCM.\n", Context);

//    hPipe = (HANDLE)Context;
    for (;;)
    {
        DPRINT("SCM: Waiting for new connection on named pipe...\n");
        /* Create named pipe */
        if (!ScmCreateNamedPipe())
        {
            DPRINT1("\nSCM: Failed to create named pipe\n");
            break;
            //ExitThread(0);
        }
        DPRINT("\nSCM: named pipe session created.\n");
        Sleep(10);
    }
    DPRINT("\n\nWARNING: ScmNamedPipeListenerThread(%p) - Aborted.\n\n", Context);
    return ERROR_SUCCESS;
}


BOOL
StartScmNamedPipeThreadListener(VOID)
{
    DWORD dwThreadId;
    HANDLE hThread;

    hThread = CreateThread(NULL,
                           0,
                           ScmNamedPipeListenerThread,
                           NULL, /*(LPVOID)hPipe,*/
                           0,
                           &dwThreadId);
    if (!hThread)
    {
        DPRINT1("SERVICES: Could not create thread (Status %lx)\n", GetLastError());
        return FALSE;
    }

    CloseHandle(hThread);

    return TRUE;
}


BOOL WINAPI
ShutdownHandlerRoutine(DWORD dwCtrlType)
{
    DPRINT1("ShutdownHandlerRoutine() called\n");

    if (dwCtrlType & (CTRL_SHUTDOWN_EVENT | CTRL_LOGOFF_EVENT))
    {
        DPRINT1("Shutdown event received!\n");
        ScmShutdown = TRUE;

        ScmAutoShutdownServices();
        ScmShutdownServiceDatabase();

        /* Set the shutdwon event */
        SetEvent(hScmShutdownEvent);
    }

    return TRUE;
}


int WINAPI
wWinMain(HINSTANCE hInstance,
         HINSTANCE hPrevInstance,
         LPWSTR lpCmdLine,
         int nShowCmd)
{
    HANDLE hScmStartEvent = NULL;
    SC_RPC_LOCK Lock = NULL;
    BOOL bCanDeleteNamedPipeCriticalSection = FALSE;
    DWORD dwError;

    DPRINT("SERVICES: Service Control Manager\n");

    /* Create start event */
    if (!ScmCreateStartEvent(&hScmStartEvent))
    {
        DPRINT1("SERVICES: Failed to create start event\n");
        goto done;
    }

    DPRINT("SERVICES: created start event with handle %p.\n", hScmStartEvent);

    /* Create the shutdown event */
    hScmShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (hScmShutdownEvent == NULL)
    {
        DPRINT1("SERVICES: Failed to create shutdown event\n");
        goto done;
    }

    /* Initialize our communication named pipe's critical section */
    ScmInitNamedPipeCriticalSection();
    bCanDeleteNamedPipeCriticalSection = TRUE;

//    ScmInitThreadManager();

    /* FIXME: more initialization */

    /* Read the control set values */
    if (!ScmGetControlSetValues())
    {
        DPRINT1("SERVICES: failed to read the control set values\n");
        goto done;
    }

    /* Create the services database */
    dwError = ScmCreateServiceDatabase();
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("SERVICES: failed to create SCM database (Error %lu)\n", dwError);
        goto done;
    }

    /* Update the services database */
    ScmGetBootAndSystemDriverState();

    /* Register the Service Control Manager process with CSRSS */
    if (!RegisterServicesProcess(GetCurrentProcessId()))
    {
        DPRINT1("SERVICES: Could not register SCM process\n");
        goto done;
    }

    /* Acquire the service start lock until autostart services have been started */
    dwError = ScmAcquireServiceStartLock(TRUE, &Lock);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("SERVICES: failed to acquire the service start lock (Error %lu)\n", dwError);
        goto done;
    }

    /* Start the RPC server */
    ScmStartRpcServer();

    DPRINT("SERVICES: Initialized.\n");

    /* Signal start event */
    SetEvent(hScmStartEvent);

    /* Register event handler (used for system shutdown) */
    SetConsoleCtrlHandler(ShutdownHandlerRoutine, TRUE);

    /* Wait for the LSA server */
    ScmWaitForLsa();

    /* Start auto-start services */
    ScmAutoStartServices();

    /* FIXME: more to do ? */

    /* Release the service start lock */
    ScmReleaseServiceStartLock(&Lock);

    DPRINT("SERVICES: Running.\n");

    /* Wait until the shutdown event gets signaled */
    WaitForSingleObject(hScmShutdownEvent, INFINITE);

done:
    /* Delete our communication named pipe's critical section */
    if (bCanDeleteNamedPipeCriticalSection == TRUE)
        ScmDeleteNamedPipeCriticalSection();

    /* Close the shutdown event */
    if (hScmShutdownEvent != NULL)
        CloseHandle(hScmShutdownEvent);

    /* Close the start event */
    if (hScmStartEvent != NULL)
        CloseHandle(hScmStartEvent);

    DPRINT("SERVICES: Finished.\n");

    ExitThread(0);

    return 0;
}

/* EOF */
