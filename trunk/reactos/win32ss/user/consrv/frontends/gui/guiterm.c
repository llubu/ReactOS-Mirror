/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/consrv/frontends/gui/guiterm.c
 * PURPOSE:         GUI Terminal Front-End
 * PROGRAMMERS:     G� van Geldorp
 *                  Johannes Anderwald
 *                  Jeffrey Morlan
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#include "consrv.h"
#include "include/conio.h"
#include "include/console.h"
#include "include/settings.h"
#include "guiterm.h"
#include "guisettings.h"
#include "resource.h"

#include <windowsx.h>

#define NDEBUG
#include <debug.h>

/* GUI Console Window Class name */
#define GUI_CONSOLE_WINDOW_CLASS L"ConsoleWindowClass"

#ifndef WM_APP
    #define WM_APP 0x8000
#endif
#define PM_CREATE_CONSOLE       (WM_APP + 1)
#define PM_DESTROY_CONSOLE      (WM_APP + 2)
#define PM_RESIZE_TERMINAL      (WM_APP + 3)
#define PM_CONSOLE_BEEP         (WM_APP + 4)
#define PM_CONSOLE_SET_TITLE    (WM_APP + 5)


/* Not defined in any header file */
// extern VOID WINAPI PrivateCsrssManualGuiCheck(LONG Check);
// From win32ss/user/win32csr/dllmain.c
VOID
WINAPI
PrivateCsrssManualGuiCheck(LONG Check)
{
    NtUserCallOneParam(Check, ONEPARAM_ROUTINE_CSRSS_GUICHECK);
}

/* GLOBALS ********************************************************************/

/**************************************************************\
\** Define the Console Leader Process for the console window **/
#define GWLP_CONSOLEWND_ALLOC  (2 * sizeof(LONG_PTR))
#define GWLP_CONSOLE_LEADER_PID 0
#define GWLP_CONSOLE_LEADER_TID 4

#define SetConsoleWndConsoleLeaderCID(GuiData)  \
do {                                            \
    PCONSOLE_PROCESS_DATA ProcessData;          \
    CLIENT_ID ConsoleLeaderCID;                 \
    ProcessData = CONTAINING_RECORD((GuiData)->Console->ProcessList.Blink,  \
                                    CONSOLE_PROCESS_DATA,                   \
                                    ConsoleLink);                           \
    ConsoleLeaderCID = ProcessData->Process->ClientId;                      \
    SetWindowLongPtrW((GuiData)->hWindow, GWLP_CONSOLE_LEADER_PID, (LONG_PTR)(ConsoleLeaderCID.UniqueProcess));  \
    SetWindowLongPtrW((GuiData)->hWindow, GWLP_CONSOLE_LEADER_TID, (LONG_PTR)(ConsoleLeaderCID.UniqueThread ));  \
} while (0)
/**************************************************************/

static BOOL    ConsInitialized = FALSE;
static HICON   ghDefaultIcon = NULL;
static HICON   ghDefaultIconSm = NULL;
static HCURSOR ghDefaultCursor = NULL;
static HWND    NotifyWnd = NULL;

typedef struct _GUICONSOLE_MENUITEM
{
    UINT uID;
    const struct _GUICONSOLE_MENUITEM *SubMenu;
    WORD wCmdID;
} GUICONSOLE_MENUITEM, *PGUICONSOLE_MENUITEM;

static const GUICONSOLE_MENUITEM GuiConsoleEditMenuItems[] =
{
    { IDS_MARK, NULL, ID_SYSTEM_EDIT_MARK },
    { IDS_COPY, NULL, ID_SYSTEM_EDIT_COPY },
    { IDS_PASTE, NULL, ID_SYSTEM_EDIT_PASTE },
    { IDS_SELECTALL, NULL, ID_SYSTEM_EDIT_SELECTALL },
    { IDS_SCROLL, NULL, ID_SYSTEM_EDIT_SCROLL },
    { IDS_FIND, NULL, ID_SYSTEM_EDIT_FIND },

    { 0, NULL, 0 } /* End of list */
};

static const GUICONSOLE_MENUITEM GuiConsoleMainMenuItems[] =
{
    { IDS_EDIT, GuiConsoleEditMenuItems, 0 },
    { IDS_DEFAULTS, NULL, ID_SYSTEM_DEFAULTS },
    { IDS_PROPERTIES, NULL, ID_SYSTEM_PROPERTIES },

    { 0, NULL, 0 } /* End of list */
};

/*
 * Default 16-color palette for foreground and background
 * (corresponding flags in comments).
 */
const COLORREF s_Colors[16] =
{
    RGB(0, 0, 0),       // (Black)
    RGB(0, 0, 128),     // BLUE
    RGB(0, 128, 0),     // GREEN
    RGB(0, 128, 128),   // BLUE  | GREEN
    RGB(128, 0, 0),     // RED
    RGB(128, 0, 128),   // BLUE  | RED
    RGB(128, 128, 0),   // GREEN | RED
    RGB(192, 192, 192), // BLUE  | GREEN | RED

    RGB(128, 128, 128), // (Grey)  INTENSITY
    RGB(0, 0, 255),     // BLUE  | INTENSITY
    RGB(0, 255, 0),     // GREEN | INTENSITY
    RGB(0, 255, 255),   // BLUE  | GREEN | INTENSITY
    RGB(255, 0, 0),     // RED   | INTENSITY
    RGB(255, 0, 255),   // BLUE  | RED   | INTENSITY
    RGB(255, 255, 0),   // GREEN | RED   | INTENSITY
    RGB(255, 255, 255)  // BLUE  | GREEN | RED | INTENSITY
};

/* FUNCTIONS ******************************************************************/

static VOID
GuiConsoleAppendMenuItems(HMENU hMenu,
                          const GUICONSOLE_MENUITEM *Items)
{
    UINT i = 0;
    WCHAR szMenuString[255];
    HMENU hSubMenu;

    do
    {
        if (Items[i].uID != (UINT)-1)
        {
            if (LoadStringW(ConSrvDllInstance,
                            Items[i].uID,
                            szMenuString,
                            sizeof(szMenuString) / sizeof(szMenuString[0])) > 0)
            {
                if (Items[i].SubMenu != NULL)
                {
                    hSubMenu = CreatePopupMenu();
                    if (hSubMenu != NULL)
                    {
                        GuiConsoleAppendMenuItems(hSubMenu,
                                                  Items[i].SubMenu);

                        if (!AppendMenuW(hMenu,
                                         MF_STRING | MF_POPUP,
                                         (UINT_PTR)hSubMenu,
                                         szMenuString))
                        {
                            DestroyMenu(hSubMenu);
                        }
                    }
                }
                else
                {
                    AppendMenuW(hMenu,
                                MF_STRING,
                                Items[i].wCmdID,
                                szMenuString);
                }
            }
        }
        else
        {
            AppendMenuW(hMenu,
                        MF_SEPARATOR,
                        0,
                        NULL);
        }
        i++;
    } while (!(Items[i].uID == 0 && Items[i].SubMenu == NULL && Items[i].wCmdID == 0));
}

static VOID
GuiConsoleCreateSysMenu(HWND hWnd)
{
    HMENU hMenu;
    hMenu = GetSystemMenu(hWnd, FALSE);
    if (hMenu != NULL)
    {
        GuiConsoleAppendMenuItems(hMenu, GuiConsoleMainMenuItems);
        DrawMenuBar(hWnd);
    }
}


static VOID
GuiConsoleCopy(PGUI_CONSOLE_DATA GuiData);
static VOID
GuiConsolePaste(PGUI_CONSOLE_DATA GuiData);
static VOID
GuiConsoleUpdateSelection(PCONSOLE Console, PCOORD coord);
static VOID WINAPI
GuiDrawRegion(PCONSOLE Console, SMALL_RECT* Region);
static VOID
GuiConsoleResizeWindow(PGUI_CONSOLE_DATA GuiData);


static LRESULT
GuiConsoleHandleSysMenuCommand(PGUI_CONSOLE_DATA GuiData, WPARAM wParam, LPARAM lParam)
{
    LRESULT Ret = TRUE;
    PCONSOLE Console = GuiData->Console;
    PCONSOLE_SCREEN_BUFFER ActiveBuffer;

    if (!ConSrvValidateConsoleUnsafe(Console, CONSOLE_RUNNING, TRUE))
    {
        Ret = FALSE;
        goto Quit;
    }
    ActiveBuffer = Console->ActiveBuffer;

    switch (wParam)
    {
        case ID_SYSTEM_EDIT_MARK:
        {
            LPWSTR WindowTitle = NULL;
            SIZE_T Length = 0;

            Console->dwSelectionCursor.X = 0;
            Console->dwSelectionCursor.Y = 0;
            Console->Selection.dwSelectionAnchor = Console->dwSelectionCursor;
            Console->Selection.dwFlags |= CONSOLE_SELECTION_IN_PROGRESS;
            GuiConsoleUpdateSelection(Console, &Console->Selection.dwSelectionAnchor);

            Length = Console->Title.Length + sizeof(L"Mark - ")/sizeof(WCHAR) + 1;
            WindowTitle = ConsoleAllocHeap(0, Length * sizeof(WCHAR));
            wcscpy(WindowTitle, L"Mark - ");
            wcscat(WindowTitle, Console->Title.Buffer);
            SetWindowText(GuiData->hWindow, WindowTitle);
            ConsoleFreeHeap(WindowTitle);

            break;
        }

        case ID_SYSTEM_EDIT_COPY:
            GuiConsoleCopy(GuiData);
            break;

        case ID_SYSTEM_EDIT_PASTE:
            GuiConsolePaste(GuiData);
            break;

        case ID_SYSTEM_EDIT_SELECTALL:
        {
            Console->Selection.dwSelectionAnchor.X = 0;
            Console->Selection.dwSelectionAnchor.Y = 0;
            Console->dwSelectionCursor.X = ActiveBuffer->ViewSize.X - 1;
            Console->dwSelectionCursor.Y = ActiveBuffer->ViewSize.Y - 1;
            GuiConsoleUpdateSelection(Console, &Console->dwSelectionCursor);
            break;
        }

        case ID_SYSTEM_EDIT_SCROLL:
            DPRINT1("Scrolling is not handled yet\n");
            break;

        case ID_SYSTEM_EDIT_FIND:
            DPRINT1("Finding is not handled yet\n");
            break;

        case ID_SYSTEM_DEFAULTS:
            GuiConsoleShowConsoleProperties(GuiData, TRUE);
            break;

        case ID_SYSTEM_PROPERTIES:
            GuiConsoleShowConsoleProperties(GuiData, FALSE);
            break;

        default:
            Ret = FALSE;
            break;
    }

    LeaveCriticalSection(&Console->Lock);

Quit:
    if (!Ret)
        Ret = DefWindowProcW(GuiData->hWindow, WM_SYSCOMMAND, wParam, lParam);

    return Ret;
}

static PGUI_CONSOLE_DATA
GuiGetGuiData(HWND hWnd)
{
    /* This function ensures that the console pointer is not NULL */
    PGUI_CONSOLE_DATA GuiData = (PGUI_CONSOLE_DATA)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
    return ( ((GuiData == NULL) || (GuiData->hWindow == hWnd && GuiData->Console != NULL)) ? GuiData : NULL );
}

