/*
 * Implementation of the Microsoft Installer (msi.dll)
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

/* Msi top level apis directly related to installs */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wine/debug.h"
#include "msi.h"
#include "msidefs.h"
#include "msipriv.h"
#include "action.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);

/***********************************************************************
 * MsiDoActionA       (MSI.@)
 */
UINT WINAPI MsiDoActionA( MSIHANDLE hInstall, LPCSTR szAction )
{
    LPWSTR szwAction;
    UINT ret;

    TRACE("%s\n", debugstr_a(szAction));

    szwAction = strdupAtoW(szAction);
    if (szAction && !szwAction)
        return ERROR_FUNCTION_FAILED; 

    ret = MsiDoActionW( hInstall, szwAction );
    msi_free( szwAction );
    return ret;
}

/***********************************************************************
 * MsiDoActionW       (MSI.@)
 */
UINT WINAPI MsiDoActionW( MSIHANDLE hInstall, LPCWSTR szAction )
{
    MSIPACKAGE *package;
    UINT ret;

    TRACE("%s\n",debugstr_w(szAction));

    if (!szAction)
        return ERROR_INVALID_PARAMETER;

    package = msihandle2msiinfo( hInstall, MSIHANDLETYPE_PACKAGE );
    if (!package)
        return ERROR_INVALID_HANDLE;
 
    ret = ACTION_PerformUIAction( package, szAction );
    msiobj_release( &package->hdr );

    return ret;
}

/***********************************************************************
 * MsiSequenceA       (MSI.@)
 */
UINT WINAPI MsiSequenceA( MSIHANDLE hInstall, LPCSTR szTable, INT iSequenceMode )
{
    LPWSTR szwTable;
    UINT ret;

    TRACE("%s\n", debugstr_a(szTable));

    szwTable = strdupAtoW(szTable);
    if (szTable && !szwTable)
        return ERROR_FUNCTION_FAILED; 

    ret = MsiSequenceW( hInstall, szwTable, iSequenceMode );
    msi_free( szwTable );
    return ret;
}

/***********************************************************************
 * MsiSequenceW       (MSI.@)
 */
UINT WINAPI MsiSequenceW( MSIHANDLE hInstall, LPCWSTR szTable, INT iSequenceMode )
{
    MSIPACKAGE *package;
    UINT ret;

    TRACE("%s\n", debugstr_w(szTable));

    package = msihandle2msiinfo( hInstall, MSIHANDLETYPE_PACKAGE );
    if (!package)
        return ERROR_INVALID_HANDLE;

    ret = MSI_Sequence( package, szTable, iSequenceMode );
    msiobj_release( &package->hdr );
 
    return ret;
}

UINT msi_strcpy_to_awstring( LPCWSTR str, awstring *awbuf, DWORD *sz )
{
    UINT len, r = ERROR_SUCCESS;

    if (awbuf->str.w && !sz )
        return ERROR_INVALID_PARAMETER;

    if (!sz)
        return r;
 
    if (awbuf->unicode)
    {
        len = lstrlenW( str );
        if (awbuf->str.w) 
            lstrcpynW( awbuf->str.w, str, *sz );
    }
    else
    {
        len = WideCharToMultiByte( CP_ACP, 0, str, -1, NULL, 0, NULL, NULL );
        if (len)
            len--;
        WideCharToMultiByte( CP_ACP, 0, str, -1, awbuf->str.a, *sz, NULL, NULL );
        if ( *sz && (len >= *sz) )
            awbuf->str.a[*sz - 1] = 0;
    }

    if (awbuf->str.w && len >= *sz)
        r = ERROR_MORE_DATA;
    *sz = len;
    return r;
}

/***********************************************************************
 * MsiGetTargetPath   (internal)
 */
UINT WINAPI MSI_GetTargetPath( MSIHANDLE hInstall, LPCWSTR szFolder,
                               awstring *szPathBuf, DWORD* pcchPathBuf )
{
    MSIPACKAGE *package;
    LPWSTR path;
    UINT r;

    if (!szFolder)
        return ERROR_INVALID_PARAMETER;

    package = msihandle2msiinfo( hInstall, MSIHANDLETYPE_PACKAGE );
    if (!package)
        return ERROR_INVALID_HANDLE;

    path = resolve_folder( package, szFolder, FALSE, FALSE, NULL );
    msiobj_release( &package->hdr );

    if (!path)
        return ERROR_DIRECTORY;

    r = msi_strcpy_to_awstring( path, szPathBuf, pcchPathBuf );
    msi_free( path );
    return r;
}

