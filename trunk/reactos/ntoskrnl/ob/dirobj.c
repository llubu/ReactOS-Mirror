/*
 * COPYRIGHT:      See COPYING in the top level directory
 * PROJECT:        ReactOS kernel
 * FILE:           ntoskrnl/ob/dirobj.c
 * PURPOSE:        Interface functions to directory object
 * PROGRAMMER:     David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                 22/05/98: Created
 */

/* INCLUDES ***************************************************************/

#include <windows.h>
#include <wstring.h>
#include <ddk/ntddk.h>
#include <internal/ob.h>
#include <internal/io.h>
#include <internal/string.h>

#define NDEBUG
#include <internal/debug.h>


/* FUNCTIONS **************************************************************/

NTSTATUS NtOpenDirectoryObject(PHANDLE DirectoryHandle,
			       ACCESS_MASK DesiredAccess,
			       POBJECT_ATTRIBUTES ObjectAttributes)
{
   return(ZwOpenDirectoryObject(DirectoryHandle,
				DesiredAccess,
				ObjectAttributes));
}

NTSTATUS ZwOpenDirectoryObject(PHANDLE DirectoryHandle,
			       ACCESS_MASK DesiredAccess,
			       POBJECT_ATTRIBUTES ObjectAttributes)
/*
 * FUNCTION: Opens a namespace directory object
 * ARGUMENTS:
 *       DirectoryHandle (OUT) = Variable which receives the directory handle
 *       DesiredAccess = Desired access to the directory
 *       ObjectAttributes = Structure describing the directory
 * RETURNS: Status
 * NOTES: Undocumented
 */
{
   PVOID Object;
   NTSTATUS Status;
   
   *DirectoryHandle = 0;
   
   Status = ObReferenceObjectByName(ObjectAttributes->ObjectName,
				    ObjectAttributes->Attributes,
				    NULL,
				    DesiredAccess,
				    ObDirectoryType,
				    UserMode,
				    NULL,
				    &Object);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
       
   *DirectoryHandle = ObInsertHandle(KeGetCurrentProcess(),Object,
				     DesiredAccess,FALSE);
   return(STATUS_SUCCESS);
}

NTSTATUS NtQueryDirectoryObject(IN HANDLE DirObjHandle,
				OUT POBJDIR_INFORMATION DirObjInformation, 
				IN ULONG                BufferLength, 
				IN BOOLEAN              GetNextIndex, 
				IN BOOLEAN              IgnoreInputIndex, 
				IN OUT PULONG           ObjectIndex,
				OUT PULONG              DataWritten OPTIONAL)
{
   return(ZwQueryDirectoryObject(DirObjHandle,
				 DirObjInformation,
				 BufferLength,
				 GetNextIndex,
				 IgnoreInputIndex,
				 ObjectIndex,
				 DataWritten));
}

NTSTATUS ZwQueryDirectoryObject(IN HANDLE DirObjHandle,
				OUT POBJDIR_INFORMATION DirObjInformation, 
				IN ULONG                BufferLength, 
				IN BOOLEAN              GetNextIndex, 
				IN BOOLEAN              IgnoreInputIndex, 
				IN OUT PULONG           ObjectIndex,
				OUT PULONG              DataWritten OPTIONAL)
