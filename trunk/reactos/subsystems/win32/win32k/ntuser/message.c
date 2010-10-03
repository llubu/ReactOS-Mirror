/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Messages
 * FILE:             subsys/win32k/ntuser/message.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISION HISTORY:
 *       06-06-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>

#define NDEBUG
#include <debug.h>

#define PM_BADMSGFLAGS ~((QS_RAWINPUT << 16)|PM_QS_SENDMESSAGE|PM_QS_PAINT|PM_QS_POSTMESSAGE|PM_QS_INPUT|PM_NOYIELD|PM_REMOVE)

typedef struct
{
   UINT uFlags;
   UINT uTimeout;
   ULONG_PTR Result;
}
DOSENDMESSAGE, *PDOSENDMESSAGE;

/* FUNCTIONS *****************************************************************/

NTSTATUS FASTCALL
IntInitMessageImpl(VOID)
{
   return STATUS_SUCCESS;
}

NTSTATUS FASTCALL
IntCleanupMessageImpl(VOID)
{
   return STATUS_SUCCESS;
}

#define MMS_SIZE_WPARAM      -1
#define MMS_SIZE_WPARAMWCHAR -2
#define MMS_SIZE_LPARAMSZ    -3
#define MMS_SIZE_SPECIAL     -4
#define MMS_FLAG_READ        0x01
#define MMS_FLAG_WRITE       0x02
#define MMS_FLAG_READWRITE   (MMS_FLAG_READ | MMS_FLAG_WRITE)
typedef struct tagMSGMEMORY
{
   UINT Message;
   UINT Size;
   INT Flags;
}
MSGMEMORY, *PMSGMEMORY;

static MSGMEMORY MsgMemory[] =
   {
      { WM_CREATE, MMS_SIZE_SPECIAL, MMS_FLAG_READWRITE },
      { WM_DDE_ACK, sizeof(KMDDELPARAM), MMS_FLAG_READ },
      { WM_DDE_EXECUTE, MMS_SIZE_WPARAM, MMS_FLAG_READ },
      { WM_GETMINMAXINFO, sizeof(MINMAXINFO), MMS_FLAG_READWRITE },
      { WM_GETTEXT, MMS_SIZE_WPARAMWCHAR, MMS_FLAG_WRITE },
      { WM_NCCALCSIZE, MMS_SIZE_SPECIAL, MMS_FLAG_READWRITE },
      { WM_NCCREATE, MMS_SIZE_SPECIAL, MMS_FLAG_READWRITE },
      { WM_SETTEXT, MMS_SIZE_LPARAMSZ, MMS_FLAG_READ },
      { WM_STYLECHANGED, sizeof(STYLESTRUCT), MMS_FLAG_READ },
      { WM_STYLECHANGING, sizeof(STYLESTRUCT), MMS_FLAG_READWRITE },
      { WM_COPYDATA, MMS_SIZE_SPECIAL, MMS_FLAG_READ },
      { WM_WINDOWPOSCHANGED, sizeof(WINDOWPOS), MMS_FLAG_READ },
      { WM_WINDOWPOSCHANGING, sizeof(WINDOWPOS), MMS_FLAG_READWRITE },
   };

static PMSGMEMORY FASTCALL
FindMsgMemory(UINT Msg)
{
   PMSGMEMORY MsgMemoryEntry;

   /* See if this message type is present in the table */
   for (MsgMemoryEntry = MsgMemory;
         MsgMemoryEntry < MsgMemory + sizeof(MsgMemory) / sizeof(MSGMEMORY);
         MsgMemoryEntry++)
   {
      if (Msg == MsgMemoryEntry->Message)
      {
         return MsgMemoryEntry;
      }
   }

   return NULL;
}

static UINT FASTCALL
MsgMemorySize(PMSGMEMORY MsgMemoryEntry, WPARAM wParam, LPARAM lParam)
{
   CREATESTRUCTW *Cs;
   PUNICODE_STRING WindowName;
   PUNICODE_STRING ClassName;
   UINT Size = 0;

   _SEH2_TRY
   {
      if (MMS_SIZE_WPARAM == MsgMemoryEntry->Size)
      {
         Size = (UINT)wParam;
      }
      else if (MMS_SIZE_WPARAMWCHAR == MsgMemoryEntry->Size)
      {
         Size = (UINT) (wParam * sizeof(WCHAR));
      }
      else if (MMS_SIZE_LPARAMSZ == MsgMemoryEntry->Size)
      {
         Size = (UINT) ((wcslen((PWSTR) lParam) + 1) * sizeof(WCHAR));
      }
      else if (MMS_SIZE_SPECIAL == MsgMemoryEntry->Size)
      {
         switch(MsgMemoryEntry->Message)
         {
            case WM_CREATE:
            case WM_NCCREATE:
               Cs = (CREATESTRUCTW *) lParam;
               WindowName = (PUNICODE_STRING) Cs->lpszName;
               ClassName = (PUNICODE_STRING) Cs->lpszClass;
               Size = sizeof(CREATESTRUCTW) + WindowName->Length + sizeof(WCHAR);
               if (IS_ATOM(ClassName->Buffer))
               {
                  Size += sizeof(WCHAR) + sizeof(ATOM);
               }
               else
               {
                  Size += sizeof(WCHAR) + ClassName->Length + sizeof(WCHAR);
               }
               break;

            case WM_NCCALCSIZE:
               Size = wParam ? sizeof(NCCALCSIZE_PARAMS) + sizeof(WINDOWPOS) : sizeof(RECT);
               break;

            case WM_COPYDATA:
               Size = sizeof(COPYDATASTRUCT) + ((PCOPYDATASTRUCT)lParam)->cbData;
               break;

            case WM_COPYGLOBALDATA:
               Size = wParam;
               break;

            default:
               ASSERT(FALSE);
               Size = 0;
               break;
         }
      }
      else
      {
         Size = MsgMemoryEntry->Size;
      }
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
      DPRINT1("Exception caught in MsgMemorySize()! Status: 0x%x\n", _SEH2_GetExceptionCode());
      Size = 0;
   }
   _SEH2_END;
   return Size;
}

static NTSTATUS
PackParam(LPARAM *lParamPacked, UINT Msg, WPARAM wParam, LPARAM lParam, BOOL NonPagedPoolNeeded)
{
   NCCALCSIZE_PARAMS *UnpackedNcCalcsize;
   NCCALCSIZE_PARAMS *PackedNcCalcsize;
   CREATESTRUCTW *UnpackedCs;
   CREATESTRUCTW *PackedCs;
   PLARGE_STRING WindowName;
   PUNICODE_STRING ClassName;
   POOL_TYPE PoolType;
   UINT Size;
   PCHAR CsData;

   *lParamPacked = lParam;

    if (NonPagedPoolNeeded)
       PoolType = NonPagedPool;
    else
       PoolType = PagedPool;

   if (WM_NCCALCSIZE == Msg && wParam)
   {

      UnpackedNcCalcsize = (NCCALCSIZE_PARAMS *) lParam;
      PackedNcCalcsize = ExAllocatePoolWithTag(PoolType,
                         sizeof(NCCALCSIZE_PARAMS) + sizeof(WINDOWPOS),
                         TAG_MSG);

      if (NULL == PackedNcCalcsize)
      {
         DPRINT1("Not enough memory to pack lParam\n");
         return STATUS_NO_MEMORY;
      }
      RtlCopyMemory(PackedNcCalcsize, UnpackedNcCalcsize, sizeof(NCCALCSIZE_PARAMS));
      PackedNcCalcsize->lppos = (PWINDOWPOS) (PackedNcCalcsize + 1);
      RtlCopyMemory(PackedNcCalcsize->lppos, UnpackedNcCalcsize->lppos, sizeof(WINDOWPOS));
      *lParamPacked = (LPARAM) PackedNcCalcsize;
   }
   else if (WM_CREATE == Msg || WM_NCCREATE == Msg)
   {
      UnpackedCs = (CREATESTRUCTW *) lParam;
      WindowName = (PLARGE_STRING) UnpackedCs->lpszName;
      ClassName = (PUNICODE_STRING) UnpackedCs->lpszClass;
      Size = sizeof(CREATESTRUCTW) + WindowName->Length + sizeof(WCHAR);
      if (IS_ATOM(ClassName->Buffer))
      {
         Size += sizeof(WCHAR) + sizeof(ATOM);
      }
      else
      {
         Size += sizeof(WCHAR) + ClassName->Length + sizeof(WCHAR);
      }
      PackedCs = ExAllocatePoolWithTag(PoolType, Size, TAG_MSG);
      if (NULL == PackedCs)
      {
         DPRINT1("Not enough memory to pack lParam\n");
         return STATUS_NO_MEMORY;
      }
      RtlCopyMemory(PackedCs, UnpackedCs, sizeof(CREATESTRUCTW));
      CsData = (PCHAR) (PackedCs + 1);
      PackedCs->lpszName = (LPCWSTR) (CsData - (PCHAR) PackedCs);
      RtlCopyMemory(CsData, WindowName->Buffer, WindowName->Length);
      CsData += WindowName->Length;
      *((WCHAR *) CsData) = L'\0';
      CsData += sizeof(WCHAR);
      PackedCs->lpszClass = (LPCWSTR) (CsData - (PCHAR) PackedCs);
      if (IS_ATOM(ClassName->Buffer))
      {
         *((WCHAR *) CsData) = L'A';
         CsData += sizeof(WCHAR);
         *((ATOM *) CsData) = (ATOM)(DWORD_PTR) ClassName->Buffer;
         CsData += sizeof(ATOM);
      }
      else
      {
         *((WCHAR *) CsData) = L'S';
         CsData += sizeof(WCHAR);
         RtlCopyMemory(CsData, ClassName->Buffer, ClassName->Length);
         CsData += ClassName->Length;
         *((WCHAR *) CsData) = L'\0';
         CsData += sizeof(WCHAR);
      }
      ASSERT(CsData == (PCHAR) PackedCs + Size);
      *lParamPacked = (LPARAM) PackedCs;
   }

   else if (PoolType == NonPagedPool)
   {
      PMSGMEMORY MsgMemoryEntry;
      PVOID PackedData;

      MsgMemoryEntry = FindMsgMemory(Msg);

      if ((!MsgMemoryEntry) || (MsgMemoryEntry->Size < 0))
      {
         /* Keep previous behavior */
         return STATUS_SUCCESS;
      }
      PackedData = ExAllocatePoolWithTag(NonPagedPool, MsgMemorySize(MsgMemoryEntry, wParam, lParam), TAG_MSG);
      RtlCopyMemory(PackedData, (PVOID)lParam, MsgMemorySize(MsgMemoryEntry, wParam, lParam));
      *lParamPacked = (LPARAM)PackedData;
   }

   return STATUS_SUCCESS;
}

