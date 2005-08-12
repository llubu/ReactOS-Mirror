/* $Id$
 *
 * subsys/csr/csrsrv/debug.c - CSR server - debugging management
 * 
 * ReactOS Operating System
 * 
 * --------------------------------------------------------------------
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.LIB. If not, write
 * to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge,
 * MA 02139, USA.  
 *
 * --------------------------------------------------------------------
 */
#include "srv.h"

//#define NDEBUG
#include <debug.h>

/*=====================================================================
 *	PUBLIC API
 *===================================================================*/

NTSTATUS STDCALL CsrDebugProcess (PCSR_PROCESS pCsrProcess)
{
	NTSTATUS Status = STATUS_NOT_IMPLEMENTED;
	
	DPRINT("CSRSRV: %s(%08lx) called\n", __FUNCTION__, pCsrProcess);
	
	return Status;
}

NTSTATUS STDCALL CsrDebugProcessStop (PCSR_PROCESS pCsrProcess)
{
	NTSTATUS Status = STATUS_NOT_IMPLEMENTED;
	
	DPRINT("CSRSRV: %s(%08lx) called\n", __FUNCTION__, pCsrProcess);
	
	return Status;
}

/* EOF */
