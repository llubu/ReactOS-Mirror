/*
 * ReactOS kernel
 * Copyright (C) 2000 David Welch <welch@cwcom.net>
 * Copyright (C) 1999 Gareth Owen <gaz@athene.co.uk>, Ramon von Handel
 * Copyright (C) 1991, 1992 Linus Torvalds
 * 
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING. If not, write
 * to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge,
 * MA 02139, USA.  
 *
 */
/* $Id$
 *
 * PROJECT:        ReactOS kernel
 * FILE:           ntoskrnl/hal/x86/udelay.c
 * PURPOSE:        Busy waiting
 * PROGRAMMER:     David Welch (david.welch@seh.ox.ac.uk)
 * UPDATE HISTORY:
 *                 06/11/99 Created
 */

/* INCLUDES ***************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ******************************************************************/

#define 	TMR_CTRL	0x43	/*	I/O for control		*/
#define		TMR_CNT0	0x40	/*	I/O for counter 0	*/
#define		TMR_CNT1	0x41	/*	I/O for counter 1	*/
#define		TMR_CNT2	0x42	/*	I/O for counter 2	*/

#define		TMR_SC0		0	/*	Select channel 0 	*/
#define		TMR_SC1		0x40	/*	Select channel 1 	*/
#define		TMR_SC2		0x80	/*	Select channel 2 	*/

#define		TMR_LOW		0x10	/*	RW low byte only 	*/
#define		TMR_HIGH	0x20	/*	RW high byte only 	*/
#define		TMR_BOTH	0x30	/*	RW both bytes 		*/

#define		TMR_MD0		0	/*	Mode 0 			*/
#define		TMR_MD1		0x2	/*	Mode 1 			*/
#define		TMR_MD2		0x4	/*	Mode 2 			*/
#define		TMR_MD3		0x6	/*	Mode 3 			*/
#define		TMR_MD4		0x8	/*	Mode 4 			*/
#define		TMR_MD5		0xA	/*	Mode 5 			*/

#define		TMR_BCD		1	/*	BCD mode 		*/

#define		TMR_LATCH	0	/*	Latch command 		*/

#define		TMR_READ	0xF0	/*    Read command 		*/
#define		TMR_CNT		0x20	/*    CNT bit  (Active low, subtract it) */
#define		TMR_STAT	0x10	/*    Status bit  (Active low, subtract it) */
#define		TMR_CH2		0x8	/*    Channel 2 bit 		*/
#define		TMR_CH1		0x4	/*    Channel 1 bit 		*/
#define		TMR_CH0		0x2	/*    Channel 0 bit 		*/

#define MILLISEC        10                     /* Number of millisec between interrupts */
#define HZ              (1000 / MILLISEC)      /* Number of interrupts per second */
#define CLOCK_TICK_RATE 1193182                /* Clock frequency of the timer chip */
#define LATCH           (CLOCK_TICK_RATE / HZ) /* Count to program into the timer chip */
#define PRECISION       8                      /* Number of bits to calibrate for delay loop */

static BOOLEAN UdelayCalibrated = FALSE;

/* FUNCTIONS **************************************************************/

/*
 * NOTE: This function MUST NOT be optimized by the compiler!
 * If it is, it obviously will not delay AT ALL, and the system
 * will appear completely frozen at boot since
 * HalpCalibrateStallExecution will never return.
 * There are three options to stop optimization:
 * 1. Use a volatile automatic variable. Making it delay quite a bit
 *    due to memory accesses, and keeping the code portable. However,
 *    as this involves memory access it depends on both the CPU cache,
 *    e.g. if the stack used is already in a cache line or not, and
 *    whether or not we're MP. If MP, another CPU could (probably would)
 *    also access RAM at the same time - making the delay imprecise.
 * 2. Use compiler-specific #pragma's to disable optimization.
 * 3. Use inline assembly, making it equally unportable as #2.
 * For supported compilers we use inline assembler. For the others,
 * portable plain C.
 */
