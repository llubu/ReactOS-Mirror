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
/* $Id: input.c,v 1.38 2004/09/28 15:02:30 weiden Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Window classes
 * FILE:             subsys/win32k/ntuser/class.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISION HISTORY:
 *       06-06-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#include <w32k.h>
#include <rosrtl/string.h>

/* GLOBALS *******************************************************************/

static HANDLE MouseDeviceHandle;
static HANDLE MouseThreadHandle;
static CLIENT_ID MouseThreadId;
static HANDLE KeyboardThreadHandle;
static CLIENT_ID KeyboardThreadId;
static HANDLE KeyboardDeviceHandle;
static KEVENT InputThreadsStart;
static BOOLEAN InputThreadsRunning = FALSE;
PUSER_MESSAGE_QUEUE pmPrimitiveMessageQueue = 0;

/* FUNCTIONS *****************************************************************/

#define ClearMouseInput(mi) \
  mi.dx = 0; \
  mi.dy = 0; \
  mi.mouseData = 0; \
  mi.dwFlags = 0;

#define SendMouseEvent(mi) \
  if(mi.dx != 0 || mi.dy != 0) \
    mi.dwFlags |= MOUSEEVENTF_MOVE; \
  if(mi.dwFlags) \
    IntMouseInput(&mi); \
  ClearMouseInput(mi);

VOID FASTCALL
ProcessMouseInputData(PMOUSE_INPUT_DATA Data, ULONG InputCount)
{
  PMOUSE_INPUT_DATA mid;
  MOUSEINPUT mi;
  ULONG i;

  ClearMouseInput(mi);
  mi.time = 0;
  mi.dwExtraInfo = 0;
  for(i = 0; i < InputCount; i++)
  {
    mid = (Data + i);
    mi.dx += mid->LastX;
    mi.dy += mid->LastY;

    if(mid->ButtonFlags)
    {
      if(mid->ButtonFlags & MOUSE_LEFT_BUTTON_DOWN)
      {
        mi.dwFlags |= MOUSEEVENTF_LEFTDOWN;
        SendMouseEvent(mi);
      }
      if(mid->ButtonFlags & MOUSE_LEFT_BUTTON_UP)
      {
        mi.dwFlags |= MOUSEEVENTF_LEFTUP;
        SendMouseEvent(mi);
      }
      if(mid->ButtonFlags & MOUSE_MIDDLE_BUTTON_DOWN)
      {
        mi.dwFlags |= MOUSEEVENTF_MIDDLEDOWN;
        SendMouseEvent(mi);
      }
      if(mid->ButtonFlags & MOUSE_MIDDLE_BUTTON_UP)
      {
        mi.dwFlags |= MOUSEEVENTF_MIDDLEUP;
        SendMouseEvent(mi);
      }
      if(mid->ButtonFlags & MOUSE_RIGHT_BUTTON_DOWN)
      {
        mi.dwFlags |= MOUSEEVENTF_RIGHTDOWN;
        SendMouseEvent(mi);
      }
      if(mid->ButtonFlags & MOUSE_RIGHT_BUTTON_UP)
      {
        mi.dwFlags |= MOUSEEVENTF_RIGHTUP;
        SendMouseEvent(mi);
      }
      if(mid->ButtonFlags & MOUSE_BUTTON_4_DOWN)
      {
        mi.mouseData |= XBUTTON1;
        mi.dwFlags |= MOUSEEVENTF_XDOWN;
        SendMouseEvent(mi);
      }
      if(mid->ButtonFlags & MOUSE_BUTTON_4_UP)
      {
        mi.mouseData |= XBUTTON1;
        mi.dwFlags |= MOUSEEVENTF_XUP;
        SendMouseEvent(mi);
      }
      if(mid->ButtonFlags & MOUSE_BUTTON_5_DOWN)
      {
        mi.mouseData |= XBUTTON2;
        mi.dwFlags |= MOUSEEVENTF_XDOWN;
        SendMouseEvent(mi);
      }
      if(mid->ButtonFlags & MOUSE_BUTTON_5_UP)
      {
        mi.mouseData |= XBUTTON2;
        mi.dwFlags |= MOUSEEVENTF_XUP;
        SendMouseEvent(mi);
      }
      if(mid->ButtonFlags & MOUSE_WHEEL)
      {
        mi.mouseData = mid->ButtonData;
        mi.dwFlags |= MOUSEEVENTF_WHEEL;
        SendMouseEvent(mi);
      }
    }
  }

  SendMouseEvent(mi);
}

