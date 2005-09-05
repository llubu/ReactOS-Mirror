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
 * PURPOSE:          Menus
 * FILE:             subsys/win32k/ntuser/menu.c
 * PROGRAMER:        Thomas Weidenmueller (w3seek@users.sourceforge.net)
 * REVISION HISTORY:
 *       07/30/2003  CSH  Created
 */
/* INCLUDES ******************************************************************/

#include <w32k.h>

#define NDEBUG
#include <debug.h>

/* INTERNAL ******************************************************************/

/* maximum number of menu items a menu can contain */
#define MAX_MENU_ITEMS (0x4000)
#define MAX_GOINTOSUBMENU (0x10)

#define UpdateMenuItemState(state, change) \
{\
  if((change) & MFS_DISABLED) { \
    (state) |= MFS_DISABLED; \
  } else { \
    (state) &= ~MFS_DISABLED; \
  } \
  if((change) & MFS_CHECKED) { \
    (state) |= MFS_CHECKED; \
  } else { \
    (state) &= ~MFS_CHECKED; \
  } \
  if((change) & MFS_HILITE) { \
    (state) |= MFS_HILITE; \
  } else { \
    (state) &= ~MFS_HILITE; \
  } \
  if((change) & MFS_DEFAULT) { \
    (state) |= MFS_DEFAULT; \
  } else { \
    (state) &= ~MFS_DEFAULT; \
  } \
  if((change) & MF_MOUSESELECT) { \
    (state) |= MF_MOUSESELECT; \
  } else { \
    (state) &= ~MF_MOUSESELECT; \
  } \
}

#define FreeMenuText(MenuItem) \
{ \
  if((MENU_ITEM_TYPE((MenuItem)->fType) == MF_STRING) && \
           (MenuItem)->Text.Length) { \
    RtlFreeUnicodeString(&(MenuItem)->Text); \
  } \
}

#define InRect(r, x, y) \
      ( ( ((r).right >=  x)) && \
        ( ((r).left <= x)) && \
        ( ((r).bottom >=  y)) && \
        ( ((r).top <= y)) )

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
    if(MenuItem->Text.Length)
      DbgPrint(" %d. %wZ\n", ++cnt, &MenuItem->Text);
    else
      DbgPrint(" %d. NO TEXT dwTypeData==%d\n", ++cnt, (DWORD)MenuItem->Text.Buffer);
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
  PW32THREAD W32Thread = PsGetWin32Thread();

  if(!W32Thread)
  {
    return NULL;
  }

  NTSTATUS Status = ObmReferenceObjectByHandle(W32Thread->Desktop->WindowStation->HandleTable,
                                               hMenu, otMenu, (PVOID*)&MenuObject);
  if (!NT_SUCCESS(Status))
  {
    return NULL;
  }
  return MenuObject;
}

