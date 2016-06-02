/*
 * Copyright 2011 Andr� Hentschel
 * Copyright 2013 Mislav Bla�evic
 * Copyright 2015,2016 Mark Jansen
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

#if !defined(SDBWRITE_HOSTTOOL)

#define WIN32_NO_STATUS
#include "windows.h"
#include "ntndk.h"
#include "apphelp.h"
#include "wine/unicode.h"

#else

#include <typedefs.h>
#include <guiddef.h>

#include "sdbtypes.h"
#include "sdbpapi.h"
#include "sdbtagid.h"

#endif

BOOL WINAPI SdbWriteStringRefTag(PDB db, TAG tag, TAGID tagid);
TAGID WINAPI SdbBeginWriteListTag(PDB db, TAG tag);
BOOL WINAPI SdbEndWriteListTag(PDB db, TAGID tagid);


static void WINAPI SdbpWrite(PDB db, const void* data, DWORD size)
{
    if (db->write_iter + size > db->size)
    {
        /* Round to powers of two to prevent too many reallocations */
        while (db->size < db->write_iter + size) db->size <<= 1;
        db->data = SdbReAlloc(db->data, db->size);
    }

    memcpy(db->data + db->write_iter, data, size);
    db->write_iter += size;
}

static BOOL WINAPI SdbpGetOrAddStringRef(PDB db, LPCWSTR string, TAGID* tagid)
{
    /* TODO:
    - Insert or find in stringtable
    - return TAGID
    */

    return FALSE;
}

static void WINAPI SdbpWriteStringtable(PDB db)
{
    TAGID table = SdbBeginWriteListTag(db, TAG_STRINGTABLE);
    /* TODO: Write out all strings*/
    SdbEndWriteListTag(db, table);
}

/**
 * Creates new shim database file
 * 
 * If a file already exists on specified path, that file shall be overwritten.
 * 
 * @note Use SdbCloseDatabasWrite to close the database opened with this function.
 *
 * @param [in]  path    Path to the new shim database.
 * @param [in]  type    Type of path. Either DOS_PATH or NT_PATH.
 *
 * @return  Success: Handle to the newly created shim database, NULL otherwise.
 */
PDB WINAPI SdbCreateDatabase(LPCWSTR path, PATH_TYPE type)
{
    static const DWORD version_major = 2, version_minor = 1;
    static const char* magic = "sdbf";
    PDB db;

    db = SdbpCreate(path, type, TRUE);
    if (!db)
        return NULL;

    db->size = sizeof(DWORD) + sizeof(DWORD) + strlen(magic);
    db->data = SdbAlloc(db->size);

    SdbpWrite(db, &version_major, sizeof(DWORD));
    SdbpWrite(db, &version_minor, sizeof(DWORD));
    SdbpWrite(db, magic, strlen(magic));

    return db;
}

/**
 * Closes specified database and writes data to file.
 *
 * @param [in]  db  Handle to the shim database.
 */
void WINAPI SdbCloseDatabaseWrite(PDB db)
{
    SdbpWriteStringtable(db);
    SdbpFlush(db);
    SdbCloseDatabase(db);
}

/**
 * Writes a tag-only (NULL) entry to the specified shim database.
 *
 * @param [in]  db  Handle to the shim database.
 * @param [in]  tag A tag for the entry.
 *
 * @return  TRUE if it succeeds, FALSE if it fails.
 */
BOOL WINAPI SdbWriteNULLTag(PDB db, TAG tag)
{
    if (!SdbpCheckTagType(tag, TAG_TYPE_NULL))
        return FALSE;

    SdbpWrite(db, &tag, sizeof(TAG));
    return TRUE;
}

/**
 * Writes a WORD entry to the specified shim database.
 *
 * @param [in]  db      Handle to the shim database.
 * @param [in]  tag     A tag for the entry.
 * @param [in]  data    WORD entry which will be written to the database.
 *
 * @return  TRUE if it succeeds, FALSE if it fails.
 */
BOOL WINAPI SdbWriteWORDTag(PDB db, TAG tag, WORD data)
{
    if (!SdbpCheckTagType(tag, TAG_TYPE_WORD))
        return FALSE;

    SdbpWrite(db, &tag, sizeof(TAG));
    SdbpWrite(db, &data, sizeof(data));
    return TRUE;
}

/**
 * Writes a DWORD entry to the specified shim database.
 *
 * @param [in]  db      Handle to the shim database.
 * @param [in]  tag     A tag for the entry.
 * @param [in]  data    DWORD entry which will be written to the database.
 *
 * @return  TRUE if it succeeds, FALSE if it fails.
 */
BOOL WINAPI SdbWriteDWORDTag(PDB db, TAG tag, DWORD data)
{
    if (!SdbpCheckTagType(tag, TAG_TYPE_DWORD))
        return FALSE;

    SdbpWrite(db, &tag, sizeof(TAG));
    SdbpWrite(db, &data, sizeof(data));
    return TRUE;
}

/**
 * Writes a DWORD entry to the specified shim database.
 *
 * @param [in]  db      Handle to the shim database.
 * @param [in]  tag     A tag for the entry.
 * @param [in]  data    QWORD entry which will be written to the database.
 *
 * @return  TRUE if it succeeds, FALSE if it fails.
 */
