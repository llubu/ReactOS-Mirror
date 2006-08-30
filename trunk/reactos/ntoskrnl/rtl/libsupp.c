/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/rtl/libsupp.c
 * PURPOSE:         RTL Support Routines
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 *                  Gunnar Dalsnes
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

extern ULONG NtGlobalFlag;

/* FUNCTIONS *****************************************************************/

BOOLEAN
NTAPI
RtlpCheckForActiveDebugger(BOOLEAN Type)
{
    /* This check is meaningless in kernel-mode */
    return Type;
}

BOOLEAN
NTAPI
RtlpSetInDbgPrint(IN BOOLEAN NewValue)
{
    /* This check is meaningless in kernel-mode */
    return FALSE;
}

KPROCESSOR_MODE
STDCALL
RtlpGetMode()
{
   return KernelMode;
}

PVOID
STDCALL
RtlpAllocateMemory(UINT Bytes,
                   ULONG Tag)
{
    return ExAllocatePoolWithTag(PagedPool,
                                 (SIZE_T)Bytes,
                                 Tag);
}


VOID
STDCALL
RtlpFreeMemory(PVOID Mem,
               ULONG Tag)
{
    ExFreePoolWithTag(Mem,
                      Tag);
}

/*
 * @implemented
 */
VOID STDCALL
RtlAcquirePebLock(VOID)
{

}

/*
 * @implemented
 */
VOID STDCALL
RtlReleasePebLock(VOID)
{

}

NTSTATUS
STDCALL
LdrShutdownThread(VOID)
{
    return STATUS_SUCCESS;
}


PPEB
STDCALL
RtlpCurrentPeb(VOID)
{
   return ((PEPROCESS)(KeGetCurrentThread()->ApcState.Process))->Peb;
}

NTSTATUS
STDCALL
RtlDeleteHeapLock(
    PRTL_CRITICAL_SECTION CriticalSection)
{
    KEBUGCHECK(0);
    return STATUS_SUCCESS;
}

NTSTATUS
STDCALL
RtlEnterHeapLock(
    PRTL_CRITICAL_SECTION CriticalSection)
{
    KEBUGCHECK(0);
    return STATUS_SUCCESS;
}

NTSTATUS
STDCALL
RtlInitializeHeapLock(
    PRTL_CRITICAL_SECTION CriticalSection)
{
   KEBUGCHECK(0);
   return STATUS_SUCCESS;
}

NTSTATUS
STDCALL
RtlLeaveHeapLock(
    PRTL_CRITICAL_SECTION CriticalSection)
{
    KEBUGCHECK(0);
    return STATUS_SUCCESS;
}

#ifdef DBG
VOID FASTCALL
CHECK_PAGED_CODE_RTL(char *file, int line)
{
  if(KeGetCurrentIrql() > APC_LEVEL)
  {
    DbgPrint("%s:%i: Pagable code called at IRQL > APC_LEVEL (%d)\n", file, line, KeGetCurrentIrql());
    KEBUGCHECK(0);
  }
}
#endif

VOID
NTAPI
RtlpCheckLogException(IN PEXCEPTION_RECORD ExceptionRecord,
                      IN PCONTEXT ContextRecord,
                      IN PVOID ContextData,
                      IN ULONG Size)
{
    /* Check the global flag */
    if (NtGlobalFlag & FLG_ENABLE_EXCEPTION_LOGGING)
    {
        /* FIXME: Log this exception */
    }
}

BOOLEAN
NTAPI
RtlpHandleDpcStackException(IN PEXCEPTION_REGISTRATION_RECORD RegistrationFrame,
                            IN ULONG_PTR RegistrationFrameEnd,
                            IN OUT PULONG_PTR StackLow,
                            IN OUT PULONG_PTR StackHigh)
{
    PKPRCB Prcb;
    ULONG_PTR DpcStack;

    /* Check if we are at DISPATCH or higher */
    if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
    {
        /* Get the PRCB and DPC Stack */
        Prcb = KeGetCurrentPrcb();
        DpcStack = (ULONG_PTR)Prcb->DpcStack;

        /* Check if we are in a DPC and the stack matches */
        if ((Prcb->DpcRoutineActive) &&
            (RegistrationFrameEnd <= DpcStack) &&
            ((ULONG_PTR)RegistrationFrame >= DpcStack - 4096))
        {
            /* Update the limits to the DPC Stack's */
            *StackHigh = DpcStack;
            *StackLow = DpcStack - 4096;
            return TRUE;
        }
    }

    /* Not in DPC stack */
    return FALSE;
}

