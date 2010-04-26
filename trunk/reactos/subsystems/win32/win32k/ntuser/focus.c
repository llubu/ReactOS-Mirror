/*
 * ReactOS Win32 Subsystem
 * Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 ReactOS Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include <win32k.h>

#define NDEBUG
#include <debug.h>

HWND FASTCALL
IntGetCaptureWindow(VOID)
{
   PUSER_MESSAGE_QUEUE ForegroundQueue = IntGetFocusMessageQueue();
   return ForegroundQueue != NULL ? ForegroundQueue->CaptureWindow : 0;
}

HWND FASTCALL
IntGetFocusWindow(VOID)
{
   PUSER_MESSAGE_QUEUE ForegroundQueue = IntGetFocusMessageQueue();
   return ForegroundQueue != NULL ? ForegroundQueue->FocusWindow : 0;
}

HWND FASTCALL
IntGetThreadFocusWindow(VOID)
{
   PTHREADINFO pti;
   PUSER_MESSAGE_QUEUE ThreadQueue;

   pti = PsGetCurrentThreadWin32Thread();
   ThreadQueue = pti->MessageQueue;
   return ThreadQueue != NULL ? ThreadQueue->FocusWindow : 0;
}

VOID FASTCALL
co_IntSendDeactivateMessages(HWND hWndPrev, HWND hWnd)
{
   if (hWndPrev)
   {
      co_IntPostOrSendMessage(hWndPrev, WM_NCACTIVATE, FALSE, 0);
      co_IntPostOrSendMessage(hWndPrev, WM_ACTIVATE,
                              MAKEWPARAM(WA_INACTIVE, UserGetWindowLong(hWndPrev, GWL_STYLE, FALSE) & WS_MINIMIZE),
                              (LPARAM)hWnd);
   }
}

VOID FASTCALL
co_IntSendActivateMessages(HWND hWndPrev, HWND hWnd, BOOL MouseActivate)
{
   USER_REFERENCE_ENTRY Ref, RefPrev;
   PWINDOW_OBJECT Window, WindowPrev = NULL;

   if ((Window = UserGetWindowObject(hWnd)))
   { 
      UserRefObjectCo(Window, &Ref);

      WindowPrev = UserGetWindowObject(hWndPrev);

      if (WindowPrev) UserRefObjectCo(WindowPrev, &RefPrev);

      /* Send palette messages */
      if (co_IntPostOrSendMessage(hWnd, WM_QUERYNEWPALETTE, 0, 0))
      {
         UserPostMessage( HWND_BROADCAST,
                          WM_PALETTEISCHANGING,
                         (WPARAM)hWnd,
                          0);
      }

      if (UserGetWindow(hWnd, GW_HWNDPREV) != NULL)
         co_WinPosSetWindowPos(Window, HWND_TOP, 0, 0, 0, 0,
                               SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOSENDCHANGING);

      if (!IntGetOwner(Window) && !IntGetParent(Window))
      {
         co_IntShellHookNotify(HSHELL_WINDOWACTIVATED, (LPARAM) hWnd);
      }

      if (Window->Wnd)
      {  // Set last active for window and it's owner.
         Window->Wnd->hWndLastActive = hWnd;
         if (Window->Wnd->spwndOwner)
            Window->Wnd->spwndOwner->hWndLastActive = hWnd;
         Window->Wnd->state |= WNDS_ACTIVEFRAME;
      }

      if (WindowPrev && WindowPrev->Wnd)
         WindowPrev->Wnd->state &= ~WNDS_ACTIVEFRAME;

      if (Window && WindowPrev)
      {
         PWINDOW_OBJECT cWindow;
         HWND *List, *phWnd;
         HANDLE OldTID = IntGetWndThreadId(WindowPrev);
         HANDLE NewTID = IntGetWndThreadId(Window);

 DPRINT("SendActiveMessage Old -> %x, New -> %x\n", OldTID, NewTID);

         if (OldTID != NewTID)
         {
            List = IntWinListChildren(UserGetWindowObject(IntGetDesktopWindow()));
            if (List)
            {
               for (phWnd = List; *phWnd; ++phWnd)
               {
                  cWindow = UserGetWindowObject(*phWnd);
                  if (cWindow && (IntGetWndThreadId(cWindow) == OldTID))
                  {  // FALSE if the window is being deactivated,
                     // ThreadId that owns the window being activated.
                    co_IntPostOrSendMessage(*phWnd, WM_ACTIVATEAPP, FALSE, (LPARAM)NewTID);
                  }
               }
               for (phWnd = List; *phWnd; ++phWnd)
               {
                  cWindow = UserGetWindowObject(*phWnd);
                  if (cWindow && (IntGetWndThreadId(cWindow) == NewTID))
                  { // TRUE if the window is being activated,
                    // ThreadId that owns the window being deactivated.
                    co_IntPostOrSendMessage(*phWnd, WM_ACTIVATEAPP, TRUE, (LPARAM)OldTID);
                  }
               }
               ExFreePool(List);
            }
         }
         UserDerefObjectCo(WindowPrev); // Now allow the previous window to die.
      }

      UserDerefObjectCo(Window);

      /* FIXME: IntIsWindow */

      co_IntPostOrSendMessage(hWnd, WM_NCACTIVATE, (WPARAM)(hWnd == UserGetForegroundWindow()), 0);
      /* FIXME: WA_CLICKACTIVE */
      co_IntPostOrSendMessage(hWnd, WM_ACTIVATE,
                              MAKEWPARAM(MouseActivate ? WA_CLICKACTIVE : WA_ACTIVE,
                                         UserGetWindowLong(hWnd, GWL_STYLE, FALSE) & WS_MINIMIZE),
                              (LPARAM)hWndPrev);
   }
}

