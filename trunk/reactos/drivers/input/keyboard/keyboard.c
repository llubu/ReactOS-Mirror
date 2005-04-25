/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/dd/keyboard/keyboard.c
 * PURPOSE:          Keyboard driver
 * PROGRAMMER:       Victor Kirhenshtein (sauros@iname.com)
 *                   Jason Filby (jasonfilby@yahoo.com)
 */

/* INCLUDES ****************************************************************/

#include <ddk/ntddk.h>
#include <string.h>
#include <ntos/keyboard.h>
#include <ntos/minmax.h>
#include <rosrtl/string.h>

#include <ddk/ntddkbd.h>
#include <ddk/ntdd8042.h>

#define NDEBUG
#include <debug.h>

#include "keyboard.h"

/* GLOBALS *******************************************************************/

/*
 * Driver data
 */

static KEY_EVENT_RECORD kbdBuffer[KBD_BUFFER_SIZE];
static int bufHead,bufTail;
static int keysInBuffer;
static int extKey;
static BYTE ledStatus;
static BYTE capsDown,numDown,scrollDown;
static DWORD ctrlKeyState;
static PKINTERRUPT KbdInterrupt;
static KDPC KbdDpc;
static PIO_WORKITEM KbdWorkItem = NULL;
static BOOLEAN AlreadyOpened = FALSE;

/*
 * PURPOSE: Current irp being processed
 */
static PIRP CurrentIrp;

/*
 * PURPOSE: Number of keys that have been read into the current irp's buffer
 */
static ULONG KeysRead;
static ULONG KeysRequired;

/*
 * Virtual key codes table
 *
 * Comments:
 *   * PrtSc = VK_PRINT
 *   * Alt+PrtSc (SysRq) = VK_EXECUTE
 *  * Alt = VK_MENU
 */

static const WORD vkTable[128]=
{
   /* 00 - 07 */ 0, VK_ESCAPE, VK_1, VK_2, VK_3, VK_4, VK_5, VK_6,
   /* 08 - 0F */ VK_7, VK_8, VK_9, VK_0, 189, 187, VK_BACK, VK_TAB,
   /* 10 - 17 */ VK_Q, VK_W, VK_E, VK_R, VK_T, VK_Y, VK_U, VK_I,
   /* 18 - 1F */ VK_O, VK_P, 219, 221, VK_RETURN, VK_CONTROL, VK_A, VK_S,
   /* 20 - 27 */ VK_D, VK_F, VK_G, VK_H, VK_J, VK_K, VK_L, 186,
   /* 28 - 2F */ 222, 192, VK_SHIFT, 220, VK_Z, VK_X, VK_C, VK_V,
   /* 30 - 37 */ VK_B, VK_N, VK_M, 188, 190, 191, VK_SHIFT, VK_MULTIPLY,
   /* 38 - 3F */ VK_MENU, VK_SPACE, VK_CAPITAL, VK_F1, VK_F2, VK_F3, VK_F4, VK_F5,
   /* 40 - 47 */ VK_F6, VK_F7, VK_F8, VK_F9, VK_F10, VK_NUMLOCK, VK_SCROLL, VK_HOME,
   /* 48 - 4F */ VK_UP, VK_PRIOR, VK_SUBTRACT, VK_LEFT, VK_CLEAR, VK_RIGHT, VK_ADD, VK_END,
   /* 50 - 57 */ VK_DOWN, VK_NEXT, VK_INSERT, VK_DELETE, VK_EXECUTE, 0, 0, VK_F11,
   /* 58 - 5F */ VK_F12, 0, 0, 91, 92, 93, 0, 0,
   /* 60 - 67 */ 0, 0, 0, 0, 0, 0, 0, 0,
   /* 68 - 6F */ 0, 0, 0, 0, 0, 0, 0, 0,
   /* 70 - 77 */ 0, 0, 0, 0, 0, 0, 0, 0,
   /* 78 - 7F */ 0, 0, 0, 0, 0, 0, 0, VK_PAUSE
};
static const WORD vkKeypadTable[13]=	/* 47 - 53 */
{
   VK_NUMPAD7, VK_NUMPAD8, VK_NUMPAD9, VK_SUBTRACT,
   VK_NUMPAD4, VK_NUMPAD5, VK_NUMPAD6, VK_ADD,
   VK_NUMPAD1, VK_NUMPAD2, VK_NUMPAD3, VK_NUMPAD0, VK_DECIMAL
};


