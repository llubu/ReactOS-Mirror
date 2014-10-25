/*
 * COPYRIGHT:       See COPYING in the top level directory
 * WINE COPYRIGHT:
 * Copyright 1999, 2000 Juergen Schmied <juergen.schmied@debitel.net>
 * Copyright 2003 CodeWeavers Inc. (Ulrich Czekalla)
 * Copyright 2006 Robert Reif
 * Copyright 2006 Herv� Poussineau
 *
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/advapi32/wine/security.c
 */

#include <advapi32.h>

#include <sddl.h>

WINE_DEFAULT_DEBUG_CHANNEL(advapi);

static DWORD ComputeStringSidSize(LPCWSTR StringSid);
static BOOL ParseStringSidToSid(LPCWSTR StringSid, PSID pSid, LPDWORD cBytes);

#define MAX_GUID_STRING_LEN 39

BOOL WINAPI
AddAuditAccessAceEx(PACL pAcl,
                    DWORD dwAceRevision,
                    DWORD AceFlags,
                    DWORD dwAccessMask,
                    PSID pSid,
                    BOOL bAuditSuccess,
                    BOOL bAuditFailure);

typedef struct RECORD
{
    LPCWSTR key;
    DWORD value;
} RECORD;


typedef struct _MAX_SID
{
    /* same fields as struct _SID */
    BYTE Revision;
    BYTE SubAuthorityCount;
    SID_IDENTIFIER_AUTHORITY IdentifierAuthority;
    DWORD SubAuthority[SID_MAX_SUB_AUTHORITIES];
} MAX_SID;

typedef struct WELLKNOWNSID
{
    WCHAR wstr[2];
    WELL_KNOWN_SID_TYPE Type;
    MAX_SID Sid;
} WELLKNOWNSID;

typedef struct _ACEFLAG
{
   LPCWSTR wstr;
   DWORD value;
} ACEFLAG, *LPACEFLAG;

static const WELLKNOWNSID WellKnownSids[] =
{
    { {0,0}, WinNullSid, { SID_REVISION, 1, { SECURITY_NULL_SID_AUTHORITY }, { SECURITY_NULL_RID } } },
    { {'W','D'}, WinWorldSid, { SID_REVISION, 1, { SECURITY_WORLD_SID_AUTHORITY }, { SECURITY_WORLD_RID } } },
    { {0,0}, WinLocalSid, { SID_REVISION, 1, { SECURITY_LOCAL_SID_AUTHORITY }, { SECURITY_LOCAL_RID } } },
    { {'C','O'}, WinCreatorOwnerSid, { SID_REVISION, 1, { SECURITY_CREATOR_SID_AUTHORITY }, { SECURITY_CREATOR_OWNER_RID } } },
    { {'C','G'}, WinCreatorGroupSid, { SID_REVISION, 1, { SECURITY_CREATOR_SID_AUTHORITY }, { SECURITY_CREATOR_GROUP_RID } } },
    { {0,0}, WinCreatorOwnerServerSid, { SID_REVISION, 1, { SECURITY_CREATOR_SID_AUTHORITY }, { SECURITY_CREATOR_OWNER_SERVER_RID } } },
    { {0,0}, WinCreatorGroupServerSid, { SID_REVISION, 1, { SECURITY_CREATOR_SID_AUTHORITY }, { SECURITY_CREATOR_GROUP_SERVER_RID } } },
    { {0,0}, WinNtAuthoritySid, { SID_REVISION, 0, { SECURITY_NT_AUTHORITY }, { SECURITY_NULL_RID } } },
    { {0,0}, WinDialupSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_DIALUP_RID } } },
    { {'N','U'}, WinNetworkSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_NETWORK_RID } } },
    { {0,0}, WinBatchSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_BATCH_RID } } },
    { {'I','U'}, WinInteractiveSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_INTERACTIVE_RID } } },
    { {'S','U'}, WinServiceSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_SERVICE_RID } } },
    { {'A','N'}, WinAnonymousSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_ANONYMOUS_LOGON_RID } } },
    { {0,0}, WinProxySid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_PROXY_RID } } },
    { {'E','D'}, WinEnterpriseControllersSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_ENTERPRISE_CONTROLLERS_RID } } },
    { {'P','S'}, WinSelfSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_PRINCIPAL_SELF_RID } } },
    { {'A','U'}, WinAuthenticatedUserSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_AUTHENTICATED_USER_RID } } },
    { {'R','C'}, WinRestrictedCodeSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_RESTRICTED_CODE_RID } } },
    { {0,0}, WinTerminalServerSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_TERMINAL_SERVER_RID } } },
    { {0,0}, WinRemoteLogonIdSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_REMOTE_LOGON_RID } } },
    { {'S','Y'}, WinLocalSystemSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_LOCAL_SYSTEM_RID } } },
    { {'L','S'}, WinLocalServiceSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_LOCAL_SERVICE_RID } } },
    { {'N','S'}, WinNetworkServiceSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_NETWORK_SERVICE_RID } } },
    { {0,0}, WinBuiltinDomainSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID } } },
    { {'B','A'}, WinBuiltinAdministratorsSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS } } },
    { {'B','U'}, WinBuiltinUsersSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_USERS } } },
    { {'B','G'}, WinBuiltinGuestsSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_GUESTS } } },
    { {'P','U'}, WinBuiltinPowerUsersSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_POWER_USERS } } },
    { {'A','O'}, WinBuiltinAccountOperatorsSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ACCOUNT_OPS } } },
    { {'S','O'}, WinBuiltinSystemOperatorsSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_SYSTEM_OPS } } },
    { {'P','O'}, WinBuiltinPrintOperatorsSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_PRINT_OPS } } },
    { {'B','O'}, WinBuiltinBackupOperatorsSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_BACKUP_OPS } } },
    { {'R','E'}, WinBuiltinReplicatorSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_REPLICATOR } } },
    { {'R','U'}, WinBuiltinPreWindows2000CompatibleAccessSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_PREW2KCOMPACCESS } } },
    { {'R','D'}, WinBuiltinRemoteDesktopUsersSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_REMOTE_DESKTOP_USERS } } },
    { {'N','O'}, WinBuiltinNetworkConfigurationOperatorsSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_NETWORK_CONFIGURATION_OPS } } },
    { {0,0}, WinNTLMAuthenticationSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_PACKAGE_BASE_RID, SECURITY_PACKAGE_NTLM_RID } } },
    { {0,0}, WinDigestAuthenticationSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_PACKAGE_BASE_RID, SECURITY_PACKAGE_DIGEST_RID } } },
    { {0,0}, WinSChannelAuthenticationSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_PACKAGE_BASE_RID, SECURITY_PACKAGE_SCHANNEL_RID } } },
    { {0,0}, WinThisOrganizationSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_THIS_ORGANIZATION_RID } } },
    { {0,0}, WinOtherOrganizationSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_OTHER_ORGANIZATION_RID } } },
    { {0,0}, WinBuiltinIncomingForestTrustBuildersSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_INCOMING_FOREST_TRUST_BUILDERS  } } },
    { {0,0}, WinBuiltinPerfMonitoringUsersSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_MONITORING_USERS } } },
    { {0,0}, WinBuiltinPerfLoggingUsersSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_LOGGING_USERS } } },
    { {0,0}, WinBuiltinAuthorizationAccessSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_AUTHORIZATIONACCESS } } },
    { {0,0}, WinBuiltinTerminalServerLicenseServersSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_TS_LICENSE_SERVERS } } },
    { {0,0}, WinBuiltinDCOMUsersSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_DCOM_USERS } } },
    { {'L','W'}, WinLowLabelSid, { SID_REVISION, 1, { SECURITY_MANDATORY_LABEL_AUTHORITY}, { SECURITY_MANDATORY_LOW_RID} } },
    { {'M','E'}, WinMediumLabelSid, { SID_REVISION, 1, { SECURITY_MANDATORY_LABEL_AUTHORITY}, { SECURITY_MANDATORY_MEDIUM_RID } } },
    { {'H','I'}, WinHighLabelSid, { SID_REVISION, 1, { SECURITY_MANDATORY_LABEL_AUTHORITY}, { SECURITY_MANDATORY_HIGH_RID } } },
    { {'S','I'}, WinSystemLabelSid, { SID_REVISION, 1, { SECURITY_MANDATORY_LABEL_AUTHORITY}, { SECURITY_MANDATORY_SYSTEM_RID } } },
};

typedef struct WELLKNOWNRID
{
    WELL_KNOWN_SID_TYPE Type;
    DWORD Rid;
} WELLKNOWNRID;

static const WELLKNOWNRID WellKnownRids[] = {
    { WinAccountAdministratorSid,    DOMAIN_USER_RID_ADMIN },
    { WinAccountGuestSid,            DOMAIN_USER_RID_GUEST },
    { WinAccountKrbtgtSid,           DOMAIN_USER_RID_KRBTGT },
    { WinAccountDomainAdminsSid,     DOMAIN_GROUP_RID_ADMINS },
    { WinAccountDomainUsersSid,      DOMAIN_GROUP_RID_USERS },
    { WinAccountDomainGuestsSid,     DOMAIN_GROUP_RID_GUESTS },
    { WinAccountComputersSid,        DOMAIN_GROUP_RID_COMPUTERS },
    { WinAccountControllersSid,      DOMAIN_GROUP_RID_CONTROLLERS },
    { WinAccountCertAdminsSid,       DOMAIN_GROUP_RID_CERT_ADMINS },
    { WinAccountSchemaAdminsSid,     DOMAIN_GROUP_RID_SCHEMA_ADMINS },
    { WinAccountEnterpriseAdminsSid, DOMAIN_GROUP_RID_ENTERPRISE_ADMINS },
    { WinAccountPolicyAdminsSid,     DOMAIN_GROUP_RID_POLICY_ADMINS },
    { WinAccountRasAndIasServersSid, DOMAIN_ALIAS_RID_RAS_SERVERS },
};

static const SID sidWorld = { SID_REVISION, 1, { SECURITY_WORLD_SID_AUTHORITY} , { SECURITY_WORLD_RID } };

/*
 * ACE types
 */
static const WCHAR SDDL_ACCESS_ALLOWED[]        = {'A',0};
static const WCHAR SDDL_ACCESS_DENIED[]         = {'D',0};
static const WCHAR SDDL_OBJECT_ACCESS_ALLOWED[] = {'O','A',0};
static const WCHAR SDDL_OBJECT_ACCESS_DENIED[]  = {'O','D',0};
static const WCHAR SDDL_AUDIT[]                 = {'A','U',0};
static const WCHAR SDDL_ALARM[]                 = {'A','L',0};
static const WCHAR SDDL_OBJECT_AUDIT[]          = {'O','U',0};
static const WCHAR SDDL_OBJECT_ALARM[]          = {'O','L',0};

/*
 * SDDL ADS Rights
 */
#define ADS_RIGHT_DS_CREATE_CHILD   0x0001
#define ADS_RIGHT_DS_DELETE_CHILD   0x0002
#define ADS_RIGHT_ACTRL_DS_LIST     0x0004
#define ADS_RIGHT_DS_SELF           0x0008
#define ADS_RIGHT_DS_READ_PROP      0x0010
#define ADS_RIGHT_DS_WRITE_PROP     0x0020
#define ADS_RIGHT_DS_DELETE_TREE    0x0040
#define ADS_RIGHT_DS_LIST_OBJECT    0x0080
#define ADS_RIGHT_DS_CONTROL_ACCESS 0x0100

/*
 * ACE flags
 */
static const WCHAR SDDL_CONTAINER_INHERIT[]  = {'C','I',0};
static const WCHAR SDDL_OBJECT_INHERIT[]     = {'O','I',0};
static const WCHAR SDDL_NO_PROPAGATE[]       = {'N','P',0};
static const WCHAR SDDL_INHERIT_ONLY[]       = {'I','O',0};
static const WCHAR SDDL_INHERITED[]          = {'I','D',0};
static const WCHAR SDDL_AUDIT_SUCCESS[]      = {'S','A',0};
static const WCHAR SDDL_AUDIT_FAILURE[]      = {'F','A',0};

