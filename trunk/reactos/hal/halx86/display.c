/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002 ReactOS Team
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
/* $Id: display.c,v 1.7 2003/08/24 11:58:16 dwelch Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/hal/x86/display.c
 * PURPOSE:         Blue screen display
 * PROGRAMMER:      Eric Kohl (ekohl@rz-online.de)
 * UPDATE HISTORY:
 *                  Created 08/10/99
 */

/* DISPLAY OWNERSHIP
 *
 * So, who owns the physical display and is allowed to write to it?
 *
 * In MS NT, upon boot HAL owns the display. Somewhere in the boot
 * sequence (haven't figured out exactly where or by who), some
 * component calls HalAcquireDisplayOwnership. From that moment on,
 * the display is owned by that component and is switched to graphics
 * mode. The display is not supposed to return to text mode, except
 * in case of a bug check. The bug check will call HalDisplayString
 * to output a string to the text screen. HAL will notice that it
 * currently doesn't own the display and will re-take ownership, by
 * calling the callback function passed to HalAcquireDisplayOwnership.
 * After the bugcheck, execution is halted. So, under NT, the only
 * possible sequence of display modes is text mode -> graphics mode ->
 * text mode (the latter hopefully happening very infrequently).
 *
 * Things are a little bit different in the current state of ReactOS.
 * We want to have a functional interactive text mode. We should be
 * able to switch from text mode to graphics mode when a GUI app is
 * started and switch back to text mode when it's finished. Then, when
 * another GUI app is started, another switch to and from graphics mode
 * is possible. Also, when the system bugchecks in graphics mode we want
 * to switch back to text mode to show the registers and stack trace.
 * Last but not least, HalDisplayString is used a lot more in ReactOS,
 * e.g. to print debug messages when the /DEBUGPORT=SCREEN boot option
 * is present.
 * 3 Components are involved in Reactos: HAL, BLUE.SYS and VIDEOPRT.SYS.
 * As in NT, on boot HAL owns the display. When entering the text mode
 * command interpreter, BLUE.SYS kicks in. It will write directly to the
 * screen, more or less behind HALs back.
 * When a GUI app is started, WIN32K.SYS will open the DISPLAY device.
 * This open call will end up in VIDEOPRT.SYS. That component will then
 * take ownership of the display by calling HalAcquireDisplayOwnership.
 * When the GUI app terminates (WIN32K.SYS will close the DISPLAY device),
 * we want to give ownership of the display back to HAL. Using the
 * standard exported HAL functions, that's a bit of a problem, because
 * there is no function defined to do that. In NT, this is handled by
 * HalDisplayString, but that solution isn't satisfactory in ReactOS,
 * because HalDisplayString is (in some cases) also used to output debug
 * messages. If we do it the NT way, the first debug message output while
 * in graphics mode would switch the display back to text mode.
 * So, instead, if HalDisplayString detects that HAL doesn't have ownership
 * of the display, it doesn't do anything.
 * To return ownership to HAL, a new function is exported,
 * HalReleaseDisplayOwnership. This function is called by the DISPLAY
 * device Close routine in VIDEOPRT.SYS. It is also called at the beginning
 * of a bug check, so HalDisplayString is activated again.
 * Now, while the display is in graphics mode (not owned by HAL), BLUE.SYS
 * should also refrain from writing to the screen buffer. The text mode
 * screen buffer might overlap the graphics mode screen buffer, so changing
 * something in the text mode buffer might mess up the graphics screen. To
 * allow BLUE.SYS to detect if HAL owns the display, another new function is
 * exported, HalQueryDisplayOwnership. BLUE.SYS will call this function to
 * check if it's allowed to touch the text mode buffer.
 *
 * In an ideal world, when HAL takes ownership of the display, it should set
 * up the CRT using real-mode (actually V86 mode, but who cares) INT 0x10
 * calls. Unfortunately, this will require HAL to setup a real-mode interrupt
 * table etc. So, we chickened out of that by having the loader set up the
 * display before switching to protected mode. If HAL is given back ownership
 * after a GUI app terminates, the INT 0x10 calls are made by VIDEOPRT.SYS,
 * since there is already support for them via the VideoPortInt10 routine.
 */

#include <ddk/ntddk.h>
#include <mps.h>

#define SCREEN_SYNCHRONIZATION

#define VGA_AC_INDEX            0x3c0
#define VGA_AC_READ             0x3c0
#define VGA_AC_WRITE            0x3c1