/*
 * ASCII translation tables
 */

static const BYTE asciiTable1[10]=
{
   ')','!','@','#','$','%','^','&','*','('
};
static const BYTE asciiTable2[16]=
{
   '0','1','2','3','4','5','6','7','8','9','*','+',0,'-','.','/'
};
static const BYTE asciiTable3[37]=
{
   ';','=',',','-','.','/','`', 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   '[', '\\', ']', '\''
};
static const BYTE asciiTable4[37]=
{
   ':','+','<','_','>','?','~', 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   '{', '|', '}', '"'
};

VOID STDCALL
KdpServiceDispatcher(ULONG Code, PVOID Context1, PVOID Context2);

static LONG DoSystemDebug = -1;
static BOOLEAN InSysRq = FALSE;

/* FUNCTIONS *****************************************************************/

static void KbdWrite(int addr,BYTE data)
/*
 * FUNCTION: Write data to keyboard
 */
{
   BYTE status;

   do
   {
      status=READ_PORT_UCHAR((PUCHAR)KBD_CTRL_PORT); // Wait until input buffer empty
   } while(status & KBD_IBF);
   WRITE_PORT_UCHAR((PUCHAR)addr,data);
}

static int KbdReadData(void)
/*
 * FUNCTION: Read data from port 0x60
 */
{
   int i;
   BYTE status,data;

   i=500000;
   do
   {
      status=READ_PORT_UCHAR((PUCHAR)KBD_CTRL_PORT);
      if (!(status & KBD_OBF))      // Check if data available
         continue;
      data=READ_PORT_UCHAR((PUCHAR)KBD_DATA_PORT);
      if (status & (KBD_GTO | KBD_PERR))  // Check for timeout error
         continue;
      return data;
   } while(--i);
   return -1;     // Timed out
}


/*
 * Set keyboard LED's
 */

static void SetKeyboardLEDs(BYTE status)
{
   KbdWrite(KBD_DATA_PORT,0xED);
   if (KbdReadData()!=KBD_ACK)      // Error
      return;
   KbdWrite(KBD_DATA_PORT,status);
   KbdReadData();
}


/*
 * Process scan code
 */

