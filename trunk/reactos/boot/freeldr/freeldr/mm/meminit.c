/*
 *  FreeLoader
 *  Copyright (C) 1998-2003  Brian Palmer  <brianp@sginet.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <freeldr.h>

#define NDEBUG
#include <debug.h>

#ifdef DBG
typedef struct
{
	ULONG		Type;
	UCHAR	TypeString[20];
} FREELDR_MEMORY_TYPE, *PFREELDR_MEMORY_TYPE;

ULONG				MemoryTypeCount = 5;
FREELDR_MEMORY_TYPE		MemoryTypeArray[] =
{
	{ 0, "Unknown Memory" },
	{ BiosMemoryUsable, "Usable Memory" },
	{ BiosMemoryReserved, "Reserved Memory" },
	{ BiosMemoryAcpiReclaim, "ACPI Reclaim Memory" },
	{ BiosMemoryAcpiNvs, "ACPI NVS Memory" },
};
#endif

PVOID	PageLookupTableAddress = NULL;
ULONG		TotalPagesInLookupTable = 0;
ULONG		FreePagesInLookupTable = 0;
ULONG		LastFreePageHint = 0;

BOOLEAN MmInitializeMemoryManager(VOID)
{
	BIOS_MEMORY_MAP	BiosMemoryMap[32];
	ULONG		BiosMemoryMapEntryCount;
#ifdef DBG
	ULONG		Index;
#endif

	DbgPrint((DPRINT_MEMORY, "Initializing Memory Manager.\n"));

	RtlZeroMemory(BiosMemoryMap, sizeof(BIOS_MEMORY_MAP) * 32);

	BiosMemoryMapEntryCount = MachGetMemoryMap(BiosMemoryMap, sizeof(BiosMemoryMap) / sizeof(BIOS_MEMORY_MAP));

#ifdef DBG
	// Dump the system memory map
	if (BiosMemoryMapEntryCount != 0)
	{
		DbgPrint((DPRINT_MEMORY, "System Memory Map (Base Address, Length, Type):\n"));
		for (Index=0; Index<BiosMemoryMapEntryCount; Index++)
		{
			DbgPrint((DPRINT_MEMORY, "%x%x\t %x%x\t %s\n", BiosMemoryMap[Index].BaseAddress, BiosMemoryMap[Index].Length, MmGetSystemMemoryMapTypeString(BiosMemoryMap[Index].Type)));
		}
	}
#endif

	// If we got the system memory map then fixup invalid entries
	if (BiosMemoryMapEntryCount != 0)
	{
		MmFixupSystemMemoryMap(BiosMemoryMap, &BiosMemoryMapEntryCount);
	}

	// Find address for the page lookup table
	TotalPagesInLookupTable = MmGetAddressablePageCountIncludingHoles(BiosMemoryMap, BiosMemoryMapEntryCount);
	PageLookupTableAddress = MmFindLocationForPageLookupTable(BiosMemoryMap, BiosMemoryMapEntryCount);
	LastFreePageHint = TotalPagesInLookupTable;

	if (PageLookupTableAddress == 0)
	{
		// If we get here then we probably couldn't
		// find a contiguous chunk of memory big
		// enough to hold the page lookup table
		printf("Error initializing memory manager!\n");
		return FALSE;
	}

	// Initialize the page lookup table
	MmInitPageLookupTable(PageLookupTableAddress, TotalPagesInLookupTable, BiosMemoryMap, BiosMemoryMapEntryCount);
	MmUpdateLastFreePageHint(PageLookupTableAddress, TotalPagesInLookupTable);

	FreePagesInLookupTable = MmCountFreePagesInLookupTable(PageLookupTableAddress, TotalPagesInLookupTable);

	DbgPrint((DPRINT_MEMORY, "Memory Manager initialized. %d pages available.\n", FreePagesInLookupTable));
	return TRUE;
}

#ifdef DBG
PUCHAR MmGetSystemMemoryMapTypeString(ULONG Type)
{
	ULONG		Index;

	for (Index=1; Index<MemoryTypeCount; Index++)
	{
		if (MemoryTypeArray[Index].Type == Type)
		{
			return MemoryTypeArray[Index].TypeString;
		}
	}

	return MemoryTypeArray[0].TypeString;
}
#endif

ULONG MmGetPageNumberFromAddress(PVOID Address)
{
	return ((ULONG)Address) / MM_PAGE_SIZE;
}

PVOID MmGetEndAddressOfAnyMemory(PBIOS_MEMORY_MAP BiosMemoryMap, ULONG MapCount)
{
	ULONGLONG		MaxStartAddressSoFar;
	ULONGLONG		EndAddressOfMemory;
	ULONG		Index;

	MaxStartAddressSoFar = 0;
	EndAddressOfMemory = 0;
	for (Index=0; Index<MapCount; Index++)
	{
		if (MaxStartAddressSoFar <= BiosMemoryMap[Index].BaseAddress)
		{
			MaxStartAddressSoFar = BiosMemoryMap[Index].BaseAddress;
			EndAddressOfMemory = (MaxStartAddressSoFar + BiosMemoryMap[Index].Length);
			if (EndAddressOfMemory > 0xFFFFFFFF)
			{
				EndAddressOfMemory = 0xFFFFFFFF;
			}
		}
	}

	DbgPrint((DPRINT_MEMORY, "MmGetEndAddressOfAnyMemory() returning 0x%x\n", (ULONG)EndAddressOfMemory));

	return (PVOID)(ULONG)EndAddressOfMemory;
}

ULONG MmGetAddressablePageCountIncludingHoles(PBIOS_MEMORY_MAP BiosMemoryMap, ULONG MapCount)
{
	ULONG		PageCount;
	ULONGLONG		EndAddress;

	EndAddress = (ULONGLONG)(ULONG)MmGetEndAddressOfAnyMemory(BiosMemoryMap, MapCount);

	// Since MmGetEndAddressOfAnyMemory() won't
	// return addresses higher than 0xFFFFFFFF
	// then we need to adjust the end address
	// to 0x100000000 so we don't get an
	// off-by-one error
	if (EndAddress >= 0xFFFFFFFF)
	{
		EndAddress = 0x100000000LL;

		DbgPrint((DPRINT_MEMORY, "MmGetEndAddressOfAnyMemory() returned 0xFFFFFFFF, correcting to be 0x100000000.\n"));
	}

	PageCount = (EndAddress / MM_PAGE_SIZE);

	DbgPrint((DPRINT_MEMORY, "MmGetAddressablePageCountIncludingHoles() returning %d\n", PageCount));

	return PageCount;
}

PVOID MmFindLocationForPageLookupTable(PBIOS_MEMORY_MAP BiosMemoryMap, ULONG MapCount)
{
	ULONG					TotalPageCount;
	ULONG					PageLookupTableSize;
	PVOID				PageLookupTableMemAddress;
	int					Index;
	BIOS_MEMORY_MAP		TempBiosMemoryMap[32];

	TotalPageCount = MmGetAddressablePageCountIncludingHoles(BiosMemoryMap, MapCount);
	PageLookupTableSize = TotalPageCount * sizeof(PAGE_LOOKUP_TABLE_ITEM);
	PageLookupTableMemAddress = 0;

	RtlCopyMemory(TempBiosMemoryMap, BiosMemoryMap, sizeof(BIOS_MEMORY_MAP) * 32);
	MmSortBiosMemoryMap(TempBiosMemoryMap, MapCount);

	for (Index=(MapCount-1); Index>=0; Index--)
	{
		// If this is usable memory with a big enough length
		// then we'll put our page lookup table here
		if (TempBiosMemoryMap[Index].Type == BiosMemoryUsable && TempBiosMemoryMap[Index].Length >= PageLookupTableSize)
		{
			PageLookupTableMemAddress = (PVOID)(ULONG)(TempBiosMemoryMap[Index].BaseAddress + (TempBiosMemoryMap[Index].Length - PageLookupTableSize));
			break;
		}
	}

	DbgPrint((DPRINT_MEMORY, "MmFindLocationForPageLookupTable() returning 0x%x\n", PageLookupTableMemAddress));

	return PageLookupTableMemAddress;
}

VOID MmSortBiosMemoryMap(PBIOS_MEMORY_MAP BiosMemoryMap, ULONG MapCount)
{
	ULONG					Index;
	ULONG					LoopCount;
	BIOS_MEMORY_MAP		TempMapItem;

	// Loop once for each entry in the memory map minus one
	// On each loop iteration go through and sort the memory map
	for (LoopCount=0; LoopCount<(MapCount-1); LoopCount++)
	{
		for (Index=0; Index<(MapCount-1); Index++)
		{
			if (BiosMemoryMap[Index].BaseAddress > BiosMemoryMap[Index+1].BaseAddress)
			{
				TempMapItem = BiosMemoryMap[Index];
				BiosMemoryMap[Index] = BiosMemoryMap[Index+1];
				BiosMemoryMap[Index+1] = TempMapItem;
			}
		}
	}
}

VOID MmInitPageLookupTable(PVOID PageLookupTable, ULONG TotalPageCount, PBIOS_MEMORY_MAP BiosMemoryMap, ULONG MapCount)
{
	ULONG		MemoryMapStartPage;
	ULONG		MemoryMapEndPage;
	ULONG		MemoryMapPageCount;
	ULONG		MemoryMapPageAllocated;
	ULONG		PageLookupTableStartPage;
	ULONG		PageLookupTablePageCount;
	ULONG		Index;

	DbgPrint((DPRINT_MEMORY, "MmInitPageLookupTable()\n"));

	// Mark every page as allocated initially
	// We will go through and mark pages again according to the memory map
	// But this will mark any holes not described in the map as allocated
	MmMarkPagesInLookupTable(PageLookupTable, 0, TotalPageCount, 1);

	for (Index=0; Index<MapCount; Index++)
	{
		MemoryMapStartPage = MmGetPageNumberFromAddress((PVOID)(ULONG)BiosMemoryMap[Index].BaseAddress);
		MemoryMapEndPage = MmGetPageNumberFromAddress((PVOID)(ULONG)(BiosMemoryMap[Index].BaseAddress + BiosMemoryMap[Index].Length - 1));
		MemoryMapPageCount = (MemoryMapEndPage - MemoryMapStartPage) + 1;
		MemoryMapPageAllocated = (BiosMemoryMap[Index].Type == BiosMemoryUsable) ? LoaderFree : LoaderFirmwarePermanent;/*BiosMemoryMap[Index].Type*/;
		DbgPrint((DPRINT_MEMORY, "Marking pages as type %d: StartPage: %d PageCount: %d\n", MemoryMapPageAllocated, MemoryMapStartPage, MemoryMapPageCount));
		MmMarkPagesInLookupTable(PageLookupTable, MemoryMapStartPage, MemoryMapPageCount, MemoryMapPageAllocated);
	}

	// Mark the low memory region below 1MB as reserved (256 pages in region)
	DbgPrint((DPRINT_MEMORY, "Marking the low 1MB region as reserved.\n"));
	MmMarkPagesInLookupTable(PageLookupTable, 0, 256, LoaderFirmwarePermanent);

	// Mark the pages that the lookup table occupies as reserved
	PageLookupTableStartPage = MmGetPageNumberFromAddress(PageLookupTable);
	PageLookupTablePageCount = MmGetPageNumberFromAddress((PVOID)((ULONG_PTR)PageLookupTable + ROUND_UP(TotalPageCount * sizeof(PAGE_LOOKUP_TABLE_ITEM), MM_PAGE_SIZE))) - PageLookupTableStartPage;
	DbgPrint((DPRINT_MEMORY, "Marking the page lookup table pages as reserved StartPage: %d PageCount: %d\n", PageLookupTableStartPage, PageLookupTablePageCount));
	MmMarkPagesInLookupTable(PageLookupTable, PageLookupTableStartPage, PageLookupTablePageCount, LoaderFirmwareTemporary);
}

