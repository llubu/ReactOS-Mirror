/* $Id: defwnd.c,v 1.6 2002/09/01 20:39:55 dwelch Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS user32.dll
 * FILE:            lib/user32/windows/window.c
 * PURPOSE:         Window management
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      06-06-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#include <windows.h>
#include <user32.h>
#include <window.h>
#include <debug.h>
#include <user32/wininternal.h>

/* GLOBALS *******************************************************************/

static HBITMAP hbitmapClose;
static HBITMAP hbitmapMinimize;
static HBITMAP hbitmapMinimizeD;
static HBITMAP hbitmapMaximize;
static HBITMAP hbitmapMaximizeD;
static HBITMAP hbitmapRestore;
static HBITMAP hbitmapRestoreD;

static COLORREF SysColours[] =
  {
    RGB(224, 224, 224) /* COLOR_SCROLLBAR */,
    RGB(192, 192, 192) /* COLOR_BACKGROUND */,
    RGB(0, 64, 128) /* COLOR_ACTIVECAPTION */,
    RGB(255, 255, 255) /* COLOR_INACTIVECAPTION */,
    RGB(255, 255, 255) /* COLOR_MENU */,
    RGB(255, 255, 255) /* COLOR_WINDOW */,
    RGB(0, 0, 0) /* COLOR_WINDOWFRAME */,
    RGB(0, 0, 0) /* COLOR_MENUTEXT */,
    RGB(0, 0, 0) /* COLOR_WINDOWTEXT */,
    RGB(255, 255, 255) /* COLOR_CAPTIONTEXT */,
    RGB(128, 128, 128) /* COLOR_ACTIVEBORDER */,
    RGB(255, 255, 255) /* COLOR_INACTIVEBORDER */,
    RGB(255, 255, 232) /* COLOR_APPWORKSPACE */,
    RGB(224, 224, 224) /* COLOR_HILIGHT */,
    RGB(0, 0, 0) /* COLOR_HILIGHTTEXT */,
    RGB(192, 192, 192) /* COLOR_BTNFACE */,
    RGB(128, 128, 128) /* COLOR_BTNSHADOW */,
    RGB(192, 192, 192) /* COLOR_GRAYTEXT */,
    RGB(0, 0, 0) /* COLOR_BTNTEXT */,
    RGB(0, 0, 0) /* COLOR_INACTIVECAPTIONTEXT */,
    RGB(255, 255, 255) /* COLOR_BTNHILIGHT */,
    RGB(32, 32, 32) /* COLOR_3DDKSHADOW */,
    RGB(192, 192, 192) /* COLOR_3DLIGHT */,
    RGB(0, 0, 0) /* COLOR_INFOTEXT */,
    RGB(255, 255, 192) /* COLOR_INFOBK */,
    RGB(184, 180, 184) /* COLOR_ALTERNATEBTNFACE */,
    RGB(0, 0, 255) /* COLOR_HOTLIGHT */,
    RGB(16, 132, 208) /* COLOR_GRADIENTACTIVECAPTION */,
    RGB(181, 181, 181) /* COLOR_GRADIENTINACTIVECAPTION */,
  };

/* FUNCTIONS *****************************************************************/

/* ReactOS extension */
HPEN STDCALL
GetSysColorPen(int nIndex)
{
  return(CreatePen(PS_SOLID, 1, SysColours[nIndex]));
}

HBRUSH STDCALL
GetSysColorBrush(int nIndex)
{
  return(CreateSolidBrush(SysColours[nIndex]));
}


LRESULT STDCALL
DefFrameProcA(HWND hWnd,
	      HWND hWndMDIClient,
	      UINT uMsg,
	      WPARAM wParam,
	      LPARAM lParam)
{
  return((LRESULT)0);
}

LRESULT STDCALL
DefFrameProcW(HWND hWnd,
	      HWND hWndMDIClient,
	      UINT uMsg,
	      WPARAM wParam,
	      LPARAM lParam)
{
  return((LRESULT)0);
}


BOOL 
DefWndRedrawIconTitle(HWND hWnd)
{
}

ULONG
UserHasAnyFrameStyle(ULONG Style, ULONG ExStyle)
{
  return((Style & (WS_THICKFRAME | WS_DLGFRAME | WS_BORDER)) ||
	 (ExStyle & WS_EX_DLGMODALFRAME) ||
	 (!(Style & (WS_CHILD | WS_POPUP))));
}

ULONG
UserHasDlgFrameStyle(ULONG Style, ULONG ExStyle)
{
  return((ExStyle & WS_EX_DLGMODALFRAME) ||
	 ((Style & WS_DLGFRAME) && (!(Style & WS_THICKFRAME))));
}

ULONG
UserHasThickFrameStyle(ULONG Style, ULONG ExStyle)
{
  return((Style & WS_THICKFRAME) &&
	 (!((Style & (WS_DLGFRAME | WS_BORDER)) == WS_DLGFRAME)));
}

ULONG 
UserHasThinFrameStyle(ULONG Style, ULONG ExStyle)
{
  return((Style & WS_BORDER) ||
	 (!(Style & (WS_CHILD | WS_POPUP))));
}

