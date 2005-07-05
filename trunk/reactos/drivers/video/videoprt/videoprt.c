/*
 * VideoPort driver
 *
 * Copyright (C) 2002, 2003, 2004 ReactOS Team
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

/* GLOBAL VARIABLES ***********************************************************/

ULONG CsrssInitialized = FALSE;
PKPROCESS Csrss = NULL;

/* PRIVATE FUNCTIONS **********************************************************/

NTSTATUS STDCALL
DriverEntry(
   IN PDRIVER_OBJECT DriverObject,
   IN PUNICODE_STRING RegistryPath)
{
   return STATUS_SUCCESS;
}

PVOID STDCALL
IntVideoPortImageDirectoryEntryToData(
   PVOID BaseAddress,
   ULONG Directory)
{
   PIMAGE_NT_HEADERS NtHeader;
   ULONG Va;

   NtHeader = RtlImageNtHeader(BaseAddress);
   if (NtHeader == NULL)
      return NULL;

   if (Directory >= NtHeader->OptionalHeader.NumberOfRvaAndSizes)
      return NULL;

   Va = NtHeader->OptionalHeader.DataDirectory[Directory].VirtualAddress;
   if (Va == 0)
      return NULL;

   return (PVOID)((ULONG_PTR)BaseAddress + Va);
}

PVOID STDCALL
IntVideoPortGetProcAddress(
   IN PVOID HwDeviceExtension,
   IN PUCHAR FunctionName)
{
   SYSTEM_GDI_DRIVER_INFORMATION GdiDriverInfo;
   PVOID BaseAddress;
   PIMAGE_EXPORT_DIRECTORY ExportDir;
   PUSHORT OrdinalPtr;
   PULONG NamePtr;
   PULONG AddressPtr;
   ULONG i = 0;
   NTSTATUS Status;

   DPRINT("VideoPortGetProcAddress(%s)\n", FunctionName);

   RtlInitUnicodeString(&GdiDriverInfo.DriverName, L"videoprt");
   Status = ZwSetSystemInformation(
      SystemLoadGdiDriverInformation,
      &GdiDriverInfo,
      sizeof(SYSTEM_GDI_DRIVER_INFORMATION));
   if (!NT_SUCCESS(Status))
   {
      DPRINT("Couldn't get our own module handle?\n");
      return NULL;
   }

   BaseAddress = GdiDriverInfo.ImageAddress;

   /* Get the pointer to the export directory */
   ExportDir = (PIMAGE_EXPORT_DIRECTORY)IntVideoPortImageDirectoryEntryToData(
      BaseAddress,
      IMAGE_DIRECTORY_ENTRY_EXPORT);

   /* Search by name */
   AddressPtr = (PULONG)
      ((ULONG_PTR)BaseAddress + (ULONG_PTR)ExportDir->AddressOfFunctions);
   OrdinalPtr = (PUSHORT)
      ((ULONG_PTR)BaseAddress + (ULONG_PTR)ExportDir->AddressOfNameOrdinals);
   NamePtr = (PULONG)
      ((ULONG_PTR)BaseAddress + (ULONG_PTR)ExportDir->AddressOfNames);
   for (i = 0; i < ExportDir->NumberOfNames; i++, NamePtr++, OrdinalPtr++)
   {
      if (!_strnicmp((PCHAR)FunctionName, (PCHAR)((ULONG_PTR)BaseAddress + *NamePtr),
                     strlen((PCHAR)FunctionName)))
      {
         return (PVOID)((ULONG_PTR)BaseAddress +
                        (ULONG_PTR)AddressPtr[*OrdinalPtr]);
      }
   }

   DPRINT("VideoPortGetProcAddress: Can't resolve symbol %s\n", FunctionName);

   return NULL;
}

VOID STDCALL
IntVideoPortDeferredRoutine(
   IN PKDPC Dpc,
   IN PVOID DeferredContext,
   IN PVOID SystemArgument1,
   IN PVOID SystemArgument2)
{
   PVOID HwDeviceExtension =
      &((PVIDEO_PORT_DEVICE_EXTENSION)DeferredContext)->MiniPortDeviceExtension;
   ((PMINIPORT_DPC_ROUTINE)SystemArgument1)(HwDeviceExtension, SystemArgument2);
}

ULONG STDCALL
IntVideoPortAllocateDeviceNumber(VOID)
{
   NTSTATUS Status;
   ULONG DeviceNumber;
   WCHAR SymlinkBuffer[20];
   UNICODE_STRING SymlinkName;

   for (DeviceNumber = 0;;)
   {
      OBJECT_ATTRIBUTES Obj;
      HANDLE ObjHandle;

      swprintf(SymlinkBuffer, L"\\??\\DISPLAY%lu", DeviceNumber + 1);
      RtlInitUnicodeString(&SymlinkName, SymlinkBuffer);
      InitializeObjectAttributes(&Obj, &SymlinkName, 0, NULL, NULL);
      Status = ZwOpenSymbolicLinkObject(&ObjHandle, GENERIC_READ, &Obj);
      if (NT_SUCCESS(Status))
      {
         ZwClose(ObjHandle);
         DeviceNumber++;
         continue;
      }
      else if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
         break;
      else
      {
         DPRINT1("ZwOpenSymbolicLinkObject() returned unexpected status: 0x%08lx\n", Status);
         return 0xFFFFFFFF;
      }
   }

   return DeviceNumber;
}

