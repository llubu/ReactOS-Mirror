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
 */
/* $Id: window.c,v 1.80 2003/08/06 18:43:58 weiden Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Windows
 * FILE:             subsys/win32k/ntuser/window.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISION HISTORY:
 *       06-06-2001  CSH  Created
 */
/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <internal/safe.h>
#include <win32k/win32k.h>
#include <include/object.h>
#include <include/guicheck.h>
#include <include/window.h>
#include <include/class.h>
#include <include/error.h>
#include <include/winsta.h>
#include <include/winpos.h>
#include <include/callback.h>
#include <include/msgqueue.h>
#include <include/rect.h>
#include <include/dce.h>
#include <include/paint.h>
#include <include/painting.h>
#include <include/scroll.h>
#include <include/vis.h>
#include <include/menu.h>

#define NDEBUG
#include <win32k/debug1.h>
#include <debug.h>

#define TAG_WNAM  TAG('W', 'N', 'A', 'M')

typedef struct _REGISTERED_MESSAGE
{
  LIST_ENTRY ListEntry;
  WCHAR MessageName[1];
} REGISTERED_MESSAGE, *PREGISTERED_MESSAGE;

static LIST_ENTRY RegisteredMessageListHead;

#define REGISTERED_MESSAGE_MIN 0xc000
#define REGISTERED_MESSAGE_MAX 0xffff

PWINDOW_OBJECT FASTCALL
W32kGetParent(PWINDOW_OBJECT Wnd);

/* FUNCTIONS *****************************************************************/

PWINDOW_OBJECT FASTCALL
W32kGetAncestor(PWINDOW_OBJECT Wnd, UINT Type)
{
  if (W32kIsDesktopWindow(Wnd)) return NULL;

  switch (Type)
  {
    case GA_PARENT:
      return Wnd->Parent;

    case GA_ROOT:
      while(!W32kIsDesktopWindow(Wnd->Parent))
      {
        Wnd = Wnd->Parent;
      }
      return Wnd;
    
    case GA_ROOTOWNER:
      while ((Wnd = W32kGetParent(Wnd)));
      return Wnd;
  }

  return NULL;
}


HWND STDCALL
NtUserGetAncestor(HWND hWnd, UINT Type)
{
  PWINDOW_OBJECT Wnd, WndAncestor;
  HWND hWndAncestor = NULL;

  W32kAcquireWinStaLockShared();

  if (!(Wnd = W32kGetWindowObject(hWnd)))
  {
    SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
    return NULL;
  }

  WndAncestor = W32kGetAncestor(Wnd, Type);
  if (WndAncestor) hWndAncestor = WndAncestor->Self;

  W32kReleaseWinStaLock();

  return hWndAncestor;
}

PWINDOW_OBJECT FASTCALL
W32kGetParent(PWINDOW_OBJECT Wnd)
{
  if (Wnd->Style & WS_POPUP)
  {
    return W32kGetWindowObject(Wnd->ParentHandle); /* wine use HWND for owner window (unknown reason) */
  }
  else if (Wnd->Style & WS_CHILD) 
  {
    return Wnd->Parent;
  }

  return NULL;
}


HWND STDCALL
NtUserGetParent(HWND hWnd)
{
  PWINDOW_OBJECT Wnd, WndParent;
  HWND hWndParent = NULL;

  W32kAcquireWinStaLockShared();

  if (!(Wnd = W32kGetWindowObject(hWnd)))
  {
    SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
    return NULL;
  }

  WndParent = W32kGetParent(Wnd);
  if (WndParent) hWndParent = WndParent->Self;

  W32kReleaseWinStaLock();

  return hWndParent;
}



HWND FASTCALL
W32kSetFocusWindow(HWND hWnd)
{
  PUSER_MESSAGE_QUEUE OldMessageQueue;
  PDESKTOP_OBJECT DesktopObject;
  PWINDOW_OBJECT WindowObject;
  HWND hWndOldFocus;

  DPRINT("W32kSetFocusWindow(hWnd 0x%x)\n", hWnd);

  if (hWnd != (HWND)0)
    {
      WindowObject = W32kGetWindowObject(hWnd);
      if (!WindowObject)
        {
          DPRINT("Bad window handle 0x%x\n", hWnd);
          SetLastWin32Error(ERROR_INVALID_HANDLE);
      	  return (HWND)0;
        }
    }
  else
  {
    WindowObject = NULL;
  }

  DesktopObject = W32kGetActiveDesktop();
  if (!DesktopObject)
    {
      DPRINT("No active desktop\n");
      if (WindowObject != NULL)
        {
    	    W32kReleaseWindowObject(WindowObject);
        }
      SetLastWin32Error(ERROR_INVALID_HANDLE);
  	  return (HWND)0;
    }

  hWndOldFocus = (HWND)0;
  OldMessageQueue = (PUSER_MESSAGE_QUEUE)DesktopObject->ActiveMessageQueue;
  if (OldMessageQueue != NULL)
    {
      hWndOldFocus = OldMessageQueue->FocusWindow;
    }

  if (WindowObject != NULL)
    {
      WindowObject->MessageQueue->FocusWindow = hWnd;
      (PUSER_MESSAGE_QUEUE)DesktopObject->ActiveMessageQueue =
        WindowObject->MessageQueue;
      W32kReleaseWindowObject(WindowObject);
    }
  else
    {
      (PUSER_MESSAGE_QUEUE)DesktopObject->ActiveMessageQueue = NULL;
    }

  DPRINT("hWndOldFocus = 0x%x\n", hWndOldFocus);

  return hWndOldFocus;
}

BOOL FASTCALL
W32kIsChildWindow(HWND Parent, HWND Child)
{
  PWINDOW_OBJECT BaseWindow = W32kGetWindowObject(Child);
  PWINDOW_OBJECT Window = BaseWindow;
  while (Window != NULL && Window->Style & WS_CHILD)
    {
      if (Window->Self == Parent)
	{
	  W32kReleaseWindowObject(BaseWindow);
	  return(TRUE);
	}
      Window = Window->Parent;
    }
  W32kReleaseWindowObject(BaseWindow);
  return(FALSE);  
}

BOOL FASTCALL
W32kIsWindowVisible(HWND Wnd)
{
  PWINDOW_OBJECT BaseWindow = W32kGetWindowObject(Wnd);
  PWINDOW_OBJECT Window = BaseWindow;
  BOOLEAN Result = FALSE;
  while (Window != NULL && Window->Style & WS_CHILD)
    {
      if (!(Window->Style & WS_VISIBLE))
	{
	  W32kReleaseWindowObject(BaseWindow);
	  return(FALSE);
	}
      Window = Window->Parent;
    }
  if (Window != NULL && Window->Style & WS_VISIBLE)
    {
      Result = TRUE;
    }
  W32kReleaseWindowObject(BaseWindow);
  return(Result);
}

BOOL FASTCALL
W32kIsDesktopWindow(PWINDOW_OBJECT WindowObject)
{
  BOOL IsDesktop;
  ASSERT(WindowObject);
  IsDesktop = WindowObject->Parent == NULL;
  return(IsDesktop);
}

HWND FASTCALL W32kGetDesktopWindow(VOID)
{
  return W32kGetActiveDesktop()->DesktopWindow;
}

PWINDOW_OBJECT FASTCALL
W32kGetWindowObject(HWND hWnd)
{
  PWINDOW_OBJECT WindowObject;
  NTSTATUS Status;
  Status = 
    ObmReferenceObjectByHandle(PsGetWin32Process()->WindowStation->
			       HandleTable,
			       hWnd,
			       otWindow,
			       (PVOID*)&WindowObject);
  if (!NT_SUCCESS(Status))
    {
      return(NULL);
    }
  return(WindowObject);
}

VOID FASTCALL
W32kReleaseWindowObject(PWINDOW_OBJECT Window)
{
  ObmDereferenceObject(Window);
}

/*!
 * Internal function.
 * Returns client window rectangle relative to the upper-left corner of client area.
 *
 * \note Does not check the validity of the parameters
*/
VOID FASTCALL
W32kGetClientRect(PWINDOW_OBJECT WindowObject, PRECT Rect)
{
  ASSERT( WindowObject );
  ASSERT( Rect );

  Rect->left = Rect->top = 0;
  Rect->right = WindowObject->ClientRect.right - WindowObject->ClientRect.left;
  Rect->bottom = 
    WindowObject->ClientRect.bottom - WindowObject->ClientRect.top;
}


