/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/dbg/kdb.c
 * PURPOSE:         Kernel debugger
 * 
 * PROGRAMMERS:     David Welch (welch@mcmail.com)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#include "kdb.h"
#define NDEBUG
#include <internal/debug.h>

/* TYPES *********************************************************************/

/* DEFINES *******************************************************************/

#define KDB_STACK_SIZE                   (4096*3)
#define KDB_MAXIMUM_BREAKPOINT_COUNT     256
#define KDB_MAXIMUM_HW_BREAKPOINT_COUNT  4
#define KDB_MAXIMUM_SW_BREAKPOINT_COUNT  256

#define __STRING(x) #x
#define _STRING(x) __STRING(x)

/* GLOBALS *******************************************************************/

STATIC LONG KdbEntryCount = 0;
STATIC CHAR KdbStack[KDB_STACK_SIZE];

STATIC ULONG KdbBreakPointCount = 0;  /* Number of used breakpoints in the array */
STATIC KDB_BREAKPOINT KdbBreakPoints[KDB_MAXIMUM_BREAKPOINT_COUNT] = {{0}};  /* Breakpoint array */
STATIC ULONG KdbSwBreakPointCount = 0;  /* Number of enabled software breakpoints */
STATIC ULONG KdbHwBreakPointCount = 0;  /* Number of enabled hardware breakpoints */
STATIC PKDB_BREAKPOINT KdbSwBreakPoints[KDB_MAXIMUM_SW_BREAKPOINT_COUNT]; /* Enabled software breakpoints, orderless */
STATIC PKDB_BREAKPOINT KdbHwBreakPoints[KDB_MAXIMUM_HW_BREAKPOINT_COUNT]; /* Enabled hardware breakpoints, orderless */
STATIC PKDB_BREAKPOINT KdbBreakPointToReenable = NULL; /* Set to a breakpoint struct when single stepping after
                                                          a software breakpoint was hit, to reenable it */
LONG KdbLastBreakPointNr = -1;  /* Index of the breakpoint which cause KDB to be entered */
ULONG KdbNumSingleSteps = 0; /* How many single steps to do */
BOOLEAN KdbSingleStepOver = FALSE; /* Whether to step over calls/reps. */

STATIC BOOLEAN KdbEnteredOnSingleStep = FALSE; /* Set to true when KDB was entered because of single step */
PEPROCESS KdbCurrentProcess = NULL;  /* The current process context in which KDB runs */
PEPROCESS KdbOriginalProcess = NULL; /* The process in whichs context KDB was intered */
PETHREAD KdbCurrentThread = NULL;  /* The current thread context in which KDB runs */
PETHREAD KdbOriginalThread = NULL; /* The thread in whichs context KDB was entered */
PKDB_KTRAP_FRAME KdbCurrentTrapFrame = NULL; /* Pointer to the current trapframe */
STATIC KDB_KTRAP_FRAME KdbTrapFrame = { { 0 } };  /* The trapframe which was passed to KdbEnterDebuggerException */
STATIC KDB_KTRAP_FRAME KdbThreadTrapFrame = { { 0 } }; /* The trapframe of the current thread (KdbCurrentThread) */
STATIC KAPC_STATE KdbApcState;

/* Array of conditions when to enter KDB */
STATIC KDB_ENTER_CONDITION KdbEnterConditions[][2] =
{
   /* First chance       Last chance */
   { KdbDoNotEnter,      KdbEnterFromKmode },   /* Zero devide */
   { KdbEnterAlways,     KdbDoNotEnter },       /* Debug trap */
   { KdbDoNotEnter,      KdbEnterAlways },      /* NMI */
   { KdbEnterFromKmode,  KdbDoNotEnter },       /* INT3 */
   { KdbDoNotEnter,      KdbEnterFromKmode },   /* Overflow */
   { KdbDoNotEnter,      KdbEnterFromKmode },
   { KdbDoNotEnter,      KdbEnterFromKmode },   /* Invalid opcode */
   { KdbDoNotEnter,      KdbEnterFromKmode },   /* No math coprocessor fault */
   { KdbEnterAlways,     KdbEnterAlways },
   { KdbEnterAlways,     KdbEnterAlways },
   { KdbDoNotEnter,      KdbEnterFromKmode },
   { KdbDoNotEnter,      KdbEnterFromKmode },
   { KdbDoNotEnter,      KdbEnterFromKmode },   /* Stack fault */
   { KdbDoNotEnter,      KdbEnterFromKmode },   /* General protection fault */
   { KdbDoNotEnter,      KdbEnterFromKmode },   /* Page fault */
   { KdbEnterAlways,     KdbEnterAlways },      /* Reserved (15) */
   { KdbDoNotEnter,      KdbEnterFromKmode },   /* FPU fault */
   { KdbDoNotEnter,      KdbEnterFromKmode },
   { KdbDoNotEnter,      KdbEnterFromKmode },
   { KdbDoNotEnter,      KdbEnterFromKmode },   /* SIMD fault */
   { KdbDoNotEnter,      KdbEnterFromKmode }    /* Last entry: used for unknown exceptions */
};

/* Exception descriptions */
STATIC CONST PCHAR ExceptionNrToString[] =
{
   "Divide Error",
   "Debug Trap",
   "NMI",
   "Breakpoint",
   "Overflow",
   "BOUND range exceeded",
   "Invalid Opcode",
   "No Math Coprocessor",
   "Double Fault",
   "Unknown(9)",
   "Invalid TSS",
   "Segment Not Present",
   "Stack Segment Fault",
   "General Protection",
   "Page Fault",
   "Reserved(15)",
   "Math Fault",
   "Alignment Check",
   "Machine Check",
   "SIMD Fault"
};

/* FUNCTIONS *****************************************************************/

/*!\brief Overwrites the instruction at \a Address with \a NewInst and stores
 *        the old instruction in *OldInst.
 *
 * \param Process  Process in which's context to overwrite the instruction.
 * \param Address  Address at which to overwrite the instruction.
 * \param NewInst  New instruction (written to \a Address)
 * \param OldInst  Old instruction (read from \a Address)
 *
 * \returns NTSTATUS
 */
