#ifndef _WIN32K_INTGDI_H
#define _WIN32K_INTGDI_H

#include "region.h"

/* Brush functions */

XLATEOBJ* FASTCALL
IntGdiCreateBrushXlate(PDC Dc, GDIBRUSHOBJ *BrushObj, BOOLEAN *Failed);

VOID FASTCALL
IntGdiInitBrushInstance(GDIBRUSHINST *BrushInst, PGDIBRUSHOBJ BrushObj, XLATEOBJ *XlateObj);

HBRUSH STDCALL
IntGdiCreateDIBBrush(
   CONST BITMAPINFO *BitmapInfo,
   UINT ColorSpec,
   UINT BitmapInfoSize,
   CONST VOID *PackedDIB);

HBRUSH STDCALL
IntGdiCreateHatchBrush(
   INT Style,
   COLORREF Color);

HBRUSH STDCALL
IntGdiCreatePatternBrush(
   HBITMAP hBitmap);

HBRUSH STDCALL
IntGdiCreateSolidBrush(
   COLORREF Color);

HBRUSH STDCALL
IntGdiCreateNullBrush(VOID);

BOOL FASTCALL
IntPatBlt(
   PDC dc,
   INT XLeft,
   INT YLeft,
   INT Width,
   INT Height,
   DWORD ROP,
   PGDIBRUSHOBJ BrushObj);

/* Pen functions */

HPEN FASTCALL
IntGdiCreatePenIndirect(PLOGPEN lgpn);

/* Line functions */

BOOL FASTCALL
IntGdiLineTo(DC  *dc,
             int XEnd,
             int YEnd);

BOOL FASTCALL
IntGdiMoveToEx(DC      *dc,
               int     X,
               int     Y,
               LPPOINT Point);

BOOL FASTCALL
IntGdiPolyBezier(DC      *dc,
                 LPPOINT pt,
                 DWORD   Count);

BOOL FASTCALL
IntGdiPolyline(DC      *dc,
               LPPOINT pt,
               int     Count);

BOOL FASTCALL
IntGdiPolyBezierTo(DC      *dc,
                   LPPOINT pt,
                   DWORD   Count);

BOOL FASTCALL
IntGdiPolyPolyline(DC      *dc,
                   LPPOINT pt,
                   LPDWORD PolyPoints,
                   DWORD   Count);

BOOL FASTCALL
IntGdiPolylineTo(DC      *dc,
                 LPPOINT pt,
                 DWORD   Count);

BOOL FASTCALL
IntGdiArc(DC  *dc,
          int LeftRect,
          int TopRect,
          int RightRect,
          int BottomRect,
          int XStartArc,
          int YStartArc,
          int XEndArc,
          int YEndArc);

INT FASTCALL
IntGdiGetArcDirection(DC *dc);

/* Shape functions */

BOOL FASTCALL
IntGdiPolygon(PDC    dc,
              PPOINT UnsafePoints,
              int    Count);

BOOL FASTCALL
IntGdiPolyPolygon(DC      *dc,
                  LPPOINT Points,
                  LPINT   PolyCounts,
                  int     Count);

/* Rgn functions */

int FASTCALL
IntGdiGetClipBox(HDC    hDC,
			     LPRECT rc);

HRGN FASTCALL REGION_CropRgn(HRGN hDst, HRGN hSrc, const PRECT lpRect, PPOINT lpPt);
void FASTCALL REGION_UnionRectWithRegion(const RECT *rect, ROSRGNDATA *rgn);
INT FASTCALL UnsafeIntGetRgnBox(PROSRGNDATA Rgn, LPRECT pRect);
BOOL FASTCALL UnsafeIntRectInRegion(PROSRGNDATA Rgn, CONST LPRECT rc);

#define UnsafeIntCreateRectRgnIndirect(prc) \
  NtGdiCreateRectRgn((prc)->left, (prc)->top, (prc)->right, (prc)->bottom)

#define UnsafeIntUnionRectWithRgn(rgndest, prc) \
  REGION_UnionRectWithRegion((prc), (rgndest))

/* DC functions */

BOOL FASTCALL
IntGdiGetDCOrgEx(DC *dc, LPPOINT  Point);

INT FASTCALL
IntGdiGetObject(HANDLE handle, INT count, LPVOID buffer);

HDC FASTCALL
IntGdiCreateDC(PUNICODE_STRING Driver,
               PUNICODE_STRING Device,
               PUNICODE_STRING Output,
               CONST PDEVMODEW InitData,
               BOOL CreateAsIC);

COLORREF FASTCALL
IntGetDCColor(HDC hDC, ULONG Object);

COLORREF FASTCALL
IntSetDCColor(HDC hDC, ULONG Object, COLORREF Color);

/* Coord functions */

BOOL FASTCALL
IntGdiCombineTransform(LPXFORM XFormResult,
                       LPXFORM xform1,
                       LPXFORM xform2);

/* RECT functions */

VOID FASTCALL
IntGdiSetRect(PRECT Rect, INT left, INT top, INT right, INT bottom);

VOID FASTCALL
IntGdiSetEmptyRect(PRECT Rect);

BOOL FASTCALL
IntGdiIsEmptyRect(const RECT* Rect);

VOID FASTCALL
IntGdiOffsetRect(LPRECT Rect, INT x, INT y);

BOOL FASTCALL
IntGdiUnionRect(PRECT Dest, const RECT* Src1, const RECT* Src2);

BOOL FASTCALL
IntGdiIntersectRect(PRECT Dest, const RECT* Src1, const RECT* Src2);

/* Stock objects */

BOOL FASTCALL
IntSetSysColors(UINT nColors, INT *Elements, COLORREF *Colors);

BOOL FASTCALL
IntGetSysColorBrushes(HBRUSH *Brushes, UINT nBrushes);

BOOL FASTCALL
IntGetSysColorPens(HPEN *Pens, UINT nPens);

BOOL FASTCALL
IntGetSysColors(COLORREF *Colors, UINT nColors);

/* Other Stuff */

INT FASTCALL
IntGdiGetDeviceCaps(PDC dc, INT Index);

int STDCALL IntGdiExtSelectClipRgn (PDC dc, HRGN hrgn, int fnMode);

INT
FASTCALL
IntGdiEscape(PDC    dc,
             INT    Escape,
             INT    InSize,
             LPCSTR InData,
             LPVOID OutData);

BOOL
FASTCALL
IntEnumDisplaySettings(
  IN PUNICODE_STRING pDeviceName  OPTIONAL,
  IN DWORD iModeNum,
  IN OUT LPDEVMODEW pDevMode,
  IN DWORD dwFlags);

LONG
FASTCALL
IntChangeDisplaySettings(
  IN PUNICODE_STRING pDeviceName  OPTIONAL,
  IN LPDEVMODEW pDevMode,
  IN DWORD dwflags,
  IN PVOID lParam  OPTIONAL);

HBITMAP
FASTCALL
IntCreateCompatibleBitmap(PDC Dc,
                          INT Width,
                          INT Height);

#endif /* _WIN32K_INTGDI_H */