static NTSTATUS
UnpackParam(LPARAM lParamPacked, UINT Msg, WPARAM wParam, LPARAM lParam, BOOL NonPagedPoolUsed)
{
   NCCALCSIZE_PARAMS *UnpackedParams;
   NCCALCSIZE_PARAMS *PackedParams;
   PWINDOWPOS UnpackedWindowPos;

   if (lParamPacked == lParam)
   {
      return STATUS_SUCCESS;
   }

   if (WM_NCCALCSIZE == Msg && wParam)
   {
      PackedParams = (NCCALCSIZE_PARAMS *) lParamPacked;
      UnpackedParams = (NCCALCSIZE_PARAMS *) lParam;
      UnpackedWindowPos = UnpackedParams->lppos;
      RtlCopyMemory(UnpackedParams, PackedParams, sizeof(NCCALCSIZE_PARAMS));
      UnpackedParams->lppos = UnpackedWindowPos;
      RtlCopyMemory(UnpackedWindowPos, PackedParams + 1, sizeof(WINDOWPOS));
      ExFreePool((PVOID) lParamPacked);

      return STATUS_SUCCESS;
   }
   else if (WM_CREATE == Msg || WM_NCCREATE == Msg)
   {
      ExFreePool((PVOID) lParamPacked);

      return STATUS_SUCCESS;
   }
   else if (NonPagedPoolUsed)
   {
      PMSGMEMORY MsgMemoryEntry;
      MsgMemoryEntry = FindMsgMemory(Msg);
      if (MsgMemoryEntry->Size < 0)
      {
         /* Keep previous behavior */
         return STATUS_INVALID_PARAMETER;
      }

      if (MsgMemory->Flags == MMS_FLAG_READWRITE)
      {
         //RtlCopyMemory((PVOID)lParam, (PVOID)lParamPacked, MsgMemory->Size);
      }
      ExFreePool((PVOID) lParamPacked);
      return STATUS_SUCCESS;
   }

   ASSERT(FALSE);

   return STATUS_INVALID_PARAMETER;
}

static
VOID
FASTCALL
IntCallWndProc
( PWINDOW_OBJECT Window, HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
   BOOL SameThread = FALSE;

   if (Window->pti == ((PTHREADINFO)PsGetCurrentThreadWin32Thread()))
      SameThread = TRUE;

   if ((!SameThread && (Window->pti->fsHooks & HOOKID_TO_FLAG(WH_CALLWNDPROC))) ||
        (SameThread && ISITHOOKED(WH_CALLWNDPROC)) )
   {
      CWPSTRUCT CWP;
      CWP.hwnd    = hWnd;
      CWP.message = Msg;
      CWP.wParam  = wParam;
      CWP.lParam  = lParam;
      co_HOOK_CallHooks( WH_CALLWNDPROC, HC_ACTION, SameThread, (LPARAM)&CWP );
   }
}

static
VOID
FASTCALL
IntCallWndProcRet
( PWINDOW_OBJECT Window, HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, LRESULT *uResult)
{
   BOOL SameThread = FALSE;

   if (Window->pti == ((PTHREADINFO)PsGetCurrentThreadWin32Thread()))
      SameThread = TRUE;

   if ((!SameThread && (Window->pti->fsHooks & HOOKID_TO_FLAG(WH_CALLWNDPROCRET))) ||
        (SameThread && ISITHOOKED(WH_CALLWNDPROCRET)) )
   {
      CWPRETSTRUCT CWPR;
      CWPR.hwnd    = hWnd;
      CWPR.message = Msg;
      CWPR.wParam  = wParam;
      CWPR.lParam  = lParam;
      CWPR.lResult = *uResult;
      co_HOOK_CallHooks( WH_CALLWNDPROCRET, HC_ACTION, SameThread, (LPARAM)&CWPR );
   }
}

LRESULT
FASTCALL
IntDispatchMessage(PMSG pMsg)
{
  LARGE_INTEGER TickCount;
  LONG Time;
  LRESULT retval;
  PMSGMEMORY MsgMemoryEntry;
  INT lParamBufferSize;
  LPARAM lParamPacked;
  PWINDOW_OBJECT Window = NULL;

  if (pMsg->hwnd)
  {
     Window = UserGetWindowObject(pMsg->hwnd);
     if (!Window || !Window->Wnd) return 0;
  }

  if (((pMsg->message == WM_SYSTIMER) ||
       (pMsg->message == WM_TIMER)) &&
      (pMsg->lParam) )
  {
     if (pMsg->message == WM_TIMER)
     {
        if (ValidateTimerCallback(PsGetCurrentThreadWin32Thread(),pMsg->lParam))
        {
           KeQueryTickCount(&TickCount);
           Time = MsqCalculateMessageTime(&TickCount);
           return co_IntCallWindowProc((WNDPROC)pMsg->lParam,
                                        TRUE,
                                        pMsg->hwnd,
                                        WM_TIMER,
                                        pMsg->wParam,
                                        (LPARAM)Time,
                                        sizeof(LPARAM));
        }
        return 0;        
     }
     else
     {
        PTIMER pTimer = FindSystemTimer(pMsg);
        if (pTimer && pTimer->pfn)
        {
           KeQueryTickCount(&TickCount);
           Time = MsqCalculateMessageTime(&TickCount);
           pTimer->pfn(pMsg->hwnd, WM_SYSTIMER, (UINT)pMsg->wParam, Time);
        }
        return 0;
     }
  }
  // Need a window!
  if ( !Window || !Window->Wnd ) return 0;

  /* See if this message type is present in the table */
  MsgMemoryEntry = FindMsgMemory(pMsg->message);
  if ( !MsgMemoryEntry )
  {
     lParamBufferSize = -1;
  }
  else
  {
     lParamBufferSize = MsgMemorySize(MsgMemoryEntry, pMsg->wParam, pMsg->lParam);
  }

  if (! NT_SUCCESS(PackParam(&lParamPacked, pMsg->message, pMsg->wParam, pMsg->lParam, FALSE)))
  {
     DPRINT1("Failed to pack message parameters\n");
     return 0;
  }

  retval = co_IntCallWindowProc( Window->Wnd->lpfnWndProc,
                                !Window->Wnd->Unicode,
                                 pMsg->hwnd,
                                 pMsg->message,
                                 pMsg->wParam,
                                 lParamPacked,
                                 lParamBufferSize);

  if (! NT_SUCCESS(UnpackParam(lParamPacked, pMsg->message, pMsg->wParam, pMsg->lParam, FALSE)))
  {
     DPRINT1("Failed to unpack message parameters\n");
  }

  if (pMsg->message == WM_PAINT)
  {
  /* send a WM_NCPAINT and WM_ERASEBKGND if the non-client area is still invalid */
     HRGN hrgn = IntSysCreateRectRgn( 0, 0, 0, 0 );
     co_UserGetUpdateRgn( Window, hrgn, TRUE );
     REGION_FreeRgnByHandle( hrgn );
  }
  return retval;
}

VOID FASTCALL
co_IntSendHitTestMessages(PUSER_MESSAGE_QUEUE ThreadQueue, LPMSG Msg)
{
   if(!Msg->hwnd || ThreadQueue->CaptureWindow)
   {
      return;
   }

   switch(Msg->message)
   {
      case WM_MOUSEMOVE:
         {
            co_IntSendMessage(Msg->hwnd, WM_SETCURSOR, (WPARAM)Msg->hwnd, MAKELPARAM(HTCLIENT, Msg->message));
            break;
         }
      case WM_NCMOUSEMOVE:
         {
            co_IntSendMessage(Msg->hwnd, WM_SETCURSOR, (WPARAM)Msg->hwnd, MAKELPARAM(Msg->wParam, Msg->message));
            break;
         }
      case WM_LBUTTONDOWN:
      case WM_MBUTTONDOWN:
      case WM_RBUTTONDOWN:
      case WM_XBUTTONDOWN:
      case WM_LBUTTONDBLCLK:
      case WM_MBUTTONDBLCLK:
      case WM_RBUTTONDBLCLK:
      case WM_XBUTTONDBLCLK:
         {
            WPARAM wParam;
            PSYSTEM_CURSORINFO CurInfo;
			CurInfo = IntGetSysCursorInfo();

            wParam = (WPARAM)(CurInfo->ButtonsDown);

            co_IntSendMessage(Msg->hwnd, WM_MOUSEMOVE, wParam, Msg->lParam);
            co_IntSendMessage(Msg->hwnd, WM_SETCURSOR, (WPARAM)Msg->hwnd, MAKELPARAM(HTCLIENT, Msg->message));
            break;
         }
      case WM_NCLBUTTONDOWN:
      case WM_NCMBUTTONDOWN:
      case WM_NCRBUTTONDOWN:
      case WM_NCXBUTTONDOWN:
      case WM_NCLBUTTONDBLCLK:
      case WM_NCMBUTTONDBLCLK:
      case WM_NCRBUTTONDBLCLK:
      case WM_NCXBUTTONDBLCLK:
         {
            co_IntSendMessage(Msg->hwnd, WM_NCMOUSEMOVE, (WPARAM)Msg->wParam, Msg->lParam);
            co_IntSendMessage(Msg->hwnd, WM_SETCURSOR, (WPARAM)Msg->hwnd, MAKELPARAM(Msg->wParam, Msg->message));
            break;
         }
   }
}

BOOL FASTCALL
co_IntActivateWindowMouse(
   PUSER_MESSAGE_QUEUE ThreadQueue,
   LPMSG Msg,
   PWINDOW_OBJECT MsgWindow,
   USHORT *HitTest)
{
   ULONG Result;
   PWINDOW_OBJECT Parent;

   ASSERT_REFS_CO(MsgWindow);

   if(*HitTest == (USHORT)HTTRANSPARENT)
   {
      /* eat the message, search again! */
      return TRUE;
   }

   Parent = IntGetParent(MsgWindow);//fixme: deref retval?

   /* If no parent window, pass MsgWindows HWND as wParam. Fixes bug #3111 */
   Result = co_IntSendMessage(MsgWindow->hSelf,
                              WM_MOUSEACTIVATE,
                              (WPARAM) (Parent ? Parent->hSelf : MsgWindow->hSelf),
                              (LPARAM)MAKELONG(*HitTest, Msg->message)
                             );

   switch (Result)
   {
      case MA_NOACTIVATEANDEAT:
         return TRUE;
      case MA_NOACTIVATE:
         break;
      case MA_ACTIVATEANDEAT:
         co_IntMouseActivateWindow(MsgWindow);
         return TRUE;
      default:
         /* MA_ACTIVATE */
         co_IntMouseActivateWindow(MsgWindow);
         break;
   }

   return FALSE;
}

