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
/* $Id: dib24bpp.c,v 1.13 2003/08/12 21:55:47 gvg Exp $ */
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
DIB_24BPP_PutPixel(PSURFOBJ SurfObj, LONG x, LONG y, ULONG c)
{
  PBYTE byteaddr = SurfObj->pvScan0 + y * SurfObj->lDelta;
  PRGBTRIPLE addr = (PRGBTRIPLE)byteaddr + x;

  *(PULONG)(addr) = c;
}

ULONG
DIB_24BPP_GetPixel(PSURFOBJ SurfObj, LONG x, LONG y)
{
  PBYTE byteaddr = SurfObj->pvScan0 + y * SurfObj->lDelta;
  PRGBTRIPLE addr = (PRGBTRIPLE)byteaddr + x;

  return *(PULONG)(addr) & 0x00ffffff;
}

VOID
DIB_24BPP_HLine(PSURFOBJ SurfObj, LONG x1, LONG x2, LONG y, ULONG c)
{
  PBYTE byteaddr = SurfObj->pvScan0 + y * SurfObj->lDelta;
  PRGBTRIPLE addr = (PRGBTRIPLE)byteaddr + x1;
  LONG cx = x1;

  while(cx < x2) {
    *(PULONG)(addr) = c;
    ++addr;
    ++cx;
  }
}

VOID
DIB_24BPP_VLine(PSURFOBJ SurfObj, LONG x, LONG y1, LONG y2, ULONG c)
{
  PBYTE byteaddr = SurfObj->pvScan0 + y1 * SurfObj->lDelta;
  PRGBTRIPLE addr = (PRGBTRIPLE)byteaddr + x;
  LONG lDelta = SurfObj->lDelta;

  byteaddr = (PBYTE)addr;
  while(y1++ < y2) {
    *(PULONG)(addr) = c;

    byteaddr += lDelta;
    addr = (PRGBTRIPLE)byteaddr;
  }
}

