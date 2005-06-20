#ifndef _WIN32K_INTENG_H
#define _WIN32K_INTENG_H

typedef ULONG HCLIP;

#define ENUM_RECT_LIMIT   50

typedef struct _RECT_ENUM
{
  ULONG c;
  RECTL arcl[ENUM_RECT_LIMIT];
} RECT_ENUM;

typedef struct tagSPAN
{
  LONG Y;
  LONG X;
  ULONG Width;
} SPAN, *PSPAN;

#define R3_OPINDEX_SRCCOPY 0xcc
#define R3_OPINDEX_NOOP 0xaa
#define R4_NOOP ((R3_OPINDEX_NOOP << 8) | R3_OPINDEX_NOOP)
#define R4_MASK ((R3_OPINDEX_NOOP << 8) | R3_OPINDEX_SRCCOPY)

#define ROP2_TO_MIX(Rop2) (((Rop2) << 8) | (Rop2))
#define ROP3_USES_SOURCE(Rop3) ((((Rop3) & 0xCC0000) >> 2) != ((Rop3) & 0x330000))
#define ROP4_USES_SOURCE(Rop4) (((((Rop4) & 0xCC) >> 2) != ((Rop4) & 0x33)) || ((((Rop4) & 0xCC00) >> 2) != ((Rop4) & 0x3300)))
#define ROP3_USES_PATTERN(Rop3) ((((Rop3) & 0xF00000) >> 4) != ((Rop3) & 0x0F0000))
#define ROP4_USES_PATTERN(Rop4) (((((Rop4) & 0xF0) >> 4) != ((Rop4) & 0x0F)) || ((((Rop4) & 0xF000) >> 4) != ((Rop4) & 0x0F00)))
#define ROP3_TO_ROP4(Rop3) ((((Rop3) >> 8) & 0xff00) | (((Rop3) >> 16) & 0x00ff))

/* Definitions of IntEngXxx functions */

#define IntEngLockProcessDriverObjs(W32Process) \
  ExAcquireFastMutex(&(W32Process)->DriverObjListLock)

#define IntEngUnLockProcessDriverObjs(W32Process) \
  ExReleaseFastMutex(&(W32Process)->DriverObjListLock)

VOID FASTCALL
IntEngCleanupDriverObjs(struct _EPROCESS *Process,
                        PW32PROCESS Win32Process);

BOOL STDCALL
IntEngLineTo(SURFOBJ *Surface,
             CLIPOBJ *Clip,
             BRUSHOBJ *Brush,
             LONG x1,
             LONG y1,
             LONG x2,
             LONG y2,
             RECTL *RectBounds,
             MIX mix);

BOOL STDCALL
IntEngBitBltEx(SURFOBJ *DestObj,
               SURFOBJ *SourceObj,
               SURFOBJ *Mask,
               CLIPOBJ *ClipRegion,
               XLATEOBJ *ColorTranslation,
               RECTL *DestRect,
               POINTL *SourcePoint,
               POINTL *MaskOrigin,
               BRUSHOBJ *Brush,
               POINTL *BrushOrigin,
               ROP4 Rop4,
               BOOL RemoveMouse);
#define IntEngBitBlt(DestObj, SourceObj, Mask, ClipRegion, ColorTranslation, \
                     DestRect, SourcePoint, MaskOrigin, Brush, BrushOrigin, \
                     Rop4) \
        IntEngBitBltEx((DestObj), (SourceObj), (Mask), (ClipRegion), \
                       (ColorTranslation), (DestRect), (SourcePoint), \
                       (MaskOrigin), (Brush), (BrushOrigin), (Rop4), TRUE)

BOOL STDCALL
IntEngStretchBlt(SURFOBJ *DestObj,
                 SURFOBJ *SourceObj,
                 SURFOBJ *Mask,
                 CLIPOBJ *ClipRegion,
                 XLATEOBJ *ColorTranslation,
                 RECTL *DestRect,
                 RECTL *SourceRect,
                 POINTL *pMaskOrigin,
                 BRUSHOBJ *Brush,
                 POINTL *BrushOrigin,
                 ULONG Mode);

BOOL STDCALL
IntEngGradientFill(SURFOBJ *psoDest,
                   CLIPOBJ *pco,
                   XLATEOBJ *pxlo,
                   TRIVERTEX *pVertex,
                   ULONG nVertex,
                   PVOID pMesh,
                   ULONG nMesh,
                   RECTL *prclExtents,
                   POINTL *pptlDitherOrg,
                   ULONG ulMode);

XLATEOBJ* FASTCALL
IntEngCreateXlate(USHORT DestPalType,
                  USHORT SourcePalType,
                  HPALETTE PaletteDest,
                  HPALETTE PaletteSource);

XLATEOBJ* FASTCALL
IntEngCreateMonoXlate(USHORT SourcePalType,
                      HPALETTE PaletteDest,
                      HPALETTE PaletteSource,
                      ULONG BackgroundColor);

XLATEOBJ* FASTCALL
IntEngCreateSrcMonoXlate(HPALETTE PaletteDest,
                         ULONG ForegroundColor,
                         ULONG BackgroundColor);

BOOL STDCALL
IntEngPolyline(SURFOBJ *DestSurf,
               CLIPOBJ *Clip,
               BRUSHOBJ *Brush,
               CONST LPPOINT  pt,
               LONG dCount,
               MIX mix);

CLIPOBJ* FASTCALL
IntEngCreateClipRegion(ULONG count,
                       PRECTL pRect,
                       PRECTL rcBounds);

VOID FASTCALL
IntEngDeleteClipRegion(CLIPOBJ *ClipObj);

BOOLEAN FASTCALL
ClipobjToSpans(PSPAN *Spans,
               UINT *Count,
               CLIPOBJ *ClipRegion,
               PRECTL Boundary);

BOOL FASTCALL
IntEngTransparentBlt(SURFOBJ *Dest,
                     SURFOBJ *Source,
                     CLIPOBJ *Clip,
                     XLATEOBJ *ColorTranslation,
                     PRECTL DestRect,
                     PRECTL SourceRect,
                     ULONG iTransColor,
                     ULONG Reserved);

BOOL STDCALL
IntEngPaint(IN SURFOBJ *Surface,
            IN CLIPOBJ *ClipRegion,
            IN BRUSHOBJ *Brush,
            IN POINTL *BrushOrigin,
            IN MIX Mix);

VOID STDCALL
IntEngMovePointer(IN SURFOBJ *pso,
                  IN LONG x,
                  IN LONG y,
                  IN RECTL *prcl);


#endif /* _WIN32K_INTENG_H */
