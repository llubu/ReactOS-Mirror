/* $Id: rtl.h,v 1.22 1999/12/26 17:22:18 ea Exp $
 * 
 */

#ifndef __DDK_RTL_H
#define __DDK_RTL_H

#include <stddef.h>

typedef struct _CONTROLLER_OBJECT
{
   CSHORT Type;
   CSHORT Size;   
   PVOID ControllerExtension;
   KDEVICE_QUEUE DeviceWaitQueue;
   ULONG Spare1;
   LARGE_INTEGER Spare2;
} CONTROLLER_OBJECT, *PCONTROLLER_OBJECT;

typedef struct _STRING
{
   /*
    * Length in bytes of the string stored in buffer
    */
   USHORT Length;

   /*
    * Maximum length of the string 
    */
   USHORT MaximumLength;

   /*
    * String
    */
   PCHAR Buffer;
} STRING, *PSTRING;

typedef STRING ANSI_STRING;
typedef PSTRING PANSI_STRING;

typedef STRING OEM_STRING;
typedef PSTRING POEM_STRING;


typedef struct _TIME_FIELDS
{
   CSHORT Year;
   CSHORT Month;
   CSHORT Day;
   CSHORT Hour;
   CSHORT Minute;
   CSHORT Second;
   CSHORT Milliseconds;
   CSHORT Weekday;
} TIME_FIELDS, *PTIME_FIELDS;

typedef struct _RTL_BITMAP
{
   ULONG  SizeOfBitMap;
   PULONG Buffer;
} RTL_BITMAP, *PRTL_BITMAP;

typedef struct {
	ULONG		Length;
	ULONG		Unknown[11];
} RTL_HEAP_DEFINITION, *PRTL_HEAP_DEFINITION;


/*
 * PURPOSE: Flags for RtlQueryRegistryValues
 */
enum
{
   RTL_QUERY_REGISTRY_SUBKEY,
   RTL_QUERY_REGISTRY_TOPKEY,
   RTL_QUERY_REGISTRY_REQUIRED,
   RTL_QUERY_REGISTRY_NOVALUE,
   RTL_QUERY_REGISTRY_NOEXPAND,
   RTL_QUERY_REGISTRY_DIRECT,
   RTL_QUERY_REGISTRY_DELETE,
};

typedef NTSTATUS (*PRTL_QUERY_REGISTRY_ROUTINE)(PWSTR ValueName,
						ULONG ValueType,
						PVOID ValueData,
						ULONG ValueLength,
						PVOID Context,
						PVOID EntryContext);

typedef struct _RTL_QUERY_REGISTRY_TABLE
{
   PRTL_QUERY_REGISTRY_ROUTINE QueryRoutine;
   ULONG Flags;
   PWSTR Name;
   PVOID EntryContext;
   ULONG DefaultType;
   PVOID DefaultData;
   ULONG DefaultLength;
} RTL_QUERY_REGISTRY_TABLE, *PRTL_QUERY_REGISTRY_TABLE;

/*
 * PURPOSE: Used with RtlCheckRegistryKey, RtlCreateRegistryKey, 
 * RtlDeleteRegistryKey
 */
enum
{
   RTL_REGISTRY_ABSOLUTE,
   RTL_REGISTRY_SERVICES,
   RTL_REGISTRY_CONTROL,
   RTL_REGISTRY_WINDOWS_NT,
   RTL_REGISTRY_DEVICEMAP,
   RTL_REGISTRY_USER,
   RTL_REGISTRY_OPTIONAL,
   RTL_REGISTRY_VALUE,
};


#if defined(__NTOSKRNL__) || defined(__NTDLL__)
#define NLS_MB_CODE_PAGE_TAG     NlsMbCodePageTag
#define NLS_MB_OEM_CODE_PAGE_TAG NlsMbOemCodePageTag
#else
#define NLS_MB_CODE_PAGE_TAG     (*NlsMbCodePageTag)
#define NLS_MB_OEM_CODE_PAGE_TAG (*NlsMbOemCodePageTag)
#endif /* __NTOSKRNL__ || __NTDLL__ */

extern BOOLEAN NLS_MB_CODE_PAGE_TAG;
extern BOOLEAN NLS_MB_OEM_CODE_PAGE_TAG;


