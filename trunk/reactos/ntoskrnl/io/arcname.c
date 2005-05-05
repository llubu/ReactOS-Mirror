/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/arcname.c
 * PURPOSE:         Creates ARC names for boot devices
 * 
 * PROGRAMMERS:     Eric Kohl (ekohl@rz-online.de)
 */


/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* MACROS *******************************************************************/

#define FS_VOLUME_BUFFER_SIZE (MAX_PATH + sizeof(FILE_FS_VOLUME_INFORMATION))

/* FUNCTIONS ****************************************************************/

NTSTATUS INIT_FUNCTION
IoCreateArcNames(VOID)
{
  PCONFIGURATION_INFORMATION ConfigInfo;
  PDRIVE_LAYOUT_INFORMATION LayoutInfo = NULL;
  WCHAR DeviceNameBuffer[80];
  WCHAR ArcNameBuffer[80];
  UNICODE_STRING DeviceName;
  UNICODE_STRING ArcName;
  ULONG i, j, k;
  NTSTATUS Status;
  PFILE_OBJECT FileObject;
  PDEVICE_OBJECT DeviceObject;
  BOOL IsRemovableMedia;

  DPRINT("IoCreateArcNames() called\n");

  ConfigInfo = IoGetConfigurationInformation();

  /* create ARC names for floppy drives */
  DPRINT("Floppy drives: %lu\n", ConfigInfo->FloppyCount);
  for (i = 0; i < ConfigInfo->FloppyCount; i++)
    {
      swprintf(DeviceNameBuffer,
	       L"\\Device\\Floppy%lu",
	       i);
      RtlInitUnicodeString(&DeviceName,
			   DeviceNameBuffer);

      swprintf(ArcNameBuffer,
	       L"\\ArcName\\multi(0)disk(0)fdisk(%lu)",
	       i);
      RtlInitUnicodeString(&ArcName,
			   ArcNameBuffer);
      DPRINT("%wZ ==> %wZ\n",
	     &ArcName,
	     &DeviceName);

      Status = IoAssignArcName(&ArcName,
			       &DeviceName);
      if (!NT_SUCCESS(Status))
	return(Status);
    }

  /* create ARC names for hard disk drives */
  DPRINT("Disk drives: %lu\n", ConfigInfo->DiskCount);
  for (i = 0, k = 0; i < ConfigInfo->DiskCount; i++)
    {
      swprintf(DeviceNameBuffer,
	       L"\\Device\\Harddisk%lu\\Partition0",
	       i);
      RtlInitUnicodeString(&DeviceName,
			   DeviceNameBuffer);


      Status = IoGetDeviceObjectPointer(&DeviceName,
				        FILE_READ_DATA,
				        &FileObject,
				        &DeviceObject);
      if (!NT_SUCCESS(Status))
        {
	  continue;
	}
      IsRemovableMedia = DeviceObject->Characteristics & FILE_REMOVABLE_MEDIA ? TRUE : FALSE;
      ObDereferenceObject(FileObject);
      if (IsRemovableMedia)
        {
          continue;
	}

      swprintf(ArcNameBuffer,
	       L"\\ArcName\\multi(0)disk(0)rdisk(%lu)partition(0)",
	       k);
      RtlInitUnicodeString(&ArcName,
			   ArcNameBuffer);
      DPRINT("%wZ ==> %wZ\n",
	     &ArcName,
	     &DeviceName);

      Status = IoAssignArcName(&ArcName,
			       &DeviceName);
      if (!NT_SUCCESS(Status))
	return(Status);

      Status = xHalQueryDriveLayout(&DeviceName,
				    &LayoutInfo);
      if (!NT_SUCCESS(Status))
	return(Status);

      DPRINT("Number of partitions: %u\n", LayoutInfo->PartitionCount);

      for (j = 0;j < LayoutInfo->PartitionCount; j++)
	{
	  swprintf(DeviceNameBuffer,
		   L"\\Device\\Harddisk%lu\\Partition%lu",
		   i,
		   j + 1);
	  RtlInitUnicodeString(&DeviceName,
			       DeviceNameBuffer);

	  swprintf(ArcNameBuffer,
		   L"\\ArcName\\multi(0)disk(0)rdisk(%lu)partition(%lu)",
		   k,
		   j + 1);
	  RtlInitUnicodeString(&ArcName,
			       ArcNameBuffer);
	  DPRINT("%wZ ==> %wZ\n",
		 &ArcName,
		 &DeviceName);

	  Status = IoAssignArcName(&ArcName,
				   &DeviceName);
	  if (!NT_SUCCESS(Status))
	    return(Status);
	}

      ExFreePool(LayoutInfo);
      LayoutInfo = NULL;
      k++;
    }

  /* create ARC names for cdrom drives */
  DPRINT("CD-ROM drives: %lu\n", ConfigInfo->CdRomCount);
  for (i = 0; i < ConfigInfo->CdRomCount; i++)
    {
      swprintf(DeviceNameBuffer,
	       L"\\Device\\CdRom%lu",
	       i);
      RtlInitUnicodeString(&DeviceName,
			   DeviceNameBuffer);

      swprintf(ArcNameBuffer,
	       L"\\ArcName\\multi(0)disk(0)cdrom(%lu)",
	       i);
      RtlInitUnicodeString(&ArcName,
			   ArcNameBuffer);
      DPRINT("%wZ ==> %wZ\n",
	     &ArcName,
	     &DeviceName);

      Status = IoAssignArcName(&ArcName,
			       &DeviceName);
      if (!NT_SUCCESS(Status))
	return(Status);
    }

  DPRINT("IoCreateArcNames() done\n");

  return(STATUS_SUCCESS);
}


