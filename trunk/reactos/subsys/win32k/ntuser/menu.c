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
/* $Id: menu.c,v 1.19 2003/08/20 10:08:53 weiden Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Menus
 * FILE:             subsys/win32k/ntuser/menu.c
 * PROGRAMER:        Thomas Weidenmueller (w3seek@users.sourceforge.net)
 * REVISION HISTORY:
 *       07/30/2003  CSH  Created
 */
/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <win32k/win32k.h>
#include <napi/win32.h>
#include <include/menu.h>
#include <include/error.h>
#include <include/winsta.h>
#include <include/object.h>
#include <include/guicheck.h>
#include <include/window.h>
#include <include/color.h>

#define NDEBUG
#include <debug.h>

/* INTERNAL ******************************************************************/

/* maximum number of menu items a menu can contain */
#define MAX_MENU_ITEMS (0x4000)

#ifndef MIIM_STRING
#define MIIM_STRING      (0x00000040)
#endif
#ifndef MIIM_BITMAP
#define MIIM_BITMAP      (0x00000080)
#endif
#ifndef MIIM_FTYPE
#define MIIM_FTYPE       (0x00000100)
#endif

/* TODO - Optimize */
#define UpdateMenuItemState(state, change) \
{\
  if((change) & MFS_DISABLED) { \
    if(!((state) & MFS_DISABLED)) (state) |= MFS_DISABLED; \
  } else { \
    if((state) & MFS_DISABLED) (state) ^= MFS_DISABLED; \
  } \
  if((change) & MFS_CHECKED) { \
    if(!((state) & MFS_CHECKED)) (state) |= MFS_CHECKED; \
  } else { \
    if((state) & MFS_CHECKED) (state) ^= MFS_CHECKED; \
  } \
  if((change) & MFS_HILITE) { \
    if(!((state) & MFS_HILITE)) (state) |= MFS_HILITE; \
  } else { \
    if((state) & MFS_HILITE) (state) ^= MFS_HILITE; \
  } \
}

#define FreeMenuText(MenuItem) \
{ \
  if((MENU_ITEM_TYPE((MenuItem)->fType) == MF_STRING) && \
           (MenuItem)->dwTypeData) { \
    ExFreePool((MenuItem)->dwTypeData); \
    (MenuItem)->dwTypeData = 0; \
    (MenuItem)->cch = 0; \
  } \
}

NTSTATUS FASTCALL
InitMenuImpl(VOID)
{
  return(STATUS_SUCCESS);
}

NTSTATUS FASTCALL
CleanupMenuImpl(VOID)
{
  return(STATUS_SUCCESS);
}

#if 0
void FASTCALL
DumpMenuItemList(PMENU_ITEM MenuItem)
{
  UINT cnt = 0;
  while(MenuItem)
  {
    if(MenuItem->dwTypeData)
      DbgPrint(" %d. %ws\n", ++cnt, (LPWSTR)MenuItem->dwTypeData);
    else
      DbgPrint(" %d. NO TEXT dwTypeData==%d\n", ++cnt, MenuItem->dwTypeData);
    DbgPrint("   fType=");
    if(MFT_BITMAP & MenuItem->fType) DbgPrint("MFT_BITMAP ");
    if(MFT_MENUBARBREAK & MenuItem->fType) DbgPrint("MFT_MENUBARBREAK ");
    if(MFT_MENUBREAK & MenuItem->fType) DbgPrint("MFT_MENUBREAK ");
    if(MFT_OWNERDRAW & MenuItem->fType) DbgPrint("MFT_OWNERDRAW ");
    if(MFT_RADIOCHECK & MenuItem->fType) DbgPrint("MFT_RADIOCHECK ");
    if(MFT_RIGHTJUSTIFY & MenuItem->fType) DbgPrint("MFT_RIGHTJUSTIFY ");
    if(MFT_SEPARATOR & MenuItem->fType) DbgPrint("MFT_SEPARATOR ");
    if(MFT_STRING & MenuItem->fType) DbgPrint("MFT_STRING ");
    DbgPrint("\n   fState=");
    if(MFS_DISABLED & MenuItem->fState) DbgPrint("MFS_DISABLED ");
    else DbgPrint("MFS_ENABLED ");
    if(MFS_CHECKED & MenuItem->fState) DbgPrint("MFS_CHECKED ");
    else DbgPrint("MFS_UNCHECKED ");
    if(MFS_HILITE & MenuItem->fState) DbgPrint("MFS_HILITE ");
    else DbgPrint("MFS_UNHILITE ");
    if(MFS_DEFAULT & MenuItem->fState) DbgPrint("MFS_DEFAULT ");
    if(MFS_GRAYED & MenuItem->fState) DbgPrint("MFS_GRAYED ");
    DbgPrint("\n   wId=%d\n", MenuItem->wID);
    MenuItem = MenuItem->Next;
  }
  DbgPrint("Entries: %d\n", cnt);
  return;
}
#endif

