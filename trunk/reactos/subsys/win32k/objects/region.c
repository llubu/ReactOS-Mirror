

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <win32k/region.h>

// #define NDEBUG
#include <internal/debug.h>

INT  W32kCombineRgn(HRGN  hDest,
                    HRGN  hSrc1,
                    HRGN  hSrc2,
                    INT  CombineMode)
{
  UNIMPLEMENTED;
}

HRGN  W32kCreateEllipticRgn(INT  LeftRect,
                            INT  TopRect,
                            INT  RightRect,
                            INT  BottomRect)
{
  UNIMPLEMENTED;
}

HRGN  W32kCreateEllipticRgnIndirect(CONST PRECT  rc)
{
  UNIMPLEMENTED;
}

HRGN  W32kCreatePolygonRgn(CONST PPOINT  pt,
                           INT  Count,
                           INT  PolyFillMode)
{
  UNIMPLEMENTED;
}

HRGN  W32kCreatePolyPolygonRgn(CONST PPOINT  pt,
                               CONST PINT  PolyCounts,
                               INT  Count,
                               INT  PolyFillMode)
{
  UNIMPLEMENTED;
}

HRGN  W32kCreateRectRgn(INT  LeftRect,
                        INT  TopRect,
                        INT  RightRect,
                        INT  BottomRect)
{
  UNIMPLEMENTED;
}

HRGN  W32kCreateRectRgnIndirect(CONST PRECT  rc)
{
  UNIMPLEMENTED;
}

HRGN  W32kCreateRoundRectRgn(INT  LeftRect,
                             INT  TopRect,
                             INT  RightRect,
                             INT  BottomRect,
                             INT  WidthEllipse,
                             INT  HeightEllipse)
{
  UNIMPLEMENTED;
}

BOOL  W32kEqualRgn(HRGN  hSrcRgn1,
                   HRGN  hSrcRgn2)
{
  UNIMPLEMENTED;
}

HRGN  W32kExtCreateRegion(CONST PXFORM  Xform,
                          DWORD  Count,
                          CONST PRGNDATA  RgnData)
{
  UNIMPLEMENTED;
}

BOOL  W32kFillRgn(HDC  hDC,
                  HRGN  hRgn,
                  HBRUSH  hBrush)
{
  UNIMPLEMENTED;
}

BOOL  W32kFrameRgn(HDC  hDC,
                   HRGN  hRgn,
                   HBRUSH  hBrush,
                   INT  Width,
                   INT  Height)
{
  UNIMPLEMENTED;
}

INT  W32kGetRgnBox(HRGN  hRgn,
                   LPRECT  hRect)
{
  UNIMPLEMENTED;
}

BOOL  W32kInvertRgn(HDC  hDC,
                    HRGN  hRgn)
{
  UNIMPLEMENTED;
}

INT  W32kOffsetRgn(HRGN  hRgn,
                   INT  XOffset,
                   INT  YOffset)
{
  UNIMPLEMENTED;
}

BOOL  W32kPaintRgn(HDC  hDC,
                   HRGN  hRgn)
{
  UNIMPLEMENTED;
}

BOOL  W32kPtInRegion(HRGN  hRgn,
                     INT  X,
                     INT  Y)
{
  UNIMPLEMENTED;
}

BOOL  W32kRectInRegion(HRGN  hRgn,
                       CONST LPRECT  rc)
{
  UNIMPLEMENTED;
}

BOOL  W32kSetRectRgn(HRGN  hRgn,
                     INT  LeftRect,
                     INT  TopRect,
                     INT  RightRect,
                     INT  BottomRect)
{
  UNIMPLEMENTED;
}

