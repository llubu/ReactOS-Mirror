/*
 * WinDDI.h - definition of the GDI - DDI interface
 */

#ifndef __DDK_WINDDI_H
#define __DDK_WINDDI_H

#if defined(WIN32_LEAN_AND_MEAN) && defined(_GNU_H_WINDOWS32_STRUCTURES)
#error "windows.h cannot be included before winddi.h if WIN32_LEAN_AND_MEAN is defined"
#endif

#ifdef WIN32_LEAN_AND_MEAN
#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#define WIN32_LEAN_AND_MEAN
#else
#include <windows.h>
#endif

#ifndef IN
#define IN
#define OUT
#define OPTIONAL
#endif

#ifndef PTRDIFF
typedef DWORD PTRDIFF;
#endif

/* FIXME: find definitions for these structs  */
typedef PVOID  PCOLORADJUSTMENT;
typedef PVOID  PDD_CALLBACKS;
typedef PVOID  PDD_HALINFO;
typedef PVOID  PDD_PALETTECALLBACKS;
typedef PVOID  PDD_SURFACECALLBACKS;
typedef PVOID  PFONTINFO;
typedef PVOID  PGAMMA_TABLES;
typedef PVOID  PGLYPHDATA;
typedef PVOID  PLINEATTRS;
typedef DWORD  MIX;
typedef DWORD  ROP4;
typedef PVOID  PSTROBJ;
typedef PVOID  PTTPOLYGONHEADER;
typedef PVOID  PVIDEOMEMORY;

#define  DDI_DRIVER_VERSION  0x00010000

/* FIXME: how big should this constant be?  */
#define  HS_DDI_MAX  6

/*  EngCreateBitmap format types  */
enum _BMF_TYPES
{
  BMF_1BPP = 1,
  BMF_4BPP,
  BMF_8BPP,
  BMF_16BPP, 
  BMF_24BPP, 
  BMF_32BPP, 
  BMF_4RLE, 
  BMF_8RLE
};

#define  BMF_TOPDOWN     0x00000001
#define  BMF_NOZEROINIT  0x00000002
#define  BMF_DONTCACHE   0x00000004
#define  BMF_USERMEM     0x00000008
#define  BMF_KMSECTION   0x00000010

/*  Options for CLIPOBJ_cEnumStart BuildOrder field  */
enum _CD_ORDERS
{
  CD_RIGHTDOWN,
  CD_LEFTDOWN,
  CD_RIGHTUP,
  CD_LEFTUP,
  CD_ANY
};

/*  Options for CLIPOBJ_cEnumStart Type field  */
#define  CT_RECTANGLE  1

#define  DCR_SOLID     0
#define  DCR_DRIVER    1
#define  DCR_HALFTONE  2

#define  DMMAXDEVICENAME  32
#define  DMMAXFORMNAME  32

#define  DM_DEFAULT     0x00000001
#define  DM_MONOCHROME  0x00000002

#define  ED_ABORTDOC  0x00000001

enum _ESCAPE_CODES
{
  ESC_PASSTHROUGH,
  ESC_QUERYESCSUPPORT 
};

#define  FM_INFO_TECH_TRUETYPE              0x00000001
#define  FM_INFO_TECH_BITMAP                0x00000002
#define  FM_INFO_TECH_STROKE                0x00000004
#define  FM_INFO_TECH_OUTLINE_NOT_TRUETYPE  0x00000008
#define  FM_INFO_ARB_XFORMS                 0x00000010
#define  FM_INFO_1BPP                       0x00000020
#define  FM_INFO_4BPP                       0x00000040
#define  FM_INFO_8BPP                       0x00000080
#define  FM_INFO_16BPP                      0x00000100
#define  FM_INFO_24BPP                      0x00000200
#define  FM_INFO_32BPP                      0x00000400
#define  FM_INFO_INTEGER_WIDTH              0x00000800
#define  FM_INFO_CONSTANT_WIDTH             0x00001000
#define  FM_INFO_NOT_CONTIGUOUS             0x00002000
#define  FM_INFO_PID_EMBEDDED               0x00004000
#define  FM_INFO_RETURNS_OUTLINES           0x00008000
#define  FM_INFO_RETURNS_STROKES            0x00010000
#define  FM_INFO_RETURNS_BITMAPS            0x00020000
#define  FM_INFO_UNICODE_COMPLIANT          0x00040000
#define  FM_INFO_RIGHT_HANDED               0x00080000
#define  FM_INFO_INTEGRAL_SCALING           0x00100000
#define  FM_INFO_90DEGREE_ROTATIONS         0x00200000
#define  FM_INFO_OPTICALLY_FIXED_PITCH      0x00400000
#define  FM_INFO_DO_NOT_ENUMERATE           0x00800000
#define  FM_INFO_ISOTROPIC_SCALING_ONLY     0x01000000
#define  FM_INFO_ANISOTROPIC_SCALING_ONLY   0x02000000
#define  FM_INFO_TID_EMBEDDED               0x04000000
#define  FM_INFO_FAMILY_EQUIV               0x08000000
#define  FM_INFO_DBCS_FIXED_PITCH           0x10000000
#define  FM_INFO_NONNEGATIVE_AC             0x20000000
#define  FM_INFO_IGNORE_TC_RA_ABLE          0x40000000
#define  FM_INFO_TECH_TYPE1                 0x80000000