static const char * debugstr_sid(PSID sid)
{
    int auth = 0;
    SID * psid = (SID *)sid;

    if (psid == NULL)
        return "(null)";

    auth = psid->IdentifierAuthority.Value[5] +
           (psid->IdentifierAuthority.Value[4] << 8) +
           (psid->IdentifierAuthority.Value[3] << 16) +
           (psid->IdentifierAuthority.Value[2] << 24);

    switch (psid->SubAuthorityCount) {
    case 0:
        return wine_dbg_sprintf("S-%d-%d", psid->Revision, auth);
    case 1:
        return wine_dbg_sprintf("S-%d-%d-%lu", psid->Revision, auth,
            psid->SubAuthority[0]);
    case 2:
        return wine_dbg_sprintf("S-%d-%d-%lu-%lu", psid->Revision, auth,
            psid->SubAuthority[0], psid->SubAuthority[1]);
    case 3:
        return wine_dbg_sprintf("S-%d-%d-%lu-%lu-%lu", psid->Revision, auth,
            psid->SubAuthority[0], psid->SubAuthority[1], psid->SubAuthority[2]);
    case 4:
        return wine_dbg_sprintf("S-%d-%d-%lu-%lu-%lu-%lu", psid->Revision, auth,
            psid->SubAuthority[0], psid->SubAuthority[1], psid->SubAuthority[2],
            psid->SubAuthority[3]);
    case 5:
        return wine_dbg_sprintf("S-%d-%d-%lu-%lu-%lu-%lu-%lu", psid->Revision, auth,
            psid->SubAuthority[0], psid->SubAuthority[1], psid->SubAuthority[2],
            psid->SubAuthority[3], psid->SubAuthority[4]);
    case 6:
        return wine_dbg_sprintf("S-%d-%d-%lu-%lu-%lu-%lu-%lu-%lu", psid->Revision, auth,
            psid->SubAuthority[3], psid->SubAuthority[1], psid->SubAuthority[2],
            psid->SubAuthority[0], psid->SubAuthority[4], psid->SubAuthority[5]);
    case 7:
        return wine_dbg_sprintf("S-%d-%d-%lu-%lu-%lu-%lu-%lu-%lu-%lu", psid->Revision, auth,
            psid->SubAuthority[0], psid->SubAuthority[1], psid->SubAuthority[2],
            psid->SubAuthority[3], psid->SubAuthority[4], psid->SubAuthority[5],
            psid->SubAuthority[6]);
    case 8:
        return wine_dbg_sprintf("S-%d-%d-%lu-%lu-%lu-%lu-%lu-%lu-%lu-%lu", psid->Revision, auth,
            psid->SubAuthority[0], psid->SubAuthority[1], psid->SubAuthority[2],
            psid->SubAuthority[3], psid->SubAuthority[4], psid->SubAuthority[5],
            psid->SubAuthority[6], psid->SubAuthority[7]);
    }
    return "(too-big)";
}

static const ACEFLAG AceRights[] =
{
    { SDDL_GENERIC_ALL,     GENERIC_ALL },
    { SDDL_GENERIC_READ,    GENERIC_READ },
    { SDDL_GENERIC_WRITE,   GENERIC_WRITE },
    { SDDL_GENERIC_EXECUTE, GENERIC_EXECUTE },

    { SDDL_READ_CONTROL,    READ_CONTROL },
    { SDDL_STANDARD_DELETE, DELETE },
    { SDDL_WRITE_DAC,       WRITE_DAC },
    { SDDL_WRITE_OWNER,     WRITE_OWNER },

    { SDDL_READ_PROPERTY,   ADS_RIGHT_DS_READ_PROP},
    { SDDL_WRITE_PROPERTY,  ADS_RIGHT_DS_WRITE_PROP},
    { SDDL_CREATE_CHILD,    ADS_RIGHT_DS_CREATE_CHILD},
    { SDDL_DELETE_CHILD,    ADS_RIGHT_DS_DELETE_CHILD},
    { SDDL_LIST_CHILDREN,   ADS_RIGHT_ACTRL_DS_LIST},
    { SDDL_SELF_WRITE,      ADS_RIGHT_DS_SELF},
    { SDDL_LIST_OBJECT,     ADS_RIGHT_DS_LIST_OBJECT},
    { SDDL_DELETE_TREE,     ADS_RIGHT_DS_DELETE_TREE},
    { SDDL_CONTROL_ACCESS,  ADS_RIGHT_DS_CONTROL_ACCESS},

    { SDDL_FILE_ALL,        FILE_ALL_ACCESS },
    { SDDL_FILE_READ,       FILE_GENERIC_READ },
    { SDDL_FILE_WRITE,      FILE_GENERIC_WRITE },
    { SDDL_FILE_EXECUTE,    FILE_GENERIC_EXECUTE },

    { SDDL_KEY_ALL,         KEY_ALL_ACCESS },
    { SDDL_KEY_READ,        KEY_READ },
    { SDDL_KEY_WRITE,       KEY_WRITE },
    { SDDL_KEY_EXECUTE,     KEY_EXECUTE },
    { NULL, 0 },
};

/* set last error code from NT status and get the proper boolean return value */
/* used for functions that are a simple wrapper around the corresponding ntdll API */
static __inline BOOL set_ntstatus( NTSTATUS status )
{
    if (status) SetLastError( RtlNtStatusToDosError( status ));
    return !status;
}

static const RECORD SidTable[] =
{
	{ SDDL_ACCOUNT_OPERATORS, WinBuiltinAccountOperatorsSid },
	{ SDDL_ALIAS_PREW2KCOMPACC, WinBuiltinPreWindows2000CompatibleAccessSid },
	{ SDDL_ANONYMOUS, WinAnonymousSid },
	{ SDDL_AUTHENTICATED_USERS, WinAuthenticatedUserSid },
	{ SDDL_BUILTIN_ADMINISTRATORS, WinBuiltinAdministratorsSid },
	{ SDDL_BUILTIN_GUESTS, WinBuiltinGuestsSid },
	{ SDDL_BACKUP_OPERATORS, WinBuiltinBackupOperatorsSid },
	{ SDDL_BUILTIN_USERS, WinBuiltinUsersSid },
	{ SDDL_CERT_SERV_ADMINISTRATORS, WinAccountCertAdminsSid /* FIXME: DOMAIN_GROUP_RID_CERT_ADMINS */ },
	{ SDDL_CREATOR_GROUP, WinCreatorGroupSid },
	{ SDDL_CREATOR_OWNER, WinCreatorOwnerSid },
	{ SDDL_DOMAIN_ADMINISTRATORS, WinAccountDomainAdminsSid /* FIXME: DOMAIN_GROUP_RID_ADMINS */ },
	{ SDDL_DOMAIN_COMPUTERS, WinAccountComputersSid /* FIXME: DOMAIN_GROUP_RID_COMPUTERS */ },
	{ SDDL_DOMAIN_DOMAIN_CONTROLLERS, WinAccountControllersSid /* FIXME: DOMAIN_GROUP_RID_CONTROLLERS */ },
	{ SDDL_DOMAIN_GUESTS, WinAccountDomainGuestsSid /* FIXME: DOMAIN_GROUP_RID_GUESTS */ },
	{ SDDL_DOMAIN_USERS, WinAccountDomainUsersSid /* FIXME: DOMAIN_GROUP_RID_USERS */ },
	{ SDDL_ENTERPRISE_ADMINS, WinAccountEnterpriseAdminsSid /* FIXME: DOMAIN_GROUP_RID_ENTERPRISE_ADMINS */ },
	{ SDDL_ENTERPRISE_DOMAIN_CONTROLLERS, WinEnterpriseControllersSid },
	{ SDDL_EVERYONE, WinWorldSid },
	{ SDDL_GROUP_POLICY_ADMINS, WinAccountPolicyAdminsSid /* FIXME: DOMAIN_GROUP_RID_POLICY_ADMINS */ },
	{ SDDL_INTERACTIVE, WinInteractiveSid },
	{ SDDL_LOCAL_ADMIN, WinAccountAdministratorSid /* FIXME: DOMAIN_USER_RID_ADMIN */ },
	{ SDDL_LOCAL_GUEST, WinAccountGuestSid /* FIXME: DOMAIN_USER_RID_GUEST */ },
	{ SDDL_LOCAL_SERVICE, WinLocalServiceSid },
	{ SDDL_LOCAL_SYSTEM, WinLocalSystemSid },
	{ SDDL_NETWORK, WinNetworkSid },
	{ SDDL_NETWORK_CONFIGURATION_OPS, WinBuiltinNetworkConfigurationOperatorsSid },
	{ SDDL_NETWORK_SERVICE, WinNetworkServiceSid },
	{ SDDL_PRINTER_OPERATORS, WinBuiltinPrintOperatorsSid },
	{ SDDL_PERSONAL_SELF, WinSelfSid },
	{ SDDL_POWER_USERS, WinBuiltinPowerUsersSid },
	{ SDDL_RAS_SERVERS, WinAccountRasAndIasServersSid /* FIXME: DOMAIN_ALIAS_RID_RAS_SERVERS */ },
	{ SDDL_REMOTE_DESKTOP, WinBuiltinRemoteDesktopUsersSid },
	{ SDDL_REPLICATOR, WinBuiltinReplicatorSid },
	{ SDDL_RESTRICTED_CODE, WinRestrictedCodeSid },
	{ SDDL_SCHEMA_ADMINISTRATORS, WinAccountSchemaAdminsSid /* FIXME: DOMAIN_GROUP_RID_SCHEMA_ADMINS */ },
	{ SDDL_SERVER_OPERATORS, WinBuiltinSystemOperatorsSid },
	{ SDDL_SERVICE, WinServiceSid },
	{ NULL, 0 },
};

/************************************************************
 *                ADVAPI_IsLocalComputer
 *
 * Checks whether the server name indicates local machine.
 */
BOOL ADVAPI_IsLocalComputer(LPCWSTR ServerName)
{
    DWORD dwSize = MAX_COMPUTERNAME_LENGTH + 1;
    BOOL Result;
    LPWSTR buf;

    if (!ServerName || !ServerName[0])
        return TRUE;

    buf = HeapAlloc(GetProcessHeap(), 0, dwSize * sizeof(WCHAR));
    Result = GetComputerNameW(buf,  &dwSize);
    if (Result && (ServerName[0] == '\\') && (ServerName[1] == '\\'))
        ServerName += 2;
    Result = Result && !lstrcmpW(ServerName, buf);
    HeapFree(GetProcessHeap(), 0, buf);

    return Result;
}

/* Exported functions */

/*
 * @implemented
 */
