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
/* $Id: mouse.c,v 1.48 2003/12/13 22:38:29 weiden Exp $
 *
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Mouse
 * FILE:             subsys/win32k/eng/mouse.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISION HISTORY:
 *       06-06-2001  CSH  Created
 */
/* INCLUDES ******************************************************************/

#include <windows.h>
#include <ddk/ntddk.h>
#include <ddk/ntddmou.h>
#include <win32k/win32k.h>
#include <win32k/dc.h>
#include "objects.h"
#include "include/msgqueue.h"
#include "include/object.h"
#include "include/winsta.h"
#include "include/desktop.h"
#include "include/window.h"
#include "include/cursoricon.h"
#include "include/callback.h"
#include <include/mouse.h>

#define NDEBUG
#include <debug.h>

#define GETSYSCURSOR(x) ((x) - OCR_NORMAL)

/* FUNCTIONS *****************************************************************/

BOOL FASTCALL
IntCheckClipCursor(LONG *x, LONG *y, PSYSTEM_CURSORINFO CurInfo)
{
  if(CurInfo->CursorClipInfo.IsClipped)
  {
    if(*x > CurInfo->CursorClipInfo.Right)
      *x = CurInfo->CursorClipInfo.Right;
    if(*x < CurInfo->CursorClipInfo.Left)
      *x = CurInfo->CursorClipInfo.Left;
    if(*y > CurInfo->CursorClipInfo.Bottom)
      *y = CurInfo->CursorClipInfo.Bottom;
    if(*y < CurInfo->CursorClipInfo.Top)
      *y = CurInfo->CursorClipInfo.Top;
    return TRUE;
  }
  return TRUE;
}

BOOL FASTCALL
IntDetectDblClick(PSYSTEM_CURSORINFO CurInfo, DWORD TickCount)
{
  LONG dX, dY;
  BOOL res = ((TickCount - CurInfo->LastBtnDown) < CurInfo->DblClickSpeed);
  if(res)
  {
    /* check if the second click is within the DblClickWidth and DblClickHeight values */
    dX = CurInfo->LastBtnDownX - CurInfo->x;
    dY = CurInfo->LastBtnDownY - CurInfo->y;
    if(dX < 0) dX = -dX;
    if(dY < 0) dY = -dY;
    
    res = (dX <= CurInfo->DblClickWidth) &&
          (dY <= CurInfo->DblClickHeight);

    if(res)
    {
      CurInfo->LastBtnDown = 0; /* prevent sending 2 or more DBLCLK messages */
      CurInfo->LastBtnDownX = CurInfo->x;
      CurInfo->LastBtnDownY = CurInfo->y;
    }
    else
    {
      CurInfo->LastBtnDown = TickCount;
      CurInfo->LastBtnDownX = CurInfo->x;
      CurInfo->LastBtnDownY = CurInfo->y;
    }
  }
  else
  {
    CurInfo->LastBtnDown = TickCount;
    CurInfo->LastBtnDownX = CurInfo->x;
    CurInfo->LastBtnDownY = CurInfo->y;
  }
  return res;
}

BOOL FASTCALL
IntSwapMouseButton(PWINSTATION_OBJECT WinStaObject, BOOL Swap)
{
  BOOL res = WinStaObject->SystemCursor.SwapButtons;
  WinStaObject->SystemCursor.SwapButtons = Swap;
  return res;
}

INT STDCALL
MouseSafetyOnDrawStart(PSURFOBJ SurfObj, PSURFGDI SurfGDI, LONG HazardX1,
		       LONG HazardY1, LONG HazardX2, LONG HazardY2)
