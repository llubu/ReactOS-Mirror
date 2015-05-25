/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/filesystems/fastfat/close.c
 * PURPOSE:          VFAT Filesystem
 * PROGRAMMER:       Jason Filby (jasonfilby@yahoo.com)
 */

/* INCLUDES *****************************************************************/

#include "vfat.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

/*
 * FUNCTION: Closes a file
 */
NTSTATUS
VfatCloseFile(
    PDEVICE_EXTENSION DeviceExt,
    PFILE_OBJECT FileObject)
{
    PVFATFCB pFcb;
    PVFATCCB pCcb;
    NTSTATUS Status = STATUS_SUCCESS;

    DPRINT("VfatCloseFile(DeviceExt %p, FileObject %p)\n",
            DeviceExt, FileObject);

    /* FIXME : update entry in directory? */
    pCcb = (PVFATCCB) (FileObject->FsContext2);
    pFcb = (PVFATFCB) (FileObject->FsContext);

    if (pFcb == NULL)
    {
        return STATUS_SUCCESS;
    }

    if (pFcb->Flags & FCB_IS_VOLUME)
    {
        DPRINT("Volume\n");
        FileObject->FsContext2 = NULL;
    }
    else
    {
        vfatReleaseFCB(DeviceExt, pFcb);
    }

    FileObject->FsContext2 = NULL;
    FileObject->FsContext = NULL;
    FileObject->SectionObjectPointer = NULL;
    DeviceExt->OpenHandleCount--;

    if (pCcb)
    {
        vfatDestroyCCB(pCcb);
    }

#ifdef ENABLE_SWAPOUT
    if (DeviceExt->OpenHandleCount == 0)
    {
        VfatCheckForDismount(DeviceExt, FALSE);
    }
#endif

    return Status;
}

/*
 * FUNCTION: Closes a file
 */
NTSTATUS
VfatClose(
    PVFAT_IRP_CONTEXT IrpContext)
{
    NTSTATUS Status;

    DPRINT("VfatClose(DeviceObject %p, Irp %p)\n", IrpContext->DeviceObject, IrpContext->Irp);

    if (IrpContext->DeviceObject == VfatGlobalData->DeviceObject)
    {
        DPRINT("Closing file system\n");
        IrpContext->Irp->IoStatus.Information = 0;
        return STATUS_SUCCESS;
    }
#if 0
    /* There occurs a dead look at the call to CcRosDeleteFileCache/ObDereferenceObject/VfatClose
       in CmLazyCloseThreadMain if VfatClose is execute asynchronous in a worker thread. */
    if (!ExAcquireResourceExclusiveLite(&IrpContext->DeviceExt->DirResource, BooleanFlagOn(IrpContext->Flags, IRPCONTEXT_CANWAIT)))
#else
    if (!ExAcquireResourceExclusiveLite(&IrpContext->DeviceExt->DirResource, TRUE))
#endif
    {
        return VfatMarkIrpContextForQueue(IrpContext);
    }

    Status = VfatCloseFile(IrpContext->DeviceExt, IrpContext->FileObject);
    ExReleaseResourceLite(&IrpContext->DeviceExt->DirResource);

    IrpContext->Irp->IoStatus.Information = 0;

    return Status;
}

/* EOF */
