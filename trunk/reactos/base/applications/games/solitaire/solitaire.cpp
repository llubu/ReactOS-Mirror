#include <windows.h>
#include <commctrl.h>
#include <tchar.h>
#include "resource.h"
#include "cardlib/cardlib.h"

#include "solitaire.h"

TCHAR szHelpPath[MAX_PATH];

DWORD        dwAppStartTime;
HWND        hwndMain;
HWND        hwndStatus;
HINSTANCE    hInstance;

TCHAR szAppName[] = _T("Solitaire");

CardWindow SolWnd;

LRESULT CALLBACK WndProc (HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);

void MakePath(TCHAR *szDest, UINT nDestLen, TCHAR *szExt)
{
    TCHAR *ptr;
    
    ptr = szDest + GetModuleFileName(GetModuleHandle(0), szDest, nDestLen) - 1;
    while(*ptr-- != '.');
    lstrcpy(ptr + 1, szExt);
}

int main ( int argc, char** argv )
{
    return WinMain ( NULL, NULL, NULL, SW_SHOW );
}

//
//    Main entry point
//
int WINAPI WinMain (HINSTANCE hInst, HINSTANCE hPrev, PSTR szCmdLine, int iCmdShow)
{
    HWND        hwnd;
    MSG            msg;
    WNDCLASSEX    wndclass;
    INITCOMMONCONTROLSEX ice;
    HACCEL        hAccelTable;            

    hInstance = hInst;

    //Window class for the main application parent window
    wndclass.cbSize            = sizeof(wndclass);
    wndclass.style            = 0;//CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc    = WndProc;
    wndclass.cbClsExtra        = 0;
    wndclass.cbWndExtra        = 0;
    wndclass.hInstance        = hInst;
    wndclass.hIcon            = LoadIcon (hInst, MAKEINTRESOURCE(IDI_ICON1));
    wndclass.hCursor        = LoadCursor (NULL, IDC_ARROW);
    wndclass.hbrBackground    = (HBRUSH)NULL;
    wndclass.lpszMenuName    = MAKEINTRESOURCE(IDR_MENU1);
    wndclass.lpszClassName    = szAppName;
    wndclass.hIconSm        = (HICON)LoadImage(hInst, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, 16, 16, 0);

    RegisterClassEx(&wndclass);

    ice.dwSize = sizeof(ice);
    ice.dwICC = ICC_BAR_CLASSES;
    InitCommonControlsEx(&ice);

    srand((unsigned)GetTickCount());//timeGetTime());

//    InitCardLib();

//    LoadSettings();

    //Construct the path to our help file
    MakePath(szHelpPath, MAX_PATH, _T(".hlp"));
    
    hwnd = CreateWindow(szAppName,        // window class name
                szAppName,                // window caption
                WS_OVERLAPPEDWINDOW
                ,//|WS_CLIPCHILDREN,    // window style
                CW_USEDEFAULT,            // initial x position
                CW_USEDEFAULT,            // initial y position
                CW_USEDEFAULT,            // initial x size
                CW_USEDEFAULT,            // initial y size
                NULL,                    // parent window handle
                NULL,                    // use window class menu
                hInst,                    // program instance handle
                NULL);                    // creation parameters

    hwndMain = hwnd;

    ShowWindow(hwnd, iCmdShow);
    UpdateWindow(hwnd);

    hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR1));

    while(GetMessage(&msg, NULL,0,0))
    {
        if(!TranslateAccelerator(hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

//    SaveSettings();

    return msg.wParam;
}


//-----------------------------------------------------------------------------
LRESULT CALLBACK WndProc (HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    static int nWidth, nHeight;
    int nStatusHeight = 0;//20;
    int parts[] = { 100, -1 };
    UINT ret;

    MINMAXINFO *mmi;

    switch(iMsg)
    {
    case WM_CREATE:
        hwndStatus = CreateStatusWindow(WS_CHILD | WS_VISIBLE | CCS_BOTTOM | SBARS_SIZEGRIP, "Ready", hwnd, 0);
        
        //SendMessage(hwndStatus, SB_SIMPLE, (WPARAM)TRUE, 0);

        SendMessage(hwndStatus, SB_SETPARTS, 2, (LPARAM)parts); 
        SendMessage(hwndStatus, SB_SETTEXT, 0 | SBT_NOBORDERS, (LPARAM)"");

        ShowWindow(hwndStatus, SW_HIDE);

        SolWnd.Create(hwnd, WS_EX_CLIENTEDGE, WS_CHILD|WS_VISIBLE, 0, 0, 0, 0);

        CreateSol();

        NewGame();

        dwAppStartTime = GetTickCount();

        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_SIZE:
        nWidth  = LOWORD(lParam);
        nHeight = HIWORD(lParam);

        MoveWindow(SolWnd, 0, 0, nWidth, nHeight-nStatusHeight, TRUE);
        //MoveWindow(hwndStatus, 0, nHeight-nStatusHeight, nWidth, nHeight, TRUE);
        //parts[0] = nWidth - 256;
        //SendMessage(hwndStatus, SB_SETPARTS, 2, (LPARAM)parts); 

        return 0;

    case WM_GETMINMAXINFO:
        mmi = (MINMAXINFO *)lParam;        
        mmi->ptMinTrackSize.x = 600;
        mmi->ptMinTrackSize.y = 400;

        return 0;

    case WM_COMMAND:
    
        switch(LOWORD(wParam))
        {
        case IDM_GAME_NEW:
            //simulate a button click on the new button..
            NewGame();
            return 0;

        case IDM_GAME_DECK:
            //ShowDeckOptionsDlg(hwnd);
            return 0;

        case IDM_GAME_OPTIONS:
            //ShowGameOptionsDlg(hwnd);
            return 0;

        case IDM_HELP_CONTENTS:

            WinHelp(hwnd, szHelpPath, HELP_CONTENTS, 0);//HELP_KEY, (DWORD)"How to play");
            
            return 0;

        case IDM_HELP_ABOUT:
            MessageBox(hwnd, _T("Solitaire by J Brown\r\n\r\nCardLib version 1.0."), szAppName, MB_OK|MB_ICONINFORMATION);

            return 0;

        case IDM_GAME_EXIT:
            PostMessage(hwnd, WM_CLOSE, 0, 0);
            return 0;
        }

        return 0;

    case WM_CLOSE:
        
        ret = IDOK;

        if(fGameStarted)
        {
            ret = MessageBox(hwnd, _T("Quit the current game?"), szAppName, MB_OKCANCEL|MB_ICONQUESTION);
        }

        if(ret == IDOK)
        {
            WinHelp(hwnd, szHelpPath, HELP_QUIT, 0);
            DestroyWindow(hwnd);
        }

        return 0;
    }

    return DefWindowProc (hwnd, iMsg, wParam, lParam);
}

