/*
 *  Notepad
 *
 *  Copyright 2000 Mike McCormack <Mike_McCormack@looksmart.com.au>
 *  Copyright 1997,98 Marcel Baur <mbaur@g26.ethz.ch>
 *  Copyright 2002 Sylvain Petreolle <spetreolle@yahoo.fr>
 *  Copyright 2002 Andriy Palamarchuk
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
 *
 */

#define UNICODE
#define _UNICODE

#include <windows.h>
#include <stdio.h>
#include <tchar.h>

#include "main.h"
#include "dialog.h"
#include "notepad_res.h"

NOTEPAD_GLOBALS Globals;
static ATOM aFINDMSGSTRING;

/***********************************************************************
 *
 *           SetFileName
 *
 *  Sets Global File Name.
 */
VOID SetFileName(LPCWSTR szFileName)
{
    lstrcpy(Globals.szFileName, szFileName);
    Globals.szFileTitle[0] = 0;
    GetFileTitle(szFileName, Globals.szFileTitle, sizeof(Globals.szFileTitle));
}

/***********************************************************************
 *
 *           NOTEPAD_MenuCommand
 *
 *  All handling of main menu events
 */
static int NOTEPAD_MenuCommand(WPARAM wParam)
{
    switch (wParam)
    {
    case CMD_NEW:               DIALOG_FileNew(); break;
    case CMD_OPEN:              DIALOG_FileOpen(); break;
    case CMD_SAVE:              DIALOG_FileSave(); break;
    case CMD_SAVE_AS:           DIALOG_FileSaveAs(); break;
    case CMD_PRINT:             DIALOG_FilePrint(); break;
    case CMD_PAGE_SETUP:        DIALOG_FilePageSetup(); break;
    case CMD_PRINTER_SETUP:     DIALOG_FilePrinterSetup();break;
    case CMD_EXIT:              DIALOG_FileExit(); break;

    case CMD_UNDO:             DIALOG_EditUndo(); break;
    case CMD_CUT:              DIALOG_EditCut(); break;
    case CMD_COPY:             DIALOG_EditCopy(); break;
    case CMD_PASTE:            DIALOG_EditPaste(); break;
    case CMD_DELETE:           DIALOG_EditDelete(); break;
    case CMD_SELECT_ALL:       DIALOG_EditSelectAll(); break;
    case CMD_TIME_DATE:        DIALOG_EditTimeDate();break;

    case CMD_SEARCH:           DIALOG_Search(); break;
    case CMD_SEARCH_NEXT:      DIALOG_SearchNext(); break;

    case CMD_WRAP:             DIALOG_EditWrap(); break;
    case CMD_FONT:             DIALOG_SelectFont(); break;

    case CMD_HELP_CONTENTS:    DIALOG_HelpContents(); break;
    case CMD_HELP_SEARCH:      DIALOG_HelpSearch(); break;
    case CMD_HELP_ON_HELP:     DIALOG_HelpHelp(); break;
    case CMD_LICENSE:          DIALOG_HelpLicense(); break;
    case CMD_NO_WARRANTY:      DIALOG_HelpNoWarranty(); break;
    case CMD_ABOUT_WINE:       DIALOG_HelpAboutWine(); break;

    default:
	break;
    }
   return 0;
}

/***********************************************************************
 *
 *           NOTEPAD_FindNext
 */

static VOID NOTEPAD_FindNext(FINDREPLACE *pFindReplace)
{
    int iTextLength, iTargetLength;
    LPTSTR pszText = NULL;
    DWORD dwPosition, dwDummy;
    BOOL bMatches = FALSE;

    iTargetLength = _tcslen(pFindReplace->lpstrFindWhat);

    iTextLength = GetWindowTextLength(Globals.hEdit);

    if (iTextLength > 0)
    {
        pszText = (LPTSTR) HeapAlloc(GetProcessHeap(), 0, (iTextLength + 1) * sizeof(TCHAR));
        if (!pszText)
            return;

        GetWindowText(Globals.hEdit, pszText, iTextLength + 1);
    }

    SendMessage(Globals.hEdit, EM_GETSEL, (WPARAM) &dwDummy, (LPARAM) &dwPosition);

    while(dwPosition < iTextLength)
    {
        /* Make proper comparison */
        if (pFindReplace->Flags & FR_MATCHCASE)
            bMatches = !_tcsncmp(&pszText[dwPosition], pFindReplace->lpstrFindWhat, iTargetLength);
        else
            bMatches = !_tcsnicmp(&pszText[dwPosition], pFindReplace->lpstrFindWhat, iTargetLength);

        if (bMatches && pFindReplace->Flags & FR_WHOLEWORD)
        {
            if ((dwPosition > 0) && !_istspace(pszText[dwPosition-1]))
                bMatches = FALSE;
            if ((dwPosition < iTextLength - 1) && !_istspace(pszText[dwPosition+1]))
                bMatches = FALSE;
        }

        if (bMatches)
            break;

        if (pFindReplace->Flags & FR_DOWN) 
            dwPosition++;
        else
            dwPosition--;
    }

    if (bMatches)
    {
        SendMessage(Globals.hEdit, EM_SETSEL, dwPosition, dwPosition + iTargetLength);
        SendMessage(Globals.hEdit, EM_SCROLLCARET, 0, 0);
    }

    if (pszText)
        HeapFree(GetProcessHeap(), 0, pszText);
}

