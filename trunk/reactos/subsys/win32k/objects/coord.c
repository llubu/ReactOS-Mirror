/* $Id: coord.c,v 1.8 2002/07/13 21:37:26 ei Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Coordinate systems
 * FILE:             subsys/win32k/objects/coord.c
 * PROGRAMER:        Unknown
 */

/* INCLUDES ******************************************************************/

#include <windows.h>
#include <ddk/ntddk.h>
#include <win32k/coord.h>
#include <win32k/dc.h>

//#define NDEBUG
#include <win32k/debug1.h>

/* FUNCTIONS *****************************************************************/

BOOL STDCALL W32kCombineTransform(LPXFORM  XFormResult,
                           CONST LPXFORM  xform1,
                           CONST LPXFORM  xform2)
{
  XFORM  xformTemp;

  /* Check for illegal parameters */
  if (!XFormResult || !xform1 || !xform2)
  {
    return  FALSE;
  }
  /* Create the result in a temporary XFORM, since xformResult may be
   * equal to xform1 or xform2 */
  xformTemp.eM11 = xform1->eM11 * xform2->eM11 + xform1->eM12 * xform2->eM21;
  xformTemp.eM12 = xform1->eM11 * xform2->eM12 + xform1->eM12 * xform2->eM22;
  xformTemp.eM21 = xform1->eM21 * xform2->eM11 + xform1->eM22 * xform2->eM21;
  xformTemp.eM22 = xform1->eM21 * xform2->eM12 + xform1->eM22 * xform2->eM22;
  xformTemp.eDx  = xform1->eDx  * xform2->eM11 + xform1->eDy  * xform2->eM21 + xform2->eDx;
  xformTemp.eDy  = xform1->eDx  * xform2->eM12 + xform1->eDy  * xform2->eM22 + xform2->eDy;

  /* Copy the result to xformResult */
  *XFormResult = xformTemp;

  return  TRUE;
}

VOID STATIC
CoordDPtoLP(PDC Dc, LPPOINT Point)
{
FLOAT x, y;
  x = (FLOAT)Point->x;
  y = (FLOAT)Point->y;
  Point->x = x * Dc->w.xformVport2World.eM11 +
    y * Dc->w.xformVport2World.eM21 + Dc->w.xformVport2World.eDx;
  Point->y = x * Dc->w.xformVport2World.eM12 +
    y * Dc->w.xformVport2World.eM22 + Dc->w.xformVport2World.eDy;
}

BOOL STDCALL
W32kDPtoLP(HDC  hDC,
	   LPPOINT  Points,
	   int  Count)
{
  PDC Dc;
  ULONG i;

  Dc = DC_HandleToPtr (hDC);
  if (Dc == NULL || !Dc->w.vport2WorldValid)
    {
      return(FALSE);
    }

  for (i = 0; i < Count; i++)
    {
      CoordDPtoLP(Dc, &Points[i]);
    }
  DC_ReleasePtr( hDC );
  return(TRUE);
}

int
STDCALL
W32kGetGraphicsMode(HDC  hDC)
{
  PDC  dc;
  int  GraphicsMode;

  dc = DC_HandleToPtr (hDC);
  if (!dc)
  {
    return  0;
  }

  GraphicsMode = dc->w.GraphicsMode;
  DC_ReleasePtr( hDC );
  return  GraphicsMode;
}

BOOL
STDCALL
W32kGetWorldTransform(HDC  hDC,
                      LPXFORM  XForm)
{
  PDC  dc;

  dc = DC_HandleToPtr (hDC);
  if (!dc)
  {
    return  FALSE;
  }
  if (!XForm)
  {
    return  FALSE;
  }
  *XForm = dc->w.xformWorld2Wnd;
  DC_ReleasePtr( hDC );
  return  TRUE;
}

VOID STATIC
CoordLPtoDP(PDC Dc, LPPOINT Point)
{
  FLOAT x, y;
  x = (FLOAT)Point->x;
  y = (FLOAT)Point->y;
  Point->x = x * Dc->w.xformWorld2Vport.eM11 +
    y * Dc->w.xformWorld2Vport.eM21 + Dc->w.xformWorld2Vport.eDx;
  Point->y = x * Dc->w.xformWorld2Vport.eM12 +
    y * Dc->w.xformWorld2Vport.eM22 + Dc->w.xformWorld2Vport.eDy;
}

BOOL STDCALL
W32kLPtoDP(HDC hDC, LPPOINT Points, INT Count)
{
  PDC Dc;
  ULONG i;

  Dc = DC_HandleToPtr (hDC);
  if (Dc == NULL)
    {
      return(FALSE);
    }

  for (i = 0; i < Count; i++)
    {
      CoordLPtoDP(Dc, &Points[i]);
    }
  DC_ReleasePtr( hDC );
  return(TRUE);
}

