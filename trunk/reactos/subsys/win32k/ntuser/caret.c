/* $Id: caret.c,v 1.8 2003/11/23 11:39:48 navaraf Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Caret functions
 * FILE:             subsys/win32k/ntuser/caret.c
 * PROGRAMER:        Thomas Weidenmueller (w3seek@users.sourceforge.net)
 * REVISION HISTORY:
 *       10/15/2003  Created
 */

#include <win32k/win32k.h>
#include <ddk/ntddk.h>
#include <internal/safe.h>
#include <include/error.h>
#include <include/window.h>
#include <include/caret.h>
#include <include/timer.h>
#include <include/callback.h>
#include <rosrtl/string.h>

#define NDEBUG
#include <debug.h>

#define MIN_CARETBLINKRATE 100
#define MAX_CARETBLINKRATE 10000
#define DEFAULT_CARETBLINKRATE 530
#define CARET_REGKEY L"\\Registry\\User\\.Default\\Control Panel\\Desktop"
#define CARET_VALUENAME L"CursorBlinkRate"

BOOL FASTCALL
IntHideCaret(PTHRDCARETINFO CaretInfo)
{
  if(CaretInfo->hWnd && CaretInfo->Visible && CaretInfo->Showing)
  {
    IntCallWindowProc(NULL, CaretInfo->hWnd, WM_SYSTIMER, IDCARETTIMER, 0);
    CaretInfo->Showing = 0;
    return TRUE;
  }
  return FALSE;
}

BOOL FASTCALL
IntDestroyCaret(PW32THREAD Win32Thread)
{
  PTHRDCARETINFO CaretInfo = ThrdCaretInfo(Win32Thread);
  IntHideCaret(CaretInfo);
  RtlZeroMemory(CaretInfo, sizeof(THRDCARETINFO));
  return TRUE;
}

BOOL FASTCALL
IntSetCaretBlinkTime(UINT uMSeconds)
{
  /* Don't save the new value to the registry! */
  NTSTATUS Status;
  PWINSTATION_OBJECT WinStaObject;
  
  Status = IntValidateWindowStationHandle(PROCESS_WINDOW_STATION(),
	                                  KernelMode,
				          0,
				          &WinStaObject);
  if(!NT_SUCCESS(Status))
  {
    SetLastNtError(Status);
    return FALSE;
  }
  
  /* windows doesn't do this check */
  if((uMSeconds < MIN_CARETBLINKRATE) || (uMSeconds > MAX_CARETBLINKRATE))
  {
    SetLastWin32Error(ERROR_INVALID_PARAMETER);
    ObDereferenceObject(WinStaObject);
    return FALSE;
  }
  
  WinStaObject->CaretBlinkRate = uMSeconds;
  
  ObDereferenceObject(WinStaObject);
  return TRUE;
}

#define CARET_VALUE_BUFFER_SIZE 32
UINT FASTCALL
IntQueryCaretBlinkRate(VOID)
{
  UNICODE_STRING KeyName, ValueName;
  NTSTATUS Status;
  HANDLE KeyHandle = NULL;
  OBJECT_ATTRIBUTES KeyAttributes;
  PKEY_VALUE_PARTIAL_INFORMATION KeyValuePartialInfo;
  ULONG Length = 0;
  ULONG ResLength = 0;
  ULONG Val = 0;
  
  RtlRosInitUnicodeStringFromLiteral(&KeyName, CARET_REGKEY);
  RtlRosInitUnicodeStringFromLiteral(&ValueName, CARET_VALUENAME);
  
  InitializeObjectAttributes(&KeyAttributes, &KeyName, OBJ_CASE_INSENSITIVE,
                             NULL, NULL);
  
  Status = ZwOpenKey(&KeyHandle, KEY_READ, &KeyAttributes);
  if(!NT_SUCCESS(Status))
  {
    return 0;
  }
  
  Status = ZwQueryValueKey(KeyHandle, &ValueName, KeyValuePartialInformation,
                           0, 0, &ResLength);
  if((Status != STATUS_BUFFER_TOO_SMALL))
  {
    NtClose(KeyHandle);
    return 0;
  }
  
  ResLength += sizeof(KEY_VALUE_PARTIAL_INFORMATION);
  KeyValuePartialInfo = ExAllocatePool(PagedPool, ResLength);
  Length = ResLength;
  
  if(!KeyValuePartialInfo)
  {
    NtClose(KeyHandle);
    return 0;
  }
  
  Status = ZwQueryValueKey(KeyHandle, &ValueName, KeyValuePartialInformation,
                           (PVOID)KeyValuePartialInfo, Length, &ResLength);
  if(!NT_SUCCESS(Status) || (KeyValuePartialInfo->Type != REG_SZ))
  {
    NtClose(KeyHandle);
    ExFreePool(KeyValuePartialInfo);
    return 0;
  }
  
  ValueName.Length = KeyValuePartialInfo->DataLength;
  ValueName.MaximumLength = KeyValuePartialInfo->DataLength;
  ValueName.Buffer = (PWSTR)KeyValuePartialInfo->Data;
  
  Status = RtlUnicodeStringToInteger(&ValueName, 0, &Val);
  if(!NT_SUCCESS(Status))
  {
    Val = 0;
  }
  
  ExFreePool(KeyValuePartialInfo);
  NtClose(KeyHandle);
  
  return (UINT)Val;
}

