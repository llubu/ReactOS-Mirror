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
/* $Id: line.c,v 1.18 2003/08/11 21:10:49 royce Exp $ */

// Some code from the WINE project source (www.winehq.com)

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <internal/safe.h>
#include <ddk/ntddk.h>
#include <win32k/dc.h>
#include <win32k/line.h>
#include <win32k/path.h>
#include <win32k/pen.h>
#include <win32k/region.h>
#include <include/error.h>
#include <include/inteng.h>
#include <include/object.h>
#include <include/path.h>

#define NDEBUG
#include <win32k/debug1.h>


BOOL
STDCALL
W32kAngleArc(HDC  hDC,
                   int  X,
                   int  Y,
                   DWORD  Radius,
                   FLOAT  StartAngle,
                   FLOAT  SweepAngle)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kArc(HDC  hDC,
              int  LeftRect,
              int  TopRect,
              int  RightRect,
              int  BottomRect,
              int  XStartArc,
              int  YStartArc,
              int  XEndArc,
              int  YEndArc)
{
  DC *dc = DC_HandleToPtr(hDC);
  if(!dc) return FALSE;

  if(PATH_IsPathOpen(dc->w.path))
    return PATH_Arc(hDC, LeftRect, TopRect, RightRect, BottomRect,
                    XStartArc, YStartArc, XEndArc, YEndArc);

//   EngArc(dc, LeftRect, TopRect, RightRect, BottomRect, UNIMPLEMENTED
//          XStartArc, YStartArc, XEndArc, YEndArc);

  DC_ReleasePtr( hDC );
  return TRUE;
}

BOOL
STDCALL
W32kArcTo(HDC  hDC,
                int  LeftRect,
                int  TopRect,
                int  RightRect,
                int  BottomRect,
                int  XRadial1,
                int  YRadial1,
                int  XRadial2,
                int  YRadial2)
{
  BOOL result;
  DC *dc = DC_HandleToPtr(hDC);
  if(!dc) return FALSE;

  // Line from current position to starting point of arc
  W32kLineTo(hDC, XRadial1, YRadial1);

  // Then the arc is drawn.
  result = W32kArc(hDC, LeftRect, TopRect, RightRect, BottomRect,
                   XRadial1, YRadial1, XRadial2, YRadial2);

  // If no error occured, the current position is moved to the ending point of the arc.
  if(result)
  {
    W32kMoveToEx(hDC, XRadial2, YRadial2, NULL);
  }
  DC_ReleasePtr( hDC );
  return result;
}

INT
STDCALL
W32kGetArcDirection(HDC  hDC)
{
  PDC dc;
  int ret;

  dc = DC_HandleToPtr (hDC);
  if (!dc)
  {
    return 0;
  }

  ret = dc->w.ArcDirection;
  DC_ReleasePtr( hDC );
  return ret;
}

BOOL
STDCALL
W32kLineTo(HDC  hDC,
           int  XEnd,
           int  YEnd)
{
  DC *dc = DC_HandleToPtr(hDC);
  SURFOBJ *SurfObj = (SURFOBJ*)AccessUserObject((ULONG)dc->Surface);
  BOOL Ret;
  PPENOBJ Pen;
  RECT Bounds;

  if (NULL == dc)
    {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return FALSE;
    }

  if (PATH_IsPathOpen(dc->w.path))
    {
      Ret = PATH_LineTo(hDC, XEnd, YEnd);
    }
  else
    {
      Pen = (PPENOBJ) GDIOBJ_LockObj(dc->w.hPen, GO_PEN_MAGIC);
      ASSERT(NULL != Pen);

      if (dc->w.CursPosX <= XEnd)
	{
	  Bounds.left = dc->w.CursPosX;
	  Bounds.right = XEnd;
	}
      else
	{
	  Bounds.left = XEnd;
	  Bounds.right = dc->w.CursPosX;
	}
      Bounds.left += dc->w.DCOrgX;
      Bounds.right += dc->w.DCOrgX;
      if (dc->w.CursPosY <= YEnd)
	{
	  Bounds.top = dc->w.CursPosY;
	  Bounds.bottom = YEnd;
	}
      else
	{
	  Bounds.top = YEnd;
	  Bounds.bottom = dc->w.CursPosY;
        }
      Bounds.top += dc->w.DCOrgY;
      Bounds.bottom += dc->w.DCOrgY;

      Ret = IntEngLineTo(SurfObj,
                         dc->CombinedClip,
                         PenToBrushObj(dc, Pen),
                         dc->w.DCOrgX + dc->w.CursPosX, dc->w.DCOrgY + dc->w.CursPosY,
                         dc->w.DCOrgX + XEnd,           dc->w.DCOrgY + YEnd,
                         &Bounds,
                         dc->w.ROPmode);

      GDIOBJ_UnlockObj( dc->w.hPen, GO_PEN_MAGIC);
    }

  if (Ret)
    {
      dc->w.CursPosX = XEnd;
      dc->w.CursPosY = YEnd;
    }
  DC_ReleasePtr(hDC);

  return Ret;
}

