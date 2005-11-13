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

#ifndef __FREELDR_H
#define __FREELDR_H

#define UINT64_C(val) val##ULL

#include <stdlib.h>
#include <stdio.h>
#include <ddk/ntddk.h>
#include <ndk/ntndk.h>
#include <arch.h>
#include <rtl.h>
#include <disk.h>
#include <fs.h>
#include <ui.h>
#include <multiboot.h>
#include <mm.h>
#include <cache.h>
#include <machine.h>
#include <inifile.h>
#include <inffile.h>
#include <video.h>
#include <portio.h>
#include <reactos.h>
#include <registry.h>
#include <fsrec.h>
/* file system headers */
#include <fs/ext2.h>
#include <fs/fat.h>
#include <fs/ntfs.h>
#include <fs/iso.h>
/* ui support */
#include <ui/tui.h>
#include <ui/gui.h>
/* arch files */
#include <arch/i386/hardware.h>
#include <arch/i386/i386.h>
#include <arch/i386/machpc.h>
#include <arch/i386/machxbox.h>
#include <internal/i386/ke.h>
/* misc files */
#include <keycodes.h>
#include <version.h>
#include <cmdline.h>
/* Needed by boot manager */
#include <bootmgr.h>
#include <oslist.h>
#include <drivemap.h>
#include <miscboot.h>
#include <options.h>
#include <linux.h>
/* Externals */
#include <reactos/rossym.h>
#include <reactos/buildno.h>

#define Ke386EraseFlags(x)     __asm__ __volatile__("pushl $0 ; popfl\n")

extern BOOL UserInterfaceUp;	/* Tells us if the user interface is displayed */

VOID BootMain(LPSTR CmdLine);
VOID RunLoader(VOID);

#endif  // defined __FREELDR_H