ULONG
UserHasBigFrameStyle(ULONG Style, ULONG ExStyle)
{
  return((Style & (WS_THICKFRAME | WS_DLGFRAME)) ||
	 (ExStyle & WS_EX_DLGMODALFRAME));
}

static void UserGetInsideRectNC( HWND hwnd, RECT *rect )
{
  RECT WindowRect;
  ULONG Style;
  ULONG ExStyle;

  Style = GetWindowLong(hwnd, GWL_STYLE);
  ExStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
  GetWindowRect(hwnd, &WindowRect);
  rect->top    = rect->left = 0;
  rect->right  = WindowRect.right - WindowRect.left;
  rect->bottom = WindowRect.bottom - WindowRect.top;
  
  if (Style & WS_ICONIC) 
    {
      return;
    }

  /* Remove frame from rectangle */
  if (UserHasThickFrameStyle(Style, ExStyle ))
      {
	InflateRect( rect, -GetSystemMetrics(SM_CXFRAME), 
		     -GetSystemMetrics(SM_CYFRAME) );
      }
    else
      {
	if (UserHasDlgFrameStyle(Style, ExStyle ))
	  {
	    InflateRect( rect, -GetSystemMetrics(SM_CXDLGFRAME), 
			 -GetSystemMetrics(SM_CYDLGFRAME));
	    /* FIXME: this isn't in NC_AdjustRect? why not? */
	    if (ExStyle & WS_EX_DLGMODALFRAME)
	      InflateRect( rect, -1, 0 );
	  }
	else
	  {
	    if (UserHasThinFrameStyle(Style, ExStyle))
	      {
		InflateRect( rect, -GetSystemMetrics(SM_CXBORDER), -GetSystemMetrics(SM_CYBORDER) );
	      }
	  }
      }
}

void UserDrawSysButton( HWND hwnd, HDC hdc, BOOL down )
{
    RECT rect;
    HDC hdcMem;
    HBITMAP hbitmap;
    ULONG Style;

    Style = GetWindowLong(hwnd, GWL_STYLE);
    UserGetInsideRectNC( hwnd, &rect );
    hdcMem = CreateCompatibleDC( hdc );
    hbitmap = SelectObject( hdcMem, hbitmapClose );
    BitBlt(hdc, rect.left, rect.top, GetSystemMetrics(SM_CXSIZE), GetSystemMetrics(SM_CYSIZE),
	   hdcMem, (Style & WS_CHILD) ? GetSystemMetrics(SM_CXSIZE) : 0, 0,
	   down ? NOTSRCCOPY : SRCCOPY );
    SelectObject( hdcMem, hbitmap );
    DeleteDC( hdcMem );
}

static void UserDrawMaxButton( HWND hwnd, HDC hdc, BOOL down )
{
    RECT rect;
    HDC hdcMem;

    UserGetInsideRectNC( hwnd, &rect );
    hdcMem = CreateCompatibleDC( hdc );
    SelectObject( hdcMem,  (IsZoomed(hwnd) 
			    ? (down ? hbitmapRestoreD : hbitmapRestore)
			    : (down ? hbitmapMaximizeD : hbitmapMaximize)) );
    BitBlt( hdc, rect.right - GetSystemMetrics(SM_CXSIZE) - 1, rect.top,
	    GetSystemMetrics(SM_CXSIZE) + 1, GetSystemMetrics(SM_CYSIZE), hdcMem, 0, 0,
	    SRCCOPY );
    DeleteDC( hdcMem );
}

static void UserDrawMinButton( HWND hwnd, HDC hdc, BOOL down)
{
  RECT rect;
  HDC hdcMem;
    
  UserGetInsideRectNC(hwnd, &rect);
  hdcMem = CreateCompatibleDC(hdc);
  SelectObject(hdcMem, (down ? hbitmapMinimizeD : hbitmapMinimize));
  if (GetWindowLong(hwnd, GWL_STYLE) & WS_MAXIMIZEBOX) 
    {
      rect.right -= GetSystemMetrics(SM_CXSIZE)+1;
    }
  BitBlt( hdc, rect.right - GetSystemMetrics(SM_CXSIZE) - 1, rect.top,
	  GetSystemMetrics(SM_CXSIZE) + 1, GetSystemMetrics(SM_CYSIZE), 
	  hdcMem, 0, 0,
	  SRCCOPY );
  DeleteDC( hdcMem );
}

