/*
 *  FreeLoader
 *
 *  Copyright (C) 2004  Eric Kohl
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

#define NDEBUG
#include <debug.h>

#include <pshpack1.h>

typedef struct _ROUTING_SLOT
{
  UCHAR  BusNumber;
  UCHAR  DeviceNumber;
  UCHAR  LinkA;
  USHORT BitmapA;
  UCHAR  LinkB;
  USHORT BitmapB;
  UCHAR  LinkC;
  USHORT BitmapC;
  UCHAR  LinkD;
  USHORT BitmapD;
  UCHAR  SlotNumber;
  UCHAR  Reserved;
} ROUTING_SLOT, *PROUTING_SLOT;

typedef struct _PCI_IRQ_ROUTING_TABLE
{
  ULONG Signature;
  USHORT Version;
  USHORT Size;
  UCHAR  RouterBus;
  UCHAR  RouterSlot;
  USHORT ExclusiveIRQs;
  ULONG CompatibleRouter;
  ULONG MiniportData;
  UCHAR  Reserved[11];
  UCHAR  Checksum;
  ROUTING_SLOT Slot[1];
} PCI_IRQ_ROUTING_TABLE, *PPCI_IRQ_ROUTING_TABLE;

#include <poppack.h>

typedef struct _PCI_REGISTRY_INFO
{
    UCHAR MajorRevision;
    UCHAR MinorRevision;
    UCHAR NoBuses;
    UCHAR HardwareMechanism;
} PCI_REGISTRY_INFO, *PPCI_REGISTRY_INFO;

static PPCI_IRQ_ROUTING_TABLE
GetPciIrqRoutingTable(VOID)
{
  PPCI_IRQ_ROUTING_TABLE Table;
  PUCHAR Ptr;
  ULONG Sum;
  ULONG i;

  Table = (PPCI_IRQ_ROUTING_TABLE)0xF0000;
  while ((ULONG_PTR)Table < 0x100000)
    {
      if (Table->Signature == 0x52495024)
	{
	  DPRINTM(DPRINT_HWDETECT,
		    "Found signature\n");

	  Ptr = (PUCHAR)Table;
	  Sum = 0;
	  for (i = 0; i < Table->Size; i++)
	    {
	      Sum += Ptr[i];
	    }

	  if ((Sum & 0xFF) != 0)
	    {
	      DPRINTM(DPRINT_HWDETECT,
			"Invalid routing table\n");
	      return NULL;
	    }

	  DPRINTM(DPRINT_HWDETECT,
		   "Valid checksum\n");

	  return Table;
	}

      Table = (PPCI_IRQ_ROUTING_TABLE)((ULONG_PTR)Table + 0x10);
    }

  return NULL;
}


static BOOLEAN
FindPciBios(PPCI_REGISTRY_INFO BusData)
{
  REGS  RegsIn;
  REGS  RegsOut;

  RegsIn.b.ah = 0xB1; /* Subfunction B1h */
  RegsIn.b.al = 0x01; /* PCI BIOS present */

  Int386(0x1A, &RegsIn, &RegsOut);

  if (INT386_SUCCESS(RegsOut) && RegsOut.d.edx == 0x20494350 && RegsOut.b.ah == 0)
    {
      DPRINTM(DPRINT_HWDETECT, "Found PCI bios\n");

      DPRINTM(DPRINT_HWDETECT, "AL: %x\n", RegsOut.b.al);
      DPRINTM(DPRINT_HWDETECT, "BH: %x\n", RegsOut.b.bh);
      DPRINTM(DPRINT_HWDETECT, "BL: %x\n", RegsOut.b.bl);
      DPRINTM(DPRINT_HWDETECT, "CL: %x\n", RegsOut.b.cl);

      BusData->NoBuses = RegsOut.b.cl + 1;
      BusData->MajorRevision = RegsOut.b.bh;
      BusData->MinorRevision = RegsOut.b.bl;
      BusData->HardwareMechanism = RegsOut.b.cl;

      return TRUE;
    }


  DPRINTM(DPRINT_HWDETECT, "No PCI bios found\n");

  return FALSE;
}


