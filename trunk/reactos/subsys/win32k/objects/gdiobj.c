/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * GDIOBJ.C - GDI object manipulation routines
 *
 * $Id: gdiobj.c,v 1.44 2003/09/26 10:45:45 gvg Exp $
 *
 */

#undef WIN32_LEAN_AND_MEAN
#define WIN32_NO_STATUS
#include <windows.h>
#include <ddk/ntddk.h>
#include <include/dce.h>
#include <include/object.h>
#include <win32k/gdiobj.h>
#include <win32k/brush.h>
#include <win32k/pen.h>
#include <win32k/text.h>
#include <win32k/dc.h>
#include <win32k/bitmaps.h>
#include <win32k/region.h>
#include <win32k/cursoricon.h>
#include <include/palette.h>
#define NDEBUG
#include <win32k/debug1.h>

/*! Size of the GDI handle table
 * http://www.windevnet.com/documents/s=7290/wdj9902b/9902b.htm
 * gdi handle table can hold 0x4000 handles
*/
#define GDI_HANDLE_COUNT 0x4000

#define GDI_GLOBAL_PROCESS ((HANDLE) 0xffffffff)

#define GDI_HANDLE_INDEX_MASK (GDI_HANDLE_COUNT - 1)
#define GDI_HANDLE_TYPE_MASK  0x007f0000
#define GDI_HANDLE_STOCK_MASK 0x00800000

#define GDI_HANDLE_CREATE(i, t)    ((HANDLE)(((i) & GDI_HANDLE_INDEX_MASK) | ((t) & GDI_HANDLE_TYPE_MASK)))
#define GDI_HANDLE_GET_INDEX(h)    (((DWORD)(h)) & GDI_HANDLE_INDEX_MASK)
#define GDI_HANDLE_GET_TYPE(h)     (((DWORD)(h)) & GDI_HANDLE_TYPE_MASK)
#define GDI_HANDLE_IS_TYPE(h, t)   ((t) == (((DWORD)(h)) & GDI_HANDLE_TYPE_MASK))
#define GDI_HANDLE_IS_STOCKOBJ(h)  (0 != (((DWORD)(h)) & GDI_HANDLE_STOCK_MASK))
#define GDI_HANDLE_SET_STOCKOBJ(h) ((h) = (HANDLE)(((DWORD)(h)) | GDI_HANDLE_STOCK_MASK))

#define GDI_TYPE_TO_MAGIC(t) ((WORD) ((t) >> 16))
#define GDI_MAGIC_TO_TYPE(m) ((DWORD)(m) << 16)

#define GDI_VALID_OBJECT(h, obj, t, f) \
  (NULL != (obj) \
   && (GDI_MAGIC_TO_TYPE((obj)->Magic) == (t) || GDI_OBJECT_TYPE_DONTCARE == (t)) \
   && (GDI_HANDLE_GET_TYPE((h)) == (t) || GDI_OBJECT_TYPE_DONTCARE == (t)) \
   && (((obj)->hProcessId == PsGetCurrentProcessId()) \
       || (GDI_GLOBAL_PROCESS == (obj)->hProcessId) \
       || ((f) & GDIOBJFLAG_IGNOREPID)))

typedef struct _GDI_HANDLE_TABLE
{
  WORD  wTableSize;
  PGDIOBJHDR Handles[1];
} GDI_HANDLE_TABLE, *PGDI_HANDLE_TABLE;

/*  GDI stock objects */

static LOGBRUSH WhiteBrush =
{ BS_SOLID, RGB(255,255,255), 0 };

static LOGBRUSH LtGrayBrush =
/* FIXME : this should perhaps be BS_HATCHED, at least for 1 bitperpixel */
{ BS_SOLID, RGB(192,192,192), 0 };

static LOGBRUSH GrayBrush =
/* FIXME : this should perhaps be BS_HATCHED, at least for 1 bitperpixel */
{ BS_SOLID, RGB(128,128,128), 0 };

static LOGBRUSH DkGrayBrush =
/* This is BS_HATCHED, for 1 bitperpixel. This makes the spray work in pbrush */
/* NB_HATCH_STYLES is an index into HatchBrushes */
{ BS_HATCHED, RGB(0,0,0), NB_HATCH_STYLES };

static LOGBRUSH BlackBrush =
{ BS_SOLID, RGB(0,0,0), 0 };