#define  FM_SEL_ITALIC      0x00000001
#define  FM_SEL_UNDERSCORE  0x00000002
#define  FM_SEL_NEGATIVE    0x00000004
#define  FM_SEL_OUTLINED    0x00000008
#define  FM_SEL_STRIKEOUT   0x00000010
#define  FM_SEL_BOLD        0x00000020
#define  FM_SEL_REGULAR     0x00000040

#define  FM_TYPE_LICENSED   0x00000002
#define  FM_READONLY_EMBED  0x00000004
#define  FM_EDITABLE_EMBED  0x00000008
#define  FM_NO_EMBEDDING    0x00000002

#define  FO_TYPE_RASTER    RASTER_FONTTYPE
#define  FO_TYPE_DEVICE    DEVICE_FONTTYPE
#define  FO_TYPE_TRUETYPE  TRUETYPE_FONTTYPE
#define  FO_SIM_BOLD       0x00002000
#define  FO_SIM_ITALIC     0x00004000
#define  FO_EM_HEIGHT      0x00008000
#define  FO_GRAY16         0x00010000
#define  FO_NOGRAY16       0x00020000
#define  FO_NOHINTS        0x00040000
#define  FO_NO_CHOICE      0x00080000

enum _FP_MODES
{
  FP_ALTERNATEMODE = 1,
  FP_WINDINGMODE
};

enum _GLYPH_MODE
{
  FO_HGLYPHS,
  FO_GLYPHBITS,
  FO_PATHOBJ
};
 
/*  EngAssocateSurface hook flags  */
#define  HOOK_BITBLT             0x00000001
#define  HOOK_STRETCHBLT         0x00000002
#define  HOOK_PLGBLT             0x00000004
#define  HOOK_TEXTOUT            0x00000008
#define  HOOK_PAINT              0x00000010
#define  HOOK_STROKEPATH         0x00000020
#define  HOOK_FILLPATH           0x00000040
#define  HOOK_STROKEANDFILLPATH  0x00000080
#define  HOOK_LINETO             0x00000100
#define  HOOK_COPYBITS           0x00000400
#define  HOOK_SYNCHRONIZE        0x00001000
#define  HOOK_SYNCHRONIZEACCESS  0x00004000

enum _DRV_HOOK_FUNCS
{
  INDEX_DrvEnablePDEV,
  INDEX_DrvCompletePDEV,
  INDEX_DrvDisablePDEV,
  INDEX_DrvEnableSurface,
  INDEX_DrvDisableSurface,
  INDEX_DrvAssertMode,
  INDEX_DrvResetPDEV = 7,
  INDEX_DrvCreateDeviceBitmap = 10,
  INDEX_DrvDeleteDeviceBitmap,
  INDEX_DrvRealizeBrush,
  INDEX_DrvDitherColor,
  INDEX_DrvStrokePath,
  INDEX_DrvFillPath,
  INDEX_DrvStrokeAndFillPath,
  INDEX_DrvPaint,
  INDEX_DrvBitBlt,
  INDEX_DrvCopyBits,
  INDEX_DrvStretchBlt,
  INDEX_DrvSetPalette = 22,
  INDEX_DrvTextOut,
  INDEX_DrvEscape,
  INDEX_DrvDrawEscape,
  INDEX_DrvQueryFont,
  INDEX_DrvQueryFontTree,
  INDEX_DrvQueryFontData,
  INDEX_DrvSetPointerShape,
  INDEX_DrvMovePointer,
  INDEX_DrvLineTo,
  INDEX_DrvSendPage,
  INDEX_DrvStartPage,
  INDEX_DrvEndDoc,
  INDEX_DrvStartDoc,
  INDEX_DrvGetGlyphMode = 37,
  INDEX_DrvSynchronize,
  INDEX_DrvSaveScreenBits = 40,
  INDEX_DrvGetModes,
  INDEX_DrvFree,
  INDEX_DrvDestroyFont,
  INDEX_DrvQueryFontCaps,
  INDEX_DrvLoadFontFile,
  INDEX_DrvUnloadFontFile,
  INDEX_DrvFontManagement,
  INDEX_DrvQueryTrueTypeTable,
  INDEX_DrvQueryTrueTypeOutline,
  INDEX_DrvGetTrueTypeFile,
  INDEX_DrvQueryFontFile,
  INDEX_DrvQueryAdvanceWidths = 53,
  INDEX_DrvSetPixelFormat,
  INDEX_DrvDescribePixelFormat,
  INDEX_DrvSwapBuffers,
  INDEX_DrvStartBanding,
  INDEX_DrvNextBand,
  INDEX_DrvGetDirectDrawInfo,
  INDEX_DrvEnableDirectDraw,
  INDEX_DrvDisableDirectDraw,
  INDEX_DrvQuerySpoolType,
  INDEX_LAST
};

/*  EngCreatePalette mode types  */
#define  PAL_INDEXED    0x00000001
#define  PAL_BITFIELDS  0x00000002
#define  PAL_RGB        0x00000004
#define  PAL_BGR        0x00000008

enum _QUERY_ADVANCE_WIDTH_TYPES
{
  QAW_GETWIDTHS = 1,
  QAW_GETEASYWIDTHS 
};

#define  QC_OUTLINES  0x00000001
#define  QC_1BIT      0x00000002
#define  QC_4BIT      0x00000004

