/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/fs.c
 * PURPOSE:         Filesystem functions
 *
 * PROGRAMMERS:     David Welch (welch@mcmail.com)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, IoInitFileSystemImplementation)
#endif

/* TYPES *******************************************************************/

typedef struct _FILE_SYSTEM_OBJECT
{
  PDEVICE_OBJECT DeviceObject;
  LIST_ENTRY Entry;
} FILE_SYSTEM_OBJECT, *PFILE_SYSTEM_OBJECT;

typedef struct _FS_CHANGE_NOTIFY_ENTRY
{
  LIST_ENTRY FsChangeNotifyList;
  PDRIVER_OBJECT DriverObject;
  PDRIVER_FS_NOTIFICATION FSDNotificationProc;
} FS_CHANGE_NOTIFY_ENTRY, *PFS_CHANGE_NOTIFY_ENTRY;

/* GLOBALS ******************************************************************/

static ERESOURCE FileSystemListLock;
static LIST_ENTRY FileSystemListHead;

static KGUARDED_MUTEX FsChangeNotifyListLock;
static LIST_ENTRY FsChangeNotifyListHead;

static VOID
IopNotifyFileSystemChange(PDEVICE_OBJECT DeviceObject,
			  BOOLEAN DriverActive);


/* FUNCTIONS *****************************************************************/

VOID INIT_FUNCTION
IoInitFileSystemImplementation(VOID)
{
  InitializeListHead(&FileSystemListHead);
  ExInitializeResourceLite(&FileSystemListLock);

  InitializeListHead(&FsChangeNotifyListHead);
  KeInitializeGuardedMutex(&FsChangeNotifyListLock);
}


VOID
IoShutdownRegisteredFileSystems(VOID)
{
  FILE_SYSTEM_OBJECT* current;
  PIRP Irp;
  KEVENT Event;
  IO_STATUS_BLOCK IoStatusBlock;
  NTSTATUS Status;

  DPRINT("IoShutdownRegisteredFileSystems()\n");

  KeEnterCriticalRegion();
  ExAcquireResourceSharedLite(&FileSystemListLock,TRUE);
  KeInitializeEvent(&Event,
		    NotificationEvent,
		    FALSE);

  LIST_FOR_EACH(current, &FileSystemListHead, FILE_SYSTEM_OBJECT,Entry)
    {
      /* send IRP_MJ_SHUTDOWN */
      Irp = IoBuildSynchronousFsdRequest(IRP_MJ_SHUTDOWN,
					 current->DeviceObject,
					 NULL,
					 0,
					 0,
					 &Event,
					 &IoStatusBlock);

      Status = IoCallDriver(current->DeviceObject,Irp);
      if (Status == STATUS_PENDING)
	{
	  KeWaitForSingleObject(&Event,
				Executive,
				KernelMode,
				FALSE,
				NULL);
	}
    }

  ExReleaseResourceLite(&FileSystemListLock);
  KeLeaveCriticalRegion();
}


static NTSTATUS
IopMountFileSystem(PDEVICE_OBJECT DeviceObject,
		   PDEVICE_OBJECT DeviceToMount)
{
  IO_STATUS_BLOCK IoStatusBlock;
  PIO_STACK_LOCATION StackPtr;
  KEVENT Event;
  PIRP Irp;
  NTSTATUS Status;

  DPRINT("IopMountFileSystem(DeviceObject 0x%p, DeviceToMount 0x%p)\n",
	 DeviceObject,DeviceToMount);

  ASSERT_IRQL(PASSIVE_LEVEL);

  KeInitializeEvent(&Event, NotificationEvent, FALSE);
  Irp = IoAllocateIrp(DeviceObject->StackSize, TRUE);
  if (Irp==NULL)
    {
      return(STATUS_INSUFFICIENT_RESOURCES);
    }

  Irp->UserIosb = &IoStatusBlock;
  DPRINT("Irp->UserIosb 0x%p\n", Irp->UserIosb);
  Irp->UserEvent = &Event;
  Irp->Tail.Overlay.Thread = PsGetCurrentThread();

  StackPtr = IoGetNextIrpStackLocation(Irp);
  StackPtr->MajorFunction = IRP_MJ_FILE_SYSTEM_CONTROL;
  StackPtr->MinorFunction = IRP_MN_MOUNT_VOLUME;
  StackPtr->Flags = 0;
  StackPtr->Control = 0;
  StackPtr->DeviceObject = DeviceObject;
  StackPtr->FileObject = NULL;
  StackPtr->CompletionRoutine = NULL;

  StackPtr->Parameters.MountVolume.Vpb = DeviceToMount->Vpb;
  StackPtr->Parameters.MountVolume.DeviceObject = DeviceToMount;

  Status = IoCallDriver(DeviceObject,Irp);
  if (Status==STATUS_PENDING)
    {
      KeWaitForSingleObject(&Event,Executive,KernelMode,FALSE,NULL);
      Status = IoStatusBlock.Status;
    }

  return(Status);
}


