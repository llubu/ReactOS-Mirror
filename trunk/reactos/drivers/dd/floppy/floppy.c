//
//  FLOPPY.C - NEC-765/8272A floppy device driver
//    written by Rex Jolliff
//    with help from various other sources, including but not limited to:
//    Art Baker's NT Device Driver Book, Linux Source, and the internet.
//
//  Modification History:
//    08/19/98  RJJ  Created.
//
//  To do:
// FIXME: get it working
// FIXME: add support for DMA hardware
// FIXME: should add support for floppy tape/zip devices

#include <ddk/ntddk.h>

#include "floppy.h"

#define  VERSION  "V0.0.1"

//  ---------------------------------------------------  File Statics

typedef struct _FLOPPY_CONTROLLER_PARAMETERS 
{
  int              PortBase;
  int              Vector;
  int              IrqL;
  int              SynchronizeIrqL;
  KINTERRUPT_MODE  InterruptMode;
  KAFFINITY        Affinity;
} FLOPPY_CONTROLLER_PARAMETERS, *PFLOPPY_CONTROLLER_PARAMETERS;

#define  FLOPPY_MAX_CONTROLLERS  2
FLOPPY_CONTROLLER_PARAMETERS ControllerParameters[FLOPPY_MAX_CONTROLLERS] = 
{
  {0x03f0, 6, 6, 6, LevelSensitive, 0xffff},
  {0x0370, 6, 6, 6, LevelSensitive, 0xffff},
};

FLOPPY_DEVICE_PARAMETERS DeviceTypes[] = 
{
    /* Unknown  */
  {0,  500, 16, 16, 8000, 1000, 3000,  0, 20, 5,  80, 3000, 20, {3,1,2,0,2}, 0, 0, { 7, 4, 8, 2, 1, 5, 3,10}, 1500, 0, "unknown"},
    /* 5 1/4 360 KB PC*/
  {1,  300, 16, 16, 8000, 1000, 3000,  0, 20, 5,  40, 3000, 17, {3,1,2,0,2}, 0, 0, { 1, 0, 0, 0, 0, 0, 0, 0}, 1500, 1, "360K PC"},
    /* 5 1/4 HD AT*/
  {2,  500, 16, 16, 6000,  400, 3000, 14, 20, 6,  83, 3000, 17, {3,1,2,0,2}, 0, 0, { 2, 5, 6,23,10,20,11, 0}, 1500, 2, "1.2M"},
    /* 3 1/2 DD*/
  {3,  250, 16, 16, 3000, 1000, 3000,  0, 20, 5,  83, 3000, 20, {3,1,2,0,2}, 0, 0, { 4,22,21,30, 3, 0, 0, 0}, 1500, 4, "720k"},
    /* 3 1/2 HD*/
  {4,  500, 16, 16, 4000,  400, 3000, 10, 20, 5,  83, 3000, 20, {3,1,2,0,2}, 0, 0, { 7, 4,25,22,31,21,29,11}, 1500, 7, "1.44M"},
    /* 3 1/2 ED*/
  {5, 1000, 15,  8, 3000,  400, 3000, 10, 20, 5,  83, 3000, 40, {3,1,2,0,2}, 0, 0, { 7, 8, 4,25,28,22,31,21}, 1500, 8, "2.88M AMI BIOS"},
    /* 3 1/2 ED*/
  {6, 1000, 15,  8, 3000,  400, 3000, 10, 20, 5,  83, 3000, 40, {3,1,2,0,2}, 0, 0, { 7, 8, 4,25,28,22,31,21}, 1500, 8, "2.88M"}
};

static BOOLEAN FloppyInitialized = FALSE;

//  ------------------------------------------------------  Functions

//    ModuleEntry
//
//  DESCRIPTION:
//    This function initializes the driver, locates and claims 
//    hardware resources, and creates various NT objects needed
//    to process I/O requests.
//
//  RUN LEVEL:
//    PASSIVE_LEVEL
//
//  ARGUMENTS:
//    IN  PDRIVER_OBJECT   DriverObject  System allocated Driver Object
//                                       for this driver
//    IN  PUNICODE_STRING  RegistryPath  Name of registry driver service 
//                                       key
//
//  RETURNS:
//    NTSTATUS  

