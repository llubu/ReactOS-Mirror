/* $Id:$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/i386/pfault.c
 * PURPOSE:         Paging file functions
 * 
 * PROGRAMMERS:     David Welch (welch@mcmail.com)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* EXTERNS *******************************************************************/

extern VOID MmSafeCopyFromUserUnsafeStart(VOID);
extern VOID MmSafeCopyFromUserRestart(VOID);
extern VOID MmSafeCopyToUserUnsafeStart(VOID);
extern VOID MmSafeCopyToUserRestart(VOID);

extern ULONG MmGlobalKernelPageDirectory[1024];

BOOLEAN
Mmi386MakeKernelPageTableGlobal(PVOID Address);

/* FUNCTIONS *****************************************************************/

NTSTATUS MmPageFault(ULONG Cs,
                     PULONG Eip,
                     PULONG Eax,
                     ULONG Cr2,
                     ULONG ErrorCode)
{
   KPROCESSOR_MODE Mode;
   NTSTATUS Status;

   DPRINT("MmPageFault(Eip %x, Cr2 %x, ErrorCode %x)\n",
          *Eip, Cr2, ErrorCode);

   if (ErrorCode & 0x4)
   {
      Mode = UserMode;
   }
   else
   {
      Mode = KernelMode;
   }

   if (Mode == KernelMode && Cr2 >= KERNEL_BASE &&
         Mmi386MakeKernelPageTableGlobal((PVOID)Cr2))
   {
      return(STATUS_SUCCESS);
   }

   if (ErrorCode & 0x1)
   {
      Status = MmAccessFault(Mode, Cr2, FALSE);
   }
   else
   {
      Status = MmNotPresentFault(Mode, Cr2, FALSE);
   }

   if (KeGetCurrentThread() != NULL &&
      KeGetCurrentThread()->Alerted[UserMode] != 0 &&
      Cs != KERNEL_CS)
   {
      KIRQL oldIrql;
      
      KeRaiseIrql(APC_LEVEL, &oldIrql);
      KiDeliverApc(KernelMode, NULL, NULL);
      KeLowerIrql(oldIrql);
   }
   if (!NT_SUCCESS(Status) && (Mode == KernelMode) &&
         ((*Eip) >= (ULONG)MmSafeCopyFromUserUnsafeStart) &&
         ((*Eip) <= (ULONG)MmSafeCopyFromUserRestart))
   {
      (*Eip) = (ULONG)MmSafeCopyFromUserRestart;
      (*Eax) = STATUS_ACCESS_VIOLATION;
      return(STATUS_SUCCESS);
   }
   if (!NT_SUCCESS(Status) && (Mode == KernelMode) &&
         ((*Eip) >= (ULONG)MmSafeCopyToUserUnsafeStart) &&
         ((*Eip) <= (ULONG)MmSafeCopyToUserRestart))
   {
      (*Eip) = (ULONG)MmSafeCopyToUserRestart;
      (*Eax) = STATUS_ACCESS_VIOLATION;
      return(STATUS_SUCCESS);
   }
   return(Status);
}
