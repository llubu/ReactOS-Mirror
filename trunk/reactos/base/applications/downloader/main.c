/* PROJECT:         ReactOS Downloader
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/applications/downloader/xml.c
 * PURPOSE:         Main program
 * PROGRAMMERS:     Maarten Bosma, Lester Kortenhoeven, Dmitry Chapyshev
 */

#include <windows.h>
#include <commctrl.h>
#include <richedit.h>
#include <tchar.h>
#include <stdio.h>
#include <shlwapi.h>
#include <shlobj.h>
#include "resources.h"
#include "structures.h"

HWND hwnd, hCategories, hApps, hDownloadButton, hUninstallButton, hUpdateButton, hHelpButton, hProfButton;
HBITMAP hLogo, hUnderline;
WCHAR* DescriptionHeadline = L"";
WCHAR* DescriptionText = L"";
WCHAR ApplicationText[700];

struct Category Root;
struct Application* SelectedApplication;

INT_PTR CALLBACK DownloadProc (HWND, UINT, WPARAM, LPARAM);
BOOL ProcessXML (const char* filename, struct Category* Root);
VOID FreeTree (struct Category* Node);
WCHAR Strings [STRING_COUNT][MAX_STRING_LENGHT];


BOOL
getUninstaller(WCHAR* RegName, WCHAR* Uninstaller) {
    HKEY hKey1;
    HKEY hKey2;
    DWORD Type = 0;
    DWORD Size = MAX_PATH;
    WCHAR Value[MAX_PATH];
    WCHAR KeyName[MAX_PATH];
    LONG i = 0;

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall",0,KEY_READ,&hKey1) == ERROR_SUCCESS) {
        while (RegEnumKeyExW(hKey1,i,KeyName,&Size,NULL,NULL,NULL,NULL) == ERROR_SUCCESS) {
            ++i;
            RegOpenKeyExW(hKey1,KeyName,0,KEY_READ,&hKey2);
            Size = MAX_PATH;
            if (RegQueryValueExW(hKey2,L"DisplayName",0,&Type,(LPBYTE)Value,&Size) == ERROR_SUCCESS) {
                Size = MAX_PATH;
                if (StrCmpW(Value,RegName) == 0) {
                    if (RegQueryValueExW(hKey2,L"UninstallString",0,&Type,(LPBYTE)Uninstaller,&Size) == ERROR_SUCCESS) {
                        RegCloseKey(hKey2);
                        RegCloseKey(hKey1);
                        return TRUE;
                    } else {
                        RegCloseKey(hKey2);
                        RegCloseKey(hKey1);
                        return FALSE;
                    }
                }
            }
            RegCloseKey(hKey2);
            Size = MAX_PATH;
        }
        RegCloseKey(hKey1);
    }
    return FALSE;
}

void
ShowMessage (WCHAR* title, WCHAR* message)
{
    DescriptionHeadline = title;
    DescriptionText = message;
    InvalidateRect(hwnd,NULL,TRUE);
    UpdateWindow(hwnd);
}

void
AddItems(HWND hwnd, struct Category* Category, struct Category* Parent)
{
    TV_INSERTSTRUCTW Insert;

    Insert.item.lParam = (LPARAM)Category;
    Insert.item.mask = TVIF_TEXT|TVIF_PARAM|TVIF_IMAGE|TVIF_SELECTEDIMAGE;;
    Insert.item.pszText = Category->Name;
    Insert.item.cchTextMax = lstrlenW(Category->Name);
    Insert.item.iImage = Category->Icon;
    Insert.item.iSelectedImage = Category->Icon;
    Insert.hInsertAfter = TVI_LAST;
    Insert.hParent = Category->Parent ? Category->Parent->TreeviewItem : TVI_ROOT;

    Category->TreeviewItem = (HTREEITEM)SendMessage(hwnd, TVM_INSERTITEM, 0, (LPARAM)&Insert);

    if(Category->Next)
        AddItems (hwnd,Category->Next,Parent);

    if(Category->Children)
        AddItems (hwnd,Category->Children,Category);
}

