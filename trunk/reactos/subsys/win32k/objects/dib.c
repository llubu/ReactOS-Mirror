/*
 * $Id: dib.c,v 1.32 2003/08/31 07:56:24 gvg Exp $
 *
 * ReactOS W32 Subsystem
 * Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 ReactOS Team
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
 */

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <win32k/bitmaps.h>
#include <win32k/debug.h>
#include "../eng/handle.h"
#include <ntos/minmax.h>
#include <include/error.h>
#include <include/inteng.h>
#include <include/eng.h>
#include <include/dib.h>
#include <internal/safe.h>
#include <include/surface.h>
#include <include/palette.h>

#define NDEBUG
#include <win32k/debug1.h>

UINT STDCALL NtGdiSetDIBColorTable(HDC  hDC,
                           UINT  StartIndex,
                           UINT  Entries,
                           CONST RGBQUAD  *Colors)
{
  PDC dc;
  PALETTEENTRY * palEntry;
  PPALOBJ palette;
  const RGBQUAD *end;

  if (!(dc = (PDC)AccessUserObject((ULONG)hDC))) return 0;

  if (!(palette = (PPALOBJ)PALETTE_LockPalette((ULONG)dc->DevInfo->hpalDefault)))
  {
//    GDI_ReleaseObj( hdc );
    return 0;
  }

  // Transfer color info

  if (dc->w.bitsPerPixel <= 8)
  {
    palEntry = palette->logpalette->palPalEntry + StartIndex;
    if (StartIndex + Entries > (UINT) (1 << dc->w.bitsPerPixel))
      Entries = (1 << dc->w.bitsPerPixel) - StartIndex;

    if (StartIndex + Entries > palette->logpalette->palNumEntries)
      Entries = palette->logpalette->palNumEntries - StartIndex;

    for (end = Colors + Entries; Colors < end; palEntry++, Colors++)
    {
      palEntry->peRed   = Colors->rgbRed;
      palEntry->peGreen = Colors->rgbGreen;
      palEntry->peBlue  = Colors->rgbBlue;
    }
  }
  else
  {
    Entries = 0;
  }

  PALETTE_UnlockPalette(dc->DevInfo->hpalDefault);
//  GDI_ReleaseObj(hdc);

  return Entries;
}