VOID STDCALL __attribute__((noinline))
__KeStallExecutionProcessor(ULONG Loops)
{
  if (!Loops)
  {
    return;
  }
#if defined(__GNUC__)
  __asm__ __volatile__ (
    "mov %0, %%eax\n"
    "ROSL1: dec %%eax\n"
    "jnz ROSL1" : : "d" (Loops));

#elif defined(_MSC_VER)
  __asm mov eax, Loops
ROSL1:
  __asm dec eax
  __asm jnz ROSL1
#else
   volatile unsigned int target = Loops;
   unsigned int i;
   for (i=0; i<target;i++);
#endif
}

VOID STDCALL KeStallExecutionProcessor(ULONG Microseconds)
{
   PKIPCR Pcr = (PKIPCR)KeGetCurrentKPCR();

   if (Pcr->PrcbData.FeatureBits & X86_FEATURE_TSC)
   {
      LARGE_INTEGER EndCount, CurrentCount;
      Ki386RdTSC(EndCount);
      EndCount.QuadPart += Microseconds * (ULONGLONG)Pcr->PrcbData.MHz;
      do
      {
         Ki386RdTSC(CurrentCount);
      }
      while (CurrentCount.QuadPart < EndCount.QuadPart);
   }
   else
   {
      __KeStallExecutionProcessor((Pcr->StallScaleFactor*Microseconds)/1000);
   }
}

static ULONG Read8254Timer(VOID)
{
  ULONG Count;
  ULONG flags;

  /* save flags and disable interrupts */
  Ki386SaveFlags(flags);
  Ki386DisableInterrupts();

  WRITE_PORT_UCHAR((PUCHAR) TMR_CTRL, TMR_SC0 | TMR_LATCH);
  Count = READ_PORT_UCHAR((PUCHAR) TMR_CNT0);
  Count |= READ_PORT_UCHAR((PUCHAR) TMR_CNT0) << 8;

  /* restore flags */
  Ki386RestoreFlags(flags);

  return Count;
}


VOID WaitFor8254Wraparound(VOID)
{
  ULONG CurCount, PrevCount = ~0;
  LONG Delta;

  CurCount = Read8254Timer();

  do
    {
      PrevCount = CurCount;
      CurCount = Read8254Timer();
      Delta = CurCount - PrevCount;

      /*
       * This limit for delta seems arbitrary, but it isn't, it's
       * slightly above the level of error a buggy Mercury/Neptune
       * chipset timer can cause.
       */

    }
   while (Delta < 300);
}

