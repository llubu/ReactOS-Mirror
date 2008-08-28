/*
 * Network Connections Shell Folder
 *
 * Copyright 2008       Johannes Anderwald <janderwald@reactos.org>
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

#include <precomp.h>

WINE_DEFAULT_DEBUG_CHANNEL (shell);

/***********************************************************************
*   IShellFolder implementation
*/

typedef struct {
    const IShellFolder2Vtbl  *lpVtbl;
    LONG                       ref;
    const IContextMenu2Vtbl *lpVtblContextMenu;
    const IPersistFolder2Vtbl *lpVtblPersistFolder2;

    /* both paths are parsible from the desktop */
    LPITEMIDLIST pidlRoot;	/* absolute pidl */
    LPCITEMIDLIST apidl;    /* currently focused font item */
} IGenericSFImpl, *LPIGenericSFImpl;


static const shvheader NetConnectSFHeader[] = {
    {IDS_SHV_COLUMN_NAME, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 20},
    {IDS_SHV_COLUMN_TYPE, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 8},
    {IDS_SHV_COLUMN_STATE, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 10},
    {IDS_SHV_COLUMN_DEVNAME, SHCOLSTATE_TYPE_DATE | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 12},
    {IDS_SHV_COLUMN_PHONE, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 10},
    {IDS_SHV_COLUMN_OWNER, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 5}
};

#define NETCONNECTSHELLVIEWCOLUMNS 6

#define COLUMN_NAME     0
#define COLUMN_TYPE     1
#define COLUMN_STATUS   2
#define COLUMN_DEVNAME  3
#define COLUMN_PHONE    4
#define COLUMN_OWNER    5

static LPIGenericSFImpl __inline impl_from_IContextMenu2(IContextMenu2 *iface)
{
    return (LPIGenericSFImpl)((char *)iface - FIELD_OFFSET(IGenericSFImpl, lpVtblContextMenu));
}

static LPIGenericSFImpl __inline impl_from_IPersistFolder2(IPersistFolder2 *iface)
{
    return (LPIGenericSFImpl)((char *)iface - FIELD_OFFSET(IGenericSFImpl, lpVtblPersistFolder2));
}



/**************************************************************************
 *	ISF_NetConnect_fnQueryInterface
 *
 * NOTE
 *     supports not IPersist/IPersistFolder
 */
static HRESULT WINAPI ISF_NetConnect_fnQueryInterface (IShellFolder2 *iface, REFIID riid, LPVOID *ppvObj)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;

    *ppvObj = NULL;

    if (IsEqualIID (riid, &IID_IUnknown) ||
        IsEqualIID (riid, &IID_IShellFolder) ||
        IsEqualIID (riid, &IID_IShellFolder2))
    {
        *ppvObj = This;
    }
    else if (IsEqualIID (riid, &IID_IPersistFolder) ||
             IsEqualIID (riid, &IID_IPersistFolder2))
    {
        *ppvObj = (LPVOID *)&This->lpVtblPersistFolder2;
    }
    if (*ppvObj)
    {
        IUnknown_AddRef ((IUnknown *) (*ppvObj));
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI ISF_NetConnect_fnAddRef (IShellFolder2 * iface)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;
    ULONG refCount = InterlockedIncrement(&This->ref);

    return refCount;
}

static ULONG WINAPI ISF_NetConnect_fnRelease (IShellFolder2 * iface)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);


    if (!refCount) 
    {
        SHFree (This->pidlRoot);
        CoTaskMemFree(This);
    }
    return refCount;
}

/**************************************************************************
*	ISF_NetConnect_fnParseDisplayName
*/
static HRESULT WINAPI ISF_NetConnect_fnParseDisplayName (IShellFolder2 * iface,
               HWND hwndOwner, LPBC pbcReserved, LPOLESTR lpszDisplayName,
               DWORD * pchEaten, LPITEMIDLIST * ppidl, DWORD * pdwAttributes)
{
    //IGenericSFImpl *This = (IGenericSFImpl *)iface;

    HRESULT hr = E_UNEXPECTED;

    *ppidl = 0;
    if (pchEaten)
        *pchEaten = 0;		/* strange but like the original */

    return hr;
}

