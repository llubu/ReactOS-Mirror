#ifndef __INCLUDE_CM_H
#define __INCLUDE_CM_H

#ifdef DBG
#define CHECKED 1
#else
#define CHECKED 0
#endif

#define  REG_ROOT_KEY_NAME		L"\\Registry"
#define  REG_MACHINE_KEY_NAME		L"\\Registry\\Machine"
#define  REG_HARDWARE_KEY_NAME		L"\\Registry\\Machine\\HARDWARE"
#define  REG_DESCRIPTION_KEY_NAME	L"\\Registry\\Machine\\HARDWARE\\DESCRIPTION"
#define  REG_DEVICEMAP_KEY_NAME		L"\\Registry\\Machine\\HARDWARE\\DEVICEMAP"
#define  REG_RESOURCEMAP_KEY_NAME	L"\\Registry\\Machine\\HARDWARE\\RESOURCEMAP"
#define  REG_CLASSES_KEY_NAME		L"\\Registry\\Machine\\Software\\Classes"
#define  REG_SYSTEM_KEY_NAME		L"\\Registry\\Machine\\System"
#define  REG_SOFTWARE_KEY_NAME		L"\\Registry\\Machine\\Software"
#define  REG_SAM_KEY_NAME		L"\\Registry\\Machine\\Sam"
#define  REG_SEC_KEY_NAME		L"\\Registry\\Machine\\Security"
#define  REG_USER_KEY_NAME		L"\\Registry\\User"
#define  REG_DEFAULT_USER_KEY_NAME	L"\\Registry\\User\\.Default"
#define  REG_CURRENT_USER_KEY_NAME	L"\\Registry\\User\\CurrentUser"

#define  SYSTEM_REG_FILE		L"\\SystemRoot\\System32\\Config\\SYSTEM"
#define  SYSTEM_LOG_FILE		L"\\SystemRoot\\System32\\Config\\SYSTEM.log"
#define  SOFTWARE_REG_FILE		L"\\SystemRoot\\System32\\Config\\SOFTWARE"
#define  DEFAULT_USER_REG_FILE		L"\\SystemRoot\\System32\\Config\\DEFAULT"
#define  SAM_REG_FILE			L"\\SystemRoot\\System32\\Config\\SAM"
#define  SEC_REG_FILE			L"\\SystemRoot\\System32\\Config\\SECURITY"

#define  REG_SYSTEM_FILE_NAME		L"\\SYSTEM"
#define  REG_SOFTWARE_FILE_NAME		L"\\SOFTWARE"
#define  REG_DEFAULT_USER_FILE_NAME	L"\\DEFAULT"
#define  REG_SAM_FILE_NAME		L"\\SAM"
#define  REG_SEC_FILE_NAME		L"\\SECURITY"

#define  REG_BLOCK_SIZE                4096
#define  REG_HBIN_DATA_OFFSET          32
#define  REG_INIT_BLOCK_LIST_SIZE      32
#define  REG_INIT_HASH_TABLE_SIZE      3
#define  REG_EXTEND_HASH_TABLE_SIZE    4
#define  REG_VALUE_LIST_CELL_MULTIPLE  4

#define  REG_HIVE_ID                   0x66676572
#define  REG_BIN_ID                    0x6e696268
#define  REG_KEY_CELL_ID               0x6b6e
#define  REG_HASH_TABLE_CELL_ID        0x666c
#define  REG_VALUE_CELL_ID             0x6b76


// BLOCK_OFFSET = offset in file after header block
typedef ULONG BLOCK_OFFSET, *PBLOCK_OFFSET;