/*!
 * Return the dimension of the window in the screen coordinates.
 * \param	hWnd	window handle.
 * \param	Rect	pointer to the buffer where the coordinates are returned.
*/
BOOL STDCALL
NtUserGetWindowRect(HWND hWnd, LPRECT Rect)
{
  PWINDOW_OBJECT Wnd;
  RECT SafeRect;

  W32kAcquireWinStaLockShared();
  if (!(Wnd = W32kGetWindowObject(hWnd)))
  {
    W32kReleaseWinStaLock();
    SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);      
    return FALSE;
  }
  
  SafeRect = Wnd->WindowRect;
  W32kReleaseWinStaLock();

  if (! NT_SUCCESS(MmCopyToCaller(Rect, &SafeRect, sizeof(RECT))))
  {
    return FALSE;
  }

  return TRUE;
}

/*!
 * Returns client window rectangle relative to the upper-left corner of client area.
 *
 * \param	hWnd	window handle.
 * \param	Rect	pointer to the buffer where the coordinates are returned.
 *
*/
BOOL STDCALL
NtUserGetClientRect(HWND hWnd, LPRECT Rect)
{
  PWINDOW_OBJECT WindowObject;
  RECT SafeRect;

  W32kAcquireWinStaLockShared();
  if (!(WindowObject = W32kGetWindowObject(hWnd)))
  {
    W32kReleaseWinStaLock();
    SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);      
    return FALSE;
  }

  W32kGetClientRect(WindowObject, &SafeRect);
  W32kReleaseWinStaLock();

  if (! NT_SUCCESS(MmCopyToCaller(Rect, &SafeRect, sizeof(RECT))))
  {
    return(FALSE);
  }

  return(TRUE);
}

HWND FASTCALL
W32kGetActiveWindow(VOID)
{
  PUSER_MESSAGE_QUEUE Queue;
  Queue = (PUSER_MESSAGE_QUEUE)W32kGetActiveDesktop()->ActiveMessageQueue;
  if (Queue == NULL)
    {
      return(NULL);
    }
  else
    {
      return(Queue->ActiveWindow);
    }
}

HWND FASTCALL
W32kGetFocusWindow(VOID)
{
  PUSER_MESSAGE_QUEUE Queue;
  PDESKTOP_OBJECT pdo = W32kGetActiveDesktop();

  if( !pdo )
	return NULL;

  Queue = (PUSER_MESSAGE_QUEUE)pdo->ActiveMessageQueue;

  if (Queue == NULL)
      return(NULL);
  else
      return(Queue->FocusWindow);
}


NTSTATUS FASTCALL
InitWindowImpl(VOID)
{
  InitializeListHead(&RegisteredMessageListHead);

  return(STATUS_SUCCESS);
}

NTSTATUS FASTCALL
CleanupWindowImpl(VOID)
{
  return(STATUS_SUCCESS);
}


DWORD STDCALL
NtUserAlterWindowStyle(DWORD Unknown0,
		       DWORD Unknown1,
		       DWORD Unknown2)
{
  UNIMPLEMENTED

  return(0);
}

DWORD STDCALL
NtUserChildWindowFromPointEx(HWND Parent,
			     LONG x,
			     LONG y,
			     UINT Flags)
{
  UNIMPLEMENTED

  return(0);
}

HWND STDCALL
W32kCreateDesktopWindow(PWINSTATION_OBJECT WindowStation,
			PWNDCLASS_OBJECT DesktopClass,
			ULONG Width, ULONG Height)
{
  PWSTR WindowName;
  HWND Handle;
  PWINDOW_OBJECT WindowObject;

  /* Create the window object. */
  WindowObject = (PWINDOW_OBJECT)ObmCreateObject(WindowStation->HandleTable, 
						 &Handle, 
						 otWindow,
						 sizeof(WINDOW_OBJECT));
  if (!WindowObject) 
    {
      return((HWND)0);
    }

  /*
   * Fill out the structure describing it.
   */
  WindowObject->Class = DesktopClass;
  WindowObject->ExStyle = 0;
  WindowObject->Style = WS_VISIBLE;
  WindowObject->x = 0;
  WindowObject->y = 0;
  WindowObject->Width = Width;
  WindowObject->Height = Height;
  WindowObject->ParentHandle = NULL;
  WindowObject->Parent = NULL;
  WindowObject->Menu = NULL;
  WindowObject->Instance = NULL;
  WindowObject->Parameters = NULL;
  WindowObject->Self = Handle;
  WindowObject->MessageQueue = NULL;
  WindowObject->ExtraData = NULL;
  WindowObject->ExtraDataSize = 0;
  WindowObject->WindowRect.left = 0;
  WindowObject->WindowRect.top = 0;
  WindowObject->WindowRect.right = Width;
  WindowObject->WindowRect.bottom = Height;
  WindowObject->ClientRect = WindowObject->WindowRect;
  WindowObject->UserData = 0;
  /*FIXME: figure out what the correct strange value is and what to do with it (and how to set the wndproc values correctly) */
  WindowObject->WndProcA = DesktopClass->lpfnWndProcA;
  WindowObject->WndProcW = DesktopClass->lpfnWndProcW;
  WindowObject->OwnerThread = PsGetCurrentThread();

  InitializeListHead(&WindowObject->ChildrenListHead);
  ExInitializeFastMutex(&WindowObject->ChildrenListLock);

  WindowName = ExAllocatePool(NonPagedPool, sizeof(L"DESKTOP"));
  wcscpy(WindowName, L"DESKTOP");
  RtlInitUnicodeString(&WindowObject->WindowName, WindowName);

  return(Handle);
}

VOID FASTCALL
W32kInitDesktopWindow(ULONG Width, ULONG Height)
{
  PWINDOW_OBJECT DesktopWindow;
  HRGN DesktopRgn;
  
  DesktopWindow = W32kGetWindowObject(PsGetWin32Thread()->Desktop->DesktopWindow);
  if (NULL == DesktopWindow)
    {
      return;
    }
  DesktopWindow->WindowRect.right = Width;
  DesktopWindow->WindowRect.bottom = Height;
  DesktopWindow->ClientRect = DesktopWindow->WindowRect;

  DesktopRgn = UnsafeW32kCreateRectRgnIndirect(&(DesktopWindow->WindowRect));
  VIS_WindowLayoutChanged(PsGetWin32Thread()->Desktop, DesktopWindow, DesktopRgn);
  W32kDeleteObject(DesktopRgn);
  W32kReleaseWindowObject(DesktopWindow);
}

