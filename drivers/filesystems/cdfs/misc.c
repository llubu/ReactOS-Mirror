/*
 *  ReactOS kernel
 *  Copyright (C) 2002, 2004 ReactOS Team
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
 * FILE:             drivers/filesystems/cdfs/misc.c
 * PURPOSE:          CDROM (ISO 9660) filesystem driver
 * PROGRAMMER:       Eric Kohl
 * UPDATE HISTORY:
 */

/* INCLUDES *****************************************************************/

#include "cdfs.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

/*
 * FUNCTION: Used with IRP to set them to TopLevelIrp field
 * ARGUMENTS:
 *           Irp = The IRP to set
 * RETURNS: TRUE if top level was null, else FALSE
 */
BOOLEAN
CdfsIsIrpTopLevel(
    PIRP Irp)
{
    BOOLEAN ReturnCode = FALSE;

    DPRINT("CdfsIsIrpTopLevel()\n");

    if (IoGetTopLevelIrp() == NULL)
    {
        IoSetTopLevelIrp(Irp);
        ReturnCode = TRUE;
    }

    return ReturnCode;
}


/*
 * FUNCTION: Allocate and fill a CDFS_IRP_CONTEXT struct in order to use it for IRP
 * ARGUMENTS:
 *           DeviceObject = Used to fill in struct 
 *           Irp = The IRP that need IRP_CONTEXT struct
 * RETURNS: NULL or PCDFS_IRP_CONTEXT
 */
PCDFS_IRP_CONTEXT
CdfsAllocateIrpContext(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PCDFS_IRP_CONTEXT IrpContext;

    DPRINT("CdfsAllocateIrpContext()\n");

    IrpContext = (PCDFS_IRP_CONTEXT)ExAllocateFromNPagedLookasideList(&CdfsGlobalData->IrpContextLookasideList);
    if (IrpContext == NULL)
        return NULL;

    RtlZeroMemory(IrpContext, sizeof(CDFS_IRP_CONTEXT));

//    IrpContext->Identifier.Type = NTFS_TYPE_IRP_CONTEST;
//    IrpContext->Identifier.Size = sizeof(NTFS_IRP_CONTEXT);
    IrpContext->Irp = Irp;
    IrpContext->DeviceObject = DeviceObject;
    IrpContext->Stack = IoGetCurrentIrpStackLocation(Irp);
    IrpContext->MajorFunction = IrpContext->Stack->MajorFunction;
    IrpContext->MinorFunction = IrpContext->Stack->MinorFunction;
    IrpContext->FileObject = IrpContext->Stack->FileObject;
    IrpContext->IsTopLevel = (IoGetTopLevelIrp() == Irp);
    IrpContext->PriorityBoost = IO_NO_INCREMENT;
    IrpContext->Flags = IRPCONTEXT_COMPLETE;

    if (IrpContext->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL ||
        IrpContext->MajorFunction == IRP_MJ_DEVICE_CONTROL ||
        IrpContext->MajorFunction == IRP_MJ_SHUTDOWN ||
        (IrpContext->MajorFunction != IRP_MJ_CLEANUP &&
         IrpContext->MajorFunction != IRP_MJ_CLOSE &&
         IoIsOperationSynchronous(Irp)))
    {
        IrpContext->Flags |= IRPCONTEXT_CANWAIT;
    }

    return IrpContext;
}


VOID
CdfsSwapString(PWCHAR Out,
               PUCHAR In,
               ULONG Count)
{
    PUCHAR t = (PUCHAR)Out;
    ULONG i;

    for (i = 0; i < Count; i += 2)
    {
        t[i] = In[i+1];
        t[i+1] = In[i];
        if (t[i+1] == 0 && t[i] == ';')
            break;
    }
    if ((i>2)&&(t[i-2] == '.'))
    {
        t[i-2] = 0;
        t[i-1] = 0;
    }
    t[i] = 0;
    t[i+1] = 0;
}


VOID
CdfsDateTimeToSystemTime(PFCB Fcb,
                         PLARGE_INTEGER SystemTime)
{
    TIME_FIELDS TimeFields;
    LARGE_INTEGER LocalTime;

    TimeFields.Milliseconds = 0;
    TimeFields.Second = Fcb->Entry.Second;
    TimeFields.Minute = Fcb->Entry.Minute;
    TimeFields.Hour = Fcb->Entry.Hour;

    TimeFields.Day = Fcb->Entry.Day;
    TimeFields.Month = Fcb->Entry.Month;
    TimeFields.Year = Fcb->Entry.Year + 1900;

    RtlTimeFieldsToTime(&TimeFields,
        &LocalTime);
    ExLocalTimeToSystemTime(&LocalTime, SystemTime);
}


