/*
 * PROJECT:         ReactOS Native Headers
 * FILE:            include/ndk/setypes.h
 * PURPOSE:         Defintions for Security Subsystem Types not defined in DDK/IFS
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * UPDATE HISTORY:
 *                  Created 06/10/04
 */
#ifndef _SETYPES_H
#define _SETYPES_H

/* DEPENDENCIES **************************************************************/

/* EXPORTED DATA *************************************************************/

/* CONSTANTS *****************************************************************/
#ifdef NTOS_MODE_USER
#define SE_MIN_WELL_KNOWN_PRIVILEGE       (2L)
#define SE_CREATE_TOKEN_PRIVILEGE         (2L)
#define SE_ASSIGNPRIMARYTOKEN_PRIVILEGE   (3L)
#define SE_LOCK_MEMORY_PRIVILEGE          (4L)
#define SE_INCREASE_QUOTA_PRIVILEGE       (5L)
#define SE_UNSOLICITED_INPUT_PRIVILEGE    (6L)
#define SE_MACHINE_ACCOUNT_PRIVILEGE      (6L)
#define SE_TCB_PRIVILEGE                  (7L)
#define SE_SECURITY_PRIVILEGE             (8L)
#define SE_TAKE_OWNERSHIP_PRIVILEGE       (9L)
#define SE_LOAD_DRIVER_PRIVILEGE          (10L)
#define SE_SYSTEM_PROFILE_PRIVILEGE       (11L)
#define SE_SYSTEMTIME_PRIVILEGE           (12L)
#define SE_PROF_SINGLE_PROCESS_PRIVILEGE  (13L)
#define SE_INC_BASE_PRIORITY_PRIVILEGE    (14L)
#define SE_CREATE_PAGEFILE_PRIVILEGE      (15L)
#define SE_CREATE_PERMANENT_PRIVILEGE     (16L)
#define SE_BACKUP_PRIVILEGE               (17L)
#define SE_RESTORE_PRIVILEGE              (18L)
#define SE_SHUTDOWN_PRIVILEGE             (19L)
#define SE_DEBUG_PRIVILEGE                (20L)
#define SE_AUDIT_PRIVILEGE                (21L)
#define SE_SYSTEM_ENVIRONMENT_PRIVILEGE   (22L)
#define SE_CHANGE_NOTIFY_PRIVILEGE        (23L)
#define SE_REMOTE_SHUTDOWN_PRIVILEGE      (24L)
#define SE_MAX_WELL_KNOWN_PRIVILEGE       (SE_REMOTE_SHUTDOWN_PRIVILEGE)
#endif

/* ENUMERATIONS **************************************************************/

/* TYPES *********************************************************************/

#ifndef NTOS_MODE_USER
typedef struct _SEP_AUDIT_POLICY_CATEGORIES
{
    UCHAR System:4;
    UCHAR Logon:4;
    UCHAR ObjectAccess:4;
    UCHAR PrivilegeUse:4;
    UCHAR DetailedTracking:4;
    UCHAR PolicyChange:4;
    UCHAR AccountManagement:4;
    UCHAR DirectoryServiceAccess:4;
    UCHAR AccountLogon:4;
} SEP_AUDIT_POLICY_CATEGORIES, *PSEP_AUDIT_POLICY_CATEGORIES;

typedef struct _SEP_AUDIT_POLICY_OVERLAY
{
    ULONGLONG PolicyBits:36;
    UCHAR SetBit:1;
} SEP_AUDIT_POLICY_OVERLAY, *PSEP_AUDIT_POLICY_OVERLAY;

typedef struct _SEP_AUDIT_POLICY
{
    union
    {
        SEP_AUDIT_POLICY_CATEGORIES PolicyElements;
        SEP_AUDIT_POLICY_OVERLAY PolicyOverlay;
        ULONGLONG Overlay;
    };
} SEP_AUDIT_POLICY, *PSEP_AUDIT_POLICY;

typedef struct _TOKEN
{
    TOKEN_SOURCE TokenSource;                         /* 0x00 */
    LUID TokenId;                                     /* 0x10 */
    LUID AuthenticationId;                            /* 0x18 */
    LUID ParentTokenId;                               /* 0x20 */
    LARGE_INTEGER ExpirationTime;                     /* 0x28 */
    struct _ERESOURCE *TokenLock;                     /* 0x30 */
    SEP_AUDIT_POLICY  AuditPolicy;                    /* 0x38 */
    LUID ModifiedId;                                  /* 0x40 */
    ULONG SessionId;                                  /* 0x48 */
    ULONG UserAndGroupCount;                          /* 0x4C */
    ULONG RestrictedSidCount;                         /* 0x50 */
    ULONG PrivilegeCount;                             /* 0x54 */
    ULONG VariableLength;                             /* 0x58 */
    ULONG DynamicCharged;                             /* 0x5C */
    ULONG DynamicAvailable;                           /* 0x60 */
    ULONG DefaultOwnerIndex;                          /* 0x64 */
    PSID_AND_ATTRIBUTES UserAndGroups;                /* 0x68 */
    PSID_AND_ATTRIBUTES RestrictedSids;               /* 0x6C */
    PSID PrimaryGroup;                                /* 0x70 */
    PLUID_AND_ATTRIBUTES Privileges;                  /* 0x74 */
    PULONG DynamicPart;                               /* 0x78 */
    PACL DefaultDacl;                                 /* 0x7C */
    TOKEN_TYPE TokenType;                             /* 0x80 */
    SECURITY_IMPERSONATION_LEVEL ImpersonationLevel;  /* 0x84 */
    ULONG TokenFlags;                                 /* 0x88 */
    BOOLEAN TokenInUse;                               /* 0x8C */
    PVOID ProxyData;                                  /* 0x90 */
    PVOID AuditData;                                  /* 0x94 */
    LUID OriginatingLogonSession;                     /* 0x98 */
    ULONG VariablePart;                               /* 0xA0 */
} TOKEN, *PTOKEN;

typedef struct _AUX_DATA
{
    PPRIVILEGE_SET PrivilegeSet;
    GENERIC_MAPPING GenericMapping;
    ULONG Reserved;
} AUX_DATA, *PAUX_DATA;

typedef struct _SE_AUDIT_PROCESS_CREATION_INFO
{
    POBJECT_NAME_INFORMATION ImageFileName;
} SE_AUDIT_PROCESS_CREATION_INFO, *PSE_AUDIT_PROCESS_CREATION_INFO;

#endif
#endif