VOID FASTCALL
co_IntSendKillFocusMessages(HWND hWndPrev, HWND hWnd)
{
   if (hWndPrev)
   {
      co_IntPostOrSendMessage(hWndPrev, WM_KILLFOCUS, (WPARAM)hWnd, 0);
   }
}

VOID FASTCALL
co_IntSendSetFocusMessages(HWND hWndPrev, HWND hWnd)
{
   if (hWnd)
   {
      co_IntPostOrSendMessage(hWnd, WM_SETFOCUS, (WPARAM)hWndPrev, 0);
   }
}

HWND FASTCALL
IntFindChildWindowToOwner(PWINDOW_OBJECT Root, PWINDOW_OBJECT Owner)
{
   HWND Ret;
   PWINDOW_OBJECT Child, OwnerWnd;

   for(Child = Root->spwndChild; Child; Child = Child->spwndNext)
   {
      OwnerWnd = UserGetWindowObject(Child->hOwner);
      if(!OwnerWnd)
         continue;

      if(OwnerWnd == Owner)
      {
         Ret = Child->hSelf;
         return Ret;
      }
   }

   return NULL;
}

static BOOL FASTCALL
co_IntSetForegroundAndFocusWindow(PWINDOW_OBJECT Window, PWINDOW_OBJECT FocusWindow, BOOL MouseActivate)
{
   HWND hWnd = Window->hSelf;
   HWND hWndPrev = NULL;
   HWND hWndFocus = FocusWindow->hSelf;
   HWND hWndFocusPrev = NULL;
   PUSER_MESSAGE_QUEUE PrevForegroundQueue;
   PWND Wnd;

   ASSERT_REFS_CO(Window);

   DPRINT("IntSetForegroundAndFocusWindow(%x, %x, %s)\n", hWnd, hWndFocus, MouseActivate ? "TRUE" : "FALSE");

   Wnd = Window->Wnd;

   if ((Wnd->style & (WS_CHILD | WS_POPUP)) == WS_CHILD)
   {
      DPRINT("Failed - Child\n");
      return FALSE;
   }

   if (0 == (Wnd->style & WS_VISIBLE) &&
       Window->pti->pEThread->ThreadsProcess != CsrProcess)
   {
      DPRINT("Failed - Invisible\n");
      return FALSE;
   }

   PrevForegroundQueue = IntGetFocusMessageQueue();
   if (PrevForegroundQueue != 0)
   {
      hWndPrev = PrevForegroundQueue->ActiveWindow;
   }

   if (hWndPrev == hWnd)
   {
      DPRINT("Failed - Same\n");
      return TRUE;
   }

   hWndFocusPrev = (PrevForegroundQueue == FocusWindow->pti->MessageQueue
                    ? FocusWindow->pti->MessageQueue->FocusWindow : NULL);

   /* FIXME: Call hooks. */

   co_IntSendDeactivateMessages(hWndPrev, hWnd);
   co_IntSendKillFocusMessages(hWndFocusPrev, hWndFocus);

   IntSetFocusMessageQueue(Window->pti->MessageQueue);
   if (Window->pti->MessageQueue)
   {
      Window->pti->MessageQueue->ActiveWindow = hWnd;
   }

   if (FocusWindow->pti->MessageQueue)
   {
      FocusWindow->pti->MessageQueue->FocusWindow = hWndFocus;
   }

   if (PrevForegroundQueue != Window->pti->MessageQueue)
   {
      /* FIXME: Send WM_ACTIVATEAPP to all thread windows. */
   }

   co_IntSendSetFocusMessages(hWndFocusPrev, hWndFocus);
   co_IntSendActivateMessages(hWndPrev, hWnd, MouseActivate);

   return TRUE;
}