/**************************************************************************
 *  CreateNetConnectEnumListss()
 */
static BOOL CreateNetConnectEnumList(IEnumIDList *list, DWORD dwFlags)
{
    HRESULT hr;
    INetConnectionManager * INetConMan;
    IEnumNetConnection * IEnumCon;
    INetConnection * INetCon;
    ULONG Count;
    LPITEMIDLIST pidl;

    /* get an instance to of IConnectionManager */
    hr = INetConnectionManager_Constructor(NULL, &IID_INetConnectionManager, (LPVOID*)&INetConMan);
    if (FAILED(hr))
        return FALSE;

    hr = INetConnectionManager_EnumConnections(INetConMan, NCME_DEFAULT, &IEnumCon);
    if (FAILED(hr))
    {
        INetConnectionManager_Release(INetConMan);
        return FALSE;
    }

    do
    {
        hr = IEnumNetConnection_Next(IEnumCon, 1, &INetCon, &Count);
        if (hr == S_OK)
        {
            pidl = ILCreateNetConnectItem(INetCon);
            if (pidl)
            {
                AddToEnumList(list, pidl);
            }
        }
        else
        {
            break;
        }
    }while(TRUE);

    IEnumNetConnection_Release(IEnumCon);
    INetConnectionManager_Release(INetConMan);

    return TRUE;
}

/**************************************************************************
*		ISF_NetConnect_fnEnumObjects
*/
static HRESULT WINAPI ISF_NetConnect_fnEnumObjects (IShellFolder2 * iface,
               HWND hwndOwner, DWORD dwFlags, LPENUMIDLIST * ppEnumIDList)
{
    //IGenericSFImpl *This = (IGenericSFImpl *)iface;

    *ppEnumIDList = IEnumIDList_Constructor();
    if(*ppEnumIDList)
        CreateNetConnectEnumList(*ppEnumIDList, dwFlags);


    return (*ppEnumIDList) ? S_OK : E_OUTOFMEMORY;
}

/**************************************************************************
*		ISF_NetConnect_fnBindToObject
*/
static HRESULT WINAPI ISF_NetConnect_fnBindToObject (IShellFolder2 * iface,
               LPCITEMIDLIST pidl, LPBC pbcReserved, REFIID riid, LPVOID * ppvOut)
{
    //IGenericSFImpl *This = (IGenericSFImpl *)iface;

    return E_NOTIMPL;
}

/**************************************************************************
*	ISF_NetConnect_fnBindToStorage
*/
static HRESULT WINAPI ISF_NetConnect_fnBindToStorage (IShellFolder2 * iface,
               LPCITEMIDLIST pidl, LPBC pbcReserved, REFIID riid, LPVOID * ppvOut)
{
    //IGenericSFImpl *This = (IGenericSFImpl *)iface;


    *ppvOut = NULL;
    return E_NOTIMPL;
}

/**************************************************************************
* 	ISF_NetConnect_fnCompareIDs
*/

static HRESULT WINAPI ISF_NetConnect_fnCompareIDs (IShellFolder2 * iface,
               LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    //IGenericSFImpl *This = (IGenericSFImpl *)iface;



    return E_NOTIMPL;
}

/**************************************************************************
*	ISF_NetConnect_fnCreateViewObject
*/
static HRESULT WINAPI ISF_NetConnect_fnCreateViewObject (IShellFolder2 * iface,
               HWND hwndOwner, REFIID riid, LPVOID * ppvOut)
{
    //IGenericSFImpl *This = (IGenericSFImpl *)iface;
    IShellView* pShellView;
    CSFV cvf;
    HRESULT hr = E_NOINTERFACE;

    if (!ppvOut)
        return hr;

    *ppvOut = NULL;

    if (IsEqualIID (riid, &IID_IShellView))
    {
        ZeroMemory(&cvf, sizeof(cvf));
        cvf.cbSize = sizeof(cvf);
        cvf.pshf = (IShellFolder*)iface;

        hr = SHCreateShellFolderViewEx(&cvf, &pShellView);
        if (SUCCEEDED(hr))
        {
            hr = IShellView_QueryInterface (pShellView, riid, ppvOut);
            IShellView_Release (pShellView);
        }
    }

    return hr;
}

