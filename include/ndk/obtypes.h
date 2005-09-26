/*
 * PROJECT:         ReactOS Native Headers
 * FILE:            include/ndk/obtypes.h
 * PURPOSE:         Defintions for Object Manager Types not defined in DDK/IFS
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * UPDATE HISTORY:
 *                  Created 06/10/04
 */
#ifndef _OBTYPES_H
#define _OBTYPES_H

/* DEPENDENCIES **************************************************************/

/* CONSTANTS *****************************************************************/

#ifdef NTOS_MODE_USER
/* Definitions for Object Creation */
#define OBJ_INHERIT 2L
#define OBJ_PERMANENT 16L
#define OBJ_EXCLUSIVE 32L
#define OBJ_CASE_INSENSITIVE 64L
#define OBJ_OPENIF 128L
#define OBJ_OPENLINK 256L
#define OBJ_VALID_ATTRIBUTES 498L
#define InitializeObjectAttributes(p,n,a,r,s) { \
    (p)->Length = sizeof(OBJECT_ATTRIBUTES); \
    (p)->RootDirectory = (r); \
    (p)->Attributes = (a); \
    (p)->ObjectName = (n); \
    (p)->SecurityDescriptor = (s); \
    (p)->SecurityQualityOfService = NULL; \
}

/* Directory Object Access Rights */
#define DIRECTORY_QUERY                 0x0001
#define DIRECTORY_TRAVERSE              0x0002
#define DIRECTORY_CREATE_OBJECT         0x0004
#define DIRECTORY_CREATE_SUBDIRECTORY   0x0008
#define DIRECTORY_ALL_ACCESS            (STANDARD_RIGHTS_REQUIRED | 0xF)
#endif

/* Duplication Flags */
#define DUPLICATE_SAME_ATTRIBUTES       0x00000004

/* Values for DosDeviceDriveType */
#define DOSDEVICE_DRIVE_UNKNOWN		0
#define DOSDEVICE_DRIVE_CALCULATE	1
#define DOSDEVICE_DRIVE_REMOVABLE	2
#define DOSDEVICE_DRIVE_FIXED		3
#define DOSDEVICE_DRIVE_REMOTE		4
#define DOSDEVICE_DRIVE_CDROM		5
#define DOSDEVICE_DRIVE_RAMDISK		6

#ifndef NTOS_MODE_USER
/* Object Flags */
#define OB_FLAG_CREATE_INFO    0x01
#define OB_FLAG_KERNEL_MODE    0x02
#define OB_FLAG_CREATOR_INFO   0x04
#define OB_FLAG_EXCLUSIVE      0x08
#define OB_FLAG_PERMANENT      0x10
#define OB_FLAG_SECURITY       0x20
#define OB_FLAG_SINGLE_PROCESS 0x40

/* ENUMERATIONS **************************************************************/

typedef enum _OB_OPEN_REASON
{
    ObCreateHandle,
    ObOpenHandle,
    ObDuplicateHandle,
    ObInheritHandle,
    ObMaxOpenReason
} OB_OPEN_REASON;
#endif

typedef enum _OBJECT_INFORMATION_CLASS
{
    ObjectBasicInformation,
    ObjectNameInformation,
    ObjectTypeInformation,
    ObjectAllTypesInformation,
    ObjectHandleInformation
} OBJECT_INFORMATION_CLASS;

/* FUNCTION TYPES ************************************************************/

#ifndef NTOS_MODE_USER
/* Object Callbacks FIXME: Update these soon */
typedef NTSTATUS
(NTAPI *OB_OPEN_METHOD)(
    OB_OPEN_REASON  Reason,
    PVOID  ObjectBody,
    PEPROCESS  Process,
    ULONG  HandleCount,
    ACCESS_MASK  GrantedAccess
);

typedef NTSTATUS
(NTAPI *OB_PARSE_METHOD)(
    PVOID  Object,
    PVOID  *NextObject,
    PUNICODE_STRING  FullPath,
    PWSTR  *Path,
    ULONG  Attributes
);

typedef VOID
(NTAPI *OB_DELETE_METHOD)(
    PVOID  DeletedObject
);

typedef VOID
(NTAPI *OB_CLOSE_METHOD)(
    PVOID  ClosedObject,
    ULONG  HandleCount
);

typedef VOID
(NTAPI *OB_DUMP_METHOD)(VOID);

typedef NTSTATUS
(NTAPI *OB_OKAYTOCLOSE_METHOD)(VOID);

