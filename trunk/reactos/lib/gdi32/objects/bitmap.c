#include "precomp.h"

/*
 * @implemented
 */
HBITMAP STDCALL
CreateDIBSection(
   HDC hDC,
   CONST BITMAPINFO *BitmapInfo,
   UINT Usage,
   VOID **Bits,
   HANDLE hSection,
   DWORD dwOffset)
{
   PBITMAPINFO pConvertedInfo;
   UINT ConvertedInfoSize;
   HBITMAP hBitmap = NULL;

   pConvertedInfo = ConvertBitmapInfo(BitmapInfo, Usage,
                                      &ConvertedInfoSize, FALSE);
   if (pConvertedInfo)
   {
      hBitmap = NtGdiCreateDIBSection(hDC, pConvertedInfo, Usage, Bits,
                                      hSection, dwOffset);
      if (BitmapInfo != pConvertedInfo)
         RtlFreeHeap(RtlGetProcessHeap(), 0, pConvertedInfo);
   }

   return hBitmap;
}

/*
 * @implemented
 */
BOOL STDCALL
StretchBlt(
   HDC hdcDest,      /* handle to destination DC */
   int nXOriginDest, /* x-coord of destination upper-left corner */
   int nYOriginDest, /* y-coord of destination upper-left corner */
   int nWidthDest,   /* width of destination rectangle */
   int nHeightDest,  /* height of destination rectangle */
   HDC hdcSrc,       /* handle to source DC */
   int nXOriginSrc,  /* x-coord of source upper-left corner */
   int nYOriginSrc,  /* y-coord of source upper-left corner */
   int nWidthSrc,    /* width of source rectangle */
   int nHeightSrc,   /* height of source rectangle */
   DWORD dwRop)      /* raster operation code */
	
{
   if ((nWidthDest != nWidthSrc) || (nHeightDest != nHeightSrc))
   {
      return NtGdiStretchBlt(hdcDest, nXOriginDest, nYOriginDest, nWidthDest,
                             nHeightDest, hdcSrc, nXOriginSrc, nYOriginSrc,
                             nWidthSrc, nHeightSrc, dwRop);
   }
  
   return NtGdiBitBlt(hdcDest, nXOriginDest, nYOriginDest, nWidthDest,
                      nHeightDest, hdcSrc, nXOriginSrc, nYOriginSrc, dwRop);
}
