#include <ntddk.h>
#define NDEBUG
#include <debug.h>
#include "../vgaddi.h"
#include "../vgavideo/vgavideo.h"
#include "brush.h"
#include "bitblt.h"

BOOL STDCALL
DrvTransparentBlt(PSURFOBJ Dest,
		  PSURFOBJ Source,
		  PCLIPOBJ Clip,
		  PXLATEOBJ ColorTranslation,
		  PRECTL DestRect,
		  PRECTL SourceRect,
		  ULONG TransparentColor,
		  ULONG Reserved)
{
  LONG dx, dy, sx, sy;

  dx = abs(DestRect->right  - DestRect->left);
  dy = abs(DestRect->bottom - DestRect->top);

  sx = abs(SourceRect->right  - SourceRect->left);
  sy = abs(SourceRect->bottom - SourceRect->top);

  if(sx<dx) dx = sx;
  if(sy<dy) dy = sy;

  // FIXME: adjust using SourceRect
  DIB_TransparentBltToVGA(DestRect->left, DestRect->top, dx, dy, Source->pvBits, Source->lDelta, TransparentColor);

  return TRUE;
}
