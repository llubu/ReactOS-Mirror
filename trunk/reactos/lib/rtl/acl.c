/* COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * PURPOSE:         Security manager
 * FILE:            lib/rtl/acl.c
 * PROGRAMER:       David Welch <welch@cwcom.net>
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ***************************************************************/

BOOLEAN NTAPI
RtlFirstFreeAce(PACL Acl,
                PACE* Ace)
{
   PACE Current;
   ULONG_PTR AclEnd;
   ULONG i;

   PAGED_CODE_RTL();

   Current = (PACE)(Acl + 1);
   *Ace = NULL;

   if (Acl->AceCount == 0)
   {
      *Ace = Current;
      return(TRUE);
   }
   
   i = 0;
   AclEnd = (ULONG_PTR)Acl + Acl->AclSize;
   do
   {
      if ((ULONG_PTR)Current >= AclEnd)
      {
         return(FALSE);
      }
      if (Current->Header.AceType == ACCESS_ALLOWED_COMPOUND_ACE_TYPE &&
          Acl->AclRevision < ACL_REVISION3)
      {
         return(FALSE);
      }
      Current = (PACE)((ULONG_PTR)Current + Current->Header.AceSize);
   }
   while (++i < Acl->AceCount);

   if ((ULONG_PTR)Current < AclEnd)
   {
      *Ace = Current;
   }

   return(TRUE);
}


/*
 * @implemented
 */
NTSTATUS NTAPI
RtlGetAce(PACL Acl,
          ULONG AceIndex,
          PVOID *Ace)
{
   ULONG i;

   PAGED_CODE_RTL();

   if (Acl->AclRevision < MIN_ACL_REVISION ||
       Acl->AclRevision > MAX_ACL_REVISION ||
       AceIndex >= Acl->AceCount)
   {
      return(STATUS_INVALID_PARAMETER);
   }
   
   *Ace = (PVOID)((PACE)(Acl + 1));

   for (i = 0; i < AceIndex; i++)
   {
      if ((ULONG_PTR)*Ace >= (ULONG_PTR)Acl + Acl->AclSize)
      {
         return(STATUS_INVALID_PARAMETER);
      }
      *Ace = (PVOID)((PACE)((ULONG_PTR)(*Ace) + ((PACE)(*Ace))->Header.AceSize));
   }

   if ((ULONG_PTR)*Ace >= (ULONG_PTR)Acl + Acl->AclSize)
   {
      return(STATUS_INVALID_PARAMETER);
   }

   return(STATUS_SUCCESS);
}


static NTSTATUS
RtlpAddKnownAce (PACL Acl,
                 ULONG Revision,
                 ULONG Flags,
                 ACCESS_MASK AccessMask,
                 PSID Sid,
                 ULONG Type)
{
   PACE Ace;
   ULONG InvalidFlags;

   PAGED_CODE_RTL();

   if (!RtlValidSid(Sid))
   {
      return(STATUS_INVALID_SID);
   }
   if (Acl->AclRevision > MAX_ACL_REVISION ||
       Revision > MAX_ACL_REVISION)
   {
      return(STATUS_UNKNOWN_REVISION);
   }
   if (Revision < Acl->AclRevision)
   {
      Revision = Acl->AclRevision;
   }

   /* Validate the flags */
   if (Type == SYSTEM_AUDIT_ACE_TYPE)
      InvalidFlags = Flags & ~(VALID_INHERIT_FLAGS |
                               SUCCESSFUL_ACCESS_ACE_FLAG | FAILED_ACCESS_ACE_FLAG);
   else
      InvalidFlags = Flags & ~VALID_INHERIT_FLAGS;

   if (InvalidFlags != 0)
   {
      return(STATUS_INVALID_PARAMETER);
   }

   if (!RtlFirstFreeAce(Acl, &Ace))
   {
      return(STATUS_INVALID_ACL);
   }
   if (Ace == NULL)
   {
      return(STATUS_ALLOTTED_SPACE_EXCEEDED);
   }
   if ((ULONG_PTR)Ace + RtlLengthSid(Sid) + sizeof(ACE) >
       (ULONG_PTR)Acl + Acl->AclSize)
   {
      return(STATUS_ALLOTTED_SPACE_EXCEEDED);
   }
   Ace->Header.AceFlags = Flags;
   Ace->Header.AceType = Type;
   Ace->Header.AceSize = RtlLengthSid(Sid) + sizeof(ACE);
   Ace->AccessMask = AccessMask;
   RtlCopySid(RtlLengthSid(Sid), (PSID)(Ace + 1), Sid);
   Acl->AceCount++;
   Acl->AclRevision = Revision;
   return(STATUS_SUCCESS);
}


