#pragma once

#define IntPtInWindow(WndObject,x,y) \
  ((x) >= (WndObject)->Wnd->rcWindow.left && \
   (x) < (WndObject)->Wnd->rcWindow.right && \
   (y) >= (WndObject)->Wnd->rcWindow.top && \
   (y) < (WndObject)->Wnd->rcWindow.bottom && \
   (!(WndObject)->hrgnClip || ((WndObject)->Wnd->style & WS_MINIMIZE) || \
    NtGdiPtInRegion((WndObject)->hrgnClip, (INT)((x) - (WndObject)->Wnd->rcWindow.left), \
                    (INT)((y) - (WndObject)->Wnd->rcWindow.top))))

UINT
FASTCALL co_WinPosArrangeIconicWindows(PWINDOW_OBJECT parent);
BOOL FASTCALL
IntGetClientOrigin(PWINDOW_OBJECT Window, LPPOINT Point);
LRESULT FASTCALL
co_WinPosGetNonClientSize(PWINDOW_OBJECT Window, RECTL* WindowRect, RECTL* ClientRect);
UINT FASTCALL
co_WinPosGetMinMaxInfo(PWINDOW_OBJECT Window, POINT* MaxSize, POINT* MaxPos,
		    POINT* MinTrack, POINT* MaxTrack);
UINT FASTCALL
co_WinPosMinMaximize(PWINDOW_OBJECT WindowObject, UINT ShowFlag, RECTL* NewPos);
BOOLEAN FASTCALL
co_WinPosSetWindowPos(PWINDOW_OBJECT Wnd, HWND WndInsertAfter, INT x, INT y, INT cx,
		   INT cy, UINT flags);
BOOLEAN FASTCALL
co_WinPosShowWindow(PWINDOW_OBJECT Window, INT Cmd);
USHORT FASTCALL
co_WinPosWindowFromPoint(PWINDOW_OBJECT ScopeWin, PUSER_MESSAGE_QUEUE OnlyHitTests, POINT *WinPoint,
		      PWINDOW_OBJECT* Window);
VOID FASTCALL co_WinPosActivateOtherWindow(PWINDOW_OBJECT Window);

VOID FASTCALL WinPosInitInternalPos(PWINDOW_OBJECT WindowObject,
                                    POINT *pt, RECTL *RestoreRect);