HWND STDCALL
NtUserCreateWindowEx(DWORD dwExStyle,
		     PUNICODE_STRING lpClassName,
		     PUNICODE_STRING lpWindowName,
		     DWORD dwStyle,
		     LONG x,
		     LONG y,
		     LONG nWidth,
		     LONG nHeight,
		     HWND hWndParent,
		     HMENU hMenu,
		     HINSTANCE hInstance,
		     LPVOID lpParam,
		     DWORD dwShowMode)
{
  PWINSTATION_OBJECT WinStaObject;
  PWNDCLASS_OBJECT ClassObject;
  PWINDOW_OBJECT WindowObject;
  PWINDOW_OBJECT ParentWindow;
  UNICODE_STRING WindowName;
  NTSTATUS Status;
  HANDLE Handle;
  POINT MaxSize, MaxPos, MinTrack, MaxTrack;
  CREATESTRUCTW Cs;
  LRESULT Result;
  DPRINT("NtUserCreateWindowEx\n");

  /* Initialize gui state if necessary. */
  W32kGraphicsCheck(TRUE);

  if (!RtlCreateUnicodeString(&WindowName,
                              NULL == lpWindowName->Buffer ?
                              L"" : lpWindowName->Buffer))
    {
      SetLastNtError(STATUS_INSUFFICIENT_RESOURCES);
      return((HWND)0);
    }

  if (hWndParent != NULL)
    {
      ParentWindow = W32kGetWindowObject(hWndParent);
    }
  else
    {
      hWndParent = PsGetWin32Thread()->Desktop->DesktopWindow;
      ParentWindow = W32kGetWindowObject(hWndParent);
    }

  /* Check the class. */
  Status = ClassReferenceClassByNameOrAtom(&ClassObject, lpClassName->Buffer);
  if (!NT_SUCCESS(Status))
    {
      RtlFreeUnicodeString(&WindowName);
      W32kReleaseWindowObject(ParentWindow);
      return((HWND)0);
    }

  /* Check the window station. */
  DPRINT("IoGetCurrentProcess() %X\n", IoGetCurrentProcess());
  DPRINT("PROCESS_WINDOW_STATION %X\n", PROCESS_WINDOW_STATION());
  Status = ValidateWindowStationHandle(PROCESS_WINDOW_STATION(),
				       KernelMode,
				       0,
				       &WinStaObject);
  if (!NT_SUCCESS(Status))
    {
      RtlFreeUnicodeString(&WindowName);
      ObmDereferenceObject(ClassObject);
      W32kReleaseWindowObject(ParentWindow);
      DPRINT("Validation of window station handle (0x%X) failed\n",
	     PROCESS_WINDOW_STATION());
      return (HWND)0;
    }

  /* Create the window object. */
  WindowObject = (PWINDOW_OBJECT)
    ObmCreateObject(PsGetWin32Process()->WindowStation->HandleTable, &Handle,
        otWindow, sizeof(WINDOW_OBJECT) + ClassObject->cbWndExtra
        );

  DPRINT("Created object with handle %X\n", Handle);
  if (!WindowObject)
    {
      ObDereferenceObject(WinStaObject);
      ObmDereferenceObject(ClassObject);
      RtlFreeUnicodeString(&WindowName);
      W32kReleaseWindowObject(ParentWindow);
      SetLastNtError(STATUS_INSUFFICIENT_RESOURCES);
      return (HWND)0;
    }
  ObDereferenceObject(WinStaObject);

  /*
   * Fill out the structure describing it.
   */
  WindowObject->Class = ClassObject;
  WindowObject->ExStyle = dwExStyle;
  WindowObject->Style = dwStyle | WIN_NCACTIVATED;
  WindowObject->x = x;
  WindowObject->y = y;
  WindowObject->Width = nWidth;
  WindowObject->Height = nHeight;
  WindowObject->ParentHandle = hWndParent;
  WindowObject->Menu = hMenu;
  WindowObject->Instance = hInstance;
  WindowObject->Parameters = lpParam;
  WindowObject->Self = Handle;
  WindowObject->MessageQueue = PsGetWin32Thread()->MessageQueue;
  WindowObject->Parent = ParentWindow;
  WindowObject->UserData = 0;
  WindowObject->Unicode = ClassObject->Unicode;
  WindowObject->WndProcA = ClassObject->lpfnWndProcA;
  WindowObject->WndProcW = ClassObject->lpfnWndProcW;
  WindowObject->OwnerThread = PsGetCurrentThread();

  /* extra window data */
  if (ClassObject->cbWndExtra != 0)
    {
      WindowObject->ExtraData = (PULONG)(WindowObject + 1);
      WindowObject->ExtraDataSize = ClassObject->cbWndExtra;
      RtlZeroMemory(WindowObject->ExtraData, WindowObject->ExtraDataSize);
    }
  else
    {
      WindowObject->ExtraData = NULL;
      WindowObject->ExtraDataSize = 0;
    }

  ExAcquireFastMutexUnsafe(&ParentWindow->ChildrenListLock);
  InsertHeadList(&ParentWindow->ChildrenListHead,
		 &WindowObject->SiblingListEntry);
  ExReleaseFastMutexUnsafe(&ParentWindow->ChildrenListLock);

  InitializeListHead(&WindowObject->ChildrenListHead);
  InitializeListHead(&WindowObject->PropListHead);
  ExInitializeFastMutex(&WindowObject->ChildrenListLock);

  RtlInitUnicodeString(&WindowObject->WindowName, WindowName.Buffer);
  RtlFreeUnicodeString(&WindowName);


  /* Correct the window style. */
  if (!(dwStyle & WS_CHILD))
    {
      WindowObject->Style |= WS_CLIPSIBLINGS;
      if (!(dwStyle & WS_POPUP))
	{
	  WindowObject->Style |= WS_CAPTION;
	  /* FIXME: Note the window needs a size. */ 
	}
    }

  /* Insert the window into the thread's window list. */
  ExAcquireFastMutexUnsafe (&PsGetWin32Thread()->WindowListLock);
  InsertTailList (&PsGetWin32Thread()->WindowListHead, 
		  &WindowObject->ThreadListEntry);
  ExReleaseFastMutexUnsafe (&PsGetWin32Thread()->WindowListLock);

  /*
   * Insert the window into the list of windows associated with the thread's
   * desktop. 
   */
  InsertTailList(&PsGetWin32Thread()->Desktop->WindowListHead,
		 &WindowObject->DesktopListEntry);
  /* Allocate a DCE for this window. */
  if (dwStyle & CS_OWNDC) WindowObject->Dce = DceAllocDCE(WindowObject->Self,DCE_WINDOW_DC);
  /* FIXME:  Handle "CS_CLASSDC" */

  /* Initialize the window dimensions. */
  WindowObject->WindowRect.left = x;
  WindowObject->WindowRect.top = y;
  WindowObject->WindowRect.right = x + nWidth;
  WindowObject->WindowRect.bottom = y + nHeight;
  WindowObject->ClientRect = WindowObject->WindowRect;

  /*
   * Get the size and position of the window.
   */
  if ((dwStyle & WS_THICKFRAME) || !(dwStyle & (WS_POPUP | WS_CHILD)))
    {
      WinPosGetMinMaxInfo(WindowObject, &MaxSize, &MaxPos, &MinTrack,
			  &MaxTrack);
      x = min(MaxSize.x, y);
      y = min(MaxSize.y, y);
      x = max(MinTrack.x, x);
      y = max(MinTrack.y, y);
    }

  WindowObject->WindowRect.left = x;
  WindowObject->WindowRect.top = y;
  WindowObject->WindowRect.right = x + nWidth;
  WindowObject->WindowRect.bottom = y + nHeight;
  WindowObject->ClientRect = WindowObject->WindowRect;

  /* FIXME: Initialize the window menu. */
  
  /* Initialize the window's scrollbars */
  if (dwStyle & WS_VSCROLL)
      SCROLL_CreateScrollBar(WindowObject, SB_VERT);
  if (dwStyle & WS_HSCROLL)
      SCROLL_CreateScrollBar(WindowObject, SB_HORZ);

  /* Send a NCCREATE message. */
  Cs.lpCreateParams = lpParam;
  Cs.hInstance = hInstance;
  Cs.hMenu = hMenu;
  Cs.hwndParent = hWndParent;
  Cs.cx = nWidth;
  Cs.cy = nHeight;
  Cs.x = x;
  Cs.y = y;
  Cs.style = dwStyle;
  Cs.lpszName = lpWindowName->Buffer;
  Cs.lpszClass = lpClassName->Buffer;
  Cs.dwExStyle = dwExStyle;
  DPRINT("NtUserCreateWindowEx(): About to send NCCREATE message.\n");
  Result = W32kSendNCCREATEMessage(WindowObject->Self, &Cs);
  if (!Result)
    {
      /* FIXME: Cleanup. */
      W32kReleaseWindowObject(ParentWindow);
      DPRINT("NtUserCreateWindowEx(): NCCREATE message failed.\n");
      return((HWND)0);
    }
 
  /* Calculate the non-client size. */
  MaxPos.x = WindowObject->WindowRect.left;
  MaxPos.y = WindowObject->WindowRect.top;
  DPRINT("NtUserCreateWindowEx(): About to get non-client size.\n");
  Result = WinPosGetNonClientSize(WindowObject->Self, 
				  &WindowObject->WindowRect,
				  &WindowObject->ClientRect);
  W32kOffsetRect(&WindowObject->WindowRect, 
		 MaxPos.x - WindowObject->WindowRect.left,
		 MaxPos.y - WindowObject->WindowRect.top);

  /* Send the CREATE message. */
  DPRINT("NtUserCreateWindowEx(): about to send CREATE message.\n");
  Result = W32kSendCREATEMessage(WindowObject->Self, &Cs);
  if (Result == (LRESULT)-1)
    {
      /* FIXME: Cleanup. */
      W32kReleaseWindowObject(ParentWindow);
      DPRINT("NtUserCreateWindowEx(): send CREATE message failed.\n");
      return((HWND)0);
    } 

  /* Send move and size messages. */
  if (!(WindowObject->Flags & WINDOWOBJECT_NEED_SIZE))
    {
      LONG lParam;
      
      lParam = 
	MAKE_LONG(WindowObject->ClientRect.right - 
		  WindowObject->ClientRect.left,
		  WindowObject->ClientRect.bottom - 
		  WindowObject->ClientRect.top);
 
      DPRINT("NtUserCreateWindow(): About to send WM_SIZE\n");
      W32kCallWindowProc(NULL, WindowObject->Self, WM_SIZE, SIZE_RESTORED, 
			 lParam);
      lParam = 
	MAKE_LONG(WindowObject->ClientRect.left,
		  WindowObject->ClientRect.top);
      DPRINT("NtUserCreateWindow(): About to send WM_MOVE\n");
      W32kCallWindowProc(NULL, WindowObject->Self, WM_MOVE, 0, lParam);
    }

  /* Move from parent-client to screen coordinates */
  if (0 != (WindowObject->Style & WS_CHILD))
    {
    W32kOffsetRect(&WindowObject->WindowRect,
		   ParentWindow->ClientRect.left,
		   ParentWindow->ClientRect.top);
    W32kOffsetRect(&WindowObject->ClientRect,
		   ParentWindow->ClientRect.left,
		   ParentWindow->ClientRect.top);
    }

  /* Show or maybe minimize or maximize the window. */
  if (WindowObject->Style & (WS_MINIMIZE | WS_MAXIMIZE))
    {
      RECT NewPos;
      UINT16 SwFlag;

      SwFlag = (WindowObject->Style & WS_MINIMIZE) ? SW_MINIMIZE : 
	SW_MAXIMIZE;
      WinPosMinMaximize(WindowObject, SwFlag, &NewPos);
      SwFlag = 
	((WindowObject->Style & WS_CHILD) || W32kGetActiveWindow()) ?
	SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED :
	SWP_NOZORDER | SWP_FRAMECHANGED;
      DPRINT("NtUserCreateWindow(): About to minimize/maximize\n");
      WinPosSetWindowPos(WindowObject->Self, 0, NewPos.left, NewPos.top,
			 NewPos.right, NewPos.bottom, SwFlag);
    }

  /* Notify the parent window of a new child. */
  if ((WindowObject->Style & WS_CHILD) ||
      (!(WindowObject->ExStyle & WS_EX_NOPARENTNOTIFY)))
    {
      DPRINT("NtUserCreateWindow(): About to notify parent\n");
      W32kCallWindowProc(NULL, WindowObject->Parent->Self,
			 WM_PARENTNOTIFY, 
			 MAKEWPARAM(WM_CREATE, WindowObject->IDMenu),
			 (LPARAM)WindowObject->Self);
    }

  if (dwStyle & WS_VISIBLE)
    {
      DPRINT("NtUserCreateWindow(): About to show window\n");
      WinPosShowWindow(WindowObject->Self, dwShowMode);
    }

  DPRINT("NtUserCreateWindow(): = %X\n", Handle);
  return((HWND)Handle);
}

