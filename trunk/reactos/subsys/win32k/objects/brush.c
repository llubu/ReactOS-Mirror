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
    GDIOBJ_SetOwnership(pBrush->hbmPattern, PsGetCurrentProcess());
    NtGdiDeleteObject(pBrush->hbmPattern);
  }
  
  return TRUE;
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

      BITMAPOBJ_UnlockBitmap(BrushObj->hbmPattern);
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
   UINT PaletteType;

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
   BITMAPOBJ_UnlockBitmap(hPattern);

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

   GDIOBJ_SetOwnership(hPattern, NULL);

   BRUSHOBJ_UnlockBrush(hBrush);
   
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

   hPattern = NtGdiCreateBitmap(8, 8, 1, 1, HatchBrushes[Style]);
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

   GDIOBJ_SetOwnership(hPattern, NULL);

   BRUSHOBJ_UnlockBrush(hBrush);
   
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

   GDIOBJ_SetOwnership(hPattern, NULL);

   BRUSHOBJ_UnlockBrush(hBrush);
   
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
   BrushObject->BrushAttr.lbColor = Color & 0xFFFFFF;
   /* FIXME: Fill in the rest of fields!!! */

   BRUSHOBJ_UnlockBrush(hBrush);
   
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
   BRUSHOBJ_UnlockBrush(hBrush);
   
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
      
      BrushOrigin.x = BrushObj->ptOrigin.x + dc->w.DCOrgX;
      BrushOrigin.y = BrushObj->ptOrigin.y + dc->w.DCOrgY;

      IntGdiInitBrushInstance(&BrushInst, BrushObj, dc->XlateBrush);

      ret = IntEngBitBlt(
         BitmapObj,
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

   BITMAPOBJ_UnlockBitmap(dc->w.hBitmap);

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
      DC_UnlockDc(hDC);
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
        BRUSHOBJ_UnlockBrush(r->hBrush);
      }
      r++;
   }

   DC_UnlockDc( hDC );
	
   return TRUE;
}

/* PUBLIC FUNCTIONS ***********************************************************/

HBRUSH STDCALL
NtGdiCreateDIBBrush(
   CONST BITMAPINFO *BitmapInfoAndData,
   UINT ColorSpec,
   UINT BitmapInfoSize,
   CONST VOID *PackedDIB)
{
   BITMAPINFO *SafeBitmapInfoAndData;
   NTSTATUS Status;
   HBRUSH hBrush;

   SafeBitmapInfoAndData = EngAllocMem(0, BitmapInfoSize, 0);
   if (SafeBitmapInfoAndData == NULL)
   {
      SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
      return NULL;
   }

   Status = MmCopyFromCaller(SafeBitmapInfoAndData, BitmapInfoAndData,
                             BitmapInfoSize);
   if (!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      return 0;
   }

   hBrush = IntGdiCreateDIBBrush(SafeBitmapInfoAndData, ColorSpec,
                                 BitmapInfoSize, PackedDIB);

   EngFreeMem(SafeBitmapInfoAndData);

   return hBrush;
}

HBRUSH STDCALL
NtGdiCreateHatchBrush(
   INT Style,
   COLORREF Color)
{
   return IntGdiCreateHatchBrush(Style, Color);
}

HBRUSH STDCALL
NtGdiCreatePatternBrush(
   HBITMAP hBitmap)
{
   return IntGdiCreatePatternBrush(hBitmap);
}

HBRUSH STDCALL
NtGdiCreateSolidBrush(COLORREF Color)
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
      NTSTATUS Status;
      POINT SafePoint;
      SafePoint.x = dc->w.brushOrgX;
      SafePoint.y = dc->w.brushOrgY;
      Status = MmCopyToCaller(Point, &SafePoint, sizeof(POINT));
      if(!NT_SUCCESS(Status))
      {
        DC_UnlockDc(hDC);
        SetLastNtError(Status);
        return FALSE;
      }
   }

   dc->w.brushOrgX = XOrg;
   dc->w.brushOrgY = YOrg;
   DC_UnlockDc(hDC);

   return TRUE;
}

BOOL STDCALL
NtGdiPolyPatBlt(
   HDC hDC,
   DWORD dwRop,
   PPATRECT pRects,
   INT cRects,
   ULONG Reserved)
{
   PPATRECT rb = NULL;
   NTSTATUS Status;
   BOOL Ret;
    
   if (cRects > 0)
   {
      rb = ExAllocatePoolWithTag(PagedPool, sizeof(PATRECT) * cRects, TAG_PATBLT);
      if (!rb)
      {
         SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
         return FALSE;
      }
      Status = MmCopyFromCaller(rb, pRects, sizeof(PATRECT) * cRects);
      if (!NT_SUCCESS(Status))
      {
         ExFreePool(rb);
         SetLastNtError(Status);
         return FALSE;
      }
   }
    
   Ret = IntGdiPolyPatBlt(hDC, dwRop, pRects, cRects, Reserved);
	
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
      DC_UnlockDc(hDC);
      /* Yes, Windows really returns TRUE in this case */
      return TRUE;
   }

   BrushObj = BRUSHOBJ_LockBrush(dc->w.hBrush);
   if (BrushObj == NULL)
   {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      DC_UnlockDc(hDC);
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

   BRUSHOBJ_UnlockBrush(dc->w.hBrush);
   DC_UnlockDc(hDC);

   return ret;
}

/* EOF */
