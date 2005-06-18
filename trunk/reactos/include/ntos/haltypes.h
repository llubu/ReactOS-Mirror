/* $Id$
 *
 * COPYRIGHT:                See COPYING in the top level directory
 * PROJECT:                  ReactOS kernel
 * FILE:                     include/ddk/haltypes.h
 * PURPOSE:                  HAL provided defintions for device drivers
 * PROGRAMMER:               David Welch (welch@mcmail.com)
 * REVISION HISTORY:
 *              23/06/98:   Taken from linux system.h
 */


#ifndef __INCLUDE_NTOS_HALTYPES_H
#define __INCLUDE_NTOS_HALTYPES_H

#ifndef STDCALL_FUNC
#define STDCALL_FUNC(a)  (STDCALL a)
#endif
#ifndef FASTCALL_FUNC
#define FASTCALL_FUNC(a) (FASTCALL a)
#endif

#include "types.h"


/* HalReturnToFirmware */
#define FIRMWARE_HALT   1
#define FIRMWARE_REBOOT 3

#ifndef __USE_W32API

enum
{
   DEVICE_DESCRIPTION_VERSION,
   DEVICE_DESCRIPTION_VERSION1,
};

/*
 * DMA speed specifiers
 */
typedef enum _DMA_SPEED
{
  Compatible,
  TypeA,
  TypeB,
  TypeC,
  TypeF,
  MaximumDmaSpeed
} DMA_SPEED, *PDMA_SPEED;

/*
 * DMA width specifiers
 */
typedef enum _DMA_WIDTH
{
  Width8Bits,
  Width16Bits,
  Width32Bits,
  MaximumDmaWidth
} DMA_WIDTH, *PDMA_WIDTH;

/*
 * PURPOSE: Types for HalGetBusData
 */
typedef enum _BUS_DATA_TYPE
{
   ConfigurationSpaceUndefined = -1,
   Cmos,
   EisaConfiguration,
   Pos,
   CbusConfiguration,
   PCIConfiguration,
   VMEConfiguration,
   NuBusConfiguration,
   PCMCIAConfiguration,
   MPIConfiguration,
   MPSAConfiguration,
   PNPISAConfiguration,
   MaximumBusDataType,
} BUS_DATA_TYPE, *PBUS_DATA_TYPE;

typedef struct _DEVICE_DESCRIPTION
{
  ULONG Version;
  BOOLEAN Master;
  BOOLEAN ScatterGather;
  BOOLEAN DemandMode;
  BOOLEAN AutoInitialize;
  BOOLEAN Dma32BitAddresses;
  BOOLEAN IgnoreCount;
  BOOLEAN Reserved1;        /* Must be false */
  BOOLEAN Dma64BitAddresses;
  ULONG BusNumber;
  ULONG DmaChannel;
  INTERFACE_TYPE InterfaceType;
  DMA_WIDTH DmaWidth;
  DMA_SPEED DmaSpeed;
  ULONG MaximumLength;
  ULONG DmaPort;
} DEVICE_DESCRIPTION, *PDEVICE_DESCRIPTION;


/* PCI bus definitions */

#define PCI_TYPE0_ADDRESSES	6
#define PCI_TYPE1_ADDRESSES	2
#define PCI_TYPE2_ADDRESSES	5

