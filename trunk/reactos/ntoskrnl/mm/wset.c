/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: wset.c,v 1.20 2004/08/01 07:24:58 hbirr Exp $
 * 
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/wset.c
 * PURPOSE:         Manages working sets
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/mm.h>
#include <internal/ps.h>
#include <ntos/minmax.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS
MmTrimUserMemory(ULONG Target, ULONG Priority, PULONG NrFreedPages)
{
   PFN_TYPE CurrentPage;
   PFN_TYPE NextPage;
   NTSTATUS Status;

   (*NrFreedPages) = 0;

   CurrentPage = MmGetLRUFirstUserPage();
   while (CurrentPage != 0 && Target > 0)
   {
      NextPage = MmGetLRUNextUserPage(CurrentPage);

      Status = MmPageOutPhysicalAddress(CurrentPage);
      if (NT_SUCCESS(Status))
      {
         DPRINT("Succeeded\n");
         Target--;
         (*NrFreedPages)++;
      }
      else if (Status == STATUS_PAGEFILE_QUOTA)
      {
         MmSetLRULastPage(CurrentPage);
      }

      CurrentPage = NextPage;
   }
   return(STATUS_SUCCESS);
}

/*
 * @unimplemented
 */
ULONG
STDCALL
MmTrimAllSystemPagableMemory (
	IN ULONG PurgeTransitionList
	)
{
	UNIMPLEMENTED;
	return 0;
}
