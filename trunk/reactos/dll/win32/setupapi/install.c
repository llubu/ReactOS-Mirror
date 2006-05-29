/*
 * Setupapi install routines
 *
 * Copyright 2002 Alexandre Julliard for CodeWeavers
 *           2005-2006 Herv� Poussineau (hpoussin@reactos.org)
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "setupapi_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(setupapi);

/* Unicode constants */
static const WCHAR BackSlash[] = {'\\',0};
static const WCHAR InfDirectory[] = {'i','n','f','\\',0};

/* info passed to callback functions dealing with files */
struct files_callback_info
{
    HSPFILEQ queue;
    PCWSTR   src_root;
    UINT     copy_flags;
    HINF     layout;
};

/* info passed to callback functions dealing with the registry */
struct registry_callback_info
{
    HKEY default_root;
    BOOL delete;
};

/* info passed to callback functions dealing with registering dlls */
struct register_dll_info
{
    PSP_FILE_CALLBACK_W callback;
    PVOID               callback_context;
    BOOL                unregister;
};

/* info passed to callback functions dealing with Needs directives */
struct needs_callback_info
{
    UINT type;

    HWND             owner;
    UINT             flags;
    HKEY             key_root;
    LPCWSTR          src_root;
    UINT             copy_flags;
    PVOID            callback;
    PVOID            context;
    HDEVINFO         devinfo;
    PSP_DEVINFO_DATA devinfo_data;
    PVOID            reserved1;
    PVOID            reserved2;
};

typedef BOOL (*iterate_fields_func)( HINF hinf, PCWSTR field, void *arg );

/* Unicode constants */
static const WCHAR AddService[] = {'A','d','d','S','e','r','v','i','c','e',0};
static const WCHAR CopyFiles[]  = {'C','o','p','y','F','i','l','e','s',0};
static const WCHAR DelFiles[]   = {'D','e','l','F','i','l','e','s',0};
static const WCHAR RenFiles[]   = {'R','e','n','F','i','l','e','s',0};
static const WCHAR Ini2Reg[]    = {'I','n','i','2','R','e','g',0};
static const WCHAR LogConf[]    = {'L','o','g','C','o','n','f',0};
static const WCHAR AddReg[]     = {'A','d','d','R','e','g',0};
static const WCHAR DelReg[]     = {'D','e','l','R','e','g',0};
static const WCHAR BitReg[]     = {'B','i','t','R','e','g',0};
static const WCHAR UpdateInis[] = {'U','p','d','a','t','e','I','n','i','s',0};
static const WCHAR CopyINF[]    = {'C','o','p','y','I','N','F',0};
static const WCHAR UpdateIniFields[] = {'U','p','d','a','t','e','I','n','i','F','i','e','l','d','s',0};
static const WCHAR RegisterDlls[]    = {'R','e','g','i','s','t','e','r','D','l','l','s',0};
static const WCHAR UnregisterDlls[]  = {'U','n','r','e','g','i','s','t','e','r','D','l','l','s',0};
static const WCHAR ProfileItems[]    = {'P','r','o','f','i','l','e','I','t','e','m','s',0};
static const WCHAR Include[]         = {'I','n','c','l','u','d','e',0};
static const WCHAR Needs[]           = {'N','e','e','d','s',0};


/***********************************************************************
 *            get_field_string
 *
 * Retrieve the contents of a field, dynamically growing the buffer if necessary.
 */
static WCHAR *get_field_string( INFCONTEXT *context, DWORD index, WCHAR *buffer,
                                WCHAR *static_buffer, DWORD *size )
{
    DWORD required;

    if (SetupGetStringFieldW( context, index, buffer, *size, &required )) return buffer;
    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        /* now grow the buffer */
        if (buffer != static_buffer) HeapFree( GetProcessHeap(), 0, buffer );
        if (!(buffer = HeapAlloc( GetProcessHeap(), 0, required*sizeof(WCHAR) ))) return NULL;
        *size = required;
        if (SetupGetStringFieldW( context, index, buffer, *size, &required )) return buffer;
    }
    if (buffer != static_buffer) HeapFree( GetProcessHeap(), 0, buffer );
    return NULL;
}


/***********************************************************************
 *            copy_files_callback
 *
 * Called once for each CopyFiles entry in a given section.
 */
static BOOL copy_files_callback( HINF hinf, PCWSTR field, void *arg )
{
    struct files_callback_info *info = arg;

    if (field[0] == '@')  /* special case: copy single file */
        SetupQueueDefaultCopyW( info->queue, info->layout, info->src_root, NULL, &field[1], info->copy_flags );
    else
        SetupQueueCopySectionW( info->queue, info->src_root, info->layout, hinf, field, info->copy_flags );
    return TRUE;
}


/***********************************************************************
 *            delete_files_callback
 *
 * Called once for each DelFiles entry in a given section.
 */
static BOOL delete_files_callback( HINF hinf, PCWSTR field, void *arg )
{
    struct files_callback_info *info = arg;
    SetupQueueDeleteSectionW( info->queue, hinf, 0, field );
    return TRUE;
}


/***********************************************************************
 *            rename_files_callback
 *
 * Called once for each RenFiles entry in a given section.
 */
static BOOL rename_files_callback( HINF hinf, PCWSTR field, void *arg )
{
    struct files_callback_info *info = arg;
    SetupQueueRenameSectionW( info->queue, hinf, 0, field );
    return TRUE;
}


/***********************************************************************
 *            get_root_key
 *
 * Retrieve the registry root key from its name.
 */
static HKEY get_root_key( const WCHAR *name, HKEY def_root )
{
    static const WCHAR HKCR[] = {'H','K','C','R',0};
    static const WCHAR HKCU[] = {'H','K','C','U',0};
    static const WCHAR HKLM[] = {'H','K','L','M',0};
    static const WCHAR HKU[]  = {'H','K','U',0};
    static const WCHAR HKR[]  = {'H','K','R',0};

    if (!strcmpiW( name, HKCR )) return HKEY_CLASSES_ROOT;
    if (!strcmpiW( name, HKCU )) return HKEY_CURRENT_USER;
    if (!strcmpiW( name, HKLM )) return HKEY_LOCAL_MACHINE;
    if (!strcmpiW( name, HKU )) return HKEY_USERS;
    if (!strcmpiW( name, HKR )) return def_root;
    return 0;
}


/***********************************************************************
 *            append_multi_sz_value
 *
 * Append a multisz string to a multisz registry value.
 */
static void append_multi_sz_value( HKEY hkey, const WCHAR *value, const WCHAR *strings,
                                   DWORD str_size )
{
    DWORD size, type, total;
    WCHAR *buffer, *p;

    if (RegQueryValueExW( hkey, value, NULL, &type, NULL, &size )) return;
    if (type != REG_MULTI_SZ) return;

    if (!(buffer = HeapAlloc( GetProcessHeap(), 0, (size + str_size) * sizeof(WCHAR) ))) return;
    if (RegQueryValueExW( hkey, value, NULL, NULL, (BYTE *)buffer, &size )) goto done;

    /* compare each string against all the existing ones */
    total = size;
    while (*strings)
    {
        int len = strlenW(strings) + 1;

        for (p = buffer; *p; p += strlenW(p) + 1)
            if (!strcmpiW( p, strings )) break;

        if (!*p)  /* not found, need to append it */
        {
            memcpy( p, strings, len * sizeof(WCHAR) );
            p[len] = 0;
            total += len;
        }
        strings += len;
    }
    if (total != size)
    {
        TRACE( "setting value %s to %s\n", debugstr_w(value), debugstr_w(buffer) );
        RegSetValueExW( hkey, value, 0, REG_MULTI_SZ, (BYTE *)buffer, total );
    }
 done:
    HeapFree( GetProcessHeap(), 0, buffer );
}


/***********************************************************************
 *            delete_multi_sz_value
 *
 * Remove a string from a multisz registry value.
 */
