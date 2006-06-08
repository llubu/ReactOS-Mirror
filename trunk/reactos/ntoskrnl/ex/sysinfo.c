/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ex/sysinfo.c
 * PURPOSE:         System information functions
 *
 * PROGRAMMERS:     David Welch (welch@mcmail.com)
 *                  Aleksey Bragin (aleksey@studiocerebral.com)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

extern PEPROCESS PsIdleProcess;
extern ULONG NtGlobalFlag; /* FIXME: it should go in a ddk/?.h */
ULONGLONG STDCALL KeQueryInterruptTime(VOID);

VOID MmPrintMemoryStatistic(VOID);

/* FUNCTIONS *****************************************************************/

/*
 * @unimplemented
 */
VOID
STDCALL
ExEnumHandleTable (
	PULONG	HandleTable,
	PVOID	Callback,
	PVOID	Param,
	PHANDLE	Handle OPTIONAL
	)
{
	UNIMPLEMENTED;
}

/*
 * @implemented
 */
VOID
STDCALL
ExGetCurrentProcessorCpuUsage (
	PULONG	CpuUsage
	)
{
	PKPRCB Prcb;
	ULONG TotalTime;
	ULONGLONG ScaledIdle;

	Prcb = KeGetCurrentPrcb();

	ScaledIdle = Prcb->IdleThread->KernelTime * 100;
	TotalTime = Prcb->KernelTime + Prcb->UserTime;
	if (TotalTime != 0)
		*CpuUsage = 100 - (ScaledIdle / TotalTime);
	else
		*CpuUsage = 0;
}

/*
 * @implemented
 */
VOID
STDCALL
ExGetCurrentProcessorCounts (
	PULONG	ThreadKernelTime,
	PULONG	TotalCpuTime,
	PULONG	ProcessorNumber
	)
{
	PKPRCB Prcb;

	Prcb = KeGetCurrentPrcb();

	*ThreadKernelTime = Prcb->KernelTime + Prcb->UserTime;
	*TotalCpuTime = Prcb->CurrentThread->KernelTime;
	*ProcessorNumber = KeGetCurrentProcessorNumber();
}

/*
 * @implemented
 */
BOOLEAN
STDCALL
ExIsProcessorFeaturePresent(IN ULONG ProcessorFeature)
{
    /* Quick check to see if it exists at all */
    if (ProcessorFeature >= PROCESSOR_FEATURE_MAX) return(FALSE);

    /* Return our support for it */
    return(SharedUserData->ProcessorFeatures[ProcessorFeature]);
}

/*
 * @implemented
 */
BOOLEAN
STDCALL
ExVerifySuite(SUITE_TYPE SuiteType)
{
    if (SuiteType == Personal) return TRUE;
    return FALSE;
}

NTSTATUS STDCALL
NtQuerySystemEnvironmentValue (IN	PUNICODE_STRING	VariableName,
			       OUT	PWSTR		ValueBuffer,
			       IN	ULONG		ValueBufferLength,
			       IN OUT	PULONG		ReturnLength  OPTIONAL)
{
  ANSI_STRING AName;
  UNICODE_STRING WName;
  BOOLEAN Result;
  PCH Value;
  ANSI_STRING AValue;
  UNICODE_STRING WValue;
  KPROCESSOR_MODE PreviousMode;
  NTSTATUS Status = STATUS_SUCCESS;

  PAGED_CODE();

  PreviousMode = ExGetPreviousMode();

  if(PreviousMode != KernelMode)
  {
    _SEH_TRY
    {
      ProbeForRead(VariableName,
                   sizeof(UNICODE_STRING),
                   sizeof(ULONG));
      ProbeForWrite(ValueBuffer,
                    ValueBufferLength,
                    sizeof(WCHAR));
      if(ReturnLength != NULL)
      {
        ProbeForWriteUlong(ReturnLength);
      }
    }
    _SEH_EXCEPT(_SEH_ExSystemExceptionFilter)
    {
      Status = _SEH_GetExceptionCode();
    }
    _SEH_END;

    if(!NT_SUCCESS(Status))
    {
      return Status;
    }
  }

  /*
   * Copy the name to kernel space if necessary and convert it to ANSI.
   */
  Status = ProbeAndCaptureUnicodeString(&WName,
                                        PreviousMode,
                                        VariableName);
  if(NT_SUCCESS(Status))
  {
    /*
     * according to ntinternals the SeSystemEnvironmentName privilege is required!
     */
    if(!SeSinglePrivilegeCheck(SeSystemEnvironmentPrivilege,
                               PreviousMode))
    {
      ReleaseCapturedUnicodeString(&WName,
                                   PreviousMode);
      DPRINT1("NtQuerySystemEnvironmentValue: Caller requires the SeSystemEnvironmentPrivilege privilege!\n");
      return STATUS_PRIVILEGE_NOT_HELD;
    }

    /*
     * convert the value name to ansi
     */
    Status = RtlUnicodeStringToAnsiString(&AName, &WName, TRUE);
    ReleaseCapturedUnicodeString(&WName,
                                 PreviousMode);
    if(!NT_SUCCESS(Status))
    {
      return Status;
    }

  /*
   * Create a temporary buffer for the value
   */
    Value = ExAllocatePool(NonPagedPool, ValueBufferLength);
    if (Value == NULL)
    {
      RtlFreeAnsiString(&AName);
      return STATUS_INSUFFICIENT_RESOURCES;
    }

    /*
     * Get the environment variable
     */
    Result = HalGetEnvironmentVariable(AName.Buffer, Value, ValueBufferLength);
    if(!Result)
    {
      RtlFreeAnsiString(&AName);
      ExFreePool(Value);
      return STATUS_UNSUCCESSFUL;
    }

    /*
     * Convert the result to UNICODE, protect with SEH in case the value buffer
     * isn't NULL-terminated!
     */
    _SEH_TRY
    {
      RtlInitAnsiString(&AValue, Value);
      Status = RtlAnsiStringToUnicodeString(&WValue, &AValue, TRUE);
    }
    _SEH_EXCEPT(_SEH_ExSystemExceptionFilter)
    {
      Status = _SEH_GetExceptionCode();
    }
    _SEH_END;

    if(NT_SUCCESS(Status))
    {
      /*
       * Copy the result back to the caller.
       */
      _SEH_TRY
      {
        RtlCopyMemory(ValueBuffer, WValue.Buffer, WValue.Length);
        ValueBuffer[WValue.Length / sizeof(WCHAR)] = L'\0';
        if(ReturnLength != NULL)
        {
          *ReturnLength = WValue.Length + sizeof(WCHAR);
        }

        Status = STATUS_SUCCESS;
      }
      _SEH_EXCEPT(_SEH_ExSystemExceptionFilter)
      {
        Status = _SEH_GetExceptionCode();
      }
      _SEH_END;
    }

    /*
     * Cleanup allocated resources.
     */
    RtlFreeAnsiString(&AName);
    ExFreePool(Value);
  }

  return Status;
}


