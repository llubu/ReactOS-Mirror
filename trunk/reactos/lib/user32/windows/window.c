/* $Id: window.c,v 1.83 2003/11/30 20:03:47 navaraf Exp $
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
#include <string.h>
#include <user32/callback.h>
#include <user32/regcontrol.h>

#define NDEBUG
#include <debug.h>

static BOOL ControlsInitCalled = FALSE;

/* FUNCTIONS *****************************************************************/


NTSTATUS STDCALL
User32SendNCCALCSIZEMessageForKernel(PVOID Arguments, ULONG ArgumentLength)
{
  PSENDNCCALCSIZEMESSAGE_CALLBACK_ARGUMENTS CallbackArgs;
  SENDNCCALCSIZEMESSAGE_CALLBACK_RESULT Result;
  WNDPROC Proc;

  DPRINT("User32SendNCCALCSIZEMessageForKernel.\n");
  CallbackArgs = (PSENDNCCALCSIZEMESSAGE_CALLBACK_ARGUMENTS)Arguments;
  if (ArgumentLength != sizeof(SENDNCCALCSIZEMESSAGE_CALLBACK_ARGUMENTS))
    {
      DPRINT("Wrong length.\n");
      return(STATUS_INFO_LENGTH_MISMATCH);
    }
  Proc = (WNDPROC)NtUserGetWindowLong(CallbackArgs->Wnd, GWL_WNDPROC, FALSE);
  DPRINT("Proc %X\n", Proc);
  /* Call the window procedure; notice kernel messages are always unicode. */
  if (CallbackArgs->Validate)
    {
      Result.Result = CallWindowProcW(Proc, CallbackArgs->Wnd, WM_NCCALCSIZE, 
				      TRUE, 
				      (LPARAM)&CallbackArgs->Params);
      Result.Params = CallbackArgs->Params;
    }
  else
    {
      Result.Result = CallWindowProcW(Proc, CallbackArgs->Wnd, WM_NCCALCSIZE,
				      FALSE, (LPARAM)&CallbackArgs->Rect);
      Result.Rect = CallbackArgs->Rect;
    }
  DPRINT("Returning result %d.\n", Result);
  return(ZwCallbackReturn(&Result, sizeof(Result), STATUS_SUCCESS));
}


NTSTATUS STDCALL
User32SendGETMINMAXINFOMessageForKernel(PVOID Arguments, ULONG ArgumentLength)
{
  PSENDGETMINMAXINFO_CALLBACK_ARGUMENTS CallbackArgs;
  SENDGETMINMAXINFO_CALLBACK_RESULT Result;
  WNDPROC Proc;

  DPRINT("User32SendGETMINAXINFOMessageForKernel.\n");
  CallbackArgs = (PSENDGETMINMAXINFO_CALLBACK_ARGUMENTS)Arguments;
  if (ArgumentLength != sizeof(SENDGETMINMAXINFO_CALLBACK_ARGUMENTS))
    {
      DPRINT("Wrong length.\n");
      return(STATUS_INFO_LENGTH_MISMATCH);
    }
  Proc = (WNDPROC)NtUserGetWindowLong(CallbackArgs->Wnd, GWL_WNDPROC, FALSE);
  DPRINT("Proc %X\n", Proc);
  /* Call the window procedure; notice kernel messages are always unicode. */
  Result.Result = CallWindowProcW(Proc, CallbackArgs->Wnd, WM_GETMINMAXINFO, 
				  0, (LPARAM)&CallbackArgs->MinMaxInfo);
  Result.MinMaxInfo = CallbackArgs->MinMaxInfo;
  DPRINT("Returning result %d.\n", Result);
  return(ZwCallbackReturn(&Result, sizeof(Result), STATUS_SUCCESS));
}


NTSTATUS STDCALL
User32SendCREATEMessageForKernel(PVOID Arguments, ULONG ArgumentLength)
{
  PSENDCREATEMESSAGE_CALLBACK_ARGUMENTS CallbackArgs;
  WNDPROC Proc;
  LRESULT Result;

  DPRINT("User32SendCREATEMessageForKernel.\n");
  CallbackArgs = (PSENDCREATEMESSAGE_CALLBACK_ARGUMENTS)Arguments;
  if (ArgumentLength != sizeof(SENDCREATEMESSAGE_CALLBACK_ARGUMENTS))
    {
      DPRINT("Wrong length.\n");
      return(STATUS_INFO_LENGTH_MISMATCH);
    }
  Proc = (WNDPROC)NtUserGetWindowLong(CallbackArgs->Wnd, GWL_WNDPROC, FALSE);
  DPRINT("Proc %X\n", Proc);
  /* Call the window procedure; notice kernel messages are always unicode. */
  Result = CallWindowProcW(Proc, CallbackArgs->Wnd, WM_CREATE, 0, 
			   (LPARAM)&CallbackArgs->CreateStruct);
  DPRINT("Returning result %d.\n", Result);
  return(ZwCallbackReturn(&Result, sizeof(LRESULT), STATUS_SUCCESS));
}