STATIC NTSTATUS
KdbpOverwriteInstruction(
   IN  PEPROCESS Process,
   IN  ULONG_PTR Address,
   IN  UCHAR NewInst,
   OUT PUCHAR OldInst  OPTIONAL)
{
   NTSTATUS Status;
   ULONG Protect;
   PEPROCESS CurrentProcess = PsGetCurrentProcess();
   KAPC_STATE ApcState;

   /* Get the protection for the address. */
   Protect = MmGetPageProtect(Process, (PVOID)PAGE_ROUND_DOWN(Address));
   
   /* Return if that page isn't present. */
   if (Protect & PAGE_NOACCESS)
   {
      return STATUS_MEMORY_NOT_ALLOCATED;
   }
   
   /* Attach to the process */
   if (CurrentProcess != Process)
   {
      KeStackAttachProcess(EPROCESS_TO_KPROCESS(Process), &ApcState);
   }

   /* Make the page writeable if it is read only. */
   if (Protect & (PAGE_READONLY|PAGE_EXECUTE|PAGE_EXECUTE_READ))
   {
      MmSetPageProtect(Process, (PVOID)PAGE_ROUND_DOWN(Address),
	               (Protect & ~(PAGE_READONLY|PAGE_EXECUTE|PAGE_EXECUTE_READ)) | PAGE_READWRITE);
   }
   
   /* Copy the old instruction back to the caller. */
   if (OldInst != NULL)
   {
      Status = KdbpSafeReadMemory(OldInst, (PUCHAR)Address, 1);
      if (!NT_SUCCESS(Status))
      {
         if (Protect & (PAGE_READONLY|PAGE_EXECUTE|PAGE_EXECUTE_READ))
         {
            MmSetPageProtect(Process, (PVOID)PAGE_ROUND_DOWN(Address), Protect);
         }
         /* Detach from process */
         if (CurrentProcess != Process)
         {
            KeDetachProcess();
         }
	 return Status;
      }
   }
   
   /* Copy the new instruction in its place. */
   Status = KdbpSafeWriteMemory((PUCHAR)Address, &NewInst, 1);
   
   /* Restore the page protection. */
   if (Protect & (PAGE_READONLY|PAGE_EXECUTE|PAGE_EXECUTE_READ))
   {
      MmSetPageProtect(Process, (PVOID)PAGE_ROUND_DOWN(Address), Protect);
   }
   
   /* Detach from process */
   if (CurrentProcess != Process)
   {
      KeUnstackDetachProcess(&ApcState);
   }

   return Status;
}

/*!\brief Checks whether the given instruction can be single stepped or has to be
 *        stepped over using a temporary breakpoint.
 *
 * \retval TRUE   Instruction is a call.
 * \retval FALSE  Instruction is not a call.
 */
BOOLEAN
KdbpShouldStepOverInstruction(ULONG_PTR Eip)
{
   UCHAR Mem[3];
   INT i = 0;

   if (!NT_SUCCESS(KdbpSafeReadMemory(Mem, (PVOID)Eip, sizeof (Mem))))
   {
      KdbpPrint("Couldn't access memory at 0x%x\n", (UINT)Eip);
      return FALSE;
   }

   /* Check if the current instruction is a call. */
   while ((i < sizeof (Mem)) && (Mem[i] == 0x66 || Mem[i] == 0x67))
      i++;
   if (i == sizeof (Mem))
      return FALSE;
   if (Mem[i] == 0xE8 || Mem[i] == 0x9A || Mem[i] == 0xF2 || Mem[i] == 0xF3 ||
       (((i + 1) < sizeof (Mem)) && Mem[i] == 0xFF && (Mem[i+1] & 0x38) == 0x10))
   {
      return TRUE;
   }
   return FALSE;
}

/*!\brief Steps over an instruction
 *
 * If the given instruction should be stepped over, this function inserts a
 * temporary breakpoint after the instruction and returns TRUE, otherwise it
 * returns FALSE.
 *
 * \retval TRUE   Temporary breakpoint set after instruction.
 * \retval FALSE  No breakpoint was set.
 */
BOOLEAN
KdbpStepOverInstruction(ULONG_PTR Eip)
{
   LONG InstLen;

   if (!KdbpShouldStepOverInstruction(Eip))
      return FALSE;

   InstLen = KdbpGetInstLength(Eip);
   if (InstLen < 1)
      return FALSE;

   if (!NT_SUCCESS(KdbpInsertBreakPoint(Eip + InstLen, KdbBreakPointTemporary, 0, 0, NULL, FALSE, NULL)))
      return FALSE;

   return TRUE;
}

/*!\brief Steps into an instruction (interrupts)
 *
 * If the given instruction should be stepped into, this function inserts a
 * temporary breakpoint at the target instruction and returns TRUE, otherwise it
 * returns FALSE.
 *
 * \retval TRUE   Temporary breakpoint set at target instruction.
 * \retval FALSE  No breakpoint was set.
 */
BOOLEAN
KdbpStepIntoInstruction(ULONG_PTR Eip)
{
   struct __attribute__((packed)) {
      USHORT Limit;
      ULONG Base;
   } Idtr;
   UCHAR Mem[2];
   INT IntVect;
   ULONG IntDesc[2];
   ULONG_PTR TargetEip;

   /* Read memory */
   if (!NT_SUCCESS(KdbpSafeReadMemory(Mem, (PVOID)Eip, sizeof (Mem))))
   {
      /*KdbpPrint("Couldn't access memory at 0x%x\n", (UINT)Eip);*/
      return FALSE;
   }

   /* Check for INT instruction */
   /* FIXME: Check for iret */
   if (Mem[0] == 0xcc)
      IntVect = 3;
   else if (Mem[0] == 0xcd)
      IntVect = Mem[1];
   else if (Mem[0] == 0xce && KdbCurrentTrapFrame->Tf.Eflags & (1<<11)) /* 1 << 11 is the overflow flag */
      IntVect = 4;
   else
      return FALSE;

   if (IntVect < 32) /* We should be informed about interrupts < 32 by the kernel, no need to breakpoint them */
   {
      return FALSE;
   }

   /* Read the interrupt descriptor table register  */
   asm volatile("sidt %0" : : "m"(Idtr));
   if (IntVect >= (Idtr.Limit + 1) / 8)
   {
      /*KdbpPrint("IDT does not contain interrupt vector %d\n.", IntVect);*/
      return TRUE;
   }

   /* Get the interrupt descriptor */
   if (!NT_SUCCESS(KdbpSafeReadMemory(IntDesc, (PVOID)(Idtr.Base + (IntVect * 8)), sizeof (IntDesc))))
   {
      /*KdbpPrint("Couldn't access memory at 0x%x\n", (UINT)Idtr.Base + (IntVect * 8));*/
      return FALSE;
   }
   
   /* Check descriptor and get target eip (16 bit interrupt/trap gates not supported) */
   if ((IntDesc[1] & (1 << 15)) == 0) /* not present */
   {
      return FALSE;
   }
   if ((IntDesc[1] & 0x1f00) == 0x0500) /* Task gate */
   {
      /* FIXME: Task gates not supported */
      return FALSE;
   }
   else if (((IntDesc[1] & 0x1fe0) == 0x0e00) || /* 32 bit Interrupt gate */
            ((IntDesc[1] & 0x1fe0) == 0x0f00))   /* 32 bit Trap gate */
   {
      /* FIXME: Should the segment selector of the interrupt gate be checked? */
      TargetEip = (IntDesc[1] & 0xffff0000) | (IntDesc[0] & 0x0000ffff);
   }
   else
   {
      return FALSE;
   }

   /* Insert breakpoint */
   if (!NT_SUCCESS(KdbpInsertBreakPoint(TargetEip, KdbBreakPointTemporary, 0, 0, NULL, FALSE, NULL)))
      return FALSE;

   return TRUE;
}

/*!\brief Gets the number of the next breakpoint >= Start.
 *
 * \param Start   Breakpoint number to start searching at. -1 if no more breakpoints are found.
 *
 * \returns Breakpoint number (-1 if no more breakpoints are found)
 */
LONG
KdbpGetNextBreakPointNr(
   IN ULONG Start  OPTIONAL)
{
   for (; Start < RTL_NUMBER_OF(KdbBreakPoints); Start++)
   {
      if (KdbBreakPoints[Start].Type != KdbBreakPointNone)
         return Start;
   }
   return -1;
}