// Converts a DIB to a device-dependent bitmap
INT STDCALL
NtGdiSetDIBits(
	HDC  hDC,
	HBITMAP  hBitmap,
	UINT  StartScan,
	UINT  ScanLines,
	CONST VOID  *Bits,
	CONST BITMAPINFO  *bmi,
	UINT  ColorUse)
{
  DC         *dc;
  BITMAPOBJ  *bitmap;
  HBITMAP     SourceBitmap, DestBitmap;
  INT         result = 0;
  BOOL        copyBitsResult;
  PSURFOBJ    DestSurf, SourceSurf;
  PSURFGDI    DestGDI;
  SIZEL       SourceSize;
  POINTL      ZeroPoint;
  RECTL       DestRect;
  PXLATEOBJ   XlateObj;
  PPALGDI     hDCPalette;
  //RGBQUAD  *lpRGB;
  HPALETTE    DDB_Palette, DIB_Palette;
  ULONG       DDB_Palette_Type, DIB_Palette_Type;
  const BYTE *vBits = (const BYTE*)Bits;
  INT         scanDirection = 1, DIBWidth;

  // Check parameters
  if (!(dc = DC_LockDc(hDC)))
     return 0;

  if (!(bitmap = BITMAPOBJ_LockBitmap(hBitmap)))
  {
    DC_UnlockDc(hDC);
    return 0;
  }

  // Get RGB values
  //if (ColorUse == DIB_PAL_COLORS)
  //  lpRGB = DIB_MapPaletteColors(hDC, bmi);
  //else
  //  lpRGB = &bmi->bmiColors[0];

  // Create a temporary surface for the destination bitmap
  DestBitmap = BitmapToSurf(bitmap);

  DestSurf   = (PSURFOBJ) AccessUserObject( (ULONG)DestBitmap );
  DestGDI    = (PSURFGDI) AccessInternalObject( (ULONG)DestBitmap );

  // Create source surface
  SourceSize.cx = bmi->bmiHeader.biWidth;
  SourceSize.cy = abs(bmi->bmiHeader.biHeight);

  // Determine width of DIB
  DIBWidth = DIB_GetDIBWidthBytes(SourceSize.cx, bmi->bmiHeader.biBitCount);

  // Determine DIB Vertical Orientation
  if(bmi->bmiHeader.biHeight > 0)
  {
    scanDirection = -1;
    vBits += DIBWidth * bmi->bmiHeader.biHeight - DIBWidth;
  }

  SourceBitmap = EngCreateBitmap(SourceSize,
                                 DIBWidth * scanDirection,
                                 BitmapFormat(bmi->bmiHeader.biBitCount, bmi->bmiHeader.biCompression),
                                 0,
                                 (PVOID)vBits );
  SourceSurf = (PSURFOBJ)AccessUserObject((ULONG)SourceBitmap);

  // Destination palette obtained from the hDC
  hDCPalette = PALETTE_LockPalette(dc->DevInfo->hpalDefault);
  DDB_Palette_Type = hDCPalette->Mode;
  DDB_Palette = dc->DevInfo->hpalDefault;
  PALETTE_UnlockPalette(dc->DevInfo->hpalDefault);

  // Source palette obtained from the BITMAPINFO
  DIB_Palette = BuildDIBPalette ( (PBITMAPINFO)bmi, (PINT)&DIB_Palette_Type );

  // Determine XLATEOBJ for color translation
  XlateObj = IntEngCreateXlate(DDB_Palette_Type, DIB_Palette_Type, DDB_Palette, DIB_Palette);

  // Zero point
  ZeroPoint.x = 0;
  ZeroPoint.y = 0;

  // Determine destination rectangle
  DestRect.top	= 0;
  DestRect.left	= 0;
  DestRect.right	= SourceSize.cx;
  DestRect.bottom	= SourceSize.cy;

  copyBitsResult = EngCopyBits(DestSurf, SourceSurf, NULL, XlateObj, &DestRect, &ZeroPoint);

  // If it succeeded, return number of scanlines copies
  if(copyBitsResult == TRUE)
  {
    result = SourceSize.cy - 1;
  }

  // Clean up
  EngDeleteXlate(XlateObj);
  PALETTE_FreePalette(DIB_Palette);
  EngDeleteSurface(SourceBitmap);
  EngDeleteSurface(DestBitmap);

//  if (ColorUse == DIB_PAL_COLORS)
//    WinFree((LPSTR)lpRGB);

  BITMAPOBJ_UnlockBitmap(hBitmap);
  DC_UnlockDc(hDC);

  return result;
}

INT STDCALL
NtGdiSetDIBitsToDevice(
	HDC  hDC,
	INT  XDest,
	INT  YDest,
	DWORD  Width,
	DWORD  Height,
	INT  XSrc,
	INT  YSrc,
	UINT  StartScan,
	UINT  ScanLines,
	CONST VOID  *Bits,
	CONST BITMAPINFO  *bmi,
	UINT  ColorUse)
{
  UNIMPLEMENTED;
  return 0;
}

UINT STDCALL NtGdiGetDIBColorTable(HDC  hDC,
                           UINT  StartIndex,
                           UINT  Entries,
                           RGBQUAD  *Colors)
{
  UNIMPLEMENTED;
}

