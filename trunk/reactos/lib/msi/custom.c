/*
 * Custom Action processing for the Microsoft Installer (msi.dll)
 *
 * Copyright 2005 Aric Stewart for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * Pages I need
 *
http://msdn.microsoft.com/library/default.asp?url=/library/en-us/msi/setup/summary_list_of_all_custom_action_types.asp
 */

#include <stdarg.h>
#include <stdio.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winreg.h"
#include "wine/debug.h"
#include "fdi.h"
#include "msi.h"
#include "msidefs.h"
#include "msiquery.h"
#include "fcntl.h"
#include "objbase.h"
#include "objidl.h"
#include "msipriv.h"
#include "winnls.h"
#include "winuser.h"
#include "shlobj.h"
#include "wine/unicode.h"
#include "winver.h"
#include "action.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);

#define CUSTOM_ACTION_TYPE_MASK 0x3F
static const WCHAR c_collen[] = {'C',':','\\',0};
static const WCHAR cszTempFolder[]= {'T','e','m','p','F','o','l','d','e','r',0};

typedef struct tagMSIRUNNINGACTION
{
    HANDLE handle;
    BOOL   process;
    LPWSTR name;
} MSIRUNNINGACTION;

static UINT HANDLE_CustomType1(MSIPACKAGE *package, LPCWSTR source,
                               LPCWSTR target, const INT type, LPCWSTR action);
static UINT HANDLE_CustomType2(MSIPACKAGE *package, LPCWSTR source,
                               LPCWSTR target, const INT type, LPCWSTR action);
static UINT HANDLE_CustomType18(MSIPACKAGE *package, LPCWSTR source,
                                LPCWSTR target, const INT type, LPCWSTR action);
static UINT HANDLE_CustomType19(MSIPACKAGE *package, LPCWSTR source,
                                LPCWSTR target, const INT type, LPCWSTR action);
static UINT HANDLE_CustomType50(MSIPACKAGE *package, LPCWSTR source,
                                LPCWSTR target, const INT type, LPCWSTR action);
static UINT HANDLE_CustomType34(MSIPACKAGE *package, LPCWSTR source,
                                LPCWSTR target, const INT type, LPCWSTR action);


static BOOL check_execution_scheduling_options(MSIPACKAGE *package, LPCWSTR action, UINT options)
{
    if (!package->script)
        return TRUE;

    if ((options & msidbCustomActionTypeClientRepeat) == 
            msidbCustomActionTypeClientRepeat)
    {
        if (!(package->script->InWhatSequence & SEQUENCE_UI &&
            package->script->InWhatSequence & SEQUENCE_EXEC))
        {
            TRACE("Skipping action due to dbCustomActionTypeClientRepeat option.\n");
            return FALSE;
        }
    }
    else if (options & msidbCustomActionTypeFirstSequence)
    {
        if (package->script->InWhatSequence & SEQUENCE_UI &&
            package->script->InWhatSequence & SEQUENCE_EXEC )
        {
            TRACE("Skipping action due to msidbCustomActionTypeFirstSequence option.\n");
            return FALSE;
        }
    }
    else if (options & msidbCustomActionTypeOncePerProcess)
    {
        if (check_unique_action(package,action))
        {
            TRACE("Skipping action due to msidbCustomActionTypeOncePerProcess option.\n");
            return FALSE;
        }
        else
            register_unique_action(package,action);
    }

    return TRUE;
}

