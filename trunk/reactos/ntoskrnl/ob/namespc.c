/* $Id$
 *
 * COPYRIGHT:      See COPYING in the top level directory
 * PROJECT:        ReactOS kernel
 * FILE:           ntoskrnl/ob/namespc.c
 * PURPOSE:        Manages the system namespace
 * 
 * PROGRAMMERS:    David Welch (welch@mcmail.com)
 */

/* INCLUDES ***************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>


/* GLOBALS ****************************************************************/

POBJECT_TYPE ObDirectoryType = NULL;
POBJECT_TYPE ObTypeObjectType = NULL;

PDIRECTORY_OBJECT NameSpaceRoot = NULL;
 /* FIXME: Move this somewhere else once devicemap support is in */
PDEVICE_MAP ObSystemDeviceMap = NULL;

static GENERIC_MAPPING ObpDirectoryMapping = {
	STANDARD_RIGHTS_READ|DIRECTORY_QUERY|DIRECTORY_TRAVERSE,
	STANDARD_RIGHTS_WRITE|DIRECTORY_CREATE_OBJECT|DIRECTORY_CREATE_SUBDIRECTORY,
	STANDARD_RIGHTS_EXECUTE|DIRECTORY_QUERY|DIRECTORY_TRAVERSE,
	DIRECTORY_ALL_ACCESS};

static GENERIC_MAPPING ObpTypeMapping = {
	STANDARD_RIGHTS_READ,
	STANDARD_RIGHTS_WRITE,
	STANDARD_RIGHTS_EXECUTE,
	0x000F0001};

/* FUNCTIONS **************************************************************/

/*
 * @implemented
 */
NTSTATUS STDCALL
ObReferenceObjectByName(PUNICODE_STRING ObjectPath,
			ULONG Attributes,
			PACCESS_STATE PassedAccessState,
			ACCESS_MASK DesiredAccess,
			POBJECT_TYPE ObjectType,
			KPROCESSOR_MODE AccessMode,
			PVOID ParseContext,
			PVOID* ObjectPtr)
{
   PVOID Object = NULL;
   UNICODE_STRING RemainingPath;
   OBJECT_ATTRIBUTES ObjectAttributes;
   NTSTATUS Status;

   InitializeObjectAttributes(&ObjectAttributes,
			      ObjectPath,
			      Attributes,
			      NULL,
			      NULL);
   Status = ObFindObject(&ObjectAttributes,
			 &Object,
			 &RemainingPath,
			 ObjectType);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
CHECKPOINT;
DPRINT("RemainingPath.Buffer '%S' Object %p\n", RemainingPath.Buffer, Object);

   if (RemainingPath.Buffer != NULL || Object == NULL)
     {
CHECKPOINT;
DPRINT("Object %p\n", Object);
	*ObjectPtr = NULL;
	RtlFreeUnicodeString (&RemainingPath);
	return(STATUS_OBJECT_NAME_NOT_FOUND);
     }
   *ObjectPtr = Object;
   RtlFreeUnicodeString (&RemainingPath);
   return(STATUS_SUCCESS);
}


/**********************************************************************
 * NAME							EXPORTED
 *	ObOpenObjectByName
 *	
 * DESCRIPTION
 *	Obtain a handle to an existing object.
 *	
 * ARGUMENTS
 *	ObjectAttributes
 *		...
 *	ObjectType
 *		...
 *	ParseContext
 *		...
 *	AccessMode
 *		...
 *	DesiredAccess
 *		...
 *	PassedAccessState
 *		...
 *	Handle
 *		Handle to close.
 *
 * RETURN VALUE
 * 	Status.
 *
 * @implemented
 */
NTSTATUS STDCALL
ObOpenObjectByName(IN POBJECT_ATTRIBUTES ObjectAttributes,
		   IN POBJECT_TYPE ObjectType,
		   IN OUT PVOID ParseContext,
		   IN KPROCESSOR_MODE AccessMode,
		   IN ACCESS_MASK DesiredAccess,
		   IN PACCESS_STATE PassedAccessState,
		   OUT PHANDLE Handle)
{
   UNICODE_STRING RemainingPath;
   PVOID Object = NULL;
   NTSTATUS Status;

   DPRINT("ObOpenObjectByName(...)\n");

   Status = ObFindObject(ObjectAttributes,
			 &Object,
			 &RemainingPath,
			 ObjectType);
   if (!NT_SUCCESS(Status))
     {
	DPRINT("ObFindObject() failed (Status %lx)\n", Status);
	return Status;
     }

   if (RemainingPath.Buffer != NULL ||
       Object == NULL)
     {
	RtlFreeUnicodeString(&RemainingPath);
	return STATUS_UNSUCCESSFUL;
     }

   Status = ObCreateHandle(PsGetCurrentProcess(),
			   Object,
			   DesiredAccess,
			   FALSE,
			   Handle);

   ObDereferenceObject(Object);
   RtlFreeUnicodeString(&RemainingPath);

   return Status;
}

