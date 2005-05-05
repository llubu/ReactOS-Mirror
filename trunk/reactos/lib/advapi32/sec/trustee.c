/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/advapi32/sec/trustee.c
 * PURPOSE:         Trustee functions
 */

#include "advapi32.h"

#define NDEBUG
#include "debug.h"


/******************************************************************************
 * BuildImpersonateTrusteeA [ADVAPI32.@]
 */
VOID WINAPI
BuildImpersonateTrusteeA(PTRUSTEE_A pTrustee,
                         PTRUSTEE_A pImpersonateTrustee)
{
  pTrustee->pMultipleTrustee = pImpersonateTrustee;
  pTrustee->MultipleTrusteeOperation = TRUSTEE_IS_IMPERSONATE;
}


/******************************************************************************
 * BuildImpersonateTrusteeW [ADVAPI32.@]
 */
VOID WINAPI
BuildImpersonateTrusteeW(PTRUSTEE_W pTrustee,
                         PTRUSTEE_W pImpersonateTrustee)
{
  pTrustee->pMultipleTrustee = pImpersonateTrustee;
  pTrustee->MultipleTrusteeOperation = TRUSTEE_IS_IMPERSONATE;
}


/******************************************************************************
 * BuildExplicitAccessWithNameA [ADVAPI32.@]
 */
VOID WINAPI
BuildExplicitAccessWithNameA(PEXPLICIT_ACCESSA pExplicitAccess,
                             LPSTR pTrusteeName,
                             DWORD AccessPermissions,
                             ACCESS_MODE AccessMode,
                             DWORD Inheritance)
{
    pExplicitAccess->grfAccessPermissions = AccessPermissions;
    pExplicitAccess->grfAccessMode = AccessMode;
    pExplicitAccess->grfInheritance = Inheritance;

    pExplicitAccess->Trustee.pMultipleTrustee = NULL;
    pExplicitAccess->Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    pExplicitAccess->Trustee.TrusteeForm = TRUSTEE_IS_NAME;
    pExplicitAccess->Trustee.TrusteeType = TRUSTEE_IS_UNKNOWN;
    pExplicitAccess->Trustee.ptstrName = pTrusteeName;
}


/******************************************************************************
 * BuildExplicitAccessWithNameW [ADVAPI32.@]
 */
VOID WINAPI
BuildExplicitAccessWithNameW(PEXPLICIT_ACCESSW pExplicitAccess,
                             LPWSTR pTrusteeName,
                             DWORD AccessPermissions,
                             ACCESS_MODE AccessMode,
                             DWORD Inheritance)
{
    pExplicitAccess->grfAccessPermissions = AccessPermissions;
    pExplicitAccess->grfAccessMode = AccessMode;
    pExplicitAccess->grfInheritance = Inheritance;

    pExplicitAccess->Trustee.pMultipleTrustee = NULL;
    pExplicitAccess->Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    pExplicitAccess->Trustee.TrusteeForm = TRUSTEE_IS_NAME;
    pExplicitAccess->Trustee.TrusteeType = TRUSTEE_IS_UNKNOWN;
    pExplicitAccess->Trustee.ptstrName = pTrusteeName;
}


/******************************************************************************
 * BuildImpersonateExplicitAccessWithNameA [ADVAPI32.@]
 */
VOID WINAPI
BuildImpersonateExplicitAccessWithNameA(PEXPLICIT_ACCESS_A pExplicitAccess,
                                        LPSTR pTrusteeName,
                                        PTRUSTEE_A pTrustee,
                                        DWORD AccessPermissions,
                                        ACCESS_MODE AccessMode,
                                        DWORD Inheritance)
{
    pExplicitAccess->grfAccessPermissions = AccessPermissions;
    pExplicitAccess->grfAccessMode = AccessMode;
    pExplicitAccess->grfInheritance = Inheritance;

    pExplicitAccess->Trustee.pMultipleTrustee = pTrustee;
    pExplicitAccess->Trustee.MultipleTrusteeOperation = TRUSTEE_IS_IMPERSONATE;
    pExplicitAccess->Trustee.TrusteeForm = TRUSTEE_IS_NAME;
    pExplicitAccess->Trustee.TrusteeType = TRUSTEE_IS_UNKNOWN;
    pExplicitAccess->Trustee.ptstrName = pTrusteeName;
}


/******************************************************************************
 * BuildImpersonateExplicitAccessWithNameW [ADVAPI32.@]
 */