static LOGBRUSH NullBrush =
{ BS_NULL, 0, 0 };

static LOGPEN WhitePen =
{ PS_SOLID, { 0, 0 }, RGB(255,255,255) };

static LOGPEN BlackPen =
{ PS_SOLID, { 0, 0 }, RGB(0,0,0) };

static LOGPEN NullPen =
{ PS_NULL, { 0, 0 }, 0 };

static LOGFONTW OEMFixedFont =
{ 14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, OEM_CHARSET,
  0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, L"" };

static LOGFONTW AnsiFixedFont =
{ 14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
  0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, L"" };

/*static LOGFONTW AnsiVarFont =
 *{ 14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
 *  0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, L"MS Sans Serif" }; */

static LOGFONTW SystemFont =
{ 14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
  0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, L"System" };

static LOGFONTW DeviceDefaultFont =
{ 14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
  0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, L"" };

static LOGFONTW SystemFixedFont =
{ 14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
  0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, L"" };

/* FIXME: Is this correct? */
static LOGFONTW DefaultGuiFont =
{ 14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
  0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, L"MS Sans Serif" };

#define NB_STOCK_OBJECTS (DEFAULT_GUI_FONT + 1)

static HGDIOBJ *StockObjects[NB_STOCK_OBJECTS];
static PGDI_HANDLE_TABLE  HandleTable = 0;
static FAST_MUTEX  HandleTableMutex;
static FAST_MUTEX  RefCountHandling;

/*!
 * Allocate GDI object table.
 * \param	Size - number of entries in the object table.
*/
static PGDI_HANDLE_TABLE FASTCALL
GDIOBJ_iAllocHandleTable (WORD Size)
{
  PGDI_HANDLE_TABLE  handleTable;

  ExAcquireFastMutexUnsafe (&HandleTableMutex);
  handleTable = ExAllocatePool(PagedPool,
                               sizeof(GDI_HANDLE_TABLE) +
                               sizeof(PGDIOBJ) * Size);
  ASSERT( handleTable );
  memset (handleTable,
          0,
          sizeof(GDI_HANDLE_TABLE) + sizeof(PGDIOBJ) * Size);
  handleTable->wTableSize = Size;
  ExReleaseFastMutexUnsafe (&HandleTableMutex);

  return handleTable;
}

/*!
 * Returns the entry into the handle table by index.
*/
static PGDIOBJHDR FASTCALL
GDIOBJ_iGetObjectForIndex(WORD TableIndex)
{
  if (0 == TableIndex || HandleTable->wTableSize < TableIndex)
    {
      DPRINT1("Invalid TableIndex %u\n", (unsigned) TableIndex);
      return NULL;
    }

  return HandleTable->Handles[TableIndex];
}

/*!
 * Finds next free entry in the GDI handle table.
 * \return	index into the table is successful, zero otherwise.
*/
static WORD FASTCALL
GDIOBJ_iGetNextOpenHandleIndex (void)
{
  WORD tableIndex;

  ExAcquireFastMutexUnsafe (&HandleTableMutex);
  for (tableIndex = 1; tableIndex < HandleTable->wTableSize; tableIndex++)
    {
      if (NULL == HandleTable->Handles[tableIndex])
	{
	  HandleTable->Handles[tableIndex] = (PGDIOBJHDR) -1;
	  break;
	}
    }
  ExReleaseFastMutexUnsafe (&HandleTableMutex);

  return (tableIndex < HandleTable->wTableSize) ? tableIndex : 0;
}