static void UserDrawCaptionNC( HDC hdc, RECT *rect, HWND hwnd,
			    DWORD style, BOOL active )
{
  RECT r = *rect;
  char buffer[256];

  if (!hbitmapClose)
    {
	if (!(hbitmapClose = LoadBitmapW( 0, MAKEINTRESOURCE(OBM_CLOSE) )))
        {    
	    return;
        }
	hbitmapMinimize  = LoadBitmapW( 0, MAKEINTRESOURCE(OBM_REDUCE) );
	hbitmapMinimizeD = LoadBitmapW( 0, MAKEINTRESOURCE(OBM_REDUCED) );
	hbitmapMaximize  = LoadBitmapW( 0, MAKEINTRESOURCE(OBM_ZOOM) );
	hbitmapMaximizeD = LoadBitmapW( 0, MAKEINTRESOURCE(OBM_ZOOMD) );
	hbitmapRestore   = LoadBitmapW( 0, MAKEINTRESOURCE(OBM_RESTORE) );
	hbitmapRestoreD  = LoadBitmapW( 0, MAKEINTRESOURCE(OBM_RESTORED) );
    }
    
  if (GetWindowLong(hwnd, GWL_EXSTYLE) & WS_EX_DLGMODALFRAME)
    {
      HBRUSH hbrushOld = SelectObject(hdc, GetSysColorBrush(COLOR_WINDOW) );
      PatBlt( hdc, r.left, r.top, 1, r.bottom-r.top+1,PATCOPY );
      PatBlt( hdc, r.right-1, r.top, 1, r.bottom-r.top+1, PATCOPY );
      PatBlt( hdc, r.left, r.top-1, r.right-r.left, 1, PATCOPY );
      r.left++;
      r.right--;
      SelectObject( hdc, hbrushOld );
    }

  MoveToEx( hdc, r.left, r.bottom, NULL );
  LineTo( hdc, r.right, r.bottom );

  if (style & WS_SYSMENU)
    {
      UserDrawSysButton( hwnd, hdc, FALSE );
      r.left += GetSystemMetrics(SM_CXSIZE) + 1;
      MoveToEx( hdc, r.left - 1, r.top, NULL );
      LineTo( hdc, r.left - 1, r.bottom );
    }
  if (style & WS_MAXIMIZEBOX)
    {
      UserDrawMaxButton( hwnd, hdc, FALSE );
      r.right -= GetSystemMetrics(SM_CXSIZE) + 1;
    }
  if (style & WS_MINIMIZEBOX)
    {
      UserDrawMinButton( hwnd, hdc, FALSE );
      r.right -= GetSystemMetrics(SM_CXSIZE) + 1;
    }
  
  FillRect( hdc, &r, GetSysColorBrush(active ? COLOR_ACTIVECAPTION :
					    COLOR_INACTIVECAPTION) );
  
  if (GetWindowTextA( hwnd, buffer, sizeof(buffer) ))
    {
      if (active) SetTextColor( hdc, GetSysColor( COLOR_CAPTIONTEXT ) );
      else SetTextColor( hdc, GetSysColor( COLOR_INACTIVECAPTIONTEXT ) );
      SetBkMode( hdc, TRANSPARENT );
      DrawTextA( hdc, buffer, -1, &r,
		 DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX );
    }
}

VOID
UserDrawFrameNC(HDC hdc, RECT* rect, BOOL dlgFrame, BOOL active)
{
  INT width, height;
  
  if (dlgFrame)
    {
      width = GetSystemMetrics(SM_CXDLGFRAME) - 1;
      height = GetSystemMetrics(SM_CYDLGFRAME) - 1;
      SelectObject( hdc, GetSysColorBrush(active ? COLOR_ACTIVECAPTION :
					  COLOR_INACTIVECAPTION) );
    }
  else
    {
      width = GetSystemMetrics(SM_CXFRAME) - 2;
      height = GetSystemMetrics(SM_CYFRAME) - 2;
      SelectObject( hdc, GetSysColorBrush(active ? COLOR_ACTIVEBORDER :
					    COLOR_INACTIVEBORDER) );
    }
  
  /* Draw frame */
  PatBlt( hdc, rect->left, rect->top,
	  rect->right - rect->left, height, PATCOPY );
  PatBlt( hdc, rect->left, rect->top,
	  width, rect->bottom - rect->top, PATCOPY );
  PatBlt( hdc, rect->left, rect->bottom - 1,
	  rect->right - rect->left, -height, PATCOPY );
  PatBlt( hdc, rect->right - 1, rect->top,
	  -width, rect->bottom - rect->top, PATCOPY );

  if (dlgFrame)
    {
      InflateRect( rect, -width, -height );
    } 
  else
    {
      INT decYOff = GetSystemMetrics(SM_CXFRAME) + 
	GetSystemMetrics(SM_CXSIZE) - 1;
      INT decXOff = GetSystemMetrics(SM_CYFRAME) + 
	GetSystemMetrics(SM_CYSIZE) - 1;

      /* Draw inner rectangle */
      
      SelectObject( hdc, GetStockObject(NULL_BRUSH) );
      Rectangle( hdc, rect->left + width, rect->top + height,
		 rect->right - width , rect->bottom - height );
      
      /* Draw the decorations */
      
      MoveToEx( hdc, rect->left, rect->top + decYOff, NULL );
      LineTo( hdc, rect->left + width, rect->top + decYOff );
      MoveToEx( hdc, rect->right - 1, rect->top + decYOff, NULL );
      LineTo( hdc, rect->right - width - 1, rect->top + decYOff );
      MoveToEx( hdc, rect->left, rect->bottom - decYOff, NULL );
      LineTo( hdc, rect->left + width, rect->bottom - decYOff );
      MoveToEx( hdc, rect->right - 1, rect->bottom - decYOff, NULL );
      LineTo( hdc, rect->right - width - 1, rect->bottom - decYOff );
      
      MoveToEx( hdc, rect->left + decXOff, rect->top, NULL );
      LineTo( hdc, rect->left + decXOff, rect->top + height);
      MoveToEx( hdc, rect->left + decXOff, rect->bottom - 1, NULL );
      LineTo( hdc, rect->left + decXOff, rect->bottom - height - 1 );
      MoveToEx( hdc, rect->right - decXOff, rect->top, NULL );
      LineTo( hdc, rect->right - decXOff, rect->top + height );
      MoveToEx( hdc, rect->right - decXOff, rect->bottom - 1, NULL );
      LineTo( hdc, rect->right - decXOff, rect->bottom - height - 1 );
      
      InflateRect( rect, -width - 1, -height - 1 );
    }
}

