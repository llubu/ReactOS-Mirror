/* $Id: pci.h,v 1.8 2004/08/16 09:13:00 ekohl Exp $ */

#ifndef __PCI_H
#define __PCI_H


typedef enum {
  pbtUnknown = 0,
  pbtType1,
  pbtType2,
} PCI_BUS_TYPE;


typedef struct _PCI_DEVICE
{
  // Entry on device list
  LIST_ENTRY ListEntry;
  // Physical Device Object of device
  PDEVICE_OBJECT Pdo;
  // PCI bus number
  ULONG BusNumber;
  // PCI slot number
  PCI_SLOT_NUMBER SlotNumber;
  // PCI configuration data
  PCI_COMMON_CONFIG PciConfig;
  // Flag used during enumeration to locate removed devices
  BOOLEAN RemovePending;
} PCI_DEVICE, *PPCI_DEVICE;


typedef enum {
  dsStopped,
  dsStarted,
  dsPaused,
  dsRemoved,
  dsSurpriseRemoved
} PCI_DEVICE_STATE;


typedef struct _COMMON_DEVICE_EXTENSION
{
  // Pointer to device object, this device extension is associated with
  PDEVICE_OBJECT DeviceObject;
  // Wether this device extension is for an FDO or PDO
  BOOLEAN IsFDO;
  // Wether the device is removed
  BOOLEAN Removed;
  // Current device power state for the device
  DEVICE_POWER_STATE DevicePowerState;
} __attribute((packed)) COMMON_DEVICE_EXTENSION, *PCOMMON_DEVICE_EXTENSION;

/* Physical Device Object device extension for a child device */
typedef struct _PDO_DEVICE_EXTENSION
{
  // Common device data
  COMMON_DEVICE_EXTENSION Common;
  // Functional device object
  PDEVICE_OBJECT Fdo;
  // PCI bus number
  ULONG BusNumber;
  // PCI slot number
  PCI_SLOT_NUMBER SlotNumber;
  // Device ID
  UNICODE_STRING DeviceID;
  // Instance ID
  UNICODE_STRING InstanceID;
  // Hardware IDs
  UNICODE_STRING HardwareIDs;
  // Compatible IDs
  UNICODE_STRING CompatibleIDs;
  // Textual description of device
  UNICODE_STRING DeviceDescription;
  // Textual description of device location
  UNICODE_STRING DeviceLocation;
} __attribute((packed)) PDO_DEVICE_EXTENSION, *PPDO_DEVICE_EXTENSION;

/* Functional Device Object device extension for the PCI driver device object */
typedef struct _FDO_DEVICE_EXTENSION
{
  // Common device data
  COMMON_DEVICE_EXTENSION Common;
  // Current state of the driver
  PCI_DEVICE_STATE State;
  // Namespace device list
  LIST_ENTRY DeviceListHead;
  // Number of (not removed) devices in device list
  ULONG DeviceListCount;
  // Lock for namespace device list
  KSPIN_LOCK DeviceListLock;
  // Lower device object
  PDEVICE_OBJECT Ldo;
} __attribute((packed)) FDO_DEVICE_EXTENSION, *PFDO_DEVICE_EXTENSION;


/* fdo.c */

NTSTATUS
FdoPnpControl(
  PDEVICE_OBJECT DeviceObject,
  PIRP Irp);

NTSTATUS
FdoPowerControl(
  PDEVICE_OBJECT DeviceObject,
  PIRP Irp);

/* pci.c */

BOOLEAN
PciCreateUnicodeString(
  PUNICODE_STRING Destination,
  PWSTR Source,
  POOL_TYPE PoolType);

NTSTATUS
PciDuplicateUnicodeString(
  PUNICODE_STRING Destination,
  PUNICODE_STRING Source,
  POOL_TYPE PoolType);

BOOLEAN
PciCreateDeviceIDString(
  PUNICODE_STRING DeviceID,
  PPCI_DEVICE Device);

BOOLEAN
PciCreateInstanceIDString(
  PUNICODE_STRING InstanceID,
  PPCI_DEVICE Device);

BOOLEAN
PciCreateHardwareIDsString(
  PUNICODE_STRING HardwareIDs,
  PPCI_DEVICE Device);

BOOLEAN
PciCreateCompatibleIDsString(
  PUNICODE_STRING HardwareIDs,
  PPCI_DEVICE Device);

BOOLEAN
PciCreateDeviceDescriptionString(
  PUNICODE_STRING DeviceDescription,
  PPCI_DEVICE Device);

BOOLEAN
PciCreateDeviceLocationString(
  PUNICODE_STRING DeviceLocation,
  PPCI_DEVICE Device);

/* pdo.c */

NTSTATUS
PdoPnpControl(
  PDEVICE_OBJECT DeviceObject,
  PIRP Irp);

NTSTATUS
PdoPowerControl(
  PDEVICE_OBJECT DeviceObject,
  PIRP Irp);

#endif  /*  __PCI_H  */
