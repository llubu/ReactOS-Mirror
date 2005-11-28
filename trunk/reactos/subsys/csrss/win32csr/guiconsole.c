/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            subsys/csrss/win32csr/guiconsole.c
 * PURPOSE:         Implementation of gui-mode consoles
 */

/* INCLUDES ******************************************************************/

#include "w32csr.h"

#define NDEBUG
#include <debug.h>

/* Not defined in any header file */
extern VOID STDCALL PrivateCsrssManualGuiCheck(LONG Check);

/* GLOBALS *******************************************************************/

typedef struct GUI_CONSOLE_DATA_TAG
{
  HFONT Font;
  unsigned CharWidth;
  unsigned CharHeight;
  PWCHAR LineBuffer;
  BOOL CursorBlinkOn;
  BOOL ForceCursorOff;
  CRITICAL_SECTION Lock;
  RECT Selection;
  POINT SelectionStart;
  BOOL MouseDown;
} GUI_CONSOLE_DATA, *PGUI_CONSOLE_DATA;

#ifndef WM_APP
#define WM_APP 0x8000
#endif
#define PM_CREATE_CONSOLE  (WM_APP + 1)
#define PM_DESTROY_CONSOLE (WM_APP + 2)

#define CURSOR_BLINK_TIME 500

static BOOL ConsInitialized = FALSE;
static HWND NotifyWnd;

/* FUNCTIONS *****************************************************************/

static VOID FASTCALL
GuiConsoleGetDataPointers(HWND hWnd, PCSRSS_CONSOLE *Console, PGUI_CONSOLE_DATA *GuiData)
{
  *Console = (PCSRSS_CONSOLE) GetWindowLongPtrW(hWnd, GWL_USERDATA);
  *GuiData = (NULL == *Console ? NULL : (*Console)->PrivateData);
}

static BOOL FASTCALL
GuiConsoleHandleNcCreate(HWND hWnd, CREATESTRUCTW *Create)
{
  RECT Rect;
  PCSRSS_CONSOLE Console = (PCSRSS_CONSOLE) Create->lpCreateParams;
  PGUI_CONSOLE_DATA GuiData;
  HDC Dc;
  HFONT OldFont;
  TEXTMETRICW Metrics;

  GuiData = HeapAlloc(Win32CsrApiHeap, HEAP_ZERO_MEMORY,
                      sizeof(GUI_CONSOLE_DATA) +
                      (Console->Size.X + 1) * sizeof(WCHAR));
  if (NULL == GuiData)
    {
      DPRINT1("GuiConsoleNcCreate: HeapAlloc failed\n");
      return FALSE;
    }

  InitializeCriticalSection(&GuiData->Lock);

  GuiData->LineBuffer = (PWCHAR)(GuiData + 1);

  GuiData->Font = CreateFontW(12, 0, 0, TA_BASELINE, FW_NORMAL,
                              FALSE, FALSE, FALSE, OEM_CHARSET,
                              OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                              NONANTIALIASED_QUALITY, FIXED_PITCH | FF_DONTCARE,
                              L"Bitstream Vera Sans Mono");
  if (NULL == GuiData->Font)
    {
      DPRINT1("GuiConsoleNcCreate: CreateFont failed\n");
      DeleteCriticalSection(&GuiData->Lock);
      HeapFree(Win32CsrApiHeap, 0, GuiData);
      return FALSE;
    }
  Dc = GetDC(hWnd);
  if (NULL == Dc)
    {
      DPRINT1("GuiConsoleNcCreate: GetDC failed\n");
      DeleteObject(GuiData->Font);
      DeleteCriticalSection(&GuiData->Lock);
      HeapFree(Win32CsrApiHeap, 0, GuiData);
      return FALSE;
    }
  OldFont = SelectObject(Dc, GuiData->Font);
  if (NULL == OldFont)
    {
      DPRINT1("GuiConsoleNcCreate: SelectObject failed\n");
      ReleaseDC(hWnd, Dc);
      DeleteObject(GuiData->Font);
      DeleteCriticalSection(&GuiData->Lock);
      HeapFree(Win32CsrApiHeap, 0, GuiData);
      return FALSE;
    }
  if (! GetTextMetricsW(Dc, &Metrics))
    {
      DPRINT1("GuiConsoleNcCreate: GetTextMetrics failed\n");
      SelectObject(Dc, OldFont);
      ReleaseDC(hWnd, Dc);
      DeleteObject(GuiData->Font);
      DeleteCriticalSection(&GuiData->Lock);
      HeapFree(Win32CsrApiHeap, 0, GuiData);
      return FALSE;
    }
  GuiData->CharWidth = Metrics.tmMaxCharWidth;
  GuiData->CharHeight = Metrics.tmHeight + Metrics.tmExternalLeading;
  SelectObject(Dc, OldFont);

  ReleaseDC(hWnd, Dc);
  GuiData->CursorBlinkOn = TRUE;
  GuiData->ForceCursorOff = FALSE;

  GuiData->Selection.left = -1;

  Console->PrivateData = GuiData;
  SetWindowLongPtrW(hWnd, GWL_USERDATA, (DWORD_PTR) Console);

  GetWindowRect(hWnd, &Rect);
  Rect.right = Rect.left + Console->Size.X * GuiData->CharWidth +
               2 * GetSystemMetrics(SM_CXFIXEDFRAME);
  Rect.bottom = Rect.top + Console->Size.Y * GuiData->CharHeight +
               2 * GetSystemMetrics(SM_CYFIXEDFRAME) + GetSystemMetrics(SM_CYCAPTION);
  MoveWindow(hWnd, Rect.left, Rect.top, Rect.right - Rect.left,
             Rect.bottom - Rect.top, FALSE);

  SetTimer(hWnd, 1, CURSOR_BLINK_TIME, NULL);

  return (BOOL) DefWindowProcW(hWnd, WM_NCCREATE, 0, (LPARAM) Create);
}

