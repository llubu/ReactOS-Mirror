#ifndef _INCLUDE_DDK_SEFUNCS_H
#define _INCLUDE_DDK_SEFUNCS_H
/* $Id: sefuncs.h,v 1.7 2000/04/05 01:38:05 ekohl Exp $ */
NTSTATUS STDCALL RtlCreateAcl(PACL Acl, ULONG AclSize, ULONG AclRevision);
NTSTATUS STDCALL RtlQueryInformationAcl (PACL Acl, PVOID Information, ULONG InformationLength, ACL_INFORMATION_CLASS InformationClass);
NTSTATUS STDCALL RtlSetInformationAcl (PACL Acl, PVOID Information, ULONG InformationLength, ACL_INFORMATION_CLASS InformationClass);
BOOLEAN STDCALL RtlValidAcl (PACL Acl);
NTSTATUS STDCALL RtlAddAccessAllowedAce(PACL Acl, ULONG Revision, ACCESS_MASK AccessMask, PSID Sid);
NTSTATUS STDCALL RtlAddAccessDeniedAce(PACL Acl, ULONG Revision, ACCESS_MASK AccessMask, PSID Sid);
NTSTATUS STDCALL RtlAddAce(PACL Acl, ULONG Revision, ULONG StartingIndex, PACE AceList, ULONG AceListLength);
NTSTATUS STDCALL RtlAddAuditAccessAce (PACL Acl, ULONG Revision, ACCESS_MASK AccessMask, PSID Sid, BOOLEAN Success, BOOLEAN Failure);
NTSTATUS STDCALL RtlDeleteAce(PACL Acl, ULONG AceIndex);
BOOLEAN STDCALL RtlFirstFreeAce(PACL Acl, PACE* Ace);
NTSTATUS STDCALL RtlGetAce(PACL Acl, ULONG AceIndex, PACE *Ace);
NTSTATUS STDCALL RtlCreateSecurityDescriptor (PSECURITY_DESCRIPTOR SecurityDescriptor, ULONG Revision);
BOOLEAN STDCALL RtlValidSecurityDescriptor (PSECURITY_DESCRIPTOR SecurityDescriptor);
ULONG STDCALL RtlLengthSecurityDescriptor (PSECURITY_DESCRIPTOR SecurityDescriptor);
NTSTATUS STDCALL RtlSetDaclSecurityDescriptor (PSECURITY_DESCRIPTOR SecurityDescriptor, BOOLEAN DaclPresent, PACL Dacl, BOOLEAN DaclDefaulted);
NTSTATUS STDCALL RtlGetDaclSecurityDescriptor (PSECURITY_DESCRIPTOR SecurityDescriptor, PBOOLEAN DaclPresent, PACL* Dacl, PBOOLEAN DaclDefauted);
NTSTATUS STDCALL RtlSetOwnerSecurityDescriptor (PSECURITY_DESCRIPTOR SecurityDescriptor, PSID Owner, BOOLEAN OwnerDefaulted);
NTSTATUS STDCALL RtlGetOwnerSecurityDescriptor (PSECURITY_DESCRIPTOR SecurityDescriptor, PSID* Owner, PBOOLEAN OwnerDefaulted);
NTSTATUS STDCALL RtlSetGroupSecurityDescriptor (PSECURITY_DESCRIPTOR SecurityDescriptor, PSID Group, BOOLEAN GroupDefaulted);
NTSTATUS STDCALL RtlGetGroupSecurityDescriptor (PSECURITY_DESCRIPTOR SecurityDescriptor, PSID* Group, PBOOLEAN GroupDefaulted);
NTSTATUS STDCALL RtlAllocateAndInitializeSid (PSID_IDENTIFIER_AUTHORITY IdentifierAuthority,
					      UCHAR SubAuthorityCount,
					      ULONG SubAuthority0,
					      ULONG SubAuthority1,
					      ULONG SubAuthority2,
					      ULONG SubAuthority3,
					      ULONG SubAuthority4,
					      ULONG SubAuthority5,
					      ULONG SubAuthority6,
					      ULONG SubAuthority7,
					      PSID *Sid);
