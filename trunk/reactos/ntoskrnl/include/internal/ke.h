/*
 *  ReactOS kernel
 *  Copyright (C) 2000 David Welch <welch@cwcom.net>
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

#ifndef __NTOSKRNL_INCLUDE_INTERNAL_KE_H
#define __NTOSKRNL_INCLUDE_INTERNAL_KE_H

/* INCLUDES *****************************************************************/

#ifndef __ASM__
#include <ddk/ntifs.h>
#include <stdarg.h>
#endif /* not __ASM__ */

#include "arch/ke.h"

/* INTERNAL KERNEL FUNCTIONS ************************************************/

#ifdef __USE_W32API
struct _KPROCESS* KeGetCurrentProcess(VOID);
VOID KeSetGdtSelector(ULONG Entry, ULONG Value1, ULONG Value2);
#endif

#ifndef __ASM__

struct _KTHREAD;
struct _KIRQ_TRAPFRAME;
struct _KPCR;
struct _KEXCEPTION_FRAME;

VOID STDCALL 
DbgBreakPointNoBugCheck(VOID);

VOID
STDCALL
KeProfileInterrupt(
    PKTRAP_FRAME TrapFrame
);

VOID
STDCALL
KeProfileInterruptWithSource(
	IN PKTRAP_FRAME   		TrapFrame,
	IN KPROFILE_SOURCE		Source
);

BOOLEAN
STDCALL
KiIpiServiceRoutine(IN PKTRAP_FRAME TrapFrame,
	            IN struct _KEXCEPTION_FRAME* ExceptionFrame);

VOID STDCALL KeUpdateSystemTime(PKTRAP_FRAME TrapFrame, KIRQL Irql);
VOID STDCALL KeUpdateRunTime(PKTRAP_FRAME TrapFrame, KIRQL Irql);

KIRQL KeAcquireDispatcherDatabaseLock(VOID);
VOID KeAcquireDispatcherDatabaseLockAtDpcLevel(VOID);
VOID KeReleaseDispatcherDatabaseLock(KIRQL Irql);
VOID KeReleaseDispatcherDatabaseLockFromDpcLevel(VOID);

BOOLEAN KeDispatcherObjectWake(DISPATCHER_HEADER* hdr);
VOID STDCALL KeExpireTimers(PKDPC Apc,
			    PVOID Arg1,
			    PVOID Arg2,
			    PVOID Arg3);
VOID KeInitializeDispatcherHeader(DISPATCHER_HEADER* Header, ULONG Type,
				  ULONG Size, ULONG SignalState);
VOID KeDumpStackFrames(PULONG Frame);
BOOLEAN KiTestAlert(VOID);
VOID KeRemoveAllWaitsThread(struct _ETHREAD* Thread, NTSTATUS WaitStatus, BOOL Unblock);
PULONG KeGetStackTopThread(struct _ETHREAD* Thread);
VOID KeContextToTrapFrame(PCONTEXT Context, PKTRAP_FRAME TrapFrame);
VOID STDCALL KiDeliverApc(KPROCESSOR_MODE PreviousMode,
                  PVOID Reserved,
                  PKTRAP_FRAME TrapFrame);
		  
VOID KiInitializeUserApc(IN PVOID Reserved,
			 IN PKTRAP_FRAME TrapFrame,
			 IN PKNORMAL_ROUTINE NormalRoutine,
			 IN PVOID NormalContext,
			 IN PVOID SystemArgument1,
			 IN PVOID SystemArgument2);

VOID STDCALL KiAttachProcess(struct _KTHREAD *Thread, struct _KPROCESS *Process, KIRQL ApcLock, struct _KAPC_STATE *SavedApcState);

VOID STDCALL KiSwapProcess(struct _KPROCESS *NewProcess, struct _KPROCESS *OldProcess);

BOOLEAN
STDCALL
KeTestAlertThread(IN KPROCESSOR_MODE AlertMode);

BOOLEAN STDCALL KeRemoveQueueApc (PKAPC Apc);
PLIST_ENTRY STDCALL KeRundownQueue(IN PKQUEUE Queue);

extern LARGE_INTEGER SystemBootTime;

/* INITIALIZATION FUNCTIONS *************************************************/

VOID KeInitExceptions(VOID);
VOID KeInitInterrupts(VOID);
VOID KeInitTimer(VOID);
VOID KeInitDpc(struct _KPCR* Pcr);
VOID KeInitDispatcher(VOID);
VOID KeInitializeDispatcher(VOID);
VOID KeInitializeTimerImpl(VOID);
VOID KeInitializeBugCheck(VOID);
VOID Phase1Initialization(PVOID Context);

VOID KeInit1(PCHAR CommandLine, PULONG LastKernelAddress);
VOID KeInit2(VOID);

BOOLEAN KiDeliverUserApc(PKTRAP_FRAME TrapFrame);

VOID
STDCALL
KiMoveApcState (PKAPC_STATE OldState,
		PKAPC_STATE NewState);

VOID
KiAddProfileEvent(KPROFILE_SOURCE Source, ULONG Pc);
VOID 
KiDispatchException(PEXCEPTION_RECORD ExceptionRecord,
		    PCONTEXT Context,
		    PKTRAP_FRAME Tf,
		    KPROCESSOR_MODE PreviousMode,
		    BOOLEAN SearchFrames);
VOID KeTrapFrameToContext(PKTRAP_FRAME TrapFrame,
			  PCONTEXT Context);
VOID
KeApplicationProcessorInit(VOID);
VOID
KePrepareForApplicationProcessorInit(ULONG id);
ULONG
KiUserTrapHandler(PKTRAP_FRAME Tf, ULONG ExceptionNr, PVOID Cr2);
VOID STDCALL
KePushAndStackSwitchAndSysRet(ULONG Push, PVOID NewStack);
VOID STDCALL
KeStackSwitchAndRet(PVOID NewStack);
VOID STDCALL
KeBugCheckWithTf(ULONG BugCheckCode,
		 ULONG BugCheckParameter1,
		 ULONG BugCheckParameter2,
		 ULONG BugCheckParameter3,
		 ULONG BugCheckParameter4,
		 PKTRAP_FRAME Tf);
#define KEBUGCHECKWITHTF(a,b,c,d,e,f) DbgPrint("KeBugCheckWithTf at %s:%i\n",__FILE__,__LINE__), KeBugCheckWithTf(a,b,c,d,e,f)
VOID
KiDumpTrapFrame(PKTRAP_FRAME Tf, ULONG ExceptionNr, ULONG cr2);

VOID
STDCALL
KeFlushCurrentTb(VOID);

#endif /* not __ASM__ */

#define MAXIMUM_PROCESSORS      32

#endif /* __NTOSKRNL_INCLUDE_INTERNAL_KE_H */