VOID WINAPI
BuildImpersonateExplicitAccessWithNameW(PEXPLICIT_ACCESS_W pExplicitAccess,
                                        LPWSTR pTrusteeName,
                                        PTRUSTEE_W pTrustee,
                                        DWORD AccessPermissions,
                                        ACCESS_MODE AccessMode,
                                        DWORD Inheritance)
{
    pExplicitAccess->grfAccessPermissions = AccessPermissions;
    pExplicitAccess->grfAccessMode = AccessMode;
    pExplicitAccess->grfInheritance = Inheritance;

    pExplicitAccess->Trustee.pMultipleTrustee = pTrustee;
    pExplicitAccess->Trustee.MultipleTrusteeOperation = TRUSTEE_IS_IMPERSONATE;
    pExplicitAccess->Trustee.TrusteeForm = TRUSTEE_IS_NAME;
    pExplicitAccess->Trustee.TrusteeType = TRUSTEE_IS_UNKNOWN;
    pExplicitAccess->Trustee.ptstrName = pTrusteeName;
}


/******************************************************************************
 * BuildTrusteeWithSidA [ADVAPI32.@]
 */
VOID WINAPI
BuildTrusteeWithSidA(PTRUSTEE_A pTrustee, PSID pSid)
{
    DPRINT("%p %p\n", pTrustee, pSid);

    pTrustee->pMultipleTrustee = NULL;
    pTrustee->MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    pTrustee->TrusteeForm = TRUSTEE_IS_SID;
    pTrustee->TrusteeType = TRUSTEE_IS_UNKNOWN;
    pTrustee->ptstrName = (LPSTR) pSid;
}


/******************************************************************************
 * BuildTrusteeWithSidW [ADVAPI32.@]
 */
VOID WINAPI
BuildTrusteeWithSidW(PTRUSTEE_W pTrustee, PSID pSid)
{
    DPRINT("%p %p\n", pTrustee, pSid);

    pTrustee->pMultipleTrustee = NULL;
    pTrustee->MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    pTrustee->TrusteeForm = TRUSTEE_IS_SID;
    pTrustee->TrusteeType = TRUSTEE_IS_UNKNOWN;
    pTrustee->ptstrName = (LPWSTR) pSid;
}


/******************************************************************************
 * BuildTrusteeWithNameA [ADVAPI32.@]
 */
VOID WINAPI
BuildTrusteeWithNameA(PTRUSTEE_A pTrustee, LPSTR name)
{
    DPRINT("%p %s\n", pTrustee, name);

    pTrustee->pMultipleTrustee = NULL;
    pTrustee->MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    pTrustee->TrusteeForm = TRUSTEE_IS_NAME;
    pTrustee->TrusteeType = TRUSTEE_IS_UNKNOWN;
    pTrustee->ptstrName = name;
}


/******************************************************************************
 * BuildTrusteeWithNameW [ADVAPI32.@]
 */
VOID WINAPI
BuildTrusteeWithNameW(PTRUSTEE_W pTrustee, LPWSTR name)
{
    DPRINT("%p %s\n", pTrustee, name);

    pTrustee->pMultipleTrustee = NULL;
    pTrustee->MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    pTrustee->TrusteeForm = TRUSTEE_IS_NAME;
    pTrustee->TrusteeType = TRUSTEE_IS_UNKNOWN;
    pTrustee->ptstrName = name;
}


/******************************************************************************
 * BuildTrusteeWithObjectsAndNameA [ADVAPI32.@]
 */
VOID WINAPI
BuildTrusteeWithObjectsAndNameA(PTRUSTEEA pTrustee, POBJECTS_AND_NAME_A pObjName,
                                SE_OBJECT_TYPE ObjectType, LPSTR ObjectTypeName,
                                LPSTR InheritedObjectTypeName, LPSTR Name)
{
    DPRINT("%p %p 0x%08x %p %p %s\n", pTrustee, pObjName,
           ObjectType, ObjectTypeName, InheritedObjectTypeName, debugstr_a(Name));

    pTrustee->pMultipleTrustee = NULL;
    pTrustee->MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    pTrustee->TrusteeForm = TRUSTEE_IS_OBJECTS_AND_NAME;
    pTrustee->TrusteeType = TRUSTEE_IS_UNKNOWN;
    pTrustee->ptstrName = Name;
}


/******************************************************************************
 * BuildTrusteeWithObjectsAndNameW [ADVAPI32.@]
 */
VOID WINAPI
BuildTrusteeWithObjectsAndNameW(PTRUSTEEW pTrustee, POBJECTS_AND_NAME_W pObjName,
                                SE_OBJECT_TYPE ObjectType, LPWSTR ObjectTypeName,
                                LPWSTR InheritedObjectTypeName, LPWSTR Name)
{
    DPRINT("%p %p 0x%08x %p %p %s\n", pTrustee, pObjName,
           ObjectType, ObjectTypeName, InheritedObjectTypeName, debugstr_w(Name));

    pTrustee->pMultipleTrustee = NULL;
    pTrustee->MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    pTrustee->TrusteeForm = TRUSTEE_IS_OBJECTS_AND_NAME;
    pTrustee->TrusteeType = TRUSTEE_IS_UNKNOWN;
    pTrustee->ptstrName = Name;
}


