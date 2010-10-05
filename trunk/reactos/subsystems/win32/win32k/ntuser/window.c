/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Windows
 * FILE:             subsystems/win32/win32k/ntuser/window.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISION HISTORY:
 *       06-06-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>

#define NDEBUG
#include <debug.h>


/* dialog resources appear to pass this in 16 bits, handle them properly */
#define CW_USEDEFAULT16 (0x8000)

#define POINT_IN_RECT(p, r) (((r.bottom >= p.y) && (r.top <= p.y))&&((r.left <= p.x )&&( r.right >= p.x )))

/* PRIVATE FUNCTIONS **********************************************************/

/*
 * InitWindowImpl
 *
 * Initialize windowing implementation.
 */

NTSTATUS FASTCALL
InitWindowImpl(VOID)
{
   return STATUS_SUCCESS;
}

/*
 * CleanupWindowImpl
 *
 * Cleanup windowing implementation.
 */

NTSTATUS FASTCALL
CleanupWindowImpl(VOID)
{
   return STATUS_SUCCESS;
}

/* HELPER FUNCTIONS ***********************************************************/

BOOL FASTCALL UserUpdateUiState(PWND Wnd, WPARAM wParam)
{
    WORD Action = LOWORD(wParam);
    WORD Flags = HIWORD(wParam);

    if (Flags & ~(UISF_HIDEFOCUS | UISF_HIDEACCEL | UISF_ACTIVE))
    {
        SetLastWin32Error(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    switch (Action)
    {
        case UIS_INITIALIZE:
            SetLastWin32Error(ERROR_INVALID_PARAMETER);
            return FALSE;

        case UIS_SET:
            if (Flags & UISF_HIDEFOCUS)
                Wnd->HideFocus = TRUE;
            if (Flags & UISF_HIDEACCEL)
                Wnd->HideAccel = TRUE;
            break;

        case UIS_CLEAR:
            if (Flags & UISF_HIDEFOCUS)
                Wnd->HideFocus = FALSE;
            if (Flags & UISF_HIDEACCEL)
                Wnd->HideAccel = FALSE;
            break;
    }

    return TRUE;
}

PWINDOW_OBJECT FASTCALL IntGetWindowObject(HWND hWnd)
{
   PWINDOW_OBJECT Window;

   if (!hWnd) return NULL;

   Window = UserGetWindowObject(hWnd);
   if (Window)
   {
      ASSERT(Window->head.cLockObj >= 0);

      Window->head.cLockObj++;

      ASSERT(Window->Wnd);
   }
   return Window;
}

/* temp hack */
PWINDOW_OBJECT FASTCALL UserGetWindowObject(HWND hWnd)
{
   PTHREADINFO ti;
   PWINDOW_OBJECT Window;

   if (PsGetCurrentProcess() != PsInitialSystemProcess)
   {
       ti = GetW32ThreadInfo();
       if (ti == NULL)
       {
          SetLastWin32Error(ERROR_ACCESS_DENIED);
          return NULL;
       }
   }

   if (!hWnd)
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      return NULL;
   }

   Window = (PWINDOW_OBJECT)UserGetObject(gHandleTable, hWnd, otWindow);
   if (!Window || 0 != (Window->state & WINDOWSTATUS_DESTROYED))
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      return NULL;
   }

   ASSERT(Window->head.cLockObj >= 0);

   ASSERT(Window->Wnd);

   return Window;
}


/*
 * IntIsWindow
 *
 * The function determines whether the specified window handle identifies
 * an existing window.
 *
 * Parameters
 *    hWnd
 *       Handle to the window to test.
 *
 * Return Value
 *    If the window handle identifies an existing window, the return value
 *    is TRUE. If the window handle does not identify an existing window,
 *    the return value is FALSE.
 */

BOOL FASTCALL
IntIsWindow(HWND hWnd)
{
   PWINDOW_OBJECT Window;

   if (!(Window = UserGetWindowObject(hWnd)))
      return FALSE;

   return TRUE;
}



PWINDOW_OBJECT FASTCALL
IntGetParent(PWINDOW_OBJECT Wnd)
{
   if (Wnd->Wnd->style & WS_POPUP)
   {
       return Wnd->spwndOwner;
   }
   else if (Wnd->Wnd->style & WS_CHILD)
   {
      return Wnd->spwndParent;
   }

   return NULL;
}


/*
 * IntWinListChildren
 *
 * Compile a list of all child window handles from given window.
 *
 * Remarks
 *    This function is similar to Wine WIN_ListChildren. The caller
 *    must free the returned list with ExFreePool.
 */

HWND* FASTCALL
IntWinListChildren(PWINDOW_OBJECT Window)
{
   PWINDOW_OBJECT Child;
   HWND *List;
   UINT Index, NumChildren = 0;

   if (!Window) return NULL;

   for (Child = Window->spwndChild; Child; Child = Child->spwndNext)
      ++NumChildren;

   List = ExAllocatePoolWithTag(PagedPool, (NumChildren + 1) * sizeof(HWND), TAG_WINLIST);
   if(!List)
   {
      DPRINT1("Failed to allocate memory for children array\n");
      SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
      return NULL;
   }
   for (Child = Window->spwndChild, Index = 0;
         Child != NULL;
         Child = Child->spwndNext, ++Index)
      List[Index] = Child->hSelf;
   List[Index] = NULL;

   return List;
}

/***********************************************************************
 *           IntSendDestroyMsg
 */
static void IntSendDestroyMsg(HWND hWnd)
{

   PWINDOW_OBJECT Window;
#if 0 /* FIXME */

   GUITHREADINFO info;

   if (GetGUIThreadInfo(GetCurrentThreadId(), &info))
   {
      if (hWnd == info.hwndCaret)
      {
         DestroyCaret();
      }
   }
#endif

   Window = UserGetWindowObject(hWnd);
   if (Window)
   {
//      USER_REFERENCE_ENTRY Ref;
//      UserRefObjectCo(Window, &Ref);

      if (!Window->spwndOwner && !IntGetParent(Window))
      {
         co_IntShellHookNotify(HSHELL_WINDOWDESTROYED, (LPARAM) hWnd);
      }

//      UserDerefObjectCo(Window);
   }

   /* The window could already be destroyed here */

   /*
    * Send the WM_DESTROY to the window.
    */

   co_IntSendMessage(hWnd, WM_DESTROY, 0, 0);

   /*
    * This WM_DESTROY message can trigger re-entrant calls to DestroyWindow
    * make sure that the window still exists when we come back.
    */
#if 0 /* FIXME */

   if (IsWindow(Wnd))
   {
      HWND* pWndArray;
      int i;

      if (!(pWndArray = WIN_ListChildren( hwnd )))
         return;

      /* start from the end (FIXME: is this needed?) */
      for (i = 0; pWndArray[i]; i++)
         ;

      while (--i >= 0)
      {
         if (IsWindow( pWndArray[i] ))
            WIN_SendDestroyMsg( pWndArray[i] );
      }
      HeapFree(GetProcessHeap(), 0, pWndArray);
   }
   else
   {
      DPRINT("destroyed itself while in WM_DESTROY!\n");
   }
#endif
}

static VOID
UserFreeWindowInfo(PTHREADINFO ti, PWINDOW_OBJECT WindowObject)
{
    PCLIENTINFO ClientInfo = GetWin32ClientInfo();
    PWND Wnd = WindowObject->Wnd;

    if (!Wnd) return;
    
    if (ClientInfo->CallbackWnd.pvWnd == DesktopHeapAddressToUser(WindowObject->Wnd))
    {
        ClientInfo->CallbackWnd.hWnd = NULL;
        ClientInfo->CallbackWnd.pvWnd = NULL;
    }

   if (Wnd->strName.Buffer != NULL)
   {
       Wnd->strName.Length = 0;
       Wnd->strName.MaximumLength = 0;
       DesktopHeapFree(Wnd->head.rpdesk,
                       Wnd->strName.Buffer);
       Wnd->strName.Buffer = NULL;
   }

    DesktopHeapFree(Wnd->head.rpdesk, Wnd);
    WindowObject->Wnd = NULL;
}

/***********************************************************************
 *           IntDestroyWindow
 *
 * Destroy storage associated to a window. "Internals" p.358
 *
 * This is the "functional" DestroyWindows function ei. all stuff
 * done in CreateWindow is undone here and not in DestroyWindow:-P

 */
static LRESULT co_UserFreeWindow(PWINDOW_OBJECT Window,
                                   PPROCESSINFO ProcessData,
                                   PTHREADINFO ThreadData,
                                   BOOLEAN SendMessages)
{
   HWND *Children;
   HWND *ChildHandle;
   PWINDOW_OBJECT Child;
   PMENU_OBJECT Menu;
   BOOLEAN BelongsToThreadData;
   PWND Wnd;

   ASSERT(Window);

   Wnd = Window->Wnd;

   if(Window->state & WINDOWSTATUS_DESTROYING)
   {
      DPRINT("Tried to call IntDestroyWindow() twice\n");
      return 0;
   }
   Window->state |= WINDOWSTATUS_DESTROYING;
   Wnd->style &= ~WS_VISIBLE;

   IntNotifyWinEvent(EVENT_OBJECT_DESTROY, Wnd, OBJID_WINDOW, 0);

   /* remove the window already at this point from the thread window list so we
      don't get into trouble when destroying the thread windows while we're still
      in IntDestroyWindow() */
   RemoveEntryList(&Window->ThreadListEntry);

   BelongsToThreadData = IntWndBelongsToThread(Window, ThreadData);

   IntDeRegisterShellHookWindow(Window->hSelf);

   if(SendMessages)
   {
      /* Send destroy messages */
      IntSendDestroyMsg(Window->hSelf);
   }

   /* free child windows */
   Children = IntWinListChildren(Window);
   if (Children)
   {
      for (ChildHandle = Children; *ChildHandle; ++ChildHandle)
      {
         if ((Child = IntGetWindowObject(*ChildHandle)))
         {
            if(!IntWndBelongsToThread(Child, ThreadData))
            {
               /* send WM_DESTROY messages to windows not belonging to the same thread */
               IntSendDestroyMsg(Child->hSelf);
            }
            else
               co_UserFreeWindow(Child, ProcessData, ThreadData, SendMessages);

            UserDereferenceObject(Child);
         }
      }
      ExFreePool(Children);
   }

   if(SendMessages)
   {
      /*
       * Clear the update region to make sure no WM_PAINT messages will be
       * generated for this window while processing the WM_NCDESTROY.
       */
      co_UserRedrawWindow(Window, NULL, 0,
                          RDW_VALIDATE | RDW_NOFRAME | RDW_NOERASE |
                          RDW_NOINTERNALPAINT | RDW_NOCHILDREN);
      if(BelongsToThreadData)
         co_IntSendMessage(Window->hSelf, WM_NCDESTROY, 0, 0);
   }
   DestroyTimersForWindow(ThreadData, Window);
   HOOK_DestroyThreadHooks(ThreadData->pEThread); // This is needed here too!

   /* flush the message queue */
   MsqRemoveWindowMessagesFromQueue(Window);

   /* from now on no messages can be sent to this window anymore */
   Window->state |= WINDOWSTATUS_DESTROYED;
   Wnd->state |= WNDS_DESTROYED;
   Wnd->fnid |= FNID_FREED;

   /* don't remove the WINDOWSTATUS_DESTROYING bit */

   /* reset shell window handles */
   if(ThreadData->rpdesk)
   {
      if (Window->hSelf == ThreadData->rpdesk->rpwinstaParent->ShellWindow)
         ThreadData->rpdesk->rpwinstaParent->ShellWindow = NULL;

      if (Window->hSelf == ThreadData->rpdesk->rpwinstaParent->ShellListView)
         ThreadData->rpdesk->rpwinstaParent->ShellListView = NULL;
   }

   /* Unregister hot keys */
   UnregisterWindowHotKeys (Window);

   /* FIXME: do we need to fake QS_MOUSEMOVE wakebit? */

#if 0 /* FIXME */

   WinPosCheckInternalPos(Window->hSelf);
   if (Window->hSelf == GetCapture())
   {
      ReleaseCapture();
   }

   /* free resources associated with the window */
   TIMER_RemoveWindowTimers(Window->hSelf);
#endif

   if (!(Wnd->style & WS_CHILD) && Wnd->IDMenu
       && (Menu = UserGetMenuObject((HMENU)Wnd->IDMenu)))
   {
      IntDestroyMenuObject(Menu, TRUE, TRUE);
      Wnd->IDMenu = 0;
   }

   if(Window->SystemMenu
         && (Menu = UserGetMenuObject(Window->SystemMenu)))
   {
      IntDestroyMenuObject(Menu, TRUE, TRUE);
      Window->SystemMenu = (HMENU)0;
   }

   DceFreeWindowDCE(Window);    /* Always do this to catch orphaned DCs */
#if 0 /* FIXME */

   WINPROC_FreeProc(Window->winproc, WIN_PROC_WINDOW);
   CLASS_RemoveWindow(Window->Class);
#endif

   IntUnlinkWindow(Window);

   UserReferenceObject(Window);
   UserDeleteObject(Window->hSelf, otWindow);

   IntDestroyScrollBars(Window);

   /* dereference the class */
   IntDereferenceClass(Wnd->pcls,
                       Window->pti->pDeskInfo,
                       Window->pti->ppi);
   Wnd->pcls = NULL;

   if(Window->hrgnClip)
   {
      GreDeleteObject(Window->hrgnClip);
   }

   ASSERT(Window->Wnd != NULL);
   UserFreeWindowInfo(Window->pti, Window);

   UserDereferenceObject(Window);

   IntClipboardFreeWindow(Window);

   return 0;
}

VOID FASTCALL
IntGetWindowBorderMeasures(PWINDOW_OBJECT Window, UINT *cx, UINT *cy)
{
   PWND Wnd = Window->Wnd;
   if(HAS_DLGFRAME(Wnd->style, Wnd->ExStyle) && !(Wnd->style & WS_MINIMIZE))
   {
      *cx = UserGetSystemMetrics(SM_CXDLGFRAME);
      *cy = UserGetSystemMetrics(SM_CYDLGFRAME);
   }
   else
   {
      if(HAS_THICKFRAME(Wnd->style, Wnd->ExStyle)&& !(Wnd->style & WS_MINIMIZE))
      {
         *cx = UserGetSystemMetrics(SM_CXFRAME);
         *cy = UserGetSystemMetrics(SM_CYFRAME);
      }
      else if(HAS_THINFRAME(Wnd->style, Wnd->ExStyle))
      {
         *cx = UserGetSystemMetrics(SM_CXBORDER);
         *cy = UserGetSystemMetrics(SM_CYBORDER);
      }
      else
      {
         *cx = *cy = 0;
      }
   }
}

//
// Same as User32:IntGetWndProc.
//
WNDPROC FASTCALL
IntGetWindowProc(PWND pWnd,
                 BOOL Ansi)
{
   INT i;
   PCLS Class;
   WNDPROC gcpd, Ret = 0;

   ASSERT(UserIsEnteredExclusive() == TRUE);

   Class = pWnd->pcls;

   if (pWnd->state & WNDS_SERVERSIDEWINDOWPROC)
   {
      for ( i = FNID_FIRST; i <= FNID_SWITCH; i++)
      {
         if (GETPFNSERVER(i) == pWnd->lpfnWndProc)
         {
            if (Ansi)
               Ret = GETPFNCLIENTA(i);
            else
               Ret = GETPFNCLIENTW(i);
         }
      }
      return Ret;
   }

   if (Class->fnid == FNID_EDIT)
      Ret = pWnd->lpfnWndProc;
   else
   {
      Ret = pWnd->lpfnWndProc;

      if (Class->fnid <= FNID_GHOST && Class->fnid >= FNID_BUTTON)
      {
         if (Ansi)
         {
            if (GETPFNCLIENTW(Class->fnid) == pWnd->lpfnWndProc)
               Ret = GETPFNCLIENTA(Class->fnid);
         }
         else
         {
            if (GETPFNCLIENTA(Class->fnid) == pWnd->lpfnWndProc)
               Ret = GETPFNCLIENTW(Class->fnid);
         }
      }
      if ( Ret != pWnd->lpfnWndProc)
         return Ret;
   }
   if ( Ansi == !!(pWnd->state & WNDS_ANSIWINDOWPROC) )
      return Ret;

   gcpd = (WNDPROC)UserGetCPD(
                       pWnd,
                      (Ansi ? UserGetCPDA2U : UserGetCPDU2A )|UserGetCPDWindow,
                      (ULONG_PTR)Ret);

   return (gcpd ? gcpd : Ret);
}

