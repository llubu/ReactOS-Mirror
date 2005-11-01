/*
 *	Shell Folder stuff
 *
 *	Copyright 1997			Marcus Meissner
 *	Copyright 1998, 1999, 2002	Juergen Schmied
 *
 *	IShellFolder2 and related interfaces
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

#include "config.h"
#include "wine/port.h"

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#define COBJMACROS

#include "winerror.h"
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "wingdi.h"
#include "winuser.h"

#include "ole2.h"
#include "shlguid.h"

#include "pidl.h"
#include "undocshell.h"
#include "shell32_main.h"
#include "shlwapi.h"
#include "wine/debug.h"
#include "shfldr.h"

WINE_DEFAULT_DEBUG_CHANNEL (shell);

static const WCHAR wszDotShellClassInfo[] = {
    '.','S','h','e','l','l','C','l','a','s','s','I','n','f','o',0};

/***************************************************************************
 *  SHELL32_GetCustomFolderAttribute (internal function)
 *
 * Gets a value from the folder's desktop.ini file, if one exists.
 *
 * PARAMETERS
 *  pidl          [I] Folder containing the desktop.ini file.
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
static inline BOOL SHELL32_GetCustomFolderAttributeFromPath(
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

BOOL SHELL32_GetCustomFolderAttribute(
    LPCITEMIDLIST pidl, LPCWSTR pwszHeading, LPCWSTR pwszAttribute,
    LPWSTR pwszValue, DWORD cchValue)
{
    DWORD dwAttrib = FILE_ATTRIBUTE_SYSTEM;
    WCHAR wszFolderPath[MAX_PATH];

    /* Hack around not having system attribute on non-Windows file systems */
    if (0)
        dwAttrib = _ILGetFileAttributes(pidl, NULL, 0);

    if (dwAttrib & FILE_ATTRIBUTE_SYSTEM)
    {
        if (!SHGetPathFromIDListW(pidl, wszFolderPath))
            return FALSE;

        return SHELL32_GetCustomFolderAttributeFromPath(wszFolderPath, pwszHeading, 
                                                pwszAttribute, pwszValue, cchValue);
    }
    return FALSE;
}

/***************************************************************************
 *  GetNextElement (internal function)
 *
 * gets a part of a string till the first backslash
 *
 * PARAMETERS
 *  pszNext [IN] string to get the element from
 *  pszOut  [IN] pointer to buffer whitch receives string
 *  dwOut   [IN] length of pszOut
 *
 *  RETURNS
 *    LPSTR pointer to first, not yet parsed char
 */

LPCWSTR GetNextElementW (LPCWSTR pszNext, LPWSTR pszOut, DWORD dwOut)
{
    LPCWSTR pszTail = pszNext;
    DWORD dwCopy;

    TRACE ("(%s %p 0x%08lx)\n", debugstr_w (pszNext), pszOut, dwOut);

    *pszOut = 0x0000;

    if (!pszNext || !*pszNext)
	return NULL;

    while (*pszTail && (*pszTail != (WCHAR) '\\'))
	pszTail++;

    dwCopy = (const WCHAR *) pszTail - (const WCHAR *) pszNext + 1;
    lstrcpynW (pszOut, pszNext, (dwOut < dwCopy) ? dwOut : dwCopy);

    if (*pszTail)
	pszTail++;
    else
	pszTail = NULL;

    TRACE ("--(%s %s 0x%08lx %p)\n", debugstr_w (pszNext), debugstr_w (pszOut), dwOut, pszTail);
    return pszTail;
}

