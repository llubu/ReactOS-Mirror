/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/bug.c
 * PURPOSE:         Bugcheck Support
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, KiInitializeBugCheck)
#endif

/* ROS Internal. Please deprecate */
NTHALAPI
VOID
NTAPI
HalReleaseDisplayOwnership(
    VOID
);

/* GLOBALS *******************************************************************/

LIST_ENTRY BugcheckCallbackListHead;
LIST_ENTRY BugcheckReasonCallbackListHead;
KSPIN_LOCK BugCheckCallbackLock;
ULONG KeBugCheckActive, KeBugCheckOwner;
LONG KeBugCheckOwnerRecursionCount;
PRTL_MESSAGE_RESOURCE_DATA KiBugCodeMessages;
ULONG KeBugCheckCount = 1;
ULONG KiHardwareTrigger;
PUNICODE_STRING KiBugCheckDriver;
ULONG_PTR KiBugCheckData[5];

/* PRIVATE FUNCTIONS *********************************************************/

BOOLEAN
NTAPI
KiRosPrintAddress(PVOID address)
{
    PLIST_ENTRY current_entry;
    PLDR_DATA_TABLE_ENTRY current;
    extern LIST_ENTRY ModuleListHead;
    ULONG_PTR RelativeAddress;
    ULONG i = 0;

    do
    {
        current_entry = ModuleListHead.Flink;

        while (current_entry != &ModuleListHead)
        {
            current = CONTAINING_RECORD(current_entry,
                                        LDR_DATA_TABLE_ENTRY,
                                        InLoadOrderLinks);

            if (address >= (PVOID)current->DllBase &&
                address < (PVOID)((ULONG_PTR)current->DllBase +
                                             current->SizeOfImage))
            {
                RelativeAddress = (ULONG_PTR)address -
                                  (ULONG_PTR)current->DllBase;
                DbgPrint("<%wZ: %x>", &current->FullDllName, RelativeAddress);
                return(TRUE);
            }
            current_entry = current_entry->Flink;
        }
    } while(++i <= 1);

    return(FALSE);
}

VOID
NTAPI
KeRosDumpStackFrames(IN PULONG Frame OPTIONAL,
                     IN ULONG FrameCount OPTIONAL)
{
    ULONG Frames[32];
    ULONG i, Addr;

    /* If the caller didn't ask, assume 32 frames */
    if (!FrameCount) FrameCount = 32;

    /* Get the current frames */
    FrameCount = RtlCaptureStackBackTrace(2, FrameCount, (PVOID*)Frames, NULL);

    /* Now loop them (skip the two. One for the dumper, one for the caller) */
    for (i = 0; i < FrameCount; i++)
    {
        /* Get the EIP */
        Addr = Frames[i];

        /* If we had a custom frame, make sure we've reached it first */
        if ((Frame) && (Frame[1] == Addr))
        {
            Frame = NULL;
        }
        else if (Frame)
        {
            /* Skip this entry */
            continue;
        }

        /* Print it out */
        if (!KeRosPrintAddress((PVOID)Addr)) DbgPrint("<%X>", Addr);

        /* Go to the next frame */
        DbgPrint("\n");
    }

    /* Finish the output */
    DbgPrint("\n");
}

VOID
INIT_FUNCTION
NTAPI
KiInitializeBugCheck(VOID)
{
    PRTL_MESSAGE_RESOURCE_DATA BugCheckData;
    LDR_RESOURCE_INFO ResourceInfo;
    PIMAGE_RESOURCE_DATA_ENTRY ResourceDataEntry;
    NTSTATUS Status;

    /* Cache the Bugcheck Message Strings. Prepare the Lookup Data */
    ResourceInfo.Type = 11;
    ResourceInfo.Name = 1;
    ResourceInfo.Language = 9;

    /* Do the lookup. */
    Status = LdrFindResource_U((PVOID)KERNEL_BASE,
                               &ResourceInfo,
                               RESOURCE_DATA_LEVEL,
                               &ResourceDataEntry);

    /* Make sure it worked */
    if (NT_SUCCESS(Status))
    {
        /* Now actually get a pointer to it */
        Status = LdrAccessResource((PVOID)KERNEL_BASE,
                                   ResourceDataEntry,
                                   (PVOID*)&BugCheckData,
                                   NULL);
        if (NT_SUCCESS(Status)) KiBugCodeMessages = BugCheckData;
    }
}