VOID
STDCALL
ObQueryDeviceMapInformation(PEPROCESS Process,
			    PPROCESS_DEVICEMAP_INFORMATION DeviceMapInfo)
{
	//KIRQL OldIrql ;
	
	/*
	 * FIXME: This is an ugly hack for now, to always return the System Device Map
	 * instead of returning the Process Device Map. Not important yet since we don't use it
	 */
	   
	 /* FIXME: Acquire the DeviceMap Spinlock */
	 // KeAcquireSpinLock(DeviceMap->Lock, &OldIrql);
	 
	 /* Make a copy */
	 DeviceMapInfo->Query.DriveMap = ObSystemDeviceMap->DriveMap;
	 RtlMoveMemory(DeviceMapInfo->Query.DriveType, ObSystemDeviceMap->DriveType, sizeof(ObSystemDeviceMap->DriveType));
	 
	 /* FIXME: Release the DeviceMap Spinlock */
	 // KeReleasepinLock(DeviceMap->Lock, OldIrql);
}	 
	 
VOID
ObpAddEntryDirectory(PDIRECTORY_OBJECT Parent,
		     POBJECT_HEADER Header,
		     PWSTR Name)
/*
 * FUNCTION: Add an entry to a namespace directory
 * ARGUMENTS:
 *         Parent = directory to add in
 *         Header = Header of the object to add the entry for
 *         Name = Name to give the entry
 */
{
  KIRQL oldlvl;

  RtlpCreateUnicodeString(&Header->Name, Name, NonPagedPool);
  Header->Parent = Parent;

  KeAcquireSpinLock(&Parent->Lock, &oldlvl);
  InsertTailList(&Parent->head, &Header->Entry);
  KeReleaseSpinLock(&Parent->Lock, oldlvl);
}


VOID
ObpRemoveEntryDirectory(POBJECT_HEADER Header)
/*
 * FUNCTION: Remove an entry from a namespace directory
 * ARGUMENTS:
 *         Header = Header of the object to remove
 */
{
  KIRQL oldlvl;

  DPRINT("ObpRemoveEntryDirectory(Header %x)\n",Header);

  KeAcquireSpinLock(&(Header->Parent->Lock),&oldlvl);
  RemoveEntryList(&(Header->Entry));
  KeReleaseSpinLock(&(Header->Parent->Lock),oldlvl);
}


PVOID
ObpFindEntryDirectory(PDIRECTORY_OBJECT DirectoryObject,
		      PWSTR Name,
		      ULONG Attributes)
{
   PLIST_ENTRY current = DirectoryObject->head.Flink;
   POBJECT_HEADER current_obj;

   DPRINT("ObFindEntryDirectory(dir %x, name %S)\n",DirectoryObject, Name);
   
   if (Name[0]==0)
     {
	return(DirectoryObject);
     }
   if (Name[0]=='.' && Name[1]==0)
     {
	return(DirectoryObject);
     }
   if (Name[0]=='.' && Name[1]=='.' && Name[2]==0)
     {
	return(BODY_TO_HEADER(DirectoryObject)->Parent);
     }
   while (current!=(&(DirectoryObject->head)))
     {
	current_obj = CONTAINING_RECORD(current,OBJECT_HEADER,Entry);
	DPRINT("  Scanning: %S for: %S\n",current_obj->Name.Buffer, Name);
	if (Attributes & OBJ_CASE_INSENSITIVE)
	  {
	     if (_wcsicmp(current_obj->Name.Buffer, Name)==0)
	       {
		  DPRINT("Found it %x\n",HEADER_TO_BODY(current_obj));
		  return(HEADER_TO_BODY(current_obj));
	       }
	  }
	else
	  {
	     if ( wcscmp(current_obj->Name.Buffer, Name)==0)
	       {
		  DPRINT("Found it %x\n",HEADER_TO_BODY(current_obj));
		  return(HEADER_TO_BODY(current_obj));
	       }
	  }
	current = current->Flink;
     }
   DPRINT("    Not Found: %s() = NULL\n",__FUNCTION__);
   return(NULL);
}


