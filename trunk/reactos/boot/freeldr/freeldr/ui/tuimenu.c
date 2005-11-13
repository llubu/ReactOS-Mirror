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

BOOL TuiDisplayMenu(PCSTR MenuItemList[], ULONG MenuItemCount, ULONG DefaultMenuItem, LONG MenuTimeOut, ULONG* SelectedMenuItem, BOOL CanEscape, UiMenuKeyPressFilterCallback KeyPressFilter)
{
	TUI_MENU_INFO	MenuInformation;
	ULONG		LastClockSecond;
	ULONG		CurrentClockSecond;
	ULONG		KeyPress;

	//
	// The first thing we need to check is the timeout
	// If it's zero then don't bother with anything,
	// just return the default item
	//
	if (MenuTimeOut == 0)
	{
		if (SelectedMenuItem != NULL)
		{
			*SelectedMenuItem = DefaultMenuItem;
		}

		return TRUE;
	}

	//
	// Setup the MENU_INFO structure
	//
	MenuInformation.MenuItemList = MenuItemList;
	MenuInformation.MenuItemCount = MenuItemCount;
	MenuInformation.MenuTimeRemaining = MenuTimeOut;
	MenuInformation.SelectedMenuItem = DefaultMenuItem;

	//
	// Calculate the size of the menu box
	//
	TuiCalcMenuBoxSize(&MenuInformation);

	//
	// Draw the menu
	//
	TuiDrawMenu(&MenuInformation);

	//
	// Get the current second of time
	//
	MachRTCGetCurrentDateTime(NULL, NULL, NULL, NULL, NULL, &LastClockSecond);

	//
	// Process keys
	//
	while (1)
	{
		//
		// Process key presses
		//
		KeyPress = TuiProcessMenuKeyboardEvent(&MenuInformation, KeyPressFilter);
		if (KeyPress == KEY_ENTER)
		{
			//
			// If they pressed enter then exit this loop
			//
			break;
		}
		else if (CanEscape && KeyPress == KEY_ESC)
		{
			//
			// They pressed escape, so just return FALSE
			//
			return FALSE;
		}

		//
		// Update the date & time
		//
		TuiUpdateDateTime();

		VideoCopyOffScreenBufferToVRAM();

		if (MenuInformation.MenuTimeRemaining > 0)
		{
			MachRTCGetCurrentDateTime(NULL, NULL, NULL, NULL, NULL, &CurrentClockSecond);
			if (CurrentClockSecond != LastClockSecond)
			{
				//
				// Update the time information
				//
				LastClockSecond = CurrentClockSecond;
				MenuInformation.MenuTimeRemaining--;

				//
				// Update the menu
				//
				TuiDrawMenuBox(&MenuInformation);

				VideoCopyOffScreenBufferToVRAM();
			}
		}
		else if (MenuInformation.MenuTimeRemaining == 0)
		{
			//
			// A time out occurred, exit this loop and return default OS
			//
			break;
		}
	}

	//
	// Update the selected menu item information
	//
	if (SelectedMenuItem != NULL)
	{
		*SelectedMenuItem = MenuInformation.SelectedMenuItem;
	}

	return TRUE;
}

VOID TuiCalcMenuBoxSize(PTUI_MENU_INFO MenuInfo)
{
	ULONG		Idx;
	ULONG		Width;
	ULONG		Height;
	ULONG		Length;

	//
	// Height is the menu item count plus 2 (top border & bottom border)
	//
	Height = MenuInfo->MenuItemCount + 2;
	Height -= 1; // Height is zero-based

	//
	// Find the length of the longest string in the menu
	//
	Width = 0;
	for(Idx=0; Idx<MenuInfo->MenuItemCount; Idx++)
	{
		Length = strlen(MenuInfo->MenuItemList[Idx]);

		if (Length > Width)
		{
			Width = Length;
		}
	}

	//
	// Allow room for left & right borders, plus 8 spaces on each side
	//
	Width += 18;

	//
	// Calculate the menu box area
	//
	MenuInfo->Left = (UiScreenWidth - Width) / 2;
	MenuInfo->Right = (MenuInfo->Left) + Width;
	MenuInfo->Top = (((UiScreenHeight - TUI_TITLE_BOX_CHAR_HEIGHT) - Height) / 2) + TUI_TITLE_BOX_CHAR_HEIGHT;
	MenuInfo->Bottom = (MenuInfo->Top) + Height;
}

