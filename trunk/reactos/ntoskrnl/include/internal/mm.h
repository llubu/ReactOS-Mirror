/*
 * Higher level memory managment definitions
 */

#ifndef __INCLUDE_INTERNAL_MM_H
#define __INCLUDE_INTERNAL_MM_H

#include <internal/ntoskrnl.h>
#include <internal/mmhal.h>

/* TYPES *********************************************************************/

struct _EPROCESS;
typedef ULONG SWAPENTRY;

enum
{
   MEMORY_AREA_INVALID,
   MEMORY_AREA_SECTION_VIEW_COMMIT,
   MEMORY_AREA_CONTINUOUS_MEMORY,
   MEMORY_AREA_NO_CACHE,
   MEMORY_AREA_IO_MAPPING,
   MEMORY_AREA_SYSTEM,
   MEMORY_AREA_MDL_MAPPING,
   MEMORY_AREA_COMMIT,
   MEMORY_AREA_RESERVE,
   MEMORY_AREA_SECTION_VIEW_RESERVE,
   MEMORY_AREA_CACHE_SEGMENT,
};

#define PAGE_TO_SECTION_PAGE_DIRECTORY_OFFSET(x) \
                          ((x) / (4*1024*1024))
#define PAGE_TO_SECTION_PAGE_TABLE_OFFSET(x) \
                      ((((x)) % (4*1024*1024)) / (4*1024))

#define NR_SECTION_PAGE_TABLES           (1024)
#define NR_SECTION_PAGE_ENTRIES          (1024)

#define SPE_PAGEIN_PENDING                      (0x1)
#define SPE_MPW_PENDING                         (0x2)
#define SPE_PAGEOUT_PENDING                     (0x4)
#define SPE_DIRTY                               (0x8)
#define SPE_IN_PAGEFILE                         (0x10)

typedef struct
{
   ULONG Pages[NR_SECTION_PAGE_ENTRIES];
} SECTION_PAGE_TABLE, *PSECTION_PAGE_TABLE;

typedef struct
{
   PSECTION_PAGE_TABLE PageTables[NR_SECTION_PAGE_TABLES];
} SECTION_PAGE_DIRECTORY, *PSECTION_PAGE_DIRECTORY;

typedef struct
{
   CSHORT Type;
   CSHORT Size;
   LARGE_INTEGER MaximumSize;
   ULONG SectionPageProtection;
   ULONG AllocateAttributes;
   PFILE_OBJECT FileObject;
   LIST_ENTRY ViewListHead;
   KSPIN_LOCK ViewListLock;
   KMUTEX Lock;
   SECTION_PAGE_DIRECTORY PageDirectory;
} SECTION_OBJECT, *PSECTION_OBJECT;

typedef struct
{
   ULONG Type;
   PVOID BaseAddress;
   ULONG Length;
   ULONG Attributes;
   LIST_ENTRY Entry;
   ULONG LockCount;
   struct _EPROCESS* Process;
   union
     {
	struct
	  {	     
	     SECTION_OBJECT* Section;
	     ULONG ViewOffset;
	     LIST_ENTRY ViewListEntry;
	  } SectionData;
     } Data;
} MEMORY_AREA, *PMEMORY_AREA;

typedef struct _MWORKING_SET
{
   PVOID Address[1020];
   struct _MWORKING_SET* Next;
} MWORKING_SET, *PMWORKING_SET;

typedef struct _MADDRESS_SPACE
{
   LIST_ENTRY MAreaListHead;
   KMUTEX Lock;
   ULONG LowestAddress;
   struct _EPROCESS* Process;
   ULONG WorkingSetSize;
   ULONG WorkingSetLruFirst;
   ULONG WorkingSetLruLast;
   ULONG WorkingSetPagesAllocated;
} MADDRESS_SPACE, *PMADDRESS_SPACE;

/* FUNCTIONS */

VOID MmLockAddressSpace(PMADDRESS_SPACE AddressSpace);
VOID MmUnlockAddressSpace(PMADDRESS_SPACE AddressSpace);
VOID MmInitializeKernelAddressSpace(VOID);
PMADDRESS_SPACE MmGetCurrentAddressSpace(VOID);
PMADDRESS_SPACE MmGetKernelAddressSpace(VOID);
NTSTATUS MmInitializeAddressSpace(struct _EPROCESS* Process,
				  PMADDRESS_SPACE AddressSpace);
NTSTATUS MmDestroyAddressSpace(PMADDRESS_SPACE AddressSpace);
PVOID STDCALL MmAllocateSection (IN ULONG Length);
NTSTATUS MmCreateMemoryArea(struct _EPROCESS* Process,
			    PMADDRESS_SPACE AddressSpace,
			    ULONG Type,
			    PVOID* BaseAddress,
			    ULONG Length,
			    ULONG Attributes,
			    MEMORY_AREA** Result);
MEMORY_AREA* MmOpenMemoryAreaByAddress(PMADDRESS_SPACE AddressSpace, 
				       PVOID Address);
NTSTATUS MmInitMemoryAreas(VOID);
VOID ExInitNonPagedPool(ULONG BaseAddress);
NTSTATUS MmFreeMemoryArea(PMADDRESS_SPACE AddressSpace,
			  PVOID BaseAddress,
			  ULONG Length,
			  BOOLEAN FreePages);
VOID MmDumpMemoryAreas(PLIST_ENTRY ListHead);
NTSTATUS MmLockMemoryArea(MEMORY_AREA* MemoryArea);
NTSTATUS MmUnlockMemoryArea(MEMORY_AREA* MemoryArea);
NTSTATUS MmInitSectionImplementation(VOID);

