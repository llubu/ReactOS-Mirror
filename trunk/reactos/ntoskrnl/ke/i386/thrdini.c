/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/i386/thread.c
 * PURPOSE:         i386 Thread Context Creation
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

typedef struct _KSWITCHFRAME
{
    PVOID ExceptionList;
    BOOLEAN ApcBypassDisable;
    PVOID RetAddr;
} KSWITCHFRAME, *PKSWITCHFRAME;

typedef struct _KSTART_FRAME
{
    PKSYSTEM_ROUTINE SystemRoutine;
    PKSTART_ROUTINE StartRoutine;
    PVOID StartContext;
    BOOLEAN UserThread;
} KSTART_FRAME, *PKSTART_FRAME;

typedef struct _KUINIT_FRAME
{
    KSWITCHFRAME CtxSwitchFrame;
    KSTART_FRAME StartFrame;
    KTRAP_FRAME TrapFrame;
    FX_SAVE_AREA FxSaveArea;
} KUINIT_FRAME, *PKUINIT_FRAME;

typedef struct _KKINIT_FRAME
{
    KSWITCHFRAME CtxSwitchFrame;
    KSTART_FRAME StartFrame;
    FX_SAVE_AREA FxSaveArea;
} KKINIT_FRAME, *PKKINIT_FRAME;

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
KiThreadStartup(VOID)
{
    PKTRAP_FRAME TrapFrame;
    PKSTART_FRAME StartFrame;
    PKUINIT_FRAME InitFrame;
    
    /* Get the start and trap frames */
    InitFrame = KeGetCurrentThread()->KernelStack;
    StartFrame = &InitFrame->StartFrame;
    TrapFrame = &InitFrame->TrapFrame;

    /* Lower to APC level */
    KfLowerIrql(APC_LEVEL);
    
    /* Call the system routine */
    StartFrame->SystemRoutine(StartFrame->StartRoutine, StartFrame->StartContext);
    
    /* If we returned, we better be a user thread */
    if (!StartFrame->UserThread) DbgBreakPoint();
    
    /* Exit to user-mode */
    KiServiceExit2(TrapFrame);
}

