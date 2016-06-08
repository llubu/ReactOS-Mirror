/*
 *    common shell dialogs
 *
 * Copyright 2000 Juergen Schmied
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

typedef struct
{
    HWND hwndOwner ;
    HICON hIcon ;
    LPCWSTR lpstrDirectory ;
    LPCWSTR lpstrTitle ;
    LPCWSTR lpstrDescription ;
    UINT uFlags ;
} RUNFILEDLGPARAMS ;

typedef BOOL (WINAPI * LPFNOFN) (OPENFILENAMEW *) ;

WINE_DEFAULT_DEBUG_CHANNEL(shell);
static INT_PTR CALLBACK RunDlgProc(HWND, UINT, WPARAM, LPARAM);
static void FillList(HWND, LPWSTR, BOOL);


/*************************************************************************
 * PickIconDlg                    [SHELL32.62]
 *
 */

typedef struct
{
    HMODULE hLibrary;
    HWND hDlgCtrl;
    WCHAR szName[MAX_PATH];
    INT Index;
} PICK_ICON_CONTEXT, *PPICK_ICON_CONTEXT;

BOOL CALLBACK EnumPickIconResourceProc(HMODULE hModule,
    LPCWSTR lpszType,
    LPWSTR lpszName,
    LONG_PTR lParam
)
{
    WCHAR szName[100];
    int index;
    HICON hIcon;
    HWND hDlgCtrl = (HWND)lParam;

    if (IS_INTRESOURCE(lpszName))
        swprintf(szName, L"%u", (DWORD)lpszName);
    else
        wcscpy(szName, (WCHAR*)lpszName);

    hIcon = LoadIconW(hModule, lpszName);
    if (hIcon == NULL)
        return TRUE;

    index = SendMessageW(hDlgCtrl, LB_ADDSTRING, 0, (LPARAM)szName);
    if (index != LB_ERR)
        SendMessageW(hDlgCtrl, LB_SETITEMDATA, index, (LPARAM)hIcon);

    return TRUE;
}

static void
DestroyIconList(HWND hDlgCtrl)
{
    int count;
    int index;

    count = SendMessageW(hDlgCtrl, LB_GETCOUNT, 0, 0);
    if (count == LB_ERR)
        return;

    for(index = 0; index < count; index++)
    {
        HICON hIcon = (HICON)SendMessageW(hDlgCtrl, LB_GETITEMDATA, index, 0);
        DestroyIcon(hIcon);
    }
}

