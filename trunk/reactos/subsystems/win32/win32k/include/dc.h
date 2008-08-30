#ifndef __WIN32K_DC_H
#define __WIN32K_DC_H

#include "brush.h"
#include "bitmaps.h"

/* Constants ******************************************************************/

// Get/SetBounds/Rect support.
#define DCB_WINDOWMGR 0x8000 // Queries the Windows bounding rectangle instead of the application's

// GDIDEVICE flags
#define PDEV_DISPLAY             0x00000001 // Display device
#define PDEV_HARDWARE_POINTER    0x00000002 // Supports hardware cursor
#define PDEV_SOFTWARE_POINTER    0x00000004
#define PDEV_GOTFONTS            0x00000040 // Has font driver
#define PDEV_PRINTER             0x00000080
#define PDEV_ALLOCATEDBRUSHES    0x00000100
#define PDEV_HTPAL_IS_DEVPAL     0x00000200
#define PDEV_DISABLED            0x00000400
#define PDEV_SYNCHRONIZE_ENABLED 0x00000800
#define PDEV_FONTDRIVER          0x00002000 // Font device
#define PDEV_GAMMARAMP_TABLE     0x00004000
#define PDEV_UMPD                0x00008000
#define PDEV_SHARED_DEVLOCK      0x00010000
#define PDEV_META_DEVICE         0x00020000
#define PDEV_DRIVER_PUNTED_CALL  0x00040000 // Driver calls back to GDI engine
#define PDEV_CLONE_DEVICE        0x00080000

/* Type definitions ***********************************************************/

typedef struct _WIN_DC_INFO
{
  HRGN     hClipRgn;     /* Clip region (may be 0) */
  HRGN     hVisRgn;      /* Should me to DC. Visible region (must never be 0) */
  HRGN     hGCClipRgn;   /* GC clip region (ClipRgn AND VisRgn) */
  HBITMAP  hBitmap;

  BYTE   bitsPerPixel;
} WIN_DC_INFO;

// EXtended CLip and Window Region Object
typedef struct _XCLIPOBJ
{
  WNDOBJ  eClipWnd;
  PVOID   pClipRgn;    // prgnRao_ or (prgnVis_ if (prgnRao_ == z))
  DWORD   Unknown1[16];
  DWORD   nComplexity; // count/mode based on # of rect in regions scan.
  PVOID   pUnknown;    // UnK pointer to a large drawing structure.
                       // We will use it for CombinedClip ptr.
} XCLIPOBJ, *PXCLIPOBJ;

typedef struct _DCLEVEL
{
  HPALETTE          hpal;
  struct _PALGDI  * ppal;
  PVOID             pColorSpace; // COLORSPACE*
  LONG              lIcmMode;
  LONG              lSaveDepth;
  DWORD             unk1_00000000;
  HGDIOBJ           hdcSave;
  POINTL            ptlBrushOrigin;
  PGDIBRUSHOBJ      pbrFill;
  PGDIBRUSHOBJ      pbrLine;
  PVOID             plfnt; // LFONTOBJ* (TEXTOBJ*)
  HGDIOBJ           hPath; // HPATH
  FLONG             flPath;
  LINEATTRS         laPath; // 0x20 bytes
  PVOID             prgnClip; // PROSRGNDATA
  PVOID             prgnMeta;
  COLORADJUSTMENT   ca;
  FLONG             flFontState;
  UNIVERSAL_FONT_ID ufi;
  UNIVERSAL_FONT_ID ufiLoc[4]; // Local List.
  UNIVERSAL_FONT_ID *pUFI;
  ULONG             uNumUFIs;
  BOOL              ufiSet;
  FLONG             fl;
  FLONG             flBrush;
  MATRIX            mxWorldToDevice;
  MATRIX            mxDeviceToWorld;
  MATRIX            mxWorldToPage;
  FLOATOBJ          efM11PtoD;
  FLOATOBJ          efM22PtoD;
  FLOATOBJ          efDxPtoD;
  FLOATOBJ          efDyPtoD;
  FLOATOBJ          efM11_TWIPS;
  FLOATOBJ          efM22_TWIPS;
  FLOATOBJ          efPr11;
  FLOATOBJ          efPr22;
  PBITMAPOBJ        pSurface; // SURFACE*
  SIZE              sizl;
} DCLEVEL, *PDCLEVEL;