PMENU_OBJECT FASTCALL
IntGetMenuObject(HMENU hMenu)
{
  PMENU_OBJECT MenuObject;
  NTSTATUS Status = ObmReferenceObjectByHandle(PsGetWin32Process()->
                      WindowStation->HandleTable, hMenu, otMenu, 
                      (PVOID*)&MenuObject);
  if (!NT_SUCCESS(Status))
  {
    return NULL;
  }
  return MenuObject;
}

VOID FASTCALL
IntReleaseMenuObject(PMENU_OBJECT MenuObject)
{
  ObmDereferenceObject(MenuObject);
}

BOOL FASTCALL
IntFreeMenuItem(PMENU_OBJECT MenuObject, PMENU_ITEM MenuItem,
    BOOL RemoveFromList, BOOL bRecurse)
{
  FreeMenuText(MenuItem);
  if(RemoveFromList)
  {
    /* FIXME - Remove from List */
    MenuObject->MenuItemCount--;
  }
  if(bRecurse && MenuItem->hSubMenu)
  {
    PMENU_OBJECT SubMenuObject;
    SubMenuObject = (PMENU_OBJECT)IntGetWindowObject(
		MenuItem->hSubMenu );
    if(SubMenuObject)
    {
      IntDestroyMenuObject(SubMenuObject, bRecurse);
    }
  }
  
  /* Free memory */
  ExFreePool(MenuItem);
  
  return TRUE;
}

BOOL FASTCALL
IntRemoveMenuItem(PMENU_OBJECT MenuObject, UINT uPosition, UINT uFlags, 
                   BOOL bRecurse)
{
  PMENU_ITEM PrevMenuItem, MenuItem;
  if(IntGetMenuItemByFlag(MenuObject, uPosition, uFlags, &MenuItem, 
                           &PrevMenuItem) > -1)
  {
    if(MenuItem)
    {
      if(PrevMenuItem)
        PrevMenuItem->Next = MenuItem->Next;
      else
      {
        MenuObject->MenuItemList = MenuItem->Next;
      }
      return IntFreeMenuItem(MenuObject, MenuItem, TRUE, bRecurse);
    }
  }
  return FALSE;
}

UINT FASTCALL
IntDeleteMenuItems(PMENU_OBJECT MenuObject, BOOL bRecurse)
{
  UINT res = 0;
  PMENU_ITEM NextItem;
  PMENU_ITEM CurItem = MenuObject->MenuItemList;
  while(CurItem)
  {
    NextItem = CurItem->Next;
    IntFreeMenuItem(MenuObject, CurItem, FALSE, bRecurse);
    CurItem = NextItem;
    res++;
  }
  MenuObject->MenuItemCount = 0;
  MenuObject->MenuItemList = NULL;
  return res;
}

BOOL FASTCALL
IntDestroyMenuObject(PMENU_OBJECT MenuObject, BOOL bRecurse)
{
  PW32PROCESS W32Process;
  
  if(MenuObject)
  {  
    W32Process = PsGetWin32Process();
    /* remove all menu items */
    ExAcquireFastMutexUnsafe (&MenuObject->MenuItemsLock);
    IntDeleteMenuItems(MenuObject, bRecurse); /* do not destroy submenus */
    ExReleaseFastMutexUnsafe (&MenuObject->MenuItemsLock);
    
    ExAcquireFastMutexUnsafe(&W32Process->MenuListLock);
    RemoveEntryList(&MenuObject->ListEntry);
    ExReleaseFastMutexUnsafe(&W32Process->MenuListLock);
    
    IntReleaseMenuObject(MenuObject); // needed?
    
    ObmCloseHandle(W32Process->WindowStation->HandleTable, MenuObject->Self);
  
    return TRUE;
  }
  return FALSE;
}

PMENU_OBJECT FASTCALL
IntCreateMenu(PHANDLE Handle)
{
  PW32PROCESS Win32Process = PsGetWin32Process();
  
  PMENU_OBJECT MenuObject = (PMENU_OBJECT)ObmCreateObject(
      Win32Process->WindowStation->HandleTable, Handle, 
      otMenu, sizeof(MENU_OBJECT));
  
  if(!MenuObject)
  {
    *Handle = 0;
    return NULL;
  }

  MenuObject->Self = *Handle;
  MenuObject->RtoL = FALSE; /* default */
  MenuObject->MenuInfo.cbSize = sizeof(MENUINFO); /* not used */
  MenuObject->MenuInfo.fMask = 0; /* not used */
  MenuObject->MenuInfo.dwStyle = 0; /* FIXME */
  MenuObject->MenuInfo.cyMax = 0; /* default */
  MenuObject->MenuInfo.hbrBack = NtGdiGetSysColorBrush(COLOR_MENU); /*default background color */
  MenuObject->MenuInfo.dwContextHelpID = 0; /* default */  
  MenuObject->MenuInfo.dwMenuData = 0; /* default */
  
  MenuObject->MenuItemCount = 0;
  MenuObject->MenuItemList = NULL;
  ExInitializeFastMutex(&MenuObject->MenuItemsLock);
  
  /* Insert menu item into process menu handle list */
  ExAcquireFastMutexUnsafe (&Win32Process->MenuListLock);
  InsertTailList (&Win32Process->MenuListHead,  &MenuObject->ListEntry);
  ExReleaseFastMutexUnsafe (&Win32Process->MenuListLock);
  
  return MenuObject;
}

