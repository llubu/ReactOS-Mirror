/* $Id: sd.c,v 1.10 2003/06/01 18:14:24 ekohl Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Security descriptor functions
 * FILE:              lib/ntdll/rtl/sd.c
 * PROGRAMER:         David Welch <welch@cwcom.net>
 * REVISION HISTORY:
 *                 26/07/98: Added stubs for security functions
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <ntdll/ntdll.h>

/* FUNCTIONS ***************************************************************/

NTSTATUS STDCALL
RtlCreateSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,
			    ULONG Revision)
{
  if (Revision != 1)
    {
      return(STATUS_UNSUCCESSFUL);
    }

  SecurityDescriptor->Revision = 1;
  SecurityDescriptor->Sbz1 = 0;
  SecurityDescriptor->Control = 0;
  SecurityDescriptor->Owner = NULL;
  SecurityDescriptor->Group = NULL;
  SecurityDescriptor->Sacl = NULL;
  SecurityDescriptor->Dacl = NULL;

  return(STATUS_SUCCESS);
}

ULONG STDCALL
RtlLengthSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor)
{
  PSID Owner;
  PSID Group;
  ULONG Length;
  PACL Dacl;
  PACL Sacl;

  Length = sizeof(SECURITY_DESCRIPTOR);

  if (SecurityDescriptor->Owner != NULL)
    {
	Owner = SecurityDescriptor->Owner;
	if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
	  {
	     Owner = (PSID)((ULONG)Owner + 
			    (ULONG)SecurityDescriptor);
	  }
	Length = Length + ((sizeof(SID) + (Owner->SubAuthorityCount - 1) * 
			   sizeof(ULONG) + 3) & 0xfc);
    }

  if (SecurityDescriptor->Group != NULL)
    {
	Group = SecurityDescriptor->Group;
	if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
	  {
	     Group = (PSID)((ULONG)Group + (ULONG)SecurityDescriptor);
	  }
	Length = Length + ((sizeof(SID) + (Group->SubAuthorityCount - 1) *
			   sizeof(ULONG) + 3) & 0xfc);
    }

  if (SecurityDescriptor->Control & SE_DACL_PRESENT &&
      SecurityDescriptor->Dacl != NULL)
    {
	Dacl = SecurityDescriptor->Dacl;
	if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
	  {
	     Dacl = (PACL)((ULONG)Dacl + (PVOID)SecurityDescriptor);
	  }
	Length = Length + ((Dacl->AclSize + 3) & 0xfc);
    }

  if (SecurityDescriptor->Control & SE_SACL_PRESENT &&
      SecurityDescriptor->Sacl != NULL)
    {
	Sacl = SecurityDescriptor->Sacl;
	if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
	  {
	     Sacl = (PACL)((ULONG)Sacl + (PVOID)SecurityDescriptor);
	  }
	Length = Length + ((Sacl->AclSize + 3) & 0xfc);
    }

  return(Length);
}


NTSTATUS STDCALL
RtlGetDaclSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,
			     PBOOLEAN DaclPresent,
			     PACL* Dacl,
			     PBOOLEAN DaclDefaulted)
{
   if (SecurityDescriptor->Revision != 1)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   if (!(SecurityDescriptor->Control & SE_DACL_PRESENT))
     {
	*DaclPresent = 0;
	return(STATUS_SUCCESS);
     }
   *DaclPresent = 1;
   if (SecurityDescriptor->Dacl == NULL)
     {
	*Dacl = NULL;
     }
   else
     {
	if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
	  {
	     *Dacl = (PACL)((ULONG)SecurityDescriptor->Dacl +
			    (PVOID)SecurityDescriptor);
	  }
	else
	  {
	     *Dacl = SecurityDescriptor->Dacl;
	  }
     }
   if (SecurityDescriptor->Control & SE_DACL_DEFAULTED)
     {
	*DaclDefaulted = 1;
     }
   else
     {
	*DaclDefaulted = 0;
     }
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
RtlSetDaclSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,
			     BOOLEAN DaclPresent,
			     PACL Dacl,
			     BOOLEAN DaclDefaulted)
{
   if (SecurityDescriptor->Revision != 1)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   if (!DaclPresent)
     {
	SecurityDescriptor->Control = SecurityDescriptor->Control & ~(SE_DACL_PRESENT);
	return(STATUS_SUCCESS);
     }
   SecurityDescriptor->Control = SecurityDescriptor->Control | SE_DACL_PRESENT;
   SecurityDescriptor->Dacl = Dacl;
   SecurityDescriptor->Control = SecurityDescriptor->Control & ~(SE_DACL_DEFAULTED);
   if (DaclDefaulted)
     {
	SecurityDescriptor->Control = SecurityDescriptor->Control | SE_DACL_DEFAULTED;
     }
   return(STATUS_SUCCESS);
}