NTSTATUS STDCALL
IntVideoPortCreateAdapterDeviceObject(
   IN PDRIVER_OBJECT DriverObject,
   IN PVIDEO_PORT_DRIVER_EXTENSION DriverExtension,
   IN PDEVICE_OBJECT PhysicalDeviceObject,
   OUT PDEVICE_OBJECT *DeviceObject  OPTIONAL)
{
   PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;
   ULONG DeviceNumber;
   ULONG Size;
   NTSTATUS Status;
   WCHAR DeviceBuffer[20];
   UNICODE_STRING DeviceName;
   PDEVICE_OBJECT DeviceObject_;

   if (DeviceObject == NULL)
      DeviceObject = &DeviceObject_;

   /*
    * Find the first free device number that can be used for video device
    * object names and symlinks.
    */

   DeviceNumber = IntVideoPortAllocateDeviceNumber();
   if (DeviceNumber == 0xFFFFFFFF)
   {
      DPRINT("Can't find free device number\n");
      return STATUS_UNSUCCESSFUL;
   }

   /*
    * Create the device object.
    */

   /* Create a unicode device name. */
   swprintf(DeviceBuffer, L"\\Device\\Video%lu", DeviceNumber);
   RtlInitUnicodeString(&DeviceName, DeviceBuffer);

   /* Create the device object. */
   Status = IoCreateDevice(
      DriverObject,
      sizeof(VIDEO_PORT_DEVICE_EXTENSION) +
      DriverExtension->InitializationData.HwDeviceExtensionSize,
      &DeviceName,
      FILE_DEVICE_VIDEO,
      0,
      TRUE,
      DeviceObject);

   if (!NT_SUCCESS(Status))
   {
      DPRINT("IoCreateDevice call failed with status 0x%08x\n", Status);
      return Status;
   }

   /*
    * Set the buffering strategy here. If you change this, remember
    * to change VidDispatchDeviceControl too.
    */

   (*DeviceObject)->Flags |= DO_BUFFERED_IO;

   /*
    * Initialize device extension.
    */

   DeviceExtension = (PVIDEO_PORT_DEVICE_EXTENSION)((*DeviceObject)->DeviceExtension);
   DeviceExtension->DeviceNumber = DeviceNumber;
   DeviceExtension->DriverObject = DriverObject;
   DeviceExtension->PhysicalDeviceObject = PhysicalDeviceObject;
   DeviceExtension->FunctionalDeviceObject = *DeviceObject;
   DeviceExtension->DriverExtension = DriverExtension;

   DeviceExtension->RegistryPath.Length =
   DeviceExtension->RegistryPath.MaximumLength =
      DriverExtension->RegistryPath.Length + (9 * sizeof(WCHAR));
   DeviceExtension->RegistryPath.Length -= sizeof(WCHAR);
   DeviceExtension->RegistryPath.Buffer = ExAllocatePoolWithTag(
      NonPagedPool,
      DeviceExtension->RegistryPath.MaximumLength,
      TAG_VIDEO_PORT);
   swprintf(DeviceExtension->RegistryPath.Buffer, L"%s\\Device0",
      DriverExtension->RegistryPath.Buffer);

   if (PhysicalDeviceObject != NULL)
   {
      /* Get bus number from the upper level bus driver. */
      Size = sizeof(ULONG);
      Status = IoGetDeviceProperty(
         PhysicalDeviceObject,
         DevicePropertyBusNumber,
         Size,
         &DeviceExtension->SystemIoBusNumber,
         &Size);
      if (!NT_SUCCESS(Status))
      {
         DPRINT("Couldn't get an information from bus driver. We will try to\n"
                "use legacy detection method, but even that doesn't mean that\n"
                "it will work.\n");
         DeviceExtension->PhysicalDeviceObject = NULL;
      }
   }

   DeviceExtension->AdapterInterfaceType =
      DriverExtension->InitializationData.AdapterInterfaceType;

   if (PhysicalDeviceObject != NULL)
   {
      /* Get bus type from the upper level bus driver. */
      Size = sizeof(ULONG);
      IoGetDeviceProperty(
         PhysicalDeviceObject,
         DevicePropertyLegacyBusType,
         Size,
         &DeviceExtension->AdapterInterfaceType,
         &Size);

      /* Get bus device address from the upper level bus driver. */
      Size = sizeof(ULONG);
      IoGetDeviceProperty(
         PhysicalDeviceObject,
         DevicePropertyAddress,
         Size,
         &DeviceExtension->SystemIoSlotNumber,
         &Size);
   }

   InitializeListHead(&DeviceExtension->AddressMappingListHead);
   KeInitializeDpc(
      &DeviceExtension->DpcObject,
      IntVideoPortDeferredRoutine,
      DeviceExtension);

   KeInitializeMutex(&DeviceExtension->DeviceLock, 0);

   /* Attach the device. */
   if (PhysicalDeviceObject != NULL)
      DeviceExtension->NextDeviceObject = IoAttachDeviceToDeviceStack(
         *DeviceObject, PhysicalDeviceObject);

   return STATUS_SUCCESS;
}