/*
 * @implemented
 */
NTSTATUS NTAPI
RtlAddAccessAllowedAce (IN OUT PACL Acl,
                        IN ULONG Revision,
                        IN ACCESS_MASK AccessMask,
                        IN PSID Sid)
{
   PAGED_CODE_RTL();

   return RtlpAddKnownAce (Acl,
                           Revision,
                           0,
                           AccessMask,
                           Sid,
                           ACCESS_ALLOWED_ACE_TYPE);
}


/*
 * @implemented
 */
NTSTATUS NTAPI
RtlAddAccessAllowedAceEx (IN OUT PACL Acl,
                          IN ULONG Revision,
                          IN ULONG Flags,
                          IN ACCESS_MASK AccessMask,
                          IN PSID Sid)
{
   PAGED_CODE_RTL();

   return RtlpAddKnownAce (Acl,
                           Revision,
                           Flags,
                           AccessMask,
                           Sid,
                           ACCESS_ALLOWED_ACE_TYPE);
}


/*
 * @implemented
 */
NTSTATUS NTAPI
RtlAddAccessDeniedAce (PACL Acl,
                       ULONG Revision,
                       ACCESS_MASK AccessMask,
                       PSID Sid)
{
   PAGED_CODE_RTL();

   return RtlpAddKnownAce (Acl,
                           Revision,
                           0,
                           AccessMask,
                           Sid,
                           ACCESS_DENIED_ACE_TYPE);
}


/*
 * @implemented
 */
NTSTATUS NTAPI
RtlAddAccessDeniedAceEx (IN OUT PACL Acl,
                         IN ULONG Revision,
                         IN ULONG Flags,
                         IN ACCESS_MASK AccessMask,
                         IN PSID Sid)
{
   PAGED_CODE_RTL();

   return RtlpAddKnownAce (Acl,
                           Revision,
                           Flags,
                           AccessMask,
                           Sid,
                           ACCESS_DENIED_ACE_TYPE);
}


static VOID
RtlpAddData(PVOID AceList,
            ULONG AceListLength,
            PVOID Ace,
            ULONG Offset)
{
   if (Offset > 0)
   {
      RtlCopyMemory((PVOID)((ULONG_PTR)Ace + AceListLength),
                    Ace,
                    Offset);
   }

   if (AceListLength != 0)
   {
      RtlCopyMemory(Ace,
                    AceList,
                    AceListLength);
   }
}


/*
 * @implemented
 */
NTSTATUS NTAPI
RtlAddAce(PACL Acl,
          ULONG AclRevision,
          ULONG StartingIndex,
          PVOID AceList,
          ULONG AceListLength)
{
   PACE Ace;
   PACE Current;
   ULONG NewAceCount;
   ULONG Index;

   PAGED_CODE_RTL();

   if (Acl->AclRevision < MIN_ACL_REVISION ||
       Acl->AclRevision > MAX_ACL_REVISION ||
       !RtlFirstFreeAce(Acl, &Ace))
   {
      return(STATUS_INVALID_PARAMETER);
   }

   if (Acl->AclRevision <= AclRevision)
   {
      AclRevision = Acl->AclRevision;
   }

   if (((ULONG_PTR)AceList + AceListLength) <= (ULONG_PTR)AceList)
   {
      return(STATUS_INVALID_PARAMETER);
   }

   for (Current = AceList, NewAceCount = 0;
        (ULONG_PTR)Current < ((ULONG_PTR)AceList + AceListLength);
        Current = (PACE)((ULONG_PTR)Current + Current->Header.AceSize),
        ++NewAceCount)
   {
      if (((PACE)AceList)->Header.AceType == ACCESS_ALLOWED_COMPOUND_ACE_TYPE &&
          AclRevision < ACL_REVISION3)
      {
         return(STATUS_INVALID_PARAMETER);
      }
   }

   if (Ace == NULL ||
       ((ULONG_PTR)Ace + AceListLength) > ((ULONG_PTR)Acl + Acl->AclSize))
   {
      return(STATUS_BUFFER_TOO_SMALL);
   }

   Current = (PACE)(Acl + 1);
   for (Index = 0; Index < StartingIndex && Index < Acl->AceCount; Index++)
   {
      Current = (PACE)((ULONG_PTR)Current + Current->Header.AceSize);
   }

   RtlpAddData(AceList,
               AceListLength,
               Current,
               (ULONG)((ULONG_PTR)Ace - (ULONG_PTR)Current));
   Acl->AceCount = Acl->AceCount + NewAceCount;
   Acl->AclRevision = AclRevision;

   return(STATUS_SUCCESS);
}


