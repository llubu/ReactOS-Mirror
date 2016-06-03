/*
 *    Shell Folder stuff
 *
 *    Copyright 1997            Marcus Meissner
 *    Copyright 1998, 1999, 2002    Juergen Schmied
 *
 *    IShellFolder2 and related interfaces
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

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

/***************************************************************************
 *  SHELL32_GetCustomFolderAttributeFromPath (internal function)
 *
 * Gets a value from the folder's desktop.ini file, if one exists.
 *
 * PARAMETERS
 *  pwszFolderPath[I] Folder containing the desktop.ini file.
 *  pwszHeading   [I] Heading in .ini file.
 *  pwszAttribute [I] Attribute in .ini file.
 *  pwszValue     [O] Buffer to store value into.
 *  cchValue      [I] Size in characters including NULL of buffer pointed to
 *                    by pwszValue.
 *
 *  RETURNS
 *    TRUE if returned non-NULL value.
 *    FALSE otherwise.
 */
static BOOL __inline SHELL32_GetCustomFolderAttributeFromPath(
    LPWSTR pwszFolderPath, LPCWSTR pwszHeading, LPCWSTR pwszAttribute,
    LPWSTR pwszValue, DWORD cchValue)
{
    static const WCHAR wszDesktopIni[] =
            {'d','e','s','k','t','o','p','.','i','n','i',0};
    static const WCHAR wszDefault[] = {0};

    PathAddBackslashW(pwszFolderPath);
    PathAppendW(pwszFolderPath, wszDesktopIni);
    return GetPrivateProfileStringW(pwszHeading, pwszAttribute, wszDefault,
                                    pwszValue, cchValue, pwszFolderPath);
}

/***************************************************************************
 *  GetNextElement (internal function)
 *
 * Gets a part of a string till the first backslash.
 *
 * PARAMETERS
 *  pszNext [IN] string to get the element from
 *  pszOut  [IN] pointer to buffer which receives string
 *  dwOut   [IN] length of pszOut
 *
 *  RETURNS
 *    LPSTR pointer to first, not yet parsed char
 */

LPCWSTR GetNextElementW (LPCWSTR pszNext, LPWSTR pszOut, DWORD dwOut)
{
    LPCWSTR pszTail = pszNext;
    DWORD dwCopy;

    TRACE ("(%s %p 0x%08x)\n", debugstr_w (pszNext), pszOut, dwOut);

    *pszOut = 0x0000;

    if (!pszNext || !*pszNext)
        return NULL;

    while (*pszTail && (*pszTail != (WCHAR) '\\'))
        pszTail++;

    dwCopy = pszTail - pszNext + 1;
    lstrcpynW (pszOut, pszNext, (dwOut < dwCopy) ? dwOut : dwCopy);

    if (*pszTail)
        pszTail++;
    else
        pszTail = NULL;

    TRACE ("--(%s %s 0x%08x %p)\n", debugstr_w (pszNext), debugstr_w (pszOut), dwOut, pszTail);
    return pszTail;
}

HRESULT SHELL32_ParseNextElement (IShellFolder2 * psf, HWND hwndOwner, LPBC pbc,
                  LPITEMIDLIST * pidlInOut, LPOLESTR szNext, DWORD * pEaten, DWORD * pdwAttributes)
{
    HRESULT hr = E_INVALIDARG;
    LPITEMIDLIST pidlIn = pidlInOut ? *pidlInOut : NULL;
    LPITEMIDLIST pidlOut = NULL;
    LPITEMIDLIST pidlTemp = NULL;
    CComPtr<IShellFolder> psfChild;

    TRACE ("(%p, %p, %p, %s)\n", psf, pbc, pidlIn, debugstr_w (szNext));

    /* get the shellfolder for the child pidl and let it analyse further */
    hr = psf->BindToObject(pidlIn, pbc, IID_PPV_ARG(IShellFolder, &psfChild));
    if (FAILED(hr))
        return hr;

    hr = psfChild->ParseDisplayName(hwndOwner, pbc, szNext, pEaten, &pidlOut, pdwAttributes);
    if (FAILED(hr))
        return hr;

    pidlTemp = ILCombine (pidlIn, pidlOut);
    if (!pidlTemp)
    {
        hr = E_OUTOFMEMORY;
        if (pidlOut)
            ILFree(pidlOut);
        return hr;
    }

    if (pidlOut)
        ILFree (pidlOut);

    if (pidlIn)
        ILFree (pidlIn);

    *pidlInOut = pidlTemp;

    TRACE ("-- pidl=%p ret=0x%08x\n", pidlInOut ? *pidlInOut : NULL, hr);
    return S_OK;
}

