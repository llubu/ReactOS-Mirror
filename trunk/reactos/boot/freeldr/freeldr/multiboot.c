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
#include <arch.h>
#include <rtl.h>
#include <fs.h>
#include <multiboot.h>
#include <ui.h>
#include <inifile.h>
#include <mm.h>

unsigned long				next_module_load_base = 0;
module_t*	pOpenModule = NULL;


BOOL MultiBootLoadKernel(FILE *KernelImage)
{
	U32*				ImageHeaders;
	int					Idx;
	U32					dwHeaderChecksum;
	U32					dwFileLoadOffset;
	U32					dwDataSize;
	U32					dwBssSize;

	// Allocate 8192 bytes for multiboot header
	ImageHeaders = (U32*)MmAllocateMemory(8192);
	if (ImageHeaders == NULL)
	{
		return FALSE;
	}

	/*
	 * Load the first 8192 bytes of the kernel image
	 * so we can search for the multiboot header
	 */
	if (!FsReadFile(KernelImage, 8192, NULL, ImageHeaders))
	{
		MmFreeMemory(ImageHeaders);
		return FALSE;
	}

	/*
	 * Now find the multiboot header and copy it
	 */
	for (Idx=0; Idx<2048; Idx++)
	{
		// Did we find it?
		if (ImageHeaders[Idx] == MULTIBOOT_HEADER_MAGIC)
		{
			// Yes, copy it and break out of this loop
			memcpy(&mb_header, &ImageHeaders[Idx], sizeof(multiboot_header_t));

			break;
		}
	}

	MmFreeMemory(ImageHeaders);

	/*
	 * If we reached the end of the 8192 bytes without
	 * finding the multiboot header then return error
	 */
	if (Idx == 2048)
	{
		UiMessageBox("No multiboot header found!");
		return FALSE;
	}

	/*printf("multiboot header:\n");
	printf("0x%x\n", mb_header.magic);
	printf("0x%x\n", mb_header.flags);
	printf("0x%x\n", mb_header.checksum);
	printf("0x%x\n", mb_header.header_addr);
	printf("0x%x\n", mb_header.load_addr);
	printf("0x%x\n", mb_header.load_end_addr);
	printf("0x%x\n", mb_header.bss_end_addr);
	printf("0x%x\n", mb_header.entry_addr);
	getch();*/

	/*
	 * Calculate the checksum and make sure it matches
	 */
	dwHeaderChecksum = mb_header.magic;
	dwHeaderChecksum += mb_header.flags;
	dwHeaderChecksum += mb_header.checksum;
	if (dwHeaderChecksum != 0)
	{
		UiMessageBox("Multiboot header checksum invalid!");
		return FALSE;
	}
	
	/*
	 * Get the file offset, this should be 0, and move the file pointer
	 */
	dwFileLoadOffset = (Idx * sizeof(U32)) - (mb_header.header_addr - mb_header.load_addr);
	FsSetFilePointer(KernelImage, dwFileLoadOffset);
	
	/*
	 * Load the file image
	 */
	dwDataSize = (mb_header.load_end_addr - mb_header.load_addr);
	FsReadFile(KernelImage, dwDataSize, NULL, (void*)mb_header.load_addr);

	/*
	 * Initialize bss area
	 */
	dwBssSize = (mb_header.bss_end_addr - mb_header.load_end_addr);
	memset((void*)mb_header.load_end_addr, 0, dwBssSize);

	next_module_load_base = ROUND_UP(mb_header.bss_end_addr, /*PAGE_SIZE*/4096);

	return TRUE;
}