VOID
CdfsFileFlagsToAttributes(PFCB Fcb,
                          PULONG FileAttributes)
{
    /* FIXME: Fix attributes */

    *FileAttributes = // FILE_ATTRIBUTE_READONLY |
        ((Fcb->Entry.FileFlags & FILE_FLAG_HIDDEN) ? FILE_ATTRIBUTE_HIDDEN : 0) |
        ((Fcb->Entry.FileFlags & FILE_FLAG_DIRECTORY) ? FILE_ATTRIBUTE_DIRECTORY : 0) |
        ((Fcb->Entry.FileFlags & FILE_FLAG_SYSTEM) ? FILE_ATTRIBUTE_SYSTEM : 0) |
        ((Fcb->Entry.FileFlags & FILE_FLAG_READONLY) ? FILE_ATTRIBUTE_READONLY : 0);
}

BOOLEAN
CdfsIsNameLegalDOS8Dot3(IN UNICODE_STRING FileName
    )
{
    ULONG i;
    STRING DbcsName;
    CHAR DbcsNameBuffer[12];

    /* 8dot3 filename is max 12 length */
    if (FileName.Length / sizeof(WCHAR) > 12)
    {
        return FALSE;
    }

    ASSERT(FileName.Length >= sizeof(WCHAR));
    for (i = 0; i < FileName.Length / sizeof(WCHAR) ; i++)
    {
        /* Don't allow spaces in FileName */
        if (FileName.Buffer[i] == L' ')
            return FALSE;
    }

    /* If FileName is finishing with a dot, remove it */
    if (FileName.Buffer[FileName.Length / sizeof(WCHAR) - 1] == '.')
    {
        FileName.Length -= sizeof(WCHAR);
    }

    /* Finally, convert the string to call the FsRtl function */
    RtlInitEmptyAnsiString(&DbcsName, DbcsNameBuffer, sizeof(DbcsNameBuffer));
    if (!NT_SUCCESS(RtlUnicodeStringToCountedOemString(&DbcsName,
                                                       &FileName,
                                                       FALSE)))
    {

        return FALSE;
    }
    return FsRtlIsFatDbcsLegal(DbcsName, FALSE, FALSE, FALSE);
}

BOOLEAN
CdfsIsRecordValid(IN PDEVICE_EXTENSION DeviceExt,
                  IN PDIR_RECORD Record)
{
    if (Record->RecordLength < Record->FileIdLength + FIELD_OFFSET(DIR_RECORD, FileId))
    {
        DPRINT1("Found corrupted entry! %u - %u\n", Record->RecordLength, Record->FileIdLength + FIELD_OFFSET(DIR_RECORD, FileId));
        return FALSE;
    }

    if (Record->FileIdLength == 0)
    {
        DPRINT1("Found corrupted entry (null size)!\n");
        return FALSE;
    }

    if (DeviceExt->CdInfo.JolietLevel == 0)
    {
        if (Record->FileId[0] == ANSI_NULL && Record->FileIdLength != 1)
        {
            DPRINT1("Found corrupted entry!\n");
            return FALSE;
        }
    }
    else
    {
        if (Record->FileIdLength & 1 && Record->FileIdLength != 1)
        {
            DPRINT1("Found corrupted entry! %u\n", Record->FileIdLength);
            return FALSE;
        }

        if (Record->FileIdLength == 1 && Record->FileId[0] != 0 &&  Record->FileId[0] != 1)
        {
            DPRINT1("Found corrupted entry! %c\n", Record->FileId[0]);
            DPRINT1("%wc\n", ((PWSTR)Record->FileId)[0]);
            return FALSE;
        }
    }

    return TRUE;
}