NTSTATUS STDCALL
User32SendNCCREATEMessageForKernel(PVOID Arguments, ULONG ArgumentLength)
{
  PSENDNCCREATEMESSAGE_CALLBACK_ARGUMENTS CallbackArgs;
  WNDPROC Proc;
  LRESULT Result;

  DPRINT("User32SendNCCREATEMessageForKernel.\n");
  CallbackArgs = (PSENDNCCREATEMESSAGE_CALLBACK_ARGUMENTS)Arguments;
  if (ArgumentLength != sizeof(SENDNCCREATEMESSAGE_CALLBACK_ARGUMENTS))
    {
      DPRINT("Wrong length.\n");
      return(STATUS_INFO_LENGTH_MISMATCH);
    }
  Proc = (WNDPROC)NtUserGetWindowLong(CallbackArgs->Wnd, GWL_WNDPROC, FALSE);
  DPRINT("Proc %X\n", Proc);
  /* Call the window procedure; notice kernel messages are always unicode. */
  Result = CallWindowProcW(Proc, CallbackArgs->Wnd, WM_NCCREATE, 0, 
			   (LPARAM)&CallbackArgs->CreateStruct);
  DPRINT("Returning result %d.\n", Result);
  return(ZwCallbackReturn(&Result, sizeof(LRESULT), STATUS_SUCCESS));
}


NTSTATUS STDCALL
User32SendWINDOWPOSCHANGINGMessageForKernel(PVOID Arguments, ULONG ArgumentLength)
{
  PSENDWINDOWPOSCHANGING_CALLBACK_ARGUMENTS CallbackArgs;
  WNDPROC Proc;
  LRESULT Result;

  DPRINT("User32SendWINDOWPOSCHANGINGMessageForKernel.\n");
  CallbackArgs = (PSENDWINDOWPOSCHANGING_CALLBACK_ARGUMENTS)Arguments;
  if (ArgumentLength != sizeof(SENDWINDOWPOSCHANGING_CALLBACK_ARGUMENTS))
    {
      DPRINT("Wrong length.\n");
      return(STATUS_INFO_LENGTH_MISMATCH);
    }
  Proc = (WNDPROC)NtUserGetWindowLong(CallbackArgs->Wnd, GWL_WNDPROC, FALSE);
  DPRINT("Proc %X\n", Proc);
  /* Call the window procedure; notice kernel messages are always unicode. */
  Result = CallWindowProcW(Proc, CallbackArgs->Wnd, WM_WINDOWPOSCHANGING, 0, 
			   (LPARAM)&CallbackArgs->WindowPos);
  DPRINT("Returning result %d.\n", Result);
  return(ZwCallbackReturn(&Result, sizeof(LRESULT), STATUS_SUCCESS));
}


NTSTATUS STDCALL
User32SendWINDOWPOSCHANGEDMessageForKernel(PVOID Arguments, ULONG ArgumentLength)
{
  PSENDWINDOWPOSCHANGED_CALLBACK_ARGUMENTS CallbackArgs;
  WNDPROC Proc;
  LRESULT Result;

  DPRINT("User32SendWINDOWPOSCHANGEDMessageForKernel.\n");
  CallbackArgs = (PSENDWINDOWPOSCHANGED_CALLBACK_ARGUMENTS)Arguments;
  if (ArgumentLength != sizeof(SENDWINDOWPOSCHANGED_CALLBACK_ARGUMENTS))
    {
      DPRINT("Wrong length.\n");
      return(STATUS_INFO_LENGTH_MISMATCH);
    }
  Proc = (WNDPROC)NtUserGetWindowLong(CallbackArgs->Wnd, GWL_WNDPROC, FALSE);
  DPRINT("Proc %X\n", Proc);
  /* Call the window procedure; notice kernel messages are always unicode. */
  Result = CallWindowProcW(Proc, CallbackArgs->Wnd, WM_WINDOWPOSCHANGED, 0, 
			   (LPARAM)&CallbackArgs->WindowPos);
  DPRINT("Returning result %d.\n", Result);
  return(ZwCallbackReturn(&Result, sizeof(LRESULT), STATUS_SUCCESS));
}


NTSTATUS STDCALL
User32SendSTYLECHANGINGMessageForKernel(PVOID Arguments, ULONG ArgumentLength)
{
  PSENDSTYLECHANGING_CALLBACK_ARGUMENTS CallbackArgs;
  WNDPROC Proc;
  LRESULT Result;

  DPRINT("User32SendSTYLECHANGINGMessageForKernel.\n");
  CallbackArgs = (PSENDSTYLECHANGING_CALLBACK_ARGUMENTS)Arguments;
  if (ArgumentLength != sizeof(SENDSTYLECHANGING_CALLBACK_ARGUMENTS))
    {
      DPRINT("Wrong length.\n");
      return(STATUS_INFO_LENGTH_MISMATCH);
    }
  Proc = (WNDPROC)NtUserGetWindowLong(CallbackArgs->Wnd, GWL_WNDPROC, FALSE);
  DPRINT("Proc %X\n", Proc);
  /* Call the window procedure; notice kernel messages are always unicode. */
  Result = CallWindowProcW(Proc, CallbackArgs->Wnd, WM_STYLECHANGING, CallbackArgs->WhichStyle,
			   (LPARAM)&CallbackArgs->Style);
  DPRINT("Returning result %d.\n", Result);
  return(ZwCallbackReturn(&Result, sizeof(LRESULT), STATUS_SUCCESS));
}


