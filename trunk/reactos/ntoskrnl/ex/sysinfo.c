/* $Id: sysinfo.c,v 1.50 2004/10/08 20:02:30 gvg Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ex/sysinfo.c
 * PURPOSE:         System information functions
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 *                  20/03/2003: implemented querying SystemProcessInformation,
 *                              no more copying to-from the caller (Aleksey
 *                              Bragin <aleksey@studiocerebral.com>)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

extern ULONG NtGlobalFlag; /* FIXME: it should go in a ddk/?.h */
ULONGLONG STDCALL KeQueryInterruptTime(VOID);

VOID MmPrintMemoryStatistic(VOID);

extern ULONG Ke386CpuidFlags;
extern ULONG Ke386Cpuid;

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
 * @unimplemented
 */
VOID
STDCALL
ExGetCurrentProcessorCounts (
	PVOID	IdleThreadTime,
	PVOID	SystemTime,
	PVOID	Number
	)
{
	UNIMPLEMENTED;
}
/*
 * @unimplemented
 */
VOID
STDCALL
ExGetCurrentProcessorCpuUsage (
	PVOID	RetVal
	)
{
	UNIMPLEMENTED;
}

NTSTATUS STDCALL
NtQuerySystemEnvironmentValue (IN	PUNICODE_STRING	UnsafeName,
			       OUT	PVOID		UnsafeValue,
			       IN	ULONG		Length,
			       IN OUT	PULONG		UnsafeReturnLength)
{
  NTSTATUS Status;
  ANSI_STRING AName;
  UNICODE_STRING WName;
  BOOLEAN Result;
  PCH Value;
  ANSI_STRING AValue;
  UNICODE_STRING WValue;
  ULONG ReturnLength;

  /*
   * Copy the name to kernel space if necessary and convert it to ANSI.
   */
  if (ExGetPreviousMode() != KernelMode)
    {
      Status = RtlCaptureUnicodeString(&WName, UnsafeName);
      if (!NT_SUCCESS(Status))
	{
	  return(Status);
	}
      Status = RtlUnicodeStringToAnsiString(&AName, UnsafeName, TRUE);
      if (!NT_SUCCESS(Status))
	{
	  return(Status);
	}
    }
  else
    {
      Status = RtlUnicodeStringToAnsiString(&AName, UnsafeName, TRUE);
      if (!NT_SUCCESS(Status))
	{
	  return(Status);
	}
    }

  /*
   * Create a temporary buffer for the value
   */
  Value = ExAllocatePool(NonPagedPool, Length);
  if (Value == NULL)
    {
      RtlFreeAnsiString(&AName);
      if (ExGetPreviousMode() != KernelMode)
	{
	  RtlFreeUnicodeString(&WName);
	}
      return(STATUS_NO_MEMORY);
    }

  /*
   * Get the environment variable
   */
  Result = HalGetEnvironmentVariable(AName.Buffer, Value, Length);
  if (!Result)
    {
      RtlFreeAnsiString(&AName);
      if (ExGetPreviousMode() != KernelMode)
	{
	  RtlFreeUnicodeString(&WName);
	}
      ExFreePool(Value);
      return(STATUS_UNSUCCESSFUL);
    }

  /*
   * Convert the result to UNICODE.
   */
  RtlInitAnsiString(&AValue, Value);
  Status = RtlAnsiStringToUnicodeString(&WValue, &AValue, TRUE);
  if (!NT_SUCCESS(Status))
    {
      RtlFreeAnsiString(&AName);
      if (ExGetPreviousMode() != KernelMode)
	{
	  RtlFreeUnicodeString(&WName);
	}
      ExFreePool(Value);
      return(Status);
    }
  ReturnLength = WValue.Length;

  /*
   * Copy the result back to the caller.
   */
  if (ExGetPreviousMode() != KernelMode)
    {
      Status = MmCopyToCaller(UnsafeValue, WValue.Buffer, ReturnLength);
      if (!NT_SUCCESS(Status))
	{
	  RtlFreeAnsiString(&AName);
	  if (ExGetPreviousMode() != KernelMode)
	    {
	      RtlFreeUnicodeString(&WName);
	    }
	  ExFreePool(Value);
	  RtlFreeUnicodeString(&WValue);
	  return(Status);
	}

      Status = MmCopyToCaller(UnsafeReturnLength, &ReturnLength, 
			      sizeof(ULONG));
      if (!NT_SUCCESS(Status))
	{
	  RtlFreeAnsiString(&AName);
	  if (ExGetPreviousMode() != KernelMode)
	    {
	      RtlFreeUnicodeString(&WName);
	    }
	  ExFreePool(Value);
	  RtlFreeUnicodeString(&WValue);
	  return(Status);
	}
    }
  else
    {
      memcpy(UnsafeValue, WValue.Buffer, ReturnLength);
      memcpy(UnsafeReturnLength, &ReturnLength, sizeof(ULONG));
    }

  /*
   * Free temporary buffers.
   */
  RtlFreeAnsiString(&AName);
  if (ExGetPreviousMode() != KernelMode)
    {
      RtlFreeUnicodeString(&WName);
    }
  ExFreePool(Value);
  RtlFreeUnicodeString(&WValue);

  return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
NtSetSystemEnvironmentValue (IN	PUNICODE_STRING	UnsafeName,
			     IN	PUNICODE_STRING	UnsafeValue)
{
  UNICODE_STRING WName;
  ANSI_STRING AName;
  UNICODE_STRING WValue;
  ANSI_STRING AValue;
  BOOLEAN Result;
  NTSTATUS Status;

  /*
   * Check for required privilege.
   */
  /* FIXME: Not implemented. */

  /*
   * Copy the name to kernel space if necessary and convert it to ANSI.
   */
  if (ExGetPreviousMode() != KernelMode)
    {
      Status = RtlCaptureUnicodeString(&WName, UnsafeName);
      if (!NT_SUCCESS(Status))
	{
	  return(Status);
	}
      Status = RtlUnicodeStringToAnsiString(&AName, UnsafeName, TRUE);
      if (!NT_SUCCESS(Status))
	{
	  return(Status);
	}      
    }
  else
    {
      Status = RtlUnicodeStringToAnsiString(&AName, UnsafeName, TRUE);
      if (!NT_SUCCESS(Status))
	{
	  return(Status);
	}
    }

  /*
   * Copy the value to kernel space and convert to ANSI.
   */
  if (ExGetPreviousMode() != KernelMode)
    {
      Status = RtlCaptureUnicodeString(&WValue, UnsafeValue);
      if (!NT_SUCCESS(Status))
	{
	  RtlFreeUnicodeString(&WName);
	  RtlFreeAnsiString(&AName);
	  return(Status);
	}
      Status = RtlUnicodeStringToAnsiString(&AValue, UnsafeValue, TRUE);
      if (!NT_SUCCESS(Status))
	{
	  RtlFreeUnicodeString(&WName);
	  RtlFreeAnsiString(&AName);
	  RtlFreeUnicodeString(&WValue);
	  return(Status);
	}      
    }
  else
    {
      Status = RtlUnicodeStringToAnsiString(&AValue, UnsafeValue, TRUE);
      if (!NT_SUCCESS(Status))
	{
	  RtlFreeAnsiString(&AName);
	  return(Status);
	}
    }

  /*
   * Set the environment variable
   */
  Result = HalSetEnvironmentVariable(AName.Buffer, AValue.Buffer);

  /*
   * Free everything and return status.
   */
  RtlFreeAnsiString(&AName);
  RtlFreeAnsiString(&AValue);
  if (ExGetPreviousMode() != KernelMode)
    {
      RtlFreeUnicodeString(&WName);
      RtlFreeUnicodeString(&WValue);
    }

  if (!Result)
    {
      return(STATUS_UNSUCCESSFUL);
    }
  return(STATUS_SUCCESS);
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
	if (Size < sizeof (SYSTEM_BASIC_INFORMATION))
	{
		return (STATUS_INFO_LENGTH_MISMATCH);
	}
	Sbi->Unknown = 0;
	Sbi->MaximumIncrement = 100000; /* FIXME */
	Sbi->PhysicalPageSize = PAGE_SIZE; /* FIXME: it should be PAGE_SIZE */
	Sbi->NumberOfPhysicalPages = MmStats.NrTotalPages;
	Sbi->LowestPhysicalPage = 0; /* FIXME */ 
	Sbi->HighestPhysicalPage = MmStats.NrTotalPages; /* FIXME */
	Sbi->AllocationGranularity = MM_VIRTMEM_GRANULARITY; /* hard coded on Intel? */
	Sbi->LowestUserAddress = 0x10000; /* Top of 64k */
	Sbi->HighestUserAddress = (ULONG_PTR)MmHighestUserAddress;
	Sbi->ActiveProcessors = 0x00000001; /* FIXME */
	Sbi->NumberProcessors = KeNumberProcessors;
	return (STATUS_SUCCESS);
}

/* Class 1 - Processor Information */
QSI_DEF(SystemProcessorInformation)
{
	PSYSTEM_PROCESSOR_INFORMATION Spi 
		= (PSYSTEM_PROCESSOR_INFORMATION) Buffer;

	*ReqSize = sizeof (SYSTEM_PROCESSOR_INFORMATION);
	/*
	 * Check user buffer's size 
	 */
	if (Size < sizeof (SYSTEM_PROCESSOR_INFORMATION))
	{
		return (STATUS_INFO_LENGTH_MISMATCH);
	}	
	Spi->ProcessorArchitecture = 0; /* Intel Processor */
	Spi->ProcessorLevel	   = ((Ke386Cpuid >> 8) & 0xf);
	Spi->ProcessorRevision	   = (Ke386Cpuid & 0xf) | ((Ke386Cpuid << 4) & 0xf00);
	Spi->Unknown 		   = 0;
	Spi->FeatureBits	   = Ke386CpuidFlags;

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
	
	PsLookupProcessByProcessId((PVOID) 1, &TheIdleProcess);
	
	Spi->IdleTime.QuadPart = TheIdleProcess->Pcb.KernelTime * 100000LL;

	Spi->ReadTransferCount.QuadPart = IoReadTransferCount;
	Spi->WriteTransferCount.QuadPart = IoWriteTransferCount;
	Spi->OtherTransferCount.QuadPart = IoOtherTransferCount;
	Spi->ReadOperationCount = IoReadOperationCount;
	Spi->WriteOperationCount = IoWriteOperationCount;
	Spi->OtherOperationCount = IoOtherOperationCount;

	Spi->AvailablePages = MmStats.NrFreePages;
/*
        Add up all the used "Commitied" memory + pagefile.
        Not sure this is right. 8^\
 */
	Spi->TotalCommittedPages = MiMemoryConsumers[MC_PPOOL].PagesUsed +
				   MiMemoryConsumers[MC_NPPOOL].PagesUsed+
                                   MiMemoryConsumers[MC_CACHE].PagesUsed+
		                   MiMemoryConsumers[MC_USER].PagesUsed+
			           MiUsedSwapPages;
/*
	Add up the full system total + pagefile.
	All this make Taskmgr happy but not sure it is the right numbers.
	This too, fixes some of GlobalMemoryStatusEx numbers.
*/
        Spi->TotalCommitLimit = MmStats.NrTotalPages + MiFreeSwapPages +
                                MiUsedSwapPages;

	Spi->PeakCommitment = 0; /* FIXME */
	Spi->PageFaults = 0; /* FIXME */
	Spi->WriteCopyFaults = 0; /* FIXME */
	Spi->TransitionFaults = 0; /* FIXME */
	Spi->CacheTransitionFaults = 0; /* FIXME */
	Spi->DemandZeroFaults = 0; /* FIXME */
	Spi->PagesRead = 0; /* FIXME */
	Spi->PageReadIos = 0; /* FIXME */
	Spi->CacheReads = 0; /* FIXME */
	Spi->CacheIos = 0; /* FIXME */
	Spi->PagefilePagesWritten = 0; /* FIXME */
	Spi->PagefilePageWriteIos = 0; /* FIXME */
	Spi->MappedFilePagesWritten = 0; /* FIXME */
	Spi->MappedFilePageWriteIos = 0; /* FIXME */

	Spi->PagedPoolUsage = MiMemoryConsumers[MC_PPOOL].PagesUsed;
	Spi->PagedPoolAllocs = 0; /* FIXME */
	Spi->PagedPoolFrees = 0; /* FIXME */
	Spi->NonPagedPoolUsage = MiMemoryConsumers[MC_NPPOOL].PagesUsed;
	Spi->NonPagedPoolAllocs = 0; /* FIXME */
	Spi->NonPagedPoolFrees = 0; /* FIXME */

	Spi->TotalFreeSystemPtes = 0; /* FIXME */
	
	Spi->SystemCodePage = MmStats.NrSystemPages; /* FIXME */
	
	Spi->TotalSystemDriverPages = 0; /* FIXME */
	Spi->TotalSystemCodePages = 0; /* FIXME */
	Spi->SmallNonPagedLookasideListAllocateHits = 0; /* FIXME */
	Spi->SmallPagedLookasideListAllocateHits = 0; /* FIXME */
	Spi->Reserved3 = 0; /* FIXME */

	Spi->MmSystemCachePage = MiMemoryConsumers[MC_CACHE].PagesUsed;
	Spi->PagedPoolPage = MmPagedPoolSize; /* FIXME */

	Spi->SystemDriverPage = 0; /* FIXME */
	Spi->FastReadNoWait = 0; /* FIXME */
	Spi->FastReadWait = 0; /* FIXME */
	Spi->FastReadResourceMiss = 0; /* FIXME */
	Spi->FastReadNotPossible = 0; /* FIXME */

	Spi->FastMdlReadNoWait = 0; /* FIXME */
	Spi->FastMdlReadWait = 0; /* FIXME */
	Spi->FastMdlReadResourceMiss = 0; /* FIXME */
	Spi->FastMdlReadNotPossible = 0; /* FIXME */

	Spi->MapDataNoWait = 0; /* FIXME */
	Spi->MapDataWait = 0; /* FIXME */
	Spi->MapDataNoWaitMiss = 0; /* FIXME */
	Spi->MapDataWaitMiss = 0; /* FIXME */

	Spi->PinMappedDataCount = 0; /* FIXME */
	Spi->PinReadNoWait = 0; /* FIXME */
	Spi->PinReadWait = 0; /* FIXME */
	Spi->PinReadNoWaitMiss = 0; /* FIXME */
	Spi->PinReadWaitMiss = 0; /* FIXME */
	Spi->CopyReadNoWait = 0; /* FIXME */
	Spi->CopyReadWait = 0; /* FIXME */
	Spi->CopyReadNoWaitMiss = 0; /* FIXME */
	Spi->CopyReadWaitMiss = 0; /* FIXME */

	Spi->MdlReadNoWait = 0; /* FIXME */
	Spi->MdlReadWait = 0; /* FIXME */
	Spi->MdlReadNoWaitMiss = 0; /* FIXME */
	Spi->MdlReadWaitMiss = 0; /* FIXME */
	Spi->ReadAheadIos = 0; /* FIXME */
	Spi->LazyWriteIos = 0; /* FIXME */
	Spi->LazyWritePages = 0; /* FIXME */
	Spi->DataFlushes = 0; /* FIXME */
	Spi->DataPages = 0; /* FIXME */
	Spi->ContextSwitches = 0; /* FIXME */
	Spi->FirstLevelTbFills = 0; /* FIXME */
	Spi->SecondLevelTbFills = 0; /* FIXME */
	Spi->SystemCalls = 0; /* FIXME */

	ObDereferenceObject(TheIdleProcess);

	return (STATUS_SUCCESS);
}

/* Class 3 - Time Of Day Information */
QSI_DEF(SystemTimeOfDayInformation)
{
	LARGE_INTEGER CurrentTime;

	PSYSTEM_TIMEOFDAY_INFORMATION Sti
		= (PSYSTEM_TIMEOFDAY_INFORMATION) Buffer;

	*ReqSize = sizeof (SYSTEM_TIMEOFDAY_INFORMATION);
	/*
	 * Check user buffer's size 
	 */
	if (Size < sizeof (SYSTEM_TIMEOFDAY_INFORMATION))
	{
		return (STATUS_INFO_LENGTH_MISMATCH);
	}

	KeQuerySystemTime(&CurrentTime);

	Sti->BootTime= SystemBootTime;
	Sti->CurrentTime = CurrentTime;
	Sti->TimeZoneBias.QuadPart = _SystemTimeZoneInfo.Bias;
	Sti->TimeZoneId = 0;		/* FIXME */
	Sti->Reserved = 0;		/* FIXME */

	return (STATUS_SUCCESS);
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
	ULONG ovlSize=0, nThreads;
	PEPROCESS pr, syspr;
	unsigned char *pCur;

	/* scan the process list */

	PSYSTEM_PROCESSES Spi
		= (PSYSTEM_PROCESSES) Buffer;

	*ReqSize = sizeof(SYSTEM_PROCESSES);

	if (Size < sizeof(SYSTEM_PROCESSES))
	{
		return (STATUS_INFO_LENGTH_MISMATCH); // in case buffer size is too small
	}
	
	syspr = PsGetNextProcess(NULL);
	pr = syspr;
	pCur = (unsigned char *)Spi;

	do
	{
		PSYSTEM_PROCESSES SpiCur;
		int curSize, i = 0;
		ANSI_STRING	imgName;
		int inLen=32; // image name len in bytes
		PLIST_ENTRY current_entry;
		PETHREAD current;

		SpiCur = (PSYSTEM_PROCESSES)pCur;

		nThreads = PsEnumThreadsByProcess(pr);
		
		// size of the structure for every process
		curSize = sizeof(SYSTEM_PROCESSES)-sizeof(SYSTEM_THREADS)+sizeof(SYSTEM_THREADS)*nThreads;
		ovlSize += curSize+inLen;

		if (ovlSize > Size)
		{
			*ReqSize = ovlSize;
			ObDereferenceObject(pr);

			return (STATUS_INFO_LENGTH_MISMATCH); // in case buffer size is too small
		}

		// fill system information
		SpiCur->NextEntryDelta = curSize+inLen; // relative offset to the beginnnig of the next structure
		SpiCur->ThreadCount = nThreads;
		SpiCur->CreateTime = pr->CreateTime;
		SpiCur->UserTime.QuadPart = pr->Pcb.UserTime * 100000LL; 
		SpiCur->KernelTime.QuadPart = pr->Pcb.KernelTime * 100000LL;
		SpiCur->ProcessName.Length = strlen(pr->ImageFileName) * sizeof(WCHAR);
		SpiCur->ProcessName.MaximumLength = inLen;
		SpiCur->ProcessName.Buffer = (void*)(pCur+curSize);

		// copy name to the end of the struct
		RtlInitAnsiString(&imgName, pr->ImageFileName);
		RtlAnsiStringToUnicodeString(&SpiCur->ProcessName, &imgName, FALSE);

		SpiCur->BasePriority = pr->Pcb.BasePriority;
		SpiCur->ProcessId = pr->UniqueProcessId;
		SpiCur->InheritedFromProcessId = (DWORD)(pr->InheritedFromUniqueProcessId);
		SpiCur->HandleCount = ObpGetHandleCountByHandleTable(&pr->HandleTable);
		SpiCur->VmCounters.PeakVirtualSize = pr->PeakVirtualSize;
		SpiCur->VmCounters.VirtualSize = pr->VirtualSize.QuadPart;
		SpiCur->VmCounters.PageFaultCount = pr->LastFaultCount;
		SpiCur->VmCounters.PeakWorkingSetSize = pr->Vm.PeakWorkingSetSize; // Is this right using ->Vm. here ?
		SpiCur->VmCounters.WorkingSetSize = pr->Vm.WorkingSetSize; // Is this right using ->Vm. here ?
		SpiCur->VmCounters.QuotaPeakPagedPoolUsage =
					pr->QuotaPeakPoolUsage[0];
		SpiCur->VmCounters.QuotaPagedPoolUsage =
					pr->QuotaPoolUsage[0];
		SpiCur->VmCounters.QuotaPeakNonPagedPoolUsage =
					pr->QuotaPeakPoolUsage[1];
		SpiCur->VmCounters.QuotaNonPagedPoolUsage =
					pr->QuotaPoolUsage[1];
		SpiCur->VmCounters.PagefileUsage = pr->PagefileUsage; // FIXME
		SpiCur->VmCounters.PeakPagefileUsage = pr->PeakPagefileUsage;
		// KJK::Hyperion: I don't know what does this mean. VM_COUNTERS
		// doesn't seem to contain any equivalent field
		//SpiCur->TotalPrivateBytes = pr->NumberOfPrivatePages; //FIXME: bytes != pages

          current_entry = pr->ThreadListHead.Flink;
          while (current_entry != &pr->ThreadListHead)
               {
                 current = CONTAINING_RECORD(current_entry, ETHREAD,
                                             ThreadListEntry);

                 SpiCur->Threads[i].KernelTime.QuadPart = current->Tcb.KernelTime * 100000LL;
                 SpiCur->Threads[i].UserTime.QuadPart = current->Tcb.UserTime * 100000LL;
//                 SpiCur->Threads[i].CreateTime = current->CreateTime;
                 SpiCur->Threads[i].WaitTime = current->Tcb.WaitTime;
                 SpiCur->Threads[i].StartAddress = (PVOID) current->StartAddress;
                 SpiCur->Threads[i].ClientId = current->Cid;
                 SpiCur->Threads[i].Priority = current->Tcb.Priority;
                 SpiCur->Threads[i].BasePriority = current->Tcb.BasePriority;
                 SpiCur->Threads[i].ContextSwitchCount = current->Tcb.ContextSwitches;
                 SpiCur->Threads[i].State = current->Tcb.State;
                 SpiCur->Threads[i].WaitReason = current->Tcb.WaitReason;
                 i++;
                 current_entry = current_entry->Flink;
               }

		pr = PsGetNextProcess(pr);

		if ((pr == syspr) || (pr == NULL))
		{
			SpiCur->NextEntryDelta = 0;
			break;
		}
		else
			pCur = pCur + curSize + inLen;
	}  while ((pr != syspr) && (pr != NULL));

	*ReqSize = ovlSize;
	if (pr != NULL)
	  {
	    ObDereferenceObject(pr);
	  }
	return (STATUS_SUCCESS);
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
	PSYSTEM_PROCESSORTIME_INFO Spi
		= (PSYSTEM_PROCESSORTIME_INFO) Buffer;

        PEPROCESS TheIdleProcess;
	TIME CurrentTime;

	*ReqSize = sizeof (SYSTEM_PROCESSORTIME_INFO);
	/*
	 * Check user buffer's size 
	 */
	if (Size < sizeof (SYSTEM_PROCESSORTIME_INFO))
	{
		return (STATUS_INFO_LENGTH_MISMATCH);
	}

        PsLookupProcessByProcessId((PVOID) 1, &TheIdleProcess);

	CurrentTime.QuadPart = KeQueryInterruptTime();

        Spi->TotalProcessorRunTime.QuadPart = 
		TheIdleProcess->Pcb.KernelTime * 100000LL; // IdleTime
        Spi->TotalProcessorTime.QuadPart =  KiKernelTime * 100000LL; // KernelTime
        Spi->TotalProcessorUserTime.QuadPart = KiUserTime * 100000LL;
        Spi->TotalDPCTime.QuadPart = KiDpcTime * 100000LL;
        Spi->TotalInterruptTime.QuadPart = KiInterruptTime * 100000LL;
        Spi->TotalInterrupts = KiInterruptCount; // Interrupt Count

	ObDereferenceObject(TheIdleProcess);
        
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
	/* FIXME */
	DPRINT1("NtQuerySystemInformation - SystemHandleInformation not implemented\n");
	return (STATUS_NOT_IMPLEMENTED);
}

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
	SYSTEM_PAGEFILE_INFORMATION *Spfi = (SYSTEM_PAGEFILE_INFORMATION *) Buffer;

	if (Size < sizeof (SYSTEM_PAGEFILE_INFORMATION))
	{
		* ReqSize = sizeof (SYSTEM_PAGEFILE_INFORMATION);
		return (STATUS_INFO_LENGTH_MISMATCH);
	}

	UNICODE_STRING FileName; /* FIXME */

	/* FIXME */
	Spfi->RelativeOffset = 0;

	Spfi->CurrentSizePages = MiFreeSwapPages + MiUsedSwapPages;
	Spfi->TotalUsedPages = MiUsedSwapPages;
	Spfi->PeakUsedPages = MiUsedSwapPages; /* FIXME */
	Spfi->PagefileFileName = FileName;
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
	SYSTEM_CACHE_INFORMATION *Sci = (SYSTEM_CACHE_INFORMATION *) Buffer;

	if (Size < sizeof (SYSTEM_CACHE_INFORMATION))
	{
		* ReqSize = sizeof (SYSTEM_CACHE_INFORMATION);
		return (STATUS_INFO_LENGTH_MISMATCH);
	}
	/* Return the Byte size not the page size. */
	Sci->CurrentSize = 
		MiMemoryConsumers[MC_CACHE].PagesUsed * PAGE_SIZE;
	Sci->PeakSize = 
	        MiMemoryConsumers[MC_CACHE].PagesUsed * PAGE_SIZE; /* FIXME */

	Sci->PageFaultCount = 0; /* FIXME */
	Sci->MinimumWorkingSet = 0; /* FIXME */
	Sci->MaximumWorkingSet = 0; /* FIXME */
	Sci->TransitionSharedPages = 0; /* FIXME */
	Sci->TransitionSharedPagesPeak = 0; /* FIXME */

	return (STATUS_SUCCESS);
}

