/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/hal/x86/printk.c
 * PURPOSE:         Writing to the console 
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *             ??/??/??: Created
 *             05/06/98: Implemented BSOD
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/ntoskrnl.h>
#include <string.h>
#include <internal/string.h>
#include <internal/mmhal.h>
#include <internal/halio.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS ******************************************************************/

//#define BOCHS_DEBUGGING 1
//#define SERIAL_DEBUGGING
#define SERIAL_PORT 0x03f8
#define SERIAL_BAUD_RATE 19200
#define SERIAL_LINE_CONTROL (SR_LCR_CS8 | SR_LCR_ST1 | SR_LCR_PNO)


#define IDMAP_BASE         (0xd0000000)

/*
 * Return a linear address which can be used to access the physical memory
 * starting at x 
 */
extern inline unsigned int physical_to_linear(unsigned int x)
{
        return(x+IDMAP_BASE);
}

extern inline unsigned int linear_to_physical(unsigned int x)
{
        return(x-IDMAP_BASE);
}


#ifdef BOCHS_DEBUGGING
#define BOCHS_LOGGER_PORT (0x3ed)
#endif

#ifdef SERIAL_DEBUGGING
#define   SER_RBR   SERIAL_PORT + 0
#define   SER_THR   SERIAL_PORT + 0
#define   SER_DLL   SERIAL_PORT + 0
#define   SER_IER   SERIAL_PORT + 1
#define   SER_DLM   SERIAL_PORT + 1
#define   SER_IIR   SERIAL_PORT + 2
#define   SER_LCR   SERIAL_PORT + 3
#define     SR_LCR_CS5 0x00
#define     SR_LCR_CS6 0x01
#define     SR_LCR_CS7 0x02
#define     SR_LCR_CS8 0x03
#define     SR_LCR_ST1 0x00
#define     SR_LCR_ST2 0x04
#define     SR_LCR_PNO 0x00
#define     SR_LCR_POD 0x08
#define     SR_LCR_PEV 0x18
#define     SR_LCR_PMK 0x28
#define     SR_LCR_PSP 0x38
#define     SR_LCR_BRK 0x40
#define     SR_LCR_DLAB 0x80
#define   SER_MCR   SERIAL_PORT + 4
#define     SR_MCR_DTR 0x01
#define     SR_MCR_RTS 0x02
#define   SER_LSR   SERIAL_PORT + 5
#define     SR_LSR_TBE 0x20
#define   SER_MSR   SERIAL_PORT + 6
#endif

/*
 * PURPOSE: Current cursor position
 */
static unsigned int cursorx=0, cursory=0;
static unsigned int lines_seen = 0;
static unsigned char CharAttribute = 0x17;

//#define NR_ROWS     25
#define NR_ROWS     50
#define NR_COLUMNS  80
#define VIDMEM_BASE 0xb8000

/*
 * PURPOSE: Points to the base of text mode video memory
 */
static char* vidmem = (char *)(VIDMEM_BASE + IDMAP_BASE);

#define CRTC_COMMAND 0x3d4
#define CRTC_DATA 0x3d5
#define CRTC_CURLO 0x0f
#define CRTC_CURHI 0x0e

/*
 * PURPOSE: This flag is set to true if the system is in HAL managed 
 * console mode. This is initially true then once the graphics drivers
 * initialize, it is turned off, HAL console mode is reentered if a fatal
 * error occurs or on system shutdown
 */
static unsigned int in_hal_console = 1;

/* FUNCTIONS ***************************************************************/


void __putchar(char c);


void HalSwitchToBlueScreen(void)
/*
 * FUNCTION: Switches the monitor to text mode and writes a blue background
 * NOTE: This function is entirely self contained and can be used from any
 * graphics mode. 
 */
{
   /*
    * Sanity check
    */
   if (in_hal_console)
     {
	return;
     }

   /*
    * Reset the cursor position
    */
   cursorx=0;
   cursory=0;

   outb_p(CRTC_COMMAND, CRTC_CURLO);
   outb_p(CRTC_DATA, 0);
   outb_p(CRTC_COMMAND, CRTC_CURHI);
   outb_p(CRTC_DATA, 0);

   /*
    * This code section is taken from the sample routines by
    * Jeff Morgan (kinfira@hotmail.com)
    */
   
}


NTSTATUS STDCALL NtDisplayString(IN PUNICODE_STRING DisplayString)
{
//   DbgPrint("NtDisplayString(%w)\n",DisplayString->Buffer);
   DbgPrint("%w",DisplayString->Buffer);
   return(STATUS_SUCCESS);
}