BOOL FASTCALL
co_IntTranslateMouseMessage(
   PUSER_MESSAGE_QUEUE ThreadQueue,
   LPMSG Msg,
   USHORT *HitTest,
   BOOL Remove)
{
   PWINDOW_OBJECT Window;
   USER_REFERENCE_ENTRY Ref, DesktopRef;

   if(!(Window = UserGetWindowObject(Msg->hwnd)))
   {
      /* let's just eat the message?! */
      return TRUE;
   }

   *HitTest = HTCLIENT;

   UserRefObjectCo(Window, &Ref);

   if ( ThreadQueue == Window->pti->MessageQueue &&
        ThreadQueue->CaptureWindow != Window->hSelf)
   {
      /* only send WM_NCHITTEST messages if we're not capturing the window! */
      if (Remove ) 
      {
         *HitTest = co_IntSendMessage(Window->hSelf, WM_NCHITTEST, 0,
                                      MAKELONG(Msg->pt.x, Msg->pt.y));
      } 
      /* else we are going to see this message again, but then with Remove == TRUE */

      if (*HitTest == (USHORT)HTTRANSPARENT)
      {
         PWINDOW_OBJECT DesktopWindow;
         HWND hDesktop = IntGetDesktopWindow();

         if ((DesktopWindow = UserGetWindowObject(hDesktop)))
         {
            PWINDOW_OBJECT Wnd;

            UserRefObjectCo(DesktopWindow, &DesktopRef);

            co_WinPosWindowFromPoint(DesktopWindow, Window->pti->MessageQueue, &Msg->pt, &Wnd);
            if (Wnd)
            {
               if (Wnd != Window)
               {
                  /* post the message to the other window */
                  Msg->hwnd = Wnd->hSelf;
                  if(!(Wnd->state & WINDOWSTATUS_DESTROYING))
                  {
                     MsqPostMessage(Wnd->pti->MessageQueue, Msg, FALSE,
                                    Msg->message == WM_MOUSEMOVE ? QS_MOUSEMOVE :
                                    QS_MOUSEBUTTON);
                  }

                  /* eat the message */
                  UserDereferenceObject(Wnd);
                  UserDerefObjectCo(DesktopWindow);
                  UserDerefObjectCo(Window);
                  return TRUE;
               }
               UserDereferenceObject(Wnd);
            }

            UserDerefObjectCo(DesktopWindow);
         }
      }
   }

   if ( gspv.bMouseClickLock && 
        ( (Msg->message == WM_LBUTTONUP) ||
          (Msg->message == WM_LBUTTONDOWN) ) )
   {
      if (MsqIsClkLck(Msg, Remove))
      {
        // FIXME: drop the message, hack: use WM_NULL
        Msg->message = WM_NULL;
      }
   }

   if (IS_BTN_MESSAGE(Msg->message, DOWN))
   {
      /* generate double click messages, if necessary */
      if ((((*HitTest) != HTCLIENT) ||
            (Window->Wnd->pcls->style & CS_DBLCLKS)) &&
            MsqIsDblClk(Msg, Remove))
      {
         Msg->message += WM_LBUTTONDBLCLK - WM_LBUTTONDOWN;
      }
   }

   if(Msg->message != WM_MOUSEWHEEL)
   {

      if ((*HitTest) != HTCLIENT)
      {
         Msg->message += WM_NCMOUSEMOVE - WM_MOUSEMOVE;
         if ( (Msg->message == WM_NCRBUTTONUP) &&
              (((*HitTest) == HTCAPTION) || ((*HitTest) == HTSYSMENU)) )
         {
            Msg->message = WM_CONTEXTMENU;
            Msg->wParam = (WPARAM)Window->hSelf;
         }
         else
         {
            Msg->wParam = *HitTest;
         }
         Msg->lParam = MAKELONG(Msg->pt.x, Msg->pt.y);
      }
      else if ( ThreadQueue->MoveSize == NULL &&
                ThreadQueue->MenuOwner == NULL )
      {
         /* NOTE: Msg->pt should remain in screen coordinates. -- FiN */
         Msg->lParam = MAKELONG(
                          Msg->pt.x - (WORD)Window->Wnd->rcClient.left,
                          Msg->pt.y - (WORD)Window->Wnd->rcClient.top);
      }
   }

   UserDerefObjectCo(Window);
   return FALSE;
}

BOOL ProcessMouseMessage(MSG* Msg, USHORT HitTest, UINT RemoveMsg)
{
    MOUSEHOOKSTRUCT MHook;
    EVENTMSG Event;

    Event.message = Msg->message;
    Event.time    = Msg->time;
    Event.hwnd    = Msg->hwnd;
    Event.paramL  = Msg->pt.x;
    Event.paramH  = Msg->pt.y;
    co_HOOK_CallHooks( WH_JOURNALRECORD, HC_ACTION, 0, (LPARAM)&Event);


    MHook.pt           = Msg->pt;
    MHook.hwnd         = Msg->hwnd;
    MHook.wHitTestCode = HitTest;
    MHook.dwExtraInfo  = 0;
    if (co_HOOK_CallHooks( WH_MOUSE,
                           RemoveMsg ? HC_ACTION : HC_NOREMOVE,
                           Msg->message,
                           (LPARAM)&MHook ))
    {
        if (ISITHOOKED(WH_CBT))
        {
            MHook.pt           = Msg->pt;
            MHook.hwnd         = Msg->hwnd;
            MHook.wHitTestCode = HitTest;
            MHook.dwExtraInfo  = 0;
            co_HOOK_CallHooks( WH_CBT,
                               HCBT_CLICKSKIPPED,
                               Msg->message,
                               (LPARAM)&MHook);
        }
        return FALSE;
    }

	return TRUE;
}

BOOL ProcessKeyboardMessage(MSG* Msg, UINT RemoveMsg)
{
   EVENTMSG Event;

   Event.message = Msg->message;
   Event.hwnd    = Msg->hwnd;
   Event.time    = Msg->time;
   Event.paramL  = (Msg->wParam & 0xFF) | (HIWORD(Msg->lParam) << 8);
   Event.paramH  = Msg->lParam & 0x7FFF;
   if (HIWORD(Msg->lParam) & 0x0100) Event.paramH |= 0x8000;
   co_HOOK_CallHooks( WH_JOURNALRECORD, HC_ACTION, 0, (LPARAM)&Event);

    if (co_HOOK_CallHooks( WH_KEYBOARD,
                           RemoveMsg ? HC_ACTION : HC_NOREMOVE,
                           LOWORD(Msg->wParam),
                           Msg->lParam))
    {
        if (ISITHOOKED(WH_CBT))
        {
            /* skip this message */
            co_HOOK_CallHooks( WH_CBT,
                               HCBT_KEYSKIPPED,
                               LOWORD(Msg->wParam),
                               Msg->lParam );
        }
        return FALSE;
    }
	return TRUE;
}
/*
 * Internal version of PeekMessage() doing all the work
 */
BOOL FASTCALL
co_IntPeekMessage( PUSER_MESSAGE Msg,
                   PWINDOW_OBJECT Window,
                   UINT MsgFilterMin,
                   UINT MsgFilterMax,
                   UINT RemoveMsg )
{
   PTHREADINFO pti;
   LARGE_INTEGER LargeTickCount;
   PUSER_MESSAGE_QUEUE ThreadQueue;
   PUSER_MESSAGE Message;
   BOOL Present, RemoveMessages;
   USER_REFERENCE_ENTRY Ref;
   USHORT HitTest;

   /* The queues and order in which they are checked are documented in the MSDN
      article on GetMessage() */

   pti = PsGetCurrentThreadWin32Thread();
   ThreadQueue = pti->MessageQueue;

   /* Inspect RemoveMsg flags */
   /* Note:
       The only flag we process is PM_REMOVE.
       Processing (High word) PM_QS_Xx Is needed. This and MsgFilterXxx can result
       with QS_Xx flags to be used to isolate which message check to test for.
       ATM, we look at all messages and the filters are sent to co_MsqFindMessage
       and there, it is cross checked.
       Example: Wine server/queue.c is_keyboard_msg, check_msg_filter and
                filter_contains_hw_range.
    */
   RemoveMessages = RemoveMsg & PM_REMOVE;

/*
   If no filter is specified, messages are processed in the following order:

    * Sent messages
    * Posted messages
    * Input (hardware) messages and system internal events
    * Sent messages (again)
    * WM_PAINT messages
    * WM_TIMER messages
 */
CheckMessages:

   HitTest = HTNOWHERE;

   Present = FALSE;

   KeQueryTickCount(&LargeTickCount);
   ThreadQueue->LastMsgRead = LargeTickCount.u.LowPart;

   /* Dispatch sent messages here. */
   while (co_MsqDispatchOneSentMessage(ThreadQueue))
      ;

   /* Now look for a quit message. */

   if (ThreadQueue->QuitPosted)
   {
      /* According to the PSDK, WM_QUIT messages are always returned, regardless
         of the filter specified */
      Msg->Msg.hwnd = NULL;
      Msg->Msg.message = WM_QUIT;
      Msg->Msg.wParam = ThreadQueue->QuitExitCode;
      Msg->Msg.lParam = 0;
      Msg->FreeLParam = FALSE;
      if (RemoveMessages)
      {
         ThreadQueue->QuitPosted = FALSE;
      }
      goto MsgExit;
   }

   /* Now check for normal messages. */
   Present = co_MsqFindMessage( ThreadQueue,
                                FALSE,
                                RemoveMessages,
                                Window,
                                MsgFilterMin,
                                MsgFilterMax,
                               &Message );
   if (Present)
   {
      RtlCopyMemory(Msg, Message, sizeof(USER_MESSAGE));
      if (RemoveMessages)
      {
         MsqDestroyMessage(Message);
      }
      goto MessageFound;
   }

   /* Check for hardware events. */
   Present = co_MsqFindMessage( ThreadQueue,
                                TRUE,
                                RemoveMessages,
                                Window,
                                MsgFilterMin,
                                MsgFilterMax,
                               &Message );
   if (Present)
   {
      RtlCopyMemory(Msg, Message, sizeof(USER_MESSAGE));
      if (RemoveMessages)
      {
         MsqDestroyMessage(Message);
      }
      goto MessageFound;
   }

   /* Check for sent messages again. */
   while (co_MsqDispatchOneSentMessage(ThreadQueue))
      ;

   /* Check for paint messages. */
   if ( IntGetPaintMessage( Window,
                            MsgFilterMin,
                            MsgFilterMax,
                            pti,
                            &Msg->Msg,
                            RemoveMessages))
   {
      Msg->FreeLParam = FALSE;
      goto MsgExit;
   }

   if (PostTimerMessages(Window))
      goto CheckMessages;

   if(Present)
   {
MessageFound:

      if(RemoveMessages)
      {
         PWINDOW_OBJECT MsgWindow = NULL;

         /* Mouse message process */

         if( Msg->Msg.hwnd &&
            ( MsgWindow = UserGetWindowObject(Msg->Msg.hwnd) ) &&
             Msg->Msg.message >= WM_MOUSEFIRST &&
             Msg->Msg.message <= WM_MOUSELAST )
         {
            USHORT HitTest;

            UserRefObjectCo(MsgWindow, &Ref);

            if ( co_IntTranslateMouseMessage( ThreadQueue,
                                              &Msg->Msg,
                                              &HitTest,
                                              TRUE))
         /* FIXME - check message filter again, if the message doesn't match anymore,
                    search again */
            {
               UserDerefObjectCo(MsgWindow);
               /* eat the message, search again */
               goto CheckMessages;
            }

            if(ThreadQueue->CaptureWindow == NULL)
            {
               co_IntSendHitTestMessages(ThreadQueue, &Msg->Msg);

               if ( ( Msg->Msg.message != WM_MOUSEMOVE &&
                      Msg->Msg.message != WM_NCMOUSEMOVE ) &&
                     IS_BTN_MESSAGE(Msg->Msg.message, DOWN) &&
                     co_IntActivateWindowMouse(ThreadQueue, &Msg->Msg, MsgWindow, &HitTest) )
               {
                  UserDerefObjectCo(MsgWindow);
                  /* eat the message, search again */
                  goto CheckMessages;
               }
            }

            UserDerefObjectCo(MsgWindow);
         }
         else
         {
            co_IntSendHitTestMessages(ThreadQueue, &Msg->Msg);
         }

//         if(MsgWindow)
//         {
//            UserDereferenceObject(MsgWindow);
//         }

         goto MsgExit;
      }

      if ( ( Msg->Msg.hwnd &&
             Msg->Msg.message >= WM_MOUSEFIRST &&
             Msg->Msg.message <= WM_MOUSELAST ) &&
           co_IntTranslateMouseMessage( ThreadQueue,
                                       &Msg->Msg,
                                       &HitTest,
                                        FALSE) )
    /* FIXME - check message filter again, if the message doesn't match anymore,
               search again */
      {
         /* eat the message, search again */
         goto CheckMessages;
      }

MsgExit:
      pti->rpdesk->htEx = HitTest; /* Now set the capture hit. */

      if ( ISITHOOKED(WH_MOUSE) && IS_MOUSE_MESSAGE(Msg->Msg.message))
      {
          if (!ProcessMouseMessage(&Msg->Msg, HitTest, RemoveMsg))
          {
             return FALSE;
          }
      }

      if ( ISITHOOKED(WH_KEYBOARD) && IS_KBD_MESSAGE(Msg->Msg.message))
      {
          if(!ProcessKeyboardMessage(&Msg->Msg, RemoveMsg))
          {
              return FALSE;
          }
      }
      // The WH_GETMESSAGE hook enables an application to monitor messages about to
      // be returned by the GetMessage or PeekMessage function.
      if (ISITHOOKED(WH_GETMESSAGE))
      {
         //DPRINT1("Peek WH_GETMESSAGE -> %x\n",&Msg);
         co_HOOK_CallHooks( WH_GETMESSAGE, HC_ACTION, RemoveMsg & PM_REMOVE, (LPARAM)&Msg->Msg);
      }
      return TRUE;
   }

   return Present;
}