HRESULT SHELL32_ParseNextElement (IShellFolder2 * psf, HWND hwndOwner, LPBC pbc,
				  LPITEMIDLIST * pidlInOut, LPOLESTR szNext, DWORD * pEaten, DWORD * pdwAttributes)
{
    HRESULT hr = E_INVALIDARG;
    LPITEMIDLIST pidlOut = NULL,
      pidlTemp = NULL;
    IShellFolder *psfChild;

    TRACE ("(%p, %p, %p, %s)\n", psf, pbc, pidlInOut ? *pidlInOut : NULL, debugstr_w (szNext));

    /* get the shellfolder for the child pidl and let it analyse further */
    hr = IShellFolder_BindToObject (psf, *pidlInOut, pbc, &IID_IShellFolder, (LPVOID *) & psfChild);

    if (SUCCEEDED(hr)) {
	hr = IShellFolder_ParseDisplayName (psfChild, hwndOwner, pbc, szNext, pEaten, &pidlOut, pdwAttributes);
	IShellFolder_Release (psfChild);

	if (SUCCEEDED(hr)) {
	    pidlTemp = ILCombine (*pidlInOut, pidlOut);

	    if (!pidlTemp)
		hr = E_OUTOFMEMORY;
	}

	if (pidlOut)
	    ILFree (pidlOut);
    }

    ILFree (*pidlInOut);
    *pidlInOut = pidlTemp;

    TRACE ("-- pidl=%p ret=0x%08lx\n", pidlInOut ? *pidlInOut : NULL, hr);
    return hr;
}

/***********************************************************************
 *	SHELL32_CoCreateInitSF
 *
 * Creates a shell folder and initializes it with a pidl and a root folder
 * via IPersistFolder3 or IPersistFolder.
 *
 * NOTES
 *   pathRoot can be NULL for Folders beeing a drive.
 *   In this case the absolute path is build from pidlChild (eg. C:)
 */
HRESULT SHELL32_CoCreateInitSF (LPCITEMIDLIST pidlRoot,	LPCSTR pathRoot,
    LPCITEMIDLIST pidlChild, REFCLSID clsid, REFIID riid, LPVOID * ppvOut)
{
    HRESULT hr;

    TRACE ("%p %s %p\n", pidlRoot, pathRoot, pidlChild);

    if (SUCCEEDED ((hr = SHCoCreateInstance (NULL, clsid, NULL, riid, ppvOut)))) {
	LPITEMIDLIST pidlAbsolute = ILCombine (pidlRoot, pidlChild);
	IPersistFolder *pPF;
	IPersistFolder3 *ppf;

        if (_ILIsFolder(pidlChild) &&
            SUCCEEDED (IUnknown_QueryInterface ((IUnknown *) * ppvOut, &IID_IPersistFolder3, (LPVOID *) & ppf))) 
        {
	    PERSIST_FOLDER_TARGET_INFO ppfti;
	    char szDestPath[MAX_PATH];

	    ZeroMemory (&ppfti, sizeof (ppfti));

	    /* build path */
	    if (pathRoot) {
		lstrcpyA (szDestPath, pathRoot);
		PathAddBackslashA(szDestPath);          /* FIXME: why have drives a backslash here ? */
	    } else {
		szDestPath[0] = '\0';
	    }

	    if (pidlChild) {
		LPSTR pszChild = _ILGetTextPointer(pidlChild);

		if (pszChild)
		    lstrcatA (szDestPath, pszChild);
		else
		    hr = E_INVALIDARG;
	    }

	    /* fill the PERSIST_FOLDER_TARGET_INFO */
	    ppfti.dwAttributes = -1;
	    ppfti.csidl = -1;
	    MultiByteToWideChar (CP_ACP, 0, szDestPath, -1, ppfti.szTargetParsingName, MAX_PATH);

	    IPersistFolder3_InitializeEx (ppf, NULL, pidlAbsolute, &ppfti);
	    IPersistFolder3_Release (ppf);
	}
	else if (SUCCEEDED ((hr = IUnknown_QueryInterface ((IUnknown *) * ppvOut, &IID_IPersistFolder, (LPVOID *) & pPF)))) {
	    IPersistFolder_Initialize (pPF, pidlAbsolute);
	    IPersistFolder_Release (pPF);
	}
	ILFree (pidlAbsolute);
    }
    TRACE ("-- (%p) ret=0x%08lx\n", *ppvOut, hr);
    return hr;
}