VOID
CdfsShortNameCacheGet
(PFCB DirectoryFcb, 
 PLARGE_INTEGER StreamOffset, 
 PUNICODE_STRING LongName, 
 PUNICODE_STRING ShortName)
{
    PLIST_ENTRY Entry;
    PCDFS_SHORT_NAME ShortNameEntry;
    GENERATE_NAME_CONTEXT Context = { 0 };

    DPRINT("CdfsShortNameCacheGet(%I64d,%wZ)\n", StreamOffset->QuadPart, LongName);

    /* Get the name list resource */
    ExAcquireResourceExclusiveLite(&DirectoryFcb->NameListResource, TRUE);

    /* Try to find the name in our cache */
    for (Entry = DirectoryFcb->ShortNameList.Flink; 
        Entry != &DirectoryFcb->ShortNameList;
        Entry = Entry->Flink)
    {
        ShortNameEntry = CONTAINING_RECORD(Entry, CDFS_SHORT_NAME, Entry);
        if (ShortNameEntry->StreamOffset.QuadPart == StreamOffset->QuadPart)
        {
            /* Cache hit */
            RtlCopyUnicodeString(ShortName, &ShortNameEntry->Name);
            ExReleaseResourceLite(&DirectoryFcb->NameListResource);
            DPRINT("Yield short name %wZ from cache\n", ShortName);
            return;
        }
    }

    /* Cache miss */
    if (!CdfsIsNameLegalDOS8Dot3(*LongName))
    {
        RtlGenerate8dot3Name(LongName, FALSE, &Context, ShortName);
    }
    else
    {
        /* copy short name */
        RtlUpcaseUnicodeString
            (ShortName,
            LongName,
            FALSE);
    }

    DPRINT("Initial Guess %wZ\n", ShortName);

    /* Make it unique by scanning the cache and bumping */
    /* Note that incrementing the ambiguous name is enough, since we add new
    * entries at the tail.  We'll scan over all collisions. */
    /* XXX could perform better. */
    for (Entry = DirectoryFcb->ShortNameList.Flink; 
        Entry != &DirectoryFcb->ShortNameList;
        Entry = Entry->Flink)
    {
        ShortNameEntry = CONTAINING_RECORD(Entry, CDFS_SHORT_NAME, Entry);
        if (RtlCompareUnicodeString
            (ShortName,
            &ShortNameEntry->Name,
            TRUE) == 0) /* Match */
        {
            RtlGenerate8dot3Name(LongName, FALSE, &Context, ShortName);
            DPRINT("Collide; try %wZ\n", ShortName);
        }
    }

    /* We've scanned over all entries and now have a unique one.  Cache it. */
    ShortNameEntry = ExAllocatePoolWithTag(PagedPool,
                                           sizeof(CDFS_SHORT_NAME),
                                           CDFS_SHORT_NAME_TAG);
    if (!ShortNameEntry) 
    {
        /* We couldn't cache it, but we can return it.  We run the risk of
        * generating a non-unique name later. */
        ExReleaseResourceLite(&DirectoryFcb->NameListResource);
        DPRINT1("Couldn't cache potentially clashing 8.3 name %wZ\n", ShortName);
        return;
    }

    ShortNameEntry->StreamOffset = *StreamOffset;
    RtlInitEmptyUnicodeString(&ShortNameEntry->Name,
                              ShortNameEntry->NameBuffer,
                              sizeof(ShortNameEntry->NameBuffer));
    RtlCopyUnicodeString(&ShortNameEntry->Name, ShortName);
    InsertTailList(&DirectoryFcb->ShortNameList, &ShortNameEntry->Entry);
    ExReleaseResourceLite(&DirectoryFcb->NameListResource);

    DPRINT("Returning short name %wZ for long name %wZ\n", ShortName, LongName);
}

VOID
CdfsGetDirEntryName(PDEVICE_EXTENSION DeviceExt,
                    PDIR_RECORD Record,
                    PWSTR Name)
                    /*
                    * FUNCTION: Retrieves the file name from a directory record.
                    */
{
    if (Record->FileIdLength == 1 && Record->FileId[0] == 0)
    {
        wcscpy(Name, L".");
    }
    else if (Record->FileIdLength == 1 && Record->FileId[0] == 1)
    {
        wcscpy(Name, L"..");
    }
    else
    {
        if (DeviceExt->CdInfo.JolietLevel == 0)
        {
            ULONG i;

            for (i = 0; i < Record->FileIdLength && Record->FileId[i] != ';'; i++)
                Name[i] = (WCHAR)Record->FileId[i];
            Name[i] = 0;
        }
        else
        {
            CdfsSwapString(Name,
                Record->FileId,
                Record->FileIdLength);
        }
    }

    DPRINT("Name '%S'\n", Name);
}

/* EOF */
