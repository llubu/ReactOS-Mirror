
/* $Id: rw.c,v 1.33 2001/10/11 15:39:51 hbirr Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/fs/vfat/rw.c
 * PURPOSE:          VFAT Filesystem
 * PROGRAMMER:       Jason Filby (jasonfilby@yahoo.com)
 *
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <wchar.h>
#include <ntos/minmax.h>

#define NDEBUG
#include <debug.h>

#include "vfat.h"

/* GLOBALS *******************************************************************/

#define ROUND_UP(N, S) ((((N) + (S) - 1) / (S)) * (S))
#define ROUND_DOWN(N, S) ((N) - ((N) % (S)))

/* FUNCTIONS *****************************************************************/

NTSTATUS
NextCluster(PDEVICE_EXTENSION DeviceExt,
	    ULONG FirstCluster,
	    PULONG CurrentCluster,
	    BOOLEAN Extend)
     /*
      * Return the next cluster in a FAT chain, possibly extending the chain if
      * necessary
      */
{
  if (FirstCluster == 1)
  {
    (*CurrentCluster) += DeviceExt->Boot->SectorsPerCluster;
      return(STATUS_SUCCESS);
    }
  else
 /* CN: FIXME: Real bug here or in dirwr, where CurrentCluster isn't initialized when 0*/
  if (FirstCluster == 0)
    {
      NTSTATUS Status;

      Status = GetNextCluster(DeviceExt, 0, CurrentCluster,
			      Extend);
      return(Status);
    }
  else
    {
      NTSTATUS Status;

      Status = GetNextCluster(DeviceExt, (*CurrentCluster), CurrentCluster,
			      Extend);
      return(Status);
    }
}

NTSTATUS
OffsetToCluster(PDEVICE_EXTENSION DeviceExt,
		ULONG FirstCluster,
		ULONG FileOffset,
		PULONG Cluster,
		BOOLEAN Extend)
     /*
      * Return the cluster corresponding to an offset within a file,
      * possibly extending the file if necessary
      */
{
  ULONG CurrentCluster;
  ULONG i;
  NTSTATUS Status;

  if (FirstCluster == 1)
    {
      /* root of FAT16 or FAT12 */
      *Cluster = DeviceExt->rootStart + FileOffset
	/ (DeviceExt->BytesPerCluster) * DeviceExt->Boot->SectorsPerCluster;
      return(STATUS_SUCCESS);
    }
  else
    {
      CurrentCluster = FirstCluster;
      for (i = 0; i < FileOffset / DeviceExt->BytesPerCluster; i++)
	{
	  Status = GetNextCluster (DeviceExt, CurrentCluster, &CurrentCluster,
				   Extend);
	  if (!NT_SUCCESS(Status))
	    {
	      return(Status);
	    }
	}
      *Cluster = CurrentCluster;
      return(STATUS_SUCCESS);
    }
}

NTSTATUS
VfatReadCluster(PDEVICE_EXTENSION DeviceExt,
		ULONG FirstCluster,
		PULONG CurrentCluster,
		PVOID Destination,
		ULONG InternalOffset,
		ULONG InternalLength)
{
  PVOID BaseAddress = NULL;
  NTSTATUS Status;

  if (InternalLength == DeviceExt->BytesPerCluster)
  {
    Status = VfatRawReadCluster(DeviceExt, FirstCluster,
                                Destination, *CurrentCluster, 1);
  }
  else
  {
    BaseAddress = ExAllocatePool(NonPagedPool, DeviceExt->BytesPerCluster);
    if (BaseAddress == NULL)
    {
      return(STATUS_NO_MEMORY);
    }
    Status = VfatRawReadCluster(DeviceExt, FirstCluster,
                                BaseAddress, *CurrentCluster, 1);
    memcpy(Destination, BaseAddress + InternalOffset, InternalLength);
    ExFreePool(BaseAddress);
  }
  if (!NT_SUCCESS(Status))
  {
     return(Status);
  }
  Status = NextCluster(DeviceExt, FirstCluster, CurrentCluster, FALSE);
  return(Status);
}

NTSTATUS
VfatReadFile (PDEVICE_EXTENSION DeviceExt, PFILE_OBJECT FileObject,
	      PVOID Buffer, ULONG Length, ULONG ReadOffset,
	      PULONG LengthRead, ULONG NoCache)
