#ifndef _WIN32K_CARET_H
#define _WIN32K_CARET_H

#include <windows.h>
#include <internal/ps.h>

#define IDCARETTIMER (0xffff)

/* a copy of this structure is in lib/user32/include/user32.h */
typedef struct _THRDCARETINFO
{
  HWND hWnd;
  HBITMAP Bitmap;
  POINT Pos;
  SIZE Size;
  BYTE Visible;
  BYTE Showing;
} THRDCARETINFO, *PTHRDCARETINFO;

BOOL FASTCALL
IntDestroyCaret(PW32THREAD Win32Thread);

BOOL FASTCALL
IntSetCaretBlinkTime(UINT uMSeconds);

BOOL FASTCALL
IntSetCaretPos(int X, int Y);

BOOL FASTCALL
IntSwitchCaretShowing(PVOID Info);

VOID FASTCALL
IntDrawCaret(HWND hWnd);

#endif /* _WIN32K_CARET_H */

/* EOF */
