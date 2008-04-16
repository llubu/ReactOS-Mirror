/*
 *  GDI object common header definition
 *
 */

#ifndef __WIN32K_GDIOBJ_H
#define __WIN32K_GDIOBJ_H

/* Public GDI Object/Handle definitions */
#include <win32k/ntgdihdl.h>
#include <include/win32.h>

typedef struct _GDI_HANDLE_TABLE
{
/* The table must be located at the beginning of this structure so it can be
 * properly mapped!
 */
//////////////////////////////////////////////////////////////////////////////
  GDI_TABLE_ENTRY Entries[GDI_HANDLE_COUNT];
  DEVCAPS         DevCaps;                 // Device Capabilities.
  FLONG           flDeviceUniq;            // Device settings uniqueness.
  PVOID           pvLangPack;              // Language Pack.
  CFONT           cfPublic[GDI_CFONT_MAX]; // Public Fonts.
  DWORD           dwCsbSupported1;         // OEM code-page bitfield.
//////////////////////////////////////////////////////////////////////////////
  PPAGED_LOOKASIDE_LIST LookasideLists;

  ULONG           FirstFree;
  ULONG           FirstUnused;

} GDI_HANDLE_TABLE, *PGDI_HANDLE_TABLE;

extern PGDI_HANDLE_TABLE GdiHandleTable;

typedef PVOID PGDIOBJ;

typedef BOOL (INTERNAL_CALL *GDICLEANUPPROC)(PVOID ObjectBody);

/* Every GDI Object must have this standard type of header.
 * It's for thread locking. */
typedef struct _BASEOBJECT
{
  HGDIOBJ     hHmgr;
  ULONG       ulShareCount;
  USHORT      cExclusiveLock;
  USHORT      BaseFlags;
  PW32THREAD  Tid;
} BASEOBJECT, *POBJ;

enum BASEFLAGS
{
    BASEFLAG_LOOKASIDE = 0x80
};

BOOL    INTERNAL_CALL GDIOBJ_OwnedByCurrentProcess(HGDIOBJ ObjectHandle);
VOID    INTERNAL_CALL GDIOBJ_SetOwnership(HGDIOBJ ObjectHandle, PEPROCESS Owner);
VOID    INTERNAL_CALL GDIOBJ_CopyOwnership(HGDIOBJ CopyFrom, HGDIOBJ CopyTo);
BOOL    INTERNAL_CALL GDIOBJ_ConvertToStockObj(HGDIOBJ *hObj);
VOID    INTERNAL_CALL GDIOBJ_UnlockObjByPtr(POBJ Object);
VOID    INTERNAL_CALL GDIOBJ_ShareUnlockObjByPtr(POBJ Object);
BOOL    INTERNAL_CALL GDIOBJ_ValidateHandle(HGDIOBJ hObj, ULONG ObjectType);
POBJ    INTERNAL_CALL GDIOBJ_AllocObj(UCHAR ObjectType);
POBJ    INTERNAL_CALL GDIOBJ_AllocObjWithHandle(ULONG ObjectType);
VOID    INTERNAL_CALL GDIOBJ_FreeObj (POBJ pObj, UCHAR ObjectType);
BOOL    INTERNAL_CALL GDIOBJ_FreeObjByHandle (HGDIOBJ hObj, DWORD ObjectType);
PGDIOBJ INTERNAL_CALL GDIOBJ_LockObj (HGDIOBJ hObj, DWORD ObjectType);
PGDIOBJ INTERNAL_CALL GDIOBJ_ShareLockObj (HGDIOBJ hObj, DWORD ObjectType);

PVOID   INTERNAL_CALL GDI_MapHandleTable(PSECTION_OBJECT SectionObject, PEPROCESS Process);

#define GDIOBJ_GetObjectType(Handle) \
  GDI_HANDLE_GET_TYPE(Handle)

#define GDIOBJFLAG_DEFAULT	(0x0)
#define GDIOBJFLAG_IGNOREPID 	(0x1)
#define GDIOBJFLAG_IGNORELOCK 	(0x2)

BOOL FASTCALL  NtGdiDeleteObject(HGDIOBJ hObject);
BOOL FASTCALL  IsObjectDead(HGDIOBJ);

#endif
