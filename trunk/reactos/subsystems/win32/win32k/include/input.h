#pragma once

#include <internal/kbd.h>

typedef struct _KBL
{
  LIST_ENTRY List;
  DWORD Flags;
  WCHAR Name[KL_NAMELENGTH];    // used w GetKeyboardLayoutName same as wszKLID.
  struct _KBDTABLES* KBTables;  // KBDTABLES in ntoskrnl/include/internal/kbd.h
  HANDLE hModule;
  ULONG RefCount;
  HKL hkl;
  DWORD klid; // Low word - language id. High word - device id.
} KBL, *PKBL;

typedef struct _ATTACHINFO
{
  struct _ATTACHINFO* paiNext;
  PTHREADINFO pti1;
  PTHREADINFO pti2;
} ATTACHINFO, *PATTACHINFO;

extern PATTACHINFO gpai;

#define KBL_UNLOAD 1
#define KBL_PRELOAD 2
#define KBL_RESET 4

NTSTATUS FASTCALL
InitInputImpl(VOID);
NTSTATUS FASTCALL
InitKeyboardImpl(VOID);
PUSER_MESSAGE_QUEUE W32kGetPrimitiveMessageQueue(VOID);
VOID W32kUnregisterPrimitiveMessageQueue(VOID);
PKBL W32kGetDefaultKeyLayout(VOID);
VOID FASTCALL W32kKeyProcessMessage(LPMSG Msg, PKBDTABLES KeyLayout, BYTE Prefix);
BOOL FASTCALL IntBlockInput(PTHREADINFO W32Thread, BOOL BlockIt);
BOOL FASTCALL IntMouseInput(MOUSEINPUT *mi);
BOOL FASTCALL IntKeyboardInput(KEYBDINPUT *ki);

BOOL UserInitDefaultKeyboardLayout();
PKBL UserHklToKbl(HKL hKl);
BOOL FASTCALL UserAttachThreadInput(PTHREADINFO,PTHREADINFO,BOOL);
BOOL FASTCALL IntConnectThreadInput(PTHREADINFO,PTHREADINFO*,PUSER_MESSAGE_QUEUE*);

#define ThreadHasInputAccess(W32Thread) \
  (TRUE)

extern PTHREADINFO ptiRawInput;
