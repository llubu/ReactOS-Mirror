/*
 * Implementation of the Microsoft Installer (msi.dll)
 *
 * Copyright 2002-2005 Mike McCormack for CodeWeavers
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_MSI_PRIVATE__
#define __WINE_MSI_PRIVATE__

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "msi.h"
#include "msiquery.h"
#include "objbase.h"
#include "objidl.h"
#include "winnls.h"
#include "wine/list.h"

#define MSI_DATASIZEMASK 0x00ff
#define MSITYPE_VALID    0x0100
#define MSITYPE_STRING   0x0800
#define MSITYPE_NULLABLE 0x1000
#define MSITYPE_KEY      0x2000

#define MSITYPE_BINARY 0x8900

struct tagMSITABLE;
typedef struct tagMSITABLE MSITABLE;

struct string_table;
typedef struct string_table string_table;

struct tagMSIOBJECTHDR;
typedef struct tagMSIOBJECTHDR MSIOBJECTHDR;

typedef VOID (*msihandledestructor)( MSIOBJECTHDR * );

struct tagMSIOBJECTHDR
{
    UINT magic;
    UINT type;
    LONG refcount;
    msihandledestructor destructor;
    struct tagMSIOBJECTHDR *next;
    struct tagMSIOBJECTHDR *prev;
};

typedef struct tagMSIDATABASE
{
    MSIOBJECTHDR hdr;
    IStorage *storage;
    string_table *strings;
    LPCWSTR mode;
    MSITABLE *first_table, *last_table;
} MSIDATABASE;

typedef struct tagMSIVIEW MSIVIEW;

typedef struct tagMSIQUERY
{
    MSIOBJECTHDR hdr;
    MSIVIEW *view;
    UINT row;
    MSIDATABASE *db;
    struct list mem;
} MSIQUERY;

/* maybe we can use a Variant instead of doing it ourselves? */
typedef struct tagMSIFIELD
{
    UINT type;
    union
    {
        INT iVal;
        LPWSTR szwVal;
        IStream *stream;
    } u;
} MSIFIELD;

typedef struct tagMSIRECORD
{
    MSIOBJECTHDR hdr;
    UINT count;       /* as passed to MsiCreateRecord */
    MSIFIELD fields[1]; /* nb. array size is count+1 */
} MSIRECORD;

typedef struct tagMSIVIEWOPS
{
    /*
     * fetch_int - reads one integer from {row,col} in the table
     *
     *  This function should be called after the execute method.
     *  Data returned by the function should not change until 
     *   close or delete is called.
     *  To get a string value, query the database's string table with
     *   the integer value returned from this function.
     */
    UINT (*fetch_int)( struct tagMSIVIEW *, UINT row, UINT col, UINT *val );

    /*
     * fetch_int - reads one integer from {row,col} in the table
     *
     *  This function is similar to fetch_int, except fetches a
     *    stream instead of an integer.
     */
    UINT (*fetch_stream)( struct tagMSIVIEW *, UINT row, UINT col, IStream **stm );

    /*
     * get_int - sets one integer at {row,col} in the table
     *
     *  Similar semantics to fetch_int
     */
    UINT (*set_int)( struct tagMSIVIEW *, UINT row, UINT col, UINT val );

    /*
     * Inserts a new row into the database from the records contents
     */
    UINT (*insert_row)( struct tagMSIVIEW *, MSIRECORD * );

    /*
     * execute - loads the underlying data into memory so it can be read
     */
    UINT (*execute)( struct tagMSIVIEW *, MSIRECORD * );

    /*
     * close - clears the data read by execute from memory
     */
    UINT (*close)( struct tagMSIVIEW * );

    /*
     * get_dimensions - returns the number of rows or columns in a table.
     *
     *  The number of rows can only be queried after the execute method
     *   is called. The number of columns can be queried at any time.
     */
    UINT (*get_dimensions)( struct tagMSIVIEW *, UINT *rows, UINT *cols );

    /*
     * get_column_info - returns the name and type of a specific column
     *
     *  The name is HeapAlloc'ed by this function and should be freed by
     *   the caller.
     *  The column information can be queried at any time.
     */
    UINT (*get_column_info)( struct tagMSIVIEW *, UINT n, LPWSTR *name, UINT *type );

    /*
     * modify - not yet implemented properly
     */
    UINT (*modify)( struct tagMSIVIEW *, MSIMODIFY, MSIRECORD * );

    /*
     * delete - destroys the structure completely
     */
    UINT (*delete)( struct tagMSIVIEW * );

} MSIVIEWOPS;