static WNDPROC
IntSetWindowProc(PWND pWnd,
                 WNDPROC NewWndProc,
                 BOOL Ansi)
{
   INT i;
   PCALLPROCDATA CallProc;
   PCLS Class;
   WNDPROC Ret, chWndProc = NULL;

   // Retrieve previous window proc.
   Ret = IntGetWindowProc(pWnd, Ansi);

   Class = pWnd->pcls;

   if (IsCallProcHandle(NewWndProc))
   {
      CallProc = UserGetObject(gHandleTable, NewWndProc, otCallProc);
      if (CallProc)
      {  // Reset new WndProc.
         NewWndProc = CallProc->pfnClientPrevious;
         // Reset Ansi from CallProc handle. This is expected with wine "deftest".
         Ansi = !!(CallProc->wType & UserGetCPDU2A);
      }
   }
   // Switch from Client Side call to Server Side call if match. Ref: "deftest".
   for ( i = FNID_FIRST; i <= FNID_SWITCH; i++)
   {
       if (GETPFNCLIENTW(i) == NewWndProc)
       {
          chWndProc = GETPFNSERVER(i);
          break;
       }
       if (GETPFNCLIENTA(i) == NewWndProc)
       {
          chWndProc = GETPFNSERVER(i);
          break;
       }
   }
   // If match, set/reset to Server Side and clear ansi.
   if (chWndProc)
   {
      pWnd->lpfnWndProc = chWndProc;
      pWnd->Unicode = TRUE;
      pWnd->state &= ~WNDS_ANSIWINDOWPROC;
      pWnd->state |= WNDS_SERVERSIDEWINDOWPROC;
   }
   else
   {
      pWnd->Unicode = !Ansi;
      // Handle the state change in here.
      if (Ansi)
         pWnd->state |= WNDS_ANSIWINDOWPROC;
      else
         pWnd->state &= ~WNDS_ANSIWINDOWPROC;

      if (pWnd->state & WNDS_SERVERSIDEWINDOWPROC)
         pWnd->state &= ~WNDS_SERVERSIDEWINDOWPROC;

      if (!NewWndProc) NewWndProc = pWnd->lpfnWndProc;

      if (Class->fnid <= FNID_GHOST && Class->fnid >= FNID_BUTTON)
      {
         if (Ansi)
         {
            if (GETPFNCLIENTW(Class->fnid) == NewWndProc)
               chWndProc = GETPFNCLIENTA(Class->fnid);
         }
         else
         {
            if (GETPFNCLIENTA(Class->fnid) == NewWndProc)
               chWndProc = GETPFNCLIENTW(Class->fnid);
         }
      }
      // Now set the new window proc.
      pWnd->lpfnWndProc = (chWndProc ? chWndProc : NewWndProc);
   }
   return Ret;
}

// Move this to user space!
BOOL FASTCALL
IntGetWindowInfo(PWINDOW_OBJECT Window, PWINDOWINFO pwi)
{
   PWND Wnd = Window->Wnd;

   pwi->cbSize = sizeof(WINDOWINFO);
   pwi->rcWindow = Window->Wnd->rcWindow;
   pwi->rcClient = Window->Wnd->rcClient;
   pwi->dwStyle = Wnd->style;
   pwi->dwExStyle = Wnd->ExStyle;
   pwi->dwWindowStatus = (UserGetForegroundWindow() == Window->hSelf); /* WS_ACTIVECAPTION */
   IntGetWindowBorderMeasures(Window, &pwi->cxWindowBorders, &pwi->cyWindowBorders);
   pwi->atomWindowType = (Wnd->pcls ? Wnd->pcls->atomClassName : 0);
   pwi->wCreatorVersion = 0x400; /* FIXME - return a real version number */
   return TRUE;
}

static BOOL FASTCALL
IntSetMenu(
   PWINDOW_OBJECT Window,
   HMENU Menu,
   BOOL *Changed)
{
   PMENU_OBJECT OldMenu, NewMenu = NULL;
   PWND Wnd = Window->Wnd;

   if ((Wnd->style & (WS_CHILD | WS_POPUP)) == WS_CHILD)
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      return FALSE;
   }

   *Changed = (Wnd->IDMenu != (UINT) Menu);
   if (! *Changed)
   {
      return TRUE;
   }

   if (Wnd->IDMenu)
   {
      OldMenu = IntGetMenuObject((HMENU) Wnd->IDMenu);
      ASSERT(NULL == OldMenu || OldMenu->MenuInfo.Wnd == Window->hSelf);
   }
   else
   {
      OldMenu = NULL;
   }

   if (NULL != Menu)
   {
      NewMenu = IntGetMenuObject(Menu);
      if (NULL == NewMenu)
      {
         if (NULL != OldMenu)
         {
            IntReleaseMenuObject(OldMenu);
         }
         SetLastWin32Error(ERROR_INVALID_MENU_HANDLE);
         return FALSE;
      }
      if (NULL != NewMenu->MenuInfo.Wnd)
      {
         /* Can't use the same menu for two windows */
         if (NULL != OldMenu)
         {
            IntReleaseMenuObject(OldMenu);
         }
         SetLastWin32Error(ERROR_INVALID_MENU_HANDLE);
         return FALSE;
      }

   }

   Wnd->IDMenu = (UINT) Menu;
   if (NULL != NewMenu)
   {
      NewMenu->MenuInfo.Wnd = Window->hSelf;
      IntReleaseMenuObject(NewMenu);
   }
   if (NULL != OldMenu)
   {
      OldMenu->MenuInfo.Wnd = NULL;
      IntReleaseMenuObject(OldMenu);
   }

   return TRUE;
}


/* INTERNAL ******************************************************************/


VOID FASTCALL
co_DestroyThreadWindows(struct _ETHREAD *Thread)
{
   PTHREADINFO WThread;
   PLIST_ENTRY Current;
   PWINDOW_OBJECT Wnd;
   USER_REFERENCE_ENTRY Ref;
   WThread = (PTHREADINFO)Thread->Tcb.Win32Thread;

   while (!IsListEmpty(&WThread->WindowListHead))
   {
      Current = WThread->WindowListHead.Flink;
      Wnd = CONTAINING_RECORD(Current, WINDOW_OBJECT, ThreadListEntry);

      DPRINT("thread cleanup: while destroy wnds, wnd=0x%x\n",Wnd);

      /* window removes itself from the list */

      /*
      fixme: it is critical that the window removes itself! if now, we will loop
      here forever...
      */

      //ASSERT(co_UserDestroyWindow(Wnd));

      UserRefObjectCo(Wnd, &Ref);//faxme: temp hack??
      if (!co_UserDestroyWindow(Wnd))
      {
         DPRINT1("Unable to destroy window 0x%x at thread cleanup... This is _VERY_ bad!\n", Wnd);
      }
      UserDerefObjectCo(Wnd);//faxme: temp hack??
   }
}



/*!
 * Internal function.
 * Returns client window rectangle relative to the upper-left corner of client area.
 *
 * \note Does not check the validity of the parameters
*/
VOID FASTCALL
IntGetClientRect(PWINDOW_OBJECT Window, RECTL *Rect)
{
   ASSERT( Window );
   ASSERT( Rect );

   Rect->left = Rect->top = 0;
   Rect->right = Window->Wnd->rcClient.right - Window->Wnd->rcClient.left;
   Rect->bottom = Window->Wnd->rcClient.bottom - Window->Wnd->rcClient.top;
}


#if 0
HWND FASTCALL
IntGetFocusWindow(VOID)
{
   PUSER_MESSAGE_QUEUE Queue;
   PDESKTOP pdo = IntGetActiveDesktop();

   if( !pdo )
      return NULL;

   Queue = (PUSER_MESSAGE_QUEUE)pdo->ActiveMessageQueue;

   if (Queue == NULL)
      return(NULL);
   else
      return(Queue->FocusWindow);
}
#endif

PMENU_OBJECT FASTCALL
IntGetSystemMenu(PWINDOW_OBJECT Window, BOOL bRevert, BOOL RetMenu)
{
   PMENU_OBJECT Menu, NewMenu = NULL, SysMenu = NULL, ret = NULL;
   PTHREADINFO W32Thread;
   HMENU hNewMenu, hSysMenu;
   ROSMENUITEMINFO ItemInfo;

   if(bRevert)
   {
      W32Thread = PsGetCurrentThreadWin32Thread();

      if(!W32Thread->rpdesk)
         return NULL;

      if(Window->SystemMenu)
      {
         Menu = UserGetMenuObject(Window->SystemMenu);
         if(Menu)
         {
            IntDestroyMenuObject(Menu, TRUE, TRUE);
            Window->SystemMenu = (HMENU)0;
         }
      }

      if(W32Thread->rpdesk->rpwinstaParent->SystemMenuTemplate)
      {
         /* clone system menu */
         Menu = UserGetMenuObject(W32Thread->rpdesk->rpwinstaParent->SystemMenuTemplate);
         if(!Menu)
            return NULL;

         NewMenu = IntCloneMenu(Menu);
         if(NewMenu)
         {
            Window->SystemMenu = NewMenu->MenuInfo.Self;
            NewMenu->MenuInfo.Flags |= MF_SYSMENU;
            NewMenu->MenuInfo.Wnd = Window->hSelf;
            ret = NewMenu;
            //IntReleaseMenuObject(NewMenu);
         }
      }
      else
      {
         hSysMenu = UserCreateMenu(FALSE);
         if (NULL == hSysMenu)
         {
            return NULL;
         }
         SysMenu = IntGetMenuObject(hSysMenu);
         if (NULL == SysMenu)
         {
            UserDestroyMenu(hSysMenu);
            return NULL;
         }
         SysMenu->MenuInfo.Flags |= MF_SYSMENU;
         SysMenu->MenuInfo.Wnd = Window->hSelf;
         hNewMenu = co_IntLoadSysMenuTemplate();
         if(!hNewMenu)
         {
            IntReleaseMenuObject(SysMenu);
            UserDestroyMenu(hSysMenu);
            return NULL;
         }
         Menu = IntGetMenuObject(hNewMenu);
         if(!Menu)
         {
            IntReleaseMenuObject(SysMenu);
            UserDestroyMenu(hSysMenu);
            return NULL;
         }

         NewMenu = IntCloneMenu(Menu);
         if(NewMenu)
         {
            NewMenu->MenuInfo.Flags |= MF_SYSMENU | MF_POPUP;
            IntReleaseMenuObject(NewMenu);
            UserSetMenuDefaultItem(NewMenu, SC_CLOSE, FALSE);

            ItemInfo.cbSize = sizeof(MENUITEMINFOW);
            ItemInfo.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_STATE | MIIM_SUBMENU;
            ItemInfo.fType = MF_POPUP;
            ItemInfo.fState = MFS_ENABLED;
            ItemInfo.dwTypeData = NULL;
            ItemInfo.cch = 0;
            ItemInfo.hSubMenu = NewMenu->MenuInfo.Self;
            IntInsertMenuItem(SysMenu, (UINT) -1, TRUE, &ItemInfo);

            Window->SystemMenu = SysMenu->MenuInfo.Self;

            ret = SysMenu;
         }
         IntDestroyMenuObject(Menu, FALSE, TRUE);
      }
      if(RetMenu)
         return ret;
      else
         return NULL;
   }
   else
   {
      if(Window->SystemMenu)
         return IntGetMenuObject((HMENU)Window->SystemMenu);
      else
         return NULL;
   }
}


BOOL FASTCALL
IntIsChildWindow(PWINDOW_OBJECT Parent, PWINDOW_OBJECT BaseWindow)
{
   PWINDOW_OBJECT Window;
   PWND Wnd;

   Window = BaseWindow;
   while (Window)
   {
      Wnd = Window->Wnd;
      if (Window == Parent)
      {
         return(TRUE);
      }
      if(!(Wnd->style & WS_CHILD))
      {
         break;
      }

      Window = Window->spwndParent;
   }

   return(FALSE);
}

BOOL FASTCALL
IntIsWindowVisible(PWINDOW_OBJECT BaseWindow)
{
   PWINDOW_OBJECT Window;
   PWND Wnd;

   Window = BaseWindow;
   while(Window)
   {
      Wnd = Window->Wnd;
      if(!(Wnd->style & WS_CHILD))
      {
         break;
      }
      if(!(Wnd->style & WS_VISIBLE))
      {
         return FALSE;
      }

      Window = Window->spwndParent;
   }

   if(Window && Wnd->style & WS_VISIBLE)
   {
      return TRUE;
   }

   return FALSE;
}


static VOID FASTCALL
IntLinkWnd(
   PWND Wnd,
   PWND WndInsertAfter) /* set to NULL if top sibling */
{
   if ((Wnd->spwndPrev = WndInsertAfter))
   {
      /* link after WndInsertAfter */
      if ((Wnd->spwndNext = WndInsertAfter->spwndNext))
         Wnd->spwndNext->spwndPrev = Wnd;

      Wnd->spwndPrev->spwndNext = Wnd;
   }
   else
   {
      /* link at top */
      if ((Wnd->spwndNext = Wnd->spwndParent->spwndChild))
         Wnd->spwndNext->spwndPrev = Wnd;

      Wnd->spwndParent->spwndChild = Wnd;
    }
}

/* 
   link the window into siblings list 
   children and parent are kept in place.
*/
VOID FASTCALL
IntLinkWindow(
   PWINDOW_OBJECT Wnd,
   PWINDOW_OBJECT WndInsertAfter /* set to NULL if top sibling */
)
{
   IntLinkWnd(Wnd->Wnd, 
              WndInsertAfter ? WndInsertAfter->Wnd : NULL);

  if ((Wnd->spwndPrev = WndInsertAfter))
   {
      /* link after WndInsertAfter */
      if ((Wnd->spwndNext = WndInsertAfter->spwndNext))
         Wnd->spwndNext->spwndPrev = Wnd;

      Wnd->spwndPrev->spwndNext = Wnd;
   }
   else
   {
      /* link at top */
     if ((Wnd->spwndNext = Wnd->spwndParent->spwndChild))
         Wnd->spwndNext->spwndPrev = Wnd;

     Wnd->spwndParent->spwndChild = Wnd;
   }
}


VOID FASTCALL IntLinkHwnd(PWINDOW_OBJECT Wnd, HWND hWndPrev)
{
    if (hWndPrev == HWND_NOTOPMOST)
    {
        if (!(Wnd->Wnd->ExStyle & WS_EX_TOPMOST) && 
            (Wnd->Wnd->ExStyle2 & WS_EX2_LINKED)) return;  /* nothing to do */
        Wnd->Wnd->ExStyle &= ~WS_EX_TOPMOST;
        hWndPrev = HWND_TOP;  /* fallback to the HWND_TOP case */
    }

    IntUnlinkWindow(Wnd);  /* unlink it from the previous location */
    
    if (hWndPrev == HWND_BOTTOM)
    {
        /* Link in the bottom of the list */
        PWINDOW_OBJECT WndInsertAfter;

        WndInsertAfter = Wnd->spwndParent->spwndChild;
        while( WndInsertAfter && WndInsertAfter->spwndNext)
            WndInsertAfter = WndInsertAfter->spwndNext;

        IntLinkWindow(Wnd, WndInsertAfter);
        Wnd->Wnd->ExStyle &= ~WS_EX_TOPMOST;
    }
    else if (hWndPrev == HWND_TOPMOST)
    {
        /* Link in the top of the list */
        IntLinkWindow(Wnd, NULL);

        Wnd->Wnd->ExStyle |= WS_EX_TOPMOST;
    }
    else if (hWndPrev == HWND_TOP)
    {
        /* Link it after the last topmost window */
        PWINDOW_OBJECT WndInsertBefore;

        WndInsertBefore = Wnd->spwndParent->spwndChild;

        if (!(Wnd->Wnd->ExStyle & WS_EX_TOPMOST))  /* put it above the first non-topmost window */
        {
            while (WndInsertBefore != NULL && WndInsertBefore->spwndNext != NULL)
            {
                if (!(WndInsertBefore->Wnd->ExStyle & WS_EX_TOPMOST)) break;
                if (WndInsertBefore == Wnd->spwndOwner)  /* keep it above owner */
                {
                    Wnd->Wnd->ExStyle |= WS_EX_TOPMOST;
                    break;
                }
                WndInsertBefore = WndInsertBefore->spwndNext;
            }
        }

        IntLinkWindow(Wnd, WndInsertBefore ? WndInsertBefore->spwndPrev : NULL);
    }
    else
    {
        /* Link it after hWndPrev */
        PWINDOW_OBJECT WndInsertAfter;

        WndInsertAfter = UserGetWindowObject(hWndPrev);
        /* Are we called with an erroneous handle */
        if(WndInsertAfter == NULL)
        {
            /* Link in a default position */
            IntLinkHwnd(Wnd, HWND_TOP);
            return;
        }

        IntLinkWindow(Wnd, WndInsertAfter);

        /* Fix the WS_EX_TOPMOST flag */
        if (!(WndInsertAfter->Wnd->ExStyle & WS_EX_TOPMOST)) 
        {
            Wnd->Wnd->ExStyle &= ~WS_EX_TOPMOST;
        }
        else
        {
            if(WndInsertAfter->spwndNext &&
               WndInsertAfter->spwndNext->Wnd->ExStyle & WS_EX_TOPMOST)
            {
                Wnd->Wnd->ExStyle |= WS_EX_TOPMOST;
            }
        }
    }
}

HWND FASTCALL
IntSetOwner(HWND hWnd, HWND hWndNewOwner)
{
   PWINDOW_OBJECT Wnd, WndOldOwner, WndNewOwner;
   HWND ret;

   Wnd = IntGetWindowObject(hWnd);
   if(!Wnd)
      return NULL;

   WndOldOwner = Wnd->spwndOwner;

   ret = WndOldOwner ? WndOldOwner->hSelf : 0;

   if((WndNewOwner = UserGetWindowObject(hWndNewOwner)))
   {
       Wnd->spwndOwner= WndNewOwner;
       Wnd->Wnd->spwndOwner = WndNewOwner->Wnd;
   }
   else
   {
       Wnd->spwndOwner = NULL;
       Wnd->Wnd->spwndOwner = NULL;
   }

   UserDereferenceObject(Wnd);
   return ret;
}