BOOL FASTCALL
IntSetMenuFlagRtoL(PMENU_OBJECT MenuObject)
{
  if(MenuObject)
  {
    MenuObject->RtoL = TRUE;
    return TRUE;
  }
  return FALSE;
}

BOOL FASTCALL
IntSetMenuContextHelpId(PMENU_OBJECT MenuObject, DWORD dwContextHelpId)
{
  if(MenuObject)
  {
    MenuObject->MenuInfo.dwContextHelpID = dwContextHelpId;
    return TRUE;
  }
  return FALSE;
}

BOOL FASTCALL
IntGetMenuInfo(PMENU_OBJECT MenuObject, LPMENUINFO lpmi)
{
  if(MenuObject)
  {
    if(lpmi->fMask & MIM_BACKGROUND)
      lpmi->hbrBack = MenuObject->MenuInfo.hbrBack;
    if(lpmi->fMask & MIM_HELPID)
      lpmi->dwContextHelpID = MenuObject->MenuInfo.dwContextHelpID;
    if(lpmi->fMask & MIM_MAXHEIGHT)
      lpmi->cyMax = MenuObject->MenuInfo.cyMax;
    if(lpmi->fMask & MIM_MENUDATA)
      lpmi->dwMenuData = MenuObject->MenuInfo.dwMenuData;
    if(lpmi->fMask & MIM_STYLE)
      lpmi->dwStyle = MenuObject->MenuInfo.dwStyle;
    return TRUE;
  }
  return FALSE;
}

BOOL FASTCALL
IntSetMenuInfo(PMENU_OBJECT MenuObject, LPMENUINFO lpmi)
{
  if(MenuObject)
  {
    if(lpmi->fMask & MIM_BACKGROUND)
      MenuObject->MenuInfo.hbrBack = lpmi->hbrBack;
    if(lpmi->fMask & MIM_HELPID)
      MenuObject->MenuInfo.dwContextHelpID = lpmi->dwContextHelpID;
    if(lpmi->fMask & MIM_MAXHEIGHT)
      MenuObject->MenuInfo.cyMax = lpmi->cyMax;
    if(lpmi->fMask & MIM_MENUDATA)
      MenuObject->MenuInfo.dwMenuData = lpmi->dwMenuData;
    if(lpmi->fMask & MIM_STYLE)
      MenuObject->MenuInfo.dwStyle = lpmi->dwStyle;
    if(lpmi->fMask & MIM_APPLYTOSUBMENUS)
    {
      /* FIXME */
    }
    return TRUE;
  }
  return FALSE;
}


int FASTCALL
IntGetMenuItemByFlag(PMENU_OBJECT MenuObject, UINT uSearchBy, UINT fFlag, 
                      PMENU_ITEM *MenuItem, PMENU_ITEM *PrevMenuItem)
{
  PMENU_ITEM PrevItem = NULL;
  PMENU_ITEM CurItem = MenuObject->MenuItemList;
  int p;
  if(MF_BYPOSITION & fFlag)
  {
    p = uSearchBy;
    while(CurItem && (p > 0))
    {
      PrevItem = CurItem;
      CurItem = CurItem->Next;
      p--;
    }
    if(CurItem)
    {
      if(MenuItem) *MenuItem = CurItem;
      if(PrevMenuItem) *PrevMenuItem = PrevItem;
    }
    else
    {
      if(MenuItem) *MenuItem = NULL;
      if(PrevMenuItem) *PrevMenuItem = NULL; /* ? */
      return -1;
    }

    return uSearchBy - p;
  }
  else
  {
    p = 0;
    while(CurItem)
    {
      if(CurItem->wID == uSearchBy)
      {
        if(MenuItem) *MenuItem = CurItem;
        if(PrevMenuItem) *PrevMenuItem = PrevItem;
        return p;
      }
      PrevItem = CurItem;
      CurItem = CurItem->Next;
      p++;
    }
  }
  return -1;
}


