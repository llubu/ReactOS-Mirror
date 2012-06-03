/*
 *  ReactOS kernel
 *  Copyright (C) 2004 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/* $Id$
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           SAM interface library
 * FILE:              lib/samlib/samlib.c
 * PROGRAMER:         Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(samlib);

/* GLOBALS *******************************************************************/


/* FUNCTIONS *****************************************************************/

void __RPC_FAR * __RPC_USER midl_user_allocate(SIZE_T len)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
}


void __RPC_USER midl_user_free(void __RPC_FAR * ptr)
{
    HeapFree(GetProcessHeap(), 0, ptr);
}


handle_t __RPC_USER
PSAMPR_SERVER_NAME_bind(PSAMPR_SERVER_NAME pszSystemName)
{
    handle_t hBinding = NULL;
    LPWSTR pszStringBinding;
    RPC_STATUS status;

    TRACE("PSAMPR_SERVER_NAME_bind() called\n");

    status = RpcStringBindingComposeW(NULL,
                                      L"ncacn_np",
                                      pszSystemName,
                                      L"\\pipe\\samr",
                                      NULL,
                                      &pszStringBinding);
    if (status)
    {
        TRACE("RpcStringBindingCompose returned 0x%x\n", status);
        return NULL;
    }

    /* Set the binding handle that will be used to bind to the server. */
    status = RpcBindingFromStringBindingW(pszStringBinding,
                                          &hBinding);
    if (status)
    {
        TRACE("RpcBindingFromStringBinding returned 0x%x\n", status);
    }

    status = RpcStringFreeW(&pszStringBinding);
    if (status)
    {
//        TRACE("RpcStringFree returned 0x%x\n", status);
    }

    return hBinding;
}


void __RPC_USER
PSAMPR_SERVER_NAME_unbind(PSAMPR_SERVER_NAME pszSystemName,
                          handle_t hBinding)
{
    RPC_STATUS status;

    TRACE("PSAMPR_SERVER_NAME_unbind() called\n");

    status = RpcBindingFree(&hBinding);
    if (status)
    {
        TRACE("RpcBindingFree returned 0x%x\n", status);
    }
}