typedef NTSTATUS
(NTAPI *OB_QUERYNAME_METHOD)(
    PVOID  ObjectBody,
    POBJECT_NAME_INFORMATION  ObjectNameInfo,
    ULONG  Length,
    PULONG  ReturnLength
);

typedef PVOID
(NTAPI *OB_FIND_METHOD)(
    PVOID  WinStaObject,
    PWSTR  Name,
    ULONG  Attributes
);

typedef NTSTATUS
(NTAPI *OB_SECURITY_METHOD)(
    PVOID Object,
    SECURITY_OPERATION_CODE OperationType,
    SECURITY_INFORMATION SecurityInformation,
    PSECURITY_DESCRIPTOR NewSecurityDescriptor,
    PULONG ReturnLength,
    PSECURITY_DESCRIPTOR *OldSecurityDescriptor,
    POOL_TYPE PoolType,
    PGENERIC_MAPPING GenericMapping
);

/* FIXME: TEMPORARY HACK */
typedef NTSTATUS
(NTAPI *OB_CREATE_METHOD)(
    PVOID  ObjectBody,
    PVOID  Parent,
    PWSTR  RemainingPath,
    struct _OBJECT_ATTRIBUTES*  ObjectAttributes
);
#endif

/* TYPES *********************************************************************/

#ifdef NTOS_MODE_USER
typedef struct _OBJECT_NAME_INFORMATION
{
    UNICODE_STRING Name;
} OBJECT_NAME_INFORMATION, *POBJECT_NAME_INFORMATION;
#endif

typedef struct _OBJECT_HANDLE_ATTRIBUTE_INFORMATION
{
    BOOLEAN Inherit;
    BOOLEAN ProtectFromClose;
} OBJECT_HANDLE_ATTRIBUTE_INFORMATION, *POBJECT_HANDLE_ATTRIBUTE_INFORMATION;

typedef struct _OBJECT_DIRECTORY_INFORMATION
{
    UNICODE_STRING ObjectName;
    UNICODE_STRING ObjectTypeName;
} OBJECT_DIRECTORY_INFORMATION, *POBJECT_DIRECTORY_INFORMATION;

#ifndef NTOS_MODE_USER
typedef struct _OBJECT_BASIC_INFORMATION
{
    ULONG Attributes;
    ACCESS_MASK GrantedAccess;
    ULONG HandleCount;
    ULONG PointerCount;
    ULONG PagedPoolUsage;
    ULONG NonPagedPoolUsage;
    ULONG Reserved[3];
    ULONG NameInformationLength;
    ULONG TypeInformationLength;
    ULONG SecurityDescriptorLength;
    LARGE_INTEGER CreateTime;
} OBJECT_BASIC_INFORMATION, *POBJECT_BASIC_INFORMATION;

typedef struct _OBJECT_HEADER_NAME_INFO
{
    struct _DIRECTORY_OBJECT *Directory;
    UNICODE_STRING Name;
    ULONG QueryReferences;
    ULONG Reserved2;
    ULONG DbgReferenceCount;
} OBJECT_HEADER_NAME_INFO, *POBJECT_HEADER_NAME_INFO;

typedef struct _OBJECT_CREATE_INFORMATION
{
    ULONG Attributes;
    HANDLE RootDirectory;
    PVOID ParseContext;
    KPROCESSOR_MODE ProbeMode;
    ULONG PagedPoolCharge;
    ULONG NonPagedPoolCharge;
    ULONG SecurityDescriptorCharge;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    PSECURITY_QUALITY_OF_SERVICE SecurityQos;
    SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;
} OBJECT_CREATE_INFORMATION, *POBJECT_CREATE_INFORMATION;

typedef struct _OBJECT_TYPE_INITIALIZER
{
    USHORT Length;
    UCHAR UseDefaultObject;
    UCHAR CaseInsensitive;
    ULONG InvalidAttributes;
    GENERIC_MAPPING GenericMapping;
    ULONG ValidAccessMask;
    UCHAR SecurityRequired;
    UCHAR MaintainHandleCount;
    UCHAR MaintainTypeList;
    POOL_TYPE PoolType;
    ULONG DefaultPagedPoolCharge;
    ULONG DefaultNonPagedPoolCharge;
    OB_DUMP_METHOD DumpProcedure;
    OB_OPEN_METHOD OpenProcedure;
    OB_CLOSE_METHOD CloseProcedure;
    OB_DELETE_METHOD DeleteProcedure;
    OB_PARSE_METHOD ParseProcedure;
    OB_SECURITY_METHOD SecurityProcedure;
    OB_QUERYNAME_METHOD QueryNameProcedure;
    OB_OKAYTOCLOSE_METHOD OkayToCloseProcedure;
} OBJECT_TYPE_INITIALIZER, *POBJECT_TYPE_INITIALIZER;

