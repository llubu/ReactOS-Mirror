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

#ifndef __FSREC_H
#define __FSREC_H

BOOL	FsRecognizeVolume(U32 DriveNumber, U32 VolumeStartSector, U8* VolumeType);
BOOL	FsRecIsIso9660(U32 DriveNumber);
BOOL	FsRecIsExt2(U32 DriveNumber, U32 VolumeStartSector);
BOOL	FsRecIsFat(U32 DriveNumber, U32 VolumeStartSector);
BOOL	FsRecIsNtfs(U32 DriveNumber, U32 VolumeStartSector);

#endif // #defined __FSREC_H