static NTSTATUS FASTCALL
CopyMsgToKernelMem(MSG *KernelModeMsg, MSG *UserModeMsg, PMSGMEMORY MsgMemoryEntry)
{
   NTSTATUS Status;

   PVOID KernelMem;
   UINT Size;

   *KernelModeMsg = *UserModeMsg;

   /* See if this message type is present in the table */
   if (NULL == MsgMemoryEntry)
   {
      /* Not present, no copying needed */
      return STATUS_SUCCESS;
   }

   /* Determine required size */
   Size = MsgMemorySize(MsgMemoryEntry, UserModeMsg->wParam, UserModeMsg->lParam);

   if (0 != Size)
   {
      /* Allocate kernel mem */
      KernelMem = ExAllocatePoolWithTag(PagedPool, Size, TAG_MSG);
      if (NULL == KernelMem)
      {
         DPRINT1("Not enough memory to copy message to kernel mem\n");
         return STATUS_NO_MEMORY;
      }
      KernelModeMsg->lParam = (LPARAM) KernelMem;

      /* Copy data if required */
      if (0 != (MsgMemoryEntry->Flags & MMS_FLAG_READ))
      {
         Status = MmCopyFromCaller(KernelMem, (PVOID) UserModeMsg->lParam, Size);
         if (! NT_SUCCESS(Status))
         {
            DPRINT1("Failed to copy message to kernel: invalid usermode buffer\n");
            ExFreePoolWithTag(KernelMem, TAG_MSG);
            return Status;
         }
      }
      else
      {
         /* Make sure we don't pass any secrets to usermode */
         RtlZeroMemory(KernelMem, Size);
      }
   }
   else
   {
      KernelModeMsg->lParam = 0;
   }

   return STATUS_SUCCESS;
}

static NTSTATUS FASTCALL
CopyMsgToUserMem(MSG *UserModeMsg, MSG *KernelModeMsg)
{
   NTSTATUS Status;
   PMSGMEMORY MsgMemoryEntry;
   UINT Size;

   /* See if this message type is present in the table */
   MsgMemoryEntry = FindMsgMemory(UserModeMsg->message);
   if (NULL == MsgMemoryEntry)
   {
      /* Not present, no copying needed */
      return STATUS_SUCCESS;
   }

   /* Determine required size */
   Size = MsgMemorySize(MsgMemoryEntry, UserModeMsg->wParam, UserModeMsg->lParam);

   if (0 != Size)
   {
      /* Copy data if required */
      if (0 != (MsgMemoryEntry->Flags & MMS_FLAG_WRITE))
      {
         Status = MmCopyToCaller((PVOID) UserModeMsg->lParam, (PVOID) KernelModeMsg->lParam, Size);
         if (! NT_SUCCESS(Status))
         {
            DPRINT1("Failed to copy message from kernel: invalid usermode buffer\n");
            ExFreePool((PVOID) KernelModeMsg->lParam);
            return Status;
         }
      }

      ExFreePool((PVOID) KernelModeMsg->lParam);
   }

   return STATUS_SUCCESS;
}

static BOOL FASTCALL
co_IntWaitMessage( PWINDOW_OBJECT Window,
                   UINT MsgFilterMin,
                   UINT MsgFilterMax )
{
   PTHREADINFO pti;
   PUSER_MESSAGE_QUEUE ThreadQueue;
   NTSTATUS Status = STATUS_SUCCESS;
   USER_MESSAGE Msg;

   pti = PsGetCurrentThreadWin32Thread();
   ThreadQueue = pti->MessageQueue;

   do
   {
      if ( co_IntPeekMessage( &Msg,
                               Window,
                               MsgFilterMin,
                               MsgFilterMax,
                               PM_NOREMOVE))
      {
         return TRUE;
      }
      /* Nothing found. Wait for new messages. */
      Status = co_MsqWaitForNewMessages( ThreadQueue,
                                         Window,
                                         MsgFilterMin,
                                         MsgFilterMax);
   }
   while ( (STATUS_WAIT_0 <= Status && Status <= STATUS_WAIT_63) ||
           STATUS_TIMEOUT == Status );

   if (!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      DPRINT1("Exit co_IntWaitMessage on error!\n");
   }

   return FALSE;
}

BOOL FASTCALL
co_IntGetPeekMessage( PMSG pMsg,
                      HWND hWnd,
                      UINT MsgFilterMin,
                      UINT MsgFilterMax,
                      UINT RemoveMsg,
                      BOOL bGMSG )
{
   BOOL Present;
   PWINDOW_OBJECT Window;
   USER_MESSAGE Msg;

   if ( hWnd == HWND_TOPMOST ||
        hWnd == HWND_BROADCAST )
      hWnd = HWND_BOTTOM;

   /* Validate input */
   if (hWnd && hWnd != HWND_BOTTOM)
   {
      if (!(Window = UserGetWindowObject(hWnd)))
      {
         if (bGMSG)
            return -1;
         else
            return FALSE;
      }
   }
   else
   {
      Window = (PWINDOW_OBJECT)hWnd;
   }

   if (MsgFilterMax < MsgFilterMin)
   {
      MsgFilterMin = 0;
      MsgFilterMax = 0;
   }

   do
   {
      Present = co_IntPeekMessage( &Msg,
                                    Window,
                                    MsgFilterMin,
                                    MsgFilterMax,
                                    RemoveMsg );
      if (Present)
      {
         RtlCopyMemory( pMsg, &Msg.Msg, sizeof(MSG));

         if (bGMSG)
            return (WM_QUIT != pMsg->message);
         else
            return TRUE;
      }

      if ( bGMSG && !co_IntWaitMessage(Window, MsgFilterMin, MsgFilterMax) )
      {
         return -1;
      }
      else
      {
         if (!(RemoveMsg & PM_NOYIELD))
         {
         // Yield this thread!
            UserLeave();
            ZwYieldExecution();
            UserEnterExclusive();
         // Fall through to fail.
         }
      }
   }
   while( bGMSG && !Present );

   return FALSE;
}

BOOL FASTCALL
UserPostThreadMessage( DWORD idThread,
                       UINT Msg,
                       WPARAM wParam,
                       LPARAM lParam )
{
   MSG Message;
   PETHREAD peThread;
   PTHREADINFO pThread;
   LARGE_INTEGER LargeTickCount;
   NTSTATUS Status;

   DPRINT1("UserPostThreadMessage wParam 0x%x  lParam 0x%x\n", wParam,lParam);

   if (FindMsgMemory(Msg) != 0)
   {
      SetLastWin32Error(ERROR_MESSAGE_SYNC_ONLY );
      return FALSE;
   }

   Status = PsLookupThreadByThreadId((HANDLE)idThread,&peThread);

   if( Status == STATUS_SUCCESS )
   {
      pThread = (PTHREADINFO)peThread->Tcb.Win32Thread;
      if( !pThread ||
          !pThread->MessageQueue ||
         (pThread->TIF_flags & TIF_INCLEANUP))
      {
         ObDereferenceObject( peThread );
         return FALSE;
      }

      Message.hwnd = NULL;
      Message.message = Msg;
      Message.wParam = wParam;
      Message.lParam = lParam;
      Message.pt = gpsi->ptCursor;

      KeQueryTickCount(&LargeTickCount);
      pThread->timeLast = Message.time = MsqCalculateMessageTime(&LargeTickCount);
      MsqPostMessage(pThread->MessageQueue, &Message, FALSE, QS_POSTMESSAGE);
      ObDereferenceObject( peThread );
      return TRUE;
   }
   else
   {
      SetLastNtError( Status );
   }
   return FALSE;
}

BOOL FASTCALL
UserPostMessage( HWND Wnd,
                 UINT Msg,
                 WPARAM wParam,
                 LPARAM lParam )
{
   PTHREADINFO pti;
   MSG Message;
   LARGE_INTEGER LargeTickCount;

   if (FindMsgMemory(Msg) != 0)
   {
      SetLastWin32Error(ERROR_MESSAGE_SYNC_ONLY );
      return FALSE;
   }

   if (!Wnd) 
      return UserPostThreadMessage( PtrToInt(PsGetCurrentThreadId()),
                                    Msg,
                                    wParam,
                                    lParam);

   if (Wnd == HWND_BROADCAST)
   {
      HWND *List;
      PWINDOW_OBJECT DesktopWindow;
      ULONG i;

      DesktopWindow = UserGetWindowObject(IntGetDesktopWindow());
      List = IntWinListChildren(DesktopWindow);

      if (List != NULL)
      {
         for (i = 0; List[i]; i++)
            UserPostMessage(List[i], Msg, wParam, lParam);
         ExFreePool(List);
      }
   }
   else
   {
      PWINDOW_OBJECT Window;

      Window = UserGetWindowObject(Wnd);
      if ( !Window || !Window->Wnd )
      {
         return FALSE;
      }

      pti = Window->Wnd->head.pti;
      if ( pti->TIF_flags & TIF_INCLEANUP )
      {
         DPRINT1("Attempted to post message to window 0x%x when the thread is in cleanup!\n", Wnd);
         return FALSE;
      }

      if ( Window->state & WINDOWSTATUS_DESTROYING )
      {
         DPRINT1("Attempted to post message to window 0x%x that is being destroyed!\n", Wnd);
         /* FIXME - last error code? */
         return FALSE;
      }

      if (WM_QUIT == Msg)
      {
          MsqPostQuitMessage(Window->pti->MessageQueue, wParam);
      }
      else
      {
         Message.hwnd = Wnd;
         Message.message = Msg;
         Message.wParam = wParam;
         Message.lParam = lParam;
         Message.pt = gpsi->ptCursor;
         KeQueryTickCount(&LargeTickCount);
         pti->timeLast = Message.time = MsqCalculateMessageTime(&LargeTickCount);
         MsqPostMessage(Window->pti->MessageQueue, &Message, FALSE, QS_POSTMESSAGE);
      }
   }
   return TRUE;
}


