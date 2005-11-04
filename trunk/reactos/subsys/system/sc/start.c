/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS SC utility
 * FILE:        subsys/system/sc/start.c
 * PURPOSE:     control ReactOS services
 * PROGRAMMERS: Ged Murphy (gedmurphy@gmail.com)
 * REVISIONS:
 *           Ged Murphy 20/10/05 Created
 *
 */

#include "sc.h"

BOOL Start(LPCTSTR ServiceName, LPCTSTR *ServiceArgs, INT ArgCount)
{
    SC_HANDLE hSc;
    SERVICE_STATUS_PROCESS ServiceStatus;
    DWORD BytesNeeded;

    /* testing */
    _tprintf(_T("service to start - %s\n\n"), ServiceName);
    _tprintf(_T("Arguments :\n"));
    while (*ServiceArgs)
    {
        printf("%s\n", *ServiceArgs);
        ServiceArgs++;
    }
    

    /* get a handle to the service requested for starting */
    hSc = OpenService(hSCManager, ServiceName, SERVICE_ALL_ACCESS);

    if (hSc == NULL)
    {
        _tprintf(_T("openService failed\n"));
        ReportLastError();
        return FALSE;
    }

    /* start the service opened */
    if (! StartService(hSc, ArgCount, ServiceArgs))
    {
        _tprintf(_T("StartService failed\n"));
        ReportLastError();
        return FALSE;
    }
    
    if (! QueryServiceStatusEx(
            hSc,
            SC_STATUS_PROCESS_INFO,
            (LPBYTE)&ServiceStatus,
            sizeof(SERVICE_STATUS_PROCESS),
            &BytesNeeded))
    {
        _tprintf(_T("QueryServiceStatusEx 1 failed\n"));
        ReportLastError();
        return FALSE;
    }

    
    while (ServiceStatus.dwCurrentState == SERVICE_START_PENDING)
    {
        /* wait before checking status */
        Sleep(ServiceStatus.dwWaitHint);
        
        /* check status again */
        if (! QueryServiceStatusEx(
                hSc,
                SC_STATUS_PROCESS_INFO,
                (LPBYTE)&ServiceStatus,
                sizeof(SERVICE_STATUS_PROCESS),
                &BytesNeeded))
        {
            _tprintf(_T("QueryServiceStatusEx 2 failed\n"));
            ReportLastError();
            return FALSE;
        }
    }

    CloseServiceHandle(hSc);
    
    if (ServiceStatus.dwCurrentState == SERVICE_RUNNING)
    {
        _tprintf(_T("%s is running\n"), ServiceName);
        return TRUE;
    }
    else
    {
        _tprintf(_T("Failed to start %s\n"), ServiceName);
        _tprintf(_T("Curent state: %lu\n"), ServiceStatus.dwCurrentState);
        _tprintf(_T("Exit code: %lu\n"), ServiceStatus.dwWin32ExitCode);
        _tprintf(_T("Service Specific exit code: %lu\n"),
                ServiceStatus.dwServiceSpecificExitCode);
        _tprintf(_T("Check point: %lu\n"), ServiceStatus.dwCheckPoint);
        _tprintf(_T("Wait hint: %lu\n"), ServiceStatus.dwWaitHint);
        
        return FALSE;
    }
}
