/* $Id: copy.c,v 1.17 2003/06/07 11:34:36 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/cc/copy.c
 * PURPOSE:         Implements cache managers copy interface
 * PROGRAMMER:      Hartmut Birr
 * UPDATE HISTORY:
 *                  Created 05.10.2001
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <ddk/ntifs.h>
#include <internal/mm.h>
#include <internal/cc.h>
#include <internal/pool.h>
#include <internal/io.h>
#include <ntos/minmax.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

#define ROUND_DOWN(N, S) ((N) - ((N) % (S)))

static PHYSICAL_ADDRESS CcZeroPage = (PHYSICAL_ADDRESS)0LL;

/* FUNCTIONS *****************************************************************/

VOID 
CcInitCacheZeroPage(VOID)
{
   NTSTATUS Status;

   Status = MmRequestPageMemoryConsumer(MC_NPPOOL, FALSE, &CcZeroPage);
   if (!NT_SUCCESS(Status))
   {
       DbgPrint("Can't allocate CcZeroPage.\n");
       KeBugCheck(0);
   }
   Status = MiZeroPage(CcZeroPage);
   if (!NT_SUCCESS(Status))
   {
       DbgPrint("Can't zero out CcZeroPage.\n");
       KeBugCheck(0);
   }
}

NTSTATUS
ReadCacheSegmentChain(PBCB Bcb, ULONG ReadOffset, ULONG Length,
		      PVOID Buffer)
{
  PCACHE_SEGMENT head;
  PCACHE_SEGMENT current;
  PCACHE_SEGMENT previous;
  IO_STATUS_BLOCK Iosb;
  LARGE_INTEGER SegOffset;
  NTSTATUS Status;
  ULONG TempLength;
  KEVENT Event;

  Status = CcRosGetCacheSegmentChain(Bcb, ReadOffset, Length, &head);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }
  current = head;
  while (current != NULL)
    {
      /*
       * If the current segment is valid then copy it into the
       * user buffer.
       */
      if (current->Valid)
	{
	  TempLength = min(Bcb->CacheSegmentSize, Length);
	  memcpy(Buffer, current->BaseAddress, TempLength);
	  Buffer += TempLength;
	  Length = Length - TempLength; 
	  previous = current;
	  current = current->NextInChain;
	  CcRosReleaseCacheSegment(Bcb, previous, TRUE, FALSE, FALSE);
	}
      /*
       * Otherwise read in as much as we can.
       */
      else
	{
	  PCACHE_SEGMENT current2;
	  ULONG current_size;
	  PMDL Mdl;
	  ULONG i;
	  ULONG offset;

	  /*
	   * Count the maximum number of bytes we could read starting
	   * from the current segment.
	   */
	  current2 = current;
	  current_size = 0;
	  while (current2 != NULL && !current2->Valid)
	    {
	      current2 = current2->NextInChain;
	      current_size += Bcb->CacheSegmentSize;
	    }
	  
	  /*
	   * Create an MDL which contains all their pages.
	   */
	  Mdl = MmCreateMdl(NULL, NULL, current_size);
	  Mdl->MdlFlags |= (MDL_PAGES_LOCKED | MDL_IO_PAGE_READ);
	  current2 = current;
	  offset = 0;
	  while (current2 != NULL && !current2->Valid)
	    {
	      for (i = 0; i < (Bcb->CacheSegmentSize / PAGE_SIZE); i++)
		{
		  PVOID address;
		  PHYSICAL_ADDRESS page;
		  address = current2->BaseAddress + (i * PAGE_SIZE);
		  page = MmGetPhysicalAddressForProcess(NULL, address);
		  ((PULONG)(Mdl + 1))[offset] = page.u.LowPart;
		  offset++;
		}
	      current2 = current2->NextInChain;
	    }

	  /*
	   * Read in the information.
	   */
	  SegOffset.QuadPart = current->FileOffset;
	  KeInitializeEvent(&Event, NotificationEvent, FALSE);
	  Status = IoPageRead(Bcb->FileObject,
			      Mdl,
			      &SegOffset,
			      &Event,
			      &Iosb);
	  if (Status == STATUS_PENDING)
	  {
	     KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
	     Status = Iosb.Status;
	  }
	  if (!NT_SUCCESS(Status) && Status != STATUS_END_OF_FILE)
	    {
	      while (current != NULL)
		{
		  previous = current;
		  current = current->NextInChain;
		  CcRosReleaseCacheSegment(Bcb, previous, FALSE, FALSE, FALSE);
		}
	      return(Status);
	    }
	  while (current != NULL && !current->Valid)
	    {
	      previous = current;
	      current = current->NextInChain;
	      TempLength = min(Bcb->CacheSegmentSize, Length);
	      memcpy(Buffer, previous->BaseAddress, TempLength);
	      Buffer += TempLength;
	      Length = Length - TempLength; 
	      CcRosReleaseCacheSegment(Bcb, previous, TRUE, FALSE, FALSE);
	    }
	}
    }
  return(STATUS_SUCCESS);
}

