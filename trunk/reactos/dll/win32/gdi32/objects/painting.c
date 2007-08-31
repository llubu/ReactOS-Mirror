#include "precomp.h"


#if 0 /* FIXME: enable this as soon as we have working usermode gdi */

BOOL
STDCALL
LineTo( HDC hDC, INT x, INT y )
{
// Handle something other than a normal dc object.
 if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
 {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
      return MFDRV_MetaParam2( hDC, META_LINETO, x, y);
    else
    {
      PLDC pLDC = GdiGetLDC(hDC);
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return FALSE;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
        return MFDRV_LineTo( hDC, x, y )
      }
      return FALSE;
    }
 }
 return NtGdiLineTo( hDC, x, y);
}


BOOL
STDCALL
MoveToEx( HDC hDC, INT x, INT y, LPPOINT Point )
{
 PDC_ATTR Dc_Attr;
 
 if (!GdiGetHandleUserData((HGDIOBJ) hDC, (PVOID) &Dc_Attr)) return FALSE;
 
 if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
 {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
      return MFDRV_MetaParam2( hDC, META_MOVETO, x, y);
    else
    {
      PLDC pLDC = Dc_Attr->pvLDC;
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return FALSE;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
        if (!EMFDRV_MoveTo( hDC, x, y)) return FALSE;
      }
    }
 }

 if ( Point )
 {
    if ( Dc_Attr->ulDirty_ & DIRTY_PTLCURRENT ) // Double hit!
    {
       Point->x = Dc_Attr->ptfxCurrent.x; // ret prev before change.
       Point->y = Dc_Attr->ptfxCurrent.y;
       DPtoLP ( hDC, Point, 1);          // reconvert back.
    }
    else
    {
       Point->x = Dc_Attr->ptlCurrent.x;
       Point->y = Dc_Attr->ptlCurrent.y;
    }
 }

 Dc_Attr->ptlCurrent.x = x;
 Dc_Attr->ptlCurrent.y = y;
 
 Dc_Attr->ulDirty_ |= ( DIRTY_PTLCURRENT|DIRTY_STYLESTATE); // Set dirty
 return TRUE;
}


BOOL
STDCALL
Ellipse(HDC hDC, INT Left, INT Top, INT Right, INT Bottom)
{
// Handle something other than a normal dc object.
 if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
 {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
      return MFDRV_MetaParam4(hDC, META_ELLIPSE, Left, Top, Right, Bottom );
    else
    {
      PLDC pLDC = GdiGetLDC(hDC);
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return FALSE;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
        return EMFDRV_Ellipse( hDC, Left, Top, Right, Bottom );
      }
      return FALSE;
    }
 }
 return NtGdiEllipse( hDC, Left, Top, Right, Bottom);
}


BOOL
STDCALL
Rectangle(HDC, INT Left, INT Top, INT Right, INT Bottom)
{
// Handle something other than a normal dc object.
 if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
 {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
      return MFDRV_MetaParam4(hDC, META_RECTANGLE, Left, Top, Right, Bottom );
    else
    {
      PLDC pLDC = GdiGetLDC(hDC);
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return FALSE;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
        return EMFDRV_Rectangle( hDC, Left, Top, Right, Bottom );
      }
      return FALSE;
    }
 }
 return NtGdiRectangle( hDC, Left, Top, Right, Bottom);
}


BOOL
STDCALL
RoundRect(HDC, INT Left, INT Top, INT Right, INT Bottom, 
                                                INT ell_Width, INT ell_Height)
{
// Handle something other than a normal dc object.
 if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
 {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
      return MFDRV_MetaParam6( hDC, META_ROUNDRECT, Left, Top, Right, Bottom,
                                                      ell_Width, ell_Height  );
    else
    {
      PLDC pLDC = GdiGetLDC(hDC);
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return FALSE;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
        return EMFDRV_RoundRect( hDC, Left, Top, Right, Bottom, 
                                                      ell_Width, ell_Height );
      }
      return FALSE;
    }
 }
 return NtGdiRoundRect( hDc, Left, Top, Right, Bottom, ell_Width, ell_Height);
}


COLORREF
STDCALL
GetPixel( HDC hDC, INT x, INT y )
{
 if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC) return CLR_INVALID;
 if (!GdiIsHandleValid((HGDIOBJ) hDC)) return CLR_INVALID;
 return NtGdiGetPixel( hDC, x, y);
}


COLORREF
STDCALL
SetPixel( HDC hDC, INT x, INT y, COLORREF Color )
{
// Handle something other than a normal dc object.
 if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
 {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
      return MFDRV_MetaParam4(hDC, META_SETPIXEL, x, y, HIWORD(Color),
                                                              LOWORD(Color));
    else
    {
      PLDC pLDC = GdiGetLDC(hDC);
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return 0;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
        return EMFDRV_SetPixel( hDC, x, y, Color );
      }
      return 0;
    }
 }
 return NtGdiSetPixel( hDC, x, y, Color);
}


