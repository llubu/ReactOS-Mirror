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
/* $Id: display.c,v 1.5 2003/07/10 21:04:31 chorns Exp $
 *
 * PROJECT:         ReactOS user32.dll
 * FILE:            lib/user32/misc/dde.c
 * PURPOSE:         DDE
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
WINBOOL STDCALL
EnumDisplayDevicesA(
  LPCSTR lpDevice,
  DWORD iDevNum,
  PDISPLAY_DEVICE lpDisplayDevice,
  DWORD dwFlags)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
EnumDisplayDevicesW(
  LPCWSTR lpDevice,
  DWORD iDevNum,
  PDISPLAY_DEVICE lpDisplayDevice,
  DWORD dwFlags)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
EnumDisplayMonitors(
  HDC hdc,
  LPRECT lprcClip,
  MONITORENUMPROC lpfnEnum,
  LPARAM dwData)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
EnumDisplaySettingsA(
  LPCSTR lpszDeviceName,
  DWORD iModeNum,
  LPDEVMODEA lpDevMode)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
EnumDisplaySettingsExA(
  LPCSTR lpszDeviceName,
  DWORD iModeNum,
  LPDEVMODEW lpDevMode,
  DWORD dwFlags)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
EnumDisplaySettingsExW(
  LPCWSTR lpszDeviceName,
  DWORD iModeNum,
  LPDEVMODEA lpDevMode,
  DWORD dwFlags)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
EnumDisplaySettingsW(
  LPCWSTR lpszDeviceName,
  DWORD iModeNum,
  LPDEVMODEW lpDevMode)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
GetMonitorInfoA(
  HMONITOR hMonitor,
  LPMONITORINFO lpmi)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
GetMonitorInfoW(
  HMONITOR hMonitor,
  LPMONITORINFO lpmi)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
LONG
STDCALL
ChangeDisplaySettingsA(
  LPDEVMODEA lpDevMode,
  DWORD dwflags)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
LONG
STDCALL
ChangeDisplaySettingsExA(
  LPCSTR lpszDeviceName,
  LPDEVMODEA lpDevMode,
  HWND hwnd,
  DWORD dwflags,
  LPVOID lParam)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
LONG
STDCALL
ChangeDisplaySettingsExW(
  LPCWSTR lpszDeviceName,
  LPDEVMODEW lpDevMode,
  HWND hwnd,
  DWORD dwflags,
  LPVOID lParam)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
LONG
STDCALL
ChangeDisplaySettingsW(
  LPDEVMODEW lpDevMode,
  DWORD dwflags)
{
  UNIMPLEMENTED;
  return 0;
}