NTSTATUS 
ReadCacheSegment(PCACHE_SEGMENT CacheSeg)
{
  ULONG Size;
  PMDL Mdl;
  NTSTATUS Status;
  LARGE_INTEGER SegOffset;
  IO_STATUS_BLOCK IoStatus;
  KEVENT Event;

  SegOffset.QuadPart = CacheSeg->FileOffset;
  Size = CacheSeg->Bcb->AllocationSize.QuadPart - CacheSeg->FileOffset;
  if (Size > CacheSeg->Bcb->CacheSegmentSize)
    {
      Size = CacheSeg->Bcb->CacheSegmentSize;
    }
  Mdl = MmCreateMdl(NULL, CacheSeg->BaseAddress, Size);
  MmBuildMdlForNonPagedPool(Mdl);
  KeInitializeEvent(&Event, NotificationEvent, FALSE);
  Status = IoPageRead(CacheSeg->Bcb->FileObject, Mdl, &SegOffset, & Event, &IoStatus); 
  if (Status == STATUS_PENDING)
  {
     KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
     Status = IoStatus.Status;
  }

  if (!NT_SUCCESS(Status) && Status != STATUS_END_OF_FILE)
    {
      DPRINT1("IoPageRead failed, Status %x\n", Status);
      return Status;
    }
  if (CacheSeg->Bcb->CacheSegmentSize > Size)
    {
      memset (CacheSeg->BaseAddress + Size, 0, 
	      CacheSeg->Bcb->CacheSegmentSize - Size);
    }
  return STATUS_SUCCESS;
}

NTSTATUS 
WriteCacheSegment(PCACHE_SEGMENT CacheSeg)
{
  ULONG Size;
  PMDL Mdl;
  NTSTATUS Status;
  IO_STATUS_BLOCK IoStatus;
  LARGE_INTEGER SegOffset;
  KEVENT Event;

  CacheSeg->Dirty = FALSE;
  SegOffset.QuadPart = CacheSeg->FileOffset;
  Size = CacheSeg->Bcb->AllocationSize.QuadPart - CacheSeg->FileOffset;
  if (Size > CacheSeg->Bcb->CacheSegmentSize)
    {
      Size = CacheSeg->Bcb->CacheSegmentSize;
    }
  Mdl = MmCreateMdl(NULL, CacheSeg->BaseAddress, Size);
  MmBuildMdlForNonPagedPool(Mdl);
  KeInitializeEvent(&Event, NotificationEvent, FALSE);
  Status = IoPageWrite(CacheSeg->Bcb->FileObject, Mdl, &SegOffset, &Event, &IoStatus);
  if (Status == STATUS_PENDING)
  {
     KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
     Status = IoStatus.Status;
  }
  if (!NT_SUCCESS(Status) && (Status != STATUS_END_OF_FILE))
    {
      DPRINT1("IoPageWrite failed, Status %x\n", Status);
      CacheSeg->Dirty = TRUE;
      return(Status);
    }
  return(STATUS_SUCCESS);
}