LRESULT FASTCALL
co_IntSendMessage( HWND hWnd,
                   UINT Msg,
                   WPARAM wParam,
                   LPARAM lParam )
{
   ULONG_PTR Result = 0;
   if(co_IntSendMessageTimeout(hWnd, Msg, wParam, lParam, SMTO_NORMAL, 0, &Result))
   {
      return (LRESULT)Result;
   }
   return 0;
}

static
LRESULT FASTCALL
co_IntSendMessageTimeoutSingle( HWND hWnd,
                                UINT Msg,
                                WPARAM wParam,
                                LPARAM lParam,
                                UINT uFlags,
                                UINT uTimeout,
                                ULONG_PTR *uResult )
{
   ULONG_PTR Result;
   NTSTATUS Status;
   PWINDOW_OBJECT Window = NULL;
   PMSGMEMORY MsgMemoryEntry;
   INT lParamBufferSize;
   LPARAM lParamPacked;
   PTHREADINFO Win32Thread;
   DECLARE_RETURN(LRESULT);
   USER_REFERENCE_ENTRY Ref;

   if (!(Window = UserGetWindowObject(hWnd)))
   {
       RETURN( FALSE);
   }

   UserRefObjectCo(Window, &Ref);

   Win32Thread = PsGetCurrentThreadWin32Thread();

   IntCallWndProc( Window, hWnd, Msg, wParam, lParam);

   if ( NULL != Win32Thread &&
        Window->pti->MessageQueue == Win32Thread->MessageQueue)
   {
      if (Win32Thread->TIF_flags & TIF_INCLEANUP)
      {
         /* Never send messages to exiting threads */
          RETURN( FALSE);
      }

      /* See if this message type is present in the table */
      MsgMemoryEntry = FindMsgMemory(Msg);
      if (NULL == MsgMemoryEntry)
      {
         lParamBufferSize = -1;
      }
      else
      {
         lParamBufferSize = MsgMemorySize(MsgMemoryEntry, wParam, lParam);
      }

      if (! NT_SUCCESS(PackParam(&lParamPacked, Msg, wParam, lParam, FALSE)))
      {
          DPRINT1("Failed to pack message parameters\n");
          RETURN( FALSE);
      }

      Result = (ULONG_PTR)co_IntCallWindowProc( Window->Wnd->lpfnWndProc,
                                               !Window->Wnd->Unicode,
                                                hWnd,
                                                Msg,
                                                wParam,
                                                lParamPacked,
                                                lParamBufferSize );
      if(uResult)
      {
         *uResult = Result;
      }

      IntCallWndProcRet( Window, hWnd, Msg, wParam, lParam, (LRESULT *)uResult);

      if (! NT_SUCCESS(UnpackParam(lParamPacked, Msg, wParam, lParam, FALSE)))
      {
         DPRINT1("Failed to unpack message parameters\n");
         RETURN( TRUE);
      }

      RETURN( TRUE);
   }

   if (uFlags & SMTO_ABORTIFHUNG && MsqIsHung(Window->pti->MessageQueue))
   {
      /* FIXME - Set a LastError? */
      RETURN( FALSE);
   }

   if (Window->state & WINDOWSTATUS_DESTROYING)
   {
      /* FIXME - last error? */
      DPRINT1("Attempted to send message to window 0x%x that is being destroyed!\n", hWnd);
      RETURN( FALSE);
   }

   do
   {
      Status = co_MsqSendMessage( Window->pti->MessageQueue,
                                                       hWnd,
                                                        Msg,
                                                     wParam,
                                                     lParam,
                                                   uTimeout,
                                      (uFlags & SMTO_BLOCK),
                                                 MSQ_NORMAL,
                                                    uResult );
   }
   while ((STATUS_TIMEOUT == Status) &&
          (uFlags & SMTO_NOTIMEOUTIFNOTHUNG) &&
          !MsqIsHung(Window->pti->MessageQueue));

   IntCallWndProcRet( Window, hWnd, Msg, wParam, lParam, (LRESULT *)uResult);

   if (STATUS_TIMEOUT == Status)
   {
/*
   MSDN says:
      Microsoft Windows 2000: If GetLastError returns zero, then the function
      timed out.
      XP+ : If the function fails or times out, the return value is zero.
      To get extended error information, call GetLastError. If GetLastError
      returns ERROR_TIMEOUT, then the function timed out.
 */
      SetLastWin32Error(ERROR_TIMEOUT);
      RETURN( FALSE);
   }
   else if (! NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      RETURN( FALSE);
   }

   RETURN( TRUE);

CLEANUP:
   if (Window) UserDerefObjectCo(Window);
   END_CLEANUP;
}

LRESULT FASTCALL
co_IntSendMessageTimeout( HWND hWnd,
                          UINT Msg,
                          WPARAM wParam,
                          LPARAM lParam,
                          UINT uFlags,
                          UINT uTimeout,
                          ULONG_PTR *uResult )
{
   PWINDOW_OBJECT DesktopWindow;
   HWND *Children;
   HWND *Child;

   if (HWND_BROADCAST != hWnd)
   {
      return co_IntSendMessageTimeoutSingle(hWnd, Msg, wParam, lParam, uFlags, uTimeout, uResult);
   }

   DesktopWindow = UserGetWindowObject(IntGetDesktopWindow());
   if (NULL == DesktopWindow)
   {
      SetLastWin32Error(ERROR_INTERNAL_ERROR);
      return 0;
   }

   Children = IntWinListChildren(DesktopWindow);
   if (NULL == Children)
   {
      return 0;
   }

   for (Child = Children; NULL != *Child; Child++)
   {
      co_IntSendMessageTimeoutSingle(*Child, Msg, wParam, lParam, uFlags, uTimeout, uResult);
   }

   ExFreePool(Children);

   return (LRESULT) TRUE;
}

LRESULT FASTCALL co_IntSendMessageNoWait(HWND hWnd,
                                         UINT Msg,
                                         WPARAM wParam,
                                         LPARAM lParam)
{
   ULONG_PTR Result = 0;
   co_IntSendMessageWithCallBack(hWnd,
                                 Msg,
                                 wParam,
                                 lParam,
                                 NULL,
                                 0,
                                 &Result);
   return Result;
}

LRESULT FASTCALL
co_IntSendMessageWithCallBack( HWND hWnd,
                               UINT Msg,
                               WPARAM wParam,
                               LPARAM lParam,
                               SENDASYNCPROC CompletionCallback,
                               ULONG_PTR CompletionCallbackContext,
                               ULONG_PTR *uResult)
{
   ULONG_PTR Result;
   PWINDOW_OBJECT Window = NULL;
   PMSGMEMORY MsgMemoryEntry;
   INT lParamBufferSize;
   LPARAM lParamPacked;
   PTHREADINFO Win32Thread;
   DECLARE_RETURN(LRESULT);
   USER_REFERENCE_ENTRY Ref;
   PUSER_SENT_MESSAGE Message;

   if (!(Window = UserGetWindowObject(hWnd)))
   {
       RETURN(FALSE);
   }

   UserRefObjectCo(Window, &Ref);

   if (Window->state & WINDOWSTATUS_DESTROYING)
   {
      /* FIXME - last error? */
      DPRINT1("Attempted to send message to window 0x%x that is being destroyed!\n", hWnd);
      RETURN(FALSE);
   }

   Win32Thread = PsGetCurrentThreadWin32Thread();

   IntCallWndProc( Window, hWnd, Msg, wParam, lParam);

   if (Win32Thread == NULL)
   {
     ASSERT(FALSE);
     RETURN(FALSE);
   }

   if (Win32Thread->TIF_flags & TIF_INCLEANUP)
   {
      /* Never send messages to exiting threads */
       RETURN(FALSE);
   }

   /* See if this message type is present in the table */
   MsgMemoryEntry = FindMsgMemory(Msg);
   if (NULL == MsgMemoryEntry)
   {
      lParamBufferSize = -1;
   }
   else
   {
      lParamBufferSize = MsgMemorySize(MsgMemoryEntry, wParam, lParam);
   }

   if (! NT_SUCCESS(PackParam(&lParamPacked, Msg, wParam, lParam, Window->pti->MessageQueue != Win32Thread->MessageQueue)))
   {
       DPRINT1("Failed to pack message parameters\n");
       RETURN( FALSE);
   }

   /* If this is not a callback and it can be sent now, then send it. */
   if ((Window->pti->MessageQueue == Win32Thread->MessageQueue) && (CompletionCallback == NULL))
   {

      Result = (ULONG_PTR)co_IntCallWindowProc( Window->Wnd->lpfnWndProc,
                                               !Window->Wnd->Unicode,
                                                hWnd,
                                                Msg,
                                                wParam,
                                                lParamPacked,
                                                lParamBufferSize );
      if(uResult)
      {
         *uResult = Result;
      }
   }

   IntCallWndProcRet( Window, hWnd, Msg, wParam, lParam, (LRESULT *)uResult);

   if ((Window->pti->MessageQueue == Win32Thread->MessageQueue) && (CompletionCallback == NULL))
   {
      if (! NT_SUCCESS(UnpackParam(lParamPacked, Msg, wParam, lParam, FALSE)))
      {
         DPRINT1("Failed to unpack message parameters\n");
      }
      RETURN(TRUE);
   }

   if(!(Message = ExAllocatePoolWithTag(NonPagedPool, sizeof(USER_SENT_MESSAGE), TAG_USRMSG)))
   {
      DPRINT1("MsqSendMessage(): Not enough memory to allocate a message");
      return STATUS_INSUFFICIENT_RESOURCES;
   }

   Message->Msg.hwnd = hWnd;
   Message->Msg.message = Msg;
   Message->Msg.wParam = wParam;
   Message->Msg.lParam = lParamPacked;
   Message->CompletionEvent = NULL;
   Message->Result = 0;
   Message->SenderQueue = NULL; //Win32Thread->MessageQueue;

   IntReferenceMessageQueue(Window->pti->MessageQueue);
   Message->CompletionCallback = CompletionCallback;
   Message->CompletionCallbackContext = CompletionCallbackContext;
   Message->HookMessage = MSQ_NORMAL | MSQ_SENTNOWAIT;
   Message->HasPackedLParam = (lParamBufferSize > 0);

   InsertTailList(&Window->pti->MessageQueue->SentMessagesListHead, &Message->ListEntry);
   IntDereferenceMessageQueue(Window->pti->MessageQueue);

   RETURN(TRUE);

CLEANUP:
   if (Window) UserDerefObjectCo(Window);
   END_CLEANUP;
}

/* This function posts a message if the destination's message queue belongs to
   another thread, otherwise it sends the message. It does not support broadcast
   messages! */