/*!
 * Allocate memory for GDI object and return handle to it.
 *
 * \param Size - size of the GDI object. This shouldn't to include the size of GDIOBJHDR.
 * The actual amount of allocated memory is sizeof(GDIOBJHDR)+Size
 * \param ObjectType - type of object \ref GDI object types
 * \param CleanupProcPtr - Routine to be called on destruction of object
 *
 * \return Handle of the allocated object.
 *
 * \note Use GDIOBJ_Lock() to obtain pointer to the new object.
*/
HGDIOBJ FASTCALL
GDIOBJ_AllocObj(WORD Size, DWORD ObjectType, GDICLEANUPPROC CleanupProc)
{
  PGDIOBJHDR  newObject;
  WORD Index;
  
  Index = GDIOBJ_iGetNextOpenHandleIndex ();
  if (0 == Index)
    {
      DPRINT1("Out of GDI handles\n");
      return NULL;
    }

  DPRINT("GDIOBJ_AllocObj: handle: %d, size: %d, type: 0x%08x\n", Index, Size, ObjectType);
  newObject = ExAllocatePool(PagedPool, Size + sizeof (GDIOBJHDR));
  if (newObject == NULL)
  {
    DPRINT1("GDIOBJ_AllocObj: failed\n");
    return NULL;
  }
  RtlZeroMemory (newObject, Size + sizeof(GDIOBJHDR));

  newObject->wTableIndex = Index;

  newObject->dwCount = 0;
  newObject->hProcessId = PsGetCurrentProcessId ();
  newObject->CleanupProc = CleanupProc;
  newObject->Magic = GDI_TYPE_TO_MAGIC(ObjectType);
  newObject->lockfile = NULL;
  newObject->lockline = 0;
  HandleTable->Handles[Index] = newObject;

  return GDI_HANDLE_CREATE(Index, ObjectType);
}

/*!
 * Free memory allocated for the GDI object. For each object type this function calls the
 * appropriate cleanup routine.
 *
 * \param hObj       - handle of the object to be deleted.
 * \param ObjectType - one of the \ref GDI object types
 * or GDI_OBJECT_TYPE_DONTCARE.
 * \param Flag       - if set to GDIOBJFLAG_IGNOREPID then the routine doesn't check if the process that
 * tries to delete the object is the same one that created it.
 *
 * \return Returns TRUE if succesful.
 *
 * \note You should only use GDIOBJFLAG_IGNOREPID if you are cleaning up after the process that terminated.
 * \note This function deferres object deletion if it is still in use.
*/
BOOL STDCALL
GDIOBJ_FreeObj(HGDIOBJ hObj, DWORD ObjectType, DWORD Flag)
{
  PGDIOBJHDR objectHeader;
  PGDIOBJ Obj;
  BOOL 	bRet = TRUE;

  objectHeader = GDIOBJ_iGetObjectForIndex(GDI_HANDLE_GET_INDEX(hObj));
  DPRINT("GDIOBJ_FreeObj: hObj: 0x%08x, object: %x\n", hObj, objectHeader);

  if (! GDI_VALID_OBJECT(hObj, objectHeader, ObjectType, Flag)
      || GDI_GLOBAL_PROCESS == objectHeader->hProcessId)

    {
      DPRINT1("Can't delete hObj:0x%08x, type:0x%08x, flag:%d\n", hObj, ObjectType, Flag);
      return FALSE;
    }

  DPRINT("FreeObj: locks: %x\n", objectHeader->dwCount );
  if (!(Flag & GDIOBJFLAG_IGNORELOCK))
    {
      /* check that the reference count is zero. if not then set flag
       * and delete object when releaseobj is called */
      ExAcquireFastMutex(&RefCountHandling);
      if ((objectHeader->dwCount & ~0x80000000) > 0 )
	{
	  DPRINT("GDIOBJ_FreeObj: delayed object deletion: count %d\n", objectHeader->dwCount);
	  objectHeader->dwCount |= 0x80000000;
	  ExReleaseFastMutex(&RefCountHandling);
	  return TRUE;
	}
      ExReleaseFastMutex(&RefCountHandling);
    }

  /* allow object to delete internal data */
  if (NULL != objectHeader->CleanupProc)
    {
      Obj = (PGDIOBJ)((PCHAR)objectHeader + sizeof(GDIOBJHDR));
      bRet = (*(objectHeader->CleanupProc))(Obj);
    }

  ExFreePool(objectHeader);
  HandleTable->Handles[GDI_HANDLE_GET_INDEX(hObj)] = NULL;

  return bRet;
}

