/*
 *  FreeLoader
 *
 *  Copyright (C) 2003, 2004  Eric Kohl
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
#include <debug.h>
#include <disk.h>
#include <mm.h>
#include <machine.h>
#include <portio.h>
#include <video.h>

#include "../../reactos/registry.h"
#include "hardware.h"


#define MILLISEC     (10)
#define PRECISION    (8)

#define HZ (100)
#define CLOCK_TICK_RATE (1193182)
#define LATCH (CLOCK_TICK_RATE / HZ)


/* No Mouse */
#define MOUSE_TYPE_NONE			0
/* Microsoft Mouse with 2 buttons */
#define MOUSE_TYPE_MICROSOFT	1
/* Logitech Mouse with 3 buttons */
#define MOUSE_TYPE_LOGITECH		2
/* Microsoft Wheel Mouse (aka Z Mouse) */
#define MOUSE_TYPE_WHEELZ		3
/* Mouse Systems Mouse */
#define MOUSE_TYPE_MOUSESYSTEMS	4


/* PS2 stuff */

/* Controller registers. */
#define CONTROLLER_REGISTER_STATUS                      0x64
#define CONTROLLER_REGISTER_CONTROL                     0x64
#define CONTROLLER_REGISTER_DATA                        0x60

/* Controller commands. */
#define CONTROLLER_COMMAND_READ_MODE                    0x20
#define CONTROLLER_COMMAND_WRITE_MODE                   0x60
#define CONTROLLER_COMMAND_GET_VERSION                  0xA1
#define CONTROLLER_COMMAND_MOUSE_DISABLE                0xA7
#define CONTROLLER_COMMAND_MOUSE_ENABLE                 0xA8
#define CONTROLLER_COMMAND_TEST_MOUSE                   0xA9
#define CONTROLLER_COMMAND_SELF_TEST                    0xAA
#define CONTROLLER_COMMAND_KEYBOARD_TEST                0xAB
#define CONTROLLER_COMMAND_KEYBOARD_DISABLE             0xAD
#define CONTROLLER_COMMAND_KEYBOARD_ENABLE              0xAE
#define CONTROLLER_COMMAND_WRITE_MOUSE_OUTPUT_BUFFER    0xD3
#define CONTROLLER_COMMAND_WRITE_MOUSE                  0xD4

/* Controller status */
#define CONTROLLER_STATUS_OUTPUT_BUFFER_FULL            0x01
#define CONTROLLER_STATUS_INPUT_BUFFER_FULL             0x02
#define CONTROLLER_STATUS_SELF_TEST                     0x04
#define CONTROLLER_STATUS_COMMAND                       0x08
#define CONTROLLER_STATUS_UNLOCKED                      0x10
#define CONTROLLER_STATUS_MOUSE_OUTPUT_BUFFER_FULL      0x20
#define CONTROLLER_STATUS_GENERAL_TIMEOUT               0x40
#define CONTROLLER_STATUS_PARITY_ERROR                  0x80
#define AUX_STATUS_OUTPUT_BUFFER_FULL                   (CONTROLLER_STATUS_OUTPUT_BUFFER_FULL | \
                                                         CONTROLLER_STATUS_MOUSE_OUTPUT_BUFFER_FULL)

/* Timeout in ms for sending to keyboard controller. */
#define CONTROLLER_TIMEOUT                              250


typedef struct _CM_DISK_GEOMETRY_DEVICE_DATA
{
  ULONG BytesPerSector;
  ULONG NumberOfCylinders;
  ULONG SectorsPerTrack;
  ULONG NumberOfHeads;
} CM_DISK_GEOMETRY_DEVICE_DATA, *PCM_DISK_GEOMETRY_DEVICE_DATA;


typedef struct _CM_PNP_BIOS_DEVICE_NODE
{
  USHORT Size;
  UCHAR  Node;
  ULONG ProductId;
  UCHAR  DeviceType[3];
  USHORT DeviceAttributes;
} __attribute__((packed)) CM_PNP_BIOS_DEVICE_NODE, *PCM_PNP_BIOS_DEVICE_NODE;


typedef struct _CM_PNP_BIOS_INSTALLATION_CHECK
{
  UCHAR  Signature[4];
  UCHAR  Revision;
  UCHAR  Length;
  USHORT ControlField;
  UCHAR  Checksum;
  ULONG EventFlagAddress;
  USHORT RealModeEntryOffset;
  USHORT RealModeEntrySegment;
  USHORT ProtectedModeEntryOffset;
  ULONG ProtectedModeCodeBaseAddress;
  ULONG OemDeviceId;
  USHORT RealModeDataBaseAddress;
  ULONG ProtectedModeDataBaseAddress;
} __attribute__((packed)) CM_PNP_BIOS_INSTALLATION_CHECK, *PCM_PNP_BIOS_INSTALLATION_CHECK;


static char Hex[] = "0123456789ABCDEF";
static unsigned int delay_count = 1;


/* FUNCTIONS ****************************************************************/


static VOID
__StallExecutionProcessor(ULONG Loops)
{
  register unsigned int i;
  for (i = 0; i < Loops; i++);
}


VOID StallExecutionProcessor(ULONG Microseconds)
{
  ULONGLONG LoopCount = ((ULONGLONG)delay_count * (ULONGLONG)Microseconds) / 1000ULL;
  __StallExecutionProcessor((ULONG)LoopCount);
}


static ULONG
Read8254Timer(VOID)
{
  ULONG Count;

  WRITE_PORT_UCHAR((PUCHAR)0x43, 0x00);
  Count = READ_PORT_UCHAR((PUCHAR)0x40);
  Count |= READ_PORT_UCHAR((PUCHAR)0x40) << 8;

  return Count;
}


static VOID
WaitFor8254Wraparound(VOID)
{
  ULONG CurCount;
  ULONG PrevCount = ~0;
  LONG Delta;

  CurCount = Read8254Timer();

  do
    {
      PrevCount = CurCount;
      CurCount = Read8254Timer();
      Delta = CurCount - PrevCount;

      /*
       * This limit for delta seems arbitrary, but it isn't, it's
       * slightly above the level of error a buggy Mercury/Neptune
       * chipset timer can cause.
       */
    }
  while (Delta < 300);
}


VOID
HalpCalibrateStallExecution(VOID)
{
  ULONG i;
  ULONG calib_bit;
  ULONG CurCount;

  /* Initialise timer interrupt with MILLISECOND ms interval        */
  WRITE_PORT_UCHAR((PUCHAR)0x43, 0x34);  /* binary, mode 2, LSB/MSB, ch 0 */
  WRITE_PORT_UCHAR((PUCHAR)0x40, LATCH & 0xff); /* LSB */
  WRITE_PORT_UCHAR((PUCHAR)0x40, LATCH >> 8); /* MSB */
  
  /* Stage 1:  Coarse calibration                                   */
  
  WaitFor8254Wraparound();
  
  delay_count = 1;
  
  do {
    delay_count <<= 1;                  /* Next delay count to try */

    WaitFor8254Wraparound();
  
    __StallExecutionProcessor(delay_count);      /* Do the delay */
  
    CurCount = Read8254Timer();
  } while (CurCount > LATCH / 2);
  
  delay_count >>= 1;              /* Get bottom value for delay     */
  
  /* Stage 2:  Fine calibration                                     */
  
  calib_bit = delay_count;        /* Which bit are we going to test */
  
  for(i=0;i<PRECISION;i++) {
    calib_bit >>= 1;             /* Next bit to calibrate          */
    if(!calib_bit) break;        /* If we have done all bits, stop */
  
    delay_count |= calib_bit;        /* Set the bit in delay_count */
  
    WaitFor8254Wraparound();
  
    __StallExecutionProcessor(delay_count);      /* Do the delay */
  
    CurCount = Read8254Timer();
    if (CurCount <= LATCH / 2)   /* If a tick has passed, turn the */
      delay_count &= ~calib_bit; /* calibrated bit back off        */
  }
  
  /* We're finished:  Do the finishing touches                      */
  delay_count /= (MILLISEC / 2);   /* Calculate delay_count for 1ms */
}


VOID
SetComponentInformation(FRLDRHKEY ComponentKey,
			ULONG Flags,
			ULONG Key,
			ULONG Affinity)
{
  CM_COMPONENT_INFORMATION CompInfo;
  LONG Error;

  CompInfo.Flags = Flags;
  CompInfo.Version = 0;
  CompInfo.Key = Key;
  CompInfo.Affinity = Affinity;

  /* Set 'Component Information' value */
  Error = RegSetValue(ComponentKey,
		      "Component Information",
		      REG_BINARY,
		      (PUCHAR)&CompInfo,
		      sizeof(CM_COMPONENT_INFORMATION));
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_HWDETECT, "RegSetValue() failed (Error %u)\n", (int)Error));
    }
}


