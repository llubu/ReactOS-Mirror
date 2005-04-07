/*
 *  ReactOS Task Manager
 *
 * TaskMgr.c : Defines the entry point for the application.
 *
 *  Copyright (C) 1999 - 2001  Brian Palmer  <brianp@reactos.org>
 *                2005         Klemens Friedl <frik85@reactos.at>
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

#include "precomp.h"

#define STATUS_WINDOW   2001

/* Global Variables: */
HINSTANCE hInst;                 /* current instance */

HWND hMainWnd;                   /* Main Window */
HWND hStatusWnd;                 /* Status Bar Window */
HWND hTabWnd;                    /* Tab Control Window */

int  nMinimumWidth;              /* Minimum width of the dialog (OnSize()'s cx) */
int  nMinimumHeight;             /* Minimum height of the dialog (OnSize()'s cy) */

int  nOldWidth;                  /* Holds the previous client area width */
int  nOldHeight;                 /* Holds the previous client area height */

BOOL bInMenuLoop = FALSE;        /* Tells us if we are in the menu loop */

TASKMANAGER_SETTINGS TaskManagerSettings;


int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
    HANDLE hProcess;
    HANDLE hToken; 
    TOKEN_PRIVILEGES tkp; 

    /* Initialize global variables */
    hInst = hInstance;

    /* Change our priority class to HIGH */
    hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, GetCurrentProcessId());
    SetPriorityClass(hProcess, HIGH_PRIORITY_CLASS);
    CloseHandle(hProcess);

    /* Now lets get the SE_DEBUG_NAME privilege
     * so that we can debug processes 
     */

    /* Get a token for this process.  */
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
        /* Get the LUID for the debug privilege.  */
        LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &tkp.Privileges[0].Luid); 

        tkp.PrivilegeCount = 1;  /* one privilege to set */
        tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED; 

        /* Get the debug privilege for this process. */
        AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0); 
    }

    /* Load our settings from the registry */
    LoadSettings();

    /* Initialize perf data */
    if (!PerfDataInitialize()) {
        return -1;
    }

    DialogBox(hInst, (LPCTSTR)IDD_TASKMGR_DIALOG, NULL, TaskManagerWndProc);
 
    /* Save our settings to the registry */
    SaveSettings();
    PerfDataUninitialize();
    return 0;
}

