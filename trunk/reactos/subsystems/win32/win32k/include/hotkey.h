#pragma once

#include <include/winsta.h>
#include <include/window.h>

typedef struct _HOT_KEY_ITEM
{
  LIST_ENTRY ListEntry;
  struct _ETHREAD *Thread;
  HWND hWnd;
  int id;
  UINT fsModifiers;
  UINT vk;
} HOT_KEY_ITEM, *PHOT_KEY_ITEM;

NTSTATUS FASTCALL
InitHotkeyImpl(VOID);

//NTSTATUS FASTCALL
//CleanupHotKeys(PWINSTATION_OBJECT WinStaObject);

BOOL FASTCALL
GetHotKey (UINT fsModifiers,
	   UINT vk,
	   struct _ETHREAD **Thread,
	   HWND *hWnd,
	   int *id);

VOID FASTCALL
UnregisterWindowHotKeys(PWND Window);

VOID FASTCALL
UnregisterThreadHotKeys(struct _ETHREAD *Thread);

/* EOF */