SSI_DEF(SystemFileCacheInformation)
{
	if (Size < sizeof (SYSTEM_CACHE_INFORMATION))
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

/* Class 23 - Interrupt Information */
QSI_DEF(SystemInterruptInformation)
{
	/* FIXME */
	DPRINT1("NtQuerySystemInformation - SystemInterruptInformation not implemented\n");
	return (STATUS_NOT_IMPLEMENTED);
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

	PsLookupProcessByProcessId((PVOID) 1, &TheIdleProcess);

        DPRINT("PID: %d, KernelTime: %u PFFree: %d PFUsed: %d\n",
               TheIdleProcess->UniqueProcessId,
               TheIdleProcess->Pcb.KernelTime,
               MiFreeSwapPages,
               MiUsedSwapPages);

#ifndef NDEBUG
	MmPrintMemoryStatistic();
#endif
	
	*Spi = MiMemoryConsumers[MC_USER].PagesUsed;

	ObDereferenceObject(TheIdleProcess);

	return (STATUS_SUCCESS);
}

/* Class 26 - Load Image */
SSI_DEF(SystemLoadImage)
{
  PSYSTEM_LOAD_IMAGE Sli = (PSYSTEM_LOAD_IMAGE)Buffer;

  if (sizeof(SYSTEM_LOAD_IMAGE) != Size)
    {
      return(STATUS_INFO_LENGTH_MISMATCH);
    }

  return(LdrpLoadImage(&Sli->ModuleName,
		       &Sli->ModuleBase,
		       &Sli->SectionPointer,
		       &Sli->EntryPoint,
		       &Sli->ExportDirectory));
}

/* Class 27 - Unload Image */
SSI_DEF(SystemUnloadImage)
{
  PSYSTEM_UNLOAD_IMAGE Sui = (PSYSTEM_UNLOAD_IMAGE)Buffer;

  if (sizeof(SYSTEM_UNLOAD_IMAGE) != Size)
    {
      return(STATUS_INFO_LENGTH_MISMATCH);
    }

  return(LdrpUnloadImage(Sui->ModuleBase));
}

/* Class 28 - Time Adjustment Information */
QSI_DEF(SystemTimeAdjustmentInformation)
{
	if (sizeof (SYSTEM_SET_TIME_ADJUSTMENT) > Size)
	{
		* ReqSize = sizeof (SYSTEM_SET_TIME_ADJUSTMENT);
		return (STATUS_INFO_LENGTH_MISMATCH);
	}
	/* FIXME: */
	DPRINT1("NtQuerySystemInformation - SystemTimeAdjustmentInformation not implemented\n");
	return (STATUS_NOT_IMPLEMENTED);
}

SSI_DEF(SystemTimeAdjustmentInformation)
{
	if (sizeof (SYSTEM_SET_TIME_ADJUSTMENT) > Size)
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
	/* FIXME */
	DPRINT1("NtQuerySystemInformation - SystemKernelDebuggerInformation not implemented\n");
	return (STATUS_NOT_IMPLEMENTED);
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
  srqi->Reserved1 = (void*)0x200000;

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
  PSYSTEM_LOAD_AND_CALL_IMAGE Slci = (PSYSTEM_LOAD_AND_CALL_IMAGE)Buffer;

  if (sizeof(SYSTEM_LOAD_AND_CALL_IMAGE) != Size)
    {
      return(STATUS_INFO_LENGTH_MISMATCH);
    }

  return(LdrpLoadAndCallImage(&Slci->ModuleName));
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
		return (STATUS_INFO_LENGTH_MISMATCH);
	}
	/* Copy the time zone information struct */
        memcpy (
		Buffer,
                & _SystemTimeZoneInfo,
                sizeof (TIME_ZONE_INFORMATION)
		);

	return (STATUS_SUCCESS);
}


