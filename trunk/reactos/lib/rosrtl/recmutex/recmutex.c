#define NTOS_MODE_USER
#include <ntos.h>
#include <rosrtl/recmutex.h>

VOID RecursiveMutexInit( PRECURSIVE_MUTEX RecMutex ) {
    RtlZeroMemory( RecMutex, sizeof(*RecMutex) );
    KeInitializeSpinLock( &RecMutex->SpinLock );
    ExInitializeFastMutex( &RecMutex->Mutex );
    KeInitializeEvent( &RecMutex->StateLockedEvent, 
		       NotificationEvent, FALSE );    
}

/* NOTE: When we leave, the FAST_MUTEX must have been released.  The result
 * is that we always exit in the same irql as entering */
UINT RecursiveMutexEnter( PRECURSIVE_MUTEX RecMutex, BOOL ToWrite ) {
    NTSTATUS Status = STATUS_SUCCESS;
    PVOID CurrentThread = KeGetCurrentThread();

    /* Wait for the previous user to unlock the RecMutex state.  There might be
     * multiple waiters waiting to change the state.  We need to check each
     * time we get the event whether somebody still has the state locked */

    if( !RecMutex ) return FALSE;

    if( CurrentThread == RecMutex->CurrentThread || 
	(!ToWrite && !RecMutex->Writer) ) {
	RecMutex->LockCount++;
	return TRUE;
    }

    if( KeGetCurrentIrql() == PASSIVE_LEVEL ) {
	ExAcquireFastMutex( &RecMutex->Mutex );
	RecMutex->OldIrql = PASSIVE_LEVEL;
	while( RecMutex->Locked ) {
	    ExReleaseFastMutex( &RecMutex->Mutex );
	    Status = KeWaitForSingleObject( &RecMutex->StateLockedEvent,
					    UserRequest,
					    KernelMode,
					    FALSE,
					    NULL );
	    ExAcquireFastMutex( &RecMutex->Mutex );
	    if( Status == STATUS_SUCCESS ) break;
	}
	RecMutex->Locked = TRUE;
	RecMutex->Writer = ToWrite;
	RecMutex->CurrentThread = CurrentThread;
	RecMutex->LockCount++;
	ExReleaseFastMutex( &RecMutex->Mutex );
    } else {
	KeAcquireSpinLock( &RecMutex->SpinLock, &RecMutex->OldIrql );
	RecMutex->Locked = TRUE;
	RecMutex->Writer = ToWrite;
	RecMutex->CurrentThread = CurrentThread;
	RecMutex->LockCount++;
    }

    return TRUE;
}

VOID RecursiveMutexLeave( PRECURSIVE_MUTEX RecMutex ) {
    if( RecMutex->LockCount == 0 ) {
	return;
    } else
	RecMutex->LockCount--;

    if( !RecMutex->LockCount ) {
	RecMutex->CurrentThread = NULL;
	if( RecMutex->OldIrql == PASSIVE_LEVEL ) {
	    ExAcquireFastMutex( &RecMutex->Mutex );
	    RecMutex->Locked = FALSE;
	    RecMutex->Writer = FALSE;
	    ExReleaseFastMutex( &RecMutex->Mutex );
	} else {
	    RecMutex->Locked = FALSE;
	    RecMutex->Writer = FALSE;
	    KeReleaseSpinLock( &RecMutex->SpinLock, RecMutex->OldIrql );
	}

	RecMutex->OldIrql = PASSIVE_LEVEL;
	KePulseEvent( &RecMutex->StateLockedEvent, IO_NETWORK_INCREMENT, 
		      FALSE );
    }
}

