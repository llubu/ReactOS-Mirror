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
 *  $Id: painting.c,v 1.56 2003/12/30 16:55:00 weiden Exp $
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

#include <ddk/ntddk.h>
#include <internal/safe.h>
#include <win32k/win32k.h>
#include <include/object.h>
#include <include/guicheck.h>
#include <include/window.h>
#include <include/desktop.h>
#include <include/winpos.h>
#include <include/class.h>
#include <include/caret.h>
#include <include/error.h>
#include <include/winsta.h>
#include <windows.h>
#include <include/painting.h>
#include <user32/wininternal.h>
#include <include/rect.h>
#include <win32k/coord.h>
#include <win32k/region.h>
#include <include/vis.h>
#include <include/intgdi.h>

#define NDEBUG
#include <debug.h>

/* #define FIN_DEBUG */

/* PRIVATE FUNCTIONS **********************************************************/

VOID FASTCALL
IntValidateParent(PWINDOW_OBJECT Child, HRGN ValidRegion)
{
   HWND Parent;
   PWINDOW_OBJECT ParentWindow;

   Parent = NtUserGetAncestor(Child->Self, GA_PARENT);
   while (Parent && Parent != IntGetDesktopWindow())
   {
      ParentWindow = IntGetWindowObject(Parent);
      if (ParentWindow && !(ParentWindow->Style & WS_CLIPCHILDREN))
      {
         if (ParentWindow->UpdateRegion != 0)
         {
            INT OffsetX, OffsetY;

            /*
             * We must offset the child region by the offset of the
             * child rect in the parent.
             */
            OffsetX = Child->WindowRect.left - ParentWindow->WindowRect.left;
            OffsetY = Child->WindowRect.top - ParentWindow->WindowRect.top;
            NtGdiOffsetRgn(ValidRegion, OffsetX, OffsetY );
            NtGdiCombineRgn(ParentWindow->UpdateRegion, ParentWindow->UpdateRegion,
               ValidRegion, RGN_DIFF);
            /* FIXME: If the resulting region is empty, remove fake posted paint message */
            NtGdiOffsetRgn(ValidRegion, -OffsetX, -OffsetY);
         }
      }
      IntReleaseWindowObject(ParentWindow);
      Parent = NtUserGetAncestor(Parent, GA_PARENT);
   }
}

/*
 * IntGetNCUpdateRegion
 *
 * Get nonclient part of window update region. 
 * 
 * Return Value
 *    Handle to region that represents invalid nonclient window area. The
 *    caller is responsible for deleting it.
 *
 * Remarks
 *    This function also marks the nonclient update region of window
 *    as valid, clears the WINDOWOBJECT_NEED_NCPAINT flag.
 */

HRGN FASTCALL
IntGetNCUpdateRegion(PWINDOW_OBJECT Window)
{
   HRGN WindowRgn;
   HRGN NonclientRgn;

   /*
    * Generate the update region.
    */

   WindowRgn = UnsafeIntCreateRectRgnIndirect(&Window->ClientRect);
   NtGdiOffsetRgn(WindowRgn, 
      -Window->WindowRect.left,
      -Window->WindowRect.top);
   NonclientRgn = NtGdiCreateRectRgn(0, 0, 0, 0);
   if (NtGdiCombineRgn(NonclientRgn, Window->UpdateRegion,
       WindowRgn, RGN_DIFF) == NULLREGION)
   {
      NtGdiDeleteObject(NonclientRgn);
      NonclientRgn = NULL;
   }

   /*
    * Remove the nonclient region from the standard update region.
    */

   if (NtGdiCombineRgn(Window->UpdateRegion, Window->UpdateRegion,
       WindowRgn, RGN_AND) == NULLREGION)
   {
      NtGdiDeleteObject(Window->UpdateRegion);
      Window->UpdateRegion = NULL;
   }

   return NonclientRgn;
}

/*
 * IntPaintWindows
 *
 * Internal function used by IntRedrawWindow.
 */