PWINDOW_OBJECT FASTCALL
co_IntSetParent(PWINDOW_OBJECT Wnd, PWINDOW_OBJECT WndNewParent)
{
   PWINDOW_OBJECT WndOldParent;
   BOOL WasVisible;

   ASSERT(Wnd);
   ASSERT(WndNewParent);
   ASSERT_REFS_CO(Wnd);
   ASSERT_REFS_CO(WndNewParent);

   /* Some applications try to set a child as a parent */
   if (IntIsChildWindow(Wnd, WndNewParent))
   {
      SetLastWin32Error( ERROR_INVALID_PARAMETER );
      return NULL;
   }

   /*
    * Windows hides the window first, then shows it again
    * including the WM_SHOWWINDOW messages and all
    */
   WasVisible = co_WinPosShowWindow(Wnd, SW_HIDE);

   /* Window must belong to current process */
   if (Wnd->pti->pEThread->ThreadsProcess != PsGetCurrentProcess())
      return NULL;

   WndOldParent = Wnd->spwndParent;

   if (WndOldParent) UserReferenceObject(WndOldParent); /* caller must deref */

   if (WndNewParent != WndOldParent)
   {
      /* Unlink the window from the siblings list */
      IntUnlinkWindow(Wnd);

      /* Set the new parent */
      Wnd->spwndParent = WndNewParent;
      Wnd->Wnd->spwndParent = WndNewParent->Wnd;

      /* Link the window with its new siblings*/
      IntLinkHwnd(Wnd, HWND_TOP);

   }

   /*
    * SetParent additionally needs to make hwnd the top window
    * in the z-order and send the expected WM_WINDOWPOSCHANGING and
    * WM_WINDOWPOSCHANGED notification messages.
    */
   co_WinPosSetWindowPos(Wnd, (0 == (Wnd->Wnd->ExStyle & WS_EX_TOPMOST) ? HWND_TOP : HWND_TOPMOST),
                         0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE
                         | (WasVisible ? SWP_SHOWWINDOW : 0));

   /*
    * FIXME: a WM_MOVE is also generated (in the DefWindowProc handler
    * for WM_WINDOWPOSCHANGED) in Windows, should probably remove SWP_NOMOVE
    */

   return WndOldParent;
}

BOOL FASTCALL
IntSetSystemMenu(PWINDOW_OBJECT Window, PMENU_OBJECT Menu)
{
   PMENU_OBJECT OldMenu;
   if(Window->SystemMenu)
   {
      OldMenu = IntGetMenuObject(Window->SystemMenu);
      if(OldMenu)
      {
         OldMenu->MenuInfo.Flags &= ~ MF_SYSMENU;
         IntReleaseMenuObject(OldMenu);
      }
   }

   if(Menu)
   {
      /* FIXME check window style, propably return FALSE ? */
      Window->SystemMenu = Menu->MenuInfo.Self;
      Menu->MenuInfo.Flags |= MF_SYSMENU;
   }
   else
      Window->SystemMenu = (HMENU)0;

   return TRUE;
}

/* unlink the window from siblings. children and parent are kept in place. */
VOID FASTCALL
IntUnlinkWnd(PWND Wnd)
{
   if (Wnd->spwndNext)
      Wnd->spwndNext->spwndPrev = Wnd->spwndPrev;

   if (Wnd->spwndPrev)
      Wnd->spwndPrev->spwndNext = Wnd->spwndNext;
  
   if (Wnd->spwndParent && Wnd->spwndParent->spwndChild == Wnd)
      Wnd->spwndParent->spwndChild = Wnd->spwndNext;

   Wnd->spwndPrev = Wnd->spwndNext = NULL;
}


/* unlink the window from siblings. children and parent are kept in place. */
VOID FASTCALL
IntUnlinkWindow(PWINDOW_OBJECT Wnd)
{

    IntUnlinkWnd(Wnd->Wnd);

    if (Wnd->spwndNext)
       Wnd->spwndNext->spwndPrev = Wnd->spwndPrev;
 
    if (Wnd->spwndPrev)
       Wnd->spwndPrev->spwndNext = Wnd->spwndNext;

   if (Wnd->spwndParent && Wnd->spwndParent->spwndChild == Wnd)
      Wnd->spwndParent->spwndChild = Wnd->spwndNext;
 
   Wnd->spwndPrev = Wnd->spwndNext = NULL;
}

BOOL FASTCALL
IntIsWindowInDestroy(PWINDOW_OBJECT Window)
{
   return ((Window->state & WINDOWSTATUS_DESTROYING) == WINDOWSTATUS_DESTROYING);
}


BOOL
FASTCALL
IntGetWindowPlacement(PWINDOW_OBJECT Window, WINDOWPLACEMENT *lpwndpl)
{
   PWND Wnd;
   POINT Size;

   Wnd = Window->Wnd;
   if (!Wnd) return FALSE;

   if(lpwndpl->length != sizeof(WINDOWPLACEMENT))
   {
      return FALSE;
   }

   lpwndpl->flags = 0;
   if (0 == (Wnd->style & WS_VISIBLE))
   {
      lpwndpl->showCmd = SW_HIDE;
   }
   else if (0 != (Window->state & WINDOWOBJECT_RESTOREMAX) ||
            0 != (Wnd->style & WS_MAXIMIZE))
   {
      lpwndpl->showCmd = SW_MAXIMIZE;
   }
   else if (0 != (Wnd->style & WS_MINIMIZE))
   {
      lpwndpl->showCmd = SW_MINIMIZE;
   }
   else if (0 != (Wnd->style & WS_VISIBLE))
   {
      lpwndpl->showCmd = SW_SHOWNORMAL;
   }

   Size.x = Wnd->rcWindow.left;
   Size.y = Wnd->rcWindow.top;
   WinPosInitInternalPos(Window, &Size,
                         &Wnd->rcWindow);

   lpwndpl->rcNormalPosition = Wnd->InternalPos.NormalRect;
   lpwndpl->ptMinPosition = Wnd->InternalPos.IconPos;
   lpwndpl->ptMaxPosition = Wnd->InternalPos.MaxPos;

   return TRUE;
}


/* FUNCTIONS *****************************************************************/

/*
 * @unimplemented
 */
DWORD APIENTRY
NtUserAlterWindowStyle(DWORD Unknown0,
                       DWORD Unknown1,
                       DWORD Unknown2)
{
   UNIMPLEMENTED

   return(0);
}

/*
 * As best as I can figure, this function is used by EnumWindows,
 * EnumChildWindows, EnumDesktopWindows, & EnumThreadWindows.
 *
 * It's supposed to build a list of HWNDs to return to the caller.
 * We can figure out what kind of list by what parameters are
 * passed to us.
 */
/*
 * @implemented
 */
NTSTATUS
APIENTRY
NtUserBuildHwndList(
   HDESK hDesktop,
   HWND hwndParent,
   BOOLEAN bChildren,
   ULONG dwThreadId,
   ULONG lParam,
   HWND* pWnd,
   ULONG* pBufSize)
{
   NTSTATUS Status;
   ULONG dwCount = 0;

   if (pBufSize == 0)
       return ERROR_INVALID_PARAMETER;

   if (hwndParent || !dwThreadId)
   {
      PDESKTOP Desktop;
      PWINDOW_OBJECT Parent, Window;

      if(!hwndParent)
      {
         if(hDesktop == NULL && !(Desktop = IntGetActiveDesktop()))
         {
            return ERROR_INVALID_HANDLE;
         }

         if(hDesktop)
         {
            Status = IntValidateDesktopHandle(hDesktop,
                                              UserMode,
                                              0,
                                              &Desktop);
            if(!NT_SUCCESS(Status))
            {
               return ERROR_INVALID_HANDLE;
            }
         }
         hwndParent = Desktop->DesktopWindow;
      }
      else
      {
         hDesktop = 0;
      }

      if((Parent = UserGetWindowObject(hwndParent)) &&
         (Window = Parent->spwndChild))
      {
         BOOL bGoDown = TRUE;

         Status = STATUS_SUCCESS;
         while(TRUE)
         {
            if (bGoDown)
            {
               if(dwCount++ < *pBufSize && pWnd)
               {
                  _SEH2_TRY
                  {
                     ProbeForWrite(pWnd, sizeof(HWND), 1);
                     *pWnd = Window->hSelf;
                     pWnd++;
                  }
                  _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                  {
                     Status = _SEH2_GetExceptionCode();
                  }
                  _SEH2_END
                  if(!NT_SUCCESS(Status))
                  {
                     SetLastNtError(Status);
                     break;
                  }
               }
               if (Window->spwndChild && bChildren)
               {
                  Window = Window->spwndChild;
                  continue;
               }
               bGoDown = FALSE;
            }
            if (Window->spwndNext)
            {
               Window = Window->spwndNext;
               bGoDown = TRUE;
               continue;
            }
            Window = Window->spwndParent;
            if (Window == Parent)
            {
               break;
            }
         }
      }

      if(hDesktop)
      {
         ObDereferenceObject(Desktop);
      }
   }
   else
   {
      PETHREAD Thread;
      PTHREADINFO W32Thread;
      PLIST_ENTRY Current;
      PWINDOW_OBJECT Window;

      Status = PsLookupThreadByThreadId((HANDLE)dwThreadId, &Thread);
      if(!NT_SUCCESS(Status))
      {
         return ERROR_INVALID_PARAMETER;
      }
      if(!(W32Thread = (PTHREADINFO)Thread->Tcb.Win32Thread))
      {
         ObDereferenceObject(Thread);
         DPRINT("Thread is not a GUI Thread!\n");
         return ERROR_INVALID_PARAMETER;
      }

      Current = W32Thread->WindowListHead.Flink;
      while(Current != &(W32Thread->WindowListHead))
      {
         Window = CONTAINING_RECORD(Current, WINDOW_OBJECT, ThreadListEntry);
         ASSERT(Window);

         if(bChildren || Window->spwndOwner != NULL)
         {
             if(dwCount < *pBufSize && pWnd)
             {
                Status = MmCopyToCaller(pWnd++, &Window->hSelf, sizeof(HWND));
                if(!NT_SUCCESS(Status))
                {
                   SetLastNtError(Status);
                   break;
                }
             }
             dwCount++;
         }
         Current = Current->Flink;
      }

      ObDereferenceObject(Thread);
   }

   *pBufSize = dwCount;
   return STATUS_SUCCESS;
}


/*
 * @implemented
 */
HWND APIENTRY
NtUserChildWindowFromPointEx(HWND hwndParent,
                             LONG x,
                             LONG y,
                             UINT uiFlags)
{
   PWINDOW_OBJECT Parent;
   POINTL Pt;
   HWND Ret;
   HWND *List, *phWnd;

   if(!(Parent = UserGetWindowObject(hwndParent)))
   {
      return NULL;
   }

   Pt.x = x;
   Pt.y = y;

   if(Parent->hSelf != IntGetDesktopWindow())
   {
      Pt.x += Parent->Wnd->rcClient.left;
      Pt.y += Parent->Wnd->rcClient.top;
   }

   if(!IntPtInWindow(Parent, Pt.x, Pt.y))
   {
      return NULL;
   }

   Ret = Parent->hSelf;
   if((List = IntWinListChildren(Parent)))
   {
      for(phWnd = List; *phWnd; phWnd++)
      {
         PWINDOW_OBJECT Child;
         PWND ChildWnd;
         if((Child = UserGetWindowObject(*phWnd)))
         {
            ChildWnd = Child->Wnd;
            if(!(ChildWnd->style & WS_VISIBLE) && (uiFlags & CWP_SKIPINVISIBLE))
            {
               continue;
            }
            if((ChildWnd->style & WS_DISABLED) && (uiFlags & CWP_SKIPDISABLED))
            {
               continue;
            }
            if((ChildWnd->ExStyle & WS_EX_TRANSPARENT) && (uiFlags & CWP_SKIPTRANSPARENT))
            {
               continue;
            }
            if(IntPtInWindow(Child, Pt.x, Pt.y))
            {
               Ret = Child->hSelf;
               break;
            }
         }
      }
      ExFreePool(List);
   }

   return Ret;
}

void FASTCALL
IntFixWindowCoordinates(CREATESTRUCTW* Cs, PWINDOW_OBJECT ParentWindow, DWORD* dwShowMode)
{
#define IS_DEFAULT(x)  ((x) == CW_USEDEFAULT || (x) == (SHORT)0x8000)

   /* default positioning for overlapped windows */
    if(!(Cs->style & (WS_POPUP | WS_CHILD)))
   {
      PMONITOR pMonitor;
      PRTL_USER_PROCESS_PARAMETERS ProcessParams;

      pMonitor = IntGetPrimaryMonitor();
      ASSERT(pMonitor);

      ProcessParams = PsGetCurrentProcess()->Peb->ProcessParameters;

      if (IS_DEFAULT(Cs->x))
      {
          if (!IS_DEFAULT(Cs->y)) *dwShowMode = Cs->y;

          if(ProcessParams->WindowFlags & STARTF_USEPOSITION)
          {
              Cs->x = ProcessParams->StartingX;
              Cs->y = ProcessParams->StartingY;
          }
          else
          {
               Cs->x = pMonitor->cWndStack * (UserGetSystemMetrics(SM_CXSIZE) + UserGetSystemMetrics(SM_CXFRAME));
               Cs->y = pMonitor->cWndStack * (UserGetSystemMetrics(SM_CYSIZE) + UserGetSystemMetrics(SM_CYFRAME));
               if (Cs->x > ((pMonitor->rcWork.right - pMonitor->rcWork.left) / 4) ||
                   Cs->y > ((pMonitor->rcWork.bottom - pMonitor->rcWork.top) / 4))
               {
                  /* reset counter and position */
                  Cs->x = 0;
                  Cs->y = 0;
                  pMonitor->cWndStack = 0;
               }
               pMonitor->cWndStack++;
          }
      }

      if (IS_DEFAULT(Cs->cx))
      {
          if (ProcessParams->WindowFlags & STARTF_USEPOSITION)
          {
              Cs->cx = ProcessParams->CountX;
              Cs->cy = ProcessParams->CountY;
          }
          else
          {
              Cs->cx = (pMonitor->rcWork.right - pMonitor->rcWork.left) * 3 / 4;
              Cs->cy = (pMonitor->rcWork.bottom - pMonitor->rcWork.top) * 3 / 4;
          }
      }
      /* neither x nor cx are default. Check the y values .
       * In the trace we see Outlook and Outlook Express using
       * cy set to CW_USEDEFAULT when opening the address book.
       */
      else if (IS_DEFAULT(Cs->cy))
      {
          DPRINT("Strange use of CW_USEDEFAULT in nHeight\n");
          Cs->cy = (pMonitor->rcWork.bottom - pMonitor->rcWork.top) * 3 / 4;
      }
   }
   else
   {
      /* if CW_USEDEFAULT is set for non-overlapped windows, both values are set to zero */
      if(IS_DEFAULT(Cs->x))
      {
         Cs->x = 0;
         Cs->y = 0;
      }
      if(IS_DEFAULT(Cs->cx))
      {
         Cs->cx = 0;
         Cs->cy = 0;
      }
   }

#undef IS_DEFAULT
}