UINT FASTCALL
IntGetCaretBlinkTime(VOID)
{
  NTSTATUS Status;
  PWINSTATION_OBJECT WinStaObject;
  UINT Ret;
  
  Status = IntValidateWindowStationHandle(PROCESS_WINDOW_STATION(),
	                                  KernelMode,
				          0,
				          &WinStaObject);
  if(!NT_SUCCESS(Status))
  {
    SetLastNtError(Status);
    return 0;
  }
  
  Ret = WinStaObject->CaretBlinkRate;
  if(!Ret)
  {
    /* load it from the registry the first call only! */
    Ret = WinStaObject->CaretBlinkRate = IntQueryCaretBlinkRate();
  }
  
  /* windows doesn't do this check */
  if((Ret < MIN_CARETBLINKRATE) || (Ret > MAX_CARETBLINKRATE))
  {
    Ret = DEFAULT_CARETBLINKRATE;
  }
  
  ObDereferenceObject(WinStaObject);
  return Ret;
}

BOOL FASTCALL
IntSetCaretPos(int X, int Y)
{
  PTHRDCARETINFO CaretInfo = ThrdCaretInfo(PsGetCurrentThread()->Win32Thread);
  
  if(CaretInfo->hWnd)
  {
    if(CaretInfo->Pos.x != X || CaretInfo->Pos.y != Y)
    {
      IntHideCaret(CaretInfo);
      CaretInfo->Showing = 0;
      CaretInfo->Pos.x = X;
      CaretInfo->Pos.y = Y;
      IntCallWindowProc(NULL, CaretInfo->hWnd, WM_SYSTIMER, IDCARETTIMER, 0);
      IntSetTimer(CaretInfo->hWnd, IDCARETTIMER, IntGetCaretBlinkTime(), NULL, TRUE);
    }
    return TRUE;
  }

  return FALSE;
}

BOOL FASTCALL
IntSwitchCaretShowing(PVOID Info)
{
  PTHRDCARETINFO CaretInfo = ThrdCaretInfo(PsGetCurrentThread()->Win32Thread);
  
  if(CaretInfo->hWnd)
  {
    CaretInfo->Showing = (CaretInfo->Showing ? 0 : 1);
    MmCopyToCaller(Info, CaretInfo, sizeof(THRDCARETINFO));
    return TRUE;
  }
  
  return FALSE;
}

VOID FASTCALL
IntDrawCaret(HWND hWnd)
{
  PTHRDCARETINFO CaretInfo = ThrdCaretInfo(PsGetCurrentThread()->Win32Thread);
  
  if(CaretInfo->hWnd && CaretInfo->Visible && CaretInfo->Showing)
  {
    IntCallWindowProc(NULL, CaretInfo->hWnd, WM_SYSTIMER, IDCARETTIMER, 0);
    CaretInfo->Showing = 1;
  }
}