static void ProcessScanCode(BYTE scanCode,BOOL isDown)
{
   switch(scanCode)
   {
      case 0x1D:  // Ctrl
         if (extKey)
         {
            if (isDown)
               ctrlKeyState|=RIGHT_CTRL_PRESSED;
            else
               ctrlKeyState&=~RIGHT_CTRL_PRESSED;
         }
         else
         {
            if (isDown)
               ctrlKeyState|=LEFT_CTRL_PRESSED;
            else
               ctrlKeyState&=~LEFT_CTRL_PRESSED;
         }
         break;
      case 0x2A:  // Left shift
      case 0x36:  // Right shift
         if (isDown)
            ctrlKeyState|=SHIFT_PRESSED;
         else
            ctrlKeyState&=~SHIFT_PRESSED;
         break;
      case 0x38:  // Alt
         if (extKey)
         {
            if (isDown)
               ctrlKeyState|=RIGHT_ALT_PRESSED;
            else
               ctrlKeyState&=~RIGHT_ALT_PRESSED;
         }
         else
         {
            if (isDown)
               ctrlKeyState|=LEFT_ALT_PRESSED;
            else
               ctrlKeyState&=~LEFT_ALT_PRESSED;
         }
         break;
      case 0x3A:  // CapsLock
         if (ctrlKeyState & CTRL_PRESSED)
            break;
         if (isDown)
         {
            if (!capsDown)
            {
               capsDown=1;
               if (ctrlKeyState & CAPSLOCK_ON)
               {
                  ledStatus&=~KBD_LED_CAPS;
                  ctrlKeyState&=~CAPSLOCK_ON;
               }
               else
               {
                  ledStatus|=KBD_LED_CAPS;
                  ctrlKeyState|=CAPSLOCK_ON;
               }
               SetKeyboardLEDs(ledStatus);
            }
         }
         else
         {
            capsDown=0;
         }
         break;
      case 0x45:  // NumLock
         if (ctrlKeyState & CTRL_PRESSED)
            break;
         if (isDown)
         {
            if (!numDown)
            {
               numDown=1;
               if (ctrlKeyState & NUMLOCK_ON)
               {
                  ledStatus&=~KBD_LED_NUM;
                  ctrlKeyState&=~NUMLOCK_ON;
               }
               else
               {
                  ledStatus|=KBD_LED_NUM;
                  ctrlKeyState|=NUMLOCK_ON;
               }
               SetKeyboardLEDs(ledStatus);
            }
         }
         else
         {
            numDown=0;
         }
         break;
      case 0x46:  // ScrollLock
         if (ctrlKeyState & CTRL_PRESSED)
            break;
         if (isDown)
         {
            if (!scrollDown)
            {
               scrollDown=1;
               if (ctrlKeyState & SCROLLLOCK_ON)
               {
                  ledStatus&=~KBD_LED_SCROLL;
                  ctrlKeyState&=~SCROLLLOCK_ON;
               }
               else
               {
                  ledStatus|=KBD_LED_SCROLL;
                  ctrlKeyState|=SCROLLLOCK_ON;
               }
               SetKeyboardLEDs(ledStatus);
            }
         }
         else
         {
            scrollDown=0;
         }
         break;
      default:
         break;
   }
}


/*
 * Translate virtual key code to ASCII
 */

static BYTE VirtualToAscii(WORD keyCode,BOOL isDown)
{
   if ((ctrlKeyState & ALT_PRESSED)&&(ctrlKeyState & CTRL_PRESSED))
      return 0;		// Ctrl+Alt+char always 0
   if ((!isDown)&&(ctrlKeyState & ALT_PRESSED))
      return 0;		// Alt+char is 0 when key is released

   if (ctrlKeyState & CTRL_PRESSED)
   {
      if ((keyCode>=VK_A)&&(keyCode<=VK_Z))
         return keyCode-VK_A+1;
      switch(keyCode)
      {
         case VK_SPACE:
            return ' ';
         case VK_BACK:
            return 127;
         case VK_RETURN:
            return '\r';                     
         case 219:                      /* [ */
            if (ctrlKeyState & SHIFT_PRESSED)
               return 0;
            return 27;
         case 220:         /* \ */
            if (ctrlKeyState & SHIFT_PRESSED)
               return 0;
            return 28;
         case 221:         /* ] */
            if (ctrlKeyState & SHIFT_PRESSED)
               return 0;
             return 29;
         default:
            return 0;
      }
   }

   if ((keyCode>=VK_A)&&(keyCode<=VK_Z))
   {
      if (ctrlKeyState & CAPSLOCK_ON)
         if (ctrlKeyState & SHIFT_PRESSED)
            return keyCode-VK_A+'a';
         else
            return keyCode-VK_A+'A';
      else
         if (ctrlKeyState & SHIFT_PRESSED)
            return keyCode-VK_A+'A';
         else
            return keyCode-VK_A+'a';
   }

   if ((keyCode>=VK_0)&&(keyCode<=VK_9))
   {
      if (ctrlKeyState & SHIFT_PRESSED)
         return asciiTable1[keyCode-VK_0];
      else
         return keyCode-VK_0+'0';
   }

   if ((keyCode>=VK_NUMPAD0)&&(keyCode<=VK_DIVIDE))
      return asciiTable2[keyCode-VK_NUMPAD0];

   if ((keyCode>=186)&&(keyCode<=222))
  {
      if (ctrlKeyState & SHIFT_PRESSED)
         return asciiTable4[keyCode-186];
      else
         return asciiTable3[keyCode-186];
   }

   switch(keyCode)
   {
      case VK_SPACE:
         return ' ';
      case VK_RETURN:
         return '\r';
      case VK_BACK:
         return 8;
      case VK_TAB:
         return 9;
   }
   return 0;
}