static VOID
DetectPnpBios(FRLDRHKEY SystemKey, ULONG *BusNumber)
{
  PCM_FULL_RESOURCE_DESCRIPTOR FullResourceDescriptor;
  PCM_PNP_BIOS_DEVICE_NODE DeviceNode;
  PCM_PNP_BIOS_INSTALLATION_CHECK InstData;
  char Buffer[80];
  FRLDRHKEY BusKey;
  ULONG x;
  ULONG NodeSize = 0;
  ULONG NodeCount = 0;
  UCHAR NodeNumber;
  ULONG FoundNodeCount;
  int i;
  ULONG PnpBufferSize;
  ULONG Size;
  char *Ptr;
  LONG Error;

  InstData = (PCM_PNP_BIOS_INSTALLATION_CHECK)PnpBiosSupported();
  if (InstData == NULL || strncmp(InstData->Signature, "$PnP", 4))
    {
      DbgPrint((DPRINT_HWDETECT, "PnP-BIOS not supported\n"));
      return;
    }
  DbgPrint((DPRINT_HWDETECT, "Signature '%c%c%c%c'\n",
	    InstData->Signature[0], InstData->Signature[1],
	    InstData->Signature[2], InstData->Signature[3]));


  x = PnpBiosGetDeviceNodeCount(&NodeSize, &NodeCount);
  NodeCount &= 0xFF; // needed since some fscked up BIOSes return
                     // wrong info (e.g. Mac Virtual PC)
                     // e.g. look: http://my.execpc.com/~geezer/osd/pnp/pnp16.c
  if (x != 0 || NodeSize == 0 || NodeCount == 0)
    {
      DbgPrint((DPRINT_HWDETECT, "PnP-BIOS failed to enumerate device nodes\n"));
      return;
    }
  DbgPrint((DPRINT_HWDETECT, "PnP-BIOS supported\n"));
  DbgPrint((DPRINT_HWDETECT, "MaxNodeSize %u  NodeCount %u\n", NodeSize, NodeCount));
  DbgPrint((DPRINT_HWDETECT, "Estimated buffer size %u\n", NodeSize * NodeCount));

  /* Create new bus key */
  sprintf(Buffer,
	  "MultifunctionAdapter\\%u", *BusNumber);
  Error = RegCreateKey(SystemKey,
		       Buffer,
		       &BusKey);
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_HWDETECT, "RegCreateKey() failed (Error %u)\n", (int)Error));
      return;
    }

  /* Increment bus number */
  (*BusNumber)++;

  /* Set 'Component Information' value similar to my NT4 box */
  SetComponentInformation(BusKey,
                          0x0,
                          0x0,
                          0xFFFFFFFF);

  /* Set 'Identifier' value */
  Error = RegSetValue(BusKey,
		      "Identifier",
		      REG_SZ,
		      (PUCHAR)"PNP BIOS",
		      9);
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_HWDETECT, "RegSetValue() failed (Error %u)\n", (int)Error));
      return;
    }

  /* Set 'Configuration Data' value */
  Size = sizeof(CM_FULL_RESOURCE_DESCRIPTOR) + (NodeSize * NodeCount);
  FullResourceDescriptor = MmAllocateMemory(Size);
  if (FullResourceDescriptor == NULL)
    {
      DbgPrint((DPRINT_HWDETECT,
		"Failed to allocate resource descriptor\n"));
      return;
    }
  memset(FullResourceDescriptor, 0, Size);

  /* Initialize resource descriptor */
  FullResourceDescriptor->InterfaceType = Internal;
  FullResourceDescriptor->BusNumber = 0;
  FullResourceDescriptor->PartialResourceList.Count = 1;
  FullResourceDescriptor->PartialResourceList.PartialDescriptors[0].Type =
    CmResourceTypeDeviceSpecific;
  FullResourceDescriptor->PartialResourceList.PartialDescriptors[0].ShareDisposition =
    CmResourceShareUndetermined;

  Ptr = (char *)(((PVOID)&FullResourceDescriptor->PartialResourceList.PartialDescriptors[0]) +
		 sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));

  /* Set instalation check data */
  memcpy (Ptr, InstData, sizeof(CM_PNP_BIOS_INSTALLATION_CHECK));
  Ptr += sizeof(CM_PNP_BIOS_INSTALLATION_CHECK);

  /* Copy device nodes */
  FoundNodeCount = 0;
  PnpBufferSize = sizeof(CM_PNP_BIOS_INSTALLATION_CHECK);
  for (i = 0; i < 0xFF; i++)
    {
      NodeNumber = (UCHAR)i;

      x = PnpBiosGetDeviceNode(&NodeNumber, (PVOID)DISKREADBUFFER);
      if (x == 0)
	{
	  DeviceNode = (PCM_PNP_BIOS_DEVICE_NODE)DISKREADBUFFER;

	  DbgPrint((DPRINT_HWDETECT,
		    "Node: %u  Size %u (0x%x)\n",
		    DeviceNode->Node,
		    DeviceNode->Size,
		    DeviceNode->Size));

	  memcpy (Ptr,
		  DeviceNode,
		  DeviceNode->Size);

	  Ptr += DeviceNode->Size;
	  PnpBufferSize += DeviceNode->Size;

	  FoundNodeCount++;
	  if (FoundNodeCount >= NodeCount)
	    break;
	}
    }

  /* Set real data size */
  FullResourceDescriptor->PartialResourceList.PartialDescriptors[0].u.DeviceSpecificData.DataSize =
    PnpBufferSize;
  Size = sizeof(CM_FULL_RESOURCE_DESCRIPTOR) + PnpBufferSize;

  DbgPrint((DPRINT_HWDETECT, "Real buffer size: %u\n", PnpBufferSize));
  DbgPrint((DPRINT_HWDETECT, "Resource size: %u\n", Size));

  /* Set 'Configuration Data' value */
  Error = RegSetValue(BusKey,
		      "Configuration Data",
		      REG_FULL_RESOURCE_DESCRIPTOR,
		      (PUCHAR) FullResourceDescriptor,
		      Size);
  MmFreeMemory(FullResourceDescriptor);
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_HWDETECT,
		"RegSetValue(Configuration Data) failed (Error %u)\n",
		(int)Error));
    }
}



static VOID
SetHarddiskConfigurationData(FRLDRHKEY DiskKey,
			     ULONG DriveNumber)
{
  PCM_FULL_RESOURCE_DESCRIPTOR FullResourceDescriptor;
  PCM_DISK_GEOMETRY_DEVICE_DATA DiskGeometry;
  EXTENDED_GEOMETRY ExtGeometry;
  GEOMETRY Geometry;
  ULONG Size;
  LONG Error;

  /* Set 'Configuration Data' value */
  Size = sizeof(CM_FULL_RESOURCE_DESCRIPTOR) +
	 sizeof(CM_DISK_GEOMETRY_DEVICE_DATA);
  FullResourceDescriptor = MmAllocateMemory(Size);
  if (FullResourceDescriptor == NULL)
    {
      DbgPrint((DPRINT_HWDETECT,
		"Failed to allocate a full resource descriptor\n"));
      return;
    }

  memset(FullResourceDescriptor, 0, Size);
  FullResourceDescriptor->InterfaceType = InterfaceTypeUndefined;
  FullResourceDescriptor->BusNumber = 0;
  FullResourceDescriptor->PartialResourceList.Count = 1;
  FullResourceDescriptor->PartialResourceList.PartialDescriptors[0].Type =
    CmResourceTypeDeviceSpecific;
//  FullResourceDescriptor->PartialResourceList.PartialDescriptors[0].ShareDisposition =
//  FullResourceDescriptor->PartialResourceList.PartialDescriptors[0].Flags =
  FullResourceDescriptor->PartialResourceList.PartialDescriptors[0].u.DeviceSpecificData.DataSize =
    sizeof(CM_DISK_GEOMETRY_DEVICE_DATA);

  /* Get pointer to geometry data */
  DiskGeometry = ((PVOID)FullResourceDescriptor) + sizeof(CM_FULL_RESOURCE_DESCRIPTOR);

  /* Get the disk geometry */
  ExtGeometry.Size = sizeof(EXTENDED_GEOMETRY);
  if (DiskGetExtendedDriveParameters(DriveNumber, &ExtGeometry, ExtGeometry.Size))
    {
      DiskGeometry->BytesPerSector = ExtGeometry.BytesPerSector;
      DiskGeometry->NumberOfCylinders = ExtGeometry.Cylinders;
      DiskGeometry->SectorsPerTrack = ExtGeometry.SectorsPerTrack;
      DiskGeometry->NumberOfHeads = ExtGeometry.Heads;
    }
  else if(MachDiskGetDriveGeometry(DriveNumber, &Geometry))
    {
      DiskGeometry->BytesPerSector = Geometry.BytesPerSector;
      DiskGeometry->NumberOfCylinders = Geometry.Cylinders;
      DiskGeometry->SectorsPerTrack = Geometry.Sectors;
      DiskGeometry->NumberOfHeads = Geometry.Heads;
    }
  else
    {
      DbgPrint((DPRINT_HWDETECT, "Reading disk geometry failed\n"));
      MmFreeMemory(FullResourceDescriptor);
      return;
    }
  DbgPrint((DPRINT_HWDETECT,
	   "Disk %x: %u Cylinders  %u Heads  %u Sectors  %u Bytes\n",
	   DriveNumber,
	   DiskGeometry->NumberOfCylinders,
	   DiskGeometry->NumberOfHeads,
	   DiskGeometry->SectorsPerTrack,
	   DiskGeometry->BytesPerSector));

  Error = RegSetValue(DiskKey,
		      "Configuration Data",
		      REG_FULL_RESOURCE_DESCRIPTOR,
		      (PUCHAR) FullResourceDescriptor,
		      Size);
  MmFreeMemory(FullResourceDescriptor);
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_HWDETECT,
		"RegSetValue(Configuration Data) failed (Error %u)\n",
		(int)Error));
    }
}