BOOLEAN STDCALL
CcCopyRead (IN PFILE_OBJECT FileObject,
	    IN PLARGE_INTEGER FileOffset,
	    IN ULONG Length,
	    IN BOOLEAN Wait,
	    OUT PVOID Buffer,
	    OUT PIO_STATUS_BLOCK IoStatus)
{
  ULONG ReadOffset;
  ULONG TempLength;
  NTSTATUS Status = STATUS_SUCCESS;
  PVOID BaseAddress;
  PCACHE_SEGMENT CacheSeg;
  BOOLEAN Valid;
  ULONG ReadLength = 0;
  PBCB Bcb;
  KIRQL oldirql;
  PLIST_ENTRY current_entry;
  PCACHE_SEGMENT current;
  
  DPRINT("CcCopyRead(FileObject %x, FileOffset %x, "
	 "Length %d, Wait %d, Buffer %x, IoStatus %x)\n",
	 FileObject, (ULONG)FileOffset->QuadPart, Length, Wait,
	 Buffer, IoStatus);

  Bcb = FileObject->SectionObjectPointer->SharedCacheMap;
  ReadOffset = FileOffset->QuadPart;
  
  DPRINT("AllocationSize %d, FileSize %d\n",
         (ULONG)Bcb->AllocationSize.QuadPart,
         (ULONG)Bcb->FileSize.QuadPart);

  /*
   * Check for the nowait case that all the cache segments that would
   * cover this read are in memory.
   */
  if (!Wait)
    {
      KeAcquireSpinLock(&Bcb->BcbLock, &oldirql);
      current_entry = Bcb->BcbSegmentListHead.Flink;
      while (current_entry != &Bcb->BcbSegmentListHead)
	{
	  current = CONTAINING_RECORD(current_entry, CACHE_SEGMENT, 
				      BcbSegmentListEntry);
	  if (!current->Valid && current->FileOffset < ReadOffset + Length
	      && current->FileOffset + Bcb->CacheSegmentSize > ReadOffset)
	    {
	      KeReleaseSpinLock(&Bcb->BcbLock, oldirql);
	      IoStatus->Status = STATUS_UNSUCCESSFUL;
	      IoStatus->Information = 0;
	      return FALSE;
	    }
	  current_entry = current_entry->Flink;
	}
      KeReleaseSpinLock(&Bcb->BcbLock, oldirql);
    }

  TempLength = ReadOffset % Bcb->CacheSegmentSize;
  if (TempLength != 0)
    {
      TempLength = min (Length, Bcb->CacheSegmentSize - TempLength);
      Status = CcRosRequestCacheSegment(Bcb,
					ROUND_DOWN(ReadOffset, 
						   Bcb->CacheSegmentSize),
					&BaseAddress, &Valid, &CacheSeg);
      if (!NT_SUCCESS(Status))
	{
	  IoStatus->Information = 0;
	  IoStatus->Status = Status;
	  DPRINT("CcRosRequestCacheSegment faild, Status %x\n", Status);
	  return FALSE;
	}
      if (!Valid)
	{
	  Status = ReadCacheSegment(CacheSeg);
	  if (!NT_SUCCESS(Status))
	    {
	      IoStatus->Information = 0;
	      IoStatus->Status = Status;
              CcRosReleaseCacheSegment(Bcb, CacheSeg, FALSE, FALSE, FALSE);
	      return FALSE;
	    }
	}
      memcpy (Buffer, BaseAddress + ReadOffset % Bcb->CacheSegmentSize, 
	      TempLength);
      CcRosReleaseCacheSegment(Bcb, CacheSeg, TRUE, FALSE, FALSE);
      ReadLength += TempLength;
      Length -= TempLength;
      ReadOffset += TempLength;
      Buffer += TempLength;
    }  
  while (Length > 0)
    {
      TempLength = min(max(Bcb->CacheSegmentSize, 65536), Length);
      ReadCacheSegmentChain(Bcb, ReadOffset, TempLength, Buffer);
      ReadLength += TempLength;
      Length -= TempLength;
      ReadOffset += TempLength;
      Buffer += TempLength;
    }
  IoStatus->Status = STATUS_SUCCESS;
  IoStatus->Information = ReadLength;
  DPRINT("CcCopyRead O.K.\n");
  return TRUE;
}

