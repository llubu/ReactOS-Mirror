/* $Id: receive.c,v 1.3 2001/12/02 23:34:42 dwelch Exp $
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/lpc/receive.c
 * PURPOSE:         Communication mechanism
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/ob.h>
#include <internal/port.h>
#include <internal/dbg.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

/**********************************************************************
 * NAME							SYSTEM
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 */
NTSTATUS STDCALL
NtReadRequestData (HANDLE		PortHandle,
		   PLPC_MESSAGE	Message,
		   ULONG		Index,
		   PVOID		Buffer,
		   ULONG		BufferLength,
		   PULONG		Returnlength)
{
	UNIMPLEMENTED;
}


/* EOF */