static VOID
SetHarddiskIdentifier(FRLDRHKEY DiskKey,
		      ULONG DriveNumber)
{
  PMASTER_BOOT_RECORD Mbr;
  ULONG *Buffer;
  ULONG i;
  ULONG Checksum;
  ULONG Signature;
  char Identifier[20];
  LONG Error;

  /* Read the MBR */
  if (!MachDiskReadLogicalSectors(DriveNumber, 0ULL, 1, (PVOID)DISKREADBUFFER))
    {
      DbgPrint((DPRINT_HWDETECT, "Reading MBR failed\n"));
      return;
    }

  Buffer = (ULONG*)DISKREADBUFFER;
  Mbr = (PMASTER_BOOT_RECORD)DISKREADBUFFER;

  Signature =  Mbr->Signature;
  DbgPrint((DPRINT_HWDETECT, "Signature: %x\n", Signature));

  /* Calculate the MBR checksum */
  Checksum = 0;
  for (i = 0; i < 128; i++)
    {
      Checksum += Buffer[i];
    }
  Checksum = ~Checksum + 1;
  DbgPrint((DPRINT_HWDETECT, "Checksum: %x\n", Checksum));

  /* Convert checksum and signature to identifier string */
  Identifier[0] = Hex[(Checksum >> 28) & 0x0F];
  Identifier[1] = Hex[(Checksum >> 24) & 0x0F];
  Identifier[2] = Hex[(Checksum >> 20) & 0x0F];
  Identifier[3] = Hex[(Checksum >> 16) & 0x0F];
  Identifier[4] = Hex[(Checksum >> 12) & 0x0F];
  Identifier[5] = Hex[(Checksum >> 8) & 0x0F];
  Identifier[6] = Hex[(Checksum >> 4) & 0x0F];
  Identifier[7] = Hex[Checksum & 0x0F];
  Identifier[8] = '-';
  Identifier[9] = Hex[(Signature >> 28) & 0x0F];
  Identifier[10] = Hex[(Signature >> 24) & 0x0F];
  Identifier[11] = Hex[(Signature >> 20) & 0x0F];
  Identifier[12] = Hex[(Signature >> 16) & 0x0F];
  Identifier[13] = Hex[(Signature >> 12) & 0x0F];
  Identifier[14] = Hex[(Signature >> 8) & 0x0F];
  Identifier[15] = Hex[(Signature >> 4) & 0x0F];
  Identifier[16] = Hex[Signature & 0x0F];
  Identifier[17] = '-';
  Identifier[18] = 'A';
  Identifier[19] = 0;
  DbgPrint((DPRINT_HWDETECT, "Identifier: %xsn", Identifier));

  /* Set identifier */
  Error = RegSetValue(DiskKey,
		      "Identifier",
		      REG_SZ,
		      (PUCHAR) Identifier,
		      20);
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_HWDETECT,
		"RegSetValue(Identifier) failed (Error %u)\n",
		(int)Error));
    }
}


static VOID
DetectBiosDisks(FRLDRHKEY SystemKey,
		FRLDRHKEY BusKey)
{
  PCM_FULL_RESOURCE_DESCRIPTOR FullResourceDescriptor;
  PCM_INT13_DRIVE_PARAMETER Int13Drives;
  GEOMETRY Geometry;
  CHAR Buffer[80];
  FRLDRHKEY DiskKey;
  ULONG DiskCount;
  ULONG Size;
  ULONG i;
  LONG Error;
  BOOL Changed;

  /* Count the number of visible drives */
  DiskReportError(FALSE);
  DiskCount = 0;

  /* There are some really broken BIOSes out there. There are even BIOSes
   * that happily report success when you ask them to read from non-existent
   * harddisks. So, we set the buffer to known contents first, then try to
   * read. If the BIOS reports success but the buffer contents haven't
   * changed then we fail anyway */
  memset((PVOID) DISKREADBUFFER, 0xcd, 512);
  while (MachDiskReadLogicalSectors(0x80 + DiskCount, 0ULL, 1, (PVOID)DISKREADBUFFER))
    {
      Changed = FALSE;
      for (i = 0; ! Changed && i < 512; i++)
        {
          Changed = ((PUCHAR)DISKREADBUFFER)[i] != 0xcd;
        }
      if (! Changed)
        {
          DbgPrint((DPRINT_HWDETECT, "BIOS reports success for disk %d but data didn't change\n",
                    (int)DiskCount));
          break;
        }
      DiskCount++;
      memset((PVOID) DISKREADBUFFER, 0xcd, 512);
    }
  DiskReportError(TRUE);
  DbgPrint((DPRINT_HWDETECT, "BIOS reports %d harddisk%s\n",
	    (int)DiskCount, (DiskCount == 1) ? "": "s"));

  /* Allocate resource descriptor */
  Size = sizeof(CM_FULL_RESOURCE_DESCRIPTOR) +
	 sizeof(CM_INT13_DRIVE_PARAMETER) * DiskCount;
  FullResourceDescriptor = MmAllocateMemory(Size);
  if (FullResourceDescriptor == NULL)
    {
      DbgPrint((DPRINT_HWDETECT,
		"Failed to allocate resource descriptor\n"));
      return;
    }

  /* Initialize resource descriptor */
  memset(FullResourceDescriptor, 0, Size);
  FullResourceDescriptor->InterfaceType = InterfaceTypeUndefined;
  FullResourceDescriptor->BusNumber = -1;
  FullResourceDescriptor->PartialResourceList.Count = 1;
  FullResourceDescriptor->PartialResourceList.PartialDescriptors[0].Type =
    CmResourceTypeDeviceSpecific;
//  FullResourceDescriptor->PartialResourceList.PartialDescriptors[0].ShareDisposition =
//  FullResourceDescriptor->PartialResourceList.PartialDescriptors[0].Flags =
  FullResourceDescriptor->PartialResourceList.PartialDescriptors[0].u.DeviceSpecificData.DataSize =
    sizeof(CM_INT13_DRIVE_PARAMETER) * DiskCount;

  /* Get harddisk Int13 geometry data */
  Int13Drives = ((PVOID)FullResourceDescriptor) + sizeof(CM_FULL_RESOURCE_DESCRIPTOR);
  for (i = 0; i < DiskCount; i++)
    {
      if (MachDiskGetDriveGeometry(0x80 + i, &Geometry))
	{
	  Int13Drives[i].DriveSelect = 0x80 + i;
	  Int13Drives[i].MaxCylinders = Geometry.Cylinders - 1;
	  Int13Drives[i].SectorsPerTrack = Geometry.Sectors;
	  Int13Drives[i].MaxHeads = Geometry.Heads - 1;
	  Int13Drives[i].NumberDrives = DiskCount;

	  DbgPrint((DPRINT_HWDETECT,
		    "Disk %x: %u Cylinders  %u Heads  %u Sectors  %u Bytes\n",
		    0x80 + i,
		    Geometry.Cylinders - 1,
		    Geometry.Heads -1,
		    Geometry.Sectors,
		    Geometry.BytesPerSector));
	}
    }

  /* Set 'Configuration Data' value */
  Error = RegSetValue(SystemKey,
		      "Configuration Data",
		      REG_FULL_RESOURCE_DESCRIPTOR,
		      (PUCHAR) FullResourceDescriptor,
		      Size);
  MmFreeMemory(FullResourceDescriptor);
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_HWDETECT,
		"RegSetValue(Configuration Data) failed (Error %u)\n",
		(int)Error));
      return;
    }

  /* Create and fill subkey for each harddisk */
  for (i = 0; i < DiskCount; i++)
    {
      /* Create disk key */
      sprintf (Buffer,
	       "DiskController\\0\\DiskPeripheral\\%u",
	       i);

      Error = RegCreateKey(BusKey,
			   Buffer,
			   &DiskKey);
      if (Error != ERROR_SUCCESS)
	{
	  DbgPrint((DPRINT_HWDETECT, "Failed to create drive key\n"));
	  continue;
	}
      DbgPrint((DPRINT_HWDETECT, "Created key: %s\n", Buffer));

      /* Set disk values */
      SetHarddiskConfigurationData(DiskKey, 0x80 + i);
      SetHarddiskIdentifier(DiskKey, 0x80 + i);
    }
}


static ULONG
GetFloppyCount(VOID)
{
  UCHAR Data;

  WRITE_PORT_UCHAR((PUCHAR)0x70, 0x10);
  Data = READ_PORT_UCHAR((PUCHAR)0x71);

  return ((Data & 0xF0) ? 1 : 0) + ((Data & 0x0F) ? 1 : 0);
}


static UCHAR
GetFloppyType(UCHAR DriveNumber)
{
  UCHAR Data;

  WRITE_PORT_UCHAR((PUCHAR)0x70, 0x10);
  Data = READ_PORT_UCHAR((PUCHAR)0x71);

  if (DriveNumber == 0)
    return Data >> 4;
  else if (DriveNumber == 1)
    return Data & 0x0F;

  return 0;
}


static PVOID
GetInt1eTable(VOID)
{
  PUSHORT SegPtr = (PUSHORT)0x7A;
  PUSHORT OfsPtr = (PUSHORT)0x78;

  return (PVOID)(((ULONG)(*SegPtr)) << 4) + (ULONG)(*OfsPtr);
}