typedef struct _PCI_COMMON_CONFIG
{
  USHORT VendorID;		/* read-only */
  USHORT DeviceID;		/* read-only */
  USHORT Command;
  USHORT Status;
  UCHAR  RevisionID;		/* read-only */
  UCHAR  ProgIf;		/* read-only */
  UCHAR  SubClass;		/* read-only */
  UCHAR  BaseClass;		/* read-only */
  UCHAR  CacheLineSize;		/* read-only */
  UCHAR  LatencyTimer;		/* read-only */
  UCHAR  HeaderType;		/* read-only */
  UCHAR  BIST;
  union
    {
      struct _PCI_HEADER_TYPE_0
	{
	  ULONG  BaseAddresses[PCI_TYPE0_ADDRESSES];
	  ULONG  CIS;
	  USHORT SubVendorID;
	  USHORT SubSystemID;
	  ULONG  ROMBaseAddress;
	  ULONG  Reserved2[2];

	  UCHAR  InterruptLine;
	  UCHAR  InterruptPin;		/* read-only */
	  UCHAR  MinimumGrant;		/* read-only */
	  UCHAR  MaximumLatency;	/* read-only */
	} type0;

      /* PCI to PCI Bridge */
      struct _PCI_HEADER_TYPE_1
	{
	  ULONG  BaseAddresses[PCI_TYPE1_ADDRESSES];
	  UCHAR  PrimaryBus;
	  UCHAR  SecondaryBus;
	  UCHAR  SubordinateBus;
	  UCHAR  SecondaryLatency;
	  UCHAR  IOBase;
	  UCHAR  IOLimit;
	  USHORT SecondaryStatus;
	  USHORT MemoryBase;
	  USHORT MemoryLimit;
	  USHORT PrefetchBase;
	  USHORT PrefetchLimit;
	  ULONG  PrefetchBaseUpper32;
	  ULONG  PrefetchLimitUpper32;
	  USHORT IOBaseUpper16;
	  USHORT IOLimitUpper16;
	  UCHAR  CapabilitiesPtr;
	  UCHAR  Reserved1[3];
	  ULONG  ROMBaseAddress;
	  UCHAR  InterruptLine;
	  UCHAR  InterruptPin;
	  USHORT BridgeControl;
	} type1;

      /* PCI to CARDBUS Bridge */
      struct _PCI_HEADER_TYPE_2
	{
	  ULONG  SocketRegistersBaseAddress;
	  UCHAR  CapabilitiesPtr;
	  UCHAR  Reserved;
	  USHORT SecondaryStatus;
	  UCHAR  PrimaryBus;
	  UCHAR  SecondaryBus;
	  UCHAR  SubordinateBus;
	  UCHAR  SecondaryLatency;
	  struct
	    {
	      ULONG Base;
	      ULONG Limit;
	    } Range[PCI_TYPE2_ADDRESSES-1];
	  UCHAR  InterruptLine;
	  UCHAR  InterruptPin;
	  USHORT BridgeControl;
	} type2;
    } u;
  UCHAR DeviceSpecific[192];
} PCI_COMMON_CONFIG, *PPCI_COMMON_CONFIG;

#define PCI_COMMON_HDR_LENGTH (FIELD_OFFSET (PCI_COMMON_CONFIG, DeviceSpecific))

#define PCI_MAX_DEVICES                     32
#define PCI_MAX_FUNCTION                    8
#define PCI_MAX_BRIDGE_NUMBER               0xFF

#define PCI_INVALID_VENDORID                0xFFFF

/* Bit encodings for PCI_COMMON_CONFIG.HeaderType */

#define PCI_MULTIFUNCTION                   0x80
#define PCI_DEVICE_TYPE                     0x00
#define PCI_BRIDGE_TYPE                     0x01
#define PCI_CARDBUS_BRIDGE_TYPE             0x02

#define PCI_CONFIGURATION_TYPE(PciData) \
	(((PPCI_COMMON_CONFIG)(PciData))->HeaderType & ~PCI_MULTIFUNCTION)

#define PCI_MULTIFUNCTION_DEVICE(PciData) \
	((((PPCI_COMMON_CONFIG)(PciData))->HeaderType & PCI_MULTIFUNCTION) != 0)


/* Bit encodings for PCI_COMMON_CONFIG.Command */

#define PCI_ENABLE_IO_SPACE                 0x0001
#define PCI_ENABLE_MEMORY_SPACE             0x0002
#define PCI_ENABLE_BUS_MASTER               0x0004
#define PCI_ENABLE_SPECIAL_CYCLES           0x0008
#define PCI_ENABLE_WRITE_AND_INVALIDATE     0x0010
#define PCI_ENABLE_VGA_COMPATIBLE_PALETTE   0x0020
#define PCI_ENABLE_PARITY                   0x0040
#define PCI_ENABLE_WAIT_CYCLE               0x0080
#define PCI_ENABLE_SERR                     0x0100
#define PCI_ENABLE_FAST_BACK_TO_BACK        0x0200