UINT ACTION_CustomAction(MSIPACKAGE *package,LPCWSTR action, BOOL execute)
{
    UINT rc = ERROR_SUCCESS;
    MSIRECORD * row = 0;
    static const WCHAR ExecSeqQuery[] =
    {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ',
     '`','C','u','s','t','o' ,'m','A','c','t','i','o','n','`',
     ' ','W','H','E','R','E',' ','`','A','c','t','i' ,'o','n','`',' ',
     '=',' ','\'','%','s','\'',0};
    UINT type;
    LPWSTR source;
    LPWSTR target;
    WCHAR *deformated=NULL;

    row = MSI_QueryGetRecord( package->db, ExecSeqQuery, action );
    if (!row)
        return ERROR_CALL_NOT_IMPLEMENTED;

    type = MSI_RecordGetInteger(row,2);

    source = load_dynamic_stringW(row,3);
    target = load_dynamic_stringW(row,4);

    TRACE("Handling custom action %s (%x %s %s)\n",debugstr_w(action),type,
          debugstr_w(source), debugstr_w(target));

    /* handle some of the deferred actions */
    if (type & msidbCustomActionTypeTSAware)
        FIXME("msidbCustomActionTypeTSAware not handled\n");

    if (type & msidbCustomActionTypeInScript)
    {
        if (type & msidbCustomActionTypeNoImpersonate)
            FIXME("msidbCustomActionTypeNoImpersonate not handled\n");

        if (type & msidbCustomActionTypeRollback)
        {
            FIXME("Rollback only action... rollbacks not supported yet\n");
            schedule_action(package, ROLLBACK_SCRIPT, action);
            HeapFree(GetProcessHeap(),0,source);
            HeapFree(GetProcessHeap(),0,target);
            msiobj_release(&row->hdr);
            return ERROR_SUCCESS;
        }
        if (!execute)
        {
            if (type & msidbCustomActionTypeCommit)
            {
                TRACE("Deferring Commit Action!\n");
                schedule_action(package, COMMIT_SCRIPT, action);
            }
            else
            {
                TRACE("Deferring Action!\n");
                schedule_action(package, INSTALL_SCRIPT, action);
            }

            HeapFree(GetProcessHeap(),0,source);
            HeapFree(GetProcessHeap(),0,target);
            msiobj_release(&row->hdr);
            return ERROR_SUCCESS;
        }
        else
        {
            /*Set ActionData*/

            static const WCHAR szActionData[] = {
            'C','u','s','t','o','m','A','c','t','i','o','n','D','a','t','a',0};
            static const WCHAR szBlank[] = {0};
            LPWSTR actiondata = load_dynamic_property(package,action,NULL);
            if (actiondata)
                MSI_SetPropertyW(package,szActionData,actiondata);
            else
                MSI_SetPropertyW(package,szActionData,szBlank);
        }
    }
    else if (!check_execution_scheduling_options(package,action,type))
        return ERROR_SUCCESS;

    switch (type & CUSTOM_ACTION_TYPE_MASK)
    {
        case 1: /* DLL file stored in a Binary table stream */
            rc = HANDLE_CustomType1(package,source,target,type,action);
            break;
        case 2: /* EXE file stored in a Binary table strem */
            rc = HANDLE_CustomType2(package,source,target,type,action);
            break;
        case 18: /*EXE file installed with package */
            rc = HANDLE_CustomType18(package,source,target,type,action);
            break;
        case 19: /* Error that halts install */
            rc = HANDLE_CustomType19(package,source,target,type,action);
            break;
        case 50: /*EXE file specified by a property value */
            rc = HANDLE_CustomType50(package,source,target,type,action);
            break;
        case 34: /*EXE to be run in specified directory */
            rc = HANDLE_CustomType34(package,source,target,type,action);
            break;
        case 35: /* Directory set with formatted text. */
            deformat_string(package,target,&deformated);
            MSI_SetTargetPathW(package, source, deformated);
            HeapFree(GetProcessHeap(),0,deformated);
            break;
        case 51: /* Property set with formatted text. */
            deformat_string(package,target,&deformated);
            rc = MSI_SetPropertyW(package,source,deformated);
            HeapFree(GetProcessHeap(),0,deformated);
            break;
        default:
            FIXME("UNHANDLED ACTION TYPE %i (%s %s)\n",
             type & CUSTOM_ACTION_TYPE_MASK, debugstr_w(source),
             debugstr_w(target));
    }

    HeapFree(GetProcessHeap(),0,source);
    HeapFree(GetProcessHeap(),0,target);
    msiobj_release(&row->hdr);
    return rc;
}


