/*
 * VideoPort driver
 *
 * Copyright (C) 2002 - 2005 ReactOS Team
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; see the file COPYING.LIB.
 * If not, write to the Free Software Foundation,
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Id$
 */

#include "videoprt.h"

/* PRIVATE FUNCTIONS **********************************************************/

NTSTATUS STDCALL
IntVideoPortMapPhysicalMemory(
   IN HANDLE Process,
   IN PHYSICAL_ADDRESS PhysicalAddress,
   IN ULONG SizeInBytes,
   IN ULONG Protect,
   IN OUT PVOID *VirtualAddress  OPTIONAL)
{
   OBJECT_ATTRIBUTES ObjAttribs;
   UNICODE_STRING UnicodeString;
   HANDLE hMemObj;
   NTSTATUS Status;
   SIZE_T Size;

   /* Initialize object attribs */
   RtlInitUnicodeString(&UnicodeString, L"\\Device\\PhysicalMemory");
   InitializeObjectAttributes(&ObjAttribs,
                              &UnicodeString,
                              OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                              NULL, NULL);

   /* Open physical memory section */
   Status = ZwOpenSection(&hMemObj, SECTION_ALL_ACCESS, &ObjAttribs);
   if (!NT_SUCCESS(Status))
   {
      DPRINT("ZwOpenSection() failed! (0x%x)\n", Status);
      return Status;
   }

   /* Map view of section */
   Size = SizeInBytes;
   Status = ZwMapViewOfSection(hMemObj,
                               Process,
                               VirtualAddress,
                               0,
                               Size,
                               (PLARGE_INTEGER)(&PhysicalAddress),
                               &Size,
                               ViewUnmap,
                               0,
                               Protect);
   ZwClose(hMemObj);
   if (!NT_SUCCESS(Status))
   {
      DPRINT("ZwMapViewOfSection() failed! (0x%x)\n", Status);
   }

   return Status;
}


