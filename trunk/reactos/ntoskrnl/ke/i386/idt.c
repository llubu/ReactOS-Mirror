/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/idt.c
 * PURPOSE:         IDT managment
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

IDT_DESCRIPTOR KiIdt[256];

#include <pshpack1.h>

struct
{
  USHORT Length;
  ULONG Base;
} KiIdtDescriptor = {256 * 8, (ULONG)KiIdt};

#include <poppack.h>

/* FUNCTIONS *****************************************************************/