BOOLEAN STDCALL
RtlValidSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor)
{
  PSID Owner;
  PSID Group;
  PACL Sacl;
  PACL Dacl;

  if (SecurityDescriptor->Revision != 1)
    {
      return(FALSE);
    }

  Owner = SecurityDescriptor->Owner;
  if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
    {
      Owner = (PSID)((ULONG)Owner + (ULONG)SecurityDescriptor);
    }

  if (!RtlValidSid(Owner))
    {
      return(FALSE);
    }

  Group = SecurityDescriptor->Group;
  if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
    {
      Group = (PSID)((ULONG)Group + (ULONG)SecurityDescriptor);
    }

  if (!RtlValidSid(Group))
    {
      return(FALSE);
    }

  if (SecurityDescriptor->Control & SE_DACL_PRESENT &&
      SecurityDescriptor->Dacl != NULL)
    {
      Dacl = SecurityDescriptor->Dacl;
      if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
	{
	  Dacl = (PACL)((ULONG)Dacl + (ULONG)SecurityDescriptor);
	}

      if (!RtlValidAcl(Dacl))
	{
	  return(FALSE);
	}
    }

  if (SecurityDescriptor->Control & SE_SACL_PRESENT &&
      SecurityDescriptor->Sacl != NULL)
    {
      Sacl = SecurityDescriptor->Sacl;
      if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
	{
	  Sacl = (PACL)((ULONG)Sacl + (ULONG)SecurityDescriptor);
	}

      if (!RtlValidAcl(Sacl))
	{
	  return(FALSE);
	}
    }

  return(TRUE);
}


NTSTATUS STDCALL
RtlSetOwnerSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,
			      PSID Owner,
			      BOOLEAN OwnerDefaulted)
{
   if (SecurityDescriptor->Revision != 1)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   SecurityDescriptor->Owner = Owner;
   SecurityDescriptor->Control = SecurityDescriptor->Control & ~(SE_OWNER_DEFAULTED);
   if (OwnerDefaulted)
     {
	SecurityDescriptor->Control = SecurityDescriptor->Control | SE_OWNER_DEFAULTED;
     }
   return(STATUS_SUCCESS);
}

NTSTATUS STDCALL
RtlGetOwnerSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,
			      PSID* Owner,
			      PBOOLEAN OwnerDefaulted)
{
   if (SecurityDescriptor->Revision != 1)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   if (SecurityDescriptor->Owner != NULL)
     {
	if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
	  {
	     *Owner = (PSID)((ULONG)SecurityDescriptor->Owner +
			     (PVOID)SecurityDescriptor);
	  }
	else
	  {
	     *Owner = SecurityDescriptor->Owner;
	  }
     }
   else
     {
	*Owner = NULL;
     }
   if (SecurityDescriptor->Control & SE_OWNER_DEFAULTED)
     {
	*OwnerDefaulted = 1;
     }
   else
     {
	*OwnerDefaulted = 0;
     }
   return(STATUS_SUCCESS);
}

NTSTATUS STDCALL
RtlSetGroupSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,
			      PSID Group,
			      BOOLEAN GroupDefaulted)
{
   if (SecurityDescriptor->Revision != 1)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   SecurityDescriptor->Group = Group;
   SecurityDescriptor->Control = SecurityDescriptor->Control & ~(SE_GROUP_DEFAULTED);
   if (GroupDefaulted)
     {
	SecurityDescriptor->Control = SecurityDescriptor->Control | SE_GROUP_DEFAULTED;
     }
   return(STATUS_SUCCESS);
}

NTSTATUS STDCALL
RtlGetGroupSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,
			      PSID* Group,
			      PBOOLEAN GroupDefaulted)
{
   if (SecurityDescriptor->Revision != 1)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   if (SecurityDescriptor->Group != NULL)
     {
	if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
	  {
	     *Group = (PSID)((ULONG)SecurityDescriptor->Group +
			     (PVOID)SecurityDescriptor);
	  }
	else
	  {
	     *Group = SecurityDescriptor->Group;
	  }
     }
   else
     {
	*Group = NULL;
     }
   if (SecurityDescriptor->Control & SE_GROUP_DEFAULTED)
     {
	*GroupDefaulted = 1;
     }
   else
     {
	*GroupDefaulted = 0;
     }
   return(STATUS_SUCCESS);
}


