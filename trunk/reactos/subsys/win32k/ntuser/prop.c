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
/* $Id: prop.c,v 1.6 2003/11/20 15:35:33 weiden Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Window properties
 * FILE:             subsys/win32k/ntuser/prop.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISION HISTORY:
 *       06-06-2001  CSH  Created
 */
/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <win32k/win32k.h>
#include <internal/safe.h>
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

//#define NDEBUG
#include <debug.h>

typedef struct _PROPLISTITEM
{
  ATOM Atom;
  HANDLE Data;
} PROPLISTITEM, *PPROPLISTITEM;

/* FUNCTIONS *****************************************************************/

PPROPERTY FASTCALL
IntGetProp(PWINDOW_OBJECT WindowObject, ATOM Atom)
{
  PLIST_ENTRY ListEntry;
  PPROPERTY Property;
  
  ListEntry = WindowObject->PropListHead.Flink;
  while (ListEntry != &WindowObject->PropListHead)
    {
      Property = CONTAINING_RECORD(ListEntry, PROPERTY, PropListEntry);
      if (Property->Atom == Atom)
	{
	  return(Property);
	}
      ListEntry = ListEntry->Flink;
    }
  return(NULL);
}

NTSTATUS STDCALL
NtUserBuildPropList(HWND hWnd,
		    LPVOID Buffer,
		    DWORD BufferSize,
		    DWORD *Count)
{
  PWINDOW_OBJECT WindowObject;
  PPROPERTY Property;
  PLIST_ENTRY ListEntry;
  PROPLISTITEM listitem, *li;
  NTSTATUS Status;
  DWORD Cnt = 0;
  
  if (!(WindowObject = IntGetWindowObject(hWnd)))
  {
    return STATUS_INVALID_HANDLE;
  }
  
  if(Buffer)
  {
    if(!BufferSize || (BufferSize % sizeof(PROPLISTITEM) != 0))
    {
      IntReleaseWindowObject(WindowObject);
      return STATUS_INVALID_PARAMETER;
    }
    
    /* copy list */
    ExAcquireFastMutexUnsafe(&WindowObject->PropListLock);
    
    li = (PROPLISTITEM *)Buffer;
    ListEntry = WindowObject->PropListHead.Flink;
    while((BufferSize >= sizeof(PROPLISTITEM)) && (ListEntry != &WindowObject->PropListHead))
    {
      Property = CONTAINING_RECORD(ListEntry, PROPERTY, PropListEntry);
      listitem.Atom = Property->Atom;
      listitem.Data = Property->Data;
      
      Status = MmCopyToCaller(li, &listitem, sizeof(PROPLISTITEM));
      if(!NT_SUCCESS(Status))
      {
        ExReleaseFastMutexUnsafe(&WindowObject->PropListLock);
        IntReleaseWindowObject(WindowObject);
        return Status;
      }
      
      BufferSize -= sizeof(PROPLISTITEM);
      Cnt++;
      li++;
      ListEntry = ListEntry->Flink;
    }
    
    ExReleaseFastMutexUnsafe(&WindowObject->PropListLock);
  }
  else
  {
    ExAcquireFastMutexUnsafe(&WindowObject->PropListLock);
    Cnt = WindowObject->PropListItems * sizeof(PROPLISTITEM);
    ExReleaseFastMutexUnsafe(&WindowObject->PropListLock);
  }
  
  IntReleaseWindowObject(WindowObject);
  
  if(Count)
  {
    Status = MmCopyToCaller(Count, &Cnt, sizeof(DWORD));
    if(!NT_SUCCESS(Status))
    {
      return Status;
    }
  }
  
  return STATUS_SUCCESS;
}

HANDLE STDCALL
NtUserRemoveProp(HWND hWnd, ATOM Atom)
{
  PWINDOW_OBJECT WindowObject;
  PPROPERTY Prop;
  HANDLE Data;

  if (!(WindowObject = IntGetWindowObject(hWnd)))
  {
    SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
    return NULL;
  }
  
  ExAcquireFastMutexUnsafe(&WindowObject->PropListLock);
  Prop = IntGetProp(WindowObject, Atom);
  
  if (Prop == NULL)
    {
      ExReleaseFastMutexUnsafe(&WindowObject->PropListLock);
      IntReleaseWindowObject(WindowObject);
      return(NULL);
    }
  Data = Prop->Data;
  RemoveEntryList(&Prop->PropListEntry);
  ExFreePool(Prop);
  WindowObject->PropListItems--;
  ExReleaseFastMutexUnsafe(&WindowObject->PropListLock);
  IntReleaseWindowObject(WindowObject);
  return(Data);
}

HANDLE STDCALL
NtUserGetProp(HWND hWnd, ATOM Atom)
{
  PWINDOW_OBJECT WindowObject;
  PPROPERTY Prop;
  HANDLE Data = NULL;

  IntAcquireWinLockShared();

  if (!(WindowObject = IntGetWindowObject(hWnd)))
  {
    IntReleaseWinLock();
    SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
    return FALSE;
  }
  
  ExAcquireFastMutexUnsafe(&WindowObject->PropListLock);
  Prop = IntGetProp(WindowObject, Atom);
  ExReleaseFastMutexUnsafe(&WindowObject->PropListLock);
  if (Prop != NULL)
  {
    Data = Prop->Data;
  }
  
  IntReleaseWinLock();

  return(Data);
}

BOOL FASTCALL
IntSetProp(PWINDOW_OBJECT Wnd, ATOM Atom, HANDLE Data)
{
  PPROPERTY Prop;

  Prop = IntGetProp(Wnd, Atom);

  if (Prop == NULL)
  {
    Prop = ExAllocatePool(PagedPool, sizeof(PROPERTY));
    if (Prop == NULL)
    {
      return FALSE;
    }
    Prop->Atom = Atom;
    InsertTailList(&Wnd->PropListHead, &Prop->PropListEntry);
    Wnd->PropListItems++;
  }

  Prop->Data = Data;
  return TRUE;
}


BOOL STDCALL
NtUserSetProp(HWND hWnd, ATOM Atom, HANDLE Data)
{
  PWINDOW_OBJECT Wnd;
  BOOL ret;

  IntAcquireWinLockExclusive();

  if (!(Wnd = IntGetWindowObject(hWnd)))
  {
    IntReleaseWinLock();
    SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
    return FALSE;
  }
  
  ExAcquireFastMutexUnsafe(&Wnd->PropListLock);
  ret = IntSetProp(Wnd, Atom, Data);
  ExReleaseFastMutexUnsafe(&Wnd->PropListLock);
  
  IntReleaseWinLock();
  return ret;
}

/* EOF */