/***********************************************************************
 *    SHELL32_CoCreateInitSF
 *
 * Creates a shell folder and initializes it with a pidl and a root folder
 * via IPersistFolder3 or IPersistFolder.
 *
 * NOTES
 *   pathRoot can be NULL for Folders being a drive.
 *   In this case the absolute path is built from pidlChild (eg. C:)
 */
static HRESULT SHELL32_CoCreateInitSF (LPCITEMIDLIST pidlRoot, LPCWSTR pathRoot,
                LPCITEMIDLIST pidlChild, REFCLSID clsid, IShellFolder** ppsfOut)
{
    HRESULT hr;
    CComPtr<IShellFolder> pShellFolder;

    TRACE ("%p %s %p\n", pidlRoot, debugstr_w(pathRoot), pidlChild);

    hr = SHCoCreateInstance(NULL, &clsid, NULL, IID_PPV_ARG(IShellFolder, &pShellFolder));
    if (SUCCEEDED (hr))
    {
        LPITEMIDLIST pidlAbsolute = ILCombine (pidlRoot, pidlChild);
        CComPtr<IPersistFolder> ppf;
        CComPtr<IPersistFolder3> ppf3;

        if (SUCCEEDED(pShellFolder->QueryInterface(IID_PPV_ARG(IPersistFolder3, &ppf3))))
        {
            PERSIST_FOLDER_TARGET_INFO ppfti;

            ZeroMemory (&ppfti, sizeof (ppfti));

            /* fill the PERSIST_FOLDER_TARGET_INFO */
            ppfti.dwAttributes = -1;
            ppfti.csidl = -1;

            /* build path */
            if (pathRoot)
            {
                lstrcpynW (ppfti.szTargetParsingName, pathRoot, MAX_PATH - 1);
                PathAddBackslashW(ppfti.szTargetParsingName); /* FIXME: why have drives a backslash here ? */
            }

            if (pidlChild)
            {
                int len = wcslen(ppfti.szTargetParsingName);

                if (!_ILSimpleGetTextW(pidlChild, ppfti.szTargetParsingName + len, MAX_PATH - len))
                    hr = E_INVALIDARG;
            }

            ppf3->InitializeEx(NULL, pidlAbsolute, &ppfti);
        }
        else if (SUCCEEDED((hr = pShellFolder->QueryInterface(IID_PPV_ARG(IPersistFolder, &ppf)))))
        {
            ppf->Initialize(pidlAbsolute);
        }
        ILFree (pidlAbsolute);
    }

    *ppsfOut = pShellFolder.Detach();

    TRACE ("-- (%p) ret=0x%08x\n", *ppsfOut, hr);

    return hr;
}

void SHELL32_GetCLSIDForDirectory(LPCWSTR pathRoot, LPCITEMIDLIST pidl, CLSID* pclsidFolder)
{
    static const WCHAR wszDotShellClassInfo[] = {
        '.','S','h','e','l','l','C','l','a','s','s','I','n','f','o',0 };
    static const WCHAR wszCLSID[] = {'C','L','S','I','D',0};
    WCHAR wszCLSIDValue[CHARS_IN_GUID], wszFolderPath[MAX_PATH], *pwszPathTail = wszFolderPath;

    /* see if folder CLSID should be overridden by desktop.ini file */
    if (pathRoot) {
        lstrcpynW(wszFolderPath, pathRoot, MAX_PATH);
        pwszPathTail = PathAddBackslashW(wszFolderPath);
    }

    _ILSimpleGetTextW(pidl,pwszPathTail,MAX_PATH - (int)(pwszPathTail - wszFolderPath));

    if (SHELL32_GetCustomFolderAttributeFromPath (wszFolderPath,
        wszDotShellClassInfo, wszCLSID, wszCLSIDValue, CHARS_IN_GUID))
        CLSIDFromString (wszCLSIDValue, pclsidFolder);
}