enum _QFF_MODES
{
  QFF_DESCRIPTION = 1,
  QFF_NUMFACES
};

#define  RB_DITHERCOLOR  0x80000000

enum _SPS_RC
{
  SPS_ERROR,
  SPS_DECLINE,
  SPS_ACCEPT_NOEXCLUDE,
  SPS_ACCEPT_EXCLUDE
};

#define SPS_CHANGE        0x00000001L
#define SPS_ASYNCCHANGE   0x00000002L
#define SPS_ANIMATESTART  0x00000004L
#define SPS_ANIMATEUPDATE 0x00000008L

#define  SS_SAVE     0
#define  SS_RESTORE  1
#define  SS_FREE     2

enum _SURF_TYPES
{
  STYPE_BITMAP = 1,
  STYPE_DEVICE, 
  STYPE_DEVBITMAP
};

#define  WO_RGN_CLIENT_DELTA   0x00000001
#define  WO_RGN_CLIENT         0x00000002
#define  WO_RGN_SURFACE_DELTA  0x00000004
#define  WO_RGN_SURFACE        0x00000008
#define  WO_RGN_UPDATE_ALL     0x00000010

#define  WOC_RGN_CLIENT_DELTA   0x00000001
#define  WOC_RGN_CLIENT         0x00000002
#define  WOC_RGN_SURFACE_DELTA  0x00000004
#define  WOC_RGN_SURFACE        0x00000008
#define  WOC_CHANGED            0x00000010
#define  WOC_DELETE             0x00000020

typedef HANDLE  HDEV;
typedef HANDLE  HGLYPH;
typedef HANDLE  HSURF;
typedef HANDLE  DHPDEV;
typedef HANDLE  DHSURF;
typedef ULONG  (*PFN)(VOID);

typedef struct _DRVFN
{
  ULONG  iFunc;
  PFN  pfn;
} DRVFN, *PDRVFN;

/*
 * DRVENABLEDATA - this structure is passed to the DDI from the GDI
 *   in the function DrvEnableDriver to determine driver parameters.
 */

typedef struct _DRVENABLEDATA
{
  ULONG  iDriverVersion;
  ULONG  c;
  DRVFN  *pdrvfn;
} DRVENABLEDATA, *PDRVENABLEDATA;

/* FIXME: replace this with correct def for LDECI4  */
typedef DWORD  LDECI4;

typedef struct _CIECHROMA 
{
  LDECI4  x;
  LDECI4  y;
  LDECI4  Y;
} CIECHROMA, *PCIECHROMA;

typedef struct _COLORINFO 
{
  CIECHROMA  Red;
  CIECHROMA  Green;
  CIECHROMA  Blue;
  CIECHROMA  Cyan;
  CIECHROMA  Magenta;
  CIECHROMA  Yellow;
  CIECHROMA  AlignmentWhite;
  LDECI4  RedGamma;
  LDECI4  GreenGamma;
  LDECI4  BlueGamma;
  LDECI4  MagentaInCyanDye;
  LDECI4  YellowInCyanDye;
  LDECI4  CyanInMagentaDye;
  LDECI4  YellowInMagentaDye;
  LDECI4  CyanInYellowDye;
  LDECI4  MagentaInYellowDye;
} COLORINFO, *PCOLORINFO;

typedef struct _DEVINFO 
{
  ULONG  flGraphicsCaps;
  LOGFONT  lfDefaultFont;
  LOGFONT  lfAnsiVarFont;
  LOGFONT  lfAnsiFixFont;
  ULONG  cFonts;
  ULONG  iDitherFormat;
  USHORT  cxDither;
  USHORT  cyDither;
  HPALETTE  hpalDefault;
} DEVINFO, *PDEVINFO;

typedef struct _GDIINFO 
{
  ULONG  ulVersion;
  ULONG  ulTechnology;
  ULONG  ulHorzSize;
  ULONG  ulVertSize;
  ULONG  ulHorzRes;
  ULONG  ulVertRes;
  ULONG  cBitsPixel;
  ULONG  cPlanes;
  ULONG  ulNumColors;
  ULONG  flRaster;
  ULONG  ulLogPixelsX;
  ULONG  ulLogPixelsY;
  ULONG  flTextCaps;
  ULONG  ulDACRed;
  ULONG  ulDACGreen;
  ULONG  ulDACBlue;
  ULONG  ulAspectX;
  ULONG  ulAspectY;
  ULONG  ulAspectXY;
  LONG  xStyleStep;
  LONG  yStyleStep;
  LONG  denStyleStep;
  POINTL  ptlPhysOffset;
  SIZEL  szlPhysSize;
  ULONG  ulNumPalReg;
  COLORINFO  ciDevice;
  ULONG  ulDevicePelsDPI;
  ULONG  ulPrimaryOrder;
  ULONG  ulHTPatternSize;
  ULONG  ulHTOutputFormat;
  ULONG  flHTFlags;
  ULONG  ulVRefresh;
  ULONG  ulBltAlignment;
  ULONG  ulPanningHorzRes;
  ULONG  ulPanningVertRes;
} GDIINFO, *PGDIINFO;

