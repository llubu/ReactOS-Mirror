/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 2003 ReactOS Team
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
 * PURPOSE:          HotKey support
 * FILE:             subsys/win32k/ntuser/hotkey.c
 * PROGRAMER:        Eric Kohl
 * REVISION HISTORY:
 *       02-11-2003  EK  Created
 */

/* INCLUDES ******************************************************************/

#include <w32k.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

/* FUNCTIONS *****************************************************************/

NTSTATUS FASTCALL
InitHotKeys(PWINSTATION_OBJECT WinStaObject)
{
   InitializeListHead(&WinStaObject->HotKeyListHead);
   ExInitializeFastMutex(&WinStaObject->HotKeyListLock);

   return STATUS_SUCCESS;
}


NTSTATUS FASTCALL
CleanupHotKeys(PWINSTATION_OBJECT WinStaObject)
{

   return STATUS_SUCCESS;
}


BOOL
GetHotKey (PWINSTATION_OBJECT WinStaObject,
           UINT fsModifiers,
           UINT vk,
           struct _ETHREAD **Thread,
           HWND *hWnd,
           int *id)
{
   PLIST_ENTRY Entry;
   PHOT_KEY_ITEM HotKeyItem;

   if(!WinStaObject)
   {
      return FALSE;
   }

   IntLockHotKeys(WinStaObject);

   Entry = WinStaObject->HotKeyListHead.Flink;
   while (Entry != &WinStaObject->HotKeyListHead)
   {
      HotKeyItem = (PHOT_KEY_ITEM) CONTAINING_RECORD(Entry,
                   HOT_KEY_ITEM,
                   ListEntry);
      if (HotKeyItem->fsModifiers == fsModifiers &&
            HotKeyItem->vk == vk)
      {
         if (Thread != NULL)
            *Thread = HotKeyItem->Thread;

         if (hWnd != NULL)
            *hWnd = HotKeyItem->hWnd;

         if (id != NULL)
            *id = HotKeyItem->id;

         IntUnLockHotKeys(WinStaObject);

         return TRUE;
      }

      Entry = Entry->Flink;
   }

   IntUnLockHotKeys(WinStaObject);

   return FALSE;
}


VOID
UnregisterWindowHotKeys(PWINDOW_OBJECT Window)
{
   PLIST_ENTRY Entry;
   PHOT_KEY_ITEM HotKeyItem;
   PWINSTATION_OBJECT WinStaObject = NULL;

   if(Window->OwnerThread && Window->OwnerThread->ThreadsProcess)
      WinStaObject = Window->OwnerThread->Tcb.Win32Thread->Desktop->WindowStation;

   if(!WinStaObject)
      return;

   IntLockHotKeys(WinStaObject);

   Entry = WinStaObject->HotKeyListHead.Flink;
   while (Entry != &WinStaObject->HotKeyListHead)
   {
      HotKeyItem = (PHOT_KEY_ITEM) CONTAINING_RECORD (Entry,
                   HOT_KEY_ITEM,
                   ListEntry);
      Entry = Entry->Flink;
      if (HotKeyItem->hWnd == Window->hSelf)
      {
         RemoveEntryList (&HotKeyItem->ListEntry);
         ExFreePool (HotKeyItem);
      }
   }

   IntUnLockHotKeys(WinStaObject);
}


VOID
UnregisterThreadHotKeys(struct _ETHREAD *Thread)
{
   PLIST_ENTRY Entry;
   PHOT_KEY_ITEM HotKeyItem;
   PWINSTATION_OBJECT WinStaObject = NULL;

   if(Thread->Tcb.Win32Thread && Thread->Tcb.Win32Thread->Desktop)
      WinStaObject = Thread->Tcb.Win32Thread->Desktop->WindowStation;

   if(!WinStaObject)
      return;

   IntLockHotKeys(WinStaObject);

   Entry = WinStaObject->HotKeyListHead.Flink;
   while (Entry != &WinStaObject->HotKeyListHead)
   {
      HotKeyItem = (PHOT_KEY_ITEM) CONTAINING_RECORD (Entry,
                   HOT_KEY_ITEM,
                   ListEntry);
      Entry = Entry->Flink;
      if (HotKeyItem->Thread == Thread)
      {
         RemoveEntryList (&HotKeyItem->ListEntry);
         ExFreePool (HotKeyItem);
      }
   }

   IntUnLockHotKeys(WinStaObject);
}