typedef struct _OBJECT_TYPE
{
    ERESOURCE Mutex;
    LIST_ENTRY TypeList;
    UNICODE_STRING Name;
    PVOID DefaultObject;
    ULONG Index;
    ULONG TotalNumberOfObjects;
    ULONG TotalNumberOfHandles;
    ULONG HighWaterNumberOfObjects;
    ULONG HighWaterNumberOfHandles;
    OBJECT_TYPE_INITIALIZER TypeInfo;
    ULONG Key;
    ERESOURCE ObjectLocks[4];
} OBJECT_TYPE;

typedef struct _OBJECT_HANDLE_COUNT_ENTRY
{
    struct _EPROCESS *Process;
    ULONG HandleCount;
} OBJECT_HANDLE_COUNT_ENTRY, *POBJECT_HANDLE_COUNT_ENTRY;

typedef struct _OBJECT_HANDLE_COUNT_DATABASE
{
    ULONG CountEntries;
    POBJECT_HANDLE_COUNT_ENTRY HandleCountEntries[1];
} OBJECT_HANDLE_COUNT_DATABASE, *POBJECT_HANDLE_COUNT_DATABASE;

typedef struct _OBJECT_HEADER_HANDLE_INFO
{
    union
    {
        POBJECT_HANDLE_COUNT_DATABASE HandleCountDatabase;
        OBJECT_HANDLE_COUNT_ENTRY SingleEntry;
    };
} OBJECT_HEADER_HANDLE_INFO, *POBJECT_HEADER_HANDLE_INFO;

typedef struct _OBJECT_HEADER_CREATOR_INFO
{
    LIST_ENTRY TypeList;
    PVOID CreatorUniqueProcess;
    USHORT CreatorBackTraceIndex;
    USHORT Reserved;
} OBJECT_HEADER_CREATOR_INFO, *POBJECT_HEADER_CREATOR_INFO;

typedef struct _OBJECT_HEADER
{
    LIST_ENTRY Entry; /* FIXME: REMOVE THIS SOON */
    LONG PointerCount;
    union
    {
        LONG HandleCount;
        PVOID NextToFree;
    };
    POBJECT_TYPE Type;
    UCHAR NameInfoOffset;
    UCHAR HandleInfoOffset;
    UCHAR QuotaInfoOffset;
    UCHAR Flags;
    union
    {
        POBJECT_CREATE_INFORMATION ObjectCreateInfo;
        PVOID QuotaBlockCharged;
    };
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    QUAD Body;
} OBJECT_HEADER, *POBJECT_HEADER;

/*
 * FIXME: These will eventually become centerfold in the compliant Ob Manager
 * For now, they are only here so Device Map is properly defined before the header
 * changes
 */
typedef struct _OBJECT_DIRECTORY_ENTRY
{
    struct _OBJECT_DIRECTORY_ENTRY *ChainLink;
    PVOID Object;
    ULONG HashValue;
} OBJECT_DIRECTORY_ENTRY, *POBJECT_DIRECTORY_ENTRY;

#define NUMBER_HASH_BUCKETS 37
typedef struct _OBJECT_DIRECTORY
{
    struct _OBJECT_DIRECTORY_ENTRY *HashBuckets[NUMBER_HASH_BUCKETS];
    struct _EX_PUSH_LOCK *Lock;
    struct _DEVICE_MAP *DeviceMap;
    ULONG SessionId;
} OBJECT_DIRECTORY, *POBJECT_DIRECTORY;

typedef struct _DEVICE_MAP
{
    POBJECT_DIRECTORY   DosDevicesDirectory;
    POBJECT_DIRECTORY   GlobalDosDevicesDirectory;
    ULONG               ReferenceCount;
    ULONG               DriveMap;
    UCHAR               DriveType[32];
} DEVICE_MAP, *PDEVICE_MAP;

/* EXPORTED DATA *************************************************************/

extern POBJECT_TYPE NTSYSAPI ObDirectoryType;
extern PDEVICE_MAP NTSYSAPI ObSystemDeviceMap;
#endif

#endif