struct tagMSIVIEW
{
    MSIOBJECTHDR hdr;
    MSIVIEWOPS   *ops;
};

struct msi_dialog_tag;
typedef struct msi_dialog_tag msi_dialog;

typedef struct tagMSIPACKAGE
{
    MSIOBJECTHDR hdr;
    MSIDATABASE *db;
    struct list components;
    struct list features;
    struct list files;
    struct list folders;
    LPWSTR ActionFormat;
    LPWSTR LastAction;

    struct list classes;
    struct list extensions;
    struct tagMSIPROGID *progids;
    UINT loaded_progids;
    struct tagMSIVERB *verbs;
    UINT loaded_verbs;
    struct list mimes;
    struct list appids;
    
    struct tagMSISCRIPT *script;

    struct tagMSIRUNNINGACTION *RunningAction;
    UINT RunningActionCount;

    LPWSTR PackagePath;
    LPWSTR msiFilePath;
    LPWSTR ProductCode;

    UINT CurrentInstallState;
    msi_dialog *dialog;
    LPWSTR next_dialog;
    
    struct list subscriptions;
} MSIPACKAGE;

typedef struct tagMSIPREVIEW
{
    MSIOBJECTHDR hdr;
    MSIPACKAGE *package;
    msi_dialog *dialog;
} MSIPREVIEW;

#define MSIHANDLETYPE_ANY 0
#define MSIHANDLETYPE_DATABASE 1
#define MSIHANDLETYPE_SUMMARYINFO 2
#define MSIHANDLETYPE_VIEW 3
#define MSIHANDLETYPE_RECORD 4
#define MSIHANDLETYPE_PACKAGE 5
#define MSIHANDLETYPE_PREVIEW 6

#define MSI_MAJORVERSION 2
#define MSI_MINORVERSION 0
#define MSI_BUILDNUMBER 2600

#define GUID_SIZE 39

#define MSIHANDLE_MAGIC 0x4d434923
#define MSIMAXHANDLES 0xf0

#define MSISUMINFO_OFFSET 0x30LL

DEFINE_GUID(CLSID_IMsiServer,   0x000C101C,0x0000,0x0000,0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46);
DEFINE_GUID(CLSID_IMsiServerX1, 0x000C103E,0x0000,0x0000,0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46);
DEFINE_GUID(CLSID_IMsiServerX2, 0x000C1090,0x0000,0x0000,0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46);
DEFINE_GUID(CLSID_IMsiServerX3, 0x000C1094,0x0000,0x0000,0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46);

DEFINE_GUID(CLSID_IMsiServerMessage, 0x000C101D,0x0000,0x0000,0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46);


/* handle functions */
extern void *msihandle2msiinfo(MSIHANDLE handle, UINT type);
extern MSIHANDLE alloc_msihandle( MSIOBJECTHDR * );
extern void *alloc_msiobject(UINT type, UINT size, msihandledestructor destroy );
extern void msiobj_addref(MSIOBJECTHDR *);
extern int msiobj_release(MSIOBJECTHDR *);
extern void msiobj_lock(MSIOBJECTHDR *);
extern void msiobj_unlock(MSIOBJECTHDR *);
extern MSIHANDLE msiobj_findhandle( MSIOBJECTHDR *hdr );