static VOID
DetectBiosFloppyPeripheral(FRLDRHKEY ControllerKey)
{
  PCM_FULL_RESOURCE_DESCRIPTOR FullResourceDescriptor;
  PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;
  PCM_FLOPPY_DEVICE_DATA FloppyData;
  char KeyName[32];
  char Identifier[20];
  FRLDRHKEY PeripheralKey;
  ULONG Size;
  LONG Error;
  ULONG FloppyNumber;
  UCHAR FloppyType;
  ULONG MaxDensity[6] = {0, 360, 1200, 720, 1440, 2880};
  PUCHAR Ptr;

  for (FloppyNumber = 0; FloppyNumber < 2; FloppyNumber++)
  {
    FloppyType = GetFloppyType(FloppyNumber);

    if ((FloppyType > 5) || (FloppyType == 0))
      continue;

    DiskResetController(FloppyNumber);

    Ptr = GetInt1eTable();

    sprintf(KeyName, "FloppyDiskPeripheral\\%u", FloppyNumber);

    Error = RegCreateKey(ControllerKey,
			 "FloppyDiskPeripheral\\0",
			 &PeripheralKey);
    if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_HWDETECT, "Failed to create peripheral key\n"));
      return;
    }

    DbgPrint((DPRINT_HWDETECT, "Created key: %s\n", KeyName));

    /* Set 'ComponentInformation' value */
    SetComponentInformation(PeripheralKey,
			    0x0,
			    FloppyNumber,
			    0xFFFFFFFF);

    Size = sizeof(CM_FULL_RESOURCE_DESCRIPTOR) +
	   sizeof(CM_FLOPPY_DEVICE_DATA);
    FullResourceDescriptor = MmAllocateMemory(Size);
    if (FullResourceDescriptor == NULL)
    {
      DbgPrint((DPRINT_HWDETECT,
		"Failed to allocate resource descriptor\n"));
      return;
    }

    memset(FullResourceDescriptor, 0, Size);
    FullResourceDescriptor->InterfaceType = Isa;
    FullResourceDescriptor->BusNumber = 0;
    FullResourceDescriptor->PartialResourceList.Count = 1;

    PartialDescriptor = &FullResourceDescriptor->PartialResourceList.PartialDescriptors[0];
    PartialDescriptor->Type = CmResourceTypeDeviceSpecific;
    PartialDescriptor->ShareDisposition = CmResourceShareUndetermined;
    PartialDescriptor->u.DeviceSpecificData.DataSize = sizeof(CM_FLOPPY_DEVICE_DATA);

    FloppyData = ((PVOID)FullResourceDescriptor) + sizeof(CM_FULL_RESOURCE_DESCRIPTOR);
    FloppyData->Version = 2;
    FloppyData->Revision = 0;
    FloppyData->MaxDensity = MaxDensity[FloppyType];
    FloppyData->MountDensity = 0;
    RtlCopyMemory(&FloppyData->StepRateHeadUnloadTime,
                  Ptr,
                  11);
    FloppyData->MaximumTrackValue = (FloppyType == 1) ? 39 : 79;
    FloppyData->DataTransferRate = 0;

    /* Set 'Configuration Data' value */
    Error = RegSetValue(PeripheralKey,
			"Configuration Data",
			REG_FULL_RESOURCE_DESCRIPTOR,
			(PUCHAR) FullResourceDescriptor,
			Size);
    MmFreeMemory(FullResourceDescriptor);
    if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_HWDETECT,
		"RegSetValue(Configuration Data) failed (Error %u)\n",
		(int)Error));
      return;
    }

    /* Set 'Identifier' value */
    sprintf(Identifier, "FLOPPY%u", FloppyNumber + 1);
    Error = RegSetValue(PeripheralKey,
			"Identifier",
			REG_SZ,
			(PUCHAR)Identifier,
			strlen(Identifier) + 1);
    if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_HWDETECT,
		"RegSetValue() failed (Error %u)\n",
		(int)Error));
    }
  }
}


static VOID
DetectBiosFloppyController(FRLDRHKEY SystemKey,
			   FRLDRHKEY BusKey)
{
  PCM_FULL_RESOURCE_DESCRIPTOR FullResourceDescriptor;
  PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;
  FRLDRHKEY ControllerKey;
  ULONG Size;
  LONG Error;
  ULONG FloppyCount;

  FloppyCount = GetFloppyCount();
  DbgPrint((DPRINT_HWDETECT,
	    "Floppy count: %u\n",
	    FloppyCount));

  if (FloppyCount == 0)
    return;

  Error = RegCreateKey(BusKey,
		       "DiskController\\0",
		       &ControllerKey);
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_HWDETECT, "Failed to create controller key\n"));
      return;
    }

  DbgPrint((DPRINT_HWDETECT, "Created key: DiskController\\0\n"));

  /* Set 'ComponentInformation' value */
  SetComponentInformation(ControllerKey,
			  0x64,
			  0,
			  0xFFFFFFFF);

  Size = sizeof(CM_FULL_RESOURCE_DESCRIPTOR) +
	 2 * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);
  FullResourceDescriptor = MmAllocateMemory(Size);
  if (FullResourceDescriptor == NULL)
    {
      DbgPrint((DPRINT_HWDETECT,
		"Failed to allocate resource descriptor\n"));
      return;
    }
  memset(FullResourceDescriptor, 0, Size);

  /* Initialize resource descriptor */
  FullResourceDescriptor->InterfaceType = Isa;
  FullResourceDescriptor->BusNumber = 0;
  FullResourceDescriptor->PartialResourceList.Count = 3;

  /* Set IO Port */
  PartialDescriptor = &FullResourceDescriptor->PartialResourceList.PartialDescriptors[0];
  PartialDescriptor->Type = CmResourceTypePort;
  PartialDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
  PartialDescriptor->Flags = CM_RESOURCE_PORT_IO;
  PartialDescriptor->u.Port.Start.LowPart = 0x03F0;
  PartialDescriptor->u.Port.Start.HighPart = 0x0;
  PartialDescriptor->u.Port.Length = 8;

  /* Set Interrupt */
  PartialDescriptor = &FullResourceDescriptor->PartialResourceList.PartialDescriptors[1];
  PartialDescriptor->Type = CmResourceTypeInterrupt;
  PartialDescriptor->ShareDisposition = CmResourceShareUndetermined;
  PartialDescriptor->Flags = CM_RESOURCE_INTERRUPT_LATCHED;
  PartialDescriptor->u.Interrupt.Level = 6;
  PartialDescriptor->u.Interrupt.Vector = 6;
  PartialDescriptor->u.Interrupt.Affinity = 0xFFFFFFFF;

  /* Set DMA channel */
  PartialDescriptor = &FullResourceDescriptor->PartialResourceList.PartialDescriptors[2];
  PartialDescriptor->Type = CmResourceTypeDma;
  PartialDescriptor->ShareDisposition = CmResourceShareUndetermined;
  PartialDescriptor->Flags = 0;
  PartialDescriptor->u.Dma.Channel = 2;
  PartialDescriptor->u.Dma.Port = 0;

  /* Set 'Configuration Data' value */
  Error = RegSetValue(ControllerKey,
		      "Configuration Data",
		      REG_FULL_RESOURCE_DESCRIPTOR,
		      (PUCHAR) FullResourceDescriptor,
		      Size);
  MmFreeMemory(FullResourceDescriptor);
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_HWDETECT,
		"RegSetValue(Configuration Data) failed (Error %u)\n",
		(int)Error));
      return;
    }

  DetectBiosFloppyPeripheral(ControllerKey);
}


static VOID
InitializeSerialPort(ULONG Port,
		     ULONG LineControl)
{
  WRITE_PORT_UCHAR((PUCHAR)Port + 3, 0x80);  /* set DLAB on   */
  WRITE_PORT_UCHAR((PUCHAR)Port,     0x60);  /* speed LO byte */
  WRITE_PORT_UCHAR((PUCHAR)Port + 1, 0);     /* speed HI byte */
  WRITE_PORT_UCHAR((PUCHAR)Port + 3, LineControl);
  WRITE_PORT_UCHAR((PUCHAR)Port + 1, 0);     /* set comm and DLAB to 0 */
  WRITE_PORT_UCHAR((PUCHAR)Port + 4, 0x09);  /* DR int enable */
  READ_PORT_UCHAR((PUCHAR)Port + 5);  /* clear error bits */
}


static ULONG
DetectSerialMouse(ULONG Port)
{
  CHAR Buffer[4];
  ULONG i;
  ULONG TimeOut = 200;
  UCHAR LineControl;

  /* Shutdown mouse or something like that */
  LineControl = READ_PORT_UCHAR((PUCHAR)Port + 4);
  WRITE_PORT_UCHAR((PUCHAR)Port + 4, (LineControl & ~0x02) | 0x01);
  StallExecutionProcessor(100000);

  /* Clear buffer */
  while (READ_PORT_UCHAR((PUCHAR)Port + 5) & 0x01)
    READ_PORT_UCHAR((PUCHAR)Port);

  /*
   * Send modem control with 'Data Terminal Ready', 'Request To Send' and
   * 'Output Line 2' message. This enables mouse to identify.
   */
  WRITE_PORT_UCHAR((PUCHAR)Port + 4, 0x0b);

  /* Wait 10 milliseconds for the mouse getting ready */
  StallExecutionProcessor(10000);

  /* Read first four bytes, which contains Microsoft Mouse signs */
  for (i = 0; i < 4; i++)
    {
      while (((READ_PORT_UCHAR((PUCHAR)Port + 5) & 1) == 0) && (TimeOut > 0))
	{
	  StallExecutionProcessor(1000);
	  --TimeOut;
	  if (TimeOut == 0)
	    return MOUSE_TYPE_NONE;
	}
      Buffer[i] = READ_PORT_UCHAR((PUCHAR)Port);
    }

  DbgPrint((DPRINT_HWDETECT,
	    "Mouse data: %x %x %x %x\n",
	    Buffer[0],Buffer[1],Buffer[2],Buffer[3]));

  /* Check that four bytes for signs */
  for (i = 0; i < 4; ++i)
    {
      if (Buffer[i] == 'B')
	{
	  /* Sign for Microsoft Ballpoint */
//	  DbgPrint("Microsoft Ballpoint device detected\n");
//	  DbgPrint("THIS DEVICE IS NOT SUPPORTED, YET\n");
	  return MOUSE_TYPE_NONE;
	}
      else if (Buffer[i] == 'M')
	{
	  /* Sign for Microsoft Mouse protocol followed by button specifier */
	  if (i == 3)
	    {
	      /* Overflow Error */
	      return MOUSE_TYPE_NONE;
	    }

	  switch (Buffer[i + 1])
	    {
	      case '3':
		DbgPrint((DPRINT_HWDETECT,
			  "Microsoft Mouse with 3-buttons detected\n"));
		return MOUSE_TYPE_LOGITECH;

	      case 'Z':
		DbgPrint((DPRINT_HWDETECT,
			  "Microsoft Wheel Mouse detected\n"));
		return MOUSE_TYPE_WHEELZ;

	      /* case '2': */
	      default:
		DbgPrint((DPRINT_HWDETECT,
			  "Microsoft Mouse with 2-buttons detected\n"));
		return MOUSE_TYPE_MICROSOFT;
	    }
	}
    }

  return MOUSE_TYPE_NONE;
}


