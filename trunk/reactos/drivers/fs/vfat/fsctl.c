/*
 *  ReactOS kernel
 *  Copyright (C) 2002 ReactOS Team
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
/* $Id: fsctl.c,v 1.35 2004/08/05 02:48:18 navaraf Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/fs/vfat/fsctl.c
 * PURPOSE:          VFAT Filesystem
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <ntos/kefuncs.h>
#include <rosrtl/string.h>
#include <wchar.h>

#define NDEBUG
#include <debug.h>

#include "vfat.h"

/* FUNCTIONS ****************************************************************/

#define  CACHEPAGESIZE(pDeviceExt) ((pDeviceExt)->FatInfo.BytesPerCluster > PAGE_SIZE ? \
		   (pDeviceExt)->FatInfo.BytesPerCluster : PAGE_SIZE)


static NTSTATUS
VfatHasFileSystem(PDEVICE_OBJECT DeviceToMount,
		  PBOOLEAN RecognizedFS,
		  PFATINFO pFatInfo)
{
   NTSTATUS Status;
   PARTITION_INFORMATION PartitionInfo;
   DISK_GEOMETRY DiskGeometry;
   FATINFO FatInfo;
   ULONG Size;
   ULONG Sectors;
   LARGE_INTEGER Offset;
   struct _BootSector* Boot;
   BOOL PartitionInfoIsValid = FALSE;

   DPRINT("VfatHasFileSystem\n");

   *RecognizedFS = FALSE;

   Size = sizeof(DISK_GEOMETRY);
   Status = VfatBlockDeviceIoControl(DeviceToMount,
				     IOCTL_DISK_GET_DRIVE_GEOMETRY,
				     NULL,
				     0,
				     &DiskGeometry,
				     &Size,
				     FALSE);
   if (!NT_SUCCESS(Status))
   {
      DPRINT("VfatBlockDeviceIoControl faild (%x)\n", Status);
      return Status;
   }
   FatInfo.FixedMedia = DiskGeometry.MediaType == FixedMedia ? TRUE : FALSE;
   if (DiskGeometry.MediaType == FixedMedia || DiskGeometry.MediaType == RemovableMedia)
   {
      // We have found a hard disk
      Size = sizeof(PARTITION_INFORMATION);
      Status = VfatBlockDeviceIoControl(DeviceToMount,
					IOCTL_DISK_GET_PARTITION_INFO,
					NULL,
					0,
					&PartitionInfo,
					&Size,
					FALSE);
      if (!NT_SUCCESS(Status))
      {
         DPRINT("VfatBlockDeviceIoControl faild (%x)\n", Status);
         return Status;
      }
      PartitionInfoIsValid = TRUE;
#if defined(DBG) && !defined(NDEBUG)
      DbgPrint("Partition Information:\n");
      DbgPrint("StartingOffset      %u\n", PartitionInfo.StartingOffset.QuadPart  / 512);
      DbgPrint("PartitionLength     %u\n", PartitionInfo.PartitionLength.QuadPart / 512);
      DbgPrint("HiddenSectors       %u\n", PartitionInfo.HiddenSectors);
      DbgPrint("PartitionNumber     %u\n", PartitionInfo.PartitionNumber);
      DbgPrint("PartitionType       %u\n", PartitionInfo.PartitionType);
      DbgPrint("BootIndicator       %u\n", PartitionInfo.BootIndicator);
      DbgPrint("RecognizedPartition %u\n", PartitionInfo.RecognizedPartition);
      DbgPrint("RewritePartition    %u\n", PartitionInfo.RewritePartition);
#endif
      if (PartitionInfo.PartitionType)
      {
         if (PartitionInfo.PartitionType == PARTITION_FAT_12       ||
             PartitionInfo.PartitionType == PARTITION_FAT_16       ||
             PartitionInfo.PartitionType == PARTITION_HUGE         ||
             PartitionInfo.PartitionType == PARTITION_FAT32        ||
             PartitionInfo.PartitionType == PARTITION_FAT32_XINT13 ||
             PartitionInfo.PartitionType == PARTITION_XINT13)
         {
            *RecognizedFS = TRUE;
         }
      }
      else if (DiskGeometry.MediaType == RemovableMedia &&
               PartitionInfo.PartitionNumber > 0 &&
               PartitionInfo.StartingOffset.QuadPart == 0LL &&
               PartitionInfo.PartitionLength.QuadPart > 0LL)
      {
         /* This is possible a removable media formated as super floppy */
         *RecognizedFS = TRUE;
      }
   }
   /*
    * Floppy disk driver can return Unknown as media type if it
    * doesn't know yet what floppy in the drive really is. This is
    * perfectly correct to do under Windows.
    */
   if (DiskGeometry.MediaType == Unknown)
   {
      *RecognizedFS = TRUE;
      DiskGeometry.BytesPerSector = 512;
   }
   if (DiskGeometry.MediaType > Unknown && DiskGeometry.MediaType < RemovableMedia )
   {
      *RecognizedFS = TRUE;
   }
   if (*RecognizedFS == FALSE)
   {
      return STATUS_SUCCESS;
   }

   Boot = ExAllocatePool(NonPagedPool, DiskGeometry.BytesPerSector);
   if (Boot == NULL)
   {
      *RecognizedFS=FALSE;
      return STATUS_INSUFFICIENT_RESOURCES;
   }

   Offset.QuadPart = 0;

   Status = VfatReadDisk(DeviceToMount, &Offset, DiskGeometry.BytesPerSector, (PUCHAR) Boot, FALSE);
   if (NT_SUCCESS(Status))
   {
      if (Boot->Signatur1 != 0xaa55)
      {
         DPRINT1("Signature %04x\n", Boot->Signatur1);
         *RecognizedFS=FALSE;
      }
      if (*RecognizedFS &&
	  Boot->BytesPerSector != 512 &&
	  Boot->BytesPerSector != 1024 &&
          Boot->BytesPerSector != 2048 && 
	  Boot->BytesPerSector != 4096)
      {
         DPRINT1("BytesPerSector %d\n", Boot->BytesPerSector);
         *RecognizedFS=FALSE;
      }

      if (*RecognizedFS &&
          Boot->FATCount != 1 && 
	  Boot->FATCount != 2)
      {
         DPRINT1("FATCount %d\n", Boot->FATCount);
         *RecognizedFS=FALSE;
      }

      if (*RecognizedFS &&
          Boot->Media != 0xf0 && 
	  Boot->Media != 0xf8 &&
	  Boot->Media != 0xf9 &&
	  Boot->Media != 0xfa && 
	  Boot->Media != 0xfb &&
	  Boot->Media != 0xfc &&
	  Boot->Media != 0xfd &&
	  Boot->Media != 0xfe && 
	  Boot->Media != 0xff)
      {
         DPRINT1("Media             %02x\n", Boot->Media);
         *RecognizedFS=FALSE;
      }

      if (*RecognizedFS &&
	  Boot->SectorsPerCluster != 1 &&
	  Boot->SectorsPerCluster != 2 &&
          Boot->SectorsPerCluster != 4 && 
	  Boot->SectorsPerCluster != 8 &&
          Boot->SectorsPerCluster != 16 &&
	  Boot->SectorsPerCluster != 32 && 
          Boot->SectorsPerCluster != 64 &&
	  Boot->SectorsPerCluster != 128)
      {
         DPRINT1("SectorsPerCluster %02x\n", Boot->SectorsPerCluster);
         *RecognizedFS=FALSE;
      }

      if (*RecognizedFS &&
          Boot->BytesPerSector * Boot->SectorsPerCluster > 32 * 1024)
      {
         DPRINT1("ClusterSize %dx\n", Boot->BytesPerSector * Boot->SectorsPerCluster);
         *RecognizedFS=FALSE;
      }

      if (*RecognizedFS)
      {
         FatInfo.VolumeID = Boot->VolumeID;
         FatInfo.FATStart = Boot->ReservedSectors;
         FatInfo.FATCount = Boot->FATCount;
         FatInfo.FATSectors = Boot->FATSectors ? Boot->FATSectors : ((struct _BootSector32*) Boot)->FATSectors32;
         FatInfo.BytesPerSector = Boot->BytesPerSector;
         FatInfo.SectorsPerCluster = Boot->SectorsPerCluster;
         FatInfo.BytesPerCluster = FatInfo.BytesPerSector * FatInfo.SectorsPerCluster;
         FatInfo.rootDirectorySectors = ((Boot->RootEntries * 32) + Boot->BytesPerSector - 1) / Boot->BytesPerSector;
         FatInfo.rootStart = FatInfo.FATStart + FatInfo.FATCount * FatInfo.FATSectors;
         FatInfo.dataStart = FatInfo.rootStart + FatInfo.rootDirectorySectors;
         FatInfo.Sectors = Sectors = Boot->Sectors ? Boot->Sectors : Boot->SectorsHuge;
         Sectors -= Boot->ReservedSectors + FatInfo.FATCount * FatInfo.FATSectors + FatInfo.rootDirectorySectors;
         FatInfo.NumberOfClusters = Sectors / Boot->SectorsPerCluster;
         if (FatInfo.NumberOfClusters < 4085)
         {
            DPRINT("FAT12\n");
            FatInfo.FatType = FAT12;
         }
         else if (FatInfo.NumberOfClusters >= 65525)
         {
            DPRINT("FAT32\n");
            FatInfo.FatType = FAT32;
            FatInfo.RootCluster = ((struct _BootSector32*) Boot)->RootCluster;
            FatInfo.rootStart = FatInfo.dataStart + ((FatInfo.RootCluster - 2) * FatInfo.SectorsPerCluster);
            FatInfo.VolumeID = ((struct _BootSector32*) Boot)->VolumeID;
         }
         else
         {
            DPRINT("FAT16\n");
            FatInfo.FatType = FAT16;
         }
	 if (PartitionInfoIsValid &&
	     FatInfo.Sectors > PartitionInfo.PartitionLength.QuadPart / FatInfo.BytesPerSector)
	 {
	    CHECKPOINT1;
	    *RecognizedFS = FALSE;
	 }
	    
         if (pFatInfo && *RecognizedFS)
         {
            *pFatInfo = FatInfo;
         }
      }
   }

   ExFreePool(Boot);
   DPRINT("VfatHasFileSystem done\n");
   return Status;
}

