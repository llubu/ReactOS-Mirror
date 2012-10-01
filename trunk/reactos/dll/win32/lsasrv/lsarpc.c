/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         Local Security Authority (LSA) Server
 * FILE:            reactos/dll/win32/lsasrv/lsarpc.h
 * PURPOSE:         RPC interface functions
 *
 * PROGRAMMERS:     Eric Kohl
 */

/* INCLUDES ****************************************************************/

#include "lsasrv.h"

WINE_DEFAULT_DEBUG_CHANNEL(lsasrv);


/* GLOBALS *****************************************************************/

static RTL_CRITICAL_SECTION PolicyHandleTableLock;


/* FUNCTIONS ***************************************************************/

VOID
LsarStartRpcServer(VOID)
{
    RPC_STATUS Status;

    RtlInitializeCriticalSection(&PolicyHandleTableLock);

    TRACE("LsarStartRpcServer() called\n");

    Status = RpcServerUseProtseqEpW(L"ncacn_np",
                                    10,
                                    L"\\pipe\\lsarpc",
                                    NULL);
    if (Status != RPC_S_OK)
    {
        WARN("RpcServerUseProtseqEpW() failed (Status %lx)\n", Status);
        return;
    }

    Status = RpcServerRegisterIf(lsarpc_v0_0_s_ifspec,
                                 NULL,
                                 NULL);
    if (Status != RPC_S_OK)
    {
        WARN("RpcServerRegisterIf() failed (Status %lx)\n", Status);
        return;
    }

    Status = RpcServerListen(1, 20, TRUE);
    if (Status != RPC_S_OK)
    {
        WARN("RpcServerListen() failed (Status %lx)\n", Status);
        return;
    }

    TRACE("LsarStartRpcServer() done\n");
}


void __RPC_USER LSAPR_HANDLE_rundown(LSAPR_HANDLE hHandle)
{

}


/* Function 0 */
NTSTATUS WINAPI LsarClose(
    LSAPR_HANDLE *ObjectHandle)
{
    PLSA_DB_OBJECT DbObject;
    NTSTATUS Status = STATUS_SUCCESS;

    TRACE("0x%p\n", ObjectHandle);

//    RtlEnterCriticalSection(&PolicyHandleTableLock);

    Status = LsapValidateDbObject(*ObjectHandle,
                                  LsaDbIgnoreObject,
                                  0,
                                  &DbObject);
    if (Status == STATUS_SUCCESS)
    {
        Status = LsapCloseDbObject(DbObject);
        *ObjectHandle = NULL;
    }

//    RtlLeaveCriticalSection(&PolicyHandleTableLock);

    return Status;
}


/* Function 1 */
NTSTATUS WINAPI LsarDelete(
    LSAPR_HANDLE ObjectHandle)
{
    /* Deprecated */
    return STATUS_NOT_SUPPORTED;
}


/* Function 2 */
NTSTATUS WINAPI LsarEnumeratePrivileges(
    LSAPR_HANDLE PolicyHandle,
    DWORD *EnumerationContext,
    PLSAPR_PRIVILEGE_ENUM_BUFFER EnumerationBuffer,
    DWORD PreferedMaximumLength)
{
    PLSA_DB_OBJECT PolicyObject;
    NTSTATUS Status;

    TRACE("LsarEnumeratePrivileges(%p %p %p %lu)\n",
          PolicyHandle, EnumerationContext, EnumerationBuffer,
          PreferedMaximumLength);

    Status = LsapValidateDbObject(PolicyHandle,
                                  LsaDbPolicyObject,
                                  POLICY_VIEW_LOCAL_INFORMATION,
                                  &PolicyObject);
    if (!NT_SUCCESS(Status))
        return Status;

    if (EnumerationContext == NULL)
        return STATUS_INVALID_PARAMETER;

    return LsarpEnumeratePrivileges(EnumerationContext,
                                    EnumerationBuffer,
                                    PreferedMaximumLength);
}


/* Function 3 */
NTSTATUS WINAPI LsarQuerySecurityObject(
    LSAPR_HANDLE ObjectHandle,
    SECURITY_INFORMATION SecurityInformation,
    PLSAPR_SR_SECURITY_DESCRIPTOR *SecurityDescriptor)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 4 */
NTSTATUS WINAPI LsarSetSecurityObject(
    LSAPR_HANDLE ObjectHandle,
    SECURITY_INFORMATION SecurityInformation,
    PLSAPR_SR_SECURITY_DESCRIPTOR SecurityDescriptor)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 5 */
