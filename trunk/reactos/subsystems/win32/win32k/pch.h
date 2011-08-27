#ifndef __W32K_H
#define __W32K_H
/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Graphics Subsystem
 * FILE:            subsys/win32k/w32k.h
 * PURPOSE:         Main Win32K Header
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#define _NO_COM

/* DDK/NDK/SDK Headers */
#undef NTDDI_VERSION
#define NTDDI_VERSION NTDDI_WS03SP1
#include <ntddk.h>
#include <ntddmou.h>
#include <ntifs.h>
#include <tvout.h>
#include <ndk/exfuncs.h>
#include <ndk/kdfuncs.h>
#include <ndk/kefuncs.h>
#include <ndk/lpcfuncs.h>
#include <ndk/mmfuncs.h>
#include <ndk/obfuncs.h>
#include <ndk/psfuncs.h>
#include <ndk/rtlfuncs.h>
#include <ntstrsafe.h>

/* Win32 Headers */
/* FIXME: Defines in winbase.h that we need... */
typedef struct _SECURITY_ATTRIBUTES SECURITY_ATTRIBUTES, *LPSECURITY_ATTRIBUTES;
#define WINBASEAPI
#define STARTF_USESIZE 2
#define STARTF_USEPOSITION 4
#include <stdarg.h>
#include <windef.h>
#include <math.h>

/* Avoid type casting, by defining RECT to RECTL */
#define RECT RECTL
#define PRECT PRECTL
#define LPRECT LPRECTL
#define LPCRECT LPCRECTL
#define POINT POINTL
#define LPPOINT PPOINTL
#define PPOINT PPOINTL

#include <winerror.h>
#include <wingdi.h>
#define NT_BUILD_ENVIRONMENT
#include <winddi.h>
#include <winuser.h>
#include <prntfont.h>
#include <dde.h>
#include <wincon.h>
#define _NOCSECT_TYPE
#include <ddrawi.h>

/* SEH Support with PSEH */
#include <pseh/pseh2.h>

/* CSRSS Header */
#include <csrss/csrss.h>

/* Public Win32K Headers */
#include <win32k/callback.h>
#include <win32k/ntusrtyp.h>
#include <win32k/ntuser.h>
#include <win32k/ntgdityp.h>
#include <win32k/ntgdibad.h>
#include <win32k/ntgdihdl.h>
#include <ntgdi.h>

/* Undocumented user definitions */
#include <undocuser.h>

/* Freetype headers*/
#include <ft2build.h>
#include <freetype/freetype.h>

/* Internal Win32K Header */
#include "include/win32kp.h"

/* Undocumented stuff */
typedef DRIVEROBJ *PDRIVEROBJ;

/* User heap */
extern HANDLE GlobalUserHeap;

PWIN32HEAP
UserCreateHeap(OUT PSECTION_OBJECT *SectionObject,
               IN OUT PVOID *SystemBase,
               IN SIZE_T HeapSize);

static __inline PVOID
UserHeapAlloc(SIZE_T Bytes)
{
    return RtlAllocateHeap(GlobalUserHeap,
                           HEAP_NO_SERIALIZE,
                           Bytes);
}

static __inline BOOL
UserHeapFree(PVOID lpMem)
{
    return RtlFreeHeap(GlobalUserHeap,
                       HEAP_NO_SERIALIZE,
                       lpMem);
}

static __inline PVOID
UserHeapReAlloc(PVOID lpMem,
                SIZE_T Bytes)
{
#if 0
    /* NOTE: ntoskrnl doesn't export RtlReAllocateHeap... */
    return RtlReAllocateHeap(GlobalUserHeap,
                             HEAP_NO_SERIALIZE,
                             lpMem,
                             Bytes);
#else
    SIZE_T PrevSize;
    PVOID pNew;

    PrevSize = RtlSizeHeap(GlobalUserHeap,
                           HEAP_NO_SERIALIZE,
                           lpMem);

    if (PrevSize == Bytes)
        return lpMem;

    pNew = RtlAllocateHeap(GlobalUserHeap,
                           HEAP_NO_SERIALIZE,
                           Bytes);
    if (pNew != NULL)
    {
        if (PrevSize < Bytes)
            Bytes = PrevSize;

        RtlCopyMemory(pNew,
                      lpMem,
                      Bytes);

        RtlFreeHeap(GlobalUserHeap,
                    HEAP_NO_SERIALIZE,
                    lpMem);
    }

    return pNew;
#endif
}

static __inline PVOID
UserHeapAddressToUser(PVOID lpMem)
{
    PPROCESSINFO W32Process = PsGetCurrentProcessWin32Process();
    return (PVOID)(((ULONG_PTR)lpMem - (ULONG_PTR)GlobalUserHeap) +
                   (ULONG_PTR)W32Process->HeapMappings.UserMapping);
}

#define ROUND_DOWN(n, align) \
    (((ULONG)n) & ~((align) - 1l))

#define ROUND_UP(n, align) \
    ROUND_DOWN(((ULONG)n) + (align) - 1, (align))

#define LIST_FOR_EACH(elem, list, type, field) \
    for ((elem) = CONTAINING_RECORD((list)->Flink, type, field); \
         &(elem)->field != (list) && ((&((elem)->field)) != NULL); \
         (elem) = CONTAINING_RECORD((elem)->field.Flink, type, field))

#define LIST_FOR_EACH_SAFE(cursor, cursor2, list, type, field) \
    for ((cursor) = CONTAINING_RECORD((list)->Flink, type, field), \
         (cursor2) = CONTAINING_RECORD((cursor)->field.Flink, type, field); \
         &(cursor)->field != (list) && ((&((cursor)->field)) != NULL); \
         (cursor) = (cursor2), \
         (cursor2) = CONTAINING_RECORD((cursor)->field.Flink, type, field))

#endif /* __W32K_H */