static NTSTATUS
IopLoadFileSystem(IN PDEVICE_OBJECT DeviceObject)
{
  IO_STATUS_BLOCK IoStatusBlock;
  PIO_STACK_LOCATION StackPtr;
  KEVENT Event;
  PIRP Irp;
  NTSTATUS Status;

  DPRINT("IopLoadFileSystem(DeviceObject 0x%p)\n", DeviceObject);

  ASSERT_IRQL(PASSIVE_LEVEL);

  KeInitializeEvent(&Event, NotificationEvent, FALSE);
  Irp = IoAllocateIrp(DeviceObject->StackSize, TRUE);
  if (Irp==NULL)
    {
      return(STATUS_INSUFFICIENT_RESOURCES);
    }

  Irp->UserIosb = &IoStatusBlock;
  DPRINT("Irp->UserIosb 0x%p\n", Irp->UserIosb);
  Irp->UserEvent = &Event;
  Irp->Tail.Overlay.Thread = PsGetCurrentThread();

  StackPtr = IoGetNextIrpStackLocation(Irp);
  StackPtr->MajorFunction = IRP_MJ_FILE_SYSTEM_CONTROL;
  StackPtr->MinorFunction = IRP_MN_LOAD_FILE_SYSTEM;
  StackPtr->Flags = 0;
  StackPtr->Control = 0;
  StackPtr->DeviceObject = DeviceObject;
  StackPtr->FileObject = NULL;
  StackPtr->CompletionRoutine = NULL;

  Status = IoCallDriver(DeviceObject,Irp);
  if (Status==STATUS_PENDING)
    {
      KeWaitForSingleObject(&Event,Executive,KernelMode,FALSE,NULL);
      Status = IoStatusBlock.Status;
    }

  return(Status);
}


NTSTATUS
IoMountVolume(IN PDEVICE_OBJECT DeviceObject,
	      IN BOOLEAN AllowRawMount)