/*!\brief Returns information of the specified breakpoint.
 *
 * \param BreakPointNr         Number of the breakpoint to return information of.
 * \param Address              Receives the address of the breakpoint.
 * \param Type                 Receives the type of the breakpoint (hardware or software)
 * \param Size                 Size - for memory breakpoints.
 * \param AccessType           Access type - for hardware breakpoints.
 * \param DebugReg             Debug register - for enabled hardware breakpoints.
 * \param Enabled              Whether the breakpoint is enabled or not.
 * \param Process              The owning process of the breakpoint.
 * \param ConditionExpression  The expression which was given as condition for the bp.
 *
 * \returns NULL on failure, pointer to a KDB_BREAKPOINT struct on success.
 */
BOOLEAN
KdbpGetBreakPointInfo(
   IN  ULONG BreakPointNr,
   OUT ULONG_PTR *Address  OPTIONAL,
   OUT KDB_BREAKPOINT_TYPE *Type  OPTIONAL,
   OUT UCHAR *Size  OPTIONAL,
   OUT KDB_ACCESS_TYPE *AccessType  OPTIONAL,
   OUT UCHAR *DebugReg  OPTIONAL,
   OUT BOOLEAN *Enabled  OPTIONAL,
   OUT BOOLEAN *Global  OPTIONAL,
   OUT PEPROCESS *Process  OPTIONAL,
   OUT PCHAR *ConditionExpression  OPTIONAL)
{
   PKDB_BREAKPOINT bp;

   if (BreakPointNr >= RTL_NUMBER_OF(KdbBreakPoints) ||
       KdbBreakPoints[BreakPointNr].Type == KdbBreakPointNone)
   {
      return FALSE;
   }
   
   bp = KdbBreakPoints + BreakPointNr;
   if (Address != NULL)
      *Address = bp->Address;
   if (Type != NULL)
      *Type = bp->Type;
   if (bp->Type == KdbBreakPointHardware)
   {
      if (Size != NULL)
         *Size = bp->Data.Hw.Size;
      if (AccessType != NULL)
         *AccessType = bp->Data.Hw.AccessType;
      if (DebugReg != NULL && bp->Enabled)
         *DebugReg = bp->Data.Hw.DebugReg;
   }
   if (Enabled != NULL)
      *Enabled = bp->Enabled;
   if (Global != NULL)
      *Global = bp->Global;
   if (Process != NULL)
      *Process = bp->Process;
   if (ConditionExpression != NULL)
      *ConditionExpression = bp->ConditionExpression;

   return TRUE;
}

/*!\brief Inserts a breakpoint into the breakpoint array.
 *
 * The \a Process of the breakpoint is set to \a KdbCurrentProcess
 *
 * \param Address              Address at which to set the breakpoint.
 * \param Type                 Type of breakpoint (hardware or software)
 * \param Size                 Size of breakpoint (for hardware/memory breakpoints)
 * \param AccessType           Access type (for hardware breakpoins)
 * \param ConditionExpression  Expression which must evaluate to true for conditional breakpoints.
 * \param Global               Wether the breakpoint is global or local to a process.
 * \param BreakPointNumber     Receives the breakpoint number on success
 *
 * \returns NTSTATUS
 */
NTSTATUS
KdbpInsertBreakPoint(
   IN  ULONG_PTR Address,
   IN  KDB_BREAKPOINT_TYPE Type,
   IN  UCHAR Size  OPTIONAL,
   IN  KDB_ACCESS_TYPE AccessType  OPTIONAL,
   IN  PCHAR ConditionExpression  OPTIONAL,
   IN  BOOLEAN Global,
   OUT PULONG BreakPointNumber  OPTIONAL)
{
   LONG i;
   PVOID Condition;
   PCHAR ConditionExpressionDup;
   LONG ErrOffset;
   CHAR ErrMsg[128];

   ASSERT(Type != KdbBreakPointNone);

   if (Type == KdbBreakPointHardware)
   {
      if ((Address % Size) != 0)
      {
         KdbpPrint("Address (0x%x) must be aligned to a multiple of the size (%d)\n", Address, Size);
         return STATUS_UNSUCCESSFUL;
      }
      if (AccessType == KdbAccessExec && Size != 1)
      {
         KdbpPrint("Size must be 1 for execution breakpoints.\n");
         return STATUS_UNSUCCESSFUL;
      }
   }

   if (KdbBreakPointCount == KDB_MAXIMUM_BREAKPOINT_COUNT)
   {
      return STATUS_UNSUCCESSFUL;
   }
   
   /* Parse conditon expression string and duplicate it */
   if (ConditionExpression != NULL)
   {
      Condition = KdbpRpnParseExpression(ConditionExpression, &ErrOffset, ErrMsg);
      if (Condition == NULL)
      {
         if (ErrOffset >= 0)
            KdbpPrint("Couldn't parse expression: %s at character %d\n", ErrMsg, ErrOffset);
         else
            KdbpPrint("Couldn't parse expression: %s", ErrMsg);
         return STATUS_UNSUCCESSFUL;
      }

      i = strlen(ConditionExpression) + 1;
      ConditionExpressionDup = ExAllocatePoolWithTag(NonPagedPool, i, TAG_KDBG);
      RtlCopyMemory(ConditionExpressionDup, ConditionExpression, i);

   }
   else
   {
      Condition = NULL;
      ConditionExpressionDup = NULL;
   }

   /* Find unused breakpoint */
   if (Type == KdbBreakPointTemporary)
   {
      for (i = RTL_NUMBER_OF(KdbBreakPoints) - 1; i >= 0; i--)
      {
         if (KdbBreakPoints[i].Type == KdbBreakPointNone)
            break;
      }
   }
   else
   {
      for (i = 0; i < RTL_NUMBER_OF(KdbBreakPoints); i++)
      {
         if (KdbBreakPoints[i].Type == KdbBreakPointNone)
            break;
      }
   }
   ASSERT(i < RTL_NUMBER_OF(KdbBreakPoints));
   
   /* Set the breakpoint */
   ASSERT(KdbCurrentProcess != NULL);
   KdbBreakPoints[i].Type = Type;
   KdbBreakPoints[i].Address = Address;
   KdbBreakPoints[i].Enabled = FALSE;
   KdbBreakPoints[i].Global = Global;
   KdbBreakPoints[i].Process = KdbCurrentProcess;
   KdbBreakPoints[i].ConditionExpression = ConditionExpressionDup;
   KdbBreakPoints[i].Condition = Condition;
   if (Type == KdbBreakPointHardware)
   {

      KdbBreakPoints[i].Data.Hw.Size = Size;
      KdbBreakPoints[i].Data.Hw.AccessType = AccessType;
   }
   KdbBreakPointCount++;
   
   if (Type != KdbBreakPointTemporary)
      KdbpPrint("Breakpoint %d inserted.\n", i);

   /* Try to enable the breakpoint */
   KdbpEnableBreakPoint(i, NULL);

   /* Return the breakpoint number */
   if (BreakPointNumber != NULL)
      *BreakPointNumber = i;

   return STATUS_SUCCESS;
}

/*!\brief Deletes a breakpoint
 *
 * \param BreakPointNr  Number of the breakpoint to delete. Can be -1
 * \param BreakPoint    Breakpoint to delete. Can be NULL.
 *
 * \retval TRUE   Success.
 * \retval FALSE  Failure (invalid breakpoint number)
 */