BOOL WINAPI
OpenProcessToken(HANDLE ProcessHandle,
                 DWORD DesiredAccess,
                 PHANDLE TokenHandle)
{
    NTSTATUS Status;

    TRACE("%p, %x, %p.\n", ProcessHandle, DesiredAccess, TokenHandle);

    Status = NtOpenProcessToken(ProcessHandle,
                                DesiredAccess,
                                TokenHandle);
    if (!NT_SUCCESS(Status))
    {
        ERR("NtOpenProcessToken failed! Status %08x.\n", Status);
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    TRACE("Returning token %p.\n", *TokenHandle);

    return TRUE;
}

/*
 * @implemented
 */
BOOL WINAPI
OpenThreadToken(HANDLE ThreadHandle,
                DWORD DesiredAccess,
                BOOL OpenAsSelf,
                PHANDLE TokenHandle)
{
    NTSTATUS Status;

    Status = NtOpenThreadToken(ThreadHandle,
                               DesiredAccess,
                               OpenAsSelf,
                               TokenHandle);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL WINAPI
AdjustTokenGroups(HANDLE TokenHandle,
                  BOOL ResetToDefault,
                  PTOKEN_GROUPS NewState,
                  DWORD BufferLength,
                  PTOKEN_GROUPS PreviousState,
                  PDWORD ReturnLength)
{
    NTSTATUS Status;

    Status = NtAdjustGroupsToken(TokenHandle,
                                 ResetToDefault,
                                 NewState,
                                 BufferLength,
                                 PreviousState,
                                 (PULONG)ReturnLength);
    if (!NT_SUCCESS(Status))
    {
       SetLastError(RtlNtStatusToDosError(Status));
       return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL WINAPI
AdjustTokenPrivileges(HANDLE TokenHandle,
                      BOOL DisableAllPrivileges,
                      PTOKEN_PRIVILEGES NewState,
                      DWORD BufferLength,
                      PTOKEN_PRIVILEGES PreviousState,
                      PDWORD ReturnLength)
{
    NTSTATUS Status;

    Status = NtAdjustPrivilegesToken(TokenHandle,
                                     DisableAllPrivileges,
                                     NewState,
                                     BufferLength,
                                     PreviousState,
                                     (PULONG)ReturnLength);
    if (STATUS_NOT_ALL_ASSIGNED == Status)
    {
        SetLastError(ERROR_NOT_ALL_ASSIGNED);
        return TRUE;
    }

    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    /* AdjustTokenPrivileges is documented to do this */
    SetLastError(ERROR_SUCCESS);

    return TRUE;
}

/*
 * @implemented
 */
BOOL WINAPI
GetTokenInformation(HANDLE TokenHandle,
                    TOKEN_INFORMATION_CLASS TokenInformationClass,
                    LPVOID TokenInformation,
                    DWORD TokenInformationLength,
                    PDWORD ReturnLength)
{
    NTSTATUS Status;

    Status = NtQueryInformationToken(TokenHandle,
                                     TokenInformationClass,
                                     TokenInformation,
                                     TokenInformationLength,
                                     (PULONG)ReturnLength);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

  return TRUE;
}

/*
 * @implemented
 */
BOOL WINAPI
SetTokenInformation(HANDLE TokenHandle,
                    TOKEN_INFORMATION_CLASS TokenInformationClass,
                    LPVOID TokenInformation,
                    DWORD TokenInformationLength)
{
    NTSTATUS Status;

    Status = NtSetInformationToken(TokenHandle,
                                   TokenInformationClass,
                                   TokenInformation,
                                   TokenInformationLength);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL WINAPI
SetThreadToken(IN PHANDLE ThreadHandle  OPTIONAL,
               IN HANDLE TokenHandle)
{
    NTSTATUS Status;
    HANDLE hThread;

    hThread = (ThreadHandle != NULL) ? *ThreadHandle : NtCurrentThread();

    Status = NtSetInformationThread(hThread,
                                    ThreadImpersonationToken,
                                    &TokenHandle,
                                    sizeof(HANDLE));
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}

BOOL WINAPI
CreateRestrictedToken(HANDLE TokenHandle,
                      DWORD Flags,
                      DWORD DisableSidCount,
                      PSID_AND_ATTRIBUTES pSidAndAttributes,
                      DWORD DeletePrivilegeCount,
                      PLUID_AND_ATTRIBUTES pLUIDAndAttributes,
                      DWORD RestrictedSidCount,
                      PSID_AND_ATTRIBUTES pSIDAndAttributes,
                      PHANDLE NewTokenHandle)
{
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @implemented
 */
BOOL WINAPI
AllocateAndInitializeSid(PSID_IDENTIFIER_AUTHORITY pIdentifierAuthority,
                         BYTE nSubAuthorityCount,
                         DWORD dwSubAuthority0,
                         DWORD dwSubAuthority1,
                         DWORD dwSubAuthority2,
                         DWORD dwSubAuthority3,
                         DWORD dwSubAuthority4,
                         DWORD dwSubAuthority5,
                         DWORD dwSubAuthority6,
                         DWORD dwSubAuthority7,
                         PSID *pSid)
{
    NTSTATUS Status;

    Status = RtlAllocateAndInitializeSid(pIdentifierAuthority,
                                         nSubAuthorityCount,
                                         dwSubAuthority0,
                                         dwSubAuthority1,
                                         dwSubAuthority2,
                                         dwSubAuthority3,
                                         dwSubAuthority4,
                                         dwSubAuthority5,
                                         dwSubAuthority6,
                                         dwSubAuthority7,
                                         pSid);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 *
 * RETURNS
 *  Docs says this function does NOT return a value
 *  even thou it's defined to return a PVOID...
 */
PVOID
WINAPI
FreeSid(PSID pSid)
{
    return RtlFreeSid(pSid);
}

/*
 * @implemented
 */
BOOL WINAPI
CopySid(DWORD nDestinationSidLength,
        PSID pDestinationSid,
        PSID pSourceSid)
{
    NTSTATUS Status;

    Status = RtlCopySid(nDestinationSidLength,
                        pDestinationSid,
                        pSourceSid);
    if (!NT_SUCCESS (Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

  return TRUE;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
CreateWellKnownSid(IN WELL_KNOWN_SID_TYPE WellKnownSidType,
                   IN PSID DomainSid  OPTIONAL,
                   OUT PSID pSid,
                   IN OUT DWORD* cbSid)
{
    unsigned int i;
    TRACE("(%d, %s, %p, %p)\n", WellKnownSidType, debugstr_sid(DomainSid), pSid, cbSid);

    if (cbSid == NULL || (DomainSid && !IsValidSid(DomainSid)))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    for (i = 0; i < sizeof(WellKnownSids)/sizeof(WellKnownSids[0]); i++) {
        if (WellKnownSids[i].Type == WellKnownSidType) {
            DWORD length = GetSidLengthRequired(WellKnownSids[i].Sid.SubAuthorityCount);

            if (*cbSid < length)
            {
                *cbSid = length;
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                return FALSE;
            }
            if (!pSid)
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return FALSE;
            }
            CopyMemory(pSid, &WellKnownSids[i].Sid.Revision, length);
            *cbSid = length;
            return TRUE;
        }
    }

    if (DomainSid == NULL || *GetSidSubAuthorityCount(DomainSid) == SID_MAX_SUB_AUTHORITIES)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    for (i = 0; i < sizeof(WellKnownRids)/sizeof(WellKnownRids[0]); i++)
        if (WellKnownRids[i].Type == WellKnownSidType) {
            UCHAR domain_subauth = *GetSidSubAuthorityCount(DomainSid);
            DWORD domain_sid_length = GetSidLengthRequired(domain_subauth);
            DWORD output_sid_length = GetSidLengthRequired(domain_subauth + 1);

            if (*cbSid < output_sid_length)
            {
                *cbSid = output_sid_length;
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                return FALSE;
            }
            if (!pSid)
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return FALSE;
            }
            CopyMemory(pSid, DomainSid, domain_sid_length);
            (*GetSidSubAuthorityCount(pSid))++;
            (*GetSidSubAuthority(pSid, domain_subauth)) = WellKnownRids[i].Rid;
            *cbSid = output_sid_length;
            return TRUE;
        }

    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
IsWellKnownSid(IN PSID pSid,
               IN WELL_KNOWN_SID_TYPE WellKnownSidType)
{
    unsigned int i;
    TRACE("(%s, %d)\n", debugstr_sid(pSid), WellKnownSidType);

    for (i = 0; i < sizeof(WellKnownSids) / sizeof(WellKnownSids[0]); i++)
    {
        if (WellKnownSids[i].Type == WellKnownSidType)
        {
            if (EqualSid(pSid, (PSID)(&WellKnownSids[i].Sid.Revision)))
                return TRUE;
        }
    }

    return FALSE;
}

/*
 * @implemented
 */
BOOL
WINAPI
IsValidSid(PSID pSid)
{
    return (BOOL)RtlValidSid(pSid);
}

/*
 * @implemented
 */
BOOL
WINAPI
EqualSid(PSID pSid1,
         PSID pSid2)
{
    SetLastError(ERROR_SUCCESS);
    return RtlEqualSid (pSid1, pSid2);
}

/*
 * @implemented
 */
BOOL
WINAPI
EqualPrefixSid(PSID pSid1,
               PSID pSid2)
{
    return RtlEqualPrefixSid (pSid1, pSid2);
}

/*
 * @implemented
 */
DWORD
WINAPI
GetSidLengthRequired(UCHAR nSubAuthorityCount)
{
    return (DWORD)RtlLengthRequiredSid(nSubAuthorityCount);
}

/*
 * @implemented
 */
BOOL
WINAPI
InitializeSid(PSID Sid,
              PSID_IDENTIFIER_AUTHORITY pIdentifierAuthority,
              BYTE nSubAuthorityCount)
{
    NTSTATUS Status;

    Status = RtlInitializeSid(Sid,
                              pIdentifierAuthority,
                              nSubAuthorityCount);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
PSID_IDENTIFIER_AUTHORITY
WINAPI
GetSidIdentifierAuthority(PSID pSid)
{
    return RtlIdentifierAuthoritySid(pSid);
}

/*
 * @implemented
 */
PDWORD
WINAPI
GetSidSubAuthority(PSID pSid,
                   DWORD nSubAuthority)
{
    SetLastError(ERROR_SUCCESS);
    return (PDWORD)RtlSubAuthoritySid(pSid, nSubAuthority);
}

/*
 * @implemented
 */
PUCHAR
WINAPI
GetSidSubAuthorityCount(PSID pSid)
{
    SetLastError(ERROR_SUCCESS);
    return RtlSubAuthorityCountSid(pSid);
}

/*
 * @implemented
 */
DWORD
WINAPI
GetLengthSid(PSID pSid)
{
    return (DWORD)RtlLengthSid(pSid);
}

/*
 * @implemented
 */
BOOL
WINAPI
GetKernelObjectSecurity(HANDLE Handle,
                        SECURITY_INFORMATION RequestedInformation,
                        PSECURITY_DESCRIPTOR pSecurityDescriptor,
                        DWORD nLength,
                        LPDWORD lpnLengthNeeded)
{
    NTSTATUS Status;

    Status = NtQuerySecurityObject(Handle,
                                   RequestedInformation,
                                   pSecurityDescriptor,
                                   nLength,
                                   lpnLengthNeeded);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
InitializeAcl(PACL pAcl,
              DWORD nAclLength,
              DWORD dwAclRevision)
{
    NTSTATUS Status;

    Status = RtlCreateAcl(pAcl,
                          nAclLength,
                          dwAclRevision);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}

/**********************************************************************
 * ImpersonateNamedPipeClient			EXPORTED
 *
 * @implemented
 */
BOOL
WINAPI
ImpersonateNamedPipeClient(HANDLE hNamedPipe)
{
    IO_STATUS_BLOCK StatusBlock;
    NTSTATUS Status;

    TRACE("ImpersonateNamedPipeClient() called\n");

    Status = NtFsControlFile(hNamedPipe,
                             NULL,
                             NULL,
                             NULL,
                             &StatusBlock,
                             FSCTL_PIPE_IMPERSONATE,
                             NULL,
                             0,
                             NULL,
                             0);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
AddAccessAllowedAce(PACL pAcl,
                    DWORD dwAceRevision,
                    DWORD AccessMask,
                    PSID pSid)
{
    NTSTATUS Status;

    Status = RtlAddAccessAllowedAce(pAcl,
                                    dwAceRevision,
                                    AccessMask,
                                    pSid);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL WINAPI
AddAccessAllowedAceEx(PACL pAcl,
                      DWORD dwAceRevision,
                      DWORD AceFlags,
                      DWORD AccessMask,
                      PSID pSid)
{
    NTSTATUS Status;

    Status = RtlAddAccessAllowedAceEx(pAcl,
                                      dwAceRevision,
                                      AceFlags,
                                      AccessMask,
                                      pSid);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
AddAccessDeniedAce(PACL pAcl,
                   DWORD dwAceRevision,
                   DWORD AccessMask,
                   PSID pSid)
{
    NTSTATUS Status;

    Status = RtlAddAccessDeniedAce(pAcl,
                                   dwAceRevision,
                                   AccessMask,
                                   pSid);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL WINAPI
AddAccessDeniedAceEx(PACL pAcl,
                     DWORD dwAceRevision,
                     DWORD AceFlags,
                     DWORD AccessMask,
                     PSID pSid)
{
    NTSTATUS Status;

    Status = RtlAddAccessDeniedAceEx(pAcl,
                                     dwAceRevision,
                                     AceFlags,
                                     AccessMask,
                                     pSid);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
AddAce(PACL pAcl,
       DWORD dwAceRevision,
       DWORD dwStartingAceIndex,
       LPVOID pAceList,
       DWORD nAceListLength)
{
    NTSTATUS Status;

    Status = RtlAddAce(pAcl,
                       dwAceRevision,
                       dwStartingAceIndex,
                       pAceList,
                       nAceListLength);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
DeleteAce(PACL pAcl,
          DWORD dwAceIndex)
{
    NTSTATUS Status;

    Status = RtlDeleteAce(pAcl,
                          dwAceIndex);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
FindFirstFreeAce(PACL pAcl,
                 LPVOID *pAce)
{
    return RtlFirstFreeAce(pAcl,
                           (PACE*)pAce);
}


/*
 * @implemented
 */
BOOL
WINAPI
GetAce(PACL pAcl,
       DWORD dwAceIndex,
       LPVOID *pAce)
{
    NTSTATUS Status;

    Status = RtlGetAce(pAcl,
                       dwAceIndex,
                       pAce);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
GetAclInformation(PACL pAcl,
                  LPVOID pAclInformation,
                  DWORD nAclInformationLength,
                  ACL_INFORMATION_CLASS dwAclInformationClass)
{
    NTSTATUS Status;

    Status = RtlQueryInformationAcl(pAcl,
                                    pAclInformation,
                                    nAclInformationLength,
                                    dwAclInformationClass);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
IsValidAcl(PACL pAcl)
{
    return RtlValidAcl (pAcl);
}

/*
 * @implemented
 */
BOOL WINAPI
AllocateLocallyUniqueId(PLUID Luid)
{
    NTSTATUS Status;

    Status = NtAllocateLocallyUniqueId (Luid);
    if (!NT_SUCCESS (Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}

/**********************************************************************
 * LookupPrivilegeDisplayNameA			EXPORTED
 *
 * @unimplemented
 */
BOOL
WINAPI
LookupPrivilegeDisplayNameA(LPCSTR lpSystemName,
                            LPCSTR lpName,
                            LPSTR lpDisplayName,
                            LPDWORD cbDisplayName,
                            LPDWORD lpLanguageId)
{
    FIXME("%s() not implemented!\n", __FUNCTION__);
    SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/**********************************************************************
 * LookupPrivilegeDisplayNameW			EXPORTED
 *
 * @unimplemented
 */
BOOL
WINAPI
LookupPrivilegeDisplayNameW(LPCWSTR lpSystemName,
                            LPCWSTR lpName,
                            LPWSTR lpDisplayName,
                            LPDWORD cbDisplayName,
                            LPDWORD lpLanguageId)
{
    FIXME("%s() not implemented!\n", __FUNCTION__);
    SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/**********************************************************************
 * LookupPrivilegeNameA				EXPORTED
 *
 * @implemented
 */
BOOL
WINAPI
LookupPrivilegeNameA(LPCSTR lpSystemName,
                     PLUID lpLuid,
                     LPSTR lpName,
                     LPDWORD cchName)
{
    UNICODE_STRING lpSystemNameW;
    BOOL ret;
    DWORD wLen = 0;

    TRACE("%s %p %p %p\n", debugstr_a(lpSystemName), lpLuid, lpName, cchName);

    RtlCreateUnicodeStringFromAsciiz(&lpSystemNameW, lpSystemName);
    ret = LookupPrivilegeNameW(lpSystemNameW.Buffer, lpLuid, NULL, &wLen);
    if (!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        LPWSTR lpNameW = HeapAlloc(GetProcessHeap(), 0, wLen * sizeof(WCHAR));

        ret = LookupPrivilegeNameW(lpSystemNameW.Buffer, lpLuid, lpNameW,
         &wLen);
        if (ret)
        {
            /* Windows crashes if cchName is NULL, so will I */
            unsigned int len = WideCharToMultiByte(CP_ACP, 0, lpNameW, -1, lpName,
             *cchName, NULL, NULL);

            if (len == 0)
            {
                /* WideCharToMultiByte failed */
                ret = FALSE;
            }
            else if (len > *cchName)
            {
                *cchName = len;
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                ret = FALSE;
            }
            else
            {
                /* WideCharToMultiByte succeeded, output length needs to be
                 * length not including NULL terminator
                 */
                *cchName = len - 1;
            }
        }
        HeapFree(GetProcessHeap(), 0, lpNameW);
    }
    RtlFreeUnicodeString(&lpSystemNameW);
    return ret;
}

/******************************************************************************
 * GetFileSecurityA [ADVAPI32.@]
 *
 * Obtains Specified information about the security of a file or directory.
 *
 * PARAMS
 *  lpFileName           [I] Name of the file to get info for
 *  RequestedInformation [I] SE_ flags from "winnt.h"
 *  pSecurityDescriptor  [O] Destination for security information
 *  nLength              [I] Length of pSecurityDescriptor
 *  lpnLengthNeeded      [O] Destination for length of returned security information
 *
 * RETURNS
 *  Success: TRUE. pSecurityDescriptor contains the requested information.
 *  Failure: FALSE. lpnLengthNeeded contains the required space to return the info.
 *
 * NOTES
 *  The information returned is constrained by the callers access rights and
 *  privileges.
 *
 * @implemented
 */
BOOL
WINAPI
GetFileSecurityA(LPCSTR lpFileName,
                 SECURITY_INFORMATION RequestedInformation,
                 PSECURITY_DESCRIPTOR pSecurityDescriptor,
                 DWORD nLength,
                 LPDWORD lpnLengthNeeded)
{
    UNICODE_STRING FileName;
    BOOL bResult;

    if (!RtlCreateUnicodeStringFromAsciiz(&FileName, lpFileName))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    bResult = GetFileSecurityW(FileName.Buffer,
                               RequestedInformation,
                               pSecurityDescriptor,
                               nLength,
                               lpnLengthNeeded);

    RtlFreeUnicodeString(&FileName);

    return bResult;
}

/*
 * @implemented
 */
BOOL
WINAPI
GetFileSecurityW(LPCWSTR lpFileName,
                 SECURITY_INFORMATION RequestedInformation,
                 PSECURITY_DESCRIPTOR pSecurityDescriptor,
                 DWORD nLength,
                 LPDWORD lpnLengthNeeded)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK StatusBlock;
    UNICODE_STRING FileName;
    ULONG AccessMask = 0;
    HANDLE FileHandle;
    NTSTATUS Status;

    TRACE("GetFileSecurityW() called\n");

    QuerySecurityAccessMask(RequestedInformation, &AccessMask);

    if (!RtlDosPathNameToNtPathName_U(lpFileName,
                                      &FileName,
                                      NULL,
                                      NULL))
    {
        ERR("Invalid path\n");
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    InitializeObjectAttributes(&ObjectAttributes,
                               &FileName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenFile(&FileHandle,
                        AccessMask,
                        &ObjectAttributes,
                        &StatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        0);

    RtlFreeHeap(RtlGetProcessHeap(),
                0,
                FileName.Buffer);

    if (!NT_SUCCESS(Status))
    {
        ERR("NtOpenFile() failed (Status %lx)\n", Status);
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    Status = NtQuerySecurityObject(FileHandle,
                                   RequestedInformation,
                                   pSecurityDescriptor,
                                   nLength,
                                   lpnLengthNeeded);
    NtClose(FileHandle);
    if (!NT_SUCCESS(Status))
    {
        ERR("NtQuerySecurityObject() failed (Status %lx)\n", Status);
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}

/******************************************************************************
 * SetFileSecurityA [ADVAPI32.@]
 * Sets the security of a file or directory
 *
 * @implemented
 */
BOOL
WINAPI
SetFileSecurityA(LPCSTR lpFileName,
                 SECURITY_INFORMATION SecurityInformation,
                 PSECURITY_DESCRIPTOR pSecurityDescriptor)
{
    UNICODE_STRING FileName;
    BOOL bResult;

    if (!RtlCreateUnicodeStringFromAsciiz(&FileName, lpFileName))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    bResult = SetFileSecurityW(FileName.Buffer,
                               SecurityInformation,
                               pSecurityDescriptor);

    RtlFreeUnicodeString(&FileName);

    return bResult;
}

/******************************************************************************
 * SetFileSecurityW [ADVAPI32.@]
 * Sets the security of a file or directory
 *
 * @implemented
 */
BOOL
WINAPI
SetFileSecurityW(LPCWSTR lpFileName,
                 SECURITY_INFORMATION SecurityInformation,
                 PSECURITY_DESCRIPTOR pSecurityDescriptor)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK StatusBlock;
    UNICODE_STRING FileName;
    ULONG AccessMask = 0;
    HANDLE FileHandle;
    NTSTATUS Status;

    TRACE("SetFileSecurityW() called\n");

    SetSecurityAccessMask(SecurityInformation, &AccessMask);

    if (!RtlDosPathNameToNtPathName_U(lpFileName,
                                      &FileName,
                                      NULL,
                                      NULL))
    {
        ERR("Invalid path\n");
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    InitializeObjectAttributes(&ObjectAttributes,
                               &FileName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenFile(&FileHandle,
                        AccessMask,
                        &ObjectAttributes,
                        &StatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        0);

    RtlFreeHeap(RtlGetProcessHeap(),
                0,
                FileName.Buffer);

    if (!NT_SUCCESS(Status))
    {
        ERR("NtOpenFile() failed (Status %lx)\n", Status);
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    Status = NtSetSecurityObject(FileHandle,
                                 SecurityInformation,
                                 pSecurityDescriptor);
    NtClose(FileHandle);

    if (!NT_SUCCESS(Status))
    {
        ERR("NtSetSecurityObject() failed (Status %lx)\n", Status);
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}

/******************************************************************************
 * QueryWindows31FilesMigration [ADVAPI32.@]
 *
 * PARAMS
 *   x1 []
 */
BOOL WINAPI
QueryWindows31FilesMigration( DWORD x1 )
{
	FIXME("(%d):stub\n",x1);
	return TRUE;
}

/******************************************************************************
 * SynchronizeWindows31FilesAndWindowsNTRegistry [ADVAPI32.@]
 *
 * PARAMS
 *   x1 []
 *   x2 []
 *   x3 []
 *   x4 []
 */
BOOL WINAPI
SynchronizeWindows31FilesAndWindowsNTRegistry( DWORD x1, DWORD x2, DWORD x3,
                                               DWORD x4 )
{
	FIXME("(0x%08x,0x%08x,0x%08x,0x%08x):stub\n",x1,x2,x3,x4);
	return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
RevertToSelf(VOID)
{
    NTSTATUS Status;
    HANDLE Token = NULL;

    Status = NtSetInformationThread(NtCurrentThread(),
                                    ThreadImpersonationToken,
                                    &Token,
                                    sizeof(HANDLE));
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
ImpersonateSelf(SECURITY_IMPERSONATION_LEVEL ImpersonationLevel)
{
    NTSTATUS Status;

    Status = RtlImpersonateSelf(ImpersonationLevel);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
AccessCheck(IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
            IN HANDLE ClientToken,
            IN DWORD DesiredAccess,
            IN PGENERIC_MAPPING GenericMapping,
            OUT PPRIVILEGE_SET PrivilegeSet OPTIONAL,
            IN OUT LPDWORD PrivilegeSetLength,
            OUT LPDWORD GrantedAccess,
            OUT LPBOOL AccessStatus)
{
    NTSTATUS Status;
    NTSTATUS NtAccessStatus;

    /* Do the access check */
    Status = NtAccessCheck(pSecurityDescriptor,
                           ClientToken,
                           DesiredAccess,
                           GenericMapping,
                           PrivilegeSet,
                           (PULONG)PrivilegeSetLength,
                           (PACCESS_MASK)GrantedAccess,
                           &NtAccessStatus);

    /* See if the access check operation succeeded */
    if (!NT_SUCCESS(Status))
    {
        /* Check failed */
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    /* Now check the access status  */
    if (!NT_SUCCESS(NtAccessStatus))
    {
        /* Access denied */
        SetLastError(RtlNtStatusToDosError(NtAccessStatus));
        *AccessStatus = FALSE;
    }
    else
    {
        /* Access granted */
        *AccessStatus = TRUE;
    }

    /* Check succeeded */
    return TRUE;
}

/*
 * @unimplemented
 */
BOOL WINAPI AccessCheckByType(
    PSECURITY_DESCRIPTOR pSecurityDescriptor, 
    PSID PrincipalSelfSid,
    HANDLE ClientToken, 
    DWORD DesiredAccess, 
    POBJECT_TYPE_LIST ObjectTypeList,
    DWORD ObjectTypeListLength,
    PGENERIC_MAPPING GenericMapping,
    PPRIVILEGE_SET PrivilegeSet,
    LPDWORD PrivilegeSetLength, 
    LPDWORD GrantedAccess,
    LPBOOL AccessStatus)
{
	FIXME("stub\n");

	*AccessStatus = TRUE;

	return !*AccessStatus;
}

/*
 * @implemented
 */
BOOL
WINAPI
SetKernelObjectSecurity(HANDLE Handle,
                        SECURITY_INFORMATION SecurityInformation,
                        PSECURITY_DESCRIPTOR SecurityDescriptor)
{
    NTSTATUS Status;

    Status = NtSetSecurityObject(Handle,
                                 SecurityInformation,
                                 SecurityDescriptor);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
AddAuditAccessAce(PACL pAcl,
                  DWORD dwAceRevision,
                  DWORD dwAccessMask,
                  PSID pSid,
                  BOOL bAuditSuccess,
                  BOOL bAuditFailure)
{
    NTSTATUS Status;

    Status = RtlAddAuditAccessAce(pAcl,
                                  dwAceRevision,
                                  dwAccessMask,
                                  pSid,
                                  bAuditSuccess,
                                  bAuditFailure);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL WINAPI
AddAuditAccessAceEx(PACL pAcl,
                    DWORD dwAceRevision,
                    DWORD AceFlags,
                    DWORD dwAccessMask,
                    PSID pSid,
                    BOOL bAuditSuccess,
                    BOOL bAuditFailure)
{
    NTSTATUS Status;

    Status = RtlAddAuditAccessAceEx(pAcl,
                                    dwAceRevision,
                                    AceFlags,
                                    dwAccessMask,
                                    pSid,
                                    bAuditSuccess,
                                    bAuditFailure);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}

/******************************************************************************
 * LookupAccountNameA [ADVAPI32.@]
 *
 * @implemented
 */
BOOL
WINAPI
LookupAccountNameA(LPCSTR SystemName,
                   LPCSTR AccountName,
                   PSID Sid,
                   LPDWORD SidLength,
                   LPSTR ReferencedDomainName,
                   LPDWORD hReferencedDomainNameLength,
                   PSID_NAME_USE SidNameUse)
{
    BOOL ret;
    UNICODE_STRING lpSystemW;
    UNICODE_STRING lpAccountW;
    LPWSTR lpReferencedDomainNameW = NULL;

    RtlCreateUnicodeStringFromAsciiz(&lpSystemW, SystemName);
    RtlCreateUnicodeStringFromAsciiz(&lpAccountW, AccountName);

    if (ReferencedDomainName)
        lpReferencedDomainNameW = HeapAlloc(GetProcessHeap(),
                                            0,
                                            *hReferencedDomainNameLength * sizeof(WCHAR));

    ret = LookupAccountNameW(lpSystemW.Buffer,
                             lpAccountW.Buffer,
                             Sid,
                             SidLength,
                             lpReferencedDomainNameW,
                             hReferencedDomainNameLength,
                             SidNameUse);

    if (ret && lpReferencedDomainNameW)
    {
        WideCharToMultiByte(CP_ACP,
                            0,
                            lpReferencedDomainNameW,
                            *hReferencedDomainNameLength + 1,
                            ReferencedDomainName,
                            *hReferencedDomainNameLength + 1,
                            NULL,
                            NULL);
    }

    RtlFreeUnicodeString(&lpSystemW);
    RtlFreeUnicodeString(&lpAccountW);
    HeapFree(GetProcessHeap(), 0, lpReferencedDomainNameW);

    return ret;
}

/**********************************************************************
 *	PrivilegeCheck					EXPORTED
 *
 * @implemented
 */
BOOL WINAPI
PrivilegeCheck(HANDLE ClientToken,
               PPRIVILEGE_SET RequiredPrivileges,
               LPBOOL pfResult)
{
    BOOLEAN Result;
    NTSTATUS Status;

    Status = NtPrivilegeCheck(ClientToken,
                              RequiredPrivileges,
                              &Result);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    *pfResult = (BOOL)Result;

    return TRUE;
}

/******************************************************************************
 * GetSecurityInfoExW         EXPORTED
 */
DWORD
WINAPI
GetSecurityInfoExA(HANDLE hObject,
                   SE_OBJECT_TYPE ObjectType,
                   SECURITY_INFORMATION SecurityInfo,
                   LPCSTR lpProvider,
                   LPCSTR lpProperty,
                   PACTRL_ACCESSA *ppAccessList,
                   PACTRL_AUDITA *ppAuditList,
                   LPSTR *lppOwner,
                   LPSTR *lppGroup)
{
    FIXME("%s() not implemented!\n", __FUNCTION__);
    return ERROR_BAD_PROVIDER;
}


/******************************************************************************
 * GetSecurityInfoExW         EXPORTED
 */
DWORD
WINAPI
GetSecurityInfoExW(HANDLE hObject,
                   SE_OBJECT_TYPE ObjectType,
                   SECURITY_INFORMATION SecurityInfo,
                   LPCWSTR lpProvider,
                   LPCWSTR lpProperty,
                   PACTRL_ACCESSW *ppAccessList,
                   PACTRL_AUDITW *ppAuditList,
                   LPWSTR *lppOwner,
                   LPWSTR *lppGroup)
{
    FIXME("%s() not implemented!\n", __FUNCTION__);
    return ERROR_BAD_PROVIDER;
}

/*
 * @implemented
 */
BOOL
WINAPI
SetAclInformation(PACL pAcl,
                  LPVOID pAclInformation,
                  DWORD nAclInformationLength,
                  ACL_INFORMATION_CLASS dwAclInformationClass)
{
    NTSTATUS Status;

    Status = RtlSetInformationAcl(pAcl,
                                  pAclInformation,
                                  nAclInformationLength,
                                  dwAclInformationClass);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}

/**********************************************************************
 * SetNamedSecurityInfoA			EXPORTED
 *
 * @implemented
 */
DWORD
WINAPI
SetNamedSecurityInfoA(LPSTR pObjectName,
                      SE_OBJECT_TYPE ObjectType,
                      SECURITY_INFORMATION SecurityInfo,
                      PSID psidOwner,
                      PSID psidGroup,
                      PACL pDacl,
                      PACL pSacl)
{
    UNICODE_STRING ObjectName;
    DWORD Ret;

    if (!RtlCreateUnicodeStringFromAsciiz(&ObjectName, pObjectName))
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    Ret = SetNamedSecurityInfoW(ObjectName.Buffer,
                                ObjectType,
                                SecurityInfo,
                                psidOwner,
                                psidGroup,
                                pDacl,
                                pSacl);

    RtlFreeUnicodeString(&ObjectName);

    return Ret;
}

/*
 * @implemented
 */
BOOL
WINAPI
AreAllAccessesGranted(DWORD GrantedAccess,
                      DWORD DesiredAccess)
{
    return (BOOL)RtlAreAllAccessesGranted(GrantedAccess,
                                          DesiredAccess);
}

/*
 * @implemented
 */
BOOL
WINAPI
AreAnyAccessesGranted(DWORD GrantedAccess,
                      DWORD DesiredAccess)
{
    return (BOOL)RtlAreAnyAccessesGranted(GrantedAccess,
                                          DesiredAccess);
}

/******************************************************************************
 * ParseAclStringFlags
 */
static DWORD ParseAclStringFlags(LPCWSTR* StringAcl)
{
    DWORD flags = 0;
    LPCWSTR szAcl = *StringAcl;

    while (*szAcl != '(')
    {
        if (*szAcl == 'P')
        {
            flags |= SE_DACL_PROTECTED;
        }
        else if (*szAcl == 'A')
        {
            szAcl++;
            if (*szAcl == 'R')
                flags |= SE_DACL_AUTO_INHERIT_REQ;
            else if (*szAcl == 'I')
                flags |= SE_DACL_AUTO_INHERITED;
        }
        szAcl++;
    }

    *StringAcl = szAcl;
    return flags;
}

/******************************************************************************
 * ParseAceStringType
 */
static const ACEFLAG AceType[] =
{
    { SDDL_ALARM,          SYSTEM_ALARM_ACE_TYPE },
    { SDDL_AUDIT,          SYSTEM_AUDIT_ACE_TYPE },
    { SDDL_ACCESS_ALLOWED, ACCESS_ALLOWED_ACE_TYPE },
    { SDDL_ACCESS_DENIED,  ACCESS_DENIED_ACE_TYPE },
    /*
    { SDDL_OBJECT_ACCESS_ALLOWED, ACCESS_ALLOWED_OBJECT_ACE_TYPE },
    { SDDL_OBJECT_ACCESS_DENIED,  ACCESS_DENIED_OBJECT_ACE_TYPE },
    { SDDL_OBJECT_ALARM,          SYSTEM_ALARM_OBJECT_ACE_TYPE },
    { SDDL_OBJECT_AUDIT,          SYSTEM_AUDIT_OBJECT_ACE_TYPE },
    */
    { NULL, 0 },
};

static BYTE ParseAceStringType(LPCWSTR* StringAcl)
{
    UINT len = 0;
    LPCWSTR szAcl = *StringAcl;
    const ACEFLAG *lpaf = AceType;

    while (lpaf->wstr &&
        (len = strlenW(lpaf->wstr)) &&
        strncmpW(lpaf->wstr, szAcl, len))
        lpaf++;

    if (!lpaf->wstr)
        return 0;

    *StringAcl += len;
    return lpaf->value;
}


/******************************************************************************
 * ParseAceStringFlags
 */
static const ACEFLAG AceFlags[] =
{
    { SDDL_CONTAINER_INHERIT, CONTAINER_INHERIT_ACE },
    { SDDL_AUDIT_FAILURE,     FAILED_ACCESS_ACE_FLAG },
    { SDDL_INHERITED,         INHERITED_ACE },
    { SDDL_INHERIT_ONLY,      INHERIT_ONLY_ACE },
    { SDDL_NO_PROPAGATE,      NO_PROPAGATE_INHERIT_ACE },
    { SDDL_OBJECT_INHERIT,    OBJECT_INHERIT_ACE },
    { SDDL_AUDIT_SUCCESS,     SUCCESSFUL_ACCESS_ACE_FLAG },
    { NULL, 0 },
};

static BYTE ParseAceStringFlags(LPCWSTR* StringAcl)
{
    UINT len = 0;
    BYTE flags = 0;
    LPCWSTR szAcl = *StringAcl;

    while (*szAcl != ';')
    {
        const ACEFLAG *lpaf = AceFlags;

        while (lpaf->wstr &&
               (len = strlenW(lpaf->wstr)) &&
               strncmpW(lpaf->wstr, szAcl, len))
            lpaf++;

        if (!lpaf->wstr)
            return 0;

        flags |= lpaf->value;
        szAcl += len;
    }

    *StringAcl = szAcl;
    return flags;
}


/******************************************************************************
 * ParseAceStringRights
 */
static DWORD ParseAceStringRights(LPCWSTR* StringAcl)
{
    UINT len = 0;
    DWORD rights = 0;
    LPCWSTR szAcl = *StringAcl;

    if ((*szAcl == '0') && (*(szAcl + 1) == 'x'))
    {
        LPCWSTR p = szAcl;

        while (*p && *p != ';')
            p++;

        if (p - szAcl <= 10 /* 8 hex digits + "0x" */ )
        {
            rights = strtoulW(szAcl, NULL, 16);
            szAcl = p;
        }
        else
            WARN("Invalid rights string format: %s\n", debugstr_wn(szAcl, p - szAcl));
    }
    else
    {
        while (*szAcl != ';')
        {
            const ACEFLAG *lpaf = AceRights;

            while (lpaf->wstr &&
                   (len = strlenW(lpaf->wstr)) &&
                   strncmpW(lpaf->wstr, szAcl, len))
            {
                lpaf++;
            }

            if (!lpaf->wstr)
                return 0;

            rights |= lpaf->value;
            szAcl += len;
        }
    }

    *StringAcl = szAcl;
    return rights;
}


/******************************************************************************
 * ParseStringAclToAcl
 * 
 * dacl_flags(string_ace1)(string_ace2)... (string_acen) 
 */
static BOOL
ParseStringAclToAcl(LPCWSTR StringAcl,
                    LPDWORD lpdwFlags, 
                    PACL pAcl,
                    LPDWORD cBytes)
{
    DWORD val;
    DWORD sidlen;
    DWORD length = sizeof(ACL);
    DWORD acesize = 0;
    DWORD acecount = 0;
    PACCESS_ALLOWED_ACE pAce = NULL; /* pointer to current ACE */

    TRACE("%s\n", debugstr_w(StringAcl));

    if (!StringAcl)
        return FALSE;

    if (pAcl) /* pAce is only useful if we're setting values */
        pAce = (PACCESS_ALLOWED_ACE) (pAcl + 1);

    /* Parse ACL flags */
    *lpdwFlags = ParseAclStringFlags(&StringAcl);

    /* Parse ACE */
    while (*StringAcl == '(')
    {
        StringAcl++;

        /* Parse ACE type */
        val = ParseAceStringType(&StringAcl);
        if (pAce)
            pAce->Header.AceType = (BYTE) val;
        if (*StringAcl != ';')
            goto lerr;
        StringAcl++;

        /* Parse ACE flags */
        val = ParseAceStringFlags(&StringAcl);
        if (pAce)
            pAce->Header.AceFlags = (BYTE) val;
        if (*StringAcl != ';')
            goto lerr;
        StringAcl++;

        /* Parse ACE rights */
        val = ParseAceStringRights(&StringAcl);
        if (pAce)
            pAce->Mask = val;
        if (*StringAcl != ';')
            goto lerr;
        StringAcl++;

        /* Parse ACE object guid */
        if (*StringAcl != ';')
        {
            FIXME("Support for *_OBJECT_ACE_TYPE not implemented\n");
            goto lerr;
        }
        StringAcl++;

        /* Parse ACE inherit object guid */
        if (*StringAcl != ';')
        {
            FIXME("Support for *_OBJECT_ACE_TYPE not implemented\n");
            goto lerr;
        }
        StringAcl++;

        /* Parse ACE account sid */
        if (ParseStringSidToSid(StringAcl, pAce ? &pAce->SidStart : NULL, &sidlen))
        {
            while (*StringAcl && *StringAcl != ')')
                StringAcl++;
        }

        if (*StringAcl != ')')
            goto lerr;
        StringAcl++;

        acesize = sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) + sidlen;
        length += acesize;
        if (pAce)
        {
            pAce->Header.AceSize = acesize;
            pAce = (PACCESS_ALLOWED_ACE)((LPBYTE)pAce + acesize);
        }
        acecount++;
    }

    *cBytes = length;

    if (length > 0xffff)
    {
        ERR("ACL too large\n");
        goto lerr;
    }

    if (pAcl)
    {
        pAcl->AclRevision = ACL_REVISION;
        pAcl->Sbz1 = 0;
        pAcl->AclSize = length;
        pAcl->AceCount = acecount++;
        pAcl->Sbz2 = 0;
    }
    return TRUE;

lerr:
    SetLastError(ERROR_INVALID_ACL);
    WARN("Invalid ACE string format\n");
    return FALSE;
}


/******************************************************************************
 * ParseStringSecurityDescriptorToSecurityDescriptor
 */
static BOOL
ParseStringSecurityDescriptorToSecurityDescriptor(LPCWSTR StringSecurityDescriptor,
                                                  SECURITY_DESCRIPTOR_RELATIVE* SecurityDescriptor,
                                                  LPDWORD cBytes)
{
    BOOL bret = FALSE;
    WCHAR toktype;
    WCHAR tok[MAX_PATH];
    LPCWSTR lptoken;
    LPBYTE lpNext = NULL;
    DWORD len;

    *cBytes = sizeof(SECURITY_DESCRIPTOR);

    if (SecurityDescriptor)
        lpNext = (LPBYTE)(SecurityDescriptor + 1);

    while (*StringSecurityDescriptor)
    {
        toktype = *StringSecurityDescriptor;

        /* Expect char identifier followed by ':' */
        StringSecurityDescriptor++;
        if (*StringSecurityDescriptor != ':')
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            goto lend;
        }
        StringSecurityDescriptor++;

        /* Extract token */
        lptoken = StringSecurityDescriptor;
        while (*lptoken && *lptoken != ':')
            lptoken++;

        if (*lptoken)
            lptoken--;

        len = lptoken - StringSecurityDescriptor;
        memcpy( tok, StringSecurityDescriptor, len * sizeof(WCHAR) );
        tok[len] = 0;

        switch (toktype)
        {
            case 'O':
            {
                DWORD bytes;

                if (!ParseStringSidToSid(tok, lpNext, &bytes))
                    goto lend;

                if (SecurityDescriptor)
                {
                    SecurityDescriptor->Owner = lpNext - (LPBYTE)SecurityDescriptor;
                    lpNext += bytes; /* Advance to next token */
                }

                *cBytes += bytes;

                break;
            }

            case 'G':
            {
                DWORD bytes;

                if (!ParseStringSidToSid(tok, lpNext, &bytes))
                    goto lend;

                if (SecurityDescriptor)
                {
                    SecurityDescriptor->Group = lpNext - (LPBYTE)SecurityDescriptor;
                    lpNext += bytes; /* Advance to next token */
                }

                *cBytes += bytes;

                break;
            }

            case 'D':
            {
                DWORD flags;
                DWORD bytes;

                if (!ParseStringAclToAcl(tok, &flags, (PACL)lpNext, &bytes))
                    goto lend;

                if (SecurityDescriptor)
                {
                    SecurityDescriptor->Control |= SE_DACL_PRESENT | flags;
                    SecurityDescriptor->Dacl = lpNext - (LPBYTE)SecurityDescriptor;
                    lpNext += bytes; /* Advance to next token */
                }

                *cBytes += bytes;

                break;
            }

            case 'S':
            {
                DWORD flags;
                DWORD bytes;

                if (!ParseStringAclToAcl(tok, &flags, (PACL)lpNext, &bytes))
                    goto lend;

                if (SecurityDescriptor)
                {
                    SecurityDescriptor->Control |= SE_SACL_PRESENT | flags;
                    SecurityDescriptor->Sacl = lpNext - (LPBYTE)SecurityDescriptor;
                    lpNext += bytes; /* Advance to next token */
                }

                *cBytes += bytes;

                break;
            }

            default:
                FIXME("Unknown token\n");
                SetLastError(ERROR_INVALID_PARAMETER);
                goto lend;
        }

        StringSecurityDescriptor = lptoken;
    }

    bret = TRUE;

lend:
    return bret;
}

/* Winehq cvs 20050916 */
/******************************************************************************
 * ConvertStringSecurityDescriptorToSecurityDescriptorA [ADVAPI32.@]
 * @implemented
 */
BOOL
WINAPI
ConvertStringSecurityDescriptorToSecurityDescriptorA(LPCSTR StringSecurityDescriptor,
                                                     DWORD StringSDRevision,
                                                     PSECURITY_DESCRIPTOR* SecurityDescriptor,
                                                     PULONG SecurityDescriptorSize)
{
    UINT len;
    BOOL ret = FALSE;
    LPWSTR StringSecurityDescriptorW;

    len = MultiByteToWideChar(CP_ACP, 0, StringSecurityDescriptor, -1, NULL, 0);
    StringSecurityDescriptorW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));

    if (StringSecurityDescriptorW)
    {
        MultiByteToWideChar(CP_ACP, 0, StringSecurityDescriptor, -1, StringSecurityDescriptorW, len);

        ret = ConvertStringSecurityDescriptorToSecurityDescriptorW(StringSecurityDescriptorW,
                                                                   StringSDRevision, SecurityDescriptor,
                                                                   SecurityDescriptorSize);
        HeapFree(GetProcessHeap(), 0, StringSecurityDescriptorW);
    }

    return ret;
}

/******************************************************************************
 * ConvertStringSecurityDescriptorToSecurityDescriptorW [ADVAPI32.@]
 * @implemented
 */
BOOL WINAPI
ConvertStringSecurityDescriptorToSecurityDescriptorW(LPCWSTR StringSecurityDescriptor,
                                                     DWORD StringSDRevision,
                                                     PSECURITY_DESCRIPTOR* SecurityDescriptor,
                                                     PULONG SecurityDescriptorSize)
{
    DWORD cBytes;
    SECURITY_DESCRIPTOR* psd;
    BOOL bret = FALSE;

    TRACE("%s\n", debugstr_w(StringSecurityDescriptor));

    if (GetVersion() & 0x80000000)
    {
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        goto lend;
    }
    else if (!StringSecurityDescriptor || !SecurityDescriptor)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto lend;
    }
    else if (StringSDRevision != SID_REVISION)
    {
        SetLastError(ERROR_UNKNOWN_REVISION);
        goto lend;
    }

    /* Compute security descriptor length */
    if (!ParseStringSecurityDescriptorToSecurityDescriptor(StringSecurityDescriptor,
        NULL, &cBytes))
        goto lend;

    psd = *SecurityDescriptor = LocalAlloc(GMEM_ZEROINIT, cBytes);
    if (!psd) goto lend;

    psd->Revision = SID_REVISION;
    psd->Control |= SE_SELF_RELATIVE;

    if (!ParseStringSecurityDescriptorToSecurityDescriptor(StringSecurityDescriptor,
             (SECURITY_DESCRIPTOR_RELATIVE *)psd, &cBytes))
    {
        LocalFree(psd);
        goto lend;
    }

    if (SecurityDescriptorSize)
        *SecurityDescriptorSize = cBytes;

    bret = TRUE;

lend:
    TRACE(" ret=%d\n", bret);
    return bret;
}

static void DumpString(LPCWSTR string, int cch, WCHAR **pwptr, ULONG *plen)
{
    if (cch == -1)
        cch = strlenW(string);

    if (plen)
        *plen += cch;

    if (pwptr)
    {
        memcpy(*pwptr, string, sizeof(WCHAR)*cch);
        *pwptr += cch;
    }
}

static BOOL DumpSidNumeric(PSID psid, WCHAR **pwptr, ULONG *plen)
{
    DWORD i;
    WCHAR fmt[] = { 'S','-','%','u','-','%','d',0 };
    WCHAR subauthfmt[] = { '-','%','u',0 };
    WCHAR buf[26];
    SID *pisid = psid;

    if( !IsValidSid( psid ) || pisid->Revision != SDDL_REVISION)
    {
        SetLastError(ERROR_INVALID_SID);
        return FALSE;
    }

    if (pisid->IdentifierAuthority.Value[0] ||
     pisid->IdentifierAuthority.Value[1])
    {
        FIXME("not matching MS' bugs\n");
        SetLastError(ERROR_INVALID_SID);
        return FALSE;
    }

    sprintfW( buf, fmt, pisid->Revision,
        MAKELONG(
            MAKEWORD( pisid->IdentifierAuthority.Value[5],
                    pisid->IdentifierAuthority.Value[4] ),
            MAKEWORD( pisid->IdentifierAuthority.Value[3],
                    pisid->IdentifierAuthority.Value[2] )
        ) );
    DumpString(buf, -1, pwptr, plen);

    for( i=0; i<pisid->SubAuthorityCount; i++ )
    {
        sprintfW( buf, subauthfmt, pisid->SubAuthority[i] );
        DumpString(buf, -1, pwptr, plen);
    }
    return TRUE;
}

static BOOL DumpSid(PSID psid, WCHAR **pwptr, ULONG *plen)
{
    size_t i;
    for (i = 0; i < sizeof(WellKnownSids) / sizeof(WellKnownSids[0]); i++)
    {
        if (WellKnownSids[i].wstr[0] && EqualSid(psid, (PSID)&(WellKnownSids[i].Sid.Revision)))
        {
            DumpString(WellKnownSids[i].wstr, 2, pwptr, plen);
            return TRUE;
        }
    }

    return DumpSidNumeric(psid, pwptr, plen);
}

static const LPCWSTR AceRightBitNames[32] = {
        SDDL_CREATE_CHILD,        /*  0 */
        SDDL_DELETE_CHILD,
        SDDL_LIST_CHILDREN,
        SDDL_SELF_WRITE,
        SDDL_READ_PROPERTY,       /*  4 */
        SDDL_WRITE_PROPERTY,
        SDDL_DELETE_TREE,
        SDDL_LIST_OBJECT,
        SDDL_CONTROL_ACCESS,      /*  8 */
        NULL,
        NULL,
        NULL,
        NULL,                     /* 12 */
        NULL,
        NULL,
        NULL,
        SDDL_STANDARD_DELETE,     /* 16 */
        SDDL_READ_CONTROL,
        SDDL_WRITE_DAC,
        SDDL_WRITE_OWNER,
        NULL,                     /* 20 */
        NULL,
        NULL,
        NULL,
        NULL,                     /* 24 */
        NULL,
        NULL,
        NULL,
        SDDL_GENERIC_ALL,         /* 28 */
        SDDL_GENERIC_EXECUTE,
        SDDL_GENERIC_WRITE,
        SDDL_GENERIC_READ
};

static void DumpRights(DWORD mask, WCHAR **pwptr, ULONG *plen)
{
    static const WCHAR fmtW[] = {'0','x','%','x',0};
    WCHAR buf[15];
    size_t i;

    if (mask == 0)
        return;

    /* first check if the right have name */
    for (i = 0; i < sizeof(AceRights)/sizeof(AceRights[0]); i++)
    {
        if (AceRights[i].wstr == NULL)
            break;
        if (mask == AceRights[i].value)
        {
            DumpString(AceRights[i].wstr, -1, pwptr, plen);
            return;
        }
    }

    /* then check if it can be built from bit names */
    for (i = 0; i < 32; i++)
    {
        if ((mask & (1 << i)) && (AceRightBitNames[i] == NULL))
        {
            /* can't be built from bit names */
            sprintfW(buf, fmtW, mask);
            DumpString(buf, -1, pwptr, plen);
            return;
        }
    }

    /* build from bit names */
    for (i = 0; i < 32; i++)
        if (mask & (1 << i))
            DumpString(AceRightBitNames[i], -1, pwptr, plen);
}

static BOOL DumpAce(LPVOID pace, WCHAR **pwptr, ULONG *plen)
{
    ACCESS_ALLOWED_ACE *piace; /* all the supported ACEs have the same memory layout */
    static const WCHAR openbr = '(';
    static const WCHAR closebr = ')';
    static const WCHAR semicolon = ';';

    if (((PACE_HEADER)pace)->AceType > SYSTEM_ALARM_ACE_TYPE || ((PACE_HEADER)pace)->AceSize < sizeof(ACCESS_ALLOWED_ACE))
    {
        SetLastError(ERROR_INVALID_ACL);
        return FALSE;
    }

    piace = pace;
    DumpString(&openbr, 1, pwptr, plen);
    switch (piace->Header.AceType)
    {
        case ACCESS_ALLOWED_ACE_TYPE:
            DumpString(SDDL_ACCESS_ALLOWED, -1, pwptr, plen);
            break;
        case ACCESS_DENIED_ACE_TYPE:
            DumpString(SDDL_ACCESS_DENIED, -1, pwptr, plen);
            break;
        case SYSTEM_AUDIT_ACE_TYPE:
            DumpString(SDDL_AUDIT, -1, pwptr, plen);
            break;
        case SYSTEM_ALARM_ACE_TYPE:
            DumpString(SDDL_ALARM, -1, pwptr, plen);
            break;
    }
    DumpString(&semicolon, 1, pwptr, plen);

    if (piace->Header.AceFlags & OBJECT_INHERIT_ACE)
        DumpString(SDDL_OBJECT_INHERIT, -1, pwptr, plen);
    if (piace->Header.AceFlags & CONTAINER_INHERIT_ACE)
        DumpString(SDDL_CONTAINER_INHERIT, -1, pwptr, plen);
    if (piace->Header.AceFlags & NO_PROPAGATE_INHERIT_ACE)
        DumpString(SDDL_NO_PROPAGATE, -1, pwptr, plen);
    if (piace->Header.AceFlags & INHERIT_ONLY_ACE)
        DumpString(SDDL_INHERIT_ONLY, -1, pwptr, plen);
    if (piace->Header.AceFlags & INHERITED_ACE)
        DumpString(SDDL_INHERITED, -1, pwptr, plen);
    if (piace->Header.AceFlags & SUCCESSFUL_ACCESS_ACE_FLAG)
        DumpString(SDDL_AUDIT_SUCCESS, -1, pwptr, plen);
    if (piace->Header.AceFlags & FAILED_ACCESS_ACE_FLAG)
        DumpString(SDDL_AUDIT_FAILURE, -1, pwptr, plen);
    DumpString(&semicolon, 1, pwptr, plen);
    DumpRights(piace->Mask, pwptr, plen);
    DumpString(&semicolon, 1, pwptr, plen);
    /* objects not supported */
    DumpString(&semicolon, 1, pwptr, plen);
    /* objects not supported */
    DumpString(&semicolon, 1, pwptr, plen);
    if (!DumpSid((PSID)&piace->SidStart, pwptr, plen))
        return FALSE;
    DumpString(&closebr, 1, pwptr, plen);
    return TRUE;
}

static BOOL DumpAcl(PACL pacl, WCHAR **pwptr, ULONG *plen, BOOL protected, BOOL autoInheritReq, BOOL autoInherited)
{
    WORD count;
    int i;

    if (protected)
        DumpString(SDDL_PROTECTED, -1, pwptr, plen);
    if (autoInheritReq)
        DumpString(SDDL_AUTO_INHERIT_REQ, -1, pwptr, plen);
    if (autoInherited)
        DumpString(SDDL_AUTO_INHERITED, -1, pwptr, plen);

    if (pacl == NULL)
        return TRUE;

    if (!IsValidAcl(pacl))
        return FALSE;

    count = pacl->AceCount;
    for (i = 0; i < count; i++)
    {
        LPVOID ace;
        if (!GetAce(pacl, i, &ace))
            return FALSE;
        if (!DumpAce(ace, pwptr, plen))
            return FALSE;
    }

    return TRUE;
}

static BOOL DumpOwner(PSECURITY_DESCRIPTOR SecurityDescriptor, WCHAR **pwptr, ULONG *plen)
{
    static const WCHAR prefix[] = {'O',':',0};
    BOOL bDefaulted;
    PSID psid;

    if (!GetSecurityDescriptorOwner(SecurityDescriptor, &psid, &bDefaulted))
        return FALSE;

    if (psid == NULL)
        return TRUE;

    DumpString(prefix, -1, pwptr, plen);
    if (!DumpSid(psid, pwptr, plen))
        return FALSE;
    return TRUE;
}

static BOOL DumpGroup(PSECURITY_DESCRIPTOR SecurityDescriptor, WCHAR **pwptr, ULONG *plen)
{
    static const WCHAR prefix[] = {'G',':',0};
    BOOL bDefaulted;
    PSID psid;

    if (!GetSecurityDescriptorGroup(SecurityDescriptor, &psid, &bDefaulted))
        return FALSE;

    if (psid == NULL)
        return TRUE;

    DumpString(prefix, -1, pwptr, plen);
    if (!DumpSid(psid, pwptr, plen))
        return FALSE;
    return TRUE;
}

static BOOL DumpDacl(PSECURITY_DESCRIPTOR SecurityDescriptor, WCHAR **pwptr, ULONG *plen)
{
    static const WCHAR dacl[] = {'D',':',0};
    SECURITY_DESCRIPTOR_CONTROL control;
    BOOL present, defaulted;
    DWORD revision;
    PACL pacl;

    if (!GetSecurityDescriptorDacl(SecurityDescriptor, &present, &pacl, &defaulted))
        return FALSE;

    if (!GetSecurityDescriptorControl(SecurityDescriptor, &control, &revision))
        return FALSE;

    if (!present)
        return TRUE;

    DumpString(dacl, 2, pwptr, plen);
    if (!DumpAcl(pacl, pwptr, plen, control & SE_DACL_PROTECTED, control & SE_DACL_AUTO_INHERIT_REQ, control & SE_DACL_AUTO_INHERITED))
        return FALSE;
    return TRUE;
}

static BOOL DumpSacl(PSECURITY_DESCRIPTOR SecurityDescriptor, WCHAR **pwptr, ULONG *plen)
{
    static const WCHAR sacl[] = {'S',':',0};
    SECURITY_DESCRIPTOR_CONTROL control;
    BOOL present, defaulted;
    DWORD revision;
    PACL pacl;

    if (!GetSecurityDescriptorSacl(SecurityDescriptor, &present, &pacl, &defaulted))
        return FALSE;

    if (!GetSecurityDescriptorControl(SecurityDescriptor, &control, &revision))
        return FALSE;

    if (!present)
        return TRUE;

    DumpString(sacl, 2, pwptr, plen);
    if (!DumpAcl(pacl, pwptr, plen, control & SE_SACL_PROTECTED, control & SE_SACL_AUTO_INHERIT_REQ, control & SE_SACL_AUTO_INHERITED))
        return FALSE;
    return TRUE;
}

/******************************************************************************
 * ConvertSecurityDescriptorToStringSecurityDescriptorW [ADVAPI32.@]
 * @implemented
 */
BOOL WINAPI
ConvertSecurityDescriptorToStringSecurityDescriptorW(PSECURITY_DESCRIPTOR SecurityDescriptor,
                                                     DWORD SDRevision,
                                                     SECURITY_INFORMATION SecurityInformation,
                                                     LPWSTR *OutputString,
                                                     PULONG OutputLen)
{
    ULONG len;
    WCHAR *wptr, *wstr;

    if (SDRevision != SDDL_REVISION_1)
    {
        ERR("Program requested unknown SDDL revision %d\n", SDRevision);
        SetLastError(ERROR_UNKNOWN_REVISION);
        return FALSE;
    }

    len = 0;
    if (SecurityInformation & OWNER_SECURITY_INFORMATION)
        if (!DumpOwner(SecurityDescriptor, NULL, &len))
            return FALSE;
    if (SecurityInformation & GROUP_SECURITY_INFORMATION)
        if (!DumpGroup(SecurityDescriptor, NULL, &len))
            return FALSE;
    if (SecurityInformation & DACL_SECURITY_INFORMATION)
        if (!DumpDacl(SecurityDescriptor, NULL, &len))
            return FALSE;
    if (SecurityInformation & SACL_SECURITY_INFORMATION)
        if (!DumpSacl(SecurityDescriptor, NULL, &len))
            return FALSE;

    wstr = wptr = LocalAlloc(0, (len + 1)*sizeof(WCHAR));
    if (wstr == NULL)
        return FALSE;
        
    if (SecurityInformation & OWNER_SECURITY_INFORMATION)
        if (!DumpOwner(SecurityDescriptor, &wptr, NULL))
            return FALSE;
    if (SecurityInformation & GROUP_SECURITY_INFORMATION)
        if (!DumpGroup(SecurityDescriptor, &wptr, NULL))
            return FALSE;
    if (SecurityInformation & DACL_SECURITY_INFORMATION)
        if (!DumpDacl(SecurityDescriptor, &wptr, NULL))
            return FALSE;
    if (SecurityInformation & SACL_SECURITY_INFORMATION)
        if (!DumpSacl(SecurityDescriptor, &wptr, NULL))
            return FALSE;
    *wptr = 0;

    TRACE("ret: %s, %d\n", wine_dbgstr_w(wstr), len);
    *OutputString = wstr;
    if (OutputLen)
        *OutputLen = strlenW(*OutputString)+1;
    return TRUE;
}

/******************************************************************************
 * ConvertSecurityDescriptorToStringSecurityDescriptorA [ADVAPI32.@]
 * @implemented
 */
BOOL WINAPI
ConvertSecurityDescriptorToStringSecurityDescriptorA(PSECURITY_DESCRIPTOR SecurityDescriptor,
                                                     DWORD SDRevision,
                                                     SECURITY_INFORMATION Information,
                                                     LPSTR *OutputString,
                                                     PULONG OutputLen)
{
    LPWSTR wstr;
    ULONG len;

    if (ConvertSecurityDescriptorToStringSecurityDescriptorW(SecurityDescriptor, SDRevision, Information, &wstr, &len))
    {
        int lenA;

        lenA = WideCharToMultiByte(CP_ACP, 0, wstr, len, NULL, 0, NULL, NULL);
        *OutputString = HeapAlloc(GetProcessHeap(), 0, lenA);
        if (*OutputString == NULL)
        {
            LocalFree(wstr);
            *OutputLen = 0;
            return FALSE;
        }
        WideCharToMultiByte(CP_ACP, 0, wstr, len, *OutputString, lenA, NULL, NULL);
        LocalFree(wstr);

        if (OutputLen != NULL)
            *OutputLen = lenA;
        return TRUE;
    }
    else
    {
        *OutputString = NULL;
        if (OutputLen)
            *OutputLen = 0;
        return FALSE;
    }
}

/*
 * @implemented
 */
BOOL
WINAPI
ConvertStringSidToSidW(IN LPCWSTR StringSid,
                       OUT PSID* sid)
{
    DWORD size;
    DWORD i, cBytes, identAuth, csubauth;
    BOOL ret;
    SID* pisid;

    TRACE("%s %p\n", debugstr_w(StringSid), sid);

    if (!StringSid)
    {
        SetLastError(ERROR_INVALID_SID);
        return FALSE;
    }

    for (i = 0; i < sizeof(SidTable) / sizeof(SidTable[0]) - 1; i++)
    {
        if (wcscmp(StringSid, SidTable[i].key) == 0)
        {
            WELL_KNOWN_SID_TYPE knownSid = (WELL_KNOWN_SID_TYPE)SidTable[i].value;
            size = SECURITY_MAX_SID_SIZE;
            *sid = LocalAlloc(0, size);
            if (!*sid)
            {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                return FALSE;
            }
            ret = CreateWellKnownSid(knownSid,
                                     NULL,
                                     *sid,
                                     &size);
            if (!ret)
            {
                SetLastError(ERROR_INVALID_SID);
                LocalFree(*sid);
            }
            return ret;
        }
    }

    /* That's probably a string S-R-I-S-S... */
    if (StringSid[0] != 'S' || StringSid[1] != '-')
    {
        SetLastError(ERROR_INVALID_SID);
        return FALSE;
    }

    cBytes = ComputeStringSidSize(StringSid);
    pisid = (SID*)LocalAlloc( 0, cBytes );
    if (!pisid)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    i = 0;
    ret = FALSE;
    csubauth = ((cBytes - GetSidLengthRequired(0)) / sizeof(DWORD));

    StringSid += 2; /* Advance to Revision */
    pisid->Revision = atoiW(StringSid);

    if (pisid->Revision != SDDL_REVISION)
    {
        TRACE("Revision %d is unknown\n", pisid->Revision);
        goto lend; /* ERROR_INVALID_SID */
    }
    if (csubauth == 0)
    {
        TRACE("SubAuthorityCount is 0\n");
        goto lend; /* ERROR_INVALID_SID */
    }

    pisid->SubAuthorityCount = csubauth;

    /* Advance to identifier authority */
    while (*StringSid && *StringSid != '-')
        StringSid++;
    if (*StringSid == '-')
        StringSid++;

    /* MS' implementation can't handle values greater than 2^32 - 1, so
     * we don't either; assume most significant bytes are always 0
     */
    pisid->IdentifierAuthority.Value[0] = 0;
    pisid->IdentifierAuthority.Value[1] = 0;
    identAuth = atoiW(StringSid);
    pisid->IdentifierAuthority.Value[5] = identAuth & 0xff;
    pisid->IdentifierAuthority.Value[4] = (identAuth & 0xff00) >> 8;
    pisid->IdentifierAuthority.Value[3] = (identAuth & 0xff0000) >> 16;
    pisid->IdentifierAuthority.Value[2] = (identAuth & 0xff000000) >> 24;

    /* Advance to first sub authority */
    while (*StringSid && *StringSid != '-')
        StringSid++;
    if (*StringSid == '-')
        StringSid++;

    while (*StringSid)
    {
        pisid->SubAuthority[i++] = atoiW(StringSid);

        while (*StringSid && *StringSid != '-')
            StringSid++;
        if (*StringSid == '-')
            StringSid++;
    }

    if (i != pisid->SubAuthorityCount)
        goto lend; /* ERROR_INVALID_SID */

    *sid = pisid;
    ret = TRUE;

lend:
    if (!ret)
    {
        LocalFree(pisid);
        SetLastError(ERROR_INVALID_SID);
    }

    TRACE("returning %s\n", ret ? "TRUE" : "FALSE");
    return ret;
}

/*
 * @implemented
 */
BOOL
WINAPI
ConvertStringSidToSidA(IN LPCSTR StringSid,
                       OUT PSID* sid)
{
    BOOL bRetVal = FALSE;

    TRACE("%s, %p\n", debugstr_a(StringSid), sid);
    if (GetVersion() & 0x80000000)
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    else if (!StringSid || !sid)
        SetLastError(ERROR_INVALID_PARAMETER);
    else
    {
        UINT len = MultiByteToWideChar(CP_ACP, 0, StringSid, -1, NULL, 0);
        LPWSTR wStringSid = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (wStringSid == NULL)
            return FALSE;
        MultiByteToWideChar(CP_ACP, 0, StringSid, - 1, wStringSid, len);
        bRetVal = ConvertStringSidToSidW(wStringSid, sid);
        HeapFree(GetProcessHeap(), 0, wStringSid);
    }
    return bRetVal;
}

/*
 * @implemented
 */
BOOL
WINAPI
ConvertSidToStringSidW(PSID Sid,
                       LPWSTR *StringSid)
{
    NTSTATUS Status;
    UNICODE_STRING UnicodeString;
    WCHAR FixedBuffer[64];

    if (!RtlValidSid(Sid))
    {
        SetLastError(ERROR_INVALID_SID);
        return FALSE;
    }

    UnicodeString.Length = 0;
    UnicodeString.MaximumLength = sizeof(FixedBuffer);
    UnicodeString.Buffer = FixedBuffer;
    Status = RtlConvertSidToUnicodeString(&UnicodeString, Sid, FALSE);
    if (STATUS_BUFFER_TOO_SMALL == Status)
    {
        Status = RtlConvertSidToUnicodeString(&UnicodeString, Sid, TRUE);
    }

    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    *StringSid = LocalAlloc(LMEM_FIXED, UnicodeString.Length + sizeof(WCHAR));
    if (NULL == *StringSid)
    {
        if (UnicodeString.Buffer != FixedBuffer)
        {
            RtlFreeUnicodeString(&UnicodeString);
        }
      SetLastError(ERROR_NOT_ENOUGH_MEMORY);
      return FALSE;
    }

    MoveMemory(*StringSid, UnicodeString.Buffer, UnicodeString.Length);
    ZeroMemory((PCHAR) *StringSid + UnicodeString.Length, sizeof(WCHAR));
    if (UnicodeString.Buffer != FixedBuffer)
    {
        RtlFreeUnicodeString(&UnicodeString);
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
ConvertSidToStringSidA(PSID Sid,
                       LPSTR *StringSid)
{
    LPWSTR StringSidW;
    int Len;

    if (!ConvertSidToStringSidW(Sid, &StringSidW))
    {
        return FALSE;
    }

    Len = WideCharToMultiByte(CP_ACP, 0, StringSidW, -1, NULL, 0, NULL, NULL);
    if (Len <= 0)
    {
        LocalFree(StringSidW);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    *StringSid = LocalAlloc(LMEM_FIXED, Len);
    if (NULL == *StringSid)
    {
        LocalFree(StringSidW);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    if (!WideCharToMultiByte(CP_ACP, 0, StringSidW, -1, *StringSid, Len, NULL, NULL))
    {
        LocalFree(StringSid);
        LocalFree(StringSidW);
        return FALSE;
    }

    LocalFree(StringSidW);

    return TRUE;
}

/*
 * @unimplemented
 */
BOOL WINAPI
CreateProcessWithLogonW(LPCWSTR lpUsername,
                        LPCWSTR lpDomain,
                        LPCWSTR lpPassword,
                        DWORD dwLogonFlags,
                        LPCWSTR lpApplicationName,
                        LPWSTR lpCommandLine,
                        DWORD dwCreationFlags,
                        LPVOID lpEnvironment,
                        LPCWSTR lpCurrentDirectory,
                        LPSTARTUPINFOW lpStartupInfo,
                        LPPROCESS_INFORMATION lpProcessInformation)
{
    FIXME("%s %s %s 0x%08x %s %s 0x%08x %p %s %p %p stub\n", debugstr_w(lpUsername), debugstr_w(lpDomain),
    debugstr_w(lpPassword), dwLogonFlags, debugstr_w(lpApplicationName),
    debugstr_w(lpCommandLine), dwCreationFlags, lpEnvironment, debugstr_w(lpCurrentDirectory),
    lpStartupInfo, lpProcessInformation);

    return FALSE;
}

BOOL
WINAPI
CreateProcessWithTokenW(IN HANDLE hToken,
                        IN DWORD dwLogonFlags,
                        IN LPCWSTR lpApplicationName OPTIONAL,
                        IN OUT LPWSTR lpCommandLine OPTIONAL,
                        IN DWORD dwCreationFlags,
                        IN LPVOID lpEnvironment OPTIONAL,
                        IN LPCWSTR lpCurrentDirectory OPTIONAL,
                        IN LPSTARTUPINFOW lpStartupInfo,
                        OUT LPPROCESS_INFORMATION lpProcessInfo)
{
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @implemented
 */
BOOL WINAPI
DuplicateTokenEx(IN HANDLE ExistingTokenHandle,
                 IN DWORD dwDesiredAccess,
                 IN LPSECURITY_ATTRIBUTES lpTokenAttributes  OPTIONAL,
                 IN SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
                 IN TOKEN_TYPE TokenType,
                 OUT PHANDLE DuplicateTokenHandle)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;
    SECURITY_QUALITY_OF_SERVICE Sqos;

    TRACE("%p 0x%08x 0x%08x 0x%08x %p\n", ExistingTokenHandle, dwDesiredAccess,
        ImpersonationLevel, TokenType, DuplicateTokenHandle);

    Sqos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    Sqos.ImpersonationLevel = ImpersonationLevel;
    Sqos.ContextTrackingMode = 0;
    Sqos.EffectiveOnly = FALSE;

    if (lpTokenAttributes != NULL)
    {
        InitializeObjectAttributes(&ObjectAttributes,
                                   NULL,
                                   lpTokenAttributes->bInheritHandle ? OBJ_INHERIT : 0,
                                   NULL,
                                   lpTokenAttributes->lpSecurityDescriptor);
    }
    else
    {
        InitializeObjectAttributes(&ObjectAttributes,
                                   NULL,
                                   0,
                                   NULL,
                                   NULL);
    }

    ObjectAttributes.SecurityQualityOfService = &Sqos;

    Status = NtDuplicateToken(ExistingTokenHandle,
                              dwDesiredAccess,
                              &ObjectAttributes,
                              FALSE,
                              TokenType,
                              DuplicateTokenHandle);
    if (!NT_SUCCESS(Status))
    {
        ERR("NtDuplicateToken failed: Status %08x\n", Status);
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    TRACE("Returning token %p.\n", *DuplicateTokenHandle);

    return TRUE;
}

/*
 * @implemented
 */
BOOL WINAPI
DuplicateToken(IN HANDLE ExistingTokenHandle,
               IN SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
               OUT PHANDLE DuplicateTokenHandle)
{
    return DuplicateTokenEx(ExistingTokenHandle,
                            TOKEN_IMPERSONATE | TOKEN_QUERY,
                            NULL,
                            ImpersonationLevel,
                            TokenImpersonation,
                            DuplicateTokenHandle);
}

/******************************************************************************
 * ComputeStringSidSize
 */
static DWORD ComputeStringSidSize(LPCWSTR StringSid)
{
    if (StringSid[0] == 'S' && StringSid[1] == '-') /* S-R-I(-S)+ */
    {
        int ctok = 0;
        while (*StringSid)
        {
            if (*StringSid == '-')
                ctok++;
            StringSid++;
        }

        if (ctok >= 3)
            return GetSidLengthRequired(ctok - 2);
    }
    else /* String constant format  - Only available in winxp and above */
    {
        unsigned int i;

        for (i = 0; i < sizeof(WellKnownSids)/sizeof(WellKnownSids[0]); i++)
            if (!strncmpW(WellKnownSids[i].wstr, StringSid, 2))
                return GetSidLengthRequired(WellKnownSids[i].Sid.SubAuthorityCount);
    }

    return GetSidLengthRequired(0);
}

/******************************************************************************
 * ParseStringSidToSid
 */
static BOOL ParseStringSidToSid(LPCWSTR StringSid, PSID pSid, LPDWORD cBytes)
{
    BOOL bret = FALSE;
    SID* pisid=pSid;

    TRACE("%s, %p, %p\n", debugstr_w(StringSid), pSid, cBytes);
    if (!StringSid)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        TRACE("StringSid is NULL, returning FALSE\n");
        return FALSE;
    }

    while (*StringSid == ' ')
        StringSid++;

    *cBytes = ComputeStringSidSize(StringSid);
    if (!pisid) /* Simply compute the size */
    {
        TRACE("only size requested, returning TRUE\n");
        return TRUE;
    }

    if (StringSid[0] == 'S' && StringSid[1] == '-') /* S-R-I-S-S */
    {
        DWORD i = 0, identAuth;
        DWORD csubauth = ((*cBytes - GetSidLengthRequired(0)) / sizeof(DWORD));

        StringSid += 2; /* Advance to Revision */
        pisid->Revision = atoiW(StringSid);

        if (pisid->Revision != SDDL_REVISION)
        {
            TRACE("Revision %d is unknown\n", pisid->Revision);
            goto lend; /* ERROR_INVALID_SID */
        }
        if (csubauth == 0)
        {
            TRACE("SubAuthorityCount is 0\n");
            goto lend; /* ERROR_INVALID_SID */
        }

        pisid->SubAuthorityCount = csubauth;

        /* Advance to identifier authority */
        while (*StringSid && *StringSid != '-')
            StringSid++;
        if (*StringSid == '-')
            StringSid++;

        /* MS' implementation can't handle values greater than 2^32 - 1, so
         * we don't either; assume most significant bytes are always 0
         */
        pisid->IdentifierAuthority.Value[0] = 0;
        pisid->IdentifierAuthority.Value[1] = 0;
        identAuth = atoiW(StringSid);
        pisid->IdentifierAuthority.Value[5] = identAuth & 0xff;
        pisid->IdentifierAuthority.Value[4] = (identAuth & 0xff00) >> 8;
        pisid->IdentifierAuthority.Value[3] = (identAuth & 0xff0000) >> 16;
        pisid->IdentifierAuthority.Value[2] = (identAuth & 0xff000000) >> 24;

        /* Advance to first sub authority */
        while (*StringSid && *StringSid != '-')
            StringSid++;
        if (*StringSid == '-')
            StringSid++;

        while (*StringSid)
        {
            pisid->SubAuthority[i++] = atoiW(StringSid);

            while (*StringSid && *StringSid != '-')
                StringSid++;
            if (*StringSid == '-')
                StringSid++;
        }

        if (i != pisid->SubAuthorityCount)
            goto lend; /* ERROR_INVALID_SID */

        bret = TRUE;
    }
    else /* String constant format  - Only available in winxp and above */
    {
        unsigned int i;
        pisid->Revision = SDDL_REVISION;

        for (i = 0; i < sizeof(WellKnownSids)/sizeof(WellKnownSids[0]); i++)
            if (!strncmpW(WellKnownSids[i].wstr, StringSid, 2))
            {
                DWORD j;
                pisid->SubAuthorityCount = WellKnownSids[i].Sid.SubAuthorityCount;
                pisid->IdentifierAuthority = WellKnownSids[i].Sid.IdentifierAuthority;
                for (j = 0; j < WellKnownSids[i].Sid.SubAuthorityCount; j++)
                    pisid->SubAuthority[j] = WellKnownSids[i].Sid.SubAuthority[j];
                bret = TRUE;
            }

        if (!bret)
            FIXME("String constant not supported: %s\n", debugstr_wn(StringSid, 2));
    }

lend:
    if (!bret)
        SetLastError(ERROR_INVALID_SID);

    TRACE("returning %s\n", bret ? "TRUE" : "FALSE");
    return bret;
}

/**********************************************************************
 * GetNamedSecurityInfoA			EXPORTED
 *
 * @implemented
 */
DWORD
WINAPI
GetNamedSecurityInfoA(LPSTR pObjectName,
                      SE_OBJECT_TYPE ObjectType,
                      SECURITY_INFORMATION SecurityInfo,
                      PSID *ppsidOwner,
                      PSID *ppsidGroup,
                      PACL *ppDacl,
                      PACL *ppSacl,
                      PSECURITY_DESCRIPTOR *ppSecurityDescriptor)
{
    DWORD len;
    LPWSTR wstr = NULL;
    DWORD r;

    TRACE("%s %d %d %p %p %p %p %p\n", pObjectName, ObjectType, SecurityInfo,
        ppsidOwner, ppsidGroup, ppDacl, ppSacl, ppSecurityDescriptor);

    if( pObjectName )
    {
        len = MultiByteToWideChar( CP_ACP, 0, pObjectName, -1, NULL, 0 );
        wstr = HeapAlloc( GetProcessHeap(), 0, len*sizeof(WCHAR));
        MultiByteToWideChar( CP_ACP, 0, pObjectName, -1, wstr, len );
    }

    r = GetNamedSecurityInfoW( wstr, ObjectType, SecurityInfo, ppsidOwner,
                           ppsidGroup, ppDacl, ppSacl, ppSecurityDescriptor );

    HeapFree( GetProcessHeap(), 0, wstr );

    return r;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
GetWindowsAccountDomainSid(IN PSID pSid,
                           OUT PSID ppDomainSid,
                           IN OUT DWORD* cbSid)
{
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
EqualDomainSid(IN PSID pSid1,
               IN PSID pSid2,
               OUT BOOL* pfEqual)
{
    UNIMPLEMENTED;
    return FALSE;
}

/* EOF */
