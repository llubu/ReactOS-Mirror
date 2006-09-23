/*
 * ReactOS Win32 Subsystem
 *
 * Copyright (C) 1998 - 2004 ReactOS Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id$
 */

#include <w32k.h>

#define NDEBUG
#include <debug.h>

static const USHORT HatchBrushes[NB_HATCH_STYLES][8] =
{
  {0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00}, /* HS_HORIZONTAL */
  {0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08}, /* HS_VERTICAL   */
  {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80}, /* HS_FDIAGONAL  */
  {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01}, /* HS_BDIAGONAL  */
  {0x08, 0x08, 0x08, 0xff, 0x08, 0x08, 0x08, 0x08}, /* HS_CROSS      */
  {0x81, 0x42, 0x24, 0x18, 0x18, 0x24, 0x42, 0x81}  /* HS_DIAGCROSS  */
};

BOOL INTERNAL_CALL
BRUSH_Cleanup(PVOID ObjectBody)
{
  PGDIBRUSHOBJ pBrush = (PGDIBRUSHOBJ)ObjectBody;
  if(pBrush->flAttrs & (GDIBRUSH_IS_HATCH | GDIBRUSH_IS_BITMAP))
  {
    ASSERT(pBrush->hbmPattern);
    GDIOBJ_SetOwnership(GdiHandleTable, pBrush->hbmPattern, PsGetCurrentProcess());
    NtGdiDeleteObject(pBrush->hbmPattern);
  }

  return TRUE;
}

INT FASTCALL
BRUSH_GetObject (PGDIBRUSHOBJ BrushObj, INT Count, BRUSHOBJ * Buffer)
{	
	if (Buffer)
	{
	    Buffer->iSolidColor = BrushObj->BrushAttr.lbColor;
        Buffer->pvRbrush = BrushObj->ulRealization;
        Buffer->flColorType = 0;
	}
	return sizeof(BRUSHOBJ);
}


XLATEOBJ* FASTCALL
IntGdiCreateBrushXlate(PDC Dc, GDIBRUSHOBJ *BrushObj, BOOLEAN *Failed)
{
   XLATEOBJ *Result = NULL;

   if (BrushObj->flAttrs & GDIBRUSH_IS_NULL)
   {
      Result = NULL;
      *Failed = FALSE;
   }
   else if (BrushObj->flAttrs & GDIBRUSH_IS_SOLID)
   {
      Result = IntEngCreateXlate(0, PAL_RGB, Dc->w.hPalette, NULL);
      *Failed = FALSE;
   }
   else
   {
      BITMAPOBJ *Pattern = BITMAPOBJ_LockBitmap(BrushObj->hbmPattern);
      if (Pattern == NULL)
         return NULL;

      /* Special case: 1bpp pattern */
      if (Pattern->SurfObj.iBitmapFormat == BMF_1BPP)
      {
         if (Dc->w.bitsPerPixel != 1)
            Result = IntEngCreateSrcMonoXlate(Dc->w.hPalette, Dc->w.textColor, Dc->w.backgroundColor);
      }
      else if (BrushObj->flAttrs & GDIBRUSH_IS_DIB)
      {
         Result = IntEngCreateXlate(0, 0, Dc->w.hPalette, Pattern->hDIBPalette);
      }

      BITMAPOBJ_UnlockBitmap(Pattern);
      *Failed = FALSE;
   }

   return Result;
}

VOID FASTCALL
IntGdiInitBrushInstance(GDIBRUSHINST *BrushInst, PGDIBRUSHOBJ BrushObj, XLATEOBJ *XlateObj)
{
   ASSERT(BrushInst);
   ASSERT(BrushObj);
   if (BrushObj->flAttrs & GDIBRUSH_IS_NULL)
      BrushInst->BrushObject.iSolidColor = 0;
   else if (BrushObj->flAttrs & GDIBRUSH_IS_SOLID)
      BrushInst->BrushObject.iSolidColor = XLATEOBJ_iXlate(XlateObj, BrushObj->BrushAttr.lbColor);
   else
      BrushInst->BrushObject.iSolidColor = 0xFFFFFFFF;
   BrushInst->BrushObject.pvRbrush = BrushObj->ulRealization;
   BrushInst->BrushObject.flColorType = 0;
   BrushInst->GdiBrushObject = BrushObj;
   BrushInst->XlateObject = XlateObj;
}