static NTSTATUS
VfatMountDevice(PDEVICE_EXTENSION DeviceExt,
		PDEVICE_OBJECT DeviceToMount)
/*
 * FUNCTION: Mounts the device
 */
{
   NTSTATUS Status;
   BOOLEAN RecognizedFS;

   DPRINT("Mounting VFAT device...\n");

   Status = VfatHasFileSystem(DeviceToMount, &RecognizedFS, &DeviceExt->FatInfo);
   if (!NT_SUCCESS(Status))
   {
      return(Status);
   }
   DPRINT("MountVfatdev %d, PAGE_SIZE = %d\n", DeviceExt->FatInfo.BytesPerCluster, PAGE_SIZE);


   return(STATUS_SUCCESS);
}


static NTSTATUS
VfatMount (PVFAT_IRP_CONTEXT IrpContext)
/*
 * FUNCTION: Mount the filesystem
 */
{
   PDEVICE_OBJECT DeviceObject = NULL;
   PDEVICE_EXTENSION DeviceExt = NULL;
   BOOLEAN RecognizedFS;
   NTSTATUS Status;
   PVFATFCB Fcb = NULL;
   PVFATFCB VolumeFcb = NULL;
   PVFATCCB Ccb = NULL;
   PDEVICE_OBJECT DeviceToMount;
   UNICODE_STRING NameU;

   DPRINT("VfatMount(IrpContext %x)\n", IrpContext);

   assert (IrpContext);

   if (IrpContext->DeviceObject != VfatGlobalData->DeviceObject)
   {
      Status = STATUS_INVALID_DEVICE_REQUEST;
      goto ByeBye;
   }

   DeviceToMount = IrpContext->Stack->Parameters.MountVolume.DeviceObject;

   Status = VfatHasFileSystem (DeviceToMount, &RecognizedFS, NULL);
   if (!NT_SUCCESS(Status))
   {
      goto ByeBye;
   }

   if (RecognizedFS == FALSE)
   {
      DPRINT("VFAT: Unrecognized Volume\n");
      Status = STATUS_UNRECOGNIZED_VOLUME;
      goto ByeBye;
   }

   DPRINT("VFAT: Recognized volume\n");
   Status = IoCreateDevice(VfatGlobalData->DriverObject,
			   sizeof (DEVICE_EXTENSION),
			   NULL,
			   FILE_DEVICE_FILE_SYSTEM,
			   0,
			   FALSE,
			   &DeviceObject);
   if (!NT_SUCCESS(Status))
   {
      goto ByeBye;
   }

   DeviceObject->Flags = DeviceObject->Flags | DO_DIRECT_IO;
   DeviceExt = (PVOID) DeviceObject->DeviceExtension;
   RtlZeroMemory(DeviceExt, sizeof(DEVICE_EXTENSION));

   /* use same vpb as device disk */
   DeviceObject->Vpb = DeviceToMount->Vpb;
   Status = VfatMountDevice(DeviceExt, DeviceToMount);
   if (!NT_SUCCESS(Status))
   {
      /* FIXME: delete device object */
      goto ByeBye;
   }

#ifndef NDEBUG
   DbgPrint("BytesPerSector:     %d\n", DeviceExt->FatInfo.BytesPerSector);
   DbgPrint("SectorsPerCluster:  %d\n", DeviceExt->FatInfo.SectorsPerCluster);
   DbgPrint("FATCount:           %d\n", DeviceExt->FatInfo.FATCount);
   DbgPrint("FATSectors:         %d\n", DeviceExt->FatInfo.FATSectors);
   DbgPrint("RootStart:          %d\n", DeviceExt->FatInfo.rootStart);
   DbgPrint("DataStart:          %d\n", DeviceExt->FatInfo.dataStart);
   if (DeviceExt->FatInfo.FatType == FAT32)
   {
      DbgPrint("RootCluster:        %d\n", DeviceExt->FatInfo.RootCluster);
   }
#endif

  DeviceExt->StorageDevice = DeviceToMount;
  DeviceExt->StorageDevice->Vpb->DeviceObject = DeviceObject;
  DeviceExt->StorageDevice->Vpb->RealDevice = DeviceExt->StorageDevice;
  DeviceExt->StorageDevice->Vpb->Flags |= VPB_MOUNTED;
  DeviceObject->StackSize = DeviceExt->StorageDevice->StackSize + 1;
  DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

  DPRINT("FsDeviceObject %lx\n", DeviceObject);

   DeviceExt->FATFileObject = IoCreateStreamFileObject(NULL, DeviceExt->StorageDevice);
   RtlRosInitUnicodeStringFromLiteral(&NameU, L"\\$$Fat$$");
   Fcb = vfatNewFCB(&NameU);
   if (Fcb == NULL)
   {
      Status = STATUS_INSUFFICIENT_RESOURCES;
      goto ByeBye;
   }
   Ccb = ExAllocateFromNPagedLookasideList(&VfatGlobalData->CcbLookasideList);
   if (Ccb == NULL)
   {
      Status =  STATUS_INSUFFICIENT_RESOURCES;
      goto ByeBye;
   }

   memset(Ccb, 0, sizeof (VFATCCB));
   DeviceExt->FATFileObject->Flags = DeviceExt->FATFileObject->Flags | FO_FCB_IS_VALID | FO_DIRECT_CACHE_PAGING_READ;
   DeviceExt->FATFileObject->FsContext = Fcb;
   DeviceExt->FATFileObject->FsContext2 = Ccb;
   DeviceExt->FATFileObject->SectionObjectPointer = &Fcb->SectionObjectPointers;
   DeviceExt->FATFileObject->PrivateCacheMap = NULL;
   DeviceExt->FATFileObject->Vpb = DeviceObject->Vpb;
   Fcb->FileObject = DeviceExt->FATFileObject;

   Fcb->Flags = FCB_IS_FAT;

   Fcb->RFCB.FileSize.QuadPart = DeviceExt->FatInfo.FATSectors * DeviceExt->FatInfo.BytesPerSector;
   Fcb->RFCB.ValidDataLength = Fcb->RFCB.FileSize;
   Fcb->RFCB.AllocationSize = Fcb->RFCB.FileSize;

   if (DeviceExt->FatInfo.FatType != FAT12)
   {
      Status = CcRosInitializeFileCache(DeviceExt->FATFileObject, CACHEPAGESIZE(DeviceExt));
   }
   else
   {
      Status = CcRosInitializeFileCache(DeviceExt->FATFileObject, 2 * PAGE_SIZE);
   }
   if (!NT_SUCCESS (Status))
   {
      DbgPrint ("CcRosInitializeFileCache failed\n");
      goto ByeBye;
   }
   DeviceExt->LastAvailableCluster = 2;
   ExInitializeResourceLite(&DeviceExt->DirResource);
   ExInitializeResourceLite(&DeviceExt->FatResource);

   switch (DeviceExt->FatInfo.FatType)
   {
      case FAT12:
         DeviceExt->GetNextCluster = FAT12GetNextCluster;
         DeviceExt->FindAndMarkAvailableCluster = FAT12FindAndMarkAvailableCluster;
         DeviceExt->WriteCluster = FAT12WriteCluster;
         break;

      case FAT16:
         DeviceExt->GetNextCluster = FAT16GetNextCluster;
         DeviceExt->FindAndMarkAvailableCluster = FAT16FindAndMarkAvailableCluster;
         DeviceExt->WriteCluster = FAT16WriteCluster;
         break;

      case FAT32:
         DeviceExt->GetNextCluster = FAT32GetNextCluster;
         DeviceExt->FindAndMarkAvailableCluster = FAT32FindAndMarkAvailableCluster;
         DeviceExt->WriteCluster = FAT32WriteCluster;
         break;
   }

   InitializeListHead(&DeviceExt->FcbListHead);
   RtlRosInitUnicodeStringFromLiteral(&NameU, L"\\$$Volume$$");

   VolumeFcb = vfatNewFCB(&NameU);
   if (VolumeFcb == NULL)
   {
      Status = STATUS_INSUFFICIENT_RESOURCES;
      goto ByeBye;
   }
   VolumeFcb->Flags = FCB_IS_VOLUME;
   VolumeFcb->RFCB.FileSize.QuadPart = DeviceExt->FatInfo.Sectors * DeviceExt->FatInfo.BytesPerSector;
   VolumeFcb->RFCB.ValidDataLength = VolumeFcb->RFCB.FileSize;
   VolumeFcb->RFCB.AllocationSize = VolumeFcb->RFCB.FileSize;
   DeviceExt->VolumeFcb = VolumeFcb;

   ExAcquireResourceExclusiveLite(&VfatGlobalData->VolumeListLock, TRUE);
   InsertHeadList(&VfatGlobalData->VolumeListHead, &DeviceExt->VolumeListEntry);
   ExReleaseResourceLite(&VfatGlobalData->VolumeListLock);

   /* read serial number */
   DeviceObject->Vpb->SerialNumber = DeviceExt->FatInfo.VolumeID;

   /* read volume label */
   ReadVolumeLabel(DeviceExt,  DeviceObject->Vpb);

   Status = STATUS_SUCCESS;
ByeBye:

  if (!NT_SUCCESS(Status))
  {
     // cleanup
     if (DeviceExt && DeviceExt->FATFileObject)
        ObDereferenceObject (DeviceExt->FATFileObject);
     if (Fcb)
        vfatDestroyFCB(Fcb);
     if (Ccb)
        vfatDestroyCCB(Ccb);
     if (DeviceObject)
       IoDeleteDevice(DeviceObject);
     if (VolumeFcb)
        vfatDestroyFCB(VolumeFcb);
  }
  return Status;
}


