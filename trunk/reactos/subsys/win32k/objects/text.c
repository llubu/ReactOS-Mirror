

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <win32k/text.h>

// #define NDEBUG
#include <internal/debug.h>

int  W32kAddFontResource(LPCTSTR  Filename)
{
  UNIMPLEMENTED;
}

HFONT  W32kCreateFont(int  Height,
                      int  Width,
                      int  Escapement,
                      int  Orientation,
                      int  Weight,
                      DWORD  Italic,
                      DWORD  Underline,
                      DWORD  StrikeOut,
                      DWORD  CharSet,
                      DWORD  OutputPrecision,
                      DWORD  ClipPrecision,
                      DWORD  Quality,
                      DWORD  PitchAndFamily,
                      LPCTSTR  Face)
{
  UNIMPLEMENTED;
}

HFONT  W32kCreateFontIndirect(CONST LPLOGFONT  lf)
{
  UNIMPLEMENTED;
}

BOOL  W32kCreateScalableFontResource(DWORD  Hidden,
                                     LPCTSTR  FontRes,
                                     LPCTSTR  FontFile,
                                     LPCTSTR  CurrentPath)
{
  UNIMPLEMENTED;
}

int  W32kEnumFontFamilies(HDC  hDC,
                          LPCTSTR  Family,
                          FONTENUMPROC  EnumFontFamProc,
                          LPARAM  lParam)
{
  UNIMPLEMENTED;
}

int  W32kEnumFontFamiliesEx(HDC  hDC,
                            LPLOGFONT  Logfont,
                            FONTENUMPROC  EnumFontFamExProc,
                            LPARAM  lParam,
                            DWORD  Flags)
{
  UNIMPLEMENTED;
}

int  W32kEnumFonts(HDC  hDC,
                   LPCTSTR FaceName,
                   FONTENUMPROC  FontFunc,
                   LPARAM  lParam)
{
  UNIMPLEMENTED;
}

BOOL  W32kExtTextOut(HDC  hDC,
                     int  X,
                     int  Y,
                     UINT  Options,
                     CONST LPRECT  rc,
                     LPCTSTR  String,
                     UINT  Count,
                     CONST LPINT  Dx)
{
  UNIMPLEMENTED;
}

BOOL  W32kGetAspectRatioFilterEx(HDC  hDC,
                                 LPSIZE  AspectRatio)
{
  UNIMPLEMENTED;
}

BOOL  W32kGetCharABCWidths(HDC  hDC,
                           UINT  FirstChar,
                           UINT  LastChar,
                           LPABC  abc)
{
  UNIMPLEMENTED;
}

BOOL  W32kGetCharABCWidthsFloat(HDC  hDC,
                                UINT  FirstChar,
                                UINT  LastChar,
                                LPABCFLOAT  abcF)
{
  UNIMPLEMENTED;
}

DWORD  W32kGetCharacterPlacement(HDC  hDC,
                                 LPCTSTR  String,
                                 int  Count,
                                 int  MaxExtent,
                                 LPGCP_RESULTS  Results,
                                 DWORD  Flags)
{
  UNIMPLEMENTED;
}

BOOL  W32kGetCharWidth(HDC  hDC,
                       UINT  FirstChar,
                       UINT  LastChar,
                       LPINT  Buffer)
{
  UNIMPLEMENTED;
}

BOOL  W32kGetCharWidth32(HDC  hDC,
                         UINT  FirstChar,
                         UINT  LastChar,
                         LPINT  Buffer)
{
  UNIMPLEMENTED;
}

BOOL  W32kGetCharWidthFloat(HDC  hDC,
                            UINT  FirstChar,
                            UINT  LastChar,
                            PFLOAT  Buffer)
{
  UNIMPLEMENTED;
}

DWORD  W32kGetFontLanguageInfo(HDC  hDC)
{
  UNIMPLEMENTED;
}

DWORD  W32kGetGlyphOutline(HDC  hDC,
                           UINT  Char,
                           UINT  Format,
                           LPGLYPHMETRICS  gm,
                           DWORD  Bufsize,
                           LPVOID  Buffer,
                           CONST LPMAT2 mat2)
{
  UNIMPLEMENTED;
}

DWORD  W32kGetKerningPairs(HDC  hDC,
                           DWORD  NumPairs,
                           LPKERNINGPAIR  krnpair)
{
  UNIMPLEMENTED;
}

UINT  W32kGetOutlineTextMetrics(HDC  hDC,
                                UINT  Data,
                                LPOUTLINETEXTMETRIC  otm)
{
  UNIMPLEMENTED;
}

BOOL  W32kGetRasterizerCaps(LPRASTERIZER_STATUS  rs,
                            UINT  Size)
{
  UNIMPLEMENTED;
}

BOOL  W32kGetTextExtentExPoint(HDC  hDC,
                               LPCTSTR String,
                               int  Count,
                               int  MaxExtent,
                               LPINT  Fit,
                               LPINT  Dx,
                               LPSIZE  Size)
{
  UNIMPLEMENTED;
}

BOOL  W32kGetTextExtentPoint(HDC  hDC,
                             LPCTSTR  String,
                             int  Count,
                             LPSIZE  Size)
{
  UNIMPLEMENTED;
}

BOOL  W32kGetTextExtentPoint32(HDC  hDC,
                               LPCTSTR  String,
                               int  Count,
                               LPSIZE  Size)
{
  UNIMPLEMENTED;
}

int  W32kGetTextFace(HDC  hDC,
                     int  Count,
                     LPTSTR  FaceName)
{
  UNIMPLEMENTED;
}

BOOL  W32kGetTextMetrics(HDC  hDC,
                         LPTEXTMETRIC  tm)
{
  UNIMPLEMENTED;
}

BOOL  W32kPolyTextOut(HDC  hDC,
                      CONST LPPOLYTEXT  txt,
                      int  Count)
{
  UNIMPLEMENTED;
}

BOOL  W32kRemoveFontResource(LPCTSTR  FileName)
{
  UNIMPLEMENTED;
}

DWORD  W32kSetMapperFlags(HDC  hDC,
                          DWORD  Flag)
{
  UNIMPLEMENTED;
}

UINT  W32kSetTextAlign(HDC  hDC,
                       UINT  Mode)
{
  UNIMPLEMENTED;
}

COLORREF  W32kSetTextColor(HDC  hDC,
                           COLORREF  Color)
{
  UNIMPLEMENTED;
}

BOOL  W32kSetTextJustification(HDC  hDC,
                               int  BreakExtra,
                               int  BreakCount)
{
  UNIMPLEMENTED;
}

BOOL  W32kTextOut(HDC  hDC,
                  int  XStart,
                  int  YStart,
                  LPCTSTR  String,
                  int  Count)
{
  UNIMPLEMENTED;
}

