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
/* $Id: dib32bpp.c,v 1.2 2003/05/18 17:16:17 ea Exp $ */
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
DIB_32BPP_PutPixel(PSURFOBJ SurfObj, LONG x, LONG y, ULONG c)
{
  PBYTE byteaddr = SurfObj->pvScan0 + y * SurfObj->lDelta;
  PDWORD addr = (PDWORD)byteaddr + x;

  *addr = c;
}

ULONG
DIB_32BPP_GetPixel(PSURFOBJ SurfObj, LONG x, LONG y)
{
  PBYTE byteaddr = SurfObj->pvScan0 + y * SurfObj->lDelta;
  PDWORD addr = (PDWORD)byteaddr + x;

  return (ULONG)(*addr);
}

VOID
DIB_32BPP_HLine(PSURFOBJ SurfObj, LONG x1, LONG x2, LONG y, ULONG c)
{
  PBYTE byteaddr = SurfObj->pvScan0 + y * SurfObj->lDelta;
  PDWORD addr = (PDWORD)byteaddr + x1;
  LONG cx = x1;

  while(cx < x2) {
    *addr = (DWORD)c;
    ++addr;
    ++cx;
  }
}

VOID
DIB_32BPP_VLine(PSURFOBJ SurfObj, LONG x, LONG y1, LONG y2, ULONG c)
{
  PBYTE byteaddr = SurfObj->pvScan0 + y1 * SurfObj->lDelta;
  PDWORD addr = (PDWORD)byteaddr + x;
  LONG lDelta = SurfObj->lDelta / sizeof(DWORD);

  byteaddr = (PBYTE)addr;
  while(y1++ < y2) {
    *addr = (DWORD)c;

    addr += lDelta;
  }
}

BOOLEAN
DIB_32BPP_BitBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                         SURFGDI *DestGDI,  SURFGDI *SourceGDI,
                         PRECTL  DestRect,  POINTL  *SourcePoint,
                         XLATEOBJ *ColorTranslation)
{
  ULONG     i, j, sx, sy, xColor, f1;
  PBYTE    SourceBits, DestBits, SourceLine, DestLine;
  PBYTE    SourceBits_4BPP, SourceLine_4BPP;

  DestBits = DestSurf->pvScan0 + (DestRect->top * DestSurf->lDelta) + 4 * DestRect->left;

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
            DIB_32BPP_PutPixel(DestSurf, i, j, XLATEOBJ_iXlate(ColorTranslation, 0));
          } else {
            DIB_32BPP_PutPixel(DestSurf, i, j, XLATEOBJ_iXlate(ColorTranslation, 1));
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
          DIB_32BPP_PutPixel(DestSurf, i, j, xColor);
          if(f1 == 1) { SourceLine_4BPP++; f1 = 0; } else { f1 = 1; }
          sx++;
        }

        SourceBits_4BPP += SourceSurf->lDelta;
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
          xColor = *SourceBits;
          *((PDWORD) DestBits) = (DWORD)XLATEOBJ_iXlate(ColorTranslation, xColor);
          SourceBits += 1;
	  DestBits += 4;
        }

        SourceLine += SourceSurf->lDelta;
        DestLine += DestSurf->lDelta;
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
          *((PDWORD) DestBits) = (DWORD)XLATEOBJ_iXlate(ColorTranslation, xColor);
          SourceBits += 2;
	  DestBits += 4;
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
          *((PDWORD)DestBits) = (DWORD)XLATEOBJ_iXlate(ColorTranslation, xColor);
          SourceBits += 3;
	  DestBits += 4;
        }

        SourceLine += SourceSurf->lDelta;
        DestLine += DestSurf->lDelta;
      }
      break;

    case 32:
      if (NULL == ColorTranslation || 0 != (ColorTranslation->flXlate & XO_TRIVIAL))
      {
	SourceBits = SourceSurf->pvScan0 + (SourcePoint->y * SourceSurf->lDelta) + 4 * SourcePoint->x;
	for (j = DestRect->top; j < DestRect->bottom; j++)
	{
	  RtlCopyMemory(DestBits, SourceBits, 4 * (DestRect->right - DestRect->left));
	  SourceBits += SourceSurf->lDelta;
	  DestBits += DestSurf->lDelta;
	}
      }
      else
      {
	/* FIXME */
	DPRINT1("DIB_32BPP_Bitblt: Unhandled ColorTranslation for 32 -> 32 copy");
        return FALSE;
      }
      break;

    default:
      DbgPrint("DIB_32BPP_Bitblt: Unhandled Source BPP: %u\n", SourceGDI->BitsPerPixel);
      return FALSE;
  }

  return TRUE;
}
/* EOF */
