/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002 ReactOS Team
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
/*
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ldr/init.c
 * PURPOSE:         Loaders for PE executables
 * PROGRAMMERS:     Jean Michault
 *                  Rex Jolliff (rex@lvcablemodem.com)
 * UPDATE HISTORY:
 *   DW   22/05/98  Created
 *   RJJ  10/12/98  Completed image loader function and added hooks for MZ/PE
 *   RJJ  10/12/98  Built driver loader function and added hooks for PE/COFF
 *   RJJ  10/12/98  Rolled in David's code to load COFF drivers
 *   JM   14/12/98  Built initial PE user module loader
 *   RJJ  06/03/99  Moved user PE loader into NTDLL
 *   EA   19990717  LdrGetSystemDirectory()
 *   EK   20000618  Using SystemRoot link instead of LdrGetSystemDirectory()
 *   HYP  20020911  Code to determine smss's path from the registry
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/i386/segment.h>
#include <internal/module.h>
#include <internal/ntoskrnl.h>
#include <internal/ob.h>
#include <internal/ps.h>
#include <internal/ldr.h>
#include <napi/teb.h>

//#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/


static NTSTATUS
LdrpMapProcessImage(PHANDLE SectionHandle,
		    PUNICODE_STRING ImagePath)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  HANDLE FileHandle;
  NTSTATUS Status;

  /* Open image file */
  InitializeObjectAttributes(&ObjectAttributes,
			     ImagePath,
			     0,
			     NULL,
			     NULL);

  DPRINT("Opening image file %S\n", ObjectAttributes.ObjectName->Buffer);
  Status = NtOpenFile(&FileHandle,
		      FILE_ALL_ACCESS,
		      &ObjectAttributes,
		      NULL,
		      0,
		      0);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("NtOpenFile() failed (Status %lx)\n", Status);
      return(Status);
    }

  /* Create a section for the image */
  DPRINT("Creating section\n");
  Status = NtCreateSection(SectionHandle,
			   SECTION_ALL_ACCESS,
			   NULL,
			   NULL,
			   PAGE_READWRITE,
			   SEC_COMMIT | SEC_IMAGE,
			   FileHandle);
  NtClose(FileHandle);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("NtCreateSection() failed (Status %lx)\n", Status);
    }

  return(Status);
}


static NTSTATUS
LdrpCreateProcessEnvironment(HANDLE ProcessHandle,
			     PUNICODE_STRING ImagePath,
			     PVOID* ImageBaseAddress)
{
  NTSTATUS Status;
  ULONG BytesWritten;
  ULONG Offset;

#if 0
  PVOID PpbBase;
  ULONG PpbSize;

  /* create the PPB */
  PpbBase = NULL;
  PpbSize = Ppb->AllocationSize;
  Status = NtAllocateVirtualMemory(ProcessHandle,
				   &PpbBase,
				   0,
				   &PpbSize,
				   MEM_RESERVE | MEM_COMMIT,
				   PAGE_READWRITE);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }

  DPRINT("Ppb->AllocationSize %x\n", Ppb->AllocationSize);
  DPRINT("Ppb->Size %x\n", Ppb->Size);

  /* write process parameters block*/
  NtWriteVirtualMemory(ProcessHandle,
		       PpbBase,
		       Ppb,
		       Ppb->AllocationSize,
		       &BytesWritten);

  /* write pointer to process parameter block */
  Offset = FIELD_OFFSET(PEB, ProcessParameters);
  NtWriteVirtualMemory(ProcessHandle,
		       (PVOID)(PEB_BASE + Offset),
		       &PpbBase,
		       sizeof(PpbBase),
		       &BytesWritten);