NTSTATUS 
DriverEntry(IN PDRIVER_OBJECT DriverObject, 
            IN PUNICODE_STRING RegistryPath) 
{
  NTSTATUS  RC;
  PFLOPPY_DEVICE_EXTENSION  DeviceExtension;

  //  Export other driver entry points...
  DriverObject->DriverStartIo = FloppyStartIo;
  DriverObject->MajorFunction[IRP_MJ_CREATE] = FloppyDispatchOpenClose;
  DriverObject->MajorFunction[IRP_MJ_CLOSE] = FloppyDispatchOpenClose;
  DriverObject->MajorFunction[IRP_MJ_READ] = FloppyDispatchReadWrite;
  DriverObject->MajorFunction[IRP_MJ_WRITE] = FloppyDispatchReadWrite;
  DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = FloppyDispatchDeviceControl;

  //   Try to detect controller and abort if it fails
  if (!FloppyCreateController(DriverObject, 
                              &ControllerParameters[0],
                              0))
    {
      DPRINT("Could not find floppy controller");
      return STATUS_NO_SUCH_DEVICE;
    }

  return STATUS_SUCCESS;
}

static BOOLEAN
FloppyCreateController(PDRIVER_OBJECT DriverObject,
                       PFLOPPY_CONTROLLER_PARAMETERS ControllerParameters,
                       int Index)
{
  PFLOPPY_CONTROLLER_TYPE ControllerType;
  PCONTROLLER_OBJECT ControllerObject;
  PFLOPPY_CONTROLLER_EXTENSION ControllerExtension;

  /*  Detect controller and determine type  */
  if (!FloppyGetControllerVersion(ControllerParameters, &ControllerType))
    {
      return FALSE;
    }

  // FIXME: Register port ranges and interrupts with HAL

  /*  Create controller object for FDC  */
  ControllerObject = IoCreateController(sizeof(FLOPPY_CONTROLLER_EXTENSION));
  if (ControllerObject == NULL) 
    {
      DPRINT("Could not create controller object for controller %d\n",
             Index);
      return FALSE;
    }

  // FIXME: fill out controller data
  ControllerExtension = (PFLOPPY_CONTROLLER_EXTENSION)
      ControllerObject->ControllerExtension;
  ControllerExtension->Number = Index;
  ControllerExtension->PortBase = ControllerParameters->PortBase;
  ControllerExtension->Vector = ControllerParameters->Vector;
  ControllerExtension->FDCType = ControllerType;

  /*  Initialize the spin lock in the controller extension  */
  KeInitializeSpinLock(&ControllerExtension->SpinLock);

  /*  Register an interrupt handler for this controller  */
  RC = IoConnectInterrupt(&ControllerExtension->Interrupt,
                          FloppyIsr, 
                          ControllerExtension, 
                          &ControllerExtension->SpinLock, 
                          ControllerExtension->Vector, 
                          ControllerParameters->IrqL, 
                          ControllerParameters->SynchronizeIrqL, 
                          ControllerParameters->InterruptMode, 
                          FALSE, 
                          ControllerParameters->Affinity, 
                          FALSE);
  if (!NT_SUCCESS(RC)) 
    {
      DPRINT("Could not Connect Interrupt %d\n", ControllerExtension->Vector);
      IoDeleteController(ControllerObject);
      return FALSE;
    }

  // FIXME: setup DMA stuff for controller

  //  Check for each possible drive and create devices for them
  for (DriveIdx = 0; DriveIdx < FLOPPY_MAX_DRIVES; DriveIdx++)
    {
      // FIXME: try to identify the drive
      // FIXME: create a device if it's there
    }
    
  return  STATUS_SUCCESS;
}