/*
 * VOID
 * InitializeObjectAttributes (
 *	POBJECT_ATTRIBUTES	InitializedAttributes,
 *	PUNICODE_STRING		ObjectName,
 *	ULONG			Attributes,
 *	HANDLE			RootDirectory,
 *	PSECURITY_DESCRIPTOR	SecurityDescriptor
 *	);
 *
 * FUNCTION: Sets up a parameter of type OBJECT_ATTRIBUTES for a 
 * subsequent call to ZwCreateXXX or ZwOpenXXX
 * ARGUMENTS:
 *        InitializedAttributes (OUT) = Caller supplied storage for the
 *                                      object attributes
 *        ObjectName = Full path name for object
 *        Attributes = Attributes for the object
 *        RootDirectory = Where the object should be placed or NULL
 *        SecurityDescriptor = Ignored
 */

#define InitializeObjectAttributes(p,n,a,r,s) \
{ \
	(p)->Length = sizeof(OBJECT_ATTRIBUTES); \
	(p)->ObjectName = n; \
	(p)->Attributes = a; \
	(p)->RootDirectory = r; \
	(p)->SecurityDescriptor = s; \
	(p)->SecurityQualityOfService = NULL; \
}

VOID
InitializeListHead (
	PLIST_ENTRY	ListHead
	);

VOID
InsertHeadList (
	PLIST_ENTRY	ListHead,
	PLIST_ENTRY	Entry
	);

VOID
InsertTailList (
	PLIST_ENTRY	ListHead,
	PLIST_ENTRY	Entry
	);

BOOLEAN
IsListEmpty (
	PLIST_ENTRY	ListHead
	);

PSINGLE_LIST_ENTRY
PopEntryList (
	PSINGLE_LIST_ENTRY	ListHead
	);

VOID
PushEntryList (
	PSINGLE_LIST_ENTRY	ListHead,
	PSINGLE_LIST_ENTRY	Entry
	);

VOID
RemoveEntryList (
	PLIST_ENTRY	Entry
	);

PLIST_ENTRY
RemoveHeadList (
	PLIST_ENTRY	ListHead
	);

PLIST_ENTRY
RemoveTailList (
	PLIST_ENTRY	ListHead
	);

PVOID
STDCALL
RtlAllocateHeap (
	HANDLE	Heap,
	ULONG	Flags,
	ULONG	Size
	);

WCHAR
STDCALL
RtlAnsiCharToUnicodeChar (
	CHAR	AnsiChar
	);

ULONG
STDCALL
RtlAnsiStringToUnicodeSize (
	PANSI_STRING	AnsiString
	);

NTSTATUS
STDCALL
RtlAnsiStringToUnicodeString (
	PUNICODE_STRING	DestinationString,
	PANSI_STRING	SourceString,
	BOOLEAN		AllocateDestinationString
	);

NTSTATUS
STDCALL
RtlAppendAsciizToString(
	PSTRING	Destination,
	PCSZ	Source
	);

NTSTATUS
STDCALL
RtlAppendStringToString (
	PSTRING	Destination,
	PSTRING	Source
	);

NTSTATUS
STDCALL
RtlAppendUnicodeStringToString (
	PUNICODE_STRING	Destination,
	PUNICODE_STRING	Source
	);

NTSTATUS
STDCALL
RtlAppendUnicodeToString (
	PUNICODE_STRING	Destination,
	PWSTR		Source
	);

NTSTATUS
STDCALL
RtlCharToInteger (
	PCSZ	String,
	ULONG	Base,
	PULONG	Value
	);

NTSTATUS
STDCALL
RtlCheckRegistryKey (
	ULONG	RelativeTo,
	PWSTR	Path
	);

UINT
STDCALL
RtlCompactHeap (
	HANDLE	hheap,
	DWORD	flags
	);

ULONG
STDCALL
RtlCompareMemory (
	PVOID	Source1,
	PVOID	Source2,
	ULONG	Length
	);

LONG
STDCALL
RtlCompareString (
	PSTRING	String1,
	PSTRING	String2,
	BOOLEAN	CaseInsensitive
	);

LONG
STDCALL
RtlCompareUnicodeString (
	PUNICODE_STRING	String1,
	PUNICODE_STRING	String2,
	BOOLEAN		BaseInsensitive
	);

LARGE_INTEGER
STDCALL
RtlConvertLongToLargeInteger (
	LONG	SignedInteger
	);