#endif

  /* Set image file name */
  Status = NtSetInformationProcess(ProcessHandle,
				   ProcessImageFileName,
				   "SMSS",
				   5);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("NtSetInformationProcess() failed (Status %lx)\n", Status);
      return(Status);
    }


  /* Read image base address. */
  Offset = FIELD_OFFSET(PEB, ImageBaseAddress);
  NtReadVirtualMemory(ProcessHandle,
		      (PVOID)(PEB_BASE + Offset),
		      ImageBaseAddress,
		      sizeof(PVOID),
		      &BytesWritten);

  return(STATUS_SUCCESS);
}


static NTSTATUS
LdrpCreateStack(HANDLE ProcessHandle,
		PINITIAL_TEB InitialTeb)
{
  ULONG OldPageProtection;
  NTSTATUS Status;

  InitialTeb->StackAllocate = NULL;

  /* Allocate the reserved stack space */
  Status = NtAllocateVirtualMemory(ProcessHandle,
				   &InitialTeb->StackAllocate,
				   0,
				   &InitialTeb->StackReserve,
				   MEM_RESERVE,
				   PAGE_READWRITE);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("Stack allocation failed (Status %x)", Status);
      return(Status);
    }

  DPRINT("StackAllocate: %p ReserveSize: 0x%lX\n",
	 InitialTeb->StackAllocate, InitialTeb->StackReserve);

  InitialTeb->StackBase = (PVOID)((ULONG)InitialTeb->StackAllocate + InitialTeb->StackReserve);
  InitialTeb->StackLimit = (PVOID)((ULONG)InitialTeb->StackBase - InitialTeb->StackCommit);

  DPRINT("StackBase: %p  StackCommit: 0x%lX\n",
	 InitialTeb->StackBase, InitialTeb->StackCommit);

  /* Commit stack */
  Status = NtAllocateVirtualMemory(ProcessHandle,
				   &InitialTeb->StackLimit,
				   0,
				   &InitialTeb->StackCommit,
				   MEM_COMMIT,
				   PAGE_READWRITE);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("Error comitting stack page!\n");
      /* release the stack space */
      NtFreeVirtualMemory(ProcessHandle,
			  InitialTeb->StackAllocate,
			  &InitialTeb->StackReserve,
			  MEM_RELEASE);
      return(Status);
    }

  DPRINT1("StackLimit: %p\nStackCommit: 0x%lX\n",
	 InitialTeb->StackLimit,
	 InitialTeb->StackCommit);

  /* Protect guard page */
  Status = NtProtectVirtualMemory(ProcessHandle,
				  InitialTeb->StackLimit,
				  PAGE_SIZE,
				  PAGE_GUARD | PAGE_READWRITE,
				  &OldPageProtection);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("Error protecting guard page!\n");

      /* release the stack space */
      NtFreeVirtualMemory(ProcessHandle,
			  InitialTeb->StackAllocate,
			  &InitialTeb->StackReserve,
			  MEM_RELEASE);
      return(Status);
    }

  return(STATUS_SUCCESS);
}