BOOLEAN STDCALL
CcCopyWrite (IN PFILE_OBJECT FileObject,
	     IN PLARGE_INTEGER FileOffset,
	     IN ULONG Length,
	     IN BOOLEAN Wait,
	     IN PVOID Buffer)
{
   NTSTATUS Status;
   ULONG WriteOffset;
   KIRQL oldirql;
   PBCB Bcb;
   PLIST_ENTRY current_entry;
   PCACHE_SEGMENT CacheSeg;
   ULONG TempLength;
   PVOID BaseAddress;
   BOOLEAN Valid;

   DPRINT("CcCopyWrite(FileObject %x, FileOffset %x, "
	  "Length %d, Wait %d, Buffer %x)\n",
          FileObject, (ULONG)FileOffset->QuadPart, Length, Wait, Buffer);

   Bcb = FileObject->SectionObjectPointer->SharedCacheMap;
   WriteOffset = (ULONG)FileOffset->QuadPart;

   if (!Wait)
     {
       /* testing, if the requested datas are available */
       KeAcquireSpinLock(&Bcb->BcbLock, &oldirql);
       current_entry = Bcb->BcbSegmentListHead.Flink;
       while (current_entry != &Bcb->BcbSegmentListHead)
	 {
	   CacheSeg = CONTAINING_RECORD(current_entry, CACHE_SEGMENT, 
					BcbSegmentListEntry);
	   if (!CacheSeg->Valid)
	     {
	       if ((WriteOffset >= CacheSeg->FileOffset && 
		    WriteOffset < CacheSeg->FileOffset + Bcb->CacheSegmentSize)
		   || (WriteOffset + Length > CacheSeg->FileOffset && 
		       WriteOffset + Length <= CacheSeg->FileOffset + 
		       Bcb->CacheSegmentSize))
		 {
		   KeReleaseSpinLock(&Bcb->BcbLock, oldirql);
		   /* datas not available */
		   return(FALSE);
		 }
	     }
	   current_entry = current_entry->Flink;
	 }
       KeReleaseSpinLock(&Bcb->BcbLock, oldirql);
     }

   TempLength = WriteOffset % Bcb->CacheSegmentSize;
   if (TempLength != 0)
     {
       ULONG ROffset;
       ROffset = ROUND_DOWN(WriteOffset, Bcb->CacheSegmentSize);
       TempLength = min (Length, Bcb->CacheSegmentSize - TempLength);
       Status = CcRosRequestCacheSegment(Bcb, ROffset,
					 &BaseAddress, &Valid, &CacheSeg);
       if (!NT_SUCCESS(Status))
	 {
	   return(FALSE);
	 }
       if (!Valid)
	 {
	   if (!NT_SUCCESS(ReadCacheSegment(CacheSeg)))
	     {
	       return(FALSE);
	     }
	 }
       memcpy (BaseAddress + WriteOffset % Bcb->CacheSegmentSize, 
	       Buffer, TempLength);
       CcRosReleaseCacheSegment(Bcb, CacheSeg, TRUE, TRUE, FALSE);
       
       Length -= TempLength;
       WriteOffset += TempLength;
       Buffer += TempLength;
     }
   
   while (Length > 0)
     {
       TempLength = min (Bcb->CacheSegmentSize, Length);
       Status = CcRosRequestCacheSegment(Bcb, WriteOffset,
					 &BaseAddress, &Valid, &CacheSeg);
       if (!NT_SUCCESS(Status))
	 {
	   return(FALSE);
	 }
       if (!Valid && TempLength < Bcb->CacheSegmentSize)
	 {
	   if (!NT_SUCCESS(ReadCacheSegment(CacheSeg)))
	     {
               CcRosReleaseCacheSegment(Bcb, CacheSeg, FALSE, FALSE, FALSE);
	       return FALSE;
	     }
	 }
       memcpy (BaseAddress, Buffer, TempLength);
       CcRosReleaseCacheSegment(Bcb, CacheSeg, TRUE, TRUE, FALSE);
       Length -= TempLength;
       WriteOffset += TempLength;
       Buffer += TempLength;
     }
   return(TRUE);
}

