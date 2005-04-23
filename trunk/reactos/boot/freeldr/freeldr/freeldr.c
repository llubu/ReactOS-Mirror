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
#include <rtl.h>
#include <arch.h>
#include <machine.h>
#include <mm.h>
#include <debug.h>
#include <bootmgr.h>
#include <fs.h>
#include <cmdline.h>

VOID BootMain(char *CmdLine)
{
	CmdLineParse(CmdLine);

	MachInit();

	DebugInit();

	DbgPrint((DPRINT_WARNING, "BootMain() called. BootDrive = 0x%x BootPartition = %d\n", BootDrive, BootPartition));

	if (!MmInitializeMemoryManager())
	{
		printf("Press any key to reboot.\n");
		MachConsGetCh();
		return;
	}

	RunLoader();
}
