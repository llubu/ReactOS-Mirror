/*
 *  ReactOS kernel
 *  Copyright (C) 2000  David Welch <welch@cwcom.net>
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
/*
 * PROJECT:          ReactOS kernel
 * FILE:             services/fs/minix/minix.c
 * PURPOSE:          Minix FSD
 * PROGRAMMER:       David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 */

/* INCLUDES *****************************************************************/

#include <ntddk.h>
#include <ntifs.h>

//#define NDEBUG
#include <debug.h>

#include "minix.h"

/* GLOBALS *******************************************************************/

static PDRIVER_OBJECT DriverObject;

/* FUNCTIONS ****************************************************************/

VOID MinixMount(PDEVICE_OBJECT DeviceToMount)
{
  PDEVICE_OBJECT DeviceObject;
  MINIX_DEVICE_EXTENSION* DeviceExt;

  IoCreateDevice(DriverObject,
		 sizeof(MINIX_DEVICE_EXTENSION),
		 NULL,
		 FILE_DEVICE_FILE_SYSTEM,
		 0,
		 FALSE,
		 &DeviceObject);
  DeviceObject->Flags = DeviceObject->Flags | DO_DIRECT_IO;
  DeviceExt = DeviceObject->DeviceExtension;

  MinixReadSector(DeviceToMount,1,DeviceExt->superblock_buf);
  DeviceExt->sb = (struct minix_super_block *)(DeviceExt->superblock_buf);

  DeviceExt->StorageDevice = DeviceToMount;
  DeviceExt->StorageDevice->Vpb->DeviceObject = DeviceObject;
  DeviceExt->StorageDevice->Vpb->RealDevice = DeviceExt->StorageDevice;
  DeviceExt->StorageDevice->Vpb->Flags |= VPB_MOUNTED;
  DeviceObject->StackSize = DeviceExt->StorageDevice->StackSize + 1;
  DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

  DeviceExt->FileObject = IoCreateStreamFileObject(NULL, DeviceObject);
}

NTSTATUS STDCALL
MinixFileSystemControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
   PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
//   PVPB	vpb = Stack->Parameters.Mount.Vpb;
   PDEVICE_OBJECT DeviceToMount = Stack->Parameters.Mount.DeviceObject;
   NTSTATUS Status;
   char* superblock_buf;
   struct minix_super_block* sb;

   DbgPrint("MinixFileSystemControl(DeviceObject %x, Irp %x)\n",DeviceObject,
	  Irp);
   DPRINT("DeviceToMount %x\n",DeviceToMount);

   superblock_buf = ExAllocatePool(NonPagedPool,BLOCKSIZE);

   DPRINT("MinixReadSector %x\n",MinixReadSector);
   MinixReadSector(DeviceToMount,1,superblock_buf);
   sb = (struct minix_super_block *)superblock_buf;
   DPRINT("Magic %x\n",sb->s_magic);
   DPRINT("Imap blocks %x\n",sb->s_imap_blocks);
   DPRINT("Zmap blocks %x\n",sb->s_zmap_blocks);
   if (sb->s_magic==MINIX_SUPER_MAGIC2)
     {
	DPRINT("%s() = STATUS_SUCCESS\n",__FUNCTION__);
	MinixMount(DeviceToMount);
	Status = STATUS_SUCCESS;
     }
   else
     {
	DPRINT("%s() = STATUS_UNRECOGNIZED_VOLUME\n",__FUNCTION__);
	Status = STATUS_UNRECOGNIZED_VOLUME;
     }

   Irp->IoStatus.Status = Status;
   Irp->IoStatus.Information = 0;

   IoCompleteRequest(Irp, IO_NO_INCREMENT);
   return(Status);
}

NTSTATUS STDCALL
DriverEntry(PDRIVER_OBJECT _DriverObject,
	    PUNICODE_STRING RegistryPath)
/*
 * FUNCTION: Called by the system to initalize the driver
 * ARGUMENTS:
 *           DriverObject = object describing this driver
 *           RegistryPath = path to our configuration entries
 * RETURNS: Success or failure
 */
{
   PDEVICE_OBJECT DeviceObject;
   NTSTATUS ret;
   UNICODE_STRING DeviceName;

   DbgPrint("Minix FSD 0.0.1\n");

   DriverObject = _DriverObject;

   RtlInitUnicodeString(&DeviceName,
			L"\\Device\\Minix");
   ret = IoCreateDevice(DriverObject,
			0,
			&DeviceName,
			FILE_DEVICE_FILE_SYSTEM,
			0,
			FALSE,
			&DeviceObject);
   if (!NT_SUCCESS(ret))
     {
	return(ret);
     }

   DeviceObject->Flags = 0;
   DriverObject->MajorFunction[IRP_MJ_CLOSE] = MinixClose;
   DriverObject->MajorFunction[IRP_MJ_CREATE] = MinixCreate;
   DriverObject->MajorFunction[IRP_MJ_READ] = MinixRead;
   DriverObject->MajorFunction[IRP_MJ_WRITE] = MinixWrite;
   DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] =
                      MinixFileSystemControl;
   DriverObject->MajorFunction[IRP_MJ_DIRECTORY_CONTROL] =
                      MinixDirectoryControl;
   DriverObject->DriverUnload = NULL;

   IoRegisterFileSystem(DeviceObject);

   return(STATUS_SUCCESS);
}

