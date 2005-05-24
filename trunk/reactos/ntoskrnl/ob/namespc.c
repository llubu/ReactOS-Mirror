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
PDIRECTORY_OBJECT ObpTypeDirectoryObject = NULL;
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

NTSTATUS
STDCALL
ObpAllocateObject(POBJECT_CREATE_INFORMATION ObjectCreateInfo,
                  PUNICODE_STRING ObjectName,
                  POBJECT_TYPE ObjectType,
                  ULONG ObjectSize,
                  POBJECT_HEADER *ObjectHeader);

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
   UNICODE_STRING ObjectName;
   OBJECT_ATTRIBUTES ObjectAttributes;
   OBJECT_CREATE_INFORMATION ObjectCreateInfo;
   NTSTATUS Status;

   PAGED_CODE();

   InitializeObjectAttributes(&ObjectAttributes,
			      ObjectPath,
			      Attributes | OBJ_OPENIF,
			      NULL,
			      NULL);
    
   /* Capture all the info */
   DPRINT("Capturing Create Info\n");
   Status = ObpCaptureObjectAttributes(&ObjectAttributes,
                                       AccessMode,
                                       ObjectType,
                                       &ObjectCreateInfo,
                                       &ObjectName);
   if (!NT_SUCCESS(Status))
     {
	DPRINT("ObpCaptureObjectAttributes() failed (Status %lx)\n", Status);
	return Status;
     }
     
   Status = ObFindObject(&ObjectCreateInfo,
                         &ObjectName,
			 &Object,
			 &RemainingPath,
			 ObjectType);

   ObpReleaseCapturedAttributes(&ObjectCreateInfo);
   if (ObjectName.Buffer) ExFreePool(ObjectName.Buffer);

   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   DPRINT("RemainingPath.Buffer '%S' Object %p\n", RemainingPath.Buffer, Object);

   if (RemainingPath.Buffer != NULL || Object == NULL)
     {
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
   UNICODE_STRING ObjectName;
   OBJECT_CREATE_INFORMATION ObjectCreateInfo;
   NTSTATUS Status;

   PAGED_CODE();

   DPRINT("ObOpenObjectByName(...)\n");

    /* Capture all the info */
    DPRINT("Capturing Create Info\n");
    Status = ObpCaptureObjectAttributes(ObjectAttributes,
                                        AccessMode,
                                        ObjectType,
                                        &ObjectCreateInfo,
                                        &ObjectName);
   if (!NT_SUCCESS(Status))
     {
	DPRINT("ObpCaptureObjectAttributes() failed (Status %lx)\n", Status);
	return Status;
     }
                                        
   Status = ObFindObject(&ObjectCreateInfo,
                         &ObjectName,
			 &Object,
			 &RemainingPath,
			 ObjectType);
   ObpReleaseCapturedAttributes(&ObjectCreateInfo);
   if (ObjectName.Buffer) ExFreePool(ObjectName.Buffer);
   if (!NT_SUCCESS(Status))
     {
	DPRINT("ObFindObject() failed (Status %lx)\n", Status);
	return Status;
     }

   DPRINT("OBject: %x, Remaining Path: %wZ\n", Object, &RemainingPath);
   if (Object == NULL)
     {
       RtlFreeUnicodeString(&RemainingPath);
       return STATUS_UNSUCCESSFUL;
     }
   if (RemainingPath.Buffer != NULL)
   {
      if (wcschr(RemainingPath.Buffer + 1, L'\\') == NULL)
         Status = STATUS_OBJECT_NAME_NOT_FOUND;
      else
         Status =STATUS_OBJECT_PATH_NOT_FOUND;
      RtlFreeUnicodeString(&RemainingPath);
      ObDereferenceObject(Object);
      return Status;
   }
   
   Status = ObpCreateHandle(PsGetCurrentProcess(),
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

  ASSERT(HEADER_TO_OBJECT_NAME(Header));
  HEADER_TO_OBJECT_NAME(Header)->Directory = Parent;

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

  KeAcquireSpinLock(&(HEADER_TO_OBJECT_NAME(Header)->Directory->Lock),&oldlvl);
  if (Header->Entry.Flink && Header->Entry.Blink)
  {
    RemoveEntryList(&(Header->Entry));
    Header->Entry.Flink = Header->Entry.Blink = NULL;
  }
  KeReleaseSpinLock(&(HEADER_TO_OBJECT_NAME(Header)->Directory->Lock),oldlvl);
}

NTSTATUS
STDCALL
ObpCreateDirectory(OB_OPEN_REASON Reason,
                   PVOID ObjectBody,
                   PEPROCESS Process,
                   ULONG HandleCount,
                   ACCESS_MASK GrantedAccess)
{
    PDIRECTORY_OBJECT Directory = ObjectBody;
    
    if (Reason == ObCreateHandle)
    {
        InitializeListHead(&Directory->head);
        KeInitializeSpinLock(&Directory->Lock);
    }
    
    return STATUS_SUCCESS;
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
	return(HEADER_TO_OBJECT_NAME(BODY_TO_HEADER(DirectoryObject))->Directory);
     }
   while (current!=(&(DirectoryObject->head)))
     {
	current_obj = CONTAINING_RECORD(current,OBJECT_HEADER,Entry);
	DPRINT("  Scanning: %S for: %S\n",HEADER_TO_OBJECT_NAME(current_obj)->Name.Buffer, Name);
	if (Attributes & OBJ_CASE_INSENSITIVE)
	  {
	     if (_wcsicmp(HEADER_TO_OBJECT_NAME(current_obj)->Name.Buffer, Name)==0)
	       {
		  DPRINT("Found it %x\n",&current_obj->Body);
		  return(&current_obj->Body);
	       }
	  }
	else
	  {
	     if ( wcscmp(HEADER_TO_OBJECT_NAME(current_obj)->Name.Buffer, Name)==0)
	       {
		  DPRINT("Found it %x\n",&current_obj->Body);
		  return(&current_obj->Body);
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

VOID 
INIT_FUNCTION
ObInit(VOID)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING Name;
    SECURITY_DESCRIPTOR SecurityDescriptor;
    OBJECT_TYPE_INITIALIZER ObjectTypeInitializer;

    /* Initialize the security descriptor cache */
    ObpInitSdCache();

    /* Create the Type Type */
    DPRINT("Creating Type Type\n");
    RtlZeroMemory(&ObjectTypeInitializer, sizeof(ObjectTypeInitializer));
    RtlInitUnicodeString(&Name, L"Type");
    ObjectTypeInitializer.Length = sizeof(ObjectTypeInitializer);
    ObjectTypeInitializer.ValidAccessMask = OBJECT_TYPE_ALL_ACCESS;
    ObjectTypeInitializer.UseDefaultObject = TRUE;
    ObjectTypeInitializer.MaintainTypeList = TRUE;
    ObjectTypeInitializer.PoolType = NonPagedPool;
    ObjectTypeInitializer.GenericMapping = ObpTypeMapping;
    ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof(OBJECT_TYPE);
    ObpCreateTypeObject(&ObjectTypeInitializer, &Name, &ObTypeObjectType);
  
    /* Create the Directory Type */
    DPRINT("Creating Directory Type\n");
    RtlZeroMemory(&ObjectTypeInitializer, sizeof(ObjectTypeInitializer));
    RtlInitUnicodeString(&Name, L"Directory");
    ObjectTypeInitializer.Length = sizeof(ObjectTypeInitializer);
    ObjectTypeInitializer.ValidAccessMask = DIRECTORY_ALL_ACCESS;
    ObjectTypeInitializer.UseDefaultObject = FALSE;
    ObjectTypeInitializer.OpenProcedure = ObpCreateDirectory;
    ObjectTypeInitializer.ParseProcedure = ObpParseDirectory;
    ObjectTypeInitializer.MaintainTypeList = FALSE;
    ObjectTypeInitializer.GenericMapping = ObpDirectoryMapping;
    ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof(DIRECTORY_OBJECT);
    ObpCreateTypeObject(&ObjectTypeInitializer, &Name, &ObDirectoryType);

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
    DPRINT("Creating Root Directory\n");    
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
    ObInsertObject((PVOID)NameSpaceRoot,
                   NULL,
                   DIRECTORY_ALL_ACCESS,
                   0,
                   NULL,
                   NULL);

    /* Create '\ObjectTypes' directory */
    RtlInitUnicodeString(&Name, L"\\ObjectTypes");
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
                   (PVOID*)&ObpTypeDirectoryObject);
    ObInsertObject((PVOID)ObpTypeDirectoryObject,
                   NULL,
                   DIRECTORY_ALL_ACCESS,
                   0,
                   NULL,
                   NULL);
    
    /* Insert the two objects we already created but couldn't add */
    /* NOTE: Uses TypeList & Creator Info in OB 2.0 */
    ObpAddEntryDirectory(ObpTypeDirectoryObject, BODY_TO_HEADER(ObTypeObjectType), NULL);
    ObpAddEntryDirectory(ObpTypeDirectoryObject, BODY_TO_HEADER(ObDirectoryType), NULL);

    /* Create 'symbolic link' object type */
    ObInitSymbolicLinkImplementation();

    /* FIXME: Hack Hack! */
    ObSystemDeviceMap = ExAllocatePoolWithTag(NonPagedPool, sizeof(*ObSystemDeviceMap), TAG('O', 'b', 'D', 'm'));
    RtlZeroMemory(ObSystemDeviceMap, sizeof(*ObSystemDeviceMap));
}

NTSTATUS
STDCALL
ObpCreateTypeObject(POBJECT_TYPE_INITIALIZER ObjectTypeInitializer,
                    PUNICODE_STRING TypeName,
                    POBJECT_TYPE *ObjectType)
{
    POBJECT_HEADER Header;
    POBJECT_TYPE LocalObjectType;
    NTSTATUS Status;

    DPRINT("ObpCreateTypeObject(ObjectType: %wZ)\n", TypeName);
    
    /* Allocate the Object */
    Status = ObpAllocateObject(NULL, 
                               TypeName,
                               ObTypeObjectType, 
                               OBJECT_ALLOC_SIZE(sizeof(OBJECT_TYPE)),
                               &Header);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ObpAllocateObject failed!\n");
        return Status;
    }
    
    LocalObjectType = (POBJECT_TYPE)&Header->Body;
    DPRINT("Local ObjectType: %p Header: %p \n", LocalObjectType, Header);
    
    /* Check if this is the first Object Type */
    if (!ObTypeObjectType)
    {
        ObTypeObjectType = LocalObjectType;
        Header->Type = ObTypeObjectType;
        LocalObjectType->Key = TAG('O', 'b', 'j', 'T');
    }
    else
    {   
        #if 0
        ANSI_STRING Tag;
        ULONG i;
        
        DPRINT1("Convert: %wZ \n", TypeName);
        Status = RtlUnicodeStringToAnsiString(&Tag, TypeName, TRUE);
        DPRINT1("Convert done\n");
        if (NT_SUCCESS(Status))
        {
            /* Add spaces if needed */
            for (i = 3; i >= Tag.Length; i--) Tag.Buffer[i] = ' ';
            
            /* Use the first four letters */
            LocalObjectType->Key = *(PULONG)Tag.Buffer;
            ExFreePool(Tag.Buffer);
        }
        else
        #endif
        {
            /* Some weird problem. Use Unicode name */
            LocalObjectType->Key = *(PULONG)TypeName->Buffer;
            Status = STATUS_SUCCESS;
        }
    }
    
    /* Set it up */
    LocalObjectType->TypeInfo = *ObjectTypeInitializer;
    LocalObjectType->Name = *TypeName;
    
    /* Insert it into the Object Directory */
    if (ObpTypeDirectoryObject)
    {
        ObpAddEntryDirectory(ObpTypeDirectoryObject, Header, TypeName->Buffer);
        ObReferenceObject(ObpTypeDirectoryObject);
    }
        
    *ObjectType = LocalObjectType;
    return Status;
} 

/* EOF */