#define VGA_MISC_WRITE          0x3c2

#define VGA_SEQ_INDEX           0x3c4
#define VGA_SEQ_DATA            0x3c5

#define VGA_DAC_READ_INDEX      0x3c7
#define VGA_DAC_WRITE_INDEX     0x3c8
#define VGA_DAC_DATA            0x3c9

#define VGA_MISC_READ           0x3cc

#define VGA_GC_INDEX            0x3ce
#define VGA_GC_DATA             0x3cf

#define VGA_CRTC_INDEX          0x3d4
#define VGA_CRTC_DATA           0x3d5

#define VGA_INSTAT_READ         0x3da

#define VGA_SEQ_NUM_REGISTERS   5
#define VGA_CRTC_NUM_REGISTERS  25
#define VGA_GC_NUM_REGISTERS    9
#define VGA_AC_NUM_REGISTERS    21

#define CRTC_COLUMNS       0x01
#define CRTC_OVERFLOW      0x07
#define CRTC_ROWS          0x12
#define CRTC_SCANLINES     0x09

#define CRTC_CURHI         0x0e
#define CRTC_CURLO         0x0f


#define CHAR_ATTRIBUTE_BLACK  0x00  /* black on black */
#define CHAR_ATTRIBUTE        0x17  /* grey on blue */


/* VARIABLES ****************************************************************/

static ULONG CursorX = 0;      /* Cursor Position */
static ULONG CursorY = 0;
static ULONG SizeX = 80;       /* Display size */
static ULONG SizeY = 25;

static BOOLEAN DisplayInitialized = FALSE;
static BOOLEAN HalOwnsDisplay = TRUE;

static WORD *VideoBuffer = NULL;

static PHAL_RESET_DISPLAY_PARAMETERS HalResetDisplayParameters = NULL;

static UCHAR TextPalette[64][3] = 
  {
    {0, 0, 0}    /* 0 */,
    {0, 0, 42}   /* 1 */,
    {0, 42, 0}   /* 2 */,
    {0, 42, 42}  /* 3 */,
    {42, 0, 0}   /* 4 */,
    {42, 42, 42} /* 5 */,
    {42, 42, 0}  /* 6 */,
    {42, 42, 42} /* 7 */,
    {0, 0, 21}   /* 8 */,
    {0, 0, 63}   /* 9 */,
    {0, 42, 21}  /* 10 */,
    {0, 42, 63}  /* 11 */,
    {42, 0, 21}  /* 12 */,
    {42, 0, 63}  /* 13 */,
    {42, 42, 21} /* 14 */,
    {42, 42, 63} /* 15 */,
    {0, 21, 0}   /* 16 */,
    {0, 21, 42}  /* 17 */,
    {0, 63, 0}   /* 18 */,
    {0, 63, 42}  /* 19 */,
    {42, 21, 0}  /* 20 */,
    {42, 21, 42} /* 21 */,
    {42, 63, 0}   /* 22 */,
    {42, 63, 42} /* 23 */,
    {0, 21, 21}  /* 24 */,
    {0, 21, 63}  /* 25 */,
    {0, 63, 21}  /* 26 */,
    {0, 63, 63}  /* 27 */,
    {42, 21, 21} /* 28 */,
    {42, 21, 63} /* 29 */,
    {42, 63, 21} /* 30 */,
    {42, 63, 63} /* 31 */,
    {21, 0, 0}   /* 32 */,
    {21, 0, 42}  /* 33 */,
    {21, 42, 0}  /* 34 */,
    {21, 42, 42} /* 35 */,
    {63, 0, 0}   /* 36 */,
    {63, 0, 42}  /* 37 */,
    {63, 42, 0}  /* 38 */,
    {63, 42, 42} /* 39 */,
    {21, 0, 21}  /* 40 */,
    {21, 0, 63}  /* 41 */,
    {21, 42, 21} /* 42 */,
    {21, 42, 63} /* 43 */,
    {63, 42, 0}  /* 44 */,
    {63, 0, 63}  /* 45 */,
    {63, 42, 21} /* 46 */,
    {63, 42, 63} /* 47 */,
    {21, 21, 0}  /* 48 */,
    {21, 21, 42} /* 49 */,
    {21, 63, 0}  /* 50 */,
    {21, 63, 42} /* 51 */,
    {63, 21, 0}  /* 52 */,
    {63, 21, 42} /* 53 */,
    {63, 63, 0}  /* 54 */,
    {63, 63, 42} /* 55 */,
    {21, 21, 21} /* 56 */,
    {21, 21, 63} /* 57 */,
    {21, 63, 21} /* 58 */,
    {21, 63, 63} /* 59 */,
    {63, 21, 21} /* 60 */,
    {63, 21, 63} /* 61 */,
    {63, 63, 21} /* 62 */,
    {63, 63, 63} /* 63 */,
  };