/* add this table to the list of cached tables in the database */
extern void add_table(MSIDATABASE *db, MSITABLE *table);
extern void remove_table( MSIDATABASE *db, MSITABLE *table );
extern void free_table( MSIDATABASE *db, MSITABLE *table );
extern void free_cached_tables( MSIDATABASE *db );
extern UINT find_cached_table(MSIDATABASE *db, LPCWSTR name, MSITABLE **table);
extern UINT get_table(MSIDATABASE *db, LPCWSTR name, MSITABLE **table);
extern UINT load_string_table( MSIDATABASE *db );
extern UINT MSI_CommitTables( MSIDATABASE *db );
extern HRESULT init_string_table( IStorage *stg );


/* string table functions */
extern BOOL msi_addstring( string_table *st, UINT string_no, const CHAR *data, int len, UINT refcount );
extern BOOL msi_addstringW( string_table *st, UINT string_no, const WCHAR *data, int len, UINT refcount );
extern UINT msi_id2stringW( string_table *st, UINT string_no, LPWSTR buffer, UINT *sz );
extern UINT msi_id2stringA( string_table *st, UINT string_no, LPSTR buffer, UINT *sz );

extern LPWSTR MSI_makestring( MSIDATABASE *db, UINT stringid);
extern UINT msi_string2idW( string_table *st, LPCWSTR buffer, UINT *id );
extern UINT msi_string2idA( string_table *st, LPCSTR str, UINT *id );
extern string_table *msi_init_stringtable( int entries, UINT codepage );
extern VOID msi_destroy_stringtable( string_table *st );
extern UINT msi_string_count( string_table *st );
extern UINT msi_id_refcount( string_table *st, UINT i );
extern UINT msi_string_totalsize( string_table *st, UINT *last );
extern UINT msi_strcmp( string_table *st, UINT lval, UINT rval, UINT *res );
extern const WCHAR *msi_string_lookup_id( string_table *st, UINT id );
extern UINT msi_string_get_codepage( string_table *st );


extern UINT VIEW_find_column( MSIVIEW *view, LPCWSTR name, UINT *n );

extern BOOL TABLE_Exists( MSIDATABASE *db, LPWSTR name );

extern UINT read_raw_stream_data( MSIDATABASE*, LPCWSTR stname,
                              USHORT **pdata, UINT *psz );

/* action internals */
extern UINT ACTION_DoTopLevelINSTALL( MSIPACKAGE *, LPCWSTR, LPCWSTR, LPCWSTR );
extern void ACTION_free_package_structures( MSIPACKAGE* );
extern UINT ACTION_DialogBox( MSIPACKAGE*, LPCWSTR);

/* record internals */
extern UINT MSI_RecordSetIStream( MSIRECORD *, unsigned int, IStream *);
extern UINT MSI_RecordGetIStream( MSIRECORD *, unsigned int, IStream **);
extern const WCHAR *MSI_RecordGetString( MSIRECORD *, unsigned int );
extern MSIRECORD *MSI_CreateRecord( unsigned int );
extern UINT MSI_RecordSetInteger( MSIRECORD *, unsigned int, int );
extern UINT MSI_RecordSetStringW( MSIRECORD *, unsigned int, LPCWSTR );
extern UINT MSI_RecordSetStringA( MSIRECORD *, unsigned int, LPCSTR );
extern BOOL MSI_RecordIsNull( MSIRECORD *, unsigned int );
extern UINT MSI_RecordGetStringW( MSIRECORD * , unsigned int, LPWSTR, DWORD *);
extern UINT MSI_RecordGetStringA( MSIRECORD *, unsigned int, LPSTR, DWORD *);
extern int MSI_RecordGetInteger( MSIRECORD *, unsigned int );
extern UINT MSI_RecordReadStream( MSIRECORD *, unsigned int, char *, DWORD *);
extern unsigned int MSI_RecordGetFieldCount( MSIRECORD *rec );
extern UINT MSI_RecordSetStreamW( MSIRECORD *, unsigned int, LPCWSTR );
extern UINT MSI_RecordSetStreamA( MSIRECORD *, unsigned int, LPCSTR );
extern UINT MSI_RecordDataSize( MSIRECORD *, unsigned int );
extern UINT MSI_RecordStreamToFile( MSIRECORD *, unsigned int, LPCWSTR );