static BOOL
IsHotKey (PWINSTATION_OBJECT WinStaObject,
          UINT fsModifiers,
          UINT vk)
{
   PLIST_ENTRY Entry;
   PHOT_KEY_ITEM HotKeyItem;

   Entry = WinStaObject->HotKeyListHead.Flink;
   while (Entry != &WinStaObject->HotKeyListHead)
   {
      HotKeyItem = (PHOT_KEY_ITEM) CONTAINING_RECORD (Entry,
                   HOT_KEY_ITEM,
                   ListEntry);
      if (HotKeyItem->fsModifiers == fsModifiers &&
            HotKeyItem->vk == vk)
      {
         return TRUE;
      }

      Entry = Entry->Flink;
   }

   return FALSE;
}


BOOL STDCALL
NtUserRegisterHotKey(HWND hWnd,
                     int id,
                     UINT fsModifiers,
                     UINT vk)
{
   PHOT_KEY_ITEM HotKeyItem;
   PWINDOW_OBJECT Window;
   PWINSTATION_OBJECT WinStaObject = NULL;
   PETHREAD HotKeyThread;
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserRegisterHotKey\n");
   UserEnterExclusive();

   if (hWnd == NULL)
   {
      HotKeyThread = PsGetCurrentThread();
   }
   else
   {
      Window = IntGetWindowObject(hWnd);
      if(!Window)
      {
         SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
         RETURN( FALSE);
      }
      HotKeyThread = Window->OwnerThread;
      IntReleaseWindowObject(Window);
   }


   if(HotKeyThread->ThreadsProcess && HotKeyThread->ThreadsProcess->Win32Process)
      WinStaObject = HotKeyThread->Tcb.Win32Thread->Desktop->WindowStation;

   if(!WinStaObject)
   {
      RETURN( FALSE);
   }

   IntLockHotKeys(WinStaObject);

   /* Check for existing hotkey */
   if (IsHotKey (WinStaObject, fsModifiers, vk))
   {
      IntUnLockHotKeys(WinStaObject);
      RETURN( FALSE);
   }

   HotKeyItem = ExAllocatePoolWithTag (PagedPool, sizeof(HOT_KEY_ITEM), TAG_HOTKEY);
   if (HotKeyItem == NULL)
   {
      IntUnLockHotKeys(WinStaObject);
      RETURN( FALSE);
   }

   HotKeyItem->Thread = HotKeyThread;
   HotKeyItem->hWnd = hWnd;
   HotKeyItem->id = id;
   HotKeyItem->fsModifiers = fsModifiers;
   HotKeyItem->vk = vk;

   InsertHeadList (&WinStaObject->HotKeyListHead,
                   &HotKeyItem->ListEntry);

   IntUnLockHotKeys(WinStaObject);

   RETURN( TRUE);

CLEANUP:
   DPRINT("Leave NtUserRegisterHotKey, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


BOOL STDCALL
NtUserUnregisterHotKey(HWND hWnd,
                       int id)
{
   PLIST_ENTRY Entry;
   PHOT_KEY_ITEM HotKeyItem;
   PWINDOW_OBJECT Window;
   PWINSTATION_OBJECT WinStaObject = NULL;
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserUnregisterHotKey\n");
   UserEnterExclusive();

   Window = IntGetWindowObject(hWnd);
   if(!Window)
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      RETURN( FALSE);
   }

   if(Window->OwnerThread->ThreadsProcess && Window->OwnerThread->ThreadsProcess->Win32Process)
      WinStaObject = Window->OwnerThread->Tcb.Win32Thread->Desktop->WindowStation;

   if(!WinStaObject)
   {
      IntReleaseWindowObject(Window);
      RETURN( FALSE);
   }

   IntLockHotKeys(WinStaObject);

   Entry = WinStaObject->HotKeyListHead.Flink;
   while (Entry != &WinStaObject->HotKeyListHead)
   {
      HotKeyItem = (PHOT_KEY_ITEM) CONTAINING_RECORD (Entry,
                   HOT_KEY_ITEM,
                   ListEntry);
      if (HotKeyItem->hWnd == hWnd &&
            HotKeyItem->id == id)
      {
         RemoveEntryList (&HotKeyItem->ListEntry);
         ExFreePool (HotKeyItem);
         IntUnLockHotKeys(WinStaObject);

         IntReleaseWindowObject(Window);
         RETURN( TRUE);
      }

      Entry = Entry->Flink;
   }

   IntUnLockHotKeys(WinStaObject);

   IntReleaseWindowObject(Window);
   RETURN( FALSE);

CLEANUP:
   DPRINT("Leave NtUserUnregisterHotKey, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/* EOF */