typedef struct _DEVMODEW 
{
  WCHAR  dmDeviceName[DMMAXDEVICENAME];
  WORD  dmSpecVersion;
  WORD  dmDriverVersion;
  WORD  dmSize;
  WORD  dmDriverExtra;
  DWORD  dmFields;
  short  dmOrientation;
  short  dmPaperSize;
  short  dmPaperLength;
  short  dmPaperWidth;
  short  dmScale;
  short  dmCopies;
  short  dmDefaultSource;
  short  dmPrintQuality;
  short  dmColor;
  short  dmDuplex;
  short  dmYResolution;
  short  dmTTOption;
  short  dmCollate;
  WCHAR  dmFormName[DMMAXFORMNAME];
  WORD  dmLogPixels;
  DWORD  dmBitsPerPel;
  DWORD  dmPelsWidth;
  DWORD  dmPelsHeight;
  DWORD  dmDisplayFlags;
  DWORD  dmDisplayFrequency;
} DEVMODEW, *PDEVMODEW;

typedef struct _BRUSHOBJ 
{
  ULONG  iSolidColor;
  PVOID  pvRbrush;

  /*  remainder of fields are for GDI internal use  */
  LOGBRUSH  logbrush;
} BRUSHOBJ, *PBRUSHOBJ;

typedef struct _CLIPOBJ 
{
  ULONG  iUniq;
  RECTL  rclBounds;
  BYTE  iDComplexity;
  BYTE  iFComplexity;
  BYTE  iMode;
  BYTE  fjOptions;
} CLIPOBJ, *PCLIPOBJ;

typedef struct _ENUMRECTS 
{
  ULONG  c;
  RECTL  arcl[1];
} ENUMRECTS, *PENUMRECTS;

typedef struct _FONTOBJ 
{
  ULONG  iUniq;  
  ULONG  iFace; 
  ULONG  cxMax;
  ULONG  flFontType;
  ULONG  iTTUniq;
  ULONG  iFile;  
  SIZE  sizLogResPpi;
  ULONG  ulStyleSize;
  PVOID  pvConsumer;
  PVOID  pvProducer;
} FONTOBJ, *PFONTOBJ;

typedef struct _IFIMETRICS 
{
  ULONG cjThis;
  ULONG ulVersion;
  PTRDIFF dpwszFamilyName;
  PTRDIFF dpwszStyleName;
  PTRDIFF dpwszFaceName;
  PTRDIFF dpwszUniqueName;
  PTRDIFF dpFontSim;
  LONG lEmbedId;
  LONG lItalicAngle;
  LONG lCharBias;
  PTRDIFF dpCharSets;
  BYTE jWinCharSet;
  BYTE jWinPitchAndFamily;
  USHORT usWinWeight;
  ULONG flInfo;
  USHORT fsSelection;
  USHORT fsType;
  WORD fwdUnitsPerEm;
  WORD fwdLowestPPEm;
  WORD fwdWinAscender;
  WORD fwdWinDescender;
  WORD fwdMacAscender;
  WORD fwdMacDescender;
  WORD fwdMacLineGap;
  WORD fwdTypoAscender;
  WORD fwdTypoDescender;
  WORD fwdTypoLineGap;
  WORD fwdAveCharWidth;
  WORD fwdMaxCharInc;
  WORD fwdCapHeight;
  WORD fwdXHeight;
  WORD fwdSubScriptXSize;
  WORD fwdSubScriptYSize;
  WORD fwdSubScriptXOffset;
  WORD fwdSubScriptYOffset;
  WORD fwdSuperScriptXSize;
  WORD fwdSuperScriptYSize;
  WORD fwdSuperScriptXOffset;
  WORD fwdSuperScriptYOffset;
  WORD fwdUnderscoreSize;
  WORD fwdUnderscorePosition;
  WORD fwdStrikeoutSize;
  WORD fwdStrikeoutPosition;
  BYTE chFirstChar;
  BYTE chLastChar;
  BYTE chDefaultChar;
  BYTE chBreakChar;
  WCHAR wcFirstChar;
  WCHAR wcLastChar;
  WCHAR wcDefaultChar;
  WCHAR wcBreakChar;
  POINTL ptlBaseline;
  POINTL ptlAspect;
  POINTL ptlCaret;
  RECTL rclFontBox;
  BYTE achVendId[4];
  ULONG cKerningPairs;
  ULONG ulPanoseCulture;
  PANOSE panose;
} IFIMETRICS, *PIFIMETRICS;

typedef struct _PALOBJ
{
  ULONG   ulReserved;
} PALOBJ, *PPALOBJ;

typedef struct _PATHOBJ
{
  ULONG  fl;
  ULONG  cCurves;
} PATHOBJ, *PPATHOBJ;

typedef struct _SURFOBJ 
{
  DHSURF  dhsurf;
  HSURF  hsurf;
  DHPDEV  dhpdev;
  HDEV  hdev;
  SIZEL  sizlBitmap;
  ULONG  cjBits;
  PVOID  pvBits;
  PVOID  pvScan0;
  LONG  lDelta;
  ULONG  iUniq;
  ULONG  iBitmapFormat;
  USHORT  iType;
  USHORT  fjBitmap;
} SURFOBJ, *PSURFOBJ;

typedef struct _WNDOBJ
{
  CLIPOBJ  coClient;
  PVOID  pvConsumer;
  RECTL  rclClient;
} WNDOBJ, *PWNDOBJ;

typedef VOID (CALLBACK * WNDOBJCHANGEPROC)(PWNDOBJ WndObj, ULONG Flags);