/***********************************************************************
 *    SHELL32_BindToFS [Internal]
 *
 * Common code for IShellFolder_BindToObject.
 *
 * PARAMS
 *  pidlRoot     [I] The parent shell folder's absolute pidl.
 *  pathRoot     [I] Absolute dos path of the parent shell folder.
 *  pidlComplete [I] PIDL of the child. Relative to pidlRoot.
 *  riid         [I] GUID of the interface, which ppvOut shall be bound to.
 *  ppvOut       [O] A reference to the child's interface (riid).
 *
 * NOTES
 *  pidlComplete has to contain at least one non empty SHITEMID.
 *  This function makes special assumptions on the shell namespace, which
 *  means you probably can't use it for your IShellFolder implementation.
 */
HRESULT SHELL32_BindToFS (LPCITEMIDLIST pidlRoot,
                             LPCWSTR pathRoot, LPCITEMIDLIST pidlComplete, REFIID riid, LPVOID * ppvOut)
{
    CComPtr<IShellFolder> pSF;
    HRESULT hr;
    LPCITEMIDLIST pidlChild;

    if (!pidlRoot || !ppvOut || !pidlComplete || !pidlComplete->mkid.cb)
        return E_INVALIDARG;

    if (_ILIsValue(pidlComplete))
    {
        ERR("Binding to file is unimplemented\n");
        return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }
    if (!_ILIsFolder(pidlComplete) && !_ILIsDrive(pidlComplete))
    {
        ERR("Got an unknown type of pidl!\n");
        return E_FAIL;
    }

    *ppvOut = NULL;

    pidlChild = (_ILIsPidlSimple (pidlComplete)) ? pidlComplete : ILCloneFirst (pidlComplete);

    CLSID clsidFolder = CLSID_ShellFSFolder;
    DWORD attributes = _ILGetFileAttributes(ILFindLastID(pidlChild), NULL, 0);
    if ((attributes & (FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_READONLY)) != 0)
        SHELL32_GetCLSIDForDirectory(pathRoot, pidlChild, &clsidFolder);

    hr = SHELL32_CoCreateInitSF (pidlRoot, pathRoot, pidlChild, clsidFolder, &pSF);

    if (pidlChild != pidlComplete)
        ILFree ((LPITEMIDLIST)pidlChild);

    if (SUCCEEDED (hr)) {
        if (_ILIsPidlSimple (pidlComplete)) {
            /* no sub folders */
            hr = pSF->QueryInterface(riid, ppvOut);
        } else {
            /* go deeper */
            hr = pSF->BindToObject(ILGetNext (pidlComplete), NULL, riid, ppvOut);
        }
    }

    TRACE ("-- returning (%p) %08x\n", *ppvOut, hr);

    return hr;
}

HRESULT SHELL32_BindToGuidItem(LPCITEMIDLIST pidlRoot,
                               PCUIDLIST_RELATIVE pidl,
                               LPBC pbcReserved,
                               REFIID riid,
                               LPVOID *ppvOut)
{
    CComPtr<IPersistFolder> pFolder;
    HRESULT hr;

    if (!pidlRoot || !ppvOut || !pidl || !pidl->mkid.cb)
        return E_INVALIDARG;

    *ppvOut = NULL;

    GUID *pGUID = _ILGetGUIDPointer(pidl);
    if (!pGUID)
    {
        ERR("SHELL32_BindToGuidItem called for non guid item!\n");
        return E_INVALIDARG;
    }

    hr = SHCoCreateInstance(NULL, pGUID, NULL, IID_PPV_ARG(IPersistFolder, &pFolder));
    if (FAILED(hr))
        return hr;

    if (_ILIsPidlSimple (pidl))
    {
        hr = pFolder->Initialize(ILCombine(pidlRoot, pidl));
        if (FAILED(hr))
            return hr;

        return pFolder->QueryInterface(riid, ppvOut);
    }
    else
    {
        LPITEMIDLIST pidlChild = ILCloneFirst (pidl);
        if (!pidlChild)
            return E_OUTOFMEMORY;

        hr = pFolder->Initialize(ILCombine(pidlRoot, pidlChild));
        ILFree(pidlChild);
        if (FAILED(hr))
            return hr;

        CComPtr<IShellFolder> psf;
        hr = pFolder->QueryInterface(IID_PPV_ARG(IShellFolder, &psf));
        if (FAILED(hr))
            return hr;

        return psf->BindToObject(ILGetNext (pidl), pbcReserved, riid, ppvOut);
    }
}