NTSTATUS STDCALL
ObpParseDirectory(PVOID Object,
		  PVOID * NextObject,
		  PUNICODE_STRING FullPath,
		  PWSTR * Path,
		  ULONG Attributes)
{
  PWSTR Start;
  PWSTR End;
  PVOID FoundObject;

  DPRINT("ObpParseDirectory(Object %x, Path %x, *Path %S)\n",
	 Object,Path,*Path);

  *NextObject = NULL;

  if ((*Path) == NULL)
    {
      return STATUS_UNSUCCESSFUL;
    }

  Start = *Path;
  if (*Start == L'\\')
    Start++;

  End = wcschr(Start, L'\\');
  if (End != NULL)
    {
      *End = 0;
    }

  FoundObject = ObpFindEntryDirectory(Object, Start, Attributes);
  if (FoundObject == NULL)
    {
      if (End != NULL)
	{
	  *End = L'\\';
	}
      return STATUS_UNSUCCESSFUL;
    }

  ObReferenceObjectByPointer(FoundObject,
			     STANDARD_RIGHTS_REQUIRED,
			     NULL,
			     UserMode);

  if (End != NULL)
    {
      *End = L'\\';
      *Path = End;
    }
  else
    {
      *Path = NULL;
    }

  *NextObject = FoundObject;

  return STATUS_SUCCESS;
}


NTSTATUS STDCALL
ObpCreateDirectory(PVOID ObjectBody,
		   PVOID Parent,
		   PWSTR RemainingPath,
		   POBJECT_ATTRIBUTES ObjectAttributes)
{
  PDIRECTORY_OBJECT DirectoryObject = (PDIRECTORY_OBJECT)ObjectBody;

  DPRINT("ObpCreateDirectory(ObjectBody %x, Parent %x, RemainingPath %S)\n",
	 ObjectBody, Parent, RemainingPath);

  if (RemainingPath != NULL && wcschr(RemainingPath+1, '\\') != NULL)
    {
      return(STATUS_UNSUCCESSFUL);
    }

  InitializeListHead(&DirectoryObject->head);
  KeInitializeSpinLock(&DirectoryObject->Lock);

  return(STATUS_SUCCESS);
}