typedef struct _XFORMOBJ
{
  /* FIXME: what does this beast look like?  */
} XFORMOBJ, *PXFORMOBJ;

typedef struct _XLATEOBJ 
{
  ULONG  iUniq;
  ULONG  flXlate;
  USHORT  iSrcType;
  USHORT  iDstType;
  ULONG  cEntries;
  ULONG  *pulXlate;
} XLATEOBJ, *PXLATEOBJ;

/*
 * Functions Prefixed with Drv are calls made from GDI to DDI, and
 * everything else are calls made from DDI to GDI.  DDI is
 * not allowed to make calls to any other kernel or user modules.
 */

/*  GDI --> DDI calls  */
VOID DrvAssertMode(IN DHPDEV  PDev, 
                   IN BOOL  ShouldEnable);
BOOL DrvBitBlt(IN PSURFOBJ  DestSurface, 
               IN PSURFOBJ  SrcSurface, 
               IN PSURFOBJ  MaskSurface, 
               IN PCLIPOBJ  ClipObj, 
               IN PXLATEOBJ  XLateObj, 
               IN PRECTL  DestRectL, 
               IN PPOINTL  SrcPointL, 
               IN PPOINTL  MaskPointL, 
               IN PBRUSHOBJ  BrushObj, 
               IN PPOINTL  BrushPointL, 
               IN ROP4  RasterOp); 
VOID DrvCompletePDEV(IN DHPDEV  PDev,
                     IN HDEV  Dev);
BOOL DrvCopyBits(OUT PSURFOBJ  DestSurface, 
                 IN PSURFOBJ  SrcSurface, 
                 IN PCLIPOBJ  ClipObj, 
                 IN PXLATEOBJ  XLateObj, 
                 IN PRECTL  DestRectL, 
                 IN PPOINTL  SrcPointL); 
HBITMAP DrvCreateDeviceBitmap(IN DHPDEV  DPev, 
                              IN SIZEL  SizeL, 
                              IN ULONG  Format); 
VOID DrvDeleteDeviceBitmap(IN DHSURF  Surface); 
LONG DrvDescribePixelFormat(IN DHPDEV  DPev, 
                            IN LONG  PixelFormat, 
                            IN ULONG  DescriptorSize, 
                            OUT PPIXELFORMATDESCRIPTOR  PFD); 
VOID DrvDestroyFont(IN PFONTOBJ FontObj); 
VOID DrvDisableDirectDraw(IN DHPDEV  PDev);
VOID DrvDisableDriver(VOID);
VOID DrvDisablePDEV(IN DHPDEV PDev); 
VOID DrvDisableSurface(IN DHPDEV PDev); 
ULONG DrvDitherColor(IN DHPDEV  DPev, 
                     IN ULONG  Mode, 
                     IN ULONG  RGB, 
                     OUT PULONG  DitherBits); 
ULONG DrvDrawEscape(IN PSURFOBJ  SurfObj, 
                    IN ULONG  EscCode, 
                    IN PCLIPOBJ  ClipObj, 
                    IN PRECTL  RectL, 
                    IN ULONG  InputSize, 
                    IN PVOID  *InputData); 
BOOL DrvEnableDirectDraw(IN DHPDEV  PDev,
                         IN PDD_CALLBACKS Callbacks,
                         IN PDD_SURFACECALLBACKS  SurfaceCallbacks,
                         IN PDD_PALETTECALLBACKS  PaletteCallbacks);
BOOL DrvEnableDriver(IN ULONG Version, 
                     IN ULONG DEDSize, 
                     OUT PDRVENABLEDATA DED);
DHPDEV DrvEnablePDEV(IN DEVMODEW  *DM,
                     IN LPWSTR  LogAddress,
                     IN ULONG  PatternCount,
                     OUT HSURF  *SurfPatterns,
                     IN ULONG  CapsSize,
                     OUT ULONG  *DevCaps,
                     IN ULONG  DevInfoSize,
                     OUT DEVINFO  *DI,
                     IN LPWSTR  DevDataFile,
                     IN LPWSTR  DeviceName,
                     IN HANDLE  Driver);
HSURF DrvEnableSurface(IN DHPDEV  PDev);
BOOL DrvEndDoc(IN PSURFOBJ  SurfObj, 
               IN ULONG  Flags); 
ULONG DrvEscape(IN PSURFOBJ  SurfObj, 
                IN ULONG  EscCode, 
                IN ULONG  InputSize, 
                IN PVOID  *InputData, 
                IN ULONG  OutputSize, 
                OUT PVOID  *OutputData); 
BOOL DrvFillPath(IN PSURFOBJ  SurfObj, 
                 IN PPATHOBJ  PathObj, 
                 IN PCLIPOBJ  ClipObj, 
                 IN PBRUSHOBJ  BrushObj, 
                 IN PPOINTL  BrushOrg, 
                 IN MIX  Mix, 
                 IN ULONG  Options); 
ULONG DrvFontManagement(IN PSURFOBJ  SurfObj, 
                        IN PFONTOBJ  FontObj, 
                        IN ULONG  Mode, 
                        IN ULONG  InputSize, 
                        IN PVOID  InputData, 
                        IN ULONG  OutputSize, 
                        OUT PVOID  OutputData); 
VOID DrvFree(IN PVOID  Obj, 
             IN ULONG  ID); 
