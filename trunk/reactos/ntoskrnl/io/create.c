/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/create.c
 * PURPOSE:         Handling file create/open apis
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  24/05/98: Created
 */

/* INCLUDES ***************************************************************/

#include <windows.h>
#include <ddk/ntddk.h>
#include <internal/ob.h>
#include <internal/io.h>
#include <internal/string.h>
#include <wstring.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *************************************************************/

NTSTATUS ZwCreateFile(PHANDLE FileHandle,
		      ACCESS_MASK DesiredAccess,
		      POBJECT_ATTRIBUTES ObjectAttributes,
		      PIO_STATUS_BLOCK IoStatusBlock,
		      PLARGE_INTEGER AllocateSize,
		      ULONG FileAttributes,
		      ULONG ShareAccess,
		      ULONG CreateDisposition,
		      ULONG CreateOptions,
		      PVOID EaBuffer,
		      ULONG EaLength)
/*
 * FUNCTION: Either causes a new file or directory to be created, or it opens
 * an existing file, device, directory or volume, giving the caller a handle
 * for the file object. This handle can be used by subsequent calls to
 * manipulate data within the file or the file object's state of attributes.
 * ARGUMENTS:
 *        FileHandle (OUT) = Points to a variable which receives the file
 *                           handle on return
 *        DesiredAccess = Desired access to the file
 *        ObjectAttributes = Structure describing the file
 *        IoStatusBlock (OUT) = Receives information about the operation on
 *                              return
 *        AllocationSize = Initial size of the file in bytes
 *        FileAttributes = Attributes to create the file with
 *        ShareAccess = Type of shared access the caller would like to the file
 *        CreateDisposition = Specifies what to do, depending on whether the
 *                            file already exists
 *        CreateOptions = Options for creating a new file
 *        EaBuffer = Undocumented
 *        EaLength = Undocumented
 * RETURNS: Status
 */
{
   UNIMPLEMENTED;
}

NTSTATUS ZwOpenFile(PHANDLE FileHandle,
		    ACCESS_MASK DesiredAccess,
		    POBJECT_ATTRIBUTES ObjectAttributes,
		    PIO_STATUS_BLOCK IoStatusBlock,
		    ULONG ShareAccess,
		    ULONG OpenOptions)
/*
 * FUNCTION: Opens a file (simpler than ZwCreateFile)
 * ARGUMENTS:
 *       FileHandle (OUT) = Variable that receives the file handle on return
 *       DesiredAccess = Access desired by the caller to the file
 *       ObjectAttributes = Structue describing the file to be opened
 *       IoStatusBlock (OUT) = Receives details about the result of the
 *                             operation
 *       ShareAccess = Type of shared access the caller requires
 *       OpenOptions = Options for the file open
 * RETURNS: Status
 * NOTE: Undocumented
 */
{
   PVOID Object;
   NTSTATUS Status;
   PIRP Irp;
   KEVENT Event;
   PDEVICE_OBJECT DeviceObject;
   PFILE_OBJECT FileObject;   
   PIO_STACK_LOCATION StackLoc;
   PWSTR Remainder;
   
   DPRINT("ZwOpenFile(FileHandle %x, ObjectAttributes %x, "
	  "ObjectAttributes->ObjectName->Buffer %w)\n",FileHandle,
	  ObjectAttributes,ObjectAttributes->ObjectName->Buffer);
   
   assert_irql(PASSIVE_LEVEL);
   
   *FileHandle=0;

   FileObject = ObGenericCreateObject(FileHandle,0,NULL,OBJTYP_FILE);
   memset(FileObject,0,sizeof(FILE_OBJECT));
   
   Status =  ObOpenObjectByName(ObjectAttributes,&Object,&Remainder);
   
   if (Status != STATUS_SUCCESS && Status != STATUS_FS_QUERY_REQUIRED)
     {
	DPRINT("%s() = Failed to find object\n",__FUNCTION__);
	ObDeleteHandle(*FileHandle);
	*FileHandle=0;
	ExFreePool(FileObject);
	return(STATUS_UNSUCCESSFUL);
     }
   
   
   DeviceObject = (PDEVICE_OBJECT)Object;
   DeviceObject = IoGetAttachedDevice(DeviceObject);
   DPRINT("DeviceObject %x\n",DeviceObject);
   
   if (Status == STATUS_SUCCESS)
     {
	CHECKPOINT;
	FileObject->Flags = FileObject->Flags | FO_DIRECT_DEVICE_OPEN;
	FileObject->FileName.Buffer = ExAllocatePool(NonPagedPool,
						     ObjectAttributes->Length);
	RtlCopyUnicodeString(&(FileObject->FileName),
			     ObjectAttributes->ObjectName);
     }
   else
     {
	CHECKPOINT;
	if (DeviceObject->DeviceType != FILE_DEVICE_FILE_SYSTEM &&
	    DeviceObject->DeviceType != FILE_DEVICE_DISK)
	  {
	     ObDeleteHandle(*FileHandle);
	     *FileHandle=0;
	     ExFreePool(FileObject);
	     return(STATUS_UNSUCCESSFUL);
	  }
	if (!(DeviceObject->Vpb->Flags & VPB_MOUNTED))
	  {
	     Status = IoTryToMountStorageDevice(DeviceObject);
	     if (Status!=STATUS_SUCCESS)
	       {
		  ObDeleteHandle(*FileHandle);
		  *FileHandle=0;
		  ExFreePool(FileObject);
		  return(Status);
	       }
	     DeviceObject = IoGetAttachedDevice(DeviceObject);
	  }
	DPRINT("Remainder %w\n",Remainder);
	FileObject->FileName.Buffer = ExAllocatePool(NonPagedPool,
						     wstrlen(Remainder));
	RtlInitUnicodeString(&(FileObject->FileName),Remainder);
	DPRINT("FileObject->FileName.Buffer %x %w\n",
	       FileObject->FileName.Buffer,FileObject->FileName.Buffer);
     }
   CHECKPOINT;
   
   FileObject->DeviceObject=DeviceObject;
   FileObject->Vpb=DeviceObject->Vpb;
   
   KeInitializeEvent(&Event,NotificationEvent,FALSE);
   
   Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);
   if (Irp==NULL)
     {
	ObDeleteHandle(*FileHandle);
	*FileHandle=0;
	ExFreePool(FileObject);
	return(STATUS_UNSUCCESSFUL);
     }
   
   StackLoc = IoGetNextIrpStackLocation(Irp);
   StackLoc->MajorFunction = IRP_MJ_CREATE;
   StackLoc->MinorFunction = 0;
   StackLoc->Flags = 0;
   StackLoc->Control = 0;
   StackLoc->DeviceObject = DeviceObject;
   StackLoc->FileObject=FileObject;
   Status = IoCallDriver(DeviceObject,Irp);
   if (Status==STATUS_PENDING)
     {
	KeWaitForSingleObject(&Event,Executive,KernelMode,FALSE,NULL);
	Status = IoStatusBlock->Status;
     }
   
   if (Status!=STATUS_SUCCESS)
     {
	ObDeleteHandle(*FileHandle);
	*FileHandle=0;
	ExFreePool(FileObject);
     }
   
   return(Status);
}