/***********************************************************************
 *    SHELL32_GetDisplayNameOfChild
 *
 * Retrieves the display name of a child object of a shellfolder.
 *
 * For a pidl eg. [subpidl1][subpidl2][subpidl3]:
 * - it binds to the child shellfolder [subpidl1]
 * - asks it for the displayname of [subpidl2][subpidl3]
 *
 * Is possible the pidl is a simple pidl. In this case it asks the
 * subfolder for the displayname of an empty pidl. The subfolder
 * returns the own displayname eg. "::{guid}". This is used for
 * virtual folders with the registry key WantsFORPARSING set.
 */
HRESULT SHELL32_GetDisplayNameOfChild (IShellFolder2 * psf,
                       LPCITEMIDLIST pidl, DWORD dwFlags, LPSTRRET strRet)
{
    LPITEMIDLIST pidlFirst = ILCloneFirst(pidl);
    if (!pidlFirst)
        return E_OUTOFMEMORY;

    CComPtr<IShellFolder> psfChild;
    HRESULT hr = psf->BindToObject(pidlFirst, NULL, IID_PPV_ARG(IShellFolder, &psfChild));
    if (SUCCEEDED (hr))
    {
        hr = psfChild->GetDisplayNameOf(ILGetNext (pidl), dwFlags, strRet);
    }
    ILFree (pidlFirst);

    return hr;
}

HRESULT HCR_GetClassName(REFIID riid, LPSTRRET strRet)
{
    BOOL bRet;
    WCHAR wstrName[MAX_PATH+1];
    bRet = HCR_GetClassNameW(riid, wstrName, MAX_PATH);
    if (!bRet)
        return E_FAIL;

    return SHSetStrRet(strRet, wstrName);
}

HRESULT SHELL32_GetDisplayNameOfGUIDItem(IShellFolder2* psf, LPCWSTR pszFolderPath, PCUITEMID_CHILD pidl, DWORD dwFlags, LPSTRRET strRet)
{
    HRESULT hr;
    GUID const *clsid = _ILGetGUIDPointer (pidl);

    if (!strRet)
        return E_INVALIDARG;

    /* First of all check if we need to query the name from the child item */
    if (GET_SHGDN_FOR (dwFlags) == SHGDN_FORPARSING && 
        GET_SHGDN_RELATION (dwFlags) == SHGDN_NORMAL)
    {
        int bWantsForParsing = FALSE;

        /*
            * We can only get a filesystem path from a shellfolder if the
            *  value WantsFORPARSING in CLSID\\{...}\\shellfolder exists.
            *
            * Exception: The MyComputer folder doesn't have this key,
            *   but any other filesystem backed folder it needs it.
            */
        if (IsEqualIID (*clsid, CLSID_MyComputer))
        {
            bWantsForParsing = TRUE;
        }
        else
        {
            HKEY hkeyClass;
            if (HCR_RegOpenClassIDKey(*clsid, &hkeyClass))
            {
                LONG res = SHGetValueW(hkeyClass, L"Shellfolder", L"WantsForParsing", NULL, NULL, NULL);
                bWantsForParsing = (res == ERROR_SUCCESS);
                RegCloseKey(hkeyClass);
            }
        }

        if (bWantsForParsing)
        {
            /*
             * we need the filesystem path to the destination folder.
             * Only the folder itself can know it
             */
            return SHELL32_GetDisplayNameOfChild (psf, pidl, dwFlags, strRet);
        }
    }

    /* Allocate the buffer for the result */
    LPWSTR pszPath = (LPWSTR)CoTaskMemAlloc((MAX_PATH + 1) * sizeof(WCHAR));
    if (!pszPath)
        return E_OUTOFMEMORY;

    hr = S_OK;

    if (GET_SHGDN_FOR (dwFlags) == SHGDN_FORPARSING)
    {
        wcscpy(pszPath, pszFolderPath);
        PWCHAR pItemName = &pszPath[wcslen(pszPath)];

        /* parsing name like ::{...} */
        pItemName[0] = ':';
        pItemName[1] = ':';
        SHELL32_GUIDToStringW (*clsid, &pItemName[2]);
    }
    else
    {
        /* user friendly name */
        if (!HCR_GetClassNameW (*clsid, pszPath, MAX_PATH))
            hr = E_FAIL;
    }

    if (SUCCEEDED(hr))
    {
        strRet->uType = STRRET_WSTR;
        strRet->pOleStr = pszPath;
    }
    else
    {
        CoTaskMemFree(pszPath);
    }

    return hr;
}

