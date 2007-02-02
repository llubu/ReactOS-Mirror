/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Hardware Abstraction Layer
 * FILE:            hal/halx86/include/hal.h
 * PURPOSE:         HAL Header
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

/* C Headers */
#include <stdio.h>

/* WDK HAL Compilation hack */
#ifdef _MSC_VER
#include <excpt.h>
#include <ntdef.h>
#undef _NTHAL_
#undef DECLSPEC_IMPORT
#define DECLSPEC_IMPORT
#define __declspec(dllimport)
#endif

/* IFS/DDK/NDK Headers */
#include <ntifs.h>
#include <bugcodes.h>
#include <ntdddisk.h>
#include <arc/arc.h>
#include <iotypes.h>
#include <kefuncs.h>
#include <intrin.h>
#include <halfuncs.h>
#include <iofuncs.h>
#include <ldrtypes.h>
#include <obfuncs.h>

/* Internal kernel headers */
#include "internal/pci.h"
#include "internal/i386/intrin_i.h"

/* Internal HAL Headers */
#include "apic.h"
#include "bus.h"
#include "halirq.h"
#include "haldma.h"
#include "halp.h"
#include "mps.h"
#include "ioapic.h"

/* Helper Header */
#include <reactos/helper.h>

/* EOF */