static void delete_multi_sz_value( HKEY hkey, const WCHAR *value, const WCHAR *string )
{
    DWORD size, type;
    WCHAR *buffer, *src, *dst;

    if (RegQueryValueExW( hkey, value, NULL, &type, NULL, &size )) return;
    if (type != REG_MULTI_SZ) return;
    /* allocate double the size, one for value before and one for after */
    if (!(buffer = HeapAlloc( GetProcessHeap(), 0, size * 2 * sizeof(WCHAR) ))) return;
    if (RegQueryValueExW( hkey, value, NULL, NULL, (BYTE *)buffer, &size )) goto done;
    src = buffer;
    dst = buffer + size;
    while (*src)
    {
        int len = strlenW(src) + 1;
        if (strcmpiW( src, string ))
        {
            memcpy( dst, src, len * sizeof(WCHAR) );
            dst += len;
        }
        src += len;
    }
    *dst++ = 0;
    if (dst != buffer + 2*size)  /* did we remove something? */
    {
        TRACE( "setting value %s to %s\n", debugstr_w(value), debugstr_w(buffer + size) );
        RegSetValueExW( hkey, value, 0, REG_MULTI_SZ,
                        (BYTE *)(buffer + size), dst - (buffer + size) );
    }
 done:
    HeapFree( GetProcessHeap(), 0, buffer );
}


/***********************************************************************
 *            do_reg_operation
 *
 * Perform an add/delete registry operation depending on the flags.
 */
static BOOL do_reg_operation( HKEY hkey, const WCHAR *value, INFCONTEXT *context, INT flags )
{
    DWORD type, size;

    if (flags & (FLG_ADDREG_DELREG_BIT | FLG_ADDREG_DELVAL))  /* deletion */
    {
        if (*value && !(flags & FLG_DELREG_KEYONLY_COMMON))
        {
            if ((flags & FLG_DELREG_MULTI_SZ_DELSTRING) == FLG_DELREG_MULTI_SZ_DELSTRING)
            {
                WCHAR *str;

                if (!SetupGetStringFieldW( context, 5, NULL, 0, &size ) || !size) return TRUE;
                if (!(str = HeapAlloc( GetProcessHeap(), 0, size * sizeof(WCHAR) ))) return FALSE;
                SetupGetStringFieldW( context, 5, str, size, NULL );
                delete_multi_sz_value( hkey, value, str );
                HeapFree( GetProcessHeap(), 0, str );
            }
            else RegDeleteValueW( hkey, value );
        }
        else RegDeleteKeyW( hkey, NULL );
        return TRUE;
    }

    if (flags & (FLG_ADDREG_KEYONLY|FLG_ADDREG_KEYONLY_COMMON)) return TRUE;

    if (flags & (FLG_ADDREG_NOCLOBBER|FLG_ADDREG_OVERWRITEONLY))
    {
        BOOL exists = !RegQueryValueExW( hkey, value, NULL, NULL, NULL, NULL );
        if (exists && (flags & FLG_ADDREG_NOCLOBBER)) return TRUE;
        if (!exists & (flags & FLG_ADDREG_OVERWRITEONLY)) return TRUE;
    }

    switch(flags & FLG_ADDREG_TYPE_MASK)
    {
    case FLG_ADDREG_TYPE_SZ:        type = REG_SZ; break;
    case FLG_ADDREG_TYPE_MULTI_SZ:  type = REG_MULTI_SZ; break;
    case FLG_ADDREG_TYPE_EXPAND_SZ: type = REG_EXPAND_SZ; break;
    case FLG_ADDREG_TYPE_BINARY:    type = REG_BINARY; break;
    case FLG_ADDREG_TYPE_DWORD:     type = REG_DWORD; break;
    case FLG_ADDREG_TYPE_NONE:      type = REG_NONE; break;
    default:                        type = flags >> 16; break;
    }

    if (!(flags & FLG_ADDREG_BINVALUETYPE) ||
        (type == REG_DWORD && SetupGetFieldCount(context) == 5))
    {
        static const WCHAR empty;
        WCHAR *str = NULL;

        if (type == REG_MULTI_SZ)
        {
            if (!SetupGetMultiSzFieldW( context, 5, NULL, 0, &size )) size = 0;
            if (size)
            {
                if (!(str = HeapAlloc( GetProcessHeap(), 0, size * sizeof(WCHAR) ))) return FALSE;
                SetupGetMultiSzFieldW( context, 5, str, size, NULL );
            }
            if (flags & FLG_ADDREG_APPEND)
            {
                if (!str) return TRUE;
                append_multi_sz_value( hkey, value, str, size );
                HeapFree( GetProcessHeap(), 0, str );
                return TRUE;
            }
            /* else fall through to normal string handling */
        }
        else
        {
            if (!SetupGetStringFieldW( context, 5, NULL, 0, &size )) size = 0;
            if (size)
            {
                if (!(str = HeapAlloc( GetProcessHeap(), 0, size * sizeof(WCHAR) ))) return FALSE;
                SetupGetStringFieldW( context, 5, str, size, NULL );
            }
        }

        if (type == REG_DWORD)
        {
            DWORD dw = str ? strtoulW( str, NULL, 0 ) : 0;
            TRACE( "setting dword %s to %lx\n", debugstr_w(value), dw );
            RegSetValueExW( hkey, value, 0, type, (BYTE *)&dw, sizeof(dw) );
        }
        else
        {
            TRACE( "setting value %s to %s\n", debugstr_w(value), debugstr_w(str) );
            if (str) RegSetValueExW( hkey, value, 0, type, (BYTE *)str, size * sizeof(WCHAR) );
            else RegSetValueExW( hkey, value, 0, type, (const BYTE *)&empty, sizeof(WCHAR) );
        }
        HeapFree( GetProcessHeap(), 0, str );
        return TRUE;
    }
    else  /* get the binary data */
    {
        BYTE *data = NULL;

        if (!SetupGetBinaryField( context, 5, NULL, 0, &size )) size = 0;
        if (size)
        {
            if (!(data = HeapAlloc( GetProcessHeap(), 0, size ))) return FALSE;
            TRACE( "setting binary data %s len %ld\n", debugstr_w(value), size );
            SetupGetBinaryField( context, 5, data, size, NULL );
        }
        RegSetValueExW( hkey, value, 0, type, data, size );
        HeapFree( GetProcessHeap(), 0, data );
        return TRUE;
    }
}


/***********************************************************************
 *            registry_callback
 *
 * Called once for each AddReg and DelReg entry in a given section.
 */
static BOOL registry_callback( HINF hinf, PCWSTR field, void *arg )
{
    struct registry_callback_info *info = arg;
    INFCONTEXT context;
    HKEY root_key, hkey;

    BOOL ok = SetupFindFirstLineW( hinf, field, NULL, &context );

    for (; ok; ok = SetupFindNextLine( &context, &context ))
    {
        WCHAR buffer[MAX_INF_STRING_LENGTH];
        INT flags;

        /* get root */
        if (!SetupGetStringFieldW( &context, 1, buffer, sizeof(buffer)/sizeof(WCHAR), NULL ))
            continue;
        if (!(root_key = get_root_key( buffer, info->default_root )))
            continue;

        /* get key */
        if (!SetupGetStringFieldW( &context, 2, buffer, sizeof(buffer)/sizeof(WCHAR), NULL ))
            *buffer = 0;

        /* get flags */
        if (!SetupGetIntField( &context, 4, &flags )) flags = 0;

        if (!info->delete)
        {
            if (flags & FLG_ADDREG_DELREG_BIT) continue;  /* ignore this entry */
        }
        else
        {
            if (!flags) flags = FLG_ADDREG_DELREG_BIT;
            else if (!(flags & FLG_ADDREG_DELREG_BIT)) continue;  /* ignore this entry */
        }

        if (info->delete || (flags & FLG_ADDREG_OVERWRITEONLY))
        {
            if (RegOpenKeyW( root_key, buffer, &hkey )) continue;  /* ignore if it doesn't exist */
        }
        else if (RegCreateKeyW( root_key, buffer, &hkey ))
        {
            ERR( "could not create key %p %s\n", root_key, debugstr_w(buffer) );
            continue;
        }
        TRACE( "key %p %s\n", root_key, debugstr_w(buffer) );

        /* get value name */
        if (!SetupGetStringFieldW( &context, 3, buffer, sizeof(buffer)/sizeof(WCHAR), NULL ))
            *buffer = 0;

        /* and now do it */
        if (!do_reg_operation( hkey, buffer, &context, flags ))
        {
            if (hkey != root_key) RegCloseKey( hkey );
            return FALSE;
        }
        if (hkey != root_key) RegCloseKey( hkey );
    }
    return TRUE;
}


/***********************************************************************
 *            do_register_dll
 *
 * Register or unregister a dll.
 */