VOID
GuiConsoleMoveWindow(PGUI_CONSOLE_DATA GuiData)
{
    /* Move the window if needed (not positioned by the system) */
    if (!GuiData->GuiInfo.AutoPosition)
    {
        SetWindowPos(GuiData->hWindow,
                     NULL,
                     GuiData->GuiInfo.WindowOrigin.x,
                     GuiData->GuiInfo.WindowOrigin.y,
                     0,
                     0,
                     SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
    }
}

static VOID
GuiConsoleResizeWindow(PGUI_CONSOLE_DATA GuiData)
{
    PCONSOLE Console = GuiData->Console;
    PCONSOLE_SCREEN_BUFFER Buff = Console->ActiveBuffer;
    SCROLLINFO sInfo;

    DWORD Width, Height;
    UINT  WidthUnit, HeightUnit;

    if (GetType(Buff) == TEXTMODE_BUFFER)
    {
        WidthUnit  = GuiData->CharWidth ;
        HeightUnit = GuiData->CharHeight;
    }
    else /* if (GetType(Buff) == GRAPHICS_BUFFER) */
    {
        WidthUnit  = 1;
        HeightUnit = 1;
    }

    Width  = Buff->ViewSize.X * WidthUnit  +
             2 * (GetSystemMetrics(SM_CXFRAME) + GetSystemMetrics(SM_CXEDGE));
    Height = Buff->ViewSize.Y * HeightUnit +
             2 * (GetSystemMetrics(SM_CYFRAME) + GetSystemMetrics(SM_CYEDGE)) + GetSystemMetrics(SM_CYCAPTION);

    /* Set scrollbar sizes */
    sInfo.cbSize = sizeof(SCROLLINFO);
    sInfo.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    sInfo.nMin = 0;
    if (Buff->ScreenBufferSize.Y > Buff->ViewSize.Y)
    {
        sInfo.nMax  = Buff->ScreenBufferSize.Y - 1;
        sInfo.nPage = Buff->ViewSize.Y;
        sInfo.nPos  = Buff->ViewOrigin.Y;
        SetScrollInfo(GuiData->hWindow, SB_VERT, &sInfo, TRUE);
        Width += GetSystemMetrics(SM_CXVSCROLL);
        ShowScrollBar(GuiData->hWindow, SB_VERT, TRUE);
    }
    else
    {
        ShowScrollBar(GuiData->hWindow, SB_VERT, FALSE);
    }

    if (Buff->ScreenBufferSize.X > Buff->ViewSize.X)
    {
        sInfo.nMax  = Buff->ScreenBufferSize.X - 1;
        sInfo.nPage = Buff->ViewSize.X;
        sInfo.nPos  = Buff->ViewOrigin.X;
        SetScrollInfo(GuiData->hWindow, SB_HORZ, &sInfo, TRUE);
        Height += GetSystemMetrics(SM_CYHSCROLL);
        ShowScrollBar(GuiData->hWindow, SB_HORZ, TRUE);
    }
    else
    {
        ShowScrollBar(GuiData->hWindow, SB_HORZ, FALSE);
    }

    /* Resize the window  */
    SetWindowPos(GuiData->hWindow, NULL, 0, 0, Width, Height,
                 SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOCOPYBITS);
    // NOTE: The SWP_NOCOPYBITS flag can be replaced by a subsequent call
    // to: InvalidateRect(GuiData->hWindow, NULL, TRUE);
}

static VOID
GuiConsoleSwitchFullScreen(PGUI_CONSOLE_DATA GuiData)
{
    PCONSOLE Console = GuiData->Console;
    // DEVMODE dmScreenSettings;

    if (!ConSrvValidateConsoleUnsafe(Console, CONSOLE_RUNNING, TRUE)) return;

    /* Switch to full-screen or to windowed mode */
    GuiData->GuiInfo.FullScreen = !GuiData->GuiInfo.FullScreen;
    DPRINT1("GuiConsoleSwitchFullScreen - Switch to %s ...\n",
            (GuiData->GuiInfo.FullScreen ? "full-screen" : "windowed mode"));

    // TODO: Change window appearance.
    // See:
    // http://stackoverflow.com/questions/2382464/win32-full-screen-and-hiding-taskbar
    // http://stackoverflow.com/questions/3549148/fullscreen-management-with-winapi
    // http://blogs.msdn.com/b/oldnewthing/archive/2010/04/12/9994016.aspx
    // http://stackoverflow.com/questions/1400654/how-do-i-put-my-opengl-app-into-fullscreen-mode
    // http://nehe.gamedev.net/tutorial/creating_an_opengl_window_win32/13001/
#if 0
    if (GuiData->GuiInfo.FullScreen)
    {
        memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));
        dmScreenSettings.dmSize       = sizeof(dmScreenSettings);
        dmScreenSettings.dmDisplayFixedOutput = DMDFO_CENTER; // DMDFO_STRETCH // DMDFO_DEFAULT
        dmScreenSettings.dmPelsWidth  = 640; // Console->ActiveBuffer->ViewSize.X * GuiData->CharWidth;
        dmScreenSettings.dmPelsHeight = 480; // Console->ActiveBuffer->ViewSize.Y * GuiData->CharHeight;
        dmScreenSettings.dmBitsPerPel = 32;
        dmScreenSettings.dmFields     = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
        ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN);
    }
    else
    {
    }
#endif

    LeaveCriticalSection(&Console->Lock);
}

static BOOL
GuiConsoleHandleNcCreate(HWND hWnd, LPCREATESTRUCTW Create)
{
    PGUI_CONSOLE_DATA GuiData = (PGUI_CONSOLE_DATA)Create->lpCreateParams;
    PCONSOLE Console;
    HDC Dc;
    HFONT OldFont;
    TEXTMETRICW Metrics;
    SIZE CharSize;

    DPRINT("GuiConsoleHandleNcCreate\n");

    if (NULL == GuiData)
    {
        DPRINT1("GuiConsoleNcCreate: No GUI data\n");
        return FALSE;
    }

    Console = GuiData->Console;

    GuiData->hWindow = hWnd;

    GuiData->Font = CreateFontW(LOWORD(GuiData->GuiInfo.FontSize),
                                0, // HIWORD(GuiData->GuiInfo.FontSize),
                                0,
                                TA_BASELINE,
                                GuiData->GuiInfo.FontWeight,
                                FALSE,
                                FALSE,
                                FALSE,
                                OEM_CHARSET,
                                OUT_DEFAULT_PRECIS,
                                CLIP_DEFAULT_PRECIS,
                                NONANTIALIASED_QUALITY,
                                FIXED_PITCH | GuiData->GuiInfo.FontFamily /* FF_DONTCARE */,
                                GuiData->GuiInfo.FaceName);

    if (NULL == GuiData->Font)
    {
        DPRINT1("GuiConsoleNcCreate: CreateFont failed\n");
        GuiData->hWindow = NULL;
        SetEvent(GuiData->hGuiInitEvent);
        return FALSE;
    }
    Dc = GetDC(GuiData->hWindow);
    if (NULL == Dc)
    {
        DPRINT1("GuiConsoleNcCreate: GetDC failed\n");
        DeleteObject(GuiData->Font);
        GuiData->hWindow = NULL;
        SetEvent(GuiData->hGuiInitEvent);
        return FALSE;
    }
    OldFont = SelectObject(Dc, GuiData->Font);
    if (NULL == OldFont)
    {
        DPRINT1("GuiConsoleNcCreate: SelectObject failed\n");
        ReleaseDC(GuiData->hWindow, Dc);
        DeleteObject(GuiData->Font);
        GuiData->hWindow = NULL;
        SetEvent(GuiData->hGuiInitEvent);
        return FALSE;
    }
    if (!GetTextMetricsW(Dc, &Metrics))
    {
        DPRINT1("GuiConsoleNcCreate: GetTextMetrics failed\n");
        SelectObject(Dc, OldFont);
        ReleaseDC(GuiData->hWindow, Dc);
        DeleteObject(GuiData->Font);
        GuiData->hWindow = NULL;
        SetEvent(GuiData->hGuiInitEvent);
        return FALSE;
    }
    GuiData->CharWidth  = Metrics.tmMaxCharWidth;
    GuiData->CharHeight = Metrics.tmHeight + Metrics.tmExternalLeading;

    /* Measure real char width more precisely if possible. */
    if (GetTextExtentPoint32W(Dc, L"R", 1, &CharSize))
        GuiData->CharWidth = CharSize.cx;

    SelectObject(Dc, OldFont);

    ReleaseDC(GuiData->hWindow, Dc);

    // FIXME: Keep these instructions here ? ///////////////////////////////////
    Console->ActiveBuffer->CursorBlinkOn = TRUE;
    Console->ActiveBuffer->ForceCursorOff = FALSE;
    ////////////////////////////////////////////////////////////////////////////

    SetWindowLongPtrW(GuiData->hWindow, GWLP_USERDATA, (DWORD_PTR)GuiData);

    SetTimer(GuiData->hWindow, CONGUI_UPDATE_TIMER, CONGUI_UPDATE_TIME, NULL);
    GuiConsoleCreateSysMenu(GuiData->hWindow);

    DPRINT("GuiConsoleHandleNcCreate - setting start event\n");
    SetEvent(GuiData->hGuiInitEvent);

    return (BOOL)DefWindowProcW(GuiData->hWindow, WM_NCCREATE, 0, (LPARAM)Create);
}

static VOID
SmallRectToRect(PGUI_CONSOLE_DATA GuiData, PRECT Rect, PSMALL_RECT SmallRect)
{
    PCONSOLE Console = GuiData->Console;
    PCONSOLE_SCREEN_BUFFER Buffer = Console->ActiveBuffer;
    UINT WidthUnit, HeightUnit;

    if (GetType(Buffer) == TEXTMODE_BUFFER)
    {
        WidthUnit  = GuiData->CharWidth ;
        HeightUnit = GuiData->CharHeight;
    }
    else /* if (GetType(Buffer) == GRAPHICS_BUFFER) */
    {
        WidthUnit  = 1;
        HeightUnit = 1;
    }

    Rect->left   = (SmallRect->Left       - Buffer->ViewOrigin.X) * WidthUnit ;
    Rect->top    = (SmallRect->Top        - Buffer->ViewOrigin.Y) * HeightUnit;
    Rect->right  = (SmallRect->Right  + 1 - Buffer->ViewOrigin.X) * WidthUnit ;
    Rect->bottom = (SmallRect->Bottom + 1 - Buffer->ViewOrigin.Y) * HeightUnit;
}

static VOID
GuiConsoleUpdateSelection(PCONSOLE Console, PCOORD coord)
{
    PGUI_CONSOLE_DATA GuiData = Console->TermIFace.Data;
    RECT oldRect, newRect;

    SmallRectToRect(GuiData, &oldRect, &Console->Selection.srSelection);

    if (coord != NULL)
    {
        SMALL_RECT rc;
        /* exchange left/top with right/bottom if required */
        rc.Left   = min(Console->Selection.dwSelectionAnchor.X, coord->X);
        rc.Top    = min(Console->Selection.dwSelectionAnchor.Y, coord->Y);
        rc.Right  = max(Console->Selection.dwSelectionAnchor.X, coord->X);
        rc.Bottom = max(Console->Selection.dwSelectionAnchor.Y, coord->Y);

        SmallRectToRect(GuiData, &newRect, &rc);

        if (Console->Selection.dwFlags & CONSOLE_SELECTION_NOT_EMPTY)
        {
            if (memcmp(&rc, &Console->Selection.srSelection, sizeof(SMALL_RECT)) != 0)
            {
                HRGN rgn1, rgn2;

                /* calculate the region that needs to be updated */
                if ((rgn1 = CreateRectRgnIndirect(&oldRect)))
                {
                    if ((rgn2 = CreateRectRgnIndirect(&newRect)))
                    {
                        if (CombineRgn(rgn1, rgn2, rgn1, RGN_XOR) != ERROR)
                        {
                            InvalidateRgn(GuiData->hWindow, rgn1, FALSE);
                        }
                        DeleteObject(rgn2);
                    }
                    DeleteObject(rgn1);
                }
            }
        }
        else
        {
            InvalidateRect(GuiData->hWindow, &newRect, FALSE);
        }
        Console->Selection.dwFlags |= CONSOLE_SELECTION_NOT_EMPTY;
        Console->Selection.srSelection = rc;
        Console->dwSelectionCursor = *coord;
        ConioPause(Console, PAUSED_FROM_SELECTION);
    }
    else
    {
        /* clear the selection */
        if (Console->Selection.dwFlags & CONSOLE_SELECTION_NOT_EMPTY)
        {
            InvalidateRect(GuiData->hWindow, &oldRect, FALSE);
        }
        Console->Selection.dwFlags = CONSOLE_NO_SELECTION;
        ConioUnpause(Console, PAUSED_FROM_SELECTION);
    }
}


