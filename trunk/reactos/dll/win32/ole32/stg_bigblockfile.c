/******************************************************************************
 *
 * BigBlockFile
 *
 * This is the implementation of a file that consists of blocks of
 * a predetermined size.
 * This class is used in the Compound File implementation of the
 * IStorage and IStream interfaces. It provides the functionality
 * to read and write any blocks in the file as well as setting and
 * obtaining the size of the file.
 * The blocks are indexed sequentially from the start of the file
 * starting with -1.
 *
 * TODO:
 * - Support for a transacted mode
 *
 * Copyright 1999 Thuy Nguyen
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winerror.h"
#include "objbase.h"
#include "ole2.h"

#include "storage32.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(storage);

/***********************************************************
 * Data structures used internally by the BigBlockFile
 * class.
 */

/* We map in PAGE_SIZE-sized chunks. Must be a multiple of 4096. */
#define PAGE_SIZE       131072

/* We keep a list of recently-discarded pages. This controls the
 * size of that list. */
#define MAX_VICTIM_PAGES 16

/***
 * This structure identifies the paged that are mapped
 * from the file and their position in memory. It is
 * also used to hold a reference count to those pages.
 *
 * page_index identifies which PAGE_SIZE chunk from the
 * file this mapping represents. (The mappings are always
 * PAGE_SIZE-aligned.)
 */

typedef struct MappedPage MappedPage;
struct MappedPage
{
    MappedPage *next;
    MappedPage *prev;

    DWORD  page_index;
    DWORD  mapped_bytes;
    LPVOID lpBytes;
    LONG   refcnt;
};

struct BigBlockFile
{
    BOOL fileBased;
    ULARGE_INTEGER filesize;
    HANDLE hfile;
    HANDLE hfilemap;
    DWORD flProtect;
    MappedPage *maplist;
    MappedPage *victimhead, *victimtail;
    ULONG num_victim_pages;
    ILockBytes *pLkbyt;
};

/***********************************************************
 * Prototypes for private methods
 */

/* Note that this evaluates a and b multiple times, so don't
 * pass expressions with side effects. */
#define ROUND_UP(a, b) ((((a) + (b) - 1)/(b))*(b))

/******************************************************************************
 *      BIGBLOCKFILE_FileInit
 *
 * Initialize a big block object supported by a file.
 */
static BOOL BIGBLOCKFILE_FileInit(LPBIGBLOCKFILE This, HANDLE hFile)
{
  This->pLkbyt = NULL;
  This->hfile = hFile;

  if (This->hfile == INVALID_HANDLE_VALUE)
    return FALSE;

  This->filesize.u.LowPart = GetFileSize(This->hfile,
					 &This->filesize.u.HighPart);

  if( This->filesize.u.LowPart || This->filesize.u.HighPart )
  {
    /* create the file mapping object
     */
    This->hfilemap = CreateFileMappingA(This->hfile,
                                        NULL,
                                        This->flProtect,
                                        0, 0,
                                        NULL);

    if (!This->hfilemap)
    {
      CloseHandle(This->hfile);
      return FALSE;
    }
  }
  else
    This->hfilemap = NULL;

  This->maplist = NULL;

  TRACE("file len %u\n", This->filesize.u.LowPart);

  return TRUE;
}

/******************************************************************************
 *      BIGBLOCKFILE_LockBytesInit
 *
 * Initialize a big block object supported by an ILockBytes.
 */
static BOOL BIGBLOCKFILE_LockBytesInit(LPBIGBLOCKFILE This, ILockBytes* plkbyt)
{
    This->hfile    = 0;
    This->hfilemap = 0;
    This->pLkbyt   = plkbyt;
    ILockBytes_AddRef(This->pLkbyt);

    /* We'll get the size directly with ILockBytes_Stat */
    This->filesize.QuadPart = 0;

    TRACE("ILockBytes %p\n", This->pLkbyt);
    return TRUE;
}

/******************************************************************************
 *      BIGBLOCKFILE_FindPageInList      [PRIVATE]
 *
 */
