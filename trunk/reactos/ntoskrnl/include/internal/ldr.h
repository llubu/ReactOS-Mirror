/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            include/internal/ldr.h
 * PURPOSE:         Header for loader module
 */

#ifndef __INCLUDE_INTERNAL_LDR_H
#define __INCLUDE_INTERNAL_LDR_H

#include <pe.h>
#include <internal/io.h>
#include <internal/module.h>

#define  KERNEL_MODULE_NAME  L"ntoskrnl.exe"
#define  HAL_MODULE_NAME  L"hal.dll"
#define  DRIVER_ROOT_NAME  L"\\Driver\\"
#define  FILESYSTEM_ROOT_NAME  L"\\FileSystem\\"


extern ULONG_PTR LdrHalBase;

NTSTATUS
LdrLoadInitialProcess(PHANDLE ProcessHandle,
		      PHANDLE ThreadHandle);

VOID
LdrLoadAutoConfigDrivers (
	VOID
	);
VOID
LdrInitModuleManagement (
	VOID
	);

NTSTATUS
LdrpMapSystemDll (
	HANDLE	ProcessHandle,
	PVOID	* LdrStartupAddress
	);
PVOID
LdrpGetSystemDllEntryPoint (VOID);
PVOID 
LdrpGetSystemDllApcDispatcher(VOID);
PVOID 
LdrpGetSystemDllExceptionDispatcher(VOID);
PVOID 
LdrpGetSystemDllCallbackDispatcher(VOID);
PVOID
LdrpGetSystemDllRaiseExceptionDispatcher(VOID);
NTSTATUS
LdrpMapImage (
	HANDLE	ProcessHandle,
	HANDLE	SectionHandle,
	PVOID	* ImageBase
	);


NTSTATUS STDCALL
LdrGetProcedureAddress (IN PVOID BaseAddress,
                        IN PANSI_STRING Name,
                        IN ULONG Ordinal,
                        OUT PVOID *ProcedureAddress);

NTSTATUS
LdrpLoadImage(PUNICODE_STRING DriverName,
	      PVOID *ModuleBase,
	      PVOID *SectionPointer,
	      PVOID *EntryPoint,
	      PVOID *ExportDirectory);

NTSTATUS
LdrpUnloadImage(PVOID ModuleBase);

NTSTATUS
LdrpLoadAndCallImage(PUNICODE_STRING DriverName);

NTSTATUS
LdrpQueryModuleInformation(PVOID Buffer,
			   ULONG Size,
			   PULONG ReqSize);

PVOID STDCALL
RtlImageDirectoryEntryToData (
	IN PVOID	BaseAddress,
	IN BOOLEAN	ImageLoaded,
	IN ULONG	Directory,
	OUT PULONG	Size);
VOID
LdrInit1(VOID);
VOID
LdrInitDebug(PLOADER_MODULE Module, PWCH Name);

PVOID LdrSafePEProcessModule(
 	PVOID ModuleLoadBase,
  PVOID DriverBase,
 	PVOID ImportModuleBase,
 	PULONG DriverSize);

NTSTATUS
LdrLoadModule(PUNICODE_STRING Filename,
	      PMODULE_OBJECT *ModuleObject);

NTSTATUS
LdrUnloadModule(PMODULE_OBJECT ModuleObject);

PMODULE_OBJECT
LdrGetModuleObject(PUNICODE_STRING ModuleName);

#endif /* __INCLUDE_INTERNAL_LDR_H */
