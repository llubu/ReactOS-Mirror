/* $Id: csr.h,v 1.8 2003/12/02 11:38:46 gvg Exp $
 *
 */

#ifndef __INCLUDE_NTDLL_CSR_H
#define __INCLUDE_NTDLL_CSR_H

#include <csrss/csrss.h>

extern HANDLE WindowsApiPort; /* lpc.c */


NTSTATUS STDCALL CsrClientConnectToServer(VOID);
NTSTATUS STDCALL CsrClientCallServer(PCSRSS_API_REQUEST Request,
				     PCSRSS_API_REPLY Reply OPTIONAL,
				     ULONG Length,
				     ULONG ReplyLength);
NTSTATUS STDCALL CsrIdentifyAlertableThread(VOID);
NTSTATUS STDCALL CsrNewThread(VOID);
NTSTATUS STDCALL CsrSetPriorityClass(HANDLE Process,
				     PULONG PriorityClass);
VOID STDCALL CsrProbeForRead(IN CONST PVOID Address,
			     IN ULONG Length,
			     IN ULONG Alignment);
VOID STDCALL CsrProbeForWrite(IN CONST PVOID Address,
			      IN ULONG Length,
			      IN ULONG Alignment);
NTSTATUS STDCALL
CsrCaptureParameterBuffer(PVOID ParameterBuffer,
			  ULONG ParameterBufferSize,
			  PVOID* ClientAddress,
			  PVOID* ServerAddress);
NTSTATUS STDCALL
CsrReleaseParameterBuffer(PVOID ClientAddress);

VOID STDCALL CsrIsCsrss(VOID);

#endif /* __INCLUDE_NTDLL_CSR_H */

/* EOF */