LARGE_INTEGER
STDCALL
RtlConvertUlongToLargeInteger (
	ULONG	UnsignedInteger
	);

VOID
RtlCopyBytes (
	PVOID		Destination,
	CONST VOID	* Source,
	ULONG		Length
	);

VOID
RtlCopyMemory (
	VOID		* Destination,
	CONST VOID	* Source,
	ULONG		Length
	);

VOID
STDCALL
RtlCopyString (
	PSTRING	DestinationString,
	PSTRING	SourceString
	);

VOID
STDCALL
RtlCopyUnicodeString (
	PUNICODE_STRING	DestinationString,
	PUNICODE_STRING	SourceString
	);

HANDLE
STDCALL
RtlCreateHeap (
	ULONG			Flags,
	PVOID			BaseAddress,
	ULONG			SizeToReserve,
	ULONG			SizeToCommit,
	PVOID			Unknown,
	PRTL_HEAP_DEFINITION	Definition
	);

NTSTATUS
STDCALL
RtlCreateRegistryKey (
	ULONG	RelativeTo,
	PWSTR	Path
	);

NTSTATUS
STDCALL
RtlCreateSecurityDescriptor (
	PSECURITY_DESCRIPTOR	SecurityDescriptor,
	ULONG			Revision
	);

BOOLEAN
STDCALL
RtlCreateUnicodeString (
	OUT	PUNICODE_STRING	Destination,
	IN	PWSTR		Source
	);

BOOLEAN
STDCALL
RtlCreateUnicodeStringFromAsciiz (
	OUT	PUNICODE_STRING	Destination,
	IN	PCSZ		Source
	);

NTSTATUS
STDCALL
RtlDeleteRegistryValue (
	ULONG	RelativeTo,
	PWSTR	Path,
	PWSTR	ValueName
	);

BOOL
STDCALL
RtlDestroyHeap (
	HANDLE	hheap
	);

NTSTATUS
STDCALL
RtlDowncaseUnicodeString (
	IN OUT PUNICODE_STRING	DestinationString,
	IN PUNICODE_STRING	SourceString,
	IN BOOLEAN		AllocateDestinationString
	);

LARGE_INTEGER
STDCALL
RtlEnlargedIntegerMultiply (
	LONG	Multiplicand,
	LONG	Multiplier
	);

ULONG
STDCALL
RtlEnlargedUnsignedDivide (
	ULARGE_INTEGER	Dividend,
	ULONG		Divisor,
	PULONG		Remainder
	);

LARGE_INTEGER
STDCALL
RtlEnlargedUnsignedMultiply (
	ULONG	Multiplicand,
	ULONG	Multiplier
	);

BOOLEAN
STDCALL
RtlEqualString (
	PSTRING	String1,
	PSTRING	String2,
	BOOLEAN	CaseInSensitive
	);

BOOLEAN
STDCALL
RtlEqualUnicodeString (
	PUNICODE_STRING	String1,
	PUNICODE_STRING	String2,
	BOOLEAN		CaseInSensitive
	);

LARGE_INTEGER
STDCALL
RtlExtendedIntegerMultiply (
	LARGE_INTEGER	Multiplicand,
	LONG		Multiplier
	);

LARGE_INTEGER
STDCALL
RtlExtendedLargeIntegerDivide (
	LARGE_INTEGER	Dividend,
	ULONG		Divisor,
	PULONG		Remainder
	);

LARGE_INTEGER
STDCALL
RtlExtendedMagicDivide (
	LARGE_INTEGER	Dividend,
	LARGE_INTEGER	MagicDivisor,
	CCHAR		ShiftCount
	);

VOID
STDCALL
RtlFillMemory (
	PVOID	Destination,
	ULONG	Length,
	UCHAR	Fill
	);

VOID
STDCALL
RtlFillMemoryUlong (
	PVOID	Destination,
	ULONG	Length,
	ULONG	Fill
	);

VOID
STDCALL
RtlFreeAnsiString (
	PANSI_STRING	AnsiString
	);

BOOLEAN
STDCALL
RtlFreeHeap (
	HANDLE	Heap,
	ULONG	Flags,
	PVOID	Address
	);

VOID
STDCALL
RtlFreeOemString (
	POEM_STRING	OemString
	);

VOID
STDCALL
RtlFreeUnicodeString (
	PUNICODE_STRING	UnicodeString
	);