INT_PTR CALLBACK PickIconProc(HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
    LPMEASUREITEMSTRUCT lpmis;
    LPDRAWITEMSTRUCT lpdis;
    HICON hIcon;
    INT index, count;
    WCHAR szText[MAX_PATH], szTitle[100], szFilter[100];
    OPENFILENAMEW ofn = {0};

    PPICK_ICON_CONTEXT pIconContext = (PPICK_ICON_CONTEXT)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch(uMsg)
    {
    case WM_INITDIALOG:
        pIconContext = (PPICK_ICON_CONTEXT)lParam;
        SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG)pIconContext);
        pIconContext->hDlgCtrl = GetDlgItem(hwndDlg, IDC_PICKICON_LIST);
        SendMessageW(pIconContext->hDlgCtrl, LB_SETCOLUMNWIDTH, 32, 0);
        EnumResourceNamesW(pIconContext->hLibrary, RT_ICON, EnumPickIconResourceProc, (LPARAM)pIconContext->hDlgCtrl);
        if (PathUnExpandEnvStringsW(pIconContext->szName, szText, MAX_PATH))
            SetDlgItemTextW(hwndDlg, IDC_EDIT_PATH, szText);
        else
            SetDlgItemTextW(hwndDlg, IDC_EDIT_PATH, pIconContext->szName);

        count = SendMessageW(pIconContext->hDlgCtrl, LB_GETCOUNT, 0, 0);
        if (count != LB_ERR)
        {
            if (count > pIconContext->Index)
                SendMessageW(pIconContext->hDlgCtrl, LB_SETCURSEL, pIconContext->Index, 0);
            else
                SendMessageW(pIconContext->hDlgCtrl, LB_SETCURSEL, 0, 0);
        }
        return TRUE;
    case WM_COMMAND:
        switch(LOWORD(wParam))
        {
        case IDOK:
            index = SendMessageW(pIconContext->hDlgCtrl, LB_GETCURSEL, 0, 0);
            pIconContext->Index = index;
            GetDlgItemTextW(hwndDlg, IDC_EDIT_PATH, pIconContext->szName, MAX_PATH);
            DestroyIconList(pIconContext->hDlgCtrl);
            EndDialog(hwndDlg, 1);
            break;
        case IDCANCEL:
            DestroyIconList(pIconContext->hDlgCtrl);
            EndDialog(hwndDlg, 0);
            break;
        case IDC_PICKICON_LIST:
            if (HIWORD(wParam) == LBN_SELCHANGE)
                InvalidateRect((HWND)lParam, NULL, TRUE); // FIXME USE UPDATE RECT
            break;
        case IDC_BUTTON_PATH:
            szText[0] = 0;
            szTitle[0] = 0;
            szFilter[0] = 0;
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = hwndDlg;
            ofn.lpstrFile = szText;
            ofn.nMaxFile = MAX_PATH;
            LoadStringW(shell32_hInstance, IDS_PICK_ICON_TITLE, szTitle, sizeof(szTitle) / sizeof(WCHAR));
            ofn.lpstrTitle = szTitle;
            LoadStringW(shell32_hInstance, IDS_PICK_ICON_FILTER, szFilter, sizeof(szFilter) / sizeof(WCHAR));
            ofn.lpstrFilter = szFilter;
            if (GetOpenFileNameW(&ofn))
            {
                HMODULE hLibrary;

                if (!wcsicmp(pIconContext->szName, szText))
                    break;

                DestroyIconList(pIconContext->hDlgCtrl);

                hLibrary = LoadLibraryExW(szText, NULL, LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_IMAGE_RESOURCE);
                if (hLibrary == NULL)
                    break;
                FreeLibrary(pIconContext->hLibrary);
                pIconContext->hLibrary = hLibrary;
                wcscpy(pIconContext->szName, szText);
                EnumResourceNamesW(pIconContext->hLibrary, RT_ICON, EnumPickIconResourceProc, (LPARAM)pIconContext->hDlgCtrl);
                if (PathUnExpandEnvStringsW(pIconContext->szName, szText, MAX_PATH))
                    SetDlgItemTextW(hwndDlg, IDC_EDIT_PATH, szText);
                else
                    SetDlgItemTextW(hwndDlg, IDC_EDIT_PATH, pIconContext->szName);

                SendMessageW(pIconContext->hDlgCtrl, LB_SETCURSEL, 0, 0);
            }
            break;
        }
        break;
        case WM_MEASUREITEM:
            lpmis = (LPMEASUREITEMSTRUCT) lParam;
            lpmis->itemHeight = 32;
            lpmis->itemWidth = 64;
            return TRUE;
        case WM_DRAWITEM:
            lpdis = (LPDRAWITEMSTRUCT) lParam;
            if (lpdis->itemID == (UINT)-1)
            {
                break;
            }
            switch (lpdis->itemAction)
            {
                case ODA_SELECT:
                case ODA_DRAWENTIRE:
                    index = SendMessageW(pIconContext->hDlgCtrl, LB_GETCURSEL, 0, 0);
                    hIcon = (HICON)SendMessageW(lpdis->hwndItem, LB_GETITEMDATA, lpdis->itemID, 0);

                    if (lpdis->itemID == (UINT)index)
                        FillRect(lpdis->hDC, &lpdis->rcItem, (HBRUSH)(COLOR_HIGHLIGHT + 1));
                    else
                        FillRect(lpdis->hDC, &lpdis->rcItem, (HBRUSH)(COLOR_WINDOW + 1));

                    DrawIconEx(lpdis->hDC, lpdis->rcItem.left,lpdis->rcItem.top, hIcon,
                               0, 0, 0, NULL, DI_NORMAL);
                    break;
            }
            break;
    }

    return FALSE;
}