static ULONG
GetSerialMousePnpId(ULONG Port, char *Buffer)
{
  ULONG TimeOut;
  ULONG i = 0;
  char c;
  char x;

  WRITE_PORT_UCHAR((PUCHAR)Port + 4, 0x09);

  /* Wait 10 milliseconds for the mouse getting ready */
  StallExecutionProcessor(10000);

  WRITE_PORT_UCHAR((PUCHAR)Port + 4, 0x0b);

  StallExecutionProcessor(10000);

  for (;;)
    {
      TimeOut = 200;
      while (((READ_PORT_UCHAR((PUCHAR)Port + 5) & 1) == 0) && (TimeOut > 0))
	{
	  StallExecutionProcessor(1000);
	  --TimeOut;
	  if (TimeOut == 0)
	    {
	      return 0;
	    }
	}

      c = READ_PORT_UCHAR((PUCHAR)Port);
      if (c == 0x08 || c == 0x28)
	break;
    }

  Buffer[i++] = c;
  x = c + 1;

  for (;;)
    {
      TimeOut = 200;
      while (((READ_PORT_UCHAR((PUCHAR)Port + 5) & 1) == 0) && (TimeOut > 0))
	{
	  StallExecutionProcessor(1000);
	  --TimeOut;
	  if (TimeOut == 0)
	    return 0;
	}
      c = READ_PORT_UCHAR((PUCHAR)Port);
      Buffer[i++] = c;
      if (c == x)
	break;
      if (i >= 256)
	break;
    }

  return i;
}


static VOID
DetectSerialPointerPeripheral(FRLDRHKEY ControllerKey,
			      ULONG Base)
{
  CM_FULL_RESOURCE_DESCRIPTOR FullResourceDescriptor;
  char Buffer[256];
  char Identifier[256];
  FRLDRHKEY PeripheralKey;
  ULONG MouseType;
  ULONG Length;
  ULONG i;
  ULONG j;
  LONG Error;

  DbgPrint((DPRINT_HWDETECT,
	    "DetectSerialPointerPeripheral()\n"));

  Identifier[0] = 0;

  InitializeSerialPort(Base, 2);
  MouseType = DetectSerialMouse(Base);

  if (MouseType != MOUSE_TYPE_NONE)
    {
      Length = GetSerialMousePnpId(Base, Buffer);
      DbgPrint((DPRINT_HWDETECT,
		"PnP ID length: %u\n",
		Length));

      if (Length != 0)
	{
	  /* Convert PnP sting to ASCII */
	  if (Buffer[0] == 0x08)
	    {
	      for (i = 0; i < Length; i++)
		Buffer[i] += 0x20;
	    }
	  Buffer[Length] = 0;

	  DbgPrint((DPRINT_HWDETECT,
		    "PnP ID string: %s\n",
		    Buffer));

	  /* Copy PnpId string */
	  memcpy(&Identifier[0],
		 &Buffer[3],
		 7);
	  memcpy(&Identifier[7],
		 " - ",
		 4);

	  /* Skip device serial number */
	  i = 10;
	  if (Buffer[i] == '\\')
	    {
	      for (j = ++i; i < Length; ++i)
		{
		  if (Buffer[i] == '\\')
		    break;
		}
	      if (i >= Length)
		i -= 3;
	    }

	  /* Skip PnP class */
	  if (Buffer[i] == '\\')
	    {
	      for (j = ++i; i < Length; ++i)
		{
		  if (Buffer[i] == '\\')
		    break;
		}

	      if (i >= Length)
	        i -= 3;
	    }

	  /* Skip compatible PnP Id */
	  if (Buffer[i] == '\\')
	    {
	      for (j = ++i; i < Length; ++i)
		{
		  if (Buffer[i] == '\\')
		    break;
		}
	      if (Buffer[j] == '*')
		++j;
	      if (i >= Length)
		i -= 3;
	    }

	  /* Get product description */
	  if (Buffer[i] == '\\')
	    {
	      for (j = ++i; i < Length; ++i)
		{
		  if (Buffer[i] == ';')
		    break;
		}
	      if (i >= Length)
		i -= 3;
	      if (i > j + 1)
		{
		  memcpy(&Identifier[10],
			 &Buffer[j],
			 i - j);
		  Identifier[10 + (i-j)] = 0;
		}
	    }

	  DbgPrint((DPRINT_HWDETECT,
		    "Identifier string: %s\n",
		    Identifier));
	}

      if (Length == 0 || strlen(Identifier) < 11)
	{
	  switch (MouseType)
	    {
	      case MOUSE_TYPE_LOGITECH:
		strcpy (Identifier,
			"LOGITECH SERIAL MOUSE");
		break;

	      case MOUSE_TYPE_WHEELZ:
		strcpy (Identifier,
			"MICROSOFT SERIAL MOUSE WITH WHEEL");
		break;

	      case MOUSE_TYPE_MICROSOFT:
	      default:
		strcpy (Identifier,
			"MICROSOFT SERIAL MOUSE");
		break;
	    }
	}

      /* Create 'PointerPeripheral' key */
      Error = RegCreateKey(ControllerKey,
			   "PointerPeripheral\\0",
			   &PeripheralKey);
      if (Error != ERROR_SUCCESS)
	{
	  DbgPrint((DPRINT_HWDETECT,
		    "Failed to create peripheral key\n"));
	  return;
	}
      DbgPrint((DPRINT_HWDETECT,
		"Created key: PointerPeripheral\\0\n"));

      /* Set 'ComponentInformation' value */
      SetComponentInformation(PeripheralKey,
			      0x20,
			      0,
			      0xFFFFFFFF);

      /* Set 'Configuration Data' value */
      memset(&FullResourceDescriptor, 0, sizeof(CM_FULL_RESOURCE_DESCRIPTOR));
      FullResourceDescriptor.InterfaceType = Isa;
      FullResourceDescriptor.BusNumber = 0;
      FullResourceDescriptor.PartialResourceList.Count = 0;

      Error = RegSetValue(PeripheralKey,
			  "Configuration Data",
			  REG_FULL_RESOURCE_DESCRIPTOR,
			  (PUCHAR)&FullResourceDescriptor,
			  sizeof(CM_FULL_RESOURCE_DESCRIPTOR) -
			  sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));
      if (Error != ERROR_SUCCESS)
	{
	  DbgPrint((DPRINT_HWDETECT,
		    "RegSetValue(Configuration Data) failed (Error %u)\n",
		    (int)Error));
	}

      /* Set 'Identifier' value */
      Error = RegSetValue(PeripheralKey,
			  "Identifier",
			  REG_SZ,
			  (PUCHAR)Identifier,
			  strlen(Identifier) + 1);
      if (Error != ERROR_SUCCESS)
	{
	  DbgPrint((DPRINT_HWDETECT,
		    "RegSetValue() failed (Error %u)\n",
		    (int)Error));
	}
    }
}


static VOID
DetectSerialPorts(FRLDRHKEY BusKey)
{
  PCM_FULL_RESOURCE_DESCRIPTOR FullResourceDescriptor;
  PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;
  PCM_SERIAL_DEVICE_DATA SerialDeviceData;
  ULONG Irq[4] = {4, 3, 4, 3};
  ULONG Base;
  char Buffer[80];
  PUSHORT BasePtr;
  ULONG ControllerNumber = 0;
  FRLDRHKEY ControllerKey;
  ULONG i;
  LONG Error;
  ULONG Size;

  DbgPrint((DPRINT_HWDETECT, "DetectSerialPorts()\n"));

  ControllerNumber = 0;
  BasePtr = (PUSHORT)0x400;
  for (i = 0; i < 4; i++, BasePtr++)
    {
      Base = (ULONG)*BasePtr;
      if (Base == 0)
        continue;

      DbgPrint((DPRINT_HWDETECT,
		"Found COM%u port at 0x%x\n",
		i + 1,
		Base));

      /* Create controller key */
      sprintf(Buffer,
	      "SerialController\\%u",
	      ControllerNumber);

      Error = RegCreateKey(BusKey,
			   Buffer,
			   &ControllerKey);
      if (Error != ERROR_SUCCESS)
	{
	  DbgPrint((DPRINT_HWDETECT, "Failed to create controller key\n"));
	  continue;
	}
      DbgPrint((DPRINT_HWDETECT, "Created key: %s\n", Buffer));

      /* Set 'ComponentInformation' value */
      SetComponentInformation(ControllerKey,
			      0x78,
			      ControllerNumber,
			      0xFFFFFFFF);

      /* Build full device descriptor */
      Size = sizeof(CM_FULL_RESOURCE_DESCRIPTOR) +
	     2 * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR) +
	     sizeof(CM_SERIAL_DEVICE_DATA);
      FullResourceDescriptor = MmAllocateMemory(Size);
      if (FullResourceDescriptor == NULL)
	{
	  DbgPrint((DPRINT_HWDETECT,
		    "Failed to allocate resource descriptor\n"));
	  continue;
	}
      memset(FullResourceDescriptor, 0, Size);

      /* Initialize resource descriptor */
      FullResourceDescriptor->InterfaceType = Isa;
      FullResourceDescriptor->BusNumber = 0;
      FullResourceDescriptor->PartialResourceList.Count = 3;

      /* Set IO Port */
      PartialDescriptor = &FullResourceDescriptor->PartialResourceList.PartialDescriptors[0];
      PartialDescriptor->Type = CmResourceTypePort;
      PartialDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
      PartialDescriptor->Flags = CM_RESOURCE_PORT_IO;
      PartialDescriptor->u.Port.Start.LowPart = Base;
      PartialDescriptor->u.Port.Start.HighPart = 0x0;
      PartialDescriptor->u.Port.Length = 7;

      /* Set Interrupt */
      PartialDescriptor = &FullResourceDescriptor->PartialResourceList.PartialDescriptors[1];
      PartialDescriptor->Type = CmResourceTypeInterrupt;
      PartialDescriptor->ShareDisposition = CmResourceShareUndetermined;
      PartialDescriptor->Flags = CM_RESOURCE_INTERRUPT_LATCHED;
      PartialDescriptor->u.Interrupt.Level = Irq[i];
      PartialDescriptor->u.Interrupt.Vector = Irq[i];
      PartialDescriptor->u.Interrupt.Affinity = 0xFFFFFFFF;

      /* Set serial data (device specific) */
      PartialDescriptor = &FullResourceDescriptor->PartialResourceList.PartialDescriptors[2];
      PartialDescriptor->Type = CmResourceTypeDeviceSpecific;
      PartialDescriptor->ShareDisposition = CmResourceShareUndetermined;
      PartialDescriptor->Flags = 0;
      PartialDescriptor->u.DeviceSpecificData.DataSize = sizeof(CM_SERIAL_DEVICE_DATA);

      SerialDeviceData =
	(PCM_SERIAL_DEVICE_DATA)&FullResourceDescriptor->PartialResourceList.PartialDescriptors[3];
      SerialDeviceData->BaudClock = 1843200; /* UART Clock frequency (Hertz) */

      /* Set 'Configuration Data' value */
      Error = RegSetValue(ControllerKey,
			  "Configuration Data",
			  REG_FULL_RESOURCE_DESCRIPTOR,
			  (PUCHAR) FullResourceDescriptor,
			  Size);
      MmFreeMemory(FullResourceDescriptor);
      if (Error != ERROR_SUCCESS)
	{
	  DbgPrint((DPRINT_HWDETECT,
		    "RegSetValue(Configuration Data) failed (Error %u)\n",
		    (int)Error));
	}

      /* Set 'Identifier' value */
      sprintf(Buffer,
	      "COM%u",
	      i + 1);
      Error = RegSetValue(ControllerKey,
			  "Identifier",
			  REG_SZ,
			  (PUCHAR)Buffer,
			  strlen(Buffer) + 1);
      if (Error != ERROR_SUCCESS)
	{
	  DbgPrint((DPRINT_HWDETECT,
		    "RegSetValue() failed (Error %u)\n",
		    (int)Error));
	  continue;
	}
      DbgPrint((DPRINT_HWDETECT,
		"Created value: Identifier %s\n",
		Buffer));

      /* Detect serial mouse */
      DetectSerialPointerPeripheral(ControllerKey, Base);

      ControllerNumber++;
    }
}


