/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        tcpip/routines.c
 * PURPOSE:     Common routines
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */

#include "precomp.h"

static UINT RandomNumber = 0x12345678;


UINT Random(
    VOID)
/*
 * FUNCTION: Returns a pseudo random number
 * RETURNS:
 *     Pseudo random number
 */
{
    RandomNumber ^= 0x78563412;

    return RandomNumber;
}


__inline INT SkipToOffset(
    PNDIS_BUFFER Buffer,
    UINT Offset,
    PCHAR *Data,
    PUINT Size)
/*
 * FUNCTION: Skip Offset bytes into a buffer chain
 * ARGUMENTS:
 *     Buffer = Pointer to NDIS buffer
 *     Offset = Number of bytes to skip
 *     Data   = Address of a pointer that on return will contain the
 *              address of the offset in the buffer
 *     Size   = Address of a pointer that on return will contain the
 *              size of the destination buffer
 * RETURNS:
 *     Offset into buffer, -1 if buffer chain was smaller than Offset bytes
 * NOTES:
 *     Buffer may be NULL
 */
{
    for (;;) {

        if (!Buffer)
            return -1;

        NdisQueryBuffer(Buffer, (PVOID)Data, Size);

        if (Offset < *Size) {
            *Data = (PCHAR)((ULONG_PTR) *Data + Offset);
            *Size              -= Offset;
            break;
        }

        Offset -= *Size;

        NdisGetNextBuffer(Buffer, &Buffer);
    }

    return Offset;
}


UINT CopyBufferToBufferChain(
    PNDIS_BUFFER DstBuffer,
    UINT DstOffset,
    PCHAR SrcData,
    UINT Length)
/*
 * FUNCTION: Copies data from a buffer to an NDIS buffer chain
 * ARGUMENTS:
 *     DstBuffer = Pointer to destination NDIS buffer 
 *     DstOffset = Destination start offset
 *     SrcData   = Pointer to source buffer
 *     Length    = Number of bytes to copy
 * RETURNS:
 *     Number of bytes copied to destination buffer
 * NOTES:
 *     The number of bytes copied may be limited by the destination
 *     buffer size
 */
{
    UINT BytesCopied, BytesToCopy, DstSize;
    PCHAR DstData;

    TI_DbgPrint(DEBUG_PBUFFER, ("DstBuffer (0x%X)  DstOffset (0x%X)  SrcData (0x%X)  Length (%d)\n", DstBuffer, DstOffset, SrcData, Length));

    /* Skip DstOffset bytes in the destination buffer chain */
    if (SkipToOffset(DstBuffer, DstOffset, &DstData, &DstSize) == -1)
        return 0;

    /* Start copying the data */
    BytesCopied = 0;
    for (;;) {
        BytesToCopy = MIN(DstSize, Length);

        RtlCopyMemory((PVOID)DstData, (PVOID)SrcData, BytesToCopy);
        BytesCopied += BytesToCopy;
        SrcData      = (PCHAR)((ULONG_PTR)SrcData + BytesToCopy);

        Length -= BytesToCopy;
        if (Length == 0)
            break;

        DstSize -= BytesToCopy;
        if (DstSize == 0) {
            /* No more bytes in desination buffer. Proceed to
               the next buffer in the destination buffer chain */
            NdisGetNextBuffer(DstBuffer, &DstBuffer);
            if (!DstBuffer)
                break;

            NdisQueryBuffer(DstBuffer, (PVOID)&DstData, &DstSize);
        }
    }

    return BytesCopied;
}


UINT CopyBufferChainToBuffer(
    PCHAR DstData,
    PNDIS_BUFFER SrcBuffer,
    UINT SrcOffset,
    UINT Length)
