/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  $Id$
 *
 *  COPYRIGHT:        See COPYING in the top level directory
 *  PROJECT:          ReactOS kernel
 *  PURPOSE:          Window painting function
 *  FILE:             subsys/win32k/ntuser/painting.c
 *  PROGRAMER:        Filip Navara (xnavara@volny.cz)
 *  REVISION HISTORY:
 *       06/06/2001   Created (?)
 *       18/11/2003   Complete rewrite
 */

/* INCLUDES ******************************************************************/

#include <w32k.h>

#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS **********************************************************/

/**
 * @name IntIntersectWithParents
 *
 * Intersect window rectangle with all parent client rectangles.
 *
 * @param Child
 *        Pointer to child window to start intersecting from.
 * @param WindowRect
 *        Pointer to rectangle that we want to intersect in screen
 *        coordinates on input and intersected rectangle on output (if TRUE
 *        is returned).
 *
 * @return
 *    If any parent is minimized or invisible or the resulting rectangle
 *    is empty then FALSE is returned. Otherwise TRUE is returned.
 */

BOOL FASTCALL
IntIntersectWithParents(PWINDOW_OBJECT Child, PRECT WindowRect)
{
   PWINDOW_OBJECT ParentWindow;

   ParentWindow = Child->Parent;
   while (ParentWindow != NULL)
   {
      if (!(ParentWindow->Style & WS_VISIBLE) ||
          (ParentWindow->Style & WS_MINIMIZE))
      {
         return FALSE;
      }

      if (!IntGdiIntersectRect(WindowRect, WindowRect, &ParentWindow->ClientRect))
      {
         return FALSE;
      }

      /* FIXME: Layered windows. */

      ParentWindow = ParentWindow->Parent;
   }

   return TRUE;
}

BOOL FASTCALL
IntValidateParent(PWINDOW_OBJECT Child, BOOL Recurse)
{
   PWINDOW_OBJECT ParentWindow = Child->Parent;

   while (ParentWindow && ParentWindow->Style & WS_CHILD)
      ParentWindow = ParentWindow->Parent;

   while (ParentWindow)
   {
      if (ParentWindow->Style & WS_CLIPCHILDREN)
         break;      

      if (ParentWindow->UpdateRegion != 0)
      {
         if (Recurse)
            return FALSE;

         IntInvalidateWindows(ParentWindow, Child->UpdateRegion,
                              RDW_VALIDATE | RDW_NOCHILDREN);
      }

      ParentWindow = ParentWindow->Parent;
   }

   return TRUE;
}

/**
 * @name IntCalcWindowRgn
 *
 * Get a window or client region.
 */

HRGN FASTCALL
IntCalcWindowRgn(PWINDOW_OBJECT Window, BOOL Client)
{
   HRGN hRgnWindow;
   UINT RgnType;

   if (Client)
      hRgnWindow = UnsafeIntCreateRectRgnIndirect(&Window->ClientRect);
   else
      hRgnWindow = UnsafeIntCreateRectRgnIndirect(&Window->WindowRect);

   if (Window->WindowRegion != NULL && !(Window->Style & WS_MINIMIZE))
   {
      NtGdiOffsetRgn(hRgnWindow,
         -Window->WindowRect.left,
         -Window->WindowRect.top);
      RgnType = NtGdiCombineRgn(hRgnWindow, hRgnWindow, Window->WindowRegion, RGN_AND);
      NtGdiOffsetRgn(hRgnWindow,
         Window->WindowRect.left,
         Window->WindowRect.top);
   }

   return hRgnWindow;
}

/**
 * @name IntGetNCUpdateRgn
 *
 * Get non-client update region of a window and optionally validate it.
 *
 * @param Window
 *        Pointer to window to get the NC update region from.
 * @param Validate
 *        Set to TRUE to force validating the NC update region.
 *
 * @return
 *    Handle to NC update region. The caller is responsible for deleting
 *    it.
 */

HRGN FASTCALL
IntGetNCUpdateRgn(PWINDOW_OBJECT Window, BOOL Validate)
{
   HRGN hRgnNonClient;
   HRGN hRgnWindow;
   UINT RgnType;
   
   if (Window->UpdateRegion != NULL &&
       Window->UpdateRegion != (HRGN)1)
   {
      hRgnNonClient = NtGdiCreateRectRgn(0, 0, 0, 0);

      /*
       * If region creation fails it's safe to fallback to whole
       * window region.
       */
      if (hRgnNonClient == NULL)
      {
         return (HRGN)1;
      }

      hRgnWindow = IntCalcWindowRgn(Window, TRUE);
      if (hRgnWindow == NULL)
      {
         NtGdiDeleteObject(hRgnNonClient);
         return (HRGN)1;
      }

      RgnType = NtGdiCombineRgn(hRgnNonClient, Window->UpdateRegion,
                                hRgnWindow, RGN_DIFF);
      if (RgnType == ERROR)
      {
         NtGdiDeleteObject(hRgnWindow);
         NtGdiDeleteObject(hRgnNonClient);
         return (HRGN)1;
      }
      else if (RgnType == NULLREGION)
      {
         NtGdiDeleteObject(hRgnWindow);
         NtGdiDeleteObject(hRgnNonClient);
         return NULL;
      }

      /*
       * Remove the nonclient region from the standard update region if
       * we were asked for it.
       */

      if (Validate)
      {
         if (NtGdiCombineRgn(Window->UpdateRegion, Window->UpdateRegion,
                             hRgnWindow, RGN_AND) == NULLREGION)
         {
            GDIOBJ_SetOwnership(GdiHandleTable, Window->UpdateRegion, PsGetCurrentProcess());
            NtGdiDeleteObject(Window->UpdateRegion);
            Window->UpdateRegion = NULL;
            if (!(Window->Flags & WINDOWOBJECT_NEED_INTERNALPAINT))
               MsqDecPaintCountQueue(Window->MessageQueue);
         }
      }

      NtGdiDeleteObject(hRgnWindow);

      return hRgnNonClient;
   }
   else
   {
      return Window->UpdateRegion;
   }
}

/*
 * IntPaintWindows
 *
 * Internal function used by IntRedrawWindow.
 */