NTSTATUS STDCALL
NtSetSystemEnvironmentValue (IN	PUNICODE_STRING	VariableName,
			     IN	PUNICODE_STRING	Value)
{
  UNICODE_STRING CapturedName, CapturedValue;
  ANSI_STRING AName, AValue;
  KPROCESSOR_MODE PreviousMode;
  NTSTATUS Status;

  PAGED_CODE();

  PreviousMode = ExGetPreviousMode();

  /*
   * Copy the strings to kernel space if necessary
   */
  Status = ProbeAndCaptureUnicodeString(&CapturedName,
                                        PreviousMode,
                                        VariableName);
  if(NT_SUCCESS(Status))
  {
    Status = ProbeAndCaptureUnicodeString(&CapturedValue,
                                          PreviousMode,
                                          Value);
    if(NT_SUCCESS(Status))
    {
      /*
       * according to ntinternals the SeSystemEnvironmentName privilege is required!
       */
      if(SeSinglePrivilegeCheck(SeSystemEnvironmentPrivilege,
                                PreviousMode))
      {
        /*
         * convert the strings to ANSI
         */
        Status = RtlUnicodeStringToAnsiString(&AName,
                                              &CapturedName,
                                              TRUE);
        if(NT_SUCCESS(Status))
        {
          Status = RtlUnicodeStringToAnsiString(&AValue,
                                                &CapturedValue,
                                                TRUE);
          if(NT_SUCCESS(Status))
          {
            BOOLEAN Result = HalSetEnvironmentVariable(AName.Buffer,
                                                       AValue.Buffer);

            Status = (Result ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL);
          }
        }
      }
      else
      {
        DPRINT1("NtSetSystemEnvironmentValue: Caller requires the SeSystemEnvironmentPrivilege privilege!\n");
        Status = STATUS_PRIVILEGE_NOT_HELD;
      }

      ReleaseCapturedUnicodeString(&CapturedValue,
                                   PreviousMode);
    }

    ReleaseCapturedUnicodeString(&CapturedName,
                                 PreviousMode);
  }

  return Status;
}


/* --- Query/Set System Information --- */

/*
 * NOTE: QSI_DEF(n) and SSI_DEF(n) define _cdecl function symbols
 * so the stack is popped only in one place on x86 platform.
 */
#define QSI_USE(n) QSI##n
#define QSI_DEF(n) \
static NTSTATUS QSI_USE(n) (PVOID Buffer, ULONG Size, PULONG ReqSize)

#define SSI_USE(n) SSI##n
#define SSI_DEF(n) \
static NTSTATUS SSI_USE(n) (PVOID Buffer, ULONG Size)


/* Class 0 - Basic Information */
QSI_DEF(SystemBasicInformation)
{
	PSYSTEM_BASIC_INFORMATION Sbi
		= (PSYSTEM_BASIC_INFORMATION) Buffer;

	*ReqSize = sizeof (SYSTEM_BASIC_INFORMATION);
	/*
	 * Check user buffer's size
	 */
	if (Size != sizeof (SYSTEM_BASIC_INFORMATION))
	{
		return (STATUS_INFO_LENGTH_MISMATCH);
	}
	Sbi->Reserved = 0;
	Sbi->TimerResolution = KeMaximumIncrement;
	Sbi->PageSize = PAGE_SIZE;
	Sbi->NumberOfPhysicalPages = MmStats.NrTotalPages;
	Sbi->LowestPhysicalPageNumber = 0; /* FIXME */
	Sbi->HighestPhysicalPageNumber = MmStats.NrTotalPages; /* FIXME */
	Sbi->AllocationGranularity = MM_VIRTMEM_GRANULARITY; /* hard coded on Intel? */
	Sbi->MinimumUserModeAddress = 0x10000; /* Top of 64k */
	Sbi->MaximumUserModeAddress = (ULONG_PTR)MmHighestUserAddress;
	Sbi->ActiveProcessorsAffinityMask = KeActiveProcessors;
	Sbi->NumberOfProcessors = KeNumberProcessors;
	return (STATUS_SUCCESS);
}

/* Class 1 - Processor Information */
QSI_DEF(SystemProcessorInformation)
{
	PSYSTEM_PROCESSOR_INFORMATION Spi
		= (PSYSTEM_PROCESSOR_INFORMATION) Buffer;
	PKPRCB Prcb;
	*ReqSize = sizeof (SYSTEM_PROCESSOR_INFORMATION);
	/*
	 * Check user buffer's size
	 */
	if (Size < sizeof (SYSTEM_PROCESSOR_INFORMATION))
	{
		return (STATUS_INFO_LENGTH_MISMATCH);
	}
	Prcb = KeGetCurrentPrcb();
	Spi->ProcessorArchitecture = 0; /* Intel Processor */
	Spi->ProcessorLevel	   = Prcb->CpuType;
	Spi->ProcessorRevision	   = Prcb->CpuStep;
	Spi->Reserved 		   = 0;
	Spi->ProcessorFeatureBits	   = Prcb->FeatureBits;

	DPRINT("Arch %d Level %d Rev 0x%x\n", Spi->ProcessorArchitecture,
		Spi->ProcessorLevel, Spi->ProcessorRevision);

	return (STATUS_SUCCESS);
}

