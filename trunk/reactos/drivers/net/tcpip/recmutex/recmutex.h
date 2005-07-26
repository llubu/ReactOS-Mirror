#ifndef _ROSRTL_RECMUTEX_H
#define _ROSRTL_RECMUTEX_H

typedef struct _RECURSIVE_MUTEX {
    /* Lock. */
    FAST_MUTEX Mutex;
    /* Number of times this object was locked */
    UINT LockCount;
    /* CurrentThread */
    PVOID CurrentThread;  
    /* Notification event which signals that another thread can take over */
    KEVENT StateLockedEvent;
    /* IRQL from spin lock */
    KIRQL OldIrql;
    /* Is Locked */
    BOOLEAN Locked;
    /* Is reader or writer phase */
    BOOLEAN Writer;
    /* Spin lock needed for */
    KSPIN_LOCK SpinLock;
} RECURSIVE_MUTEX, *PRECURSIVE_MUTEX;

extern VOID RecursiveMutexInit( PRECURSIVE_MUTEX RecMutex );
extern UINT RecursiveMutexEnter( PRECURSIVE_MUTEX RecMutex, BOOL ToRead );
extern VOID RecursiveMutexLeave( PRECURSIVE_MUTEX RecMutex );

#endif/*_ROSRTL_RECMUTEX_H*/