VOID
GuiPaintTextModeBuffer(PTEXTMODE_SCREEN_BUFFER Buffer,
                       PGUI_CONSOLE_DATA GuiData,
                       HDC hDC,
                       PRECT rc);
VOID
GuiPaintGraphicsBuffer(PGRAPHICS_SCREEN_BUFFER Buffer,
                       PGUI_CONSOLE_DATA GuiData,
                       HDC hDC,
                       PRECT rc);

static VOID
GuiConsoleHandlePaint(PGUI_CONSOLE_DATA GuiData)
{
    BOOL Success = TRUE;
    PCONSOLE Console = GuiData->Console;
    PCONSOLE_SCREEN_BUFFER ActiveBuffer;
    HDC hDC;
    PAINTSTRUCT ps;

    if (!ConSrvValidateConsoleUnsafe(Console, CONSOLE_RUNNING, TRUE))
    {
        Success = FALSE;
        goto Quit;
    }
    ActiveBuffer = Console->ActiveBuffer;

    hDC = BeginPaint(GuiData->hWindow, &ps);
    if (hDC != NULL &&
        ps.rcPaint.left < ps.rcPaint.right &&
        ps.rcPaint.top < ps.rcPaint.bottom)
    {
        EnterCriticalSection(&GuiData->Lock);

        if (GetType(ActiveBuffer) == TEXTMODE_BUFFER)
        {
            GuiPaintTextModeBuffer((PTEXTMODE_SCREEN_BUFFER)ActiveBuffer,
                                   GuiData, hDC, &ps.rcPaint);
        }
        else /* if (GetType(ActiveBuffer) == GRAPHICS_BUFFER) */
        {
            GuiPaintGraphicsBuffer((PGRAPHICS_SCREEN_BUFFER)ActiveBuffer,
                                   GuiData, hDC, &ps.rcPaint);
        }

        if (Console->Selection.dwFlags & CONSOLE_SELECTION_NOT_EMPTY)
        {
            RECT rc;
            SmallRectToRect(GuiData, &rc, &Console->Selection.srSelection);

            /* invert the selection */
            if (IntersectRect(&rc, &ps.rcPaint, &rc))
            {
                PatBlt(hDC,
                       rc.left,
                       rc.top,
                       rc.right - rc.left,
                       rc.bottom - rc.top,
                       DSTINVERT);
            }
        }

        LeaveCriticalSection(&GuiData->Lock);
    }
    EndPaint(GuiData->hWindow, &ps);

Quit:
    if (Success)
        LeaveCriticalSection(&Console->Lock);
    else
        DefWindowProcW(GuiData->hWindow, WM_PAINT, 0, 0);

    return;
}

static VOID
GuiConsoleHandleKey(PGUI_CONSOLE_DATA GuiData, UINT msg, WPARAM wParam, LPARAM lParam)
{
    PCONSOLE Console = GuiData->Console;
    PCONSOLE_SCREEN_BUFFER ActiveBuffer;
    MSG Message;

    if (!ConSrvValidateConsoleUnsafe(Console, CONSOLE_RUNNING, TRUE)) return;

    ActiveBuffer = Console->ActiveBuffer;

    if ( (Console->Selection.dwFlags & CONSOLE_SELECTION_IN_PROGRESS) &&
        ((Console->Selection.dwFlags & CONSOLE_MOUSE_SELECTION) == 0) )
    {
        BOOL Interpreted = FALSE;

        /* Selection with keyboard */
        if (msg == WM_KEYDOWN)
        {
            BOOL MajPressed = (GetKeyState(VK_SHIFT) & 0x8000);

            switch (wParam)
            {
                case VK_LEFT:
                {
                    Interpreted = TRUE;
                    if (Console->dwSelectionCursor.X > 0)
                        Console->dwSelectionCursor.X--;

                    break;
                }

                case VK_RIGHT:
                {
                    Interpreted = TRUE;
                    if (Console->dwSelectionCursor.X < ActiveBuffer->ScreenBufferSize.X - 1)
                        Console->dwSelectionCursor.X++;

                    break;
                }

                case VK_UP:
                {
                    Interpreted = TRUE;
                    if (Console->dwSelectionCursor.Y > 0)
                        Console->dwSelectionCursor.Y--;

                    break;
                }

                case VK_DOWN:
                {
                    Interpreted = TRUE;
                    if (Console->dwSelectionCursor.Y < ActiveBuffer->ScreenBufferSize.Y - 1)
                        Console->dwSelectionCursor.Y++;

                    break;
                }

                case VK_HOME:
                {
                    Interpreted = TRUE;
                    Console->dwSelectionCursor.X = 0;
                    Console->dwSelectionCursor.Y = 0;
                    break;
                }

                case VK_END:
                {
                    Interpreted = TRUE;
                    Console->dwSelectionCursor.Y = ActiveBuffer->ScreenBufferSize.Y - 1;
                    break;
                }

                case VK_PRIOR:
                {
                    Interpreted = TRUE;
                    Console->dwSelectionCursor.Y -= ActiveBuffer->ViewSize.Y;
                    if (Console->dwSelectionCursor.Y < 0)
                        Console->dwSelectionCursor.Y = 0;

                    break;
                }

                case VK_NEXT:
                {
                    Interpreted = TRUE;
                    Console->dwSelectionCursor.Y += ActiveBuffer->ViewSize.Y;
                    if (Console->dwSelectionCursor.Y >= ActiveBuffer->ScreenBufferSize.Y)
                        Console->dwSelectionCursor.Y  = ActiveBuffer->ScreenBufferSize.Y - 1;

                    break;
                }

                default:
                    break;
            }

            if (Interpreted)
            {
                if (!MajPressed)
                    Console->Selection.dwSelectionAnchor = Console->dwSelectionCursor;

                GuiConsoleUpdateSelection(Console, &Console->dwSelectionCursor);
            }
        }
    }
    else
    {
        Message.hwnd = GuiData->hWindow;
        Message.message = msg;
        Message.wParam = wParam;
        Message.lParam = lParam;

        if (msg == WM_KEYDOWN)
        {
            /* If we are in selection mode (with mouse), clear the selection */
            GuiConsoleUpdateSelection(Console, NULL);
            SetWindowText(GuiData->hWindow, Console->Title.Buffer);
        }

        ConioProcessKey(Console, &Message);
    }

    LeaveCriticalSection(&Console->Lock);
}

static VOID
GuiInvalidateCell(PCONSOLE Console, SHORT x, SHORT y)
{
    SMALL_RECT CellRect = { x, y, x, y };
    GuiDrawRegion(Console, &CellRect);
}

static VOID
GuiConsoleHandleTimer(PGUI_CONSOLE_DATA GuiData)
{
    PCONSOLE Console = GuiData->Console;
    PCONSOLE_SCREEN_BUFFER Buff;

    SetTimer(GuiData->hWindow, CONGUI_UPDATE_TIMER, CURSOR_BLINK_TIME, NULL);

    if (!ConSrvValidateConsoleUnsafe(Console, CONSOLE_RUNNING, TRUE)) return;

    Buff = Console->ActiveBuffer;

    if (GetType(Buff) == TEXTMODE_BUFFER)
    {
        GuiInvalidateCell(Console, Buff->CursorPosition.X, Buff->CursorPosition.Y);
        Buff->CursorBlinkOn = !Buff->CursorBlinkOn;

        if ((GuiData->OldCursor.x != Buff->CursorPosition.X) || (GuiData->OldCursor.y != Buff->CursorPosition.Y))
        {
            SCROLLINFO xScroll;
            int OldScrollX = -1, OldScrollY = -1;
            int NewScrollX = -1, NewScrollY = -1;

            xScroll.cbSize = sizeof(SCROLLINFO);
            xScroll.fMask = SIF_POS;
            // Capture the original position of the scroll bars and save them.
            if (GetScrollInfo(GuiData->hWindow, SB_HORZ, &xScroll)) OldScrollX = xScroll.nPos;
            if (GetScrollInfo(GuiData->hWindow, SB_VERT, &xScroll)) OldScrollY = xScroll.nPos;

            // If we successfully got the info for the horizontal scrollbar
            if (OldScrollX >= 0)
            {
                if ((Buff->CursorPosition.X < Buff->ViewOrigin.X) || (Buff->CursorPosition.X >= (Buff->ViewOrigin.X + Buff->ViewSize.X)))
                {
                    // Handle the horizontal scroll bar
                    if (Buff->CursorPosition.X >= Buff->ViewSize.X) NewScrollX = Buff->CursorPosition.X - Buff->ViewSize.X + 1;
                    else NewScrollX = 0;
                }
                else
                {
                    NewScrollX = OldScrollX;
                }
            }
            // If we successfully got the info for the vertical scrollbar
            if (OldScrollY >= 0)
            {
                if ((Buff->CursorPosition.Y < Buff->ViewOrigin.Y) || (Buff->CursorPosition.Y >= (Buff->ViewOrigin.Y + Buff->ViewSize.Y)))
                {
                    // Handle the vertical scroll bar
                    if (Buff->CursorPosition.Y >= Buff->ViewSize.Y) NewScrollY = Buff->CursorPosition.Y - Buff->ViewSize.Y + 1;
                    else NewScrollY = 0;
                }
                else
                {
                    NewScrollY = OldScrollY;
                }
            }

            // Adjust scroll bars and refresh the window if the cursor has moved outside the visible area
            // NOTE: OldScroll# and NewScroll# will both be -1 (initial value) if the info for the respective scrollbar
            //       was not obtained successfully in the previous steps. This means their difference is 0 (no scrolling)
            //       and their associated scrollbar is left alone.
            if ((OldScrollX != NewScrollX) || (OldScrollY != NewScrollY))
            {
                Buff->ViewOrigin.X = NewScrollX;
                Buff->ViewOrigin.Y = NewScrollY;
                ScrollWindowEx(GuiData->hWindow,
                               (OldScrollX - NewScrollX) * GuiData->CharWidth,
                               (OldScrollY - NewScrollY) * GuiData->CharHeight,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               SW_INVALIDATE);
                if (NewScrollX >= 0)
                {
                    xScroll.nPos = NewScrollX;
                    SetScrollInfo(GuiData->hWindow, SB_HORZ, &xScroll, TRUE);
                }
                if (NewScrollY >= 0)
                {
                    xScroll.nPos = NewScrollY;
                    SetScrollInfo(GuiData->hWindow, SB_VERT, &xScroll, TRUE);
                }
                UpdateWindow(GuiData->hWindow);
                GuiData->OldCursor.x = Buff->CursorPosition.X;
                GuiData->OldCursor.y = Buff->CursorPosition.Y;
            }
        }
    }
    else /* if (GetType(Buff) == GRAPHICS_BUFFER) */
    {
    }

    LeaveCriticalSection(&Console->Lock);
}

