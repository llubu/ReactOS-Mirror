/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * PROJECT:              ReactOS kernel
 * FILE:                 ntoskrnl/ke/i386/thread.c
 * PURPOSE:              Architecture multitasking functions
 * PROGRAMMER:           David Welch (welch@cwcom.net)
 * REVISION HISTORY:
 *             27/06/98: Created
 */

/* INCLUDES ****************************************************************/

#include <ddk/ntddk.h>
#include <internal/ntoskrnl.h>
#include <internal/ps.h>
#include <internal/i386/segment.h>
#include <internal/i386/mm.h>
#include <internal/ke.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS **************************************************************/

#define FLAG_NT (1<<14)
#define FLAG_VM (1<<17)
#define FLAG_IF (1<<9)
#define FLAG_IOPL ((1<<12)+(1<<13))

NTSTATUS 
KeValidateUserContext(PCONTEXT Context)
/*
 * FUNCTION: Validates a processor context
 * ARGUMENTS:
 *        Context = Context to validate
 * RETURNS: Status
 * NOTE: This only validates the context as not violating system security, it
 * doesn't guararantee the thread won't crash at some point
 * NOTE2: This relies on there only being two selectors which can access 
 * system space
 */
{
   if (Context->Eip >= KERNEL_BASE)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   if (Context->SegCs == KERNEL_CS)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   if (Context->SegDs == KERNEL_DS)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   if (Context->SegEs == KERNEL_DS)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   if (Context->SegFs == KERNEL_DS)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   if (Context->SegGs == KERNEL_DS)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   if ((Context->EFlags & FLAG_IOPL) != 0 ||
       (Context->EFlags & FLAG_NT) ||
       (Context->EFlags & FLAG_VM) ||
       (!(Context->EFlags & FLAG_IF)))
     {
        return(STATUS_UNSUCCESSFUL);
     }
   return(STATUS_SUCCESS);
}

NTSTATUS
Ke386InitThreadWithContext(PKTHREAD Thread, PCONTEXT Context)
{
   PULONG KernelStack;

  /*
   * Setup a stack frame for exit from the task switching routine
   */
  
  KernelStack = (PULONG)(Thread->KernelStack - ((4 * 5) + sizeof(CONTEXT)));
  /* FIXME: Add initial floating point information */
  /* FIXME: Add initial debugging information */
  KernelStack[0] = 0;      /* EDI */
  KernelStack[1] = 0;      /* ESI */
  KernelStack[2] = 0;      /* EBX */
  KernelStack[3] = 0;      /* EBP */
  KernelStack[4] = (ULONG)PsBeginThreadWithContextInternal;   /* EIP */
  memcpy((PVOID)&KernelStack[5], (PVOID)Context, sizeof(CONTEXT));
  Thread->KernelStack = (PVOID)KernelStack;

  return(STATUS_SUCCESS);
}

NTSTATUS
Ke386InitThread(PKTHREAD Thread, 
		PKSTART_ROUTINE StartRoutine, 
		PVOID StartContext)
     /*
      * Initialize a thread
      */
{
  PULONG KernelStack;

  /*
   * Setup a stack frame for exit from the task switching routine
   */
  
  KernelStack = (PULONG)(Thread->KernelStack - (8*4));
  /* FIXME: Add initial floating point information */
  /* FIXME: Add initial debugging information */
  KernelStack[0] = 0;      /* EDI */
  KernelStack[1] = 0;      /* ESI */
  KernelStack[2] = 0;      /* EBX */
  KernelStack[3] = 0;      /* EBP */
  KernelStack[4] = (ULONG)PsBeginThread;   /* EIP */
  KernelStack[5] = 0;     /* Return EIP */
  KernelStack[6] = (ULONG)StartRoutine; /* First argument to PsBeginThread */
  KernelStack[7] = (ULONG)StartContext; /* Second argument to PsBeginThread */
  Thread->KernelStack = (VOID*)KernelStack;

  return(STATUS_SUCCESS);
}

/* EOF */