/* header for registry hive file : */
typedef struct _HIVE_HEADER
{
  /* Hive identifier "regf" (0x66676572) */
  ULONG  BlockId;

  /* Update counter */
  ULONG  UpdateCounter1;

  /* Update counter */
  ULONG  UpdateCounter2;

  /* When this hive file was last modified */
  LARGE_INTEGER  DateModified;

  /* Registry format version ? (1?) */
  ULONG  Unused3;

  /* Registry format version ? (3?) */
  ULONG  Unused4;

  /* Registry format version ? (0?) */
  ULONG  Unused5;

  /* Registry format version ? (1?) */
  ULONG  Unused6;

  /* Offset into file from the byte after the end of the base block.
     If the hive is volatile, this is the actual pointer to the KEY_CELL */
  BLOCK_OFFSET  RootKeyOffset;

  /* Size of each hive block ? */
  ULONG  BlockSize;

  /* (1?) */
  ULONG  Unused7;

  /* Name of hive file */
  WCHAR  FileName[64];

  /* ? */
  ULONG  Unused8[83];

  /* Checksum of first 0x200 bytes */
  ULONG  Checksum;
} __attribute__((packed)) HIVE_HEADER, *PHIVE_HEADER;

typedef struct _HBIN
{
  /* Bin identifier "hbin" (0x6E696268) */
  ULONG  BlockId;

  /* Block offset of this bin */
  BLOCK_OFFSET  BlockOffset;

  /* Size in bytes, multiple of the block size (4KB) */
  ULONG  BlockSize;

  /* ? */
  ULONG  Unused1;

  /* When this bin was last modified */
  LARGE_INTEGER  DateModified;

  /* ? */
  ULONG  Unused2;
} __attribute__((packed)) HBIN, *PHBIN;

typedef struct _CELL_HEADER
{
  /* <0 if used, >0 if free */
  LONG  CellSize;
} __attribute__((packed)) CELL_HEADER, *PCELL_HEADER;

typedef struct _KEY_CELL
{
  /* Size of this cell */
  LONG  CellSize;

  /* Key cell identifier "kn" (0x6b6e) */
  USHORT  Id;

  /* Flags */
  USHORT  Flags;

  /* Time of last flush */
  LARGE_INTEGER  LastWriteTime;

  /* ? */
  ULONG  UnUsed1;

  /* Block offset of parent key cell */
  BLOCK_OFFSET  ParentKeyOffset;

  /* Count of sub keys for the key in this key cell */
  ULONG  NumberOfSubKeys;

  /* ? */
  ULONG  UnUsed2;

  /* Block offset of has table for FIXME: subkeys/values? */
  BLOCK_OFFSET  HashTableOffset;

  /* ? */
  ULONG  UnUsed3;

  /* Count of values contained in this key cell */
  ULONG  NumberOfValues;

  /* Block offset of VALUE_LIST_CELL */
  BLOCK_OFFSET  ValueListOffset;

  /* Block offset of security cell */
  BLOCK_OFFSET  SecurityKeyOffset;

  /* Block offset of registry key class */
  BLOCK_OFFSET  ClassNameOffset;

  /* ? */
  ULONG  Unused4[5];

  /* Size in bytes of key name */
  USHORT NameSize;

  /* Size of class name in bytes */
  USHORT ClassSize;

  /* Name of key (not zero terminated) */
  UCHAR  Name[0];
} __attribute__((packed)) KEY_CELL, *PKEY_CELL;

/* KEY_CELL.Flags constants */
#define  REG_KEY_ROOT_CELL                 0x0C
#define  REG_KEY_LINK_CELL                 0x10
#define  REG_KEY_NAME_PACKED               0x20

/*
 * Hash record
 *
 * HashValue :
 *	packed name: four letters of value's name
 *	otherwise: Zero!
 */
typedef struct _HASH_RECORD
{
  BLOCK_OFFSET  KeyOffset;
  ULONG  HashValue;
} __attribute__((packed)) HASH_RECORD, *PHASH_RECORD;

typedef struct _HASH_TABLE_CELL
{
  LONG  CellSize;
  USHORT  Id;
  USHORT  HashTableSize;
  HASH_RECORD  Table[0];
} __attribute__((packed)) HASH_TABLE_CELL, *PHASH_TABLE_CELL;


