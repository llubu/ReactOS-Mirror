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
/* $Id: paint.c,v 1.19 2004/05/10 17:07:17 weiden Exp $
 * 
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           GDI Driver Paint Functions
 * FILE:              subsys/win32k/eng/paint.c
 * PROGRAMER:         Jason Filby
 * REVISION HISTORY:
 *                 3/7/1999: Created
 */

#include <w32k.h>

BOOL STDCALL FillSolid(SURFOBJ *Surface, PRECTL pRect, ULONG iColor)
{
  LONG y;
  ULONG LineWidth;
  SURFGDI *SurfaceGDI;

  SurfaceGDI = (SURFGDI*)AccessInternalObjectFromUserObject(Surface);
  MouseSafetyOnDrawStart(Surface, SurfaceGDI, pRect->left, pRect->top, pRect->right, pRect->bottom);
  LineWidth  = pRect->right - pRect->left;
  DPRINT(" LineWidth: %d, top: %d, bottom: %d\n", LineWidth, pRect->top, pRect->bottom);
  for (y = pRect->top; y < pRect->bottom; y++)
  {
    SurfaceGDI->DIB_HLine(Surface, pRect->left, pRect->right, y, iColor);
  }
  MouseSafetyOnDrawEnd(Surface, SurfaceGDI);

  return TRUE;
}

BOOL STDCALL
EngPaintRgn(SURFOBJ *Surface, CLIPOBJ *ClipRegion, ULONG iColor, MIX Mix,
               BRUSHINST *BrushInst, POINTL *BrushPoint)
{
  RECT_ENUM RectEnum;
  BOOL EnumMore;
  ULONG i;

  DPRINT("ClipRegion->iMode:%d, ClipRegion->iDComplexity: %d\n Color: %d", ClipRegion->iMode, ClipRegion->iDComplexity, iColor);
  switch(ClipRegion->iMode) {

    case TC_RECTANGLES:

    /* Rectangular clipping can be handled without enumeration.
       Note that trivial clipping is not possible, since the clipping
       region defines the area to fill */

    if (ClipRegion->iDComplexity == DC_RECT)
    {
      FillSolid(Surface, &(ClipRegion->rclBounds), iColor);
    } else {

      /* Enumerate all the rectangles and draw them */
      CLIPOBJ_cEnumStart(ClipRegion, FALSE, CT_RECTANGLES, CD_ANY, 0);

      do {
        EnumMore = CLIPOBJ_bEnum(ClipRegion, sizeof(RectEnum), (PVOID) &RectEnum);
        for (i = 0; i < RectEnum.c; i++) {
          FillSolid(Surface, RectEnum.arcl + i, iColor);
        }
      } while (EnumMore);
    }

    return(TRUE);

    default:
       return(FALSE);
  }
}

/*
 * @unimplemented
 */
BOOL STDCALL
EngPaint(IN SURFOBJ *Surface,
	 IN CLIPOBJ *ClipRegion,
	 IN BRUSHOBJ *Brush,
	 IN POINTL *BrushOrigin,
	 IN MIX  Mix)
{
  BOOLEAN ret;

  // FIXME: We only support a brush's solid color attribute
  ret = EngPaintRgn(Surface, ClipRegion, Brush->iSolidColor, Mix, NULL, BrushOrigin);

  return ret;
}

BOOL STDCALL
IntEngPaint(IN SURFOBJ *Surface,
	 IN CLIPOBJ *ClipRegion,
	 IN BRUSHOBJ *Brush,
	 IN POINTL *BrushOrigin,
	 IN MIX  Mix)
{
  SURFGDI *SurfGDI;
  BOOL ret;

  // Is the surface's Paint function hooked?
  SurfGDI = (SURFGDI*)AccessInternalObjectFromUserObject(Surface);

  DPRINT("SurfGDI type: %d, sgdi paint: %x\n", Surface->iType, SurfGDI->Paint);
  if((Surface->iType!=STYPE_BITMAP) && (SurfGDI->Paint!=NULL))
  {
    // Call the driver's DrvPaint
    MouseSafetyOnDrawStart(Surface, SurfGDI, ClipRegion->rclBounds.left,
	                         ClipRegion->rclBounds.top, ClipRegion->rclBounds.right,
							 ClipRegion->rclBounds.bottom);

    IntLockGDIDriver(SurfGDI);
    ret = SurfGDI->Paint(Surface, ClipRegion, Brush, BrushOrigin, Mix);
    IntUnLockGDIDriver(SurfGDI);
    MouseSafetyOnDrawEnd(Surface, SurfGDI);
    return ret;
  }
  return EngPaint( Surface, ClipRegion, Brush, BrushOrigin, Mix );

}
/* EOF */