/* Bit encodings for PCI_COMMON_CONFIG.Status */

#define PCI_STATUS_FAST_BACK_TO_BACK        0x0080
#define PCI_STATUS_DATA_PARITY_DETECTED     0x0100
#define PCI_STATUS_DEVSEL                   0x0600  /* 2 bits wide */
#define PCI_STATUS_SIGNALED_TARGET_ABORT    0x0800
#define PCI_STATUS_RECEIVED_TARGET_ABORT    0x1000
#define PCI_STATUS_RECEIVED_MASTER_ABORT    0x2000
#define PCI_STATUS_SIGNALED_SYSTEM_ERROR    0x4000
#define PCI_STATUS_DETECTED_PARITY_ERROR    0x8000


/* PCI device classes */

#define PCI_CLASS_PRE_20                    0x00
#define PCI_CLASS_MASS_STORAGE_CTLR         0x01
#define PCI_CLASS_NETWORK_CTLR              0x02
#define PCI_CLASS_DISPLAY_CTLR              0x03
#define PCI_CLASS_MULTIMEDIA_DEV            0x04
#define PCI_CLASS_MEMORY_CTLR               0x05
#define PCI_CLASS_BRIDGE_DEV                0x06
#define PCI_CLASS_SIMPLE_COMMS_CTLR         0x07
#define PCI_CLASS_BASE_SYSTEM_DEV           0x08
#define PCI_CLASS_INPUT_DEV                 0x09
#define PCI_CLASS_DOCKING_STATION           0x0a
#define PCI_CLASS_PROCESSOR                 0x0b
#define PCI_CLASS_SERIAL_BUS_CTLR           0x0c


/* PCI device subclasses for class 0 */

#define PCI_SUBCLASS_PRE_20_NON_VGA         0x00
#define PCI_SUBCLASS_PRE_20_VGA             0x01


/* PCI device subclasses for class 1 (mass storage controllers)*/

#define PCI_SUBCLASS_MSC_SCSI_BUS_CTLR      0x00
#define PCI_SUBCLASS_MSC_IDE_CTLR           0x01
#define PCI_SUBCLASS_MSC_FLOPPY_CTLR        0x02
#define PCI_SUBCLASS_MSC_IPI_CTLR           0x03
#define PCI_SUBCLASS_MSC_RAID_CTLR          0x04
#define PCI_SUBCLASS_MSC_OTHER              0x80


/* PCI device subclasses for class 2 (network controllers)*/

#define PCI_SUBCLASS_NET_ETHERNET_CTLR      0x00
#define PCI_SUBCLASS_NET_TOKEN_RING_CTLR    0x01
#define PCI_SUBCLASS_NET_FDDI_CTLR          0x02
#define PCI_SUBCLASS_NET_ATM_CTLR           0x03
#define PCI_SUBCLASS_NET_OTHER              0x80


/* PCI device subclasses for class 3 (display controllers)*/

#define PCI_SUBCLASS_VID_VGA_CTLR           0x00
#define PCI_SUBCLASS_VID_XGA_CTLR           0x01
#define PCI_SUBLCASS_VID_3D_CTLR            0x02
#define PCI_SUBCLASS_VID_OTHER              0x80


/* PCI device subclasses for class 4 (multimedia device)*/

#define PCI_SUBCLASS_MM_VIDEO_DEV           0x00
#define PCI_SUBCLASS_MM_AUDIO_DEV           0x01
#define PCI_SUBCLASS_MM_TELEPHONY_DEV       0x02
#define PCI_SUBCLASS_MM_OTHER               0x80


/* PCI device subclasses for class 5 (memory controller)*/

#define PCI_SUBCLASS_MEM_RAM                0x00
#define PCI_SUBCLASS_MEM_FLASH              0x01
#define PCI_SUBCLASS_MEM_OTHER              0x80


/* PCI device subclasses for class 6 (bridge device)*/

