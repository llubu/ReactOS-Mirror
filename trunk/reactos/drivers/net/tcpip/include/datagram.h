/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        include/datagram.h
 * PURPOSE:     Datagram types and constants
 */
#ifndef __DATAGRAM_H
#define __DATAGRAM_H

#include <titypes.h>


VOID DGSend(
  PVOID Context,
  PDATAGRAM_SEND_REQUEST SendRequest);

VOID DGDeliverData(
  PADDRESS_FILE AddrFile,
  PIP_ADDRESS Address,
  PIP_PACKET IPPacket,
  UINT DataSize);

VOID DGCancelSendRequest(
  PADDRESS_FILE AddrFile,
  PVOID Context);

VOID DGCancelReceiveRequest(
  PADDRESS_FILE AddrFile,
  PVOID Context);

NTSTATUS DGTransmit(
  PADDRESS_FILE AddressFile,
  PDATAGRAM_SEND_REQUEST SendRequest);

NTSTATUS DGSendDatagram(
    PADDRESS_FILE AddrFile,
    PTDI_CONNECTION_INFORMATION ConnInfo,
    PCHAR BufferData,
    ULONG DataSize,
    PULONG DataUsed );

NTSTATUS DGReceiveDatagram(
    PADDRESS_FILE AddrFile,
    PTDI_CONNECTION_INFORMATION ConnInfo,
    PCHAR Buffer,
    ULONG ReceiveLength,
    ULONG ReceiveFlags,
    PTDI_CONNECTION_INFORMATION ReturnInfo,
    PULONG BytesReceived);

NTSTATUS DGStartup(
  VOID);

NTSTATUS DGShutdown(
  VOID);

#endif /* __DATAGRAM_H */

/* EOF */
