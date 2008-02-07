/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/advapi32/sec/misc.c
 * PURPOSE:         Miscellaneous security functions (some ported from Wine)
 */

#include <advapi32.h>

#define NDEBUG
#include <debug.h>

/* Needed for LookupAccountNameW implementation from Wine */

typedef struct _AccountSid {
    WELL_KNOWN_SID_TYPE type;
    LPCWSTR account;
    LPCWSTR domain;
    SID_NAME_USE name_use;
} AccountSid;

static const WCHAR Account_Operators[] = { 'A','c','c','o','u','n','t',' ','O','p','e','r','a','t','o','r','s',0 };
static const WCHAR Administrator[] = {'A','d','m','i','n','i','s','t','r','a','t','o','r',0 };
static const WCHAR Administrators[] = { 'A','d','m','i','n','i','s','t','r','a','t','o','r','s',0 };
static const WCHAR ANONYMOUS_LOGON[] = { 'A','N','O','N','Y','M','O','U','S',' ','L','O','G','O','N',0 };
static const WCHAR Authenticated_Users[] = { 'A','u','t','h','e','n','t','i','c','a','t','e','d',' ','U','s','e','r','s',0 };
static const WCHAR Backup_Operators[] = { 'B','a','c','k','u','p',' ','O','p','e','r','a','t','o','r','s',0 };
static const WCHAR BATCH[] = { 'B','A','T','C','H',0 };
static const WCHAR Blank[] = { 0 };
static const WCHAR BUILTIN[] = { 'B','U','I','L','T','I','N',0 };
static const WCHAR Cert_Publishers[] = { 'C','e','r','t',' ','P','u','b','l','i','s','h','e','r','s',0 };
static const WCHAR CREATOR_GROUP[] = { 'C','R','E','A','T','O','R',' ','G','R','O','U','P',0 };
static const WCHAR CREATOR_GROUP_SERVER[] = { 'C','R','E','A','T','O','R',' ','G','R','O','U','P',' ','S','E','R','V','E','R',0 };
static const WCHAR CREATOR_OWNER[] = { 'C','R','E','A','T','O','R',' ','O','W','N','E','R',0 };
static const WCHAR CREATOR_OWNER_SERVER[] = { 'C','R','E','A','T','O','R',' ','O','W','N','E','R',' ','S','E','R','V','E','R',0 };
static const WCHAR DIALUP[] = { 'D','I','A','L','U','P',0 };
static const WCHAR Digest_Authentication[] = { 'D','i','g','e','s','t',' ','A','u','t','h','e','n','t','i','c','a','t','i','o','n',0 };
static const WCHAR DOMAIN[] = {'D','O','M','A','I','N',0};
static const WCHAR Domain_Admins[] = { 'D','o','m','a','i','n',' ','A','d','m','i','n','s',0 };
static const WCHAR Domain_Computers[] = { 'D','o','m','a','i','n',' ','C','o','m','p','u','t','e','r','s',0 };
static const WCHAR Domain_Controllers[] = { 'D','o','m','a','i','n',' ','C','o','n','t','r','o','l','l','e','r','s',0 };
static const WCHAR Domain_Guests[] = { 'D','o','m','a','i','n',' ','G','u','e','s','t','s',0 };
static const WCHAR Domain_Users[] = { 'D','o','m','a','i','n',' ','U','s','e','r','s',0 };
static const WCHAR Enterprise_Admins[] = { 'E','n','t','e','r','p','r','i','s','e',' ','A','d','m','i','n','s',0 };
static const WCHAR ENTERPRISE_DOMAIN_CONTROLLERS[] = { 'E','N','T','E','R','P','R','I','S','E',' ','D','O','M','A','I','N',' ','C','O','N','T','R','O','L','L','E','R','S',0 };
static const WCHAR Everyone[] = { 'E','v','e','r','y','o','n','e',0 };
static const WCHAR Group_Policy_Creator_Owners[] = { 'G','r','o','u','p',' ','P','o','l','i','c','y',' ','C','r','e','a','t','o','r',' ','O','w','n','e','r','s',0 };
static const WCHAR Guest[] = { 'G','u','e','s','t',0 };
static const WCHAR Guests[] = { 'G','u','e','s','t','s',0 };
static const WCHAR INTERACTIVE[] = { 'I','N','T','E','R','A','C','T','I','V','E',0 };
static const WCHAR LOCAL[] = { 'L','O','C','A','L',0 };
static const WCHAR LOCAL_SERVICE[] = { 'L','O','C','A','L',' ','S','E','R','V','I','C','E',0 };
static const WCHAR NETWORK[] = { 'N','E','T','W','O','R','K',0 };
static const WCHAR Network_Configuration_Operators[] = { 'N','e','t','w','o','r','k',' ','C','o','n','f','i','g','u','r','a','t','i','o','n',' ','O','p','e','r','a','t','o','r','s',0 };
static const WCHAR NETWORK_SERVICE[] = { 'N','E','T','W','O','R','K',' ','S','E','R','V','I','C','E',0 };
static const WCHAR NT_AUTHORITY[] = { 'N','T',' ','A','U','T','H','O','R','I','T','Y',0 };
static const WCHAR NT_Pseudo_Domain[] = { 'N','T',' ','P','s','e','u','d','o',' ','D','o','m','a','i','n',0 };
static const WCHAR NTML_Authentication[] = { 'N','T','M','L',' ','A','u','t','h','e','n','t','i','c','a','t','i','o','n',0 };
static const WCHAR NULL_SID[] = { 'N','U','L','L',' ','S','I','D',0 };
static const WCHAR Other_Organization[] = { 'O','t','h','e','r',' ','O','r','g','a','n','i','z','a','t','i','o','n',0 };
static const WCHAR Performance_Log_Users[] = { 'P','e','r','f','o','r','m','a','n','c','e',' ','L','o','g',' ','U','s','e','r','s',0 };
static const WCHAR Performance_Monitor_Users[] = { 'P','e','r','f','o','r','m','a','n','c','e',' ','M','o','n','i','t','o','r',' ','U','s','e','r','s',0 };
static const WCHAR Power_Users[] = { 'P','o','w','e','r',' ','U','s','e','r','s',0 };
static const WCHAR Pre_Windows_2000_Compatible_Access[] = { 'P','r','e','-','W','i','n','d','o','w','s',' ','2','0','0','0',' ','C','o','m','p','a','t','i','b','l','e',' ','A','c','c','e','s','s',0 };
static const WCHAR Print_Operators[] = { 'P','r','i','n','t',' ','O','p','e','r','a','t','o','r','s',0 };
static const WCHAR PROXY[] = { 'P','R','O','X','Y',0 };
static const WCHAR RAS_and_IAS_Servers[] = { 'R','A','S',' ','a','n','d',' ','I','A','S',' ','S','e','r','v','e','r','s',0 };
static const WCHAR Remote_Desktop_Users[] = { 'R','e','m','o','t','e',' ','D','e','s','k','t','o','p',' ','U','s','e','r','s',0 };
static const WCHAR REMOTE_INTERACTIVE_LOGON[] = { 'R','E','M','O','T','E',' ','I','N','T','E','R','A','C','T','I','V','E',' ','L','O','G','O','N',0 };
static const WCHAR Replicators[] = { 'R','e','p','l','i','c','a','t','o','r','s',0 };
static const WCHAR RESTRICTED[] = { 'R','E','S','T','R','I','C','T','E','D',0 };
static const WCHAR SChannel_Authentication[] = { 'S','C','h','a','n','n','e','l',' ','A','u','t','h','e','n','t','i','c','a','t','i','o','n',0 };
static const WCHAR Schema_Admins[] = { 'S','c','h','e','m','a',' ','A','d','m','i','n','s',0 };
static const WCHAR SELF[] = { 'S','E','L','F',0 };
static const WCHAR Server_Operators[] = { 'S','e','r','v','e','r',' ','O','p','e','r','a','t','o','r','s',0 };
static const WCHAR SERVICE[] = { 'S','E','R','V','I','C','E',0 };
static const WCHAR SYSTEM[] = { 'S','Y','S','T','E','M',0 };
static const WCHAR TERMINAL_SERVER_USER[] = { 'T','E','R','M','I','N','A','L',' ','S','E','R','V','E','R',' ','U','S','E','R',0 };
static const WCHAR This_Organization[] = { 'T','h','i','s',' ','O','r','g','a','n','i','z','a','t','i','o','n',0 };
static const WCHAR Users[] = { 'U','s','e','r','s',0 };

