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

VOID RunLoader(VOID)
{
	CHAR	SettingName[80];
	CHAR	SettingValue[80];
	ULONG		SectionId;
	ULONG		OperatingSystemCount;
	PCSTR	*OperatingSystemSectionNames;
	PCSTR	*OperatingSystemDisplayNames;
	ULONG		DefaultOperatingSystem;
	LONG		TimeOut;
	ULONG		SelectedOperatingSystem;

	if (!FsOpenBootVolume())
	{
		printf("Error opening boot partition for file access.\n");
		MachConsGetCh();
		return;
	}

	if (!IniFileInitialize())
	{
		printf("Press any key to reboot.\n");
		MachConsGetCh();
		return;
	}

	if (!IniOpenSection("FreeLoader", &SectionId))
	{
		printf("Section [FreeLoader] not found in freeldr.ini.\n");
		MachConsGetCh();
		return;
	}
	TimeOut = GetTimeOut();

	if (!UiInitialize(TimeOut))
	{
		printf("Press any key to reboot.\n");
		MachConsGetCh();
		return;
	}


	if (!InitOperatingSystemList(&OperatingSystemSectionNames, &OperatingSystemDisplayNames, &OperatingSystemCount))
	{
		UiMessageBox("Press ENTER to reboot.\n");
		goto reboot;
	}

	if (OperatingSystemCount == 0)
	{
		UiMessageBox("There were no operating systems listed in freeldr.ini.\nPress ENTER to reboot.");
		goto reboot;
	}

	DefaultOperatingSystem = GetDefaultOperatingSystem(OperatingSystemSectionNames, OperatingSystemCount);

	//
	// Find all the message box settings and run them
	//
	UiShowMessageBoxesInSection("FreeLoader");

	for (;;)
	{

		/* If Timeout is 0, don't even bother loading any gui */
		if (!UserInterfaceUp) {
			SelectedOperatingSystem = DefaultOperatingSystem;
			goto NoGui;
		}

		// Redraw the backdrop
		UiDrawBackdrop();

		// Show the operating system list menu
		if (!UiDisplayMenu(OperatingSystemDisplayNames, OperatingSystemCount, DefaultOperatingSystem, TimeOut, &SelectedOperatingSystem, FALSE, MainBootMenuKeyPressFilter))
		{
			UiMessageBox("Press ENTER to reboot.\n");
			goto reboot;
		}

NoGui:
		TimeOut = -1;

		// Try to open the operating system section in the .ini file
		if (!IniOpenSection(OperatingSystemSectionNames[SelectedOperatingSystem], &SectionId))
		{
			sprintf(SettingName, "Section [%s] not found in freeldr.ini.\n", OperatingSystemSectionNames[SelectedOperatingSystem]);
			UiMessageBox(SettingName);
			continue;
		}

		// Try to read the boot type
		if (!IniReadSettingByName(SectionId, "BootType", SettingValue, 80))
		{
			sprintf(SettingName, "BootType= line not found in section [%s] in freeldr.ini.\n", OperatingSystemSectionNames[SelectedOperatingSystem]);
			UiMessageBox(SettingName);
			continue;
		}

		// Install the drive mapper according to this sections drive mappings
		DriveMapMapDrivesInSection(OperatingSystemSectionNames[SelectedOperatingSystem]);
		if (stricmp(SettingValue, "ReactOS") == 0)
		{
			LoadAndBootReactOS(OperatingSystemSectionNames[SelectedOperatingSystem]);
		}
		else if (stricmp(SettingValue, "Linux") == 0)
		{
			LoadAndBootLinux(OperatingSystemSectionNames[SelectedOperatingSystem], OperatingSystemDisplayNames[SelectedOperatingSystem]);
		}
		else if (stricmp(SettingValue, "BootSector") == 0)
		{
			LoadAndBootBootSector(OperatingSystemSectionNames[SelectedOperatingSystem]);
		}
		else if (stricmp(SettingValue, "Partition") == 0)
		{
			LoadAndBootPartition(OperatingSystemSectionNames[SelectedOperatingSystem]);
		}
		else if (stricmp(SettingValue, "Drive") == 0)
		{
			LoadAndBootDrive(OperatingSystemSectionNames[SelectedOperatingSystem]);
		}
	}


reboot:
	UiUnInitialize("Rebooting...");
	return;
}

ULONG	 GetDefaultOperatingSystem(PCSTR OperatingSystemList[], ULONG	 OperatingSystemCount)
{
	CHAR	DefaultOSText[80];
	PCSTR	DefaultOSName;
	ULONG	SectionId;
	ULONG	DefaultOS = 0;
	ULONG	Idx;

	if (!IniOpenSection("FreeLoader", &SectionId))
	{
		return 0;
	}

	DefaultOSName = CmdLineGetDefaultOS();
	if (NULL == DefaultOSName)
	{
		if (IniReadSettingByName(SectionId, "DefaultOS", DefaultOSText, 80))
		{
			DefaultOSName = DefaultOSText;
		}
	}

	if (NULL != DefaultOSName)
	{
		for (Idx=0; Idx<OperatingSystemCount; Idx++)
		{
			if (stricmp(DefaultOSName, OperatingSystemList[Idx]) == 0)
			{
				DefaultOS = Idx;
				break;
			}
		}
	}

	return DefaultOS;
}

LONG GetTimeOut(VOID)
{
	CHAR	TimeOutText[20];
	LONG		TimeOut;
	ULONG		SectionId;

	TimeOut = CmdLineGetTimeOut();
	if (0 <= TimeOut)
	{
		return TimeOut;
	}

	if (!IniOpenSection("FreeLoader", &SectionId))
	{
		return -1;
	}

	if (IniReadSettingByName(SectionId, "TimeOut", TimeOutText, 20))
	{
		TimeOut = atoi(TimeOutText);
	}
	else
	{
		TimeOut = -1;
	}

	return TimeOut;
}

BOOL MainBootMenuKeyPressFilter(ULONG KeyPress)
{
	if (KeyPress == KEY_F8)
	{
		DoOptionsMenu();

		return TRUE;
	}

	// We didn't handle the key
	return FALSE;
}
