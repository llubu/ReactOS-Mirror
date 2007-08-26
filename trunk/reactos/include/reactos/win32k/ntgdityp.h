/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Win32 Graphical Subsystem (WIN32K)
 * FILE:            include/win32k/ntgdityp.h
 * PURPOSE:         Win32 Shared GDI Types for NtGdi*
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#ifndef _NTGDITYP_
#define _NTGDITYP_

/* ENUMERATIONS **************************************************************/

typedef enum _ARCTYPE
{
    GdiTypeArc,
    GdiTypeArcTo,
    GdiTypeChord,
    GdiTypePie,
} ARCTYPE, *PARCTYPE;

typedef enum _PALFUNCTYPE
{
    GdiPalAnimate,
    GdiPalSetEntries,
    GdiPalGetEntries,
    GdiPalGetSystemEntries,
    GdiPalSetColorTable,
    GdiPalGetColorTable,
} PALFUNCTYPE, *PPALFUNCTYPE;

typedef enum _POLYFUNCTYPE
{
    GdiPolyPolygon = 1,
    GdiPolyPolyLine,
    GdiPolyBezier,
    GdiPolyLineTo,
    GdiPolyBezierTo,
    GdiPolyPolyRgn,
} POLYFUNCTYPE, *PPOLYFUNCTYPE;

typedef enum _GETDCDWORD
{
    GdiGetJournal,
    GdiGetRelAbs,
    GdiGetBreakExtra,
    GdiGerCharBreak,
    GdiGetArcDirection,
    GdiGetEMFRestorDc,
    GdiGetFontLanguageInfo,
    GdiGetIsMemDc,
    GdiGetMapMode,
    GdiGetTextCharExtra,
} GETDCDWORD, *PGETDCDWORD;

typedef enum _GETSETDCDWORD
{
    GdtGetSetCopyCount = 2,
    GdiGetSetTextAlign,
    GdiGetSetRelAbs,
    GdiGetSetTextCharExtra,
    GdiGetSetSelectFont,
    GdiGetSetMapperFlagsInternal,
    GdiGetSetMapMode,
    GdiGetSetArcDirection,
} GETSETDCDWORD, *PGETSETDCDWORD;

typedef enum _GETDCPOINT
{
    GdiGetViewPortExt = 1,
    GdiGetWindowExt,
    GdiGetViewPortOrg,
    GdiGetWindowOrg,
    GdiGetAspectRatioFilter,
    GdiGetDCOrg = 6,
} GETDCPOINT, *PGETDCPOINT;

typedef enum _TRANSFORMTYPE
{
    GdiDpToLp,
    GdiLpToDp,
} TRANSFORMTYPE, *PTRANSFORMTYPE;

#define GdiWorldSpaceToPageSpace    0x203

/* FIXME: Unknown */
typedef DWORD FULLSCREENCONTROL;
typedef DWORD LFTYPE;

/* TYPES *********************************************************************/

typedef PVOID KERNEL_PVOID;
typedef DWORD UNIVERSAL_FONT_ID;
typedef UNIVERSAL_FONT_ID *PUNIVERSAL_FONT_ID;
typedef DWORD CHWIDTHINFO;
typedef CHWIDTHINFO *PCHWIDTHINFO;
typedef D3DNTHAL_CONTEXTCREATEDATA D3DNTHAL_CONTEXTCREATEI;
typedef LONG FIX;

/* FIXME: Unknown; easy to guess, usually based on public types and converted */
typedef struct _WIDTHDATA WIDTHDATA, *PWIDTHDATA;
typedef struct _DEVCAPS DEVCAPS, *PDEVCAPS;
typedef struct _REALIZATION_INFO REALIZATION_INFO, *PREALIZATION_INFO;

/* Font Structures */
typedef struct _TMDIFF
{
    ULONG cjotma;
    CHAR chFirst;
    CHAR chLast;
    CHAR ChDefault;
    CHAR ChBreak;
} TMDIFF, *PTMDIFF;

typedef struct _TMW_INTERNAL
{
    TEXTMETRICW TextMetric;
    TMDIFF Diff;
} TMW_INTERNAL, *PTMW_INTERNAL;

typedef struct _ENUMFONTDATAW 
{
    DWORD            cbSize;  // Size of ENUMFONTDATAW + (size of ENUMTEXTMETRIC + ENUMLOGFONTEXDV)
    PENUMTEXTMETRIC  lpentm;  // Ptr with in this structure.              ^^^ if Win2k + ^^^
    DWORD            FontType;
    PENUMLOGFONTEXDV lpelfedv; // Prt with in this structure.
    // The rest of the structure is variable in size.
    // Based on iEnumType and Win32Compat flags,,
    // if w2k+ {ENUMTEXTMETRIC else NEWTEXTMETRICS} or TEXTMETRIC
    // if w2k+ {ENUMLOGFONTEXDV else ENUMLOGFONT} or LOGFONT
} ENUMFONTDATAW, *PENUMFONTDATAW;

/* Number Representation */
typedef struct _EFLOAT_S
{
    LONG lMant;
    LONG lExp;
} EFLOAT_S;

/* XFORM Structures */
typedef struct _MATRIX_S
{
    EFLOAT_S efM11;
    EFLOAT_S efM12;
    EFLOAT_S efM21;
    EFLOAT_S efM22;
    EFLOAT_S efDx;
    EFLOAT_S efDy;
    FIX fxDx;
    FIX fxDy;
    FLONG flAccel;
} MATRIX_S;

/* Gdi XForm storage union */
typedef union
{
  FLOAT f;
  ULONG l;
} gxf_long;


#endif