VOID
DefWndDoPaintNC(HWND hWnd, HRGN clip)
{
  ULONG Active;
  HDC hDc;
  RECT rect;
  ULONG Style;
  ULONG ExStyle;

  Active = GetWindowLongW(hWnd, GWL_STYLE) & WIN_NCACTIVATED;
  Style = GetWindowLong(hWnd, GWL_STYLE);
  ExStyle = GetWindowLong(hWnd, GWL_EXSTYLE);

  hDc = GetDCEx(hWnd, (clip > (HRGN)1) ? clip : 0, DCX_USESTYLE | DCX_WINDOW |
		((clip > (HRGN)1) ? (DCX_INTERSECTRGN | DCX_KEEPCLIPRGN) : 0));
  if (hDc == 0)
    {
      return;
    }

  /* FIXME: Test whether we need to draw anything at all. */

  GetWindowRect(hWnd, &rect);
  rect.right = rect.right - rect.left;
  rect.bottom = rect.bottom - rect.top;
  rect.top = rect.left = 0;

  SelectObject(hDc, GetSysColorPen(COLOR_WINDOWFRAME));
  if (UserHasAnyFrameStyle(Style, ExStyle))
    {
      SelectObject(hDc, GetStockObject(NULL_BRUSH));
      Rectangle(hDc, 0, 0, rect.right, rect.bottom);
      InflateRect(&rect, -1, -1);
    }
  
  if (UserHasThickFrameStyle(Style, ExStyle))
    {
      UserDrawFrameNC(hDc, &rect, FALSE, Active);
    }
  else if (UserHasDlgFrameStyle(Style, ExStyle))
    {
      UserDrawFrameNC(hDc, &rect, TRUE, Active);
    }
  
  if (Style & WS_CAPTION)
    {
      RECT r = rect;
      r.bottom = rect.top + GetSystemMetrics(SM_CYSIZE);
      rect.top += GetSystemMetrics(SM_CYSIZE) + 
	GetSystemMetrics(SM_CYBORDER);
      UserDrawCaptionNC(hDc, &r, hWnd, Style, Active);
    }

  /* FIXME: Draw menu bar. */

  /* FIXME: Draw scroll bars. */

  /* FIXME: Draw size box. */
  
  ReleaseDC(hWnd, hDc);
}

LRESULT
DefWndPaintNC(HWND hWnd, HRGN clip)
{
  if (IsWindowVisible(hWnd))
    {
      if (IsIconic(hWnd))
	{
	  DefWndRedrawIconTitle(hWnd);
	}
      else
	{
	  DefWndDoPaintNC(hWnd, clip);
	}
    }
  return(0);
}

LRESULT
DefWndHitTestNC(HWND hWnd, POINT Point)
{
}

LRESULT
DefWndHandleLButtonDownNC(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
}

LRESULT
DefWndHandleLButtonDblClkNC(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
}

LRESULT
DefWndHandleActiveNC(HWND hWnd, WPARAM wParam)
{
}

VOID
DefWndSetRedraw(HWND hWnd, WPARAM wParam)
{
}

LRESULT 
DefWndHandleSetCursor(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
  /* Not for child windows. */
  if (hWnd != (HWND)wParam)
    {
      return(0);
    }

  switch(LOWORD(lParam))
    {
    case HTERROR:
      {
	WORD Msg = HIWORD(lParam);
	if (Msg == WM_LBUTTONDOWN || Msg == WM_MBUTTONDOWN ||
	    Msg == WM_RBUTTONDOWN)
	  {
	    MessageBeep(0);
	  }
	break;
      }

    case HTCLIENT:
      {
	HICON hCursor = (HICON)GetClassLong(hWnd, GCL_HCURSOR);
	if (hCursor)
	  {
	    SetCursor(hCursor);
	    return(TRUE);
	  }
	return(FALSE);
      }

    case HTLEFT:
    case HTRIGHT:
      {
	return((LRESULT)SetCursor(LoadCursorW(0, IDC_SIZEWE)));
      }

    case HTTOP:
    case HTBOTTOM:
      {
	return((LRESULT)SetCursor(LoadCursorW(0, IDC_SIZENS)));
      }

    case HTTOPLEFT:
    case HTBOTTOMRIGHT:
      {
	return((LRESULT)SetCursor(LoadCursorW(0, IDC_SIZENWSE)));
      }

    case HTBOTTOMLEFT:
    case HTTOPRIGHT:
      {
	return((LRESULT)SetCursor(LoadCursorW(0, IDC_SIZENESW)));
      }
    }
  return((LRESULT)SetCursor(LoadCursorW(0, IDC_ARROW)));
}