/*
 * FUNCTION: Mounts a logical volume
 * ARGUMENTS:
 *         DeviceObject = Device to mount
 * RETURNS: Status
 */
{
  PFILE_SYSTEM_OBJECT current;
  NTSTATUS Status;
  DEVICE_TYPE MatchingDeviceType;
  PDEVICE_OBJECT DevObject;

  ASSERT_IRQL(PASSIVE_LEVEL);

  DPRINT("IoMountVolume(DeviceObject 0x%p  AllowRawMount %x)\n",
	 DeviceObject, AllowRawMount);

  switch (DeviceObject->DeviceType)
    {
      case FILE_DEVICE_DISK:
      case FILE_DEVICE_VIRTUAL_DISK: /* ?? */
	MatchingDeviceType = FILE_DEVICE_DISK_FILE_SYSTEM;
	break;

      case FILE_DEVICE_CD_ROM:
	MatchingDeviceType = FILE_DEVICE_CD_ROM_FILE_SYSTEM;
	break;

      case FILE_DEVICE_NETWORK:
	MatchingDeviceType = FILE_DEVICE_NETWORK_FILE_SYSTEM;
	break;

      case FILE_DEVICE_TAPE:
	MatchingDeviceType = FILE_DEVICE_TAPE_FILE_SYSTEM;
	break;

      default:
	CPRINT("No matching file system type found for device type: %x\n",
	       DeviceObject->DeviceType);
	return(STATUS_UNRECOGNIZED_VOLUME);
    }

  KeEnterCriticalRegion();
  ExAcquireResourceSharedLite(&FileSystemListLock,TRUE);

restart:
  LIST_FOR_EACH(current,&FileSystemListHead, FILE_SYSTEM_OBJECT, Entry)
    {
      if (current->DeviceObject->DeviceType != MatchingDeviceType)
	{
	  continue;
	}
      /* If we are not allowed to mount this volume as a raw filesystem volume
         then don't try this */
      if (!AllowRawMount && RawFsIsRawFileSystemDeviceObject(current->DeviceObject))
        {
          Status = STATUS_UNRECOGNIZED_VOLUME;
        }
      else
        {
          Status = IopMountFileSystem(current->DeviceObject,
				      DeviceObject);
        }
      switch (Status)
	{
	  case STATUS_FS_DRIVER_REQUIRED:
	    DevObject = current->DeviceObject;
	    ExReleaseResourceLite(&FileSystemListLock);
	    Status = IopLoadFileSystem(DevObject);
	    if (!NT_SUCCESS(Status))
	      {
		KeLeaveCriticalRegion();
		return(Status);
	      }
	    ExAcquireResourceSharedLite(&FileSystemListLock,TRUE);
	    goto restart;

	  case STATUS_SUCCESS:
	    DeviceObject->Vpb->Flags = DeviceObject->Vpb->Flags |
	                               VPB_MOUNTED;
	    ExReleaseResourceLite(&FileSystemListLock);
	    KeLeaveCriticalRegion();
	    return(STATUS_SUCCESS);

	  case STATUS_UNRECOGNIZED_VOLUME:
	  default:
       /* do nothing */
       break;
	}
    }
  ExReleaseResourceLite(&FileSystemListLock);
  KeLeaveCriticalRegion();

  return(STATUS_UNRECOGNIZED_VOLUME);
}


/**********************************************************************
 * NAME							EXPORTED
 * 	IoVerifyVolume
 *
 * DESCRIPTION
 *	Verify the file system type and volume information or mount
 *	a file system.
 *
 * ARGUMENTS
 *	DeviceObject
 *		Device to verify or mount
 *
 *	AllowRawMount
 *		...
 *
 * RETURN VALUE
 *	Status
 *
 * @implemented
 */
NTSTATUS STDCALL
IoVerifyVolume(IN PDEVICE_OBJECT DeviceObject,
	       IN BOOLEAN AllowRawMount)
{
  IO_STATUS_BLOCK IoStatusBlock;
  PIO_STACK_LOCATION StackPtr;
  KEVENT Event;
  PIRP Irp;
  NTSTATUS Status;
  PDEVICE_OBJECT DevObject;

  DPRINT("IoVerifyVolume(DeviceObject 0x%p  AllowRawMount %x)\n",
	 DeviceObject, AllowRawMount);

  Status = STATUS_SUCCESS;

  KeWaitForSingleObject(&DeviceObject->DeviceLock,
			Executive,
			KernelMode,
			FALSE,
			NULL);

  if (DeviceObject->Vpb->Flags & VPB_MOUNTED)
    {
      /* Issue verify request to the FSD */
      DevObject = DeviceObject->Vpb->DeviceObject;

      KeInitializeEvent(&Event,
			NotificationEvent,
			FALSE);

      Irp = IoAllocateIrp(DevObject->StackSize, TRUE);
      if (Irp==NULL)
	{
	  return(STATUS_INSUFFICIENT_RESOURCES);
	}

      Irp->UserIosb = &IoStatusBlock;
      Irp->UserEvent = &Event;
      Irp->Tail.Overlay.Thread = PsGetCurrentThread();

      StackPtr = IoGetNextIrpStackLocation(Irp);
      StackPtr->MajorFunction = IRP_MJ_FILE_SYSTEM_CONTROL;
      StackPtr->MinorFunction = IRP_MN_VERIFY_VOLUME;
      StackPtr->Flags = 0;
      StackPtr->Control = 0;
      StackPtr->DeviceObject = DevObject;
      StackPtr->FileObject = NULL;
      StackPtr->CompletionRoutine = NULL;

      StackPtr->Parameters.VerifyVolume.Vpb = DeviceObject->Vpb;
      StackPtr->Parameters.VerifyVolume.DeviceObject = DeviceObject;

      Status = IoCallDriver(DevObject,
			    Irp);
      if (Status==STATUS_PENDING)
	{
	  KeWaitForSingleObject(&Event,Executive,KernelMode,FALSE,NULL);
	  Status = IoStatusBlock.Status;
	}

      if (NT_SUCCESS(Status))
	{
	  KeSetEvent(&DeviceObject->DeviceLock,
		     IO_NO_INCREMENT,
		     FALSE);
	  return(STATUS_SUCCESS);
	}
    }

  if (Status == STATUS_WRONG_VOLUME)
    {
      /* Clean existing VPB. This unmounts the filesystem. */
      DPRINT("Wrong volume!\n");

      DeviceObject->Vpb->DeviceObject = NULL;
      DeviceObject->Vpb->Flags &= ~VPB_MOUNTED;
    }

  /* Start mount sequence */
  Status = IoMountVolume(DeviceObject,
			 AllowRawMount);

  KeSetEvent(&DeviceObject->DeviceLock,
	     IO_NO_INCREMENT,
	     FALSE);

  return(Status);
}