/**
 * @name CalculateColorTableSize
 *
 * Internal routine to calculate the number of color table entries.
 *
 * @param BitmapInfoHeader
 *        Input bitmap information header, can be any version of
 *        BITMAPINFOHEADER or BITMAPCOREHEADER.
 *
 * @param ColorSpec
 *        Pointer to variable which specifiing the color mode (DIB_RGB_COLORS
 *        or DIB_RGB_COLORS). On successful return this value is normalized
 *        according to the bitmap info.
 *
 * @param ColorTableSize
 *        On successful return this variable is filled with number of
 *        entries in color table for the image with specified parameters.
 *
 * @return
 *    TRUE if the input values together form a valid image, FALSE otherwise.
 */

BOOL STDCALL
CalculateColorTableSize(
   CONST BITMAPINFOHEADER *BitmapInfoHeader,
   UINT *ColorSpec,
   UINT *ColorTableSize)
{
   WORD BitCount;
   DWORD ClrUsed;
   DWORD Compression;

   /*
    * At first get some basic parameters from the passed BitmapInfoHeader
    * structure. It can have one of the following formats:
    * - BITMAPCOREHEADER (the oldest one with totally different layout
    *                     from the others)
    * - BITMAPINFOHEADER (the standard and most common header)
    * - BITMAPV4HEADER (extension of BITMAPINFOHEADER)
    * - BITMAPV5HEADER (extension of BITMAPV4HEADER)
    */

   if (BitmapInfoHeader->biSize == sizeof(BITMAPCOREHEADER))
   {
      BitCount = ((LPBITMAPCOREHEADER)BitmapInfoHeader)->bcBitCount;
      ClrUsed = 0;
      Compression = BI_RGB;
   }
   else
   {
      BitCount = BitmapInfoHeader->biBitCount;
      ClrUsed = BitmapInfoHeader->biClrUsed;
      Compression = BitmapInfoHeader->biCompression;
   }

   switch (Compression)
   {
      case BI_BITFIELDS:
         if (*ColorSpec == DIB_PAL_COLORS)
            *ColorSpec = DIB_RGB_COLORS;

         if (BitCount != 16 && BitCount != 32)
            return FALSE;

         /*
          * For BITMAPV4HEADER/BITMAPV5HEADER the masks are included in
          * the structure itself (bV4RedMask, bV4GreenMask, and bV4BlueMask).
          * For BITMAPINFOHEADER the color masks are stored in the palette.
          */

         if (BitmapInfoHeader->biSize > sizeof(BITMAPINFOHEADER))
            *ColorTableSize = 0;
         else
            *ColorTableSize = 3;

         return TRUE;

      case BI_RGB:
         switch (BitCount)
         {
            case 1:
               *ColorTableSize = ClrUsed ? min(ClrUsed, 2) : 2;
               return TRUE;

            case 4:
               *ColorTableSize = ClrUsed ? min(ClrUsed, 16) : 16;
               return TRUE;

            case 8:
               *ColorTableSize = ClrUsed ? min(ClrUsed, 256) : 256;
               return TRUE;

            default:
               if (*ColorSpec == DIB_PAL_COLORS)
                  *ColorSpec = DIB_RGB_COLORS;
               if (BitCount != 16 && BitCount != 24 && BitCount != 32)
                  return FALSE;
               *ColorTableSize = ClrUsed;
               return TRUE;
         }

      case BI_RLE4:
         if (BitCount == 4)
         {
            *ColorTableSize = ClrUsed ? min(ClrUsed, 16) : 16;
            return TRUE;
         }
         return FALSE;

      case BI_RLE8:
         if (BitCount == 8)
         {
            *ColorTableSize = ClrUsed ? min(ClrUsed, 256) : 256;
            return TRUE;
         }
         return FALSE;

      case BI_JPEG:
      case BI_PNG:
         *ColorTableSize = ClrUsed;
         return TRUE;

      default:
         return FALSE;
   }
}