/***********************************************************************
 *  SHELL32_GetItemAttributes
 *
 * NOTES
 * Observed values:
 *  folder:    0xE0000177    FILESYSTEM | HASSUBFOLDER | FOLDER
 *  file:    0x40000177    FILESYSTEM
 *  drive:    0xf0000144    FILESYSTEM | HASSUBFOLDER | FOLDER | FILESYSANCESTOR
 *  mycomputer:    0xb0000154    HASSUBFOLDER | FOLDER | FILESYSANCESTOR
 *  (seems to be default for shell extensions if no registry entry exists)
 *
 * win2k:
 *  folder:    0xF0400177      FILESYSTEM | HASSUBFOLDER | FOLDER | FILESYSANCESTOR | CANMONIKER
 *  file:      0x40400177      FILESYSTEM | CANMONIKER
 *  drive      0xF0400154      FILESYSTEM | HASSUBFOLDER | FOLDER | FILESYSANCESTOR | CANMONIKER | CANRENAME (LABEL)
 *
 * According to the MSDN documentation this function should not set flags. It claims only to reset flags when necessary.
 * However it turns out the native shell32.dll _sets_ flags in several cases - so do we.
 */

static const DWORD dwSupportedAttr=
                      SFGAO_CANCOPY |           /*0x00000001 */
                      SFGAO_CANMOVE |           /*0x00000002 */
                      SFGAO_CANLINK |           /*0x00000004 */
                      SFGAO_CANRENAME |         /*0x00000010 */
                      SFGAO_CANDELETE |         /*0x00000020 */
                      SFGAO_HASPROPSHEET |      /*0x00000040 */
                      SFGAO_DROPTARGET |        /*0x00000100 */
                      SFGAO_LINK |              /*0x00010000 */
                      SFGAO_READONLY |          /*0x00040000 */
                      SFGAO_HIDDEN |            /*0x00080000 */
                      SFGAO_FILESYSANCESTOR |   /*0x10000000 */
                      SFGAO_FOLDER |            /*0x20000000 */
                      SFGAO_FILESYSTEM |        /*0x40000000 */
                      SFGAO_HASSUBFOLDER;       /*0x80000000 */

HRESULT SHELL32_GetGuidItemAttributes (IShellFolder * psf, LPCITEMIDLIST pidl, LPDWORD pdwAttributes)
{
    if (!_ILIsSpecialFolder(pidl))
    {
        ERR("Got wrong type of pidl!\n");
        *pdwAttributes &= SFGAO_CANLINK;
        return S_OK;
    }

    if (*pdwAttributes & ~dwSupportedAttr)
    {
        WARN ("attributes 0x%08x not implemented\n", (*pdwAttributes & ~dwSupportedAttr));
        *pdwAttributes &= dwSupportedAttr;
    }

    /* First try to get them from the registry */
    if (HCR_GetFolderAttributes(pidl, pdwAttributes) && *pdwAttributes)
    {
        return S_OK;
    }
    else
    {
        /* If we can't get it from the registry we have to query the child */
        CComPtr<IShellFolder> psf2;
        if (SUCCEEDED(psf->BindToObject(pidl, 0, IID_PPV_ARG(IShellFolder, &psf2))))
        {
            return psf2->GetAttributesOf(0, NULL, pdwAttributes);
        }
    }

    *pdwAttributes &= SFGAO_CANLINK;
    return S_OK;
}

