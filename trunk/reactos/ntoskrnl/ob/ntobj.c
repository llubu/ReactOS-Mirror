/* $Id: ntobj.c,v 1.18 2003/11/16 13:47:20 ekohl Exp $
 *
 * COPYRIGHT:     See COPYING in the top level directory
 * PROJECT:       ReactOS kernel
 * FILE:          ntoskrnl/ob/ntobj.c
 * PURPOSE:       User mode interface to object manager
 * PROGRAMMER:    David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *               10/06/98: Created
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/ob.h>
#include <internal/id.h>

#define NDEBUG
#include <internal/debug.h>


/* FUNCTIONS ************************************************************/

/**********************************************************************
 * NAME							EXPORTED
 *	NtSetInformationObject
 *	
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 */
NTSTATUS STDCALL
NtSetInformationObject (IN HANDLE ObjectHandle,
			IN OBJECT_INFORMATION_CLASS ObjectInformationClass,
			IN PVOID ObjectInformation,
			IN ULONG Length)
{
  PVOID Object;
  NTSTATUS Status;

  if (ObjectInformationClass != ObjectHandleInformation)
    return STATUS_INVALID_INFO_CLASS;

  if (Length != sizeof (OBJECT_HANDLE_ATTRIBUTE_INFORMATION))
    return STATUS_INFO_LENGTH_MISMATCH;

  Status = ObReferenceObjectByHandle (ObjectHandle,
				      0,
				      NULL,
				      KeGetPreviousMode (),
				      &Object,
				      NULL);
  if (!NT_SUCCESS (Status))
    {
      return Status;
    }

  Status = ObpSetHandleAttributes (ObjectHandle,
				   (POBJECT_HANDLE_ATTRIBUTE_INFORMATION)ObjectInformation);

  ObDereferenceObject (Object);

  return Status;
}


/**********************************************************************
 * NAME							EXPORTED
 *	NtQueryObject
 *	
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 */
NTSTATUS STDCALL
NtQueryObject (IN HANDLE ObjectHandle,
	       IN OBJECT_INFORMATION_CLASS ObjectInformationClass,
	       OUT PVOID ObjectInformation,
	       IN ULONG Length,
	       OUT PULONG ReturnLength OPTIONAL)
{
  OBJECT_HANDLE_INFORMATION HandleInfo;
  POBJECT_HEADER ObjectHeader;
  ULONG InfoLength;
  PVOID Object;
  NTSTATUS Status;

  Status = ObReferenceObjectByHandle (ObjectHandle,
				      0,
				      NULL,
				      KeGetPreviousMode(),
				      &Object,
				      &HandleInfo);
  if (!NT_SUCCESS (Status))
    {
      return Status;
    }

  ObjectHeader = BODY_TO_HEADER(Object);

  switch (ObjectInformationClass)
    {
      case ObjectBasicInformation:
	InfoLength = sizeof(OBJECT_BASIC_INFORMATION);
	if (Length != sizeof(OBJECT_BASIC_INFORMATION))
	  {
	    Status = STATUS_INFO_LENGTH_MISMATCH;
	  }
	else
	  {
	    POBJECT_BASIC_INFORMATION BasicInfo;

	    BasicInfo = (POBJECT_BASIC_INFORMATION)ObjectInformation;
	    BasicInfo->Attributes = HandleInfo.HandleAttributes;
	    BasicInfo->GrantedAccess = HandleInfo.GrantedAccess;
	    BasicInfo->HandleCount = ObjectHeader->HandleCount;
	    BasicInfo->PointerCount = ObjectHeader->RefCount;
	    BasicInfo->PagedPoolUsage = 0; /* FIXME*/
	    BasicInfo->NonPagedPoolUsage = 0; /* FIXME*/
	    BasicInfo->NameInformationLength = 0; /* FIXME*/
	    BasicInfo->TypeInformationLength = 0; /* FIXME*/
	    BasicInfo->SecurityDescriptorLength = 0; /* FIXME*/
	    if (ObjectHeader->ObjectType == ObSymbolicLinkType)
	      {
		BasicInfo->CreateTime.QuadPart =
		  ((PSYMLINK_OBJECT)Object)->CreateTime.QuadPart;
	      }
	    else
	      {
		BasicInfo->CreateTime.QuadPart = 0ULL;
	      }
	    Status = STATUS_SUCCESS;
	  }
	break;

      case ObjectNameInformation:
	Status = ObQueryNameString (Object,
				    (POBJECT_NAME_INFORMATION)ObjectInformation,
				    Length,
				    &InfoLength);
	break;

      case ObjectTypeInformation:
#if 0
//	InfoLength =
	if (Length != sizeof(OBJECT_TYPE_INFORMATION))
	  {
	    Status = STATUS_INVALID_BUFFER_SIZE;
	  }
	else
	  {
	    POBJECT_TYPE_INFORMATION TypeInfo;

	    TypeInfo = (POBJECT_TYPE_INFORMATION)ObjectInformation;
	// FIXME: Is this supposed to only be the header's Name field?
	// Can somebody check/verify this?
	RtlCopyUnicodeString(&typeinfo->Name,&ObjectHeader->Name);

	if (Status != STATUS_SUCCESS)
	  {
	    break;
	  }

	RtlCopyUnicodeString(&typeinfo->Type,&ObjectHeader->ObjectType->TypeName);
	//This should be info from the object header, not the object type, right?
	typeinfo->TotalHandles = ObjectHeader-> HandleCount;
	typeinfo->ReferenceCount = ObjectHeader -> RefCount;
	  }
#endif
	Status = STATUS_NOT_IMPLEMENTED;
	break;

      case ObjectAllTypesInformation:
	Status = STATUS_NOT_IMPLEMENTED;
	break;

      case ObjectHandleInformation:
	InfoLength = sizeof (OBJECT_HANDLE_ATTRIBUTE_INFORMATION);
	if (Length != sizeof (OBJECT_HANDLE_ATTRIBUTE_INFORMATION))
	  {
	    Status = STATUS_INFO_LENGTH_MISMATCH;
	  }
	else
	  {
	    Status = ObpQueryHandleAttributes (ObjectHandle,
					       (POBJECT_HANDLE_ATTRIBUTE_INFORMATION)ObjectInformation);
	  }
	break;

      default:
	Status = STATUS_INVALID_INFO_CLASS;
	break;
    }

  ObDereferenceObject (Object);

  if (ReturnLength != NULL)
    *ReturnLength = InfoLength;

  return Status;
}


/**********************************************************************
 * NAME							EXPORTED
 *	ObMakeTemporaryObject
 *	
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 *
 * @implemented
 */
VOID STDCALL
ObMakeTemporaryObject (IN PVOID ObjectBody)
{
  POBJECT_HEADER ObjectHeader;

  ObjectHeader = BODY_TO_HEADER(ObjectBody);
  ObjectHeader->Permanent = FALSE;
}


/**********************************************************************
 * NAME							EXPORTED
 *	NtMakeTemporaryObject
 *	
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 */
NTSTATUS STDCALL
NtMakeTemporaryObject (IN HANDLE Handle)
{
  POBJECT_HEADER ObjectHeader;
  PVOID Object;
  NTSTATUS Status;

  Status = ObReferenceObjectByHandle(Handle,
				     0,
				     NULL,
				     KeGetPreviousMode(),
				     & Object,
				     NULL);
  if (Status != STATUS_SUCCESS)
    {
      return Status;
    }

  ObjectHeader = BODY_TO_HEADER(Object);
  ObjectHeader->Permanent = FALSE;

  ObDereferenceObject(Object);

  return STATUS_SUCCESS;
}

/* EOF */
