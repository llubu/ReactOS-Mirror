/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        include/tcp.h
 * PURPOSE:     Transmission Control Protocol definitions
 */
#ifndef __TCP_H
#define __TCP_H

/* TCPv4 header structure */
typedef struct TCPv4_HEADER {
  USHORT SourcePort;        /* Source port */
  USHORT DestinationPort;   /* Destination port */
  ULONG  SequenceNumber;    /* Sequence number */
  ULONG  AckNumber;         /* Acknowledgement number */
  UCHAR  DataOffset;        /* Data offset; 32-bit words (leftmost 4 bits) */
  UCHAR  Flags;             /* Control bits (rightmost 6 bits) */
  USHORT Window;            /* Maximum acceptable receive window */
  USHORT Checksum;          /* Checksum of segment */
  USHORT Urgent;            /* Pointer to urgent data */
} __attribute__((packed)) TCPv4_HEADER, *PTCPv4_HEADER;

/* TCPv4 header flags */
#define TCP_URG   0x20
#define TCP_ACK   0x10
#define TCP_PSH   0x08
#define TCP_RST   0x04
#define TCP_SYN   0x02
#define TCP_FIN   0x01


#define TCPOPT_END_OF_LIST  0x0
#define TCPOPT_NO_OPERATION 0x1
#define TCPOPT_MAX_SEG_SIZE 0x2

#define TCPOPTLEN_MAX_SEG_SIZE  0x4


/* TCPv4 pseudo header */
typedef struct TCPv4_PSEUDO_HEADER {
  ULONG SourceAddress;      /* Source address */
  ULONG DestinationAddress; /* Destination address */
  UCHAR Zero;               /* Reserved */
  UCHAR Protocol;           /* Protocol */
  USHORT TCPLength;         /* Size of TCP segment */
} __attribute__((packed)) TCPv4_PSEUDO_HEADER, *PTCPv4_PSEUDO_HEADER;


/* Retransmission timeout constants */

/* Lower bound for retransmission timeout in TCP timer ticks */
#define TCP_MIN_RETRANSMISSION_TIMEOUT    1*1000          /* 1 tick */

/* Upper bound for retransmission timeout in TCP timer ticks */
#define TCP_MAX_RETRANSMISSION_TIMEOUT    1*60*1000       /* 1 tick */

/* Smoothing factor */
#define TCP_ALPHA_RETRANSMISSION_TIMEOUT(x)(((x)*8)/10)   /* 0.8 */

/* Delay variance factor */
#define TCP_BETA_RETRANSMISSION_TIMEOUT(x)(((x)*16)/10)   /* 1.6 */


/* Datagram/segment send request flags */

#define SRF_URG   TCP_URG
#define SRF_ACK   TCP_ACK
#define SRF_PSH   TCP_PSH
#define SRF_RST   TCP_RST
#define SRF_SYN   TCP_SYN
#define SRF_FIN   TCP_FIN


PTCP_SEGMENT TCPCreateSegment(
  PIP_PACKET IPPacket,
  ULONG SequenceNumber,
  ULONG SegmentLength);

VOID TCPFreeSegment(
  PTCP_SEGMENT Segment);

VOID TCPAddSegment(
  PCONNECTION_ENDPOINT Connection,
  PTCP_SEGMENT Segment);

inline NTSTATUS TCPBuildSendRequest(
    PTCP_SEND_REQUEST *SendRequest,
    PDATAGRAM_SEND_REQUEST *DGSendRequest,
    PCONNECTION_ENDPOINT Connection,
    DATAGRAM_COMPLETION_ROUTINE Complete,
    PVOID Context,
    PNDIS_BUFFER Buffer,
    DWORD BufferSize,
    ULONG Flags);

inline NTSTATUS TCPBuildAndTransmitSendRequest(
    PCONNECTION_ENDPOINT Connection,
    DATAGRAM_COMPLETION_ROUTINE Complete,
    PVOID Context,
    PNDIS_BUFFER Buffer,
    DWORD BufferSize,
    ULONG Flags);

NTSTATUS TCPConnect(
  PTDI_REQUEST Request,
  PTDI_CONNECTION_INFORMATION ConnInfo,
  PTDI_CONNECTION_INFORMATION ReturnInfo);

NTSTATUS TCPListen(
  PTDI_REQUEST Request,
  PTDI_CONNECTION_INFORMATION ConnInfo,
  PTDI_CONNECTION_INFORMATION ReturnInfo);

NTSTATUS TCPSendDatagram(
  PTDI_REQUEST Request,
  PTDI_CONNECTION_INFORMATION ConnInfo,
  PNDIS_BUFFER Buffer,
  ULONG DataSize);

NTSTATUS TCPStartup(
  VOID);

NTSTATUS TCPShutdown(
  VOID);

#endif /* __TCP_H */
