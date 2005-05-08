/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/ntdll/rtl/ppb.c
 * PURPOSE:         Process parameters functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/* INCLUDES ****************************************************************/

#include "rtl.h"

#define NDEBUG
#include <debug.h>

/* MACROS ****************************************************************/

#define NORMALIZE(x,addr)   {if(x) x=(PVOID)((ULONG_PTR)(x)+(ULONG_PTR)(addr));}
#define DENORMALIZE(x,addr) {if(x) x=(PVOID)((ULONG_PTR)(x)-(ULONG_PTR)(addr));}
#define ALIGN(x,align)      (((ULONG)(x)+(align)-1UL)&(~((align)-1UL)))


KPROCESSOR_MODE
RtlpGetMode();


/* FUNCTIONS ****************************************************************/


static inline VOID
RtlpCopyParameterString(PWCHAR *Ptr,
			PUNICODE_STRING Destination,
			PUNICODE_STRING Source,
			ULONG Size)
{
   Destination->Length = Source->Length;
   Destination->MaximumLength = Size ? Size : Source->MaximumLength;
   Destination->Buffer = (PWCHAR)(*Ptr);
   if (Source->Length)
     memmove (Destination->Buffer, Source->Buffer, Source->Length);
   Destination->Buffer[Destination->Length / sizeof(WCHAR)] = 0;
   *Ptr += Destination->MaximumLength/sizeof(WCHAR);
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlCreateProcessParameters(PRTL_USER_PROCESS_PARAMETERS *ProcessParameters,
			   PUNICODE_STRING ImagePathName,
			   PUNICODE_STRING DllPath,
			   PUNICODE_STRING CurrentDirectory,
			   PUNICODE_STRING CommandLine,
			   PWSTR Environment,
			   PUNICODE_STRING WindowTitle,
			   PUNICODE_STRING DesktopInfo,
			   PUNICODE_STRING ShellInfo,
			   PUNICODE_STRING RuntimeInfo)
{
   NTSTATUS Status = STATUS_SUCCESS;
   PRTL_USER_PROCESS_PARAMETERS Param = NULL;
   ULONG RegionSize = 0;
   ULONG Length = 0;
   PWCHAR Dest;
   UNICODE_STRING EmptyString;
   HANDLE CurrentDirectoryHandle;
   HANDLE ConsoleHandle;
   ULONG ConsoleFlags;

   DPRINT ("RtlCreateProcessParameters\n");

   RtlAcquirePebLock();

   EmptyString.Length = 0;
   EmptyString.MaximumLength = sizeof(WCHAR);
   EmptyString.Buffer = L"";

   if (RtlpGetMode() == UserMode)
     {
	if (DllPath == NULL)
	  DllPath = &NtCurrentPeb()->ProcessParameters->DllPath;
	if (Environment == NULL)
	  Environment  = NtCurrentPeb()->ProcessParameters->Environment;
	if (CurrentDirectory == NULL)
	  CurrentDirectory = &NtCurrentPeb()->ProcessParameters->CurrentDirectoryName;
	CurrentDirectoryHandle = NtCurrentPeb()->ProcessParameters->CurrentDirectoryHandle;
	ConsoleHandle = NtCurrentPeb()->ProcessParameters->hConsole;
	ConsoleFlags = NtCurrentPeb()->ProcessParameters->ProcessGroup;
     }
   else
     {
	if (DllPath == NULL)
	  DllPath = &EmptyString;
	if (CurrentDirectory == NULL)
	  CurrentDirectory = &EmptyString;
	CurrentDirectoryHandle = NULL;
	ConsoleHandle = NULL;
	ConsoleFlags = 0;
     }

   if (CommandLine == NULL)
     CommandLine = &EmptyString;
   if (WindowTitle == NULL)
     WindowTitle = &EmptyString;
   if (DesktopInfo == NULL)
     DesktopInfo = &EmptyString;
   if (ShellInfo == NULL)
     ShellInfo = &EmptyString;
   if (RuntimeInfo == NULL)
     RuntimeInfo = &EmptyString;

   /* size of process parameter block */
   Length = sizeof(RTL_USER_PROCESS_PARAMETERS);

   /* size of current directory buffer */
   Length += (MAX_PATH * sizeof(WCHAR));

   /* add string lengths */
   Length += ALIGN(DllPath->MaximumLength, sizeof(ULONG));
   Length += ALIGN(ImagePathName->Length + sizeof(WCHAR), sizeof(ULONG));
   Length += ALIGN(CommandLine->Length + sizeof(WCHAR), sizeof(ULONG));
   Length += ALIGN(WindowTitle->MaximumLength, sizeof(ULONG));
   Length += ALIGN(DesktopInfo->MaximumLength, sizeof(ULONG));
   Length += ALIGN(ShellInfo->MaximumLength, sizeof(ULONG));
   Length += ALIGN(RuntimeInfo->MaximumLength, sizeof(ULONG));

   /* Calculate the required block size */
   RegionSize = ROUNDUP(Length, PAGE_SIZE);

   Status = ZwAllocateVirtualMemory(NtCurrentProcess(),
				    (PVOID*)&Param,
				    0,
				    &RegionSize,
				    MEM_RESERVE | MEM_COMMIT,
				    PAGE_READWRITE);
   if (!NT_SUCCESS(Status))
     {
	RtlReleasePebLock();
	return Status;
     }

   DPRINT ("Process parameters allocated\n");

   Param->AllocationSize = RegionSize;
   Param->Size = Length;
   Param->Flags = PPF_NORMALIZED;
   Param->Environment = Environment;
   Param->CurrentDirectoryHandle = CurrentDirectoryHandle;
   Param->hConsole = ConsoleHandle;
   Param->ProcessGroup = ConsoleFlags;

   Dest = (PWCHAR)(((PBYTE)Param) + sizeof(RTL_USER_PROCESS_PARAMETERS));

   /* copy current directory */
   RtlpCopyParameterString(&Dest,
			   &Param->CurrentDirectoryName,
			   CurrentDirectory,
			   MAX_PATH * sizeof(WCHAR));

   /* make sure the current directory has a trailing backslash */
   if (Param->CurrentDirectoryName.Length > 0)
     {
	ULONG Length;

	Length = Param->CurrentDirectoryName.Length / sizeof(WCHAR);
	if (Param->CurrentDirectoryName.Buffer[Length-1] != L'\\')
	  {
	     Param->CurrentDirectoryName.Buffer[Length] = L'\\';
	     Param->CurrentDirectoryName.Buffer[Length + 1] = 0;
	     Param->CurrentDirectoryName.Length += sizeof(WCHAR);
	  }
     }

   /* copy dll path */
   RtlpCopyParameterString(&Dest,
			   &Param->DllPath,
			   DllPath,
			   0);

   /* copy image path name */
   RtlpCopyParameterString(&Dest,
			   &Param->ImagePathName,
			   ImagePathName,
			   ImagePathName->Length + sizeof(WCHAR));

   /* copy command line */
   RtlpCopyParameterString(&Dest,
			   &Param->CommandLine,
			   CommandLine,
			   CommandLine->Length + sizeof(WCHAR));

   /* copy title */
   RtlpCopyParameterString(&Dest,
			   &Param->WindowTitle,
			   WindowTitle,
			   0);

   /* copy desktop */
   RtlpCopyParameterString(&Dest,
			   &Param->DesktopInfo,
			   DesktopInfo,
			   0);

   /* copy shell info */
   RtlpCopyParameterString(&Dest,
			   &Param->ShellInfo,
			   ShellInfo,
			   0);

   /* copy runtime info */
   RtlpCopyParameterString(&Dest,
			   &Param->RuntimeInfo,
			   RuntimeInfo,
			   0);

   RtlDeNormalizeProcessParams(Param);
   *ProcessParameters = Param;
   RtlReleasePebLock();

   return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS STDCALL
RtlDestroyProcessParameters(PRTL_USER_PROCESS_PARAMETERS ProcessParameters)
{
   ULONG RegionSize = 0;

   return ZwFreeVirtualMemory (NtCurrentProcess (),
			(PVOID)ProcessParameters,
			&RegionSize,
			MEM_RELEASE);
}

/*
 * denormalize process parameters (Pointer-->Offset)
 *
 * @implemented
 */
PRTL_USER_PROCESS_PARAMETERS STDCALL
RtlDeNormalizeProcessParams(PRTL_USER_PROCESS_PARAMETERS Params)
{
   if (Params && (Params->Flags & PPF_NORMALIZED))
     {
	DENORMALIZE(Params->CurrentDirectoryName.Buffer, Params);
	DENORMALIZE(Params->DllPath.Buffer, Params);
	DENORMALIZE(Params->ImagePathName.Buffer, Params);
	DENORMALIZE(Params->CommandLine.Buffer, Params);
	DENORMALIZE(Params->WindowTitle.Buffer, Params);
	DENORMALIZE(Params->DesktopInfo.Buffer, Params);
	DENORMALIZE(Params->ShellInfo.Buffer, Params);
	DENORMALIZE(Params->RuntimeInfo.Buffer, Params);

	Params->Flags &= ~PPF_NORMALIZED;
     }

   return Params;
}

/*
 * normalize process parameters (Offset-->Pointer)
 *
 * @implemented
 */
PRTL_USER_PROCESS_PARAMETERS STDCALL
RtlNormalizeProcessParams(PRTL_USER_PROCESS_PARAMETERS Params)
{
   if (Params && !(Params->Flags & PPF_NORMALIZED))
     {
	NORMALIZE(Params->CurrentDirectoryName.Buffer, Params);
	NORMALIZE(Params->DllPath.Buffer, Params);
	NORMALIZE(Params->ImagePathName.Buffer, Params);
	NORMALIZE(Params->CommandLine.Buffer, Params);
	NORMALIZE(Params->WindowTitle.Buffer, Params);
	NORMALIZE(Params->DesktopInfo.Buffer, Params);
	NORMALIZE(Params->ShellInfo.Buffer, Params);
	NORMALIZE(Params->RuntimeInfo.Buffer, Params);

	Params->Flags |= PPF_NORMALIZED;
     }

   return Params;
}

/* EOF */
