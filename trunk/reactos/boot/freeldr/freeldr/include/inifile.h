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

BOOL	IniFileInitialize(VOID);

BOOL	IniOpenSection(PUCHAR SectionName, ULONG* SectionId);
ULONG		IniGetNumSectionItems(ULONG SectionId);
ULONG		IniGetSectionSettingNameSize(ULONG SectionId, ULONG SettingIndex);
ULONG		IniGetSectionSettingValueSize(ULONG SectionId, ULONG SettingIndex);
BOOL	IniReadSettingByNumber(ULONG SectionId, ULONG SettingNumber, PUCHAR SettingName, ULONG NameSize, PUCHAR SettingValue, ULONG ValueSize);
BOOL	IniReadSettingByName(ULONG SectionId, PUCHAR SettingName, PUCHAR Buffer, ULONG BufferSize);
BOOL	IniAddSection(PUCHAR SectionName, ULONG* SectionId);
BOOL	IniAddSettingValueToSection(ULONG SectionId, PUCHAR SettingName, PUCHAR SettingValue);


#endif // defined __PARSEINI_H