BOOL FASTCALL
co_IntSetForegroundWindow(PWINDOW_OBJECT Window)//FIXME: can Window be NULL??
{
   /*if (Window)*/ ASSERT_REFS_CO(Window);

   return co_IntSetForegroundAndFocusWindow(Window, Window, FALSE);
}

BOOL FASTCALL
co_IntMouseActivateWindow(PWINDOW_OBJECT Window)
{
   HWND Top;
   PWINDOW_OBJECT TopWindow;
   USER_REFERENCE_ENTRY Ref;
   PWND Wnd;

   ASSERT_REFS_CO(Window);

   Wnd = Window->Wnd;
   if(Wnd->style & WS_DISABLED)
   {
      BOOL Ret;
      PWINDOW_OBJECT TopWnd;
      PWINDOW_OBJECT DesktopWindow = UserGetWindowObject(IntGetDesktopWindow());
      if(DesktopWindow)
      {
         Top = IntFindChildWindowToOwner(DesktopWindow, Window);
         if((TopWnd = UserGetWindowObject(Top)))
         {
            UserRefObjectCo(TopWnd, &Ref);
            Ret = co_IntMouseActivateWindow(TopWnd);
            UserDerefObjectCo(TopWnd);

            return Ret;
         }
      }
      return FALSE;
   }


   TopWindow = UserGetAncestor(Window, GA_ROOT);
   if (!TopWindow) return FALSE;

   /* TMN: Check return valud from this function? */
   UserRefObjectCo(TopWindow, &Ref);

   co_IntSetForegroundAndFocusWindow(TopWindow, Window, TRUE);

   UserDerefObjectCo(TopWindow);

   return TRUE;
}

HWND FASTCALL
co_IntSetActiveWindow(PWINDOW_OBJECT Window OPTIONAL)
{
   PTHREADINFO pti;
   PUSER_MESSAGE_QUEUE ThreadQueue;
   HWND hWndPrev;
   HWND hWnd = 0;
   PWND Wnd;
   CBTACTIVATESTRUCT cbt;

   if (Window)
      ASSERT_REFS_CO(Window);

   pti = PsGetCurrentThreadWin32Thread();
   ThreadQueue = pti->MessageQueue;
   ASSERT(ThreadQueue != 0);

   if (Window != 0)
   {
      Wnd = Window->Wnd;
      if ((!(Wnd->style & WS_VISIBLE) &&
           Window->pti->pEThread->ThreadsProcess != CsrProcess) ||
          (Wnd->style & (WS_POPUP | WS_CHILD)) == WS_CHILD)
      {
         return ThreadQueue ? 0 : ThreadQueue->ActiveWindow;
      }
      hWnd = Window->hSelf;
   }

   hWndPrev = ThreadQueue->ActiveWindow;
   if (hWndPrev == hWnd)
   {
      return hWndPrev;
   }

   /* call CBT hook chain */
   cbt.fMouse     = FALSE;
   cbt.hWndActive = hWndPrev;
   if (co_HOOK_CallHooks( WH_CBT, HCBT_ACTIVATE, (WPARAM)hWnd, (LPARAM)&cbt))
      return 0;

   ThreadQueue->ActiveWindow = hWnd;

   co_IntSendDeactivateMessages(hWndPrev, hWnd);
   co_IntSendActivateMessages(hWndPrev, hWnd, FALSE);

   /* FIXME */
   /*   return IntIsWindow(hWndPrev) ? hWndPrev : 0;*/
   return hWndPrev;
}