VOID MmMarkPagesInLookupTable(PVOID PageLookupTable, ULONG StartPage, ULONG PageCount, TYPE_OF_MEMORY PageAllocated)
{
	PPAGE_LOOKUP_TABLE_ITEM		RealPageLookupTable = (PPAGE_LOOKUP_TABLE_ITEM)PageLookupTable;
	ULONG							Index;

	for (Index=StartPage; Index<(StartPage+PageCount); Index++)
	{
		if ((Index <= (StartPage + 16)) || (Index >= (StartPage+PageCount-16)))
		{
			DbgPrint((DPRINT_MEMORY, "Index = %d StartPage = %d PageCount = %d\n", Index, StartPage, PageCount));
		}
		RealPageLookupTable[Index].PageAllocated = PageAllocated;
		RealPageLookupTable[Index].PageAllocationLength = (PageAllocated != LoaderFree) ? 1 : 0;
	}
	DbgPrint((DPRINT_MEMORY, "MmMarkPagesInLookupTable() Done\n"));
}

VOID MmAllocatePagesInLookupTable(PVOID PageLookupTable, ULONG StartPage, ULONG PageCount)
{
	PPAGE_LOOKUP_TABLE_ITEM		RealPageLookupTable = (PPAGE_LOOKUP_TABLE_ITEM)PageLookupTable;
	ULONG							Index;

	for (Index=StartPage; Index<(StartPage+PageCount); Index++)
	{
		RealPageLookupTable[Index].PageAllocated = LoaderSystemCode;
		RealPageLookupTable[Index].PageAllocationLength = (Index == StartPage) ? PageCount : 0;
	}
}