/*
 * FUNCTION: Reads data from a file
 */
{
  ULONG CurrentCluster;
  ULONG FirstCluster;
  ULONG StartCluster;
  ULONG ClusterCount;
  PVFATFCB Fcb;
  PVFATCCB Ccb;
  NTSTATUS Status;
  ULONG TempLength;
  LARGE_INTEGER FileOffset;
  IO_STATUS_BLOCK IoStatus;

  /* PRECONDITION */
  assert (DeviceExt != NULL);
  assert (DeviceExt->BytesPerCluster != 0);
  assert (FileObject != NULL);
  assert (FileObject->FsContext2 != NULL);

  DPRINT("VfatReadFile(DeviceExt %x, FileObject %x, Buffer %x, "
	     "Length %d, ReadOffset 0x%x)\n", DeviceExt, FileObject, Buffer,
	     Length, ReadOffset);

  *LengthRead = 0;

  Ccb = (PVFATCCB)FileObject->FsContext2;
  Fcb = Ccb->pFcb;

  // Is this a read of the FAT ?
  if (Fcb->Flags & FCB_IS_FAT)
  {
    if (!NoCache)
    {
      DbgPrint ("Cached FAT read outside from VFATFS.SYS\n");
      KeBugCheck (0);
    }
    if (ReadOffset >= Fcb->RFCB.FileSize.QuadPart || ReadOffset % BLOCKSIZE != 0 || Length % BLOCKSIZE != 0)
    {
      DbgPrint ("Start or end of FAT read is not on a sector boundary\n");
      KeBugCheck (0);
    }
    if (ReadOffset + Length > Fcb->RFCB.FileSize.QuadPart)
    {
      Length = Fcb->RFCB.FileSize.QuadPart - ReadOffset;
    }

    Status = VfatReadSectors(DeviceExt->StorageDevice,
               DeviceExt->FATStart + ReadOffset / BLOCKSIZE, Length / BLOCKSIZE, Buffer);
    if (NT_SUCCESS(Status))
    {
      *LengthRead = Length;
    }
    else
    {
      DPRINT1("FAT reading failed, Status %x\n", Status);
    }
    return Status;
  }

  /*
   * Find the first cluster
   */
  FirstCluster = CurrentCluster = vfatDirEntryGetFirstCluster (DeviceExt, &Fcb->entry);

  /*
   * Truncate the read if necessary
   */
  if (!(Fcb->entry.Attrib & FILE_ATTRIBUTE_DIRECTORY))
  {
    if (ReadOffset >= Fcb->entry.FileSize)
    {
      return (STATUS_END_OF_FILE);
    }
    if ((ReadOffset + Length) > Fcb->entry.FileSize)
    {
       Length = Fcb->entry.FileSize - ReadOffset;
    }
  }

  if (FirstCluster == 1)
  {
    // root directory of FAT12 od FAT16
    if (ReadOffset + Length > DeviceExt->rootDirectorySectors * BLOCKSIZE)
    {
      Length = DeviceExt->rootDirectorySectors * BLOCKSIZE - ReadOffset;
    }
  }

  // using the Cc-interface if possible
  if (!NoCache)
  {
    FileOffset.QuadPart = ReadOffset;
    CcCopyRead(FileObject, &FileOffset, Length, TRUE, Buffer, &IoStatus);
    *LengthRead = IoStatus.Information;
    return IoStatus.Status;
  }

  /*
   * Find the cluster to start the read from
   */
  if (Ccb->LastCluster > 0 && ReadOffset > Ccb->LastOffset)
  {
    CurrentCluster = Ccb->LastCluster;
  }
  Status = OffsetToCluster(DeviceExt,
			   FirstCluster,
			   ROUND_DOWN(ReadOffset, DeviceExt->BytesPerCluster),
			   &CurrentCluster,
			   FALSE);
  if (!NT_SUCCESS(Status))
  {
    return(Status);
  }
  /*
   * If the read doesn't begin on a chunk boundary then we need special
   * handling
   */
  if ((ReadOffset % DeviceExt->BytesPerCluster) != 0 )
  {
    TempLength = min (Length, DeviceExt->BytesPerCluster - (ReadOffset % DeviceExt->BytesPerCluster));
    Ccb->LastCluster = CurrentCluster;
    Ccb->LastOffset = ROUND_DOWN(ReadOffset, DeviceExt->BytesPerCluster);
    Status = VfatReadCluster(DeviceExt, FirstCluster, &CurrentCluster, Buffer,
               ReadOffset % DeviceExt->BytesPerCluster, TempLength);
    if (NT_SUCCESS(Status))
    {
      (*LengthRead) = (*LengthRead) + TempLength;
      Length = Length - TempLength;
      Buffer = Buffer + TempLength;
      ReadOffset = ReadOffset + TempLength;
    }
  }

  while (Length >= DeviceExt->BytesPerCluster && CurrentCluster != 0xffffffff && NT_SUCCESS(Status))
  {
    StartCluster = CurrentCluster;
    ClusterCount = 0;
    // search for continous clusters
    do
    {
      ClusterCount++;
      Status = NextCluster(DeviceExt, FirstCluster, &CurrentCluster, FALSE);
    }
    while (StartCluster + ClusterCount == CurrentCluster && NT_SUCCESS(Status) &&
       Length - ClusterCount * DeviceExt->BytesPerCluster >= DeviceExt->BytesPerCluster);
    DPRINT("Count %d, Start %x Next %x\n", ClusterCount, StartCluster, CurrentCluster);
    Ccb->LastCluster = StartCluster + (ClusterCount - 1);
    Ccb->LastOffset = ReadOffset + (ClusterCount - 1) * DeviceExt->BytesPerCluster;

    Status = VfatRawReadCluster(DeviceExt, FirstCluster, Buffer, StartCluster, ClusterCount);
    if (NT_SUCCESS(Status))
    {
      ClusterCount *=  DeviceExt->BytesPerCluster;
      (*LengthRead) = (*LengthRead) + ClusterCount;
      Buffer += ClusterCount;
      Length -= ClusterCount;
      ReadOffset += ClusterCount;
    }
  }
  /*
   * If the read doesn't end on a chunk boundary then we need special
   * handling
   */
  if (Length > 0 && CurrentCluster != 0xffffffff && NT_SUCCESS(Status))
  {
    Ccb->LastCluster = CurrentCluster;
    Ccb->LastOffset = ReadOffset + DeviceExt->BytesPerCluster;

    Status = VfatReadCluster(DeviceExt, FirstCluster, &CurrentCluster,
                             Buffer, 0, Length);
    if (NT_SUCCESS(Status))
    {
      (*LengthRead) = (*LengthRead) + Length;
    }
  }
  return Status;
}

