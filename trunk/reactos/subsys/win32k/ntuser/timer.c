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
/* $Id: timer.c,v 1.5 2003/07/06 23:04:19 hyperion Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Window timers messages
 * FILE:             subsys/win32k/ntuser/timer.c
 * PROGRAMER:        Gunnar
 * REVISION HISTORY:
 *
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <win32k/win32k.h>
#include <win32k/ntuser.h>
#include <internal/ntoskrnl.h>
#include <internal/ps.h>
#include <include/msgqueue.h>
#include <messages.h>
#include <napi/win32.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

//windows 2000 has room for 32768 handle-less timers
#define NUM_HANDLE_LESS_TIMERS   1024

static FAST_MUTEX     Mutex;
static LIST_ENTRY     TimerListHead;
static KTIMER         Timer;
static RTL_BITMAP     HandleLessTimersBitMap;
static PVOID          HandleLessTimersBitMapBuffer;
static ULONG          HintIndex = 0;
static HANDLE        MsgTimerThreadHandle;
static CLIENT_ID     MsgTimerThreadId;


typedef struct _MSG_TIMER_ENTRY{
   LIST_ENTRY     ListEntry;
   LARGE_INTEGER  Timeout;
   HANDLE          ThreadID;
   UINT           Period;
   MSG            Msg;
} MSG_TIMER_ENTRY, *PMSG_TIMER_ENTRY;


/* FUNCTIONS *****************************************************************/


//return true if the new timer became the first entry
//must hold mutex while calling this
BOOL
FASTCALL
InsertTimerAscendingOrder(PMSG_TIMER_ENTRY NewTimer)
{
   PLIST_ENTRY EnumEntry, InsertAfter;
   PMSG_TIMER_ENTRY MsgTimer;

   InsertAfter = NULL;

   EnumEntry = TimerListHead.Flink;
   while (EnumEntry != &TimerListHead)
   {
      MsgTimer = CONTAINING_RECORD(EnumEntry, MSG_TIMER_ENTRY, ListEntry);
      if (NewTimer->Timeout.QuadPart > MsgTimer->Timeout.QuadPart)
      {
         InsertAfter = EnumEntry;
      }
      EnumEntry = EnumEntry->Flink;
   }

   if (InsertAfter)
   {
      InsertTailList(InsertAfter, &NewTimer->ListEntry);
      return FALSE;
   }

   //insert as first entry
   InsertHeadList(&TimerListHead, &NewTimer->ListEntry);
   return TRUE;

}

//must hold mutex while calling this
PMSG_TIMER_ENTRY
FASTCALL
RemoveTimer(HWND hWnd, UINT_PTR IDEvent, HANDLE ThreadID)
{
   PMSG_TIMER_ENTRY MsgTimer;
   PLIST_ENTRY EnumEntry;

   //remove timer if allready in the queue
   EnumEntry = TimerListHead.Flink;
   while (EnumEntry != &TimerListHead)
   {
      MsgTimer = CONTAINING_RECORD(EnumEntry, MSG_TIMER_ENTRY, ListEntry);
      EnumEntry = EnumEntry->Flink;

      if (MsgTimer->Msg.hwnd == hWnd && 
          MsgTimer->Msg.wParam == (WPARAM)IDEvent &&
          MsgTimer->ThreadID == ThreadID)
      {
         RemoveEntryList(&MsgTimer->ListEntry);
         return MsgTimer;
      }
   }

   return NULL;
}


/* 
 * NOTE: It doesn't kill timers. It just removes them from the list.
 */