static VOID
GuiConsoleHandleClose(PGUI_CONSOLE_DATA GuiData)
{
    PCONSOLE Console = GuiData->Console;
    if (!ConSrvValidateConsoleUnsafe(Console, CONSOLE_RUNNING, TRUE)) return;

    /*
     * FIXME: Windows will wait up to 5 seconds for the thread to exit.
     * We shouldn't wait here, though, since the console lock is entered.
     * A copy of the thread list probably needs to be made.
     */
    ConSrvConsoleProcessCtrlEvent(Console, 0, CTRL_CLOSE_EVENT);

    LeaveCriticalSection(&Console->Lock);
}

static LRESULT
GuiConsoleHandleNcDestroy(HWND hWnd)
{
    // PGUI_CONSOLE_DATA GuiData;

    KillTimer(hWnd, CONGUI_UPDATE_TIMER);
    GetSystemMenu(hWnd, TRUE);

    /* Free the GuiData registration */
    SetWindowLongPtrW(hWnd, GWLP_USERDATA, (DWORD_PTR)NULL);
    // GuiData->hWindow = NULL;

    // return 0;
    return DefWindowProcW(hWnd, WM_NCDESTROY, 0, 0);
}

static COORD
PointToCoord(PGUI_CONSOLE_DATA GuiData, LPARAM lParam)
{
    PCONSOLE Console = GuiData->Console;
    PCONSOLE_SCREEN_BUFFER Buffer = Console->ActiveBuffer;
    COORD Coord;
    UINT  WidthUnit, HeightUnit;

    if (GetType(Buffer) == TEXTMODE_BUFFER)
    {
        WidthUnit  = GuiData->CharWidth ;
        HeightUnit = GuiData->CharHeight;
    }
    else /* if (GetType(Buffer) == GRAPHICS_BUFFER) */
    {
        WidthUnit  = 1;
        HeightUnit = 1;
    }

    Coord.X = Buffer->ViewOrigin.X + ((SHORT)LOWORD(lParam) / (int)WidthUnit );
    Coord.Y = Buffer->ViewOrigin.Y + ((SHORT)HIWORD(lParam) / (int)HeightUnit);

    /* Clip coordinate to ensure it's inside buffer */
    if (Coord.X < 0)
        Coord.X = 0;
    else if (Coord.X >= Buffer->ScreenBufferSize.X)
        Coord.X = Buffer->ScreenBufferSize.X - 1;

    if (Coord.Y < 0)
        Coord.Y = 0;
    else if (Coord.Y >= Buffer->ScreenBufferSize.Y)
        Coord.Y = Buffer->ScreenBufferSize.Y - 1;

    return Coord;
}

static LRESULT
GuiConsoleHandleMouse(PGUI_CONSOLE_DATA GuiData, UINT msg, WPARAM wParam, LPARAM lParam)
{
    BOOL Err = FALSE;
    PCONSOLE Console = GuiData->Console;

    if (GuiData->IgnoreNextMouseSignal)
    {
        if (msg != WM_LBUTTONDOWN &&
            msg != WM_MBUTTONDOWN &&
            msg != WM_RBUTTONDOWN)
        {
            /*
             * If this mouse signal is not a button-down action,
             * then it is the last signal being ignored.
             */
            GuiData->IgnoreNextMouseSignal = FALSE;
        }
        else
        {
            /*
             * This mouse signal is a button-down action.
             * Ignore it and perform default action.
             */
            Err = TRUE;
        }
        goto Quit;
    }

    if (!ConSrvValidateConsoleUnsafe(Console, CONSOLE_RUNNING, TRUE))
    {
        Err = TRUE;
        goto Quit;
    }

    if ( (Console->Selection.dwFlags & CONSOLE_SELECTION_IN_PROGRESS) ||
         (Console->QuickEdit) )
    {
        switch (msg)
        {
            case WM_LBUTTONDOWN:
            {
                LPWSTR WindowTitle = NULL;
                SIZE_T Length = 0;

                Console->Selection.dwSelectionAnchor = PointToCoord(GuiData, lParam);
                SetCapture(GuiData->hWindow);
                Console->Selection.dwFlags |= CONSOLE_SELECTION_IN_PROGRESS | CONSOLE_MOUSE_SELECTION | CONSOLE_MOUSE_DOWN;
                GuiConsoleUpdateSelection(Console, &Console->Selection.dwSelectionAnchor);

                Length = Console->Title.Length + sizeof(L"Selection - ")/sizeof(WCHAR) + 1;
                WindowTitle = ConsoleAllocHeap(0, Length * sizeof(WCHAR));
                wcscpy(WindowTitle, L"Selection - ");
                wcscat(WindowTitle, Console->Title.Buffer);
                SetWindowText(GuiData->hWindow, WindowTitle);
                ConsoleFreeHeap(WindowTitle);

                break;
            }

            case WM_LBUTTONUP:
            {
                COORD c;

                if (!(Console->Selection.dwFlags & CONSOLE_MOUSE_DOWN)) break;

                c = PointToCoord(GuiData, lParam);
                Console->Selection.dwFlags &= ~CONSOLE_MOUSE_DOWN;
                GuiConsoleUpdateSelection(Console, &c);
                ReleaseCapture();

                break;
            }

            case WM_LBUTTONDBLCLK:
            {
                DPRINT1("Handle left-double-click for selecting a word\n");
                break;
            }

            case WM_RBUTTONDOWN:
            case WM_RBUTTONDBLCLK:
            {
                if (!(Console->Selection.dwFlags & CONSOLE_SELECTION_NOT_EMPTY))
                {
                    GuiConsolePaste(GuiData);
                }
                else
                {
                    GuiConsoleCopy(GuiData);

                    /* Clear the selection */
                    GuiConsoleUpdateSelection(Console, NULL);
                    SetWindowText(GuiData->hWindow, Console->Title.Buffer);
                }

                GuiData->IgnoreNextMouseSignal = TRUE;
                break;
            }

            case WM_MOUSEMOVE:
            {
                COORD c;

                if (!(wParam & MK_LBUTTON)) break;
                if (!(Console->Selection.dwFlags & CONSOLE_MOUSE_DOWN)) break;

                c = PointToCoord(GuiData, lParam); /* TODO: Scroll buffer to bring c into view */
                GuiConsoleUpdateSelection(Console, &c);

                break;
            }

            default:
                Err = TRUE;
                break;
        }
    }
    else if (Console->InputBuffer.Mode & ENABLE_MOUSE_INPUT)
    {
        INPUT_RECORD er;
        WORD  wKeyState         = GET_KEYSTATE_WPARAM(wParam);
        DWORD dwButtonState     = 0;
        DWORD dwControlKeyState = 0;
        DWORD dwEventFlags      = 0;

        switch (msg)
        {
            case WM_LBUTTONDOWN:
                SetCapture(GuiData->hWindow);
                dwButtonState = FROM_LEFT_1ST_BUTTON_PRESSED;
                dwEventFlags  = 0;
                break;

            case WM_MBUTTONDOWN:
                SetCapture(GuiData->hWindow);
                dwButtonState = FROM_LEFT_2ND_BUTTON_PRESSED;
                dwEventFlags  = 0;
                break;

            case WM_RBUTTONDOWN:
                SetCapture(GuiData->hWindow);
                dwButtonState = RIGHTMOST_BUTTON_PRESSED;
                dwEventFlags  = 0;
                break;

            case WM_LBUTTONUP:
                ReleaseCapture();
                dwButtonState = 0;
                dwEventFlags  = 0;
                break;

            case WM_MBUTTONUP:
                ReleaseCapture();
                dwButtonState = 0;
                dwEventFlags  = 0;
                break;

            case WM_RBUTTONUP:
                ReleaseCapture();
                dwButtonState = 0;
                dwEventFlags  = 0;
                break;

            case WM_LBUTTONDBLCLK:
                dwButtonState = FROM_LEFT_1ST_BUTTON_PRESSED;
                dwEventFlags  = DOUBLE_CLICK;
                break;

            case WM_MBUTTONDBLCLK:
                dwButtonState = FROM_LEFT_2ND_BUTTON_PRESSED;
                dwEventFlags  = DOUBLE_CLICK;
                break;

            case WM_RBUTTONDBLCLK:
                dwButtonState = RIGHTMOST_BUTTON_PRESSED;
                dwEventFlags  = DOUBLE_CLICK;
                break;

            case WM_MOUSEMOVE:
                dwButtonState = 0;
                dwEventFlags  = MOUSE_MOVED;
                break;

            case WM_MOUSEWHEEL:
                dwButtonState = GET_WHEEL_DELTA_WPARAM(wParam) << 16;
                dwEventFlags  = MOUSE_WHEELED;
                break;

            case WM_MOUSEHWHEEL:
                dwButtonState = GET_WHEEL_DELTA_WPARAM(wParam) << 16;
                dwEventFlags  = MOUSE_HWHEELED;
                break;

            default:
                Err = TRUE;
                break;
        }

        if (!Err)
        {
            if (wKeyState & MK_LBUTTON)
                dwButtonState |= FROM_LEFT_1ST_BUTTON_PRESSED;
            if (wKeyState & MK_MBUTTON)
                dwButtonState |= FROM_LEFT_2ND_BUTTON_PRESSED;
            if (wKeyState & MK_RBUTTON)
                dwButtonState |= RIGHTMOST_BUTTON_PRESSED;

            if (GetKeyState(VK_RMENU) & 0x8000)
                dwControlKeyState |= RIGHT_ALT_PRESSED;
            if (GetKeyState(VK_LMENU) & 0x8000)
                dwControlKeyState |= LEFT_ALT_PRESSED;
            if (GetKeyState(VK_RCONTROL) & 0x8000)
                dwControlKeyState |= RIGHT_CTRL_PRESSED;
            if (GetKeyState(VK_LCONTROL) & 0x8000)
                dwControlKeyState |= LEFT_CTRL_PRESSED;
            if (GetKeyState(VK_SHIFT) & 0x8000)
                dwControlKeyState |= SHIFT_PRESSED;
            if (GetKeyState(VK_NUMLOCK) & 0x0001)
                dwControlKeyState |= NUMLOCK_ON;
            if (GetKeyState(VK_SCROLL) & 0x0001)
                dwControlKeyState |= SCROLLLOCK_ON;
            if (GetKeyState(VK_CAPITAL) & 0x0001)
                dwControlKeyState |= CAPSLOCK_ON;
            /* See WM_CHAR MSDN documentation for instance */
            if (lParam & 0x01000000)
                dwControlKeyState |= ENHANCED_KEY;

            er.EventType = MOUSE_EVENT;
            er.Event.MouseEvent.dwMousePosition   = PointToCoord(GuiData, lParam);
            er.Event.MouseEvent.dwButtonState     = dwButtonState;
            er.Event.MouseEvent.dwControlKeyState = dwControlKeyState;
            er.Event.MouseEvent.dwEventFlags      = dwEventFlags;

            ConioProcessInputEvent(Console, &er);
        }
    }
    else
    {
        Err = TRUE;
    }

    LeaveCriticalSection(&Console->Lock);

Quit:
    if (Err)
        return DefWindowProcW(GuiData->hWindow, msg, wParam, lParam);
    else
        return 0;
}

VOID GuiCopyFromTextModeBuffer(PTEXTMODE_SCREEN_BUFFER Buffer);
VOID GuiCopyFromGraphicsBuffer(PGRAPHICS_SCREEN_BUFFER Buffer);