static UINT store_binary_to_temp(MSIPACKAGE *package, LPCWSTR source, 
                                LPWSTR tmp_file)
{
    DWORD sz=MAX_PATH;
    static const WCHAR f1[] = {'m','s','i',0};
    WCHAR fmt[MAX_PATH];

    if (MSI_GetPropertyW(package, cszTempFolder, fmt, &sz) 
        != ERROR_SUCCESS)
        GetTempPathW(MAX_PATH,fmt);

    if (GetTempFileNameW(fmt,f1,0,tmp_file) == 0)
    {
        TRACE("Unable to create file\n");
        return ERROR_FUNCTION_FAILED;
    }
    else
    {
        /* write out the file */
        UINT rc;
        MSIRECORD * row = 0;
        static const WCHAR fmt[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ',
         '`','B','i' ,'n','a','r','y','`',' ','W','H','E','R','E',
         ' ','`','N','a','m','e','`',' ','=',' ','\'','%','s','\'',0};
        HANDLE the_file;
        CHAR buffer[1024];

        the_file = CreateFileW(tmp_file, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL, NULL);
    
        if (the_file == INVALID_HANDLE_VALUE)
            return ERROR_FUNCTION_FAILED;

        row = MSI_QueryGetRecord(package->db, fmt, source);
        if (!row)
            return ERROR_FUNCTION_FAILED;

        do 
        {
            DWORD write;
            sz = 1024;
            rc = MSI_RecordReadStream(row,2,buffer,&sz);
            if (rc != ERROR_SUCCESS)
            {
                ERR("Failed to get stream\n");
                CloseHandle(the_file);  
                DeleteFileW(tmp_file);
                break;
            }
            WriteFile(the_file,buffer,sz,&write,NULL);
        } while (sz == 1024);

        CloseHandle(the_file);

        msiobj_release(&row->hdr);
    }

    return ERROR_SUCCESS;
}

static void file_running_action(MSIPACKAGE* package, HANDLE Handle, 
                                BOOL process, LPCWSTR name)
{
    MSIRUNNINGACTION *newbuf = NULL;
    INT count;
    count = package->RunningActionCount;
    package->RunningActionCount++;
    if (count != 0)
        newbuf = HeapReAlloc(GetProcessHeap(),0,
                        package->RunningAction,
                        package->RunningActionCount * sizeof(MSIRUNNINGACTION));
    else
        newbuf = HeapAlloc(GetProcessHeap(),0, sizeof(MSIRUNNINGACTION));

    newbuf[count].handle = Handle;
    newbuf[count].process = process;
    newbuf[count].name = strdupW(name);

    package->RunningAction = newbuf;
}

static UINT process_action_return_value(UINT type, HANDLE ThreadHandle)
{
    DWORD rc=0;
    
    if (type == 2)
    {
        GetExitCodeProcess(ThreadHandle,&rc);
    
        if (rc == 0)
            return ERROR_SUCCESS;
        else
            return ERROR_FUNCTION_FAILED;
    }

    GetExitCodeThread(ThreadHandle,&rc);

    switch (rc)
    {
        case ERROR_FUNCTION_NOT_CALLED:
        case ERROR_SUCCESS:
        case ERROR_INSTALL_USEREXIT:
        case ERROR_INSTALL_FAILURE:
            return rc;
        case ERROR_NO_MORE_ITEMS:
            return ERROR_SUCCESS;
        default:
            ERR("Invalid Return Code %lx\n",rc);
            return ERROR_INSTALL_FAILURE;
    }
}

static UINT process_handle(MSIPACKAGE* package, UINT type, 
                           HANDLE ThreadHandle, HANDLE ProcessHandle,
                           LPCWSTR Name, BOOL *finished)
{
    UINT rc = ERROR_SUCCESS;

    if (!(type & msidbCustomActionTypeAsync))
    {
        /* synchronous */
        TRACE("Synchronous Execution of action %s\n",debugstr_w(Name));
        if (ProcessHandle)
            msi_dialog_check_messages(ProcessHandle);
        else
            msi_dialog_check_messages(ThreadHandle);

        if (!(type & msidbCustomActionTypeContinue))
        {
            if (ProcessHandle)
                rc = process_action_return_value(2,ProcessHandle);
            else
                rc = process_action_return_value(1,ThreadHandle);
        }

        CloseHandle(ThreadHandle);
        if (ProcessHandle);
            CloseHandle(ProcessHandle);
        if (finished)
            *finished = TRUE;
    }
    else 
    {
        TRACE("Asynchronous Execution of action %s\n",debugstr_w(Name));
        /* asynchronous */
        if (type & msidbCustomActionTypeContinue)
        {
            if (ProcessHandle)
            {
                file_running_action(package, ProcessHandle, TRUE, Name);
                CloseHandle(ThreadHandle);
            }
            else
            file_running_action(package, ThreadHandle, FALSE, Name);
        }
        else
        {
            CloseHandle(ThreadHandle);
            if (ProcessHandle);
                CloseHandle(ProcessHandle);
        }
        if (finished)
            *finished = FALSE;
    }

    return rc;
}


