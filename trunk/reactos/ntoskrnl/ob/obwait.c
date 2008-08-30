/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ob/obwait.c
 * PURPOSE:         Handles Waiting on Objects
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 *                  Thomas Weidenmueller (w3seek@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

/*++
* @name NtWaitForMultipleObjects
* @implemented NT4
*
*     The NtWaitForMultipleObjects routine <FILLMEIN>
*
* @param ObjectCount
*        <FILLMEIN>
*
* @param HandleArray
*        <FILLMEIN>
*
* @param WaitType
*        <FILLMEIN>
*
* @param Alertable
*        <FILLMEIN>
*
* @param TimeOut
*        <FILLMEIN>
*
* @return STATUS_SUCCESS or appropriate error value.
*
* @remarks None.
*
*--*/
NTSTATUS
NTAPI
NtWaitForMultipleObjects(IN ULONG ObjectCount,
                         IN PHANDLE HandleArray,
                         IN WAIT_TYPE WaitType,
                         IN BOOLEAN Alertable,
                         IN PLARGE_INTEGER TimeOut OPTIONAL)
{
    PKWAIT_BLOCK WaitBlockArray = NULL;
    HANDLE Handles[MAXIMUM_WAIT_OBJECTS], KernelHandle;
    PVOID Objects[MAXIMUM_WAIT_OBJECTS];
    PVOID WaitObjects[MAXIMUM_WAIT_OBJECTS];
    ULONG i = 0, ReferencedObjects = 0, j;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    LARGE_INTEGER SafeTimeOut;
    BOOLEAN LockInUse;
    PHANDLE_TABLE_ENTRY HandleEntry;
    POBJECT_HEADER ObjectHeader;
    PHANDLE_TABLE HandleTable;
    ACCESS_MASK GrantedAccess;
    PVOID DefaultObject;
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();

    /* Enter a critical region since we'll play with handles */
    LockInUse = TRUE;
    KeEnterCriticalRegion();

    /* Check for valid Object Count */
    if ((ObjectCount > MAXIMUM_WAIT_OBJECTS) || !(ObjectCount))
    {
        /* Fail */
        Status = STATUS_INVALID_PARAMETER_1;
        goto Quickie;
    }

    /* Check for valid Wait Type */
    if ((WaitType != WaitAll) && (WaitType != WaitAny))
    {
        /* Fail */
        Status = STATUS_INVALID_PARAMETER_3;
        goto Quickie;
    }

    /* Enter SEH */
    _SEH_TRY
    {
        /* Check if the call came from user mode */
        if(PreviousMode != KernelMode)
        {
            /* Check if we have a timeout */
            if (TimeOut)
            {
                /* Make a local copy of the timeout on the stack */
                SafeTimeOut = ProbeForReadLargeInteger(TimeOut);
                TimeOut = &SafeTimeOut;
            }

            /* Probe all the handles */
            ProbeForRead(HandleArray,
                         ObjectCount * sizeof(HANDLE),
                         sizeof(HANDLE));
        }

       /*
        * Make a copy so we don't have to guard with SEH later and keep
        * track of what objects we referenced if dereferencing pointers
        * suddenly fails
        */
        RtlCopyMemory(Handles,
                      HandleArray,
                      ObjectCount * sizeof(HANDLE));
    }
    _SEH_HANDLE
    {
        /* Get exception code */
        Status = _SEH_GetExceptionCode();
    }
    _SEH_END;

    /* Fail if we raised an exception */
    if (!NT_SUCCESS(Status)) goto Quickie;

    /* Check if we can use the internal Wait Array */
    if (ObjectCount > THREAD_WAIT_OBJECTS)
    {
        /* Allocate from Pool */
        WaitBlockArray = ExAllocatePoolWithTag(NonPagedPool,
                                               ObjectCount *
                                               sizeof(KWAIT_BLOCK),
                                               TAG_WAIT);
        if (!WaitBlockArray)
        {
            /* Fail */
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Quickie;
        }
    }

    /* Start the loop */
    do
    {
        /* Use the right Executive Handle */
        if (ObIsKernelHandle(Handles[i], PreviousMode))
        {
            /* Use the System Handle Table and decode */
            HandleTable = ObpKernelHandleTable;
            KernelHandle = ObKernelHandleToHandle(Handles[i]);

            /* Get a pointer to it */
            HandleEntry = ExMapHandleToPointer(HandleTable, KernelHandle);
        }
        else
        {
            /* Use the Process' Handle table and get the Ex Handle */
            HandleTable = PsGetCurrentProcess()->ObjectTable;

            /* Get a pointer to it */
            HandleEntry = ExMapHandleToPointer(HandleTable, Handles[i]);
        }

        /* Check if we have an entry */
        if (!HandleEntry)
        {
            /* Fail, handle is invalid */
            Status = STATUS_INVALID_HANDLE;
            goto Quickie;
        }

        /* Check for synchronize access */
        GrantedAccess = HandleEntry->GrantedAccess;
        if ((PreviousMode != KernelMode) && (!(GrantedAccess & SYNCHRONIZE)))
        {
            /* Unlock the entry and fail */
            ExUnlockHandleTableEntry(HandleTable, HandleEntry);
            Status = STATUS_ACCESS_DENIED;
            goto Quickie;
        }

        /* Get the Object Header */
        ObjectHeader = ObpGetHandleObject(HandleEntry);

        /* Get default Object */
        DefaultObject = ObjectHeader->Type->DefaultObject;

        /* Check if it's the internal offset */
        if (IsPointerOffset(DefaultObject))
        {
            /* Increase reference count */
            InterlockedIncrement(&ObjectHeader->PointerCount);
            ReferencedObjects++;

            /* Save the Object and Wait Object, this is a relative offset */
            Objects[i] = &ObjectHeader->Body;
            WaitObjects[i] = (PVOID)((ULONG_PTR)&ObjectHeader->Body +
                                     (ULONG_PTR)DefaultObject);
        }
        else
        {
            /* This is our internal Object */
            ReferencedObjects++;
            Objects[i] = NULL;
            WaitObjects[i] = DefaultObject;
        }

        /* Unlock the Handle Table Entry */
        ExUnlockHandleTableEntry(HandleTable, HandleEntry);

        /* Keep looping */
        i++;
    } while (i < ObjectCount);

    /* For a Waitall, we can't have the same object more then once */
    if (WaitType == WaitAll)
    {
        /* Clear the main loop variable */
        i = 0;

        /* Start the loop */
        do
        {
            /* Check the current and forward object */
            for (j = i + 1; j < ObjectCount; j++)
            {
                /* Make sure they don't match */
                if (WaitObjects[i] == WaitObjects[j])
                {
                    /* Fail */
                    Status = STATUS_INVALID_PARAMETER_MIX;
                    goto Quickie;
                }
            }

            /* Keep looping */
            i++;
        } while (i < ObjectCount);
    }

    /* Now we can finally wait. Use SEH since it can raise an exception */
    _SEH_TRY
    {
        /* We're done playing with handles */
        LockInUse = FALSE;
        KeLeaveCriticalRegion();

        /* Do the kernel wait */
        Status = KeWaitForMultipleObjects(ObjectCount,
                                          WaitObjects,
                                          WaitType,
                                          UserRequest,
                                          PreviousMode,
                                          Alertable,
                                          TimeOut,
                                          WaitBlockArray);
    }
    _SEH_HANDLE
    {
        /* Get the exception code */
        Status = _SEH_GetExceptionCode();
    }
    _SEH_END;

Quickie:
    /* First derefence */
    while (ReferencedObjects)
    {
        /* Decrease the number of objects */
        ReferencedObjects--;

        /* Check if we had a valid object in this position */
        if (Objects[ReferencedObjects])
        {
            /* Dereference it */
            ObDereferenceObject(Objects[ReferencedObjects]);
        }
    }

    /* Free wait block array */
    if (WaitBlockArray) ExFreePool(WaitBlockArray);

    /* Re-enable APCs if needed */
    if (LockInUse) KeLeaveCriticalRegion();

    /* Return status */
    return Status;
}

/*++
* @name NtWaitForMultipleObjects32
* @implemented NT5.1
*
*     The NtWaitForMultipleObjects32 routine <FILLMEIN>
*
* @param ObjectCount
*        <FILLMEIN>
*
* @param HandleArray
*        <FILLMEIN>
*
* @param WaitType
*        <FILLMEIN>
*
* @param Alertable
*        <FILLMEIN>
*
* @param TimeOut
*        <FILLMEIN>
*
* @return STATUS_SUCCESS or appropriate error value.
*
* @remarks None.
*
*--*/
NTSTATUS
NTAPI
NtWaitForMultipleObjects32(IN ULONG ObjectCount,
                           IN PLONG Handles,
                           IN WAIT_TYPE WaitType,
                           IN BOOLEAN Alertable,
                           IN PLARGE_INTEGER TimeOut OPTIONAL)
{
    /* FIXME WOW64 */
    return NtWaitForMultipleObjects(ObjectCount,
                                    (PHANDLE)Handles,
                                    WaitType,
                                    Alertable,
                                    TimeOut);
}

/*++
* @name NtWaitForSingleObject
* @implemented NT4
*
*     The NtWaitForSingleObject routine <FILLMEIN>
*
* @param ObjectHandle
*        <FILLMEIN>
*
* @param Alertable
*        <FILLMEIN>
*
* @param TimeOut
*        <FILLMEIN>
*
* @return STATUS_SUCCESS or appropriate error value.
*
* @remarks None.
*
*--*/
NTSTATUS
NTAPI
NtWaitForSingleObject(IN HANDLE ObjectHandle,
                      IN BOOLEAN Alertable,
                      IN PLARGE_INTEGER TimeOut  OPTIONAL)
{
    PVOID Object, WaitableObject;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    LARGE_INTEGER SafeTimeOut;
    NTSTATUS Status = STATUS_SUCCESS;

    /* Check if we came with a timeout from user mode */
    if ((TimeOut) && (PreviousMode != KernelMode))
    {
        /* Enter SEH for proving */
        _SEH_TRY
        {
            /* Make a copy on the stack */
            SafeTimeOut = ProbeForReadLargeInteger(TimeOut);
            TimeOut = &SafeTimeOut;
        }
        _SEH_HANDLE
        {
            /* Get the exception code */
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
        if (!NT_SUCCESS(Status)) return Status;
    }

    /* Get the Object */
    Status = ObReferenceObjectByHandle(ObjectHandle,
                                       SYNCHRONIZE,
                                       NULL,
                                       PreviousMode,
                                       &Object,
                                       NULL);
    if (NT_SUCCESS(Status))
    {
        /* Get the Waitable Object */
        WaitableObject = OBJECT_TO_OBJECT_HEADER(Object)->Type->DefaultObject;

        /* Is it an offset for internal objects? */
        if (IsPointerOffset(WaitableObject))
        {
            /* Turn it into a pointer */
            WaitableObject = (PVOID)((ULONG_PTR)Object +
                                     (ULONG_PTR)WaitableObject);
        }

        /* SEH this since it can also raise an exception */
        _SEH_TRY
        {
            /* Ask the kernel to do the wait */
            Status = KeWaitForSingleObject(WaitableObject,
                                           UserRequest,
                                           PreviousMode,
                                           Alertable,
                                           TimeOut);
        }
        _SEH_HANDLE
        {
            /* Get the exception code */
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        /* Dereference the Object */
        ObDereferenceObject(Object);
    }

    /* Return the status */
    return Status;
}

/*++
* @name NtSignalAndWaitForSingleObject
* @implemented NT4
*
*     The NtSignalAndWaitForSingleObject routine <FILLMEIN>
*
* @param ObjectHandleToSignal
*        <FILLMEIN>
*
* @param WaitableObjectHandle
*        <FILLMEIN>
*
* @param Alertable
*        <FILLMEIN>
*
* @param TimeOut
*        <FILLMEIN>
*
* @return STATUS_SUCCESS or appropriate error value.
*
* @remarks None.
*
*--*/
NTSTATUS
NTAPI
NtSignalAndWaitForSingleObject(IN HANDLE ObjectHandleToSignal,
                               IN HANDLE WaitableObjectHandle,
                               IN BOOLEAN Alertable,
                               IN PLARGE_INTEGER TimeOut OPTIONAL)
{
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    POBJECT_TYPE Type;
    PVOID SignalObj, WaitObj, WaitableObject;
    LARGE_INTEGER SafeTimeOut;
    OBJECT_HANDLE_INFORMATION HandleInfo;
    NTSTATUS Status = STATUS_SUCCESS;

    /* Check if we came with a timeout from user mode */
    if ((TimeOut) && (PreviousMode != KernelMode))
    {
        /* Enter SEH for probing */
        _SEH_TRY
        {
            /* Make a copy on the stack */
            SafeTimeOut = ProbeForReadLargeInteger(TimeOut);
            TimeOut = &SafeTimeOut;
        }
        _SEH_HANDLE
        {
            /* Get the exception code */
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
        if (!NT_SUCCESS(Status)) return Status;
    }

    /* Start by getting the signal object*/
    Status = ObReferenceObjectByHandle(ObjectHandleToSignal,
                                       0,
                                       NULL,
                                       PreviousMode,
                                       &SignalObj,
                                       &HandleInfo);
    if (!NT_SUCCESS(Status)) return Status;

    /* Now get the wait object */
    Status = ObReferenceObjectByHandle(WaitableObjectHandle,
                                       SYNCHRONIZE,
                                       NULL,
                                       PreviousMode,
                                       &WaitObj,
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        /* Failed to reference the wait object */
        ObDereferenceObject(SignalObj);
        return Status;
    }

    /* Get the real waitable object */
    WaitableObject = OBJECT_TO_OBJECT_HEADER(WaitObj)->Type->DefaultObject;

    /* Handle internal offset */
    if (IsPointerOffset(WaitableObject))
    {
        /* Get real pointer */
        WaitableObject = (PVOID)((ULONG_PTR)WaitObj +
                                 (ULONG_PTR)WaitableObject);
    }

    /* Check Signal Object Type */
    Type = OBJECT_TO_OBJECT_HEADER(SignalObj)->Type;
    if (Type == ExEventObjectType)
    {
        /* Check if we came from user-mode without the right access */
        if ((PreviousMode != KernelMode) &&
            !(HandleInfo.GrantedAccess & EVENT_MODIFY_STATE))
        {
            /* Fail: lack of rights */
            Status = STATUS_ACCESS_DENIED;
            goto Quickie;
        }

        /* Set the Event */
        KeSetEvent(SignalObj, EVENT_INCREMENT, TRUE);
    }
    else if (Type == ExMutantObjectType)
    {
        /* This can raise an exception */
        _SEH_TRY
        {
            /* Release the mutant */
            KeReleaseMutant(SignalObj, MUTANT_INCREMENT, FALSE, TRUE);
        }
        _SEH_HANDLE
        {
            /* Get the exception code */
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
    }
    else if (Type == ExSemaphoreObjectType)
    {
        /* Check if we came from user-mode without the right access */
        if ((PreviousMode != KernelMode) &&
            !(HandleInfo.GrantedAccess & SEMAPHORE_MODIFY_STATE))
        {
            /* Fail: lack of rights */
            Status = STATUS_ACCESS_DENIED;
            goto Quickie;
        }

        /* This can raise an exception*/
        _SEH_TRY
        {
            /* Release the semaphore */
            KeReleaseSemaphore(SignalObj, SEMAPHORE_INCREMENT, 1, TRUE);
        }
        _SEH_HANDLE
        {
            /* Get the exception code */
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
    }
    else
    {
        /* This isn't a valid object to be waiting on */
        Status = STATUS_OBJECT_TYPE_MISMATCH;
    }

    /* Make sure we didn't fail */
    if (NT_SUCCESS(Status))
    {
        /* SEH this since it can also raise an exception */
        _SEH_TRY
        {
            /* Perform the wait now */
            Status = KeWaitForSingleObject(WaitableObject,
                                           UserRequest,
                                           PreviousMode,
                                           Alertable,
                                           TimeOut);
        }
        _SEH_HANDLE
        {
            /* Get the exception code */
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
    }

    /* We're done here, dereference both objects */
Quickie:
    ObDereferenceObject(SignalObj);
    ObDereferenceObject(WaitObj);
    return Status;
}

/* EOF */
