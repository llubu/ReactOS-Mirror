/*
 *  ReactOS kernel
 *  Copyright (C) 2000 David Welch <welch@cwcom.net>
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
/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            include/internal/io.h
 * PURPOSE:         Internal io manager declarations
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *               28/05/97: Created
 */

#ifndef __NTOSKRNL_INCLUDE_INTERNAL_IO_H
#define __NTOSKRNL_INCLUDE_INTERNAL_IO_H

#include <ddk/ntddk.h>
#include <internal/ob.h>
#include <internal/module.h>


#ifndef __USE_W32API
#define DEVICE_TYPE_FROM_CTL_CODE(ctlCode) (((ULONG)(ctlCode&0xffff0000))>>16)
#endif

#define IO_METHOD_FROM_CTL_CODE(ctlCode) (ctlCode&0x00000003)

struct _DEVICE_OBJECT_POWER_EXTENSION;

typedef struct _IO_COMPLETION_PACKET{
   PVOID             Key;
   PVOID             Context;
   IO_STATUS_BLOCK   IoStatus;
   LIST_ENTRY        ListEntry;
} IO_COMPLETION_PACKET, *PIO_COMPLETION_PACKET;

typedef struct _DEVOBJ_EXTENSION {
   CSHORT Type;
   USHORT Size;
   PDEVICE_OBJECT DeviceObject;
   ULONG PowerFlags;
   struct DEVICE_OBJECT_POWER_EXTENSION *Dope;
   ULONG ExtensionFlags;
   struct _DEVICE_NODE *DeviceNode;
   PDEVICE_OBJECT AttachedTo;
   LONG StartIoCount;
   LONG StartIoKey;
   ULONG StartIoFlags;
   struct _VPB *Vpb;
} DEVOBJ_EXTENSION, *PDEVOBJ_EXTENSION;
   
typedef struct _PRIVATE_DRIVER_EXTENSIONS {
   struct _PRIVATE_DRIVER_EXTENSIONS *Link;
   PVOID ClientIdentificationAddress;
   CHAR Extension[1];
} PRIVATE_DRIVER_EXTENSIONS, *PPRIVATE_DRIVER_EXTENSIONS;

typedef struct _DEVICE_NODE
{
  /* A tree structure. */
  struct _DEVICE_NODE *Parent;
  struct _DEVICE_NODE *PrevSibling;
  struct _DEVICE_NODE *NextSibling;
  struct _DEVICE_NODE *Child;
  /* The level of deepness in the tree. */
  UINT Level;
  /* */
//  PPO_DEVICE_NOTIFY Notify;
  /* State machine. */
//  PNP_DEVNODE_STATE State;
//  PNP_DEVNODE_STATE PreviousState;
//  PNP_DEVNODE_STATE StateHistory[20];
//  UINT StateHistoryEntry;
  /* ? */
  INT CompletionStatus;
  /* ? */
  PIRP PendingIrp;
  /* See DNF_* flags below (WinDBG documentation has WRONG values) */
  ULONG Flags;
  /* See DNUF_* flags below (and IRP_MN_QUERY_PNP_DEVICE_STATE) */
  ULONG UserFlags;
  /* See CM_PROB_* values are defined in cfg.h */
  ULONG Problem;
  /* Pointer to the PDO corresponding to the device node. */
  PDEVICE_OBJECT PhysicalDeviceObject;
  /* Resource list as assigned by the PnP arbiter. See IRP_MN_START_DEVICE
     and ARBITER_INTERFACE (not documented in DDK, but present in headers). */
  PCM_RESOURCE_LIST ResourceList;
  /* Resource list as assigned by the PnP arbiter (translated version). */
  PCM_RESOURCE_LIST ResourceListTranslated;
  /* Instance path relative to the Enum key in registry. */
  UNICODE_STRING InstancePath;
  /* Name of the driver service. */
  UNICODE_STRING ServiceName;
  /* ? */
  PDEVICE_OBJECT DuplicatePDO;
  /* See IRP_MN_QUERY_RESOURCE_REQUIREMENTS. */
  PIO_RESOURCE_REQUIREMENTS_LIST ResourceRequirements;
  /* Information about bus for bus drivers. */
  INTERFACE_TYPE InterfaceType;
  ULONG BusNumber;
  /* Information about underlying bus for child devices. */
  INTERFACE_TYPE ChildInterfaceType;
  ULONG ChildBusNumber;
  USHORT ChildBusTypeIndex;
  /* ? */
  UCHAR RemovalPolicy;
  UCHAR HardwareRemovalPolicy;
  LIST_ENTRY TargetDeviceNotify;
  LIST_ENTRY DeviceArbiterList;
  LIST_ENTRY DeviceTranslatorList;
  USHORT NoTranslatorMask;
  USHORT QueryTranslatorMask;
  USHORT NoArbiterMask;
  USHORT QueryArbiterMask;
  union {
    struct _DEVICE_NODE *LegacyDeviceNode;
    PDEVICE_RELATIONS PendingDeviceRelations;
  } OverUsed1;
  union {
    struct _DEVICE_NODE *NextResourceDeviceNode;
  } OverUsed2;
  /* See IRP_MN_QUERY_RESOURCES/IRP_MN_FILTER_RESOURCES. */
  PCM_RESOURCE_LIST BootResources;
  /* See the bitfields in DEVICE_CAPABILITIES structure. */
  ULONG CapabilityFlags;
  struct
  {
    ULONG DockStatus;
    LIST_ENTRY ListEntry;
    WCHAR *SerialNumber;
  } DockInfo;
  ULONG DisableableDepends;
  LIST_ENTRY PendedSetInterfaceState;
  LIST_ENTRY LegacyBusListEntry;
  ULONG DriverUnloadRetryCount;

  /* Not NT's */
  GUID BusTypeGuid;
  ULONG Address;
} DEVICE_NODE, *PDEVICE_NODE;