typedef UINT __stdcall CustomEntry(MSIHANDLE);

typedef struct 
{
        MSIPACKAGE *package;
        WCHAR *target;
        WCHAR *source;
} thread_struct;

static DWORD WINAPI ACTION_CallDllFunction(thread_struct *stuff)
{
    HANDLE hModule;
    LPSTR proc;
    CustomEntry *fn;
    DWORD rc = ERROR_SUCCESS;

    TRACE("calling function (%s, %s) \n", debugstr_w(stuff->source),
          debugstr_w(stuff->target));

    hModule = LoadLibraryW(stuff->source);
    if (hModule)
    {
        proc = strdupWtoA( stuff->target );
        fn = (CustomEntry*)GetProcAddress(hModule,proc);
        if (fn)
        {
            MSIHANDLE hPackage;
            MSIPACKAGE *package = stuff->package;

            TRACE("Calling function %s\n", proc);
            hPackage = msiobj_findhandle( &package->hdr );
            if (hPackage )
            {
                rc = fn(hPackage);
                msiobj_release( &package->hdr );
            }
            else
                ERR("Handle for object %p not found\n", package );
        }
        else
            ERR("Cannot load functon\n");

        HeapFree(GetProcessHeap(),0,proc);
        FreeLibrary(hModule);
    }
    else
        ERR("Unable to load library\n");
    msiobj_release( &stuff->package->hdr );
    HeapFree(GetProcessHeap(),0,stuff->source);
    HeapFree(GetProcessHeap(),0,stuff->target);
    HeapFree(GetProcessHeap(), 0, stuff);
    return rc;
}

static DWORD WINAPI DllThread(LPVOID info)
{
    thread_struct *stuff;
    DWORD rc = 0;
  
    TRACE("MSI Thread (0x%lx) started for custom action\n",
                        GetCurrentThreadId());
    
    stuff = (thread_struct*)info;
    rc = ACTION_CallDllFunction(stuff);

    TRACE("MSI Thread (0x%lx) finished (rc %li)\n",GetCurrentThreadId(), rc);
    /* clse all handles for this thread */
    MsiCloseAllHandles();
    return rc;
}

static UINT HANDLE_CustomType1(MSIPACKAGE *package, LPCWSTR source, 
                               LPCWSTR target, const INT type, LPCWSTR action)
{
    WCHAR tmp_file[MAX_PATH];
    thread_struct *info;
    DWORD ThreadId;
    HANDLE ThreadHandle;
    UINT rc = ERROR_SUCCESS;
    BOOL finished = FALSE;

    store_binary_to_temp(package, source, tmp_file);

    TRACE("Calling function %s from %s\n",debugstr_w(target),
          debugstr_w(tmp_file));

    if (!strchrW(tmp_file,'.'))
    {
        static const WCHAR dot[]={'.',0};
        strcatW(tmp_file,dot);
    } 

    info = HeapAlloc( GetProcessHeap(), 0, sizeof(*info) );
    msiobj_addref( &package->hdr );
    info->package = package;
    info->target = strdupW(target);
    info->source = strdupW(tmp_file);

    ThreadHandle = CreateThread(NULL,0,DllThread,(LPVOID)info,0,&ThreadId);

    rc = process_handle(package, type, ThreadHandle, NULL, action, &finished );

    if (!finished)
        track_tempfile(package, tmp_file, tmp_file);
    else
        DeleteFileW(tmp_file);
 
    return rc;
}

