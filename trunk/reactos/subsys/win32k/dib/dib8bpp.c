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
/* $Id: dib8bpp.c,v 1.6 2003/08/31 07:56:24 gvg Exp $ */
#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <win32k/bitmaps.h>
#include <win32k/debug.h>
#include <debug.h>
#include <ddk/winddi.h>
#include "../eng/objects.h"
#include "dib.h"

VOID
DIB_8BPP_PutPixel(PSURFOBJ SurfObj, LONG x, LONG y, ULONG c)
{
  PBYTE byteaddr = SurfObj->pvScan0 + y * SurfObj->lDelta + x;

  *byteaddr = c;
}

ULONG
DIB_8BPP_GetPixel(PSURFOBJ SurfObj, LONG x, LONG y)
{
  PBYTE byteaddr = SurfObj->pvScan0 + y * SurfObj->lDelta + x;

  return (ULONG)(*byteaddr);
}

VOID
DIB_8BPP_HLine(PSURFOBJ SurfObj, LONG x1, LONG x2, LONG y, ULONG c)
{
  PBYTE byteaddr = SurfObj->pvScan0 + y * SurfObj->lDelta;
  PBYTE addr = byteaddr + x1;
  LONG cx = x1;

  while(cx < x2) {
    *addr = c;
    ++addr;
    ++cx;
  }
}

VOID
DIB_8BPP_VLine(PSURFOBJ SurfObj, LONG x, LONG y1, LONG y2, ULONG c)
{
  PBYTE byteaddr = SurfObj->pvScan0 + y1 * SurfObj->lDelta;
  PBYTE addr = byteaddr + x;
  LONG lDelta = SurfObj->lDelta;

  byteaddr = addr;
  while(y1++ < y2) {
    *addr = c;

    addr += lDelta;
  }
}