HDWP STDCALL
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


/***********************************************************************
 *           W32kSendDestroyMsg
 */
static void W32kSendDestroyMsg(HWND Wnd)
{
#if 0 /* FIXME */
  GUITHREADINFO info;

  if (GetGUIThreadInfo(GetCurrentThreadId(), &info))
    {
      if (Wnd == info.hwndCaret)
	{
	  DestroyCaret();
	}
    }
#endif

  /*
   * Send the WM_DESTROY to the window.
   */
  NtUserSendMessage(Wnd, WM_DESTROY, 0, 0);

  /*
   * This WM_DESTROY message can trigger re-entrant calls to DestroyWindow
   * make sure that the window still exists when we come back.
   */
#if 0 /* FIXME */
  if (IsWindow(Wnd))
    {
      HWND* pWndArray;
      int i;

      if (!(pWndArray = WIN_ListChildren( hwnd ))) return;

      /* start from the end (FIXME: is this needed?) */
      for (i = 0; pWndArray[i]; i++) ;

      while (--i >= 0)
	{
	  if (IsWindow( pWndArray[i] )) WIN_SendDestroyMsg( pWndArray[i] );
	}
      HeapFree(GetProcessHeap(), 0, pWndArray);
    }
  else
    {
      DPRINT("destroyed itself while in WM_DESTROY!\n");
    }
#endif
}

static BOOLEAN W32kWndBelongsToThread(PWINDOW_OBJECT Window, PW32THREAD ThreadData)
{
  if (Window->OwnerThread && Window->OwnerThread->Win32Thread)
  {
    return (Window->OwnerThread->Win32Thread == ThreadData);
  }

  return FALSE;
}

static BOOL BuildChildWindowArray(PWINDOW_OBJECT Window, HWND **Children, unsigned *NumChildren)
{
  PLIST_ENTRY Current;
  unsigned Index;
  PWINDOW_OBJECT Child;

  *Children = NULL;
  *NumChildren = 0;
  ExAcquireFastMutexUnsafe(&Window->ChildrenListLock);
  Current = Window->ChildrenListHead.Flink;
  while (Current != &Window->ChildrenListHead)
    {
      (*NumChildren)++;
      Current = Current->Flink;
    }
  if (0 != *NumChildren)
    {
      *Children = ExAllocatePoolWithTag(PagedPool, *NumChildren * sizeof(HWND), TAG_WNAM);
      if (NULL != *Children)
	{
	  Current = Window->ChildrenListHead.Flink;
	  Index = 0;
	  while (Current != &Window->ChildrenListHead)
	    {
	      Child = CONTAINING_RECORD(Current, WINDOW_OBJECT, SiblingListEntry);
	      (*Children)[Index] = Child->Self;
	      Current = Current->Flink;
	      Index++;
	    }
	  assert(Index == *NumChildren);
	}
      else
	{
	  DPRINT1("Failed to allocate memory for children array\n");
	}
    }
  ExReleaseFastMutexUnsafe(&Window->ChildrenListLock);

  return 0 == *NumChildren || NULL != *Children;
}

/***********************************************************************
 *           W32kDestroyWindow
 *
 * Destroy storage associated to a window. "Internals" p.358
 */
static LRESULT W32kDestroyWindow(PWINDOW_OBJECT Window,
                                 PW32PROCESS ProcessData,
                                 PW32THREAD ThreadData,
                                 BOOLEAN SendMessages)
{
  HWND *Children;
  unsigned NumChildren;
  unsigned Index;
  PWINDOW_OBJECT Child;

  if (! W32kWndBelongsToThread(Window, ThreadData))
    {
      DPRINT1("Window doesn't belong to current thread\n");
      return 0;
    }

  /* free child windows */
  if (! BuildChildWindowArray(Window, &Children, &NumChildren))
    {
      return 0;
    }
  for (Index = NumChildren; 0 < Index; Index--)
    {
      Child = W32kGetWindowObject(Children[Index - 1]);
      if (NULL != Child)
	{
	  if (W32kWndBelongsToThread(Child, ThreadData))
	    {
	      W32kDestroyWindow(Child, ProcessData, ThreadData, SendMessages);
	    }
#if 0 /* FIXME */
	  else
	    {
	      SendMessageW( list[i], WM_WINE_DESTROYWINDOW, 0, 0 );
	    }
#endif
	}
    }
  if (0 != NumChildren)
    {
      ExFreePool(Children);
    }

  if (SendMessages)
    {
      /*
       * Clear the update region to make sure no WM_PAINT messages will be
       * generated for this window while processing the WM_NCDESTROY.
       */
      PaintRedrawWindow(Window, NULL, 0,
                        RDW_VALIDATE | RDW_NOFRAME | RDW_NOERASE | RDW_NOINTERNALPAINT | RDW_NOCHILDREN,
                        0);

      /*
       * Send the WM_NCDESTROY to the window being destroyed.
       */
      NtUserSendMessage(Window->Self, WM_NCDESTROY, 0, 0);
    }

  /* FIXME: do we need to fake QS_MOUSEMOVE wakebit? */

#if 0 /* FIXME */
  WinPosCheckInternalPos(Window->Self);
  if (Window->Self == GetCapture())
    {
      ReleaseCapture();
    }

  /* free resources associated with the window */
  TIMER_RemoveWindowTimers(Window->Self);
#endif

#if 0 /* FIXME */
  if (0 == (Window->Style & WS_CHILD))
    {
      HMENU Menu = (HMENU) NtUserSetWindowLongW(Window->Self, GWL_ID, 0);
      if (NULL != Menu)
	{
	  DestroyMenu(Menu);
	}
    }
  if (Window->hSysMenu)
    {
      DestroyMenu(Window->hSysMenu);
      Window->hSysMenu = 0;
    }
  DCE_FreeWindowDCE(Window->Self);    /* Always do this to catch orphaned DCs */
  WINPROC_FreeProc(Window->winproc, WIN_PROC_WINDOW);
  CLASS_RemoveWindow(Window->Class);
#endif

  ExAcquireFastMutexUnsafe(&Window->Parent->ChildrenListLock);
  RemoveEntryList(&Window->SiblingListEntry);
  ExReleaseFastMutexUnsafe(&Window->Parent->ChildrenListLock);

  RemoveEntryList(&Window->DesktopListEntry);

  ExAcquireFastMutexUnsafe (&ThreadData->WindowListLock);
  RemoveEntryList(&Window->ThreadListEntry);
  ExReleaseFastMutexUnsafe (&ThreadData->WindowListLock);

  Window->Class = NULL;
  ObmCloseHandle(ProcessData->WindowStation->HandleTable, Window->Self);

  W32kGraphicsCheck(FALSE);

  return 0;
}