#if 0
BOOL MultiBootLoadModule(FILE *ModuleImage, char *ModuleName)
{
	U32			dwModuleSize;
	module_t*	pModule;
	char*		ModuleNameString;
	char *          TempName;
	
	/*
	 * Get current module data structure and module name string array
	 */
	pModule = &multiboot_modules[mb_info.mods_count];
	do {
	  TempName = strchr( ModuleName, '\\' );
	  if( TempName )
	    ModuleName = TempName + 1;
	} while( TempName );

	ModuleNameString = multiboot_module_strings[mb_info.mods_count];
	
	dwModuleSize = FsGetFileSize(ModuleImage);
	pModule->mod_start = next_module_load_base;
	pModule->mod_end = next_module_load_base + dwModuleSize;
	strcpy(ModuleNameString, ModuleName);
	pModule->string = (unsigned long)ModuleNameString;
	
	/*
	 * Load the file image
	 */
	FsReadFile(ModuleImage, dwModuleSize, NULL, (void*)next_module_load_base);

	next_module_load_base = ROUND_UP(pModule->mod_end, /*PAGE_SIZE*/4096);
	mb_info.mods_count++;

	return TRUE;
}
#endif

PVOID MultiBootLoadModule(FILE *ModuleImage, char *ModuleName, U32* ModuleSize)
{
	U32			dwModuleSize;
	module_t*	pModule;
	char*		ModuleNameString;
	char *          TempName;
	
	/*
	 * Get current module data structure and module name string array
	 */
	pModule = &multiboot_modules[mb_info.mods_count];
	do {
	  TempName = strchr( ModuleName, '\\' );
	  if( TempName )
	    ModuleName = TempName + 1;
	} while( TempName );

	ModuleNameString = multiboot_module_strings[mb_info.mods_count];
	
	dwModuleSize = FsGetFileSize(ModuleImage);
	pModule->mod_start = next_module_load_base;
	pModule->mod_end = next_module_load_base + dwModuleSize;
	strcpy(ModuleNameString, ModuleName);
	pModule->string = (unsigned long)ModuleNameString;
	
	/*
	 * Load the file image
	 */
	FsReadFile(ModuleImage, dwModuleSize, NULL, (void*)next_module_load_base);

	next_module_load_base = ROUND_UP(pModule->mod_end, /*PAGE_SIZE*/4096);
	mb_info.mods_count++;

	if (ModuleSize != NULL)
		*ModuleSize = dwModuleSize;

	return((PVOID)pModule->mod_start);
}

#if 0
int GetBootPartition(char *OperatingSystemName)
{
	int		BootPartitionNumber = -1;
	char	value[1024];
	U32		SectionId;

	if (IniOpenSection(OperatingSystemName, &SectionId))
	{
		if (IniReadSettingByName(SectionId, "BootPartition", value, 1024))
		{
			BootPartitionNumber = atoi(value);
		}
	}

	return BootPartitionNumber;
}
#endif

PVOID MultiBootCreateModule(char *ModuleName)
{
	module_t*	pModule;
	char*		ModuleNameString;

	/*
	 * Get current module data structure and module name string array
	 */
	pModule = &multiboot_modules[mb_info.mods_count];

	ModuleNameString = multiboot_module_strings[mb_info.mods_count];
	
	pModule->mod_start = next_module_load_base;
	pModule->mod_end = -1;
	strcpy(ModuleNameString, ModuleName);
	pModule->string = (unsigned long)ModuleNameString;

	pOpenModule = pModule;

	return((PVOID)pModule->mod_start);
}


BOOL MultiBootCloseModule(PVOID ModuleBase, U32 dwModuleSize)
{
	module_t*	pModule;

	if ((pOpenModule != NULL) &&
	    ((module_t*)ModuleBase == (module_t*)pOpenModule->mod_start) &&
	    (pOpenModule->mod_end == -1))
	{
		pModule = pOpenModule;
		pModule->mod_end = pModule->mod_start + dwModuleSize;

		next_module_load_base = ROUND_UP(pModule->mod_end, /*PAGE_SIZE*/4096);
		mb_info.mods_count++;
		pOpenModule = NULL;

		return(TRUE);
	}

	return(FALSE);
}
