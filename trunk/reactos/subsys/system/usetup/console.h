/*
 *  ReactOS kernel
 *  Copyright (C) 2002 ReactOS Team
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
/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            subsys/system/usetup/console.h
 * PURPOSE:         Console support functions
 * PROGRAMMER:      Eric Kohl
 */

#ifndef __CONSOLE_H__
#define __CONSOLE_H__

#include <ntos/keyboard.h>

#define AllocConsole ConAllocConsole
#define FreeConsole ConFreeConsole
#define ReadConsoleOutputCharacters ConReadConsoleOutputCharacters
#define ReadConsoleOutputAttributes ConReadConsoleOutputAttributes
#define WriteConsoleOutputCharacters ConWriteConsoleOutputCharacters
#define WriteConsoleOutputAttributes ConWriteConsoleOutputAttributes
#define FillConsoleOutputAttribute ConFillConsoleOutputAttribute
#undef FillConsoleOutputCharacter
#define FillConsoleOutputCharacter ConFillConsoleOutputCharacter

NTSTATUS
ConAllocConsole(VOID);

VOID
ConFreeConsole(VOID);


NTSTATUS
ConReadConsoleOutputCharacters(LPSTR lpCharacter,
			       ULONG nLength,
			       COORD dwReadCoord,
			       PULONG lpNumberOfCharsRead);

NTSTATUS
ConReadConsoleOutputAttributes(PUSHORT lpAttribute,
			       ULONG nLength,
			       COORD dwReadCoord,
			       PULONG lpNumberOfAttrsRead);

NTSTATUS
ConWriteConsoleOutputCharacters(LPCSTR lpCharacter,
			        ULONG nLength,
			        COORD dwWriteCoord);

NTSTATUS
ConWriteConsoleOutputAttributes(CONST USHORT *lpAttribute,
			        ULONG nLength,
			        COORD dwWriteCoord,
			        PULONG lpNumberOfAttrsWritten);

NTSTATUS
ConFillConsoleOutputAttribute(USHORT wAttribute,
			      ULONG nLength,
			      COORD dwWriteCoord,
			      PULONG lpNumberOfAttrsWritten);
NTSTATUS
ConFillConsoleOutputCharacter(CHAR Character,
			      ULONG Length,
			      COORD WriteCoord,
			      PULONG NumberOfCharsWritten);

#if 0
NTSTATUS
SetConsoleMode(HANDLE hConsoleHandle,
	       ULONG dwMode);
#endif

VOID
ConInKey(PINPUT_RECORD Buffer);

VOID
ConOutChar(CHAR c);

VOID
ConOutPuts(LPSTR szText);

VOID
ConOutPrintf(LPSTR szFormat, ...);

SHORT
GetCursorX(VOID);

SHORT
GetCursorY(VOID);

VOID
GetScreenSize(SHORT *maxx,
	      SHORT *maxy);


VOID
SetCursorType(BOOL bInsert,
	      BOOL bVisible);

VOID
SetCursorXY(SHORT x,
	    SHORT y);


VOID
ClearScreen(VOID);

VOID
SetStatusText(char* fmt, ...);

VOID
InvertTextXY(SHORT x, SHORT y, SHORT col, SHORT row);

VOID
NormalTextXY(SHORT x, SHORT y, SHORT col, SHORT row);

VOID
SetTextXY(SHORT x, SHORT y, PCHAR Text);

VOID
SetInputTextXY(SHORT x, SHORT y, SHORT len, PWCHAR Text);

VOID
SetUnderlinedTextXY(SHORT x, SHORT y, PCHAR Text);

VOID
SetInvertedTextXY(SHORT x, SHORT y, PCHAR Text);

VOID
SetHighlightedTextXY(SHORT x, SHORT y, PCHAR Text);

VOID
PrintTextXY(SHORT x, SHORT y, char* fmt, ...);

VOID
PrintTextXYN(SHORT x, SHORT y, SHORT len, char* fmt, ...);

#endif /* __CONSOLE_H__*/

/* EOF */