static BOOL do_register_dll( const struct register_dll_info *info, const WCHAR *path,
                             INT flags, INT timeout, const WCHAR *args )
{
    HMODULE module;
    HRESULT res;
    SP_REGISTER_CONTROL_STATUSW status;
#ifdef __WINESRC__
    IMAGE_NT_HEADERS *nt;
#endif

    status.cbSize = sizeof(status);
    status.FileName = path;
    status.FailureCode = SPREG_SUCCESS;
    status.Win32Error = ERROR_SUCCESS;

    if (info->callback)
    {
        switch(info->callback( info->callback_context, SPFILENOTIFY_STARTREGISTRATION,
                               (UINT_PTR)&status, !info->unregister ))
        {
        case FILEOP_ABORT:
            SetLastError( ERROR_OPERATION_ABORTED );
            return FALSE;
        case FILEOP_SKIP:
            return TRUE;
        case FILEOP_DOIT:
            break;
        }
    }

    if (!(module = LoadLibraryExW( path, 0, LOAD_WITH_ALTERED_SEARCH_PATH )))
    {
        WARN( "could not load %s\n", debugstr_w(path) );
        status.FailureCode = SPREG_LOADLIBRARY;
        status.Win32Error = GetLastError();
        goto done;
    }

#ifdef __WINESRC__
    if ((nt = RtlImageNtHeader( module )) && !(nt->FileHeader.Characteristics & IMAGE_FILE_DLL))
    {
        /* file is an executable, not a dll */
        STARTUPINFOW startup;
        PROCESS_INFORMATION info;
        WCHAR *cmd_line;
        BOOL res;
        static const WCHAR format[] = {'"','%','s','"',' ','%','s',0};
        static const WCHAR default_args[] = {'/','R','e','g','S','e','r','v','e','r',0};

        FreeLibrary( module );
        module = NULL;
        if (!args) args = default_args;
        cmd_line = HeapAlloc( GetProcessHeap(), 0, (strlenW(path) + strlenW(args) + 4) * sizeof(WCHAR) );
        sprintfW( cmd_line, format, path, args );
        memset( &startup, 0, sizeof(startup) );
        startup.cb = sizeof(startup);
        TRACE( "executing %s\n", debugstr_w(cmd_line) );
        res = CreateProcessW( NULL, cmd_line, NULL, NULL, FALSE, 0, NULL, NULL, &startup, &info );
        HeapFree( GetProcessHeap(), 0, cmd_line );
        if (!res)
        {
            status.FailureCode = SPREG_LOADLIBRARY;
            status.Win32Error = GetLastError();
            goto done;
        }
        CloseHandle( info.hThread );

        if (WaitForSingleObject( info.hProcess, timeout*1000 ) == WAIT_TIMEOUT)
        {
            /* timed out, kill the process */
            TerminateProcess( info.hProcess, 1 );
            status.FailureCode = SPREG_TIMEOUT;
            status.Win32Error = ERROR_TIMEOUT;
        }
        CloseHandle( info.hProcess );
        goto done;
    }
#endif // __WINESRC__

    if (flags & FLG_REGSVR_DLLREGISTER)
    {
        const char *entry_point = info->unregister ? "DllUnregisterServer" : "DllRegisterServer";
        HRESULT (WINAPI *func)(void) = (void *)GetProcAddress( module, entry_point );

        if (!func)
        {
            status.FailureCode = SPREG_GETPROCADDR;
            status.Win32Error = GetLastError();
            goto done;
        }

        TRACE( "calling %s in %s\n", entry_point, debugstr_w(path) );
        res = func();

        if (FAILED(res))
        {
            WARN( "calling %s in %s returned error %lx\n", entry_point, debugstr_w(path), res );
            status.FailureCode = SPREG_REGSVR;
            status.Win32Error = res;
            goto done;
        }
    }

    if (flags & FLG_REGSVR_DLLINSTALL)
    {
        HRESULT (WINAPI *func)(BOOL,LPCWSTR) = (void *)GetProcAddress( module, "DllInstall" );

        if (!func)
        {
            status.FailureCode = SPREG_GETPROCADDR;
            status.Win32Error = GetLastError();
            goto done;
        }

        TRACE( "calling DllInstall(%d,%s) in %s\n",
               !info->unregister, debugstr_w(args), debugstr_w(path) );
        res = func( !info->unregister, args );

        if (FAILED(res))
        {
            WARN( "calling DllInstall in %s returned error %lx\n", debugstr_w(path), res );
            status.FailureCode = SPREG_REGSVR;
            status.Win32Error = res;
            goto done;
        }
    }

done:
    if (module) FreeLibrary( module );
    if (info->callback) info->callback( info->callback_context, SPFILENOTIFY_ENDREGISTRATION,
                                        (UINT_PTR)&status, !info->unregister );
    return TRUE;
}


/***********************************************************************
 *            register_dlls_callback
 *
 * Called once for each RegisterDlls entry in a given section.
 */
static BOOL register_dlls_callback( HINF hinf, PCWSTR field, void *arg )
{
    struct register_dll_info *info = arg;
    INFCONTEXT context;
    BOOL ret = TRUE;
    BOOL ok = SetupFindFirstLineW( hinf, field, NULL, &context );

    for (; ok; ok = SetupFindNextLine( &context, &context ))
    {
        WCHAR *path, *args, *p;
        WCHAR buffer[MAX_INF_STRING_LENGTH];
        INT flags, timeout;

        /* get directory */
        if (!(path = PARSER_get_dest_dir( &context ))) continue;

        /* get dll name */
        if (!SetupGetStringFieldW( &context, 3, buffer, sizeof(buffer)/sizeof(WCHAR), NULL ))
            goto done;
        if (!(p = HeapReAlloc( GetProcessHeap(), 0, path,
                               (strlenW(path) + strlenW(buffer) + 2) * sizeof(WCHAR) ))) goto done;
        path = p;
        p += strlenW(p);
        if (p == path || p[-1] != '\\') *p++ = '\\';
        strcpyW( p, buffer );

        /* get flags */
        if (!SetupGetIntField( &context, 4, &flags )) flags = 0;

        /* get timeout */
        if (!SetupGetIntField( &context, 5, &timeout )) timeout = 60;

        /* get command line */
        args = NULL;
        if (SetupGetStringFieldW( &context, 6, buffer, sizeof(buffer)/sizeof(WCHAR), NULL ))
            args = buffer;

        ret = do_register_dll( info, path, flags, timeout, args );

    done:
        HeapFree( GetProcessHeap(), 0, path );
        if (!ret) break;
    }
    return ret;
}

#ifdef __WINESRC__
/***********************************************************************
 *            fake_dlls_callback
 *
 * Called once for each WineFakeDlls entry in a given section.
 */
static BOOL fake_dlls_callback( HINF hinf, PCWSTR field, void *arg )
{
    INFCONTEXT context;
    BOOL ret = TRUE;
    BOOL ok = SetupFindFirstLineW( hinf, field, NULL, &context );

    for (; ok; ok = SetupFindNextLine( &context, &context ))
    {
        WCHAR *path, *p;
        WCHAR buffer[MAX_INF_STRING_LENGTH];

        /* get directory */
        if (!(path = PARSER_get_dest_dir( &context ))) continue;

        /* get dll name */
        if (!SetupGetStringFieldW( &context, 3, buffer, sizeof(buffer)/sizeof(WCHAR), NULL ))
            goto done;
        if (!(p = HeapReAlloc( GetProcessHeap(), 0, path,
                               (strlenW(path) + strlenW(buffer) + 2) * sizeof(WCHAR) ))) goto done;
        path = p;
        p += strlenW(p);
        if (p == path || p[-1] != '\\') *p++ = '\\';
        strcpyW( p, buffer );

        /* get source dll */
        if (SetupGetStringFieldW( &context, 4, buffer, sizeof(buffer)/sizeof(WCHAR), NULL ))
            p = buffer;  /* otherwise use target base name as default source */

        create_fake_dll( path, p );  /* ignore errors */

    done:
        HeapFree( GetProcessHeap(), 0, path );
        if (!ret) break;
    }
    return ret;
}
#endif // __WINESRC__

/***********************************************************************
 *            update_ini_callback
 *
 * Called once for each UpdateInis entry in a given section.
 */
