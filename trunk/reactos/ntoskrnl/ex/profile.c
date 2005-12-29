/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/ex/profile.c
 * PURPOSE:         Support for Executive Profile Objects
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 *                  Thomas Weidenmueller
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, ExpInitializeProfileImplementation)
#endif

#define TAG_PROFILE TAG('P', 'r', 'o', 'f')

/* GLOBALS *******************************************************************/

POBJECT_TYPE ExProfileObjectType = NULL;
KMUTEX ExpProfileMutex;

GENERIC_MAPPING ExpProfileMapping =
{
    STANDARD_RIGHTS_READ    | PROFILE_CONTROL,
    STANDARD_RIGHTS_WRITE   | PROFILE_CONTROL,
    STANDARD_RIGHTS_EXECUTE | PROFILE_CONTROL,
    PROFILE_ALL_ACCESS
};

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
ExpDeleteProfile(PVOID ObjectBody)
{
    PEPROFILE Profile;
    ULONG State;

    /* Typecast the Object */
    Profile = (PEPROFILE)ObjectBody;

    /* Check if there if the Profile was started */
    if (Profile->LockedBuffer)
    {
        /* Stop the Profile */
        State = KeStopProfile(Profile->KeProfile);
        ASSERT(State != FALSE);

        /* Unmap the Locked Buffer */
        MmUnmapLockedPages(Profile->LockedBuffer, Profile->Mdl);
        MmUnlockPages(Profile->Mdl);
        ExFreePool(Profile->Mdl);
    }

    /* Check if a Process is associated and reference it */
    if (Profile->Process) ObDereferenceObject(Profile->Process);
}

VOID
INIT_FUNCTION
NTAPI
ExpInitializeProfileImplementation(VOID)
{
    OBJECT_TYPE_INITIALIZER ObjectTypeInitializer;
    UNICODE_STRING Name;
    DPRINT("Creating Profile Object Type\n");

    /* Initialize the Mutex to lock the States */
    KeInitializeMutex(&ExpProfileMutex, 64);

    /* Create the Event Pair Object Type */
    RtlZeroMemory(&ObjectTypeInitializer, sizeof(ObjectTypeInitializer));
    RtlInitUnicodeString(&Name, L"Profile");
    ObjectTypeInitializer.Length = sizeof(ObjectTypeInitializer);
    ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof(KPROFILE);
    ObjectTypeInitializer.GenericMapping = ExpProfileMapping;
    ObjectTypeInitializer.PoolType = NonPagedPool;
    ObjectTypeInitializer.DeleteProcedure = ExpDeleteProfile;
    ObjectTypeInitializer.ValidAccessMask = PROFILE_ALL_ACCESS;
    ObpCreateTypeObject(&ObjectTypeInitializer, &Name, &ExProfileObjectType);
}