VOID
NTAPI
KeGetBugMessageText(IN ULONG BugCheckCode,
                    OUT PANSI_STRING OutputString OPTIONAL)
{
    ULONG i;
    ULONG IdOffset;
    ULONG_PTR MessageEntry;
    PCHAR BugCode;

    /* Find the message. This code is based on RtlFindMesssage */
    for (i = 0; i < KiBugCodeMessages->NumberOfBlocks; i++)
    {
        /* Check if the ID Matches */
        if ((BugCheckCode >= KiBugCodeMessages->Blocks[i].LowId) &&
            (BugCheckCode <= KiBugCodeMessages->Blocks[i].HighId))
            {
            /* Get Offset to Entry */
            MessageEntry = KiBugCodeMessages->Blocks[i].OffsetToEntries +
                           (ULONG_PTR)KiBugCodeMessages;
            IdOffset = BugCheckCode - KiBugCodeMessages->Blocks[i].LowId;

            /* Get offset to ID */
            for (i = 0; i < IdOffset; i++)
            {
                /* Advance in the Entries */
                MessageEntry += ((PRTL_MESSAGE_RESOURCE_ENTRY)MessageEntry)->
                                Length;
            }

            /* Get the final Code */
            BugCode = ((PRTL_MESSAGE_RESOURCE_ENTRY)MessageEntry)->Text;
            i = strlen(BugCode);

            /* Return it in the OutputString */
            if (OutputString)
            {
                OutputString->Buffer = BugCode;
                OutputString->Length = i + 1;
                OutputString->MaximumLength = i + 1;
            }
            else 
            {
                /* Direct Output to Screen */
                InbvDisplayString(BugCode);
                InbvDisplayString("\r");
                break;
            }
        }
    }
}

VOID
NTAPI
KiDoBugCheckCallbacks(VOID)
{
    PKBUGCHECK_CALLBACK_RECORD CurrentRecord;
    PLIST_ENTRY ListHead, NextEntry, LastEntry;
    ULONG_PTR Checksum;

    /* First make sure that the list is Initialized... it might not be */
    ListHead = &BugcheckCallbackListHead;
    if ((ListHead->Flink) && (ListHead->Blink))
    {
        /* Loop the list */
        LastEntry = ListHead;
        NextEntry = ListHead->Flink;
        while (NextEntry != ListHead)
        {
            /* Get the reord */
            CurrentRecord = CONTAINING_RECORD(NextEntry,
                                              KBUGCHECK_CALLBACK_RECORD,
                                              Entry);

            /* Validate it */
            if (CurrentRecord->Entry.Blink != LastEntry) return;
            Checksum = (ULONG_PTR)CurrentRecord->CallbackRoutine;
            Checksum += (ULONG_PTR)CurrentRecord->Buffer;
            Checksum += (ULONG_PTR)CurrentRecord->Length;
            Checksum += (ULONG_PTR)CurrentRecord->Component;

            /* Make sure it's inserted and valitdated */
            if ((CurrentRecord->State == BufferInserted) &&
                (CurrentRecord->Checksum == Checksum))
            {
                /* Call the routine */
                CurrentRecord->State = BufferStarted;
                (CurrentRecord->CallbackRoutine)(CurrentRecord->Buffer,
                                                 CurrentRecord->Length);
                CurrentRecord->State = BufferFinished;
            }

            /* Go to the next entry */
            LastEntry = NextEntry;
            NextEntry = NextEntry->Flink;
        }
    }
}

VOID
NTAPI
KiBugCheckDebugBreak(IN ULONG StatusCode)
{
    /* If KDBG isn't connected, freeze the CPU, otherwise, break */
    if (KdDebuggerNotPresent) for (;;) Ke386HaltProcessor();
    DbgBreakPointWithStatus(StatusCode);
}