static VOID
GuiConsoleCopy(PGUI_CONSOLE_DATA GuiData)
{
    if (OpenClipboard(GuiData->hWindow) == TRUE)
    {
        PCONSOLE Console = GuiData->Console;
        PCONSOLE_SCREEN_BUFFER Buffer = Console->ActiveBuffer;

        if (GetType(Buffer) == TEXTMODE_BUFFER)
        {
            GuiCopyFromTextModeBuffer((PTEXTMODE_SCREEN_BUFFER)Buffer);
        }
        else /* if (GetType(Buffer) == GRAPHICS_BUFFER) */
        {
            GuiCopyFromGraphicsBuffer((PGRAPHICS_SCREEN_BUFFER)Buffer);
        }

        CloseClipboard();
    }
}

VOID GuiPasteToTextModeBuffer(PTEXTMODE_SCREEN_BUFFER Buffer);
VOID GuiPasteToGraphicsBuffer(PGRAPHICS_SCREEN_BUFFER Buffer);

static VOID
GuiConsolePaste(PGUI_CONSOLE_DATA GuiData)
{
    if (OpenClipboard(GuiData->hWindow) == TRUE)
    {
        PCONSOLE Console = GuiData->Console;
        PCONSOLE_SCREEN_BUFFER Buffer = Console->ActiveBuffer;

        if (GetType(Buffer) == TEXTMODE_BUFFER)
        {
            GuiPasteToTextModeBuffer((PTEXTMODE_SCREEN_BUFFER)Buffer);
        }
        else /* if (GetType(Buffer) == GRAPHICS_BUFFER) */
        {
            GuiPasteToGraphicsBuffer((PGRAPHICS_SCREEN_BUFFER)Buffer);
        }

        CloseClipboard();
    }
}

static VOID
GuiConsoleGetMinMaxInfo(PGUI_CONSOLE_DATA GuiData, PMINMAXINFO minMaxInfo)
{
    PCONSOLE Console = GuiData->Console;
    PCONSOLE_SCREEN_BUFFER ActiveBuffer;
    DWORD windx, windy;
    UINT  WidthUnit, HeightUnit;

    if (!ConSrvValidateConsoleUnsafe(Console, CONSOLE_RUNNING, TRUE)) return;

    ActiveBuffer = Console->ActiveBuffer;

    if (GetType(ActiveBuffer) == TEXTMODE_BUFFER)
    {
        WidthUnit  = GuiData->CharWidth ;
        HeightUnit = GuiData->CharHeight;
    }
    else /* if (GetType(ActiveBuffer) == GRAPHICS_BUFFER) */
    {
        WidthUnit  = 1;
        HeightUnit = 1;
    }

    windx = CONGUI_MIN_WIDTH  * WidthUnit  + 2 * (GetSystemMetrics(SM_CXFRAME) + GetSystemMetrics(SM_CXEDGE));
    windy = CONGUI_MIN_HEIGHT * HeightUnit + 2 * (GetSystemMetrics(SM_CYFRAME) + GetSystemMetrics(SM_CYEDGE)) + GetSystemMetrics(SM_CYCAPTION);

    minMaxInfo->ptMinTrackSize.x = windx;
    minMaxInfo->ptMinTrackSize.y = windy;

    windx = (ActiveBuffer->ScreenBufferSize.X) * WidthUnit  + 2 * (GetSystemMetrics(SM_CXFRAME) + GetSystemMetrics(SM_CXEDGE));
    windy = (ActiveBuffer->ScreenBufferSize.Y) * HeightUnit + 2 * (GetSystemMetrics(SM_CYFRAME) + GetSystemMetrics(SM_CYEDGE)) + GetSystemMetrics(SM_CYCAPTION);

    if (ActiveBuffer->ViewSize.X < ActiveBuffer->ScreenBufferSize.X) windy += GetSystemMetrics(SM_CYHSCROLL);    // window currently has a horizontal scrollbar
    if (ActiveBuffer->ViewSize.Y < ActiveBuffer->ScreenBufferSize.Y) windx += GetSystemMetrics(SM_CXVSCROLL);    // window currently has a vertical scrollbar

    minMaxInfo->ptMaxTrackSize.x = windx;
    minMaxInfo->ptMaxTrackSize.y = windy;

    LeaveCriticalSection(&Console->Lock);
}

static VOID
GuiConsoleResize(PGUI_CONSOLE_DATA GuiData, WPARAM wParam, LPARAM lParam)
{
    PCONSOLE Console = GuiData->Console;

    if (!ConSrvValidateConsoleUnsafe(Console, CONSOLE_RUNNING, TRUE)) return;

    if ((GuiData->WindowSizeLock == FALSE) &&
        (wParam == SIZE_RESTORED || wParam == SIZE_MAXIMIZED || wParam == SIZE_MINIMIZED))
    {
        PCONSOLE_SCREEN_BUFFER Buff = Console->ActiveBuffer;
        DWORD windx, windy, charx, chary;
        UINT  WidthUnit, HeightUnit;

        if (GetType(Buff) == TEXTMODE_BUFFER)
        {
            WidthUnit  = GuiData->CharWidth ;
            HeightUnit = GuiData->CharHeight;
        }
        else /* if (GetType(Buff) == GRAPHICS_BUFFER) */
        {
            WidthUnit  = 1;
            HeightUnit = 1;
        }

        GuiData->WindowSizeLock = TRUE;

        windx = LOWORD(lParam);
        windy = HIWORD(lParam);

        // Compensate for existing scroll bars (because lParam values do not accommodate scroll bar)
        if (Buff->ViewSize.X < Buff->ScreenBufferSize.X) windy += GetSystemMetrics(SM_CYHSCROLL);    // window currently has a horizontal scrollbar
        if (Buff->ViewSize.Y < Buff->ScreenBufferSize.Y) windx += GetSystemMetrics(SM_CXVSCROLL);    // window currently has a vertical scrollbar

        charx = windx / (int)WidthUnit ;
        chary = windy / (int)HeightUnit;

        // Character alignment (round size up or down)
        if ((windx % WidthUnit ) >= (WidthUnit  / 2)) ++charx;
        if ((windy % HeightUnit) >= (HeightUnit / 2)) ++chary;

        // Compensate for added scroll bars in new window
        if (charx < Buff->ScreenBufferSize.X) windy -= GetSystemMetrics(SM_CYHSCROLL);    // new window will have a horizontal scroll bar
        if (chary < Buff->ScreenBufferSize.Y) windx -= GetSystemMetrics(SM_CXVSCROLL);    // new window will have a vertical scroll bar

        charx = windx / (int)WidthUnit ;
        chary = windy / (int)HeightUnit;

        // Character alignment (round size up or down)
        if ((windx % WidthUnit ) >= (WidthUnit  / 2)) ++charx;
        if ((windy % HeightUnit) >= (HeightUnit / 2)) ++chary;

        // Resize window
        if ((charx != Buff->ViewSize.X) || (chary != Buff->ViewSize.Y))
        {
            Buff->ViewSize.X = (charx <= Buff->ScreenBufferSize.X) ? charx : Buff->ScreenBufferSize.X;
            Buff->ViewSize.Y = (chary <= Buff->ScreenBufferSize.Y) ? chary : Buff->ScreenBufferSize.Y;
        }

        GuiConsoleResizeWindow(GuiData);

        // Adjust the start of the visible area if we are attempting to show nonexistent areas
        if ((Buff->ScreenBufferSize.X - Buff->ViewOrigin.X) < Buff->ViewSize.X) Buff->ViewOrigin.X = Buff->ScreenBufferSize.X - Buff->ViewSize.X;
        if ((Buff->ScreenBufferSize.Y - Buff->ViewOrigin.Y) < Buff->ViewSize.Y) Buff->ViewOrigin.Y = Buff->ScreenBufferSize.Y - Buff->ViewSize.Y;
        InvalidateRect(GuiData->hWindow, NULL, TRUE);

        GuiData->WindowSizeLock = FALSE;
    }

    LeaveCriticalSection(&Console->Lock);
}

/*
// HACK: This functionality is standard for general scrollbars. Don't add it by hand.

VOID
FASTCALL
GuiConsoleHandleScrollbarMenu(VOID)
{
    HMENU hMenu;

    hMenu = CreatePopupMenu();
    if (hMenu == NULL)
    {
        DPRINT("CreatePopupMenu failed\n");
        return;
    }

    //InsertItem(hMenu, MIIM_STRING, MIIM_ID | MIIM_FTYPE | MIIM_STRING, 0, NULL, IDS_SCROLLHERE);
    //InsertItem(hMenu, MFT_SEPARATOR, MIIM_FTYPE, 0, NULL, -1);
    //InsertItem(hMenu, MIIM_STRING, MIIM_ID | MIIM_FTYPE | MIIM_STRING, 0, NULL, IDS_SCROLLTOP);
    //InsertItem(hMenu, MIIM_STRING, MIIM_ID | MIIM_FTYPE | MIIM_STRING, 0, NULL, IDS_SCROLLBOTTOM);
    //InsertItem(hMenu, MFT_SEPARATOR, MIIM_FTYPE, 0, NULL, -1);
    //InsertItem(hMenu, MIIM_STRING, MIIM_ID | MIIM_FTYPE | MIIM_STRING, 0, NULL, IDS_SCROLLPAGE_UP);
    //InsertItem(hMenu, MIIM_STRING, MIIM_ID | MIIM_FTYPE | MIIM_STRING, 0, NULL, IDS_SCROLLPAGE_DOWN);
    //InsertItem(hMenu, MFT_SEPARATOR, MIIM_FTYPE, 0, NULL, -1);
    //InsertItem(hMenu, MIIM_STRING, MIIM_ID | MIIM_FTYPE | MIIM_STRING, 0, NULL, IDS_SCROLLUP);
    //InsertItem(hMenu, MIIM_STRING, MIIM_ID | MIIM_FTYPE | MIIM_STRING, 0, NULL, IDS_SCROLLDOWN);
}
*/