static MappedPage *BIGBLOCKFILE_FindPageInList(MappedPage *head,
					       ULONG page_index)
{
    for (; head != NULL; head = head->next)
    {
	if (head->page_index == page_index)
	{
	    InterlockedIncrement(&head->refcnt);
	    break;
	}
    }

    return head;

}

static void BIGBLOCKFILE_UnlinkPage(MappedPage *page)
{
    if (page->next) page->next->prev = page->prev;
    if (page->prev) page->prev->next = page->next;
}

static void BIGBLOCKFILE_LinkHeadPage(MappedPage **head, MappedPage *page)
{
    if (*head) (*head)->prev = page;
    page->next = *head;
    page->prev = NULL;
    *head = page;
}

static BOOL BIGBLOCKFILE_MapPage(BigBlockFile *This, MappedPage *page)
{
    DWORD lowoffset = PAGE_SIZE * page->page_index;
    DWORD numBytesToMap;
    DWORD desired_access;

    assert(This->fileBased);

    if( !This->hfilemap )
        return FALSE;

    if (lowoffset + PAGE_SIZE > This->filesize.u.LowPart)
        numBytesToMap = This->filesize.u.LowPart - lowoffset;
    else
        numBytesToMap = PAGE_SIZE;

    if (This->flProtect == PAGE_READONLY)
        desired_access = FILE_MAP_READ;
    else
        desired_access = FILE_MAP_WRITE;

    page->lpBytes = MapViewOfFile(This->hfilemap, desired_access, 0,
                                  lowoffset, numBytesToMap);
    page->mapped_bytes = numBytesToMap;

    TRACE("mapped page %u to %p\n", page->page_index, page->lpBytes);

    return page->lpBytes != NULL;
}


static MappedPage *BIGBLOCKFILE_CreatePage(BigBlockFile *This, ULONG page_index)
{
    MappedPage *page;

    page = HeapAlloc(GetProcessHeap(), 0, sizeof(MappedPage));
    if (page == NULL)
        return NULL;

    page->page_index = page_index;
    page->refcnt = 1;

    page->next = NULL;
    page->prev = NULL;

    if (!BIGBLOCKFILE_MapPage(This, page))
    {
        HeapFree(GetProcessHeap(),0,page);
        return NULL;
    }

    return page;
}


/******************************************************************************
 *      BIGBLOCKFILE_GetMappedView      [PRIVATE]
 *
 * Gets the page requested if it is already mapped.
 * If it's not already mapped, this method will map it
 */
static void * BIGBLOCKFILE_GetMappedView(
  LPBIGBLOCKFILE This,
  DWORD          page_index)
{
    MappedPage *page;

    page = BIGBLOCKFILE_FindPageInList(This->maplist, page_index);
    if (!page)
    {
	page = BIGBLOCKFILE_FindPageInList(This->victimhead, page_index);
	if (page)
	{
	    This->num_victim_pages--;
	}
    }

    if (page)
    {
	/* If the page is not already at the head of the list, move
	 * it there. (Also moves pages from victim to main list.) */
	if (This->maplist != page)
	{
	    if (This->victimhead == page) This->victimhead = page->next;
	    if (This->victimtail == page) This->victimtail = page->prev;

	    BIGBLOCKFILE_UnlinkPage(page);

	    BIGBLOCKFILE_LinkHeadPage(&This->maplist, page);
	}

	return page;
    }

    page = BIGBLOCKFILE_CreatePage(This, page_index);
    if (!page) return NULL;

    BIGBLOCKFILE_LinkHeadPage(&This->maplist, page);

    return page;
}

static void BIGBLOCKFILE_UnmapPage(LPBIGBLOCKFILE This, MappedPage *page)
{
    TRACE("%d at %p\n", page->page_index, page->lpBytes);

    assert(This->fileBased);

    if (page->refcnt > 0)
	ERR("unmapping inuse page %p\n", page->lpBytes);

    if (page->lpBytes)
	UnmapViewOfFile(page->lpBytes);

    page->lpBytes = NULL;
}