LRESULT
DefWndHandleSysCommand(HWND hWnd, WPARAM wParam, POINT Pt)
{
  /* FIXME: Implement system commands. */
  return(0);
}

VOID
DefWndAdjustRect(RECT* Rect, ULONG Style, BOOL Menu, ULONG ExStyle)
{
  if (Style & WS_ICONIC)
    {
      return;
    }

  if (UserHasThickFrameStyle(Style, ExStyle))
    {
      InflateRect(Rect, GetSystemMetrics(SM_CXFRAME), 
		  GetSystemMetrics(SM_CYFRAME));
    }
  else if (UserHasDlgFrameStyle(Style, ExStyle))
    {
      InflateRect(Rect, GetSystemMetrics(SM_CXDLGFRAME),
		  GetSystemMetrics(SM_CYDLGFRAME));
    }
  else if (UserHasThinFrameStyle(Style, ExStyle))
    {
      InflateRect(Rect, GetSystemMetrics(SM_CXBORDER),
		  GetSystemMetrics(SM_CYBORDER));
    }
  if (Style & WS_CAPTION)
    {
      Rect->top -= GetSystemMetrics(SM_CYCAPTION) - 
	GetSystemMetrics(SM_CYBORDER);
    }
  if (Menu)
    {
      Rect->top -= GetSystemMetrics(SM_CYMENU) + GetSystemMetrics(SM_CYBORDER);
    }
  if (Style & WS_VSCROLL)
    {
      Rect->right += GetSystemMetrics(SM_CXVSCROLL) - 1;
      if (UserHasAnyFrameStyle(Style, ExStyle))
	{
	  Rect->right++;
	}
    }
  if (Style & WS_HSCROLL)
    {
      Rect->bottom += GetSystemMetrics(SM_CYHSCROLL) - 1;
      if (UserHasAnyFrameStyle(Style, ExStyle))
	{
	  Rect->bottom++;
	}
    }
}

LRESULT STDCALL
DefWndNCCalcSize(HWND hWnd, RECT* Rect)
{
  LRESULT Result;
  LONG Style = GetClassLongW(hWnd, GCL_STYLE);
  RECT TmpRect = {0, 0, 0, 0};

  if (Style & CS_VREDRAW)
    {
      Result |= WVR_VREDRAW;
    }
  if (Style & CS_HREDRAW)
    {
      Result |= WVR_HREDRAW;
    }

  if (!(GetWindowLong(hWnd, GWL_STYLE) & WS_MINIMIZE))
    {
      DefWndAdjustRect(&TmpRect, GetWindowLong(hWnd, GWL_STYLE),
		       FALSE, GetWindowLong(hWnd, GWL_EXSTYLE));
      Rect->left -= TmpRect.left;
      Rect->top -= TmpRect.top;
      Rect->right -= TmpRect.right;
      Rect->bottom -= TmpRect.bottom;
      /* FIXME: Adjust if the window has a menu. */
      Rect->bottom = max(Rect->top, Rect->bottom);
      Rect->right = max(Rect->left, Rect->right);
    }
  return(Result);
}

LRESULT
DefWndHandleWindowPosChanging(HWND hWnd, WINDOWPOS* Pos)
{
  /* FIXME: Implement this. */
}