/***********************************************************************
 * MsiGetTargetPathA        (MSI.@)
 */
UINT WINAPI MsiGetTargetPathA( MSIHANDLE hInstall, LPCSTR szFolder, 
                               LPSTR szPathBuf, DWORD* pcchPathBuf )
{
    LPWSTR szwFolder;
    awstring path;
    UINT r;

    TRACE("%s %p %p\n", debugstr_a(szFolder), szPathBuf, pcchPathBuf);

    szwFolder = strdupAtoW(szFolder);
    if (szFolder && !szwFolder)
        return ERROR_FUNCTION_FAILED; 

    path.unicode = FALSE;
    path.str.a = szPathBuf;

    r = MSI_GetTargetPath( hInstall, szwFolder, &path, pcchPathBuf );

    msi_free( szwFolder );

    return r;
}

/***********************************************************************
 * MsiGetTargetPathW        (MSI.@)
 */
UINT WINAPI MsiGetTargetPathW( MSIHANDLE hInstall, LPCWSTR szFolder,
                               LPWSTR szPathBuf, DWORD* pcchPathBuf )
{
    awstring path;

    TRACE("%s %p %p\n", debugstr_w(szFolder), szPathBuf, pcchPathBuf);

    path.unicode = TRUE;
    path.str.w = szPathBuf;

    return MSI_GetTargetPath( hInstall, szFolder, &path, pcchPathBuf );
}

/***********************************************************************
 * MsiGetSourcePath   (internal)
 */
static UINT MSI_GetSourcePath( MSIHANDLE hInstall, LPCWSTR szFolder,
                               awstring *szPathBuf, DWORD* pcchPathBuf )
{
    MSIPACKAGE *package;
    LPWSTR path;
    UINT r;

    TRACE("%s %p %p\n", debugstr_w(szFolder), szPathBuf, pcchPathBuf );

    if (!szFolder)
        return ERROR_INVALID_PARAMETER;

    package = msihandle2msiinfo(hInstall, MSIHANDLETYPE_PACKAGE);
    if (!package)
        return ERROR_INVALID_HANDLE;

    if (szPathBuf->str.w && !pcchPathBuf )
    {
        msiobj_release( &package->hdr );
        return ERROR_INVALID_PARAMETER;
    }

    path = resolve_folder(package, szFolder, TRUE, FALSE, NULL);
    msiobj_release( &package->hdr );

    TRACE("path = %s\n",debugstr_w(path));
    if (!path)
        return ERROR_DIRECTORY;

    r = msi_strcpy_to_awstring( path, szPathBuf, pcchPathBuf );
    msi_free( path );
    return r;
}

/***********************************************************************
 * MsiGetSourcePathA     (MSI.@)
 */
UINT WINAPI MsiGetSourcePathA( MSIHANDLE hInstall, LPCSTR szFolder, 
                               LPSTR szPathBuf, DWORD* pcchPathBuf )
{
    LPWSTR folder;
    awstring str;
    UINT r;

    TRACE("%s %p %p\n", szFolder, debugstr_a(szPathBuf), pcchPathBuf);

    str.unicode = FALSE;
    str.str.a = szPathBuf;

    folder = strdupAtoW( szFolder );
    r = MSI_GetSourcePath( hInstall, folder, &str, pcchPathBuf );
    msi_free( folder );

    return r;
}

/***********************************************************************
 * MsiGetSourcePathW     (MSI.@)
 */
UINT WINAPI MsiGetSourcePathW( MSIHANDLE hInstall, LPCWSTR szFolder,
                               LPWSTR szPathBuf, DWORD* pcchPathBuf )
{
    awstring str;

    TRACE("%s %p %p\n", debugstr_w(szFolder), szPathBuf, pcchPathBuf );

    str.unicode = TRUE;
    str.str.w = szPathBuf;

    return MSI_GetSourcePath( hInstall, szFolder, &str, pcchPathBuf );
}

/***********************************************************************
 * MsiSetTargetPathA  (MSI.@)
 */
