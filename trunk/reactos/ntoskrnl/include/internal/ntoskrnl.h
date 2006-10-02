#ifndef __INCLUDE_INTERNAL_NTOSKRNL_H
#define __INCLUDE_INTERNAL_NTOSKRNL_H

/*
 * Use these to place a function in a specific section of the executable
 */
#define PLACE_IN_SECTION(s)	__attribute__((section (s)))
#ifdef __GNUC__
#define INIT_FUNCTION		PLACE_IN_SECTION("init")
#define PAGE_LOCKED_FUNCTION	PLACE_IN_SECTION("pagelk")
#define PAGE_UNLOCKED_FUNCTION	PLACE_IN_SECTION("pagepo")
#else
#define INIT_FUNCTION
#define PAGE_LOCKED_FUNCTION	
#define PAGE_UNLOCKED_FUNCTION	
#endif

#ifdef _NTOSKRNL_

#include "ke.h"
#include "i386/mm.h"
#include "i386/fpu.h"
#include "i386/v86m.h"
#include "ob.h"
#include "mm.h"
#include "ex.h"
#include "ps.h"
#include "cc.h"
#include "io.h"
#include "po.h"
#include "se.h"
#include "ldr.h"
#include "kd.h"
#include "fsrtl.h"
#include "lpc.h"
#include "rtl.h"
#ifdef KDBG
#include "../kdbg/kdb.h"
#endif
#include "dbgk.h"
#include "tag.h"
#include "test.h"
#include "inbv.h"
#include "vdm.h"

#include <pshpack1.h>
/*
 * Defines a descriptor as it appears in the processor tables
 */
typedef struct __DESCRIPTOR
{
  ULONG a;
  ULONG b;
} IDT_DESCRIPTOR, GDT_DESCRIPTOR;

#include <poppack.h>
//extern GDT_DESCRIPTOR KiGdt[256];

/*
 * Initalization functions (called once by main())
 */
VOID MmInitSystem(ULONG Phase, PLOADER_PARAMETER_BLOCK LoaderBlock, ULONG LastKernelAddress);
VOID IoInit(VOID);
VOID IoInit2(BOOLEAN BootLog);
VOID NTAPI IoInit3(VOID);
BOOLEAN NTAPI ObInit(VOID);
VOID PsInit(VOID);
VOID CmInitializeRegistry(VOID);
VOID NTAPI CmInitHives(BOOLEAN SetupBoot);
VOID CmInit2(PCHAR CommandLine);
VOID CmShutdownRegistry(VOID);
BOOLEAN CmImportSystemHive(PCHAR ChunkBase, ULONG ChunkSize);
BOOLEAN CmImportHardwareHive(PCHAR ChunkBase, ULONG ChunkSize);
VOID KdInitSystem(ULONG Reserved, PLOADER_PARAMETER_BLOCK LoaderBlock);

/* FIXME - RtlpCreateUnicodeString is obsolete and should be removed ASAP! */
BOOLEAN FASTCALL
RtlpCreateUnicodeString(
   IN OUT PUNICODE_STRING UniDest,
   IN PCWSTR  Source,
   IN POOL_TYPE PoolType);

VOID
NTAPI
RtlpLogException(IN PEXCEPTION_RECORD ExceptionRecord,
                 IN PCONTEXT ContextRecord,
                 IN PVOID ContextData,
                 IN ULONG Size);

/* FIXME: Interlocked functions that need to be made into a public header */
#ifdef __GNUC__
FORCEINLINE
LONG
InterlockedAnd(IN OUT LONG volatile *Target,
               IN LONG Set)
{
    LONG i;
    LONG j;

    j = *Target;
    do {
        i = j;
        j = InterlockedCompareExchange((PLONG)Target,
                                       i & Set,
                                       i);

    } while (i != j);

    return j;
}

FORCEINLINE
LONG
InterlockedOr(IN OUT LONG volatile *Target,
              IN LONG Set)
{
    LONG i;
    LONG j;

    j = *Target;
    do {
        i = j;
        j = InterlockedCompareExchange((PLONG)Target,
                                       i | Set,
                                       i);

    } while (i != j);

    return j;
}
#endif

/*
 * generic information class probing code
 */