BOOLEAN STDCALL
NtUserDestroyWindow(HWND Wnd)
{
  PWINDOW_OBJECT Window;
  BOOLEAN isChild;
  HWND hWndFocus;

  Window = W32kGetWindowObject(Wnd);
  if (Window == NULL)
    {
      return FALSE;
    }

  /* Check for desktop window (has NULL parent) */
  if (NULL == Window->Parent)
    {
      W32kReleaseWindowObject(Window);
      SetLastWin32Error(ERROR_ACCESS_DENIED);
      return FALSE;
    }

  /* Look whether the focus is within the tree of windows we will
   * be destroying.
   */
  hWndFocus = W32kGetFocusWindow();
  if (hWndFocus == Wnd || W32kIsChildWindow(Wnd, hWndFocus))
    {
      HWND Parent = NtUserGetAncestor(Wnd, GA_PARENT);
      if (Parent == W32kGetDesktopWindow())
      	{
      	  Parent = NULL;
      	}
        W32kSetFocusWindow(Parent);
    }

  /* Call hooks */
#if 0 /* FIXME */
  if (HOOK_CallHooks(WH_CBT, HCBT_DESTROYWND, (WPARAM) hwnd, 0, TRUE))
    {
    return FALSE;
    }
#endif

  isChild = (0 != (Window->Style & WS_CHILD));

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
      HOOK_CallHooks( WH_SHELL, HSHELL_WINDOWDESTROYED, (WPARAM)hwnd, 0L, TRUE );
      /* FIXME: clean up palette - see "Internals" p.352 */
    }

  if (! IsWindow(Wnd))
    {
    return TRUE;
    }
#endif

  /* Hide the window */
  if (! WinPosShowWindow(Wnd, SW_HIDE ))
    {
#if 0 /* FIXME */
      if (hwnd == GetActiveWindow())
	{
	  WINPOS_ActivateOtherWindow( hwnd );
	}
#endif
    }

#if 0 /* FIXME */
  if (! IsWindow(Wnd))
    {
    return TRUE;
    }
#endif

  /* Recursively destroy owned windows */
#if 0 /* FIXME */
  if (! isChild)
    {
      for (;;)
	{
	  int i;
	  BOOL GotOne = FALSE;
	  HWND *list = WIN_ListChildren(GetDesktopWindow());
	  if (list)
	    {
	      for (i = 0; list[i]; i++)
		{
		  if (GetWindow(list[i], GW_OWNER) != Wnd)
		    {
		      continue;
		    }
		  if (WIN_IsCurrentThread(list[i]))
		    {
		      DestroyWindow(list[i]);
		      GotOne = TRUE;
		      continue;
		    }
		  WIN_SetOwner(list[i], NULL);
		}
	      HeapFree(GetProcessHeap(), 0, list);
	    }
	  if (! GotOne)
	    {
	      break;
	    }
	}
    }
#endif

  /* Send destroy messages */
  W32kSendDestroyMsg(Wnd);

#if 0 /* FIXME */
  if (!IsWindow(Wnd))
    {
      return TRUE;
    }
#endif

  /* Unlink now so we won't bother with the children later on */
#if 0 /* FIXME */
  WIN_UnlinkWindow( hwnd );
#endif

  /* Destroy the window storage */
  W32kDestroyWindow(Window, PsGetWin32Process(), PsGetWin32Thread(), TRUE);

  return TRUE;
}

VOID FASTCALL
DestroyThreadWindows(struct _ETHREAD *Thread)
{
  PLIST_ENTRY LastHead;
  PW32PROCESS Win32Process;
  PW32THREAD Win32Thread;
  PWINDOW_OBJECT Window;

  Win32Thread = Thread->Win32Thread;
  Win32Process = Thread->ThreadsProcess->Win32Process;
  ExAcquireFastMutexUnsafe(&Win32Thread->WindowListLock);
  LastHead = NULL;
  while (Win32Thread->WindowListHead.Flink != &(Win32Thread->WindowListHead) &&
         Win32Thread->WindowListHead.Flink != LastHead)
    {
      LastHead = Win32Thread->WindowListHead.Flink;
      Window = CONTAINING_RECORD(Win32Thread->WindowListHead.Flink, WINDOW_OBJECT, ThreadListEntry);
      ExReleaseFastMutexUnsafe(&Win32Thread->WindowListLock);
      W32kDestroyWindow(Window, Win32Process, Win32Thread, FALSE);
      ExAcquireFastMutexUnsafe(&Win32Thread->WindowListLock);
    }
  if (Win32Thread->WindowListHead.Flink == LastHead)
    {
      /* Window at head of list was not removed, should never happen, infinite loop */
      KEBUGCHECK(0);
    }
  ExReleaseFastMutexUnsafe(&Win32Thread->WindowListLock);
}

DWORD STDCALL
NtUserEndDeferWindowPosEx(DWORD Unknown0,
			  DWORD Unknown1)
{
  UNIMPLEMENTED
    
  return 0;
}

DWORD STDCALL
NtUserFillWindow(DWORD Unknown0,
		 DWORD Unknown1,
		 DWORD Unknown2,
		 DWORD Unknown3)
{
  UNIMPLEMENTED

  return 0;
}

/*
 * FUNCTION:
 *   Searches a window's children for a window with the specified
 *   class and name
 * ARGUMENTS:
 *   hwndParent	    = The window whose childs are to be searched. 
 *					  NULL = desktop
 *
 *   hwndChildAfter = Search starts after this child window. 
 *					  NULL = start from beginning
 *
 *   ucClassName    = Class name to search for
 *					  Reguired parameter.
 *
 *   ucWindowName   = Window name
 *					  ->Buffer == NULL = don't care
 *			  
 * RETURNS:
 *   The HWND of the window if it was found, otherwise NULL
 *
 * FIXME:
 *   Should use MmCopyFromCaller, we don't want an access violation in here
 *	 
 */

HWND STDCALL
NtUserFindWindowEx(HWND hwndParent,
		   HWND hwndChildAfter,
		   PUNICODE_STRING ucClassName,
		   PUNICODE_STRING ucWindowName)
{
  NTSTATUS status;
  HWND windowHandle;
  PWINDOW_OBJECT windowObject;
  PWINDOW_OBJECT ParentWindow;
  PLIST_ENTRY currentEntry;
  PWNDCLASS_OBJECT classObject;
  
  // Get a pointer to the class
  status = ClassReferenceClassByNameOrAtom(&classObject, ucClassName->Buffer);
  if (!NT_SUCCESS(status))
    {
      return NULL;
    }
  
  // If hwndParent==NULL use the desktop window instead
  if(!hwndParent)
      hwndParent = PsGetWin32Thread()->Desktop->DesktopWindow;

  // Get the object
  ParentWindow = W32kGetWindowObject(hwndParent);

  if(!ParentWindow)
    {
      ObmDereferenceObject(classObject);
      return NULL;      
    }

  ExAcquireFastMutexUnsafe (&ParentWindow->ChildrenListLock);
  currentEntry = ParentWindow->ChildrenListHead.Flink;

  if(hwndChildAfter)
    {
      while (currentEntry != &ParentWindow->ChildrenListHead)
	{
	  windowObject = CONTAINING_RECORD (currentEntry, WINDOW_OBJECT, 
					    SiblingListEntry);
	  
	  if(windowObject->Self == hwndChildAfter)
	  {
	    /* "The search begins with the _next_ child window in the Z order." */
	    currentEntry = currentEntry->Flink;
	    break;
	  }

	  currentEntry = currentEntry->Flink;
	}

	/* If the child hwndChildAfter was not found:
	  currentEntry=&ParentWindow->ChildrenListHead now so the next
	  block of code will just fall through and the function returns NULL */
    }

  while (currentEntry != &ParentWindow->ChildrenListHead)
    {
      windowObject = CONTAINING_RECORD (currentEntry, WINDOW_OBJECT, 
					SiblingListEntry);

      if (classObject == windowObject->Class && (ucWindowName->Buffer==NULL || 
	  RtlCompareUnicodeString (ucWindowName, &windowObject->WindowName, TRUE) == 0))
	{
	  windowHandle = windowObject->Self;

	  ExReleaseFastMutexUnsafe (&ParentWindow->ChildrenListLock);
	  W32kReleaseWindowObject(ParentWindow);
	  ObmDereferenceObject (classObject);
      
	  return  windowHandle;
	}
      currentEntry = currentEntry->Flink;
  }

  ExReleaseFastMutexUnsafe (&ParentWindow->ChildrenListLock);
  W32kReleaseWindowObject(ParentWindow);
  ObmDereferenceObject (classObject);

  return  NULL;
}