/* Message handler for dialog box. */
INT_PTR CALLBACK
TaskManagerWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    HDC             hdc;
    PAINTSTRUCT     ps;
    LPRECT          pRC;
    RECT            rc;
    int             idctrl;
    LPNMHDR         pnmh;
    WINDOWPLACEMENT wp;

    switch (message) {
    case WM_INITDIALOG:
        hMainWnd = hDlg;
        return OnCreate(hDlg);

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, LOWORD(wParam));
            return TRUE;
        }
        /* Process menu commands */
        switch (LOWORD(wParam))
        {
        case ID_FILE_NEW:
            TaskManager_OnFileNew();
            break;
        case ID_OPTIONS_ALWAYSONTOP:
            TaskManager_OnOptionsAlwaysOnTop();
            break;
        case ID_OPTIONS_MINIMIZEONUSE:
            TaskManager_OnOptionsMinimizeOnUse();
            break;
        case ID_OPTIONS_HIDEWHENMINIMIZED:
            TaskManager_OnOptionsHideWhenMinimized();
            break;
        case ID_OPTIONS_SHOW16BITTASKS:
            TaskManager_OnOptionsShow16BitTasks();
            break;
        case ID_RESTORE:
            TaskManager_OnRestoreMainWindow();
            break;
        case ID_VIEW_LARGE:
            ApplicationPage_OnViewLargeIcons();
            break;
        case ID_VIEW_SMALL:
            ApplicationPage_OnViewSmallIcons();
            break;
        case ID_VIEW_DETAILS:
            ApplicationPage_OnViewDetails();
            break;
        case ID_VIEW_SHOWKERNELTIMES:
            PerformancePage_OnViewShowKernelTimes();
            break;
        case ID_VIEW_CPUHISTORY_ONEGRAPHALL:
            PerformancePage_OnViewCPUHistoryOneGraphAll();
            break;
        case ID_VIEW_CPUHISTORY_ONEGRAPHPERCPU:
            PerformancePage_OnViewCPUHistoryOneGraphPerCPU();
            break;
        case ID_VIEW_UPDATESPEED_HIGH:
            TaskManager_OnViewUpdateSpeedHigh();
            break;
        case ID_VIEW_UPDATESPEED_NORMAL:
            TaskManager_OnViewUpdateSpeedNormal();
            break;
        case ID_VIEW_UPDATESPEED_LOW:
            TaskManager_OnViewUpdateSpeedLow();
            break;
        case ID_VIEW_UPDATESPEED_PAUSED:
            TaskManager_OnViewUpdateSpeedPaused();
            break;
        case ID_VIEW_SELECTCOLUMNS:
            ProcessPage_OnViewSelectColumns();
            break;
        case ID_VIEW_REFRESH:
            PostMessage(hDlg, WM_TIMER, 0, 0);
            break;
        case ID_WINDOWS_TILEHORIZONTALLY:
            ApplicationPage_OnWindowsTileHorizontally();
            break;
        case ID_WINDOWS_TILEVERTICALLY:
            ApplicationPage_OnWindowsTileVertically();
            break;
        case ID_WINDOWS_MINIMIZE:
            ApplicationPage_OnWindowsMinimize();
            break;
        case ID_WINDOWS_MAXIMIZE:
            ApplicationPage_OnWindowsMaximize();
            break;
        case ID_WINDOWS_CASCADE:
            ApplicationPage_OnWindowsCascade();
            break;
        case ID_WINDOWS_BRINGTOFRONT:
            ApplicationPage_OnWindowsBringToFront();
            break;
        case ID_APPLICATION_PAGE_SWITCHTO:
            ApplicationPage_OnSwitchTo();
            break;
        case ID_APPLICATION_PAGE_ENDTASK:
            ApplicationPage_OnEndTask();
            break;
        case ID_APPLICATION_PAGE_GOTOPROCESS:
            ApplicationPage_OnGotoProcess();
            break;
        case ID_PROCESS_PAGE_ENDPROCESS:
            ProcessPage_OnEndProcess();
            break;
        case ID_PROCESS_PAGE_ENDPROCESSTREE:
            ProcessPage_OnEndProcessTree();
            break;
        case ID_PROCESS_PAGE_DEBUG:
            ProcessPage_OnDebug();
            break;
        case ID_PROCESS_PAGE_SETAFFINITY:
            ProcessPage_OnSetAffinity();
            break;
        case ID_PROCESS_PAGE_SETPRIORITY_REALTIME:
            ProcessPage_OnSetPriorityRealTime();
            break;
        case ID_PROCESS_PAGE_SETPRIORITY_HIGH:
            ProcessPage_OnSetPriorityHigh();
            break;
        case ID_PROCESS_PAGE_SETPRIORITY_ABOVENORMAL:
            ProcessPage_OnSetPriorityAboveNormal();
            break;
        case ID_PROCESS_PAGE_SETPRIORITY_NORMAL:
            ProcessPage_OnSetPriorityNormal();
            break;
        case ID_PROCESS_PAGE_SETPRIORITY_BELOWNORMAL:
            ProcessPage_OnSetPriorityBelowNormal();
            break;
        case ID_PROCESS_PAGE_SETPRIORITY_LOW:
            ProcessPage_OnSetPriorityLow();
            break;
        case ID_PROCESS_PAGE_DEBUGCHANNELS:
            ProcessPage_OnDebugChannels();
            break;
        case ID_HELP_ABOUT:
            OnAbout();
            break;
        case ID_FILE_EXIT:
            EndDialog(hDlg, IDOK);
            break;
        }     
        break;

    case WM_ONTRAYICON:
        switch(lParam)
        {
        case WM_RBUTTONDOWN:
            {
            POINT pt;
            BOOL OnTop;
            HMENU hMenu, hPopupMenu;
            
            GetCursorPos(&pt);
            
            OnTop = ((GetWindowLong(hMainWnd, GWL_EXSTYLE) & WS_EX_TOPMOST) != 0);
            
            hMenu = LoadMenu(hInst, MAKEINTRESOURCE(IDR_TRAY_POPUP));
            hPopupMenu = GetSubMenu(hMenu, 0);
            
            if(IsWindowVisible(hMainWnd))
            {
              DeleteMenu(hPopupMenu, ID_RESTORE, MF_BYCOMMAND);
            }
            else
            {
              SetMenuDefaultItem(hPopupMenu, ID_RESTORE, FALSE);
            }
            
            if(OnTop)
            {
              CheckMenuItem(hPopupMenu, ID_OPTIONS_ALWAYSONTOP, MF_BYCOMMAND | MF_CHECKED);
            }
            
            SetForegroundWindow(hMainWnd);
            TrackPopupMenuEx(hPopupMenu, 0, pt.x, pt.y, hMainWnd, NULL);
            
            DestroyMenu(hMenu);
            break;
            }
        case WM_LBUTTONDBLCLK:
            TaskManager_OnRestoreMainWindow();
            break;
        }
        break;

    case WM_NOTIFY:
        idctrl = (int)wParam;
        pnmh = (LPNMHDR)lParam;
        if ((pnmh->hwndFrom == hTabWnd) &&
            (pnmh->idFrom == IDC_TAB) &&
            (pnmh->code == TCN_SELCHANGE))
        {
            TaskManager_OnTabWndSelChange();
        }
        break;

    case WM_NCPAINT:
        hdc = GetDC(hDlg);
        GetClientRect(hDlg, &rc);
        Draw3dRect(hdc, rc.left, rc.top, rc.right, rc.top + 2, GetSysColor(COLOR_3DSHADOW), GetSysColor(COLOR_3DHILIGHT));
        ReleaseDC(hDlg, hdc);
        break;

    case WM_PAINT:
        hdc = BeginPaint(hDlg, &ps);
        GetClientRect(hDlg, &rc);
        Draw3dRect(hdc, rc.left, rc.top, rc.right, rc.top + 2, GetSysColor(COLOR_3DSHADOW), GetSysColor(COLOR_3DHILIGHT));
        EndPaint(hDlg, &ps);
        break;

    case WM_SIZING:
        /* Make sure the user is sizing the dialog */
        /* in an acceptable range */
        pRC = (LPRECT)lParam;
        if ((wParam == WMSZ_LEFT) || (wParam == WMSZ_TOPLEFT) || (wParam == WMSZ_BOTTOMLEFT)) {
            /* If the width is too small enlarge it to the minimum */
            if (nMinimumWidth > (pRC->right - pRC->left))
                pRC->left = pRC->right - nMinimumWidth;
        } else {
            /* If the width is too small enlarge it to the minimum */
            if (nMinimumWidth > (pRC->right - pRC->left))
                pRC->right = pRC->left + nMinimumWidth;
        }
        if ((wParam == WMSZ_TOP) || (wParam == WMSZ_TOPLEFT) || (wParam == WMSZ_TOPRIGHT)) {
            /* If the height is too small enlarge it to the minimum */
            if (nMinimumHeight > (pRC->bottom - pRC->top))
                pRC->top = pRC->bottom - nMinimumHeight;
        } else {
            /* If the height is too small enlarge it to the minimum */
            if (nMinimumHeight > (pRC->bottom - pRC->top))
                pRC->bottom = pRC->top + nMinimumHeight;
        }
        return TRUE;
        break;

    case WM_SIZE:
        /* Handle the window sizing in it's own function */
        OnSize(wParam, LOWORD(lParam), HIWORD(lParam));
        break;

    case WM_MOVE:
        /* Handle the window moving in it's own function */
        OnMove(wParam, LOWORD(lParam), HIWORD(lParam));
        break;

    case WM_DESTROY:
        ShowWindow(hDlg, SW_HIDE);
        TrayIcon_ShellRemoveTrayIcon();
        wp.length = sizeof(WINDOWPLACEMENT);
        GetWindowPlacement(hDlg, &wp);
        TaskManagerSettings.Left = wp.rcNormalPosition.left;
        TaskManagerSettings.Top = wp.rcNormalPosition.top;
        TaskManagerSettings.Right = wp.rcNormalPosition.right;
        TaskManagerSettings.Bottom = wp.rcNormalPosition.bottom;
        if (IsZoomed(hDlg) || (wp.flags & WPF_RESTORETOMAXIMIZED))
            TaskManagerSettings.Maximized = TRUE;
        else
            TaskManagerSettings.Maximized = FALSE;
        return DefWindowProc(hDlg, message, wParam, lParam);
        
    case WM_TIMER:
        /* Refresh the performance data */
        PerfDataRefresh();
        RefreshApplicationPage();
        RefreshProcessPage();
        RefreshPerformancePage();
        TrayIcon_ShellUpdateTrayIcon();
        break;

    case WM_ENTERMENULOOP:
        TaskManager_OnEnterMenuLoop(hDlg);
        break;
    case WM_EXITMENULOOP:
        TaskManager_OnExitMenuLoop(hDlg);
        break;
    case WM_MENUSELECT:
        TaskManager_OnMenuSelect(hDlg, LOWORD(wParam), HIWORD(wParam), (HMENU)lParam);
        break;
    }

    return 0;
}