#define ICIF_QUERY               0x1
#define ICIF_SET                 0x2
#define ICIF_QUERY_SIZE_VARIABLE 0x4
#define ICIF_SET_SIZE_VARIABLE   0x8
#define ICIF_SIZE_VARIABLE (ICIF_QUERY_SIZE_VARIABLE | ICIF_SET_SIZE_VARIABLE)

typedef struct _INFORMATION_CLASS_INFO
{
  ULONG RequiredSizeQUERY;
  ULONG RequiredSizeSET;
  ULONG AlignmentSET;
  ULONG AlignmentQUERY;
  ULONG Flags;
} INFORMATION_CLASS_INFO, *PINFORMATION_CLASS_INFO;

#define ICI_SQ_SAME(Type, Alignment, Flags)                                    \
  { Type, Type, Alignment, Alignment, Flags }

#define ICI_SQ(TypeQuery, TypeSet, AlignmentQuery, AlignmentSet, Flags)        \
  { TypeQuery, TypeSet, AlignmentQuery, AlignmentSet, Flags }

//
// TEMPORARY
//
#define IQS_SAME(Type, Alignment, Flags)                                    \
  { sizeof(Type), sizeof(Type), sizeof(Alignment), sizeof(Alignment), Flags }

#define IQS(TypeQuery, TypeSet, AlignmentQuery, AlignmentSet, Flags)        \
  { sizeof(TypeQuery), sizeof(TypeSet), sizeof(AlignmentQuery), sizeof(AlignmentSet), Flags }

FORCEINLINE
NTSTATUS
DefaultSetInfoBufferCheck(ULONG Class,
                          const INFORMATION_CLASS_INFO *ClassList,
                          ULONG ClassListEntries,
                          PVOID Buffer,
                          ULONG BufferLength,
                          KPROCESSOR_MODE PreviousMode)
{
    NTSTATUS Status = STATUS_SUCCESS;

    if (Class >= 0 && Class < ClassListEntries)
    {
        if (!(ClassList[Class].Flags & ICIF_SET))
        {
            Status = STATUS_INVALID_INFO_CLASS;
        }
        else if (ClassList[Class].RequiredSizeSET > 0 &&
                 BufferLength != ClassList[Class].RequiredSizeSET)
        {
            if (!(ClassList[Class].Flags & ICIF_SET_SIZE_VARIABLE))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
            }
        }

        if (NT_SUCCESS(Status))
        {
            if (PreviousMode != KernelMode)
            {
                _SEH_TRY
                {
                    ProbeForRead(Buffer,
                                 BufferLength,
                                 ClassList[Class].AlignmentSET);
                }
                _SEH_HANDLE
                {
                    Status = _SEH_GetExceptionCode();
                }
                _SEH_END;
            }
        }
    }
    else
        Status = STATUS_INVALID_INFO_CLASS;

    return Status;
}

FORCEINLINE
NTSTATUS
DefaultQueryInfoBufferCheck(ULONG Class,
                            const INFORMATION_CLASS_INFO *ClassList,
                            ULONG ClassListEntries,
                            PVOID Buffer,
                            ULONG BufferLength,
                            PULONG ReturnLength,
                            KPROCESSOR_MODE PreviousMode)
{
    NTSTATUS Status = STATUS_SUCCESS;

    if (Class >= 0 && Class < ClassListEntries)
    {
        if (!(ClassList[Class].Flags & ICIF_QUERY))
        {
            Status = STATUS_INVALID_INFO_CLASS;
        }
        else if (ClassList[Class].RequiredSizeQUERY > 0 &&
                 BufferLength != ClassList[Class].RequiredSizeQUERY)
        {
            if (!(ClassList[Class].Flags & ICIF_QUERY_SIZE_VARIABLE))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
            }
        }

        if (NT_SUCCESS(Status))
        {
            if (PreviousMode != KernelMode)
            {
                _SEH_TRY
                {
                    if (Buffer != NULL)
                    {
                        ProbeForWrite(Buffer,
                                      BufferLength,
                                      ClassList[Class].AlignmentQUERY);
                    }

                    if (ReturnLength != NULL)
                    {
                        ProbeForWriteUlong(ReturnLength);
                    }
                }
                _SEH_HANDLE
                {
                    Status = _SEH_GetExceptionCode();
                }
                _SEH_END;
            }
        }
    }
    else
        Status = STATUS_INVALID_INFO_CLASS;

    return Status;
}