VOID
NTAPI
KiInitializeContextThread(IN PKTHREAD Thread,
                          IN PKSYSTEM_ROUTINE SystemRoutine,
                          IN PKSTART_ROUTINE StartRoutine,
                          IN PVOID StartContext,
                          IN PCONTEXT ContextPointer)
{
    PFX_SAVE_AREA FxSaveArea;
    PFXSAVE_FORMAT FxSaveFormat;
    PKSTART_FRAME StartFrame;
    PKSWITCHFRAME CtxSwitchFrame;
    PKTRAP_FRAME TrapFrame;
    CONTEXT LocalContext;
    PCONTEXT Context = NULL;
    ULONG ContextFlags;

    /* Check if this is a With-Context Thread */
    if (ContextPointer)
    {
        /* Set up the Initial Frame */
        PKUINIT_FRAME InitFrame;
        InitFrame = (PKUINIT_FRAME)((ULONG_PTR)Thread->InitialStack -
                                    sizeof(KUINIT_FRAME));

        /* Copy over the context we got */
        RtlCopyMemory(&LocalContext, ContextPointer, sizeof(CONTEXT));
        Context = &LocalContext;
        ContextFlags = CONTEXT_CONTROL;

        /* Zero out the trap frame and save area */
        RtlZeroMemory(&InitFrame->TrapFrame,
                      KTRAP_FRAME_LENGTH + sizeof(FX_SAVE_AREA));

        /* Setup the Fx Area */
        FxSaveArea = &InitFrame->FxSaveArea;

        /* Check if we support FXsr */
        if (KeI386FxsrPresent)
        {
            /* Get the FX Save Format Area */
            FxSaveFormat = (PFXSAVE_FORMAT)Context->ExtendedRegisters;

            /* Set an initial state */
            FxSaveFormat->ControlWord = 0x27F;
            FxSaveFormat->StatusWord = 0;
            FxSaveFormat->TagWord = 0;
            FxSaveFormat->ErrorOffset = 0;
            FxSaveFormat->ErrorSelector = 0;
            FxSaveFormat->DataOffset = 0;
            FxSaveFormat->DataSelector = 0;
            FxSaveFormat->MXCsr = 0x1F80;
        }
        else
        {
            /* Setup the regular save area */
            Context->FloatSave.ControlWord = 0x27F;
            Context->FloatSave.StatusWord = 0;
            Context->FloatSave.TagWord = -1;
            Context->FloatSave.ErrorOffset = 0;
            Context->FloatSave.ErrorSelector = 0;
            Context->FloatSave.DataOffset =0;
            Context->FloatSave.DataSelector = 0;
        }

        /* Check if the CPU has NPX */
        if (KeI386NpxPresent)
        {
            /* Set an intial NPX State */
            Context->FloatSave.Cr0NpxState = 0;
            FxSaveArea->Cr0NpxState = 0;
            FxSaveArea->NpxSavedCpu = 0;

            /* Now set the context flags depending on XMM support */
            ContextFlags |= (KeI386FxsrPresent) ? CONTEXT_EXTENDED_REGISTERS :
                                                  CONTEXT_FLOATING_POINT;

            /* Set the Thread's NPX State */
            Thread->NpxState = NPX_STATE_NOT_LOADED;
            Thread->DispatcherHeader.NpxIrql = PASSIVE_LEVEL;
        }
        else
        {
            /* We'll use emulation */
            FxSaveArea->Cr0NpxState = CR0_EM;
            Thread->NpxState = NPX_STATE_NOT_LOADED &~ CR0_MP;
        }

        /* Disable any debug registers */
        Context->ContextFlags &= ~CONTEXT_DEBUG_REGISTERS;

        /* Setup the Trap Frame */
        TrapFrame = &InitFrame->TrapFrame;

        /* Set up a trap frame from the context. */
        KeContextToTrapFrame(Context,
                             NULL,
                             TrapFrame,
                             Context->ContextFlags | ContextFlags,
                             UserMode);

        /* Set SS, DS, ES's RPL Mask properly */
        TrapFrame->HardwareSegSs |= RPL_MASK;
        TrapFrame->SegDs |= RPL_MASK;
        TrapFrame->SegEs |= RPL_MASK;
        TrapFrame->Dr7 = 0;

        /* Set the debug mark */
        TrapFrame->DbgArgMark = 0xBADB0D00;

        /* Set the previous mode as user */
        TrapFrame->PreviousPreviousMode = UserMode;

        /* Terminate the Exception Handler List */
        TrapFrame->ExceptionList = EXCEPTION_CHAIN_END;

        /* Setup the Stack for KiThreadStartup and Context Switching */
        StartFrame = &InitFrame->StartFrame;
        CtxSwitchFrame = &InitFrame->CtxSwitchFrame;

        /* Tell the thread it will run in User Mode */
        Thread->PreviousMode = UserMode;

        /* Tell KiThreadStartup of that too */
        StartFrame->UserThread = TRUE;
    }
    else
    {
        /* Set up the Initial Frame for the system thread */
        PKKINIT_FRAME InitFrame;
        InitFrame = (PKKINIT_FRAME)((ULONG_PTR)Thread->InitialStack -
                                    sizeof(KKINIT_FRAME));

        /* Setup the Fx Area */
        FxSaveArea = &InitFrame->FxSaveArea;
        RtlZeroMemory(FxSaveArea, sizeof(FX_SAVE_AREA));

        /* Check if we have Fxsr support */
        if (KeI386FxsrPresent)
        {
            /* Set the stub FX area */
            FxSaveArea->U.FxArea.ControlWord = 0x27F;
            FxSaveArea->U.FxArea.MXCsr = 0x1F80;
        }
        else
        {
            /* Set the stub FN area */
            FxSaveArea->U.FnArea.ControlWord = 0x27F;
            FxSaveArea->U.FnArea.TagWord = -1;
        }

        /* No NPX State */
        Thread->NpxState = NPX_STATE_NOT_LOADED;

        /* Setup the Stack for KiThreadStartup and Context Switching */
        StartFrame = &InitFrame->StartFrame;
        CtxSwitchFrame = &InitFrame->CtxSwitchFrame;

        /* Tell the thread it will run in Kernel Mode */
        Thread->PreviousMode = KernelMode;

        /* Tell KiThreadStartup of that too */
        StartFrame->UserThread = FALSE;
    }

    /* Now setup the remaining data for KiThreadStartup */
    StartFrame->StartContext = StartContext;
    StartFrame->StartRoutine = StartRoutine;
    StartFrame->SystemRoutine = SystemRoutine;

    /* And set up the Context Switch Frame */
    CtxSwitchFrame->RetAddr = KiThreadStartup;
    CtxSwitchFrame->ApcBypassDisable = TRUE;
    CtxSwitchFrame->ExceptionList = EXCEPTION_CHAIN_END;

    /* Save back the new value of the kernel stack. */
    Thread->KernelStack = (PVOID)CtxSwitchFrame;
}

VOID
FASTCALL
KiIdleLoop(VOID)
{
    PKPRCB Prcb = KeGetCurrentPrcb();
    PKTHREAD OldThread, NewThread;

    /* Initialize the idle loop: disable interrupts */
    _enable();
    __asm__("nop; nop");
    _disable();
    
    /* Now loop forever */
    while (TRUE)
    {
        /* Check for pending timers, pending DPCs, or pending ready threads */
        if ((Prcb->DpcData[0].DpcQueueDepth) ||
            (Prcb->TimerRequest) ||
            (Prcb->DeferredReadyListHead.Next))
        {
            /* Quiesce the DPC software interrupt */
            HalClearSoftwareInterrupt(DISPATCH_LEVEL);
        
            /* Handle it */
            KiRetireDpcList(Prcb);
        }
    
        /* Check if a new thread is scheduled for execution */
        if (Prcb->NextThread)
        {
            /* Enable interupts */
            _enable();
        
            /* Capture current thread data */
            OldThread = Prcb->CurrentThread;
            NewThread = Prcb->NextThread;
        
            /* Set new thread data */
            Prcb->NextThread = NULL;
            Prcb->CurrentThread = NewThread;
        
            /* The thread is now running */
            NewThread->State = Running;
        
            /* Switch away from the idle thread */
            KiSwapContext(OldThread, NewThread);
            
            /* We are back in the idle thread -- disable interrupts again */
            _enable();
            __asm__("nop");
            _disable();
        }
        else
        {
            /* Continue staying idle. Note the HAL returns with interrupts on */
            Prcb->PowerState.IdleFunction(&Prcb->PowerState);
        }
    }
}

/* EOF */