static void BIGBLOCKFILE_DeletePage(LPBIGBLOCKFILE This, MappedPage *page)
{
    BIGBLOCKFILE_UnmapPage(This, page);

    HeapFree(GetProcessHeap(), 0, page);
}

/******************************************************************************
 *      BIGBLOCKFILE_ReleaseMappedPage      [PRIVATE]
 *
 * Decrements the reference count of the mapped page.
 */
static void BIGBLOCKFILE_ReleaseMappedPage(
  LPBIGBLOCKFILE This,
  MappedPage    *page)
{
    assert(This != NULL);
    assert(page != NULL);

    /* If the page is no longer refenced, move it to the victim list.
     * If the victim list is too long, kick somebody off. */
    if (!InterlockedDecrement(&page->refcnt))
    {
	if (This->maplist == page) This->maplist = page->next;

	BIGBLOCKFILE_UnlinkPage(page);

	if (MAX_VICTIM_PAGES > 0)
	{
	    if (This->num_victim_pages >= MAX_VICTIM_PAGES)
	    {
		MappedPage *victim = This->victimtail;
		if (victim)
		{
		    This->victimtail = victim->prev;
		    if (This->victimhead == victim)
			This->victimhead = victim->next;

		    BIGBLOCKFILE_UnlinkPage(victim);
		    BIGBLOCKFILE_DeletePage(This, victim);
		}
	    }
	    else This->num_victim_pages++;

	    BIGBLOCKFILE_LinkHeadPage(&This->victimhead, page);
	    if (This->victimtail == NULL) This->victimtail = page;
	}
	else
	    BIGBLOCKFILE_DeletePage(This, page);
    }
}

static void BIGBLOCKFILE_DeleteList(LPBIGBLOCKFILE This, MappedPage *list)
{
    while (list != NULL)
    {
	MappedPage *next = list->next;

	BIGBLOCKFILE_DeletePage(This, list);

	list = next;
    }
}

/******************************************************************************
 *      BIGBLOCKFILE_FreeAllMappedPages     [PRIVATE]
 *
 * Unmap all currently mapped pages.
 * Empty mapped pages list.
 */
static void BIGBLOCKFILE_FreeAllMappedPages(
  LPBIGBLOCKFILE This)
{
    BIGBLOCKFILE_DeleteList(This, This->maplist);
    BIGBLOCKFILE_DeleteList(This, This->victimhead);

    This->maplist = NULL;
    This->victimhead = NULL;
    This->victimtail = NULL;
    This->num_victim_pages = 0;
}

static void BIGBLOCKFILE_UnmapList(LPBIGBLOCKFILE This, MappedPage *list)
{
    for (; list != NULL; list = list->next)
    {
	BIGBLOCKFILE_UnmapPage(This, list);
    }
}

static void BIGBLOCKFILE_UnmapAllMappedPages(LPBIGBLOCKFILE This)
{
    BIGBLOCKFILE_UnmapList(This, This->maplist);
    BIGBLOCKFILE_UnmapList(This, This->victimhead);
}

static void BIGBLOCKFILE_RemapList(LPBIGBLOCKFILE This, MappedPage *list)
{
    while (list != NULL)
    {
	MappedPage *next = list->next;

	if (list->page_index * PAGE_SIZE > This->filesize.u.LowPart)
	{
            TRACE("discarding %u\n", list->page_index);

	    /* page is entirely outside of the file, delete it */
	    BIGBLOCKFILE_UnlinkPage(list);
	    BIGBLOCKFILE_DeletePage(This, list);
	}
	else
	{
	    /* otherwise, remap it */
	    BIGBLOCKFILE_MapPage(This, list);
	}

	list = next;
    }
}

static void BIGBLOCKFILE_RemapAllMappedPages(LPBIGBLOCKFILE This)
{
    BIGBLOCKFILE_RemapList(This, This->maplist);
    BIGBLOCKFILE_RemapList(This, This->victimhead);
}