/*
 * Translate scan code to virtual key code
 */

static WORD ScanToVirtual(BYTE scanCode)
{
   if ((scanCode>=0x47)&&(scanCode<=0x53)&&(ctrlKeyState & NUMLOCK_ON)&&
       (!extKey)&&(!(ctrlKeyState & SHIFT_PRESSED)))
      return vkKeypadTable[scanCode-0x47];
   if ((scanCode==0x35)&&(extKey))		// Gray divide
      return VK_DIVIDE;
   if ((scanCode==0x37)&&(extKey))     // Print screen
      return VK_PRINT;
   return vkTable[scanCode];
}


/*
 * Debug request handler
 */

static VOID STDCALL
KbdWorkItemRoutine(IN PDEVICE_OBJECT DeviceObject,
                   IN PVOID Context)
{
   LONG Debug;

   Debug = InterlockedExchange(&DoSystemDebug, -1);
   if (Debug != -1)
     {
       KdpServiceDispatcher(TAG('R', 'o', 's', ' '), (PVOID)Debug, NULL);
     }
}


/*
 * Keyboard IRQ handler
 */

static VOID STDCALL
KbdDpcRoutine(PKDPC Dpc,
	      PVOID DeferredContext,
	      PVOID SystemArgument1,
	      PVOID SystemArgument2)
{
   PIRP Irp = (PIRP)SystemArgument2;
   PDEVICE_OBJECT DeviceObject = (PDEVICE_OBJECT)SystemArgument1;

   if (SystemArgument1 == NULL && DoSystemDebug != -1)
     {
       if (KbdWorkItem != NULL && DoSystemDebug == 10) /* 10 is Tab + K (enter kernel debugger) */
         {
           IoQueueWorkItem(KbdWorkItem, (PIO_WORKITEM_ROUTINE)KbdWorkItemRoutine, DelayedWorkQueue, NULL);
         }
       else
         {
           KdpServiceDispatcher(TAG('R', 'o', 's', ' '), (PVOID)DoSystemDebug, NULL);
           DoSystemDebug = -1;
         }
       return;
     }
     
   CHECKPOINT;
   DPRINT("KbdDpcRoutine(DeviceObject %x, Irp %x)\n",
	    DeviceObject,Irp);
   Irp->IoStatus.Status = STATUS_SUCCESS;
   Irp->IoStatus.Information = 0;
   IoCompleteRequest(Irp,IO_NO_INCREMENT);
   IoStartNextPacket(DeviceObject,FALSE);
}