BOOL FASTCALL
IntFreeMenuItem(PMENU_OBJECT MenuObject, PMENU_ITEM MenuItem,
    BOOL RemoveFromList, BOOL bRecurse)
{
  FreeMenuText(MenuItem);
  if(RemoveFromList)
  {
    /* FIXME - Remove from List */
    MenuObject->MenuInfo.MenuItemCount--;
  }
  if(bRecurse && MenuItem->hSubMenu)
  {
    PMENU_OBJECT SubMenuObject;
    SubMenuObject = IntGetMenuObject(MenuItem->hSubMenu );
    if(SubMenuObject)
    {
      IntDestroyMenuObject(SubMenuObject, bRecurse, TRUE);
      IntReleaseMenuObject(SubMenuObject);
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
  MenuObject->MenuInfo.MenuItemCount = 0;
  MenuObject->MenuItemList = NULL;
  return res;
}

BOOL FASTCALL
IntDestroyMenuObject(PMENU_OBJECT MenuObject,
                     BOOL bRecurse, BOOL RemoveFromProcess)
{
  if(MenuObject)
  {
    PWINSTATION_OBJECT WindowStation;
    NTSTATUS Status;

    /* remove all menu items */
    IntLockMenuItems(MenuObject);
    IntDeleteMenuItems(MenuObject, bRecurse); /* do not destroy submenus */
    IntUnLockMenuItems(MenuObject);

    if(RemoveFromProcess)
    {
      IntLockProcessMenus((PW32PROCESS)MenuObject->Process->Win32Process);
      RemoveEntryList(&MenuObject->ListEntry);
      IntUnLockProcessMenus((PW32PROCESS)MenuObject->Process->Win32Process);
    }

    Status = ObReferenceObjectByHandle(MenuObject->Process->Win32WindowStation,
                                       0,
                                       ExWindowStationObjectType,
                                       KernelMode,
                                       (PVOID*)&WindowStation,
                                       NULL);
    if(NT_SUCCESS(Status))
    {
      ObmCloseHandle(WindowStation->HandleTable, MenuObject->MenuInfo.Self);
      ObDereferenceObject(WindowStation);
      return TRUE;
    }
  }
  return FALSE;
}

PMENU_OBJECT FASTCALL
IntCreateMenu(PHANDLE Handle, BOOL IsMenuBar)
{
  PMENU_OBJECT MenuObject;
  PW32THREAD Win32Thread = PsGetWin32Thread();

  MenuObject = (PMENU_OBJECT)ObmCreateObject(
      Win32Thread->Desktop->WindowStation->HandleTable, Handle,
      otMenu, sizeof(MENU_OBJECT));

  if(!MenuObject)
  {
    *Handle = 0;
    return NULL;
  }

  MenuObject->Process = PsGetCurrentProcess();
  MenuObject->RtoL = FALSE; /* default */
  MenuObject->MenuInfo.cbSize = sizeof(MENUINFO); /* not used */
  MenuObject->MenuInfo.fMask = 0; /* not used */
  MenuObject->MenuInfo.dwStyle = 0; /* FIXME */
  MenuObject->MenuInfo.cyMax = 0; /* default */
  MenuObject->MenuInfo.hbrBack =
     NtGdiCreateSolidBrush(RGB(192, 192, 192)); /* FIXME: default background color */
  MenuObject->MenuInfo.dwContextHelpID = 0; /* default */
  MenuObject->MenuInfo.dwMenuData = 0; /* default */
  MenuObject->MenuInfo.Self = *Handle;
  MenuObject->MenuInfo.FocusedItem = NO_SELECTED_ITEM;
  MenuObject->MenuInfo.Flags = (IsMenuBar ? 0 : MF_POPUP);
  MenuObject->MenuInfo.Wnd = NULL;
  MenuObject->MenuInfo.WndOwner = NULL;
  MenuObject->MenuInfo.Height = 0;
  MenuObject->MenuInfo.Width = 0;
  MenuObject->MenuInfo.TimeToHide = FALSE;

  MenuObject->MenuInfo.MenuItemCount = 0;
  MenuObject->MenuItemList = NULL;
  ExInitializeFastMutex(&MenuObject->MenuItemsLock);

  /* Insert menu item into process menu handle list */
  IntLockProcessMenus(PsGetWin32Process());
  InsertTailList(&PsGetWin32Process()->MenuListHead, &MenuObject->ListEntry);
  IntUnLockProcessMenus(PsGetWin32Process());

  return MenuObject;
}

BOOL FASTCALL
IntCloneMenuItems(PMENU_OBJECT Destination, PMENU_OBJECT Source)
{
  PMENU_ITEM MenuItem, NewMenuItem = NULL;
  PMENU_ITEM Old = NULL;

  if(!Source->MenuInfo.MenuItemCount)
    return FALSE;

  IntLockMenuItems(Destination);
  IntLockMenuItems(Source);

  MenuItem = Source->MenuItemList;
  while(MenuItem)
  {
    Old = NewMenuItem;
    if(NewMenuItem)
      NewMenuItem->Next = MenuItem;
    NewMenuItem = ExAllocatePoolWithTag(PagedPool, sizeof(MENU_ITEM), TAG_MENUITEM);
    if(!NewMenuItem)
      break;
    NewMenuItem->fType = MenuItem->fType;
    NewMenuItem->fState = MenuItem->fState;
    NewMenuItem->wID = MenuItem->wID;
    NewMenuItem->hSubMenu = MenuItem->hSubMenu;
    NewMenuItem->hbmpChecked = MenuItem->hbmpChecked;
    NewMenuItem->hbmpUnchecked = MenuItem->hbmpUnchecked;
    NewMenuItem->dwItemData = MenuItem->dwItemData;
    if((MENU_ITEM_TYPE(NewMenuItem->fType) == MF_STRING))
    {
      if(MenuItem->Text.Length)
      {
        NewMenuItem->Text.Length = 0;
        NewMenuItem->Text.MaximumLength = MenuItem->Text.MaximumLength;
        NewMenuItem->Text.Buffer = (PWSTR)ExAllocatePoolWithTag(PagedPool, MenuItem->Text.MaximumLength, TAG_STRING);
        if(!NewMenuItem->Text.Buffer)
        {
          ExFreePool(NewMenuItem);
          break;
        }
        RtlCopyUnicodeString(&NewMenuItem->Text, &MenuItem->Text);
      }
      else
      {
        NewMenuItem->Text.Buffer = MenuItem->Text.Buffer;
      }
    }
    else
    {
      NewMenuItem->Text.Buffer = MenuItem->Text.Buffer;
    }
    NewMenuItem->hbmpItem = MenuItem->hbmpItem;

    NewMenuItem->Next = NULL;
    if(Old)
      Old->Next = NewMenuItem;
    else
      Destination->MenuItemList = NewMenuItem;
    Destination->MenuInfo.MenuItemCount++;
    MenuItem = MenuItem->Next;
  }

  IntUnLockMenuItems(Source);
  IntUnLockMenuItems(Destination);
  return TRUE;
}

PMENU_OBJECT FASTCALL
IntCloneMenu(PMENU_OBJECT Source)
{
  HANDLE Handle;
  PMENU_OBJECT MenuObject;

  if(!Source)
    return NULL;

  MenuObject = (PMENU_OBJECT)ObmCreateObject(
    PsGetWin32Thread()->Desktop->WindowStation->HandleTable, &Handle,
    otMenu, sizeof(MENU_OBJECT));
  if(!MenuObject)
    return NULL;

  MenuObject->Process = PsGetCurrentProcess();
  MenuObject->RtoL = Source->RtoL;
  MenuObject->MenuInfo.cbSize = sizeof(MENUINFO); /* not used */
  MenuObject->MenuInfo.fMask = Source->MenuInfo.fMask;
  MenuObject->MenuInfo.dwStyle = Source->MenuInfo.dwStyle;
  MenuObject->MenuInfo.cyMax = Source->MenuInfo.cyMax;
  MenuObject->MenuInfo.hbrBack = Source->MenuInfo.hbrBack;
  MenuObject->MenuInfo.dwContextHelpID = Source->MenuInfo.dwContextHelpID;
  MenuObject->MenuInfo.dwMenuData = Source->MenuInfo.dwMenuData;
  MenuObject->MenuInfo.Self = Handle;
  MenuObject->MenuInfo.FocusedItem = NO_SELECTED_ITEM;
  MenuObject->MenuInfo.Wnd = NULL;
  MenuObject->MenuInfo.WndOwner = NULL;
  MenuObject->MenuInfo.Height = 0;
  MenuObject->MenuInfo.Width = 0;
  MenuObject->MenuInfo.TimeToHide = FALSE;

  MenuObject->MenuInfo.MenuItemCount = 0;
  MenuObject->MenuItemList = NULL;
  ExInitializeFastMutex(&MenuObject->MenuItemsLock);

  /* Insert menu item into process menu handle list */
  IntLockProcessMenus(PsGetWin32Process());
  InsertTailList(&PsGetWin32Process()->MenuListHead, &MenuObject->ListEntry);
  IntUnLockProcessMenus(PsGetWin32Process());

  IntCloneMenuItems(MenuObject, Source);

  return MenuObject;
}

BOOL FASTCALL
IntSetMenuFlagRtoL(PMENU_OBJECT MenuObject)
{
  MenuObject->RtoL = TRUE;
  return TRUE;
}

BOOL FASTCALL
IntSetMenuContextHelpId(PMENU_OBJECT MenuObject, DWORD dwContextHelpId)
{
  MenuObject->MenuInfo.dwContextHelpID = dwContextHelpId;
  return TRUE;
}

BOOL FASTCALL
IntGetMenuInfo(PMENU_OBJECT MenuObject, PROSMENUINFO lpmi)
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
  if (sizeof(MENUINFO) < lpmi->cbSize)
    {
      RtlCopyMemory((char *) lpmi + sizeof(MENUINFO),
                    (char *) &MenuObject->MenuInfo + sizeof(MENUINFO),
                    lpmi->cbSize - sizeof(MENUINFO));
    }

  return TRUE;
}


BOOL FASTCALL
IntIsMenu(HMENU hMenu)
{
  PMENU_OBJECT Menu;

  if((Menu = IntGetMenuObject(hMenu)))
  {
    IntReleaseMenuObject(Menu);
    return TRUE;
  }
  return FALSE;
}


BOOL FASTCALL
IntSetMenuInfo(PMENU_OBJECT MenuObject, PROSMENUINFO lpmi)
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
  if (sizeof(MENUINFO) < lpmi->cbSize)
    {
      MenuObject->MenuInfo.FocusedItem = lpmi->FocusedItem;
      MenuObject->MenuInfo.Height = lpmi->Height;
      MenuObject->MenuInfo.Width = lpmi->Width;
      MenuObject->MenuInfo.Wnd = lpmi->Wnd;
      MenuObject->MenuInfo.WndOwner = lpmi->WndOwner;
      MenuObject->MenuInfo.TimeToHide = lpmi->TimeToHide;
    }

  return TRUE;
}