void
CategoryChoosen(HWND hwnd, struct Category* Category)
{
    struct Application* CurrentApplication;
    TV_INSERTSTRUCTW Insert;
    WCHAR Uninstaller[200];
    SelectedApplication = NULL;

    if(Category->Children && !Category->Apps)
        ShowMessage(Category->Name, Strings[IDS_CHOOSE_SUB]);
    else if(!Category->Children && Category->Apps)
        ShowMessage(Category->Name, Strings[IDS_CHOOSE_APP]);
    else if(Category->Children && Category->Apps)
        ShowMessage(Category->Name, Strings[IDS_CHOOSE_BOTH]);
    else
        ShowMessage(Category->Name, Strings[IDS_NO_APPS]);

    (void)TreeView_DeleteItem(hwnd, TVI_ROOT);
    (void)TreeView_DeleteItem(hwnd, TVI_ROOT); // Delete twice to bypass bug in windows

    Insert.item.mask = TVIF_TEXT|TVIF_PARAM|TVIF_IMAGE;
    Insert.hInsertAfter = TVI_LAST;
    Insert.hParent = TVI_ROOT;

    CurrentApplication = Category->Apps;
    while(CurrentApplication)
    {
        Insert.item.lParam = (LPARAM)CurrentApplication;
        Insert.item.pszText = CurrentApplication->Name;
        Insert.item.cchTextMax = lstrlenW(CurrentApplication->Name);
        Insert.item.iImage = 10;
        if(StrCmpW(CurrentApplication->RegName,L"")) {
            if(getUninstaller(CurrentApplication->RegName, Uninstaller))
                Insert.item.iImage = 9;
        }
        SendMessage(hwnd, TVM_INSERTITEM, 0, (LPARAM)&Insert);
        CurrentApplication = CurrentApplication->Next;
    }
}

BOOL CreateToolTip(HWND hwndTool, HWND hDlg, WCHAR* pText)
{
    HWND hwndTip;
    TOOLINFO toolInfo;

    if (!hwndTool || !hDlg || !pText)
        return FALSE;

    hwndTip = CreateWindowExW(0, TOOLTIPS_CLASS, NULL,
                                   WS_POPUP |TTS_ALWAYSTIP | TTS_BALLOON,
                                   CW_USEDEFAULT, CW_USEDEFAULT,
                                   CW_USEDEFAULT, CW_USEDEFAULT,
                                   hDlg, NULL, 
                                   GetModuleHandle(NULL), NULL);
    if (!hwndTip)
        return FALSE;

    ZeroMemory(&toolInfo, sizeof(TOOLINFO));
    toolInfo.cbSize = sizeof(toolInfo);
    toolInfo.hwnd = hDlg;
    toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
    toolInfo.uId = (UINT_PTR)hwndTool;
    toolInfo.lpszText = pText;
    SendMessage(hwndTip, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);

    return TRUE;
}