static COLORREF FASTCALL
GuiConsoleRGBFromAttribute(BYTE Attribute)
{
  int Red = (Attribute & 0x04 ? (Attribute & 0x08 ? 0xff : 0x80) : 0x00);
  int Green = (Attribute & 0x02 ? (Attribute & 0x08 ? 0xff : 0x80) : 0x00);
  int Blue = (Attribute & 0x01 ? (Attribute & 0x08 ? 0xff : 0x80) : 0x00);

  return RGB(Red, Green, Blue);
}

static VOID FASTCALL
GuiConsoleSetTextColors(HDC Dc, BYTE Attribute)
{
  SetTextColor(Dc, GuiConsoleRGBFromAttribute(Attribute & 0x0f));
  SetBkColor(Dc, GuiConsoleRGBFromAttribute((Attribute & 0xf0) >> 4));
}

static VOID FASTCALL
GuiConsoleGetLogicalCursorPos(PCSRSS_SCREEN_BUFFER Buff, ULONG *CursorX, ULONG *CursorY)
{
  *CursorX = Buff->CurrentX;
  if (Buff->CurrentY < Buff->ShowY)
    {
      *CursorY = Buff->MaxY - Buff->ShowY + Buff->CurrentY;
    }
  else
    {
      *CursorY = Buff->CurrentY - Buff->ShowY;
    }
}


static VOID FASTCALL
GuiConsoleUpdateSelection(HWND hWnd, PRECT rc, PGUI_CONSOLE_DATA GuiData)
{
  RECT oldRect = GuiData->Selection;

  if(rc != NULL)
  {
    RECT changeRect = *rc;

    GuiData->Selection = *rc;

    changeRect.left *= GuiData->CharWidth;
    changeRect.top *= GuiData->CharHeight;
    changeRect.right *= GuiData->CharWidth;
    changeRect.bottom *= GuiData->CharHeight;

    if(rc->left != oldRect.left ||
       rc->top != oldRect.top ||
       rc->right != oldRect.right ||
       rc->bottom != oldRect.bottom)
    {
      if(oldRect.left != -1)
      {
        HRGN rgn1, rgn2;

        oldRect.left *= GuiData->CharWidth;
        oldRect.top *= GuiData->CharHeight;
        oldRect.right *= GuiData->CharWidth;
        oldRect.bottom *= GuiData->CharHeight;

        /* calculate the region that needs to be updated */
        if((rgn1 = CreateRectRgnIndirect(&oldRect)))
        {
          if((rgn2 = CreateRectRgnIndirect(&changeRect)))
          {
            if(CombineRgn(rgn1, rgn2, rgn1, RGN_XOR) != ERROR)
            {
              InvalidateRgn(hWnd, rgn1, FALSE);
            }

            DeleteObject(rgn2);
          }
          DeleteObject(rgn1);
        }
      }
      else
      {
        InvalidateRect(hWnd, &changeRect, FALSE);
      }
    }
  }
  else if(oldRect.left != -1)
  {
    /* clear the selection */
    GuiData->Selection.left = -1;
    oldRect.left *= GuiData->CharWidth;
    oldRect.top *= GuiData->CharHeight;
    oldRect.right *= GuiData->CharWidth;
    oldRect.bottom *= GuiData->CharHeight;
    InvalidateRect(hWnd, &oldRect, FALSE);
  }
}