/*
 * FUNCTION: Notify the mouse driver that drawing is about to begin in
 * a rectangle on a particular surface.
 */
{
  RECTL MouseRect;
  LONG tmp;
  PSYSTEM_CURSORINFO CurInfo;
  BOOL MouseEnabled = FALSE;
  PCURICON_OBJECT Cursor;


  /* Mouse is not allowed to move if GDI is busy drawing */
   
  if(IntGetWindowStationObject(InputWindowStation))
  {
    CurInfo = &InputWindowStation->SystemCursor;
    
    MouseEnabled = CurInfo->Enabled && CurInfo->ShowingCursor;
  }
  else
    return FALSE;
  
  ExAcquireFastMutexUnsafe(&CurInfo->CursorMutex);
  CurInfo->SafetySwitch2 = TRUE;
  ExReleaseFastMutexUnsafe(&CurInfo->CursorMutex);
    
  if (SurfObj == NULL)
    {
      ObDereferenceObject(InputWindowStation);
      return(FALSE);
    }


  if (SurfObj->iType != STYPE_DEVICE || MouseEnabled == FALSE)
    {
      ObDereferenceObject(InputWindowStation);
      return(FALSE);
    }

  if (SPS_ACCEPT_NOEXCLUDE == SurfGDI->PointerStatus)
    {
      /* Hardware cursor, no need to remove it */
      ObDereferenceObject(InputWindowStation);
      return(FALSE);
    }
  
  if(!(Cursor = CurInfo->CurrentCursorObject))
  {
    ObDereferenceObject(InputWindowStation);
    return(FALSE);
  }

  if (HazardX1 > HazardX2)
    {
      tmp = HazardX2; HazardX2 = HazardX1; HazardX1 = tmp;
    }
  if (HazardY1 > HazardY2)
    {
      tmp = HazardY2; HazardY2 = HazardY1; HazardY1 = tmp;
    }

  if (CurInfo->x - (INT) Cursor->IconInfo.xHotspot + Cursor->Size.cx >= HazardX1
      && CurInfo->x - (INT) Cursor->IconInfo.xHotspot <= HazardX2
      && CurInfo->y - (INT) Cursor->IconInfo.yHotspot + Cursor->Size.cy >= HazardY1
      && CurInfo->y - (INT) Cursor->IconInfo.yHotspot <= HazardY2)
    {
      /* Mouse is not allowed to move if GDI is busy drawing */
      ExAcquireFastMutexUnsafe(&CurInfo->CursorMutex);
      CurInfo->SafetySwitch = TRUE;
      SurfGDI->MovePointer(SurfObj, -1, -1, &MouseRect);
      ExReleaseFastMutexUnsafe(&CurInfo->CursorMutex);
    }
    
  ObDereferenceObject(InputWindowStation);
  return(TRUE);
}

INT FASTCALL
MouseSafetyOnDrawEnd(PSURFOBJ SurfObj, PSURFGDI SurfGDI)
/*
 * FUNCTION: Notify the mouse driver that drawing has finished on a surface.
 */
{
  RECTL MouseRect;
  PSYSTEM_CURSORINFO CurInfo;
  BOOL MouseEnabled = FALSE;
    
  if(IntGetWindowStationObject(InputWindowStation))
  {
    CurInfo = &InputWindowStation->SystemCursor;
  }
  else
    return FALSE;
    
  if(SurfObj == NULL)
  {
    ExAcquireFastMutexUnsafe(&CurInfo->CursorMutex);
    CurInfo->SafetySwitch2 = FALSE;
    ExReleaseFastMutexUnsafe(&CurInfo->CursorMutex);
    ObDereferenceObject(InputWindowStation);
    return FALSE;
  }
  
  MouseEnabled = CurInfo->Enabled && CurInfo->ShowingCursor;

  if (SurfObj->iType != STYPE_DEVICE || MouseEnabled == FALSE)
    {
      ExAcquireFastMutexUnsafe(&CurInfo->CursorMutex);
      CurInfo->SafetySwitch2 = FALSE;
      ExReleaseFastMutexUnsafe(&CurInfo->CursorMutex);
      ObDereferenceObject(InputWindowStation);
      return(FALSE);
    }

  if (SPS_ACCEPT_NOEXCLUDE == SurfGDI->PointerStatus)
    {
      /* Hardware cursor, it wasn't removed so need to restore it */
      ExAcquireFastMutexUnsafe(&CurInfo->CursorMutex);
      CurInfo->SafetySwitch2 = FALSE;
      ExReleaseFastMutexUnsafe(&CurInfo->CursorMutex);
      ObDereferenceObject(InputWindowStation);
      return(FALSE);
    }
  
  ExAcquireFastMutexUnsafe(&CurInfo->CursorMutex);
  if (CurInfo->SafetySwitch)
    {
      SurfGDI->MovePointer(SurfObj, CurInfo->x, CurInfo->y, &MouseRect);
      CurInfo->SafetySwitch = FALSE;
      CurInfo->SafetySwitch2 = FALSE;
    }
  else
    {
      CurInfo->SafetySwitch2 = FALSE;      
    }
  ExReleaseFastMutexUnsafe(&CurInfo->CursorMutex);
  ObDereferenceObject(InputWindowStation);
  return(TRUE);
}