// Converts a device-dependent bitmap to a DIB
INT STDCALL NtGdiGetDIBits(HDC  hDC,
                   HBITMAP hBitmap,
                   UINT  StartScan,
                   UINT  ScanLines,
                   LPVOID  Bits,
                   LPBITMAPINFO UnsafeInfo,
                   UINT  Usage)
{
  BITMAPINFO Info;
  BITMAPCOREHEADER *Core;
  PBITMAPOBJ BitmapObj;
  INT Result;
  NTSTATUS Status;
  PDC DCObj;
  PPALGDI PalGdi;
  struct
    {
    BITMAPINFO Info;
    DWORD BitFields[3];
    } InfoWithBitFields;
  DWORD *BitField;
  DWORD InfoSize;

  BitmapObj = BITMAPOBJ_LockBitmap(hBitmap);
  if (NULL == BitmapObj)
    {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return 0;
    }

  RtlZeroMemory(&Info, sizeof(BITMAPINFO));
  Status = MmCopyFromCaller(&(Info.bmiHeader.biSize),
                            &(UnsafeInfo->bmiHeader.biSize),
                            sizeof(DWORD));
  if (! NT_SUCCESS(Status))
    {
    SetLastNtError(Status);
    BITMAPOBJ_UnlockBitmap(hBitmap);
    return 0;
    }

  /* If the bits are not requested, UnsafeInfo can point to either a
     BITMAPINFOHEADER or a BITMAPCOREHEADER */
  if (sizeof(BITMAPINFOHEADER) != Info.bmiHeader.biSize &&
      (sizeof(BITMAPCOREHEADER) != Info.bmiHeader.biSize ||
       NULL != Bits))
    {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      BITMAPOBJ_UnlockBitmap(hBitmap);
      return 0;
    }

  Status = MmCopyFromCaller(&(Info.bmiHeader),
                            &(UnsafeInfo->bmiHeader),
                            Info.bmiHeader.biSize);
  if (! NT_SUCCESS(Status))
    {
      SetLastNtError(Status);
      BITMAPOBJ_UnlockBitmap(hBitmap);
      return 0;
    }

  if (NULL == Bits)
    {
      if (sizeof(BITMAPINFOHEADER) == Info.bmiHeader.biSize)
	{
	  if (0 != Info.bmiHeader.biBitCount)
	    {
	      UNIMPLEMENTED;
	    }

	  Info.bmiHeader.biWidth = BitmapObj->bitmap.bmWidth;
	  Info.bmiHeader.biHeight = BitmapObj->bitmap.bmHeight;
	  Info.bmiHeader.biPlanes = BitmapObj->bitmap.bmPlanes;
	  Info.bmiHeader.biBitCount = BitmapObj->bitmap.bmBitsPixel;
	  Info.bmiHeader.biCompression = BI_RGB;
	  Info.bmiHeader.biSizeImage = BitmapObj->bitmap.bmHeight * BitmapObj->bitmap.bmWidthBytes;
	}
      else
	{
	  Core = (BITMAPCOREHEADER *)(&Info.bmiHeader);
	  if (0 != Core->bcBitCount)
	    {
	      UNIMPLEMENTED;
	    }

	  Core->bcWidth = BitmapObj->bitmap.bmWidth;
	  Core->bcHeight = BitmapObj->bitmap.bmHeight;
	  Core->bcPlanes = BitmapObj->bitmap.bmPlanes;
	  Core->bcBitCount = BitmapObj->bitmap.bmBitsPixel;
	}

      Status = MmCopyToCaller(UnsafeInfo, &Info, Info.bmiHeader.biSize);
      if (! NT_SUCCESS(Status))
	{
	  SetLastNtError(Status);
	  BITMAPOBJ_UnlockBitmap(hBitmap);
	  return 0;
	}
      Result = 1;
    }
  else if (0 == StartScan && Info.bmiHeader.biHeight == (LONG) (StartScan + ScanLines) &&
           Info.bmiHeader.biWidth == BitmapObj->bitmap.bmWidth &&
           Info.bmiHeader.biHeight == BitmapObj->bitmap.bmHeight &&
           Info.bmiHeader.biPlanes == BitmapObj->bitmap.bmPlanes &&
           Info.bmiHeader.biBitCount == BitmapObj->bitmap.bmBitsPixel &&
           8 < Info.bmiHeader.biBitCount)
    {
      Info.bmiHeader.biSizeImage = BitmapObj->bitmap.bmHeight * BitmapObj->bitmap.bmWidthBytes;
      Status = MmCopyToCaller(Bits, BitmapObj->bitmap.bmBits, Info.bmiHeader.biSizeImage);
      if (! NT_SUCCESS(Status))
	{
	  SetLastNtError(Status);
	  BITMAPOBJ_UnlockBitmap(hBitmap);
	  return 0;
	}
      RtlZeroMemory(&InfoWithBitFields, sizeof(InfoWithBitFields));
      RtlCopyMemory(&(InfoWithBitFields.Info), &Info, sizeof(BITMAPINFO));
      if (BI_BITFIELDS == Info.bmiHeader.biCompression)
	{
	  DCObj = DC_LockDc(hDC);
	  if (NULL == DCObj)
	    {
	      SetLastWin32Error(ERROR_INVALID_HANDLE);
	      BITMAPOBJ_UnlockBitmap(hBitmap);
	      return 0;
	    }
	  PalGdi = PALETTE_LockPalette(DCObj->w.hPalette);
	  BitField = (DWORD *) ((char *) &InfoWithBitFields + InfoWithBitFields.Info.bmiHeader.biSize);
	  BitField[0] = PalGdi->RedMask;
	  BitField[1] = PalGdi->GreenMask;
	  BitField[2] = PalGdi->BlueMask;
	  PALETTE_UnlockPalette(DCObj->w.hPalette);
	  InfoSize = InfoWithBitFields.Info.bmiHeader.biSize + 3 * sizeof(DWORD);
	  DC_UnlockDc(hDC);
	}
      else
	{
	  InfoSize = Info.bmiHeader.biSize;
	}
      Status = MmCopyToCaller(UnsafeInfo, &InfoWithBitFields, InfoSize);
      if (! NT_SUCCESS(Status))
	{
	  SetLastNtError(Status);
	  BITMAPOBJ_UnlockBitmap(hBitmap);
	  return 0;
	}
    }
  else
    {
    UNIMPLEMENTED;
    }

  BITMAPOBJ_UnlockBitmap(hBitmap);

  return Result;
}