ULONG STDCALL RtlLengthRequiredSid (UCHAR SubAuthorityCount);
PSID_IDENTIFIER_AUTHORITY STDCALL RtlIdentifierAuthoritySid (PSID Sid);
NTSTATUS STDCALL RtlInitializeSid (PSID Sid, PSID_IDENTIFIER_AUTHORITY IdentifierAuthority, UCHAR SubAuthorityCount);
PULONG STDCALL RtlSubAuthoritySid (PSID Sid, ULONG SubAuthority);
NTSTATUS STDCALL RtlCopySid (ULONG BufferLength, PSID Dest, PSID Src);
BOOLEAN STDCALL RtlEqualPrefixSid (PSID Sid1, PSID Sid2);
BOOLEAN STDCALL RtlEqualSid(PSID Sid1, PSID Sid2);
PSID STDCALL RtlFreeSid (PSID Sid);
ULONG STDCALL RtlLengthSid (PSID Sid);
PULONG STDCALL RtlSubAuthoritySid (PSID Sid, ULONG SubAuthority);
PUCHAR STDCALL RtlSubAuthorityCountSid (PSID Sid);
BOOLEAN STDCALL RtlValidSid (PSID Sid);
NTSTATUS STDCALL RtlAbsoluteToSelfRelativeSD (PSECURITY_DESCRIPTOR AbsSD, PSECURITY_DESCRIPTOR RelSD, PULONG BufferLength);
BOOLEAN STDCALL SeAccessCheck (IN PSECURITY_DESCRIPTOR SecurityDescriptor,
		      IN PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext,
		      IN BOOLEAN SubjectContextLocked,
		      IN ACCESS_MASK DesiredAccess,
		      IN ACCESS_MASK PreviouslyGrantedAccess,
		      OUT PPRIVILEGE_SET* Privileges,
		      IN PGENERIC_MAPPING GenericMapping,
		      IN KPROCESSOR_MODE AccessMode,
		      OUT PACCESS_MODE GrantedAccess,
		      OUT PNTSTATUS AccessStatus);
NTSTATUS STDCALL SeAssignSecurity (PSECURITY_DESCRIPTOR ParentDescriptor,
				   PSECURITY_DESCRIPTOR ExplicitDescriptor,
				   PSECURITY_DESCRIPTOR* NewDescriptor,
				   BOOLEAN IsDirectoryObject,
				   PSECURITY_SUBJECT_CONTEXT SubjectContext,
				   PGENERIC_MAPPING GenericMapping,
				   POOL_TYPE PoolType);
NTSTATUS STDCALL SeDeassignSecurity (PSECURITY_DESCRIPTOR* SecurityDescriptor);
BOOLEAN STDCALL SeSinglePrivilegeCheck (LUID PrivilegeValue, KPROCESSOR_MODE PreviousMode);
ULONG STDCALL RtlLengthSid (PSID Sid);
NTSTATUS STDCALL RtlCopySid(ULONG BufferLength, PSID Src, PSID Dest);

VOID SeImpersonateClient(PSE_SOME_STRUCT2 a,
			 PETHREAD Thread);

NTSTATUS SeCreateClientSecurity(PETHREAD Thread,
				PSECURITY_QUALITY_OF_SERVICE Qos,
				ULONG e,
				PSE_SOME_STRUCT2 f);
NTSTATUS SeExchangePrimaryToken(PEPROCESS Process,
				PACCESS_TOKEN NewToken,
				PACCESS_TOKEN* OldTokenP);
VOID STDCALL SeReleaseSubjectContext (PSECURITY_SUBJECT_CONTEXT SubjectContext);
VOID STDCALL SeCaptureSubjectContext (PSECURITY_SUBJECT_CONTEXT SubjectContext);
NTSTATUS SeCaptureLuidAndAttributesArray(PLUID_AND_ATTRIBUTES Src,
					 ULONG PrivilegeCount,
					 KPROCESSOR_MODE PreviousMode,
					 PLUID_AND_ATTRIBUTES AllocatedMem,
					 ULONG AllocatedLength,
					 POOL_TYPE PoolType,
					 ULONG d,
					 PLUID_AND_ATTRIBUTES* Dest,
					 PULONG Length);


#endif /* ndef _INCLUDE_DDK_SEFUNCS_H */
