/* $Id$
 * 
 */
#ifndef __DDK_RTL_H
#define __DDK_RTL_H

#if defined(__NTOSKRNL__) || defined(__NTDRIVER__) || defined(__NTHAL__) || defined(__NTDLL__) || defined (__NTAPP__)

#include <stddef.h>
#include <stdarg.h>

#endif /* __NTOSKRNL__ || __NTDRIVER__ || __NTHAL__ || __NTDLL__ || __NTAPP__ */

#include <pe.h>
#include <ole32/guiddef.h>

#ifndef __USE_W32API

/*
 * PURPOSE: Flags for RtlQueryRegistryValues
 */
#define RTL_QUERY_REGISTRY_SUBKEY	(0x00000001)
#define RTL_QUERY_REGISTRY_TOPKEY	(0x00000002)
#define RTL_QUERY_REGISTRY_REQUIRED	(0x00000004)
#define RTL_QUERY_REGISTRY_NOVALUE	(0x00000008)
#define RTL_QUERY_REGISTRY_NOEXPAND	(0x00000010)
#define RTL_QUERY_REGISTRY_DIRECT	(0x00000020)
#define RTL_QUERY_REGISTRY_DELETE	(0x00000040)


/*
 * PURPOSE: Flags used by RtlIsTextUnicode and IsTextUnicode
 */
#define IS_TEXT_UNICODE_ASCII16			(0x00000001)
#define IS_TEXT_UNICODE_REVERSE_ASCII16		(0x00000010)
#define IS_TEXT_UNICODE_STATISTICS		(0x00000002)
#define IS_TEXT_UNICODE_REVERSE_STATISTICS	(0x00000020)
#define IS_TEXT_UNICODE_CONTROLS		(0x00000004)
#define IS_TEXT_UNICODE_REVERSE_CONTROLS	(0x00000040)
#define IS_TEXT_UNICODE_SIGNATURE		(0x00000008)
#define IS_TEXT_UNICODE_REVERSE_SIGNATURE	(0x00000080)
#define IS_TEXT_UNICODE_ILLEGAL_CHARS		(0x00000100)
#define IS_TEXT_UNICODE_ODD_LENGTH		(0x00000200)
#define IS_TEXT_UNICODE_NULL_BYTES		(0x00001000)
#define IS_TEXT_UNICODE_UNICODE_MASK		(0x0000000F)
#define IS_TEXT_UNICODE_REVERSE_MASK		(0x000000F0)
#define IS_TEXT_UNICODE_NOT_UNICODE_MASK	(0x00000F00)
#define IS_TEXT_UNICODE_NOT_ASCII_MASK		(0x0000F000)

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


/*
 * VOID
 * InitializeListHead (
 *		PLIST_ENTRY	ListHead
 *		);
 *
 * FUNCTION: Initializes a double linked list
 * ARGUMENTS:
 *         ListHead = Caller supplied storage for the head of the list
 */
static __inline VOID
InitializeListHead(
	IN PLIST_ENTRY  ListHead)
{
	ListHead->Flink = ListHead->Blink = ListHead;
}


/*
 * VOID
 * InsertHeadList (
 *		PLIST_ENTRY	ListHead,
 *		PLIST_ENTRY	Entry
 *		);
 *
 * FUNCTION: Inserts an entry in a double linked list
 * ARGUMENTS:
 *        ListHead = Head of the list
 *        Entry = Entry to insert
 */
static __inline VOID
InsertHeadList(
	IN PLIST_ENTRY  ListHead,
	IN PLIST_ENTRY  Entry)
{
	PLIST_ENTRY OldFlink;
	OldFlink = ListHead->Flink;
	Entry->Flink = OldFlink;
	Entry->Blink = ListHead;
	OldFlink->Blink = Entry;
	ListHead->Flink = Entry;
}


/*
 * VOID
 * InsertTailList (
 *		PLIST_ENTRY	ListHead,
 *		PLIST_ENTRY	Entry
 *		);
 *
 * FUNCTION:
 *	Inserts an entry in a double linked list
 *
 * ARGUMENTS:
 *	ListHead = Head of the list
 *	Entry = Entry to insert
 */
static __inline VOID
InsertTailList(
	IN PLIST_ENTRY  ListHead,
	IN PLIST_ENTRY  Entry)
{
	PLIST_ENTRY OldBlink;
	OldBlink = ListHead->Blink;
	Entry->Flink = ListHead;
	Entry->Blink = OldBlink;
	OldBlink->Flink = Entry;
	ListHead->Blink = Entry;
}

/*
 * BOOLEAN
 * IsListEmpty (
 *	PLIST_ENTRY	ListHead
 *	);
 *
 * FUNCTION:
 *	Checks if a double linked list is empty
 *
 * ARGUMENTS:
 *	ListHead = Head of the list
*/
#define IsListEmpty(ListHead) \
	((ListHead)->Flink == (ListHead))


/*
 * PSINGLE_LIST_ENTRY
 * PopEntryList (
 *	PSINGLE_LIST_ENTRY	ListHead
 *	);
 *
 * FUNCTION:
 *	Removes an entry from the head of a single linked list
 *
 * ARGUMENTS:
 *	ListHead = Head of the list
 *
 * RETURNS:
 *	The removed entry
 */
#define PopEntryList(ListHead) \
	(ListHead)->Next; \
	{ \
		PSINGLE_LIST_ENTRY _FirstEntry; \
		_FirstEntry = (ListHead)->Next; \
		if (_FirstEntry != NULL) \
			(ListHead)->Next = _FirstEntry->Next; \
	}

#define RtlCopyMemory(Destination,Source,Length) \
	memcpy((Destination),(Source),(Length))

#define PushEntryList(_ListHead, _Entry) \
	(_Entry)->Next = (_ListHead)->Next; \
	(_ListHead)->Next = (_Entry); \

/*
 *BOOLEAN
 *RemoveEntryList (
 *	PLIST_ENTRY	Entry
 *	);
 *
 * FUNCTION:
 *	Removes an entry from a double linked list
 *
 * ARGUMENTS:
 *	ListEntry = Entry to remove
 */
static __inline BOOLEAN
RemoveEntryList(
  IN PLIST_ENTRY  Entry)
{
  PLIST_ENTRY OldFlink;
  PLIST_ENTRY OldBlink;

  OldFlink = Entry->Flink;
  OldBlink = Entry->Blink;
  OldFlink->Blink = OldBlink;
  OldBlink->Flink = OldFlink;
  return (OldFlink == OldBlink);
}


/*
 * PLIST_ENTRY
 * RemoveHeadList (
 *	PLIST_ENTRY	ListHead
 *	);
 *
 * FUNCTION:
 *	Removes the head entry from a double linked list
 *
 * ARGUMENTS:
 *	ListHead = Head of the list
 *
 * RETURNS:
 *	The removed entry
 */
static __inline PLIST_ENTRY 
RemoveHeadList(
  IN PLIST_ENTRY  ListHead)
{
  PLIST_ENTRY Flink;
  PLIST_ENTRY Entry;

  Entry = ListHead->Flink;
  Flink = Entry->Flink;
  ListHead->Flink = Flink;
  Flink->Blink = ListHead;
  return Entry;
}


/*
 * PLIST_ENTRY
 * RemoveTailList (
 *	PLIST_ENTRY	ListHead
 *	);
 *
 * FUNCTION:
 *	Removes the tail entry from a double linked list
 *
 * ARGUMENTS:
 *	ListHead = Head of the list
 *
 * RETURNS:
 *	The removed entry
 */
static __inline PLIST_ENTRY
RemoveTailList(
  IN PLIST_ENTRY  ListHead)
{
  PLIST_ENTRY Blink;
  PLIST_ENTRY Entry;

  Entry = ListHead->Blink;
  Blink = Entry->Blink;
  ListHead->Blink = Blink;
  Blink->Flink = ListHead;
  return Entry;
}


/*
 * FIFO versions are slower but ensures that entries with equal SortField value
 * are placed in FIFO order (assuming that entries are removed from Head).
 */

#define InsertAscendingListFIFO(ListHead, Type, ListEntryField, NewEntry, SortField)\
{\
  PLIST_ENTRY current;\
\
  current = (ListHead)->Flink;\
  while (current != (ListHead))\
  {\
    if (CONTAINING_RECORD(current, Type, ListEntryField)->SortField >\
        (NewEntry)->SortField)\
    {\
      break;\
    }\
    current = current->Flink;\
  }\
\
  InsertTailList(current, &((NewEntry)->ListEntryField));\
}


#define InsertDescendingListFIFO(ListHead, Type, ListEntryField, NewEntry, SortField)\
{\
  PLIST_ENTRY current;\
\
  current = (ListHead)->Flink;\
  while (current != (ListHead))\
  {\
    if (CONTAINING_RECORD(current, Type, ListEntryField)->SortField <\
        (NewEntry)->SortField)\
    {\
      break;\
    }\
    current = current->Flink;\
  }\
\
  InsertTailList(current, &((NewEntry)->ListEntryField));\
}


#define InsertAscendingList(ListHead, Type, ListEntryField, NewEntry, SortField)\
{\
  PLIST_ENTRY current;\
\
  current = (ListHead)->Flink;\
  while (current != (ListHead))\
  {\
    if (CONTAINING_RECORD(current, Type, ListEntryField)->SortField >=\
        (NewEntry)->SortField)\
    {\
      break;\
    }\
    current = current->Flink;\
  }\
\
  InsertTailList(current, &((NewEntry)->ListEntryField));\
}