BOOL
STDCALL
W32kModifyWorldTransform(HDC  hDC,
                               CONST LPXFORM  XForm,
                               DWORD  Mode)
{
  PDC  dc;

  dc = DC_HandleToPtr (hDC);
  if (!dc)
  {
//    SetLastError( ERROR_INVALID_HANDLE );
    return  FALSE;
  }
  if (!XForm)
  {
    return FALSE;
  }

  /* Check that graphics mode is GM_ADVANCED */
  if (dc->w.GraphicsMode!=GM_ADVANCED)
  {
    return FALSE;
  }
  switch (Mode)
  {
    case MWT_IDENTITY:
      dc->w.xformWorld2Wnd.eM11 = 1.0f;
      dc->w.xformWorld2Wnd.eM12 = 0.0f;
      dc->w.xformWorld2Wnd.eM21 = 0.0f;
      dc->w.xformWorld2Wnd.eM22 = 1.0f;
      dc->w.xformWorld2Wnd.eDx  = 0.0f;
      dc->w.xformWorld2Wnd.eDy  = 0.0f;
      break;

    case MWT_LEFTMULTIPLY:
      W32kCombineTransform(&dc->w.xformWorld2Wnd, XForm, &dc->w.xformWorld2Wnd );
      break;

    case MWT_RIGHTMULTIPLY:
      W32kCombineTransform(&dc->w.xformWorld2Wnd, &dc->w.xformWorld2Wnd, XForm);
      break;

    default:
	  DC_ReleasePtr( hDC );
      return FALSE;
  }
  DC_UpdateXforms (dc);
  DC_ReleasePtr( hDC );
  return  TRUE;
}

BOOL
STDCALL
W32kOffsetViewportOrgEx(HDC  hDC,
                              int  XOffset,
                              int  YOffset,
                              LPPOINT  Point)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kOffsetWindowOrgEx(HDC  hDC,
                            int  XOffset,
                            int  YOffset,
                            LPPOINT  Point)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kScaleViewportExtEx(HDC  hDC,
                             int  Xnum,
                             int  Xdenom,
                             int  Ynum,
                             int  Ydenom,
                             LPSIZE  Size)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kScaleWindowExtEx(HDC  hDC,
                           int  Xnum,
                           int  Xdenom,
                           int  Ynum,
                           int  Ydenom,
                           LPSIZE  Size)
{
  UNIMPLEMENTED;
}

int
STDCALL
W32kSetGraphicsMode(HDC  hDC,
                         int  Mode)
{
  INT ret;
  DC *dc;

  dc = DC_HandleToPtr (hDC);
  if (!dc)
  {
    return 0;
  }

  /* One would think that setting the graphics mode to GM_COMPATIBLE
   * would also reset the world transformation matrix to the unity
   * matrix. However, in Windows, this is not the case. This doesn't
   * make a lot of sense to me, but that's the way it is.
   */

  if ((Mode != GM_COMPATIBLE) && (Mode != GM_ADVANCED))
  {
	DC_ReleasePtr( hDC );
    return 0;
  }
  ret = dc->w.GraphicsMode;
  dc->w.GraphicsMode = Mode;
  DC_ReleasePtr( hDC );
  return  ret;
}

int
STDCALL
W32kSetMapMode(HDC  hDC,
                    int  MapMode)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kSetViewportExtEx(HDC  hDC,
                           int  XExtent,
                           int  YExtent,
                           LPSIZE  Size)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kSetViewportOrgEx(HDC  hDC,
                           int  X,
                           int  Y,
                           LPPOINT  Point)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kSetWindowExtEx(HDC  hDC,
                         int  XExtent,
                         int  YExtent,
                         LPSIZE  Size)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kSetWindowOrgEx(HDC  hDC,
                         int  X,
                         int  Y,
                         LPPOINT  Point)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kSetWorldTransform(HDC  hDC,
                            CONST LPXFORM  XForm)
{
  PDC  dc;

  dc = DC_HandleToPtr (hDC);
  if (!dc)
  {
    return  FALSE;
  }
  if (!XForm)
  {
	DC_ReleasePtr( hDC );
    return  FALSE;
  }

  /* Check that graphics mode is GM_ADVANCED */
  if (dc->w.GraphicsMode != GM_ADVANCED)
  {
  	DC_ReleasePtr( hDC );
    return  FALSE;
  }
  dc->w.xformWorld2Wnd = *XForm;
  DC_UpdateXforms (dc);
  DC_ReleasePtr( hDC );
  return  TRUE;
}