ULONG MmCountFreePagesInLookupTable(PVOID PageLookupTable, ULONG TotalPageCount)
{
	PPAGE_LOOKUP_TABLE_ITEM		RealPageLookupTable = (PPAGE_LOOKUP_TABLE_ITEM)PageLookupTable;
	ULONG							Index;
	ULONG							FreePageCount;

	FreePageCount = 0;
	for (Index=0; Index<TotalPageCount; Index++)
	{
		if (RealPageLookupTable[Index].PageAllocated == LoaderFree)
		{
			FreePageCount++;
		}
	}

	return FreePageCount;
}

ULONG MmFindAvailablePages(PVOID PageLookupTable, ULONG TotalPageCount, ULONG PagesNeeded, BOOLEAN FromEnd)
{
	PPAGE_LOOKUP_TABLE_ITEM		RealPageLookupTable = (PPAGE_LOOKUP_TABLE_ITEM)PageLookupTable;
	ULONG							AvailablePagesSoFar;
	ULONG							Index;

	if (LastFreePageHint > TotalPageCount)
	{
		LastFreePageHint = TotalPageCount;
	}

	AvailablePagesSoFar = 0;
	if (FromEnd)
	{
		/* Allocate "high" (from end) pages */
		for (Index=LastFreePageHint-1; Index>0; Index--)
		{
			if (RealPageLookupTable[Index].PageAllocated != LoaderFree)
			{
				AvailablePagesSoFar = 0;
				continue;
			}
			else
			{
				AvailablePagesSoFar++;
			}

			if (AvailablePagesSoFar >= PagesNeeded)
			{
				return Index;
			}
		}
	}
	else
	{
		DbgPrint((DPRINT_MEMORY, "Alloc low memory, LastFreePageHint %d, TPC %d\n", LastFreePageHint, TotalPageCount));
		/* Allocate "low" pages */
		for (Index=1; Index < LastFreePageHint; Index++)
		{
			if (RealPageLookupTable[Index].PageAllocated != LoaderFree)
			{
				AvailablePagesSoFar = 0;
				continue;
			}
			else
			{
				AvailablePagesSoFar++;
			}

			if (AvailablePagesSoFar >= PagesNeeded)
			{
				return Index - AvailablePagesSoFar + 1;
			}
		}
	}

	return 0;
}

