/* $Id: cleanup.c,v 1.15 2004/08/28 22:19:12 navaraf Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/fs/vfat/cleanup.c
 * PURPOSE:          VFAT Filesystem
 * PROGRAMMER:       Jason Filby (jasonfilby@yahoo.com)
 *                   Hartmut Birr
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#define NDEBUG
#include <debug.h>

#include "vfat.h"

/* FUNCTIONS ****************************************************************/

static NTSTATUS
VfatCleanupFile(PVFAT_IRP_CONTEXT IrpContext)
/*
 * FUNCTION: Cleans up after a file has been closed.
 */
{
  PVFATFCB pFcb;
  PFILE_OBJECT FileObject = IrpContext->FileObject;
  
  DPRINT("VfatCleanupFile(DeviceExt %x, FileObject %x)\n",
	 IrpContext->DeviceExt, FileObject);
  
  /* FIXME: handle file/directory deletion here */
  pFcb = (PVFATFCB) FileObject->FsContext;
  if (pFcb)
    {
      if (!(pFcb->entry.Attrib & FILE_ATTRIBUTE_DIRECTORY) &&
          FsRtlAreThereCurrentFileLocks(&pFcb->FileLock))
       {
         /* remove all locks this process have on this file */
         FsRtlFastUnlockAll(&pFcb->FileLock,
                            FileObject,
                            IoGetRequestorProcess(IrpContext->Irp),
                            NULL);
       }

     if (pFcb->Flags & FCB_IS_DIRTY)
       {
	 VfatUpdateEntry (pFcb);
       }

     /* Uninitialize file cache if initialized for this file object. */
     if (FileObject->PrivateCacheMap)
       {
         CcRosReleaseFileCache (FileObject);
       }

     pFcb->OpenHandleCount--;
     IoRemoveShareAccess(FileObject, &pFcb->FCBShareAccess);
    }
  return STATUS_SUCCESS;
}

NTSTATUS VfatCleanup (PVFAT_IRP_CONTEXT IrpContext)
/*
 * FUNCTION: Cleans up after a file has been closed.
 */
{
   NTSTATUS Status;

   DPRINT("VfatCleanup(DeviceObject %x, Irp %x)\n", IrpContext->DeviceObject, IrpContext->Irp);

   if (IrpContext->DeviceObject == VfatGlobalData->DeviceObject)
     {
       Status = STATUS_SUCCESS;
       goto ByeBye;
     }

   if (!ExAcquireResourceExclusiveLite (&IrpContext->DeviceExt->DirResource,
                                        (BOOLEAN)(IrpContext->Flags & IRPCONTEXT_CANWAIT)))
     {
       return VfatQueueRequest (IrpContext);
     }

   Status = VfatCleanupFile(IrpContext);

   ExReleaseResourceLite (&IrpContext->DeviceExt->DirResource);

ByeBye:
   IrpContext->Irp->IoStatus.Status = Status;
   IrpContext->Irp->IoStatus.Information = 0;

   IoCompleteRequest (IrpContext->Irp, IO_NO_INCREMENT);
   VfatFreeIrpContext(IrpContext);
   return (Status);
}

/* EOF */