BOOL DrvGetDirectDrawInfo(IN DHPDEV  PDev,
                          IN PDD_HALINFO  HalInfo,
                          IN PDWORD  NumHeaps,
                          IN PVIDEOMEMORY  List,
                          IN PDWORD  NumFourCCCodes,
                          IN PDWORD  FourCC);
ULONG DrvGetGlyphMode(IN DHPDEV  DPev,
                      IN PFONTOBJ  FontObj); 
ULONG DrvGetModes(IN HANDLE Driver,
                  IN ULONG DataSize,
                  OUT PDEVMODEW DM);
PVOID DrvGetTrueTypeFile(IN ULONG  FileNumber, 
                         IN PULONG  Size); 
BOOL DrvLineTo(IN PSURFOBJ SurfObj, 
               IN PCLIPOBJ ClipObj, 
               IN PBRUSHOBJ  BrushObj, 
               IN LONG  x1, 
               IN LONG  y1, 
               IN LONG  x2, 
               IN LONG  y2, 
               IN PRECTL  Bounds, 
               IN MIX  Mix); 
ULONG DrvLoadFontFile(IN ULONG  FileNumber, 
                      IN PVOID  ViewData, 
                      IN ULONG  ViewSize, 
                      IN ULONG  LangID); 
VOID DrvMovePointer(IN PSURFOBJ  SurfObj, 
                    IN LONG  x, 
                    IN LONG  y, 
                    IN PRECTL  RectL); 
BOOL DrvNextBand(IN PSURFOBJ  SurfObj, 
                 OUT PPOINTL  PointL); 
BOOL DrvPaint(IN PSURFOBJ  SurfObj, 
              IN PCLIPOBJ  ClipObj, 
              IN PBRUSHOBJ  BrushObj, 
              IN PPOINTL  BrushOrg, 
              IN MIX  Mix); 
BOOL DrvQueryAdvanceWidths(IN DHPDEV  DPev, 
                           IN PFONTOBJ  FontObj, 
                           IN ULONG  Mode, 
                           IN HGLYPH  Glyph, 
                           OUT PVOID  *Widths, 
                           IN ULONG  NumGlyphs); 
PIFIMETRICS DrvQueryFont(IN DHPDEV  PDev, 
                         IN ULONG  FileNumber, 
                         IN ULONG  FaceIndex, 
                         IN PULONG  Identifier); 
LONG DrvQueryFontCaps(IN ULONG  CapsSize, 
                      OUT PULONG  CapsData); 
LONG DrvQueryFontData(IN DHPDEV  DPev, 
                      IN PFONTOBJ  FontObj, 
                      IN ULONG  Mode, 
                      IN HGLYPH  Glyph, 
                      IN PGLYPHDATA  GlyphData, 
                      IN PVOID  DataBuffer, 
                      IN ULONG  BufferSize); 
LONG DrvQueryFontFile(IN ULONG  FileNumber,
                      IN ULONG  Mode,
                      IN ULONG  BufSize,
                      OUT PULONG  Buf);
PVOID DrvQueryFontTree(IN DHPDEV  PDev,
                       IN ULONG  FileNumber,
                       IN ULONG  FaceIndex,
                       IN ULONG  Mode,
                       OUT ULONG  *ID);
BOOL DrvQuerySpoolType(DHPDEV PDev, 
                       LPWSTR SpoolType);
LONG DrvQueryTrueTypeOutline(IN DHPDEV  PDev,
                             IN PFONTOBJ  FontObj,
                             IN HGLYPH  Glyph,
                             IN BOOL  MetricsOnly,
                             IN PGLYPHDATA  GlyphData,
                             IN ULONG  BufSize,
                             OUT PTTPOLYGONHEADER Polygons);
LONG DrvQueryTrueTypeTable(IN ULONG  FileNumber,
                           IN ULONG  Font,
                           IN ULONG  Tag,
                           IN PTRDIFF  Start,
                           IN ULONG  BufSize,
                           OUT BYTE  *Buf);
BOOL DrvRealizeBrush(IN PBRUSHOBJ  BrushObj,
                     IN PSURFOBJ  TargetSurface,
                     IN PSURFOBJ  PatternSurface,
                     IN PSURFOBJ  MaskSurface,
                     IN PXLATEOBJ  XLateObj,
                     IN ULONG  iHatch);
BOOL DrvResetPDEV(IN DHPDEV  PDevOld, 
                  IN DHPDEV  PDevNew);
ULONG DrvSaveScreenBits(IN PSURFOBJ SurfObj,
                        IN ULONG Mode,
                        IN ULONG ID,
                        IN PRECTL RectL);
BOOL DrvSendPage(IN PSURFOBJ SurfObj);
BOOL DrvSetPalette(IN DHPDEV  PDev,
                   IN PPALOBJ  PaletteObj,
                   IN ULONG  Flags,
                   IN ULONG  Start,
                   IN ULONG  NumColors);
ULONG DrvSetPointerShape(IN PSURFOBJ  SurfObj,
                         IN PSURFOBJ  MaskSurface,
                         IN PSURFOBJ  ColorSurface,
                         IN PXLATEOBJ  XLateObj,
                         IN LONG  xHot,
                         IN LONG  yHot,
                         IN LONG  x,
                         IN LONG  y,
                         IN PRECTL  RectL,
                         IN ULONG  Flags);
BOOL DrvStartBanding(IN PSURFOBJ  SurfObj, 
                     IN PPOINTL  PointL);