/* FIXME: we have to detach the device object in IntVideoPortFindAdapter if it fails */
NTSTATUS STDCALL
IntVideoPortFindAdapter(
   IN PDRIVER_OBJECT DriverObject,
   IN PVIDEO_PORT_DRIVER_EXTENSION DriverExtension,
   IN PDEVICE_OBJECT DeviceObject)
{
   WCHAR DeviceVideoBuffer[20];
   PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;
   ULONG Size;
   NTSTATUS Status;
   VIDEO_PORT_CONFIG_INFO ConfigInfo;
   SYSTEM_BASIC_INFORMATION SystemBasicInfo;
   UCHAR Again = FALSE;
   WCHAR DeviceBuffer[20];
   UNICODE_STRING DeviceName;
   WCHAR SymlinkBuffer[20];
   UNICODE_STRING SymlinkName;
   BOOL LegacyDetection = FALSE;
   ULONG DeviceNumber;

   DeviceExtension = (PVIDEO_PORT_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
   DeviceNumber = DeviceExtension->DeviceNumber;

   /*
    * Setup a ConfigInfo structure that we will pass to HwFindAdapter.
    */

   RtlZeroMemory(&ConfigInfo, sizeof(VIDEO_PORT_CONFIG_INFO));
   ConfigInfo.Length = sizeof(VIDEO_PORT_CONFIG_INFO);
   ConfigInfo.AdapterInterfaceType = DeviceExtension->AdapterInterfaceType;
   if (ConfigInfo.AdapterInterfaceType == PCIBus)
      ConfigInfo.InterruptMode = LevelSensitive;
   else
      ConfigInfo.InterruptMode = Latched;
   ConfigInfo.DriverRegistryPath = DriverExtension->RegistryPath.Buffer;
   ConfigInfo.VideoPortGetProcAddress = IntVideoPortGetProcAddress;
   ConfigInfo.SystemIoBusNumber = DeviceExtension->SystemIoBusNumber;
   ConfigInfo.BusInterruptLevel = DeviceExtension->InterruptLevel;
   ConfigInfo.BusInterruptVector = DeviceExtension->InterruptVector;

   Size = sizeof(SystemBasicInfo);
   Status = ZwQuerySystemInformation(
      SystemBasicInformation,
      &SystemBasicInfo,
      Size,
      &Size);

   if (NT_SUCCESS(Status))
   {
      ConfigInfo.SystemMemorySize =
         SystemBasicInfo.NumberOfPhysicalPages *
         SystemBasicInfo.PageSize;
   }

   /*
    * Call miniport HwVidFindAdapter entry point to detect if
    * particular device is present. There are two possible code
    * paths. The first one is for Legacy drivers (NT4) and cases
    * when we don't have information about what bus we're on. The
    * second case is the standard one for Plug & Play drivers.
    */
   if (DeviceExtension->PhysicalDeviceObject == NULL)
   {
      LegacyDetection = TRUE;
   }

   if (LegacyDetection)
   {
      ULONG BusNumber, MaxBuses;

      MaxBuses = DeviceExtension->AdapterInterfaceType == PCIBus ? 8 : 1;

      for (BusNumber = 0; BusNumber < MaxBuses; BusNumber++)
      {
         DeviceExtension->SystemIoBusNumber =
         ConfigInfo.SystemIoBusNumber = BusNumber;

         RtlZeroMemory(&DeviceExtension->MiniPortDeviceExtension,
                       DriverExtension->InitializationData.HwDeviceExtensionSize);

         /* FIXME: Need to figure out what string to pass as param 3. */
         Status = DriverExtension->InitializationData.HwFindAdapter(
            &DeviceExtension->MiniPortDeviceExtension,
            DriverExtension->HwContext,
            NULL,
            &ConfigInfo,
            &Again);

         if (Status == ERROR_DEV_NOT_EXIST)
         {
            continue;
         }
         else if (Status == NO_ERROR)
         {
            break;
         }
         else
         {
            DPRINT("HwFindAdapter call failed with error 0x%X\n", Status);
            RtlFreeUnicodeString(&DeviceExtension->RegistryPath);
            IoDeleteDevice(DeviceObject);

            return Status;
         }
      }
   }
   else
   {
      /* FIXME: Need to figure out what string to pass as param 3. */
      Status = DriverExtension->InitializationData.HwFindAdapter(
         &DeviceExtension->MiniPortDeviceExtension,
         DriverExtension->HwContext,
         NULL,
         &ConfigInfo,
         &Again);
   }

   if (Status != NO_ERROR)
   {
      DPRINT("HwFindAdapter call failed with error 0x%X\n", Status);
      RtlFreeUnicodeString(&DeviceExtension->RegistryPath);
      IoDeleteDevice(DeviceObject);
      return Status;
   }

   /*
    * Now we know the device is present, so let's do all additional tasks
    * such as creating symlinks or setting up interrupts and timer.
    */

   /* Create a unicode device name. */
   swprintf(DeviceBuffer, L"\\Device\\Video%lu", DeviceNumber);
   RtlInitUnicodeString(&DeviceName, DeviceBuffer);

   /* Create symbolic link "\??\DISPLAYx" */
   swprintf(SymlinkBuffer, L"\\??\\DISPLAY%lu", DeviceNumber + 1);
   RtlInitUnicodeString(&SymlinkName, SymlinkBuffer);
   IoCreateSymbolicLink(&SymlinkName, &DeviceName);

   /* Add entry to DEVICEMAP\VIDEO key in registry. */
   swprintf(DeviceVideoBuffer, L"\\Device\\Video%d", DeviceNumber);
   RtlWriteRegistryValue(
      RTL_REGISTRY_DEVICEMAP,
      L"VIDEO",
      DeviceVideoBuffer,
      REG_SZ,
      DeviceExtension->RegistryPath.Buffer,
      DeviceExtension->RegistryPath.MaximumLength);

   /* FIXME: Allocate hardware resources for device. */

   /*
    * Allocate interrupt for device.
    */

   if (!IntVideoPortSetupInterrupt(DeviceObject, DriverExtension, &ConfigInfo))
   {
      RtlFreeUnicodeString(&DeviceExtension->RegistryPath);
      IoDeleteDevice(DeviceObject);
      return STATUS_INSUFFICIENT_RESOURCES;
   }

   /*
    * Allocate timer for device.
    */

   if (!IntVideoPortSetupTimer(DeviceObject, DriverExtension))
   {
      if (DeviceExtension->InterruptObject != NULL)
         IoDisconnectInterrupt(DeviceExtension->InterruptObject);
      RtlFreeUnicodeString(&DeviceExtension->RegistryPath);
      IoDeleteDevice(DeviceObject);
      DPRINT("STATUS_INSUFFICIENT_RESOURCES\n");
      return STATUS_INSUFFICIENT_RESOURCES;
   }

   /*
    * Query children of the device.
    */
   VideoPortEnumerateChildren(&DeviceExtension->MiniPortDeviceExtension, NULL);

   DPRINT("STATUS_SUCCESS\n");
   return STATUS_SUCCESS;
}

VOID FASTCALL
IntAttachToCSRSS(PKPROCESS *CallingProcess, PKAPC_STATE ApcState)
{
   *CallingProcess = (PKPROCESS)PsGetCurrentProcess();
   if (*CallingProcess != Csrss)
   {
      KeStackAttachProcess(Csrss, ApcState);
   }
}

VOID FASTCALL
IntDetachFromCSRSS(PKPROCESS *CallingProcess, PKAPC_STATE ApcState)
{
   if (*CallingProcess != Csrss)
   {
      KeUnstackDetachProcess(ApcState);
   }
}

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @implemented
 */

ULONG STDCALL
VideoPortInitialize(
   IN PVOID Context1,
   IN PVOID Context2,
   IN PVIDEO_HW_INITIALIZATION_DATA HwInitializationData,
   IN PVOID HwContext)
{
   PDRIVER_OBJECT DriverObject = Context1;
   PUNICODE_STRING RegistryPath = Context2;
   NTSTATUS Status;
   PVIDEO_PORT_DRIVER_EXTENSION DriverExtension;
   BOOL LegacyDetection = FALSE;

   DPRINT("VideoPortInitialize\n");

   /*
    * NOTE:
    * The driver extension can be already allocated in case that we were
    * called by legacy driver and failed detecting device. Some miniport
    * drivers in that case adjust parameters and calls VideoPortInitialize
    * again.
    */

   DriverExtension = IoGetDriverObjectExtension(DriverObject, DriverObject);
   if (DriverExtension == NULL)
   {
      Status = IoAllocateDriverObjectExtension(
         DriverObject,
         DriverObject,
         sizeof(VIDEO_PORT_DRIVER_EXTENSION),
         (PVOID *)&DriverExtension);

      if (!NT_SUCCESS(Status))
      {
         return Status;
      }
   }

   /*
    * Copy the correct miniport initializtation data to the device extension.
    */

   RtlCopyMemory(
      &DriverExtension->InitializationData,
      HwInitializationData,
      min(sizeof(VIDEO_HW_INITIALIZATION_DATA),
          HwInitializationData->HwInitDataSize));
   if (sizeof(VIDEO_HW_INITIALIZATION_DATA) > HwInitializationData->HwInitDataSize)
   {
      RtlZeroMemory((PVOID)((ULONG_PTR)&DriverExtension->InitializationData +
                                       HwInitializationData->HwInitDataSize),
                    sizeof(VIDEO_HW_INITIALIZATION_DATA) -
                    HwInitializationData->HwInitDataSize);
   }
   DriverExtension->HwContext = HwContext;

   /* we can't use RtlDuplicateUnicodeString because only ntdll exposes it... */
   if (RegistryPath->Length != 0)
   {
      DriverExtension->RegistryPath.Length = 0;
      DriverExtension->RegistryPath.MaximumLength = RegistryPath->Length + sizeof(UNICODE_NULL);
      DriverExtension->RegistryPath.Buffer = ExAllocatePoolWithTag(PagedPool,
                                                                   DriverExtension->RegistryPath.MaximumLength,
                                                                   TAG('U', 'S', 'T', 'R'));
      if (DriverExtension->RegistryPath.Buffer == NULL)
      {
         RtlInitUnicodeString(&DriverExtension->RegistryPath, NULL);
         return STATUS_INSUFFICIENT_RESOURCES;
      }

      RtlCopyUnicodeString(&DriverExtension->RegistryPath, RegistryPath);
   }
   else
   {
      RtlInitUnicodeString(&DriverExtension->RegistryPath, NULL);
   }

   switch (HwInitializationData->HwInitDataSize)
   {
      /*
       * NT4 drivers are special case, because we must use legacy method
       * of detection instead of the Plug & Play one.
       */

      case SIZE_OF_NT4_VIDEO_HW_INITIALIZATION_DATA:
         DPRINT("We were loaded by a Windows NT miniport driver.\n");
         LegacyDetection = TRUE;
         break;

      case SIZE_OF_W2K_VIDEO_HW_INITIALIZATION_DATA:
         DPRINT("We were loaded by a Windows 2000 miniport driver.\n");
         break;

      case sizeof(VIDEO_HW_INITIALIZATION_DATA):
         DPRINT("We were loaded by a Windows XP or later miniport driver.\n");
         break;

      default:
         DPRINT("Invalid HwInitializationData size.\n");
         return STATUS_UNSUCCESSFUL;
   }

   DriverObject->MajorFunction[IRP_MJ_CREATE] = IntVideoPortDispatchOpen;
   DriverObject->MajorFunction[IRP_MJ_CLOSE] = IntVideoPortDispatchClose;
   DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = IntVideoPortDispatchDeviceControl;
   DriverObject->DriverUnload = IntVideoPortUnload;

   /*
    * Plug & Play drivers registers the device in AddDevice routine. For
    * legacy drivers we must do it now.
    */

   if (LegacyDetection)
   {
      PDEVICE_OBJECT DeviceObject;
      Status = IntVideoPortCreateAdapterDeviceObject(DriverObject, DriverExtension,
                                                     NULL, &DeviceObject);
      DPRINT("IntVideoPortCreateAdapterDeviceObject returned 0x%x\n", Status);
      if (!NT_SUCCESS(Status))
         return Status;
      Status = IntVideoPortFindAdapter(DriverObject, DriverExtension, DeviceObject);
      DPRINT("IntVideoPortFindAdapter returned 0x%x\n", Status);
      return Status;
   }
   else
   {
      DriverObject->DriverExtension->AddDevice = IntVideoPortAddDevice;
      DriverObject->MajorFunction[IRP_MJ_PNP] = IntVideoPortDispatchPnp;
      DriverObject->MajorFunction[IRP_MJ_POWER] = IntVideoPortDispatchPower;

      return STATUS_SUCCESS;
   }
}

/*
 * @implemented
 */

VOID
VideoPortDebugPrint(
   IN VIDEO_DEBUG_LEVEL DebugPrintLevel,
   IN PCHAR DebugMessage, ...)
{
   char Buffer[256];
   va_list ap;

   va_start(ap, DebugMessage);
   vsprintf(Buffer, DebugMessage, ap);
   va_end(ap);

   DbgPrint(Buffer);
}

/*
 * @unimplemented
 */

VOID STDCALL
VideoPortLogError(
   IN PVOID HwDeviceExtension,
   IN PVIDEO_REQUEST_PACKET Vrp OPTIONAL,
   IN VP_STATUS ErrorCode,
   IN ULONG UniqueId)
{
   DPRINT1("VideoPortLogError ErrorCode %d (0x%x) UniqueId %lu (0x%lx)\n",
           ErrorCode, ErrorCode, UniqueId, UniqueId);
   if (NULL != Vrp)
   {
      DPRINT1("Vrp->IoControlCode %lu (0x%lx)\n", Vrp->IoControlCode, Vrp->IoControlCode);
   }
}

/*
 * @implemented
 */

UCHAR STDCALL
VideoPortGetCurrentIrql(VOID)
{
   return KeGetCurrentIrql();
}

typedef struct QueryRegistryCallbackContext
{
   PVOID HwDeviceExtension;
   PVOID HwContext;
   PMINIPORT_GET_REGISTRY_ROUTINE HwGetRegistryRoutine;
} QUERY_REGISTRY_CALLBACK_CONTEXT, *PQUERY_REGISTRY_CALLBACK_CONTEXT;

static NTSTATUS STDCALL
QueryRegistryCallback(
   IN PWSTR ValueName,
   IN ULONG ValueType,
   IN PVOID ValueData,
   IN ULONG ValueLength,
   IN PVOID Context,
   IN PVOID EntryContext)
{
   PQUERY_REGISTRY_CALLBACK_CONTEXT CallbackContext = (PQUERY_REGISTRY_CALLBACK_CONTEXT) Context;

   DPRINT("Found registry value for name %S: type %d, length %d\n",
          ValueName, ValueType, ValueLength);
   return (*(CallbackContext->HwGetRegistryRoutine))(
      CallbackContext->HwDeviceExtension,
      CallbackContext->HwContext,
      ValueName,
      ValueData,
      ValueLength);
}

/*
 * @unimplemented
 */

VP_STATUS STDCALL
VideoPortGetRegistryParameters(
   IN PVOID HwDeviceExtension,
   IN PWSTR ParameterName,
   IN UCHAR IsParameterFileName,
   IN PMINIPORT_GET_REGISTRY_ROUTINE GetRegistryRoutine,
   IN PVOID HwContext)
{
   RTL_QUERY_REGISTRY_TABLE QueryTable[2];
   QUERY_REGISTRY_CALLBACK_CONTEXT Context;
   PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;

   DPRINT("VideoPortGetRegistryParameters ParameterName %S\n", ParameterName);

   DeviceExtension = VIDEO_PORT_GET_DEVICE_EXTENSION(HwDeviceExtension);

   if (IsParameterFileName)
   {
      UNIMPLEMENTED;
   }

   Context.HwDeviceExtension = HwDeviceExtension;
   Context.HwContext = HwContext;
   Context.HwGetRegistryRoutine = GetRegistryRoutine;

   QueryTable[0].QueryRoutine = QueryRegistryCallback;
   QueryTable[0].Flags = RTL_QUERY_REGISTRY_REQUIRED;
   QueryTable[0].Name = ParameterName;
   QueryTable[0].EntryContext = NULL;
   QueryTable[0].DefaultType = REG_NONE;
   QueryTable[0].DefaultData = NULL;
   QueryTable[0].DefaultLength = 0;

   QueryTable[1].QueryRoutine = NULL;
   QueryTable[1].Name = NULL;

   return NT_SUCCESS(RtlQueryRegistryValues(
      RTL_REGISTRY_ABSOLUTE,
      DeviceExtension->RegistryPath.Buffer,
      QueryTable,
      &Context,
      NULL)) ? ERROR_SUCCESS : ERROR_INVALID_PARAMETER;
}

/*
 * @implemented
 */

VP_STATUS STDCALL
VideoPortSetRegistryParameters(
   IN PVOID HwDeviceExtension,
   IN PWSTR ValueName,
   IN PVOID ValueData,
   IN ULONG ValueLength)
{
   DPRINT("VideoPortSetRegistryParameters\n");
   ASSERT_IRQL(PASSIVE_LEVEL);
   return RtlWriteRegistryValue(
      RTL_REGISTRY_ABSOLUTE,
      VIDEO_PORT_GET_DEVICE_EXTENSION(HwDeviceExtension)->RegistryPath.Buffer,
      ValueName,
      REG_BINARY,
      ValueData,
      ValueLength);
}

/*
 * @implemented
 */

VP_STATUS STDCALL
VideoPortGetVgaStatus(
   IN PVOID HwDeviceExtension,
   OUT PULONG VgaStatus)
{
   PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;

   DPRINT("VideoPortGetVgaStatus\n");

   DeviceExtension = VIDEO_PORT_GET_DEVICE_EXTENSION(HwDeviceExtension);
   if (KeGetCurrentIrql() == PASSIVE_LEVEL)
   {
      if (DeviceExtension->AdapterInterfaceType == PCIBus)
      {
         /* VgaStatus: 0 == VGA not enabled, 1 == VGA enabled. */
         /* Assumed for now */
         *VgaStatus = 1;
         return NO_ERROR;
      }
   }

   return ERROR_INVALID_FUNCTION;
}

/*
 * @implemented
 */

PVOID STDCALL
VideoPortGetRomImage(
   IN PVOID HwDeviceExtension,
   IN PVOID Unused1,
   IN ULONG Unused2,
   IN ULONG Length)
{
   static PVOID RomImageBuffer = NULL;
   PKPROCESS CallingProcess;
   KAPC_STATE ApcState;

   DPRINT("VideoPortGetRomImage(HwDeviceExtension 0x%X Length 0x%X)\n",
          HwDeviceExtension, Length);

   /* If the length is zero then free the existing buffer. */
   if (Length == 0)
   {
      if (RomImageBuffer != NULL)
      {
         ExFreePool(RomImageBuffer);
         RomImageBuffer = NULL;
      }
      return NULL;
   }
   else
   {
      /*
       * The DDK says we shouldn't use the legacy C0000 method but get the
       * rom base address from the corresponding pci or acpi register but
       * lets ignore that and use C0000 anyway. We have already mapped the
       * bios area into memory so we'll copy from there.
       */

      /* Copy the bios. */
      Length = min(Length, 0x10000);
      if (RomImageBuffer != NULL)
      {
         ExFreePool(RomImageBuffer);
      }

      RomImageBuffer = ExAllocatePool(PagedPool, Length);
      if (RomImageBuffer == NULL)
      {
         return NULL;
      }

      IntAttachToCSRSS(&CallingProcess, &ApcState);
      RtlCopyMemory(RomImageBuffer, (PUCHAR)0xC0000, Length);
      IntDetachFromCSRSS(&CallingProcess, &ApcState);

      return RomImageBuffer;
   }
}

/*
 * @implemented
 */

BOOLEAN STDCALL
VideoPortScanRom(
   IN PVOID HwDeviceExtension,
   IN PUCHAR RomBase,
   IN ULONG RomLength,
   IN PUCHAR String)
{
   ULONG StringLength;
   BOOLEAN Found;
   PUCHAR SearchLocation;

   DPRINT("VideoPortScanRom RomBase %p RomLength 0x%x String %s\n", RomBase, RomLength, String);

   StringLength = strlen((PCHAR)String);
   Found = FALSE;
   SearchLocation = RomBase;
   for (SearchLocation = RomBase;
        !Found && SearchLocation < RomBase + RomLength - StringLength;
        SearchLocation++)
   {
      Found = (RtlCompareMemory(SearchLocation, String, StringLength) == StringLength);
      if (Found)
      {
         DPRINT("Match found at %p\n", SearchLocation);
      }
   }

   return Found;
}

/*
 * @implemented
 */

BOOLEAN STDCALL
VideoPortSynchronizeExecution(
   IN PVOID HwDeviceExtension,
   IN VIDEO_SYNCHRONIZE_PRIORITY Priority,
   IN PMINIPORT_SYNCHRONIZE_ROUTINE SynchronizeRoutine,
   OUT PVOID Context)
{
   BOOLEAN Ret;
   PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;
   KIRQL OldIrql;

   switch (Priority)
   {
      case VpLowPriority:
         Ret = (*SynchronizeRoutine)(Context);
         break;

      case VpMediumPriority:
         DeviceExtension = VIDEO_PORT_GET_DEVICE_EXTENSION(HwDeviceExtension);
         if (DeviceExtension->InterruptObject == NULL)
            Ret = (*SynchronizeRoutine)(Context);
         else
            Ret = KeSynchronizeExecution(
               DeviceExtension->InterruptObject,
               SynchronizeRoutine,
               Context);
         break;

      case VpHighPriority:
         OldIrql = KeGetCurrentIrql();
         if (OldIrql < SYNCH_LEVEL)
            OldIrql = KfRaiseIrql(SYNCH_LEVEL);

         Ret = (*SynchronizeRoutine)(Context);

         if (OldIrql < SYNCH_LEVEL)
            KfLowerIrql(OldIrql);
         break;

      default:
         Ret = FALSE;
   }

   return Ret;
}

/*
 * @implemented
 */

VP_STATUS STDCALL
VideoPortEnumerateChildren(
   IN PVOID HwDeviceExtension,
   IN PVOID Reserved)
{
   PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;
   ULONG Status;
   VIDEO_CHILD_ENUM_INFO ChildEnumInfo;
   VIDEO_CHILD_TYPE ChildType;
   UCHAR ChildDescriptor[256];
   ULONG ChildId;
   ULONG Unused;
   INT i;

   DeviceExtension = VIDEO_PORT_GET_DEVICE_EXTENSION(HwDeviceExtension);
   if (DeviceExtension->DriverExtension->InitializationData.HwGetVideoChildDescriptor == NULL)
   {
      DPRINT("Miniport's HwGetVideoChildDescriptor is NULL!\n");
      return NO_ERROR;
   }

   /* Setup the ChildEnumInfo */
   ChildEnumInfo.Size = sizeof (ChildEnumInfo);
   ChildEnumInfo.ChildDescriptorSize = sizeof (ChildDescriptor);
   ChildEnumInfo.ACPIHwId = 0;
   ChildEnumInfo.ChildHwDeviceExtension = NULL; /* FIXME: must be set to
                                                   ChildHwDeviceExtension... */

   /* Enumerate the children */
   for (i = 1; ; i++)
   {
      ChildEnumInfo.ChildIndex = i;
      RtlZeroMemory(ChildDescriptor, sizeof(ChildDescriptor));
      Status = DeviceExtension->DriverExtension->InitializationData.HwGetVideoChildDescriptor(
                  HwDeviceExtension,
                  &ChildEnumInfo,
                  &ChildType,
                  ChildDescriptor,
                  &ChildId,
                  &Unused);
      if (Status == VIDEO_ENUM_INVALID_DEVICE)
      {
         DPRINT("Child device %d is invalid!\n", ChildEnumInfo.ChildIndex);
         continue;
      }
      else if (Status == VIDEO_ENUM_NO_MORE_DEVICES)
      {
         DPRINT("End of child enumeration! (%d children enumerated)\n", i - 1);
         break;
      }
      else if (Status != VIDEO_ENUM_MORE_DEVICES)
      {
         DPRINT("HwGetVideoChildDescriptor returned unknown status code 0x%x!\n", Status);
         break;
      }

#ifndef NDEBUG
      if (ChildType == Monitor)
      {
         INT j;
         PUCHAR p = ChildDescriptor;
         DPRINT("Monitor device enumerated! (ChildId = 0x%x)\n", ChildId);
         for (j = 0; j < sizeof (ChildDescriptor); j += 8)
         {
            DPRINT("%02x %02x %02x %02x %02x %02x %02x %02x\n",
                   p[j+0], p[j+1], p[j+2], p[j+3],
                   p[j+4], p[j+5], p[j+6], p[j+7]);
         }
      }
      else if (ChildType == Other)
      {
         DPRINT("\"Other\" device enumerated: DeviceId = %S\n", (PWSTR)ChildDescriptor);
      }
      else
      {
         DPRINT("HwGetVideoChildDescriptor returned unsupported type: %d\n", ChildType);
      }
#endif /* NDEBUG */

   }

   return NO_ERROR;
}

/*
 * @unimplemented
 */

VP_STATUS STDCALL
VideoPortCreateSecondaryDisplay(
   IN PVOID HwDeviceExtension,
   IN OUT PVOID *SecondaryDeviceExtension,
   IN ULONG Flag)
{
   DPRINT1("VideoPortCreateSecondaryDisplay: Unimplemented.\n");
   return NO_ERROR;
}

/*
 * @implemented
 */

BOOLEAN STDCALL
VideoPortQueueDpc(
   IN PVOID HwDeviceExtension,
   IN PMINIPORT_DPC_ROUTINE CallbackRoutine,
   IN PVOID Context)
{
   return KeInsertQueueDpc(
      &VIDEO_PORT_GET_DEVICE_EXTENSION(HwDeviceExtension)->DpcObject,
      (PVOID)CallbackRoutine,
      (PVOID)Context);
}

/*
 * @unimplemented
 */

PVOID STDCALL
VideoPortGetAssociatedDeviceExtension(IN PVOID DeviceObject)
{
   DPRINT1("VideoPortGetAssociatedDeviceExtension: Unimplemented.\n");
   return NULL;
}

/*
 * @implemented
 */

VP_STATUS STDCALL
VideoPortGetVersion(
   IN PVOID HwDeviceExtension,
   IN OUT PVPOSVERSIONINFO VpOsVersionInfo)
{
   RTL_OSVERSIONINFOEXW Version;

   Version.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOEXW);
   if (VpOsVersionInfo->Size >= sizeof(VPOSVERSIONINFO))
   {
#if 1
      if (NT_SUCCESS(RtlGetVersion((PRTL_OSVERSIONINFOW)&Version)))
      {
         VpOsVersionInfo->MajorVersion = Version.dwMajorVersion;
         VpOsVersionInfo->MinorVersion = Version.dwMinorVersion;
         VpOsVersionInfo->BuildNumber = Version.dwBuildNumber;
         VpOsVersionInfo->ServicePackMajor = Version.wServicePackMajor;
         VpOsVersionInfo->ServicePackMinor = Version.wServicePackMinor;
         return NO_ERROR;
      }
      return ERROR_INVALID_PARAMETER;
#else
      VpOsVersionInfo->MajorVersion = 5;
      VpOsVersionInfo->MinorVersion = 0;
      VpOsVersionInfo->BuildNumber = 2195;
      VpOsVersionInfo->ServicePackMajor = 4;
      VpOsVersionInfo->ServicePackMinor = 0;
      return NO_ERROR;
#endif
   }

   return ERROR_INVALID_PARAMETER;
}