static VOID FASTCALL
GuiConsolePaint(PCSRSS_CONSOLE Console,
                PGUI_CONSOLE_DATA GuiData,
                HDC hDC,
                PRECT rc)
{
    PCSRSS_SCREEN_BUFFER Buff;
    ULONG TopLine, BottomLine, LeftChar, RightChar;
    ULONG Line, Char, Start;
    PBYTE From;
    PWCHAR To;
    BYTE LastAttribute, Attribute;
    ULONG CursorX, CursorY, CursorHeight;
    HBRUSH CursorBrush, OldBrush;
    HFONT OldFont;

    Buff = Console->ActiveBuffer;

    TopLine = rc->top / GuiData->CharHeight;
    BottomLine = (rc->bottom + (GuiData->CharHeight - 1)) / GuiData->CharHeight - 1;
    LeftChar = rc->left / GuiData->CharWidth;
    RightChar = (rc->right + (GuiData->CharWidth - 1)) / GuiData->CharWidth - 1;
    LastAttribute = Buff->Buffer[(TopLine * Buff->MaxX + LeftChar) * 2 + 1];

    GuiConsoleSetTextColors(hDC,
                            LastAttribute);

    EnterCriticalSection(&Buff->Header.Lock);

    OldFont = SelectObject(hDC,
                           GuiData->Font);

    for (Line = TopLine; Line <= BottomLine; Line++)
    {
        if (Line + Buff->ShowY < Buff->MaxY)
        {
            From = Buff->Buffer + ((Line + Buff->ShowY) * Buff->MaxX + LeftChar) * 2;
        }
        else
        {
            From = Buff->Buffer +
                   ((Line - (Buff->MaxY - Buff->ShowY)) * Buff->MaxX + LeftChar) * 2;
        }
        Start = LeftChar;
        To = GuiData->LineBuffer;

        for (Char = LeftChar; Char <= RightChar; Char++)
        {
            if (*(From + 1) != LastAttribute)
            {
                TextOutW(hDC,
                         Start * GuiData->CharWidth,
                         Line * GuiData->CharHeight,
                         GuiData->LineBuffer,
                         Char - Start);
                Start = Char;
                To = GuiData->LineBuffer;
                Attribute = *(From + 1);
                if (Attribute != LastAttribute)
                {
                    GuiConsoleSetTextColors(hDC,
                                            Attribute);
                    LastAttribute = Attribute;
                }
            }

            MultiByteToWideChar(Console->OutputCodePage,
                                0,
                                (PCHAR)From,
                                1,
                                To,
                                1);
            To++;
            From += 2;
        }

        TextOutW(hDC,
                 Start * GuiData->CharWidth,
                 Line * GuiData->CharHeight,
                 GuiData->LineBuffer,
                 RightChar - Start + 1);
    }

    if (Buff->CursorInfo.bVisible && GuiData->CursorBlinkOn &&
        !GuiData->ForceCursorOff)
    {
        GuiConsoleGetLogicalCursorPos(Buff,
                                      &CursorX,
                                      &CursorY);
        if (LeftChar <= CursorX && CursorX <= RightChar &&
            TopLine <= CursorY && CursorY <= BottomLine)
        {
            CursorHeight = (GuiData->CharHeight * Buff->CursorInfo.dwSize) / 100;
            if (CursorHeight < 1)
            {
                CursorHeight = 1;
            }
            From = Buff->Buffer + (Buff->CurrentY * Buff->MaxX + Buff->CurrentX) * 2 + 1;
            CursorBrush = CreateSolidBrush(GuiConsoleRGBFromAttribute(*From));
            OldBrush = SelectObject(hDC,
                                    CursorBrush);
            PatBlt(hDC,
                   CursorX * GuiData->CharWidth,
                   CursorY * GuiData->CharHeight + (GuiData->CharHeight - CursorHeight),
                   GuiData->CharWidth,
                   CursorHeight,
                   PATCOPY);
            SelectObject(hDC,
                         OldBrush);
            DeleteObject(CursorBrush);
        }
    }

    LeaveCriticalSection(&Buff->Header.Lock);

    SelectObject(hDC,
                 OldFont);
}