BOOL WINAPI PickIconDlg(
    HWND hwndOwner,
    LPWSTR lpstrFile,
    UINT nMaxFile,
    INT* lpdwIconIndex)
{
    HMODULE hLibrary;
    int res;
    PICK_ICON_CONTEXT IconContext;

    hLibrary = LoadLibraryExW(lpstrFile, NULL, LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_IMAGE_RESOURCE);
    IconContext.hLibrary = hLibrary;
    IconContext.Index = *lpdwIconIndex;
    StringCchCopyNW(IconContext.szName, _countof(IconContext.szName), lpstrFile, nMaxFile);

    res = DialogBoxParamW(shell32_hInstance, MAKEINTRESOURCEW(IDD_PICK_ICON), hwndOwner, PickIconProc, (LPARAM)&IconContext);
    if (res)
    {
        StringCchCopyNW(lpstrFile, nMaxFile, IconContext.szName, _countof(IconContext.szName));
        *lpdwIconIndex = IconContext.Index;
    }

    FreeLibrary(hLibrary);
    return res;
}

/*************************************************************************
 * RunFileDlg                    [internal]
 *
 * The Unicode function that is available as ordinal 61 on Windows NT/2000/XP/...
 */
void WINAPI RunFileDlg(
    HWND hwndOwner,
    HICON hIcon,
    LPCWSTR lpstrDirectory,
    LPCWSTR lpstrTitle,
    LPCWSTR lpstrDescription,
    UINT uFlags)
{
    TRACE("\n");

    RUNFILEDLGPARAMS rfdp;
    rfdp.hwndOwner        = hwndOwner;
    rfdp.hIcon            = hIcon;
    rfdp.lpstrDirectory   = lpstrDirectory;
    rfdp.lpstrTitle       = lpstrTitle;
    rfdp.lpstrDescription = lpstrDescription;
    rfdp.uFlags           = uFlags;

    DialogBoxParamW(shell32_hInstance, MAKEINTRESOURCEW(IDD_RUN), hwndOwner, RunDlgProc, (LPARAM)&rfdp);
}


/* find the directory that contains the file being run */
static LPWSTR RunDlg_GetParentDir(LPCWSTR cmdline)
{
    const WCHAR *src;
    WCHAR *dest, *result, *result_end=NULL;
    static const WCHAR dotexeW[] = L".exe";

    result = (WCHAR *)HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR)*(strlenW(cmdline)+5));

    if (NULL == result)
    {
        TRACE("HeapAlloc couldn't allocate %d bytes\n", sizeof(WCHAR)*(strlenW(cmdline)+5));
        return NULL;
    }

    src = cmdline;
    dest = result;

    if (*src == '"')
    {
        src++;
        while (*src && *src != '"')
        {
            if (*src == '\\')
                result_end = dest;
            *dest++ = *src++;
        }
    }
    else {
        while (*src)
        {
            if (isspaceW(*src))
            {
                *dest = 0;
                if (INVALID_FILE_ATTRIBUTES != GetFileAttributesW(result))
                    break;
                strcatW(dest, dotexeW);
                if (INVALID_FILE_ATTRIBUTES != GetFileAttributesW(result))
                    break;
            }
            else if (*src == '\\')
                result_end = dest;
            *dest++ = *src++;
        }
    }

    if (result_end)
    {
        *result_end = 0;
        return result;
    }
    else
    {
        HeapFree(GetProcessHeap(), 0, result);
        return NULL;
    }
}

static void EnableOkButtonFromEditContents(HWND hwnd)
{
    BOOL Enable = FALSE;
    INT Length, n;
    HWND Edit = GetDlgItem(hwnd, IDC_RUNDLG_EDITPATH);
    Length = GetWindowTextLengthW(Edit);
    if (Length > 0)
    {
        PWCHAR psz = (PWCHAR)HeapAlloc(GetProcessHeap(), 0, (Length + 1) * sizeof(WCHAR));
        if (psz)
        {
            GetWindowTextW(Edit, psz, Length + 1);
            for (n = 0; n < Length && !Enable; ++n)
                Enable = psz[n] != ' ';
            HeapFree(GetProcessHeap(), 0, psz);
        }
        else
            Enable = TRUE;
    }
    EnableWindow(GetDlgItem(hwnd, IDOK), Enable);
}