static BOOLEAN STDCALL
KeyboardHandler(PKINTERRUPT Interrupt,
		PVOID Context)
{
   BYTE thisKey;
   BOOL isDown;
   static BYTE lastKey;
   CHAR Status;
   PDEVICE_OBJECT deviceObject = (PDEVICE_OBJECT) Context;
   PDEVICE_EXTENSION deviceExtension = (PDEVICE_EXTENSION) deviceObject->DeviceExtension;
   		
   CHECKPOINT;

   /*
    * Check status
    */
   Status = READ_PORT_UCHAR((PUCHAR)KBD_CTRL_PORT);
   if (!(Status & KBD_OBF))
     {
       return (FALSE);
     }

   // Read scan code
   thisKey=READ_PORT_UCHAR((PUCHAR)KBD_DATA_PORT);

	// Call hook routine. May change scancode value.
   if (deviceExtension->IsrHookCallback) {
   	BOOLEAN cont = FALSE, ret;
	//BUG BUG: rewrite to have valid CurrentScanState!!!
	ret = (*deviceExtension->IsrHookCallback)(
                	deviceObject,
                	NULL,//&deviceExtension->CurrentInput,
                	NULL,//&deviceExtension->CurrentOutput,
                	Status,
                	&thisKey, //&scanCode,
                	&cont,
                	NULL //&deviceExtension->CurrentScanState
                	);

    	if (!cont) {
        	return ret;
    	}
	}       


   if ((thisKey==0xE0)||(thisKey==0xE1))   // Extended key
   {
      extKey=1;         // Wait for next byte
      lastKey=thisKey;
      return FALSE;
   }

   isDown=!(thisKey & 0x80);
   thisKey&=0x7F;

   // The keyboard maintains its own internal caps lock and num lock
   // statuses.  In caps lock mode E0 AA precedes make code and
   // E0 2A follow break code.  In num lock mode, E0 2A precedes
   // make code and E0 AA follow break code.  We maintain our own caps lock
   // and num lock statuses, so we will just ignore these.
   // Some keyboards have L-Shift/R-Shift modes instead of caps lock
   // mode.  If right shift pressed, E0 B6 / E0 36 pairs generated.
   if (extKey & ((thisKey==0x2A)||(thisKey==0x36)))
   {
      extKey=0;
      return FALSE;
   }

   // Check for PAUSE sequence
   if (extKey && (lastKey==0xE1))
   {
      if (thisKey==0x1D)
         lastKey=0xFF;     // Sequence is OK
      else
         extKey=0;
      return FALSE;
   }
   if (extKey && (lastKey==0xFF))
   {
      if (thisKey!=0x45)
      {
         extKey=0;         // Bad sequence
         return FALSE;
      }
      thisKey=0x7F;        // Pseudo-code for PAUSE
   }

   ProcessScanCode(thisKey,isDown);

//   DbgPrint("Key: %c\n",VirtualToAscii(ScanToVirtual(thisKey),isDown));
//   DbgPrint("Key: %x\n",ScanToVirtual(thisKey));
   if (ScanToVirtual(thisKey) == VK_TAB && isDown)
     {
       InSysRq = TRUE;
     }
   else if (ScanToVirtual(thisKey) == VK_TAB && !isDown)
     {
       InSysRq = FALSE;
     }
   else if (InSysRq == TRUE && ScanToVirtual(thisKey) >= VK_A &&
	    ScanToVirtual(thisKey) <= VK_Z && isDown)
     {
       InterlockedExchange(&DoSystemDebug, ScanToVirtual(thisKey) - VK_A);
       KeInsertQueueDpc(&KbdDpc, NULL, NULL);
       return(TRUE);
     }

   if (CurrentIrp!=NULL)
     {
	KEY_EVENT_RECORD* rec = (KEY_EVENT_RECORD *)
	  CurrentIrp->AssociatedIrp.SystemBuffer;
	
	CHECKPOINT;
	
	rec[KeysRead].bKeyDown=isDown;
	rec[KeysRead].wRepeatCount=1;
	rec[KeysRead].wVirtualKeyCode=ScanToVirtual(thisKey);
	rec[KeysRead].wVirtualScanCode=thisKey;
        rec[KeysRead].uChar.AsciiChar=VirtualToAscii(rec->wVirtualKeyCode,isDown);
	rec[KeysRead].dwControlKeyState=ctrlKeyState;
	if (extKey)
	  {
	     rec[KeysRead].dwControlKeyState|=ENHANCED_KEY;
	     extKey = 0;
	  }
	KeysRead++;
	DPRINT("KeysRequired %d KeysRead %x\n",KeysRequired,KeysRead);
	if (KeysRead==KeysRequired)
	  {
	  	 PDEVICE_OBJECT DeviceObject = (PDEVICE_OBJECT) Context;
	     KeInsertQueueDpc(&KbdDpc,DeviceObject,CurrentIrp);
	     CurrentIrp=NULL;
	  }
	CHECKPOINT;
	return TRUE;
     }
   
   // Buffer is full ?
   if (keysInBuffer==KBD_BUFFER_SIZE)      // Buffer is full
   {
      extKey=0;
      return(TRUE);
   }
   kbdBuffer[bufHead].bKeyDown=isDown;
   kbdBuffer[bufHead].wRepeatCount=1;
   kbdBuffer[bufHead].wVirtualKeyCode=ScanToVirtual(thisKey);
   kbdBuffer[bufHead].wVirtualScanCode=thisKey;
   kbdBuffer[bufHead].uChar.UnicodeChar=0;
   // kbdBuffer[bufHead].uChar.AsciiChar=TranslateScanCode(thisKey);
   kbdBuffer[bufHead].uChar.AsciiChar=VirtualToAscii(kbdBuffer[bufHead].wVirtualKeyCode,isDown);
   kbdBuffer[bufHead].dwControlKeyState=ctrlKeyState;
   if (extKey) 
      kbdBuffer[bufHead].dwControlKeyState|=ENHANCED_KEY;
   bufHead++;
   bufHead&=KBD_WRAP_MASK;    // Modulo KBD_BUFFER_SIZE
   keysInBuffer++;
   extKey=0;
   return TRUE;
}