BOOL DrvStartDoc(IN PSURFOBJ  SurfObj,
                 IN LPWSTR  DocName,
                 IN DWORD  JobID);
BOOL DrvStartPage(IN PSURFOBJ  SurfObj);
BOOL DrvStretchBlt(IN PSURFOBJ  DestSurface,
                   IN PSURFOBJ  SrcSurface,
                   IN PSURFOBJ  MaskSurface,
                   IN PCLIPOBJ  ClipObj,
                   IN PXLATEOBJ  XLateObj,
                   IN PCOLORADJUSTMENT  CA,
                   IN PPOINTL  HTOrg,
                   IN PRECTL  Dest,
                   IN PRECTL  Src,
                   IN PPOINTL  Mask,
                   IN ULONG  Mode);
BOOL DrvStrokeAndFillPath(IN PSURFOBJ  SurfObj,
                          IN PPATHOBJ  PathObj,
                          IN PCLIPOBJ  ClipObj,
                          IN PXFORMOBJ  XFormObj,
                          IN PBRUSHOBJ  StrokeBrush,
                          IN PLINEATTRS  LineAttrs,
                          IN PBRUSHOBJ  FillBrush,
                          IN PPOINTL  BrushOrg,
                          IN MIX  MixFill,
                          IN ULONG  Options);
BOOL DrvStrokePath(IN PSURFOBJ  SurfObj,
                   IN PPATHOBJ  PathObj,
                   IN PCLIPOBJ  PClipObj,
                   IN PXFORMOBJ  XFormObj,
                   IN PBRUSHOBJ  BrushObj,
                   IN PPOINTL  BrushOrg,
                   IN PLINEATTRS  LineAttrs,
                   IN MIX  Mix);
VOID DrvSynchronize(IN DHPDEV PDev,
                    IN PRECTL RectL);
BOOL DrvTextOut(IN PSURFOBJ  SurfObj,
                IN PSTROBJ  StrObj,
                IN PFONTOBJ  FontObj,
                IN PCLIPOBJ  ClipObj,
                IN PRECTL    ExtraRect,
                IN PRECTL    OpaqueRect,
                IN PBRUSHOBJ  ForegroundBrush,
                IN PBRUSHOBJ  OpaqueBrush,
                IN PPOINTL  OrgPoint,
                IN MIX  Mix);
BOOL DrvUnloadFontFile(IN ULONG  FileNumber);

/*  DDI --> GDI calls  */
PVOID BRUSHOBJ_pvAllocRbrush(IN PBRUSHOBJ  BrushObj, 
                             IN ULONG  ObjSize); 
PVOID BRUSHOBJ_pvGetRbrush(IN PBRUSHOBJ  BrushObj); 
BOOL CLIPOBJ_bEnum(IN PCLIPOBJ  ClipObj, 
                   IN ULONG  ObjSize, 
                   OUT ULONG  *EnumRects); 
ULONG CLIPOBJ_cEnumStart(IN PCLIPOBJ  ClipObj, 
                         IN BOOL  ShouldDoAll, 
                         IN ULONG  ClipType, 
                         IN ULONG  BuildOrder, 
                         IN ULONG  MaxRects); 
PPATHOBJ CLIPOBJ_ppoGetPath(PCLIPOBJ ClipObj);

/*
EngAcquireSemaphore
*/

/* FIXME: find correct defines for following symbols  */
#define  ALLOC_TAG  1
#define  FL_ZERO_MEMORY  1

PVOID  APIENTRY  EngAllocMem(ULONG  Flags,
                             ULONG  MemSize,
                             ULONG  Tag);

/*
EngAllocUserMem
*/

BOOL EngAssociateSurface(IN HSURF  Surface,
                         IN HDEV  Dev,
                         IN ULONG  Hooks);

/*
EngBitBlt
EngCheckAbort
EngComputeGlyphSet
EngCopyBits
*/

HBITMAP EngCreateBitmap(IN SIZEL  Size,
                        IN LONG  Width,
                        IN ULONG  Format,
                        IN ULONG  Flags,
                        IN PVOID  Bits);

/*
EngCreateClip
EngCreateDeviceBitmap
*/

HSURF EngCreateDeviceSurface(IN DHSURF  Surface,
                             IN SIZEL  Size,
                             IN ULONG  FormatVersion);

/*
EngCreateDriverObj
EngCreateEvent
*/

HPALETTE EngCreatePalette(IN ULONG  Mode,
                          IN ULONG  NumColors,
                          IN PULONG  *Colors, 
                          IN ULONG  Red, 
                          IN ULONG  Green, 
                          IN ULONG  Blue); 
/*
EngCreatePath
EngCreateSemaphore
EngCreateWnd
EngDebugBreak = NTOSKRNL.DbgBreakPoint
*/

VOID  APIENTRY  EngDebugPrint(PCHAR  StandardPrefix,
                              PCHAR  DebugMessage,
                              va_list  ArgList);

