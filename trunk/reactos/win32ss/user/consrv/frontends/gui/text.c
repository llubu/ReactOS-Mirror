/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/consrv/frontends/gui/text.c
 * PURPOSE:         GUI Terminal Front-End - Support for text-mode screen-buffers
 * PROGRAMMERS:     G� van Geldorp
 *                  Johannes Anderwald
 *                  Jeffrey Morlan
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#include "consrv.h"
#include "include/conio.h"
#include "include/settings.h"
#include "guisettings.h"

#define NDEBUG
#include <debug.h>


/* GLOBALS ********************************************************************/

/* Copied from consrv/text.c */
#define ConsoleAnsiCharToUnicodeChar(Console, dWChar, sChar) \
    MultiByteToWideChar((Console)->OutputCodePage, 0, (sChar), 1, (dWChar), 1)


/* FUNCTIONS ******************************************************************/

VOID
GuiCopyFromTextModeBuffer(PTEXTMODE_SCREEN_BUFFER Buffer)
{
    /*
     * This function supposes that the system clipboard was opened.
     */

    PCONSOLE Console = Buffer->Header.Console;

    HANDLE hData;
    PBYTE ptr;
    LPWSTR data, dstPos;
    ULONG selWidth, selHeight;
    ULONG xPos, yPos, size;

    selWidth  = Console->Selection.srSelection.Right - Console->Selection.srSelection.Left + 1;
    selHeight = Console->Selection.srSelection.Bottom - Console->Selection.srSelection.Top + 1;
    DPRINT("Selection is (%d|%d) to (%d|%d)\n",
           Console->Selection.srSelection.Left,
           Console->Selection.srSelection.Top,
           Console->Selection.srSelection.Right,
           Console->Selection.srSelection.Bottom);

    /* Basic size for one line and termination */
    size = selWidth + 1;
    if (selHeight > 0)
    {
        /* Multiple line selections have to get \r\n appended */
        size += ((selWidth + 2) * (selHeight - 1));
    }
    size *= sizeof(WCHAR);

    /* Allocate memory, it will be passed to the system and may not be freed here */
    hData = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, size);
    if (hData == NULL) return;

    data = GlobalLock(hData);
    if (data == NULL) return;

    DPRINT("Copying %dx%d selection\n", selWidth, selHeight);
    dstPos = data;

    for (yPos = 0; yPos < selHeight; yPos++)
    {
        ptr = ConioCoordToPointer(Buffer, 
                                  Console->Selection.srSelection.Left,
                                  yPos + Console->Selection.srSelection.Top);
        /* Copy only the characters, leave attributes alone */
        for (xPos = 0; xPos < selWidth; xPos++)
        {
            ConsoleAnsiCharToUnicodeChar(Console, &dstPos[xPos], (LPCSTR)&ptr[xPos * 2]);
        }
        dstPos += selWidth;
        if (yPos != (selHeight - 1))
        {
            wcscat(data, L"\r\n");
            dstPos += 2;
        }
    }

    DPRINT("Setting data <%S> to clipboard\n", data);
    GlobalUnlock(hData);

    EmptyClipboard();
    SetClipboardData(CF_UNICODETEXT, hData);
}

VOID
GuiPasteToTextModeBuffer(PTEXTMODE_SCREEN_BUFFER Buffer)
{
    /*
     * This function supposes that the system clipboard was opened.
     */

    PCONSOLE Console = Buffer->Header.Console;

    HANDLE hData;
    LPWSTR str;
    WCHAR CurChar = 0;

    SHORT VkKey; // MAKEWORD(low = vkey_code, high = shift_state);
    INPUT_RECORD er;

    hData = GetClipboardData(CF_UNICODETEXT);
    if (hData == NULL) return;

    str = GlobalLock(hData);
    if (str == NULL) return;

    DPRINT("Got data <%S> from clipboard\n", str);

    er.EventType = KEY_EVENT;
    er.Event.KeyEvent.wRepeatCount = 1;
    while (*str)
    {
        /* \r or \n characters. Go to the line only if we get "\r\n" sequence. */
        if (CurChar == L'\r' && *str == L'\n')
        {
            str++;
            continue;
        }
        CurChar = *str++;

        /* Get the key code (+ shift state) corresponding to the character */
        VkKey = VkKeyScanW(CurChar);
        if (VkKey == 0xFFFF)
        {
            DPRINT1("VkKeyScanW failed - Should simulate the key...\n");
            continue;
        }

        /* Pressing some control keys */

        /* Pressing the character key, with the control keys maintained pressed */
        er.Event.KeyEvent.bKeyDown = TRUE;
        er.Event.KeyEvent.wVirtualKeyCode = LOBYTE(VkKey);
        er.Event.KeyEvent.wVirtualScanCode = MapVirtualKeyW(LOBYTE(VkKey), MAPVK_VK_TO_CHAR);
        er.Event.KeyEvent.uChar.UnicodeChar = CurChar;
        er.Event.KeyEvent.dwControlKeyState = 0;
        if (HIBYTE(VkKey) & 1)
            er.Event.KeyEvent.dwControlKeyState |= SHIFT_PRESSED;
        if (HIBYTE(VkKey) & 2)
            er.Event.KeyEvent.dwControlKeyState |= LEFT_CTRL_PRESSED; // RIGHT_CTRL_PRESSED;
        if (HIBYTE(VkKey) & 4)
            er.Event.KeyEvent.dwControlKeyState |= LEFT_ALT_PRESSED; // RIGHT_ALT_PRESSED;

        ConioProcessInputEvent(Console, &er);

        /* Up all the character and control keys */
        er.Event.KeyEvent.bKeyDown = FALSE;
        ConioProcessInputEvent(Console, &er);
    }

    GlobalUnlock(hData);
}

