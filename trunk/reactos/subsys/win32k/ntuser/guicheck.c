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
/* $Id: guicheck.c,v 1.22 2004/12/24 17:45:58 weiden Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          GUI state check
 * FILE:             subsys/win32k/ntuser/guicheck.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 * NOTES:            The GuiCheck() function performs a few delayed operations:
 *                   1) A GUI process is assigned a window station
 *                   2) A message queue is created for a GUI thread before use
 *                   3) The system window classes are registered for a process
 * REVISION HISTORY:
 *       06-06-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#include <w32k.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

static LONG NrGuiAppsRunning = 0;
static FAST_MUTEX GuiSwitchLock;

/* FUNCTIONS *****************************************************************/

static BOOL FASTCALL
AddGuiApp(PW32PROCESS W32Data)
{
  W32Data->Flags |= W32PF_CREATEDWINORDC;
  if (InterlockedIncrement(&NrGuiAppsRunning) == 1)
    {
      BOOL Initialized;
      
      ExAcquireFastMutex(&GuiSwitchLock);
      Initialized = IntInitializeDesktopGraphics();
      ExReleaseFastMutex(&GuiSwitchLock);
      
      if (!Initialized)
        {
          W32Data->Flags &= ~W32PF_CREATEDWINORDC;
          InterlockedDecrement(&NrGuiAppsRunning);
          return FALSE;
        }
    }
  return TRUE;
}

static void FASTCALL
RemoveGuiApp(PW32PROCESS W32Data)
{
  W32Data->Flags &= ~W32PF_CREATEDWINORDC;
  if (InterlockedDecrement(&NrGuiAppsRunning) == 0)
    {
      ExAcquireFastMutex(&GuiSwitchLock);
      IntEndDesktopGraphics();
      ExReleaseFastMutex(&GuiSwitchLock);
    }
}

BOOL FASTCALL
IntGraphicsCheck(BOOL Create)
{
  PW32PROCESS W32Data;

  W32Data = PsGetWin32Process();
  if (Create)
    {
      if (! (W32Data->Flags & W32PF_CREATEDWINORDC) && ! (W32Data->Flags & W32PF_MANUALGUICHECK))
        {
          return AddGuiApp(W32Data);
        }
    }
  else
    {
      if ((W32Data->Flags & W32PF_CREATEDWINORDC) && ! (W32Data->Flags & W32PF_MANUALGUICHECK))
        {
          RemoveGuiApp(W32Data);
        }
    }

  return TRUE;
}

VOID STDCALL
NtUserManualGuiCheck(LONG Check)
{
  PW32PROCESS W32Data;

  W32Data = PsGetWin32Process();
  if (0 == Check)
    {
      W32Data->Flags |= W32PF_MANUALGUICHECK;
    }
  else if (0 < Check)
    {
      if (! (W32Data->Flags & W32PF_CREATEDWINORDC))
        {
          AddGuiApp(W32Data);
        }
    }
  else
    {
      if (W32Data->Flags & W32PF_CREATEDWINORDC)
        {
          RemoveGuiApp(W32Data);
        }
    }
}

NTSTATUS FASTCALL
InitGuiCheckImpl (VOID)
{
  ExInitializeFastMutex(&GuiSwitchLock);
  return STATUS_SUCCESS;
}

/* EOF */