BOOL
SetupControls (HWND hwnd)
{
    TV_INSERTSTRUCTW Insert = {0};
    HIMAGELIST hImageList;
    HINSTANCE hInstance = GetModuleHandle(NULL);
    WCHAR Cats[MAX_STRING_LENGHT], Apps[MAX_STRING_LENGHT];
    WCHAR Tooltip1[MAX_STRING_LENGHT], Tooltip2[MAX_STRING_LENGHT], Tooltip3[MAX_STRING_LENGHT];
    char Buf[MAX_PATH];

	// Getting downloader.xml path
    if(!GetSystemDirectoryA(Buf,sizeof(Buf))) return FALSE;
    strcat(Buf, "\\downloader.xml");

    // Parse the XML file
    if (!ProcessXML(Buf, &Root))
        return FALSE;

    LoadStringW(hInstance, IDS_CATS_TITLE, Cats, MAX_STRING_LENGHT);
    LoadStringW(hInstance, IDS_APPS_TITLE, Apps, MAX_STRING_LENGHT);

    // Set up the controls
    hCategories = CreateWindowExW(0, WC_TREEVIEWW, Cats,
                                  WS_CHILD|WS_VISIBLE|WS_BORDER|TVS_HASLINES|TVS_LINESATROOT|TVS_HASBUTTONS|TVS_SHOWSELALWAYS,
                                  0, 0, 0, 0, hwnd, NULL, hInstance, NULL);

    hApps = CreateWindowExW(0, WC_TREEVIEWW, Apps,
                            WS_CHILD|WS_VISIBLE|WS_BORDER|TVS_HASLINES|TVS_LINESATROOT|TVS_HASBUTTONS|TVS_SHOWSELALWAYS,
                            0, 0, 0, 0, hwnd, NULL, hInstance, NULL);

    hLogo = LoadBitmap(GetModuleHandle(NULL),
                       MAKEINTRESOURCE(IDB_LOGO));
    hUnderline = LoadBitmap(GetModuleHandle(NULL),
                            MAKEINTRESOURCE(IDB_UNDERLINE));

    hHelpButton = CreateWindowW(L"Button", L"",
                                WS_CHILD | WS_VISIBLE | BS_ICON,
                                550, 10, 40, 40,
                                hwnd, 0, hInstance, NULL);
    LoadString(hInstance, TTT_HELPBUTTON, Tooltip1, MAX_STRING_LENGHT);
    CreateToolTip(hHelpButton, hwnd, Tooltip1);

    hUpdateButton = CreateWindowW(L"Button", L"",
                                  WS_CHILD | WS_VISIBLE | BS_ICON,
                                  450, 10, 40, 40,
                                  hwnd, 0, hInstance, NULL);
    LoadString(hInstance, TTT_UPDATEBUTTON, Tooltip2, MAX_STRING_LENGHT);
    CreateToolTip(hUpdateButton, hwnd, Tooltip2);

    hProfButton = CreateWindowW(L"Button", L"",
                                WS_CHILD | WS_VISIBLE | BS_ICON,
                                500, 10, 40, 40,
                                hwnd, 0, hInstance, NULL);
    LoadString(hInstance, TTT_PROFBUTTON, Tooltip3, MAX_STRING_LENGHT);
    CreateToolTip(hProfButton, hwnd, Tooltip3);

    hDownloadButton = CreateWindowW(L"Button", L"",
                                    WS_CHILD | WS_VISIBLE | BS_BITMAP,
                                    330, 505, 140, 33,
                                    hwnd, 0, hInstance, NULL);

    hUninstallButton = CreateWindowW(L"Button", L"",
                                     WS_CHILD | WS_VISIBLE | BS_BITMAP,
                                     260, 505, 140, 33,
                                     hwnd, 0, hInstance, NULL);

    SendMessageW(hProfButton,
                BM_SETIMAGE,
                (WPARAM)IMAGE_ICON,
                (LPARAM)(HANDLE)LoadIcon(hInstance,MAKEINTRESOURCE(IDI_PROF)));
    SendMessageW(hHelpButton,
                BM_SETIMAGE,
                (WPARAM)IMAGE_ICON,
                (LPARAM)(HANDLE)LoadIcon(hInstance, MAKEINTRESOURCE(IDI_HELP)));
    SendMessageW(hUpdateButton,
                BM_SETIMAGE,
                (WPARAM)IMAGE_ICON,
                (LPARAM)(HANDLE)LoadIcon(hInstance, MAKEINTRESOURCE(IDI_UPDATE)));
    SendMessageW(hDownloadButton,
                BM_SETIMAGE,
                (WPARAM)IMAGE_BITMAP,
                (LPARAM)(HANDLE)LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_DOWNLOAD)));
    SendMessageW(hUninstallButton,
                BM_SETIMAGE,
                (WPARAM)IMAGE_BITMAP,
                (LPARAM)(HANDLE)LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_UNINSTALL)));
    ShowWindow(hUninstallButton, SW_HIDE);

    // Set deflaut entry for hApps
    Insert.item.mask = TVIF_TEXT|TVIF_IMAGE;
    Insert.item.pszText = Strings[IDS_CHOOSE_CATEGORY];
    Insert.item.cchTextMax = lstrlenW(Strings[IDS_CHOOSE_CATEGORY]);
    Insert.item.iImage = 0;
    SendMessage(hApps, TVM_INSERTITEM, 0, (LPARAM)&Insert);

    // Create Tree Icons
    hImageList = ImageList_Create(16, 16, ILC_COLORDDB, 1, 1);
    SendMessageW(hCategories, TVM_SETIMAGELIST, TVSIL_NORMAL, (LPARAM)(HIMAGELIST)hImageList);
    SendMessageW(hApps, TVM_SETIMAGELIST, TVSIL_NORMAL, (LPARAM)(HIMAGELIST)hImageList);

    ImageList_Add(hImageList,
                  LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_TREEVIEW_ICON_0)), NULL);
    ImageList_Add(hImageList,
                  LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_TREEVIEW_ICON_1)), NULL);
    ImageList_Add(hImageList,
                  LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_TREEVIEW_ICON_2)), NULL);
    ImageList_Add(hImageList,
                  LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_TREEVIEW_ICON_3)), NULL);
    ImageList_Add(hImageList,
                  LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_TREEVIEW_ICON_4)), NULL);
    ImageList_Add(hImageList,
                  LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_TREEVIEW_ICON_5)), NULL);
    ImageList_Add(hImageList,
                  LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_TREEVIEW_ICON_6)), NULL);
    ImageList_Add(hImageList,
                  LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_TREEVIEW_ICON_7)), NULL);
    ImageList_Add(hImageList,
                  LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_TREEVIEW_ICON_8)), NULL);
    ImageList_Add(hImageList,
                  LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_TREEVIEW_ICON_9)), NULL);
    ImageList_Add(hImageList,
                  LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_TREEVIEW_ICON_10)), NULL);
    ImageList_Add(hImageList,
                  LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_TREEVIEW_ICON_11)), NULL);
    ImageList_Add(hImageList,
                  LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_TREEVIEW_ICON_12)), NULL);
    ImageList_Add(hImageList,
                  LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_TREEVIEW_ICON_13)), NULL);

    // Fill the TreeViews
    AddItems (hCategories, Root.Children, NULL);

    return TRUE;
}