/*
 * @implemented
 */
NTSTATUS NTAPI
RtlAddAuditAccessAce(PACL Acl,
                     ULONG Revision,
                     ACCESS_MASK AccessMask,
                     PSID Sid,
                     BOOLEAN Success,
                     BOOLEAN Failure)
{
   ULONG Flags = 0;

   PAGED_CODE_RTL();

   if (Success)
   {
      Flags |= SUCCESSFUL_ACCESS_ACE_FLAG;
   }

   if (Failure)
   {
      Flags |= FAILED_ACCESS_ACE_FLAG;
   }

   return RtlpAddKnownAce (Acl,
                           Revision,
                           Flags,
                           AccessMask,
                           Sid,
                           SYSTEM_AUDIT_ACE_TYPE);
}


/*
 * @implemented
 */
NTSTATUS NTAPI
RtlAddAuditAccessAceEx(PACL Acl,
                       ULONG Revision,
                       ULONG Flags,
                       ACCESS_MASK AccessMask,
                       PSID Sid,
                       BOOLEAN Success,
                       BOOLEAN Failure)
{
   if (Success)
   {
      Flags |= SUCCESSFUL_ACCESS_ACE_FLAG;
   }

   if (Failure)
   {
      Flags |= FAILED_ACCESS_ACE_FLAG;
   }

   return RtlpAddKnownAce (Acl,
                           Revision,
                           Flags,
                           AccessMask,
                           Sid,
                           SYSTEM_AUDIT_ACE_TYPE);
}


static VOID
RtlpDeleteData(PVOID Ace,
               ULONG AceSize,
               ULONG Offset)
{
   if (AceSize < Offset)
   {
      RtlCopyMemory(Ace,
                    (PVOID)((ULONG_PTR)Ace + AceSize),
                    Offset - AceSize);
   }

   if (Offset - AceSize < Offset)
   {
      RtlZeroMemory((PVOID)((ULONG_PTR)Ace + Offset - AceSize),
                    AceSize);
   }
}


/*
 * @implemented
 */
NTSTATUS NTAPI
RtlDeleteAce(PACL Acl,
             ULONG AceIndex)
{
   PACE Ace;
   PACE Current;

   PAGED_CODE_RTL();

   if (Acl->AclRevision < MIN_ACL_REVISION ||
       Acl->AclRevision > MAX_ACL_REVISION ||
       Acl->AceCount <= AceIndex ||
       !RtlFirstFreeAce(Acl, &Ace))
   {
      return(STATUS_INVALID_PARAMETER);
   }

   Current = (PACE)(Acl + 1);

   while(AceIndex--)
   {
      Current = (PACE)((ULONG_PTR)Current + Current->Header.AceSize);
   }

   RtlpDeleteData(Current,
                  Current->Header.AceSize,
                  (ULONG)((ULONG_PTR)Ace - (ULONG_PTR)Current));
   Acl->AceCount--;

   return(STATUS_SUCCESS);
}


/*
 * @implemented
 */
