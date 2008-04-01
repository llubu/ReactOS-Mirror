/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ob/sdcache.c
 * PURPOSE:         No purpose listed.
 *
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

#define SD_CACHE_ENTRIES 0x100
OB_SD_CACHE_LIST ObsSecurityDescriptorCache[SD_CACHE_ENTRIES];

ULONGLONG Cycles;
ULONG TimeDelta;

#define ObpSdCacheBeginPerfCount() \
    Cycles = __rdtsc();

#define ObpSdCacheEndPerfCount() \
    TimeDelta += __rdtsc() - Cycles;


/* PRIVATE FUNCTIONS **********************************************************/

VOID
FORCEINLINE
ObpSdAcquireLock(IN POB_SD_CACHE_LIST CacheEntry)
{
    /* Acquire the lock */
    KeEnterCriticalRegion();
    ExAcquirePushLockExclusive(&CacheEntry->PushLock);
}

VOID
FORCEINLINE
ObpSdReleaseLock(IN POB_SD_CACHE_LIST CacheEntry)
{
    /* Release the lock */
    ExReleasePushLockExclusive(&CacheEntry->PushLock);
    KeLeaveCriticalRegion();
}

VOID
FORCEINLINE
ObpSdAcquireLockShared(IN POB_SD_CACHE_LIST CacheEntry)
{
    /* Acquire the lock */
    KeEnterCriticalRegion();
    ExAcquirePushLockShared(&CacheEntry->PushLock);
}

VOID
FORCEINLINE
ObpSdReleaseLockShared(IN POB_SD_CACHE_LIST CacheEntry)
{
    /* Release the lock */
    ExReleasePushLock(&CacheEntry->PushLock);
    KeLeaveCriticalRegion();
}

NTSTATUS
NTAPI
ObpInitSdCache(VOID)
{
    ULONG i;

    /* Loop each cache entry */
    for (i = 0; i < SD_CACHE_ENTRIES; i++)
    {
        /* Initialize the lock and the list */
        InitializeListHead(&ObsSecurityDescriptorCache[i].Head);
        ExInitializePushLock((PULONG_PTR)&ObsSecurityDescriptorCache[i].PushLock);
    }

    /* Return success */
    return STATUS_SUCCESS;
}

ULONG
NTAPI
ObpHash(IN PVOID Buffer,
        IN ULONG Length)
{
    PULONG p, pp;
    PUCHAR pb, ppb;
    ULONG Hash = 0;
    
    /* Setup aligned and byte buffers */
    p = Buffer;
    ppb = (PUCHAR)((ULONG_PTR)Buffer + Length);
    pp = (PULONG)ALIGN_DOWN(p + Length, ULONG);

    /* Loop aligned data */
    while (p < pp)
    {
        /* XOR-rotate */
        Hash ^= *p++;
        Hash = _rotl(Hash, 3);
    }
    
    /* Loop non-aligned data */
    pb = (PUCHAR)p;
    while (pb < ppb)
    {
        /* XOR-rotate */
        Hash ^= *pb++;
        Hash = _rotl(Hash, 3);
    }

    /* Return the hash */
    return Hash;
}

ULONG
NTAPI
ObpHashSecurityDescriptor(IN PSECURITY_DESCRIPTOR SecurityDescriptor,
                          IN ULONG Length)
{
    /* Just hash the entire SD */
    return ObpHash(SecurityDescriptor, Length);
}

PSECURITY_DESCRIPTOR_HEADER
NTAPI
ObpCreateCacheEntry(IN PSECURITY_DESCRIPTOR SecurityDescriptor,
                    IN ULONG Length,
                    IN ULONG FullHash,
                    IN ULONG RefCount)
{
    ULONG CacheSize;
    PSECURITY_DESCRIPTOR_HEADER SdHeader;
    ASSERT(Length == RtlLengthSecurityDescriptor(SecurityDescriptor));
    
    /* Calculate the memory we'll need to allocate and allocate it */
    CacheSize = Length + (sizeof(SECURITY_DESCRIPTOR_HEADER) - sizeof(QUAD));
    SdHeader = ExAllocatePoolWithTag(PagedPool, CacheSize, TAG('O', 'b', 'S', 'c'));
    if (!SdHeader) return NULL;
    
    /* Setup the header */
    SdHeader->RefCount = RefCount;
    SdHeader->FullHash = FullHash;
    
    /* Copy the descriptor */
    RtlCopyMemory(&SdHeader->SecurityDescriptor, SecurityDescriptor, Length);
    
    /* Return it */
    return SdHeader;
}

