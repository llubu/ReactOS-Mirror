/*
 *  ReactOS applications
 *  Copyright (C) 2004-2008 ReactOS Team
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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS GUI first stage setup application
 * FILE:        base/setup/reactos/reactos.c
 * PROGRAMMERS: Eric Kohl
 *              Matthias Kupfer
 *              Dmitry Chapyshev (dmitry@reactos.org)
 */

#include "reactos.h"
#include "resource.h"

/* GLOBALS ******************************************************************/


LONG LoadGenentry(HINF hinf,PCTSTR name,PGENENTRY *gen,PINFCONTEXT context);

/* FUNCTIONS ****************************************************************/

static VOID
CenterWindow(HWND hWnd)
{
    HWND hWndParent;
    RECT rcParent;
    RECT rcWindow;

    hWndParent = GetParent(hWnd);
    if (hWndParent == NULL)
        hWndParent = GetDesktopWindow();

    GetWindowRect(hWndParent, &rcParent);
    GetWindowRect(hWnd, &rcWindow);

    SetWindowPos(hWnd,
                 HWND_TOP,
                 ((rcParent.right - rcParent.left) - (rcWindow.right - rcWindow.left)) / 2,
                 ((rcParent.bottom - rcParent.top) - (rcWindow.bottom - rcWindow.top)) / 2,
                 0,
                 0,
                 SWP_NOSIZE);
}

static HFONT
CreateTitleFont(VOID)
{
    NONCLIENTMETRICS ncm;
    LOGFONT LogFont;
    HDC hdc;
    INT FontSize;
    HFONT hFont;

    ncm.cbSize = sizeof(NONCLIENTMETRICS);
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);

    LogFont = ncm.lfMessageFont;
    LogFont.lfWeight = FW_BOLD;
    _tcscpy(LogFont.lfFaceName, _T("MS Shell Dlg"));

    hdc = GetDC(NULL);
    FontSize = 12;
    LogFont.lfHeight = 0 - GetDeviceCaps (hdc, LOGPIXELSY) * FontSize / 72;
    hFont = CreateFontIndirect(&LogFont);
    ReleaseDC(NULL, hdc);

    return hFont;
}

static INT_PTR CALLBACK
StartDlgProc(HWND hwndDlg,
             UINT uMsg,
             WPARAM wParam,
             LPARAM lParam)
{
    PSETUPDATA pSetupData;

    /* Retrieve pointer to the global setup data */
    pSetupData = (PSETUPDATA)GetWindowLongPtr (hwndDlg, GWL_USERDATA);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            /* Save pointer to the global setup data */
            pSetupData = (PSETUPDATA)((LPPROPSHEETPAGE)lParam)->lParam;
            SetWindowLongPtr(hwndDlg, GWL_USERDATA, (DWORD_PTR)pSetupData);

            /* Center the wizard window */
            CenterWindow(GetParent(hwndDlg));

            /* Set title font */
            SendDlgItemMessage(hwndDlg,
                               IDC_STARTTITLE,
                               WM_SETFONT,
                               (WPARAM)pSetupData->hTitleFont,
                               (LPARAM)TRUE);
            break;

        case WM_NOTIFY:
        {
            LPNMHDR lpnm = (LPNMHDR)lParam;

            switch (lpnm->code)
            {
                case PSN_SETACTIVE:
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT);
                    break;

                default:
                    break;
            }
        }
        break;

        default:
            break;

    }

    return FALSE;
}