BOOL
STDCALL
W32kMoveToEx(HDC  hDC,
                   int  X,
                   int  Y,
                   LPPOINT  Point)
{
  DC *dc = DC_HandleToPtr( hDC );

  if(!dc) return FALSE;

  if(Point) {
    Point->x = dc->w.CursPosX;
    Point->y = dc->w.CursPosY;
  }
  dc->w.CursPosX = X;
  dc->w.CursPosY = Y;

  if(PATH_IsPathOpen(dc->w.path)){
  	DC_ReleasePtr( hDC );
    return PATH_MoveTo(hDC);
  }
  DC_ReleasePtr( hDC );
  return TRUE;
}

BOOL
STDCALL
W32kPolyBezier(HDC  hDC,
                     CONST LPPOINT  pt,
                     DWORD  Count)
{
  DC *dc = DC_HandleToPtr(hDC);
  if(!dc) return FALSE;

  if(PATH_IsPathOpen(dc->w.path)){
	DC_ReleasePtr( hDC );
    return PATH_PolyBezier(hDC, pt, Count);
  }

  /* We'll convert it into line segments and draw them using Polyline */
  {
    POINT *Pts;
    INT nOut;
    BOOL ret;

    Pts = GDI_Bezier(pt, Count, &nOut);
    if(!Pts) return FALSE;
    DbgPrint("Pts = %p, no = %d\n", Pts, nOut);
    ret = W32kPolyline(dc->hSelf, Pts, nOut);
    ExFreePool(Pts);
	DC_ReleasePtr( hDC );
    return ret;
  }
}

BOOL
STDCALL
W32kPolyBezierTo(HDC  hDC,
                       CONST LPPOINT  pt,
                       DWORD  Count)
{
  DC *dc = DC_HandleToPtr(hDC);
  BOOL ret;

  if(!dc) return FALSE;

  if(PATH_IsPathOpen(dc->w.path))
    ret = PATH_PolyBezierTo(hDC, pt, Count);
  else { /* We'll do it using PolyBezier */
    POINT *npt;
    npt = ExAllocatePool(NonPagedPool, sizeof(POINT) * (Count + 1));
    if(!npt) return FALSE;
    npt[0].x = dc->w.CursPosX;
    npt[0].y = dc->w.CursPosY;
    memcpy(npt + 1, pt, sizeof(POINT) * Count);
    ret = W32kPolyBezier(dc->hSelf, npt, Count+1);
    ExFreePool(npt);
  }
  if(ret) {
    dc->w.CursPosX = pt[Count-1].x;
    dc->w.CursPosY = pt[Count-1].y;
  }
  DC_ReleasePtr( hDC );
  return ret;
}