BOOLEAN
NTAPI
ObpCompareSecurityDescriptors(IN PSECURITY_DESCRIPTOR Sd1,
                              IN ULONG Length1,
                              IN PSECURITY_DESCRIPTOR Sd2)
{
    ULONG Length2;
    ASSERT(Length1 == RtlLengthSecurityDescriptor(Sd1));
    
    /* Get the length of the second SD */
    Length2 = RtlLengthSecurityDescriptor(Sd2);
    
    /* Compare lengths */
    if (Length1 != Length2) return FALSE;
    
    /* Compare contents */
    return RtlEqualMemory(Sd1, Sd2, Length1);
}

PVOID
NTAPI
ObpDestroySecurityDescriptorHeader(IN PSECURITY_DESCRIPTOR_HEADER SdHeader)
{
    ASSERT(SdHeader->RefCount == 0);

    /* Just unlink the SD and return it back to the caller */
    RemoveEntryList(&SdHeader->Link);
    return SdHeader;
}

PSECURITY_DESCRIPTOR
NTAPI
ObpReferenceSecurityDescriptor(IN POBJECT_HEADER ObjectHeader)
{
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    PSECURITY_DESCRIPTOR_HEADER SdHeader;
    ObpSdCacheBeginPerfCount();
    
    /* Get the SD */
    SecurityDescriptor = ObjectHeader->SecurityDescriptor;
    if (!SecurityDescriptor)
    {
        /* No SD, nothing to do */
        ObpSdCacheEndPerfCount();
        return NULL;
    }
    
    /* Lock the object */
    ObpAcquireObjectLockShared(ObjectHeader);
    
    /* Get the object header */
    SdHeader = ObpGetHeaderForSd(SecurityDescriptor);
    
    /* Do the reference */
    InterlockedIncrement((PLONG)&SdHeader->RefCount);
    
    /* Release the lock and return */
    ObpReleaseObjectLock(ObjectHeader);
    ObpSdCacheEndPerfCount();
    return SecurityDescriptor;
}

/* PUBLIC FUNCTIONS ***********************************************************/

/*++
 * @name ObReferenceSecurityDescriptor
 * @implemented NT5.2
 *
 *     The ObReferenceSecurityDescriptor routine <FILLMEIN>
 *
 * @param SecurityDescriptor
 *        <FILLMEIN>
 *
 * @param Count
 *        <FILLMEIN>
 *
 * @return STATUS_SUCCESS or appropriate error value.
 *
 * @remarks None.
 *
 *--*/
VOID
NTAPI
ObReferenceSecurityDescriptor(IN PSECURITY_DESCRIPTOR SecurityDescriptor,
                              IN ULONG Count)
{
    PSECURITY_DESCRIPTOR_HEADER SdHeader;
    ObpSdCacheBeginPerfCount();

    /* Get the header */
    SdHeader = ObpGetHeaderForSd(SecurityDescriptor);
    
    /* Do the references */
    InterlockedExchangeAdd((PLONG)&SdHeader->RefCount, Count);
    ObpSdCacheEndPerfCount();
}

/*++
 * @name ObDereferenceSecurityDescriptor
 * @implemented NT5.2
 *
 *     The ObDereferenceSecurityDescriptor routine <FILLMEIN>
 *
 * @param SecurityDescriptor
 *        <FILLMEIN>
 *
 * @param Count
 *        <FILLMEIN>
 *
 * @return STATUS_SUCCESS or appropriate error value.
 *
 * @remarks None.
 *
 *--*/
VOID
NTAPI
ObDereferenceSecurityDescriptor(IN PSECURITY_DESCRIPTOR SecurityDescriptor,
                                IN ULONG Count)
{
    PSECURITY_DESCRIPTOR_HEADER SdHeader;
    LONG OldValue, NewValue;
    ULONG Index;
    POB_SD_CACHE_LIST CacheEntry;
    ObpSdCacheBeginPerfCount();
    
    /* Get the header */
    SdHeader = ObpGetHeaderForSd(SecurityDescriptor);
    
    /* Get the current reference count */
    OldValue = SdHeader->RefCount;
    
    /* Check if the caller is destroying this SD -- we need the lock for that */
    while (OldValue != Count)
    {
        /* He isn't, we can just try to derefeference atomically */
        NewValue = InterlockedCompareExchange((PLONG)&SdHeader->RefCount,
                                              OldValue - Count,
                                              OldValue);
        if (NewValue == OldValue) ObpSdCacheEndPerfCount(); return;
        
        /* Try again */
        OldValue = NewValue;
    }
    
    /* At this point, we need the lock, so choose an entry */
    Index = SdHeader->FullHash % SD_CACHE_ENTRIES;
    CacheEntry = &ObsSecurityDescriptorCache[Index];
    
    /* Acquire the lock for it */
    ObpSdAcquireLock(CacheEntry);
    ASSERT(SdHeader->RefCount != 0);
    
    /* Now do the dereference */
    if (InterlockedExchangeAdd((PLONG)&SdHeader->RefCount, -(LONG)Count) == Count)
    {
        /* We're down to zero -- destroy the header */
        SdHeader = ObpDestroySecurityDescriptorHeader(SdHeader);
        
        /* Release the lock */
        ObpSdReleaseLock(CacheEntry);
        
        /* Free the header */
        ExFreePool(SdHeader);
    }
    else
    {
        /* Just release the lock */
        ObpSdReleaseLock(CacheEntry);
    }
    
    ObpSdCacheEndPerfCount();
}

