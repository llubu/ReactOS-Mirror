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
/* $Id: timer.c,v 1.5 2003/04/02 23:29:04 gdalsnes Exp $
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
#include <kernel32/error.h>
#include <user32.h>
#include <debug.h>

/* FUNCTIONS *****************************************************************/

WINBOOL
STDCALL
KillTimer(
  HWND hWnd,
  UINT_PTR IDEvent)
{

   NTSTATUS Status;
   
   Status = NtUserKillTimer(hWnd, IDEvent); 
   
   if (!NT_SUCCESS(Status))
   {
      SetLastErrorByStatus(Status);
      return FALSE;
   }

   return TRUE;

}

UINT_PTR
STDCALL
SetTimer(
  HWND hWnd,
  UINT_PTR IDEvent,
  UINT Period,
  TIMERPROC TimerFunc)
{
   NTSTATUS Status;

   Status = NtUserSetTimer(hWnd, &IDEvent, Period, TimerFunc); 
   
   if (!NT_SUCCESS(Status))
   {
      SetLastErrorByStatus(Status);
      return FALSE;
   }

   if (hWnd == NULL) 
      return IDEvent;

   return TRUE;
}