NTSTATUS
NTAPI
NtCreateProfile(OUT PHANDLE ProfileHandle,
                IN HANDLE Process OPTIONAL,
                IN PVOID ImageBase,
                IN ULONG ImageSize,
                IN ULONG BucketSize,
                IN PVOID Buffer,
                IN ULONG BufferSize,
                IN KPROFILE_SOURCE ProfileSource,
                IN KAFFINITY Affinity)
{
    HANDLE hProfile;
    PEPROFILE Profile;
    PEPROCESS pProcess;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Segment = 0, Log2 = 0;
    PAGED_CODE();

    /* Easy way out */
    if(!BufferSize) return STATUS_INVALID_PARAMETER_7;

    /* Check if this is a low-memory profile */
    if ((!BucketSize) && (ImageBase < (PVOID)(0x10000)))
    {
        /* Validate size */
        if (BufferSize < sizeof(ULONG)) return STATUS_INVALID_PARAMETER_7;

        /* This will become a segmented profile object */
        Segment = (ULONG)ImageBase;
        ImageBase = 0;

        /* Recalculate the bucket size */
        BucketSize = ImageSize / (BufferSize / sizeof(ULONG));

        /* Convert it to log2 */
        BucketSize--;
        while (BucketSize >>= 1) Log2++;
        BucketSize += Log2 + 1;
    }

    /* Validate bucket size */
    if ((BucketSize > 31) || (BucketSize < 2))
    {
        DPRINT1("Bucket size invalid\n");
        return STATUS_INVALID_PARAMETER;
    }

    /* Make sure that the buckets can map the range */
    if ((ImageSize >> (BucketSize - 2)) > BufferSize)
    {
        DPRINT1("Bucket size too small\n");
        return STATUS_BUFFER_TOO_SMALL;
    }

    /* Make sure that the range isn't too gigantic */
    if (((ULONG_PTR)ImageBase + ImageSize) < ImageSize)
    {
        DPRINT1("Range too big\n");
        return STATUS_BUFFER_OVERFLOW;
    }

    /* Check if we were called from user-mode */
    if(PreviousMode != KernelMode)
    {
        /* Entry SEH */
        _SEH_TRY
        {
            /* Make sure that the handle pointer is valid */
            ProbeForWriteHandle(ProfileHandle);

            /* Check if the buffer is valid */
            ProbeForWrite(Buffer,
                          BufferSize,
                          sizeof(ULONG));
        }
        _SEH_EXCEPT(_SEH_ExSystemExceptionFilter)
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        /* Bail out if we failed */
        if(!NT_SUCCESS(Status)) return Status;
    }

    /* Check if a process was specified */
    if (Process)
    {
        /* Reference it */
        Status = ObReferenceObjectByHandle(Process,
                                           PROCESS_QUERY_INFORMATION,
                                           PsProcessType,
                                           PreviousMode,
                                           (PVOID*)&pProcess,
                                           NULL);
        if (!NT_SUCCESS(Status)) return(Status);
    }
    else
    {
        /* Segmented profile objects cannot be used system-wide */
        if (Segment) return STATUS_INVALID_PARAMETER;

        /* No process was specified, which means a System-Wide Profile */
        pProcess = NULL;

        /* For this, we need to check the Privilege */
        if(!SeSinglePrivilegeCheck(SeSystemProfilePrivilege, PreviousMode))
        {
            DPRINT1("NtCreateProfile: Caller requires the SeSystemProfilePrivilege privilege!\n");
            return STATUS_PRIVILEGE_NOT_HELD;
        }
    }

    /* Create the object */
    InitializeObjectAttributes(&ObjectAttributes,
                               NULL,
                               0,
                               NULL,
                               NULL);
    Status = ObCreateObject(KernelMode,
                            ExProfileObjectType,
                            &ObjectAttributes,
                            PreviousMode,
                            NULL,
                            sizeof(EPROFILE),
                            0,
                            sizeof(EPROFILE) + sizeof(KPROFILE),
                            (PVOID*)&Profile);
    if (!NT_SUCCESS(Status)) return(Status);

    /* Initialize it */
    Profile->ImageBase = ImageBase;
    Profile->ImageSize = ImageSize;
    Profile->Buffer = Buffer;
    Profile->BufferSize = BufferSize;
    Profile->BucketSize = BucketSize;
    Profile->LockedBuffer = NULL;
    Profile->Segment = Segment;
    Profile->ProfileSource = ProfileSource;
    Profile->Affinity = Affinity;
    Profile->Process = pProcess;

    /* Insert into the Object Tree */
    Status = ObInsertObject ((PVOID)Profile,
                             NULL,
                             PROFILE_CONTROL,
                             0,
                             NULL,
                             &hProfile);
    ObDereferenceObject(Profile);

    /* Check for Success */
    if (!NT_SUCCESS(Status))
    {
        /* Dereference Process on failure */
        if (pProcess) ObDereferenceObject(pProcess);
        return Status;
    }

    /* Enter SEH */
    _SEH_TRY
    {
        /* Copy the created handle back to the caller*/
        *ProfileHandle = hProfile;
    }
    _SEH_EXCEPT(_SEH_ExSystemExceptionFilter)
    {
        Status = _SEH_GetExceptionCode();
    }
    _SEH_END;

    /* Return Status */
    return Status;
}

NTSTATUS
NTAPI
NtQueryPerformanceCounter(OUT PLARGE_INTEGER PerformanceCounter,
                          OUT PLARGE_INTEGER PerformanceFrequency OPTIONAL)
{
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    LARGE_INTEGER PerfFrequency;
    NTSTATUS Status = STATUS_SUCCESS;

    /* Check if we were called from user-mode */
    if(PreviousMode != KernelMode)
    {
        /* Entry SEH Block */
        _SEH_TRY
        {
            /* Make sure the counter and frequency are valid */
            ProbeForWriteLargeInteger(PerformanceCounter);
            if (PerformanceFrequency)
            {
                ProbeForWriteLargeInteger(PerformanceFrequency);
            }
        }
        _SEH_EXCEPT(_SEH_ExSystemExceptionFilter)
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        /* If the pointers are invalid, bail out */
        if(!NT_SUCCESS(Status)) return Status;
    }

    /* Enter a new SEH Block */
    _SEH_TRY
    {
        /* Query the Kernel */
        *PerformanceCounter = KeQueryPerformanceCounter(&PerfFrequency);

        /* Return Frequency if requested */
        if(PerformanceFrequency) *PerformanceFrequency = PerfFrequency;
    }
    _SEH_EXCEPT(_SEH_ExSystemExceptionFilter)
    {
        Status = _SEH_GetExceptionCode();
    }
    _SEH_END;

    /* Return status to caller */
    return Status;
}

