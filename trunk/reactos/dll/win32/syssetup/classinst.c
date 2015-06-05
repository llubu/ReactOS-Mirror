/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/syssetup/classinst.c
 * PURPOSE:     Class installers
 * PROGRAMMERS: Copyright 2006 Herv� Poussineau (hpoussin@reactos.org)
 */

#include "precomp.h"

#define NDEBUG
#include <debug.h>

/*
 * @unimplemented
 */
DWORD
WINAPI
ComputerClassInstaller(
    IN DI_FUNCTION InstallFunction,
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL)
{
    switch (InstallFunction)
    {
        default:
            DPRINT1("Install function %u ignored\n", InstallFunction);
            return ERROR_DI_DO_DEFAULT;
    }
}


/*
 * @unimplemented
 */
DWORD
WINAPI
CriticalDeviceCoInstaller(
    IN DI_FUNCTION InstallFunction,
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL,
    IN OUT PCOINSTALLER_CONTEXT_DATA Context)
{
    switch (InstallFunction)
    {
        default:
            DPRINT1("Install function %u ignored\n", InstallFunction);
            return ERROR_SUCCESS;
    }
}


/*
 * @unimplemented
 */
DWORD
WINAPI
DeviceBayClassInstaller(
    IN DI_FUNCTION InstallFunction,
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL)
{
    switch (InstallFunction)
    {
        default:
            DPRINT("Install function %u ignored\n", InstallFunction);
            return ERROR_DI_DO_DEFAULT;
    }
}


/*
 * @unimplemented
 */
DWORD
WINAPI
EisaUpHalCoInstaller(
    IN DI_FUNCTION InstallFunction,
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL,
    IN OUT PCOINSTALLER_CONTEXT_DATA Context)
{
    switch (InstallFunction)
    {
        default:
            DPRINT1("Install function %u ignored\n", InstallFunction);
            return ERROR_SUCCESS;
    }
}


/*
 * @implemented
 */
DWORD
WINAPI
HdcClassInstaller(
    IN DI_FUNCTION InstallFunction,
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL)
{
    DPRINT("HdcClassInstaller()\n");
    return ERROR_DI_DO_DEFAULT;
}


/*
 * @unimplemented
 */
DWORD
WINAPI
KeyboardClassInstaller(
    IN DI_FUNCTION InstallFunction,
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL)
{
    switch (InstallFunction)
    {
        default:
            DPRINT("Install function %u ignored\n", InstallFunction);
            return ERROR_DI_DO_DEFAULT;
    }
}


/*
 * @unimplemented
 */
DWORD
WINAPI
MouseClassInstaller(
    IN DI_FUNCTION InstallFunction,
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL)
{
    switch (InstallFunction)
    {
        default:
            DPRINT("Install function %u ignored\n", InstallFunction);
            return ERROR_DI_DO_DEFAULT;
    }
}


/*
 * @unimplemented
 */
DWORD
WINAPI
NtApmClassInstaller(
    IN DI_FUNCTION InstallFunction,
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL)
{
    switch (InstallFunction)
    {
        default:
            DPRINT("Install function %u ignored\n", InstallFunction);
            return ERROR_DI_DO_DEFAULT;
    }
}


/*
 * @unimplemented
 */
DWORD
WINAPI
ScsiClassInstaller(
    IN DI_FUNCTION InstallFunction,
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL)
{
    switch (InstallFunction)
    {
        default:
            DPRINT("Install function %u ignored\n", InstallFunction);
            return ERROR_DI_DO_DEFAULT;
    }
}


/*
 * @unimplemented
 */
DWORD
WINAPI
StorageCoInstaller(
    IN DI_FUNCTION InstallFunction,
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL,
    IN OUT PCOINSTALLER_CONTEXT_DATA Context)
{
    switch (InstallFunction)
    {
        default:
            DPRINT1("Install function %u ignored\n", InstallFunction);
            return ERROR_SUCCESS;
    }
}


/*
 * @implemented
 */
DWORD
WINAPI
TapeClassInstaller(
    IN DI_FUNCTION InstallFunction,
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL)
{
    DPRINT("TapeClassInstaller()\n");
    return ERROR_DI_DO_DEFAULT;
}


/*
 * @implemented
 */
DWORD
WINAPI
VolumeClassInstaller(
    IN DI_FUNCTION InstallFunction,
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL)
{
    DPRINT("VolumeClassInstaller()\n");
    return ERROR_DI_DO_DEFAULT;
}

/* EOF */