ULONG MmFindAvailablePagesBeforePage(PVOID PageLookupTable, ULONG TotalPageCount, ULONG PagesNeeded, ULONG LastPage)
{
	PPAGE_LOOKUP_TABLE_ITEM		RealPageLookupTable = (PPAGE_LOOKUP_TABLE_ITEM)PageLookupTable;
	ULONG							AvailablePagesSoFar;
	ULONG							Index;

	if (LastPage > TotalPageCount)
	{
		return MmFindAvailablePages(PageLookupTable, TotalPageCount, PagesNeeded, TRUE);
	}

	AvailablePagesSoFar = 0;
	for (Index=LastPage-1; Index>0; Index--)
	{
		if (RealPageLookupTable[Index].PageAllocated != LoaderFree)
		{
			AvailablePagesSoFar = 0;
			continue;
		}
		else
		{
			AvailablePagesSoFar++;
		}

		if (AvailablePagesSoFar >= PagesNeeded)
		{
			return Index;
		}
	}

	return 0;
}

VOID MmFixupSystemMemoryMap(PBIOS_MEMORY_MAP BiosMemoryMap, ULONG* MapCount)
{
	UINT		Index;
	UINT		Index2;

	// Loop through each entry in the array
	for (Index=0; Index<*MapCount; Index++)
	{
		// If the entry type isn't usable then remove
		// it from the memory map (this will help reduce
		// the size of our lookup table)
		if (BiosMemoryMap[Index].Type != BiosMemoryUsable)
		{
			// Slide every entry after this down one
			for (Index2=Index; Index2<(*MapCount - 1); Index2++)
			{
				BiosMemoryMap[Index2] = BiosMemoryMap[Index2 + 1];
			}
			(*MapCount)--;
			Index--;
		}
	}
}

VOID MmUpdateLastFreePageHint(PVOID PageLookupTable, ULONG TotalPageCount)
{
	PPAGE_LOOKUP_TABLE_ITEM		RealPageLookupTable = (PPAGE_LOOKUP_TABLE_ITEM)PageLookupTable;
	ULONG							Index;

	for (Index=TotalPageCount-1; Index>0; Index--)
	{
		if (RealPageLookupTable[Index].PageAllocated == LoaderFree)
		{
			LastFreePageHint = Index + 1;
			break;
		}
	}
}

BOOLEAN MmAreMemoryPagesAvailable(PVOID PageLookupTable, ULONG TotalPageCount, PVOID PageAddress, ULONG PageCount)
{
	PPAGE_LOOKUP_TABLE_ITEM		RealPageLookupTable = (PPAGE_LOOKUP_TABLE_ITEM)PageLookupTable;
	ULONG							StartPage;
	ULONG							Index;

	StartPage = MmGetPageNumberFromAddress(PageAddress);

	// Make sure they aren't trying to go past the
	// end of availabe memory
	if ((StartPage + PageCount) > TotalPageCount)
	{
		return FALSE;
	}

	for (Index=StartPage; Index<(StartPage + PageCount); Index++)
	{
		// If this page is allocated then there obviously isn't
		// memory availabe so return FALSE
		if (RealPageLookupTable[Index].PageAllocated != LoaderFree)
		{
			return FALSE;
		}
	}

	return TRUE;
}