#define MM_LOWEST_USER_ADDRESS (4096)

PMEMORY_AREA MmSplitMemoryArea(struct _EPROCESS* Process,
			       PMADDRESS_SPACE AddressSpace,
			       PMEMORY_AREA OriginalMemoryArea,
			       PVOID BaseAddress,
			       ULONG Length,
			       ULONG NewType,
			       ULONG NewAttributes);
PVOID MmInitializePageList(PVOID FirstPhysKernelAddress,
			   PVOID LastPhysKernelAddress,
			   ULONG MemorySizeInPages,
			   ULONG LastKernelBase);

PVOID MmAllocPage(SWAPENTRY SavedSwapEntry);
VOID MmDereferencePage(PVOID PhysicalAddress);
VOID MmReferencePage(PVOID PhysicalAddress);
VOID MmDeletePageTable(struct _EPROCESS* Process, 
		       PVOID Address);
NTSTATUS MmCopyMmInfo(struct _EPROCESS* Src, 
		      struct _EPROCESS* Dest);
NTSTATUS MmReleaseMmInfo(struct _EPROCESS* Process);
NTSTATUS Mmi386ReleaseMmInfo(struct _EPROCESS* Process);
VOID MmDeletePageEntry(struct _EPROCESS* Process, 
		       PVOID Address, 
		       BOOL FreePage);

VOID MmBuildMdlFromPages(PMDL Mdl);
PVOID MmGetMdlPageAddress(PMDL Mdl, PVOID Offset);
VOID MiShutdownMemoryManager(VOID);
ULONG MmGetPhysicalAddressForProcess(struct _EPROCESS* Process,
				     PVOID Address);
NTSTATUS STDCALL MmUnmapViewOfSection(struct _EPROCESS* Process,
				      PMEMORY_AREA MemoryArea);
NTSTATUS MmSafeCopyFromUser(PVOID Dest, PVOID Src, ULONG NumberOfBytes);
NTSTATUS MmSafeCopyToUser(PVOID Dest, PVOID Src, ULONG NumberOfBytes);
VOID MmInitPagingFile(VOID);

/* FIXME: it should be in ddk/mmfuncs.h */
NTSTATUS
STDCALL
MmCreateSection (
	OUT	PSECTION_OBJECT		* SectionObject,
	IN	ACCESS_MASK		DesiredAccess,
	IN	POBJECT_ATTRIBUTES	ObjectAttributes	OPTIONAL,
	IN	PLARGE_INTEGER		MaximumSize,
	IN	ULONG			SectionPageProtection,
	IN	ULONG			AllocationAttributes,
	IN	HANDLE			FileHandle		OPTIONAL,
	IN	PFILE_OBJECT		File			OPTIONAL
	);

NTSTATUS MmPageFault(ULONG Cs,
		     PULONG Eip,
		     PULONG Eax,
		     ULONG Cr2,
		     ULONG ErrorCode);

NTSTATUS MmAccessFault(KPROCESSOR_MODE Mode,
		       ULONG Address);
NTSTATUS MmNotPresentFault(KPROCESSOR_MODE Mode,
			   ULONG Address);
NTSTATUS MmNotPresentFaultVirtualMemory(PMADDRESS_SPACE AddressSpace,
					MEMORY_AREA* MemoryArea, 
					PVOID Address);
NTSTATUS MmNotPresentFaultSectionView(PMADDRESS_SPACE AddressSpace,
				      MEMORY_AREA* MemoryArea, 
				      PVOID Address);
NTSTATUS MmWaitForPage(PVOID Page);
VOID MmClearWaitPage(PVOID Page);
VOID MmSetWaitPage(PVOID Page);
BOOLEAN MmIsPageDirty(struct _EPROCESS* Process, PVOID Address);
BOOLEAN MmIsPageTablePresent(PVOID PAddress);
ULONG MmPageOutSectionView(PMADDRESS_SPACE AddressSpace,
			   MEMORY_AREA* MemoryArea, 
			   PVOID Address);
ULONG MmPageOutVirtualMemory(PMADDRESS_SPACE AddressSpace,
			     PMEMORY_AREA MemoryArea,
			     PVOID Address);
MEMORY_AREA* MmOpenMemoryAreaByRegion(PMADDRESS_SPACE AddressSpace, 
				      PVOID Address,
				      ULONG Length);

VOID ExUnmapPage(PVOID Addr);
PVOID ExAllocatePage(VOID);

VOID MmLockWorkingSet(struct _EPROCESS* Process);
VOID MmUnlockWorkingSet(struct _EPROCESS* Process);
VOID MmInitializeWorkingSet(struct _EPROCESS* Process,
			    PMADDRESS_SPACE AddressSpace);
ULONG MmTrimWorkingSet(struct _EPROCESS* Process,
		       ULONG ReduceHint);
VOID MmRemovePageFromWorkingSet(struct _EPROCESS* Process,
				PVOID Address);
BOOLEAN MmAddPageToWorkingSet(struct _EPROCESS* Process,
			      PVOID Address);

VOID MmInitPagingFile(VOID);
VOID MmReserveSwapPages(ULONG Nr);
VOID MmDereserveSwapPages(ULONG Nr);
SWAPENTRY MmAllocSwapPage(VOID);
VOID MmFreeSwapPage(SWAPENTRY Entry);

VOID MmInit1(boot_param* bp, ULONG LastKernelAddress);
VOID MmInit2(VOID);
VOID MmInit3(VOID);
NTSTATUS MmInitPagerThread(VOID);

VOID MmInitKernelMap(PVOID BaseAddress);
unsigned int alloc_pool_region(unsigned int nr_pages);

#endif
