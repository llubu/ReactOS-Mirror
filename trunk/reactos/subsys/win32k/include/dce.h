#ifndef _WIN32K_DCE_H
#define _WIN32K_DCE_H

/* Ported from WINE by Jason Filby */

typedef struct tagDCE *PDCE;

#include <user32/wininternal.h>
#include <include/window.h>

typedef HANDLE HDCE;

/* DC hook codes */
#define DCHC_INVALIDVISRGN      0x0001
#define DCHC_DELETEDC           0x0002

#define DCHF_INVALIDATEVISRGN   0x0001
#define DCHF_VALIDATEVISRGN     0x0002

typedef enum
{
    DCE_CACHE_DC,   /* This is a cached DC (allocated by USER) */
    DCE_CLASS_DC,   /* This is a class DC (style CS_CLASSDC) */
    DCE_WINDOW_DC   /* This is a window DC (style CS_OWNDC) */
} DCE_TYPE, *PDCE_TYPE;

typedef struct tagDCE
{
    struct tagDCE *next;
    HDC          hDC;
    HWND         hwndCurrent;
    HWND         hwndDC;
    HRGN         hClipRgn;
    DCE_TYPE     type;
    DWORD        DCXFlags;
    HANDLE       Self;
} DCE;  /* PDCE already declared at top of file */

#define  DCEOBJ_AllocDCE()  \
  ((HDCE) GDIOBJ_AllocObj (GDI_OBJECT_TYPE_DCE))
#define  DCEOBJ_FreeDCE(hDCE)  GDIOBJ_FreeObj((HGDIOBJ)hDCE, GDI_OBJECT_TYPE_DCE)
#define  DCEOBJ_LockDCE(hDCE) ((PDCE)GDIOBJ_LockObj((HGDIOBJ)hDCE, GDI_OBJECT_TYPE_DCE))
#define  DCEOBJ_UnlockDCE(hDCE) GDIOBJ_UnlockObj((HGDIOBJ)hDCE)
BOOL INTERNAL_CALL DCE_Cleanup(PVOID ObjectBody);

PDCE FASTCALL DceAllocDCE(HWND hWnd, DCE_TYPE Type);
PDCE FASTCALL DCE_FreeDCE(PDCE dce);
VOID FASTCALL DCE_FreeWindowDCE(HWND);
HRGN STDCALL  DceGetVisRgn(HWND hWnd, ULONG Flags, HWND hWndChild, ULONG CFlags);
INT  FASTCALL DCE_ExcludeRgn(HDC, HWND, HRGN);
BOOL FASTCALL DCE_InvalidateDCE(HWND, const PRECTL);
HWND FASTCALL IntWindowFromDC(HDC hDc);
PDCE FASTCALL DceFreeDCE(PDCE dce, BOOLEAN Force);
void FASTCALL DceFreeWindowDCE(PWINDOW_OBJECT Window);
void FASTCALL DceEmptyCache(void);
VOID FASTCALL DceResetActiveDCEs(PWINDOW_OBJECT Window, int DeltaX, int DeltaY);

#endif /* _WIN32K_DCE_H */