/****************************************************************************
 *      BIGBLOCKFILE_GetProtectMode
 *
 * This function will return a protection mode flag for a file-mapping object
 * from the open flags of a file.
 */
static DWORD BIGBLOCKFILE_GetProtectMode(DWORD openFlags)
{
    switch(STGM_ACCESS_MODE(openFlags))
    {
    case STGM_WRITE:
    case STGM_READWRITE:
        return PAGE_READWRITE;
    }
    return PAGE_READONLY;
}


/* ILockByte Interfaces */

/******************************************************************************
 * This method is part of the ILockBytes interface.
 *
 * It reads a block of information from the byte array at the specified
 * offset.
 *
 * See the documentation of ILockBytes for more info.
 */
static HRESULT ImplBIGBLOCKFILE_ReadAt(
      BigBlockFile* const This,
      ULARGE_INTEGER ulOffset,  /* [in] */
      void*          pv,        /* [length_is][size_is][out] */
      ULONG          cb,        /* [in] */
      ULONG*         pcbRead)   /* [out] */
{
    ULONG first_page = ulOffset.u.LowPart / PAGE_SIZE;
    ULONG offset_in_page = ulOffset.u.LowPart % PAGE_SIZE;
    ULONG bytes_left = cb;
    ULONG page_index = first_page;
    ULONG bytes_from_page;
    LPVOID writePtr = pv;

    HRESULT rc = S_OK;

    TRACE("(%p)-> %i %p %i %p\n",This, ulOffset.u.LowPart, pv, cb, pcbRead);

    /* verify a sane environment */
    if (!This) return E_FAIL;

    if (offset_in_page + bytes_left > PAGE_SIZE)
        bytes_from_page = PAGE_SIZE - offset_in_page;
    else
        bytes_from_page = bytes_left;

    if (pcbRead)
        *pcbRead = 0;

    while (bytes_left)
    {
        LPBYTE readPtr;
        BOOL eof = FALSE;
        MappedPage *page = BIGBLOCKFILE_GetMappedView(This, page_index);

        if (!page || !page->lpBytes)
        {
            rc = STG_E_READFAULT;
            break;
        }

        TRACE("page %i,  offset %u, bytes_from_page %u, bytes_left %u\n",
            page->page_index, offset_in_page, bytes_from_page, bytes_left);

        if (page->mapped_bytes < bytes_from_page)
        {
            eof = TRUE;
            bytes_from_page = page->mapped_bytes;
        }

        readPtr = (BYTE*)page->lpBytes + offset_in_page;
        memcpy(writePtr,readPtr,bytes_from_page);
        BIGBLOCKFILE_ReleaseMappedPage(This, page);

        if (pcbRead)
            *pcbRead += bytes_from_page;
        bytes_left -= bytes_from_page;

        if (bytes_left && !eof)
        {
            writePtr = (LPBYTE)writePtr + bytes_from_page;
            page_index ++;
            offset_in_page = 0;
            if (bytes_left > PAGE_SIZE)
                bytes_from_page = PAGE_SIZE;
            else
                bytes_from_page = bytes_left;
        }
        if (eof)
        {
            rc = STG_E_READFAULT;
            break;
        }
    }

    TRACE("finished\n");
    return rc;
}

/******************************************************************************
 * This method is part of the ILockBytes interface.
 *
 * It writes the specified bytes at the specified offset.
 * position. If the file is too small, it will be resized.
 *
 * See the documentation of ILockBytes for more info.
 */