void FillSolidRect(HDC hDC, LPCRECT lpRect, COLORREF clr)
{
    SetBkColor(hDC, clr);
    ExtTextOut(hDC, 0, 0, ETO_OPAQUE, lpRect, NULL, 0, NULL);
}

void FillSolidRect2(HDC hDC, int x, int y, int cx, int cy, COLORREF clr)
{
    RECT rect;

    SetBkColor(hDC, clr);
    rect.left = x;
    rect.top = y;
    rect.right = x + cx;
    rect.bottom = y + cy;
    ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &rect, NULL, 0, NULL);
}

void Draw3dRect(HDC hDC, int x, int y, int cx, int cy, COLORREF clrTopLeft, COLORREF clrBottomRight)
{
    FillSolidRect2(hDC, x, y, cx - 1, 1, clrTopLeft);
    FillSolidRect2(hDC, x, y, 1, cy - 1, clrTopLeft);
    FillSolidRect2(hDC, x + cx, y, -1, cy, clrBottomRight);
    FillSolidRect2(hDC, x, y + cy, cx, -1, clrBottomRight);
}

void Draw3dRect2(HDC hDC, LPRECT lpRect, COLORREF clrTopLeft, COLORREF clrBottomRight)
{
    Draw3dRect(hDC, lpRect->left, lpRect->top, lpRect->right - lpRect->left,
        lpRect->bottom - lpRect->top, clrTopLeft, clrBottomRight);
}