//
// Initialize keyboard
//
static void KeyboardConnectInterrupt(PDEVICE_OBJECT DeviceObject)
{
   ULONG MappedIrq;
   KIRQL Dirql;
   KAFFINITY Affinity;
   NTSTATUS Status;
   
   MappedIrq = HalGetInterruptVector(Internal,
				     0,
				     0,
				     KEYBOARD_IRQ,
				     &Dirql,
				     &Affinity);
   Status = IoConnectInterrupt(&KbdInterrupt,
			       KeyboardHandler,
			       (PVOID)DeviceObject,
			       NULL,
			       MappedIrq,
			       Dirql,
			       Dirql,
			       0,
			       FALSE,
			       Affinity,
			       FALSE);
}

VOID
KbdClearInput(VOID)
{
  ULONG i;
  CHAR Status;

  for (i = 0; i < 100; i++)
    {
      Status = READ_PORT_UCHAR((PUCHAR)KBD_CTRL_PORT);
      if (!(Status & KBD_OBF))
	{
	  return;
	}
      (VOID)READ_PORT_UCHAR((PUCHAR)KBD_DATA_PORT);
    }
}

static int InitializeKeyboard(PDEVICE_OBJECT DeviceObject)
{
   // Initialize variables
   bufHead=0;
   bufTail=0;
   keysInBuffer=0;
   ledStatus=0;
   capsDown=0;
   numDown=0;
   scrollDown=0;
   ctrlKeyState=0;
   extKey=0;

   KbdClearInput();
   KeyboardConnectInterrupt(DeviceObject);
   KeInitializeDpc(&KbdDpc,KbdDpcRoutine,NULL);
   KbdWorkItem = IoAllocateWorkItem(DeviceObject);
   if (KbdWorkItem == NULL)
     {
        DPRINT("Warning: Couldn't allocate work item!\n");
     }
   return 0;
}

/*
 * Read data from keyboard buffer
 */
