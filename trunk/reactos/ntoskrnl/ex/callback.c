/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ex/callback.c
 * PURPOSE:         Executive callbacks
 * PROGRAMMERS:     Filip Navara
 *                  Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, ExpInitializeCallbacks)
#endif


/* TYPES ********************************************************************/

/* Mapping for Callback Object */
GENERIC_MAPPING ExpCallbackMapping =
{
   CALLBACK_READ,
   CALLBACK_WRITE,
   CALLBACK_EXECUTE,
   CALLBACK_ALL_ACCESS
};

/* Structure used to hold Callbacks */
typedef struct _CALLBACK_REGISTRATION
{
   LIST_ENTRY RegisteredCallbacks;
   PCALLBACK_OBJECT CallbackObject;
   PCALLBACK_FUNCTION CallbackFunction;
   PVOID CallbackContext;
   ULONG InUse;
   BOOLEAN PendingDeletion;
} CALLBACK_REGISTRATION, *PCALLBACK_REGISTRATION;

typedef struct
{
    PCALLBACK_OBJECT *CallbackObject;
    PWSTR Name;
} SYSTEM_CALLBACKS;

/* Kernel Default Callbacks */
PCALLBACK_OBJECT SetSystemTimeCallback;
PCALLBACK_OBJECT SetSystemStateCallback;
PCALLBACK_OBJECT PowerStateCallback;

SYSTEM_CALLBACKS ExpInitializeCallback[] =
{
   {&SetSystemTimeCallback, L"\\Callback\\SetSystemTime"},
   {&SetSystemStateCallback, L"\\Callback\\SetSystemState"},
   {&PowerStateCallback, L"\\Callback\\PowerState"},
   {NULL, NULL}
};

POBJECT_TYPE ExCallbackObjectType;
KEVENT ExpCallbackEvent;

/* FUNCTIONS *****************************************************************/

/*++
 * @name ExpInitializeCallbacks
 *
 * Creates the Callback Object as a valid Object Type in the Kernel.
 * Internal function, subject to further review
 *
 * @return TRUE if the Callback Object Type was successfully created.
 *
 * @remarks None
 *
 *--*/
VOID
INIT_FUNCTION
NTAPI
ExpInitializeCallbacks(VOID)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;
    UNICODE_STRING DirName = RTL_CONSTANT_STRING(L"\\Callback");
    UNICODE_STRING CallbackName;
    UNICODE_STRING Name;
    OBJECT_TYPE_INITIALIZER ObjectTypeInitializer;
    HANDLE DirectoryHandle;
    ULONG i;

    /*  Initialize the Callback Object type  */
    RtlZeroMemory(&ObjectTypeInitializer, sizeof(ObjectTypeInitializer));
    RtlInitUnicodeString(&Name, L"Callback");
    ObjectTypeInitializer.Length = sizeof(ObjectTypeInitializer);
    ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof(CALLBACK_OBJECT);
    ObjectTypeInitializer.GenericMapping = ExpCallbackMapping;
    ObjectTypeInitializer.PoolType = NonPagedPool;

    Status = ObCreateObjectType(&Name, &ObjectTypeInitializer, NULL, &ExCallbackObjectType);

    /* Fail if it wasn't created successfully */
    if (!NT_SUCCESS(Status))
    {
        return;
    }

    /* Initialize the Object */
    InitializeObjectAttributes(
        &ObjectAttributes,
        &DirName,
        OBJ_CASE_INSENSITIVE | OBJ_PERMANENT,
        NULL,
        NULL
        );

    /* Create the Object Directory */
    Status = NtCreateDirectoryObject(
        &DirectoryHandle,
        DIRECTORY_ALL_ACCESS,
        &ObjectAttributes
        );

    /* Fail if couldn't create */
    if (!NT_SUCCESS(Status))
    {
        return;
    }

    /* Close Handle... */
    NtClose(DirectoryHandle);

    /* Initialize Event used when unregistering */
    KeInitializeEvent(&ExpCallbackEvent, NotificationEvent, 0);

    /* Default NT Kernel Callbacks. */
    for (i=0; ExpInitializeCallback[i].CallbackObject; i++)
    {
        /* Create the name from the structure */
        RtlInitUnicodeString(&CallbackName, ExpInitializeCallback[i].Name);

        /* Initialize the Object Attributes Structure */
        InitializeObjectAttributes(
            &ObjectAttributes,
            &CallbackName,
            OBJ_PERMANENT | OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );

        /* Create the Callback Object */
        Status = ExCreateCallback(
            (PCALLBACK_OBJECT*)&(ExpInitializeCallback[i].CallbackObject),
            &ObjectAttributes,
            TRUE,
            TRUE
            );

        /* Make sure Global Callbacks have been created */
        if (!NT_SUCCESS(Status))
        {
            return;
        }
    }
    /* Everything successful */
}