/* Allocates and initializes a window*/
PWINDOW_OBJECT FASTCALL IntCreateWindow(CREATESTRUCTW* Cs, 
                                        PLARGE_STRING WindowName, 
                                        PCLS Class,
                                        PWINDOW_OBJECT ParentWindow,
                                        PWINDOW_OBJECT OwnerWindow)
{
   PWND Wnd = NULL;
   PWINDOW_OBJECT Window;
   HWND hWnd;
   PTHREADINFO pti = NULL;
   PMENU_OBJECT SystemMenu;
   BOOL MenuChanged;
   BOOL bUnicodeWindow;

   pti = PsGetCurrentThreadWin32Thread();

   /* Automatically add WS_EX_WINDOWEDGE */
   if ((Cs->dwExStyle & WS_EX_DLGMODALFRAME) ||
         ((!(Cs->dwExStyle & WS_EX_STATICEDGE)) &&
         (Cs->style & (WS_DLGFRAME | WS_THICKFRAME))))
      Cs->dwExStyle |= WS_EX_WINDOWEDGE;
   else
      Cs->dwExStyle &= ~WS_EX_WINDOWEDGE;

   /* Is it a unicode window? */
   bUnicodeWindow =!(Cs->dwExStyle & WS_EX_SETANSICREATOR);
   Cs->dwExStyle &= ~WS_EX_SETANSICREATOR;

   /* Allocate the new window */
   Window = (PWINDOW_OBJECT) UserCreateObject( gHandleTable,
                                               pti->rpdesk,
                                               (PHANDLE)&hWnd,
                                               otWindow,
                                               sizeof(WINDOW_OBJECT));
   if (!Window)
   {
      goto AllocError;
   }

   Wnd = DesktopHeapAlloc(pti->rpdesk, sizeof(WND) + Class->cbwndExtra);

   if (!Wnd)
   {
      goto AllocError;
   }

   RtlZeroMemory(Wnd, sizeof(WND) + Class->cbwndExtra);

   DPRINT("Created object with handle %X\n", hWnd);

   if (NULL == pti->rpdesk->DesktopWindow)
   {
      /* If there is no desktop window yet, we must be creating it */
      pti->rpdesk->DesktopWindow = hWnd;
      pti->rpdesk->pDeskInfo->spwnd = Wnd;
   }

   /*
    * Fill out the structure describing it.
    */
   Window->Wnd = Wnd;
   Window->pti = pti;
   Window->hSelf = hWnd;
   Window->spwndParent = ParentWindow;
   Window->spwndOwner = OwnerWindow;

   Wnd->head.h = hWnd;
   Wnd->head.pti = pti;
   Wnd->head.rpdesk = pti->rpdesk;
   Wnd->fnid = 0;
   Wnd->hWndLastActive = hWnd;
   Wnd->state2 |= WNDS2_WIN40COMPAT;
   Wnd->pcls = Class;
   Wnd->hModule = Cs->hInstance;
   Wnd->style = Cs->style & ~WS_VISIBLE;
   Wnd->ExStyle = Cs->dwExStyle;
   Wnd->cbwndExtra = Wnd->pcls->cbwndExtra;
   Wnd->spwndOwner = OwnerWindow ? OwnerWindow->Wnd : NULL;
   Wnd->spwndParent = ParentWindow ? ParentWindow->Wnd : NULL;      

   IntReferenceMessageQueue(Window->pti->MessageQueue);
   if (Wnd->spwndParent != NULL && Cs->hwndParent != 0)
   {
       Wnd->HideFocus = Wnd->spwndParent->HideFocus;
       Wnd->HideAccel = Wnd->spwndParent->HideAccel;
   }

   if (Wnd->pcls->CSF_flags & CSF_SERVERSIDEPROC)
      Wnd->state |= WNDS_SERVERSIDEWINDOWPROC;

 /* BugBoy Comments: Comment below say that System classes are always created
    as UNICODE. In windows, creating a window with the ANSI version of CreateWindow
    sets the window to ansi as verified by testing with IsUnicodeWindow API.

    No where can I see in code or through testing does the window change back
    to ANSI after being created as UNICODE in ROS. I didnt do more testing to
    see what problems this would cause.*/

   // Set WndProc from Class.
   Wnd->lpfnWndProc  = Wnd->pcls->lpfnWndProc;

   // GetWindowProc, test for non server side default classes and set WndProc.
    if ( Wnd->pcls->fnid <= FNID_GHOST && Wnd->pcls->fnid >= FNID_BUTTON )
    {
      if (bUnicodeWindow)
      {
         if (GETPFNCLIENTA(Wnd->pcls->fnid) == Wnd->lpfnWndProc)
            Wnd->lpfnWndProc = GETPFNCLIENTW(Wnd->pcls->fnid);  
      }
      else
      {
         if (GETPFNCLIENTW(Wnd->pcls->fnid) == Wnd->lpfnWndProc)
            Wnd->lpfnWndProc = GETPFNCLIENTA(Wnd->pcls->fnid);
      }
    }

   // If not an Unicode caller, set Ansi creator bit.
   if (!bUnicodeWindow) Wnd->state |= WNDS_ANSICREATOR;

   // Clone Class Ansi/Unicode proc type.
   if (Wnd->pcls->CSF_flags & CSF_ANSIPROC)
   {
      Wnd->state |= WNDS_ANSIWINDOWPROC;
      Wnd->Unicode = FALSE;
   }
   else
   { /*
       It seems there can be both an Ansi creator and Unicode Class Window
       WndProc, unless the following overriding conditions occur:
     */
      if ( !bUnicodeWindow &&
          ( Class->atomClassName == gpsi->atomSysClass[ICLS_BUTTON]    ||
            Class->atomClassName == gpsi->atomSysClass[ICLS_COMBOBOX]  ||
            Class->atomClassName == gpsi->atomSysClass[ICLS_COMBOLBOX] ||
            Class->atomClassName == gpsi->atomSysClass[ICLS_DIALOG]    ||
            Class->atomClassName == gpsi->atomSysClass[ICLS_EDIT]      ||
            Class->atomClassName == gpsi->atomSysClass[ICLS_IME]       ||
            Class->atomClassName == gpsi->atomSysClass[ICLS_LISTBOX]   ||
            Class->atomClassName == gpsi->atomSysClass[ICLS_MDICLIENT] ||
            Class->atomClassName == gpsi->atomSysClass[ICLS_STATIC] ) )
      { // Override Class and set the window Ansi WndProc.
         Wnd->state |= WNDS_ANSIWINDOWPROC;
         Wnd->Unicode = FALSE;
      }
      else
      { // Set the window Unicode WndProc.
         Wnd->state &= ~WNDS_ANSIWINDOWPROC;
         Wnd->Unicode = TRUE;
      }
   }

   /* BugBoy Comments: if the window being created is a edit control, ATOM 0xCxxx,
      then my testing shows that windows (2k and XP) creates a CallProc for it immediately 
      Dont understand why it does this. */
   if (Class->atomClassName == gpsi->atomSysClass[ICLS_EDIT])
   {
      PCALLPROCDATA CallProc;
      //CallProc = CreateCallProc(NULL, Wnd->lpfnWndProc, bUnicodeWindow, Wnd->ti->ppi);
      CallProc = CreateCallProc(NULL, Wnd->lpfnWndProc, Wnd->Unicode , Wnd->head.pti->ppi);

      if (!CallProc)
      {
         SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
         DPRINT1("Warning: Unable to create CallProc for edit control. Control may not operate correctly! hwnd %x\n",hWnd);
      }
      else
      {
         UserAddCallProcToClass(Wnd->pcls, CallProc);
      }
   }

   InitializeListHead(&Wnd->PropListHead);

   if ( WindowName->Buffer != NULL && WindowName->Length > 0 )
   {
      Wnd->strName.Buffer = DesktopHeapAlloc(Wnd->head.rpdesk,
                                             WindowName->Length + sizeof(UNICODE_NULL));
      if (Wnd->strName.Buffer == NULL)
      {
          goto AllocError;
      }

      RtlCopyMemory(Wnd->strName.Buffer, WindowName->Buffer, WindowName->Length);
      Wnd->strName.Buffer[WindowName->Length / sizeof(WCHAR)] = L'\0';
      Wnd->strName.Length = WindowName->Length;
   }

   /* Correct the window style. */
   if ((Wnd->style & (WS_CHILD | WS_POPUP)) != WS_CHILD)
   {
      Wnd->style |= WS_CLIPSIBLINGS;
      if (!(Wnd->style & WS_POPUP))
      {
         Wnd->style |= WS_CAPTION;
         Window->state |= WINDOWOBJECT_NEED_SIZE;
      }
   }

   if ((Wnd->ExStyle & WS_EX_DLGMODALFRAME) ||
          (Wnd->style & (WS_DLGFRAME | WS_THICKFRAME)))
        Wnd->ExStyle |= WS_EX_WINDOWEDGE;
    else
        Wnd->ExStyle &= ~WS_EX_WINDOWEDGE;

   /* create system menu */
   if((Cs->style & WS_SYSMENU) )//&& (dwStyle & WS_CAPTION) == WS_CAPTION)
   {
      SystemMenu = IntGetSystemMenu(Window, TRUE, TRUE);
      if(SystemMenu)
      {
         Window->SystemMenu = SystemMenu->MenuInfo.Self;
         IntReleaseMenuObject(SystemMenu);
      }
   }

   /* Set the window menu */
   if ((Cs->style & (WS_CHILD | WS_POPUP)) != WS_CHILD)
   {
       if (Cs->hMenu)
         IntSetMenu(Window, Cs->hMenu, &MenuChanged);
      else if (Wnd->pcls->lpszMenuName) // Take it from the parent.
      {
          UNICODE_STRING MenuName;
          HMENU hMenu;

          if (IS_INTRESOURCE(Wnd->pcls->lpszMenuName))
          {
             MenuName.Length = 0;
             MenuName.MaximumLength = 0;
             MenuName.Buffer = Wnd->pcls->lpszMenuName;
          }
          else
          {
             RtlInitUnicodeString( &MenuName, Wnd->pcls->lpszMenuName);
          }
          hMenu = co_IntCallLoadMenu( Wnd->pcls->hModule, &MenuName);
          if (hMenu) IntSetMenu(Window, hMenu, &MenuChanged);
      }
   }
   else // Not a child
      Wnd->IDMenu = (UINT) Cs->hMenu;

   /* Insert the window into the thread's window list. */
   InsertTailList (&pti->WindowListHead, &Window->ThreadListEntry);

   /*  Handle "CS_CLASSDC", it is tested first. */
   if ( (Wnd->pcls->style & CS_CLASSDC) && !(Wnd->pcls->pdce) )
   {  /* One DCE per class to have CLASS. */
      Wnd->pcls->pdce = DceAllocDCE( Window, DCE_CLASS_DC );
   }
   else if ( Wnd->pcls->style & CS_OWNDC)
   {  /* Allocate a DCE for this window. */
      DceAllocDCE(Window, DCE_WINDOW_DC);
   }

   return Window;

AllocError:

   if(Window)
      UserDereferenceObject(Window);

   if(Wnd)
       DesktopHeapFree(Wnd->head.rpdesk, Wnd);
   
   SetLastNtError(STATUS_INSUFFICIENT_RESOURCES);
   return NULL;
}

/*
 * @implemented
 */
PWND FASTCALL
co_UserCreateWindowEx(CREATESTRUCTW* Cs,
                     PUNICODE_STRING ClassName,
                     PLARGE_STRING WindowName)
{
   PWINDOW_OBJECT Window = NULL, ParentWindow = NULL, OwnerWindow;
   HWND hWnd, hWndParent, hWndOwner;
   DWORD dwStyle;
   PWINSTATION_OBJECT WinSta;
   PWND Wnd = NULL;
   PCLS Class = NULL;
   SIZE Size;
   POINT MaxPos;
   CBT_CREATEWNDW CbtCreate;
   LRESULT Result;
   USER_REFERENCE_ENTRY ParentRef, Ref;
   PTHREADINFO pti;
   DWORD dwShowMode = SW_SHOW;
   DECLARE_RETURN(PWND);

   /* Get the current window station and reference it */
   pti = GetW32ThreadInfo();
   if (pti == NULL || pti->rpdesk == NULL)
   {
      DPRINT1("Thread is not attached to a desktop! Cannot create window!\n");
      return NULL; //There is nothing to cleanup
   }
   WinSta = pti->rpdesk->rpwinstaParent;
   ObReferenceObjectByPointer(WinSta, KernelMode, ExWindowStationObjectType, 0);

   /* Get the class and reference it*/
   Class = IntGetAndReferenceClass(ClassName, Cs->hInstance);
   if(!Class)
   {
       DPRINT1("Failed to find class %wZ\n", ClassName);
       RETURN(NULL);
   }

   /* Now find the parent and the owner window */
   hWndParent = IntGetDesktopWindow();
   hWndOwner = NULL;

    if (Cs->hwndParent == HWND_MESSAGE)
    {
        Cs->hwndParent = hWndParent = IntGetMessageWindow();
    }
    else if (Cs->hwndParent)
    {
        if ((Cs->style & (WS_CHILD|WS_POPUP)) != WS_CHILD)
            hWndOwner = Cs->hwndParent;
        else
            hWndParent = Cs->hwndParent;
    }
    else if ((Cs->style & (WS_CHILD|WS_POPUP)) == WS_CHILD)
    {
         DPRINT1("Cannot create a child window without a parrent!\n");
         SetLastWin32Error(ERROR_TLW_WITH_WSCHILD);
         RETURN(NULL);  /* WS_CHILD needs a parent, but WS_POPUP doesn't */
    }

    ParentWindow = hWndParent ? UserGetWindowObject(hWndParent): NULL;
    OwnerWindow = hWndOwner ? UserGetWindowObject(hWndOwner): NULL;

    /* FIXME: is this correct?*/
    if(OwnerWindow)
        OwnerWindow = UserGetAncestor(OwnerWindow, GA_ROOT);

   /* Fix the position and the size of the window */
   if (ParentWindow) 
   {
       UserRefObjectCo(ParentWindow, &ParentRef);
       IntFixWindowCoordinates(Cs, ParentWindow, &dwShowMode);
   }

   /* Allocate and initialize the new window */
   Window = IntCreateWindow(Cs, 
                            WindowName, 
                            Class, 
                            ParentWindow, 
                            OwnerWindow);
   if(!Window)
   {
       DPRINT1("IntCreateWindow failed!\n");
       RETURN(0);
   }

   Wnd = Window->Wnd;
   hWnd = Window->hSelf;

   UserRefObjectCo(Window, &Ref);
   ObDereferenceObject(WinSta);

   /* Call the WH_CBT hook */
   dwStyle = Cs->style;
   Cs->style = Wnd->style; /* HCBT_CREATEWND needs the real window style */
   CbtCreate.lpcs = Cs;
   CbtCreate.hwndInsertAfter = HWND_TOP;
   if (ISITHOOKED(WH_CBT))
   {
      if (co_HOOK_CallHooks(WH_CBT, HCBT_CREATEWND, (WPARAM) hWnd, (LPARAM) &CbtCreate))
      {
         DPRINT1("HCBT_CREATEWND hook failed!\n");
         RETURN( (PWND) NULL);
      }
   }
   Cs->style = dwStyle; /* NCCREATE and WM_NCCALCSIZE need the original values*/

   /* Send the WM_GETMINMAXINFO message*/
   Size.cx = Cs->cx;
   Size.cy = Cs->cy;

   if ((dwStyle & WS_THICKFRAME) || !(dwStyle & (WS_POPUP | WS_CHILD)))
   {
      POINT MaxSize, MaxPos, MinTrack, MaxTrack;

      co_WinPosGetMinMaxInfo(Window, &MaxSize, &MaxPos, &MinTrack, &MaxTrack);
      if (Size.cx > MaxTrack.x) Size.cx = MaxTrack.x;
      if (Size.cy > MaxTrack.y) Size.cy = MaxTrack.y;
      if (Size.cx < MinTrack.x) Size.cx = MinTrack.x;
      if (Size.cy < MinTrack.y) Size.cy = MinTrack.y;
   }

   Wnd->rcWindow.left = Cs->x;
   Wnd->rcWindow.top = Cs->y;
   Wnd->rcWindow.right = Cs->x + Size.cx;
   Wnd->rcWindow.bottom = Cs->y + Size.cy;
   if (0 != (Wnd->style & WS_CHILD) && ParentWindow)
   {
      RECTL_vOffsetRect(&Wnd->rcWindow, 
                        ParentWindow->Wnd->rcClient.left,
                        ParentWindow->Wnd->rcClient.top);
   }
   Wnd->rcClient = Wnd->rcWindow;


   /* Link the window*/
   if (NULL != ParentWindow)
   {
      /* link the window into the siblings list */
      if ((dwStyle & (WS_CHILD|WS_MAXIMIZE)) == WS_CHILD)
          IntLinkHwnd(Window, HWND_BOTTOM);
      else
          IntLinkHwnd(Window, HWND_TOP);
   }
   
   /* Send the NCCREATE message */
   Result = co_IntSendMessage(Window->hSelf, WM_NCCREATE, 0, (LPARAM) Cs);
   if (!Result)
   {
      DPRINT1("co_UserCreateWindowEx(): NCCREATE message failed\n");
      RETURN((PWND)0);
   }

   /* Send the WM_NCCALCSIZE message */
   MaxPos.x = Window->Wnd->rcWindow.left;
   MaxPos.y = Window->Wnd->rcWindow.top;

   Result = co_WinPosGetNonClientSize(Window, &Wnd->rcWindow, &Wnd->rcClient);

   RECTL_vOffsetRect(&Wnd->rcWindow, MaxPos.x - Wnd->rcWindow.left,
                                     MaxPos.y - Wnd->rcWindow.top);


   /* Send the WM_CREATE message. */
   Result = co_IntSendMessage(Window->hSelf, WM_CREATE, 0, (LPARAM) Cs);
   if (Result == (LRESULT)-1)
   {
      DPRINT1("co_UserCreateWindowEx(): WM_CREATE message failed\n");
      RETURN((PWND)0);
   }

   /* Send the EVENT_OBJECT_CREATE event*/
   IntNotifyWinEvent(EVENT_OBJECT_CREATE, Window->Wnd, OBJID_WINDOW, 0);

   /* By setting the flag below it can be examined to determine if the window
      was created successfully and a valid pwnd was passed back to caller since
      from here the function has to succeed. */
   Window->Wnd->state2 |= WNDS2_WMCREATEMSGPROCESSED;

   /* Send the WM_SIZE and WM_MOVE messages. */
   if (!(Window->state & WINDOWOBJECT_NEED_SIZE))
   {
        co_WinPosSendSizeMove(Window);
   }

   /* Show or maybe minimize or maximize the window. */
   if (Wnd->style & (WS_MINIMIZE | WS_MAXIMIZE))
   {
      RECTL NewPos;
      UINT16 SwFlag;

      SwFlag = (Wnd->style & WS_MINIMIZE) ? SW_MINIMIZE : SW_MAXIMIZE;

      co_WinPosMinMaximize(Window, SwFlag, &NewPos);

      SwFlag = ((Wnd->style & WS_CHILD) || UserGetActiveWindow()) ?
                SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED :
                SWP_NOZORDER | SWP_FRAMECHANGED;

      co_WinPosSetWindowPos(Window, 0, NewPos.left, NewPos.top,
                            NewPos.right, NewPos.bottom, SwFlag);
   }

   /* Send the WM_PARENTNOTIFY message */
   if ((Wnd->style & WS_CHILD) &&
       (!(Wnd->ExStyle & WS_EX_NOPARENTNOTIFY)) && ParentWindow)
   {
      co_IntSendMessage(ParentWindow->hSelf,
                        WM_PARENTNOTIFY,
                        MAKEWPARAM(WM_CREATE, Wnd->IDMenu),
                        (LPARAM)Window->hSelf);
   }

   /* Notify the shell that a new window was created */
   if ((!hWndParent) && (!hWndOwner))
   {
      co_IntShellHookNotify(HSHELL_WINDOWCREATED, (LPARAM)hWnd);
   }

   /* Initialize and show the window's scrollbars */
   if (Wnd->style & WS_VSCROLL)
   {
      co_UserShowScrollBar(Window, SB_VERT, TRUE);
   }
   if (Wnd->style & WS_HSCROLL)
   {
      co_UserShowScrollBar(Window, SB_HORZ, TRUE);
   }

   /* Show the new window */
   if (Cs->style & WS_VISIBLE)
   {
      if (Wnd->style & WS_MAXIMIZE)
         dwShowMode = SW_SHOW;
      else if (Wnd->style & WS_MINIMIZE)
         dwShowMode = SW_SHOWMINIMIZED;

      co_WinPosShowWindow(Window, dwShowMode);

      if (Wnd->ExStyle & WS_EX_MDICHILD)
      {
        co_IntSendMessage(ParentWindow->hSelf, WM_MDIREFRESHMENU, 0, 0);
        /* ShowWindow won't activate child windows */
        co_WinPosSetWindowPos(Window, HWND_TOP, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);
      }
   }

   DPRINT("co_UserCreateWindowEx(): Created window %X\n", hWnd);
   RETURN( Wnd);

CLEANUP:
   if (!_ret_)
   {
       /* If the window was created, the class will be dereferenced by co_UserDestroyWindow */
       if (Window) 
            co_UserDestroyWindow(Window);
       else if (Class)
           IntDereferenceClass(Class, pti->pDeskInfo, pti->ppi);
   }

   if (Window)
   {
      UserDerefObjectCo(Window);
      UserDereferenceObject(Window);
   }
   if (ParentWindow) UserDerefObjectCo(ParentWindow);

   END_CLEANUP;
}