BOOLEAN
KdbpDeleteBreakPoint(
   IN LONG BreakPointNr  OPTIONAL,
   IN OUT PKDB_BREAKPOINT BreakPoint  OPTIONAL)
{
   if (BreakPointNr < 0)
   {
      ASSERT(BreakPoint != NULL);
      BreakPointNr = BreakPoint - KdbBreakPoints;
   }
   if (BreakPointNr < 0 || BreakPointNr >= KDB_MAXIMUM_BREAKPOINT_COUNT)
   {
      KdbpPrint("Invalid breakpoint: %d\n", BreakPointNr);
      return FALSE;
   }
   if (BreakPoint == NULL)
   {
      BreakPoint = KdbBreakPoints + BreakPointNr;
   }
   if (BreakPoint->Type == KdbBreakPointNone)
   {
      KdbpPrint("Invalid breakpoint: %d\n", BreakPointNr);
      return FALSE;
   }

   if (BreakPoint->Enabled &&
       !KdbpDisableBreakPoint(-1, BreakPoint))
      return FALSE;

   if (BreakPoint->Type != KdbBreakPointTemporary)
      KdbpPrint("Breakpoint %d deleted.\n", BreakPointNr);
   BreakPoint->Type = KdbBreakPointNone;
   KdbBreakPointCount--;
   
   return TRUE;
}

/*!\brief Checks if the breakpoint was set by the debugger
 *
 * Tries to find a breakpoint in the breakpoint array which caused
 * the debug exception to happen.
 *
 * \param ExpNr      Exception Number (1 or 3)
 * \param TrapFrame  Exception trapframe
 *
 * \returns Breakpoint number, -1 on error.
 */
STATIC LONG
KdbpIsBreakPointOurs(
   IN ULONG ExpNr,
   IN PKTRAP_FRAME TrapFrame)
{
   INT i;
   ASSERT(ExpNr == 1 || ExpNr == 3);

   if (ExpNr == 3) /* Software interrupt */
   {
      ULONG_PTR BpEip = (ULONG_PTR)TrapFrame->Eip - 1; /* Get EIP of INT3 instruction */
      for (i = 0; i < KdbSwBreakPointCount; i++)
      {
         ASSERT((KdbSwBreakPoints[i]->Type == KdbBreakPointSoftware ||
                 KdbSwBreakPoints[i]->Type == KdbBreakPointTemporary));
         ASSERT(KdbSwBreakPoints[i]->Enabled);
         if (KdbSwBreakPoints[i]->Address == BpEip)
         {
            return KdbSwBreakPoints[i] - KdbBreakPoints;
         }
      }
   }
   else if (ExpNr == 1) /* Hardware interrupt */
   {
      UCHAR DebugReg;
      for (i = 0; i < KdbHwBreakPointCount; i++)
      {
         ASSERT(KdbHwBreakPoints[i]->Type == KdbBreakPointHardware &&
                KdbHwBreakPoints[i]->Enabled);
         DebugReg = KdbHwBreakPoints[i]->Data.Hw.DebugReg;
         if ((TrapFrame->Dr6 & (1 << DebugReg)) != 0)
         {
            return KdbHwBreakPoints[i] - KdbBreakPoints;
         }
      }
   }
   
   return -1;
}

/*!\brief Enables a breakpoint.
 *
 * \param BreakPointNr  Number of the breakpoint to enable Can be -1.
 * \param BreakPoint    Breakpoint to enable. Can be NULL.
 *
 * \retval TRUE   Success.
 * \retval FALSE  Failure.
 *
 * \sa KdbpDisableBreakPoint
 */
BOOLEAN
KdbpEnableBreakPoint(
   IN LONG BreakPointNr  OPTIONAL,
   IN OUT PKDB_BREAKPOINT BreakPoint  OPTIONAL)
{
   NTSTATUS Status;
   INT i;
   ULONG ul;

   if (BreakPointNr < 0)
   {
      ASSERT(BreakPoint != NULL);
      BreakPointNr = BreakPoint - KdbBreakPoints;
   }
   if (BreakPointNr < 0 || BreakPointNr >= KDB_MAXIMUM_BREAKPOINT_COUNT)
   {
      KdbpPrint("Invalid breakpoint: %d\n", BreakPointNr);
      return FALSE;
   }
   if (BreakPoint == NULL)
   {
      BreakPoint = KdbBreakPoints + BreakPointNr;
   }
   if (BreakPoint->Type == KdbBreakPointNone)
   {
      KdbpPrint("Invalid breakpoint: %d\n", BreakPointNr);
      return FALSE;
   }
   
   if (BreakPoint->Enabled == TRUE)
   {
      KdbpPrint("Breakpoint %d is already enabled.\n", BreakPointNr);
      return TRUE;
   }

   if (BreakPoint->Type == KdbBreakPointSoftware ||
       BreakPoint->Type == KdbBreakPointTemporary)
   {
      if (KdbSwBreakPointCount >= KDB_MAXIMUM_SW_BREAKPOINT_COUNT)
      {
         KdbpPrint("Maximum number of SW breakpoints (%d) used. "
                   "Disable another breakpoint in order to enable this one.\n",
                   KDB_MAXIMUM_SW_BREAKPOINT_COUNT);
         return FALSE;
      }
      Status = KdbpOverwriteInstruction(BreakPoint->Process, BreakPoint->Address,
                                        0xCC, &BreakPoint->Data.SavedInstruction);
      if (!NT_SUCCESS(Status))
      {
         KdbpPrint("Couldn't access memory at 0x%x\n", BreakPoint->Address);
         return FALSE;
      }
      KdbSwBreakPoints[KdbSwBreakPointCount++] = BreakPoint;
   }
   else
   {
      if (BreakPoint->Data.Hw.AccessType == KdbAccessExec)
         ASSERT(BreakPoint->Data.Hw.Size == 1);
      ASSERT((BreakPoint->Address % BreakPoint->Data.Hw.Size) == 0);
      if (KdbHwBreakPointCount >= KDB_MAXIMUM_HW_BREAKPOINT_COUNT)
      {
         KdbpPrint("Maximum number of HW breakpoints (%d) already used. "
                   "Disable another breakpoint in order to enable this one.\n",
                   KDB_MAXIMUM_HW_BREAKPOINT_COUNT);
         return FALSE;
      }

      /* Find unused hw breakpoint */
      ASSERT(KDB_MAXIMUM_HW_BREAKPOINT_COUNT == 4);
      for (i = 0; i < KDB_MAXIMUM_HW_BREAKPOINT_COUNT; i++)
      {
         if ((KdbTrapFrame.Tf.Dr7 & (0x3 << (i * 2))) == 0)
            break;
      }
      ASSERT(i < KDB_MAXIMUM_HW_BREAKPOINT_COUNT);

      /* Set the breakpoint address. */
      switch (i)
      {
      case 0:
         KdbTrapFrame.Tf.Dr0 = BreakPoint->Address;
         break;
      case 1:
         KdbTrapFrame.Tf.Dr1 = BreakPoint->Address;
         break;
      case 2:
         KdbTrapFrame.Tf.Dr2 = BreakPoint->Address;
         break;
      case 3:
         KdbTrapFrame.Tf.Dr3 = BreakPoint->Address;
         break;
      }

      /* Enable the global breakpoint */
      KdbTrapFrame.Tf.Dr7 |= (0x2 << (i * 2));

      /* Enable the exact match bits. */
      KdbTrapFrame.Tf.Dr7 |= 0x00000300;

      /* Clear existing state. */
      KdbTrapFrame.Tf.Dr7 &= ~(0xF << (16 + (i * 4)));

      /* Set the breakpoint type. */
      switch (BreakPoint->Data.Hw.AccessType)
      {
      case KdbAccessExec:
         ul = 0;
         break;
      case KdbAccessWrite:
         ul = 1;
         break;
      case KdbAccessRead:
      case KdbAccessReadWrite:
         ul = 3;
         break;
      default:
         ASSERT(0);
         return TRUE;
         break;
      }
      KdbTrapFrame.Tf.Dr7 |= (ul << (16 + (i * 4)));

      /* Set the breakpoint length. */
      KdbTrapFrame.Tf.Dr7 |= ((BreakPoint->Data.Hw.Size - 1) << (18 + (i * 4)));

      /* Update KdbCurrentTrapFrame - values are taken from there by the CLI */
      if (&KdbTrapFrame != KdbCurrentTrapFrame)
      {
         KdbCurrentTrapFrame->Tf.Dr0 = KdbTrapFrame.Tf.Dr0;
         KdbCurrentTrapFrame->Tf.Dr1 = KdbTrapFrame.Tf.Dr1;
         KdbCurrentTrapFrame->Tf.Dr2 = KdbTrapFrame.Tf.Dr2;
         KdbCurrentTrapFrame->Tf.Dr3 = KdbTrapFrame.Tf.Dr3;
         KdbCurrentTrapFrame->Tf.Dr6 = KdbTrapFrame.Tf.Dr6;
         KdbCurrentTrapFrame->Tf.Dr7 = KdbTrapFrame.Tf.Dr7;
      }

      BreakPoint->Data.Hw.DebugReg = i;
      KdbHwBreakPoints[KdbHwBreakPointCount++] = BreakPoint;
   }

   BreakPoint->Enabled = TRUE;
   if (BreakPoint->Type != KdbBreakPointTemporary)
      KdbpPrint("Breakpoint %d enabled.\n", BreakPointNr);
   return TRUE;
}