PVOID STDCALL
IntVideoPortMapMemory(
   IN PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension,
   IN PHYSICAL_ADDRESS IoAddress,
   IN ULONG NumberOfUchars,
   IN UCHAR InIoSpace,
   IN HANDLE ProcessHandle,
   OUT VP_STATUS *Status)
{
   PHYSICAL_ADDRESS TranslatedAddress;
   PVIDEO_PORT_ADDRESS_MAPPING AddressMapping;
   ULONG AddressSpace;
   PVOID MappedAddress;
   PLIST_ENTRY Entry;

   DPRINT("- IoAddress: %lx\n", IoAddress.u.LowPart);
   DPRINT("- NumberOfUchars: %lx\n", NumberOfUchars);
   DPRINT("- InIoSpace: %x\n", InIoSpace);

   InIoSpace &= ~VIDEO_MEMORY_SPACE_DENSE;
   if ((InIoSpace & VIDEO_MEMORY_SPACE_P6CACHE) != 0)
   {
      DPRINT("VIDEO_MEMORY_SPACE_P6CACHE not supported, turning off\n");
      InIoSpace &= ~VIDEO_MEMORY_SPACE_P6CACHE;
   }

   if (ProcessHandle != NULL && (InIoSpace & VIDEO_MEMORY_SPACE_USER_MODE) == 0)
   {
      DPRINT("ProcessHandle is not NULL (0x%x) but InIoSpace does not have "
             "VIDEO_MEMORY_SPACE_USER_MODE set! Setting "
             "VIDEO_MEMORY_SPACE_USER_MODE.\n",
             ProcessHandle);
      InIoSpace |= VIDEO_MEMORY_SPACE_USER_MODE;
   }
   else if (ProcessHandle == NULL && (InIoSpace & VIDEO_MEMORY_SPACE_USER_MODE) != 0)
   {
      DPRINT("ProcessHandle is NULL (0x%x) but InIoSpace does have "
             "VIDEO_MEMORY_SPACE_USER_MODE set! Setting ProcessHandle "
             "to NtCurrentProcess()\n",
             ProcessHandle);
      ProcessHandle = NtCurrentProcess();
   }

   if ((InIoSpace & VIDEO_MEMORY_SPACE_USER_MODE) == 0 &&
       !IsListEmpty(&DeviceExtension->AddressMappingListHead))
   {
      Entry = DeviceExtension->AddressMappingListHead.Flink;
      while (Entry != &DeviceExtension->AddressMappingListHead)
      {
         AddressMapping = CONTAINING_RECORD(
            Entry,
            VIDEO_PORT_ADDRESS_MAPPING,
            List);
         if (IoAddress.QuadPart == AddressMapping->IoAddress.QuadPart &&
             NumberOfUchars <= AddressMapping->NumberOfUchars)
         {
            {
               AddressMapping->MappingCount++;
               if (Status)
                  *Status = NO_ERROR;
               return AddressMapping->MappedAddress;
            }
         }
         Entry = Entry->Flink;
      }
   }

   AddressSpace = (ULONG)InIoSpace;
   AddressSpace &= ~VIDEO_MEMORY_SPACE_USER_MODE;
   if (HalTranslateBusAddress(
          DeviceExtension->AdapterInterfaceType,
          DeviceExtension->SystemIoBusNumber,
          IoAddress,
          &AddressSpace,
          &TranslatedAddress) == FALSE)
   {
      if (Status)
         *Status = ERROR_NOT_ENOUGH_MEMORY;

      return NULL;
   }

   /* I/O space */
   if (AddressSpace != 0)
   {
      ASSERT(0 == TranslatedAddress.u.HighPart);
      if (Status)
         *Status = NO_ERROR;

      return (PVOID)TranslatedAddress.u.LowPart;
   }

   /* user space */
   if ((InIoSpace & VIDEO_MEMORY_SPACE_USER_MODE) != 0)
   {
      NTSTATUS NtStatus;
      MappedAddress = NULL;
      NtStatus = IntVideoPortMapPhysicalMemory(ProcessHandle,
                                               TranslatedAddress,
                                               NumberOfUchars,
                                               PAGE_READWRITE/* | PAGE_WRITECOMBINE*/,
                                               &MappedAddress);
      if (!NT_SUCCESS(NtStatus))
      {
         DPRINT("IntVideoPortMapPhysicalMemory() failed! (0x%x)\n", NtStatus);
         if (Status)
            *Status = NO_ERROR;
         return NULL;
      }
      DPRINT("Mapped user address = 0x%08x\n", MappedAddress);
   }
   else /* kernel space */
   {
      MappedAddress = MmMapIoSpace(
         TranslatedAddress,
         NumberOfUchars,
         MmNonCached);
   }

   if (MappedAddress != NULL)
   {
      if (Status)
      {
         *Status = NO_ERROR;
      }
      if ((InIoSpace & VIDEO_MEMORY_SPACE_USER_MODE) == 0)
      {
         AddressMapping = ExAllocatePoolWithTag(
            PagedPool,
            sizeof(VIDEO_PORT_ADDRESS_MAPPING),
            TAG_VIDEO_PORT);

         if (AddressMapping == NULL)
            return MappedAddress;

         RtlZeroMemory(AddressMapping, sizeof(VIDEO_PORT_ADDRESS_MAPPING));
         AddressMapping->NumberOfUchars = NumberOfUchars;
         AddressMapping->IoAddress = IoAddress;
         AddressMapping->SystemIoBusNumber = DeviceExtension->SystemIoBusNumber;
         AddressMapping->MappedAddress = MappedAddress;
         AddressMapping->MappingCount = 1;
         InsertHeadList(
            &DeviceExtension->AddressMappingListHead,
            &AddressMapping->List);
      }

      return MappedAddress;
   }

   if (Status)
      *Status = NO_ERROR;

   return NULL;
}

