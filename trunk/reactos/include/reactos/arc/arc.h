#ifndef _ARC_
#define _ARC_

typedef enum _IDENTIFIER_FLAG
{
    Failed = 0x01,
    ReadOnly = 0x02,
    Removable = 0x04,
    ConsoleIn = 0x08,
    ConsoleOut = 0x10,
    Input = 0x20,
    Output = 0x40
} IDENTIFIER_FLAG;

typedef enum _CONFIGURATION_CLASS
{
    SystemClass,
    ProcessorClass,
    CacheClass,
    AdapterClass,
    ControllerClass,
    PeripheralClass,
    MemoryClass,
    MaximumClass
} CONFIGURATION_CLASS;

typedef enum _TYPE_OF_MEMORY
{
    LoaderExceptionBlock,
    LoaderSystemBlock,
    LoaderFree,
    LoaderBad,
    LoaderLoadedProgram,
    LoaderFirmwareTemporary,
    LoaderFirmwarePermanent,
    LoaderOsloaderHeap,
    LoaderOsloaderStack,
    LoaderSystemCode,
    LoaderHalCode,
    LoaderBootDriver,
    LoaderConsoleInDriver,
    LoaderConsoleOutDriver,
    LoaderStartupDpcStack,
    LoaderStartupKernelStack,
    LoaderStartupPanicStack,
    LoaderStartupPcrPage,
    LoaderStartupPdrPage,
    LoaderRegistryData,
    LoaderMemoryData,
    LoaderNlsData,
    LoaderSpecialMemory,
    LoaderBBTMemory,
    LoaderReserve,
    LoaderXIPRom,
    LoaderHALCachedMemory,
    LoaderLargePageFiller,
    LoaderMaximum
} TYPE_OF_MEMORY;

typedef struct _MEMORY_ALLOCATION_DESCRIPTOR
{
    LIST_ENTRY ListEntry;
    TYPE_OF_MEMORY MemoryType;
    ULONG BasePage;
    ULONG PageCount;
} MEMORY_ALLOCATION_DESCRIPTOR, *PMEMORY_ALLOCATION_DESCRIPTOR;

typedef struct _CONFIGURATION_COMPONENT
{
    CONFIGURATION_CLASS Class;
    CONFIGURATION_TYPE Type;
    IDENTIFIER_FLAG Flags;
    USHORT Version;
    USHORT Revision;
    ULONG Key;
    ULONG AffinityMask;
    ULONG ConfigurationDataLength;
    ULONG IdentifierLength;
    LPSTR Identifier;
} CONFIGURATION_COMPONENT, *PCONFIGURATION_COMPONENT;

typedef struct _CONFIGURATION_COMPONENT_DATA
{
    struct _CONFIGURATION_COMPONENT_DATA *Parent;
    struct _CONFIGURATION_COMPONENT_DATA *Child;
    struct _CONFIGURATION_COMPONENT_DATA *Sibling;
    CONFIGURATION_COMPONENT ComponentEntry;
    PVOID ConfigurationData;
} CONFIGURATION_COMPONENT_DATA, *PCONFIGURATION_COMPONENT_DATA;

typedef struct _ARC_DISK_INFORMATION
{
    LIST_ENTRY DiskSignatureListHead;
} ARC_DISK_INFORMATION, *PARC_DISK_INFORMATION;

typedef struct _MONITOR_CONFIGURATION_DATA
{
    USHORT Version;
    USHORT Revision;
    USHORT HorizontalResolution;
    USHORT HorizontalDisplayTime;
    USHORT HorizontalBackPorch;
    USHORT HorizontalFrontPorch;
    USHORT HorizontalSync;
    USHORT VerticalResolution;
    USHORT VerticalBackPorch;
    USHORT VerticalFrontPorch;
    USHORT VerticalSync;
    USHORT HorizontalScreenSize;
    USHORT VerticalScreenSize;
} MONITOR_CONFIGURATION_DATA, *PMONITOR_CONFIGURATION_DATA;

typedef struct _FLOPPY_CONFIGURATION_DATA
{
    USHORT Version;
    USHORT Revision;
    CHAR Size[8];
    ULONG MaxDensity;
    ULONG MountDensity;
} FLOPPY_CONFIGURATION_DATA, *PFLOPPY_CONFIGURATION_DATA;

//
// SMBIOS Table Header (FIXME: maybe move to smbios.h?)
//
typedef struct _SMBIOS_TABLE_HEADER
{
   CHAR Signature[4];
   UCHAR Checksum;
   UCHAR Length;
   UCHAR MajorVersion;
   UCHAR MinorVersion;
   USHORT MaximumStructureSize;
   UCHAR EntryPointRevision;
   UCHAR Reserved[5];
   CHAR Signature2[5];
   UCHAR IntermediateChecksum;
   USHORT StructureTableLength;
   ULONG StructureTableAddress;
   USHORT NumberStructures;
   UCHAR Revision;
} SMBIOS_TABLE_HEADER, *PSMBIOS_TABLE_HEADER;

//
// NLS Data Block
//
typedef struct _NLS_DATA_BLOCK
{
    PVOID AnsiCodePageData;
    PVOID OemCodePageData;
    PVOID UnicodeCodePageData;
} NLS_DATA_BLOCK, *PNLS_DATA_BLOCK;