DWORD STDCALL
NtUserFlashWindowEx(DWORD Unknown0)
{
  UNIMPLEMENTED

  return 0;
}

DWORD STDCALL
NtUserGetForegroundWindow(VOID)
{
  UNIMPLEMENTED

  return 0;
}

DWORD STDCALL
NtUserGetInternalWindowPos(DWORD Unknown0,
			   DWORD Unknown1,
			   DWORD Unknown2)
{
  UNIMPLEMENTED

  return 0;
}

DWORD STDCALL
NtUserGetOpenClipboardWindow(VOID)
{
  UNIMPLEMENTED

  return 0;
}

DWORD STDCALL
NtUserGetWindowDC(HWND hWnd)
{
  return (DWORD) NtUserGetDCEx( hWnd, 0, DCX_USESTYLE | DCX_WINDOW );
}

DWORD STDCALL
NtUserGetWindowPlacement(DWORD Unknown0,
			 DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

DWORD STDCALL
NtUserInternalGetWindowText(DWORD Unknown0,
			    DWORD Unknown1,
			    DWORD Unknown2)
{
  UNIMPLEMENTED

  return 0;
}

DWORD STDCALL
NtUserLockWindowUpdate(DWORD Unknown0)
{
  UNIMPLEMENTED

  return 0;
}

BOOL STDCALL
NtUserMoveWindow(      
    HWND hWnd,
    int X,
    int Y,
    int nWidth,
    int nHeight,
    BOOL bRepaint)
{
	UINT	flags = SWP_NOZORDER | SWP_NOACTIVATE;

	if(!bRepaint)
		flags |= SWP_NOREDRAW;
	return NtUserSetWindowPos(hWnd, 0, X, Y, nWidth, nHeight, flags);
}

/*
	QueryWindow based on KJK::Hyperion and James Tabor.

	0 = QWUniqueProcessId
	1 = QWUniqueThreadId
	4 = QWIsHung            Implements IsHungAppWindow found
                                by KJK::Hyperion.

        9 = QWKillWindow        When I called this with hWnd ==
                                DesktopWindow, it shutdown the system
                                and rebooted.
*/
DWORD STDCALL
NtUserQueryWindow(HWND hWnd, DWORD Index)
{

PWINDOW_OBJECT Window = W32kGetWindowObject(hWnd);

        if(Window == NULL) return((DWORD)NULL);

        W32kReleaseWindowObject(Window);

        switch(Index)
        {
        case 0x00:
                return((DWORD)Window->OwnerThread->ThreadsProcess->UniqueProcessId);

        case 0x01:
                return((DWORD)Window->OwnerThread->Cid.UniqueThread);

        default:
                return((DWORD)NULL);
        }

}

DWORD STDCALL
NtUserRealChildWindowFromPoint(DWORD Unknown0,
			       DWORD Unknown1,
			       DWORD Unknown2)
{
  UNIMPLEMENTED

  return 0;
}

BOOL
STDCALL
NtUserRedrawWindow
(
 HWND hWnd,
 CONST RECT *lprcUpdate,
 HRGN hrgnUpdate,
 UINT flags
)
{
 RECT SafeUpdateRect;
 NTSTATUS Status;
 PWINDOW_OBJECT Wnd;

 if (!(Wnd = W32kGetWindowObject(hWnd)))
 {
   SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
   return FALSE;
 }

 if(NULL != lprcUpdate)
 {
  Status = MmCopyFromCaller(&SafeUpdateRect, (PRECT)lprcUpdate, sizeof(RECT));

  if(!NT_SUCCESS(Status))
  {
   /* FIXME: set last error */
   return FALSE;
  }
 }
 
 Status = PaintRedrawWindow
 (
  Wnd,
  NULL == lprcUpdate ? NULL : &SafeUpdateRect,
  hrgnUpdate,
  flags,
  0
 );

 if(!NT_SUCCESS(Status))
 {
  /* FIXME: set last error */
  return FALSE;
 }
 
 return TRUE;
}

UINT STDCALL
NtUserRegisterWindowMessage(PUNICODE_STRING MessageNameUnsafe)
{
  PLIST_ENTRY Current;
  PREGISTERED_MESSAGE NewMsg, RegMsg;
  UINT Msg = REGISTERED_MESSAGE_MIN;
  UNICODE_STRING MessageName;
  NTSTATUS Status;

  Status = MmCopyFromCaller(&MessageName, MessageNameUnsafe, sizeof(UNICODE_STRING));
  if (! NT_SUCCESS(Status))
    {
      SetLastNtError(Status);
      return 0;
    }

  NewMsg = ExAllocatePoolWithTag(PagedPool,
                                 sizeof(REGISTERED_MESSAGE) +
                                 MessageName.Length,
                                 TAG_WNAM);
  if (NULL == NewMsg)
    {
      SetLastNtError(STATUS_NO_MEMORY);
      return 0;
    }

  Status = MmCopyFromCaller(NewMsg->MessageName, MessageName.Buffer, MessageName.Length);
  if (! NT_SUCCESS(Status))
    {
      ExFreePool(NewMsg);
      SetLastNtError(Status);
      return 0;
    }
  NewMsg->MessageName[MessageName.Length / sizeof(WCHAR)] = L'\0';
  if (wcslen(NewMsg->MessageName) != MessageName.Length / sizeof(WCHAR))
    {
      ExFreePool(NewMsg);
      SetLastNtError(STATUS_INVALID_PARAMETER);
      return 0;
    }

  Current = RegisteredMessageListHead.Flink;
  while (Current != &RegisteredMessageListHead)
    {
      RegMsg = CONTAINING_RECORD(Current, REGISTERED_MESSAGE, ListEntry);
      if (0 == wcscmp(NewMsg->MessageName, RegMsg->MessageName))
	{
	  ExFreePool(NewMsg);
	  return Msg;
	}
      Msg++;
      Current = Current->Flink;
    }

  if (REGISTERED_MESSAGE_MAX < Msg)
    {
      ExFreePool(NewMsg);
      SetLastNtError(STATUS_INSUFFICIENT_RESOURCES);
      return 0;
    }

  InsertTailList(&RegisteredMessageListHead, &(NewMsg->ListEntry));

  return Msg;
}

DWORD STDCALL
NtUserScrollWindowEx(DWORD Unknown0,
		     DWORD Unknown1,
		     DWORD Unknown2,
		     DWORD Unknown3,
		     DWORD Unknown4,
		     DWORD Unknown5,
		     DWORD Unknown6,
		     DWORD Unknown7)
{
  UNIMPLEMENTED

  return 0;
}

DWORD STDCALL
NtUserSetImeOwnerWindow(DWORD Unknown0,
			DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

DWORD STDCALL
NtUserSetInternalWindowPos(DWORD Unknown0,
			   DWORD Unknown1,
			   DWORD Unknown2,
			   DWORD Unknown3)
{
  UNIMPLEMENTED

  return 0;

}

DWORD STDCALL
NtUserSetLayeredWindowAttributes(DWORD Unknown0,
				 DWORD Unknown1,
				 DWORD Unknown2,
				 DWORD Unknown3)
{
  UNIMPLEMENTED

  return 0;
}

DWORD STDCALL
NtUserSetLogonNotifyWindow(DWORD Unknown0)
{
  UNIMPLEMENTED

  return 0;
}

DWORD STDCALL
NtUserSetShellWindowEx(DWORD Unknown0,
		       DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

DWORD STDCALL
NtUserSetWindowFNID(DWORD Unknown0,
		    DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

LONG STDCALL
NtUserGetWindowLong(HWND hWnd, DWORD Index, BOOL Ansi)
{
  PWINDOW_OBJECT WindowObject;
  NTSTATUS Status;
  LONG Result;

  Status = 
    ObmReferenceObjectByHandle(PsGetWin32Process()->WindowStation->HandleTable,
			       hWnd,
			       otWindow,
			       (PVOID*)&WindowObject);
  if (!NT_SUCCESS(Status))
    {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return 0;
    }

  if (0 <= (int) Index)
    {
      if (WindowObject->ExtraDataSize - sizeof(LONG) < Index ||
          0 != Index % sizeof(LONG))
	{
	  SetLastWin32Error(ERROR_INVALID_PARAMETER);
	  return 0;
	}
      Result = WindowObject->ExtraData[Index / sizeof(LONG)];
    }
  else
    {
      switch (Index)
	{
	case GWL_EXSTYLE:
	  Result = WindowObject->ExStyle;
	  break;

	case GWL_STYLE:
	  Result = WindowObject->Style;
	  break;
    /* FIXME: need to return the "invalid" value if the caller asks for it
	   (but we cant do that because that breaks stuff in user32 which wont be fixable until we have an implementation of IsWindowUnicode available */
	case GWL_WNDPROC:
	  if (WindowObject->Unicode)
	  {
		Result = (LONG) WindowObject->WndProcW;
	  }
	  else
	  {
		Result = (LONG) WindowObject->WndProcA;
	  }
	  break;

	case GWL_HINSTANCE:
	  Result = (LONG) WindowObject->Instance;
	  break;

	case GWL_HWNDPARENT:
	  Result = (LONG) WindowObject->ParentHandle;
	  break;

	case GWL_ID:
	  Result = (LONG) WindowObject->IDMenu;
	  break;

	case GWL_USERDATA:
	  Result = WindowObject->UserData;
	  break;
    
	default:
	  DPRINT1("NtUserGetWindowLong(): Unsupported index %d\n", Index);
	  SetLastWin32Error(ERROR_INVALID_PARAMETER);
	  Result = 0;
	  break;
	}
    }

  ObmDereferenceObject(WindowObject);

  return Result;
}

LONG STDCALL
NtUserSetWindowLong(HWND hWnd, DWORD Index, LONG NewValue, BOOL Ansi)
{
  PWINDOW_OBJECT WindowObject;
  NTSTATUS Status;
  LONG OldValue;
  STYLESTRUCT Style;

  Status = 
    ObmReferenceObjectByHandle(PsGetWin32Process()->WindowStation->HandleTable,
			       hWnd,
			       otWindow,
			       (PVOID*)&WindowObject);
  if (!NT_SUCCESS(Status))
    {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return(0);
    }

  if (0 <= (int) Index)
    {
      if (WindowObject->ExtraDataSize - sizeof(LONG) < Index ||
          0 != Index % sizeof(LONG))
	{
	  SetLastWin32Error(ERROR_INVALID_PARAMETER);
	  return 0;
	}
      OldValue = WindowObject->ExtraData[Index / sizeof(LONG)];
      WindowObject->ExtraData[Index / sizeof(LONG)] = NewValue;
    }
  else
    {
      switch (Index)
	{
	case GWL_EXSTYLE:
	  OldValue = (LONG) WindowObject->ExStyle;
	  Style.styleOld = OldValue;
	  Style.styleNew = NewValue;
	  W32kSendSTYLECHANGINGMessage(hWnd, GWL_EXSTYLE, &Style);
	  WindowObject->ExStyle = (DWORD)Style.styleNew;
	  W32kSendSTYLECHANGEDMessage(hWnd, GWL_EXSTYLE, &Style);
	  break;

	case GWL_STYLE:
	  OldValue = (LONG) WindowObject->Style;
	  Style.styleOld = OldValue;
	  Style.styleNew = NewValue;
	  W32kSendSTYLECHANGINGMessage(hWnd, GWL_STYLE, &Style);
	  WindowObject->Style = (DWORD)Style.styleNew;
	  W32kSendSTYLECHANGEDMessage(hWnd, GWL_STYLE, &Style);
	  break;

	case GWL_WNDPROC:
	  /* FIXME: should check if window belongs to current process */
	  /*OldValue = (LONG) WindowObject->WndProc;
	  WindowObject->WndProc = (WNDPROC) NewValue;*/
	  OldValue = 0;
	  break;

	case GWL_HINSTANCE:
	  OldValue = (LONG) WindowObject->Instance;
	  WindowObject->Instance = (HINSTANCE) NewValue;
	  break;

	case GWL_HWNDPARENT:
	  OldValue = (LONG) WindowObject->ParentHandle;
	  WindowObject->ParentHandle = (HWND) NewValue;
	  /* FIXME: Need to update window lists of old and new parent */
	  UNIMPLEMENTED;
	  break;

	case GWL_ID:
	  OldValue = (LONG) WindowObject->IDMenu;
	  WindowObject->IDMenu = (UINT) NewValue;
	  break;

	case GWL_USERDATA:
	  OldValue = WindowObject->UserData;
	  WindowObject->UserData = NewValue;
	  break;
    
	default:
	  DPRINT1("NtUserSetWindowLong(): Unsupported index %d\n", Index);
	  SetLastWin32Error(ERROR_INVALID_PARAMETER);
	  OldValue = 0;
	  break;
	}
    }

  ObmDereferenceObject(WindowObject);
  return(OldValue);
}

DWORD STDCALL
NtUserSetWindowPlacement(DWORD Unknown0,
			 DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

BOOL 
STDCALL NtUserSetWindowPos(      
    HWND hWnd,
    HWND hWndInsertAfter,
    int X,
    int Y,
    int cx,
    int cy,
    UINT uFlags)
{
  return WinPosSetWindowPos(hWnd, hWndInsertAfter, X, Y, cx, cy, uFlags);
}

DWORD STDCALL
NtUserSetWindowRgn(DWORD Unknown0,
		   DWORD Unknown1,
		   DWORD Unknown2)
{
  UNIMPLEMENTED

  return 0;
}


WORD STDCALL
NtUserSetWindowWord(HWND hWnd, INT Index, WORD NewVal)
{
  UNIMPLEMENTED
  return 0;
}


BOOL STDCALL
NtUserShowWindow(HWND hWnd,
		 LONG nCmdShow)
{
  return(WinPosShowWindow(hWnd, nCmdShow));
}

DWORD STDCALL
NtUserShowWindowAsync(DWORD Unknown0,
		      DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

BOOL STDCALL NtUserUpdateWindow( HWND hWnd )
{
    PWINDOW_OBJECT pWindow = W32kGetWindowObject( hWnd);

    if (!pWindow)
        return FALSE;
    if (pWindow->UpdateRegion)
        NtUserSendMessage( hWnd, WM_PAINT,0,0);
    W32kReleaseWindowObject(pWindow);
    return TRUE;
}

DWORD STDCALL
NtUserUpdateLayeredWindow(DWORD Unknown0,
			  DWORD Unknown1,
			  DWORD Unknown2,
			  DWORD Unknown3,
			  DWORD Unknown4,
			  DWORD Unknown5,
			  DWORD Unknown6,
			  DWORD Unknown7,
			  DWORD Unknown8)
{
  UNIMPLEMENTED

  return 0;
}

DWORD STDCALL
NtUserWindowFromPoint(DWORD Unknown0,
		      DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

HWND STDCALL
NtUserGetDesktopWindow()
{
  return W32kGetDesktopWindow();
}

HWND STDCALL
NtUserGetCapture(VOID)
{
  PWINDOW_OBJECT Window;
  Window = W32kGetCaptureWindow();
  if (Window != NULL)
    {
      return(Window->Self);
    }
  else
    {
      return(NULL);
    }
}

HWND STDCALL
NtUserSetCapture(HWND Wnd)
{
  PWINDOW_OBJECT Window;
  PWINDOW_OBJECT Prev;

  Prev = W32kGetCaptureWindow();

  if (Prev != NULL)
    {
      W32kSendMessage(Prev->Self, WM_CAPTURECHANGED, 0L, (LPARAM)Wnd, FALSE);
    }

  if (Wnd == NULL)
    {
      W32kSetCaptureWindow(NULL);
    }
  else  
    {
      Window = W32kGetWindowObject(Wnd);
      W32kSetCaptureWindow(Window);
      W32kReleaseWindowObject(Window);
    }
  if (Prev != NULL)
    {
      return(Prev->Self);
    }
  else
    {
      return(NULL);
    }
}


DWORD FASTCALL
W32kGetWindowThreadProcessId(PWINDOW_OBJECT Wnd, PDWORD pid)
{
   if (pid) *pid = (DWORD) Wnd->OwnerThread->ThreadsProcess->UniqueProcessId;
   return (DWORD) Wnd->OwnerThread->Cid.UniqueThread;
   
}


DWORD STDCALL
NtUserGetWindowThreadProcessId(HWND hWnd, LPDWORD UnsafePid)
{
   PWINDOW_OBJECT Wnd;
   DWORD tid, pid;

   W32kAcquireWinStaLockShared();

   if (!(Wnd = W32kGetWindowObject(hWnd)))
   {
      W32kReleaseWinStaLock();
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      return 0;
   }

   tid = W32kGetWindowThreadProcessId(Wnd, &pid);
   W32kReleaseWinStaLock();
   
   if (UnsafePid) MmCopyToCaller(UnsafePid, &pid, sizeof(DWORD));
   
   return tid;
}


/*
 * As best as I can figure, this function is used by EnumWindows,
 * EnumChildWindows, EnumDesktopWindows, & EnumThreadWindows.
 *
 * It's supposed to build a list of HWNDs to return to the caller.
 * We can figure out what kind of list by what parameters are
 * passed to us.
 */
ULONG
STDCALL
NtUserBuildHwndList(
  HDESK hDesktop,
  HWND hwndParent,
  BOOLEAN bChildren,
  ULONG dwThreadId,
  ULONG lParam,
  HWND* pWnd,
  ULONG nBufSize)
{
  ULONG dwCount = 0;

  /* FIXME handle bChildren */
  if ( hwndParent )
    {
      PWINDOW_OBJECT WindowObject = NULL;
      PLIST_ENTRY ChildListEntry;

      WindowObject = W32kGetWindowObject ( hwndParent );
      if ( !WindowObject )
	{
	  DPRINT("Bad window handle 0x%x\n", hWnd);
	  SetLastWin32Error(ERROR_INVALID_HANDLE);
	  return 0;
	}

      ExAcquireFastMutex ( &WindowObject->ChildrenListLock );
      ChildListEntry = WindowObject->ChildrenListHead.Flink;
      while (ChildListEntry != &WindowObject->ChildrenListHead)
	{
	  PWINDOW_OBJECT Child;
	  Child = CONTAINING_RECORD(ChildListEntry, WINDOW_OBJECT, 
				    SiblingListEntry);
	  if ( pWnd && dwCount < nBufSize )
	    pWnd[dwCount] = Child->Self;
	  dwCount++;
	  ChildListEntry = ChildListEntry->Flink;
	}
      ExReleaseFastMutex ( &WindowObject->ChildrenListLock );
      W32kReleaseWindowObject ( WindowObject );
    }
  else if ( dwThreadId )
    {
      NTSTATUS Status;
      struct _ETHREAD* Thread;
      struct _EPROCESS* ThreadsProcess;
      struct _W32PROCESS*   Win32Process;
      struct _WINSTATION_OBJECT* WindowStation;
      PUSER_HANDLE_TABLE HandleTable;
      PLIST_ENTRY Current;
      PUSER_HANDLE_BLOCK Block = NULL;
      int i;

      Status = PsLookupThreadByThreadId ( (PVOID)dwThreadId, &Thread );
      if ( !NT_SUCCESS(Status) || !Thread )
	{
	  DPRINT("Bad ThreadId 0x%x\n", dwThreadId );
	  SetLastWin32Error(ERROR_INVALID_HANDLE);
	  return 0;
	}
      ThreadsProcess = Thread->ThreadsProcess;
      ASSERT(ThreadsProcess);
      Win32Process = ThreadsProcess->Win32Process;
      ASSERT(Win32Process);
      WindowStation = Win32Process->WindowStation;
      ASSERT(WindowStation);
      HandleTable = (PUSER_HANDLE_TABLE)(WindowStation->HandleTable);
      ASSERT(HandleTable);

      ExAcquireFastMutex(&HandleTable->ListLock);

      Current = HandleTable->ListHead.Flink;
      while ( Current != &HandleTable->ListHead )
	{
	  Block = CONTAINING_RECORD(Current, USER_HANDLE_BLOCK, ListEntry);
	  for ( i = 0; i < HANDLE_BLOCK_ENTRIES; i++ )
	    {
	      PVOID ObjectBody = Block->Handles[i].ObjectBody;
	      if ( ObjectBody )
	      {
		if ( pWnd && dwCount < nBufSize )
		  {
		    pWnd[dwCount] =
		      (HWND)ObmReferenceObjectByPointer ( ObjectBody, otWindow );
		  }
		dwCount++;
	      }
	    }
	  Current = Current->Flink;
	}

      ExReleaseFastMutex(&HandleTable->ListLock);
    }
  else
    {
      PDESKTOP_OBJECT DesktopObject = NULL;
      KIRQL OldIrql;
      PLIST_ENTRY WindowListEntry;

      if ( hDesktop )
	DesktopObject = W32kGetDesktopObject ( hDesktop );
      else
	DesktopObject = W32kGetActiveDesktop();
      if (!DesktopObject)
	{
	  DPRINT("Bad desktop handle 0x%x\n", hDesktop );
	  SetLastWin32Error(ERROR_INVALID_HANDLE);
	  return 0;
	}

      KeAcquireSpinLock ( &DesktopObject->Lock, &OldIrql );
      WindowListEntry = DesktopObject->WindowListHead.Flink;
      while ( WindowListEntry != &DesktopObject->WindowListHead )
	{
	  PWINDOW_OBJECT Child;
	  Child = CONTAINING_RECORD(WindowListEntry, WINDOW_OBJECT,
				    SiblingListEntry);
	  if ( pWnd && dwCount < nBufSize )
	    pWnd[dwCount] = Child->Self;
	  dwCount++;
	  WindowListEntry = WindowListEntry->Flink;
	}
      KeReleaseSpinLock ( &DesktopObject->Lock, OldIrql );
    }

  return dwCount;
}

VOID STDCALL
NtUserValidateRect(HWND hWnd, const RECT* Rect)
{
  (VOID)NtUserRedrawWindow(hWnd, Rect, 0, RDW_VALIDATE | RDW_NOCHILDREN);
}

/*
 * @unimplemented
 */
DWORD STDCALL
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


HWND STDCALL
NtUserGetWindow(HWND hWnd, UINT Relationship)
{
  PWINDOW_OBJECT Wnd;
  HWND hWndResult = NULL;

  W32kAcquireWinStaLockShared();

  if (!(Wnd = W32kGetWindowObject(hWnd)))
  {
    W32kReleaseWinStaLock();
    SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
    return NULL;
  }
  
  switch (Relationship)
  {
    case GW_HWNDFIRST:
      if (Wnd->Parent && Wnd->Parent->FirstChild)
      {
        hWndResult = Wnd->Parent->FirstChild->Self;
      }
      break;
    case GW_HWNDLAST:
      if (Wnd->Parent && Wnd->Parent->LastChild)
      {
        hWndResult = Wnd->Parent->LastChild->Self;
      }
      break;
    case GW_HWNDNEXT:
      if (Wnd->NextSibling)
      {
        hWndResult = Wnd->NextSibling->Self;
      }
      break;
    case GW_HWNDPREV:
      if (Wnd->PrevSibling)
      {
        hWndResult = Wnd->PrevSibling->Self;
      }
      break;
    case GW_OWNER:
      hWndResult = Wnd->hWndOwner;
      break;
    case GW_CHILD:
      if (Wnd->FirstChild)
      {
        hWndResult = Wnd->FirstChild->Self;
      }
      break;
  }

  W32kReleaseWinStaLock();

  return hWndResult;
}


HWND STDCALL
NtUserGetLastActivePopup(HWND hWnd)
{
  PWINDOW_OBJECT Wnd;
  HWND hWndLastPopup;

  W32kAcquireWinStaLockShared();

  if (!(Wnd = W32kGetWindowObject(hWnd)))
  {
    W32kReleaseWinStaLock();
    SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
    return NULL;
  }

  hWndLastPopup = Wnd->hWndLastPopup;

  W32kReleaseWinStaLock();

  return hWndLastPopup;
}


/*
 * @implemented
 */
BOOL
STDCALL
NtUserSetMenu(
  HWND hWnd,
  HMENU hMenu,
  BOOL bRepaint)
{
  PWINDOW_OBJECT WindowObject;
  PMENU_OBJECT MenuObject;
  WindowObject = W32kGetWindowObject((HWND)hWnd);
  if(!WindowObject)
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return FALSE;
  }
  
  if(hMenu)
  {
    /* assign new menu handle */
    MenuObject = W32kGetMenuObject((HWND)hMenu);
    if(!MenuObject)
    {
      W32kReleaseWindowObject(WindowObject);
      SetLastWin32Error(ERROR_INVALID_MENU_HANDLE);
      return FALSE;
    }
    
    WindowObject->Menu = hMenu;
  }
  else
  {
    /* remove the menu handle */
    WindowObject->Menu = 0;
  }
  
  W32kReleaseMenuObject(MenuObject);
  W32kReleaseWindowObject(WindowObject);
  
  /* FIXME (from wine)
  if(bRepaint)
  {
    if (IsWindowVisible(hWnd))
        SetWindowPos( hWnd, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE |
                      SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED );
  }
  */
  
  return TRUE;
}


/*
 * @unimplemented
 */
HMENU
STDCALL
NtUserGetSystemMenu(
  HWND hWnd,
  BOOL bRevert)
{
  UNIMPLEMENTED

  return 0;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
NtUserSetSystemMenu(
  HWND hWnd,
  HMENU hMenu)
{
  UNIMPLEMENTED

  return 0;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
NtUserCallHwndLock(
  HWND hWnd,
  DWORD Unknown1)
{
  UNIMPLEMENTED
  /* DrawMenuBar() calls it with Unknown1==0x55 */
  return 0;
}


/* EOF */