NTSTATUS STDCALL
User32SendSTYLECHANGEDMessageForKernel(PVOID Arguments, ULONG ArgumentLength)
{
  PSENDSTYLECHANGED_CALLBACK_ARGUMENTS CallbackArgs;
  WNDPROC Proc;
  LRESULT Result;

  DPRINT("User32SendSTYLECHANGEDGMessageForKernel.\n");
  CallbackArgs = (PSENDSTYLECHANGED_CALLBACK_ARGUMENTS)Arguments;
  if (ArgumentLength != sizeof(SENDSTYLECHANGED_CALLBACK_ARGUMENTS))
    {
      DPRINT("Wrong length.\n");
      return(STATUS_INFO_LENGTH_MISMATCH);
    }
  Proc = (WNDPROC)NtUserGetWindowLong(CallbackArgs->Wnd, GWL_WNDPROC, FALSE);
  DPRINT("Proc %X\n", Proc);
  /* Call the window procedure; notice kernel messages are always unicode. */
  Result = CallWindowProcW(Proc, CallbackArgs->Wnd, WM_STYLECHANGED, CallbackArgs->WhichStyle,
			   (LPARAM)&CallbackArgs->Style);
  DPRINT("Returning result %d.\n", Result);
  return(ZwCallbackReturn(&Result, sizeof(LRESULT), STATUS_SUCCESS));
}


