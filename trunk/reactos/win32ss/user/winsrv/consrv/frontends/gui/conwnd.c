/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            frontends/gui/conwnd.c
 * PURPOSE:         GUI Console Window Class
 * PROGRAMMERS:     G� van Geldorp
 *                  Johannes Anderwald
 *                  Jeffrey Morlan
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#include <consrv.h>

#include <windowsx.h>

#define NDEBUG
#include <debug.h>

#include "guiterm.h"
#include "conwnd.h"
#include "resource.h"

/* GLOBALS ********************************************************************/

// #define PM_CREATE_CONSOLE       (WM_APP + 1)
// #define PM_DESTROY_CONSOLE      (WM_APP + 2)

// See guiterm.c
#define CONGUI_MIN_WIDTH      10
#define CONGUI_MIN_HEIGHT     10
#define CONGUI_UPDATE_TIME    0
#define CONGUI_UPDATE_TIMER   1

#define CURSOR_BLINK_TIME 500


/**************************************************************\
\** Define the Console Leader Process for the console window **/
#define GWLP_CONWND_ALLOC      (2 * sizeof(LONG_PTR))
#define GWLP_CONSOLE_LEADER_PID 0
#define GWLP_CONSOLE_LEADER_TID 4

VOID
SetConWndConsoleLeaderCID(IN PGUI_CONSOLE_DATA GuiData)
{
    PCONSOLE_PROCESS_DATA ProcessData;
    CLIENT_ID ConsoleLeaderCID;

    ProcessData = CONTAINING_RECORD(GuiData->Console->ProcessList.Blink,
                                    CONSOLE_PROCESS_DATA,
                                    ConsoleLink);
    ConsoleLeaderCID = ProcessData->Process->ClientId;
    SetWindowLongPtrW(GuiData->hWindow, GWLP_CONSOLE_LEADER_PID,
                      (LONG_PTR)(ConsoleLeaderCID.UniqueProcess));
    SetWindowLongPtrW(GuiData->hWindow, GWLP_CONSOLE_LEADER_TID,
                      (LONG_PTR)(ConsoleLeaderCID.UniqueThread));
}
/**************************************************************/

HICON   ghDefaultIcon = NULL;
HICON   ghDefaultIconSm = NULL;
HCURSOR ghDefaultCursor = NULL;

typedef struct _GUICONSOLE_MENUITEM
{
    UINT uID;
    const struct _GUICONSOLE_MENUITEM *SubMenu;
    WORD wCmdID;
} GUICONSOLE_MENUITEM, *PGUICONSOLE_MENUITEM;

static const GUICONSOLE_MENUITEM GuiConsoleEditMenuItems[] =
{
    { IDS_MARK,         NULL, ID_SYSTEM_EDIT_MARK       },
    { IDS_COPY,         NULL, ID_SYSTEM_EDIT_COPY       },
    { IDS_PASTE,        NULL, ID_SYSTEM_EDIT_PASTE      },
    { IDS_SELECTALL,    NULL, ID_SYSTEM_EDIT_SELECTALL  },
    { IDS_SCROLL,       NULL, ID_SYSTEM_EDIT_SCROLL     },
    { IDS_FIND,         NULL, ID_SYSTEM_EDIT_FIND       },

    { 0, NULL, 0 } /* End of list */
};

static const GUICONSOLE_MENUITEM GuiConsoleMainMenuItems[] =
{
    { IDS_EDIT,         GuiConsoleEditMenuItems, 0 },
    { IDS_DEFAULTS,     NULL, ID_SYSTEM_DEFAULTS   },
    { IDS_PROPERTIES,   NULL, ID_SYSTEM_PROPERTIES },

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

static LRESULT CALLBACK
ConWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

BOOLEAN
RegisterConWndClass(IN HINSTANCE hInstance)
{
    WNDCLASSEXW WndClass;
    ATOM WndClassAtom;

    ghDefaultIcon   = LoadImageW(hInstance,
                                 MAKEINTRESOURCEW(IDI_TERMINAL),
                                 IMAGE_ICON,
                                 GetSystemMetrics(SM_CXICON),
                                 GetSystemMetrics(SM_CYICON),
                                 LR_SHARED);
    ghDefaultIconSm = LoadImageW(hInstance,
                                 MAKEINTRESOURCEW(IDI_TERMINAL),
                                 IMAGE_ICON,
                                 GetSystemMetrics(SM_CXSMICON),
                                 GetSystemMetrics(SM_CYSMICON),
                                 LR_SHARED);
    ghDefaultCursor = LoadCursorW(NULL, IDC_ARROW);

    WndClass.cbSize = sizeof(WNDCLASSEXW);
    WndClass.lpszClassName = GUI_CONWND_CLASS;
    WndClass.lpfnWndProc = ConWndProc;
    WndClass.style = CS_DBLCLKS /* | CS_HREDRAW | CS_VREDRAW */;
    WndClass.hInstance = hInstance;
    WndClass.hIcon = ghDefaultIcon;
    WndClass.hIconSm = ghDefaultIconSm;
    WndClass.hCursor = ghDefaultCursor;
    WndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH); // The color of a terminal when it is switched off.
    WndClass.lpszMenuName = NULL;
    WndClass.cbClsExtra = 0;
    WndClass.cbWndExtra = GWLP_CONWND_ALLOC;

    WndClassAtom = RegisterClassExW(&WndClass);
    if (WndClassAtom == 0)
    {
        DPRINT1("Failed to register GUI console class\n");
    }
    else
    {
        NtUserConsoleControl(GuiConsoleWndClassAtom, &WndClassAtom, sizeof(ATOM));
    }

    return (WndClassAtom != 0);
}

