#ifndef _WIN32K_MENU_H
#define _WIN32K_MENU_H

#include <ddk/ntddk.h>
#include <napi/win32.h>
#include <win32k/menu.h>

#define IS_ATOM(x) \
  (((ULONG_PTR)(x) > 0x0) && ((ULONG_PTR)(x) < 0x10000))
  
#define MENU_ITEM_TYPE(flags) \
  ((flags) & (MF_STRING | MF_BITMAP | MF_OWNERDRAW | MF_SEPARATOR))
  
#ifndef MF_END
#define MF_END             (0x0080)
#endif

typedef struct _MENU_ITEM
{
  struct _MENU_ITEM *Next;
  UINT fType;
  UINT fState;
  UINT wID;
  HMENU hSubMenu;
  HBITMAP hbmpChecked;
  HBITMAP hbmpUnchecked;
  ULONG_PTR dwItemData;
  UNICODE_STRING Text;
  HBITMAP hbmpItem;
  RECT Rect;
  UINT XTab;
} MENU_ITEM, *PMENU_ITEM;

typedef struct _MENU_OBJECT
{
  PEPROCESS Process;
  LIST_ENTRY ListEntry;
  FAST_MUTEX MenuItemsLock;
  PMENU_ITEM MenuItemList;
  ROSMENUINFO MenuInfo;
  BOOL RtoL;
} MENU_OBJECT, *PMENU_OBJECT;

PMENU_OBJECT FASTCALL
IntGetMenuObject(HMENU hMenu);

#define IntLockMenuItems(MenuObj) \
  ExAcquireFastMutex(&(MenuObj)->MenuItemsLock)

#define IntUnLockMenuItems(MenuObj) \
  ExReleaseFastMutex(&(MenuObj)->MenuItemsLock)

#define IntLockProcessMenus(W32Process) \
  ExAcquireFastMutex(&(W32Process)->MenuListLock)

#define IntUnLockProcessMenus(W32Process) \
  ExReleaseFastMutex(&(W32Process)->MenuListLock)

#define IntReleaseMenuObject(MenuObj) \
  ObmDereferenceObject(MenuObj)

BOOL FASTCALL
IntFreeMenuItem(PMENU_OBJECT MenuObject, PMENU_ITEM MenuItem,
    BOOL RemoveFromList, BOOL bRecurse);
    
BOOL FASTCALL
IntRemoveMenuItem(PMENU_OBJECT MenuObject, UINT uPosition, UINT uFlags, 
                   BOOL bRecurse);
    
UINT FASTCALL
IntDeleteMenuItems(PMENU_OBJECT MenuObject, BOOL bRecurse);

BOOL FASTCALL
IntDestroyMenuObject(PMENU_OBJECT MenuObject, BOOL bRecurse, BOOL RemoveFromProcess);

PMENU_OBJECT FASTCALL
IntCreateMenu(PHANDLE Handle, BOOL IsMenuBar);

PMENU_OBJECT FASTCALL
IntCloneMenu(PMENU_OBJECT Source);

BOOL FASTCALL
IntSetMenuFlagRtoL(PMENU_OBJECT MenuObject);

BOOL FASTCALL
IntSetMenuContextHelpId(PMENU_OBJECT MenuObject, DWORD dwContextHelpId);

BOOL FASTCALL
IntGetMenuInfo(PMENU_OBJECT MenuObject, PROSMENUINFO lpmi);

BOOL FASTCALL
IntIsMenu(HMENU hMenu);

BOOL FASTCALL
IntSetMenuInfo(PMENU_OBJECT MenuObject, PROSMENUINFO lpmi);

int FASTCALL
IntGetMenuItemByFlag(PMENU_OBJECT MenuObject, UINT uSearchBy, UINT fFlag, 
                      PMENU_ITEM *MenuItem, PMENU_ITEM *PrevMenuItem);
                   
UINT FASTCALL
IntEnableMenuItem(PMENU_OBJECT MenuObject, UINT uIDEnableItem, UINT uEnable);

DWORD FASTCALL
IntCheckMenuItem(PMENU_OBJECT MenuObject, UINT uIDCheckItem, UINT uCheck);

BOOL FASTCALL
IntSetMenuDefaultItem(PMENU_OBJECT MenuObject, UINT uItem, UINT fByPos);

BOOL FASTCALL
IntSetMenuItemRect(PMENU_OBJECT MenuObject, UINT Item, BOOL fByPos, RECT *rcRect);

BOOL FASTCALL
IntCleanupMenus(struct _EPROCESS *Process, PW32PROCESS Win32Process);

BOOL FASTCALL
IntInsertMenuItem(PMENU_OBJECT MenuObject, UINT uItem, BOOL fByPosition,
                  PROSMENUITEMINFO ItemInfo);


NTSTATUS FASTCALL
InitMenuImpl(VOID);

NTSTATUS FASTCALL
CleanupMenuImpl(VOID);

#endif /* _WIN32K_MENU_H */
