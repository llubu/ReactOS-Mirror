/*
 * COPYRIGHT:                See COPYING in the top level directory
 * PROJECT:                  ReactOS kernel
 * FILE:                     include/internal/hal/ddk.h
 * PURPOSE:                  HAL provided defintions for device drivers
 * PROGRAMMER:               David Welch (welch@mcmail.com)
 * REVISION HISTORY:
 *              23/06/98:   Taken from linux system.h
 */


#ifndef __INCLUDE_INTERNAL_HAL_DDK_H
#define __INCLUDE_INTERNAL_HAL_DDK_H

/* HalReturnToFirmware */
#define FIRMWARE_HALT   1
#define FIRMWARE_REBOOT 3


enum
{
   DEVICE_DESCRIPTION_VERSION,
   DEVICE_DESCRIPTION_VERSION1,
};

typedef ULONG DMA_WIDTH;
typedef ULONG DMA_SPEED;
typedef ULONG BUS_DATA_TYPE;

/*
 * PURPOSE: Types for HalGetBusData
 */
enum
{
   Cmos,
   EisaConfiguration,
   Pos,
   PCIConfiguration,
   MaximumBusDataType,
};

typedef struct _DEVICE_DESCRIPTION
{
   ULONG Version;
   BOOLEAN Master;
   BOOLEAN ScatterGather;
   BOOLEAN DemandMode;
   BOOLEAN AutoInitialize;
   BOOLEAN Dma32BitAddress;
   BOOLEAN IgnoreCount;
   BOOLEAN Reserved1;
   BOOLEAN Reserved2;
   ULONG BusNumber;
   ULONG DmaChannel;
   INTERFACE_TYPE InterfaceType;
   DMA_WIDTH DmaWidth;
   DMA_SPEED DmaSpeed;
   ULONG MaximumLength;
   ULONG DmaPort;
} DEVICE_DESCRIPTION, *PDEVICE_DESCRIPTION;

typedef BOOLEAN STDCALL (*RESET_DISPLAY_ROUTINE)(ULONG SizeX, ULONG SizeY);

VOID HalAcquireDisplayOwnership(RESET_DISPLAY_ROUTINE ResetRoutine);
PVOID HalAllocateCommonBuffer(PADAPTER_OBJECT AdapterObject,
			      ULONG Length,
			      PPHYSICAL_ADDRESS LogicalAddress,
			      BOOLEAN CacheEnabled);
NTSTATUS HalAssignSlotResources(PUNICODE_STRING RegistryPath,
				PUNICODE_STRING DriverClassName,
				PDRIVER_OBJECT DriverObject,
				PDEVICE_OBJECT DeviceObject,
				INTERFACE_TYPE BusType,
				ULONG BusNumber,
				ULONG SlotNumber,
				PCM_RESOURCE_LIST* AllocatedResources);
VOID HalExamineMBR(PDEVICE_OBJECT DeviceObject,
		   ULONG SectorSize,
		   ULONG MBRTypeIdentifier,
		   PVOID Buffer);
VOID HalFreeCommonBuffer(PADAPTER_OBJECT AdapterObject,
			 ULONG Length,
			 PHYSICAL_ADDRESS LogicalAddress,
			 PVOID VirtualAddress,
			 BOOLEAN CacheEnabled);
PADAPTER_OBJECT HalGetAdapter(PDEVICE_DESCRIPTION DeviceDescription,
			      PULONG NumberOfMapRegisters);
ULONG HalGetBusData(BUS_DATA_TYPE BusDataType,
		    ULONG BusNumber,
		    ULONG SlotNumber,
		    PVOID Buffer,
		    ULONG Length);
ULONG HalGetBusDataByOffset(BUS_DATA_TYPE BusDataType,
			    ULONG BusNumber,
			    ULONG SlotNumber,
			    PVOID Buffer,
			    ULONG Offset,
			    ULONG Length);
ULONG HalGetDmaAlignmentRequirement(VOID);
ULONG HalGetInterruptVector(INTERFACE_TYPE InterfaceType,
			    ULONG BusNumber,
			    ULONG BusInterruptLevel,
			    ULONG BusInterruptVector,
			    PKIRQL Irql,
			    PKAFFINITY Affinity);
BOOLEAN HalMakeBeep(ULONG Frequency);
VOID HalQueryDisplayParameters(PULONG DispSizeX,
			       PULONG DispSizeY,
			       PULONG CursorPosX,
			       PULONG CursorPosY);
VOID HalQueryRealTimeClock(PTIME_FIELDS pTime);
VOID HalQuerySystemInformation(VOID);
ULONG HalReadDmaCounter(PADAPTER_OBJECT AdapterObject);
VOID HalReturnToFirmware(ULONG Action);
ULONG HalSetBusData(BUS_DATA_TYPE BusDataType,
		    ULONG BusNumber,
		    ULONG SlotNumber,
		    PVOID Buffer,
		    ULONG Length);
ULONG HalSetBusDataByOffset(BUS_DATA_TYPE BusDataType,
			    ULONG BusNumber,
			    ULONG SlotNumber,
			    PVOID Buffer,
			    ULONG Offset,
			    ULONG Length);
VOID HalSetDisplayParameters(ULONG CursorPosX,
			     ULONG CursorPosY);
BOOLEAN HalTranslateBusAddress(INTERFACE_TYPE InterfaceType,
			       ULONG BusNumber,
			       PHYSICAL_ADDRESS BusAddress,
			       PULONG AddressSpace,
			       PPHYSICAL_ADDRESS TranslatedAddress);

#endif /* __INCLUDE_INTERNAL_HAL_DDK_H */