/* For Flags field */
#define DNF_PROCESSED                           0x00000001
#define DNF_STARTED                             0x00000002
#define DNF_START_FAILED                        0x00000004
#define DNF_ENUMERATED                          0x00000008
#define DNF_DELETED                             0x00000010
#define DNF_MADEUP                              0x00000020
#define DNF_START_REQUEST_PENDING               0x00000040
#define DNF_NO_RESOURCE_REQUIRED                0x00000080
#define DNF_INSUFFICIENT_RESOURCES              0x00000100
#define DNF_RESOURCE_ASSIGNED                   0x00000200
#define DNF_RESOURCE_REPORTED                   0x00000400
#define DNF_HAL_NODE                            0x00000800 // ???
#define DNF_ADDED                               0x00001000
#define DNF_ADD_FAILED                          0x00002000
#define DNF_LEGACY_DRIVER                       0x00004000
#define DNF_STOPPED                             0x00008000
#define DNF_WILL_BE_REMOVED                     0x00010000
#define DNF_NEED_TO_ENUM                        0x00020000
#define DNF_NOT_CONFIGURED                      0x00040000
#define DNF_REINSTALL                           0x00080000
#define DNF_RESOURCE_REQUIREMENTS_NEED_FILTERED 0x00100000 // ???
#define DNF_DISABLED                            0x00200000
#define DNF_RESTART_OK                          0x00400000
#define DNF_NEED_RESTART                        0x00800000
#define DNF_VISITED                             0x01000000
#define DNF_ASSIGNING_RESOURCES                 0x02000000
#define DNF_BEEING_ENUMERATED                   0x04000000
#define DNF_NEED_ENUMERATION_ONLY               0x08000000
#define DNF_LOCKED                              0x10000000
#define DNF_HAS_BOOT_CONFIG                     0x20000000
#define DNF_BOOT_CONFIG_RESERVED                0x40000000
#define DNF_HAS_PROBLEM                         0x80000000 // ???

/* For UserFlags field */
#define DNUF_DONT_SHOW_IN_UI    0x0002
#define DNUF_NOT_DISABLEABLE    0x0008