/*
 * FUNCTION: Copies data from an NDIS buffer chain to a buffer
 * ARGUMENTS:
 *     DstData   = Pointer to destination buffer
 *     SrcBuffer = Pointer to source NDIS buffer
 *     SrcOffset = Source start offset
 *     Length    = Number of bytes to copy
 * RETURNS:
 *     Number of bytes copied to destination buffer
 * NOTES:
 *     The number of bytes copied may be limited by the source
 *     buffer size
 */
{
    UINT BytesCopied, BytesToCopy, SrcSize;
    PCHAR SrcData;

    TI_DbgPrint(DEBUG_PBUFFER, ("DstData 0x%X  SrcBuffer 0x%X  SrcOffset 0x%X  Length %d\n",DstData,SrcBuffer, SrcOffset, Length));
    
    /* Skip SrcOffset bytes in the source buffer chain */
    if (SkipToOffset(SrcBuffer, SrcOffset, &SrcData, &SrcSize) == -1)
        return 0;

    /* Start copying the data */
    BytesCopied = 0;
    for (;;) {
        BytesToCopy = MIN(SrcSize, Length);

        TI_DbgPrint(DEBUG_PBUFFER, ("Copying (%d) bytes from 0x%X to 0x%X\n", BytesToCopy, SrcData, DstData));

        RtlCopyMemory((PVOID)DstData, (PVOID)SrcData, BytesToCopy);
        BytesCopied += BytesToCopy;
        DstData      = (PCHAR)((ULONG_PTR)DstData + BytesToCopy);

        Length -= BytesToCopy;
        if (Length == 0)
            break;

        SrcSize -= BytesToCopy;
        if (SrcSize == 0) {
            /* No more bytes in source buffer. Proceed to
               the next buffer in the source buffer chain */
            NdisGetNextBuffer(SrcBuffer, &SrcBuffer);
            if (!SrcBuffer)
                break;

            NdisQueryBuffer(SrcBuffer, (PVOID)&SrcData, &SrcSize);
        }
    }

    return BytesCopied;
}


UINT CopyPacketToBuffer(
    PCHAR DstData,
    PNDIS_PACKET SrcPacket,
    UINT SrcOffset,
    UINT Length)
/*
 * FUNCTION: Copies data from an NDIS packet to a buffer
 * ARGUMENTS:
 *     DstData   = Pointer to destination buffer
 *     SrcPacket = Pointer to source NDIS packet
 *     SrcOffset = Source start offset
 *     Length    = Number of bytes to copy
 * RETURNS:
 *     Number of bytes copied to destination buffer
 * NOTES:
 *     The number of bytes copied may be limited by the source
 *     buffer size
 */
{
    PNDIS_BUFFER FirstBuffer;
    PVOID Address;
    UINT FirstLength;
    UINT TotalLength;

    TI_DbgPrint(DEBUG_PBUFFER, ("DstData (0x%X)  SrcPacket (0x%X)  SrcOffset (0x%X)  Length (%d)\n", DstData, SrcPacket, SrcOffset, Length));

    NdisGetFirstBufferFromPacket(SrcPacket,
                                 &FirstBuffer,
                                 &Address,
                                 &FirstLength,
                                 &TotalLength);

    return CopyBufferChainToBuffer(DstData, FirstBuffer, SrcOffset, Length);
}


UINT CopyPacketToBufferChain(
    PNDIS_BUFFER DstBuffer,
    UINT DstOffset,
    PNDIS_PACKET SrcPacket,
    UINT SrcOffset,
    UINT Length)