BOOLEAN
DIB_8BPP_BitBltSrcCopy(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
		       SURFGDI *DestGDI,  SURFGDI *SourceGDI,
		       PRECTL  DestRect,  POINTL  *SourcePoint,
		       XLATEOBJ *ColorTranslation)
{
  LONG     i, j, sx, sy, xColor, f1;
  PBYTE    SourceBits, DestBits, SourceLine, DestLine;
  PBYTE    SourceBits_4BPP, SourceLine_4BPP;

  DestBits = DestSurf->pvScan0 + (DestRect->top * DestSurf->lDelta) + DestRect->left;

  switch(SourceGDI->BitsPerPixel)
  {
    case 1:
      sx = SourcePoint->x;
      sy = SourcePoint->y;

      for (j=DestRect->top; j<DestRect->bottom; j++)
      {
        sx = SourcePoint->x;
        for (i=DestRect->left; i<DestRect->right; i++)
        {
          if(DIB_1BPP_GetPixel(SourceSurf, sx, sy) == 0)
          {
            DIB_8BPP_PutPixel(DestSurf, i, j, XLATEOBJ_iXlate(ColorTranslation, 0));
          } else {
            DIB_8BPP_PutPixel(DestSurf, i, j, XLATEOBJ_iXlate(ColorTranslation, 1));
          }
          sx++;
        }
        sy++;
      }
      break;

    case 4:
      SourceBits_4BPP = SourceSurf->pvScan0 + (SourcePoint->y * SourceSurf->lDelta) + (SourcePoint->x >> 1);

      for (j=DestRect->top; j<DestRect->bottom; j++)
      {
        SourceLine_4BPP = SourceBits_4BPP;
        sx = SourcePoint->x;
        f1 = sx & 1;

        for (i=DestRect->left; i<DestRect->right; i++)
        {
          xColor = XLATEOBJ_iXlate(ColorTranslation,
              (*SourceLine_4BPP & altnotmask[sx&1]) >> (4 * (1-(sx & 1))));
          DIB_8BPP_PutPixel(DestSurf, i, j, xColor);
          if(f1 == 1) { SourceLine_4BPP++; f1 = 0; } else { f1 = 1; }
          sx++;
        }

        SourceBits_4BPP += SourceSurf->lDelta;
      }
      break;

    case 8:
      if (NULL == ColorTranslation || 0 != (ColorTranslation->flXlate & XO_TRIVIAL))
      {
	if (DestRect->top < SourcePoint->y)
	  {
	    SourceBits = SourceSurf->pvScan0 + (SourcePoint->y * SourceSurf->lDelta) + SourcePoint->x;
	    for (j = DestRect->top; j < DestRect->bottom; j++)
	      {
		RtlMoveMemory(DestBits, SourceBits, DestRect->right - DestRect->left);
		SourceBits += SourceSurf->lDelta;
		DestBits += DestSurf->lDelta;
	      }
	  }
	else
	  {
	    SourceBits = SourceSurf->pvScan0 + ((SourcePoint->y + DestRect->bottom - DestRect->top - 1) * SourceSurf->lDelta) + SourcePoint->x;
	    DestBits = DestSurf->pvScan0 + ((DestRect->bottom - 1) * DestSurf->lDelta) + DestRect->left;
	    for (j = DestRect->bottom - 1; DestRect->top <= j; j--)
	      {
		RtlMoveMemory(DestBits, SourceBits, DestRect->right - DestRect->left);
		SourceBits -= SourceSurf->lDelta;
		DestBits -= DestSurf->lDelta;
	      }
	  }
      }
      else
      {
	if (DestRect->top < SourcePoint->y)
	  {
	    SourceLine = SourceSurf->pvScan0 + (SourcePoint->y * SourceSurf->lDelta) + SourcePoint->x;
	    DestLine = DestBits;
	    for (j = DestRect->top; j < DestRect->bottom; j++)
	      {
		SourceBits = SourceLine;
		DestBits = DestLine;
		for (i=DestRect->left; i<DestRect->right; i++)
		  {
		    *DestBits++ = XLATEOBJ_iXlate(ColorTranslation, *SourceBits++);
		  }
		SourceLine += SourceSurf->lDelta;
		DestLine += DestSurf->lDelta;
	      }
	  }
	else
	  {
	    SourceLine = SourceSurf->pvScan0 + ((SourcePoint->y + DestRect->bottom - DestRect->top - 1) * SourceSurf->lDelta) + SourcePoint->x;
	    DestLine = DestSurf->pvScan0 + ((DestRect->bottom - 1) * DestSurf->lDelta) + DestRect->left;
	    for (j = DestRect->bottom - 1; DestRect->top <= j; j--)
	      {
		SourceBits = SourceLine;
		DestBits = DestLine;
		for (i=DestRect->left; i<DestRect->right; i++)
		  {
		    *DestBits++ = XLATEOBJ_iXlate(ColorTranslation, *SourceBits++);
		  }
		SourceLine -= SourceSurf->lDelta;
		DestLine -= DestSurf->lDelta;
	      }
	  }
      }
      break;

    case 16:
      SourceLine = SourceSurf->pvScan0 + (SourcePoint->y * SourceSurf->lDelta) + 2 * SourcePoint->x;
      DestLine = DestBits;

      for (j = DestRect->top; j < DestRect->bottom; j++)
      {
        SourceBits = SourceLine;
        DestBits = DestLine;

        for (i = DestRect->left; i < DestRect->right; i++)
        {
          xColor = *((PWORD) SourceBits);
          *DestBits = XLATEOBJ_iXlate(ColorTranslation, xColor);
          SourceBits += 2;
	  DestBits += 1;
        }

        SourceLine += SourceSurf->lDelta;
        DestLine += DestSurf->lDelta;
      }
      break;

    case 24:
      SourceLine = SourceSurf->pvScan0 + (SourcePoint->y * SourceSurf->lDelta) + 3 * SourcePoint->x;
      DestLine = DestBits;

      for (j = DestRect->top; j < DestRect->bottom; j++)
      {
        SourceBits = SourceLine;
        DestBits = DestLine;

        for (i = DestRect->left; i < DestRect->right; i++)
        {
          xColor = (*(SourceBits + 2) << 0x10) +
             (*(SourceBits + 1) << 0x08) +
             (*(SourceBits));
          *DestBits = XLATEOBJ_iXlate(ColorTranslation, xColor);
          SourceBits += 3;
	  DestBits += 1;
        }

        SourceLine += SourceSurf->lDelta;
        DestLine += DestSurf->lDelta;
      }
      break;

    case 32:
      SourceLine = SourceSurf->pvScan0 + (SourcePoint->y * SourceSurf->lDelta) + 4 * SourcePoint->x;
      DestLine = DestBits;

      for (j = DestRect->top; j < DestRect->bottom; j++)
      {
        SourceBits = SourceLine;
        DestBits = DestLine;

        for (i = DestRect->left; i < DestRect->right; i++)
        {
          xColor = *((PDWORD) SourceBits);
          *DestBits = XLATEOBJ_iXlate(ColorTranslation, xColor);
          SourceBits += 4;
	  DestBits += 1;
        }

        SourceLine += SourceSurf->lDelta;
        DestLine += DestSurf->lDelta;
      }
      break;

    default:
      DbgPrint("DIB_8BPP_Bitblt: Unhandled Source BPP: %u\n", SourceGDI->BitsPerPixel);
      return FALSE;
  }

  return TRUE;
}