INT STDCALL NtGdiStretchDIBits(HDC  hDC,
                       INT  XDest,
                       INT  YDest,
                       INT  DestWidth,
                       INT  DestHeight,
                       INT  XSrc,
                       INT  YSrc,
                       INT  SrcWidth,
                       INT  SrcHeight,
                       CONST VOID  *Bits,
                       CONST BITMAPINFO  *BitsInfo,
                       UINT  Usage,
                       DWORD  ROP)
{
  UNIMPLEMENTED;
}

LONG STDCALL NtGdiGetBitmapBits(HBITMAP  hBitmap,
                        LONG  Count,
                        LPVOID  Bits)
{
  PBITMAPOBJ  bmp;
  LONG  height, ret;

  bmp = BITMAPOBJ_LockBitmap (hBitmap);
  if (!bmp)
  {
    return 0;
  }

  /* If the bits vector is null, the function should return the read size */
  if (Bits == NULL)
  {
    return bmp->bitmap.bmWidthBytes * bmp->bitmap.bmHeight;
  }

  if (Count < 0)
  {
    DPRINT ("(%ld): Negative number of bytes passed???\n", Count);
    Count = -Count;
  }

  /* Only get entire lines */
  height = Count / bmp->bitmap.bmWidthBytes;
  if (height > bmp->bitmap.bmHeight)
  {
    height = bmp->bitmap.bmHeight;
  }
  Count = height * bmp->bitmap.bmWidthBytes;
  if (Count == 0)
  {
    DPRINT("Less then one entire line requested\n");
    return  0;
  }

  DPRINT("(%08x, %ld, %p) %dx%d %d colors fetched height: %ld\n",
         hBitmap, Count, Bits, bmp->bitmap.bmWidth, bmp->bitmap.bmHeight,
         1 << bmp->bitmap.bmBitsPixel, height );
#if 0
  /* FIXME: Call DDI CopyBits here if available  */
  if(bmp->DDBitmap)
  {
    DPRINT("Calling device specific BitmapBits\n");
    if(bmp->DDBitmap->funcs->pBitmapBits)
    {
      ret = bmp->DDBitmap->funcs->pBitmapBits(hbitmap, bits, count,
                                              DDB_GET);
    }
    else
    {
      ERR_(bitmap)("BitmapBits == NULL??\n");
      ret = 0;
    }
  }
  else
#endif
  {
    if(!bmp->bitmap.bmBits)
    {
      DPRINT ("Bitmap is empty\n");
      ret = 0;
    }
    else
    {
      memcpy(Bits, bmp->bitmap.bmBits, Count);
      ret = Count;
    }
  }

  return  ret;
}