/*
 * FUNCTION: Copies data from an NDIS packet to an NDIS buffer chain
 * ARGUMENTS:
 *     DstBuffer = Pointer to destination NDIS buffer
 *     DstOffset = Destination start offset
 *     SrcPacket = Pointer to source NDIS packet
 *     SrcOffset = Source start offset
 *     Length    = Number of bytes to copy
 * RETURNS:
 *     Number of bytes copied to destination buffer
 * NOTES:
 *     The number of bytes copied may be limited by the source and
 *     destination buffer sizes
 */
{
    PNDIS_BUFFER SrcBuffer;
    PCHAR DstData, SrcData;
    UINT DstSize, SrcSize;
    UINT Count, Total;

    TI_DbgPrint(DEBUG_PBUFFER, ("DstBuffer (0x%X)  DstOffset (0x%X)  SrcPacket (0x%X)  SrcOffset (0x%X)  Length (%d)\n", DstBuffer, DstOffset, SrcPacket, SrcOffset, Length));

    /* Skip DstOffset bytes in the destination buffer chain */
    NdisQueryBuffer(DstBuffer, (PVOID)&DstData, &DstSize);
    if (SkipToOffset(DstBuffer, DstOffset, &DstData, &DstSize) == -1)
        return 0;

    /* Skip SrcOffset bytes in the source packet */
    NdisGetFirstBufferFromPacket(SrcPacket, &SrcBuffer, &SrcData, &SrcSize, &Total);
    if (SkipToOffset(SrcBuffer, SrcOffset, &SrcData, &SrcSize) == -1)
        return 0;

    /* Copy the data */
    for (Total = 0;;) {
        /* Find out how many bytes we can copy at one time */
        if (Length < SrcSize)
            Count = Length;
        else
            Count = SrcSize;
        if (DstSize < Count)
            Count = DstSize;

        RtlCopyMemory((PVOID)DstData, (PVOID)SrcData, Count);

        Total  += Count;
        Length -= Count;
        if (Length == 0)
            break;

        DstSize -= Count;
        if (DstSize == 0) {
            /* No more bytes in destination buffer. Proceed to
               the next buffer in the destination buffer chain */
            NdisGetNextBuffer(DstBuffer, &DstBuffer);
            if (!DstBuffer)
                break;

            NdisQueryBuffer(DstBuffer, (PVOID)&DstData, &DstSize);
        }

        SrcSize -= Count;
        if (SrcSize == 0) {
            /* No more bytes in source buffer. Proceed to
               the next buffer in the source buffer chain */
            NdisGetNextBuffer(SrcBuffer, &SrcBuffer);
            if (!SrcBuffer)
                break;

            NdisQueryBuffer(SrcBuffer, (PVOID)&SrcData, &SrcSize);
        }
    }

    return Total;
}


PVOID AdjustPacket(
    PNDIS_PACKET Packet,
    UINT Available,
    UINT Needed)
/*
 * FUNCTION: Adjusts the amount of unused space at the beginning of the packet
 * ARGUMENTS:
 *     Packet    = Pointer to packet
 *     Available = Number of bytes available at start of first buffer
 *     Needed    = Number of bytes needed for the header
 * RETURNS:
 *     Pointer to start of packet
 */
{
    PNDIS_BUFFER NdisBuffer;
    INT Adjust;

    TI_DbgPrint(DEBUG_PBUFFER, ("Available = %d, Needed = %d.\n", Available, Needed));

    Adjust = Available - Needed;

    NdisQueryPacket(Packet, NULL, NULL, &NdisBuffer, NULL);

    /* If Adjust is zero there is no need to adjust this packet as
       there is no additional space at start the of first buffer */
    if (Adjust != 0) {
        NdisBuffer->MappedSystemVa  = (PVOID) ((ULONG_PTR)(NdisBuffer->MappedSystemVa) + Adjust);
        NdisBuffer->ByteOffset     += Adjust;
        NdisBuffer->ByteCount      -= Adjust;
    }

    return NdisBuffer->MappedSystemVa;
}


UINT ResizePacket(
    PNDIS_PACKET Packet,
    UINT Size)
/*
 * FUNCTION: Resizes an NDIS packet
 * ARGUMENTS:
 *     Packet = Pointer to packet
 *     Size   = Number of bytes in first buffer
 * RETURNS:
 *     Previous size of first buffer
 */
{
    PNDIS_BUFFER NdisBuffer;
    UINT OldSize;

    NdisQueryPacket(Packet, NULL, NULL, &NdisBuffer, NULL);

    OldSize = NdisBuffer->ByteCount;

    if (Size != OldSize)
        NdisBuffer->ByteCount = Size;

    return OldSize;
}

#ifdef DBG

