#include <ntddk.h>
#define NDEBUG
#include <debug.h>
#include "../vgaddi.h"
#include "../vgavideo/vgavideo.h"
#include "brush.h"
#include "bitblt.h"

typedef BOOL (*PFN_VGABlt)(SURFOBJ*, SURFOBJ*, XLATEOBJ*, RECTL*, POINTL*);
typedef BOOL STDCALL (*PBLTRECTFUNC)(PSURFOBJ OutputObj,
                                     PSURFOBJ InputObj,
                                     PSURFOBJ Mask,
                                     PXLATEOBJ ColorTranslation,
                                     PRECTL OutputRect,
                                     PPOINTL InputPoint,
                                     PPOINTL MaskOrigin,
                                     PBRUSHOBJ Brush,
                                     PPOINTL BrushOrigin,
                                     ROP4 Rop4);

static BOOL FASTCALL VGADDI_IntersectRect(PRECTL prcDst, PRECTL prcSrc1, PRECTL prcSrc2)
{
  static const RECTL rclEmpty = { 0, 0, 0, 0 };

  prcDst->left  = max(prcSrc1->left, prcSrc2->left);
  prcDst->right = min(prcSrc1->right, prcSrc2->right);

  if (prcDst->left < prcDst->right)
  {
    prcDst->top    = max(prcSrc1->top, prcSrc2->top);
    prcDst->bottom = min(prcSrc1->bottom, prcSrc2->bottom);

    if (prcDst->top < prcDst->bottom) return(TRUE);
  }

  *prcDst = rclEmpty;

  return(FALSE);
}

BOOL
DIBtoVGA(SURFOBJ *Dest, SURFOBJ *Source, XLATEOBJ *ColorTranslation,
	 RECTL *DestRect, POINTL *SourcePoint)
{
  LONG dx, dy;

  dx = DestRect->right  - DestRect->left;
  dy = DestRect->bottom - DestRect->top;

  if (NULL == ColorTranslation || 0 != (ColorTranslation->flXlate & XO_TRIVIAL))
    {
      DIB_BltToVGA(DestRect->left, DestRect->top, dx, dy,
                   Source->pvScan0 + SourcePoint->y * Source->lDelta + (SourcePoint->x >> 1),
		   Source->lDelta);
    }
  else
    {
      /* Perform color translation */
      DIB_BltToVGAWithXlate(DestRect->left, DestRect->top, dx, dy,
                            Source->pvScan0 + SourcePoint->y * Source->lDelta + (SourcePoint->x >> 1),
		            Source->lDelta, ColorTranslation);
    }
}

BOOL 
VGAtoDIB(SURFOBJ *Dest, SURFOBJ *Source, XLATEOBJ *ColorTranslation,
	 RECTL *DestRect, POINTL *SourcePoint)
{
  LONG i, j, dx, dy, RGBulong;
  BYTE  *GDIpos, *initial, idxColor;
  
  // Used by the temporary DFB
  PDEVSURF	TargetSurf;
  DEVSURF	DestDevSurf;
  PSURFOBJ	TargetBitmapSurf;
  HBITMAP	hTargetBitmap;
  SIZEL		InterSize;
  POINTL	ZeroPoint;

  // FIXME: Optimize to retrieve entire bytes at a time (see /display/vgavideo/vgavideo.c:vgaGetByte)

  GDIpos = Dest->pvBits /* + (DestRect->top * Dest->lDelta) + (DestRect->left >> 1) */ ;
  dx = DestRect->right  - DestRect->left;
  dy = DestRect->bottom - DestRect->top;

  if(ColorTranslation == NULL)
  {
    // Prepare a Dest Dev Target and copy from the DFB to the DIB
    DestDevSurf.NextScan = Dest->lDelta;
    DestDevSurf.StartBmp = Dest->pvScan0;

    DIB_BltFromVGA(SourcePoint->x, SourcePoint->y, dx, dy, Dest->pvBits, Dest->lDelta);

  } else {
    // Color translation
    for(j=SourcePoint->y; j<SourcePoint->y+dy; j++)
    {
       initial = GDIpos;
       for(i=SourcePoint->x; i<SourcePoint->x+dx; i++)
       {
         *GDIpos = XLATEOBJ_iXlate(ColorTranslation, vgaGetPixel(i, j));
         GDIpos+=1;
       }
       GDIpos = initial + Dest->lDelta;
    }
  }
}