static VOID FASTCALL
co_IntPaintWindows(PWINDOW_OBJECT Window, ULONG Flags, BOOL Recurse)
{
   HDC hDC;
   HWND hWnd = Window->hSelf;
   HRGN TempRegion;

   if (Flags & (RDW_ERASENOW | RDW_UPDATENOW))
   {
      if (Window->UpdateRegion)
      {
         if (!IntValidateParent(Window, Recurse))
            return;
      }

      if (Flags & RDW_UPDATENOW)
      {
         if (Window->UpdateRegion != NULL ||
             Window->Flags & WINDOWOBJECT_NEED_INTERNALPAINT)
         {
            co_IntSendMessage(hWnd, WM_PAINT, 0, 0);
         }
      }
      else
      {
         if (Window->Flags & WINDOWOBJECT_NEED_NCPAINT)
         {
            TempRegion = IntGetNCUpdateRgn(Window, TRUE);
            Window->Flags &= ~WINDOWOBJECT_NEED_NCPAINT;
            MsqDecPaintCountQueue(Window->MessageQueue);
            co_IntSendMessage(hWnd, WM_NCPAINT, (WPARAM)TempRegion, 0);
            if ((HANDLE) 1 != TempRegion && NULL != TempRegion)
            {
               /* NOTE: The region can already be deleted! */
               GDIOBJ_FreeObj(GdiHandleTable, TempRegion, GDI_OBJECT_TYPE_REGION | GDI_OBJECT_TYPE_SILENT);
            }
         }

         if (Window->Flags & WINDOWOBJECT_NEED_ERASEBKGND)
         {
            if (Window->UpdateRegion)
            {
               hDC = UserGetDCEx(Window, Window->UpdateRegion,
                                 DCX_CACHE | DCX_USESTYLE |
                                 DCX_INTERSECTRGN | DCX_KEEPCLIPRGN);
               if (co_IntSendMessage(hWnd, WM_ERASEBKGND, (WPARAM)hDC, 0))
               {
                  Window->Flags &= ~WINDOWOBJECT_NEED_ERASEBKGND;
               }
               UserReleaseDC(Window, hDC, FALSE);
            }
         }
      }
   }

   /*
    * Check that the window is still valid at this point
    */
   if (!IntIsWindow(hWnd))
   {
      return;
   }

   /*
    * Paint child windows.
    */
   if (!(Flags & RDW_NOCHILDREN) && !(Window->Style & WS_MINIMIZE) &&
       ((Flags & RDW_ALLCHILDREN) || !(Window->Style & WS_CLIPCHILDREN)))
   {
      HWND *List, *phWnd;

      if ((List = IntWinListChildren(Window)))
      {
         /* FIXME: Handle WS_EX_TRANSPARENT */
         for (phWnd = List; *phWnd; ++phWnd)
         {
            Window = UserGetWindowObject(*phWnd);
            if (Window && (Window->Style & WS_VISIBLE))
            {
               USER_REFERENCE_ENTRY Ref;
               UserRefObjectCo(Window, &Ref);
               co_IntPaintWindows(Window, Flags, TRUE);
               UserDerefObjectCo(Window);
            }
         }
         ExFreePool(List);
      }
   }
}

/*
 * IntInvalidateWindows
 *
 * Internal function used by IntRedrawWindow.
 */

VOID FASTCALL
IntInvalidateWindows(PWINDOW_OBJECT Window, HRGN hRgn, ULONG Flags)
{
   INT RgnType;
   BOOL HadPaintMessage, HadNCPaintMessage;
   BOOL HasPaintMessage, HasNCPaintMessage;

   /*
    * If the nonclient is not to be redrawn, clip the region to the client
    * rect
    */
   if (0 != (Flags & RDW_INVALIDATE) && 0 == (Flags & RDW_FRAME))
   {
      HRGN hRgnClient;

      hRgnClient = UnsafeIntCreateRectRgnIndirect(&Window->ClientRect);
      RgnType = NtGdiCombineRgn(hRgn, hRgn, hRgnClient, RGN_AND);
      NtGdiDeleteObject(hRgnClient);
   }

   /*
    * Clip the given region with window rectangle (or region)
    */

   if (!Window->WindowRegion || (Window->Style & WS_MINIMIZE))
   {
      HRGN hRgnWindow;

      hRgnWindow = UnsafeIntCreateRectRgnIndirect(&Window->WindowRect);
      RgnType = NtGdiCombineRgn(hRgn, hRgn, hRgnWindow, RGN_AND);
      NtGdiDeleteObject(hRgnWindow);
   }
   else
   {
      NtGdiOffsetRgn(hRgn,
         -Window->WindowRect.left,
         -Window->WindowRect.top);
      RgnType = NtGdiCombineRgn(hRgn, hRgn, Window->WindowRegion, RGN_AND);
      NtGdiOffsetRgn(hRgn,
         Window->WindowRect.left,
         Window->WindowRect.top);
   }

   /*
    * Save current state of pending updates
    */

   HadPaintMessage = Window->UpdateRegion != NULL ||
                     Window->Flags & WINDOWOBJECT_NEED_INTERNALPAINT;
   HadNCPaintMessage = Window->Flags & WINDOWOBJECT_NEED_NCPAINT;

   /*
    * Update the region and flags
    */

   if (Flags & RDW_INVALIDATE && RgnType != NULLREGION)
   {
      if (Window->UpdateRegion == NULL)
      {
         Window->UpdateRegion = NtGdiCreateRectRgn(0, 0, 0, 0);
         GDIOBJ_SetOwnership(GdiHandleTable, Window->UpdateRegion, NULL);
      }

      if (NtGdiCombineRgn(Window->UpdateRegion, Window->UpdateRegion,
                          hRgn, RGN_OR) == NULLREGION)
      {
         GDIOBJ_SetOwnership(GdiHandleTable, Window->UpdateRegion, PsGetCurrentProcess());
         NtGdiDeleteObject(Window->UpdateRegion);
         Window->UpdateRegion = NULL;
      }

      if (Flags & RDW_FRAME)
         Window->Flags |= WINDOWOBJECT_NEED_NCPAINT;
      if (Flags & RDW_ERASE)
         Window->Flags |= WINDOWOBJECT_NEED_ERASEBKGND;

      Flags |= RDW_FRAME;
   }

   if (Flags & RDW_VALIDATE && RgnType != NULLREGION)
   {
      if (Window->UpdateRegion != NULL)
      {
         if (NtGdiCombineRgn(Window->UpdateRegion, Window->UpdateRegion,
                             hRgn, RGN_DIFF) == NULLREGION)
         {
            GDIOBJ_SetOwnership(GdiHandleTable, Window->UpdateRegion, PsGetCurrentProcess());
            NtGdiDeleteObject(Window->UpdateRegion);
            Window->UpdateRegion = NULL;
         }
      }

      if (Window->UpdateRegion == NULL)
         Window->Flags &= ~WINDOWOBJECT_NEED_ERASEBKGND;
      if (Flags & RDW_NOFRAME)
         Window->Flags &= ~WINDOWOBJECT_NEED_NCPAINT;
      if (Flags & RDW_NOERASE)
         Window->Flags &= ~WINDOWOBJECT_NEED_ERASEBKGND;
   }

   if (Flags & RDW_INTERNALPAINT)
   {
      Window->Flags |= WINDOWOBJECT_NEED_INTERNALPAINT;
   }

   if (Flags & RDW_NOINTERNALPAINT)
   {
      Window->Flags &= ~WINDOWOBJECT_NEED_INTERNALPAINT;
   }

   /*
    * Process children if needed
    */

   if (!(Flags & RDW_NOCHILDREN) && !(Window->Style & WS_MINIMIZE) &&
         ((Flags & RDW_ALLCHILDREN) || !(Window->Style & WS_CLIPCHILDREN)))
   {
      PWINDOW_OBJECT Child;

      for (Child = Window->FirstChild; Child; Child = Child->NextSibling)
      {
         if (Child->Style & WS_VISIBLE)
         {
            /*
             * Recursive call to update children UpdateRegion
             */
            HRGN hRgnTemp = NtGdiCreateRectRgn(0, 0, 0, 0);
            NtGdiCombineRgn(hRgnTemp, hRgn, 0, RGN_COPY);
            IntInvalidateWindows(Child, hRgnTemp, Flags);
            NtGdiDeleteObject(hRgnTemp);
         }

      }
   }

   /*
    * Fake post paint messages to window message queue if needed
    */

   HasPaintMessage = Window->UpdateRegion != NULL ||
                     Window->Flags & WINDOWOBJECT_NEED_INTERNALPAINT;
   HasNCPaintMessage = Window->Flags & WINDOWOBJECT_NEED_NCPAINT;

   if (HasPaintMessage != HadPaintMessage)
   {
      if (HadPaintMessage)
         MsqDecPaintCountQueue(Window->MessageQueue);
      else
         MsqIncPaintCountQueue(Window->MessageQueue);
   }

   if (HasNCPaintMessage != HadNCPaintMessage)
   {
      if (HadNCPaintMessage)
         MsqDecPaintCountQueue(Window->MessageQueue);
      else
         MsqIncPaintCountQueue(Window->MessageQueue);
   }

}