VOID
GuiPaintTextModeBuffer(PTEXTMODE_SCREEN_BUFFER Buffer,
                       PGUI_CONSOLE_DATA GuiData,
                       HDC hDC,
                       PRECT rc)
{
    PCONSOLE Console = Buffer->Header.Console;
    // ASSERT(Console == GuiData->Console);

    ULONG TopLine, BottomLine, LeftChar, RightChar;
    ULONG Line, Char, Start;
    PBYTE From;
    PWCHAR To;
    BYTE LastAttribute, Attribute;
    ULONG CursorX, CursorY, CursorHeight;
    HBRUSH CursorBrush, OldBrush;
    HFONT OldFont;

    if (Buffer->Buffer == NULL) return;

    TopLine = rc->top / GuiData->CharHeight + Buffer->ViewOrigin.Y;
    BottomLine = (rc->bottom + (GuiData->CharHeight - 1)) / GuiData->CharHeight - 1 + Buffer->ViewOrigin.Y;
    LeftChar = rc->left / GuiData->CharWidth + Buffer->ViewOrigin.X;
    RightChar = (rc->right + (GuiData->CharWidth - 1)) / GuiData->CharWidth - 1 + Buffer->ViewOrigin.X;
    LastAttribute = ConioCoordToPointer(Buffer, LeftChar, TopLine)[1];

    SetTextColor(hDC, RGBFromAttrib(Console, TextAttribFromAttrib(LastAttribute)));
    SetBkColor(hDC, RGBFromAttrib(Console, BkgdAttribFromAttrib(LastAttribute)));

    if (BottomLine >= Buffer->ScreenBufferSize.Y) BottomLine = Buffer->ScreenBufferSize.Y - 1;
    if (RightChar >= Buffer->ScreenBufferSize.X) RightChar = Buffer->ScreenBufferSize.X - 1;

    OldFont = SelectObject(hDC, GuiData->Font);

    for (Line = TopLine; Line <= BottomLine; Line++)
    {
        WCHAR LineBuffer[80];
        From = ConioCoordToPointer(Buffer, LeftChar, Line);
        Start = LeftChar;
        To = LineBuffer;

        for (Char = LeftChar; Char <= RightChar; Char++)
        {
            if (*(From + 1) != LastAttribute || (Char - Start == sizeof(LineBuffer) / sizeof(WCHAR)))
            {
                TextOutW(hDC,
                         (Start - Buffer->ViewOrigin.X) * GuiData->CharWidth,
                         (Line - Buffer->ViewOrigin.Y) * GuiData->CharHeight,
                         LineBuffer,
                         Char - Start);
                Start = Char;
                To = LineBuffer;
                Attribute = *(From + 1);
                if (Attribute != LastAttribute)
                {
                    SetTextColor(hDC, RGBFromAttrib(Console, TextAttribFromAttrib(Attribute)));
                    SetBkColor(hDC, RGBFromAttrib(Console, BkgdAttribFromAttrib(Attribute)));
                    LastAttribute = Attribute;
                }
            }

            MultiByteToWideChar(Console->OutputCodePage,
                                0, (PCHAR)From, 1, To, 1);
            To++;
            From += 2;
        }

        TextOutW(hDC,
                 (Start - Buffer->ViewOrigin.X) * GuiData->CharWidth,
                 (Line - Buffer->ViewOrigin.Y) * GuiData->CharHeight,
                 LineBuffer,
                 RightChar - Start + 1);
    }

    /*
     * Draw the caret
     */
    if (Buffer->CursorInfo.bVisible &&
        Buffer->CursorBlinkOn &&
        !Buffer->ForceCursorOff)
    {
        CursorX = Buffer->CursorPosition.X;
        CursorY = Buffer->CursorPosition.Y;
        if (LeftChar <= CursorX && CursorX <= RightChar &&
            TopLine  <= CursorY && CursorY <= BottomLine)
        {
            CursorHeight = ConioEffectiveCursorSize(Console, GuiData->CharHeight);
            From = ConioCoordToPointer(Buffer, Buffer->CursorPosition.X, Buffer->CursorPosition.Y) + 1;

            if (*From != DEFAULT_SCREEN_ATTRIB)
            {
                CursorBrush = CreateSolidBrush(RGBFromAttrib(Console, *From));
            }
            else
            {
                CursorBrush = CreateSolidBrush(RGBFromAttrib(Console, Buffer->ScreenDefaultAttrib));
            }

            OldBrush = SelectObject(hDC, CursorBrush);
            PatBlt(hDC,
                   (CursorX - Buffer->ViewOrigin.X) * GuiData->CharWidth,
                   (CursorY - Buffer->ViewOrigin.Y) * GuiData->CharHeight + (GuiData->CharHeight - CursorHeight),
                   GuiData->CharWidth,
                   CursorHeight,
                   PATCOPY);
            SelectObject(hDC, OldBrush);
            DeleteObject(CursorBrush);
        }
    }

    SelectObject(hDC, OldFont);
}

/* EOF */
