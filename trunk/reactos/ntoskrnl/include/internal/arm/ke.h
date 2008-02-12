#ifndef __NTOSKRNL_INCLUDE_INTERNAL_ARM_KE_H
#define __NTOSKRNL_INCLUDE_INTERNAL_ARM_KE_H

#if __GNUC__ >=3
#pragma GCC system_header
#endif

typedef union _ARM_TTB_REGISTER
{
    struct
    {
        ULONG Reserved:14;
        ULONG BaseAddress:18;
    };
    ULONG AsUlong;
} ARM_TTB_REGISTER;

typedef union _ARM_DOMAIN_REGISTER
{
    struct
    {
        ULONG Domain0:2;
        ULONG Domain1:2;
        ULONG Domain2:2;
        ULONG Domain3:2;
        ULONG Domain4:2;
        ULONG Domain5:2;
        ULONG Domain6:2;
        ULONG Domain7:2;
        ULONG Domain8:2;
        ULONG Domain9:2;
        ULONG Domain10:2;
        ULONG Domain11:2;
        ULONG Domain12:2;
        ULONG Domain13:2;
        ULONG Domain14:2;
        ULONG Domain15:2;
    };
    ULONG AsUlong;
} ARM_DOMAIN_REGISTER;

typedef union _ARM_CONTROL_REGISTER
{
    struct
    {
        ULONG MmuEnabled:1;
        ULONG AlignmentFaultsEnabled:1;
        ULONG DCacheEnabled:1;
        ULONG Sbo:3;
        ULONG BigEndianEnabled:1;
        ULONG System:1;
        ULONG Rom:1;
        ULONG Sbz:2;
        ULONG ICacheEnabled:1;
        ULONG HighVectors:1;
        ULONG RoundRobinReplacementEnabled:1;
        ULONG Armv4Compat:1;
        ULONG Sbo1:1;
        ULONG Sbz1:1;
        ULONG Sbo2:1;
        ULONG Reserved:14;
    };
    ULONG AsUlong;
} ARM_CONTROL_REGISTER, *PARM_CONTROL_REGISTER;

typedef enum _ARM_DOMAINS
{
    Domain0,
    Domain1,
    Domain2,
    Domain3,
    Domain4,
    Domain5,
    Domain6,
    Domain7,
    Domain8,
    Domain9,
    Domain10,
    Domain11,
    Domain12,
    Domain13,
    Domain14,
    Domain15
} ARM_DOMAINS;

VOID
NTAPI
KeArmInitThreadWithContext(
    IN PKTHREAD Thread,
    IN PKSYSTEM_ROUTINE SystemRoutine,
    IN PKSTART_ROUTINE StartRoutine,
    IN PVOID StartContext,
    IN PCONTEXT Context
);

#define KeArchInitThreadWithContext KeArmInitThreadWithContext

#endif