#define InsertDescendingList(ListHead, Type, ListEntryField, NewEntry, SortField)\
{\
  PLIST_ENTRY current;\
\
  current = (ListHead)->Flink;\
  while (current != (ListHead))\
  {\
    if (CONTAINING_RECORD(current, Type, ListEntryField)->SortField <=\
        (NewEntry)->SortField)\
    {\
      break;\
    }\
    current = current->Flink;\
  }\
\
  InsertTailList(current, &((NewEntry)->ListEntryField));\
}


/*
 * BOOLEAN
 * IsXstEntry (
 *  PLIST_ENTRY ListHead,
 *  PLIST_ENTRY Entry
 *  );
*/
#define IsFirstEntry(ListHead, Entry) ((ListHead)->Flink == Entry)
  
#define IsLastEntry(ListHead, Entry) ((ListHead)->Blink == Entry)
  
#define RtlEqualMemory(Destination,Source,Length)   (!memcmp((Destination), (Source), (Length)))

NTSTATUS
STDCALL
RtlAppendUnicodeToString (
	PUNICODE_STRING	Destination,
	PCWSTR		Source
	);

SIZE_T STDCALL
RtlCompareMemory(IN const VOID *Source1,
                 IN const VOID *Source2,
                 IN SIZE_T Length);

BOOLEAN
STDCALL
RtlEqualUnicodeString (
	PUNICODE_STRING	String1,
	PUNICODE_STRING	String2,
	BOOLEAN		CaseInSensitive
	);

NTSTATUS
STDCALL
RtlQueryRegistryValues (
	IN	ULONG				RelativeTo,
	IN	PCWSTR				Path,
	IN	PRTL_QUERY_REGISTRY_TABLE	QueryTable,
	IN	PVOID				Context,
	IN	PVOID				Environment
	);

NTSTATUS
STDCALL
RtlWriteRegistryValue (
	ULONG	RelativeTo,
	PCWSTR	Path,
	PCWSTR	ValueName,
	ULONG	ValueType,
	PVOID	ValueData,
	ULONG	ValueLength
	);

NTSTATUS STDCALL
RtlDeleteRegistryValue(IN ULONG RelativeTo,
		       IN PCWSTR Path,
		       IN PCWSTR ValueName);

VOID STDCALL
RtlMoveMemory (PVOID Destination, CONST VOID* Source, ULONG Length);

BOOLEAN STDCALL
RtlEqualLuid(IN PLUID Luid1,
	     IN PLUID Luid2);

VOID
STDCALL
RtlFillMemory (
	PVOID	Destination,
	ULONG	Length,
	UCHAR	Fill
	);

VOID STDCALL
RtlZeroMemory (PVOID Destination, ULONG Length);

#else /* __USE_W32API */

#include <ddk/ntifs.h>

#endif /* __USE_W32API */


/*
 * PURPOSE: Used with RtlCheckRegistryKey, RtlCreateRegistryKey, 
 * RtlDeleteRegistryKey
 */
#define RTL_REGISTRY_ABSOLUTE   0
#define RTL_REGISTRY_SERVICES   1
#define RTL_REGISTRY_CONTROL    2
#define RTL_REGISTRY_WINDOWS_NT 3
#define RTL_REGISTRY_DEVICEMAP  4
#define RTL_REGISTRY_USER       5
#define RTL_REGISTRY_ENUM       6   /* ReactOS specific: Used internally in kernel only */
#define RTL_REGISTRY_MAXIMUM    7

#define RTL_REGISTRY_HANDLE     0x40000000
#define RTL_REGISTRY_OPTIONAL   0x80000000


#define SHORT_SIZE	(sizeof(USHORT))
#define SHORT_MASK	(SHORT_SIZE-1)
#define LONG_SIZE	(sizeof(ULONG))
#define LONG_MASK	(LONG_SIZE-1)
#define LOWBYTE_MASK	0x00FF

#define FIRSTBYTE(Value)	((Value) & LOWBYTE_MASK)
#define SECONDBYTE(Value)	(((Value) >> 8) & LOWBYTE_MASK)
#define THIRDBYTE(Value)	(((Value) >> 16) & LOWBYTE_MASK)
#define FOURTHBYTE(Value)	(((Value) >> 24) & LOWBYTE_MASK)

/* FIXME: reverse byte-order on big-endian machines (e.g. MIPS) */
#define SHORT_LEAST_SIGNIFICANT_BIT	0
#define SHORT_MOST_SIGNIFICANT_BIT	1

#define LONG_LEAST_SIGNIFICANT_BIT	0
#define LONG_3RD_MOST_SIGNIFICANT_BIT	1
#define LONG_2RD_MOST_SIGNIFICANT_BIT	2
#define LONG_MOST_SIGNIFICANT_BIT	3


#define NLS_ANSI_CODE_PAGE       NlsAnsiCodePage
#define NLS_LEAD_BYTE_INFO       NlsLeadByteInfo
#define NLS_MB_CODE_PAGE_TAG     NlsMbCodePageTag
#define NLS_MB_OEM_CODE_PAGE_TAG NlsMbOemCodePageTag
#define NLS_OEM_LEAD_BYTE_INFO   NlsOemLeadByteInfo

#if defined(__NTOSKRNL__) || defined(__NTDLL__)
extern USHORT  EXPORTED NlsAnsiCodePage;
extern PUSHORT EXPORTED NlsLeadByteInfo;
extern BOOLEAN EXPORTED NlsMbCodePageTag;
extern BOOLEAN EXPORTED NlsMbOemCodePageTag;
extern PUSHORT EXPORTED NlsOemLeadByteInfo;
#else
extern USHORT  IMPORTED NlsAnsiCodePage;
extern PUSHORT IMPORTED NlsLeadByteInfo;
extern BOOLEAN IMPORTED NlsMbCodePageTag;
extern BOOLEAN IMPORTED NlsMbOemCodePageTag;
extern PUSHORT IMPORTED NlsOemLeadByteInfo;
#endif /* __NTOSKRNL__ || __NTDLL__ */

/*
VOID
PushEntryList (
	PSINGLE_LIST_ENTRY	ListHead,
	PSINGLE_LIST_ENTRY	Entry
	);
*/
/*
#define PushEntryList(ListHead,Entry) \
	(Entry)->Next = (ListHead)->Next; \
	(ListHead)->Next = (Entry)
*/


NTSTATUS STDCALL
RtlAbsoluteToSelfRelativeSD (PSECURITY_DESCRIPTOR AbsSD,
			     PSECURITY_DESCRIPTOR RelSD,
			     PULONG BufferLength);

NTSTATUS STDCALL
RtlAddAccessAllowedAce (PACL Acl,
			ULONG Revision,
			ACCESS_MASK AccessMask,
			PSID Sid);

NTSTATUS 
STDCALL 
RtlAddAccessAllowedAceEx(
	IN OUT PACL pAcl,
	IN DWORD dwAceRevision,
	IN DWORD AceFlags,
	IN DWORD AccessMask,
	IN PSID pSid);


NTSTATUS STDCALL
RtlAddAccessDeniedAce (PACL Acl,
		       ULONG Revision,
		       ACCESS_MASK AccessMask,
		       PSID Sid);

NTSTATUS STDCALL
RtlAddAce (PACL Acl,
	   ULONG Revision,
	   ULONG StartingIndex,
	   PACE AceList,
	   ULONG AceListLength);

NTSTATUS STDCALL
RtlAddAtomToAtomTable (IN PRTL_ATOM_TABLE AtomTable,
		       IN PWSTR AtomName,
		       OUT PRTL_ATOM Atom);

NTSTATUS STDCALL
RtlAddAuditAccessAce (PACL Acl,
		      ULONG Revision,
		      ACCESS_MASK AccessMask,
		      PSID Sid,
		      BOOLEAN Success,
		      BOOLEAN Failure);

NTSTATUS STDCALL
RtlAddRange (IN OUT PRTL_RANGE_LIST RangeList,
	     IN ULONGLONG Start,
	     IN ULONGLONG End,
	     IN UCHAR Attributes,
	     IN ULONG Flags,  /* RTL_RANGE_LIST_ADD_... flags */
	     IN PVOID UserData OPTIONAL,
	     IN PVOID Owner OPTIONAL);

NTSTATUS STDCALL
RtlAllocateAndInitializeSid (IN PSID_IDENTIFIER_AUTHORITY IdentifierAuthority,
			     IN UCHAR SubAuthorityCount,
			     IN ULONG SubAuthority0,
			     IN ULONG SubAuthority1,
			     IN ULONG SubAuthority2,
			     IN ULONG SubAuthority3,
			     IN ULONG SubAuthority4,
			     IN ULONG SubAuthority5,
			     IN ULONG SubAuthority6,
			     IN ULONG SubAuthority7,
			     OUT PSID *Sid);

PVOID STDCALL
RtlAllocateHeap (
	HANDLE	Heap,
	ULONG	Flags,
	ULONG	Size
	);

WCHAR STDCALL
RtlAnsiCharToUnicodeChar (IN CHAR AnsiChar);

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