int FASTCALL
IntInsertMenuItemToList(PMENU_OBJECT MenuObject, PMENU_ITEM MenuItem, int pos)
{
  PMENU_ITEM CurItem;
  PMENU_ITEM LastItem = NULL;
  UINT npos = 0;
  
  CurItem = MenuObject->MenuItemList;
  if(pos <= -1)
  {
    while(CurItem)
    {
      LastItem = CurItem;
      CurItem = CurItem->Next;
      npos++;
    }
  }
  else
  {
    while(CurItem && (pos > 0))
    {
      LastItem = CurItem;
      CurItem = CurItem->Next;
      pos--;
      npos++;
    }
  }

  if(CurItem)
  {
    if(LastItem)
    {
      /* insert the item before CurItem */
      MenuItem->Next = LastItem->Next;
      LastItem->Next = MenuItem;
    }
    else
    {
      /* insert at the beginning */
      MenuObject->MenuItemList = MenuItem;
      MenuItem->Next = CurItem;
    }
  }
  else
  {
    if(LastItem)
    {
      /* insert at the end */
      LastItem->Next = MenuItem;
      MenuItem->Next = NULL;
    }
    else
    {
      /* insert first item */
      MenuObject->MenuItemList = MenuItem;
      MenuItem->Next = NULL;
    }
  }
  MenuObject->MenuItemCount++;
  
  return npos;
}

BOOL FASTCALL
IntSetMenuItemInfo(PMENU_OBJECT MenuObject, PMENU_ITEM MenuItem, LPCMENUITEMINFOW lpmii)
{
  if(!MenuItem || !MenuObject || !lpmii)
  {
    return FALSE;
  }

  /*if((MenuItem->fMask & (MIIM_TYPE | MIIM_STRING)) && 
           (MENU_ITEM_TYPE(MenuItem->fType) == MF_STRING) && 
           MenuItem->dwTypeData)
  {
    // delete old string
    ExFreePool(MenuItem->dwTypeData);
    MenuItem->dwTypeData = 0;
    MenuItem->cch = 0;
  }*/
  
  MenuItem->fType = lpmii->fType;
  MenuItem->cch = lpmii->cch;
  
  if(lpmii->fMask & MIIM_BITMAP)
  {
    MenuItem->hbmpItem = lpmii->hbmpItem;
  }
  if(lpmii->fMask & MIIM_CHECKMARKS)
  {
    MenuItem->hbmpChecked = lpmii->hbmpChecked;
    MenuItem->hbmpUnchecked = lpmii->hbmpUnchecked;
  }
  if(lpmii->fMask & MIIM_DATA)
  {
    MenuItem->dwItemData = lpmii->dwItemData;
  }
  if(lpmii->fMask & (MIIM_FTYPE | MIIM_TYPE))
  {
    MenuItem->fType = lpmii->fType;
  }
  if(lpmii->fMask & MIIM_ID)
  {
    MenuItem->wID = lpmii->wID;
  }
  if(lpmii->fMask & MIIM_STATE)
  {
    UpdateMenuItemState(MenuItem->fState, lpmii->fState);
    /* FIXME - only one item can have MFS_DEFAULT */
  }
  
  if(lpmii->fMask & MIIM_SUBMENU)
  {
    MenuItem->hSubMenu = lpmii->hSubMenu;
  }
  if((lpmii->fMask & (MIIM_TYPE | MIIM_STRING)) && 
           (MENU_ITEM_TYPE(lpmii->fType) == MF_STRING) && lpmii->dwTypeData)
  {
    FreeMenuText(MenuItem);
    MenuItem->dwTypeData = (LPWSTR)ExAllocatePool(PagedPool, (lpmii->cch + 1) * sizeof(WCHAR));
    if(!MenuItem->dwTypeData)
    {
      MenuItem->cch = 0;
      /* FIXME Set last error code? */
      SetLastWin32Error(STATUS_NO_MEMORY);
      return FALSE;
    }
    MenuItem->cch = lpmii->cch;
    memcpy(MenuItem->dwTypeData, lpmii->dwTypeData, (lpmii->cch + 1) * sizeof(WCHAR));
  }
  
  return TRUE;
}

BOOL FASTCALL
IntInsertMenuItem(PMENU_OBJECT MenuObject, UINT uItem, WINBOOL fByPosition,
                   LPCMENUITEMINFOW lpmii)
{
  int pos = (int)uItem;
  PMENU_ITEM MenuItem;
  
  if(MenuObject->MenuItemCount >= MAX_MENU_ITEMS)
  {
    /* FIXME Set last error code? */
    SetLastWin32Error(STATUS_NO_MEMORY);
    return FALSE;
  }
  
  if(fByPosition)
  {
    /* calculate position */
    if(pos > MenuObject->MenuItemCount)
      pos = MenuObject->MenuItemCount;
  }
  else
  {
    pos = IntGetMenuItemByFlag(MenuObject, uItem, MF_BYCOMMAND, NULL, NULL);
  }
  if(pos < -1) pos = -1;
  
  MenuItem = ExAllocatePool(PagedPool, sizeof(MENU_ITEM));
  if(!MenuItem)
  {
    /* FIXME Set last error code? */
    SetLastWin32Error(STATUS_NO_MEMORY);
    return FALSE;
  }
  
  MenuItem->fType = MFT_STRING;
  MenuItem->fState = MFS_ENABLED | MFS_UNCHECKED;
  MenuItem->wID = 0;
  MenuItem->hSubMenu = (HMENU)0;
  MenuItem->hbmpChecked = (HBITMAP)0;
  MenuItem->hbmpUnchecked = (HBITMAP)0;
  MenuItem->dwItemData = (ULONG_PTR)NULL;
  MenuItem->cch = 0;
  MenuItem->hbmpItem = (HBITMAP)0;  
  
  if(!IntSetMenuItemInfo(MenuObject, MenuItem, lpmii))
  {
    ExFreePool(MenuItem);
    return FALSE;
  }
  
  pos = IntInsertMenuItemToList(MenuObject, MenuItem, pos);

  return pos >= 0;
}