#define PCI_SUBCLASS_BR_HOST                0x00
#define PCI_SUBCLASS_BR_ISA                 0x01
#define PCI_SUBCLASS_BR_EISA                0x02
#define PCI_SUBCLASS_BR_MCA                 0x03
#define PCI_SUBCLASS_BR_PCI_TO_PCI          0x04
#define PCI_SUBCLASS_BR_PCMCIA              0x05
#define PCI_SUBCLASS_BR_NUBUS               0x06
#define PCI_SUBCLASS_BR_CARDBUS             0x07
#define PCI_SUBCLASS_BR_OTHER               0x80


/* PCI device subclasses for class C (serial bus controller)*/

#define PCI_SUBCLASS_SB_IEEE1394            0x00
#define PCI_SUBCLASS_SB_ACCESS              0x01
#define PCI_SUBCLASS_SB_SSA                 0x02
#define PCI_SUBCLASS_SB_USB                 0x03
#define PCI_SUBCLASS_SB_FIBRE_CHANNEL       0x04


/* Bit encodes for PCI_COMMON_CONFIG.u.type0.BaseAddresses */

#define PCI_ADDRESS_IO_SPACE                0x00000001
#define PCI_ADDRESS_MEMORY_TYPE_MASK        0x00000006
#define PCI_ADDRESS_MEMORY_PREFETCHABLE     0x00000008

#define PCI_ADDRESS_IO_ADDRESS_MASK         0xfffffffc
#define PCI_ADDRESS_MEMORY_ADDRESS_MASK     0xfffffff0
#define PCI_ADDRESS_ROM_ADDRESS_MASK        0xfffff800

#define PCI_TYPE_32BIT      0
#define PCI_TYPE_20BIT      2
#define PCI_TYPE_64BIT      4


/* Bit encodes for PCI_COMMON_CONFIG.u.type0.ROMBaseAddresses */

#define PCI_ROMADDRESS_ENABLED              0x00000001



typedef struct _PCI_SLOT_NUMBER
{
  union
    {
      struct
	{
	  ULONG DeviceNumber:5;
	  ULONG FunctionNumber:3;
	  ULONG Reserved:24;
	} bits;
      ULONG AsULONG;
    } u;
} PCI_SLOT_NUMBER, *PPCI_SLOT_NUMBER;

typedef enum _RESOURCE_TRANSLATION_DIRECTION {
    TranslateChildToParent,
    TranslateParentToChild
} RESOURCE_TRANSLATION_DIRECTION;

typedef NTSTATUS STDCALL_FUNC
(*PTRANSLATE_RESOURCE_HANDLER)(IN PVOID Context,
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Source,
    IN RESOURCE_TRANSLATION_DIRECTION Direction,
    IN ULONG AlternativesCount, OPTIONAL
    IN IO_RESOURCE_DESCRIPTOR Alternatives[], OPTIONAL
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR Target
);

typedef NTSTATUS STDCALL_FUNC
(*PTRANSLATE_RESOURCE_REQUIREMENTS_HANDLER)(IN PVOID Context,
    IN PIO_RESOURCE_DESCRIPTOR Source,
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    OUT PULONG TargetCount,
    OUT PIO_RESOURCE_DESCRIPTOR *Target);

typedef struct _TRANSLATOR_INTERFACE {
    USHORT Size;
    USHORT Version;
    PVOID Context;
    PINTERFACE_REFERENCE InterfaceReference;
    PINTERFACE_DEREFERENCE InterfaceDereference;
    PTRANSLATE_RESOURCE_HANDLER TranslateResources;
    PTRANSLATE_RESOURCE_REQUIREMENTS_HANDLER TranslateResourceRequirements;
} TRANSLATOR_INTERFACE, *PTRANSLATOR_INTERFACE;


/* Hal dispatch table */

typedef enum _HAL_QUERY_INFORMATION_CLASS
{
  HalInstalledBusInformation,
  HalProfileSourceInformation,
  HalSystemDockInformation,
  HalPowerInformation,
  HalProcessorSpeedInformation,
  HalCallbackInformation,
  HalMapRegisterInformation,
  HalMcaLogInformation,
  HalFrameBufferCachingInformation,
  HalDisplayBiosInformation
  /* information levels >= 0x8000000 reserved for OEM use */
} HAL_QUERY_INFORMATION_CLASS, *PHAL_QUERY_INFORMATION_CLASS;