/*++
 * @name ExCreateCallback
 * @implemented
 *
 * Opens or creates a Callback Object. Creates only if Create is true.
 * Allows multiple Callback Functions to be registred only if
 * AllowMultipleCallbacks is true.
 * See: http://www.osronline.com/ddkx/kmarch/k102_967m.htm
 *      http://www.osronline.com/article.cfm?id=24
 *
 * @param CallbackObject
 *        Pointer that will receive the Callback Object.
 *
 * @param CallbackName
 *        Name of Callback
 *
 * @param Create
 *        Determines if the object will be created if it doesn't exit
 *
 * @param AllowMultipleCallbacks
 *        Determines if more then one registered callback function
 *        can be attached to this Callback Object.
 *
 * @return STATUS_SUCESS if not failed.
 *
 * @remarks Must be called at IRQL = PASSIVE_LEVEL
 *
 *--*/
NTSTATUS
NTAPI
ExCreateCallback(OUT PCALLBACK_OBJECT *CallbackObject,
                 IN POBJECT_ATTRIBUTES ObjectAttributes,
                 IN BOOLEAN Create,
                 IN BOOLEAN AllowMultipleCallbacks)
{
    PCALLBACK_OBJECT Callback;
    NTSTATUS    Status;
    HANDLE     Handle;

    PAGED_CODE();

    /* Open a handle to the callback if it exists */
    if (ObjectAttributes->ObjectName)
    {
        Status = ObOpenObjectByName(ObjectAttributes,
                                    ExCallbackObjectType,
                                    KernelMode,
                                    NULL,
                                    0,
                                    NULL,
                                    &Handle);
    }
    else
    {
        Status = STATUS_UNSUCCESSFUL;
    }

    /* We weren't able to open it...should we create it? */
    if(!NT_SUCCESS(Status) && Create )
    {
        Status = ObCreateObject(KernelMode,
                                ExCallbackObjectType,
                                ObjectAttributes,
                                KernelMode,
                                NULL,
                                sizeof(CALLBACK_OBJECT),
                                0,
                                0,
                                (PVOID *)&Callback );

        /* We Created it...let's initialize the structure now */
        if(NT_SUCCESS(Status))
        {
            KeInitializeSpinLock(&Callback->Lock);      /*  SpinLock   */
            InitializeListHead(&Callback->RegisteredCallbacks);   /*  Callback Entries */
            Callback->AllowMultipleCallbacks = AllowMultipleCallbacks; /*  Multiple Callbacks */
            /*  Create the object */
            Status = ObInsertObject(Callback,
                                    NULL,
                                    FILE_READ_DATA,
                                    0,
                                    NULL,
                                    &Handle );
        }
    }

    if(NT_SUCCESS(Status))
    {
        /* Get a pointer to the new object from the handle we just got */
        Status = ObReferenceObjectByHandle(Handle,
                                           0,
                                           ExCallbackObjectType,
                                           KernelMode,
                                           (PVOID)&Callback,
                                           NULL);
        /* Close the Handle, since we now have the pointer */
        ZwClose(Handle);
    }

    /* Everything went fine, so return a pointer to the Object */
    if (NT_SUCCESS(Status))
    {
        *CallbackObject = (PCALLBACK_OBJECT)Callback;
    }
    return Status;
}

