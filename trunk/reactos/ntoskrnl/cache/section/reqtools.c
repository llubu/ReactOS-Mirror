/*
 * Copyright (C) 1998-2005 ReactOS Team (and the authors from the programmers section)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 *
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/section.c
 * PURPOSE:         Implements section objects
 *
 * PROGRAMMERS:     Rex Jolliff
 *                  David Welch
 *                  Eric Kohl
 *                  Emanuele Aliberti
 *                  Eugene Ingerman
 *                  Casper Hornstrup
 *                  KJK::Hyperion
 *                  Guido de Jong
 *                  Ge van Geldorp
 *                  Royce Mitchell III
 *                  Filip Navara
 *                  Aleksey Bragin
 *                  Jason Filby
 *                  Thomas Weidenmueller
 *                  Gunnar Andre' Dalsnes
 *                  Mike Nordell
 *                  Alex Ionescu
 *                  Gregor Anich
 *                  Steven Edwards
 *                  Herve Poussineau
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include "newmm.h"
#define NDEBUG
#include <debug.h>

#define DPRINTC DPRINT

VOID
NTAPI
MmBuildMdlFromPages(PMDL Mdl, PPFN_NUMBER Pages);

NTSTATUS
NTAPI
MiGetOnePage
(PMMSUPPORT AddressSpace,
 PMEMORY_AREA MemoryArea,
 PMM_REQUIRED_RESOURCES Required)
{
	int i;
	NTSTATUS Status = STATUS_SUCCESS;

	for (i = 0; i < Required->Amount; i++)
	{
		DPRINTC("MiGetOnePage(%s:%d)\n", Required->File, Required->Line);
		Status = MmRequestPageMemoryConsumer(Required->Consumer, TRUE, &Required->Page[i]);
		if (!NT_SUCCESS(Status))
		{
			while (i > 0)
			{
				MmReleasePageMemoryConsumer(Required->Consumer, Required->Page[i-1]);
				i--;
			}
			return Status;
		}
	}
	
	return Status;
}

NTSTATUS
NTAPI
MiReadFilePage
(PMMSUPPORT AddressSpace, 
 PMEMORY_AREA MemoryArea, 
 PMM_REQUIRED_RESOURCES RequiredResources)
{
	PFILE_OBJECT FileObject = RequiredResources->Context;
	PPFN_NUMBER Page = &RequiredResources->Page[RequiredResources->Offset];
	PLARGE_INTEGER FileOffset = &RequiredResources->FileOffset;
	NTSTATUS Status;
    PVOID PageBuf = NULL;
    KEVENT Event;
	IO_STATUS_BLOCK IOSB;
    UCHAR MdlBase[sizeof(MDL) + sizeof(ULONG)];
    PMDL Mdl = (PMDL)MdlBase;
    KIRQL OldIrql;

	DPRINTC
		("Pulling page %08x%08x from %wZ to %x\n", 
		 FileOffset->u.HighPart, FileOffset->u.LowPart,
		 &FileObject->FileName,
		 Page);

	Status = MmRequestPageMemoryConsumer(RequiredResources->Consumer, TRUE, Page);
	if (!NT_SUCCESS(Status))
	{
		DPRINT1("Status: %x\n", Status);
		return Status;
	}

    MmInitializeMdl(Mdl, NULL, PAGE_SIZE);
    MmBuildMdlFromPages(Mdl, Page);
    Mdl->MdlFlags |= MDL_PAGES_LOCKED;
		 
    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    Status = IoPageRead
        (FileObject,
         Mdl,
         FileOffset,
         &Event,
         &IOSB);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IOSB.Status;
    }
    if (Mdl->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA)
    {
        MmUnmapLockedPages (Mdl->MappedSystemVa, Mdl);
    }

    PageBuf = MiMapPageInHyperSpace(PsGetCurrentProcess(), *Page, &OldIrql);
    if (!PageBuf)
    {
        MmReleasePageMemoryConsumer(RequiredResources->Consumer, *Page);
        return STATUS_NO_MEMORY;
    }
	RtlZeroMemory
		((PCHAR)PageBuf+RequiredResources->Amount,
		 PAGE_SIZE-RequiredResources->Amount);
    MiUnmapPageInHyperSpace(PsGetCurrentProcess(), PageBuf, OldIrql);
	
	DPRINT("Read Status %x (Page %x)\n", Status, *Page);

	if (!NT_SUCCESS(Status))
	{
		MmReleasePageMemoryConsumer(RequiredResources->Consumer, *Page);
		DPRINT("Status: %x\n", Status);
		return Status;
	}

	return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
MiSwapInPage
(PMMSUPPORT AddressSpace,
 PMEMORY_AREA MemoryArea,
 PMM_REQUIRED_RESOURCES Resources)
{
	NTSTATUS Status;

	Status = MmRequestPageMemoryConsumer(Resources->Consumer, TRUE, &Resources->Page[Resources->Offset]);
	if (!NT_SUCCESS(Status))
	{
		DPRINT1("MmRequestPageMemoryConsumer failed, status = %x\n", Status);
		return Status;
	}
	
	Status = MmReadFromSwapPage(Resources->SwapEntry, Resources->Page[Resources->Offset]);
	if (!NT_SUCCESS(Status))
	{
		DPRINT1("MmReadFromSwapPage failed, status = %x\n", Status);
		return Status;
	}

	MmSetSavedSwapEntryPage(Resources->Page[Resources->Offset], Resources->SwapEntry);

	DPRINT1("MiSwapInPage(%x,%x)\n", Resources->Page[Resources->Offset], Resources->SwapEntry);

	return Status;
}

NTSTATUS
NTAPI
MiWriteFilePage
(PMMSUPPORT AddressSpace, 
 PMEMORY_AREA MemoryArea, 
 PMM_REQUIRED_RESOURCES Required)
{
	DPRINT1("MiWriteFilePage(%x,%x)\n", Required->Page[Required->Offset], Required->FileOffset.LowPart);

	return MiWriteBackPage
		(Required->Context, 
		 &Required->FileOffset, 
		 PAGE_SIZE, 
		 Required->Page[Required->Offset]);
}