static NTSTATUS
VfatVerify (PVFAT_IRP_CONTEXT IrpContext)
/*
 * FUNCTION: Verify the filesystem
 */
{
  PDEVICE_OBJECT DeviceToVerify;
  NTSTATUS Status = STATUS_SUCCESS;
  FATINFO FatInfo;
  BOOLEAN RecognizedFS;
  PDEVICE_EXTENSION DeviceExt = IrpContext->DeviceExt;

  DPRINT("VfatVerify(IrpContext %x)\n", IrpContext);

  DeviceToVerify = IrpContext->Stack->Parameters.VerifyVolume.DeviceObject;
  Status = VfatBlockDeviceIoControl(DeviceToVerify,
				    IOCTL_DISK_CHECK_VERIFY,
				    NULL,
				    0,
				    NULL,
				    0,
				    TRUE);
  DeviceToVerify->Flags &= ~DO_VERIFY_VOLUME;
  if (!NT_SUCCESS(Status) && Status != STATUS_VERIFY_REQUIRED)
    {
      DPRINT("VfatBlockDeviceIoControl() failed (Status %lx)\n", Status);
      Status = STATUS_WRONG_VOLUME;
    }
  else
    {
      Status = VfatHasFileSystem(DeviceToVerify, &RecognizedFS, &FatInfo);
      if (!NT_SUCCESS(Status) || RecognizedFS == FALSE)
        {
          Status = STATUS_WRONG_VOLUME;
        }
      else if (sizeof(FATINFO) == RtlCompareMemory(&FatInfo, &DeviceExt->FatInfo, sizeof(FATINFO)))
        {
          /*
           * FIXME:
           *   Preformated floppy disks have very often a serial number of 0000:0000. 
           *   We should calculate a crc sum over the sectors from the root directory as secondary volume number. 
	   *   Each write to the root directory must update this crc sum.
           */
  
        }
      else
      	{
      	  Status = STATUS_WRONG_VOLUME;
        }
     }
    
  return Status;
}