static BOOL update_ini_callback( HINF hinf, PCWSTR field, void *arg )
{
    INFCONTEXT context;

    BOOL ok = SetupFindFirstLineW( hinf, field, NULL, &context );

    for (; ok; ok = SetupFindNextLine( &context, &context ))
    {
        WCHAR buffer[MAX_INF_STRING_LENGTH];
        WCHAR  filename[MAX_INF_STRING_LENGTH];
        WCHAR  section[MAX_INF_STRING_LENGTH];
        WCHAR  entry[MAX_INF_STRING_LENGTH];
        WCHAR  string[MAX_INF_STRING_LENGTH];
        LPWSTR divider;

        if (!SetupGetStringFieldW( &context, 1, filename,
                                   sizeof(filename)/sizeof(WCHAR), NULL ))
            continue;

        if (!SetupGetStringFieldW( &context, 2, section,
                                   sizeof(section)/sizeof(WCHAR), NULL ))
            continue;

        if (!SetupGetStringFieldW( &context, 4, buffer,
                                   sizeof(buffer)/sizeof(WCHAR), NULL ))
            continue;

        divider = strchrW(buffer,'=');
        if (divider)
        {
            *divider = 0;
            strcpyW(entry,buffer);
            divider++;
            strcpyW(string,divider);
        }
        else
        {
            strcpyW(entry,buffer);
            string[0]=0;
        }

        TRACE("Writing %s = %s in %s of file %s\n",debugstr_w(entry),
               debugstr_w(string),debugstr_w(section),debugstr_w(filename));
        WritePrivateProfileStringW(section,entry,string,filename);

    }
    return TRUE;
}

static BOOL update_ini_fields_callback( HINF hinf, PCWSTR field, void *arg )
{
    FIXME( "should update ini fields %s\n", debugstr_w(field) );
    return TRUE;
}

static BOOL ini2reg_callback( HINF hinf, PCWSTR field, void *arg )
{
    FIXME( "should do ini2reg %s\n", debugstr_w(field) );
    return TRUE;
}

static BOOL logconf_callback( HINF hinf, PCWSTR field, void *arg )
{
    FIXME( "should do logconf %s\n", debugstr_w(field) );
    return TRUE;
}

static BOOL bitreg_callback( HINF hinf, PCWSTR field, void *arg )
{
    FIXME( "should do bitreg %s\n", debugstr_w(field) );
    return TRUE;
}

static BOOL profile_items_callback( HINF hinf, PCWSTR field, void *arg )
{
    FIXME( "should do profile items %s\n", debugstr_w(field) );
    return TRUE;
}

static BOOL copy_inf_callback( HINF hinf, PCWSTR field, void *arg )
{
    FIXME( "should do copy inf %s\n", debugstr_w(field) );
    return TRUE;
}


/***********************************************************************
 *            iterate_section_fields
 *
 * Iterate over all fields of a certain key of a certain section
 */
static BOOL iterate_section_fields( HINF hinf, PCWSTR section, PCWSTR key,
                                    iterate_fields_func callback, void *arg )
{
    WCHAR static_buffer[200];
    WCHAR *buffer = static_buffer;
    DWORD size = sizeof(static_buffer)/sizeof(WCHAR);
    INFCONTEXT context;
    BOOL ret = FALSE;

    BOOL ok = SetupFindFirstLineW( hinf, section, key, &context );
    while (ok)
    {
        UINT i, count = SetupGetFieldCount( &context );
        for (i = 1; i <= count; i++)
        {
            if (!(buffer = get_field_string( &context, i, buffer, static_buffer, &size )))
                goto done;
            if (!callback( hinf, buffer, arg ))
            {
                WARN("callback failed for %s %s err %ld\n",
                     debugstr_w(section), debugstr_w(buffer), GetLastError() );
                goto done;
            }
        }
        ok = SetupFindNextMatchLineW( &context, key, &context );
    }
    ret = TRUE;
 done:
    if (buffer && buffer != static_buffer) HeapFree( GetProcessHeap(), 0, buffer );
    return ret;
}


/***********************************************************************
 *            SetupInstallFilesFromInfSectionA   (SETUPAPI.@)
 */
BOOL WINAPI SetupInstallFilesFromInfSectionA( HINF hinf, HINF hlayout, HSPFILEQ queue,
                                              PCSTR section, PCSTR src_root, UINT flags )
{
    UNICODE_STRING sectionW;
    BOOL ret = FALSE;

    if (!RtlCreateUnicodeStringFromAsciiz( &sectionW, section ))
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }
    if (!src_root)
        ret = SetupInstallFilesFromInfSectionW( hinf, hlayout, queue, sectionW.Buffer,
                                                NULL, flags );
    else
    {
        UNICODE_STRING srcW;
        if (RtlCreateUnicodeStringFromAsciiz( &srcW, src_root ))
        {
            ret = SetupInstallFilesFromInfSectionW( hinf, hlayout, queue, sectionW.Buffer,
                                                    srcW.Buffer, flags );
            RtlFreeUnicodeString( &srcW );
        }
        else SetLastError( ERROR_NOT_ENOUGH_MEMORY );
    }
    RtlFreeUnicodeString( &sectionW );
    return ret;
}


/***********************************************************************
 *            SetupInstallFilesFromInfSectionW   (SETUPAPI.@)
 */
BOOL WINAPI SetupInstallFilesFromInfSectionW( HINF hinf, HINF hlayout, HSPFILEQ queue,
                                              PCWSTR section, PCWSTR src_root, UINT flags )
{
    struct files_callback_info info;

    info.queue      = queue;
    info.src_root   = src_root;
    info.copy_flags = flags;
    info.layout     = hlayout;
    return iterate_section_fields( hinf, section, CopyFiles, copy_files_callback, &info );
}


/***********************************************************************
 *            SetupInstallFromInfSectionA   (SETUPAPI.@)
 */
BOOL WINAPI SetupInstallFromInfSectionA( HWND owner, HINF hinf, PCSTR section, UINT flags,
                                         HKEY key_root, PCSTR src_root, UINT copy_flags,
                                         PSP_FILE_CALLBACK_A callback, PVOID context,
                                         HDEVINFO devinfo, PSP_DEVINFO_DATA devinfo_data )
{
    UNICODE_STRING sectionW, src_rootW;
    struct callback_WtoA_context ctx;
    BOOL ret = FALSE;

    src_rootW.Buffer = NULL;
    if (src_root && !RtlCreateUnicodeStringFromAsciiz( &src_rootW, src_root ))
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }

    if (RtlCreateUnicodeStringFromAsciiz( &sectionW, section ))
    {
        ctx.orig_context = context;
        ctx.orig_handler = callback;
        ret = SetupInstallFromInfSectionW( owner, hinf, sectionW.Buffer, flags, key_root,
                                           src_rootW.Buffer, copy_flags, QUEUE_callback_WtoA,
                                           &ctx, devinfo, devinfo_data );
        RtlFreeUnicodeString( &sectionW );
    }
    else SetLastError( ERROR_NOT_ENOUGH_MEMORY );

    RtlFreeUnicodeString( &src_rootW );
    return ret;
}


/***********************************************************************
 *            include_callback
 *
 * Called once for each Include entry in a given section.
 */
static BOOL include_callback( HINF hinf, PCWSTR field, void *arg )
{
    return SetupOpenAppendInfFileW( field, hinf, NULL );
}


/***********************************************************************
 *            needs_callback
 *
 * Called once for each Needs entry in a given section.
 */
static BOOL needs_callback( HINF hinf, PCWSTR field, void *arg )
{
    struct needs_callback_info *info = arg;

    switch (info->type)
    {
        case 0:
            return SetupInstallFromInfSectionW(info->owner, *(HINF*)hinf, field, info->flags,
               info->key_root, info->src_root, info->copy_flags, info->callback,
               info->context, info->devinfo, info->devinfo_data);
        case 1:
            return SetupInstallServicesFromInfSectionExW(*(HINF*)hinf, field, info->flags,
                info->devinfo, info->devinfo_data, info->reserved1, info->reserved2);
        default:
            ERR("Unknown info type %ld\n", info->type);
            return FALSE;
    }
}


/***********************************************************************
 *            SetupInstallFromInfSectionW   (SETUPAPI.@)
 */
