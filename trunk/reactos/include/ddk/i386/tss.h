/*
 * 
 */

#ifndef __INCLUDE_DDK_I386_TSS_H
#define __INCLUDE_DDK_I386_TSS_H

typedef struct
{
   unsigned short previous_task;
   unsigned short reserved1;
   unsigned long esp0;
   unsigned short ss0;
   unsigned short reserved2;
   unsigned long esp1;
   unsigned short ss1;
   unsigned short reserved3;
   unsigned long esp2;
   unsigned short ss2;
   unsigned short reserved4;
   unsigned long cr3;
   unsigned long eip;
   unsigned long eflags;
   unsigned long eax;
   unsigned long ecx;
   unsigned long edx;
   unsigned long ebx;
   unsigned long esp;
   unsigned long ebp;
   unsigned long esi;
   unsigned long edi;
   unsigned short es;
   unsigned short reserved5;
   unsigned short cs;
   unsigned short reserved6;
   unsigned short ss;
   unsigned short reserved7;
   unsigned short ds;
   unsigned short reserved8;
   unsigned short fs;
   unsigned short reserved9;
   unsigned short gs;
   unsigned short reserved10;
   unsigned short ldt;
   unsigned short reserved11;
   unsigned short trap;
   unsigned short iomap_base;

   unsigned short nr;
   PVOID KernelStackBase;
   PVOID SavedKernelEsp;
   PVOID SavedKernelStackBase;
   
   unsigned char io_bitmap[1];
} hal_thread_state;


#endif /* __INCLUDE_DDK_I386_TSS_H */