typedef enum _HAL_SET_INFORMATION_CLASS
{
  HalProfileSourceInterval,
  HalProfileSourceInterruptHandler,
  HalMcaRegisterDriver
} HAL_SET_INFORMATION_CLASS, *PHAL_SET_INFORMATION_CLASS;


typedef struct _BUS_HANDLER *PBUS_HANDLER;
typedef struct _DEVICE_HANDLER_OBJECT *PDEVICE_HANDLER_OBJECT;


typedef BOOLEAN STDCALL_FUNC
(*PHAL_RESET_DISPLAY_PARAMETERS)(ULONG Columns, ULONG Rows);

#endif /* __USE_W32API */
/* Control codes of HalDeviceControl function */
#define BCTL_EJECT				0x0001
#define BCTL_QUERY_DEVICE_ID			0x0002
#define BCTL_QUERY_DEVICE_UNIQUE_ID		0x0003
#define BCTL_QUERY_DEVICE_CAPABILITIES		0x0004
#define BCTL_QUERY_DEVICE_RESOURCES		0x0005
#define BCTL_QUERY_DEVICE_RESOURCE_REQUIREMENTS	0x0006
#define BCTL_QUERY_EJECT                            0x0007
#define BCTL_SET_LOCK                               0x0008
#define BCTL_SET_POWER                              0x0009
#define BCTL_SET_RESUME                             0x000A
#define BCTL_SET_DEVICE_RESOURCES                   0x000B

/* Defines for BCTL structures */
typedef struct
{
  BOOLEAN PowerSupported;
  BOOLEAN ResumeSupported;
  BOOLEAN LockSupported;
  BOOLEAN EjectSupported;
  BOOLEAN Removable;
} BCTL_DEVICE_CAPABILITIES, *PBCTL_DEVICE_CAPABILITIES;

#ifndef __USE_W32API
typedef struct _DEVICE_CONTROL_CONTEXT
{
  NTSTATUS Status;
  PDEVICE_HANDLER_OBJECT DeviceHandler;
  PDEVICE_OBJECT DeviceObject;
  ULONG ControlCode;
  PVOID Buffer;
  PULONG BufferLength;
  PVOID Context;
} DEVICE_CONTROL_CONTEXT, *PDEVICE_CONTROL_CONTEXT;

typedef struct _PM_DISPATCH_TABLE {
    ULONG   Signature;
    ULONG   Version;
    PVOID   Function[1];
} PM_DISPATCH_TABLE, *PPM_DISPATCH_TABLE;

typedef VOID STDCALL_FUNC
(*PDEVICE_CONTROL_COMPLETION)(IN PDEVICE_CONTROL_CONTEXT ControlContext);

typedef NTSTATUS STDCALL_FUNC
(*pHalDeviceControl)(IN PDEVICE_HANDLER_OBJECT DeviceHandler,
		     IN PDEVICE_OBJECT DeviceObject,
		     IN ULONG ControlCode,
		     IN OUT PVOID Buffer OPTIONAL,
		     IN OUT PULONG BufferLength OPTIONAL,
		     IN PVOID Context,
		     IN PDEVICE_CONTROL_COMPLETION CompletionRoutine);

typedef VOID FASTCALL_FUNC
(*pHalExamineMBR)(IN PDEVICE_OBJECT DeviceObject,
		  IN ULONG SectorSize,
		  IN ULONG MBRTypeIdentifier,
		  OUT PVOID *Buffer);

typedef VOID FASTCALL_FUNC
(*pHalIoAssignDriveLetters)(IN struct _LOADER_PARAMETER_BLOCK *LoaderBlock,
			    IN PSTRING NtDeviceName,
			    OUT PUCHAR NtSystemPath,
			    OUT PSTRING NtSystemPathString);