static
HWND FASTCALL
co_IntSetFocusWindow(PWINDOW_OBJECT Window OPTIONAL)
{
   HWND hWndPrev = 0;
   PTHREADINFO pti;
   PUSER_MESSAGE_QUEUE ThreadQueue;

   if (Window)
      ASSERT_REFS_CO(Window);

   pti = PsGetCurrentThreadWin32Thread();
   ThreadQueue = pti->MessageQueue;
   ASSERT(ThreadQueue != 0);

   hWndPrev = ThreadQueue->FocusWindow;

   if (Window != 0)
   {
      if (hWndPrev == Window->hSelf)
      {
         return hWndPrev;
      }

     if (co_HOOK_CallHooks( WH_CBT, HCBT_SETFOCUS, (WPARAM)Window->hSelf, (LPARAM)hWndPrev))
        return 0;

      ThreadQueue->FocusWindow = Window->hSelf;

      co_IntSendKillFocusMessages(hWndPrev, Window->hSelf);
      co_IntSendSetFocusMessages(hWndPrev, Window->hSelf);
   }
   else
   {
      ThreadQueue->FocusWindow = 0;

     if (co_HOOK_CallHooks( WH_CBT, HCBT_SETFOCUS, (WPARAM)0, (LPARAM)hWndPrev))
        return 0;

      co_IntSendKillFocusMessages(hWndPrev, 0);
   }
   return hWndPrev;
}


/*
 * @implemented
 */
HWND FASTCALL
UserGetForegroundWindow(VOID)
{
   PUSER_MESSAGE_QUEUE ForegroundQueue;

   ForegroundQueue = IntGetFocusMessageQueue();
   return( ForegroundQueue != NULL ? ForegroundQueue->ActiveWindow : 0);
}


/*
 * @implemented
 */