UINT WINAPI MsiSetTargetPathA( MSIHANDLE hInstall, LPCSTR szFolder,
                               LPCSTR szFolderPath )
{
    LPWSTR szwFolder = NULL, szwFolderPath = NULL;
    UINT rc = ERROR_OUTOFMEMORY;

    if ( !szFolder || !szFolderPath )
        return ERROR_INVALID_PARAMETER;

    szwFolder = strdupAtoW(szFolder);
    szwFolderPath = strdupAtoW(szFolderPath);
    if (!szwFolder || !szwFolderPath)
        goto end;

    rc = MsiSetTargetPathW( hInstall, szwFolder, szwFolderPath );

end:
    msi_free(szwFolder);
    msi_free(szwFolderPath);

    return rc;
}

/*
 * Ok my original interpretation of this was wrong. And it looks like msdn has
 * changed a bit also. The given folder path does not have to actually already
 * exist, it just cannot be read only and must be a legal folder path.
 */
UINT MSI_SetTargetPathW(MSIPACKAGE *package, LPCWSTR szFolder, 
                             LPCWSTR szFolderPath)
{
    DWORD attrib;
    LPWSTR path = NULL;
    LPWSTR path2 = NULL;
    MSIFOLDER *folder;
    MSIFILE *file;

    TRACE("%p %s %s\n",package, debugstr_w(szFolder),debugstr_w(szFolderPath));

    attrib = GetFileAttributesW(szFolderPath);
    /* native MSI tests writeability by making temporary files at each drive */
    if ( attrib != INVALID_FILE_ATTRIBUTES &&
          (attrib & FILE_ATTRIBUTE_OFFLINE ||
           attrib & FILE_ATTRIBUTE_READONLY))
        return ERROR_FUNCTION_FAILED;

    path = resolve_folder(package,szFolder,FALSE,FALSE,&folder);
    if (!path)
        return ERROR_DIRECTORY;

    msi_free(folder->Property);
    folder->Property = build_directory_name(2, szFolderPath, NULL);

    if (lstrcmpiW(path, folder->Property) == 0)
    {
        /*
         *  Resolved Target has not really changed, so just 
         *  set this folder and do not recalculate everything.
         */
        msi_free(folder->ResolvedTarget);
        folder->ResolvedTarget = NULL;
        path2 = resolve_folder(package,szFolder,FALSE,TRUE,NULL);
        msi_free(path2);
    }
    else
    {
        MSIFOLDER *f;

        LIST_FOR_EACH_ENTRY( f, &package->folders, MSIFOLDER, entry )
        {
            msi_free(f->ResolvedTarget);
            f->ResolvedTarget=NULL;
        }

        LIST_FOR_EACH_ENTRY( f, &package->folders, MSIFOLDER, entry )
        {
            path2 = resolve_folder(package, f->Directory, FALSE, TRUE, NULL);
            msi_free(path2);
        }

        LIST_FOR_EACH_ENTRY( file, &package->files, MSIFILE, entry )
        {
            MSICOMPONENT *comp = file->Component;
            LPWSTR p;

            if (!comp)
                continue;

            p = resolve_folder(package, comp->Directory, FALSE, FALSE, NULL);
            msi_free(file->TargetPath);

            file->TargetPath = build_directory_name(2, p, file->FileName);
            msi_free(p);
        }
    }
    msi_free(path);

    return ERROR_SUCCESS;
}

/***********************************************************************
 * MsiSetTargetPathW  (MSI.@)
 */
UINT WINAPI MsiSetTargetPathW(MSIHANDLE hInstall, LPCWSTR szFolder, 
                             LPCWSTR szFolderPath)
{
    MSIPACKAGE *package;
    UINT ret;

    TRACE("%s %s\n",debugstr_w(szFolder),debugstr_w(szFolderPath));

    if ( !szFolder || !szFolderPath )
        return ERROR_INVALID_PARAMETER;

    package = msihandle2msiinfo(hInstall, MSIHANDLETYPE_PACKAGE);
    if (!package)
        return ERROR_INVALID_HANDLE;

    ret = MSI_SetTargetPathW( package, szFolder, szFolderPath );
    msiobj_release( &package->hdr );
    return ret;
}