LRESULT STDCALL
User32DefWindowProc(HWND hWnd,
		    UINT Msg,
		    WPARAM wParam,
		    LPARAM lParam,
		    BOOL bUnicode)
{
  switch (Msg)
    {
    case WM_NCPAINT:
      {
	return(DefWndPaintNC(hWnd, (HRGN)wParam));
      }

    case WM_NCHITTEST:
      {
	POINT Point;
	Point.x = SLOWORD(lParam);
	Point.y = SHIWORD(lParam);
	return(DefWndHitTestNC(hWnd, Point));
      }

    case WM_NCLBUTTONDOWN:
      {
	return(DefWndHandleLButtonDownNC(hWnd, wParam, lParam));
      }

    case WM_LBUTTONDBLCLK:
    case WM_NCLBUTTONDBLCLK:
      {
	return(DefWndHandleLButtonDblClkNC(hWnd, wParam, lParam));
      }

    case WM_NCRBUTTONDOWN:
      {
	if (wParam == HTCAPTION)
	  {
	    SetCapture(hWnd);
	  }
	break;
      }

    case WM_RBUTTONUP:
      {
	POINT Pt;
	if (hWnd == GetCapture())
	  {
	    ReleaseCapture();
	  }
	Pt.x = SLOWORD(lParam);
	Pt.y = SHIWORD(lParam);
	ClientToScreen(hWnd, &Pt);
	lParam = MAKELPARAM(Pt.x, Pt.y);
	if (bUnicode)
	  {
	    SendMessageW(hWnd, WM_CONTEXTMENU, (WPARAM)hWnd, lParam);
	  }
	else
	  {
	    SendMessageA (hWnd, WM_CONTEXTMENU, (WPARAM)hWnd, lParam);
	  }
	break;
      }

    case WM_NCRBUTTONUP:
      {
	/* Wine does nothing here. */
	break;
      }

    case WM_CONTEXTMENU:
      {
	if (GetWindowLong(hWnd, GWL_STYLE) & WS_CHILD)
	  {
	    if (bUnicode)
	      {
		SendMessageW(GetParent(hWnd), Msg, wParam, lParam);
	      }
	    else
	      {
		SendMessageA(hWnd, WM_CONTEXTMENU, wParam, lParam);
	      }
	  }
	else
	  {
	    LONG HitCode;
	    POINT Pt;

	    Pt.x = SLOWORD(lParam);
	    Pt.y = SHIWORD(lParam);
	    
	    if (GetWindowLong(hWnd, GWL_STYLE) & WS_CHILD)
	      {
		ScreenToClient(GetParent(hWnd), &Pt);
	      }

	    HitCode = DefWndHitTestNC(hWnd, Pt);

	    if (HitCode == HTCAPTION || HitCode == HTSYSMENU)
	      {
		TrackPopupMenu(GetSystemMenu(hWnd, FALSE),
			       TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
			       Pt.x, Pt.y, 0, hWnd, NULL);
	      }
	  }
	break;
      }

    case WM_NCACTIVATE:
      {
	return(DefWndHandleActiveNC(hWnd, wParam));
      }

    case WM_NCDESTROY:
      {
	return(0);
      }

    case WM_PRINT:
      {
	return(0);
      }

    case WM_PAINTICON:
    case WM_PAINT:
      {
	PAINTSTRUCT Ps;
	HDC hDc = BeginPaint(hWnd, &Ps);
	if (hDc)
	  {
	    HICON hIcon;
	    if (GetWindowLongW(hWnd, GWL_STYLE) & WS_MINIMIZE &&
		(hIcon = (HICON)GetClassLongW(hWnd, GCL_HICON)) != NULL)
	      {
		RECT WindowRect;
		INT x, y;
		GetWindowRect(hWnd, &WindowRect);
		x = (WindowRect.right - WindowRect.left - 
		     GetSystemMetrics(SM_CXICON)) / 2;
		y = (WindowRect.bottom - WindowRect.top - 
		     GetSystemMetrics(SM_CYICON)) / 2;
		DrawIcon(hDc, x, y, hIcon);
	      }
	    EndPaint(hWnd, &Ps);
	  }
	return(0);
      }

    case WM_SYNCPAINT:
      {
	HRGN hRgn;
	hRgn = CreateRectRgn(0, 0, 0, 0);
	if (GetUpdateRgn(hWnd, hRgn, FALSE) != NULLREGION)
	  {
	    RedrawWindow(hWnd, NULL, hRgn, 
			 RDW_ERASENOW | RDW_ERASE | RDW_FRAME |
			 RDW_ALLCHILDREN);
	  }
	DeleteObject(hRgn);
	return(0);
      }

    case WM_SETREDRAW:
      {
	DefWndSetRedraw(hWnd, wParam);
	return(0);
      }

    case WM_CLOSE:
      {
	DestroyWindow(hWnd);
	return(0);
      }

    case WM_MOUSEACTIVATE:
      {
	if (GetWindowLong(hWnd, GWL_STYLE) & WS_CHILD)
	  {
	    LONG Ret;
	    if (bUnicode)
	      {
		Ret = SendMessageW(GetParent(hWnd), WM_MOUSEACTIVATE,
				   wParam, lParam);
	      }
	    else
	      {
		Ret = SendMessageA(GetParent(hWnd), WM_MOUSEACTIVATE,
				   wParam, lParam);
	      }
	    if (Ret)
	      {
		return(Ret);
	      }
	  }
	return((LOWORD(lParam) >= HTCLIENT) ? MA_ACTIVATE : MA_NOACTIVATE);
      }

    case WM_ACTIVATE:
      {
	if (LOWORD(lParam) != WA_INACTIVE &&
	    GetWindowLong(hWnd, GWL_STYLE) & WS_MINIMIZE)
	  {
	    /* Check if the window is minimized. */
	    SetFocus(hWnd);
	  }
	break;
      }

    case WM_MOUSEWHEEL:
      {
	if (GetWindowLong(hWnd, GWL_STYLE & WS_CHILD))
	  {
	    if (bUnicode)
	      {
		return(SendMessageW(GetParent(hWnd), WM_MOUSEWHEEL,
				    wParam, lParam));
	      }
	    else
	      {
		return(SendMessageA(GetParent(hWnd), WM_MOUSEWHEEL,
				    wParam, lParam));
	      }
	  }
	break;
      }

    case WM_ERASEBKGND:
    case WM_ICONERASEBKGND:
      {
	RECT Rect;
	HBRUSH hBrush = (HBRUSH)GetClassLongW(hWnd, GCL_HBRBACKGROUND);
	GetClipBox((HDC)wParam, &Rect);
	FillRect((HDC)wParam, &Rect, hBrush);
	return(1);
      }

    case WM_GETDLGCODE:
      {
	return(0);
      }

      /* FIXME: Implement colour controls. */

    case WM_SETCURSOR:
      {
	if (GetWindowLong(hWnd, GWL_STYLE) & WS_CHILD)
	  {
	    if (LOWORD(lParam) < HTLEFT || LOWORD(lParam) > HTBOTTOMRIGHT)
	      {
		BOOL bResult;
		if (bUnicode)
		  {
		    bResult = SendMessageW(GetParent(hWnd), WM_SETCURSOR, 
					   wParam, lParam);
		  }
		else		  
		  {
		    bResult = SendMessageA(GetParent(hWnd), WM_SETCURSOR, 
					   wParam, lParam);
		  }
		if (bResult)
		  {
		    return(TRUE);
		  }
	      }
	  }
	return(DefWndHandleSetCursor(hWnd, wParam, lParam));      
      }

    case WM_SYSCOMMAND:
      {
	POINT Pt;
	Pt.x = SLOWORD(lParam);
	Pt.y = SHIWORD(lParam);
	return(DefWndHandleSysCommand(hWnd, wParam, Pt));
      }

      /* FIXME: Handle key messages. */

    case WM_SHOWWINDOW:
      {
	if (lParam)
	  {
	    return(0);
	  }
	/* FIXME: Check for a popup window. */
	if ((GetWindowLongW(hWnd, GWL_STYLE) & WS_VISIBLE && !wParam) ||
	    (!(GetWindowLongW(hWnd, GWL_STYLE) & WS_VISIBLE) && wParam))
	  {
	    return(0);
	  } 
	ShowWindow(hWnd, wParam ? SW_SHOWNOACTIVATE : SW_HIDE);
	break;
      }

    case WM_CANCELMODE:
      {
	/* FIXME: Check for a desktop. */
	if (GetCapture() == hWnd)
	  {
	    ReleaseCapture();
	  }
	break;
      }

    case WM_VKEYTOITEM:
    case WM_CHARTOITEM:
      return(-1);

    case WM_DROPOBJECT:
      /* FIXME: Implement this. */
      break;

    case WM_QUERYDROPOBJECT:
      {
	if (GetWindowLongW(hWnd, GWL_EXSTYLE) & WS_EX_ACCEPTFILES)
	  {
	    return(1);
	  }
	break;
      }

    case WM_QUERYDRAGICON:
      {
	UINT Len;
	HICON hIcon;

	hIcon = (HICON)GetClassLongW(hWnd, GCL_HICON);
	if (hIcon)
	  {
	    return((LRESULT)hIcon);
	  }
	for (Len = 1; Len < 64; Len++)
	  {
	    if ((hIcon = LoadIconW(NULL, MAKEINTRESOURCE(Len))) != NULL)
	      {
		return((LRESULT)hIcon);
	      }
	  }
	return((LRESULT)LoadIconW(0, IDI_APPLICATION));
      }

      /* FIXME: WM_ISACTIVEICON */

    case WM_NOTIFYFORMAT:
      {
	if (IsWindowUnicode(hWnd))
	  {
	    return(NFR_UNICODE);
	  }
	else
	  {
	    return(NFR_ANSI);
	  }
      }

    case WM_SETICON:
      {
	INT Index = (wParam != 0) ? GCL_HICON : GCL_HICONSM;
	HICON hOldIcon = (HICON)GetClassLongW(hWnd, Index);
	SetClassLongW(hWnd, Index, lParam);
	SetWindowPos(hWnd, 0, 0, 0, 0, 0, 
		     SWP_FRAMECHANGED | SWP_NOSIZE | SWP_NOMOVE |
		     SWP_NOACTIVATE | SWP_NOZORDER);
	return((LRESULT)hOldIcon);
      }

    case WM_GETICON:
      {
	INT Index = (wParam != 0) ? GCL_HICON : GCL_HICONSM;
	return(GetClassLongW(hWnd, Index));
      }

    case WM_HELP:
      {
	if (bUnicode)
	  {
	    SendMessageW(GetParent(hWnd), Msg, wParam, lParam);
	  }
	else
	  {
	    SendMessageA(GetParent(hWnd), Msg, wParam, lParam);
	  }
	break;
      }
    }
  return 0;
}