static VOID FASTCALL
GuiConsoleHandlePaint(HWND hWnd, HDC hDCPaint)
{
    RECT rcUpdate;
    HDC hDC;
    PAINTSTRUCT ps;
    PCSRSS_CONSOLE Console;
    PGUI_CONSOLE_DATA GuiData;

    if (GetUpdateRect(hWnd,
                      &rcUpdate,
                      FALSE))
    {
        hDC = (hDCPaint != NULL ? hDCPaint : BeginPaint(hWnd,
                                                        &ps));
        if (hDC != NULL)
        {
            GuiConsoleGetDataPointers(hWnd,
                                      &Console,
                                      &GuiData);
            if (Console != NULL && GuiData != NULL &&
                Console->ActiveBuffer != NULL)
            {
                EnterCriticalSection(&GuiData->Lock);

                GuiConsolePaint(Console,
                                GuiData,
                                hDC,
                                &rcUpdate);

                if (GuiData->Selection.left != -1)
                {
                    RECT rc = GuiData->Selection;

                    rc.left *= GuiData->CharWidth;
                    rc.top *= GuiData->CharHeight;
                    rc.right *= GuiData->CharWidth;
                    rc.bottom *= GuiData->CharHeight;

                    /* invert the selection */
                    if (IntersectRect(&rc,
                                      &rcUpdate,
                                      &rc))
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

            if (hDCPaint == NULL)
            {
                EndPaint(hWnd,
                         &ps);
            }
        }
    }
}

static VOID FASTCALL
GuiConsoleHandleKey(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  PCSRSS_CONSOLE Console;
  PGUI_CONSOLE_DATA GuiData;
  MSG Message;

  GuiConsoleGetDataPointers(hWnd, &Console, &GuiData);
  Message.hwnd = hWnd;
  Message.message = msg;
  Message.wParam = wParam;
  Message.lParam = lParam;

  if(msg == WM_CHAR || msg == WM_SYSKEYDOWN)
  {
    /* clear the selection */
    GuiConsoleUpdateSelection(hWnd, NULL, GuiData);
  }

  ConioProcessKey(&Message, Console, FALSE);
}

static VOID FASTCALL
GuiIntDrawRegion(PGUI_CONSOLE_DATA GuiData, HWND Wnd, RECT *Region)
{
  RECT RegionRect;

  RegionRect.left = Region->left * GuiData->CharWidth;
  RegionRect.top = Region->top * GuiData->CharHeight;
  RegionRect.right = (Region->right + 1) * GuiData->CharWidth;
  RegionRect.bottom = (Region->bottom + 1) * GuiData->CharHeight;

  InvalidateRect(Wnd, &RegionRect, FALSE);
}

static VOID STDCALL
GuiDrawRegion(PCSRSS_CONSOLE Console, RECT *Region)
{
  PGUI_CONSOLE_DATA GuiData = (PGUI_CONSOLE_DATA) Console->PrivateData;

  if (NULL != Console->hWindow && NULL != GuiData)
    {
      GuiIntDrawRegion(GuiData, Console->hWindow, Region);
    }
}

static VOID FASTCALL
GuiInvalidateCell(PGUI_CONSOLE_DATA GuiData, HWND Wnd, UINT x, UINT y)
{
  RECT CellRect;

  CellRect.left = x;
  CellRect.top = y;
  CellRect.right = x;
  CellRect.bottom = y;

  GuiIntDrawRegion(GuiData, Wnd, &CellRect);
}

static VOID STDCALL
GuiWriteStream(PCSRSS_CONSOLE Console, RECT *Region, LONG CursorStartX, LONG CursorStartY,
               UINT ScrolledLines, CHAR *Buffer, UINT Length)
{
  PGUI_CONSOLE_DATA GuiData = (PGUI_CONSOLE_DATA) Console->PrivateData;
  PCSRSS_SCREEN_BUFFER Buff = Console->ActiveBuffer;
  LONG CursorEndX, CursorEndY;
  RECT ScrollRect;

  if (NULL == Console->hWindow || NULL == GuiData)
    {
      return;
    }

  if (0 != ScrolledLines)
    {
      ScrollRect.left = 0;
      ScrollRect.top = 0;
      ScrollRect.right = Console->Size.X * GuiData->CharWidth;
      ScrollRect.bottom = Region->top * GuiData->CharHeight;

      ScrollWindowEx(Console->hWindow,
                     0,
                     -(ScrolledLines * GuiData->CharHeight),
                     &ScrollRect,
                     NULL,
                     NULL,
                     NULL,
                     SW_INVALIDATE);
    }

  GuiIntDrawRegion(GuiData, Console->hWindow, Region);

  if (CursorStartX < Region->left || Region->right < CursorStartX
      || CursorStartY < Region->top || Region->bottom < CursorStartY)
    {
      GuiInvalidateCell(GuiData, Console->hWindow, CursorStartX, CursorStartY);
    }

  ConioPhysicalToLogical(Buff, Buff->CurrentX, Buff->CurrentY,
                         &CursorEndX, &CursorEndY);
  if ((CursorEndX < Region->left || Region->right < CursorEndX
       || CursorEndY < Region->top || Region->bottom < CursorEndY)
      && (CursorEndX != CursorStartX || CursorEndY != CursorStartY))
    {
      GuiInvalidateCell(GuiData, Console->hWindow, CursorEndX, CursorEndY);
    }
}

static BOOL STDCALL
GuiSetCursorInfo(PCSRSS_CONSOLE Console, PCSRSS_SCREEN_BUFFER Buff)
{
  RECT UpdateRect;

  if (Console->ActiveBuffer == Buff)
    {
      ConioPhysicalToLogical(Buff, Buff->CurrentX, Buff->CurrentY,
                             &UpdateRect.left, &UpdateRect.top);
      UpdateRect.right = UpdateRect.left;
      UpdateRect.bottom = UpdateRect.top;
      ConioDrawRegion(Console, &UpdateRect);
    }

  return TRUE;
}

static BOOL STDCALL
GuiSetScreenInfo(PCSRSS_CONSOLE Console, PCSRSS_SCREEN_BUFFER Buff, UINT OldCursorX, UINT OldCursorY)
{
  RECT UpdateRect;

  if (Console->ActiveBuffer == Buff)
    {
      /* Redraw char at old position (removes cursor) */
      UpdateRect.left = OldCursorX;
      UpdateRect.top = OldCursorY;
      UpdateRect.right = OldCursorX;
      UpdateRect.bottom = OldCursorY;
      ConioDrawRegion(Console, &UpdateRect);
      /* Redraw char at new position (shows cursor) */
      ConioPhysicalToLogical(Buff, Buff->CurrentX, Buff->CurrentY,
                             &(UpdateRect.left), &(UpdateRect.top));
      UpdateRect.right = UpdateRect.left;
      UpdateRect.bottom = UpdateRect.top;
      ConioDrawRegion(Console, &UpdateRect);
    }

  return TRUE;
}

static VOID FASTCALL
GuiConsoleHandleTimer(HWND hWnd)
{
  PCSRSS_CONSOLE Console;
  PGUI_CONSOLE_DATA GuiData;
  RECT CursorRect;
  ULONG CursorX, CursorY;

  GuiConsoleGetDataPointers(hWnd, &Console, &GuiData);
  GuiData->CursorBlinkOn = ! GuiData->CursorBlinkOn;

  GuiConsoleGetLogicalCursorPos(Console->ActiveBuffer, &CursorX, &CursorY);
  CursorRect.left = CursorX;
  CursorRect.top = CursorY;
  CursorRect.right = CursorX;
  CursorRect.bottom = CursorY;
  GuiDrawRegion(Console, &CursorRect);
}

static VOID FASTCALL
GuiConsoleHandleClose(HWND hWnd)
{
  PCSRSS_CONSOLE Console;
  PGUI_CONSOLE_DATA GuiData;
  PLIST_ENTRY current_entry;
  PCSRSS_PROCESS_DATA current;

  GuiConsoleGetDataPointers(hWnd, &Console, &GuiData);

  EnterCriticalSection(&Console->Header.Lock);

  current_entry = Console->ProcessList.Flink;
  while (current_entry != &Console->ProcessList)
    {
      current = CONTAINING_RECORD(current_entry, CSRSS_PROCESS_DATA, ProcessEntry);
      current_entry = current_entry->Flink;

      ConioConsoleCtrlEvent(CTRL_CLOSE_EVENT, current);
    }

  LeaveCriticalSection(&Console->Header.Lock);
}

static VOID FASTCALL
GuiConsoleHandleNcDestroy(HWND hWnd)
{
  PCSRSS_CONSOLE Console;
  PGUI_CONSOLE_DATA GuiData;

  GuiConsoleGetDataPointers(hWnd, &Console, &GuiData);
  KillTimer(hWnd, 1);
  Console->PrivateData = NULL;
  DeleteCriticalSection(&GuiData->Lock);
  HeapFree(Win32CsrApiHeap, 0, GuiData);
}

static VOID FASTCALL
GuiConsoleLeftMouseDown(HWND hWnd, LPARAM lParam)
{
  PCSRSS_CONSOLE Console;
  PGUI_CONSOLE_DATA GuiData;
  POINTS pt;
  RECT rc;

  GuiConsoleGetDataPointers(hWnd, &Console, &GuiData);
  if (Console == NULL || GuiData == NULL) return;

  pt = MAKEPOINTS(lParam);

  rc.left = pt.x / GuiData->CharWidth;
  rc.top = pt.y / GuiData->CharHeight;
  rc.right = rc.left + 1;
  rc.bottom = rc.top + 1;

  GuiData->SelectionStart.x = rc.left;
  GuiData->SelectionStart.y = rc.top;

  SetCapture(hWnd);

  GuiData->MouseDown = TRUE;

  GuiConsoleUpdateSelection(hWnd, &rc, GuiData);
}

static VOID FASTCALL
GuiConsoleLeftMouseUp(HWND hWnd, LPARAM lParam)
{
  PCSRSS_CONSOLE Console;
  PGUI_CONSOLE_DATA GuiData;
  RECT rc;
  POINTS pt;

  GuiConsoleGetDataPointers(hWnd, &Console, &GuiData);
  if (Console == NULL || GuiData == NULL) return;
  if (GuiData->Selection.left == -1 || !GuiData->MouseDown) return;

  pt = MAKEPOINTS(lParam);

  rc.left = GuiData->SelectionStart.x;
  rc.top = GuiData->SelectionStart.y;
  rc.right = (pt.x >= 0 ? (pt.x / GuiData->CharWidth) + 1 : 0);
  rc.bottom = (pt.y >= 0 ? (pt.y / GuiData->CharHeight) + 1 : 0);

  /* exchange left/top with right/bottom if required */
  if(rc.left >= rc.right)
  {
    LONG tmp;
    tmp = rc.left;
    rc.left = max(rc.right - 1, 0);
    rc.right = tmp + 1;
  }
  if(rc.top >= rc.bottom)
  {
    LONG tmp;
    tmp = rc.top;
    rc.top = max(rc.bottom - 1, 0);
    rc.bottom = tmp + 1;
  }

  GuiData->MouseDown = FALSE;

  GuiConsoleUpdateSelection(hWnd, &rc, GuiData);

  ReleaseCapture();
}

static VOID FASTCALL
GuiConsoleMouseMove(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
  PCSRSS_CONSOLE Console;
  PGUI_CONSOLE_DATA GuiData;
  RECT rc;
  POINTS pt;

  if (!(wParam & MK_LBUTTON)) return;

  GuiConsoleGetDataPointers(hWnd, &Console, &GuiData);
  if (Console == NULL || GuiData == NULL || !GuiData->MouseDown) return;

  pt = MAKEPOINTS(lParam);

  rc.left = GuiData->SelectionStart.x;
  rc.top = GuiData->SelectionStart.y;
  rc.right = (pt.x >= 0 ? (pt.x / GuiData->CharWidth) + 1 : 0);
  rc.bottom = (pt.y >= 0 ? (pt.y / GuiData->CharHeight) + 1 : 0);

  /* exchange left/top with right/bottom if required */
  if(rc.left >= rc.right)
  {
    LONG tmp;
    tmp = rc.left;
    rc.left = max(rc.right - 1, 0);
    rc.right = tmp + 1;
  }
  if(rc.top >= rc.bottom)
  {
    LONG tmp;
    tmp = rc.top;
    rc.top = max(rc.bottom - 1, 0);
    rc.bottom = tmp + 1;
  }

  GuiConsoleUpdateSelection(hWnd, &rc, GuiData);
}

static VOID FASTCALL
GuiConsoleRightMouseDown(HWND hWnd)
{
  PCSRSS_CONSOLE Console;
  PGUI_CONSOLE_DATA GuiData;

  GuiConsoleGetDataPointers(hWnd, &Console, &GuiData);
  if (Console == NULL || GuiData == NULL) return;

  if (GuiData->Selection.left == -1)
  {
    /* FIXME - paste text from clipboard */
  }
  else
  {
    /* FIXME - copy selection to clipboard */

    GuiConsoleUpdateSelection(hWnd, NULL, GuiData);
  }

}

static LRESULT CALLBACK
GuiConsoleWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  LRESULT Result = 0;

  switch(msg)
    {
      case WM_NCCREATE:
        Result = (LRESULT) GuiConsoleHandleNcCreate(hWnd, (CREATESTRUCTW *) lParam);
        break;
      case WM_PAINT:
        GuiConsoleHandlePaint(hWnd, (HDC)wParam);
        break;
      case WM_KEYDOWN:
      case WM_KEYUP:
      case WM_SYSKEYDOWN:
      case WM_SYSKEYUP:
      case WM_CHAR:
        GuiConsoleHandleKey(hWnd, msg, wParam, lParam);
        break;
      case WM_TIMER:
        GuiConsoleHandleTimer(hWnd);
        break;
      case WM_CLOSE:
        GuiConsoleHandleClose(hWnd);
        break;
      case WM_NCDESTROY:
        GuiConsoleHandleNcDestroy(hWnd);
        break;
      case WM_LBUTTONDOWN:
          GuiConsoleLeftMouseDown(hWnd, lParam);
        break;
      case WM_LBUTTONUP:
          GuiConsoleLeftMouseUp(hWnd, lParam);
        break;
      case WM_RBUTTONDOWN:
          GuiConsoleRightMouseDown(hWnd);
        break;
      case WM_MOUSEMOVE:
          GuiConsoleMouseMove(hWnd, wParam, lParam);
        break;
      default:
        Result = DefWindowProcW(hWnd, msg, wParam, lParam);
        break;
    }

  return Result;
}

static LRESULT CALLBACK
GuiConsoleNotifyWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  HWND NewWindow;
  LONG WindowCount;
  MSG Msg;
  PWCHAR Buffer, Title;
  PCSRSS_CONSOLE Console = (PCSRSS_CONSOLE) lParam;

  switch(msg)
    {
      case WM_CREATE:
        SetWindowLongW(hWnd, GWL_USERDATA, 0);
        return 0;
      case PM_CREATE_CONSOLE:
        Buffer = HeapAlloc(Win32CsrApiHeap, 0,
                           Console->Title.Length + sizeof(WCHAR));
        if (NULL != Buffer)
          {
            memcpy(Buffer, Console->Title.Buffer, Console->Title.Length);
            Buffer[Console->Title.Length / sizeof(WCHAR)] = L'\0';
            Title = Buffer;
          }
        else
          {
            Title = L"";
          }
        NewWindow = CreateWindowW(L"ConsoleWindowClass",
                                  Title,
                                  WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                                  CW_USEDEFAULT,
                                  CW_USEDEFAULT,
                                  CW_USEDEFAULT,
                                  CW_USEDEFAULT,
                                  NULL,
                                  NULL,
                                  (HINSTANCE) GetModuleHandleW(NULL),
                                  (PVOID) Console);
        if (NULL != Buffer)
          {
            HeapFree(Win32CsrApiHeap, 0, Buffer);
          }
        Console->hWindow = NewWindow;
        if (NULL != NewWindow)
          {
            SetWindowLongW(hWnd, GWL_USERDATA, GetWindowLongW(hWnd, GWL_USERDATA) + 1);
            ShowWindow(NewWindow, SW_SHOW);
          }
        return (LRESULT) NewWindow;
      case PM_DESTROY_CONSOLE:
        /* Window creation is done using a PostMessage(), so it's possible that the
         * window that we want to destroy doesn't exist yet. So first empty the message
         * queue */
        while(PeekMessageW(&Msg, NULL, 0, 0, PM_REMOVE))
          {
            TranslateMessage(&Msg);
            DispatchMessageW(&Msg);
          }
        DestroyWindow(Console->hWindow);
        Console->hWindow = NULL;
        WindowCount = GetWindowLongW(hWnd, GWL_USERDATA);
        WindowCount--;
        SetWindowLongW(hWnd, GWL_USERDATA, WindowCount);
        if (0 == WindowCount)
          {
            NotifyWnd = NULL;
            DestroyWindow(hWnd);
            PrivateCsrssManualGuiCheck(-1);
            PostQuitMessage(0);
          }
        return 0;
      default:
        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
}

static DWORD STDCALL
GuiConsoleGuiThread(PVOID Data)
{
  MSG msg;
  PHANDLE GraphicsStartupEvent = (PHANDLE) Data;

  NotifyWnd = CreateWindowW(L"Win32CsrCreateNotify",
                            L"",
                            WS_OVERLAPPEDWINDOW,
                            CW_USEDEFAULT,
                            CW_USEDEFAULT,
                            CW_USEDEFAULT,
                            CW_USEDEFAULT,
                            NULL,
                            NULL,
                            (HINSTANCE) GetModuleHandleW(NULL),
                            NULL);
  if (NULL == NotifyWnd)
    {
      PrivateCsrssManualGuiCheck(-1);
      SetEvent(*GraphicsStartupEvent);
      return 1;
    }

  SetEvent(*GraphicsStartupEvent);

  while(GetMessageW(&msg, NULL, 0, 0))
    {
      TranslateMessage(&msg);
      DispatchMessageW(&msg);
    }

  return 1;
}

static BOOL FASTCALL
GuiInit(VOID)
{
  WNDCLASSEXW wc;

  if (NULL == NotifyWnd)
    {
      PrivateCsrssManualGuiCheck(+1);
    }

  wc.cbSize = sizeof(WNDCLASSEXW);
  wc.lpszClassName = L"Win32CsrCreateNotify";
  wc.lpfnWndProc = GuiConsoleNotifyWndProc;
  wc.style = 0;
  wc.hInstance = (HINSTANCE) GetModuleHandleW(NULL);
  wc.hIcon = NULL;
  wc.hCursor = NULL;
  wc.hbrBackground = NULL;
  wc.lpszMenuName = NULL;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hIconSm = NULL;
  if (RegisterClassExW(&wc) == 0)
    {
      DPRINT1("Failed to register notify wndproc\n");
      return FALSE;
    }

  wc.cbSize = sizeof(WNDCLASSEXW);
  wc.lpszClassName = L"ConsoleWindowClass";
  wc.lpfnWndProc = GuiConsoleWndProc;
  wc.style = 0;
  wc.hInstance = (HINSTANCE) GetModuleHandleW(NULL);
  wc.hIcon = LoadIconW(Win32CsrDllHandle, MAKEINTRESOURCEW(1));
  wc.hCursor = LoadCursorW(NULL, MAKEINTRESOURCEW(IDC_ARROW));
  wc.hbrBackground = NULL;
  wc.lpszMenuName = NULL;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hIconSm = LoadImageW(Win32CsrDllHandle, MAKEINTRESOURCEW(1), IMAGE_ICON,
                          GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON),
                          LR_SHARED);
  if (RegisterClassExW(&wc) == 0)
    {
      DPRINT1("Failed to register console wndproc\n");
      return FALSE;
    }

  return TRUE;
}