SSI_DEF(SystemCurrentTimeZoneInformation)
{
	/*
	 * Check user buffer's size 
	 */
	if (Size < sizeof (TIME_ZONE_INFORMATION))
	{
		return (STATUS_INFO_LENGTH_MISMATCH);
	}
	/* Copy the time zone information struct */
	memcpy (
		& _SystemTimeZoneInfo,
		(TIME_ZONE_INFORMATION *) Buffer,
		sizeof (TIME_ZONE_INFORMATION)
		);
	return (STATUS_SUCCESS);
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
			  OUT PVOID UnsafeSystemInformation,
			  IN ULONG Length,
			  OUT PULONG UnsafeResultLength)
{
  ULONG ResultLength;
  PVOID SystemInformation;
  NTSTATUS Status;
  NTSTATUS FStatus;

/*	DPRINT("NtQuerySystemInformation Start. Class:%d\n",
					SystemInformationClass );
*/
  /*if (ExGetPreviousMode() == KernelMode)
    {*/
      SystemInformation = UnsafeSystemInformation;
    /*}
  else
    {
      SystemInformation = ExAllocatePool(NonPagedPool, Length);
      if (SystemInformation == NULL)
	{
	  return(STATUS_NO_MEMORY);
	}
    }*/
  
  /* Clear user buffer. */
  RtlZeroMemory(SystemInformation, Length);

  /*
   * Check the request is valid.
   */
  if ((SystemInformationClass >= SystemInformationClassMin) && 
      (SystemInformationClass < SystemInformationClassMax))
    {
      if (NULL != CallQS [SystemInformationClass].Query)
	{
	  /*
	   * Hand the request to a subhandler.
	   */
	  FStatus = CallQS [SystemInformationClass].Query(SystemInformation,
							  Length,
							  &ResultLength);
	  /*if (ExGetPreviousMode() != KernelMode)
	    {
	      Status = MmCopyToCaller(UnsafeSystemInformation, 
				      SystemInformation,
				      Length);
	      ExFreePool(SystemInformation);
	      if (!NT_SUCCESS(Status))
		{
		  return(Status);
		}
	    }*/
	  if (UnsafeResultLength != NULL)
	    {
	      /*if (ExGetPreviousMode() == KernelMode)
		{
		  *UnsafeResultLength = ResultLength;
		}
	      else
		{*/
		  Status = MmCopyToCaller(UnsafeResultLength,
					  &ResultLength,
					  sizeof(ULONG));
		  if (!NT_SUCCESS(Status))
		    {
		      return(Status);
		    }
		/*}*/
	    }
	  return(FStatus);
	}
    }
  return (STATUS_INVALID_INFO_CLASS);
}


NTSTATUS
STDCALL
NtSetSystemInformation (
	IN	SYSTEM_INFORMATION_CLASS	SystemInformationClass,
	IN	PVOID				SystemInformation,
	IN	ULONG				SystemInformationLength
	)
{
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
	if (	(SystemInformationClass >= SystemInformationClassMin)
		&& (SystemInformationClass < SystemInformationClassMax)
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
	IN	UINT	NumberOfBytesToFlush
	)
{
	UNIMPLEMENTED;
	return(STATUS_NOT_IMPLEMENTED);
}


/* EOF */