//    FloppyGetControllerVersion
//
//  DESCRIPTION
//    Get the type/version of the floppy controller
//
//  RUN LEVEL:
//    PASSIVE_LEVEL
//
//  ARGUMENTS:
//    IN OUT  PFLOPPY_DEVICE_EXTENSION  DeviceExtension
//
//  RETURNS:
//    BOOL  success or failure
//
//  COMMENTS:
//    This routine (get_fdc_version) was originally written by David C. Niemi
//
static BOOLEAN  
FloppyGetControllerVersion(IN PFLOPPY_CONTROLLER_PARAMETERS ControllerParameters,
                           OUT PFLOPPY_CONTROLLER_TYPE ControllerType)
{
  BYTE ResultReturned
  BYTE Result[FLOPPY_MAX_REPLIES];
  int  ResultLength;

  /* 82072 and better know DUMPREGS */
  if (!FloppyWriteCommandByte(ControllerParameters->PortBase, 
                              FLOPPY_CMD_DUMP_FDC))
    {
      return FALSE;
    }
  ResultLength = FloppyReadResultCode(PortBase, &Result));
  if (ResultLength < 0)
    {
      return FALSE;
    }

  /* 8272a/765 don't know DUMPREGS */
  if ((ResultLength == 1) && (Result[0] == 0x80))
    {
      DPRINT("FDC %d is an 8272A\n", PortBase);
      *ControllerType = FDC_8272A;       
      return TRUE;
    }
  if (r != 10) 
    {
      DPRINT("FDC %d init: DUMP_FDC: unexpected return of %d bytes.\n",
             PortBase, 
             ResultLength);
      return FALSE;
    }

  if (!FloppyConfigure(PortBase, FALSE, 0x0a)) 
    {
      DPRINT("FDC %d is an 82072\n", PortBase);
      *ControllerType = FDC_82072;
      return TRUE;
    }
  FloppyWriteCommandByte(PortBase, FLOPPY_CMD_PPND_RW);
  if (FloppyNeedsMoreOutput(PortBase, Result) == FLOPPY_NEEDS_OUTPUT) 
    {
      FloppyWriteCommandByte(PortBase, 0);
    } 
  else 
    {
      DPRINT("FDC %d is an 82072A\n", PortBase);
      *ControllerType = FDC_82072A;
      return TRUE;
    }

  /* Pre-1991 82077, doesn't know LOCK/UNLOCK */
  FloppyWriteCommandByte(PortBase, FLOPPY_CMD_UNLK_FIFO);
  ResultLength = FloppyReadResultCode(PortBase, &Result));
  if ((ResultLength == 1) && (Result[0] == 0x80))
    {
      DPRINT("FDC %d is a pre-1991 82077\n", PortBase);
      *ControllerType = FDC_82077_ORIG;  
      return TRUE;
    }
  if ((ResultLength != 1) || (Result[0] != 0x00)) 
    {
      DPRINT("FDC %d init: UNLOCK: unexpected return of %d bytes.\n",
             PortBase, 
             ResultLength);
      return FALSE;
    }

  /* Revised 82077AA passes all the tests */
  FloppyWriteCommandByte(PortBase, FLOPPY_CMD_PARTID);
  ResultLength = FloppyReadResultCode(PortBase, &Result));
  if (ResultLength != 1) 
    {
      DPRINT("FDC %d init: PARTID: unexpected return of %d bytes.\n",
             PortBase, 
             ResultLength);
      return FALSE;
    }
  if (Result[0] == 0x80) 
    {
      DPRINT("FDC %d is a post-1991 82077\n", PortBase);
      *ControllerType = FDC_82077;
      return TRUE;
    }
  switch (Result[0] >> 5) 
    {
    case 0x0:
      /* Either a 82078-1 or a 82078SL running at 5Volt */
      DPRINT("FDC %d is an 82078.\n", PortBase);
      *ControllerType = FDC_82078;
      return TRUE;

    case 0x1:
      DPRINT("FDC %d is a 44pin 82078\n", PortBase);
      *ControllerType = FDC_82078;
      return TRUE;

    case 0x2:
      DPRINT("FDC %d is a S82078B\n", PortBase);
      *ControllerType = FDC_S82078B;
      return TRUE;

    case 0x3:
      DPRINT("FDC %d is a National Semiconductor PC87306\n", PortBase);
      *ControllerType = FDC_87306;
      return TRUE;

    default:
      DPRINT("FDC %d init: 82078 variant with unknown PARTID=%d.\n",
             PortBase, 
             Result[0] >> 5);
      *ControllerType = FDC_82078_UNKN;
      return TRUE;
    }
}

