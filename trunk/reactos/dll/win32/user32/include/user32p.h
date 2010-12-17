/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS System Libraries
 * FILE:            lib/user32/include/user32p.h
 * PURPOSE:         Win32 User Library Private Headers
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#pragma once

/* Private User32 Headers */
#include "accel.h"
#include "cursor.h"
#include "controls.h"
#include "draw.h"
#include "dde_private.h"
#include "menu.h"
#include "message.h"
#include "regcontrol.h"
#include "resource.h"
#include "scroll.h"
#include "window.h"
#include "winpos.h"
#include "winsta.h"
#include "ntwrapper.h"

/* One/Two Param Functions */
#define NtUserMsqSetWakeMask(dwWaitMask) \
  (HANDLE)NtUserCallOneParam(dwWaitMask, ONEPARAM_ROUTINE_GETINPUTEVENT)

#define NtUserMsqClearWakeMask() \
  NtUserCallNoParam(NOPARAM_ROUTINE_MSQCLEARWAKEMASK)

#define NtUserValidateRgn(hWnd, hRgn) \
  (BOOL)NtUserCallTwoParam((DWORD_PTR)hWnd, (DWORD_PTR)hRgn, TWOPARAM_ROUTINE_VALIDATERGN)

#define NtUserSetWindowContextHelpId(hWnd, dwContextHelpId) \
  (BOOL)NtUserCallHwndParam(hWnd, dwContextHelpId, HWNDPARAM_ROUTINE_SETWNDCONTEXTHLPID)

#define NtUserSetCaretPos(X, Y) \
  (BOOL)NtUserCallTwoParam((DWORD_PTR)X, (DWORD_PTR)Y, TWOPARAM_ROUTINE_SETCARETPOS)

#define NtUserSetGUIThreadHandle(field, hwnd) \
  (BOOL)NtUserCallTwoParam((DWORD_PTR)field, (DWORD_PTR)hwnd, TWOPARAM_ROUTINE_SETGUITHRDHANDLE)

#define NtUserSetMenuBarHeight(menu, height) \
  (BOOL)NtUserCallTwoParam((DWORD_PTR)menu, (DWORD_PTR)height, TWOPARAM_ROUTINE_SETMENUBARHEIGHT)

#define NtUserRegisterLogonProcess(hproc, x) \
  (BOOL)NtUserCallTwoParam((DWORD_PTR)hproc, (DWORD_PTR)x, TWOPARAM_ROUTINE_REGISTERLOGONPROC)

#define NtUserGetSysColorBrushes(HBrushes, count) \
  (BOOL)NtUserCallTwoParam((DWORD_PTR)(HBrushes), (DWORD_PTR)(count), TWOPARAM_ROUTINE_GETSYSCOLORBRUSHES)

#define NtUserGetSysColors(ColorRefs, count) \
  (BOOL)NtUserCallTwoParam((DWORD_PTR)(ColorRefs), (DWORD_PTR)(count), TWOPARAM_ROUTINE_GETSYSCOLORS)

#define NtUserSetCaretBlinkTime(uMSeconds) \
  (BOOL)NtUserCallOneParam((DWORD_PTR)uMSeconds, ONEPARAM_ROUTINE_SETCARETBLINKTIME)

/*
#define NtUserEnumClipboardFormats(format) \
  (UINT)NtUserCallOneParam(format, ONEPARAM_ROUTINE_ENUMCLIPBOARDFORMATS)
*/

#define NtUserWindowFromDC(hDC) \
  (HWND)NtUserCallOneParam((DWORD_PTR)hDC, ONEPARAM_ROUTINE_WINDOWFROMDC)

#define NtUserSwitchCaretShowing(CaretInfo) \
  (BOOL)NtUserCallOneParam((DWORD_PTR)CaretInfo, ONEPARAM_ROUTINE_SWITCHCARETSHOWING)

#define NtUserSwapMouseButton(fSwap) \
  (BOOL)NtUserCallOneParam((DWORD_PTR)fSwap, ONEPARAM_ROUTINE_SWAPMOUSEBUTTON)

#define NtUserSetMessageExtraInfo(lParam) \
  (LPARAM)NtUserCallOneParam((DWORD_PTR)lParam, ONEPARAM_ROUTINE_SETMESSAGEEXTRAINFO)

#define NtUserGetWindowContextHelpId(hwnd) \
  NtUserCallOneParam((DWORD_PTR)hwnd, ONEPARAM_ROUTINE_GETWNDCONTEXTHLPID)

#define NtUserGetCursorPos(lpPoint) \
  (BOOL)NtUserCallOneParam((DWORD_PTR)lpPoint, ONEPARAM_ROUTINE_GETCURSORPOSITION)

#define NtUserEnableProcessWindowGhosting(bEnable) \
  NtUserCallOneParam((DWORD_PTR)bEnable, ONEPARAM_ROUTINE_ENABLEPROCWNDGHSTING)

#define NtUserShowCursor(bShow) \
  NtUserCallOneParam((DWORD_PTR)bShow, ONEPARAM_ROUTINE_SHOWCURSOR)

#define NtUserGetDesktopMapping(Ptr) \
  (PVOID)NtUserCallOneParam((DWORD_PTR)Ptr, ONEPARAM_ROUTINE_GETDESKTOPMAPPING)

