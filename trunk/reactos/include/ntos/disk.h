/* $Id: disk.h,v 1.12 2002/11/14 18:21:03 chorns Exp $
 *
 * COPYRIGHT:    See COPYING in the top level directory
 * PROJECT:      ReactOS kernel
 * FILE:         include/disk.h
 * PURPOSE:      Disk related definitions used by all the parts of the system
 * PROGRAMMER:   David Welch <welch@cwcom.net>
 * UPDATE HISTORY: 
 *               27/06/00: Created
 */

#ifndef __INCLUDE_DISK_H
#define __INCLUDE_DISK_H


#define IOCTL_DISK_GET_DRIVE_GEOMETRY   CTL_CODE(FILE_DEVICE_DISK,  0, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DISK_GET_PARTITION_INFO   CTL_CODE(FILE_DEVICE_DISK,  1, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_DISK_SET_PARTITION_INFO   CTL_CODE(FILE_DEVICE_DISK,  2, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_DISK_GET_DRIVE_LAYOUT     CTL_CODE(FILE_DEVICE_DISK,  3, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_DISK_SET_DRIVE_LAYOUT     CTL_CODE(FILE_DEVICE_DISK,  4, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_DISK_VERIFY               CTL_CODE(FILE_DEVICE_DISK,  5, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DISK_FORMAT_TRACKS        CTL_CODE(FILE_DEVICE_DISK,  6, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_DISK_REASSIGN_BLOCKS      CTL_CODE(FILE_DEVICE_DISK,  7, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_DISK_PERFORMANCE          CTL_CODE(FILE_DEVICE_DISK,  8, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DISK_IS_WRITABLE          CTL_CODE(FILE_DEVICE_DISK,  9, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DISK_LOGGING              CTL_CODE(FILE_DEVICE_DISK, 10, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DISK_FORMAT_TRACKS_EX     CTL_CODE(FILE_DEVICE_DISK, 11, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_DISK_HISTOGRAM_STRUCTURE  CTL_CODE(FILE_DEVICE_DISK, 12, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DISK_HISTOGRAM_DATA       CTL_CODE(FILE_DEVICE_DISK, 13, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DISK_HISTOGRAM_RESET      CTL_CODE(FILE_DEVICE_DISK, 14, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DISK_REQUEST_STRUCTURE    CTL_CODE(FILE_DEVICE_DISK, 15, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DISK_REQUEST_DATA         CTL_CODE(FILE_DEVICE_DISK, 16, METHOD_BUFFERED, FILE_ANY_ACCESS)


#define PARTITION_ENTRY_UNUSED          0x00
#define PARTITION_FAT_12                0x01
#define PARTITION_XENIX_1               0x02
#define PARTITION_XENIX_2               0x03
#define PARTITION_FAT_16                0x04
#define PARTITION_EXTENDED              0x05
#define PARTITION_HUGE                  0x06
#define PARTITION_IFS                   0x07
#define PARTITION_FAT32                 0x0B
#define PARTITION_FAT32_XINT13          0x0C
#define PARTITION_XINT13                0x0E
#define PARTITION_XINT13_EXTENDED       0x0F
#define PARTITION_PREP                  0x41
#define PARTITION_LDM                   0x42
#define PARTITION_UNIX                  0x63

#define PARTITION_NTFT                  0x80
#define VALID_NTFT                      0xC0

#if 0
#define PTEmpty                         0x00
#define PTDOS3xPrimary                  0x01
#define PTXENIXRoot                     0x02
#define PTXENIXUsr                      0x03
#define PTOLDDOS16Bit                   0x04
#define PTDosExtended                   0x05
#define PTDos5xPrimary                  0x06
#define PTIfs                           0x07	// e.g.: HPFS, NTFS, etc
#define PTAIX                           0x08
#define PTAIXBootable                   0x09
#define PTOS2BootMgr                    0x0A
#define PTWin95FAT32                    0x0B
#define PTWin95FAT32LBA                 0x0C
#define PTWin95FAT16LBA                 0x0E
#define PTWin95ExtendedLBA              0x0F
#define PTVenix286                      0x40
#define PTNovell                        0x51
#define PTMicroport                     0x52
#define PTGnuHurd                       0x63
#define PTNetware286                    0x64
#define PTNetware386                    0x65
#define PTPCIX                          0x75
#define PTOldMinix                      0x80
#define PTMinix                         0x81
#define PTLinuxSwap                     0x82
#define PTLinuxExt2                     0x83
#define PTAmoeba                        0x93
#define PTAmoebaBBT                     0x94
#define PTBSD                           0xA5
#define PTBSDIFS                        0xB7
#define PTBSDISwap                      0xB8
#define PTSyrinx                        0xC7
#define PTCPM                           0xDB
#define PTDOSAccess                     0xE1
#define PTDOSRO                         0xE3
#define PTDOSSecondary                  0xF2
#define PTBBT                           0xFF
#endif

#define IsRecognizedPartition(P)  \
    ((P) == PARTITION_FAT_12       || \
     (P) == PARTITION_FAT_16       || \
     (P) == PARTITION_HUGE         || \
     (P) == PARTITION_IFS          || \
     (P) == PARTITION_FAT32        || \
     (P) == PARTITION_FAT32_XINT13 || \
     (P) == PARTITION_XINT13)

#define IsContainerPartition(P)  \
    ((P) == PARTITION_EXTENDED || \
     (P) == PARTITION_XINT13_EXTENDED)


typedef enum _MEDIA_TYPE
{
  Unknown,
  F5_1Pt2_512,
  F3_1Pt44_512,
  F3_2Pt88_512,
  F3_20Pt8_512,
  F3_720_512,
  F5_360_512,
  F5_320_512,
  F5_320_1024,
  F5_180_512,
  F5_160_512,
  RemovableMedia,
  FixedMedia
} MEDIA_TYPE;

typedef struct _PARTITION_INFORMATION
{
  LARGE_INTEGER StartingOffset;
  LARGE_INTEGER PartitionLength;
  DWORD HiddenSectors;
  DWORD PartitionNumber;
  BYTE PartitionType;
  BOOLEAN BootIndicator;
  BOOLEAN RecognizedPartition;
  BOOLEAN RewritePartition;
} PARTITION_INFORMATION, *PPARTITION_INFORMATION;

typedef struct _SET_PARTITION_INFORMATION
{
  ULONG PartitionType;
} SET_PARTITION_INFORMATION, *PSET_PARTITION_INFORMATION;

typedef struct _DISK_GEOMETRY
{
  LARGE_INTEGER Cylinders;
  MEDIA_TYPE MediaType;
  DWORD TracksPerCylinder;
  DWORD SectorsPerTrack;
  DWORD BytesPerSector;
} DISK_GEOMETRY, *PDISK_GEOMETRY;

#ifndef __USE_W32API

typedef struct _DRIVE_LAYOUT_INFORMATION
{
  DWORD PartitionCount;
  DWORD Signature;
  PARTITION_INFORMATION PartitionEntry[1];
} DRIVE_LAYOUT_INFORMATION, *PDRIVE_LAYOUT_INFORMATION;

#endif /* !__USE_W32API */

#endif /* __INCLUDE_DISK_H */

/* EOF */