static const AccountSid ACCOUNT_SIDS[] = {
    { WinNullSid, NULL_SID, Blank, SidTypeWellKnownGroup },
    { WinWorldSid, Everyone, Blank, SidTypeWellKnownGroup },
    { WinLocalSid, LOCAL, Blank, SidTypeWellKnownGroup },
    { WinCreatorOwnerSid, CREATOR_OWNER, Blank, SidTypeWellKnownGroup },
    { WinCreatorGroupSid, CREATOR_GROUP, Blank, SidTypeWellKnownGroup },
    { WinCreatorOwnerServerSid, CREATOR_OWNER_SERVER, Blank, SidTypeWellKnownGroup },
    { WinCreatorGroupServerSid, CREATOR_GROUP_SERVER, Blank, SidTypeWellKnownGroup },
    { WinNtAuthoritySid, NT_Pseudo_Domain, NT_Pseudo_Domain, SidTypeDomain },
    { WinDialupSid, DIALUP, NT_AUTHORITY, SidTypeWellKnownGroup },
    { WinNetworkSid, NETWORK, NT_AUTHORITY, SidTypeWellKnownGroup },
    { WinBatchSid, BATCH, NT_AUTHORITY, SidTypeWellKnownGroup },
    { WinInteractiveSid, INTERACTIVE, NT_AUTHORITY, SidTypeWellKnownGroup },
    { WinServiceSid, SERVICE, NT_AUTHORITY, SidTypeWellKnownGroup },
    { WinAnonymousSid, ANONYMOUS_LOGON, NT_AUTHORITY, SidTypeWellKnownGroup },
    { WinProxySid, PROXY, NT_AUTHORITY, SidTypeWellKnownGroup },
    { WinEnterpriseControllersSid, ENTERPRISE_DOMAIN_CONTROLLERS, NT_AUTHORITY, SidTypeWellKnownGroup },
    { WinSelfSid, SELF, NT_AUTHORITY, SidTypeWellKnownGroup },
    { WinAuthenticatedUserSid, Authenticated_Users, NT_AUTHORITY, SidTypeWellKnownGroup },
    { WinRestrictedCodeSid, RESTRICTED, NT_AUTHORITY, SidTypeWellKnownGroup },
    { WinTerminalServerSid, TERMINAL_SERVER_USER, NT_AUTHORITY, SidTypeWellKnownGroup },
    { WinRemoteLogonIdSid, REMOTE_INTERACTIVE_LOGON, NT_AUTHORITY, SidTypeWellKnownGroup },
    { WinLocalSystemSid, SYSTEM, NT_AUTHORITY, SidTypeWellKnownGroup },
    { WinLocalServiceSid, LOCAL_SERVICE, NT_AUTHORITY, SidTypeWellKnownGroup },
    { WinNetworkServiceSid, NETWORK_SERVICE, NT_AUTHORITY, SidTypeWellKnownGroup },
    { WinBuiltinDomainSid, BUILTIN, BUILTIN, SidTypeDomain },
    { WinBuiltinAdministratorsSid, Administrators, BUILTIN, SidTypeAlias },
    { WinBuiltinUsersSid, Users, BUILTIN, SidTypeAlias },
    { WinBuiltinGuestsSid, Guests, BUILTIN, SidTypeAlias },
    { WinBuiltinPowerUsersSid, Power_Users, BUILTIN, SidTypeAlias },
    { WinBuiltinAccountOperatorsSid, Account_Operators, BUILTIN, SidTypeAlias },
    { WinBuiltinSystemOperatorsSid, Server_Operators, BUILTIN, SidTypeAlias },
    { WinBuiltinPrintOperatorsSid, Print_Operators, BUILTIN, SidTypeAlias },
    { WinBuiltinBackupOperatorsSid, Backup_Operators, BUILTIN, SidTypeAlias },
    { WinBuiltinReplicatorSid, Replicators, BUILTIN, SidTypeAlias },
    { WinBuiltinPreWindows2000CompatibleAccessSid, Pre_Windows_2000_Compatible_Access, BUILTIN, SidTypeAlias },
    { WinBuiltinRemoteDesktopUsersSid, Remote_Desktop_Users, BUILTIN, SidTypeAlias },
    { WinBuiltinNetworkConfigurationOperatorsSid, Network_Configuration_Operators, BUILTIN, SidTypeAlias },
    { WinNTLMAuthenticationSid, NTML_Authentication, NT_AUTHORITY, SidTypeWellKnownGroup },
    { WinDigestAuthenticationSid, Digest_Authentication, NT_AUTHORITY, SidTypeWellKnownGroup },
    { WinSChannelAuthenticationSid, SChannel_Authentication, NT_AUTHORITY, SidTypeWellKnownGroup },
    { WinThisOrganizationSid, This_Organization, NT_AUTHORITY, SidTypeWellKnownGroup },
    { WinOtherOrganizationSid, Other_Organization, NT_AUTHORITY, SidTypeWellKnownGroup },
    { WinBuiltinPerfMonitoringUsersSid, Performance_Monitor_Users, BUILTIN, SidTypeAlias },
    { WinBuiltinPerfLoggingUsersSid, Performance_Log_Users, BUILTIN, SidTypeAlias },
};

/* Interface to ntmarta.dll ***************************************************/

NTMARTA NtMartaStatic = { 0 };
static PNTMARTA NtMarta = NULL;

