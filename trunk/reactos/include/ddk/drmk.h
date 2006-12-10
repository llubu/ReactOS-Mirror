/*
    ReactOS Kernel Streaming
    Digital Rights Management

    Author: Andrew Greenwood
*/

#ifndef DRMK_H
#define DRMK_H

#include <ntddk.h>
#include <ks.h>
#include <punknown.h>

typedef struct
{
    DWORD Flags;
    PDEVICE_OBJECT DeviceObject;
    PFILE_OBJECT FileObject;
    PVOID Context;
} DRMFORWARD, *PDRMFORWARD, *PCDRMFORWARD;

typedef struct
{
    BOOL CopyProtect;
    ULONG Reserved;
    BOOL DigitalOutputDisable;
} DRMRIGHTS, *PDRMRIGHTS;


/* ===============================================================
    Digital Rights Management Functions
    TODO: Check calling convention
*/

NTAPI NTSTATUS
DrmAddContentHandlers(
    IN  ULONG ContentId,
    IN  PVOID *paHandlers,
    IN  ULONG NumHandlers);

NTAPI NTSTATUS
DrmCreateContentMixed(
    IN  PULONG paContentId,
    IN  ULONG cContentId,
    OUT PULONG pMixedContentId);

NTAPI NTSTATUS
DrmDestroyContent(
    IN  ULONG ContentId);

NTAPI NTSTATUS
DrmForwardContentToDeviceObject(
    IN  ULONG ContentId,
    IN  PVOID Reserved,
    IN  PCDRMFORWARD DrmForward);

NTAPI NTSTATUS
DrmForwardContentToFileObject(
    IN  ULONG ContentId,
    IN  PFILE_OBJECT FileObject);

NTAPI NTSTATUS
DrmForwardContentToInterface(
    IN  ULONG ContentId,
    IN  PUNKNOWN pUnknown,
    IN  ULONG NumMethods);

NTAPI NTSTATUS
DrmGetContentRights(
    IN  ULONG ContentId,
    OUT PDRMRIGHTS DrmRights);


#endif