NTSTATUS
NTAPI
SamAddMemberToAlias(IN SAM_HANDLE AliasHandle,
                    IN PSID MemberId)
{
    NTSTATUS Status;

    TRACE("SamAddMemberToAlias(%p %p)\n",
          AliasHandle, MemberId);

    RpcTryExcept
    {
        Status = SamrAddMemberToAlias((SAMPR_HANDLE)AliasHandle,
                                      (PRPC_SID)MemberId);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}


NTSTATUS
NTAPI
SamCloseHandle(IN SAM_HANDLE SamHandle)
{
    NTSTATUS Status;

    TRACE("SamCloseHandle(%p)\n", SamHandle);

    RpcTryExcept
    {
        Status = SamrCloseHandle((SAMPR_HANDLE *)&SamHandle);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}


NTSTATUS
NTAPI
SamConnect(IN OUT PUNICODE_STRING ServerName,
           OUT PSAM_HANDLE ServerHandle,
           IN ACCESS_MASK DesiredAccess,
           IN POBJECT_ATTRIBUTES ObjectAttributes)
{
    NTSTATUS Status;

    TRACE("SamConnect(%p,%p,0x%08x,%p)\n",
          ServerName, ServerHandle, DesiredAccess, ObjectAttributes);

    RpcTryExcept
    {
        Status = SamrConnect((PSAMPR_SERVER_NAME)ServerName,
                             (SAMPR_HANDLE *)ServerHandle,
                             DesiredAccess);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}


NTSTATUS
NTAPI
SamCreateAliasInDomain(IN SAM_HANDLE DomainHandle,
                       IN PUNICODE_STRING AccountName,
                       IN ACCESS_MASK DesiredAccess,
                       OUT PSAM_HANDLE AliasHandle,
                       OUT PULONG RelativeId)
{
    NTSTATUS Status;

    TRACE("SamCreateAliasInDomain(%p,%p,0x%08x,%p,%p)\n",
          DomainHandle, AccountName, DesiredAccess, AliasHandle, RelativeId);

    RpcTryExcept
    {
        Status = SamrCreateAliasInDomain((SAMPR_HANDLE)DomainHandle,
                                         (PRPC_UNICODE_STRING)AccountName,
                                         DesiredAccess,
                                         (SAMPR_HANDLE *)AliasHandle,
                                         RelativeId);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}


NTSTATUS
NTAPI
SamCreateUserInDomain(IN SAM_HANDLE DomainHandle,
                      IN PUNICODE_STRING AccountName,
                      IN ACCESS_MASK DesiredAccess,
                      OUT PSAM_HANDLE UserHandle,
                      OUT PULONG RelativeId)
{
    NTSTATUS Status;

    TRACE("SamCreateUserInDomain(%p,%p,0x%08x,%p,%p)\n",
          DomainHandle, AccountName, DesiredAccess, UserHandle, RelativeId);

    RpcTryExcept
    {
        Status = SamrCreateUserInDomain((SAMPR_HANDLE)DomainHandle,
                                        (PRPC_UNICODE_STRING)AccountName,
                                        DesiredAccess,
                                        (SAMPR_HANDLE *)UserHandle,
                                        RelativeId);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}


NTSTATUS
NTAPI
SamEnumerateDomainsInSamServer(IN SAM_HANDLE ServerHandle,
                               IN OUT PSAM_ENUMERATE_HANDLE EnumerationContext,
                               OUT PVOID *Buffer,
                               IN ULONG PreferedMaximumLength,
                               OUT PULONG CountReturned)
{
    PSAMPR_ENUMERATION_BUFFER EnumBuffer = NULL;
    NTSTATUS Status;

    TRACE("SamEnumerateDomainsInSamServer(%p,%p,%p,%lu,%p)\n",
          ServerHandle, EnumerationContext, Buffer, PreferedMaximumLength,
          CountReturned);

    if ((EnumerationContext == NULL) ||
        (Buffer == NULL) ||
        (CountReturned == NULL))
        return STATUS_INVALID_PARAMETER;

    *Buffer = NULL;

    RpcTryExcept
    {
        Status = SamrEnumerateDomainsInSamServer((SAMPR_HANDLE)ServerHandle,
                                                 EnumerationContext,
                                                 (PSAMPR_ENUMERATION_BUFFER *)&EnumBuffer,
                                                 PreferedMaximumLength,
                                                 CountReturned);

        if (EnumBuffer != NULL)
        {
            if (EnumBuffer->Buffer != NULL)
            {
                *Buffer = EnumBuffer->Buffer;
            }

            midl_user_free(EnumBuffer);
        }
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}


NTSTATUS
NTAPI
SamFreeMemory(IN PVOID Buffer)
{
    if (Buffer != NULL)
        midl_user_free(Buffer);

    return STATUS_SUCCESS;
}


NTSTATUS
NTAPI
SamLookupDomainInSamServer(IN SAM_HANDLE ServerHandle,
                           IN PUNICODE_STRING Name,
                           OUT PSID *DomainId)
{
    NTSTATUS Status;

    TRACE("SamLookupDomainInSamServer(%p,%p,%p)\n",
          ServerHandle, Name, DomainId);

    RpcTryExcept
    {
        Status = SamrLookupDomainInSamServer((SAMPR_HANDLE)ServerHandle,
                                             (PRPC_UNICODE_STRING)Name,
                                             (PRPC_SID *)DomainId);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}


NTSTATUS
NTAPI
SamOpenAlias(IN SAM_HANDLE DomainHandle,
             IN ACCESS_MASK DesiredAccess,
             IN ULONG AliasId,
             OUT PSAM_HANDLE AliasHandle)
{
    NTSTATUS Status;

    TRACE("SamOpenAlias(%p 0x%08x %lx %p)\n",
          DomainHandle, DesiredAccess, AliasId, AliasHandle);

    RpcTryExcept
    {
        Status = SamrOpenAlias((SAMPR_HANDLE)DomainHandle,
                               DesiredAccess,
                               AliasId,
                               (SAMPR_HANDLE *)AliasHandle);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}


NTSTATUS
NTAPI
SamOpenDomain(IN SAM_HANDLE ServerHandle,
              IN ACCESS_MASK DesiredAccess,
              IN PSID DomainId,
              OUT PSAM_HANDLE DomainHandle)
{
    NTSTATUS Status;

    TRACE("SamOpenDomain(%p,0x%08x,%p,%p)\n",
          ServerHandle, DesiredAccess, DomainId, DomainHandle);

    RpcTryExcept
    {
        Status = SamrOpenDomain((SAMPR_HANDLE)ServerHandle,
                                DesiredAccess,
                                (PRPC_SID)DomainId,
                                (SAMPR_HANDLE *)DomainHandle);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}


NTSTATUS
NTAPI
SamOpenUser(IN SAM_HANDLE DomainHandle,
            IN ACCESS_MASK DesiredAccess,
            IN ULONG UserId,
            OUT PSAM_HANDLE UserHandle)
{
    NTSTATUS Status;

    TRACE("SamOpenUser(%p,0x%08x,%lx,%p)\n",
          DomainHandle, DesiredAccess, UserId, UserHandle);

    RpcTryExcept
    {
        Status = SamrOpenUser((SAMPR_HANDLE)DomainHandle,
                              DesiredAccess,
                              UserId,
                              (SAMPR_HANDLE *)UserHandle);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}


NTSTATUS
NTAPI
SamQueryInformationDomain(IN SAM_HANDLE DomainHandle,
                          IN DOMAIN_INFORMATION_CLASS DomainInformationClass,
                          OUT PVOID *Buffer)
{
    NTSTATUS Status;

    TRACE("SamQueryInformationDomain(%p %lu %p)\n",
          DomainHandle, DomainInformationClass, Buffer);

    RpcTryExcept
    {
        Status = SamrQueryInformationDomain((SAMPR_HANDLE)DomainHandle,
                                            DomainInformationClass,
                                            (PSAMPR_DOMAIN_INFO_BUFFER *)Buffer);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}


NTSTATUS
NTAPI
SamQueryInformationUser(IN SAM_HANDLE UserHandle,
                        IN USER_INFORMATION_CLASS UserInformationClass,
                        OUT PVOID *Buffer)
{
    NTSTATUS Status;

    TRACE("SamQueryInformationUser(%p %lu %p)\n",
          UserHandle, UserInformationClass, Buffer);

    RpcTryExcept
    {
        Status = SamrQueryInformationUser((SAMPR_HANDLE)UserHandle,
                                          UserInformationClass,
                                          (PSAMPR_USER_INFO_BUFFER *)Buffer);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}


NTSTATUS
NTAPI
SamSetInformationDomain(IN SAM_HANDLE DomainHandle,
                        IN DOMAIN_INFORMATION_CLASS DomainInformationClass,
                        IN PVOID DomainInformation)
{
    NTSTATUS Status;

    TRACE("SamSetInformationDomain(%p %lu %p)\n",
          DomainHandle, DomainInformationClass, DomainInformation);

    RpcTryExcept
    {
        Status = SamrSetInformationDomain((SAMPR_HANDLE)DomainHandle,
                                          DomainInformationClass,
                                          DomainInformation);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}


NTSTATUS
NTAPI
SamSetInformationUser(IN SAM_HANDLE UserHandle,
                      IN USER_INFORMATION_CLASS UserInformationClass,
                      IN PVOID Buffer)
{
    NTSTATUS Status;

    TRACE("SamSetInformationUser(%p %lu %p)\n",
          UserHandle, UserInformationClass, Buffer);

    RpcTryExcept
    {
        Status = SamrSetInformationUser((SAMPR_HANDLE)UserHandle,
                                        UserInformationClass,
                                        Buffer);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}


NTSTATUS
NTAPI
SamShutdownSamServer(IN SAM_HANDLE ServerHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