PVOID
NTAPI
KiPcToFileHeader(IN PVOID Eip,
                 OUT PLDR_DATA_TABLE_ENTRY *LdrEntry,
                 IN BOOLEAN DriversOnly,
                 OUT PBOOLEAN InKernel)
{
    ULONG i = 0;
    PVOID ImageBase, EipBase = NULL;
    PLDR_DATA_TABLE_ENTRY Entry;
    PLIST_ENTRY ListHead, NextEntry;
    extern LIST_ENTRY ModuleListHead;

    /* Assume no */
    *InKernel = FALSE;

    /* Set list pointers and make sure it's valid */
    ListHead = &ModuleListHead;
    NextEntry = ListHead->Flink;
    if (NextEntry)
    {
        /* Start loop */
        while (NextEntry != ListHead)
        {
            /* Increase entry */
            i++;

            /* Check if this is a kernel entry and we only want drivers */
            if ((i <= 2) && (DriversOnly == TRUE))
            {
                /* Skip it */
                NextEntry = NextEntry->Flink;
                continue;
            }

            /* Get the loader entry */
            Entry = CONTAINING_RECORD(NextEntry,
                                      LDR_DATA_TABLE_ENTRY,
                                      InLoadOrderLinks);

            /* Move to the next entry */
            NextEntry = NextEntry->Flink;
            ImageBase = Entry->DllBase;

            /* Check if this is the right one */
            if (((ULONG_PTR)Eip >= (ULONG_PTR)Entry->DllBase) &&
                ((ULONG_PTR)Eip < ((ULONG_PTR)Entry->DllBase + Entry->SizeOfImage)))
            {
                /* Return this entry */
                *LdrEntry = Entry;
                EipBase = ImageBase;

                /* Check if this was a kernel or HAL entry */
                if (i <= 2) *InKernel = TRUE;
                break;
            }
        }
    }

    /* Return the base address */
    return EipBase;
}

PCHAR
NTAPI
KeBugCheckUnicodeToAnsi(IN PUNICODE_STRING Unicode,
                        OUT PCHAR Ansi,
                        IN ULONG Length)
{
    PCHAR p;
    PWCHAR pw;
    ULONG i;

    /* Set length and normalize it */
    i = Unicode->Length / sizeof(WCHAR);
    i = min(i, Length - 1);

    /* Set source and destination, and copy */
    pw = Unicode->Buffer;
    p = Ansi;
    while (i--) *p++ = (CHAR)*pw++;

    /* Null terminate and return */
    *p = ANSI_NULL;
    return Ansi;
}

VOID
NTAPI
KiDumpParameterImages(IN PCHAR Message,
                      IN PULONG_PTR Parameters,
                      IN ULONG ParameterCount,
                      IN PKE_BUGCHECK_UNICODE_TO_ANSI ConversionRoutine)
{
    ULONG i;
    BOOLEAN InSystem;
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    PVOID ImageBase;
    PUNICODE_STRING DriverName;
    CHAR AnsiName[32];
    PIMAGE_NT_HEADERS NtHeader;
    ULONG TimeStamp;
    BOOLEAN FirstRun = TRUE;

    /* Loop parameters */
    for (i = 0; i < ParameterCount; i++)
    {
        /* Get the base for this parameter */
        ImageBase = KiPcToFileHeader((PVOID)Parameters[i],
                                     &LdrEntry,
                                     FALSE,
                                     &InSystem);
        if (!ImageBase)
        {
            /* Driver wasn't found, check for unloaded driver */
            DriverName = NULL; // FIXME: ROS can't
            if (!DriverName) continue;

            /* Convert the driver name */
            ImageBase = (PVOID)Parameters[i];
            ConversionRoutine(DriverName, AnsiName, sizeof(AnsiName));
        }
        else
        {
            /* Get the NT Headers and Timestamp */
            NtHeader = RtlImageNtHeader(LdrEntry->DllBase);
            TimeStamp = NtHeader->FileHeader.TimeDateStamp;

            /* Convert the driver name */
            DriverName = &LdrEntry->BaseDllName;
            ConversionRoutine(&LdrEntry->BaseDllName,
                              AnsiName,
                              sizeof(AnsiName));
        }

        /* Format driver name */
        sprintf(Message,
                "%s**  %12s - Address %p base at %p, DateStamp %08lx\n",
                FirstRun ? "\r\n*":"*",
                AnsiName,
                (PVOID)Parameters[i],
                ImageBase,
                TimeStamp);

        /* Check if we only had one parameter */
        if (ParameterCount <= 1)
        {
            /* Then just save the name */
            KiBugCheckDriver = DriverName;
        }
        else
        {
            /* Otherwise, display the message */
            InbvDisplayString(Message);
        }

        /* Loop again */
        FirstRun = FALSE;
    }
}