typedef NTSTATUS FASTCALL_FUNC
(*pHalIoReadPartitionTable)(IN PDEVICE_OBJECT DeviceObject,
			    IN ULONG SectorSize,
			    IN BOOLEAN ReturnRecognizedPartitions,
			    OUT PDRIVE_LAYOUT_INFORMATION *PartitionBuffer);

typedef NTSTATUS FASTCALL_FUNC
(*pHalIoSetPartitionInformation)(IN PDEVICE_OBJECT DeviceObject,
				 IN ULONG SectorSize,
				 IN ULONG PartitionNumber,
				 IN ULONG PartitionType);

typedef NTSTATUS FASTCALL_FUNC
(*pHalIoWritePartitionTable)(IN PDEVICE_OBJECT DeviceObject,
			     IN ULONG SectorSize,
			     IN ULONG SectorsPerTrack,
			     IN ULONG NumberOfHeads,
			     IN PDRIVE_LAYOUT_INFORMATION PartitionBuffer);

typedef PBUS_HANDLER FASTCALL_FUNC
(*pHalHandlerForBus)(IN INTERFACE_TYPE InterfaceType,
		     IN ULONG BusNumber);

typedef VOID FASTCALL_FUNC
(*pHalReferenceBusHandler)(IN PBUS_HANDLER BusHandler);

typedef NTSTATUS STDCALL_FUNC
(*pHalQuerySystemInformation)(IN HAL_QUERY_INFORMATION_CLASS InformationClass,
			      IN ULONG BufferSize,
			      IN OUT PVOID Buffer,
			      OUT PULONG ReturnedLength);


typedef NTSTATUS STDCALL_FUNC
(*pHalSetSystemInformation)(IN HAL_SET_INFORMATION_CLASS InformationClass,
			    IN ULONG BufferSize,
			    IN PVOID Buffer);


typedef NTSTATUS STDCALL_FUNC
(*pHalQueryBusSlots)(IN PBUS_HANDLER BusHandler,
		     IN ULONG BufferSize,
		     OUT PULONG SlotNumbers,
		     OUT PULONG ReturnedLength);

typedef NTSTATUS STDCALL_FUNC
(*pHalInitPnpDriver)(VOID);
 
typedef NTSTATUS STDCALL_FUNC
(*pHalInitPowerManagement)(IN PPM_DISPATCH_TABLE  PmDriverDispatchTable,
    			   OUT PPM_DISPATCH_TABLE *PmHalDispatchTable);

typedef struct _DMA_ADAPTER * STDCALL_FUNC
(*pHalGetDmaAdapter)(IN PVOID Context,
    		     IN struct _DEVICE_DESCRIPTION *DeviceDescriptor,
    		     OUT PULONG NumberOfMapRegisters);
		     
typedef NTSTATUS STDCALL_FUNC
(*pHalGetInterruptTranslator)(IN INTERFACE_TYPE ParentInterfaceType,
			      IN ULONG ParentBusNumber,
			      IN INTERFACE_TYPE BridgeInterfaceType,
			      IN USHORT Size,
			      IN USHORT Version,
			      OUT PTRANSLATOR_INTERFACE Translator,
			      OUT PULONG BridgeBusNumber);

typedef NTSTATUS STDCALL_FUNC
(*pHalStartMirroring)(VOID);

typedef NTSTATUS STDCALL_FUNC
(*pHalEndMirroring)(IN ULONG PassNumber);

typedef NTSTATUS STDCALL_FUNC
(*pHalMirrorPhysicalMemory)(IN PHYSICAL_ADDRESS PhysicalAddress,
    IN LARGE_INTEGER NumberOfBytes);

typedef NTSTATUS STDCALL_FUNC
(*pHalMirrorVerify)(IN PHYSICAL_ADDRESS PhysicalAddress,
    IN LARGE_INTEGER NumberOfBytes);

typedef VOID STDCALL_FUNC
(*pHalEndOfBoot)(VOID);
        