VOID
RtlGetCallersAddress (
	PVOID	* CallersAddress
	);

VOID
STDCALL
RtlGetDefaultCodePage (
	PUSHORT AnsiCodePage,
	PUSHORT OemCodePage
	);

HANDLE
STDCALL
RtlGetProcessHeap (
	VOID
	);

VOID
STDCALL
RtlInitAnsiString (
	PANSI_STRING	DestinationString,
	PCSZ		SourceString
	);

NTSTATUS
STDCALL
RtlInitializeContext (
	IN	HANDLE			ProcessHandle,
	IN	PCONTEXT		Context,
	IN	PVOID			Parameter,
	IN	PTHREAD_START_ROUTINE	StartAddress,
	IN OUT	PINITIAL_TEB		InitialTeb
	);

VOID
STDCALL
RtlInitString (
	PSTRING	DestinationString,
	PCSZ	SourceString
	);

VOID
STDCALL
RtlInitUnicodeString (
	PUNICODE_STRING	DestinationString,
	PCWSTR		SourceString
	);

NTSTATUS
STDCALL
RtlIntegerToChar (
	IN	ULONG	Value,
	IN	ULONG	Base,
	IN	ULONG	Length,
	IN OUT	PCHAR	String
	);

NTSTATUS
STDCALL
RtlIntegerToUnicodeString (
	IN	ULONG		Value,
	IN	ULONG		Base,
	IN OUT	PUNICODE_STRING	String
	);

LARGE_INTEGER
STDCALL
RtlLargeIntegerAdd (
	LARGE_INTEGER	Addend1,
	LARGE_INTEGER	Addend2
	);

/*
 * VOID
 * RtlLargeIntegerAnd (
 *	PLARGE_INTEGER	Result,
 *	LARGE_INTEGER	Source,
 *	LARGE_INTEGER	Mask
 *	);
 */
#define RtlLargeIntegerAnd(Result, Source, Mask) \
{ \
	Result.HighPart = Source.HighPart & Mask.HighPart; \
	Result.LowPart = Source.LowPart & Mask.LowPart; \
}

LARGE_INTEGER
STDCALL
RtlLargeIntegerArithmeticShift (
	LARGE_INTEGER	LargeInteger,
	CCHAR	ShiftCount
	);

LARGE_INTEGER
STDCALL
RtlLargeIntegerDivide (
	LARGE_INTEGER	Dividend,
	LARGE_INTEGER	Divisor,
	PLARGE_INTEGER	Remainder
	);

/*
 * BOOLEAN
 * RtlLargeIntegerEqualTo (
 *	LARGE_INTEGER	Operand1,
 *	LARGE_INTEGER	Operand2
 *	);
 */
#define RtlLargeIntegerEqualTo(X,Y) \
	(!(((X).LowPart ^ (Y).LowPart) | ((X).HighPart ^ (Y).HighPart)))

/*
 * BOOLEAN
 * RtlLargeIntegerEqualToZero (
 *	LARGE_INTEGER	Operand
 *	);
 */
#define RtlLargeIntegerEqualToZero(X) \
	(!((X).LowPart | (X).HighPart))

/*
 * BOOLEAN
 * RtlLargeIntegerGreaterThan (
 *	LARGE_INTEGER	Operand1,
 *	LARGE_INTEGER	Operand2
 *	);
 */
#define RtlLargeIntegerGreaterThan(X,Y) \
	((((X).HighPart == (Y).HighPart) && ((X).LowPart > (Y).LowPart)) || \
	  ((X).HighPart > (Y).HighPart))

/*
 * BOOLEAN
 * RtlLargeIntegerGreaterThanOrEqualTo (
 *	LARGE_INTEGER	Operand1,
 *	LARGE_INTEGER	Operand2
 *	);
 */
#define RtlLargeIntegerGreaterThanOrEqualTo(X,Y) \
	((((X).HighPart == (Y).HighPart) && ((X).LowPart >= (Y).LowPart)) || \
	  ((X).HighPart > (Y).HighPart))

/*
 * BOOLEAN
 * RtlLargeIntegerGreaterThanOrEqualToZero (
 *	LARGE_INTEGER	Operand1
 *	);
 */
#define RtlLargeIntegerGreaterOrEqualToZero(X) \
	((X).HighPart >= 0)

/*
 * BOOLEAN
 * RtlLargeIntegerGreaterThanZero (
 *	LARGE_INTEGER	Operand1
 *	);
 */
