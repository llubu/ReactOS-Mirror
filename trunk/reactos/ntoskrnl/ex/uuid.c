/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ex/uuid.c
 * PURPOSE:         UUID generator
 *
 * PROGRAMMERS:     No programmer listed.
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

#define SEED_BUFFER_SIZE 6

/* Number of 100ns ticks per clock tick. To be safe, assume that the clock
   resolution is at least 1000 * 100 * (1/1000000) = 1/10 of a second */
#define TICKS_PER_CLOCK_TICK 1000
#define SECSPERDAY  86400
#define TICKSPERSEC 10000000

/* UUID system time starts at October 15, 1582 */
#define SECS_15_OCT_1582_TO_1601  ((17 + 30 + 31 + 365 * 18 + 5) * SECSPERDAY)
#define TICKS_15_OCT_1582_TO_1601 ((ULONGLONG)SECS_15_OCT_1582_TO_1601 * TICKSPERSEC)


/* GLOBALS ****************************************************************/

static FAST_MUTEX UuidMutex;
static LARGE_INTEGER UuidLastTime;
static ULONG UuidSequence;
static BOOLEAN UuidSequenceInitialized = FALSE;
static BOOLEAN UuidSequenceChanged = FALSE;
static UCHAR UuidSeed[SEED_BUFFER_SIZE];
static ULONG UuidCount;



/* FUNCTIONS ****************************************************************/

VOID INIT_FUNCTION
ExpInitUuids(VOID)
{
  ExInitializeFastMutex(&UuidMutex);

  KeQuerySystemTime((PLARGE_INTEGER)&UuidLastTime);
  UuidLastTime.QuadPart += TICKS_15_OCT_1582_TO_1601;

  UuidCount = TICKS_PER_CLOCK_TICK;
  RtlZeroMemory(UuidSeed, SEED_BUFFER_SIZE);
}


#define VALUE_BUFFER_SIZE 256

static NTSTATUS
ExpLoadUuidSequence(PULONG Sequence)
{
  UCHAR ValueBuffer[VALUE_BUFFER_SIZE];
  PKEY_VALUE_PARTIAL_INFORMATION ValueInfo;
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING Name;
  HANDLE KeyHandle;
  ULONG ValueLength;
  NTSTATUS Status;

  RtlInitUnicodeString(&Name,
		       L"\\Registry\\Machine\\Software\\Microsoft\\Rpc");
  InitializeObjectAttributes(&ObjectAttributes,
			     &Name,
			     OBJ_CASE_INSENSITIVE,
			     NULL,
			     NULL);
  Status = ZwOpenKey(&KeyHandle,
		     KEY_QUERY_VALUE,
		     &ObjectAttributes);
  if (!NT_SUCCESS(Status))
  {
    DPRINT("ZwOpenKey() failed (Status %lx)\n", Status);
    return Status;
  }

  RtlInitUnicodeString(&Name,
		       L"UuidSequenceNumber");

  ValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)ValueBuffer;
  Status = ZwQueryValueKey(KeyHandle,
			   &Name,
			   KeyValuePartialInformation,
			   ValueBuffer,
			   VALUE_BUFFER_SIZE,
			   &ValueLength);
  ZwClose(KeyHandle);
  if (!NT_SUCCESS(Status))
  {
    DPRINT("ZwQueryValueKey() failed (Status %lx)\n", Status);
    return Status;
  }

  *Sequence = *((PULONG)ValueInfo->Data);

  DPRINT("Loaded sequence %lx\n", *Sequence);

  return STATUS_SUCCESS;
}
#undef VALUE_BUFFER_SIZE