NTSTATUS
NTAPI
NtStartProfile(IN HANDLE ProfileHandle)
{
    PEPROFILE Profile;
    PKPROFILE KeProfile;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    PVOID TempLockedBuffer;
    NTSTATUS Status;
    PAGED_CODE();

    /* Get the Object */
    Status = ObReferenceObjectByHandle(ProfileHandle,
                                       PROFILE_CONTROL,
                                       ExProfileObjectType,
                                       PreviousMode,
                                       (PVOID*)&Profile,
                                       NULL);
    if (!NT_SUCCESS(Status)) return(Status);

    /* To avoid a Race, wait on the Mutex */
    KeWaitForSingleObject(&ExpProfileMutex,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);

    /* The Profile can still be enabled though, so handle that */
    if (Profile->LockedBuffer)
    {
        /* Release our lock, dereference and return */
        KeReleaseMutex(&ExpProfileMutex, FALSE);
        ObDereferenceObject(Profile);
        return STATUS_PROFILING_NOT_STOPPED;
    }

    /* Allocate a Kernel Profile Object. */
    KeProfile = ExAllocatePoolWithTag(NonPagedPool,
                                      sizeof(EPROFILE),
                                      TAG_PROFILE);

    /* Allocate the Mdl Structure */
    Profile->Mdl = MmCreateMdl(NULL, Profile->Buffer, Profile->BufferSize);

    /* Probe and Lock for Write Access */
    MmProbeAndLockPages(Profile->Mdl, PreviousMode, IoWriteAccess);

    /* Map the pages */
    TempLockedBuffer = MmMapLockedPages(Profile->Mdl, KernelMode);

    /* Initialize the Kernel Profile Object */
    Profile->KeProfile = KeProfile;
    KeInitializeProfile(KeProfile,
                        (PKPROCESS)Profile->Process,
                        Profile->ImageBase,
                        Profile->ImageSize,
                        Profile->BucketSize,
                        Profile->ProfileSource,
                        Profile->Affinity);

    /* Start the Profiling */
    KeStartProfile(KeProfile, TempLockedBuffer);

    /* Now it's safe to save this */
    Profile->LockedBuffer = TempLockedBuffer;

    /* Release mutex, dereference and return */
    KeReleaseMutex(&ExpProfileMutex, FALSE);
    ObDereferenceObject(Profile);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
NtStopProfile(IN HANDLE ProfileHandle)
{
    PEPROFILE Profile;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status;
    PAGED_CODE();

    /* Get the Object */
    Status = ObReferenceObjectByHandle(ProfileHandle,
                                       PROFILE_CONTROL,
                                       ExProfileObjectType,
                                       PreviousMode,
                                       (PVOID*)&Profile,
                                       NULL);
    if (!NT_SUCCESS(Status)) return(Status);

    /* Get the Mutex */
    KeWaitForSingleObject(&ExpProfileMutex,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);

    /* Make sure the Profile Object is really Started */
    if (!Profile->LockedBuffer)
    {
        Status = STATUS_PROFILING_NOT_STARTED;
        goto Exit;
    }

    /* Stop the Profile */
    KeStopProfile(Profile->KeProfile);

    /* Unlock the Buffer */
    MmUnmapLockedPages(Profile->LockedBuffer, Profile->Mdl);
    MmUnlockPages(Profile->Mdl);
    ExFreePool(Profile->KeProfile);

    /* Clear the Locked Buffer pointer, meaning the Object is Stopped */
    Profile->LockedBuffer = NULL;

Exit:
    /* Release Mutex, Dereference and Return */
    KeReleaseMutex(&ExpProfileMutex, FALSE);
    ObDereferenceObject(Profile);
    return Status;
}

NTSTATUS
NTAPI
NtQueryIntervalProfile(IN KPROFILE_SOURCE ProfileSource,
                       OUT PULONG Interval)
{
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    ULONG ReturnInterval;
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();

    /* Check if we were called from user-mode */
    if(PreviousMode != KernelMode)
    {
        /* Enter SEH Block */
        _SEH_TRY
        {
            /* Validate interval */
            ProbeForWriteUlong(Interval);
        }
        _SEH_EXCEPT(_SEH_ExSystemExceptionFilter)
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        /* If pointer was invalid, bail out */
        if(!NT_SUCCESS(Status)) return Status;
    }

    /* Query the Interval */
    ReturnInterval = KeQueryIntervalProfile(ProfileSource);

    /* Enter SEH block for return */
    _SEH_TRY
    {
        /* Return the data */
        *Interval = ReturnInterval;
    }
    _SEH_EXCEPT(_SEH_ExSystemExceptionFilter)
    {
        Status = _SEH_GetExceptionCode();
    }
    _SEH_END;

    /* Return Success */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
NtSetIntervalProfile(IN ULONG Interval,
                     IN KPROFILE_SOURCE Source)
{
    /* Let the Kernel do the job */
    KeSetIntervalProfile(Interval, Source);

    /* Nothing can go wrong */
    return STATUS_SUCCESS;
}