#define RtlLargeIntegerGreaterThanZero(X) \
	((((X).HighPart == 0) && ((X).LowPart > 0)) || \
	  ((X).HighPart > 0 ))

/*
 * BOOLEAN
 * RtlLargeIntegerLessThan (
 *	LARGE_INTEGER	Operand1,
 *	LARGE_INTEGER	Operand2
 *	);
 */
#define RtlLargeIntegerLessThan(X,Y) \
	((((X).HighPart == (Y).HighPart) && ((X).LowPart < (Y).LowPart)) || \
	  ((X).HighPart < (Y).HighPart))

/*
 * BOOLEAN
 * RtlLargeIntegerLessThanOrEqualTo (
 *	LARGE_INTEGER	Operand1,
 *	LARGE_INTEGER	Operand2
 *	);
 */
#define RtlLargeIntegerLessThanOrEqualTo(X,Y) \
	((((X).HighPart == (Y).HighPart) && ((X).LowPart <= (Y).LowPart)) || \
	  ((X).HighPart < (Y).HighPart))

/*
 * BOOLEAN
 * RtlLargeIntegerLessThanOrEqualToZero (
 *	LARGE_INTEGER	Operand
 *	);
 */
#define RtlLargeIntegerLessOrEqualToZero(X) \
	(((X).HighPart < 0) || !((X).LowPart | (X).HighPart))

/*
 * BOOLEAN
 * RtlLargeIntegerLessThanZero (
 *	LARGE_INTEGER	Operand
 *	);
 */
#define RtlLargeIntegerLessThanZero(X) \
	(((X).HighPart < 0))

LARGE_INTEGER
STDCALL
RtlLargeIntegerNegate (
	LARGE_INTEGER	Subtrahend
	);

/*
 * BOOLEAN
 * RtlLargeIntegerNotEqualTo (
 *	LARGE_INTEGER	Operand1,
 *	LARGE_INTEGER	Operand2
 *	);
 */
#define RtlLargeIntegerNotEqualTo(X,Y) \
	((((X).LowPart ^ (Y).LowPart) | ((X).HighPart ^ (Y).HighPart)))

/*
 * BOOLEAN
 * RtlLargeIntegerNotEqualToZero (
 *	LARGE_INTEGER	Operand
 *	);
 */
#define RtlLargeIntegerNotEqualToZero(X) \
	(((X).LowPart | (X).HighPart))

LARGE_INTEGER
STDCALL
RtlLargeIntegerShiftLeft (
	LARGE_INTEGER	LargeInteger,
	CCHAR		ShiftCount
	);

LARGE_INTEGER
STDCALL
RtlLargeIntegerShiftRight (
	LARGE_INTEGER	LargeInteger,
	CCHAR		ShiftCount
	);

LARGE_INTEGER
STDCALL
RtlLargeIntegerSubtract (
	LARGE_INTEGER	Minuend,
	LARGE_INTEGER	Subtrahend
	);

ULONG
STDCALL
RtlLengthSecurityDescriptor (
	PSECURITY_DESCRIPTOR	SecurityDescriptor
	);

BOOL
STDCALL
RtlLockHeap (
	HANDLE	hheap
	);

VOID
STDCALL
RtlMoveMemory (
	PVOID		Destination,
	CONST VOID	* Source,
	ULONG		Length
	);

NTSTATUS
STDCALL
RtlMultiByteToUnicodeN (
	PWCHAR UnicodeString,
	ULONG  UnicodeSize,
	PULONG ResultSize,
	PCHAR  MbString,
	ULONG  MbSize
	);

NTSTATUS
STDCALL
RtlMultiByteToUnicodeSize (
	PULONG UnicodeSize,
	PCHAR  MbString,
	ULONG  MbSize
	);

DWORD
STDCALL
RtlNtStatusToDosError (
	NTSTATUS	StatusCode
	);

int
STDCALL
RtlNtStatusToPsxErrno (
	NTSTATUS	StatusCode
	);

ULONG
STDCALL
RtlOemStringToUnicodeSize (
	POEM_STRING	AnsiString
	);

NTSTATUS
STDCALL
RtlOemStringToUnicodeString (
	PUNICODE_STRING	DestinationString,
	POEM_STRING	SourceString,
	BOOLEAN		AllocateDestinationString
	);