BOOL WINAPI SetupInstallFromInfSectionW( HWND owner, HINF hinf, PCWSTR section, UINT flags,
                                         HKEY key_root, PCWSTR src_root, UINT copy_flags,
                                         PSP_FILE_CALLBACK_W callback, PVOID context,
                                         HDEVINFO devinfo, PSP_DEVINFO_DATA devinfo_data )
{
    struct needs_callback_info needs_info;

    /* Parse 'Include' and 'Needs' directives */
    iterate_section_fields( hinf, section, Include, include_callback, NULL);
    needs_info.type = 0;
    needs_info.owner = owner;
    needs_info.flags = flags;
    needs_info.key_root = key_root;
    needs_info.src_root = src_root;
    needs_info.copy_flags = copy_flags;
    needs_info.callback = callback;
    needs_info.context = context;
    needs_info.devinfo = devinfo;
    needs_info.devinfo_data = devinfo_data;
    iterate_section_fields( hinf, section, Needs, needs_callback, &needs_info);

    if (flags & SPINST_FILES)
    {
        SP_DEVINSTALL_PARAMS_W install_params;
        struct files_callback_info info;
        HSPFILEQ queue = NULL;
        BOOL use_custom_queue;
        BOOL ret;

        install_params.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
        use_custom_queue = SetupDiGetDeviceInstallParamsW(devinfo, devinfo_data, &install_params) && (install_params.Flags & DI_NOVCP);
        if (!use_custom_queue && ((queue = SetupOpenFileQueue()) == (HSPFILEQ)INVALID_HANDLE_VALUE ))
            return FALSE;
        info.queue      = use_custom_queue ? install_params.FileQueue : queue;
        info.src_root   = src_root;
        info.copy_flags = copy_flags;
        info.layout     = hinf;
        ret = (iterate_section_fields( hinf, section, CopyFiles, copy_files_callback, &info ) &&
               iterate_section_fields( hinf, section, DelFiles, delete_files_callback, &info ) &&
               iterate_section_fields( hinf, section, RenFiles, rename_files_callback, &info ));
        if (!use_custom_queue)
        {
            if (ret)
                ret = SetupCommitFileQueueW( owner, queue, callback, context );
            SetupCloseFileQueue( queue );
        }
        if (!ret) return FALSE;
    }
    if (flags & SPINST_INIFILES)
    {
        if (!iterate_section_fields( hinf, section, UpdateInis, update_ini_callback, NULL ) ||
            !iterate_section_fields( hinf, section, UpdateIniFields,
                                     update_ini_fields_callback, NULL ))
            return FALSE;
    }
    if (flags & SPINST_INI2REG)
    {
        if (!iterate_section_fields( hinf, section, Ini2Reg, ini2reg_callback, NULL ))
            return FALSE;
    }
    if (flags & SPINST_LOGCONFIG)
    {
        if (!iterate_section_fields( hinf, section, LogConf, logconf_callback, NULL ))
            return FALSE;
    }
    if (flags & SPINST_REGSVR)
    {
        struct register_dll_info info;

        info.unregister = FALSE;
        if (flags & SPINST_REGISTERCALLBACKAWARE)
        {
            info.callback         = callback;
            info.callback_context = context;
        }
        else info.callback = NULL;

        if (!iterate_section_fields( hinf, section, RegisterDlls, register_dlls_callback, &info ))
            return FALSE;

#ifdef __WINESRC__
        if (!iterate_section_fields( hinf, section, WineFakeDlls, fake_dlls_callback, NULL ))
            return FALSE;
#endif // __WINESRC__
    }
    if (flags & SPINST_UNREGSVR)
    {
        struct register_dll_info info;

        info.unregister = TRUE;
        if (flags & SPINST_REGISTERCALLBACKAWARE)
        {
            info.callback         = callback;
            info.callback_context = context;
        }
        else info.callback = NULL;

        if (!iterate_section_fields( hinf, section, UnregisterDlls, register_dlls_callback, &info ))
            return FALSE;
    }
    if (flags & SPINST_REGISTRY)
    {
        struct registry_callback_info info;

        info.default_root = key_root;
        info.delete = TRUE;
        if (!iterate_section_fields( hinf, section, DelReg, registry_callback, &info ))
            return FALSE;
        info.delete = FALSE;
        if (!iterate_section_fields( hinf, section, AddReg, registry_callback, &info ))
            return FALSE;
    }
    if (flags & SPINST_BITREG)
    {
        if (!iterate_section_fields( hinf, section, BitReg, bitreg_callback, NULL ))
            return FALSE;
    }
    if (flags & SPINST_PROFILEITEMS)
    {
        if (!iterate_section_fields( hinf, section, ProfileItems, profile_items_callback, NULL ))
            return FALSE;
    }
    if (flags & SPINST_COPYINF)
    {
        if (!iterate_section_fields( hinf, section, CopyINF, copy_inf_callback, NULL ))
            return FALSE;
    }

    return TRUE;
}


/***********************************************************************
 *		InstallHinfSectionW  (SETUPAPI.@)
 *
 * NOTE: 'cmdline' is <section> <mode> <path> from
 *   RUNDLL32.EXE SETUPAPI.DLL,InstallHinfSection <section> <mode> <path>
 */
void WINAPI InstallHinfSectionW( HWND hwnd, HINSTANCE handle, LPCWSTR cmdline, INT show )
{
    WCHAR *p, *path, section[MAX_PATH];
    void *callback_context;
    UINT mode;
    HINF hinf;

    TRACE("hwnd %p, handle %p, cmdline %s\n", hwnd, handle, debugstr_w(cmdline));

    lstrcpynW( section, cmdline, sizeof(section)/sizeof(WCHAR) );

    if (!(p = strchrW( section, ' ' ))) return;
    *p++ = 0;
    while (*p == ' ') p++;
    mode = atoiW( p );

    if (!(p = strchrW( p, ' ' ))) return;
    path = p + 1;
    while (*path == ' ') path++;

    hinf = SetupOpenInfFileW( path, NULL, INF_STYLE_WIN4, NULL );
    if (hinf == INVALID_HANDLE_VALUE) return;

    callback_context = SetupInitDefaultQueueCallback( hwnd );
    SetupInstallFromInfSectionW( hwnd, hinf, section, SPINST_ALL, NULL, NULL, SP_COPY_NEWER,
                                 SetupDefaultQueueCallbackW, callback_context,
                                 NULL, NULL );
    SetupTermDefaultQueueCallback( callback_context );
    SetupCloseInfFile( hinf );

    /* FIXME: should check the mode and maybe reboot */
    /* there isn't much point in doing that since we */
    /* don't yet handle deferred file copies anyway. */
}


/***********************************************************************
 *		InstallHinfSectionA  (SETUPAPI.@)
 */
void WINAPI InstallHinfSectionA( HWND hwnd, HINSTANCE handle, LPCSTR cmdline, INT show )
{
    UNICODE_STRING cmdlineW;

    if (RtlCreateUnicodeStringFromAsciiz( &cmdlineW, cmdline ))
    {
        InstallHinfSectionW( hwnd, handle, cmdlineW.Buffer, show );
        RtlFreeUnicodeString( &cmdlineW );
    }
}


/***********************************************************************
 *		SetupInstallServicesFromInfSectionA  (SETUPAPI.@)
 */
BOOL WINAPI SetupInstallServicesFromInfSectionA( HINF hinf, PCSTR sectionname, DWORD flags )
{
    return SetupInstallServicesFromInfSectionExA( hinf, sectionname, flags,
                                                  NULL, NULL, NULL, NULL );
}


/***********************************************************************
 *		SetupInstallServicesFromInfSectionW  (SETUPAPI.@)
 */
BOOL WINAPI SetupInstallServicesFromInfSectionW( HINF hinf, PCWSTR sectionname, DWORD flags )
{
    return SetupInstallServicesFromInfSectionExW( hinf, sectionname, flags,
                                                  NULL, NULL, NULL, NULL );
}


/***********************************************************************
 *		SetupInstallServicesFromInfSectionExA  (SETUPAPI.@)
 */
BOOL WINAPI SetupInstallServicesFromInfSectionExA( HINF hinf, PCSTR sectionname, DWORD flags, HDEVINFO devinfo, PSP_DEVINFO_DATA devinfo_data, PVOID reserved1, PVOID reserved2 )
{
    UNICODE_STRING sectionnameW;
    BOOL ret = FALSE;

    if (RtlCreateUnicodeStringFromAsciiz( &sectionnameW, sectionname ))
    {
        ret = SetupInstallServicesFromInfSectionExW( hinf, sectionnameW.Buffer, flags, devinfo, devinfo_data, reserved1, reserved2 );
        RtlFreeUnicodeString( &sectionnameW );
    }
    else
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );

    return ret;
}