int FASTCALL
IntGetMenuItemByFlag(PMENU_OBJECT MenuObject, UINT uSearchBy, UINT fFlag,
                     PMENU_ITEM *MenuItem, PMENU_ITEM *PrevMenuItem)
{
  PMENU_ITEM PrevItem = NULL;
  PMENU_ITEM CurItem = MenuObject->MenuItemList;
  int p;
  int ret;

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
      else if (0 != (CurItem->fType & MF_POPUP))
      {
        MenuObject = IntGetMenuObject(CurItem->hSubMenu);
        if (NULL != MenuObject)
        {
          ret = IntGetMenuItemByFlag(MenuObject, uSearchBy, fFlag,
                                     MenuItem, PrevMenuItem);
          if (-1 != ret)
          {
            IntReleaseMenuObject(MenuObject);
            return ret;
          }
        }
        IntReleaseMenuObject(MenuObject);
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
      /* append item */
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
  MenuObject->MenuInfo.MenuItemCount++;

  return npos;
}

BOOL FASTCALL
IntGetMenuItemInfo(PMENU_OBJECT MenuObject, PMENU_ITEM MenuItem, PROSMENUITEMINFO lpmii)
{
  NTSTATUS Status;

  if(lpmii->fMask & MIIM_BITMAP)
    {
      lpmii->hbmpItem = MenuItem->hbmpItem;
    }
  if(lpmii->fMask & MIIM_CHECKMARKS)
    {
      lpmii->hbmpChecked = MenuItem->hbmpChecked;
      lpmii->hbmpUnchecked = MenuItem->hbmpUnchecked;
    }
  if(lpmii->fMask & MIIM_DATA)
    {
      lpmii->dwItemData = MenuItem->dwItemData;
    }
  if(lpmii->fMask & (MIIM_FTYPE | MIIM_TYPE))
    {
      lpmii->fType = MenuItem->fType;
    }
  if(lpmii->fMask & MIIM_ID)
    {
      lpmii->wID = MenuItem->wID;
    }
  if(lpmii->fMask & MIIM_STATE)
    {
      lpmii->fState = MenuItem->fState;
    }
  if(lpmii->fMask & MIIM_SUBMENU)
    {
      lpmii->hSubMenu = MenuItem->hSubMenu;
    }
  if (lpmii->fMask & (MIIM_STRING | MIIM_TYPE))
    {
      if (lpmii->dwTypeData == NULL)
        {
          lpmii->cch = MenuItem->Text.Length / sizeof(WCHAR);
        }
      else
        {
          Status = MmCopyToCaller(lpmii->dwTypeData, MenuItem->Text.Buffer,
                                  min(lpmii->cch * sizeof(WCHAR),
                                      MenuItem->Text.MaximumLength));
          if (! NT_SUCCESS(Status))
            {
              SetLastNtError(Status);
              return FALSE;
            }
        }
    }

  if (sizeof(ROSMENUITEMINFO) == lpmii->cbSize)
    {
      lpmii->Rect = MenuItem->Rect;
      lpmii->XTab = MenuItem->XTab;
    }

  return TRUE;
}

BOOL FASTCALL
IntSetMenuItemInfo(PMENU_OBJECT MenuObject, PMENU_ITEM MenuItem, PROSMENUITEMINFO lpmii)
{
  PMENU_OBJECT SubMenuObject;

  if(!MenuItem || !MenuObject || !lpmii)
  {
    return FALSE;
  }

  MenuItem->fType = lpmii->fType;

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
    /*
     * Delete the menu item type when changing type from
     * MF_STRING.
     */
    if (MenuItem->fType != lpmii->fType &&
        MENU_ITEM_TYPE(MenuItem->fType) == MF_STRING)
    {
      FreeMenuText(MenuItem);
      RtlInitUnicodeString(&MenuItem->Text, NULL);
    }
    MenuItem->fType = lpmii->fType;
  }
  if(lpmii->fMask & MIIM_ID)
  {
    MenuItem->wID = lpmii->wID;
  }
  if(lpmii->fMask & MIIM_STATE)
  {
    /* remove MFS_DEFAULT flag from all other menu items if this item
       has the MFS_DEFAULT state */
    if(lpmii->fState & MFS_DEFAULT)
      IntSetMenuDefaultItem(MenuObject, -1, 0);
    /* update the menu item state flags */
    UpdateMenuItemState(MenuItem->fState, lpmii->fState);
  }

  if(lpmii->fMask & MIIM_SUBMENU)
  {
    MenuItem->hSubMenu = lpmii->hSubMenu;
    /* Make sure the submenu is marked as a popup menu */
    if (MenuItem->hSubMenu)
    {
      SubMenuObject = IntGetMenuObject(MenuItem->hSubMenu);
      if (SubMenuObject != NULL)
      {
        SubMenuObject->MenuInfo.Flags |= MF_POPUP;
        MenuItem->fType |= MF_POPUP;
        IntReleaseMenuObject(SubMenuObject);
      }
      else
      {
        MenuItem->fType &= ~MF_POPUP;
      }
    }
    else
    {
      MenuItem->fType &= ~MF_POPUP;
    }
  }
  if ((lpmii->fMask & (MIIM_TYPE | MIIM_STRING)) &&
      (MENU_ITEM_TYPE(lpmii->fType) == MF_STRING))
  {
    FreeMenuText(MenuItem);

    if(lpmii->dwTypeData && lpmii->cch)
    {
      UNICODE_STRING Source;

      Source.Length =
      Source.MaximumLength = lpmii->cch * sizeof(WCHAR);
      Source.Buffer = lpmii->dwTypeData;

      MenuItem->Text.Buffer = (PWSTR)ExAllocatePoolWithTag(
        PagedPool, Source.Length + sizeof(WCHAR), TAG_STRING);
      if(MenuItem->Text.Buffer != NULL)
      {
        MenuItem->Text.Length = 0;
        MenuItem->Text.MaximumLength = Source.Length + sizeof(WCHAR);
        RtlCopyUnicodeString(&MenuItem->Text, &Source);
        MenuItem->Text.Buffer[MenuItem->Text.Length / sizeof(WCHAR)] = 0;
      }
      else
      {
        RtlInitUnicodeString(&MenuItem->Text, NULL);
      }
    }
    else
    {
      MenuItem->fType |= MF_SEPARATOR;
      RtlInitUnicodeString(&MenuItem->Text, NULL);
    }
  }

  if (sizeof(ROSMENUITEMINFO) == lpmii->cbSize)
    {
      MenuItem->Rect = lpmii->Rect;
      MenuItem->XTab = lpmii->XTab;
    }

  return TRUE;
}

