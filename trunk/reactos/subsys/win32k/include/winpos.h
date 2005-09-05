#ifndef _WIN32K_WINPOS_H
#define _WIN32K_WINPOS_H

/* Undocumented flags. */
#define SWP_NOCLIENTMOVE          0x0800
#define SWP_NOCLIENTSIZE          0x1000

#define IntPtInWindow(WndObject,x,y) \
  ((x) >= (WndObject)->WindowRect.left && \
   (x) < (WndObject)->WindowRect.right && \
   (y) >= (WndObject)->WindowRect.top && \
   (y) < (WndObject)->WindowRect.bottom && \
   (!(WndObject)->WindowRegion || ((WndObject)->Style & WS_MINIMIZE) || \
    NtGdiPtInRegion((WndObject)->WindowRegion, (INT)((x) - (WndObject)->WindowRect.left), \
                    (INT)((y) - (WndObject)->WindowRect.top))))

UINT
FASTCALL co_WinPosArrangeIconicWindows(PWINDOW_OBJECT parent);
BOOL FASTCALL
IntGetClientOrigin(HWND hWnd, LPPOINT Point);
LRESULT FASTCALL
co_WinPosGetNonClientSize(HWND Wnd, RECT* WindowRect, RECT* ClientRect);
UINT FASTCALL
co_WinPosGetMinMaxInfo(PWINDOW_OBJECT Window, POINT* MaxSize, POINT* MaxPos,
		    POINT* MinTrack, POINT* MaxTrack);
UINT FASTCALL
co_WinPosMinMaximize(PWINDOW_OBJECT WindowObject, UINT ShowFlag, RECT* NewPos);
BOOLEAN FASTCALL
co_WinPosSetWindowPos(HWND Wnd, HWND WndInsertAfter, INT x, INT y, INT cx,
		   INT cy, UINT flags);
BOOLEAN FASTCALL
co_WinPosShowWindow(HWND Wnd, INT Cmd);
USHORT FASTCALL
co_WinPosWindowFromPoint(PWINDOW_OBJECT ScopeWin, PUSER_MESSAGE_QUEUE OnlyHitTests, POINT *WinPoint,
		      PWINDOW_OBJECT* Window);
VOID FASTCALL co_WinPosActivateOtherWindow(PWINDOW_OBJECT Window);

PINTERNALPOS FASTCALL WinPosInitInternalPos(PWINDOW_OBJECT WindowObject,
                                            POINT *pt, PRECT RestoreRect);

#endif /* _WIN32K_WINPOS_H */
