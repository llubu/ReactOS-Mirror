#ifndef __INCLUDE_NTOS_HALFUNCS_H
#define __INCLUDE_NTOS_HALFUNCS_H

#include <ntos/haltypes.h>

BOOLEAN STDCALL
HalAllProcessorsStarted(VOID);

BOOLEAN STDCALL
HalBeginSystemInterrupt(ULONG Vector,
  KIRQL Irql,
  PKIRQL OldIrql);

BOOLEAN STDCALL
HalDisableSystemInterrupt(ULONG Vector,
  KIRQL Irql);

BOOLEAN STDCALL
HalEnableSystemInterrupt(ULONG Vector,
  KIRQL Irql,
  KINTERRUPT_MODE InterruptMode);

VOID STDCALL
HalEndSystemInterrupt(KIRQL Irql,
  ULONG Unknown2);

VOID STDCALL
HalInitializeProcessor(ULONG ProcessorNumber,
  PVOID ProcessorStack);

BOOLEAN STDCALL
HalInitSystem(ULONG BootPhase,
  PLOADER_PARAMETER_BLOCK LoaderBlock);

VOID STDCALL
HalReportResourceUsage(VOID);

VOID
STDCALL
IoAssignDriveLetters(IN	PLOADER_PARAMETER_BLOCK	LoaderBlock,
  IN  PSTRING NtDeviceName,
  OUT PUCHAR NtSystemPath,
  OUT PSTRING NtSystemPathString);

KIRQL
STDCALL
KeRaiseIrqlToSynchLevel(VOID);

VOID STDCALL
HalReturnToFirmware(ULONG Action);

VOID FASTCALL
HalRequestSoftwareInterrupt(KIRQL SoftwareInterruptRequested);

/* Non-standard functions */
VOID STDCALL
HalReleaseDisplayOwnership();

BOOLEAN STDCALL
HalQueryDisplayOwnership();

#endif /* __INCLUDE_NTOS_HALDDK_H */

/* EOF */