/***********************************************************************
 *           MsiGetMode    (MSI.@)
 *
 * Returns an internal installer state (if it is running in a mode iRunMode)
 *
 * PARAMS
 *   hInstall    [I]  Handle to the installation
 *   hRunMode    [I]  Checking run mode
 *        MSIRUNMODE_ADMIN             Administrative mode
 *        MSIRUNMODE_ADVERTISE         Advertisement mode
 *        MSIRUNMODE_MAINTENANCE       Maintenance mode
 *        MSIRUNMODE_ROLLBACKENABLED   Rollback is enabled
 *        MSIRUNMODE_LOGENABLED        Log file is writing
 *        MSIRUNMODE_OPERATIONS        Operations in progress??
 *        MSIRUNMODE_REBOOTATEND       We need to reboot after installation completed
 *        MSIRUNMODE_REBOOTNOW         We need to reboot to continue the installation
 *        MSIRUNMODE_CABINET           Files from cabinet are installed
 *        MSIRUNMODE_SOURCESHORTNAMES  Long names in source files is suppressed
 *        MSIRUNMODE_TARGETSHORTNAMES  Long names in destination files is suppressed
 *        MSIRUNMODE_RESERVED11        Reserved
 *        MSIRUNMODE_WINDOWS9X         Running under Windows95/98
 *        MSIRUNMODE_ZAWENABLED        Demand installation is supported
 *        MSIRUNMODE_RESERVED14        Reserved
 *        MSIRUNMODE_RESERVED15        Reserved
 *        MSIRUNMODE_SCHEDULED         called from install script
 *        MSIRUNMODE_ROLLBACK          called from rollback script
 *        MSIRUNMODE_COMMIT            called from commit script
 *
 * RETURNS
 *    In the state: TRUE
 *    Not in the state: FALSE
 *
 */
BOOL WINAPI MsiGetMode(MSIHANDLE hInstall, MSIRUNMODE iRunMode)
{
    BOOL r = FALSE;

    switch (iRunMode)
    {
    case MSIRUNMODE_WINDOWS9X:
        if (GetVersion() & 0x80000000)
            r = TRUE;
        break;

    case MSIRUNMODE_RESERVED11:
    case MSIRUNMODE_RESERVED14:
    case MSIRUNMODE_RESERVED15:
        break;

    case MSIRUNMODE_SCHEDULED:
    case MSIRUNMODE_ROLLBACK:
    case MSIRUNMODE_COMMIT:
        break;

    default:
        FIXME("%ld %d\n", hInstall, iRunMode);
        r = TRUE;
    }

    return r;
}

/***********************************************************************
 *           MsiSetMode    (MSI.@)
 */
BOOL WINAPI MsiSetMode(MSIHANDLE hInstall, MSIRUNMODE iRunMode, BOOL fState)
{
    switch (iRunMode)
    {
    case MSIRUNMODE_RESERVED11:
    case MSIRUNMODE_WINDOWS9X:
    case MSIRUNMODE_RESERVED14:
    case MSIRUNMODE_RESERVED15:
        return FALSE;
    default:
        FIXME("%ld %d %d\n", hInstall, iRunMode, fState);
    }
    return TRUE;
}

/***********************************************************************
 * MsiSetFeatureStateA (MSI.@)
 *
 * According to the docs, when this is called it immediately recalculates
 * all the component states as well
 */
UINT WINAPI MsiSetFeatureStateA(MSIHANDLE hInstall, LPCSTR szFeature,
                                INSTALLSTATE iState)
{
    LPWSTR szwFeature = NULL;
    UINT rc;

    szwFeature = strdupAtoW(szFeature);

    if (!szwFeature)
        return ERROR_FUNCTION_FAILED;
   
    rc = MsiSetFeatureStateW(hInstall,szwFeature, iState); 

    msi_free(szwFeature);

    return rc;
}



UINT WINAPI MSI_SetFeatureStateW(MSIPACKAGE* package, LPCWSTR szFeature,
                                INSTALLSTATE iState)
{
    UINT rc = ERROR_SUCCESS;
    MSIFEATURE *feature, *child;

    TRACE("%s %i\n", debugstr_w(szFeature), iState);

    feature = get_loaded_feature(package,szFeature);
    if (!feature)
        return ERROR_UNKNOWN_FEATURE;

    if (iState == INSTALLSTATE_ADVERTISED && 
        feature->Attributes & msidbFeatureAttributesDisallowAdvertise)
        return ERROR_FUNCTION_FAILED;

    feature->ActionRequest = iState;
    feature->Action = iState;

    ACTION_UpdateComponentStates(package,szFeature);

    /* update all the features that are children of this feature */
    LIST_FOR_EACH_ENTRY( child, &package->features, MSIFEATURE, entry )
    {
        if (lstrcmpW(szFeature, child->Feature_Parent) == 0)
            MSI_SetFeatureStateW(package, child->Feature, iState);
    }
    
    return rc;
}