BOOLEAN STDCALL
KbdSynchronizeRoutine(PVOID Context)
{
   PIRP Irp = (PIRP)Context;
   KEY_EVENT_RECORD* rec = (KEY_EVENT_RECORD *)Irp->AssociatedIrp.SystemBuffer;
   PIO_STACK_LOCATION stk = IoGetCurrentIrpStackLocation(Irp);
   ULONG NrToRead = stk->Parameters.Read.Length/sizeof(KEY_EVENT_RECORD);
   int i;

   DPRINT("NrToRead %d keysInBuffer %d\n",NrToRead,keysInBuffer);
   NrToRead = min(NrToRead,keysInBuffer);

   DPRINT("NrToRead %d stk->Parameters.Read.Length %d\n",
	  NrToRead,stk->Parameters.Read.Length);
   DPRINT("sizeof(KEY_EVENT_RECORD) %d\n",sizeof(KEY_EVENT_RECORD));
   for (i=0;i<NrToRead;i++)
     {
	memcpy(&rec[i],&kbdBuffer[bufTail],sizeof(KEY_EVENT_RECORD));
	bufTail++;
	bufTail&=KBD_WRAP_MASK;
	keysInBuffer--;
    }
   if ((stk->Parameters.Read.Length/sizeof(KEY_EVENT_RECORD))==NrToRead)
     {
	return(TRUE);
     }

   KeysRequired=stk->Parameters.Read.Length/sizeof(KEY_EVENT_RECORD);
   KeysRead=NrToRead;
   CurrentIrp=Irp;

   return(FALSE);
}

VOID STDCALL KbdStartIo(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
#ifndef NDEBUG
   PIO_STACK_LOCATION stk = IoGetCurrentIrpStackLocation(Irp);
#endif

   DPRINT("KeyboardStartIo(DeviceObject %x Irp %x)\n",DeviceObject,Irp);
   
   if (KeSynchronizeExecution(KbdInterrupt, KbdSynchronizeRoutine, Irp))
     {
        KIRQL oldIrql;
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	oldIrql = KeGetCurrentIrql();
        if (oldIrql < DISPATCH_LEVEL)
          {
            KeRaiseIrql (DISPATCH_LEVEL, &oldIrql);
            IoStartNextPacket (DeviceObject, FALSE);
            KeLowerIrql(oldIrql);
	  }
        else
          {
            IoStartNextPacket (DeviceObject, FALSE);
	  }
     }
   
   DPRINT("stk->Parameters.Read.Length %d\n",stk->Parameters.Read.Length);
   DPRINT("KeysRequired %d\n",KeysRequired);     
}

NTSTATUS STDCALL KbdInternalDeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    PIO_STACK_LOCATION                  stk;
	PINTERNAL_I8042_HOOK_KEYBOARD      	hookKeyboard;
    PDEVICE_EXTENSION					DevExt = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
	NTSTATUS                    		status = STATUS_INVALID_DEVICE_REQUEST;

	Irp->IoStatus.Information = 0;
    stk = IoGetCurrentIrpStackLocation(Irp);
    
	switch (stk->Parameters.DeviceIoControl.IoControlCode) 
	{
		/*-----------------11/29/2001 4:12PM----------------
		 * This internal ioctrl belongs in i8042 driver. Should be
		 * moved to the appropriate driver later.
		 * --------------------------------------------------*/
    	case IOCTL_INTERNAL_I8042_HOOK_KEYBOARD:
        	
			if (stk->Parameters.DeviceIoControl.InputBufferLength < sizeof(INTERNAL_I8042_HOOK_KEYBOARD)) 
			{
				DPRINT(("Keyboard IOCTL_INTERNAL_I8042_HOOK_KEYBOARD invalid buffer size\n"));
            	status = STATUS_INVALID_PARAMETER;
        	}
        	else {
            	//
            	// Copy the values if they are filled in
            	//
            	hookKeyboard = (PINTERNAL_I8042_HOOK_KEYBOARD)
                	stk->Parameters.DeviceIoControl.Type3InputBuffer;

            	DevExt->HookContext = hookKeyboard->Context;
            	if (hookKeyboard->InitializationRoutine) {
					DbgPrint("Keyboard: InitializationHookCallback NOT IMPLEMENTED\n");
                	DevExt->InitializationHookCallback =
                    	hookKeyboard->InitializationRoutine;
            	}
    
            	if (hookKeyboard->IsrRoutine) {
                	DevExt->IsrHookCallback = hookKeyboard->IsrRoutine;
            	}
    
            	status = STATUS_SUCCESS;
        	}
        	break;
		default:
        	status = STATUS_INVALID_DEVICE_REQUEST;
			break;
	}
    
	Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}			   