static VOID
DetectParallelPorts(FRLDRHKEY BusKey)
{
  PCM_FULL_RESOURCE_DESCRIPTOR FullResourceDescriptor;
  PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;
  ULONG Irq[3] = {7, 5, (ULONG)-1};
  char Buffer[80];
  FRLDRHKEY ControllerKey;
  PUSHORT BasePtr;
  ULONG Base;
  ULONG ControllerNumber;
  ULONG i;
  LONG Error;
  ULONG Size;

  DbgPrint((DPRINT_HWDETECT, "DetectParallelPorts() called\n"));

  ControllerNumber = 0;
  BasePtr = (PUSHORT)0x408;
  for (i = 0; i < 3; i++, BasePtr++)
    {
      Base = (ULONG)*BasePtr;
      if (Base == 0)
        continue;

      DbgPrint((DPRINT_HWDETECT,
		"Parallel port %u: %x\n",
		ControllerNumber,
		Base));

      /* Create controller key */
      sprintf(Buffer,
	      "ParallelController\\%u",
	      ControllerNumber);

      Error = RegCreateKey(BusKey,
			   Buffer,
			   &ControllerKey);
      if (Error != ERROR_SUCCESS)
	{
	  DbgPrint((DPRINT_HWDETECT, "Failed to create controller key\n"));
	  continue;
	}
      DbgPrint((DPRINT_HWDETECT, "Created key: %s\n", Buffer));

      /* Set 'ComponentInformation' value */
      SetComponentInformation(ControllerKey,
			      0x40,
			      ControllerNumber,
			      0xFFFFFFFF);

      /* Build full device descriptor */
      Size = sizeof(CM_FULL_RESOURCE_DESCRIPTOR);
      if (Irq[i] != (ULONG)-1)
	Size += sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);

      FullResourceDescriptor = MmAllocateMemory(Size);
      if (FullResourceDescriptor == NULL)
	{
	  DbgPrint((DPRINT_HWDETECT,
		    "Failed to allocate resource descriptor\n"));
	  continue;
	}
      memset(FullResourceDescriptor, 0, Size);

      /* Initialize resource descriptor */
      FullResourceDescriptor->InterfaceType = Isa;
      FullResourceDescriptor->BusNumber = 0;
      FullResourceDescriptor->PartialResourceList.Count = (Irq[i] != (ULONG)-1) ? 2 : 1;

      /* Set IO Port */
      PartialDescriptor = &FullResourceDescriptor->PartialResourceList.PartialDescriptors[0];
      PartialDescriptor->Type = CmResourceTypePort;
      PartialDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
      PartialDescriptor->Flags = CM_RESOURCE_PORT_IO;
      PartialDescriptor->u.Port.Start.LowPart = Base;
      PartialDescriptor->u.Port.Start.HighPart = 0x0;
      PartialDescriptor->u.Port.Length = 3;

      /* Set Interrupt */
      if (Irq[i] != (ULONG)-1)
	{
	  PartialDescriptor = &FullResourceDescriptor->PartialResourceList.PartialDescriptors[1];
	  PartialDescriptor->Type = CmResourceTypeInterrupt;
	  PartialDescriptor->ShareDisposition = CmResourceShareUndetermined;
	  PartialDescriptor->Flags = CM_RESOURCE_INTERRUPT_LATCHED;
	  PartialDescriptor->u.Interrupt.Level = Irq[i];
	  PartialDescriptor->u.Interrupt.Vector = Irq[i];
	  PartialDescriptor->u.Interrupt.Affinity = 0xFFFFFFFF;
	}

      /* Set 'Configuration Data' value */
      Error = RegSetValue(ControllerKey,
			  "Configuration Data",
			  REG_FULL_RESOURCE_DESCRIPTOR,
			  (PUCHAR) FullResourceDescriptor,
			  Size);
      MmFreeMemory(FullResourceDescriptor);
      if (Error != ERROR_SUCCESS)
	{
	  DbgPrint((DPRINT_HWDETECT,
		    "RegSetValue(Configuration Data) failed (Error %u)\n",
		    (int)Error));
	}

      /* Set 'Identifier' value */
      sprintf(Buffer,
	      "PARALLEL%u",
	      i + 1);
      Error = RegSetValue(ControllerKey,
			  "Identifier",
			  REG_SZ,
			  (PUCHAR)Buffer,
			  strlen(Buffer) + 1);
      if (Error != ERROR_SUCCESS)
	{
	  DbgPrint((DPRINT_HWDETECT,
		    "RegSetValue() failed (Error %u)\n",
		    (int)Error));
	  continue;
	}
      DbgPrint((DPRINT_HWDETECT,
		"Created value: Identifier %s\n",
		Buffer));

      ControllerNumber++;
    }

  DbgPrint((DPRINT_HWDETECT, "DetectParallelPorts() done\n"));
}


static BOOLEAN
DetectKeyboardDevice(VOID)
{
  UCHAR Status;
  UCHAR Scancode;

  WRITE_PORT_UCHAR((PUCHAR)CONTROLLER_REGISTER_DATA,
		   0xF2);

  StallExecutionProcessor(10000);

  Status = READ_PORT_UCHAR((PUCHAR)CONTROLLER_REGISTER_STATUS);
  if ((Status & 0x01) != 0x01)
    {
      /* PC/XT keyboard or no keyboard */
      return FALSE;
    }

  Scancode = READ_PORT_UCHAR((PUCHAR)CONTROLLER_REGISTER_DATA);
  if (Scancode != 0xFA)
    {
      /* No ACK received */
      return FALSE;
    }

  StallExecutionProcessor(10000);
  Status = READ_PORT_UCHAR((PUCHAR)CONTROLLER_REGISTER_STATUS);
  if ((Status & 0x01) != 0x01)
    {
      /* Found AT keyboard */
      return TRUE;
    }

  Scancode = READ_PORT_UCHAR((PUCHAR)CONTROLLER_REGISTER_DATA);
  if (Scancode != 0xAB)
    {
      /* No 0xAB received */
      return FALSE;
    }

  StallExecutionProcessor(10000);
  Status = READ_PORT_UCHAR((PUCHAR)CONTROLLER_REGISTER_STATUS);
  if ((Status & 0x01) != 0x01)
    {
      /* No byte in buffer */
      return FALSE;
    }

  Scancode = READ_PORT_UCHAR((PUCHAR)CONTROLLER_REGISTER_DATA);
  if (Scancode != 0x41)
    {
      /* No 0x41 received */
      return FALSE;
    }

  /* Found MF-II keyboard */
  return TRUE;
}


