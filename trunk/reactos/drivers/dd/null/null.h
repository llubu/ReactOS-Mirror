/* $Id$
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/dd/null/null.h
 * PURPOSE:          NULL device driver internal definitions
 * PROGRAMMER:       KJK::Hyperion <noog@libero.it>
 * UPDATE HISTORY:
 *              29/04/2002: Created
 */

typedef enum __tagNULL_EXTENSION{
 NullBitBucket,
 NullZeroStream,
} NULL_EXTENSION, *PNULL_EXTENSION;

#define NULL_DEVICE_TYPE(__DEVICE__) (*((PNULL_EXTENSION)((__DEVICE__)->DeviceExtension)))

NTSTATUS STDCALL
DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath);

/* EOF */