typedef struct _VALUE_LIST_CELL
{
  LONG  CellSize;
  BLOCK_OFFSET  ValueOffset[0];
} __attribute__((packed)) VALUE_LIST_CELL, *PVALUE_LIST_CELL;

typedef struct _VALUE_CELL
{
  LONG  CellSize;
  USHORT Id;	// "kv"
  USHORT NameSize;	// length of Name
  ULONG  DataSize;	// length of datas in the cell pointed by DataOffset
  BLOCK_OFFSET  DataOffset;// datas are here if high bit of DataSize is set
  ULONG  DataType;
  USHORT Flags;
  USHORT Unused1;
  UCHAR  Name[0]; /* warning : not zero terminated */
} __attribute__((packed)) VALUE_CELL, *PVALUE_CELL;

/* VALUE_CELL.Flags constants */
#define REG_VALUE_NAME_PACKED             0x0001

/* VALUE_CELL.DataSize mask constants */
#define REG_DATA_SIZE_MASK                 0x7FFFFFFF
#define REG_DATA_IN_OFFSET                 0x80000000


typedef struct _DATA_CELL
{
  LONG  CellSize;
  UCHAR  Data[0];
} __attribute__((packed)) DATA_CELL, *PDATA_CELL;

typedef struct _REGISTRY_HIVE
{
  LIST_ENTRY  HiveList;
  ULONG  Flags;
  UNICODE_STRING  HiveFileName;
  UNICODE_STRING  LogFileName;
  ULONG  FileSize;
  PHIVE_HEADER  HiveHeader;
  ULONG  UpdateCounter;
  ULONG  BlockListSize;
  PHBIN  *BlockList;
  ULONG  FreeListSize;
  ULONG  FreeListMax;
  PCELL_HEADER *FreeList;
  BLOCK_OFFSET *FreeListOffset;
  ERESOURCE  HiveResource;

  PULONG BitmapBuffer;
  RTL_BITMAP  DirtyBitMap;
  BOOLEAN  HiveDirty;
} REGISTRY_HIVE, *PREGISTRY_HIVE;

/* REGISTRY_HIVE.Flags constants */
/* When set, the hive uses pointers instead of offsets. */
#define HIVE_POINTER    0x00000001

/* When set, the hive is not backed by a file.
   Therefore, it can not be flushed to disk. */
#define HIVE_NO_FILE    0x00000002

/* When set, a modified (dirty) hive is not synchronized automatically.
   Explicit synchronization (save/flush) works. */
#define HIVE_NO_SYNCH   0x00000004

#define IsPointerHive(Hive)  ((Hive)->Flags & HIVE_POINTER)
#define IsNoFileHive(Hive)  ((Hive)->Flags & HIVE_NO_FILE)
#define IsNoSynchHive(Hive)  ((Hive)->Flags & HIVE_NO_SYNCH)


#define IsFreeCell(Cell)(Cell->CellSize >= 0)
#define IsUsedCell(Cell)(Cell->CellSize < 0)


/* KEY_OBJECT.Flags */

/* When set, the key is scheduled for deletion, and all
   attempts to access the key must not succeed */
#define KO_MARKED_FOR_DELETE              0x00000001


/* Type defining the Object Manager Key Object */
typedef struct _KEY_OBJECT
{
  /* Fields used by the Object Manager */
  CSHORT Type;
  CSHORT Size;

  /* Key flags */
  ULONG Flags;

  /* Key name */
  UNICODE_STRING Name;

  /* Registry hive the key belongs to */
  PREGISTRY_HIVE RegistryHive;

  /* Block offset of the key cell this key belongs in */
  BLOCK_OFFSET KeyCellOffset;

  /* KEY_CELL this key belong in */
  PKEY_CELL KeyCell;

  /* Link to the parent KEY_OBJECT for this key */
  struct _KEY_OBJECT *ParentKey;

  /* Subkeys loaded in SubKeys */
  ULONG NumberOfSubKeys;

  /* Space allocated in SubKeys */
  ULONG SizeOfSubKeys;

  /* List of subkeys loaded */
  struct _KEY_OBJECT **SubKeys;
} KEY_OBJECT, *PKEY_OBJECT;