/**************************************************************************
*  ISF_NetConnect_fnGetAttributesOf
*/
static HRESULT WINAPI ISF_NetConnect_fnGetAttributesOf (IShellFolder2 * iface,
               UINT cidl, LPCITEMIDLIST * apidl, DWORD * rgfInOut)
{
    //IGenericSFImpl *This = (IGenericSFImpl *)iface;
    HRESULT hr = S_OK;
    static const DWORD dwNetConnectAttributes = SFGAO_STORAGE | SFGAO_HASPROPSHEET | SFGAO_STORAGEANCESTOR | 
        SFGAO_FILESYSANCESTOR | SFGAO_FOLDER | SFGAO_FILESYSTEM | SFGAO_HASSUBFOLDER | SFGAO_CANRENAME | SFGAO_CANDELETE;


    if (!rgfInOut)
        return E_INVALIDARG;

    if (cidl && !apidl)
        return E_INVALIDARG;

    if (*rgfInOut == 0)
        *rgfInOut = ~0;

    if(cidl == 0)
        *rgfInOut = dwNetConnectAttributes;

    /* make sure SFGAO_VALIDATE is cleared, some apps depend on that */
    *rgfInOut &= ~SFGAO_VALIDATE;

    return hr;
}

/**************************************************************************
*	ISF_NetConnect_fnGetUIObjectOf
*
* PARAMETERS
*  hwndOwner [in]  Parent window for any output
*  cidl      [in]  array size
*  apidl     [in]  simple pidl array
*  riid      [in]  Requested Interface
*  prgfInOut [   ] reserved
*  ppvObject [out] Resulting Interface
*
*/
static HRESULT WINAPI ISF_NetConnect_fnGetUIObjectOf (IShellFolder2 * iface,
               HWND hwndOwner, UINT cidl, LPCITEMIDLIST * apidl, REFIID riid,
               UINT * prgfInOut, LPVOID * ppvOut)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;

    IUnknown *pObj = NULL;
    HRESULT hr = E_INVALIDARG;

    if (!ppvOut)
        return hr;

    *ppvOut = NULL;

    if (IsEqualIID (riid, &IID_IContextMenu) && (cidl >= 1))
    {
        pObj = (IUnknown*)(&This->lpVtblContextMenu);
        This->apidl = apidl[0];
        IUnknown_AddRef(pObj);
        hr = S_OK;
    }
    else
        hr = E_NOINTERFACE;

    if (SUCCEEDED(hr) && !pObj)
        hr = E_OUTOFMEMORY;

    *ppvOut = pObj;
    return hr;
}

/**************************************************************************
*	ISF_NetConnect_fnGetDisplayNameOf
*
*/
static HRESULT WINAPI ISF_NetConnect_fnGetDisplayNameOf (IShellFolder2 * iface,
               LPCITEMIDLIST pidl, DWORD dwFlags, LPSTRRET strRet)
{
    LPWSTR pszName;
    HRESULT hr = E_FAIL;
    NETCON_PROPERTIES * pProperties;
    VALUEStruct * val;
    //IGenericSFImpl *This = (IGenericSFImpl *)iface;

    if (!strRet)
        return E_INVALIDARG;

    pszName = CoTaskMemAlloc(MAX_PATH * sizeof(WCHAR));
    if (!pszName)
        return E_OUTOFMEMORY;

    if (_ILIsNetConnect (pidl))
    {
        if (LoadStringW(netshell_hInstance, IDS_NETWORKCONNECTION, pszName, MAX_PATH))
        {
            pszName[MAX_PATH-1] = L'\0';
            hr = S_OK;
        }
    }
    else
    {
        val = _ILGetValueStruct(pidl);
        if (val)
        {
            if (INetConnection_GetProperties((INetConnection*)val->pItem, &pProperties) == NOERROR)
            {
                if (pProperties->pszwName)
                {
                    wcscpy(pszName, pProperties->pszwName);
                    hr = S_OK;
                }
                //NcFreeNetconProperties(pProperties);
            }
        }

    }

    if (SUCCEEDED(hr))
    {
        strRet->uType = STRRET_WSTR;
        strRet->u.pOleStr = pszName;
    }
    else
    {
        CoTaskMemFree(pszName);
    }

    return hr;
}