/*
 * @implemented
 */
VOID STDCALL
IoRegisterFileSystem(IN PDEVICE_OBJECT DeviceObject)
{
  PFILE_SYSTEM_OBJECT Fs;

  DPRINT("IoRegisterFileSystem(DeviceObject 0x%p)\n", DeviceObject);

  Fs = ExAllocatePoolWithTag(NonPagedPool,
			     sizeof(FILE_SYSTEM_OBJECT),
			     TAG_FILE_SYSTEM);
  ASSERT(Fs!=NULL);

  Fs->DeviceObject = DeviceObject;
  KeEnterCriticalRegion();
  ExAcquireResourceExclusiveLite(&FileSystemListLock, TRUE);

  /* The RAW filesystem device objects must be last in the list so the
     raw filesystem driver is the last filesystem driver asked to mount
     a volume. It is always the first filesystem driver registered so
     we use InsertHeadList() here as opposed to the other alternative
     InsertTailList(). */
  InsertHeadList(&FileSystemListHead,
		 &Fs->Entry);

  ExReleaseResourceLite(&FileSystemListLock);
  KeLeaveCriticalRegion();

  IopNotifyFileSystemChange(DeviceObject,
			    TRUE);
}


/*
 * @implemented
 */
VOID STDCALL
IoUnregisterFileSystem(IN PDEVICE_OBJECT DeviceObject)
{
  PFILE_SYSTEM_OBJECT current;

  DPRINT("IoUnregisterFileSystem(DeviceObject 0x%p)\n", DeviceObject);

  KeEnterCriticalRegion();
  ExAcquireResourceExclusiveLite(&FileSystemListLock, TRUE);

  LIST_FOR_EACH(current,&FileSystemListHead, FILE_SYSTEM_OBJECT,Entry)
    {
      if (current->DeviceObject == DeviceObject)
	{
     RemoveEntryList(&current->Entry);
	  ExFreePoolWithTag(current, TAG_FILE_SYSTEM);
	  ExReleaseResourceLite(&FileSystemListLock);
	  KeLeaveCriticalRegion();
	  IopNotifyFileSystemChange(DeviceObject, FALSE);
	  return;
	}
    }
    
  ExReleaseResourceLite(&FileSystemListLock);
  KeLeaveCriticalRegion();
}


/**********************************************************************
 * NAME							EXPORTED
 * 	IoGetBaseFileSystemDeviceObject@4
 *
 * DESCRIPTION
 *	Get the DEVICE_OBJECT associated to
 *	a FILE_OBJECT.
 *
 * ARGUMENTS
 *	FileObject
 *
 * RETURN VALUE
 *
 * NOTE
 * 	From Bo Branten's ntifs.h v13.
 *
 * @implemented
 */