static INT_PTR CALLBACK
TypeDlgProc(HWND hwndDlg,
            UINT uMsg,
            WPARAM wParam,
            LPARAM lParam)
{
    PSETUPDATA pSetupData;

    /* Retrieve pointer to the global setup data */
    pSetupData = (PSETUPDATA)GetWindowLongPtr (hwndDlg, GWL_USERDATA);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            /* Save pointer to the global setup data */
            pSetupData = (PSETUPDATA)((LPPROPSHEETPAGE)lParam)->lParam;
            SetWindowLongPtr(hwndDlg, GWL_USERDATA, (DWORD_PTR)pSetupData);

            /* Check the 'install' radio button */
            CheckDlgButton(hwndDlg, IDC_INSTALL, BST_CHECKED);

            /* Disable the 'update' radio button and text */
            EnableWindow(GetDlgItem(hwndDlg, IDC_UPDATE), FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_UPDATETEXT), FALSE);
            break;

        case WM_NOTIFY:
        {
            LPNMHDR lpnm = (LPNMHDR)lParam;

            switch (lpnm->code)
            {
                case PSN_SETACTIVE:
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT | PSWIZB_BACK);
                    break;

                case PSN_QUERYCANCEL:
                    SetWindowLongPtr(hwndDlg,
                                     DWL_MSGRESULT,
                                     MessageBox(GetParent(hwndDlg),
                                                pSetupData->szAbortMessage,
                                                pSetupData->szAbortTitle,
                                                MB_YESNO | MB_ICONQUESTION) != IDYES);
                    return TRUE;

                case PSN_WIZNEXT: // set the selected data
                    pSetupData->RepairUpdateFlag = !(SendMessage(GetDlgItem(hwndDlg, IDC_INSTALL),
                                                                 BM_GETCHECK,
                                                                 (WPARAM) 0,
                                                                 (LPARAM) 0) == BST_CHECKED);
                    return TRUE;

                default:
                    break;
            }
        }
        break;

        default:
            break;

    }
    return FALSE;
}

static INT_PTR CALLBACK
DeviceDlgProc(HWND hwndDlg,
              UINT uMsg,
              WPARAM wParam,
              LPARAM lParam)
{
    PSETUPDATA pSetupData;
    LONG i;
    LRESULT tindex;
    HWND hList;

    /* Retrieve pointer to the global setup data */
    pSetupData = (PSETUPDATA)GetWindowLongPtr (hwndDlg, GWL_USERDATA);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            /* Save pointer to the global setup data */
            pSetupData = (PSETUPDATA)((LPPROPSHEETPAGE)lParam)->lParam;
            SetWindowLongPtr(hwndDlg, GWL_USERDATA, (DWORD_PTR)pSetupData);

            hList = GetDlgItem(hwndDlg, IDC_COMPUTER);

            for (i=0; i < pSetupData->CompCount; i++)
            {
                tindex = SendMessage(hList, CB_ADDSTRING, (WPARAM) 0, (LPARAM) pSetupData->pComputers[i].Value);
                SendMessage(hList, CB_SETITEMDATA, tindex, i);
            }
            SendMessage(hList, CB_SETCURSEL, 0, 0); // set first as default

            hList = GetDlgItem(hwndDlg, IDC_DISPLAY);

            for (i=0; i < pSetupData->DispCount; i++)
            {
                tindex = SendMessage(hList, CB_ADDSTRING, (WPARAM) 0, (LPARAM) pSetupData->pDisplays[i].Value);
                SendMessage(hList, CB_SETITEMDATA, tindex, i);
            }
            SendMessage(hList, CB_SETCURSEL, 0, 0); // set first as default

            hList = GetDlgItem(hwndDlg, IDC_KEYBOARD);

            for (i=0; i < pSetupData->KeybCount; i++)
            {
                tindex = SendMessage(hList,CB_ADDSTRING,(WPARAM)0,(LPARAM)pSetupData->pKeyboards[i].Value);
                SendMessage(hList,CB_SETITEMDATA,tindex,i);
            }
            SendMessage(hList,CB_SETCURSEL,0,0); // set first as default
            break;

        case WM_NOTIFY:
        {
            LPNMHDR lpnm = (LPNMHDR)lParam;

            switch (lpnm->code)
            {
                case PSN_SETACTIVE:
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT | PSWIZB_BACK);
                    break;

                case PSN_QUERYCANCEL:
                    SetWindowLongPtr(hwndDlg,
                                     DWL_MSGRESULT,
                                     MessageBox(GetParent(hwndDlg),
                                                pSetupData->szAbortMessage,
                                                pSetupData->szAbortTitle,
                                             MB_YESNO | MB_ICONQUESTION) != IDYES);
                    return TRUE;

                case PSN_WIZNEXT: // set the selected data
                {
                    hList = GetDlgItem(hwndDlg, IDC_COMPUTER); 

                    tindex = SendMessage(hList, CB_GETCURSEL, (WPARAM) 0, (LPARAM) 0);
                    if (tindex != CB_ERR)
                    {
                        pSetupData->SelectedComputer = SendMessage(hList,
                                                                 CB_GETITEMDATA,
                                                                 (WPARAM) tindex,
                                                                 (LPARAM) 0);
                    }

                    hList = GetDlgItem(hwndDlg, IDC_DISPLAY);

                    tindex = SendMessage(hList, CB_GETCURSEL, (WPARAM) 0, (LPARAM) 0);
                    if (tindex != CB_ERR)
                    {
                        pSetupData->SelectedDisplay = SendMessage(hList,
                                                                CB_GETITEMDATA,
                                                                (WPARAM) tindex,
                                                                (LPARAM) 0);
                    }

                    hList =GetDlgItem(hwndDlg, IDC_KEYBOARD);

                    tindex = SendMessage(hList, CB_GETCURSEL, (WPARAM) 0, (LPARAM) 0);
                    if (tindex != CB_ERR)
                    {
                        pSetupData->SelectedKeyboard = SendMessage(hList,
                                                                 CB_GETITEMDATA,
                                                                 (WPARAM) tindex,
                                                                 (LPARAM) 0);
                    }
                    return TRUE;
                }

                default:
                    break;
            }
        }
        break;

        default:
            break;

    }
    return FALSE;
}