BOOL OnCreate(HWND hWnd)
{
    HMENU   hMenu;
    HMENU   hEditMenu;
    HMENU   hViewMenu;
    HMENU   hUpdateSpeedMenu;
    HMENU   hCPUHistoryMenu;
    int     nActivePage;
    int     nParts[3];
    RECT    rc;
    TCHAR   szTemp[256];
    TCITEM  item;

    SendMessage(hMainWnd, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(hInst, MAKEINTRESOURCE(IDI_TASKMANAGER)));

    /* Initialize the Windows Common Controls DLL */
    InitCommonControls();

    /* Get the minimum window sizes */
    GetWindowRect(hWnd, &rc);
    nMinimumWidth = (rc.right - rc.left);
    nMinimumHeight = (rc.bottom - rc.top);

    /* Create the status bar */
    hStatusWnd = CreateStatusWindow(WS_VISIBLE|WS_CHILD|WS_CLIPSIBLINGS|SBT_NOBORDERS, _T(""), hWnd, STATUS_WINDOW);
    if(!hStatusWnd)
        return FALSE;

    /* Create the status bar panes */
    nParts[0] = 100;
    nParts[1] = 210;
    nParts[2] = 400;
    SendMessage(hStatusWnd, SB_SETPARTS, 3, (long)nParts);

    /* Create tab pages */
    hTabWnd = GetDlgItem(hWnd, IDC_TAB);
#if 1
    hApplicationPage = CreateDialog(hInst, MAKEINTRESOURCE(IDD_APPLICATION_PAGE), hWnd, ApplicationPageWndProc);
    hProcessPage = CreateDialog(hInst, MAKEINTRESOURCE(IDD_PROCESS_PAGE), hWnd, ProcessPageWndProc);
    hPerformancePage = CreateDialog(hInst, MAKEINTRESOURCE(IDD_PERFORMANCE_PAGE), hWnd, PerformancePageWndProc);
#else
    hApplicationPage = CreateDialog(hInst, MAKEINTRESOURCE(IDD_APPLICATION_PAGE), hTabWnd, ApplicationPageWndProc);
    hProcessPage = CreateDialog(hInst, MAKEINTRESOURCE(IDD_PROCESS_PAGE), hTabWnd, ProcessPageWndProc);
    hPerformancePage = CreateDialog(hInst, MAKEINTRESOURCE(IDD_PERFORMANCE_PAGE), hTabWnd, PerformancePageWndProc);
#endif

    /* Insert tabs */
    LoadString(hInst, IDS_TAB_APPS, szTemp, 256);
    memset(&item, 0, sizeof(TCITEM));
    item.mask = TCIF_TEXT;
    item.pszText = szTemp;
    TabCtrl_InsertItem(hTabWnd, 0, &item);
    LoadString(hInst, IDS_TAB_PROCESSES, szTemp, 256);
    memset(&item, 0, sizeof(TCITEM));
    item.mask = TCIF_TEXT;
    item.pszText = szTemp;
    TabCtrl_InsertItem(hTabWnd, 1, &item);
    LoadString(hInst, IDS_TAB_PERFORMANCE, szTemp, 256);
    memset(&item, 0, sizeof(TCITEM));
    item.mask = TCIF_TEXT;
    item.pszText = szTemp;
    TabCtrl_InsertItem(hTabWnd, 2, &item);

    /* Size everything correctly */
    GetClientRect(hWnd, &rc);
    nOldWidth = rc.right;
    nOldHeight = rc.bottom;
    /* nOldStartX = rc.left; */
    /*nOldStartY = rc.top;  */

#define PAGE_OFFSET_LEFT    17
#define PAGE_OFFSET_TOP     72
#define PAGE_OFFSET_WIDTH   (PAGE_OFFSET_LEFT*2)
#define PAGE_OFFSET_HEIGHT  (PAGE_OFFSET_TOP+32)

    if ((TaskManagerSettings.Left != 0) ||
        (TaskManagerSettings.Top != 0) ||
        (TaskManagerSettings.Right != 0) ||
        (TaskManagerSettings.Bottom != 0))
    {
        MoveWindow(hWnd, TaskManagerSettings.Left, TaskManagerSettings.Top, TaskManagerSettings.Right - TaskManagerSettings.Left, TaskManagerSettings.Bottom - TaskManagerSettings.Top, TRUE);
#ifdef __GNUC__TEST__
        MoveWindow(hApplicationPage, TaskManagerSettings.Left + PAGE_OFFSET_LEFT, TaskManagerSettings.Top + PAGE_OFFSET_TOP, TaskManagerSettings.Right - TaskManagerSettings.Left - PAGE_OFFSET_WIDTH, TaskManagerSettings.Bottom - TaskManagerSettings.Top - PAGE_OFFSET_HEIGHT, FALSE);
        MoveWindow(hProcessPage, TaskManagerSettings.Left + PAGE_OFFSET_LEFT, TaskManagerSettings.Top + PAGE_OFFSET_TOP, TaskManagerSettings.Right - TaskManagerSettings.Left - PAGE_OFFSET_WIDTH, TaskManagerSettings.Bottom - TaskManagerSettings.Top - PAGE_OFFSET_HEIGHT, FALSE);
        MoveWindow(hPerformancePage, TaskManagerSettings.Left + PAGE_OFFSET_LEFT, TaskManagerSettings.Top + PAGE_OFFSET_TOP, TaskManagerSettings.Right - TaskManagerSettings.Left - PAGE_OFFSET_WIDTH, TaskManagerSettings.Bottom - TaskManagerSettings.Top - PAGE_OFFSET_HEIGHT, FALSE);
#endif
    }
    if (TaskManagerSettings.Maximized)
        ShowWindow(hWnd, SW_MAXIMIZE);

    /* Set the always on top style */
    hMenu = GetMenu(hWnd);
    hEditMenu = GetSubMenu(hMenu, 1);
    hViewMenu = GetSubMenu(hMenu, 2);
    hUpdateSpeedMenu = GetSubMenu(hViewMenu, 1);
    hCPUHistoryMenu = GetSubMenu(hViewMenu, 7);

    /* Check or uncheck the always on top menu item */
    if (TaskManagerSettings.AlwaysOnTop) {
        CheckMenuItem(hEditMenu, ID_OPTIONS_ALWAYSONTOP, MF_BYCOMMAND|MF_CHECKED);
        SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
    } else {
        CheckMenuItem(hEditMenu, ID_OPTIONS_ALWAYSONTOP, MF_BYCOMMAND|MF_UNCHECKED);
        SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
    }

    /* Check or uncheck the minimize on use menu item */
    if (TaskManagerSettings.MinimizeOnUse)
        CheckMenuItem(hEditMenu, ID_OPTIONS_MINIMIZEONUSE, MF_BYCOMMAND|MF_CHECKED);
    else
        CheckMenuItem(hEditMenu, ID_OPTIONS_MINIMIZEONUSE, MF_BYCOMMAND|MF_UNCHECKED);

    /* Check or uncheck the hide when minimized menu item */
    if (TaskManagerSettings.HideWhenMinimized)
        CheckMenuItem(hEditMenu, ID_OPTIONS_HIDEWHENMINIMIZED, MF_BYCOMMAND|MF_CHECKED);
    else
        CheckMenuItem(hEditMenu, ID_OPTIONS_HIDEWHENMINIMIZED, MF_BYCOMMAND|MF_UNCHECKED);

    /* Check or uncheck the show 16-bit tasks menu item */
    if (TaskManagerSettings.Show16BitTasks)
        CheckMenuItem(hEditMenu, ID_OPTIONS_SHOW16BITTASKS, MF_BYCOMMAND|MF_CHECKED);
    else
        CheckMenuItem(hEditMenu, ID_OPTIONS_SHOW16BITTASKS, MF_BYCOMMAND|MF_UNCHECKED);

    if (TaskManagerSettings.View_LargeIcons)
        CheckMenuRadioItem(hViewMenu, ID_VIEW_LARGE, ID_VIEW_DETAILS, ID_VIEW_LARGE, MF_BYCOMMAND);
    else if (TaskManagerSettings.View_SmallIcons)
        CheckMenuRadioItem(hViewMenu, ID_VIEW_LARGE, ID_VIEW_DETAILS, ID_VIEW_SMALL, MF_BYCOMMAND);
    else
        CheckMenuRadioItem(hViewMenu, ID_VIEW_LARGE, ID_VIEW_DETAILS, ID_VIEW_DETAILS, MF_BYCOMMAND);

    if (TaskManagerSettings.ShowKernelTimes)
        CheckMenuItem(hViewMenu, ID_VIEW_SHOWKERNELTIMES, MF_BYCOMMAND|MF_CHECKED);
    else
        CheckMenuItem(hViewMenu, ID_VIEW_SHOWKERNELTIMES, MF_BYCOMMAND|MF_UNCHECKED);

    if (TaskManagerSettings.UpdateSpeed == 1)
        CheckMenuRadioItem(hUpdateSpeedMenu, ID_VIEW_UPDATESPEED_HIGH, ID_VIEW_UPDATESPEED_PAUSED, ID_VIEW_UPDATESPEED_HIGH, MF_BYCOMMAND);
    else if (TaskManagerSettings.UpdateSpeed == 2)
        CheckMenuRadioItem(hUpdateSpeedMenu, ID_VIEW_UPDATESPEED_HIGH, ID_VIEW_UPDATESPEED_PAUSED, ID_VIEW_UPDATESPEED_NORMAL, MF_BYCOMMAND);
    else if (TaskManagerSettings.UpdateSpeed == 4)
        CheckMenuRadioItem(hUpdateSpeedMenu, ID_VIEW_UPDATESPEED_HIGH, ID_VIEW_UPDATESPEED_PAUSED, ID_VIEW_UPDATESPEED_LOW, MF_BYCOMMAND);
    else
        CheckMenuRadioItem(hUpdateSpeedMenu, ID_VIEW_UPDATESPEED_HIGH, ID_VIEW_UPDATESPEED_PAUSED, ID_VIEW_UPDATESPEED_PAUSED, MF_BYCOMMAND);

    if (TaskManagerSettings.CPUHistory_OneGraphPerCPU)
        CheckMenuRadioItem(hCPUHistoryMenu, ID_VIEW_CPUHISTORY_ONEGRAPHALL, ID_VIEW_CPUHISTORY_ONEGRAPHPERCPU, ID_VIEW_CPUHISTORY_ONEGRAPHPERCPU, MF_BYCOMMAND);
    else
        CheckMenuRadioItem(hCPUHistoryMenu, ID_VIEW_CPUHISTORY_ONEGRAPHALL, ID_VIEW_CPUHISTORY_ONEGRAPHPERCPU, ID_VIEW_CPUHISTORY_ONEGRAPHALL, MF_BYCOMMAND);

    nActivePage = TaskManagerSettings.ActiveTabPage;
    TabCtrl_SetCurFocus/*Sel*/(hTabWnd, 0);
    TabCtrl_SetCurFocus/*Sel*/(hTabWnd, 1);
    TabCtrl_SetCurFocus/*Sel*/(hTabWnd, 2);
    TabCtrl_SetCurFocus/*Sel*/(hTabWnd, nActivePage);

    if (TaskManagerSettings.UpdateSpeed == 1)
        SetTimer(hWnd, 1, 1000, NULL);
    else if (TaskManagerSettings.UpdateSpeed == 2)
        SetTimer(hWnd, 1, 2000, NULL);
    else if (TaskManagerSettings.UpdateSpeed == 4)
        SetTimer(hWnd, 1, 4000, NULL);

    /*
     * Refresh the performance data
     * Sample it twice so we can establish
     * the delta values & cpu usage
     */
    PerfDataRefresh();
    PerfDataRefresh();

    RefreshApplicationPage();
    RefreshProcessPage();
    RefreshPerformancePage();

    TrayIcon_ShellAddTrayIcon();

    return TRUE;
}