//
// Subsystem Specific Loader Blocks
//
typedef struct _PROFILE_PARAMETER_BLOCK
{
    USHORT Status;
    USHORT Reserved;
    USHORT DockingState;
    USHORT Capabilities;
    ULONG DockID;
    ULONG SerialNumber;
} PROFILE_PARAMETER_BLOCK, *PPROFILE_PARAMETER_BLOCK;

typedef struct _HEADLESS_LOADER_BLOCK
{
    UCHAR UsedBiosSettings;
    UCHAR DataBits;
    UCHAR StopBits;
    UCHAR Parity;
    ULONG BaudRate;
    ULONG PortNumber;
    PUCHAR PortAddress;
    USHORT PciDeviceId;
    USHORT PciVendorId;
    UCHAR PciBusNumber;
    UCHAR PciSlotNumber;
    UCHAR PciFunctionNumber;
    ULONG PciFlags;
    GUID SystemGUID;
    UCHAR IsMMIODevice;
    UCHAR TerminalType;
} HEADLESS_LOADER_BLOCK, *PHEADLESS_LOADER_BLOCK;

typedef struct _NETWORK_LOADER_BLOCK
{
    PCHAR DHCPServerACK;
    ULONG DHCPServerACKLength;
    PCHAR BootServerReplyPacket;
    ULONG BootServerReplyPacketLength;
} NETWORK_LOADER_BLOCK, *PNETWORK_LOADER_BLOCK;

typedef struct _LOADER_PERFORMANCE_DATA
{
    ULONGLONG StartTime;
    ULONGLONG EndTime;
} LOADER_PERFORMANCE_DATA, *PLOADER_PERFORMANCE_DATA;

//
// Extended Loader Parameter Block
//
typedef struct _LOADER_PARAMETER_EXTENSION
{
    ULONG Size;
    PROFILE_PARAMETER_BLOCK Profile;
    ULONG MajorVersion;
    ULONG MinorVersion;
    PVOID EmInfFileImage;
    ULONG EmInfFileSize;
    PVOID TriageDumpBlock;
    //
    // NT 5.1
    //
    ULONG LoaderPagesSpanned;
    PHEADLESS_LOADER_BLOCK HeadlessLoaderBlock;
    PSMBIOS_TABLE_HEADER SMBiosEPSHeader;
    PVOID DrvDBImage;
    ULONG DrvDBSize;
    PNETWORK_LOADER_BLOCK NetworkLoaderBlock;
    //
    // NT 5.2+
    //
    PCHAR HalpIRQLToTPR;
    PCHAR HalpVectorToIRQL;
    LIST_ENTRY FirmwareDescriptorListHead;
    PVOID AcpiTable;
    ULONG AcpiTableSize;
    //
    // NT 5.2 SP1+
    //
    ULONG BootViaWinload:1;
    ULONG BootViaEFI:1;
    ULONG Reserved:30;
    LOADER_PERFORMANCE_DATA LoaderPerformanceData;
    LIST_ENTRY BootApplicationPersistentData;
    PVOID WmdTestResult;
    GUID BootIdentifier;
} LOADER_PARAMETER_EXTENSION, *PLOADER_PARAMETER_EXTENSION;

//
// Architecture specific Loader Parameter Blocks
//
typedef struct _IA64_LOADER_BLOCK
{
    ULONG PlaceHolder;
} IA64_LOADER_BLOCK, *PIA64_LOADER_BLOCK;

typedef struct _ALPHA_LOADER_BLOCK
{
    ULONG PlaceHolder;
} ALPHA_LOADER_BLOCK, *PALPHA_LOADER_BLOCK;

typedef struct _I386_LOADER_BLOCK
{
    PVOID CommonDataArea;
    ULONG MachineType;
    ULONG Reserved;
} I386_LOADER_BLOCK, *PI386_LOADER_BLOCK;

//
// Loader Parameter Block
//
typedef struct _LOADER_PARAMETER_BLOCK
{
    LIST_ENTRY LoadOrderListHead;
    LIST_ENTRY MemoryDescriptorListHead;
    LIST_ENTRY BootDriverListHead;
    ULONG_PTR KernelStack;
    ULONG_PTR Prcb;
    ULONG_PTR Process;
    ULONG_PTR Thread;
    ULONG RegistryLength;
    PVOID RegistryBase;
    PCONFIGURATION_COMPONENT_DATA ConfigurationRoot;
    LPSTR ArcBootDeviceName;
    LPSTR ArcHalDeviceName;
    LPSTR NtBootPathName;
    LPSTR NtHalPathName;
    LPSTR LoadOptions;
    PNLS_DATA_BLOCK NlsData;
    PARC_DISK_INFORMATION ArcDiskInformation;
    PVOID OemFontFile;
    struct _SETUP_LOADER_BLOCK *SetupLdrBlock;
    PLOADER_PARAMETER_EXTENSION Extension;
    union
    {
        I386_LOADER_BLOCK I386;
        ALPHA_LOADER_BLOCK Alpha;
        IA64_LOADER_BLOCK Ia64;
    } u;
} LOADER_PARAMETER_BLOCK, *PLOADER_PARAMETER_BLOCK;

#endif