/**************************************************************************
*  ISF_NetConnect_fnSetNameOf
*  Changes the name of a file object or subfolder, possibly changing its item
*  identifier in the process.
*
* PARAMETERS
*  hwndOwner [in]  Owner window for output
*  pidl      [in]  simple pidl of item to change
*  lpszName  [in]  the items new display name
*  dwFlags   [in]  SHGNO formatting flags
*  ppidlOut  [out] simple pidl returned
*/
static HRESULT WINAPI ISF_NetConnect_fnSetNameOf (IShellFolder2 * iface,
               HWND hwndOwner, LPCITEMIDLIST pidl,	/*simple pidl */
               LPCOLESTR lpName, DWORD dwFlags, LPITEMIDLIST * pPidlOut)
{
    //IGenericSFImpl *This = (IGenericSFImpl *)iface;

    return E_NOTIMPL;
}

static HRESULT WINAPI ISF_NetConnect_fnGetDefaultSearchGUID (
               IShellFolder2 * iface, GUID * pguid)
{
    //IGenericSFImpl *This = (IGenericSFImpl *)iface;

    return E_NOTIMPL;
}

static HRESULT WINAPI ISF_NetConnect_fnEnumSearches (IShellFolder2 * iface,
               IEnumExtraSearch ** ppenum)
{
    //IGenericSFImpl *This = (IGenericSFImpl *)iface;

    return E_NOTIMPL;
}

static HRESULT WINAPI ISF_NetConnect_fnGetDefaultColumn (IShellFolder2 * iface,
               DWORD dwRes, ULONG * pSort, ULONG * pDisplay)
{
    //IGenericSFImpl *This = (IGenericSFImpl *)iface;

    if (pSort)
        *pSort = 0;
    if (pDisplay)
        *pDisplay = 0;

    return S_OK;
}

static HRESULT WINAPI ISF_NetConnect_fnGetDefaultColumnState (
               IShellFolder2 * iface, UINT iColumn, DWORD * pcsFlags)
{
    //IGenericSFImpl *This = (IGenericSFImpl *)iface;

    if (!pcsFlags || iColumn >= NETCONNECTSHELLVIEWCOLUMNS)
        return E_INVALIDARG;
    *pcsFlags = NetConnectSFHeader[iColumn].pcsFlags;
    return S_OK;
}

static HRESULT WINAPI ISF_NetConnect_fnGetDetailsEx (IShellFolder2 * iface,
               LPCITEMIDLIST pidl, const SHCOLUMNID * pscid, VARIANT * pv)
{
    //IGenericSFImpl *This = (IGenericSFImpl *)iface;

    return E_NOTIMPL;
}