static UCHAR Text80x25Registers[] =
{
  /* MISC */
  0x67,
  /* SEQ */
  0x03, 0x00, 0x03, 0x00, 0x02,
  /* CRTC */
  0x5F, 0x4F, 0x50, 0x82, 0x55, 0x81, 0xBF, 0x1F,
  0x00, 0x4F, 0x0D, 0x0E, 0x00, 0x00, 0x00, 0x50,
  0x9C, 0x0E, 0x8F, 0x28, 0x1F, 0x96, 0xB9, 0xA3,
  0xFF,
  /* GC */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x0E, 0x00,
  0xFF,
  /* AC */
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07,
  0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
  0x0C, 0x00, 0x0F, 0x08, 0x00
};

static UCHAR Text80x50Registers[] =
{
  /* MISC */
  0x67,
  /* SEQ */
  0x03, 0x00, 0x03, 0x00, 0x02,
  /* CRTC */
  0x5F, 0x4F, 0x50, 0x82, 0x55, 0x81, 0xBF, 0x1F,
  0x00, 0x47, 0x06, 0x07, 0x00, 0x00, 0x01, 0x40,
  0x9C, 0x8E, 0x8F, 0x28, 0x1F, 0x96, 0xB9, 0xA3,
  0xFF, 
  /* GC */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x0E, 0x00,
  0xFF, 
  /* AC */
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07,
  0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
  0x0C, 0x00, 0x0F, 0x08, 0x00,
};


/* STATIC FUNCTIONS *********************************************************/

VOID
HalClearDisplay (UCHAR CharAttribute)
{
   WORD *ptr = (WORD*)VideoBuffer;
   ULONG i;

  for (i = 0; i < SizeX * SizeY; i++, ptr++)
    *ptr = ((CharAttribute << 8) + ' ');

  CursorX = 0;
  CursorY = 0;
}


VOID
HalScrollDisplay (VOID)
{
  WORD *ptr;
  int i;

  ptr = VideoBuffer + SizeX;
  RtlMoveMemory(VideoBuffer,
		ptr,
		SizeX * (SizeY - 1) * 2);

  ptr = VideoBuffer  + (SizeX * (SizeY - 1));
  for (i = 0; i < SizeX; i++, ptr++)
    {
      *ptr = (CHAR_ATTRIBUTE << 8) + ' ';
    }
}


static VOID
HalPutCharacter (CHAR Character)
{
  WORD *ptr;

  ptr = VideoBuffer + ((CursorY * SizeX) + CursorX);
  *ptr = (CHAR_ATTRIBUTE << 8) + Character;
}

