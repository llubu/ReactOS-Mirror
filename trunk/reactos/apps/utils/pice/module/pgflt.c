/*++

Copyright (c) 1998-2001 Klaus P. Gerlicher

Module Name:

    pgflt.c

Abstract:

    page fault handling on x86

Environment:

    Kernel mode only

Author:

    Klaus P. Gerlicher

Revision History:

    25-Nov-1999:	created
    15-Nov-2000:    general cleanup of source files

Copyright notice:

  This file may be distributed under the terms of the GNU Public License.

--*/

////////////////////////////////////////////////////
// INCLUDES
////
#include "remods.h"

#include "precomp.h"

////////////////////////////////////////////////////
// GLOBALS
////

char tempPageFault[1024];
extern void NewInt31Handler(void);

ULONG OldIntEHandler=0;
ULONG error_code;
BOOLEAN bInPageFaultHandler = FALSE;
static ULONG PCR_SEL = PCR_SELECTOR;
static ULONG OLD_PCR;

////////////////////////////////////////////////////
// FUNCTIONS
////

//*************************************************************************
// HandleInDebuggerFault()
//
//*************************************************************************
ULONG HandleInDebuggerFault(FRAME* ptr,ULONG address)
{
	PEPROCESS tsk;

    ENTER_FUNC();

	DPRINT((2,"HandleInDebuggerFault(): ###### page fault @ %.8X while inside debugger, eip: %x\n",address, ptr->eip));

	// fault in this page fault handler
	if(bInPageFaultHandler)
	{
    	DPRINT((2,"HandleInDebuggerFault(): ###### page fault @ %.8X while in page fault handler\n",address));

        DPRINT((2,"!!! machine is halted !!!\n"));
        __asm__ __volatile__ ("hlt");

        LEAVE_FUNC();
		return 0;
	}

	bInPageFaultHandler = TRUE;

    // when we come here from DebuggerShell() we live on a different stack
    // so the current task is different as well
    tsk = IoGetCurrentProcess();

    DPRINT((2,"%.8X (%.4X:%.8X %.8X %s %s %s task=%.8X )\n",
        address,
        ptr->cs,
        ptr->eip,
        ptr->eflags,
        (ptr->error_code&1)?"PLP":"NP",
        (ptr->error_code&2)?"WRITE":"READ",
        (ptr->error_code&4)?"USER-MODE":"KERNEL-MODE",
        (ULONG)tsk));

	if(!bInPrintk)
    {
    	DPRINT((0,"HandleInDebuggerFault(): unexpected pagefault in command handler!\n",address));
    }
	else
    {
    	DPRINT((0,"HandleInDebuggerFault(): unexpected pagefault in command handler while in PrintkCallback()!\n",address));
    }

    if(tsk)
    {
	    PULONG pPGD;
	    PULONG pPTE;

        pPGD = ADDR_TO_PDE(address);

        DPRINT((0,"PGD for %.8X @ %.8X = %.8X\n",address,(ULONG)pPGD,(ULONG)(*pPGD) ));

        if(pPGD && (*pPGD)&_PAGE_PRESENT)
        {
            // not large page
            if(!((*pPGD)&_PAGE_4M))
            {
                pPTE = ADDR_TO_PTE(address);
                if(pPTE)
                {
                    DPRINT((0,"PTE for %.8X @ %.8X = %.8X\n",address,(ULONG)pPTE,(ULONG)(*pPTE) ));
                }
            }
        }
    }

    IntelStackWalk(ptr->eip,CurrentEBP,ulRealStackPtr);

    DPRINT((0,"!!! machine is halted !!!\n"));
    __asm__ __volatile__ ("hlt");

    LEAVE_FUNC();

	return 2;
}