/*
 * FUNCTION: Reads information from a namespace directory
 * ARGUMENTS:
 *        DirObjInformation (OUT) = Buffer to hold the data read
 *        BufferLength = Size of the buffer in bytes
 *        GetNextIndex = If TRUE then set ObjectIndex to the index of the
 *                       next object
 *                       If FALSE then set ObjectIndex to the number of
 *                       objects in the directory
 *        IgnoreInputIndex = If TRUE start reading at index 0
 *                           If FALSE start reading at the index specified
 *                           by object index
 *        ObjectIndex = Zero based index into the directory, interpretation
 *                      depends on IgnoreInputIndex and GetNextIndex
 *        DataWritten (OUT) = Caller supplied storage for the number of bytes
 *                            written (or NULL)
 * RETURNS: Status
 */
{
   PDIRECTORY_OBJECT dir = NULL;
   ULONG EntriesToRead;
   PLIST_ENTRY current_entry;
   POBJECT_HEADER current;
   ULONG i=0;
   ULONG EntriesToSkip;
   NTSTATUS Status;
   
   DPRINT("ZwQueryDirectoryObject(DirObjHandle %x)\n",DirObjHandle);
   DPRINT("dir %x namespc_root %x\n",dir,HEADER_TO_BODY(&(namespc_root.hdr)));
   
//   assert_irql(PASSIVE_LEVEL);
   
   Status = ObReferenceObjectByHandle(DirObjHandle,
				      DIRECTORY_QUERY,
				      ObDirectoryType,
				      UserMode,
				      (PVOID*)&dir,
				      NULL);
   if (Status != STATUS_SUCCESS)
     {
	return(Status);
     }
   
   EntriesToRead = BufferLength / sizeof(OBJDIR_INFORMATION);
   *DataWritten = 0;
   
   DPRINT("EntriesToRead %d\n",EntriesToRead);
   
   current_entry = dir->head.Flink;
   
   /*
    * Optionally, skip over some entries at the start of the directory
    */
   if (!IgnoreInputIndex)
     {
	CHECKPOINT;
	
	EntriesToSkip = *ObjectIndex;
	while ( i<EntriesToSkip && current_entry!=NULL)
	  {
	     current_entry = current_entry->Flink;
	  }
     }
   
   DPRINT("DirObjInformation %x\n",DirObjInformation);
   
   /*
    * Read the maximum entries possible into the buffer
    */
   while ( i<EntriesToRead && current_entry!=(&(dir->head)))
     {
	current = CONTAINING_RECORD(current_entry,OBJECT_HEADER,Entry);
	DPRINT("Scanning %w\n",current->Name.Buffer);
	DirObjInformation[i].ObjectName.Buffer = 
	               ExAllocatePool(NonPagedPool,(current->Name.Length+1)*2);
	DirObjInformation[i].ObjectName.Length = current->Name.Length;
	DirObjInformation[i].ObjectName.MaximumLength = current->Name.Length;
	DPRINT("DirObjInformation[i].ObjectName.Buffer %x\n",
	       DirObjInformation[i].ObjectName.Buffer);
	RtlCopyUnicodeString(&DirObjInformation[i].ObjectName,
			     &(current->Name));
	i++;
	current_entry = current_entry->Flink;
	(*DataWritten) = (*DataWritten) + sizeof(OBJDIR_INFORMATION);
	CHECKPOINT;
     }
   CHECKPOINT;
   
   /*
    * Optionally, count the number of entries in the directory
    */
   if (GetNextIndex)
     {
	*ObjectIndex=i;
     }
   else
     {
	while ( current_entry!=(&(dir->head)) )
	  {
	     current_entry=current_entry->Flink;
	     i++;
	  }
	*ObjectIndex=i;
     }
   return(STATUS_SUCCESS);
}


NTSTATUS NtCreateDirectoryObject(PHANDLE DirectoryHandle,
				 ACCESS_MASK DesiredAccess,
				 POBJECT_ATTRIBUTES ObjectAttributes)
{
   return(ZwCreateDirectoryObject(DirectoryHandle,
				  DesiredAccess,
				  ObjectAttributes));
}

NTSTATUS ZwCreateDirectoryObject(PHANDLE DirectoryHandle,
				 ACCESS_MASK DesiredAccess,
				 POBJECT_ATTRIBUTES ObjectAttributes)
/*
 * FUNCTION: Creates or opens a directory object (a container for other
 * objects)
 * ARGUMENTS:
 *        DirectoryHandle (OUT) = Caller supplied storage for the handle
 *                                of the directory
 *        DesiredAccess = Access desired to the directory
 *        ObjectAttributes = Object attributes initialized with
 *                           InitializeObjectAttributes
 * RETURNS: Status
 */
{
   PDIRECTORY_OBJECT dir;
   
   dir = ObCreateObject(DirectoryHandle,
			DesiredAccess,
			ObjectAttributes,
			ObDirectoryType);
   return(STATUS_SUCCESS);
}

VOID InitializeObjectAttributes(POBJECT_ATTRIBUTES InitializedAttributes,
				PUNICODE_STRING ObjectName,
				ULONG Attributes,
				HANDLE RootDirectory,
				PSECURITY_DESCRIPTOR SecurityDescriptor)
/*
 * FUNCTION: Sets up a parameter of type OBJECT_ATTRIBUTES for a 
 * subsequent call to ZwCreateXXX or ZwOpenXXX
 * ARGUMENTS:
 *        InitializedAttributes (OUT) = Caller supplied storage for the
 *                                      object attributes
 *        ObjectName = Full path name for object
 *        Attributes = Attributes for the object
 *        RootDirectory = Where the object should be placed or NULL
 *        SecurityDescriptor = Ignored
 * 
 * NOTE:
 *     Either ObjectName is a fully qualified pathname or a path relative
 *     to RootDirectory
 */
{
   DPRINT("InitializeObjectAttributes(InitializedAttributes %x "
	  "ObjectName %x Attributes %x RootDirectory %x)\n",
	  InitializedAttributes,ObjectName,Attributes,RootDirectory);
   InitializedAttributes->Length=sizeof(OBJECT_ATTRIBUTES);
   InitializedAttributes->RootDirectory=RootDirectory;
   InitializedAttributes->ObjectName=ObjectName;
   InitializedAttributes->Attributes=Attributes;
   InitializedAttributes->SecurityDescriptor=SecurityDescriptor;
   InitializedAttributes->SecurityQualityOfService=NULL;
}