BOOL FASTCALL
IntInsertMenuItem(PMENU_OBJECT MenuObject, UINT uItem, BOOL fByPosition,
                  PROSMENUITEMINFO ItemInfo)
{
  int pos = (int)uItem;
  PMENU_ITEM MenuItem;

  if (MAX_MENU_ITEMS <= MenuObject->MenuInfo.MenuItemCount)
    {
      SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
      return FALSE;
    }

  if (fByPosition)
    {
      /* calculate position */
      if(MenuObject->MenuInfo.MenuItemCount < pos)
        {
          pos = MenuObject->MenuInfo.MenuItemCount;
        }
    }
  else
    {
      pos = IntGetMenuItemByFlag(MenuObject, uItem, MF_BYCOMMAND, NULL, NULL);
    }
  if (pos < -1)
    {
      pos = -1;
    }

  MenuItem = ExAllocatePoolWithTag(PagedPool, sizeof(MENU_ITEM), TAG_MENUITEM);
  if (NULL == MenuItem)
    {
      SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
      return FALSE;
    }

  MenuItem->fType = MFT_STRING;
  MenuItem->fState = MFS_ENABLED | MFS_UNCHECKED;
  MenuItem->wID = 0;
  MenuItem->hSubMenu = (HMENU)0;
  MenuItem->hbmpChecked = (HBITMAP)0;
  MenuItem->hbmpUnchecked = (HBITMAP)0;
  MenuItem->dwItemData = 0;
  RtlInitUnicodeString(&MenuItem->Text, NULL);
  MenuItem->hbmpItem = (HBITMAP)0;

  if (! IntSetMenuItemInfo(MenuObject, MenuItem, ItemInfo))
    {
      ExFreePool(MenuItem);
      return FALSE;
    }

  /* Force size recalculation! */
  MenuObject->MenuInfo.Height = 0;

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


DWORD FASTCALL
IntBuildMenuItemList(PMENU_OBJECT MenuObject, PVOID Buffer, ULONG nMax)
{
  DWORD res = 0;
  UINT sz;
  ROSMENUITEMINFO mii;
  PVOID Buf;
  PMENU_ITEM CurItem = MenuObject->MenuItemList;
  PWCHAR StrOut;
  NTSTATUS Status;
  WCHAR NulByte;

  if (0 != nMax)
    {
      if (nMax < MenuObject->MenuInfo.MenuItemCount * sizeof(ROSMENUITEMINFO))
        {
          return 0;
        }
      StrOut = (PWCHAR)((char *) Buffer + MenuObject->MenuInfo.MenuItemCount
                                          * sizeof(ROSMENUITEMINFO));
      nMax -= MenuObject->MenuInfo.MenuItemCount * sizeof(ROSMENUITEMINFO);
      sz = sizeof(ROSMENUITEMINFO);
      Buf = Buffer;
      mii.cbSize = sizeof(ROSMENUITEMINFO);
      mii.fMask = 0;
      NulByte = L'\0';

      while (NULL != CurItem)
        {
          mii.cch = CurItem->Text.Length / sizeof(WCHAR);
          mii.dwItemData = CurItem->dwItemData;
          if (0 != CurItem->Text.Length)
            {
              mii.dwTypeData = StrOut;
            }
          else
            {
              mii.dwTypeData = NULL;
            }
          mii.fState = CurItem->fState;
          mii.fType = CurItem->fType;
          mii.hbmpChecked = CurItem->hbmpChecked;
          mii.hbmpItem = CurItem->hbmpItem;
          mii.hbmpUnchecked = CurItem->hbmpUnchecked;
          mii.hSubMenu = CurItem->hSubMenu;
          mii.Rect = CurItem->Rect;
          mii.XTab = CurItem->XTab;

          Status = MmCopyToCaller(Buf, &mii, sizeof(ROSMENUITEMINFO));
          if (! NT_SUCCESS(Status))
            {
              SetLastNtError(Status);
              return 0;
            }
          Buf = (PVOID)((ULONG_PTR)Buf + sizeof(ROSMENUITEMINFO));

          if (0 != CurItem->Text.Length
              && (nMax >= CurItem->Text.Length + sizeof(WCHAR)))
            {
              /* copy string */
              Status = MmCopyToCaller(StrOut, CurItem->Text.Buffer,
                                      CurItem->Text.Length);
              if (! NT_SUCCESS(Status))
                {
                  SetLastNtError(Status);
                  return 0;
                }
              StrOut += CurItem->Text.Length / sizeof(WCHAR);
              Status = MmCopyToCaller(StrOut, &NulByte, sizeof(WCHAR));
              if (! NT_SUCCESS(Status))
                {
                  SetLastNtError(Status);
                  return 0;
                }
              StrOut++;
              nMax -= CurItem->Text.Length + sizeof(WCHAR);
            }
          else if (0 != CurItem->Text.Length)
            {
              break;
            }

          CurItem = CurItem->Next;
          res++;
        }
    }
  else
    {
      while (NULL != CurItem)
        {
          res += sizeof(ROSMENUITEMINFO) + CurItem->Text.Length + sizeof(WCHAR);
          CurItem = CurItem->Next;
        }
    }

  return res;
}


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

  if(uItem == (UINT)-1)
  {
    while(MenuItem)
    {
      if(MenuItem->fState & MFS_DEFAULT)
        MenuItem->fState ^= MFS_DEFAULT;
      MenuItem = MenuItem->Next;
    }
    return TRUE;
  }

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


UINT FASTCALL
IntGetMenuDefaultItem(PMENU_OBJECT MenuObject, UINT fByPos, UINT gmdiFlags,
  DWORD *gismc)
{
  UINT x = 0;
  UINT res = -1;
  UINT sres;
  PMENU_OBJECT SubMenuObject;
  PMENU_ITEM MenuItem = MenuObject->MenuItemList;

  while(MenuItem)
  {
    if(MenuItem->fState & MFS_DEFAULT)
    {

      if(!(gmdiFlags & GMDI_USEDISABLED) && (MenuItem->fState & MFS_DISABLED))
        break;

      if(fByPos & MF_BYPOSITION)
        res = x;
      else
        res = MenuItem->wID;

      if((*gismc < MAX_GOINTOSUBMENU) && (gmdiFlags & GMDI_GOINTOPOPUPS) &&
         MenuItem->hSubMenu)
      {

        SubMenuObject = IntGetMenuObject(MenuItem->hSubMenu);
        if(!SubMenuObject || (SubMenuObject == MenuObject))
          break;

        IntLockMenuItems(SubMenuObject);
        IntUnLockMenuItems(MenuObject);

        (*gismc)++;
        sres = IntGetMenuDefaultItem(SubMenuObject, fByPos, gmdiFlags, gismc);
        (*gismc)--;

        IntUnLockMenuItems(SubMenuObject);
        IntLockMenuItems(MenuObject);
        IntReleaseMenuObject(SubMenuObject);

        if(sres > (UINT)-1)
          res = sres;
      }

      break;
    }

    MenuItem = MenuItem->Next;
    x++;
  }

  return res;
}

VOID FASTCALL
co_IntInitTracking(PWINDOW_OBJECT WindowObject, PMENU_OBJECT MenuObject, BOOL Popup,
                UINT Flags)
{
  /* FIXME - hide caret */

  if(!(Flags & TPM_NONOTIFY))
    co_IntSendMessage(WindowObject->Self, WM_SETCURSOR, (WPARAM)WindowObject->Self, HTCAPTION);

  /* FIXME - send WM_SETCURSOR message */

  if(!(Flags & TPM_NONOTIFY))
    co_IntSendMessage(WindowObject->Self, WM_INITMENU, (WPARAM)MenuObject->MenuInfo.Self, 0);
}

VOID FASTCALL
co_IntExitTracking(PWINDOW_OBJECT WindowObject, PMENU_OBJECT MenuObject, BOOL Popup,
                UINT Flags)
{
  if(!(Flags & TPM_NONOTIFY))
    co_IntSendMessage(WindowObject->Self, WM_EXITMENULOOP, 0 /* FIXME */, 0);

  /* FIXME - Show caret again */
}

INT FASTCALL
IntTrackMenu(PMENU_OBJECT MenuObject, PWINDOW_OBJECT WindowObject, INT x, INT y,
             RECT lprect)
{
  return 0;
}

BOOL FASTCALL
co_IntTrackPopupMenu(PMENU_OBJECT MenuObject, PWINDOW_OBJECT WindowObject,
                  UINT Flags, POINT *Pos, UINT MenuPos, RECT *ExcludeRect)
{
  co_IntInitTracking(WindowObject, MenuObject, TRUE, Flags);

  co_IntExitTracking(WindowObject, MenuObject, TRUE, Flags);
  return FALSE;
}

BOOL FASTCALL
IntSetMenuItemRect(PMENU_OBJECT MenuObject, UINT Item, BOOL fByPos, RECT *rcRect)
{
  PMENU_ITEM mi;
  if(IntGetMenuItemByFlag(MenuObject, Item, (fByPos ? MF_BYPOSITION : MF_BYCOMMAND),
                          &mi, NULL) > -1)
  {
    mi->Rect = *rcRect;
    return TRUE;
  }
  return FALSE;
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
    KeAttachProcess(&Process->Pcb);
  }

  IntLockProcessMenus(Win32Process);
  while (Win32Process->MenuListHead.Flink != &(Win32Process->MenuListHead) &&
         Win32Process->MenuListHead.Flink != LastHead)
  {
    LastHead = Win32Process->MenuListHead.Flink;
    MenuObject = CONTAINING_RECORD(Win32Process->MenuListHead.Flink, MENU_OBJECT, ListEntry);

    IntUnLockProcessMenus(Win32Process);
    IntDestroyMenuObject(MenuObject, FALSE, TRUE);
    IntLockProcessMenus(Win32Process);
  }
  IntUnLockProcessMenus(Win32Process);

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
 VOID* Buffer,
 ULONG nBufSize,
 DWORD Reserved)
{
  DWORD res = -1;
  PMENU_OBJECT MenuObject;
  DECLARE_RETURN(DWORD);  
  
  DPRINT("Enter NtUserBuildMenuItemList\n");
  UserEnterExclusive();
  
  MenuObject = IntGetMenuObject(hMenu);
  if(!MenuObject)
  {
    SetLastWin32Error(ERROR_INVALID_MENU_HANDLE);
    RETURN( (DWORD)-1);
  }

  if(Buffer)
  {
    IntLockMenuItems(MenuObject);
    res = IntBuildMenuItemList(MenuObject, Buffer, nBufSize);
    IntUnLockMenuItems(MenuObject);
  }
  else
  {
    res = MenuObject->MenuInfo.MenuItemCount;
  }

  IntReleaseMenuObject(MenuObject);

  RETURN( res);
  
CLEANUP:
  DPRINT("Leave NtUserBuildMenuItemList, ret=%i\n",_ret_);
  UserLeave();
  END_CLEANUP;  
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
  PMENU_OBJECT MenuObject;
  DECLARE_RETURN(DWORD); 
  
  DPRINT("Enter NtUserCheckMenuItem\n");
  UserEnterExclusive();
  
  MenuObject = IntGetMenuObject(hmenu);
  if(!MenuObject)
  {
    SetLastWin32Error(ERROR_INVALID_MENU_HANDLE);
    RETURN( (DWORD)-1);
  }
  IntLockMenuItems(MenuObject);
  res = IntCheckMenuItem(MenuObject, uIDCheckItem, uCheck);
  IntUnLockMenuItems(MenuObject);
  IntReleaseMenuObject(MenuObject);
  RETURN( res);
  
CLEANUP:
  DPRINT("Leave NtUserCheckMenuItem, ret=%i\n",_ret_);
  UserLeave();
  END_CLEANUP;  
}