NTSTATUS
NTAPI
ProbeAndCaptureLargeString(
    OUT PLARGE_STRING plstrSafe,
    IN PLARGE_STRING plstrUnsafe)
{
    LARGE_STRING lstrTemp;
    PVOID pvBuffer = NULL;

    _SEH2_TRY
    {
        /* Probe and copy the string */
        ProbeForRead(plstrUnsafe, sizeof(LARGE_STRING), sizeof(ULONG));
        lstrTemp = *plstrUnsafe;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Fail */
        _SEH2_YIELD(return _SEH2_GetExceptionCode();)
    }
    _SEH2_END

    if (lstrTemp.Length != 0)
    {
        /* Allocate a buffer from paged pool */
        pvBuffer = ExAllocatePoolWithTag(PagedPool, lstrTemp.Length, TAG_STRING);
        if (!pvBuffer)
        {
            return STATUS_NO_MEMORY;
        }

        _SEH2_TRY
        {
            /* Probe and copy the buffer */
            ProbeForRead(lstrTemp.Buffer, lstrTemp.Length, sizeof(WCHAR));
            RtlCopyMemory(pvBuffer, lstrTemp.Buffer, lstrTemp.Length);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Cleanup and fail */
            ExFreePool(pvBuffer);
            _SEH2_YIELD(return _SEH2_GetExceptionCode();)
        }
        _SEH2_END
    }

    /* Set the output string */
    plstrSafe->Buffer = pvBuffer;
    plstrSafe->Length = lstrTemp.Length;
    plstrSafe->MaximumLength = lstrTemp.Length;

    return STATUS_SUCCESS;
}

/**
 * \todo Allow passing plstrClassName as ANSI.
 */
HWND
NTAPI
NtUserCreateWindowEx(
    DWORD dwExStyle,
    PLARGE_STRING plstrClassName,
    PLARGE_STRING plstrClsVersion,
    PLARGE_STRING plstrWindowName,
    DWORD dwStyle,
    int x,
    int y,
    int nWidth,
    int nHeight,
    HWND hWndParent,
    HMENU hMenu,
    HINSTANCE hInstance,
    LPVOID lpParam,
    DWORD dwFlags,
    PVOID acbiBuffer)
{
    NTSTATUS Status;
    LARGE_STRING lstrWindowName;
    LARGE_STRING lstrClassName;
    UNICODE_STRING ustrClassName;
    CREATESTRUCTW Cs;
    HWND hwnd = NULL;
    PWND pwnd;

    lstrWindowName.Buffer = NULL;
    lstrClassName.Buffer = NULL;

    /* Check if we got a Window name */
    if (plstrWindowName)
    {
        /* Copy the string to kernel mode */
        Status = ProbeAndCaptureLargeString(&lstrWindowName, plstrWindowName);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("NtUserCreateWindowEx: failed to capture plstrWindowName\n");
            SetLastNtError(Status);
            return NULL;
        }
        plstrWindowName = &lstrWindowName;
    }

    /* Check if the class is an atom */
    if (IS_ATOM(plstrClassName))
    {
        /* It is, pass the atom in the UNICODE_STRING */
        ustrClassName.Buffer = (PVOID)plstrClassName;
        ustrClassName.Length = 0;
        ustrClassName.MaximumLength = 0;
    }
    else
    {
        /* It's not, capture the class name */
        Status = ProbeAndCaptureLargeString(&lstrClassName, plstrClassName);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("NtUserCreateWindowEx: failed to capture plstrClassName\n");
            /* Set last error, cleanup and return */
            SetLastNtError(Status);
            goto cleanup;
        }

        /* We pass it on as a UNICODE_STRING */
        ustrClassName.Buffer = lstrClassName.Buffer;
        ustrClassName.Length = lstrClassName.Length;
        ustrClassName.MaximumLength = lstrClassName.MaximumLength;
    }

    /* Fill the CREATESTRUCTW */
    /* we will keep here the original parameters */
    Cs.style = dwStyle;
    Cs.lpCreateParams = lpParam;
    Cs.hInstance = hInstance;
    Cs.hMenu = hMenu;
    Cs.hwndParent = hWndParent;
    Cs.cx = nWidth;
    Cs.cy = nHeight;
    Cs.x = x;
    Cs.y = y;
//   Cs.lpszName = (LPCWSTR) WindowName->Buffer;
//   Cs.lpszClass = (LPCWSTR) ClassName->Buffer;
    Cs.lpszName = (LPCWSTR) plstrWindowName;
    Cs.lpszClass = (LPCWSTR) &ustrClassName;
    Cs.dwExStyle = dwExStyle;

    UserEnterExclusive();

    /* Call the internal function */
    pwnd = co_UserCreateWindowEx(&Cs, &ustrClassName, plstrWindowName);

	if(!pwnd)
    {
        DPRINT1("co_UserCreateWindowEx failed!\n");
    }
    hwnd = pwnd ? UserHMGetHandle(pwnd) : NULL;

    UserLeave();

cleanup:
    if (lstrWindowName.Buffer)
    {
        ExFreePoolWithTag(lstrWindowName.Buffer, TAG_STRING);
    }
    if (lstrClassName.Buffer)
    {
        ExFreePoolWithTag(lstrClassName.Buffer, TAG_STRING);
    }

   return hwnd;
}

/*
 * @unimplemented
 */
HDWP APIENTRY
NtUserDeferWindowPos(HDWP WinPosInfo,
                     HWND Wnd,
                     HWND WndInsertAfter,
                     int x,
                     int y,
                     int cx,
                     int cy,
                     UINT Flags)
{
   UNIMPLEMENTED

   return 0;
}


BOOLEAN FASTCALL co_UserDestroyWindow(PWINDOW_OBJECT Window)
{
   BOOLEAN isChild;
   PWND Wnd;
   HWND hWnd;
   PTHREADINFO ti;
   MSG msg;

   ASSERT_REFS_CO(Window); // FIXME: temp hack?

   hWnd = Window->hSelf;

   Wnd = Window->Wnd;

   if (!Wnd) return TRUE; // FIXME: Need to finish object rewrite or lock the thread when killing the window!

   DPRINT("co_UserDestroyWindow \n");

   /* Check for owner thread */
   if ( (Window->pti->pEThread != PsGetCurrentThread()) ||
        Wnd->head.pti != PsGetCurrentThreadWin32Thread() )
   {
      SetLastWin32Error(ERROR_ACCESS_DENIED);
      return FALSE;
   }

   /* If window was created successfully and it is hooked */
   if ((Wnd->state2 & WNDS2_WMCREATEMSGPROCESSED) && (ISITHOOKED(WH_CBT)))
   {
      if (co_HOOK_CallHooks(WH_CBT, HCBT_DESTROYWND, (WPARAM) hWnd, 0)) return FALSE;
   }

   /* Look whether the focus is within the tree of windows we will
    * be destroying.
    */
   if (!co_WinPosShowWindow(Window, SW_HIDE))
   {
      if (UserGetActiveWindow() == Window->hSelf)
      {
         co_WinPosActivateOtherWindow(Window);
      }
   }

   if (Window->pti->MessageQueue->ActiveWindow == Window->hSelf)
      Window->pti->MessageQueue->ActiveWindow = NULL;
   if (Window->pti->MessageQueue->FocusWindow == Window->hSelf)
      Window->pti->MessageQueue->FocusWindow = NULL;
   if (Window->pti->MessageQueue->CaptureWindow == Window->hSelf)
      Window->pti->MessageQueue->CaptureWindow = NULL;

   /*
    * Check if this window is the Shell's Desktop Window. If so set hShellWindow to NULL
    */

   ti = PsGetCurrentThreadWin32Thread();

   if ((ti != NULL) & (ti->pDeskInfo != NULL))
   {
      if (ti->pDeskInfo->hShellWindow == hWnd)
      {
         DPRINT1("Destroying the ShellWindow!\n");
         ti->pDeskInfo->hShellWindow = NULL;
      }
   }

   IntDereferenceMessageQueue(Window->pti->MessageQueue);

   IntEngWindowChanged(Window, WOC_DELETE);
   isChild = (0 != (Wnd->style & WS_CHILD));

#if 0 /* FIXME */

   if (isChild)
   {
      if (! USER_IsExitingThread(GetCurrentThreadId()))
      {
         send_parent_notify(hwnd, WM_DESTROY);
      }
   }
   else if (NULL != GetWindow(Wnd, GW_OWNER))
   {
      co_HOOK_CallHooks( WH_SHELL, HSHELL_WINDOWDESTROYED, (WPARAM)hwnd, 0L, TRUE );
      /* FIXME: clean up palette - see "Internals" p.352 */
   }
#endif

   if (!IntIsWindow(Window->hSelf))
   {
      return TRUE;
   }

   /* Recursively destroy owned windows */
   if (! isChild)
   {
      for (;;)
      {
         BOOL GotOne = FALSE;
         HWND *Children;
         HWND *ChildHandle;
         PWINDOW_OBJECT Child, Desktop;

         Desktop = IntIsDesktopWindow(Window) ? Window :
                   UserGetWindowObject(IntGetDesktopWindow());
         Children = IntWinListChildren(Desktop);

         if (Children)
         {
            for (ChildHandle = Children; *ChildHandle; ++ChildHandle)
            {
               Child = UserGetWindowObject(*ChildHandle);
               if (Child == NULL)
                  continue;
               if (Child->spwndOwner != Window)
               {
                  continue;
               }

               if (IntWndBelongsToThread(Child, PsGetCurrentThreadWin32Thread()))
               {
                  USER_REFERENCE_ENTRY ChildRef;
                  UserRefObjectCo(Child, &ChildRef);//temp hack?
                  co_UserDestroyWindow(Child);
                  UserDerefObjectCo(Child);//temp hack?

                  GotOne = TRUE;
                  continue;
               }

               if (Child->spwndOwner != NULL)
               {
                  Child->spwndOwner = NULL;
                  Child->Wnd->spwndOwner = NULL;
               }

            }
            ExFreePool(Children);
         }
         if (! GotOne)
         {
            break;
         }
      }
   }

    /* Generate mouse move message for the next window */
    msg.message = WM_MOUSEMOVE;
    msg.wParam = IntGetSysCursorInfo()->ButtonsDown;
    msg.lParam = MAKELPARAM(gpsi->ptCursor.x, gpsi->ptCursor.y);
    msg.pt = gpsi->ptCursor;
    MsqInsertSystemMessage(&msg);

   if (!IntIsWindow(Window->hSelf))
   {
      return TRUE;
   }

   /* Destroy the window storage */
   co_UserFreeWindow(Window, PsGetCurrentProcessWin32Process(), PsGetCurrentThreadWin32Thread(), TRUE);

   return TRUE;
}


/*
 * @implemented
 */
