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
/* $Id$
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Window timers messages
 * FILE:             subsys/win32k/ntuser/timer.c
 * PROGRAMER:        Gunnar
 *                   Thomas Weidenmueller (w3seek@users.sourceforge.net)
 * REVISION HISTORY: 10/04/2003 Implemented System Timers
 *
 */

/* INCLUDES ******************************************************************/

#include <w32k.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

/* Windows 2000 has room for 32768 window-less timers */
#define NUM_WINDOW_LESS_TIMERS   1024

static FAST_MUTEX     Mutex;
static RTL_BITMAP     WindowLessTimersBitMap;
static PVOID          WindowLessTimersBitMapBuffer;
static ULONG          HintIndex = 0;


#define IntLockWindowlessTimerBitmap() \
  ExAcquireFastMutex(&Mutex)

#define IntUnlockWindowlessTimerBitmap() \
  ExReleaseFastMutex(&Mutex)

/* FUNCTIONS *****************************************************************/


UINT_PTR FASTCALL
IntSetTimer(HWND Wnd, UINT_PTR IDEvent, UINT Elapse, TIMERPROC TimerFunc, BOOL SystemTimer)
{
  PWINDOW_OBJECT WindowObject;
  UINT_PTR Ret = 0;

  DPRINT("IntSetTimer wnd %x id %p elapse %u timerproc %p systemtimer %s\n",
         Wnd, IDEvent, Elapse, TimerFunc, SystemTimer ? "TRUE" : "FALSE");

  if ((Wnd == NULL) && ! SystemTimer)
    {
      DPRINT("Window-less timer\n");
      /* find a free, window-less timer id */
      IntLockWindowlessTimerBitmap();
      IDEvent = RtlFindClearBitsAndSet(&WindowLessTimersBitMap, 1, HintIndex);

      if (IDEvent == (UINT_PTR) -1)
        {
          IntUnlockWindowlessTimerBitmap();
          DPRINT1("Unable to find a free window-less timer id\n");
          SetLastWin32Error(ERROR_NO_SYSTEM_RESOURCES);
          return 0;
        }

      HintIndex = ++IDEvent;
      IntUnlockWindowlessTimerBitmap();
      Ret = IDEvent;
    }
  else
    {
      WindowObject = IntGetWindowObject(Wnd);
      if (! WindowObject)
        {
          DPRINT1("Invalid window handle\n");
          SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
          return 0;
        }

      if (WindowObject->OwnerThread->ThreadsProcess != PsGetCurrentProcess())
        {
          IntReleaseWindowObject(WindowObject);
          DPRINT1("Trying to set timer for window in another process (shatter attack?)\n");
          SetLastWin32Error(ERROR_ACCESS_DENIED);
          return 0;
        }
      IntReleaseWindowObject(WindowObject);
      Ret = 1;
    }

  #if 1

  /* Win NT/2k/XP */
  if (Elapse > 0x7fffffff)
    {
      DPRINT("Adjusting uElapse\n");
      Elapse = 1;
    }

  #else

  /* Win Server 2003 */
  if (Elapse > 0x7fffffff)
    {
      DPRINT("Adjusting uElapse\n");
      Elapse = 0x7fffffff;
    }

  #endif

  /* Win 2k/XP */
  if (Elapse < 10)
    {
      DPRINT("Adjusting uElapse\n");
      Elapse = 10;
    }

  if (! MsqSetTimer(PsGetWin32Thread()->MessageQueue, Wnd,
                    IDEvent, Elapse, TimerFunc,
                    SystemTimer ? WM_SYSTIMER : WM_TIMER))
    {
      DPRINT1("Failed to set timer in message queue\n");
      SetLastWin32Error(ERROR_NO_SYSTEM_RESOURCES);
      return 0;
    }


  return Ret;
}


BOOL FASTCALL
IntKillTimer(HWND Wnd, UINT_PTR IDEvent, BOOL SystemTimer)
{
  DPRINT("IntKillTimer wnd %x id %p systemtimer %s\n",
         Wnd, IDEvent, SystemTimer ? "TRUE" : "FALSE");

  if (! MsqKillTimer(PsGetWin32Thread()->MessageQueue, Wnd,
                     IDEvent, SystemTimer ? WM_SYSTIMER : WM_TIMER))
    {
      DPRINT1("Unable to locate timer in message queue\n");
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return FALSE;
    }

  /* window-less timer? */
  if ((Wnd == NULL) && ! SystemTimer)
    {
      /* Release the id */
      IntLockWindowlessTimerBitmap();

      ASSERT(RtlAreBitsSet(&WindowLessTimersBitMap, IDEvent - 1, 1));
      RtlClearBits(&WindowLessTimersBitMap, IDEvent - 1, 1);

      IntUnlockWindowlessTimerBitmap();
    }

  return TRUE;
}

NTSTATUS FASTCALL
InitTimerImpl(VOID)
{
  ULONG BitmapBytes;

  ExInitializeFastMutex(&Mutex);

  BitmapBytes = ROUND_UP(NUM_WINDOW_LESS_TIMERS, sizeof(ULONG) * 8) / 8;
  WindowLessTimersBitMapBuffer = ExAllocatePoolWithTag(PagedPool, BitmapBytes, TAG_TIMERBMP);
  RtlInitializeBitMap(&WindowLessTimersBitMap,
                      WindowLessTimersBitMapBuffer,
                      BitmapBytes * 8);

  /* yes we need this, since ExAllocatePool isn't supposed to zero out allocated memory */
  RtlClearAllBits(&WindowLessTimersBitMap);

  return STATUS_SUCCESS;
}


UINT_PTR
STDCALL
NtUserSetTimer
(
 HWND hWnd,
 UINT_PTR nIDEvent,
 UINT uElapse,
 TIMERPROC lpTimerFunc
)
{
  return IntSetTimer(hWnd, nIDEvent, uElapse, lpTimerFunc, FALSE);
}


BOOL
STDCALL
NtUserKillTimer
(
 HWND hWnd,
 UINT_PTR uIDEvent
)
{
  return IntKillTimer(hWnd, uIDEvent, FALSE);
}


UINT_PTR
STDCALL
NtUserSetSystemTimer(
 HWND hWnd,
 UINT_PTR nIDEvent,
 UINT uElapse,
 TIMERPROC lpTimerFunc
)
{
  return IntSetTimer(hWnd, nIDEvent, uElapse, lpTimerFunc, TRUE);
}


BOOL
STDCALL
NtUserKillSystemTimer(
 HWND hWnd,
 UINT_PTR uIDEvent
)
{
  return IntKillTimer(hWnd, uIDEvent, TRUE);
}

/* EOF */