UINT FASTCALL
IntEnableMenuItem(PMENU_OBJECT MenuObject, UINT uIDEnableItem, UINT uEnable)
{
  PMENU_ITEM MenuItem;
  UINT res = IntGetMenuItemByFlag(MenuObject, uIDEnableItem, uEnable, &MenuItem, NULL);
  if(!MenuItem || (res == (UINT)-1))
  {
    return (UINT)-1;
  }
  
  res = MenuItem->fState & (MF_GRAYED | MF_DISABLED);
  
  if(uEnable & MF_DISABLED)
  {
    if(!(MenuItem->fState & MF_DISABLED))
        MenuItem->fState |= MF_DISABLED;
    if(uEnable & MF_GRAYED)
    {
      if(!(MenuItem->fState & MF_GRAYED))
          MenuItem->fState |= MF_GRAYED;
    }
  }
  else
  {
    if(uEnable & MF_GRAYED)
    {
      if(!(MenuItem->fState & MF_GRAYED))
          MenuItem->fState |= MF_GRAYED;
      if(!(MenuItem->fState & MF_DISABLED))
          MenuItem->fState |= MF_DISABLED;
    }
    else
    {
      if(MenuItem->fState & MF_DISABLED)
          MenuItem->fState ^= MF_DISABLED;
      if(MenuItem->fState & MF_GRAYED)
          MenuItem->fState ^= MF_GRAYED;
    }
  }
  
  return res;
}

/*
DWORD FASTCALL
IntBuildMenuItemList(PMENU_OBJECT MenuObject, MENUITEMINFOW *lpmiil, ULONG nMax)
{
  DWORD Index = 0;
  PMENU_ITEM CurItem = MenuObject->MenuItemList;
  while(CurItem && (nMax > 0))
  {
    memcpy(&lpmiil[Index], &CurItem->MenuItem, sizeof(MENUITEMINFOW));
    CurItem = CurItem->Next;
    Index++;
    nMax--;
  }
  return Index;
}
*/

DWORD FASTCALL
IntCheckMenuItem(PMENU_OBJECT MenuObject, UINT uIDCheckItem, UINT uCheck)
{
  PMENU_ITEM MenuItem;
  int res = -1;

  if((IntGetMenuItemByFlag(MenuObject, uIDCheckItem, uCheck, &MenuItem, NULL) < 0) || !MenuItem)
  {
    return -1;
  }

  res = (DWORD)(MenuItem->fState & MF_CHECKED);
  if(uCheck & MF_CHECKED)
  {
    if(!(MenuItem->fState & MF_CHECKED))
        MenuItem->fState |= MF_CHECKED;
  }
  else
  {
    if(MenuItem->fState & MF_CHECKED)
        MenuItem->fState ^= MF_CHECKED;
  }

  return (DWORD)res;
}

BOOL FASTCALL
IntHiliteMenuItem(PWINDOW_OBJECT WindowObject, PMENU_OBJECT MenuObject,
  UINT uItemHilite, UINT uHilite)
{
  PMENU_ITEM MenuItem;
  BOOL res = IntGetMenuItemByFlag(MenuObject, uItemHilite, uHilite, &MenuItem, NULL);
  if(!MenuItem || !res)
  {
    return FALSE;
  }
  
  if(uHilite & MF_HILITE)
  {
    if(!(MenuItem->fState & MF_HILITE))
        MenuItem->fState |= MF_HILITE;
  }
  else
  {
    if(MenuItem->fState & MF_HILITE)
        MenuItem->fState ^= MF_HILITE;
  }
  
  /* FIXME - update the window's menu */
  
  return TRUE;
}