static VOID STDCALL
GuiInitScreenBuffer(PCSRSS_CONSOLE Console, PCSRSS_SCREEN_BUFFER Buffer)
{
  Buffer->DefaultAttrib = 0x0f;
}

static BOOL STDCALL
GuiChangeTitle(PCSRSS_CONSOLE Console)
{
  PWCHAR Buffer, Title;

  Buffer = HeapAlloc(Win32CsrApiHeap, 0,
                     Console->Title.Length + sizeof(WCHAR));
  if (NULL != Buffer)
    {
      memcpy(Buffer, Console->Title.Buffer, Console->Title.Length);
      Buffer[Console->Title.Length / sizeof(WCHAR)] = L'\0';
      Title = Buffer;
    }
  else
    {
      Title = L"";
    }
  SendMessageW(Console->hWindow, WM_SETTEXT, 0, (LPARAM) Title);
  if (NULL != Buffer)
    {
      HeapFree(Win32CsrApiHeap, 0, Buffer);
    }

  return TRUE;
}

static BOOL STDCALL
GuiChangeIcon(PCSRSS_CONSOLE Console)
{
  SendMessageW(Console->hWindow, WM_SETICON, ICON_BIG, (LPARAM)Console->hWindowIcon);
  SendMessageW(Console->hWindow, WM_SETICON, ICON_SMALL, (LPARAM)Console->hWindowIcon);

  return TRUE;
}

