
#ifndef __WIN32K_TEXT_H
#define __WIN32K_TEXT_H

int  W32kAddFontResource(LPCWSTR  Filename);

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
                      LPCWSTR  Face);

HFONT  W32kCreateFontIndirect(CONST LPLOGFONT lf);

BOOL  W32kCreateScalableFontResource(DWORD  Hidden,
                                     LPCWSTR  FontRes,
                                     LPCWSTR  FontFile,
                                     LPCWSTR  CurrentPath);

int  W32kEnumFontFamilies(HDC  hDC,
                          LPCWSTR  Family,
                          FONTENUMPROC  EnumFontFamProc,
                          LPARAM  lParam);

int  W32kEnumFontFamiliesEx(HDC  hDC,
                            LPLOGFONT  Logfont,
                            FONTENUMPROC  EnumFontFamExProc,
                            LPARAM  lParam,
                            DWORD  Flags);

int  W32kEnumFonts(HDC  hDC,
                   LPCWSTR FaceName,
                   FONTENUMPROC  FontFunc,
                   LPARAM  lParam);

BOOL  W32kExtTextOut(HDC  hDC,
                     int  X,
                     int  Y,
                     UINT  Options,
                     CONST LPRECT  rc,
                     LPCWSTR  String,
                     UINT  Count,
                     CONST LPINT  Dx);

BOOL  W32kGetAspectRatioFilterEx(HDC  hDC,
                                 LPSIZE  AspectRatio);

BOOL  W32kGetCharABCWidths(HDC  hDC,
                           UINT  FirstChar,
                           UINT  LastChar,
                           LPABC  abc);

BOOL  W32kGetCharABCWidthsFloat(HDC  hDC,
                                UINT  FirstChar,
                                UINT  LastChar,
                                LPABCFLOAT  abcF);

DWORD  W32kGetCharacterPlacement(HDC  hDC,
                                 LPCWSTR  String,
                                 int  Count,
                                 int  MaxExtent,
                                 LPGCP_RESULTS  Results,
                                 DWORD  Flags);

BOOL  W32kGetCharWidth(HDC  hDC,
                       UINT  FirstChar,
                       UINT  LastChar,
                       LPINT  Buffer);

BOOL  W32kGetCharWidth32(HDC  hDC,
                         UINT  FirstChar,
                         UINT  LastChar,
                         LPINT  Buffer);

BOOL  W32kGetCharWidthFloat(HDC  hDC,
                            UINT  FirstChar,
                            UINT  LastChar,
                            PFLOAT  Buffer);

DWORD  W32kGetFontLanguageInfo(HDC  hDC);

DWORD  W32kGetGlyphOutline(HDC  hDC,
                           UINT  Char,
                           UINT  Format,
                           LPGLYPHMETRICS  gm,
                           DWORD  Bufsize,
                           LPVOID  Buffer,
                           CONST LPMAT2 mat2);

DWORD  W32kGetKerningPairs(HDC  hDC,
                           DWORD  NumPairs,
                           LPKERNINGPAIR  krnpair);

UINT  W32kGetOutlineTextMetrics(HDC  hDC,
                                UINT  Data,
                                LPOUTLINETEXTMETRIC  otm);

BOOL  W32kGetRasterizerCaps(LPRASTERIZER_STATUS  rs,
                            UINT  Size);

UINT  W32kGetTextCharset(HDC  hDC);

UINT  W32kGetTextCharsetInfo(HDC  hDC,
                             LPFONTSIGNATURE  Sig,
                             DWORD  Flags);

BOOL  W32kGetTextExtentExPoint(HDC  hDC,
                               LPCWSTR String,
                               int  Count,
                               int  MaxExtent,
                               LPINT  Fit,
                               LPINT  Dx,
                               LPSIZE  Size);

BOOL  W32kGetTextExtentPoint(HDC  hDC,
                             LPCWSTR  String,
                             int  Count,
                             LPSIZE  Size);

BOOL  W32kGetTextExtentPoint32(HDC  hDC,
                               LPCWSTR  String,
                               int  Count,
                               LPSIZE  Size);

int  W32kGetTextFace(HDC  hDC,
                     int  Count,
                     LPWSTR  FaceName);

BOOL  W32kGetTextMetrics(HDC  hDC,
                         LPTEXTMETRIC  tm);

BOOL  W32kPolyTextOut(HDC  hDC,
                      CONST LPPOLYTEXT  txt,
                      int  Count);

BOOL  W32kRemoveFontResource(LPCWSTR  FileName);

DWORD  W32kSetMapperFlags(HDC  hDC,
                          DWORD  Flag);

UINT  W32kSetTextAlign(HDC  hDC,
                       UINT  Mode);

COLORREF  W32kSetTextColor(HDC  hDC,
                           COLORREF  Color);

BOOL  W32kSetTextJustification(HDC  hDC,
                               int  BreakExtra,
                               int  BreakCount);

BOOL  W32kTextOut(HDC  hDC,
                  int  XStart,
                  int  YStart,
                  LPCWSTR  String,
                  int  Count);

UINT  W32kTranslateCharsetInfo(PDWORD  Src,
                               LPCHARSETINFO  CSI,   
                               DWORD  Flags);

#endif