HBRUSH STDCALL
IntGdiCreateDIBBrush(
   CONST BITMAPINFO *BitmapInfo,
   UINT ColorSpec,
   UINT BitmapInfoSize,
   CONST VOID *PackedDIB)
{
   HBRUSH hBrush;
   PGDIBRUSHOBJ BrushObject;
   HBITMAP hPattern;
   ULONG_PTR DataPtr;
   UINT PaletteEntryCount;
   PBITMAPOBJ BitmapObject;
   INT PaletteType;

   if (BitmapInfo->bmiHeader.biSize < sizeof(BITMAPINFOHEADER))
   {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return NULL;
   }

   if (!CalculateColorTableSize(&BitmapInfo->bmiHeader, &ColorSpec,
                                &PaletteEntryCount))
   {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return NULL;
   }

   DataPtr = (ULONG_PTR)BitmapInfo + BitmapInfo->bmiHeader.biSize;
   if (ColorSpec == DIB_RGB_COLORS)
      DataPtr += PaletteEntryCount * sizeof(RGBQUAD);
   else
      DataPtr += PaletteEntryCount * sizeof(USHORT);

   hPattern = NtGdiCreateBitmap(BitmapInfo->bmiHeader.biWidth,
                                BitmapInfo->bmiHeader.biHeight,
                                BitmapInfo->bmiHeader.biPlanes,
                                BitmapInfo->bmiHeader.biBitCount,
                                (PVOID)DataPtr);
   if (hPattern == NULL)
   {
      SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
      return NULL;
   }

   BitmapObject = BITMAPOBJ_LockBitmap(hPattern);
   ASSERT(BitmapObject != NULL);
   BitmapObject->hDIBPalette = BuildDIBPalette(BitmapInfo, &PaletteType);
   BITMAPOBJ_UnlockBitmap(BitmapObject);

   hBrush = BRUSHOBJ_AllocBrush();
   if (hBrush == NULL)
   {
      NtGdiDeleteObject(hPattern);
      SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
      return NULL;
   }

   BrushObject = BRUSHOBJ_LockBrush(hBrush);
   ASSERT(BrushObject != NULL);

   BrushObject->flAttrs |= GDIBRUSH_IS_BITMAP | GDIBRUSH_IS_DIB;
   BrushObject->hbmPattern = hPattern;
   /* FIXME: Fill in the rest of fields!!! */

   GDIOBJ_SetOwnership(GdiHandleTable, hPattern, NULL);

   BRUSHOBJ_UnlockBrush(BrushObject);

   return hBrush;
}

HBRUSH STDCALL
IntGdiCreateHatchBrush(
   INT Style,
   COLORREF Color)
{
   HBRUSH hBrush;
   PGDIBRUSHOBJ BrushObject;
   HBITMAP hPattern;

   if (Style < 0 || Style >= NB_HATCH_STYLES)
   {
      return 0;
   }

   hPattern = NtGdiCreateBitmap(8, 8, 1, 1, (LPBYTE)HatchBrushes[Style]);
   if (hPattern == NULL)
   {
      SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
      return NULL;
   }

   hBrush = BRUSHOBJ_AllocBrush();
   if (hBrush == NULL)
   {
      NtGdiDeleteObject(hPattern);
      SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
      return NULL;
   }

   BrushObject = BRUSHOBJ_LockBrush(hBrush);
   ASSERT(BrushObject != NULL);

   BrushObject->flAttrs |= GDIBRUSH_IS_HATCH;
   BrushObject->hbmPattern = hPattern;
   BrushObject->BrushAttr.lbColor = Color & 0xFFFFFF;

   GDIOBJ_SetOwnership(GdiHandleTable, hPattern, NULL);

   BRUSHOBJ_UnlockBrush(BrushObject);

   return hBrush;
}

