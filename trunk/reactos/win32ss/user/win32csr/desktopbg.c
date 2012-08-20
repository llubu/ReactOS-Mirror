/*
 * win32ss/user/win32csr/desktopbg.c
 *
 * Desktop background window functions
 *
 * ReactOS Operating System
 *
 * Looks as a hax fix to a problem of not having a proper class window proc support
 * which ReactOS has now.
 *
 * Notes on reason why this is here:
 * http://www.reactos.org/archives/public/ros-kernel/2003-November/000538.html
 * http://www.reactos.org/archives/public/ros-kernel/2003-November/000545.html
 * http://www.reactos.org/archives/public/ros-kernel/2003-November/000586.html
 * http://www.reactos.org/archives/public/ros-kernel/2003-November/000674.html
 * Revision 6908 Move desktop window proc from WIN32K to CSRSS
 * Bugs 42, 48 and 57.
 *
 * http://www.reactos.org/archives/public/ros-kernel/2003-December/001186.html
 *
 
   Now this is used to start Desktops Threads with TEB support.
   These first three are Application, Winlogon and ScreenSaver desktops. 
 
 */

#define NDEBUG

#include "w32csr.h"
#include <debug.h>

#define DESKTOP_WINDOW_ATOM 32769

#define PM_SHOW_DESKTOP 1
#define PM_HIDE_DESKTOP 2

typedef struct tagDTBG_THREAD_DATA
{
    HDESK Desktop;
    HANDLE Event;
    NTSTATUS Status;
} DTBG_THREAD_DATA, *PDTBG_THREAD_DATA;

typedef struct tagPRIVATE_NOTIFY_DESKTOP
{
    NMHDR hdr;
    struct /* PM_SHOW_DESKTOP */
    {
        int Width;
        int Height;
    } ShowDesktop;
} PRIVATE_NOTIFY_DESKTOP, *PPRIVATE_NOTIFY_DESKTOP;