VOID
FASTCALL
RemoveTimersThread(HANDLE ThreadID)
{
   PMSG_TIMER_ENTRY MsgTimer;
   PLIST_ENTRY EnumEntry;

   ExAcquireFastMutex(&Mutex);

   EnumEntry = TimerListHead.Flink;
   while (EnumEntry != &TimerListHead)
   {
      MsgTimer = CONTAINING_RECORD(EnumEntry, MSG_TIMER_ENTRY, ListEntry);
      EnumEntry = EnumEntry->Flink;

      if (MsgTimer->ThreadID == ThreadID)
      {
         if (MsgTimer->Msg.hwnd == NULL)
         {
            RtlClearBits(&HandleLessTimersBitMap, ((UINT_PTR)MsgTimer->Msg.wParam) - 1, 1);   
         }

         RemoveEntryList(&MsgTimer->ListEntry);
         ExFreePool(MsgTimer);
      }
   }

   ExReleaseFastMutex(&Mutex);

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
 ULONG Index;
 PMSG_TIMER_ENTRY MsgTimer = NULL;
 PMSG_TIMER_ENTRY NewTimer;
 LARGE_INTEGER CurrentTime;
 HANDLE ThreadID;

 //FIXME: WINE: window must be owned by the calling thread
#if 0
 if(hWnd && !(hWnd = WIN_IsCurrentThread(hWnd))
 {
  return STATUS_UNSUCCESSFUL;
 }
#endif

 ThreadID = PsGetCurrentThreadId();
 KeQuerySystemTime(&CurrentTime);
 ExAcquireFastMutex(&Mutex);

 if(hWnd == NULL)
 {
  /* find a free, handle-less timer id */
  Index = RtlFindClearBitsAndSet(&HandleLessTimersBitMap, 1, HintIndex);

  if(Index == -1)
  {
   /* FIXME: set the last error */
   ExReleaseFastMutex(&Mutex);
   return 0;
  }

  ++ Index;
  HintIndex = Index;
  return Index;
 }
 else
 {
  /* remove timer if already in the queue */
  MsgTimer = RemoveTimer(hWnd, nIDEvent, ThreadID); 
 }

 if(MsgTimer)
 {
  /* modify existing (removed) timer */
  NewTimer = MsgTimer;

  NewTimer->Period = uElapse;
  NewTimer->Timeout.QuadPart = CurrentTime.QuadPart + (uElapse * 10000);
  NewTimer->Msg.lParam = (LPARAM)lpTimerFunc;
 }
 else
 {
  /* FIXME: use lookaside? */
  NewTimer = ExAllocatePool(PagedPool, sizeof(MSG_TIMER_ENTRY));

  NewTimer->Msg.hwnd = hWnd;
  NewTimer->Msg.message = WM_TIMER;
  NewTimer->Msg.wParam = (WPARAM)nIDEvent;
  NewTimer->Msg.lParam = (LPARAM)lpTimerFunc;
  NewTimer->Period = uElapse;
  NewTimer->Timeout.QuadPart = CurrentTime.QuadPart + (uElapse * 10000);
  NewTimer->ThreadID = ThreadID;
 }

 if(InsertTimerAscendingOrder(NewTimer))
 {
  /* new timer is first in queue and expires first */
  KeSetTimer(&Timer, NewTimer->Timeout, NULL);
 }

 ExReleaseFastMutex(&Mutex);

 return 1;
}


BOOL
STDCALL
NtUserKillTimer
(
 HWND hWnd,
 UINT_PTR uIDEvent
)
{
 PMSG_TIMER_ENTRY MsgTimer;
 
 ExAcquireFastMutex(&Mutex);

 /* handle-less timer? */
 if(hWnd == NULL)
 {
  if(!RtlAreBitsSet(&HandleLessTimersBitMap, uIDEvent - 1, 1))
  {
   /* bit was not set */
   /* FIXME: set the last error */
   ExReleaseFastMutex(&Mutex); 
   return FALSE;
  }

  RtlClearBits(&HandleLessTimersBitMap, uIDEvent - 1, 1);
 }

 MsgTimer = RemoveTimer(hWnd, uIDEvent, PsGetCurrentThreadId());

 ExReleaseFastMutex(&Mutex);

 if(MsgTimer == NULL)
 {
  /* didn't find timer */
  /* FIXME: set the last error */
  return FALSE;
 }

 /* FIXME: use lookaside? */
 ExFreePool(MsgTimer);

 return TRUE;
}

DWORD
STDCALL
NtUserSetSystemTimer(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3)
{
  UNIMPLEMENTED

  return 0;
}


static NTSTATUS STDCALL
TimerThreadMain(
   PVOID StartContext
   )
{
   NTSTATUS Status;
   LARGE_INTEGER CurrentTime;
   PLIST_ENTRY EnumEntry;
   PMSG_TIMER_ENTRY MsgTimer;
   PETHREAD Thread;

   for (;;)
   {

      Status = KeWaitForSingleObject(   &Timer,
                                        Executive,
                                        KernelMode,
                                        FALSE,
                                        NULL
                                        );
      if (!NT_SUCCESS(Status))
      {
         DPRINT1("Error waiting in TimerThreadMain\n");
         KeBugCheck(0);
      }

      ExAcquireFastMutex(&Mutex);

      KeQuerySystemTime(&CurrentTime);

      EnumEntry = TimerListHead.Flink;
      while (EnumEntry != &TimerListHead)
      {
         MsgTimer = CONTAINING_RECORD(EnumEntry, MSG_TIMER_ENTRY, ListEntry);
         EnumEntry = EnumEntry->Flink;

         if (CurrentTime.QuadPart >= MsgTimer->Timeout.QuadPart)
         {
            RemoveEntryList(&MsgTimer->ListEntry);

            /* 
             * FIXME: 1) Find a faster way of getting the thread message queue? (lookup by id is slow)
             */

            if (!NT_SUCCESS(PsLookupThreadByThreadId(MsgTimer->ThreadID, &Thread)))
            {
               ExFreePool(MsgTimer);
               continue;
            }
            
            MsqPostMessage(((PW32THREAD)Thread->Win32Thread)->MessageQueue, MsqCreateMessage(&MsgTimer->Msg));

            ObDereferenceObject(Thread);

            //set up next periodic timeout
            MsgTimer->Timeout.QuadPart += (MsgTimer->Period * 10000); 
            InsertTimerAscendingOrder(MsgTimer);

         }                          
         else                       
         {
            break;
         }
      }
   
      //set up next timeout from first entry (if any)
      if (!IsListEmpty(&TimerListHead))
      {
         MsgTimer = CONTAINING_RECORD( TimerListHead.Flink, MSG_TIMER_ENTRY, ListEntry);
         KeSetTimer(&Timer, MsgTimer->Timeout, NULL);
      }

      ExReleaseFastMutex(&Mutex);

   }

}



NTSTATUS FASTCALL
InitTimerImpl(VOID)
{
   NTSTATUS Status;
   ULONG BitmapBytes;

   BitmapBytes = ROUND_UP(NUM_HANDLE_LESS_TIMERS, sizeof(ULONG) * 8) / 8;

   InitializeListHead(&TimerListHead);
   KeInitializeTimer(&Timer);
   ExInitializeFastMutex(&Mutex);

   HandleLessTimersBitMapBuffer = ExAllocatePool(PagedPool, BitmapBytes);
   RtlInitializeBitMap(&HandleLessTimersBitMap,
                       HandleLessTimersBitMapBuffer,
                       BitmapBytes * 8);

   //yes we need this, since ExAllocatePool isn't supposed to zero out allocated memory
   RtlClearAllBits(&HandleLessTimersBitMap); 

   Status = PsCreateSystemThread(&MsgTimerThreadHandle,
                                 THREAD_ALL_ACCESS,
                                 NULL,
                                 NULL,
                                 &MsgTimerThreadId,
                                 TimerThreadMain,
                                 NULL);
   return Status;
}

/* EOF */