static VOID DisplayIPHeader(
    PCHAR Header,
    UINT Length)
{
    /* FIXME: IPv4 only */
    PIPv4_HEADER IPHeader = (PIPv4_HEADER)Header;

    DbgPrint("IPv4 header:\n");
    DbgPrint("VerIHL: 0x%x (version 0x%x, length %d 32-bit words)\n",
      IPHeader->VerIHL, (IPHeader->VerIHL & 0xF0) >> 4, IPHeader->VerIHL & 0x0F);
    DbgPrint("  Tos: %d\n", IPHeader->Tos);
    DbgPrint("  TotalLength: %d\n", WN2H(IPHeader->TotalLength));
    DbgPrint("  Id: %d\n", WN2H(IPHeader->Id));
    DbgPrint("  FlagsFragOfs: 0x%x (offset 0x%x)\n", WN2H(IPHeader->FlagsFragOfs), WN2H(IPHeader->FlagsFragOfs) & IPv4_FRAGOFS_MASK);
    if ((WN2H(IPHeader->FlagsFragOfs) & IPv4_DF_MASK) > 0) DbgPrint("    IPv4_DF - Don't fragment\n");
    if ((WN2H(IPHeader->FlagsFragOfs) & IPv4_MF_MASK) > 0) DbgPrint("    IPv4_MF - More fragments\n");
    DbgPrint("  Ttl: %d\n", IPHeader->Ttl);
    DbgPrint("  Protocol: %d\n", IPHeader->Protocol);
    DbgPrint("  Checksum: 0x%x\n", WN2H(IPHeader->Checksum));
    DbgPrint("  SrcAddr: %d.%d.%d.%d\n",
      ((IPHeader->SrcAddr >> 0) & 0xFF), ((IPHeader->SrcAddr >> 8) & 0xFF),
      ((IPHeader->SrcAddr >> 16) & 0xFF), ((IPHeader->SrcAddr >> 24) & 0xFF));
    DbgPrint("  DstAddr: %d.%d.%d.%d\n",
      ((IPHeader->DstAddr >> 0) & 0xFF), ((IPHeader->DstAddr >> 8) & 0xFF),
      ((IPHeader->DstAddr >> 16) & 0xFF), ((IPHeader->DstAddr >> 24) & 0xFF));
}

VOID DisplayIPPacket(
    PIP_PACKET IPPacket)
{
    PCHAR p;
    UINT Length;
    PNDIS_BUFFER Buffer;
    PNDIS_BUFFER NextBuffer;
    PCHAR CharBuffer;

    if ((DebugTraceLevel & (DEBUG_PBUFFER | DEBUG_IP)) != (DEBUG_PBUFFER | DEBUG_IP)) {
        return;
    }

    if (!IPPacket) {
        TI_DbgPrint(MIN_TRACE, ("Cannot display null packet.\n"));
        return;
    }

	  TI_DbgPrint(MIN_TRACE, ("IPPacket is at (0x%X).\n", IPPacket));
    TI_DbgPrint(MIN_TRACE, ("Header buffer is at (0x%X).\n", IPPacket->Header));
    TI_DbgPrint(MIN_TRACE, ("Header size is (%d).\n", IPPacket->HeaderSize));
    TI_DbgPrint(MIN_TRACE, ("TotalSize (%d).\n", IPPacket->TotalSize));
    TI_DbgPrint(MIN_TRACE, ("ContigSize (%d).\n", IPPacket->ContigSize));
    TI_DbgPrint(MIN_TRACE, ("NdisPacket (0x%X).\n", IPPacket->NdisPacket));

    if (IPPacket->NdisPacket) {
        NdisQueryPacket(IPPacket->NdisPacket, NULL, NULL, &Buffer, NULL);
        for (; Buffer != NULL; Buffer = NextBuffer) {
            NdisGetNextBuffer(Buffer, &NextBuffer);
            NdisQueryBuffer(Buffer, (PVOID)&p, &Length);
	    OskitDumpBuffer( p, Length );
        }
    } else {
        p      = IPPacket->Header;
        Length = IPPacket->ContigSize;
	OskitDumpBuffer( p, Length );
    }

    if (IPPacket->NdisPacket) {
        NdisQueryPacket(IPPacket->NdisPacket, NULL, NULL, NULL, &Length);
        Length -= MaxLLHeaderSize;
        CharBuffer = exAllocatePool(NonPagedPool, Length);
        Length = CopyPacketToBuffer(CharBuffer, IPPacket->NdisPacket, MaxLLHeaderSize, Length);
        DisplayIPHeader(CharBuffer, Length);
        exFreePool(CharBuffer);
    } else {
        CharBuffer = IPPacket->Header;
        Length = IPPacket->ContigSize;
        DisplayIPHeader(CharBuffer, Length);
    }
}