BOOL FASTCALL
MouseMoveCursor(LONG X, LONG Y)
{
  HDC hDC;
  PDC dc;
  RECTL MouseRect;
  BOOL res = FALSE;
  PSURFOBJ SurfObj;
  PSURFGDI SurfGDI;
  PSYSTEM_CURSORINFO CurInfo;
  MSG Msg;
  LARGE_INTEGER LargeTickCount;
  ULONG TickCount;
  
  if(!InputWindowStation)
    return FALSE;
  
  if(IntGetWindowStationObject(InputWindowStation))
  {
    CurInfo = &InputWindowStation->SystemCursor;
    if(!CurInfo->Enabled)
    {
      ObDereferenceObject(InputWindowStation);
      return FALSE;
    }
    hDC = IntGetScreenDC();
    if(!hDC)
    {
      ObDereferenceObject(InputWindowStation);
      return FALSE;
    }
    dc = DC_LockDc(hDC);
    SurfObj = (PSURFOBJ)AccessUserObject((ULONG) dc->Surface);
    SurfGDI = (PSURFGDI)AccessInternalObject((ULONG) dc->Surface);
    DC_UnlockDc( hDC );
    IntCheckClipCursor(&X, &Y, CurInfo);
    if((X != CurInfo->x) || (Y != CurInfo->y))
    {
      /* send MOUSEMOVE message */
      KeQueryTickCount(&LargeTickCount);
      TickCount = LargeTickCount.u.LowPart;
      Msg.wParam = CurInfo->ButtonsDown;
      Msg.lParam = MAKELPARAM(X, Y);
      Msg.message = WM_MOUSEMOVE;
      Msg.time = TickCount;
      Msg.pt.x = X;
      Msg.pt.y = Y;
      MsqInsertSystemMessage(&Msg, TRUE);
      /* move cursor */
      CurInfo->x = X;
      CurInfo->y = Y;
      if(CurInfo->Enabled)
      {
        ExAcquireFastMutexUnsafe(&CurInfo->CursorMutex);
        SurfGDI->MovePointer(SurfObj, CurInfo->x, CurInfo->y, &MouseRect);
        ExReleaseFastMutexUnsafe(&CurInfo->CursorMutex);
      }
      res = TRUE;
    }
        
    ObDereferenceObject(InputWindowStation);
    return res;
  }
  else
    return FALSE;
}