NTSTATUS STDCALL KbdDispatch(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
   PIO_STACK_LOCATION stk = IoGetCurrentIrpStackLocation(Irp);
   NTSTATUS Status;

   DPRINT("DeviceObject %x\n",DeviceObject);
   DPRINT("Irp %x\n",Irp);
   
   DPRINT("IRP_MJ_CREATE %d stk->MajorFunction %d\n",
	  IRP_MJ_CREATE, stk->MajorFunction);
   DPRINT("AlreadyOpened %d\n",AlreadyOpened);
   
   switch (stk->MajorFunction)
     {
      case IRP_MJ_CREATE:
	if (AlreadyOpened == TRUE)
	  {
	     CHECKPOINT;
//	     Status = STATUS_UNSUCCESSFUL;
	     Status = STATUS_SUCCESS;
	  }
	else
	  {
	     CHECKPOINT;
	     Status = STATUS_SUCCESS;
	     AlreadyOpened = TRUE;
	  }
	break;
	
      case IRP_MJ_CLOSE:
        Status = STATUS_SUCCESS;
	break;

      case IRP_MJ_READ:
        DPRINT("Handling Read request\n");
	DPRINT("Queueing packet\n");
	IoMarkIrpPending(Irp);
	IoStartPacket(DeviceObject,Irp,NULL,NULL);
	return(STATUS_PENDING);

      default:
        Status = STATUS_NOT_IMPLEMENTED;
	break;
     }

   Irp->IoStatus.Status = Status;
   Irp->IoStatus.Information = 0;
   IoCompleteRequest(Irp,IO_NO_INCREMENT);
   DPRINT("Status %d\n",Status);
   return(Status);
}

NTSTATUS STDCALL DriverEntry(PDRIVER_OBJECT DriverObject, 
			     PUNICODE_STRING RegistryPath)
/*
 * FUNCTION: Module entry point
 */
{
   PDEVICE_OBJECT DeviceObject;
   UNICODE_STRING DeviceName = ROS_STRING_INITIALIZER(L"\\Device\\Keyboard");
   UNICODE_STRING SymlinkName = ROS_STRING_INITIALIZER(L"\\??\\Keyboard");
   
   DPRINT("Keyboard Driver 0.0.4\n");

   DriverObject->MajorFunction[IRP_MJ_CREATE] = KbdDispatch;
   DriverObject->MajorFunction[IRP_MJ_CLOSE] = KbdDispatch;
   DriverObject->MajorFunction[IRP_MJ_READ] = KbdDispatch;
   DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = 
         KbdInternalDeviceControl;  

   DriverObject->DriverStartIo = KbdStartIo;

   IoCreateDevice(DriverObject,
		  sizeof(DEVICE_EXTENSION),
		  &DeviceName,
		  FILE_DEVICE_KEYBOARD,
		  0,
		  TRUE,
		  &DeviceObject);
	
   RtlZeroMemory(DeviceObject->DeviceExtension, sizeof(DEVICE_EXTENSION));

   DeviceObject->Flags = DeviceObject->Flags | DO_BUFFERED_IO;
   InitializeKeyboard( DeviceObject );
   
   IoCreateSymbolicLink(&SymlinkName, &DeviceName);
   
   return(STATUS_SUCCESS);
}