/*
 * @unimplemented
 */

BOOLEAN STDCALL
VideoPortCheckForDeviceExistence(
   IN PVOID HwDeviceExtension,
   IN USHORT VendorId,
   IN USHORT DeviceId,
   IN UCHAR RevisionId,
   IN USHORT SubVendorId,
   IN USHORT SubSystemId,
   IN ULONG Flags)
{
   DPRINT1("VideoPortCheckForDeviceExistence: Unimplemented.\n");
   return TRUE;
}

/*
 * @unimplemented
 */

VP_STATUS STDCALL
VideoPortRegisterBugcheckCallback(
   IN PVOID HwDeviceExtension,
   IN ULONG BugcheckCode,
   IN PVOID Callback,
   IN ULONG BugcheckDataSize)
{
   DPRINT1("VideoPortRegisterBugcheckCallback(): Unimplemented.\n");
   return NO_ERROR;
}

/*
 * @implemented
 */

LONGLONG STDCALL
VideoPortQueryPerformanceCounter(
   IN PVOID HwDeviceExtension,
   OUT PLONGLONG PerformanceFrequency OPTIONAL)
{
   LARGE_INTEGER Result;

   DPRINT("VideoPortQueryPerformanceCounter\n");
   Result = KeQueryPerformanceCounter((PLARGE_INTEGER)PerformanceFrequency);
   return Result.QuadPart;
}

/*
 * @implemented
 */

VOID STDCALL
VideoPortAcquireDeviceLock(
   IN PVOID  HwDeviceExtension)
{
   PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;
   NTSTATUS Status;
   (void)Status;

   DPRINT("VideoPortAcquireDeviceLock\n");
   DeviceExtension = VIDEO_PORT_GET_DEVICE_EXTENSION(HwDeviceExtension);
   Status = KeWaitForMutexObject(&DeviceExtension->DeviceLock, Executive,
                                 KernelMode, FALSE, NULL);
   ASSERT(Status == STATUS_SUCCESS);
}

/*
 * @implemented
 */

VOID STDCALL
VideoPortReleaseDeviceLock(
   IN PVOID  HwDeviceExtension)
{
   PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;
   LONG Status;
   (void)Status;

   DPRINT("VideoPortReleaseDeviceLock\n");
   DeviceExtension = VIDEO_PORT_GET_DEVICE_EXTENSION(HwDeviceExtension);
   Status = KeReleaseMutex(&DeviceExtension->DeviceLock, FALSE);
   ASSERT(Status == 0);
}