/* OnMove()
 * This function handles all the moving events for the application
 * It moves every child window that needs moving
 */
void OnMove( UINT nType, int cx, int cy )
{
#ifdef __GNUC__TEST__
    MoveWindow(hApplicationPage, TaskManagerSettings.Left + PAGE_OFFSET_LEFT, TaskManagerSettings.Top + PAGE_OFFSET_TOP, TaskManagerSettings.Right - TaskManagerSettings.Left - PAGE_OFFSET_WIDTH, TaskManagerSettings.Bottom - TaskManagerSettings.Top - PAGE_OFFSET_HEIGHT, FALSE);
    MoveWindow(hProcessPage, TaskManagerSettings.Left + PAGE_OFFSET_LEFT, TaskManagerSettings.Top + PAGE_OFFSET_TOP, TaskManagerSettings.Right - TaskManagerSettings.Left - PAGE_OFFSET_WIDTH, TaskManagerSettings.Bottom - TaskManagerSettings.Top - PAGE_OFFSET_HEIGHT, FALSE);
    MoveWindow(hPerformancePage, TaskManagerSettings.Left + PAGE_OFFSET_LEFT, TaskManagerSettings.Top + PAGE_OFFSET_TOP, TaskManagerSettings.Right - TaskManagerSettings.Left - PAGE_OFFSET_WIDTH, TaskManagerSettings.Bottom - TaskManagerSettings.Top - PAGE_OFFSET_HEIGHT, FALSE);
#endif
}

/* OnSize()
 * This function handles all the sizing events for the application
 * It re-sizes every window, and child window that needs re-sizing
 */
void OnSize( UINT nType, int cx, int cy )
{
    int     nParts[3];
    int     nXDifference;
    int     nYDifference;
    RECT    rc;

    if (nType == SIZE_MINIMIZED)
    {
        if(TaskManagerSettings.HideWhenMinimized)
        {
          ShowWindow(hMainWnd, SW_HIDE);
        }
        return;
    }

    nXDifference = cx - nOldWidth;
    nYDifference = cy - nOldHeight;
    nOldWidth = cx;
    nOldHeight = cy;

    /* Update the status bar size */
    GetWindowRect(hStatusWnd, &rc);
    SendMessage(hStatusWnd, WM_SIZE, nType, MAKELPARAM(cx, cy + (rc.bottom - rc.top)));

    /* Update the status bar pane sizes */
    nParts[0] = bInMenuLoop ? -1 : 100;
    nParts[1] = 210;
    nParts[2] = cx;
    SendMessage(hStatusWnd, SB_SETPARTS, bInMenuLoop ? 1 : 3, (long)nParts);

    /* Resize the tab control */
    GetWindowRect(hTabWnd, &rc);
    cx = (rc.right - rc.left) + nXDifference;
    cy = (rc.bottom - rc.top) + nYDifference;
    SetWindowPos(hTabWnd, NULL, 0, 0, cx, cy, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOMOVE|SWP_NOZORDER);

    /* Resize the application page */
    GetWindowRect(hApplicationPage, &rc);
    cx = (rc.right - rc.left) + nXDifference;
    cy = (rc.bottom - rc.top) + nYDifference;
    SetWindowPos(hApplicationPage, NULL, 0, 0, cx, cy, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOMOVE|SWP_NOZORDER);
    
    /* Resize the process page */
    GetWindowRect(hProcessPage, &rc);
    cx = (rc.right - rc.left) + nXDifference;
    cy = (rc.bottom - rc.top) + nYDifference;
    SetWindowPos(hProcessPage, NULL, 0, 0, cx, cy, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOMOVE|SWP_NOZORDER);
    
    /* Resize the performance page */
    GetWindowRect(hPerformancePage, &rc);
    cx = (rc.right - rc.left) + nXDifference;
    cy = (rc.bottom - rc.top) + nYDifference;
    SetWindowPos(hPerformancePage, NULL, 0, 0, cx, cy, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOMOVE|SWP_NOZORDER);
}