static HRESULT WINAPI ISF_NetConnect_fnGetDetailsOf (IShellFolder2 * iface,
               LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS * psd)
{
    //IGenericSFImpl *This = (IGenericSFImpl *)iface;
    WCHAR buffer[MAX_PATH] = {0};
    HRESULT hr = E_FAIL;
    VALUEStruct * val;
    NETCON_PROPERTIES * pProperties;

    if (iColumn >= NETCONNECTSHELLVIEWCOLUMNS)
        return E_FAIL;

    psd->fmt = NetConnectSFHeader[iColumn].fmt;
    psd->cxChar = NetConnectSFHeader[iColumn].cxChar;
    if (pidl == NULL)
    {
        psd->str.uType = STRRET_WSTR;
        if (LoadStringW(netshell_hInstance, NetConnectSFHeader[iColumn].colnameid, buffer, MAX_PATH))
            hr = SHStrDupW(buffer, &psd->str.u.pOleStr);

        return hr;
    }

    if (iColumn == COLUMN_NAME)
    {
        psd->str.uType = STRRET_WSTR;
        return IShellFolder2_GetDisplayNameOf(iface, pidl, SHGDN_NORMAL, &psd->str);
    }

    val = _ILGetValueStruct(pidl);
    if (!val)
        return E_FAIL;

    if (INetConnection_GetProperties((INetConnection*)val->pItem, &pProperties) != NOERROR)
        return E_FAIL;


    switch(iColumn)
    {
        case COLUMN_TYPE:
            if (pProperties->MediaType  == NCM_LAN || pProperties->MediaType == NCM_SHAREDACCESSHOST_RAS)
            {
                if (LoadStringW(netshell_hInstance, IDS_TYPE_ETHERNET, buffer, MAX_PATH))
                {
                    psd->str.uType = STRRET_WSTR;
                    hr = SHStrDupW(buffer, &psd->str.u.pOleStr);
                }
            }
            break;
        case COLUMN_STATUS:
            buffer[0] = L'\0';
            if (pProperties->Status == NCS_HARDWARE_DISABLED)
                LoadStringW(netshell_hInstance, IDS_STATUS_NON_OPERATIONAL, buffer, MAX_PATH);
            else if (pProperties->Status == NCS_DISCONNECTED)
                LoadStringW(netshell_hInstance, IDS_STATUS_UNREACHABLE, buffer, MAX_PATH);
            else if (pProperties->Status == NCS_MEDIA_DISCONNECTED)
                LoadStringW(netshell_hInstance, IDS_STATUS_DISCONNECTED, buffer, MAX_PATH);
            else if (pProperties->Status == NCS_CONNECTING)
                LoadStringW(netshell_hInstance, IDS_STATUS_CONNECTING, buffer, MAX_PATH);
            else if (pProperties->Status == NCS_CONNECTED)
                LoadStringW(netshell_hInstance, IDS_STATUS_CONNECTED, buffer, MAX_PATH);

            if (buffer[0])
            {
                buffer[MAX_PATH-1] = L'\0';
                psd->str.uType = STRRET_WSTR;
                hr = SHStrDupW(buffer, &psd->str.u.pOleStr);
            }
            break;
        case COLUMN_DEVNAME:
            wcscpy(buffer, pProperties->pszwDeviceName);
            buffer[MAX_PATH-1] = L'\0';
            psd->str.uType = STRRET_WSTR;
            hr = SHStrDupW(buffer, &psd->str.u.pOleStr);
            break;
        case COLUMN_PHONE:
        case COLUMN_OWNER:
            psd->str.u.cStr[0] = '\0';
            psd->str.uType = STRRET_CSTR;
            break;
    }
#if 0
    NcFreeNetconProperties(pProperties);
#endif
    return hr;
}

static HRESULT WINAPI ISF_NetConnect_fnMapColumnToSCID (IShellFolder2 * iface,
               UINT column, SHCOLUMNID * pscid)
{
    //IGenericSFImpl *This = (IGenericSFImpl *)iface;

    return E_NOTIMPL;
}

static const IShellFolder2Vtbl vt_ShellFolder2 = {
    ISF_NetConnect_fnQueryInterface,
    ISF_NetConnect_fnAddRef,
    ISF_NetConnect_fnRelease,
    ISF_NetConnect_fnParseDisplayName,
    ISF_NetConnect_fnEnumObjects,
    ISF_NetConnect_fnBindToObject,
    ISF_NetConnect_fnBindToStorage,
    ISF_NetConnect_fnCompareIDs,
    ISF_NetConnect_fnCreateViewObject,
    ISF_NetConnect_fnGetAttributesOf,
    ISF_NetConnect_fnGetUIObjectOf,
    ISF_NetConnect_fnGetDisplayNameOf,
    ISF_NetConnect_fnSetNameOf,
    /* ShellFolder2 */
    ISF_NetConnect_fnGetDefaultSearchGUID,
    ISF_NetConnect_fnEnumSearches,
    ISF_NetConnect_fnGetDefaultColumn,
    ISF_NetConnect_fnGetDefaultColumnState,
    ISF_NetConnect_fnGetDetailsEx,
    ISF_NetConnect_fnGetDetailsOf,
    ISF_NetConnect_fnMapColumnToSCID
};