static BOOL GetLineText( HINF hinf, PCWSTR section_name, PCWSTR key_name, PWSTR *value)
{
    DWORD required;
    PWSTR buf = NULL;

    *value = NULL;

    if (! SetupGetLineTextW( NULL, hinf, section_name, key_name, NULL, 0, &required )
        && GetLastError() != ERROR_INSUFFICIENT_BUFFER )
        return FALSE;

    buf = HeapAlloc( GetProcessHeap(), 0, required * sizeof(WCHAR) );
    if ( ! buf )
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    if (! SetupGetLineTextW( NULL, hinf, section_name, key_name, buf, required, &required ) )
    {
        HeapFree( GetProcessHeap(), 0, buf );
        return FALSE;
    }

    *value = buf;
    return TRUE;
}


static BOOL GetIntField( HINF hinf, PCWSTR section_name, PCWSTR key_name, INT *value)
{
    LPWSTR buffer, end;
    INT res;

    if (! GetLineText( hinf, section_name, key_name, &buffer ) )
        return FALSE;

    res = wcstol( buffer, &end, 0 );
    if (end != buffer && !*end)
    {
        HeapFree(GetProcessHeap(), 0, buffer);
        *value = res;
        return TRUE;
    }
    else
    {
        HeapFree(GetProcessHeap(), 0, buffer);
        SetLastError( ERROR_INVALID_DATA );
        return FALSE;
    }
}


BOOL GetStringField( PINFCONTEXT context, DWORD index, PWSTR *value)
{
    DWORD RequiredSize;
    BOOL ret;

    ret = SetupGetStringFieldW(
        context,
        index,
        NULL, 0,
        &RequiredSize);
    if (!ret)
        return FALSE;
    else if (RequiredSize == 0)
    {
        *value = NULL;
        return TRUE;
    }

    /* We got the needed size for the buffer */
    *value = MyMalloc(RequiredSize * sizeof(WCHAR));
    if (!*value)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    ret = SetupGetStringFieldW(
        context,
        index,
        *value, RequiredSize, NULL);
    if (!ret)
        MyFree(*value);

    return ret;
}


static BOOL InstallOneService(
    struct DeviceInfoSet *list,
    IN HINF hInf,
    IN LPCWSTR ServiceSection,
    IN LPCWSTR ServiceName,
    IN UINT ServiceFlags)
{
    SC_HANDLE hSCManager = NULL;
    SC_HANDLE hService = NULL;
    LPDWORD GroupOrder = NULL;
    LPQUERY_SERVICE_CONFIG ServiceConfig = NULL;
    BOOL ret = FALSE;

    HKEY hGroupOrderListKey = NULL;
    LPWSTR ServiceBinary = NULL;
    LPWSTR LoadOrderGroup = NULL;
    LPWSTR DisplayName = NULL;
    LPWSTR Description = NULL;
    LPWSTR Dependencies = NULL;
    INT ServiceType, StartType, ErrorControl;
    DWORD dwRegType;
    DWORD tagId = (DWORD)-1;
    BOOL useTag;

    if (!GetIntField(hInf, ServiceSection, L"ServiceType", &ServiceType))
        goto cleanup;
    if (!GetIntField(hInf, ServiceSection, L"StartType", &StartType))
        goto cleanup;
    if (!GetIntField(hInf, ServiceSection, L"ErrorControl", &ErrorControl))
        goto cleanup;
    useTag = (ServiceType == SERVICE_BOOT_START || ServiceType == SERVICE_SYSTEM_START);

    hSCManager = OpenSCManagerW(NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_CREATE_SERVICE);
    if (hSCManager == NULL)
        goto cleanup;

    if (!GetLineText(hInf, ServiceSection, L"ServiceBinary", &ServiceBinary))
        goto cleanup;

    /* Don't check return value, as these fields are optional and
     * GetLineText initialize output parameter even on failure */
    GetLineText(hInf, ServiceSection, L"LoadOrderGroup", &LoadOrderGroup);
    GetLineText(hInf, ServiceSection, L"DisplayName", &DisplayName);
    GetLineText(hInf, ServiceSection, L"Description", &Description);
    GetLineText(hInf, ServiceSection, L"Dependencies", &Dependencies);

    hService = OpenServiceW(
        hSCManager,
        ServiceName,
        GENERIC_READ | GENERIC_WRITE);
    if (hService == NULL && GetLastError() != ERROR_SERVICE_DOES_NOT_EXIST)
        goto cleanup;

    if (hService && (ServiceFlags & SPSVCINST_DELETEEVENTLOGENTRY))
    {
        ret = DeleteService(hService);
        if (!ret && GetLastError() != ERROR_SERVICE_MARKED_FOR_DELETE)
            goto cleanup;
    }

    if (hService == NULL)
    {
        /* Create new service */
        hService = CreateServiceW(
            hSCManager,
            ServiceName,
            DisplayName,
            0,
            ServiceType,
            StartType,
            ErrorControl,
            ServiceBinary,
            LoadOrderGroup,
            useTag ? &tagId : NULL,
            Dependencies,
            NULL, NULL);
        if (hService == NULL)
            goto cleanup;
    }
    else
    {
        DWORD bufferSize;
        /* Read current configuration */
        if (!QueryServiceConfigW(hService, NULL, 0, &bufferSize))
        {
            if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
                goto cleanup;
            ServiceConfig = MyMalloc(bufferSize);
            if (!ServiceConfig)
            {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                goto cleanup;
            }
            if (!QueryServiceConfigW(hService, ServiceConfig, bufferSize, &bufferSize))
                goto cleanup;
        }
        tagId = ServiceConfig->dwTagId;

        /* Update configuration */
        ret = ChangeServiceConfigW(
            hService,
            ServiceType,
            (ServiceFlags & SPSVCINST_NOCLOBBER_STARTTYPE) ? SERVICE_NO_CHANGE : StartType,
            (ServiceFlags & SPSVCINST_NOCLOBBER_ERRORCONTROL) ? SERVICE_NO_CHANGE : ErrorControl,
            ServiceBinary,
            (ServiceFlags & SPSVCINST_NOCLOBBER_LOADORDERGROUP && ServiceConfig->lpLoadOrderGroup) ? NULL : LoadOrderGroup,
            useTag ? &tagId : NULL,
            (ServiceFlags & SPSVCINST_NOCLOBBER_DEPENDENCIES && ServiceConfig->lpDependencies) ? NULL : Dependencies,
            NULL, NULL,
            (ServiceFlags & SPSVCINST_NOCLOBBER_DISPLAYNAME && ServiceConfig->lpDisplayName) ? NULL : DisplayName);
        if (!ret)
            goto cleanup;
    }

    /* FIXME: use Description and SPSVCINST_NOCLOBBER_DESCRIPTION */

    if (useTag)
    {
        /* Add the tag to SYSTEM\CurrentControlSet\Control\GroupOrderList key */
        LONG rc;
        LPCWSTR lpLoadOrderGroup;
        DWORD bufferSize;

        lpLoadOrderGroup = LoadOrderGroup;
        if ((ServiceFlags & SPSVCINST_NOCLOBBER_LOADORDERGROUP) && ServiceConfig && ServiceConfig->lpLoadOrderGroup)
            lpLoadOrderGroup = ServiceConfig->lpLoadOrderGroup;

        rc = RegOpenKey(
            list ? list->HKLM : HKEY_LOCAL_MACHINE,
            L"SYSTEM\\CurrentControlSet\\Control\\GroupOrderList",
            &hGroupOrderListKey);
        if (rc != ERROR_SUCCESS)
        {
            SetLastError(rc);
            goto cleanup;
        }
        rc = RegQueryValueExW(hGroupOrderListKey, lpLoadOrderGroup, NULL, &dwRegType, NULL, &bufferSize);
        if (rc == ERROR_FILE_NOT_FOUND)
            bufferSize = sizeof(DWORD);
        else if (rc != ERROR_SUCCESS)
        {
            SetLastError(rc);
            goto cleanup;
        }
        else if (dwRegType != REG_BINARY || bufferSize == 0 || bufferSize % sizeof(DWORD) != 0)
        {
            SetLastError(ERROR_GEN_FAILURE);
            goto cleanup;
        }
        /* Allocate buffer to store existing data + the new tag */
        GroupOrder = MyMalloc(bufferSize + sizeof(DWORD));
        if (!GroupOrder)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto cleanup;
        }
        if (rc == ERROR_SUCCESS)
        {
            /* Read existing data */
            rc = RegQueryValueExW(
                hGroupOrderListKey,
                lpLoadOrderGroup,
                NULL,
                NULL,
                (BYTE*)GroupOrder,
                &bufferSize);
            if (rc != ERROR_SUCCESS)
            {
                SetLastError(rc);
                goto cleanup;
            }
            if (ServiceFlags & SPSVCINST_TAGTOFRONT)
                memmove(&GroupOrder[2], &GroupOrder[1], bufferSize - sizeof(DWORD));
        }
        else
        {
            GroupOrder[0] = 0;
        }
        GroupOrder[0]++;
        if (ServiceFlags & SPSVCINST_TAGTOFRONT)
            GroupOrder[1] = tagId;
        else
            GroupOrder[bufferSize / sizeof(DWORD)] = tagId;

        rc = RegSetValueExW(
            hGroupOrderListKey,
            lpLoadOrderGroup,
            0,
            REG_BINARY,
            (BYTE*)GroupOrder,
            bufferSize + sizeof(DWORD));
        if (rc != ERROR_SUCCESS)
        {
            SetLastError(rc);
            goto cleanup;
        }
    }

    ret = TRUE;

