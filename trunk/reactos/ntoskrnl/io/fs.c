/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/fs.c
 * PURPOSE:         Filesystem functions
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/io.h>

#define NDEBUG
#include <internal/debug.h>

/* TYPES *******************************************************************/

typedef struct
{
   PDEVICE_OBJECT DeviceObject;
   LIST_ENTRY Entry;
} FILE_SYSTEM_OBJECT;

/* GLOBALS ******************************************************************/

static KSPIN_LOCK FileSystemListLock = {0,};
static LIST_ENTRY FileSystemListHead = {NULL,NULL};

/* FUNCTIONS *****************************************************************/

NTSTATUS
STDCALL
NtFsControlFile(
	IN HANDLE DeviceHandle,
	IN HANDLE Event OPTIONAL, 
	IN PIO_APC_ROUTINE ApcRoutine OPTIONAL, 
	IN PVOID ApcContext OPTIONAL, 
	OUT PIO_STATUS_BLOCK IoStatusBlock, 
	IN ULONG IoControlCode,
	IN PVOID InputBuffer, 
	IN ULONG InputBufferSize,
	OUT PVOID OutputBuffer,
	IN ULONG OutputBufferSize
	)
{
   return(ZwFsControlFile(DeviceHandle,
			  Event,
			  ApcRoutine,
			  ApcContext,
			  IoStatusBlock,
			  IoControlCode,
			  InputBuffer,
			  InputBufferSize,
			  OutputBuffer,
			  OutputBufferSize));
}

NTSTATUS
STDCALL
ZwFsControlFile(
	IN HANDLE DeviceHandle,
	IN HANDLE Event OPTIONAL, 
	IN PIO_APC_ROUTINE ApcRoutine OPTIONAL, 
	IN PVOID ApcContext OPTIONAL, 
	OUT PIO_STATUS_BLOCK IoStatusBlock, 
	IN ULONG IoControlCode,
	IN PVOID InputBuffer, 
	IN ULONG InputBufferSize,
	OUT PVOID OutputBuffer,
	IN ULONG OutputBufferSize
	)
{
   UNIMPLEMENTED;
}

VOID IoInitFileSystemImplementation(VOID)
{
   InitializeListHead(&FileSystemListHead);
   KeInitializeSpinLock(&FileSystemListLock);
}

NTSTATUS IoAskFileSystemToMountDevice(PDEVICE_OBJECT DeviceObject,
				      PDEVICE_OBJECT DeviceToMount)
{
   PIRP Irp;
   KEVENT Event;
   IO_STATUS_BLOCK IoStatusBlock;
   NTSTATUS Status;
   
   DPRINT("IoAskFileSystemToMountDevice(DeviceObject %x, DeviceToMount %x)\n",
	  DeviceObject,DeviceToMount);
   
   KeInitializeEvent(&Event,NotificationEvent,FALSE);
   Irp = IoBuildFilesystemControlRequest(IRP_MN_MOUNT_VOLUME,
					 DeviceObject,
					 &Event,
					 &IoStatusBlock,
					 DeviceToMount);
   Status = IoCallDriver(DeviceObject,Irp);
   if (Status==STATUS_PENDING)
     {
	KeWaitForSingleObject(&Event,Executive,KernelMode,FALSE,NULL);
	Status = IoStatusBlock.Status;
     }
   return(Status);
}

NTSTATUS IoAskFileSystemToLoad(PDEVICE_OBJECT DeviceObject)
{
   UNIMPLEMENTED;
}

NTSTATUS IoTryToMountStorageDevice(PDEVICE_OBJECT DeviceObject)
/*
 * FUNCTION: Trys to mount a storage device
 * ARGUMENTS:
 *         DeviceObject = Device to try and mount
 * RETURNS: Status
 */
{
   KIRQL oldlvl;
   PLIST_ENTRY current_entry;
   FILE_SYSTEM_OBJECT* current;
   NTSTATUS Status;
   
   DPRINT("IoTryToMountStorageDevice(DeviceObject %x)\n",DeviceObject);
   
   KeAcquireSpinLock(&FileSystemListLock,&oldlvl);
   current_entry = FileSystemListHead.Flink;
   while (current_entry!=(&FileSystemListHead))
     {
	current = CONTAINING_RECORD(current_entry,FILE_SYSTEM_OBJECT,Entry);
	Status = IoAskFileSystemToMountDevice(current->DeviceObject, 
					      DeviceObject);
	switch (Status)
	  {
	   case STATUS_FS_DRIVER_REQUIRED:
	     KeReleaseSpinLock(&FileSystemListLock,oldlvl);
	     (void)IoAskFileSystemToLoad(DeviceObject);
	     KeAcquireSpinLock(&FileSystemListLock,&oldlvl);
	     current_entry = FileSystemListHead.Flink;
	     break;
	   
	   case STATUS_SUCCESS:
	     DeviceObject->Vpb->Flags = DeviceObject->Vpb->Flags |
	                                VPB_MOUNTED;
	     KeReleaseSpinLock(&FileSystemListLock,oldlvl);
	     return(STATUS_SUCCESS);
	     
	   case STATUS_UNRECOGNIZED_VOLUME:
	   default:
	     current_entry = current_entry->Flink;
	  }
     }
CHECKPOINT;
   KeReleaseSpinLock(&FileSystemListLock,oldlvl);
CHECKPOINT;
   return(STATUS_UNRECOGNIZED_VOLUME);
}

VOID IoRegisterFileSystem(PDEVICE_OBJECT DeviceObject)
{
   FILE_SYSTEM_OBJECT* fs;
   
   DPRINT("IoRegisterFileSystem(DeviceObject %x)\n",DeviceObject);
   
   fs=ExAllocatePool(NonPagedPool,sizeof(FILE_SYSTEM_OBJECT));
   assert(fs!=NULL);
   
   fs->DeviceObject = DeviceObject;   
   ExInterlockedInsertTailList(&FileSystemListHead,&fs->Entry,
			       &FileSystemListLock);
}

VOID IoUnregisterFileSystem(PDEVICE_OBJECT DeviceObject)
{
   KIRQL oldlvl;
   PLIST_ENTRY current_entry;
   FILE_SYSTEM_OBJECT* current;

   DPRINT("IoUnregisterFileSystem(DeviceObject %x)\n",DeviceObject);
   
   KeAcquireSpinLock(&FileSystemListLock,&oldlvl);
   current_entry = FileSystemListHead.Flink;
   while (current_entry!=(&FileSystemListHead))
     {
	current = CONTAINING_RECORD(current_entry,FILE_SYSTEM_OBJECT,Entry);
	if (current->DeviceObject == DeviceObject)
	  {
	     RemoveEntryList(current_entry);
	     ExFreePool(current);
	     KeReleaseSpinLock(&FileSystemListLock,oldlvl);
	     return;
	  }
	current_entry = current_entry->Flink;
     }
   KeReleaseSpinLock(&FileSystemListLock,oldlvl);
}