BOOLEAN STDCALL
CcZeroData (IN PFILE_OBJECT     FileObject,
	    IN PLARGE_INTEGER   StartOffset,
	    IN PLARGE_INTEGER   EndOffset,
	    IN BOOLEAN          Wait)
{
  NTSTATUS Status;
  LARGE_INTEGER WriteOffset;
  ULONG Length;
  PMDL Mdl;
  ULONG i;
  IO_STATUS_BLOCK Iosb;
  KEVENT Event;
  
  DPRINT("CcZeroData(FileObject %x, StartOffset %I64x, EndOffset %I64x, "
	 "Wait %d\n", FileObject, StartOffset->QuadPart, EndOffset->QuadPart, 
	 Wait);
  
  Length = EndOffset->u.LowPart - StartOffset->u.LowPart;

  if (FileObject->SectionObjectPointer->SharedCacheMap == NULL)
    {
      /* File is not cached */
      WriteOffset.QuadPart = StartOffset->QuadPart;
      
      while (Length > 0)
	{
	  if (Length + WriteOffset.u.LowPart % PAGE_SIZE > 262144)
	    {
	      Mdl = MmCreateMdl(NULL, (PVOID)WriteOffset.u.LowPart, 
				262144 - WriteOffset.u.LowPart % PAGE_SIZE);
	      WriteOffset.QuadPart += 
		(262144 - WriteOffset.u.LowPart % PAGE_SIZE);
	      Length -= (262144 - WriteOffset.u.LowPart % PAGE_SIZE);
	    }
	  else
	    {
	      Mdl = 
		MmCreateMdl(NULL, (PVOID)WriteOffset.u.LowPart, 
			    Length - WriteOffset.u.LowPart % PAGE_SIZE);
	      WriteOffset.QuadPart += 
		(Length - WriteOffset.u.LowPart % PAGE_SIZE);
	      Length = 0;
	    }
	  if (Mdl == NULL)
	    {
	      return(FALSE);
	    }
	  Mdl->MdlFlags |= (MDL_PAGES_LOCKED | MDL_IO_PAGE_READ);
	  for (i = 0; i < ((Mdl->Size - sizeof(MDL)) / sizeof(ULONG)); i++)
	    {
	      ((PULONG)(Mdl + 1))[i] = CcZeroPage.u.LowPart;
	    }
          KeInitializeEvent(&Event, NotificationEvent, FALSE);
	  Status = IoPageWrite(FileObject, Mdl, StartOffset, &Event, &Iosb);
          if (Status == STATUS_PENDING)
	  {
             KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
             Status = Iosb.Status;
	  }
	  if (!NT_SUCCESS(Status))
	    {
	      return(FALSE);
	    }
	}
    }
  else
    {
      /* File is cached */
      KIRQL oldirql;
      PBCB Bcb;
      PLIST_ENTRY current_entry;
      PCACHE_SEGMENT CacheSeg, current, previous;
      ULONG TempLength;
      ULONG Start;
      ULONG count;
      ULONG size;
      PHYSICAL_ADDRESS page;

      Start = StartOffset->u.LowPart;
      Bcb = FileObject->SectionObjectPointer->SharedCacheMap;
      if (Wait)
	{
          /* testing, if the requested datas are available */
	  KeAcquireSpinLock(&Bcb->BcbLock, &oldirql);
	  current_entry = Bcb->BcbSegmentListHead.Flink;
	  while (current_entry != &Bcb->BcbSegmentListHead)
	    {
	      CacheSeg = CONTAINING_RECORD(current_entry, CACHE_SEGMENT, 
					   BcbSegmentListEntry);
	      if (!CacheSeg->Valid)
		{
		  if ((Start >= CacheSeg->FileOffset && 
		       Start < CacheSeg->FileOffset + Bcb->CacheSegmentSize)
		      || (Start + Length > CacheSeg->FileOffset && 
			  Start + Length <= 
			  CacheSeg->FileOffset + Bcb->CacheSegmentSize))
		    {
		      KeReleaseSpinLock(&Bcb->BcbLock, oldirql);
		      /* datas not available */
		      return(FALSE);
		    }
		}
	      current_entry = current_entry->Flink;
	    }
	  KeReleaseSpinLock(&Bcb->BcbLock, oldirql);
	}
      
      while (Length > 0)
	{
	  ULONG RStart = ROUND_DOWN(Start, Bcb->CacheSegmentSize);
	  WriteOffset.QuadPart = ROUND_DOWN(Start, Bcb->CacheSegmentSize);
	  if (Start % Bcb->CacheSegmentSize + Length > 262144)
	    {
	      Mdl = MmCreateMdl(NULL, NULL, 262144);
	      if (Mdl == NULL)
		{
		  return FALSE;
		}
	      Status = CcRosGetCacheSegmentChain (Bcb, RStart,
						  262144, &CacheSeg);
	      if (!NT_SUCCESS(Status))
		{
		  ExFreePool(Mdl);
		  return(FALSE);
		}
	    }
	  else
	    {
	      ULONG RLength;
	      RLength = Start % Bcb->CacheSegmentSize + Length;
	      RLength = ROUND_UP(RLength, Bcb->CacheSegmentSize);
	      Mdl = MmCreateMdl(NULL, (PVOID)RStart, RLength);
	      if (Mdl == NULL)
		{
		  return(FALSE);
		}
	      Status = CcRosGetCacheSegmentChain (Bcb, RStart, RLength,
						  &CacheSeg);
	      if (!NT_SUCCESS(Status))
		{
		  ExFreePool(Mdl);
		  return(FALSE);
		}
	    }
	  Mdl->MdlFlags |= (MDL_PAGES_LOCKED|MDL_IO_PAGE_READ);
	  current = CacheSeg;
	  count = 0;
	  while (current != NULL)
	    {
	      if ((Start % Bcb->CacheSegmentSize) != 0 || 
		  Start % Bcb->CacheSegmentSize + Length < 
		  Bcb->CacheSegmentSize)
		{
		  if (!current->Valid)
		    {
		      /* Segment lesen */
		      Status = ReadCacheSegment(current);
		      if (!NT_SUCCESS(Status))
			{
			  DPRINT1("ReadCacheSegment failed, status %x\n", 
				  Status);
			}
		    }
		  TempLength = min (Length, Bcb->CacheSegmentSize - 
				    Start % Bcb->CacheSegmentSize);
		  memset (current->BaseAddress + Start % Bcb->CacheSegmentSize,
			  0, TempLength);
		}
	      else
		{
		  TempLength = Bcb->CacheSegmentSize;
		  memset (current->BaseAddress, 0, Bcb->CacheSegmentSize);
		}
	      Start += TempLength;
	      Length -= TempLength;
	      
	      size = ((Mdl->Size - sizeof(MDL)) / sizeof(ULONG));
	      for (i = 0; i < (Bcb->CacheSegmentSize / PAGE_SIZE) && 
		     count < size; i++)
		{
		  PVOID Address;
		  Address = current->BaseAddress + (i * PAGE_SIZE);
		  page = 
		    MmGetPhysicalAddressForProcess(NULL, Address);
		  ((PULONG)(Mdl + 1))[count++] = page.u.LowPart;
		}
	      current = current->NextInChain;
	    }
	  
	  /* Write the Segment */
          KeInitializeEvent(&Event, NotificationEvent, FALSE);
	  Status = IoPageWrite(FileObject, Mdl, &WriteOffset, &Event, &Iosb);
          if (Status == STATUS_PENDING)
	  {
             KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
             Status = Iosb.Status;
	  }
	  if (!NT_SUCCESS(Status))
	    {
	      DPRINT1("IoPageWrite failed, status %x\n", Status);
	    }
	  current = CacheSeg;
	  while (current != NULL)
	    {
	      previous = current;
	      current = current->NextInChain;
	      CcRosReleaseCacheSegment(Bcb, previous, TRUE, FALSE, FALSE);
	    }
	}
    }
  return(TRUE);
}