/*!
 * Lock multiple objects. Use this function when you need to lock multiple objects and some of them may be
 * duplicates. You should use this function to avoid trying to lock the same object twice!
 *
 * \param	pList 	pointer to the list that contains handles to the objects. You should set hObj and ObjectType fields.
 * \param	nObj	number of objects to lock
 * \return	for each entry in pList this function sets pObj field to point to the object.
 *
 * \note this function uses an O(n^2) algoritm because we shouldn't need to call it with more than 3 or 4 objects.
*/
BOOL FASTCALL
GDIOBJ_LockMultipleObj(PGDIMULTILOCK pList, INT nObj)
{
  INT i, j;
  ASSERT( pList );
  /* FIXME - check for "invalid" handles */
  /* go through the list checking for duplicate objects */
  for (i = 0; i < nObj; i++)
    {
      pList[i].pObj = NULL;
      for (j = 0; j < i; j++)
	{
	  if (pList[i].hObj == pList[j].hObj)
	    {
	      /* already locked, so just copy the pointer to the object */
	      pList[i].pObj = pList[j].pObj;
	      break;
	    }
	}

      if (NULL == pList[i].pObj)
	{
	  /* object hasn't been locked, so lock it. */
	  if (NULL != pList[i].hObj)
	    {
	      pList[i].pObj = GDIOBJ_LockObj(pList[i].hObj, pList[i].ObjectType);
	    }
	}
    }

  return TRUE;
}

/*!
 * Unlock multiple objects. Use this function when you need to unlock multiple objects and some of them may be
 * duplicates.
 *
 * \param	pList 	pointer to the list that contains handles to the objects. You should set hObj and ObjectType fields.
 * \param	nObj	number of objects to lock
 *
 * \note this function uses O(n^2) algoritm because we shouldn't need to call it with more than 3 or 4 objects.
*/
BOOL FASTCALL
GDIOBJ_UnlockMultipleObj(PGDIMULTILOCK pList, INT nObj)
{
  INT i, j;
  ASSERT(pList);

  /* go through the list checking for duplicate objects */
  for (i = 0; i < nObj; i++)
    {
      if (NULL != pList[i].pObj)
	{
	  for (j = i + 1; j < nObj; j++)
	    {
	      if ((pList[i].pObj == pList[j].pObj))
		{
		  /* set the pointer to zero for all duplicates */
		  pList[j].pObj = NULL;
		}
	    }
	  GDIOBJ_UnlockObj(pList[i].hObj, pList[i].ObjectType);
	  pList[i].pObj = NULL;
	}
    }

  return TRUE;
}

/*!
 * Marks the object as global. (Creator process ID is set to GDI_GLOBAL_PROCESS). Global objects may be
 * accessed by any process.
 * \param 	ObjectHandle - handle of the object to make global.
 *
 * \note	Only stock objects should be marked global.
*/
VOID FASTCALL
GDIOBJ_MarkObjectGlobal(HGDIOBJ ObjectHandle)
{
  PGDIOBJHDR ObjHdr;

  DPRINT("GDIOBJ_MarkObjectGlobal handle 0x%08x\n", ObjectHandle);
  ObjHdr = GDIOBJ_iGetObjectForIndex(GDI_HANDLE_GET_INDEX(ObjectHandle));
  if (NULL == ObjHdr)
    {
      return;
    }

  ObjHdr->hProcessId = GDI_GLOBAL_PROCESS;
}

/*!
 * Removes the global mark from the object. Global objects may be
 * accessed by any process.
 * \param 	ObjectHandle - handle of the object to make local.
 *
 * \note	Only stock objects should be marked global.
*/
VOID FASTCALL
GDIOBJ_UnmarkObjectGlobal(HGDIOBJ ObjectHandle)
{
  PGDIOBJHDR ObjHdr;

  DPRINT("GDIOBJ_MarkObjectGlobal handle 0x%08x\n", ObjectHandle);
  ObjHdr = GDIOBJ_iGetObjectForIndex(GDI_HANDLE_GET_INDEX(ObjectHandle));
  if (NULL == ObjHdr || GDI_GLOBAL_PROCESS != ObjHdr->hProcessId)
    {
      return;
    }

  ObjHdr->hProcessId = PsGetCurrentProcessId();
}

/*!
 * Get the type of the object.
 * \param 	ObjectHandle - handle of the object.
 * \return 	One of the \ref GDI object types
*/
DWORD FASTCALL
GDIOBJ_GetObjectType(HGDIOBJ ObjectHandle)
{
  PGDIOBJHDR ObjHdr;

  ObjHdr = GDIOBJ_iGetObjectForIndex(GDI_HANDLE_GET_INDEX(ObjectHandle));
  if (NULL == ObjHdr)
    {
      DPRINT1("Invalid ObjectHandle 0x%08x\n", ObjectHandle);
      return 0;
    }
  DPRINT("GDIOBJ_GetObjectType for handle 0x%08x returns 0x%08x\n", ObjectHandle,
         GDI_MAGIC_TO_TYPE(ObjHdr->Magic));

  return GDI_MAGIC_TO_TYPE(ObjHdr->Magic);
}