VOID TuiDrawMenu(PTUI_MENU_INFO MenuInfo)
{
	ULONG		Idx;

	//
	// Draw the backdrop
	//
	UiDrawBackdrop();

	//
	// Update the status bar
	//
	UiDrawStatusText("Use \x18\x19 to select, then press ENTER.");

	//
	// Draw the menu box
	//
	TuiDrawMenuBox(MenuInfo);

	//
	// Draw each line of the menu
	//
	for (Idx=0; Idx<MenuInfo->MenuItemCount; Idx++)
	{
		TuiDrawMenuItem(MenuInfo, Idx);
	}

	VideoCopyOffScreenBufferToVRAM();
}

VOID TuiDrawMenuBox(PTUI_MENU_INFO MenuInfo)
{
	CHAR	MenuLineText[80];
	CHAR	TempString[80];
	ULONG		Idx;

	//
	// Draw the menu box
	//
	UiDrawBox(MenuInfo->Left,
		MenuInfo->Top,
		MenuInfo->Right,
		MenuInfo->Bottom,
		D_VERT,
		D_HORZ,
		FALSE,		// Filled
		TRUE,		// Shadow
		ATTR(UiMenuFgColor, UiMenuBgColor));

	//
	// If there is a timeout draw the time remaining
	//
	if (MenuInfo->MenuTimeRemaining >= 0)
	{
		strcpy(MenuLineText, "[ Time Remaining: ");
		_itoa(MenuInfo->MenuTimeRemaining, TempString, 10);
		strcat(MenuLineText, TempString);
		strcat(MenuLineText, " ]");

		UiDrawText(MenuInfo->Right - strlen(MenuLineText) - 1,
			MenuInfo->Bottom,
			MenuLineText,
			ATTR(UiMenuFgColor, UiMenuBgColor));
	}

	//
	// Now draw the separators
	//
	for (Idx=0; Idx<MenuInfo->MenuItemCount; Idx++)
	{
		if (_stricmp(MenuInfo->MenuItemList[Idx], "SEPARATOR") == 0)
		{
			UiDrawText(MenuInfo->Left, MenuInfo->Top + Idx + 1, "\xC7", ATTR(UiMenuFgColor, UiMenuBgColor));
			UiDrawText(MenuInfo->Right, MenuInfo->Top + Idx + 1, "\xB6", ATTR(UiMenuFgColor, UiMenuBgColor));
		}
	}
}