/* Class 2 - Performance Information */
QSI_DEF(SystemPerformanceInformation)
{
	PSYSTEM_PERFORMANCE_INFORMATION Spi
		= (PSYSTEM_PERFORMANCE_INFORMATION) Buffer;

	PEPROCESS TheIdleProcess;

	*ReqSize = sizeof (SYSTEM_PERFORMANCE_INFORMATION);
	/*
	 * Check user buffer's size
	 */
	if (Size < sizeof (SYSTEM_PERFORMANCE_INFORMATION))
	{
		return (STATUS_INFO_LENGTH_MISMATCH);
	}

	TheIdleProcess = PsIdleProcess;

	Spi->IdleProcessTime.QuadPart = TheIdleProcess->Pcb.KernelTime * 100000LL;

	Spi->IoReadTransferCount = IoReadTransferCount;
	Spi->IoWriteTransferCount = IoWriteTransferCount;
	Spi->IoOtherTransferCount = IoOtherTransferCount;
	Spi->IoReadOperationCount = IoReadOperationCount;
	Spi->IoWriteOperationCount = IoWriteOperationCount;
	Spi->IoOtherOperationCount = IoOtherOperationCount;

	Spi->AvailablePages = MmStats.NrFreePages;
/*
        Add up all the used "Commitied" memory + pagefile.
        Not sure this is right. 8^\
 */
	Spi->CommittedPages = MiMemoryConsumers[MC_PPOOL].PagesUsed +
				   MiMemoryConsumers[MC_NPPOOL].PagesUsed+
                                   MiMemoryConsumers[MC_CACHE].PagesUsed+
		                   MiMemoryConsumers[MC_USER].PagesUsed+
			           MiUsedSwapPages;
/*
	Add up the full system total + pagefile.
	All this make Taskmgr happy but not sure it is the right numbers.
	This too, fixes some of GlobalMemoryStatusEx numbers.
*/
        Spi->CommitLimit = MmStats.NrTotalPages + MiFreeSwapPages +
                                MiUsedSwapPages;

	Spi->PeakCommitment = 0; /* FIXME */
	Spi->PageFaultCount = 0; /* FIXME */
	Spi->CopyOnWriteCount = 0; /* FIXME */
	Spi->TransitionCount = 0; /* FIXME */
	Spi->CacheTransitionCount = 0; /* FIXME */
	Spi->DemandZeroCount = 0; /* FIXME */
	Spi->PageReadCount = 0; /* FIXME */
	Spi->PageReadIoCount = 0; /* FIXME */
	Spi->CacheReadCount = 0; /* FIXME */
	Spi->CacheIoCount = 0; /* FIXME */
    Spi->DirtyPagesWriteCount = 0; /* FIXME */
	Spi->DirtyWriteIoCount = 0; /* FIXME */
	Spi->MappedPagesWriteCount = 0; /* FIXME */
	Spi->MappedWriteIoCount = 0; /* FIXME */

	Spi->PagedPoolPages = MiMemoryConsumers[MC_PPOOL].PagesUsed;
	Spi->PagedPoolAllocs = 0; /* FIXME */
	Spi->PagedPoolFrees = 0; /* FIXME */
	Spi->NonPagedPoolPages = MiMemoryConsumers[MC_NPPOOL].PagesUsed;
	Spi->NonPagedPoolAllocs = 0; /* FIXME */
	Spi->NonPagedPoolFrees = 0; /* FIXME */

	Spi->FreeSystemPtes = 0; /* FIXME */

	Spi->ResidentSystemCodePage = MmStats.NrSystemPages; /* FIXME */

	Spi->TotalSystemDriverPages = 0; /* FIXME */
	Spi->TotalSystemCodePages = 0; /* FIXME */
	Spi->NonPagedPoolLookasideHits = 0; /* FIXME */
	Spi->PagedPoolLookasideHits = 0; /* FIXME */
	Spi->Spare3Count = 0; /* FIXME */

	Spi->ResidentSystemCachePage = MiMemoryConsumers[MC_CACHE].PagesUsed;
	Spi->ResidentPagedPoolPage = MmPagedPoolSize; /* FIXME */

	Spi->ResidentSystemDriverPage = 0; /* FIXME */
	Spi->CcFastReadNoWait = 0; /* FIXME */
	Spi->CcFastReadWait = 0; /* FIXME */
	Spi->CcFastReadResourceMiss = 0; /* FIXME */
	Spi->CcFastReadNotPossible = 0; /* FIXME */

	Spi->CcFastMdlReadNoWait = 0; /* FIXME */
	Spi->CcFastMdlReadWait = 0; /* FIXME */
	Spi->CcFastMdlReadResourceMiss = 0; /* FIXME */
	Spi->CcFastMdlReadNotPossible = 0; /* FIXME */

	Spi->CcMapDataNoWait = 0; /* FIXME */
	Spi->CcMapDataWait = 0; /* FIXME */
	Spi->CcMapDataNoWaitMiss = 0; /* FIXME */
	Spi->CcMapDataWaitMiss = 0; /* FIXME */

	Spi->CcPinMappedDataCount = 0; /* FIXME */
	Spi->CcPinReadNoWait = 0; /* FIXME */
	Spi->CcPinReadWait = 0; /* FIXME */
	Spi->CcPinReadNoWaitMiss = 0; /* FIXME */
	Spi->CcPinReadWaitMiss = 0; /* FIXME */
	Spi->CcCopyReadNoWait = 0; /* FIXME */
	Spi->CcCopyReadWait = 0; /* FIXME */
	Spi->CcCopyReadNoWaitMiss = 0; /* FIXME */
	Spi->CcCopyReadWaitMiss = 0; /* FIXME */

	Spi->CcMdlReadNoWait = 0; /* FIXME */
	Spi->CcMdlReadWait = 0; /* FIXME */
	Spi->CcMdlReadNoWaitMiss = 0; /* FIXME */
	Spi->CcMdlReadWaitMiss = 0; /* FIXME */
	Spi->CcReadAheadIos = 0; /* FIXME */
	Spi->CcLazyWriteIos = 0; /* FIXME */
	Spi->CcLazyWritePages = 0; /* FIXME */
	Spi->CcDataFlushes = 0; /* FIXME */
	Spi->CcDataPages = 0; /* FIXME */
	Spi->ContextSwitches = 0; /* FIXME */
	Spi->FirstLevelTbFills = 0; /* FIXME */
	Spi->SecondLevelTbFills = 0; /* FIXME */
	Spi->SystemCalls = 0; /* FIXME */

	return (STATUS_SUCCESS);
}

/* Class 3 - Time Of Day Information */
QSI_DEF(SystemTimeOfDayInformation)
{
  PSYSTEM_TIMEOFDAY_INFORMATION Sti;
  LARGE_INTEGER CurrentTime;

  Sti = (PSYSTEM_TIMEOFDAY_INFORMATION)Buffer;
  *ReqSize = sizeof (SYSTEM_TIMEOFDAY_INFORMATION);

  /* Check user buffer's size */
  if (Size != sizeof (SYSTEM_TIMEOFDAY_INFORMATION))
    {
      return STATUS_INFO_LENGTH_MISMATCH;
    }

  KeQuerySystemTime(&CurrentTime);

  Sti->BootTime= SystemBootTime;
  Sti->CurrentTime = CurrentTime;
  Sti->TimeZoneBias.QuadPart = ExpTimeZoneBias.QuadPart;
  Sti->TimeZoneId = ExpTimeZoneId;
  Sti->Reserved = 0;

  return STATUS_SUCCESS;
}

/* Class 4 - Path Information */
QSI_DEF(SystemPathInformation)
{
	/* FIXME: QSI returns STATUS_BREAKPOINT. Why? */
	DPRINT1("NtQuerySystemInformation - SystemPathInformation not implemented\n");

	return (STATUS_BREAKPOINT);
}