/*!
 * Initialization of the GDI object engine.
*/
VOID FASTCALL
InitGdiObjectHandleTable (VOID)
{
  DPRINT("InitGdiObjectHandleTable\n");
  ExInitializeFastMutex (&HandleTableMutex);
  ExInitializeFastMutex (&RefCountHandling);

  HandleTable = GDIOBJ_iAllocHandleTable (GDI_HANDLE_COUNT);
  DPRINT("HandleTable: %x\n", HandleTable );

  InitEngHandleTable();
}

/*!
 * Creates a bunch of stock objects: brushes, pens, fonts.
*/
VOID FASTCALL
CreateStockObjects(void)
{
  unsigned Object;

  DPRINT("Beginning creation of stock objects\n");

  /* Create GDI Stock Objects from the logical structures we've defined */

  StockObjects[WHITE_BRUSH] =  NtGdiCreateBrushIndirect(&WhiteBrush);
  StockObjects[LTGRAY_BRUSH] = NtGdiCreateBrushIndirect(&LtGrayBrush);
  StockObjects[GRAY_BRUSH] =   NtGdiCreateBrushIndirect(&GrayBrush);
  StockObjects[DKGRAY_BRUSH] = NtGdiCreateBrushIndirect(&DkGrayBrush);
  StockObjects[BLACK_BRUSH] =  NtGdiCreateBrushIndirect(&BlackBrush);
  StockObjects[NULL_BRUSH] =   NtGdiCreateBrushIndirect(&NullBrush);

  StockObjects[WHITE_PEN] = NtGdiCreatePenIndirect(&WhitePen);
  StockObjects[BLACK_PEN] = NtGdiCreatePenIndirect(&BlackPen);
  StockObjects[NULL_PEN] =  NtGdiCreatePenIndirect(&NullPen);

  (void) TextIntCreateFontIndirect(&OEMFixedFont, (HFONT*)&StockObjects[OEM_FIXED_FONT]);
  (void) TextIntCreateFontIndirect(&AnsiFixedFont, (HFONT*)&StockObjects[ANSI_FIXED_FONT]);
  (void) TextIntCreateFontIndirect(&SystemFont, (HFONT*)&StockObjects[SYSTEM_FONT]);
  (void) TextIntCreateFontIndirect(&DeviceDefaultFont, (HFONT*)&StockObjects[DEVICE_DEFAULT_FONT]);
  (void) TextIntCreateFontIndirect(&SystemFixedFont, (HFONT*)&StockObjects[SYSTEM_FIXED_FONT]);
  (void) TextIntCreateFontIndirect(&DefaultGuiFont, (HFONT*)&StockObjects[DEFAULT_GUI_FONT]);

  StockObjects[DEFAULT_PALETTE] = (HGDIOBJ*)PALETTE_Init();

  for (Object = 0; Object < NB_STOCK_OBJECTS; Object++)
    {
      if (NULL != StockObjects[Object])
	{
	  GDIOBJ_MarkObjectGlobal(StockObjects[Object]);
/*	  GDI_HANDLE_SET_STOCKOBJ(StockObjects[Object]);*/
	}
    }

  DPRINT("Completed creation of stock objects\n");
}

/*!
 * Return stock object.
 * \param	Object - stock object id.
 * \return	Handle to the object.
*/
HGDIOBJ STDCALL
NtGdiGetStockObject(INT Object)
{
  DPRINT("NtGdiGetStockObject index %d\n", Object);

  return ((Object < 0) || (NB_STOCK_OBJECTS <= Object)) ? NULL : StockObjects[Object];
}

/*!
 * Delete GDI object
 * \param	hObject object handle
 * \return	if the function fails the returned value is FALSE.
*/
BOOL STDCALL
NtGdiDeleteObject(HGDIOBJ hObject)
{
  DPRINT("NtGdiDeleteObject handle 0x%08x\n", hObject);

  return GDIOBJ_FreeObj(hObject, GDI_OBJECT_TYPE_DONTCARE, GDIOBJFLAG_DEFAULT);
}