/* For Problem field */
#define CM_PROB_NOT_CONFIGURED  1
#define CM_PROB_FAILED_START    10
#define CM_PROB_NORMAL_CONFLICT 12
#define CM_PROB_NEED_RESTART    14
#define CM_PROB_REINSTALL       18
#define CM_PROB_WILL_BE_REMOVED 21
#define CM_PROB_DISABLED        22
#define CM_PROB_FAILED_INSTALL  28
#define CM_PROB_FAILED_ADD      31

/*
 * VOID
 * IopDeviceNodeSetFlag(
 *   PDEVICE_NODE DeviceNode,
 *   ULONG Flag);
 */
#define IopDeviceNodeSetFlag(DeviceNode, Flag)((DeviceNode)->Flags |= (Flag))

/*
 * VOID
 * IopDeviceNodeClearFlag(
 *   PDEVICE_NODE DeviceNode,
 *   ULONG Flag);
 */
#define IopDeviceNodeClearFlag(DeviceNode, Flag)((DeviceNode)->Flags &= ~(Flag))

/*
 * BOOLEAN
 * IopDeviceNodeHasFlag(
 *   PDEVICE_NODE DeviceNode,
 *   ULONG Flag);
 */
#define IopDeviceNodeHasFlag(DeviceNode, Flag)(((DeviceNode)->Flags & (Flag)) > 0)

/*
 * VOID
 * IopDeviceNodeSetUserFlag(
 *   PDEVICE_NODE DeviceNode,
 *   ULONG UserFlag);
 */
#define IopDeviceNodeSetUserFlag(DeviceNode, UserFlag)((DeviceNode)->UserFlags |= (UserFlag))

/*
 * VOID
 * IopDeviceNodeClearUserFlag(
 *   PDEVICE_NODE DeviceNode,
 *   ULONG UserFlag);
 */
#define IopDeviceNodeClearUserFlag(DeviceNode, UserFlag)((DeviceNode)->UserFlags &= ~(UserFlag))

/*
 * BOOLEAN
 * IopDeviceNodeHasUserFlag(
 *   PDEVICE_NODE DeviceNode,
 *   ULONG UserFlag);
 */
#define IopDeviceNodeHasUserFlag(DeviceNode, UserFlag)(((DeviceNode)->UserFlags & (UserFlag)) > 0)

 /*
 * VOID
 * IopDeviceNodeSetProblem(
 *   PDEVICE_NODE DeviceNode,
 *   ULONG Problem);
 */
#define IopDeviceNodeSetProblem(DeviceNode, Problem)((DeviceNode)->Problem |= (Problem))

/*
 * VOID
 * IopDeviceNodeClearProblem(
 *   PDEVICE_NODE DeviceNode,
 *   ULONG Problem);
 */
#define IopDeviceNodeClearProblem(DeviceNode, Problem)((DeviceNode)->Problem &= ~(Problem))

/*
 * BOOLEAN
 * IopDeviceNodeHasProblem(
 *   PDEVICE_NODE DeviceNode,
 *   ULONG Problem);
 */
#define IopDeviceNodeHasProblem(DeviceNode, Problem)(((DeviceNode)->Problem & (Problem)) > 0)


/*
   Called on every visit of a node during a preorder-traversal of the device
   node tree.
   If the routine returns STATUS_UNSUCCESSFUL the traversal will stop and
   STATUS_SUCCESS is returned to the caller who initiated the tree traversal.
   Any other returned status code will be returned to the caller. If a status
   code that indicates an error (other than STATUS_UNSUCCESSFUL) is returned,
   the traversal is stopped immediately and the status code is returned to
   the caller.
 */
typedef NTSTATUS (*DEVICETREE_TRAVERSE_ROUTINE)(
  PDEVICE_NODE DeviceNode,
  PVOID Context);

/* Context information for traversing the device tree */
typedef struct _DEVICETREE_TRAVERSE_CONTEXT
{
  /* Current device node during a traversal */
  PDEVICE_NODE DeviceNode;
  /* Initial device node where we start the traversal */
  PDEVICE_NODE FirstDeviceNode;
  /* Action routine to be called for every device node */
  DEVICETREE_TRAVERSE_ROUTINE Action;
  /* Context passed to the action routine */
  PVOID Context;
} DEVICETREE_TRAVERSE_CONTEXT, *PDEVICETREE_TRAVERSE_CONTEXT;

