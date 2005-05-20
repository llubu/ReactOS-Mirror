/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/ppool.c
 * PURPOSE:         Implements the paged pool
 *
 * PROGRAMMERS:     David Welch (welch@mcmail.com)
 *                  Royce Mitchell III
 */

/* INCLUDES *****************************************************************/

#ifdef PPOOL_UMODE_TEST
#include "ppool_umode.h"
#else//PPOOL_UMODE_TEST
#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>
#endif//PPOOL_UMODE_TEST

#undef ASSERT
#define ASSERT(x) if (!(x)) {DbgPrint("Assertion "#x" failed at %s:%d\n", __FILE__,__LINE__); KeBugCheck(0); }

// enable "magic"
//#define R_MAGIC
#define R_MUTEX FAST_MUTEX
#define R_ACQUIRE_MUTEX(pool) /*DPRINT1("Acquiring PPool Mutex\n");*/ ExAcquireFastMutex(&pool->Mutex)
#define R_RELEASE_MUTEX(pool) /*DPRINT1("Releasing PPool Mutex\n");*/ ExReleaseFastMutex(&pool->Mutex)
#define R_PRINT_ADDRESS(addr) KeRosPrintAddress(addr)
#define R_PANIC() KeBugCheck(0)
#define R_DEBUG DbgPrint
#define R_EXTRA_STACK_UP 2
#define R_GET_STACK_FRAMES(ptr,cnt) KeRosGetStackFrames(ptr,cnt)

#include "rpoolmgr.h"

/* GLOBALS *******************************************************************/

PVOID MmPagedPoolBase;
ULONG MmPagedPoolSize;
ULONG MmTotalPagedPoolQuota = 0; // TODO FIXME commented out until we use it
static PR_POOL MmPagedPool = NULL;

/* FUNCTIONS *****************************************************************/

VOID INIT_FUNCTION
MmInitializePagedPool()
{
	/*
	 * We are still at a high IRQL level at this point so explicitly commit
	 * the first page of the paged pool before writing the first block header.
	 */
	MmCommitPagedPoolAddress ( (PVOID)MmPagedPoolBase, FALSE );

	MmPagedPool = RPoolInit ( MmPagedPoolBase,
		MmPagedPoolSize,
		MM_POOL_ALIGNMENT,
		MM_CACHE_LINE_SIZE,
		PAGE_SIZE );

	ExInitializeFastMutex(&MmPagedPool->Mutex);
}

/**********************************************************************
 * NAME       INTERNAL
 * ExAllocatePagedPoolWithTag@12
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 */
PVOID STDCALL
ExAllocatePagedPoolWithTag (IN POOL_TYPE PoolType,
                            IN ULONG  NumberOfBytes,
                            IN ULONG  Tag)
{
	int align;

	if ( NumberOfBytes >= PAGE_SIZE )
		align = 2;
	else if ( PoolType == PagedPoolCacheAligned )
		align = 1;
	else
		align = 0;

	ASSERT_IRQL(APC_LEVEL);

	return RPoolAlloc ( MmPagedPool, NumberOfBytes, Tag, align );
}

VOID STDCALL
ExFreePagedPool(IN PVOID Block)
{
	ASSERT_IRQL(APC_LEVEL);
	RPoolFree ( MmPagedPool, Block );
}

VOID STDCALL
ExRosDumpPagedPoolByTag ( ULONG Tag )
{
	// TODO FIXME - should we ASSERT_IRQL?
	RPoolDumpByTag ( MmPagedPool, Tag );
}

ULONG STDCALL
ExRosQueryPagedPoolTag ( PVOID Addr )
{
	// TODO FIXME - should we ASSERT_IRQL?
	return RPoolQueryTag ( Addr );
}

#ifdef PPOOL_UMODE_TEST

PVOID TestAlloc ( ULONG Bytes )
{
	PVOID ret;

	//printf ( "Allocating block: " ); RPoolStats ( MmPagedPool );
	//RPoolRedZoneCheck ( MmPagedPool, __FILE__, __LINE__ );

	ret = ExAllocatePagedPoolWithTag ( PagedPool, Bytes, 0 );

	//printf ( "Block %x allocated: ", ret ); RPoolStats ( MmPagedPool );
	//RPoolRedZoneCheck ( MmPagedPool, __FILE__, __LINE__ );

	return ret;
}