BOOLEAN
NTAPI
RtlpCaptureStackLimits(IN ULONG_PTR Ebp,
                       IN ULONG_PTR *StackBegin,
                       IN ULONG_PTR *StackEnd)
{
    PKTHREAD Thread = KeGetCurrentThread();

    /* FIXME: Super native implementation */

    /* FIXME: ROS HACK */
    if (!Thread) return FALSE;

    /* Start with defaults */
    *StackBegin = Thread->StackLimit;
    *StackEnd = (ULONG_PTR)Thread->StackBase;

    /* Check if we seem to be on the DPC stack */
    if ((*StackBegin > Ebp) || (Ebp > *StackEnd))
    {
        /* FIXME: TODO */
        ASSERT(FALSE);
    }

    /* Return success */
    return TRUE;
}

/* RTL Atom Tables ************************************************************/

NTSTATUS
RtlpInitAtomTableLock(PRTL_ATOM_TABLE AtomTable)
{
   ExInitializeFastMutex(&AtomTable->FastMutex);

   return STATUS_SUCCESS;
}


VOID
RtlpDestroyAtomTableLock(PRTL_ATOM_TABLE AtomTable)
{
}


BOOLEAN
RtlpLockAtomTable(PRTL_ATOM_TABLE AtomTable)
{
   ExAcquireFastMutex(&AtomTable->FastMutex);
   return TRUE;
}

VOID
RtlpUnlockAtomTable(PRTL_ATOM_TABLE AtomTable)
{
   ExReleaseFastMutex(&AtomTable->FastMutex);
}

BOOLEAN
RtlpCreateAtomHandleTable(PRTL_ATOM_TABLE AtomTable)
{
   AtomTable->ExHandleTable = ExCreateHandleTable(NULL);
   return (AtomTable->ExHandleTable != NULL);
}

VOID
RtlpDestroyAtomHandleTable(PRTL_ATOM_TABLE AtomTable)
{
   if (AtomTable->ExHandleTable)
   {
      ExSweepHandleTable(AtomTable->ExHandleTable,
                         NULL,
                         NULL);
      ExDestroyHandleTable(AtomTable->ExHandleTable);
      AtomTable->ExHandleTable = NULL;
   }
}

PRTL_ATOM_TABLE
RtlpAllocAtomTable(ULONG Size)
{
   PRTL_ATOM_TABLE Table = ExAllocatePool(NonPagedPool,
                                          Size);
   if (Table != NULL)
   {
      RtlZeroMemory(Table,
                    Size);
   }
   
   return Table;
}

VOID
RtlpFreeAtomTable(PRTL_ATOM_TABLE AtomTable)
{
   ExFreePool(AtomTable);
}

PRTL_ATOM_TABLE_ENTRY
RtlpAllocAtomTableEntry(ULONG Size)
{
   PRTL_ATOM_TABLE_ENTRY Entry = ExAllocatePool(NonPagedPool,
                                                Size);
   if (Entry != NULL)
   {
      RtlZeroMemory(Entry,
                    Size);
   }

   return Entry;
}

VOID
RtlpFreeAtomTableEntry(PRTL_ATOM_TABLE_ENTRY Entry)
{
   ExFreePool(Entry);
}

VOID
RtlpFreeAtomHandle(PRTL_ATOM_TABLE AtomTable, PRTL_ATOM_TABLE_ENTRY Entry)
{
   ExDestroyHandle(AtomTable->ExHandleTable,
                   (HANDLE)((ULONG_PTR)Entry->HandleIndex << 2));
}

BOOLEAN
RtlpCreateAtomHandle(PRTL_ATOM_TABLE AtomTable, PRTL_ATOM_TABLE_ENTRY Entry)
{
   HANDLE_TABLE_ENTRY ExEntry;
   HANDLE Handle;
   USHORT HandleIndex;
   
   ExEntry.Object = Entry;
   ExEntry.GrantedAccess = 0x1; /* FIXME - valid handle */
   
   Handle = ExCreateHandle(AtomTable->ExHandleTable,
                                &ExEntry);
   if (Handle != NULL)
   {
      HandleIndex = (USHORT)((ULONG_PTR)Handle >> 2);
      /* FIXME - Handle Indexes >= 0xC000 ?! */
      if ((ULONG_PTR)HandleIndex >> 2 < 0xC000)
      {
         Entry->HandleIndex = HandleIndex;
         Entry->Atom = 0xC000 + HandleIndex;
         
         return TRUE;
      }
      else
         ExDestroyHandle(AtomTable->ExHandleTable,
                         Handle);
   }
   
   return FALSE;
}