/* Bits 31-22 (top 10 bits) of the cell index is the directory index */
#define CmiDirectoryIndex(CellIndex)(CellIndex & 0xffc000000)
/* Bits 21-12 (middle 10 bits) of the cell index is the table index */
#define CmiTableIndex(Cellndex)(CellIndex & 0x003ff000)
/* Bits 11-0 (bottom 12 bits) of the cell index is the byte offset */
#define CmiByteOffset(Cellndex)(CellIndex & 0x00000fff)


extern BOOLEAN CmiDoVerify;
extern PREGISTRY_HIVE CmiVolatileHive;
extern POBJECT_TYPE CmiKeyType;
extern KSPIN_LOCK CmiKeyListLock;

extern LIST_ENTRY CmiHiveListHead;
extern ERESOURCE CmiHiveListLock;


VOID
CmiVerifyBinCell(PHBIN BinCell);
VOID
CmiVerifyKeyCell(PKEY_CELL KeyCell);
VOID
CmiVerifyRootKeyCell(PKEY_CELL RootKeyCell);
VOID
CmiVerifyKeyObject(PKEY_OBJECT KeyObject);
VOID
CmiVerifyRegistryHive(PREGISTRY_HIVE RegistryHive);

#ifdef DBG
#define VERIFY_BIN_CELL CmiVerifyBinCell
#define VERIFY_KEY_CELL CmiVerifyKeyCell
#define VERIFY_ROOT_KEY_CELL CmiVerifyRootKeyCell
#define VERIFY_VALUE_CELL CmiVerifyValueCell
#define VERIFY_VALUE_LIST_CELL CmiVerifyValueListCell
#define VERIFY_KEY_OBJECT CmiVerifyKeyObject
#define VERIFY_REGISTRY_HIVE CmiVerifyRegistryHive
#else
#define VERIFY_BIN_CELL(x)
#define VERIFY_KEY_CELL(x)
#define VERIFY_ROOT_KEY_CELL(x)
#define VERIFY_VALUE_CELL(x)
#define VERIFY_VALUE_LIST_CELL(x)
#define VERIFY_KEY_OBJECT(x)
#define VERIFY_REGISTRY_HIVE(x)
#endif

NTSTATUS STDCALL
CmiObjectParse(IN PVOID  ParsedObject,
	       OUT PVOID  *NextObject,
	       IN PUNICODE_STRING  FullPath,
	       IN OUT PWSTR  *Path,
	       IN ULONG  Attribute);

NTSTATUS STDCALL
CmiObjectCreate(PVOID ObjectBody,
		PVOID Parent,
		PWSTR RemainingPath,
		POBJECT_ATTRIBUTES ObjectAttributes);

VOID STDCALL
CmiObjectDelete(PVOID  DeletedObject);

NTSTATUS STDCALL
CmiObjectSecurity(PVOID ObjectBody,
		  SECURITY_OPERATION_CODE OperationCode,
		  SECURITY_INFORMATION SecurityInformation,
		  PSECURITY_DESCRIPTOR SecurityDescriptor,
		  PULONG BufferLength);

NTSTATUS STDCALL
CmiObjectQueryName (PVOID ObjectBody,
		    POBJECT_NAME_INFORMATION ObjectNameInfo,
		    ULONG Length,
		    PULONG ReturnLength);

NTSTATUS
CmiImportHiveBins(PREGISTRY_HIVE Hive,
		  PUCHAR ChunkPtr);

VOID
CmiFreeHiveBins(PREGISTRY_HIVE Hive);

NTSTATUS
CmiCreateHiveFreeCellList(PREGISTRY_HIVE Hive);

VOID
CmiFreeHiveFreeCellList(PREGISTRY_HIVE Hive);

NTSTATUS
CmiCreateHiveBitmap(PREGISTRY_HIVE Hive);


VOID
CmiAddKeyToList(IN PKEY_OBJECT ParentKey,
		IN PKEY_OBJECT NewKey);