static HRESULT ImplBIGBLOCKFILE_WriteAt(
      BigBlockFile* const This,
      ULARGE_INTEGER ulOffset,    /* [in] */
      const void*    pv,          /* [size_is][in] */
      ULONG          cb,          /* [in] */
      ULONG*         pcbWritten)  /* [out] */
{
    ULONG size_needed = ulOffset.u.LowPart + cb;
    ULONG first_page = ulOffset.u.LowPart / PAGE_SIZE;
    ULONG offset_in_page = ulOffset.u.LowPart % PAGE_SIZE;
    ULONG bytes_left = cb;
    ULONG page_index = first_page;
    ULONG bytes_to_page;
    LPCVOID readPtr = pv;

    HRESULT rc = S_OK;

    TRACE("(%p)-> %i %p %i %p\n",This, ulOffset.u.LowPart, pv, cb, pcbWritten);

    /* verify a sane environment */
    if (!This) return E_FAIL;

    if (This->flProtect != PAGE_READWRITE)
        return STG_E_ACCESSDENIED;

    if (size_needed > This->filesize.u.LowPart)
    {
        ULARGE_INTEGER newSize;
        newSize.u.HighPart = 0;
        newSize.u.LowPart = size_needed;
        BIGBLOCKFILE_SetSize(This, newSize);
    }

    if (offset_in_page + bytes_left > PAGE_SIZE)
        bytes_to_page = PAGE_SIZE - offset_in_page;
    else
        bytes_to_page = bytes_left;

    if (pcbWritten)
        *pcbWritten = 0;

    while (bytes_left)
    {
        LPBYTE writePtr;
        MappedPage *page = BIGBLOCKFILE_GetMappedView(This, page_index);

        TRACE("page %i,  offset %u, bytes_to_page %u, bytes_left %u\n",
            page ? page->page_index : 0, offset_in_page, bytes_to_page, bytes_left);

        if (!page)
        {
            ERR("Unable to get a page to write. This should never happen\n");
            rc = E_FAIL;
            break;
        }

        if (page->mapped_bytes < bytes_to_page)
        {
            ERR("Not enough bytes mapped to the page. This should never happen\n");
            rc = E_FAIL;
            break;
        }

        writePtr = (BYTE*)page->lpBytes + offset_in_page;
        memcpy(writePtr,readPtr,bytes_to_page);
        BIGBLOCKFILE_ReleaseMappedPage(This, page);

        if (pcbWritten)
            *pcbWritten += bytes_to_page;
        bytes_left -= bytes_to_page;

        if (bytes_left)
        {
            readPtr = (const BYTE *)readPtr + bytes_to_page;
            page_index ++;
            offset_in_page = 0;
            if (bytes_left > PAGE_SIZE)
                bytes_to_page = PAGE_SIZE;
            else
                bytes_to_page = bytes_left;
        }
    }

    return rc;
}

/******************************************************************************
 *      BIGBLOCKFILE_Construct
 *
 * Construct a big block file. Create the file mapping object.
 * Create the read only mapped pages list, the writable mapped page list
 * and the blocks in use list.
 */
BigBlockFile *BIGBLOCKFILE_Construct(HANDLE hFile, ILockBytes* pLkByt, DWORD openFlags,
                                     BOOL fileBased)
{
    BigBlockFile *This;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(BigBlockFile));

    if (This == NULL)
        return NULL;

    This->fileBased = fileBased;
    This->flProtect = BIGBLOCKFILE_GetProtectMode(openFlags);

    This->maplist = NULL;
    This->victimhead = NULL;
    This->victimtail = NULL;
    This->num_victim_pages = 0;

    if (This->fileBased)
    {
        if (!BIGBLOCKFILE_FileInit(This, hFile))
        {
            HeapFree(GetProcessHeap(), 0, This);
            return NULL;
        }
    }
    else
    {
        if (!BIGBLOCKFILE_LockBytesInit(This, pLkByt))
        {
            HeapFree(GetProcessHeap(), 0, This);
            return NULL;
        }
    }

    return This;
}

/******************************************************************************
 *      BIGBLOCKFILE_Destructor
 *
 * Destructor. Clean up, free memory.
 */
void BIGBLOCKFILE_Destructor(BigBlockFile *This)
{
    BIGBLOCKFILE_FreeAllMappedPages(This);

    if (This->fileBased)
    {
        CloseHandle(This->hfilemap);
        CloseHandle(This->hfile);
    }
    else
    {
        ILockBytes_Release(This->pLkbyt);
    }

    HeapFree(GetProcessHeap(), 0, This);
}