static VOID
RtlpQuerySecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,
			    PSID* Owner,
			    PULONG OwnerLength,
			    PSID* Group,
			    PULONG GroupLength,
			    PACL* Dacl,
			    PULONG DaclLength,
			    PACL* Sacl,
			    PULONG SaclLength)
{
  if (SecurityDescriptor->Owner == NULL)
    {
      *Owner = NULL;
    }
  else
    {
      *Owner = SecurityDescriptor->Owner;
      if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
	{
	  *Owner = (PSID)((ULONG)*Owner + (ULONG)SecurityDescriptor);
	}
    }

  if (*Owner != NULL)
    {
      *OwnerLength = (RtlLengthSid(*Owner) + 3) & ~3;
    }
  else
    {
      *OwnerLength = 0;
    }

  if ((SecurityDescriptor->Control & SE_DACL_PRESENT) &&
      SecurityDescriptor->Dacl != NULL)
    {
      *Dacl = SecurityDescriptor->Dacl;
      if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
	{
	  *Dacl = (PACL)((ULONG)*Dacl + (ULONG)SecurityDescriptor);
	}
    }
  else
    {
      *Dacl = NULL;
    }

  if (*Dacl != NULL)
    {
      *DaclLength = ((*Dacl)->AclSize + 3) & ~3;
    }
  else
    {
      *DaclLength = 0;
    }

  if (SecurityDescriptor->Group != NULL)
    {
      *Group = NULL;
    }
  else
    {
      *Group = SecurityDescriptor->Group;
      if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
	{
	  *Group = (PSID)((ULONG)*Group + (ULONG)SecurityDescriptor);
	}
    }

  if (*Group != NULL)
    {
      *GroupLength = (RtlLengthSid(*Group) + 3) & ~3;
    }
  else
    {
      *GroupLength = 0;
    }

  if ((SecurityDescriptor->Control & SE_SACL_PRESENT) &&
      SecurityDescriptor->Sacl != NULL)
    {
      *Sacl = SecurityDescriptor->Sacl;
      if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
	{
	  *Sacl = (PACL)((ULONG)*Sacl + (ULONG)SecurityDescriptor);
	}
    }
  else
    {
      *Sacl = NULL;
    }

  if (*Sacl != NULL)
    {
      *SaclLength = ((*Sacl)->AclSize + 3) & ~3;
    }
}


NTSTATUS STDCALL
RtlMakeSelfRelativeSD(PSECURITY_DESCRIPTOR AbsSD,
		      PSECURITY_DESCRIPTOR RelSD,
		      PULONG BufferLength)
{
  PSID Owner;
  PSID Group;
  PACL Sacl;
  PACL Dacl;
  ULONG OwnerLength;
  ULONG GroupLength;
  ULONG SaclLength;
  ULONG DaclLength;
  ULONG TotalLength;
  ULONG Current;

  RtlpQuerySecurityDescriptor(AbsSD,
			      &Owner,
			      &OwnerLength,
			      &Group,
			      &GroupLength,
			      &Dacl,
			      &DaclLength,
			      &Sacl,
			      &SaclLength);

  TotalLength = OwnerLength + GroupLength + SaclLength + DaclLength + sizeof(SECURITY_DESCRIPTOR);
  if (*BufferLength < TotalLength)
    {
      return(STATUS_BUFFER_TOO_SMALL);
    }

  RtlZeroMemory(RelSD,
		TotalLength);
  memmove(RelSD,
	  AbsSD,
	  sizeof(SECURITY_DESCRIPTOR));
  Current = (ULONG)RelSD + sizeof(SECURITY_DESCRIPTOR);

  if (SaclLength != 0)
    {
      memmove((PVOID)Current,
	      Sacl,
	      SaclLength);
      RelSD->Sacl = (PACL)((ULONG)Current - (ULONG)RelSD);
      Current += SaclLength;
    }

  if (DaclLength != 0)
    {
      memmove((PVOID)Current,
	      Dacl,
	      DaclLength);
      RelSD->Dacl = (PACL)((ULONG)Current - (ULONG)RelSD);
      Current += DaclLength;
    }

  if (OwnerLength != 0)
    {
      memmove((PVOID)Current,
	      Owner,
	      OwnerLength);
      RelSD->Owner = (PSID)((ULONG)Current - (ULONG)RelSD);
      Current += OwnerLength;
    }

  if (GroupLength != 0)
    {
      memmove((PVOID)Current,
	      Group,
	      GroupLength);
      RelSD->Group = (PSID)((ULONG)Current - (ULONG)RelSD);
    }

  RelSD->Control |= SE_SELF_RELATIVE;

  return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
RtlAbsoluteToSelfRelativeSD(PSECURITY_DESCRIPTOR AbsSD,
			    PSECURITY_DESCRIPTOR RelSD,
			    PULONG BufferLength
	)
{
  if (AbsSD->Control & SE_SELF_RELATIVE)
    {
      return(STATUS_BAD_DESCRIPTOR_FORMAT);
    }

  return(RtlMakeSelfRelativeSD(AbsSD, RelSD, BufferLength));
}


NTSTATUS STDCALL
RtlGetControlSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,
				PSECURITY_DESCRIPTOR_CONTROL Control,
				PULONG Revision)
{
  *Revision = SecurityDescriptor->Revision;

  if (SecurityDescriptor->Revision != 1)
    {
      return(STATUS_UNKNOWN_REVISION);
    }

  *Control = SecurityDescriptor->Control;

  return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
RtlGetSaclSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,
			     PBOOLEAN SaclPresent,
			     PACL *Sacl,
			     PBOOLEAN SaclDefaulted)
{
   if (SecurityDescriptor->Revision != 1)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   if (!(SecurityDescriptor->Control & SE_SACL_PRESENT))
     {
	*SaclPresent = 0;
	return(STATUS_SUCCESS);
     }
   *SaclPresent = 1;
   if (SecurityDescriptor->Sacl == NULL)
     {
	*Sacl = NULL;
     }
   else
     {
	if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
	  {
	     *Sacl = (PACL)((ULONG)SecurityDescriptor->Sacl +
			    (PVOID)SecurityDescriptor);
	  }
	else
	  {
	     *Sacl = SecurityDescriptor->Sacl;
	  }
     }
   if (SecurityDescriptor->Control & SE_SACL_DEFAULTED)
     {
	*SaclDefaulted = 1;
     }
   else
     {
	*SaclDefaulted = 0;
     }
   return(STATUS_SUCCESS);
}