VOID STDCALL
MouseThreadMain(PVOID StartContext)
{
  UNICODE_STRING MouseDeviceName;
  OBJECT_ATTRIBUTES MouseObjectAttributes;
  IO_STATUS_BLOCK Iosb;
  NTSTATUS Status;
  
  RtlRosInitUnicodeStringFromLiteral(&MouseDeviceName, L"\\??\\Mouse"); /* FIXME - does win use the same? */
  InitializeObjectAttributes(&MouseObjectAttributes,
                             &MouseDeviceName,
                             0,
                             NULL,
                             NULL);
  Status = NtOpenFile(&MouseDeviceHandle,
                      FILE_ALL_ACCESS,
                      &MouseObjectAttributes,
                      &Iosb,
                      0,
                      FILE_SYNCHRONOUS_IO_ALERT);
  if(!NT_SUCCESS(Status))
  {
    DPRINT1("Win32K: Failed to open mouse.\n");
    return; //(Status);
  }
  
  for(;;)
  {
    /*
     * Wait to start input.
     */
    DPRINT("Mouse Input Thread Waiting for start event\n");
    Status = KeWaitForSingleObject(&InputThreadsStart,
                                   0,
                                   KernelMode,
                                   TRUE,
                                   NULL);
    DPRINT("Mouse Input Thread Starting...\n");
    
    /*
     * Receive and process keyboard input.
     */
    while(InputThreadsRunning)
    {
      MOUSE_INPUT_DATA MouseInput;
      Status = NtReadFile(MouseDeviceHandle,
                          NULL,
                          NULL,
                          NULL,
                          &Iosb,
                          &MouseInput,
                          sizeof(MOUSE_INPUT_DATA),
                          NULL,
                          NULL);
      if(Status == STATUS_ALERTED && !InputThreadsRunning)
      {
        break;
      }
      if(Status == STATUS_PENDING)
      {
        NtWaitForSingleObject(MouseDeviceHandle, FALSE, NULL);
        Status = Iosb.Status;
      }
      if(!NT_SUCCESS(Status))
      {
        DPRINT1("Win32K: Failed to read from mouse.\n");
        return; //(Status);
      }
      DPRINT("MouseEvent\n");
      
      ProcessMouseInputData(&MouseInput, Iosb.Information / sizeof(MOUSE_INPUT_DATA));
    }
    DPRINT("Mouse Input Thread Stopped...\n");
  }
}

