/* $Id: msfs.c,v 1.6 2003/09/20 20:31:57 weiden Exp $
 *
 * COPYRIGHT:  See COPYING in the top level directory
 * PROJECT:    ReactOS kernel
 * FILE:       services/fs/ms/msfs.c
 * PURPOSE:    Mailslot filesystem
 * PROGRAMMER: Eric Kohl <ekohl@rz-online.de>
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include "msfs.h"

#define NDEBUG
#include <debug.h>


/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL
DriverEntry(PDRIVER_OBJECT DriverObject,
	    PUNICODE_STRING RegistryPath)
{
   PMSFS_DEVICE_EXTENSION DeviceExtension;
   PDEVICE_OBJECT DeviceObject;
   UNICODE_STRING DeviceName;
   NTSTATUS Status;
   
   DbgPrint("Mailslot FSD 0.0.1\n");
   
   DriverObject->Flags = 0;
   DriverObject->MajorFunction[IRP_MJ_CREATE] = (PDRIVER_DISPATCH)MsfsCreate;
   DriverObject->MajorFunction[IRP_MJ_CREATE_MAILSLOT] =
     (PDRIVER_DISPATCH)MsfsCreateMailslot;
   DriverObject->MajorFunction[IRP_MJ_CLOSE] = (PDRIVER_DISPATCH)MsfsClose;
   DriverObject->MajorFunction[IRP_MJ_READ] = (PDRIVER_DISPATCH)MsfsRead;
   DriverObject->MajorFunction[IRP_MJ_WRITE] = (PDRIVER_DISPATCH)MsfsWrite;
   DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION] =
     (PDRIVER_DISPATCH)MsfsQueryInformation;
   DriverObject->MajorFunction[IRP_MJ_SET_INFORMATION] =
     (PDRIVER_DISPATCH)MsfsSetInformation;
//   DriverObject->MajorFunction[IRP_MJ_DIRECTORY_CONTROL] =
//     (PDRIVER_DISPATCH)MsfsDirectoryControl;
//   DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS] = (PDRIVER_DISPATCH)MsfsFlushBuffers;
//   DriverObject->MajorFunction[IRP_MJ_SHUTDOWN] = (PDRIVER_DISPATCH)MsfsShutdown;
//   DriverObject->MajorFunction[IRP_MJ_QUERY_SECURITY] = 
//     (PDRIVER_DISPATCH)MsfsQuerySecurity;
//   DriverObject->MajorFunction[IRP_MJ_SET_SECURITY] =
//     (PDRIVER_DISPATCH)MsfsSetSecurity;
   DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] =
     (PDRIVER_DISPATCH)MsfsFileSystemControl;
   
   DriverObject->DriverUnload = NULL;
   
   RtlInitUnicodeString(&DeviceName,
			L"\\Device\\MailSlot");
   Status = IoCreateDevice(DriverObject,
			   sizeof(MSFS_DEVICE_EXTENSION),
			   &DeviceName,
			   FILE_DEVICE_MAILSLOT,
			   0,
			   FALSE,
			   &DeviceObject);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }

   /* initialize device extension */
   DeviceExtension = DeviceObject->DeviceExtension;
   InitializeListHead(&DeviceExtension->MailslotListHead);
   KeInitializeMutex(&DeviceExtension->MailslotListLock,
		     0);
   
   return(STATUS_SUCCESS);
}

/* EOF */