BOOL
STDCALL
NtUserCreateCaret(
  HWND hWnd,
  HBITMAP hBitmap,
  int nWidth,
  int nHeight)
{
  PWINDOW_OBJECT WindowObject;
  PTHRDCARETINFO CaretInfo;
  
  WindowObject = IntGetWindowObject(hWnd);
  if(!WindowObject)
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return FALSE;
  }
  
  if(WindowObject->OwnerThread != PsGetCurrentThread())
  {
    IntReleaseWindowObject(WindowObject);
    SetLastWin32Error(ERROR_ACCESS_DENIED);
    return FALSE;
  }
  
  IntRemoveTimer(hWnd, IDCARETTIMER, PsGetCurrentThreadId(), TRUE);
  
  CaretInfo = ThrdCaretInfo(WindowObject->OwnerThread->Win32Thread);
  
  IntHideCaret(CaretInfo);
  
  CaretInfo->hWnd = hWnd;
  if(hBitmap)
  {
    CaretInfo->Bitmap = hBitmap;
    CaretInfo->Size.cx = CaretInfo->Size.cy = 0;
  }
  else
  {
    CaretInfo->Bitmap = (HBITMAP)0;
    CaretInfo->Size.cx = nWidth;
    CaretInfo->Size.cy = nHeight;
  }
  CaretInfo->Pos.x = CaretInfo->Pos.y = 0;
  CaretInfo->Visible = 0;
  CaretInfo->Showing = 0;
  
  IntReleaseWindowObject(WindowObject);  
  return TRUE;
}

UINT
STDCALL
NtUserGetCaretBlinkTime(VOID)
{
  return IntGetCaretBlinkTime();
}

BOOL
STDCALL
NtUserGetCaretPos(
  LPPOINT lpPoint)
{
  PTHRDCARETINFO CaretInfo;
  NTSTATUS Status;
  
  CaretInfo = ThrdCaretInfo(PsGetCurrentThread()->Win32Thread);
  
  Status = MmCopyToCaller(lpPoint, &(CaretInfo->Pos), sizeof(POINT));
  if(!NT_SUCCESS(Status))
  {
    SetLastNtError(Status);
    return FALSE;
  }
  
  return TRUE;
}

BOOL
STDCALL
NtUserHideCaret(
  HWND hWnd)
{
  PWINDOW_OBJECT WindowObject;
  PTHRDCARETINFO CaretInfo;
  
  WindowObject = IntGetWindowObject(hWnd);
  if(!WindowObject)
  {
    SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
    return FALSE;
  }
  
  if(WindowObject->OwnerThread != PsGetCurrentThread())
  {
    IntReleaseWindowObject(WindowObject);
    SetLastWin32Error(ERROR_ACCESS_DENIED);
    return FALSE;
  }
  
  CaretInfo = ThrdCaretInfo(PsGetCurrentThread()->Win32Thread);
  
  if(CaretInfo->hWnd != hWnd)
  {
    IntReleaseWindowObject(WindowObject);
    SetLastWin32Error(ERROR_ACCESS_DENIED);
    return FALSE;
  }
  
  if(CaretInfo->Visible)
  {
    IntRemoveTimer(hWnd, IDCARETTIMER, PsGetCurrentThreadId(), TRUE);
    
    IntHideCaret(CaretInfo);
    CaretInfo->Visible = 0;
    CaretInfo->Showing = 0;
  }
  
  IntReleaseWindowObject(WindowObject);
  return TRUE;
}

BOOL
STDCALL
NtUserShowCaret(
  HWND hWnd)
{
  PWINDOW_OBJECT WindowObject;
  PTHRDCARETINFO CaretInfo;
  
  WindowObject = IntGetWindowObject(hWnd);
  if(!WindowObject)
  {
    SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
    return FALSE;
  }
  
  if(WindowObject->OwnerThread != PsGetCurrentThread())
  {
    IntReleaseWindowObject(WindowObject);
    SetLastWin32Error(ERROR_ACCESS_DENIED);
    return FALSE;
  }
  
  CaretInfo = ThrdCaretInfo(PsGetCurrentThread()->Win32Thread);
  
  if(CaretInfo->hWnd != hWnd)
  {
    IntReleaseWindowObject(WindowObject);
    SetLastWin32Error(ERROR_ACCESS_DENIED);
    return FALSE;
  }
  
  if(!CaretInfo->Visible)
  {
    CaretInfo->Visible = 1;
    if(!CaretInfo->Showing)
    {
      IntCallWindowProc(NULL, CaretInfo->hWnd, WM_SYSTIMER, IDCARETTIMER, 0);
    }
    IntSetTimer(hWnd, IDCARETTIMER, IntGetCaretBlinkTime(), NULL, TRUE);
  }
  
  IntReleaseWindowObject(WindowObject);
  return TRUE;
}