STATIC VOID STDCALL
KeyboardThreadMain(PVOID StartContext)
{
  UNICODE_STRING KeyboardDeviceName;
  OBJECT_ATTRIBUTES KeyboardObjectAttributes;
  IO_STATUS_BLOCK Iosb;
  NTSTATUS Status;
  MSG msg;
  PUSER_MESSAGE_QUEUE FocusQueue;
  struct _ETHREAD *FocusThread;
  
  RtlRosInitUnicodeStringFromLiteral(&KeyboardDeviceName, L"\\??\\Keyboard");
  InitializeObjectAttributes(&KeyboardObjectAttributes,
			     &KeyboardDeviceName,
			     0,
			     NULL,
			     NULL);
  Status = NtOpenFile(&KeyboardDeviceHandle,
		      FILE_ALL_ACCESS,
		      &KeyboardObjectAttributes,
		      &Iosb,
		      0,
		      FILE_SYNCHRONOUS_IO_ALERT);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("Win32K: Failed to open keyboard.\n");
      return; //(Status);
    }

  for (;;)
    {
      /*
       * Wait to start input.
       */
      DPRINT( "Keyboard Input Thread Waiting for start event\n" );
      Status = KeWaitForSingleObject(&InputThreadsStart,
				     0,
				     KernelMode,
				     TRUE,
				     NULL);
      DPRINT( "Keyboard Input Thread Starting...\n" );

      /*
       * Receive and process keyboard input.
       */
      while (InputThreadsRunning)
	{
	  KEY_EVENT_RECORD KeyEvent;
	  LPARAM lParam = 0;
	  UINT fsModifiers;
	  struct _ETHREAD *Thread;
	  HWND hWnd;
	  int id;

	  Status = NtReadFile (KeyboardDeviceHandle,
			       NULL,
			       NULL,
			       NULL,
			       &Iosb,
			       &KeyEvent,
			       sizeof(KEY_EVENT_RECORD),
			       NULL,
			       NULL);
	  DPRINT( "KeyRaw: %s %04x\n",
		 KeyEvent.bKeyDown ? "down" : "up",
		 KeyEvent.wVirtualScanCode );

	  if (Status == STATUS_ALERTED && !InputThreadsRunning)
	    {
	      break;
	    }
	  if (!NT_SUCCESS(Status))
	    {
	      DPRINT1("Win32K: Failed to read from keyboard.\n");
	      return; //(Status);
	    }

	  DPRINT( "Key: %s\n", KeyEvent.bKeyDown ? "down" : "up" );

	  fsModifiers = 0;
	  if (KeyEvent.dwControlKeyState & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED))
	    fsModifiers |= MOD_ALT;

	  if (KeyEvent.dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))
	    fsModifiers |= MOD_CONTROL;

	  if (KeyEvent.dwControlKeyState & SHIFT_PRESSED)
	    fsModifiers |= MOD_SHIFT;

	  /* FIXME: Support MOD_WIN */

	  lParam = KeyEvent.wRepeatCount | 
	    ((KeyEvent.wVirtualScanCode << 16) & 0x00FF0000) | 0x40000000;
	  
	  /* Bit 24 indicates if this is an extended key */
	  if (KeyEvent.dwControlKeyState & ENHANCED_KEY)
	    {
	      lParam |= (1 << 24);
	    }
	  
	  if (fsModifiers & MOD_ALT)
	    {
	      /* Context mode. 1 if ALT if pressed while the key is pressed */
	      lParam |= (1 << 29);
	    }
	  
	  if (GetHotKey(InputWindowStation,
			fsModifiers,
			KeyEvent.wVirtualKeyCode,
			&Thread,
			&hWnd,
			&id))
	    {
	      if (KeyEvent.bKeyDown)
		{
		  DPRINT("Hot key pressed (hWnd %lx, id %d)\n", hWnd, id);
		  MsqPostHotKeyMessage (Thread,
					hWnd,
					(WPARAM)id,
					MAKELPARAM((WORD)fsModifiers, 
						   (WORD)msg.wParam));
		}
	      continue;	
	    }

	  /* Find the target thread whose locale is in effect */
	  if (!IntGetScreenDC())
	    {
	      FocusQueue = W32kGetPrimitiveMessageQueue();
	    }
	  else
	    {
      	      FocusQueue = IntGetFocusMessageQueue();
	    }

	  if (!FocusQueue) continue;

	  if(KeyEvent.bKeyDown && (fsModifiers & MOD_ALT)) 
	    msg.message = WM_SYSKEYDOWN;
	  else if(KeyEvent.bKeyDown)
	    msg.message = WM_KEYDOWN;
	  else if(fsModifiers & MOD_ALT)
	    msg.message = WM_SYSKEYUP;
	  else
	    msg.message = WM_KEYUP;

	  msg.wParam = KeyEvent.wVirtualKeyCode;
	  msg.lParam = lParam;
	  msg.hwnd = FocusQueue->FocusWindow;

	  FocusThread = FocusQueue->Thread;

	  if (FocusThread && FocusThread->Tcb.Win32Thread &&
	      FocusThread->Tcb.Win32Thread->KeyboardLayout)
	    {
	      W32kKeyProcessMessage(&msg,
				    FocusThread->Tcb.Win32Thread->KeyboardLayout);
	    } 
	  else
	    continue;
	  
	  /*
	   * Post a keyboard message.
	   */
	  MsqPostKeyboardMessage(msg.message,msg.wParam,msg.lParam);
	}
      DPRINT( "KeyboardInput Thread Stopped...\n" );
    }
}