// The CreateDIBitmap function creates a device-dependent bitmap (DDB) from a DIB and, optionally, sets the bitmap bits
// The DDB that is created will be whatever bit depth your reference DC is
HBITMAP STDCALL NtGdiCreateDIBitmap(HDC hdc, const BITMAPINFOHEADER *header,
                               DWORD init, LPCVOID bits, const BITMAPINFO *data,
                               UINT coloruse)
{
  HBITMAP handle;
  BOOL fColor;
  DWORD width;
  int height;
  WORD bpp;
  WORD compr;

  if (DIB_GetBitmapInfo( header, &width, &height, &bpp, &compr ) == -1) return 0;
  if (height < 0) height = -height;

  // Check if we should create a monochrome or color bitmap. We create a monochrome bitmap only if it has exactly 2
  // colors, which are black followed by white, nothing else. In all other cases, we create a color bitmap.

  if (bpp != 1) fColor = TRUE;
  else if ((coloruse != DIB_RGB_COLORS) ||
           (init != CBM_INIT) || !data) fColor = FALSE;
  else
  {
    if (data->bmiHeader.biSize == sizeof(BITMAPINFOHEADER))
    {
      RGBQUAD *rgb = data->bmiColors;
      DWORD col = RGB( rgb->rgbRed, rgb->rgbGreen, rgb->rgbBlue );

      // Check if the first color of the colormap is black
      if ((col == RGB(0, 0, 0)))
      {
        rgb++;
        col = RGB( rgb->rgbRed, rgb->rgbGreen, rgb->rgbBlue );

        // If the second color is white, create a monochrome bitmap
        fColor =  (col != RGB(0xff,0xff,0xff));
      }
    else fColor = TRUE;
  }
  else if (data->bmiHeader.biSize == sizeof(BITMAPCOREHEADER))
  {
    RGBTRIPLE *rgb = ((BITMAPCOREINFO *)data)->bmciColors;
     DWORD col = RGB( rgb->rgbtRed, rgb->rgbtGreen, rgb->rgbtBlue);

    if ((col == RGB(0,0,0)))
    {
      rgb++;
      col = RGB( rgb->rgbtRed, rgb->rgbtGreen, rgb->rgbtBlue );
      fColor = (col != RGB(0xff,0xff,0xff));
    }
    else fColor = TRUE;
  }
  else
  {
      DPRINT("(%ld): wrong size for data\n", data->bmiHeader.biSize );
      return 0;
    }
  }

  // Now create the bitmap

  if (init == CBM_INIT)
    {
      handle = NtGdiCreateCompatibleBitmap(hdc, width, height);
    }
  else
    {
      handle = NtGdiCreateBitmap(width, height, 1, bpp, NULL);
    }

  if (!handle) return 0;

  if (init == CBM_INIT)
  {
    NtGdiSetDIBits(hdc, handle, 0, height, bits, data, coloruse);
  }

  return handle;
}

HBITMAP STDCALL NtGdiCreateDIBSection(HDC hDC,
                              CONST BITMAPINFO  *bmi,
                              UINT  Usage,
                              VOID  *Bits,
                              HANDLE  hSection,
                              DWORD  dwOffset)
{
  HBITMAP hbitmap = 0;
  DC *dc;
  BOOL bDesktopDC = FALSE;

  // If the reference hdc is null, take the desktop dc
  if (hDC == 0)
  {
    hDC = NtGdiCreateCompatableDC(0);
    bDesktopDC = TRUE;
  }

  if ((dc = DC_LockDc(hDC)))
  {
    hbitmap = DIB_CreateDIBSection ( dc, (BITMAPINFO*)bmi, Usage, Bits,
      hSection, dwOffset, 0);
    DC_UnlockDc(hDC);
  }

  if (bDesktopDC)
    NtGdiDeleteDC(hDC);

  return hbitmap;
}