typedef struct {
    ULONG                           Version;
    pHalQuerySystemInformation      HalQuerySystemInformation;
    pHalSetSystemInformation        HalSetSystemInformation;
    pHalQueryBusSlots               HalQueryBusSlots;
    ULONG                           Spare1;
    pHalExamineMBR                  HalExamineMBR;
    pHalIoAssignDriveLetters        HalIoAssignDriveLetters;
    pHalIoReadPartitionTable        HalIoReadPartitionTable;
    pHalIoSetPartitionInformation   HalIoSetPartitionInformation;
    pHalIoWritePartitionTable       HalIoWritePartitionTable;

    pHalHandlerForBus               HalReferenceHandlerForBus;
    pHalReferenceBusHandler         HalReferenceBusHandler;
    pHalReferenceBusHandler         HalDereferenceBusHandler;

    pHalInitPnpDriver               HalInitPnpDriver;
    pHalInitPowerManagement         HalInitPowerManagement;

    pHalGetDmaAdapter               HalGetDmaAdapter;
    pHalGetInterruptTranslator      HalGetInterruptTranslator;

    pHalStartMirroring              HalStartMirroring;
    pHalEndMirroring                HalEndMirroring;
    pHalMirrorPhysicalMemory        HalMirrorPhysicalMemory;
    pHalEndOfBoot                   HalEndOfBoot;
    pHalMirrorVerify                HalMirrorVerify;

} HAL_DISPATCH, *PHAL_DISPATCH;

#ifdef __NTOSKRNL__
extern HAL_DISPATCH EXPORTED HalDispatchTable;
#else
extern PHAL_DISPATCH IMPORTED HalDispatchTable;
#endif

#endif /* !__USE_W32API */

#ifndef __USE_W32API
#ifdef __NTOSKRNL__
#define HALDISPATCH (&HalDispatchTable)
#else
#define HALDISPATCH ((PHAL_DISPATCH)&HalDispatchTable)
#endif


#define HAL_DISPATCH_VERSION		3
#define HalDispatchTableVersion		HALDISPATCH->Version
#define HalQuerySystemInformation	HALDISPATCH->HalQuerySystemInformation
#define HalSetSystemInformation		HALDISPATCH->HalSetSystemInformation
#define HalQueryBusSlots		HALDISPATCH->HalQueryBusSlots
#define HalDeviceControl		HALDISPATCH->HalDeviceControl
#define HalExamineMBR			HALDISPATCH->HalExamineMBR
#define HalIoAssignDriveLetters		HALDISPATCH->HalIoAssignDriveLetters
#define HalIoReadPartitionTable		HALDISPATCH->HalIoReadPartitionTable
#define HalIoSetPartitionInformation	HALDISPATCH->HalIoSetPartitionInformation
#define HalIoWritePartitionTable	HALDISPATCH->HalIoWritePartitionTable
#define HalReferenceHandlerForBus	HALDISPATCH->HalReferenceHandlerForBus
#define HalReferenceBusHandler		HALDISPATCH->HalReferenceBusHandler
#define HalDereferenceBusHandler	HALDISPATCH->HalDereferenceBusHandler


#endif /* !__USE_W32API */

/* Hal private dispatch table */

typedef struct _HAL_PRIVATE_DISPATCH
{
  ULONG Version;
} HAL_PRIVATE_DISPATCH, *PHAL_PRIVATE_DISPATCH;

#ifdef __NTOSKRNL__
extern HAL_PRIVATE_DISPATCH EXPORTED HalPrivateDispatchTable;
#else
extern PHAL_PRIVATE_DISPATCH IMPORTED HalPrivateDispatchTable;
#endif

#define HAL_PRIVATE_DISPATCH_VERSION	1



/*
 * Kernel debugger section
 */

typedef struct _KD_PORT_INFORMATION
{
  ULONG ComPort;
  ULONG BaudRate;
  ULONG BaseAddress;
} KD_PORT_INFORMATION, *PKD_PORT_INFORMATION;


#ifdef __NTHAL__
extern ULONG EXPORTED KdComPortInUse;
#else
extern ULONG IMPORTED KdComPortInUse;
#endif

#endif /* __INCLUDE_DDK_HALTYPES_H */

/* EOF */