BOOL
STDCALL
SetPixelV( HDC hDC, INT x, INT y, COLORREF Color )
{
   COLORREF Cr = SetPixel( hDC, x, y, Color );
   if (Cr) return TRUE;
   return FALSE;
}


BOOL
STDCALL
FillRgn( HDC hDC, HRGN hRgn, HBRUSH hBrush )
{

 if ( (!hRgn) || (!hBrush) ) return FALSE;
 
// Handle something other than a normal dc object.
 if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
 {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
      return MFDRV_FillRgn( hDC, hRgn, hBrush);
    else
    {
      PLDC pLDC = GdiGetLDC(hDC);
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return FALSE;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
        return EMFDRV_FillRgn(( hDC, hRgn, hBrush);
      }
      return FALSE;
    }
 }
 return NtGdiFillRgn( hDC, hRgn, hBrush);
}


BOOL
STDCALL
FrameRgn( HDC hDC, HRGN hRgn, HBRUSH hBrush, INT nWidth, INT nHeight )
{

 if ( (!hRgn) || (!hBrush) ) return FALSE;
 
// Handle something other than a normal dc object.
 if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
 {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
      return MFDRV_FrameRgn( hDC, hRgn, hBrush, nWidth, nHeight );
    else
    {
      PLDC pLDC = GdiGetLDC(hDC);
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return FALSE;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
        return EMFDRV_FrameRgn( hDC, hRgn, hBrush, nWidth, nHeight );
      }
      return FALSE;
    }
 }
 return NtGdiFrameRgn( hDC, hRgn, hBrush, nWidth, nHeight);
}


BOOL
STDCALL
InvertRgn( HDC hDC, HRGN hRgn )
{

 if ( !hRgn ) return FALSE;
 
// Handle something other than a normal dc object.
 if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
 {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
      return MFDRV_InvertRgn( hDC, HRGN hRgn ); // Use this instead of MFDRV_MetaParam.
    else
    {  
      PLDC pLDC = GdiGetLDC(hDC);
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return FALSE;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
        return EMFDRV_PaintInvertRgn( hDC, hRgn, EMR_INVERTRGN );
      }
      return FALSE;
    }
 }
 return NtGdiInvertRgn( hDC, hRgn);
}

BOOL
STDCALL
PaintRgn( HDC hDC, HRGN hRgn )
{
// Handle something other than a normal dc object.
 if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
 {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
      return MFDRV_PaintRgn( hDC, HRGN hRgn ); // Use this instead of MFDRV_MetaParam.
    else
    {
      PLDC pLDC = GdiGetLDC(hDC);
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return FALSE;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
        return EMFDRV_PaintInvertRgn( hDC, hRgn, EMR_PAINTRGN );
      }
      return FALSE;
    }
 }
 // Could just use Dc_Attr->hbrush
 HBRUSH hbrush = (HBRUSH) GetDCObject( hDC, GDI_OBJECT_TYPE_BRUSH);
 
 return NtGdiFillRgn( hDC, hRgn, hBrush);
}

#endif

BOOL
STDCALL
PolyBezier(HDC hDC ,const POINT* Point, DWORD cPoints)
{
#if 0
// Handle something other than a normal dc object.
 if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
 {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
 /*
  * Since MetaFiles don't record Beziers and they don't even record
  * approximations to them using lines.
  */
      return FALSE;
    else
    {
      PLDC pLDC = GdiGetLDC(hDC);
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return FALSE;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
        return FALSE; // Not supported yet.
      }
      return FALSE;
    }
 }
#endif
 return NtGdiPolyPolyDraw( hDC ,(PPOINT) Point, &cPoints, 1, GdiPolyBezier );
}
 

BOOL
STDCALL
PolyBezierTo(HDC hDC, const POINT* Point ,DWORD cPoints)
{
#if 0
// Handle something other than a normal dc object.
 if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
 {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
      return FALSE;
    else
    {
      PLDC pLDC = GdiGetLDC(hDC);
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return FALSE;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
        return FALSE; // Not supported yet.
      }
      return FALSE;
    }
 }
#endif
 return NtGdiPolyPolyDraw( hDC , (PPOINT) Point, &cPoints, 1, GdiPolyBezierTo );
}


BOOL
STDCALL
PolyDraw(HDC hDC, const POINT* Point, const BYTE *lpbTypes, int cCount )
{
#if 0
// Handle something other than a normal dc object.
 if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
 {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
      return FALSE;
    else
    { 
      PLDC pLDC = GdiGetLDC(hDC);
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return FALSE;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
        return FALSE; // Not supported yet.
      }
      return FALSE;
    }
 }
#endif
 return NtGdiPolyDraw( hDC , (PPOINT) Point, (PBYTE)lpbTypes, cCount );
}


BOOL
STDCALL
Polygon(HDC hDC, const POINT *Point, int Count)
{
#if 0
// Handle something other than a normal dc object.
 if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
 {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
      return MFDRV_Polygon( hDC, Point, Count );
    else
    {
      PLDC pLDC = GdiGetLDC(hDC);
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return FALSE;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
        return EMFDRV_Polygon( hDC, Point, Count );
      }
      return FALSE;
    }
 }