/*!\brief Disables a breakpoint.
 *
 * \param BreakPointNr  Number of the breakpoint to disable. Can be -1
 * \param BreakPoint    Breakpoint to disable. Can be NULL.
 *
 * \retval TRUE   Success.
 * \retval FALSE  Failure.
 *
 * \sa KdbpEnableBreakPoint
 */
BOOLEAN
KdbpDisableBreakPoint(
   IN LONG BreakPointNr  OPTIONAL,
   IN OUT PKDB_BREAKPOINT BreakPoint  OPTIONAL)
{
   INT i;
   NTSTATUS Status;
   
   if (BreakPointNr < 0)
   {
      ASSERT(BreakPoint != NULL);
      BreakPointNr = BreakPoint - KdbBreakPoints;
   }
   if (BreakPointNr < 0 || BreakPointNr >= KDB_MAXIMUM_BREAKPOINT_COUNT)
   {
      KdbpPrint("Invalid breakpoint: %d\n", BreakPointNr);
      return FALSE;
   }
   if (BreakPoint == NULL)
   {
      BreakPoint = KdbBreakPoints + BreakPointNr;
   }
   if (BreakPoint->Type == KdbBreakPointNone)
   {
      KdbpPrint("Invalid breakpoint: %d\n", BreakPointNr);
      return FALSE;
   }

   if (BreakPoint->Enabled == FALSE)
   {
      KdbpPrint("Breakpoint %d is not enabled.\n", BreakPointNr);
      return TRUE;
   }

   if (BreakPoint->Type == KdbBreakPointSoftware ||
       BreakPoint->Type == KdbBreakPointTemporary)
   {
      ASSERT(KdbSwBreakPointCount > 0);
      Status = KdbpOverwriteInstruction(BreakPoint->Process, BreakPoint->Address,
                                        BreakPoint->Data.SavedInstruction, NULL);
      if (!NT_SUCCESS(Status))
      {
         KdbpPrint("Couldn't restore original instruction.\n");
         return FALSE;
      }
         
      for (i = 0; i < KdbSwBreakPointCount; i++)
      {
         if (KdbSwBreakPoints[i] == BreakPoint)
         {
            KdbSwBreakPoints[i] = KdbSwBreakPoints[--KdbSwBreakPointCount];
            i = -1; /* if the last breakpoint is disabled dont break with i >= KdbSwBreakPointCount */
            break;
         }
      }
      if (i != -1) /* not found */
         ASSERT(0);
   }
   else
   {
      ASSERT(BreakPoint->Type == KdbBreakPointHardware);
      
      /* Clear the breakpoint. */
      KdbTrapFrame.Tf.Dr7 &= ~(0x3 << (BreakPoint->Data.Hw.DebugReg * 2));
      if ((KdbTrapFrame.Tf.Dr7 & 0xFF) == 0)
      {
         /*
          * If no breakpoints are enabled then clear the exact match flags.
          */
         KdbTrapFrame.Tf.Dr7 &= 0xFFFFFCFF;
      }

      for (i = 0; i < KdbHwBreakPointCount; i++)
      {
         if (KdbHwBreakPoints[i] == BreakPoint)
         {
            KdbHwBreakPoints[i] = KdbHwBreakPoints[--KdbHwBreakPointCount];
            i = -1; /* if the last breakpoint is disabled dont break with i >= KdbHwBreakPointCount */
            break;
         }
      }
      if (i != -1) /* not found */
         ASSERT(0);
   }

   BreakPoint->Enabled = FALSE;
   if (BreakPoint->Type != KdbBreakPointTemporary)
      KdbpPrint("Breakpoint %d disabled.\n", BreakPointNr);
   return TRUE;
}

/*!\brief Gets the first or last chance enter-condition for exception nr. \a ExceptionNr
 *
 * \param ExceptionNr  Number of the exception to get condition of.
 * \param FirstChance  Whether to get first or last chance condition.
 * \param Condition    Receives the condition setting.
 *
 * \retval TRUE   Success.
 * \retval FALSE  Failure (invalid exception nr)
 */
BOOLEAN
KdbpGetEnterCondition(
   IN LONG ExceptionNr,
   IN BOOLEAN FirstChance,
   OUT KDB_ENTER_CONDITION *Condition)
{
   if (ExceptionNr >= RTL_NUMBER_OF(KdbEnterConditions))
      return FALSE;

   *Condition = KdbEnterConditions[ExceptionNr][FirstChance ? 0 : 1];
   return TRUE;
}

/*!\brief Sets the first or last chance enter-condition for exception nr. \a ExceptionNr
 *
 * \param ExceptionNr  Number of the exception to set condition of (-1 for all)
 * \param FirstChance  Whether to set first or last chance condition.
 * \param Condition    The new condition setting.
 *
 * \retval TRUE   Success.
 * \retval FALSE  Failure (invalid exception nr)
 */