VOID
NTAPI
KiDisplayBlueScreen(IN ULONG MessageId,
                    IN BOOLEAN IsHardError,
                    IN PCHAR HardErrCaption OPTIONAL,
                    IN PCHAR HardErrMessage OPTIONAL,
                    IN PCHAR Message)
{
    CHAR AnsiName[75];

    /* FIXMEs: Use inbv to clear, fill and write to screen. */

    /* Check if this is a hard error */
    if (IsHardError)
    {
        /* Display caption and message */
        if (HardErrCaption) InbvDisplayString(HardErrCaption);
        if (HardErrMessage) InbvDisplayString(HardErrMessage);
        return;
    }

    /* Begin the display */
    InbvDisplayString("\r\n");

    /* Print out initial message */
    KeGetBugMessageText(BUGCHECK_MESSAGE_INTRO, NULL);
    InbvDisplayString("\r\n\r\n");

    /* Check if we have a driver */
    if (KiBugCheckDriver)
    {
        /* Print out into to driver name */
        KeGetBugMessageText(BUGCODE_ID_DRIVER, NULL);

        /* Convert and print out driver name */
        KeBugCheckUnicodeToAnsi(KiBugCheckDriver, AnsiName, sizeof(AnsiName));
        InbvDisplayString(" ");
        InbvDisplayString(AnsiName);
        InbvDisplayString("\r\n\r\n");
    }

    /* Check if this is the generic message */
    if (MessageId == BUGCODE_PSS_MESSAGE)
    {
        /* It is, so get the bug code string as well */
        KeGetBugMessageText(KiBugCheckData[0], NULL);
        InbvDisplayString("\r\n\r\n");
    }

    /* Print second introduction message */
    KeGetBugMessageText(PSS_MESSAGE_INTRO, NULL);
    InbvDisplayString("\r\n\r\n");

    /* Get the bug code string */
    KeGetBugMessageText(MessageId, NULL);
    InbvDisplayString("\r\n\r\n");

    /* Print message for technical information */
    KeGetBugMessageText(BUGCHECK_TECH_INFO, NULL);

    /* Show the techincal Data */
    sprintf(AnsiName,
            "\r\n\r\n*** STOP: 0x%08lX (0x%p,0x%p,0x%p,0x%p)\r\n\r\n",
            KiBugCheckData[0],
            (PVOID)KiBugCheckData[1],
            (PVOID)KiBugCheckData[2],
            (PVOID)KiBugCheckData[3],
            (PVOID)KiBugCheckData[4]);
    InbvDisplayString(AnsiName);

    /* Check if we have a driver*/
    if (KiBugCheckDriver)
    {
        /* Display technical driver data */
        InbvDisplayString(Message);
    }
    else
    {
        /* Dump parameter information */
        KiDumpParameterImages(Message,
                              (PVOID)&KiBugCheckData[1],
                              4,
                              KeBugCheckUnicodeToAnsi);
    }
}