//*************************************************************************
// HandlePageFault()
//
// returns:
// 0    =       let the system handle it
// 1    =       call DebuggerShell()
// 2    =       FATAL error inside debugger
//*************************************************************************
ULONG HandlePageFault(FRAME* ptr)
{
    PVOID address;
	PEPROCESS tsk, tsk1;
	PMADDRESS_SPACE vma;
    PLIST_ENTRY current_entry;
	MEMORY_AREA* current;
    ULONG value;
	PKTHREAD CurrentThread;
	PETHREAD CurrentEThread;

	// get linear address of page fault
	__asm__ __volatile__("movl %%cr2,%0"
					      :"=r" (address));

  	DPRINT((0,"\nPageFault: bInDebShell: %d, error: %d, addr: %x\n", bInDebuggerShell, ptr->error_code, address));

    // there's something terribly wrong if we get a fault in our command handler
    if(bInDebuggerShell)
    {
		DPRINT((2,"return handleindebuggerfault\n"));
		return HandleInDebuggerFault(ptr,(ULONG)address);
    }

	ASSERT(IsAddressValid((ULONG)ptr));
    // remember error code so we can push it back on the stack
    error_code = ptr->error_code;

    //ei Check IRQL here!!!
/*
    if(in_interrupt())
    {
        Print(OUTPUT_WINDOW,"pICE: system is currently processing an interrupt!\n");
        return 1;
    }
*/
	// current process
    tsk = IoGetCurrentProcess();
	if( !tsk || !(IsAddressValid((ULONG)tsk))){
		DPRINT((0,"tsk address not valid: tsk: %x\n", tsk));
		return 0;
	}

	// lookup VMA for this address
	vma = &(tsk->AddressSpace);
	if( !vma || !(IsAddressValid((ULONG)vma))){
		DPRINT((0,"vma not valid: vma: %x\n", vma));
		return 0;
	}

	current_entry = vma->MAreaListHead.Flink;
	ASSERT(current_entry);
	DPRINT((0,"vma: %x, current_entry: %x, kernel arena: %x\n", vma, current_entry, my_init_mm));
	while(current_entry != &vma->MAreaListHead)
	{
		ASSERT(current_entry);
		ASSERT(IsAddressValid((ULONG)current_entry));
		current = CONTAINING_RECORD(current_entry,
						MEMORY_AREA,
						Entry);
		DPRINT((0,"address: %x    %x - %x Attrib: %x, Type: %x\n", address, current->BaseAddress, current->BaseAddress + current->Length, current->Attributes, current->Type));

		if( (address >= current->BaseAddress) && (address <= current->BaseAddress + current->Length ))
		{
			//page not present
			if( !(error_code & 1) ){
				//check it is in pageable area
				if( current->Type == MEMORY_AREA_SECTION_VIEW_COMMIT ||
					current->Type == MEMORY_AREA_SECTION_VIEW_RESERVE ||
					current->Type == MEMORY_AREA_VIRTUAL_MEMORY ||
					current->Type == MEMORY_AREA_PAGED_POOL ||
					current->Type == MEMORY_AREA_SHARED_DATA
						){
	                //Print(OUTPUT_WINDOW,"pICE: VMA Pageable Section.\n");
					DPRINT((2,"return 0 1\n"));
					return 0; //let the system handle this
				}
				Print(OUTPUT_WINDOW,"pICE: VMA Page not present in non-pageable Section!\n");
				DPRINT((2,"Type: currenttype: %x return 1 2\n", current->Type));
				return 1;
			}
			else{ //access violation

				if( error_code & 4 )
				{   //user mode
					if( (ULONG)address >= KERNEL_BASE )
					{
						Print(OUTPUT_WINDOW,"pICE: User mode program trying to access kernel memory!\n");
						DPRINT((2,"return 1 3\n"));
						return 1;
					}
					DPRINT((2,"return 0 4\n"));
					return 0;
				}
				/*
				if(error_code & 2)
		        {
					//on write
		            if(!(current->Attributes & PAGE_READONLY))
		            {
		                Print(OUTPUT_WINDOW,"pICE: virtual memory arena is not writeable!\n");
		                return 1;
		            }
		        }
		        // READ ACCESS
		        else
		        {
		            // test EXT bit in error code
				    if (error_code & 1)
		            {
		                Print(OUTPUT_WINDOW,"pICE: page-level protection fault!\n");
		                return 1;
		            }
		            //
				*/
					/*
					if (!(current->Attributes & PAGE_EXECUTE_READ))
		            {
		                Print(OUTPUT_WINDOW,"pICE: VMA is not readable!\n");
		                return 1;
		            }
					*/

		        // let the system handle it
				DPRINT((2,"return 0 5\n"));
		        return 0;
			}
		}
		current_entry = current_entry->Flink;
	}

    Print(OUTPUT_WINDOW,"pICE: no virtual memory arena at this address!\n");
	DPRINT((2,"return 0 6\n"));
	return 0;

    // let the system handle it
//    return 0;
}

//*************************************************************************
// NewIntEHandler()
//
//*************************************************************************
__asm__ ("
NewIntEHandler:
		pushfl
        cli
        cld
        pushal
	    pushl %ds

	    // setup default data selectors
	    movw %ss,%ax
	    movw %ax,%ds

		/*
		 * Load the PCR selector.
		 */
		movl 	%fs, %eax
		movl	%eax, _OLD_PCR
	 	movl	_PCR_SEL, %eax
		movl	%eax, %fs

        // get frame ptr
        lea 40(%esp),%eax
        pushl %eax
        call _HandlePageFault
        addl $4,%esp

		pushl 	%eax
	 	movl	_OLD_PCR, %eax
		movl	%eax, %fs
		popl	%eax

        cmpl $0,%eax
        je call_old_inte_handler

        cmpl $2,%eax
        je call_handler_unknown_reason

	    popl %ds
        popal
		popfl
        // remove error code. will be restored later when we call
        // original handler again.
        addl $4,%esp
		// call debugger loop
        pushl $" STR(REASON_PAGEFAULT) "
		jmp NewInt31Handler

call_old_inte_handler:
	    popl %ds
        popal
		popfl
		// chain to old handler
		.byte 0x2e
		jmp *_OldIntEHandler

call_handler_unknown_reason:
	    popl %ds
        popal
		popfl
        // remove error code. will be restored later when we call
        // original handler again.
        addl $4,%esp
		// call debugger loop
        pushl $" STR(REASON_INTERNAL_ERROR) "
		jmp NewInt31Handler
        ");


//*************************************************************************
// InstallIntEHook()
//
//*************************************************************************
void InstallIntEHook(void)
{
	ULONG LocalIntEHandler;

	ENTER_FUNC();

	MaskIrqs();
	if(!OldIntEHandler)
	{
		__asm__ __volatile__("mov $NewIntEHandler,%0"
			:"=r" (LocalIntEHandler)
			:
			:"eax");
		OldIntEHandler=SetGlobalInt(0x0E,(ULONG)LocalIntEHandler);
	}
	UnmaskIrqs();
	DPRINT((2,"OldIntE @ %x\n", OldIntEHandler));
    LEAVE_FUNC();
}

//*************************************************************************
// DeInstallIntEHook()
//
//*************************************************************************
void DeInstallIntEHook(void)
{
	ENTER_FUNC();

	MaskIrqs();
	if(OldIntEHandler)
	{
		SetGlobalInt(0x0E,(ULONG)OldIntEHandler);
        OldIntEHandler=0;
	}
	UnmaskIrqs();

    LEAVE_FUNC();
}