/*
 * IntIsWindowDrawable
 *
 * Remarks
 *    Window is drawable when it is visible and all parents are not
 *    minimized.
 */

BOOL FASTCALL
IntIsWindowDrawable(PWINDOW_OBJECT Window)
{
   PWINDOW_OBJECT Wnd;

   for (Wnd = Window; Wnd != NULL; Wnd = Wnd->Parent)
   {
      if (!(Wnd->Style & WS_VISIBLE) ||
            ((Wnd->Style & WS_MINIMIZE) && (Wnd != Window)))
      {
         return FALSE;
      }
   }

   return TRUE;
}

/*
 * IntRedrawWindow
 *
 * Internal version of NtUserRedrawWindow that takes WINDOW_OBJECT as
 * first parameter.
 */

BOOL FASTCALL
co_UserRedrawWindow(PWINDOW_OBJECT Window, const RECT* UpdateRect, HRGN UpdateRgn,
                    ULONG Flags)
{
   HRGN hRgn = NULL;

   /*
    * Step 1.
    * Validation of passed parameters.
    */

   if (!IntIsWindowDrawable(Window) ||
         (Flags & (RDW_VALIDATE | RDW_INVALIDATE)) ==
         (RDW_VALIDATE | RDW_INVALIDATE))
   {
      return FALSE;
   }

   /*
    * Step 2.
    * Transform the parameters UpdateRgn and UpdateRect into
    * a region hRgn specified in screen coordinates.
    */

   if (Flags & (RDW_INVALIDATE | RDW_VALIDATE))
   {
      if (UpdateRgn != NULL)
      {
         hRgn = NtGdiCreateRectRgn(0, 0, 0, 0);
         if (NtGdiCombineRgn(hRgn, UpdateRgn, NULL, RGN_COPY) == NULLREGION)
         {
            NtGdiDeleteObject(hRgn);
            hRgn = NULL;
         }
         else
            NtGdiOffsetRgn(hRgn, Window->ClientRect.left, Window->ClientRect.top);
      }
      else if (UpdateRect != NULL)
      {
         if (!IntGdiIsEmptyRect(UpdateRect))
         {
            hRgn = UnsafeIntCreateRectRgnIndirect((RECT *)UpdateRect);
            NtGdiOffsetRgn(hRgn, Window->ClientRect.left, Window->ClientRect.top);
         }
      }
      else if ((Flags & (RDW_INVALIDATE | RDW_FRAME)) == (RDW_INVALIDATE | RDW_FRAME) ||
               (Flags & (RDW_VALIDATE | RDW_NOFRAME)) == (RDW_VALIDATE | RDW_NOFRAME))
      {
         if (!IntGdiIsEmptyRect(&Window->WindowRect))
            hRgn = UnsafeIntCreateRectRgnIndirect(&Window->WindowRect);
      }
      else
      {
         if (!IntGdiIsEmptyRect(&Window->ClientRect))
            hRgn = UnsafeIntCreateRectRgnIndirect(&Window->ClientRect);
      }
   }

   /*
    * Step 3.
    * Adjust the window update region depending on hRgn and flags.
    */

   if (Flags & (RDW_INVALIDATE | RDW_VALIDATE | RDW_INTERNALPAINT | RDW_NOINTERNALPAINT) &&
       hRgn != NULL)
   {
      IntInvalidateWindows(Window, hRgn, Flags);
   }

   /*
    * Step 4.
    * Repaint and erase windows if needed.
    */

   if (Flags & (RDW_ERASENOW | RDW_UPDATENOW))
   {
      co_IntPaintWindows(Window, Flags, FALSE);
   }

   /*
    * Step 5.
    * Cleanup ;-)
    */

   if (hRgn != NULL)
   {
      NtGdiDeleteObject(hRgn);
   }

   return TRUE;
}

BOOL FASTCALL
IntIsWindowDirty(PWINDOW_OBJECT Window)
{
   return (Window->Style & WS_VISIBLE) &&
          ((Window->UpdateRegion != NULL) ||
           (Window->Flags & WINDOWOBJECT_NEED_INTERNALPAINT) ||
           (Window->Flags & WINDOWOBJECT_NEED_NCPAINT));
}

HWND FASTCALL
IntFindWindowToRepaint(PWINDOW_OBJECT Window, PW32THREAD Thread)
{
   HWND hChild;
   PWINDOW_OBJECT TempWindow;

   for (; Window != NULL; Window = Window->NextSibling)
   {
      if (IntWndBelongsToThread(Window, Thread) &&
          IntIsWindowDirty(Window))
      {
         /* Make sure all non-transparent siblings are already drawn. */
         if (Window->ExStyle & WS_EX_TRANSPARENT)
         {
            for (TempWindow = Window->NextSibling; TempWindow != NULL;
                 TempWindow = TempWindow->NextSibling)
            {
               if (!(TempWindow->ExStyle & WS_EX_TRANSPARENT) &&
                   IntWndBelongsToThread(TempWindow, Thread) &&
                   IntIsWindowDirty(TempWindow))
               {
                  return TempWindow->hSelf;
               }
            }
         }

         return Window->hSelf;
      }

      if (Window->FirstChild)
      {
         hChild = IntFindWindowToRepaint(Window->FirstChild, Thread);
         if (hChild != NULL)
            return hChild;
      }
   }

   return NULL;
}

BOOL FASTCALL
IntGetPaintMessage(HWND hWnd, UINT MsgFilterMin, UINT MsgFilterMax,
                   PW32THREAD Thread, MSG *Message, BOOL Remove)
{
   PUSER_MESSAGE_QUEUE MessageQueue = (PUSER_MESSAGE_QUEUE)Thread->MessageQueue;

   if (!MessageQueue->PaintCount)
      return FALSE;

   if ((MsgFilterMin != 0 || MsgFilterMax != 0) &&
         (MsgFilterMin > WM_PAINT || MsgFilterMax < WM_PAINT))
      return FALSE;

   Message->hwnd = IntFindWindowToRepaint(UserGetDesktopWindow(), PsGetCurrentThreadWin32Thread());

   if (Message->hwnd == NULL)
   {
      DPRINT1("PAINTING BUG: Thread marked as containing dirty windows, but no dirty windows found!\n");
      return FALSE;
   }

   if (hWnd != NULL && Message->hwnd != hWnd)
      return FALSE;

   Message->message = WM_PAINT;
   Message->wParam = Message->lParam = 0;

   return TRUE;
}