static void
ResizeControl (HWND hwnd, int x1, int y1, int x2, int y2)
{
    // Make resizing a little easier
    MoveWindow(hwnd, x1, y1, x2-x1, y2-y1, TRUE);
}

static void
DrawBitmap (HDC hdc, int x, int y, HBITMAP hBmp)
{
    BITMAP bm;
    HDC hdcMem = CreateCompatibleDC(hdc);

    SelectObject(hdcMem, hBmp);
    GetObject(hBmp, sizeof(bm), &bm);
    TransparentBlt(hdc, x, y, bm.bmWidth, bm.bmHeight, hdcMem, 0, 0, bm.bmWidth, bm.bmHeight, 0xFFFFFF);

    DeleteDC(hdcMem);
}

static void
DrawDescription (HDC hdc, RECT DescriptionRect)
{
    int i;
    HFONT Font;
    RECT Rect = {DescriptionRect.left+5, DescriptionRect.top+3, DescriptionRect.right-2, DescriptionRect.top+22};

    // Backgroud
    Rectangle(hdc, DescriptionRect.left, DescriptionRect.top, DescriptionRect.right, DescriptionRect.bottom);

    // Underline
    for (i=DescriptionRect.left+1;i<DescriptionRect.right-1;i++)
        DrawBitmap(hdc, i, DescriptionRect.top+22, hUnderline); // less code then stretching ;)

    // Headline
    Font = CreateFont(-16 , 0, 0, 0, FW_EXTRABOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
                      OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE, L"Arial");
    SelectObject(hdc, Font);
    DrawTextW(hdc, DescriptionHeadline, lstrlenW(DescriptionHeadline), &Rect, DT_SINGLELINE|DT_NOPREFIX);
    DeleteObject(Font);

    // Description
    Font = CreateFont(-13 , 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
                      OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE, L"Arial");
    SelectObject(hdc, Font);
    Rect.top += 50;
    Rect.bottom = DescriptionRect.bottom-2;
    DrawTextW(hdc, DescriptionText, lstrlenW(DescriptionText), &Rect, DT_WORDBREAK|DT_NOPREFIX); // ToDo: Call TabbedTextOut to draw a nice table
    DeleteObject(Font);

}

void showUninstaller() {
    int Split_Vertical = 200;
    RECT Rect;

    GetClientRect(hwnd,&Rect);
    ShowWindow(hUninstallButton,SW_SHOW);
    MoveWindow(hDownloadButton,(Split_Vertical+Rect.right-Rect.left)/2,Rect.bottom-Rect.top-45,140,35,TRUE);;
}

void hideUninstaller() {
    int Split_Vertical = 200;
    RECT Rect;

    GetClientRect(hwnd,&Rect);
    ShowWindow(hUninstallButton,SW_HIDE);
    MoveWindow(hDownloadButton,(Split_Vertical+Rect.right-Rect.left)/2-70,Rect.bottom-Rect.top-45,140,35,TRUE);
}

void startUninstaller(WCHAR* Uninstaller) {
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;

    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    CreateProcessW(NULL,Uninstaller,NULL,NULL,FALSE,0,NULL,NULL,&si,&pi);
    CloseHandle(pi.hThread);
    // WaitForSingleObject(pi.hProcess, INFINITE); // If you want to wait for the Unistaller
    CloseHandle(pi.hProcess);
    hideUninstaller();
}

