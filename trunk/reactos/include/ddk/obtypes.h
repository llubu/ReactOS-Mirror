#ifndef _INCLUDE_DDK_OBTYPES_H
#define _INCLUDE_DDK_OBTYPES_H
/* $Id: obtypes.h,v 1.8 2000/06/15 18:37:33 ekohl Exp $ */
struct _DIRECTORY_OBJECT;
struct _OBJECT_ATTRIBUTES;

typedef ULONG ACCESS_STATE, *PACCESS_STATE;

typedef struct _OBJECT_HANDLE_INFORMATION {
    ULONG HandleAttributes;
    ACCESS_MASK GrantedAccess;
} OBJECT_HANDLE_INFORMATION, *POBJECT_HANDLE_INFORMATION;

typedef struct _OBJECT_TYPE
{   
   /*
    * PURPOSE: Name of the type
    */
   UNICODE_STRING TypeName;
   
   /*
    * PURPOSE: Total number of objects of this type
    */
   ULONG TotalObjects;
   
   /*
    * PURPOSE: Total number of handles of this type
    */
   ULONG TotalHandles;
   
   /*
    * PURPOSE: Maximum objects of this type
    */
   ULONG MaxObjects;
   
   /*
    * PURPOSE: Maximum handles of this type
    */
   ULONG MaxHandles;
   
   /*
    * PURPOSE: Paged pool charge
    */
   ULONG PagedPoolCharge;
   
   /*
    * PURPOSE: Nonpaged pool charge
    */
   ULONG NonpagedPoolCharge;
   
   /*
    * PURPOSE: Dumps the object
    * NOTE: To be defined
    */
   VOID (*Dump)(VOID);
   
   /*
    * PURPOSE: Opens the object
    * NOTE: To be defined
    */
   VOID (*Open)(VOID);
   
   /*
    * PURPOSE: Called to close an object if OkayToClose returns true
    */
   VOID (*Close)(PVOID ObjectBody, ULONG HandleCount);

   /*
    * PURPOSE: Called to delete an object when the last reference is removed
    */
   VOID (*Delete)(PVOID ObjectBody);

   /*
    * PURPOSE: Called when an open attempts to open a file apparently
    * residing within the object
    * RETURNS: a pointer to the object that corresponds to the child
    *   child of ParsedObject that is on Path.  Path is modified to
    *   to point to the remainder of the path after the child. NULL
    *   should be return when a leaf is reached and Path should be
    *   left unchanged as a reault.
    */
//   PVOID (*Parse)(PVOID ParsedObject, PWSTR* Path);

   /*
    * RETURNS
    *     STATUS_SUCCESS       NextObject was found
    *     STATUS_UNSUCCESSFUL  NextObject not found
    *     STATUS_REPARSE       Path changed, restart parsing the path
    */
   NTSTATUS (*Parse)(PVOID ParsedObject,
		     PVOID *NextObject,
		     PUNICODE_STRING FullPath,
		     PWSTR *Path);
   
   /*
    */
   NTSTATUS (*Security)(PVOID Object,
			ULONG InfoClass,
			PVOID Info,
			PULONG InfoLength);
   
   /*
    */
   VOID (*QueryName)(VOID);
   
   /*
    * PURPOSE: Called when a process asks to close the object
    */
   VOID (*OkayToClose)(VOID);
   
   NTSTATUS (*Create)(PVOID ObjectBody,
		      PVOID Parent,
		      PWSTR RemainingPath,
		      struct _OBJECT_ATTRIBUTES* ObjectAttributes);
   
} OBJECT_TYPE, *POBJECT_TYPE;


typedef struct _OBJECT_HEADER
/*
 * PURPOSE: Header for every object managed by the object manager
 */
{   
   UNICODE_STRING Name;
   LIST_ENTRY Entry;
   LONG RefCount;   
   LONG HandleCount;   
   BOOLEAN Permanent;   
   struct _DIRECTORY_OBJECT* Parent;   
   POBJECT_TYPE ObjectType;
   
   /*
    * PURPOSE: Object type
    * NOTE: This overlaps the first member of the object body
    */
   CSHORT Type;
   
   /*
    * PURPOSE: Object size
    * NOTE: This overlaps the second member of the object body
    */
   CSHORT Size;
   
   
} OBJECT_HEADER, *POBJECT_HEADER;

typedef struct _OBJECT_ATTRIBUTES {
	ULONG Length;
	HANDLE RootDirectory;
	PUNICODE_STRING ObjectName;
	ULONG Attributes;
	SECURITY_DESCRIPTOR *SecurityDescriptor;       
	SECURITY_QUALITY_OF_SERVICE *SecurityQualityOfService;  
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;

typedef struct _HANDLE_TABLE
{
   LIST_ENTRY ListHead;
   KSPIN_LOCK ListLock;
} HANDLE_TABLE, *PHANDLE_TABLE;

extern POBJECT_TYPE ObDirectoryType;

#if 0 
#define OBJECT_TYPE_0			0
#define OBJECT_TYPE_1			1
#define OBJECT_TYPE_DIRECTORY		2
#define OBJECT_TYPE_SYMBOLICLINK	3
#define OBJECT_TYPE_TOKEN		4
#define OBJECT_TYPE_PROCESS		5
#define OBJECT_TYPE_THREAD		6
#define OBJECT_TYPE_EVENT		7
#define OBJECT_TYPE_8			8
#define OBJECT_TYPE_MUTANT		9
#define OBJECT_TYPE_SEMAPHORE		10
#define OBJECT_TYPE_TIMER		11
#define OBJECT_TYPE_12			12
#define OBJECT_TYPE_WINDOWSTATION	13
#define OBJECT_TYPE_DESKTOP		14
#define OBJECT_TYPE_SECTION		15
#define OBJECT_TYPE_KEY			16
#define OBJECT_TYPE_PORT		17
#define OBJECT_TYPE_18			18
#define OBJECT_TYPE_19			19
#define OBJECT_TYPE_20			20
#define OBJECT_TYPE_21			21
#define OBJECT_TYPE_IOCOMPLETION	22
#define OBJECT_TYPE_FILE		23
#endif

#endif /* ndef _INCLUDE_DDK_OBTYPES_H */
