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

#ifndef __REACTOS_H
#define __REACTOS_H


///////////////////////////////////////////////////////////////////////////////////////
//
// ReactOS Loading Functions
//
///////////////////////////////////////////////////////////////////////////////////////
VOID LoadAndBootReactOS(PCHAR OperatingSystemName);

///////////////////////////////////////////////////////////////////////////////////////
//
// ReactOS Setup Loader Functions
//
///////////////////////////////////////////////////////////////////////////////////////
VOID ReactOSRunSetupLoader(VOID);

///////////////////////////////////////////////////////////////////////////////////////
//
// ARC Path Functions
//
///////////////////////////////////////////////////////////////////////////////////////
BOOL DissectArcPath(CHAR *ArcPath, CHAR *BootPath, ULONG* BootDrive, ULONG* BootPartition);
VOID ConstructArcPath(PCHAR ArcPath, PCHAR SystemFolder, ULONG Disk, ULONG Partition);
ULONG ConvertArcNameToBiosDriveNumber(PCHAR ArcPath);


#endif // defined __REACTOS_H
