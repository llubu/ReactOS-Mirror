/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
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
/* $Id: hook.c,v 1.11 2003/11/09 13:50:04 navaraf Exp $
 *
 * PROJECT:         ReactOS user32.dll
 * FILE:            lib/user32/windows/input.c
 * PURPOSE:         Input
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      09-05-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#include <windows.h>
#include <user32.h>
#include <debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * @unimplemented
 */
WINBOOL
STDCALL
UnhookWindowsHookEx(
  HHOOK hhk)
{
  UNIMPLEMENTED;
  return FALSE;
}
#if 0
WINBOOL
STDCALL
CallMsgFilter(
  LPMSG lpMsg,
  int nCode)
{
  UNIMPLEMENTED;
  return FALSE;
}
#endif


/*
 * @unimplemented
 */
WINBOOL
STDCALL
CallMsgFilterA(
  LPMSG lpMsg,
  int nCode)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
CallMsgFilterW(
  LPMSG lpMsg,
  int nCode)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
LRESULT
STDCALL
CallNextHookEx(
  HHOOK hhk,
  int nCode,
  WPARAM wParam,
  LPARAM lParam)
{
  UNIMPLEMENTED;
  return (LRESULT)0;
}

/*
 * @unimplemented
 */
HHOOK
STDCALL
SetWindowsHookW ( int idHook, HOOKPROC lpfn )
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
HHOOK
STDCALL
SetWindowsHookA ( int idHook, HOOKPROC lpfn )
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
WINBOOL
STDCALL
DeregisterShellHookWindow(HWND hWnd)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
WINBOOL
STDCALL
RegisterShellHookWindow(HWND hWnd)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
WINBOOL
STDCALL
UnhookWindowsHook ( int nCode, HOOKPROC pfnFilterProc )
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
VOID
STDCALL
NotifyWinEvent(
	       DWORD event,
	       HWND  hwnd,
	       LONG  idObject,
	       LONG  idChild
	       )
{
  UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
HWINEVENTHOOK
STDCALL
SetWinEventHook(
		DWORD        eventMin,
		DWORD        eventMax,
		HMODULE      hmodWinEventProc,
		WINEVENTPROC pfnWinEventProc,
		DWORD        idProcess,
		DWORD        idThread,
		DWORD        dwFlags
		)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
WINBOOL
STDCALL
UnhookWinEvent ( HWINEVENTHOOK hWinEventHook )
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
WINBOOL
STDCALL
IsWinEventHookInstalled(
    DWORD event)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
HHOOK
STDCALL
SetWindowsHookExA(
    int idHook,
    HOOKPROC lpfn,
    HINSTANCE hMod,
    DWORD dwThreadId)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
HHOOK
STDCALL
SetWindowsHookExW(
    int idHook,
    HOOKPROC lpfn,
    HINSTANCE hMod,
    DWORD dwThreadId)
{
  UNIMPLEMENTED;
  return 0;
}

