/* $Id: pool.c,v 1.26 2003/12/30 18:52:05 fireball Exp $
 * 
 * COPYRIGHT:    See COPYING in the top level directory
 * PROJECT:      ReactOS kernel
 * FILE:         ntoskrnl/mm/pool.c
 * PURPOSE:      Implements the kernel memory pool
 * PROGRAMMER:   David Welch (welch@mcmail.com)
 */

/* INCLUDES ****************************************************************/

#include <ddk/ntddk.h>
#include <reactos/bugcodes.h>
#include <internal/ntoskrnl.h>
#include <internal/pool.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *****************************************************************/

#define TAG_NONE (ULONG)(('N'<<0) + ('o'<<8) + ('n'<<16) + ('e'<<24))

/* FUNCTIONS ***************************************************************/

#if defined(__GNUC__)
PVOID STDCALL STATIC
#else
STATIC PVOID STDCALL
#endif
EiAllocatePool(POOL_TYPE PoolType,
	       ULONG NumberOfBytes,
	       ULONG Tag,
	       PVOID Caller)
{
   PVOID Block;
  
   
   switch(PoolType)
     {
      case NonPagedPool:
      case NonPagedPoolMustSucceed:
      case NonPagedPoolCacheAligned:
      case NonPagedPoolCacheAlignedMustS:
	Block = 
	  ExAllocateNonPagedPoolWithTag(PoolType,
					NumberOfBytes,
					Tag,
					Caller);
	break;
	
      case PagedPool:
      case PagedPoolCacheAligned:
	Block = ExAllocatePagedPoolWithTag(PoolType,NumberOfBytes,Tag);
	break;
	
      default:
	return(NULL);
     };
   
   if ((PoolType==NonPagedPoolMustSucceed || 
	PoolType==NonPagedPoolCacheAlignedMustS) && Block==NULL)     
     {
	KEBUGCHECK(MUST_SUCCEED_POOL_EMPTY);
     }
   return(Block);
}

/*
 * @implemented
 */
PVOID STDCALL
ExAllocatePool (POOL_TYPE PoolType, ULONG NumberOfBytes)
/*
 * FUNCTION: Allocates pool memory of a specified type and returns a pointer
 * to the allocated block. This routine is used for general purpose allocation
 * of memory
 * ARGUMENTS:
 *        PoolType
 *               Specifies the type of memory to allocate which can be one
 *               of the following:
 *  
 *               NonPagedPool
 *               NonPagedPoolMustSucceed
 *               NonPagedPoolCacheAligned
 *               NonPagedPoolCacheAlignedMustS
 *               PagedPool
 *               PagedPoolCacheAligned
 *        
 *        NumberOfBytes
 *               Specifies the number of bytes to allocate
 * RETURNS: The allocated block on success
 *          NULL on failure
 */
{
   PVOID Block;
#if defined(__GNUC__)
   Block = EiAllocatePool(PoolType,
			  NumberOfBytes,
			  TAG_NONE,
			  (PVOID)__builtin_return_address(0));
#elif defined(_MSC_VER)
   Block = EiAllocatePool(PoolType,
			  NumberOfBytes,
			  TAG_NONE,
			  &ExAllocatePool);
#else
#error Unknown compiler
#endif
   return(Block);
}


/*
 * @implemented
 */
PVOID STDCALL
ExAllocatePoolWithTag (ULONG PoolType, ULONG NumberOfBytes, ULONG Tag)
{
   PVOID Block;
#if defined(__GNUC__)
   Block = EiAllocatePool(PoolType,
			  NumberOfBytes,
			  Tag,
			  (PVOID)__builtin_return_address(0));
#elif defined(_MSC_VER)
   Block = EiAllocatePool(PoolType,
			  NumberOfBytes,
			  Tag,
			  &ExAllocatePoolWithTag);
#else
#error Unknown compiler
#endif
   return(Block);
}


/*
 * @implemented
 */
PVOID STDCALL
ExAllocatePoolWithQuota (POOL_TYPE PoolType, ULONG NumberOfBytes)
{
  return(ExAllocatePoolWithQuotaTag(PoolType, NumberOfBytes, TAG_NONE));
}


/*
 * @unimplemented
 */
PVOID STDCALL
ExAllocatePoolWithQuotaTag (IN	POOL_TYPE	PoolType,
			    IN	ULONG		NumberOfBytes,
			    IN	ULONG		Tag)
{
#if 0
  PVOID Block;
  Block = EiAllocatePool(PoolType,
			 NumberOfBytes,
			 Tag,
			 (PVOID)__builtin_return_address(0));
  return(Block);
#else
  UNIMPLEMENTED;
  return(NULL);
#endif
}

/*
 * @implemented
 */
VOID STDCALL
ExFreePool(IN PVOID Block)
{
  if (Block >= MmPagedPoolBase && (char*)Block < ((char*)MmPagedPoolBase + MmPagedPoolSize))
    {
      ExFreePagedPool(Block);
    }
  else
    {
      ExFreeNonPagedPool(Block);
    }
}

/*
 * @implemented
 */
VOID STDCALL
ExFreePoolWithTag(IN PVOID Block, IN ULONG Tag)
{
  /* FIXME: Validate the tag */
  ExFreePool(Block);
}

/* EOF */