/*++
 * @name ExNotifyCallback
 * @implemented
 *
 * Calls a function pointer (a registered callback)
 * See: http://www.osronline.com/ddkx/kmarch/k102_2f5e.htm
 *      http://msdn.microsoft.com/library/en-us/Kernel_d/hh/Kernel_d/Synchro_e954f515-e536-4e12-8419-e7e54c4a963b.xml.asp?frame=true
 *      http://vmsone.com/~decuslib/vmssig/vmslt99b/nt/wdm-callback.txt
 *
 * @param CallbackObject
 *        Which callback to call
 *
 * @param Argument1
 *        Pointer/data to send to callback function
 *
 * @param Argument2
 *        Pointer/data to send to callback function
 *
 * @return None
 *
 * @remarks None
 *
 *--*/
VOID
NTAPI
ExNotifyCallback(IN PCALLBACK_OBJECT OpaqueCallbackObject,
                 IN PVOID Argument1,
                 IN PVOID Argument2)
{
    PCALLBACK_OBJECT   CallbackObject = (PCALLBACK_OBJECT)OpaqueCallbackObject;
    PLIST_ENTRY             RegisteredCallbacks;
    PCALLBACK_REGISTRATION  CallbackRegistration;
    KIRQL                   OldIrql;

    /* Acquire the Lock */
    OldIrql = KfAcquireSpinLock(&CallbackObject->Lock);

    /* Enumerate through all the registered functions */
    for (RegisteredCallbacks = CallbackObject->RegisteredCallbacks.Flink;
         RegisteredCallbacks != &CallbackObject->RegisteredCallbacks;
         RegisteredCallbacks = RegisteredCallbacks->Flink)
    {
        /* Get a pointer to a Callback Registration from the List Entries */
        CallbackRegistration = CONTAINING_RECORD(RegisteredCallbacks,
                                                 CALLBACK_REGISTRATION,
                                                 RegisteredCallbacks);

        /* Don't bother doing Callback Notification if it's pending to be deleted */
        if (!CallbackRegistration->PendingDeletion)
        {
            /* Mark the Callback in use, so it won't get deleted while we are calling it */
            CallbackRegistration->InUse += 1;

            /* Release the Spinlock before making the call */
            KfReleaseSpinLock(&CallbackObject->Lock, OldIrql);

            /* Call the Registered Function */
            CallbackRegistration->CallbackFunction(
                CallbackRegistration->CallbackContext,
                Argument1,
                Argument2
                );

            /* Get SpinLock back */
            OldIrql = KfAcquireSpinLock(&CallbackObject->Lock);

            /* We are not in use anymore */
            CallbackRegistration->InUse -= 1;

            /* If another instance of this function isn't running and deletion is pending, signal the event */
            if (CallbackRegistration->PendingDeletion  && CallbackRegistration->InUse == 0)
            {
                KeSetEvent(&ExpCallbackEvent, 0, FALSE);
            }
        }
    }
    /* Unsynchronize and release the Callback Object */
    KfReleaseSpinLock(&CallbackObject->Lock, OldIrql);
}

/*++
 * @name ExRegisterCallback
 * @implemented
 *
 * Allows a function to associate a callback pointer (Function) to
 * a created Callback object
 * See: DDK, OSR, links in ExNotifyCallback
 *
 * @param CallbackObject
 *        The Object Created with ExCreateCallBack
 *
 * @param CallBackFunction
 *        Pointer to the function to be called back
 *
 * @param CallBackContext
 *        Block of memory that can contain user-data which will be
 *        passed on to the callback
 *
 * @return A handle to a Callback Registration Structure (MSDN Documentation)
 *
 * @remarks None
 *
 *--*/