/***********************************************************************
 *	SHELL32_BindToChild [Internal]
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
HRESULT SHELL32_BindToChild (LPCITEMIDLIST pidlRoot,
                             LPCSTR pathRoot, LPCITEMIDLIST pidlComplete, REFIID riid, LPVOID * ppvOut)
{
    GUID const *clsid;
    IShellFolder *pSF;
    HRESULT hr;
    LPITEMIDLIST pidlChild;

    if (!pidlRoot || !ppvOut || !pidlComplete || !pidlComplete->mkid.cb)
        return E_INVALIDARG;

    *ppvOut = NULL;

    pidlChild = ILCloneFirst (pidlComplete);

    if ((clsid = _ILGetGUIDPointer (pidlChild))) {
        /* virtual folder */
        hr = SHELL32_CoCreateInitSF (pidlRoot, pathRoot, pidlChild, clsid, &IID_IShellFolder, (LPVOID *) & pSF);
    } else {
        /* file system folder */
        CLSID clsidFolder = CLSID_ShellFSFolder;
        static const WCHAR wszCLSID[] = {'C','L','S','I','D',0};
        WCHAR wszCLSIDValue[CHARS_IN_GUID], wszFolderPath[MAX_PATH], *pwszPathTail = wszFolderPath;
       
        /* see if folder CLSID should be overridden by desktop.ini file */
        if (pathRoot) {
            MultiByteToWideChar(CP_ACP, 0, pathRoot, -1, wszFolderPath, MAX_PATH);
            pwszPathTail = PathAddBackslashW(wszFolderPath);
        }
        MultiByteToWideChar(CP_ACP, 0, _ILGetTextPointer(pidlChild), -1, pwszPathTail, MAX_PATH);
        if (SHELL32_GetCustomFolderAttributeFromPath (wszFolderPath,
            wszDotShellClassInfo, wszCLSID, wszCLSIDValue, CHARS_IN_GUID))
            CLSIDFromString (wszCLSIDValue, &clsidFolder);

		hr = SHELL32_CoCreateInitSF (pidlRoot, pathRoot, pidlChild,
            &clsidFolder, &IID_IShellFolder, (LPVOID *)&pSF);
    }
    ILFree (pidlChild);

    if (SUCCEEDED (hr)) {
        if (_ILIsPidlSimple (pidlComplete)) {
            /* no sub folders */
            hr = IShellFolder_QueryInterface (pSF, riid, ppvOut);
        } else {
            /* go deeper */
            hr = IShellFolder_BindToObject (pSF, ILGetNext (pidlComplete), NULL, riid, ppvOut);
        }
        IShellFolder_Release (pSF);
    }

    TRACE ("-- returning (%p) %08lx\n", *ppvOut, hr);

    return hr;
}

/***********************************************************************
 *	SHELL32_GetDisplayNameOfChild
 *
 * Retrives the display name of a child object of a shellfolder.
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
				       LPCITEMIDLIST pidl, DWORD dwFlags, LPSTR szOut, DWORD dwOutLen)
{
    LPITEMIDLIST pidlFirst;
    HRESULT hr = E_INVALIDARG;

    TRACE ("(%p)->(pidl=%p 0x%08lx %p 0x%08lx)\n", psf, pidl, dwFlags, szOut, dwOutLen);
    pdump (pidl);

    pidlFirst = ILCloneFirst (pidl);
    if (pidlFirst) {
	IShellFolder2 *psfChild;

	hr = IShellFolder_BindToObject (psf, pidlFirst, NULL, &IID_IShellFolder, (LPVOID *) & psfChild);
	if (SUCCEEDED (hr)) {
	    STRRET strTemp;
	    LPITEMIDLIST pidlNext = ILGetNext (pidl);

	    hr = IShellFolder_GetDisplayNameOf (psfChild, pidlNext, dwFlags, &strTemp);
	    if (SUCCEEDED (hr)) {
		hr = StrRetToStrNA (szOut, dwOutLen, &strTemp, pidlNext);
	    }
	    IShellFolder_Release (psfChild);
	}
	ILFree (pidlFirst);
    } else
	hr = E_OUTOFMEMORY;

    TRACE ("-- ret=0x%08lx %s\n", hr, szOut);

    return hr;
}

/***********************************************************************
 *  SHELL32_GetItemAttributes
 *
 * NOTES
 * observerd values:
 *  folder:	0xE0000177	FILESYSTEM | HASSUBFOLDER | FOLDER
 *  file:	0x40000177	FILESYSTEM
 *  drive:	0xf0000144	FILESYSTEM | HASSUBFOLDER | FOLDER | FILESYSANCESTOR
 *  mycomputer:	0xb0000154	HASSUBFOLDER | FOLDER | FILESYSANCESTOR
 *  (seems to be default for shell extensions if no registry entry exists)
 *
 * win2k:
 *  folder:    0xF0400177      FILESYSTEM | HASSUBFOLDER | FOLDER | FILESYSANCESTOR | CANMONIKER
 *  file:      0x40400177      FILESYSTEM | CANMONIKER
 *  drive      0xF0400154      FILESYSTEM | HASSUBFOLDER | FOLDER | FILESYSANCESTOR | CANMONIKER | CANRENAME (LABEL)
 *
 * This function does not set flags!! It only resets flags when necessary.
 */