LRESULT STDCALL
DefWindowProcA(HWND hWnd,
	       UINT Msg,
	       WPARAM wParam,
	       LPARAM lParam)
{
  LRESULT Result;
  static ATOM WindowTextAtom = 0;
  PSTR WindowText;

  switch (Msg)
    {
    case WM_NCCREATE:
      {
	CREATESTRUCTA* Cs = (CREATESTRUCTA*)lParam;
	if (HIWORD(Cs->lpszName))
	  {
	    WindowTextAtom = GlobalAddAtomA("USER32!WindowTextAtomA");
	    WindowText = User32AllocHeap(strlen(Cs->lpszName) * sizeof(CHAR));
	    strcpy(WindowText, Cs->lpszName);
	    SetPropA(hWnd, WindowTextAtom, WindowText);
	  }
	return(1);
      }

    case WM_NCCALCSIZE:
      {
	return(DefWndNCCalcSize(hWnd, (RECT*)lParam));
      }

    case WM_WINDOWPOSCHANGING:
      {
	return(DefWndHandleWindowPosChanging(hWnd, (WINDOWPOS*)lParam));
      }

    case WM_GETTEXTLENGTH:
      {
	if (WindowTextAtom == 0 ||
	    (WindowText = GetPropA(hWnd, WindowTextAtom)) == NULL)
	  {
	    return(0);
	  }
	return(wcslen(WindowText));
      }

    case WM_GETTEXT:
      {
	if (WindowTextAtom == 0 ||
	    (WindowText = GetPropA(hWnd, WindowTextAtom)) == NULL)
	  {
	    if (wParam > 1)
	      {
		((PSTR)lParam) = '\0';
	      }
	    return(0);
	  }
	strncpy((PWSTR)lParam, WindowText, wParam);
	return(min(wParam, wcslen(WindowText)));
      }

    case WM_SETTEXT:
      {
	if (WindowTextAtom != 0)
	  {
	    WindowTextAtom = GlobalAddAtomA("USER32!WindowTextAtomW");	    
	  }
	if (WindowTextAtom != 0 &&
	    (WindowText = GetPropA(hWnd, WindowTextAtom)) == NULL)
	  {
	    User32FreeHeap(WindowText);
	  }
	WindowText = User32AllocHeap(strlen((PSTR)lParam) * sizeof(CHAR));
	strcpy(WindowText, (PSTR)lParam);
	SetPropA(hWnd, WindowTextAtom, WindowText);
      }

    case WM_NCDESTROY:
      {
	if (WindowTextAtom != 0 &&
	    (WindowText = RemovePropA(hWnd, WindowTextAtom)) == NULL)
	  {
	    User32FreeHeap(WindowText);
	  }
	if (WindowTextAtom != 0)
	  {
	    GlobalDeleteAtom(WindowTextAtom);
	  }
	/* FIXME: Destroy scroll bars here as well. */
	return(0);
      }

    default:
      Result = User32DefWindowProc(hWnd, Msg, wParam, lParam, FALSE);
      break;
    }

  return(Result);
}

