/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           GDI Palette Functions
 * FILE:              subsys/win32k/eng/palette.c
 * PROGRAMER:         Jason Filby
 * REVISION HISTORY:
 *                 11/7/1999: Created
 */

#include <ddk/winddi.h>
#include <include/object.h>
#include "objects.h"

HPALETTE STDCALL
EngCreatePalette(ULONG Mode,
		 ULONG NumColors,
		 PULONG *Colors, // FIXME: This was implemented with ULONG *Colors!!
		 ULONG Red,
		 ULONG Green,
		 ULONG Blue)
{
  HPALETTE NewPalette;
  PALOBJ *PalObj;
  PALGDI *PalGDI;

  PalObj = EngAllocMem(FL_ZERO_MEMORY, sizeof(PALOBJ), 0);
  PalGDI = EngAllocMem(FL_ZERO_MEMORY, sizeof(PALGDI), 0);

  NewPalette = (HPALETTE)CreateGDIHandle(PalGDI, PalObj);

  PalGDI->Mode = Mode;

  if(Colors != NULL)
  {
    PalGDI->IndexedColors = ExAllocatePool(NonPagedPool, sizeof(PALETTEENTRY) * NumColors);
    RtlCopyMemory(PalGDI->IndexedColors, Colors, sizeof(PALETTEENTRY) * NumColors);
  }

  if(Mode==PAL_INDEXED)
  {
    PalGDI->NumColors     = NumColors;
    PalGDI->IndexedColors = (PULONG)Colors;
  } else
  if(Mode==PAL_BITFIELDS)
  {
    PalGDI->RedMask   = Red;
    PalGDI->GreenMask = Green;
    PalGDI->BlueMask  = Blue;
  }

  return NewPalette;
}

BOOL STDCALL
EngDeletePalette(IN HPALETTE Palette)
{
  PALOBJ *PalObj;
  PALGDI *PalGDI;

  PalGDI = (PALGDI*)AccessInternalObject((ULONG)Palette);
  PalObj = (PALOBJ*)AccessUserObject((ULONG)Palette);

  EngFreeMem(PalGDI);
  EngFreeMem(PalObj);
  FreeGDIHandle((ULONG)Palette);

  return TRUE;
}

ULONG STDCALL
PALOBJ_cGetColors(PALOBJ *PalObj,
		  ULONG Start,
		  ULONG Colors,
		  ULONG *PaletteEntry)
{
  ULONG i, entry;
  PALGDI *PalGDI;

  PalGDI = (PALGDI*)AccessInternalObjectFromUserObject(PalObj);

  for(i=Start; i<Colors; i++)
  {
    PaletteEntry[i] = PalGDI->IndexedColors[i];
  }

  return Colors;
}
