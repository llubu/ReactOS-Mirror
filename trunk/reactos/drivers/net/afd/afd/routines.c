/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Ancillary Function Driver
 * FILE:        afd/routines.c
 * PURPOSE:     Support routines
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/02-2001 Created
 */
#include <afd.h>
#include <debug.h>


VOID DumpName(
  LPSOCKADDR Name)
{
  AFD_DbgPrint(MIN_TRACE, ("DumpName:\n"));
  AFD_DbgPrint(MIN_TRACE, ("  sa_family: %d\n", Name->sa_family));
  AFD_DbgPrint(MIN_TRACE, ("  sin_port: %d\n", WN2H(((LPSOCKADDR_IN)Name)->sin_port)));
  AFD_DbgPrint(MIN_TRACE, ("  in_addr: 0x%x\n", WN2H(((LPSOCKADDR_IN)Name)->sin_addr.S_un.S_addr)));
}


ULONG WSABufferSize(
  LPWSABUF Buffers,
  DWORD BufferCount)
{
  ULONG i;
  LPWSABUF p;
  ULONG Count = 0;

  p = Buffers;
  for (i = 0; i < BufferCount; i++) {
    Count += p->len;
    p++;
  }

  AFD_DbgPrint(MAX_TRACE, ("Buffer is %d bytes.\n", Count));

  return Count;
}


NTSTATUS MergeWSABuffers(
  LPWSABUF Buffers,
  DWORD BufferCount,
  PVOID Destination,
  ULONG MaxLength,
  PULONG BytesCopied)
{
  ULONG Length;
  LPWSABUF p;
  ULONG i;

  *BytesCopied = 0;
  if (BufferCount == 0)
    return STATUS_SUCCESS;

  p = Buffers;

  AFD_DbgPrint(MAX_TRACE, ("Destination is 0x%X\n", Destination));
  AFD_DbgPrint(MAX_TRACE, ("p is 0x%X\n", p));

  for (i = 0; i < BufferCount; i++) {
    Length = p->len;
    if (Length > MaxLength)
      /* Don't copy out of bounds */
      Length = MaxLength;

    RtlCopyMemory(Destination, p->buf, Length);
    Destination += Length;
    AFD_DbgPrint(MAX_TRACE, ("Destination is 0x%X\n", Destination));
    p++;
    AFD_DbgPrint(MAX_TRACE, ("p is 0x%X\n", p));

    *BytesCopied += Length;

    MaxLength -= Length;
    if (MaxLength == 0)
      /* Destination buffer is full */
      break;
  }

  return STATUS_SUCCESS;
}

/*
 * NOTES: ReceiveQueueLock must be acquired for the FCB when called
 */