static VOID DisplayTCPHeader(
    PCHAR Header,
    UINT Length)
{
    /* FIXME: IPv4 only */
    PIPv4_HEADER IPHeader = (PIPv4_HEADER)Header;
    PTCPv4_HEADER TCPHeader;

    if (IPHeader->Protocol != IPPROTO_TCP) {
        DbgPrint("This is not a TCP datagram. Protocol is %d\n", IPHeader->Protocol);
        return;
    }

    TCPHeader = (PTCPv4_HEADER)((PCHAR)IPHeader + (IPHeader->VerIHL & 0x0F) * 4);

    DbgPrint("TCP header:\n");
    DbgPrint("  SourcePort: %d\n", WN2H(TCPHeader->SourcePort));
    DbgPrint("  DestinationPort: %d\n", WN2H(TCPHeader->DestinationPort));
    DbgPrint("  SequenceNumber: 0x%x\n", DN2H(TCPHeader->SequenceNumber));
    DbgPrint("  AckNumber: 0x%x\n", DN2H(TCPHeader->AckNumber));
    DbgPrint("  DataOffset: 0x%x (0x%x) 32-bit words\n", TCPHeader->DataOffset, TCPHeader->DataOffset >> 4);
    DbgPrint("  Flags: 0x%x (0x%x)\n", TCPHeader->Flags, TCPHeader->Flags & 0x3F);
    if ((TCPHeader->Flags & TCP_URG) > 0) DbgPrint("    TCP_URG - Urgent Pointer field significant\n");
    if ((TCPHeader->Flags & TCP_ACK) > 0) DbgPrint("    TCP_ACK - Acknowledgement field significant\n");
    if ((TCPHeader->Flags & TCP_PSH) > 0) DbgPrint("    TCP_PSH - Push Function\n");
    if ((TCPHeader->Flags & TCP_RST) > 0) DbgPrint("    TCP_RST - Reset the connection\n");
    if ((TCPHeader->Flags & TCP_SYN) > 0) DbgPrint("    TCP_SYN - Synchronize sequence numbers\n");
    if ((TCPHeader->Flags & TCP_FIN) > 0) DbgPrint("    TCP_FIN - No more data from sender\n");
    DbgPrint("  Window: 0x%x\n", WN2H(TCPHeader->Window));
    DbgPrint("  Checksum: 0x%x\n", WN2H(TCPHeader->Checksum));
    DbgPrint("  Urgent: 0x%x\n", WN2H(TCPHeader->Urgent));
}


VOID DisplayTCPPacket(
    PIP_PACKET IPPacket)
{
    UINT Length;
    PCHAR Buffer;

    if ((DebugTraceLevel & (DEBUG_PBUFFER | DEBUG_TCP)) != (DEBUG_PBUFFER | DEBUG_TCP)) {
        return;
    }

    if (!IPPacket) {
        TI_DbgPrint(MIN_TRACE, ("Cannot display null packet.\n"));
        return;
    }

    DisplayIPPacket(IPPacket);

	  TI_DbgPrint(MIN_TRACE, ("IPPacket is at (0x%X).\n", IPPacket));
    TI_DbgPrint(MIN_TRACE, ("Header buffer is at (0x%X).\n", IPPacket->Header));
    TI_DbgPrint(MIN_TRACE, ("Header size is (%d).\n", IPPacket->HeaderSize));
    TI_DbgPrint(MIN_TRACE, ("TotalSize (%d).\n", IPPacket->TotalSize));
    TI_DbgPrint(MIN_TRACE, ("ContigSize (%d).\n", IPPacket->ContigSize));
    TI_DbgPrint(MIN_TRACE, ("NdisPacket (0x%X).\n", IPPacket->NdisPacket));

    if (IPPacket->NdisPacket) {
        NdisQueryPacket(IPPacket->NdisPacket, NULL, NULL, NULL, &Length);
        Length -= MaxLLHeaderSize;
        Buffer = exAllocatePool(NonPagedPool, Length);
        Length = CopyPacketToBuffer(Buffer, IPPacket->NdisPacket, MaxLLHeaderSize, Length);
        DisplayTCPHeader(Buffer, Length);
        exFreePool(Buffer);
    } else {
        Buffer = IPPacket->Header;
        Length = IPPacket->ContigSize;
        DisplayTCPHeader(Buffer, Length);
    }
}

