/* $Id: finfo.c,v 1.1 1999/12/11 21:14:48 dwelch Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/fs/vfat/finfo.c
 * PURPOSE:          VFAT Filesystem
 * PROGRAMMER:       Jason Filby (jasonfilby@yahoo.com)
 *
 */

/* INCLUDES *****************************************************************/

#include <wchar.h>
#include <internal/string.h>
#include <ddk/ntddk.h>
#include <ddk/cctypes.h>

#define NDEBUG
#include <internal/debug.h>

#include "vfat.h"

/* FUNCTIONS ****************************************************************/

NTSTATUS FsdGetStandardInformation(PVFATFCB FCB, PDEVICE_OBJECT DeviceObject,
                                   PFILE_STANDARD_INFORMATION StandardInfo)
/*
 * FUNCTION: Retrieve the standard file information
 */
{
  PDEVICE_EXTENSION DeviceExtension;
  unsigned long AllocSize;

  DeviceExtension = DeviceObject->DeviceExtension;
  /* PRECONDITION */
  assert(DeviceExtension != NULL);
  assert(DeviceExtension->BytesPerCluster != 0);
  assert(StandardInfo != NULL);
  assert(FCB != NULL);

  RtlZeroMemory(StandardInfo, sizeof(FILE_STANDARD_INFORMATION));

  /* Make allocsize a rounded up multiple of BytesPerCluster */
  AllocSize = ((FCB->entry.FileSize +  DeviceExtension->BytesPerCluster - 1) /
              DeviceExtension->BytesPerCluster) *
              DeviceExtension->BytesPerCluster;

  StandardInfo->AllocationSize = RtlConvertUlongToLargeInteger(AllocSize);
  StandardInfo->EndOfFile      = RtlConvertUlongToLargeInteger(FCB->entry.FileSize);
  StandardInfo->NumberOfLinks  = 0;
  StandardInfo->DeletePending  = FALSE;
  if((FCB->entry.Attrib & 0x10)>0) {
    StandardInfo->Directory    = TRUE;
  } else {
    StandardInfo->Directory    = FALSE;
  }

  return STATUS_SUCCESS;
}

NTSTATUS FsdSetPositionInformation(PFILE_OBJECT FileObject,
				   PVFATFCB FCB,
				   PDEVICE_OBJECT DeviceObject,
                                   PFILE_POSITION_INFORMATION PositionInfo)
 {
    DPRINT("FsdSetPositionInformation()\n");
    
    DPRINT("PositionInfo %x\n", PositionInfo);
    DPRINT("Setting position %d\n", PositionInfo->CurrentByteOffset.u.LowPart);
    memcpy(&FileObject->CurrentByteOffset,&PositionInfo->CurrentByteOffset,
	   sizeof(LARGE_INTEGER));
    
    return(STATUS_SUCCESS);
 }
 
NTSTATUS FsdGetPositionInformation(PFILE_OBJECT FileObject,
				   PVFATFCB FCB,
				   PDEVICE_OBJECT DeviceObject,
                                   PFILE_POSITION_INFORMATION PositionInfo)
 {
    DPRINT("FsdGetPositionInformation()\n");
    
    memcpy(&PositionInfo->CurrentByteOffset, &FileObject->CurrentByteOffset,
	   sizeof(LARGE_INTEGER));
    DPRINT("Getting position %x\n", PositionInfo->CurrentByteOffset.u.LowPart);
    return(STATUS_SUCCESS);
 }

NTSTATUS FsdGetBasicInformation(PFILE_OBJECT FileObject,
                                PVFATFCB FCB,
                                PDEVICE_OBJECT DeviceObject,
                                PFILE_BASIC_INFORMATION BasicInfo)
{
    DPRINT("FsdGetBasicInformation()\n");

    FsdDosDateTimeToFileTime(FCB->entry.CreationDate,FCB->entry.CreationTime,
        &BasicInfo->CreationTime);
    FsdDosDateTimeToFileTime(FCB->entry.AccessDate,0,
        &BasicInfo->LastAccessTime);
    FsdDosDateTimeToFileTime(FCB->entry.UpdateDate,FCB->entry.UpdateTime,
        &BasicInfo->LastWriteTime);
    FsdDosDateTimeToFileTime(FCB->entry.UpdateDate,FCB->entry.UpdateTime,
        &BasicInfo->ChangeTime);

    BasicInfo->FileAttributes = FCB->entry.Attrib;

    DPRINT("Getting attributes %x\n", BasicInfo->FileAttributes);

    return(STATUS_SUCCESS);
}