static
HWND FASTCALL
co_IntFixCaret(PWINDOW_OBJECT Window, LPRECT lprc, UINT flags)
{
   PDESKTOP_OBJECT Desktop;
   PTHRDCARETINFO CaretInfo;
   HWND hWndCaret;
   PWINDOW_OBJECT WndCaret;

   ASSERT_REFS_CO(Window);

   Desktop = ((PW32THREAD)PsGetCurrentThread()->Tcb.Win32Thread)->Desktop;
   CaretInfo = ((PUSER_MESSAGE_QUEUE)Desktop->ActiveMessageQueue)->CaretInfo;
   hWndCaret = CaretInfo->hWnd;

   WndCaret = UserGetWindowObject(hWndCaret);

   //fix: check for WndCaret can be null
   if (WndCaret == Window ||
         ((flags & SW_SCROLLCHILDREN) && IntIsChildWindow(Window, WndCaret)))
   {
      POINT pt, FromOffset, ToOffset, Offset;
      RECT rcCaret;

      pt.x = CaretInfo->Pos.x;
      pt.y = CaretInfo->Pos.y;
      IntGetClientOrigin(WndCaret, &FromOffset);
      IntGetClientOrigin(Window, &ToOffset);
      Offset.x = FromOffset.x - ToOffset.x;
      Offset.y = FromOffset.y - ToOffset.y;
      rcCaret.left = pt.x;
      rcCaret.top = pt.y;
      rcCaret.right = pt.x + CaretInfo->Size.cx;
      rcCaret.bottom = pt.y + CaretInfo->Size.cy;
      if (IntGdiIntersectRect(lprc, lprc, &rcCaret))
      {
         co_UserHideCaret(0);
         lprc->left = pt.x;
         lprc->top = pt.y;
         return hWndCaret;
      }
   }

   return 0;
}

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * NtUserBeginPaint
 *
 * Status
 *    @implemented
 */