static VOID
DetectPciIrqRoutingTable(PCONFIGURATION_COMPONENT_DATA BusKey)
{
  PCM_PARTIAL_RESOURCE_LIST PartialResourceList;
  PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;
  PPCI_IRQ_ROUTING_TABLE Table;
  PCONFIGURATION_COMPONENT_DATA TableKey;
  ULONG Size;

  Table = GetPciIrqRoutingTable();
  if (Table != NULL)
    {
      DPRINTM(DPRINT_HWDETECT, "Table size: %u\n", Table->Size);

      /* Set 'Configuration Data' value */
      Size = FIELD_OFFSET(CM_PARTIAL_RESOURCE_LIST, PartialDescriptors) +
         2 * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR) + Table->Size;
      PartialResourceList = MmHeapAlloc(Size);
      if (PartialResourceList == NULL)
      {
          DPRINTM(DPRINT_HWDETECT,
              "Failed to allocate resource descriptor\n");
          return;
      }

      /* Initialize resource descriptor */
      memset(PartialResourceList, 0, Size);
      PartialResourceList->Version = 1;
      PartialResourceList->Revision = 1;
      PartialResourceList->Count = 2;

      PartialDescriptor = &PartialResourceList->PartialDescriptors[0];
      PartialDescriptor->Type = CmResourceTypeBusNumber;
      PartialDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
      PartialDescriptor->u.BusNumber.Start = 0;
      PartialDescriptor->u.BusNumber.Length = 1;

      PartialDescriptor = &PartialResourceList->PartialDescriptors[1];
      PartialDescriptor->Type = CmResourceTypeDeviceSpecific;
      PartialDescriptor->ShareDisposition = CmResourceShareUndetermined;
      PartialDescriptor->u.DeviceSpecificData.DataSize = Table->Size;

      memcpy(&PartialResourceList->PartialDescriptors[2],
          Table, Table->Size);

      FldrCreateComponentKey(BusKey,
                             PeripheralClass,
                             RealModeIrqRoutingTable,
                             0x0,
                             0x0,
                             0xFFFFFFFF,
                             "PCI Real-mode IRQ Routing Table",
                             PartialResourceList,
                             Size,
                             &TableKey);

      MmHeapFree(PartialResourceList);
    }
}


VOID
DetectPciBios(PCONFIGURATION_COMPONENT_DATA SystemKey, ULONG *BusNumber)
{
  PCM_PARTIAL_RESOURCE_LIST PartialResourceList;
  PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;
  PCI_REGISTRY_INFO BusData;
  PCONFIGURATION_COMPONENT_DATA BiosKey;
  ULONG Size;
  PCONFIGURATION_COMPONENT_DATA BusKey;
  ULONG i;

  /* Report the PCI BIOS */
  if (FindPciBios(&BusData))
    {
      /* Set 'Configuration Data' value */
      Size = FIELD_OFFSET(CM_PARTIAL_RESOURCE_LIST,
                          PartialDescriptors);
      PartialResourceList = MmHeapAlloc(Size);
      if (PartialResourceList == NULL)
      {
          DPRINTM(DPRINT_HWDETECT,
              "Failed to allocate resource descriptor\n");
          return;
      }

      /* Initialize resource descriptor */
      memset(PartialResourceList, 0, Size);

      /* Create new bus key */
      FldrCreateComponentKey(SystemKey,
                             AdapterClass,
                             MultiFunctionAdapter,
                             0x0,
                             0x0,
                             0xFFFFFFFF,
                             "PCI BIOS",
                             PartialResourceList,
                             Size,
                             &BiosKey);

      /* Increment bus number */
      (*BusNumber)++;

      MmHeapFree(PartialResourceList);

      DetectPciIrqRoutingTable(BiosKey);

      /* Report PCI buses */
      for (i = 0; i < (ULONG)BusData.NoBuses; i++)
      {
          /* Check if this is the first bus */
          if (i == 0)
          {
              /* Set 'Configuration Data' value */
              Size = FIELD_OFFSET(CM_PARTIAL_RESOURCE_LIST,
                                  PartialDescriptors) +
                     sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR) +
                     sizeof(PCI_REGISTRY_INFO);
              PartialResourceList = MmHeapAlloc(Size);
              if (!PartialResourceList)
              {
                  DPRINTM(DPRINT_HWDETECT,
                            "Failed to allocate resource descriptor\n");
                  return;
              }

              /* Initialize resource descriptor */
              memset(PartialResourceList, 0, Size);
              PartialResourceList->Version = 1;
              PartialResourceList->Revision = 1;
              PartialResourceList->Count = 1;
              PartialDescriptor = &PartialResourceList->PartialDescriptors[0];
              PartialDescriptor->Type = CmResourceTypeDeviceSpecific;
              PartialDescriptor->ShareDisposition = CmResourceShareUndetermined;
              PartialDescriptor->u.DeviceSpecificData.DataSize = sizeof(PCI_REGISTRY_INFO);
              memcpy(&PartialResourceList->PartialDescriptors[1],
                     &BusData,
                     sizeof(PCI_REGISTRY_INFO));
          }
          else
          {
              /* Set 'Configuration Data' value */
              Size = FIELD_OFFSET(CM_PARTIAL_RESOURCE_LIST,
                                  PartialDescriptors);
              PartialResourceList = MmHeapAlloc(Size);
              if (!PartialResourceList)
              {
                  DPRINTM(DPRINT_HWDETECT,
                            "Failed to allocate resource descriptor\n");
                  return;
              }

              /* Initialize resource descriptor */
              memset(PartialResourceList, 0, Size);
          }

          /* Create the bus key */
          FldrCreateComponentKey(SystemKey,
                                 AdapterClass,
                                 MultiFunctionAdapter,
                                 0x0,
                                 0x0,
                                 0xFFFFFFFF,
                                 "PCI",
                                 PartialResourceList,
                                 Size,
                                 &BusKey);

          MmHeapFree(PartialResourceList);

          /* Increment bus number */
          (*BusNumber)++;
      }
    }
}

/* EOF */