/******************************************************************************
 *      BIGBLOCKFILE_ReadAt
 */
HRESULT BIGBLOCKFILE_ReadAt(BigBlockFile *This, ULARGE_INTEGER offset,
                            void* buffer, ULONG size, ULONG* bytesRead)
{
    if (This->fileBased)
        return ImplBIGBLOCKFILE_ReadAt(This,offset,buffer,size,bytesRead);
    else
        return ILockBytes_ReadAt(This->pLkbyt,offset,buffer,size,bytesRead);
}

/******************************************************************************
 *      BIGBLOCKFILE_WriteAt
 */
HRESULT BIGBLOCKFILE_WriteAt(BigBlockFile *This, ULARGE_INTEGER offset,
                             const void* buffer, ULONG size, ULONG* bytesRead)
{
    if (This->fileBased)
        return ImplBIGBLOCKFILE_WriteAt(This,offset,buffer,size,bytesRead);
    else
        return ILockBytes_WriteAt(This->pLkbyt,offset,buffer,size,bytesRead);
}

/******************************************************************************
 *      BIGBLOCKFILE_SetSize
 *
 * Sets the size of the file.
 *
 */
HRESULT BIGBLOCKFILE_SetSize(BigBlockFile *This, ULARGE_INTEGER newSize)
{
    HRESULT hr = S_OK;
    LARGE_INTEGER newpos;

    if (!This->fileBased)
        return ILockBytes_SetSize(This->pLkbyt, newSize);

    if (This->filesize.u.LowPart == newSize.u.LowPart)
        return hr;

    TRACE("from %u to %u\n", This->filesize.u.LowPart, newSize.u.LowPart);

    /*
     * Unmap all views, must be done before call to SetEndFile.
     *
     * Just ditch the victim list because there is no guarantee we will need them
     * and it is not worth the performance hit to unmap and remap them all.
     */
    BIGBLOCKFILE_DeleteList(This, This->victimhead);
    This->victimhead = NULL;
    This->victimtail = NULL;
    This->num_victim_pages = 0;

    BIGBLOCKFILE_UnmapAllMappedPages(This);

    newpos.QuadPart = newSize.QuadPart;
    if (SetFilePointerEx(This->hfile, newpos, NULL, FILE_BEGIN))
    {
        if( This->hfilemap ) CloseHandle(This->hfilemap);

        SetEndOfFile(This->hfile);

        /* re-create the file mapping object */
        This->hfilemap = CreateFileMappingA(This->hfile, NULL, This->flProtect,
                                            0, 0, NULL);
    }

    This->filesize = newSize;
    BIGBLOCKFILE_RemapAllMappedPages(This);
    return hr;
}

/******************************************************************************
 *      BIGBLOCKFILE_GetSize
 *
 * Gets the size of the file.
 *
 */
static HRESULT BIGBLOCKFILE_GetSize(BigBlockFile *This, ULARGE_INTEGER *size)
{
    HRESULT hr = S_OK;
    if(This->fileBased)
        *size = This->filesize;
    else
    {
        STATSTG stat;
        hr = ILockBytes_Stat(This->pLkbyt, &stat, STATFLAG_NONAME);
        if(SUCCEEDED(hr)) *size = stat.cbSize;
    }
    return hr;
}

/******************************************************************************
 *      BIGBLOCKFILE_Expand
 *
 * Grows the file to the specified size if necessary.
 */
HRESULT BIGBLOCKFILE_Expand(BigBlockFile *This, ULARGE_INTEGER newSize)
{
    ULARGE_INTEGER size;
    HRESULT hr;

    hr = BIGBLOCKFILE_GetSize(This, &size);
    if(FAILED(hr)) return hr;

    if (newSize.QuadPart > size.QuadPart)
        hr = BIGBLOCKFILE_SetSize(This, newSize);
    return hr;
}