BOOLEAN APIENTRY
NtUserDestroyWindow(HWND Wnd)
{
   PWINDOW_OBJECT Window;
   DECLARE_RETURN(BOOLEAN);
   BOOLEAN ret;
   USER_REFERENCE_ENTRY Ref;

   DPRINT("Enter NtUserDestroyWindow\n");
   UserEnterExclusive();

   if (!(Window = UserGetWindowObject(Wnd)))
   {
      RETURN(FALSE);
   }

   UserRefObjectCo(Window, &Ref);//faxme: dunno if win should be reffed during destroy..
   ret = co_UserDestroyWindow(Window);
   UserDerefObjectCo(Window);//faxme: dunno if win should be reffed during destroy..

   RETURN(ret);

CLEANUP:
   DPRINT("Leave NtUserDestroyWindow, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}



/*
 * @unimplemented
 */
DWORD
APIENTRY
NtUserDrawMenuBarTemp(
   HWND hWnd,
   HDC hDC,
   PRECT hRect,
   HMENU hMenu,
   HFONT hFont)
{
   /* we'll use this function just for caching the menu bar */
   UNIMPLEMENTED
   return 0;
}


/*
 * @unimplemented
 */
DWORD APIENTRY
NtUserEndDeferWindowPosEx(DWORD Unknown0,
                          DWORD Unknown1)
{
   UNIMPLEMENTED

   return 0;
}


/*
 * FillWindow: Called from User; Dialog, Edit and ListBox procs during a WM_ERASEBKGND.
 */
/*
 * @unimplemented
 */
BOOL APIENTRY
NtUserFillWindow(HWND hWndPaint,
                 HWND hWndPaint1,
                 HDC  hDC,
                 HBRUSH hBrush)
{
   UNIMPLEMENTED

   return 0;
}


static HWND FASTCALL
IntFindWindow(PWINDOW_OBJECT Parent,
              PWINDOW_OBJECT ChildAfter,
              RTL_ATOM ClassAtom,
              PUNICODE_STRING WindowName)
{
   BOOL CheckWindowName;
   HWND *List, *phWnd;
   HWND Ret = NULL;
   UNICODE_STRING CurrentWindowName;

   ASSERT(Parent);

   CheckWindowName = WindowName->Length != 0;

   if((List = IntWinListChildren(Parent)))
   {
      phWnd = List;
      if(ChildAfter)
      {
         /* skip handles before and including ChildAfter */
         while(*phWnd && (*(phWnd++) != ChildAfter->hSelf))
            ;
      }

      /* search children */
      while(*phWnd)
      {
         PWINDOW_OBJECT Child;
         if(!(Child = UserGetWindowObject(*(phWnd++))))
         {
            continue;
         }

         /* Do not send WM_GETTEXT messages in the kernel mode version!
            The user mode version however calls GetWindowText() which will
            send WM_GETTEXT messages to windows belonging to its processes */
         if (!ClassAtom || Child->Wnd->pcls->atomClassName == ClassAtom)
         {
             // HACK: use UNICODE_STRING instead of LARGE_STRING
             CurrentWindowName.Buffer = Child->Wnd->strName.Buffer;
             CurrentWindowName.Length = Child->Wnd->strName.Length;
             CurrentWindowName.MaximumLength = Child->Wnd->strName.MaximumLength;
             if(!CheckWindowName || 
                (Child->Wnd->strName.Length < 0xFFFF &&
                 !RtlCompareUnicodeString(WindowName, &CurrentWindowName, TRUE)))
             {
            Ret = Child->hSelf;
            break;
         }
      }
      }
      ExFreePool(List);
   }

   return Ret;
}

/*
 * FUNCTION:
 *   Searches a window's children for a window with the specified
 *   class and name
 * ARGUMENTS:
 *   hwndParent     = The window whose childs are to be searched.
 *       NULL = desktop
 *       HWND_MESSAGE = message-only windows
 *
 *   hwndChildAfter = Search starts after this child window.
 *       NULL = start from beginning
 *
 *   ucClassName    = Class name to search for
 *       Reguired parameter.
 *
 *   ucWindowName   = Window name
 *       ->Buffer == NULL = don't care
 *
 * RETURNS:
 *   The HWND of the window if it was found, otherwise NULL
 */
/*
 * @implemented
 */
HWND APIENTRY
NtUserFindWindowEx(HWND hwndParent,
                   HWND hwndChildAfter,
                   PUNICODE_STRING ucClassName,
                   PUNICODE_STRING ucWindowName,
                   DWORD dwUnknown)
{
   PWINDOW_OBJECT Parent, ChildAfter;
   UNICODE_STRING ClassName = {0}, WindowName = {0};
   HWND Desktop, Ret = NULL;
   RTL_ATOM ClassAtom = (RTL_ATOM)0;
   DECLARE_RETURN(HWND);

   DPRINT("Enter NtUserFindWindowEx\n");
   UserEnterShared();

   if (ucClassName != NULL || ucWindowName != NULL)
   {
       _SEH2_TRY
       {
           if (ucClassName != NULL)
           {
               ClassName = ProbeForReadUnicodeString(ucClassName);
               if (ClassName.Length != 0)
               {
                   ProbeForRead(ClassName.Buffer,
                                ClassName.Length,
                                sizeof(WCHAR));
               }
               else if (!IS_ATOM(ClassName.Buffer))
               {
                   SetLastWin32Error(ERROR_INVALID_PARAMETER);
                   _SEH2_LEAVE;
               }

               if (!IntGetAtomFromStringOrAtom(&ClassName,
                                               &ClassAtom))
               {
                   _SEH2_LEAVE;
               }
           }

           if (ucWindowName != NULL)
           {
               WindowName = ProbeForReadUnicodeString(ucWindowName);
               if (WindowName.Length != 0)
               {
                   ProbeForRead(WindowName.Buffer,
                                WindowName.Length,
                                sizeof(WCHAR));
               }
           }
       }
       _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
       {
           SetLastNtError(_SEH2_GetExceptionCode());
           _SEH2_YIELD(RETURN(NULL));
       }
       _SEH2_END;

       if (ucClassName != NULL)
       {
           if (ClassName.Length == 0 && ClassName.Buffer != NULL &&
               !IS_ATOM(ClassName.Buffer))
           {
               SetLastWin32Error(ERROR_INVALID_PARAMETER);
               RETURN(NULL);
           }
           else if (ClassAtom == (RTL_ATOM)0)
           {
               /* LastError code was set by IntGetAtomFromStringOrAtom */
               RETURN(NULL);
           }
       }
   }

   Desktop = IntGetCurrentThreadDesktopWindow();

   if(hwndParent == NULL)
      hwndParent = Desktop;
   else if(hwndParent == HWND_MESSAGE)
   {
     hwndParent = IntGetMessageWindow();
   }

   if(!(Parent = UserGetWindowObject(hwndParent)))
   {
      RETURN( NULL);
   }

   ChildAfter = NULL;
   if(hwndChildAfter && !(ChildAfter = UserGetWindowObject(hwndChildAfter)))
   {
      RETURN( NULL);
   }

   _SEH2_TRY
   {
       if(Parent->hSelf == Desktop)
       {
          HWND *List, *phWnd;
          PWINDOW_OBJECT TopLevelWindow;
          BOOLEAN CheckWindowName;
          BOOLEAN WindowMatches;
          BOOLEAN ClassMatches;

          /* windows searches through all top-level windows if the parent is the desktop
             window */

          if((List = IntWinListChildren(Parent)))
          {
             phWnd = List;

             if(ChildAfter)
             {
                /* skip handles before and including ChildAfter */
                while(*phWnd && (*(phWnd++) != ChildAfter->hSelf))
                   ;
             }

             CheckWindowName = WindowName.Length != 0;

             /* search children */
             while(*phWnd)
             {
                 UNICODE_STRING ustr;

                if(!(TopLevelWindow = UserGetWindowObject(*(phWnd++))))
                {
                   continue;
                }

                /* Do not send WM_GETTEXT messages in the kernel mode version!
                   The user mode version however calls GetWindowText() which will
                   send WM_GETTEXT messages to windows belonging to its processes */
                ustr.Buffer = TopLevelWindow->Wnd->strName.Buffer;
                ustr.Length = TopLevelWindow->Wnd->strName.Length;
                ustr.MaximumLength = TopLevelWindow->Wnd->strName.MaximumLength;
                WindowMatches = !CheckWindowName || 
                                (TopLevelWindow->Wnd->strName.Length < 0xFFFF && 
                                 !RtlCompareUnicodeString(&WindowName, &ustr, TRUE));
                ClassMatches = (ClassAtom == (RTL_ATOM)0) ||
                               ClassAtom == TopLevelWindow->Wnd->pcls->atomClassName;

                if (WindowMatches && ClassMatches)
                {
                   Ret = TopLevelWindow->hSelf;
                   break;
                }

                if (IntFindWindow(TopLevelWindow, NULL, ClassAtom, &WindowName))
                {
                   /* window returns the handle of the top-level window, in case it found
                      the child window */
                   Ret = TopLevelWindow->hSelf;
                   break;
                }

             }
             ExFreePool(List);
          }
       }
       else
          Ret = IntFindWindow(Parent, ChildAfter, ClassAtom, &WindowName);

#if 0

       if(Ret == NULL && hwndParent == NULL && hwndChildAfter == NULL)
       {
          /* FIXME - if both hwndParent and hwndChildAfter are NULL, we also should
                     search the message-only windows. Should this also be done if
                     Parent is the desktop window??? */
          PWINDOW_OBJECT MsgWindows;

          if((MsgWindows = UserGetWindowObject(IntGetMessageWindow())))
          {
             Ret = IntFindWindow(MsgWindows, ChildAfter, ClassAtom, &WindowName);
          }
       }
#endif
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
       SetLastNtError(_SEH2_GetExceptionCode());
       Ret = NULL;
   }
   _SEH2_END;

   RETURN( Ret);

CLEANUP:
   DPRINT("Leave NtUserFindWindowEx, ret %i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


/*
 * @unimplemented
 */
BOOL APIENTRY
NtUserFlashWindowEx(IN PFLASHWINFO pfwi)
{
   UNIMPLEMENTED

   return 0;
}


/*
 * @implemented
 */
PWINDOW_OBJECT FASTCALL UserGetAncestor(PWINDOW_OBJECT Wnd, UINT Type)
{
   PWINDOW_OBJECT WndAncestor, Parent;

   if (Wnd->hSelf == IntGetDesktopWindow())
   {
      return NULL;
   }

   switch (Type)
   {
      case GA_PARENT:
         {
            WndAncestor = Wnd->spwndParent;
            break;
         }

      case GA_ROOT:
         {
            WndAncestor = Wnd;
            Parent = NULL;

            for(;;)
            {
               if(!(Parent = WndAncestor->spwndParent))
               {
                  break;
               }
               if(IntIsDesktopWindow(Parent))
               {
                  break;
               }

               WndAncestor = Parent;
            }
            break;
         }

      case GA_ROOTOWNER:
         {
            WndAncestor = Wnd;

            for (;;)
            {
               PWINDOW_OBJECT Parent;

               Parent = IntGetParent(WndAncestor);

               if (!Parent)
               {
                  break;
               }

               WndAncestor = Parent;
            }
            break;
         }

      default:
         {
            return NULL;
         }
   }

   return WndAncestor;
}

/*
 * @implemented
 */
HWND APIENTRY
NtUserGetAncestor(HWND hWnd, UINT Type)
{
   PWINDOW_OBJECT Window, Ancestor;
   DECLARE_RETURN(HWND);

   DPRINT("Enter NtUserGetAncestor\n");
   UserEnterExclusive();

   if (!(Window = UserGetWindowObject(hWnd)))
   {
      RETURN(NULL);
   }

   Ancestor = UserGetAncestor(Window, Type);
   /* faxme: can UserGetAncestor ever return NULL for a valid window? */

   RETURN(Ancestor ? Ancestor->hSelf : NULL);

CLEANUP:
   DPRINT("Leave NtUserGetAncestor, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


BOOL
APIENTRY
NtUserGetComboBoxInfo(
   HWND hWnd,
   PCOMBOBOXINFO pcbi)
{
   PWINDOW_OBJECT Wnd;
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserGetComboBoxInfo\n");
   UserEnterShared();

   if (!(Wnd = UserGetWindowObject(hWnd)))
   {
      RETURN( FALSE );
   }
   _SEH2_TRY
   {
       if(pcbi)
       {
          ProbeForWrite(pcbi,
                        sizeof(COMBOBOXINFO),
                        1);
       }
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
       SetLastNtError(_SEH2_GetExceptionCode());
       _SEH2_YIELD(RETURN(FALSE));
   }
   _SEH2_END;

   // Pass the user pointer, it was already probed.
   RETURN( (BOOL) co_IntSendMessage( Wnd->hSelf, CB_GETCOMBOBOXINFO, 0, (LPARAM)pcbi));

CLEANUP:
   DPRINT("Leave NtUserGetComboBoxInfo, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


/*
 * @implemented
 */
DWORD APIENTRY
NtUserGetInternalWindowPos( HWND hWnd,
                            LPRECT rectWnd,
                            LPPOINT ptIcon)
{
   PWINDOW_OBJECT Window;
   DWORD Ret = 0;
   BOOL Hit = FALSE;
   WINDOWPLACEMENT wndpl;

   UserEnterShared();

   if (!(Window = UserGetWindowObject(hWnd)) || !Window->Wnd)
   {
      Hit = FALSE;
      goto Exit;
   }

   _SEH2_TRY
   {
       if(rectWnd)
       {
          ProbeForWrite(rectWnd,
                        sizeof(RECT),
                        1);
       }
       if(ptIcon)
       {
          ProbeForWrite(ptIcon,
                        sizeof(POINT),
                        1);
       }
       
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
       SetLastNtError(_SEH2_GetExceptionCode());
       Hit = TRUE;
   }
   _SEH2_END;

   wndpl.length = sizeof(WINDOWPLACEMENT);   

   if (IntGetWindowPlacement(Window, &wndpl) && !Hit)
   {
      _SEH2_TRY
      {
          if (rectWnd)
          {
             RtlCopyMemory(rectWnd, &wndpl.rcNormalPosition , sizeof(RECT));
          }
          if (ptIcon)
          {
             RtlCopyMemory(ptIcon, &wndpl.ptMinPosition, sizeof(POINT));
          }
       
      }
      _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
      {
          SetLastNtError(_SEH2_GetExceptionCode());
          Hit = TRUE;
      }
      _SEH2_END;

      if (!Hit) Ret = wndpl.showCmd;
   }
Exit:
   UserLeave();
   return Ret;
}

DWORD
APIENTRY
NtUserGetListBoxInfo(
   HWND hWnd)
{
   PWINDOW_OBJECT Wnd;
   DECLARE_RETURN(DWORD);

   DPRINT("Enter NtUserGetListBoxInfo\n");
   UserEnterShared();

   if (!(Wnd = UserGetWindowObject(hWnd)))
   {
      RETURN( 0 );
   }

   RETURN( (DWORD) co_IntSendMessage( Wnd->hSelf, LB_GETLISTBOXINFO, 0, 0 ));

CLEANUP:
   DPRINT("Leave NtUserGetListBoxInfo, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


HWND FASTCALL
co_UserSetParent(HWND hWndChild, HWND hWndNewParent)
{
   PWINDOW_OBJECT Wnd = NULL, WndParent = NULL, WndOldParent;
   HWND hWndOldParent = NULL;
   USER_REFERENCE_ENTRY Ref, ParentRef;

   if (IntIsBroadcastHwnd(hWndChild) || IntIsBroadcastHwnd(hWndNewParent))
   {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return( NULL);
   }

   if (hWndChild == IntGetDesktopWindow())
   {
      SetLastWin32Error(ERROR_ACCESS_DENIED);
      return( NULL);
   }

   if (hWndNewParent)
   {
      if (!(WndParent = UserGetWindowObject(hWndNewParent)))
      {
         return( NULL);
      }
   }
   else
   {
      if (!(WndParent = UserGetWindowObject(IntGetDesktopWindow())))
      {
         return( NULL);
      }
   }

   if (!(Wnd = UserGetWindowObject(hWndChild)))
   {
      return( NULL);
   }

   UserRefObjectCo(Wnd, &Ref);
   UserRefObjectCo(WndParent, &ParentRef);

   WndOldParent = co_IntSetParent(Wnd, WndParent);

   UserDerefObjectCo(WndParent);
   UserDerefObjectCo(Wnd);

   if (WndOldParent)
   {
      hWndOldParent = WndOldParent->hSelf;
      UserDereferenceObject(WndOldParent);
   }

   return( hWndOldParent);
}

/*
 * NtUserSetParent
 *
 * The NtUserSetParent function changes the parent window of the specified
 * child window.
 *
 * Remarks
 *    The new parent window and the child window must belong to the same
 *    application. If the window identified by the hWndChild parameter is
 *    visible, the system performs the appropriate redrawing and repainting.
 *    For compatibility reasons, NtUserSetParent does not modify the WS_CHILD
 *    or WS_POPUP window styles of the window whose parent is being changed.
 *
 * Status
 *    @implemented
 */

HWND APIENTRY
NtUserSetParent(HWND hWndChild, HWND hWndNewParent)
{
   DECLARE_RETURN(HWND);

   DPRINT("Enter NtUserSetParent\n");
   UserEnterExclusive();

   /*
      Check Parent first from user space, set it here.
    */
   if (!hWndNewParent)
   {
      hWndNewParent = IntGetDesktopWindow();
   }
   else if (hWndNewParent == HWND_MESSAGE)
   {
      hWndNewParent = IntGetMessageWindow();
   }

   RETURN( co_UserSetParent(hWndChild, hWndNewParent));

CLEANUP:
   DPRINT("Leave NtUserSetParent, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/*
 * UserGetShellWindow
 *
 * Returns a handle to shell window that was set by NtUserSetShellWindowEx.
 *
 * Status
 *    @implemented
 */
HWND FASTCALL UserGetShellWindow(VOID)
{
   PWINSTATION_OBJECT WinStaObject;
   HWND Ret;

   NTSTATUS Status = IntValidateWindowStationHandle(PsGetCurrentProcess()->Win32WindowStation,
                     KernelMode,
                     0,
                     &WinStaObject);

   if (!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      return( (HWND)0);
   }

   Ret = (HWND)WinStaObject->ShellWindow;

   ObDereferenceObject(WinStaObject);
   return( Ret);
}

/*
 * NtUserSetShellWindowEx
 *
 * This is undocumented function to set global shell window. The global
 * shell window has special handling of window position.
 *
 * Status
 *    @implemented
 */
BOOL APIENTRY
NtUserSetShellWindowEx(HWND hwndShell, HWND hwndListView)
{
   PWINSTATION_OBJECT WinStaObject;
   PWINDOW_OBJECT WndShell, WndListView;
   DECLARE_RETURN(BOOL);
   USER_REFERENCE_ENTRY Ref;
   NTSTATUS Status;
   PTHREADINFO ti;

   DPRINT("Enter NtUserSetShellWindowEx\n");
   UserEnterExclusive();

   if (!(WndShell = UserGetWindowObject(hwndShell)))
   {
      RETURN(FALSE);
   }

   if(!(WndListView = UserGetWindowObject(hwndListView)))
   {
      RETURN(FALSE);
   }

   Status = IntValidateWindowStationHandle(PsGetCurrentProcess()->Win32WindowStation,
                     KernelMode,
                     0,
                     &WinStaObject);

   if (!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      RETURN( FALSE);
   }

   /*
    * Test if we are permitted to change the shell window.
    */
   if (WinStaObject->ShellWindow)
   {
      ObDereferenceObject(WinStaObject);
      RETURN( FALSE);
   }

   /*
    * Move shell window into background.
    */
   if (hwndListView && hwndListView != hwndShell)
   {
      /*
       * Disabled for now to get Explorer working.
       * -- Filip, 01/nov/2003
       */
#if 0
      co_WinPosSetWindowPos(hwndListView, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
#endif

      if (WndListView->Wnd->ExStyle & WS_EX_TOPMOST)
      {
         ObDereferenceObject(WinStaObject);
         RETURN( FALSE);
      }
   }

   if (WndShell->Wnd->ExStyle & WS_EX_TOPMOST)
   {
      ObDereferenceObject(WinStaObject);
      RETURN( FALSE);
   }

   UserRefObjectCo(WndShell, &Ref);
   co_WinPosSetWindowPos(WndShell, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);

   WinStaObject->ShellWindow = hwndShell;
   WinStaObject->ShellListView = hwndListView;

   ti = GetW32ThreadInfo();
   if (ti->pDeskInfo) ti->pDeskInfo->hShellWindow = hwndShell;

   UserDerefObjectCo(WndShell);

   ObDereferenceObject(WinStaObject);
   RETURN( TRUE);

CLEANUP:
   DPRINT("Leave NtUserSetShellWindowEx, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/*
 * NtUserGetSystemMenu
 *
 * The NtUserGetSystemMenu function allows the application to access the
 * window menu (also known as the system menu or the control menu) for
 * copying and modifying.
 *
 * Parameters
 *    hWnd
 *       Handle to the window that will own a copy of the window menu.
 *    bRevert
 *       Specifies the action to be taken. If this parameter is FALSE,
 *       NtUserGetSystemMenu returns a handle to the copy of the window menu
 *       currently in use. The copy is initially identical to the window menu
 *       but it can be modified.
 *       If this parameter is TRUE, GetSystemMenu resets the window menu back
 *       to the default state. The previous window menu, if any, is destroyed.
 *
 * Return Value
 *    If the bRevert parameter is FALSE, the return value is a handle to a
 *    copy of the window menu. If the bRevert parameter is TRUE, the return
 *    value is NULL.
 *
 * Status
 *    @implemented
 */

HMENU APIENTRY
NtUserGetSystemMenu(HWND hWnd, BOOL bRevert)
{
   PWINDOW_OBJECT Window;
   PMENU_OBJECT Menu;
   DECLARE_RETURN(HMENU);

   DPRINT("Enter NtUserGetSystemMenu\n");
   UserEnterShared();

   if (!(Window = UserGetWindowObject(hWnd)))
   {
      RETURN(NULL);
   }

   if (!(Menu = IntGetSystemMenu(Window, bRevert, FALSE)))
   {
      RETURN(NULL);
   }

   RETURN(Menu->MenuInfo.Self);

CLEANUP:
   DPRINT("Leave NtUserGetSystemMenu, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/*
 * NtUserSetSystemMenu
 *
 * Status
 *    @implemented
 */

BOOL APIENTRY
NtUserSetSystemMenu(HWND hWnd, HMENU hMenu)
{
   BOOL Result = FALSE;
   PWINDOW_OBJECT Window;
   PMENU_OBJECT Menu;
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserSetSystemMenu\n");
   UserEnterExclusive();

   if (!(Window = UserGetWindowObject(hWnd)))
   {
      RETURN( FALSE);
   }

   if (hMenu)
   {
      /*
       * Assign new menu handle.
       */
      if (!(Menu = UserGetMenuObject(hMenu)))
      {
         RETURN( FALSE);
      }

      Result = IntSetSystemMenu(Window, Menu);
   }

   RETURN( Result);

CLEANUP:
   DPRINT("Leave NtUserSetSystemMenu, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

LONG FASTCALL
co_UserSetWindowLong(HWND hWnd, DWORD Index, LONG NewValue, BOOL Ansi)
{
   PWINDOW_OBJECT Window, Parent;
   PWND Wnd;
   PWINSTATION_OBJECT WindowStation;
   LONG OldValue;
   STYLESTRUCT Style;

   if (hWnd == IntGetDesktopWindow())
   {
      SetLastWin32Error(STATUS_ACCESS_DENIED);
      return( 0);
   }

   if (!(Window = UserGetWindowObject(hWnd)))
   {
      return( 0);
   }

   Wnd = Window->Wnd;

   if (!Wnd) return 0; // No go on zero.

   if ((INT)Index >= 0)
   {
      if ((Index + sizeof(LONG)) > Wnd->cbwndExtra)
      {
         SetLastWin32Error(ERROR_INVALID_INDEX);
         return( 0);
      }

      OldValue = *((LONG *)((PCHAR)(Wnd + 1) + Index));
/*
      if ( Index == DWLP_DLGPROC && Wnd->state & WNDS_DIALOGWINDOW)
      {
         OldValue = (LONG)IntSetWindowProc( Wnd,
                                           (WNDPROC)NewValue,
                                            Ansi);
         if (!OldValue) return 0;
      }
*/
      *((LONG *)((PCHAR)(Wnd + 1) + Index)) = NewValue;
   }
   else
   {
      switch (Index)
      {
         case GWL_EXSTYLE:
            OldValue = (LONG) Wnd->ExStyle;
            Style.styleOld = OldValue;
            Style.styleNew = NewValue;

            /*
             * Remove extended window style bit WS_EX_TOPMOST for shell windows.
             */
            WindowStation = Window->pti->rpdesk->rpwinstaParent;
            if(WindowStation)
            {
               if (hWnd == WindowStation->ShellWindow || hWnd == WindowStation->ShellListView)
                  Style.styleNew &= ~WS_EX_TOPMOST;
            }

            co_IntSendMessage(hWnd, WM_STYLECHANGING, GWL_EXSTYLE, (LPARAM) &Style);
            Wnd->ExStyle = (DWORD)Style.styleNew;
            co_IntSendMessage(hWnd, WM_STYLECHANGED, GWL_EXSTYLE, (LPARAM) &Style);
            break;

         case GWL_STYLE:
            OldValue = (LONG) Wnd->style;
            Style.styleOld = OldValue;
            Style.styleNew = NewValue;
            co_IntSendMessage(hWnd, WM_STYLECHANGING, GWL_STYLE, (LPARAM) &Style);
            Wnd->style = (DWORD)Style.styleNew;
            co_IntSendMessage(hWnd, WM_STYLECHANGED, GWL_STYLE, (LPARAM) &Style);
            break;

         case GWL_WNDPROC:
         {
            if ( Wnd->head.pti->ppi != PsGetCurrentProcessWin32Process() ||
                 Wnd->fnid & FNID_FREED)
            {
               SetLastWin32Error(ERROR_ACCESS_DENIED);
               return( 0);
            }
            OldValue = (LONG)IntSetWindowProc(Wnd,
                                              (WNDPROC)NewValue,
                                              Ansi);
            break;
         }

         case GWL_HINSTANCE:
            OldValue = (LONG) Wnd->hModule;
            Wnd->hModule = (HINSTANCE) NewValue;
            break;

         case GWL_HWNDPARENT:
            Parent = Window->spwndParent;
            if (Parent && (Parent->hSelf == IntGetDesktopWindow()))
               OldValue = (LONG) IntSetOwner(Window->hSelf, (HWND) NewValue);
            else
               OldValue = (LONG) co_UserSetParent(Window->hSelf, (HWND) NewValue);
            break;

         case GWL_ID:
            OldValue = (LONG) Wnd->IDMenu;
            Wnd->IDMenu = (UINT) NewValue;
            break;

         case GWL_USERDATA:
            OldValue = Wnd->dwUserData;
            Wnd->dwUserData = NewValue;
            break;

         default:
            DPRINT1("NtUserSetWindowLong(): Unsupported index %d\n", Index);
            SetLastWin32Error(ERROR_INVALID_INDEX);
            OldValue = 0;
            break;
      }
   }

   return( OldValue);
}

/*
 * NtUserSetWindowLong
 *
 * The NtUserSetWindowLong function changes an attribute of the specified
 * window. The function also sets the 32-bit (long) value at the specified
 * offset into the extra window memory.
 *
 * Status
 *    @implemented
 */

LONG APIENTRY
NtUserSetWindowLong(HWND hWnd, DWORD Index, LONG NewValue, BOOL Ansi)
{
   DECLARE_RETURN(LONG);

   DPRINT("Enter NtUserSetWindowLong\n");
   UserEnterExclusive();

   RETURN( co_UserSetWindowLong(hWnd, Index, NewValue, Ansi));

CLEANUP:
   DPRINT("Leave NtUserSetWindowLong, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/*
 * NtUserSetWindowWord
 *
 * Legacy function similar to NtUserSetWindowLong.
 *
 * Status
 *    @implemented
 */

WORD APIENTRY
NtUserSetWindowWord(HWND hWnd, INT Index, WORD NewValue)
{
   PWINDOW_OBJECT Window;
   WORD OldValue;
   DECLARE_RETURN(WORD);

   DPRINT("Enter NtUserSetWindowWord\n");
   UserEnterExclusive();

   if (!(Window = UserGetWindowObject(hWnd)))
   {
      RETURN( 0);
   }

   switch (Index)
   {
      case GWL_ID:
      case GWL_HINSTANCE:
      case GWL_HWNDPARENT:
         RETURN( co_UserSetWindowLong(Window->hSelf, Index, (UINT)NewValue, TRUE));
      default:
         if (Index < 0)
         {
            SetLastWin32Error(ERROR_INVALID_INDEX);
            RETURN( 0);
         }
   }

   if (Index > Window->Wnd->cbwndExtra - sizeof(WORD))
   {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      RETURN( 0);
   }

   OldValue = *((WORD *)((PCHAR)(Window->Wnd + 1) + Index));
   *((WORD *)((PCHAR)(Window->Wnd + 1) + Index)) = NewValue;

   RETURN( OldValue);

CLEANUP:
   DPRINT("Leave NtUserSetWindowWord, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/*
 * @implemented
 */
BOOL APIENTRY
NtUserGetWindowPlacement(HWND hWnd,
                         WINDOWPLACEMENT *lpwndpl)
{
   PWINDOW_OBJECT Window;
   PWND Wnd;
   POINT Size;
   WINDOWPLACEMENT Safepl;
   NTSTATUS Status;
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserGetWindowPlacement\n");
   UserEnterShared();

   if (!(Window = UserGetWindowObject(hWnd)))
   {
      RETURN( FALSE);
   }
   Wnd = Window->Wnd;

   Status = MmCopyFromCaller(&Safepl, lpwndpl, sizeof(WINDOWPLACEMENT));
   if(!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      RETURN( FALSE);
   }
   if(Safepl.length != sizeof(WINDOWPLACEMENT))
   {
      RETURN( FALSE);
   }

   Safepl.flags = 0;
   if (0 == (Wnd->style & WS_VISIBLE))
   {
      Safepl.showCmd = SW_HIDE;
   }
   else if ((0 != (Window->state & WINDOWOBJECT_RESTOREMAX) ||
            0 != (Wnd->style & WS_MAXIMIZE)) &&
            0 == (Wnd->style & WS_MINIMIZE))
   {
      Safepl.showCmd = SW_SHOWMAXIMIZED;
   }
   else if (0 != (Wnd->style & WS_MINIMIZE))
   {
      Safepl.showCmd = SW_SHOWMINIMIZED;
   }
   else if (0 != (Wnd->style & WS_VISIBLE))
   {
      Safepl.showCmd = SW_SHOWNORMAL;
   }

   Size.x = Wnd->rcWindow.left;
   Size.y = Wnd->rcWindow.top;
   WinPosInitInternalPos(Window, &Size,
                         &Wnd->rcWindow);

   Safepl.rcNormalPosition = Wnd->InternalPos.NormalRect;
   Safepl.ptMinPosition = Wnd->InternalPos.IconPos;
   Safepl.ptMaxPosition = Wnd->InternalPos.MaxPos;

   Status = MmCopyToCaller(lpwndpl, &Safepl, sizeof(WINDOWPLACEMENT));
   if(!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      RETURN( FALSE);
   }

   RETURN( TRUE);

CLEANUP:
   DPRINT("Leave NtUserGetWindowPlacement, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


/*
 * @unimplemented
 */
BOOL APIENTRY
NtUserLockWindowUpdate(HWND hWnd)
{
   UNIMPLEMENTED

   return 0;
}


/*
 * @implemented
 */
BOOL APIENTRY
NtUserMoveWindow(
   HWND hWnd,
   int X,
   int Y,
   int nWidth,
   int nHeight,
   BOOL bRepaint)
{
   return NtUserSetWindowPos(hWnd, 0, X, Y, nWidth, nHeight,
                             (bRepaint ? SWP_NOZORDER | SWP_NOACTIVATE :
                              SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW));
}

/*
 QueryWindow based on KJK::Hyperion and James Tabor.

 0 = QWUniqueProcessId
 1 = QWUniqueThreadId
 2 = QWActiveWindow
 3 = QWFocusWindow
 4 = QWIsHung            Implements IsHungAppWindow found
                                by KJK::Hyperion.

 9 = QWKillWindow        When I called this with hWnd ==
                           DesktopWindow, it shutdown the system
                           and rebooted.
*/
/*
 * @implemented
 */
DWORD APIENTRY
NtUserQueryWindow(HWND hWnd, DWORD Index)
{
   PWINDOW_OBJECT Window;
   PWND pWnd;
   DWORD Result;
   DECLARE_RETURN(UINT);

   DPRINT("Enter NtUserQueryWindow\n");
   UserEnterShared();

   if (!(Window = UserGetWindowObject(hWnd)) || !Window->Wnd)
   {
      RETURN( 0);
   }

   pWnd = Window->Wnd;

   switch(Index)
   {
      case QUERY_WINDOW_UNIQUE_PROCESS_ID:
         Result = (DWORD)IntGetWndProcessId(Window);
         break;

      case QUERY_WINDOW_UNIQUE_THREAD_ID:
         Result = (DWORD)IntGetWndThreadId(Window);
         break;

      case QUERY_WINDOW_ACTIVE:
         Result = (DWORD)UserGetActiveWindow();
         break;

      case QUERY_WINDOW_FOCUS:
         Result = (DWORD)IntGetFocusWindow();
         break;

      case QUERY_WINDOW_ISHUNG:
         Result = (DWORD)MsqIsHung(Window->pti->MessageQueue);
         break;

      case QUERY_WINDOW_REAL_ID:
         Result = (DWORD)pWnd->head.pti->pEThread->Cid.UniqueProcess;

      default:
         Result = (DWORD)NULL;
         break;
   }

   RETURN( Result);

CLEANUP:
   DPRINT("Leave NtUserQueryWindow, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


/*
 * @unimplemented
 */
DWORD APIENTRY
NtUserRealChildWindowFromPoint(DWORD Unknown0,
                               DWORD Unknown1,
                               DWORD Unknown2)
{
   UNIMPLEMENTED

   return 0;
}


/*
 * @implemented
 */
UINT APIENTRY
NtUserRegisterWindowMessage(PUNICODE_STRING MessageNameUnsafe)
{
   UNICODE_STRING SafeMessageName;
   NTSTATUS Status;
   UINT Ret;
   DECLARE_RETURN(UINT);

   DPRINT("Enter NtUserRegisterWindowMessage\n");
   UserEnterExclusive();

   if(MessageNameUnsafe == NULL)
   {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      RETURN( 0);
   }

   Status = IntSafeCopyUnicodeStringTerminateNULL(&SafeMessageName, MessageNameUnsafe);
   if(!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      RETURN( 0);
   }

   Ret = (UINT)IntAddAtom(SafeMessageName.Buffer);
   if (SafeMessageName.Buffer)
      ExFreePoolWithTag(SafeMessageName.Buffer, TAG_STRING);
   RETURN( Ret);

CLEANUP:
   DPRINT("Leave NtUserRegisterWindowMessage, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


/*
 * @unimplemented
 */
DWORD APIENTRY
NtUserSetImeOwnerWindow(DWORD Unknown0,
                        DWORD Unknown1)
{
   UNIMPLEMENTED

   return 0;
}


/*
 * @unimplemented
 */
DWORD APIENTRY
NtUserSetInternalWindowPos(
   HWND    hwnd,
   UINT    showCmd,
   LPRECT  rect,
   LPPOINT pt)
{
   UNIMPLEMENTED

   return 0;

}


/*
 * @unimplemented
 */
BOOL APIENTRY
NtUserSetLayeredWindowAttributes(HWND hwnd,
			   COLORREF crKey,
			   BYTE bAlpha,
			   DWORD dwFlags)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL APIENTRY
NtUserSetLogonNotifyWindow(HWND hWnd)
{
   UNIMPLEMENTED

   return 0;
}


/*
 * @implemented
 */
BOOL APIENTRY
NtUserSetMenu(
   HWND hWnd,
   HMENU Menu,
   BOOL Repaint)
{
   PWINDOW_OBJECT Window;
   BOOL Changed;
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserSetMenu\n");
   UserEnterExclusive();

   if (!(Window = UserGetWindowObject(hWnd)))
   {
      RETURN( FALSE);
   }

   if (! IntSetMenu(Window, Menu, &Changed))
   {
      RETURN( FALSE);
   }

   if (Changed && Repaint)
   {
      USER_REFERENCE_ENTRY Ref;

      UserRefObjectCo(Window, &Ref);
      co_WinPosSetWindowPos(Window, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE |
                            SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED);

      UserDerefObjectCo(Window);
   }

   RETURN( TRUE);

CLEANUP:
   DPRINT("Leave NtUserSetMenu, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


/*
 * @implemented
 */
BOOL APIENTRY
NtUserSetWindowFNID(HWND hWnd,
                    WORD fnID)
{
   PWINDOW_OBJECT Window;
   PWND Wnd;
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserSetWindowFNID\n");
   UserEnterExclusive();

   if (!(Window = UserGetWindowObject(hWnd)))
   {
      RETURN( FALSE);
   }
   Wnd = Window->Wnd;

   if (Wnd->pcls)
   {  // From user land we only set these.
      if ((fnID != FNID_DESTROY) || ((fnID < FNID_BUTTON) && (fnID > FNID_IME)) )
      {
         RETURN( FALSE);
      }
      else
         Wnd->pcls->fnid |= fnID;
   }
   RETURN( TRUE);

CLEANUP:
   DPRINT("Leave NtUserSetWindowFNID\n");
   UserLeave();
   END_CLEANUP;
}


/*
 * @implemented
 */
BOOL APIENTRY
NtUserSetWindowPlacement(HWND hWnd,
                         WINDOWPLACEMENT *lpwndpl)
{
   PWINDOW_OBJECT Window;
   PWND Wnd;
   WINDOWPLACEMENT Safepl;
   NTSTATUS Status;
   DECLARE_RETURN(BOOL);
   USER_REFERENCE_ENTRY Ref;

   DPRINT("Enter NtUserSetWindowPlacement\n");
   UserEnterExclusive();

   if (!(Window = UserGetWindowObject(hWnd)))
   {
      RETURN( FALSE);
   }
   Wnd = Window->Wnd;

   Status = MmCopyFromCaller(&Safepl, lpwndpl, sizeof(WINDOWPLACEMENT));
   if(!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      RETURN( FALSE);
   }
   if(Safepl.length != sizeof(WINDOWPLACEMENT))
   {
      RETURN( FALSE);
   }

   UserRefObjectCo(Window, &Ref);

   if ((Wnd->style & (WS_MAXIMIZE | WS_MINIMIZE)) == 0)
   {
      co_WinPosSetWindowPos(Window, NULL,
                            Safepl.rcNormalPosition.left, Safepl.rcNormalPosition.top,
                            Safepl.rcNormalPosition.right - Safepl.rcNormalPosition.left,
                            Safepl.rcNormalPosition.bottom - Safepl.rcNormalPosition.top,
                            SWP_NOZORDER | SWP_NOACTIVATE);
   }

   /* FIXME - change window status */
   co_WinPosShowWindow(Window, Safepl.showCmd);

   Wnd->InternalPosInitialized = TRUE;
   Wnd->InternalPos.NormalRect = Safepl.rcNormalPosition;
   Wnd->InternalPos.IconPos = Safepl.ptMinPosition;
   Wnd->InternalPos.MaxPos = Safepl.ptMaxPosition;

   UserDerefObjectCo(Window);
   RETURN(TRUE);

CLEANUP:
   DPRINT("Leave NtUserSetWindowPlacement, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


/*
 * @implemented
 */
BOOL APIENTRY
NtUserSetWindowPos(
   HWND hWnd,
   HWND hWndInsertAfter,
   int X,
   int Y,
   int cx,
   int cy,
   UINT uFlags)
{
   DECLARE_RETURN(BOOL);
   PWINDOW_OBJECT Window;
   BOOL ret;
   USER_REFERENCE_ENTRY Ref;

   DPRINT("Enter NtUserSetWindowPos\n");
   UserEnterExclusive();

   if (!(Window = UserGetWindowObject(hWnd)))
   {
      RETURN(FALSE);
   }

   /* First make sure that coordinates are valid for WM_WINDOWPOSCHANGING */
   if (!(uFlags & SWP_NOMOVE))
   {
      if (X < -32768) X = -32768;
      else if (X > 32767) X = 32767;
      if (Y < -32768) Y = -32768;
      else if (Y > 32767) Y = 32767;
   }
   if (!(uFlags & SWP_NOSIZE))
   {
      if (cx < 0) cx = 0;
      else if (cx > 32767) cx = 32767;
      if (cy < 0) cy = 0;
      else if (cy > 32767) cy = 32767;
   }

   UserRefObjectCo(Window, &Ref);
   ret = co_WinPosSetWindowPos(Window, hWndInsertAfter, X, Y, cx, cy, uFlags);
   UserDerefObjectCo(Window);

   RETURN(ret);

CLEANUP:
   DPRINT("Leave NtUserSetWindowPos, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


INT FASTCALL
IntGetWindowRgn(PWINDOW_OBJECT Window, HRGN hRgn)
{
   INT Ret;
   HRGN VisRgn;
   ROSRGNDATA *pRgn;
   PWND Wnd;

   if(!Window)
   {
      return ERROR;
   }
   if(!hRgn)
   {
      return ERROR;
   }

   Wnd = Window->Wnd;

   /* Create a new window region using the window rectangle */
   VisRgn = IntSysCreateRectRgnIndirect(&Window->Wnd->rcWindow);
   NtGdiOffsetRgn(VisRgn, -Window->Wnd->rcWindow.left, -Window->Wnd->rcWindow.top);
   /* if there's a region assigned to the window, combine them both */
   if(Window->hrgnClip && !(Wnd->style & WS_MINIMIZE))
      NtGdiCombineRgn(VisRgn, VisRgn, Window->hrgnClip, RGN_AND);
   /* Copy the region into hRgn */
   NtGdiCombineRgn(hRgn, VisRgn, NULL, RGN_COPY);

   if((pRgn = RGNOBJAPI_Lock(hRgn, NULL)))
   {
      Ret = REGION_Complexity(pRgn);
      RGNOBJAPI_Unlock(pRgn);
   }
   else
      Ret = ERROR;

   REGION_FreeRgnByHandle(VisRgn);

   return Ret;
}

INT FASTCALL
IntGetWindowRgnBox(PWINDOW_OBJECT Window, RECTL *Rect)
{
   INT Ret;
   HRGN VisRgn;
   ROSRGNDATA *pRgn;
   PWND Wnd;

   if(!Window)
   {
      return ERROR;
   }
   if(!Rect)
   {
      return ERROR;
   }

   Wnd = Window->Wnd;

   /* Create a new window region using the window rectangle */
   VisRgn = IntSysCreateRectRgnIndirect(&Window->Wnd->rcWindow);
   NtGdiOffsetRgn(VisRgn, -Window->Wnd->rcWindow.left, -Window->Wnd->rcWindow.top);
   /* if there's a region assigned to the window, combine them both */
   if(Window->hrgnClip && !(Wnd->style & WS_MINIMIZE))
      NtGdiCombineRgn(VisRgn, VisRgn, Window->hrgnClip, RGN_AND);

   if((pRgn = RGNOBJAPI_Lock(VisRgn, NULL)))
   {
      Ret = REGION_Complexity(pRgn);
      *Rect = pRgn->rdh.rcBound;
      RGNOBJAPI_Unlock(pRgn);
   }
   else
      Ret = ERROR;

   REGION_FreeRgnByHandle(VisRgn);

   return Ret;
}


/*
 * @implemented
 */
INT APIENTRY
NtUserSetWindowRgn(
   HWND hWnd,
   HRGN hRgn,
   BOOL bRedraw)
{
   HRGN hrgnCopy;
   PWINDOW_OBJECT Window;
   DECLARE_RETURN(INT);

   DPRINT("Enter NtUserSetWindowRgn\n");
   UserEnterExclusive();

   if (!(Window = UserGetWindowObject(hWnd)))
   {
      RETURN( 0);
   }

   if (hRgn) // The region will be deleted in user32.
   {
      if (GDIOBJ_ValidateHandle(hRgn, GDI_OBJECT_TYPE_REGION))
      {
         hrgnCopy = IntSysCreateRectRgn(0, 0, 0, 0);
         NtGdiCombineRgn(hrgnCopy, hRgn, 0, RGN_COPY);
      }
      else
         RETURN( 0);
   }
   else
      hrgnCopy = (HRGN) 1;

   if (Window->hrgnClip)
   {
      /* Delete no longer needed region handle */
      GreDeleteObject(Window->hrgnClip);
   }
   Window->hrgnClip = hrgnCopy;

   /* FIXME - send WM_WINDOWPOSCHANGING and WM_WINDOWPOSCHANGED messages to the window */

   if(bRedraw)
   {
      USER_REFERENCE_ENTRY Ref;
      UserRefObjectCo(Window, &Ref);
      co_UserRedrawWindow(Window, NULL, NULL, RDW_INVALIDATE);
      UserDerefObjectCo(Window);
   }

   RETURN( (INT)hRgn);

CLEANUP:
   DPRINT("Leave NtUserSetWindowRgn, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


/*
 * @implemented
 */
BOOL APIENTRY
NtUserShowWindow(HWND hWnd, LONG nCmdShow)
{
   PWINDOW_OBJECT Window;
   BOOL ret;
   DECLARE_RETURN(BOOL);
   USER_REFERENCE_ENTRY Ref;

   DPRINT("Enter NtUserShowWindow\n");
   UserEnterExclusive();

   if (!(Window = UserGetWindowObject(hWnd)))
   {
      RETURN(FALSE);
   }

   UserRefObjectCo(Window, &Ref);
   ret = co_WinPosShowWindow(Window, nCmdShow);
   UserDerefObjectCo(Window);

   RETURN(ret);

CLEANUP:
   DPRINT("Leave NtUserShowWindow, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


/*
 * @unimplemented
 */
BOOL APIENTRY
NtUserShowWindowAsync(HWND hWnd, LONG nCmdShow)
{
#if 0
   UNIMPLEMENTED
   return 0;
#else
   return NtUserShowWindow(hWnd, nCmdShow);
#endif
}


/*
 * @unimplemented
 */
BOOL
APIENTRY
NtUserUpdateLayeredWindow(
   HWND hwnd,
   HDC hdcDst,
   POINT *pptDst,
   SIZE *psize,
   HDC hdcSrc,
   POINT *pptSrc,
   COLORREF crKey,
   BLENDFUNCTION *pblend,
   DWORD dwFlags,
   RECT *prcDirty)
{
   UNIMPLEMENTED

   return 0;
}

/*
 *    @unimplemented
 */
HWND APIENTRY
NtUserWindowFromPhysicalPoint(POINT Point)
{
   UNIMPLEMENTED

   return NULL;
}

/*
 *    @implemented
 */
HWND APIENTRY
NtUserWindowFromPoint(LONG X, LONG Y)
{
   POINT pt;
   HWND Ret;
   PWINDOW_OBJECT DesktopWindow = NULL, Window = NULL;
   DECLARE_RETURN(HWND);
   USER_REFERENCE_ENTRY Ref;

   DPRINT("Enter NtUserWindowFromPoint\n");
   UserEnterExclusive();

   if ((DesktopWindow = UserGetWindowObject(IntGetDesktopWindow())))
   {
      PTHREADINFO pti;

      pt.x = X;
      pt.y = Y;

      //hmm... threads live on desktops thus we have a reference on the desktop and indirectly the desktop window
      //its possible this referencing is useless, thou it shouldnt hurt...
      UserRefObjectCo(DesktopWindow, &Ref);

      pti = PsGetCurrentThreadWin32Thread();
      co_WinPosWindowFromPoint(DesktopWindow, pti->MessageQueue, &pt, &Window);

      if(Window)
      {
         Ret = Window->hSelf;

         RETURN( Ret);
      }
   }

   RETURN( NULL);

CLEANUP:
   if (Window) UserDereferenceObject(Window);
   if (DesktopWindow) UserDerefObjectCo(DesktopWindow);

   DPRINT("Leave NtUserWindowFromPoint, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;

}


/*
 * NtUserDefSetText
 *
 * Undocumented function that is called from DefWindowProc to set
 * window text.
 *
 * Status
 *    @implemented
 */
BOOL APIENTRY
NtUserDefSetText(HWND hWnd, PLARGE_STRING WindowText)
{
   PWINDOW_OBJECT Window;
   PWND Wnd;
   LARGE_STRING SafeText;
   UNICODE_STRING UnicodeString;
   BOOL Ret = TRUE;

   DPRINT("Enter NtUserDefSetText\n");

   if (WindowText != NULL)
   {
      _SEH2_TRY
      {
         SafeText = ProbeForReadLargeString(WindowText);
      }
      _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
      {
         Ret = FALSE;
         SetLastNtError(_SEH2_GetExceptionCode());
      }
      _SEH2_END;

      if (!Ret)
         return FALSE;
   }
   else
      return TRUE;

   UserEnterExclusive();

   if(!(Window = UserGetWindowObject(hWnd)) || !Window->Wnd)
   {
      UserLeave();
      return FALSE;
   }
   Wnd = Window->Wnd;

   // ReactOS uses Unicode and not mixed. Up/Down converting will take time.
   // Brought to you by: The Wine Project! Dysfunctional Thought Processes!
   // Now we know what the bAnsi is for.
   RtlInitUnicodeString(&UnicodeString, NULL);
   if (SafeText.Buffer)
   {
      _SEH2_TRY
      {
         if (SafeText.bAnsi)
            ProbeForRead(SafeText.Buffer, SafeText.Length, sizeof(CHAR));
         else
            ProbeForRead(SafeText.Buffer, SafeText.Length, sizeof(WCHAR));
         Ret = RtlLargeStringToUnicodeString(&UnicodeString, &SafeText);
      }
      _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
      {
         Ret = FALSE;
         SetLastNtError(_SEH2_GetExceptionCode());
      }
      _SEH2_END;
      if (!Ret) goto Exit;
   }

   if (UnicodeString.Length != 0)
   {
      if (Wnd->strName.MaximumLength > 0 &&
          UnicodeString.Length <= Wnd->strName.MaximumLength - sizeof(UNICODE_NULL))
      {
         ASSERT(Wnd->strName.Buffer != NULL);

         Wnd->strName.Length = UnicodeString.Length;
         Wnd->strName.Buffer[UnicodeString.Length / sizeof(WCHAR)] = L'\0';
         RtlCopyMemory(Wnd->strName.Buffer,
                              UnicodeString.Buffer,
                              UnicodeString.Length);
      }
      else
      {
         PWCHAR buf;
         Wnd->strName.MaximumLength = Wnd->strName.Length = 0;
         buf = Wnd->strName.Buffer;
         Wnd->strName.Buffer = NULL;
         if (buf != NULL)
         {
            DesktopHeapFree(Wnd->head.rpdesk, buf);
         }

         Wnd->strName.Buffer = DesktopHeapAlloc(Wnd->head.rpdesk,
                                                   UnicodeString.Length + sizeof(UNICODE_NULL));
         if (Wnd->strName.Buffer != NULL)
         {
            Wnd->strName.Buffer[UnicodeString.Length / sizeof(WCHAR)] = L'\0';
            RtlCopyMemory(Wnd->strName.Buffer,
                                 UnicodeString.Buffer,
                                 UnicodeString.Length);
            Wnd->strName.MaximumLength = UnicodeString.Length + sizeof(UNICODE_NULL);
            Wnd->strName.Length = UnicodeString.Length;
         }
         else
         {
            SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
            Ret = FALSE;
            goto Exit;
         }
      }
   }
   else
   {
      Wnd->strName.Length = 0;
      if (Wnd->strName.Buffer != NULL)
          Wnd->strName.Buffer[0] = L'\0';
   }

   // HAX! FIXME! Windows does not do this in here!
   // In User32, these are called after: NotifyWinEvent EVENT_OBJECT_NAMECHANGE than
   // RepaintButton, StaticRepaint, NtUserCallHwndLock HWNDLOCK_ROUTINE_REDRAWFRAMEANDHOOK, etc.
   /* Send shell notifications */
   if (!Window->spwndOwner && !IntGetParent(Window))
   {
      co_IntShellHookNotify(HSHELL_REDRAW, (LPARAM) hWnd);
   }

   Ret = TRUE;
Exit:
   if (UnicodeString.Buffer) RtlFreeUnicodeString(&UnicodeString);
   DPRINT("Leave NtUserDefSetText, ret=%i\n", Ret);
   UserLeave();
   return Ret;
}

/*
 * NtUserInternalGetWindowText
 *
 * Status
 *    @implemented
 */

INT APIENTRY
NtUserInternalGetWindowText(HWND hWnd, LPWSTR lpString, INT nMaxCount)
{
   PWINDOW_OBJECT Window;
   PWND Wnd;
   NTSTATUS Status;
   INT Result;
   DECLARE_RETURN(INT);

   DPRINT("Enter NtUserInternalGetWindowText\n");
   UserEnterShared();

   if(lpString && (nMaxCount <= 1))
   {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      RETURN( 0);
   }

   if(!(Window = UserGetWindowObject(hWnd)))
   {
      RETURN( 0);
   }
   Wnd = Window->Wnd;

   Result = Wnd->strName.Length / sizeof(WCHAR);
   if(lpString)
   {
      const WCHAR Terminator = L'\0';
      INT Copy;
      WCHAR *Buffer = (WCHAR*)lpString;

      Copy = min(nMaxCount - 1, Result);
      if(Copy > 0)
      {
         Status = MmCopyToCaller(Buffer, Wnd->strName.Buffer, Copy * sizeof(WCHAR));
         if(!NT_SUCCESS(Status))
         {
            SetLastNtError(Status);
            RETURN( 0);
         }
         Buffer += Copy;
      }

      Status = MmCopyToCaller(Buffer, &Terminator, sizeof(WCHAR));
      if(!NT_SUCCESS(Status))
      {
         SetLastNtError(Status);
         RETURN( 0);
      }

      Result = Copy;
   }

   RETURN( Result);

CLEANUP:
   DPRINT("Leave NtUserInternalGetWindowText, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


BOOL
FASTCALL
IntShowOwnedPopups(PWINDOW_OBJECT OwnerWnd, BOOL fShow )
{
   int count = 0;
   PWINDOW_OBJECT pWnd;
   HWND *win_array;

//   ASSERT(OwnerWnd);

   win_array = IntWinListChildren(UserGetWindowObject(IntGetDesktopWindow()));

   if (!win_array)
      return TRUE;

   while (win_array[count])
      count++;
   while (--count >= 0)
   {
      if (!(pWnd = UserGetWindowObject( win_array[count] )))
         continue;
      if (pWnd->spwndOwner != OwnerWnd)
         continue;

      if (fShow)
      {
         if (pWnd->Wnd->state & WNDS_HIDDENPOPUP)
         {
            /* In Windows, ShowOwnedPopups(TRUE) generates
             * WM_SHOWWINDOW messages with SW_PARENTOPENING,
             * regardless of the state of the owner
             */
            co_IntSendMessage(win_array[count], WM_SHOWWINDOW, SW_SHOWNORMAL, SW_PARENTOPENING);
            continue;
         }
      }
      else
      {
         if (pWnd->Wnd->style & WS_VISIBLE)
         {
            /* In Windows, ShowOwnedPopups(FALSE) generates
             * WM_SHOWWINDOW messages with SW_PARENTCLOSING,
             * regardless of the state of the owner
             */
            co_IntSendMessage(win_array[count], WM_SHOWWINDOW, SW_HIDE, SW_PARENTCLOSING);
            continue;
         }
      }

   }
   ExFreePool( win_array );
   return TRUE;
}

/*
 * NtUserValidateHandleSecure
 *
 * Status
 *    @implemented
 */

BOOL
APIENTRY
NtUserValidateHandleSecure(
   HANDLE handle,
   BOOL Restricted)
{
   if(!Restricted)
   {
     UINT uType;
     {
       PUSER_HANDLE_ENTRY entry;
       if (!(entry = handle_to_entry(gHandleTable, handle )))
       {
          SetLastWin32Error(ERROR_INVALID_HANDLE);
          return FALSE;
       }
       uType = entry->type;
     }
     switch (uType)
     {
       case otWindow:
       {
         PWINDOW_OBJECT Window;
         if ((Window = UserGetWindowObject((HWND) handle))) return TRUE;
         return FALSE;
       }
       case otMenu:
       {
         PMENU_OBJECT Menu;
         if ((Menu = UserGetMenuObject((HMENU) handle))) return TRUE;
         return FALSE;
       }
       case otAccel:
       {
         PACCELERATOR_TABLE Accel;
         if ((Accel = UserGetAccelObject((HACCEL) handle))) return TRUE;
         return FALSE;
       }
       case otCursorIcon:
       {
         PCURICON_OBJECT Cursor;
         if ((Cursor = UserGetCurIconObject((HCURSOR) handle))) return TRUE;
         return FALSE;
       }
       case otHook:
       {
         PHOOK Hook;
         if ((Hook = IntGetHookObject((HHOOK) handle))) return TRUE;
         return FALSE;
       }
       case otMonitor:
       {
         PMONITOR Monitor;
         if ((Monitor = UserGetMonitorObject((HMONITOR) handle))) return TRUE;
         return FALSE;
       }
       case otCallProc:
       {
         WNDPROC_INFO Proc;
         return UserGetCallProcInfo( handle, &Proc );
       }
       default:
         SetLastWin32Error(ERROR_INVALID_HANDLE);
     }
   }
   else
   { /* Is handle entry restricted? */
     UNIMPLEMENTED
   }
   return FALSE;
}


/* EOF */