HRESULT SHELL32_GetItemAttributes (IShellFolder * psf, LPCITEMIDLIST pidl, LPDWORD pdwAttributes)
{
    DWORD dwAttributes;
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
    
    TRACE ("0x%08lx\n", *pdwAttributes);

    if (*pdwAttributes & ~dwSupportedAttr)
    {
        WARN ("attributes 0x%08lx not implemented\n", (*pdwAttributes & ~dwSupportedAttr));
        *pdwAttributes &= dwSupportedAttr;
    }

    dwAttributes = *pdwAttributes;

    if (_ILIsDrive (pidl)) {
        *pdwAttributes &= SFGAO_HASSUBFOLDER|SFGAO_FILESYSTEM|SFGAO_FOLDER|SFGAO_FILESYSANCESTOR|
	    SFGAO_DROPTARGET|SFGAO_HASPROPSHEET|SFGAO_CANLINK;
    } else if (_ILGetGUIDPointer (pidl) && HCR_GetFolderAttributes(pidl, &dwAttributes)) {
	*pdwAttributes = dwAttributes;
    } else if (_ILGetDataPointer (pidl)) {
	dwAttributes = _ILGetFileAttributes (pidl, NULL, 0);

        if ((SFGAO_FILESYSANCESTOR & *pdwAttributes) && !(dwAttributes & FILE_ATTRIBUTE_DIRECTORY))
            *pdwAttributes &= ~SFGAO_FILESYSANCESTOR;

	if ((SFGAO_FOLDER & *pdwAttributes) && !(dwAttributes & FILE_ATTRIBUTE_DIRECTORY))
	    *pdwAttributes &= ~(SFGAO_FOLDER | SFGAO_HASSUBFOLDER);

	if ((SFGAO_HIDDEN & *pdwAttributes) && !(dwAttributes & FILE_ATTRIBUTE_HIDDEN))
	    *pdwAttributes &= ~SFGAO_HIDDEN;

	if ((SFGAO_READONLY & *pdwAttributes) && !(dwAttributes & FILE_ATTRIBUTE_READONLY))
	    *pdwAttributes &= ~SFGAO_READONLY;

	if (SFGAO_LINK & *pdwAttributes) {
	    char ext[MAX_PATH];

	    if (!_ILGetExtension(pidl, ext, MAX_PATH) || lstrcmpiA(ext, "lnk"))
		*pdwAttributes &= ~SFGAO_LINK;
	}

	if (SFGAO_HASSUBFOLDER & *pdwAttributes)
	{
	    IShellFolder *psf2;
	    if (SUCCEEDED(IShellFolder_BindToObject(psf, pidl, 0, (REFIID)&IID_IShellFolder, (LPVOID *)&psf2)))
	    {
	        IEnumIDList	*pEnumIL = NULL;
	        if (SUCCEEDED(IShellFolder_EnumObjects(psf2, 0, SHCONTF_FOLDERS, &pEnumIL)))
	        {
	            if (IEnumIDList_Skip(pEnumIL, 1) != S_OK)
	                *pdwAttributes &= ~SFGAO_HASSUBFOLDER;
	            IEnumIDList_Release(pEnumIL);
	        }
	        IShellFolder_Release(psf2);
	    }
	}
    } else {
	*pdwAttributes &= SFGAO_HASSUBFOLDER|SFGAO_FOLDER|SFGAO_FILESYSANCESTOR|SFGAO_DROPTARGET|SFGAO_HASPROPSHEET|SFGAO_CANRENAME|SFGAO_CANLINK;
    }
    TRACE ("-- 0x%08lx\n", *pdwAttributes);
    return S_OK;
}