static INT_PTR CALLBACK
SummaryDlgProc(HWND hwndDlg,
               UINT uMsg,
               WPARAM wParam,
               LPARAM lParam)
{
    PSETUPDATA pSetupData;

    /* Retrieve pointer to the global setup data */
    pSetupData = (PSETUPDATA)GetWindowLongPtr (hwndDlg, GWL_USERDATA);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            /* Save pointer to the global setup data */
            pSetupData = (PSETUPDATA)((LPPROPSHEETPAGE)lParam)->lParam;
            SetWindowLongPtr(hwndDlg, GWL_USERDATA, (DWORD_PTR)pSetupData);
            break;

        case WM_NOTIFY:
        {
            LPNMHDR lpnm = (LPNMHDR)lParam;

            switch (lpnm->code)
            {
                case PSN_SETACTIVE: 
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT | PSWIZB_BACK);
                    break;

                case PSN_QUERYCANCEL:
                    SetWindowLongPtr(hwndDlg,
                                     DWL_MSGRESULT,
                                     MessageBox(GetParent(hwndDlg),
                                                pSetupData->szAbortMessage,
                                                pSetupData->szAbortTitle,
                                                MB_YESNO | MB_ICONQUESTION) != IDYES);
                    return TRUE;
                default:
                    break;
            }
        }
        break;

        default:
            break;
    }

    return FALSE;
}

static INT_PTR CALLBACK
ProcessDlgProc(HWND hwndDlg,
               UINT uMsg,
               WPARAM wParam,
               LPARAM lParam)
{
    PSETUPDATA pSetupData;

    /* Retrieve pointer to the global setup data */
    pSetupData = (PSETUPDATA)GetWindowLongPtr (hwndDlg, GWL_USERDATA);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            /* Save pointer to the global setup data */
            pSetupData = (PSETUPDATA)((LPPROPSHEETPAGE)lParam)->lParam;
            SetWindowLongPtr(hwndDlg, GWL_USERDATA, (DWORD_PTR)pSetupData);
            break;

        case WM_NOTIFY:
        {
            LPNMHDR lpnm = (LPNMHDR)lParam;

            switch (lpnm->code)
            {
                case PSN_SETACTIVE:
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT);
                   // disable all buttons during installation process
                   // PropSheet_SetWizButtons(GetParent(hwndDlg), 0 );
                   break;
                case PSN_QUERYCANCEL:
                    SetWindowLongPtr(hwndDlg,
                                     DWL_MSGRESULT,
                                     MessageBox(GetParent(hwndDlg),
                                                pSetupData->szAbortMessage,
                                                pSetupData->szAbortTitle,
                                                MB_YESNO | MB_ICONQUESTION) != IDYES);
                    return TRUE;
                default:
                   break;
            }
        }
        break;

        default:
            break;

    }

    return FALSE;
}