/* Class 5 - Process Information */
QSI_DEF(SystemProcessInformation)
{
	ULONG ovlSize = 0, nThreads;
	PEPROCESS pr = NULL, syspr;
	unsigned char *pCur;
	NTSTATUS Status = STATUS_SUCCESS;

	_SEH_TRY
	{
		/* scan the process list */

		PSYSTEM_PROCESS_INFORMATION Spi
			= (PSYSTEM_PROCESS_INFORMATION) Buffer;

		*ReqSize = sizeof(SYSTEM_PROCESS_INFORMATION);

		if (Size < sizeof(SYSTEM_PROCESS_INFORMATION))
		{
			return (STATUS_INFO_LENGTH_MISMATCH); // in case buffer size is too small
		}

		syspr = PsGetNextProcess(NULL);
		pr = syspr;
		pCur = (unsigned char *)Spi;

		do
		{
			PSYSTEM_PROCESS_INFORMATION SpiCur;
			int curSize;
			ANSI_STRING	imgName;
			int inLen=32; // image name len in bytes
			PLIST_ENTRY current_entry;
			PETHREAD current;
            PSYSTEM_THREAD_INFORMATION ThreadInfo;

			SpiCur = (PSYSTEM_PROCESS_INFORMATION)pCur;

			nThreads = 0;
			current_entry = pr->ThreadListHead.Flink;
			while (current_entry != &pr->ThreadListHead)
			{
				nThreads++;
				current_entry = current_entry->Flink;
			}

			// size of the structure for every process
			curSize = sizeof(SYSTEM_PROCESS_INFORMATION)+sizeof(SYSTEM_THREAD_INFORMATION)*nThreads;
			ovlSize += curSize+inLen;

			if (ovlSize > Size)
			{
				*ReqSize = ovlSize;
				ObDereferenceObject(pr);

				return (STATUS_INFO_LENGTH_MISMATCH); // in case buffer size is too small
			}

			// fill system information
			SpiCur->NextEntryOffset = curSize+inLen; // relative offset to the beginnnig of the next structure
			SpiCur->NumberOfThreads = nThreads;
			SpiCur->CreateTime = pr->CreateTime;
			SpiCur->UserTime.QuadPart = pr->Pcb.UserTime * 100000LL;
			SpiCur->KernelTime.QuadPart = pr->Pcb.KernelTime * 100000LL;
			SpiCur->ImageName.Length = strlen(pr->ImageFileName) * sizeof(WCHAR);
			SpiCur->ImageName.MaximumLength = inLen;
			SpiCur->ImageName.Buffer = (void*)(pCur+curSize);

			// copy name to the end of the struct
			if(pr != PsIdleProcess)
			{
				RtlInitAnsiString(&imgName, pr->ImageFileName);
				RtlAnsiStringToUnicodeString(&SpiCur->ImageName, &imgName, FALSE);
			}
			else
			{
                		RtlInitUnicodeString(&SpiCur->ImageName, NULL);
			}

			SpiCur->BasePriority = pr->Pcb.BasePriority;
			SpiCur->UniqueProcessId = pr->UniqueProcessId;
			SpiCur->InheritedFromUniqueProcessId = pr->InheritedFromUniqueProcessId;
			SpiCur->HandleCount = (pr->ObjectTable ? ObpGetHandleCountByHandleTable(pr->ObjectTable) : 0);
			SpiCur->PeakVirtualSize = pr->PeakVirtualSize;
			SpiCur->VirtualSize = pr->VirtualSize;
			SpiCur->PageFaultCount = pr->Vm.PageFaultCount;
			SpiCur->PeakWorkingSetSize = pr->Vm.PeakWorkingSetSize;
			SpiCur->WorkingSetSize = pr->Vm.WorkingSetSize;
			SpiCur->QuotaPeakPagedPoolUsage = pr->QuotaPeak[0];
			SpiCur->QuotaPagedPoolUsage = pr->QuotaUsage[0];
			SpiCur->QuotaPeakNonPagedPoolUsage = pr->QuotaPeak[1];
			SpiCur->QuotaNonPagedPoolUsage = pr->QuotaUsage[1];
			SpiCur->PagefileUsage = pr->QuotaUsage[3];
			SpiCur->PeakPagefileUsage = pr->QuotaPeak[3];
			SpiCur->PrivatePageCount = pr->CommitCharge;
            ThreadInfo = (PSYSTEM_THREAD_INFORMATION)(SpiCur + 1);

		        current_entry = pr->ThreadListHead.Flink;
        		while (current_entry != &pr->ThreadListHead)
			{
				current = CONTAINING_RECORD(current_entry, ETHREAD,
				                            ThreadListEntry);

                
				ThreadInfo->KernelTime.QuadPart = current->Tcb.KernelTime * 100000LL;
				ThreadInfo->UserTime.QuadPart = current->Tcb.UserTime * 100000LL;
//				SpiCur->TH[i].CreateTime = current->CreateTime;
				ThreadInfo->WaitTime = current->Tcb.WaitTime;
				ThreadInfo->StartAddress = (PVOID) current->StartAddress;
				ThreadInfo->ClientId = current->Cid;
				ThreadInfo->Priority = current->Tcb.Priority;
				ThreadInfo->BasePriority = current->Tcb.BasePriority;
				ThreadInfo->ContextSwitches = current->Tcb.ContextSwitches;
				ThreadInfo->ThreadState = current->Tcb.State;
				ThreadInfo->WaitReason = current->Tcb.WaitReason;
				ThreadInfo++;
				current_entry = current_entry->Flink;
			}

			pr = PsGetNextProcess(pr);
			nThreads = 0;
			if ((pr == syspr) || (pr == NULL))
			{
				SpiCur->NextEntryOffset = 0;
				break;
			}
			else
				pCur = pCur + curSize + inLen;
		}  while ((pr != syspr) && (pr != NULL));

		if(pr != NULL)
			ObDereferenceObject(pr);
		Status = STATUS_SUCCESS;
	}
	_SEH_HANDLE
	{
		if(pr != NULL)
			ObDereferenceObject(pr);
		Status = _SEH_GetExceptionCode();
	}
	_SEH_END

	*ReqSize = ovlSize;
	return Status;
}

/* Class 6 - Call Count Information */
QSI_DEF(SystemCallCountInformation)
{
	/* FIXME */
	DPRINT1("NtQuerySystemInformation - SystemCallCountInformation not implemented\n");
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 7 - Device Information */
QSI_DEF(SystemDeviceInformation)
{
	PSYSTEM_DEVICE_INFORMATION Sdi
		= (PSYSTEM_DEVICE_INFORMATION) Buffer;
	PCONFIGURATION_INFORMATION ConfigInfo;

	*ReqSize = sizeof (SYSTEM_DEVICE_INFORMATION);
	/*
	 * Check user buffer's size
	 */
	if (Size < sizeof (SYSTEM_DEVICE_INFORMATION))
	{
		return (STATUS_INFO_LENGTH_MISMATCH);
	}

	ConfigInfo = IoGetConfigurationInformation ();

	Sdi->NumberOfDisks = ConfigInfo->DiskCount;
	Sdi->NumberOfFloppies = ConfigInfo->FloppyCount;
	Sdi->NumberOfCdRoms = ConfigInfo->CdRomCount;
	Sdi->NumberOfTapes = ConfigInfo->TapeCount;
	Sdi->NumberOfSerialPorts = ConfigInfo->SerialCount;
	Sdi->NumberOfParallelPorts = ConfigInfo->ParallelCount;

	return (STATUS_SUCCESS);
}

/* Class 8 - Processor Performance Information */
QSI_DEF(SystemProcessorPerformanceInformation)
{
	PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION Spi
		= (PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) Buffer;

        LONG i;
	LARGE_INTEGER CurrentTime;
	PKPRCB Prcb;

	*ReqSize = KeNumberProcessors * sizeof (SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION);
	/*
	 * Check user buffer's size
	 */
	if (Size < KeNumberProcessors * sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION))
	{
		return (STATUS_INFO_LENGTH_MISMATCH);
	}

	CurrentTime.QuadPart = KeQueryInterruptTime();
	Prcb = ((PKPCR)KPCR_BASE)->Prcb;
	for (i = 0; i < KeNumberProcessors; i++)
	{
	   Spi->IdleTime.QuadPart = (Prcb->IdleThread->KernelTime + Prcb->IdleThread->UserTime) * 100000LL; // IdleTime
           Spi->KernelTime.QuadPart =  Prcb->KernelTime * 100000LL; // KernelTime
           Spi->UserTime.QuadPart = Prcb->UserTime * 100000LL;
           Spi->DpcTime.QuadPart = Prcb->DpcTime * 100000LL;
           Spi->InterruptTime.QuadPart = Prcb->InterruptTime * 100000LL;
           Spi->InterruptCount = Prcb->InterruptCount; // Interrupt Count
	   Spi++;
	   Prcb = (PKPRCB)((ULONG_PTR)Prcb + PAGE_SIZE);
	}

	return (STATUS_SUCCESS);
}