LRESULT FASTCALL
co_IntPostOrSendMessage( HWND hWnd,
                         UINT Msg,
                         WPARAM wParam,
                         LPARAM lParam )
{
   ULONG_PTR Result;
   PTHREADINFO pti;
   PWINDOW_OBJECT Window;

   if ( hWnd == HWND_BROADCAST )
   {
      return 0;
   }

   if(!(Window = UserGetWindowObject(hWnd)))
   {
      return 0;
   }

   pti = PsGetCurrentThreadWin32Thread();

   if ( Window->pti->MessageQueue != pti->MessageQueue &&
        FindMsgMemory(Msg) == 0 )
   {
      Result = UserPostMessage(hWnd, Msg, wParam, lParam);
   }
   else
   {
      if ( !co_IntSendMessageTimeoutSingle(hWnd, Msg, wParam, lParam, SMTO_NORMAL, 0, &Result) )
      {
         Result = 0;
      }
   }

   return (LRESULT)Result;
}

LRESULT FASTCALL
co_IntDoSendMessage( HWND hWnd,
                     UINT Msg,
                     WPARAM wParam,
                     LPARAM lParam,
                     PDOSENDMESSAGE dsm,
                     PNTUSERSENDMESSAGEINFO UnsafeInfo )
{
   PTHREADINFO pti;
   LRESULT Result = TRUE;
   NTSTATUS Status;
   PWINDOW_OBJECT Window = NULL;
   NTUSERSENDMESSAGEINFO Info;
   MSG UserModeMsg;
   MSG KernelModeMsg;
   PMSGMEMORY MsgMemoryEntry;

   RtlZeroMemory(&Info, sizeof(NTUSERSENDMESSAGEINFO));

   /* FIXME: Call hooks. */
   if (HWND_BROADCAST != hWnd)
   {
      Window = UserGetWindowObject(hWnd);
      if ( !Window || !Window->Wnd )
      {
         /* Tell usermode to not touch this one */
         Info.HandledByKernel = TRUE;
         MmCopyToCaller(UnsafeInfo, &Info, sizeof(NTUSERSENDMESSAGEINFO));
         return 0;
      }
   }

   /* Check for an exiting window. */
   if (Window && Window->state & WINDOWSTATUS_DESTROYING)
   {
      DPRINT1("co_IntDoSendMessage Window Exiting!\n");
   }

   /* See if the current thread can handle the message */
   pti = PsGetCurrentThreadWin32Thread();

   // This is checked in user mode!!!!!!!
   if ( HWND_BROADCAST != hWnd &&
        NULL != pti &&
        Window->pti->MessageQueue == pti->MessageQueue &&
       !ISITHOOKED(WH_CALLWNDPROC) &&
       !ISITHOOKED(WH_CALLWNDPROCRET) &&
        ( Msg < WM_DDE_FIRST || Msg > WM_DDE_LAST ) )
   {
      /* Gather the information usermode needs to call the window proc directly */
      Info.HandledByKernel = FALSE;

      Status = MmCopyFromCaller(&(Info.Ansi), &(UnsafeInfo->Ansi),
                                sizeof(BOOL));
      if (! NT_SUCCESS(Status))
      {
         Info.Ansi = ! Window->Wnd->Unicode;
      }

      Info.Ansi = !Window->Wnd->Unicode;
      Info.Proc = Window->Wnd->lpfnWndProc;
   }
   else
   {
      /* Must be handled by other thread */
//      if (HWND_BROADCAST != hWnd)
//      {
//         UserDereferenceObject(Window);
//      }
      Info.HandledByKernel = TRUE;
      UserModeMsg.hwnd = hWnd;
      UserModeMsg.message = Msg;
      UserModeMsg.wParam = wParam;
      UserModeMsg.lParam = lParam;
      MsgMemoryEntry = FindMsgMemory(UserModeMsg.message);

      Status = CopyMsgToKernelMem(&KernelModeMsg, &UserModeMsg, MsgMemoryEntry);
      if (! NT_SUCCESS(Status))
      {
         MmCopyToCaller(UnsafeInfo, &Info, sizeof(NTUSERSENDMESSAGEINFO));
         SetLastWin32Error(ERROR_INVALID_PARAMETER);
         return (dsm ? 0 : -1);
      }

      if(!dsm)
      {
         Result = co_IntSendMessage( KernelModeMsg.hwnd,
                                     KernelModeMsg.message,
                                     KernelModeMsg.wParam,
                                     KernelModeMsg.lParam );
      }
      else
      {
         Result = co_IntSendMessageTimeout( KernelModeMsg.hwnd,
                                            KernelModeMsg.message,
                                            KernelModeMsg.wParam,
                                            KernelModeMsg.lParam,
                                            dsm->uFlags,
                                            dsm->uTimeout,
                                           &dsm->Result );
      }

      Status = CopyMsgToUserMem(&UserModeMsg, &KernelModeMsg);
      if (! NT_SUCCESS(Status))
      {
         MmCopyToCaller(UnsafeInfo, &Info, sizeof(NTUSERSENDMESSAGEINFO));
         SetLastWin32Error(ERROR_INVALID_PARAMETER);
         return(dsm ? 0 : -1);
      }
   }

   Status = MmCopyToCaller(UnsafeInfo, &Info, sizeof(NTUSERSENDMESSAGEINFO));
   if (! NT_SUCCESS(Status))
   {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
   }

   return (LRESULT)Result;
}


BOOL FASTCALL
UserSendNotifyMessage( HWND hWnd,
                       UINT Msg,
                       WPARAM wParam,
                       LPARAM lParam )
{
   BOOL Result = TRUE;

   if (FindMsgMemory(Msg) != 0)
   {
      SetLastWin32Error(ERROR_MESSAGE_SYNC_ONLY );
      return FALSE;
   }

   // Basicly the same as IntPostOrSendMessage
   if (hWnd == HWND_BROADCAST) //Handle Broadcast
   {
      HWND *List;
      PWINDOW_OBJECT DesktopWindow;
      ULONG i;

      DesktopWindow = UserGetWindowObject(IntGetDesktopWindow());
      List = IntWinListChildren(DesktopWindow);

      if (List != NULL)
      {
         for (i = 0; List[i]; i++)
         {
            UserSendNotifyMessage(List[i], Msg, wParam, lParam);
         }
         ExFreePool(List);
      }
   }
   else
   {
     ULONG_PTR PResult;
     PTHREADINFO pti;
     PWINDOW_OBJECT Window;

      if ( !(Window = UserGetWindowObject(hWnd)) ) return FALSE;

      pti = PsGetCurrentThreadWin32Thread();

      if (Window->pti->MessageQueue != pti->MessageQueue)
      { // Send message w/o waiting for it.
         Result = UserPostMessage(hWnd, Msg, wParam, lParam);
      }
      else
      { // Handle message and callback.
         Result = co_IntSendMessageTimeoutSingle( hWnd,
                                                  Msg,
                                                  wParam,
                                                  lParam,
                                                  SMTO_NORMAL,
                                                  0,
                                                 &PResult );
      }
   }
   return Result;
}


DWORD APIENTRY
IntGetQueueStatus(BOOL ClearChanges)
{
   PTHREADINFO pti;
   PUSER_MESSAGE_QUEUE Queue;
   DWORD Result;
   DECLARE_RETURN(DWORD);

   DPRINT("Enter IntGetQueueStatus\n");

   pti = PsGetCurrentThreadWin32Thread();
   Queue = pti->MessageQueue;

   Result = MAKELONG(Queue->QueueBits, Queue->ChangedBits);
   if (ClearChanges)
   {
      Queue->ChangedBits = 0;
   }

   RETURN(Result);

CLEANUP:
   DPRINT("Leave IntGetQueueStatus, ret=%i\n",_ret_);
   END_CLEANUP;
}

BOOL APIENTRY
IntInitMessagePumpHook()
{
   if (((PTHREADINFO)PsGetCurrentThread()->Tcb.Win32Thread)->pcti)
   {
     ((PTHREADINFO)PsGetCurrentThread()->Tcb.Win32Thread)->pcti->dwcPumpHook++;
     return TRUE;
   }
   return FALSE;
}

BOOL APIENTRY
IntUninitMessagePumpHook()
{
   if (((PTHREADINFO)PsGetCurrentThread()->Tcb.Win32Thread)->pcti)
   {
      if (((PTHREADINFO)PsGetCurrentThread()->Tcb.Win32Thread)->pcti->dwcPumpHook <= 0)
      {
         return FALSE;
      }
      ((PTHREADINFO)PsGetCurrentThread()->Tcb.Win32Thread)->pcti->dwcPumpHook--;
      return TRUE;
   }
   return FALSE;
}

/** Functions ******************************************************************/