/*
EngDeleteClip
EngDeleteDriverObj
EngDeleteEvent
EngDeletePalette
EngDeletePath
EngDeleteSemaphore
EngDeleteSurface
EngDeleteWnd
EngDeviceIoControl
EngEnumForms
EngEraseSurface
EngFillPath
EngFindImageProcAddress
EngFindResource
EngFreeMem
EngFreeModule
EngFreeUserMem
EngGetCurrentCodePage
EngGetDriverName
EngGetFileChangeTime
EngGetFilePath
EngGetForm
EngGetLastError
EngGetPrinter
EngGetPrinterData
EngGetPrinterDataFileName
EngGetProcessHandle
EngGetType1FontList
EngLineTo
EngLoadImage
EngLoadModule
EngLoadModuleForWrite
EngLockDriverObj
EngLockSurface
EngMapEvent
EngMapFontFile
EngMapModule
EngMarkBandingSurface
EngMovePointer
EngMulDiv
EngMultiByteToUnicodeN
EngMultiByteToWideChar
EngPaint
EngProbeForRead
EngProbeForReadAndWrite = NTOSKRNL.ProbeForWrite
EngQueryLocalTime
EngQueryPalette
EngQueryPerformanceCounter
EngQueryPerformanceFrequency
EngReleaseSemaphore
EngRestoreFloatingPointState
EngSaveFloatingPointState
EngSecureMem
EngSetEvent
EngSetLastError
EngSetPointerShape
EngSetPointerTag
EngSetPrinterData
EngSort
EngStretchBlt
EngStrokeAndFillPath
EngStrokePath
EngTextOut
EngUnicodeToMultiByteN
EngUnloadImage
EngUnlockDriverObj
EngUnlockSurface
EngUnmapEvent
EngUnmapFontFile
EngUnsecureMem = NTOSKRNL.MmUnsecureVirtualMemory
EngWaitForSingleObject
EngWideCharToMultiByte
EngWritePrinter
FLOATOBJ_Add
FLOATOBJ_AddFloat
FLOATOBJ_AddFloatObj
FLOATOBJ_AddLong
FLOATOBJ_Div
FLOATOBJ_DivFloat
FLOATOBJ_DivFloatObj
FLOATOBJ_DivLong
FLOATOBJ_Equal
FLOATOBJ_EqualLong
FLOATOBJ_GetFloat
FLOATOBJ_GetLong
FLOATOBJ_GreaterThan
FLOATOBJ_GreaterThanLong
FLOATOBJ_LessThan
FLOATOBJ_LessThanLong
FLOATOBJ_Mul
FLOATOBJ_MulFloat
FLOATOBJ_MulFloatObj
FLOATOBJ_MulLong
FLOATOBJ_Neg
FLOATOBJ_SetFloat
FLOATOBJ_SetLong
FLOATOBJ_Sub
FLOATOBJ_SubFloat
FLOATOBJ_SubFloatObj
FLOATOBJ_SubLong
*/

ULONG FONTOBJ_cGetAllGlyphHandles(IN PFONTOBJ  FontObj,
                                  IN HGLYPH  *Glyphs);
ULONG FONTOBJ_cGetGlyphs(IN PFONTOBJ  FontObj,
                         IN ULONG  Mode,
                         IN ULONG  NumGlyphs,
                         IN HGLYPH  *GlyphHandles,
                         IN PVOID  *OutGlyphs);
PGAMMA_TABLES FONTOBJ_pGetGammaTables(IN PFONTOBJ  FontObj);
IFIMETRICS *FONTOBJ_pifi(IN PFONTOBJ  FontObj);
PVOID  FONTOBJ_pvTrueTypeFontFile(IN PFONTOBJ  FontObj,
                                  IN ULONG  *FileSize);
XFORMOBJ *FONTOBJ_pxoGetXform(IN PFONTOBJ  FontObj);
VOID  FONTOBJ_vGetInfo(IN PFONTOBJ  FontObj,
                       IN ULONG  InfoSize,
                       OUT PFONTINFO  FontInfo);

/*
HT_ComputeRGBGammaTable
HT_Get8BPPFormatPalette
PALOBJ_cGetColors
PATHOBJ_bCloseFigure
PATHOBJ_bEnum
PATHOBJ_bEnumClipLines
PATHOBJ_bMoveTo
PATHOBJ_bPolyBezierTo
PATHOBJ_bPolyLineTo
PATHOBJ_vEnumStart
PATHOBJ_vEnumStartClipLines
PATHOBJ_vGetBounds
RtlAnsiCharToUnicodeChar = NTOSKRNL.RtlAnsiCharToUnicodeChar
RtlMultiByteToUnicodeN = NTOSKRNL.RtlMultiByteToUnicodeN
RtlRaiseException = NTOSKRNL.RtlRaiseException
RtlUnicodeToMultiByteN = NTOSKRNL.RtlUnicodeToMultiByteN
RtlUnicodeToMultiByteSize = NTOSKRNL.RtlUnicodeToMultiByteSize
RtlUnwind = NTOSKRNL.RtlUnwind
RtlUpcaseUnicodeChar = NTOSKRNL.RtlUpcaseUnicodeChar
RtlUpcaseUnicodeToMultiByteN = NTOSKRNL.RtlUpcaseUnicodeToMultiByteN
STROBJ_bEnum
STROBJ_dwGetCodePage
STROBJ_vEnumStart
WNDOBJ_bEnum
WNDOBJ_cEnumStart
WNDOBJ_vSetConsumer
XFORMOBJ_bApplyXform
XFORMOBJ_iGetFloatObjXform
XFORMOBJ_iGetXform
XLATEOBJ_cGetPalette
XLATEOBJ_iXlate
XLATEOBJ_piVector
*/

#endif