#endif/* DBG */

void GetDataPtr( PNDIS_PACKET Packet,
		 UINT Offset, 
		 PCHAR *DataOut,
		 PUINT Size ) {
    PNDIS_BUFFER Buffer;

    NdisQueryPacket(Packet, NULL, NULL, &Buffer, NULL);
    if( !Buffer ) return;
    SkipToOffset( Buffer, Offset, DataOut, Size );
}


#undef NdisAllocatePacket
#undef NdisAllocateBuffer
#undef NdisFreeBuffer
#undef NdisFreePacket

NDIS_STATUS AllocatePacketWithBufferX( PNDIS_PACKET *NdisPacket,
				       PCHAR Data, UINT Len,
				       PCHAR File, UINT Line ) {
    PNDIS_PACKET Packet;
    PNDIS_BUFFER Buffer;
    NDIS_STATUS Status;
    PCHAR NewData;

    NewData = ExAllocatePool( NonPagedPool, Len );
    if( !NewData ) return NDIS_STATUS_NOT_ACCEPTED; // XXX 
    TrackWithTag(EXALLOC_TAG, NewData, File, Line);

    if( Data ) 
	RtlCopyMemory(NewData, Data, Len);

    NdisAllocatePacket( &Status, &Packet, GlobalPacketPool );
    if( Status != NDIS_STATUS_SUCCESS ) {
	ExFreePool( NewData );
	return Status;
    }
    TrackWithTag(NDIS_PACKET_TAG, Packet, File, Line);

    NdisAllocateBuffer( &Status, &Buffer, GlobalBufferPool, NewData, Len );
    if( Status != NDIS_STATUS_SUCCESS ) {
	ExFreePool( NewData );
	FreeNdisPacket( Packet );
    }
    TrackWithTag(NDIS_BUFFER_TAG, Buffer, File, Line);

    NdisChainBufferAtFront( Packet, Buffer );
    *NdisPacket = Packet;

    return NDIS_STATUS_SUCCESS;
}


VOID FreeNdisPacketX
( PNDIS_PACKET Packet,
  PCHAR File,
  UINT Line )
/*
 * FUNCTION: Frees an NDIS packet
 * ARGUMENTS:
 *     Packet = Pointer to NDIS packet to be freed
 */
{
    PNDIS_BUFFER Buffer, NextBuffer;

    TI_DbgPrint(DEBUG_PBUFFER, ("Packet (0x%X)\n", Packet));

    /* Free all the buffers in the packet first */
    NdisQueryPacket(Packet, NULL, NULL, &Buffer, NULL);
    for (; Buffer != NULL; Buffer = NextBuffer) {
        PVOID Data;
        UINT Length;

        NdisGetNextBuffer(Buffer, &NextBuffer);
        NdisQueryBuffer(Buffer, &Data, &Length);
	UntrackFL(File,Line,Buffer);
        NdisFreeBuffer(Buffer);
	UntrackFL(File,Line,Data);
        ExFreePool(Data);
    }

    /* Finally free the NDIS packet discriptor */
    UntrackFL(File,Line,Packet);
    NdisFreePacket(Packet);
}
