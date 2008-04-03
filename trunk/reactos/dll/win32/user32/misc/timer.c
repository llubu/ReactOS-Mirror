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
/* $Id$
 *
 * PROJECT:         ReactOS user32.dll
 * FILE:            lib/user32/misc/dde.c
 * PURPOSE:         DDE
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      09-05-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#include <user32.h>

#include <wine/debug.h>
/* FUNCTIONS *****************************************************************/


/*
 * @implemented
 */
BOOL
STDCALL
KillSystemTimer(
  HWND hWnd,
  UINT_PTR IDEvent)
{
  return NtUserCallHwndParam(hWnd, IDEvent, HWNDPARAM_ROUTINE_KILLSYSTEMTIMER);
}


/*
 * @implemented
 */
BOOL
STDCALL
KillTimer(
  HWND hWnd,
  UINT_PTR IDEvent)
{
  return NtUserKillTimer(hWnd, IDEvent);
}


/*
 * @implemented
 */
UINT_PTR
STDCALL
SetSystemTimer(
  HWND hWnd,
  UINT_PTR IDEvent,
  UINT Period,
  TIMERPROC TimerFunc)
{
  return NtUserSetSystemTimer(hWnd, IDEvent, Period, TimerFunc);
}


/*
 * @implemented
 */
UINT_PTR
STDCALL
SetTimer(
  HWND hWnd,
  UINT_PTR IDEvent,
  UINT Period,
  TIMERPROC TimerFunc)
{
  return NtUserSetTimer(hWnd, IDEvent, Period, TimerFunc);
}




