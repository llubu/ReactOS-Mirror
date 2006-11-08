#ifndef __REACTOS_LDR_H
#define __REACTOS_LDR_H

#define MB_FLAGS_MEM_INFO                   (0x1)
#define MB_FLAGS_BOOT_DEVICE                (0x2)
#define MB_FLAGS_COMMAND_LINE               (0x4)
#define MB_FLAGS_MODULE_INFO                (0x8)
#define MB_FLAGS_AOUT_SYMS                  (0x10)
#define MB_FLAGS_ELF_SYMS                   (0x20)
#define MB_FLAGS_MMAP_INFO                  (0x40)
#define MB_FLAGS_DRIVES_INFO                (0x80)
#define MB_FLAGS_CONFIG_TABLE               (0x100)
#define MB_FLAGS_BOOT_LOADER_NAME           (0x200)
#define MB_FLAGS_APM_TABLE                  (0x400)
#define MB_FLAGS_GRAPHICS_TABLE             (0x800)
#define MB_FLAGS_ACPI_TABLE                 (0x1000)

typedef struct _LOADER_MODULE
{
    ULONG ModStart;
    ULONG ModEnd;
    ULONG String;
    ULONG Reserved;
} LOADER_MODULE, *PLOADER_MODULE;

typedef struct _ROS_LOADER_PARAMETER_BLOCK
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
} ROS_LOADER_PARAMETER_BLOCK, *PROS_LOADER_PARAMETER_BLOCK;

extern ULONG MmFreeLdrMemHigher, MmFreeLdrMemLower;
extern BOOLEAN AcpiTableDetected;
extern ULONG MmFreeLdrPageDirectoryStart, MmFreeLdrPageDirectoryEnd;

#endif
