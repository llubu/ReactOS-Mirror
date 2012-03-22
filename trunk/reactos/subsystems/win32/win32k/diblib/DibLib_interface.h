
#include "RopFunctions.h"

typedef struct
{
    ULONG iFormat;
    PBYTE pvScan0;
    PBYTE pjBase;
    LONG lDelta;
    POINTL ptOrig;
    BYTE jBpp;
} SURFINFO;

typedef struct
{
    SURFINFO siSrc;
    SURFINFO siDst;
    SURFINFO siPat;
    SURFINFO siMsk;

    ULONG ulWidth;
    ULONG ulHeight;
    ULONG ulPatWidth;
    ULONG ulPatHeight;
    XLATEOBJ *pxlo;
    PFN_XLATE pfnXlate;
    ULONG rop4;
    PFN_DOROP apfnDoRop[2];
    ULONG ulSolidColor;
    BYTE jDstBpp;
} BLTDATA, *PBLTDATA;

typedef
VOID
(FASTCALL
*PFN_DIBFUNCTION)(PBLTDATA pBltData);

VOID FASTCALL Dib_BitBlt_NOOP(PBLTDATA pBltData);
VOID FASTCALL Dib_BitBlt_SOLIDFILL(PBLTDATA pBltData);
VOID FASTCALL Dib_BitBlt_BLACKNESS(PBLTDATA pBltData);
VOID FASTCALL Dib_BitBlt_WHITENESS(PBLTDATA pBltData);
VOID FASTCALL Dib_BitBlt_PATCOPY(PBLTDATA pBltData);
VOID FASTCALL Dib_BitBlt_NOTPATCOPY(PBLTDATA pBltData);
VOID FASTCALL Dib_BitBlt_DSTINVERT(PBLTDATA pBltData);
VOID FASTCALL Dib_BitBlt_SRCCOPY(PBLTDATA pBltData);
VOID FASTCALL Dib_BitBlt_NOTSRCCOPY(PBLTDATA pBltData);
VOID FASTCALL Dib_BitBlt_SRCERASE(PBLTDATA pBltData);
VOID FASTCALL Dib_BitBlt_NOTSRCERASE(PBLTDATA pBltData);
VOID FASTCALL Dib_BitBlt_PATINVERT(PBLTDATA pBltData);
VOID FASTCALL Dib_BitBlt_SRCINVERT(PBLTDATA pBltData);
VOID FASTCALL Dib_BitBlt_SRCAND(PBLTDATA pBltData);
VOID FASTCALL Dib_BitBlt_MERGEPAINT(PBLTDATA pBltData);
VOID FASTCALL Dib_BitBlt_MERGECOPY(PBLTDATA pBltData);
VOID FASTCALL Dib_BitBlt_SRCPAINT(PBLTDATA pBltData);
VOID FASTCALL Dib_BitBlt_PATPAINT(PBLTDATA pBltData);
VOID FASTCALL Dib_SrcPatBlt(PBLTDATA pBltData);
VOID FASTCALL Dib_PatPaint(PBLTDATA pBltData);
VOID FASTCALL Dib_SrcPaint(PBLTDATA pBltData);
VOID FASTCALL Dib_BitBlt(PBLTDATA pBltData);


extern PFN_DIBFUNCTION apfnDibFunction[];
extern UCHAR aiIndexPerRop[256];