BOOLEAN
UnRegisterConWndClass(HINSTANCE hInstance)
{
    return !!UnregisterClassW(GUI_CONWND_CLASS, hInstance);
}



static VOID
GetScreenBufferSizeUnits(IN PCONSOLE_SCREEN_BUFFER Buffer,
                         IN PGUI_CONSOLE_DATA GuiData,
                         OUT PUINT WidthUnit,
                         OUT PUINT HeightUnit)
{
    if (Buffer == NULL || GuiData == NULL ||
        WidthUnit == NULL || HeightUnit == NULL)
    {
        return;
    }

    if (GetType(Buffer) == TEXTMODE_BUFFER)
    {
        *WidthUnit  = GuiData->CharWidth ;
        *HeightUnit = GuiData->CharHeight;
    }
    else /* if (GetType(Buffer) == GRAPHICS_BUFFER) */
    {
        *WidthUnit  = 1;
        *HeightUnit = 1;
    }
}

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
    HMENU hMenu = GetSystemMenu(hWnd, FALSE);
    if (hMenu != NULL)
    {
        GuiConsoleAppendMenuItems(hMenu, GuiConsoleMainMenuItems);
        DrawMenuBar(hWnd);
    }
}

static VOID
GuiSendMenuEvent(PCONSOLE Console, UINT CmdId)
{
    INPUT_RECORD er;

    er.EventType = MENU_EVENT;
    er.Event.MenuEvent.dwCommandId = CmdId;

    DPRINT("Menu item ID: %d\n", CmdId);
    ConioProcessInputEvent(Console, &er);
}

static VOID
GuiConsoleCopy(PGUI_CONSOLE_DATA GuiData);
static VOID
GuiConsolePaste(PGUI_CONSOLE_DATA GuiData);
static VOID
GuiConsoleUpdateSelection(PCONSOLE Console, PCOORD coord);
static VOID
GuiConsoleResizeWindow(PGUI_CONSOLE_DATA GuiData, DWORD WidthUnit, DWORD HeightUnit);