static INT_PTR CALLBACK
RestartDlgProc(HWND hwndDlg,
               UINT uMsg,
               WPARAM wParam,
               LPARAM lParam)
{
    PSETUPDATA pSetupData;

    /* Retrieve pointer to the global setup data */
    pSetupData = (PSETUPDATA)GetWindowLongPtr (hwndDlg, GWL_USERDATA);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            /* Save pointer to the global setup data */
            pSetupData = (PSETUPDATA)((LPPROPSHEETPAGE)lParam)->lParam;
            SetWindowLongPtr(hwndDlg, GWL_USERDATA, (DWORD_PTR)pSetupData);

            /* Set title font */
            /*SendDlgItemMessage(hwndDlg,
                                 IDC_STARTTITLE,
                                 WM_SETFONT,
                                 (WPARAM)hTitleFont,
                                 (LPARAM)TRUE);*/
            break;

        case WM_TIMER:
        {
            INT Position;
            HWND hWndProgress;

            hWndProgress = GetDlgItem(hwndDlg, IDC_RESTART_PROGRESS);
            Position = SendMessage(hWndProgress, PBM_GETPOS, 0, 0);
            if (Position == 300)
            {
                KillTimer(hwndDlg, 1);
                PropSheet_PressButton(GetParent(hwndDlg), PSBTN_FINISH);
            }
            else
            {
                SendMessage(hWndProgress, PBM_SETPOS, Position + 1, 0);
            }
            return TRUE;
        }

        case WM_DESTROY:
            return TRUE;

        case WM_NOTIFY:
        {
            LPNMHDR lpnm = (LPNMHDR)lParam;

            switch (lpnm->code)
            {
                case PSN_SETACTIVE: // Only "Finish" for closing the App
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_FINISH);
                    SendDlgItemMessage(hwndDlg, IDC_RESTART_PROGRESS, PBM_SETRANGE, 0, MAKELPARAM(0, 300));
                    SendDlgItemMessage(hwndDlg, IDC_RESTART_PROGRESS, PBM_SETPOS, 0, 0);
                    SetTimer(hwndDlg, 1, 50, NULL);
                    break;

                default:
                    break;
            }
        }
        break;

        default:
            break;

    }

    return FALSE;
}