BOOL
SaveSettings(HWND hwnd)
{
    HKEY hKey;
    TCHAR szBuf[MAX_PATH];
    DWORD dwValue;

    if (RegOpenKey(HKEY_LOCAL_MACHINE,
                   TEXT("Software\\ReactOS\\Downloader"),
                   &hKey) != ERROR_SUCCESS)
        return FALSE;

    GetDlgItemText(hwnd, IDC_DOWNLOAD_FOLDER_EDIT, szBuf, sizeof(szBuf) / sizeof(TCHAR));
    if (GetFileAttributes(szBuf) == 0xFFFFFFFF)
        if (!CreateDirectory((LPCTSTR)szBuf,NULL))
        {
            LoadString(GetModuleHandle(NULL), IDS_UNABLECREATE_FOLDER, szBuf, sizeof(szBuf) / sizeof(TCHAR));
            MessageBox(hwnd, (LPCTSTR)szBuf, NULL, MB_ICONSTOP);
            return FALSE;
        }
    if (RegSetValueEx(hKey,
                      L"DownloadFolder",
                      0,
                      REG_SZ,
                      (LPBYTE)szBuf,
                      (DWORD)(sizeof(szBuf) / sizeof(TCHAR))))
        return FALSE;

    GetDlgItemText(hwnd, IDC_UPDATE_SERVER_EDIT, szBuf, sizeof(szBuf) / sizeof(TCHAR));
    if (RegSetValueEx(hKey,
                      L"UpdateServer",
                      0,
                      REG_SZ,
                      (LPBYTE)szBuf,
                      (DWORD)(sizeof(szBuf) / sizeof(TCHAR))))
        return FALSE;

    if (SendDlgItemMessage(hwnd, IDC_DELINST_FILES_CHECKBOX, BM_GETCHECK, 0, 0) == BST_CHECKED)
        dwValue = 0x1;
    else
        dwValue = 0x0;
    if (RegSetValueEx(hKey,
                      L"DeleteInstaller",
                      0,
                      REG_DWORD,
                      (LPBYTE)&dwValue,
                      sizeof(DWORD)))
            return FALSE;

    RegCloseKey(hKey);

    return TRUE;
}

BOOL
InitProfDlg(HWND hwnd)
{
    HKEY hKey;
    TCHAR Buf[MAX_PATH];
    DWORD dwDisp, dwSize;
    DWORD dwValue, dwType = REG_DWORD;

    if (RegOpenKey(HKEY_LOCAL_MACHINE,
                   TEXT("Software\\ReactOS\\Downloader"),
                   &hKey) != ERROR_SUCCESS)
    {
        if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, L"Software\\ReactOS\\Downloader",
                           0, NULL, REG_OPTION_NON_VOLATILE,
                           KEY_WRITE, NULL, &hKey, &dwDisp))
        {
            LoadString(GetModuleHandle(NULL), IDS_NOTCREATE_REGKEY, Buf, sizeof(Buf) / sizeof(TCHAR));
            MessageBox(hwnd, (LPCTSTR)Buf, NULL, MB_ICONSTOP);
            return FALSE;
        }
    }

    dwSize = MAX_PATH;
    if (RegQueryValueEx(hKey,
                        L"DownloadFolder",
                        NULL,
                        NULL,
                        (LPBYTE)&Buf,
                        &dwSize) == ERROR_SUCCESS)
    {
        SetDlgItemText(hwnd, IDC_DOWNLOAD_FOLDER_EDIT, Buf);
    }
    else
    {
        TCHAR DPath[256];
        int i;

        if (!GetWindowsDirectory(Buf, sizeof(Buf) / sizeof(TCHAR))) return FALSE;

        for (i = 0; i < 4; i++)
        {
            if (i == 3)
            {
                DPath[i] = '\0';
                break;
            }
            DPath[i] = Buf[i];
        }
        LoadString(GetModuleHandle(NULL), IDS_DOWNLOAD_FOLDER, Buf, sizeof(Buf) / sizeof(TCHAR));
        lstrcat((LPTSTR)DPath, Buf);
        if (RegSetValueEx(hKey,
                          L"DownloadFolder",
                          0,
                          REG_SZ,
                          (LPBYTE)DPath,
                          (DWORD)(sizeof(DPath) / sizeof(TCHAR))))
            return FALSE;
        else
        {
            if (GetFileAttributes(DPath) == 0xFFFFFFFF)
                if (!CreateDirectory((LPCTSTR)DPath,NULL)) return FALSE;
            SetDlgItemText(hwnd, IDC_DOWNLOAD_FOLDER_EDIT, DPath);
        }
    }

    dwSize = MAX_PATH;
    if (RegQueryValueEx(hKey,
                        L"UpdateServer",
                        NULL,
                        NULL,
                        (LPBYTE)&Buf,
                        &dwSize) == ERROR_SUCCESS)
    {
        SetDlgItemText(hwnd, IDC_UPDATE_SERVER_EDIT, Buf);
    }
    else
    {
        LoadString(GetModuleHandle(NULL), IDS_UPDATE_URL, Buf, sizeof(Buf) / sizeof(TCHAR));
        if (RegSetValueEx(hKey,
                          L"UpdateServer",
                          0,
                          REG_SZ,
                          (LPBYTE)Buf,
                          (DWORD)(sizeof(Buf) / sizeof(TCHAR))))
            return FALSE;
        else
        {
            SetDlgItemText(hwnd, IDC_UPDATE_SERVER_EDIT, Buf);
        }
    }

    dwSize = sizeof(DWORD);
    if (RegQueryValueEx(hKey,
                        L"DeleteInstaller",
                        NULL,
                        &dwType,
                        (LPBYTE)&dwValue,
                        &dwSize) == ERROR_SUCCESS)
    {
        if (dwValue == 0x1)
            SendDlgItemMessage(hwnd, IDC_DELINST_FILES_CHECKBOX, BM_SETCHECK, 1, 1);
    }
    else
    {
        dwValue = 0x0;
        if (RegSetValueEx(hKey,
                          L"DeleteInstaller",
                          0,
                          REG_DWORD,
                          (LPBYTE)&dwValue,
                          sizeof(DWORD)))
            return FALSE;
    }

    RegCloseKey(hKey);

    return TRUE;
}

