//
//  IDE.H - defines and typedefs for the IDE Driver module.
//

#ifndef __IDE_H
#define __IDE_H

#ifdef __cplusplus
extern "C" {
#endif

#define  IDE_MAXIMUM_DEVICES    8

#define IDE_MAX_NAME_LENGTH     50
#define IDE_NT_ROOTDIR_NAME     "\\Device"
#define IDE_NT_DEVICE_NAME      "\\HardDrive"
#define IDE_NT_PARTITION_NAME   "\\Partition"
#define IDE_WIN32_DEVICE_NAME   "\\DosDevices\\IDE"
#define IDE_DRIVER_NAME         "IDEDRIVER"

#define  IDE_SECTOR_BUF_SZ         512
#define  IDE_MAX_SECTORS_PER_XFER  256
#define  IDE_MAX_RESET_RETRIES     10000
#define  IDE_MAX_WRITE_RETRIES     1000
#define  IDE_MAX_BUSY_RETRIES      100
#define  IDE_MAX_DRQ_RETRIES       1000

#define  IDE_REG_ALT_STATUS     0x0006
#define  IDE_REG_DEV_CNTRL      0x0006  /* device control register */
#define    IDE_DC_SRST            0x04  /* drive reset (both drives) */
#define    IDE_DC_nIEN            0x02  /* IRQ enable (active low) */
#define  IDE_REG_DRV_ADDR       0x0007
#define  IDE_REG_DATA_PORT      0x0000
#define  IDE_REG_ERROR          0x0001  /* error register */
#define    IDE_ER_AMNF            0x01  /* addr mark not found */
#define    IDE_ER_TK0NF           0x02  /* track 0 not found */
#define    IDE_ER_ABRT            0x04  /* command aborted */
#define    IDE_ER_MCR             0x08  /* media change requested */
#define    IDE_ER_IDNF            0x10  /* ID not found */
#define    IDE_ER_MC              0x20  /* Media changed */
#define    IDE_ER_UNC             0x40  /* Uncorrectable data error */
#define  IDE_REG_PRECOMP        0x0001
#define  IDE_REG_SECTOR_CNT     0x0002
#define  IDE_REG_SECTOR_NUM     0x0003
#define  IDE_REG_CYL_LOW        0x0004
#define  IDE_REG_CYL_HIGH       0x0005
#define  IDE_REG_DRV_HEAD       0x0006
#define    IDE_DH_FIXED           0xA0
#define    IDE_DH_LBA             0x40
#define    IDE_DH_HDMASK          0x0F
#define    IDE_DH_DRV0            0x00
#define    IDE_DH_DRV1            0x10
#define  IDE_REG_STATUS         0x0007
#define    IDE_SR_BUSY            0x80
#define    IDE_SR_DRDY            0x40
#define    IDE_SR_DRQ             0x08
#define    IDE_SR_ERR             0x01
#define  IDE_REG_COMMAND        0x0007
#define    IDE_CMD_READ           0x20
#define    IDE_CMD_READ_RETRY     0x21
#define    IDE_CMD_WRITE          0x30
#define    IDE_CMD_WRITE_RETRY    0x31
#define    IDE_CMD_IDENT_DRV      0xEC

//
//  Access macros for command registers
//  Each macro takes an address of the command port block, and data
//
#define IDEReadError(Address)                (inb_p((Address) + IDE_REG_ERROR))
#define IDEWritePrecomp(Address, Data)       (outb_p((Address) + IDE_REG_PRECOMP, (Data)))
#define IDEReadSectorCount(Address)          (inb_p((Address) + IDE_REG_SECTOR_CNT))
#define IDEWriteSectorCount(Address, Data)   (outb_p((Address) + IDE_REG_SECTOR_CNT, (Data)))
#define IDEReadSectorNum(Address)            (inb_p((Address) + IDE_REG_SECTOR_NUM))
#define IDEWriteSectorNum(Address, Data)     (outb_p((Address) + IDE_REG_SECTOR_NUM, (Data)))
#define IDEReadCylinderLow(Address)          (inb_p((Address) + IDE_REG_CYL_LOW))
#define IDEWriteCylinderLow(Address, Data)   (outb_p((Address) + IDE_REG_CYL_LOW, (Data)))
#define IDEReadCylinderHigh(Address)         (inb_p((Address) + IDE_REG_CYL_HIGH))
#define IDEWriteCylinderHigh(Address, Data)  (outb_p((Address) + IDE_REG_CYL_HIGH, (Data)))
#define IDEReadDriveHead(Address)            (inb_p((Address) + IDE_REG_DRV_HEAD))
#define IDEWriteDriveHead(Address, Data)     (outb_p((Address) + IDE_REG_DRV_HEAD, (Data)))
#define IDEReadStatus(Address)               (inb_p((Address) + IDE_REG_STATUS))
#define IDEWriteCommand(Address, Data)       (outb_p((Address) + IDE_REG_COMMAND, (Data)))
#define IDEReadBlock(Address, Buffer)        (insw((Address) + IDE_REG_DATA_PORT, (Buffer), IDE_SECTOR_BUF_SZ / 2))
#define IDEWriteBlock(Address, Buffer)        (outsw((Address) + IDE_REG_DATA_PORT, (Buffer), IDE_SECTOR_BUF_SZ / 2))

//
//  Access macros for control registers
//  Each macro takes an address of the control port blank and data
//
#define IDEWriteDriveControl(Address, Data)  (outb_p((Address) + IDE_REG_DEV_CNTRL, (Data)))

//    IDE_DEVICE_EXTENSION
//
//  DESCRIPTION:
//    Extension to be placed in each device object
//
//  ACCESS:
//    Allocated from NON-PAGED POOL
//    Available at any IRQL
//

typedef struct _IDE_DEVICE_EXTENSION {
  PDEVICE_OBJECT         DeviceObject;
  PCONTROLLER_OBJECT     ControllerObject;
  struct _IDE_DEVICE_EXTESION  *DiskExtension;
  int                    UnitNumber;
  BOOLEAN                LBASupported;
  BOOLEAN                DMASupported;
  int                    BytesPerSector;
  int                    LogicalHeads;
  int                    SectorsPerLogCyl;
  int                    SectorsPerLogTrk;
  int                    Offset;
  int                    Size;

  int                    Operation;
  ULONG                  BytesRequested;
  ULONG                  BytesToTransfer;
  ULONG                  BytesRemaining;
  ULONG                  StartingSector;
  int                    SectorsTransferred;
  BYTE                  *TargetAddress;

} IDE_DEVICE_EXTENSION, *PIDE_DEVICE_EXTENSION;

//    IDE_TIMER_STATES
//
//  DESCRIPTION:
//    An enumeration containing the states in the timer DFA
//

typedef enum _IDE_TIMER_STATES {
  IDETimerIdle
} IDE_TIMER_STATES;

//    IDE_CONTROLLER_EXTENSION
//
//  DESCRIPTION:
//    Driver-defined structure used to hold miscellaneous controller information.
//
//  ACCESS:
//    Allocated from NON-PAGED POOL
//    Available at any IRQL
//

typedef struct _IDE_CONTROLLER_EXTENSION {
  KSPIN_LOCK             SpinLock;
  int                    Number;
  int                    Vector;
  int                    CommandPortBase;
  int                    ControlPortBase;
  BOOLEAN                DMASupported;
  BOOLEAN                ControllerInterruptBug;
  PKINTERRUPT            Interrupt;

  BOOLEAN                OperationInProgress;
  BYTE                   DeviceStatus;
  PIDE_DEVICE_EXTENSION  DeviceForOperation;
  PIRP                   CurrentIrp;

  IDE_TIMER_STATES       TimerState;
  LONG                   TimerCount;
  PDEVICE_OBJECT         TimerDevice;

} IDE_CONTROLLER_EXTENSION, *PIDE_CONTROLLER_EXTENSION;

//    IDE_DRIVE_IDENTIFY

typedef struct _IDE_DRIVE_IDENTIFY {
  WORD  ConfigBits;          /*00*/
  WORD  LogicalCyls;         /*01*/
  WORD  Reserved02;          /*02*/
  WORD  LogicalHeads;        /*03*/
  WORD  BytesPerTrack;       /*04*/
  WORD  BytesPerSector;      /*05*/
  WORD  SectorsPerTrack;     /*06*/
  BYTE   InterSectorGap;      /*07*/
  BYTE   InterSectorGapSize;
  BYTE   Reserved08H;         /*08*/
  BYTE   BytesInPLO;
  WORD  VendorUniqueCnt;     /*09*/
  char   SerialNumber[20];    /*10*/
  WORD  ControllerType;      /*20*/
  WORD  BufferSize;          /*21*/
  WORD  ECCByteCnt;          /*22*/
  char   FirmwareRev[8];      /*23*/
  char   ModelNumber[40];     /*27*/
  WORD  RWMultImplemented;   /*47*/
  WORD  DWordIOSupported;    /*48*/
  WORD  LBADMASupported;     /*49*/
  WORD  Reserved50;          /*50*/
  WORD  MinPIOTransTime;     /*51*/
  WORD  MinDMATransTime;     /*52*/
  WORD  TMFieldsValid;       /*53*/
  WORD  TMCylinders;         /*54*/
  WORD  TMHeads;             /*55*/
  WORD  TMSectorsPerTrk;     /*56*/
  WORD  TMCapacity;          /*57*/
  WORD  Reserved53[198];     /*58*/
} IDE_DRIVE_IDENTIFY, *PIDE_DRIVE_IDENTIFY;


#ifdef __cplusplus
}
#endif

#endif  /*  __IDE_H  */


