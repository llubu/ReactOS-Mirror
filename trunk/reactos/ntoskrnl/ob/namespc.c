/*
 * COPYRIGHT:      See COPYING in the top level directory
 * PROJECT:        ReactOS kernel
 * FILE:           ntoskrnl/ob/namespc.c
 * PURPOSE:        Manages the system namespace
 * PROGRAMMER:     David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                 22/05/98: Created
 */

/* INCLUDES ***************************************************************/

#include <wchar.h>
#include <ddk/ntddk.h>
#include <internal/ob.h>
#include <internal/io.h>
#include <internal/string.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS ****************************************************************/

POBJECT_TYPE ObDirectoryType = NULL;

PDIRECTORY_OBJECT NameSpaceRoot = NULL;

/* FUNCTIONS **************************************************************/

NTSTATUS STDCALL ObReferenceObjectByName(PUNICODE_STRING ObjectPath,
					 ULONG Attributes,
					 PACCESS_STATE PassedAccessState,
					 ACCESS_MASK DesiredAccess,
					 POBJECT_TYPE ObjectType,
					 KPROCESSOR_MODE AccessMode,
					 PVOID ParseContext,
					 PVOID* ObjectPtr)
{
   PVOID Object;
   UNICODE_STRING RemainingPath;
//   PWSTR RemainingPath;
   OBJECT_ATTRIBUTES ObjectAttributes;
   
   InitializeObjectAttributes(&ObjectAttributes,
			      ObjectPath,
			      0,
			      NULL,
			      NULL);
   ObFindObject(&ObjectAttributes,
		&Object,
		&RemainingPath);
   
   if (RemainingPath.Buffer != NULL ||
       Object == NULL)
     {
	*ObjectPtr = NULL;
	RtlFreeUnicodeString (&RemainingPath);
	return(STATUS_UNSUCCESSFUL);
     }
   *ObjectPtr = Object;
   RtlFreeUnicodeString (&RemainingPath);
   return(STATUS_SUCCESS);
}

VOID ObAddEntryDirectory(PDIRECTORY_OBJECT Parent,
			 POBJECT Object,
			 PWSTR Name)
/*
 * FUNCTION: Add an entry to a namespace directory
 * ARGUMENTS:
 *         parent = directory to add in
 *         name = Name to give the entry
 *         Object = Header of the object to add the entry for
 */
{
   KIRQL oldlvl;
   POBJECT_HEADER Header = BODY_TO_HEADER(Object);

   RtlInitUnicodeString(&Header->Name, wcsdup(Name));
   Header->Parent = Parent;

   KeAcquireSpinLock(&Parent->Lock, &oldlvl);
   InsertTailList(&Parent->head, &Header->Entry);
   KeReleaseSpinLock(&Parent->Lock, oldlvl);
}

PVOID ObpFindEntryDirectory(PDIRECTORY_OBJECT DirectoryObject,
			    PWSTR Name,
			    ULONG Attributes)
{
   PLIST_ENTRY current = DirectoryObject->head.Flink;
   POBJECT_HEADER current_obj;

   DPRINT("ObDirLookup(dir %x, name %S)\n",DirectoryObject, Name);
   
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
	DPRINT("Scanning %S %S\n",current_obj->Name.Buffer, Name);
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
   DPRINT("%s() = NULL\n",__FUNCTION__);
   return(NULL);

}

NTSTATUS
ObpParseDirectory (
	PVOID		Object,
	PVOID		* NextObject,
	PUNICODE_STRING	FullPath,
	PWSTR		* Path
	)
{
   PWSTR end;
   PVOID FoundObject;
   
   DPRINT("ObpParseDirectory(Object %x, Path %x, *Path %S)\n",
	  Object,Path,*Path);

   *NextObject = NULL;

   if ((*Path) == NULL)
     {
	return STATUS_UNSUCCESSFUL;
     }
   
   end = wcschr((*Path)+1, '\\');
   if (end != NULL)
     {
	*end = 0;
     }

   FoundObject = ObpFindEntryDirectory(Object, (*Path)+1, 0);
   
   if (FoundObject == NULL)
     {
	if (end != NULL)
	  {
	     *end = '\\';
	  }
	return STATUS_UNSUCCESSFUL;
     }
   
   ObReferenceObjectByPointer(FoundObject,
			      STANDARD_RIGHTS_REQUIRED,
			      NULL,
			      UserMode);
   
   if (end != NULL)
     {
	*end = '\\';
	*Path = end;
     }
   else
     {
	*Path = NULL;
     }

   *NextObject = FoundObject;

   return STATUS_SUCCESS;
}

NTSTATUS ObpCreateDirectory(PVOID ObjectBody,
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
   
   if (Parent != NULL && RemainingPath != NULL)
     {
	ObAddEntryDirectory(Parent, ObjectBody, RemainingPath+1);
     }
   InitializeListHead(&DirectoryObject->head);
   KeInitializeSpinLock(&DirectoryObject->Lock);
   return(STATUS_SUCCESS);
}

VOID ObInit(VOID)
/*
 * FUNCTION: Initialize the object manager namespace
 */
{
   ANSI_STRING AnsiString;
   
   ObDirectoryType = ExAllocatePool(NonPagedPool,sizeof(OBJECT_TYPE));
   
   ObDirectoryType->TotalObjects = 0;
   ObDirectoryType->TotalHandles = 0;
   ObDirectoryType->MaxObjects = ULONG_MAX;
   ObDirectoryType->MaxHandles = ULONG_MAX;
   ObDirectoryType->PagedPoolCharge = 0;
   ObDirectoryType->NonpagedPoolCharge = sizeof(DIRECTORY_OBJECT);
   ObDirectoryType->Dump = NULL;
   ObDirectoryType->Open = NULL;
   ObDirectoryType->Close = NULL;
   ObDirectoryType->Delete = NULL;
   ObDirectoryType->Parse = ObpParseDirectory;
   ObDirectoryType->Security = NULL;
   ObDirectoryType->QueryName = NULL;
   ObDirectoryType->OkayToClose = NULL;
   ObDirectoryType->Create = ObpCreateDirectory;
   
   RtlInitAnsiString(&AnsiString,"Directory");
   RtlAnsiStringToUnicodeString(&ObDirectoryType->TypeName,
				&AnsiString,TRUE);
   
   NameSpaceRoot = ObCreateObject(NULL,
				  STANDARD_RIGHTS_REQUIRED,
				  NULL,
				  ObDirectoryType);
}

VOID ObRemoveEntry(POBJECT_HEADER Header)
{
   KIRQL oldlvl;
   
   DPRINT("ObRemoveEntry(Header %x)\n",Header);
   
   KeAcquireSpinLock(&(Header->Parent->Lock),&oldlvl);
   RemoveEntryList(&(Header->Entry));
   KeReleaseSpinLock(&(Header->Parent->Lock),oldlvl);
}

