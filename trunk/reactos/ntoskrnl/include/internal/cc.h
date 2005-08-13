#ifndef __INCLUDE_INTERNAL_CC_H
#define __INCLUDE_INTERNAL_CC_H

typedef struct _BCB
{
    LIST_ENTRY BcbSegmentListHead;
    LIST_ENTRY BcbRemoveListEntry;
    BOOLEAN RemoveOnClose;
    ULONG TimeStamp;
    PFILE_OBJECT FileObject;
    ULONG CacheSegmentSize;
    LARGE_INTEGER AllocationSize;
    LARGE_INTEGER FileSize;
    KSPIN_LOCK BcbLock;
    ULONG RefCount;
#if defined(DBG) || defined(KDBG)
	BOOLEAN Trace; /* enable extra trace output for this BCB and it's cache segments */
#endif
} BCB, *PBCB;

typedef struct _CACHE_SEGMENT
{
    /* Base address of the region where the cache segment data is mapped. */
    PVOID BaseAddress;
    /*
     * Memory area representing the region where the cache segment data is
     * mapped.
     */
    struct _MEMORY_AREA* MemoryArea;
    /* Are the contents of the cache segment data valid. */
    BOOLEAN Valid;
    /* Are the contents of the cache segment data newer than those on disk. */
    BOOLEAN Dirty;
    /* Page out in progress */
    BOOLEAN PageOut;
    ULONG MappedCount;
    /* Entry in the list of segments for this BCB. */
    LIST_ENTRY BcbSegmentListEntry;
    /* Entry in the list of segments which are dirty. */
    LIST_ENTRY DirtySegmentListEntry;
    /* Entry in the list of segments. */
    LIST_ENTRY CacheSegmentListEntry;
    LIST_ENTRY CacheSegmentLRUListEntry;
    /* Offset in the file which this cache segment maps. */
    ULONG FileOffset;
    /* Lock. */
    FAST_MUTEX Lock;
    /* Number of references. */
    ULONG ReferenceCount;
    /* Pointer to the BCB for the file which this cache segment maps data for. */
    PBCB Bcb;
    /* Pointer to the next cache segment in a chain. */
    struct _CACHE_SEGMENT* NextInChain;
} CACHE_SEGMENT, *PCACHE_SEGMENT;

typedef struct _INTERNAL_BCB
{
    PUBLIC_BCB PFCB;
    PCACHE_SEGMENT CacheSegment;
    BOOLEAN Dirty;
    CSHORT RefCount; /* (At offset 0x34 on WinNT4) */
} INTERNAL_BCB, *PINTERNAL_BCB;

VOID
STDCALL
CcMdlReadCompleteDev(
    IN PMDL MdlChain,
    IN PFILE_OBJECT FileObject
);

VOID
STDCALL
CcMdlWriteCompleteDev(
    IN PLARGE_INTEGER FileOffset,
    IN PMDL MdlChain,
    IN PFILE_OBJECT FileObject
);

NTSTATUS
CcRosFlushCacheSegment(PCACHE_SEGMENT CacheSegment);

NTSTATUS
CcRosGetCacheSegment(
    PBCB Bcb,
    ULONG FileOffset,
    PULONG BaseOffset,
    PVOID* BaseAddress,
    PBOOLEAN UptoDate,
    PCACHE_SEGMENT* CacheSeg
);

VOID
CcInitView(VOID);

NTSTATUS
CcRosFreeCacheSegment(
    PBCB,
    PCACHE_SEGMENT
);

NTSTATUS
ReadCacheSegment(PCACHE_SEGMENT CacheSeg);

NTSTATUS
WriteCacheSegment(PCACHE_SEGMENT CacheSeg);

VOID CcInit(VOID);

NTSTATUS
CcRosUnmapCacheSegment(
    PBCB Bcb,
    ULONG FileOffset,
    BOOLEAN NowDirty
);

PCACHE_SEGMENT
CcRosLookupCacheSegment(
    PBCB Bcb,
    ULONG FileOffset
);

NTSTATUS
CcRosGetCacheSegmentChain(
    PBCB Bcb,
    ULONG FileOffset,
    ULONG Length,
    PCACHE_SEGMENT* CacheSeg
);

VOID
CcInitCacheZeroPage(VOID);

NTSTATUS
CcRosMarkDirtyCacheSegment(
    PBCB Bcb,
    ULONG FileOffset
);

NTSTATUS
CcRosFlushDirtyPages(
    ULONG Target,
    PULONG Count
);

VOID
CcRosDereferenceCache(PFILE_OBJECT FileObject);

VOID
CcRosReferenceCache(PFILE_OBJECT FileObject);

VOID
CcRosSetRemoveOnClose(PSECTION_OBJECT_POINTERS SectionObjectPointer);

NTSTATUS
CcRosReleaseCacheSegment(
    BCB*		    Bcb,
    CACHE_SEGMENT*    CacheSeg,
    BOOLEAN	    Valid,
    BOOLEAN	    Dirty,
    BOOLEAN	    Mapped
);

NTSTATUS
STDCALL
CcRosRequestCacheSegment(
    BCB*		    Bcb,
    ULONG		    FileOffset,
    PVOID*	    BaseAddress,
    PBOOLEAN	    UptoDate,
    CACHE_SEGMENT**   CacheSeg
);

NTSTATUS
CcTryToInitializeFileCache(PFILE_OBJECT FileObject);

/*
 * Macro for generic cache manage bugchecking. Note that this macro assumes
 * that the file name including extension is always longer than 4 characters.
 */
#define KEBUGCHECKCC \
    KEBUGCHECKEX(CACHE_MANAGER, \
    (*(DWORD*)(__FILE__ + sizeof(__FILE__) - 4) << 16) | \
    (__LINE__ & 0xFFFF), 0, 0, 0)

#endif