static NTSTATUS
VfatGetVolumeBitmap(PVFAT_IRP_CONTEXT IrpContext)
{
   DPRINT("VfatGetVolumeBitmap (IrpContext %x)\n", IrpContext);

   return STATUS_INVALID_DEVICE_REQUEST;
}


static NTSTATUS 
VfatGetRetrievalPointers(PVFAT_IRP_CONTEXT IrpContext)
{
  PIO_STACK_LOCATION Stack;
   LARGE_INTEGER Vcn;
   PGET_RETRIEVAL_DESCRIPTOR RetrievalPointers;
   PFILE_OBJECT FileObject;
   ULONG MaxExtentCount;
   PVFATFCB Fcb;
   PDEVICE_EXTENSION DeviceExt;
   ULONG FirstCluster;
   ULONG CurrentCluster;
   ULONG LastCluster;
   NTSTATUS Status;

   DPRINT("VfatGetRetrievalPointers(IrpContext %x)\n", IrpContext);

   DeviceExt = IrpContext->DeviceExt;
   FileObject = IrpContext->FileObject;
   Stack = IrpContext->Stack;
   if (Stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(LARGE_INTEGER) ||
       Stack->Parameters.DeviceIoControl.Type3InputBuffer == NULL)
   {
      return STATUS_INVALID_PARAMETER;
   }
   if (IrpContext->Irp->UserBuffer == NULL ||
       Stack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(GET_RETRIEVAL_DESCRIPTOR) + sizeof(MAPPING_PAIR))
   {
      return STATUS_BUFFER_TOO_SMALL;
   }

   Fcb = FileObject->FsContext;

   ExAcquireResourceSharedLite(&Fcb->MainResource, TRUE);
   
   Vcn = *(PLARGE_INTEGER)Stack->Parameters.DeviceIoControl.Type3InputBuffer;
   RetrievalPointers = IrpContext->Irp->UserBuffer;

   MaxExtentCount = ((Stack->Parameters.DeviceIoControl.OutputBufferLength - sizeof(GET_RETRIEVAL_DESCRIPTOR)) / sizeof(MAPPING_PAIR));


   if (Vcn.QuadPart >= Fcb->RFCB.AllocationSize.QuadPart / DeviceExt->FatInfo.BytesPerCluster)
   {
      Status = STATUS_INVALID_PARAMETER;
      goto ByeBye;
   }

   CurrentCluster = FirstCluster = vfatDirEntryGetFirstCluster(DeviceExt, &Fcb->entry);
   Status = OffsetToCluster(DeviceExt, FirstCluster, 
                            Vcn.u.LowPart * DeviceExt->FatInfo.BytesPerCluster, 
			    &CurrentCluster, FALSE);
   if (!NT_SUCCESS(Status))
   {
      goto ByeBye;
   }

   RetrievalPointers->StartVcn = Vcn.QuadPart;
   RetrievalPointers->NumberOfPairs = 0;
   RetrievalPointers->Pair[0].Lcn = CurrentCluster - 2;
   LastCluster = 0;
   while (CurrentCluster != 0xffffffff && RetrievalPointers->NumberOfPairs < MaxExtentCount)
   {

      LastCluster = CurrentCluster;
      Status = NextCluster(DeviceExt, CurrentCluster, &CurrentCluster, FALSE);
      Vcn.QuadPart++;
      if (!NT_SUCCESS(Status))
      {
         goto ByeBye;
      }
      
      if (LastCluster + 1 != CurrentCluster)
      {
	 RetrievalPointers->Pair[RetrievalPointers->NumberOfPairs].Vcn = Vcn.QuadPart;
	 RetrievalPointers->NumberOfPairs++;
	 if (RetrievalPointers->NumberOfPairs < MaxExtentCount)
	 {
	    RetrievalPointers->Pair[RetrievalPointers->NumberOfPairs].Lcn = CurrentCluster - 2;
	 }
      }
   }
   
   IrpContext->Irp->IoStatus.Information = sizeof(GET_RETRIEVAL_DESCRIPTOR) + sizeof(MAPPING_PAIR) * RetrievalPointers->NumberOfPairs;
   Status = STATUS_SUCCESS;

ByeBye:
   ExReleaseResourceLite(&Fcb->MainResource);

   return Status;
}