#define FindNtMartaProc(Name)                                                  \
    NtMartaStatic.Name = (PVOID)GetProcAddress(NtMartaStatic.hDllInstance,     \
                                               "Acc" # Name );                 \
    if (NtMartaStatic.Name == NULL)                                            \
    {                                                                          \
        return GetLastError();                                                 \
    }

static DWORD
LoadAndInitializeNtMarta(VOID)
{
    /* this code may be executed simultaneously by multiple threads in case they're
       trying to initialize the interface at the same time, but that's no problem
       because the pointers returned by GetProcAddress will be the same. However,
       only one of the threads will change the NtMarta pointer to the NtMartaStatic
       structure, the others threads will detect that there were other threads
       initializing the structure faster and will release the reference to the
       DLL */

    NtMartaStatic.hDllInstance = LoadLibraryW(L"ntmarta.dll");
    if (NtMartaStatic.hDllInstance == NULL)
    {
        return GetLastError();
    }

#if 0
    FindNtMartaProc(LookupAccountTrustee);
    FindNtMartaProc(LookupAccountName);
    FindNtMartaProc(LookupAccountSid);
    FindNtMartaProc(SetEntriesInAList);
    FindNtMartaProc(ConvertAccessToSecurityDescriptor);
    FindNtMartaProc(ConvertSDToAccess);
    FindNtMartaProc(ConvertAclToAccess);
    FindNtMartaProc(GetAccessForTrustee);
    FindNtMartaProc(GetExplicitEntries);
#endif
    FindNtMartaProc(RewriteGetNamedRights);
    FindNtMartaProc(RewriteSetNamedRights);
    FindNtMartaProc(RewriteGetHandleRights);
    FindNtMartaProc(RewriteSetHandleRights);
    FindNtMartaProc(RewriteSetEntriesInAcl);
    FindNtMartaProc(RewriteGetExplicitEntriesFromAcl);
    FindNtMartaProc(TreeResetNamedSecurityInfo);
    FindNtMartaProc(GetInheritanceSource);
    FindNtMartaProc(FreeIndexArray);

    return ERROR_SUCCESS;
}

DWORD
CheckNtMartaPresent(VOID)
{
    DWORD ErrorCode;

    if (InterlockedCompareExchangePointer(&NtMarta,
                                          NULL,
                                          NULL) == NULL)
    {
        /* we're the first one trying to use ntmarta, initialize it and change
           the pointer after initialization */
        ErrorCode = LoadAndInitializeNtMarta();

        if (ErrorCode == ERROR_SUCCESS)
        {
            /* try change the NtMarta pointer */
            if (InterlockedCompareExchangePointer(&NtMarta,
                                                  &NtMartaStatic,
                                                  NULL) != NULL)
            {
                /* another thread initialized ntmarta in the meanwhile, release
                   the reference of the dll loaded. */
                FreeLibrary(NtMartaStatic.hDllInstance);
            }
        }
#if DBG
        else
        {
            DPRINT1("Failed to initialize ntmarta.dll! Error: 0x%x", ErrorCode);
        }
#endif
    }
    else
    {
        /* ntmarta was already initialized */
        ErrorCode = ERROR_SUCCESS;
    }

    return ErrorCode;
}

VOID UnloadNtMarta(VOID)
{
    if (InterlockedExchangePointer(&NtMarta,
                                   NULL) != NULL)
    {
        FreeLibrary(NtMartaStatic.hDllInstance);
    }
}

/******************************************************************************/

/*
 * @implemented
 */
BOOL STDCALL
AreAllAccessesGranted(DWORD GrantedAccess,
		      DWORD DesiredAccess)
{
  return((BOOL)RtlAreAllAccessesGranted(GrantedAccess,
					DesiredAccess));
}


/*
 * @implemented
 */
BOOL STDCALL
AreAnyAccessesGranted(DWORD GrantedAccess,
		      DWORD DesiredAccess)
{
  return((BOOL)RtlAreAnyAccessesGranted(GrantedAccess,
					DesiredAccess));
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
BOOL WINAPI
GetFileSecurityA(LPCSTR lpFileName,
		 SECURITY_INFORMATION RequestedInformation,
		 PSECURITY_DESCRIPTOR pSecurityDescriptor,
		 DWORD nLength,
		 LPDWORD lpnLengthNeeded)
{
  UNICODE_STRING FileName;
  NTSTATUS Status;
  BOOL bResult;

  Status = RtlCreateUnicodeStringFromAsciiz(&FileName,
					    (LPSTR)lpFileName);
  if (!NT_SUCCESS(Status))
    {
      SetLastError(RtlNtStatusToDosError(Status));
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
BOOL WINAPI
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

  DPRINT("GetFileSecurityW() called\n");

  if (RequestedInformation &
      (OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION))
    {
      AccessMask |= READ_CONTROL;
    }

  if (RequestedInformation & SACL_SECURITY_INFORMATION)
    {
      AccessMask |= ACCESS_SYSTEM_SECURITY;
    }

  if (!RtlDosPathNameToNtPathName_U(lpFileName,
				    &FileName,
				    NULL,
				    NULL))
    {
      DPRINT("Invalid path\n");
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
      DPRINT("NtOpenFile() failed (Status %lx)\n", Status);
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
      DPRINT("NtQuerySecurityObject() failed (Status %lx)\n", Status);
      SetLastError(RtlNtStatusToDosError(Status));
      return FALSE;
    }

  return TRUE;
}


/*
 * @implemented
 */
BOOL STDCALL
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
      return(FALSE);
    }
  return(TRUE);
}


/******************************************************************************
 * SetFileSecurityA [ADVAPI32.@]
 * Sets the security of a file or directory
 *
 * @implemented
 */
BOOL STDCALL
SetFileSecurityA (LPCSTR lpFileName,
		  SECURITY_INFORMATION SecurityInformation,
		  PSECURITY_DESCRIPTOR pSecurityDescriptor)
{
  UNICODE_STRING FileName;
  NTSTATUS Status;
  BOOL bResult;

  Status = RtlCreateUnicodeStringFromAsciiz(&FileName,
					    (LPSTR)lpFileName);
  if (!NT_SUCCESS(Status))
    {
      SetLastError(RtlNtStatusToDosError(Status));
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
BOOL STDCALL
SetFileSecurityW (LPCWSTR lpFileName,
		  SECURITY_INFORMATION SecurityInformation,
		  PSECURITY_DESCRIPTOR pSecurityDescriptor)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  IO_STATUS_BLOCK StatusBlock;
  UNICODE_STRING FileName;
  ULONG AccessMask = 0;
  HANDLE FileHandle;
  NTSTATUS Status;

  DPRINT("SetFileSecurityW() called\n");

  if (SecurityInformation &
      (OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION))
    {
      AccessMask |= WRITE_OWNER;
    }

  if (SecurityInformation & DACL_SECURITY_INFORMATION)
    {
      AccessMask |= WRITE_DAC;
    }

  if (SecurityInformation & SACL_SECURITY_INFORMATION)
    {
      AccessMask |= ACCESS_SYSTEM_SECURITY;
    }

  if (!RtlDosPathNameToNtPathName_U(lpFileName,
				    &FileName,
				    NULL,
				    NULL))
    {
      DPRINT("Invalid path\n");
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
      DPRINT("NtOpenFile() failed (Status %lx)\n", Status);
      SetLastError(RtlNtStatusToDosError(Status));
      return FALSE;
    }

  Status = NtSetSecurityObject(FileHandle,
			       SecurityInformation,
			       pSecurityDescriptor);
  NtClose(FileHandle);

  if (!NT_SUCCESS(Status))
    {
      DPRINT("NtSetSecurityObject() failed (Status %lx)\n", Status);
      SetLastError(RtlNtStatusToDosError(Status));
      return FALSE;
    }

  return TRUE;
}


/*
 * @implemented
 */
BOOL STDCALL
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
ImpersonateAnonymousToken(IN HANDLE ThreadHandle)
{
    NTSTATUS Status;

    Status = NtImpersonateAnonymousToken(ThreadHandle);

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
BOOL STDCALL
ImpersonateLoggedOnUser(HANDLE hToken)
{
  SECURITY_QUALITY_OF_SERVICE Qos;
  OBJECT_ATTRIBUTES ObjectAttributes;
  HANDLE NewToken;
  TOKEN_TYPE Type;
  ULONG ReturnLength;
  BOOL Duplicated;
  NTSTATUS Status;

  /* Get the token type */
  Status = NtQueryInformationToken (hToken,
				    TokenType,
				    &Type,
				    sizeof(TOKEN_TYPE),
				    &ReturnLength);
  if (!NT_SUCCESS(Status))
    {
      SetLastError (RtlNtStatusToDosError (Status));
      return FALSE;
    }

  if (Type == TokenPrimary)
    {
      /* Create a duplicate impersonation token */
      Qos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
      Qos.ImpersonationLevel = SecurityImpersonation;
      Qos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
      Qos.EffectiveOnly = FALSE;

      ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
      ObjectAttributes.RootDirectory = NULL;
      ObjectAttributes.ObjectName = NULL;
      ObjectAttributes.Attributes = 0;
      ObjectAttributes.SecurityDescriptor = NULL;
      ObjectAttributes.SecurityQualityOfService = &Qos;

      Status = NtDuplicateToken (hToken,
				 TOKEN_IMPERSONATE | TOKEN_QUERY,
				 &ObjectAttributes,
				 FALSE,
				 TokenImpersonation,
				 &NewToken);
      if (!NT_SUCCESS(Status))
	{
	  SetLastError (RtlNtStatusToDosError (Status));
	  return FALSE;
	}

      Duplicated = TRUE;
    }
  else
    {
      /* User the original impersonation token */
      NewToken = hToken;
      Duplicated = FALSE;
    }

  /* Impersonate the the current thread */
  Status = NtSetInformationThread (NtCurrentThread (),
				   ThreadImpersonationToken,
				   &NewToken,
				   sizeof(HANDLE));

  if (Duplicated == TRUE)
    {
      NtClose (NewToken);
    }

  if (!NT_SUCCESS(Status))
    {
      SetLastError (RtlNtStatusToDosError (Status));
      return FALSE;
    }

  return TRUE;
}


/*
 * @implemented
 */
BOOL STDCALL
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
BOOL STDCALL
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


/******************************************************************************
 * GetUserNameA [ADVAPI32.@]
 *
 * Get the current user name.
 *
 * PARAMS
 *  lpszName [O]   Destination for the user name.
 *  lpSize   [I/O] Size of lpszName.
 *
 *
 * @implemented
 */
BOOL WINAPI
GetUserNameA( LPSTR lpszName, LPDWORD lpSize )
{
  UNICODE_STRING NameW;
  ANSI_STRING NameA;
  BOOL Ret;

  /* apparently Win doesn't check whether lpSize is valid at all! */

  NameW.Length = 0;
  NameW.MaximumLength = (*lpSize) * sizeof(WCHAR);
  NameW.Buffer = LocalAlloc(LMEM_FIXED, NameW.MaximumLength);
  if(NameW.Buffer == NULL)
  {
    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return FALSE;
  }

  NameA.Length = 0;
  NameA.MaximumLength = ((*lpSize) < 0xFFFF ? (USHORT)(*lpSize) : 0xFFFF);
  NameA.Buffer = lpszName;

  Ret = GetUserNameW(NameW.Buffer,
                     lpSize);
  if(Ret)
  {
    RtlUnicodeStringToAnsiString(&NameA, &NameW, FALSE);
    NameA.Buffer[NameA.Length] = '\0';

    *lpSize = NameA.Length + 1;
  }

  LocalFree(NameW.Buffer);

  return Ret;
}

/******************************************************************************
 * GetUserNameW [ADVAPI32.@]
 *
 * See GetUserNameA.
 *
 * @implemented
 */
BOOL WINAPI
GetUserNameW ( LPWSTR lpszName, LPDWORD lpSize )
{
  HANDLE hToken = INVALID_HANDLE_VALUE;
  DWORD tu_len = 0;
  char* tu_buf = NULL;
  TOKEN_USER* token_user = NULL;
  DWORD an_len = 0;
  SID_NAME_USE snu = SidTypeUser;
  WCHAR* domain_name = NULL;
  DWORD dn_len = 0;

  if ( !OpenThreadToken ( GetCurrentThread(), TOKEN_QUERY, FALSE, &hToken ) )
  {
    DWORD dwLastError = GetLastError();
    if ( dwLastError != ERROR_NO_TOKEN
      && dwLastError != ERROR_NO_IMPERSONATION_TOKEN )
    {
      /* don't call SetLastError(),
         as OpenThreadToken() ought to have set one */
      return FALSE;
    }
    if ( !OpenProcessToken ( GetCurrentProcess(), TOKEN_QUERY, &hToken ) )
    {
      /* don't call SetLastError(),
         as OpenProcessToken() ought to have set one */
      return FALSE;
    }
  }
  tu_buf = LocalAlloc ( LMEM_FIXED, 36 );
  if ( !tu_buf )
  {
    SetLastError ( ERROR_NOT_ENOUGH_MEMORY );
    return FALSE;
  }
  if ( !GetTokenInformation ( hToken, TokenUser, tu_buf, 36, &tu_len ) || tu_len > 36 )
  {
    LocalFree ( tu_buf );
    tu_buf = LocalAlloc ( LMEM_FIXED, tu_len );
    if ( !tu_buf )
    {
      SetLastError ( ERROR_NOT_ENOUGH_MEMORY );
      return FALSE;
    }
    if ( !GetTokenInformation ( hToken, TokenUser, tu_buf, tu_len, &tu_len ) )
    {
      /* don't call SetLastError(),
         as GetTokenInformation() ought to have set one */
      LocalFree ( tu_buf );
      CloseHandle ( hToken );
      return FALSE;
    }
  }
  token_user = (TOKEN_USER*)tu_buf;

  an_len = *lpSize;
  dn_len = 32;
  domain_name = LocalAlloc ( LMEM_FIXED, dn_len * sizeof(WCHAR) );
  if ( !domain_name )
  {
    LocalFree ( tu_buf );
    SetLastError ( ERROR_NOT_ENOUGH_MEMORY );
    return FALSE;
  }
  if ( !LookupAccountSidW ( NULL, token_user->User.Sid, lpszName, &an_len, domain_name, &dn_len, &snu )
    || dn_len > 32 )
  {
    if ( dn_len > 32 )
    {
      LocalFree ( domain_name );
      domain_name = LocalAlloc ( LMEM_FIXED, dn_len * sizeof(WCHAR) );
      if ( !domain_name )
      {
        LocalFree ( tu_buf );
        SetLastError ( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
      }
    }
    if ( !LookupAccountSidW ( NULL, token_user->User.Sid, lpszName, &an_len, domain_name, &dn_len, &snu ) )
    {
      /* don't call SetLastError(),
         as LookupAccountSid() ought to have set one */
      LocalFree ( domain_name );
      CloseHandle ( hToken );
      return FALSE;
    }
  }

  LocalFree ( domain_name );
  LocalFree ( tu_buf );
  CloseHandle ( hToken );

  if ( an_len > *lpSize )
  {
    *lpSize = an_len;
    SetLastError(ERROR_INSUFFICIENT_BUFFER);
    return FALSE;
  }

  return TRUE;
}


/******************************************************************************
 * LookupAccountSidA [ADVAPI32.@]
 *
 * @implemented
 */
BOOL STDCALL
LookupAccountSidA (LPCSTR lpSystemName,
		   PSID lpSid,
		   LPSTR lpName,
		   LPDWORD cchName,
		   LPSTR lpReferencedDomainName,
		   LPDWORD cchReferencedDomainName,
		   PSID_NAME_USE peUse)
{
  UNICODE_STRING NameW, ReferencedDomainNameW, SystemNameW;
  DWORD szName, szReferencedDomainName;
  BOOL Ret;

  /*
   * save the buffer sizes the caller passed to us, as they may get modified and
   * we require the original values when converting back to ansi
   */
  szName = *cchName;
  szReferencedDomainName = *cchReferencedDomainName;

  /*
   * allocate buffers for the unicode strings to receive
   */

  if(szName > 0)
  {
    NameW.Length = 0;
    NameW.MaximumLength = szName * sizeof(WCHAR);
    NameW.Buffer = (PWSTR)LocalAlloc(LMEM_FIXED, NameW.MaximumLength);
    if(NameW.Buffer == NULL)
    {
      SetLastError(ERROR_OUTOFMEMORY);
      return FALSE;
    }
  }
  else
    NameW.Buffer = NULL;

  if(szReferencedDomainName > 0)
  {
    ReferencedDomainNameW.Length = 0;
    ReferencedDomainNameW.MaximumLength = szReferencedDomainName * sizeof(WCHAR);
    ReferencedDomainNameW.Buffer = (PWSTR)LocalAlloc(LMEM_FIXED, ReferencedDomainNameW.MaximumLength);
    if(ReferencedDomainNameW.Buffer == NULL)
    {
      if(szName > 0)
      {
        LocalFree(NameW.Buffer);
      }
      SetLastError(ERROR_OUTOFMEMORY);
      return FALSE;
    }
  }
  else
    ReferencedDomainNameW.Buffer = NULL;

  /*
   * convert the system name to unicode - if present
   */

  if(lpSystemName != NULL)
  {
    ANSI_STRING SystemNameA;

    RtlInitAnsiString(&SystemNameA, lpSystemName);
    RtlAnsiStringToUnicodeString(&SystemNameW, &SystemNameA, TRUE);
  }
  else
    SystemNameW.Buffer = NULL;

  /*
   * it's time to call the unicode version
   */

  Ret = LookupAccountSidW(SystemNameW.Buffer,
                          lpSid,
                          NameW.Buffer,
                          cchName,
                          ReferencedDomainNameW.Buffer,
                          cchReferencedDomainName,
                          peUse);
  if(Ret)
  {
    /*
     * convert unicode strings back to ansi, don't forget that we can't convert
     * more than 0xFFFF (USHORT) characters! Also don't forget to explicitly
     * terminate the converted string, the Rtl functions don't do that!
     */
    if(lpName != NULL)
    {
      ANSI_STRING NameA;

      NameA.Length = 0;
      NameA.MaximumLength = ((szName <= 0xFFFF) ? (USHORT)szName : 0xFFFF);
      NameA.Buffer = lpName;

      RtlUnicodeStringToAnsiString(&NameA, &NameW, FALSE);
      NameA.Buffer[NameA.Length] = '\0';
    }

    if(lpReferencedDomainName != NULL)
    {
      ANSI_STRING ReferencedDomainNameA;

      ReferencedDomainNameA.Length = 0;
      ReferencedDomainNameA.MaximumLength = ((szReferencedDomainName <= 0xFFFF) ?
                                             (USHORT)szReferencedDomainName : 0xFFFF);
      ReferencedDomainNameA.Buffer = lpReferencedDomainName;

      RtlUnicodeStringToAnsiString(&ReferencedDomainNameA, &ReferencedDomainNameW, FALSE);
      ReferencedDomainNameA.Buffer[ReferencedDomainNameA.Length] = '\0';
    }
  }

  /*
   * free previously allocated buffers
   */

  if(SystemNameW.Buffer != NULL)
  {
    RtlFreeUnicodeString(&SystemNameW);
  }
  if(NameW.Buffer != NULL)
  {
    LocalFree(NameW.Buffer);
  }
  if(ReferencedDomainNameW.Buffer != NULL)
  {
    LocalFree(ReferencedDomainNameW.Buffer);
  }

  return Ret;
}


/******************************************************************************
 * LookupAccountSidW [ADVAPI32.@]
 *
 * @implemented
 */
BOOL WINAPI
LookupAccountSidW (
	LPCWSTR pSystemName,
	PSID pSid,
	LPWSTR pAccountName,
	LPDWORD pdwAccountName,
	LPWSTR pDomainName,
	LPDWORD pdwDomainName,
	PSID_NAME_USE peUse )
{
	LSA_UNICODE_STRING SystemName;
	LSA_OBJECT_ATTRIBUTES ObjectAttributes = {0};
	LSA_HANDLE PolicyHandle = NULL;
	NTSTATUS Status;
	PLSA_REFERENCED_DOMAIN_LIST ReferencedDomain = NULL;
	PLSA_TRANSLATED_NAME TranslatedName = NULL;
	BOOL ret;

	RtlInitUnicodeString ( &SystemName, pSystemName );
	Status = LsaOpenPolicy ( &SystemName, &ObjectAttributes, POLICY_LOOKUP_NAMES, &PolicyHandle );
	if ( !NT_SUCCESS(Status) )
	{
		SetLastError ( LsaNtStatusToWinError(Status) );
		return FALSE;
	}
	Status = LsaLookupSids ( PolicyHandle, 1, &pSid, &ReferencedDomain, &TranslatedName );

	LsaClose ( PolicyHandle );

	if ( !NT_SUCCESS(Status) || Status == STATUS_SOME_NOT_MAPPED )
	{
		SetLastError ( LsaNtStatusToWinError(Status) );
		ret = FALSE;
	}
	else
	{
		ret = TRUE;
		if ( TranslatedName )
		{
			DWORD dwSrcLen = TranslatedName->Name.Length / sizeof(WCHAR);
			if ( *pdwAccountName <= dwSrcLen )
			{
				*pdwAccountName = dwSrcLen + 1;
				ret = FALSE;
			}
			else
			{
				*pdwAccountName = dwSrcLen;
				if (pAccountName)
				{
					RtlCopyMemory ( pAccountName, TranslatedName->Name.Buffer, TranslatedName->Name.Length );
					pAccountName[TranslatedName->Name.Length / sizeof(WCHAR)] = L'\0';
				}
			}
			if ( peUse )
				*peUse = TranslatedName->Use;
		}

		if ( ReferencedDomain )
		{
			if ( ReferencedDomain->Entries > 0 )
			{
				DWORD dwSrcLen = ReferencedDomain->Domains[0].Name.Length / sizeof(WCHAR);
				if ( *pdwDomainName <= dwSrcLen )
				{
					*pdwDomainName = dwSrcLen + 1;
					ret = FALSE;
				}
				else
				{
					*pdwDomainName = dwSrcLen;
					RtlCopyMemory ( pDomainName, ReferencedDomain->Domains[0].Name.Buffer, ReferencedDomain->Domains[0].Name.Length );
					                pDomainName[ReferencedDomain->Domains[0].Name.Length / sizeof(WCHAR)] = L'\0';
				}
			}
		}

		if ( !ret )
			SetLastError(ERROR_INSUFFICIENT_BUFFER);
	}

	if ( ReferencedDomain )
		LsaFreeMemory ( ReferencedDomain );
	if ( TranslatedName )
		LsaFreeMemory ( TranslatedName );

	return ret;
}



/******************************************************************************
 * LookupAccountNameA [ADVAPI32.@]
 *
 * @implemented
 */
BOOL STDCALL
LookupAccountNameA (LPCSTR SystemName,
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
        lpReferencedDomainNameW = HeapAlloc(GetProcessHeap(), 0, *hReferencedDomainNameLength * sizeof(WCHAR));

    ret = LookupAccountNameW(lpSystemW.Buffer, lpAccountW.Buffer, Sid, SidLength, lpReferencedDomainNameW,
        hReferencedDomainNameLength, SidNameUse);

    if (ret && lpReferencedDomainNameW)
    {
        WideCharToMultiByte(CP_ACP, 0, lpReferencedDomainNameW, *hReferencedDomainNameLength,
            ReferencedDomainName, *hReferencedDomainNameLength, NULL, NULL);
    }

    RtlFreeUnicodeString(&lpSystemW);
    RtlFreeUnicodeString(&lpAccountW);
    HeapFree(GetProcessHeap(), 0, lpReferencedDomainNameW);

    return ret;
}


/******************************************************************************
 * LookupAccountNameW [ADVAPI32.@]
 *
 * @unimplemented
 */
BOOL WINAPI LookupAccountNameW(LPCWSTR lpSystemName, LPCWSTR lpAccountName, PSID Sid,
                               LPDWORD cbSid, LPWSTR ReferencedDomainName,
                               LPDWORD cchReferencedDomainName, PSID_NAME_USE peUse)
{
    /* Default implementation: Always return a default SID */
    SID_IDENTIFIER_AUTHORITY identifierAuthority = {SECURITY_NT_AUTHORITY};
    BOOL ret;
    PSID pSid;
    static const WCHAR dm[] = {'D','O','M','A','I','N',0};
    unsigned int i;

    DPRINT("%s %s %p %p %p %p %p - stub\n", lpSystemName, lpAccountName,
          Sid, cbSid, ReferencedDomainName, cchReferencedDomainName, peUse);

    for (i = 0; i < (sizeof(ACCOUNT_SIDS) / sizeof(ACCOUNT_SIDS[0])); i++)
    {
        if (!wcscmp(lpAccountName, ACCOUNT_SIDS[i].account))
        {
            if (*cchReferencedDomainName)
                *ReferencedDomainName = '\0';
            *cchReferencedDomainName = 0;
            *peUse = SidTypeWellKnownGroup;
            return CreateWellKnownSid(ACCOUNT_SIDS[i].type, NULL, Sid, cbSid);
        }
    }

    ret = AllocateAndInitializeSid(&identifierAuthority,
        2,
        SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS,
        0, 0, 0, 0, 0, 0,
        &pSid);

    if (!ret)
       return FALSE;

    if (!RtlValidSid(pSid))
    {
       FreeSid(pSid);
       return FALSE;
    }

    if (Sid != NULL && (*cbSid >= GetLengthSid(pSid)))
       CopySid(*cbSid, Sid, pSid);
    if (*cbSid < GetLengthSid(pSid))
    {
       SetLastError(ERROR_INSUFFICIENT_BUFFER);
       ret = FALSE;
    }
    *cbSid = GetLengthSid(pSid);
    
    if (ReferencedDomainName != NULL && (*cchReferencedDomainName > wcslen(dm)))
      wcscpy(ReferencedDomainName, dm);

    if (*cchReferencedDomainName <= wcslen(dm))
    {
       SetLastError(ERROR_INSUFFICIENT_BUFFER);
       ret = FALSE;
    }

    *cchReferencedDomainName = wcslen(dm)+1;

    FreeSid(pSid);

    return ret;
}


/**********************************************************************
 * LookupPrivilegeValueA				EXPORTED
 *
 * @implemented
 */
BOOL STDCALL
LookupPrivilegeValueA (LPCSTR lpSystemName,
		       LPCSTR lpName,
		       PLUID lpLuid)
{
  UNICODE_STRING SystemName;
  UNICODE_STRING Name;
  BOOL Result;

  /* Remote system? */
  if (lpSystemName != NULL)
    {
      RtlCreateUnicodeStringFromAsciiz (&SystemName,
					(LPSTR)lpSystemName);
    }

  /* Check the privilege name is not NULL */
  if (lpName == NULL)
    {
      SetLastError (ERROR_INVALID_PARAMETER);
      return FALSE;
    }

  RtlCreateUnicodeStringFromAsciiz (&Name,
				    (LPSTR)lpName);

  Result = LookupPrivilegeValueW ((lpSystemName != NULL) ? SystemName.Buffer : NULL,
				  Name.Buffer,
				  lpLuid);

  RtlFreeUnicodeString (&Name);

  /* Remote system? */
  if (lpSystemName != NULL)
    {
      RtlFreeUnicodeString (&SystemName);
    }

  return Result;
}


/**********************************************************************
 * LookupPrivilegeValueW				EXPORTED
 *
 * @unimplemented
 */
BOOL STDCALL
LookupPrivilegeValueW (LPCWSTR SystemName,
		       LPCWSTR PrivName,
		       PLUID Luid)
{
  static const WCHAR * const DefaultPrivNames[] =
    {
      L"SeCreateTokenPrivilege",
      L"SeAssignPrimaryTokenPrivilege",
      L"SeLockMemoryPrivilege",
      L"SeIncreaseQuotaPrivilege",
      L"SeUnsolicitedInputPrivilege",
      L"SeMachineAccountPrivilege",
      L"SeTcbPrivilege",
      L"SeSecurityPrivilege",
      L"SeTakeOwnershipPrivilege",
      L"SeLoadDriverPrivilege",
      L"SeSystemProfilePrivilege",
      L"SeSystemtimePrivilege",
      L"SeProfileSingleProcessPrivilege",
      L"SeIncreaseBasePriorityPrivilege",
      L"SeCreatePagefilePrivilege",
      L"SeCreatePermanentPrivilege",
      L"SeBackupPrivilege",
      L"SeRestorePrivilege",
      L"SeShutdownPrivilege",
      L"SeDebugPrivilege",
      L"SeAuditPrivilege",
      L"SeSystemEnvironmentPrivilege",
      L"SeChangeNotifyPrivilege",
      L"SeRemoteShutdownPrivilege",
      L"SeUndockPrivilege",
      L"SeSyncAgentPrivilege",
      L"SeEnableDelegationPrivilege",
      L"SeManageVolumePrivilege",
      L"SeImpersonatePrivilege",
      L"SeCreateGlobalPrivilege"
    };
  unsigned Priv;

  if (NULL != SystemName && L'\0' != *SystemName)
    {
      DPRINT1("LookupPrivilegeValueW: not implemented for remote system\n");
      SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
      return FALSE;
    }

  for (Priv = 0; Priv < sizeof(DefaultPrivNames) / sizeof(DefaultPrivNames[0]); Priv++)
    {
      if (0 == wcscmp(PrivName, DefaultPrivNames[Priv]))
        {
          Luid->LowPart = Priv + 1;
          Luid->HighPart = 0;
          return TRUE;
        }
    }

  DPRINT1("LookupPrivilegeValueW: no such privilege %S\n", PrivName);
  SetLastError(ERROR_NO_SUCH_PRIVILEGE);
  return FALSE;
}


/**********************************************************************
 * LookupPrivilegeDisplayNameA			EXPORTED
 *
 * @unimplemented
 */
BOOL STDCALL
LookupPrivilegeDisplayNameA (LPCSTR lpSystemName,
			     LPCSTR lpName,
			     LPSTR lpDisplayName,
			     LPDWORD cbDisplayName,
			     LPDWORD lpLanguageId)
{
  DPRINT1("LookupPrivilegeDisplayNameA: stub\n");
  SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}


/**********************************************************************
 * LookupPrivilegeDisplayNameW			EXPORTED
 *
 * @unimplemented
 */
BOOL STDCALL
LookupPrivilegeDisplayNameW (LPCWSTR lpSystemName,
			     LPCWSTR lpName,
			     LPWSTR lpDisplayName,
			     LPDWORD cbDisplayName,
			     LPDWORD lpLanguageId)
{
  DPRINT1("LookupPrivilegeDisplayNameW: stub\n");
  SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}


/**********************************************************************
 * LookupPrivilegeNameA				EXPORTED
 *
 * @unimplemented
 */
BOOL STDCALL
LookupPrivilegeNameA (LPCSTR lpSystemName,
		      PLUID lpLuid,
		      LPSTR lpName,
		      LPDWORD cbName)
{
  DPRINT1("LookupPrivilegeNameA: stub\n");
  SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}


/**********************************************************************
 * LookupPrivilegeNameW				EXPORTED
 *
 * @unimplemented
 */
BOOL STDCALL
LookupPrivilegeNameW (LPCWSTR lpSystemName,
		      PLUID lpLuid,
		      LPWSTR lpName,
		      LPDWORD cbName)
{
  DPRINT1("LookupPrivilegeNameW: stub\n");
  SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}


static DWORD
pGetSecurityInfoCheck(SECURITY_INFORMATION SecurityInfo,
                      PSID* ppsidOwner,
                      PSID* ppsidGroup,
                      PACL* ppDacl,
                      PACL* ppSacl,
                      PSECURITY_DESCRIPTOR* ppSecurityDescriptor)
{
    if ((SecurityInfo & (OWNER_SECURITY_INFORMATION |
                         GROUP_SECURITY_INFORMATION |
                         DACL_SECURITY_INFORMATION |
                         SACL_SECURITY_INFORMATION)) &&
        ppSecurityDescriptor == NULL)
    {
        /* if one of the SIDs or ACLs are present, the security descriptor
           most not be NULL */
        return ERROR_INVALID_PARAMETER;
    }
    else
    {
        /* reset the pointers unless they're ignored */
        if ((SecurityInfo & OWNER_SECURITY_INFORMATION) &&
            ppsidOwner != NULL)
        {
            *ppsidOwner = NULL;
        }
        if ((SecurityInfo & GROUP_SECURITY_INFORMATION) &&
            ppsidGroup != NULL)
        {
            *ppsidGroup = NULL;
        }
        if ((SecurityInfo & DACL_SECURITY_INFORMATION) &&
            ppDacl != NULL)
        {
            *ppDacl = NULL;
        }
        if ((SecurityInfo & SACL_SECURITY_INFORMATION) &&
            ppSacl != NULL)
        {
            *ppSacl = NULL;
        }

        if (SecurityInfo & (OWNER_SECURITY_INFORMATION |
                            GROUP_SECURITY_INFORMATION |
                            DACL_SECURITY_INFORMATION |
                            SACL_SECURITY_INFORMATION))
        {
            *ppSecurityDescriptor = NULL;
        }

        return ERROR_SUCCESS;
    }
}


static DWORD
pSetSecurityInfoCheck(PSECURITY_DESCRIPTOR pSecurityDescriptor,
                      SECURITY_INFORMATION SecurityInfo,
                      PSID psidOwner,
                      PSID psidGroup,
                      PACL pDacl,
                      PACL pSacl)
{
    /* initialize a security descriptor on the stack */
    if (!InitializeSecurityDescriptor(pSecurityDescriptor,
                                      SECURITY_DESCRIPTOR_REVISION))
    {
        return GetLastError();
    }

    if (SecurityInfo & OWNER_SECURITY_INFORMATION)
    {
        if (RtlValidSid(psidOwner))
        {
            if (!SetSecurityDescriptorOwner(pSecurityDescriptor,
                                            psidOwner,
                                            FALSE))
            {
                return GetLastError();
            }
        }
        else
        {
            return ERROR_INVALID_PARAMETER;
        }
    }

    if (SecurityInfo & GROUP_SECURITY_INFORMATION)
    {
        if (RtlValidSid(psidGroup))
        {
            if (!SetSecurityDescriptorGroup(pSecurityDescriptor,
                                            psidGroup,
                                            FALSE))
            {
                return GetLastError();
            }
        }
        else
        {
            return ERROR_INVALID_PARAMETER;
        }
    }

    if (SecurityInfo & DACL_SECURITY_INFORMATION)
    {
        if (pDacl != NULL)
        {
            if (SetSecurityDescriptorDacl(pSecurityDescriptor,
                                          TRUE,
                                          pDacl,
                                          FALSE))
            {
                /* check if the DACL needs to be protected from being
                   modified by inheritable ACEs */
                if (SecurityInfo & PROTECTED_DACL_SECURITY_INFORMATION)
                {
                    goto ProtectDacl;
                }
            }
            else
            {
                return GetLastError();
            }
        }
        else
        {
ProtectDacl:
            /* protect the DACL from being modified by inheritable ACEs */
            if (!SetSecurityDescriptorControl(pSecurityDescriptor,
                                              SE_DACL_PROTECTED,
                                              SE_DACL_PROTECTED))
            {
                return GetLastError();
            }
        }
    }

    if (SecurityInfo & SACL_SECURITY_INFORMATION)
    {
        if (pSacl != NULL)
        {
            if (SetSecurityDescriptorSacl(pSecurityDescriptor,
                                          TRUE,
                                          pSacl,
                                          FALSE))
            {
                /* check if the SACL needs to be protected from being
                   modified by inheritable ACEs */
                if (SecurityInfo & PROTECTED_SACL_SECURITY_INFORMATION)
                {
                    goto ProtectSacl;
                }
            }
            else
            {
                return GetLastError();
            }
        }
        else
        {
ProtectSacl:
            /* protect the SACL from being modified by inheritable ACEs */
            if (!SetSecurityDescriptorControl(pSecurityDescriptor,
                                              SE_SACL_PROTECTED,
                                              SE_SACL_PROTECTED))
            {
                return GetLastError();
            }
        }
    }

    return ERROR_SUCCESS;
}


/**********************************************************************
 * GetNamedSecurityInfoW			EXPORTED
 *
 * @implemented
 */
DWORD STDCALL
GetNamedSecurityInfoW(LPWSTR pObjectName,
                      SE_OBJECT_TYPE ObjectType,
                      SECURITY_INFORMATION SecurityInfo,
                      PSID *ppsidOwner,
                      PSID *ppsidGroup,
                      PACL *ppDacl,
                      PACL *ppSacl,
                      PSECURITY_DESCRIPTOR *ppSecurityDescriptor)
{
    DWORD ErrorCode;

    if (pObjectName != NULL)
    {
        ErrorCode = CheckNtMartaPresent();
        if (ErrorCode == ERROR_SUCCESS)
        {
            ErrorCode = pGetSecurityInfoCheck(SecurityInfo,
                                              ppsidOwner,
                                              ppsidGroup,
                                              ppDacl,
                                              ppSacl,
                                              ppSecurityDescriptor);

            if (ErrorCode == ERROR_SUCCESS)
            {
                /* call the MARTA provider */
                ErrorCode = AccRewriteGetNamedRights(pObjectName,
                                                     ObjectType,
                                                     SecurityInfo,
                                                     ppsidOwner,
                                                     ppsidGroup,
                                                     ppDacl,
                                                     ppSacl,
                                                     ppSecurityDescriptor);
            }
        }
    }
    else
        ErrorCode = ERROR_INVALID_PARAMETER;

    return ErrorCode;
}


/**********************************************************************
 * GetNamedSecurityInfoA			EXPORTED
 *
 * @implemented
 */
DWORD STDCALL
GetNamedSecurityInfoA(LPSTR pObjectName,
                      SE_OBJECT_TYPE ObjectType,
                      SECURITY_INFORMATION SecurityInfo,
                      PSID *ppsidOwner,
                      PSID *ppsidGroup,
                      PACL *ppDacl,
                      PACL *ppSacl,
                      PSECURITY_DESCRIPTOR *ppSecurityDescriptor)
{
    UNICODE_STRING ObjectName;
    NTSTATUS Status;
    DWORD Ret;

    Status = RtlCreateUnicodeStringFromAsciiz(&ObjectName,
                                              pObjectName);
    if (!NT_SUCCESS(Status))
    {
        return RtlNtStatusToDosError(Status);
    }

    Ret = GetNamedSecurityInfoW(ObjectName.Buffer,
                                ObjectType,
                                SecurityInfo,
                                ppsidOwner,
                                ppsidGroup,
                                ppDacl,
                                ppSacl,
                                ppSecurityDescriptor);

    RtlFreeUnicodeString(&ObjectName);

    return Ret;
}


/**********************************************************************
 * SetNamedSecurityInfoW			EXPORTED
 *
 * @implemented
 */
DWORD STDCALL
SetNamedSecurityInfoW(LPWSTR pObjectName,
                      SE_OBJECT_TYPE ObjectType,
                      SECURITY_INFORMATION SecurityInfo,
                      PSID psidOwner,
                      PSID psidGroup,
                      PACL pDacl,
                      PACL pSacl)
{
    DWORD ErrorCode;

    if (pObjectName != NULL)
    {
        ErrorCode = CheckNtMartaPresent();
        if (ErrorCode == ERROR_SUCCESS)
        {
            SECURITY_DESCRIPTOR SecurityDescriptor;

            ErrorCode = pSetSecurityInfoCheck(&SecurityDescriptor,
                                              SecurityInfo,
                                              psidOwner,
                                              psidGroup,
                                              pDacl,
                                              pSacl);

            if (ErrorCode == ERROR_SUCCESS)
            {
                /* call the MARTA provider */
                ErrorCode = AccRewriteSetNamedRights(pObjectName,
                                                     ObjectType,
                                                     SecurityInfo,
                                                     &SecurityDescriptor);
            }
        }
    }
    else
        ErrorCode = ERROR_INVALID_PARAMETER;

    return ErrorCode;
}


/**********************************************************************
 * SetNamedSecurityInfoA			EXPORTED
 *
 * @implemented
 */
DWORD STDCALL
SetNamedSecurityInfoA(LPSTR pObjectName,
                      SE_OBJECT_TYPE ObjectType,
                      SECURITY_INFORMATION SecurityInfo,
                      PSID psidOwner,
                      PSID psidGroup,
                      PACL pDacl,
                      PACL pSacl)
{
    UNICODE_STRING ObjectName;
    NTSTATUS Status;
    DWORD Ret;

    Status = RtlCreateUnicodeStringFromAsciiz(&ObjectName,
                                              pObjectName);
    if (!NT_SUCCESS(Status))
    {
        return RtlNtStatusToDosError(Status);
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


/**********************************************************************
 * GetSecurityInfo				EXPORTED
 *
 * @implemented
 */
DWORD STDCALL
GetSecurityInfo(HANDLE handle,
                SE_OBJECT_TYPE ObjectType,
                SECURITY_INFORMATION SecurityInfo,
                PSID* ppsidOwner,
                PSID* ppsidGroup,
                PACL* ppDacl,
                PACL* ppSacl,
                PSECURITY_DESCRIPTOR* ppSecurityDescriptor)
{
    DWORD ErrorCode;

    if (handle != NULL)
    {
        ErrorCode = CheckNtMartaPresent();
        if (ErrorCode == ERROR_SUCCESS)
        {
            ErrorCode = pGetSecurityInfoCheck(SecurityInfo,
                                              ppsidOwner,
                                              ppsidGroup,
                                              ppDacl,
                                              ppSacl,
                                              ppSecurityDescriptor);

            if (ErrorCode == ERROR_SUCCESS)
            {
                /* call the MARTA provider */
                ErrorCode = AccRewriteGetHandleRights(handle,
                                                      ObjectType,
                                                      SecurityInfo,
                                                      ppsidOwner,
                                                      ppsidGroup,
                                                      ppDacl,
                                                      ppSacl,
                                                      ppSecurityDescriptor);
            }
        }
    }
    else
        ErrorCode = ERROR_INVALID_HANDLE;

    return ErrorCode;
}


/**********************************************************************
 * SetSecurityInfo				EXPORTED
 *
 * @implemented
 */
DWORD
WINAPI
SetSecurityInfo(HANDLE handle,
                SE_OBJECT_TYPE ObjectType,
                SECURITY_INFORMATION SecurityInfo,
                PSID psidOwner,
                PSID psidGroup,
                PACL pDacl,
                PACL pSacl)
{
    DWORD ErrorCode;

    if (handle != NULL)
    {
        ErrorCode = CheckNtMartaPresent();
        if (ErrorCode == ERROR_SUCCESS)
        {
            SECURITY_DESCRIPTOR SecurityDescriptor;

            ErrorCode = pSetSecurityInfoCheck(&SecurityDescriptor,
                                              SecurityInfo,
                                              psidOwner,
                                              psidGroup,
                                              pDacl,
                                              pSacl);

            if (ErrorCode == ERROR_SUCCESS)
            {
                /* call the MARTA provider */
                ErrorCode = AccRewriteSetHandleRights(handle,
                                                      ObjectType,
                                                      SecurityInfo,
                                                      &SecurityDescriptor);
            }
        }
    }
    else
        ErrorCode = ERROR_INVALID_HANDLE;

    return ErrorCode;
}


/******************************************************************************
 * GetSecurityInfoExW         EXPORTED
 */
DWORD WINAPI GetSecurityInfoExA(
   HANDLE hObject,
   SE_OBJECT_TYPE ObjectType,
   SECURITY_INFORMATION SecurityInfo,
   LPCSTR lpProvider,
   LPCSTR lpProperty,
   PACTRL_ACCESSA *ppAccessList,
   PACTRL_AUDITA *ppAuditList,
   LPSTR *lppOwner,
   LPSTR *lppGroup
   )
{
  DPRINT1("GetSecurityInfoExA stub!\n");
  return ERROR_BAD_PROVIDER;
}


/******************************************************************************
 * GetSecurityInfoExW         EXPORTED
 */
DWORD WINAPI GetSecurityInfoExW(
   HANDLE hObject,
   SE_OBJECT_TYPE ObjectType,
   SECURITY_INFORMATION SecurityInfo,
   LPCWSTR lpProvider,
   LPCWSTR lpProperty,
   PACTRL_ACCESSW *ppAccessList,
   PACTRL_AUDITW *ppAuditList,
   LPWSTR *lppOwner,
   LPWSTR *lppGroup
   )
{
  DPRINT1("GetSecurityInfoExW stub!\n");
  return ERROR_BAD_PROVIDER;
}


/**********************************************************************
 * ImpersonateNamedPipeClient			EXPORTED
 *
 * @implemented
 */
BOOL STDCALL
ImpersonateNamedPipeClient(HANDLE hNamedPipe)
{
  IO_STATUS_BLOCK StatusBlock;
  NTSTATUS Status;

  DPRINT("ImpersonateNamedPipeClient() called\n");

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
BOOL STDCALL
CreatePrivateObjectSecurity(PSECURITY_DESCRIPTOR ParentDescriptor,
                            PSECURITY_DESCRIPTOR CreatorDescriptor,
                            PSECURITY_DESCRIPTOR *NewDescriptor,
                            BOOL IsDirectoryObject,
                            HANDLE Token,
                            PGENERIC_MAPPING GenericMapping)
{
    NTSTATUS Status;

    Status = RtlNewSecurityObject(ParentDescriptor,
                                  CreatorDescriptor,
                                  NewDescriptor,
                                  IsDirectoryObject,
                                  Token,
                                  GenericMapping);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}


/*
 * @unimplemented
 */
BOOL STDCALL
CreatePrivateObjectSecurityEx(PSECURITY_DESCRIPTOR ParentDescriptor,
                              PSECURITY_DESCRIPTOR CreatorDescriptor,
                              PSECURITY_DESCRIPTOR* NewDescriptor,
                              GUID* ObjectType,
                              BOOL IsContainerObject,
                              ULONG AutoInheritFlags,
                              HANDLE Token,
                              PGENERIC_MAPPING GenericMapping)
{
    DPRINT1("%s() not implemented!\n", __FUNCTION__);
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL STDCALL
CreatePrivateObjectSecurityWithMultipleInheritance(PSECURITY_DESCRIPTOR ParentDescriptor,
                                                   PSECURITY_DESCRIPTOR CreatorDescriptor,
                                                   PSECURITY_DESCRIPTOR* NewDescriptor,
                                                   GUID** ObjectTypes,
                                                   ULONG GuidCount,
                                                   BOOL IsContainerObject,
                                                   ULONG AutoInheritFlags,
                                                   HANDLE Token,
                                                   PGENERIC_MAPPING GenericMapping)
{
    DPRINT1("%s() not implemented!\n", __FUNCTION__);
    return FALSE;
}


/*
 * @implemented
 */
BOOL STDCALL
DestroyPrivateObjectSecurity(PSECURITY_DESCRIPTOR *ObjectDescriptor)
{
    NTSTATUS Status;

    Status = RtlDeleteSecurityObject(ObjectDescriptor);
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
BOOL STDCALL
GetPrivateObjectSecurity(PSECURITY_DESCRIPTOR ObjectDescriptor,
                         SECURITY_INFORMATION SecurityInformation,
                         PSECURITY_DESCRIPTOR ResultantDescriptor,
                         DWORD DescriptorLength,
                         PDWORD ReturnLength)
{
    NTSTATUS Status;

    Status = RtlQuerySecurityObject(ObjectDescriptor,
                                    SecurityInformation,
                                    ResultantDescriptor,
                                    DescriptorLength,
                                    ReturnLength);
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
BOOL STDCALL
SetPrivateObjectSecurity(SECURITY_INFORMATION SecurityInformation,
                         PSECURITY_DESCRIPTOR ModificationDescriptor,
                         PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor,
                         PGENERIC_MAPPING GenericMapping,
                         HANDLE Token)
{
    NTSTATUS Status;

    Status = RtlSetSecurityObject(SecurityInformation,
                                  ModificationDescriptor,
                                  ObjectsSecurityDescriptor,
                                  GenericMapping,
                                  Token);
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
DWORD STDCALL
TreeResetNamedSecurityInfoW(LPWSTR pObjectName,
                            SE_OBJECT_TYPE ObjectType,
                            SECURITY_INFORMATION SecurityInfo,
                            PSID pOwner,
                            PSID pGroup,
                            PACL pDacl,
                            PACL pSacl,
                            BOOL KeepExplicit,
                            FN_PROGRESSW fnProgress,
                            PROG_INVOKE_SETTING ProgressInvokeSetting,
                            PVOID Args)
{
    DWORD ErrorCode;

    if (pObjectName != NULL)
    {
        ErrorCode = CheckNtMartaPresent();
        if (ErrorCode == ERROR_SUCCESS)
        {
            switch (ObjectType)
            {
                case SE_FILE_OBJECT:
                case SE_REGISTRY_KEY:
                {
                    /* check the SecurityInfo flags for sanity (both, the protected
                       and unprotected dacl/sacl flag must not be passed together) */
                    if (((SecurityInfo & DACL_SECURITY_INFORMATION) &&
                         (SecurityInfo & (PROTECTED_DACL_SECURITY_INFORMATION | UNPROTECTED_DACL_SECURITY_INFORMATION)) ==
                             (PROTECTED_DACL_SECURITY_INFORMATION | UNPROTECTED_DACL_SECURITY_INFORMATION))

                        ||

                        ((SecurityInfo & SACL_SECURITY_INFORMATION) &&
                         (SecurityInfo & (PROTECTED_SACL_SECURITY_INFORMATION | UNPROTECTED_SACL_SECURITY_INFORMATION)) ==
                             (PROTECTED_SACL_SECURITY_INFORMATION | UNPROTECTED_SACL_SECURITY_INFORMATION)))
                    {
                        ErrorCode = ERROR_INVALID_PARAMETER;
                        break;
                    }

                    /* call the MARTA provider */
                    ErrorCode = AccTreeResetNamedSecurityInfo(pObjectName,
                                                              ObjectType,
                                                              SecurityInfo,
                                                              pOwner,
                                                              pGroup,
                                                              pDacl,
                                                              pSacl,
                                                              KeepExplicit,
                                                              fnProgress,
                                                              ProgressInvokeSetting,
                                                              Args);
                    break;
                }

                default:
                    /* object type not supported */
                    ErrorCode = ERROR_INVALID_PARAMETER;
                    break;
            }
        }
    }
    else
        ErrorCode = ERROR_INVALID_PARAMETER;

    return ErrorCode;
}

#ifdef HAS_FN_PROGRESSW

typedef struct _INERNAL_FNPROGRESSW_DATA
{
    FN_PROGRESSA fnProgress;
    PVOID Args;
} INERNAL_FNPROGRESSW_DATA, *PINERNAL_FNPROGRESSW_DATA;

static VOID STDCALL
InternalfnProgressW(LPWSTR pObjectName,
                    DWORD Status,
                    PPROG_INVOKE_SETTING pInvokeSetting,
                    PVOID Args,
                    BOOL SecuritySet)
{
    PINERNAL_FNPROGRESSW_DATA pifnProgressData = (PINERNAL_FNPROGRESSW_DATA)Args;
    INT ObjectNameSize;
    LPSTR pObjectNameA;

    ObjectNameSize = WideCharToMultiByte(CP_ACP,
                                         0,
                                         pObjectName,
                                         -1,
                                         NULL,
                                         0,
                                         NULL,
                                         NULL);

    if (ObjectNameSize > 0)
    {
        pObjectNameA = RtlAllocateHeap(RtlGetProcessHeap(),
                                       0,
                                       ObjectNameSize);
        if (pObjectNameA != NULL)
        {
            pObjectNameA[0] = '\0';
            WideCharToMultiByte(CP_ACP,
                                0,
                                pObjectName,
                                -1,
                                pObjectNameA,
                                ObjectNameSize,
                                NULL,
                                NULL);

            pifnProgressData->fnProgress((LPWSTR)pObjectNameA, /* FIXME: wrong cast!! */
                                         Status,
                                         pInvokeSetting,
                                         pifnProgressData->Args,
                                         SecuritySet);

            RtlFreeHeap(RtlGetProcessHeap(),
                        0,
                        pObjectNameA);
        }
    }
}
#endif


/*
 * @implemented
 */
DWORD STDCALL
TreeResetNamedSecurityInfoA(LPSTR pObjectName,
                            SE_OBJECT_TYPE ObjectType,
                            SECURITY_INFORMATION SecurityInfo,
                            PSID pOwner,
                            PSID pGroup,
                            PACL pDacl,
                            PACL pSacl,
                            BOOL KeepExplicit,
                            FN_PROGRESSA fnProgress,
                            PROG_INVOKE_SETTING ProgressInvokeSetting,
                            PVOID Args)
{
#ifndef HAS_FN_PROGRESSW
    /* That's all this function does, at least up to w2k3... Even MS was too
       lazy to implement it... */
    return ERROR_CALL_NOT_IMPLEMENTED;
#else
    INERNAL_FNPROGRESSW_DATA ifnProgressData;
    UNICODE_STRING ObjectName;
    NTSTATUS Status;
    DWORD Ret;

    Status = RtlCreateUnicodeStringFromAsciiz(&ObjectName,
                                              pObjectName);
    if (!NT_SUCCESS(Status))
    {
        return RtlNtStatusToDosError(Status);
    }

    ifnProgressData.fnProgress = fnProgress;
    ifnProgressData.Args = Args;

    Ret = TreeResetNamedSecurityInfoW(ObjectName.Buffer,
                                      ObjectType,
                                      SecurityInfo,
                                      pOwner,
                                      pGroup,
                                      pDacl,
                                      pSacl,
                                      KeepExplicit,
                                      (fnProgress != NULL ? InternalfnProgressW : NULL),
                                      ProgressInvokeSetting,
                                      &ifnProgressData);

    RtlFreeUnicodeString(&ObjectName);

    return Ret;
#endif
}

/* EOF */