static LRESULT
GuiConsoleHandleSysMenuCommand(PGUI_CONSOLE_DATA GuiData, WPARAM wParam, LPARAM lParam)
{
    LRESULT Ret = TRUE;
    PCONSOLE Console = GuiData->Console;
    PCONSOLE_SCREEN_BUFFER ActiveBuffer;

    if (!ConDrvValidateConsoleUnsafe(Console, CONSOLE_RUNNING, TRUE))
    {
        Ret = FALSE;
        goto Quit;
    }
    ActiveBuffer = GuiData->ActiveBuffer;

    /*
     * In case the selected menu item belongs to the user-reserved menu id range,
     * send to him a menu event and return directly. The user must handle those
     * reserved menu commands...
     */
    if (GuiData->CmdIdLow <= (UINT)wParam && (UINT)wParam <= GuiData->CmdIdHigh)
    {
        GuiSendMenuEvent(Console, (UINT)wParam);
        goto Unlock_Quit;
    }

    /* ... otherwise, perform actions. */
    switch (wParam)
    {
        case ID_SYSTEM_EDIT_MARK:
        {
            /* Clear the old selection */
            // GuiConsoleUpdateSelection(Console, NULL);
            Console->Selection.dwFlags = CONSOLE_NO_SELECTION;

            /* Restart a new selection */
            Console->dwSelectionCursor.X = ActiveBuffer->ViewOrigin.X;
            Console->dwSelectionCursor.Y = ActiveBuffer->ViewOrigin.Y;
            Console->Selection.dwSelectionAnchor = Console->dwSelectionCursor;
            GuiConsoleUpdateSelection(Console, &Console->Selection.dwSelectionAnchor);

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
            /* Clear the old selection */
            // GuiConsoleUpdateSelection(Console, NULL);
            Console->Selection.dwFlags = CONSOLE_NO_SELECTION;

            /*
             * The selection area extends to the whole screen buffer's width.
             */
            Console->Selection.dwSelectionAnchor.X = 0;
            Console->Selection.dwSelectionAnchor.Y = 0;
            Console->dwSelectionCursor.X = ActiveBuffer->ScreenBufferSize.X - 1;

            /*
             * Determine whether the selection must extend to just some part
             * (for text-mode screen buffers) or to all of the screen buffer's
             * height (for graphics ones).
             */
            if (GetType(ActiveBuffer) == TEXTMODE_BUFFER)
            {
                /*
                 * We select all the characters from the first line
                 * to the line where the cursor is positioned.
                 */
                Console->dwSelectionCursor.Y = ActiveBuffer->CursorPosition.Y;
            }
            else /* if (GetType(ActiveBuffer) == GRAPHICS_BUFFER) */
            {
                /*
                 * We select all the screen buffer area.
                 */
                Console->dwSelectionCursor.Y = ActiveBuffer->ScreenBufferSize.Y - 1;
            }

            /* Restart a new selection */
            Console->Selection.dwFlags |= CONSOLE_MOUSE_SELECTION;
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

Unlock_Quit:
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

static VOID
GuiConsoleResizeWindow(PGUI_CONSOLE_DATA GuiData, DWORD WidthUnit, DWORD HeightUnit)
{
    PCONSOLE_SCREEN_BUFFER Buff = GuiData->ActiveBuffer;
    SCROLLINFO sInfo;

    DWORD Width, Height;

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

static BOOL
GuiConsoleHandleNcCreate(HWND hWnd, LPCREATESTRUCTW Create)
{
    PGUI_CONSOLE_DATA GuiData = (PGUI_CONSOLE_DATA)Create->lpCreateParams;
    PCONSOLE Console;
    HDC hDC;
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
    hDC = GetDC(GuiData->hWindow);
    if (NULL == hDC)
    {
        DPRINT1("GuiConsoleNcCreate: GetDC failed\n");
        DeleteObject(GuiData->Font);
        GuiData->hWindow = NULL;
        SetEvent(GuiData->hGuiInitEvent);
        return FALSE;
    }
    OldFont = SelectObject(hDC, GuiData->Font);
    if (NULL == OldFont)
    {
        DPRINT1("GuiConsoleNcCreate: SelectObject failed\n");
        ReleaseDC(GuiData->hWindow, hDC);
        DeleteObject(GuiData->Font);
        GuiData->hWindow = NULL;
        SetEvent(GuiData->hGuiInitEvent);
        return FALSE;
    }
    if (!GetTextMetricsW(hDC, &Metrics))
    {
        DPRINT1("GuiConsoleNcCreate: GetTextMetrics failed\n");
        SelectObject(hDC, OldFont);
        ReleaseDC(GuiData->hWindow, hDC);
        DeleteObject(GuiData->Font);
        GuiData->hWindow = NULL;
        SetEvent(GuiData->hGuiInitEvent);
        return FALSE;
    }
    GuiData->CharWidth  = Metrics.tmMaxCharWidth;
    GuiData->CharHeight = Metrics.tmHeight + Metrics.tmExternalLeading;

    /* Measure real char width more precisely if possible. */
    if (GetTextExtentPoint32W(hDC, L"R", 1, &CharSize))
        GuiData->CharWidth = CharSize.cx;

    SelectObject(hDC, OldFont);

    ReleaseDC(GuiData->hWindow, hDC);

    /* Initialize the terminal framebuffer */
    GuiData->hMemDC  = CreateCompatibleDC(NULL);
    GuiData->hBitmap = NULL;
    GuiData->hSysPalette = NULL; /* Original system palette */

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
    PCONSOLE_SCREEN_BUFFER Buffer = GuiData->ActiveBuffer;
    UINT WidthUnit, HeightUnit;

    GetScreenBufferSizeUnits(Buffer, GuiData, &WidthUnit, &HeightUnit);

    Rect->left   = (SmallRect->Left       - Buffer->ViewOrigin.X) * WidthUnit ;
    Rect->top    = (SmallRect->Top        - Buffer->ViewOrigin.Y) * HeightUnit;
    Rect->right  = (SmallRect->Right  + 1 - Buffer->ViewOrigin.X) * WidthUnit ;
    Rect->bottom = (SmallRect->Bottom + 1 - Buffer->ViewOrigin.Y) * HeightUnit;
}

static VOID
GuiConsoleUpdateSelection(PCONSOLE Console, PCOORD coord)
{
    PGUI_CONSOLE_DATA GuiData = Console->TermIFace.Data;
    RECT oldRect;

    SmallRectToRect(GuiData, &oldRect, &Console->Selection.srSelection);

    if (coord != NULL)
    {
        RECT newRect;
        SMALL_RECT rc;

        /* Exchange left/top with right/bottom if required */
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

                /* Calculate the region that needs to be updated */
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

        if ((Console->Selection.dwFlags & CONSOLE_SELECTION_IN_PROGRESS) == 0)
        {
            LPWSTR SelectionType, WindowTitle = NULL;
            SIZE_T Length = 0;

            /* Clear the old selection */
            if (Console->Selection.dwFlags & CONSOLE_SELECTION_NOT_EMPTY)
            {
                InvalidateRect(GuiData->hWindow, &oldRect, FALSE);
            }

            if (Console->Selection.dwFlags & CONSOLE_MOUSE_SELECTION)
            {
                SelectionType = L"Selection - ";
            }
            else
            {
                SelectionType = L"Mark - ";
            }

            Length = Console->Title.Length + wcslen(SelectionType) + 1;
            WindowTitle = ConsoleAllocHeap(0, Length * sizeof(WCHAR));
            wcscpy(WindowTitle, SelectionType);
            wcscat(WindowTitle, Console->Title.Buffer);
            SetWindowText(GuiData->hWindow, WindowTitle);
            ConsoleFreeHeap(WindowTitle);

            Console->Selection.dwFlags |= CONSOLE_SELECTION_IN_PROGRESS;
            ConioPause(Console, PAUSED_FROM_SELECTION);
        }
    }
    else
    {
        /* Clear the selection */
        if (Console->Selection.dwFlags & CONSOLE_SELECTION_NOT_EMPTY)
        {
            InvalidateRect(GuiData->hWindow, &oldRect, FALSE);
        }

        Console->Selection.dwFlags = CONSOLE_NO_SELECTION;
        ConioUnpause(Console, PAUSED_FROM_SELECTION);

        SetWindowText(GuiData->hWindow, Console->Title.Buffer);
    }
}


VOID
GuiPaintTextModeBuffer(PTEXTMODE_SCREEN_BUFFER Buffer,
                       PGUI_CONSOLE_DATA GuiData,
                       PRECT rcView,
                       PRECT rcFramebuffer);
VOID
GuiPaintGraphicsBuffer(PGRAPHICS_SCREEN_BUFFER Buffer,
                       PGUI_CONSOLE_DATA GuiData,
                       PRECT rcView,
                       PRECT rcFramebuffer);

static VOID
GuiConsoleHandlePaint(PGUI_CONSOLE_DATA GuiData)
{
    BOOL Success = TRUE;
    PCONSOLE Console = GuiData->Console;
    PCONSOLE_SCREEN_BUFFER ActiveBuffer;
    PAINTSTRUCT ps;
    RECT rcPaint;

    if (!ConDrvValidateConsoleUnsafe(Console, CONSOLE_RUNNING, TRUE))
    {
        Success = FALSE;
        goto Quit;
    }
    ActiveBuffer = GuiData->ActiveBuffer;

    BeginPaint(GuiData->hWindow, &ps);
    if (ps.hdc != NULL &&
        ps.rcPaint.left < ps.rcPaint.right &&
        ps.rcPaint.top < ps.rcPaint.bottom)
    {
        EnterCriticalSection(&GuiData->Lock);

        /* Compose the current screen-buffer on-memory */
        if (GetType(ActiveBuffer) == TEXTMODE_BUFFER)
        {
            GuiPaintTextModeBuffer((PTEXTMODE_SCREEN_BUFFER)ActiveBuffer,
                                   GuiData, &ps.rcPaint, &rcPaint);
        }
        else /* if (GetType(ActiveBuffer) == GRAPHICS_BUFFER) */
        {
            GuiPaintGraphicsBuffer((PGRAPHICS_SCREEN_BUFFER)ActiveBuffer,
                                   GuiData, &ps.rcPaint, &rcPaint);
        }

        /* Send it to screen */
        BitBlt(ps.hdc,
               ps.rcPaint.left,
               ps.rcPaint.top,
               rcPaint.right  - rcPaint.left,
               rcPaint.bottom - rcPaint.top,
               GuiData->hMemDC,
               rcPaint.left,
               rcPaint.top,
               SRCCOPY);

        if (Console->Selection.dwFlags & CONSOLE_SELECTION_NOT_EMPTY)
        {
            SmallRectToRect(GuiData, &rcPaint, &Console->Selection.srSelection);

            /* Invert the selection */
            if (IntersectRect(&rcPaint, &ps.rcPaint, &rcPaint))
            {
                InvertRect(ps.hdc, &rcPaint);
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

static BOOL
IsSystemKey(WORD VirtualKeyCode)
{
    switch (VirtualKeyCode)
    {
        /* From MSDN, "Virtual-Key Codes" */
        case VK_RETURN:
        case VK_SHIFT:
        case VK_CONTROL:
        case VK_MENU:
        case VK_PAUSE:
        case VK_CAPITAL:
        case VK_ESCAPE:
        case VK_LWIN:
        case VK_RWIN:
        case VK_NUMLOCK:
        case VK_SCROLL:
            return TRUE;
        default:
            return FALSE;
    }
}

static VOID
GuiConsoleHandleKey(PGUI_CONSOLE_DATA GuiData, UINT msg, WPARAM wParam, LPARAM lParam)
{
    PCONSOLE Console = GuiData->Console;
    PCONSOLE_SCREEN_BUFFER ActiveBuffer;

    if (!ConDrvValidateConsoleUnsafe(Console, CONSOLE_RUNNING, TRUE)) return;

    ActiveBuffer = GuiData->ActiveBuffer;

    if (Console->Selection.dwFlags & CONSOLE_SELECTION_IN_PROGRESS)
    {
        WORD VirtualKeyCode = LOWORD(wParam);

        if (msg != WM_KEYDOWN) goto Quit;

        if (VirtualKeyCode == VK_RETURN)
        {
            /* Copy (and clear) selection if ENTER is pressed */
            GuiConsoleCopy(GuiData);
            goto Quit;
        }
        else if ( VirtualKeyCode == VK_ESCAPE ||
                 (VirtualKeyCode == 'C' && GetKeyState(VK_CONTROL) & 0x8000) )
        {
            /* Cancel selection if ESC or Ctrl-C are pressed */
            GuiConsoleUpdateSelection(Console, NULL);
            goto Quit;
        }

        if ((Console->Selection.dwFlags & CONSOLE_MOUSE_SELECTION) == 0)
        {
            /* Keyboard selection mode */
            BOOL Interpreted = FALSE;
            BOOL MajPressed  = (GetKeyState(VK_SHIFT) & 0x8000);

            switch (VirtualKeyCode)
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
            else if (!IsSystemKey(VirtualKeyCode))
            {
                /* Emit an error beep sound */
                SendNotifyMessage(GuiData->hWindow, PM_CONSOLE_BEEP, 0, 0);
            }

            goto Quit;
        }
        else
        {
            /* Mouse selection mode */

            if (!IsSystemKey(VirtualKeyCode))
            {
                /* Clear the selection and send the key into the input buffer */
                GuiConsoleUpdateSelection(Console, NULL);
            }
            else
            {
                goto Quit;
            }
        }
    }

    if ((Console->Selection.dwFlags & CONSOLE_SELECTION_IN_PROGRESS) == 0)
    {
        MSG Message;

        Message.hwnd = GuiData->hWindow;
        Message.message = msg;
        Message.wParam = wParam;
        Message.lParam = lParam;

        ConioProcessKey(Console, &Message);
    }

Quit:
    LeaveCriticalSection(&Console->Lock);
}


// FIXME: Remove after fixing GuiConsoleHandleTimer
VOID
GuiInvalidateCell(IN OUT PFRONTEND This, SHORT x, SHORT y);

static VOID
GuiConsoleHandleTimer(PGUI_CONSOLE_DATA GuiData)
{
    PCONSOLE Console = GuiData->Console;
    PCONSOLE_SCREEN_BUFFER Buff;

    SetTimer(GuiData->hWindow, CONGUI_UPDATE_TIMER, CURSOR_BLINK_TIME, NULL);

    if (!ConDrvValidateConsoleUnsafe(Console, CONSOLE_RUNNING, TRUE)) return;

    Buff = GuiData->ActiveBuffer;

    if (GetType(Buff) == TEXTMODE_BUFFER)
    {
        GuiInvalidateCell(&Console->TermIFace, Buff->CursorPosition.X, Buff->CursorPosition.Y);
        Buff->CursorBlinkOn = !Buff->CursorBlinkOn;

        if ((GuiData->OldCursor.x != Buff->CursorPosition.X) ||
            (GuiData->OldCursor.y != Buff->CursorPosition.Y))
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
                if ((Buff->CursorPosition.X < Buff->ViewOrigin.X) ||
                    (Buff->CursorPosition.X >= (Buff->ViewOrigin.X + Buff->ViewSize.X)))
                {
                    // Handle the horizontal scroll bar
                    if (Buff->CursorPosition.X >= Buff->ViewSize.X)
                        NewScrollX = Buff->CursorPosition.X - Buff->ViewSize.X + 1;
                    else
                        NewScrollX = 0;
                }
                else
                {
                    NewScrollX = OldScrollX;
                }
            }
            // If we successfully got the info for the vertical scrollbar
            if (OldScrollY >= 0)
            {
                if ((Buff->CursorPosition.Y < Buff->ViewOrigin.Y) ||
                    (Buff->CursorPosition.Y >= (Buff->ViewOrigin.Y + Buff->ViewSize.Y)))
                {
                    // Handle the vertical scroll bar
                    if (Buff->CursorPosition.Y >= Buff->ViewSize.Y)
                        NewScrollY = Buff->CursorPosition.Y - Buff->ViewSize.Y + 1;
                    else
                        NewScrollY = 0;
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
                // InvalidateRect(GuiData->hWindow, NULL, FALSE);
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

static BOOL
GuiConsoleHandleClose(PGUI_CONSOLE_DATA GuiData)
{
    PCONSOLE Console = GuiData->Console;

    if (!ConDrvValidateConsoleUnsafe(Console, CONSOLE_RUNNING, TRUE))
        return TRUE;

    // TODO: Prompt for termination ? (Warn the user about possible apps running in this console)

    /*
     * FIXME: Windows will wait up to 5 seconds for the thread to exit.
     * We shouldn't wait here, though, since the console lock is entered.
     * A copy of the thread list probably needs to be made.
     */
    ConDrvConsoleProcessCtrlEvent(Console, 0, CTRL_CLOSE_EVENT);

    LeaveCriticalSection(&Console->Lock);
    return FALSE;
}

static LRESULT
GuiConsoleHandleNcDestroy(HWND hWnd)
{
    PGUI_CONSOLE_DATA GuiData = GuiGetGuiData(hWnd);

    KillTimer(hWnd, CONGUI_UPDATE_TIMER);
    GetSystemMenu(hWnd, TRUE);

    if (GuiData)
    {
        /* Free the terminal framebuffer */
        if (GuiData->hMemDC ) DeleteDC(GuiData->hMemDC);
        if (GuiData->hBitmap) DeleteObject(GuiData->hBitmap);
        // if (GuiData->hSysPalette) DeleteObject(GuiData->hSysPalette);
        if (GuiData->Font) DeleteObject(GuiData->Font);
    }

    /* Free the GuiData registration */
    SetWindowLongPtrW(hWnd, GWLP_USERDATA, (DWORD_PTR)NULL);

    return DefWindowProcW(hWnd, WM_NCDESTROY, 0, 0);
}

static COORD
PointToCoord(PGUI_CONSOLE_DATA GuiData, LPARAM lParam)
{
    PCONSOLE_SCREEN_BUFFER Buffer = GuiData->ActiveBuffer;
    COORD Coord;
    UINT  WidthUnit, HeightUnit;

    GetScreenBufferSizeUnits(Buffer, GuiData, &WidthUnit, &HeightUnit);

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
            msg != WM_RBUTTONDOWN &&
            msg != WM_MOUSEMOVE)
        {
            /*
             * If this mouse signal is not a button-down action or a move,
             * then it is the last signal being ignored.
             */
            GuiData->IgnoreNextMouseSignal = FALSE;
        }
        else
        {
            /*
             * This mouse signal is a button-down action or a move.
             * Ignore it and perform default action.
             */
            Err = TRUE;
        }
        goto Quit;
    }

    if (!ConDrvValidateConsoleUnsafe(Console, CONSOLE_RUNNING, TRUE))
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
                /* Clear the old selection */
                // GuiConsoleUpdateSelection(Console, NULL);
                Console->Selection.dwFlags = CONSOLE_NO_SELECTION;

                /* Restart a new selection */
                Console->Selection.dwSelectionAnchor = PointToCoord(GuiData, lParam);
                SetCapture(GuiData->hWindow);
                Console->Selection.dwFlags |= CONSOLE_MOUSE_SELECTION | CONSOLE_MOUSE_DOWN;
                GuiConsoleUpdateSelection(Console, &Console->Selection.dwSelectionAnchor);

                break;
            }

            case WM_LBUTTONUP:
            {
                // COORD c;

                if (!(Console->Selection.dwFlags & CONSOLE_MOUSE_DOWN)) break;

                // c = PointToCoord(GuiData, lParam);
                Console->Selection.dwFlags &= ~CONSOLE_MOUSE_DOWN;
                // GuiConsoleUpdateSelection(Console, &c);
                ReleaseCapture();

                break;
            }

            case WM_LBUTTONDBLCLK:
            {
                PCONSOLE_SCREEN_BUFFER Buffer = GuiData->ActiveBuffer;

                if (GetType(Buffer) == TEXTMODE_BUFFER)
                {
#ifdef IS_WHITESPACE
#undef IS_WHITESPACE
#endif
#define IS_WHITESPACE(c)    \
    ((c) == L'\0' || (c) == L' ' || (c) == L'\t' || (c) == L'\r' || (c) == L'\n')

                    PTEXTMODE_SCREEN_BUFFER TextBuffer = (PTEXTMODE_SCREEN_BUFFER)Buffer;
                    COORD cL, cR;
                    PCHAR_INFO ptrL, ptrR;

                    /* Starting point */
                    cL = cR = PointToCoord(GuiData, lParam);
                    ptrL = ptrR = ConioCoordToPointer(TextBuffer, cL.X, cL.Y);

                    /* Enlarge the selection by checking for whitespace */
                    while ((0 < cL.X) && !IS_WHITESPACE(ptrL->Char.UnicodeChar)
                                      && !IS_WHITESPACE((ptrL-1)->Char.UnicodeChar))
                    {
                        --cL.X;
                        --ptrL;
                    }
                    while ((cR.X < TextBuffer->ScreenBufferSize.X - 1) &&
                           !IS_WHITESPACE(ptrR->Char.UnicodeChar)      &&
                           !IS_WHITESPACE((ptrR+1)->Char.UnicodeChar))
                    {
                        ++cR.X;
                        ++ptrR;
                    }

                    /*
                     * Update the selection started with the single
                     * left-click that preceded this double-click.
                     */
                    Console->Selection.dwSelectionAnchor = cL;
                    Console->dwSelectionCursor           = cR;

                    Console->Selection.dwFlags |= CONSOLE_MOUSE_SELECTION | CONSOLE_MOUSE_DOWN;
                    GuiConsoleUpdateSelection(Console, &Console->dwSelectionCursor);

                    /* Ignore the next mouse move signal */
                    GuiData->IgnoreNextMouseSignal = TRUE;
                }

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
                }

                /* Ignore the next mouse move signal */
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
                Err = FALSE; // TRUE;
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

VOID
GuiCopyFromTextModeBuffer(PTEXTMODE_SCREEN_BUFFER Buffer,
                          PGUI_CONSOLE_DATA GuiData);
VOID
GuiCopyFromGraphicsBuffer(PGRAPHICS_SCREEN_BUFFER Buffer,
                          PGUI_CONSOLE_DATA GuiData);

static VOID
GuiConsoleCopy(PGUI_CONSOLE_DATA GuiData)
{
    PCONSOLE Console = GuiData->Console;

    if (OpenClipboard(GuiData->hWindow) == TRUE)
    {
        PCONSOLE_SCREEN_BUFFER Buffer = GuiData->ActiveBuffer;

        if (GetType(Buffer) == TEXTMODE_BUFFER)
        {
            GuiCopyFromTextModeBuffer((PTEXTMODE_SCREEN_BUFFER)Buffer, GuiData);
        }
        else /* if (GetType(Buffer) == GRAPHICS_BUFFER) */
        {
            GuiCopyFromGraphicsBuffer((PGRAPHICS_SCREEN_BUFFER)Buffer, GuiData);
        }

        CloseClipboard();
    }

    /* Clear the selection */
    GuiConsoleUpdateSelection(Console, NULL);
}

VOID
GuiPasteToTextModeBuffer(PTEXTMODE_SCREEN_BUFFER Buffer,
                         PGUI_CONSOLE_DATA GuiData);
VOID
GuiPasteToGraphicsBuffer(PGRAPHICS_SCREEN_BUFFER Buffer,
                         PGUI_CONSOLE_DATA GuiData);

static VOID
GuiConsolePaste(PGUI_CONSOLE_DATA GuiData)
{
    if (OpenClipboard(GuiData->hWindow) == TRUE)
    {
        PCONSOLE_SCREEN_BUFFER Buffer = GuiData->ActiveBuffer;

        if (GetType(Buffer) == TEXTMODE_BUFFER)
        {
            GuiPasteToTextModeBuffer((PTEXTMODE_SCREEN_BUFFER)Buffer, GuiData);
        }
        else /* if (GetType(Buffer) == GRAPHICS_BUFFER) */
        {
            GuiPasteToGraphicsBuffer((PGRAPHICS_SCREEN_BUFFER)Buffer, GuiData);
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

    if (!ConDrvValidateConsoleUnsafe(Console, CONSOLE_RUNNING, TRUE)) return;

    ActiveBuffer = GuiData->ActiveBuffer;

    GetScreenBufferSizeUnits(ActiveBuffer, GuiData, &WidthUnit, &HeightUnit);

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

    if (!ConDrvValidateConsoleUnsafe(Console, CONSOLE_RUNNING, TRUE)) return;

    if ((GuiData->WindowSizeLock == FALSE) &&
        (wParam == SIZE_RESTORED || wParam == SIZE_MAXIMIZED || wParam == SIZE_MINIMIZED))
    {
        PCONSOLE_SCREEN_BUFFER Buff = GuiData->ActiveBuffer;
        DWORD windx, windy, charx, chary;
        UINT  WidthUnit, HeightUnit;

        GetScreenBufferSizeUnits(Buff, GuiData, &WidthUnit, &HeightUnit);

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

        GuiConsoleResizeWindow(GuiData, WidthUnit, HeightUnit);

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

    if (!ConDrvValidateConsoleUnsafe(Console, CONSOLE_RUNNING, TRUE)) return 0;

    Buff = GuiData->ActiveBuffer;

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

        GetScreenBufferSizeUnits(Buff, GuiData, &WidthUnit, &HeightUnit);

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
        // InvalidateRect(GuiData->hWindow, NULL, FALSE);
    }

Quit:
    LeaveCriticalSection(&Console->Lock);
    return 0;
}


BOOL
EnterFullScreen(PGUI_CONSOLE_DATA GuiData);
VOID
LeaveFullScreen(PGUI_CONSOLE_DATA GuiData);
VOID
SwitchFullScreen(PGUI_CONSOLE_DATA GuiData, BOOL FullScreen);
VOID
GuiConsoleSwitchFullScreen(PGUI_CONSOLE_DATA GuiData);

static LRESULT CALLBACK
ConWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
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
    if (GuiData == NULL) return DefWindowProcW(hWnd, msg, wParam, lParam);

    // TEMPORARY HACK until all of the functions can deal with a NULL GuiData->ActiveBuffer ...
    if (GuiData->ActiveBuffer == NULL) return DefWindowProcW(hWnd, msg, wParam, lParam);

    /*
     * Just retrieve a pointer to the console in case somebody needs it.
     * It is not NULL because it was checked in GuiGetGuiData.
     * Each helper function which needs the console has to validate and lock it.
     */
    Console = GuiData->Console;

    /* We have a console, start message dispatching */
    switch (msg)
    {
        case WM_ACTIVATE:
        {
            WORD ActivationState = LOWORD(wParam);

            DPRINT1("WM_ACTIVATE - ActivationState = %d\n");

            if ( ActivationState == WA_ACTIVE ||
                 ActivationState == WA_CLICKACTIVE )
            {
                if (GuiData->GuiInfo.FullScreen)
                {
                    EnterFullScreen(GuiData);
                    // // PostMessageW(GuiData->hWindow, WM_SYSCOMMAND, SC_RESTORE, 0);
                    // SendMessageW(GuiData->hWindow, WM_SYSCOMMAND, SC_RESTORE, 0);
                }
            }
            else // if (ActivationState == WA_INACTIVE)
            {
                if (GuiData->GuiInfo.FullScreen)
                {
                    SendMessageW(GuiData->hWindow, WM_SYSCOMMAND, SC_MINIMIZE, 0);
                    LeaveFullScreen(GuiData);
                    // // PostMessageW(GuiData->hWindow, WM_SYSCOMMAND, SC_MINIMIZE, 0);
                    // SendMessageW(GuiData->hWindow, WM_SYSCOMMAND, SC_MINIMIZE, 0);
                }
            }

            /*
             * When we are in QuickEdit mode, ignore the next mouse signal
             * when we are going to be enabled again via the mouse, in order
             * to prevent e.g. an erroneous right-click from the user which
             * would have as an effect to paste some unwanted text...
             */
            if (Console->QuickEdit && (ActivationState == WA_CLICKACTIVE))
                GuiData->IgnoreNextMouseSignal = TRUE;

            break;
        }

        case WM_CLOSE:
            if (GuiConsoleHandleClose(GuiData)) goto Default;
            break;

        case WM_PAINT:
            GuiConsoleHandlePaint(GuiData);
            break;

        case WM_TIMER:
            GuiConsoleHandleTimer(GuiData);
            break;

        case WM_PALETTECHANGED:
        {
            PCONSOLE_SCREEN_BUFFER ActiveBuffer = GuiData->ActiveBuffer;

            DPRINT("WM_PALETTECHANGED called\n");

            /*
             * Protects against infinite loops:
             * "... A window that receives this message must not realize
             * its palette, unless it determines that wParam does not contain
             * its own window handle." (WM_PALETTECHANGED description - MSDN)
             *
             * This message is sent to all windows, including the one that
             * changed the system palette and caused this message to be sent.
             * The wParam of this message contains the handle of the window
             * that caused the system palette to change. To avoid an infinite
             * loop, care must be taken to check that the wParam of this message
             * does not match the window's handle.
             */
            if ((HWND)wParam == hWnd) break;

            DPRINT("WM_PALETTECHANGED ok\n");

            // if (GetType(ActiveBuffer) == GRAPHICS_BUFFER)
            if (ActiveBuffer->PaletteHandle)
            {
                DPRINT("WM_PALETTECHANGED changing palette\n");

                /* Specify the use of the system palette for the framebuffer */
                SetSystemPaletteUse(GuiData->hMemDC, ActiveBuffer->PaletteUsage);

                /* Realize the (logical) palette */
                RealizePalette(GuiData->hMemDC);
            }

            DPRINT("WM_PALETTECHANGED quit\n");

            break;
        }

        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_CHAR:
        case WM_DEADCHAR:
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_SYSCHAR:
        case WM_SYSDEADCHAR:
        {
            /* Detect Alt-Enter presses and switch back and forth to fullscreen mode */
            if (msg == WM_SYSKEYDOWN && (HIWORD(lParam) & KF_ALTDOWN) && wParam == VK_RETURN)
            {
                /* Switch only at first Alt-Enter press, and ignore subsequent key repetitions */
                if ((HIWORD(lParam) & (KF_UP | KF_REPEAT)) != KF_REPEAT)
                    GuiConsoleSwitchFullScreen(GuiData);

                break;
            }

            GuiConsoleHandleKey(GuiData, msg, wParam, lParam);
            break;
        }

        case WM_SETCURSOR:
        {
            /*
             * The message was sent because we are manually triggering a change.
             * Check whether the mouse is indeed present on this console window
             * and take appropriate decisions.
             */
            if (wParam == -1 && lParam == -1)
            {
                POINT mouseCoords;
                HWND  hWndHit;

                /* Get the placement of the mouse */
                GetCursorPos(&mouseCoords);

                /* On which window is placed the mouse ? */
                hWndHit = WindowFromPoint(mouseCoords);

                /* It's our window. Perform the hit-test to be used later on. */
                if (hWndHit == hWnd)
                {
                    wParam = (WPARAM)hWnd;
                    lParam = DefWindowProcW(hWndHit, WM_NCHITTEST, 0,
                                            MAKELPARAM(mouseCoords.x, mouseCoords.y));
                }
            }

            /* Set the mouse cursor only when we are in the client area */
            if ((HWND)wParam == hWnd && LOWORD(lParam) == HTCLIENT)
            {
                if (GuiData->MouseCursorRefCount >= 0)
                {
                    /* Show the cursor */
                    SetCursor(GuiData->hCursor);
                }
                else
                {
                    /* Hide the cursor if the reference count is negative */
                    SetCursor(NULL);
                }
                return TRUE;
            }
            else
            {
                goto Default;
            }
        }

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
             * See: http://git.reactos.org/?p=reactos.git;a=blob;f=reactos/win32ss/user/user32/windows/defwnd.c;hb=332bc8f482f40fd05ab510f78276576719fbfba8#l1103
             * and line 1135 too.
             */
#if 0
            if (DefWindowProcW(hWnd, WM_NCHITTEST, 0, lParam) == HTCAPTION)
            {
                /* Call DefWindowProcW with the WM_CONTEXTMENU message */
                msg = WM_CONTEXTMENU;
            }
#endif
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

        case WM_INITMENU:
        {
            HMENU hMenu = (HMENU)wParam;
            if (hMenu != NULL)
            {
                /* Enable or disable the Close menu item */
                EnableMenuItem(hMenu, SC_CLOSE, MF_BYCOMMAND |
                               (GuiData->IsCloseButtonEnabled ? MF_ENABLED : MF_GRAYED));

                /* Enable or disable the Copy and Paste items */
                EnableMenuItem(hMenu, ID_SYSTEM_EDIT_COPY , MF_BYCOMMAND |
                               ((Console->Selection.dwFlags & CONSOLE_SELECTION_IN_PROGRESS) &&
                                (Console->Selection.dwFlags & CONSOLE_SELECTION_NOT_EMPTY) ? MF_ENABLED : MF_GRAYED));
                // FIXME: Following whether the active screen buffer is text-mode
                // or graphics-mode, search for CF_UNICODETEXT or CF_BITMAP formats.
                EnableMenuItem(hMenu, ID_SYSTEM_EDIT_PASTE, MF_BYCOMMAND |
                               (!(Console->Selection.dwFlags & CONSOLE_SELECTION_IN_PROGRESS) &&
                                IsClipboardFormatAvailable(CF_UNICODETEXT) ? MF_ENABLED : MF_GRAYED));
            }

            if (ConDrvValidateConsoleUnsafe(Console, CONSOLE_RUNNING, TRUE))
            {
                GuiSendMenuEvent(Console, WM_INITMENU);
                LeaveCriticalSection(&Console->Lock);
            }
            break;
        }

        case WM_MENUSELECT:
        {
            if (HIWORD(wParam) == 0xFFFF) // Allow all the menu flags
            {
                if (ConDrvValidateConsoleUnsafe(Console, CONSOLE_RUNNING, TRUE))
                {
                    GuiSendMenuEvent(Console, WM_MENUSELECT);
                    LeaveCriticalSection(&Console->Lock);
                }
            }
            break;
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
            if (ConDrvValidateConsoleUnsafe(Console, CONSOLE_RUNNING, TRUE))
            {
                BOOL SetFocus = (msg == WM_SETFOCUS);
                INPUT_RECORD er;

                er.EventType = FOCUS_EVENT;
                er.Event.FocusEvent.bSetFocus = SetFocus;
                ConioProcessInputEvent(Console, &er);

                if (SetFocus)
                    DPRINT1("TODO: Create console caret\n");
                else
                    DPRINT1("TODO: Destroy console caret\n");

                LeaveCriticalSection(&Console->Lock);
            }
            break;
        }

        case WM_GETMINMAXINFO:
            GuiConsoleGetMinMaxInfo(GuiData, (PMINMAXINFO)lParam);
            break;

        case WM_MOVE:
        {
            if (ConDrvValidateConsoleUnsafe(Console, CONSOLE_RUNNING, TRUE))
            {
                // TODO: Simplify the code.
                // See: GuiConsoleNotifyWndProc() PM_CREATE_CONSOLE.

                RECT rcWnd;

                /* Retrieve our real position */
                GetWindowRect(GuiData->hWindow, &rcWnd);
                GuiData->GuiInfo.WindowOrigin.x = rcWnd.left;
                GuiData->GuiInfo.WindowOrigin.y = rcWnd.top;

                LeaveCriticalSection(&Console->Lock);
            }
            break;
        }

        case WM_SIZE:
            GuiConsoleResize(GuiData, wParam, lParam);
            break;

        case PM_RESIZE_TERMINAL:
        {
            PCONSOLE_SCREEN_BUFFER Buff = GuiData->ActiveBuffer;
            HDC hDC;
            HBITMAP hnew, hold;

            DWORD Width, Height;
            UINT  WidthUnit, HeightUnit;

            GetScreenBufferSizeUnits(Buff, GuiData, &WidthUnit, &HeightUnit);

            Width  = Buff->ScreenBufferSize.X * WidthUnit ;
            Height = Buff->ScreenBufferSize.Y * HeightUnit;

            /* Recreate the framebuffer */
            hDC  = GetDC(GuiData->hWindow);
            hnew = CreateCompatibleBitmap(hDC, Width, Height);
            ReleaseDC(GuiData->hWindow, hDC);
            hold = SelectObject(GuiData->hMemDC, hnew);
            if (GuiData->hBitmap)
            {
                if (hold == GuiData->hBitmap) DeleteObject(GuiData->hBitmap);
            }
            GuiData->hBitmap = hnew;

            /* Resize the window to the user's values */
            GuiData->WindowSizeLock = TRUE;
            GuiConsoleResizeWindow(GuiData, WidthUnit, HeightUnit);
            GuiData->WindowSizeLock = FALSE;
            break;
        }

        case PM_APPLY_CONSOLE_INFO:
        {
            if (ConDrvValidateConsoleUnsafe(Console, CONSOLE_RUNNING, TRUE))
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

/* EOF */