HBRUSH STDCALL
IntGdiCreatePatternBrush(
   HBITMAP hBitmap)
{
   HBRUSH hBrush;
   PGDIBRUSHOBJ BrushObject;
   HBITMAP hPattern;

   hPattern = BITMAPOBJ_CopyBitmap(hBitmap);
   if (hPattern == NULL)
   {
      SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
      return NULL;
   }

   hBrush = BRUSHOBJ_AllocBrush();
   if (hBrush == NULL)
   {
      NtGdiDeleteObject(hPattern);
      SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
      return NULL;
   }

   BrushObject = BRUSHOBJ_LockBrush(hBrush);
   ASSERT(BrushObject != NULL);

   BrushObject->flAttrs |= GDIBRUSH_IS_BITMAP;
   BrushObject->hbmPattern = hPattern;
   /* FIXME: Fill in the rest of fields!!! */

   GDIOBJ_SetOwnership(GdiHandleTable, hPattern, NULL);

   BRUSHOBJ_UnlockBrush(BrushObject);

   return hBrush;
}

HBRUSH STDCALL
IntGdiCreateSolidBrush(
   COLORREF Color)
{
   HBRUSH hBrush;
   PGDIBRUSHOBJ BrushObject;

   hBrush = BRUSHOBJ_AllocBrush();
   if (hBrush == NULL)
   {
      SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
      return NULL;
   }

   BrushObject = BRUSHOBJ_LockBrush(hBrush);
   ASSERT(BrushObject != NULL);

   BrushObject->flAttrs |= GDIBRUSH_IS_SOLID;

   BrushObject->BrushAttr.lbColor = Color;
   /* FIXME: Fill in the rest of fields!!! */

   BRUSHOBJ_UnlockBrush(BrushObject);

   return hBrush;
}

HBRUSH STDCALL
IntGdiCreateNullBrush(VOID)
{
   HBRUSH hBrush;
   PGDIBRUSHOBJ BrushObject;

   hBrush = BRUSHOBJ_AllocBrush();
   if (hBrush == NULL)
   {
      SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
      return NULL;
   }

   BrushObject = BRUSHOBJ_LockBrush(hBrush);
   ASSERT(BrushObject != NULL);
   BrushObject->flAttrs |= GDIBRUSH_IS_NULL;
   BRUSHOBJ_UnlockBrush(BrushObject);

   return hBrush;
}

BOOL FASTCALL
IntPatBlt(
   PDC dc,
   INT XLeft,
   INT YLeft,
   INT Width,
   INT Height,
   DWORD ROP,
   PGDIBRUSHOBJ BrushObj)
{
   RECTL DestRect;
   BITMAPOBJ *BitmapObj;
   GDIBRUSHINST BrushInst;
   POINTL BrushOrigin;
   BOOL ret = TRUE;

   ASSERT(BrushObj);

   BitmapObj = BITMAPOBJ_LockBitmap(dc->w.hBitmap);
   if (BitmapObj == NULL)
   {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return FALSE;
   }

   if (!(BrushObj->flAttrs & GDIBRUSH_IS_NULL))
   {
      if (Width > 0)
      {
         DestRect.left = XLeft + dc->w.DCOrgX;
         DestRect.right = XLeft + Width + dc->w.DCOrgX;
      }
      else
      {
         DestRect.left = XLeft + Width + 1 + dc->w.DCOrgX;
         DestRect.right = XLeft + dc->w.DCOrgX + 1;
      }

      if (Height > 0)
      {
         DestRect.top = YLeft + dc->w.DCOrgY;
         DestRect.bottom = YLeft + Height + dc->w.DCOrgY;
      }
      else
      {
         DestRect.top = YLeft + Height + dc->w.DCOrgY + 1;
         DestRect.bottom = YLeft + dc->w.DCOrgY + 1;
      }

      IntLPtoDP(dc, (LPPOINT)&DestRect, 2);

      BrushOrigin.x = BrushObj->ptOrigin.x + dc->w.DCOrgX;
      BrushOrigin.y = BrushObj->ptOrigin.y + dc->w.DCOrgY;

      IntGdiInitBrushInstance(&BrushInst, BrushObj, dc->XlateBrush);

      ret = IntEngBitBlt(
         &BitmapObj->SurfObj,
         NULL,
         NULL,
         dc->CombinedClip,
         NULL,
         &DestRect,
         NULL,
         NULL,
         &BrushInst.BrushObject,
         &BrushOrigin,
         ROP3_TO_ROP4(ROP));
   }

   BITMAPOBJ_UnlockBitmap(BitmapObj);

   return ret;
}