static UINT HANDLE_CustomType2(MSIPACKAGE *package, LPCWSTR source, 
                               LPCWSTR target, const INT type, LPCWSTR action)
{
    WCHAR tmp_file[MAX_PATH];
    STARTUPINFOW si;
    PROCESS_INFORMATION info;
    BOOL rc;
    INT len;
    WCHAR *deformated;
    WCHAR *cmd;
    static const WCHAR spc[] = {' ',0};
    UINT prc = ERROR_SUCCESS;
    BOOL finished = FALSE;

    memset(&si,0,sizeof(STARTUPINFOW));

    store_binary_to_temp(package, source, tmp_file);

    deformat_string(package,target,&deformated);

    len = strlenW(tmp_file)+2;

    if (deformated)
        len += strlenW(deformated);
   
    cmd = HeapAlloc(GetProcessHeap(),0,sizeof(WCHAR)*len);

    strcpyW(cmd,tmp_file);
    if (deformated)
    {
        strcatW(cmd,spc);
        strcatW(cmd,deformated);

        HeapFree(GetProcessHeap(),0,deformated);
    }

    TRACE("executing exe %s \n",debugstr_w(cmd));

    rc = CreateProcessW(NULL, cmd, NULL, NULL, FALSE, 0, NULL,
                  c_collen, &si, &info);

    HeapFree(GetProcessHeap(),0,cmd);

    if ( !rc )
    {
        ERR("Unable to execute command\n");
        return ERROR_SUCCESS;
    }

    prc = process_handle(package, type, info.hThread, info.hProcess, action, 
                          &finished);

    if (!finished)
        track_tempfile(package, tmp_file, tmp_file);
    else
        DeleteFileW(tmp_file);
    
    return prc;
}

static UINT HANDLE_CustomType18(MSIPACKAGE *package, LPCWSTR source,
                                LPCWSTR target, const INT type, LPCWSTR action)
{
    STARTUPINFOW si;
    PROCESS_INFORMATION info;
    BOOL rc;
    WCHAR *deformated;
    WCHAR *cmd;
    INT len;
    static const WCHAR spc[] = {' ',0};
    int index;
    UINT prc;

    memset(&si,0,sizeof(STARTUPINFOW));

    index = get_loaded_file(package,source);

    len = strlenW(package->files[index].TargetPath);

    deformat_string(package,target,&deformated);
    if (deformated)
        len += strlenW(deformated);
    len += 2;

    cmd = HeapAlloc(GetProcessHeap(),0,len * sizeof(WCHAR));

    strcpyW(cmd, package->files[index].TargetPath);
    if (deformated)
    {
        strcatW(cmd, spc);
        strcatW(cmd, deformated);

        HeapFree(GetProcessHeap(),0,deformated);
    }

    TRACE("executing exe %s \n",debugstr_w(cmd));

    rc = CreateProcessW(NULL, cmd, NULL, NULL, FALSE, 0, NULL,
                  c_collen, &si, &info);

    HeapFree(GetProcessHeap(),0,cmd);
    
    if ( !rc )
    {
        ERR("Unable to execute command\n");
        return ERROR_SUCCESS;
    }

    prc = process_handle(package, type, info.hThread, info.hProcess, action, 
                         NULL);

    return prc;
}

static UINT HANDLE_CustomType19(MSIPACKAGE *package, LPCWSTR source,
                                LPCWSTR target, const INT type, LPCWSTR action)
{
    static const WCHAR query[] = {
      'S','E','L','E','C','T',' ','`','M','e','s','s','a','g','e','`',' ',
      'F','R','O','M',' ','`','E','r','r','o','r','`',' ',
      'W','H','E','R','E',' ','`','E','r','r','o','r','`',' ','=',' ',
      '\'','%','s','\'',0
    };
    MSIRECORD *row = 0;
    LPWSTR deformated = NULL;

    deformat_string( package, target, &deformated );

    /* first try treat the error as a number */
    row = MSI_QueryGetRecord( package->db, query, deformated );
    if( row )
    {
        LPCWSTR error = MSI_RecordGetString( row, 1 );
        MessageBoxW( NULL, error, NULL, MB_OK );
        msiobj_release( &row->hdr );
    }
    else
        MessageBoxW( NULL, deformated, NULL, MB_OK );

    HeapFree( GetProcessHeap(), 0, deformated );

    return ERROR_FUNCTION_FAILED;
}