NTSTATUS STDCALL
NtUserAcquireOrReleaseInputOwnership(BOOLEAN Release)
{
  if (Release && InputThreadsRunning && !pmPrimitiveMessageQueue)
    {
      DPRINT( "Releasing input: PM = %08x\n", pmPrimitiveMessageQueue );
      KeClearEvent(&InputThreadsStart);
      InputThreadsRunning = FALSE;
      
      NtAlertThread(KeyboardThreadHandle);
    }
  else if (!Release && !InputThreadsRunning)
    {
      InputThreadsRunning = TRUE;
      KeSetEvent(&InputThreadsStart, IO_NO_INCREMENT, FALSE);
    }

  return(STATUS_SUCCESS);
}

NTSTATUS FASTCALL
InitInputImpl(VOID)
{
  NTSTATUS Status;

  KeInitializeEvent(&InputThreadsStart, NotificationEvent, FALSE);

  Status = PsCreateSystemThread(&KeyboardThreadHandle,
				THREAD_ALL_ACCESS,
				NULL,
				NULL,
				&KeyboardThreadId,
				KeyboardThreadMain,
				NULL);
  if (!NT_SUCCESS(Status))
  {
    DPRINT1("Win32K: Failed to create keyboard thread.\n");
  }

  /* Initialize the default keyboard layout */
  (VOID)W32kGetDefaultKeyLayout();
  

  Status = PsCreateSystemThread(&MouseThreadHandle,
				THREAD_ALL_ACCESS,
				NULL,
				NULL,
				&MouseThreadId,
				MouseThreadMain,
				NULL);
  if (!NT_SUCCESS(Status))
  {
    DPRINT1("Win32K: Failed to create mouse thread.\n");
  }
  
  return STATUS_SUCCESS;
}

NTSTATUS FASTCALL
CleanupInputImp(VOID)
{
  return(STATUS_SUCCESS);
}

BOOL
STDCALL
NtUserDragDetect(
  HWND hWnd,
  LONG x,
  LONG y)
{
  UNIMPLEMENTED
  return 0;
}

BOOL FASTCALL
IntBlockInput(PW32THREAD W32Thread, BOOL BlockIt)
{
  PW32THREAD OldBlock;
  ASSERT(W32Thread);
  
  if(!W32Thread->Desktop || (W32Thread->IsExiting && BlockIt))
  {
    /*
     * fail blocking if exiting the thread
     */
    
    return FALSE;
  }
  
  /*
   * FIXME - check access rights of the window station
   *         e.g. services running in the service window station cannot block input
   */
  if(!ThreadHasInputAccess(W32Thread) ||
     !IntIsActiveDesktop(W32Thread->Desktop))
  {
    SetLastWin32Error(ERROR_ACCESS_DENIED);
    return FALSE;
  }
  
  ASSERT(W32Thread->Desktop);
  OldBlock = W32Thread->Desktop->BlockInputThread;
  if(OldBlock)
  {
    if(OldBlock != W32Thread)
    {
      SetLastWin32Error(ERROR_ACCESS_DENIED);
      return FALSE;
    }
    W32Thread->Desktop->BlockInputThread = (BlockIt ? W32Thread : NULL);
    return OldBlock == NULL;
  }
  
  W32Thread->Desktop->BlockInputThread = (BlockIt ? W32Thread : NULL);
  return OldBlock == NULL;
}