/*!
 * Internal function. Called when the process is destroyed to free the remaining GDI handles.
 * \param	Process - PID of the process that will be destroyed.
*/
BOOL FASTCALL
CleanupForProcess (struct _EPROCESS *Process, INT Pid)
{
  DWORD i;
  PGDIOBJHDR objectHeader;
  PEPROCESS CurrentProcess;

  DPRINT("Starting CleanupForProcess prochandle %x Pid %d\n", Process, Pid);
  CurrentProcess = PsGetCurrentProcess();
  if (CurrentProcess != Process)
    {
      KeAttachProcess(Process);
    }

  for(i = 1; i < HandleTable->wTableSize; i++)
    {
      objectHeader = GDIOBJ_iGetObjectForIndex(i);
      if (NULL != objectHeader &&
          (INT) objectHeader->hProcessId == Pid)
	{
	  DPRINT("CleanupForProcess: %d, process: %d, locks: %d, magic: 0x%x", i, objectHeader->hProcessId, objectHeader->dwCount, objectHeader->Magic);
	  GDIOBJ_FreeObj(GDI_HANDLE_CREATE(i, GDI_OBJECT_TYPE_DONTCARE),
	                 GDI_OBJECT_TYPE_DONTCARE,
	                 GDIOBJFLAG_IGNOREPID | GDIOBJFLAG_IGNORELOCK);
	}
    }

  if (CurrentProcess != Process)
    {
      KeDetachProcess();
    }

  DPRINT("Completed cleanup for process %d\n", Pid);

  return TRUE;
}

#define GDIOBJ_TRACKLOCKS

#ifdef GDIOBJ_LockObj
#undef GDIOBJ_LockObj
PGDIOBJ FASTCALL
GDIOBJ_LockObjDbg (const char* file, int line, HGDIOBJ hObj, DWORD ObjectType)
{
  PGDIOBJ rc;
  PGDIOBJHDR ObjHdr = GDIOBJ_iGetObjectForIndex(GDI_HANDLE_GET_INDEX(hObj));

  if (! GDI_VALID_OBJECT(hObj, ObjHdr, ObjectType, GDIOBJFLAG_DEFAULT))
    {
      int reason = 0;
      if (NULL == ObjHdr)
	{
	  reason = 1;
	}
      else if (GDI_MAGIC_TO_TYPE(ObjHdr->Magic) != ObjectType && ObjectType != GDI_OBJECT_TYPE_DONTCARE)
	{
	  reason = 2;
	}
      else if (ObjHdr->hProcessId != GDI_GLOBAL_PROCESS
	   && ObjHdr->hProcessId != PsGetCurrentProcessId())
	{
	  reason = 3;
	}
      else if (GDI_HANDLE_GET_TYPE(hObj) != ObjectType && ObjectType != GDI_OBJECT_TYPE_DONTCARE)
	{
	  reason = 4;
	}
      DPRINT1("GDIOBJ_LockObj failed for 0x%08x, reqtype 0x%08x reason %d\n",
              hObj, ObjectType, reason );
      DPRINT1("\tcalled from: %s:%i\n", file, line );
      return NULL;
    }
  if (NULL != ObjHdr->lockfile)
    {
      DPRINT1("Caution! GDIOBJ_LockObj trying to lock object (0x%x) second time\n", hObj );
      DPRINT1("\tcalled from: %s:%i\n", file, line );
      DPRINT1("\tpreviously locked from: %s:%i\n", ObjHdr->lockfile, ObjHdr->lockline );
    }
  DPRINT("(%s:%i) GDIOBJ_LockObj(0x%08x,0x%08x)\n", file, line, hObj, ObjectType);
  rc = GDIOBJ_LockObj(hObj, ObjectType);
  if (rc && NULL == ObjHdr->lockfile)
    {
      ObjHdr->lockfile = file;
      ObjHdr->lockline = line;
    }

  return rc;
}
#endif//GDIOBJ_LockObj

