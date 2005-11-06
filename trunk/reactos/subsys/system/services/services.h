/*
 * services.h
 */

#include <stdio.h>
#define WIN32_NO_STATUS
#include <windows.h>
#define NTOS_MODE_USER
#include <ndk/ntndk.h>
#include <services/services.h>

typedef struct _SERVICE
{
    LIST_ENTRY ServiceListEntry;
    LPWSTR lpServiceName;
    LPWSTR lpDisplayName;
    LPWSTR lpServiceGroup;
    BOOL bDeleted;

    SERVICE_STATUS Status;
    DWORD dwStartType;
    DWORD dwErrorControl;
    DWORD dwTag;

    ULONG Flags;

    BOOLEAN ServiceVisited;

    HANDLE ControlPipeHandle;
    ULONG ProcessId;
    ULONG ThreadId;

    WCHAR szServiceName[1];
} SERVICE, *PSERVICE;


/* VARIABLES ***************************************************************/

extern BOOL ScmShutdown;


/* FUNCTIONS ***************************************************************/

/* config.c */

DWORD ScmOpenServiceKey(LPWSTR lpServiceName,
                        REGSAM samDesired,
                        PHKEY phKey);

DWORD ScmCreateServiceKey(LPWSTR lpServiceName,
                          REGSAM samDesired,
                          PHKEY phKey);

DWORD ScmWriteDependencies(HKEY hServiceKey,
                           LPWSTR lpDependencies,
                           DWORD dwDependenciesLength);

DWORD ScmMarkServiceForDelete(PSERVICE pService);
BOOL ScmIsDeleteFlagSet(HKEY hServiceKey);

DWORD ScmReadString(HKEY hServiceKey,
                    LPWSTR lpValueName,
                    LPWSTR *lpValue);


/* database.c */

DWORD ScmCreateServiceDatabase(VOID);
VOID ScmGetBootAndSystemDriverState(VOID);
VOID ScmAutoStartServices(VOID);

PSERVICE ScmGetServiceEntryByName(LPWSTR lpServiceName);
PSERVICE ScmGetServiceEntryByDisplayName(LPWSTR lpDisplayName);
DWORD ScmCreateNewServiceRecord(LPWSTR lpServiceName,
                                PSERVICE *lpServiceRecord);
DWORD ScmMarkServiceForDelete(PSERVICE pService);


/* rpcserver.c */

VOID ScmStartRpcServer(VOID);


/* services.c */

VOID PrintString(LPCSTR fmt, ...);



/* EOF */