static BOOL BgInitialized = FALSE;
static HWND VisibleDesktopWindow = NULL;
#if 0
static
LRESULT
CALLBACK
DtbgWindowProc(HWND Wnd,
               UINT Msg,
               WPARAM wParam,
               LPARAM lParam)
{
    PAINTSTRUCT PS;

    switch (Msg)
    {
    case WM_ERASEBKGND:
        PaintDesktop((HDC)wParam);
        return 1;

    case WM_PAINT:
        if (BeginPaint(Wnd, &PS))
            EndPaint(Wnd, &PS);
        return 0;

    case WM_SETCURSOR:
        return (LRESULT)SetCursor(LoadCursorW(0, (LPCWSTR)IDC_ARROW));

    case WM_NCCREATE:
        return (LRESULT)TRUE;

    case WM_CREATE:
        NtUserSetWindowFNID(Wnd, FNID_DESKTOP); // Anti-ReactOS hack!
    case WM_CLOSE:
        return 0;

    case WM_DISPLAYCHANGE:
        MoveWindow(Wnd, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
        break;

    case WM_NOTIFY:
    {
        PPRIVATE_NOTIFY_DESKTOP nmh = (PPRIVATE_NOTIFY_DESKTOP)lParam;

        /* Use WM_NOTIFY for private messages since
         * it can't be sent between processes!
         */
        switch (nmh->hdr.code)
        {
        case PM_SHOW_DESKTOP:
        {
            LRESULT Result;

            Result = !SetWindowPos(Wnd, NULL, 0, 0,
                                   nmh->ShowDesktop.Width,
                                   nmh->ShowDesktop.Height,
                                   SWP_NOACTIVATE | SWP_NOZORDER |
                                   SWP_SHOWWINDOW);

            UpdateWindow(Wnd);
            VisibleDesktopWindow = Wnd;
            return Result;
        }

        case PM_HIDE_DESKTOP:
        {
            LRESULT Result;

            Result = !SetWindowPos(Wnd, NULL, 0, 0, 0, 0,
                                   SWP_NOACTIVATE | SWP_NOZORDER |
                                   SWP_NOMOVE | SWP_NOSIZE | SWP_HIDEWINDOW);

            UpdateWindow(Wnd);
            VisibleDesktopWindow = NULL;
            return Result;
        }

        default:
            DPRINT("Unknown notification code 0x%x sent to the desktop window!\n",
                   nmh->hdr.code);
            return 0;
        }
    }

    default:
        return DefWindowProcW(Wnd, Msg, wParam, lParam);
    }

    return 0;
}

static
BOOL
FASTCALL
DtbgInit(VOID)
{
    WNDCLASSEXW Class;
    ATOM ClassAtom;

    /*
     * Create the desktop window class
     */
    Class.cbSize = sizeof(WNDCLASSEXW);
    Class.style = 0; // Local Class
    Class.lpfnWndProc = DtbgWindowProc;
    Class.cbClsExtra = 0;
    Class.cbWndExtra = 0;
    Class.hInstance = (HINSTANCE)GetModuleHandleW(NULL);
    Class.hIcon = NULL;
    Class.hCursor = NULL;
    Class.hbrBackground = GetSysColorBrush(COLOR_BACKGROUND);
    Class.lpszMenuName = NULL;
    Class.lpszClassName = (LPCWSTR)DESKTOP_WINDOW_ATOM;
    ClassAtom = RegisterClassExW(&Class);

    if (ClassAtom == INVALID_ATOM)
    {
        DPRINT1("Unable to register desktop background class (error %d)\n",
                GetLastError());

        return FALSE;
    }

    VisibleDesktopWindow = NULL;

    return TRUE;
}
#endif
static
DWORD
WINAPI
DtbgDesktopThread(PVOID Data)
{
    HWND BackgroundWnd;
    MSG msg;
    PDTBG_THREAD_DATA ThreadData = (PDTBG_THREAD_DATA)Data;

    DPRINT("DtbgDesktopThread\n");

    if (!SetThreadDesktop(ThreadData->Desktop))
    {
        DPRINT1("Failed to set thread desktop\n");
        ThreadData->Status = STATUS_UNSUCCESSFUL;
        SetEvent(ThreadData->Event);
        return 1;
    }

    BackgroundWnd = CreateWindowW((LPCWSTR)DESKTOP_WINDOW_ATOM,
                                  L"",
                                  WS_POPUP | WS_CLIPCHILDREN,
                                  GetSystemMetrics(SM_XVIRTUALSCREEN),
                                  GetSystemMetrics(SM_YVIRTUALSCREEN),
                                  GetSystemMetrics(SM_CXVIRTUALSCREEN),
                                  GetSystemMetrics(SM_CYVIRTUALSCREEN),
                                  NULL, NULL,
                                  (HINSTANCE)GetModuleHandleW(L"user32.dll"), // Run in win32k/user32.
                                  NULL);

    if (NULL == BackgroundWnd)
    {
        DPRINT1("Failed to create desktop background window\n");
        ThreadData->Status = STATUS_UNSUCCESSFUL;
        SetEvent(ThreadData->Event);
        return 1;
    }

    DPRINT("BackgroundWnd 0x%p\n",BackgroundWnd);

    ThreadData->Status = STATUS_SUCCESS;
    SetEvent(ThreadData->Event);

    while (GetMessageW(&msg, NULL, 0, 0))
    {
        if (msg.message == WM_QUIT)
        {
           DPRINT1("DtbgDesktopThread WM_QUIT\n");
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    DPRINT1("DtbgDesktopThread Exit\n");
    return 1;
}

CSR_API(CsrCreateDesktop)
{
    DTBG_THREAD_DATA ThreadData;
    HANDLE ThreadHandle;

    DPRINT("CsrCreateDesktop\n");

    if (!BgInitialized)
    {
        BgInitialized = TRUE;

       // if (!DtbgInit())
       //     return STATUS_UNSUCCESSFUL;
    }

    /*
     * The desktop handle we got from win32k is in
     * the scope of CSRSS so we can just use it.
     */
    ThreadData.Desktop = Request->Data.CreateDesktopRequest.DesktopHandle;

    ThreadData.Event = CreateEventW(NULL, FALSE, FALSE, NULL);

    if (NULL == ThreadData.Event)
    {
        DPRINT1("Failed to create event (error %d)\n", GetLastError());
        return STATUS_UNSUCCESSFUL;
    }

    ThreadHandle = CreateThread(NULL,
                                0,
                                DtbgDesktopThread,
                                (PVOID)&ThreadData,
                                0,
                                NULL);

    if (NULL == ThreadHandle)
    {
        CloseHandle(ThreadData.Event);
        DPRINT1("Failed to create desktop window thread.\n");
        return STATUS_UNSUCCESSFUL;
    }

    CloseHandle(ThreadHandle);

    WaitForSingleObject(ThreadData.Event, INFINITE);
    CloseHandle(ThreadData.Event);

    return ThreadData.Status;
}

CSR_API(CsrShowDesktop)
{
#if 0
    PRIVATE_NOTIFY_DESKTOP nmh;
    DPRINT("CsrShowDesktop\n");

    nmh.hdr.hwndFrom = Request->Data.ShowDesktopRequest.DesktopWindow;
    nmh.hdr.idFrom = 0;
    nmh.hdr.code = PM_SHOW_DESKTOP;

    nmh.ShowDesktop.Width = (int)Request->Data.ShowDesktopRequest.Width;
    nmh.ShowDesktop.Height = (int)Request->Data.ShowDesktopRequest.Height;

    if (SendMessageW(Request->Data.ShowDesktopRequest.DesktopWindow,
                     WM_NOTIFY,
                     (WPARAM)nmh.hdr.hwndFrom,
                     (LPARAM)&nmh))
    {
        return STATUS_UNSUCCESSFUL;
    }
#endif
    return STATUS_SUCCESS;
}

CSR_API(CsrHideDesktop)
{
#if 0
    PRIVATE_NOTIFY_DESKTOP nmh;
    DPRINT("CsrHideDesktop\n");

    nmh.hdr.hwndFrom = Request->Data.ShowDesktopRequest.DesktopWindow;
    nmh.hdr.idFrom = 0;
    nmh.hdr.code = PM_HIDE_DESKTOP;

    if (SendMessageW(Request->Data.ShowDesktopRequest.DesktopWindow,
                     WM_NOTIFY,
                     (WPARAM)nmh.hdr.hwndFrom,
                     (LPARAM)&nmh))
    {
        return STATUS_UNSUCCESSFUL;
    }
#endif
    return STATUS_SUCCESS;
}

BOOL
FASTCALL
DtbgIsDesktopVisible(VOID)
{
    VisibleDesktopWindow = GetDesktopWindow(); // DESKTOPWNDPROC

    if (VisibleDesktopWindow != NULL &&
            !IsWindowVisible(VisibleDesktopWindow))
    {
        VisibleDesktopWindow = NULL;
    }

    return VisibleDesktopWindow != NULL;
}

/* EOF */
