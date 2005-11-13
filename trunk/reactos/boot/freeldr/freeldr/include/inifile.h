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

#ifndef __PARSEINI_H
#define __PARSEINI_H

#define INI_FILE_COMMENT_CHAR	';'



// This structure describes a single .ini file item
// The item format in the .ini file is:
// Name=Value
typedef struct
{
	LIST_ITEM	ListEntry;
	PCHAR		ItemName;
	PCHAR		ItemValue;

} INI_SECTION_ITEM, *PINI_SECTION_ITEM;

// This structure describes a .ini file section
// The section format in the .ini file is:
// [Section Name]
// This structure has a list of section items with
// one INI_SECTION_ITEM for each line in the section
typedef struct
{
	LIST_ITEM			ListEntry;
	PCHAR				SectionName;
	ULONG					SectionItemCount;
	PINI_SECTION_ITEM	SectionItemList;

} INI_SECTION, *PINI_SECTION;

extern	PINI_SECTION		IniFileSectionListHead;
extern	ULONG					IniFileSectionCount;
extern	ULONG					IniFileSettingCount;

PFILE	IniOpenIniFile();

BOOL	IniParseFile(PCHAR IniFileData, ULONG IniFileSize);
ULONG		IniGetNextLineSize(PCHAR IniFileData, ULONG IniFileSize, ULONG CurrentOffset);
ULONG		IniGetNextLine(PCHAR IniFileData, ULONG IniFileSize, PCHAR Buffer, ULONG BufferSize, ULONG CurrentOffset);
BOOL	IniIsLineEmpty(PCHAR LineOfText, ULONG TextLength);
BOOL	IniIsCommentLine(PCHAR LineOfText, ULONG TextLength);
BOOL	IniIsSectionName(PCHAR LineOfText, ULONG TextLength);
ULONG		IniGetSectionNameSize(PCHAR SectionNameLine, ULONG LineLength);
VOID	IniExtractSectionName(PCHAR SectionName, PCHAR SectionNameLine, ULONG LineLength);
BOOL	IniIsSetting(PCHAR LineOfText, ULONG TextLength);
ULONG		IniGetSettingNameSize(PCHAR SettingNameLine, ULONG LineLength);
ULONG		IniGetSettingValueSize(PCHAR SettingValueLine, ULONG LineLength);
VOID	IniExtractSettingName(PCHAR SettingName, PCHAR SettingNameLine, ULONG LineLength);
VOID	IniExtractSettingValue(PCHAR SettingValue, PCHAR SettingValueLine, ULONG LineLength);

BOOL	IniFileInitialize(VOID);

BOOL	IniOpenSection(PCSTR SectionName, ULONG* SectionId);
ULONG		IniGetNumSectionItems(ULONG SectionId);
ULONG		IniGetSectionSettingNameSize(ULONG SectionId, ULONG SettingIndex);
ULONG		IniGetSectionSettingValueSize(ULONG SectionId, ULONG SettingIndex);
BOOL	IniReadSettingByNumber(ULONG SectionId, ULONG SettingNumber, PCHAR SettingName, ULONG NameSize, PCHAR SettingValue, ULONG ValueSize);
BOOL	IniReadSettingByName(ULONG SectionId, PCSTR SettingName, PCHAR Buffer, ULONG BufferSize);
BOOL	IniAddSection(PCSTR SectionName, ULONG* SectionId);
BOOL	IniAddSettingValueToSection(ULONG SectionId, PCSTR SettingName, PCSTR SettingValue);


#endif // defined __PARSEINI_H