NTSTATUS FsdSetDispositionInformation(PFILE_OBJECT FileObject,
                                      PVFATFCB FCB,
                                      PDEVICE_OBJECT DeviceObject,
                                      PFILE_DISPOSITION_INFORMATION DispositionInfo)
{
    DPRINT("FsdSetDispositionInformation()\n");

    FileObject->DeletePending = DispositionInfo->DeleteFile;
   
    return(STATUS_SUCCESS);
}


NTSTATUS FsdQueryInformation(PDEVICE_OBJECT DeviceObject, PIRP Irp)
/*
 * FUNCTION: Retrieve the specified file information
 */
{
   PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
   FILE_INFORMATION_CLASS FileInformationClass =
     Stack->Parameters.QueryFile.FileInformationClass;
   PFILE_OBJECT FileObject = NULL;
   PVFATFCB FCB = NULL;
//   PVFATCCB CCB = NULL;

   NTSTATUS RC = STATUS_SUCCESS;
   void *SystemBuffer;

   /* PRECONDITION */
   assert(DeviceObject != NULL);
   assert(Irp != NULL);

   /* INITIALIZATION */
   Stack = IoGetCurrentIrpStackLocation(Irp);
   FileInformationClass = Stack->Parameters.QueryFile.FileInformationClass;
   FileObject = Stack->FileObject;
//   CCB = (PVFATCCB)(FileObject->FsContext2);
//   FCB = CCB->Buffer; // Should be CCB->FCB???
   FCB = ((PVFATCCB)(FileObject->FsContext2))->pFcb;

  // FIXME : determine Buffer for result :
  if (Irp->MdlAddress) 
    SystemBuffer = MmGetSystemAddressForMdl(Irp->MdlAddress);
  else
    SystemBuffer = Irp->UserBuffer;
//   SystemBuffer = Irp->AssociatedIrp.SystemBuffer;

   switch(FileInformationClass) {
      case FileStandardInformation:
         RC = FsdGetStandardInformation(FCB, DeviceObject, SystemBuffer);
      break;
      case FilePositionInformation:
         RC = FsdGetPositionInformation(FileObject,
					FCB, 
					DeviceObject, 
					SystemBuffer);
      break;
      case FileBasicInformation:
         RC = FsdGetBasicInformation(FileObject,
                                     FCB, 
                                     DeviceObject, 
                                     SystemBuffer);
      break;
      default:
       RC=STATUS_NOT_IMPLEMENTED;
   }

   Irp->IoStatus.Status = RC;
   Irp->IoStatus.Information = 0;
   IoCompleteRequest(Irp, IO_NO_INCREMENT);

   return RC;
}

NTSTATUS VfatSetInformation(PDEVICE_OBJECT DeviceObject, PIRP Irp)
/*
 * FUNCTION: Retrieve the specified file information
 */
{
   PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
   FILE_INFORMATION_CLASS FileInformationClass;
   PFILE_OBJECT FileObject = NULL;
   PVFATFCB FCB = NULL;
//   PVFATCCB CCB = NULL;   
   NTSTATUS RC = STATUS_SUCCESS;
   PVOID SystemBuffer;

   /* PRECONDITION */
   assert(DeviceObject != NULL);
   assert(Irp != NULL);
   
   DPRINT("FsdSetInformation(DeviceObject %x, Irp %x)\n",
	    DeviceObject,Irp);
   
   /* INITIALIZATION */
   Stack = IoGetCurrentIrpStackLocation(Irp);
   FileInformationClass = Stack->Parameters.SetFile.FileInformationClass;
   FileObject = Stack->FileObject;
   FCB = ((PVFATCCB)(FileObject->FsContext2))->pFcb;

   // FIXME : determine Buffer for result :
  if (Irp->MdlAddress) 
     SystemBuffer = MmGetSystemAddressForMdl(Irp->MdlAddress);
   else
     SystemBuffer = Irp->UserBuffer;
   //   SystemBuffer = Irp->AssociatedIrp.SystemBuffer;
   
   DPRINT("FileInformationClass %d\n",FileInformationClass);
   DPRINT("SystemBuffer %x\n",SystemBuffer);

   switch(FileInformationClass) 
     {
      case FilePositionInformation:
	RC = FsdSetPositionInformation(FileObject,
				       FCB, 
				       DeviceObject, 
				       SystemBuffer);
	break;
      case FileDispositionInformation:
        RC = FsdSetDispositionInformation(FileObject,
                                          FCB, 
                                          DeviceObject, 
                                          SystemBuffer);
        break;
      default:
	RC = STATUS_NOT_IMPLEMENTED;
     }
   
   Irp->IoStatus.Status = RC;
   Irp->IoStatus.Information = 0;
   IoCompleteRequest(Irp, IO_NO_INCREMENT);
   
   return RC;
}