VOID INIT_FUNCTION
ObInit(VOID)
/*
 * FUNCTION: Initialize the object manager namespace
 */
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING Name;
  SECURITY_DESCRIPTOR SecurityDescriptor;

  /* Initialize the security descriptor cache */
  ObpInitSdCache();

  /* create 'directory' object type */
  ObDirectoryType = ExAllocatePool(NonPagedPool,sizeof(OBJECT_TYPE));
  
  ObDirectoryType->Tag = TAG('D', 'I', 'R', 'T');
  ObDirectoryType->TotalObjects = 0;
  ObDirectoryType->TotalHandles = 0;
  ObDirectoryType->PeakObjects = 0;
  ObDirectoryType->PeakHandles = 0;
  ObDirectoryType->PagedPoolCharge = 0;
  ObDirectoryType->NonpagedPoolCharge = sizeof(DIRECTORY_OBJECT);
  ObDirectoryType->Mapping = &ObpDirectoryMapping;
  ObDirectoryType->Dump = NULL;
  ObDirectoryType->Open = NULL;
  ObDirectoryType->Close = NULL;
  ObDirectoryType->Delete = NULL;
  ObDirectoryType->Parse = ObpParseDirectory;
  ObDirectoryType->Security = NULL;
  ObDirectoryType->QueryName = NULL;
  ObDirectoryType->OkayToClose = NULL;
  ObDirectoryType->Create = ObpCreateDirectory;
  ObDirectoryType->DuplicationNotify = NULL;

  RtlInitUnicodeString(&ObDirectoryType->TypeName,
		       L"Directory");

  /* create 'type' object type*/
  ObTypeObjectType = ExAllocatePool(NonPagedPool,sizeof(OBJECT_TYPE));
  
  ObTypeObjectType->Tag = TAG('T', 'y', 'p', 'T');
  ObTypeObjectType->TotalObjects = 0;
  ObTypeObjectType->TotalHandles = 0;
  ObTypeObjectType->PeakObjects = 0;
  ObTypeObjectType->PeakHandles = 0;
  ObTypeObjectType->PagedPoolCharge = 0;
  ObTypeObjectType->NonpagedPoolCharge = sizeof(TYPE_OBJECT);
  ObTypeObjectType->Mapping = &ObpTypeMapping;
  ObTypeObjectType->Dump = NULL;
  ObTypeObjectType->Open = NULL;
  ObTypeObjectType->Close = NULL;
  ObTypeObjectType->Delete = NULL;
  ObTypeObjectType->Parse = NULL;
  ObTypeObjectType->Security = NULL;
  ObTypeObjectType->QueryName = NULL;
  ObTypeObjectType->OkayToClose = NULL;
  ObTypeObjectType->Create = NULL;
  ObTypeObjectType->DuplicationNotify = NULL;

  RtlInitUnicodeString(&ObTypeObjectType->TypeName,
		       L"ObjectType");

  /* Create security descriptor */
  RtlCreateSecurityDescriptor(&SecurityDescriptor,
			      SECURITY_DESCRIPTOR_REVISION1);

  RtlSetOwnerSecurityDescriptor(&SecurityDescriptor,
				SeAliasAdminsSid,
				FALSE);

  RtlSetGroupSecurityDescriptor(&SecurityDescriptor,
				SeLocalSystemSid,
				FALSE);

  RtlSetDaclSecurityDescriptor(&SecurityDescriptor,
			       TRUE,
			       SePublicDefaultDacl,
			       FALSE);

  /* Create root directory */
  InitializeObjectAttributes(&ObjectAttributes,
			     NULL,
			     OBJ_PERMANENT,
			     NULL,
			     &SecurityDescriptor);
  ObCreateObject(KernelMode,
		 ObDirectoryType,
		 &ObjectAttributes,
		 KernelMode,
		 NULL,
		 sizeof(DIRECTORY_OBJECT),
		 0,
		 0,
		 (PVOID*)&NameSpaceRoot);

  /* Create '\ObjectTypes' directory */
  RtlRosInitUnicodeStringFromLiteral(&Name,
		       L"\\ObjectTypes");
  InitializeObjectAttributes(&ObjectAttributes,
			     &Name,
			     OBJ_PERMANENT,
			     NULL,
			     &SecurityDescriptor);
  ObCreateObject(KernelMode,
		 ObDirectoryType,
		 &ObjectAttributes,
		 KernelMode,
		 NULL,
		 sizeof(DIRECTORY_OBJECT),
		 0,
		 0,
		 NULL);

  ObpCreateTypeObject(ObDirectoryType);
  ObpCreateTypeObject(ObTypeObjectType);

  /* Create 'symbolic link' object type */
  ObInitSymbolicLinkImplementation();
  
  /* FIXME: Hack Hack! */
  ObSystemDeviceMap = ExAllocatePoolWithTag(NonPagedPool, sizeof(*ObSystemDeviceMap), TAG('O', 'b', 'D', 'm'));
  RtlZeroMemory(ObSystemDeviceMap, sizeof(*ObSystemDeviceMap));
}


NTSTATUS
ObpCreateTypeObject(POBJECT_TYPE ObjectType)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  WCHAR NameString[120];
  PTYPE_OBJECT TypeObject = NULL;
  UNICODE_STRING Name;
  NTSTATUS Status;

  DPRINT("ObpCreateTypeObject(ObjectType: %wZ)\n", &ObjectType->TypeName);
  wcscpy(NameString, L"\\ObjectTypes\\");
  wcscat(NameString, ObjectType->TypeName.Buffer);
  RtlInitUnicodeString(&Name,
		       NameString);

  InitializeObjectAttributes(&ObjectAttributes,
			     &Name,
			     OBJ_PERMANENT,
			     NULL,
			     NULL);
  Status = ObCreateObject(KernelMode,
			  ObTypeObjectType,
			  &ObjectAttributes,
			  KernelMode,
			  NULL,
			  sizeof(TYPE_OBJECT),
			  0,
			  0,
			  (PVOID*)&TypeObject);
  if (NT_SUCCESS(Status))
    {
      TypeObject->ObjectType = ObjectType;
    }

  return(STATUS_SUCCESS);
}

/* EOF */