static NTSTATUS
VfatMoveFile(PVFAT_IRP_CONTEXT IrpContext)
{
   DPRINT("VfatMoveFile(IrpContext %x)\n", IrpContext);

   return STATUS_INVALID_DEVICE_REQUEST;
}

static NTSTATUS
VfatRosQueryLcnMapping(PVFAT_IRP_CONTEXT IrpContext)
{
   PDEVICE_EXTENSION DeviceExt;
   PROS_QUERY_LCN_MAPPING LcnQuery;
   PIO_STACK_LOCATION Stack;

   DPRINT("VfatGetRetrievalPointers(IrpContext %x)\n", IrpContext);

   DeviceExt = IrpContext->DeviceExt;
   Stack = IrpContext->Stack;
   if (IrpContext->Irp->AssociatedIrp.SystemBuffer == NULL ||
       Stack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(ROS_QUERY_LCN_MAPPING))
   {
      return STATUS_BUFFER_TOO_SMALL;
   }
   LcnQuery = (PROS_QUERY_LCN_MAPPING)(IrpContext->Irp->AssociatedIrp.SystemBuffer);
   LcnQuery->LcnDiskOffset.QuadPart = DeviceExt->FatInfo.dataStart * DeviceExt->FatInfo.BytesPerSector;
   IrpContext->Irp->IoStatus.Information = sizeof(ROS_QUERY_LCN_MAPPING);
   return(STATUS_SUCCESS);
}

