/*
 *  ReactOS kernel
 *  Copyright (C) 2002 ReactOS Team
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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/filesystems/cdfs/cleanup.c
 * PURPOSE:          CDROM (ISO 9660) filesystem driver
 * PROGRAMMER:
 * UPDATE HISTORY:
 */

/* INCLUDES *****************************************************************/

#include "cdfs.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

static NTSTATUS
CdfsCleanupFile(PCDFS_IRP_CONTEXT IrpContext,
                PFILE_OBJECT FileObject)
                /*
                * FUNCTION: Cleans up after a file has been closed.
                */
{
    PDEVICE_EXTENSION DeviceExt;
    PFCB Fcb;

    DPRINT("CdfsCleanupFile(DeviceExt %p, FileObject %p)\n",
        DeviceExt,
        FileObject);

    DeviceExt = IrpContext->DeviceObject->DeviceExtension;
    Fcb = FileObject->FsContext;
    if (!Fcb)
    {
        return STATUS_SUCCESS;
    }

    /* Notify about the cleanup */
    FsRtlNotifyCleanup(DeviceExt->NotifySync,
                       &(DeviceExt->NotifyList),
                       FileObject->FsContext2);

   if (!CdfsFCBIsDirectory(Fcb) &&
       FsRtlAreThereCurrentFileLocks(&Fcb->FileLock))
    {
        FsRtlFastUnlockAll(&Fcb->FileLock,
                           FileObject,
                           IoGetRequestorProcess(IrpContext->Irp),
                           NULL);
    }

    /* Uninitialize file cache if initialized for this file object. */
    if (FileObject->SectionObjectPointer && FileObject->SectionObjectPointer->SharedCacheMap)
    {
        CcUninitializeCacheMap (FileObject, NULL, NULL);
    }

    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
CdfsCleanup(
    PCDFS_IRP_CONTEXT IrpContext)
{
    PIRP Irp;
    PDEVICE_OBJECT DeviceObject;
    PDEVICE_EXTENSION DeviceExtension;
    PIO_STACK_LOCATION Stack;
    PFILE_OBJECT FileObject;
    NTSTATUS Status;

    DPRINT("CdfsCleanup() called\n");

    ASSERT(IrpContext);

    Irp = IrpContext->Irp;
    DeviceObject = IrpContext->DeviceObject;
    Stack = IrpContext->Stack;

    if (DeviceObject == CdfsGlobalData->DeviceObject)
    {
        DPRINT("Closing file system\n");
        Status = STATUS_SUCCESS;
        goto ByeBye;
    }

    FileObject = Stack->FileObject;
    DeviceExtension = DeviceObject->DeviceExtension;

    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(&DeviceExtension->DirResource, TRUE);

    Status = CdfsCleanupFile(IrpContext, FileObject);

    ExReleaseResourceLite(&DeviceExtension->DirResource);
    KeLeaveCriticalRegion();

ByeBye:
    Irp->IoStatus.Information = 0;

    return(Status);
}

/* EOF */
