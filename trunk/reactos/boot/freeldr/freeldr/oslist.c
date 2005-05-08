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
#include <inifile.h>
#include <oslist.h>
#include <rtl.h>
#include <mm.h>
#include <ui.h>

BOOL InitOperatingSystemList(PCHAR **SectionNamesPointer, PCHAR **DisplayNamesPointer, ULONG* OperatingSystemCountPointer)
{
	ULONG		Idx;
	ULONG		CurrentOperatingSystemIndex;
	CHAR	SettingName[260];
	CHAR	SettingValue[260];
	ULONG		OperatingSystemCount;
	ULONG		SectionId;
	ULONG		OperatingSystemSectionId;
	ULONG		SectionSettingCount;
	PCHAR	*OperatingSystemSectionNames;
	PCHAR	*OperatingSystemDisplayNames;

	//
	// Open the [FreeLoader] section
	//
	if (!IniOpenSection("Operating Systems", &SectionId))
	{
		UiMessageBox("Section [Operating Systems] not found in freeldr.ini.");
		return FALSE;
	}

	SectionSettingCount = IniGetNumSectionItems(SectionId);
	OperatingSystemCount = CountOperatingSystems(SectionId);

	//
	// Allocate memory to hold operating system lists
	//
	if (!AllocateListMemory(&OperatingSystemSectionNames, &OperatingSystemDisplayNames, OperatingSystemCount))
	{
		return FALSE;
	}

	//
	// Now loop through and read the operating system section and display names
	//
	CurrentOperatingSystemIndex = 0;
	for (Idx=0; Idx<SectionSettingCount; Idx++)
	{
		IniReadSettingByNumber(SectionId, Idx, SettingName, 260, SettingValue, 260);

		if (IniOpenSection(SettingName, &OperatingSystemSectionId))
		{
			// Copy the section name
			strcpy(OperatingSystemSectionNames[CurrentOperatingSystemIndex], SettingName);

			// Copy the display name
			RemoveQuotes(SettingValue);
			strcpy(OperatingSystemDisplayNames[CurrentOperatingSystemIndex], SettingValue);

			CurrentOperatingSystemIndex++;
		}
	}

	*OperatingSystemCountPointer = OperatingSystemCount;
	*SectionNamesPointer = OperatingSystemSectionNames;
	*DisplayNamesPointer = OperatingSystemDisplayNames;

	return TRUE;
}

ULONG CountOperatingSystems(ULONG SectionId)
{
	ULONG		Idx;
	CHAR	SettingName[260];
	CHAR	SettingValue[260];
	ULONG		OperatingSystemCount = 0;
	ULONG		SectionSettingCount;

	//
	// Loop through and count the operating systems
	//
	SectionSettingCount = IniGetNumSectionItems(SectionId);
	for (Idx=0; Idx<SectionSettingCount; Idx++)
	{
		IniReadSettingByNumber(SectionId, Idx, SettingName, 260, SettingValue, 260);

		if (IniOpenSection(SettingName, NULL))
		{
			OperatingSystemCount++;
		}
		else
		{
			sprintf(SettingName, "Operating System '%s' is listed in\nfreeldr.ini but doesn't have a [section].", SettingValue);
			UiMessageBox(SettingName);
		}
	}

	return OperatingSystemCount;
}

BOOL AllocateListMemory(PCHAR **SectionNamesPointer, PCHAR **DisplayNamesPointer, ULONG OperatingSystemCount)
{
	ULONG		Idx;
	PCHAR	*OperatingSystemSectionNames = NULL;
	PCHAR	*OperatingSystemDisplayNames = NULL;

	//
	// Allocate memory to hold operating system list arrays
	//
	OperatingSystemSectionNames = MmAllocateMemory( sizeof(PCHAR) * OperatingSystemCount);
	OperatingSystemDisplayNames = MmAllocateMemory( sizeof(PCHAR) * OperatingSystemCount);

	//
	// If either allocation failed then return FALSE
	//
	if ( (OperatingSystemSectionNames == NULL) || (OperatingSystemDisplayNames == NULL) )
	{
		if (OperatingSystemSectionNames != NULL)
		{
			MmFreeMemory(OperatingSystemSectionNames);
		}

		if (OperatingSystemDisplayNames != NULL)
		{
			MmFreeMemory(OperatingSystemDisplayNames);
		}

		return FALSE;
	}

	//
	// Clear our newly allocated memory
	//
	memset(OperatingSystemSectionNames, 0, sizeof(PCHAR) * OperatingSystemCount);
	memset(OperatingSystemDisplayNames, 0, sizeof(PCHAR) * OperatingSystemCount);

	//
	// Loop through each array element and allocate it's string memory
	//
	for (Idx=0; Idx<OperatingSystemCount; Idx++)
	{
		OperatingSystemSectionNames[Idx] = MmAllocateMemory(80);
		OperatingSystemDisplayNames[Idx] = MmAllocateMemory(80);

		//
		// If it failed then jump to the cleanup code
		//
		if ( (OperatingSystemSectionNames[Idx] == NULL) || (OperatingSystemDisplayNames[Idx] == NULL))
		{
			goto AllocateListMemoryFailed;
		}
	}

	*SectionNamesPointer = OperatingSystemSectionNames;
	*DisplayNamesPointer = OperatingSystemDisplayNames;

	return TRUE;

AllocateListMemoryFailed:

	//
	// Loop through each array element and free it's string memory
	//
	for (Idx=0; Idx<OperatingSystemCount; Idx++)
	{
		if (OperatingSystemSectionNames[Idx] != NULL)
		{
			MmFreeMemory(OperatingSystemSectionNames[Idx]);
		}

		if (OperatingSystemDisplayNames[Idx] != NULL)
		{
			MmFreeMemory(OperatingSystemDisplayNames[Idx]);
		}
	}

	//
	// Free operating system list arrays
	//
	MmFreeMemory(OperatingSystemSectionNames);
	MmFreeMemory(OperatingSystemDisplayNames);

	return FALSE;
}

BOOL RemoveQuotes(PCHAR QuotedString)
{
	CHAR	TempString[200];

	//
	// If this string is not quoted then return FALSE
	//
	if ((QuotedString[0] != '\"') && (QuotedString[strlen(QuotedString)-1] != '\"'))
	{
		return FALSE;
	}

	if (QuotedString[0] == '\"')
	{
		strcpy(TempString, (QuotedString + 1));
	}
	else
	{
		strcpy(TempString, QuotedString);
	}

	if (TempString[strlen(TempString)-1] == '\"')
	{
		TempString[strlen(TempString)-1] = '\0';
	}

	strcpy(QuotedString, TempString);

	return TRUE;
}