/* Dialog procedure for RunFileDlg */
static INT_PTR CALLBACK RunDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    RUNFILEDLGPARAMS *prfdp = (RUNFILEDLGPARAMS *)GetWindowLongPtrW(hwnd, DWLP_USER);

    switch (message)
    {
        case WM_INITDIALOG:
            prfdp = (RUNFILEDLGPARAMS *)lParam ;
            SetWindowLongPtrW(hwnd, DWLP_USER, (LONG_PTR)prfdp);

            if (prfdp->lpstrTitle)
                SetWindowTextW(hwnd, prfdp->lpstrTitle);
            if (prfdp->lpstrDescription)
                SetWindowTextW(GetDlgItem(hwnd, IDC_RUNDLG_DESCRIPTION), prfdp->lpstrDescription);
            if (prfdp->uFlags & RFF_NOBROWSE)
            {
                HWND browse = GetDlgItem(hwnd, IDC_RUNDLG_BROWSE);
                ShowWindow(browse, SW_HIDE);
                EnableWindow(browse, FALSE);
            }
            if (prfdp->uFlags & RFF_NOLABEL)
                ShowWindow(GetDlgItem(hwnd, IDC_RUNDLG_LABEL), SW_HIDE);
            if (prfdp->uFlags & RFF_CALCDIRECTORY)
                FIXME("RFF_CALCDIRECTORY not supported\n");

            /* Use the default Shell Run icon if no one is specified */
            if (prfdp->hIcon == NULL)
                prfdp->hIcon = LoadIconW(shell32_hInstance, MAKEINTRESOURCEW(IDI_SHELL_RUN));
            /*
             * NOTE: Starting Windows Vista, the "Run File" dialog gets a
             * title icon that remains the same as the default one, even if
             * the user specifies a custom icon.
             * Since we currently imitate Windows 2003, therefore do not show
             * any title icon.
             */
            // SendMessageW(hwnd, WM_SETICON, ICON_BIG, (LPARAM)prfdp->hIcon);
            // SendMessageW(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)prfdp->hIcon);
            SendMessageW(GetDlgItem(hwnd, IDC_RUNDLG_ICON), STM_SETICON, (WPARAM)prfdp->hIcon, 0);

            FillList(GetDlgItem(hwnd, IDC_RUNDLG_EDITPATH), NULL, (prfdp->uFlags & RFF_NODEFAULT) == 0);
            EnableOkButtonFromEditContents(hwnd);
            SetFocus(GetDlgItem(hwnd, IDC_RUNDLG_EDITPATH));
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                {
                    int ic;
                    HWND htxt = GetDlgItem(hwnd, IDC_RUNDLG_EDITPATH);
                    if ((ic = GetWindowTextLengthW(htxt)))
                    {
                        WCHAR *psz, *parent = NULL;
                        SHELLEXECUTEINFOW sei;

                        ZeroMemory(&sei, sizeof(sei));
                        sei.cbSize = sizeof(sei);
                        psz = (WCHAR*)HeapAlloc(GetProcessHeap(), 0, (ic + 1)*sizeof(WCHAR));

                        if (psz)
                        {
                            GetWindowTextW(htxt, psz, ic + 1);

                            /* according to http://www.codeproject.com/KB/shell/runfiledlg.aspx we should send a
                             * WM_NOTIFY before execution */

                            sei.hwnd = hwnd;
                            sei.nShow = SW_SHOWNORMAL;
                            sei.lpFile = psz;

                            if (prfdp->lpstrDirectory)
                                sei.lpDirectory = prfdp->lpstrDirectory;
                            else
                                sei.lpDirectory = parent = RunDlg_GetParentDir(sei.lpFile);

                            if (!ShellExecuteExW(&sei))
                            {
                                HeapFree(GetProcessHeap(), 0, psz);
                                HeapFree(GetProcessHeap(), 0, parent);
                                SendMessageW(htxt, CB_SETEDITSEL, 0, MAKELPARAM (0, -1));
                                return TRUE;
                            }

                            GetWindowTextW(htxt, psz, ic + 1);
                            FillList(htxt, psz, FALSE);

                            HeapFree(GetProcessHeap(), 0, psz);
                            HeapFree(GetProcessHeap(), 0, parent);
                            EndDialog(hwnd, 0);
                        }
                    }
                }

                case IDCANCEL:
                    EndDialog(hwnd, 0);
                    return TRUE;

                case IDC_RUNDLG_BROWSE:
                {
                    HMODULE hComdlg = NULL;
                    LPFNOFN ofnProc = NULL;
                    static const WCHAR comdlg32W[] = L"comdlg32";
                    WCHAR szFName[1024] = {0};
                    WCHAR filter[MAX_PATH], szCaption[MAX_PATH];
                    OPENFILENAMEW ofn;

                    LoadStringW(shell32_hInstance, IDS_RUNDLG_BROWSE_FILTER, filter, MAX_PATH);
                    LoadStringW(shell32_hInstance, IDS_RUNDLG_BROWSE_CAPTION, szCaption, MAX_PATH);

                    ZeroMemory(&ofn, sizeof(ofn));
                    ofn.lStructSize = sizeof(ofn);
                    ofn.hwndOwner = hwnd;
                    ofn.lpstrFilter = filter;
                    ofn.lpstrFile = szFName;
                    ofn.nMaxFile = _countof(szFName) - 1;
                    ofn.lpstrTitle = szCaption;
                    ofn.Flags = OFN_ENABLESIZING | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
                    ofn.lpstrInitialDir = prfdp->lpstrDirectory;

                    if (NULL == (hComdlg = LoadLibraryExW(comdlg32W, NULL, 0)) ||
                        NULL == (ofnProc = (LPFNOFN)GetProcAddress(hComdlg, "GetOpenFileNameW")))
                    {
                        ERR("Couldn't get GetOpenFileName function entry (lib=%p, proc=%p)\n", hComdlg, ofnProc);
                        ShellMessageBoxW(shell32_hInstance, hwnd, MAKEINTRESOURCEW(IDS_RUNDLG_BROWSE_ERROR), NULL, MB_OK | MB_ICONERROR);
                        return TRUE;
                    }

                    if (ofnProc(&ofn))
                    {
                        SetFocus(GetDlgItem(hwnd, IDOK));
                        SetWindowTextW(GetDlgItem(hwnd, IDC_RUNDLG_EDITPATH), szFName);
                        SendMessageW(GetDlgItem(hwnd, IDC_RUNDLG_EDITPATH), CB_SETEDITSEL, 0, MAKELPARAM(0, -1));
                        EnableOkButtonFromEditContents(hwnd);
                        SetFocus(GetDlgItem(hwnd, IDOK));
                    }

                    FreeLibrary(hComdlg);

                    return TRUE;
                }
                case IDC_RUNDLG_EDITPATH:
                {
                    if (HIWORD(wParam) == CBN_EDITCHANGE)
                    {
                        EnableOkButtonFromEditContents(hwnd);
                    }
                    return TRUE;
                }
            }
            return TRUE;
    }
    return FALSE;
}