PVOID
NTAPI
ExRegisterCallback(IN PCALLBACK_OBJECT OpaqueCallbackObject,
                   IN PCALLBACK_FUNCTION CallbackFunction,
                   IN PVOID CallbackContext)
{
    PCALLBACK_OBJECT CallbackObject = (PCALLBACK_OBJECT)OpaqueCallbackObject;
    PCALLBACK_REGISTRATION  CallbackRegistration = NULL;
    KIRQL     OldIrql;

    PAGED_CODE();

    /* Create reference to Callback Object */
    ObReferenceObject (CallbackObject);

    /* Allocate memory for the structure */
    CallbackRegistration = ExAllocatePoolWithTag(
                               NonPagedPool,
                               sizeof(CallbackRegistration),
                               CALLBACK_TAG
                               );
    /* Fail if memory allocation failed */
    if(!CallbackRegistration)
    {
        ObDereferenceObject (CallbackObject);
        return NULL;
    }

    /* Create Callback Registration */
    CallbackRegistration->CallbackObject = CallbackObject; /* When unregistering, drivers send a handle to the Registration, not the object... */
    CallbackRegistration->CallbackFunction = CallbackFunction; /* NotifyCallback uses Objects, so this needs to be here in order to call the registered functions */
    CallbackRegistration->CallbackContext = CallbackContext; /* The documented NotifyCallback returns the Context, so we must save this somewhere */

    /* Acquire SpinLock */
    OldIrql = KfAcquireSpinLock (&CallbackObject->Lock);

    /* Add Callback if 1) No Callbacks registered or 2) Multiple Callbacks allowed */
    if(CallbackObject->AllowMultipleCallbacks || IsListEmpty(&CallbackObject->RegisteredCallbacks))
    {
        InsertTailList(&CallbackObject->RegisteredCallbacks,&CallbackRegistration->RegisteredCallbacks);
    }
    else
    {
        ExFreePool(CallbackRegistration);
        CallbackRegistration = NULL;
    }

    /* Release SpinLock */
    KfReleaseSpinLock(&CallbackObject->Lock, OldIrql);

    /* Return handle to Registration Object */
    return (PVOID)CallbackRegistration;
}

/*++
 * @name ExUnregisterCallback
 * @implemented
 *
 * Deregisters a CallBack
 * See: DDK, OSR, links in ExNotifyCallback
 *
 * @param CallbackRegistration
 *        Callback Registration Handle
 *
 * @return None
 *
 * @remarks None
 *
 *--*/
VOID
NTAPI
ExUnregisterCallback(IN PVOID CallbackRegistrationHandle)
{
    PCALLBACK_REGISTRATION  CallbackRegistration;
    PCALLBACK_OBJECT    CallbackObject;
    KIRQL                   OldIrql;

    PAGED_CODE();

    /* Convert Handle to valid Structure Pointer */
    CallbackRegistration = (PCALLBACK_REGISTRATION) CallbackRegistrationHandle;

    /* Get the Callback Object */
    CallbackObject = CallbackRegistration->CallbackObject;

    /* Lock the Object */
    OldIrql = KfAcquireSpinLock (&CallbackObject->Lock);

    /* We can't Delete the Callback if it's in use, because this would create a call towards a null pointer => crash */
    while (CallbackRegistration->InUse)
    {
        /* Similarly, we also don't want to wait ages for all pending callbacks to be called */
        CallbackRegistration->PendingDeletion = TRUE;

        /* We are going to wait for the event, so the Lock isn't necessary */
        KfReleaseSpinLock (&CallbackObject->Lock, OldIrql);

        /* Make sure the event is cleared */
        KeClearEvent (&ExpCallbackEvent);

        /* Wait for the Event */
        KeWaitForSingleObject (
            &ExpCallbackEvent,
            Executive,
            KernelMode,
            FALSE,
            NULL
            );

        /* We need the Lock again */
        OldIrql = KfAcquireSpinLock(&CallbackObject->Lock);
    }

    /* Remove the Callback */
    RemoveEntryList(&CallbackRegistration->RegisteredCallbacks);

    /* It's now safe to release the lock */
    KfReleaseSpinLock(&CallbackObject->Lock, OldIrql);

    /* Delete this registration */
    ExFreePool(CallbackRegistration);

    /* Remove the reference */
    ObDereferenceObject(CallbackObject);
}

/* EOF */