NTSTATUS
LdrLoadInitialProcess(PHANDLE ProcessHandle,
		      PHANDLE ThreadHandle)
{
  SECTION_IMAGE_INFORMATION Sii;
  UNICODE_STRING ImagePath;
  HANDLE SectionHandle;
  CONTEXT Context;
  INITIAL_TEB InitialTeb;
  ULONG ResultLength;
  PVOID ImageBaseAddress;
  ULONG InitialStack[5];
  NTSTATUS Status;

  /* Get the absolute path to smss.exe. */
  RtlInitUnicodeStringFromLiteral(&ImagePath,
				  L"\\SystemRoot\\system32\\smss.exe");

  /* Map process image */
  Status = LdrpMapProcessImage(&SectionHandle,
			       &ImagePath);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("LdrpMapImage() failed (Status %lx)\n", Status);
      return(Status);
    }

  /* Get information about the process image. */
   Status = NtQuerySection(SectionHandle,
			   SectionImageInformation,
			   &Sii,
			   sizeof(Sii),
			   &ResultLength);
  if (!NT_SUCCESS(Status) || ResultLength != sizeof(Sii))
    {
      DPRINT1("ZwQuerySection failed (Status %X)\n", Status);
      NtClose(ProcessHandle);
      NtClose(SectionHandle);
      return(Status);
    }

  DPRINT("Creating process\n");
  Status = NtCreateProcess(ProcessHandle,
			   PROCESS_ALL_ACCESS,
			   NULL,
			   SystemProcessHandle,
			   FALSE,
			   SectionHandle,
			   NULL,
			   NULL);
  NtClose(SectionHandle);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("NtCreateProcess() failed (Status %lx)\n", Status);
      return(Status);
    }

  /* Create process environment */
  DPRINT("Creating the process environment\n");
  Status = LdrpCreateProcessEnvironment(*ProcessHandle,
					&ImagePath,
					&ImageBaseAddress);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("LdrpCreateProcessEnvironment() failed (Status %lx)\n", Status);
      NtClose(*ProcessHandle);
      return(Status);
    }
  DPRINT("ImageBaseAddress: %p\n", ImageBaseAddress);


  /* Calculate initial stack sizes */
  if (Sii.StackReserve > 0x100000)
    InitialTeb.StackReserve = Sii.StackReserve;
  else
    InitialTeb.StackReserve = 0x100000; /* 1MByte */

  /* FIXME */
#if 0
  if (Sii.StackCommit > PAGE_SIZE)
    InitialTeb.StackCommit =  Sii.StackCommit;
  else
    InitialTeb.StackCommit = PAGE_SIZE;
#endif
  InitialTeb.StackCommit = InitialTeb.StackReserve - PAGE_SIZE;

  /* add guard page size */
  InitialTeb.StackCommit += PAGE_SIZE;
  DPRINT("StackReserve 0x%lX  StackCommit 0x%lX\n",
	 InitialTeb.StackReserve, InitialTeb.StackCommit);


  /* Create the process stack */
  Status = LdrpCreateStack(*ProcessHandle,
			   &InitialTeb);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("Failed to write initial stack.\n");
      NtClose(ProcessHandle);
      return(Status);
    }


  /*
   * Initialize context to point to LdrStartup
   */
  memset(&Context,0,sizeof(CONTEXT));
  Context.Eip = (ULONG)(ImageBaseAddress + (ULONG)Sii.EntryPoint);
  Context.SegCs = USER_CS;
  Context.SegDs = USER_DS;
  Context.SegEs = USER_DS;
  Context.SegFs = TEB_SELECTOR;
  Context.SegGs = USER_DS;
  Context.SegSs = USER_DS;
  Context.EFlags = 0x202;
  Context.Esp = (ULONG)InitialTeb.StackBase - 20;

  /*
   * Write in the initial stack.
   */
  InitialStack[0] = 0;
  InitialStack[1] = PEB_BASE;
  Status = NtWriteVirtualMemory(*ProcessHandle,
				(PVOID)Context.Esp,
				InitialStack,
				sizeof(InitialStack),
				&ResultLength);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("Failed to write initial stack.\n");

      NtFreeVirtualMemory(*ProcessHandle,
			  InitialTeb.StackAllocate,
			  &InitialTeb.StackReserve,
			  MEM_RELEASE);
      NtClose(*ProcessHandle);
      return(Status);
    }

  /* Create initial thread */
  DPRINT1("Creating thread for initial process\n");
  Status = NtCreateThread(ThreadHandle,
			  THREAD_ALL_ACCESS,
			  NULL,
			  *ProcessHandle,
			  NULL,
			  &Context,
			  &InitialTeb,
			  FALSE);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("NtCreateThread() failed (Status %lx)\n", Status);

      NtFreeVirtualMemory(*ProcessHandle,
			  InitialTeb.StackAllocate,
			  &InitialTeb.StackReserve,
			  MEM_RELEASE);

      NtClose(*ProcessHandle);
      return(Status);
    }

  DPRINT("Process created successfully\n");

  return(STATUS_SUCCESS);
}

/* EOF */