#define NtUserSetCursorPos(x, y) \
  (BOOL)NtUserCallTwoParam((DWORD)x, (DWORD)y, TWOPARAM_ROUTINE_SETCURSORPOS)

#define ShowCaret(hwnd) \
  NtUserShowCaret(hwnd)

#define HideCaret(hwnd) \
  NtUserHideCaret(hwnd)

/* Internal Thread Data */
extern HINSTANCE User32Instance;
extern HINSTANCE hImmInstance;

/* Critical Section*/
extern RTL_CRITICAL_SECTION User32Crit;

typedef struct _USER32_TRACKINGLIST {
    TRACKMOUSEEVENT tme;
    POINT pos; /* center of hover rectangle */
    UINT_PTR timer;
} USER32_TRACKINGLIST,*PUSER32_TRACKINGLIST;

typedef struct _USER32_THREAD_DATA
{
    USER32_TRACKINGLIST tracking_info; /* TrackMouseEvent stuff */
} USER32_THREAD_DATA, *PUSER32_THREAD_DATA;

PUSER32_THREAD_DATA User32GetThreadData();

/* FIXME: Belongs to some header. */
BOOL WINAPI GdiDllInitialize(HANDLE, DWORD, LPVOID);
void InitStockObjects(void);
VOID DeleteFrameBrushes(VOID);

/* message spy definitions */
#define SPY_DISPATCHMESSAGE       0x0101
#define SPY_SENDMESSAGE           0x0103
#define SPY_DEFWNDPROC            0x0105

#define SPY_RESULT_OK             0x0001
#define SPY_RESULT_INVALIDHWND    0x0003
#define SPY_RESULT_DEFWND         0x0005


extern const char *SPY_GetMsgName(UINT msg, HWND hWnd);
extern const char *SPY_GetVKeyName(WPARAM wParam);
extern void SPY_EnterMessage(INT iFlag, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
extern void SPY_ExitMessage(INT iFlag, HWND hwnd, UINT msg,
                            LRESULT lReturn, WPARAM wParam, LPARAM lParam);
extern int SPY_Init(void);


/* Validate window handle types */
#define VALIDATE_TYPE_FREE     0
#define VALIDATE_TYPE_WIN      1
#define VALIDATE_TYPE_MENU     2
#define VALIDATE_TYPE_CURSOR   3
#define VALIDATE_TYPE_MWPOS    4
#define VALIDATE_TYPE_HOOK     5
#define VALIDATE_TYPE_CALLPROC 7
#define VALIDATE_TYPE_ACCEL    8
#define VALIDATE_TYPE_MONITOR  12
#define VALIDATE_TYPE_EVENT    15
#define VALIDATE_TYPE_TIMER    16

#define FIRST_USER_HANDLE 0x0020  /* first possible value for low word of user handle */
#define LAST_USER_HANDLE  0xffef  /* last possible value for low word of user handle */
#define NB_USER_HANDLES  ((LAST_USER_HANDLE - FIRST_USER_HANDLE + 1) >> 1)
#define USER_HANDLE_TO_INDEX(hwnd) ((LOWORD(hwnd) - FIRST_USER_HANDLE) >> 1)

#define USER_HEADER_TO_BODY(ObjectHeader) \
  ((PVOID)(((PUSER_OBJECT_HEADER)ObjectHeader) + 1))

#define USER_BODY_TO_HEADER(ObjectBody) \
  ((PUSER_OBJECT_HEADER)(((PUSER_OBJECT_HEADER)ObjectBody) - 1))

#define HANDLEENTRY_INDESTROY 1

typedef struct _USER_HANDLE_ENTRY
{
    void          *ptr;          /* pointer to object */
    union
    {
        PVOID pi;
        PTHREADINFO pti;          // pointer to Win32ThreadInfo
        PPROCESSINFO ppi;         // pointer to W32ProcessInfo
    };
    unsigned char  type;         /* object type (0 if free) */
    unsigned char  flags;
    unsigned short generation;   /* generation counter */
} USER_HANDLE_ENTRY, * PUSER_HANDLE_ENTRY;

typedef struct _USER_HANDLE_TABLE
{
   PUSER_HANDLE_ENTRY handles;
   PUSER_HANDLE_ENTRY freelist;
   int nb_handles;
   int allocated_handles;
} USER_HANDLE_TABLE, * PUSER_HANDLE_TABLE;

extern PUSER_HANDLE_TABLE gHandleTable;
extern PUSER_HANDLE_ENTRY gHandleEntries;

PUSER_HANDLE_ENTRY FASTCALL GetUser32Handle(HANDLE);
PVOID FASTCALL ValidateHandle(HANDLE, UINT);

#define WIN_GetFullHandle(h) ((HWND)(h))

#ifndef __ms_va_list
# if defined(__x86_64__) && defined (__GNUC__)
#  define __ms_va_list __builtin_ms_va_list
#  define __ms_va_start(list,arg) __builtin_ms_va_start(list,arg)
#  define __ms_va_end(list) __builtin_ms_va_end(list)
# else
#  define __ms_va_list va_list
#  define __ms_va_start(list,arg) va_start(list,arg)
#  define __ms_va_end(list) va_end(list)
# endif
#endif

#ifdef _MSC_VER
#ifndef inline
#define inline __inline
#endif
#endif

/* Hack away wine hack */
#define __TRY if(1)
#define __EXCEPT_PAGE_FAULT else
#define __ENDTRY

/* EOF */