BOOL FASTCALL
IntSetMenuDefaultItem(PMENU_OBJECT MenuObject, UINT uItem, UINT fByPos)
{
  BOOL ret = FALSE;
  PMENU_ITEM MenuItem = MenuObject->MenuItemList;  
  if(fByPos)
  {
    UINT pos = 0;
    while(MenuItem)
    {
      if(pos == uItem)
      {
        if(!(MenuItem->fState & MFS_DEFAULT))
          MenuItem->fState |= MFS_DEFAULT;
        ret = TRUE;
      }
      else
      {
        if(MenuItem->fState & MFS_DEFAULT)
          MenuItem->fState ^= MFS_DEFAULT;
      }
      pos++;
      MenuItem = MenuItem->Next;
    }
  }
  else
  {
    while(MenuItem)
    {
      if(!ret && (MenuItem->wID == uItem))
      {
        if(!(MenuItem->fState & MFS_DEFAULT))
          MenuItem->fState |= MFS_DEFAULT;
        ret = TRUE;
      }
      else
      {
        if(MenuItem->fState & MFS_DEFAULT)
          MenuItem->fState ^= MFS_DEFAULT;
      }
      MenuItem = MenuItem->Next;
    }
  }
  return ret;
}

/*!
 * Internal function. Called when the process is destroyed to free the remaining menu handles.
*/
BOOL FASTCALL
IntCleanupMenus(struct _EPROCESS *Process, PW32PROCESS Win32Process)
{
  PEPROCESS CurrentProcess;
  PLIST_ENTRY LastHead = NULL;
  PMENU_OBJECT MenuObject;

  CurrentProcess = PsGetCurrentProcess();
  if (CurrentProcess != Process)
  {
    KeAttachProcess(Process);
  }
  
  ExAcquireFastMutexUnsafe(&Win32Process->MenuListLock); 
  while (Win32Process->MenuListHead.Flink != &(Win32Process->MenuListHead) &&
         Win32Process->MenuListHead.Flink != LastHead)
  {
    LastHead = Win32Process->MenuListHead.Flink;
    MenuObject = CONTAINING_RECORD(Win32Process->MenuListHead.Flink, MENU_OBJECT, ListEntry);
    
    ExReleaseFastMutexUnsafe(&Win32Process->MenuListLock);
    IntDestroyMenuObject(MenuObject, FALSE);
    ExAcquireFastMutexUnsafe(&Win32Process->MenuListLock); 
  }
  ExReleaseFastMutexUnsafe(&Win32Process->MenuListLock);
  
  if (CurrentProcess != Process)
  {
    KeDetachProcess();
  }
  return TRUE;
}

/* FUNCTIONS *****************************************************************/


/*
 * @implemented
 */
DWORD
STDCALL
NtUserBuildMenuItemList(
 HMENU hMenu,
 LPCMENUITEMINFOW* lpmiil,
 ULONG nBufSize,
 DWORD Reserved)
{
  DWORD res = -1;
  PMENU_OBJECT MenuObject = IntGetMenuObject(hMenu);
  if(!MenuObject)
  {
    SetLastWin32Error(ERROR_INVALID_MENU_HANDLE);
    return (DWORD)-1;
  }
  
  if(lpmiil)
  {
    /* FIXME need to pass the menu strings to user32 somehow....
    
    ExAcquireFastMutexUnsafe(&MenuObject->MenuItemsLock);
    res = IntBuildMenuItemList(MenuObject, lpmiil, nBufSize / sizeof(LPCMENUITEMINFO));
    ExReleaseFastMutexUnsafe(&MenuObject->MenuItemsLock);
    */
  }
  else
  {
    res = MenuObject->MenuItemCount;
  }
  
  IntReleaseMenuObject(MenuObject);

  return res;
}


/*
 * @implemented
 */
DWORD STDCALL
NtUserCheckMenuItem(
  HMENU hmenu,
  UINT uIDCheckItem,
  UINT uCheck)
{
  DWORD res = 0;
  PMENU_OBJECT MenuObject = IntGetMenuObject(hmenu);
  if(!MenuObject)
  {
    SetLastWin32Error(ERROR_INVALID_MENU_HANDLE);
    return (DWORD)-1;
  }
  ExAcquireFastMutexUnsafe(&MenuObject->MenuItemsLock);
  res = IntCheckMenuItem(MenuObject, uIDCheckItem, uCheck);
  ExReleaseFastMutexUnsafe(&MenuObject->MenuItemsLock);
  IntReleaseMenuObject(MenuObject);
  return res;
}


/*
 * @implemented
 */
HMENU STDCALL
NtUserCreateMenu(VOID)
{
  PWINSTATION_OBJECT WinStaObject;
  HANDLE Handle;
  
  NTSTATUS Status = ValidateWindowStationHandle(PROCESS_WINDOW_STATION(),
				       KernelMode,
				       0,
				       &WinStaObject);
				       
  if (!NT_SUCCESS(Status))
  {
    DPRINT("Validation of window station handle (0x%X) failed\n",
      PROCESS_WINDOW_STATION());
    SetLastWin32Error(Status);
    return (HMENU)0;
  }

  IntCreateMenu(&Handle);
  ObDereferenceObject(WinStaObject);
  return (HMENU)Handle;
}


/*
 * @implemented
 */