/*
 * VOID
 * IopInitDeviceTreeTraverseContext(
 *   PDEVICETREE_TRAVERSE_CONTEXT DeviceTreeTraverseContext,
 *   PDEVICE_NODE DeviceNode,
 *   DEVICETREE_TRAVERSE_ROUTINE Action,
 *   PVOID Context);
 */
#define IopInitDeviceTreeTraverseContext( \
  _DeviceTreeTraverseContext, _DeviceNode, _Action, _Context) { \
  (_DeviceTreeTraverseContext)->FirstDeviceNode = (_DeviceNode); \
  (_DeviceTreeTraverseContext)->Action = (_Action); \
  (_DeviceTreeTraverseContext)->Context = (_Context); }


extern PDEVICE_NODE IopRootDeviceNode;
extern ULONG IoOtherOperationCount;
extern ULONGLONG IoOtherTransferCount;

VOID
PnpInit(VOID);

VOID
IopInitDriverImplementation(VOID);

NTSTATUS
IopGetSystemPowerDeviceObject(PDEVICE_OBJECT *DeviceObject);
NTSTATUS
IopCreateDeviceNode(PDEVICE_NODE ParentNode,
                    PDEVICE_OBJECT PhysicalDeviceObject,
                    PDEVICE_NODE *DeviceNode);
NTSTATUS
IopFreeDeviceNode(PDEVICE_NODE DeviceNode);

VOID
IoInitCancelHandling(VOID);
VOID
IoInitFileSystemImplementation(VOID);
VOID
IoInitVpbImplementation(VOID);

NTSTATUS
IoMountVolume(IN PDEVICE_OBJECT DeviceObject,
	      IN BOOLEAN AllowRawMount);
POBJECT IoOpenSymlink(POBJECT SymbolicLink);
POBJECT IoOpenFileOnDevice(POBJECT SymbolicLink, PWCHAR Name);

VOID STDCALL
IoSecondStageCompletion(
   PKAPC Apc,
   PKNORMAL_ROUTINE* NormalRoutine,
   PVOID* NormalContext,
   PVOID* SystemArgument1,
   PVOID* SystemArgument2);

NTSTATUS STDCALL
IopCreateFile(PVOID ObjectBody,
	      PVOID Parent,
	      PWSTR RemainingPath,
	      POBJECT_ATTRIBUTES ObjectAttributes);
NTSTATUS STDCALL
IopCreateDevice(PVOID ObjectBody,
		PVOID Parent,
		PWSTR RemainingPath,
		POBJECT_ATTRIBUTES ObjectAttributes);

NTSTATUS 
STDCALL
IopAttachVpb(PDEVICE_OBJECT DeviceObject);

PIRP IoBuildSynchronousFsdRequestWithMdl(ULONG MajorFunction,
					 PDEVICE_OBJECT DeviceObject,
					 PMDL Mdl,
					 PLARGE_INTEGER StartingOffset,
					 PKEVENT Event,
					 PIO_STATUS_BLOCK IoStatusBlock,
					 BOOLEAN PagingIo);

VOID IoInitShutdownNotification(VOID);
VOID IoShutdownRegisteredDevices(VOID);
VOID IoShutdownRegisteredFileSystems(VOID);

NTSTATUS STDCALL
IoPageWrite(PFILE_OBJECT	FileObject,
	    PMDL		Mdl,
	    PLARGE_INTEGER	Offset,
	    PKEVENT		Event,
	    PIO_STATUS_BLOCK	StatusBlock);

NTSTATUS
IoCreateArcNames(VOID);

NTSTATUS
IoCreateSystemRootLink(PCHAR ParameterLine);

NTSTATUS
IopInitiatePnpIrp(
  PDEVICE_OBJECT DeviceObject,
  PIO_STATUS_BLOCK IoStatusBlock,
  ULONG MinorFunction,
  PIO_STACK_LOCATION Stack);

BOOLEAN
IopCreateUnicodeString(
  PUNICODE_STRING	Destination,
  PWSTR Source,
  POOL_TYPE PoolType);


