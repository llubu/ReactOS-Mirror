/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ps/security.c
 * PURPOSE:         Process Manager: Process/Thread Security
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Eric Kohl
 *                  Thomas Weidenmueller (w3seek@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* PRIVATE FUNCTIONS *********************************************************/

VOID
NTAPI
PspDeleteProcessSecurity(IN PEPROCESS Process)
{
    PAGED_CODE();
    PSTRACE(PS_SECURITY_DEBUG, "Process: %p\n", Process);

    /* Check if we have a token */
    if (Process->Token.Object)
    {
        /* Deassign it */
        SeDeassignPrimaryToken(Process);
        Process->Token.Object = NULL;
    }
}

VOID
NTAPI
PspDeleteThreadSecurity(IN PETHREAD Thread)
{
    PAGED_CODE();
    PSTRACE(PS_SECURITY_DEBUG, "Thread: %p\n", Thread);

    /* Check if we have active impersonation info */
    if (Thread->ActiveImpersonationInfo)
    {
        /* Dereference its token */
        ObDereferenceObject(Thread->ImpersonationInfo->Token);
    }

    /* Check if we have impersonation info */
    if (Thread->ImpersonationInfo)
    {
        /* Free it */
        ExFreePool(Thread->ImpersonationInfo);
        PspClearCrossThreadFlag(Thread, CT_ACTIVE_IMPERSONATION_INFO_BIT);
        Thread->ImpersonationInfo = NULL;
    }
}

NTSTATUS
NTAPI
PspInitializeProcessSecurity(IN PEPROCESS Process,
                             IN PEPROCESS Parent OPTIONAL)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PTOKEN NewToken, ParentToken;
    PAGED_CODE();
    PSTRACE(PS_SECURITY_DEBUG, "Process: %p\n", Process);

    /* If we have a parent, then duplicate the Token */
    if (Parent)
    {
        /* Get the Parent Token */
        ParentToken = PsReferencePrimaryToken(Parent);

        /* Duplicate it */
        Status = SeSubProcessToken(ParentToken,
                                   &NewToken,
                                   TRUE,
                                   0);//MmGetSessionId(Process));

        /* Dereference the Parent */
        ObFastDereferenceObject(&Parent->Token, ParentToken);

        /* Set the new Token */
        ObInitializeFastReference(&Process->Token, NewToken);
    }
    else
    {
#ifdef SCHED_REWRITE
        /* No parent, assign the Boot Token */
        ObInitializeFastReference(&Process->Token, NULL);
        SeAssignPrimaryToken(Process, PspBootAccessToken);
#else
        DPRINT1("PspInitializeProcessSecurity called with no parent.\n");
#endif
    }

    /* Return to caller */
    return Status;
}

NTSTATUS
NTAPI
PspWriteTebImpersonationInfo(IN PETHREAD Thread,
                             IN PETHREAD CurrentThread)
{
    PEPROCESS Process;
    PTEB Teb;
    BOOLEAN Attached = FALSE;
    BOOLEAN IsImpersonating;
    KAPC_STATE ApcState;
    PAGED_CODE();
    PSTRACE(PS_SECURITY_DEBUG, "Thread: %p\n", Thread);

    /* Sanity check */
    ASSERT(CurrentThread == PsGetCurrentThread());

    /* Get process and TEB */
    Process = Thread->ThreadsProcess;
    Teb = Thread->Tcb.Teb;
    if (Teb)
    {
        /* Check if we're not in the right process */
        if (Thread->Tcb.ApcState.Process != &Process->Pcb)
        {
            /* Attach to the process */
            KeStackAttachProcess(&Process->Pcb, &ApcState);
            Attached = TRUE;
        }

        /* Check if we're in a different thread */
        if (Thread != CurrentThread)
        {
            /* Acquire thread rundown protection */
            ExAcquireRundownProtection(&Thread->RundownProtect);
        }

        /* Check if the thread is impersonating */
        IsImpersonating = Thread->ActiveImpersonationInfo;
        if (IsImpersonating)
        {
            /* Set TEB data */
            Teb->ImpersonationLocale = -1;
            Teb->IsImpersonating = 1;
        }
        else
        {
            /* Set TEB data */
            Teb->ImpersonationLocale = 0;
            Teb->IsImpersonating = 0;
        }

        /* Check if we're in a different thread */
        if (Thread != CurrentThread)
        {
            /* Release protection */
            ExReleaseRundownProtection(&Thread->RundownProtect);
        }

        /* Dettach */
        if (Attached) KeUnstackDetachProcess(&ApcState);
    }

    /* Return to caller */
    return STATUS_SUCCESS;
}


