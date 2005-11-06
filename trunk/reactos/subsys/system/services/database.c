/*
 *
 * service control manager
 *
 * ReactOS Operating System
 *
 * --------------------------------------------------------------------
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.LIB. If not, write
 * to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge,
 * MA 02139, USA.
 *
 */

/* INCLUDES *****************************************************************/

#include "services.h"

#define NDEBUG
#include <debug.h>


/* TYPES *********************************************************************/

typedef struct _SERVICE_GROUP
{
  LIST_ENTRY GroupListEntry;
  UNICODE_STRING GroupName;

  BOOLEAN ServicesRunning;
  ULONG TagCount;
  PULONG TagArray;

} SERVICE_GROUP, *PSERVICE_GROUP;


/* GLOBALS *******************************************************************/

LIST_ENTRY GroupListHead;
LIST_ENTRY ServiceListHead;

static RTL_RESOURCE DatabaseLock;


/* FUNCTIONS *****************************************************************/

PSERVICE
ScmGetServiceEntryByName(LPWSTR lpServiceName)
{
    PLIST_ENTRY ServiceEntry;
    PSERVICE CurrentService;

    DPRINT("ScmGetServiceEntryByName() called\n");

    ServiceEntry = ServiceListHead.Flink;
    while (ServiceEntry != &ServiceListHead)
    {
        CurrentService = CONTAINING_RECORD(ServiceEntry,
                                           SERVICE,
                                           ServiceListEntry);
        if (_wcsicmp(CurrentService->lpServiceName, lpServiceName) == 0)
        {
            DPRINT("Found service: '%S'\n", CurrentService->lpServiceName);
            return CurrentService;
        }

        ServiceEntry = ServiceEntry->Flink;
    }

    DPRINT("Couldn't find a matching service\n");

    return NULL;
}


PSERVICE
ScmGetServiceEntryByDisplayName(LPWSTR lpDisplayName)
{
    PLIST_ENTRY ServiceEntry;
    PSERVICE CurrentService;

    DPRINT("ScmGetServiceEntryByDisplayName() called\n");

    ServiceEntry = ServiceListHead.Flink;
    while (ServiceEntry != &ServiceListHead)
    {
        CurrentService = CONTAINING_RECORD(ServiceEntry,
                                           SERVICE,
                                           ServiceListEntry);
        if (_wcsicmp(CurrentService->lpDisplayName, lpDisplayName) == 0)
        {
            DPRINT("Found service: '%S'\n", CurrentService->lpDisplayName);
            return CurrentService;
        }

        ServiceEntry = ServiceEntry->Flink;
    }

    DPRINT("Couldn't find a matching service\n");

    return NULL;
}