static LRESULT
GuiConsoleHandleScroll(PGUI_CONSOLE_DATA GuiData, UINT uMsg, WPARAM wParam)
{
    PCONSOLE Console = GuiData->Console;
    PCONSOLE_SCREEN_BUFFER Buff;
    SCROLLINFO sInfo;
    int fnBar;
    int old_pos, Maximum;
    PSHORT pShowXY;

    if (!ConSrvValidateConsoleUnsafe(Console, CONSOLE_RUNNING, TRUE)) return 0;

    Buff = Console->ActiveBuffer;

    if (uMsg == WM_HSCROLL)
    {
        fnBar = SB_HORZ;
        Maximum = Buff->ScreenBufferSize.X - Buff->ViewSize.X;
        pShowXY = &Buff->ViewOrigin.X;
    }
    else
    {
        fnBar = SB_VERT;
        Maximum = Buff->ScreenBufferSize.Y - Buff->ViewSize.Y;
        pShowXY = &Buff->ViewOrigin.Y;
    }

    /* set scrollbar sizes */
    sInfo.cbSize = sizeof(SCROLLINFO);
    sInfo.fMask = SIF_RANGE | SIF_POS | SIF_PAGE | SIF_TRACKPOS;

    if (!GetScrollInfo(GuiData->hWindow, fnBar, &sInfo)) goto Quit;

    old_pos = sInfo.nPos;

    switch (LOWORD(wParam))
    {
        case SB_LINELEFT:
            sInfo.nPos -= 1;
            break;

        case SB_LINERIGHT:
            sInfo.nPos += 1;
            break;

        case SB_PAGELEFT:
            sInfo.nPos -= sInfo.nPage;
            break;

        case SB_PAGERIGHT:
            sInfo.nPos += sInfo.nPage;
            break;

        case SB_THUMBTRACK:
            sInfo.nPos = sInfo.nTrackPos;
            ConioPause(Console, PAUSED_FROM_SCROLLBAR);
            break;

        case SB_THUMBPOSITION:
            ConioUnpause(Console, PAUSED_FROM_SCROLLBAR);
            break;

        case SB_TOP:
            sInfo.nPos = sInfo.nMin;
            break;

        case SB_BOTTOM:
            sInfo.nPos = sInfo.nMax;
            break;

        default:
            break;
    }

    sInfo.nPos = max(sInfo.nPos, 0);
    sInfo.nPos = min(sInfo.nPos, Maximum);

    if (old_pos != sInfo.nPos)
    {
        USHORT OldX = Buff->ViewOrigin.X;
        USHORT OldY = Buff->ViewOrigin.Y;
        UINT   WidthUnit, HeightUnit;

        *pShowXY = sInfo.nPos;

        if (GetType(Buff) == TEXTMODE_BUFFER)
        {
            WidthUnit  = GuiData->CharWidth ;
            HeightUnit = GuiData->CharHeight;
        }
        else /* if (GetType(Buff) == GRAPHICS_BUFFER) */
        {
            WidthUnit  = 1;
            HeightUnit = 1;
        }

        ScrollWindowEx(GuiData->hWindow,
                       (OldX - Buff->ViewOrigin.X) * WidthUnit ,
                       (OldY - Buff->ViewOrigin.Y) * HeightUnit,
                       NULL,
                       NULL,
                       NULL,
                       NULL,
                       SW_INVALIDATE);

        sInfo.fMask = SIF_POS;
        SetScrollInfo(GuiData->hWindow, fnBar, &sInfo, TRUE);

        UpdateWindow(GuiData->hWindow);
    }

Quit:
    LeaveCriticalSection(&Console->Lock);
    return 0;
}

static LRESULT CALLBACK
GuiConsoleWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LRESULT Result = 0;
    PGUI_CONSOLE_DATA GuiData = NULL;
    PCONSOLE Console = NULL;

    /*
     * - If it's the first time we create a window for the terminal,
     *   just initialize it and return.
     *
     * - If we are destroying the window, just do it and return.
     */
    if (msg == WM_NCCREATE)
    {
        return (LRESULT)GuiConsoleHandleNcCreate(hWnd, (LPCREATESTRUCTW)lParam);
    }
    else if (msg == WM_NCDESTROY)
    {
        return GuiConsoleHandleNcDestroy(hWnd);
    }

    /*
     * Now the terminal window is initialized.
     * Get the terminal data via the window's data.
     * If there is no data, just go away.
     */
    GuiData = GuiGetGuiData(hWnd);
    if (GuiData == NULL) return 0;

    /*
     * Each helper function which needs the console
     * has to validate and lock it.
     */

    /* We have a console, start message dispatching */
    switch (msg)
    {
        case WM_ACTIVATE:
        {
            if (LOWORD(wParam) == WA_CLICKACTIVE) GuiData->IgnoreNextMouseSignal = TRUE;
            break;
        }

        case WM_CLOSE:
            GuiConsoleHandleClose(GuiData);
            break;

        case WM_PAINT:
            GuiConsoleHandlePaint(GuiData);
            break;

        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_CHAR:
        {
            /* Detect Alt-Enter presses and switch back and forth to fullscreen mode */
            if (msg == WM_SYSKEYDOWN && (HIWORD(lParam) & KF_ALTDOWN) && wParam == VK_RETURN)
            {
                /* Switch only at first Alt-Enter press, and ignore subsequent key repetitions */
                if ((HIWORD(lParam) & (KF_UP | KF_REPEAT)) != KF_REPEAT) GuiConsoleSwitchFullScreen(GuiData);
                break;
            }

            GuiConsoleHandleKey(GuiData, msg, wParam, lParam);
            break;
        }

        case WM_TIMER:
            GuiConsoleHandleTimer(GuiData);
            break;

        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_MBUTTONUP:
        case WM_RBUTTONUP:
        case WM_LBUTTONDBLCLK:
        case WM_MBUTTONDBLCLK:
        case WM_RBUTTONDBLCLK:
        case WM_MOUSEMOVE:
        case WM_MOUSEWHEEL:
        case WM_MOUSEHWHEEL:
        {
            Result = GuiConsoleHandleMouse(GuiData, msg, wParam, lParam);
            break;
        }

        case WM_HSCROLL:
        case WM_VSCROLL:
        {
            Result = GuiConsoleHandleScroll(GuiData, msg, wParam);
            break;
        }

        case WM_NCRBUTTONDOWN:
        {
            DPRINT1("WM_NCRBUTTONDOWN\n");
            /*
             * HACK: !! Because, when we deal with WM_RBUTTON* and we do not
             * call after that DefWindowProc, on ReactOS, right-clicks on the
             * (non-client) application title-bar does not display the system
             * menu and does not trigger a WM_NCRBUTTONUP message too.
             * See: http://git.reactos.org/?p=reactos.git;a=blob;f=reactos/win32ss/user/user32/windows/defwnd.c;hb=HEAD#l1103
             * and line 1135 too.
             */
            if (DefWindowProcW(hWnd, WM_NCHITTEST, 0, lParam) == HTCAPTION)
            {
                /* Call DefWindowProcW with the WM_CONTEXTMENU message */
                msg = WM_CONTEXTMENU;
            }
            goto Default;
        }
        // case WM_NCRBUTTONUP:
            // DPRINT1("WM_NCRBUTTONUP\n");
            // goto Default;

        case WM_CONTEXTMENU:
        {
            if (DefWindowProcW(hWnd /*GuiData->hWindow*/, WM_NCHITTEST, 0, lParam) == HTCLIENT)
            {
                HMENU hMenu = CreatePopupMenu();
                if (hMenu != NULL)
                {
                    GuiConsoleAppendMenuItems(hMenu, GuiConsoleEditMenuItems);
                    TrackPopupMenuEx(hMenu,
                                     TPM_RIGHTBUTTON,
                                     GET_X_LPARAM(lParam),
                                     GET_Y_LPARAM(lParam),
                                     hWnd,
                                     NULL);
                    DestroyMenu(hMenu);
                }
                break;
            }
            else
            {
                goto Default;
            }
        }

        case WM_COMMAND:
        case WM_SYSCOMMAND:
        {
            Result = GuiConsoleHandleSysMenuCommand(GuiData, wParam, lParam);
            break;
        }

        case WM_SETFOCUS:
        case WM_KILLFOCUS:
        {
            Console = GuiData->Console; // Not NULL because checked in GuiGetGuiData.
            if (ConSrvValidateConsoleUnsafe(Console, CONSOLE_RUNNING, TRUE))
            {
                INPUT_RECORD er;
                er.EventType = FOCUS_EVENT;
                er.Event.FocusEvent.bSetFocus = (msg == WM_SETFOCUS);
                ConioProcessInputEvent(Console, &er);

                if (msg == WM_SETFOCUS)
                    DPRINT1("TODO: Create console caret\n");
                else // if (msg == WM_KILLFOCUS)
                    DPRINT1("TODO: Destroy console caret\n");

                LeaveCriticalSection(&Console->Lock);
            }
            break;
        }

        case WM_GETMINMAXINFO:
            GuiConsoleGetMinMaxInfo(GuiData, (PMINMAXINFO)lParam);
            break;

        case WM_SIZE:
            GuiConsoleResize(GuiData, wParam, lParam);
            break;

        case PM_RESIZE_TERMINAL:
        {
            /* Resize the window to the user's values */
            GuiData->WindowSizeLock = TRUE;
            GuiConsoleResizeWindow(GuiData);
            GuiData->WindowSizeLock = FALSE;
            break;
        }

        case PM_APPLY_CONSOLE_INFO:
        {
            Console = GuiData->Console; // Not NULL because checked in GuiGetGuiData.
            if (ConSrvValidateConsoleUnsafe(Console, CONSOLE_RUNNING, TRUE))
            {
                GuiApplyUserSettings(GuiData, (HANDLE)wParam, (BOOL)lParam);
                LeaveCriticalSection(&Console->Lock);
            }
            break;
        }

        case PM_CONSOLE_BEEP:
            DPRINT1("Beep !!\n");
            Beep(800, 200);
            break;

        // case PM_CONSOLE_SET_TITLE:
            // SetWindowText(GuiData->hWindow, GuiData->Console->Title.Buffer);
            // break;

        default: Default:
            Result = DefWindowProcW(hWnd, msg, wParam, lParam);
            break;
    }

    return Result;
}



/******************************************************************************
 *                        GUI Terminal Initialization                         *
 ******************************************************************************/

static LRESULT CALLBACK
GuiConsoleNotifyWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HWND NewWindow;
    LONG WindowCount;
    MSG Msg;

    switch (msg)
    {
        case WM_CREATE:
        {
            SetWindowLongW(hWnd, GWL_USERDATA, 0);
            return 0;
        }
    
        case PM_CREATE_CONSOLE:
        {
            PGUI_CONSOLE_DATA GuiData = (PGUI_CONSOLE_DATA)lParam;
            PCONSOLE Console = GuiData->Console;

            NewWindow = CreateWindowExW(WS_EX_CLIENTEDGE,
                                        GUI_CONSOLE_WINDOW_CLASS,
                                        Console->Title.Buffer,
                                        WS_OVERLAPPEDWINDOW | WS_HSCROLL | WS_VSCROLL,
                                        CW_USEDEFAULT,
                                        CW_USEDEFAULT,
                                        CW_USEDEFAULT,
                                        CW_USEDEFAULT,
                                        NULL,
                                        NULL,
                                        ConSrvDllInstance,
                                        (PVOID)GuiData);
            if (NULL != NewWindow)
            {
                WindowCount = GetWindowLongW(hWnd, GWL_USERDATA);
                WindowCount++;
                SetWindowLongW(hWnd, GWL_USERDATA, WindowCount);

                DPRINT("Set icons via PM_CREATE_CONSOLE\n");
                if (GuiData->hIcon == NULL)
                {
                    DPRINT("Not really /o\\...\n");
                    GuiData->hIcon   = ghDefaultIcon;
                    GuiData->hIconSm = ghDefaultIconSm;
                }
                else if (GuiData->hIcon != ghDefaultIcon)
                {
                    DPRINT("Yes \\o/\n");
                    SendMessageW(GuiData->hWindow, WM_SETICON, ICON_BIG, (LPARAM)GuiData->hIcon);
                    SendMessageW(GuiData->hWindow, WM_SETICON, ICON_SMALL, (LPARAM)GuiData->hIconSm);
                }

                /* Move and resize the window to the user's values */
                /* CAN WE DEADLOCK ?? */
                GuiConsoleMoveWindow(GuiData);
                GuiData->WindowSizeLock = TRUE;
                GuiConsoleResizeWindow(GuiData);
                GuiData->WindowSizeLock = FALSE;

                // ShowWindow(NewWindow, (int)wParam);
                ShowWindowAsync(NewWindow, (int)wParam);
                DPRINT("Window showed\n");
            }

            return (LRESULT)NewWindow;
        }

        case PM_DESTROY_CONSOLE:
        {
            PGUI_CONSOLE_DATA GuiData = (PGUI_CONSOLE_DATA)lParam;

            /*
             * Window creation is done using a PostMessage(), so it's possible
             * that the window that we want to destroy doesn't exist yet.
             * So first empty the message queue.
             */
            /*
            while (PeekMessageW(&Msg, NULL, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&Msg);
                DispatchMessageW(&Msg);
            }*/
            while (PeekMessageW(&Msg, NULL, 0, 0, PM_REMOVE)) ;

            if (GuiData->hWindow != NULL) /* && DestroyWindow(GuiData->hWindow) */
            {
                DestroyWindow(GuiData->hWindow);

                WindowCount = GetWindowLongW(hWnd, GWL_USERDATA);
                WindowCount--;
                SetWindowLongW(hWnd, GWL_USERDATA, WindowCount);
                if (0 == WindowCount)
                {
                    NotifyWnd = NULL;
                    DestroyWindow(hWnd);
                    DPRINT("CONSRV: Going to quit the Gui Thread!!\n");
                    PostQuitMessage(0);
                }
            }

            return 0;
        }

        default:
            return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
}