VOID STATIC
HalSetDisplayMode(PUCHAR Registers)
{
  UCHAR Port;
  ULONG i;

  /* Write MISC register. */
  WRITE_PORT_UCHAR((PUCHAR)VGA_MISC_WRITE, *Registers);
  Registers++;
  /* Write SEQUENCER registers. */
  for (i = 0; i < VGA_SEQ_NUM_REGISTERS; i++)
    {
      WRITE_PORT_UCHAR((PUCHAR)VGA_SEQ_INDEX, i);
      WRITE_PORT_UCHAR((PUCHAR)VGA_SEQ_DATA, *Registers);
      Registers++;
    }
  /* Unlock CRTC registers. */
  WRITE_PORT_UCHAR((PUCHAR)VGA_CRTC_INDEX, 0x03);
  Port = READ_PORT_UCHAR((PUCHAR)VGA_CRTC_DATA);
  WRITE_PORT_UCHAR((PUCHAR)VGA_CRTC_DATA, Port | 0x80);
  WRITE_PORT_UCHAR((PUCHAR)VGA_CRTC_INDEX, 0x11);
  Port = READ_PORT_UCHAR((PUCHAR)VGA_CRTC_DATA);
  WRITE_PORT_UCHAR((PUCHAR)VGA_CRTC_DATA, Port & ~0x80);
  /* Make sure they stay unlocked. */
  Registers[0x03] |= 0x80;
  Registers[0x11] &= ~0x80;
  /* Write CRTC registers. */
  for (i = 0; i < VGA_CRTC_NUM_REGISTERS; i++)
    {
      WRITE_PORT_UCHAR((PUCHAR)VGA_CRTC_INDEX, i);
      WRITE_PORT_UCHAR((PUCHAR)VGA_CRTC_DATA, *Registers);
      Registers++;
    }
  /* Write GC registers. */
  for (i = 0; i < VGA_GC_NUM_REGISTERS; i++)
    {
      WRITE_PORT_UCHAR((PUCHAR)VGA_GC_INDEX, i);
      WRITE_PORT_UCHAR((PUCHAR)VGA_GC_DATA, *Registers);
      Registers++;
    }
  /* Write AC registers. */
  for (i = 0; i < VGA_AC_NUM_REGISTERS; i++)
    {
      (VOID)READ_PORT_UCHAR((PUCHAR)VGA_INSTAT_READ);
      WRITE_PORT_UCHAR((PUCHAR)VGA_AC_INDEX, i);
      WRITE_PORT_UCHAR((PUCHAR)VGA_AC_WRITE, *Registers);
      Registers++;
    }
  /* Reset palette. */
  for (i = 0; i < 64; i++)
    {
      WRITE_PORT_UCHAR((PUCHAR)0x03c8, i);
      WRITE_PORT_UCHAR((PUCHAR)0x03c9, TextPalette[i][0]);
      WRITE_PORT_UCHAR((PUCHAR)0x03c9, TextPalette[i][1]);
      WRITE_PORT_UCHAR((PUCHAR)0x03c9, TextPalette[i][2]);
    }
  /* Lock 16-colour palette and unblank display. */
  (VOID)READ_PORT_UCHAR((PUCHAR)VGA_INSTAT_READ);
  WRITE_PORT_UCHAR((PUCHAR)VGA_AC_INDEX, 0x20);  
}

/* PRIVATE FUNCTIONS ********************************************************/

VOID
HalInitializeDisplay (PLOADER_PARAMETER_BLOCK LoaderBlock)
/*
 * FUNCTION: Initalize the display
 * ARGUMENTS:
 *         InitParameters = Parameters setup by the boot loader
 */
{
  if (DisplayInitialized == FALSE)
    {
      ULONG ScanLines;
      ULONG Data;

      VideoBuffer = (WORD *)(0xd0000000 + 0xb8000);
//      VideoBuffer = HalMapPhysicalMemory (0xb8000, 2);

      /* Set cursor position */
//      CursorX = LoaderBlock->cursorx;
//      CursorY = LoaderBlock->cursory;
      CursorX = 0;
      CursorY = 0;

      /* read screen size from the crtc */
      /* FIXME: screen size should be read from the boot parameters */
      WRITE_PORT_UCHAR((PUCHAR)VGA_CRTC_INDEX, CRTC_COLUMNS);
      SizeX = READ_PORT_UCHAR((PUCHAR)VGA_CRTC_DATA) + 1;
      WRITE_PORT_UCHAR((PUCHAR)VGA_CRTC_INDEX, CRTC_ROWS);
      SizeY = READ_PORT_UCHAR((PUCHAR)VGA_CRTC_DATA);
      WRITE_PORT_UCHAR((PUCHAR)VGA_CRTC_INDEX, CRTC_OVERFLOW);
      Data = READ_PORT_UCHAR((PUCHAR)VGA_CRTC_DATA);
      SizeY |= (((Data & 0x02) << 7) | ((Data & 0x40) << 3));
      SizeY++;
      WRITE_PORT_UCHAR((PUCHAR)VGA_CRTC_INDEX, CRTC_SCANLINES);
      ScanLines = (READ_PORT_UCHAR((PUCHAR)VGA_CRTC_DATA) & 0x1F) + 1;
      SizeY = SizeY / ScanLines;

#ifdef BOCHS_30ROWS
      SizeY=30;
#endif
      HalClearDisplay(CHAR_ATTRIBUTE_BLACK);

      DisplayInitialized = TRUE;
    }
}


/* PUBLIC FUNCTIONS *********************************************************/

VOID STDCALL
HalReleaseDisplayOwnership()
/*
 * FUNCTION: Release ownership of display back to HAL
 */
{
  if (HalResetDisplayParameters == NULL)
    return;

  if (HalOwnsDisplay == TRUE)
    return;

  if (!HalResetDisplayParameters(SizeX, SizeY))
    {
      if (SizeX == 80 && SizeY == 25)
	{
	  HalSetDisplayMode(Text80x25Registers);
	}
      else
	{
	  SizeX = 80;
	  SizeY = 50;
	  HalSetDisplayMode(Text80x50Registers);
	}      
    }
  HalOwnsDisplay = TRUE;
  HalClearDisplay(CHAR_ATTRIBUTE);
}


