/*
 * internal executive prototypes
 */

#ifndef __NTOSKRNL_INCLUDE_INTERNAL_EXECUTIVE_H
#define __NTOSKRNL_INCLUDE_INTERNAL_EXECUTIVE_H

#include <ddk/ntddk.h>
#include <ntos/time.h>

typedef struct _WINSTATION_OBJECT
{   
   CSHORT Type;
   CSHORT Size;

   KSPIN_LOCK Lock;
   UNICODE_STRING Name;
   LIST_ENTRY DesktopListHead;
   PRTL_ATOM_TABLE AtomTable;
   PVOID HandleTable;
   /* FIXME: Clipboard */
} WINSTATION_OBJECT, *PWINSTATION_OBJECT;

typedef struct _DESKTOP_OBJECT
{   
   CSHORT Type;
   CSHORT Size;

   LIST_ENTRY ListEntry;
   KSPIN_LOCK Lock;
   UNICODE_STRING Name;
   struct _WINSTATION_OBJECT *WindowStation;
} DESKTOP_OBJECT, *PDESKTOP_OBJECT;

/* GLOBAL VARIABLES *********************************************************/

TIME_ZONE_INFORMATION SystemTimeZoneInfo;

/* INITIALIZATION FUNCTIONS *************************************************/

VOID
ExpWin32kInit(VOID);

VOID 
ExInit (VOID);
VOID 
ExInitTimeZoneInfo (VOID);
VOID 
ExInitializeWorkerThreads(VOID);

#endif /* __NTOSKRNL_INCLUDE_INTERNAL_EXECUTIVE_H */