/***********************************************************************
 * MsiSetFeatureStateW (MSI.@)
 */
UINT WINAPI MsiSetFeatureStateW(MSIHANDLE hInstall, LPCWSTR szFeature,
                                INSTALLSTATE iState)
{
    MSIPACKAGE* package;
    UINT rc = ERROR_SUCCESS;

    TRACE("%s %i\n",debugstr_w(szFeature), iState);

    package = msihandle2msiinfo(hInstall, MSIHANDLETYPE_PACKAGE);
    if (!package)
        return ERROR_INVALID_HANDLE;

    rc = MSI_SetFeatureStateW(package,szFeature,iState);

    msiobj_release( &package->hdr );
    return rc;
}

/***********************************************************************
* MsiGetFeatureStateA   (MSI.@)
*/
UINT WINAPI MsiGetFeatureStateA(MSIHANDLE hInstall, LPSTR szFeature,
                  INSTALLSTATE *piInstalled, INSTALLSTATE *piAction)
{
    LPWSTR szwFeature = NULL;
    UINT rc;
    
    szwFeature = strdupAtoW(szFeature);

    rc = MsiGetFeatureStateW(hInstall,szwFeature,piInstalled, piAction);

    msi_free( szwFeature);

    return rc;
}

UINT MSI_GetFeatureStateW(MSIPACKAGE *package, LPWSTR szFeature,
                  INSTALLSTATE *piInstalled, INSTALLSTATE *piAction)
{
    MSIFEATURE *feature;

    feature = get_loaded_feature(package,szFeature);
    if (!feature)
        return ERROR_UNKNOWN_FEATURE;

    if (piInstalled)
        *piInstalled = feature->Installed;

    if (piAction)
        *piAction = feature->Action;

    TRACE("returning %i %i\n", feature->Installed, feature->Action);

    return ERROR_SUCCESS;
}

/***********************************************************************
* MsiGetFeatureStateW   (MSI.@)
*/
UINT WINAPI MsiGetFeatureStateW(MSIHANDLE hInstall, LPWSTR szFeature,
                  INSTALLSTATE *piInstalled, INSTALLSTATE *piAction)
{
    MSIPACKAGE* package;
    UINT ret;

    TRACE("%ld %s %p %p\n", hInstall, debugstr_w(szFeature), piInstalled, piAction);

    package = msihandle2msiinfo(hInstall, MSIHANDLETYPE_PACKAGE);
    if (!package)
        return ERROR_INVALID_HANDLE;
    ret = MSI_GetFeatureStateW(package, szFeature, piInstalled, piAction);
    msiobj_release( &package->hdr );
    return ret;
}

/***********************************************************************
 * MsiSetComponentStateA (MSI.@)
 */
UINT WINAPI MsiSetComponentStateA(MSIHANDLE hInstall, LPCSTR szComponent,
                                  INSTALLSTATE iState)
{
    UINT rc;
    LPWSTR szwComponent = strdupAtoW(szComponent);

    rc = MsiSetComponentStateW(hInstall, szwComponent, iState);

    msi_free(szwComponent);

    return rc;
}

/***********************************************************************
 * MsiGetComponentStateA (MSI.@)
 */
UINT WINAPI MsiGetComponentStateA(MSIHANDLE hInstall, LPSTR szComponent,
                  INSTALLSTATE *piInstalled, INSTALLSTATE *piAction)
{
    LPWSTR szwComponent= NULL;
    UINT rc;
    
    szwComponent= strdupAtoW(szComponent);

    rc = MsiGetComponentStateW(hInstall,szwComponent,piInstalled, piAction);

    msi_free( szwComponent);

    return rc;
}

static UINT MSI_SetComponentStateW(MSIPACKAGE *package, LPCWSTR szComponent,
                                   INSTALLSTATE iState)
{
    MSICOMPONENT *comp;

    TRACE("%p %s %d\n", package, debugstr_w(szComponent), iState);

    comp = get_loaded_component(package, szComponent);
    if (!comp)
        return ERROR_UNKNOWN_COMPONENT;

    comp->Installed = iState;

    return ERROR_SUCCESS;
}