NTSTATUS WINAPI LsarChangePassword(
    handle_t IDL_handle,
    PRPC_UNICODE_STRING String1,
    PRPC_UNICODE_STRING String2,
    PRPC_UNICODE_STRING String3,
    PRPC_UNICODE_STRING String4,
    PRPC_UNICODE_STRING String5)
{
    /* Deprecated */
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 6 */
NTSTATUS WINAPI LsarOpenPolicy(
    LPWSTR SystemName,
    PLSAPR_OBJECT_ATTRIBUTES ObjectAttributes,
    ACCESS_MASK DesiredAccess,
    LSAPR_HANDLE *PolicyHandle)
{
    PLSA_DB_OBJECT PolicyObject;
    NTSTATUS Status;

    TRACE("LsarOpenPolicy called!\n");

    RtlEnterCriticalSection(&PolicyHandleTableLock);

    Status = LsapOpenDbObject(NULL,
                              NULL,
                              L"Policy",
                              LsaDbPolicyObject,
                              DesiredAccess,
                              &PolicyObject);

    RtlLeaveCriticalSection(&PolicyHandleTableLock);

    if (NT_SUCCESS(Status))
        *PolicyHandle = (LSAPR_HANDLE)PolicyObject;

    TRACE("LsarOpenPolicy done!\n");

    return Status;
}


/* Function 7 */
NTSTATUS WINAPI LsarQueryInformationPolicy(
    LSAPR_HANDLE PolicyHandle,
    POLICY_INFORMATION_CLASS InformationClass,
    PLSAPR_POLICY_INFORMATION *PolicyInformation)
{
    PLSA_DB_OBJECT DbObject;
    ACCESS_MASK DesiredAccess = 0;
    NTSTATUS Status;

    TRACE("LsarQueryInformationPolicy(%p,0x%08x,%p)\n",
          PolicyHandle, InformationClass, PolicyInformation);

    if (PolicyInformation)
    {
        TRACE("*PolicyInformation %p\n", *PolicyInformation);
    }

    switch (InformationClass)
    {
        case PolicyAuditLogInformation:
        case PolicyAuditEventsInformation:
        case PolicyAuditFullQueryInformation:
            DesiredAccess = POLICY_VIEW_AUDIT_INFORMATION;
            break;

        case PolicyPrimaryDomainInformation:
        case PolicyAccountDomainInformation:
        case PolicyLsaServerRoleInformation:
        case PolicyReplicaSourceInformation:
        case PolicyDefaultQuotaInformation:
        case PolicyModificationInformation:
        case PolicyDnsDomainInformation:
        case PolicyDnsDomainInformationInt:
        case PolicyLocalAccountDomainInformation:
            DesiredAccess = POLICY_VIEW_LOCAL_INFORMATION;
            break;

        case PolicyPdAccountInformation:
            DesiredAccess = POLICY_GET_PRIVATE_INFORMATION;
            break;

        default:
            ERR("Invalid InformationClass!\n");
            return STATUS_INVALID_PARAMETER;
    }

    Status = LsapValidateDbObject(PolicyHandle,
                                  LsaDbPolicyObject,
                                  DesiredAccess,
                                  &DbObject);
    if (!NT_SUCCESS(Status))
        return Status;

    switch (InformationClass)
    {
        case PolicyAuditLogInformation:      /* 1 */
            Status = LsarQueryAuditLog(PolicyHandle,
                                       PolicyInformation);

        case PolicyAuditEventsInformation:   /* 2 */
            Status = LsarQueryAuditEvents(PolicyHandle,
                                          PolicyInformation);
            break;

        case PolicyPrimaryDomainInformation: /* 3 */
            Status = LsarQueryPrimaryDomain(PolicyHandle,
                                            PolicyInformation);
            break;

        case PolicyPdAccountInformation:     /* 4 */
            Status = LsarQueryPdAccount(PolicyHandle,
                                        PolicyInformation);

        case PolicyAccountDomainInformation: /* 5 */
            Status = LsarQueryAccountDomain(PolicyHandle,
                                            PolicyInformation);
            break;

        case PolicyLsaServerRoleInformation: /* 6 */
            Status = LsarQueryServerRole(PolicyHandle,
                                         PolicyInformation);
            break;

        case PolicyReplicaSourceInformation: /* 7 */
            Status = LsarQueryReplicaSource(PolicyHandle,
                                            PolicyInformation);

        case PolicyDefaultQuotaInformation:  /* 8 */
            Status = LsarQueryDefaultQuota(PolicyHandle,
                                           PolicyInformation);
            break;

        case PolicyModificationInformation:  /* 9 */
            Status = LsarQueryModification(PolicyHandle,
                                           PolicyInformation);
            break;

        case PolicyAuditFullQueryInformation: /* 11 (0xB) */
            Status = LsarQueryAuditFull(PolicyHandle,
                                        PolicyInformation);
            break;

        case PolicyDnsDomainInformation:      /* 12 (0xC) */
            Status = LsarQueryDnsDomain(PolicyHandle,
                                        PolicyInformation);
            break;

        case PolicyDnsDomainInformationInt:   /* 13 (0xD) */
            Status = LsarQueryDnsDomainInt(PolicyHandle,
                                           PolicyInformation);
            break;

        case PolicyLocalAccountDomainInformation: /* 14 (0xE) */
            Status = LsarQueryLocalAccountDomain(PolicyHandle,
                                                 PolicyInformation);
            break;

        default:
            ERR("Invalid InformationClass!\n");
            Status = STATUS_INVALID_PARAMETER;
    }

    return Status;
}


/* Function 8 */
NTSTATUS WINAPI LsarSetInformationPolicy(
    LSAPR_HANDLE PolicyHandle,
    POLICY_INFORMATION_CLASS InformationClass,
    PLSAPR_POLICY_INFORMATION PolicyInformation)
{
    PLSA_DB_OBJECT DbObject;
    ACCESS_MASK DesiredAccess = 0;
    NTSTATUS Status;

    TRACE("LsarSetInformationPolicy(%p,0x%08x,%p)\n",
          PolicyHandle, InformationClass, PolicyInformation);

    if (PolicyInformation)
    {
        TRACE("*PolicyInformation %p\n", *PolicyInformation);
    }

    switch (InformationClass)
    {
        case PolicyAuditLogInformation:
        case PolicyAuditFullSetInformation:
            DesiredAccess = POLICY_AUDIT_LOG_ADMIN;
            break;

        case PolicyAuditEventsInformation:
            DesiredAccess = POLICY_SET_AUDIT_REQUIREMENTS;
            break;

        case PolicyPrimaryDomainInformation:
        case PolicyAccountDomainInformation:
        case PolicyDnsDomainInformation:
        case PolicyDnsDomainInformationInt:
        case PolicyLocalAccountDomainInformation:
            DesiredAccess = POLICY_TRUST_ADMIN;
            break;

        case PolicyLsaServerRoleInformation:
        case PolicyReplicaSourceInformation:
            DesiredAccess = POLICY_SERVER_ADMIN;
            break;

        case PolicyDefaultQuotaInformation:
            DesiredAccess = POLICY_SET_DEFAULT_QUOTA_LIMITS;
            break;

        default:
            ERR("Invalid InformationClass!\n");
            return STATUS_INVALID_PARAMETER;
    }

    Status = LsapValidateDbObject(PolicyHandle,
                                  LsaDbPolicyObject,
                                  DesiredAccess,
                                  &DbObject);
    if (!NT_SUCCESS(Status))
        return Status;

    switch (InformationClass)
    {
        case PolicyAuditEventsInformation:
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case PolicyPrimaryDomainInformation:
            Status = LsarSetPrimaryDomain(PolicyHandle,
                                          (PLSAPR_POLICY_PRIMARY_DOM_INFO)PolicyInformation);
            break;

        case PolicyAccountDomainInformation:
            Status = LsarSetAccountDomain(PolicyHandle,
                                          (PLSAPR_POLICY_ACCOUNT_DOM_INFO)PolicyInformation);
            break;

        case PolicyDnsDomainInformation:
            Status = LsarSetDnsDomain(PolicyHandle,
                                      (PLSAPR_POLICY_DNS_DOMAIN_INFO)PolicyInformation);
            break;

        case PolicyLsaServerRoleInformation:
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        default:
            Status = STATUS_INVALID_PARAMETER;
            break;
    }

    return Status;
}


/* Function 9 */
NTSTATUS WINAPI LsarClearAuditLog(
    LSAPR_HANDLE ObjectHandle)
{
    /* Deprecated */
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 10 */
NTSTATUS WINAPI LsarCreateAccount(
    LSAPR_HANDLE PolicyHandle,
    PRPC_SID AccountSid,
    ACCESS_MASK DesiredAccess,
    LSAPR_HANDLE *AccountHandle)
{
    PLSA_DB_OBJECT PolicyObject;
    PLSA_DB_OBJECT AccountObject = NULL;
    LPWSTR SidString = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    /* Validate the AccountSid */
    if (!RtlValidSid(AccountSid))
        return STATUS_INVALID_PARAMETER;

    /* Validate the PolicyHandle */
    Status = LsapValidateDbObject(PolicyHandle,
                                  LsaDbPolicyObject,
                                  POLICY_CREATE_ACCOUNT,
                                  &PolicyObject);
    if (!NT_SUCCESS(Status))
    {
        ERR("LsapValidateDbObject returned 0x%08lx\n", Status);
        return Status;
    }

    /* Create SID string */
    if (!ConvertSidToStringSid((PSID)AccountSid,
                               &SidString))
    {
        ERR("ConvertSidToStringSid failed\n");
        Status = STATUS_INVALID_PARAMETER;
        goto done;
    }

    /* Create the Account object */
    Status = LsapCreateDbObject(PolicyObject,
                                L"Accounts",
                                SidString,
                                LsaDbAccountObject,
                                DesiredAccess,
                                &AccountObject);
    if (!NT_SUCCESS(Status))
    {
        ERR("LsapCreateDbObject failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    /* Set the Sid attribute */
    Status = LsapSetObjectAttribute(AccountObject,
                                    L"Sid",
                                    (PVOID)AccountSid,
                                    GetLengthSid(AccountSid));

done:
    if (SidString != NULL)
        LocalFree(SidString);

    if (!NT_SUCCESS(Status))
    {
        if (AccountObject != NULL)
            LsapCloseDbObject(AccountObject);
    }
    else
    {
        *AccountHandle = (LSAPR_HANDLE)AccountObject;
    }

    return STATUS_SUCCESS;
}


/* Function 11 */
NTSTATUS WINAPI LsarEnumerateAccounts(
    LSAPR_HANDLE PolicyHandle,
    DWORD *EnumerationContext,
    PLSAPR_ACCOUNT_ENUM_BUFFER EnumerationBuffer,
    DWORD PreferedMaximumLength)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 12 */
NTSTATUS WINAPI LsarCreateTrustedDomain(
    LSAPR_HANDLE PolicyHandle,
    PLSAPR_TRUST_INFORMATION TrustedDomainInformation,
    ACCESS_MASK DesiredAccess,
    LSAPR_HANDLE *TrustedDomainHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 13 */
NTSTATUS WINAPI LsarEnumerateTrustedDomains(
    LSAPR_HANDLE PolicyHandle,
    DWORD *EnumerationContext,
    PLSAPR_TRUSTED_ENUM_BUFFER EnumerationBuffer,
    DWORD PreferedMaximumLength)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 14 */
NTSTATUS WINAPI LsarLookupNames(
    LSAPR_HANDLE PolicyHandle,
    DWORD Count,
    PRPC_UNICODE_STRING Names,
    PLSAPR_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    PLSAPR_TRANSLATED_SIDS TranslatedSids,
    LSAP_LOOKUP_LEVEL LookupLevel,
    DWORD *MappedCount)
{
    SID_IDENTIFIER_AUTHORITY IdentifierAuthority = {SECURITY_NT_AUTHORITY};
    static const UNICODE_STRING DomainName = RTL_CONSTANT_STRING(L"DOMAIN");
    PLSAPR_REFERENCED_DOMAIN_LIST OutputDomains = NULL;
    PLSA_TRANSLATED_SID OutputSids = NULL;
    ULONG OutputSidsLength;
    ULONG i;
    PSID Sid;
    ULONG SidLength;
    NTSTATUS Status;

    TRACE("LsarLookupNames(%p, %lu, %p, %p, %p, %d, %p)\n",
          PolicyHandle, Count, Names, ReferencedDomains, TranslatedSids,
          LookupLevel, MappedCount);

    TranslatedSids->Entries = Count;
    TranslatedSids->Sids = NULL;
    *ReferencedDomains = NULL;

    OutputSidsLength = Count * sizeof(LSA_TRANSLATED_SID);
    OutputSids = MIDL_user_allocate(OutputSidsLength);
    if (OutputSids == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(OutputSids, OutputSidsLength);

    OutputDomains = MIDL_user_allocate(sizeof(LSAPR_REFERENCED_DOMAIN_LIST));
    if (OutputDomains == NULL)
    {
        MIDL_user_free(OutputSids);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    OutputDomains->Entries = Count;
    OutputDomains->Domains = MIDL_user_allocate(Count * sizeof(LSA_TRUST_INFORMATION));
    if (OutputDomains->Domains == NULL)
    {
        MIDL_user_free(OutputDomains);
        MIDL_user_free(OutputSids);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = RtlAllocateAndInitializeSid(&IdentifierAuthority,
                                         2,
                                         SECURITY_BUILTIN_DOMAIN_RID,
                                         DOMAIN_ALIAS_RID_ADMINS,
                                         0, 0, 0, 0, 0, 0,
                                         &Sid);
    if (!NT_SUCCESS(Status))
    {
        MIDL_user_free(OutputDomains->Domains);
        MIDL_user_free(OutputDomains);
        MIDL_user_free(OutputSids);
        return Status;
    }

    SidLength = RtlLengthSid(Sid);

    for (i = 0; i < Count; i++)
    {
        OutputDomains->Domains[i].Sid = MIDL_user_allocate(SidLength);
        RtlCopyMemory(OutputDomains->Domains[i].Sid, Sid, SidLength);

        OutputDomains->Domains[i].Name.Buffer = MIDL_user_allocate(DomainName.MaximumLength);
        OutputDomains->Domains[i].Name.Length = DomainName.Length;
        OutputDomains->Domains[i].Name.MaximumLength = DomainName.MaximumLength;
        RtlCopyMemory(OutputDomains->Domains[i].Name.Buffer, DomainName.Buffer, DomainName.MaximumLength);
    }

    for (i = 0; i < Count; i++)
    {
        OutputSids[i].Use = SidTypeWellKnownGroup;
        OutputSids[i].RelativeId = DOMAIN_USER_RID_ADMIN; //DOMAIN_ALIAS_RID_ADMINS;
        OutputSids[i].DomainIndex = i;
    }

    *ReferencedDomains = OutputDomains;

    *MappedCount = Count;

    TranslatedSids->Entries = Count;
    TranslatedSids->Sids = OutputSids;

    return STATUS_SUCCESS;
}


/* Function 15 */
NTSTATUS WINAPI LsarLookupSids(
    LSAPR_HANDLE PolicyHandle,
    PLSAPR_SID_ENUM_BUFFER SidEnumBuffer,
    PLSAPR_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    PLSAPR_TRANSLATED_NAMES TranslatedNames,
    LSAP_LOOKUP_LEVEL LookupLevel,
    DWORD *MappedCount)
{
    SID_IDENTIFIER_AUTHORITY IdentifierAuthority = {SECURITY_NT_AUTHORITY};
    static const UNICODE_STRING DomainName = RTL_CONSTANT_STRING(L"DOMAIN");
    PLSAPR_REFERENCED_DOMAIN_LIST OutputDomains = NULL;
    PLSAPR_TRANSLATED_NAME OutputNames = NULL;
    ULONG OutputNamesLength;
    ULONG i;
    PSID Sid;
    ULONG SidLength;
    NTSTATUS Status;

    TRACE("LsarLookupSids(%p, %p, %p, %p, %d, %p)\n",
          PolicyHandle, SidEnumBuffer, ReferencedDomains, TranslatedNames,
          LookupLevel, MappedCount);

    TranslatedNames->Entries = SidEnumBuffer->Entries;
    TranslatedNames->Names = NULL;
    *ReferencedDomains = NULL;

    OutputNamesLength = SidEnumBuffer->Entries * sizeof(LSA_TRANSLATED_NAME);
    OutputNames = MIDL_user_allocate(OutputNamesLength);
    if (OutputNames == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(OutputNames, OutputNamesLength);

    OutputDomains = MIDL_user_allocate(sizeof(LSAPR_REFERENCED_DOMAIN_LIST));
    if (OutputDomains == NULL)
    {
        MIDL_user_free(OutputNames);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    OutputDomains->Entries = SidEnumBuffer->Entries;
    OutputDomains->Domains = MIDL_user_allocate(SidEnumBuffer->Entries * sizeof(LSA_TRUST_INFORMATION));
    if (OutputDomains->Domains == NULL)
    {
        MIDL_user_free(OutputDomains);
        MIDL_user_free(OutputNames);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = RtlAllocateAndInitializeSid(&IdentifierAuthority,
                                         2,
                                         SECURITY_BUILTIN_DOMAIN_RID,
                                         DOMAIN_ALIAS_RID_ADMINS,
                                         0, 0, 0, 0, 0, 0,
                                         &Sid);
    if (!NT_SUCCESS(Status))
    {
        MIDL_user_free(OutputDomains->Domains);
        MIDL_user_free(OutputDomains);
        MIDL_user_free(OutputNames);
        return Status;
    }

    SidLength = RtlLengthSid(Sid);

    for (i = 0; i < SidEnumBuffer->Entries; i++)
    {
        OutputDomains->Domains[i].Sid = MIDL_user_allocate(SidLength);
        RtlCopyMemory(OutputDomains->Domains[i].Sid, Sid, SidLength);

        OutputDomains->Domains[i].Name.Buffer = MIDL_user_allocate(DomainName.MaximumLength);
        OutputDomains->Domains[i].Name.Length = DomainName.Length;
        OutputDomains->Domains[i].Name.MaximumLength = DomainName.MaximumLength;
        RtlCopyMemory(OutputDomains->Domains[i].Name.Buffer, DomainName.Buffer, DomainName.MaximumLength);
    }

    Status = LsapLookupSids(SidEnumBuffer,
                            OutputNames);

    *ReferencedDomains = OutputDomains;

    *MappedCount = SidEnumBuffer->Entries;

    TranslatedNames->Entries = SidEnumBuffer->Entries;
    TranslatedNames->Names = OutputNames;

    return Status;
}


/* Function 16 */
NTSTATUS WINAPI LsarCreateSecret(
    LSAPR_HANDLE PolicyHandle,
    PRPC_UNICODE_STRING SecretName,
    ACCESS_MASK DesiredAccess,
    LSAPR_HANDLE *SecretHandle)
{
    PLSA_DB_OBJECT PolicyObject;
    PLSA_DB_OBJECT SecretObject = NULL;
    LARGE_INTEGER Time;
    NTSTATUS Status = STATUS_SUCCESS;

    /* Validate the PolicyHandle */
    Status = LsapValidateDbObject(PolicyHandle,
                                  LsaDbPolicyObject,
                                  POLICY_CREATE_SECRET,
                                  &PolicyObject);
    if (!NT_SUCCESS(Status))
    {
        ERR("LsapValidateDbObject returned 0x%08lx\n", Status);
        return Status;
    }

    /* Get the current time */
    Status = NtQuerySystemTime(&Time);
    if (!NT_SUCCESS(Status))
    {
        ERR("NtQuerySystemTime failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    /* Create the Secret object */
    Status = LsapCreateDbObject(PolicyObject,
                                L"Secrets",
                                SecretName->Buffer,
                                LsaDbSecretObject,
                                DesiredAccess,
                                &SecretObject);
    if (!NT_SUCCESS(Status))
    {
        ERR("LsapCreateDbObject failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    /* Set the CurrentTime attribute */
    Status = LsapSetObjectAttribute(SecretObject,
                                    L"CurrentTime",
                                    (PVOID)&Time,
                                    sizeof(LARGE_INTEGER));
    if (!NT_SUCCESS(Status))
    {
        ERR("LsapSetObjectAttribute (CurrentTime) failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    /* Set the OldTime attribute */
    Status = LsapSetObjectAttribute(SecretObject,
                                    L"OldTime",
                                    (PVOID)&Time,
                                    sizeof(LARGE_INTEGER));

done:
    if (!NT_SUCCESS(Status))
    {
        if (SecretObject != NULL)
            LsapCloseDbObject(SecretObject);
    }
    else
    {
        *SecretHandle = (LSAPR_HANDLE)SecretObject;
    }

    return STATUS_SUCCESS;
}


/* Function 17 */
NTSTATUS WINAPI LsarOpenAccount(
    LSAPR_HANDLE PolicyHandle,
    PRPC_SID AccountSid,
    ACCESS_MASK DesiredAccess,
    LSAPR_HANDLE *AccountHandle)
{
    PLSA_DB_OBJECT PolicyObject;
    PLSA_DB_OBJECT AccountObject = NULL;
    LPWSTR SidString = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    /* Validate the AccountSid */
    if (!RtlValidSid(AccountSid))
        return STATUS_INVALID_PARAMETER;

    /* Validate the PolicyHandle */
    Status = LsapValidateDbObject(PolicyHandle,
                                  LsaDbPolicyObject,
                                  0,
                                  &PolicyObject);
    if (!NT_SUCCESS(Status))
    {
        ERR("LsapValidateDbObject returned 0x%08lx\n", Status);
        return Status;
    }

    /* Create SID string */
    if (!ConvertSidToStringSid((PSID)AccountSid,
                               &SidString))
    {
        ERR("ConvertSidToStringSid failed\n");
        Status = STATUS_INVALID_PARAMETER;
        goto done;
    }

    /* Create the Account object */
    Status = LsapOpenDbObject(PolicyObject,
                              L"Accounts",
                              SidString,
                              LsaDbAccountObject,
                              DesiredAccess,
                              &AccountObject);
    if (!NT_SUCCESS(Status))
    {
        ERR("LsapOpenDbObject failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    /* Set the Sid attribute */
    Status = LsapSetObjectAttribute(AccountObject,
                                    L"Sid",
                                    (PVOID)AccountSid,
                                    GetLengthSid(AccountSid));

done:
    if (SidString != NULL)
        LocalFree(SidString);

    if (!NT_SUCCESS(Status))
    {
        if (AccountObject != NULL)
            LsapCloseDbObject(AccountObject);
    }
    else
    {
        *AccountHandle = (LSAPR_HANDLE)AccountObject;
    }

    return Status;
}


/* Function 18 */
NTSTATUS WINAPI LsarEnumeratePrivilegesAccount(
    LSAPR_HANDLE AccountHandle,
    PLSAPR_PRIVILEGE_SET *Privileges)
{
    PLSA_DB_OBJECT AccountObject;
    ULONG PrivilegeSetSize = 0;
    PLSAPR_PRIVILEGE_SET PrivilegeSet = NULL;
    NTSTATUS Status;

    *Privileges = NULL;

    /* Validate the AccountHandle */
    Status = LsapValidateDbObject(AccountHandle,
                                  LsaDbAccountObject,
                                  ACCOUNT_VIEW,
                                  &AccountObject);
    if (!NT_SUCCESS(Status))
    {
        ERR("LsapValidateDbObject returned 0x%08lx\n", Status);
        return Status;
    }

    /* Get the size of the privilege set */
    Status = LsapGetObjectAttribute(AccountObject,
                                    L"Privilgs",
                                    NULL,
                                    &PrivilegeSetSize);
    if (!NT_SUCCESS(Status))
        return Status;

    /* Allocate a buffer for the privilege set */
    PrivilegeSet = MIDL_user_allocate(PrivilegeSetSize);
    if (PrivilegeSet == NULL)
        return STATUS_NO_MEMORY;

    /* Get the privilege set */
    Status = LsapGetObjectAttribute(AccountObject,
                                    L"Privilgs",
                                    PrivilegeSet,
                                    &PrivilegeSetSize);
    if (!NT_SUCCESS(Status))
    {
        MIDL_user_free(PrivilegeSet);
        return Status;
    }

    /* Return a pointer to the privilege set */
    *Privileges = PrivilegeSet;

    return STATUS_SUCCESS;
}


/* Function 19 */
NTSTATUS WINAPI LsarAddPrivilegesToAccount(
    LSAPR_HANDLE AccountHandle,
    PLSAPR_PRIVILEGE_SET Privileges)
{
    PLSA_DB_OBJECT AccountObject;
    PPRIVILEGE_SET CurrentPrivileges = NULL;
    PPRIVILEGE_SET NewPrivileges = NULL;
    ULONG PrivilegeSetSize = 0;
    ULONG PrivilegeCount;
    ULONG i, j;
    BOOL bFound;
    NTSTATUS Status;

    /* Validate the AccountHandle */
    Status = LsapValidateDbObject(AccountHandle,
                                  LsaDbAccountObject,
                                  ACCOUNT_ADJUST_PRIVILEGES,
                                  &AccountObject);
    if (!NT_SUCCESS(Status))
    {
        ERR("LsapValidateDbObject returned 0x%08lx\n", Status);
        return Status;
    }

    Status = LsapGetObjectAttribute(AccountObject,
                                    L"Privilgs",
                                    NULL,
                                    &PrivilegeSetSize);
    if (!NT_SUCCESS(Status) || PrivilegeSetSize == 0)
    {
        /* The Privilgs attribute does not exist */

        PrivilegeSetSize = sizeof(PRIVILEGE_SET) +
                           (Privileges->PrivilegeCount - 1) * sizeof(LUID_AND_ATTRIBUTES);
        Status = LsapSetObjectAttribute(AccountObject,
                                        L"Privilgs",
                                        Privileges,
                                        PrivilegeSetSize);
    }
    else
    {
        /* The Privilgs attribute exists */

        /* Allocate memory for the stored privilege set */
        CurrentPrivileges = MIDL_user_allocate(PrivilegeSetSize);
        if (CurrentPrivileges == NULL)
            return STATUS_NO_MEMORY;

        /* Get the current privilege set */
        Status = LsapGetObjectAttribute(AccountObject,
                                        L"Privilgs",
                                        CurrentPrivileges,
                                        &PrivilegeSetSize);
        if (!NT_SUCCESS(Status))
        {
            TRACE("LsapGetObjectAttribute() failed (Status 0x%08lx)\n", Status);
            goto done;
        }

        PrivilegeCount = CurrentPrivileges->PrivilegeCount;
        TRACE("Current privilege count: %lu\n", PrivilegeCount);

        /* Calculate the number privileges in the combined privilege set */
        for (i = 0; i < Privileges->PrivilegeCount; i++)
        {
            bFound = FALSE;
            for (j = 0; j < CurrentPrivileges->PrivilegeCount; j++)
            {
                if (RtlEqualLuid(&(Privileges->Privilege[i].Luid),
                                 &(CurrentPrivileges->Privilege[i].Luid)))
                {
                    bFound = TRUE;
                    break;
                }
            }

            if (bFound == FALSE)
            {
                TRACE("Found new privilege\n");
                PrivilegeCount++;
            }
        }
        TRACE("New privilege count: %lu\n", PrivilegeCount);

        /* Calculate the size of the new privilege set and allocate it */
        PrivilegeSetSize = sizeof(PRIVILEGE_SET) +
                           (PrivilegeCount - 1) * sizeof(LUID_AND_ATTRIBUTES);
        NewPrivileges = MIDL_user_allocate(PrivilegeSetSize);
        if (NewPrivileges == NULL)
        {
            Status = STATUS_NO_MEMORY;
            goto done;
        }

        /* Initialize the new privilege set */
        NewPrivileges->PrivilegeCount = PrivilegeCount;
        NewPrivileges->Control = 0;

        /* Copy all privileges from the current privilege set */
        RtlCopyLuidAndAttributesArray(CurrentPrivileges->PrivilegeCount,
                                      &(CurrentPrivileges->Privilege[0]),
                                      &(NewPrivileges->Privilege[0]));

        /* Add new privileges to the new privilege set */
        PrivilegeCount = CurrentPrivileges->PrivilegeCount;
        for (i = 0; i < Privileges->PrivilegeCount; i++)
        {
            bFound = FALSE;
            for (j = 0; j < CurrentPrivileges->PrivilegeCount; j++)
            {
                if (RtlEqualLuid(&(Privileges->Privilege[i].Luid),
                                 &(CurrentPrivileges->Privilege[i].Luid)))
                {
                    /* Overwrite attributes if a matching privilege was found */
                    NewPrivileges->Privilege[j].Attributes = Privileges->Privilege[i].Attributes;

                    bFound = TRUE;
                    break;
                }
            }

            if (bFound == FALSE)
            {
                /* Copy the new privilege */
                RtlCopyLuidAndAttributesArray(1,
                                              (PLUID_AND_ATTRIBUTES)&(Privileges->Privilege[i]),
                                              &(NewPrivileges->Privilege[PrivilegeCount]));
                PrivilegeCount++;
            }
        }

        /* Set the new priivliege set */
        Status = LsapSetObjectAttribute(AccountObject,
                                        L"Privilgs",
                                        NewPrivileges,
                                        PrivilegeSetSize);
    }

done:
    if (CurrentPrivileges != NULL)
        MIDL_user_free(CurrentPrivileges);

    if (NewPrivileges != NULL)
        MIDL_user_free(NewPrivileges);

    return Status;
}


/* Function 20 */
NTSTATUS WINAPI LsarRemovePrivilegesFromAccount(
    LSAPR_HANDLE AccountHandle,
    BOOL AllPrivileges,
    PLSAPR_PRIVILEGE_SET Privileges)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 21 */
NTSTATUS WINAPI LsarGetQuotasForAccount(
    LSAPR_HANDLE AccountHandle,
    PQUOTA_LIMITS QuotaLimits)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 22 */
NTSTATUS WINAPI LsarSetQuotasForAccount(
    LSAPR_HANDLE AccountHandle,
    PQUOTA_LIMITS QuotaLimits)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 23 */
NTSTATUS WINAPI LsarGetSystemAccessAccount(
    LSAPR_HANDLE AccountHandle,
    ACCESS_MASK *SystemAccess)
{
    PLSA_DB_OBJECT AccountObject;
    ULONG Size;
    NTSTATUS Status;

    /* Validate the account handle */
    Status = LsapValidateDbObject(AccountHandle,
                                  LsaDbAccountObject,
                                  ACCOUNT_VIEW,
                                  &AccountObject);
    if (!NT_SUCCESS(Status))
    {
        ERR("Invalid handle (Status %lx)\n", Status);
        return Status;
    }

    /* Get the system access flags */
    Status = LsapGetObjectAttribute(AccountObject,
                                    L"ActSysAc",
                                    SystemAccess,
                                    &Size);

    return Status;
}


/* Function 24 */
NTSTATUS WINAPI LsarSetSystemAccessAccount(
    LSAPR_HANDLE AccountHandle,
    ACCESS_MASK SystemAccess)
{
    PLSA_DB_OBJECT AccountObject;
    NTSTATUS Status;

    /* Validate the account handle */
    Status = LsapValidateDbObject(AccountHandle,
                                  LsaDbAccountObject,
                                  ACCOUNT_ADJUST_SYSTEM_ACCESS,
                                  &AccountObject);
    if (!NT_SUCCESS(Status))
    {
        ERR("Invalid handle (Status %lx)\n", Status);
        return Status;
    }

    /* Set the system access flags */
    Status = LsapSetObjectAttribute(AccountObject,
                                    L"ActSysAc",
                                    &SystemAccess,
                                    sizeof(ACCESS_MASK));

    return Status;
}


/* Function 25 */
NTSTATUS WINAPI LsarOpenTrustedDomain(
    LSAPR_HANDLE PolicyHandle,
    PRPC_SID TrustedDomainSid,
    ACCESS_MASK DesiredAccess,
    LSAPR_HANDLE *TrustedDomainHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 26 */
NTSTATUS WINAPI LsarQueryInfoTrustedDomain(
    LSAPR_HANDLE TrustedDomainHandle,
    TRUSTED_INFORMATION_CLASS InformationClass,
    PLSAPR_TRUSTED_DOMAIN_INFO *TrustedDomainInformation)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 27 */
NTSTATUS WINAPI LsarSetInformationTrustedDomain(
    LSAPR_HANDLE TrustedDomainHandle,
    TRUSTED_INFORMATION_CLASS InformationClass,
    PLSAPR_TRUSTED_DOMAIN_INFO TrustedDomainInformation)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 28 */
NTSTATUS WINAPI LsarOpenSecret(
    LSAPR_HANDLE PolicyHandle,
    PRPC_UNICODE_STRING SecretName,
    ACCESS_MASK DesiredAccess,
    LSAPR_HANDLE *SecretHandle)
{
    PLSA_DB_OBJECT PolicyObject;
    PLSA_DB_OBJECT SecretObject = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    /* Validate the PolicyHandle */
    Status = LsapValidateDbObject(PolicyHandle,
                                  LsaDbPolicyObject,
                                  0,
                                  &PolicyObject);
    if (!NT_SUCCESS(Status))
    {
        ERR("LsapValidateDbObject returned 0x%08lx\n", Status);
        return Status;
    }

    /* Create the secret object */
    Status = LsapOpenDbObject(PolicyObject,
                              L"Secrets",
                              SecretName->Buffer,
                              LsaDbSecretObject,
                              DesiredAccess,
                              &SecretObject);
    if (!NT_SUCCESS(Status))
    {
        ERR("LsapOpenDbObject failed (Status 0x%08lx)\n", Status);
        goto done;
    }

done:
    if (!NT_SUCCESS(Status))
    {
        if (SecretObject != NULL)
            LsapCloseDbObject(SecretObject);
    }
    else
    {
        *SecretHandle = (LSAPR_HANDLE)SecretObject;
    }

    return Status;
}


/* Function 29 */
NTSTATUS WINAPI LsarSetSecret(
    LSAPR_HANDLE SecretHandle,
    PLSAPR_CR_CIPHER_VALUE EncryptedCurrentValue,
    PLSAPR_CR_CIPHER_VALUE EncryptedOldValue)
{
    PLSA_DB_OBJECT SecretObject;
    PBYTE CurrentValue = NULL;
    PBYTE OldValue = NULL;
    ULONG CurrentValueLength = 0;
    ULONG OldValueLength = 0;
    LARGE_INTEGER Time;
    NTSTATUS Status;

    TRACE("LsarSetSecret(%p %p %p)\n", SecretHandle,
          EncryptedCurrentValue, EncryptedOldValue);

    /* Validate the SecretHandle */
    Status = LsapValidateDbObject(SecretHandle,
                                  LsaDbSecretObject,
                                  SECRET_SET_VALUE,
                                  &SecretObject);
    if (!NT_SUCCESS(Status))
    {
        ERR("LsapValidateDbObject returned 0x%08lx\n", Status);
        return Status;
    }

    if (EncryptedCurrentValue != NULL)
    {
        /* FIXME: Decrypt the current value */
        CurrentValue = EncryptedCurrentValue->Buffer;
        CurrentValueLength = EncryptedCurrentValue->MaximumLength;
    }

    /* Set the current value */
    Status = LsapSetObjectAttribute(SecretObject,
                                    L"CurrentValue",
                                    CurrentValue,
                                    CurrentValueLength);
    if (!NT_SUCCESS(Status))
    {
        ERR("LsapSetObjectAttribute failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    /* Get the current time */
    Status = NtQuerySystemTime(&Time);
    if (!NT_SUCCESS(Status))
    {
        ERR("NtQuerySystemTime failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    /* Set the current time */
    Status = LsapSetObjectAttribute(SecretObject,
                                    L"CurrentTime",
                                    &Time,
                                    sizeof(LARGE_INTEGER));
    if (!NT_SUCCESS(Status))
    {
        ERR("LsapSetObjectAttribute failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    if (EncryptedOldValue != NULL)
    {
        /* FIXME: Decrypt the old value */
        OldValue = EncryptedOldValue->Buffer;
        OldValueLength = EncryptedOldValue->MaximumLength;
    }

    /* Set the old value */
    Status = LsapSetObjectAttribute(SecretObject,
                                    L"OldValue",
                                    OldValue,
                                    OldValueLength);
    if (!NT_SUCCESS(Status))
    {
        ERR("LsapSetObjectAttribute failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    /* Set the old time */
    Status = LsapSetObjectAttribute(SecretObject,
                                    L"OldTime",
                                    &Time,
                                    sizeof(LARGE_INTEGER));
    if (!NT_SUCCESS(Status))
    {
        ERR("LsapSetObjectAttribute failed (Status 0x%08lx)\n", Status);
    }

done:
    return Status;
}


/* Function 30 */
NTSTATUS WINAPI LsarQuerySecret(
    LSAPR_HANDLE SecretHandle,
    PLSAPR_CR_CIPHER_VALUE *EncryptedCurrentValue,
    PLARGE_INTEGER CurrentValueSetTime,
    PLSAPR_CR_CIPHER_VALUE *EncryptedOldValue,
    PLARGE_INTEGER OldValueSetTime)
{
    PLSA_DB_OBJECT SecretObject;
    PLSAPR_CR_CIPHER_VALUE EncCurrentValue = NULL;
    PLSAPR_CR_CIPHER_VALUE EncOldValue = NULL;
    PBYTE CurrentValue = NULL;
    PBYTE OldValue = NULL;
    ULONG CurrentValueLength = 0;
    ULONG OldValueLength = 0;
    ULONG BufferSize;
    NTSTATUS Status;

    TRACE("LsarQuerySecret(%p %p %p %p %p)\n", SecretHandle,
          EncryptedCurrentValue, CurrentValueSetTime,
          EncryptedOldValue, OldValueSetTime);

    /* Validate the SecretHandle */
    Status = LsapValidateDbObject(SecretHandle,
                                  LsaDbSecretObject,
                                  SECRET_QUERY_VALUE,
                                  &SecretObject);
    if (!NT_SUCCESS(Status))
    {
        ERR("LsapValidateDbObject returned 0x%08lx\n", Status);
        return Status;
    }

    if (EncryptedCurrentValue != NULL)
    {
        CurrentValueLength = 0;

        /* Get the size of the current value */
        Status = LsapGetObjectAttribute(SecretObject,
                                        L"CurrentValue",
                                        NULL,
                                        &CurrentValueLength);
        if (!NT_SUCCESS(Status))
            goto done;

        /* Allocate a buffer for the current value */
        CurrentValue = midl_user_allocate(CurrentValueLength);
        if (CurrentValue == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto done;
        }

        /* Get the current value */
        Status = LsapGetObjectAttribute(SecretObject,
                                        L"CurrentValue",
                                        CurrentValue,
                                        &CurrentValueLength);
        if (!NT_SUCCESS(Status))
            goto done;

        /* Allocate a buffer for the encrypted current value */
        EncCurrentValue = midl_user_allocate(sizeof(LSAPR_CR_CIPHER_VALUE));
        if (EncCurrentValue == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto done;
        }

        /* FIXME: Encrypt the current value */
        EncCurrentValue->Length = (USHORT)(CurrentValueLength - sizeof(WCHAR));
        EncCurrentValue->MaximumLength = (USHORT)CurrentValueLength;
        EncCurrentValue->Buffer = (PBYTE)CurrentValue;
    }

    if (CurrentValueSetTime != NULL)
    {
        BufferSize = sizeof(LARGE_INTEGER);

        /* Get the current value time */
        Status = LsapGetObjectAttribute(SecretObject,
                                        L"CurrentTime",
                                        (PBYTE)CurrentValueSetTime,
                                        &BufferSize);
        if (!NT_SUCCESS(Status))
            goto done;
    }

    if (EncryptedOldValue != NULL)
    {
        OldValueLength = 0;

        /* Get the size of the old value */
        Status = LsapGetObjectAttribute(SecretObject,
                                        L"OldValue",
                                        NULL,
                                        &OldValueLength);
        if (!NT_SUCCESS(Status))
            goto done;

        /* Allocate a buffer for the old value */
        OldValue = midl_user_allocate(OldValueLength);
        if (OldValue == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto done;
        }

        /* Get the old value */
        Status = LsapGetObjectAttribute(SecretObject,
                                        L"OldValue",
                                        OldValue,
                                        &OldValueLength);
        if (!NT_SUCCESS(Status))
            goto done;

        /* Allocate a buffer for the encrypted old value */
        EncOldValue = midl_user_allocate(sizeof(LSAPR_CR_CIPHER_VALUE) + OldValueLength);
        if (EncOldValue == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto done;
        }

        /* FIXME: Encrypt the old value */
        EncOldValue->Length = (USHORT)(OldValueLength - sizeof(WCHAR));
        EncOldValue->MaximumLength = (USHORT)OldValueLength;
        EncOldValue->Buffer = (PBYTE)OldValue;
    }

    if (OldValueSetTime != NULL)
    {
        BufferSize = sizeof(LARGE_INTEGER);

        /* Get the old value time */
        Status = LsapGetObjectAttribute(SecretObject,
                                        L"OldTime",
                                        (PBYTE)OldValueSetTime,
                                        &BufferSize);
        if (!NT_SUCCESS(Status))
            goto done;
    }


done:
    if (NT_SUCCESS(Status))
    {
        if (EncryptedCurrentValue != NULL)
            *EncryptedCurrentValue = EncCurrentValue;

        if (EncryptedOldValue != NULL)
            *EncryptedOldValue = EncOldValue;
    }
    else
    {
        if (EncryptedCurrentValue != NULL)
            *EncryptedCurrentValue = NULL;

        if (EncryptedOldValue != NULL)
            *EncryptedOldValue = NULL;

        if (EncCurrentValue != NULL)
            midl_user_free(EncCurrentValue);

        if (EncOldValue != NULL)
            midl_user_free(EncOldValue);

        if (CurrentValue != NULL)
            midl_user_free(CurrentValue);

        if (OldValue != NULL)
            midl_user_free(OldValue);
    }

    TRACE("LsarQuerySecret done (Status 0x%08lx)\n", Status);

    return Status;
}


/* Function 31 */
NTSTATUS WINAPI LsarLookupPrivilegeValue(
    LSAPR_HANDLE PolicyHandle,
    PRPC_UNICODE_STRING Name,
    PLUID Value)
{
    NTSTATUS Status;

    TRACE("LsarLookupPrivilegeValue(%p, %wZ, %p)\n",
          PolicyHandle, Name, Value);

    Status = LsapValidateDbObject(PolicyHandle,
                                  LsaDbPolicyObject,
                                  POLICY_LOOKUP_NAMES,
                                  NULL);
    if (!NT_SUCCESS(Status))
    {
        ERR("Invalid handle (Status %lx)\n", Status);
        return Status;
    }

    TRACE("Privilege: %wZ\n", Name);

    Status = LsarpLookupPrivilegeValue((PUNICODE_STRING)Name,
                                       Value);

    return Status;
}


/* Function 32 */
NTSTATUS WINAPI LsarLookupPrivilegeName(
    LSAPR_HANDLE PolicyHandle,
    PLUID Value,
    PRPC_UNICODE_STRING *Name)
{
    NTSTATUS Status;

    TRACE("LsarLookupPrivilegeName(%p, %p, %p)\n",
          PolicyHandle, Value, Name);

    Status = LsapValidateDbObject(PolicyHandle,
                                  LsaDbPolicyObject,
                                  POLICY_LOOKUP_NAMES,
                                  NULL);
    if (!NT_SUCCESS(Status))
    {
        ERR("Invalid handle\n");
        return Status;
    }

    Status = LsarpLookupPrivilegeName(Value, (PUNICODE_STRING*)Name);

    return Status;
}


/* Function 33 */
NTSTATUS WINAPI LsarLookupPrivilegeDisplayName(
    LSAPR_HANDLE PolicyHandle,
    PRPC_UNICODE_STRING Name,
    USHORT ClientLanguage,
    USHORT ClientSystemDefaultLanguage,
    PRPC_UNICODE_STRING *DisplayName,
    USHORT *LanguageReturned)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 34 */
NTSTATUS WINAPI LsarDeleteObject(
    LSAPR_HANDLE *ObjectHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 35 */
NTSTATUS WINAPI LsarEnumerateAccountsWithUserRight(
    LSAPR_HANDLE PolicyHandle,
    PRPC_UNICODE_STRING UserRight,
    PLSAPR_ACCOUNT_ENUM_BUFFER EnumerationBuffer)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 36 */
NTSTATUS WINAPI LsarEnumerateAccountRights(
    LSAPR_HANDLE PolicyHandle,
    PRPC_SID AccountSid,
    PLSAPR_USER_RIGHT_SET UserRights)
{
    PLSA_DB_OBJECT PolicyObject;
    NTSTATUS Status;

    TRACE("LsarEnumerateAccountRights(%p %p %p)\n",
          PolicyHandle, AccountSid, UserRights);

    Status = LsapValidateDbObject(PolicyHandle,
                                  LsaDbPolicyObject,
                                  ACCOUNT_VIEW,
                                  &PolicyObject);
    if (!NT_SUCCESS(Status))
        return Status;

    UserRights->Entries = 0;
    UserRights->UserRights = NULL;
    return STATUS_OBJECT_NAME_NOT_FOUND;
}


/* Function 37 */
NTSTATUS WINAPI LsarAddAccountRights(
    LSAPR_HANDLE PolicyHandle,
    PRPC_SID AccountSid,
    PLSAPR_USER_RIGHT_SET UserRights)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 38 */
NTSTATUS WINAPI LsarRemoveAccountRights(
    LSAPR_HANDLE PolicyHandle,
    PRPC_SID AccountSid,
    BOOL AllRights,
    PLSAPR_USER_RIGHT_SET UserRights)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 39 */
NTSTATUS WINAPI LsarQueryTrustedDomainInfo(
    LSAPR_HANDLE PolicyHandle,
    PRPC_SID TrustedDomainSid,
    TRUSTED_INFORMATION_CLASS InformationClass,
    PLSAPR_TRUSTED_DOMAIN_INFO *TrustedDomainInformation)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 40 */
NTSTATUS WINAPI LsarSetTrustedDomainInfo(
    LSAPR_HANDLE PolicyHandle,
    PRPC_SID TrustedDomainSid,
    TRUSTED_INFORMATION_CLASS InformationClass,
    PLSAPR_TRUSTED_DOMAIN_INFO TrustedDomainInformation)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 41 */
NTSTATUS WINAPI LsarDeleteTrustedDomain(
    LSAPR_HANDLE PolicyHandle,
    PRPC_SID TrustedDomainSid)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 42 */
NTSTATUS WINAPI LsarStorePrivateData(
    LSAPR_HANDLE PolicyHandle,
    PRPC_UNICODE_STRING KeyName,
    PLSAPR_CR_CIPHER_VALUE EncryptedData)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 43 */
NTSTATUS WINAPI LsarRetrievePrivateData(
    LSAPR_HANDLE PolicyHandle,
    PRPC_UNICODE_STRING KeyName,
    PLSAPR_CR_CIPHER_VALUE *EncryptedData)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 44 */
NTSTATUS WINAPI LsarOpenPolicy2(
    LPWSTR SystemName,
    PLSAPR_OBJECT_ATTRIBUTES ObjectAttributes,
    ACCESS_MASK DesiredAccess,
    LSAPR_HANDLE *PolicyHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 45 */
NTSTATUS WINAPI LsarGetUserName(
    LPWSTR SystemName,
    PRPC_UNICODE_STRING *UserName,
    PRPC_UNICODE_STRING *DomainName)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 46 */
NTSTATUS WINAPI LsarQueryInformationPolicy2(
    LSAPR_HANDLE PolicyHandle,
    POLICY_INFORMATION_CLASS InformationClass,
    PLSAPR_POLICY_INFORMATION *PolicyInformation)
{
    return LsarQueryInformationPolicy(PolicyHandle,
                                      InformationClass,
                                      PolicyInformation);
}


/* Function 47 */
NTSTATUS WINAPI LsarSetInformationPolicy2(
    LSAPR_HANDLE PolicyHandle,
    POLICY_INFORMATION_CLASS InformationClass,
    PLSAPR_POLICY_INFORMATION PolicyInformation)
{
    return LsarSetInformationPolicy(PolicyHandle,
                                    InformationClass,
                                    PolicyInformation);
}


/* Function 48 */
NTSTATUS WINAPI LsarQueryTrustedDomainInfoByName(
    LSAPR_HANDLE PolicyHandle,
    PRPC_UNICODE_STRING TrustedDomainName,
    POLICY_INFORMATION_CLASS InformationClass,
    PLSAPR_TRUSTED_DOMAIN_INFO *PolicyInformation)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 49 */
NTSTATUS WINAPI LsarSetTrustedDomainInfoByName(
    LSAPR_HANDLE PolicyHandle,
    PRPC_UNICODE_STRING TrustedDomainName,
    POLICY_INFORMATION_CLASS InformationClass,
    PLSAPR_TRUSTED_DOMAIN_INFO PolicyInformation)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 50 */
NTSTATUS WINAPI LsarEnumerateTrustedDomainsEx(
    LSAPR_HANDLE PolicyHandle,
    DWORD *EnumerationContext,
    PLSAPR_TRUSTED_ENUM_BUFFER_EX EnumerationBuffer,
    DWORD PreferedMaximumLength)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 51 */
NTSTATUS WINAPI LsarCreateTrustedDomainEx(
    LSAPR_HANDLE PolicyHandle,
    PLSAPR_TRUSTED_DOMAIN_INFORMATION_EX TrustedDomainInformation,
    PLSAPR_TRUSTED_DOMAIN_AUTH_INFORMATION AuthentificationInformation,
    ACCESS_MASK DesiredAccess,
    LSAPR_HANDLE *TrustedDomainHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 52 */
NTSTATUS WINAPI LsarSetPolicyReplicationHandle(
    PLSAPR_HANDLE PolicyHandle)
{
    /* Deprecated */
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 53 */
NTSTATUS WINAPI LsarQueryDomainInformationPolicy(
    LSAPR_HANDLE PolicyHandle,
    POLICY_INFORMATION_CLASS InformationClass,
    PLSAPR_POLICY_DOMAIN_INFORMATION *PolicyInformation)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 54 */
NTSTATUS WINAPI LsarSetDomainInformationPolicy(
    LSAPR_HANDLE PolicyHandle,
    POLICY_INFORMATION_CLASS InformationClass,
    PLSAPR_POLICY_DOMAIN_INFORMATION PolicyInformation)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 55 */
NTSTATUS WINAPI LsarOpenTrustedDomainByName(
    LSAPR_HANDLE PolicyHandle,
    PRPC_UNICODE_STRING TrustedDomainName,
    ACCESS_MASK DesiredAccess,
    LSAPR_HANDLE *TrustedDomainHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 56 */
NTSTATUS WINAPI LsarTestCall(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 57 */
NTSTATUS WINAPI LsarLookupSids2(
    LSAPR_HANDLE PolicyHandle,
    PLSAPR_SID_ENUM_BUFFER SidEnumBuffer,
    PLSAPR_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    PLSAPR_TRANSLATED_NAMES_EX TranslatedNames,
    LSAP_LOOKUP_LEVEL LookupLevel,
    DWORD *MappedCount,
    DWORD LookupOptions,
    DWORD ClientRevision)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 58 */
NTSTATUS WINAPI LsarLookupNames2(
    LSAPR_HANDLE PolicyHandle,
    DWORD Count,
    PRPC_UNICODE_STRING Names,
    PLSAPR_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    PLSAPR_TRANSLATED_SIDS_EX TranslatedSids,
    LSAP_LOOKUP_LEVEL LookupLevel,
    DWORD *MappedCount,
    DWORD LookupOptions,
    DWORD ClientRevision)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 59 */
NTSTATUS WINAPI LsarCreateTrustedDomainEx2(
    LSAPR_HANDLE PolicyHandle,
    PLSAPR_TRUSTED_DOMAIN_INFORMATION_EX TrustedDomainInformation,
    PLSAPR_TRUSTED_DOMAIN_AUTH_INFORMATION_INTERNAL AuthentificationInformation,
    ACCESS_MASK DesiredAccess,
    LSAPR_HANDLE *TrustedDomainHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 60 */
NTSTATUS WINAPI CredrWrite(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 61 */
NTSTATUS WINAPI CredrRead(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 62 */
NTSTATUS WINAPI CredrEnumerate(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 63 */
NTSTATUS WINAPI CredrWriteDomainCredentials(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 64 */
NTSTATUS WINAPI CredrReadDomainCredentials(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 65 */
NTSTATUS WINAPI CredrDelete(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 66 */
NTSTATUS WINAPI CredrGetTargetInfo(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 67 */
NTSTATUS WINAPI CredrProfileLoaded(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 68 */
NTSTATUS WINAPI LsarLookupNames3(
    LSAPR_HANDLE PolicyHandle,
    DWORD Count,
    PRPC_UNICODE_STRING Names,
    PLSAPR_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    PLSAPR_TRANSLATED_SIDS_EX2 TranslatedSids,
    LSAP_LOOKUP_LEVEL LookupLevel,
    DWORD *MappedCount,
    DWORD LookupOptions,
    DWORD ClientRevision)
{
    SID_IDENTIFIER_AUTHORITY IdentifierAuthority = {SECURITY_NT_AUTHORITY};
    static const UNICODE_STRING DomainName = RTL_CONSTANT_STRING(L"DOMAIN");
    PLSAPR_REFERENCED_DOMAIN_LIST DomainsBuffer = NULL;
    PLSAPR_TRANSLATED_SID_EX2 SidsBuffer = NULL;
    ULONG SidsBufferLength;
    ULONG DomainSidLength;
    ULONG AccountSidLength;
    PSID DomainSid;
    PSID AccountSid;
    ULONG i;
    NTSTATUS Status;

    TRACE("LsarLookupNames3(%p, %lu, %p, %p, %p, %d, %p, %lu, %lu)\n",
          PolicyHandle, Count, Names, ReferencedDomains, TranslatedSids,
          LookupLevel, MappedCount, LookupOptions, ClientRevision);

    if (Count == 0)
        return STATUS_NONE_MAPPED;

    TranslatedSids->Entries = Count;
    TranslatedSids->Sids = NULL;
    *ReferencedDomains = NULL;

    SidsBufferLength = Count * sizeof(LSAPR_TRANSLATED_SID_EX2);
    SidsBuffer = MIDL_user_allocate(SidsBufferLength);
    if (SidsBuffer == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    for (i = 0; i < Count; i++)
    {
        SidsBuffer[i].Use = SidTypeUser;
        SidsBuffer[i].Sid = NULL;
        SidsBuffer[i].DomainIndex = -1;
        SidsBuffer[i].Flags = 0;
    }

    DomainsBuffer = MIDL_user_allocate(sizeof(LSAPR_REFERENCED_DOMAIN_LIST));
    if (DomainsBuffer == NULL)
    {
        MIDL_user_free(SidsBuffer);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    DomainsBuffer->Entries = Count;
    DomainsBuffer->Domains = MIDL_user_allocate(Count * sizeof(LSA_TRUST_INFORMATION));
    if (DomainsBuffer->Domains == NULL)
    {
        MIDL_user_free(DomainsBuffer);
        MIDL_user_free(SidsBuffer);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = RtlAllocateAndInitializeSid(&IdentifierAuthority,
                                         2,
                                         SECURITY_BUILTIN_DOMAIN_RID,
                                         DOMAIN_ALIAS_RID_ADMINS,
                                         0, 0, 0, 0, 0, 0,
                                         &DomainSid);
    if (!NT_SUCCESS(Status))
    {
        MIDL_user_free(DomainsBuffer->Domains);
        MIDL_user_free(DomainsBuffer);
        MIDL_user_free(SidsBuffer);
        return Status;
    }

    DomainSidLength = RtlLengthSid(DomainSid);

    for (i = 0; i < Count; i++)
    {
        DomainsBuffer->Domains[i].Sid = MIDL_user_allocate(DomainSidLength);
        RtlCopyMemory(DomainsBuffer->Domains[i].Sid,
                      DomainSid,
                      DomainSidLength);

        DomainsBuffer->Domains[i].Name.Buffer = MIDL_user_allocate(DomainName.MaximumLength);
        DomainsBuffer->Domains[i].Name.Length = DomainName.Length;
        DomainsBuffer->Domains[i].Name.MaximumLength = DomainName.MaximumLength;
        RtlCopyMemory(DomainsBuffer->Domains[i].Name.Buffer,
                      DomainName.Buffer,
                      DomainName.MaximumLength);
    }

    Status = RtlAllocateAndInitializeSid(&IdentifierAuthority,
                                         3,
                                         SECURITY_BUILTIN_DOMAIN_RID,
                                         DOMAIN_ALIAS_RID_ADMINS,
                                         DOMAIN_USER_RID_ADMIN,
                                         0, 0, 0, 0, 0,
                                         &AccountSid);
    if (!NT_SUCCESS(Status))
    {
        MIDL_user_free(DomainsBuffer->Domains);
        MIDL_user_free(DomainsBuffer);
        MIDL_user_free(SidsBuffer);
        return Status;
    }

    AccountSidLength = RtlLengthSid(AccountSid);

    for (i = 0; i < Count; i++)
    {
        SidsBuffer[i].Use = SidTypeWellKnownGroup;
        SidsBuffer[i].Sid = MIDL_user_allocate(AccountSidLength);

        RtlCopyMemory(SidsBuffer[i].Sid,
                      AccountSid,
                      AccountSidLength);

        SidsBuffer[i].DomainIndex = i;
        SidsBuffer[i].Flags = 0;
    }

    *ReferencedDomains = DomainsBuffer;
    *MappedCount = Count;

    TranslatedSids->Entries = Count;
    TranslatedSids->Sids = SidsBuffer;

    return STATUS_SUCCESS;
}


/* Function 69 */
NTSTATUS WINAPI CredrGetSessionTypes(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 70 */
NTSTATUS WINAPI LsarRegisterAuditEvent(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 71 */
NTSTATUS WINAPI LsarGenAuditEvent(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 72 */
NTSTATUS WINAPI LsarUnregisterAuditEvent(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 73 */
NTSTATUS WINAPI LsarQueryForestTrustInformation(
    LSAPR_HANDLE PolicyHandle,
    PLSA_UNICODE_STRING TrustedDomainName,
    LSA_FOREST_TRUST_RECORD_TYPE HighestRecordType,
    PLSA_FOREST_TRUST_INFORMATION *ForestTrustInfo)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 74 */
NTSTATUS WINAPI LsarSetForestTrustInformation(
    LSAPR_HANDLE PolicyHandle,
    PLSA_UNICODE_STRING TrustedDomainName,
    LSA_FOREST_TRUST_RECORD_TYPE HighestRecordType,
    PLSA_FOREST_TRUST_INFORMATION ForestTrustInfo,
    BOOL CheckOnly,
    PLSA_FOREST_TRUST_COLLISION_INFORMATION *CollisionInfo)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 75 */
NTSTATUS WINAPI CredrRename(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 76 */
NTSTATUS WINAPI LsarLookupSids3(
    LSAPR_HANDLE PolicyHandle,
    PLSAPR_SID_ENUM_BUFFER SidEnumBuffer,
    PLSAPR_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    PLSAPR_TRANSLATED_NAMES_EX TranslatedNames,
    LSAP_LOOKUP_LEVEL LookupLevel,
    DWORD *MappedCount,
    DWORD LookupOptions,
    DWORD ClientRevision)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 77 */
NTSTATUS WINAPI LsarLookupNames4(
    handle_t RpcHandle,
    DWORD Count,
    PRPC_UNICODE_STRING Names,
    PLSAPR_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    PLSAPR_TRANSLATED_SIDS_EX2 TranslatedSids,
    LSAP_LOOKUP_LEVEL LookupLevel,
    DWORD *MappedCount,
    DWORD LookupOptions,
    DWORD ClientRevision)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 78 */
NTSTATUS WINAPI LsarOpenPolicySce(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 79 */
NTSTATUS WINAPI LsarAdtRegisterSecurityEventSource(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 80 */
NTSTATUS WINAPI LsarAdtUnregisterSecurityEventSource(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 81 */
NTSTATUS WINAPI LsarAdtReportSecurityEvent(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 82 */
NTSTATUS WINAPI CredrFindBestCredential(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 83 */
NTSTATUS WINAPI LsarSetAuditPolicy(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 84 */
NTSTATUS WINAPI LsarQueryAuditPolicy(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 85 */
NTSTATUS WINAPI LsarEnumerateAuditPolicy(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 86 */
NTSTATUS WINAPI LsarEnumerateAuditCategories(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 87 */
NTSTATUS WINAPI LsarEnumerateAuditSubCategories(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 88 */
NTSTATUS WINAPI LsarLookupAuditCategoryName(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 89 */
NTSTATUS WINAPI LsarLookupAuditSubCategoryName(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 90 */
NTSTATUS WINAPI LsarSetAuditSecurity(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 91 */
NTSTATUS WINAPI LsarQueryAuditSecurity(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 92 */
NTSTATUS WINAPI CredReadByTokenHandle(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 93 */
NTSTATUS WINAPI CredrRestoreCredentials(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 94 */
NTSTATUS WINAPI CredrBackupCredentials(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
