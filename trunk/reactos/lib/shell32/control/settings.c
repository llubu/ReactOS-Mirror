/*
 *  ReactOS shell32 - Control Panel
 *
 *  settings.c
 *
 *  Copyright (C) 2002  Robert Dickenson <robd@reactos.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
    
#define WIN32_LEAN_AND_MEAN     // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <cpl.h>
#include <commctrl.h>
#include <tchar.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <memory.h>
//#include <process.h>
    
#include "control.h"
//#include "framewnd.h"
//#include "shell32.h"
#include "trace.h"

//#define WIN32_LEAN_AND_MEAN     // Exclude rarely-used stuff from Windows headers
//#include <windows.h>
//#include <tchar.h>
//#include <assert.h>
//#define ASSERT assert
  
//#include "control.h"
#include "settings.h"


////////////////////////////////////////////////////////////////////////////////
// Global and Local Variables:
//

extern DWORD nListStyle;
extern DWORD nSortOrder;

////////////////////////////////////////////////////////////////////////////////
// Local module support methods
//

BOOL CheckResult(LONG error)
{
    if (error != ERROR_SUCCESS) {
    	PTSTR msg;
    	if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
	    	0, error, MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT), (PTSTR)&msg, 0, NULL))
		    MessageBox(NULL, msg, szTitle, MB_ICONERROR | MB_OK);
    	else
	    	MessageBox(NULL, _T("Error"), szTitle, MB_ICONERROR | MB_OK);
    	LocalFree(msg);
        return FALSE;
    }
    return TRUE;
}

static BOOL CreateRegistryPath(LPTSTR szRegPath, int nMaxLen)
{
    LPTSTR pRegPath = szRegPath;

    // Initialise registry path string from application PATH and KEY resources
    int nLength = LoadString(hInst, IDS_APP_REG_PATH, szRegPath, nMaxLen);
    nLength += LoadString(hInst, IDS_APP_REG_KEY, szRegPath + nLength, nMaxLen - nLength);
//    ASSERT(nLength < (nMaxLen - 1));
    szRegPath[nLength] = _T('\\');

    // walk the registry path string creating the tree if required
    while ((pRegPath = _tcschr(pRegPath, _T('\\')))) {
        LONG  result;
        HKEY  hKey = NULL;
        *pRegPath = _T('\0');
        // Open (or create) the key
        result = RegCreateKeyEx(HKEY_CURRENT_USER, szRegPath, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL);
        if (!CheckResult(result)) return FALSE;
        RegCloseKey(hKey);
        *pRegPath = _T('\\');
        pRegPath = pRegPath + 1;
    }
    szRegPath[nLength] = _T('\0');
    return TRUE;
}

BOOL LoadSettings(RECT* prc)
{
    BOOL retval = TRUE;
    TCHAR szRegPath[MAX_LOADSTRING];

    HKEY  hKey;
    DWORD dwSize;
    LONG  result;

    if (!CreateRegistryPath(szRegPath, MAX_LOADSTRING)) return FALSE;

    // Open the key
    result = RegOpenKeyEx(HKEY_CURRENT_USER, szRegPath, 0, KEY_READ, &hKey);
    if (!CheckResult(result)) return FALSE;

    // Read the settings
    dwSize = sizeof(nListStyle);
    result = RegQueryValueEx(hKey, _T("ListStyle"), NULL, NULL, (LPBYTE)&nListStyle, &dwSize);
    dwSize = sizeof(nSortOrder);
    result = RegQueryValueEx(hKey, _T("SortOrder"), NULL, NULL, (LPBYTE)&nSortOrder, &dwSize);
    dwSize = sizeof(RECT);
    result = RegQueryValueEx(hKey, _T("WindowPos"), NULL, NULL, (LPBYTE)prc, &dwSize);
    if (result != ERROR_SUCCESS) {
        retval = FALSE;
    }

    // Close the key
    RegCloseKey(hKey);
    return retval;
}

void SaveSettings(RECT* prc)
{
    TCHAR szRegPath[MAX_LOADSTRING];
    HKEY hKey = NULL;
    LONG result;

    if (!CreateRegistryPath(szRegPath, MAX_LOADSTRING)) return;

    // Open the key
    result = RegOpenKeyEx(HKEY_CURRENT_USER, szRegPath, 0, KEY_WRITE, &hKey);
    if (!CheckResult(result)) return;

    // Save the settings
    result = RegSetValueEx(hKey, _T("ListStyle"), 0, REG_DWORD, (LPBYTE)&nListStyle, sizeof(nListStyle));
    if (!CheckResult(result)) goto abort;
    result = RegSetValueEx(hKey, _T("SortOrder"), 0, REG_DWORD, (LPBYTE)&nSortOrder, sizeof(nSortOrder));
    if (!CheckResult(result)) goto abort;
    result = RegSetValueEx(hKey, _T("WindowPos"), 0, REG_BINARY, (LPBYTE)prc, sizeof(RECT));
    if (!CheckResult(result)) goto abort;

abort:
    // Close the key
    RegCloseKey(hKey);
}
