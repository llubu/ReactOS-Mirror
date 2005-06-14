#ifndef _INCLUDE_DDK_OBFUNCS_H
#define _INCLUDE_DDK_OBFUNCS_H
/* OBJECT MANAGER ************************************************************/

NTSTATUS STDCALL
ObAssignSecurity(IN PACCESS_STATE AccessState,
		 IN PSECURITY_DESCRIPTOR SecurityDescriptor,
		 IN PVOID Object,
		 IN POBJECT_TYPE Type);

NTSTATUS STDCALL
ObCreateObject (IN KPROCESSOR_MODE ObjectAttributesAccessMode OPTIONAL,
		IN POBJECT_TYPE ObjectType,
		IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
		IN KPROCESSOR_MODE AccessMode,
		IN OUT PVOID ParseContext OPTIONAL,
		IN ULONG ObjectSize,
		IN ULONG PagedPoolCharge OPTIONAL,
		IN ULONG NonPagedPoolCharge OPTIONAL,
		OUT PVOID *Object);

VOID FASTCALL
ObfDereferenceObject(IN PVOID Object);

VOID FASTCALL
ObfReferenceObject(IN PVOID Object);

#define ObDereferenceObject(Object) \
  ObfDereferenceObject(Object)

#define ObReferenceObject(Object) \
  ObfReferenceObject(Object)

ULONG STDCALL
ObGetObjectPointerCount(IN PVOID Object);

NTSTATUS STDCALL
ObGetObjectSecurity(IN PVOID Object,
		    OUT PSECURITY_DESCRIPTOR *SecurityDescriptor,
		    OUT PBOOLEAN MemoryAllocated);

NTSTATUS STDCALL
ObInsertObject(IN PVOID Object,
	       IN PACCESS_STATE PassedAccessState OPTIONAL,
	       IN ACCESS_MASK DesiredAccess,
	       IN ULONG AdditionalReferences,
	       OUT PVOID* ReferencedObject OPTIONAL,
	       OUT PHANDLE Handle);

VOID STDCALL
ObMakeTemporaryObject(IN PVOID ObjectBody);

NTSTATUS STDCALL
ObOpenObjectByName(IN POBJECT_ATTRIBUTES ObjectAttributes,
		   IN POBJECT_TYPE ObjectType,
		   IN OUT PVOID ParseContext OPTIONAL,
		   IN KPROCESSOR_MODE AccessMode,
		   IN ACCESS_MASK DesiredAccess,
		   IN PACCESS_STATE PassedAccessState,
		   OUT PHANDLE Handle);

NTSTATUS STDCALL
ObOpenObjectByPointer(IN PVOID Object,
		      IN ULONG HandleAttributes,
		      IN PACCESS_STATE PassedAccessState OPTIONAL,
		      IN ACCESS_MASK DesiredAccess OPTIONAL,
		      IN POBJECT_TYPE ObjectType OPTIONAL,
		      IN KPROCESSOR_MODE AccessMode,
		      OUT PHANDLE Handle);

NTSTATUS STDCALL
ObQueryNameString(IN PVOID Object,
		  OUT POBJECT_NAME_INFORMATION ObjectNameInfo,
		  IN ULONG Length,
		  OUT PULONG ReturnLength);

NTSTATUS STDCALL
ObQueryObjectAuditingByHandle(IN HANDLE Handle,
			      OUT PBOOLEAN GenerateOnClose);

/*
 * FUNCTION: Performs access validation on an object handle and if access
 * is granted returns a pointer to the object's body
 * ARGUMENTS:
 *         Handle = Handle to the object 
 *         DesiredAccess = Desired access to the object
 *         ObjectType (OPTIONAL) = Pointer to the object's type definition
 *         AccessMode = Access mode to use for the check
 *         Object (OUT) = Caller supplied storage for a pointer to the object's
 *                        body
 *         HandleInformation (OUT) = Points to a structure which receives
 *                                   information about the handle
 * RETURNS: Status
 */
NTSTATUS STDCALL
ObReferenceObjectByHandle(IN HANDLE Handle,
			  IN ACCESS_MASK DesiredAccess,
			  IN POBJECT_TYPE ObjectType OPTIONAL,
			  IN KPROCESSOR_MODE AccessMode,
			  OUT PVOID* Object,
			  OUT POBJECT_HANDLE_INFORMATION HandleInfo OPTIONAL);

/*
 * FUNCTION: Increments the reference count for a given object
 * ARGUMENTS:
 *      Object = Points to the body of the object
 *      AccessMode = Requested access to the object
 *      ObjectType = Pointer to the object's type definition
 *      AccessMode = Access mode to use for the security check
 * RETURNS: Status
 */
NTSTATUS STDCALL
ObReferenceObjectByPointer(IN PVOID Object,
			   IN ACCESS_MASK DesiredAccess,
			   IN POBJECT_TYPE ObjectType,
			   IN KPROCESSOR_MODE AccessMode);

NTSTATUS STDCALL
ObReferenceObjectByName(IN PUNICODE_STRING ObjectPath,
			IN ULONG Attributes,
			IN PACCESS_STATE PassedAccessState OPTIONAL,
			IN ACCESS_MASK DesiredAccess OPTIONAL,
			IN POBJECT_TYPE ObjectType,
			IN KPROCESSOR_MODE AccessMode,
			IN OUT PVOID ParseContext OPTIONAL,
			OUT PVOID* ObjectPtr);

VOID STDCALL
ObReleaseObjectSecurity(IN PSECURITY_DESCRIPTOR SecurityDescriptor,
			IN BOOLEAN MemoryAllocated);

/*
NTSTATUS STDCALL
ObSetSecurityDescriptorInfo(IN PVOID Object,
			    IN PSECURITY_INFORMATION SecurityInformation,
			    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
			    ULONG Param4,
			    IN POOL_TYPE PoolType,
			    IN PGENERIC_MAPPING GenericMapping);
*/

NTSTATUS STDCALL
ObFindHandleForObject(IN PEPROCESS Process,
                      IN PVOID Object,
                      IN POBJECT_TYPE ObjectType,
                      IN POBJECT_HANDLE_INFORMATION HandleInformation,
                      OUT PHANDLE HandleReturn);

#endif /* ndef _INCLUDE_DDK_OBFUNCS_H */
