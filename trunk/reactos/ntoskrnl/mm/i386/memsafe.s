.globl _MmSafeCopyFromUser
.globl _MmSafeCopyFromUserUnsafeStart
.globl _MmSafeCopyFromUserRestart
.globl _MmSafeCopyToUser
.globl _MmSafeCopyToUserUnsafeStart
.globl _MmSafeCopyToUserRestart
.globl @MmSafeReadPtr@4
.globl _MmSafeReadPtrStart
.globl _MmSafeReadPtrEnd

	/*
	 * NTSTATUS MmSafeCopyFromUser(PVOID Dest, PVOID Src, 
	 *                             ULONG NumberOfBytes)
	 */
_MmSafeCopyFromUser:
	pushl	%ebp
	movl	%esp,%ebp

	pushl   %esi
	pushl	%edi
	pushl	%ecx
	
	movl    8(%ebp),%edi
	movl	12(%ebp),%esi
	movl	16(%ebp),%ecx

	/*
	 * Default return code
	 */ 
	movl    $0,%eax

_MmSafeCopyFromUserUnsafeStart:	        
	/*
	 * This is really a synthetic instruction since if we incur a
	 * pagefault then eax will be set to an appropiate STATUS code
	 */
	cld 
	rep movsb

_MmSafeCopyFromUserRestart:

	popl	%ecx
	popl	%edi
	popl	%esi

        popl    %ebp
        ret

/*****************************************************************************/

	/*
	 * NTSTATUS MmSafeCopyToUser(PVOID Dest, PVOID Src, 
	 *			ULONG NumberOfBytes)
	 */ 
_MmSafeCopyToUser:
	pushl	%ebp
	movl	%esp,%ebp

	pushl   %esi
	pushl	%edi
	pushl	%ecx
	
	movl    8(%ebp),%edi
	movl	12(%ebp),%esi
	movl	16(%ebp),%ecx

	/*
	 * Default return code
	 */ 
	movl    $0,%eax

_MmSafeCopyToUserUnsafeStart:	 	 
	/*
	 * This is really a synthetic instruction since if we incur a
	 * pagefault then eax will be set to an appropiate STATUS code
	 */
	cld 
	rep movsb

_MmSafeCopyToUserRestart:

	popl	%ecx
	popl	%edi
	popl	%esi
	
	popl    %ebp
	ret

/*****************************************************************************/

	/*
	 * PVOID FASTCALL MmSafeReadPtr(PVOID Source)
	 */
@MmSafeReadPtr@4:
_MmSafeReadPtrStart:
	/*
	 * If we incur a pagefault, eax will be set NULL
	 */
	movl	(%ecx),%eax
_MmSafeReadPtrEnd:
	ret