/***********************************************************************
 *
 *           NOTEPAD_FindTerm
 */

static VOID NOTEPAD_FindTerm(VOID)
{
    Globals.hFindReplaceDlg = NULL;
}

/***********************************************************************
 * Data Initialization
 */
static VOID NOTEPAD_InitData(VOID)
{
    LPWSTR p = Globals.szFilter;
    static const WCHAR txt_files[] = { '*','.','t','x','t',0 };
    static const WCHAR all_files[] = { '*','.','*',0 };

    LoadString(Globals.hInstance, STRING_TEXT_FILES_TXT, p, MAX_STRING_LEN);
    p += lstrlen(p) + 1;
    lstrcpy(p, txt_files);
    p += lstrlen(p) + 1;
    LoadString(Globals.hInstance, STRING_ALL_FILES, p, MAX_STRING_LEN);
    p += lstrlen(p) + 1;
    lstrcpy(p, all_files);
    p += lstrlen(p) + 1;
    *p = '\0';
}

/***********************************************************************
 * Enable/disable items on the menu based on control state
 */
static VOID NOTEPAD_InitMenuPopup(HMENU menu, int index)
{
    int enable;

    EnableMenuItem(menu, CMD_UNDO,
        SendMessage(Globals.hEdit, EM_CANUNDO, 0, 0) ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(menu, CMD_PASTE,
        IsClipboardFormatAvailable(CF_TEXT) ? MF_ENABLED : MF_GRAYED);
    enable = SendMessage(Globals.hEdit, EM_GETSEL, 0, 0);
    enable = (HIWORD(enable) == LOWORD(enable)) ? MF_GRAYED : MF_ENABLED;
    EnableMenuItem(menu, CMD_CUT, enable);
    EnableMenuItem(menu, CMD_COPY, enable);
    EnableMenuItem(menu, CMD_DELETE, enable);
    
    EnableMenuItem(menu, CMD_SELECT_ALL,
        GetWindowTextLength(Globals.hEdit) ? MF_ENABLED : MF_GRAYED);
}

/***********************************************************************
 *
 *           NOTEPAD_WndProc
 */
static LRESULT WINAPI NOTEPAD_WndProc(HWND hWnd, UINT msg, WPARAM wParam,
                               LPARAM lParam)
{
    switch (msg) {

    case WM_CREATE:
    {
        static const WCHAR editW[] = { 'e','d','i','t',0 };
        RECT rc;
        GetClientRect(hWnd, &rc);
        Globals.hEdit = CreateWindowEx(WS_EX_CLIENTEDGE, editW, NULL,
                             WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | WS_HSCROLL |
                             ES_AUTOVSCROLL | ES_MULTILINE | ES_NOHIDESEL,
                             0, 0, rc.right, rc.bottom, hWnd,
                             NULL, Globals.hInstance, NULL);
        break;
    }

    case WM_COMMAND:
        NOTEPAD_MenuCommand(LOWORD(wParam));
        break;

    case WM_DESTROYCLIPBOARD:
        /*MessageBox(Globals.hMainWnd, "Empty clipboard", "Debug", MB_ICONEXCLAMATION);*/
        break;

    case WM_CLOSE:
        if (DoCloseFile()) {
            DestroyWindow(hWnd);
        }
        break;

    case WM_QUERYENDSESSION:
        if (DoCloseFile()) {
            return 1;
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    case WM_SIZE:
        SetWindowPos(Globals.hEdit, NULL, 0, 0, LOWORD(lParam), HIWORD(lParam),
                     SWP_NOOWNERZORDER | SWP_NOZORDER);
        break;

    case WM_SETFOCUS:
        SetFocus(Globals.hEdit);
        break;

    case WM_DROPFILES:
    {
        WCHAR szFileName[MAX_PATH];
        HANDLE hDrop = (HANDLE) wParam;

        DragQueryFile(hDrop, 0, szFileName, SIZEOF(szFileName));
        DragFinish(hDrop);
        DoOpenFile(szFileName);
        break;
    }
    
    case WM_INITMENUPOPUP:
        NOTEPAD_InitMenuPopup((HMENU)wParam, lParam);
        break;

    default:
        if (msg == aFINDMSGSTRING)
        {
            FINDREPLACE *pFindReplace = (FINDREPLACE *) lParam;

            if (pFindReplace->Flags & FR_FINDNEXT)
                NOTEPAD_FindNext(pFindReplace);
            else if (pFindReplace->Flags & FR_DIALOGTERM)
                NOTEPAD_FindTerm();
            break;
        }

        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}

static int AlertFileDoesNotExist(LPCWSTR szFileName)
{
   int nResult;
   WCHAR szMessage[MAX_STRING_LEN];
   WCHAR szResource[MAX_STRING_LEN];

   LoadString(Globals.hInstance, STRING_DOESNOTEXIST, szResource, SIZEOF(szResource));
   wsprintf(szMessage, szResource, szFileName);

   LoadString(Globals.hInstance, STRING_NOTEPAD, szResource, SIZEOF(szResource));

   nResult = MessageBox(Globals.hMainWnd, szMessage, szResource,
                        MB_ICONEXCLAMATION | MB_YESNO);

   return(nResult);
}

static void HandleCommandLine(LPWSTR cmdline)
{
    WCHAR delimiter;
    int opt_print=0;

    /* skip white space */
    while (*cmdline == ' ') cmdline++;

    /* skip executable name */
    delimiter = (*cmdline == '"' ? '"' : ' ');

    do
    {
        cmdline++;
    }
    while (*cmdline && *cmdline != delimiter);
    if (*cmdline == delimiter) cmdline++;

    while (*cmdline == ' ' || *cmdline == '-' || *cmdline == '/')
    {
        WCHAR option;

        if (*cmdline++ == ' ') continue;

        option = *cmdline;
        if (option) cmdline++;
        while (*cmdline == ' ') cmdline++;

        switch(option)
        {
            case 'p':
            case 'P':
                opt_print=1;
                break;
        }
    }

    if (*cmdline)
    {
        /* file name is passed in the command line */
        LPCWSTR file_name;
        BOOL file_exists;
        WCHAR buf[MAX_PATH];

        if (cmdline[0] == '"')
        {
            cmdline++;
            cmdline[lstrlen(cmdline) - 1] = 0;
        }

        if (FileExists(cmdline))
        {
            file_exists = TRUE;
            file_name = cmdline;
        }
        else
        {
            static const WCHAR txtW[] = { '.','t','x','t',0 };

            /* try to find file with ".txt" extension */
            if (!lstrcmp(txtW, cmdline + lstrlen(cmdline) - lstrlen(txtW)))
            {
                file_exists = FALSE;
                file_name = cmdline;
            }
            else
            {
                lstrcpyn(buf, cmdline, MAX_PATH - lstrlen(txtW) - 1);
                lstrcat(buf, txtW);
                file_name = buf;
                file_exists = FileExists(buf);
            }
        }

        if (file_exists)
        {
            DoOpenFile(file_name);
            InvalidateRect(Globals.hMainWnd, NULL, FALSE);
            if (opt_print)
                DIALOG_FilePrint();
        }
        else
        {
            switch (AlertFileDoesNotExist(file_name)) {
            case IDYES:
                DoOpenFile(file_name);
                break;

            case IDNO:
                break;
            }
        }
     }
}

/***********************************************************************
 *
 *           WinMain
 */
int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE prev, LPSTR cmdline, int show)
{
    MSG        msg;
    HACCEL      hAccel;
    WNDCLASSEX class;
    static const WCHAR className[] = {'N','P','C','l','a','s','s',0};
    static const WCHAR winName[]   = {'N','o','t','e','p','a','d',0};

    aFINDMSGSTRING = RegisterWindowMessage(FINDMSGSTRING);

    ZeroMemory(&Globals, sizeof(Globals));
    Globals.hInstance       = hInstance;

    ZeroMemory(&class, sizeof(class));
    class.cbSize        = sizeof(class);
    class.lpfnWndProc   = NOTEPAD_WndProc;
    class.hInstance     = Globals.hInstance;
    class.hIcon         = LoadIcon(0, IDI_APPLICATION);
    class.hCursor       = LoadCursor(0, IDC_ARROW);
    class.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    class.lpszMenuName  = MAKEINTRESOURCE(MAIN_MENU);
    class.lpszClassName = className;

    if (!RegisterClassEx(&class)) return FALSE;

    /* Setup windows */

    Globals.hMainWnd =
        CreateWindow(className, winName, WS_OVERLAPPEDWINDOW,
                     CW_USEDEFAULT, 0, CW_USEDEFAULT, 0,
                     NULL, NULL, Globals.hInstance, NULL);
    if (!Globals.hMainWnd)
    {
        ShowLastError();
        ExitProcess(1);
    }

    NOTEPAD_InitData();
    DIALOG_FileNew();

    ShowWindow(Globals.hMainWnd, show);
    UpdateWindow(Globals.hMainWnd);
    DragAcceptFiles(Globals.hMainWnd, TRUE);

    HandleCommandLine(GetCommandLine());

    hAccel = LoadAccelerators( hInstance, MAKEINTRESOURCE(ID_ACCEL) );

    while (GetMessage(&msg, 0, 0, 0))
    {
        if (!IsDialogMessage(Globals.hFindReplaceDlg, &msg) &&
            !TranslateAccelerator(Globals.hMainWnd, hAccel, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return msg.wParam;
}