HDC STDCALL
NtUserBeginPaint(HWND hWnd, PAINTSTRUCT* UnsafePs)
{
   PWINDOW_OBJECT Window = NULL;
   PAINTSTRUCT Ps;
   PROSRGNDATA Rgn;
   NTSTATUS Status;
   DECLARE_RETURN(HDC);
   USER_REFERENCE_ENTRY Ref;

   DPRINT("Enter NtUserBeginPaint\n");
   UserEnterExclusive();

   if (!(Window = UserGetWindowObject(hWnd)))
   {
      RETURN( NULL);
   }

   UserRefObjectCo(Window, &Ref);
   
   co_UserHideCaret(Window);

   if (Window->Flags & WINDOWOBJECT_NEED_NCPAINT)
   {
      HRGN hRgn;

      hRgn = IntGetNCUpdateRgn(Window, FALSE);
      Window->Flags &= ~WINDOWOBJECT_NEED_NCPAINT;
      MsqDecPaintCountQueue(Window->MessageQueue);
      co_IntSendMessage(hWnd, WM_NCPAINT, (WPARAM)hRgn, 0);
      if (hRgn != (HANDLE)1 && hRgn != NULL)
      {
         /* NOTE: The region can already by deleted! */
         GDIOBJ_FreeObj(GdiHandleTable, hRgn, GDI_OBJECT_TYPE_REGION | GDI_OBJECT_TYPE_SILENT);
      }
   }

   RtlZeroMemory(&Ps, sizeof(PAINTSTRUCT));

   Ps.hdc = UserGetDCEx(Window, Window->UpdateRegion, DCX_INTERSECTRGN | DCX_USESTYLE);
   if (!Ps.hdc)
   {
      RETURN(NULL);
   }

   if (Window->UpdateRegion != NULL)
   {
      MsqDecPaintCountQueue(Window->MessageQueue);
      Rgn = RGNDATA_LockRgn(Window->UpdateRegion);
      if (NULL != Rgn)
      {
         UnsafeIntGetRgnBox(Rgn, &Ps.rcPaint);
         RGNDATA_UnlockRgn(Rgn);
         IntGdiIntersectRect(&Ps.rcPaint, &Ps.rcPaint, &Window->ClientRect);
         if (! IntGdiIsEmptyRect(&Ps.rcPaint))
         {
            IntGdiOffsetRect(&Ps.rcPaint,
                             -Window->ClientRect.left,
                             -Window->ClientRect.top);
         }
      }
      else
      {
         IntGetClientRect(Window, &Ps.rcPaint);
      }
      GDIOBJ_SetOwnership(GdiHandleTable, Window->UpdateRegion, PsGetCurrentProcess());
      Window->UpdateRegion = NULL;
   }
   else
   {
      if (Window->Flags & WINDOWOBJECT_NEED_INTERNALPAINT)
         MsqDecPaintCountQueue(Window->MessageQueue);
         
      IntGetClientRect(Window, &Ps.rcPaint);
   }

   Window->Flags &= ~WINDOWOBJECT_NEED_INTERNALPAINT;

   if (Window->Flags & WINDOWOBJECT_NEED_ERASEBKGND)
   {
      Window->Flags &= ~WINDOWOBJECT_NEED_ERASEBKGND;
      Ps.fErase = !co_IntSendMessage(hWnd, WM_ERASEBKGND, (WPARAM)Ps.hdc, 0);
   }
   else
   {
      Ps.fErase = FALSE;
   }

   Status = MmCopyToCaller(UnsafePs, &Ps, sizeof(PAINTSTRUCT));
   if (! NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      RETURN(NULL);
   }

   RETURN(Ps.hdc);

CLEANUP:
   if (Window) UserDerefObjectCo(Window);
   
   DPRINT("Leave NtUserBeginPaint, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;

}

/*
 * NtUserEndPaint
 *
 * Status
 *    @implemented
 */

BOOL STDCALL
NtUserEndPaint(HWND hWnd, CONST PAINTSTRUCT* lPs)
{
   PWINDOW_OBJECT Window;
   DECLARE_RETURN(BOOL);
   USER_REFERENCE_ENTRY Ref;

   DPRINT("Enter NtUserEndPaint\n");
   UserEnterExclusive();

   if (!(Window = UserGetWindowObject(hWnd)))
   {
      RETURN(FALSE);
   }

   UserReleaseDC(Window, lPs->hdc, TRUE);

   UserRefObjectCo(Window, &Ref);
   co_UserShowCaret(Window);
   UserDerefObjectCo(Window);

   RETURN(TRUE);

CLEANUP:
   DPRINT("Leave NtUserEndPaint, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


INT FASTCALL
co_UserGetUpdateRgn(PWINDOW_OBJECT Window, HRGN hRgn, BOOL bErase)
{
   int RegionType;
   RECT Rect;

   ASSERT_REFS_CO(Window);

   if (Window->UpdateRegion == NULL)
   {
      RegionType = (NtGdiSetRectRgn(hRgn, 0, 0, 0, 0) ? NULLREGION : ERROR);
   }
   else
   {
      Rect = Window->ClientRect;
      IntIntersectWithParents(Window, &Rect);
      NtGdiSetRectRgn(hRgn, Rect.left, Rect.top, Rect.right, Rect.bottom);
      RegionType = NtGdiCombineRgn(hRgn, hRgn, Window->UpdateRegion, RGN_AND);
      NtGdiOffsetRgn(hRgn, -Window->ClientRect.left, -Window->ClientRect.top);
   }

   if (bErase && RegionType != NULLREGION && RegionType != ERROR)
   {
      co_UserRedrawWindow(Window, NULL, NULL, RDW_ERASENOW | RDW_NOCHILDREN);
   }

   return RegionType;
}

/*
 * NtUserGetUpdateRgn
 *
 * Status
 *    @implemented
 */

INT STDCALL
NtUserGetUpdateRgn(HWND hWnd, HRGN hRgn, BOOL bErase)
{
   DECLARE_RETURN(INT);
   PWINDOW_OBJECT Window;
   INT ret;
   USER_REFERENCE_ENTRY Ref;

   DPRINT("Enter NtUserGetUpdateRgn\n");
   UserEnterExclusive();

   if (!(Window = UserGetWindowObject(hWnd)))
   {
      RETURN(ERROR);
   }

   UserRefObjectCo(Window, &Ref);
   ret = co_UserGetUpdateRgn(Window, hRgn, bErase);
   UserDerefObjectCo(Window);

   RETURN(ret);

CLEANUP:
   DPRINT("Leave NtUserGetUpdateRgn, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/*
 * NtUserGetUpdateRect
 *
 * Status
 *    @implemented
 */

BOOL STDCALL
NtUserGetUpdateRect(HWND hWnd, LPRECT UnsafeRect, BOOL bErase)
{
   PWINDOW_OBJECT Window;
   RECT Rect;
   INT RegionType;
   PROSRGNDATA RgnData;
   NTSTATUS Status;
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserGetUpdateRect\n");
   UserEnterExclusive();

   if (!(Window = UserGetWindowObject(hWnd)))
   {
      RETURN(FALSE);
   }

   if (Window->UpdateRegion == NULL)
   {
      Rect.left = Rect.top = Rect.right = Rect.bottom = 0;
   }
   else
   {
      /* Get the update region bounding box. */
      if (Window->UpdateRegion == (HRGN)1)
      {
         Rect = Window->ClientRect;
      }
      else
      {
         RgnData = RGNDATA_LockRgn(Window->UpdateRegion);
         ASSERT(RgnData != NULL);
         RegionType = UnsafeIntGetRgnBox(RgnData, &Rect);
         RGNDATA_UnlockRgn(RgnData);

         if (RegionType != ERROR && RegionType != NULLREGION)
            IntGdiIntersectRect(&Rect, &Rect, &Window->ClientRect);
      }

      if (IntIntersectWithParents(Window, &Rect))
      {
         IntGdiOffsetRect(&Rect,
                          -Window->ClientRect.left,
                          -Window->ClientRect.top);
      } else
      {
         Rect.left = Rect.top = Rect.right = Rect.bottom = 0;
      }
   }

   if (bErase && !IntGdiIsEmptyRect(&Rect))
   {
      USER_REFERENCE_ENTRY Ref;
      UserRefObjectCo(Window, &Ref);
      co_UserRedrawWindow(Window, NULL, NULL, RDW_ERASENOW | RDW_NOCHILDREN);
      UserDerefObjectCo(Window);
   }

   if (UnsafeRect != NULL)
   {
      Status = MmCopyToCaller(UnsafeRect, &Rect, sizeof(RECT));
      if (!NT_SUCCESS(Status))
      {
         SetLastWin32Error(ERROR_INVALID_PARAMETER);
         RETURN(FALSE);
      }
   }

   RETURN(!IntGdiIsEmptyRect(&Rect));

CLEANUP:
   DPRINT("Leave NtUserGetUpdateRect, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/*
 * NtUserRedrawWindow
 *
 * Status
 *    @implemented
 */

BOOL STDCALL
NtUserRedrawWindow(HWND hWnd, CONST RECT *lprcUpdate, HRGN hrgnUpdate,
                   UINT flags)
{
   RECT SafeUpdateRect;
   NTSTATUS Status;
   PWINDOW_OBJECT Wnd;
   DECLARE_RETURN(BOOL);
   USER_REFERENCE_ENTRY Ref;

   DPRINT("Enter NtUserRedrawWindow\n");
   UserEnterExclusive();

   if (!(Wnd = UserGetWindowObject(hWnd ? hWnd : IntGetDesktopWindow())))
   {
      RETURN( FALSE);
   }

   if (lprcUpdate != NULL)
   {
      Status = MmCopyFromCaller(&SafeUpdateRect, (PRECT)lprcUpdate,
                                sizeof(RECT));

      if (!NT_SUCCESS(Status))
      {
         SetLastWin32Error(ERROR_INVALID_PARAMETER);
         RETURN( FALSE);
      }
   }

   UserRefObjectCo(Wnd, &Ref);

   Status = co_UserRedrawWindow(Wnd, NULL == lprcUpdate ? NULL : &SafeUpdateRect,
                                hrgnUpdate, flags);

   UserDerefObjectCo(Wnd);

   if (!NT_SUCCESS(Status))
   {
      /* IntRedrawWindow fails only in case that flags are invalid */
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      RETURN( FALSE);
   }

   RETURN( TRUE);

CLEANUP:
   DPRINT("Leave NtUserRedrawWindow, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}



static
DWORD FASTCALL
UserScrollDC(HDC hDC, INT dx, INT dy, const RECT *lprcScroll,
             const RECT *lprcClip, HRGN hrgnUpdate, LPRECT lprcUpdate)
{
   RECT rSrc, rClipped_src, rClip, rDst, offset;
   PDC DC;

   /*
    * Compute device clipping region (in device coordinates).
    */

   DC = DC_LockDc(hDC);
   if (NULL == DC)
   {
      return FALSE;
   }
   if (lprcScroll)
      rSrc = *lprcScroll;
   else
      IntGdiGetClipBox(hDC, &rSrc);
   IntLPtoDP(DC, (LPPOINT)&rSrc, 2);

   if (lprcClip)
      rClip = *lprcClip;
   else
      IntGdiGetClipBox(hDC, &rClip);
   IntLPtoDP(DC, (LPPOINT)&rClip, 2);

   IntGdiIntersectRect(&rClipped_src, &rSrc, &rClip);

   rDst = rClipped_src;
   IntGdiSetRect(&offset, 0, 0, dx, dy);
   IntLPtoDP(DC, (LPPOINT)&offset, 2);
   IntGdiOffsetRect(&rDst, offset.right - offset.left,  offset.bottom - offset.top);
   IntGdiIntersectRect(&rDst, &rDst, &rClip);

   /*
    * Copy bits, if possible.
    */

   if (rDst.bottom > rDst.top && rDst.right > rDst.left)
   {
      RECT rDst_lp = rDst, rSrc_lp = rDst;

      IntGdiOffsetRect(&rSrc_lp, offset.left - offset.right, offset.top - offset.bottom);
      IntDPtoLP(DC, (LPPOINT)&rDst_lp, 2);
      IntDPtoLP(DC, (LPPOINT)&rSrc_lp, 2);
      DC_UnlockDc(DC);

      if (!NtGdiBitBlt(hDC, rDst_lp.left, rDst_lp.top, rDst_lp.right - rDst_lp.left,
                       rDst_lp.bottom - rDst_lp.top, hDC, rSrc_lp.left, rSrc_lp.top,
                       SRCCOPY, 0, 0))
         return FALSE;
   }
   else
   {
      DC_UnlockDc(DC);
   }

   /*
    * Compute update areas.  This is the clipped source or'ed with the
    * unclipped source translated minus the clipped src translated (rDst)
    * all clipped to rClip.
    */

   if (hrgnUpdate || lprcUpdate)
   {
      HRGN hRgn = hrgnUpdate, hRgn2;

      if (hRgn)
         NtGdiSetRectRgn(hRgn, rClipped_src.left, rClipped_src.top, rClipped_src.right, rClipped_src.bottom);
      else
         hRgn = NtGdiCreateRectRgn(rClipped_src.left, rClipped_src.top, rClipped_src.right, rClipped_src.bottom);

      hRgn2 = UnsafeIntCreateRectRgnIndirect(&rSrc);
      NtGdiOffsetRgn(hRgn2, offset.right - offset.left,  offset.bottom - offset.top);
      NtGdiCombineRgn(hRgn, hRgn, hRgn2, RGN_OR);

      NtGdiSetRectRgn(hRgn2, rDst.left, rDst.top, rDst.right, rDst.bottom);
      NtGdiCombineRgn(hRgn, hRgn, hRgn2, RGN_DIFF);

      NtGdiSetRectRgn(hRgn2, rClip.left, rClip.top, rClip.right, rClip.bottom);
      NtGdiCombineRgn(hRgn, hRgn, hRgn2, RGN_AND);

      if (lprcUpdate)
      {
         NtGdiGetRgnBox(hRgn, lprcUpdate);

         /* Put the lprcUpdate in logical coordinate */
         NtGdiDPtoLP(hDC, (LPPOINT)lprcUpdate, 2);
      }
      if (!hrgnUpdate)
         NtGdiDeleteObject(hRgn);
      NtGdiDeleteObject(hRgn2);
   }
   return TRUE;
}




/*
 * NtUserScrollDC
 *
 * Status
 *    @implemented
 */

DWORD STDCALL
NtUserScrollDC(HDC hDC, INT dx, INT dy, const RECT *lprcScroll,
               const RECT *lprcClip, HRGN hrgnUpdate, LPRECT lprcUpdate)
{
   DECLARE_RETURN(DWORD);

   DPRINT("Enter NtUserScrollDC\n");
   UserEnterExclusive();

   RETURN( UserScrollDC(hDC, dx, dy, lprcScroll, lprcClip, hrgnUpdate, lprcUpdate));

CLEANUP:
   DPRINT("Leave NtUserScrollDC, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;

}

/*
 * NtUserScrollWindowEx
 *
 * Status
 *    @implemented
 */

DWORD STDCALL
NtUserScrollWindowEx(HWND hWnd, INT dx, INT dy, const RECT *UnsafeRect,
                     const RECT *UnsafeClipRect, HRGN hrgnUpdate, LPRECT rcUpdate, UINT flags)
{
   RECT rc, cliprc, caretrc, rect, clipRect;
   INT Result;
   PWINDOW_OBJECT Window = NULL, CaretWnd;
   HDC hDC;
   HRGN hrgnTemp;
   HWND hwndCaret;
   BOOL bUpdate = (rcUpdate || hrgnUpdate || flags & (SW_INVALIDATE | SW_ERASE));
   BOOL bOwnRgn = TRUE;
   NTSTATUS Status;
   DECLARE_RETURN(DWORD);
   USER_REFERENCE_ENTRY Ref, CaretRef;

   DPRINT("Enter NtUserScrollWindowEx\n");
   UserEnterExclusive();

   Window = UserGetWindowObject(hWnd);
   if (!Window || !IntIsWindowDrawable(Window))
   {
      Window = NULL; /* prevent deref at cleanup */
      RETURN( ERROR);
   }
   UserRefObjectCo(Window, &Ref);

   IntGetClientRect(Window, &rc);

   if (NULL != UnsafeRect)
   {
      Status = MmCopyFromCaller(&rect, UnsafeRect, sizeof(RECT));
      if (! NT_SUCCESS(Status))
      {
         SetLastNtError(Status);
         RETURN( ERROR);
      }
      IntGdiIntersectRect(&rc, &rc, &rect);
   }

   if (NULL != UnsafeClipRect)
   {
      Status = MmCopyFromCaller(&clipRect, UnsafeClipRect, sizeof(RECT));
      if (! NT_SUCCESS(Status))
      {
         SetLastNtError(Status);
         RETURN( ERROR);
      }
      IntGdiIntersectRect(&cliprc, &rc, &clipRect);
   }
   else
      cliprc = rc;

   if (cliprc.right <= cliprc.left || cliprc.bottom <= cliprc.top ||
         (dx == 0 && dy == 0))
   {
      RETURN( NULLREGION);
   }

   caretrc = rc;
   hwndCaret = co_IntFixCaret(Window, &caretrc, flags);

   if (hrgnUpdate)
      bOwnRgn = FALSE;
   else if (bUpdate)
      hrgnUpdate = NtGdiCreateRectRgn(0, 0, 0, 0);

   hDC = UserGetDCEx(Window, 0, DCX_CACHE | DCX_USESTYLE);
   if (hDC)
   {
      UserScrollDC(hDC, dx, dy, &rc, &cliprc, hrgnUpdate, rcUpdate);
      UserReleaseDC(Window, hDC, FALSE);
   }

   /*
    * Take into account the fact that some damage may have occurred during
    * the scroll.
    */

   hrgnTemp = NtGdiCreateRectRgn(0, 0, 0, 0);
   Result = co_UserGetUpdateRgn(Window, hrgnTemp, FALSE);
   if (Result != NULLREGION)
   {
      HRGN hrgnClip = UnsafeIntCreateRectRgnIndirect(&cliprc);
      NtGdiOffsetRgn(hrgnTemp, dx, dy);
      NtGdiCombineRgn(hrgnTemp, hrgnTemp, hrgnClip, RGN_AND);
      co_UserRedrawWindow(Window, NULL, hrgnTemp, RDW_INVALIDATE | RDW_ERASE);
      NtGdiDeleteObject(hrgnClip);
   }

   NtGdiDeleteObject(hrgnTemp);

   if (flags & SW_SCROLLCHILDREN)
   {
      HWND *List = IntWinListChildren(Window);
      if (List)
      {
         int i;
         RECT r, dummy;
         POINT ClientOrigin;
         PWINDOW_OBJECT Wnd;
         USER_REFERENCE_ENTRY WndRef;

         IntGetClientOrigin(Window, &ClientOrigin);
         for (i = 0; List[i]; i++)
         {
            if (!(Wnd = UserGetWindowObject(List[i])))
               continue;

            r = Wnd->WindowRect;
            r.left -= ClientOrigin.x;
            r.top -= ClientOrigin.y;
            r.right -= ClientOrigin.x;
            r.bottom -= ClientOrigin.y;

            if (! UnsafeRect || IntGdiIntersectRect(&dummy, &r, &rc))
            {
               UserRefObjectCo(Wnd, &WndRef);
               co_WinPosSetWindowPos(Wnd, 0, r.left + dx, r.top + dy, 0, 0,
                                     SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE |
                                     SWP_NOREDRAW);
               UserDerefObjectCo(Wnd);
            }

         }
         ExFreePool(List);
      }
   }

   if (flags & (SW_INVALIDATE | SW_ERASE))
      co_UserRedrawWindow(Window, NULL, hrgnUpdate, RDW_INVALIDATE | RDW_ERASE |
                          ((flags & SW_ERASE) ? RDW_ERASENOW : 0) |
                          ((flags & SW_SCROLLCHILDREN) ? RDW_ALLCHILDREN : 0));

   if (bOwnRgn && hrgnUpdate)
      NtGdiDeleteObject(hrgnUpdate);

   if ((CaretWnd = UserGetWindowObject(hwndCaret)))
   {
      UserRefObjectCo(CaretWnd, &CaretRef);

      co_IntSetCaretPos(caretrc.left + dx, caretrc.top + dy);
      co_UserShowCaret(CaretWnd);

      UserDerefObjectCo(CaretWnd);
   }

   RETURN(Result);

CLEANUP:
   if (Window)
      UserDerefObjectCo(Window);

   DPRINT("Leave NtUserScrollWindowEx, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


BOOL
UserDrawSysMenuButton(
   PWINDOW_OBJECT pWnd, 
   HDC hDc, 
   LPRECT lpRc, 
   BOOL Down)
{
   HICON hIcon;
   PCURICON_OBJECT pIcon;
   
   ASSERT(pWnd && lpRc);
   
   /* Get the icon to draw. We don't care about WM_GETICON here. */
   
   hIcon = pWnd->Class->hIconSm;
   
   if(!hIcon)
   {
      DPRINT("Wnd class has no small icon.\n");
      hIcon = pWnd->Class->hIcon;
   }
   
   if(!hIcon)
   {
      DPRINT("Wnd class hasn't any icon.\n");
      //FIXME: Draw "winlogo" icon. 
      return FALSE;
   }
   
   if(!(pIcon = UserGetCurIconObject(hIcon)))
   {
      DPRINT1("UserGetCurIconObject() failed!\n");
      return FALSE;
   }

   return UserDrawIconEx(hDc, lpRc->left, lpRc->top, pIcon,
                        UserGetSystemMetrics(SM_CXSMICON), 
                        UserGetSystemMetrics(SM_CYSMICON),
                        0, NULL, DI_NORMAL);
}

BOOL
UserDrawCaptionText(HDC hDc,
   const PUNICODE_STRING Text,
   const LPRECT lpRc,
   UINT uFlags)
{
   HFONT hOldFont = NULL, hFont = NULL;
   COLORREF OldTextColor;
   NONCLIENTMETRICS nclm;
   NTSTATUS Status;
   #ifndef NDEBUG
   INT i;
   DPRINT("%s:", __FUNCTION__);
   for(i = 0; i < Text->Length/sizeof(WCHAR); i++)
      DbgPrint("%C", Text->Buffer[i]);
   DbgPrint(", %d\n", Text->Length/sizeof(WCHAR));
   #endif
   
   nclm.cbSize = sizeof(nclm);
   if(!IntSystemParametersInfo(SPI_GETNONCLIENTMETRICS,
      sizeof(NONCLIENTMETRICS), &nclm, 0)) 
   {
      DPRINT1("%s: IntSystemParametersInfo() failed!\n", __FUNCTION__);
      return FALSE;
   }

   NtGdiSetBkMode(hDc, TRANSPARENT);

   if(uFlags & DC_SMALLCAP)
      Status = TextIntCreateFontIndirect(&nclm.lfSmCaptionFont, &hFont);
   else Status = TextIntCreateFontIndirect(&nclm.lfCaptionFont, &hFont);

   if(!NT_SUCCESS(Status)) 
   {
      DPRINT1("%s: TextIntCreateFontIndirect() failed! Status: 0x%x\n",
         __FUNCTION__, Status);
      return FALSE;
   }

   hOldFont = NtGdiSelectObject(hDc, hFont);
   if(!hOldFont) 
   {
      DPRINT1("%s: SelectObject() failed!\n", __FUNCTION__);
      NtGdiDeleteObject(hFont);
      return FALSE;
   }

   if(uFlags & DC_INBUTTON)
      OldTextColor = NtGdiSetTextColor(hDc, IntGetSysColor(COLOR_BTNTEXT));
   else OldTextColor = NtGdiSetTextColor(hDc, IntGetSysColor(uFlags & DC_ACTIVE
         ? COLOR_CAPTIONTEXT : COLOR_INACTIVECAPTIONTEXT));
   
   //FIXME: If string doesn't fit to rc, truncate it and add ellipsis.
   
   NtGdiTextOut(hDc, lpRc->left, 
      lpRc->top, Text->Buffer, 
      Text->Length/sizeof(WCHAR));
   
   NtGdiSetTextColor(hDc, OldTextColor);
   NtGdiSelectObject(hDc, hOldFont);
   NtGdiDeleteObject(hFont);
   
   return TRUE;
}

BOOL UserDrawCaption(
   PWINDOW_OBJECT pWnd,
   HDC hDc,
   LPCRECT lpRc,
   HFONT hFont,
   HICON hIcon,
   const PUNICODE_STRING str,
   UINT uFlags)
{
   BOOL Ret = FALSE;
   HBITMAP hMemBmp = NULL, hOldBmp = NULL;
   HBRUSH hOldBrush = NULL;
   HDC hMemDc = NULL;
   ULONG Height;
   UINT VCenter = 0, Padding = 0;
   RECT r = *lpRc;
   LONG ButtonWidth, IconWidth;
   BOOL HasIcon;
   
   //ASSERT(pWnd != NULL);

   hMemBmp = NtGdiCreateCompatibleBitmap(hDc, 
      lpRc->right - lpRc->left, 
      lpRc->bottom - lpRc->top);

   if(!hMemBmp)
   {
      DPRINT1("%s: NtGdiCreateCompatibleBitmap() failed!\n", __FUNCTION__);
      return FALSE;
   }

   hMemDc = NtGdiCreateCompatibleDC(hDc);
   if(!hMemDc)
   {
      DPRINT1("%s: NtGdiCreateCompatibleDC() failed!\n", __FUNCTION__);
      goto cleanup;
   }

   hOldBmp = NtGdiSelectObject(hMemDc, hMemBmp);
   if(!hOldBmp)
   {
      DPRINT1("%s: NtGdiSelectObject() failed!\n", __FUNCTION__);
      goto cleanup;
   }

   Height = UserGetSystemMetrics(SM_CYCAPTION) - 1;
   VCenter = (lpRc->bottom - lpRc->top) / 2;
   Padding = VCenter - (Height / 2);

   if ((!hIcon) && (pWnd != NULL))
   {
     HasIcon = (uFlags & DC_ICON) && (pWnd->Style & WS_SYSMENU) 
        && !(uFlags & DC_SMALLCAP) && !(pWnd->ExStyle & WS_EX_DLGMODALFRAME) 
        && !(pWnd->ExStyle & WS_EX_TOOLWINDOW);
   }
   else 
     HasIcon = (BOOL) hIcon;

   IconWidth = UserGetSystemMetrics(SM_CXSIZE) + Padding;

   r.left = Padding;
   r.right = r.left + (lpRc->right - lpRc->left);
   r.top = Padding;
   r.bottom = r.top + (Height / 2);

   // Draw the caption background
   if(uFlags & DC_INBUTTON)
   {
      hOldBrush = NtGdiSelectObject(hMemDc, 
         IntGetSysColorBrush(uFlags & DC_ACTIVE ?
            COLOR_BTNFACE : COLOR_BTNSHADOW));

      if(!hOldBrush)
      {
         DPRINT1("%s: NtGdiSelectObject() failed!\n", __FUNCTION__);
         goto cleanup;
      }

      if(!NtGdiPatBlt(hMemDc, 0, 0,
         lpRc->right - lpRc->left,
         lpRc->bottom - lpRc->top,
         PATCOPY)) 
      {
         DPRINT1("%s: NtGdiPatBlt() failed!\n", __FUNCTION__);
         goto cleanup;
      }
      
      if(HasIcon) r.left+=IconWidth;
   }
   else 
   {
      r.right = (lpRc->right - lpRc->left);
      if(uFlags & DC_SMALLCAP)
         ButtonWidth = UserGetSystemMetrics(SM_CXSMSIZE) - 2;
      else ButtonWidth = UserGetSystemMetrics(SM_CXSIZE) - 2;
        
      hOldBrush = NtGdiSelectObject(hMemDc, 
         IntGetSysColorBrush(uFlags & DC_ACTIVE ? 
            COLOR_ACTIVECAPTION : COLOR_INACTIVECAPTION));
   
      if(!hOldBrush)
      {
         DPRINT1("%s: NtGdiSelectObject() failed!\n", __FUNCTION__);
         goto cleanup;
      }

      if(HasIcon && (uFlags & DC_GRADIENT))
      {
         NtGdiPatBlt(hMemDc, 0, 0, 
            IconWidth+1, 
            lpRc->bottom - lpRc->top, 
            PATCOPY);
         r.left+=IconWidth;
      }
      else 
      {
         NtGdiPatBlt(hMemDc, 0, 0, 
            lpRc->right - lpRc->left, 
            lpRc->bottom - lpRc->top, 
            PATCOPY);
      }
         
      if(uFlags & DC_GRADIENT)
      {
         static GRADIENT_RECT gcap = {0, 1};
         TRIVERTEX vert[2];
         COLORREF Colors[2];
         PDC pMemDc;

		 if (pWnd != NULL)
		 {
			 if(pWnd->Style & WS_SYSMENU)
			 {
				r.right -= 3 + ButtonWidth;
				if(!(uFlags & DC_SMALLCAP))
				{
				   if(pWnd->Style & (WS_MAXIMIZEBOX | WS_MINIMIZEBOX))
					  r.right -= 2 + 2 * ButtonWidth;
				   else r.right -= 2;
				   r.right -= 2;
				}
	            
				//Draw buttons background
				if(!NtGdiSelectObject(hMemDc, 
				   IntGetSysColorBrush(uFlags & DC_ACTIVE ? 
					  COLOR_GRADIENTACTIVECAPTION:COLOR_GRADIENTINACTIVECAPTION)))
				{
				   DPRINT1("%s: NtGdiSelectObject() failed!\n", __FUNCTION__);
				   goto cleanup;
				}

				NtGdiPatBlt(hMemDc, 
				   r.right, 
				   0, 
				   lpRc->right - lpRc->left - r.right, 
				   lpRc->bottom - lpRc->top, 
				   PATCOPY);
			 }
		 }

         Colors[0] = IntGetSysColor((uFlags & DC_ACTIVE) ? 
            COLOR_ACTIVECAPTION : COLOR_INACTIVECAPTION);

         Colors[1] = IntGetSysColor((uFlags & DC_ACTIVE) ? 
            COLOR_GRADIENTACTIVECAPTION : COLOR_GRADIENTINACTIVECAPTION);

         vert[0].x = r.left;
         vert[0].y = 0;
         vert[0].Red = (WORD)Colors[0]<<8;
         vert[0].Green = (WORD)Colors[0] & 0xFF00;
         vert[0].Blue = (WORD)(Colors[0]>>8) & 0xFF00;
         vert[0].Alpha = 0;

         vert[1].x = r.right;
         vert[1].y = lpRc->bottom - lpRc->top;
         vert[1].Red = (WORD)Colors[1]<<8;
         vert[1].Green = (WORD)Colors[1] & 0xFF00;
         vert[1].Blue = (WORD)(Colors[1]>>8) & 0xFF00;
         vert[1].Alpha = 0;
            
         pMemDc = DC_LockDc(hMemDc);
         if(!pMemDc)
         {
            DPRINT1("%s: Can't lock dc!\n", __FUNCTION__);
            goto cleanup;
         }

         if(!IntGdiGradientFill(pMemDc, vert, 2, &gcap, 
            1, GRADIENT_FILL_RECT_H))
         {
            DPRINT1("%s: IntGdiGradientFill() failed!\n", __FUNCTION__);
         }

         DC_UnlockDc(pMemDc);
      } //if(uFlags & DC_GRADIENT)
   }
   
   if(HasIcon)
   {
      r.top ++;
      r.left -= --IconWidth;
	  /* FIXME: Draw the Icon when pWnd == NULL but  hIcon is valid */
	  if (pWnd != NULL)
		UserDrawSysMenuButton(pWnd, hMemDc, &r, FALSE);
      r.left += IconWidth;
      r.top --;
   }

   r.top ++;
   r.left += 2;

   r.bottom = r.top + Height;

   if((uFlags & DC_TEXT))
   {
      if(!(uFlags & DC_GRADIENT))
      {
         r.right = (lpRc->right - lpRc->left);

         if(uFlags & DC_SMALLCAP) 
            ButtonWidth = UserGetSystemMetrics(SM_CXSMSIZE) - 2;
         else ButtonWidth = UserGetSystemMetrics(SM_CXSIZE) - 2;

         if(pWnd->Style & WS_SYSMENU)
         {
            r.right -= 3 + ButtonWidth;
            if(! (uFlags & DC_SMALLCAP))
            {
               if(pWnd->Style & (WS_MAXIMIZEBOX | WS_MINIMIZEBOX))
                  r.right -= 2 + 2 * ButtonWidth;
               else r.right -= 2;
               r.right -= 2;
            }
         }
      }

	  /* FIXME: hFont isn't handled */
      if (str)
         UserDrawCaptionText(hMemDc, str, &r, uFlags);
	  else if (pWnd != NULL)
	     UserDrawCaptionText(hMemDc, &pWnd->WindowName, &r, uFlags);
   } 

   if(!NtGdiBitBlt(hDc, lpRc->left, lpRc->top, 
      lpRc->right - lpRc->left, lpRc->bottom - lpRc->top,
      hMemDc, 0, 0, SRCCOPY, 0, 0)) 
   {
      DPRINT1("%s: NtGdiBitBlt() failed!\n", __FUNCTION__);
      goto cleanup;
   }

   Ret = TRUE;

cleanup:
   if (hOldBrush) NtGdiSelectObject(hMemDc, hOldBrush);
   if (hOldBmp) NtGdiSelectObject(hMemDc, hOldBmp);
   if (hMemBmp) NtGdiDeleteObject(hMemBmp);
   if (hMemDc) NtGdiDeleteObjectApp(hMemDc);

   return Ret;
}


BOOL
STDCALL
NtUserDrawCaptionTemp(
   HWND hWnd,
   HDC hDC,
   LPCRECT lpRc,
   HFONT hFont,
   HICON hIcon,
   const PUNICODE_STRING str,
   UINT uFlags)
{
   PWINDOW_OBJECT pWnd = NULL;
   RECT SafeRect;
   UNICODE_STRING SafeStr = {0};
   BOOL Ret = FALSE;
   
   UserEnterExclusive();
   
   if (hWnd != NULL)
   {
     if(!(pWnd = UserGetWindowObject(hWnd)))
     {
        UserLeave();
        return FALSE;
     }
   }

   _SEH_TRY
   {
      ProbeForRead(lpRc, sizeof(RECT), sizeof(ULONG));
      RtlCopyMemory(&SafeRect, lpRc, sizeof(RECT));
      if (str != NULL)
	  {
        SafeStr = ProbeForReadUnicodeString(str);
	    if (SafeStr.Length != 0)
        {
          ProbeForRead(SafeStr.Buffer,
               SafeStr.Length,
               sizeof(WCHAR));
        }
		Ret = UserDrawCaption(pWnd, hDC, &SafeRect, hFont, hIcon, &SafeStr, uFlags);
      }
	  else
	    Ret = UserDrawCaption(pWnd, hDC, &SafeRect, hFont, hIcon, NULL, uFlags);
   }
   _SEH_HANDLE
   {
      SetLastNtError(_SEH_GetExceptionCode());
   }
   _SEH_END;
   
   UserLeave();
   return Ret;
}

BOOL 
STDCALL 
NtUserDrawCaption(HWND hWnd,
   HDC hDC,
   LPCRECT lpRc,
   UINT uFlags)
{
	return NtUserDrawCaptionTemp(hWnd, hDC, lpRc, 0, 0, NULL, uFlags);
}

/* EOF */
