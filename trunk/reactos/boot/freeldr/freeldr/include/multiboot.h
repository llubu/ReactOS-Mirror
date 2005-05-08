/* multiboot.h - the header for Multiboot */
/* Copyright (C) 1999  Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. */


#ifndef __MULTIBOOT_H
#define __MULTIBOOT_H

/* Macros. */

/* The magic number for the Multiboot header. */
#define MULTIBOOT_HEADER_MAGIC          0x1BADB002

/* The flags for the Multiboot header. */
#define MULTIBOOT_HEADER_FLAGS          0x00010003

/* The magic number passed by a Multiboot-compliant boot loader. */
#define MULTIBOOT_BOOTLOADER_MAGIC      0x2BADB002

/* The size of our stack (16KB). */
#define STACK_SIZE                      0x4000

/* C symbol format. HAVE_ASM_USCORE is defined by configure. */
#ifdef HAVE_ASM_USCORE
# define EXT_C(sym)                     _ ## sym
#else
# define EXT_C(sym)                     sym
#endif

#define MB_INFO_FLAG_MEM_SIZE			0x00000001
#define MB_INFO_FLAG_BOOT_DEVICE		0x00000002
#define MB_INFO_FLAG_COMMAND_LINE		0x00000004
#define MB_INFO_FLAG_MODULES			0x00000008
#define MB_INFO_FLAG_AOUT_SYMS			0x00000010
#define MB_INFO_FLAG_ELF_SYMS			0x00000020
#define MB_INFO_FLAG_MEMORY_MAP			0x00000040
#define MB_INFO_FLAG_DRIVES				0x00000080
#define MB_INFO_FLAG_CONFIG_TABLE		0x00000100
#define MB_INFO_FLAG_BOOT_LOADER_NAME	0x00000200
#define MB_INFO_FLAG_APM_TABLE			0x00000400
#define MB_INFO_FLAG_GRAPHICS_TABLE		0x00000800
#define MB_INFO_FLAG_ACPI_TABLE         0x00001000

#ifndef ASM
/* Do not include here in boot.S. */

/* Types. */

/* The Multiboot header. */
typedef struct multiboot_header
{
  unsigned long magic;
  unsigned long flags;
  unsigned long checksum;
  unsigned long header_addr;
  unsigned long load_addr;
  unsigned long load_end_addr;
  unsigned long bss_end_addr;
  unsigned long entry_addr;
} multiboot_header_t;

/* The symbol table for a.out. */
typedef struct aout_symbol_table
{
  unsigned long tabsize;
  unsigned long strsize;
  unsigned long addr;
  unsigned long reserved;
} aout_symbol_table_t;

/* The section header table for ELF. */
typedef struct elf_section_header_table
{
  unsigned long num;
  unsigned long size;
  unsigned long addr;
  unsigned long shndx;
} elf_section_header_table_t;

typedef struct _LOADER_PARAMETER_BLOCK
{
   ULONG Flags;
   ULONG MemLower;
   ULONG MemHigher;
   ULONG BootDevice;
   ULONG CommandLine;
   ULONG ModsCount;
   ULONG ModsAddr;
   UCHAR Syms[12];
   ULONG MmapLength;
   ULONG MmapAddr;
   ULONG DrivesCount;
   ULONG DrivesAddr;
   ULONG ConfigTable;
   ULONG BootLoaderName;
   ULONG PageDirectoryStart;
   ULONG PageDirectoryEnd;
   ULONG KernelBase;
} LOADER_PARAMETER_BLOCK, *PLOADER_PARAMETER_BLOCK;

/* The module structure. */
typedef struct _FRLDR_MODULE {
  ULONG_PTR ModuleStart;
  ULONG_PTR ModuleEnd;
  LPSTR ModuleName;
  ULONG Reserved;
} FRLDR_MODULE, *PFRLDR_MODULE;

/* The memory map. Be careful that the offset 0 is base_addr_low
   but no size. */
typedef struct memory_map
{
  //unsigned long size;
  unsigned long base_addr_low;
  unsigned long base_addr_high;
  unsigned long length_low;
  unsigned long length_high;
  unsigned long type;
  unsigned long reserved;
} memory_map_t;


LOADER_PARAMETER_BLOCK LoaderBlock; /* Multiboot info structure passed to kernel */
char					multiboot_kernel_cmdline[255];	// Command line passed to kernel
FRLDR_MODULE			multiboot_modules[64];		// Array to hold boot module info loaded for the kernel
char					multiboot_module_strings[64][256];	// Array to hold module names
unsigned long			multiboot_memory_map_descriptor_size;
memory_map_t			multiboot_memory_map[32];		// Memory map


void	boot_reactos(void);

#include "fs.h"		// Included FILE structure definition

BOOL
STDCALL
FrLdrBootReactOs(VOID);

BOOL
STDCALL
FrLdrMapKernel(FILE *KernelImage);

ULONG_PTR
STDCALL
FrLdrCreateModule(LPSTR ModuleName);

ULONG_PTR
STDCALL
FrLdrLoadModule(FILE *ModuleImage,
                LPSTR ModuleName,
                PULONG ModuleSize);

BOOL
STDCALL
FrLdrLoadKernel(PCHAR szFileName,
                INT nPos);

BOOL
FrLdrLoadNlsFile(PCHAR szSystemRoot,
                 PCHAR szErrorOut);

BOOL
FrLdrLoadDriver(PCHAR szFileName,
                INT nPos);
BOOL
LoadSymbolFile(PCHAR szSystemRoot,
               PCHAR ModuleName,
               INT nPos);

VOID
FrLdrLoadBootDrivers(PCHAR szSystemRoot,
                     INT nPos);

BOOL
STDCALL
FrLdrCloseModule(ULONG_PTR ModuleBase,
                 ULONG dwModuleSize);

VOID
STDCALL
FrLdrStartup(ULONG Magic);

VOID
FASTCALL
FrLdrGetKernelBase(VOID);

VOID
FASTCALL
FrLdrSetupPae(ULONG Magic);

VOID
FASTCALL
FrLdrGetPaeMode(VOID);

VOID
FASTCALL
FrLdrSetupPageDirectory(VOID);

VOID
LoadAndBootReactOS(PCHAR OperatingSystemName);

VOID FASTCALL AsmCode(VOID);
typedef VOID (FASTCALL *ASMCODE)(ULONG Magic,
                                 PLOADER_PARAMETER_BLOCK LoaderBlock);

int	GetBootPartition(char *OperatingSystemName);



#endif /* ! ASM */


#endif // defined __MULTIBOOT_H