BOOL
STDCALL
NtUserBlockInput(
  BOOL BlockIt)
{
  return IntBlockInput(PsGetWin32Thread(), BlockIt);
}

BOOL FASTCALL
IntSwapMouseButton(PWINSTATION_OBJECT WinStaObject, BOOL Swap)
{
  PSYSTEM_CURSORINFO CurInfo;
  BOOL res;
  
  CurInfo = IntGetSysCursorInfo(WinStaObject);
  res = CurInfo->SwapButtons;
  CurInfo->SwapButtons = Swap;
  return res;
}

BOOL FASTCALL
IntMouseInput(MOUSEINPUT *mi)
{
  const UINT SwapBtnMsg[2][2] = {{WM_LBUTTONDOWN, WM_RBUTTONDOWN},
                                 {WM_LBUTTONUP, WM_RBUTTONUP}};
  const WPARAM SwapBtn[2] = {MK_LBUTTON, MK_RBUTTON};
  POINT MousePos;
  PSYSTEM_CURSORINFO CurInfo;
  PWINSTATION_OBJECT WinSta;
  BOOL DoMove, SwapButtons;
  MSG Msg;
  HBITMAP hBitmap;
  BITMAPOBJ *BitmapObj;
  SURFOBJ *SurfObj;
  PDC dc;
  RECTL PointerRect;
  PWINDOW_OBJECT DesktopWindow;
  NTSTATUS Status;
  
#if 1
  HDC hDC;
  
  /* FIXME - get the screen dc from the window station or desktop */
  if(!(hDC = IntGetScreenDC()))
  {
    return FALSE;
  }
#endif
  
  ASSERT(mi);
#if 0
  WinSta = PsGetWin32Process()->WindowStation;
#else
  /* FIXME - ugly hack but as long as we're using this dumb callback from the
             mouse class driver, we can't access the window station from the calling
             process */
  WinSta = InputWindowStation;
#endif
  ASSERT(WinSta);
  
  CurInfo = IntGetSysCursorInfo(WinSta);
  
  if(!mi->time)
  {
    LARGE_INTEGER LargeTickCount;
    KeQueryTickCount(&LargeTickCount);
    mi->time = LargeTickCount.u.LowPart;
  }
  
  SwapButtons = CurInfo->SwapButtons;
  DoMove = FALSE;

  ExAcquireFastMutex(&CurInfo->CursorMutex);
  MousePos.x = CurInfo->x;
  MousePos.y = CurInfo->y;
  if(mi->dwFlags & MOUSEEVENTF_MOVE)
  {
    if(mi->dwFlags & MOUSEEVENTF_ABSOLUTE)
    {
      MousePos.x = mi->dx;
      MousePos.y = mi->dy;
    }
    else
    {
      MousePos.x += mi->dx;
      MousePos.y += mi->dy;
    }
    
    Status = ObmReferenceObjectByHandle(WinSta->HandleTable,
      WinSta->ActiveDesktop->DesktopWindow, otWindow, (PVOID*)&DesktopWindow);
    if (NT_SUCCESS(Status))
    {
      if(MousePos.x >= DesktopWindow->ClientRect.right)
        MousePos.x = DesktopWindow->ClientRect.right - 1;
      if(MousePos.y >= DesktopWindow->ClientRect.bottom)
        MousePos.y = DesktopWindow->ClientRect.bottom - 1;
    }
    ObmDereferenceObject(DesktopWindow);

    if(MousePos.x < 0)
      MousePos.x = 0;
    if(MousePos.y < 0)
      MousePos.y = 0;
    
    if(CurInfo->CursorClipInfo.IsClipped)
    {
      /* The mouse cursor needs to be clipped */
      
      if(MousePos.x >= (LONG)CurInfo->CursorClipInfo.Right)
        MousePos.x = (LONG)CurInfo->CursorClipInfo.Right;
      if(MousePos.x < (LONG)CurInfo->CursorClipInfo.Left)
        MousePos.x = (LONG)CurInfo->CursorClipInfo.Left;
      if(MousePos.y >= (LONG)CurInfo->CursorClipInfo.Bottom)
        MousePos.y = (LONG)CurInfo->CursorClipInfo.Bottom;
      if(MousePos.y < (LONG)CurInfo->CursorClipInfo.Top)
        MousePos.y = (LONG)CurInfo->CursorClipInfo.Top;
    }
    
    DoMove = (MousePos.x != CurInfo->x || MousePos.y != CurInfo->y);
    if(DoMove)
    {
      CurInfo->x = MousePos.x;
      CurInfo->y = MousePos.y;
    }
  }

  ExReleaseFastMutex(&CurInfo->CursorMutex);
  
  if (DoMove)
  {
    dc = DC_LockDc(hDC);
    if (dc)
    {
      hBitmap = dc->w.hBitmap;
      DC_UnlockDc(hDC);

      BitmapObj = BITMAPOBJ_LockBitmap(hBitmap);
      if (BitmapObj)
      {
        SurfObj = &BitmapObj->SurfObj;

        if (GDIDEV(SurfObj)->MovePointer)
        {
          GDIDEV(SurfObj)->MovePointer(SurfObj, MousePos.x, MousePos.y, &PointerRect);
        }

        BITMAPOBJ_UnlockBitmap(hBitmap);

        ExAcquireFastMutex(&CurInfo->CursorMutex);
        SetPointerRect(CurInfo, &PointerRect);
        ExReleaseFastMutex(&CurInfo->CursorMutex);
      }
    }
  }

  /*
   * Insert the messages into the system queue
   */
  
  Msg.wParam = CurInfo->ButtonsDown;
  Msg.lParam = MAKELPARAM(MousePos.x, MousePos.y);
  Msg.pt = MousePos;
  if(DoMove)
  {
    Msg.message = WM_MOUSEMOVE;
    MsqInsertSystemMessage(&Msg);
  }
  
  Msg.message = 0;
  if(mi->dwFlags & MOUSEEVENTF_LEFTDOWN)
  {
    Msg.message = SwapBtnMsg[0][SwapButtons];
    CurInfo->ButtonsDown |= SwapBtn[SwapButtons];
    MsqInsertSystemMessage(&Msg);
  }
  else if(mi->dwFlags & MOUSEEVENTF_LEFTUP)
  {
    Msg.message = SwapBtnMsg[1][SwapButtons];
    CurInfo->ButtonsDown &= ~SwapBtn[SwapButtons];
    MsqInsertSystemMessage(&Msg);
  }
  if(mi->dwFlags & MOUSEEVENTF_MIDDLEDOWN)
  {
    Msg.message = WM_MBUTTONDOWN;
    CurInfo->ButtonsDown |= MK_MBUTTON;
    MsqInsertSystemMessage(&Msg);
  }
  else if(mi->dwFlags & MOUSEEVENTF_MIDDLEUP)
  {
    Msg.message = WM_MBUTTONUP;
    CurInfo->ButtonsDown &= ~MK_MBUTTON;
    MsqInsertSystemMessage(&Msg);
  }
  if(mi->dwFlags & MOUSEEVENTF_RIGHTDOWN)
  {
    Msg.message = SwapBtnMsg[0][!SwapButtons];
    CurInfo->ButtonsDown |= SwapBtn[!SwapButtons];
    MsqInsertSystemMessage(&Msg);
  }
  else if(mi->dwFlags & MOUSEEVENTF_RIGHTUP)
  {
    Msg.message = SwapBtnMsg[1][!SwapButtons];
    CurInfo->ButtonsDown &= ~SwapBtn[!SwapButtons];
    MsqInsertSystemMessage(&Msg);
  }
  
  if((mi->dwFlags & (MOUSEEVENTF_XDOWN | MOUSEEVENTF_XUP)) &&
     (mi->dwFlags & MOUSEEVENTF_WHEEL))
  {
    /* fail because both types of events use the mouseData field */
    return FALSE;
  }
  
  if(mi->dwFlags & MOUSEEVENTF_XDOWN)
  {
    Msg.message = WM_XBUTTONDOWN;
    if(mi->mouseData & XBUTTON1)
    {
      Msg.wParam = MAKEWPARAM(CurInfo->ButtonsDown, XBUTTON1);
      CurInfo->ButtonsDown |= XBUTTON1;
      MsqInsertSystemMessage(&Msg);
    }
    if(mi->mouseData & XBUTTON2)
    {
      Msg.wParam = MAKEWPARAM(CurInfo->ButtonsDown, XBUTTON2);
      CurInfo->ButtonsDown |= XBUTTON2;
      MsqInsertSystemMessage(&Msg);
    }
  }
  else if(mi->dwFlags & MOUSEEVENTF_XUP)
  {
    Msg.message = WM_XBUTTONUP;
    if(mi->mouseData & XBUTTON1)
    {
      Msg.wParam = MAKEWPARAM(CurInfo->ButtonsDown, XBUTTON1);
      CurInfo->ButtonsDown &= ~XBUTTON1;
      MsqInsertSystemMessage(&Msg);
    }
    if(mi->mouseData & XBUTTON2)
    {
      Msg.wParam = MAKEWPARAM(CurInfo->ButtonsDown, XBUTTON2);
      CurInfo->ButtonsDown &= ~XBUTTON2;
      MsqInsertSystemMessage(&Msg);
    }
  }
  if(mi->dwFlags & MOUSEEVENTF_WHEEL)
  {
    Msg.message = WM_MOUSEWHEEL;
    Msg.wParam = MAKEWPARAM(CurInfo->ButtonsDown, mi->mouseData);
    MsqInsertSystemMessage(&Msg);
  }
  
  return TRUE;
}