#ifdef GDIOBJ_UnlockObj
#undef GDIOBJ_UnlockObj
BOOL FASTCALL
GDIOBJ_UnlockObjDbg (const char* file, int line, HGDIOBJ hObj, DWORD ObjectType)
{
  PGDIOBJHDR ObjHdr = GDIOBJ_iGetObjectForIndex(GDI_HANDLE_GET_INDEX(hObj));

  if (! GDI_VALID_OBJECT(hObj, ObjHdr, ObjectType, GDIOBJFLAG_DEFAULT))
    {
      DPRINT1("GDIBOJ_UnlockObj failed for 0x%08x, reqtype 0x%08x\n",
		  hObj, ObjectType);
      DPRINT1("\tcalled from: %s:%i\n", file, line);
      return FALSE;
    }
  DPRINT("(%s:%i) GDIOBJ_UnlockObj(0x%08x,0x%08x)\n", file, line, hObj, ObjectType);
  ObjHdr->lockfile = NULL;
  ObjHdr->lockline = 0;

  return GDIOBJ_UnlockObj(hObj, ObjectType);
}
#endif//GDIOBJ_LockObj

/*!
 * Return pointer to the object by handle.
 *
 * \param hObj 		Object handle
 * \param ObjectType	one of the object types defined in \ref GDI object types
 * \return		Pointer to the object.
 *
 * \note Process can only get pointer to the objects it created or global objects.
 *
 * \todo Don't allow to lock the objects twice! Synchronization!
*/
PGDIOBJ FASTCALL
GDIOBJ_LockObj(HGDIOBJ hObj, DWORD ObjectType)
{
  PGDIOBJHDR ObjHdr = GDIOBJ_iGetObjectForIndex(GDI_HANDLE_GET_INDEX(hObj));

  DPRINT("GDIOBJ_LockObj: hObj: 0x%08x, type: 0x%08x, objhdr: %x\n", hObj, ObjectType, ObjHdr);
  if (! GDI_VALID_OBJECT(hObj, ObjHdr, ObjectType, GDIOBJFLAG_DEFAULT))
    {
      DPRINT1("GDIBOJ_LockObj failed for 0x%08x, type 0x%08x\n",
		  hObj, ObjectType);
      return NULL;
    }

  if(0 < ObjHdr->dwCount)
    {
      DPRINT1("Caution! GDIOBJ_LockObj trying to lock object (0x%x) second time\n", hObj);
      DPRINT1("\t called from: %x\n", __builtin_return_address(0));
    }

  ExAcquireFastMutex(&RefCountHandling);
  ObjHdr->dwCount++;
  ExReleaseFastMutex(&RefCountHandling);
  return (PGDIOBJ)((PCHAR)ObjHdr + sizeof(GDIOBJHDR));
}

/*!
 * Release GDI object. Every object locked by GDIOBJ_LockObj() must be unlocked. You should unlock the object
 * as soon as you don't need to have access to it's data.

 * \param hObj 		Object handle
 * \param ObjectType	one of the object types defined in \ref GDI object types
 *
 * \note This function performs delayed cleanup. If the object is locked when GDI_FreeObj() is called
 * then \em this function frees the object when reference count is zero.
 *
 * \todo Change synchronization algorithm.
*/
#undef GDIOBJ_UnlockObj
BOOL FASTCALL
GDIOBJ_UnlockObj(HGDIOBJ hObj, DWORD ObjectType)
{
  PGDIOBJHDR ObjHdr = GDIOBJ_iGetObjectForIndex(GDI_HANDLE_GET_INDEX(hObj));

  DPRINT("GDIOBJ_UnlockObj: hObj: 0x%08x, type: 0x%08x, objhdr: %x\n", hObj, ObjectType, ObjHdr);
  if (! GDI_VALID_OBJECT(hObj, ObjHdr, ObjectType, GDIOBJFLAG_DEFAULT))
    {
    DPRINT1( "GDIOBJ_UnLockObj: failed\n");
    return FALSE;
  }

  ExAcquireFastMutex(&RefCountHandling);
  if (0 == (ObjHdr->dwCount & ~0x80000000))
    {
      ExReleaseFastMutex(&RefCountHandling);
      DPRINT1( "GDIOBJ_UnLockObj: unlock object (0x%x) that is not locked\n", hObj );
      return FALSE;
    }

  ObjHdr->dwCount--;

  if (ObjHdr->dwCount == 0x80000000)
    {
      //delayed object release
      ObjHdr->dwCount = 0;
      ExReleaseFastMutex(&RefCountHandling);
      DPRINT("GDIOBJ_UnlockObj: delayed delete\n");
      return GDIOBJ_FreeObj(hObj, ObjectType, GDIOBJFLAG_DEFAULT);
    }
  ExReleaseFastMutex(&RefCountHandling);

  return TRUE;
}

/* EOF */