BOOL LoadSetupData(
    PSETUPDATA pSetupData)
{
    WCHAR szPath[MAX_PATH];
    TCHAR tmp[10];
    WCHAR *ch;
    HINF hTxtsetupSif;
    INFCONTEXT InfContext;
    //TCHAR szValue[MAX_PATH];
    DWORD LineLength;
    LONG Count;

    GetModuleFileNameW(NULL,szPath,MAX_PATH);
    ch = strrchrW(szPath,L'\\');
    if (ch != NULL)
        *ch = L'\0';

    wcscat(szPath, L"\\txtsetup.sif");
    hTxtsetupSif = SetupOpenInfFileW(szPath, NULL, INF_STYLE_OLDNT, NULL);
    if (hTxtsetupSif == INVALID_HANDLE_VALUE)
    {
        TCHAR message[512], caption[64];

        // txtsetup.sif cannot be found
        LoadString(pSetupData->hInstance, IDS_NO_TXTSETUP_SIF, message, sizeof(message)/sizeof(TCHAR));
        LoadString(pSetupData->hInstance, IDS_CAPTION, caption, sizeof(caption)/sizeof(TCHAR));

        MessageBox(NULL, message, caption, MB_OK | MB_ICONERROR);
        return FALSE;
    }

    // get language list
    pSetupData->LangCount = SetupGetLineCount(hTxtsetupSif, _T("Language"));
    if (pSetupData->LangCount > 0)
    {
        pSetupData->pLanguages = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(LANG) * pSetupData->LangCount);
        if (pSetupData->pLanguages != NULL)
        {
            Count = 0;
            if (SetupFindFirstLine(hTxtsetupSif, _T("Language"), NULL, &InfContext))
            {
                do
                {
                    SetupGetStringField(&InfContext,
                                        0,
                                        pSetupData->pLanguages[Count].LangId,
                                        sizeof(pSetupData->pLanguages[Count].LangId) / sizeof(TCHAR),
                                        &LineLength);

                    SetupGetStringField(&InfContext,
                                        1,
                                        pSetupData->pLanguages[Count].LangName,
                                        sizeof(pSetupData->pLanguages[Count].LangName) / sizeof(TCHAR),
                                        &LineLength);
                    ++Count;
                }
                while (SetupFindNextLine(&InfContext, &InfContext) && Count < pSetupData->LangCount);
            }
        }
    }

    // get keyboard layout list
    pSetupData->KbLayoutCount = SetupGetLineCount(hTxtsetupSif, _T("KeyboardLayout"));
    if (pSetupData->KbLayoutCount > 0)
    {
        pSetupData->pKbLayouts = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(KBLAYOUT) * pSetupData->KbLayoutCount);
        if (pSetupData->pKbLayouts != NULL)
        {
            Count = 0;
            if (SetupFindFirstLine(hTxtsetupSif, _T("KeyboardLayout"), NULL, &InfContext))
            {
                do
                {
                    SetupGetStringField(&InfContext,
                                        0,
                                        pSetupData->pKbLayouts[Count].LayoutId,
                                        sizeof(pSetupData->pKbLayouts[Count].LayoutId) / sizeof(TCHAR),
                                        &LineLength);

                    SetupGetStringField(&InfContext,
                                        1,
                                        pSetupData->pKbLayouts[Count].LayoutName,
                                        sizeof(pSetupData->pKbLayouts[Count].LayoutName) / sizeof(TCHAR),
                                        &LineLength);
                    ++Count;
                }
                while (SetupFindNextLine(&InfContext, &InfContext) && Count < pSetupData->KbLayoutCount);
            }
        }
    }

    // get default for keyboard and language
    pSetupData->DefaultKBLayout = -1;
    pSetupData->DefaultLang = -1;

    // TODO: get defaults from underlaying running system
    if (SetupFindFirstLine(hTxtsetupSif, _T("NLS"), _T("DefaultLayout"), &InfContext))
    {
        SetupGetStringField(&InfContext, 1, tmp, sizeof(tmp) / sizeof(TCHAR), &LineLength);
        for (Count = 0; Count < pSetupData->KbLayoutCount; Count++)
            if (_tcscmp(tmp, pSetupData->pKbLayouts[Count].LayoutId) == 0)
            {
                pSetupData->DefaultKBLayout = Count;
                break;
            }
    }

    if (SetupFindFirstLine(hTxtsetupSif, _T("NLS"), _T("DefaultLanguage"), &InfContext))
    {
        SetupGetStringField(&InfContext, 1, tmp, sizeof(tmp) / sizeof(TCHAR), &LineLength);
        for (Count = 0; Count < pSetupData->LangCount; Count++)
            if (_tcscmp(tmp, pSetupData->pLanguages[Count].LangId) == 0)
            {
                pSetupData->DefaultLang = Count;
                break;
            }
    }

    // get computers list
    pSetupData->CompCount = LoadGenentry(hTxtsetupSif,_T("Computer"),&pSetupData->pComputers,&InfContext);

    // get display list
    pSetupData->DispCount = LoadGenentry(hTxtsetupSif,_T("Display"),&pSetupData->pDisplays,&InfContext);

    // get keyboard list
    pSetupData->KeybCount = LoadGenentry(hTxtsetupSif, _T("Keyboard"),&pSetupData->pKeyboards,&InfContext);

    // get install directory
    if (SetupFindFirstLine(hTxtsetupSif, _T("SetupData"), _T("DefaultPath"), &InfContext))
    {
        SetupGetStringField(&InfContext,
                            1,
                            pSetupData->InstallDir,
                            sizeof(pSetupData->InstallDir) / sizeof(TCHAR),
                            &LineLength);
    }
    SetupCloseInfFile(hTxtsetupSif);

    return TRUE;
}