/* Class 9 - Flags Information */
QSI_DEF(SystemFlagsInformation)
{
	if (sizeof (SYSTEM_FLAGS_INFORMATION) != Size)
	{
		* ReqSize = sizeof (SYSTEM_FLAGS_INFORMATION);
		return (STATUS_INFO_LENGTH_MISMATCH);
	}
	((PSYSTEM_FLAGS_INFORMATION) Buffer)->Flags = NtGlobalFlag;
	return (STATUS_SUCCESS);
}

SSI_DEF(SystemFlagsInformation)
{
	if (sizeof (SYSTEM_FLAGS_INFORMATION) != Size)
	{
		return (STATUS_INFO_LENGTH_MISMATCH);
	}
	NtGlobalFlag = ((PSYSTEM_FLAGS_INFORMATION) Buffer)->Flags;
	return (STATUS_SUCCESS);
}

/* Class 10 - Call Time Information */
QSI_DEF(SystemCallTimeInformation)
{
	/* FIXME */
	DPRINT1("NtQuerySystemInformation - SystemCallTimeInformation not implemented\n");
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 11 - Module Information */
QSI_DEF(SystemModuleInformation)
{
	return LdrpQueryModuleInformation(Buffer, Size, ReqSize);
}

/* Class 12 - Locks Information */
QSI_DEF(SystemLocksInformation)
{
	/* FIXME */
	DPRINT1("NtQuerySystemInformation - SystemLocksInformation not implemented\n");
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 13 - Stack Trace Information */
QSI_DEF(SystemStackTraceInformation)
{
	/* FIXME */
	DPRINT1("NtQuerySystemInformation - SystemStackTraceInformation not implemented\n");
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 14 - Paged Pool Information */
QSI_DEF(SystemPagedPoolInformation)
{
	/* FIXME */
	DPRINT1("NtQuerySystemInformation - SystemPagedPoolInformation not implemented\n");
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 15 - Non Paged Pool Information */
QSI_DEF(SystemNonPagedPoolInformation)
{
	/* FIXME */
	DPRINT1("NtQuerySystemInformation - SystemNonPagedPoolInformation not implemented\n");
	return (STATUS_NOT_IMPLEMENTED);
}


/* Class 16 - Handle Information */
QSI_DEF(SystemHandleInformation)
{
	PEPROCESS pr, syspr;
	ULONG curSize, i = 0;
	ULONG hCount = 0;

        PSYSTEM_HANDLE_INFORMATION Shi =
        	(PSYSTEM_HANDLE_INFORMATION) Buffer;

        DPRINT("NtQuerySystemInformation - SystemHandleInformation\n");

	if (Size < sizeof (SYSTEM_HANDLE_INFORMATION))
        {
		* ReqSize = sizeof (SYSTEM_HANDLE_INFORMATION);
		return (STATUS_INFO_LENGTH_MISMATCH);
	}

	DPRINT("SystemHandleInformation 1\n");

        /* First Calc Size from Count. */
        syspr = PsGetNextProcess(NULL);
	pr = syspr;

        do
	  {
            hCount = hCount + (pr->ObjectTable ? ObpGetHandleCountByHandleTable(pr->ObjectTable) : 0);
            pr = PsGetNextProcess(pr);

	    if ((pr == syspr) || (pr == NULL))
		break;
        } while ((pr != syspr) && (pr != NULL));

	if(pr != NULL)
	{
          ObDereferenceObject(pr);
	}

	DPRINT("SystemHandleInformation 2\n");

        curSize = sizeof(SYSTEM_HANDLE_INFORMATION)+
                  (  (sizeof(SYSTEM_HANDLE_TABLE_ENTRY_INFO) * hCount) -
                     (sizeof(SYSTEM_HANDLE_TABLE_ENTRY_INFO) ));

        Shi->NumberOfHandles = hCount;

        if (curSize > Size)
          {
            *ReqSize = curSize;
             return (STATUS_INFO_LENGTH_MISMATCH);
          }

	DPRINT("SystemHandleInformation 3\n");

        /* Now get Handles from all processs. */
        syspr = PsGetNextProcess(NULL);
	pr = syspr;

	 do
	  {
            int Count = 0, HandleCount;

            HandleCount = (pr->ObjectTable ? ObpGetHandleCountByHandleTable(pr->ObjectTable) : 0);

            for (Count = 0; HandleCount > 0 ; HandleCount--)
               {
                 Shi->Handles[i].UniqueProcessId = (ULONG)pr->UniqueProcessId;
                 Count++;
                 i++;
               }

	    pr = PsGetNextProcess(pr);

	    if ((pr == syspr) || (pr == NULL))
		break;
	   } while ((pr != syspr) && (pr != NULL));

	if(pr != NULL)
	{
          ObDereferenceObject(pr);
	}

	DPRINT("SystemHandleInformation 4\n");
	return (STATUS_SUCCESS);

}
/*
SSI_DEF(SystemHandleInformation)
{

	return (STATUS_SUCCESS);
}
*/

/* Class 17 -  Information */
QSI_DEF(SystemObjectInformation)
{
	/* FIXME */
	DPRINT1("NtQuerySystemInformation - SystemObjectInformation not implemented\n");
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 18 -  Information */
QSI_DEF(SystemPageFileInformation)
{
	UNICODE_STRING FileName; /* FIXME */
	SYSTEM_PAGEFILE_INFORMATION *Spfi = (SYSTEM_PAGEFILE_INFORMATION *) Buffer;

	if (Size < sizeof (SYSTEM_PAGEFILE_INFORMATION))
	{
		* ReqSize = sizeof (SYSTEM_PAGEFILE_INFORMATION);
		return (STATUS_INFO_LENGTH_MISMATCH);
	}

	RtlInitUnicodeString(&FileName, NULL); /* FIXME */

	/* FIXME */
	Spfi->NextEntryOffset = 0;

	Spfi->TotalSize = MiFreeSwapPages + MiUsedSwapPages;
	Spfi->TotalInUse = MiUsedSwapPages;
	Spfi->PeakUsage = MiUsedSwapPages; /* FIXME */
	Spfi->PageFileName = FileName;
	return (STATUS_SUCCESS);
}

/* Class 19 - Vdm Instemul Information */
QSI_DEF(SystemVdmInstemulInformation)
{
	/* FIXME */
	DPRINT1("NtQuerySystemInformation - SystemVdmInstemulInformation not implemented\n");
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 20 - Vdm Bop Information */
QSI_DEF(SystemVdmBopInformation)
{
	/* FIXME */
	DPRINT1("NtQuerySystemInformation - SystemVdmBopInformation not implemented\n");
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 21 - File Cache Information */
QSI_DEF(SystemFileCacheInformation)
{
	SYSTEM_FILECACHE_INFORMATION *Sci = (SYSTEM_FILECACHE_INFORMATION *) Buffer;

	if (Size < sizeof (SYSTEM_FILECACHE_INFORMATION))
	{
		* ReqSize = sizeof (SYSTEM_FILECACHE_INFORMATION);
		return (STATUS_INFO_LENGTH_MISMATCH);
	}

	RtlZeroMemory(Sci, sizeof(SYSTEM_FILECACHE_INFORMATION));

	/* Return the Byte size not the page size. */
	Sci->CurrentSize =
		MiMemoryConsumers[MC_CACHE].PagesUsed * PAGE_SIZE;
	Sci->PeakSize =
	        MiMemoryConsumers[MC_CACHE].PagesUsed * PAGE_SIZE; /* FIXME */

	Sci->PageFaultCount = 0; /* FIXME */
	Sci->MinimumWorkingSet = 0; /* FIXME */
	Sci->MaximumWorkingSet = 0; /* FIXME */

	return (STATUS_SUCCESS);
}

SSI_DEF(SystemFileCacheInformation)
{
	if (Size < sizeof (SYSTEM_FILECACHE_INFORMATION))
	{
		return (STATUS_INFO_LENGTH_MISMATCH);
	}
	/* FIXME */
	DPRINT1("NtSetSystemInformation - SystemFileCacheInformation not implemented\n");
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 22 - Pool Tag Information */
QSI_DEF(SystemPoolTagInformation)
{
	/* FIXME */
	DPRINT1("NtQuerySystemInformation - SystemPoolTagInformation not implemented\n");
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 23 - Interrupt Information for all processors */
QSI_DEF(SystemInterruptInformation)
{
  PKPRCB Prcb;
  LONG i;
  ULONG ti;
  PSYSTEM_INTERRUPT_INFORMATION sii = (PSYSTEM_INTERRUPT_INFORMATION)Buffer;

  if(Size < KeNumberProcessors * sizeof(SYSTEM_INTERRUPT_INFORMATION))
  {
    return (STATUS_INFO_LENGTH_MISMATCH);
  }

  ti = KeQueryTimeIncrement();

  Prcb = ((PKPCR)KPCR_BASE)->Prcb;
  for (i = 0; i < KeNumberProcessors; i++)
  {
    sii->ContextSwitches = Prcb->KeContextSwitches;
    sii->DpcCount = 0; /* FIXME */
    sii->DpcRate = 0; /* FIXME */
    sii->TimeIncrement = ti;
    sii->DpcBypassCount = 0; /* FIXME */
    sii->ApcBypassCount = 0; /* FIXME */
    sii++;
    Prcb = (PKPRCB)((ULONG_PTR)Prcb + PAGE_SIZE);
  }

  return STATUS_SUCCESS;
}

/* Class 24 - DPC Behaviour Information */
QSI_DEF(SystemDpcBehaviourInformation)
{
	/* FIXME */
	DPRINT1("NtQuerySystemInformation - SystemDpcBehaviourInformation not implemented\n");
	return (STATUS_NOT_IMPLEMENTED);
}

SSI_DEF(SystemDpcBehaviourInformation)
{
	/* FIXME */
	DPRINT1("NtSetSystemInformation - SystemDpcBehaviourInformation not implemented\n");
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 25 - Full Memory Information */
QSI_DEF(SystemFullMemoryInformation)
{
	PULONG Spi = (PULONG) Buffer;

	PEPROCESS TheIdleProcess;

	* ReqSize = sizeof (ULONG);

	if (sizeof (ULONG) != Size)
	{
		return (STATUS_INFO_LENGTH_MISMATCH);
	}
	DPRINT("SystemFullMemoryInformation\n");

	TheIdleProcess = PsIdleProcess;

        DPRINT("PID: %d, KernelTime: %u PFFree: %d PFUsed: %d\n",
               TheIdleProcess->UniqueProcessId,
               TheIdleProcess->Pcb.KernelTime,
               MiFreeSwapPages,
               MiUsedSwapPages);

#ifndef NDEBUG
	MmPrintMemoryStatistic();
#endif

	*Spi = MiMemoryConsumers[MC_USER].PagesUsed;

	return (STATUS_SUCCESS);
}

/* Class 26 - Load Image */
SSI_DEF(SystemLoadImage)
{
  PSYSTEM_GDI_DRIVER_INFORMATION Sli = (PSYSTEM_GDI_DRIVER_INFORMATION)Buffer;

  if (sizeof(SYSTEM_GDI_DRIVER_INFORMATION) != Size)
    {
      return(STATUS_INFO_LENGTH_MISMATCH);
    }

  return(LdrpLoadImage(&Sli->DriverName,
		       &Sli->ImageAddress,
		       &Sli->SectionPointer,
		       &Sli->EntryPoint,
		       (PVOID)&Sli->ExportSectionPointer));
}

/* Class 27 - Unload Image */
SSI_DEF(SystemUnloadImage)
{
  PVOID Sui = (PVOID)Buffer;

  if (sizeof(PVOID) != Size)
    {
      return(STATUS_INFO_LENGTH_MISMATCH);
    }

  return(LdrpUnloadImage(Sui));
}

/* Class 28 - Time Adjustment Information */
QSI_DEF(SystemTimeAdjustmentInformation)
{
	if (sizeof (SYSTEM_SET_TIME_ADJUST_INFORMATION) > Size)
	{
		* ReqSize = sizeof (SYSTEM_SET_TIME_ADJUST_INFORMATION);
		return (STATUS_INFO_LENGTH_MISMATCH);
	}
	/* FIXME: */
	DPRINT1("NtQuerySystemInformation - SystemTimeAdjustmentInformation not implemented\n");
	return (STATUS_NOT_IMPLEMENTED);
}

SSI_DEF(SystemTimeAdjustmentInformation)
{
	if (sizeof (SYSTEM_SET_TIME_ADJUST_INFORMATION) > Size)
	{
		return (STATUS_INFO_LENGTH_MISMATCH);
	}
	/* FIXME: */
	DPRINT1("NtSetSystemInformation - SystemTimeAdjustmentInformation not implemented\n");
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 29 - Summary Memory Information */
QSI_DEF(SystemSummaryMemoryInformation)
{
	/* FIXME */
	DPRINT1("NtQuerySystemInformation - SystemSummaryMemoryInformation not implemented\n");
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 30 - Next Event Id Information */
QSI_DEF(SystemNextEventIdInformation)
{
	/* FIXME */
	DPRINT1("NtQuerySystemInformation - SystemNextEventIdInformation not implemented\n");
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 31 - Event Ids Information */
QSI_DEF(SystemEventIdsInformation)
{
	/* FIXME */
	DPRINT1("NtQuerySystemInformation - SystemEventIdsInformation not implemented\n");
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 32 - Crash Dump Information */
QSI_DEF(SystemCrashDumpInformation)
{
	/* FIXME */
	DPRINT1("NtQuerySystemInformation - SystemCrashDumpInformation not implemented\n");
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 33 - Exception Information */
QSI_DEF(SystemExceptionInformation)
{
	/* FIXME */
	DPRINT1("NtQuerySystemInformation - SystemExceptionInformation not implemented\n");
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 34 - Crash Dump State Information */
QSI_DEF(SystemCrashDumpStateInformation)
{
	/* FIXME */
	DPRINT1("NtQuerySystemInformation - SystemCrashDumpStateInformation not implemented\n");
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 35 - Kernel Debugger Information */
QSI_DEF(SystemKernelDebuggerInformation)
{
  PSYSTEM_KERNEL_DEBUGGER_INFORMATION skdi = (PSYSTEM_KERNEL_DEBUGGER_INFORMATION) Buffer;

  *ReqSize = sizeof(SYSTEM_KERNEL_DEBUGGER_INFORMATION);
  if (Size < sizeof(SYSTEM_KERNEL_DEBUGGER_INFORMATION))
    {
      return STATUS_INFO_LENGTH_MISMATCH;
    }

  skdi->KernelDebuggerEnabled = KD_DEBUGGER_ENABLED;
  skdi->KernelDebuggerNotPresent = KD_DEBUGGER_NOT_PRESENT;

  return STATUS_SUCCESS;
}

/* Class 36 - Context Switch Information */
QSI_DEF(SystemContextSwitchInformation)
{
	/* FIXME */
	DPRINT1("NtQuerySystemInformation - SystemContextSwitchInformation not implemented\n");
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 37 - Registry Quota Information */
QSI_DEF(SystemRegistryQuotaInformation)
{
  PSYSTEM_REGISTRY_QUOTA_INFORMATION srqi = (PSYSTEM_REGISTRY_QUOTA_INFORMATION) Buffer;

  *ReqSize = sizeof(SYSTEM_REGISTRY_QUOTA_INFORMATION);
  if (Size < sizeof(SYSTEM_REGISTRY_QUOTA_INFORMATION))
    {
      return STATUS_INFO_LENGTH_MISMATCH;
    }

  DPRINT1("Faking max registry size of 32 MB\n");
  srqi->RegistryQuotaAllowed = 0x2000000;
  srqi->RegistryQuotaUsed = 0x200000;
  srqi->PagedPoolSize = 0x200000;

  return STATUS_SUCCESS;
}

SSI_DEF(SystemRegistryQuotaInformation)
{
	/* FIXME */
	DPRINT1("NtSetSystemInformation - SystemRegistryQuotaInformation not implemented\n");
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 38 - Load And Call Image */
SSI_DEF(SystemLoadAndCallImage)
{
  PUNICODE_STRING Slci = (PUNICODE_STRING)Buffer;

  if (sizeof(UNICODE_STRING) != Size)
    {
      return(STATUS_INFO_LENGTH_MISMATCH);
    }

  return(LdrpLoadAndCallImage(Slci));
}

/* Class 39 - Priority Separation */
SSI_DEF(SystemPrioritySeperation)
{
	/* FIXME */
	DPRINT1("NtSetSystemInformation - SystemPrioritySeperation not implemented\n");
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 40 - Plug Play Bus Information */
QSI_DEF(SystemPlugPlayBusInformation)
{
	/* FIXME */
	DPRINT1("NtQuerySystemInformation - SystemPlugPlayBusInformation not implemented\n");
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 41 - Dock Information */
QSI_DEF(SystemDockInformation)
{
	/* FIXME */
	DPRINT1("NtQuerySystemInformation - SystemDockInformation not implemented\n");
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 42 - Power Information */
QSI_DEF(SystemPowerInformation)
{
	/* FIXME */
	DPRINT1("NtQuerySystemInformation - SystemPowerInformation not implemented\n");
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 43 - Processor Speed Information */
QSI_DEF(SystemProcessorSpeedInformation)
{
	/* FIXME */
	DPRINT1("NtQuerySystemInformation - SystemProcessorSpeedInformation not implemented\n");
	return (STATUS_NOT_IMPLEMENTED);
}

/* Class 44 - Current Time Zone Information */
QSI_DEF(SystemCurrentTimeZoneInformation)
{
  * ReqSize = sizeof (TIME_ZONE_INFORMATION);

  if (sizeof (TIME_ZONE_INFORMATION) != Size)
    {
      return STATUS_INFO_LENGTH_MISMATCH;
    }

  /* Copy the time zone information struct */
  memcpy(Buffer,
         &ExpTimeZoneInfo,
         sizeof(TIME_ZONE_INFORMATION));

  return STATUS_SUCCESS;
}


SSI_DEF(SystemCurrentTimeZoneInformation)
{
  /* Check user buffer's size */
  if (Size < sizeof (TIME_ZONE_INFORMATION))
    {
      return STATUS_INFO_LENGTH_MISMATCH;
    }

  return ExpSetTimeZoneInformation((PTIME_ZONE_INFORMATION)Buffer);
}


/* Class 45 - Lookaside Information */
QSI_DEF(SystemLookasideInformation)
{
	/* FIXME */
	DPRINT1("NtQuerySystemInformation - SystemLookasideInformation not implemented\n");
	return (STATUS_NOT_IMPLEMENTED);
}


/* Class 46 - Set time slip event */
SSI_DEF(SystemSetTimeSlipEvent)
{
	/* FIXME */
	DPRINT1("NtSetSystemInformation - SystemSetTimSlipEvent not implemented\n");
	return (STATUS_NOT_IMPLEMENTED);
}


/* Class 47 - Create a new session (TSE) */
SSI_DEF(SystemCreateSession)
{
	/* FIXME */
	DPRINT1("NtSetSystemInformation - SystemCreateSession not implemented\n");
	return (STATUS_NOT_IMPLEMENTED);
}


/* Class 48 - Delete an existing session (TSE) */
SSI_DEF(SystemDeleteSession)
{
	/* FIXME */
	DPRINT1("NtSetSystemInformation - SystemDeleteSession not implemented\n");
	return (STATUS_NOT_IMPLEMENTED);
}


/* Class 49 - UNKNOWN */
QSI_DEF(SystemInvalidInfoClass4)
{
	/* FIXME */
	DPRINT1("NtQuerySystemInformation - SystemInvalidInfoClass4 not implemented\n");
	return (STATUS_NOT_IMPLEMENTED);
}


/* Class 50 - System range start address */
QSI_DEF(SystemRangeStartInformation)
{
	/* FIXME */
	DPRINT1("NtQuerySystemInformation - SystemRangeStartInformation not implemented\n");
	return (STATUS_NOT_IMPLEMENTED);
}


/* Class 51 - Driver verifier information */
QSI_DEF(SystemVerifierInformation)
{
	/* FIXME */
	DPRINT1("NtQuerySystemInformation - SystemVerifierInformation not implemented\n");
	return (STATUS_NOT_IMPLEMENTED);
}


SSI_DEF(SystemVerifierInformation)
{
	/* FIXME */
	DPRINT1("NtSetSystemInformation - SystemVerifierInformation not implemented\n");
	return (STATUS_NOT_IMPLEMENTED);
}


/* Class 52 - Add a driver verifier */
SSI_DEF(SystemAddVerifier)
{
	/* FIXME */
	DPRINT1("NtSetSystemInformation - SystemAddVerifier not implemented\n");
	return (STATUS_NOT_IMPLEMENTED);
}


/* Class 53 - A session's processes  */
QSI_DEF(SystemSessionProcessesInformation)
{
	/* FIXME */
	DPRINT1("NtQuerySystemInformation - SystemSessionProcessInformation not implemented\n");
	return (STATUS_NOT_IMPLEMENTED);
}


/* Query/Set Calls Table */
typedef
struct _QSSI_CALLS
{
	NTSTATUS (* Query) (PVOID,ULONG,PULONG);
	NTSTATUS (* Set) (PVOID,ULONG);

} QSSI_CALLS;

// QS	Query & Set
// QX	Query
// XS	Set
// XX	unknown behaviour
//
#define SI_QS(n) {QSI_USE(n),SSI_USE(n)}
#define SI_QX(n) {QSI_USE(n),NULL}
#define SI_XS(n) {NULL,SSI_USE(n)}
#define SI_XX(n) {NULL,NULL}

static
QSSI_CALLS
CallQS [] =
{
	SI_QX(SystemBasicInformation),
	SI_QX(SystemProcessorInformation),
	SI_QX(SystemPerformanceInformation),
	SI_QX(SystemTimeOfDayInformation),
	SI_QX(SystemPathInformation), /* should be SI_XX */
	SI_QX(SystemProcessInformation),
	SI_QX(SystemCallCountInformation),
	SI_QX(SystemDeviceInformation),
	SI_QX(SystemProcessorPerformanceInformation),
	SI_QS(SystemFlagsInformation),
	SI_QX(SystemCallTimeInformation), /* should be SI_XX */
	SI_QX(SystemModuleInformation),
	SI_QX(SystemLocksInformation),
	SI_QX(SystemStackTraceInformation), /* should be SI_XX */
	SI_QX(SystemPagedPoolInformation), /* should be SI_XX */
	SI_QX(SystemNonPagedPoolInformation), /* should be SI_XX */
	SI_QX(SystemHandleInformation),
	SI_QX(SystemObjectInformation),
	SI_QX(SystemPageFileInformation),
	SI_QX(SystemVdmInstemulInformation),
	SI_QX(SystemVdmBopInformation), /* it should be SI_XX */
	SI_QS(SystemFileCacheInformation),
	SI_QX(SystemPoolTagInformation),
	SI_QX(SystemInterruptInformation),
	SI_QS(SystemDpcBehaviourInformation),
	SI_QX(SystemFullMemoryInformation), /* it should be SI_XX */
	SI_XS(SystemLoadImage),
	SI_XS(SystemUnloadImage),
	SI_QS(SystemTimeAdjustmentInformation),
	SI_QX(SystemSummaryMemoryInformation), /* it should be SI_XX */
	SI_QX(SystemNextEventIdInformation), /* it should be SI_XX */
	SI_QX(SystemEventIdsInformation), /* it should be SI_XX */
	SI_QX(SystemCrashDumpInformation),
	SI_QX(SystemExceptionInformation),
	SI_QX(SystemCrashDumpStateInformation),
	SI_QX(SystemKernelDebuggerInformation),
	SI_QX(SystemContextSwitchInformation),
	SI_QS(SystemRegistryQuotaInformation),
	SI_XS(SystemLoadAndCallImage),
	SI_XS(SystemPrioritySeperation),
	SI_QX(SystemPlugPlayBusInformation), /* it should be SI_XX */
	SI_QX(SystemDockInformation), /* it should be SI_XX */
	SI_QX(SystemPowerInformation), /* it should be SI_XX */
	SI_QX(SystemProcessorSpeedInformation), /* it should be SI_XX */
	SI_QS(SystemCurrentTimeZoneInformation), /* it should be SI_QX */
	SI_QX(SystemLookasideInformation),
	SI_XS(SystemSetTimeSlipEvent),
	SI_XS(SystemCreateSession),
	SI_XS(SystemDeleteSession),
	SI_QX(SystemInvalidInfoClass4), /* it should be SI_XX */
	SI_QX(SystemRangeStartInformation),
	SI_QS(SystemVerifierInformation),
	SI_XS(SystemAddVerifier),
	SI_QX(SystemSessionProcessesInformation)
};


/*
 * @implemented
 */
NTSTATUS STDCALL
NtQuerySystemInformation (IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
			  OUT PVOID SystemInformation,
			  IN ULONG Length,
			  OUT PULONG UnsafeResultLength)
{
  KPROCESSOR_MODE PreviousMode;
  ULONG ResultLength;
  NTSTATUS FStatus = STATUS_NOT_IMPLEMENTED;

  PAGED_CODE();
  
  PreviousMode = ExGetPreviousMode();
  
  _SEH_TRY
    {
      if (PreviousMode != KernelMode)
        {
          /* SystemKernelDebuggerInformation needs only BOOLEAN alignment */
          ProbeForWrite(SystemInformation, Length, 1); 
          if (UnsafeResultLength != NULL)
            ProbeForWriteUlong(UnsafeResultLength);
        }

      /* Clear user buffer. */
      RtlZeroMemory(SystemInformation, Length);

      /*
       * Check the request is valid.
       */
      if (SystemInformationClass >= MaxSystemInfoClass)
        {
          return (STATUS_INVALID_INFO_CLASS);
        }

      if (NULL != CallQS [SystemInformationClass].Query)
	{
	  /*
	   * Hand the request to a subhandler.
	   */
	  FStatus = CallQS [SystemInformationClass].Query(SystemInformation,
							  Length,
							  &ResultLength);
	  if (UnsafeResultLength != NULL)
	    {
              if (PreviousMode != KernelMode)
                {
                  _SEH_TRY
                    {
                      *UnsafeResultLength = ResultLength;
                    }
                  _SEH_EXCEPT(_SEH_ExSystemExceptionFilter)
                    {
                      FStatus = _SEH_GetExceptionCode();
                    }
                  _SEH_END;
                }
              else
                {
                  *UnsafeResultLength = ResultLength;
                }
	    }
	}
    }
  _SEH_EXCEPT(_SEH_ExSystemExceptionFilter)
    {
      FStatus = _SEH_GetExceptionCode();
    }
  _SEH_END;

  return (FStatus);
}


NTSTATUS
STDCALL
NtSetSystemInformation (
	IN	SYSTEM_INFORMATION_CLASS	SystemInformationClass,
	IN	PVOID				SystemInformation,
	IN	ULONG				SystemInformationLength
	)
{
        PAGED_CODE();

	/*
	 * If called from user mode, check
	 * possible unsafe arguments.
	 */
#if 0
        if (KernelMode != KeGetPreviousMode())
        {
		// Check arguments
		//ProbeForWrite(
		//	SystemInformation,
		//	Length
		//	);
		//ProbeForWrite(
		//	ResultLength,
		//	sizeof (ULONG)
		//	);
        }
#endif
	/*
	 * Check the request is valid.
	 */
	if (	(SystemInformationClass >= SystemBasicInformation)
		&& (SystemInformationClass < MaxSystemInfoClass)
		)
	{
		if (NULL != CallQS [SystemInformationClass].Set)
		{
			/*
			 * Hand the request to a subhandler.
			 */
			return CallQS [SystemInformationClass].Set (
					SystemInformation,
					SystemInformationLength
					);
		}
	}
	return (STATUS_INVALID_INFO_CLASS);
}


NTSTATUS
STDCALL
NtFlushInstructionCache (
	IN	HANDLE	ProcessHandle,
	IN	PVOID	BaseAddress,
	IN	ULONG	NumberOfBytesToFlush
	)
{
        PAGED_CODE();

	__asm__("wbinvd\n");
	return STATUS_SUCCESS;
}


/* EOF */