BOOLEAN
KdbpSetEnterCondition(
   IN LONG ExceptionNr,
   IN BOOLEAN FirstChance,
   IN KDB_ENTER_CONDITION Condition)
{
   if (ExceptionNr < 0)
   {
      for (ExceptionNr = 0; ExceptionNr < RTL_NUMBER_OF(KdbEnterConditions); ExceptionNr++)
      {
         if (ExceptionNr == 1 || ExceptionNr == 8 ||
             ExceptionNr == 9 || ExceptionNr == 15) /* Reserved exceptions */
         {
            continue;
         }
         KdbEnterConditions[ExceptionNr][FirstChance ? 0 : 1] = Condition;
      }
   }
   else
   {
      if (ExceptionNr >= RTL_NUMBER_OF(KdbEnterConditions) ||
          ExceptionNr == 1 || ExceptionNr == 8 || /* Do not allow changing of the debug */
          ExceptionNr == 9 || ExceptionNr == 15)  /* trap or reserved exceptions */
      {
         return FALSE;
      }
      KdbEnterConditions[ExceptionNr][FirstChance ? 0 : 1] = Condition;
   }
   return TRUE;
}

/*!\brief Switches to another thread context
 *
 * \param ThreadId  Id of the thread to switch to.
 *
 * \retval TRUE   Success.
 * \retval FALSE  Failure (i.e. invalid thread id)
 */
BOOLEAN
KdbpAttachToThread(
   PVOID ThreadId)
{
   PETHREAD Thread = NULL;
   PEPROCESS Process;

   /* Get a pointer to the thread */
   if (!NT_SUCCESS(PsLookupThreadByThreadId(ThreadId, &Thread)))
   {
      KdbpPrint("Invalid thread id: 0x%08x\n", (UINT)ThreadId);
      return FALSE;
   }
   Process = Thread->ThreadsProcess;

   if (KeIsExecutingDpc() && Process != KdbCurrentProcess)
   {
      KdbpPrint("Cannot attach to thread within another process while executing a DPC.\n");
      return FALSE;
   }
   
   /* Save the current thread's context (if we previously attached to a thread) */
   if (KdbCurrentThread != KdbOriginalThread)
   {
      ASSERT(KdbCurrentTrapFrame == &KdbThreadTrapFrame);
      RtlCopyMemory(KdbCurrentThread->Tcb.TrapFrame, &KdbCurrentTrapFrame->Tf, sizeof (KTRAP_FRAME));
   }
   else
   {
      ASSERT(KdbCurrentTrapFrame == &KdbTrapFrame);
   }

   /* Switch to the thread's context */
   if (Thread != KdbOriginalThread)
   {
      ASSERT(Thread->Tcb.TrapFrame != NULL);
      RtlCopyMemory(&KdbThreadTrapFrame.Tf, Thread->Tcb.TrapFrame, sizeof (KTRAP_FRAME));
      asm volatile(
         "movl %%cr0, %0"    "\n\t"
         "movl %%cr2, %1"    "\n\t"
         "movl %%cr3, %2"    "\n\t"
         "movl %%cr4, %3"    "\n\t"
         : "=r"(KdbTrapFrame.Cr0), "=r"(KdbTrapFrame.Cr2),
           "=r"(KdbTrapFrame.Cr3), "=r"(KdbTrapFrame.Cr4));
      KdbCurrentTrapFrame = &KdbThreadTrapFrame;
   }
   else /* Switching back to original thread */
   {
      KdbCurrentTrapFrame = &KdbTrapFrame;
   }
   KdbCurrentThread = Thread;

   /* Attach to the thread's process */
   ASSERT(KdbCurrentProcess == PsGetCurrentProcess());
   if (KdbCurrentProcess != Process)
   {
      if (KdbCurrentProcess != KdbOriginalProcess) /* detach from previously attached process */
      {
         KeUnstackDetachProcess(&KdbApcState);
      }
      if (KdbOriginalProcess != Process)
      {
         KeStackAttachProcess(EPROCESS_TO_KPROCESS(Process), &KdbApcState);
      }
      KdbCurrentProcess = Process;
   }

   return TRUE;
}

/*!\brief Switches to another process/thread context
 *
 * This function switches to the first thread in the specified process.
 *
 * \param ProcessId  Id of the process to switch to.
 *
 * \retval TRUE   Success.
 * \retval FALSE  Failure (i.e. invalid process id)
 */
BOOLEAN
KdbpAttachToProcess(
   PVOID ProcessId)
{
   PEPROCESS Process = NULL;
   PETHREAD Thread;
   PLIST_ENTRY Entry;

   /* Get a pointer to the process */
   if (!NT_SUCCESS(PsLookupProcessByProcessId(ProcessId, &Process)))
   {
      KdbpPrint("Invalid process id: 0x%08x\n", (UINT)ProcessId);
      return FALSE;
   }

   Entry = Process->ThreadListHead.Flink;
   if (Entry == &KdbCurrentProcess->ThreadListHead)
   {
      KdbpPrint("No threads in process 0x%08x, cannot attach to process!\n", (UINT)ProcessId);
      return FALSE;
   }

   Thread = CONTAINING_RECORD(Entry, ETHREAD, ThreadListEntry);

   return KdbpAttachToThread(Thread->Cid.UniqueThread);
}

/*!\brief Calls the main loop ...
 */
STATIC VOID
KdbpCallMainLoop()
{
   KdbpCliMainLoop(KdbEnteredOnSingleStep);
}

/*!\brief Internal function to enter KDB.
 *
 * Disables interrupts, releases display ownership, ...
 */
STATIC VOID
KdbpInternalEnter()
{  
   PETHREAD Thread;
   PVOID SavedInitialStack, SavedStackBase, SavedKernelStack;
   ULONG SavedStackLimit;
   
   KbdDisableMouse();
   if (KdDebugState & KD_DEBUG_SCREEN)
   {
      HalReleaseDisplayOwnership();
   }

   /* Call the interface's main loop on a different stack */
   Thread = PsGetCurrentThread();
   SavedInitialStack = Thread->Tcb.InitialStack;
   SavedStackBase = Thread->Tcb.StackBase;
   SavedStackLimit = Thread->Tcb.StackLimit;
   SavedKernelStack = Thread->Tcb.KernelStack;
   Thread->Tcb.InitialStack = Thread->Tcb.StackBase = (char*)KdbStack + KDB_STACK_SIZE;
   Thread->Tcb.StackLimit = (ULONG)KdbStack;
   Thread->Tcb.KernelStack = (char*)KdbStack + KDB_STACK_SIZE;

   /*KdbpPrint("Switching to KDB stack 0x%08x-0x%08x\n", Thread->Tcb.StackLimit, Thread->Tcb.StackBase);*/

   KdbpStackSwitchAndCall(Thread->Tcb.KernelStack, KdbpCallMainLoop);

   Thread->Tcb.InitialStack = SavedInitialStack;
   Thread->Tcb.StackBase = SavedStackBase;
   Thread->Tcb.StackLimit = SavedStackLimit;
   Thread->Tcb.KernelStack = SavedKernelStack;
   KbdEnableMouse();
}

/*!\brief KDB Exception filter
 *
 * Called by the exception dispatcher.
 *
 * \param ExceptionRecord  Unused.
 * \param PreviousMode     UserMode if the exception was raised from umode, otherwise KernelMode.
 * \param Context          Unused.
 * \param TrapFrame        Exception TrapFrame.
 * \param FirstChance      TRUE when called before exception frames were serached,
 *                         FALSE for the second call.
 *
 * \returns KD_CONTINUE_TYPE
 */