BOOL
ChooseFolder(HWND hwnd)
{
    BROWSEINFO fi;
    LPCITEMIDLIST lpItemList;
    TCHAR szPath[MAX_PATH],Buf[256];

    ZeroMemory(&fi, sizeof(BROWSEINFO));
    fi.hwndOwner = hwnd;
    LoadString(GetModuleHandle(NULL), IDS_CHOOSE_FOLDER, Buf, sizeof(Buf) / sizeof(TCHAR));
    fi.lpszTitle = (LPCTSTR)Buf;
    fi.ulFlags = BIF_DONTGOBELOWDOMAIN | BIF_RETURNONLYFSDIRS | BIF_BROWSEFORCOMPUTER | BIF_NEWDIALOGSTYLE;
    fi.lpfn = NULL;
    fi.lParam = -1;
    fi.iImage = 0;

    if(!(lpItemList = SHBrowseForFolder(&fi))) return FALSE;
    SHGetPathFromIDList(lpItemList, szPath);

    if (_tcslen(szPath) == 0) return FALSE;
    SetDlgItemText(hwnd, IDC_DOWNLOAD_FOLDER_EDIT, szPath);

    return TRUE;
}

INT_PTR CALLBACK
ProfDlgProc(HWND hDlg,
            UINT message,
            WPARAM wParam,
            LPARAM lParam)
{
    static HICON hIcon;
    UNREFERENCED_PARAMETER(lParam);

    switch (message)
    {
        case WM_INITDIALOG:
        {
            hIcon = LoadImage(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_MAIN),IMAGE_ICON,16,16,0);
            SendMessage(hDlg,WM_SETICON,ICON_SMALL,(LPARAM)hIcon);
            InitProfDlg(hDlg);
        }
        break;

        case WM_COMMAND:
        {
            switch (wParam)
            {
                case IDC_CHOOSE_BUTTON:
                    ChooseFolder(hDlg);
                break;
                case IDOK:
                {
                    SaveSettings(hDlg);
                    DestroyIcon(hIcon);
                    EndDialog(hDlg,LOWORD(wParam));
                    return TRUE;
                }
                break;
                case IDCANCEL:
                {
                    DestroyIcon(hIcon);
                    EndDialog(hDlg,LOWORD(wParam));
                    return TRUE;
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}

BOOL IsApplicationInstalled(struct Application* App)
{
    WCHAR Uninstaller[200];
    if(StrCmpW(App->RegName, L"")) {
        if(getUninstaller(App->RegName, Uninstaller)) {
            return TRUE;
        }
    }
    return FALSE;
}

struct Application* GetDependency(const WCHAR* Dependency)
{
    struct Category* Category = Root.Children;

    while (Category->Next)
    {
        while (Category->Apps)
        {
            if(StrCmpW(Category->Apps->RegName, Dependency) == 0)
                return Category->Apps;
            Category->Apps = Category->Apps->Next;
        }
        Category = Category->Next;
    }
    return NULL;
}

LRESULT CALLBACK
WndProc (HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    static RECT DescriptionRect;
    struct Application* AppToInstall;
    WCHAR InstallDep[260];
    WCHAR InstallDepBuffer[260];
    WCHAR Title[260];

    switch (Message)
    {
        case WM_CREATE:
        {
            if(!SetupControls(hwnd))
                return -1;
            ShowMessage(Strings[IDS_WELCOME_TITLE], Strings[IDS_WELCOME]);
        }
        break;

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            HDC BackbufferHdc = CreateCompatibleDC(hdc);
            HBITMAP BackbufferBmp = CreateCompatibleBitmap(hdc, ps.rcPaint.right, ps.rcPaint.bottom);
            SelectObject(BackbufferHdc, BackbufferBmp);

            FillRect(BackbufferHdc, &ps.rcPaint, CreateSolidBrush(RGB(235,235,235)));
            DrawBitmap(BackbufferHdc, 10, 12, hLogo);
            DrawDescription(BackbufferHdc, DescriptionRect);

            BitBlt(hdc, 0, 0, ps.rcPaint.right, ps.rcPaint.bottom, BackbufferHdc, 0, 0, SRCCOPY);
            DeleteObject(BackbufferBmp);
            DeleteDC(BackbufferHdc);
            EndPaint(hwnd, &ps);
        }
        break;

        case WM_COMMAND:
        {
            if(HIWORD(wParam) == BN_CLICKED)
            {
                if (lParam == (LPARAM)hProfButton)
                {
                    DialogBox(GetModuleHandle(NULL),
                              MAKEINTRESOURCE(IDD_PROF),
                              hwnd,
                              ProfDlgProc);
                }
                if (lParam == (LPARAM)hDownloadButton)
                {
                    if(SelectedApplication)
                    {
                        /* install dependencies */
                        if(StrCmpW(SelectedApplication->Depends, L""))
                        {
                            AppToInstall = SelectedApplication;
                            SelectedApplication = GetDependency(SelectedApplication->Depends);
                            if (SelectedApplication)
                                if (!IsApplicationInstalled(SelectedApplication))
                                {
                                    LoadString(GetModuleHandle(NULL), IDS_INSTALL_DEP, InstallDep, sizeof(InstallDep) / sizeof(WCHAR));
                                    LoadString(GetModuleHandle(NULL), IDS_WINDOW_TITLE, Title, sizeof(Title) / sizeof(WCHAR));
                                    _snwprintf(InstallDepBuffer, sizeof(InstallDepBuffer) / sizeof(WCHAR), InstallDep, SelectedApplication->Name, AppToInstall->Name, SelectedApplication->Name);
                                    if (MessageBox(hwnd, InstallDepBuffer, Title, MB_YESNO | MB_ICONINFORMATION) == IDYES)
                                    {
                                        DialogBoxW(GetModuleHandle(NULL), MAKEINTRESOURCEW(IDD_DOWNLOAD), 0, DownloadProc);
                                    }
                                }
                            SelectedApplication = AppToInstall;
                        }

                        /* download and install the app */
                        DialogBoxW(GetModuleHandle(NULL), MAKEINTRESOURCEW(IDD_DOWNLOAD), 0, DownloadProc);
                        
                        /* install req. hacks to get it working */
                        if(StrCmpW(SelectedApplication->PostInstallAction, L""))
                        {
                            AppToInstall = SelectedApplication;
                            CopyMemory(SelectedApplication->Location, SelectedApplication->PostInstallAction, sizeof(SelectedApplication->Location));
                            DialogBoxW(GetModuleHandle(NULL), MAKEINTRESOURCEW(IDD_DOWNLOAD), 0, DownloadProc);
                            SelectedApplication = AppToInstall;
                        }
                    }
                    else
                        ShowMessage(Strings[IDS_NO_APP_TITLE], Strings[IDS_NO_APP]);
                }
                else if (lParam == (LPARAM)hUninstallButton)
                {
                    if(SelectedApplication)
                    {
                        WCHAR Uninstaller[200];
                        if(StrCmpW(SelectedApplication->RegName, L"")) {
                            if(getUninstaller(SelectedApplication->RegName, Uninstaller))
                                startUninstaller(Uninstaller);
                        }
                    }
                }
                else if (lParam == (LPARAM)hUpdateButton)
                {
                    ShowMessage(Strings[IDS_UPDATE_TITLE], Strings[IDS_UPDATE]);
                }
                else if (lParam == (LPARAM)hHelpButton)
                {
                    ShowMessage(Strings[IDS_HELP_TITLE], Strings[IDS_HELP]);
                }
            }
        }
        break;

        case WM_NOTIFY:
        {
            LPNMHDR data = (LPNMHDR)lParam;
            WCHAR Uninstaller[200];

            if(data->code == TVN_SELCHANGED)
            {
                BOOL bShowUninstaller = FALSE;
                if(data->hwndFrom == hCategories)
                {
                    struct Category* Category = (struct Category*) ((LPNMTREEVIEW)lParam)->itemNew.lParam;
                    CategoryChoosen (hApps, Category);
                }
                else if(data->hwndFrom == hApps)
                {
                    SelectedApplication = (struct Application*) ((LPNMTREEVIEW)lParam)->itemNew.lParam;
                    if(SelectedApplication)
                    {
                        ApplicationText[0]=L'\0';
                        if(StrCmpW(SelectedApplication->Version, L"")) {
                            StrCatW(ApplicationText, Strings[IDS_VERSION]);
                            StrCatW(ApplicationText, SelectedApplication->Version);
                            StrCatW(ApplicationText, L"\n");
                        }
                        if(StrCmpW(SelectedApplication->Licence, L"")) {
                            StrCatW(ApplicationText, Strings[IDS_LICENCE]);
                            StrCatW(ApplicationText, SelectedApplication->Licence);
                            StrCatW(ApplicationText, L"\n");
                        }
                        if(StrCmpW(SelectedApplication->Maintainer, L"")) {
                            StrCatW(ApplicationText, Strings[IDS_MAINTAINER]);
                            StrCatW(ApplicationText, SelectedApplication->Maintainer);
                            StrCatW(ApplicationText, L"\n");
                        }
                        if(StrCmpW(SelectedApplication->Licence, L"") || StrCmpW(SelectedApplication->Version, L"") || StrCmpW(SelectedApplication->Maintainer, L""))
                            StrCatW(ApplicationText, L"\n");
                        StrCatW(ApplicationText, SelectedApplication->Description);
                        ShowMessage(SelectedApplication->Name, ApplicationText);
                        if(StrCmpW(SelectedApplication->RegName, L"")) {
                            if(getUninstaller(SelectedApplication->RegName, Uninstaller)) {
                                bShowUninstaller = TRUE;
                            }
                        }
                    }
                }
                if (bShowUninstaller)
                    showUninstaller();
                else
                    hideUninstaller();
            }
        }
        break;

        case WM_SIZING:
        {
            LPRECT pRect = (LPRECT)lParam;
            if (pRect->right-pRect->left < 520)
                pRect->right = pRect->left + 520;

            if (pRect->bottom-pRect->top < 300)
                pRect->bottom = pRect->top + 300;
        }
        break;

        case WM_SIZE:
        {
            int Split_Hozizontal = (HIWORD(lParam)-(45+60))/2 + 60;
            int Split_Vertical = 200;
            RECT Rect;

            ResizeControl(hCategories, 10, 60, Split_Vertical, HIWORD(lParam)-10);
            ResizeControl(hApps, Split_Vertical+5, 60, LOWORD(lParam)-10, Split_Hozizontal);
            SetRect(&Rect, Split_Vertical+5, Split_Hozizontal+5, LOWORD(lParam)-10, HIWORD(lParam)-50);
            DescriptionRect = Rect;

            MoveWindow(hHelpButton, LOWORD(lParam)-50, 10, 40, 40, TRUE);
            MoveWindow(hUpdateButton, LOWORD(lParam)-150, 10, 40, 40, TRUE);
            MoveWindow(hProfButton, LOWORD(lParam)-100, 10, 40, 40, TRUE);
            if(IsWindowVisible(hUninstallButton))
                MoveWindow(hDownloadButton, (Split_Vertical+LOWORD(lParam))/2, HIWORD(lParam)-45, 140, 35, TRUE);
            else
                MoveWindow(hDownloadButton, (Split_Vertical+LOWORD(lParam))/2-70, HIWORD(lParam)-45, 140, 35, TRUE);
            MoveWindow(hUninstallButton, (Split_Vertical+LOWORD(lParam))/2-140, HIWORD(lParam)-45, 140, 35, TRUE);
        }
        break;

        case WM_DESTROY:
        {
            DeleteObject(hLogo);
            if(Root.Children)
                FreeTree(Root.Children);
            PostQuitMessage(0);
            return 0;
        }
        break;
    }

    return DefWindowProc (hwnd, Message, wParam, lParam);
}

INT WINAPI
wWinMain (HINSTANCE hInstance,
          HINSTANCE hPrevInst,
          LPTSTR lpCmdLine,
          INT nCmdShow)
{
    int i;
    WNDCLASSEXW WndClass = {0};
    MSG msg;

    InitCommonControls();

    // Load strings
    for(i=0; i<STRING_COUNT; i++)
        LoadStringW(hInstance, i, Strings[i], MAX_STRING_LENGHT); // if you know a better method please tell me.

    // Create the window
    WndClass.cbSize            = sizeof(WNDCLASSEX);
    WndClass.lpszClassName     = L"Downloader";
    WndClass.lpfnWndProc       = WndProc;
    WndClass.hInstance         = hInstance;
    WndClass.style             = CS_HREDRAW | CS_VREDRAW;
    WndClass.hIcon             = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAIN));
    WndClass.hCursor           = LoadCursor(NULL, IDC_ARROW);

    RegisterClassExW(&WndClass);

    hwnd = CreateWindowW(L"Downloader",
                         Strings[IDS_WINDOW_TITLE],
                         WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN,
                         CW_USEDEFAULT,
                         CW_USEDEFAULT,
                         720, 550,
                         NULL, NULL,
                         hInstance,
                         NULL);

    // Show it
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    // Message Loop
    while(GetMessage(&msg,NULL,0,0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