cleanup:
    if (hSCManager != NULL)
        CloseServiceHandle(hSCManager);
    if (hService != NULL)
        CloseServiceHandle(hService);
    if (hGroupOrderListKey != NULL)
        RegCloseKey(hGroupOrderListKey);
    MyFree(ServiceConfig);
    MyFree(ServiceBinary);
    MyFree(LoadOrderGroup);
    MyFree(DisplayName);
    MyFree(Description);
    MyFree(Dependencies);
    MyFree(GroupOrder);

    TRACE("Returning %d\n", ret);
    return ret;
}


/***********************************************************************
 *		SetupInstallServicesFromInfSectionExW  (SETUPAPI.@)
 */
BOOL WINAPI SetupInstallServicesFromInfSectionExW( HINF hinf, PCWSTR sectionname, DWORD flags, HDEVINFO DeviceInfoSet, PSP_DEVINFO_DATA DeviceInfoData, PVOID reserved1, PVOID reserved2 )
{
    struct DeviceInfoSet *list = NULL;
    BOOL ret = FALSE;

    TRACE("%p, %s, 0x%lx, %p, %p, %p, %p\n", hinf, debugstr_w(sectionname),
        flags, DeviceInfoSet, DeviceInfoData, reserved1, reserved2);

    if (!sectionname)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (flags & ~(SPSVCINST_TAGTOFRONT | SPSVCINST_DELETEEVENTLOGENTRY | SPSVCINST_NOCLOBBER_DISPLAYNAME | SPSVCINST_NOCLOBBER_STARTTYPE | SPSVCINST_NOCLOBBER_ERRORCONTROL | SPSVCINST_NOCLOBBER_LOADORDERGROUP | SPSVCINST_NOCLOBBER_DEPENDENCIES | SPSVCINST_STOPSERVICE))
    {
        TRACE("Unknown flags: 0x%08lx\n", flags & ~(SPSVCINST_TAGTOFRONT | SPSVCINST_DELETEEVENTLOGENTRY | SPSVCINST_NOCLOBBER_DISPLAYNAME | SPSVCINST_NOCLOBBER_STARTTYPE | SPSVCINST_NOCLOBBER_ERRORCONTROL | SPSVCINST_NOCLOBBER_LOADORDERGROUP | SPSVCINST_NOCLOBBER_DEPENDENCIES | SPSVCINST_STOPSERVICE));
        SetLastError(ERROR_INVALID_FLAGS);
    }
    else if (DeviceInfoSet == (HDEVINFO)INVALID_HANDLE_VALUE)
        SetLastError(ERROR_INVALID_HANDLE);
    else if (DeviceInfoSet && (list = (struct DeviceInfoSet *)DeviceInfoSet)->magic != SETUP_DEV_INFO_SET_MAGIC)
        SetLastError(ERROR_INVALID_HANDLE);
    else if (DeviceInfoData && DeviceInfoData->cbSize != sizeof(SP_DEVINFO_DATA))
        SetLastError(ERROR_INVALID_USER_BUFFER);
    else if (reserved1 != NULL || reserved2 != NULL)
        SetLastError(ERROR_INVALID_PARAMETER);
    else
    {
        struct needs_callback_info needs_info;
        LPWSTR ServiceName = NULL;
        LPWSTR ServiceSection = NULL;
        INT ServiceFlags;
        INFCONTEXT ContextService;
        BOOL bNeedReboot = FALSE;

        /* Parse 'Include' and 'Needs' directives */
        iterate_section_fields( hinf, sectionname, Include, include_callback, NULL);
        needs_info.type = 1;
        needs_info.flags = flags;
        needs_info.devinfo = DeviceInfoSet;
        needs_info.devinfo_data = DeviceInfoData;
        needs_info.reserved1 = reserved1;
        needs_info.reserved2 = reserved2;
        iterate_section_fields( hinf, sectionname, Needs, needs_callback, &needs_info);

        if (flags & SPSVCINST_STOPSERVICE)
        {
            FIXME("Stopping the device not implemented\n");
            /* This may lead to require a reboot */
            /* bNeedReboot = TRUE; */
#if 0
            SERVICE_STATUS ServiceStatus;
            ret = ControlService(hService, SERVICE_CONTROL_STOP, &ServiceStatus);
            if (!ret && GetLastError() != ERROR_SERVICE_NOT_ACTIVE)
                goto cleanup;
            if (ServiceStatus.dwCurrentState != SERVICE_STOP_PENDING && ServiceStatus.dwCurrentState != SERVICE_STOPPED)
            {
                SetLastError(ERROR_INSTALL_SERVICE_FAILURE);
                goto cleanup;
            }
#endif
            flags &= ~SPSVCINST_STOPSERVICE;
        }

        ret = SetupFindFirstLineW(hinf, sectionname, AddService, &ContextService);
        while (ret)
        {
            if (!GetStringField(&ContextService, 1, &ServiceName))
                goto nextservice;

            ret = SetupGetIntField(
                &ContextService,
                2, /* Field index */
                &ServiceFlags);
            if (!ret)
            {
                /* The field may be empty. Ignore the error */
                ServiceFlags = 0;
            }

            if (!GetStringField(&ContextService, 3, &ServiceSection))
                goto nextservice;

            ret = InstallOneService(list, hinf, ServiceSection, ServiceName, (ServiceFlags & ~SPSVCINST_ASSOCSERVICE) | flags);
            if (!ret)
                goto nextservice;

            if (ServiceFlags & SPSVCINST_ASSOCSERVICE)
            {
                ret = SetupDiSetDeviceRegistryPropertyW(DeviceInfoSet, DeviceInfoData, SPDRP_SERVICE, (LPBYTE)ServiceName, (strlenW(ServiceName) + 1) * sizeof(WCHAR));
                if (!ret)
                    goto nextservice;
            }

nextservice:
            HeapFree(GetProcessHeap(), 0, ServiceName);
            HeapFree(GetProcessHeap(), 0, ServiceSection);
            ServiceName = ServiceSection = NULL;
            ret = SetupFindNextMatchLineW(&ContextService, AddService, &ContextService);
        }

        if (bNeedReboot)
            SetLastError(ERROR_SUCCESS_REBOOT_REQUIRED);
        else
            SetLastError(ERROR_SUCCESS);
        ret = TRUE;
    }

    TRACE("Returning %d\n", ret);
    return ret;
}


/***********************************************************************
 *		SetupCopyOEMInfA  (SETUPAPI.@)
 */