BOOLEAN
DIB_8BPP_BitBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
		 SURFGDI *DestGDI,  SURFGDI *SourceGDI,
		 PRECTL  DestRect,  POINTL  *SourcePoint,
		 PBRUSHOBJ Brush, PPOINTL BrushOrigin,
		 XLATEOBJ *ColorTranslation, ULONG Rop4)
{
  LONG     i, j, k, sx, sy;
  ULONG    Dest, Source, Pattern;
  PULONG   DestBits;
  BOOL     UsesSource = ((Rop4 & 0xCC0000) >> 2) != (Rop4 & 0x330000);
  BOOL     UsesPattern = ((Rop4 & 0xF00000) >> 4) != (Rop4 & 0x0F0000);  
  LONG     RoundedRight = DestRect->right - (DestRect->right & 0x3);

  if (Rop4 == SRCCOPY)
    {
      return(DIB_8BPP_BitBltSrcCopy(DestSurf, SourceSurf, DestGDI, SourceGDI, DestRect, SourcePoint, ColorTranslation));
    }
  else
    {
      sy = SourcePoint->y;

      for (j=DestRect->top; j<DestRect->bottom; j++)
      {
        sx = SourcePoint->x;
	DestBits = (PULONG)(DestSurf->pvScan0 + DestRect->left + j * DestSurf->lDelta);
        for (i=DestRect->left; i<RoundedRight; i+=4, DestBits++)
	  {
	    Dest = *DestBits;
	    if (UsesSource)
	      {
		Source = 0;
		for (k = 0; k < 4; k++)
		  {
		    Source |= (DIB_GetSource(SourceSurf, SourceGDI, sx + i + k, sy, ColorTranslation) << (k * 8));
		  }
	      }
	    if (UsesPattern)
	      {
		/* FIXME: No support for pattern brushes. */
		Pattern = (Brush->iSolidColor & 0xFF) |
                          ((Brush->iSolidColor & 0xFF) << 8) |
                          ((Brush->iSolidColor & 0xFF) << 16) |
                          ((Brush->iSolidColor & 0xFF) << 24);
	      }
	    *DestBits = DIB_DoRop(Rop4, Dest, Source, Pattern);	    
	  }
	if (i < DestRect->right)
	  {
	    Dest = *DestBits;
	    for (; i < DestRect->right; i++)
	      {
		if (UsesSource)
		  {
		    Source = DIB_GetSource(SourceSurf, SourceGDI, sx + i, sy, ColorTranslation);
		  }
		if (UsesPattern)
		  {
		    /* FIXME: No support for pattern brushes. */
		    Pattern = (Brush->iSolidColor & 0xFF) |
                              ((Brush->iSolidColor & 0xFF) << 8) |
                              ((Brush->iSolidColor & 0xFF) << 16) |
                              ((Brush->iSolidColor & 0xFF) << 24);
		  }				
		DIB_8BPP_PutPixel(DestSurf, i, j, DIB_DoRop(Rop4, Dest, Source, Pattern) & 0xFFFF);
		Dest >>= 8;
	      }	 
	  }
      }
    }
  return TRUE;
}

/* EOF */