VOID HalpCalibrateStallExecution(VOID)
{
  ULONG i;
  ULONG calib_bit;
  ULONG CurCount;
  PKIPCR Pcr;
  LARGE_INTEGER StartCount, EndCount;

  if (UdelayCalibrated)
    {
      return;
    }

  UdelayCalibrated = TRUE;
  Pcr = (PKIPCR)KeGetCurrentKPCR();

  /* Initialise timer interrupt with MILLISEC ms interval        */
  WRITE_PORT_UCHAR((PUCHAR) TMR_CTRL, TMR_SC0 | TMR_BOTH | TMR_MD2);  /* binary, mode 2, LSB/MSB, ch 0 */
  WRITE_PORT_UCHAR((PUCHAR) TMR_CNT0, LATCH & 0xff); /* LSB */
  WRITE_PORT_UCHAR((PUCHAR) TMR_CNT0, LATCH >> 8); /* MSB */

  if (Pcr->PrcbData.FeatureBits & X86_FEATURE_TSC)
  {
      
     WaitFor8254Wraparound();
     Ki386RdTSC(StartCount);

     WaitFor8254Wraparound();
     Ki386RdTSC(EndCount);

     Pcr->PrcbData.MHz = (ULONG)(EndCount.QuadPart - StartCount.QuadPart) / 10000;
     DPRINT("%luMHz\n", Pcr->PrcbData.MHz);
     return;

  }

  DbgPrint("Calibrating delay loop... [");

  /* Stage 1:  Coarse calibration					    */

  WaitFor8254Wraparound();

  Pcr->StallScaleFactor = 1;

  do
    {
      Pcr->StallScaleFactor <<= 1;		/* Next delay count to try  */

      WaitFor8254Wraparound();

      __KeStallExecutionProcessor(Pcr->StallScaleFactor);   /* Do the delay */

      CurCount = Read8254Timer();
    }
  while (CurCount > LATCH / 2);

  Pcr->StallScaleFactor >>= 1;		    /* Get bottom value for delay   */

  /* Stage 2:  Fine calibration						    */
  DbgPrint("delay_count: %d", Pcr->StallScaleFactor);

  calib_bit = Pcr->StallScaleFactor;	/* Which bit are we going to test   */

  for (i = 0; i < PRECISION; i++)
    {
      calib_bit >>= 1;				/* Next bit to calibrate    */
      if (!calib_bit)
	{
	  break;			/* If we have done all bits, stop   */
	}

      Pcr->StallScaleFactor |= calib_bit;   /* Set the bit in delay_count   */

      WaitFor8254Wraparound();

      __KeStallExecutionProcessor(Pcr->StallScaleFactor);   /* Do the delay */

      CurCount = Read8254Timer();
      if (CurCount <= LATCH / 2)	/* If a tick has passed, turn the   */
	{				/* calibrated bit back off	    */
	  Pcr->StallScaleFactor &= ~calib_bit;
	}
    }

  /* We're finished:  Do the finishing touches				    */

  Pcr->StallScaleFactor /= (MILLISEC / 2);  /* Calculate delay_count for 1ms */

  DbgPrint("]\n");
  DbgPrint("delay_count: %d\n", Pcr->StallScaleFactor);
  DbgPrint("CPU speed: %d\n", Pcr->StallScaleFactor / 250);
#if 0
  DbgPrint("About to start delay loop test\n");
  DbgPrint("Waiting for five minutes...");
  for (i = 0; i < (5*60*1000*20); i++)
    {
      KeStallExecutionProcessor(50);
    }
  DbgPrint("finished\n");
  for(;;);
#endif
}


VOID STDCALL
HalCalibratePerformanceCounter(ULONG Count)
{
   ULONG flags;

   /* save flags and disable interrupts */
   Ki386SaveFlags(flags);
   Ki386DisableInterrupts();

   __KeStallExecutionProcessor(Count);

   /* restore flags */
   Ki386RestoreFlags(flags);
}


LARGE_INTEGER STDCALL
KeQueryPerformanceCounter(PLARGE_INTEGER PerformanceFreq)
/*
 * FUNCTION: Queries the finest grained running count available in the system
 * ARGUMENTS:
 *         PerformanceFreq (OUT) = The routine stores the number of 
 *                                 performance counter ticks per second here
 * RETURNS: The number of performance counter ticks since boot
 */
{
  PKIPCR Pcr;
  LARGE_INTEGER Value;
  ULONG Flags;

  Ki386SaveFlags(Flags);
  Ki386DisableInterrupts();

  Pcr = (PKIPCR)KeGetCurrentKPCR();

  if (Pcr->PrcbData.FeatureBits & X86_FEATURE_TSC)
  {
     Ki386RestoreFlags(Flags);
     if (NULL != PerformanceFreq)
     {
        PerformanceFreq->QuadPart = Pcr->PrcbData.MHz * (ULONGLONG)1000000;   
     }
     Ki386RdTSC(Value);
  }
  else
  {
     LARGE_INTEGER TicksOld;
     LARGE_INTEGER TicksNew;
     ULONG CountsLeft;

     Ki386RestoreFlags(Flags);

     if (NULL != PerformanceFreq)
     {
        PerformanceFreq->QuadPart = CLOCK_TICK_RATE;
     }

     do
     {
        KeQueryTickCount(&TicksOld);
        CountsLeft = Read8254Timer();
        Value.QuadPart = TicksOld.QuadPart * LATCH + (LATCH - CountsLeft);
        KeQueryTickCount(&TicksNew);
     }
     while (TicksOld.QuadPart != TicksNew.QuadPart);
  }
  return Value;
}

/* EOF */