/***********************************************************************
 *  SHELL32_CompareIDs
 */
HRESULT SHELL32_CompareIDs (IShellFolder * iface, LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    int type1,
      type2;
    char szTemp1[MAX_PATH];
    char szTemp2[MAX_PATH];
    HRESULT nReturn;
    LPITEMIDLIST firstpidl,
      nextpidl1,
      nextpidl2;
    IShellFolder *psf;

    /* test for empty pidls */
    BOOL isEmpty1 = _ILIsDesktop (pidl1);
    BOOL isEmpty2 = _ILIsDesktop (pidl2);

    if (isEmpty1 && isEmpty2)
        return MAKE_HRESULT( SEVERITY_SUCCESS, 0, 0 );
    if (isEmpty1)
        return MAKE_HRESULT( SEVERITY_SUCCESS, 0, (WORD)-1 );
    if (isEmpty2)
        return MAKE_HRESULT( SEVERITY_SUCCESS, 0, 1 );

    /* test for different types. Sort order is the PT_* constant */
    type1 = _ILGetDataPointer (pidl1)->type;
    type2 = _ILGetDataPointer (pidl2)->type;
    if (type1 < type2)
        return MAKE_HRESULT( SEVERITY_SUCCESS, 0, (WORD)-1 );
    else if (type1 > type2)
        return MAKE_HRESULT( SEVERITY_SUCCESS, 0, 1 );

    /* test for name of pidl */
    _ILSimpleGetText (pidl1, szTemp1, MAX_PATH);
    _ILSimpleGetText (pidl2, szTemp2, MAX_PATH);
    nReturn = lstrcmpiA (szTemp1, szTemp2);
    if (nReturn < 0)
        return MAKE_HRESULT( SEVERITY_SUCCESS, 0, (WORD)-1 );
    else if (nReturn > 0)
        return MAKE_HRESULT( SEVERITY_SUCCESS, 0, 1 );

    /* test of complex pidls */
    firstpidl = ILCloneFirst (pidl1);
    nextpidl1 = ILGetNext (pidl1);
    nextpidl2 = ILGetNext (pidl2);

    /* optimizing: test special cases and bind not deeper */
    /* the deeper shellfolder would do the same */
    isEmpty1 = _ILIsDesktop (nextpidl1);
    isEmpty2 = _ILIsDesktop (nextpidl2);

    if (isEmpty1 && isEmpty2) {
        return MAKE_HRESULT( SEVERITY_SUCCESS, 0, 0 );
    } else if (isEmpty1) {
        return MAKE_HRESULT( SEVERITY_SUCCESS, 0, (WORD)-1 );
    } else if (isEmpty2) {
        return MAKE_HRESULT( SEVERITY_SUCCESS, 0, 1 );
    /* optimizing end */
    } else if (SUCCEEDED (IShellFolder_BindToObject (iface, firstpidl, NULL, &IID_IShellFolder, (LPVOID *) & psf))) {
	nReturn = IShellFolder_CompareIDs (psf, lParam, nextpidl1, nextpidl2);
	IShellFolder_Release (psf);
    }
    ILFree (firstpidl);
    return nReturn;
}

/***********************************************************************
 *  SHCreateLinks
 *
 *   Undocumented.
 */
HRESULT WINAPI SHCreateLinks( HWND hWnd, LPCSTR lpszDir, LPDATAOBJECT lpDataObject,
                              UINT uFlags, LPITEMIDLIST *lppidlLinks)
{
    FIXME("%p %s %p %08x %p\n",hWnd,lpszDir,lpDataObject,uFlags,lppidlLinks);
    return E_NOTIMPL;
}