static UINT HANDLE_CustomType50(MSIPACKAGE *package, LPCWSTR source,
                                LPCWSTR target, const INT type, LPCWSTR action)
{
    STARTUPINFOW si;
    PROCESS_INFORMATION info;
    WCHAR *prop;
    BOOL rc;
    WCHAR *deformated;
    WCHAR *cmd;
    INT len;
    UINT prc;
    static const WCHAR spc[] = {' ',0};

    memset(&si,0,sizeof(STARTUPINFOW));
    memset(&info,0,sizeof(PROCESS_INFORMATION));

    prop = load_dynamic_property(package,source,&prc);
    if (!prop)
        return ERROR_SUCCESS;

    deformat_string(package,target,&deformated);
    len = strlenW(prop) + 2;
    if (deformated)
         len += strlenW(deformated);

    cmd = HeapAlloc(GetProcessHeap(),0,sizeof(WCHAR)*len);

    strcpyW(cmd,prop);
    if (deformated)
    {
        strcatW(cmd,spc);
        strcatW(cmd,deformated);

        HeapFree(GetProcessHeap(),0,deformated);
    }

    TRACE("executing exe %s \n",debugstr_w(cmd));

    rc = CreateProcessW(NULL, cmd, NULL, NULL, FALSE, 0, NULL,
                  c_collen, &si, &info);

    HeapFree(GetProcessHeap(),0,cmd);
    
    if ( !rc )
    {
        ERR("Unable to execute command\n");
        return ERROR_SUCCESS;
    }

    prc = process_handle(package, type, info.hThread, info.hProcess, action, 
                         NULL);

    return prc;
}

static UINT HANDLE_CustomType34(MSIPACKAGE *package, LPCWSTR source,
                                LPCWSTR target, const INT type, LPCWSTR action)
{
    LPWSTR filename, deformated;
    STARTUPINFOW si;
    PROCESS_INFORMATION info;
    BOOL rc;
    UINT prc;

    memset(&si,0,sizeof(STARTUPINFOW));

    filename = resolve_folder(package, source, FALSE, FALSE, NULL);

    if (!filename)
        return ERROR_FUNCTION_FAILED;

    SetCurrentDirectoryW(filename);
    HeapFree(GetProcessHeap(),0,filename);

    deformat_string(package,target,&deformated);

    if (!deformated)
        return ERROR_FUNCTION_FAILED;

    TRACE("executing exe %s \n",debugstr_w(deformated));

    rc = CreateProcessW(NULL, deformated, NULL, NULL, FALSE, 0, NULL,
                  c_collen, &si, &info);
    HeapFree(GetProcessHeap(),0,deformated);

    if ( !rc )
    {
        ERR("Unable to execute command\n");
        return ERROR_SUCCESS;
    }

    prc = process_handle(package, type, info.hThread, info.hProcess, action,
                         NULL);

    return prc;
}


void ACTION_FinishCustomActions(MSIPACKAGE* package)
{
    INT i;
    DWORD rc;

    for (i = 0; i < package->RunningActionCount; i++)
    {
        TRACE("Checking on action %s\n",
               debugstr_w(package->RunningAction[i].name));

        if (package->RunningAction[i].process)
            GetExitCodeProcess(package->RunningAction[i].handle, &rc);
        else
            GetExitCodeThread(package->RunningAction[i].handle, &rc);

        if (rc == STILL_ACTIVE)
        {
            TRACE("Waiting on action %s\n",
               debugstr_w(package->RunningAction[i].name));
            msi_dialog_check_messages(package->RunningAction[i].handle);
        }

        HeapFree(GetProcessHeap(),0,package->RunningAction[i].name);
        CloseHandle(package->RunningAction[i].handle);
    }

    HeapFree(GetProcessHeap(),0,package->RunningAction);
}