VOID STDCALL
HalAcquireDisplayOwnership(IN PHAL_RESET_DISPLAY_PARAMETERS ResetDisplayParameters)
/*
 * FUNCTION: 
 * ARGUMENTS:
 *         ResetDisplayParameters = Pointer to a driver specific
 *         reset routine.
 */
{
  HalOwnsDisplay = FALSE;
  HalResetDisplayParameters = ResetDisplayParameters;
}

VOID STDCALL
HalDisplayString(IN PCH String)
/*
 * FUNCTION: Switches the screen to HAL console mode (BSOD) if not there
 * already and displays a string
 * ARGUMENT:
 *        string = ASCII string to display
 * NOTE: Use with care because there is no support for returning from BSOD
 * mode
 */
{
  PCH pch;
#ifdef SCREEN_SYNCHRONIZATION
  int offset;
#endif
  static KSPIN_LOCK Lock;
  ULONG Flags;

  /* See comment at top of file */
  if (! HalOwnsDisplay)
    {
      return;
    }

  pch = String;

  pushfl(Flags);
  __asm__ ("cli\n\t");
  KeAcquireSpinLockAtDpcLevel(&Lock);

#if 0  
  if (HalOwnsDisplay == FALSE)
    {
      HalReleaseDisplayOwnership();
    }
#endif
  
#ifdef SCREEN_SYNCHRONIZATION
  WRITE_PORT_UCHAR((PUCHAR)VGA_CRTC_INDEX, CRTC_CURHI);
  offset = READ_PORT_UCHAR((PUCHAR)VGA_CRTC_DATA)<<8;
  WRITE_PORT_UCHAR((PUCHAR)VGA_CRTC_INDEX, CRTC_CURLO);
  offset += READ_PORT_UCHAR((PUCHAR)VGA_CRTC_DATA);
  
  CursorY = offset / SizeX;
  CursorX = offset % SizeX;
#endif
  
  while (*pch != 0)
    {
      if (*pch == '\n')
	{
	  CursorY++;
	  CursorX = 0;
	}
      else if (*pch != '\r')
	{
	  HalPutCharacter (*pch);
	  CursorX++;
	  
	  if (CursorX >= SizeX)
	    {
	      CursorY++;
	      CursorX = 0;
	    }
	}
  
      if (CursorY >= SizeY)
	{
	  HalScrollDisplay ();
	  CursorY = SizeY - 1;
	}
  
      pch++;
    }
  
#ifdef SCREEN_SYNCHRONIZATION
  offset = (CursorY * SizeX) + CursorX;
  
  WRITE_PORT_UCHAR((PUCHAR)VGA_CRTC_INDEX, CRTC_CURLO);
  WRITE_PORT_UCHAR((PUCHAR)VGA_CRTC_DATA, offset & 0xff);
  WRITE_PORT_UCHAR((PUCHAR)VGA_CRTC_INDEX, CRTC_CURHI);
  WRITE_PORT_UCHAR((PUCHAR)VGA_CRTC_DATA, (offset >> 8) & 0xff);
#endif
  KeReleaseSpinLockFromDpcLevel(&Lock);
  popfl(Flags);
}

VOID STDCALL
HalQueryDisplayParameters(OUT PULONG DispSizeX,
			  OUT PULONG DispSizeY,
			  OUT PULONG CursorPosX,
			  OUT PULONG CursorPosY)
{
  if (DispSizeX)
    *DispSizeX = SizeX;
  if (DispSizeY)
    *DispSizeY = SizeY;
  if (CursorPosX)
    *CursorPosX = CursorX;
  if (CursorPosY)
    *CursorPosY = CursorY;
}


VOID STDCALL
HalSetDisplayParameters(IN ULONG CursorPosX,
			IN ULONG CursorPosY)
{
  CursorX = (CursorPosX < SizeX) ? CursorPosX : SizeX - 1;
  CursorY = (CursorPosY < SizeY) ? CursorPosY : SizeY - 1;
}

BOOLEAN STDCALL
HalQueryDisplayOwnership()
{
  return ! HalOwnsDisplay;
}

/* EOF */