void LoadSettings(void)
{
    HKEY    hKey;
    TCHAR   szSubKey[] = _T("Software\\ReactWare\\TaskManager");
    int     i;
    DWORD   dwSize;

    /* Window size & position settings */
    TaskManagerSettings.Maximized = FALSE;
    TaskManagerSettings.Left = 0;
    TaskManagerSettings.Top = 0;
    TaskManagerSettings.Right = 0;
    TaskManagerSettings.Bottom = 0;

    /* Tab settings */
    TaskManagerSettings.ActiveTabPage = 0;

    /* Options menu settings */
    TaskManagerSettings.AlwaysOnTop = FALSE;
    TaskManagerSettings.MinimizeOnUse = TRUE;
    TaskManagerSettings.HideWhenMinimized = TRUE;
    TaskManagerSettings.Show16BitTasks = TRUE;

    /* Update speed settings */
    TaskManagerSettings.UpdateSpeed = 2;

    /* Applications page settings */
    TaskManagerSettings.View_LargeIcons = FALSE;
    TaskManagerSettings.View_SmallIcons = FALSE;
    TaskManagerSettings.View_Details = TRUE;

    /* Processes page settings */
    TaskManagerSettings.ShowProcessesFromAllUsers = FALSE; /* Server-only? */
    TaskManagerSettings.Column_ImageName = TRUE;
    TaskManagerSettings.Column_PID = TRUE;
    TaskManagerSettings.Column_CPUUsage = TRUE;
    TaskManagerSettings.Column_CPUTime = TRUE;
    TaskManagerSettings.Column_MemoryUsage = TRUE;
    TaskManagerSettings.Column_MemoryUsageDelta = FALSE;
    TaskManagerSettings.Column_PeakMemoryUsage = FALSE;
    TaskManagerSettings.Column_PageFaults = FALSE;
    TaskManagerSettings.Column_USERObjects = FALSE;
    TaskManagerSettings.Column_IOReads = FALSE;
    TaskManagerSettings.Column_IOReadBytes = FALSE;
    TaskManagerSettings.Column_SessionID = FALSE; /* Server-only? */
    TaskManagerSettings.Column_UserName = FALSE; /* Server-only? */
    TaskManagerSettings.Column_PageFaultsDelta = FALSE;
    TaskManagerSettings.Column_VirtualMemorySize = FALSE;
    TaskManagerSettings.Column_PagedPool = FALSE;
    TaskManagerSettings.Column_NonPagedPool = FALSE;
    TaskManagerSettings.Column_BasePriority = FALSE;
    TaskManagerSettings.Column_HandleCount = FALSE;
    TaskManagerSettings.Column_ThreadCount = FALSE;
    TaskManagerSettings.Column_GDIObjects = FALSE;
    TaskManagerSettings.Column_IOWrites = FALSE;
    TaskManagerSettings.Column_IOWriteBytes = FALSE;
    TaskManagerSettings.Column_IOOther = FALSE;
    TaskManagerSettings.Column_IOOtherBytes = FALSE;

    for (i = 0; i < 25; i++) {
        TaskManagerSettings.ColumnOrderArray[i] = i;
    }
    TaskManagerSettings.ColumnSizeArray[0] = 105;
    TaskManagerSettings.ColumnSizeArray[1] = 50;
    TaskManagerSettings.ColumnSizeArray[2] = 107;
    TaskManagerSettings.ColumnSizeArray[3] = 70;
    TaskManagerSettings.ColumnSizeArray[4] = 35;
    TaskManagerSettings.ColumnSizeArray[5] = 70;
    TaskManagerSettings.ColumnSizeArray[6] = 70;
    TaskManagerSettings.ColumnSizeArray[7] = 100;
    TaskManagerSettings.ColumnSizeArray[8] = 70;
    TaskManagerSettings.ColumnSizeArray[9] = 70;
    TaskManagerSettings.ColumnSizeArray[10] = 70;
    TaskManagerSettings.ColumnSizeArray[11] = 70;
    TaskManagerSettings.ColumnSizeArray[12] = 70;
    TaskManagerSettings.ColumnSizeArray[13] = 70;
    TaskManagerSettings.ColumnSizeArray[14] = 60;
    TaskManagerSettings.ColumnSizeArray[15] = 60;
    TaskManagerSettings.ColumnSizeArray[16] = 60;
    TaskManagerSettings.ColumnSizeArray[17] = 60;
    TaskManagerSettings.ColumnSizeArray[18] = 60;
    TaskManagerSettings.ColumnSizeArray[19] = 70;
    TaskManagerSettings.ColumnSizeArray[20] = 70;
    TaskManagerSettings.ColumnSizeArray[21] = 70;
    TaskManagerSettings.ColumnSizeArray[22] = 70;
    TaskManagerSettings.ColumnSizeArray[23] = 70;
    TaskManagerSettings.ColumnSizeArray[24] = 70;

    TaskManagerSettings.SortColumn = 1;
    TaskManagerSettings.SortAscending = TRUE;

    /* Performance page settings */
    TaskManagerSettings.CPUHistory_OneGraphPerCPU = TRUE;
    TaskManagerSettings.ShowKernelTimes = FALSE;

    /* Open the key */
    if (RegOpenKeyEx(HKEY_CURRENT_USER, szSubKey, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        return;
    /* Read the settings */
    dwSize = sizeof(TASKMANAGER_SETTINGS);
    RegQueryValueEx(hKey, _T("Preferences"), NULL, NULL, (LPBYTE)&TaskManagerSettings, &dwSize);

    /* Close the key */
    RegCloseKey(hKey);
}

void SaveSettings(void)
{
    HKEY hKey;
    TCHAR szSubKey1[] = _T("Software");
    TCHAR szSubKey2[] = _T("Software\\ReactWare");
    TCHAR szSubKey3[] = _T("Software\\ReactWare\\TaskManager");

    /* Open (or create) the key */
    hKey = NULL;
    RegCreateKeyEx(HKEY_CURRENT_USER, szSubKey1, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL);
    RegCloseKey(hKey);
    hKey = NULL;
    RegCreateKeyEx(HKEY_CURRENT_USER, szSubKey2, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL);
    RegCloseKey(hKey);
    hKey = NULL;
    if (RegCreateKeyEx(HKEY_CURRENT_USER, szSubKey3, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) != ERROR_SUCCESS)
        return;
    /* Save the settings */
    RegSetValueEx(hKey, _T("Preferences"), 0, REG_BINARY, (LPBYTE)&TaskManagerSettings, sizeof(TASKMANAGER_SETTINGS));
    /* Close the key */
    RegCloseKey(hKey);
}

void TaskManager_OnRestoreMainWindow(void)
{
  HMENU hMenu, hOptionsMenu;
  BOOL OnTop;

  hMenu = GetMenu(hMainWnd);
  hOptionsMenu = GetSubMenu(hMenu, OPTIONS_MENU_INDEX);
  OnTop = ((GetWindowLong(hMainWnd, GWL_EXSTYLE) & WS_EX_TOPMOST) != 0);
  
  OpenIcon(hMainWnd);
  SetForegroundWindow(hMainWnd);
  SetWindowPos(hMainWnd, (OnTop ? HWND_TOPMOST : HWND_TOP), 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_SHOWWINDOW);
}

void TaskManager_OnEnterMenuLoop(HWND hWnd)
{
    int nParts;

    /* Update the status bar pane sizes */
    nParts = -1;
    SendMessage(hStatusWnd, SB_SETPARTS, 1, (long)&nParts);
    bInMenuLoop = TRUE;
    SendMessage(hStatusWnd, SB_SETTEXT, (WPARAM)0, (LPARAM)_T(""));
}

void TaskManager_OnExitMenuLoop(HWND hWnd)
{
    RECT  rc;
    int   nParts[3];
    TCHAR text[260];

    bInMenuLoop = FALSE;
    /* Update the status bar pane sizes */
    GetClientRect(hWnd, &rc);
    nParts[0] = 100;
    nParts[1] = 210;
    nParts[2] = rc.right;
    SendMessage(hStatusWnd, SB_SETPARTS, 3, (long)nParts);
    SendMessage(hStatusWnd, SB_SETTEXT, 0, (LPARAM)_T(""));
    wsprintf(text, _T("CPU Usage: %3d%%"), PerfDataGetProcessorUsage());
    SendMessage(hStatusWnd, SB_SETTEXT, 1, (LPARAM)text);
    wsprintf(text, _T("Processes: %d"), PerfDataGetProcessCount());
    SendMessage(hStatusWnd, SB_SETTEXT, 0, (LPARAM)text);
}

void TaskManager_OnMenuSelect(HWND hWnd, UINT nItemID, UINT nFlags, HMENU hSysMenu)
{
    TCHAR str[100];

    _tcscpy(str, TEXT(""));
    if (LoadString(hInst, nItemID, str, 100)) {
        /* load appropriate string */
        LPTSTR lpsz = str;
        /* first newline terminates actual string */
        lpsz = _tcschr(lpsz, '\n');
        if (lpsz != NULL)
            *lpsz = '\0';
    }
    SendMessage(hStatusWnd, SB_SETTEXT, 0, (LPARAM)str);
}

void TaskManager_OnViewUpdateSpeedHigh(void)
{
    HMENU hMenu;
    HMENU hViewMenu;
    HMENU hUpdateSpeedMenu;

    hMenu = GetMenu(hMainWnd);
    hViewMenu = GetSubMenu(hMenu, 2);
    hUpdateSpeedMenu = GetSubMenu(hViewMenu, 1);

    TaskManagerSettings.UpdateSpeed = 1;
    CheckMenuRadioItem(hUpdateSpeedMenu, ID_VIEW_UPDATESPEED_HIGH, ID_VIEW_UPDATESPEED_PAUSED, ID_VIEW_UPDATESPEED_HIGH, MF_BYCOMMAND);

    KillTimer(hMainWnd, 1);
    SetTimer(hMainWnd, 1, 1000, NULL);
}

void TaskManager_OnViewUpdateSpeedNormal(void)
{
    HMENU hMenu;
    HMENU hViewMenu;
    HMENU hUpdateSpeedMenu;

    hMenu = GetMenu(hMainWnd);
    hViewMenu = GetSubMenu(hMenu, 2);
    hUpdateSpeedMenu = GetSubMenu(hViewMenu, 1);

    TaskManagerSettings.UpdateSpeed = 2;
    CheckMenuRadioItem(hUpdateSpeedMenu, ID_VIEW_UPDATESPEED_HIGH, ID_VIEW_UPDATESPEED_PAUSED, ID_VIEW_UPDATESPEED_NORMAL, MF_BYCOMMAND);

    KillTimer(hMainWnd, 1);
    SetTimer(hMainWnd, 1, 2000, NULL);
}

void TaskManager_OnViewUpdateSpeedLow(void)
{
    HMENU hMenu;
    HMENU hViewMenu;
    HMENU hUpdateSpeedMenu;

    hMenu = GetMenu(hMainWnd);
    hViewMenu = GetSubMenu(hMenu, 2);
    hUpdateSpeedMenu = GetSubMenu(hViewMenu, 1);

    TaskManagerSettings.UpdateSpeed = 4;
    CheckMenuRadioItem(hUpdateSpeedMenu, ID_VIEW_UPDATESPEED_HIGH, ID_VIEW_UPDATESPEED_PAUSED, ID_VIEW_UPDATESPEED_LOW, MF_BYCOMMAND);

    KillTimer(hMainWnd, 1);
    SetTimer(hMainWnd, 1, 4000, NULL);
}

void TaskManager_OnViewRefresh(void)
{
    PostMessage(hMainWnd, WM_TIMER, 0, 0);
}

void TaskManager_OnViewUpdateSpeedPaused(void)
{
    HMENU hMenu;
    HMENU hViewMenu;
    HMENU hUpdateSpeedMenu;

    hMenu = GetMenu(hMainWnd);
    hViewMenu = GetSubMenu(hMenu, 2);
    hUpdateSpeedMenu = GetSubMenu(hViewMenu, 1);
    TaskManagerSettings.UpdateSpeed = 0;
    CheckMenuRadioItem(hUpdateSpeedMenu, ID_VIEW_UPDATESPEED_HIGH, ID_VIEW_UPDATESPEED_PAUSED, ID_VIEW_UPDATESPEED_PAUSED, MF_BYCOMMAND);
    KillTimer(hMainWnd, 1);
}

void TaskManager_OnTabWndSelChange(void)
{
    int   i;
    HMENU hMenu;
    HMENU hOptionsMenu;
    HMENU hViewMenu;
    HMENU hSubMenu;
    TCHAR       szTemp[256];

    hMenu = GetMenu(hMainWnd);
    hViewMenu = GetSubMenu(hMenu, 2);
    hOptionsMenu = GetSubMenu(hMenu, 1);
    TaskManagerSettings.ActiveTabPage = TabCtrl_GetCurSel(hTabWnd);
    for (i = GetMenuItemCount(hViewMenu) - 1; i > 2; i--) {
        hSubMenu = GetSubMenu(hViewMenu, i);
        if (hSubMenu)
            DestroyMenu(hSubMenu);
        RemoveMenu(hViewMenu, i, MF_BYPOSITION);
    }
    RemoveMenu(hOptionsMenu, 3, MF_BYPOSITION);
    switch (TaskManagerSettings.ActiveTabPage) {
    case 0:
        ShowWindow(hApplicationPage, SW_SHOW);
        ShowWindow(hProcessPage, SW_HIDE);
        ShowWindow(hPerformancePage, SW_HIDE);
        BringWindowToTop(hApplicationPage);

	LoadString(hInst, IDS_MENU_LARGEICONS, szTemp, 256);
        AppendMenu(hViewMenu, MF_STRING, ID_VIEW_LARGE, szTemp);

	LoadString(hInst, IDS_MENU_SMALLICONS, szTemp, 256);
        AppendMenu(hViewMenu, MF_STRING, ID_VIEW_SMALL, szTemp);

	LoadString(hInst, IDS_MENU_DETAILS, szTemp, 256);
        AppendMenu(hViewMenu, MF_STRING, ID_VIEW_DETAILS, szTemp);

        if (GetMenuItemCount(hMenu) <= 4) {
            hSubMenu = LoadMenu(hInst, MAKEINTRESOURCE(IDR_WINDOWSMENU));

	    LoadString(hInst, IDS_MENU_WINDOWS, szTemp, 256);
            InsertMenu(hMenu, 3, MF_BYPOSITION|MF_POPUP, (UINT)hSubMenu, szTemp);

            DrawMenuBar(hMainWnd);
        }
        if (TaskManagerSettings.View_LargeIcons)
            CheckMenuRadioItem(hViewMenu, ID_VIEW_LARGE, ID_VIEW_DETAILS, ID_VIEW_LARGE, MF_BYCOMMAND);
        else if (TaskManagerSettings.View_SmallIcons)
            CheckMenuRadioItem(hViewMenu, ID_VIEW_LARGE, ID_VIEW_DETAILS, ID_VIEW_SMALL, MF_BYCOMMAND);
        else
            CheckMenuRadioItem(hViewMenu, ID_VIEW_LARGE, ID_VIEW_DETAILS, ID_VIEW_DETAILS, MF_BYCOMMAND);
        /*
         * Give the application list control focus
         */
        SetFocus(hApplicationPageListCtrl);
        break;

    case 1:
        ShowWindow(hApplicationPage, SW_HIDE);
        ShowWindow(hProcessPage, SW_SHOW);
        ShowWindow(hPerformancePage, SW_HIDE);
        BringWindowToTop(hProcessPage);

        LoadString(hInst, IDS_MENU_SELECTCOLUMNS, szTemp, 256);
        AppendMenu(hViewMenu, MF_STRING, ID_VIEW_SELECTCOLUMNS, szTemp);

        LoadString(hInst, IDS_MENU_16BITTASK, szTemp, 256);
        AppendMenu(hOptionsMenu, MF_STRING, ID_OPTIONS_SHOW16BITTASKS, szTemp);

        if (TaskManagerSettings.Show16BitTasks)
            CheckMenuItem(hOptionsMenu, ID_OPTIONS_SHOW16BITTASKS, MF_BYCOMMAND|MF_CHECKED);
        if (GetMenuItemCount(hMenu) > 4)
        {
            RemoveMenu(hMenu, 3, MF_BYPOSITION);
            DrawMenuBar(hMainWnd);
        }
        /*
         * Give the process list control focus
         */
        SetFocus(hProcessPageListCtrl);
        break;

    case 2:
        ShowWindow(hApplicationPage, SW_HIDE);
        ShowWindow(hProcessPage, SW_HIDE);
        ShowWindow(hPerformancePage, SW_SHOW);
        BringWindowToTop(hPerformancePage);
        if (GetMenuItemCount(hMenu) > 4) {
            RemoveMenu(hMenu, 3, MF_BYPOSITION);
            DrawMenuBar(hMainWnd);
        }
        hSubMenu = CreatePopupMenu();

        LoadString(hInst, IDS_MENU_ONEGRAPHALLCPUS, szTemp, 256);
        AppendMenu(hSubMenu, MF_STRING, ID_VIEW_CPUHISTORY_ONEGRAPHALL, szTemp);

        LoadString(hInst, IDS_MENU_ONEGRAPHPERCPU, szTemp, 256);
        AppendMenu(hSubMenu, MF_STRING, ID_VIEW_CPUHISTORY_ONEGRAPHPERCPU, szTemp);

        LoadString(hInst, IDS_MENU_CPUHISTORY, szTemp, 256);
        AppendMenu(hViewMenu, MF_STRING|MF_POPUP, (UINT)hSubMenu, szTemp);

        LoadString(hInst, IDS_MENU_SHOWKERNELTIMES, szTemp, 256);
        AppendMenu(hViewMenu, MF_STRING, ID_VIEW_SHOWKERNELTIMES, szTemp);

        if (TaskManagerSettings.ShowKernelTimes)
            CheckMenuItem(hViewMenu, ID_VIEW_SHOWKERNELTIMES, MF_BYCOMMAND|MF_CHECKED);
        else
            CheckMenuItem(hViewMenu, ID_VIEW_SHOWKERNELTIMES, MF_BYCOMMAND|MF_UNCHECKED);
        if (TaskManagerSettings.CPUHistory_OneGraphPerCPU)
            CheckMenuRadioItem(hSubMenu, ID_VIEW_CPUHISTORY_ONEGRAPHALL, ID_VIEW_CPUHISTORY_ONEGRAPHPERCPU, ID_VIEW_CPUHISTORY_ONEGRAPHPERCPU, MF_BYCOMMAND);
        else
            CheckMenuRadioItem(hSubMenu, ID_VIEW_CPUHISTORY_ONEGRAPHALL, ID_VIEW_CPUHISTORY_ONEGRAPHPERCPU, ID_VIEW_CPUHISTORY_ONEGRAPHALL, MF_BYCOMMAND);
        /*
         * Give the tab control focus
         */
        SetFocus(hTabWnd);
        break;
    }
}

LPTSTR GetLastErrorText(LPTSTR lpszBuf, DWORD dwSize)
{
    DWORD  dwRet;
    LPTSTR lpszTemp = NULL;

    dwRet = FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |FORMAT_MESSAGE_ARGUMENT_ARRAY,
                           NULL,
                           GetLastError(),
                           LANG_NEUTRAL,
                           (LPTSTR)&lpszTemp,
                           0,
                           NULL );

    /* supplied buffer is not long enough */
    if (!dwRet || ( (long)dwSize < (long)dwRet+14)) {
        lpszBuf[0] = TEXT('\0');
    } else {
        lpszTemp[lstrlen(lpszTemp)-2] = TEXT('\0');  /*remove cr and newline character */
        _stprintf(lpszBuf, TEXT("%s (0x%x)"), lpszTemp, (int)GetLastError());
    }
    if (lpszTemp) {
        LocalFree((HLOCAL)lpszTemp);
    }
    return lpszBuf;
}