BOOL 
DFBtoVGA(SURFOBJ *Dest, SURFOBJ *Source, XLATEOBJ *ColorTranslation,
	 RECTL *DestRect, POINTL *SourcePoint)
{
  // Do DFBs need color translation??
}

BOOL
VGAtoDFB(SURFOBJ *Dest, SURFOBJ *Source, XLATEOBJ *ColorTranslation,
	 RECTL *DestRect, POINTL *SourcePoint)
{
  // Do DFBs need color translation??
}

BOOL
VGAtoVGA(SURFOBJ *Dest, SURFOBJ *Source, XLATEOBJ *ColorTranslation,
	 RECTL *DestRect, POINTL *SourcePoint)
{
  LONG i, i2, j, dx, dy, alterx, altery;
  //LARGE_INTEGER Start, End; // for performance measurement only
  static char buf[640];

  // Calculate deltas

  dx = DestRect->right  - DestRect->left;
  dy = DestRect->bottom - DestRect->top;

  alterx = DestRect->left - SourcePoint->x;
  altery = DestRect->top - SourcePoint->y;

  //KeQueryTickCount ( &Start );

  i = SourcePoint->x;
  i2 = i + alterx;

  if (SourcePoint->y >= DestRect->top)
  {
    for(j=SourcePoint->y; j<SourcePoint->y+dy; j++)
      {
	LONG j2 = j + altery;
	vgaReadScan  ( i,  j,  dx, buf );
	vgaWriteScan ( i2, j2, dx, buf );
      }
  }
  else
  {
    for(j=(SourcePoint->y+dy-1); j>=SourcePoint->y; j--)
      {
	LONG j2 = j + altery;
	vgaReadScan  ( i,  j,  dx, buf );
	vgaWriteScan ( i2, j2, dx, buf );
      }
  }

  //KeQueryTickCount ( &End );
  //DbgPrint ( "VgaBitBlt timing: %lu\n", (ULONG)(End.QuadPart-Start.QuadPart) );

  return TRUE;
}