/**************************************************************************
* IContextMenu2 Implementation
*/

/************************************************************************
 * ISF_NetConnect_IContextMenu_QueryInterface
 */
static HRESULT WINAPI ISF_NetConnect_IContextMenu2_QueryInterface(IContextMenu2 * iface, REFIID iid, LPVOID * ppvObject)
{
    IGenericSFImpl * This = impl_from_IContextMenu2(iface);

    return IShellFolder2_QueryInterface((IShellFolder2*)This, iid, ppvObject);
}

/************************************************************************
 * ISF_NetConnect_IContextMenu_AddRef
 */
static ULONG WINAPI ISF_NetConnect_IContextMenu2_AddRef(IContextMenu2 * iface)
{
    IGenericSFImpl * This = impl_from_IContextMenu2(iface);

    return IShellFolder2_AddRef((IShellFolder2*)This);
}

/************************************************************************
 * ISF_NetConnect_IContextMenu_Release
 */
static ULONG WINAPI ISF_NetConnect_IContextMenu2_Release(IContextMenu2  * iface)
{
    IGenericSFImpl * This = impl_from_IContextMenu2(iface);

    return IShellFolder2_Release((IShellFolder2*)This);
}

/**************************************************************************
* ISF_NetConnect_IContextMenu_QueryContextMenu()
*/
static HRESULT WINAPI ISF_NetConnect_IContextMenu2_QueryContextMenu(
	IContextMenu2 *iface,
	HMENU hMenu,
	UINT indexMenu,
	UINT idCmdFirst,
	UINT idCmdLast,
	UINT uFlags)
{
    int Count = 1;
    //IGenericSFImpl * This = impl_from_IContextMenu2(iface);

    return MAKE_HRESULT(SEVERITY_SUCCESS, 0, Count);
}


/**************************************************************************
* ISF_NetConnect_IContextMenu_InvokeCommand()
*/
static HRESULT WINAPI ISF_NetConnect_IContextMenu2_InvokeCommand(
	IContextMenu2 *iface,
	LPCMINVOKECOMMANDINFO lpcmi)
{
    //IGenericSFImpl * This = impl_from_IContextMenu2(iface);

    return S_OK;
}

/**************************************************************************
 *  ISF_NetConnect_IContextMenu_GetCommandString()
 *
 */
static HRESULT WINAPI ISF_NetConnect_IContextMenu2_GetCommandString(
	IContextMenu2 *iface,
	UINT_PTR idCommand,
	UINT uFlags,
	UINT* lpReserved,
	LPSTR lpszName,
	UINT uMaxNameLen)
{
    //IGenericSFImpl * This = impl_from_IContextMenu2(iface);

	return E_FAIL;
}



/**************************************************************************
* ISF_NetConnect_IContextMenu_HandleMenuMsg()
*/
static HRESULT WINAPI ISF_NetConnect_IContextMenu2_HandleMenuMsg(
	IContextMenu2 *iface,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam)
{
    //IGenericSFImpl * This = impl_from_IContextMenu2(iface);


    return E_NOTIMPL;
}

static const IContextMenu2Vtbl vt_ContextMenu2 =
{
	ISF_NetConnect_IContextMenu2_QueryInterface,
	ISF_NetConnect_IContextMenu2_AddRef,
	ISF_NetConnect_IContextMenu2_Release,
	ISF_NetConnect_IContextMenu2_QueryContextMenu,
	ISF_NetConnect_IContextMenu2_InvokeCommand,
	ISF_NetConnect_IContextMenu2_GetCommandString,
	ISF_NetConnect_IContextMenu2_HandleMenuMsg
};

/************************************************************************
 *	ISF_NetConnect_PersistFolder2_QueryInterface
 */
