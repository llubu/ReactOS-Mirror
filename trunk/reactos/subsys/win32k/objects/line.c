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
#include <include/inteng.h>

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

  DC_ReleasePtr( hDC );
//   EngArc(dc, LeftRect, TopRect, RightRect, BottomRect, UNIMPLEMENTED
//          XStartArc, YStartArc, XEndArc, YEndArc);
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
  DC		*dc = DC_HandleToPtr(hDC);
  SURFOBJ	*SurfObj = (SURFOBJ*)AccessUserObject(dc->Surface);
  BOOL ret;
  PPENOBJ   pen;
  PROSRGNDATA  reg;
#ifndef TODO
  RECT defaultrect;
#endif

  if(!dc) return FALSE;

  if(PATH_IsPathOpen(dc->w.path)) {
    ret = PATH_LineTo(hDC, XEnd, YEnd);
  } else {
  	pen = (PPENOBJ) GDIOBJ_LockObj(dc->w.hPen, GO_PEN_MAGIC);
	reg = (PROSRGNDATA)GDIOBJ_LockObj(dc->w.hGCClipRgn, GO_REGION_MAGIC);

	ASSERT( pen );
	// not yet implemented ASSERT( reg );
#ifndef TODO
  if (NULL == reg) {
    defaultrect.left = 0;
    defaultrect.top = 0;
    defaultrect.right = 640;
    defaultrect.bottom = 480;

    reg = &defaultrect;
  }
#endif

    /* Draw the line according to the DC origin */
    ret = IntEngLineTo(SurfObj,
                       NULL, // ClipObj
                       PenToBrushObj(dc, pen),
                       dc->w.DCOrgX + dc->w.CursPosX, dc->w.DCOrgY + dc->w.CursPosY,
                       dc->w.DCOrgX + XEnd,           dc->w.DCOrgY + YEnd,
                       reg, // Bounding rectangle
                       dc->w.ROPmode); // MIX

    GDIOBJ_UnlockObj( dc->w.hGCClipRgn, GO_REGION_MAGIC );
    GDIOBJ_UnlockObj( dc->w.hPen, GO_PEN_MAGIC);
  }
  if(ret) {
    dc->w.CursPosX = XEnd;
    dc->w.CursPosY = YEnd;
  }
  DC_ReleasePtr( hDC );
  return ret;
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
  return FALSE;
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
  SURFOBJ	*SurfObj = (SURFOBJ*)AccessUserObject(dc->Surface);
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
	                 NULL,
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