NTSTATUS STDCALL
RtlSetSaclSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,
			     BOOLEAN SaclPresent,
			     PACL Sacl,
			     BOOLEAN SaclDefaulted)
{
   if (SecurityDescriptor->Revision != 1)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   if (!SaclPresent)
     {
	SecurityDescriptor->Control = SecurityDescriptor->Control & ~(SE_SACL_PRESENT);
	return(STATUS_SUCCESS);
     }
   SecurityDescriptor->Control = SecurityDescriptor->Control | SE_SACL_PRESENT;
   SecurityDescriptor->Sacl = Sacl;
   SecurityDescriptor->Control = SecurityDescriptor->Control & ~(SE_SACL_DEFAULTED);
   if (SaclDefaulted)
     {
	SecurityDescriptor->Control = SecurityDescriptor->Control | SE_SACL_DEFAULTED;
     }
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
RtlSelfRelativeToAbsoluteSD(PSECURITY_DESCRIPTOR RelSD,
			    PSECURITY_DESCRIPTOR AbsSD,
			    PDWORD AbsSDSize,
			    PACL Dacl,
			    PDWORD DaclSize,
			    PACL Sacl,
			    PDWORD SaclSize,
			    PSID Owner,
			    PDWORD OwnerSize,
			    PSID Group,
			    PDWORD GroupSize)
{
  ULONG TotalSize;
  ULONG OwnerLength;
  ULONG GroupLength;
  ULONG DaclLength;
  ULONG SaclLength;
  PSID pOwner;
  PSID pGroup;
  PACL pDacl;
  PACL pSacl;

  if (!(RelSD->Control & SE_SELF_RELATIVE))
    return STATUS_BAD_DESCRIPTOR_FORMAT;

  RtlpQuerySecurityDescriptor (RelSD,
			       &pOwner,
			       &OwnerLength,
			       &pGroup,
			       &GroupLength,
			       &pDacl,
			       &DaclLength,
			       &pSacl,
			       &SaclLength);

  if (OwnerLength > *OwnerSize ||
      GroupLength > *GroupSize ||
      DaclLength > *DaclSize ||
      SaclLength > *SaclSize)
    return STATUS_BUFFER_TOO_SMALL;

  memmove (Owner, pOwner, OwnerLength);
  memmove (Group, pGroup, GroupLength);
  memmove (Dacl, pDacl, DaclLength);
  memmove (Sacl, pSacl, SaclLength);

  memmove (AbsSD, RelSD, sizeof (SECURITY_DESCRIPTOR));

  AbsSD->Control &= ~SE_SELF_RELATIVE;
  AbsSD->Owner = Owner;
  AbsSD->Group = Group;
  AbsSD->Dacl = Dacl;
  AbsSD->Sacl = Sacl;

  *OwnerSize = OwnerLength;
  *GroupSize = GroupLength;
  *DaclSize = DaclLength;
  *SaclSize = SaclLength;

  return STATUS_SUCCESS;
}

/* EOF */
