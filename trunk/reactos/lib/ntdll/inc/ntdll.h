/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS System Libraries
 * FILE:            lib/ntdll/inc/ntdll.h
 * PURPOSE:         Native Libary Header
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

/* We're a core NT DLL, we don't import syscalls */
#define _NTSYSTEM_
#define _NTDLLBUILD_

/* C Headers */
#define _CTYPE_DISABLE_MACROS
#define _INC_SWPRINTF_INL_
#include <limits.h>
#include <stdio.h>
#include <ctype.h>

/* SDK/DDK/NDK Headers. */
#include <windows.h>
#define NTOS_MODE_USER
#include <ndk/ntndk.h>

/* ELF Support */
#include <elf/elf.h>

/* Internal NTDLL */
#include "ntdllp.h"

/* CSRSS Header */
#include <csrss/csrss.h>

/* Helper Macros */
#include <reactos/helper.h>

/* EOF */