VOID STDCALL
IntVideoPortUnmapMemory(
   IN PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension,
   IN PVOID MappedAddress)
{
   PVIDEO_PORT_ADDRESS_MAPPING AddressMapping;
   PLIST_ENTRY Entry;
   NTSTATUS Status;

   Entry = DeviceExtension->AddressMappingListHead.Flink;
   while (Entry != &DeviceExtension->AddressMappingListHead)
   {
      AddressMapping = CONTAINING_RECORD(
         Entry,
         VIDEO_PORT_ADDRESS_MAPPING,
         List);
      if (AddressMapping->MappedAddress == MappedAddress)
      {
         ASSERT(AddressMapping->MappingCount > 0);
         AddressMapping->MappingCount--;
         if (AddressMapping->MappingCount == 0)
         {
            MmUnmapIoSpace(
               AddressMapping->MappedAddress,
               AddressMapping->NumberOfUchars);
            RemoveEntryList(Entry);
            ExFreePool(AddressMapping);
         }
         return;
      }

      Entry = Entry->Flink;
   }

   /* If there was no kernelmode mapping for the given address found we assume
    * that the given address is a usermode mapping and try to unmap it.
    *
    * FIXME: Is it ok to use NtCurrentProcess?
    */
   Status = ZwUnmapViewOfSection(NtCurrentProcess(), MappedAddress);
   if (!NT_SUCCESS(Status))
   {
      DPRINT1("Warning: Mapping for address 0x%x not found!\n", (ULONG)MappedAddress);
   }
}

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @implemented
 */

PVOID STDCALL
VideoPortGetDeviceBase(
   IN PVOID HwDeviceExtension,
   IN PHYSICAL_ADDRESS IoAddress,
   IN ULONG NumberOfUchars,
   IN UCHAR InIoSpace)
{
   DPRINT("VideoPortGetDeviceBase\n");
   return IntVideoPortMapMemory(
      VIDEO_PORT_GET_DEVICE_EXTENSION(HwDeviceExtension),
      IoAddress,
      NumberOfUchars,
      InIoSpace,
      NULL,
      NULL);
}

/*
 * @implemented
 */

VOID STDCALL
VideoPortFreeDeviceBase(
   IN PVOID HwDeviceExtension,
   IN PVOID MappedAddress)
{
   DPRINT("VideoPortFreeDeviceBase\n");
   IntVideoPortUnmapMemory(
      VIDEO_PORT_GET_DEVICE_EXTENSION(HwDeviceExtension),
      MappedAddress);
}

/*
 * @unimplemented
 */

VP_STATUS STDCALL
VideoPortMapBankedMemory(
   IN PVOID HwDeviceExtension,
   IN PHYSICAL_ADDRESS PhysicalAddress,
   IN PULONG Length,
   IN PULONG InIoSpace,
   OUT PVOID *VirtualAddress,
   IN ULONG BankLength,
   IN UCHAR ReadWriteBank,
   IN PBANKED_SECTION_ROUTINE BankRoutine,
   IN PVOID Context)
{
   DPRINT("VideoPortMapBankedMemory\n");
   UNIMPLEMENTED;
   return ERROR_CALL_NOT_IMPLEMENTED;
}


/*
 * @implemented
 */

VP_STATUS STDCALL
VideoPortMapMemory(
   IN PVOID HwDeviceExtension,
   IN PHYSICAL_ADDRESS PhysicalAddress,
   IN PULONG Length,
   IN PULONG InIoSpace,
   OUT PVOID *VirtualAddress)
{
   PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;
   NTSTATUS Status;

   DPRINT("VideoPortMapMemory\n");
   DPRINT("- *VirtualAddress: 0x%x\n", *VirtualAddress);

   DeviceExtension = VIDEO_PORT_GET_DEVICE_EXTENSION(HwDeviceExtension);
   *VirtualAddress = IntVideoPortMapMemory(
      DeviceExtension,
      PhysicalAddress,
      *Length,
      *InIoSpace,
      (HANDLE)*VirtualAddress,
      &Status);

   return Status;
}

/*
 * @implemented
 */

VP_STATUS STDCALL
VideoPortUnmapMemory(
   IN PVOID HwDeviceExtension,
   IN PVOID VirtualAddress,
   IN HANDLE ProcessHandle)
{
   DPRINT("VideoPortFreeDeviceBase\n");

   IntVideoPortUnmapMemory(
      VIDEO_PORT_GET_DEVICE_EXTENSION(HwDeviceExtension),
      VirtualAddress);

   return NO_ERROR;
}

/*
 * @implemented
 */