BOOL
STDCALL
W32kPolyDraw(HDC  hDC,
                   CONST LPPOINT  pt,
                   CONST LPBYTE  Types,
                   int  Count)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kPolyline(HDC  hDC,
                   CONST LPPOINT  pt,
                   int  Count)
{
  DC		*dc = DC_HandleToPtr(hDC);
  SURFOBJ	*SurfObj = (SURFOBJ*)AccessUserObject((ULONG)dc->Surface);
  BOOL ret;
  LONG i;
  PPENOBJ   pen;
  PROSRGNDATA  reg;
  POINT *pts;

  if (!dc) 
    return(FALSE);

  if(PATH_IsPathOpen(dc->w.path))
  {
    ret = PATH_Polyline(hDC, pt, Count);
  }
  else
  {
    pen = (PPENOBJ) GDIOBJ_LockObj(dc->w.hPen, GO_PEN_MAGIC);
    reg = (PROSRGNDATA)GDIOBJ_LockObj(dc->w.hGCClipRgn, GO_REGION_MAGIC);

    ASSERT( pen );

    //FIXME: Do somthing with reg...

    //Allocate "Count" bytes of memory to hold a safe copy of pt
    if (!(pts=ExAllocatePool(NonPagedPool, sizeof(POINT) * Count))) 
    {
      GDIOBJ_UnlockObj( dc->w.hGCClipRgn, GO_REGION_MAGIC );
      GDIOBJ_UnlockObj( dc->w.hPen, GO_PEN_MAGIC);
      DC_ReleasePtr( hDC );
      return(FALSE);
    }
 
    //safly copy pt to local version
    if (STATUS_SUCCESS!=MmCopyFromCaller(pts, pt, sizeof(POINT) * Count))
    {
      ExFreePool(pts);
      GDIOBJ_UnlockObj( dc->w.hGCClipRgn, GO_REGION_MAGIC );
      GDIOBJ_UnlockObj( dc->w.hPen, GO_PEN_MAGIC);
      DC_ReleasePtr( hDC );
      return(FALSE);
    }

    //offset the array of point by the dc->w.DCOrg
    for(i=0; i<Count; i++)
    {
      pts[i].x += dc->w.DCOrgX;
      pts[i].y += dc->w.DCOrgY;
    }
  
    //get IntEngPolyline to do the drawing.
    ret = IntEngPolyline(SurfObj,
	                 dc->CombinedClip,
	                 PenToBrushObj(dc, pen),
	                 pts,
                         Count,
	                 dc->w.ROPmode);

    //Clean up
    ExFreePool(pts);
    GDIOBJ_UnlockObj( dc->w.hGCClipRgn, GO_REGION_MAGIC );
    GDIOBJ_UnlockObj( dc->w.hPen, GO_PEN_MAGIC);
  }

  DC_ReleasePtr( hDC );
  return(ret);
}

BOOL
STDCALL
W32kPolylineTo(HDC  hDC,
                     CONST LPPOINT  pt,
                     DWORD  Count)
{
  DC *dc = DC_HandleToPtr(hDC);
  BOOL ret;

  if(!dc) return FALSE;

  if(PATH_IsPathOpen(dc->w.path))
  {
    ret = PATH_PolylineTo(hDC, pt, Count);
  }
  else { /* do it using Polyline */
    POINT *pts = ExAllocatePool(NonPagedPool, sizeof(POINT) * (Count + 1));
    if(!pts) return FALSE;

    pts[0].x = dc->w.CursPosX;
    pts[0].y = dc->w.CursPosY;
    memcpy( pts + 1, pt, sizeof(POINT) * Count);
    ret = W32kPolyline(hDC, pts, Count + 1);
    ExFreePool(pts);
  }
  if(ret) {
    dc->w.CursPosX = pt[Count-1].x;
    dc->w.CursPosY = pt[Count-1].y;
  }
  DC_ReleasePtr( hDC );
  return ret;
}

BOOL
STDCALL
W32kPolyPolyline(HDC  hDC,
                       CONST LPPOINT  pt,
                       CONST LPDWORD  PolyPoints,
                       DWORD  Count)
{
   UNIMPLEMENTED;
}

int
STDCALL
W32kSetArcDirection(HDC  hDC,
                         int  ArcDirection)
{
  PDC  dc;
  INT  nOldDirection;

  dc = DC_HandleToPtr (hDC);
  if (!dc)
  {
    return 0;
  }
  if (ArcDirection != AD_COUNTERCLOCKWISE && ArcDirection != AD_CLOCKWISE)
  {
//    SetLastError(ERROR_INVALID_PARAMETER);
	DC_ReleasePtr( hDC );
    return 0;
  }

  nOldDirection = dc->w.ArcDirection;
  dc->w.ArcDirection = ArcDirection;
  DC_ReleasePtr( hDC );
  return nOldDirection;
}
/* EOF */