/*
 * This function grabs the MRU list from the registry and fills the combo-list
 * for the "Run" dialog above. fShowDefault is ignored if pszLatest != NULL.
 */
static void FillList(HWND hCb, LPWSTR pszLatest, BOOL fShowDefault)
{
    HKEY hkey;
    WCHAR *pszList = NULL, *pszCmd = NULL, *pszTmp = NULL, cMatch = 0, cMax = 0x60;
    WCHAR szIndex[2] = L"-";
    DWORD dwType, icList = 0, icCmd = 0;
    LRESULT lRet;
    UINT Nix;

    SendMessageW(hCb, CB_RESETCONTENT, 0, 0);

    lRet = RegCreateKeyExW(HKEY_CURRENT_USER,
                           L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\RunMRU",
                           0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkey, NULL);
    if (lRet != ERROR_SUCCESS)
    {
        TRACE("Unable to open or create the RunMRU key, error %d\n", GetLastError());
        return;
    }

    lRet = RegQueryValueExW(hkey, L"MRUList", NULL, &dwType, NULL, &icList);
    if (lRet == ERROR_SUCCESS && dwType == REG_SZ && icList > sizeof(WCHAR))
    {
        pszList = (WCHAR*)HeapAlloc(GetProcessHeap(), 0, icList);
        if (!pszList)
        {
            TRACE("HeapAlloc failed to allocate %d bytes\n", icList);
            goto Continue;
        }
        pszList[0] = L'\0';

        lRet = RegQueryValueExW(hkey, L"MRUList", NULL, NULL, (LPBYTE)pszList, &icList);
        if (lRet != ERROR_SUCCESS)
        {
            TRACE("Unable to grab MRUList, error %d\n", GetLastError());
            pszList[0] = L'\0';
        }
    }
    else
    {
Continue:
        icList = sizeof(WCHAR);
        pszList = (WCHAR*)HeapAlloc(GetProcessHeap(), 0, icList);
        if (!pszList)
        {
            TRACE("HeapAlloc failed to allocate %d bytes\n", icList);
            RegCloseKey(hkey);
            return;
        }
        pszList[0] = L'\0';
    }

    /* Convert the number of bytes from MRUList into number of characters (== number of indices) */
    icList /= sizeof(WCHAR);

    for (Nix = 0; Nix < icList - 1; Nix++)
    {
        if (pszList[Nix] > cMax)
            cMax = pszList[Nix];

        szIndex[0] = pszList[Nix];

        lRet = RegQueryValueExW(hkey, szIndex, NULL, &dwType, NULL, &icCmd);
        if (lRet != ERROR_SUCCESS || dwType != REG_SZ)
        {
            TRACE("Unable to grab size of index, error %d\n", GetLastError());
            continue;
        }

        if (pszCmd)
        {
            pszTmp = (WCHAR*)HeapReAlloc(GetProcessHeap(), 0, pszCmd, icCmd);
            if (!pszTmp)
            {
                TRACE("HeapReAlloc failed to reallocate %d bytes\n", icCmd);
                continue;
            }
            pszCmd = pszTmp;
        }
        else
        {
            pszCmd = (WCHAR*)HeapAlloc(GetProcessHeap(), 0, icCmd);
            if (!pszCmd)
            {
                TRACE("HeapAlloc failed to allocate %d bytes\n", icCmd);
                continue;
            }
        }

        lRet = RegQueryValueExW(hkey, szIndex, NULL, NULL, (LPBYTE)pszCmd, &icCmd);
        if (lRet != ERROR_SUCCESS)
        {
            TRACE("Unable to grab index, error %d\n", GetLastError());
            continue;
        }

        /*
         * Generally the command string will end up with "\\1".
         * Find the last backslash in the string and NULL-terminate.
         * Windows does not seem to check for what comes next, so that
         * a command of the form:
         *     c:\\my_dir\\myfile.exe
         * will be cut just after "my_dir", whereas a command of the form:
         *     c:\\my_dir\\myfile.exe\\1
         * will be cut just after "myfile.exe".
         */
        pszTmp = wcsrchr(pszCmd, L'\\');
        if (pszTmp)
            *pszTmp = L'\0';

        /*
         * In the following we try to add pszLatest to the MRU list.
         * We suppose that our caller has already correctly allocated
         * the string with enough space for us to append a "\\1".
         *
         * FIXME: TODO! (At the moment we don't append it!)
         */

        if (pszLatest)
        {
            if (wcsicmp(pszCmd, pszLatest) == 0)
            {
                SendMessageW(hCb, CB_INSERTSTRING, 0, (LPARAM)pszCmd);
                SetWindowTextW(hCb, pszCmd);
                SendMessageW(hCb, CB_SETEDITSEL, 0, MAKELPARAM(0, -1));

                cMatch = pszList[Nix];
                memmove(&pszList[1], pszList, Nix * sizeof(WCHAR));
                pszList[0] = cMatch;
                continue;
            }
        }

        if (icList - 1 != 26 || icList - 2 != Nix || cMatch || pszLatest == NULL)
        {
            SendMessageW(hCb, CB_ADDSTRING, 0, (LPARAM)pszCmd);
            if (!Nix && fShowDefault)
            {
                SetWindowTextW(hCb, pszCmd);
                SendMessageW(hCb, CB_SETEDITSEL, 0, MAKELPARAM(0, -1));
            }
        }
        else
        {
            SendMessageW(hCb, CB_INSERTSTRING, 0, (LPARAM)pszLatest);
            SetWindowTextW(hCb, pszLatest);
            SendMessageW(hCb, CB_SETEDITSEL, 0, MAKELPARAM(0, -1));

            cMatch = pszList[Nix];
            memmove(&pszList[1], pszList, Nix * sizeof(WCHAR));
            pszList[0] = cMatch;
            szIndex[0] = cMatch;
            RegSetValueExW(hkey, szIndex, 0, REG_SZ, (LPBYTE)pszLatest, (wcslen(pszLatest) + 1) * sizeof(WCHAR));
        }
    }

    if (!cMatch && pszLatest != NULL)
    {
        SendMessageW(hCb, CB_INSERTSTRING, 0, (LPARAM)pszLatest);
        SetWindowTextW(hCb, pszLatest);
        SendMessageW(hCb, CB_SETEDITSEL, 0, MAKELPARAM (0, -1));

        cMatch = ++cMax;

        if (pszList)
        {
            pszTmp = (WCHAR*)HeapReAlloc(GetProcessHeap(), 0, pszList, (++icList) * sizeof(WCHAR));
            if (!pszTmp)
            {
                TRACE("HeapReAlloc failed to reallocate enough bytes\n");
                goto Cleanup;
            }
            pszList = pszTmp;
        }
        else
        {
            pszList = (WCHAR*)HeapAlloc(GetProcessHeap(), 0, (++icList) * sizeof(WCHAR));
            if (!pszList)
            {
                TRACE("HeapAlloc failed to allocate enough bytes\n");
                goto Cleanup;
            }
        }

        memmove(&pszList[1], pszList, (icList - 1) * sizeof(WCHAR));
        pszList[0] = cMatch;
        szIndex[0] = cMatch;
        RegSetValueExW(hkey, szIndex, 0, REG_SZ, (LPBYTE)pszLatest, (wcslen(pszLatest) + 1) * sizeof(WCHAR));
    }

Cleanup:
    RegSetValueExW(hkey, L"MRUList", 0, REG_SZ, (LPBYTE)pszList, (wcslen(pszList) + 1) * sizeof(WCHAR));

    HeapFree(GetProcessHeap(), 0, pszCmd);
    HeapFree(GetProcessHeap(), 0, pszList);

    RegCloseKey(hkey);
}