VP_STATUS STDCALL
VideoPortGetAccessRanges(
   IN PVOID HwDeviceExtension,
   IN ULONG NumRequestedResources,
   IN PIO_RESOURCE_DESCRIPTOR RequestedResources OPTIONAL,
   IN ULONG NumAccessRanges,
   IN PVIDEO_ACCESS_RANGE AccessRanges,
   IN PVOID VendorId,
   IN PVOID DeviceId,
   IN PULONG Slot)
{
   PCI_SLOT_NUMBER PciSlotNumber;
   ULONG FunctionNumber;
   PCI_COMMON_CONFIG Config;
   PCM_RESOURCE_LIST AllocatedResources;
   NTSTATUS Status;
   UINT AssignedCount;
   CM_FULL_RESOURCE_DESCRIPTOR *FullList;
   CM_PARTIAL_RESOURCE_DESCRIPTOR *Descriptor;
   PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;
   USHORT VendorIdToFind;
   USHORT DeviceIdToFind;
   ULONG SlotIdToFind;
   ULONG ReturnedLength;

   DPRINT("VideoPortGetAccessRanges\n");

   DeviceExtension = VIDEO_PORT_GET_DEVICE_EXTENSION(HwDeviceExtension);

   if (NumRequestedResources == 0)
   {
      AllocatedResources = DeviceExtension->AllocatedResources;
      if (AllocatedResources == NULL &&
          DeviceExtension->AdapterInterfaceType == PCIBus)
      {
         if (DeviceExtension->PhysicalDeviceObject != NULL)
         {
            PciSlotNumber.u.AsULONG = DeviceExtension->SystemIoSlotNumber;

            ReturnedLength = HalGetBusData(
               PCIConfiguration,
               DeviceExtension->SystemIoBusNumber,
               PciSlotNumber.u.AsULONG,
               &Config,
               sizeof(PCI_COMMON_CONFIG));

            if (ReturnedLength != sizeof(PCI_COMMON_CONFIG))
            {
               return ERROR_NO_SYSTEM_RESOURCES;
            }
         }
         else
         {
            VendorIdToFind = VendorId != NULL ? *(PUSHORT)VendorId : 0;
            DeviceIdToFind = DeviceId != NULL ? *(PUSHORT)DeviceId : 0;
            SlotIdToFind = Slot != NULL ? *Slot : 0;
            PciSlotNumber.u.AsULONG = SlotIdToFind;

            DPRINT("Looking for VendorId 0x%04x DeviceId 0x%04x\n",
                   VendorIdToFind, DeviceIdToFind);

            /*
             * Search for the device id and vendor id on this bus.
             */

            for (FunctionNumber = 0; FunctionNumber < 8; FunctionNumber++)
            {
               DPRINT("- Function number: %d\n", FunctionNumber);
               PciSlotNumber.u.bits.FunctionNumber = FunctionNumber;
               ReturnedLength = HalGetBusData(
                  PCIConfiguration,
                  DeviceExtension->SystemIoBusNumber,
                  PciSlotNumber.u.AsULONG,
                  &Config,
                  sizeof(PCI_COMMON_CONFIG));
               DPRINT("- Length of data: %x\n", ReturnedLength);
               if (ReturnedLength == sizeof(PCI_COMMON_CONFIG))
               {
                  DPRINT("- Slot 0x%02x (Device %d Function %d) VendorId 0x%04x "
                         "DeviceId 0x%04x\n",
                         PciSlotNumber.u.AsULONG,
                         PciSlotNumber.u.bits.DeviceNumber,
                         PciSlotNumber.u.bits.FunctionNumber,
                         Config.VendorID,
                         Config.DeviceID);

                  if ((VendorIdToFind == 0 || Config.VendorID == VendorIdToFind) &&
                      (DeviceIdToFind == 0 || Config.DeviceID == DeviceIdToFind))
                  {
                     break;
                  }
               }
            }

            if (FunctionNumber == 8)
            {
               DPRINT("Didn't find device.\n");
               return ERROR_DEV_NOT_EXIST;
            }
         }

         Status = HalAssignSlotResources(
            NULL, NULL, NULL, NULL,
            DeviceExtension->AdapterInterfaceType,
            DeviceExtension->SystemIoBusNumber,
            PciSlotNumber.u.AsULONG,
            &AllocatedResources);

         if (!NT_SUCCESS(Status))
         {
            return Status;
         }
         DeviceExtension->AllocatedResources = AllocatedResources;
      }
      if (AllocatedResources == NULL)
         return ERROR_NO_SYSTEM_RESOURCES;

      AssignedCount = 0;
      for (FullList = AllocatedResources->List;
           FullList < AllocatedResources->List + AllocatedResources->Count;
           FullList++)
      {
         ASSERT(FullList->InterfaceType == PCIBus &&
                FullList->BusNumber == DeviceExtension->SystemIoBusNumber &&
                1 == FullList->PartialResourceList.Version &&
                1 == FullList->PartialResourceList.Revision);
	 for (Descriptor = FullList->PartialResourceList.PartialDescriptors;
              Descriptor < FullList->PartialResourceList.PartialDescriptors + FullList->PartialResourceList.Count;
              Descriptor++)
         {
            if ((Descriptor->Type == CmResourceTypeMemory ||
                 Descriptor->Type == CmResourceTypePort) &&
                AssignedCount >= NumAccessRanges)
            {
               DPRINT1("Too many access ranges found\n");
               return ERROR_NO_SYSTEM_RESOURCES;
            }
            if (Descriptor->Type == CmResourceTypeMemory)
            {
               if (NumAccessRanges <= AssignedCount)
               {
                  DPRINT1("Too many access ranges found\n");
                  return ERROR_NO_SYSTEM_RESOURCES;
               }
               DPRINT("Memory range starting at 0x%08x length 0x%08x\n",
                      Descriptor->u.Memory.Start.u.LowPart, Descriptor->u.Memory.Length);
               AccessRanges[AssignedCount].RangeStart = Descriptor->u.Memory.Start;
               AccessRanges[AssignedCount].RangeLength = Descriptor->u.Memory.Length;
               AccessRanges[AssignedCount].RangeInIoSpace = 0;
               AccessRanges[AssignedCount].RangeVisible = 0; /* FIXME: Just guessing */
               AccessRanges[AssignedCount].RangeShareable =
                  (Descriptor->ShareDisposition == CmResourceShareShared);
               AssignedCount++;
            }
            else if (Descriptor->Type == CmResourceTypePort)
            {
               DPRINT("Port range starting at 0x%04x length %d\n",
                      Descriptor->u.Memory.Start.u.LowPart, Descriptor->u.Memory.Length);
               AccessRanges[AssignedCount].RangeStart = Descriptor->u.Port.Start;
               AccessRanges[AssignedCount].RangeLength = Descriptor->u.Port.Length;
               AccessRanges[AssignedCount].RangeInIoSpace = 1;
               AccessRanges[AssignedCount].RangeVisible = 0; /* FIXME: Just guessing */
               AccessRanges[AssignedCount].RangeShareable = 0;
               AssignedCount++;
            }
            else if (Descriptor->Type == CmResourceTypeInterrupt)
            {
               DeviceExtension->InterruptLevel = Descriptor->u.Interrupt.Level;
               DeviceExtension->InterruptVector = Descriptor->u.Interrupt.Vector;
               if (Descriptor->ShareDisposition == CmResourceShareShared)
                  DeviceExtension->InterruptShared = TRUE;
               else
                  DeviceExtension->InterruptShared = FALSE;
            }
         }
      }
   }
   else
   {
      UNIMPLEMENTED
   }

   return NO_ERROR;
}