NTSTATUS FillWSABuffers(
    PAFDFCB FCB,
    LPWSABUF Buffers,
    DWORD BufferCount,
    PULONG BytesCopied)
{
  PUCHAR DstData, SrcData;
  UINT DstSize, SrcSize;
  UINT Count, Total;
  PAFD_BUFFER SrcBuffer;
  PLIST_ENTRY Entry;

  *BytesCopied = 0;
  if (BufferCount == 0)
    return STATUS_SUCCESS;

  if (IsListEmpty(&FCB->ReceiveQueue))
    return STATUS_SUCCESS;

  Entry = RemoveHeadList(&FCB->ReceiveQueue);
  SrcBuffer = CONTAINING_RECORD(Entry, AFD_BUFFER, ListEntry);
  SrcData = SrcBuffer->Buffer.buf;
  SrcSize = SrcBuffer->Buffer.len;

  DstData = Buffers->buf;
  DstSize = Buffers->len;

  /* Copy the data */
  for (Total = 0;;) {
    /* Find out how many bytes we can copy at one time */
    if (DstSize < SrcSize)
      Count = DstSize;
    else
      Count = SrcSize;

    AFD_DbgPrint(MAX_TRACE, ("DstData (0x%X) SrcData (0x%X) Count (0x%X).\n",
      DstData, SrcData, Count));

    RtlCopyMemory((PVOID)DstData, (PVOID)SrcData, Count);

    Total += Count;

    SrcSize -= Count;
    if (SrcSize == 0) {
      ExFreePool(SrcBuffer->Buffer.buf);
      ExFreePool(SrcBuffer);

      /* No more bytes in source buffer. Proceed to the next buffer
         in the source buffer chain if there is one */
      if (IsListEmpty(&FCB->ReceiveQueue)) {
        SrcBuffer = NULL;
        SrcData = 0;
        SrcSize = 0;
        break;
      }

      Entry = RemoveHeadList(&FCB->ReceiveQueue);
      SrcBuffer = CONTAINING_RECORD(Entry, AFD_BUFFER, ListEntry);
      SrcData = SrcBuffer->Buffer.buf;
      SrcSize = SrcBuffer->Buffer.len;
    }

    DstSize -= Count;
    if (DstSize == 0) {
      /* No more bytes in destination buffer. Proceed to
         the next buffer in the destination buffer chain */
      BufferCount--;
      if (BufferCount < 1)
        break;
      Buffers++;
      DstData = Buffers->buf;
      DstSize = Buffers->len;
    }
  }

  if (SrcSize > 0) {
    InsertHeadList(&FCB->ReceiveQueue, Entry);
  } else if (SrcBuffer != NULL) {
    ExFreePool(SrcBuffer->Buffer.buf);
    ExFreePool(SrcBuffer);
  }

  *BytesCopied = Total;

  return STATUS_SUCCESS;
}

ULONG ChecksumCompute(
    PVOID Data,
    UINT Count,
    ULONG Seed)
/*
 * FUNCTION: Calculate checksum of a buffer
 * ARGUMENTS:
 *     Data  = Pointer to buffer with data
 *     Count = Number of bytes in buffer
 *     Seed  = Previously calculated checksum (if any)
 * RETURNS:
 *     Checksum of buffer
 */
{
  /* FIXME: This should be done in assembler */

  register ULONG Sum = Seed;

  while (Count > 1) {
    Sum   += *(PUSHORT)Data;
    Count -= 2;
    Data   = (PVOID) ((ULONG_PTR) Data + 2);
  }

  /* Add left-over byte, if any */
  if (Count > 0)
    Sum += *(PUCHAR)Data;

  /* Fold 32-bit sum to 16 bits */
  while (Sum >> 16)
    Sum = (Sum & 0xFFFF) + (Sum >> 16);

  return ~Sum;
}

VOID BuildIPv4Header(
  PIPv4_HEADER IPHeader,
  ULONG TotalSize,
  ULONG Protocol,
  PSOCKADDR SourceAddress,
  PSOCKADDR DestinationAddress)
{
  PSOCKADDR_IN SrcNameIn = (PSOCKADDR_IN)SourceAddress;
  PSOCKADDR_IN DstNameIn = (PSOCKADDR_IN)DestinationAddress;

  /* Version = 4, Length = 5 DWORDs */
  IPHeader->VerIHL = 0x45;
  /* Normal Type-of-Service */
  IPHeader->Tos = 0;
  /* Length of header and data */
  IPHeader->TotalLength = WH2N((USHORT)TotalSize);
  /* Identification */
  IPHeader->Id = 0;
  /* One fragment at offset 0 */
  IPHeader->FlagsFragOfs = 0;
  /* Time-to-Live is 128 */
  IPHeader->Ttl = 128;
  /* Protocol number */
  IPHeader->Protocol = Protocol;
  /* Checksum is 0 (calculated later) */
  IPHeader->Checksum = 0;
  /* Source address */
  IPHeader->SrcAddr = SrcNameIn->sin_addr.S_un.S_addr;
  /* Destination address */
  IPHeader->DstAddr = DstNameIn->sin_addr.S_un.S_addr;

  /* Calculate checksum of IP header */
  IPHeader->Checksum = (USHORT)
    ChecksumCompute(IPHeader, sizeof(IPv4_HEADER), 0);
}

/* EOF */
