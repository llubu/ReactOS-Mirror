#ifndef __INCLUDE_INTERNAL_DBG_H
#define __INCLUDE_INTERNAL_DBG_H

#include <napi/dbg.h>
#include <internal/port.h>

#define KdPrintEx(_x_) DbgPrintEx _x_

NTSTATUS STDCALL 
LpcSendDebugMessagePort(PEPORT Port,
			PLPC_DBG_MESSAGE Message,
			PLPC_DBG_MESSAGE Reply);
VOID
DbgkCreateThread(PVOID StartAddress);
ULONG
DbgkForwardException(EXCEPTION_RECORD Er, ULONG FirstChance);
BOOLEAN
DbgShouldPrint(PCH Filename);

#endif /* __INCLUDE_INTERNAL_DBG_H */