BOOL WINAPI SdbWriteQWORDTag(PDB db, TAG tag, QWORD data)
{
    if (!SdbpCheckTagType(tag, TAG_TYPE_QWORD))
        return FALSE;

    SdbpWrite(db, &tag, sizeof(TAG));
    SdbpWrite(db, &data, sizeof(data));
    return TRUE;
}

/**
 * Writes a wide string entry to the specified shim database.
 *
 * @param [in]  db      Handle to the shim database.
 * @param [in]  tag     A tag for the entry.
 * @param [in]  string  Wide string entry which will be written to the database.
 *
 * @return  TRUE if it succeeds, FALSE if it fails.
 */
BOOL WINAPI SdbWriteStringTag(PDB db, TAG tag, LPCWSTR string)
{
    DWORD size;

    if (SdbpCheckTagType(tag, TAG_TYPE_STRINGREF))
    {
        TAGID tagid = 0;
        if (!SdbpGetOrAddStringRef(db, string, &tagid))
            return FALSE;

        return SdbWriteStringRefTag(db, tag, tagid);
    }

    if (!SdbpCheckTagType(tag, TAG_TYPE_STRING))
        return FALSE;

    size = SdbpStrsize(string);
    SdbpWrite(db, &tag, sizeof(TAG));
    SdbpWrite(db, &size, sizeof(size));
    SdbpWrite(db, string, size);
    return TRUE;
}

/**
 * Writes a stringref tag to specified database
 * @note Reference (tagid) is not checked for validity.
 *
 * @param [in]  db      Handle to the shim database.
 * @param [in]  tag     TAG which will be written.
 * @param [in]  tagid   TAGID of the string tag refers to.
 *
 * @return  TRUE if it succeeds, FALSE if it fails.
 */
BOOL WINAPI SdbWriteStringRefTag(PDB db, TAG tag, TAGID tagid)
{
    if (!SdbpCheckTagType(tag, TAG_TYPE_STRINGREF))
        return FALSE;

    SdbpWrite(db, &tag, sizeof(TAG));
    SdbpWrite(db, &tagid, sizeof(tagid));
    return TRUE;
}

/**
 * Writes data the specified shim database.
 *
 * @param [in]  db      Handle to the shim database.
 * @param [in]  tag     A tag for the entry.
 * @param [in]  data    Pointer to data.
 * @param [in]  size    Number of bytes to write.
 *
 * @return  TRUE if it succeeds, FALSE if it fails.
 */
BOOL WINAPI SdbWriteBinaryTag(PDB db, TAG tag, BYTE* data, DWORD size)
{
    if (!SdbpCheckTagType(tag, TAG_TYPE_BINARY))
        return FALSE;

    SdbpWrite(db, &tag, sizeof(TAG));
    SdbpWrite(db, &size, sizeof(size));
    SdbpWrite(db, data, size);
    return TRUE;
}

#if !defined(SDBWRITE_HOSTTOOL)
/**
 * Writes data from a file to the specified shim database.
 *
 * @param [in]  db      Handle to the shim database.
 * @param [in]  tag     A tag for the entry.
 * @param [in]  path    Path of the input file.
 *
 * @return  TRUE if it succeeds, FALSE if it fails.
 */
BOOL WINAPI SdbWriteBinaryTagFromFile(PDB db, TAG tag, LPCWSTR path)
{
    MEMMAPPED mapped;

    if (!SdbpCheckTagType(tag, TAG_TYPE_BINARY))
        return FALSE;

    if (!SdbpOpenMemMappedFile(path, &mapped))
        return FALSE;

    SdbWriteBinaryTag(db, tag, mapped.view, mapped.size);
    SdbpCloseMemMappedFile(&mapped);
    return TRUE;
}
#endif

/**
 * Writes a list tag to specified database All subsequent SdbWrite* functions shall write to
 * newly created list untill TAGID of that list is passed to SdbEndWriteListTag.
 *
 * @param [in]  db  Handle to the shim database.
 * @param [in]  tag TAG for the list
 *                  
 *                  RETURNS Success: TAGID of the newly created list, or TAGID_NULL on failure.
 *
 * @return  A TAGID.
 */
TAGID WINAPI SdbBeginWriteListTag(PDB db, TAG tag)
{
    TAGID list_id;
    DWORD dum = 0;

    if (!SdbpCheckTagType(tag, TAG_TYPE_LIST))
        return TAGID_NULL;

    list_id = db->write_iter;
    SdbpWrite(db, &tag, sizeof(TAG));
    SdbpWrite(db, &dum, sizeof(dum)); /* reserve some memory for storing list size */
    return list_id;
}

/**
 * Marks end of the specified list.
 *
 * @param [in]  db      Handle to the shim database.
 * @param [in]  tagid   TAGID of the list.
 *
 * @return  TRUE if it succeeds, FALSE if it fails.
 */
BOOL WINAPI SdbEndWriteListTag(PDB db, TAGID tagid)
{
    if (!SdbpCheckTagIDType(db, tagid, TAG_TYPE_LIST))
        return FALSE;

    /* Write size of list to list tag header */
    *(DWORD*)&db->data[tagid + sizeof(TAG)] = db->write_iter - tagid - sizeof(TAG) - sizeof(TAGID);
    return TRUE;
}