HRESULT SHELL32_GetFSItemAttributes(IShellFolder * psf, LPCITEMIDLIST pidl, LPDWORD pdwAttributes)
{
    DWORD dwFileAttributes, dwShellAttributes;

    if (!_ILIsFolder(pidl) && !_ILIsValue(pidl))
    {
        ERR("Got wrong type of pidl!\n");
        *pdwAttributes &= SFGAO_CANLINK;
        return S_OK;
    }

    if (*pdwAttributes & ~dwSupportedAttr)
    {
        WARN ("attributes 0x%08x not implemented\n", (*pdwAttributes & ~dwSupportedAttr));
        *pdwAttributes &= dwSupportedAttr;
    }

    dwFileAttributes = _ILGetFileAttributes(pidl, NULL, 0);

    /* Set common attributes */
    dwShellAttributes = *pdwAttributes;
    dwShellAttributes |= SFGAO_FILESYSTEM | SFGAO_DROPTARGET | SFGAO_HASPROPSHEET | SFGAO_CANDELETE |
                         SFGAO_CANRENAME | SFGAO_CANLINK | SFGAO_CANMOVE | SFGAO_CANCOPY;

    if (dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        dwShellAttributes |=  (SFGAO_FOLDER | SFGAO_HASSUBFOLDER | SFGAO_FILESYSANCESTOR);
    }
    else
        dwShellAttributes &= ~(SFGAO_FOLDER | SFGAO_HASSUBFOLDER | SFGAO_FILESYSANCESTOR);

    if (dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
        dwShellAttributes |=  SFGAO_HIDDEN;
    else
        dwShellAttributes &= ~SFGAO_HIDDEN;

    if (dwFileAttributes & FILE_ATTRIBUTE_READONLY)
        dwShellAttributes |=  SFGAO_READONLY;
    else
        dwShellAttributes &= ~SFGAO_READONLY;

    if (SFGAO_LINK & *pdwAttributes)
    {
        char ext[MAX_PATH];

        if (!_ILGetExtension(pidl, ext, MAX_PATH) || lstrcmpiA(ext, "lnk"))
        dwShellAttributes &= ~SFGAO_LINK;
    }

    if (SFGAO_HASSUBFOLDER & *pdwAttributes)
    {
        CComPtr<IShellFolder> psf2;
        if (SUCCEEDED(psf->BindToObject(pidl, 0, IID_PPV_ARG(IShellFolder, &psf2))))
        {
            CComPtr<IEnumIDList> pEnumIL;
            if (SUCCEEDED(psf2->EnumObjects(0, SHCONTF_FOLDERS, &pEnumIL)))
            {
                if (pEnumIL->Skip(1) != S_OK)
                    dwShellAttributes &= ~SFGAO_HASSUBFOLDER;
            }
        }
    }

    *pdwAttributes &= dwShellAttributes;

    TRACE ("-- 0x%08x\n", *pdwAttributes);
    return S_OK;
}

HRESULT SHELL32_CompareDetails(IShellFolder2* isf, LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    SHELLDETAILS sd;
    WCHAR wszItem1[MAX_PATH], wszItem2[MAX_PATH];

    isf->GetDetailsOf(pidl1, lParam, &sd);
    StrRetToBufW(&sd.str, pidl1, wszItem1, MAX_PATH);
    isf->GetDetailsOf(pidl2, lParam, &sd);
    StrRetToBufW(&sd.str, pidl2, wszItem2, MAX_PATH);
    int ret = wcsicmp(wszItem1, wszItem2);

    return MAKE_COMPARE_HRESULT(ret);
}

HRESULT SHELL32_CompareGuidItems(IShellFolder2* isf, LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    if (pidl1->mkid.cb == 0 || pidl2->mkid.cb == 0)
    {
        ERR("Got an empty pidl!\n");
        return E_INVALIDARG;
    }

    BOOL bIsGuidFolder1 = _ILIsSpecialFolder(pidl1);
    BOOL bIsGuidFolder2 = _ILIsSpecialFolder(pidl2);

    if (!bIsGuidFolder1 && !bIsGuidFolder2)
    {
        ERR("Got no guid pidl!\n");
        return E_INVALIDARG;
    }
    else if (bIsGuidFolder1 && bIsGuidFolder2)
    {
        return SHELL32_CompareDetails(isf, lParam, pidl1, pidl2);
    }

    /* Guid folders come first compared to everything else */
    return MAKE_COMPARE_HRESULT(bIsGuidFolder1 ? -1 : 1);
}

HRESULT SH_ParseGuidDisplayName(IShellFolder2 * pFolder,
                                HWND hwndOwner,
                                LPBC pbc,
                                LPOLESTR lpszDisplayName,
                                DWORD *pchEaten,
                                PIDLIST_RELATIVE *ppidl,
                                DWORD *pdwAttributes)
{
    LPITEMIDLIST pidl;

    if (!lpszDisplayName || !ppidl)
        return E_INVALIDARG;

    *ppidl = 0;

    if (pchEaten)
        *pchEaten = 0;

    UINT cch = wcslen(lpszDisplayName);
    if (cch < 39 || lpszDisplayName[0] != L':' || lpszDisplayName[1] != L':')
        return E_FAIL;

    pidl = _ILCreateGuidFromStrW(lpszDisplayName + 2);
    if (pidl == NULL)
        return E_FAIL;

    if (cch < 41)
    {
        *ppidl = pidl;
        if (pdwAttributes && *pdwAttributes)
        {
            SHELL32_GetGuidItemAttributes(pFolder, *ppidl, pdwAttributes);
        }
    }
    else
    {
        HRESULT hr = SHELL32_ParseNextElement(pFolder, hwndOwner, pbc, &pidl, lpszDisplayName + 41, pchEaten, pdwAttributes);
        if (SUCCEEDED(hr))
        {
            *ppidl = pidl;
        }
        return hr;
    }

    return S_OK;
}

HRESULT SHELL32_SetNameOfGuidItem(PCUITEMID_CHILD pidl, LPCOLESTR lpName, DWORD dwFlags, PITEMID_CHILD *pPidlOut)
{
    GUID const *clsid = _ILGetGUIDPointer (pidl);
    LPOLESTR pStr;
    HRESULT hr;
    WCHAR szName[100];

    if (!clsid)
    {
        ERR("Pidl is not reg item!\n");
        return E_FAIL;
    }

    hr = StringFromCLSID(*clsid, &pStr);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    swprintf(szName, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CLSID\\%s", pStr);

    DWORD cbData = (wcslen(lpName) + 1) * sizeof(WCHAR);
    LONG res = SHSetValueW(HKEY_CURRENT_USER, szName, NULL, RRF_RT_REG_SZ, lpName, cbData);

    CoTaskMemFree(pStr);

    if (res == ERROR_SUCCESS)
    {
        *pPidlOut = ILClone(pidl);
        return S_OK;
    }

    return E_FAIL;
}

HRESULT SHELL32_GetDetailsOfGuidItem(IShellFolder2* psf, PCUITEMID_CHILD pidl, UINT iColumn, SHELLDETAILS *psd)
{
    GUID const *clsid = _ILGetGUIDPointer (pidl);

    if (!clsid)
    {
        ERR("Pidl is not reg item!\n");
        return E_FAIL;
    }

    switch(iColumn)
    {
        case 0:        /* name */
            return psf->GetDisplayNameOf(pidl, SHGDN_NORMAL | SHGDN_INFOLDER, &psd->str);
        case 1:        /* comment */
            HKEY hKey;
            if (HCR_RegOpenClassIDKey(*clsid, &hKey))
            {
                psd->str.cStr[0] = 0x00;
                psd->str.uType = STRRET_CSTR;
                RegLoadMUIStringA(hKey, "InfoTip", psd->str.cStr, MAX_PATH, NULL, 0, NULL);
                RegCloseKey(hKey);
                return S_OK;
            }
    }
    return E_FAIL;
}

/***********************************************************************
 *  SHCreateLinks
 *
 *   Undocumented.
 */
HRESULT WINAPI SHCreateLinks( HWND hWnd, LPCSTR lpszDir, IDataObject * lpDataObject,
                              UINT uFlags, LPITEMIDLIST *lppidlLinks)
{
    FIXME("%p %s %p %08x %p\n", hWnd, lpszDir, lpDataObject, uFlags, lppidlLinks);
    return E_NOTIMPL;
}

/***********************************************************************
 *  SHOpenFolderAndSelectItems
 *
 *   Unimplemented.
 */
EXTERN_C HRESULT
WINAPI
SHOpenFolderAndSelectItems(LPITEMIDLIST pidlFolder,
                           UINT cidl,
                           PCUITEMID_CHILD_ARRAY apidl,
                           DWORD dwFlags)
{
    FIXME("SHOpenFolderAndSelectItems() stub\n");
    return E_NOTIMPL;
}