HBITMAP STDCALL
DIB_CreateDIBSection(
  PDC dc, BITMAPINFO *bmi, UINT usage,
  LPVOID *bits, HANDLE section,
  DWORD offset, DWORD ovr_pitch)
{
  HBITMAP res = 0;
  BITMAPOBJ *bmp = NULL;
  DIBSECTION *dib = NULL;

  // Fill BITMAP32 structure with DIB data
  BITMAPINFOHEADER *bi = &bmi->bmiHeader;
  INT effHeight;
  ULONG totalSize;
  UINT Entries = 0;
  BITMAP bm;

  DPRINT("format (%ld,%ld), planes %d, bpp %d, size %ld, colors %ld (%s)\n",
	bi->biWidth, bi->biHeight, bi->biPlanes, bi->biBitCount,
	bi->biSizeImage, bi->biClrUsed, usage == DIB_PAL_COLORS? "PAL" : "RGB");

  effHeight = bi->biHeight >= 0 ? bi->biHeight : -bi->biHeight;
  bm.bmType = 0;
  bm.bmWidth = bi->biWidth;
  bm.bmHeight = effHeight;
  bm.bmWidthBytes = ovr_pitch ? ovr_pitch : (ULONG) DIB_GetDIBWidthBytes(bm.bmWidth, bi->biBitCount);

  bm.bmPlanes = bi->biPlanes;
  bm.bmBitsPixel = bi->biBitCount;
  bm.bmBits = NULL;

  // Get storage location for DIB bits.  Only use biSizeImage if it's valid and
  // we're dealing with a compressed bitmap.  Otherwise, use width * height.
  totalSize = bi->biSizeImage && bi->biCompression != BI_RGB
    ? bi->biSizeImage : (ULONG) (bm.bmWidthBytes * effHeight);

  if (section)
/*    bm.bmBits = MapViewOfFile(section, FILE_MAP_ALL_ACCESS,
			      0L, offset, totalSize); */
    DbgPrint("DIB_CreateDIBSection: Cannot yet handle section DIBs\n");
  else if (ovr_pitch && offset)
    bm.bmBits = (LPVOID) offset;
  else {
    offset = 0;
    bm.bmBits = EngAllocUserMem(totalSize, 0);
  }

/*  bm.bmBits = ExAllocatePool(NonPagedPool, totalSize); */

  if(usage == DIB_PAL_COLORS) memcpy(bmi->bmiColors, (UINT *)DIB_MapPaletteColors(dc, bmi), sizeof(UINT *));

  // Allocate Memory for DIB and fill structure
  if (bm.bmBits)
  {
    dib = ExAllocatePool(PagedPool, sizeof(DIBSECTION));
    RtlZeroMemory(dib, sizeof(DIBSECTION));
  }

  if (dib)
  {
    dib->dsBm = bm;
    dib->dsBmih = *bi;
    dib->dsBmih.biSizeImage = totalSize;

    /* Set dsBitfields values */
    if ( usage == DIB_PAL_COLORS || bi->biBitCount <= 8)
    {
      dib->dsBitfields[0] = dib->dsBitfields[1] = dib->dsBitfields[2] = 0;
    }
    else switch(bi->biBitCount)
    {
      case 16:
        dib->dsBitfields[0] = (bi->biCompression == BI_BITFIELDS) ? *(DWORD *)bmi->bmiColors : 0x7c00;
        dib->dsBitfields[1] = (bi->biCompression == BI_BITFIELDS) ? *((DWORD *)bmi->bmiColors + 1) : 0x03e0;
        dib->dsBitfields[2] = (bi->biCompression == BI_BITFIELDS) ? *((DWORD *)bmi->bmiColors + 2) : 0x001f;
        break;

      case 24:
        dib->dsBitfields[0] = 0xff;
        dib->dsBitfields[1] = 0xff00;
        dib->dsBitfields[2] = 0xff0000;
        break;

      case 32:
        dib->dsBitfields[0] = (bi->biCompression == BI_BITFIELDS) ? *(DWORD *)bmi->bmiColors : 0xff;
        dib->dsBitfields[1] = (bi->biCompression == BI_BITFIELDS) ? *((DWORD *)bmi->bmiColors + 1) : 0xff00;
        dib->dsBitfields[2] = (bi->biCompression == BI_BITFIELDS) ? *((DWORD *)bmi->bmiColors + 2) : 0xff0000;
        break;
    }
    dib->dshSection = section;
    dib->dsOffset = offset;
  }

  // Create Device Dependent Bitmap and add DIB pointer
  if (dib)
  {
    res = NtGdiCreateDIBitmap(dc->hSelf, bi, 0, NULL, bmi, usage);
    if (! res)
      {
	return NULL;
      } 
    bmp = BITMAPOBJ_LockBitmap(res);
    if (NULL == bmp)
      {
	NtGdiDeleteObject(bmp);
	return NULL;
      }
    bmp->dib = (DIBSECTION *) dib;
    /* Install user-mode bits instead of kernel-mode bits */
    ExFreePool(bmp->bitmap.bmBits);
    bmp->bitmap.bmBits = bm.bmBits;

    /* WINE NOTE: WINE makes use of a colormap, which is a color translation table between the DIB and the X physical
                  device. Obviously, this is left out of the ReactOS implementation. Instead, we call
                  NtGdiSetDIBColorTable. */
    if(bi->biBitCount == 1) { Entries = 2; } else
    if(bi->biBitCount == 4) { Entries = 16; } else
    if(bi->biBitCount == 8) { Entries = 256; }

    bmp->ColorMap = ExAllocatePool(NonPagedPool, sizeof(RGBQUAD)*Entries);
    RtlCopyMemory(bmp->ColorMap, bmi->bmiColors, sizeof(RGBQUAD)*Entries);
  }

  // Clean up in case of errors
  if (!res || !bmp || !dib || !bm.bmBits)
  {
    DPRINT("got an error res=%08x, bmp=%p, dib=%p, bm.bmBits=%p\n", res, bmp, dib, bm.bmBits);
/*      if (bm.bmBits)
      {
      if (section)
        UnmapViewOfFile(bm.bmBits), bm.bmBits = NULL;
      else if (!offset)
      VirtualFree(bm.bmBits, 0L, MEM_RELEASE), bm.bmBits = NULL;
    } */

    if (dib) { ExFreePool(dib); dib = NULL; }
    if (bmp) { bmp = NULL; }
    if (res) { BITMAPOBJ_FreeBitmap(res); res = 0; }
  }

  if (bmp)
    {
      BITMAPOBJ_UnlockBitmap(res);
    }

  // Return BITMAP handle and storage location
  if (NULL != bm.bmBits && NULL != bits)
    {
      *bits = bm.bmBits;
    }

  return res;
}