/*************************************************************************
 * ConfirmDialog                [internal]
 *
 * Put up a confirm box, return TRUE if the user confirmed
 */
static BOOL ConfirmDialog(HWND hWndOwner, UINT PromptId, UINT TitleId)
{
  WCHAR Prompt[256];
  WCHAR Title[256];

  LoadStringW(shell32_hInstance, PromptId, Prompt, sizeof(Prompt) / sizeof(WCHAR));
  LoadStringW(shell32_hInstance, TitleId, Title, sizeof(Title) / sizeof(WCHAR));
  return MessageBoxW(hWndOwner, Prompt, Title, MB_YESNO|MB_ICONQUESTION) == IDYES;
}


/*************************************************************************
 * RestartDialogEx                [SHELL32.730]
 */

int WINAPI RestartDialogEx(HWND hWndOwner, LPCWSTR lpwstrReason, DWORD uFlags, DWORD uReason)
{
    TRACE("(%p)\n", hWndOwner);

    /* FIXME: use lpwstrReason */
    if (ConfirmDialog(hWndOwner, IDS_RESTART_PROMPT, IDS_RESTART_TITLE))
    {
        HANDLE hToken;
        TOKEN_PRIVILEGES npr;

        /* enable the shutdown privilege for the current process */
        if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken))
        {
            LookupPrivilegeValueA(0, "SeShutdownPrivilege", &npr.Privileges[0].Luid);
            npr.PrivilegeCount = 1;
            npr.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
            AdjustTokenPrivileges(hToken, FALSE, &npr, 0, 0, 0);
            CloseHandle(hToken);
        }
        ExitWindowsEx(EWX_REBOOT, uReason);
    }

    return 0;
}