static VOID
DetectKeyboardPeripheral(FRLDRHKEY ControllerKey)
{
  PCM_FULL_RESOURCE_DESCRIPTOR FullResourceDescriptor;
  PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;
  PCM_KEYBOARD_DEVICE_DATA KeyboardData;
  FRLDRHKEY PeripheralKey;
  char Buffer[80];
  ULONG Size;
  LONG Error;

  if (DetectKeyboardDevice())
  {
    /* Create controller key */
    Error = RegCreateKey(ControllerKey,
			 "KeyboardPeripheral\\0",
			 &PeripheralKey);
    if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_HWDETECT, "Failed to create peripheral key\n"));
      return;
    }
    DbgPrint((DPRINT_HWDETECT, "Created key: KeyboardPeripheral\\0\n"));

    /* Set 'ComponentInformation' value */
    SetComponentInformation(ControllerKey,
			    0x28,
			    0,
			    0xFFFFFFFF);

    /* Set 'Configuration Data' value */
    Size = sizeof(CM_FULL_RESOURCE_DESCRIPTOR) +
	   sizeof(CM_KEYBOARD_DEVICE_DATA);
    FullResourceDescriptor = MmAllocateMemory(Size);
    if (FullResourceDescriptor == NULL)
    {
      DbgPrint((DPRINT_HWDETECT,
		"Failed to allocate resource descriptor\n"));
      return;
    }

    /* Initialize resource descriptor */
    memset(FullResourceDescriptor, 0, Size);
    FullResourceDescriptor->InterfaceType = Isa;
    FullResourceDescriptor->BusNumber = 0;
    FullResourceDescriptor->PartialResourceList.Count = 1;

    PartialDescriptor = &FullResourceDescriptor->PartialResourceList.PartialDescriptors[0];
    PartialDescriptor->Type = CmResourceTypeDeviceSpecific;
    PartialDescriptor->ShareDisposition = CmResourceShareUndetermined;
    PartialDescriptor->u.DeviceSpecificData.DataSize = sizeof(CM_KEYBOARD_DEVICE_DATA);

    KeyboardData = ((PVOID)FullResourceDescriptor) + sizeof(CM_FULL_RESOURCE_DESCRIPTOR);
    KeyboardData->Version = 0;
    KeyboardData->Revision = 0;
    KeyboardData->Type = 4;
    KeyboardData->Subtype = 0;
    KeyboardData->KeyboardFlags = 0x20;

    /* Set 'Configuration Data' value */
    Error = RegSetValue(PeripheralKey,
			"Configuration Data",
			REG_FULL_RESOURCE_DESCRIPTOR,
			(PUCHAR)FullResourceDescriptor,
			Size);
    MmFreeMemory(FullResourceDescriptor);
    if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_HWDETECT,
		"RegSetValue(Configuration Data) failed (Error %u)\n",
		(int)Error));
    }

    /* Set 'Identifier' value */
    strcpy(Buffer,
	   "PCAT_ENHANCED");
    Error = RegSetValue(ControllerKey,
			"Identifier",
			REG_SZ,
			(PUCHAR)Buffer,
			strlen(Buffer) + 1);
    if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_HWDETECT,
		"RegSetValue() failed (Error %u)\n",
		(int)Error));
    }
  }
}


static VOID
DetectKeyboardController(FRLDRHKEY BusKey)
{
  PCM_FULL_RESOURCE_DESCRIPTOR FullResourceDescriptor;
  PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;
  FRLDRHKEY ControllerKey;
  ULONG Size;
  LONG Error;

  /* Create controller key */
  Error = RegCreateKey(BusKey,
		       "KeyboardController\\0",
		       &ControllerKey);
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_HWDETECT, "Failed to create controller key\n"));
      return;
    }
  DbgPrint((DPRINT_HWDETECT, "Created key: KeyboardController\\0\n"));

  /* Set 'ComponentInformation' value */
  SetComponentInformation(ControllerKey,
			  0x28,
			  0,
			  0xFFFFFFFF);

  /* Set 'Configuration Data' value */
  Size = sizeof(CM_FULL_RESOURCE_DESCRIPTOR) +
	  2 * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);
  FullResourceDescriptor = MmAllocateMemory(Size);
  if (FullResourceDescriptor == NULL)
    {
      DbgPrint((DPRINT_HWDETECT,
		"Failed to allocate resource descriptor\n"));
      return;
    }

  /* Initialize resource descriptor */
  memset(FullResourceDescriptor, 0, Size);
  FullResourceDescriptor->InterfaceType = Isa;
  FullResourceDescriptor->BusNumber = 0;
  FullResourceDescriptor->PartialResourceList.Count = 3;

  /* Set Interrupt */
  PartialDescriptor = &FullResourceDescriptor->PartialResourceList.PartialDescriptors[0];
  PartialDescriptor->Type = CmResourceTypeInterrupt;
  PartialDescriptor->ShareDisposition = CmResourceShareUndetermined;
  PartialDescriptor->Flags = CM_RESOURCE_INTERRUPT_LATCHED;
  PartialDescriptor->u.Interrupt.Level = 1;
  PartialDescriptor->u.Interrupt.Vector = 1;
  PartialDescriptor->u.Interrupt.Affinity = 0xFFFFFFFF;

  /* Set IO Port 0x60 */
  PartialDescriptor = &FullResourceDescriptor->PartialResourceList.PartialDescriptors[1];
  PartialDescriptor->Type = CmResourceTypePort;
  PartialDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
  PartialDescriptor->Flags = CM_RESOURCE_PORT_IO;
  PartialDescriptor->u.Port.Start.LowPart = 0x60;
  PartialDescriptor->u.Port.Start.HighPart = 0x0;
  PartialDescriptor->u.Port.Length = 1;

  /* Set IO Port 0x64 */
  PartialDescriptor = &FullResourceDescriptor->PartialResourceList.PartialDescriptors[2];
  PartialDescriptor->Type = CmResourceTypePort;
  PartialDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
  PartialDescriptor->Flags = CM_RESOURCE_PORT_IO;
  PartialDescriptor->u.Port.Start.LowPart = 0x64;
  PartialDescriptor->u.Port.Start.HighPart = 0x0;
  PartialDescriptor->u.Port.Length = 1;

  /* Set 'Configuration Data' value */
  Error = RegSetValue(ControllerKey,
		      "Configuration Data",
		      REG_FULL_RESOURCE_DESCRIPTOR,
		      (PUCHAR)FullResourceDescriptor,
		      Size);
  MmFreeMemory(FullResourceDescriptor);
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_HWDETECT,
		"RegSetValue(Configuration Data) failed (Error %u)\n",
		(int)Error));
      return;
    }

  DetectKeyboardPeripheral(ControllerKey);
}


static VOID
PS2ControllerWait(VOID)
{
  ULONG Timeout;
  UCHAR Status;

  for (Timeout = 0; Timeout < CONTROLLER_TIMEOUT; Timeout++)
    {
      Status = READ_PORT_UCHAR((PUCHAR)CONTROLLER_REGISTER_STATUS);
      if ((Status & CONTROLLER_STATUS_INPUT_BUFFER_FULL) == 0)
	return;

      /* Sleep for one millisecond */
      StallExecutionProcessor(1000);
    }
}


static BOOLEAN
DetectPS2AuxPort(VOID)
{
  ULONG Loops;
  UCHAR Scancode;
  UCHAR Status;

  /* Put the value 0x5A in the output buffer using the
   * "WriteAuxiliary Device Output Buffer" command (0xD3).
   * Poll the Status Register for a while to see if the value really turns up
   * in the Data Register. If the KEYBOARD_STATUS_MOUSE_OBF bit is also set
   * to 1 in the Status Register, we assume this controller has an
   *  Auxiliary Port (a.k.a. Mouse Port).
   */
  PS2ControllerWait();
  WRITE_PORT_UCHAR((PUCHAR)CONTROLLER_REGISTER_CONTROL,
		   CONTROLLER_COMMAND_WRITE_MOUSE_OUTPUT_BUFFER);
  PS2ControllerWait();

  /* 0x5A is a random dummy value */
  WRITE_PORT_UCHAR((PUCHAR)CONTROLLER_REGISTER_DATA,
		   0x5A);

  for (Loops = 0; Loops < 10; Loops++)
    {
      Status = READ_PORT_UCHAR((PUCHAR)CONTROLLER_REGISTER_STATUS);

      if ((Status & CONTROLLER_STATUS_OUTPUT_BUFFER_FULL) != 0)
	{
	  Scancode = READ_PORT_UCHAR((PUCHAR)CONTROLLER_REGISTER_DATA);
	  if ((Status & CONTROLLER_STATUS_MOUSE_OUTPUT_BUFFER_FULL) != 0)
	    {
	      return TRUE;
	    }
	  break;
	}

      StallExecutionProcessor(10000);
    }

  return FALSE;
}


static BOOLEAN
DetectPS2AuxDevice(VOID)
{
  UCHAR Scancode;
  UCHAR Status;

  PS2ControllerWait();
  WRITE_PORT_UCHAR((PUCHAR)CONTROLLER_REGISTER_CONTROL,
		   CONTROLLER_COMMAND_WRITE_MOUSE);
  PS2ControllerWait();

  /* Identify device */
  WRITE_PORT_UCHAR((PUCHAR)CONTROLLER_REGISTER_DATA,
		   0xF2);

  StallExecutionProcessor(10000);

  Status = READ_PORT_UCHAR((PUCHAR)CONTROLLER_REGISTER_STATUS);
  if ((Status & CONTROLLER_STATUS_MOUSE_OUTPUT_BUFFER_FULL) == 0)
    {
      return FALSE;
    }

  Scancode = READ_PORT_UCHAR((PUCHAR)CONTROLLER_REGISTER_DATA);
  if (Scancode != 0xFA)
    return FALSE;

  StallExecutionProcessor(10000);

  Status = READ_PORT_UCHAR((PUCHAR)CONTROLLER_REGISTER_STATUS);
  if ((Status & CONTROLLER_STATUS_MOUSE_OUTPUT_BUFFER_FULL) == 0)
    {
      return FALSE;
    }

  Scancode = READ_PORT_UCHAR((PUCHAR)CONTROLLER_REGISTER_DATA);
  if (Scancode != 0x00)
    return FALSE;

  return TRUE;
}