BOOL STDCALL
NtUserDeleteMenu(
  HMENU hMenu,
  UINT uPosition,
  UINT uFlags)
{
  BOOL res;
  PMENU_OBJECT MenuObject = IntGetMenuObject(hMenu);
  if(!MenuObject)
  {
    SetLastWin32Error(ERROR_INVALID_MENU_HANDLE);
    return FALSE;
  }

  res = IntRemoveMenuItem(MenuObject, uPosition, uFlags, TRUE);
  IntReleaseMenuObject(MenuObject);
  
  return res;
}


/*
 * @implemented
 */
BOOL STDCALL
NtUserDestroyMenu(
  HMENU hMenu)
{
  /* FIXME, check if menu belongs to the process */
  
  PMENU_OBJECT MenuObject = IntGetMenuObject(hMenu);
  if(!MenuObject)
  {
    SetLastWin32Error(ERROR_INVALID_MENU_HANDLE);
    return FALSE;
  }

  return IntDestroyMenuObject(MenuObject, FALSE);
}


/*
 * @implemented
 */
UINT STDCALL
NtUserEnableMenuItem(
  HMENU hMenu,
  UINT uIDEnableItem,
  UINT uEnable)
{
  UINT res = (UINT)-1;
  PMENU_OBJECT MenuObject;
  MenuObject = IntGetMenuObject(hMenu);
  if(!MenuObject)
  {
    SetLastWin32Error(ERROR_INVALID_MENU_HANDLE);
    return res;
  }
  ExAcquireFastMutexUnsafe(&MenuObject->MenuItemsLock);
  res = IntEnableMenuItem(MenuObject, uIDEnableItem, uEnable);
  ExReleaseFastMutexUnsafe(&MenuObject->MenuItemsLock);
  IntReleaseMenuObject(MenuObject);

  return res;
}


/*
 * @implemented
 */
DWORD STDCALL
NtUserInsertMenuItem(
  HMENU hMenu,
  UINT uItem,
  WINBOOL fByPosition,
  LPCMENUITEMINFOW lpmii)
{
  DWORD res = 0;
  PMENU_OBJECT MenuObject;
  MenuObject = IntGetMenuObject(hMenu);
  if(!MenuObject)
  {
    SetLastWin32Error(ERROR_INVALID_MENU_HANDLE);
    return 0;
  }
  ExAcquireFastMutexUnsafe(&MenuObject->MenuItemsLock);
  res = IntInsertMenuItem(MenuObject, uItem, fByPosition, lpmii);
  ExReleaseFastMutexUnsafe(&MenuObject->MenuItemsLock);
  IntReleaseMenuObject(MenuObject);
  return res;
}


/*
 * @unimplemented
 */
BOOL STDCALL
NtUserEndMenu(VOID)
{
  UNIMPLEMENTED
  
  return 0;
}


/*
 * @unimplemented
 */
UINT STDCALL
NtUserGetMenuDefaultItem(
  HMENU hMenu,
  UINT fByPos,
  UINT gmdiFlags)
{
  UNIMPLEMENTED
  
  return -1;
}


/*
 * @unimplemented
 */
BOOL STDCALL
NtUserGetMenuBarInfo(
  HWND hwnd,
  LONG idObject,
  LONG idItem,
  PMENUBARINFO pmbi)
{
  UNIMPLEMENTED

  return 0;
}


/*
 * @unimplemented
 */
UINT STDCALL
NtUserGetMenuIndex(
  HMENU hMenu,
  UINT wID)
{
  UNIMPLEMENTED

  return 0;
}


/*
 * @unimplemented
 */
BOOL STDCALL
NtUserGetMenuItemRect(
  HWND hWnd,
  HMENU hMenu,
  UINT uItem,
  LPRECT lprcItem)
{
  UNIMPLEMENTED

  return 0;
}


/*
 * @implemented
 */
BOOL STDCALL
NtUserHiliteMenuItem(
  HWND hwnd,
  HMENU hmenu,
  UINT uItemHilite,
  UINT uHilite)
{
  BOOL res = FALSE;
  PMENU_OBJECT MenuObject;
  PWINDOW_OBJECT WindowObject = IntGetWindowObject(hwnd);
  if(!WindowObject)
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return res;
  }
  MenuObject = IntGetMenuObject(hmenu);
  if(!MenuObject)
  {
    IntReleaseWindowObject(WindowObject);
    SetLastWin32Error(ERROR_INVALID_MENU_HANDLE);
    return res;
  }
  if(WindowObject->Menu == hmenu)
  {
    ExAcquireFastMutexUnsafe(&MenuObject->MenuItemsLock);
    res = IntHiliteMenuItem(WindowObject, MenuObject, uItemHilite, uHilite);
    ExReleaseFastMutexUnsafe(&MenuObject->MenuItemsLock);
  }
  IntReleaseMenuObject(MenuObject);
  IntReleaseWindowObject(WindowObject);
  return res;
}


/*
 * @implemented
 */