HMENU FASTCALL UserCreateMenu(BOOL PopupMenu)
{
  PWINSTATION_OBJECT WinStaObject;
  HANDLE Handle;

  NTSTATUS Status = IntValidateWindowStationHandle(PsGetCurrentProcess()->Win32WindowStation,
				       KernelMode,
				       0,
				       &WinStaObject);

  if (!NT_SUCCESS(Status))
  {
    DPRINT("Validation of window station handle (0x%X) failed\n",
      PsGetCurrentProcess()->Win32WindowStation);
    SetLastNtError(Status);
    return (HMENU)0;
  }

  IntCreateMenu(&Handle, !PopupMenu);

  ObDereferenceObject(WinStaObject);
  return (HMENU)Handle;
}



HMENU STDCALL
NtUserCreateMenu(BOOL PopupMenu)
{
  DECLARE_RETURN(HMENU);  
  
  DPRINT("Enter NtUserCreateMenu\n");
  UserEnterExclusive();
  
  RETURN(UserCreateMenu(PopupMenu));
   
CLEANUP:
  DPRINT("Leave NtUserCreateMenu, ret=%i\n",_ret_);
  UserLeave();
  END_CLEANUP;
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
  PMENU_OBJECT MenuObject;
  DECLARE_RETURN(BOOL);  

  DPRINT("Enter NtUserDeleteMenu\n");
  UserEnterExclusive();
  
  MenuObject = IntGetMenuObject(hMenu);
  if(!MenuObject)
  {
    SetLastWin32Error(ERROR_INVALID_MENU_HANDLE);
    RETURN( FALSE);
  }

  res = IntRemoveMenuItem(MenuObject, uPosition, uFlags, TRUE);
  IntReleaseMenuObject(MenuObject);

  RETURN( res);
  
CLEANUP:
  DPRINT("Leave NtUserDeleteMenu, ret=%i\n",_ret_);
  UserLeave();
  END_CLEANUP;
}



/*
 * @implemented
 */