HWND APIENTRY
NtUserGetForegroundWindow(VOID)
{
   DECLARE_RETURN(HWND);

   DPRINT("Enter NtUserGetForegroundWindow\n");
   UserEnterExclusive();

   RETURN( UserGetForegroundWindow());

CLEANUP:
   DPRINT("Leave NtUserGetForegroundWindow, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


HWND FASTCALL UserGetActiveWindow(VOID)
{
   PTHREADINFO pti;
   PUSER_MESSAGE_QUEUE ThreadQueue;

   pti = PsGetCurrentThreadWin32Thread();
   ThreadQueue = pti->MessageQueue;
   return( ThreadQueue ? ThreadQueue->ActiveWindow : 0);
}


HWND APIENTRY
NtUserSetActiveWindow(HWND hWnd)
{
   USER_REFERENCE_ENTRY Ref;
   DECLARE_RETURN(HWND);

   DPRINT("Enter NtUserSetActiveWindow(%x)\n", hWnd);
   UserEnterExclusive();

   if (hWnd)
   {
      PWINDOW_OBJECT Window;
      PTHREADINFO pti;
      PUSER_MESSAGE_QUEUE ThreadQueue;
      HWND hWndPrev;

      if (!(Window = UserGetWindowObject(hWnd)))
      {
         RETURN( 0);
      }

      pti = PsGetCurrentThreadWin32Thread();
      ThreadQueue = pti->MessageQueue;

      if (Window->pti->MessageQueue != ThreadQueue)
      {
         SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
         RETURN( 0);
      }

      UserRefObjectCo(Window, &Ref);
      hWndPrev = co_IntSetActiveWindow(Window);
      UserDerefObjectCo(Window);

      RETURN( hWndPrev);
   }
   else
   {
      RETURN( co_IntSetActiveWindow(0));
   }

CLEANUP:
   DPRINT("Leave NtUserSetActiveWindow, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/*
 * @implemented
 */
HWND APIENTRY
IntGetCapture(VOID)
{
   PTHREADINFO pti;
   PUSER_MESSAGE_QUEUE ThreadQueue;
   DECLARE_RETURN(HWND);

   DPRINT("Enter IntGetCapture\n");

   pti = PsGetCurrentThreadWin32Thread();
   ThreadQueue = pti->MessageQueue;
   RETURN( ThreadQueue ? ThreadQueue->CaptureWindow : 0);

CLEANUP:
   DPRINT("Leave IntGetCapture, ret=%i\n",_ret_);
   END_CLEANUP;
}

/*
 * @implemented
 */
HWND APIENTRY
NtUserSetCapture(HWND hWnd)
{
   PTHREADINFO pti;
   PUSER_MESSAGE_QUEUE ThreadQueue;
   PWINDOW_OBJECT Window;
   HWND hWndPrev;
   DECLARE_RETURN(HWND);

   DPRINT("Enter NtUserSetCapture(%x)\n", hWnd);
   UserEnterExclusive();

   pti = PsGetCurrentThreadWin32Thread();
   ThreadQueue = pti->MessageQueue;

   if((Window = UserGetWindowObject(hWnd)))
   {
      if(Window->pti->MessageQueue != ThreadQueue)
      {
         RETURN(NULL);
      }
   }

   hWndPrev = MsqSetStateWindow(ThreadQueue, MSQ_STATE_CAPTURE, hWnd);

   /* also remove other windows if not capturing anymore */
   if(hWnd == NULL)
   {
      MsqSetStateWindow(ThreadQueue, MSQ_STATE_MENUOWNER, NULL);
      MsqSetStateWindow(ThreadQueue, MSQ_STATE_MOVESIZE, NULL);
   }

   co_IntPostOrSendMessage(hWndPrev, WM_CAPTURECHANGED, 0, (LPARAM)hWnd);
   ThreadQueue->CaptureWindow = hWnd;

   RETURN( hWndPrev);

CLEANUP:
   DPRINT("Leave NtUserSetCapture, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}



HWND FASTCALL co_UserSetFocus(PWINDOW_OBJECT Window OPTIONAL)
{
   if (Window)
   {
      PTHREADINFO pti;
      PUSER_MESSAGE_QUEUE ThreadQueue;
      HWND hWndPrev;
      PWINDOW_OBJECT TopWnd;
      USER_REFERENCE_ENTRY Ref;
      PWND Wnd;

      ASSERT_REFS_CO(Window);

      pti = PsGetCurrentThreadWin32Thread();
      ThreadQueue = pti->MessageQueue;

      Wnd = Window->Wnd;
      if (Wnd->style & (WS_MINIMIZE | WS_DISABLED))
      {
         return( (ThreadQueue ? ThreadQueue->FocusWindow : 0));
      }

      if (Window->pti->MessageQueue != ThreadQueue)
      {
         SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
         return( 0);
      }

      TopWnd = UserGetAncestor(Window, GA_ROOT);
      if (TopWnd && TopWnd->hSelf != UserGetActiveWindow())
      {
//         PWINDOW_OBJECT WndTops = UserGetWindowObject(hWndTop);
         UserRefObjectCo(TopWnd, &Ref);
         co_IntSetActiveWindow(TopWnd);
         UserDerefObjectCo(TopWnd);
      }

      hWndPrev = co_IntSetFocusWindow(Window);

      return( hWndPrev);
   }
   else
   {
      return( co_IntSetFocusWindow(NULL));
   }

}


/*
 * @implemented
 */
HWND APIENTRY
NtUserSetFocus(HWND hWnd)
{
   PWINDOW_OBJECT Window;
   USER_REFERENCE_ENTRY Ref;
   DECLARE_RETURN(HWND);
   HWND ret;

   DPRINT("Enter NtUserSetFocus(%x)\n", hWnd);
   UserEnterExclusive();

   if (hWnd)
   {
      if (!(Window = UserGetWindowObject(hWnd)))
      {
         RETURN(NULL);
      }

      UserRefObjectCo(Window, &Ref);
      ret = co_UserSetFocus(Window);
      UserDerefObjectCo(Window);

      RETURN(ret);
   }
   else
   {
      RETURN( co_UserSetFocus(0));
   }

CLEANUP:
   DPRINT("Leave NtUserSetFocus, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/* EOF */