NTSTATUS
CmiRemoveKeyFromList(IN PKEY_OBJECT NewKey);

PKEY_OBJECT
CmiScanKeyList(IN PKEY_OBJECT Parent,
	       IN PUNICODE_STRING KeyName,
	       IN ULONG Attributes);

NTSTATUS
CmiCreateVolatileHive(PREGISTRY_HIVE *RegistryHive);

NTSTATUS
CmiLoadHive(POBJECT_ATTRIBUTES KeyObjectAttributes,
	    PUNICODE_STRING FileName,
	    ULONG Flags);

NTSTATUS
CmiRemoveRegistryHive(PREGISTRY_HIVE RegistryHive);

NTSTATUS
CmiFlushRegistryHive(PREGISTRY_HIVE RegistryHive);

ULONG
CmiGetMaxNameLength(IN PKEY_OBJECT KeyObject);

ULONG
CmiGetMaxClassLength(IN PKEY_OBJECT KeyObject);

ULONG
CmiGetMaxValueNameLength(IN PREGISTRY_HIVE  RegistryHive,
  IN PKEY_CELL  KeyCell);

ULONG
CmiGetMaxValueDataLength(IN PREGISTRY_HIVE  RegistryHive,
  IN PKEY_CELL  KeyCell);

NTSTATUS
CmiScanForSubKey(IN PREGISTRY_HIVE  RegistryHive,
		 IN PKEY_CELL  KeyCell,
		 OUT PKEY_CELL  *SubKeyCell,
		 OUT BLOCK_OFFSET *BlockOffset,
		 IN PUNICODE_STRING KeyName,
		 IN ACCESS_MASK  DesiredAccess,
		 IN ULONG Attributes);

NTSTATUS
CmiAddSubKey(IN PREGISTRY_HIVE  RegistryHive,
	     IN PKEY_OBJECT Parent,
	     OUT PKEY_OBJECT SubKey,
	     IN PUNICODE_STRING SubKeyName,
	     IN ULONG  TitleIndex,
	     IN PUNICODE_STRING  Class,
	     IN ULONG  CreateOptions);

NTSTATUS
CmiRemoveSubKey(IN PREGISTRY_HIVE RegistryHive,
		IN PKEY_OBJECT Parent,
		IN PKEY_OBJECT SubKey);

NTSTATUS
CmiScanKeyForValue(IN PREGISTRY_HIVE  RegistryHive,
  IN PKEY_CELL  KeyCell,
  IN PUNICODE_STRING  ValueName,
  OUT PVALUE_CELL  *ValueCell,
  OUT BLOCK_OFFSET *VBOffset);

NTSTATUS
CmiGetValueFromKeyByIndex(IN PREGISTRY_HIVE  RegistryHive,
  IN PKEY_CELL  KeyCell,
  IN ULONG  Index,
  OUT PVALUE_CELL  *ValueCell);

NTSTATUS
CmiAddValueToKey(IN PREGISTRY_HIVE  RegistryHive,
  IN PKEY_CELL  KeyCell,
  IN PUNICODE_STRING ValueName,
  OUT PVALUE_CELL *pValueCell,
  OUT BLOCK_OFFSET *pVBOffset);

NTSTATUS
CmiDeleteValueFromKey(IN PREGISTRY_HIVE  RegistryHive,
		      IN PKEY_CELL  KeyCell,
		      IN BLOCK_OFFSET KeyCellOffset,
		      IN PUNICODE_STRING ValueName);

NTSTATUS
CmiAllocateHashTableCell(IN PREGISTRY_HIVE  RegistryHive,
  OUT PHASH_TABLE_CELL  *HashBlock,
  OUT BLOCK_OFFSET  *HBOffset,
  IN ULONG  HashTableSize);

PKEY_CELL
CmiGetKeyFromHashByIndex(PREGISTRY_HIVE RegistryHive,
PHASH_TABLE_CELL  HashBlock,
ULONG  Index);

