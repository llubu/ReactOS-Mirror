/*
 *  FreeLoader
 *  Copyright (C) 1998-2003  Brian Palmer  <brianp@sginet.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
	
#include <freeldr.h>
#include <video.h>
#include <machine.h>

VOID VideoSavePaletteState(PPALETTE_ENTRY Palette, U32 ColorCount)
{
	U32		Color;

	for (Color=0; Color<ColorCount; Color++)
	{
		MachVideoGetPaletteColor(Color, &Palette[Color].Red, &Palette[Color].Green, &Palette[Color].Blue);
	}
}

VOID VideoRestorePaletteState(PPALETTE_ENTRY Palette, U32 ColorCount)
{
	U32		Color;

	MachVideoSync();

	for (Color=0; Color<ColorCount; Color++)
	{
		MachVideoSetPaletteColor(Color, Palette[Color].Red, Palette[Color].Green, Palette[Color].Blue);
	}
}