NTSTATUS VfatFileSystemControl(PVFAT_IRP_CONTEXT IrpContext)
/*
 * FUNCTION: File system control
 */
{

   NTSTATUS Status;

   DPRINT("VfatFileSystemControl(IrpContext %x)\n", IrpContext);

   assert (IrpContext);
   assert (IrpContext->Irp);
   assert (IrpContext->Stack);

   IrpContext->Irp->IoStatus.Information = 0;

   switch (IrpContext->MinorFunction)
   {
      case IRP_MN_USER_FS_REQUEST:
         switch(IrpContext->Stack->Parameters.DeviceIoControl.IoControlCode)
	 {
	    case FSCTL_GET_VOLUME_BITMAP:
               Status = VfatGetVolumeBitmap(IrpContext);
	       break;
	    case FSCTL_GET_RETRIEVAL_POINTERS:
               Status = VfatGetRetrievalPointers(IrpContext);
	       break;
	    case FSCTL_MOVE_FILE:
	       Status = VfatMoveFile(IrpContext);
	       break;
 	    case FSCTL_ROS_QUERY_LCN_MAPPING:
	       Status = VfatRosQueryLcnMapping(IrpContext);
	       break;
	    default:
	       Status = STATUS_INVALID_DEVICE_REQUEST;
	 }
	 break;

      case IRP_MN_MOUNT_VOLUME:
         Status = VfatMount(IrpContext);
	 break;

      case IRP_MN_VERIFY_VOLUME:
	DPRINT("VFATFS: IRP_MN_VERIFY_VOLUME\n");
	 Status = VfatVerify(IrpContext);
	 break;

      default:
	   DPRINT("VFAT FSC: MinorFunction %d\n", IrpContext->MinorFunction);
	   Status = STATUS_INVALID_DEVICE_REQUEST;
	   break;
   }

   IrpContext->Irp->IoStatus.Status = Status;

   IoCompleteRequest (IrpContext->Irp, IO_NO_INCREMENT);
   VfatFreeIrpContext(IrpContext);
   return (Status);
}