static DWORD WINAPI
GuiConsoleGuiThread(PVOID Data)
{
    MSG msg;
    PHANDLE GraphicsStartupEvent = (PHANDLE)Data;

    /*
     * This thread dispatches all the console notifications to the notify window.
     * It is common for all the console windows.
     */

    PrivateCsrssManualGuiCheck(+1);

    NotifyWnd = CreateWindowW(L"ConSrvCreateNotify",
                              L"",
                              WS_OVERLAPPEDWINDOW,
                              CW_USEDEFAULT,
                              CW_USEDEFAULT,
                              CW_USEDEFAULT,
                              CW_USEDEFAULT,
                              NULL,
                              NULL,
                              ConSrvDllInstance,
                              NULL);
    if (NULL == NotifyWnd)
    {
        PrivateCsrssManualGuiCheck(-1);
        SetEvent(*GraphicsStartupEvent);
        return 1;
    }

    SetEvent(*GraphicsStartupEvent);

    while (GetMessageW(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    DPRINT("CONSRV: Quit the Gui Thread!!\n");
    PrivateCsrssManualGuiCheck(-1);

    return 1;
}

static BOOL
GuiInit(VOID)
{
    WNDCLASSEXW wc;
    ATOM ConsoleClassAtom;

    /* Exit if we were already initialized */
    // if (ConsInitialized) return TRUE;

    /*
     * Initialize and register the different window classes, if needed.
     */
    if (!ConsInitialized)
    {
        /* Initialize the notification window class */
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.lpszClassName = L"ConSrvCreateNotify";
        wc.lpfnWndProc = GuiConsoleNotifyWndProc;
        wc.style = 0;
        wc.hInstance = ConSrvDllInstance;
        wc.hIcon = NULL;
        wc.hIconSm = NULL;
        wc.hCursor = NULL;
        wc.hbrBackground = NULL;
        wc.lpszMenuName = NULL;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        if (RegisterClassExW(&wc) == 0)
        {
            DPRINT1("Failed to register GUI notify wndproc\n");
            return FALSE;
        }

        /* Initialize the console window class */
        ghDefaultIcon   = LoadImageW(ConSrvDllInstance,
                                     MAKEINTRESOURCEW(IDI_TERMINAL),
                                     IMAGE_ICON,
                                     GetSystemMetrics(SM_CXICON),
                                     GetSystemMetrics(SM_CYICON),
                                     LR_SHARED);
        ghDefaultIconSm = LoadImageW(ConSrvDllInstance,
                                     MAKEINTRESOURCEW(IDI_TERMINAL),
                                     IMAGE_ICON,
                                     GetSystemMetrics(SM_CXSMICON),
                                     GetSystemMetrics(SM_CYSMICON),
                                     LR_SHARED);
        ghDefaultCursor = LoadCursorW(NULL, IDC_ARROW);
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.lpszClassName = GUI_CONSOLE_WINDOW_CLASS;
        wc.lpfnWndProc = GuiConsoleWndProc;
        wc.style = CS_DBLCLKS /* | CS_HREDRAW | CS_VREDRAW */;
        wc.hInstance = ConSrvDllInstance;
        wc.hIcon = ghDefaultIcon;
        wc.hIconSm = ghDefaultIconSm;
        wc.hCursor = ghDefaultCursor;
        wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH); // The color of a terminal when it is switch off.
        wc.lpszMenuName = NULL;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = GWLP_CONSOLEWND_ALLOC;

        ConsoleClassAtom = RegisterClassExW(&wc);
        if (ConsoleClassAtom == 0)
        {
            DPRINT1("Failed to register GUI console wndproc\n");
            return FALSE;
        }
        else
        {
            NtUserConsoleControl(GuiConsoleWndClassAtom, &ConsoleClassAtom, sizeof(ATOM));
        }

        ConsInitialized = TRUE;
    }

    /*
     * Set-up the notification window
     */
    if (NULL == NotifyWnd)
    {
        HANDLE ThreadHandle;
        HANDLE GraphicsStartupEvent;

        GraphicsStartupEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
        if (NULL == GraphicsStartupEvent) return FALSE;

        ThreadHandle = CreateThread(NULL,
                                    0,
                                    GuiConsoleGuiThread,
                                    (PVOID)&GraphicsStartupEvent,
                                    0,
                                    NULL);
        if (NULL == ThreadHandle)
        {
            CloseHandle(GraphicsStartupEvent);
            DPRINT1("CONSRV: Failed to create graphics console thread. Expect problems\n");
            return FALSE;
        }
        SetThreadPriority(ThreadHandle, THREAD_PRIORITY_HIGHEST);
        CloseHandle(ThreadHandle);

        WaitForSingleObject(GraphicsStartupEvent, INFINITE);
        CloseHandle(GraphicsStartupEvent);

        if (NULL == NotifyWnd)
        {
            DPRINT1("CONSRV: Failed to create notification window.\n");
            return FALSE;
        }
    }

    // ConsInitialized = TRUE;

    return TRUE;
}



/******************************************************************************
 *                             GUI Console Driver                             *
 ******************************************************************************/

static VOID WINAPI
GuiCleanupConsole(PCONSOLE Console)
{
    PGUI_CONSOLE_DATA GuiData = Console->TermIFace.Data;

    SendMessageW(NotifyWnd, PM_DESTROY_CONSOLE, 0, (LPARAM)GuiData);

    DPRINT("Destroying icons !! - GuiData->hIcon = 0x%p ; ghDefaultIcon = 0x%p ; GuiData->hIconSm = 0x%p ; ghDefaultIconSm = 0x%p\n",
            GuiData->hIcon, ghDefaultIcon, GuiData->hIconSm, ghDefaultIconSm);
    if (GuiData->hIcon != NULL && GuiData->hIcon != ghDefaultIcon)
    {
        DPRINT("Destroy hIcon\n");
        DestroyIcon(GuiData->hIcon);
    }
    if (GuiData->hIconSm != NULL && GuiData->hIconSm != ghDefaultIconSm)
    {
        DPRINT("Destroy hIconSm\n");
        DestroyIcon(GuiData->hIconSm);
    }

    Console->TermIFace.Data = NULL;
    DeleteCriticalSection(&GuiData->Lock);
    ConsoleFreeHeap(GuiData);

    DPRINT("Quit GuiCleanupConsole\n");
}

static VOID WINAPI
GuiDrawRegion(PCONSOLE Console, SMALL_RECT* Region)
{
    PGUI_CONSOLE_DATA GuiData = Console->TermIFace.Data;
    RECT RegionRect;

    SmallRectToRect(GuiData, &RegionRect, Region);
    /* Do not erase the background: it speeds up redrawing and reduce flickering */
    InvalidateRect(GuiData->hWindow, &RegionRect, FALSE);
}

static VOID WINAPI
GuiWriteStream(PCONSOLE Console, SMALL_RECT* Region, SHORT CursorStartX, SHORT CursorStartY,
               UINT ScrolledLines, CHAR *Buffer, UINT Length)
{
    PGUI_CONSOLE_DATA GuiData = Console->TermIFace.Data;
    PCONSOLE_SCREEN_BUFFER Buff = Console->ActiveBuffer;
    SHORT CursorEndX, CursorEndY;
    RECT ScrollRect;

    if (NULL == GuiData || NULL == GuiData->hWindow) return;
    if (GetType(Buff) != TEXTMODE_BUFFER) return;

    if (0 != ScrolledLines)
    {
        ScrollRect.left = 0;
        ScrollRect.top = 0;
        ScrollRect.right = Buff->ViewSize.X * GuiData->CharWidth;
        ScrollRect.bottom = Region->Top * GuiData->CharHeight;

        ScrollWindowEx(GuiData->hWindow,
                       0,
                       -(int)(ScrolledLines * GuiData->CharHeight),
                       &ScrollRect,
                       NULL,
                       NULL,
                       NULL,
                       SW_INVALIDATE);
    }

    GuiDrawRegion(Console, Region);

    if (CursorStartX < Region->Left || Region->Right < CursorStartX
            || CursorStartY < Region->Top || Region->Bottom < CursorStartY)
    {
        GuiInvalidateCell(Console, CursorStartX, CursorStartY);
    }

    CursorEndX = Buff->CursorPosition.X;
    CursorEndY = Buff->CursorPosition.Y;
    if ((CursorEndX < Region->Left || Region->Right < CursorEndX
            || CursorEndY < Region->Top || Region->Bottom < CursorEndY)
            && (CursorEndX != CursorStartX || CursorEndY != CursorStartY))
    {
        GuiInvalidateCell(Console, CursorEndX, CursorEndY);
    }

    // Set up the update timer (very short interval) - this is a "hack" for getting the OS to
    // repaint the window without having it just freeze up and stay on the screen permanently.
    Buff->CursorBlinkOn = TRUE;
    SetTimer(GuiData->hWindow, CONGUI_UPDATE_TIMER, CONGUI_UPDATE_TIME, NULL);
}

static BOOL WINAPI
GuiSetCursorInfo(PCONSOLE Console, PCONSOLE_SCREEN_BUFFER Buff)
{
    if (Console->ActiveBuffer == Buff)
    {
        GuiInvalidateCell(Console, Buff->CursorPosition.X, Buff->CursorPosition.Y);
    }

    return TRUE;
}

static BOOL WINAPI
GuiSetScreenInfo(PCONSOLE Console, PCONSOLE_SCREEN_BUFFER Buff, SHORT OldCursorX, SHORT OldCursorY)
{
    if (Console->ActiveBuffer == Buff)
    {
        /* Redraw char at old position (remove cursor) */
        GuiInvalidateCell(Console, OldCursorX, OldCursorY);
        /* Redraw char at new position (show cursor) */
        GuiInvalidateCell(Console, Buff->CursorPosition.X, Buff->CursorPosition.Y);
    }

    return TRUE;
}

static VOID WINAPI
GuiResizeTerminal(PCONSOLE Console)
{
    PGUI_CONSOLE_DATA GuiData = Console->TermIFace.Data;

    /* Resize the window to the user's values */
    // GuiData->WindowSizeLock = TRUE;
    // GuiConsoleResizeWindow(GuiData);
    // GuiData->WindowSizeLock = FALSE;
    // NOTE: This code ^^ causes deadlocks...

    PostMessageW(GuiData->hWindow, PM_RESIZE_TERMINAL, 0, 0);
}

static BOOL WINAPI
GuiProcessKeyCallback(PCONSOLE Console, MSG* msg, BYTE KeyStateMenu, DWORD ShiftState, UINT VirtualKeyCode, BOOL Down)
{
    if ((ShiftState & (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED) || KeyStateMenu & 0x80) &&
        (VirtualKeyCode == VK_ESCAPE || VirtualKeyCode == VK_TAB || VirtualKeyCode == VK_SPACE))
    {
        DefWindowProcW(msg->hwnd, msg->message, msg->wParam, msg->lParam);
        return TRUE;
    }

    return FALSE;
}

static VOID WINAPI
GuiRefreshInternalInfo(PCONSOLE Console)
{
    PGUI_CONSOLE_DATA GuiData = Console->TermIFace.Data;

    /* Update the console leader information held by the window */
    SetConsoleWndConsoleLeaderCID(GuiData);
}

static VOID WINAPI
GuiChangeTitle(PCONSOLE Console)
{
    PGUI_CONSOLE_DATA GuiData = Console->TermIFace.Data;
    // PostMessageW(GuiData->hWindow, PM_CONSOLE_SET_TITLE, 0, 0);
    SetWindowText(GuiData->hWindow, Console->Title.Buffer);
}

static BOOL WINAPI
GuiChangeIcon(PCONSOLE Console, HICON hWindowIcon)
{
    PGUI_CONSOLE_DATA GuiData = Console->TermIFace.Data;
    HICON hIcon, hIconSm;

    if (hWindowIcon == NULL)
    {
        hIcon   = ghDefaultIcon;
        hIconSm = ghDefaultIconSm;
    }
    else
    {
        hIcon   = CopyIcon(hWindowIcon);
        hIconSm = CopyIcon(hWindowIcon);
    }

    if (hIcon == NULL)
    {
        return FALSE;
    }

    if (hIcon != GuiData->hIcon)
    {
        if (GuiData->hIcon != NULL && GuiData->hIcon != ghDefaultIcon)
        {
            DestroyIcon(GuiData->hIcon);
        }
        if (GuiData->hIconSm != NULL && GuiData->hIconSm != ghDefaultIconSm)
        {
            DestroyIcon(GuiData->hIconSm);
        }

        GuiData->hIcon   = hIcon;
        GuiData->hIconSm = hIconSm;

        DPRINT("Set icons in GuiChangeIcon\n");
        PostMessageW(GuiData->hWindow, WM_SETICON, ICON_BIG, (LPARAM)GuiData->hIcon);
        PostMessageW(GuiData->hWindow, WM_SETICON, ICON_SMALL, (LPARAM)GuiData->hIconSm);
    }

    return TRUE;
}

static HWND WINAPI
GuiGetConsoleWindowHandle(PCONSOLE Console)
{
    PGUI_CONSOLE_DATA GuiData = Console->TermIFace.Data;
    return GuiData->hWindow;
}

static VOID WINAPI
GuiGetLargestConsoleWindowSize(PCONSOLE Console, PCOORD pSize)
{
    PGUI_CONSOLE_DATA GuiData = Console->TermIFace.Data;
    RECT WorkArea;
    LONG width, height;
    UINT WidthUnit, HeightUnit;

    if (!pSize) return;

    if (!SystemParametersInfoW(SPI_GETWORKAREA, 0, &WorkArea, 0))
    {
        DPRINT1("SystemParametersInfoW failed - What to do ??\n");
        return;
    }

    if (Console->ActiveBuffer)
    {
        if (GetType(Console->ActiveBuffer) == TEXTMODE_BUFFER)
        {
            WidthUnit  = GuiData->CharWidth ;
            HeightUnit = GuiData->CharHeight;
        }
        else /* if (GetType(Console->ActiveBuffer) == GRAPHICS_BUFFER) */
        {
            WidthUnit  = 1;
            HeightUnit = 1;
        }
    }
    else
    {
        /* Default: text mode */
        WidthUnit  = GuiData->CharWidth ;
        HeightUnit = GuiData->CharHeight;
    }

    width  = WorkArea.right;
    height = WorkArea.bottom;

    width  -= (2 * (GetSystemMetrics(SM_CXFRAME) + GetSystemMetrics(SM_CXEDGE)));
    height -= (2 * (GetSystemMetrics(SM_CYFRAME) + GetSystemMetrics(SM_CYEDGE)) + GetSystemMetrics(SM_CYCAPTION));

    if (width  < 0) width  = 0;
    if (height < 0) height = 0;

    pSize->X = (SHORT)(width  / (int)WidthUnit ) /* HACK */ + 2;
    pSize->Y = (SHORT)(height / (int)HeightUnit) /* HACK */ + 1;
}

static ULONG WINAPI
GuiGetDisplayMode(PCONSOLE Console)
{
    PGUI_CONSOLE_DATA GuiData = Console->TermIFace.Data;
    ULONG DisplayMode = 0;

    if (GuiData->GuiInfo.FullScreen)
        DisplayMode |= CONSOLE_FULLSCREEN_HARDWARE; // CONSOLE_FULLSCREEN
    else
        DisplayMode |= CONSOLE_WINDOWED;

    return DisplayMode;
}

static BOOL WINAPI
GuiSetDisplayMode(PCONSOLE Console, ULONG NewMode)
{
    PGUI_CONSOLE_DATA GuiData = Console->TermIFace.Data;

    if (NewMode & ~(CONSOLE_FULLSCREEN_MODE | CONSOLE_WINDOWED_MODE))
        return FALSE;

    GuiData->GuiInfo.FullScreen = (NewMode & CONSOLE_FULLSCREEN_MODE);
    // TODO: Change the display mode
    return TRUE;
}

static FRONTEND_VTBL GuiVtbl =
{
    GuiCleanupConsole,
    GuiDrawRegion,
    GuiWriteStream,
    GuiSetCursorInfo,
    GuiSetScreenInfo,
    GuiResizeTerminal,
    GuiProcessKeyCallback,
    GuiRefreshInternalInfo,
    GuiChangeTitle,
    GuiChangeIcon,
    GuiGetConsoleWindowHandle,
    GuiGetLargestConsoleWindowSize,
    GuiGetDisplayMode,
    GuiSetDisplayMode,
};

NTSTATUS FASTCALL
GuiInitConsole(PCONSOLE Console,
               /*IN*/ PCONSOLE_START_INFO ConsoleStartInfo,
               PCONSOLE_INFO ConsoleInfo,
               DWORD ProcessId,
               LPCWSTR IconPath,
               INT IconIndex)
{
    PGUI_CONSOLE_DATA GuiData;
    GUI_CONSOLE_INFO TermInfo;
    SIZE_T Length = 0;

    if (Console == NULL || ConsoleInfo == NULL)
        return STATUS_INVALID_PARAMETER;

    /* Initialize the GUI terminal emulator */
    if (!GuiInit()) return STATUS_UNSUCCESSFUL;

    /* Initialize the console */
    Console->TermIFace.Vtbl = &GuiVtbl;

    GuiData = ConsoleAllocHeap(HEAP_ZERO_MEMORY, sizeof(GUI_CONSOLE_DATA));
    if (!GuiData)
    {
        DPRINT1("CONSRV: Failed to create GUI_CONSOLE_DATA\n");
        return STATUS_UNSUCCESSFUL;
    }
    Console->TermIFace.Data = (PVOID)GuiData;
    GuiData->Console = Console;
    GuiData->hWindow = NULL;

    /* The console can be resized */
    Console->FixedSize = FALSE;

    InitializeCriticalSection(&GuiData->Lock);


    /*
     * Load the terminal settings
     */

    /***********************************************
     * Adapted from ConSrvInitConsole in console.c *
     ***********************************************/

    /* 1. Load the default settings */
    GuiConsoleGetDefaultSettings(&TermInfo, ProcessId);

    /* 2. Load the remaining console settings via the registry. */
    if ((ConsoleStartInfo->dwStartupFlags & STARTF_TITLEISLINKNAME) == 0)
    {
        /* Load the terminal infos from the registry. */
        GuiConsoleReadUserSettings(&TermInfo, ConsoleInfo->ConsoleTitle, ProcessId);

        /*
         * Now, update them with the properties the user might gave to us
         * via the STARTUPINFO structure before calling CreateProcess
         * (and which was transmitted via the ConsoleStartInfo structure).
         * We therefore overwrite the values read in the registry.
         */
        if (ConsoleStartInfo->dwStartupFlags & STARTF_USESHOWWINDOW)
        {
            TermInfo.ShowWindow = ConsoleStartInfo->ShowWindow;
        }
        if (ConsoleStartInfo->dwStartupFlags & STARTF_USEPOSITION)
        {
            TermInfo.AutoPosition = FALSE;
            TermInfo.WindowOrigin = ConsoleStartInfo->ConsoleWindowOrigin;
        }
        if (ConsoleStartInfo->dwStartupFlags & STARTF_RUNFULLSCREEN)
        {
            TermInfo.FullScreen = TRUE;
        }
    }


    /*
     * Set up the GUI data
     */

    Length = min(wcslen(TermInfo.FaceName) + 1, LF_FACESIZE); // wcsnlen
    wcsncpy(GuiData->GuiInfo.FaceName, TermInfo.FaceName, LF_FACESIZE);
    GuiData->GuiInfo.FaceName[Length] = L'\0';
    GuiData->GuiInfo.FontFamily     = TermInfo.FontFamily;
    GuiData->GuiInfo.FontSize       = TermInfo.FontSize;
    GuiData->GuiInfo.FontWeight     = TermInfo.FontWeight;
    GuiData->GuiInfo.UseRasterFonts = TermInfo.UseRasterFonts;
    GuiData->GuiInfo.FullScreen     = TermInfo.FullScreen;
    GuiData->GuiInfo.ShowWindow     = TermInfo.ShowWindow;
    GuiData->GuiInfo.AutoPosition   = TermInfo.AutoPosition;
    GuiData->GuiInfo.WindowOrigin   = TermInfo.WindowOrigin;

    /* Initialize the icon handles to their default values */
    GuiData->hIcon   = ghDefaultIcon;
    GuiData->hIconSm = ghDefaultIconSm;

    /* Get the associated icon, if any */
    if (IconPath == NULL || *IconPath == L'\0')
    {
        IconPath  = ConsoleStartInfo->AppPath;
        IconIndex = 0;
    }
    DPRINT("IconPath = %S ; IconIndex = %lu\n", (IconPath ? IconPath : L"n/a"), IconIndex);
    if (IconPath)
    {
        HICON hIcon = NULL, hIconSm = NULL;
        PrivateExtractIconExW(IconPath,
                              IconIndex,
                              &hIcon,
                              &hIconSm,
                              1);
        DPRINT("hIcon = 0x%p ; hIconSm = 0x%p\n", hIcon, hIconSm);
        if (hIcon != NULL)
        {
            DPRINT("Effectively set the icons\n");
            GuiData->hIcon   = hIcon;
            GuiData->hIconSm = hIconSm;
        }
    }

    /*
     * We need to wait until the GUI has been fully initialized
     * to retrieve custom settings i.e. WindowSize etc...
     * Ideally we could use SendNotifyMessage for this but its not
     * yet implemented.
     */
    GuiData->hGuiInitEvent = CreateEventW(NULL, FALSE, FALSE, NULL);

    /* Create the terminal window */
    PostMessageW(NotifyWnd, PM_CREATE_CONSOLE, GuiData->GuiInfo.ShowWindow, (LPARAM)GuiData);

    /* Wait until initialization has finished */
    WaitForSingleObject(GuiData->hGuiInitEvent, INFINITE);
    DPRINT("OK we created the console window\n");
    CloseHandle(GuiData->hGuiInitEvent);
    GuiData->hGuiInitEvent = NULL;

    /* Check whether we really succeeded in initializing the terminal window */
    if (GuiData->hWindow == NULL)
    {
        DPRINT("GuiInitConsole - We failed at creating a new terminal window\n");
        // ConioCleanupConsole(Console);
        GuiCleanupConsole(Console);
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

/* EOF */