static NTSTATUS STDCALL
CreateGroupOrderListRoutine(PWSTR ValueName,
                            ULONG ValueType,
                            PVOID ValueData,
                            ULONG ValueLength,
                            PVOID Context,
                            PVOID EntryContext)
{
    PSERVICE_GROUP Group;

    DPRINT("CreateGroupOrderListRoutine(%S, %x, %x, %x, %x, %x)\n",
           ValueName, ValueType, ValueData, ValueLength, Context, EntryContext);

    if (ValueType == REG_BINARY &&
        ValueData != NULL &&
        ValueLength >= sizeof(DWORD) &&
        ValueLength >= (*(PULONG)ValueData + 1) * sizeof(DWORD))
    {
        Group = (PSERVICE_GROUP)Context;
        Group->TagCount = ((PULONG)ValueData)[0];
        if (Group->TagCount > 0)
        {
            if (ValueLength >= (Group->TagCount + 1) * sizeof(DWORD))
            {
                Group->TagArray = (PULONG)HeapAlloc(GetProcessHeap(),
                                                    HEAP_ZERO_MEMORY,
                                                    Group->TagCount * sizeof(DWORD));
                if (Group->TagArray == NULL)
                {
                    Group->TagCount = 0;
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                RtlCopyMemory(Group->TagArray,
                              (PULONG)ValueData + 1,
                              Group->TagCount * sizeof(DWORD));
            }
            else
            {
                Group->TagCount = 0;
                return STATUS_UNSUCCESSFUL;
            }
        }
    }

    return STATUS_SUCCESS;
}


static NTSTATUS STDCALL
CreateGroupListRoutine(PWSTR ValueName,
                       ULONG ValueType,
                       PVOID ValueData,
                       ULONG ValueLength,
                       PVOID Context,
                       PVOID EntryContext)
{
    PSERVICE_GROUP Group;
    RTL_QUERY_REGISTRY_TABLE QueryTable[2];
    NTSTATUS Status;

    if (ValueType == REG_SZ)
    {
        DPRINT("Data: '%S'\n", (PWCHAR)ValueData);

        Group = (PSERVICE_GROUP)HeapAlloc(GetProcessHeap(),
                                          HEAP_ZERO_MEMORY,
                                          sizeof(SERVICE_GROUP));
        if (Group == NULL)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        if (!RtlCreateUnicodeString(&Group->GroupName,
                                    (PWSTR)ValueData))
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlZeroMemory(&QueryTable, sizeof(QueryTable));
        QueryTable[0].Name = (PWSTR)ValueData;
        QueryTable[0].QueryRoutine = CreateGroupOrderListRoutine;

        Status = RtlQueryRegistryValues(RTL_REGISTRY_CONTROL,
                                        L"GroupOrderList",
                                        QueryTable,
                                        (PVOID)Group,
                                        NULL);
        DPRINT("%x %d %S\n", Status, Group->TagCount, (PWSTR)ValueData);

        InsertTailList(&GroupListHead,
                       &Group->GroupListEntry);
    }

    return STATUS_SUCCESS;
}


DWORD
ScmCreateNewServiceRecord(LPWSTR lpServiceName,
                          PSERVICE *lpServiceRecord)
{
    PSERVICE lpService = NULL;

    DPRINT("Service: '%S'\n", lpServiceName);

    /* Allocate service entry */
    lpService = HeapAlloc(GetProcessHeap(),
                          HEAP_ZERO_MEMORY,
                          sizeof(SERVICE) + ((wcslen(lpServiceName) + 1) * sizeof(WCHAR)));
    if (lpService == NULL)
        return ERROR_NOT_ENOUGH_MEMORY;

    *lpServiceRecord = lpService;

    /* Copy service name */
    wcscpy(lpService->szServiceName, lpServiceName);
    lpService->lpServiceName = lpService->szServiceName;
    lpService->lpDisplayName = lpService->lpServiceName;

    /* Append service entry */
    InsertTailList(&ServiceListHead,
                   &lpService->ServiceListEntry);

    lpService->Status.dwCurrentState = SERVICE_STOPPED;
    lpService->Status.dwControlsAccepted = 0;
    lpService->Status.dwWin32ExitCode = ERROR_SERVICE_NEVER_STARTED;
    lpService->Status.dwServiceSpecificExitCode = 0;
    lpService->Status.dwCheckPoint = 0;
    lpService->Status.dwWaitHint = 2000; /* 2 seconds */

    return ERROR_SUCCESS;
}


static DWORD
CreateServiceListEntry(LPWSTR lpServiceName,
                       HKEY hServiceKey)
{
    PSERVICE lpService = NULL;
    LPWSTR lpDisplayName = NULL;
    LPWSTR lpGroup = NULL;
    DWORD dwSize;
    DWORD dwError;
    DWORD dwServiceType;
    DWORD dwStartType;
    DWORD dwErrorControl;
    DWORD dwTagId;

    DPRINT("Service: '%S'\n", lpServiceName);
    if (*lpServiceName == L'{')
        return ERROR_SUCCESS;

    dwSize = sizeof(DWORD);
    dwError = RegQueryValueExW(hServiceKey,
                               L"Type",
                               NULL,
                               NULL,
                               (LPBYTE)&dwServiceType,
                               &dwSize);
    if (dwError != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    if (((dwServiceType & ~SERVICE_INTERACTIVE_PROCESS) != SERVICE_WIN32_OWN_PROCESS) &&
        ((dwServiceType & ~SERVICE_INTERACTIVE_PROCESS) != SERVICE_WIN32_SHARE_PROCESS) &&
        (dwServiceType != SERVICE_KERNEL_DRIVER) &&
        (dwServiceType != SERVICE_FILE_SYSTEM_DRIVER))
        return ERROR_SUCCESS;

    DPRINT("Service type: %lx\n", dwServiceType);

    dwSize = sizeof(DWORD);
    dwError = RegQueryValueExW(hServiceKey,
                               L"Start",
                               NULL,
                               NULL,
                               (LPBYTE)&dwStartType,
                               &dwSize);
    if (dwError != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    DPRINT("Start type: %lx\n", dwStartType);

    dwSize = sizeof(DWORD);
    dwError = RegQueryValueExW(hServiceKey,
                               L"ErrorControl",
                               NULL,
                               NULL,
                               (LPBYTE)&dwErrorControl,
                               &dwSize);
    if (dwError != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    DPRINT("Error control: %lx\n", dwErrorControl);

    dwError = RegQueryValueExW(hServiceKey,
                               L"Tag",
                               NULL,
                               NULL,
                               (LPBYTE)&dwTagId,
                               &dwSize);
    if (dwError != ERROR_SUCCESS)
        dwTagId = 0;

    DPRINT("Tag: %lx\n", dwTagId);

    dwError = ScmReadString(hServiceKey,
                            L"Group",
                            &lpGroup);
    if (dwError != ERROR_SUCCESS)
        lpGroup = NULL;

    DPRINT("Group: %S\n", lpGroup);

    dwError = ScmReadString(hServiceKey,
                            L"DisplayName",
                            &lpDisplayName);
    if (dwError != ERROR_SUCCESS)
        lpDisplayName = NULL;

    DPRINT("Display name: %S\n", lpDisplayName);

    dwError = ScmCreateNewServiceRecord(lpServiceName,
                                        &lpService);
    if (dwError != ERROR_SUCCESS)
        goto done;

    lpService->Status.dwServiceType = dwServiceType;
    lpService->dwStartType = dwStartType;
    lpService->dwErrorControl = dwErrorControl;
    lpService->dwTag = dwTagId;

    if (lpGroup != NULL)
    {
        lpService->lpServiceGroup = lpGroup;
        lpGroup = NULL;
    }

    if (lpDisplayName != NULL)
    {
        lpService->lpDisplayName = lpDisplayName;
        lpDisplayName = NULL;
    }

    DPRINT("ServiceName: '%S'\n", lpService->lpServiceName);
    DPRINT("Group: '%S'\n", lpService->lpServiceGroup);
    DPRINT("Start %lx  Type %lx  Tag %lx  ErrorControl %lx\n",
           lpService->dwStartType,
           lpService->Status.dwServiceType,
           lpService->dwTag,
           lpService->dwErrorControl);

    if (ScmIsDeleteFlagSet(hServiceKey))
        lpService->bDeleted = TRUE;

done:;
    if (lpGroup != NULL)
        HeapFree(GetProcessHeap(), 0, lpGroup);

    if (lpDisplayName != NULL)
        HeapFree(GetProcessHeap(), 0, lpDisplayName);

    return dwError;
}


DWORD
ScmReadGroupList(VOID)
{
    RTL_QUERY_REGISTRY_TABLE QueryTable[2];
    NTSTATUS Status;

    InitializeListHead(&GroupListHead);

    /* Build group order list */
    RtlZeroMemory(&QueryTable,
                  sizeof(QueryTable));

    QueryTable[0].Name = L"List";
    QueryTable[0].QueryRoutine = CreateGroupListRoutine;

    Status = RtlQueryRegistryValues(RTL_REGISTRY_CONTROL,
                                    L"ServiceGroupOrder",
                                    QueryTable,
                                    NULL,
                                    NULL);

    return RtlNtStatusToDosError(Status);
}


DWORD
ScmCreateServiceDatabase(VOID)
{
    WCHAR szSubKey[MAX_PATH];
    HKEY hServicesKey;
    HKEY hServiceKey;
    DWORD dwSubKey;
    DWORD dwSubKeyLength;
    FILETIME ftLastChanged;
    DWORD dwError;

    DPRINT("ScmCreateServiceDatabase() called\n");

    dwError = ScmReadGroupList();
    if (dwError != ERROR_SUCCESS)
        return dwError;

    /* Initialize basic variables */
    InitializeListHead(&ServiceListHead);

    /* Initialize the database lock */
    RtlInitializeResource(&DatabaseLock);

    dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                            L"System\\CurrentControlSet\\Services",
                            0,
                            KEY_READ,
                            &hServicesKey);
    if (dwError != ERROR_SUCCESS)
        return dwError;

    dwSubKey = 0;
    for (;;)
    {
        dwSubKeyLength = MAX_PATH;
        dwError = RegEnumKeyExW(hServicesKey,
                                dwSubKey,
                                szSubKey,
                                &dwSubKeyLength,
                                NULL,
                                NULL,
                                NULL,
                                &ftLastChanged);
        if (dwError == ERROR_SUCCESS &&
            szSubKey[0] != L'{')
        {
            DPRINT("SubKeyName: '%S'\n", szSubKey);

            dwError = RegOpenKeyExW(hServicesKey,
                                    szSubKey,
                                    0,
                                    KEY_READ,
                                    &hServiceKey);
            if (dwError == ERROR_SUCCESS)
            {
                dwError = CreateServiceListEntry(szSubKey,
                                                 hServiceKey);

                RegCloseKey(hServiceKey);
            }
        }

        if (dwError != ERROR_SUCCESS)
            break;

        dwSubKey++;
    }

    RegCloseKey(hServicesKey);

    /* FIXME: Delete services that are marked for delete */

    DPRINT("ScmCreateServiceDatabase() done\n");

    return ERROR_SUCCESS;
}


static NTSTATUS
ScmCheckDriver(PSERVICE Service)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING DirName;
    HANDLE DirHandle;
    NTSTATUS Status;
    POBJECT_DIRECTORY_INFORMATION DirInfo;
    ULONG BufferLength;
    ULONG DataLength;
    ULONG Index;
    PLIST_ENTRY GroupEntry;
    PSERVICE_GROUP CurrentGroup;

    DPRINT("ScmCheckDriver(%S) called\n", Service->lpServiceName);

    if (Service->Status.dwServiceType == SERVICE_KERNEL_DRIVER)
    {
        RtlInitUnicodeString(&DirName,
                             L"\\Driver");
    }
    else
    {
        RtlInitUnicodeString(&DirName,
                             L"\\FileSystem");
    }

    InitializeObjectAttributes(&ObjectAttributes,
                               &DirName,
                               0,
                               NULL,
                               NULL);

    Status = NtOpenDirectoryObject(&DirHandle,
                                   DIRECTORY_QUERY | DIRECTORY_TRAVERSE,
                                   &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    BufferLength = sizeof(OBJECT_DIRECTORY_INFORMATION) +
                   2 * MAX_PATH * sizeof(WCHAR);
    DirInfo = HeapAlloc(GetProcessHeap(),
                        HEAP_ZERO_MEMORY,
                        BufferLength);

    Index = 0;
    while (TRUE)
    {
        Status = NtQueryDirectoryObject(DirHandle,
                                        DirInfo,
                                        BufferLength,
                                        TRUE,
                                        FALSE,
                                        &Index,
                                        &DataLength);
        if (Status == STATUS_NO_MORE_ENTRIES)
        {
            /* FIXME: Add current service to 'failed service' list */
            DPRINT("Service '%S' failed\n", Service->lpServiceName);
            break;
        }

        if (!NT_SUCCESS(Status))
            break;

        DPRINT("Comparing: '%S'  '%wZ'\n", Service->lpServiceName, &DirInfo->ObjectName);

        if (_wcsicmp(Service->lpServiceName, DirInfo->ObjectName.Buffer) == 0)
        {
            DPRINT("Found: '%S'  '%wZ'\n",
                   Service->lpServiceName, &DirInfo->ObjectName);

            /* Mark service as 'running' */
            Service->Status.dwCurrentState = SERVICE_RUNNING;

            /* Find the driver's group and mark it as 'running' */
            if (Service->lpServiceGroup != NULL)
            {
                GroupEntry = GroupListHead.Flink;
                while (GroupEntry != &GroupListHead)
                {
                    CurrentGroup = CONTAINING_RECORD(GroupEntry, SERVICE_GROUP, GroupListEntry);

                    DPRINT("Checking group '%wZ'\n", &CurrentGroup->GroupName);
                    if (Service->lpServiceGroup != NULL &&
                        _wcsicmp(Service->lpServiceGroup, CurrentGroup->GroupName.Buffer) == 0)
                    {
                        CurrentGroup->ServicesRunning = TRUE;
                    }

                    GroupEntry = GroupEntry->Flink;
                }
            }
            break;
        }
    }

    HeapFree(GetProcessHeap(),
             0,
             DirInfo);
    NtClose(DirHandle);

    return STATUS_SUCCESS;
}


VOID
ScmGetBootAndSystemDriverState(VOID)
{
    PLIST_ENTRY ServiceEntry;
    PSERVICE CurrentService;

    DPRINT("ScmGetBootAndSystemDriverState() called\n");

    ServiceEntry = ServiceListHead.Flink;
    while (ServiceEntry != &ServiceListHead)
    {
        CurrentService = CONTAINING_RECORD(ServiceEntry, SERVICE, ServiceListEntry);

        if (CurrentService->dwStartType == SERVICE_BOOT_START ||
            CurrentService->dwStartType == SERVICE_SYSTEM_START)
        {
            /* Check driver */
            DPRINT("  Checking service: %S\n", CurrentService->lpServiceName);

            ScmCheckDriver(CurrentService);
        }

        ServiceEntry = ServiceEntry->Flink;
    }

    DPRINT("ScmGetBootAndSystemDriverState() done\n");
}


static NTSTATUS
ScmSendStartCommand(PSERVICE Service, LPWSTR Arguments)
{
    PSCM_START_PACKET StartPacket;
    DWORD TotalLength;
#if 0
    DWORD Length;
#endif
    PWSTR Ptr;
    DWORD Count;

    DPRINT("ScmSendStartCommand() called\n");

    /* Calculate the total length of the start command line */
    TotalLength = wcslen(Service->lpServiceName) + 1;
#if 0
    if (Arguments != NULL)
    {
        Ptr = Arguments;
        while (*Ptr)
        {
            Length = wcslen(Ptr) + 1;
            TotalLength += Length;
            Ptr += Length;
        }
    }
#endif
    TotalLength++;

    /* Allocate start command packet */
    StartPacket = HeapAlloc(GetProcessHeap(),
                            HEAP_ZERO_MEMORY,
                            sizeof(SCM_START_PACKET) + (TotalLength - 1) * sizeof(WCHAR));
    if (StartPacket == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    StartPacket->Command = SCM_START_COMMAND;
    StartPacket->Size = TotalLength;
    Ptr = &StartPacket->Arguments[0];
    wcscpy(Ptr, Service->lpServiceName);
    Ptr += (wcslen(Service->lpServiceName) + 1);

    /* FIXME: Copy argument list */

    *Ptr = 0;

    /* Send the start command */
    WriteFile(Service->ControlPipeHandle,
              StartPacket,
              sizeof(SCM_START_PACKET) + (TotalLength - 1) * sizeof(WCHAR),
              &Count,
              NULL);

    /* FIXME: Read the reply */

    HeapFree(GetProcessHeap(),
             0,
             StartPacket);

    DPRINT("ScmSendStartCommand() done\n");

    return STATUS_SUCCESS;
}


static NTSTATUS
ScmStartUserModeService(PSERVICE Service)
{
    RTL_QUERY_REGISTRY_TABLE QueryTable[3];
    PROCESS_INFORMATION ProcessInformation;
    STARTUPINFOW StartupInfo;
    UNICODE_STRING ImagePath;
    ULONG Type;
    BOOL Result;
    NTSTATUS Status;

    RtlInitUnicodeString(&ImagePath, NULL);

    /* Get service data */
    RtlZeroMemory(&QueryTable,
                  sizeof(QueryTable));

    QueryTable[0].Name = L"Type";
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;
    QueryTable[0].EntryContext = &Type;

    QueryTable[1].Name = L"ImagePath";
    QueryTable[1].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;
    QueryTable[1].EntryContext = &ImagePath;

    Status = RtlQueryRegistryValues(RTL_REGISTRY_SERVICES,
                                    Service->lpServiceName,
                                    QueryTable,
                                    NULL,
                                    NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("RtlQueryRegistryValues() failed (Status %lx)\n", Status);
        return Status;
    }
    DPRINT("ImagePath: '%S'\n", ImagePath.Buffer);
    DPRINT("Type: %lx\n", Type);

    /* Create '\\.\pipe\net\NtControlPipe' instance */
    Service->ControlPipeHandle = CreateNamedPipeW(L"\\\\.\\pipe\\net\\NtControlPipe",
                                                  PIPE_ACCESS_DUPLEX,
                                                  PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                                                  100,
                                                  8000,
                                                  4,
                                                  30000,
                                                  NULL);
    DPRINT("CreateNamedPipeW() done\n");
    if (Service->ControlPipeHandle == INVALID_HANDLE_VALUE)
    {
        DPRINT1("Failed to create control pipe!\n");
        return STATUS_UNSUCCESSFUL;
    }

    StartupInfo.cb = sizeof(StartupInfo);
    StartupInfo.lpReserved = NULL;
    StartupInfo.lpDesktop = NULL;
    StartupInfo.lpTitle = NULL;
    StartupInfo.dwFlags = 0;
    StartupInfo.cbReserved2 = 0;
    StartupInfo.lpReserved2 = 0;

    Result = CreateProcessW(ImagePath.Buffer,
                            NULL,
                            NULL,
                            NULL,
                            FALSE,
                            DETACHED_PROCESS | CREATE_SUSPENDED,
                            NULL,
                            NULL,
                            &StartupInfo,
                            &ProcessInformation);
    RtlFreeUnicodeString(&ImagePath);

    if (!Result)
    {
        /* Close control pipe */
        CloseHandle(Service->ControlPipeHandle);
        Service->ControlPipeHandle = INVALID_HANDLE_VALUE;

        DPRINT1("Starting '%S' failed!\n", Service->lpServiceName);
        return STATUS_UNSUCCESSFUL;
    }

    DPRINT("Process Id: %lu  Handle %lx\n",
           ProcessInformation.dwProcessId,
           ProcessInformation.hProcess);
    DPRINT("Thread Id: %lu  Handle %lx\n",
           ProcessInformation.dwThreadId,
           ProcessInformation.hThread);

    /* Get process and thread ids */
    Service->ProcessId = ProcessInformation.dwProcessId;
    Service->ThreadId = ProcessInformation.dwThreadId;

    /* Resume Thread */
    ResumeThread(ProcessInformation.hThread);

    /* Connect control pipe */
    if (ConnectNamedPipe(Service->ControlPipeHandle, NULL))
    {
        DWORD dwProcessId = 0;
        DWORD dwRead = 0;

        DPRINT("Control pipe connected!\n");

        /* Read thread id from pipe */
        if (!ReadFile(Service->ControlPipeHandle,
                      (LPVOID)&dwProcessId,
                      sizeof(DWORD),
                      &dwRead,
                      NULL))
        {
            DPRINT1("Reading the service control pipe failed (Error %lu)\n",
                    GetLastError());
            Status = STATUS_UNSUCCESSFUL;
        }
        else
        {
            DPRINT("Received process id %lu\n", dwProcessId);

            /* FIXME: Send start command */

            Status = STATUS_SUCCESS;
        }
    }
    else
    {
        DPRINT("Connecting control pipe failed!\n");

        /* Close control pipe */
        CloseHandle(Service->ControlPipeHandle);
        Service->ControlPipeHandle = INVALID_HANDLE_VALUE;
        Service->ProcessId = 0;
        Service->ThreadId = 0;
        Status = STATUS_UNSUCCESSFUL;
    }

    ScmSendStartCommand(Service, NULL);

    /* Close process and thread handle */
    CloseHandle(ProcessInformation.hThread);
    CloseHandle(ProcessInformation.hProcess);

    return Status;
}


static NTSTATUS
ScmStartService(PSERVICE Service,
                PSERVICE_GROUP Group)
{
    WCHAR szDriverPath[MAX_PATH];
    UNICODE_STRING DriverPath;
    NTSTATUS Status;

    DPRINT("ScmStartService() called\n");

    Service->ControlPipeHandle = INVALID_HANDLE_VALUE;
    DPRINT("Service->Type: %lu\n", Service->Status.dwServiceType);

    if (Service->Status.dwServiceType == SERVICE_KERNEL_DRIVER ||
        Service->Status.dwServiceType == SERVICE_FILE_SYSTEM_DRIVER ||
        Service->Status.dwServiceType == SERVICE_RECOGNIZER_DRIVER)
    {
        /* Load driver */
        wcscpy(szDriverPath,
               L"\\Registry\\Machine\\System\\CurrentControlSet\\Services");
        wcscat(szDriverPath,
               Service->lpServiceName);

        RtlInitUnicodeString(&DriverPath,
                             szDriverPath);

        DPRINT("  Path: %wZ\n", &DriverPath);
        Status = NtLoadDriver(&DriverPath);
    }
    else
    {
        /* Start user-mode service */
        Status = ScmStartUserModeService(Service);
    }

    DPRINT("ScmStartService() done (Status %lx)\n", Status);

    if (NT_SUCCESS(Status))
    {
        if (Group != NULL)
        {
            Group->ServicesRunning = TRUE;
        }
        Service->Status.dwCurrentState = SERVICE_RUNNING;
    }
#if 0
    else
    {
        switch (Service->ErrorControl)
        {
            case SERVICE_ERROR_NORMAL:
                /* FIXME: Log error */
                break;

            case SERVICE_ERROR_SEVERE:
                if (IsLastKnownGood == FALSE)
                {
                    /* FIXME: Boot last known good configuration */
                }
                break;

            case SERVICE_ERROR_CRITICAL:
                if (IsLastKnownGood == FALSE)
                {
                    /* FIXME: Boot last known good configuration */
                }
                else
                {
                    /* FIXME: BSOD! */
                }
                break;
        }
    }
#endif

    return Status;
}


VOID
ScmAutoStartServices(VOID)
{
    PLIST_ENTRY GroupEntry;
    PLIST_ENTRY ServiceEntry;
    PSERVICE_GROUP CurrentGroup;
    PSERVICE CurrentService;
    ULONG i;

    /* Clear 'ServiceVisited' flag */
    ServiceEntry = ServiceListHead.Flink;
    while (ServiceEntry != &ServiceListHead)
    {
      CurrentService = CONTAINING_RECORD(ServiceEntry, SERVICE, ServiceListEntry);
      CurrentService->ServiceVisited = FALSE;
      ServiceEntry = ServiceEntry->Flink;
    }

    /* Start all services which are members of an existing group */
    GroupEntry = GroupListHead.Flink;
    while (GroupEntry != &GroupListHead)
    {
        CurrentGroup = CONTAINING_RECORD(GroupEntry, SERVICE_GROUP, GroupListEntry);

        DPRINT("Group '%wZ'\n", &CurrentGroup->GroupName);

        /* Start all services witch have a valid tag */
        for (i = 0; i < CurrentGroup->TagCount; i++)
        {
            ServiceEntry = ServiceListHead.Flink;
            while (ServiceEntry != &ServiceListHead)
            {
                CurrentService = CONTAINING_RECORD(ServiceEntry, SERVICE, ServiceListEntry);

                if ((CurrentService->lpServiceGroup != NULL) &&
                    (_wcsicmp(CurrentGroup->GroupName.Buffer, CurrentService->lpServiceGroup) == 0) &&
                    (CurrentService->dwStartType == SERVICE_AUTO_START) &&
                    (CurrentService->ServiceVisited == FALSE) &&
                    (CurrentService->dwTag == CurrentGroup->TagArray[i]))
                {
                    CurrentService->ServiceVisited = TRUE;
                    ScmStartService(CurrentService,
                                    CurrentGroup);
                }

                ServiceEntry = ServiceEntry->Flink;
             }
        }

        /* Start all services which have an invalid tag or which do not have a tag */
        ServiceEntry = ServiceListHead.Flink;
        while (ServiceEntry != &ServiceListHead)
        {
            CurrentService = CONTAINING_RECORD(ServiceEntry, SERVICE, ServiceListEntry);

            if ((CurrentService->lpServiceGroup != NULL) &&
                (_wcsicmp(CurrentGroup->GroupName.Buffer, CurrentService->lpServiceGroup) == 0) &&
                (CurrentService->dwStartType == SERVICE_AUTO_START) &&
                (CurrentService->ServiceVisited == FALSE))
            {
                CurrentService->ServiceVisited = TRUE;
                ScmStartService(CurrentService,
                                CurrentGroup);
            }

            ServiceEntry = ServiceEntry->Flink;
        }

        GroupEntry = GroupEntry->Flink;
    }

    /* Start all services which are members of any non-existing group */
    ServiceEntry = ServiceListHead.Flink;
    while (ServiceEntry != &ServiceListHead)
    {
        CurrentService = CONTAINING_RECORD(ServiceEntry, SERVICE, ServiceListEntry);

        if ((CurrentService->lpServiceGroup != NULL) &&
            (CurrentService->dwStartType == SERVICE_AUTO_START) &&
            (CurrentService->ServiceVisited == FALSE))
        {
            CurrentService->ServiceVisited = TRUE;
            ScmStartService(CurrentService,
                            NULL);
        }

        ServiceEntry = ServiceEntry->Flink;
    }

    /* Start all services which are not a member of any group */
    ServiceEntry = ServiceListHead.Flink;
    while (ServiceEntry != &ServiceListHead)
    {
        CurrentService = CONTAINING_RECORD(ServiceEntry, SERVICE, ServiceListEntry);

        if ((CurrentService->lpServiceGroup == NULL) &&
            (CurrentService->dwStartType == SERVICE_AUTO_START) &&
            (CurrentService->ServiceVisited == FALSE))
        {
            CurrentService->ServiceVisited = TRUE;
            ScmStartService(CurrentService,
                            NULL);
        }

        ServiceEntry = ServiceEntry->Flink;
    }

    /* Clear 'ServiceVisited' flag again */
    ServiceEntry = ServiceListHead.Flink;
    while (ServiceEntry != &ServiceListHead)
    {
        CurrentService = CONTAINING_RECORD(ServiceEntry, SERVICE, ServiceListEntry);
        CurrentService->ServiceVisited = FALSE;
        ServiceEntry = ServiceEntry->Flink;
    }
}

/* EOF */
