/*++

Copyright (c) 1998-2001 Klaus P. Gerlicher

Module Name:

    utils.h

Abstract:

    HEADER for utils.c

Environment:

    LINUX 2.2.X
    Kernel mode only

Author: 

    Klaus P. Gerlicher

Revision History:

    15-Nov-2000:    general cleanup of source files

Copyright notice:

  This file may be distributed under the terms of the GNU Public License.

--*/

#define __STR(x) #x
#define STR(x) __STR(x)

typedef struct _FRAME
{
    ULONG error_code;
    ULONG eip;
    ULONG cs;
    ULONG eflags;
}FRAME;

#define SHOW_FIELD_BYTE(ptr,field,wait)\
{\
	if(wait && WaitForKey()==FALSE)\
		return TRUE;\
	PICE_sprintf(tempCmd,#field" = %.2x\n",ptr->##field);\
	Print(OUTPUT_WINDOW,tempCmd);\
}

#define SHOW_FIELD_WORD(ptr,field,wait)\
{\
	if(wait && WaitForKey()==FALSE)\
		return TRUE;\
	PICE_sprintf(tempCmd,#field" = %.4x\n",ptr->##field);\
	Print(OUTPUT_WINDOW,tempCmd);\
}

#define SHOW_FIELD_DWORD(ptr,field,wait)\
{\
	if(wait && WaitForKey()==FALSE)\
		return TRUE;\
	sprintf(tempCmd,#field" = %.8x\n",ptr->##field);\
	Print(OUTPUT_WINDOW,tempCmd);\
}

#define SHOW_FIELD_SEG_OFS(ptr,field1,field2,wait)\
{\
	if(wait && WaitForKey()==FALSE)\
		return TRUE;\
	PICE_sprintf(tempCmd,#field1":"#field2" = %.4x:%.8x\n",ptr->##field1,ptr->##field2);\
	Print(OUTPUT_WINDOW,tempCmd);\
}

typedef struct _PCI_NUMBER
{
    union {
        struct
        {
            ULONG res2  : 2;
            ULONG reg   : 6; // 64 regs per function
            ULONG func  : 3; // 8 functions per device
            ULONG dev   : 5; // 32 device per bus
            ULONG bus   : 8; // 256 buses
            ULONG res1  : 7;
            ULONG ce    : 1; // 1 to enable
        }bits;
        ULONG AsUlong;
    }u;
}PCI_NUMBER;

typedef struct _PCI_COMMON_CONFIG {
    USHORT  VendorID;                   // (ro)
    USHORT  DeviceID;                   // (ro)
    USHORT  Command;                    // Device control
    USHORT  Status;
    UCHAR   RevisionID;                 // (ro)
    UCHAR   ProgIf;                     // (ro)
    UCHAR   SubClass;                   // (ro)
    UCHAR   BaseClass;                  // (ro)
    UCHAR   CacheLineSize;              // (ro+)
    UCHAR   LatencyTimer;               // (ro+)
    UCHAR   HeaderType;                 // (ro)
    UCHAR   BIST;                       // Built in self test
    ULONG   BaseAddresses[6];
    ULONG   CIS;
    USHORT  SubVendorID;
    USHORT  SubSystemID;
    ULONG   ROMBaseAddress;
    UCHAR   CapabilitiesPtr;
    UCHAR   Reserved1[3];
    ULONG   Reserved2;
    UCHAR   InterruptLine;      //
    UCHAR   InterruptPin;       // (ro)
    UCHAR   MinimumGrant;       // (ro)
    UCHAR   MaximumLatency;     // (ro)
}PCI_COMMON_CONFIG;

typedef struct tagPageDir
{
	ULONG P			:1;
	ULONG RW		:1;
	ULONG US		:1;
	ULONG PWT		:1;
	ULONG PCD		:1;
	ULONG A			:1;
	ULONG dummy		:1;
	ULONG PS		:1;
	ULONG G			:1;
	ULONG Avail		:3;
	ULONG PTBase	:20;
}PAGEDIR,*PPAGEDIR;

extern struct mm_struct* my_init_mm;

typedef struct tagGdt
{
	ULONG Limit_15_0		:16;
	ULONG Base_15_0			:16;
	ULONG Base_23_16		:8;
	ULONG SegType			:4;
	ULONG DescType			:1;
	ULONG Dpl				:2;
	ULONG Present			:1;
	ULONG Limit_19_16		:4;
	ULONG Avl				:1;
	ULONG Reserved			:1;
	ULONG DefOp				:1;
	ULONG Gran				:1;
	ULONG Base_31_24		:8;
}GDT,*PGDT;

typedef struct tagIdt
{
	ULONG Offset_15_0		:16;
	ULONG Selector			:16;
	ULONG Reserved			:8;
	ULONG DescType			:5;
	ULONG Dpl				:2;
	ULONG Present			:1;
	ULONG Offset_31_16		:16;
}IDT,*PIDT;

typedef struct tagDESCRIPTOR 
{
	USHORT Cpl	:2;		// current privilege level
	USHORT Ti	:1;		// table index (GDT=0 LDT=1)
	USHORT Val	:13;	// index into table
}DESCRIPTOR,*PDESCRIPTOR;

extern struct module **pmodule_list;

void PICE_memset(void* p,unsigned char c,int sz);
void PICE_memcpy(void* t,void* s,int sz);
char *PICE_strrev(char *);
ULONG PICE_strcmp(char* s1,char* s2);
ULONG PICE_strcmpi(char* s1,char* s2);
ULONG PICE_strncmpi(char* s1,char* s2,ULONG len);
USHORT PICE_strlen(char* s);
char* PICE_strcat(char* s1,char* s2);
BOOLEAN PICE_isprint(char c);
char* PICE_strcpy(char* s1,char* s2);
char* PICE_strncpy(char* s1,char* s2,int len);
char* PICE_strchr(char* s,char c);

int PICE_sprintf(char * buf, const char *fmt, ...);
int PICE_vsprintf(char *buf, const char *fmt, va_list args);

BOOLEAN IsAddressValid(ULONG Addr); 
BOOLEAN IsAddressWriteable(ULONG Addr);
BOOLEAN SetAddressWriteable(ULONG address,BOOLEAN bSet);
BOOLEAN IsRangeValid(ULONG addr,ULONG Length); 
void IntelStackWalk(ULONG pc,ULONG ebp,ULONG esp);
struct module* IsModuleLoaded(LPSTR p);

ULONG ReadPhysMem(ULONG Address,ULONG ulSize);
void WritePhysMem(ULONG Address,ULONG Datum,ULONG ulSize);

BOOLEAN IsRetAtEIP(void);
BOOLEAN IsCallInstrAtEIP(void);

ULONG GetLinearAddress(USHORT Segment,ULONG Offset); 

#define OUTPUT_BUFFER_FULL       0x01 
#define INPUT_BUFFER_FULL        0x02 
#define MOUSE_OUTPUT_BUFFER_FULL 0x20 

void ShowStoppedMsg(void);
void ShowRunningMsg(void);

void SetHardwareBreakPoints(void); 
void SetHardwareBreakPoint(ULONG ulAddress,ULONG ulReg);

// this should be in disasm.h but someone misused the header files
BOOLEAN Disasm(PULONG pOffset, PUCHAR pchDst);
//////////////////////////////////////////////////////////////////

#define GLOBAL_CODE_SEGMENT (__KERNEL_CS) 
#define GLOBAL_DATA_SEGMENT (__KERNEL_DS) 

#define OVR_CS .byte 0x2e 
#define OVR_FS .byte 0x64 

void DisplayRegs(void);
void SaveOldRegs(void);

BOOLEAN CheckLoadAbort(void);

UCHAR KeyboardGetKeyPolled(void);
void KeyboardFlushKeyboardQueue(void);

#if REAL_LINUX_VERSION_CODE >= 0x020400
#define _PAGE_4M _PAGE_PSE
#endif

UCHAR AsciiFromScan(UCHAR s);
UCHAR AsciiToScan(UCHAR s);

void outportb(USHORT port,UCHAR data);
UCHAR inportb(USHORT port);

extern unsigned long sys_call_table[];

struct mm_struct *GetInitMm(void);

void EnablePassThrough(void);