/* stream internals */
extern UINT get_raw_stream( MSIHANDLE hdb, LPCWSTR stname, IStream **stm );
extern UINT db_get_raw_stream( MSIDATABASE *db, LPCWSTR stname, IStream **stm );
extern void enum_stream_names( IStorage *stg );

/* database internals */
extern UINT MSI_OpenDatabaseW( LPCWSTR, LPCWSTR, MSIDATABASE ** );
extern UINT MSI_DatabaseOpenViewW(MSIDATABASE *, LPCWSTR, MSIQUERY ** );
extern UINT MSI_OpenQuery( MSIDATABASE *, MSIQUERY **, LPCWSTR, ... );
typedef UINT (*record_func)( MSIRECORD *, LPVOID );
extern UINT MSI_IterateRecords( MSIQUERY *, DWORD *, record_func, LPVOID );
extern MSIRECORD *MSI_QueryGetRecord( MSIDATABASE *db, LPCWSTR query, ... );
extern UINT MSI_DatabaseImport( MSIDATABASE *, LPCWSTR, LPCWSTR );
extern UINT MSI_DatabaseExport( MSIDATABASE *, LPCWSTR, LPCWSTR, LPCWSTR );
extern UINT MSI_DatabaseGetPrimaryKeys( MSIDATABASE *, LPCWSTR, MSIRECORD ** );

/* view internals */
extern UINT MSI_ViewExecute( MSIQUERY*, MSIRECORD * );
extern UINT MSI_ViewFetch( MSIQUERY*, MSIRECORD ** );
extern UINT MSI_ViewClose( MSIQUERY* );

/* package internals */
extern MSIPACKAGE *MSI_CreatePackage( MSIDATABASE * );
extern UINT MSI_OpenPackageW( LPCWSTR szPackage, MSIPACKAGE ** );
extern UINT MSI_SetTargetPathW( MSIPACKAGE *, LPCWSTR, LPCWSTR );
extern UINT MSI_SetPropertyW( MSIPACKAGE *, LPCWSTR, LPCWSTR );
extern INT MSI_ProcessMessage( MSIPACKAGE *, INSTALLMESSAGE, MSIRECORD * );
extern UINT MSI_GetPropertyW( MSIPACKAGE *, LPCWSTR, LPWSTR, DWORD * );
extern UINT MSI_GetPropertyA(MSIPACKAGE *, LPCSTR, LPSTR, DWORD* );
extern MSICONDITION MSI_EvaluateConditionW( MSIPACKAGE *, LPCWSTR );
extern UINT MSI_GetComponentStateW( MSIPACKAGE *, LPWSTR, INSTALLSTATE *, INSTALLSTATE * );
extern UINT MSI_GetFeatureStateW( MSIPACKAGE *, LPWSTR, INSTALLSTATE *, INSTALLSTATE * );
extern UINT WINAPI MSI_SetFeatureStateW(MSIPACKAGE*, LPCWSTR, INSTALLSTATE );

/* for deformating */
extern UINT MSI_FormatRecordW( MSIPACKAGE *, MSIRECORD *, LPWSTR, DWORD * );
extern UINT MSI_FormatRecordA( MSIPACKAGE *, MSIRECORD *, LPSTR, DWORD * );
    
/* registry data encoding/decoding functions */
extern BOOL unsquash_guid(LPCWSTR in, LPWSTR out);
extern BOOL squash_guid(LPCWSTR in, LPWSTR out);
extern BOOL encode_base85_guid(GUID *,LPWSTR);
extern BOOL decode_base85_guid(LPCWSTR,GUID*);
extern UINT MSIREG_OpenUninstallKey(LPCWSTR szProduct, HKEY* key, BOOL create);
extern UINT MSIREG_OpenUserProductsKey(LPCWSTR szProduct, HKEY* key, BOOL create);
extern UINT MSIREG_OpenFeatures(HKEY* key);
extern UINT MSIREG_OpenFeaturesKey(LPCWSTR szProduct, HKEY* key, BOOL create);
extern UINT MSIREG_OpenComponents(HKEY* key);
extern UINT MSIREG_OpenUserComponentsKey(LPCWSTR szComponent, HKEY* key, BOOL create);
extern UINT MSIREG_OpenComponentsKey(LPCWSTR szComponent, HKEY* key, BOOL create);
extern UINT MSIREG_OpenProductsKey(LPCWSTR szProduct, HKEY* key, BOOL create);
extern UINT MSIREG_OpenUserFeaturesKey(LPCWSTR szProduct, HKEY* key, BOOL create);
extern UINT MSIREG_OpenUserComponentsKey(LPCWSTR szComponent, HKEY* key, BOOL create);
extern UINT MSIREG_OpenUpgradeCodesKey(LPCWSTR szProduct, HKEY* key, BOOL create);
extern UINT MSIREG_OpenUserUpgradeCodesKey(LPCWSTR szProduct, HKEY* key, BOOL create);