/* The DC object structure */
typedef struct _DC
{
  /* Header for all gdi objects in the handle table.
     Do not (re)move this. */
  BASEOBJECT  BaseObject;

  DHPDEV      PDev;   // <- GDIDEVICE.hPDev DHPDEV for device.
  INT         DC_Type;
  INT         DC_Flags;
  PVOID       pPDev;  // PGDIDEVICE aka PDEVOBJ
  PVOID       hSem;   // PERESOURCE aka HSEMAPHORE
  FLONG       flGraphics;
  FLONG       flGraphics2;
  PDC_ATTR    pDc_Attr;
  DCLEVEL     DcLevel;
  DC_ATTR     Dc_Attr;
  HDC         hNext;
  HDC         hPrev;
  RECTL       erclClip;
  POINTL      ptlDCOrig;
  RECTL       erclWindow;
  RECTL       erclBounds;
  RECTL       erclBoundsApp;
  PVOID       prgnAPI; // PROSRGNDATA
  PVOID       prgnVis;
  PVOID       prgnRao;
  POINTL      ptlFillOrigin;
  unsigned    eboFill_[23]; // EBRUSHOBJ
  unsigned    eboLine_[23];
  unsigned    eboText_[23];
  unsigned    eboBackground_[23];
  HFONT       hlfntCur;
  FLONG       flSimulationFlags;
  LONG        lEscapement;
  PVOID       prfnt;    // RFONT*
  XCLIPOBJ    co;       // CLIPOBJ
  PVOID       pPFFList; // PPFF*
  PVOID       ClrxFormLnk;
  INT         ipfdDevMax;
  ULONG       ulCopyCount;
  PVOID       pSurfInfo;
  POINTL      ptlDoBanding;

  /* Reactos specific members */
  WIN_DC_INFO w;
  CLIPOBJ     *CombinedClip;
  XLATEOBJ    *XlateBrush;
  XLATEOBJ    *XlatePen;

  UNICODE_STRING    DriverName;
} DC, *PDC;

typedef struct _GRAPHICS_DEVICE
{
  CHAR szNtDeviceName[CCHDEVICENAME];           // Yes char AscII
  CHAR szWinDeviceName[CCHDEVICENAME];          // <- chk GetMonitorInfoW MxIxEX.szDevice
  struct _GRAPHICS_DEVICE * pNextGraphicsDevice;
  DWORD StateFlags;                             // See DISPLAY_DEVICE_*
} GRAPHICS_DEVICE, *PGRAPHICS_DEVICE;

typedef struct _GDIPOINTER /* should stay private to ENG? No, part of GDIDEVICE aka HDEV aka PDEV. */
{
  /* private GDI pointer handling information, required for software emulation */
  BOOL     Enabled;
  POINTL   Pos;
  SIZEL    Size;
  POINTL   HotSpot;
  XLATEOBJ *XlateObject;
  HSURF    ColorSurface;
  HSURF    MaskSurface;
  HSURF    SaveSurface;
  int      ShowPointer; /* counter negtive  do not show the mouse postive show the mouse */

  /* public pointer information */
  RECTL    Exclude; /* required publicly for SPS_ACCEPT_EXCLUDE */
  PGD_MOVEPOINTER MovePointer;
  ULONG    Status;
} GDIPOINTER, *PGDIPOINTER;

typedef struct _GDIDEVICE
{
  BASEOBJECT  BaseObject;

  struct _GDIDEVICE *ppdevNext;
  INT           cPdevRefs;
  INT           cPdevOpenRefs;
  struct _GDIDEVICE *ppdevParent;
  FLONG         flFlags;
  PERESOURCE    hsemDevLock;    // Device lock.

  PVOID         pvGammaRamp;    // Gamma ramp pointer.

  DHPDEV        hPDev;          // DHPDEV for device.

  HSURF         FillPatterns[HS_DDI_MAX];

  ULONG         DxDd_nCount;

  DEVINFO       DevInfo;
  GDIINFO       GDIInfo;
  HSURF         pSurface;       // SURFACE for this device.
  HANDLE        hSpooler;       // Handle to spooler, if spooler dev driver.
  ULONG         DisplayNumber;
  PVOID         pGraphicsDev;   // PGRAPHICS_DEVICE

  DEVMODEW      DMW;
  PVOID         pdmwDev;        // Ptr->DEVMODEW.dmSize + dmDriverExtra == alloc size.

  FLONG         DxDd_Flags;     // DxDD active status flags.

  PFILE_OBJECT  VideoFileObject;
  BOOLEAN       PreparedDriver;
  GDIPOINTER    Pointer;
  /* Stuff to keep track of software cursors; win32k gdi part */
  UINT SafetyRemoveLevel; /* at what level was the cursor removed?
			     0 for not removed */
  UINT SafetyRemoveCount;

  DRIVER_FUNCTIONS DriverFunctions;
  struct _EDD_DIRECTDRAW_GLOBAL * pEDDgpl;
} GDIDEVICE, *PGDIDEVICE;