NTSTATUS NTAPI
RtlCreateAcl(PACL Acl,
             ULONG AclSize,
             ULONG AclRevision)
{
   PAGED_CODE_RTL();

   if (AclSize < sizeof(ACL))
   {
      return(STATUS_BUFFER_TOO_SMALL);
   }

   if (AclRevision < MIN_ACL_REVISION ||
       AclRevision > MAX_ACL_REVISION ||
       AclSize > 0xffff)
   {
      return(STATUS_INVALID_PARAMETER);
   }

   AclSize = ROUND_UP(AclSize, 4);
   Acl->AclSize = AclSize;
   Acl->AclRevision = AclRevision;
   Acl->AceCount = 0;
   Acl->Sbz1 = 0;
   Acl->Sbz2 = 0;

   return(STATUS_SUCCESS);
}


/*
 * @implemented
 */
NTSTATUS NTAPI
RtlQueryInformationAcl(PACL Acl,
                       PVOID Information,
                       ULONG InformationLength,
                       ACL_INFORMATION_CLASS InformationClass)
{
   PACE Ace;

   PAGED_CODE_RTL();

   if (Acl->AclRevision < MIN_ACL_REVISION ||
       Acl->AclRevision > MAX_ACL_REVISION)
   {
      return(STATUS_INVALID_PARAMETER);
   }

   switch (InformationClass)
   {
      case AclRevisionInformation:
         {
            PACL_REVISION_INFORMATION Info = (PACL_REVISION_INFORMATION)Information;

            if (InformationLength < sizeof(ACL_REVISION_INFORMATION))
            {
               return(STATUS_BUFFER_TOO_SMALL);
            }
            Info->AclRevision = Acl->AclRevision;
         }
         break;

      case AclSizeInformation:
         {
            PACL_SIZE_INFORMATION Info = (PACL_SIZE_INFORMATION)Information;

            if (InformationLength < sizeof(ACL_SIZE_INFORMATION))
            {
               return(STATUS_BUFFER_TOO_SMALL);
            }

            if (!RtlFirstFreeAce(Acl, &Ace))
            {
               return(STATUS_INVALID_PARAMETER);
            }

            Info->AceCount = Acl->AceCount;
            if (Ace != NULL)
            {
               Info->AclBytesInUse = (DWORD)((ULONG_PTR)Ace - (ULONG_PTR)Acl);
               Info->AclBytesFree  = Acl->AclSize - Info->AclBytesInUse;
            }
            else
            {
               Info->AclBytesInUse = Acl->AclSize;
               Info->AclBytesFree  = 0;
            }
         }
         break;

      default:
         return(STATUS_INVALID_INFO_CLASS);
   }

   return(STATUS_SUCCESS);
}


/*
 * @implemented
 */
NTSTATUS NTAPI
RtlSetInformationAcl(PACL Acl,
                     PVOID Information,
                     ULONG InformationLength,
                     ACL_INFORMATION_CLASS InformationClass)
{
   PAGED_CODE_RTL();

   if (Acl->AclRevision < MIN_ACL_REVISION ||
       Acl->AclRevision > MAX_ACL_REVISION)
   {
      return(STATUS_INVALID_PARAMETER);
   }

   switch (InformationClass)
   {
      case AclRevisionInformation:
         {
            PACL_REVISION_INFORMATION Info = (PACL_REVISION_INFORMATION)Information;

            if (InformationLength < sizeof(ACL_REVISION_INFORMATION))
            {
               return(STATUS_BUFFER_TOO_SMALL);
            }

            if (Acl->AclRevision >= Info->AclRevision)
            {
               return(STATUS_INVALID_PARAMETER);
            }

            Acl->AclRevision = Info->AclRevision;
         }
         break;

      default:
         return(STATUS_INVALID_INFO_CLASS);
   }

   return(STATUS_SUCCESS);
}


/*
 * @implemented
 */
BOOLEAN NTAPI
RtlValidAcl (PACL Acl)
{
   PACE Ace;
   USHORT Size;

   PAGED_CODE_RTL();

   Size = ROUND_UP(Acl->AclSize, 4);

   if (Acl->AclRevision < MIN_ACL_REVISION ||
       Acl->AclRevision > MAX_ACL_REVISION)
   {
      return(FALSE);
   }

   if (Size != Acl->AclSize)
   {
      return(FALSE);
   }

   return(RtlFirstFreeAce(Acl, &Ace));
}

/* EOF */
