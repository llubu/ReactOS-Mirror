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

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>


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
  PFSDNOTIFICATIONPROC FSDNotificationProc;
} FS_CHANGE_NOTIFY_ENTRY, *PFS_CHANGE_NOTIFY_ENTRY;

/* GLOBALS ******************************************************************/

static ERESOURCE FileSystemListLock;
static LIST_ENTRY FileSystemListHead;

static KSPIN_LOCK FsChangeNotifyListLock;
static LIST_ENTRY FsChangeNotifyListHead;

#define TAG_FILE_SYSTEM       TAG('F', 'S', 'Y', 'S')
#define TAG_FS_CHANGE_NOTIFY  TAG('F', 'S', 'C', 'N')


static VOID
IopNotifyFileSystemChange(PDEVICE_OBJECT DeviceObject,
			  BOOLEAN DriverActive);


/* FUNCTIONS *****************************************************************/

/*
 * @unimplemented
 */
VOID
STDCALL
IoCancelFileOpen(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PFILE_OBJECT    FileObject
    )
{
	UNIMPLEMENTED;
}

/*
 * @implemented
 */
NTSTATUS STDCALL
NtFsControlFile (
	IN	HANDLE			DeviceHandle,
	IN	HANDLE			EventHandle OPTIONAL,
	IN	PIO_APC_ROUTINE		ApcRoutine OPTIONAL,
	IN	PVOID			ApcContext OPTIONAL,
	OUT	PIO_STATUS_BLOCK	IoStatusBlock,
	IN	ULONG			IoControlCode,
	IN	PVOID			InputBuffer,
	IN	ULONG			InputBufferSize,
	OUT	PVOID			OutputBuffer,
	IN	ULONG			OutputBufferSize
	)
{
  NTSTATUS Status;
  PFILE_OBJECT FileObject;
  PDEVICE_OBJECT DeviceObject;
  PIRP Irp;
  PEXTENDED_IO_STACK_LOCATION StackPtr;
  PKEVENT ptrEvent;
  KPROCESSOR_MODE PreviousMode;

  DPRINT("NtFsControlFile(DeviceHandle %x EventHandle %x ApcRoutine %x "
         "ApcContext %x IoStatusBlock %x IoControlCode %x "
         "InputBuffer %x InputBufferSize %x OutputBuffer %x "
         "OutputBufferSize %x)\n",
         DeviceHandle,EventHandle,ApcRoutine,ApcContext,IoStatusBlock,
         IoControlCode,InputBuffer,InputBufferSize,OutputBuffer,
         OutputBufferSize);

  PreviousMode = ExGetPreviousMode();

  /* Check granted access against the access rights from IoContolCode */
  Status = ObReferenceObjectByHandle(DeviceHandle,
				     (IoControlCode >> 14) & 0x3,
				     NULL,
				     PreviousMode,
				     (PVOID *) &FileObject,
				     NULL);
  if (!NT_SUCCESS(Status))
    {
      return Status;
    }

  if (EventHandle != NULL)
    {
      Status = ObReferenceObjectByHandle(EventHandle,
                                         SYNCHRONIZE,
                                         ExEventObjectType,
                                         PreviousMode,
                                         (PVOID*)&ptrEvent,
                                         NULL);
      if (!NT_SUCCESS(Status))
        {
          ObDereferenceObject(FileObject);
          return Status;
        }
    }
  else
    {
      KeResetEvent(&FileObject->Event);
      ptrEvent = &FileObject->Event;
    }

  DeviceObject = FileObject->DeviceObject;

  Irp = IoBuildDeviceIoControlRequest(IoControlCode,
				      DeviceObject,
				      InputBuffer,
				      InputBufferSize,
				      OutputBuffer,
				      OutputBufferSize,
				      FALSE,
				      ptrEvent,
				      IoStatusBlock);

  /* Trigger FileObject/Event dereferencing */
  Irp->Tail.Overlay.OriginalFileObject = FileObject;

  Irp->RequestorMode = PreviousMode;
  Irp->Overlay.AsynchronousParameters.UserApcRoutine = ApcRoutine;
  Irp->Overlay.AsynchronousParameters.UserApcContext = ApcContext;

  StackPtr = (PEXTENDED_IO_STACK_LOCATION) IoGetNextIrpStackLocation(Irp);
  StackPtr->FileObject = FileObject;
  StackPtr->DeviceObject = DeviceObject;
  StackPtr->Parameters.FileSystemControl.InputBufferLength = InputBufferSize;
  StackPtr->Parameters.FileSystemControl.OutputBufferLength = 
    OutputBufferSize;
  StackPtr->MajorFunction = IRP_MJ_FILE_SYSTEM_CONTROL;

  Status = IoCallDriver(DeviceObject,Irp);
  if (Status == STATUS_PENDING && (FileObject->Flags & FO_SYNCHRONOUS_IO))
    {
      KeWaitForSingleObject(ptrEvent,
			    Executive,
			    PreviousMode,
			    FileObject->Flags & FO_ALERTABLE_IO,
			    NULL);
      Status = IoStatusBlock->Status;
    }

  return Status;
}


