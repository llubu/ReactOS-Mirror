/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
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
/* $Id: paint.c,v 1.19 2003/12/08 18:21:24 navaraf Exp $
 *
 * PROJECT:         ReactOS user32.dll
 * FILE:            lib/user32/windows/input.c
 * PURPOSE:         Input
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      09-05-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#include <windows.h>
#include <resource.h>
#include <user32.h>
#define NDEBUG
#include <debug.h>

static HBRUSH FrameBrushes[13];
static HBITMAP hHatch;

/* FUNCTIONS *****************************************************************/

static VOID 
CreateFrameBrushes()
{
  FrameBrushes[0] = CreateSolidBrush(RGB(0,0,0));
  FrameBrushes[1] = CreateSolidBrush(RGB(0,0,128));
  FrameBrushes[2] = CreateSolidBrush(RGB(10,36,106));
  FrameBrushes[3] = CreateSolidBrush(RGB(128,128,128));
  FrameBrushes[4] = CreateSolidBrush(RGB(181,181,181));
  FrameBrushes[5] = CreateSolidBrush(RGB(212,208,200));
  FrameBrushes[6] = CreateSolidBrush(RGB(236,233,216));
  FrameBrushes[7] = CreateSolidBrush(RGB(255,255,255));
  FrameBrushes[8] = CreateSolidBrush(RGB(49,106,197));
  FrameBrushes[9] = CreateSolidBrush(RGB(58,110,165));
  FrameBrushes[10] = CreateSolidBrush(RGB(64,64,64));
  FrameBrushes[11] = CreateSolidBrush(RGB(255,255,225));
  hHatch = LoadBitmapW(NULL,MAKEINTRESOURCEW(DF_HATCH));
  FrameBrushes[12] = CreatePatternBrush(hHatch);
}

VOID 
DeleteFrameBrushes(VOID)
{
  unsigned Brush;

  for (Brush = 0; Brush < sizeof(FrameBrushes) / sizeof(HBRUSH); Brush++)
    {
      if (NULL != FrameBrushes[Brush])
	{
	  DeleteObject(FrameBrushes[Brush]);
	  FrameBrushes[Brush] = NULL;
	}
    }
  if (NULL != hHatch)
    {
      DeleteObject(hHatch);
      hHatch = NULL;
    }
}

/*
 * @implemented
 */
HDC
STDCALL
BeginPaint(
  HWND hwnd,
  LPPAINTSTRUCT lpPaint)
{
  return NtUserBeginPaint(hwnd, lpPaint);
}


/*
 * @implemented
 */
WINBOOL
STDCALL
EndPaint(
  HWND hWnd,
  CONST PAINTSTRUCT *lpPaint)
{
  return NtUserEndPaint(hWnd, lpPaint);
}


/*
 * @unimplemented
 */
int
STDCALL
ExcludeUpdateRgn(
  HDC hDC,
  HWND hWnd)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
GetUpdateRect(
  HWND hWnd,
  LPRECT lpRect,
  WINBOOL bErase)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @implemented
 */
int
STDCALL
GetUpdateRgn(
  HWND hWnd,
  HRGN hRgn,
  WINBOOL bErase)
{
  return NtUserGetUpdateRgn(hWnd, hRgn, bErase);
}


/*
 * @implemented
 */
WINBOOL
STDCALL
InvalidateRect(
  HWND hWnd,
  CONST RECT *lpRect,
  WINBOOL bErase)
{
  return NtUserInvalidateRect( hWnd, lpRect, bErase );
}


/*
 * @implemented
 */
WINBOOL
STDCALL
InvalidateRgn(
  HWND hWnd,
  HRGN hRgn,
  WINBOOL bErase)
{
  return NtUserInvalidateRgn( hWnd, hRgn, bErase );
}


/*
 * @implemented
 */
WINBOOL
STDCALL
RedrawWindow(
  HWND hWnd,
  CONST RECT *lprcUpdate,
  HRGN hrgnUpdate,
  UINT flags)
{
 return NtUserRedrawWindow(hWnd, lprcUpdate, hrgnUpdate, flags);
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
ScrollDC(HDC hDC, int dx, int dy, CONST RECT *lprcScroll, CONST RECT *lprcClip,
   HRGN hrgnUpdate, LPRECT lprcUpdate)
{
   return NtUserScrollDC(hDC, dx, dy, lprcScroll, lprcClip, hrgnUpdate,
      lprcUpdate);
}


/*
 * @unimplemented
 */
int
STDCALL
SetWindowRgn(
  HWND hWnd,
  HRGN hRgn,
  WINBOOL bRedraw)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
UpdateWindow(
  HWND hWnd)
{
  return NtUserUpdateWindow( hWnd );
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
ValidateRect(
  HWND hWnd,
  CONST RECT *lpRect)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @implemented
 */
WINBOOL
STDCALL
ValidateRgn(
  HWND hWnd,
  HRGN hRgn)
{
  return (WINBOOL) NtUserCallTwoParam((DWORD) hWnd,
                                      (DWORD) hRgn,
                                      TWOPARAM_ROUTINE_VALIDATERGN);
}


/*
 * @unimplemented
 */
int
STDCALL
GetWindowRgn(
  HWND hWnd,
  HRGN hRgn)
{
  UNIMPLEMENTED;
  return 0;
}

const BYTE MappingTable[33] = {5,9,2,3,5,7,0,0,0,7,5,5,3,2,7,5,3,3,0,5,7,10,5,0,11,4,1,1,3,8,6,12,7};
/*
 * @implemented
 */
WINBOOL
STDCALL
DrawFrame(
	  HDC    hDc,
	  RECT  *r,
	  DWORD  width,
	  DWORD  type
	  )
{
	DWORD rop;
	DWORD brush;
	HBRUSH hbrFrame;
	PATRECT p[4];
	if (NULL == FrameBrushes[0])
	{
		CreateFrameBrushes();
	}
	if (type & 4)
	{
		rop = PATINVERT;
	}
	else
	{
		rop = PATCOPY;
	}
	brush = type / 8;
	if (brush >= 33)
	{
		brush = 32;
	}
	brush = MappingTable[brush];
	hbrFrame = FrameBrushes[brush];
	p[0].hBrush = hbrFrame;
	p[1].hBrush = hbrFrame;
	p[2].hBrush = hbrFrame;
	p[3].hBrush = hbrFrame;
	p[0].r.left = r->left;
	p[0].r.top = r->top;
	p[0].r.right = r->right - r->left;
	p[0].r.bottom = width;
	p[1].r.left = r->left;
	p[1].r.top = r->bottom - width;
	p[1].r.right = r->right - r->left;
	p[1].r.bottom = width;
	p[2].r.left = r->left;
	p[2].r.top = r->top + width;
	p[2].r.right = width;
	p[2].r.bottom = r->bottom - r->top - (width * 2);
	p[3].r.left = r->right - width;
	p[3].r.top = r->top + width;
	p[3].r.right = width;
	p[3].r.bottom = r->bottom - r->top - (width * 2);
	return PolyPatBlt(hDc,rop,p,4,0);
}