static VOID STDCALL
GuiCleanupConsole(PCSRSS_CONSOLE Console)
{
  SendMessageW(NotifyWnd, PM_DESTROY_CONSOLE, 0, (LPARAM) Console);
}

static CSRSS_CONSOLE_VTBL GuiVtbl =
{
  GuiInitScreenBuffer,
  GuiWriteStream,
  GuiDrawRegion,
  GuiSetCursorInfo,
  GuiSetScreenInfo,
  GuiChangeTitle,
  GuiCleanupConsole,
  GuiChangeIcon
};

NTSTATUS FASTCALL
GuiInitConsole(PCSRSS_CONSOLE Console)
{
  HANDLE GraphicsStartupEvent;
  HANDLE ThreadHandle;

  if (! ConsInitialized)
    {
      ConsInitialized = TRUE;
      if (! GuiInit())
        {
          ConsInitialized = FALSE;
          return STATUS_UNSUCCESSFUL;
        }
    }

  Console->Vtbl = &GuiVtbl;
  Console->Size.X = 80;
  Console->Size.Y = 25;
  if (NULL == NotifyWnd)
    {
      GraphicsStartupEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
      if (NULL == GraphicsStartupEvent)
        {
          return STATUS_UNSUCCESSFUL;
        }

      ThreadHandle = CreateThread(NULL,
                                  0,
                                  GuiConsoleGuiThread,
                                  (PVOID) &GraphicsStartupEvent,
                                  0,
                                  NULL);
      if (NULL == ThreadHandle)
        {
          NtClose(GraphicsStartupEvent);
          DPRINT1("Win32Csr: Failed to create graphics console thread. Expect problems\n");
          return STATUS_UNSUCCESSFUL;
        }
      SetThreadPriority(ThreadHandle, THREAD_PRIORITY_HIGHEST);
      CloseHandle(ThreadHandle);

      WaitForSingleObject(GraphicsStartupEvent, INFINITE);
      CloseHandle(GraphicsStartupEvent);

      if (NULL == NotifyWnd)
        {
          DPRINT1("Win32Csr: Failed to create notification window.\n");
          return STATUS_UNSUCCESSFUL;
        }
    }

  PostMessageW(NotifyWnd, PM_CREATE_CONSOLE, 0, (LPARAM) Console);

  return STATUS_SUCCESS;
}

/* EOF */
