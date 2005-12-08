/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS System Libraries
 * FILE:            lib/rtl/rtl.h
 * PURPOSE:         Run-Time Libary Header
 * PROGRAMMER:      Alex Ionescu
 */

/* INCLUDES ******************************************************************/

/* We're a core NT DLL, we don't import syscalls */
#define WIN32_NO_STATUS
#define _INC_SWPRINTF_INL_

/* C Headers */
#include <stdio.h>

/* PSDK/NDK Headers */
#include <windows.h>
#include <ndk/ntndk.h>

/* Internal RTL header */
#include <reactos/helper.h>
#include "rtlp.h"

#include <pseh/pseh.h>

/* EOF */