PRTL_ATOM_TABLE_ENTRY
RtlpGetAtomEntry(PRTL_ATOM_TABLE AtomTable, ULONG Index)
{
   PHANDLE_TABLE_ENTRY ExEntry;
   PRTL_ATOM_TABLE_ENTRY Entry = NULL;
   
   /* NOTE: There's no need to explicitly enter a critical region because it's
            guaranteed that we're in a critical region right now (as we hold
            the atom table lock) */
   
   ExEntry = ExMapHandleToPointer(AtomTable->ExHandleTable,
                                  (HANDLE)((ULONG_PTR)Index << 2));
   if (ExEntry != NULL)
   {
      Entry = ExEntry->Object;
      
      ExUnlockHandleTableEntry(AtomTable->ExHandleTable,
                               ExEntry);
   }
   
   return Entry;
}

/* FIXME - RtlpCreateUnicodeString is obsolete and should be removed ASAP! */
BOOLEAN FASTCALL
RtlpCreateUnicodeString(
   IN OUT PUNICODE_STRING UniDest,
   IN PCWSTR  Source,
   IN POOL_TYPE PoolType)
{
   ULONG Length;

   Length = (wcslen (Source) + 1) * sizeof(WCHAR);
   UniDest->Buffer = ExAllocatePoolWithTag(PoolType, Length, TAG('U', 'S', 'T', 'R'));
   if (UniDest->Buffer == NULL)
      return FALSE;

   RtlCopyMemory (UniDest->Buffer,
                  Source,
                  Length);

   UniDest->MaximumLength = Length;
   UniDest->Length = Length - sizeof (WCHAR);

   return TRUE;
}

/*
 * Ldr Resource support code
 */

IMAGE_RESOURCE_DIRECTORY *find_entry_by_name( IMAGE_RESOURCE_DIRECTORY *dir,
                                              LPCWSTR name, void *root,
                                              int want_dir );
IMAGE_RESOURCE_DIRECTORY *find_entry_by_id( IMAGE_RESOURCE_DIRECTORY *dir,
                                            WORD id, void *root, int want_dir );
IMAGE_RESOURCE_DIRECTORY *find_first_entry( IMAGE_RESOURCE_DIRECTORY *dir,
                                            void *root, int want_dir );

/**********************************************************************
 *  find_entry
 *
 * Find a resource entry
 */
NTSTATUS find_entry( PVOID BaseAddress, LDR_RESOURCE_INFO *info,
                     ULONG level, void **ret, int want_dir )
{
    ULONG size;
    void *root;
    IMAGE_RESOURCE_DIRECTORY *resdirptr;

    root = RtlImageDirectoryEntryToData( BaseAddress, TRUE, IMAGE_DIRECTORY_ENTRY_RESOURCE, &size );
    if (!root) return STATUS_RESOURCE_DATA_NOT_FOUND;
    resdirptr = root;

    if (!level--) goto done;
    if (!(*ret = find_entry_by_name( resdirptr, (LPCWSTR)info->Type, root, want_dir || level )))
        return STATUS_RESOURCE_TYPE_NOT_FOUND;
    if (!level--) return STATUS_SUCCESS;

    resdirptr = *ret;
    if (!(*ret = find_entry_by_name( resdirptr, (LPCWSTR)info->Name, root, want_dir || level )))
        return STATUS_RESOURCE_NAME_NOT_FOUND;
    if (!level--) return STATUS_SUCCESS;
    if (level) return STATUS_INVALID_PARAMETER;  /* level > 3 */

    resdirptr = *ret;

    if ((*ret = find_first_entry( resdirptr, root, want_dir ))) return STATUS_SUCCESS;

    return STATUS_RESOURCE_DATA_NOT_FOUND;

done:
    *ret = resdirptr;
    return STATUS_SUCCESS;
}


/* EOF */