/*************************************************************************
 * LogoffWindowsDialog  [SHELL32.54]
 */

EXTERN_C int WINAPI LogoffWindowsDialog(HWND hWndOwner)
{
    if (ConfirmDialog(hWndOwner, IDS_LOGOFF_PROMPT, IDS_LOGOFF_TITLE))
        ExitWindowsEx(EWX_LOGOFF, 0);

    return 0;
}


/*************************************************************************
 * RestartDialog                [SHELL32.59]
 */

int WINAPI RestartDialog(HWND hWndOwner, LPCWSTR lpstrReason, DWORD uFlags)
{
    return RestartDialogEx(hWndOwner, lpstrReason, uFlags, 0);
}

 /*************************************************************************
 * Used to get the shutdown privilege
 */
VOID ExitWindows_GetShutdownPrivilege(VOID)
{
    HANDLE hToken;
    TOKEN_PRIVILEGES npr;

    /* enable shut down privilege for current process */
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken))
    {
        LookupPrivilegeValueA(0, "SeShutdownPrivilege", &npr.Privileges[0].Luid);

        npr.PrivilegeCount = 1;
        npr.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        AdjustTokenPrivileges(hToken, FALSE, &npr, 0, 0, 0);

        CloseHandle(hToken);
    }
}

/*************************************************************************
 * ExitWindowsDialog_backup
 *
 * NOTES
 *     used as a backup solution to shutdown the OS in case msgina.dll somehow
 *     cannot be found.
 */