NTSTATUS
VfatWriteCluster(PDEVICE_EXTENSION DeviceExt,
		    ULONG StartOffset,
		    ULONG FirstCluster,
		    PULONG CurrentCluster,
		    PVOID Source,
		    ULONG InternalOffset,
		    ULONG InternalLength)
{
  PVOID BaseAddress;
  NTSTATUS Status;

  if (InternalLength != DeviceExt->BytesPerCluster)
  {
     BaseAddress = ExAllocatePool(NonPagedPool, DeviceExt->BytesPerCluster);
     if (BaseAddress == NULL)
     {
       return(STATUS_NO_MEMORY);
     }
  }
  else
    BaseAddress = Source;
  if (InternalLength != DeviceExt->BytesPerCluster)
  {
    /*
     * If the data in the cache isn't valid or we are bypassing the
     * cache and not writing a cluster aligned, cluster sized region
     * then read data in to base address
     */
     Status = VfatRawReadCluster(DeviceExt, FirstCluster, BaseAddress,
				*CurrentCluster, 1);
     if (!NT_SUCCESS(Status))
     {
        if (InternalLength != DeviceExt->BytesPerCluster)
        {
          ExFreePool(BaseAddress);
        }
        return(Status);
     }
     memcpy(BaseAddress + InternalOffset, Source, InternalLength);
  }
  /*
   * Write the data back to disk
   */
  DPRINT("Writing 0x%x\n", *CurrentCluster);
  Status = VfatRawWriteCluster(DeviceExt, FirstCluster, BaseAddress,
			       *CurrentCluster, 1);
  if (InternalLength != DeviceExt->BytesPerCluster)
  {
    ExFreePool(BaseAddress);
  }
  if (!NT_SUCCESS(Status))
  {
    return Status;
  }
  Status = NextCluster(DeviceExt, FirstCluster, CurrentCluster, FALSE);
  return(Status);
}