/******************************************************************************
 * BuildTrusteeWithObjectsAndSidA [ADVAPI32.@]
 */
VOID WINAPI
BuildTrusteeWithObjectsAndSidA(PTRUSTEEA pTrustee, POBJECTS_AND_SID pObjSid,
                               GUID* pObjectGuid, GUID* pInheritedObjectGuid, PSID pSid)
{
    DPRINT("%p %p %p %p %p\n", pTrustee, pObjSid, pObjectGuid, pInheritedObjectGuid, pSid);

    pTrustee->pMultipleTrustee = NULL;
    pTrustee->MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    pTrustee->TrusteeForm = TRUSTEE_IS_OBJECTS_AND_SID;
    pTrustee->TrusteeType = TRUSTEE_IS_UNKNOWN;
    pTrustee->ptstrName = (LPSTR) pSid;
}


/******************************************************************************
 * BuildTrusteeWithObjectsAndSidW [ADVAPI32.@]
 */
VOID WINAPI
BuildTrusteeWithObjectsAndSidW(PTRUSTEEW pTrustee, POBJECTS_AND_SID pObjSid,
                               GUID* pObjectGuid, GUID* pInheritedObjectGuid, PSID pSid)
{
    DPRINT("%p %p %p %p %p\n", pTrustee, pObjSid, pObjectGuid, pInheritedObjectGuid, pSid);

    pTrustee->pMultipleTrustee = NULL;
    pTrustee->MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    pTrustee->TrusteeForm = TRUSTEE_IS_OBJECTS_AND_SID;
    pTrustee->TrusteeType = TRUSTEE_IS_UNKNOWN;
    pTrustee->ptstrName = (LPWSTR) pSid;
}


/******************************************************************************
 * GetMultipleTrusteeA [ADVAPI32.@]
 */
PTRUSTEEA WINAPI
GetMultipleTrusteeA(PTRUSTEE_A pTrustee)
{
  return pTrustee->pMultipleTrustee;
}


/******************************************************************************
 * GetMultipleTrusteeW [ADVAPI32.@]
 */
PTRUSTEEW WINAPI
GetMultipleTrusteeW(PTRUSTEE_W pTrustee)
{
  return pTrustee->pMultipleTrustee;
}


/******************************************************************************
 * GetMultipleTrusteeOperationA [ADVAPI32.@]
 */
MULTIPLE_TRUSTEE_OPERATION WINAPI
GetMultipleTrusteeOperationA(PTRUSTEE_A pTrustee)
{
  return pTrustee->MultipleTrusteeOperation;
}


/******************************************************************************
 * GetMultipleTrusteeOperationW [ADVAPI32.@]
 */
MULTIPLE_TRUSTEE_OPERATION WINAPI
GetMultipleTrusteeOperationW(PTRUSTEE_W pTrustee)
{
  return pTrustee->MultipleTrusteeOperation;
}


/******************************************************************************
 * GetTrusteeFormW [ADVAPI32.@]
 */
TRUSTEE_FORM WINAPI
GetTrusteeFormA(PTRUSTEE_A pTrustee)
{
  return pTrustee->TrusteeForm;
}


/******************************************************************************
 * GetTrusteeFormW [ADVAPI32.@]
 */
TRUSTEE_FORM WINAPI
GetTrusteeFormW(PTRUSTEE_W pTrustee)
{
  return pTrustee->TrusteeForm;
}


/******************************************************************************
 * GetTrusteeNameA [ADVAPI32.@]
 */
LPSTR WINAPI
GetTrusteeNameA(PTRUSTEE_A pTrustee)
{
  return (pTrustee->TrusteeForm == TRUSTEE_IS_NAME) ? pTrustee->ptstrName : NULL;
}


/******************************************************************************
 * GetTrusteeNameW [ADVAPI32.@]
 */
LPWSTR WINAPI
GetTrusteeNameW(PTRUSTEE_W pTrustee)
{
  return (pTrustee->TrusteeForm == TRUSTEE_IS_NAME) ? pTrustee->ptstrName : NULL;
}


/******************************************************************************
 * GetTrusteeTypeA [ADVAPI32.@]
 */
TRUSTEE_TYPE WINAPI
GetTrusteeTypeA(PTRUSTEE_A pTrustee)
{
  return pTrustee->TrusteeType;
}


/******************************************************************************
 * GetTrusteeTypeW [ADVAPI32.@]
 */
TRUSTEE_TYPE WINAPI
GetTrusteeTypeW(PTRUSTEE_W pTrustee)
{
  return pTrustee->TrusteeType;
}

/* EOF */