BOOL FASTCALL
IntGdiPolyPatBlt(
   HDC hDC,
   DWORD dwRop,
   PPATRECT pRects,
   int cRects,
   ULONG Reserved)
{
   int i;
   PPATRECT r;
   PGDIBRUSHOBJ BrushObj;
   DC *dc;

   dc = DC_LockDc(hDC);
   if (dc == NULL)
   {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return FALSE;
   }
   if (dc->IsIC)
   {
      DC_UnlockDc(dc);
      /* Yes, Windows really returns TRUE in this case */
      return TRUE;
   }

   for (r = pRects, i = 0; i < cRects; i++)
   {
      BrushObj = BRUSHOBJ_LockBrush(r->hBrush);
      if(BrushObj != NULL)
      {
        IntPatBlt(
           dc,
           r->r.left,
           r->r.top,
           r->r.right,
           r->r.bottom,
           dwRop,
           BrushObj);
        BRUSHOBJ_UnlockBrush(BrushObj);
      }
      r++;
   }

   DC_UnlockDc(dc);

   return TRUE;
}

/* PUBLIC FUNCTIONS ***********************************************************/

HBRUSH STDCALL
NtGdiCreateDIBBrush(
   IN PVOID BitmapInfoAndData,
   IN FLONG ColorSpec,
   IN UINT BitmapInfoSize,
   IN BOOL  b8X8,
   IN BOOL bPen,
   IN PVOID PackedDIB)
{
   BITMAPINFO *SafeBitmapInfoAndData;
   NTSTATUS Status = STATUS_SUCCESS;
   HBRUSH hBrush;

   SafeBitmapInfoAndData = EngAllocMem(0, BitmapInfoSize, 0);
   if (SafeBitmapInfoAndData == NULL)
   {
      SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
      return NULL;
   }

   _SEH_TRY
   {
      ProbeForRead(BitmapInfoAndData,
                   BitmapInfoSize,
                   1);
      RtlCopyMemory(SafeBitmapInfoAndData,
                    BitmapInfoAndData,
                    BitmapInfoSize);
   }
   _SEH_HANDLE
   {
      Status = _SEH_GetExceptionCode();
   }
   _SEH_END;
   
   if (!NT_SUCCESS(Status))
   {
      EngFreeMem(SafeBitmapInfoAndData);
      SetLastNtError(Status);
      return 0;
   }

   hBrush = IntGdiCreateDIBBrush(SafeBitmapInfoAndData, ColorSpec,
                                 BitmapInfoSize, PackedDIB);

   EngFreeMem(SafeBitmapInfoAndData);

   return hBrush;
}

HBRUSH STDCALL
NtGdiCreateHatchBrushInternal(
   ULONG Style,
   COLORREF Color,
   BOOL bPen)
{
   return IntGdiCreateHatchBrush(Style, Color);
}

HBRUSH STDCALL
NtGdiCreatePatternBrushInternal(
   HBITMAP hBitmap,
   BOOL bPen,
   BOOL b8x8)
{
   return IntGdiCreatePatternBrush(hBitmap);
}

