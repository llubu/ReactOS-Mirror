#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <win32k/bitmaps.h>
#include <win32k/debug.h>
#include <debug.h>
#include <ddk/winddi.h>
#include "../eng/objects.h"
#include "dib.h"

VOID DIB_24BPP_PutPixel(PSURFOBJ SurfObj, LONG x, LONG y, ULONG c)
{
  PBYTE byteaddr = SurfObj->pvBits + y * SurfObj->lDelta;
  PRGBTRIPLE addr = (PRGBTRIPLE)byteaddr + x;

  *(PULONG)(addr) = c;
}

ULONG DIB_24BPP_GetPixel(PSURFOBJ SurfObj, LONG x, LONG y)
{
  PBYTE byteaddr = SurfObj->pvBits + y * SurfObj->lDelta;
  PRGBTRIPLE addr = (PRGBTRIPLE)byteaddr + x;

  return *(PULONG)(addr) & 0x00ffffff;
}

VOID DIB_24BPP_HLine(PSURFOBJ SurfObj, LONG x1, LONG x2, LONG y, ULONG c)
{
  PBYTE byteaddr = SurfObj->pvBits + y * SurfObj->lDelta;
  PRGBTRIPLE addr = (PRGBTRIPLE)byteaddr + x1;
  LONG cx = x1;

  while(cx < x2) {
    *(PULONG)(addr) = c;
    ++addr;
    ++cx;
  }
}

VOID DIB_24BPP_VLine(PSURFOBJ SurfObj, LONG x, LONG y1, LONG y2, ULONG c)
{
  PBYTE byteaddr = SurfObj->pvBits + y1 * SurfObj->lDelta;
  PRGBTRIPLE addr = (PRGBTRIPLE)byteaddr + x;
  LONG lDelta = SurfObj->lDelta;

  byteaddr = (PBYTE)addr;
  while(y1++ < y2) {
    *(PULONG)(addr) = c;

    byteaddr += lDelta;
    addr = (PRGBTRIPLE)byteaddr;
  }
}

VOID DIB_24BPP_BltTo_24BPP(PSURFOBJ dstpsd, LONG dstx, LONG dsty, LONG w, LONG h,
  PSURFOBJ srcpsd, LONG srcx, LONG srcy)
{
  PRGBTRIPLE  dst;
  PRGBTRIPLE  src;
  PBYTE  bytedst;
  PBYTE  bytesrc;
  int  i;
  int  dlDelta = dstpsd->lDelta;
  int  slDelta = srcpsd->lDelta;

  bytedst = (char *)dstpsd->pvBits + dsty * dlDelta;
  bytesrc = (char *)srcpsd->pvBits + srcy * slDelta;
  dst = (PRGBTRIPLE)bytedst + dstx;
  src = (PRGBTRIPLE)bytesrc + srcx;

  while(--h >= 0) {
    PRGBTRIPLE  d = dst;
    PRGBTRIPLE  s = src;
    LONG  dx = dstx;
    LONG  sx = srcx;
    for(i=0; i<w; ++i) {
      *d = *s;
      ++d;
      ++s;
    }
    dst += dlDelta;
    src += slDelta;
  }
}

BOOLEAN DIB_To_24BPP_Bitblt(  SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
        SURFGDI *DestGDI,  SURFGDI *SourceGDI,
        PRECTL  DestRect,  POINTL  *SourcePoint,
        LONG    Delta,     XLATEOBJ *ColorTranslation)
{
  LONG     i, j, sx, sy, xColor, f1;
  PBYTE    DestBits, DestLine;
  PBYTE    SourceBits_4BPP, SourceLine_4BPP;
  PWORD    SourceBits_16BPP, SourceLine_16BPP;

  DestBits = DestSurf->pvBits + (DestRect->top * DestSurf->lDelta) + DestRect->left * 3;

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
      SourceBits_4BPP = SourceSurf->pvBits + (SourcePoint->y * SourceSurf->lDelta) + SourcePoint->x;

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

    case 16:
      SourceBits_16BPP = SourceSurf->pvBits + (SourcePoint->y * SourceSurf->lDelta) + 2 * SourcePoint->x;

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

    default:
      DbgPrint("DIB_24BPP_Bitblt: Unhandled Source BPP: %u\n", SourceGDI->BitsPerPixel);
      return FALSE;
  }

  return TRUE;
}