VOID FASTCALL
IntPaintWindows(PWINDOW_OBJECT Window, ULONG Flags)
{
  HDC hDC;
  HWND hWnd = Window->Self;

  if (! (Window->Style & WS_VISIBLE))
    {
      return;
    }

  if (Flags & (RDW_ERASENOW | RDW_UPDATENOW))
    {
      if (Window->Flags & WINDOWOBJECT_NEED_NCPAINT)
        {
          IntSendMessage(hWnd, WM_NCPAINT, (WPARAM)Window->NCUpdateRegion, 0);
          Window->NCUpdateRegion = NULL;
          Window->Flags &= ~WINDOWOBJECT_NEED_NCPAINT;
          MsqDecPaintCountQueue(Window->MessageQueue);
        }

      if (Window->Flags & WINDOWOBJECT_NEED_ERASEBKGND)
        {
          if (Window->UpdateRegion)
            {
#ifdef FIN_DEBUG
              {
                 RECT TempRect;
                 UnsafeIntGetRgnBox(Window->UpdateRegion, &TempRect);
                 DPRINT1("Sending WM_ERASEBKGND[1]: %d,%d-%d,%d\n",
                    TempRect.left, TempRect.top, TempRect.right, TempRect.bottom);
              }
#endif
              hDC = NtUserGetDCEx(hWnd, 0, DCX_CACHE | DCX_USESTYLE |
                                           DCX_INTERSECTUPDATE);
              if (hDC != NULL)
                {
                  if (IntSendMessage(hWnd, WM_ERASEBKGND, (WPARAM)hDC, 0))
                    {
                      Window->Flags &= ~WINDOWOBJECT_NEED_ERASEBKGND;
                    }
                  NtUserReleaseDC(hWnd, hDC);
                }
            }
        }

      if (Flags & RDW_UPDATENOW)
        {
          if (Window->UpdateRegion != NULL ||
              Window->Flags & WINDOWOBJECT_NEED_INTERNALPAINT)
            {
#ifdef FIN_DEBUG
              {
                 RECT TempRect;
                 UnsafeIntGetRgnBox(Window->UpdateRegion, &TempRect);
                 DPRINT1("Sending WM_PAINT[1]: %d,%d-%d,%d\n",
                    TempRect.left, TempRect.top, TempRect.right, TempRect.bottom);
              }
#endif
              IntSendMessage(hWnd, WM_PAINT, 0, 0);
              if (Window->Flags & WINDOWOBJECT_NEED_INTERNALPAINT)
                {
                  Window->Flags &= ~WINDOWOBJECT_NEED_INTERNALPAINT;
                  if (Window->UpdateRegion == NULL)
                    {
                      MsqDecPaintCountQueue(Window->MessageQueue);
                    }
                }
            }
        }
    }

  /*
   * Check that the window is still valid at this point
   */

  if (! IntIsWindow(hWnd))
    {
      return;
    }

  /*
   * Paint child windows.
   */
  if (!(Flags & RDW_NOCHILDREN) && !(Window->Style & WS_MINIMIZE) &&
      ((Flags & RDW_ALLCHILDREN) || !(Window->Style & WS_CLIPCHILDREN)) &&
      ! IntIsDesktopWindow(Window))
    {
      HWND *List, *phWnd;

      if ((List = IntWinListChildren(Window)))
        {
          for (phWnd = List; *phWnd; ++phWnd)
            {
              Window = IntGetWindowObject(*phWnd);
              if (Window)
                {
                  IntPaintWindows(Window, Flags);
                  IntReleaseWindowObject(Window);
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
IntInvalidateWindows(PWINDOW_OBJECT Window, HRGN hRgn, ULONG Flags,
   BOOL ValidateParent)
{
   INT RgnType;
   BOOL HadPaintMessage, HadNCPaintMessage;
   BOOL HasPaintMessage, HasNCPaintMessage;
   HRGN hRgnWindow;

   /*
    * Clip the given region with window rectangle (or region)
    */

#ifdef TODO
   if (!Window->WindowRegion)
#endif
   {
      hRgnWindow = UnsafeIntCreateRectRgnIndirect(&Window->WindowRect);
      NtGdiOffsetRgn(hRgnWindow,
         -Window->WindowRect.left,
         -Window->WindowRect.top);
      RgnType = NtGdiCombineRgn(hRgn, hRgn, hRgnWindow, RGN_AND);        
      NtGdiDeleteObject(hRgnWindow);
   }
#ifdef TODO
   else
   {
      RgnType = NtGdiCombineRgn(hRgn, hRgn, Window->WindowRegion, RGN_AND);        
   }
#endif

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
      }

      if (NtGdiCombineRgn(Window->UpdateRegion, Window->UpdateRegion,
          hRgn, RGN_OR) == NULLREGION)
      {
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
    * Validate parent covered by region
    */

   if (ValidateParent)
   {
      IntValidateParent(Window, Window->UpdateRegion);
   }

   /*
    * Split the nonclient update region.
    */

   if (Window->NCUpdateRegion == NULL)
   {
      Window->NCUpdateRegion = IntGetNCUpdateRegion(Window);
   }
   else
   {
      HRGN hRgnNonClient = IntGetNCUpdateRegion(Window);
      NtGdiCombineRgn(Window->NCUpdateRegion, Window->NCUpdateRegion,
         hRgnNonClient, RGN_OR);
      NtGdiDeleteObject(hRgnNonClient);
   }

   /*
    * Process children if needed
    */

   if (!(Flags & RDW_NOCHILDREN) && !(Window->Style & WS_MINIMIZE) &&
       ((Flags & RDW_ALLCHILDREN) || !(Window->Style & WS_CLIPCHILDREN)))
   {
      HWND *List, *phWnd;
      PWINDOW_OBJECT Child;

      if ((List = IntWinListChildren(Window)))
      {
         for (phWnd = List; *phWnd; ++phWnd)
         {
            Child = IntGetWindowObject(*phWnd);
            if(!Child)
            {
              continue;
            }
            if (Child->Style & WS_VISIBLE)
            {
               RECT TempRect;

               /*
                * Recursive call to update children UpdateRegion
                */
               HRGN hRgnTemp = NtGdiCreateRectRgn(0, 0, 0, 0);
               NtGdiCombineRgn(hRgnTemp, hRgn, 0, RGN_COPY);
               NtGdiOffsetRgn(hRgnTemp,
                  Window->WindowRect.left - Child->WindowRect.left,
                  Window->WindowRect.top - Child->WindowRect.top);
               IntInvalidateWindows(Child, hRgnTemp, Flags, FALSE);

               /*
                * Update our UpdateRegion depending on children
                */

               if (Window->UpdateRegion != NULL)
               {
                  UnsafeIntGetRgnBox(Window->UpdateRegion, &TempRect);
                  NtGdiCombineRgn(hRgnTemp, Child->UpdateRegion, 0, RGN_COPY);
                  NtGdiCombineRgn(hRgnTemp, hRgnTemp, Child->NCUpdateRegion, RGN_OR);
                  NtGdiOffsetRgn(hRgnTemp,
                     Child->WindowRect.left - Window->WindowRect.left,
                     Child->WindowRect.top - Window->WindowRect.top);
                  UnsafeIntGetRgnBox(hRgnTemp, &TempRect);
                  if (NtGdiCombineRgn(Window->UpdateRegion, Window->UpdateRegion,
                      hRgnTemp, RGN_DIFF) == NULLREGION)
                  {
                     NtGdiDeleteObject(Window->UpdateRegion);
                     Window->UpdateRegion = NULL;
                  }
               }
               NtGdiDeleteObject(hRgnTemp);
            }
            IntReleaseWindowObject(Child);
         }
         ExFreePool(List);
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
   PWINDOW_OBJECT Wnd = Window;
   
   for (; Wnd; Wnd = Wnd->Parent)
   {
      if (!(Wnd->Style & WS_VISIBLE) ||
          ((Wnd->Style & WS_MINIMIZE) && (Wnd != Window)))
         return FALSE;
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
IntRedrawWindow(PWINDOW_OBJECT Window, const RECT* UpdateRect, HRGN UpdateRgn,
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
    * a region hRgn specified in window coordinates.
    */

   if (Flags & (RDW_INVALIDATE | RDW_VALIDATE))
   {
      if (UpdateRgn != NULL)
      {
         hRgn = NtGdiCreateRectRgn(0, 0, 0, 0);
         NtGdiCombineRgn(hRgn, UpdateRgn, NULL, RGN_COPY);
         NtGdiOffsetRgn(hRgn, 
            Window->ClientRect.left - Window->WindowRect.left,
            Window->ClientRect.top - Window->WindowRect.top);
      } else
      if (UpdateRect != NULL)
      {
         hRgn = UnsafeIntCreateRectRgnIndirect((RECT *)UpdateRect);
         NtGdiOffsetRgn(hRgn, 
            Window->ClientRect.left - Window->WindowRect.left,
            Window->ClientRect.top - Window->WindowRect.top);
      } else
      if ((Flags & (RDW_INVALIDATE | RDW_FRAME)) == (RDW_INVALIDATE | RDW_FRAME) ||
          (Flags & (RDW_VALIDATE | RDW_NOFRAME)) == (RDW_VALIDATE | RDW_NOFRAME))
      {
         hRgn = UnsafeIntCreateRectRgnIndirect(&Window->WindowRect);
         NtGdiOffsetRgn(hRgn, 
            -Window->WindowRect.left,
            -Window->WindowRect.top);
      }
      else
      {
         hRgn = UnsafeIntCreateRectRgnIndirect(&Window->ClientRect);
         NtGdiOffsetRgn(hRgn, 
            -Window->WindowRect.left,
            -Window->WindowRect.top);
      }
   }

   /*
    * Step 3.
    * Adjust the window update region depending on hRgn and flags.
    */

   if (Flags & (RDW_INVALIDATE | RDW_VALIDATE | RDW_INTERNALPAINT | RDW_NOINTERNALPAINT))
   {
      IntInvalidateWindows(Window, hRgn, Flags, TRUE);
   } else
   if (Window->UpdateRegion != NULL && Flags & RDW_ERASENOW)
   {
      /* Validate parent covered by region. */
      IntValidateParent(Window, Window->UpdateRegion);
   }

   /*
    * Step 4.
    * Repaint and erase windows if needed.
    */

   if (Flags & (RDW_ERASENOW | RDW_UPDATENOW))
   {
      IntPaintWindows(Window, Flags);
   }

   /*
    * Step 5.
    * Cleanup ;-)
    */

   if (NULL != hRgn)
   {
      NtGdiDeleteObject(hRgn);
   }

   return TRUE;
}

HWND STDCALL
IntFindWindowToRepaint(HWND hWnd, PW32THREAD Thread)
{
   PWINDOW_OBJECT Window;
   PWINDOW_OBJECT Child;
   HWND hFoundWnd = NULL;

   if (hWnd == NULL)
   {
      PLIST_ENTRY CurrentEntry;

      ExAcquireFastMutex(&Thread->WindowListLock);

      for (CurrentEntry = Thread->WindowListHead.Flink;
           CurrentEntry != &Thread->WindowListHead;
           CurrentEntry = CurrentEntry->Flink)
      {
         Window = CONTAINING_RECORD(CurrentEntry, WINDOW_OBJECT, ThreadListEntry);
         if (Window->Parent != NULL && !IntIsDesktopWindow(Window->Parent))
         {
            continue;
         }
         if (Window->Style & WS_VISIBLE)
         {
            hFoundWnd = IntFindWindowToRepaint(Window->Self, Thread);
            if (hFoundWnd != NULL)
            {
                ExReleaseFastMutex(&Thread->WindowListLock);
                return hFoundWnd;
            }
         }
      }

      ExReleaseFastMutex(&Thread->WindowListLock);
      return NULL;
   }
   else
   {
      Window = IntGetWindowObject(hWnd);
      if (Window == NULL)
         return NULL;

      if ((Window->UpdateRegion != NULL ||
          Window->Flags & (WINDOWOBJECT_NEED_INTERNALPAINT | WINDOWOBJECT_NEED_NCPAINT)) && 
          IntWndBelongsToThread(Window, Thread))
      {
         IntReleaseWindowObject(Window);
         return hWnd;
      }

      ExAcquireFastMutex(&Window->ChildrenListLock);

      for (Child = Window->FirstChild; Child; Child = Child->NextSibling)
      {
         if (Child->Style & WS_VISIBLE &&
             (Child->UpdateRegion != NULL ||
              Child->Flags & WINDOWOBJECT_NEED_INTERNALPAINT ||
              Child->Flags & WINDOWOBJECT_NEED_NCPAINT))
         {
            hFoundWnd = Child->Self;
            break;
         }
      }

      if (hFoundWnd == NULL)
      {
         for (Child = Window->FirstChild; Child; Child = Child->NextSibling)
         {
            if (Child->Style & WS_VISIBLE)
            {
               hFoundWnd = IntFindWindowToRepaint(Child->Self, Thread);
               if (hFoundWnd != NULL)
                  break;
            }
         }
      }

      ExReleaseFastMutex(&Window->ChildrenListLock);
      IntReleaseWindowObject(Window);

      return hFoundWnd;
   }
}

BOOL FASTCALL
IntGetPaintMessage(HWND hWnd, PW32THREAD Thread, MSG *Message,
   BOOL Remove)
{
   PWINDOW_OBJECT Window;
   PUSER_MESSAGE_QUEUE MessageQueue = (PUSER_MESSAGE_QUEUE)Thread->MessageQueue;

   if (!MessageQueue->PaintPosted)
      return FALSE;

   if (hWnd)
      Message->hwnd = IntFindWindowToRepaint(hWnd, PsGetWin32Thread());
   else
      Message->hwnd = IntFindWindowToRepaint(NULL, PsGetWin32Thread());

   if (Message->hwnd == NULL)
   {
#if 0
      DPRINT1("PAINTING BUG: Thread marked as containing dirty windows, but no dirty windows found!\n");
      /* FIXME: Lock the queue! */
      MessageQueue->PaintPosted = 0;
      MessageQueue->PaintCount = 0;
#endif
      return FALSE;
   }

   Window = IntGetWindowObject(Message->hwnd);
   if (Window != NULL)
   {
      if (Window->Flags & WINDOWOBJECT_NEED_NCPAINT)
      {
         Message->message = WM_NCPAINT;
         Message->wParam = (WPARAM)Window->NCUpdateRegion;
         Message->lParam = 0;
         if (Remove)
         {
            Window->NCUpdateRegion = NULL;
            Window->Flags &= ~WINDOWOBJECT_NEED_NCPAINT;
            MsqDecPaintCountQueue(Window->MessageQueue);
         }
      } else
      {
#ifdef FIN_DEBUG
         {
            RECT TempRect;
            UnsafeIntGetRgnBox(Window->UpdateRegion, &TempRect);
            DPRINT1("Sending WM_PAINT[2]: %d,%d-%d,%d\n",
               TempRect.left, TempRect.top, TempRect.right, TempRect.bottom);
         }
#endif
         Message->message = WM_PAINT;
         Message->wParam = Message->lParam = 0;
         if (Remove && Window->Flags & WINDOWOBJECT_NEED_INTERNALPAINT)
         {
            Window->Flags &= ~WINDOWOBJECT_NEED_INTERNALPAINT;
            if (Window->UpdateRegion == NULL)
            {
               MsqDecPaintCountQueue(Window->MessageQueue);
            }
         }
      }

      IntReleaseWindowObject(Window);
      return TRUE;
   }

   return FALSE;
}

HWND FASTCALL
IntFixCaret(HWND hWnd, LPRECT lprc, UINT flags)
{
   GUITHREADINFO info;

   if (!NtUserGetGUIThreadInfo(0, &info))
      return 0;
   if (!info.hwndCaret)
      return 0;
   if (info.hwndCaret == hWnd ||
       ((flags & SW_SCROLLCHILDREN) && IntIsChildWindow(hWnd, info.hwndCaret)))
   {
      POINT pt, FromOffset, ToOffset, Offset;
      
      pt.x = info.rcCaret.left;
      pt.y = info.rcCaret.top;

      IntGetClientOrigin(info.hwndCaret, &FromOffset);
      IntGetClientOrigin(hWnd, &ToOffset);
      Offset.x = FromOffset.x - ToOffset.x;
      Offset.y = FromOffset.y - ToOffset.y;
      info.rcCaret.left += Offset.x;
      info.rcCaret.top += Offset.y;
      info.rcCaret.right += Offset.x;
      info.rcCaret.bottom += Offset.y;
      if (NtGdiIntersectRect(lprc, lprc, &info.rcCaret))
      {
         NtUserHideCaret(0);
         lprc->left = pt.x;
         lprc->top = pt.y;
         return info.hwndCaret;
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
NtUserBeginPaint(HWND hWnd, PAINTSTRUCT* lPs)
{
   PWINDOW_OBJECT Window;
   RECT ClientRect;
   RECT ClipRect;
   INT DcxFlags;

   if (!(Window = IntGetWindowObject(hWnd)))
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      return NULL;
   }

   NtUserHideCaret(hWnd);

   DcxFlags = DCX_INTERSECTUPDATE | DCX_WINDOWPAINT | DCX_USESTYLE;
   
   lPs->hdc = NtUserGetDCEx(hWnd, 0, DcxFlags);

   if (!lPs->hdc)
   {
      IntReleaseWindowObject(Window);
      return NULL;
   }

/*   IntRedrawWindow(Window, NULL, 0, RDW_NOINTERNALPAINT | RDW_VALIDATE | RDW_NOCHILDREN);*/
#ifdef FIN_DEBUG
   if (Window->Flags & WINDOWOBJECT_NEED_ERASEBKGND)
   {
      RECT TempRect;
      UnsafeIntGetRgnBox(Window->UpdateRegion, &TempRect);
      DPRINT1("Sending WM_ERASEBKGND[2]: %d,%d-%d,%d\n",
         TempRect.left, TempRect.top, TempRect.right, TempRect.bottom);
   }
#endif
   
   if (Window->UpdateRegion != NULL)
   {
      MsqDecPaintCountQueue(Window->MessageQueue);
      IntValidateParent(Window, Window->UpdateRegion);
      NtGdiDeleteObject(Window->UpdateRegion);
      Window->UpdateRegion = NULL;
   }

   IntGetClientRect(Window, &ClientRect);
   IntGdiGetClipBox(lPs->hdc, &ClipRect);
   NtGdiLPtoDP(lPs->hdc, (LPPOINT)&ClipRect, 2);
   NtGdiIntersectRect(&lPs->rcPaint, &ClientRect, &ClipRect);
   NtGdiDPtoLP(lPs->hdc, (LPPOINT)&lPs->rcPaint, 2);

   if (Window->Flags & WINDOWOBJECT_NEED_ERASEBKGND)
   {
      Window->Flags &= ~WINDOWOBJECT_NEED_ERASEBKGND;
      lPs->fErase = !IntSendMessage(hWnd, WM_ERASEBKGND, (WPARAM)lPs->hdc, 0);
   }
   else
   {
      lPs->fErase = FALSE;
   }

   IntReleaseWindowObject(Window);

   return lPs->hdc;
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
   NtUserReleaseDC(hWnd, lPs->hdc);
   NtUserShowCaret(hWnd);

   return TRUE;
}

/*
 * NtUserInvalidateRect
 *
 * Status
 *    @implemented
 */

DWORD STDCALL
NtUserInvalidateRect(HWND hWnd, CONST RECT *Rect, BOOL Erase)
{
   return NtUserRedrawWindow(hWnd, Rect, 0, RDW_INVALIDATE | (Erase ? RDW_ERASE : 0));
}

/*
 * NtUserInvalidateRgn
 *
 * Status
 *    @implemented
 */

DWORD STDCALL
NtUserInvalidateRgn(HWND hWnd, HRGN Rgn, BOOL Erase)
{
   return NtUserRedrawWindow(hWnd, NULL, Rgn, RDW_INVALIDATE | (Erase ? RDW_ERASE : 0));
}

/*
 * NtUserValidateRgn
 *
 * Status
 *    @implemented
 */

BOOL STDCALL
NtUserValidateRgn(HWND hWnd, HRGN hRgn)
{
   return NtUserRedrawWindow(hWnd, NULL, hRgn, RDW_VALIDATE | RDW_NOCHILDREN);
}

/*
 * NtUserUpdateWindow
 *
 * Status
 *    @implemented
 */
 
BOOL STDCALL
NtUserUpdateWindow(HWND hWnd)
{
   return NtUserRedrawWindow(hWnd, NULL, 0, RDW_UPDATENOW | RDW_ALLCHILDREN);
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
   PWINDOW_OBJECT Window;
   int RegionType;

   if (!(Window = IntGetWindowObject(hWnd)))
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      return ERROR;
   }
    
   if (Window->UpdateRegion == NULL)
   {
      RegionType = (NtGdiSetRectRgn(hRgn, 0, 0, 0, 0) ? NULLREGION : ERROR);
   }
   else
   {
      RegionType = NtGdiCombineRgn(hRgn, Window->UpdateRegion, hRgn, RGN_COPY);
      NtGdiOffsetRgn(
         hRgn,
         Window->WindowRect.left - Window->ClientRect.left,
         Window->WindowRect.top - Window->ClientRect.top);
   }

   IntReleaseWindowObject(Window);

   if (bErase && RegionType != NULLREGION && RegionType != ERROR)
   {
      NtUserRedrawWindow(hWnd, NULL, NULL, RDW_ERASENOW | RDW_NOCHILDREN);
   }
  
   return RegionType;
}

/*
 * NtUserGetUpdateRect
 *
 * Status
 *    @implemented
 */

BOOL STDCALL
NtUserGetUpdateRect(HWND hWnd, LPRECT lpRect, BOOL fErase)
{
   HRGN hRgn = NtGdiCreateRectRgn(0, 0, 0, 0);

   if (!lpRect)
   {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return FALSE;
   }

   NtUserGetUpdateRgn(hWnd, hRgn, fErase);
   NtGdiGetRgnBox(hRgn, lpRect);

   return lpRect->left < lpRect->right && lpRect->top < lpRect->bottom;
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

   if (!(Wnd = IntGetWindowObject(hWnd ? hWnd : IntGetDesktopWindow())))
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      return FALSE;
   }

   if (lprcUpdate != NULL)
   {
      Status = MmCopyFromCaller(&SafeUpdateRect, (PRECT)lprcUpdate,
         sizeof(RECT));

      if (!NT_SUCCESS(Status))
      {
         SetLastWin32Error(ERROR_INVALID_PARAMETER);
         return FALSE;
      }
   }

   Status = IntRedrawWindow(Wnd, NULL == lprcUpdate ? NULL : &SafeUpdateRect,
      hrgnUpdate, flags);

   if (!NT_SUCCESS(Status))
   {
      /* IntRedrawWindow fails only in case that flags are invalid */
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return FALSE;
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
   RECT rSrc, rClipped_src, rClip, rDst, offset;

   /*
    * Compute device clipping region (in device coordinates).
    */

   if (lprcScroll)
      rSrc = *lprcScroll;
   else
      IntGdiGetClipBox(hDC, &rSrc);
   NtGdiLPtoDP(hDC, (LPPOINT)&rSrc, 2);

   if (lprcClip)
      rClip = *lprcClip;
   else
      IntGdiGetClipBox(hDC, &rClip);
   NtGdiLPtoDP(hDC, (LPPOINT)&rClip, 2);

   NtGdiIntersectRect(&rClipped_src, &rSrc, &rClip);

   rDst = rClipped_src;
   NtGdiSetRect(&offset, 0, 0, dx, dy);
   NtGdiLPtoDP(hDC, (LPPOINT)&offset, 2);
   NtGdiOffsetRect(&rDst, offset.right - offset.left,  offset.bottom - offset.top);
   NtGdiIntersectRect(&rDst, &rDst, &rClip);

   /*
    * Copy bits, if possible.
    */

   if (rDst.bottom > rDst.top && rDst.right > rDst.left)
   {
      RECT rDst_lp = rDst, rSrc_lp = rDst;

      NtGdiOffsetRect(&rSrc_lp, offset.left - offset.right, offset.top - offset.bottom);
      NtGdiDPtoLP(hDC, (LPPOINT)&rDst_lp, 2);
      NtGdiDPtoLP(hDC, (LPPOINT)&rSrc_lp, 2);

      if (!NtGdiBitBlt(hDC, rDst_lp.left, rDst_lp.top, rDst_lp.right - rDst_lp.left,
                       rDst_lp.bottom - rDst_lp.top, hDC, rSrc_lp.left, rSrc_lp.top,
                       SRCCOPY))
         return FALSE;
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

      hRgn2 = NtGdiCreateRectRgnIndirect(&rSrc);
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
 * NtUserScrollWindowEx
 *
 * Status
 *    @implemented
 */

DWORD STDCALL
NtUserScrollWindowEx(HWND hWnd, INT dx, INT dy, const RECT *rect,
   const RECT *clipRect, HRGN hrgnUpdate, LPRECT rcUpdate, UINT flags)
{
   RECT rc, cliprc, caretrc;
   INT Result;
   PWINDOW_OBJECT Window;
   HDC hDC;
   HRGN hrgnTemp;
   HWND hwndCaret;
   BOOL bUpdate = (rcUpdate || hrgnUpdate || flags & (SW_INVALIDATE | SW_ERASE));
   BOOL bOwnRgn = TRUE;

   Window = IntGetWindowObject(hWnd);
   if (!Window || !IntIsWindowDrawable(Window))
   {
      IntReleaseWindowObject(Window);
      return ERROR;
   }

   IntGetClientRect(Window, &rc);
   if (rect)
      NtGdiIntersectRect(&rc, &rc, rect);

   if (clipRect)
      NtGdiIntersectRect(&cliprc, &rc, clipRect);
   else
      cliprc = rc;

   if (cliprc.right <= cliprc.left || cliprc.bottom <= cliprc.top ||
       (dx == 0 && dy == 0))
   { 
      return NULLREGION;
   }

   caretrc = rc;
   hwndCaret = IntFixCaret(hWnd, &caretrc, flags);

   if (hrgnUpdate)
      bOwnRgn = FALSE;
   else if (bUpdate)
      hrgnUpdate = NtGdiCreateRectRgn(0, 0, 0, 0);

   hDC = NtUserGetDCEx( hWnd, 0, DCX_CACHE | DCX_USESTYLE );
   if (hDC)
   {
      HRGN hRgn = NtGdiCreateRectRgn( 0, 0, 0, 0 );

      NtUserScrollDC(hDC, dx, dy, &rc, &cliprc, hrgnUpdate, rcUpdate);
      NtUserReleaseDC(hWnd, hDC);
      if (bUpdate)
         NtGdiCombineRgn(hrgnUpdate, hrgnUpdate, hRgn, RGN_OR);
      else
         NtUserRedrawWindow(hWnd, NULL, hRgn, RDW_INVALIDATE | RDW_ERASE);
      NtGdiDeleteObject(hRgn);
   }

   /* 
    * Take into account the fact that some damage may have occurred during
    * the scroll.
    */

   hrgnTemp = NtGdiCreateRectRgn(0, 0, 0, 0);
   Result = NtUserGetUpdateRgn(hWnd, hrgnTemp, FALSE);
   if (Result != NULLREGION)
   {
      HRGN hrgnClip = NtGdiCreateRectRgnIndirect(&cliprc);
      NtGdiOffsetRgn(hrgnTemp, dx, dy);
      NtGdiCombineRgn(hrgnTemp, hrgnTemp, hrgnClip, RGN_AND);
      NtUserRedrawWindow(hWnd, NULL, hrgnTemp, RDW_INVALIDATE | RDW_ERASE);
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

         for (i = 0; List[i]; i++)
         {
            NtUserGetWindowRect(List[i], &r);
            IntGetClientOrigin(hWnd, &ClientOrigin);
            r.left -= ClientOrigin.x;
            r.top -= ClientOrigin.y;
            r.right -= ClientOrigin.x;
            r.bottom -= ClientOrigin.y;
            if (!rect || NtGdiIntersectRect(&dummy, &r, &rc))
               WinPosSetWindowPos(List[i], 0, r.left + dx, r.top + dy, 0, 0,
                                  SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE |
                                  SWP_NOREDRAW);
         }
         ExFreePool(List);
      }
   }

   if (flags & (SW_INVALIDATE | SW_ERASE))
      NtUserRedrawWindow(hWnd, NULL, hrgnUpdate, RDW_INVALIDATE | RDW_ERASE |
                         ((flags & SW_ERASE) ? RDW_ERASENOW : 0) |
                         ((flags & SW_SCROLLCHILDREN) ? RDW_ALLCHILDREN : 0));

   if (bOwnRgn && hrgnUpdate)
      NtGdiDeleteObject(hrgnUpdate);
   
   if (hwndCaret)
   {
      IntSetCaretPos(caretrc.left + dx, caretrc.top + dy);
      NtUserShowCaret(hwndCaret);
   }

   IntReleaseWindowObject(Window);
    
   return Result;
}

/* EOF */
