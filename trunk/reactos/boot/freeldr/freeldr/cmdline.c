/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            boot/freeldr/cmdline.c
 * PURPOSE:         FreeLDR Command Line Parsing
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <freeldr.h>

/* GLOBALS ********************************************************************/

CCHAR DefaultOs[256];
CMDLINEINFO CmdLineInfo;

/* FUNCTIONS ******************************************************************/

VOID
CmdLineParse(IN PCHAR CmdLine)
{
    PCHAR End, Setting;
    ULONG Length;

    //
    // Set defaults
    //
    CmdLineInfo.DefaultOperatingSystem = NULL;
    CmdLineInfo.TimeOut = -1;
    
    //
    // Get timeout
    //
    Setting = strstr(CmdLine, "timeout=");
    if (Setting) CmdLineInfo.TimeOut = atoi(Setting +
                                            sizeof("timeout=") +
                                            sizeof(ANSI_NULL));

    //
    // Get default OS
    //
    Setting = strstr(CmdLine, "defaultos=");
    if (Setting)
    {
        //
        // Check if there's more command-line parameters following
        //
        Setting += sizeof("defaultos=") + sizeof(ANSI_NULL);
        End = strstr(Setting, " ");
        if (End) Length = End - Setting; else Length = sizeof(DefaultOs);
        
        //
        // Copy the default OS
        //
        strncpy(DefaultOs, Setting, Length);
        CmdLineInfo.DefaultOperatingSystem = DefaultOs;
    }
    
    //
    // Get ramdisk base address
    //
    Setting = strstr(CmdLine, "rdbase=");
    if (Setting) gRamDiskBase = (PVOID)strtoul(Setting +
                                               sizeof("rdbase=") -
                                               sizeof(ANSI_NULL),
                                               NULL,
                                               0);
    
    //
    // Get ramdisk size
    //
    Setting = strstr(CmdLine, "rdsize=");
    if (Setting) gRamDiskSize = strtoul(Setting +
                                        sizeof("rdsize=") -
                                        sizeof(ANSI_NULL),
                                        NULL,
                                        0);
}

PCCH
CmdLineGetDefaultOS(VOID)
{
    return CmdLineInfo.DefaultOperatingSystem;
}

LONG
CmdLineGetTimeOut(VOID)
{
    return CmdLineInfo.TimeOut;
}
