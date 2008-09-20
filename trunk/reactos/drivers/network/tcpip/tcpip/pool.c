/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        tcpip/pool.c
 * PURPOSE:     Routines for controling pools
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */

#include "precomp.h"


PVOID PoolAllocateBuffer(
    ULONG Size)
/*
 * FUNCTION: Returns a buffer from the free buffer pool
 * RETURNS:
 *     Pointer to buffer, NULL if there was not enough
 *     free resources
 */
{
    PVOID Buffer;

    /* FIXME: Get buffer from a free buffer pool with enough room */

    Buffer = ExAllocatePool(NonPagedPool, Size);

    RtlZeroMemory(Buffer, Size);

    TI_DbgPrint(DEBUG_MEMORY, ("Allocated (%i) bytes at (0x%X).\n", Size, Buffer));

    return Buffer;
}


VOID PoolFreeBuffer(
    PVOID Buffer)
/*
 * FUNCTION: Returns a buffer to the free buffer pool
 * ARGUMENTS:
 *     Buffer = Buffer to return to free buffer pool
 */
{
    /* FIXME: Put buffer in free buffer pool */

    TI_DbgPrint(DEBUG_MEMORY, ("Freeing buffer at (0x%X).\n", Buffer));

    ExFreePool(Buffer);
}

PVOID TcpipAllocateFromNPagedLookasideList( PNPAGED_LOOKASIDE_LIST List ) {
    PVOID Buffer;

    Buffer = ExAllocateFromNPagedLookasideList( List );

    TI_DbgPrint(DEBUG_MEMORY, ("Allocated from Nonpaged Lookaside List at (0x%X).\n", Buffer));

    return Buffer;
}

VOID TcpipFreeToNPagedLookasideList( PNPAGED_LOOKASIDE_LIST List,
				     PVOID Thing ) {

    TI_DbgPrint(DEBUG_MEMORY, ("Freeing buffer (0x%X) to Nonpaged Lookaside List.\n", Thing));

    ExFreeToNPagedLookasideList( List, Thing );
}

/* EOF */