BOOLEAN STDCALL
RtlAreAllAccessesGranted (ACCESS_MASK GrantedAccess,
			  ACCESS_MASK DesiredAccess);

BOOLEAN STDCALL
RtlAreAnyAccessesGranted (ACCESS_MASK GrantedAccess,
			  ACCESS_MASK DesiredAccess);

BOOLEAN
STDCALL
RtlAreBitsClear (
	PRTL_BITMAP	BitMapHeader,
	ULONG		StartingIndex,
	ULONG		Length
	);

BOOLEAN
STDCALL
RtlAreBitsSet (
	PRTL_BITMAP	BitMapHeader,
	ULONG		StartingIndex,
	ULONG		Length
	);

VOID
STDCALL
RtlAssert (
	PVOID FailedAssertion,
	PVOID FileName,
	ULONG LineNumber,
	PCHAR Message
	);

VOID
STDCALL
RtlCaptureContext (
	OUT PCONTEXT ContextRecord
	);

USHORT
STDCALL
RtlCaptureStackBackTrace (
	IN ULONG FramesToSkip,
	IN ULONG FramesToCapture,
	OUT PVOID *BackTrace,
	OUT PULONG BackTraceHash OPTIONAL
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

VOID
STDCALL
RtlClearAllBits (
	IN	PRTL_BITMAP	BitMapHeader
	);

VOID
STDCALL
RtlClearBit (
	PRTL_BITMAP BitMapHeader,
	ULONG BitNumber
	);

VOID
STDCALL
RtlClearBits (
	IN	PRTL_BITMAP	BitMapHeader,
	IN	ULONG		StartingIndex,
	IN	ULONG		NumberToClear
	);

ULONG STDCALL
RtlCompactHeap (
	HANDLE Heap,
	ULONG	Flags
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
	BOOLEAN		CaseInsensitive
	);

NTSTATUS STDCALL
RtlCompressBuffer(IN USHORT CompressionFormatAndEngine,
		  IN PUCHAR UncompressedBuffer,
		  IN ULONG UncompressedBufferSize,
		  OUT PUCHAR CompressedBuffer,
		  IN ULONG CompressedBufferSize,
		  IN ULONG UncompressedChunkSize,
		  OUT PULONG FinalCompressedSize,
		  IN PVOID WorkSpace);

NTSTATUS STDCALL
RtlCompressChunks(IN PUCHAR UncompressedBuffer,
		  IN ULONG UncompressedBufferSize,
		  OUT PUCHAR CompressedBuffer,
		  IN ULONG CompressedBufferSize,
		  IN OUT PCOMPRESSED_DATA_INFO CompressedDataInfo,
		  IN ULONG CompressedDataInfoLength,
		  IN PVOID WorkSpace);

LARGE_INTEGER STDCALL
RtlConvertLongToLargeInteger (IN LONG SignedInteger);

NTSTATUS STDCALL
RtlConvertSidToUnicodeString (IN OUT PUNICODE_STRING String,
			      IN PSID Sid,
			      IN BOOLEAN AllocateString);

LARGE_INTEGER STDCALL
RtlConvertUlongToLargeInteger (IN ULONG UnsignedInteger);

#if 0
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
#endif

#define RtlCopyBytes RtlCopyMemory

VOID STDCALL
RtlCopyLuid(IN PLUID LuidDest,
	    IN PLUID LuidSrc);

VOID STDCALL
RtlCopyLuidAndAttributesArray(ULONG Count,
			      PLUID_AND_ATTRIBUTES Src,
			      PLUID_AND_ATTRIBUTES Dest);

NTSTATUS STDCALL
RtlCopyRangeList (OUT PRTL_RANGE_LIST CopyRangeList,
		  IN PRTL_RANGE_LIST RangeList);

NTSTATUS STDCALL
RtlCopySid(ULONG BufferLength,
	   PSID Dest,
	   PSID Src);

NTSTATUS STDCALL
RtlCopySidAndAttributesArray(ULONG Count,
			     PSID_AND_ATTRIBUTES Src,
			     ULONG SidAreaSize,
			     PSID_AND_ATTRIBUTES Dest,
			     PVOID SidArea,
			     PVOID* RemainingSidArea,
			     PULONG RemainingSidAreaSize);

VOID STDCALL
RtlCopyString(PSTRING DestinationString,
	      PSTRING SourceString);

VOID STDCALL
RtlCopyUnicodeString(PUNICODE_STRING DestinationString,
		     PUNICODE_STRING SourceString);

NTSTATUS STDCALL
RtlCreateAcl (PACL Acl,
	      ULONG AclSize,
	      ULONG AclRevision);

NTSTATUS STDCALL
RtlCreateAtomTable(IN ULONG TableSize,
		   IN OUT PRTL_ATOM_TABLE *AtomTable);

HANDLE
STDCALL
RtlCreateHeap (
	ULONG			Flags,
	PVOID			BaseAddress,
	ULONG			SizeToReserve,     /* dwMaximumSize */
	ULONG			SizeToCommit,      /* dwInitialSize */
	PVOID			Unknown,
	PRTL_HEAP_DEFINITION	Definition
	);

NTSTATUS STDCALL
RtlCreateRegistryKey (ULONG RelativeTo,
		      PWSTR Path);

NTSTATUS STDCALL
RtlCreateSecurityDescriptor (PSECURITY_DESCRIPTOR SecurityDescriptor,
			     ULONG Revision);

NTSTATUS
STDCALL
RtlCreateSystemVolumeInformationFolder(
	IN  PUNICODE_STRING VolumeRootPath
	);

BOOLEAN STDCALL
RtlCreateUnicodeString (OUT PUNICODE_STRING Destination,
			IN PCWSTR Source);

BOOLEAN STDCALL
RtlCreateUnicodeStringFromAsciiz (OUT PUNICODE_STRING Destination,
				  IN PCSZ Source);

NTSTATUS
STDCALL
RtlCustomCPToUnicodeN (
	IN	PCPTABLEINFO	CustomCP,
	PWCHAR		UnicodeString,
	ULONG		UnicodeSize,
	PULONG		ResultSize,
	PCHAR		CustomString,
	ULONG		CustomSize
	);

BOOLEAN STDCALL
RtlCutoverTimeToSystemTime(IN PTIME_FIELDS CutoverTimeFields,
                           OUT PLARGE_INTEGER SystemTime,
                           IN PLARGE_INTEGER CurrentTime,
                           IN BOOLEAN ThisYearsCutoverOnly);

NTSTATUS STDCALL
RtlDecompressBuffer(IN USHORT CompressionFormat,
		    OUT PUCHAR UncompressedBuffer,
		    IN ULONG UncompressedBufferSize,
		    IN PUCHAR CompressedBuffer,
		    IN ULONG CompressedBufferSize,
		    OUT PULONG FinalUncompressedSize);

NTSTATUS STDCALL
RtlDecompressChunks(OUT PUCHAR UncompressedBuffer,
		    IN ULONG UncompressedBufferSize,
		    IN PUCHAR CompressedBuffer,
		    IN ULONG CompressedBufferSize,
		    IN PUCHAR CompressedTail,
		    IN ULONG CompressedTailSize,
		    IN PCOMPRESSED_DATA_INFO CompressedDataInfo);

NTSTATUS STDCALL
RtlDecompressFragment(IN USHORT CompressionFormat,
		      OUT PUCHAR UncompressedFragment,
		      IN ULONG UncompressedFragmentSize,
		      IN PUCHAR CompressedBuffer,
		      IN ULONG CompressedBufferSize,
		      IN ULONG FragmentOffset,
		      OUT PULONG FinalUncompressedSize,
		      IN PVOID WorkSpace);

PRTL_SPLAY_LINKS
STDCALL
RtlDelete (
	PRTL_SPLAY_LINKS Links
	);

NTSTATUS STDCALL
RtlDeleteAce (PACL Acl,
	      ULONG AceIndex);

NTSTATUS STDCALL
RtlDeleteAtomFromAtomTable (IN PRTL_ATOM_TABLE AtomTable,
			    IN RTL_ATOM Atom);

BOOLEAN
STDCALL
RtlDeleteElementGenericTable (
	PRTL_GENERIC_TABLE Table,
	PVOID Buffer
	);

BOOLEAN
STDCALL
RtlDeleteElementGenericTableAvl (
	PRTL_AVL_TABLE Table,
	PVOID Buffer
	);

VOID
STDCALL
RtlDeleteNoSplay (
	PRTL_SPLAY_LINKS Links,
	PRTL_SPLAY_LINKS *Root
	);


NTSTATUS STDCALL
RtlDeleteOwnersRanges (IN OUT PRTL_RANGE_LIST RangeList,
		       IN PVOID Owner);

NTSTATUS STDCALL
RtlDeleteRange (IN OUT PRTL_RANGE_LIST RangeList,
		IN ULONGLONG Start,
		IN ULONGLONG End,
		IN PVOID Owner);

NTSTATUS STDCALL
RtlDescribeChunk(IN USHORT CompressionFormat,
		 IN OUT PUCHAR *CompressedBuffer,
		 IN PUCHAR EndOfCompressedBufferPlus1,
		 OUT PUCHAR *ChunkBuffer,
		 OUT PULONG ChunkSize);

NTSTATUS STDCALL
RtlDestroyAtomTable (IN PRTL_ATOM_TABLE AtomTable);

HANDLE STDCALL
RtlDestroyHeap (HANDLE hheap);

NTSTATUS
STDCALL
RtlDispatchException(
	PEXCEPTION_RECORD pExcptRec, 
	CONTEXT * pContext 
	);


NTSTATUS
STDCALL
RtlDowncaseUnicodeString (
	IN OUT PUNICODE_STRING	DestinationString,
	IN PUNICODE_STRING	SourceString,
	IN BOOLEAN		AllocateDestinationString
	);

NTSTATUS
STDCALL
RtlEmptyAtomTable (
	IN	PRTL_ATOM_TABLE	AtomTable,
	IN	BOOLEAN		DeletePinned
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

PVOID
STDCALL
RtlEnumerateGenericTable (
	PRTL_GENERIC_TABLE Table,
	BOOLEAN Restart
	);
	
PVOID
STDCALL
RtlEnumerateGenericTableAvl (
	PRTL_AVL_TABLE Table,
	BOOLEAN Restart
	);

PVOID
STDCALL
RtlEnumerateGenericTableLikeADirectory (
	IN PRTL_AVL_TABLE Table,
	IN PRTL_AVL_MATCH_FUNCTION MatchFunction,
	IN PVOID MatchData,
	IN ULONG NextFlag,
	IN OUT PVOID *RestartKey,
	IN OUT PULONG DeleteCount,
	IN OUT PVOID Buffer
	);

PVOID
STDCALL
RtlEnumerateGenericTableWithoutSplaying (
	PRTL_GENERIC_TABLE Table,
	PVOID *RestartKey
	);

PVOID
STDCALL
RtlEnumerateGenericTableWithoutSplayingAvl (
	PRTL_AVL_TABLE Table,
	PVOID *RestartKey
	);

BOOLEAN STDCALL
RtlEqualPrefixSid (PSID Sid1,
		   PSID Sid2);

BOOLEAN STDCALL
RtlEqualSid (PSID Sid1,
	     PSID Sid2);

BOOLEAN
STDCALL
RtlEqualString (
	PSTRING	String1,
	PSTRING	String2,
	BOOLEAN	CaseInSensitive
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
RtlFillMemoryUlong (
	PVOID	Destination,
	ULONG	Length,
	ULONG	Fill
	);

ULONG
STDCALL
RtlFindClearBits (
	PRTL_BITMAP	BitMapHeader,
	ULONG		NumberToFind,
	ULONG		HintIndex
	);

ULONG
STDCALL
RtlFindClearBitsAndSet (
	PRTL_BITMAP	BitMapHeader,
	ULONG		NumberToFind,
	ULONG		HintIndex
	);

ULONG
STDCALL
RtlFindClearRuns (
	PRTL_BITMAP BitMapHeader,
	PRTL_BITMAP_RUN RunArray,
	ULONG SizeOfRunArray,
	BOOLEAN LocateLongestRuns
	);

ULONG
STDCALL
RtlFindLastBackwardRunClear (
	IN PRTL_BITMAP BitMapHeader,
	IN ULONG FromIndex,
	IN PULONG StartingRunIndex
	);

ULONG
STDCALL
RtlFindNextForwardRunClear (
	IN PRTL_BITMAP BitMapHeader,
	IN ULONG FromIndex,
	IN PULONG StartingRunIndex
	);
	

PUNICODE_PREFIX_TABLE_ENTRY
STDCALL
RtlFindUnicodePrefix (
	PUNICODE_PREFIX_TABLE PrefixTable,
	PUNICODE_STRING FullName,
	ULONG CaseInsensitiveIndex
	);

ULONG
STDCALL
RtlFindFirstRunClear (
	PRTL_BITMAP	BitMapHeader,
	PULONG		StartingIndex
	);

ULONG
STDCALL
RtlFindFirstRunSet (
	PRTL_BITMAP	BitMapHeader,
	PULONG		StartingIndex
	);

CCHAR STDCALL
RtlFindLeastSignificantBit (IN ULONGLONG Set);

ULONG
STDCALL
RtlFindLongestRunClear (
	PRTL_BITMAP	BitMapHeader,
	PULONG		StartingIndex
	);

ULONG
STDCALL
RtlFindLongestRunSet (
	PRTL_BITMAP	BitMapHeader,
	PULONG		StartingIndex
	);

NTSTATUS
STDCALL
RtlFindMessage (
	IN	PVOID				BaseAddress,
	IN	ULONG				Type,
	IN	ULONG				Language,
	IN	ULONG				MessageId,
	OUT	PRTL_MESSAGE_RESOURCE_ENTRY	*MessageResourceEntry
	);

CCHAR STDCALL
RtlFindMostSignificantBit (IN ULONGLONG Set);

NTSTATUS STDCALL
RtlFindRange (IN PRTL_RANGE_LIST RangeList,
	      IN ULONGLONG Minimum,
	      IN ULONGLONG Maximum,
	      IN ULONG Length,
	      IN ULONG Alignment,
	      IN ULONG Flags,
	      IN UCHAR AttributeAvailableMask,
	      IN PVOID Context OPTIONAL,
	      IN PRTL_CONFLICT_RANGE_CALLBACK Callback OPTIONAL,
	      OUT PULONGLONG Start);

ULONG
STDCALL
RtlFindSetBits (
	PRTL_BITMAP	BitMapHeader,
	ULONG		NumberToFind,
	ULONG		HintIndex
	);

ULONG
STDCALL
RtlFindSetBitsAndClear (
	PRTL_BITMAP	BitMapHeader,
	ULONG		NumberToFind,
	ULONG		HintIndex
	);

BOOLEAN STDCALL
RtlFirstFreeAce (PACL Acl,
		 PACE* Ace);

NTSTATUS STDCALL
RtlFormatCurrentUserKeyPath (IN OUT PUNICODE_STRING KeyPath);

VOID STDCALL
RtlFreeAnsiString (IN PANSI_STRING AnsiString);

BOOLEAN
STDCALL
RtlFreeHeap (
	HANDLE	Heap,
	ULONG	Flags,
	PVOID	Address
	);

VOID STDCALL
RtlFreeOemString (IN POEM_STRING OemString);

VOID STDCALL
RtlFreeRangeList (IN PRTL_RANGE_LIST RangeList);

PVOID STDCALL
RtlFreeSid (PSID Sid);

VOID STDCALL
RtlFreeUnicodeString (IN PUNICODE_STRING UnicodeString);

VOID STDCALL
RtlGenerate8dot3Name (IN PUNICODE_STRING Name,
		      IN BOOLEAN AllowExtendedCharacters,
		      IN OUT PGENERATE_NAME_CONTEXT Context,
		      OUT PUNICODE_STRING Name8dot3);

NTSTATUS STDCALL
RtlGetAce (PACL Acl,
	   ULONG AceIndex,
	   PACE *Ace);

VOID
STDCALL
RtlGetCallersAddress(
	OUT PVOID *CallersAddress,
	OUT PVOID *CallersCaller
	);

NTSTATUS STDCALL
RtlGetCompressionWorkSpaceSize (IN USHORT CompressionFormatAndEngine,
				OUT PULONG CompressBufferAndWorkSpaceSize,
				OUT PULONG CompressFragmentWorkSpaceSize);

NTSTATUS STDCALL
RtlGetControlSecurityDescriptor (PSECURITY_DESCRIPTOR SecurityDescriptor,
				 PSECURITY_DESCRIPTOR_CONTROL Control,
				 PULONG Revision);

NTSTATUS STDCALL
RtlGetDaclSecurityDescriptor (PSECURITY_DESCRIPTOR SecurityDescriptor,
			      PBOOLEAN DaclPresent,
			      PACL* Dacl,
			      PBOOLEAN DaclDefaulted);

VOID STDCALL
RtlGetDefaultCodePage (OUT PUSHORT AnsiCodePage,
		       OUT PUSHORT OemCodePage);

PVOID
STDCALL
RtlGetElementGenericTable(
	PRTL_GENERIC_TABLE Table,
	ULONG I
	);

PVOID
STDCALL
RtlGetElementGenericTableAvl (
	PRTL_AVL_TABLE Table,
	ULONG I
	);

NTSTATUS STDCALL
RtlGetFirstRange (IN PRTL_RANGE_LIST RangeList,
		  OUT PRTL_RANGE_LIST_ITERATOR Iterator,
		  OUT PRTL_RANGE *Range);

NTSTATUS STDCALL
RtlGetGroupSecurityDescriptor (PSECURITY_DESCRIPTOR SecurityDescriptor,
			       PSID* Group,
			       PBOOLEAN GroupDefaulted);

NTSTATUS STDCALL
RtlGetNextRange (IN OUT PRTL_RANGE_LIST_ITERATOR Iterator,
		 OUT PRTL_RANGE *Range,
		 IN BOOLEAN MoveForwards);

ULONG
STDCALL
RtlGetNtGlobalFlags (
	VOID
	); 

NTSTATUS STDCALL
RtlGetOwnerSecurityDescriptor (PSECURITY_DESCRIPTOR SecurityDescriptor,
			       PSID* Owner,
			       PBOOLEAN OwnerDefaulted);

NTSTATUS STDCALL
RtlGetSaclSecurityDescriptor (PSECURITY_DESCRIPTOR SecurityDescriptor,
			      PBOOLEAN SaclPresent,
			      PACL* Sacl,
			      PBOOLEAN SaclDefaulted);

NTSTATUS
STDCALL
RtlGetSetBootStatusData(
	HANDLE Filehandle,
	BOOLEAN WriteMode,
	DWORD DataClass,
	PVOID Buffer,
	ULONG BufferSize,
	DWORD DataClass2
	);

NTSTATUS STDCALL
RtlGUIDFromString (IN PUNICODE_STRING GuidString,
		   OUT GUID* Guid);

NTSTATUS
STDCALL
RtlHashUnicodeString(
	IN const UNICODE_STRING *String,
	IN BOOLEAN CaseInSensitive,
	IN ULONG HashAlgorithm,
	OUT PULONG HashValue
	);

PSID_IDENTIFIER_AUTHORITY STDCALL
RtlIdentifierAuthoritySid (PSID Sid);

PVOID
STDCALL
RtlImageDirectoryEntryToData (
	PVOID	BaseAddress,
	BOOLEAN	bFlag,
	ULONG	Directory,
	PULONG	Size
	);

PIMAGE_NT_HEADERS
STDCALL
RtlImageNtHeader (
	IN	PVOID	BaseAddress
	);

PIMAGE_SECTION_HEADER
STDCALL
RtlImageRvaToSection (
	PIMAGE_NT_HEADERS	NtHeader,
	PVOID			BaseAddress,
	ULONG			Rva
	);

ULONG
STDCALL
RtlImageRvaToVa (
	PIMAGE_NT_HEADERS	NtHeader,
	PVOID			BaseAddress,
	ULONG			Rva,
	PIMAGE_SECTION_HEADER	*SectionHeader
	);

VOID
STDCALL
RtlInitAnsiString (
	PANSI_STRING	DestinationString,
	PCSZ		SourceString
	);

VOID
STDCALL
RtlInitCodePageTable (
	IN	PUSHORT		TableBase,
	OUT	PCPTABLEINFO	CodePageTable
	);

VOID
STDCALL
RtlInitializeUnicodePrefix (
	PUNICODE_PREFIX_TABLE PrefixTable
	);

NTSTATUS STDCALL
RtlInitializeSid (PSID Sid,
		  PSID_IDENTIFIER_AUTHORITY IdentifierAuthority,
		  UCHAR SubAuthorityCount);

VOID
STDCALL
RtlInitializeBitMap(
  IN PRTL_BITMAP  BitMapHeader,
  IN PULONG  BitMapBuffer,
  IN ULONG  SizeOfBitMap); 
  
VOID
STDCALL
RtlInitNlsTables (
	IN	PUSHORT		AnsiTableBase,
	IN	PUSHORT		OemTableBase,
	IN	PUSHORT		CaseTableBase,
	OUT	PNLSTABLEINFO	NlsTable
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

VOID
STDCALL
RtlInitializeBitMap (
	IN OUT	PRTL_BITMAP	BitMapHeader,
	IN	PULONG		BitMapBuffer,
	IN	ULONG		SizeOfBitMap
	);

NTSTATUS
STDCALL
RtlInitializeContext (
	IN	HANDLE			ProcessHandle,
	OUT	PCONTEXT		ThreadContext,
	IN	PVOID			ThreadStartParam  OPTIONAL,
	IN	PTHREAD_START_ROUTINE	ThreadStartAddress,
	IN	PINITIAL_TEB		InitialTeb
	);

VOID
STDCALL
RtlInitializeGenericTable (
	PRTL_GENERIC_TABLE Table,
	PRTL_GENERIC_COMPARE_ROUTINE CompareRoutine,
	PRTL_GENERIC_ALLOCATE_ROUTINE AllocateRoutine,
	PRTL_GENERIC_FREE_ROUTINE FreeRoutine,
	PVOID TableContext
	);

VOID
STDCALL
RtlInitializeGenericTableAvl (
	PRTL_AVL_TABLE Table,
	PRTL_AVL_COMPARE_ROUTINE CompareRoutine,
	PRTL_AVL_ALLOCATE_ROUTINE AllocateRoutine,
	PRTL_AVL_FREE_ROUTINE FreeRoutine,
	PVOID TableContext
	);

VOID STDCALL
RtlInitializeRangeList (IN OUT PRTL_RANGE_LIST RangeList);

PVOID
STDCALL
RtlInsertElementGenericTable (
	PRTL_GENERIC_TABLE Table,
	PVOID Buffer,
	ULONG BufferSize,
	PBOOLEAN NewElement OPTIONAL
	);

PVOID
STDCALL
RtlInsertElementGenericTableAvl (
	PRTL_AVL_TABLE Table,
	PVOID Buffer,
	ULONG BufferSize,
	PBOOLEAN NewElement OPTIONAL
	);

PVOID
STDCALL
RtlInsertElementGenericTableFull (
	PRTL_GENERIC_TABLE Table,
	PVOID Buffer,
	ULONG BufferSize,
	PBOOLEAN NewElement OPTIONAL,
	PVOID NodeOrParent,
	TABLE_SEARCH_RESULT SearchResult
	);

PVOID
STDCALL
RtlInsertElementGenericTableFullAvl (
	PRTL_AVL_TABLE Table,
	PVOID Buffer,
	ULONG BufferSize,
	PBOOLEAN NewElement OPTIONAL,
	PVOID NodeOrParent,
	TABLE_SEARCH_RESULT SearchResult
	);

BOOLEAN
STDCALL
RtlInsertUnicodePrefix (
	PUNICODE_PREFIX_TABLE PrefixTable,
	PUNICODE_STRING Prefix,
	PUNICODE_PREFIX_TABLE_ENTRY PrefixTableEntry
	);

NTSTATUS
STDCALL
RtlInt64ToUnicodeString (
	IN ULONGLONG Value,
	IN ULONG Base OPTIONAL,
	IN OUT PUNICODE_STRING String
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
RtlIntegerToUnicode(
	IN ULONG Value,
	IN ULONG Base  OPTIONAL,
	IN ULONG Length OPTIONAL,
	IN OUT LPWSTR String
	);

NTSTATUS
STDCALL
RtlIntegerToUnicodeString (
	IN	ULONG		Value,
	IN	ULONG		Base,
	IN OUT	PUNICODE_STRING	String
	);

NTSTATUS STDCALL
RtlInvertRangeList (OUT PRTL_RANGE_LIST InvertedRangeList,
		    IN PRTL_RANGE_LIST RangeList);

LPSTR
STDCALL
RtlIpv4AddressToStringA(
	PULONG IP,
	LPSTR Buffer
	);

NTSTATUS
STDCALL
RtlIpv4AddressToStringExA(
	PULONG IP,
	PULONG Port,
	LPSTR Buffer,
	PULONG MaxSize
	);

LPWSTR
STDCALL
RtlIpv4AddressToStringW(
	PULONG IP,
	LPWSTR Buffer
	);

NTSTATUS
STDCALL
RtlIpv4AddressToStringExW(
	PULONG IP,
	PULONG Port,
	LPWSTR Buffer,
	PULONG MaxSize
	);

NTSTATUS
STDCALL
RtlIpv4StringToAddressA(
	IN LPSTR IpString,
	IN ULONG Base,
	OUT PVOID PtrToIpAddr,
	OUT ULONG IpAddr
	);

NTSTATUS
STDCALL
RtlIpv4StringToAddressExA(
	IN LPSTR IpString,
	IN ULONG Base,
	OUT PULONG IpAddr,
	OUT PULONG Port
	);

NTSTATUS
STDCALL
RtlIpv4StringToAddressW(
	IN LPWSTR IpString, 
	IN ULONG Base, 
	OUT PVOID PtrToIpAddr,
	OUT ULONG IpAddr
	);

NTSTATUS
STDCALL
RtlIpv4StringToAddressExW(
	IN LPWSTR IpString,
	IN ULONG Base,
	OUT PULONG IpAddr,
	OUT PULONG Port
	);

NTSTATUS
STDCALL
RtlIpv6AddressToStringA(
	PULONG IP,
	LPSTR Buffer
	);

NTSTATUS
STDCALL
RtlIpv6AddressToStringExA(
	PULONG IP,
	PULONG Port,
	LPSTR Buffer,
	PULONG MaxSize
	);

NTSTATUS
STDCALL
RtlIpv6AddressToStringW(
	PULONG IP,
	LPWSTR Buffer
	);

NTSTATUS
STDCALL
RtlIpv6AddressToStringExW(
	PULONG IP,
	PULONG Port,
	LPWSTR Buffer,
	PULONG MaxSize
	);

NTSTATUS
STDCALL
RtlIpv6StringToAddressA(
	IN LPSTR IpString,
	IN ULONG Base,
	OUT PVOID PtrToIpAddr,
	OUT ULONG IpAddr
	);

NTSTATUS
STDCALL
RtlIpv6StringToAddressExA(
	IN LPSTR IpString,
	IN ULONG Base,
	OUT PULONG IpAddr,
	OUT PULONG Port
	);

NTSTATUS
STDCALL
RtlIpv6StringToAddressW(
	IN LPWSTR IpString,
	IN ULONG Base,
	OUT PVOID PtrToIpAddr,
	OUT ULONG IpAddr
	);

NTSTATUS
STDCALL
RtlIpv6StringToAddressExW(
	IN LPWSTR IpString,
	IN ULONG Base,
	OUT PULONG IpAddr,
	OUT PULONG Port
	);

BOOLEAN
STDCALL
RtlIsGenericTableEmpty (
	PRTL_GENERIC_TABLE Table
	);

BOOLEAN
STDCALL
RtlIsGenericTableEmptyAvl (
	PRTL_AVL_TABLE Table
	);


BOOLEAN STDCALL
RtlIsNameLegalDOS8Dot3 (IN PUNICODE_STRING UnicodeName,
			IN PANSI_STRING AnsiName,
			OUT PBOOLEAN SpacesFound);

NTSTATUS STDCALL
RtlIsRangeAvailable (IN PRTL_RANGE_LIST RangeList,
		     IN ULONGLONG Start,
		     IN ULONGLONG End,
		     IN ULONG Flags,
		     IN UCHAR AttributeAvailableMask,
		     IN PVOID Context OPTIONAL,
		     IN PRTL_CONFLICT_RANGE_CALLBACK Callback OPTIONAL,
		     OUT PBOOLEAN Available);

ULONG
STDCALL
RtlIsTextUnicode (
	PVOID	Buffer,
	ULONG	Length,
	ULONG	*Flags
	);

BOOLEAN
STDCALL
RtlIsValidOemCharacter (
	IN PWCHAR Char
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

ULONG STDCALL
RtlLengthRequiredSid (UCHAR SubAuthorityCount);

ULONG STDCALL
RtlLengthSecurityDescriptor (PSECURITY_DESCRIPTOR SecurityDescriptor);

ULONG STDCALL
RtlLengthSid (PSID Sid);

NTSTATUS
STDCALL
RtlLockBootStatusData(
	HANDLE Filehandle
	);

BOOLEAN STDCALL
RtlLockHeap (IN HANDLE Heap);

NTSTATUS STDCALL
RtlLookupAtomInAtomTable (IN PRTL_ATOM_TABLE AtomTable,
			  IN PWSTR AtomName,
			  OUT PRTL_ATOM Atom);

PVOID
STDCALL
RtlLookupElementGenericTable (
	PRTL_GENERIC_TABLE Table,
	PVOID Buffer
	);

PVOID
STDCALL
RtlLookupElementGenericTableAvl (
	PRTL_AVL_TABLE Table,
	PVOID Buffer
	);

PVOID
STDCALL
RtlLookupElementGenericTableFull (
	PRTL_GENERIC_TABLE Table,
	PVOID Buffer,
	OUT PVOID *NodeOrParent,
	OUT TABLE_SEARCH_RESULT *SearchResult
	);

PVOID
STDCALL
RtlLookupElementGenericTableFullAvl (
	PRTL_AVL_TABLE Table,
	PVOID Buffer,
	OUT PVOID *NodeOrParent,
	OUT TABLE_SEARCH_RESULT *SearchResult
	);

NTSTATUS STDCALL
RtlMakeSelfRelativeSD (PSECURITY_DESCRIPTOR AbsSD,
		       PSECURITY_DESCRIPTOR RelSD,
		       PULONG BufferLength);

VOID STDCALL
RtlMapGenericMask (PACCESS_MASK AccessMask,
		   PGENERIC_MAPPING GenericMapping);

NTSTATUS
STDCALL
RtlMapSecurityErrorToNtStatus(
	IN ULONG SecurityError
	);

NTSTATUS STDCALL
RtlMergeRangeLists (OUT PRTL_RANGE_LIST MergedRangeList,
		    IN PRTL_RANGE_LIST RangeList1,
		    IN PRTL_RANGE_LIST RangeList2,
		    IN ULONG Flags);

NTSTATUS
STDCALL
RtlMultiByteToUnicodeN (
	PWCHAR UnicodeString,
	ULONG  UnicodeSize,
	PULONG ResultSize,
	const PCHAR  MbString,
	ULONG  MbSize
	);

NTSTATUS
STDCALL
RtlMultiByteToUnicodeSize (
	PULONG UnicodeSize,
	PCHAR  MbString,
	ULONG  MbSize
	);

PUNICODE_PREFIX_TABLE_ENTRY
STDCALL
RtlNextUnicodePrefix (
	PUNICODE_PREFIX_TABLE PrefixTable,
	BOOLEAN Restart
	);

DWORD
STDCALL
RtlNtStatusToDosError (
	NTSTATUS	StatusCode
	);

DWORD
STDCALL
RtlNtStatusToDosErrorNoTeb (
	NTSTATUS	StatusCode
	);

int
STDCALL
RtlNtStatusToPsxErrno (
	NTSTATUS	StatusCode
	);

ULONG
STDCALL
RtlNumberGenericTableElements(
	PRTL_GENERIC_TABLE Table
	);

ULONG
STDCALL
RtlNumberGenericTableElementsAvl (
	PRTL_AVL_TABLE Table
	);


ULONG
STDCALL
RtlNumberOfClearBits (
	PRTL_BITMAP	BitMapHeader
	);

ULONG
STDCALL
RtlNumberOfSetBits (
	PRTL_BITMAP	BitMapHeader
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
RtlOemToUnicodeN(
	PWSTR UnicodeString,
	ULONG MaxBytesInUnicodeString,
	PULONG BytesInUnicodeString,
	IN PCHAR OemString,
	ULONG BytesInOemString
	);

NTSTATUS STDCALL
RtlPinAtomInAtomTable (
	IN	PRTL_ATOM_TABLE	AtomTable,
	IN	RTL_ATOM	Atom
	);

VOID
FASTCALL
RtlPrefetchMemoryNonTemporal(
	IN PVOID Source,
	IN SIZE_T Length
	);

BOOLEAN
STDCALL
RtlPrefixString (
	PANSI_STRING	String1,
	PANSI_STRING	String2,
	BOOLEAN		CaseInsensitive
	);

BOOLEAN
STDCALL
RtlPrefixUnicodeString (
	PUNICODE_STRING	String1,
	PUNICODE_STRING	String2,
	BOOLEAN		CaseInsensitive
	);

NTSTATUS
STDCALL
RtlQueryAtomInAtomTable (
	IN	PRTL_ATOM_TABLE	AtomTable,
	IN	RTL_ATOM	Atom,
	IN OUT	PULONG		RefCount OPTIONAL,
	IN OUT	PULONG		PinCount OPTIONAL,
	IN OUT	PWSTR		AtomName OPTIONAL,
	IN OUT	PULONG		NameLength OPTIONAL
	);

NTSTATUS STDCALL
RtlQueryInformationAcl (PACL Acl,
			PVOID Information,
			ULONG InformationLength,
			ACL_INFORMATION_CLASS InformationClass);

NTSTATUS STDCALL
RtlQueryTimeZoneInformation (IN OUT PTIME_ZONE_INFORMATION TimeZoneInformation);

VOID STDCALL
RtlRaiseException (IN PEXCEPTION_RECORD ExceptionRecord);

VOID STDCALL
RtlRaiseStatus(NTSTATUS Status);

ULONG STDCALL
RtlRandom (PULONG Seed);

ULONG
STDCALL
RtlRandomEx(
	PULONG Seed
	); 

PRTL_SPLAY_LINKS
STDCALL
RtlRealPredecessor (
	PRTL_SPLAY_LINKS Links
	);

PRTL_SPLAY_LINKS
STDCALL
RtlRealSuccessor (
	PRTL_SPLAY_LINKS Links
	);

PVOID STDCALL
RtlReAllocateHeap (
	HANDLE Heap,
	ULONG Flags,
	PVOID Ptr,
	ULONG Size
	);

VOID
STDCALL
RtlRemoveUnicodePrefix (
	PUNICODE_PREFIX_TABLE PrefixTable,
	PUNICODE_PREFIX_TABLE_ENTRY PrefixTableEntry
	);

NTSTATUS
STDCALL
RtlReserveChunk (
	IN	USHORT	CompressionFormat,
	IN OUT	PUCHAR	*CompressedBuffer,
	IN	PUCHAR	EndOfCompressedBufferPlus1,
	OUT	PUCHAR	*ChunkBuffer,
	IN	ULONG	ChunkSize
	);

VOID STDCALL
RtlResetRtlTranslations (IN PNLSTABLEINFO NlsTable);

/*
 * VOID
 * RtlRetrieveUlong (
 *	PULONG	DestinationAddress,
 *	PULONG	SourceAddress
 *	);
 */
#define RtlRetrieveUlong(DestAddress,SrcAddress) \
	if ((ULONG)(SrcAddress) & LONG_MASK) \
	{ \
		((PUCHAR)(DestAddress))[0]=((PUCHAR)(SrcAddress))[0]; \
		((PUCHAR)(DestAddress))[1]=((PUCHAR)(SrcAddress))[1]; \
		((PUCHAR)(DestAddress))[2]=((PUCHAR)(SrcAddress))[2]; \
		((PUCHAR)(DestAddress))[3]=((PUCHAR)(SrcAddress))[3]; \
	} \
	else \
	{ \
		*((PULONG)(DestAddress))=*((PULONG)(SrcAddress)); \
	}

/*
 * VOID
 * RtlRetrieveUshort (
 *	PUSHORT	DestinationAddress,
 *	PUSHORT	SourceAddress
 *	);
 */
#define RtlRetrieveUshort(DestAddress,SrcAddress) \
	if ((ULONG)(SrcAddress) & SHORT_MASK) \
	{ \
		((PUCHAR)(DestAddress))[0]=((PUCHAR)(SrcAddress))[0]; \
		((PUCHAR)(DestAddress))[1]=((PUCHAR)(SrcAddress))[1]; \
	} \
	else \
	{ \
		*((PUSHORT)(DestAddress))=*((PUSHORT)(SrcAddress)); \
	}

VOID STDCALL
RtlSecondsSince1970ToTime (ULONG SecondsSince1970,
			   PLARGE_INTEGER Time);

VOID STDCALL
RtlSecondsSince1980ToTime (ULONG SecondsSince1980,
			   PLARGE_INTEGER Time);

NTSTATUS STDCALL
RtlSelfRelativeToAbsoluteSD (PSECURITY_DESCRIPTOR RelSD,
			     PSECURITY_DESCRIPTOR AbsSD,
			     PULONG AbsSDSize,
			     PACL Dacl,
			     PULONG DaclSize,
			     PACL Sacl,
			     PULONG SaclSize,
			     PSID Owner,
			     PULONG OwnerSize,
			     PSID Group,
			     PULONG GroupSize);

NTSTATUS
STDCALL
RtlSelfRelativeToAbsoluteSD2(
	PSECURITY_DESCRIPTOR SelfRelativeSecurityDescriptor,
	PULONG BufferSize
	);

VOID STDCALL
RtlSetAllBits (IN PRTL_BITMAP BitMapHeader);

VOID
STDCALL
RtlSetBit (
	PRTL_BITMAP BitMapHeader,
	ULONG BitNumber
	);

VOID
STDCALL
RtlSetBits (
	PRTL_BITMAP	BitMapHeader,
	ULONG		StartingIndex,
	ULONG		NumberToSet
	);

NTSTATUS STDCALL
RtlSetDaclSecurityDescriptor (PSECURITY_DESCRIPTOR SecurityDescriptor,
			      BOOLEAN DaclPresent,
			      PACL Dacl,
			      BOOLEAN DaclDefaulted);

NTSTATUS STDCALL
RtlSetGroupSecurityDescriptor (PSECURITY_DESCRIPTOR SecurityDescriptor,
			       PSID Group,
			       BOOLEAN GroupDefaulted);

NTSTATUS STDCALL
RtlSetOwnerSecurityDescriptor (PSECURITY_DESCRIPTOR SecurityDescriptor,
			       PSID Owner,
			       BOOLEAN OwnerDefaulted);

NTSTATUS STDCALL
RtlSetSaclSecurityDescriptor (PSECURITY_DESCRIPTOR SecurityDescriptor,
			      BOOLEAN SaclPresent,
			      PACL Sacl,
			      BOOLEAN SaclDefaulted);

NTSTATUS STDCALL
RtlSetInformationAcl (PACL Acl,
		      PVOID Information,
		      ULONG InformationLength,
		      ACL_INFORMATION_CLASS InformationClass);

NTSTATUS STDCALL
RtlSetTimeZoneInformation (IN OUT PTIME_ZONE_INFORMATION TimeZoneInformation);

ULONG STDCALL
RtlSizeHeap(
	IN PVOID HeapHandle, 
	IN ULONG Flags, 
	IN PVOID MemoryPointer
	); 

PRTL_SPLAY_LINKS
STDCALL
RtlSplay (
	PRTL_SPLAY_LINKS Links
	);

/*
 * VOID
 * RtlStoreUlong (
 *	PULONG	Address,
 *	ULONG	Value
 *	);
 */
#define RtlStoreUlong(Address,Value) \
	if ((ULONG)(Address) & LONG_MASK) \
	{ \
		((PUCHAR)(Address))[LONG_LEAST_SIGNIFICANT_BIT]=(UCHAR)(FIRSTBYTE(Value)); \
		((PUCHAR)(Address))[LONG_3RD_MOST_SIGNIFICANT_BIT]=(UCHAR)(FIRSTBYTE(Value)); \
		((PUCHAR)(Address))[LONG_2ND_MOST_SIGNIFICANT_BIT]=(UCHAR)(THIRDBYTE(Value)); \
		((PUCHAR)(Address))[LONG_MOST_SIGNIFICANT_BIT]=(UCHAR)(FOURTHBYTE(Value)); \
	} \
	else \
	{ \
		*((PULONG)(Address))=(ULONG)(Value); \
	}

/*
 * VOID
 * RtlStoreUshort (
 *	PUSHORT	Address,
 *	USHORT	Value
 *	);
 */
#define RtlStoreUshort(Address,Value) \
	if ((ULONG)(Address) & SHORT_MASK) \
	{ \
		((PUCHAR)(Address))[SHORT_LEAST_SIGNIFICANT_BIT]=(UCHAR)(FIRSTBYTE(Value)); \
		((PUCHAR)(Address))[SHORT_MOST_SIGNIFICANT_BIT]=(UCHAR)(SECONDBYTE(Value)); \
	} \
	else \
	{ \
		*((PUSHORT)(Address))=(USHORT)(Value); \
	}

NTSTATUS STDCALL
RtlStringFromGUID (IN REFGUID Guid,
		   OUT PUNICODE_STRING GuidString);

PULONG STDCALL
RtlSubAuthoritySid (PSID Sid,
		    ULONG SubAuthority);

PULONG STDCALL
RtlSubAuthoritySid (PSID Sid,
		    ULONG SubAuthority);

PUCHAR STDCALL
RtlSubAuthorityCountSid (PSID Sid);

PRTL_SPLAY_LINKS
STDCALL
RtlSubtreePredecessor (
	PRTL_SPLAY_LINKS Links
	);

PRTL_SPLAY_LINKS
STDCALL
RtlSubtreeSuccessor (
	PRTL_SPLAY_LINKS Links
	);

BOOLEAN
STDCALL
RtlTestBit (
	PRTL_BITMAP BitMapHeader,
	ULONG BitNumber
	);


BOOLEAN STDCALL
RtlTimeFieldsToTime (PTIME_FIELDS TimeFields,
		     PLARGE_INTEGER Time);

BOOLEAN
STDCALL
RtlTimeToSecondsSince1970 (
	PLARGE_INTEGER Time,
	PULONG SecondsSince1970
	);

BOOLEAN
STDCALL
RtlTimeToSecondsSince1980 (
	PLARGE_INTEGER Time,
	PULONG SecondsSince1980
	);

VOID
STDCALL
RtlTimeToElapsedTimeFields( 
	PLARGE_INTEGER Time, 
	PTIME_FIELDS TimeFields 
	);

VOID
STDCALL
RtlTimeToTimeFields (
	PLARGE_INTEGER	Time,
	PTIME_FIELDS	TimeFields
	);


ULONG FASTCALL
RtlUlongByteSwap (IN ULONG Source);

ULONGLONG FASTCALL
RtlUlonglongByteSwap (IN ULONGLONG Source);

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
RtlUnicodeStringToCountedOemString (
	IN OUT	POEM_STRING	DestinationString,
	IN	PUNICODE_STRING	SourceString,
	IN	BOOLEAN		AllocateDestinationString
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
RtlUnicodeToCustomCPN (
	IN	PCPTABLEINFO	CustomCP,
	PCHAR		MbString,
	ULONG		MbSize,
	PULONG		ResultSize,
	PWCHAR		UnicodeString,
	ULONG		UnicodeSize
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

ULONG STDCALL
RtlUniform (PULONG Seed);

BOOLEAN STDCALL
RtlUnlockHeap (IN HANDLE Heap);

NTSTATUS
STDCALL
RtlUnlockBootStatusData(
	HANDLE Filehandle
	);

VOID
STDCALL
RtlUnwind (
  PEXCEPTION_REGISTRATION RegistrationFrame,
  PVOID ReturnAddress,
  PEXCEPTION_RECORD ExceptionRecord,
  DWORD EaxValue
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
	IN	PCUNICODE_STRING	SourceString,
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
RtlUpcaseUnicodeStringToCountedOemString (
	IN OUT	POEM_STRING	DestinationString,
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
RtlUpcaseUnicodeToCustomCPN (
	IN	PCPTABLEINFO	CustomCP,
	PCHAR		MbString,
	ULONG		MbSize,
	PULONG		ResultSize,
	PWCHAR		UnicodeString,
	ULONG		UnicodeSize
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

CHAR STDCALL
RtlUpperChar (CHAR Source);

VOID STDCALL
RtlUpperString (PSTRING DestinationString,
		PSTRING SourceString);

USHORT FASTCALL
RtlUshortByteSwap (IN USHORT Source);

BOOLEAN STDCALL
RtlValidAcl (PACL Acl);

BOOLEAN STDCALL
RtlValidateHeap (
	HANDLE Heap,
	ULONG	Flags,
	PVOID	pmem
	);

BOOLEAN
STDCALL
RtlValidRelativeSecurityDescriptor (
	IN PSECURITY_DESCRIPTOR SecurityDescriptorInput,
	IN ULONG SecurityDescriptorLength,
	IN SECURITY_INFORMATION RequiredInformation
	);

BOOLEAN STDCALL
RtlValidSecurityDescriptor (IN PSECURITY_DESCRIPTOR SecurityDescriptor);

BOOLEAN STDCALL
RtlValidSid (IN PSID Sid);

/*
NTSTATUS
STDCALL
RtlVerifyVersionInfo(
	IN PRTL_OSVERSIONINFOEXW VersionInfo,
	IN ULONG TypeMask,
	IN ULONGLONG  ConditionMask
	);
*/

NTSTATUS
STDCALL
RtlVolumeDeviceToDosName(
	IN  PVOID VolumeDeviceObject,
	OUT PUNICODE_STRING DosName
	);

ULONG
STDCALL
RtlWalkFrameChain (
	OUT PVOID *Callers,
	IN ULONG Count,
	IN ULONG Flags
	);

BOOLEAN STDCALL
RtlZeroHeap(
    IN PVOID HeapHandle,
    IN ULONG Flags
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

NTSTATUS
FASTCALL
RtlpOemStringToCountedUnicodeString(
   IN OUT PUNICODE_STRING UniDest,
   IN POEM_STRING OemSource,
   IN BOOLEAN AllocateDestinationString,
   IN POOL_TYPE PoolType);
   
NTSTATUS 
FASTCALL
RtlpUpcaseUnicodeString(
   IN OUT PUNICODE_STRING UniDest,
   IN PCUNICODE_STRING UniSource,
   IN BOOLEAN  AllocateDestinationString,
   IN POOL_TYPE PoolType);
   
NTSTATUS 
FASTCALL
RtlpUpcaseUnicodeStringToAnsiString(
   IN OUT PANSI_STRING AnsiDest,
   IN PUNICODE_STRING UniSource,
   IN BOOLEAN  AllocateDestinationString,
   IN POOL_TYPE PoolType);
   
NTSTATUS 
FASTCALL
RtlpUpcaseUnicodeStringToCountedOemString(
   IN OUT POEM_STRING OemDest,
   IN PUNICODE_STRING UniSource,
   IN BOOLEAN AllocateDestinationString,
   IN POOL_TYPE PoolType);
   
NTSTATUS 
FASTCALL
RtlpUpcaseUnicodeStringToOemString (
   IN OUT POEM_STRING OemDest,
   IN PUNICODE_STRING UniSource,
   IN BOOLEAN  AllocateDestinationString,
   IN POOL_TYPE PoolType);
   
NTSTATUS 
FASTCALL
RtlpDowncaseUnicodeString(
   IN OUT PUNICODE_STRING UniDest,
   IN PUNICODE_STRING UniSource,
   IN BOOLEAN AllocateDestinationString,
   IN POOL_TYPE PoolType);
   
NTSTATUS
FASTCALL
RtlpAnsiStringToUnicodeString(
   IN OUT PUNICODE_STRING DestinationString,
   IN PANSI_STRING SourceString,
   IN BOOLEAN AllocateDestinationString,
   IN POOL_TYPE PoolType);   
   
NTSTATUS
FASTCALL
RtlpUnicodeStringToAnsiString(
   IN OUT PANSI_STRING AnsiDest,
   IN PUNICODE_STRING UniSource,
   IN BOOLEAN AllocateDestinationString,
   IN POOL_TYPE PoolType);
   
NTSTATUS
FASTCALL
RtlpOemStringToUnicodeString(
   IN OUT PUNICODE_STRING UniDest,
   IN POEM_STRING OemSource,
   IN BOOLEAN AllocateDestinationString,
   IN POOL_TYPE PoolType);

NTSTATUS
FASTCALL
RtlpUnicodeStringToOemString(
   IN OUT POEM_STRING OemDest,
   IN PUNICODE_STRING UniSource,
   IN BOOLEAN  AllocateDestinationString,
   IN POOL_TYPE PoolType);
   
BOOLEAN
FASTCALL
RtlpCreateUnicodeString(
   IN OUT PUNICODE_STRING UniDest,
   IN PCWSTR  Source,
   IN POOL_TYPE PoolType);   

NTSTATUS
FASTCALL
RtlpUnicodeStringToCountedOemString(
   IN OUT POEM_STRING OemDest,
   IN PUNICODE_STRING UniSource,
   IN BOOLEAN AllocateDestinationString,
   IN POOL_TYPE PoolType);

NTSTATUS STDCALL
RtlpDuplicateUnicodeString(
   INT AddNull,
   IN PUNICODE_STRING SourceString,
   PUNICODE_STRING DestinationString,
   POOL_TYPE PoolType);
   
/* Register io functions */

UCHAR
STDCALL
READ_REGISTER_UCHAR (
	PUCHAR	Register
	);

USHORT
STDCALL
READ_REGISTER_USHORT (
	PUSHORT	Register
	);

ULONG
STDCALL
READ_REGISTER_ULONG (
	PULONG	Register
	);

VOID
STDCALL
READ_REGISTER_BUFFER_UCHAR (
	PUCHAR	Register,
	PUCHAR	Buffer,
	ULONG	Count
	);

VOID
STDCALL
READ_REGISTER_BUFFER_USHORT (
	PUSHORT	Register,
	PUSHORT	Buffer,
	ULONG	Count
	);

VOID
STDCALL
READ_REGISTER_BUFFER_ULONG (
	PULONG	Register,
	PULONG	Buffer,
	ULONG	Count
	);

VOID
STDCALL
WRITE_REGISTER_UCHAR (
	PUCHAR	Register,
	UCHAR	Value
	);

VOID
STDCALL
WRITE_REGISTER_USHORT (
	PUSHORT	Register,
	USHORT	Value
	);

VOID
STDCALL
WRITE_REGISTER_ULONG (
	PULONG	Register,
	ULONG	Value
	);

VOID
STDCALL
WRITE_REGISTER_BUFFER_UCHAR (
	PUCHAR	Register,
	PUCHAR	Buffer,
	ULONG	Count
	);

VOID
STDCALL
WRITE_REGISTER_BUFFER_USHORT (
	PUSHORT	Register,
	PUSHORT	Buffer,
	ULONG	Count
	);

VOID
STDCALL
WRITE_REGISTER_BUFFER_ULONG (
	PULONG	Register,
	PULONG	Buffer,
	ULONG	Count
	);


/*  functions exported from NTOSKRNL.EXE which are considered RTL  */

#if defined(__NTOSKRNL__) || defined(__NTDRIVER__) || defined(__NTHAL__) || defined(__NTDLL__) || defined(__NTAPP__)

char *_itoa (int value, char *string, int radix);
wchar_t *_itow (int value, wchar_t *string, int radix);
int _snprintf(char * buf, size_t cnt, const char *fmt, ...);
int _snwprintf(wchar_t *buf, size_t cnt, const wchar_t *fmt, ...);
int _stricmp(const char *s1, const char *s2);
char * _strlwr(char *x);
int _strnicmp(const char *s1, const char *s2, size_t n);
char * _strnset(char* szToFill, int szFill, size_t sizeMaxFill);
char * _strrev(char *s);
char * _strset(char* szToFill, int szFill);
char * _strupr(char *x);
int _vsnprintf(char *buf, size_t cnt, const char *fmt, va_list args);
int _wcsicmp (const wchar_t* cs, const wchar_t* ct);
wchar_t * _wcslwr (wchar_t *x);
int _wcsnicmp (const wchar_t * cs,const wchar_t * ct,size_t count);
wchar_t* _wcsnset (wchar_t* wsToFill, wchar_t wcFill, size_t sizeMaxFill);
wchar_t * _wcsrev(wchar_t *s);
wchar_t *_wcsupr(wchar_t *x);

int atoi(const char *str);
long atol(const char *str);
int isdigit(int c);
int islower(int c);
int isprint(int c);
int isspace(int c);
int isupper(int c);
int isxdigit(int c);
size_t mbstowcs (wchar_t *wcstr, const char *mbstr, size_t count);
int mbtowc (wchar_t *wchar, const char *mbchar, size_t count);
void * memchr(const void *s, int c, size_t n);
void * memcpy(void *to, const void *from, size_t count);
void * memmove(void *dest,const void *src, size_t count);
void * memset(void *src, int val, size_t count);

#if 0
qsort
#endif

int rand(void);
int sprintf(char * buf, const char *fmt, ...);
void srand(unsigned seed);
char * strcat(char *s, const char *append);
char * strchr(const char *s, int c);
int strcmp(const char *s1, const char *s2);
char * strcpy(char *to, const char *from);
size_t strlen(const char *str);
char * strncat(char *dst, const char *src, size_t n);
int strncmp(const char *s1, const char *s2, size_t n);
char *strncpy(char *dst, const char *src, size_t n);
char *strrchr(const char *s, int c);
size_t strspn(const char *s1, const char *s2);
char *strstr(const char *s, const char *find);
int swprintf(wchar_t *buf, const wchar_t *fmt, ...);
int tolower(int c);
int toupper(int c);
wchar_t towlower(wchar_t c);
wchar_t towupper(wchar_t c);
int vsprintf(char *buf, const char *fmt, va_list args);
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
size_t wcstombs (char *mbstr, const wchar_t *wcstr, size_t count);
int wctomb (char *mbchar, wchar_t wchar);

#endif /* __NTOSKRNL__ || __NTDRIVER__ || __NTHAL__ || __NTDLL__ || __NTAPP__ */

#endif /* __DDK_RTL_H */