/* Internal functions *********************************************************/

#define  DC_LockDc(hDC)  \
  ((PDC) GDIOBJ_LockObj ((HGDIOBJ) hDC, GDI_OBJECT_TYPE_DC))
#define  DC_UnlockDc(pDC)  \
  GDIOBJ_UnlockObjByPtr ((POBJ)pDC)

extern PDC defaultDCstate;

NTSTATUS FASTCALL InitDcImpl(VOID);
PGDIDEVICE FASTCALL IntEnumHDev(VOID);
HDC  FASTCALL DC_AllocDC(PUNICODE_STRING  Driver);
VOID FASTCALL DC_InitDC(HDC  DCToInit);
HDC  FASTCALL DC_FindOpenDC(PUNICODE_STRING  Driver);
VOID FASTCALL DC_FreeDC(HDC);
VOID FASTCALL DC_AllocateDcAttr(HDC);
VOID FASTCALL DC_FreeDcAttr(HDC);
BOOL INTERNAL_CALL DC_Cleanup(PVOID ObjectBody);
HDC  FASTCALL DC_GetNextDC (PDC pDC);
VOID FASTCALL DC_SetNextDC (PDC pDC, HDC hNextDC);
BOOL FASTCALL DC_SetOwnership(HDC DC, PEPROCESS Owner);
VOID FASTCALL DC_LockDisplay(HDC);
VOID FASTCALL DC_UnlockDisplay(HDC);
VOID FASTCALL IntGdiCopyFromSaveState(PDC, PDC, HDC);
VOID FASTCALL IntGdiCopyToSaveState(PDC, PDC);
BOOL FASTCALL IntGdiDeleteDC(HDC, BOOL);

VOID FASTCALL DC_UpdateXforms(PDC  dc);
BOOL FASTCALL DC_InvertXform(const XFORM *xformSrc, XFORM *xformDest);

BOOL FASTCALL DCU_SyncDcAttrtoUser(PDC);
BOOL FASTCALL DCU_SynchDcAttrtoUser(HDC);
VOID FASTCALL DCU_SetDcUndeletable(HDC);

VOID FASTCALL IntGetViewportExtEx(PDC dc, LPSIZE pt);
VOID FASTCALL IntGetViewportOrgEx(PDC dc, LPPOINT pt);
VOID FASTCALL IntGetWindowExtEx(PDC dc, LPSIZE pt);
VOID FASTCALL IntGetWindowOrgEx(PDC dc, LPPOINT pt);

COLORREF FASTCALL IntGdiSetBkColor (HDC hDC, COLORREF Color);
INT FASTCALL IntGdiSetBkMode(HDC  hDC, INT  backgroundMode);
COLORREF STDCALL  IntGdiGetBkColor(HDC  hDC);
INT STDCALL  IntGdiGetBkMode(HDC  hDC);
COLORREF FASTCALL  IntGdiSetTextColor(HDC hDC, COLORREF color);
UINT FASTCALL IntGdiSetTextAlign(HDC  hDC, UINT  Mode);
UINT STDCALL  IntGdiGetTextAlign(HDC  hDC);
COLORREF STDCALL  IntGdiGetTextColor(HDC  hDC);
INT STDCALL  IntGdiSetStretchBltMode(HDC  hDC, INT  stretchBltMode);
VOID FASTCALL IntGdiReferencePdev(PGDIDEVICE pPDev);
VOID FASTCALL IntGdiUnreferencePdev(PGDIDEVICE pPDev, DWORD CleanUpType);
HDC FASTCALL IntGdiCreateDisplayDC(HDEV hDev, ULONG DcType, BOOL EmptyDC);
BOOL FASTCALL IntGdiCleanDC(HDC hDC);

extern PGDIDEVICE pPrimarySurface;

#endif /* not __WIN32K_DC_H */
