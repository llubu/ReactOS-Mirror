/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/freeldr/include/arch/arm/hardware.h
 * PURPOSE:         Header for ARC definitions (to be cleaned up)
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

#ifndef _ARM_HARDWARE_
#define __ARM_HARDWARE_

#ifndef __REGISTRY_H
#include "../../reactos/registry.h"
#endif

//
// Marvell Feroceon-based SoC:
// Buffalo Linkstation, KuroBox Pro, D-Link DS323 and others
//
#define MACH_TYPE_FEROCEON     526

//
// ARM Versatile PB:
// qemu-system-arm -M versatilepb, RealView Development Boards and others
//
#define MACH_TYPE_VERSATILE_PB 387

//
// TI Beagle Board, OMAP3530 SoC
// qemu-system-arm -M beagle, Beagle Board
//
#define MACH_TYPE_OMAP3_BEAGLE 1546

//
// Compatible boot-loaders should return us this information
//
#define ARM_BOARD_CONFIGURATION_MAJOR_VERSION 1
#define ARM_BOARD_CONFIGURATION_MINOR_VERSION 1
typedef struct _ARM_BOARD_CONFIGURATION_BLOCK
{
    ULONG MajorVersion;
    ULONG MinorVersion;
    ULONG BoardType;
    ULONG ClockRate;
    ULONG TimerRegisterBase;
    ULONG UartRegisterBase;
    ULONG MemoryMapEntryCount;
    PBIOS_MEMORY_MAP MemoryMap;
    CHAR CommandLine[256];
} ARM_BOARD_CONFIGURATION_BLOCK, *PARM_BOARD_CONFIGURATION_BLOCK;

//
// Static heap for ARC Hardware Component Tree
// 16KB oughta be enough for anyone.
//
#define HW_MAX_ARC_HEAP_SIZE 16 * 1024

//
// ARC Component Configuration Routines
//
VOID
NTAPI
FldrCreateSystemKey(
    OUT PCONFIGURATION_COMPONENT_DATA *SystemKey
);

VOID
NTAPI
FldrCreateComponentKey(
    IN PCONFIGURATION_COMPONENT_DATA SystemKey,
    IN CONFIGURATION_CLASS Class,
    IN CONFIGURATION_TYPE Type,
    IN IDENTIFIER_FLAG Flags,
    IN ULONG Key,
    IN ULONG Affinity,
    IN PCHAR IdentifierString,
    IN PCM_PARTIAL_RESOURCE_LIST ResourceList,
    IN ULONG Size,
    OUT PCONFIGURATION_COMPONENT_DATA *ComponentKey
);

VOID
ArmFeroSerialInit(IN ULONG Baudrate);

VOID
ArmFeroPutChar(IN INT Char);

INT
ArmFeroGetCh(VOID);

BOOLEAN
ArmFeroKbHit(VOID);

VOID
ArmOmap3SerialInit(IN ULONG Baudrate);

VOID
ArmOmap3PutChar(IN INT Char);

INT
ArmOmap3GetCh(VOID);

BOOLEAN
ArmOmap3KbHit(VOID);

VOID
ArmVersaSerialInit(IN ULONG Baudrate);

VOID
ArmVersaPutChar(IN INT Char);

INT
ArmVersaGetCh(VOID);

BOOLEAN
ArmVersaKbHit(VOID);

extern PARM_BOARD_CONFIGURATION_BLOCK ArmBoardBlock;

#endif