BOOL APIENTRY
NtUserPostMessage(HWND hWnd,
                  UINT Msg,
                  WPARAM wParam,
                  LPARAM lParam)
{
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserPostMessage\n");
   UserEnterExclusive();

   RETURN( UserPostMessage(hWnd, Msg, wParam, lParam));

CLEANUP:
   DPRINT("Leave NtUserPostMessage, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

BOOL APIENTRY
NtUserPostThreadMessage(DWORD idThread,
                        UINT Msg,
                        WPARAM wParam,
                        LPARAM lParam)
{
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserPostThreadMessage\n");
   UserEnterExclusive();

   RETURN( UserPostThreadMessage( idThread,
                                  Msg,
                                  wParam,
                                  lParam));

CLEANUP:
   DPRINT("Leave NtUserPostThreadMessage, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

DWORD APIENTRY
NtUserQuerySendMessage(DWORD Unknown0)
{
   UNIMPLEMENTED;

   return 0;
}


////////// API on the way out!
LRESULT APIENTRY
NtUserSendMessageTimeout( HWND hWnd,
                          UINT Msg,
                          WPARAM wParam,
                          LPARAM lParam,
                          UINT uFlags,
                          UINT uTimeout,
                          ULONG_PTR *uResult,
                          PNTUSERSENDMESSAGEINFO UnsafeInfo )
{
   DOSENDMESSAGE dsm;
   LRESULT Result;
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserSendMessageTimeout\n");
   UserEnterExclusive();

   dsm.uFlags = uFlags;
   dsm.uTimeout = uTimeout;
   Result = co_IntDoSendMessage(hWnd, Msg, wParam, lParam, &dsm, UnsafeInfo);
   if(uResult != NULL && Result != 0)
   {
      NTSTATUS Status;

      Status = MmCopyToCaller(uResult, &dsm.Result, sizeof(ULONG_PTR));
      if(!NT_SUCCESS(Status))
      {
         SetLastWin32Error(ERROR_INVALID_PARAMETER);
         RETURN( FALSE);
      }
   }
   RETURN( Result);

CLEANUP:
   DPRINT("Leave NtUserSendMessageTimeout, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

LRESULT APIENTRY
NtUserSendMessage( HWND Wnd,
                   UINT Msg,
                   WPARAM wParam,
                   LPARAM lParam,
                   PNTUSERSENDMESSAGEINFO UnsafeInfo )
{
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserSendMessage\n");
   UserEnterExclusive();

   RETURN(co_IntDoSendMessage(Wnd, Msg, wParam, lParam, NULL, UnsafeInfo));

CLEANUP:
   DPRINT("Leave NtUserSendMessage, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}
//////////

BOOL APIENTRY
NtUserWaitMessage(VOID)
{
   DECLARE_RETURN(BOOL);

   DPRINT("EnterNtUserWaitMessage\n");
   UserEnterExclusive();

   RETURN(co_IntWaitMessage(NULL, 0, 0));

CLEANUP:
   DPRINT("Leave NtUserWaitMessage, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


BOOL APIENTRY
NtUserGetMessage( PNTUSERGETMESSAGEINFO UnsafeInfo,
                  HWND hWnd,
                  UINT MsgFilterMin,
                  UINT MsgFilterMax )
/*
 * FUNCTION: Get a message from the calling thread's message queue.
 * ARGUMENTS:
 *      UnsafeMsg - Pointer to the structure which receives the returned message.
 *      Wnd - Window whose messages are to be retrieved.
 *      MsgFilterMin - Integer value of the lowest message value to be
 *                     retrieved.
 *      MsgFilterMax - Integer value of the highest message value to be
 *                     retrieved.
 */
{
   BOOL GotMessage;
   NTUSERGETMESSAGEINFO Info;
   NTSTATUS Status;
   /* FIXME: if initialization is removed, gcc complains that this may be used before initialization. Please review */
   PWINDOW_OBJECT Window = NULL;
   PMSGMEMORY MsgMemoryEntry;
   PVOID UserMem;
   UINT Size;
   USER_MESSAGE Msg;
   DECLARE_RETURN(BOOL);
//   USER_REFERENCE_ENTRY Ref;

   DPRINT("Enter NtUserGetMessage\n");
   UserEnterExclusive();

   /* Validate input */
   if (hWnd && !(Window = UserGetWindowObject(hWnd)))
   {
      RETURN(-1);
   }

//   if (Window) UserRefObjectCo(Window, &Ref);

   if (MsgFilterMax < MsgFilterMin)
   {
      MsgFilterMin = 0;
      MsgFilterMax = 0;
   }

   do
   {
      GotMessage = co_IntPeekMessage(&Msg, Window, MsgFilterMin, MsgFilterMax, PM_REMOVE);
      if (GotMessage)
      {
         Info.Msg = Msg.Msg;
         /* See if this message type is present in the table */
         MsgMemoryEntry = FindMsgMemory(Info.Msg.message);
         if (NULL == MsgMemoryEntry)
         {
            /* Not present, no copying needed */
            Info.LParamSize = 0;
         }
         else
         {
            /* Determine required size */
            Size = MsgMemorySize(MsgMemoryEntry, Info.Msg.wParam,
                                 Info.Msg.lParam);
            /* Allocate required amount of user-mode memory */
            Info.LParamSize = Size;
            UserMem = NULL;
            Status = ZwAllocateVirtualMemory(NtCurrentProcess(), &UserMem, 0,
                                             &Info.LParamSize, MEM_COMMIT, PAGE_READWRITE);

            if (! NT_SUCCESS(Status))
            {
               SetLastNtError(Status);
               RETURN( (BOOL) -1);
            }
            /* Transfer lParam data to user-mode mem */
            Status = MmCopyToCaller(UserMem, (PVOID) Info.Msg.lParam, Size);
            if (! NT_SUCCESS(Status))
            {
               ZwFreeVirtualMemory(NtCurrentProcess(), (PVOID *) &UserMem,
                                   &Info.LParamSize, MEM_DECOMMIT);
               SetLastNtError(Status);
               RETURN( (BOOL) -1);
            }
            Info.Msg.lParam = (LPARAM) UserMem;
         }
         if (Msg.FreeLParam && 0 != Msg.Msg.lParam)
         {
            ExFreePool((void *) Msg.Msg.lParam);
         }
         Status = MmCopyToCaller(UnsafeInfo, &Info, sizeof(NTUSERGETMESSAGEINFO));
         if (! NT_SUCCESS(Status))
         {
            SetLastNtError(Status);
            RETURN( (BOOL) -1);
         }
      }
      else if (! co_IntWaitMessage(Window, MsgFilterMin, MsgFilterMax))
      {
         RETURN( (BOOL) -1);
      }
   }
   while (! GotMessage);

   RETURN( WM_QUIT != Info.Msg.message);

CLEANUP:
//   if (Window) UserDerefObjectCo(Window);

   DPRINT("Leave NtUserGetMessage\n");
   UserLeave();
   END_CLEANUP;
}


BOOL
APIENTRY
NtUserGetMessageX(
   PMSG pMsg,
   HWND hWnd,
   UINT MsgFilterMin,
   UINT MsgFilterMax)
{
   MSG Msg;
   BOOL Ret = FALSE;
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserGetMessage\n");
   UserEnterExclusive();

   if ( (MsgFilterMin|MsgFilterMax) & ~WM_MAXIMUM )
   {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      RETURN( Ret);
   }

   RtlZeroMemory(&Msg, sizeof(MSG));

   Ret = co_IntGetPeekMessage(&Msg, hWnd, MsgFilterMin, MsgFilterMax, PM_REMOVE, TRUE);

   if (Ret)
   {
      _SEH2_TRY
      {
         ProbeForWrite(pMsg, sizeof(MSG), 1);
         RtlCopyMemory(pMsg, &Msg, sizeof(MSG));
      }
      _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
      {
         SetLastNtError(_SEH2_GetExceptionCode());
         Ret = FALSE;
      }
      _SEH2_END;
   }
   RETURN( Ret);

CLEANUP:
   DPRINT("Leave NtUserGetMessage\n");
   UserLeave();
   END_CLEANUP;
}

BOOL APIENTRY
NtUserPeekMessage(PNTUSERGETMESSAGEINFO UnsafeInfo,
                  HWND hWnd,
                  UINT MsgFilterMin,
                  UINT MsgFilterMax,
                  UINT RemoveMsg)
{
   NTSTATUS Status;
   BOOL Present;
   NTUSERGETMESSAGEINFO Info;
   PWINDOW_OBJECT Window;
   PMSGMEMORY MsgMemoryEntry;
   PVOID UserMem;
   UINT Size;
   USER_MESSAGE Msg;
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserPeekMessage\n");
   UserEnterExclusive();

   if (hWnd == (HWND)-1 || hWnd == (HWND)0x0000FFFF || hWnd == (HWND)0xFFFFFFFF)
      hWnd = (HWND)1;

   /* Validate input */
   if (hWnd && hWnd != (HWND)1)
   {
      if (!(Window = UserGetWindowObject(hWnd)))
      {
         RETURN(-1);
      }
   }
   else
   {
      Window = (PWINDOW_OBJECT)hWnd;
   }

   if (MsgFilterMax < MsgFilterMin)
   {
      MsgFilterMin = 0;
      MsgFilterMax = 0;
   }

   Present = co_IntPeekMessage(&Msg, Window, MsgFilterMin, MsgFilterMax, RemoveMsg);
   if (Present)
   {

      Info.Msg = Msg.Msg;
      /* See if this message type is present in the table */
      MsgMemoryEntry = FindMsgMemory(Info.Msg.message);
      if (NULL == MsgMemoryEntry)
      {
         /* Not present, no copying needed */
         Info.LParamSize = 0;
      }
      else
      {
         /* Determine required size */
         Size = MsgMemorySize(MsgMemoryEntry, Info.Msg.wParam,
                              Info.Msg.lParam);
         /* Allocate required amount of user-mode memory */
         Info.LParamSize = Size;
         UserMem = NULL;
         Status = ZwAllocateVirtualMemory(NtCurrentProcess(), &UserMem, 0,
                                          &Info.LParamSize, MEM_COMMIT, PAGE_READWRITE);
         if (! NT_SUCCESS(Status))
         {
            SetLastNtError(Status);
            RETURN( (BOOL) -1);
         }
         /* Transfer lParam data to user-mode mem */
         Status = MmCopyToCaller(UserMem, (PVOID) Info.Msg.lParam, Size);
         if (! NT_SUCCESS(Status))
         {
            ZwFreeVirtualMemory(NtCurrentProcess(), (PVOID *) &UserMem,
                                &Info.LParamSize, MEM_RELEASE);
            SetLastNtError(Status);
            RETURN( (BOOL) -1);
         }
         Info.Msg.lParam = (LPARAM) UserMem;
      }
      if (RemoveMsg && Msg.FreeLParam && 0 != Msg.Msg.lParam)
      {
         ExFreePool((void *) Msg.Msg.lParam);
      }
      Status = MmCopyToCaller(UnsafeInfo, &Info, sizeof(NTUSERGETMESSAGEINFO));
      if (! NT_SUCCESS(Status))
      {
         SetLastNtError(Status);
         RETURN( (BOOL) -1);
      }
   }

   RETURN( Present);

CLEANUP:
   DPRINT("Leave NtUserPeekMessage, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

BOOL
APIENTRY
NtUserPeekMessageX(
   PMSG pMsg,
   HWND hWnd,
   UINT MsgFilterMin,
   UINT MsgFilterMax,
   UINT RemoveMsg)
{
   MSG Msg;
   BOOL Ret = FALSE;
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserPeekMessage\n");
   UserEnterExclusive();

   if ( RemoveMsg & PM_BADMSGFLAGS )
   {
      SetLastWin32Error(ERROR_INVALID_FLAGS);
      RETURN( Ret);
   }

   RtlZeroMemory(&Msg, sizeof(MSG));

   Ret = co_IntGetPeekMessage(&Msg, hWnd, MsgFilterMin, MsgFilterMax, RemoveMsg, FALSE);

   if (Ret)
   {
      _SEH2_TRY
      {
         ProbeForWrite(pMsg, sizeof(MSG), 1);
         RtlCopyMemory(pMsg, &Msg, sizeof(MSG));
      }
      _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
      {
         SetLastNtError(_SEH2_GetExceptionCode());
         Ret = FALSE;
      }
      _SEH2_END;
   }
   RETURN( Ret);

CLEANUP:
   DPRINT("Leave NtUserPeekMessage, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

BOOL
APIENTRY
NtUserCallMsgFilter(
   LPMSG lpmsg,
   INT code)
{
   BOOL BadChk = FALSE, Ret = FALSE;
   MSG Msg;
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserCallMsgFilter\n");
   UserEnterExclusive();
   if (lpmsg)
   {
      _SEH2_TRY
      {
         ProbeForRead((PVOID)lpmsg,
                       sizeof(MSG),
                                1);
         RtlCopyMemory( &Msg,
                (PVOID)lpmsg,
                 sizeof(MSG));
      }
      _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
      {
         BadChk = TRUE;
      }
      _SEH2_END;
   }
   else
     RETURN( FALSE);

   if (BadChk) RETURN( FALSE);

   if ( ISITHOOKED(WH_SYSMSGFILTER) &&
        co_HOOK_CallHooks( WH_SYSMSGFILTER, code, 0, (LPARAM)&Msg))
   {
      Ret = TRUE;
   }
   else
   {
      if ( ISITHOOKED(WH_MSGFILTER) )
      {
         Ret = co_HOOK_CallHooks( WH_MSGFILTER, code, 0, (LPARAM)&Msg);
      }
   }

   _SEH2_TRY
   {
      ProbeForWrite((PVOID)lpmsg,
                     sizeof(MSG),
                               1);
      RtlCopyMemory((PVOID)lpmsg,
                            &Msg,
                     sizeof(MSG));
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
      BadChk = TRUE;
   }
   _SEH2_END;
   if (BadChk) RETURN( FALSE);
   RETURN( Ret)

CLEANUP:
   DPRINT("Leave NtUserCallMsgFilter. ret=%i\n", _ret_);
   UserLeave();
   END_CLEANUP;
}

LRESULT APIENTRY
NtUserDispatchMessage(PMSG UnsafeMsgInfo)
{
  LRESULT Res = 0;
  BOOL Hit = FALSE;
  MSG SafeMsg;

  UserEnterExclusive();  
  _SEH2_TRY
  {
    ProbeForRead(UnsafeMsgInfo, sizeof(MSG), 1);
    RtlCopyMemory(&SafeMsg, UnsafeMsgInfo, sizeof(MSG));
  }
  _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
  {
    SetLastNtError(_SEH2_GetExceptionCode());
    Hit = TRUE;
  }
  _SEH2_END;
  
  if (!Hit) Res = IntDispatchMessage(&SafeMsg);

  UserLeave();
  return Res;
}


BOOL APIENTRY
NtUserTranslateMessage(LPMSG lpMsg,
                       UINT flags)
{
   NTSTATUS Status;
   MSG SafeMsg;
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserTranslateMessage\n");
   UserEnterExclusive();

   Status = MmCopyFromCaller(&SafeMsg, lpMsg, sizeof(MSG));
   if(!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      RETURN( FALSE);
   }

   RETURN( IntTranslateKbdMessage(&SafeMsg, flags));

CLEANUP:
   DPRINT("Leave NtUserTranslateMessage: ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

BOOL APIENTRY
NtUserMessageCall(
   HWND hWnd,
   UINT Msg,
   WPARAM wParam,
   LPARAM lParam,
   ULONG_PTR ResultInfo,
   DWORD dwType, // fnID?
   BOOL Ansi)
{
   LRESULT lResult = 0;
   BOOL Ret = FALSE;
   BOOL BadChk = FALSE;
   PWINDOW_OBJECT Window = NULL;
   USER_REFERENCE_ENTRY Ref;

   UserEnterExclusive();

   /* Validate input */
   if (hWnd && (hWnd != INVALID_HANDLE_VALUE) && !(Window = UserGetWindowObject(hWnd)))
   {
      UserLeave();
      return FALSE;
   }
   switch(dwType)
   {
      case FNID_DEFWINDOWPROC:
         UserRefObjectCo(Window, &Ref);
         lResult = IntDefWindowProc(Window, Msg, wParam, lParam, Ansi);
         Ret = TRUE;
         UserDerefObjectCo(Window);
      break;
      case FNID_SENDNOTIFYMESSAGE:
         Ret = UserSendNotifyMessage(hWnd, Msg, wParam, lParam);
      break;
      case FNID_BROADCASTSYSTEMMESSAGE:
      {
         BROADCASTPARM parm;
         DWORD_PTR RetVal = 0;

         if (ResultInfo)
         {
            _SEH2_TRY
            {
               ProbeForWrite((PVOID)ResultInfo,
                         sizeof(BROADCASTPARM),
                                             1);
               RtlCopyMemory(&parm, (PVOID)ResultInfo, sizeof(BROADCASTPARM));
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
               BadChk = TRUE;
            }
            _SEH2_END;
            if (BadChk) break;
         }
         else
           break;

         if ( parm.recipients & BSM_ALLDESKTOPS ||
              parm.recipients == BSM_ALLCOMPONENTS )
         {
         }
         else if (parm.recipients & BSM_APPLICATIONS)
         {
            if (parm.flags & BSF_QUERY)
            {
               if (parm.flags & BSF_FORCEIFHUNG || parm.flags & BSF_NOHANG)
               {
                  co_IntSendMessageTimeout( HWND_BROADCAST,
                                            Msg,
                                            wParam,
                                            lParam,
                                            SMTO_ABORTIFHUNG,
                                            2000,
                                            &RetVal);
               }
               else if (parm.flags & BSF_NOTIMEOUTIFNOTHUNG)
               {
                  co_IntSendMessageTimeout( HWND_BROADCAST,
                                            Msg,
                                            wParam,
                                            lParam,
                                            SMTO_NOTIMEOUTIFNOTHUNG,
                                            2000,
                                            &RetVal);
               }
               else
               {
                  co_IntSendMessageTimeout( HWND_BROADCAST,
                                            Msg,
                                            wParam,
                                            lParam,
                                            SMTO_NORMAL,
                                            2000,
                                            &RetVal);
               }
            }
            else if (parm.flags & BSF_POSTMESSAGE)
            {
               Ret = UserPostMessage(HWND_BROADCAST, Msg, wParam, lParam);
            }
            else if ( parm.flags & BSF_SENDNOTIFYMESSAGE)
            {
               Ret = UserSendNotifyMessage(HWND_BROADCAST, Msg, wParam, lParam);
            }
         }
      }
      break;
      case FNID_SENDMESSAGECALLBACK:
      {
         PCALL_BACK_INFO CallBackInfo = (PCALL_BACK_INFO)ResultInfo;

         if (!CallBackInfo)
            break;

         if (!co_IntSendMessageWithCallBack(hWnd, Msg, wParam, lParam,
             CallBackInfo->CallBack, CallBackInfo->Context, NULL))
         {
            DPRINT1("Callback failure!\n");
         }
      }
      break;
      // CallNextHook bypass.
      case FNID_CALLWNDPROC:
      case FNID_CALLWNDPROCRET:
      {
         PCLIENTINFO ClientInfo = GetWin32ClientInfo();
         PHOOK NextObj, Hook = ClientInfo->phkCurrent;

         if (!ClientInfo || !Hook) break;

         UserReferenceObject(Hook);

         if (Hook->Thread && (Hook->Thread != PsGetCurrentThread()))
         {
            UserDereferenceObject(Hook);
            break;
         }

         NextObj = IntGetNextHook(Hook);
         ClientInfo->phkCurrent = NextObj;

         if ( Hook->HookId == WH_CALLWNDPROC)
         {
            CWPSTRUCT CWP;
            CWP.hwnd    = hWnd;
            CWP.message = Msg;
            CWP.wParam  = wParam;
            CWP.lParam  = lParam;
            DPRINT("WH_CALLWNDPROC: Hook %x NextHook %x\n", Hook, NextObj );

            lResult = co_IntCallHookProc( Hook->HookId,
                                          HC_ACTION,
                                        ((ClientInfo->CI_flags & CI_CURTHPRHOOK) ? 1 : 0),
                                         (LPARAM)&CWP,
                                          Hook->Proc,
                                          Hook->Ansi,
                                          &Hook->ModuleName);
         }
         else
         {
            CWPRETSTRUCT CWPR;
            CWPR.hwnd    = hWnd;
            CWPR.message = Msg;
            CWPR.wParam  = wParam;
            CWPR.lParam  = lParam;
            CWPR.lResult = ClientInfo->dwHookData;

            lResult = co_IntCallHookProc( Hook->HookId,
                                          HC_ACTION,
                                        ((ClientInfo->CI_flags & CI_CURTHPRHOOK) ? 1 : 0),
                                         (LPARAM)&CWPR,
                                          Hook->Proc,
                                          Hook->Ansi,
                                          &Hook->ModuleName);
         }
         UserDereferenceObject(Hook);
         lResult = (LRESULT) NextObj;
      }
      break;
   }

   switch(dwType)
   {
      case FNID_DEFWINDOWPROC:
      case FNID_CALLWNDPROC:
      case FNID_CALLWNDPROCRET:
         if (ResultInfo)
         {
            _SEH2_TRY
            {
                ProbeForWrite((PVOID)ResultInfo, sizeof(LRESULT), 1);
                RtlCopyMemory((PVOID)ResultInfo, &lResult, sizeof(LRESULT));
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                BadChk = TRUE;
            }
            _SEH2_END;      
         }
         break;
      default:
         break;   
   }

   UserLeave();

   return BadChk ? FALSE : Ret;
}

#define INFINITE 0xFFFFFFFF
#define WAIT_FAILED ((DWORD)0xFFFFFFFF)

DWORD
APIENTRY
NtUserWaitForInputIdle(
   IN HANDLE hProcess,
   IN DWORD dwMilliseconds,
   IN BOOL Unknown2)
{
  PEPROCESS Process;
  PPROCESSINFO W32Process;
  NTSTATUS Status;
  HANDLE Handles[2];
  LARGE_INTEGER Timeout;
  ULONGLONG StartTime, Run, Elapsed = 0;

  UserEnterExclusive();

  Status = ObReferenceObjectByHandle(hProcess,
                                     PROCESS_QUERY_INFORMATION,
                                     PsProcessType,
                                     UserMode,
                                     (PVOID*)&Process,
                                     NULL);

  if (!NT_SUCCESS(Status))
  {
     UserLeave();
     SetLastNtError(Status);
     return WAIT_FAILED;
  }

  W32Process = (PPROCESSINFO)Process->Win32Process;
  if (!W32Process)
  {
      ObDereferenceObject(Process);
      UserLeave();
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return WAIT_FAILED;
  }

  EngCreateEvent((PEVENT *)&W32Process->InputIdleEvent);

  Handles[0] = Process;
  Handles[1] = W32Process->InputIdleEvent;

  if (!Handles[1])
  {
      ObDereferenceObject(Process);
      UserLeave();
      return STATUS_SUCCESS;  /* no event to wait on */
  }

  StartTime = EngGetTickCount();

  Run = dwMilliseconds;

  DPRINT("WFII: waiting for %p\n", Handles[1] );
  do
  {
     Timeout.QuadPart = Run - Elapsed;
     UserLeave();
     Status = KeWaitForMultipleObjects( 2,
                                        Handles,
                                        WaitAny,
                                        UserRequest,
                                        UserMode,
                                        FALSE,
                             dwMilliseconds == INFINITE ? NULL : &Timeout,
                                        NULL);
     UserEnterExclusive();

     if (!NT_SUCCESS(Status))
     {
        SetLastNtError(Status);
        Status = WAIT_FAILED;
        goto WaitExit;
     }

     switch (Status)
     {
        case STATUS_WAIT_0:
           Status = WAIT_FAILED;
           goto WaitExit;

        case STATUS_WAIT_2:
        {
           USER_MESSAGE Msg;
           co_IntPeekMessage( &Msg, 0, 0, 0, PM_REMOVE | PM_QS_SENDMESSAGE );
           break;
        }

        case STATUS_USER_APC:
        case STATUS_ALERTED:
        case STATUS_TIMEOUT:
           DPRINT1("WFII: timeout\n");
           Status = STATUS_TIMEOUT;
           goto WaitExit;

        default:
           DPRINT1("WFII: finished\n");
           Status = STATUS_SUCCESS;
           goto WaitExit;
     }

     if (dwMilliseconds != INFINITE)
     {
        Elapsed = EngGetTickCount() - StartTime;

        if (Elapsed > Run)
           Status = STATUS_TIMEOUT;
           break;
     }
  }
  while (1);

WaitExit:
  if (W32Process->InputIdleEvent)
  {
     EngFreeMem((PVOID)W32Process->InputIdleEvent);
     W32Process->InputIdleEvent = NULL;
  }
  ObDereferenceObject(Process);
  UserLeave();
  return Status;
}

/* EOF */