static NTSTATUS
ExpSaveUuidSequence(PULONG Sequence)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING Name;
  HANDLE KeyHandle;
  NTSTATUS Status;

  RtlInitUnicodeString(&Name,
		       L"\\Registry\\Machine\\Software\\Microsoft\\Rpc");
  InitializeObjectAttributes(&ObjectAttributes,
			     &Name,
			     OBJ_CASE_INSENSITIVE,
			     NULL,
			     NULL);
  Status = ZwOpenKey(&KeyHandle,
		     KEY_SET_VALUE,
		     &ObjectAttributes);
  if (!NT_SUCCESS(Status))
  {
    DPRINT("ZwOpenKey() failed (Status %lx)\n", Status);
    return Status;
  }

  RtlInitUnicodeString(&Name,
		       L"UuidSequenceNumber");
  Status = ZwSetValueKey(KeyHandle,
			 &Name,
			 0,
			 REG_DWORD,
			 Sequence,
			 sizeof(ULONG));
  ZwClose(KeyHandle);
  if (!NT_SUCCESS(Status))
  {
    DPRINT("ZwSetValueKey() failed (Status %lx)\n", Status);
  }

  return Status;
}


static VOID
ExpGetRandomUuidSequence(PULONG Sequence)
{
  LARGE_INTEGER Counter;
  LARGE_INTEGER Frequency;
  ULONG Value;

  Counter = KeQueryPerformanceCounter(&Frequency);
  Value = Counter.u.LowPart ^ Counter.u.HighPart;

  *Sequence = *Sequence ^ Value;

  DPRINT("Sequence %lx\n", *Sequence);
}


static NTSTATUS
ExpCreateUuids(PULARGE_INTEGER Time,
	       PULONG Range,
	       PULONG Sequence)
{
  /*
   * Generate time element of the UUID. Account for going faster
   * than our clock as well as the clock going backwards.
   */
  while (1)
  {
    KeQuerySystemTime((PLARGE_INTEGER)Time);
    Time->QuadPart += TICKS_15_OCT_1582_TO_1601;

    if (Time->QuadPart > UuidLastTime.QuadPart)
    {
      UuidCount = 0;
      break;
    }

    if (Time->QuadPart < UuidLastTime.QuadPart)
    {
      (*Sequence)++;
      UuidSequenceChanged = TRUE;
      UuidCount = 0;
      break;
    }

    if (UuidCount < TICKS_PER_CLOCK_TICK)
    {
      UuidCount++;
      break;
    }
  }

  UuidLastTime.QuadPart = Time->QuadPart;
  Time->QuadPart += UuidCount;

  *Range = 10000; /* What does this mean? Ticks per millisecond?*/

  return STATUS_SUCCESS;
}


/*
 * @unimplemented
 */
NTSTATUS STDCALL
NtAllocateUuids(OUT PULARGE_INTEGER Time,
		OUT PULONG Range,
		OUT PULONG Sequence,
		OUT PUCHAR Seed)
{
  ULARGE_INTEGER IntTime;
  ULONG IntRange;
  NTSTATUS Status;

  ExAcquireFastMutex(&UuidMutex);

  if (!UuidSequenceInitialized)
  {
    Status = ExpLoadUuidSequence(&UuidSequence);
    if (NT_SUCCESS(Status))
    {
      UuidSequence++;
    }
    else
    {
      ExpGetRandomUuidSequence(&UuidSequence);
    }

    UuidSequenceInitialized = TRUE;
    UuidSequenceChanged = TRUE;
  }

  Status = ExpCreateUuids(&IntTime,
                          &IntRange,
                          &UuidSequence);
  if (!NT_SUCCESS(Status))
  {
    ExReleaseFastMutex(&UuidMutex);
    return Status;
  }

  if (UuidSequenceChanged)
  {
    Status = ExpSaveUuidSequence(&UuidSequence);
    if (NT_SUCCESS(Status))
      UuidSequenceChanged = FALSE;
  }

  ExReleaseFastMutex(&UuidMutex);

  Time->QuadPart = IntTime.QuadPart;
  *Range = IntRange;
  *Sequence = UuidSequence;

  RtlCopyMemory(Seed,
                UuidSeed,
                SEED_BUFFER_SIZE);

  return STATUS_SUCCESS;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
NtSetUuidSeed(IN PUCHAR Seed)
{
  RtlCopyMemory(UuidSeed,
                Seed,
                SEED_BUFFER_SIZE);
  return STATUS_SUCCESS;
}

/* EOF */