BOOL STDCALL
VGADDI_BltBrush(PSURFOBJ Dest, PSURFOBJ Source, PSURFOBJ MaskSurf,
                PXLATEOBJ ColorTranslation, PRECT DestRect,
                PPOINTL SourcePoint, PPOINTL MaskPoint,
		PBRUSHOBJ Brush, PPOINTL BrushPoint, ROP4 Rop4)
{
  UCHAR SolidColor = 0;
  ULONG Left;
  ULONG Right;
  ULONG Length;
  PUCHAR Video;
  UCHAR Mask;
  ULONG i, j;
  ULONG RasterOp = VGA_NORMAL;

  /* Punt brush blts to non-device surfaces. */
  if (Dest->iType != STYPE_DEVICE)
    {
      return(FALSE);
    }

  /* Punt pattern fills. */
  if ((Rop4 == PATCOPY || Rop4 == PATINVERT) && 
      Brush->iSolidColor == 0xFFFFFFFF)
    {
      return(FALSE);
    }

  /* Get the brush colour. */
  switch (Rop4)
    {
    case PATCOPY: SolidColor = Brush->iSolidColor; break;
    case PATINVERT: SolidColor = Brush->iSolidColor; RasterOp = VGA_XOR; break;
    case WHITENESS: SolidColor = 0xF; break;
    case BLACKNESS: SolidColor = 0x0; break;
    case DSTINVERT: SolidColor = 0xF; RasterOp = VGA_XOR; break;
    }

  /* Select write mode 3. */
  WRITE_PORT_UCHAR((PUCHAR)GRA_I, 0x05);
  WRITE_PORT_UCHAR((PUCHAR)GRA_D, 0x03);

  /* Setup set/reset register. */
  WRITE_PORT_UCHAR((PUCHAR)GRA_I, 0x00);
  WRITE_PORT_UCHAR((PUCHAR)GRA_D, (UCHAR)SolidColor);

  /* Enable writes to all pixels. */
  WRITE_PORT_UCHAR((PUCHAR)GRA_I, 0x08);
  WRITE_PORT_UCHAR((PUCHAR)GRA_D, 0xFF);

  /* Set up data rotate. */
  WRITE_PORT_UCHAR((PUCHAR)GRA_I, 0x03);
  WRITE_PORT_UCHAR((PUCHAR)GRA_D, RasterOp);
  
  /* Fill any pixels on the left which don't fall into a full row of eight. */
  if ((DestRect->left % 8) != 0)
    {
      /* Disable writes to pixels outside of the destination rectangle. */
      Mask = (1 << (8 - (DestRect->left % 8))) - 1;
      if ((DestRect->right - DestRect->left) < (8 - (DestRect->left % 8)))
	{
	  Mask &= ~((1 << (8 - (DestRect->right % 8))) - 1);
	}

      /* Write the same color to each pixel. */
      Video = (PUCHAR)vidmem + DestRect->top * 80 + (DestRect->left >> 3);
      for (i = DestRect->top; i < DestRect->bottom; i++, Video+=80)
	{
	  (VOID)READ_REGISTER_UCHAR(Video);
	  WRITE_REGISTER_UCHAR(Video, Mask);
	}

      /* Have we finished. */
      if ((DestRect->right - DestRect->left) < (8 - (DestRect->left % 8)))
	{
	  return(TRUE);
	}
    }    

  /* Fill any whole rows of eight pixels. */
  Left = (DestRect->left + 7) & ~0x7;
  Length = (DestRect->right >> 3) - (Left >> 3);
  for (i = DestRect->top; i < DestRect->bottom; i++)
    {
      Video = (PUCHAR)vidmem + i * 80 + (Left >> 3);
      for (j = 0; j < Length; j++, Video++)
	{
	  (VOID)READ_REGISTER_UCHAR(Video);
	  WRITE_REGISTER_UCHAR(Video, 0xFF);
	}
    }

  /* Fill any pixels on the right which don't fall into a complete row. */
  if ((DestRect->right % 8) != 0)
    {
      /* Disable writes to pixels outside the destination rectangle. */
      Mask = ~((1 << (8 - (DestRect->right % 8))) - 1);

      Video = (PUCHAR)vidmem + DestRect->top * 80 + (DestRect->right >> 3);
      for (i = DestRect->top; i < DestRect->bottom; i++, Video+=80)
	{
	  (VOID)READ_REGISTER_UCHAR(Video);
	  WRITE_REGISTER_UCHAR(Video, Mask);
	}
    }

  /* Restore write mode 2. */
  WRITE_PORT_UCHAR((PUCHAR)GRA_I, 0x05);
  WRITE_PORT_UCHAR((PUCHAR)GRA_D, 0x02);

  /* Set up data rotate. */
  WRITE_PORT_UCHAR((PUCHAR)GRA_I, 0x03);
  WRITE_PORT_UCHAR((PUCHAR)GRA_D, 0x00);

  return(TRUE);
}

BOOL STDCALL
VGADDI_BltSrc(PSURFOBJ Dest, PSURFOBJ Source, PSURFOBJ Mask,
              PXLATEOBJ ColorTranslation, PRECTL DestRect, PPOINTL SourcePoint,
              PPOINTL MaskOrigin, PBRUSHOBJ Brush, PPOINTL BrushOrigin, ROP4 Rop4)
{
  RECT_ENUM RectEnum;
  BOOL EnumMore;
  PFN_VGABlt  BltOperation;
  ULONG SourceType;

  SourceType = Source->iType;

  if (SourceType == STYPE_BITMAP && Dest->iType == STYPE_DEVICE)
    {
      BltOperation = DIBtoVGA;
    }
  else if (SourceType == STYPE_DEVICE && Dest->iType == STYPE_BITMAP)
    {
      BltOperation = VGAtoDIB;
    }
  else if (SourceType == STYPE_DEVICE && Dest->iType == STYPE_DEVICE)
    {
      BltOperation = VGAtoVGA;
    } 
  else if (SourceType == STYPE_DEVBITMAP && Dest->iType == STYPE_DEVICE)
    {
      BltOperation = DFBtoVGA;
    } 
  else if (SourceType == STYPE_DEVICE && Dest->iType == STYPE_DEVBITMAP)
    {
      BltOperation = VGAtoDFB;
    } 
  else
    {
      /* Punt blts not involving a device or a device-bitmap. */
      return(FALSE);
   }

  BltOperation(Dest, Source, ColorTranslation, DestRect, SourcePoint);
  return(TRUE);
}