static NTSTATUS
IopCheckCdromDevices(PULONG DeviceNumber)
{
  PCONFIGURATION_INFORMATION ConfigInfo;
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING DeviceName;
  WCHAR DeviceNameBuffer[MAX_PATH];
  HANDLE Handle;
  ULONG i;
  NTSTATUS Status;
  IO_STATUS_BLOCK IoStatusBlock;
#if 0
  PFILE_FS_VOLUME_INFORMATION FileFsVolume;
  USHORT Buffer[FS_VOLUME_BUFFER_SIZE];

  FileFsVolume = (PFILE_FS_VOLUME_INFORMATION)Buffer;
#endif

  ConfigInfo = IoGetConfigurationInformation();
  for (i = 0; i < ConfigInfo->CdRomCount; i++)
    {
#if 0
      swprintf(DeviceNameBuffer,
	       L"\\Device\\CdRom%lu\\",
	       i);
      RtlInitUnicodeString(&DeviceName,
			   DeviceNameBuffer);

      InitializeObjectAttributes(&ObjectAttributes,
				 &DeviceName,
				 0,
				 NULL,
				 NULL);

      Status = ZwOpenFile(&Handle,
			  FILE_ALL_ACCESS,
			  &ObjectAttributes,
			  &IoStatusBlock,
			  0,
			  0);
      DPRINT("ZwOpenFile()  DeviceNumber %lu  Status %lx\n", i, Status);
      if (NT_SUCCESS(Status))
	{
	  Status = ZwQueryVolumeInformationFile(Handle,
						&IoStatusBlock,
						FileFsVolume,
						FS_VOLUME_BUFFER_SIZE,
						FileFsVolumeInformation);
	  DPRINT("ZwQueryVolumeInformationFile()  Status %lx\n", Status);
	  if (NT_SUCCESS(Status))
	    {
	      DPRINT("VolumeLabel: '%S'\n", FileFsVolume->VolumeLabel);
	      if (_wcsicmp(FileFsVolume->VolumeLabel, L"REACTOS") == 0)
		{
		  ZwClose(Handle);
		  *DeviceNumber = i;
		  return(STATUS_SUCCESS);
		}
	    }
	  ZwClose(Handle);
	}
#endif

      /*
       * Check for 'reactos/ntoskrnl.exe' first...
       */

      swprintf(DeviceNameBuffer,
	       L"\\Device\\CdRom%lu\\reactos\\ntoskrnl.exe",
	       i);
      RtlInitUnicodeString(&DeviceName,
			   DeviceNameBuffer);

      InitializeObjectAttributes(&ObjectAttributes,
				 &DeviceName,
				 0,
				 NULL,
				 NULL);

      Status = ZwOpenFile(&Handle,
			  FILE_ALL_ACCESS,
			  &ObjectAttributes,
			  &IoStatusBlock,
			  0,
			  0);
      DPRINT("ZwOpenFile()  DeviceNumber %lu  Status %lx\n", i, Status);
      if (NT_SUCCESS(Status))
	{
	  DPRINT("Found ntoskrnl.exe on Cdrom%lu\n", i);
	  ZwClose(Handle);
	  *DeviceNumber = i;
	  return(STATUS_SUCCESS);
	}

      /*
       * ...and for 'reactos/system32/ntoskrnl.exe' also.
       */

      swprintf(DeviceNameBuffer,
	       L"\\Device\\CdRom%lu\\reactos\\system32\\ntoskrnl.exe",
	       i);
      RtlInitUnicodeString(&DeviceName,
			   DeviceNameBuffer);

      InitializeObjectAttributes(&ObjectAttributes,
				 &DeviceName,
				 0,
				 NULL,
				 NULL);

      Status = ZwOpenFile(&Handle,
			  FILE_ALL_ACCESS,
			  &ObjectAttributes,
			  &IoStatusBlock,
			  0,
			  0);
      DPRINT("ZwOpenFile()  DeviceNumber %lu  Status %lx\n", i, Status);
      if (NT_SUCCESS(Status))
	{
	  DPRINT("Found ntoskrnl.exe on Cdrom%lu\n", i);
	  ZwClose(Handle);
	  *DeviceNumber = i;
	  return(STATUS_SUCCESS);
	}
    }

  DPRINT("Could not find ntoskrnl.exe\n");
  *DeviceNumber = (ULONG)-1;

  return(STATUS_UNSUCCESSFUL);
}


