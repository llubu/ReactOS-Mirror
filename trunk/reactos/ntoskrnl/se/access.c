/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/se/access.c
 * PURPOSE:         Access state functions
 *
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net) - 
 *                               Based on patch by Javier M. Mellid
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

#define GENERIC_ACCESS (GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | \
                        GENERIC_ALL)

/* FUNCTIONS ***************************************************************/

/*
 * @implemented
 */
NTSTATUS
STDCALL
SeCreateAccessState(PACCESS_STATE AccessState,
                    PAUX_DATA AuxData,
                    ACCESS_MASK Access,
                    PGENERIC_MAPPING GenericMapping)
{
    ACCESS_MASK AccessMask = Access;
    PTOKEN Token;
    
    PAGED_CODE();

    /* Map the Generic Acess to Specific Access if we have a Mapping */
    if ((Access & GENERIC_ACCESS) && (GenericMapping))
    {
        RtlMapGenericMask(&AccessMask, GenericMapping);
    }

    /* Initialize the Access State */
    RtlZeroMemory(AccessState, sizeof(ACCESS_STATE));

    /* Capture the Subject Context */
    SeCaptureSubjectContext(&AccessState->SubjectSecurityContext);
    
    /* Set Access State Data */
    AccessState->AuxData = AuxData;
    AccessState->RemainingDesiredAccess  = AccessMask;
    AccessState->OriginallyDesiredAccess = AccessMask;
    ExpAllocateLocallyUniqueId(&AccessState->OperationID);

    /* Get the Token to use */
    Token =  AccessState->SubjectSecurityContext.ClientToken ?
             (PTOKEN)&AccessState->SubjectSecurityContext.ClientToken :
             (PTOKEN)&AccessState->SubjectSecurityContext.PrimaryToken;
             
    /* Check for Travers Privilege */
    if (Token->TokenFlags & TOKEN_HAS_TRAVERSE_PRIVILEGE)
    {
        /* Preserve the Traverse Privilege */
        AccessState->Flags = TOKEN_HAS_TRAVERSE_PRIVILEGE;
    }

    /* Set the Auxiliary Data */
    AuxData->PrivilegeSet = (PPRIVILEGE_SET)((ULONG_PTR)AccessState +
                                             FIELD_OFFSET(ACCESS_STATE,
                                                          Privileges));    
    if (GenericMapping) AuxData->GenericMapping = *GenericMapping;

    /* Return Sucess */
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
VOID
STDCALL
SeDeleteAccessState(IN PACCESS_STATE AccessState)
{
    PAUX_DATA AuxData;
    PAGED_CODE();

    /* Get the Auxiliary Data */
    AuxData = AccessState->AuxData;

    /* Deallocate Privileges */
    if (AccessState->PrivilegesAllocated) ExFreePool(AuxData->PrivilegeSet);
    
    /* Deallocate Name and Type Name */
    if (AccessState->ObjectName.Buffer)
    {
        ExFreePool(AccessState->ObjectName.Buffer);
    }
    if (AccessState->ObjectTypeName.Buffer) 
    {
        ExFreePool(AccessState->ObjectTypeName.Buffer);
    }

    /* Release the Subject Context */
    SeReleaseSubjectContext(&AccessState->SubjectSecurityContext);
}

/*
 * @implemented
 */
VOID
STDCALL
SeSetAccessStateGenericMapping(PACCESS_STATE AccessState,
                               PGENERIC_MAPPING GenericMapping)
{
    PAGED_CODE();

    /* Set the Generic Mapping */
    ((PAUX_DATA)AccessState->AuxData)->GenericMapping = *GenericMapping;
}

/* EOF */