LONG LoadGenentry(HINF hinf,PCTSTR name,PGENENTRY *gen,PINFCONTEXT context)
{
    LONG TotalCount;
    DWORD LineLength;

    TotalCount = SetupGetLineCount(hinf, name);
    if (TotalCount > 0)
    {
        *gen = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(GENENTRY) * TotalCount);
        if (*gen != NULL)
        {
            if (SetupFindFirstLine(hinf, name, NULL, context))
            {
                LONG Count = 0;
                do
                {
                    SetupGetStringField(context,
                                        0,
                                        (*gen)[Count].Id,
                                        sizeof((*gen)[Count].Id) / sizeof(TCHAR),
                                        &LineLength);

                    SetupGetStringField(context,
                                        1,
                                        (*gen)[Count].Value,
                                        sizeof((*gen)[Count].Value) / sizeof(TCHAR),
                                        &LineLength);
                    ++Count;
                }
                while (SetupFindNextLine(context, context) && Count < TotalCount);
            }
        }
        else return 0;
    }
    return TotalCount;
}

BOOL isUnattendSetup(VOID)
{
    WCHAR szPath[MAX_PATH];
    WCHAR *ch;
    HINF hUnattendedInf;
    INFCONTEXT InfContext;
    TCHAR szValue[MAX_PATH];
    DWORD LineLength;
    //HKEY hKey;
    BOOL result = 0;

    GetModuleFileNameW(NULL, szPath, MAX_PATH);
    ch = strrchrW(szPath, L'\\');
    if (ch != NULL)
        *ch = L'\0';

    wcscat(szPath, L"\\unattend.inf");
    hUnattendedInf = SetupOpenInfFileW(szPath, NULL, INF_STYLE_OLDNT, NULL);

    if (hUnattendedInf != INVALID_HANDLE_VALUE)
    {
        if (SetupFindFirstLine(hUnattendedInf, _T("Unattend"), _T("UnattendSetupEnabled"),&InfContext))
        {
            if (SetupGetStringField(&InfContext,
                                    1,
                                    szValue,
                                    sizeof(szValue) / sizeof(TCHAR),
                                    &LineLength) && (_tcsicmp(szValue, _T("yes")) == 0))
            {
                result = 1; // unattendSetup enabled
                // read values and store in SetupData
            }
        }
            SetupCloseInfFile(hUnattendedInf);
    }

    return result;
}

#if 0
static
VOID
EnableShutdownPrivilege(VOID)
{
    HANDLE hToken = NULL;
    TOKEN_PRIVILEGES Privileges;

    /* Get shutdown privilege */
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken))
    {
//        FatalError("OpenProcessToken() failed!");
        return;
    }

    if (!LookupPrivilegeValue(NULL,
                              SE_SHUTDOWN_NAME,
                              &Privileges.Privileges[0].Luid))
    {
//        FatalError("LookupPrivilegeValue() failed!");
       goto done;
    }

    Privileges.PrivilegeCount = 1;
    Privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if (AdjustTokenPrivileges(hToken,
                              FALSE,
                              &Privileges,
                              0,
                              (PTOKEN_PRIVILEGES)NULL,
                              NULL) == 0)
    {
//        FatalError("AdjustTokenPrivileges() failed!");
       goto done;
    }

done:
    if (hToken != NULL)
        CloseHandle(hToken);

    return;
}
#endif