BOOL FASTCALL UserDestroyMenu(HMENU hMenu)
{
  BOOL Ret;

  PMENU_OBJECT MenuObject = IntGetMenuObject(hMenu);
  if(!MenuObject)
  {
    SetLastWin32Error(ERROR_INVALID_MENU_HANDLE);
    return FALSE;
  }
  if(MenuObject->Process != PsGetCurrentProcess())
  {
    IntReleaseMenuObject(MenuObject);
    SetLastWin32Error(ERROR_ACCESS_DENIED);
    return FALSE;
  }

  Ret = IntDestroyMenuObject(MenuObject, FALSE, TRUE);

  IntReleaseMenuObject(MenuObject);
  return Ret;
}

/*
 * @implemented
 */
BOOL STDCALL
NtUserDestroyMenu(
  HMENU hMenu)
{
  BOOL Ret;
  DECLARE_RETURN(BOOL);  

  DPRINT("Enter NtUserDestroyMenu\n");
  UserEnterExclusive();

  PMENU_OBJECT MenuObject = IntGetMenuObject(hMenu);
  if(!MenuObject)
  {
    SetLastWin32Error(ERROR_INVALID_MENU_HANDLE);
    RETURN( FALSE);
  }
  if(MenuObject->Process != PsGetCurrentProcess())
  {
    IntReleaseMenuObject(MenuObject);
    SetLastWin32Error(ERROR_ACCESS_DENIED);
    RETURN( FALSE);
  }

  Ret = IntDestroyMenuObject(MenuObject, FALSE, TRUE);

  IntReleaseMenuObject(MenuObject);
  RETURN( Ret);
  
CLEANUP:
  DPRINT("Leave NtUserDestroyMenu, ret=%i\n",_ret_);
  UserLeave();
  END_CLEANUP;
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
  DECLARE_RETURN(UINT);  

  DPRINT("Enter NtUserEnableMenuItem\n");
  UserEnterExclusive();
  
  MenuObject = IntGetMenuObject(hMenu);
  if(!MenuObject)
  {
    SetLastWin32Error(ERROR_INVALID_MENU_HANDLE);
    RETURN( res);
  }
  IntLockMenuItems(MenuObject);
  res = IntEnableMenuItem(MenuObject, uIDEnableItem, uEnable);
  IntUnLockMenuItems(MenuObject);
  IntReleaseMenuObject(MenuObject);

  RETURN( res);
  
CLEANUP:
  DPRINT("Leave NtUserEnableMenuItem, ret=%i\n",_ret_);
  UserLeave();
  END_CLEANUP;
}


/*
 * @implemented
 */