UINT MSI_GetComponentStateW(MSIPACKAGE *package, LPWSTR szComponent,
                  INSTALLSTATE *piInstalled, INSTALLSTATE *piAction)
{
    MSICOMPONENT *comp;

    TRACE("%p %s %p %p\n", package, debugstr_w(szComponent),
           piInstalled, piAction);

    comp = get_loaded_component(package,szComponent);
    if (!comp)
        return ERROR_UNKNOWN_COMPONENT;

    if (piInstalled)
        *piInstalled = comp->Installed;

    if (piAction)
        *piAction = comp->Action;

    TRACE("states (%i, %i)\n", comp->Installed, comp->Action );

    return ERROR_SUCCESS;
}

/***********************************************************************
 * MsiSetComponentStateW (MSI.@)
 */
UINT WINAPI MsiSetComponentStateW(MSIHANDLE hInstall, LPCWSTR szComponent,
                                  INSTALLSTATE iState)
{
    MSIPACKAGE* package;
    UINT ret;

    package = msihandle2msiinfo(hInstall, MSIHANDLETYPE_PACKAGE);
    if (!package)
        return ERROR_INVALID_HANDLE;
    ret = MSI_SetComponentStateW(package, szComponent, iState);
    msiobj_release(&package->hdr);
    return ret;
}

/***********************************************************************
 * MsiGetComponentStateW (MSI.@)
 */
UINT WINAPI MsiGetComponentStateW(MSIHANDLE hInstall, LPWSTR szComponent,
                  INSTALLSTATE *piInstalled, INSTALLSTATE *piAction)
{
    MSIPACKAGE* package;
    UINT ret;

    TRACE("%ld %s %p %p\n", hInstall, debugstr_w(szComponent),
           piInstalled, piAction);

    package = msihandle2msiinfo(hInstall, MSIHANDLETYPE_PACKAGE);
    if (!package)
        return ERROR_INVALID_HANDLE;
    ret = MSI_GetComponentStateW( package, szComponent, piInstalled, piAction);
    msiobj_release( &package->hdr );
    return ret;
}

/***********************************************************************
 * MsiGetLanguage (MSI.@)
 */
LANGID WINAPI MsiGetLanguage(MSIHANDLE hInstall)
{
    MSIPACKAGE* package;
    LANGID langid;
    static const WCHAR szProductLanguage[] =
        {'P','r','o','d','u','c','t','L','a','n','g','u','a','g','e',0};
    
    package = msihandle2msiinfo(hInstall, MSIHANDLETYPE_PACKAGE);
    if (!package)
        return ERROR_INVALID_HANDLE;

    langid = msi_get_property_int( package, szProductLanguage, 0 );
    msiobj_release( &package->hdr );
    return langid;
}

UINT MSI_SetInstallLevel( MSIPACKAGE *package, int iInstallLevel )
{
    static const WCHAR szInstallLevel[] = {
        'I','N','S','T','A','L','L','L','E','V','E','L',0 };
    static const WCHAR fmt[] = { '%','d',0 };
    WCHAR level[6];
    UINT r;

    TRACE("%p %i\n", package, iInstallLevel);

    if (iInstallLevel<1 || iInstallLevel>32767)
        return ERROR_INVALID_PARAMETER;

    sprintfW( level, fmt, iInstallLevel );
    r = MSI_SetPropertyW( package, szInstallLevel, level );
    if ( r == ERROR_SUCCESS )
    {
        r = MSI_SetFeatureStates( package );
    }

    return r;
}

/***********************************************************************
 * MsiSetInstallLevel (MSI.@)
 */
UINT WINAPI MsiSetInstallLevel(MSIHANDLE hInstall, int iInstallLevel)
{
    MSIPACKAGE* package;
    UINT r;

    TRACE("%ld %i\n", hInstall, iInstallLevel);

    package = msihandle2msiinfo( hInstall, MSIHANDLETYPE_PACKAGE );
    if ( !package )
        return ERROR_INVALID_HANDLE;

    r = MSI_SetInstallLevel( package, iInstallLevel );

    msiobj_release( &package->hdr );

    return r;
}
