/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * FILE:              include/internal/objmgr.h
 * PURPOSE:           Object manager definitions
 * PROGRAMMER:        David Welch (welch@mcmail.com)
 */

#ifndef __INCLUDE_INTERNAL_OBJMGR_H
#define __INCLUDE_INTERNAL_OBJMGR_H

#define NTOS_MODE_KERNEL
#include <ntos.h>

struct _EPROCESS;

typedef struct
{
   CSHORT Type;
   CSHORT Size;
} COMMON_BODY_HEADER, *PCOMMON_BODY_HEADER;

typedef PVOID POBJECT;

typedef struct _DIRECTORY_OBJECT
{
   CSHORT Type;
   CSHORT Size;
   
   /*
    * PURPOSE: Head of the list of our subdirectories
    */
   LIST_ENTRY head;
   KSPIN_LOCK Lock;
} DIRECTORY_OBJECT, *PDIRECTORY_OBJECT;


typedef struct _SYMLINK_OBJECT
{
  CSHORT Type;
  CSHORT Size;
  UNICODE_STRING TargetName;
  LARGE_INTEGER CreateTime;
} SYMLINK_OBJECT, *PSYMLINK_OBJECT;


typedef struct _TYPE_OBJECT
{
  CSHORT Type;
  CSHORT Size;
  
  /* pointer to object type data */
  POBJECT_TYPE ObjectType;
} TYPE_OBJECT, *PTYPE_OBJECT;


/*
 * Enumeration of object types
 */
enum
{
   OBJTYP_INVALID,
   OBJTYP_TYPE,
   OBJTYP_DIRECTORY,
   OBJTYP_SYMLNK,
   OBJTYP_DEVICE,
   OBJTYP_THREAD,
   OBJTYP_FILE,
   OBJTYP_PROCESS,
   OBJTYP_SECTION,
   OBJTYP_MAX,
};


#define OBJECT_ALLOC_SIZE(ObjectSize) ((ObjectSize)+sizeof(OBJECT_HEADER)-sizeof(COMMON_BODY_HEADER))


extern PDIRECTORY_OBJECT NameSpaceRoot;
extern POBJECT_TYPE ObSymbolicLinkType;


POBJECT_HEADER BODY_TO_HEADER(PVOID body);
PVOID HEADER_TO_BODY(POBJECT_HEADER obj);

VOID ObpAddEntryDirectory(PDIRECTORY_OBJECT Parent,
			  POBJECT_HEADER Header,
			  PWSTR Name);
VOID ObpRemoveEntryDirectory(POBJECT_HEADER Header);

VOID
ObInitSymbolicLinkImplementation(VOID);


NTSTATUS ObCreateHandle(struct _EPROCESS* Process,
			PVOID ObjectBody,
			ACCESS_MASK GrantedAccess,
			BOOLEAN Inherit,
			PHANDLE Handle);
VOID ObCreateHandleTable(struct _EPROCESS* Parent,
			 BOOLEAN Inherit,
			 struct _EPROCESS* Process);
NTSTATUS ObFindObject(POBJECT_ATTRIBUTES ObjectAttributes,
		      PVOID* ReturnedObject,
		      PUNICODE_STRING RemainingPath,
		      POBJECT_TYPE ObjectType);
VOID ObCloseAllHandles(struct _EPROCESS* Process);
VOID ObDeleteHandleTable(struct _EPROCESS* Process);

NTSTATUS
ObDeleteHandle(PEPROCESS Process,
	       HANDLE Handle,
	       PVOID *ObjectBody);

NTSTATUS
ObpQueryHandleAttributes(HANDLE Handle,
			 POBJECT_HANDLE_ATTRIBUTE_INFORMATION HandleInfo);

NTSTATUS
ObpSetHandleAttributes(HANDLE Handle,
		       POBJECT_HANDLE_ATTRIBUTE_INFORMATION HandleInfo);

NTSTATUS
ObpCreateTypeObject(POBJECT_TYPE ObjectType);

ULONG
ObGetObjectHandleCount(PVOID Object);
NTSTATUS
ObDuplicateObject(PEPROCESS SourceProcess,
		  PEPROCESS TargetProcess,
		  HANDLE SourceHandle,
		  PHANDLE TargetHandle,
		  ACCESS_MASK DesiredAccess,
		  BOOLEAN InheritHandle,
		  ULONG	Options);

ULONG 
ObpGetHandleCountbyHandleTable(PHANDLE_TABLE HandleTable);


#endif /* __INCLUDE_INTERNAL_OBJMGR_H */