void TestFree ( PVOID ptr )
{
	//printf ( "Freeing block %x: ", ptr ); RPoolStats ( MmPagedPool );
	//RPoolRedZoneCheck ( MmPagedPool, __FILE__, __LINE__ );
	ExFreePagedPool(ptr);
	//printf ( "Block %x freed: ", ptr ); RPoolStats ( MmPagedPool );
	//RPoolRedZoneCheck ( MmPagedPool, __FILE__, __LINE__ );
}

int main()
{
#define COUNT 100
	int i, j;
	char* keepers[COUNT];
	char* trash[COUNT];
	int AllocSize[] = { 15, 31, 63, 127, 255, 511, 1023, 2047 };
	const int ALLOCS = sizeof(AllocSize) / sizeof(0[AllocSize]);
	DWORD dwStart;

	MmPagedPoolSize = 1*1024*1024;
	MmPagedPoolBase = malloc ( MmPagedPoolSize );
	MmInitializePagedPool();

	dwStart = GetTickCount();

	printf ( "test #1 phase #1\n" );
	for ( i = 0; i < COUNT; i++ )
	{
		//printf ( "keeper %i) ", i );
		keepers[i] = TestAlloc ( AllocSize[i%ALLOCS] );
		if ( !keepers[i] ) printf ( "allocation failed\n" );
		//printf ( "trash %i) ", i );
		trash[i] = TestAlloc ( AllocSize[i%ALLOCS] );
		if ( !trash[i] ) printf ( "allocation failed\n" );
	}

	printf ( "test #1 phase #2\n" );
	for ( i = 0; i < COUNT; i++ )
	{
		if ( i == 6 )
			i = i;
		//printf ( "%i) ", i );
		TestFree ( trash[i] );
	}

	printf ( "test #1 phase #3\n" );
	for ( i = 0; i < 4; i++ )
	{
		//printf ( "%i) ", i );
		keepers[i] = TestAlloc ( 4096 );
		if ( !keepers[i] ) printf ( "allocation failed\n" );
	}

	printf ( "test #1 phase #4\n" );
	for ( i = 0; i < 4; i++ )
	{
		//printf ( "%i) ", i );
		TestFree ( keepers[i] );
	}

	printf ( "test #1 phase #5\n" );
	srand(1);
	for ( i = 0; i < COUNT; i++ )
	{
		//printf ( "%i) ", i );
		trash[i] = TestAlloc ( rand()%1024+1 );
		if ( !trash[i] ) printf ( "allocation failed\n" );
	}
	printf ( "test #1 phase #6\n" );
	for ( i = 0; i < 10000; i++ )
	{
		TestFree ( trash[i%COUNT] );
		trash[i%COUNT] = TestAlloc ( rand()%1024+1 );
		if ( !trash[i%COUNT] ) printf ( "allocation failed\n" );
	}
	printf ( "test #1 phase #7\n" );
	j = 0;
	for ( i = 0; i < COUNT; i++ )
	{
		if ( trash[i] )
		{
			TestFree ( trash[i] );
			++j;
		}
	}
	printf ( "test #1 phase #8 ( freed %i of %i trash )\n", j, COUNT );
	if ( !TestAlloc ( 2048 ) )
		printf ( "Couldn't allocate 2048 bytes after freeing up a whole bunch of blocks\n" );

	free ( MmPagedPoolBase );

	printf ( "test time: %lu\n", GetTickCount() - dwStart );

	printf ( "test #2\n" );

	MmPagedPoolSize = 1024;
	MmPagedPoolBase = malloc ( MmPagedPoolSize );
	MmInitializePagedPool();

	TestAlloc ( 512 );
	i = RPoolLargestAllocPossible ( MmPagedPool, 0 );
	if ( !TestAlloc ( i ) )
	{
		printf ( "allocating last available block failed\n" );
	}

	free ( MmPagedPoolBase );

	printf ( "done!\n" );
	return 0;
}
#endif//PPOOL_UMODE_TEST

/* EOF */