NTSTATUS
VfatWriteFile (PDEVICE_EXTENSION DeviceExt, PFILE_OBJECT FileObject,
	       PVOID Buffer, ULONG Length, ULONG WriteOffset,
	       BOOLEAN NoCache, BOOLEAN PageIo)
/*
 * FUNCTION: Writes data to file
 */
{
  ULONG CurrentCluster;
  ULONG FirstCluster;
  ULONG StartCluster;
  ULONG Count;
  PVFATFCB Fcb;
  PVFATCCB pCcb;
  ULONG TempLength;
  LARGE_INTEGER SystemTime, LocalTime;
  NTSTATUS Status;
  BOOLEAN Extend;
  LARGE_INTEGER FileOffset;

  DPRINT ("VfatWriteFile(FileObject %x, Buffer %x, Length %x, "
	      "WriteOffset %x\n", FileObject, Buffer, Length, WriteOffset);

  assert (FileObject);
  pCcb = (PVFATCCB) (FileObject->FsContext2);
  assert (pCcb);
  Fcb = pCcb->pFcb;
  assert (Fcb);

//  DPRINT1("%S\n", Fcb->PathName);

  if (Length == 0)
  {
    return STATUS_SUCCESS;
  }

  // Is this a write to the FAT ?
  if (Fcb->Flags & FCB_IS_FAT)
  {
    if (!NoCache)
    {
      DbgPrint ("Cached FAT write outside from VFATFS.SYS\n");
      KeBugCheck (0);
    }
    if (WriteOffset >= Fcb->RFCB.FileSize.QuadPart || WriteOffset % BLOCKSIZE != 0 || Length % BLOCKSIZE != 0)
    {
      DbgPrint ("Start or end of FAT write is not on a sector boundary\n");
      KeBugCheck (0);
    }
    if (WriteOffset + Length > (ULONG)Fcb->RFCB.FileSize.QuadPart)
    {
      Length = (ULONG)Fcb->RFCB.FileSize.QuadPart - WriteOffset;
    }

    for (Count = 0; Count < DeviceExt->Boot->FATCount; Count++)
    {
      Status = VfatWriteSectors(DeviceExt->StorageDevice,
                 DeviceExt->FATStart + (Count * (ULONG)Fcb->RFCB.FileSize.QuadPart + WriteOffset) / BLOCKSIZE,
                 Length / BLOCKSIZE, Buffer);
      if (!NT_SUCCESS(Status))
      {
        DPRINT1("FAT writing failed, Status %x\n", Status);
      }
    }
    return Status;
  }

  /* Locate the first cluster of the file */
  FirstCluster = CurrentCluster = vfatDirEntryGetFirstCluster (DeviceExt, &Fcb->entry);

  if (PageIo)
  {
    if (FirstCluster == 0)
    {
      return STATUS_UNSUCCESSFUL;
    }
  }
  else
  {
    if (FirstCluster == 1)
    {
      // root directory of FAT12 od FAT16
      if (WriteOffset + Length > DeviceExt->rootDirectorySectors * BLOCKSIZE)
      {
        DPRINT("Writing over the end of the root directory on FAT12/16\n");
        return STATUS_END_OF_FILE;
      }
    }

    Status = vfatExtendSpace(DeviceExt, FileObject, WriteOffset + Length);
    if (!NT_SUCCESS (Status))
    {
      return Status;
    }
  }

  if (NoCache || PageIo)
  {

    FirstCluster = CurrentCluster = vfatDirEntryGetFirstCluster (DeviceExt, &Fcb->entry);
    if (pCcb->LastCluster > 0 && WriteOffset > pCcb->LastOffset)
    {
      CurrentCluster = pCcb->LastCluster;
    }
    Status = OffsetToCluster(DeviceExt,
			   FirstCluster,
			   ROUND_DOWN(WriteOffset, DeviceExt->BytesPerCluster),
			   &CurrentCluster,
			   FALSE);
    if (!NT_SUCCESS(Status) || CurrentCluster == 0xffffffff)
    {
      DPRINT1("????\n");
      return(Status);
    }
    pCcb->LastCluster = CurrentCluster;
    pCcb->LastOffset = ROUND_DOWN(WriteOffset, DeviceExt->BytesPerCluster);

    /*
     * If the offset in the cluster doesn't fall on the cluster boundary
     * then we have to write only from the specified offset
     */
    Status = STATUS_SUCCESS;
    if ((WriteOffset % DeviceExt->BytesPerCluster) != 0)
    {
      TempLength = min (Length, DeviceExt->BytesPerCluster - (WriteOffset % DeviceExt->BytesPerCluster));
      Status = VfatWriteCluster(DeviceExt,
			      ROUND_DOWN(WriteOffset, DeviceExt->BytesPerCluster),
			      FirstCluster,
			      &CurrentCluster,
			      Buffer,
			      WriteOffset % DeviceExt->BytesPerCluster,
			      TempLength);
      if (NT_SUCCESS(Status))
      {
        Buffer = Buffer + TempLength;
        Length = Length - TempLength;
        WriteOffset = WriteOffset + TempLength;
      }
    }

    while (Length >= DeviceExt->BytesPerCluster && CurrentCluster != 0xffffffff && NT_SUCCESS(Status))
    {
      StartCluster = CurrentCluster;
      Count = 0;
      // search for continous clusters
      do
      {
        Count++;
        Status = NextCluster(DeviceExt, FirstCluster, &CurrentCluster, FALSE);
      }
      while (StartCluster + Count == CurrentCluster && NT_SUCCESS(Status) &&
        Length - Count * DeviceExt->BytesPerCluster >= DeviceExt->BytesPerCluster);

      pCcb->LastCluster = StartCluster + (Count - 1);
      pCcb->LastOffset = WriteOffset + (Count - 1) * DeviceExt->BytesPerCluster;

      Status = VfatRawWriteCluster(DeviceExt, FirstCluster, Buffer, StartCluster, Count);
      if (NT_SUCCESS(Status))
      {
        Count *=  DeviceExt->BytesPerCluster;
        Buffer += Count;
        Length -= Count;
        WriteOffset += Count;
      }
    }

    /* Write the remainder */
    if (Length > 0 && CurrentCluster != 0xffffffff && NT_SUCCESS(Status))
    {
      Status = VfatWriteCluster(DeviceExt,
			     WriteOffset,
			     FirstCluster,
			     &CurrentCluster,
			     Buffer,
			     0,
		         Length);
      if (NT_SUCCESS(Status))
      {
        Length = 0;
      }
    }
    if (NT_SUCCESS(Status) && Length)
    {
      if (WriteOffset < Fcb->RFCB.AllocationSize.QuadPart)
      {
        DPRINT1("%d %d\n", WriteOffset, (ULONG)Fcb->RFCB.AllocationSize.QuadPart);
        Status = STATUS_DISK_FULL; // ???????????
      }
    }
  }
  else
  {
    // using the Cc-interface if possible
    FileOffset.QuadPart = WriteOffset;
    if(CcCopyWrite(FileObject, &FileOffset, Length, TRUE, Buffer))
    {
      Status = STATUS_SUCCESS;
    }
    else
    {
      Status = STATUS_UNSUCCESSFUL;
    }
  }


  if (!PageIo)
  {
    if(!(Fcb->entry.Attrib & FILE_ATTRIBUTE_DIRECTORY))
    {
      /* set dates and times */
      KeQuerySystemTime (&SystemTime);
      ExSystemTimeToLocalTime (&SystemTime, &LocalTime);
      FsdFileTimeToDosDateTime ((TIME*)&LocalTime,
		        &Fcb->entry.UpdateDate,
		    	&Fcb->entry.UpdateTime);
      Fcb->entry.AccessDate = Fcb->entry.UpdateDate;
      // update dates/times and length
      updEntry (DeviceExt, FileObject);
    }
  }

  return Status;
}

