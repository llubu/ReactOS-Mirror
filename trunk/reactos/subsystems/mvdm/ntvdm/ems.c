/*
 * COPYRIGHT:       GPLv2+ - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            ems.c
 * PURPOSE:         Expanded Memory Support
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "bios/bios32/bios32p.h"
#include <ndk/rtltypes.h>
#include <ndk/rtlfuncs.h>
#include "ems.h"

/* PRIVATE VARIABLES **********************************************************/

static RTL_BITMAP AllocBitmap;
static ULONG BitmapBuffer[(EMS_TOTAL_PAGES + sizeof(ULONG) - 1) / sizeof(ULONG)];
static EMS_PAGE PageTable[EMS_TOTAL_PAGES];
static EMS_HANDLE HandleTable[EMS_MAX_HANDLES];
static PVOID Mapping[EMS_PHYSICAL_PAGES] = { NULL };

/* PRIVATE FUNCTIONS **********************************************************/

static USHORT EmsFree(USHORT Handle)
{
    PLIST_ENTRY Entry;
    PEMS_HANDLE HandleEntry = &HandleTable[Handle];

    if (Handle >= EMS_MAX_HANDLES || !HandleEntry->Allocated)
    {
        return EMS_STATUS_INVALID_HANDLE;
    }

    for (Entry = HandleEntry->PageList.Flink;
         Entry != &HandleEntry->PageList;
         Entry = Entry->Flink)
    {
        PEMS_PAGE PageEntry = (PEMS_PAGE)CONTAINING_RECORD(Entry, EMS_PAGE, Entry);
        ULONG PageNumber = (ULONG)(((ULONG_PTR)PageEntry - (ULONG_PTR)PageTable) / sizeof(EMS_PAGE));

        /* Free the page */
        RtlClearBits(&AllocBitmap, PageNumber, 1);
    }

    HandleEntry->Allocated = FALSE;
    HandleEntry->PageCount = 0;
    InitializeListHead(&HandleEntry->PageList);

    return EMS_STATUS_OK;
}

static UCHAR EmsAlloc(USHORT NumPages, PUSHORT Handle)
{
    ULONG i, CurrentIndex = 0;
    PEMS_HANDLE HandleEntry;

    if (NumPages == 0) return EMS_STATUS_ZERO_PAGES;

    for (i = 0; i < EMS_MAX_HANDLES; i++)
    {
        HandleEntry = &HandleTable[i];
        if (!HandleEntry->Allocated)
        {
            *Handle = i;
            break;
        }
    }

    if (i == EMS_MAX_HANDLES) return EMS_STATUS_NO_MORE_HANDLES;
    HandleEntry->Allocated = TRUE;

    while (HandleEntry->PageCount < NumPages)
    {
        ULONG RunStart;
        ULONG RunSize = RtlFindNextForwardRunClear(&AllocBitmap, CurrentIndex, &RunStart);

        if (RunSize == 0)
        {
            /* Free what's been allocated already and report failure */
            EmsFree(*Handle);
            return EMS_STATUS_INSUFFICIENT_PAGES;
        }
        else if ((HandleEntry->PageCount + RunSize) > NumPages)
        {
            /* We don't need the entire run */
            RunSize = NumPages - HandleEntry->PageCount;
        }

        CurrentIndex = RunStart + RunSize;
        HandleEntry->PageCount += RunSize;
        RtlSetBits(&AllocBitmap, RunStart, RunSize);

        for (i = 0; i < RunSize; i++)
        {
            PageTable[RunStart + i].Handle = *Handle;
            InsertTailList(&HandleEntry->PageList, &PageTable[RunStart + i].Entry);
        }
    }

    return EMS_STATUS_OK;
}

static USHORT EmsMap(USHORT Handle, UCHAR PhysicalPage, USHORT LogicalPage)
{
    PLIST_ENTRY Entry;
    PEMS_PAGE PageEntry;
    PEMS_HANDLE HandleEntry = &HandleTable[Handle];
    ULONG PageNumber;

    if (PhysicalPage >= EMS_PHYSICAL_PAGES) return EMS_STATUS_INV_PHYSICAL_PAGE;
    if (LogicalPage == 0xFFFF)
    {
        /* Unmap */
        Mapping[PhysicalPage] = NULL;
        return EMS_STATUS_OK;
    }

    if (Handle >= EMS_MAX_HANDLES || !HandleEntry->Allocated) return EMS_STATUS_INVALID_HANDLE;

    Entry = HandleEntry->PageList.Flink;
    while (LogicalPage)
    {
        if (Entry == &HandleEntry->PageList) break;

        LogicalPage--;
        Entry = Entry->Flink;
    }

    if (Entry == &HandleEntry->PageList) return EMS_STATUS_INV_LOGICAL_PAGE; 

    PageEntry = (PEMS_PAGE)CONTAINING_RECORD(Entry, EMS_PAGE, Entry);
    PageNumber = (ULONG)(((ULONG_PTR)PageEntry - (ULONG_PTR)PageTable) / sizeof(EMS_PAGE));
    Mapping[PhysicalPage] = (PVOID)(EMS_ADDRESS + PageNumber * EMS_PAGE_SIZE);

    return EMS_STATUS_OK;
}

static VOID WINAPI EmsIntHandler(LPWORD Stack)
{
    switch (getAH())
    {
        /* Get Manager Status */
        case 0x40:
        {
            setAH(EMS_STATUS_OK);
            break;
        }

        /* Get Page Frame Segment */
        case 0x41:
        {
            setAH(EMS_STATUS_OK);
            setBX(EMS_SEGMENT);
            break;
        }

        /* Get Number Of Pages */
        case 0x42:
        {
            setAH(EMS_STATUS_OK);
            setBX(RtlNumberOfClearBits(&AllocBitmap));
            setDX(EMS_TOTAL_PAGES);
            break;
        }

        /* Get Handle And Allocate Memory */
        case 0x43:
        {
            USHORT Handle;
            UCHAR Status = EmsAlloc(getBX(), &Handle);

            setAH(Status);
            if (Status == EMS_STATUS_OK) setDX(Handle);
            break;
        }

        /* Map Memory */
        case 0x44:
        {
            setAH(EmsMap(getDX(), getAL(), getBX()));
            break;
        }

        /* Release Handle And Memory */
        case 0x45:
        {
            setAH(EmsFree(getDX()));
            break;
        }

        default:
        {
            DPRINT1("EMS function AH = %02X NOT IMPLEMENTED\n", getAH());
            setAH(EMS_STATUS_UNKNOWN_FUNCTION);
            break;
        }
    }
}

/* PUBLIC FUNCTIONS ***********************************************************/

VOID EmsReadMemory(ULONG Address, PVOID Buffer, ULONG Size)
{
    // TODO: NOT IMPLEMENTED
    UNIMPLEMENTED;
}

VOID EmsWriteMemory(ULONG Address, PVOID Buffer, ULONG Size)
{
    // TODO: NOT IMPLEMENTED
    UNIMPLEMENTED;
}

VOID EmsInitialize(VOID)
{
    ULONG i;
    RtlInitializeBitMap(&AllocBitmap, BitmapBuffer, EMS_TOTAL_PAGES);

    for (i = 0; i < EMS_MAX_HANDLES; i++)
    {
        HandleTable[i].Allocated = FALSE;
        HandleTable[i].PageCount = 0;
        InitializeListHead(&HandleTable[i].PageList);
    }

    RegisterBiosInt32(EMS_INTERRUPT_NUM, EmsIntHandler);
}