HBRUSH STDCALL
NtGdiCreateSolidBrush(COLORREF Color,
                      IN OPTIONAL HBRUSH hbr)
{
   return IntGdiCreateSolidBrush(Color);
}

/*
 * NtGdiSetBrushOrgEx
 *
 * The NtGdiSetBrushOrgEx function sets the brush origin that GDI assigns to
 * the next brush an application selects into the specified device context.
 *
 * Status
 *    @implemented
 */

BOOL STDCALL
NtGdiSetBrushOrgEx(HDC hDC, INT XOrg, INT YOrg, LPPOINT Point)
{
   PDC dc = DC_LockDc(hDC);
   if (dc == NULL)
   {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return FALSE;
   }

   if (Point != NULL)
   {
      NTSTATUS Status = STATUS_SUCCESS;
      POINT SafePoint;
      SafePoint.x = dc->w.brushOrgX;
      SafePoint.y = dc->w.brushOrgY;
      _SEH_TRY
      {
         ProbeForWrite(Point,
                       sizeof(POINT),
                       1);
         *Point = SafePoint;
      }
      _SEH_HANDLE
      {
         Status = _SEH_GetExceptionCode();
      }
      _SEH_END;

      if(!NT_SUCCESS(Status))
      {
        DC_UnlockDc(dc);
        SetLastNtError(Status);
        return FALSE;
      }
   }

   dc->w.brushOrgX = XOrg;
   dc->w.brushOrgY = YOrg;
   DC_UnlockDc(dc);

   return TRUE;
}

BOOL STDCALL
NtGdiPolyPatBlt(
   HDC hDC,
   DWORD dwRop,
   IN PPOLYPATBLT pRects,
   IN DWORD cRects,
   IN DWORD Mode)
{
   PPATRECT rb = NULL;
   NTSTATUS Status = STATUS_SUCCESS;
   BOOL Ret;

   if (cRects > 0)
   {
      rb = ExAllocatePoolWithTag(PagedPool, sizeof(PATRECT) * cRects, TAG_PATBLT);
      if (!rb)
      {
         SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
         return FALSE;
      }
      _SEH_TRY
      {
         ProbeForRead(pRects,
                      cRects * sizeof(PATRECT),
                      1);
         RtlCopyMemory(rb,
                       pRects,
                       cRects * sizeof(PATRECT));
      }
      _SEH_HANDLE
      {
         Status = _SEH_GetExceptionCode();
      }
      _SEH_END;

      if (!NT_SUCCESS(Status))
      {
         ExFreePool(rb);
         SetLastNtError(Status);
         return FALSE;
      }
   }

   Ret = IntGdiPolyPatBlt(hDC, dwRop, (PPATRECT)pRects, cRects, Mode);

   if (cRects > 0)
      ExFreePool(rb);

   return Ret;
}

BOOL STDCALL
NtGdiPatBlt(
   HDC hDC,
   INT XLeft,
   INT YLeft,
   INT Width,
   INT Height,
   DWORD ROP)
{
   PGDIBRUSHOBJ BrushObj;
   DC *dc = DC_LockDc(hDC);
   BOOL ret;

   if (dc == NULL)
   {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return FALSE;
   }
   if (dc->IsIC)
   {
      DC_UnlockDc(dc);
      /* Yes, Windows really returns TRUE in this case */
      return TRUE;
   }

   BrushObj = BRUSHOBJ_LockBrush(dc->w.hBrush);
   if (BrushObj == NULL)
   {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      DC_UnlockDc(dc);
      return FALSE;
   }

   ret = IntPatBlt(
      dc,
      XLeft,
      YLeft,
      Width,
      Height,
      ROP,
      BrushObj);

   BRUSHOBJ_UnlockBrush(BrushObj);
   DC_UnlockDc(dc);

   return ret;
}

/* EOF */