NTSTATUS STDCALL
VfatWrite (PDEVICE_OBJECT DeviceObject, PIRP Irp)
/*
 * FUNCTION: Write to a file
 */
{
  ULONG Length;
  PVOID Buffer;
  ULONG Offset;
  PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation (Irp);
  PFILE_OBJECT FileObject = Stack->FileObject;
  PDEVICE_EXTENSION DeviceExt = DeviceObject->DeviceExtension;
  NTSTATUS Status;
  ULONG NoCache;

  DPRINT ("VfatWrite(DeviceObject %x Irp %x)\n", DeviceObject, Irp);

  Length = Stack->Parameters.Write.Length;
  Buffer = MmGetSystemAddressForMdl (Irp->MdlAddress);
  Offset = Stack->Parameters.Write.ByteOffset.u.LowPart;

  if (Irp->Flags & IRP_PAGING_IO ||
      FileObject->Flags & FO_NO_INTERMEDIATE_BUFFERING)
  {
    NoCache = TRUE;
  }
  else
  {
    NoCache = FALSE;
  }

  Status = VfatWriteFile (DeviceExt, FileObject, Buffer, Length, Offset,
			  NoCache, Irp->Flags & IRP_PAGING_IO ? TRUE : FALSE);

  if (!(Irp->Flags & IRP_PAGING_IO) && NT_SUCCESS(Status))
  {
    FileObject->CurrentByteOffset.QuadPart = Offset + Length;
  }

  Irp->IoStatus.Status = Status;
  Irp->IoStatus.Information = Length;
  IoCompleteRequest (Irp, IO_NO_INCREMENT);

  return (Status);
}