PDEVICE_OBJECT STDCALL
IoGetBaseFileSystemDeviceObject(IN PFILE_OBJECT FileObject)
{
	PDEVICE_OBJECT	DeviceObject = NULL;
	PVPB		Vpb = NULL;

	/*
	 * If the FILE_OBJECT's VPB is defined,
	 * get the device from it.
	 */
	if (NULL != (Vpb = FileObject->Vpb))
	{
		if (NULL != (DeviceObject = Vpb->DeviceObject))
		{
			/* Vpb->DeviceObject DEFINED! */
			return DeviceObject;
		}
	}
	/*
	 * If that failed, try the VPB
	 * in the FILE_OBJECT's DeviceObject.
	 */
	DeviceObject = FileObject->DeviceObject;
	if (NULL == (Vpb = DeviceObject->Vpb))
	{
		/* DeviceObject->Vpb UNDEFINED! */
		return DeviceObject;
	}
	/*
	 * If that pointer to the VPB is again
	 * undefined, return directly the
	 * device object from the FILE_OBJECT.
	 */
	return (
		(NULL == Vpb->DeviceObject)
			? DeviceObject
			: Vpb->DeviceObject
		);
}


static VOID
IopNotifyFileSystemChange(PDEVICE_OBJECT DeviceObject,
			  BOOLEAN DriverActive)
{
  PFS_CHANGE_NOTIFY_ENTRY ChangeEntry;

  KeAcquireGuardedMutex(&FsChangeNotifyListLock);
  LIST_FOR_EACH(ChangeEntry, &FsChangeNotifyListHead,FS_CHANGE_NOTIFY_ENTRY, FsChangeNotifyList) 
    {
      (ChangeEntry->FSDNotificationProc)(DeviceObject, DriverActive);
    }
  KeReleaseGuardedMutex(&FsChangeNotifyListLock);
}


/*
 * @implemented
 */
NTSTATUS STDCALL
IoRegisterFsRegistrationChange(IN PDRIVER_OBJECT DriverObject,
			       IN PDRIVER_FS_NOTIFICATION FSDNotificationProc)
{
  PFS_CHANGE_NOTIFY_ENTRY Entry;

  Entry = ExAllocatePoolWithTag(NonPagedPool,
				sizeof(FS_CHANGE_NOTIFY_ENTRY),
				TAG_FS_CHANGE_NOTIFY);
  if (Entry == NULL)
    return(STATUS_INSUFFICIENT_RESOURCES);

  Entry->DriverObject = DriverObject;
  Entry->FSDNotificationProc = FSDNotificationProc;

  KeAcquireGuardedMutex(&FsChangeNotifyListLock);
  InsertHeadList(&FsChangeNotifyListHead,
			      &Entry->FsChangeNotifyList);
  KeReleaseGuardedMutex(&FsChangeNotifyListLock);

  return(STATUS_SUCCESS);
}


/*
 * @implemented
 */
VOID STDCALL
IoUnregisterFsRegistrationChange(IN PDRIVER_OBJECT DriverObject,
				 IN PDRIVER_FS_NOTIFICATION FSDNotificationProc)
{
  PFS_CHANGE_NOTIFY_ENTRY ChangeEntry;

  LIST_FOR_EACH(ChangeEntry, &FsChangeNotifyListHead, FS_CHANGE_NOTIFY_ENTRY, FsChangeNotifyList)
    {
      if (ChangeEntry->DriverObject == DriverObject &&
	  ChangeEntry->FSDNotificationProc == FSDNotificationProc)
	{
	  KeAcquireGuardedMutex(&FsChangeNotifyListLock);
     RemoveEntryList(&ChangeEntry->FsChangeNotifyList);
	  KeReleaseGuardedMutex(&FsChangeNotifyListLock);

     ExFreePoolWithTag(ChangeEntry, TAG_FS_CHANGE_NOTIFY);
	  return;
	}

    }
}

/* EOF */
