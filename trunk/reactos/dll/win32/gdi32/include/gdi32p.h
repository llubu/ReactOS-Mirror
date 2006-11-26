/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS System Libraries
 * FILE:            lib/gdi32/include/gdi32p.h
 * PURPOSE:         User-Mode Win32 GDI Library Private Header
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

#define GDI_BATCH_LIMIT 20

/* DATA **********************************************************************/

extern PGDI_TABLE_ENTRY GdiHandleTable;
extern HANDLE hProcessHeap;
extern HANDLE CurrentProcessId;
extern DWORD GDI_BatchLimit;

typedef INT
(CALLBACK* EMFPLAYPROC)(
    HDC hdc,
    INT iFunction,
    HANDLE hPageQuery
);

/* DEFINES *******************************************************************/

#define HANDLE_LIST_INC 20

/* TYPES *********************************************************************/

// Based on wmfapi.h and Wine. This is the DC_ATTR for a MetaDC file.
typedef struct tagMETAFILEDC {
  PVOID      pvMetaBuffer;
  HANDLE     hFile;
  DWORD      Size;
  PMETAHEADER mf;
  UINT       handles_size, cur_handles;
  HGDIOBJ   *handles;

  // more DC object stuff.
  HGDIOBJ    Pen;
  HGDIOBJ    Brush;
  HGDIOBJ    Palette;
  HGDIOBJ    Font;
  // Add more later.
} METAFILEDC,*PMETAFILEDC;


typedef struct tagENHMETAFILE {
  PVOID      pvMetaBuffer;
  HANDLE     hFile;      /* Handle for disk based MetaFile */
  DWORD      Size;
  PENHMETAHEADER emf;
  UINT       handles_size, cur_handles;
  HGDIOBJ   *handles;
  INT        horzres, vertres;
  INT        horzsize, vertsize;
  INT        logpixelsx, logpixelsy;
  INT        bitspixel;
  INT        textcaps;
  INT        rastercaps;
  INT        technology;
  INT        planes;
} ENHMETAFILE,*PENHMETAFILE;

/* FUNCTIONS *****************************************************************/

PVOID
HEAP_alloc(DWORD len);

NTSTATUS
HEAP_strdupA2W(
    LPWSTR* ppszW,
    LPCSTR lpszA
);

VOID
HEAP_free(LPVOID memory);

BOOL
FASTCALL
TextMetricW2A(
    TEXTMETRICA *tma,
    TEXTMETRICW *tmw
);

BOOL
FASTCALL
NewTextMetricW2A(
    NEWTEXTMETRICA *tma,
    NEWTEXTMETRICW *tmw
);

BOOL
FASTCALL
NewTextMetricExW2A(
    NEWTEXTMETRICEXA *tma,
    NEWTEXTMETRICEXW *tmw
);

BOOL
GdiIsHandleValid(HGDIOBJ hGdiObj);

BOOL
GdiGetHandleUserData(
    HGDIOBJ hGdiObj,
    PVOID *UserData
);

BOOL
WINAPI
CalculateColorTableSize(
    CONST BITMAPINFOHEADER *BitmapInfoHeader,
    UINT *ColorSpec,
    UINT *ColorTableSize
);

LPBITMAPINFO
WINAPI
ConvertBitmapInfo(
    CONST BITMAPINFO *BitmapInfo,
    UINT ColorSpec,
    UINT *BitmapInfoSize,
    BOOL FollowedByData
);

DEVMODEW *
NTAPI
GdiConvertToDevmodeW(DEVMODEA *dm);

VOID
NTAPI
LogFontA2W(
    LPLOGFONTW pW,
    CONST LOGFONTA *pA
);

VOID
NTAPI
LogFontW2A(
    LPLOGFONTA pA,
    CONST LOGFONTW *pW
);

/* FIXME: Put in some public header */
UINT
WINAPI
UserRealizePalette(HDC hDC);

/* EOF */