VOID
NTAPI
KeBugCheckWithTf(IN ULONG BugCheckCode,
                 IN ULONG_PTR BugCheckParameter1,
                 IN ULONG_PTR BugCheckParameter2,
                 IN ULONG_PTR BugCheckParameter3,
                 IN ULONG_PTR BugCheckParameter4,
                 IN PKTRAP_FRAME TrapFrame)
{
    PKPRCB Prcb = KeGetCurrentPrcb();
    CONTEXT Context;
    ULONG MessageId;
    CHAR AnsiName[128];
    BOOLEAN IsSystem, IsHardError = FALSE;
    PCHAR HardErrCaption = NULL, HardErrMessage = NULL;
    PVOID Eip = NULL, Memory;
    PVOID DriverBase;
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    PULONG_PTR HardErrorParameters;
    KIRQL OldIrql;

    /* Set active bugcheck */
    KeBugCheckActive = TRUE;
    KiBugCheckDriver = NULL;

    /* Check if this is power failure simulation */
    if (BugCheckCode == POWER_FAILURE_SIMULATE)
    {
        /* Call the Callbacks and reboot */;
        KiDoBugCheckCallbacks();
        HalReturnToFirmware(HalRebootRoutine);
    }

    /* Save the IRQL and set hardware trigger */
    Prcb->DebuggerSavedIRQL = KeGetCurrentIrql();
    InterlockedIncrement((PLONG)&KiHardwareTrigger);

    /* Capture the CPU Context */
    RtlCaptureContext(&Prcb->ProcessorState.ContextFrame);
    Context = Prcb->ProcessorState.ContextFrame;

    /* FIXME: Call the Watchdog if it's regsitered */

    /* Check which bugcode this is */
    switch (BugCheckCode)
    {
        /* These bug checks already have detailed messages, keep them */
        case UNEXPECTED_KERNEL_MODE_TRAP:
        case DRIVER_CORRUPTED_EXPOOL:
        case ACPI_BIOS_ERROR:
        case ACPI_BIOS_FATAL_ERROR:
        case THREAD_STUCK_IN_DEVICE_DRIVER:
        case DATA_BUS_ERROR:
        case FAT_FILE_SYSTEM:
        case NO_MORE_SYSTEM_PTES:
        case INACCESSIBLE_BOOT_DEVICE:
        case KMODE_EXCEPTION_NOT_HANDLED:

            /* Keep the same code */
            MessageId = BugCheckCode;
            break;

        /* Check if this is a kernel-mode exception */
        case KERNEL_MODE_EXCEPTION_NOT_HANDLED:

            /* Use the generic text message */
            MessageId = KMODE_EXCEPTION_NOT_HANDLED;

        /* File-system errors */
        case NTFS_FILE_SYSTEM:

            /* Use the generic message for FAT */
            MessageId = FAT_FILE_SYSTEM;

        /* Check if this is a coruption of the Mm's Pool */
        case DRIVER_CORRUPTED_MMPOOL:

            /* Use generic corruption message */
            MessageId = DRIVER_CORRUPTED_EXPOOL;

        /* Check if this is a signature check failure */
        case STATUS_SYSTEM_IMAGE_BAD_SIGNATURE:

            /* Use the generic corruption message */
            MessageId = BUGCODE_PSS_MESSAGE_SIGNATURE;

        /* All other codes */
        default:

            /* Use the default bugcheck message */
            MessageId = BUGCODE_PSS_MESSAGE;
    }

    /* Save bugcheck data */
    KiBugCheckData[0] = BugCheckCode;
    KiBugCheckData[1] = BugCheckParameter1;
    KiBugCheckData[2] = BugCheckParameter2;
    KiBugCheckData[3] = BugCheckParameter3;
    KiBugCheckData[4] = BugCheckParameter4;

    /* Now check what bugcheck this is */
    switch (BugCheckCode)
    {
        /* Invalid access to R/O memory or Unhandled KM Exception */
        case KERNEL_MODE_EXCEPTION_NOT_HANDLED:
        case ATTEMPTED_WRITE_TO_READONLY_MEMORY:
        case ATTEMPTED_EXECUTE_OF_NOEXECUTE_MEMORY:

            /* Check if we have a trap frame */
            if (!TrapFrame)
            {
                /* Use parameter 3 as a trap frame, if it exists */
                if (BugCheckParameter3) TrapFrame = (PVOID)BugCheckParameter3;
            }

            /* Check if we got one now and if we need to get EIP */
            if ((TrapFrame) &&
                (BugCheckCode != KERNEL_MODE_EXCEPTION_NOT_HANDLED))
            {
                /* Get EIP */
                Eip = (PVOID)TrapFrame->Eip;
            }
            break;

        /* Wrong IRQL */
        case IRQL_NOT_LESS_OR_EQUAL:

            /*
             * The NT kernel has 3 special sections:
             * MISYSPTE, POOLMI and POOLCODE. The bug check code can
             * determine in which of these sections this bugcode happened
             * and provide a more detailed analysis. For now, we don't.
             */

            /* Eip is in parameter 4 */
            Eip = (PVOID)BugCheckParameter4;

            /* Get the driver base */
            DriverBase = KiPcToFileHeader(Eip, &LdrEntry, FALSE, &IsSystem);
            if (IsSystem)
            {
                /*
                 * The error happened inside the kernel or HAL.
                 * Get the memory address that was being referenced.
                 */
                Memory = (PVOID)BugCheckParameter1;

                /* Find to which driver it belongs */
                DriverBase = KiPcToFileHeader(Memory,
                                              &LdrEntry,
                                              TRUE,
                                              &IsSystem);
                if (DriverBase)
                {
                    /* Get the driver name and update the bug code */
                    KiBugCheckDriver = &LdrEntry->BaseDllName;
                    KiBugCheckData[0] = DRIVER_PORTION_MUST_BE_NONPAGED;
                }
                else
                {
                    /* Find the driver that unloaded at this address */
                    KiBugCheckDriver = NULL; // FIXME: ROS can't locate

                    /* Check if the cause was an unloaded driver */
                    if (KiBugCheckDriver)
                    {
                        /* Update bug check code */
                        KiBugCheckData[0] =
                            SYSTEM_SCAN_AT_RAISED_IRQL_CAUGHT_IMPROPER_DRIVER_UNLOAD;
                    }
                }
            }
            else
            {
                /* Update the bug check code */
                KiBugCheckData[0] = DRIVER_IRQL_NOT_LESS_OR_EQUAL;
            }

            /* Clear EIP so we don't look it up later */
            Eip = NULL;
            break;

        /* Hard error */
        case FATAL_UNHANDLED_HARD_ERROR:

            /* Copy bug check data from hard error */
            HardErrorParameters = (PULONG_PTR)BugCheckParameter2;
            KiBugCheckData[0] = BugCheckParameter1;
            KiBugCheckData[1] = HardErrorParameters[0];
            KiBugCheckData[2] = HardErrorParameters[1];
            KiBugCheckData[3] = HardErrorParameters[2];
            KiBugCheckData[4] = HardErrorParameters[3];

            /* Remember that this is hard error and set the caption/message */
            IsHardError = TRUE;
            HardErrCaption = (PCHAR)BugCheckParameter3;
            HardErrMessage = (PCHAR)BugCheckParameter4;
            break;

        /* Page fault */
        case PAGE_FAULT_IN_NONPAGED_AREA:

            /* Assume no driver */
            DriverBase = NULL;

            /* Check if we have a trap frame */
            if (!TrapFrame)
            {
                /* We don't, use parameter 3 if possible */
                if (BugCheckParameter3) TrapFrame = (PVOID)BugCheckParameter3;
            }

            /* Check if we have a frame now */
            if (TrapFrame)
            {
                /* Get EIP */
                Eip = (PVOID)TrapFrame->Eip;

                /* Find out if was in the kernel or drivers */
                DriverBase = KiPcToFileHeader(Eip, &LdrEntry, FALSE, &IsSystem);
            }

            /*
             * Now we should check if this happened in:
             * 1) Special Pool 2) Free Special Pool 3) Session Pool
             * and update the bugcheck code appropriately.
             */

            /* Check if we had a driver base */
            if (DriverBase)
            {
                /* Find the driver that unloaded at this address */
                KiBugCheckDriver = NULL; // FIXME: ROS can't locate

                /* Check if the cause was an unloaded driver */
                if (KiBugCheckDriver)
                {
                    KiBugCheckData[0] =
                        DRIVER_UNLOADED_WITHOUT_CANCELLING_PENDING_OPERATIONS;
                }
            }
            break;

        /* Check if the driver forgot to unlock pages */
        case DRIVER_LEFT_LOCKED_PAGES_IN_PROCESS:

            /* EIP is in parameter 1 */
            Eip = (PVOID)BugCheckParameter1;
            break;

        /* Check if the driver consumed too many PTEs */
        case DRIVER_USED_EXCESSIVE_PTES:

            /* Driver base is in parameter 1 */
            DriverBase = (PVOID)BugCheckParameter1;
            /* FIXME: LdrEntry is uninitialized for god's sake!!!
               KiBugCheckDriver = &LdrEntry->BaseDllName; */
            break;

        /* Check if the driver has a stuck thread */
        case THREAD_STUCK_IN_DEVICE_DRIVER:

            /* The name is in Parameter 3 */
            KiBugCheckDriver = (PVOID)BugCheckParameter3;
            break;

        /* Anything else */
        default:
            break;
    }

    /* Do we have a driver name? */
    if (KiBugCheckDriver)
    {
        /* Convert it to ANSI */
        KeBugCheckUnicodeToAnsi(KiBugCheckDriver, AnsiName, sizeof(AnsiName));
    }
    else
    {
        /* Do we have an EIP? */
        if (Eip)
        {
            /* Dump image name */
            KiDumpParameterImages(AnsiName,
                                  (PULONG_PTR)&Eip,
                                  1,
                                  KeBugCheckUnicodeToAnsi);
        }
    }

    /* FIXME: Check if we need to save the context for KD */

    /* Check if a debugger is connected */
    if ((BugCheckCode != MANUALLY_INITIATED_CRASH) && (KdDebuggerEnabled))
    {
        /* Crash on the debugger console */
        DbgPrint("\n*** Fatal System Error: 0x%08lx\n"
                 "                       (0x%p,0x%p,0x%p,0x%p)\n\n",
                 KiBugCheckData[0],
                 KiBugCheckData[1],
                 KiBugCheckData[2],
                 KiBugCheckData[3],
                 KiBugCheckData[4]);

        /* Check if the debugger isn't currently connected */
        if (!KdDebuggerNotPresent)
        {
            /* Check if we have a driver to blame */
            if (KiBugCheckDriver)
            {
                /* Dump it */
                DbgPrint("Driver at fault: %s.\n", AnsiName);
            }

            /* Check if this was a hard error */
            if (IsHardError)
            {
                /* Print caption and message */
                if (HardErrCaption) DbgPrint(HardErrCaption);
                if (HardErrMessage) DbgPrint(HardErrMessage);
            }

            /* Break in the debugger */
            KiBugCheckDebugBreak(DBG_STATUS_BUGCHECK_FIRST);
        }
        else
        {
            /*
             * ROS HACK.
             * Ok, so debugging is enabled, but KDBG isn't there.
             * We'll manually dump the stack for the user.
             */
            KeRosDumpStackFrames(NULL, 0);
        }
    }

    /* Switching back to the blue screen so we print messages on it */
    HalReleaseDisplayOwnership();

    /* Raise IRQL to HIGH_LEVEL */
    Ke386DisableInterrupts();
    KeRaiseIrql(HIGH_LEVEL, &OldIrql);

    /* Unlock the Kernel Adress Space if we own it */
    if (KernelAddressSpaceLock.Owner == KeGetCurrentThread())
    {
        MmUnlockAddressSpace(MmGetKernelAddressSpace());
    }

    /* Avoid recursion */
    if (!InterlockedDecrement((PLONG)&KeBugCheckCount))
    {
        /* Set CPU that is bug checking now */
        KeBugCheckOwner = Prcb->Number;

#ifdef CONFIG_SMP
        /* Freeze the other CPUs */
        for (i = 0; i < KeNumberProcessors; i++) 
        {
            if (i != (LONG)KeGetCurrentProcessorNumber())
            {
                /* Send the IPI and give them one second to catch up */
                KiIpiSendRequest(1 << i, IPI_FREEZE);
                KeStallExecutionProcessor(1000000);
            }
        }
#endif

        /* Display the BSOD */
        KiDisplayBlueScreen(MessageId,
                            IsHardError,
                            HardErrCaption,
                            HardErrMessage,
                            AnsiName);

        /* FIXME: Enable debugger if it was pending */

        /* Print the last line */
        InbvDisplayString("\r\n");

        /* Save the context */
        Prcb->ProcessorState.ContextFrame = Context;

        /* FIXME: Support Triage Dump */

        /* Write the crash dump */
        MmDumpToPagingFile(KiBugCheckData[4],
                           KiBugCheckData[0],
                           KiBugCheckData[1],
                           KiBugCheckData[2],
                           KiBugCheckData[3],
                           TrapFrame);
    }

    /* Increase recursioun count */
    KeBugCheckOwnerRecursionCount++;
    if (KeBugCheckOwnerRecursionCount == 2)
    {
        /* Break in the debugger */
        KiBugCheckDebugBreak(DBG_STATUS_BUGCHECK_SECOND);
    }
    else if (KeBugCheckOwnerRecursionCount > 2)
    {
        /* Halt the CPU */
        for (;;) Ke386HaltProcessor();
    }

    /* Call the Callbacks */
    KiDoBugCheckCallbacks();

    /* FIXME: Call Watchdog if enabled */

    /* Attempt to break in the debugger (otherwise halt CPU) */
    KiBugCheckDebugBreak(DBG_STATUS_BUGCHECK_SECOND);
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
BOOLEAN
NTAPI
KeDeregisterBugCheckCallback(IN PKBUGCHECK_CALLBACK_RECORD CallbackRecord)
{
    KIRQL OldIrql;
    BOOLEAN Status = FALSE;

    /* Raise IRQL to High */
    KeRaiseIrql(HIGH_LEVEL, &OldIrql);

    /* Check the Current State */
    if (CallbackRecord->State == BufferInserted)
    {
        /* Reset state and remove from list */
        CallbackRecord->State = BufferEmpty;
        RemoveEntryList(&CallbackRecord->Entry);
        Status = TRUE;
    }

    /* Lower IRQL and return */
    KeLowerIrql(OldIrql);
    return Status;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
KeDeregisterBugCheckReasonCallback(
    IN PKBUGCHECK_REASON_CALLBACK_RECORD CallbackRecord)
{
    KIRQL OldIrql;
    BOOLEAN Status = FALSE;

    /* Raise IRQL to High */
    KeRaiseIrql(HIGH_LEVEL, &OldIrql);

    /* Check the Current State */
    if (CallbackRecord->State == BufferInserted)
    {
        /* Reset state and remove from list */
        CallbackRecord->State = BufferEmpty;
        RemoveEntryList(&CallbackRecord->Entry);
        Status = TRUE;
    }

    /* Lower IRQL and return */
    KeLowerIrql(OldIrql);
    return Status;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
KeRegisterBugCheckCallback(IN PKBUGCHECK_CALLBACK_RECORD CallbackRecord,
                           IN PKBUGCHECK_CALLBACK_ROUTINE CallbackRoutine,
                           IN PVOID Buffer,
                           IN ULONG Length,
                           IN PUCHAR Component)
{
    KIRQL OldIrql;
    BOOLEAN Status = FALSE;

    /* Raise IRQL to High */
    KeRaiseIrql(HIGH_LEVEL, &OldIrql);

    /* Check the Current State first so we don't double-register */
    if (CallbackRecord->State == BufferEmpty)
    {
        /* Set the Callback Settings and insert into the list */
        CallbackRecord->Length = Length;
        CallbackRecord->Buffer = Buffer;
        CallbackRecord->Component = Component;
        CallbackRecord->CallbackRoutine = CallbackRoutine;
        CallbackRecord->State = BufferInserted;
        InsertTailList(&BugcheckCallbackListHead, &CallbackRecord->Entry);
        Status = TRUE;
    }

    /* Lower IRQL and return */
    KeLowerIrql(OldIrql);
    return Status;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
KeRegisterBugCheckReasonCallback(
    IN PKBUGCHECK_REASON_CALLBACK_RECORD CallbackRecord,
    IN PKBUGCHECK_REASON_CALLBACK_ROUTINE CallbackRoutine,
    IN KBUGCHECK_CALLBACK_REASON Reason,
    IN PUCHAR Component)
{
    KIRQL OldIrql;
    BOOLEAN Status = FALSE;

    /* Raise IRQL to High */
    KeRaiseIrql(HIGH_LEVEL, &OldIrql);

    /* Check the Current State first so we don't double-register */
    if (CallbackRecord->State == BufferEmpty)
    {
        /* Set the Callback Settings and insert into the list */
        CallbackRecord->Component = Component;
        CallbackRecord->CallbackRoutine = CallbackRoutine;
        CallbackRecord->State = BufferInserted;
        CallbackRecord->Reason = Reason;
        InsertTailList(&BugcheckReasonCallbackListHead,
                       &CallbackRecord->Entry);
        Status = TRUE;
    }

    /* Lower IRQL and return */
    KeLowerIrql(OldIrql);
    return Status;
}

/*
 * @implemented
 */
VOID
NTAPI
KeBugCheckEx(IN ULONG BugCheckCode,
             IN ULONG_PTR BugCheckParameter1,
             IN ULONG_PTR BugCheckParameter2,
             IN ULONG_PTR BugCheckParameter3,
             IN ULONG_PTR BugCheckParameter4)
{
    /* Call the internal API */
    KeBugCheckWithTf(BugCheckCode,
                     BugCheckParameter1,
                     BugCheckParameter2,
                     BugCheckParameter3,
                     BugCheckParameter4,
                     NULL);
}

/*
 * @implemented
 */
VOID
NTAPI
KeBugCheck(ULONG BugCheckCode)
{
    /* Call the internal API */
    KeBugCheckWithTf(BugCheckCode, 0, 0, 0, 0, NULL);
}

/* EOF */
