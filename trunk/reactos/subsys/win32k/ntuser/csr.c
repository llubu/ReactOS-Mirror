/* $Id$
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Interface to csrss
 * FILE:             subsys/win32k/ntuser/csr.c
 * PROGRAMER:        Ge van Geldorp (ge@gse.nl)
 */

#include <w32k.h>

static HANDLE WindowsApiPort = NULL;
PEPROCESS CsrProcess = NULL;

NTSTATUS FASTCALL
CsrInit(void)
{
  NTSTATUS Status;
  UNICODE_STRING PortName;
  ULONG ConnectInfoLength;

  RtlInitUnicodeString(&PortName, L"\\Windows\\ApiPort");
  ConnectInfoLength = 0;
  Status = ZwConnectPort(&WindowsApiPort,
                         &PortName,
                         NULL,
                         NULL,
                         NULL,
                         NULL,
                         NULL,
                         &ConnectInfoLength);
  if (! NT_SUCCESS(Status))
    {
      return Status;
    }

  CsrProcess = PsGetCurrentProcess();

  return STATUS_SUCCESS;
}


NTSTATUS FASTCALL
CsrNotify(PCSRSS_API_REQUEST Request, PCSRSS_API_REPLY Reply)
{
  NTSTATUS Status;
  PEPROCESS OldProcess;

  if (NULL == CsrProcess)
    {
      return STATUS_INVALID_PORT_HANDLE;
    }

  Request->Header.DataSize = sizeof(CSRSS_API_REQUEST) - LPC_MESSAGE_BASE_SIZE;
  Request->Header.MessageSize = sizeof(CSRSS_API_REQUEST);

  /* Switch to the process in which the WindowsApiPort handle is valid */
  OldProcess = PsGetCurrentProcess();
  if (CsrProcess != OldProcess)
    {
      KeAttachProcess(CsrProcess);
    }
  Status = ZwRequestWaitReplyPort(WindowsApiPort,
                                  &Request->Header,
                                  &Reply->Header);
  if (CsrProcess != OldProcess)
    {
      KeDetachProcess();
    }

  if (NT_SUCCESS(Status))
    {
      Status = Reply->Status;
    }

  return Status;
}

NTSTATUS STDCALL
CsrInsertObject(PVOID Object,
                PACCESS_STATE PassedAccessState,
                ACCESS_MASK DesiredAccess,
                ULONG AdditionalReferences,
                PVOID* ReferencedObject,
                PHANDLE Handle)
{
  NTSTATUS Status;
  PEPROCESS OldProcess;

  /* Switch to the process in which the handle is valid */
  OldProcess = PsGetCurrentProcess();
  if (CsrProcess != OldProcess)
    {
      KeAttachProcess(CsrProcess);
    }

  Status = ObInsertObject(Object,
                          PassedAccessState,
                          DesiredAccess,
                          AdditionalReferences,
                          ReferencedObject,
                          Handle);

  if (CsrProcess != OldProcess)
    {
      KeDetachProcess();
    }

  return Status;
}

NTSTATUS FASTCALL
CsrCloseHandle(HANDLE Handle)
{
  NTSTATUS Status;
  PEPROCESS OldProcess;

  /* Switch to the process in which the handle is valid */
  OldProcess = PsGetCurrentProcess();
  if (CsrProcess != OldProcess)
    {
      KeAttachProcess(CsrProcess);
    }

  Status = ZwClose(Handle);

  if (CsrProcess != OldProcess)
    {
      KeDetachProcess();
    }

  return Status;
}

/* EOF */