/***********************************************************************
 *           DIB_GetDIBWidthBytes
 *
 * Return the width of a DIB bitmap in bytes. DIB bitmap data is 32-bit aligned.
 * http://www.microsoft.com/msdn/sdk/platforms/doc/sdk/win32/struc/src/str01.htm
 * 11/16/1999 (RJJ) lifted from wine
 */
INT FASTCALL DIB_GetDIBWidthBytes (INT width, INT depth)
{
  int words;

  switch(depth)
  {
    case 1:  words = (width + 31) / 32; break;
    case 4:  words = (width + 7) / 8; break;
    case 8:  words = (width + 3) / 4; break;
    case 15:
    case 16: words = (width + 1) / 2; break;
    case 24: words = (width * 3 + 3)/4; break;

    default:
      DPRINT("(%d): Unsupported depth\n", depth );
      /* fall through */
    case 32:
      words = width;
  }
  return 4 * words;
}

/***********************************************************************
 *           DIB_GetDIBImageBytes
 *
 * Return the number of bytes used to hold the image in a DIB bitmap.
 * 11/16/1999 (RJJ) lifted from wine
 */

INT STDCALL DIB_GetDIBImageBytes (INT  width, INT height, INT depth)
{
  return DIB_GetDIBWidthBytes( width, depth ) * (height < 0 ? -height : height);
}

/***********************************************************************
 *           DIB_BitmapInfoSize
 *
 * Return the size of the bitmap info structure including color table.
 * 11/16/1999 (RJJ) lifted from wine
 */

INT FASTCALL DIB_BitmapInfoSize (const BITMAPINFO * info, WORD coloruse)
{
  int colors;

  if (info->bmiHeader.biSize == sizeof(BITMAPCOREHEADER))
  {
    BITMAPCOREHEADER *core = (BITMAPCOREHEADER *)info;
    colors = (core->bcBitCount <= 8) ? 1 << core->bcBitCount : 0;
    return sizeof(BITMAPCOREHEADER) + colors * ((coloruse == DIB_RGB_COLORS) ? sizeof(RGBTRIPLE) : sizeof(WORD));
  }
  else  /* assume BITMAPINFOHEADER */
  {
    colors = info->bmiHeader.biClrUsed;
    if (!colors && (info->bmiHeader.biBitCount <= 8)) colors = 1 << info->bmiHeader.biBitCount;
    return sizeof(BITMAPINFOHEADER) + colors * ((coloruse == DIB_RGB_COLORS) ? sizeof(RGBQUAD) : sizeof(WORD));
  }
}

INT STDCALL DIB_GetBitmapInfo (const BITMAPINFOHEADER *header,
			       PDWORD width,
			       PINT height,
			       PWORD bpp,
			       PWORD compr)
{
  if (header->biSize == sizeof(BITMAPINFOHEADER))
  {
    *width  = header->biWidth;
    *height = header->biHeight;
    *bpp    = header->biBitCount;
    *compr  = header->biCompression;
    return 1;
  }
  if (header->biSize == sizeof(BITMAPCOREHEADER))
  {
    BITMAPCOREHEADER *core = (BITMAPCOREHEADER *)header;
    *width  = core->bcWidth;
    *height = core->bcHeight;
    *bpp    = core->bcBitCount;
    *compr  = 0;
    return 0;
  }
  DPRINT("(%ld): wrong size for header\n", header->biSize );
  return -1;
}

