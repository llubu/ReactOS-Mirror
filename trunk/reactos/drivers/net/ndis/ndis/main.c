/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NDIS library
 * FILE:        ndis/main.c
 * PURPOSE:     Driver entry point
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */
#include <ndissys.h>
#include <protocol.h>
#include <miniport.h>


#ifdef DBG

/* See debug.h for debug/trace constants */
//DWORD DebugTraceLevel = 0xffffffff;
DWORD DebugTraceLevel = MIN_TRACE;

#endif /* DBG */


VOID MainUnload(
    PDRIVER_OBJECT DriverObject)
/*
 * FUNCTION: Unloads the driver
 * ARGUMENTS:
 *     DriverObject = Pointer to driver object created by the system
 */
{
    NDIS_DbgPrint(MAX_TRACE, ("Leaving.\n"));
}


NTSTATUS
#ifndef _MSC_VER
STDCALL
#endif
DriverEntry(
    PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING RegistryPath)
/*
 * FUNCTION: Main driver entry point
 * ARGUMENTS:
 *     DriverObject = Pointer to a driver object for this driver
 *     RegistryPath = Registry node for configuration parameters
 * RETURNS:
 *     Status of driver initialization
 */
{
    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    InitializeListHead(&ProtocolListHead);
    KeInitializeSpinLock(&ProtocolListLock);

    InitializeListHead(&MiniportListHead);
    KeInitializeSpinLock(&MiniportListLock);

    InitializeListHead(&AdapterListHead);
    KeInitializeSpinLock(&AdapterListLock);

#ifdef _MSC_VER
    DriverObject->DriverUnload = MainUnload;
#else
    DriverObject->DriverUnload = (PDRIVER_UNLOAD)MainUnload;
#endif

    return STATUS_SUCCESS;
}

/* EOF */