void HalDisplayString(char* string)
/*
 * FUNCTION: Switches the screen to HAL console mode (BSOD) if not there
 * already and displays a string
 * ARGUMENT:
 *        string = ASCII string to display
 * NOTE: Use with care because there is no support for returning from BSOD
 * mode
 */
{
   char* str=string;

   if (!in_hal_console)
     {
	HalSwitchToBlueScreen();
     }

   while ((*str)!=0)
     {
	__putchar(*str);
	str++;
     }
}

void __putchar(char c)
/*
 * FUNCTION: Writes a character to the console and updates the cursor position
 * ARGUMENTS: 
 *         c = the character to write
 * NOTE: This function handles newlines as well
 */
{
   int offset;
   int i;
   
#ifdef BOCHS_DEBUGGING
   outb_p(BOCHS_LOGGER_PORT,c);
#endif

#ifdef SERIAL_DEBUGGING
   while ((inb_p(SER_LSR) & SR_LSR_TBE) == 0)
     ;
   outb_p(SER_THR, c);
#endif

   outb_p(CRTC_COMMAND, CRTC_CURHI);
   offset = inb_p(CRTC_DATA)<<8;
   outb_p(CRTC_COMMAND, CRTC_CURLO);
   offset += inb_p(CRTC_DATA);

   cursory = offset / NR_COLUMNS;
   cursorx = offset % NR_COLUMNS;

   switch(c)
     {
      case '\n':
	cursory++;
	cursorx = 0;
        lines_seen++;
	break;
	
      default:
	vidmem[(cursorx * 2) + (cursory * 80 * 2)] = c;
        vidmem[(cursorx * 2) + (cursory * 80 * 2) + 1] = CharAttribute;
	cursorx++;
	if (cursorx >= NR_COLUMNS)
	  {
	     cursory++;
             lines_seen++;
	     cursorx = 0;
	  }
     }
  
  if (cursory >= NR_ROWS)
    {
      unsigned short *LinePtr;

      memcpy(vidmem, 
             &vidmem[NR_COLUMNS * 2], 
             NR_COLUMNS * (NR_ROWS - 1) * 2);
      LinePtr = (unsigned short *) &vidmem[NR_COLUMNS * (NR_ROWS - 1) * 2];
      for (i = 0; i < NR_COLUMNS; i++)
        {
          LinePtr[i] = CharAttribute << 8;
        }
      cursory = NR_ROWS - 1;
    }
   
   /*
    * Set the cursor position
    */
   
   offset = (cursory * NR_COLUMNS) + cursorx;

   outb_p(CRTC_COMMAND, CRTC_CURLO);
   outb_p(CRTC_DATA, offset);
   outb_p(CRTC_COMMAND, CRTC_CURHI);
   offset >>= 8;
   outb_p(CRTC_DATA, offset);
}


ULONG DbgPrint(PCH Format, ...)
{
   char buffer[256];
   va_list ap;
   unsigned int eflags;

   /*
    * Because this is used by alomost every subsystem including irqs it
    * must be atomic. The following code sequence disables interrupts after
    * saving the previous state of the interrupt flag
    */
   __asm__("pushf\n\tpop %0\n\tcli\n\t"
	   : "=m" (eflags)
	   : /* */);

   /*
    * Process the format string into a fixed length buffer using the
    * standard C RTL function
    */
   va_start(ap,Format);
   vsprintf(buffer,Format,ap);
   va_end(ap);

   HalDisplayString (buffer);

   /*
    * Restore the interrupt flag
    */
   __asm__("push %0\n\tpopf\n\t"
	   :
	   : "m" (eflags));
   return(strlen(buffer));
}

void HalInitConsole(boot_param* bp)
/*
 * FUNCTION: Initalize the console
 * ARGUMENTS:
 *         bp = Parameters setup by the boot loader
 */
{

#ifdef SERIAL_DEBUGGING
   /*  turn on DTR and RTS  */
   outb_p(SER_MCR, SR_MCR_DTR | SR_MCR_RTS);
   /*  set baud rate, line control  */
   outb_p(SER_LCR, SERIAL_LINE_CONTROL | SR_LCR_DLAB);
   outb_p(SER_DLL, (115200 / SERIAL_BAUD_RATE) & 0xff);
   outb_p(SER_DLM, ((115200 / SERIAL_BAUD_RATE) >> 8) & 0xff);
   outb_p(SER_LCR, SERIAL_LINE_CONTROL);
#endif

   /* Set cursor position */

   cursorx=bp->cursorx;
   cursory=bp->cursory;

#if 0
   offset = (cursory * NR_COLUMNS) + cursorx;
   outb_p(CRTC_COMMAND, CRTC_CURLO);
   outb_p(CRTC_DATA, offset);
   outb_p(CRTC_COMMAND, CRTC_CURHI);
   offset >>= 8;
   outb_p(CRTC_DATA, offset);
#endif
}