static HRESULT WINAPI ISF_NetConnect_PersistFolder2_QueryInterface (IPersistFolder2 * iface,
               REFIID iid, LPVOID * ppvObj)
{
    IGenericSFImpl * This = impl_from_IPersistFolder2(iface);

    return IShellFolder2_QueryInterface ((IShellFolder2*)This, iid, ppvObj);
}

/************************************************************************
 *	ISF_NetConnect_PersistFolder2_AddRef
 */
static ULONG WINAPI ISF_NetConnect_PersistFolder2_AddRef (IPersistFolder2 * iface)
{
    IGenericSFImpl * This = impl_from_IPersistFolder2(iface);

    return IShellFolder2_AddRef((IShellFolder2*)This);
}

/************************************************************************
 *	ISF_NetConnect_PersistFolder2_Release
 */
static ULONG WINAPI ISF_NetConnect_PersistFolder2_Release (IPersistFolder2 * iface)
{
    IGenericSFImpl * This = impl_from_IPersistFolder2(iface);

    return IShellFolder2_Release((IShellFolder2*)This);
}

/************************************************************************
 *	ISF_NetConnect_PersistFolder2_GetClassID
 */
static HRESULT WINAPI ISF_NetConnect_PersistFolder2_GetClassID (
               IPersistFolder2 * iface, CLSID * lpClassId)
{
    //IGenericSFImpl * This = impl_from_IPersistFolder2(iface);

    if (!lpClassId)
        return E_POINTER;

    *lpClassId = CLSID_NetworkConnections;

    return S_OK;
}

/************************************************************************
 *	ISF_NetConnect_PersistFolder2_Initialize
 *
 * NOTES: it makes no sense to change the pidl
 */
static HRESULT WINAPI ISF_NetConnect_PersistFolder2_Initialize (
               IPersistFolder2 * iface, LPCITEMIDLIST pidl)
{
    IGenericSFImpl * This = impl_from_IPersistFolder2(iface);

    SHFree(This->pidlRoot);
    This->pidlRoot = ILClone(pidl);

    return S_OK;
}

/**************************************************************************
 *	ISF_NetConnect_PersistFolder2_GetCurFolder
 */
static HRESULT WINAPI ISF_NetConnect_PersistFolder2_GetCurFolder (
               IPersistFolder2 * iface, LPITEMIDLIST * pidl)
{
    IGenericSFImpl * This = impl_from_IPersistFolder2(iface);


    if (!pidl)
        return E_POINTER;

    *pidl = ILClone (This->pidlRoot);

    return S_OK;
}

static const IPersistFolder2Vtbl vt_PersistFolder2 =
{
    ISF_NetConnect_PersistFolder2_QueryInterface,
    ISF_NetConnect_PersistFolder2_AddRef,
    ISF_NetConnect_PersistFolder2_Release,
    ISF_NetConnect_PersistFolder2_GetClassID,
    ISF_NetConnect_PersistFolder2_Initialize,
    ISF_NetConnect_PersistFolder2_GetCurFolder
};

/**************************************************************************
*	ISF_NetConnect_Constructor
*/
HRESULT WINAPI ISF_NetConnect_Constructor (IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv)
{
    IGenericSFImpl *sf;

    if (!ppv)
        return E_POINTER;
    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    sf = (IGenericSFImpl *) CoTaskMemAlloc(sizeof (IGenericSFImpl));
    if (!sf)
        return E_OUTOFMEMORY;

    sf->ref = 1;
    sf->lpVtbl = &vt_ShellFolder2;
    sf->lpVtblPersistFolder2 = &vt_PersistFolder2;
    sf->lpVtblContextMenu = &vt_ContextMenu2;
    sf->pidlRoot = _ILCreateNetConnect();	/* my qualified pidl */

    if (!SUCCEEDED (IShellFolder2_QueryInterface ((IShellFolder2*)sf, riid, ppv)))
    {
        IShellFolder2_Release((IShellFolder2*)sf);
        return E_NOINTERFACE;
    }

    return S_OK;
}