/*
 * Use IsPointerOffset to test whether a pointer should be interpreted as an offset
 * or as a pointer
 */
#if defined(_X86_) || defined(_M_AMD64)

/* for x86 and x86-64 the MSB is 1 so we can simply test on that */
#define IsPointerOffset(Ptr) ((LONG_PTR)(Ptr) >= 0)

#elif defined(_IA64_)

/* on Itanium if the 24 most significant bits are set, we're not dealing with
   offsets anymore. */
#define IsPointerOffset(Ptr)  (((ULONG_PTR)(Ptr) & 0xFFFFFF0000000000ULL) == 0)

#else
#error IsPointerOffset() needs to be defined for this architecture
#endif

#endif

C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, SystemCall) == 0x300);
C_ASSERT(FIELD_OFFSET(KTHREAD, InitialStack) == KTHREAD_INITIAL_STACK);
C_ASSERT(FIELD_OFFSET(KTHREAD, Teb) == KTHREAD_TEB);
C_ASSERT(FIELD_OFFSET(KTHREAD, KernelStack) == KTHREAD_KERNEL_STACK);
C_ASSERT(FIELD_OFFSET(KTHREAD, NpxState) == KTHREAD_NPX_STATE);
C_ASSERT(FIELD_OFFSET(KTHREAD, ServiceTable) == KTHREAD_SERVICE_TABLE);
C_ASSERT(FIELD_OFFSET(KTHREAD, PreviousMode) == KTHREAD_PREVIOUS_MODE);
C_ASSERT(FIELD_OFFSET(KTHREAD, TrapFrame) == KTHREAD_TRAP_FRAME);
C_ASSERT(FIELD_OFFSET(KTHREAD, CallbackStack) == KTHREAD_CALLBACK_STACK);
C_ASSERT(FIELD_OFFSET(KTHREAD, ApcState.Process) == KTHREAD_APCSTATE_PROCESS);
C_ASSERT(FIELD_OFFSET(KPROCESS, DirectoryTableBase) == KPROCESS_DIRECTORY_TABLE_BASE);
C_ASSERT(FIELD_OFFSET(KPROCESS, IopmOffset) == KPROCESS_IOPM_OFFSET);
C_ASSERT(FIELD_OFFSET(KPROCESS, LdtDescriptor) == KPROCESS_LDT_DESCRIPTOR0);
C_ASSERT(FIELD_OFFSET(KV86M_TRAP_FRAME, SavedExceptionStack) == TF_SAVED_EXCEPTION_STACK);
C_ASSERT(FIELD_OFFSET(KV86M_TRAP_FRAME, regs) == TF_REGS);
C_ASSERT(FIELD_OFFSET(KV86M_TRAP_FRAME, orig_ebp) == TF_ORIG_EBP);
//C_ASSERT(FIELD_OFFSET(KPCR, Tib.ExceptionList) == KPCR_EXCEPTION_LIST);
//C_ASSERT(FIELD_OFFSET(KPCR, Self) == KPCR_SELF);
C_ASSERT(FIELD_OFFSET(KPCR, IRR) == KPCR_IRR);
C_ASSERT(FIELD_OFFSET(KPCR, IDR) == KPCR_IDR);
C_ASSERT(FIELD_OFFSET(KPCR, Irql) == KPCR_IRQL);
C_ASSERT(FIELD_OFFSET(KIPCR, PrcbData) + FIELD_OFFSET(KPRCB, CurrentThread) == KPCR_CURRENT_THREAD);
C_ASSERT(FIELD_OFFSET(KIPCR, PrcbData) + FIELD_OFFSET(KPRCB, NpxThread) == KPCR_NPX_THREAD);
C_ASSERT(FIELD_OFFSET(KTSS, Esp0) == KTSS_ESP0);
C_ASSERT(FIELD_OFFSET(KTSS, IoMapBase) == KTSS_IOMAPBASE);
C_ASSERT(sizeof(FX_SAVE_AREA) == SIZEOF_FX_SAVE_AREA);

#endif /* INCLUDE_INTERNAL_NTOSKRNL_H */