#endif
 return NtGdiPolyPolyDraw( hDC , (PPOINT) Point, (PULONG)&Count, 1, GdiPolyPolygon );
}


BOOL
STDCALL
Polyline(HDC hDC, const POINT *Point, int Count)
{
#if 0
// Handle something other than a normal dc object.
 if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
 {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
      return MFDRV_Polyline( hDC, Point, Count );
    else
    {
      PLDC pLDC = GdiGetLDC(hDC);
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return FALSE;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
        return EMFDRV_Polyline( hDC, Point, Count );
      }
      return FALSE;
    }
 }
#endif
 return NtGdiPolyPolyDraw( hDC , (PPOINT) Point, (PULONG)&Count, 1, GdiPolyPolyLine );
}


BOOL
STDCALL
PolylineTo(HDC hDC, const POINT* Point, DWORD Count)
{
#if 0
// Handle something other than a normal dc object.
 if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
 {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
      return FALSE;
    else
    {
      PLDC pLDC = GdiGetLDC(hDC);
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return FALSE;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
        return FALSE; // Not supported yet.
      }
      return FALSE;
    }
 }
#endif
 return NtGdiPolyPolyDraw( hDC , (PPOINT) Point, &Count, 1, GdiPolyLineTo ); 
}


BOOL
STDCALL
PolyPolygon(HDC hDC, const POINT* Point, const INT* Count, int Polys)
{
#if 0
// Handle something other than a normal dc object.
 if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
 {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
      return MFDRV_PolyPolygon( hDC, Point, Count, Polys);
    else
    {
      PLDC pLDC = GdiGetLDC(hDC);
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return FALSE;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
        return EMFDRV_PolyPolygon( hDC, Point, Count, Polys );
      }
      return FALSE;
    }
 }
#endif
 return NtGdiPolyPolyDraw( hDC , (PPOINT)Point, (PULONG)Count, Polys, GdiPolyPolygon );
}


BOOL
STDCALL
PolyPolyline(HDC hDC, const POINT* Point, const DWORD* Counts, DWORD Polys)
{
#if 0
// Handle something other than a normal dc object.
 if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
 {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
      return FALSE;
    else
    {
      PLDC pLDC = GdiGetLDC(hDC);
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return FALSE;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
        return EMFDRV_PolyPolyline(hDC, Point, Counts, Polys);
      }
      return FALSE;
    }
 }
#endif
 return NtGdiPolyPolyDraw( hDC , (PPOINT)Point, (PULONG)Counts, Polys, GdiPolyPolyLine );
}


BOOL
STDCALL
ExtFloodFill(
       HDC hDC,
       int nXStart, 
       int nYStart,
       COLORREF crFill,
       UINT fuFillType
             )
{
#if 0
// Handle something other than a normal dc object.
 if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
 {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
      return MFDRV_ExtFloodFill( hDC, nXStart, nYStart, crFill, fuFillType );
    else
    {
      PLDC pLDC = GdiGetLDC(hDC);
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return FALSE;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
        return EMFDRV_ExtFloodFill( hDC, nXStart, nYStart, crFill, fuFillType );
      }
      return FALSE;
    }
 }
#endif
    return NtGdiExtFloodFill(hDC, nXStart, nYStart, crFill, fuFillType);
}


BOOL
WINAPI
FloodFill(
    HDC hDC,
    int nXStart,
    int nYStart,
    COLORREF crFill)
{
    return ExtFloodFill(hDC, nXStart, nYStart, crFill, FLOODFILLBORDER);
}

BOOL WINAPI
MaskBlt(
	HDC hdcDest,
	INT nXDest,
	INT nYDest,
	INT nWidth,
	INT nHeight,
	HDC hdcSrc,
	INT nXSrc,
	INT nYSrc,
	HBITMAP hbmMask,
	INT xMask,
	INT yMask,
	DWORD dwRop)
{
	return NtGdiMaskBlt(hdcDest,
	                    nXDest,
	                    nYDest,
	                    nWidth,
	                    nHeight,
	                    hdcSrc,
	                    nXSrc,
	                    nYSrc,
	                    hbmMask,
	                    xMask,
	                    yMask,
	                    dwRop,
	                    0);
}


BOOL
WINAPI
PlgBlt(
	HDC hdcDest,
	const POINT *lpPoint,
	HDC hdcSrc,
	INT nXSrc,
	INT nYSrc,
	INT nWidth,
	INT nHeight,
	HBITMAP hbmMask,
	INT xMask,
	INT yMask)
{
	return NtGdiPlgBlt(hdcDest,
	                   (LPPOINT)lpPoint,
	                   hdcSrc,
	                   nXSrc,
	                   nYSrc,
	                   nWidth,
	                   nHeight,
	                   hbmMask,
	                   xMask,
	                   yMask,
	                   0);
}