NTSTATUS
CmiAddKeyToHashTable(PREGISTRY_HIVE  RegistryHive,
  PHASH_TABLE_CELL  HashBlock,
  PKEY_CELL  NewKeyCell,
  BLOCK_OFFSET  NKBOffset);

NTSTATUS
CmiRemoveKeyFromHashTable(PREGISTRY_HIVE RegistryHive,
			  PHASH_TABLE_CELL HashBlock,
			  BLOCK_OFFSET NKBOffset);

NTSTATUS
CmiAllocateValueCell(IN PREGISTRY_HIVE  RegistryHive,
  OUT PVALUE_CELL  *ValueCell,
  OUT BLOCK_OFFSET  *VBOffset,
  IN PUNICODE_STRING ValueName);

NTSTATUS
CmiDestroyValueCell(PREGISTRY_HIVE  RegistryHive,
  PVALUE_CELL  ValueCell,
  BLOCK_OFFSET  VBOffset);

NTSTATUS
CmiAllocateCell(PREGISTRY_HIVE  RegistryHive,
		LONG  CellSize,
		PVOID  *Cell,
		BLOCK_OFFSET *CellOffset);

NTSTATUS
CmiDestroyCell(PREGISTRY_HIVE  RegistryHive,
  PVOID  Cell,
  BLOCK_OFFSET CellOffset);

PVOID
CmiGetCell (PREGISTRY_HIVE  RegistryHive,
	    BLOCK_OFFSET  CellOffset,
	    OUT PHBIN * ppBin);

VOID
CmiMarkBlockDirty(PREGISTRY_HIVE RegistryHive,
		  BLOCK_OFFSET BlockOffset);

VOID
CmiMarkBinDirty(PREGISTRY_HIVE RegistryHive,
		BLOCK_OFFSET BinOffset);

NTSTATUS
CmiAddFree(PREGISTRY_HIVE  RegistryHive,
	   PCELL_HEADER FreeBlock,
	   BLOCK_OFFSET FreeOffset,
	   BOOLEAN MergeFreeBlocks);

NTSTATUS
CmiConnectHive(POBJECT_ATTRIBUTES KeyObjectAttributes,
	       PREGISTRY_HIVE RegistryHive);

NTSTATUS
CmiDisconnectHive (POBJECT_ATTRIBUTES KeyObjectAttributes,
		   PREGISTRY_HIVE *RegistryHive);

NTSTATUS
CmiInitHives(BOOLEAN SetupBoot);

ULONG
CmiGetPackedNameLength(IN PUNICODE_STRING Name,
		       OUT PBOOLEAN Packable);

BOOLEAN
CmiComparePackedNames(IN PUNICODE_STRING Name,
		      IN PCHAR NameBuffer,
		      IN USHORT NameBufferSize,
		      IN BOOLEAN NamePacked);

VOID
CmiCopyPackedName(PWCHAR NameBuffer,
		  PCHAR PackedNameBuffer,
		  ULONG PackedNameSize);

BOOLEAN
CmiCompareHash(PUNICODE_STRING KeyName,
	       PCHAR HashString);

BOOLEAN
CmiCompareHashI(PUNICODE_STRING KeyName,
		PCHAR HashString);

BOOLEAN
CmiCompareKeyNames(PUNICODE_STRING KeyName,
		   PKEY_CELL KeyCell);

BOOLEAN
CmiCompareKeyNamesI(PUNICODE_STRING KeyName,
		    PKEY_CELL KeyCell);


VOID
CmiSyncHives(VOID);


NTSTATUS
CmiCreateTempHive(PREGISTRY_HIVE *RegistryHive);

NTSTATUS
CmiCopyKey (PREGISTRY_HIVE DstHive,
	    PKEY_CELL DstKeyCell,
	    PREGISTRY_HIVE SrcHive,
	    PKEY_CELL SrcKeyCell);

NTSTATUS
CmiSaveTempHive (PREGISTRY_HIVE Hive,
		 HANDLE FileHandle);

#endif /*__INCLUDE_CM_H*/