/* msi dialog interface */
typedef UINT (*msi_dialog_event_handler)( MSIPACKAGE*, LPCWSTR, LPCWSTR, msi_dialog* );
extern msi_dialog *msi_dialog_create( MSIPACKAGE*, LPCWSTR, msi_dialog_event_handler );
extern UINT msi_dialog_run_message_loop( msi_dialog* );
extern void msi_dialog_end_dialog( msi_dialog* );
extern void msi_dialog_check_messages( HANDLE );
extern void msi_dialog_do_preview( msi_dialog* );
extern void msi_dialog_destroy( msi_dialog* );
extern BOOL msi_dialog_register_class( void );
extern void msi_dialog_unregister_class( void );
extern void msi_dialog_handle_event( msi_dialog*, LPCWSTR, LPCWSTR, MSIRECORD * );

/* preview */
extern MSIPREVIEW *MSI_EnableUIPreview( MSIDATABASE * );
extern UINT MSI_PreviewDialogW( MSIPREVIEW *, LPCWSTR );

/* undocumented functions */
UINT WINAPI MsiCreateAndVerifyInstallerDirectory( DWORD );
UINT WINAPI MsiDecomposeDescriptorW( LPCWSTR, LPWSTR, LPWSTR, LPWSTR, DWORD * );
UINT WINAPI MsiDecomposeDescriptorA( LPCSTR, LPSTR, LPSTR, LPSTR, DWORD * );
LANGID WINAPI MsiLoadStringW( MSIHANDLE, UINT, LPWSTR, int, LANGID );
LANGID WINAPI MsiLoadStringA( MSIHANDLE, UINT, LPSTR, int, LANGID );

/* UI globals */
extern INSTALLUILEVEL gUILevel;
extern HWND gUIhwnd;
extern INSTALLUI_HANDLERA gUIHandlerA;
extern INSTALLUI_HANDLERW gUIHandlerW;
extern DWORD gUIFilter;
extern LPVOID gUIContext;
extern WCHAR gszLogFile[MAX_PATH];

inline static char *strdupWtoA( LPCWSTR str )
{
    LPSTR ret = NULL;
    if (str)
    {
        DWORD len = WideCharToMultiByte( CP_ACP, 0, str, -1, NULL, 0, NULL, NULL
);
        if ((ret = HeapAlloc( GetProcessHeap(), 0, len )))
            WideCharToMultiByte( CP_ACP, 0, str, -1, ret, len, NULL, NULL );
    }
    return ret;
}

inline static LPWSTR strdupAtoW( LPCSTR str )
{
    LPWSTR ret = NULL;
    if (str)
    {
        DWORD len = MultiByteToWideChar( CP_ACP, 0, str, -1, NULL, 0 );
        if ((ret = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) )))
            MultiByteToWideChar( CP_ACP, 0, str, -1, ret, len );
    }
    return ret;
}

inline static LPWSTR strdupW( LPCWSTR src )
{
    LPWSTR dest;
    if (!src) return NULL;
    dest = HeapAlloc(GetProcessHeap(), 0, (lstrlenW(src)+1)*sizeof(WCHAR));
    lstrcpyW(dest, src);
    return dest;
}

#endif /* __WINE_MSI_PRIVATE__ */