BOOLEAN
DIB_24BPP_BitBltSrcCopy(  SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
		          SURFGDI *DestGDI,  SURFGDI *SourceGDI,
		          PRECTL  DestRect,  POINTL  *SourcePoint,
		          XLATEOBJ *ColorTranslation)
{
  LONG     i, j, sx, sy, xColor, f1;
  PBYTE    SourceBits, DestBits, SourceLine, DestLine;
  PBYTE    SourceBits_4BPP, SourceLine_4BPP;
  PWORD    SourceBits_16BPP, SourceLine_16BPP;

  DestBits = DestSurf->pvScan0 + (DestRect->top * DestSurf->lDelta) + DestRect->left * 3;

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
            DIB_24BPP_PutPixel(DestSurf, i, j, XLATEOBJ_iXlate(ColorTranslation, 0));
          } else {
            DIB_24BPP_PutPixel(DestSurf, i, j, XLATEOBJ_iXlate(ColorTranslation, 1));
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
        DestLine = DestBits;
        sx = SourcePoint->x;
        f1 = sx & 1;

        for (i=DestRect->left; i<DestRect->right; i++)
        {
          xColor = XLATEOBJ_iXlate(ColorTranslation,
              (*SourceLine_4BPP & altnotmask[sx&1]) >> (4 * (1-(sx & 1))));
          *DestLine++ = xColor & 0xff;
          *(PWORD)DestLine = xColor >> 8;
          DestLine += 2;
          if(f1 == 1) { SourceLine_4BPP++; f1 = 0; } else { f1 = 1; }
          sx++;
        }

        SourceBits_4BPP += SourceSurf->lDelta;
        DestBits += DestSurf->lDelta;
      }
      break;

    case 8:
      SourceLine = SourceSurf->pvScan0 + (SourcePoint->y * SourceSurf->lDelta) + SourcePoint->x;
      DestLine = DestBits;

      for (j = DestRect->top; j < DestRect->bottom; j++)
      {
        SourceBits = SourceLine;
        DestBits = DestLine;

        for (i = DestRect->left; i < DestRect->right; i++)
        {
          xColor = XLATEOBJ_iXlate(ColorTranslation, *SourceBits);
          *DestBits = xColor & 0xff;
          *(PWORD)(DestBits + 1) = xColor >> 8;
          SourceBits += 1;
	  DestBits += 3;
        }

        SourceLine += SourceSurf->lDelta;
        DestLine += DestSurf->lDelta;
      }
      break;

    case 16:
      SourceBits_16BPP = SourceSurf->pvScan0 + (SourcePoint->y * SourceSurf->lDelta) + 2 * SourcePoint->x;

      for (j=DestRect->top; j<DestRect->bottom; j++)
      {
        SourceLine_16BPP = SourceBits_16BPP;
        DestLine = DestBits;

        for (i=DestRect->left; i<DestRect->right; i++)
        {
          xColor = XLATEOBJ_iXlate(ColorTranslation, *SourceLine_16BPP);
          *DestLine++ = xColor & 0xff;
          *(PWORD)DestLine = xColor >> 8;
          DestLine += 2;
          SourceLine_16BPP++;
        }

        SourceBits_16BPP = (PWORD)((PBYTE)SourceBits_16BPP + SourceSurf->lDelta);
        DestBits += DestSurf->lDelta;
      }
      break;

    case 24:
      if (NULL == ColorTranslation || 0 != (ColorTranslation->flXlate & XO_TRIVIAL))
      {
	if (DestRect->top < SourcePoint->y)
	  {
	    SourceBits = SourceSurf->pvScan0 + (SourcePoint->y * SourceSurf->lDelta) + 3 * SourcePoint->x;
	    for (j = DestRect->top; j < DestRect->bottom; j++)
	      {
		RtlMoveMemory(DestBits, SourceBits, 3 * (DestRect->right - DestRect->left));
		SourceBits += SourceSurf->lDelta;
		DestBits += DestSurf->lDelta;
	      }
	  }
	else
	  {
	    SourceBits = SourceSurf->pvScan0 + ((SourcePoint->y + DestRect->bottom - DestRect->top - 1) * SourceSurf->lDelta) + 3 * SourcePoint->x;
	    DestBits = DestSurf->pvScan0 + ((DestRect->bottom - 1) * DestSurf->lDelta) + 3 * DestRect->left;
	    for (j = DestRect->bottom - 1; DestRect->top <= j; j--)
	      {
		RtlMoveMemory(DestBits, SourceBits, 3 * (DestRect->right - DestRect->left));
		SourceBits -= SourceSurf->lDelta;
		DestBits -= DestSurf->lDelta;
	      }
	  }
      }
      else
      {
	/* FIXME */
	DPRINT1("DIB_24BPP_Bitblt: Unhandled ColorTranslation for 16 -> 16 copy");
        return FALSE;
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
          xColor = XLATEOBJ_iXlate(ColorTranslation, *((PDWORD) SourceBits));
          *DestBits = xColor & 0xff;
          *(PWORD)(DestBits + 1) = xColor >> 8;
          SourceBits += 4;
	  DestBits += 3;
        }

        SourceLine += SourceSurf->lDelta;
        DestLine += DestSurf->lDelta;
      }
      break;

    default:
      DbgPrint("DIB_24BPP_Bitblt: Unhandled Source BPP: %u\n", SourceGDI->BitsPerPixel);
      return FALSE;
  }

  return TRUE;
}

BOOLEAN
DIB_24BPP_BitBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
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

  if (Rop4 == SRCCOPY)
    {
      return(DIB_24BPP_BitBltSrcCopy(DestSurf, SourceSurf, DestGDI, SourceGDI, DestRect, SourcePoint, ColorTranslation));
    }
  else
    {
      sy = SourcePoint->y;

      for (j = DestRect->top; j < DestRect->bottom; j++)
      {
        sx = SourcePoint->x;
	DestBits = (PULONG)(DestSurf->pvScan0 + 3 * DestRect->left + j * DestSurf->lDelta);
        for (i=DestRect->left; i<DestRect->right; i++, DestBits++)
	  {
	    Dest = *DestBits & 0x00ffffff;
	    if (UsesSource)
	      {
		Source = DIB_GetSource(SourceSurf, SourceGDI, sx + i + k, sy, ColorTranslation) & 0x00ffffff;
	      }
	    if (UsesPattern)
	      {
		/* FIXME: No support for pattern brushes. */
		Pattern = Brush->iSolidColor;
	      }
	    Dest = DIB_DoRop(Rop4, Dest, Source, Pattern);	    
	    *(PBYTE)DestBits = Dest & 0xff;
            *(PWORD)(DestBits + 1) = Dest >> 8;
	  }
      }
    }
  return TRUE;
}

/* EOF */
