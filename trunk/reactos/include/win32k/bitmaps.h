
#ifndef __WIN32K_BITMAPS_H
#define __WIN32K_BITMAPS_H

#include <win32k/dc.h>
#include <win32k/gdiobj.h>

typedef struct _DDBITMAP
{
  const PDRIVER_FUNCTIONS  pDriverFunctions;
//  DHPDEV  PDev;
//  HSURF  Surface;
} DDBITMAP;

/* GDI logical bitmap object */
typedef struct _BITMAPOBJ
{
  GDIOBJHDR   header;
  BITMAP      bitmap;
  SIZE        size;   /* For SetBitmapDimension() */
  
  DDBITMAP   *DDBitmap;

  /* For device-independent bitmaps: */
  DIBSECTION *dib;
} BITMAPOBJ, *PBITMAPOBJ;

/*  Internal interface  */

#define  BITMAPOBJ_AllocBitmap()  \
  ((PBITMAPOBJ) GDIOBJ_AllocObject(sizeof(BITMAPOBJ), GO_BITMAP_MAGIC))
#define  BITMAPOBJ_FreeBitmap(hBMObj)  GDIOBJ_FreeObject((HGDIOBJ)hBMObj)
#define  BITMAPOBJ_HandleToPtr(hBMObj)  \
  ((PBITMAPOBJ) GDIOBJ_HandleToPtr((HGDIOBJ)hBMObj, GO_BITMAP_MAGIC))
#define  BITMAPOBJ_PtrToHandle(hBMObj)  \
  ((HBITMAP) GDIOBJ_HandleToPtr((PGDIOBJ)hBMObj, GO_BITMAP_MAGIC))
#define  BITMAPOBJ_LockBitmap(hBMObj) GDIOBJ_LockObject((HGDIOBJ)hBMObj)
#define  BITMAPOBJ_UnlockBitmap(hBMObj) GDIOBJ_UnlockObject((HGDIOBJ)hBMObj)

INT  BITMAP_GetWidthBytes (INT bmWidth, INT bpp);

/*  User Entry Points  */
BOOL  W32kBitBlt(HDC  hDCDest,
                 INT  XDest,
                 INT  YDest,
                 INT  Width,
                 INT  Height,
                 HDC  hDCSrc,
                 INT  XSrc,
                 INT  YSrc,
                 DWORD  ROP);
HBITMAP  W32kCreateBitmap(INT  Width,
                          INT  Height,
                          UINT  Planes,
                          UINT  BitsPerPel,
                          CONST VOID *Bits);
HBITMAP  W32kCreateCompatibleBitmap(HDC hDC,
                                    INT  Width,
                                    INT  Height);
HBITMAP  W32kCreateBitmapIndirect(CONST BITMAP  *BM);
HBITMAP  W32kCreateDIBitmap(HDC  hDC,
                            CONST BITMAPINFOHEADER  *bmih,
                            DWORD  Init,
                            CONST VOID  *bInit,
                            CONST BITMAPINFO  *bmi,
                            UINT  Usage);
HBITMAP  W32kCreateDIBSection(HDC hDC,
                              CONST BITMAPINFO  *bmi,
                              UINT  Usage,
                              VOID  *Bits,
                              HANDLE hSection,
                              DWORD dwOffset);
HBITMAP  W32kCreateDiscardableBitmap(HDC  hDC,
                                     INT  Width,
                                     INT  Height);
BOOL  W32kExtFloodFill(HDC  hDC,
                       INT  XStart,
                       INT  YStart,
                       COLORREF  Color, 
                       UINT  FillType);
BOOL  W32kFloodFill(HDC  hDC,
                    INT  XStart,
                    INT  YStart,
                    COLORREF  Fill);
LONG  W32kGetBitmapBits(HBITMAP  hBitmap,
                        LONG  Buffer,
                        LPVOID  Bits);
BOOL  W32kGetBitmapDimensionEx(HBITMAP  hBitmap,
                               LPSIZE  Dimension);
UINT  W32kGetDIBColorTable(HDC  hDC,
                           UINT  StartIndex,
                           UINT  Entries,
                           RGBQUAD  *Colors);
INT  W32kGetDIBits(HDC  hDC,
                   HBITMAP hBitmap,
                   UINT  StartScan,
                   UINT  ScanLines,
                   LPVOID  Bits,
                   LPBITMAPINFO   bi,
                   UINT  Usage);
COLORREF  W32kGetPixel(HDC  hDC,
                       INT  XPos,
                       INT  YPos);
BOOL  W32kMaskBlt(HDC  hDCDest,
                  INT  XDest,
                  INT  YDest,
                  INT  Width,
                  INT  Height,
                  HDC  hDCSrc,
                  INT  XSrc, 
                  INT  YSrc,
                  HBITMAP  hMaskBitmap,
                  INT  xMask,
                  INT  yMask,
                  DWORD  ROP);
BOOL W32kPlgBlt(HDC  hDCDest,
                CONST POINT  *Point,
                HDC  hDCSrc, 
                INT  XSrc,  
                INT  YSrc,  
                INT  Width, 
                INT  Height,
                HBITMAP  hMaskBitmap,
                INT  xMask,      
                INT  yMask);
LONG  W32kSetBitmapBits(HBITMAP  hBitmap,
                        DWORD  Bytes,
                        CONST VOID *Bits);
BOOL  W32kSetBitmapDimensionEx(HBITMAP  hBitmap,
                               INT  Width,
                               INT  Height,
                               LPSIZE  Size);
UINT  W32kSetDIBColorTable(HDC  hDC,
                           UINT  StartIndex,
                           UINT  Entries,
                           CONST RGBQUAD  *Colors);
INT  W32kSetDIBits(HDC  hDC,
                   HBITMAP  hBitmap,
                   UINT  StartScan,
                   UINT  ScanLines,
                   CONST VOID  *Bits,
                   CONST BITMAPINFO  *bmi,
                   UINT  ColorUse);
INT  W32kSetDIBitsToDevice(HDC  hDC,
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
                           UINT  ColorUse);
COLORREF  W32kSetPixel(HDC  hDC,
                       INT  X,
                       INT  Y,
                       COLORREF  Color);
BOOL  W32kSetPixelV(HDC  hDC,
                    INT  X,
                    INT  Y,
                    COLORREF  Color);
BOOL  W32kStretchBlt(HDC  hDCDest,
                     INT  XOriginDest,
                     INT  YOriginDest,
                     INT  WidthDest,
                     INT  HeightDest,
                     HDC  hDCSrc,
                     INT  XOriginSrc,
                     INT  YOriginSrc,
                     INT  WidthSrc,    
                     INT  HeightSrc, 
                     DWORD  ROP);
INT  W32kStretchDIBits(HDC  hDC,
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
                       DWORD  ROP);


#endif