NTSTATUS STDCALL
VfatRead (PDEVICE_OBJECT DeviceObject, PIRP Irp)
/*
 * FUNCTION: Read from a file
 */
{
  ULONG Length;
  PVOID Buffer;
  ULONG Offset;
  PIO_STACK_LOCATION Stack;
  PFILE_OBJECT FileObject;
  PDEVICE_EXTENSION DeviceExt;
  NTSTATUS Status;
  ULONG LengthRead;
  PVFATFCB Fcb;
  ULONG NoCache;

  DPRINT ("VfatRead(DeviceObject %x, Irp %x)\n", DeviceObject, Irp);

  /* Precondition / Initialization */
  assert (Irp != NULL);
  Stack = IoGetCurrentIrpStackLocation (Irp);
  assert (Stack != NULL);
  FileObject = Stack->FileObject;
  assert (FileObject != NULL);
  DeviceExt = DeviceObject->DeviceExtension;
  assert (DeviceExt != NULL);

  Length = Stack->Parameters.Read.Length;
  Buffer = MmGetSystemAddressForMdl (Irp->MdlAddress);
  Offset = Stack->Parameters.Read.ByteOffset.u.LowPart;

  if (Irp->Flags & IRP_PAGING_IO ||
      FileObject->Flags & FO_NO_INTERMEDIATE_BUFFERING)
  {
    NoCache = TRUE;
  }
  else
  {
    NoCache = FALSE;
  }

  Fcb = ((PVFATCCB) (FileObject->FsContext2))->pFcb;
  /* fail if file is a directory and no paged read */
  if (Fcb->entry.Attrib & FILE_ATTRIBUTE_DIRECTORY && !(Irp->Flags & IRP_PAGING_IO))
  {
    Status = STATUS_FILE_IS_A_DIRECTORY;
  }
  else
  {
    Status = VfatReadFile (DeviceExt, FileObject, Buffer, Length,
               Offset, &LengthRead, NoCache);
  }

  if (!(Irp->Flags & IRP_PAGING_IO))
  {
    // update the file pointer
    FileObject->CurrentByteOffset.QuadPart = Offset + LengthRead;
  }

  Irp->IoStatus.Status = Status;
  Irp->IoStatus.Information = LengthRead;
  IoCompleteRequest (Irp, IO_NO_INCREMENT);

  return (Status);
}

