/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/gate.c
 * PURPOSE:         Implements Guarded Mutex
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Filip Navara (navaraf@reactos.org)
 */

/* INCLUDES ******************************************************************/

#define NTDDI_VERSION NTDDI_WS03SP1
#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* PRIVATE FUNCTIONS *********************************************************/

VOID
FASTCALL
KiAcquireGuardedMutexContented(IN OUT PKGUARDED_MUTEX GuardedMutex)
{
    ULONG BitsToRemove, BitsToAdd;
    LONG OldValue, NewValue;

    /* Increase the contention count */
    GuardedMutex->Contention++;

    /* Start by unlocking the Guarded Mutex */
    BitsToRemove = GM_LOCK_BIT;
    BitsToAdd = GM_LOCK_WAITER_INC;

    /* Get the Count Bits */
    OldValue = GuardedMutex->Count;

    /* Start change loop */
    for (;;)
    {
        /* Loop sanity checks */
        ASSERT((BitsToRemove == GM_LOCK_BIT) ||
               (BitsToRemove == (GM_LOCK_BIT | GM_LOCK_WAITER_WOKEN)));
        ASSERT((BitsToAdd == GM_LOCK_WAITER_INC) ||
               (BitsToAdd == GM_LOCK_WAITER_WOKEN));

        /* Check if the Guarded Mutex is locked */
        if (OldValue & GM_LOCK_BIT)
        {
            /* Sanity check */
            ASSERT((BitsToRemove == GM_LOCK_BIT) ||
                   ((OldValue & GM_LOCK_WAITER_WOKEN) != 0));

            /* Unlock it by removing the Lock Bit */
            NewValue = InterlockedCompareExchange(&GuardedMutex->Count,
                                                  OldValue ^ BitsToRemove,
                                                  OldValue);
            if (NewValue == OldValue) break;

            /* Value got changed behind our backs, start over */
            OldValue = NewValue;
        }
        else
        {
            /* The Guarded Mutex isn't locked, so simply set the bits */
            NewValue = InterlockedCompareExchange(&GuardedMutex->Count,
                                                  OldValue + BitsToAdd,
                                                  OldValue);
            if (NewValue != OldValue)
            {
                /* Value got changed behind our backs, start over */
                OldValue = NewValue;
                continue;
            }

            /* Now we have to wait for it */
            KeWaitForGate(&GuardedMutex->Gate, WrGuardedMutex, KernelMode);
            ASSERT((GuardedMutex->Count & GM_LOCK_WAITER_WOKEN) != 0);

            /* Ok, the wait is done, so set the new bits */
            BitsToRemove = GM_LOCK_BIT | GM_LOCK_WAITER_WOKEN;
            BitsToAdd = GM_LOCK_WAITER_WOKEN;
       }
    }
}

VOID
FORCEINLINE
FASTCALL
KiAcquireGuardedMutex(IN OUT PKGUARDED_MUTEX GuardedMutex)
{
    BOOLEAN OldBit;

    /* Remove the lock */
    OldBit = InterlockedBitTestAndReset(&GuardedMutex->Count, GM_LOCK_BIT_V);
    if (!OldBit)
    {
        /* The Guarded Mutex was already locked, enter contented case */
        KiAcquireGuardedMutexContented(GuardedMutex);
    }
}

VOID
FORCEINLINE
FASTCALL
KiReleaseGuardedMutex(IN OUT PKGUARDED_MUTEX GuardedMutex)
{
    LONG OldValue;

    /* Destroy the Owner */
    GuardedMutex->Owner = NULL;

    /* Add the Lock Bit */
    OldValue = InterlockedExchangeAdd(&GuardedMutex->Count, 1);
    ASSERT((OldValue & GM_LOCK_BIT) == 0);

    /* Check if it was already locked, but not woken */
    if ((OldValue) && !(OldValue & GM_LOCK_WAITER_WOKEN))
    {
        /* Update the Oldvalue to what it should be now */
        OldValue |= GM_LOCK_BIT;

        /* Remove the Woken bit */
        if (InterlockedCompareExchange(&GuardedMutex->Count,
                                       OldValue - GM_LOCK_WAITER_WOKEN,
                                       OldValue) == OldValue)
        {
            /* Signal the Gate */
            KeSignalGateBoostPriority(&GuardedMutex->Gate);
        }
    }
}

/* PUBLIC FUNCTIONS **********************************************************/

VOID
FASTCALL
KeInitializeGuardedMutex(OUT PKGUARDED_MUTEX GuardedMutex)
{
    /* Setup the Initial Data */
    GuardedMutex->Count = GM_LOCK_BIT;
    GuardedMutex->Owner = NULL;
    GuardedMutex->Contention = 0;

    /* Initialize the Wait Gate */
    KeInitializeGate(&GuardedMutex->Gate);
}