KD_CONTINUE_TYPE
KdbEnterDebuggerException(
   IN PEXCEPTION_RECORD ExceptionRecord  OPTIONAL,
   IN KPROCESSOR_MODE PreviousMode,
   IN PCONTEXT Context  OPTIONAL,
   IN OUT PKTRAP_FRAME TrapFrame,
   IN BOOLEAN FirstChance)
{
   ULONG ExpNr = (ULONG)TrapFrame->DebugArgMark;
   KDB_ENTER_CONDITION EnterCondition;
   KD_CONTINUE_TYPE ContinueType = kdHandleException;
   PKDB_BREAKPOINT BreakPoint;
   ULONG ul;
   ULONGLONG ull;
   BOOLEAN Resume = FALSE;
   BOOLEAN EnterConditionMet = TRUE;
   ULONG OldEflags;

   KdbCurrentProcess = PsGetCurrentProcess();

   /* Set continue type to kdContinue for single steps and breakpoints */
   if (ExpNr == 1 || ExpNr == 3)
      ContinueType = kdContinue;

   /* Check if we should handle the exception. */
   ul = min(ExpNr, RTL_NUMBER_OF(KdbEnterConditions) - 1);
   EnterCondition = KdbEnterConditions[ul][FirstChance ? 0 : 1];
   if (EnterCondition == KdbDoNotEnter ||
       (EnterCondition == KdbEnterFromUmode && PreviousMode != UserMode) ||
       (EnterCondition == KdbEnterFromKmode && PreviousMode != KernelMode))
   {
      EnterConditionMet = FALSE;
   }

   /* If we stopped on one of our breakpoints then let the user know. */
   KdbLastBreakPointNr = -1;
   KdbEnteredOnSingleStep = FALSE;

   if (FirstChance && (ExpNr == 1 || ExpNr == 3) &&
       (KdbLastBreakPointNr = KdbpIsBreakPointOurs(ExpNr, TrapFrame)) >= 0)
   {
      BreakPoint = KdbBreakPoints + KdbLastBreakPointNr;

      if (ExpNr == 3)
      {
         /*
          * The breakpoint will point to the next instruction by default so
          * point it back to the start of original instruction.
          */
         TrapFrame->Eip--;

         /*
          * ... and restore the original instruction.
          */
         if (!NT_SUCCESS(KdbpOverwriteInstruction(KdbCurrentProcess, BreakPoint->Address,
                                                  BreakPoint->Data.SavedInstruction, NULL)))
         {
            DbgPrint("Couldn't restore original instruction after INT3! Cannot continue execution.\n");
            KEBUGCHECK(0);
         }
      }

      if ((BreakPoint->Type == KdbBreakPointHardware) &&
          (BreakPoint->Data.Hw.AccessType == KdbAccessExec))
      {
         Resume = TRUE; /* Set the resume flag when continuing execution */
      }
      
      /*
       * When a temporary breakpoint is hit we have to make sure that we are
       * in the same context in which it was set, otherwise it could happen
       * that another process/thread hits it before and it gets deleted.
       */
      else if (BreakPoint->Type == KdbBreakPointTemporary &&
               BreakPoint->Process == KdbCurrentProcess)
      {
         ASSERT((TrapFrame->Eflags & X86_EFLAGS_TF) == 0);
         
         /*
          * Delete the temporary breakpoint which was used to step over or into the instruction.
          */
         KdbpDeleteBreakPoint(-1, BreakPoint);

         if (--KdbNumSingleSteps > 0)
         {
            if ((KdbSingleStepOver && !KdbpStepOverInstruction(TrapFrame->Eip)) ||
                (!KdbSingleStepOver && !KdbpStepIntoInstruction(TrapFrame->Eip)))
            {
               TrapFrame->Eflags |= X86_EFLAGS_TF;
            }
            goto continue_execution; /* return */
         }

         KdbEnteredOnSingleStep = TRUE;
      }

      /*
       * If we hit a breakpoint set by the debugger we set the single step flag,
       * ignore the next single step and reenable the breakpoint.
       */
      else if (BreakPoint->Type == KdbBreakPointSoftware ||
               BreakPoint->Type == KdbBreakPointTemporary)
      {
         ASSERT(ExpNr == 3);
         TrapFrame->Eflags |= X86_EFLAGS_TF;
         KdbBreakPointToReenable = BreakPoint;
      }
      
      /*
       * Make sure that the breakpoint should be triggered in this context
       */
      if (!BreakPoint->Global && BreakPoint->Process != KdbCurrentProcess)
      {
            goto continue_execution; /* return */
      }
      
      /*
       * Check if the condition for the breakpoint is met.
       */
      if (BreakPoint->Condition != NULL)
      {
         /* Setup the KDB trap frame */
         RtlCopyMemory(&KdbTrapFrame.Tf, TrapFrame, sizeof (KTRAP_FRAME));
         asm volatile(
            "movl %%cr0, %0"    "\n\t"
            "movl %%cr2, %1"    "\n\t"
            "movl %%cr3, %2"    "\n\t"
            "movl %%cr4, %3"    "\n\t"
            : "=r"(KdbTrapFrame.Cr0), "=r"(KdbTrapFrame.Cr2),
              "=r"(KdbTrapFrame.Cr3), "=r"(KdbTrapFrame.Cr4));

         ull = 0;
         if (!KdbpRpnEvaluateParsedExpression(BreakPoint->Condition, &KdbTrapFrame, &ull, NULL, NULL))
         {
            /* FIXME: Print warning? */
         }
         else if (ull == 0) /* condition is not met */
         {
            goto continue_execution; /* return */
         }
      }

      if (BreakPoint->Type == KdbBreakPointSoftware)
      {
         DbgPrint("Entered debugger on breakpoint #%d: EXEC 0x%04x:0x%08x\n",
                  KdbLastBreakPointNr, TrapFrame->Cs & 0xffff, TrapFrame->Eip);
      }
      else if (BreakPoint->Type == KdbBreakPointHardware)
      {
         DbgPrint("Entered debugger on breakpoint #%d: %s 0x%08x\n",
                  KdbLastBreakPointNr,
                  (BreakPoint->Data.Hw.AccessType == KdbAccessRead) ? "READ" :
                  ((BreakPoint->Data.Hw.AccessType == KdbAccessWrite) ? "WRITE" :
                   ((BreakPoint->Data.Hw.AccessType == KdbAccessReadWrite) ? "RDWR" : "EXEC")
                  ),
                  BreakPoint->Address
                 );

      }
   }
   else if (ExpNr == 1)
   {
      /* Silently ignore a debugger initiated single step. */
      if ((TrapFrame->Dr6 & 0xf) == 0 && KdbBreakPointToReenable != NULL)
      {
         /* FIXME: Make sure that the breakpoint was really hit (check bp->Address vs. tf->Eip) */
         BreakPoint = KdbBreakPointToReenable;
         KdbBreakPointToReenable = NULL;
         ASSERT(BreakPoint->Type == KdbBreakPointSoftware ||
                BreakPoint->Type == KdbBreakPointTemporary);

         /*
          * Reenable the breakpoint we disabled to execute the breakpointed
          * instruction.
          */
         if (!NT_SUCCESS(KdbpOverwriteInstruction(KdbCurrentProcess, BreakPoint->Address, 0xCC,
                                                  &BreakPoint->Data.SavedInstruction)))
         {
            DbgPrint("Warning: Couldn't reenable breakpoint %d\n",
                     BreakPoint - KdbBreakPoints);
         }

         /* Unset TF if we are no longer single stepping. */
         if (KdbNumSingleSteps == 0)
            TrapFrame->Eflags &= ~X86_EFLAGS_TF;
         goto continue_execution; /* return */
      }

      /* Check if we expect a single step */
      if ((TrapFrame->Dr6 & 0xf) == 0 && KdbNumSingleSteps > 0)
      {
         /*ASSERT((TrapFrame->Eflags & X86_EFLAGS_TF) != 0);*/
         if (--KdbNumSingleSteps > 0)
         {
            if ((KdbSingleStepOver && KdbpStepOverInstruction(TrapFrame->Eip)) ||
                (!KdbSingleStepOver && KdbpStepIntoInstruction(TrapFrame->Eip)))
            {
               TrapFrame->Eflags &= ~X86_EFLAGS_TF;
            }
            else
            {
               TrapFrame->Eflags |= X86_EFLAGS_TF;
            }
            goto continue_execution; /* return */
         }

         TrapFrame->Eflags &= ~X86_EFLAGS_TF;
         KdbEnteredOnSingleStep = TRUE;
      }
      else
      {
         if (!EnterConditionMet)
         {
            return ContinueType;
         }
         DbgPrint("Entered debugger on unexpected debug trap!\n");
      }
   }
   else if (ExpNr == 3)
   {
      if (KdbInitFileBuffer != NULL)
      {
         KdbpCliInterpretInitFile();
         EnterConditionMet = FALSE;
      }
      if (!EnterConditionMet)
      {
         return ContinueType;
      }

      DbgPrint("Entered debugger on embedded INT3 at 0x%04x:0x%08x.\n",
               TrapFrame->Cs & 0xffff, TrapFrame->Eip - 1);
   }
   else
   {
      CONST PCHAR ExceptionString = (ExpNr < RTL_NUMBER_OF(ExceptionNrToString)) ?
                                    (ExceptionNrToString[ExpNr]) :
                                    ("Unknown/User defined exception");

      if (!EnterConditionMet)
      {
         return ContinueType;
      }

      DbgPrint("Entered debugger on %s-chance exception number %d (%s)\n",
               FirstChance ? "first" : "last", ExpNr, ExceptionString);
      if (ExpNr == 14)
      {
         /* FIXME: Add noexec memory stuff */
         ULONG Cr2, Err;
         asm volatile("movl %%cr2, %0" : "=r"(Cr2));
         Err = TrapFrame->ErrorCode;
         DbgPrint("Memory at 0x%x could not be %s: ", Cr2, (Err & (1 << 1)) ? "written" : "read");
         if ((Err & (1 << 0)) == 0)
            DbgPrint("Page not present.\n");
         else
         {
            if ((Err & (1 << 3)) != 0)
               DbgPrint("Reserved bits in page directory set.\n");
            else
               DbgPrint("Page protection violation.\n");
         }
      }
   }
   
   /* Once we enter the debugger we do not expect any more single steps to happen */
   KdbNumSingleSteps = 0;
   
   /* Update the current process pointer */
   KdbCurrentProcess = KdbOriginalProcess = PsGetCurrentProcess();
   KdbCurrentThread = KdbOriginalThread = PsGetCurrentThread();
   KdbCurrentTrapFrame = &KdbTrapFrame;

   /* Setup the KDB trap frame */
   RtlCopyMemory(&KdbTrapFrame.Tf, TrapFrame, sizeof(KTRAP_FRAME));
   asm volatile(
      "movl %%cr0, %0"    "\n\t"
      "movl %%cr2, %1"    "\n\t"
      "movl %%cr3, %2"    "\n\t"
      "movl %%cr4, %3"    "\n\t"
      : "=r"(KdbTrapFrame.Cr0), "=r"(KdbTrapFrame.Cr2),
        "=r"(KdbTrapFrame.Cr3), "=r"(KdbTrapFrame.Cr4));

   /* Enter critical section */
   Ke386SaveFlags(OldEflags);
   Ke386DisableInterrupts();

   /* Exception inside the debugger? Game over. */
   if (InterlockedIncrement(&KdbEntryCount) > 1)
   {
      Ke386RestoreFlags(OldEflags);
      return kdHandleException;
   }

   /* Call the main loop. */
   KdbpInternalEnter();

   /* Check if we should single step */
   if (KdbNumSingleSteps > 0)
   {
      if ((KdbSingleStepOver && KdbpStepOverInstruction(KdbCurrentTrapFrame->Tf.Eip)) ||
          (!KdbSingleStepOver && KdbpStepIntoInstruction(KdbCurrentTrapFrame->Tf.Eip)))
      {
         ASSERT((KdbCurrentTrapFrame->Tf.Eflags & X86_EFLAGS_TF) == 0);
         /*KdbCurrentTrapFrame->Tf.Eflags &= ~X86_EFLAGS_TF;*/
      }
      else
      {
         KdbCurrentTrapFrame->Tf.Eflags |= X86_EFLAGS_TF;
      }
   }

   /* Save the current thread's trapframe */
   if (KdbCurrentTrapFrame == &KdbThreadTrapFrame)
   {
      RtlCopyMemory(KdbCurrentThread->Tcb.TrapFrame, KdbCurrentTrapFrame, sizeof (KTRAP_FRAME));
   }

   /* Detach from attached process */
   if (KdbCurrentProcess != KdbOriginalProcess)
   {
      KeUnstackDetachProcess(&KdbApcState);
   }

   /* Update the exception TrapFrame */
   RtlCopyMemory(TrapFrame, &KdbTrapFrame.Tf, sizeof(KTRAP_FRAME));
#if 0
   asm volatile(
      "movl %0, %%cr0"    "\n\t"
      "movl %1, %%cr2"    "\n\t"
      "movl %2, %%cr3"    "\n\t"
      "movl %3, %%cr4"    "\n\t"
      : : "r"(KdbTrapFrame.Cr0), "r"(KdbTrapFrame.Cr2),
          "r"(KdbTrapFrame.Cr3), "r"(KdbTrapFrame.Cr4));
#endif

   /* Decrement the entry count */
   InterlockedDecrement(&KdbEntryCount);

   /* Leave critical section */
   Ke386RestoreFlags(OldEflags);

continue_execution:
   /* Clear debug status */
   if (ExpNr == 1 || ExpNr == 3) /* FIXME: Why clear DR6 on INT3? */
   {
      /* Set the RF flag so we don't trigger the same breakpoint again. */
      if (Resume)
      {
         TrapFrame->Eflags |= X86_EFLAGS_RF;
      }
         
      /* Clear dr6 status flags. */
      TrapFrame->Dr6 &= ~0x0000e00f;

   }

   return ContinueType;
}

VOID
KdbInit()
{
   KdbpCliInit();
}

VOID
KdbDeleteProcessHook(IN PEPROCESS Process)
{
   KdbSymFreeProcessSymbols(Process);
   
   /* FIXME: Delete breakpoints for process */
}

VOID
KdbModuleLoaded(IN PUNICODE_STRING Name)
{
   KdbpCliModuleLoaded(Name);
}