NTSTATUS INIT_FUNCTION
IoCreateSystemRootLink(PCHAR ParameterLine)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  IO_STATUS_BLOCK IoStatusBlock;
  UNICODE_STRING LinkName;
  UNICODE_STRING DeviceName;
  UNICODE_STRING ArcName;
  UNICODE_STRING BootPath;
  PCHAR ParamBuffer;
  PWCHAR ArcNameBuffer;
  PCHAR p;
  NTSTATUS Status;
  ULONG Length;
  HANDLE Handle;

  /* Create local parameter line copy */
  ParamBuffer = ExAllocatePool(PagedPool, 256);
  strcpy(ParamBuffer, (char *)ParameterLine);

  DPRINT("%s\n", ParamBuffer);
  /* Format: <arc_name>\<path> [options...] */

  /* cut options off */
  p = strchr(ParamBuffer, ' ');
  if (p)
    *p = 0;
  DPRINT("%s\n", ParamBuffer);

  /* extract path */
  p = strchr(ParamBuffer, '\\');
  if (p)
    {
      DPRINT("Boot path: %s\n", p);
      RtlCreateUnicodeStringFromAsciiz(&BootPath, p);
      *p = 0;
    }
  else
    {
      DPRINT("Boot path: %s\n", "\\");
      RtlCreateUnicodeStringFromAsciiz(&BootPath, "\\");
    }
  DPRINT("ARC name: %s\n", ParamBuffer);

  p = strstr(ParamBuffer, "cdrom");
  if (p != NULL)
    {
      ULONG DeviceNumber;

      DPRINT("Booting from CD-ROM!\n");
      Status = IopCheckCdromDevices(&DeviceNumber);
      if (!NT_SUCCESS(Status))
	{
	  CPRINT("Failed to find setup disk!\n");
	  return(Status);
	}

      sprintf(p, "cdrom(%lu)", DeviceNumber);

      DPRINT("New ARC name: %s\n", ParamBuffer);

      /* Adjust original command line */
      p = strstr(ParameterLine, "cdrom");
      if (p != NULL);
	{
	  char temp[256];
	  char *q;

	  q = strchr(p, ')');
	  if (q != NULL)
	    {

	      q++;
	      strcpy(temp, q);
	      sprintf(p, "cdrom(%lu)", DeviceNumber);
	      strcat(p, temp);
	    }
	}
    }

  /* Only arc name left - build full arc name */
  ArcNameBuffer = ExAllocatePool(PagedPool, 256 * sizeof(WCHAR));
  swprintf(ArcNameBuffer,
	   L"\\ArcName\\%S", ParamBuffer);
  RtlInitUnicodeString(&ArcName, ArcNameBuffer);
  DPRINT("Arc name: %wZ\n", &ArcName);

  /* free ParamBuffer */
  ExFreePool(ParamBuffer);

  /* allocate device name string */
  DeviceName.Length = 0;
  DeviceName.MaximumLength = 256 * sizeof(WCHAR);
  DeviceName.Buffer = ExAllocatePool(PagedPool, 256 * sizeof(WCHAR));

  InitializeObjectAttributes(&ObjectAttributes,
			     &ArcName,
			     OBJ_OPENLINK,
			     NULL,
			     NULL);

  Status = ZwOpenSymbolicLinkObject(&Handle,
				    SYMBOLIC_LINK_ALL_ACCESS,
				    &ObjectAttributes);
  if (!NT_SUCCESS(Status))
    {
      RtlFreeUnicodeString(&BootPath);
      ExFreePool(DeviceName.Buffer);
      CPRINT("ZwOpenSymbolicLinkObject() '%wZ' failed (Status %x)\n",
	     &ArcName,
	     Status);
      ExFreePool(ArcName.Buffer);

      return(Status);
    }
  ExFreePool(ArcName.Buffer);

  Status = ZwQuerySymbolicLinkObject(Handle,
				     &DeviceName,
				     &Length);
  ZwClose (Handle);
  if (!NT_SUCCESS(Status))
    {
      RtlFreeUnicodeString(&BootPath);
      ExFreePool(DeviceName.Buffer);
      CPRINT("ZwQuerySymbolicObject() failed (Status %x)\n",
	     Status);

      return(Status);
    }
  DPRINT("Length: %lu DeviceName: %wZ\n", Length, &DeviceName);

  RtlAppendUnicodeStringToString(&DeviceName,
				 &BootPath);

  RtlFreeUnicodeString(&BootPath);
  DPRINT("DeviceName: %wZ\n", &DeviceName);

  /* create the '\SystemRoot' link */
  RtlRosInitUnicodeStringFromLiteral(&LinkName,
		       L"\\SystemRoot");

  Status = IoCreateSymbolicLink(&LinkName,
				&DeviceName);
  ExFreePool(DeviceName.Buffer);
  if (!NT_SUCCESS(Status))
    {
      CPRINT("IoCreateSymbolicLink() failed (Status %x)\n",
	     Status);

      return(Status);
    }

  /* Check whether '\SystemRoot'(LinkName) can be opened */
  InitializeObjectAttributes(&ObjectAttributes,
			     &LinkName,
			     0,
			     NULL,
			     NULL);

  Status = ZwOpenFile(&Handle,
		      FILE_ALL_ACCESS,
		      &ObjectAttributes,
		      &IoStatusBlock,
		      0,
		      0);
  if (!NT_SUCCESS(Status))
    {
      CPRINT("ZwOpenFile() failed to open '\\SystemRoot' (Status %x)\n",
	     Status);
      return(Status);
    }

  ZwClose(Handle);

  return(STATUS_SUCCESS);
}

/* EOF */