NTSTATUS STDCALL
User32CallSendAsyncProcForKernel(PVOID Arguments, ULONG ArgumentLength)
{
  PSENDASYNCPROC_CALLBACK_ARGUMENTS CallbackArgs;

  DPRINT("User32CallSendAsyncProcKernel()\n");
  CallbackArgs = (PSENDASYNCPROC_CALLBACK_ARGUMENTS)Arguments;
  if (ArgumentLength != sizeof(WINDOWPROC_CALLBACK_ARGUMENTS))
    {
      return(STATUS_INFO_LENGTH_MISMATCH);
    }
  CallbackArgs->Callback(CallbackArgs->Wnd, CallbackArgs->Msg,
			 CallbackArgs->Context, CallbackArgs->Result);
  return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
User32CallWindowProcFromKernel(PVOID Arguments, ULONG ArgumentLength)
{
  PWINDOWPROC_CALLBACK_ARGUMENTS CallbackArgs;
  LRESULT Result;

  CallbackArgs = (PWINDOWPROC_CALLBACK_ARGUMENTS)Arguments;
  if (ArgumentLength != sizeof(WINDOWPROC_CALLBACK_ARGUMENTS))
    {
      return(STATUS_INFO_LENGTH_MISMATCH);
    }
  if (CallbackArgs->Proc == NULL)
    {
      CallbackArgs->Proc = (WNDPROC)NtUserGetWindowLong(CallbackArgs->Wnd, GWL_WNDPROC, FALSE);
    }
  Result = CallWindowProcW(CallbackArgs->Proc, CallbackArgs->Wnd, 
			   CallbackArgs->Msg, CallbackArgs->wParam, 
			   CallbackArgs->lParam);
  return(ZwCallbackReturn(&Result, sizeof(LRESULT), STATUS_SUCCESS));
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
AllowSetForegroundWindow(DWORD dwProcessId)
{
  UNIMPLEMENTED;
  return(FALSE);
}


/*
 * @unimplemented
 */
UINT STDCALL
ArrangeIconicWindows(HWND hWnd)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
HDWP STDCALL
BeginDeferWindowPos(int nNumWindows)
{
#if 0
  UNIMPLEMENTED;
  return (HDWP)0;
#else
  return (HDWP)1;
#endif
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
BringWindowToTop(HWND hWnd)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
/*
WORD STDCALL
CascadeWindows(HWND hwndParent,
	       UINT wHow,
	       CONST RECT *lpRect,
	       UINT cKids,
	       const HWND *lpKids)
{
  UNIMPLEMENTED;
  return 0;
}
*/


/*
 * @unimplemented
 */
HWND STDCALL
ChildWindowFromPoint(HWND hWndParent,
		     POINT Point)
{
  DbgPrint("FIXME: ChildWindowFromPoint is not implemented\n");
  UNIMPLEMENTED;
  return (HWND)0;
}


/*
 * @unimplemented
 */
HWND STDCALL
ChildWindowFromPointEx(HWND hwndParent,
		       POINT pt,
		       UINT uFlags)
{
  DbgPrint("FIXME: ChildWindowFromPointEx is not implemented\n");
  UNIMPLEMENTED;
  return (HWND)0;
}


/*
 * @implemented
 */
WINBOOL STDCALL
CloseWindow(HWND hWnd)
{
    SendMessageA(hWnd, WM_SYSCOMMAND, SC_CLOSE, 0);

    return (WINBOOL)(hWnd);
}

/*
 * @implemented
 */
HWND STDCALL
CreateWindowExA(DWORD dwExStyle,
		LPCSTR lpClassName,
		LPCSTR lpWindowName,
		DWORD dwStyle,
		int x,
		int y,
		int nWidth,
		int nHeight,
		HWND hWndParent,
		HMENU hMenu,
		HINSTANCE hInstance,
		LPVOID lpParam)
{
  UNICODE_STRING WindowName;
  UNICODE_STRING ClassName;
  WNDCLASSEXA wce;
  HWND Handle;
  INT sw;

#if 0
  DbgPrint("[window] CreateWindowExA style %d, exstyle %d, parent %d\n", dwStyle, dwExStyle, hWndParent);
#endif

  /* Register built-in controls if not already done */
  if (! ControlsInitCalled)
    {
      ControlsInit();
      ControlsInitCalled = TRUE;
    }

  if (IS_ATOM(lpClassName))
    {
      RtlInitUnicodeString(&ClassName, NULL);
      ClassName.Buffer = (LPWSTR)lpClassName;
    }
  else
    {
      if (!RtlCreateUnicodeStringFromAsciiz(&(ClassName), (PCSZ)lpClassName))
	{
	  SetLastError(ERROR_OUTOFMEMORY);
	  return (HWND)0;
	}
    }

  if (!RtlCreateUnicodeStringFromAsciiz(&WindowName, (PCSZ)lpWindowName))
    {
      if (!IS_ATOM(lpClassName))
	{
	  RtlFreeUnicodeString(&ClassName);
	}
      SetLastError(ERROR_OUTOFMEMORY);
      return (HWND)0;
    }

  /* Fixup default coordinates. */
  sw = SW_SHOW;
  if (x == (LONG) CW_USEDEFAULT || nWidth == (LONG) CW_USEDEFAULT)
    {
      if (dwStyle & (WS_CHILD | WS_POPUP))
	{
	  if (x == (LONG) CW_USEDEFAULT)
	    {
	      x = y = 0;
	    }
	  if (nWidth == (LONG) CW_USEDEFAULT)
	    {
	      nWidth = nHeight = 0;
	    }
	}
      else
	{
	  STARTUPINFOA info;

	  GetStartupInfoA(&info);

	  if (x == (LONG) CW_USEDEFAULT)
	    {
	      if (y != (LONG) CW_USEDEFAULT)
		{
		  sw = y;
		}
	      x = (info.dwFlags & STARTF_USEPOSITION) ? info.dwX : 0;
	      y = (info.dwFlags & STARTF_USEPOSITION) ? info.dwY : 0;
	    }
	  
	  if (nWidth == (LONG) CW_USEDEFAULT)
	    {
	      if (info.dwFlags & STARTF_USESIZE)
		{
		  nWidth = info.dwXSize;
		  nHeight = info.dwYSize;
		}
	      else
		{
		  RECT r;

		  SystemParametersInfoA(SPI_GETWORKAREA, 0, &r, 0);
		  nWidth = (((r.right - r.left) * 3) / 4) - x;
		  nHeight = (((r.bottom - r.top) * 3) / 4) - y;
		}
	    }
	}
    }
    
  if(!hMenu && (dwStyle & (WS_OVERLAPPEDWINDOW | WS_POPUP)))
  {
    wce.cbSize = sizeof(WNDCLASSEXA);
    if(GetClassInfoExA(hInstance, lpClassName, &wce) && wce.lpszMenuName)
    {
      hMenu = LoadMenuA(hInstance, wce.lpszMenuName);
    }
  }

  Handle = NtUserCreateWindowEx(dwExStyle,
				&ClassName,
				&WindowName,
				dwStyle,
				x,
				y,
				nWidth,
				nHeight,
				hWndParent,
				hMenu,
				hInstance,
				lpParam,
				sw,
				FALSE);

#if 0
  DbgPrint("[window] NtUserCreateWindowEx() == %d\n", Handle);
#endif

  RtlFreeUnicodeString(&WindowName);

  if (!IS_ATOM(lpClassName)) 
    {
      RtlFreeUnicodeString(&ClassName);
    }
  
  return Handle;
}


/*
 * @implemented
 */
HWND STDCALL
CreateWindowExW(DWORD dwExStyle,
		LPCWSTR lpClassName,
		LPCWSTR lpWindowName,
		DWORD dwStyle,
		int x,
		int y,
		int nWidth,
		int nHeight,
		HWND hWndParent,
		HMENU hMenu,
		HINSTANCE hInstance,
		LPVOID lpParam)
{
  UNICODE_STRING WindowName;
  UNICODE_STRING ClassName;
  WNDCLASSEXW wce;
  HANDLE Handle;
  UINT sw;

  /* Register built-in controls if not already done */
  if (! ControlsInitCalled)
    {
      ControlsInit();
      ControlsInitCalled = TRUE;
    }

  if (IS_ATOM(lpClassName)) 
    {
      RtlInitUnicodeString(&ClassName, NULL);
      ClassName.Buffer = (LPWSTR)lpClassName;
    } 
  else 
    {
      RtlInitUnicodeString(&ClassName, lpClassName);
    }

  RtlInitUnicodeString(&WindowName, lpWindowName);

  /* Fixup default coordinates. */
  sw = SW_SHOW;
  if (x == (LONG) CW_USEDEFAULT || nWidth == (LONG) CW_USEDEFAULT)
    {
      if (dwStyle & (WS_CHILD | WS_POPUP))
	{
	  if (x == (LONG) CW_USEDEFAULT)
	    {
	      x = y = 0;
	    }
	  if (nWidth == (LONG) CW_USEDEFAULT)
	    {
	      nWidth = nHeight = 0;
	    }
	}
      else
	{
	  STARTUPINFOW info;

	  GetStartupInfoW(&info);

	  if (x == (LONG) CW_USEDEFAULT)
	    {
	      if (y != (LONG) CW_USEDEFAULT)
		{
		  sw = y;
		}
	      x = (info.dwFlags & STARTF_USEPOSITION) ? info.dwX : 0;
	      y = (info.dwFlags & STARTF_USEPOSITION) ? info.dwY : 0;
	    }
	  
	  if (nWidth == (LONG) CW_USEDEFAULT)
	    {
	      if (info.dwFlags & STARTF_USESIZE)
		{
		  nWidth = info.dwXSize;
		  nHeight = info.dwYSize;
		}
	      else
		{
		  RECT r;

		  SystemParametersInfoW(SPI_GETWORKAREA, 0, &r, 0);
		  nWidth = (((r.right - r.left) * 3) / 4) - x;
		  nHeight = (((r.bottom - r.top) * 3) / 4) - y;
		}
	    }
	}
    }

  if(!hMenu && (dwStyle & (WS_OVERLAPPEDWINDOW | WS_POPUP)))
  {
    wce.cbSize = sizeof(WNDCLASSEXW);
    if(GetClassInfoExW(hInstance, lpClassName, &wce) && wce.lpszMenuName)
    {
      hMenu = LoadMenuW(hInstance, wce.lpszMenuName);
    }
  }

  Handle = NtUserCreateWindowEx(dwExStyle,
				&ClassName,
				&WindowName,
				dwStyle,
				x,
				y,
				nWidth,
				nHeight,
				hWndParent,
				hMenu,
				hInstance,
				lpParam,
				sw,
				TRUE);

  return (HWND)Handle;
}


/*
 * @unimplemented
 */
HDWP STDCALL
DeferWindowPos(HDWP hWinPosInfo,
	       HWND hWnd,
	       HWND hWndInsertAfter,
	       int x,
	       int y,
	       int cx,
	       int cy,
	       UINT uFlags)
{
#if 0
  return NtUserDeferWindowPos(hWinPosInfo, hWnd, hWndInsertAfter, x, y, cx, cy, uFlags);
#else
  SetWindowPos(hWnd, hWndInsertAfter, x, y, cx, cy, uFlags);
  return hWinPosInfo;
#endif
}


/*
 * @implemented
 */
WINBOOL STDCALL
DestroyWindow(HWND hWnd)
{
  return NtUserDestroyWindow(hWnd);
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
EndDeferWindowPos(HDWP hWinPosInfo)
{
#if 0
  UNIMPLEMENTED;
  return FALSE;
#else
  return TRUE;
#endif
}


/*
 * @implemented
 */
HWND STDCALL
GetDesktopWindow(VOID)
{
	return NtUserGetDesktopWindow();
}


/*
 * @unimplemented
 */
HWND STDCALL
GetForegroundWindow(VOID)
{
   return NtUserGetForegroundWindow();
}


WINBOOL
STATIC
User32EnumWindows (
	HDESK hDesktop,
	HWND hWndparent,
	ENUMWINDOWSPROC lpfn,
	LPARAM lParam,
	DWORD dwThreadId,
	BOOL bChildren )
{
  DWORD i, dwCount = 0;
  HWND* pHwnd = NULL;
  HANDLE hHeap;

  if ( !lpfn )
    {
      SetLastError ( ERROR_INVALID_PARAMETER );
      return FALSE;
    }

  /* FIXME instead of always making two calls, should we use some
     sort of persistent buffer and only grow it ( requiring a 2nd
     call ) when the buffer wasn't already big enough? */
  /* first get how many window entries there are */
  SetLastError(0);
  dwCount = NtUserBuildHwndList (
    hDesktop, hWndparent, bChildren, dwThreadId, lParam, NULL, 0 );
  if ( !dwCount || GetLastError() )
    return FALSE;

  /* allocate buffer to receive HWND handles */
  hHeap = GetProcessHeap();
  pHwnd = HeapAlloc ( hHeap, 0, sizeof(HWND)*(dwCount+1) );
  if ( !pHwnd )
    {
      SetLastError ( ERROR_NOT_ENOUGH_MEMORY );
      return FALSE;
    }

  /* now call kernel again to fill the buffer this time */
  dwCount = NtUserBuildHwndList (
    hDesktop, hWndparent, bChildren, dwThreadId, lParam, pHwnd, dwCount );
  if ( !dwCount || GetLastError() )
    {
      if ( pHwnd )
	HeapFree ( hHeap, 0, pHwnd );
      return FALSE;
    }

  /* call the user's callback function until we're done or
     they tell us to quit */
  for ( i = 0; i < dwCount; i++ )
  {
    /* FIXME I'm only getting NULLs from Thread Enumeration, and it's
     * probably because I'm not doing it right in NtUserBuildHwndList.
     * Once that's fixed, we shouldn't have to check for a NULL HWND
     * here
     */
    if ( !(ULONG)pHwnd[i] ) /* don't enumerate a NULL HWND */
      continue;
    if ( !(*lpfn)( pHwnd[i], lParam ) )
    {
      HeapFree ( hHeap, 0, pHwnd );
      return FALSE;
    }
  }
  if ( pHwnd )
    HeapFree ( hHeap, 0, pHwnd );
  return TRUE;
}


/*
 * @implemented
 */
WINBOOL
STDCALL
EnumChildWindows(
	HWND hWndParent,
	ENUMWINDOWSPROC lpEnumFunc,
	LPARAM lParam)
{
  if ( !hWndParent )
    hWndParent = GetDesktopWindow();
  return User32EnumWindows ( NULL, hWndParent, lpEnumFunc, lParam, 0, FALSE );
}


/*
 * @implemented
 */
WINBOOL
STDCALL
EnumThreadWindows(DWORD dwThreadId,
		  ENUMWINDOWSPROC lpfn,
		  LPARAM lParam)
{
  if ( !dwThreadId )
    dwThreadId = GetCurrentThreadId();
  return User32EnumWindows ( NULL, NULL, lpfn, lParam, dwThreadId, FALSE );
}


/*
 * @implemented
 */
WINBOOL STDCALL
EnumWindows(ENUMWINDOWSPROC lpEnumFunc,
	    LPARAM lParam)
{
  return User32EnumWindows ( NULL, NULL, lpEnumFunc, lParam, 0, FALSE );
}


/*
 * @implemented
 */
WINBOOL
STDCALL
EnumDesktopWindows(
	HDESK hDesktop,
	ENUMWINDOWSPROC lpfn,
	LPARAM lParam)
{
  return User32EnumWindows ( hDesktop, NULL, lpfn, lParam, 0, FALSE );
}


/*
 * @unimplemented
 */
HWND STDCALL
FindWindowExA(HWND hwndParent,
	      HWND hwndChildAfter,
	      LPCSTR lpszClass,
	      LPCSTR lpszWindow)
{
  UNIMPLEMENTED;
  return (HWND)0;
}


/*
 * @implemented
 */
HWND STDCALL
FindWindowExW(HWND hwndParent,
	      HWND hwndChildAfter,
	      LPCWSTR lpszClass,
	      LPCWSTR lpszWindow)
{
	UNICODE_STRING ucClassName;
	UNICODE_STRING ucWindowName;

	if (IS_ATOM(lpszClass)) 
	{
		RtlInitUnicodeString(&ucClassName, NULL);
		ucClassName.Buffer = (LPWSTR)lpszClass;
    } 
	else 
    {
		RtlInitUnicodeString(&ucClassName, lpszClass);
    }

	// Window names can't be atoms, and if lpszWindow = NULL,
	// RtlInitUnicodeString will clear it
	
	RtlInitUnicodeString(&ucWindowName, lpszWindow);


	return NtUserFindWindowEx(hwndParent, hwndChildAfter, &ucClassName, &ucWindowName);
}


/*
 * @implemented
 */
HWND STDCALL
FindWindowA(LPCSTR lpClassName, LPCSTR lpWindowName)
{
  //FIXME: FindWindow does not search children, but FindWindowEx does.
  //       what should we do about this?
  return FindWindowExA (NULL, NULL, lpClassName, lpWindowName);
}


/*
 * @implemented
 */
HWND STDCALL
FindWindowW(LPCWSTR lpClassName, LPCWSTR lpWindowName)
{
  /* 
  
  There was a FIXME here earlier, but I think it is just a documentation unclarity.

  FindWindow only searches top level windows. What they mean is that child 
  windows of other windows than the desktop can be searched. 
  FindWindowExW never does a recursive search.
  
	/ Joakim
  */

  return FindWindowExW (NULL, NULL, lpClassName, lpWindowName);
}



/*
 * @unimplemented
 */
WINBOOL STDCALL
GetAltTabInfoA(HWND hwnd,
	       int iItem,
	       PALTTABINFO pati,
	       LPSTR pszItemText,
	       UINT cchItemText)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
GetAltTabInfoW(HWND hwnd,
	       int iItem,
	       PALTTABINFO pati,
	       LPWSTR pszItemText,
	       UINT cchItemText)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @implemented
 */
HWND STDCALL
GetAncestor(HWND hwnd, UINT gaFlags)
{
  return(NtUserGetAncestor(hwnd, gaFlags));
}


/*
 * @implemented
 */
WINBOOL STDCALL
GetClientRect(HWND hWnd, LPRECT lpRect)
{
  return(NtUserGetClientRect(hWnd, lpRect));
}


/*
 * @implemented
 */
WINBOOL STDCALL
GetGUIThreadInfo(DWORD idThread,
		 LPGUITHREADINFO lpgui)
{
  return (WINBOOL)NtUserGetGUIThreadInfo(idThread, lpgui);
}


/*
 * @unimplemented
 */
HWND STDCALL
GetLastActivePopup(HWND hWnd)
{
  return NtUserGetLastActivePopup(hWnd);
}


/*
 * @implemented
 */
HWND STDCALL
GetParent(HWND hWnd)
{
  return NtUserGetParent(hWnd);
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
GetProcessDefaultLayout(DWORD *pdwDefaultLayout)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
GetTitleBarInfo(HWND hwnd,
		PTITLEBARINFO pti)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @implemented
 */
HWND STDCALL
GetWindow(HWND hWnd,
	  UINT uCmd)
{
  return NtUserGetWindow(hWnd, uCmd);
}


/*
 * @implemented
 */
HWND STDCALL
GetTopWindow(HWND hWnd)
{
  if (!hWnd) hWnd = GetDesktopWindow();
  return GetWindow( hWnd, GW_CHILD );
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
GetWindowInfo(HWND hwnd,
	      PWINDOWINFO pwi)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
UINT STDCALL
GetWindowModuleFileName(HWND hwnd,
			LPSTR lpszFileName,
			UINT cchFileNameMax)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
UINT STDCALL
GetWindowModuleFileNameA(HWND hwnd,
			 LPSTR lpszFileName,
			 UINT cchFileNameMax)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
UINT STDCALL
GetWindowModuleFileNameW(HWND hwnd,
			 LPWSTR lpszFileName,
			 UINT cchFileNameMax)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
GetWindowPlacement(HWND hWnd,
		   WINDOWPLACEMENT *lpwndpl)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @implemented
 */
WINBOOL STDCALL
GetWindowRect(HWND hWnd,
	      LPRECT lpRect)
{
  return(NtUserGetWindowRect(hWnd, lpRect));
}


/*
 * @implemented
 */
int STDCALL
GetWindowTextA(HWND hWnd, LPSTR lpString, int nMaxCount)
{
  return(SendMessageA(hWnd, WM_GETTEXT, nMaxCount, (LPARAM)lpString));
}


/*
 * @implemented
 */
int STDCALL
GetWindowTextLengthA(HWND hWnd)
{
  return(SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0));
}


/*
 * @implemented
 */
int STDCALL
GetWindowTextLengthW(HWND hWnd)
{
  return(SendMessageW(hWnd, WM_GETTEXTLENGTH, 0, 0));
}


/*
 * @implemented
 */
int STDCALL
GetWindowTextW(
	HWND hWnd,
	LPWSTR lpString,
	int nMaxCount)
{
  return(SendMessageW(hWnd, WM_GETTEXT, nMaxCount, (LPARAM)lpString));
}

DWORD STDCALL
GetWindowThreadProcessId(HWND hWnd,
			 LPDWORD lpdwProcessId)
{
   return NtUserGetWindowThreadProcessId(hWnd, lpdwProcessId);
}


/*
 * @implemented
 */
WINBOOL STDCALL
IsChild(HWND hWndParent,
	HWND hWnd)
{
    // Untested
    return ((HWND)NtUserGetWindowLong(hWnd, GWL_HWNDPARENT, FALSE)) == hWndParent;
}


/*
 * @implemented
 */
WINBOOL STDCALL
IsIconic(HWND hWnd)
{
  return (NtUserGetWindowLong( hWnd, GWL_STYLE, FALSE) & WS_MINIMIZE) != 0;  
}


/*
 * @implemented
 */
WINBOOL STDCALL
IsWindow(HWND hWnd)
{
  DWORD WndProc = NtUserGetWindowLong(hWnd, GWL_WNDPROC, FALSE);
  return (0 != WndProc || ERROR_INVALID_HANDLE != GetLastError());
}


/*
 * @implemented
 */
WINBOOL STDCALL
IsWindowUnicode(HWND hWnd)
{
	return (WINBOOL)NtUserCallOneParam((DWORD)hWnd,ONEPARAM_ROUTINE_ISWINDOWUNICODE);
}


/*
 * @implemented
 */
WINBOOL STDCALL
IsWindowVisible(HWND hWnd)
{
  while (NtUserGetWindowLong(hWnd, GWL_STYLE, FALSE) & WS_CHILD)
    {
      if (!(NtUserGetWindowLong(hWnd, GWL_STYLE, FALSE) & WS_VISIBLE))
	{
	  return(FALSE);
	}
      hWnd = GetAncestor(hWnd, GA_PARENT);
    }
  return(NtUserGetWindowLong(hWnd, GWL_STYLE, FALSE) & WS_VISIBLE);
}


/*
 * @implemented
 */
WINBOOL
STDCALL
IsWindowEnabled(
  HWND hWnd)
{
    // AG: I don't know if child windows are affected if the parent is
    // disabled. I think they stop processing messages but stay appearing
    // as enabled.
    
    return (! (NtUserGetWindowLong(hWnd, GWL_STYLE, FALSE) & WS_DISABLED));
}


/*
 * @implemented
 */
WINBOOL STDCALL
IsZoomed(HWND hWnd)
{
  return NtUserGetWindowLong(hWnd, GWL_STYLE, FALSE) & WS_MAXIMIZE;
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
LockSetForegroundWindow(UINT uLockCode)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @implemented
 */
WINBOOL STDCALL
MoveWindow(HWND hWnd,
	   int X,
	   int Y,
	   int nWidth,
	   int nHeight,
	   WINBOOL bRepaint)
{
  return NtUserMoveWindow(hWnd, X, Y, nWidth, nHeight, bRepaint);
}


/*
 * @implemented
 */
WINBOOL STDCALL
AnimateWindow(HWND hwnd,
	      DWORD dwTime,
	      DWORD dwFlags)
{
  /* FIXME Add animation code */

  /* If trying to show/hide and it's already   *
   * shown/hidden or invalid window, fail with *
   * invalid parameter                         */
   
  BOOL visible;
  visible = IsWindowVisible(hwnd);
//  if(!IsWindow(hwnd) ||
//    (visible && !(dwFlags & AW_HIDE)) ||
//    (!visible && (dwFlags & AW_HIDE)))
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
  }

//  ShowWindow(hwnd, (dwFlags & AW_HIDE) ? SW_HIDE : ((dwFlags & AW_ACTIVATE) ? SW_SHOW : SW_SHOWNA));

  return TRUE;
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
OpenIcon(HWND hWnd)
{
    if (! NtUserGetWindowLong(hWnd, GWL_STYLE, FALSE) & WS_MINIMIZE)
    {
        // Not minimized - error?
        return FALSE;
    }
    
    if (! SendMessageA(hWnd, WM_QUERYOPEN, 0, 0))
    {
        // Window doesn't want to be opened - error?
        return FALSE;
    }
    
    // Now we need to do the actual opening of the window, which is something
    // I'll leave to someone more capable :)

    UNIMPLEMENTED;
    return FALSE;
}


/*
 * @unimplemented
 */
HWND STDCALL
RealChildWindowFromPoint(HWND hwndParent,
			 POINT ptParentClientCoords)
{
  UNIMPLEMENTED;
  return (HWND)0;
}

/*
 * @unimplemented
 */
WINBOOL STDCALL
SetForegroundWindow(HWND hWnd)
{
   return NtUserCallHwndLock(hWnd, HWNDLOCK_ROUTINE_SETFOREGROUNDWINDOW);
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
SetLayeredWindowAttributes(HWND hwnd,
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
HWND STDCALL
SetParent(HWND hWndChild,
	  HWND hWndNewParent)
{
  return NtUserSetParent(hWndChild, hWndNewParent);
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
SetProcessDefaultLayout(DWORD dwDefaultLayout)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
SetWindowPlacement(HWND hWnd,
		   CONST WINDOWPLACEMENT *lpwndpl)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @implemented
 */
WINBOOL STDCALL
SetWindowPos(HWND hWnd,
	     HWND hWndInsertAfter,
	     int X,
	     int Y,
	     int cx,
	     int cy,
	     UINT uFlags)
{
  return NtUserSetWindowPos(hWnd,hWndInsertAfter, X, Y, cx, cy, uFlags);
}


/*
 * @implemented
 */
WINBOOL STDCALL
SetWindowTextA(HWND hWnd,
	       LPCSTR lpString)
{
  return SendMessageA(hWnd, WM_SETTEXT, 0, (LPARAM)lpString);
}


/*
 * @implemented
 */
WINBOOL STDCALL
SetWindowTextW(HWND hWnd,
	       LPCWSTR lpString)
{
  return SendMessageW(hWnd, WM_SETTEXT, 0, (LPARAM)lpString);
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
ShowOwnedPopups(HWND hWnd,
		WINBOOL fShow)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @implemented
 */
WINBOOL STDCALL
ShowWindow(HWND hWnd,
	   int nCmdShow)
{
  return NtUserShowWindow(hWnd, nCmdShow);
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
ShowWindowAsync(HWND hWnd,
		int nCmdShow)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
/*
WORD STDCALL
TileWindows(HWND hwndParent,
	    UINT wHow,
	    CONST RECT *lpRect,
	    UINT cKids,
	    const HWND *lpKids)
{
  UNIMPLEMENTED;
  return 0;
}
*/


/*
 * @unimplemented
 */
WINBOOL STDCALL
UpdateLayeredWindow(HWND hwnd,
		    HDC hdcDst,
		    POINT *pptDst,
		    SIZE *psize,
		    HDC hdcSrc,
		    POINT *pptSrc,
		    COLORREF crKey,
		    BLENDFUNCTION *pblend,
		    DWORD dwFlags)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @implemented
 */
HWND STDCALL
WindowFromPoint(POINT Point)
{
  //TODO: Determine what the actual parameters to 
  // NtUserWindowFromPoint are.
  return NtUserWindowFromPoint(Point.x, Point.y);
}


/*
 * @implemented
 */
int STDCALL
MapWindowPoints(HWND hWndFrom, HWND hWndTo, LPPOINT lpPoints, UINT cPoints)
{
  POINT FromOffset, ToOffset;
  LONG XMove, YMove;
  ULONG i;

  NtUserGetClientOrigin(hWndFrom, &FromOffset);
  NtUserGetClientOrigin(hWndTo, &ToOffset);
  XMove = FromOffset.x - ToOffset.x;
  YMove = FromOffset.y - ToOffset.y;

  for (i = 0; i < cPoints; i++)
    {
      lpPoints[i].x += XMove;
      lpPoints[i].y += YMove;
    }
  return(MAKELONG(LOWORD(XMove), LOWORD(YMove)));
}


/*
 * @implemented
 */
WINBOOL STDCALL 
ScreenToClient(HWND hWnd, LPPOINT lpPoint)
{
  return(MapWindowPoints(NULL, hWnd, lpPoint, 1));
}


/*
 * @implemented
 */
WINBOOL STDCALL
ClientToScreen(HWND hWnd, LPPOINT lpPoint)
{
    return (MapWindowPoints( hWnd, NULL, lpPoint, 1 ));
}


/*
 * @implemented
 */
WINBOOL
STDCALL
SetWindowContextHelpId(HWND hwnd,
          DWORD dwContextHelpId)
{
  return (WINBOOL)NtUserCallTwoParam((DWORD)hwnd, (DWORD)dwContextHelpId, 
                                     TWOPARAM_ROUTINE_SETWNDCONTEXTHLPID);
}


/*
 * @implemented
 */
DWORD
STDCALL
GetWindowContextHelpId(HWND hwnd)
{
  return NtUserCallOneParam((DWORD)hwnd, ONEPARAM_ROUTINE_GETWNDCONTEXTHLPID);
}

/*
 * @implemented
 */
DWORD
STDCALL
InternalGetWindowText(HWND hWnd, LPWSTR lpString, int nMaxCount)
{
  return NtUserInternalGetWindowText(hWnd, lpString, nMaxCount);
}

/*
 * @unimplemented
 */
WINBOOL
STDCALL
IsHungAppWindow(HWND hwnd)
{
  /* FIXME: ReactOS doesnt identify hung app windows yet */
  return FALSE;
}

/*
 * @implemented
 */
VOID
STDCALL
SetLastErrorEx(DWORD dwErrCode, DWORD dwType)
{
  SetLastError(dwErrCode);
}

/*
 * @implemented
 */
WINBOOL
STDCALL
SetSystemMenu (
  HWND hwnd, 
  HMENU hMenu)
{
  if(!hwnd)
  {
    SetLastError(ERROR_INVALID_WINDOW_HANDLE);
    return FALSE;
  }
  if(!hMenu)
  {
    SetLastError(ERROR_INVALID_MENU_HANDLE);
    return FALSE;
  }
  return NtUserSetSystemMenu(hwnd, hMenu);
}

/*
 * @implemented
 */
HMENU
STDCALL
GetSystemMenu(
  HWND hWnd,
  WINBOOL bRevert)
{
  if(!hWnd)
  {
    SetLastError(ERROR_INVALID_WINDOW_HANDLE);
    return (HMENU)0;
  }
  return NtUserGetSystemMenu(hWnd, bRevert);
}

/*
 * @implemented
 */
HWND
STDCALL
GetFocus(VOID)
{
  return (HWND)NtUserGetThreadState(0);
}

/*
 * @unimplemented
 */
HWND
STDCALL
SetTaskmanWindow(HWND x)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
HWND
STDCALL
SetProgmanWindow(HWND x)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
HWND
STDCALL
GetProgmanWindow(VOID)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
HWND
STDCALL
GetTaskmanWindow(VOID)
{
  UNIMPLEMENTED;
  return FALSE;
}

/* EOF */
