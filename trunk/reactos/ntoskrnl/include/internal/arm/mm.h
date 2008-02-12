#ifndef __NTOSKRNL_INCLUDE_INTERNAL_ARM_MM_H
#define __NTOSKRNL_INCLUDE_INTERNAL_ARM_MM_H

#if __GNUC__ >=3
#pragma GCC system_header
#endif

#define TTB_SHIFT 20

typedef union _ARM_PTE
{
    union
    {
        struct
        {
            ULONG Type:2;
            ULONG Unused:30;
        } Fault;
        struct
        {
            ULONG Type:2;
            ULONG Reserved:3;
            ULONG Domain:4;
            ULONG Ignored:1;
            ULONG BaseAddress:22;
        } Coarse;
        struct
        {
            ULONG Type:2;
            ULONG Buffered:1;
            ULONG Cached:1;
            ULONG Reserved:1;
            ULONG Domain:4;
            ULONG Ignored:1;
            ULONG Access:2;
            ULONG Ignored1:8;
            ULONG BaseAddress:12;
        } Section;
        struct
        {
            ULONG Type:2;
            ULONG Reserved:3;
            ULONG Domain:4;
            ULONG Ignored:3;
            ULONG BaseAddress:20;
        } Fine;
    } L1;
    union
    {
        struct
        {
            ULONG Type:2;
            ULONG Unused:30;
        } Fault;
        struct
        {
            ULONG Type:2;
            ULONG Buffered:1;
            ULONG Cached:1;
            ULONG Access0:2;
            ULONG Access1:2;
            ULONG Access2:2;
            ULONG Access3:2;
            ULONG Ignored:4;
            ULONG BaseAddress:16;
        } Large;
        struct
        {
            ULONG Type:2;
            ULONG Buffered:1;
            ULONG Cached:1;
            ULONG Access0:2;
            ULONG Access1:2;
            ULONG Access2:2;
            ULONG Access3:2;
            ULONG BaseAddress:20;
        } Small;
        struct
        {
            ULONG Type:2;
            ULONG Buffered:1;
            ULONG Cached:1;
            ULONG Access0:2;
            ULONG Ignored:4;
            ULONG BaseAddress:22;
        } Tiny; 
    } L2;
    ULONG AsUlong;
} ARM_PTE, *PARM_PTE;

typedef struct _ARM_TRANSLATION_TABLE
{
    ARM_PTE Pte[4096];
} ARM_TRANSLATION_TABLE, *PARM_TRANSLATION_TABLE;

typedef enum _ARM_L1_PTE_TYPE
{
    FaultPte,
    CoarsePte,
    SectionPte,
    FinePte
} ARM_L1_PTE_TYPE;

typedef enum _ARM_PTE_ACCESS
{
    FaultAccess,
    SupervisorAccess,
    SharedAccess,
    UserAccess
} ARM_PTE_ACCESS;

typedef enum _ARM_DOMAIN
{
    FaultDomain,
    ClientDomain,
    InvalidDomain,
    ManagerDomain
} ARM_DOMAIN;

#endif
