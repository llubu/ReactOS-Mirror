/* $Id: errlog.c,v 1.12 2003/11/18 20:08:30 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/errlog.c
 * PURPOSE:         Error logging
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/port.h>

//#define NDEBUG
#include <internal/debug.h>

/* TYPES *********************************************************************/

#ifndef __USE_W32API
typedef struct _IO_ERROR_LOG_PACKET
{
   UCHAR MajorFunctionCode;
   UCHAR RetryCount;
   USHORT DumpDataSize;
   USHORT NumberOfStrings;
   USHORT StringOffset;
   USHORT EventCategory;
   NTSTATUS ErrorCode;
   ULONG UniqueErrorValue;
   NTSTATUS FinalStatus;
   ULONG SequenceNumber;
   ULONG IoControlCode;
   LARGE_INTEGER DeviceOffset;
   ULONG DumpData[1];
} IO_ERROR_LOG_PACKET, *PIO_ERROR_LOG_PACKET;
#endif

typedef struct _ERROR_LOG_ENTRY
{
  LIST_ENTRY Entry;
  ULONG EntrySize;
} ERROR_LOG_ENTRY, *PERROR_LOG_ENTRY;


/* GLOBALS *******************************************************************/

static KSPIN_LOCK IopAllocationLock;
static ULONG IopTotalLogSize;

static KSPIN_LOCK IopLogListLock;
static LIST_ENTRY IopLogListHead;

static BOOLEAN IopLogWorkerRunning = FALSE;


/* FUNCTIONS *****************************************************************/

NTSTATUS
IopInitErrorLog (VOID)
{
  IopTotalLogSize = 0;
  KeInitializeSpinLock (&IopAllocationLock);

  KeInitializeSpinLock (&IopLogListLock);
  InitializeListHead (&IopLogListHead);

  return STATUS_SUCCESS;
}


static VOID STDCALL
IopLogWorker (PVOID Parameter)
{
  PERROR_LOG_ENTRY LogEntry;
  KIRQL Irql;

  DPRINT1 ("IopLogWorker() called\n");

  /* Release the work item */
  ExFreePool (Parameter);


  /* FIXME: Open the error log port */


  while (TRUE)
    {
      /* Remove last entry from the list */
      KeAcquireSpinLock (&IopLogListLock,
			 &Irql);

      if (!IsListEmpty(&IopLogListHead))
	{
	  LogEntry = CONTAINING_RECORD (IopLogListHead.Blink,
					ERROR_LOG_ENTRY,
					Entry);
	  RemoveEntryList (&LogEntry->Entry);
	}
      else
	{
	  LogEntry = NULL;
	}

      KeReleaseSpinLock (&IopLogListLock,
			 Irql);

      if (LogEntry == NULL)
	{
	  DPRINT1 ("No message in log list\n");
	  break;
	}



      /* FIXME: Send the error message to the log port */



      /* Release error log entry */
      KeAcquireSpinLock (&IopAllocationLock,
			 &Irql);

      IopTotalLogSize -= LogEntry->EntrySize;
      ExFreePool (LogEntry);

      KeReleaseSpinLock (&IopAllocationLock,
			 Irql);
    }

  DPRINT1 ("IopLogWorker() done\n");
}


/*
 * @implemented
 */
PVOID STDCALL
IoAllocateErrorLogEntry (IN PVOID IoObject,
			 IN UCHAR EntrySize)
{
  PERROR_LOG_ENTRY LogEntry;
  ULONG LogEntrySize;
  KIRQL Irql;

  DPRINT1 ("IoAllocateErrorLogEntry() called\n");

  if (IoObject == NULL)
    return NULL;

  KeAcquireSpinLock (&IopAllocationLock,
		     &Irql);

  if (IopTotalLogSize > PAGE_SIZE)
    {
      KeReleaseSpinLock (&IopAllocationLock,
			 Irql);
      return NULL;
    }

  LogEntrySize = sizeof(ERROR_LOG_ENTRY) + EntrySize;
  LogEntry = ExAllocatePool (NonPagedPool,
			     LogEntrySize);
  if (LogEntry == NULL)
    {
      KeReleaseSpinLock (&IopAllocationLock,
			 Irql);
      return NULL;
    }

  IopTotalLogSize += EntrySize;

  LogEntry->EntrySize = LogEntrySize;

  KeReleaseSpinLock (&IopAllocationLock,
		     Irql);

  return (PVOID)((ULONG_PTR)LogEntry + sizeof(ERROR_LOG_ENTRY));
}


/*
 * @implemented
 */
VOID STDCALL
IoWriteErrorLogEntry (IN PVOID ElEntry)
{
  PWORK_QUEUE_ITEM LogWorkItem;
  PERROR_LOG_ENTRY LogEntry;
  KIRQL Irql;

  DPRINT1 ("IoWriteErrorLogEntry() called\n");

  LogEntry = (PERROR_LOG_ENTRY)((ULONG_PTR)ElEntry - sizeof(ERROR_LOG_ENTRY));


  /* FIXME: Get logging time */


  KeAcquireSpinLock (&IopLogListLock,
		     &Irql);

  InsertHeadList (&IopLogListHead,
		  &LogEntry->Entry);

  if (IopLogWorkerRunning == FALSE)
    {
      LogWorkItem = ExAllocatePool (NonPagedPool,
				    sizeof(WORK_QUEUE_ITEM));
      if (LogWorkItem != NULL)
	{
	  ExInitializeWorkItem (LogWorkItem,
				IopLogWorker,
				LogWorkItem);

	  ExQueueWorkItem (LogWorkItem,
			   DelayedWorkQueue);

	  IopLogWorkerRunning = TRUE;
	}
    }

  KeReleaseSpinLock (&IopLogListLock,
		     Irql);

  DPRINT1 ("IoWriteErrorLogEntry() done\n");
}

/* EOF */