DWORD STDCALL
NtUserInsertMenuItem(
  HMENU hMenu,
  UINT uItem,
  BOOL fByPosition,
  LPCMENUITEMINFOW UnsafeItemInfo)
{
  DWORD Res = 0;
  PMENU_OBJECT MenuObject;
  NTSTATUS Status;
  ROSMENUITEMINFO ItemInfo;
  DECLARE_RETURN(DWORD);  

  DPRINT("Enter NtUserInsertMenuItem\n");
  UserEnterExclusive();

  MenuObject = IntGetMenuObject(hMenu);
  if(!MenuObject)
    {
      SetLastWin32Error(ERROR_INVALID_MENU_HANDLE);
      RETURN( 0);
    }

  Status = MmCopyFromCaller(&ItemInfo, UnsafeItemInfo, sizeof(MENUITEMINFOW));
  if (! NT_SUCCESS(Status))
    {
      IntReleaseMenuObject(MenuObject);
      SetLastNtError(Status);
      RETURN( FALSE);
    }
  if (ItemInfo.cbSize != sizeof(MENUITEMINFOW))
    {
      IntReleaseMenuObject(MenuObject);
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      RETURN( FALSE);
    }

  IntLockMenuItems(MenuObject);
  Res = IntInsertMenuItem(MenuObject, uItem, fByPosition, &ItemInfo);
  IntUnLockMenuItems(MenuObject);
  IntReleaseMenuObject(MenuObject);

  RETURN( Res);
  
CLEANUP:
  DPRINT("Leave NtUserInsertMenuItem, ret=%i\n",_ret_);
  UserLeave();
  END_CLEANUP;
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
 * @implemented
 */
UINT STDCALL
NtUserGetMenuDefaultItem(
  HMENU hMenu,
  UINT fByPos,
  UINT gmdiFlags)
{
  PMENU_OBJECT MenuObject;
  UINT res = -1;
  DWORD gismc = 0;
  DECLARE_RETURN(UINT);  

  DPRINT("Enter NtUserGetMenuDefaultItem\n");
  UserEnterExclusive();

  MenuObject = IntGetMenuObject(hMenu);
  if(!MenuObject)
  {
    SetLastWin32Error(ERROR_INVALID_MENU_HANDLE);
    RETURN( res);
  }
  IntLockMenuItems(MenuObject);
  res = IntGetMenuDefaultItem(MenuObject, fByPos, gmdiFlags, &gismc);
  IntUnLockMenuItems(MenuObject);
  IntReleaseMenuObject(MenuObject);
  RETURN(res);
  
CLEANUP:
  DPRINT("Leave NtUserGetMenuDefaultItem, ret=%i\n",_ret_);
  UserLeave();
  END_CLEANUP;
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
 * @implemented
 */
BOOL STDCALL
NtUserGetMenuItemRect(
  HWND hWnd,
  HMENU hMenu,
  UINT uItem,
  LPRECT lprcItem)
{
     ROSMENUINFO mi;
     ROSMENUITEMINFO mii;
     HWND referenceHwnd;
     LPPOINT lpPoints;
     LPRECT lpRect;
     POINT FromOffset;
     LONG XMove, YMove;
     ULONG i;
     NTSTATUS Status;
     DECLARE_RETURN(BOOL);  

     DPRINT("Enter NtUserGetMenuItemRect\n");
     UserEnterShared();
          
     if(!UserMenuItemInfo(hMenu, uItem, MF_BYPOSITION, &mii, FALSE))
      RETURN( FALSE);
     	
     referenceHwnd = hWnd;

     if(!hWnd)
     {
   if(!UserMenuInfo(hMenu, &mi, FALSE)) RETURN( FALSE);
    if(mi.Wnd == 0) RETURN( FALSE);
	 referenceHwnd = mi.Wnd;
     }

     if (lprcItem == NULL) RETURN( FALSE);
     *lpRect = mii.Rect;
     lpPoints = (LPPOINT)lpRect;
      
    if(!UserGetClientOrigin(referenceHwnd, &FromOffset)) RETURN( FALSE);

    XMove = FromOffset.x;
    YMove = FromOffset.y;

    for (i = 0; i < 2; i++)
      {
        lpPoints[i].x += XMove;
        lpPoints[i].y += YMove;
      }
      
    Status = MmCopyToCaller(lprcItem, lpPoints, sizeof(POINT));
    if (! NT_SUCCESS(Status))
      {
        SetLastNtError(Status);
        RETURN( FALSE);
      }                                                              
    RETURN( TRUE);
    
CLEANUP:
  DPRINT("Leave NtUserGetMenuItemRect, ret=%i\n",_ret_);
  UserLeave();
  END_CLEANUP;
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
  PWINDOW_OBJECT WindowObject;
  DECLARE_RETURN(BOOLEAN);  

  DPRINT("Enter NtUserHiliteMenuItem\n");
  UserEnterExclusive();
  
  WindowObject = IntGetWindowObject(hwnd);
  if(!WindowObject)
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    RETURN( res);
  }
  MenuObject = IntGetMenuObject(hmenu);
  if(!MenuObject)
  {
    IntReleaseWindowObject(WindowObject);
    SetLastWin32Error(ERROR_INVALID_MENU_HANDLE);
    RETURN( res);
  }
  if(WindowObject->IDMenu == (UINT)hmenu)
  {
    IntLockMenuItems(MenuObject);
    res = IntHiliteMenuItem(WindowObject, MenuObject, uItemHilite, uHilite);
    IntUnLockMenuItems(MenuObject);
  }
  IntReleaseMenuObject(MenuObject);
  IntReleaseWindowObject(WindowObject);
  RETURN( res);
  
CLEANUP:
  DPRINT("Leave NtUserHiliteMenuItem, ret=%i\n",_ret_);
  UserLeave();
  END_CLEANUP;
}


/*
 * @implemented
 */
BOOL FASTCALL
UserMenuInfo(
 HMENU Menu,
 PROSMENUINFO UnsafeMenuInfo,
 BOOL SetOrGet)
{
  BOOL Res;
  PMENU_OBJECT MenuObject;
  DWORD Size;
  NTSTATUS Status;
  ROSMENUINFO MenuInfo;

  Status = MmCopyFromCaller(&Size, &UnsafeMenuInfo->cbSize, sizeof(DWORD));
  if (! NT_SUCCESS(Status))
    {
      SetLastNtError(Status);
      return( FALSE);
    }
  if(Size < sizeof(MENUINFO) || sizeof(ROSMENUINFO) < Size)
    {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return( FALSE);
    }
  Status = MmCopyFromCaller(&MenuInfo, UnsafeMenuInfo, Size);
  if (! NT_SUCCESS(Status))
    {
      SetLastNtError(Status);
      return( FALSE);
    }

  MenuObject = IntGetMenuObject(Menu);
  if (NULL == MenuObject)
    {
      SetLastWin32Error(ERROR_INVALID_MENU_HANDLE);
      return( FALSE);
    }

  if(SetOrGet)
    {
      /* Set MenuInfo */
      Res = IntSetMenuInfo(MenuObject, &MenuInfo);
    }
  else
    {
      /* Get MenuInfo */
      Res = IntGetMenuInfo(MenuObject, &MenuInfo);
      if (Res)
        {
          Status = MmCopyToCaller(UnsafeMenuInfo, &MenuInfo, Size);
          if (! NT_SUCCESS(Status))
            {
              SetLastNtError(Status);
              return( FALSE);
            }
        }
    }

  IntReleaseMenuObject(MenuObject);

  return( Res);
}





/*
 * @implemented
 */
BOOL
STDCALL
NtUserMenuInfo(
 HMENU Menu,
 PROSMENUINFO UnsafeMenuInfo,
 BOOL SetOrGet)
{
  DECLARE_RETURN(BOOL);  

  DPRINT("Enter NtUserMenuInfo\n");
  UserEnterShared();

  RETURN(UserMenuInfo(Menu, UnsafeMenuInfo, SetOrGet));
  
CLEANUP:
  DPRINT("Leave NtUserMenuInfo, ret=%i\n",_ret_);
  UserLeave();
  END_CLEANUP;
}



/*
 * @implemented
 */
int STDCALL
NtUserMenuItemFromPoint(
  HWND Wnd,
  HMENU Menu,
  DWORD X,
  DWORD Y)
{
  PMENU_OBJECT MenuObject;
  PWINDOW_OBJECT WindowObject = NULL;
  PMENU_ITEM mi;
  int i;
  DECLARE_RETURN(int);  

  DPRINT("Enter NtUserMenuItemFromPoint\n");
  UserEnterExclusive();

  MenuObject = IntGetMenuObject(Menu);
  if (NULL == MenuObject)
    {
      SetLastWin32Error(ERROR_INVALID_MENU_HANDLE);
      RETURN( -1);
    }

  WindowObject = IntGetWindowObject(MenuObject->MenuInfo.Wnd);
  if (NULL == WindowObject)
    {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      RETURN( -1);
    }
  X -= WindowObject->WindowRect.left;
  Y -= WindowObject->WindowRect.top;
  IntReleaseWindowObject(WindowObject);

  IntLockMenuItems(MenuObject);
  mi = MenuObject->MenuItemList;
  for (i = 0; NULL != mi; i++)
    {
      if (InRect(mi->Rect, X, Y))
        {
          break;
        }
      mi = mi->Next;
    }
  IntUnLockMenuItems(MenuObject);

  IntReleaseMenuObject(MenuObject);

  RETURN( (mi ? i : NO_SELECTED_ITEM));
  
CLEANUP:
  DPRINT("Leave NtUserMenuItemFromPoint, ret=%i\n",_ret_);
  UserLeave();
  END_CLEANUP;
}





BOOL FASTCALL
UserMenuItemInfo(
 HMENU Menu,
 UINT Item,
 BOOL ByPosition,
 PROSMENUITEMINFO UnsafeItemInfo,
 BOOL SetOrGet)
{
  PMENU_OBJECT MenuObject;
  PMENU_ITEM MenuItem;
  ROSMENUITEMINFO ItemInfo;
  NTSTATUS Status;
  UINT Size;
  BOOL Ret;

  MenuObject = IntGetMenuObject(Menu);
  if (NULL == MenuObject)
    {
      SetLastWin32Error(ERROR_INVALID_MENU_HANDLE);
      return( FALSE);
    }
  Status = MmCopyFromCaller(&Size, &UnsafeItemInfo->cbSize, sizeof(UINT));
  if (! NT_SUCCESS(Status))
    {
      IntReleaseMenuObject(MenuObject);
      SetLastNtError(Status);
      return( FALSE);
    }
  if (sizeof(MENUITEMINFOW) != Size
      && sizeof(MENUITEMINFOW) - sizeof(HBITMAP) != Size
      && sizeof(ROSMENUITEMINFO) != Size)
    {
      IntReleaseMenuObject(MenuObject);
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return( FALSE);
    }
  Status = MmCopyFromCaller(&ItemInfo, UnsafeItemInfo, Size);
  if (! NT_SUCCESS(Status))
    {
      IntReleaseMenuObject(MenuObject);
      SetLastNtError(Status);
      return( FALSE);
    }
  /* If this is a pre-0x0500 _WIN32_WINNT MENUITEMINFOW, you can't
     set/get hbmpItem */
  if (sizeof(MENUITEMINFOW) - sizeof(HBITMAP) == Size
      && 0 != (ItemInfo.fMask & MIIM_BITMAP))
    {
      IntReleaseMenuObject(MenuObject);
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return( FALSE);
    }

  if (IntGetMenuItemByFlag(MenuObject, Item,
                           (ByPosition ? MF_BYPOSITION : MF_BYCOMMAND),
                           &MenuItem, NULL) < 0)
    {
      IntReleaseMenuObject(MenuObject);
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return( FALSE);
    }

  if (SetOrGet)
    {
      Ret = IntSetMenuItemInfo(MenuObject, MenuItem, &ItemInfo);
    }
  else
    {
      Ret = IntGetMenuItemInfo(MenuObject, MenuItem, &ItemInfo);
      if (Ret)
        {
          Status = MmCopyToCaller(UnsafeItemInfo, &ItemInfo, Size);
          if (! NT_SUCCESS(Status))
            {
              IntReleaseMenuObject(MenuObject);
              SetLastNtError(Status);
              return( FALSE);
            }
        }
    }

  IntReleaseMenuObject(MenuObject);

  return( Ret);

}



/*
 * @implemented
 */
BOOL
STDCALL
NtUserMenuItemInfo(
 HMENU Menu,
 UINT Item,
 BOOL ByPosition,
 PROSMENUITEMINFO UnsafeItemInfo,
 BOOL SetOrGet)
{
  DECLARE_RETURN(BOOL);  

  DPRINT("Enter NtUserMenuItemInfo\n");
  UserEnterExclusive();

  RETURN( UserMenuItemInfo(Menu, Item, ByPosition, UnsafeItemInfo, SetOrGet));
  
CLEANUP:
  DPRINT("Leave NtUserMenuItemInfo, ret=%i\n",_ret_);
  UserLeave();
  END_CLEANUP;
  
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
  PMENU_OBJECT MenuObject;
  DECLARE_RETURN(BOOL);  

  DPRINT("Enter NtUserRemoveMenu\n");
  UserEnterExclusive();
  
  MenuObject = IntGetMenuObject(hMenu);
  if(!MenuObject)
  {
    SetLastWin32Error(ERROR_INVALID_MENU_HANDLE);
    RETURN( FALSE);
  }

  res = IntRemoveMenuItem(MenuObject, uPosition, uFlags, FALSE);
  IntReleaseMenuObject(MenuObject);

  RETURN( res);
  
CLEANUP:
  DPRINT("Leave NtUserRemoveMenu, ret=%i\n",_ret_);
  UserLeave();
  END_CLEANUP;
  
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
  PMENU_OBJECT MenuObject;
  DECLARE_RETURN(BOOL);  

  DPRINT("Enter NtUserSetMenuContextHelpId\n");
  UserEnterExclusive();
  
  MenuObject = IntGetMenuObject(hmenu);
  if(!MenuObject)
  {
    SetLastWin32Error(ERROR_INVALID_MENU_HANDLE);
    RETURN( FALSE);
  }

  res = IntSetMenuContextHelpId(MenuObject, dwContextHelpId);
  IntReleaseMenuObject(MenuObject);
  RETURN( res);
  
CLEANUP:
  DPRINT("Leave NtUserSetMenuContextHelpId, ret=%i\n",_ret_);
  UserLeave();
  END_CLEANUP;
}




/*
 * @implemented
 */
BOOL FASTCALL
UserSetMenuDefaultItem(
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
    return( FALSE);
  }
  IntLockMenuItems(MenuObject);
  res = IntSetMenuDefaultItem(MenuObject, uItem, fByPos);
  IntUnLockMenuItems(MenuObject);
  IntReleaseMenuObject(MenuObject);

  return( res);
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
  DECLARE_RETURN(BOOL);  

  DPRINT("Enter NtUserSetMenuDefaultItem\n");
  UserEnterExclusive();

  MenuObject = IntGetMenuObject(hMenu);
  if(!MenuObject)
  {
    SetLastWin32Error(ERROR_INVALID_MENU_HANDLE);
    RETURN( FALSE);
  }
  IntLockMenuItems(MenuObject);
  res = IntSetMenuDefaultItem(MenuObject, uItem, fByPos);
  IntUnLockMenuItems(MenuObject);
  IntReleaseMenuObject(MenuObject);

  RETURN( res);
  
CLEANUP:
  DPRINT("Leave NtUserSetMenuDefaultItem, ret=%i\n",_ret_);
  UserLeave();
  END_CLEANUP;
}