VOID ExitWindowsDialog_backup(HWND hWndOwner)
{
    TRACE("(%p)\n", hWndOwner);

    if (ConfirmDialog(hWndOwner, IDS_SHUTDOWN_PROMPT, IDS_SHUTDOWN_TITLE))
    {
        ExitWindows_GetShutdownPrivilege();
        ExitWindowsEx(EWX_SHUTDOWN, 0);
    }
}

/*************************************************************************
 * ExitWindowsDialog                [SHELL32.60]
 *
 * NOTES
 *     exported by ordinal
 */
/*
 * TODO:
 * - Implement the ability to show either the Welcome Screen or the classic dialog boxes based upon the
 *   registry value: SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon\LogonType.
 */
void WINAPI ExitWindowsDialog(HWND hWndOwner)
{
    typedef DWORD (WINAPI *ShellShFunc)(HWND hParent, WCHAR *Username, BOOL bHideLogoff);
    HINSTANCE msginaDll = LoadLibraryA("msgina.dll");

    TRACE("(%p)\n", hWndOwner);

    /* If the DLL cannot be found for any reason, then it simply uses a
       dialog box to ask if the user wants to shut down the computer. */
    if(!msginaDll)
    {
        TRACE("Unable to load msgina.dll.\n");
        ExitWindowsDialog_backup(hWndOwner);
        return;
    }

    ShellShFunc pShellShutdownDialog = (ShellShFunc) GetProcAddress(msginaDll, "ShellShutdownDialog");

    if(pShellShutdownDialog)
    {
        /* Actually call the function */
        DWORD returnValue = pShellShutdownDialog(hWndOwner, NULL, FALSE);

        switch(returnValue)
        {
        case 0x01: /* Log off user */
        {
            ExitWindowsEx(EWX_LOGOFF, 0);
            break;
        }
        case 0x02: /* Shut down */
        {
            ExitWindows_GetShutdownPrivilege();
            ExitWindowsEx(EWX_SHUTDOWN, 0);
            break;
        }
        case 0x03: /* Install Updates/Shutdown (?) */
        {
            break;
        }
        case 0x04: /* Reboot */
        {
            ExitWindows_GetShutdownPrivilege();
            ExitWindowsEx(EWX_REBOOT, 0);
            break;
        }
        case 0x10: /* Sleep */
        {
            if(IsPwrSuspendAllowed())
            {
                ExitWindows_GetShutdownPrivilege();
                SetSuspendState(FALSE, FALSE, FALSE);
            }
            break;
        }
        case 0x40: /* Hibernate */
        {
            if(IsPwrHibernateAllowed())
            {
                ExitWindows_GetShutdownPrivilege();
                SetSuspendState(TRUE, FALSE, TRUE);
            }
            break;
        }
        /* If the option is any other value */
        default:
            break;
        }
    }
    else
    {
        /* If the function cannot be found, then revert to using the backup solution */
        TRACE("Unable to find the 'ShellShutdownDialog' function");
        FreeLibrary(msginaDll);
        ExitWindowsDialog_backup(hWndOwner);
    }
}