// Converts a Device Independent Bitmap (DIB) to a Device Dependant Bitmap (DDB)
// The specified Device Context (DC) defines what the DIB should be converted to
PBITMAPOBJ FASTCALL DIBtoDDB(HGLOBAL hPackedDIB, HDC hdc) // FIXME: This should be removed. All references to this function should
						 // change to NtGdiSetDIBits
{
  HBITMAP hBmp = 0;
  PBITMAPOBJ pBmp = NULL;
  DIBSECTION *dib;
  LPBYTE pbits = NULL;

  // Get a pointer to the packed DIB's data
  // pPackedDIB = (LPBYTE)GlobalLock(hPackedDIB);
  dib = hPackedDIB;

  pbits = (LPBYTE)(dib + DIB_BitmapInfoSize((BITMAPINFO*)&dib->dsBmih, DIB_RGB_COLORS));

  // Create a DDB from the DIB
  hBmp = NtGdiCreateDIBitmap ( hdc, &dib->dsBmih, CBM_INIT,
    (LPVOID)pbits, (BITMAPINFO*)&dib->dsBmih, DIB_RGB_COLORS);

  // GlobalUnlock(hPackedDIB);

  // Retrieve the internal Pixmap from the DDB
  pBmp = BITMAPOBJ_LockBitmap(hBmp);

  return pBmp;
}

RGBQUAD * FASTCALL
DIB_MapPaletteColors(PDC dc, CONST BITMAPINFO* lpbmi)
{
  RGBQUAD *lpRGB;
  ULONG nNumColors,i;
  DWORD *lpIndex;
  PPALOBJ palObj;

  palObj = (PPALOBJ) PALETTE_LockPalette(dc->DevInfo->hpalDefault);

  if (NULL == palObj)
    {
//      RELEASEDCINFO(hDC);
      return NULL;
    }

  nNumColors = 1 << lpbmi->bmiHeader.biBitCount;
  if (lpbmi->bmiHeader.biClrUsed)
    {
      nNumColors = min(nNumColors, lpbmi->bmiHeader.biClrUsed);
    }

  lpRGB = (RGBQUAD *)ExAllocatePool(NonPagedPool, sizeof(RGBQUAD) * nNumColors);
  lpIndex = (DWORD *)&lpbmi->bmiColors[0];

  for (i = 0; i < nNumColors; i++)
    {
      lpRGB[i].rgbRed = palObj->logpalette->palPalEntry[*lpIndex].peRed;
      lpRGB[i].rgbGreen = palObj->logpalette->palPalEntry[*lpIndex].peGreen;
      lpRGB[i].rgbBlue = palObj->logpalette->palPalEntry[*lpIndex].peBlue;
      lpIndex++;
    }
//    RELEASEDCINFO(hDC);
  PALETTE_UnlockPalette(dc->DevInfo->hpalDefault);

  return lpRGB;
}

PPALETTEENTRY STDCALL
DIBColorTableToPaletteEntries (
	PPALETTEENTRY palEntries,
	const RGBQUAD *DIBColorTable,
	ULONG ColorCount
	)
{
  ULONG i;

  for (i = 0; i < ColorCount; i++)
    {
      palEntries->peRed   = DIBColorTable->rgbRed;
      palEntries->peGreen = DIBColorTable->rgbGreen;
      palEntries->peBlue  = DIBColorTable->rgbBlue;
      palEntries++;
      DIBColorTable++;
    }

  return palEntries;
}

HPALETTE FASTCALL
BuildDIBPalette (PBITMAPINFO bmi, PINT paletteType)
{
  BYTE bits;
  ULONG ColorCount;
  PALETTEENTRY *palEntries = NULL;
  HPALETTE hPal;

  // Determine Bits Per Pixel
  bits = bmi->bmiHeader.biBitCount;

  // Determine paletteType from Bits Per Pixel
  if (bits <= 8)
    {
      *paletteType = PAL_INDEXED;
    }
  else if(bits < 24)
    {
      *paletteType = PAL_BITFIELDS;
    }
  else
    {
      *paletteType = PAL_BGR;
    }

  if (bmi->bmiHeader.biClrUsed == 0)
    {
      ColorCount = 1 << bmi->bmiHeader.biBitCount;
    }
  else
    {
      ColorCount = bmi->bmiHeader.biClrUsed;
    }

  if (PAL_INDEXED == *paletteType)
    {
      palEntries = ExAllocatePool(NonPagedPool, sizeof(PALETTEENTRY)*ColorCount);
      DIBColorTableToPaletteEntries(palEntries, bmi->bmiColors, ColorCount);
    }
  hPal = PALETTE_AllocPalette( *paletteType, ColorCount, (ULONG*)palEntries, 0, 0, 0 );
  if (NULL != palEntries)
    {
      ExFreePool(palEntries);
    }

  return hPal;
}

/* EOF */