BOOL FASTCALL
IntKeyboardInput(KEYBDINPUT *ki)
{
  return FALSE;
}

UINT
STDCALL
NtUserSendInput(
  UINT nInputs,
  LPINPUT pInput,
  INT cbSize)
{
  PW32THREAD W32Thread;
  UINT cnt;
  
  W32Thread = PsGetWin32Thread();
  ASSERT(W32Thread);
  
  if(!W32Thread->Desktop)
  {
    return 0;
  }
  
  if(!nInputs || !pInput || (cbSize != sizeof(INPUT)))
  {
    SetLastWin32Error(ERROR_INVALID_PARAMETER);
    return 0;
  }
  
  /*
   * FIXME - check access rights of the window station
   *         e.g. services running in the service window station cannot block input
   */
  if(!ThreadHasInputAccess(W32Thread) ||
     !IntIsActiveDesktop(W32Thread->Desktop))
  {
    SetLastWin32Error(ERROR_ACCESS_DENIED);
    return 0;
  }
  
  cnt = 0;
  while(nInputs--)
  {
    INPUT SafeInput;
    NTSTATUS Status;
    
    Status = MmCopyFromCaller(&SafeInput, pInput++, sizeof(INPUT));
    if(!NT_SUCCESS(Status))
    {
      SetLastNtError(Status);
      return cnt;
    }
    
    switch(SafeInput.type)
    {
      case INPUT_MOUSE:
        if(IntMouseInput(&SafeInput.mi))
        {
          cnt++;
        }
        break;
      case INPUT_KEYBOARD:
        if(IntKeyboardInput(&SafeInput.ki))
        {
          cnt++;
        }
        break;
      case INPUT_HARDWARE:
        break;
#ifndef NDEBUG
      default:
        DPRINT1("SendInput(): Invalid input type: 0x%x\n", SafeInput.type);
        break;
#endif
    }
  }
  
  return cnt;
}

/* EOF */