VOID INIT_FUNCTION
IoInitFileSystemImplementation(VOID)
{
  InitializeListHead(&FileSystemListHead);
  ExInitializeResourceLite(&FileSystemListLock);

  InitializeListHead(&FsChangeNotifyListHead);
  KeInitializeSpinLock(&FsChangeNotifyListLock);
}


VOID
IoShutdownRegisteredFileSystems(VOID)
{
  PLIST_ENTRY current_entry;
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

  current_entry = FileSystemListHead.Flink;
  while (current_entry!=(&FileSystemListHead))
    {
      current = CONTAINING_RECORD(current_entry,FILE_SYSTEM_OBJECT,Entry);

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

      current_entry = current_entry->Flink;
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

  DPRINT("IopMountFileSystem(DeviceObject %x, DeviceToMount %x)\n",
	 DeviceObject,DeviceToMount);

  ASSERT_IRQL(PASSIVE_LEVEL);

  KeInitializeEvent(&Event, NotificationEvent, FALSE);
  Irp = IoAllocateIrp(DeviceObject->StackSize, TRUE);
  if (Irp==NULL)
    {
      return(STATUS_INSUFFICIENT_RESOURCES);
    }

  Irp->UserIosb = &IoStatusBlock;
  DPRINT("Irp->UserIosb %x\n", Irp->UserIosb);
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

  DPRINT("IopLoadFileSystem(DeviceObject %x)\n", DeviceObject);

  ASSERT_IRQL(PASSIVE_LEVEL);

  KeInitializeEvent(&Event, NotificationEvent, FALSE);
  Irp = IoAllocateIrp(DeviceObject->StackSize, TRUE);
  if (Irp==NULL)
    {
      return(STATUS_INSUFFICIENT_RESOURCES);
    }

  Irp->UserIosb = &IoStatusBlock;
  DPRINT("Irp->UserIosb %x\n", Irp->UserIosb);
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
  PLIST_ENTRY current_entry;
  FILE_SYSTEM_OBJECT* current;
  NTSTATUS Status;
  DEVICE_TYPE MatchingDeviceType;
  PDEVICE_OBJECT DevObject;

  ASSERT_IRQL(PASSIVE_LEVEL);

  DPRINT("IoMountVolume(DeviceObject %x  AllowRawMount %x)\n",
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
  current_entry = FileSystemListHead.Flink;
  while (current_entry!=(&FileSystemListHead))
    {
      current = CONTAINING_RECORD(current_entry,FILE_SYSTEM_OBJECT,Entry);
      if (current->DeviceObject->DeviceType != MatchingDeviceType)
	{
	  current_entry = current_entry->Flink;
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
	    current_entry = FileSystemListHead.Flink;
	    continue;

	  case STATUS_SUCCESS:
	    DeviceObject->Vpb->Flags = DeviceObject->Vpb->Flags |
	                               VPB_MOUNTED;
	    ExReleaseResourceLite(&FileSystemListLock);
	    KeLeaveCriticalRegion();
	    return(STATUS_SUCCESS);

	  case STATUS_UNRECOGNIZED_VOLUME:
	  default:
	    current_entry = current_entry->Flink;
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

  DPRINT("IoVerifyVolume(DeviceObject %x  AllowRawMount %x)\n",
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
PDEVICE_OBJECT STDCALL
IoGetDeviceToVerify(IN PETHREAD Thread)
/*
 * FUNCTION: Returns a pointer to the device, representing a removable-media
 * device, that is the target of the given thread's I/O request
 */
{
  return(Thread->DeviceToVerify);
}


/*
 * @implemented
 */
VOID STDCALL
IoSetDeviceToVerify(IN PETHREAD Thread,
		    IN PDEVICE_OBJECT DeviceObject)
{
  Thread->DeviceToVerify = DeviceObject;
}


/*
 * @implemented
 */
VOID STDCALL
IoSetHardErrorOrVerifyDevice(IN PIRP Irp,
			     IN PDEVICE_OBJECT DeviceObject)
{
  Irp->Tail.Overlay.Thread->DeviceToVerify = DeviceObject;
}


/*
 * @implemented
 */
VOID STDCALL
IoRegisterFileSystem(IN PDEVICE_OBJECT DeviceObject)
{
  PFILE_SYSTEM_OBJECT Fs;

  DPRINT("IoRegisterFileSystem(DeviceObject %x)\n",DeviceObject);

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
  PLIST_ENTRY current_entry;
  PFILE_SYSTEM_OBJECT current;

  DPRINT("IoUnregisterFileSystem(DeviceObject %x)\n",DeviceObject);

  KeEnterCriticalRegion();
  ExAcquireResourceExclusiveLite(&FileSystemListLock, TRUE);
  current_entry = FileSystemListHead.Flink;
  while (current_entry!=(&FileSystemListHead))
    {
      current = CONTAINING_RECORD(current_entry,FILE_SYSTEM_OBJECT,Entry);
      if (current->DeviceObject == DeviceObject)
	{
	  RemoveEntryList(current_entry);
	  ExFreePool(current);
	  ExReleaseResourceLite(&FileSystemListLock);
	  KeLeaveCriticalRegion();
	  IopNotifyFileSystemChange(DeviceObject, FALSE);
	  return;
	}
      current_entry = current_entry->Flink;
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
  PLIST_ENTRY Entry;
  KIRQL oldlvl;

  KeAcquireSpinLock(&FsChangeNotifyListLock,&oldlvl);
  Entry = FsChangeNotifyListHead.Flink;
  while (Entry != &FsChangeNotifyListHead)
    {
      ChangeEntry = CONTAINING_RECORD(Entry, FS_CHANGE_NOTIFY_ENTRY, FsChangeNotifyList);

      (ChangeEntry->FSDNotificationProc)(DeviceObject, DriverActive);

      Entry = Entry->Flink;
    }
  KeReleaseSpinLock(&FsChangeNotifyListLock,oldlvl);
}


/*
 * @implemented
 */
NTSTATUS STDCALL
IoRegisterFsRegistrationChange(IN PDRIVER_OBJECT DriverObject,
			       IN PFSDNOTIFICATIONPROC FSDNotificationProc)
{
  PFS_CHANGE_NOTIFY_ENTRY Entry;

  Entry = ExAllocatePoolWithTag(NonPagedPool,
				sizeof(FS_CHANGE_NOTIFY_ENTRY),
				TAG_FS_CHANGE_NOTIFY);
  if (Entry == NULL)
    return(STATUS_INSUFFICIENT_RESOURCES);

  Entry->DriverObject = DriverObject;
  Entry->FSDNotificationProc = FSDNotificationProc;

  ExInterlockedInsertHeadList(&FsChangeNotifyListHead,
			      &Entry->FsChangeNotifyList,
			      &FsChangeNotifyListLock);

  return(STATUS_SUCCESS);
}


/*
 * @implemented
 */
VOID STDCALL
IoUnregisterFsRegistrationChange(IN PDRIVER_OBJECT DriverObject,
				 IN PFSDNOTIFICATIONPROC FSDNotificationProc)
{
  PFS_CHANGE_NOTIFY_ENTRY ChangeEntry;
  PLIST_ENTRY Entry;
  KIRQL oldlvl;

  Entry = FsChangeNotifyListHead.Flink;
  while (Entry != &FsChangeNotifyListHead)
    {
      ChangeEntry = CONTAINING_RECORD(Entry, FS_CHANGE_NOTIFY_ENTRY, FsChangeNotifyList);
      if (ChangeEntry->DriverObject == DriverObject &&
	  ChangeEntry->FSDNotificationProc == FSDNotificationProc)
	{
	  KeAcquireSpinLock(&FsChangeNotifyListLock,&oldlvl);
	  RemoveEntryList(Entry);
	  KeReleaseSpinLock(&FsChangeNotifyListLock,oldlvl);

	  ExFreePool(Entry);
	  return;
	}

      Entry = Entry->Flink;
    }
}

/* EOF */