/* sends a command byte to the fdc */
static BOOLEAN 
FloppyWriteCommandByte(WORD PortBase, BYTE Byte)
{
  int Status;

  if ((Status = FloppyWaitUntilReady()) < 0)
    {
      return FALSE;
    }

  if ((Status & (STATUS_READY|STATUS_DIR|STATUS_DMA)) == STATUS_READY)
    {
      FloppyWriteData(PortBase, Byte);
      return 0;
    }

  if (FloppyInitialized) 
    {
      DPRINT("Unable to send byte %x to FDC. Fdc=%x Status=%x\n",
             Byte, 
             PortBase, 
             Status);
    }

  return FALSE;
}

/* gets the response from the fdc */
static int 
FloppyReadResultCode(WORD PortBase, PBYTE Result)
{
  int Replies;
  int Status;

  for (Replies = 0; Replies < FLOPPY_MAX_REPLIES; Replies++) 
    {
      if ((Status = FloppyWaitUntilReady(PortBase)) < 0)
        {
          break;
        }
      Status &= STATUS_DIR | STATUS_READY | STATUS_BUSY | STATUS_DMA;
      if ((Status & ~STATUS_BUSY) == STATUS_READY)
        {
          return Replies;
        }
      if (Status == (STATUS_DIR | STATUS_READY | STATUS_BUSY))
        {
          Result[Replies] = fd_inb(FD_DATA);
        }
      else
        {
          break;
        }
    }

  if (FloppyInitialized) 
    {
      DPRINT("get result error. Fdc=%d Last status=%x Read bytes=%d\n",
             PortBase, 
             Status, 
             Replies);
    }

  return -1;
}

/* waits until the fdc becomes ready */
static int 
FloppyWaitUntilReady(WORD PortBase)
{
  int Retries;
  int Status;

  for (Retries = 0; Retries < FLOPPY_MAX_STAT_RETRIES; Retries++) 
    {
      status = FloppyReadSTAT(PortBase);
      if (Status & STATUS_READY)
        {
          return Status;
        }
    }

  if (FloppyInitialized) 
    {
      DPRINT("Getstatus times out (%x) on fdc %d\n",
             Status, 
             PortBase);
    }

  return -1;
}


#define MORE_OUTPUT -2
/* does the fdc need more output? */
static int 
FloppyNeedsMoreOutput(WORD PortBase, PBYTE Result)
{
  int Status;

  if ((Status = FloppyWaitUntilReady(PortBase)) < 0)
    return -1;
  if ((Status & (STATUS_READY | STATUS_DIR | STATUS_DMA)) == STATUS_READY)
    {
      return FLOPPY_NEEDS_OUTPUT;
    }

  return FloppyReadResultCode(PortBase, Result);
}

static BOOLEAN
FloppyConfigure(WORD PortBase, BOOLEAN DisableFIFO, BYTE FIFODepth)
{
  BYTE Result[FLOPPY_MAX_REPLIES];

  /* Turn on FIFO */
  FloppyWriteCommandByte(FLOPPY_CMD_CFG_FIFO);
  if (FloppyNeedsOutput(PortBase, Result) != FLOPPY_NEEDS_OUTPUT)
    {
      return FALSE;
    }
  FloppyWriteCommandByte(PortBase, 0);
  FloppyWriteCommandByte(PortBase, 
                         0x10 | 
                           (DisableFIFO ? 0x20 : 0x00) | 
                           (FIFODepth & 0x0f));
  /* pre-compensation from track 0 upwards */
  FloppyWriteCommandByte(PortBase, 0); 

  return TRUE;
}       