NTSTATUS
IoCreateDriverList(VOID);

NTSTATUS
IoDestroyDriverList(VOID);

/* bootlog.c */

VOID
IopInitBootLog(BOOLEAN StartBootLog);

VOID
IopStartBootLog(VOID);

VOID
IopStopBootLog(VOID);

VOID
IopBootLog(PUNICODE_STRING DriverName, BOOLEAN Success);

VOID
IopSaveBootLogToFile(VOID);

/* cancel.c */

VOID STDCALL
IoCancelThreadIo(PETHREAD Thread);

/* errlog.c */

NTSTATUS
IopInitErrorLog(VOID);


/* rawfs.c */

BOOLEAN
RawFsIsRawFileSystemDeviceObject(IN PDEVICE_OBJECT DeviceObject);

NTSTATUS STDCALL
RawFsDriverEntry(PDRIVER_OBJECT DriverObject,
  PUNICODE_STRING RegistryPath);


/* pnproot.c */

NTSTATUS
STDCALL
PnpRootDriverEntry(
  IN PDRIVER_OBJECT DriverObject,
  IN PUNICODE_STRING RegistryPath);

NTSTATUS
PnpRootCreateDevice(
  PDEVICE_OBJECT *PhysicalDeviceObject);

/* device.c */

NTSTATUS FASTCALL
IopInitializeDevice(
   PDEVICE_NODE DeviceNode,
   PDRIVER_OBJECT DriverObject);

/* driver.c */

VOID FASTCALL
IopInitializeBootDrivers(VOID);

VOID FASTCALL
IopInitializeSystemDrivers(VOID);

NTSTATUS FASTCALL
IopCreateDriverObject(
   PDRIVER_OBJECT *DriverObject,
   PUNICODE_STRING ServiceName,
   ULONG CreateAttributes,
   BOOLEAN FileSystemDriver,
   PVOID DriverImageStart,
   ULONG DriverImageSize);

NTSTATUS FASTCALL
IopLoadServiceModule(
   IN PUNICODE_STRING ServiceName,
   OUT PMODULE_OBJECT *ModuleObject);

NTSTATUS FASTCALL
IopInitializeDriverModule(
   IN PDEVICE_NODE DeviceNode,
   IN PMODULE_OBJECT ModuleObject,
   IN PUNICODE_STRING ServiceName,
   IN BOOLEAN FileSystemDriver,
   OUT PDRIVER_OBJECT *DriverObject);

NTSTATUS FASTCALL
IopAttachFilterDrivers(
   PDEVICE_NODE DeviceNode,
   BOOLEAN Lower);

VOID FASTCALL
IopMarkLastReinitializeDriver(VOID);

VOID FASTCALL
IopReinitializeDrivers(VOID);


/* plugplay.c */

NTSTATUS INIT_FUNCTION
IopInitPlugPlayEvents(VOID);

NTSTATUS
IopQueueTargetDeviceEvent(const GUID *Guid,
                          PUNICODE_STRING DeviceIds);


/* pnpmgr.c */

NTSTATUS
IopInitializePnpServices(
   IN PDEVICE_NODE DeviceNode,
   IN BOOLEAN BootDrivers);

NTSTATUS
IopInvalidateDeviceRelations(
   IN PDEVICE_NODE DeviceNode,
   IN DEVICE_RELATION_TYPE Type);

/* timer.c */
VOID
FASTCALL
IopInitTimerImplementation(VOID);

VOID
STDCALL
IopRemoveTimerFromTimerList(
	IN PIO_TIMER Timer
);

/* iocomp.c */
VOID
FASTCALL
IopInitIoCompletionImplementation(VOID);

#define CM_RESOURCE_LIST_SIZE(ResList) \
  (ResList->Count == 1) ? \
    FIELD_OFFSET(CM_RESOURCE_LIST, List[0].PartialResourceList. \
                 PartialDescriptors[(ResList)->List[0].PartialResourceList.Count]) \
                        : \
    FIELD_OFFSET(CM_RESOURCE_LIST, List)

#endif
