#ifndef __INCLUDE_DDK_NTIFS_H
#define __INCLUDE_DDK_NTIFS_H

#if 0
typedef struct
{
   BOOLEAN Replace;
   HANDLE RootDir;
   ULONG FileNameLength;
   WCHAR FileName[1];
} FILE_RENAME_INFORMATION, *PFILE_RENAME_INFORMATION;
#endif 

typedef struct _BCB
{
   LIST_ENTRY CacheSegmentListHead;
   PFILE_OBJECT FileObject;
   KSPIN_LOCK BcbLock;
} BCB, *PBCB;

#define CACHE_SEGMENT_SIZE (0x1000)

struct _MEMORY_AREA;

typedef struct _CACHE_SEGMENT
{
   PVOID BaseAddress;
   struct _MEMORY_AREA* MemoryArea;
   BOOLEAN Valid;
   LIST_ENTRY ListEntry;
   ULONG FileOffset;
   KEVENT Lock;
   ULONG ReferenceCount;
   PBCB Bcb;
} CACHE_SEGMENT, *PCACHE_SEGMENT;

NTSTATUS CcFlushCachePage(PCACHE_SEGMENT CacheSeg);
NTSTATUS CcReleaseCachePage(PBCB Bcb,
			    PCACHE_SEGMENT CacheSeg,
			    BOOLEAN Valid);
NTSTATUS CcRequestCachePage(PBCB Bcb,
			    ULONG FileOffset,
			    PVOID* BaseAddress,
			    PBOOLEAN UptoDate,
			    PCACHE_SEGMENT* CacheSeg);
NTSTATUS CcInitializeFileCache(PFILE_OBJECT FileObject,
			       PBCB* Bcb);
NTSTATUS CcReleaseFileCache(PFILE_OBJECT FileObject,
			    PBCB Bcb);

#include <ddk/cctypes.h>

#include <ddk/ccfuncs.h>

#endif /* __INCLUDE_DDK_NTIFS_H */