NTSTATUS
NTAPI
PspAssignPrimaryToken(IN PEPROCESS Process,
                      IN PTOKEN Token)
{
    PACCESS_TOKEN OldToken;
    NTSTATUS Status;
    PAGED_CODE();
    PSTRACE(PS_SECURITY_DEBUG, "Process: %p Token: %p\n", Process, Token);

    /* Lock the process */
    PspLockProcessSecurityExclusive(Process);

    /* Exchange them */
    Status = SeExchangePrimaryToken(Process, Token, &OldToken);

    /* Release the lock */
    PspUnlockProcessSecurityExclusive(Process);

    /* Dereference Tokens and Return */
    if (NT_SUCCESS(Status)) ObDereferenceObject(OldToken);
    ObDereferenceObject(Token);
    return Status;
}

NTSTATUS
NTAPI
PspSetPrimaryToken(IN PEPROCESS Process,
                   IN HANDLE TokenHandle OPTIONAL,
                   IN PTOKEN Token OPTIONAL)
{
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    BOOLEAN IsChild;
    NTSTATUS Status, AccessStatus;
    BOOLEAN Result, SdAllocated;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    SECURITY_SUBJECT_CONTEXT SubjectContext;
    PSTRACE(PS_SECURITY_DEBUG, "Process: %p Token: %p\n", Process, Token);

    /* Make sure we got a handle */
    if (TokenHandle)
    {
        /* Reference it */
        Status = ObReferenceObjectByHandle(TokenHandle,
                                           TOKEN_ASSIGN_PRIMARY,
                                           SepTokenObjectType,
                                           PreviousMode,
                                           (PVOID*)&Token,
                                           NULL);
        if (!NT_SUCCESS(Status)) return Status;
    }

    /* Check if this is a child */
    Status = SeIsTokenChild(Token, &IsChild);
    if (!NT_SUCCESS(Status))
    {
        /* Failed, dereference */
        if (TokenHandle) ObDereferenceObject(Token);
        return Status;
    }

    /* Check if this was an independent token */
    if (!IsChild)
    {
        /* Make sure we have the privilege to assign a new one */
        if (!SeSinglePrivilegeCheck(SeAssignPrimaryTokenPrivilege,
                                    PreviousMode))
        {
            /* Failed, dereference */
            if (TokenHandle) ObDereferenceObject(Token);
            return STATUS_PRIVILEGE_NOT_HELD;
        }
    }

    /* Assign the token */
    Status = PspAssignPrimaryToken(Process, Token);
    if (NT_SUCCESS(Status))
    {
        /*
         * We need to completely reverify if the process still has access to
         * itself under this new token.
         */
        Status = ObGetObjectSecurity(Process,
                                     &SecurityDescriptor,
                                     &SdAllocated);
        if (NT_SUCCESS(Status))
        {
            /* Setup the security context */
            SubjectContext.ProcessAuditId = Process;
            SubjectContext.PrimaryToken = PsReferencePrimaryToken(Process);
            SubjectContext.ClientToken = NULL;

            /* Do the access check */
            if (!SecurityDescriptor) DPRINT1("FIX PS SDs!!\n");
            Result = SeAccessCheck(SecurityDescriptor,
                                   &SubjectContext,
                                   FALSE,
                                   MAXIMUM_ALLOWED,
                                   0,
                                   NULL,
                                   &PsProcessType->TypeInfo.GenericMapping,
                                   PreviousMode,
                                   &Process->GrantedAccess,
                                   &AccessStatus);

            /* Dereference the token and let go the SD */
            ObFastDereferenceObject(&Process->Token,
                                    SubjectContext.PrimaryToken);
            ObReleaseObjectSecurity(SecurityDescriptor, SdAllocated);

            /* Remove access if it failed */
            if (!Result) Process->GrantedAccess = 0;
        }

        /* Dereference the process */
        ObDereferenceObject(Process);
    }

    /* Dereference the token */
    if (TokenHandle) ObDereferenceObject(Token);
    return Status;
}

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtOpenProcessToken(IN HANDLE ProcessHandle,
                   IN ACCESS_MASK DesiredAccess,
                   OUT PHANDLE TokenHandle)
{
    /* Call the newer API */
    return NtOpenProcessTokenEx(ProcessHandle,
                                DesiredAccess,
                                0,
                                TokenHandle);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtOpenProcessTokenEx(IN HANDLE ProcessHandle,
                     IN ACCESS_MASK DesiredAccess,
                     IN ULONG HandleAttributes,
                     OUT PHANDLE TokenHandle)
{
    PACCESS_TOKEN Token;
    HANDLE hToken;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();
    PSTRACE(PS_SECURITY_DEBUG,
            "Process: %p DesiredAccess: %lx\n", ProcessHandle, DesiredAccess);

    /* Check if caller was user-mode */
    if (PreviousMode != KernelMode)
    {
        /* Enter SEH for probing */
        _SEH_TRY
        {
            /* Probe the token handle */
            ProbeForWriteHandle(TokenHandle);
        }
        _SEH_HANDLE
        {
            /* Get the exception code */
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        /* Fail on exception */
        if (!NT_SUCCESS(Status)) return Status;
    }

    /* Open the process token */
    Status = PsOpenTokenOfProcess(ProcessHandle, &Token);
    if(NT_SUCCESS(Status))
    {
        /* Reference it by handle and dereference the pointer */
        Status = ObOpenObjectByPointer(Token,
                                       HandleAttributes,
                                       NULL,
                                       DesiredAccess,
                                       SepTokenObjectType,
                                       PreviousMode,
                                       &hToken);
        ObDereferenceObject(Token);

        /* Make sure we got a handle */
        if(NT_SUCCESS(Status))
        {
            /* Enter SEH for write */
            _SEH_TRY
            {
                /* Return the handle */
                *TokenHandle = hToken;
            }
            _SEH_HANDLE
            {
                /* Get exception code */
                Status = _SEH_GetExceptionCode();
            }
            _SEH_END;
        }
    }

    /* Return status */
    return Status;
}

/*
 * @implemented
 */
PACCESS_TOKEN
NTAPI
PsReferencePrimaryToken(PEPROCESS Process)
{
    PACCESS_TOKEN Token;
    PAGED_CODE();
    PSTRACE(PS_SECURITY_DEBUG, "Process: %p\n", Process);

    /* Fast Reference the Token */
    Token = ObFastReferenceObject(&Process->Token);

    /* Check if we got the Token or if we got locked */
    if (!Token)
    {
        /* Lock the Process */
        PspLockProcessSecurityShared(Process);

        /* Do a Locked Fast Reference */
        Token = ObFastReferenceObjectLocked(&Process->Token);

        /* Unlock the Process */
        PspUnlockProcessSecurityShared(Process);
    }

    /* Return the Token */
    return Token;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
PsOpenTokenOfProcess(IN HANDLE ProcessHandle,
                     OUT PACCESS_TOKEN* Token)
{
    PEPROCESS Process;
    NTSTATUS Status;
    PAGED_CODE();
    PSTRACE(PS_SECURITY_DEBUG, "Process: %p\n", ProcessHandle);

    /* Get the Token */
    Status = ObReferenceObjectByHandle(ProcessHandle,
                                       PROCESS_QUERY_INFORMATION,
                                       PsProcessType,
                                       ExGetPreviousMode(),
                                       (PVOID*)&Process,
                                       NULL);
    if (NT_SUCCESS(Status))
    {
        /* Reference the token and dereference the process */
        *Token = PsReferencePrimaryToken(Process);
        ObDereferenceObject(Process);
    }

    /* Return */
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
PsAssignImpersonationToken(IN PETHREAD Thread,
                           IN HANDLE TokenHandle)
{
    PACCESS_TOKEN Token;
    SECURITY_IMPERSONATION_LEVEL ImpersonationLevel;
    NTSTATUS Status;
    PAGED_CODE();
    PSTRACE(PS_SECURITY_DEBUG, "Thread: %p Token: %p\n", Thread, TokenHandle);

    /* Check if we were given a handle */
    if (!TokenHandle)
    {
        /* Undo impersonation */
        PsRevertThreadToSelf(Thread);
        return STATUS_SUCCESS;
    }

    /* Get the token object */
    Status = ObReferenceObjectByHandle(TokenHandle,
                                       TOKEN_IMPERSONATE,
                                       SepTokenObjectType,
                                       KeGetPreviousMode(),
                                       (PVOID*)&Token,
                                       NULL);
    if (!NT_SUCCESS(Status)) return(Status);

    /* Make sure it's an impersonation token */
    if (SeTokenType(Token) != TokenImpersonation)
    {
        /* Fail */
        ObDereferenceObject(Token);
        return STATUS_BAD_TOKEN_TYPE;
    }

    /* Check if this is a job, which we don't support yet */
    if (Thread->ThreadsProcess->Job) KEBUGCHECK(0);

    /* Get the impersionation level */
    ImpersonationLevel = SeTokenImpersonationLevel(Token);

    /* Call the impersonation API */
    Status = PsImpersonateClient(Thread,
                                 Token,
                                 FALSE,
                                 FALSE,
                                 ImpersonationLevel);

    /* Dereference the token and return status */
    if (Token) ObDereferenceObject(Token);
    return Status;
}

/*
 * @implemented
 */
VOID
NTAPI
PsRevertToSelf(VOID)
{
    /* Call the per-thread API */
    PAGED_CODE();
    PsRevertThreadToSelf(PsGetCurrentThread());
}

/*
 * @implemented
 */
VOID
NTAPI
PsRevertThreadToSelf(IN PETHREAD Thread)
{
    PTOKEN Token = NULL;
    PAGED_CODE();
    PSTRACE(PS_SECURITY_DEBUG, "Thread: %p\n", Thread);

    /* Make sure we had impersonation information */
    if (Thread->ActiveImpersonationInfo)
    {
        /* Lock the thread security */
        PspLockThreadSecurityExclusive(Thread);

        /* Make sure it's still active */
        if (Thread->ActiveImpersonationInfo)
        {
            /* Disable impersonation */
            PspClearCrossThreadFlag(Thread, CT_ACTIVE_IMPERSONATION_INFO_BIT);

            /* Get the token */
            Token = Thread->ImpersonationInfo->Token;
        }

        /* Release thread security */
        PspUnlockThreadSecurityExclusive(Thread);
    }

    /* Dereference the impersonation token */
    if (Token) ObDereferenceObject(Token);

    /* Write impersonation info to the TEB */
    PspWriteTebImpersonationInfo(Thread, PsGetCurrentThread());
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
PsImpersonateClient(IN PETHREAD Thread,
                    IN PACCESS_TOKEN Token,
                    IN BOOLEAN CopyOnOpen,
                    IN BOOLEAN EffectiveOnly,
                    IN SECURITY_IMPERSONATION_LEVEL ImpersonationLevel)
{
    PPS_IMPERSONATION_INFORMATION Impersonation;
    PTOKEN OldToken = NULL;
    PAGED_CODE();
    PSTRACE(PS_SECURITY_DEBUG, "Thread: %p, Token: %p\n", Thread, Token);

    /* Check if we don't have a token */
    if (!Token)
    {
        /* Make sure we're impersonating */
        if (Thread->ActiveImpersonationInfo)
        {
            /* We seem to be, lock the thread */
            PspLockThreadSecurityExclusive(Thread);

            /* Make sure we're still impersonating */
            if (Thread->ActiveImpersonationInfo)
            {
                /* Disable impersonation */
                PspClearCrossThreadFlag(Thread,
                                        CT_ACTIVE_IMPERSONATION_INFO_BIT);

                /* Get the token */
                OldToken = Thread->ImpersonationInfo->Token;
            }

            /* Unlock the process */
            PspUnlockThreadSecurityExclusive(Thread);
        }
    }
    else
    {
        /* Check if we have impersonation info */
        Impersonation = Thread->ImpersonationInfo;
        if (!Impersonation)
        {
            /* We need to allocate a new one */
            Impersonation = ExAllocatePoolWithTag(PagedPool,
                                                  sizeof(*Impersonation),
                                                  TAG_PS_IMPERSONATION);
            if (!Impersonation) return STATUS_INSUFFICIENT_RESOURCES;

            /* Update the pointer */
            if (InterlockedCompareExchangePointer(&Thread->ImpersonationInfo,
                                                  Impersonation,
                                                  NULL))
            {
                /* Someone beat us to it, free our copy */
                ExFreePool(Impersonation);
            }
        }

        /* Check if this is a job, which we don't support yet */
        if (Thread->ThreadsProcess->Job) KEBUGCHECK(0);

        /* Lock thread security */
        PspLockThreadSecurityExclusive(Thread);

        /* Check if we're impersonating */
        if (Thread->ActiveImpersonationInfo)
        {
            /* Get the token */
            OldToken = Impersonation->Token;
        }
        else
        {
            /* Otherwise, enable impersonation */
            PspSetCrossThreadFlag(Thread, CT_ACTIVE_IMPERSONATION_INFO_BIT);
        }

        /* Now fill it out */
        Impersonation->ImpersonationLevel = ImpersonationLevel;
        Impersonation->CopyOnOpen = CopyOnOpen;
        Impersonation->EffectiveOnly = EffectiveOnly;
        Impersonation->Token = Token;
        ObReferenceObject(Token);

        /* Unlock the thread */
        PspUnlockThreadSecurityExclusive(Thread);
    }

    /* Write impersonation info to the TEB */
    PspWriteTebImpersonationInfo(Thread, PsGetCurrentThread());

    /* Dereference the token and return success */
    if (OldToken) ObDereferenceObject(OldToken);
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
PACCESS_TOKEN
NTAPI
PsReferenceEffectiveToken(IN PETHREAD Thread,
                          OUT IN PTOKEN_TYPE TokenType,
                          OUT PBOOLEAN EffectiveOnly,
                          OUT PSECURITY_IMPERSONATION_LEVEL Level)
{
    PEPROCESS Process;
    PACCESS_TOKEN Token = NULL;
    PAGED_CODE();
    PSTRACE(PS_SECURITY_DEBUG,
            "Thread: %p, TokenType: %p\n", Thread, TokenType);

    /* Check if we don't have impersonation info */
    Process = Thread->ThreadsProcess;
    if (!Thread->ActiveImpersonationInfo)
    {
        *TokenType = TokenPrimary;
        *EffectiveOnly = FALSE;

        /* Fast Reference the Token */
        Token = ObFastReferenceObject(&Process->Token);

        /* Check if we got the Token or if we got locked */
        if (!Token)
        {
            /* Lock the Process */
            PspLockProcessSecurityShared(Process);

            /* Do a Locked Fast Reference */
            Token = ObFastReferenceObjectLocked(&Process->Token);

            /* Unlock the Process */
            PspUnlockProcessSecurityShared(Process);
        }
    }
    else
    {
        /* Lock the Process */
        PspLockProcessSecurityShared(Process);

        /* Make sure impersonation is still active */
        if (Thread->ActiveImpersonationInfo)
        {
            /* Get the token */
            Token = Thread->ImpersonationInfo->Token;
            ObReferenceObject(Token);

            /* Return data to caller */
            *TokenType = TokenImpersonation;
            *EffectiveOnly = Thread->ImpersonationInfo->EffectiveOnly;
            *Level = Thread->ImpersonationInfo->ImpersonationLevel;
        }

        /* Unlock the Process */
        PspUnlockProcessSecurityShared(Process);
    }

    /* Return the token */
    return Token;
}

/*
 * @implemented
 */
PACCESS_TOKEN
NTAPI
PsReferenceImpersonationToken(IN PETHREAD Thread,
                              OUT PBOOLEAN CopyOnOpen,
                              OUT PBOOLEAN EffectiveOnly,
                              OUT PSECURITY_IMPERSONATION_LEVEL ImpersonationLevel)
{
    PTOKEN Token = NULL;
    PAGED_CODE();
    PSTRACE(PS_SECURITY_DEBUG, "Thread: %p\n", Thread);

    /* If we don't have impersonation info, just quit */
    if (!Thread->ActiveImpersonationInfo) return NULL;

    /* Lock the thread */
    PspLockThreadSecurityShared(Thread);

    /* Make sure we still have active impersonation */
    if (Thread->ActiveImpersonationInfo)
    {
        /* Return data from caller */
        ObReferenceObject(Thread->ImpersonationInfo->Token);
        *ImpersonationLevel = Thread->ImpersonationInfo->ImpersonationLevel;
        *CopyOnOpen = Thread->ImpersonationInfo->CopyOnOpen;
        *EffectiveOnly = Thread->ImpersonationInfo->EffectiveOnly;

        /* Set the token */
        Token = Thread->ImpersonationInfo->Token;
    }

    /* Unlock thread and return impersonation token */
    PspUnlockThreadSecurityShared(Thread);
    return Token;
}

#undef PsDereferenceImpersonationToken
/*
 * @implemented
 */
VOID
NTAPI
PsDereferenceImpersonationToken(IN PACCESS_TOKEN ImpersonationToken)
{
    PAGED_CODE();

    /* If we got a token, dereference it */
    if (ImpersonationToken) ObDereferenceObject(ImpersonationToken);
}

#undef PsDereferencePrimaryToken
/*
 * @implemented
 */
VOID
NTAPI
PsDereferencePrimaryToken(IN PACCESS_TOKEN PrimaryToken)
{
    PAGED_CODE();

    /* Dereference the token*/
    ObDereferenceObject(PrimaryToken);
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
PsDisableImpersonation(IN PETHREAD Thread,
                       IN PSE_IMPERSONATION_STATE ImpersonationState)
{
    PPS_IMPERSONATION_INFORMATION Impersonation = NULL;
    LONG NewValue, OldValue;
    PAGED_CODE();
    PSTRACE(PS_SECURITY_DEBUG,
            "Thread: %p State: %p\n", Thread, ImpersonationState);

    /* Check if we don't have impersonation */
    if (Thread->ActiveImpersonationInfo)
    {
        /* Lock thread security */
        PspLockThreadSecurityExclusive(Thread);

        /* Disable impersonation */
        OldValue = Thread->CrossThreadFlags;
        do
        {
            /* Attempt to change the flag */
            NewValue =
                InterlockedCompareExchange((PLONG)&Thread->CrossThreadFlags,
                                           OldValue &~
                                           CT_ACTIVE_IMPERSONATION_INFO_BIT,
                                           OldValue);
        } while (NewValue != OldValue);

        /* Make sure nobody disabled it behind our back */
        if (NewValue & CT_ACTIVE_IMPERSONATION_INFO_BIT)
        {
            /* Copy the old state */
            Impersonation = Thread->ImpersonationInfo;
            ImpersonationState->Token = Impersonation->Token;
            ImpersonationState->CopyOnOpen = Impersonation->CopyOnOpen;
            ImpersonationState->EffectiveOnly = Impersonation->EffectiveOnly;
            ImpersonationState->Level = Impersonation->ImpersonationLevel;
        }

        /* Unlock thread security */
        PspUnlockThreadSecurityExclusive(Thread);

        /* If we had impersonation info, return true */
        if (Impersonation) return TRUE;
    }

    /* Clear everything */
    ImpersonationState->Token = NULL;
    ImpersonationState->CopyOnOpen = FALSE;
    ImpersonationState->EffectiveOnly = FALSE;
    ImpersonationState->Level = SecurityAnonymous;
    return FALSE;
}

/*
 * @implemented
 */
VOID
NTAPI
PsRestoreImpersonation(IN PETHREAD Thread,
                       IN PSE_IMPERSONATION_STATE ImpersonationState)
{
    PTOKEN Token = NULL;
    PPS_IMPERSONATION_INFORMATION Impersonation;
    PAGED_CODE();
    PSTRACE(PS_SECURITY_DEBUG,
            "Thread: %p State: %p\n", Thread, ImpersonationState);

    /* Lock thread security */
    PspLockThreadSecurityExclusive(Thread);

    /* Get the impersonation info */
    Impersonation = Thread->ImpersonationInfo;

    /* Check if we're impersonating */
    if (Thread->ActiveImpersonationInfo)
    {
        /* Get the token */
        Token = Impersonation->Token;
    }

    /* Check if we have an impersonation state */
    if (ImpersonationState)
    {
        /* Fill out the impersonation info */
        Impersonation->ImpersonationLevel = ImpersonationState->Level;
        Impersonation->CopyOnOpen = ImpersonationState->CopyOnOpen;
        Impersonation->EffectiveOnly = ImpersonationState->EffectiveOnly;
        Impersonation->Token = ImpersonationState->Token;

        /* Enable impersonation */
        PspSetCrossThreadFlag(Thread, CT_ACTIVE_IMPERSONATION_INFO_BIT);
    }
    else
    {
        /* Disable impersonation */
        PspClearCrossThreadFlag(Thread, CT_ACTIVE_IMPERSONATION_INFO_BIT);
    }

    /* Unlock the thread */
    PspUnlockThreadSecurityExclusive(Thread);

    /* Dereference the token */
    if (Token) ObDereferenceObject(Token);
}

NTSTATUS
NTAPI
NtImpersonateThread(IN HANDLE ThreadHandle,
                    IN HANDLE ThreadToImpersonateHandle,
                    IN PSECURITY_QUALITY_OF_SERVICE SecurityQualityOfService)
{
    SECURITY_QUALITY_OF_SERVICE SafeServiceQoS;
    SECURITY_CLIENT_CONTEXT ClientContext;
    PETHREAD Thread;
    PETHREAD ThreadToImpersonate;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();
    PSTRACE(PS_SECURITY_DEBUG,
            "Threads: %p %p\n", ThreadHandle, ThreadToImpersonateHandle);

    /* Check if call came from user mode */
    if (PreviousMode != KernelMode)
    {
        /* Enter SEH for probing */
        _SEH_TRY
        {
            /* Probe QoS */
            ProbeForRead(SecurityQualityOfService,
                         sizeof(SECURITY_QUALITY_OF_SERVICE),
                         sizeof(ULONG));

            /* Capture it */
            SafeServiceQoS = *SecurityQualityOfService;
            SecurityQualityOfService = &SafeServiceQoS;
        }
        _SEH_HANDLE
        {
            /* Get exception status */
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        /* Fail on exception */
        if (!NT_SUCCESS(Status)) return Status;
    }

    /* Reference the thread */
    Status = ObReferenceObjectByHandle(ThreadHandle,
                                       THREAD_IMPERSONATE,
                                       PsThreadType,
                                       PreviousMode,
                                       (PVOID*)&Thread,
                                       NULL);
    if(NT_SUCCESS(Status))
    {
        /* Reference the impersonating thead */
        Status = ObReferenceObjectByHandle(ThreadToImpersonateHandle,
                                           THREAD_DIRECT_IMPERSONATION,
                                           PsThreadType,
                                           PreviousMode,
                                           (PVOID*)&ThreadToImpersonate,
                                           NULL);
        if(NT_SUCCESS(Status))
        {
            /* Create a client security context */
            Status = SeCreateClientSecurity(ThreadToImpersonate,
                                            SecurityQualityOfService,
                                            0,
                                            &ClientContext);
            if(NT_SUCCESS(Status))
            {
                /* Do the impersonation */
                SeImpersonateClient(&ClientContext, Thread);
                if(ClientContext.ClientToken)
                {
                    /* Dereference the client token if we had one */
                    ObDereferenceObject(ClientContext.ClientToken);
                }
            }

            /* Dereference the thread to impersonate */
            ObDereferenceObject(ThreadToImpersonate);
        }

        /* Dereference the main thread */
        ObDereferenceObject(Thread);
    }

    /* Return status */
    return Status;
}
/* EOF */