VOID /* STDCALL */
MouseGDICallBack(PMOUSE_INPUT_DATA Data, ULONG InputCount)
/*
 * FUNCTION: Call by the mouse driver when input events occur.
 */
{
  ULONG i;
  PSYSTEM_CURSORINFO CurInfo;
  BOOL MouseEnabled = FALSE;
  BOOL MouseMoveAdded = FALSE;
  BOOL Moved = FALSE;
  LONG mouse_ox, mouse_oy;
  LONG mouse_cx = 0, mouse_cy = 0;
  LONG dScroll = 0;
  HDC hDC;
  PDC dc;
  PSURFOBJ SurfObj;
  PSURFGDI SurfGDI;
  RECTL MouseRect;
  WORD wp;
  MSG Msg;
  LARGE_INTEGER LargeTickCount;
  ULONG TickCount;
  
  hDC = IntGetScreenDC();
  
  if(!hDC || !InputWindowStation)
    return;

  if(IntGetWindowStationObject(InputWindowStation))
  {
    CurInfo = &InputWindowStation->SystemCursor;
    MouseEnabled = CurInfo->Enabled;
    if(!MouseEnabled)
    {
      ObDereferenceObject(InputWindowStation);
      return;
    }
    mouse_ox = CurInfo->x;
    mouse_oy = CurInfo->y;
  }
  else
    return;

  dc = DC_LockDc(hDC);
  SurfObj = (PSURFOBJ)AccessUserObject((ULONG) dc->Surface);
  SurfGDI = (PSURFGDI)AccessInternalObject((ULONG) dc->Surface);
  DC_UnlockDc( hDC );

  /* Compile the total mouse movement change and dispatch button events. */
  for (i = 0; i < InputCount; i++)
  {
    mouse_cx += Data[i].LastX;
    mouse_cy += Data[i].LastY;
    
    CurInfo->x += Data[i].LastX;
    CurInfo->y += Data[i].LastY;
    
    CurInfo->x = max(CurInfo->x, 0);
    CurInfo->y = max(CurInfo->y, 0);
    CurInfo->x = min(CurInfo->x, SurfObj->sizlBitmap.cx - 1);
    CurInfo->y = min(CurInfo->y, SurfObj->sizlBitmap.cy - 1);
    
    IntCheckClipCursor(&CurInfo->x, &CurInfo->y, CurInfo);
    
    KeQueryTickCount(&LargeTickCount);
    TickCount = LargeTickCount.u.LowPart;

    Msg.wParam = CurInfo->ButtonsDown;
    Msg.lParam = MAKELPARAM(CurInfo->x, CurInfo->y);
    Msg.message = WM_MOUSEMOVE;
    Msg.time = TickCount;
    Msg.pt.x = CurInfo->x;
    Msg.pt.y = CurInfo->y;
    
    MouseMoveAdded = FALSE;
    
    //PrintInputData(i, Data[i]);
    
    if (Data[i].ButtonFlags != 0)
    {
      wp = 0;
      if ((Data[i].ButtonFlags & MOUSE_LEFT_BUTTON_DOWN) > 0)
      {
        /* insert WM_MOUSEMOVE messages before Button down messages */
        if ((0 != Data[i].LastX) || (0 != Data[i].LastY))
        {
          MsqInsertSystemMessage(&Msg, FALSE);
          MouseMoveAdded = TRUE;
        }
      	CurInfo->ButtonsDown |= CurInfo->SwapButtons ? MK_RBUTTON : MK_LBUTTON;
      	if(IntDetectDblClick(CurInfo, TickCount))
          Msg.message = CurInfo->SwapButtons ? WM_RBUTTONDBLCLK : WM_LBUTTONDBLCLK;
        else
          Msg.message = CurInfo->SwapButtons ? WM_RBUTTONDOWN : WM_LBUTTONDOWN;
      }
      if ((Data[i].ButtonFlags & MOUSE_MIDDLE_BUTTON_DOWN) > 0)
      {
        /* insert WM_MOUSEMOVE messages before Button down messages */
        if ((0 != Data[i].LastX) || (0 != Data[i].LastY))
        {
          MsqInsertSystemMessage(&Msg, FALSE);
          MouseMoveAdded = TRUE;
        }
      	CurInfo->ButtonsDown |= MK_MBUTTON;
      	if(IntDetectDblClick(CurInfo, TickCount))
      	  Msg.message = WM_MBUTTONDBLCLK;
      	else
          Msg.message = WM_MBUTTONDOWN;
      }
      if ((Data[i].ButtonFlags & MOUSE_RIGHT_BUTTON_DOWN) > 0)
      {
        /* insert WM_MOUSEMOVE messages before Button down messages */
        if ((0 != Data[i].LastX) || (0 != Data[i].LastY))
        {
          MsqInsertSystemMessage(&Msg, FALSE);
          MouseMoveAdded = TRUE;
        }
      	CurInfo->ButtonsDown |= CurInfo->SwapButtons ? MK_LBUTTON : MK_RBUTTON;
      	if(IntDetectDblClick(CurInfo, TickCount))
      	  Msg.message = CurInfo->SwapButtons ? WM_LBUTTONDBLCLK : WM_RBUTTONDBLCLK;
      	else
          Msg.message = CurInfo->SwapButtons ? WM_LBUTTONDOWN : WM_RBUTTONDOWN;
      }
      
      if ((Data[i].ButtonFlags & MOUSE_BUTTON_4_DOWN) > 0)
      {
        /* insert WM_MOUSEMOVE messages before Button down messages */
        if ((0 != Data[i].LastX) || (0 != Data[i].LastY))
        {
          MsqInsertSystemMessage(&Msg, FALSE);
          MouseMoveAdded = TRUE;
        }
      	CurInfo->ButtonsDown |= MK_XBUTTON1;
      	if(IntDetectDblClick(CurInfo, TickCount))
      	{
      	  Msg.message = WM_XBUTTONDBLCLK;
      	  wp = XBUTTON1;
   	    }
      	else
          Msg.message = WM_XBUTTONDOWN;
      }
      if ((Data[i].ButtonFlags & MOUSE_BUTTON_5_DOWN) > 0)
      {
        /* insert WM_MOUSEMOVE messages before Button down messages */
        if ((0 != Data[i].LastX) || (0 != Data[i].LastY))
        {
          MsqInsertSystemMessage(&Msg, FALSE);
          MouseMoveAdded = TRUE;
        }
      	CurInfo->ButtonsDown |= MK_XBUTTON2;
      	if(IntDetectDblClick(CurInfo, TickCount))
      	{
      	  Msg.message = WM_XBUTTONDBLCLK;
      	  wp = XBUTTON2;
   	    }
      	else
          Msg.message = WM_XBUTTONDOWN;
      }

      if ((Data[i].ButtonFlags & MOUSE_LEFT_BUTTON_UP) > 0)
      {
      	CurInfo->ButtonsDown &= CurInfo->SwapButtons ? ~MK_RBUTTON : ~MK_LBUTTON;
        Msg.message = CurInfo->SwapButtons ? WM_RBUTTONUP : WM_LBUTTONUP;
      }
      if ((Data[i].ButtonFlags & MOUSE_MIDDLE_BUTTON_UP) > 0)
      {
      	CurInfo->ButtonsDown &= ~MK_MBUTTON;
        Msg.message = WM_MBUTTONUP;
      }
      if ((Data[i].ButtonFlags & MOUSE_RIGHT_BUTTON_UP) > 0)
      {
      	CurInfo->ButtonsDown &= CurInfo->SwapButtons ? ~MK_LBUTTON : ~MK_RBUTTON;
        Msg.message = CurInfo->SwapButtons ? WM_LBUTTONUP : WM_RBUTTONUP;
      }
      if ((Data[i].ButtonFlags & MOUSE_BUTTON_4_UP) > 0)
      {
      	CurInfo->ButtonsDown &= ~MK_XBUTTON1;
        Msg.message = WM_XBUTTONUP;
      }
      if ((Data[i].ButtonFlags & MOUSE_BUTTON_5_UP) > 0)
      {
      	CurInfo->ButtonsDown &= ~MK_XBUTTON2;
        Msg.message = WM_XBUTTONUP;
      }
      if ((Data[i].ButtonFlags & MOUSE_WHEEL) > 0)
      {
        dScroll += (LONG)Data[i].ButtonData;
      }
      
      if (Data[i].ButtonFlags != MOUSE_WHEEL)
      {
        Moved = (0 != mouse_cx) || (0 != mouse_cy);
        if(Moved && MouseEnabled)
        {
          if (!CurInfo->SafetySwitch && !CurInfo->SafetySwitch2 &&
              ((mouse_ox != CurInfo->x) || (mouse_oy != CurInfo->y)))
          {
            ExAcquireFastMutexUnsafe(&CurInfo->CursorMutex);
            SurfGDI->MovePointer(SurfObj, CurInfo->x, CurInfo->y, &MouseRect);
            ExReleaseFastMutexUnsafe(&CurInfo->CursorMutex);
            mouse_cx = 0;
            mouse_cy = 0;
          }
        }
        
        Msg.wParam = CurInfo->ButtonsDown;
        MsqInsertSystemMessage(&Msg, FALSE);
      
        /* insert WM_MOUSEMOVE messages after Button up messages */
        if(!MouseMoveAdded && Moved)
        {
          Msg.message = WM_MOUSEMOVE;
          MsqInsertSystemMessage(&Msg, FALSE);
          MouseMoveAdded = TRUE;
        }
      }
    }
  }
  
  KeQueryTickCount(&LargeTickCount);
  TickCount = LargeTickCount.u.LowPart;

  /* If the mouse moved then move the pointer. */
  if ((mouse_cx != 0 || mouse_cy != 0) && MouseEnabled)
  {

    if(!MouseMoveAdded)
    {
      Msg.wParam = CurInfo->ButtonsDown;
      Msg.message = WM_MOUSEMOVE;
      Msg.pt.x = CurInfo->x;
      Msg.pt.y = CurInfo->y;
      Msg.time = TickCount;
      Msg.lParam = MAKELPARAM(CurInfo->x, CurInfo->y);
      MsqInsertSystemMessage(&Msg, TRUE);
    }
    
    if (!CurInfo->SafetySwitch && !CurInfo->SafetySwitch2 &&
        ((mouse_ox != CurInfo->x) || (mouse_oy != CurInfo->y)))
    {
      ExAcquireFastMutexUnsafe(&CurInfo->CursorMutex);
      SurfGDI->MovePointer(SurfObj, CurInfo->x, CurInfo->y, &MouseRect);
      ExReleaseFastMutexUnsafe(&CurInfo->CursorMutex);
    }
  }
  
  /* send WM_MOUSEWHEEL message */
  if(dScroll && MouseEnabled)
  {
    Msg.message = WM_MOUSEWHEEL;
    Msg.wParam = MAKEWPARAM(CurInfo->ButtonsDown, dScroll);
    Msg.lParam = MAKELPARAM(CurInfo->x, CurInfo->y);
    Msg.time = TickCount;
    Msg.pt.x = CurInfo->x;
    Msg.pt.y = CurInfo->y;
    MsqInsertSystemMessage(&Msg, FALSE);
  }

  ObDereferenceObject(InputWindowStation);
}