static VOID
DetectPS2Mouse(FRLDRHKEY BusKey)
{
  CM_FULL_RESOURCE_DESCRIPTOR FullResourceDescriptor;
  FRLDRHKEY ControllerKey;
  FRLDRHKEY PeripheralKey;
  LONG Error;

  if (DetectPS2AuxPort())
    {
      DbgPrint((DPRINT_HWDETECT, "Detected PS2 port\n"));

      /* Create controller key */
      Error = RegCreateKey(BusKey,
			   "PointerController\\0",
			   &ControllerKey);
      if (Error != ERROR_SUCCESS)
	{
	  DbgPrint((DPRINT_HWDETECT, "Failed to create controller key\n"));
	  return;
	}
      DbgPrint((DPRINT_HWDETECT, "Created key: PointerController\\0\n"));

      /* Set 'ComponentInformation' value */
      SetComponentInformation(ControllerKey,
			      0x20,
			      0,
			      0xFFFFFFFF);

      memset(&FullResourceDescriptor, 0, sizeof(CM_FULL_RESOURCE_DESCRIPTOR));

      /* Initialize resource descriptor */
      FullResourceDescriptor.InterfaceType = Isa;
      FullResourceDescriptor.BusNumber = 0;
      FullResourceDescriptor.PartialResourceList.Count = 1;

      /* Set Interrupt */
      FullResourceDescriptor.PartialResourceList.PartialDescriptors[0].Type = CmResourceTypeInterrupt;
      FullResourceDescriptor.PartialResourceList.PartialDescriptors[0].ShareDisposition = CmResourceShareUndetermined;
      FullResourceDescriptor.PartialResourceList.PartialDescriptors[0].Flags = CM_RESOURCE_INTERRUPT_LATCHED;
      FullResourceDescriptor.PartialResourceList.PartialDescriptors[0].u.Interrupt.Level = 12;
      FullResourceDescriptor.PartialResourceList.PartialDescriptors[0].u.Interrupt.Vector = 12;
      FullResourceDescriptor.PartialResourceList.PartialDescriptors[0].u.Interrupt.Affinity = 0xFFFFFFFF;

      /* Set 'Configuration Data' value */
      Error = RegSetValue(ControllerKey,
			  "Configuration Data",
			  REG_FULL_RESOURCE_DESCRIPTOR,
			  (PUCHAR)&FullResourceDescriptor,
			  sizeof(CM_FULL_RESOURCE_DESCRIPTOR));
      if (Error != ERROR_SUCCESS)
	{
	  DbgPrint((DPRINT_HWDETECT,
		    "RegSetValue(Configuration Data) failed (Error %u)\n",
		    (int)Error));
	  return;
	}


      if (DetectPS2AuxDevice())
	{
	  DbgPrint((DPRINT_HWDETECT, "Detected PS2 mouse\n"));

	  /* Create peripheral key */
	  Error = RegCreateKey(ControllerKey,
			       "PointerPeripheral\\0",
			       &PeripheralKey);
	  if (Error != ERROR_SUCCESS)
	    {
	      DbgPrint((DPRINT_HWDETECT, "Failed to create peripheral key\n"));
	      return;
	    }
	  DbgPrint((DPRINT_HWDETECT, "Created key: PointerPeripheral\\0\n"));

	  /* Set 'ComponentInformation' value */
	  SetComponentInformation(PeripheralKey,
				  0x20,
				  0,
				  0xFFFFFFFF);

	  /* Initialize resource descriptor */
	  memset(&FullResourceDescriptor, 0, sizeof(CM_FULL_RESOURCE_DESCRIPTOR));
	  FullResourceDescriptor.InterfaceType = Isa;
	  FullResourceDescriptor.BusNumber = 0;
	  FullResourceDescriptor.PartialResourceList.Count = 0;

	  /* Set 'Configuration Data' value */
	  Error = RegSetValue(PeripheralKey,
			      "Configuration Data",
			      REG_FULL_RESOURCE_DESCRIPTOR,
			      (PUCHAR)&FullResourceDescriptor,
			      sizeof(CM_FULL_RESOURCE_DESCRIPTOR) -
			      sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));
	  if (Error != ERROR_SUCCESS)
	    {
	      DbgPrint((DPRINT_HWDETECT,
			"RegSetValue(Configuration Data) failed (Error %u)\n",
			(int)Error));
	      return;
	    }

	  /* Set 'Identifier' value */
	  Error = RegSetValue(PeripheralKey,
			      "Identifier",
			      REG_SZ,
			      (PUCHAR)"MICROSOFT PS2 MOUSE",
			      20);
	  if (Error != ERROR_SUCCESS)
	    {
	      DbgPrint((DPRINT_HWDETECT,
			"RegSetValue() failed (Error %u)\n",
			(int)Error));
	      return;
	    }
	}
    }
}


static VOID
DetectDisplayController(FRLDRHKEY BusKey)
{
  CHAR Buffer[80];
  FRLDRHKEY ControllerKey;
  USHORT VesaVersion;
  LONG Error;

  Error = RegCreateKey(BusKey,
		       "DisplayController\\0",
		       &ControllerKey);
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_HWDETECT, "Failed to create controller key\n"));
      return;
    }
  DbgPrint((DPRINT_HWDETECT, "Created key: PointerController\\0\n"));

  /* Set 'ComponentInformation' value */
  SetComponentInformation(ControllerKey,
			  0x00,
			  0,
			  0xFFFFFFFF);

  /* FIXME: Set 'ComponentInformation' value */

  VesaVersion = BiosIsVesaSupported();
  if (VesaVersion != 0)
    {
      DbgPrint((DPRINT_HWDETECT,
		"VESA version %c.%c\n",
		(VesaVersion >> 8) + '0',
		(VesaVersion & 0xFF) + '0'));
    }
  else
    {
      DbgPrint((DPRINT_HWDETECT,
		"VESA not supported\n"));
    }

  if (VesaVersion >= 0x0200)
    {
      strcpy(Buffer,
             "VBE Display");
    }
  else
    {
      strcpy(Buffer,
             "VGA Display");
    }

  /* Set 'Identifier' value */
  Error = RegSetValue(ControllerKey,
		      "Identifier",
		      REG_SZ,
		      (PUCHAR)Buffer,
		      strlen(Buffer) + 1);
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_HWDETECT,
		"RegSetValue() failed (Error %u)\n",
		(int)Error));
      return;
    }

  /* FIXME: Add display peripheral (monitor) data */
}


static VOID
DetectIsaBios(FRLDRHKEY SystemKey, ULONG *BusNumber)
{
  PCM_FULL_RESOURCE_DESCRIPTOR FullResourceDescriptor;
  char Buffer[80];
  FRLDRHKEY BusKey;
  ULONG Size;
  LONG Error;

  /* Create new bus key */
  sprintf(Buffer,
	  "MultifunctionAdapter\\%u", *BusNumber);
  Error = RegCreateKey(SystemKey,
		       Buffer,
		       &BusKey);
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_HWDETECT, "RegCreateKey() failed (Error %u)\n", (int)Error));
      return;
    }

  /* Set 'Component Information' value similar to my NT4 box */
  SetComponentInformation(BusKey,
                          0x0,
                          0x0,
                          0xFFFFFFFF);

  /* Increment bus number */
  (*BusNumber)++;

  /* Set 'Identifier' value */
  Error = RegSetValue(BusKey,
		      "Identifier",
		      REG_SZ,
		      (PUCHAR)"ISA",
		      4);
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_HWDETECT, "RegSetValue() failed (Error %u)\n", (int)Error));
      return;
    }

  /* Set 'Configuration Data' value */
  Size = sizeof(CM_FULL_RESOURCE_DESCRIPTOR) -
	 sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);
  FullResourceDescriptor = MmAllocateMemory(Size);
  if (FullResourceDescriptor == NULL)
    {
      DbgPrint((DPRINT_HWDETECT,
		"Failed to allocate resource descriptor\n"));
      return;
    }

  /* Initialize resource descriptor */
  memset(FullResourceDescriptor, 0, Size);
  FullResourceDescriptor->InterfaceType = Isa;
  FullResourceDescriptor->BusNumber = 0;
  FullResourceDescriptor->PartialResourceList.Count = 0;

  /* Set 'Configuration Data' value */
  Error = RegSetValue(BusKey,
		      "Configuration Data",
		      REG_FULL_RESOURCE_DESCRIPTOR,
		      (PUCHAR) FullResourceDescriptor,
		      Size);
  MmFreeMemory(FullResourceDescriptor);
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_HWDETECT,
		"RegSetValue(Configuration Data) failed (Error %u)\n",
		(int)Error));
      return;
    }

  /* Detect ISA/BIOS devices */
  DetectBiosDisks(SystemKey, BusKey);

  DetectBiosFloppyController(SystemKey, BusKey);

  DetectSerialPorts(BusKey);

  DetectParallelPorts(BusKey);

  DetectKeyboardController(BusKey);

  DetectPS2Mouse(BusKey);

  DetectDisplayController(BusKey);

  /* FIXME: Detect more ISA devices */
}


VOID
PcHwDetect(VOID)
{
  FRLDRHKEY SystemKey;
  ULONG BusNumber = 0;
  LONG Error;

  DbgPrint((DPRINT_HWDETECT, "DetectHardware()\n"));

  /* Create the 'System' key */
  Error = RegCreateKey(NULL,
		       "\\Registry\\Machine\\HARDWARE\\DESCRIPTION\\System",
		       &SystemKey);
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_HWDETECT, "RegCreateKey() failed (Error %u)\n", (int)Error));
      return;
    }

#if 0
  DetectSystemData();
#endif
  DetectCPUs(SystemKey);

  /* Detect buses */
  DetectPciBios(SystemKey, &BusNumber);
  DetectApmBios(SystemKey, &BusNumber);
  DetectPnpBios(SystemKey, &BusNumber);
  DetectIsaBios(SystemKey, &BusNumber);
  DetectAcpiBios(SystemKey, &BusNumber);

  DbgPrint((DPRINT_HWDETECT, "DetectHardware() Done\n"));

#if 0
  printf("*** System stopped ***\n");
  for (;;);
#endif
}

/* EOF */