NTSTATUS
STDCALL
RtlOemToUnicodeN (
	PWCHAR	UnicodeString,
	ULONG	UnicodeSize,
	PULONG	ResultSize,
	PCHAR	OemString,
	ULONG	OemSize
	);

NTSTATUS
STDCALL
RtlQueryRegistryValues (
	ULONG				RelativeTo,
	PWSTR				Path,
	PRTL_QUERY_REGISTRY_TABLE	QueryTable,
	PVOID				Context,
	PVOID				Environment
	);

LPVOID
STDCALL
RtlReAllocateHeap (
	HANDLE	hheap,
	DWORD	flags,
	LPVOID	ptr,
	DWORD	size
	);

VOID
RtlRetrieveUlong (
	PULONG	DestinationAddress,
	PULONG	SourceAddress
	);

VOID
RtlRetrieveUshort (
	PUSHORT	DestinationAddress,
	PUSHORT	SourceAddress
	);

NTSTATUS
STDCALL
RtlSetDaclSecurityDescriptor (
	PSECURITY_DESCRIPTOR	SecurityDescriptor,
	BOOLEAN			DaclPresent,
	PACL			Dacl,
	BOOLEAN			DaclDefaulted
	);

DWORD
STDCALL
RtlSizeHeap (
	HANDLE	hheap,
	DWORD	flags,
	PVOID	pmem
	);

PWSTR
RtlStrtok (
	PUNICODE_STRING	_string,
	PWSTR		_sep,
	PWSTR		* temp
	);

VOID
RtlStoreLong (
	PULONG	Address,
	ULONG	Value
	);

VOID
RtlStoreUlong (
	PULONG	Address,
	ULONG	Value
	);

VOID
RtlStoreUshort (
	PUSHORT	Address,
	USHORT	Value
	);

BOOLEAN
RtlTimeFieldsToTime (
	PTIME_FIELDS	TimeFields,
	PLARGE_INTEGER	Time
	);

VOID
RtlTimeToTimeFields (
	PLARGE_INTEGER	Time,
	PTIME_FIELDS	TimeFields
	);

ULONG
STDCALL
RtlUnicodeStringToAnsiSize (
	IN	PUNICODE_STRING	UnicodeString
	);

NTSTATUS
STDCALL
RtlUnicodeStringToAnsiString (
	IN OUT	PANSI_STRING	DestinationString,
	IN	PUNICODE_STRING	SourceString,
	IN	BOOLEAN		AllocateDestinationString
	);

NTSTATUS
STDCALL
RtlUnicodeStringToInteger (
	IN	PUNICODE_STRING	String,
	IN	ULONG		Base,
	OUT	PULONG		Value
	);

ULONG
STDCALL
RtlUnicodeStringToOemSize (
	IN	PUNICODE_STRING	UnicodeString
	);

NTSTATUS
STDCALL
RtlUnicodeStringToOemString (
	IN OUT	POEM_STRING	DestinationString,
	IN	PUNICODE_STRING	SourceString,
	IN	BOOLEAN		AllocateDestinationString
	);

NTSTATUS
STDCALL
RtlUnicodeToMultiByteN (
	PCHAR	MbString,
	ULONG	MbSize,
	PULONG	ResultSize,
	PWCHAR	UnicodeString,
	ULONG	UnicodeSize
	);

NTSTATUS
STDCALL
RtlUnicodeToMultiByteSize (
	PULONG	MbSize,
	PWCHAR	UnicodeString,
	ULONG	UnicodeSize
	);

NTSTATUS
STDCALL
RtlUnicodeToOemN (
	PCHAR	OemString,
	ULONG	OemSize,
	PULONG	ResultSize,
	PWCHAR	UnicodeString,
	ULONG	UnicodeSize
	);

BOOL
STDCALL
RtlUnlockHeap (
	HANDLE	hheap
	);

WCHAR
STDCALL
RtlUpcaseUnicodeChar (
	WCHAR Source
	);

NTSTATUS
STDCALL
RtlUpcaseUnicodeString (
	IN OUT	PUNICODE_STRING	DestinationString,
	IN	PUNICODE_STRING	SourceString,
	IN	BOOLEAN		AllocateDestinationString
	);

NTSTATUS
STDCALL
RtlUpcaseUnicodeStringToAnsiString (
	IN OUT	PANSI_STRING	DestinationString,
	IN	PUNICODE_STRING	SourceString,
	IN	BOOLEAN		AllocateDestinationString
	);