BOOL STDCALL
VGADDI_BltMask(PSURFOBJ Dest, PSURFOBJ Source, PSURFOBJ Mask,
               PXLATEOBJ ColorTranslation, PRECTL DestRect,
               PPOINTL SourcePoint, PPOINTL MaskPoint, BRUSHOBJ* Brush,
	       PPOINTL BrushPoint, ROP4 Rop4)
{
  LONG i, j, dx, dy, idxColor, RGBulong = 0, c8;
  BYTE *initial, *tMask, *lMask;

  dx = DestRect->right  - DestRect->left;
  dy = DestRect->bottom - DestRect->top;

  if (ColorTranslation == NULL)
    {
      if (Mask != NULL)
	{
	  tMask = Mask->pvBits;
	  for (j=0; j<dy; j++)
	    {
	      lMask = tMask;
	      c8 = 0;
	      for (i=0; i<dx; i++)
		{
		  if((*lMask & maskbit[c8]) != 0)
		    {
		      vgaPutPixel(DestRect->left + i, DestRect->top + j, Brush->iSolidColor);
		    }
		  c8++;
		  if(c8 == 8) { lMask++; c8=0; }
		}
	      tMask += Mask->lDelta;
	    }
	}
    }
}

BOOL STDCALL
DrvBitBlt(SURFOBJ *Dest,
	  SURFOBJ *Source,
	  SURFOBJ *Mask,
	  CLIPOBJ *Clip,
	  XLATEOBJ *ColorTranslation,
	  RECTL *DestRect,
	  POINTL *SourcePoint,
	  POINTL *MaskPoint,
	  BRUSHOBJ *Brush,
	  POINTL *BrushPoint,
	  ROP4 rop4)
{
  PBLTRECTFUNC BltRectFunc;
  RECTL CombinedRect;
  BOOL Ret;
  RECT_ENUM RectEnum;
  BOOL EnumMore;
  unsigned i;
  
  switch (rop4)
    {
    case BLACKNESS:
    case PATCOPY:
    case WHITENESS:
    case PATINVERT:
    case DSTINVERT:
      BltRectFunc = VGADDI_BltBrush;
      break;

    case SRCCOPY:
      if (BMF_4BPP == Source->iBitmapFormat && BMF_4BPP == Dest->iBitmapFormat)
	{
	  BltRectFunc = VGADDI_BltSrc;
	}
      else
	{
	  return FALSE;
	}
      break;

    case 0xAACC:
      BltRectFunc = VGADDI_BltMask;
      break;

    default:
      return FALSE;
    }

  switch(NULL == Clip ? DC_TRIVIAL : Clip->iDComplexity)
  {
    case DC_TRIVIAL:
      Ret = (*BltRectFunc)(Dest, Source, Mask, ColorTranslation, DestRect,
                           SourcePoint, MaskPoint, Brush, BrushPoint,
	                   rop4);
      break;
    case DC_RECT:
      // Clip the blt to the clip rectangle
      VGADDI_IntersectRect(&CombinedRect, DestRect, &(Clip->rclBounds));
      Ret = (*BltRectFunc)(Dest, Source, Mask, ColorTranslation, &CombinedRect,
                           SourcePoint, MaskPoint, Brush, BrushPoint,
	                   rop4);
      break;
    case DC_COMPLEX:
      Ret = TRUE;
      CLIPOBJ_cEnumStart(Clip, FALSE, CT_RECTANGLES, CD_ANY, ENUM_RECT_LIMIT);
      do
	{
	  EnumMore = CLIPOBJ_bEnum(Clip, (ULONG) sizeof(RectEnum), (PVOID) &RectEnum);

	  for (i = 0; i < RectEnum.c; i++)
	    {
	      VGADDI_IntersectRect(&CombinedRect, DestRect, RectEnum.arcl + i);
	      Ret = (*BltRectFunc)(Dest, Source, Mask, ColorTranslation, &CombinedRect,
	                           SourcePoint, MaskPoint, Brush, BrushPoint, rop4) &&
	            Ret;
	    }
	}
      while (EnumMore);
      break;
  }

  return Ret;
}
