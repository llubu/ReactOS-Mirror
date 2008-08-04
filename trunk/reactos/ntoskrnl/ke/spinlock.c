/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/spinlock.c
 * PURPOSE:         Spinlock and Queued Spinlock Support
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

#define LQ_WAIT     1
#define LQ_OWN      2

/* PRIVATE FUNCTIONS *********************************************************/

VOID
FASTCALL
KeAcquireQueuedSpinLockAtDpcLevel(IN PKSPIN_LOCK_QUEUE LockHandle)
{
#ifdef CONFIG_SMP
    PKSPIN_LOCK_QUEUE Prev;

    /* Set the new lock */
    Prev = (PKSPIN_LOCK_QUEUE)
           InterlockedExchange((PLONG)LockHandle->Next,
                               (LONG)LockHandle);
    if (!Prev)
    {
        /* There was nothing there before. We now own it */
         *LockHandle->Lock |= LQ_OWN;
        return;
    }

    /* Set the wait flag */
     *LockHandle->Lock |= LQ_WAIT;

    /* Link us */
    Prev->Next = (PKSPIN_LOCK_QUEUE)LockHandle;

    /* Loop and wait */
    while (*LockHandle->Lock & LQ_WAIT)
        YieldProcessor();
#endif
}

VOID
FASTCALL
KeReleaseQueuedSpinLockFromDpcLevel(IN PKSPIN_LOCK_QUEUE LockHandle)
{
#ifdef CONFIG_SMP
    KSPIN_LOCK LockVal;
    PKSPIN_LOCK_QUEUE Waiter;

    /* Remove own and wait flags */
    *LockHandle->Lock &= ~(LQ_OWN | LQ_WAIT);
    LockVal = *LockHandle->Lock;

    /* Check if we already own it */
    if (LockVal == (KSPIN_LOCK)LockHandle)
    {
        /* Disown it */
        LockVal = (KSPIN_LOCK)
                  InterlockedCompareExchangePointer(LockHandle->Lock,
                                                    NULL,
                                                    LockHandle);
    }
    if (LockVal == (KSPIN_LOCK)LockHandle) return;

    /* Need to wait for it */
    Waiter = LockHandle->Next;
    while (!Waiter)
    {
        YieldProcessor();
        Waiter = LockHandle->Next;
    }

    /* It's gone */
    *(ULONG_PTR*)&Waiter->Lock ^= (LQ_OWN | LQ_WAIT);
    LockHandle->Next = NULL;
#endif
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
VOID
NTAPI
KeInitializeSpinLock(IN PKSPIN_LOCK SpinLock)
{
    /* Clear it */
    *SpinLock = 0;
}

/*
 * @implemented
 */
#undef KeAcquireSpinLockAtDpcLevel
VOID
NTAPI
KeAcquireSpinLockAtDpcLevel(IN PKSPIN_LOCK SpinLock)
{
    /* Do the inlined function */
    KxAcquireSpinLock(SpinLock);
}

/*
 * @implemented
 */
#undef KeReleaseSpinLockFromDpcLevel
VOID
NTAPI
KeReleaseSpinLockFromDpcLevel(IN PKSPIN_LOCK SpinLock)
{
    /* Do the lined function */
    KxReleaseSpinLock(SpinLock);
}

/*
 * @implemented
 */
VOID
FASTCALL
KefAcquireSpinLockAtDpcLevel(IN PKSPIN_LOCK SpinLock)
{
    /* Do the inlined function */
    KxAcquireSpinLock(SpinLock);
}

/*
 * @implemented
 */
VOID
FASTCALL
KefReleaseSpinLockFromDpcLevel(IN PKSPIN_LOCK SpinLock)
{
    /* Do the lined function */
    KxReleaseSpinLock(SpinLock);
}

/*
 * @implemented
 */
VOID
FASTCALL
KiAcquireSpinLock(IN PKSPIN_LOCK SpinLock)
{
    /* Do the inlined function */
    KxAcquireSpinLock(SpinLock);
}

/*
 * @implemented
 */
VOID
FASTCALL
KiReleaseSpinLock(IN PKSPIN_LOCK SpinLock)
{
    /* Do the lined function */
    KxReleaseSpinLock(SpinLock);
}

/*
 * @implemented
 */
BOOLEAN
FASTCALL
KeTryToAcquireSpinLockAtDpcLevel(IN OUT PKSPIN_LOCK SpinLock)
{
#ifdef CONFIG_SMP
    /* Check if it's already acquired */
    if (!(*SpinLock))
    {
        /* Try to acquire it */
        if (InterlockedBitTestAndSet((PLONG)SpinLock, 0))
        {
            /* Someone else acquired it */
            return FALSE;
        }
    }
    else
    {
        /* It was already acquired */
        return FALSE;
    }

#ifdef DBG
    /* On debug builds, we OR in the KTHREAD */
    *SpinLock = (ULONG_PTR)KeGetCurrentThread() | 1;
#endif
#endif

    /* All is well, return TRUE */
    return TRUE;
}

/*
 * @implemented
 */
VOID
FASTCALL
KeAcquireInStackQueuedSpinLockAtDpcLevel(IN PKSPIN_LOCK SpinLock,
                                         IN PKLOCK_QUEUE_HANDLE LockHandle)
{
#ifdef CONFIG_SMP
    /* Set it up properly */
    LockHandle->LockQueue.Next = NULL;
    LockHandle->LockQueue.Lock = SpinLock;
    KeAcquireQueuedSpinLockAtDpcLevel(LockHandle->LockQueue.Next);
#endif
}

/*
 * @implemented
 */
VOID
FASTCALL
KeReleaseInStackQueuedSpinLockFromDpcLevel(IN PKLOCK_QUEUE_HANDLE LockHandle)
{
#ifdef CONFIG_SMP
    /* Call the internal function */
    KeReleaseQueuedSpinLockFromDpcLevel(LockHandle->LockQueue.Next);
#endif
}

/*
 * @implemented
 */
KIRQL
NTAPI
KeAcquireInterruptSpinLock(IN PKINTERRUPT Interrupt)
{
    KIRQL OldIrql;

    /* Raise IRQL */
    KeRaiseIrql(Interrupt->SynchronizeIrql, &OldIrql);

    /* Acquire spinlock on MP */
    KefAcquireSpinLockAtDpcLevel(Interrupt->ActualLock);
    return OldIrql;
}

/*
 * @implemented
 */
VOID
NTAPI
KeReleaseInterruptSpinLock(IN PKINTERRUPT Interrupt,
                           IN KIRQL OldIrql)
{
    /* Release lock on MP */
    KefReleaseSpinLockFromDpcLevel(Interrupt->ActualLock);

    /* Lower IRQL */
    KeLowerIrql(OldIrql);
}

/*
 * @unimplemented
 */
KIRQL
FASTCALL
KeAcquireSpinLockForDpc(IN PKSPIN_LOCK SpinLock)
{
    UNIMPLEMENTED;
    return 0;
}

/*
 * @unimplemented
 */
VOID
FASTCALL
KeReleaseSpinLockForDpc(IN PKSPIN_LOCK SpinLock,
                        IN KIRQL OldIrql)
{
    UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
KIRQL
FASTCALL
KeAcquireInStackQueuedSpinLockForDpc(IN PKSPIN_LOCK SpinLock,
                                     IN PKLOCK_QUEUE_HANDLE LockHandle)
{
    UNIMPLEMENTED;
    return 0;
}

/*
 * @unimplemented
 */
VOID
FASTCALL
KeReleaseInStackQueuedSpinLockForDpc(IN PKLOCK_QUEUE_HANDLE LockHandle)
{
    UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
BOOLEAN
FASTCALL
KeTestSpinLock(IN PKSPIN_LOCK SpinLock)
{
    UNIMPLEMENTED;
    return FALSE;
}

/* EOF */
