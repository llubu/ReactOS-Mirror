/* $Id: continue.c,v 1.5 2004/08/15 16:39:11 chorns Exp $
 *
 * COPYRIGHT:              See COPYING in the top level directory
 * PROJECT:                ReactOS kernel
 * FILE:                   ntoskrnl/ps/i386/continue.c
 * PURPOSE:                i386 implementation of NtContinue()
 * PROGRAMMER:             Royce Mitchell III, kjk_hyperion
 * REVISION HISTORY:
 *               29/06/04: Created
 */

/* INCLUDES ****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

VOID
FASTCALL
KeRosTrapReturn ( PKTRAP_FRAME TrapFrame, PKTRAP_FRAME PrevTrapFrame );

VOID STDCALL
KeRosDumpStackFrames ( PULONG Frame, ULONG FrameCount );

/*
 * @implemented
 */
NTSTATUS STDCALL
NtContinue (
	IN PCONTEXT Context,
	IN BOOLEAN TestAlert)
{
	PKTRAP_FRAME TrapFrame = KeGetCurrentThread()->TrapFrame;
	PKTRAP_FRAME PrevTrapFrame = (PKTRAP_FRAME)TrapFrame->Edx;

	DPRINT("NtContinue: Context: Eip=0x%x, Esp=0x%x\n", Context->Eip, Context->Esp );
	PULONG Frame = 0;
	__asm__("mov %%ebp, %%ebx" : "=b" (Frame) : );
	DPRINT( "NtContinue(): Ebp=%x, prev/TF=%x/%x\n", Frame, Frame[0], TrapFrame );
#ifndef NDEBUG
	KeRosDumpStackFrames(NULL,5);
#endif

	if ( Context == NULL )
	{
		DPRINT1("NtContinue called with NULL Context\n");
		return STATUS_INVALID_PARAMETER;
	}

	if ( TrapFrame == NULL )
	{
		CPRINT("NtContinue called but TrapFrame was NULL\n");
		KEBUGCHECK(0);
	}

	/*
	* Copy the supplied context over the register information that was saved
	* on entry to kernel mode, it will then be restored on exit
	* FIXME: Validate the context
	*/
	KeContextToTrapFrame ( Context, TrapFrame );

	KeRosTrapReturn ( TrapFrame, PrevTrapFrame );

	return STATUS_SUCCESS; /* this doesn't actually happen b/c KeRosTrapReturn() won't return */
}