VOID
FASTCALL
KeAcquireGuardedMutexUnsafe(IN OUT PKGUARDED_MUTEX GuardedMutex)
{
    PKTHREAD Thread = KeGetCurrentThread();

    /* Sanity checks */
    ASSERT((KeGetCurrentIrql() == APC_LEVEL) ||
           (Thread->SpecialApcDisable < 0) ||
           (Thread->Teb == NULL) ||
           (Thread->Teb >= (PTEB)MM_SYSTEM_RANGE_START));
    ASSERT(GuardedMutex->Owner != Thread);

    /* Do the actual acquire */
    KiAcquireGuardedMutex(GuardedMutex);

    /* Set the Owner */
    GuardedMutex->Owner = Thread;
}

VOID
FASTCALL
KeReleaseGuardedMutexUnsafe(IN OUT PKGUARDED_MUTEX GuardedMutex)
{
    /* Sanity checks */
    ASSERT((KeGetCurrentIrql() == APC_LEVEL) ||
           (KeGetCurrentThread()->SpecialApcDisable < 0) ||
           (KeGetCurrentThread()->Teb == NULL) ||
           (KeGetCurrentThread()->Teb >= (PTEB)MM_SYSTEM_RANGE_START));
    ASSERT(GuardedMutex->Owner == KeGetCurrentThread());

    /* Release the mutex */
    KiReleaseGuardedMutex(GuardedMutex);
}

VOID
FASTCALL
KeAcquireGuardedMutex(IN PKGUARDED_MUTEX GuardedMutex)
{
    PKTHREAD Thread = KeGetCurrentThread();

    /* Sanity checks */
    ASSERT_IRQL_LESS_OR_EQUAL(APC_LEVEL);
    ASSERT(GuardedMutex->Owner != Thread);

    /* Disable Special APCs */
    KeEnterGuardedRegion();

    /* Do the actual acquire */
    KiAcquireGuardedMutex(GuardedMutex);

    /* Set the Owner and Special APC Disable state */
    GuardedMutex->Owner = Thread;
    GuardedMutex->SpecialApcDisable = Thread->SpecialApcDisable;
}

VOID
FASTCALL
KeReleaseGuardedMutex(IN OUT PKGUARDED_MUTEX GuardedMutex)
{
    /* Sanity checks */
    ASSERT_IRQL_LESS_OR_EQUAL(APC_LEVEL);
    ASSERT(GuardedMutex->Owner == KeGetCurrentThread());
    ASSERT(GuardedMutex->SpecialApcDisable ==
           KeGetCurrentThread()->SpecialApcDisable);

    /* Release the mutex */
    KiReleaseGuardedMutex(GuardedMutex);

    /* Re-enable APCs */
    KeLeaveGuardedRegion();
}

BOOLEAN
FASTCALL
KeTryToAcquireGuardedMutex(IN OUT PKGUARDED_MUTEX GuardedMutex)
{
    PKTHREAD Thread = KeGetCurrentThread();
    BOOLEAN OldBit;

    /* Block APCs */
    KeEnterGuardedRegion();

    /* Remove the lock */
    OldBit = InterlockedBitTestAndReset(&GuardedMutex->Count, GM_LOCK_BIT_V);
    if (OldBit)
    {
        /* Re-enable APCs */
        KeLeaveGuardedRegion();
        YieldProcessor();

        /* Return failure */
        return FALSE;
    }

    /* Set the Owner and APC State */
    GuardedMutex->Owner = Thread;
    GuardedMutex->SpecialApcDisable = Thread->SpecialApcDisable;
    return TRUE;
}

/**
 * @name KeEnterGuardedRegion
 *
 * Enters a guarded region. This causes all (incl. special kernel) APCs
 * to be disabled.
 */
#undef KeEnterGuardedRegion
VOID
NTAPI
KeEnterGuardedRegion(VOID)
{
    PKTHREAD Thread = KeGetCurrentThread();

    /* Sanity checks */
    ASSERT_IRQL_LESS_OR_EQUAL(APC_LEVEL);
    ASSERT(Thread == KeGetCurrentThread());
    ASSERT((Thread->SpecialApcDisable <= 0) &&
           (Thread->SpecialApcDisable != -32768));

    /* Disable Special APCs */
    Thread->SpecialApcDisable--;
}

/**
 * @name KeLeaveGuardedRegion
 *
 * Leaves a guarded region and delivers pending APCs if possible.
 */
#undef KeLeaveGuardedRegion
VOID
NTAPI
KeLeaveGuardedRegion(VOID)
{
    PKTHREAD Thread = KeGetCurrentThread();

    /* Sanity checks */
    ASSERT_IRQL_LESS_OR_EQUAL(APC_LEVEL);
    ASSERT(Thread == KeGetCurrentThread());
    ASSERT(Thread->SpecialApcDisable < 0);

    /* Boost the enable count and check if Special APCs are enabled */
    if (!(++Thread->SpecialApcDisable))
    {
        /* Check if there are Kernel APCs on the list */
        if (!IsListEmpty(&Thread->ApcState.ApcListHead[KernelMode]))
        {
            /* Check for APC Delivery */
            KiCheckForKernelApcDelivery();
        }
    }
}

/* EOF */