int WINAPI
_tWinMain(HINSTANCE hInst,
          HINSTANCE hPrevInstance,
          LPTSTR lpszCmdLine,
          int nCmdShow)
{
    PSETUPDATA pSetupData = NULL;
    PROPSHEETHEADER psh;
    HPROPSHEETPAGE ahpsp[8];
    PROPSHEETPAGE psp = {0};
    UINT nPages = 0;

    pSetupData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SETUPDATA));
    if (pSetupData == NULL)
    {
        return 1;
    }

    pSetupData->hInstance = hInst;
    pSetupData->bUnattend = isUnattendSetup();

    LoadString(hInst,IDS_ABORTSETUP, pSetupData->szAbortMessage, sizeof(pSetupData->szAbortMessage)/sizeof(TCHAR));
    LoadString(hInst,IDS_ABORTSETUP2, pSetupData->szAbortTitle, sizeof(pSetupData->szAbortTitle)/sizeof(TCHAR));

    /* Create title font */
    pSetupData->hTitleFont = CreateTitleFont();

    if (!pSetupData->bUnattend)
    {
        if (!LoadSetupData(pSetupData))
            return 0;

        /* Create the Start page, until setup is working */
        psp.dwSize = sizeof(PROPSHEETPAGE);
        psp.dwFlags = PSP_DEFAULT | PSP_HIDEHEADER;
        psp.hInstance = hInst;
        psp.lParam = (LPARAM)pSetupData;
        psp.pfnDlgProc = StartDlgProc;
        psp.pszTemplate = MAKEINTRESOURCE(IDD_STARTPAGE);
        ahpsp[nPages++] = CreatePropertySheetPage(&psp);

        /* Create install type selection page */
        psp.dwSize = sizeof(PROPSHEETPAGE);
        psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
        psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_TYPETITLE);
        psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_TYPESUBTITLE);
        psp.hInstance = hInst;
        psp.lParam = (LPARAM)pSetupData;
        psp.pfnDlgProc = TypeDlgProc;
        psp.pszTemplate = MAKEINTRESOURCE(IDD_TYPEPAGE);
        ahpsp[nPages++] = CreatePropertySheetPage(&psp);

        /* Create device settings page */
        psp.dwSize = sizeof(PROPSHEETPAGE);
        psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
        psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_DEVICETITLE);
        psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_DEVICESUBTITLE);
        psp.hInstance = hInst;
        psp.lParam = (LPARAM)pSetupData;
        psp.pfnDlgProc = DeviceDlgProc;
        psp.pszTemplate = MAKEINTRESOURCE(IDD_DEVICEPAGE);
        ahpsp[nPages++] = CreatePropertySheetPage(&psp);

        /* Create install device settings page / boot method / install directory */
        psp.dwSize = sizeof(PROPSHEETPAGE);
        psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
        psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_DRIVETITLE);
        psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_DRIVESUBTITLE);
        psp.hInstance = hInst;
        psp.lParam = (LPARAM)pSetupData;
        psp.pfnDlgProc = DriveDlgProc;
        psp.pszTemplate = MAKEINTRESOURCE(IDD_DRIVEPAGE);
        ahpsp[nPages++] = CreatePropertySheetPage(&psp);

        /* Create summary page */
        psp.dwSize = sizeof(PROPSHEETPAGE);
        psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
        psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_SUMMARYTITLE);
        psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_SUMMARYSUBTITLE);
        psp.hInstance = hInst;
        psp.lParam = (LPARAM)pSetupData;
        psp.pfnDlgProc = SummaryDlgProc;
        psp.pszTemplate = MAKEINTRESOURCE(IDD_SUMMARYPAGE);
        ahpsp[nPages++] = CreatePropertySheetPage(&psp);
    }

    /* Create installation progress page */
    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
    psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_PROCESSTITLE);
    psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_PROCESSSUBTITLE);
    psp.hInstance = hInst;
    psp.lParam = (LPARAM)pSetupData;
    psp.pfnDlgProc = ProcessDlgProc;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_PROCESSPAGE);
    ahpsp[nPages++] = CreatePropertySheetPage(&psp);

    /* Create finish to reboot page */
    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = PSP_DEFAULT | PSP_HIDEHEADER;
    psp.hInstance = hInst;
    psp.lParam = (LPARAM)pSetupData;
    psp.pfnDlgProc = RestartDlgProc;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_RESTARTPAGE);
    ahpsp[nPages++] = CreatePropertySheetPage(&psp);

    /* Create the property sheet */
    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_WIZARD97 | PSH_WATERMARK | PSH_HEADER;
    psh.hInstance = hInst;
    psh.hwndParent = NULL;
    psh.nPages = nPages;
    psh.nStartPage = 0;
    psh.phpage = ahpsp;
    psh.pszbmWatermark = MAKEINTRESOURCE(IDB_WATERMARK);
    psh.pszbmHeader = MAKEINTRESOURCE(IDB_HEADER);

    /* Display the wizard */
    PropertySheet(&psh);

    if (pSetupData->hTitleFont)
        DeleteObject(pSetupData->hTitleFont);

    if (pSetupData != NULL)
        HeapFree(GetProcessHeap(), 0, pSetupData);

#if 0
    EnableShutdownPrivilege();
    ExitWindowsEx(EWX_REBOOT, 0);
#endif

    return 0;
}

/* EOF */