/*
 * @unimplemented
 */

VP_STATUS STDCALL
VideoPortVerifyAccessRanges(
   IN PVOID HwDeviceExtension,
   IN ULONG NumAccessRanges,
   IN PVIDEO_ACCESS_RANGE AccessRanges)
{
   DPRINT1("VideoPortVerifyAccessRanges not implemented\n");
   return NO_ERROR;
}

/*
 * @unimplemented
 */

VP_STATUS STDCALL
VideoPortGetDeviceData(
   IN PVOID HwDeviceExtension,
   IN VIDEO_DEVICE_DATA_TYPE DeviceDataType,
   IN PMINIPORT_QUERY_DEVICE_ROUTINE CallbackRoutine,
   IN PVOID Context)
{
   DPRINT("VideoPortGetDeviceData\n");
   UNIMPLEMENTED;
   return ERROR_CALL_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */

PVOID STDCALL
VideoPortAllocatePool(
   IN PVOID HwDeviceExtension,
   IN VP_POOL_TYPE PoolType,
   IN SIZE_T NumberOfBytes,
   IN ULONG Tag)
{
   DPRINT("VideoPortAllocatePool\n");
   return ExAllocatePoolWithTag(PoolType, NumberOfBytes, Tag);
}

/*
 * @implemented
 */

VOID STDCALL
VideoPortFreePool(
   IN PVOID HwDeviceExtension,
   IN PVOID Ptr)
{
   ExFreePool(Ptr);
}

/*
 * @implemented
 */

VP_STATUS STDCALL
VideoPortAllocateBuffer(
   IN PVOID HwDeviceExtension,
   IN ULONG Size,
   OUT PVOID *Buffer)
{
   DPRINT("VideoPortAllocateBuffer\n");
   *Buffer = ExAllocatePool(PagedPool, Size);
   return *Buffer == NULL ? ERROR_NOT_ENOUGH_MEMORY : NO_ERROR;
}

/*
 * @implemented
 */

VOID STDCALL
VideoPortReleaseBuffer(
   IN PVOID HwDeviceExtension,
   IN PVOID Ptr)
{
   DPRINT("VideoPortReleaseBuffer\n");
   ExFreePool(Ptr);
}

/*
 * @unimplemented
 */

PVOID STDCALL
VideoPortLockBuffer(
   IN PVOID HwDeviceExtension,
   IN PVOID BaseAddress,
   IN ULONG Length,
   IN VP_LOCK_OPERATION Operation)
{
   DPRINT1("VideoPortLockBuffer: Unimplemented.\n");
   return NULL;
}

/*
 * @unimplemented
 */

VOID STDCALL
VideoPortUnlockBuffer(
   IN PVOID HwDeviceExtension,
   IN PVOID Mdl)
{
   DPRINT1("VideoPortUnlockBuffer: Unimplemented.\n");
}

/*
 * @unimplemented
 */

VP_STATUS STDCALL
VideoPortSetTrappedEmulatorPorts(
   IN PVOID HwDeviceExtension,
   IN ULONG NumAccessRanges,
   IN PVIDEO_ACCESS_RANGE AccessRange)
{
   DPRINT("VideoPortSetTrappedEmulatorPorts\n");
   /* Should store the ranges in the device extension for use by ntvdm. */
   return NO_ERROR;
}

/*
 * @implemented
 */

ULONG STDCALL
VideoPortGetBusData(
   IN PVOID HwDeviceExtension,
   IN BUS_DATA_TYPE BusDataType,
   IN ULONG SlotNumber,
   OUT PVOID Buffer,
   IN ULONG Offset,
   IN ULONG Length)
{
   PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;

   DPRINT("VideoPortGetBusData\n");

   DeviceExtension = VIDEO_PORT_GET_DEVICE_EXTENSION(HwDeviceExtension);

   if (BusDataType != Cmos)
   {
      /* Legacy vs. PnP behaviour */
      if (DeviceExtension->PhysicalDeviceObject != NULL)
         SlotNumber = DeviceExtension->SystemIoSlotNumber;
   }

   return HalGetBusDataByOffset(
      BusDataType,
      DeviceExtension->SystemIoBusNumber,
      SlotNumber,
      Buffer,
      Offset,
      Length);
}

/*
 * @implemented
 */

ULONG STDCALL
VideoPortSetBusData(
   IN PVOID HwDeviceExtension,
   IN BUS_DATA_TYPE BusDataType,
   IN ULONG SlotNumber,
   IN PVOID Buffer,
   IN ULONG Offset,
   IN ULONG Length)
{
   PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;

   DPRINT("VideoPortSetBusData\n");

   DeviceExtension = VIDEO_PORT_GET_DEVICE_EXTENSION(HwDeviceExtension);

   if (BusDataType != Cmos)
   {
      /* Legacy vs. PnP behaviour */
      if (DeviceExtension->PhysicalDeviceObject != NULL)
         SlotNumber = DeviceExtension->SystemIoSlotNumber;
   }

   return HalSetBusDataByOffset(
      BusDataType,
      DeviceExtension->SystemIoBusNumber,
      SlotNumber,
      Buffer,
      Offset,
      Length);
}