VOID FASTCALL
EnableMouse(HDC hDisplayDC)
{
  PDC dc;
  PSURFOBJ SurfObj;
  PSURFGDI SurfGDI;

  if( hDisplayDC && InputWindowStation)
  {
    if(!IntGetWindowStationObject(InputWindowStation))
    {
       InputWindowStation->SystemCursor.Enabled = FALSE;
       return;
    }
    
    dc = DC_LockDc(hDisplayDC);
    SurfObj = (PSURFOBJ)AccessUserObject((ULONG) dc->Surface);
    SurfGDI = (PSURFGDI)AccessInternalObject((ULONG) dc->Surface);
    DC_UnlockDc( hDisplayDC );
    
    IntSetCursor(InputWindowStation, NULL, TRUE);
    
    InputWindowStation->SystemCursor.Enabled = (SPS_ACCEPT_EXCLUDE == SurfGDI->PointerStatus ||
                                                SPS_ACCEPT_NOEXCLUDE == SurfGDI->PointerStatus);
    
    /* Move the cursor to the screen center */
    DPRINT("Setting Cursor up at 0x%x, 0x%x\n", SurfObj->sizlBitmap.cx / 2, SurfObj->sizlBitmap.cy / 2);
    MouseMoveCursor(SurfObj->sizlBitmap.cx / 2, SurfObj->sizlBitmap.cy / 2);

    ObDereferenceObject(InputWindowStation);
  }
  else
  {
    if(IntGetWindowStationObject(InputWindowStation))
    {
       IntSetCursor(InputWindowStation, NULL, TRUE);
       InputWindowStation->SystemCursor.Enabled = FALSE;
       InputWindowStation->SystemCursor.CursorClipInfo.IsClipped = FALSE;
	   ObDereferenceObject(InputWindowStation);
       return;
    }
  }
}
/* EOF */