/*
 * @implemented
 */
BOOL STDCALL
NtUserSetMenuFlagRtoL(
  HMENU hMenu)
{
  BOOL res;
  PMENU_OBJECT MenuObject;
  DECLARE_RETURN(BOOL);  

  DPRINT("Enter NtUserSetMenuFlagRtoL\n");
  UserEnterExclusive();
  
  MenuObject = IntGetMenuObject(hMenu);
  if(!MenuObject)
  {
    SetLastWin32Error(ERROR_INVALID_MENU_HANDLE);
    RETURN( FALSE);
  }

  res = IntSetMenuFlagRtoL(MenuObject);
  IntReleaseMenuObject(MenuObject);
  RETURN( res);

CLEANUP:
  DPRINT("Leave NtUserSetMenuFlagRtoL, ret=%i\n",_ret_);
  UserLeave();
  END_CLEANUP;
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
 * @implemented
 */
 /* NOTE: unused function */
BOOL STDCALL
NtUserTrackPopupMenuEx(
  HMENU hmenu,
  UINT fuFlags,
  int x,
  int y,
  HWND hwnd,
  LPTPMPARAMS lptpm)
{
  PMENU_OBJECT MenuObject;
  PWINDOW_OBJECT WindowObject;
  TPMPARAMS Safetpm;
  NTSTATUS Status;
  POINT Pos;
  BOOL Ret;

  MenuObject = IntGetMenuObject(hmenu);
  if(!MenuObject)
  {
    SetLastWin32Error(ERROR_INVALID_MENU_HANDLE);
    return FALSE;
  }

  WindowObject = IntGetWindowObject(hwnd);
  if(!WindowObject)
  {
    IntReleaseMenuObject(MenuObject);
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return FALSE;
  }

  if(lptpm)
  {
    Status = MmCopyFromCaller(&Safetpm, lptpm, sizeof(TPMPARAMS));
    if(!NT_SUCCESS(Status))
    {
      IntReleaseWindowObject(WindowObject);
      IntReleaseMenuObject(MenuObject);
      SetLastNtError(Status);
      return FALSE;
    }
    if(Safetpm.cbSize != sizeof(TPMPARAMS))
    {
      IntReleaseWindowObject(WindowObject);
      IntReleaseMenuObject(MenuObject);
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return FALSE;
    }
  }

  Pos.x = x;
  Pos.y = y;

  Ret = co_IntTrackPopupMenu(MenuObject, WindowObject, fuFlags, &Pos, 0,
                          (lptpm ? &Safetpm.rcExclude : NULL));

  IntReleaseWindowObject(WindowObject);
  IntReleaseMenuObject(MenuObject);

  return Ret;
}


/* EOF */