VOID TuiDrawMenuItem(PTUI_MENU_INFO MenuInfo, ULONG MenuItemNumber)
{
	ULONG		Idx;
	CHAR	MenuLineText[80];
	ULONG		SpaceTotal;
	ULONG		SpaceLeft;
	ULONG		SpaceRight;
	UCHAR	Attribute;

	//
	// We will want the string centered so calculate
	// how many spaces will be to the left and right
	//
	SpaceTotal = (MenuInfo->Right - MenuInfo->Left - 2) - strlen(MenuInfo->MenuItemList[MenuItemNumber]);
	SpaceLeft = (SpaceTotal / 2) + 1;
	SpaceRight = (SpaceTotal - SpaceLeft) + 1;

	//
	// Insert the spaces on the left
	//
	for (Idx=0; Idx<SpaceLeft; Idx++)
	{
		MenuLineText[Idx] = ' ';
	}
	MenuLineText[Idx] = '\0';

	//
	// Now append the text string
	//
	strcat(MenuLineText, MenuInfo->MenuItemList[MenuItemNumber]);

	//
	// Now append the spaces on the right
	//
	for (Idx=0; Idx<SpaceRight; Idx++)
	{
		strcat(MenuLineText, " ");
	}

	//
	// If it is a separator then adjust the text accordingly
	//
	if (_stricmp(MenuInfo->MenuItemList[MenuItemNumber], "SEPARATOR") == 0)
	{
		memset(MenuLineText, 0, 80);
		memset(MenuLineText, 0xC4, (MenuInfo->Right - MenuInfo->Left - 1));
		Attribute = ATTR(UiMenuFgColor, UiMenuBgColor);
	}
	else
	{
		Attribute = ATTR(UiTextColor, UiMenuBgColor);
	}

	//
	// If this is the selected menu item then draw it as selected
	// otherwise just draw it using the normal colors
	//
	if (MenuItemNumber == MenuInfo->SelectedMenuItem)
	{
		UiDrawText(MenuInfo->Left + 1,
			MenuInfo->Top + 1 + MenuItemNumber,
			MenuLineText,
			ATTR(UiSelectedTextColor, UiSelectedTextBgColor));
	}
	else
	{
		UiDrawText(MenuInfo->Left + 1,
			MenuInfo->Top + 1 + MenuItemNumber,
			MenuLineText,
			Attribute);
	}
}

ULONG TuiProcessMenuKeyboardEvent(PTUI_MENU_INFO MenuInfo, UiMenuKeyPressFilterCallback KeyPressFilter)
{
	ULONG		KeyEvent = 0;

	//
	// Check for a keypress
	//
	if (MachConsKbHit())
	{
		//
		// Cancel the timeout
		//
		if (MenuInfo->MenuTimeRemaining != -1)
		{
			MenuInfo->MenuTimeRemaining = -1;
			TuiDrawMenuBox(MenuInfo);
		}

		//
		// Get the key
		//
		KeyEvent = MachConsGetCh();

		//
		// Is it extended?
		//
		if (KeyEvent == 0)
			KeyEvent = MachConsGetCh(); // Yes - so get the extended key

		//
		// Call the supplied key filter callback function to see
		// if it is going to handle this keypress.
		//
		if (KeyPressFilter != NULL)
		{
			if (KeyPressFilter(KeyEvent))
			{
				// It processed the key character
				TuiDrawMenu(MenuInfo);

				return 0;
			}
		}

		//
		// Process the key
		//
		switch (KeyEvent)
		{
		case KEY_UP:

			if (MenuInfo->SelectedMenuItem > 0)
			{
				MenuInfo->SelectedMenuItem--;

				//
				// Update the menu
				//
				TuiDrawMenuItem(MenuInfo, MenuInfo->SelectedMenuItem + 1);	// Deselect previous item

				// Skip past any separators
				if (MenuInfo->SelectedMenuItem > 0 && _stricmp(MenuInfo->MenuItemList[MenuInfo->SelectedMenuItem], "SEPARATOR") == 0)
				{
					MenuInfo->SelectedMenuItem--;
				}

				TuiDrawMenuItem(MenuInfo, MenuInfo->SelectedMenuItem);		// Select new item
			}

			break;

		case KEY_DOWN:

			if (MenuInfo->SelectedMenuItem < (MenuInfo->MenuItemCount - 1))
			{
				MenuInfo->SelectedMenuItem++;

				//
				// Update the menu
				//
				TuiDrawMenuItem(MenuInfo, MenuInfo->SelectedMenuItem - 1);	// Deselect previous item

				// Skip past any separators
				if (MenuInfo->SelectedMenuItem < (MenuInfo->MenuItemCount - 1) && _stricmp(MenuInfo->MenuItemList[MenuInfo->SelectedMenuItem], "SEPARATOR") == 0)
				{
					MenuInfo->SelectedMenuItem++;
				}

				TuiDrawMenuItem(MenuInfo, MenuInfo->SelectedMenuItem);		// Select new item
			}

			break;
		}

		VideoCopyOffScreenBufferToVRAM();
	}

	return KeyEvent;
}