BOOL
STDCALL
NtUserMenuInfo(
 HMENU hmenu,
 LPMENUINFO lpmi,
 BOOL fsog)
{
  BOOL res = FALSE;
  PMENU_OBJECT MenuObject;
  
  if(lpmi->cbSize != sizeof(MENUINFO))
  {
    /* FIXME - Set Last Error */
    return FALSE;
  }
  MenuObject = IntGetMenuObject(hmenu);
  if(!MenuObject)
  {
    SetLastWin32Error(ERROR_INVALID_MENU_HANDLE);
    return FALSE;
  }  
  if(fsog)
  {
    /* Set MenuInfo */
    res = IntSetMenuInfo(MenuObject, lpmi);  
  }
  else
  {
    /* Get MenuInfo */
    res = IntGetMenuInfo(MenuObject, lpmi);
  }  
  IntReleaseMenuObject(MenuObject);
  return res;
}


/*
 * @unimplemented
 */
int STDCALL
NtUserMenuItemFromPoint(
  HWND hWnd,
  HMENU hMenu,
  DWORD X,
  DWORD Y)
{
  UNIMPLEMENTED

  return 0;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
NtUserMenuItemInfo(
 HMENU hMenu,
 UINT uItem,
 BOOL fByPosition,
 LPMENUITEMINFOW lpmii,
 BOOL fsog)
{
  UNIMPLEMENTED

  return 0;
}


/*
 * @implemented
 */
BOOL STDCALL
NtUserRemoveMenu(
  HMENU hMenu,
  UINT uPosition,
  UINT uFlags)
{
  BOOL res;
  PMENU_OBJECT MenuObject = IntGetMenuObject(hMenu);
  if(!MenuObject)
  {
    SetLastWin32Error(ERROR_INVALID_MENU_HANDLE);
    return FALSE;
  }

  res = IntRemoveMenuItem(MenuObject, uPosition, uFlags, FALSE);
  IntReleaseMenuObject(MenuObject);
  
  return res;
}


/*
 * @implemented
 */
BOOL STDCALL
NtUserSetMenuContextHelpId(
  HMENU hmenu,
  DWORD dwContextHelpId)
{
  BOOL res;
  PMENU_OBJECT MenuObject = IntGetMenuObject(hmenu);
  if(!MenuObject)
  {
    SetLastWin32Error(ERROR_INVALID_MENU_HANDLE);
    return FALSE;
  }

  res = IntSetMenuContextHelpId(MenuObject, dwContextHelpId);
  IntReleaseMenuObject(MenuObject);
  return res;
}


/*
 * @implemented
 */
BOOL STDCALL
NtUserSetMenuDefaultItem(
  HMENU hMenu,
  UINT uItem,
  UINT fByPos)
{
  BOOL res = FALSE;
  PMENU_OBJECT MenuObject;
  MenuObject = IntGetMenuObject(hMenu);
  if(!MenuObject)
  {
    SetLastWin32Error(ERROR_INVALID_MENU_HANDLE);
    return FALSE;
  }
  ExAcquireFastMutexUnsafe(&MenuObject->MenuItemsLock);
  res = IntSetMenuDefaultItem(MenuObject, uItem, fByPos);
  ExReleaseFastMutexUnsafe(&MenuObject->MenuItemsLock);
  IntReleaseMenuObject(MenuObject);

  return res;
}


/*
 * @implemented
 */
BOOL STDCALL
NtUserSetMenuFlagRtoL(
  HMENU hMenu)
{
  BOOL res;
  PMENU_OBJECT MenuObject = IntGetMenuObject(hMenu);
  if(!MenuObject)
  {
    SetLastWin32Error(ERROR_INVALID_MENU_HANDLE);
    return FALSE;
  }

  res = IntSetMenuFlagRtoL(MenuObject);
  IntReleaseMenuObject(MenuObject);
  return res;
}


/*
 * @unimplemented
 */
DWORD STDCALL
NtUserThunkedMenuInfo(
  HMENU hMenu,
  LPCMENUINFO lpcmi)
{
  UNIMPLEMENTED
  /* This function seems just to call SetMenuInfo() */
  return 0;
}


/*
 * @unimplemented
 */
DWORD STDCALL
NtUserThunkedMenuItemInfo(
  HMENU hMenu,
  UINT uItem,
  BOOL fByPosition,
  BOOL bInsert,
  LPMENUITEMINFOW lpmii,
  PUNICODE_STRING lpszCaption)
{
  UNIMPLEMENTED
  /* lpszCaption may be NULL, check for it and call RtlInitUnicodeString()
     if bInsert == TRUE call NtUserInsertMenuItem() else NtUserSetMenuItemInfo()
  */
  return 0;
}


/*
 * @unimplemented
 */
BOOL STDCALL
NtUserTrackPopupMenuEx(
  HMENU hmenu,
  UINT fuFlags,
  int x,
  int y,
  HWND hwnd,
  LPTPMPARAMS lptpm)
{
  UNIMPLEMENTED

  return 0;
}


/* EOF */