/*++
* @name ObLogSecurityDescriptor
* @implemented NT5.2
*
*     The ObLogSecurityDescriptor routine <FILLMEIN>
*
* @param InputSecurityDescriptor
*        <FILLMEIN>
*
* @param OutputSecurityDescriptor
*        <FILLMEIN>
*
* @param RefBias
*        <FILLMEIN>
*
* @return STATUS_SUCCESS or appropriate error value.
*
* @remarks None.
*
*--*/
NTSTATUS
NTAPI
ObLogSecurityDescriptor(IN PSECURITY_DESCRIPTOR InputSecurityDescriptor,
                        OUT PSECURITY_DESCRIPTOR *OutputSecurityDescriptor,
                        IN ULONG RefBias)
{
    PSECURITY_DESCRIPTOR_HEADER SdHeader = NULL, NewHeader  = NULL;
    ULONG Length, Hash, Index;
    POB_SD_CACHE_LIST CacheEntry;
    BOOLEAN Result;
    PLIST_ENTRY NextEntry;
    ObpSdCacheBeginPerfCount();

    /* Get the length */
    Length = RtlLengthSecurityDescriptor(InputSecurityDescriptor);
    
    /* Get the hash */
    Hash = ObpHashSecurityDescriptor(InputSecurityDescriptor, Length);
    
    /* Now select the appropriate cache entry */
    Index = Hash % SD_CACHE_ENTRIES;
    CacheEntry = &ObsSecurityDescriptorCache[Index];
    
    /* Lock it shared */
    ObpSdAcquireLockShared(CacheEntry);
    
    /* Start our search */
    while (TRUE)
    {
        /* Reset result found */
        Result = FALSE;
        
        /* Loop the hash list */
        NextEntry = CacheEntry->Head.Flink;
        while (NextEntry != &CacheEntry->Head)
        {
            /* Get the header */
            SdHeader = ObpGetHeaderForEntry(NextEntry);
            
            /* Our hashes are ordered, so quickly check if we should stop now */
            if (SdHeader->FullHash > Hash) break;
            
            /* We survived the quick hash check, now check for equalness */
            if (SdHeader->FullHash == Hash)
            {
                /* Hashes match, now compare descriptors */
                Result = ObpCompareSecurityDescriptors(InputSecurityDescriptor,
                                                       Length,
                                                       &SdHeader->SecurityDescriptor);
                if (Result) break;
            }
            
            /* Go to the next entry */
            NextEntry = NextEntry->Flink;
        }
        
        /* Check if we found anything */
        if (Result)
        {
            /* Increment its reference count */
            InterlockedExchangeAdd((PLONG)&SdHeader->RefCount, RefBias);
                                              
            /* Release the lock */
            ObpSdReleaseLockShared(CacheEntry);
            
            /* Return the descriptor */
            *OutputSecurityDescriptor = &SdHeader->SecurityDescriptor;
            
            /* Free anything that we may have had to create */
            if (NewHeader) ExFreePool(NewHeader);
            ObpSdCacheEndPerfCount();
            return STATUS_SUCCESS;
        }
        
        /* Check if we got here, and didn't create a descriptor yet */
        if (!NewHeader)
        {
            /* Release the lock */
            ObpSdReleaseLockShared(CacheEntry);
            
            /* This should be our first time in the loop, create it */
            NewHeader = ObpCreateCacheEntry(InputSecurityDescriptor,
                                            Length,
                                            Hash,
                                            RefBias);
            if (!NewHeader) return STATUS_INSUFFICIENT_RESOURCES;
            
            /* Now acquire the exclusive lock and we should hit the right path */
            ObpSdAcquireLock(CacheEntry);
        }
        else
        {
            /* We have inserted the SD, we're fine now */
            break;
        }
    }
    
    /* Okay, now let's do the insert, we should have the exclusive lock */
    InsertTailList(NextEntry, &NewHeader->Link);
    
    /* Release the lock */
    ObpSdReleaseLock(CacheEntry);
    
    /* Return the SD*/
    *OutputSecurityDescriptor = &NewHeader->SecurityDescriptor;
    ObpSdCacheEndPerfCount();
    return STATUS_SUCCESS;
}

/* EOF */