NTSTATUS
STDCALL
RtlUpcaseUnicodeStringToOemString (
	IN OUT	POEM_STRING	DestinationString,
	IN	PUNICODE_STRING	SourceString,
	IN	BOOLEAN		AllocateDestinationString
	);

NTSTATUS
STDCALL
RtlUpcaseUnicodeToMultiByteN (
	PCHAR	MbString,
	ULONG	MbSize,
	PULONG	ResultSize,
	PWCHAR	UnicodeString,
	ULONG	UnicodeSize
	);

NTSTATUS
STDCALL
RtlUpcaseUnicodeToOemN (
	PCHAR	OemString,
	ULONG	OemSize,
	PULONG	ResultSize,
	PWCHAR	UnicodeString,
	ULONG	UnicodeSize
	);

CHAR
STDCALL
RtlUpperChar (
	CHAR	Source
	);

VOID
STDCALL
RtlUpperString (
	PSTRING	DestinationString,
	PSTRING	SourceString
	);

BOOL
STDCALL
RtlValidateHeap (
	HANDLE	hheap,
	DWORD	flags,
	PVOID	pmem
	);

BOOLEAN
STDCALL
RtlValidSecurityDescriptor (
	PSECURITY_DESCRIPTOR	SecurityDescriptor
	);

NTSTATUS
STDCALL
RtlWriteRegistryValue (
	ULONG	RelativeTo,
	PWSTR	Path,
	PWSTR	ValueName,
	ULONG	ValueType,
	PVOID	ValueData,
	ULONG	ValueLength
	);

VOID
STDCALL
RtlZeroMemory (
	PVOID	Destination,
	ULONG	Length
	);

ULONG
STDCALL
RtlxAnsiStringToUnicodeSize (
	IN	PANSI_STRING 	AnsiString
	);

ULONG
STDCALL
RtlxOemStringToUnicodeSize (
	IN	POEM_STRING	OemString
	);

ULONG
STDCALL
RtlxUnicodeStringToAnsiSize (
	IN	PUNICODE_STRING	UnicodeString
	);

ULONG
STDCALL
RtlxUnicodeStringToOemSize (
	IN	PUNICODE_STRING	UnicodeString
	);


/*  functions exported from NTOSKRNL.EXE which are considered RTL  */
#if 0
_stricmp
_strlwr
_strnicmp
_strnset
_strrev
_strset
_strupr
;_vsnprintf
#endif

int _wcsicmp (const wchar_t* cs, const wchar_t* ct);
wchar_t * _wcslwr (wchar_t *x);
int _wcsnicmp (const wchar_t * cs,const wchar_t * ct,size_t count);
wchar_t* _wcsnset (wchar_t* wsToFill, wchar_t wcFill, size_t sizeMaxFill);
wchar_t * _wcsrev(wchar_t *s);
wchar_t *_wcsupr(wchar_t *x);

#if 0
atoi
atol
isdigit
islower
isprint
isspace
isupper
isxdigit
;mbstowcs
;mbtowc
memchr
memcpy
memmove
memset
;qsort
rand
sprintf
srand
strcat
strchr
strcmp
strcpy
strlen
strncat
strncmp
strcpy
strrchr
strspn
strstr
;strtok
;swprintf
tolower
toupper
towlower
towupper
vsprintf
#endif

wchar_t * wcscat(wchar_t *dest, const wchar_t *src);
wchar_t * wcschr(const wchar_t *str, wchar_t ch);
int wcscmp(const wchar_t *cs, const wchar_t *ct);
wchar_t* wcscpy(wchar_t* str1, const wchar_t* str2);
size_t wcscspn(const wchar_t *str,const wchar_t *reject);
size_t wcslen(const wchar_t *s);
wchar_t * wcsncat(wchar_t *dest, const wchar_t *src, size_t count);
int wcsncmp(const wchar_t *cs, const wchar_t *ct, size_t count);
wchar_t * wcsncpy(wchar_t *dest, const wchar_t *src, size_t count);
wchar_t * wcsrchr(const wchar_t *str, wchar_t ch);
size_t wcsspn(const wchar_t *str,const wchar_t *accept);
wchar_t *wcsstr(const wchar_t *s,const wchar_t *b);

#endif /* __DDK_RTL_H */