BOOL WINAPI SetupCopyOEMInfA(
        IN PCSTR SourceInfFileName,
        IN PCSTR OEMSourceMediaLocation,
        IN DWORD OEMSourceMediaType,
        IN DWORD CopyStyle,
        OUT PSTR DestinationInfFileName OPTIONAL,
        IN DWORD DestinationInfFileNameSize,
        OUT PDWORD RequiredSize OPTIONAL,
        OUT PSTR* DestinationInfFileNameComponent OPTIONAL)
{
    PWSTR SourceInfFileNameW = NULL;
    PWSTR OEMSourceMediaLocationW = NULL;
    PWSTR DestinationInfFileNameW = NULL;
    PWSTR DestinationInfFileNameComponentW = NULL;
    BOOL ret = FALSE;

    TRACE("%s %s 0x%lx 0x%lx %p 0%lu %p %p\n",
        SourceInfFileName, OEMSourceMediaLocation, OEMSourceMediaType,
        CopyStyle, DestinationInfFileName, DestinationInfFileNameSize,
        RequiredSize, DestinationInfFileNameComponent);

    if (!DestinationInfFileName && DestinationInfFileNameSize > 0)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (!(SourceInfFileNameW = MultiByteToUnicode(SourceInfFileName, CP_ACP)))
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (!(OEMSourceMediaLocationW = MultiByteToUnicode(OEMSourceMediaLocation, CP_ACP)))
        SetLastError(ERROR_INVALID_PARAMETER);
    else
    {
        if (DestinationInfFileNameSize != 0)
        {
            DestinationInfFileNameW = MyMalloc(DestinationInfFileNameSize * sizeof(WCHAR));
            if (!DestinationInfFileNameW)
            {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                goto cleanup;
            }
        }

        ret = SetupCopyOEMInfW(
            SourceInfFileNameW,
            OEMSourceMediaLocationW,
            OEMSourceMediaType,
            CopyStyle,
            DestinationInfFileNameW,
            DestinationInfFileNameSize,
            RequiredSize,
            DestinationInfFileNameComponent ? &DestinationInfFileNameComponentW : NULL);
        if (!ret)
            goto cleanup;

        if (DestinationInfFileNameSize != 0)
        {
            if (WideCharToMultiByte(CP_ACP, 0, DestinationInfFileNameW, -1,
                DestinationInfFileName, DestinationInfFileNameSize, NULL, NULL) == 0)
            {
                DestinationInfFileName[0] = '\0';
                goto cleanup;
            }
        }
        if (DestinationInfFileNameComponent)
        {
            if (DestinationInfFileNameComponentW)
                *DestinationInfFileNameComponent = &DestinationInfFileName[DestinationInfFileNameComponentW - DestinationInfFileNameW];
            else
                *DestinationInfFileNameComponent = NULL;
        }
        ret = TRUE;
    }

cleanup:
    MyFree(SourceInfFileNameW);
    MyFree(OEMSourceMediaLocationW);
    MyFree(DestinationInfFileNameW);

    TRACE("Returning %d\n", ret);
    return ret;
}

/***********************************************************************
 *		SetupCopyOEMInfW  (SETUPAPI.@)
 */
BOOL WINAPI SetupCopyOEMInfW(
        IN PCWSTR SourceInfFileName,
        IN PCWSTR OEMSourceMediaLocation,
        IN DWORD OEMSourceMediaType,
        IN DWORD CopyStyle,
        OUT PWSTR DestinationInfFileName OPTIONAL,
        IN DWORD DestinationInfFileNameSize,
        OUT PDWORD RequiredSize OPTIONAL,
        OUT PWSTR* DestinationInfFileNameComponent OPTIONAL)
{
    BOOL ret = FALSE;

    TRACE("%s %s 0x%lx 0x%lx %p 0%lu %p %p\n",
        debugstr_w(SourceInfFileName), debugstr_w(OEMSourceMediaLocation), OEMSourceMediaType,
        CopyStyle, DestinationInfFileName, DestinationInfFileNameSize,
        RequiredSize, DestinationInfFileNameComponent);

    if (!SourceInfFileName)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (OEMSourceMediaType != SPOST_NONE && OEMSourceMediaType != SPOST_PATH && OEMSourceMediaType != SPOST_URL)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (CopyStyle & ~(SP_COPY_DELETESOURCE | SP_COPY_REPLACEONLY | SP_COPY_NOOVERWRITE | SP_COPY_OEMINF_CATALOG_ONLY))
    {
        TRACE("Unknown flags: 0x%08lx\n", CopyStyle & ~(SP_COPY_DELETESOURCE | SP_COPY_REPLACEONLY | SP_COPY_NOOVERWRITE | SP_COPY_OEMINF_CATALOG_ONLY));
        SetLastError(ERROR_INVALID_FLAGS);
    }
    else if (!DestinationInfFileName && DestinationInfFileNameSize > 0)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (CopyStyle & SP_COPY_OEMINF_CATALOG_ONLY)
    {
        FIXME("CopyStyle 0x%lx not supported\n", SP_COPY_OEMINF_CATALOG_ONLY);
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    }
    else
    {
        HANDLE hSearch = INVALID_HANDLE_VALUE;
        WIN32_FIND_DATAW FindFileData;
        BOOL AlreadyExists;
        DWORD NextFreeNumber = 0;
        SIZE_T len;
        LPWSTR pFullFileName = NULL;
        LPWSTR pFileName; /* Pointer into pFullFileName buffer */

        if (OEMSourceMediaType == SPOST_PATH || OEMSourceMediaType == SPOST_URL)
            FIXME("OEMSourceMediaType 0x%lx ignored\n", OEMSourceMediaType);

        /* Search if the specified .inf file already exists in %WINDIR%\Inf */
        AlreadyExists = FALSE; /* FIXME */

        if (!AlreadyExists && CopyStyle & SP_COPY_REPLACEONLY)
        {
            /* FIXME: set DestinationInfFileName, RequiredSize, DestinationInfFileNameComponent */
            SetLastError(ERROR_FILE_NOT_FOUND);
            goto cleanup;
        }
        else if (AlreadyExists && (CopyStyle & SP_COPY_NOOVERWRITE))
        {
            //SetLastError(ERROR_FILE_EXISTS);
            /* FIXME: set return fields */
            SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
            FIXME("File already exists. Need to return its name!\n");
            goto cleanup;
        }

        /* Search the number to give to OEM??.INF */
        len = MAX_PATH + 1 + strlenW(InfDirectory) + 13;
        pFullFileName = MyMalloc(len * sizeof(WCHAR));
        if (!pFullFileName)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto cleanup;
        }
        len = GetSystemWindowsDirectoryW(pFullFileName, MAX_PATH);
        if (len == 0 || len > MAX_PATH)
            goto cleanup;
        if (pFullFileName[strlenW(pFullFileName) - 1] != '\\')
            strcatW(pFullFileName, BackSlash);
        strcatW(pFullFileName, InfDirectory);
        pFileName = &pFullFileName[strlenW(pFullFileName)];
        sprintfW(pFileName, L"oem*.inf", NextFreeNumber);
        hSearch = FindFirstFileW(pFullFileName, &FindFileData);
        if (hSearch == INVALID_HANDLE_VALUE)
        {
            if (GetLastError() != ERROR_FILE_NOT_FOUND)
                goto cleanup;
        }
        else
        {
            do
            {
                DWORD CurrentNumber;
                if (swscanf(FindFileData.cFileName, L"oem%lu.inf", &CurrentNumber) == 1
                    && CurrentNumber <= 99999)
                {
                    NextFreeNumber = CurrentNumber + 1;
                }
            } while (FindNextFile(hSearch, &FindFileData));
        }

        if (NextFreeNumber > 99999)
        {
            ERR("Too much custom .inf files\n");
            SetLastError(ERROR_GEN_FAILURE);
            goto cleanup;
        }

        /* Create the full path: %WINDIR%\Inf\OEM{XXXXX}.inf */
        sprintfW(pFileName, L"oem%lu.inf", NextFreeNumber);
        TRACE("Next available file is %s\n", debugstr_w(pFileName));

        if (RequiredSize)
            *RequiredSize = len;
        if (DestinationInfFileName)
        {
            if (DestinationInfFileNameSize < len)
            {
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                goto cleanup;
            }
            strcpyW(DestinationInfFileName, pFullFileName);
            if (DestinationInfFileNameComponent)
                *DestinationInfFileNameComponent = &DestinationInfFileName[pFileName - pFullFileName];
        }

        if (!CopyFileW(SourceInfFileName, pFullFileName, TRUE))
        {
            TRACE("CopyFileW() failed with error 0x%lx\n", GetLastError());
            goto cleanup;
        }

        if (CopyStyle & SP_COPY_DELETESOURCE)
        {
            if (!DeleteFileW(SourceInfFileName))
            {
                TRACE("DeleteFileW() failed with error 0x%lx\n", GetLastError());
                goto cleanup;
            }
        }

        ret = TRUE;

cleanup:
        if (hSearch != INVALID_HANDLE_VALUE)
            FindClose(hSearch);
        MyFree(pFullFileName);
    }

    TRACE("Returning %d\n", ret);
    return ret;
}