NTSTATUS vfatExtendSpace (PDEVICE_EXTENSION pDeviceExt, PFILE_OBJECT pFileObject, ULONG NewSize)
{
  ULONG FirstCluster;
  ULONG CurrentCluster;
  ULONG NewCluster;
  NTSTATUS Status;
  PVFATFCB pFcb;


  pFcb = ((PVFATCCB) (pFileObject->FsContext2))->pFcb;

  DPRINT ("New Size %d,  AllocationSize %d,  BytesPerCluster %d\n",  NewSize,
    (ULONG)pFcb->RFCB.AllocationSize.QuadPart, pDeviceExt->BytesPerCluster);

  FirstCluster = CurrentCluster = vfatDirEntryGetFirstCluster (pDeviceExt, &pFcb->entry);

  if (NewSize > pFcb->RFCB.AllocationSize.QuadPart || FirstCluster==0)
  {
    // size on disk must be extended
    if (FirstCluster == 0)
    {
      // file of size zero
      Status = NextCluster (pDeviceExt, FirstCluster, &CurrentCluster, TRUE);
      if (!NT_SUCCESS(Status))
      {
        DPRINT1("NextCluster failed, Status %x\n", Status);
        return Status;
      }
      NewCluster = FirstCluster = CurrentCluster;
    }
    else
    {
      Status = OffsetToCluster(pDeviceExt, FirstCluster,
                 pFcb->RFCB.AllocationSize.QuadPart - pDeviceExt->BytesPerCluster,
                 &CurrentCluster, FALSE);
      if (!NT_SUCCESS(Status))
      {
        DPRINT1("OffsetToCluster failed, Status %x\n", Status);
        return Status;
      }
      if (CurrentCluster == 0xffffffff)
      {
        DPRINT1("Not enough disk space.\n");
        return STATUS_DISK_FULL;
      }
      // CurrentCluster zeigt jetzt auf den letzten Cluster in der Kette
      NewCluster = CurrentCluster;
      Status = NextCluster(pDeviceExt, FirstCluster, &NewCluster, FALSE);
      if (NewCluster != 0xffffffff)
      {
        DPRINT1("Difference between size from direntry and the FAT.\n");
      }
    }

    Status = OffsetToCluster(pDeviceExt, FirstCluster,
               ROUND_DOWN(NewSize-1, pDeviceExt->BytesPerCluster),
               &NewCluster, TRUE);
    if (!NT_SUCCESS(Status) || NewCluster == 0xffffffff)
    {
      DPRINT1("Not enough free space on disk\n");
      if (pFcb->RFCB.AllocationSize.QuadPart > 0)
      {
        NewCluster = CurrentCluster;
        // FIXME: check status
        NextCluster(pDeviceExt, FirstCluster, &NewCluster, FALSE);
        WriteCluster(pDeviceExt, CurrentCluster, 0xffffffff);
      }
      // free the allocated space
      while (NewCluster != 0xffffffff)
      {
        CurrentCluster = NewCluster;
        // FIXME: check status
        NextCluster (pDeviceExt, FirstCluster, &NewCluster, FALSE);
        WriteCluster (pDeviceExt, CurrentCluster, 0);
      }
      return STATUS_DISK_FULL;
    }
    if (pFcb->RFCB.AllocationSize.QuadPart == 0)
    {
      pFcb->entry.FirstCluster = FirstCluster;
      if(pDeviceExt->FatType == FAT32)
        pFcb->entry.FirstClusterHigh = FirstCluster >> 16;
    }
    pFcb->RFCB.AllocationSize.QuadPart = ROUND_UP(NewSize, pDeviceExt->BytesPerCluster);
    if (pFcb->entry.Attrib & FILE_ATTRIBUTE_DIRECTORY)
    {
      pFcb->RFCB.FileSize.QuadPart = pFcb->RFCB.AllocationSize.QuadPart;
      pFcb->RFCB.ValidDataLength.QuadPart = pFcb->RFCB.AllocationSize.QuadPart;
    }
    else
    {
      pFcb->entry.FileSize = NewSize;
      pFcb->RFCB.FileSize.QuadPart = NewSize;
      pFcb->RFCB.ValidDataLength.QuadPart = NewSize;
    }
    CcSetFileSizes(pFileObject, (PCC_FILE_SIZES)&pFcb->RFCB.AllocationSize);
  }
  else
  {
    if (NewSize > pFcb->RFCB.FileSize.QuadPart)
    {
      // size on disk must not be extended
      if (!(pFcb->entry.Attrib & FILE_ATTRIBUTE_DIRECTORY))
      {
        pFcb->entry.FileSize = NewSize;
        pFcb->RFCB.FileSize.QuadPart = NewSize;
        CcSetFileSizes(pFileObject, (PCC_FILE_SIZES)&pFcb->RFCB.AllocationSize);
      }
    }
    else
    {
      // nothing to do
    }
  }
  return STATUS_SUCCESS;
}