LRESULT STDCALL
DefWindowProcW(HWND hWnd,
	       UINT Msg,
	       WPARAM wParam,
	       LPARAM lParam)
{
  LRESULT Result;
  static ATOM WindowTextAtom = 0;
  PWSTR WindowText;

  switch (Msg)
    {
    case WM_NCCREATE:
      {
	CREATESTRUCTW* Cs = (CREATESTRUCTW*)lParam;
	if (HIWORD(Cs->lpszName))
	  {
	    WindowTextAtom = GlobalAddAtomW(L"USER32!WindowTextAtomW");
	    WindowText = User32AllocHeap(wcslen(Cs->lpszName) * sizeof(WCHAR));
	    wcscpy(WindowText, Cs->lpszName);
	    SetPropW(hWnd, WindowTextAtom, WindowText);
	  }
	return(1);
      }

    case WM_NCCALCSIZE:
      {
	return(DefWndNCCalcSize(hWnd, (RECT*)lParam));
      }

    case WM_WINDOWPOSCHANGING:
      {
	return(DefWndHandleWindowPosChanging(hWnd, (WINDOWPOS*)lParam));
      }

    case WM_GETTEXTLENGTH:
      {
	if (WindowTextAtom == 0 ||
	    (WindowText = GetPropW(hWnd, WindowTextAtom)) == NULL)
	  {
	    return(0);
	  }
	return(wcslen(WindowText));
      }

    case WM_GETTEXT:
      {
	if (WindowTextAtom == 0 ||
	    (WindowText = GetPropW(hWnd, WindowTextAtom)) == NULL)
	  {
	    if (wParam > 1)
	      {
		((PWSTR)lParam) = '\0';
	      }
	    return(0);
	  }
	wcsncpy((PWSTR)lParam, WindowText, wParam);
	return(min(wParam, wcslen(WindowText)));
      }

    case WM_SETTEXT:
      {
	if (WindowTextAtom != 0)
	  {
	    WindowTextAtom = GlobalAddAtom(L"USER32!WindowTextAtomW");	    
	  }
	if (WindowTextAtom != 0 &&
	    (WindowText = GetPropW(hWnd, WindowTextAtom)) == NULL)
	  {
	    User32FreeHeap(WindowText);
	  }
	WindowText = User32AllocHeap(wcslen((PWSTR)lParam) * sizeof(WCHAR));
	wcscpy(WindowText, (PWSTR)lParam);
	SetPropW(hWnd, WindowTextAtom, WindowText);
      }

    case WM_NCDESTROY:
      {
	if (WindowTextAtom != 0 &&
	    (WindowText = RemovePropW(hWnd, WindowTextAtom)) == NULL)
	  {
	    User32FreeHeap(WindowText);
	  }
	if (WindowTextAtom != 0)
	  {
	    GlobalDeleteAtom(WindowTextAtom);
	  }
	/* FIXME: Destroy scroll bars here as well. */
	return(0);
      }

    default:
      Result = User32DefWindowProc(hWnd, Msg, wParam, lParam, TRUE);
      break;
    }

  return(Result);
}
