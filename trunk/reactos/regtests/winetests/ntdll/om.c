/*
 * Unit test suite for object manager functions
 *
 * Copyright 2005 Robert Shearman
 * Copyright 2005 Vitaliy Margolen
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

#include "ntdll_test.h"
#include "winternl.h"
#include "stdio.h"
#include "winnt.h"
#include "stdlib.h"

static NTSTATUS (WINAPI *pRtlCreateUnicodeStringFromAsciiz)(PUNICODE_STRING, LPCSTR);
static VOID     (WINAPI *pRtlInitUnicodeString)( PUNICODE_STRING, LPCWSTR );
static VOID     (WINAPI *pRtlFreeUnicodeString)(PUNICODE_STRING);
static NTSTATUS (WINAPI *pNtCreateEvent) ( PHANDLE, ACCESS_MASK, const POBJECT_ATTRIBUTES, BOOLEAN, BOOLEAN);
static NTSTATUS (WINAPI *pNtCreateMutant)( PHANDLE, ACCESS_MASK, const POBJECT_ATTRIBUTES, BOOLEAN );
static NTSTATUS (WINAPI *pNtOpenMutant)  ( PHANDLE, ACCESS_MASK, const POBJECT_ATTRIBUTES );
static NTSTATUS (WINAPI *pNtCreateSemaphore)( PHANDLE, ACCESS_MASK,const POBJECT_ATTRIBUTES,LONG,LONG );
static NTSTATUS (WINAPI *pNtCreateTimer) ( PHANDLE, ACCESS_MASK, const POBJECT_ATTRIBUTES, TIMER_TYPE );
static NTSTATUS (WINAPI *pNtCreateSection)( PHANDLE, ACCESS_MASK, const POBJECT_ATTRIBUTES, const PLARGE_INTEGER,
                                            ULONG, ULONG, HANDLE );
static NTSTATUS (WINAPI *pNtOpenFile)    ( PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, PIO_STATUS_BLOCK, ULONG, ULONG );
static NTSTATUS (WINAPI *pNtClose)       ( HANDLE );
static NTSTATUS (WINAPI *pNtCreateNamedPipeFile)( PHANDLE, ULONG, POBJECT_ATTRIBUTES, PIO_STATUS_BLOCK,
                                       ULONG, ULONG, ULONG, ULONG, ULONG, ULONG, ULONG, ULONG, ULONG, PLARGE_INTEGER );
static NTSTATUS (WINAPI *pNtOpenDirectoryObject)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES);
static NTSTATUS (WINAPI *pNtCreateDirectoryObject)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES);
static NTSTATUS (WINAPI *pNtOpenSymbolicLinkObject)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES);
static NTSTATUS (WINAPI *pNtCreateSymbolicLinkObject)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, PUNICODE_STRING);


void test_case_sensitive (void)
{
    static const WCHAR buffer1[] = {'\\','B','a','s','e','N','a','m','e','d','O','b','j','e','c','t','s','\\','t','e','s','t',0};
    static const WCHAR buffer2[] = {'\\','B','a','s','e','N','a','m','e','d','O','b','j','e','c','t','s','\\','T','e','s','t',0};
    static const WCHAR buffer3[] = {'\\','B','a','s','e','N','a','m','e','d','O','b','j','e','c','t','s','\\','T','E','s','t',0};
    static const WCHAR buffer4[] = {'\\','B','A','S','E','N','a','m','e','d','O','b','j','e','c','t','s','\\','t','e','s','t',0};
    NTSTATUS status;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING str;
    HANDLE Event, Mutant, h;

    pRtlInitUnicodeString(&str, buffer1);
    InitializeObjectAttributes(&attr, &str, 0, 0, NULL);
    status = pNtCreateMutant(&Mutant, GENERIC_ALL, &attr, FALSE);
    ok(status == STATUS_SUCCESS, "Failed to create Mutant(%08lx)\n", status);

    status = pNtCreateEvent(&Event, GENERIC_ALL, &attr, FALSE, FALSE);
    ok(status == STATUS_OBJECT_NAME_COLLISION,
        "NtCreateEvent should have failed with STATUS_OBJECT_NAME_COLLISION got(%08lx)\n", status);

    pRtlInitUnicodeString(&str, buffer2);
    InitializeObjectAttributes(&attr, &str, 0, 0, NULL);
    status = pNtCreateEvent(&Event, GENERIC_ALL, &attr, FALSE, FALSE);
    ok(status == STATUS_SUCCESS, "Failed to create Event(%08lx)\n", status);

    pRtlInitUnicodeString(&str, buffer3);
    InitializeObjectAttributes(&attr, &str, OBJ_CASE_INSENSITIVE, 0, NULL);
    status = pNtOpenMutant(&h, GENERIC_ALL, &attr);
    ok(status == STATUS_OBJECT_TYPE_MISMATCH,
        "NtOpenMutant should have failed with STATUS_OBJECT_TYPE_MISMATCH got(%08lx)\n", status);

    pNtClose(Mutant);

    pRtlInitUnicodeString(&str, buffer4);
    InitializeObjectAttributes(&attr, &str, OBJ_CASE_INSENSITIVE, 0, NULL);
    status = pNtCreateMutant(&Mutant, GENERIC_ALL, &attr, FALSE);
    ok(status == STATUS_OBJECT_NAME_COLLISION,
        "NtCreateMutant should have failed with STATUS_OBJECT_NAME_COLLISION got(%08lx)\n", status);

    status = pNtCreateEvent(&h, GENERIC_ALL, &attr, FALSE, FALSE);
    ok(status == STATUS_OBJECT_NAME_COLLISION,
        "NtCreateEvent should have failed with STATUS_OBJECT_NAME_COLLISION got(%08lx)\n", status);

    attr.Attributes = 0;
    status = pNtCreateMutant(&Mutant, GENERIC_ALL, &attr, FALSE);
    todo_wine ok(status == STATUS_OBJECT_PATH_NOT_FOUND,
        "NtCreateMutant should have failed with STATUS_OBJECT_PATH_NOT_FOUND got(%08lx)\n", status);

    pNtClose(Event);
}

void test_namespace_pipe(void)
{
    static const WCHAR buffer1[] = {'\\','?','?','\\','P','I','P','E','\\','t','e','s','t','\\','p','i','p','e',0};
    static const WCHAR buffer2[] = {'\\','?','?','\\','P','I','P','E','\\','T','E','S','T','\\','P','I','P','E',0};
    static const WCHAR buffer3[] = {'\\','?','?','\\','p','i','p','e','\\','t','e','s','t','\\','p','i','p','e',0};
    static const WCHAR buffer4[] = {'\\','?','?','\\','p','i','p','e','\\','t','e','s','t',0};
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING str;
    IO_STATUS_BLOCK iosb;
    NTSTATUS status;
    LARGE_INTEGER timeout;
    HANDLE pipe, h;

    timeout.QuadPart = -10000;

    pRtlInitUnicodeString(&str, buffer1);
    InitializeObjectAttributes(&attr, &str, 0, 0, NULL);
    status = pNtCreateNamedPipeFile(&pipe, GENERIC_READ|GENERIC_WRITE, &attr, &iosb, FILE_SHARE_READ|FILE_SHARE_WRITE,
                                    FILE_CREATE, FILE_PIPE_FULL_DUPLEX, FALSE, FALSE, FALSE, 1, 256, 256, &timeout);
    ok(status == STATUS_SUCCESS, "Failed to create NamedPipe(%08lx)\n", status);

    status = pNtCreateNamedPipeFile(&pipe, GENERIC_READ|GENERIC_WRITE, &attr, &iosb, FILE_SHARE_READ|FILE_SHARE_WRITE,
                                    FILE_CREATE, FILE_PIPE_FULL_DUPLEX, FALSE, FALSE, FALSE, 1, 256, 256, &timeout);
    ok(status == STATUS_INSTANCE_NOT_AVAILABLE,
        "NtCreateNamedPipeFile should have failed with STATUS_INSTANCE_NOT_AVAILABLE got(%08lx)\n", status);

    pRtlInitUnicodeString(&str, buffer2);
    InitializeObjectAttributes(&attr, &str, 0, 0, NULL);
    status = pNtCreateNamedPipeFile(&pipe, GENERIC_READ|GENERIC_WRITE, &attr, &iosb, FILE_SHARE_READ|FILE_SHARE_WRITE,
                                    FILE_CREATE, FILE_PIPE_FULL_DUPLEX, FALSE, FALSE, FALSE, 1, 256, 256, &timeout);
    todo_wine ok(status == STATUS_INSTANCE_NOT_AVAILABLE,
        "NtCreateNamedPipeFile should have failed with STATUS_INSTANCE_NOT_AVAILABLE got(%08lx)\n", status);

    attr.Attributes = OBJ_CASE_INSENSITIVE;
    status = pNtOpenFile(&h, GENERIC_READ, &attr, &iosb, FILE_SHARE_READ|FILE_SHARE_WRITE, FILE_OPEN);
    ok(status == STATUS_SUCCESS, "Failed to open NamedPipe(%08lx)\n", status);
    pNtClose(h);

    pRtlInitUnicodeString(&str, buffer3);
    InitializeObjectAttributes(&attr, &str, 0, 0, NULL);
    status = pNtOpenFile(&h, GENERIC_READ, &attr, &iosb, FILE_SHARE_READ|FILE_SHARE_WRITE, FILE_OPEN);
    todo_wine ok(status == STATUS_OBJECT_PATH_NOT_FOUND || status == STATUS_PIPE_NOT_AVAILABLE,
        "pNtOpenFile should have failed with STATUS_OBJECT_PATH_NOT_FOUND got(%08lx)\n", status);

    pRtlInitUnicodeString(&str, buffer4);
    InitializeObjectAttributes(&attr, &str, OBJ_CASE_INSENSITIVE, 0, NULL);
    status = pNtOpenFile(&h, GENERIC_READ, &attr, &iosb, FILE_SHARE_READ|FILE_SHARE_WRITE, FILE_OPEN);
    ok(status == STATUS_OBJECT_NAME_NOT_FOUND,
        "pNtOpenFile should have failed with STATUS_OBJECT_NAME_NOT_FOUND got(%08lx)\n", status);

    pNtClose(pipe);
}

#define DIRECTORY_QUERY (0x0001)
#define SYMBOLIC_LINK_QUERY 0x0001

#define DIR_TEST_CREATE_FAILURE(h,e) \
    status = pNtCreateDirectoryObject(h, DIRECTORY_QUERY, &attr);\
    ok(status == e,"NtCreateDirectoryObject should have failed with %s got(%08lx)\n", #e, status);
#define DIR_TEST_OPEN_FAILURE(h,e) \
    status = pNtOpenDirectoryObject(h, DIRECTORY_QUERY, &attr);\
    ok(status == e,"NtOpenDirectoryObject should have failed with %s got(%08lx)\n", #e, status);
#define DIR_TEST_CREATE_OPEN_FAILURE(h,n,e) \
    pRtlCreateUnicodeStringFromAsciiz(&str, n);\
    DIR_TEST_CREATE_FAILURE(h,e) DIR_TEST_OPEN_FAILURE(h,e)\
    pRtlFreeUnicodeString(&str);

#define DIR_TEST_CREATE_SUCCESS(h) \
    status = pNtCreateDirectoryObject(h, DIRECTORY_QUERY, &attr); \
    ok(status == STATUS_SUCCESS, "Failed to create Directory(%08lx)\n", status);
#define DIR_TEST_OPEN_SUCCESS(h) \
    status = pNtOpenDirectoryObject(h, DIRECTORY_QUERY, &attr); \
    ok(status == STATUS_SUCCESS, "Failed to open Directory(%08lx)\n", status);
#define DIR_TEST_CREATE_OPEN_SUCCESS(h,n) \
    pRtlCreateUnicodeStringFromAsciiz(&str, n);\
    DIR_TEST_CREATE_SUCCESS(h) pNtClose(h); DIR_TEST_OPEN_SUCCESS(h) pNtClose(h); \
    pRtlFreeUnicodeString(&str);

static void test_name_collisions(void)
{
    NTSTATUS status;
    UNICODE_STRING str;
    OBJECT_ATTRIBUTES attr;
    HANDLE dir, h, h1, h2;
    DWORD winerr;
    LARGE_INTEGER size;

    InitializeObjectAttributes(&attr, &str, 0, 0, NULL);
    pRtlCreateUnicodeStringFromAsciiz(&str, "\\");
    h = 0;
    todo_wine{ DIR_TEST_CREATE_FAILURE(&h, STATUS_OBJECT_NAME_COLLISION) }
    ok(h == 0, "Failed create returned valid handle! (%p)\n", h);
    InitializeObjectAttributes(&attr, &str, OBJ_OPENIF, 0, NULL);

    todo_wine{ DIR_TEST_CREATE_FAILURE(&h, STATUS_OBJECT_NAME_EXISTS) }
    pNtClose(h);
    status = pNtCreateMutant(&h, GENERIC_ALL, &attr, FALSE);
    todo_wine ok(status == STATUS_OBJECT_TYPE_MISMATCH,
        "NtCreateMutant should have failed with STATUS_OBJECT_TYPE_MISMATCH got(%08lx)\n", status);
    pRtlFreeUnicodeString(&str);

    pRtlCreateUnicodeStringFromAsciiz(&str, "\\??\\PIPE\\om.c-mutant");
    status = pNtCreateMutant(&h, GENERIC_ALL, &attr, FALSE);
    todo_wine ok(status == STATUS_OBJECT_TYPE_MISMATCH,
        "NtCreateMutant should have failed with STATUS_OBJECT_TYPE_MISMATCH got(%08lx)\n", status);
    pRtlFreeUnicodeString(&str);


    pRtlCreateUnicodeStringFromAsciiz(&str, "\\BaseNamedObjects");
    DIR_TEST_OPEN_SUCCESS(&dir)
    pRtlCreateUnicodeStringFromAsciiz(&str, "om.c-test");
    InitializeObjectAttributes(&attr, &str, OBJ_OPENIF, dir, NULL);
    
    h = CreateMutexA(NULL, FALSE, "om.c-test");
    ok(h != 0, "CreateMutexA failed got ret=%p (%ld)\n", h, GetLastError());
    status = pNtCreateMutant(&h1, GENERIC_ALL, &attr, FALSE);
    ok(status == STATUS_OBJECT_NAME_EXISTS && h1 != NULL,
        "NtCreateMutant should have succeeded with STATUS_OBJECT_NAME_EXISTS got(%08lx)\n", status);
    h2 = CreateMutexA(NULL, FALSE, "om.c-test");
    winerr = GetLastError();
    ok(h2 != 0 && winerr == ERROR_ALREADY_EXISTS,
        "CreateMutexA should have succeeded with ERROR_ALREADY_EXISTS got ret=%p (%ld)\n", h2, winerr);
    pNtClose(h);
    pNtClose(h1);
    pNtClose(h2);

    h = CreateEventA(NULL, FALSE, FALSE, "om.c-test");
    ok(h != 0, "CreateEventA failed got ret=%p (%ld)\n", h, GetLastError());
    status = pNtCreateEvent(&h1, GENERIC_ALL, &attr, FALSE, FALSE);
    ok(status == STATUS_OBJECT_NAME_EXISTS && h1 != NULL,
        "NtCreateEvent should have succeeded with STATUS_OBJECT_NAME_EXISTS got(%08lx)\n", status);
    h2 = CreateEventA(NULL, FALSE, FALSE, "om.c-test");
    winerr = GetLastError();
    ok(h2 != 0 && winerr == ERROR_ALREADY_EXISTS,
        "CreateEventA should have succeeded with ERROR_ALREADY_EXISTS got ret=%p (%ld)\n", h2, winerr);
    pNtClose(h);
    pNtClose(h1);
    pNtClose(h2);

    h = CreateSemaphoreA(NULL, 1, 2, "om.c-test");
    ok(h != 0, "CreateSemaphoreA failed got ret=%p (%ld)\n", h, GetLastError());
    status = pNtCreateSemaphore(&h1, GENERIC_ALL, &attr, 1, 2);
    ok(status == STATUS_OBJECT_NAME_EXISTS && h1 != NULL,
        "NtCreateSemaphore should have succeeded with STATUS_OBJECT_NAME_EXISTS got(%08lx)\n", status);
    h2 = CreateSemaphoreA(NULL, 1, 2, "om.c-test");
    winerr = GetLastError();
    ok(h2 != 0 && winerr == ERROR_ALREADY_EXISTS,
        "CreateSemaphoreA should have succeeded with ERROR_ALREADY_EXISTS got ret=%p (%ld)\n", h2, winerr);
    pNtClose(h);
    pNtClose(h1);
    pNtClose(h2);
    
    h = CreateWaitableTimerA(NULL, TRUE, "om.c-test");
    ok(h != 0, "CreateWaitableTimerA failed got ret=%p (%ld)\n", h, GetLastError());
    status = pNtCreateTimer(&h1, GENERIC_ALL, &attr, NotificationTimer);
    ok(status == STATUS_OBJECT_NAME_EXISTS && h1 != NULL,
        "NtCreateTimer should have succeeded with STATUS_OBJECT_NAME_EXISTS got(%08lx)\n", status);
    h2 = CreateWaitableTimerA(NULL, TRUE, "om.c-test");
    winerr = GetLastError();
    ok(h2 != 0 && winerr == ERROR_ALREADY_EXISTS,
        "CreateWaitableTimerA should have succeeded with ERROR_ALREADY_EXISTS got ret=%p (%ld)\n", h2, winerr);
    pNtClose(h);
    pNtClose(h1);
    pNtClose(h2);

    h = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 256, "om.c-test");
    ok(h != 0, "CreateFileMappingA failed got ret=%p (%ld)\n", h, GetLastError());
    size.u.LowPart = 256;
    size.u.HighPart = 0;
    status = pNtCreateSection(&h1, SECTION_MAP_WRITE, &attr, &size, PAGE_READWRITE, SEC_COMMIT, 0);
    ok(status == STATUS_OBJECT_NAME_EXISTS && h1 != NULL,
        "NtCreateSection should have succeeded with STATUS_OBJECT_NAME_EXISTS got(%08lx)\n", status);
    h2 = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 256, "om.c-test");
    winerr = GetLastError();
    ok(h2 != 0 && winerr == ERROR_ALREADY_EXISTS,
        "CreateFileMappingA should have succeeded with ERROR_ALREADY_EXISTS got ret=%p (%ld)\n", h2, winerr);
    pNtClose(h);
    pNtClose(h1);
    pNtClose(h2);

    pRtlFreeUnicodeString(&str);
    pNtClose(dir);
}

void test_directory(void)
{
    NTSTATUS status;
    UNICODE_STRING str;
    OBJECT_ATTRIBUTES attr;
    HANDLE dir, dir1, h;

    /* No name and/or no attributes */
    status = pNtCreateDirectoryObject(NULL, DIRECTORY_QUERY, &attr);
    todo_wine ok(status == STATUS_ACCESS_VIOLATION,
        "NtCreateDirectoryObject should have failed with STATUS_ACCESS_VIOLATION got(%08lx)\n", status);
    status = pNtOpenDirectoryObject(NULL, DIRECTORY_QUERY, &attr);
    todo_wine ok(status == STATUS_ACCESS_VIOLATION,
        "NtOpenDirectoryObject should have failed with STATUS_ACCESS_VIOLATION got(%08lx)\n", status);

    status = pNtCreateDirectoryObject(&h, DIRECTORY_QUERY, NULL);
    ok(status == STATUS_SUCCESS, "Failed to create Directory without attributes(%08lx)\n", status);
    pNtClose(h);
    status = pNtOpenDirectoryObject(&h, DIRECTORY_QUERY, NULL);
    todo_wine ok(status == STATUS_INVALID_PARAMETER,
        "NtOpenDirectoryObject should have failed with STATUS_INVALID_PARAMETER got(%08lx)\n", status);

    InitializeObjectAttributes(&attr, NULL, 0, 0, NULL);
    DIR_TEST_CREATE_SUCCESS(&dir)
    todo_wine{ DIR_TEST_OPEN_FAILURE(&h, STATUS_OBJECT_PATH_SYNTAX_BAD) }

    /* Bad name */
    InitializeObjectAttributes(&attr, &str, 0, 0, NULL);

    pRtlCreateUnicodeStringFromAsciiz(&str, "");
    DIR_TEST_CREATE_SUCCESS(&h)
    pNtClose(h);
    todo_wine{ DIR_TEST_OPEN_FAILURE(&h, STATUS_OBJECT_PATH_SYNTAX_BAD) }
    pRtlFreeUnicodeString(&str);
    pNtClose(dir);

    todo_wine{ DIR_TEST_CREATE_OPEN_FAILURE(&h, "BaseNamedObjects", STATUS_OBJECT_PATH_SYNTAX_BAD) }
    todo_wine{ DIR_TEST_CREATE_OPEN_FAILURE(&h, "\\BaseNamedObjects\\", STATUS_OBJECT_NAME_INVALID) }
    todo_wine{ DIR_TEST_CREATE_OPEN_FAILURE(&h, "\\\\BaseNamedObjects", STATUS_OBJECT_NAME_INVALID) }
    todo_wine{ DIR_TEST_CREATE_OPEN_FAILURE(&h, "\\BaseNamedObjects\\\\om.c-test", STATUS_OBJECT_NAME_INVALID) }
    todo_wine{ DIR_TEST_CREATE_OPEN_FAILURE(&h, "\\BaseNamedObjects\\om.c-test\\", STATUS_OBJECT_PATH_NOT_FOUND) }

    pRtlCreateUnicodeStringFromAsciiz(&str, "\\BaseNamedObjects\\om.c-test");
    DIR_TEST_CREATE_SUCCESS(&h)
    DIR_TEST_OPEN_SUCCESS(&dir1)
    pRtlFreeUnicodeString(&str);
    pNtClose(h);
    pNtClose(dir1);


    /* Use of root directory */

    /* Can't use symlinks as a directory */
    pRtlCreateUnicodeStringFromAsciiz(&str, "\\BaseNamedObjects\\Local");
    InitializeObjectAttributes(&attr, &str, 0, 0, NULL);
    status = pNtOpenSymbolicLinkObject(&dir, SYMBOLIC_LINK_QUERY, &attr);\
    todo_wine ok(status == STATUS_SUCCESS, "Failed to open SymbolicLink(%08lx)\n", status);
    pRtlFreeUnicodeString(&str);
    InitializeObjectAttributes(&attr, &str, 0, dir, NULL);
    pRtlCreateUnicodeStringFromAsciiz(&str, "one more level");
    todo_wine{ DIR_TEST_CREATE_FAILURE(&h, STATUS_OBJECT_TYPE_MISMATCH) }
    pRtlFreeUnicodeString(&str);
    pNtClose(h);
    pNtClose(dir);

    pRtlCreateUnicodeStringFromAsciiz(&str, "\\BaseNamedObjects");
    InitializeObjectAttributes(&attr, &str, 0, 0, NULL);
    DIR_TEST_OPEN_SUCCESS(&dir)
    pRtlFreeUnicodeString(&str);

    InitializeObjectAttributes(&attr, NULL, 0, dir, NULL);
    todo_wine{ DIR_TEST_OPEN_FAILURE(&h, STATUS_OBJECT_NAME_INVALID) }

    InitializeObjectAttributes(&attr, &str, 0, dir, NULL);
    DIR_TEST_CREATE_OPEN_SUCCESS(&h, "")
    todo_wine{ DIR_TEST_CREATE_OPEN_FAILURE(&h, "\\", STATUS_OBJECT_PATH_SYNTAX_BAD) }
    todo_wine{ DIR_TEST_CREATE_OPEN_FAILURE(&h, "\\om.c-test", STATUS_OBJECT_PATH_SYNTAX_BAD) }
    todo_wine{ DIR_TEST_CREATE_OPEN_FAILURE(&h, "\\om.c-test\\", STATUS_OBJECT_PATH_SYNTAX_BAD) }
    todo_wine{ DIR_TEST_CREATE_OPEN_FAILURE(&h, "om.c-test\\", STATUS_OBJECT_PATH_NOT_FOUND) }

    pRtlCreateUnicodeStringFromAsciiz(&str, "om.c-test");
    DIR_TEST_CREATE_SUCCESS(&dir1)
    DIR_TEST_OPEN_SUCCESS(&h)
    pRtlFreeUnicodeString(&str);

    pNtClose(h);
    pNtClose(dir1);
    pNtClose(dir);

    /* Nested directories */
    pRtlCreateUnicodeStringFromAsciiz(&str, "\\");
    InitializeObjectAttributes(&attr, &str, 0, 0, NULL);
    DIR_TEST_OPEN_SUCCESS(&dir)
    InitializeObjectAttributes(&attr, &str, 0, dir, NULL);
    todo_wine{ DIR_TEST_OPEN_FAILURE(&h, STATUS_OBJECT_PATH_SYNTAX_BAD) }
    pRtlFreeUnicodeString(&str);
    pNtClose(dir);

    InitializeObjectAttributes(&attr, &str, 0, 0, NULL);
    pRtlCreateUnicodeStringFromAsciiz(&str, "\\BaseNamedObjects\\om.c-test");
    DIR_TEST_CREATE_SUCCESS(&dir)
    pRtlFreeUnicodeString(&str);
    pRtlCreateUnicodeStringFromAsciiz(&str, "\\BaseNamedObjects\\om.c-test\\one more level");
    DIR_TEST_CREATE_SUCCESS(&h)
    pRtlFreeUnicodeString(&str);
    pNtClose(h);
    InitializeObjectAttributes(&attr, &str, 0, dir, NULL);
    pRtlCreateUnicodeStringFromAsciiz(&str, "one more level");
    DIR_TEST_CREATE_SUCCESS(&h)
    pRtlFreeUnicodeString(&str);
    pNtClose(h);

    pNtClose(dir);

    InitializeObjectAttributes(&attr, &str, 0, 0, NULL);
    pRtlCreateUnicodeStringFromAsciiz(&str, "\\BaseNamedObjects\\Global\\om.c-test");
    DIR_TEST_CREATE_SUCCESS(&dir)
    pRtlFreeUnicodeString(&str);
    pRtlCreateUnicodeStringFromAsciiz(&str, "\\BaseNamedObjects\\Local\\om.c-test\\one more level");
    DIR_TEST_CREATE_SUCCESS(&h)
    pRtlFreeUnicodeString(&str);
    pNtClose(h);
    InitializeObjectAttributes(&attr, &str, 0, dir, NULL);
    pRtlCreateUnicodeStringFromAsciiz(&str, "one more level");
    DIR_TEST_CREATE_SUCCESS(&dir)
    pRtlFreeUnicodeString(&str);
    pNtClose(h);

    pNtClose(dir);


    /* Create other objects using RootDirectory */

    InitializeObjectAttributes(&attr, &str, 0, 0, NULL);
    pRtlCreateUnicodeStringFromAsciiz(&str, "\\BaseNamedObjects");
    DIR_TEST_OPEN_SUCCESS(&dir)
    pRtlFreeUnicodeString(&str);
    InitializeObjectAttributes(&attr, &str, 0, dir, NULL);

    /* Test inavalid paths */
    pRtlCreateUnicodeStringFromAsciiz(&str, "\\om.c-mutant");
    status = pNtCreateMutant(&h, GENERIC_ALL, &attr, FALSE);
    todo_wine ok(status == STATUS_OBJECT_PATH_SYNTAX_BAD,
        "NtCreateMutant should have failed with STATUS_OBJECT_PATH_SYNTAX_BAD got(%08lx)\n", status);
    pRtlFreeUnicodeString(&str);
    pRtlCreateUnicodeStringFromAsciiz(&str, "\\om.c-mutant\\");
    status = pNtCreateMutant(&h, GENERIC_ALL, &attr, FALSE);
    todo_wine ok(status == STATUS_OBJECT_PATH_SYNTAX_BAD,
        "NtCreateMutant should have failed with STATUS_OBJECT_PATH_SYNTAX_BAD got(%08lx)\n", status);
    pRtlFreeUnicodeString(&str);

    pRtlCreateUnicodeStringFromAsciiz(&str, "om.c\\-mutant");
    status = pNtCreateMutant(&h, GENERIC_ALL, &attr, FALSE);
    todo_wine ok(status == STATUS_OBJECT_PATH_NOT_FOUND,
        "NtCreateMutant should have failed with STATUS_OBJECT_PATH_NOT_FOUND got(%08lx)\n", status);
    pRtlFreeUnicodeString(&str);

    pRtlCreateUnicodeStringFromAsciiz(&str, "om.c-mutant");
    status = pNtCreateMutant(&h, GENERIC_ALL, &attr, FALSE);
    ok(status == STATUS_SUCCESS, "Failed to create Mutant(%08lx)\n", status);
    pRtlFreeUnicodeString(&str);
    pNtClose(h);

    pNtClose(dir);
}

#define SYMLNK_TEST_CREATE_FAILURE(h,e) \
    status = pNtCreateSymbolicLinkObject(h, SYMBOLIC_LINK_QUERY, &attr, &target);\
    ok(status == e,"NtCreateSymbolicLinkObject should have failed with %s got(%08lx)\n", #e, status);
#define SYMLNK_TEST_OPEN_FAILURE(h,e) \
    status = pNtOpenSymbolicLinkObject(h, SYMBOLIC_LINK_QUERY, &attr);\
    ok(status == e,"NtOpenSymbolicLinkObject should have failed with %s got(%08lx)\n", #e, status);
#define SYMLNK_TEST_CREATE_OPEN_FAILURE(h,n,t,e) \
    pRtlCreateUnicodeStringFromAsciiz(&str, n);\
    pRtlCreateUnicodeStringFromAsciiz(&target, t);\
    SYMLNK_TEST_CREATE_FAILURE(h,e)\
    SYMLNK_TEST_OPEN_FAILURE(h,e)\
    pRtlFreeUnicodeString(&target);\
    pRtlFreeUnicodeString(&str);

#define SYMLNK_TEST_CREATE_SUCCESS(h) \
    status = pNtCreateSymbolicLinkObject(h, SYMBOLIC_LINK_QUERY, &attr, &target); \
    ok(status == STATUS_SUCCESS, "Failed to create SymbolicLink(%08lx)\n", status);
#define SYMLNK_TEST_OPEN_SUCCESS(h) \
    status = pNtOpenSymbolicLinkObject(h, SYMBOLIC_LINK_QUERY, &attr); \
    ok(status == STATUS_SUCCESS, "Failed to open SymbolicLink(%08lx)\n", status);

void test_symboliclink(void)
{
    NTSTATUS status;
    UNICODE_STRING str, target;
    OBJECT_ATTRIBUTES attr;
    HANDLE dir, link, h;
    IO_STATUS_BLOCK iosb;

    /* No name and/or no attributes */
    todo_wine{ SYMLNK_TEST_CREATE_OPEN_FAILURE(NULL, "", "", STATUS_ACCESS_VIOLATION) }

    status = pNtCreateSymbolicLinkObject(&h, SYMBOLIC_LINK_QUERY, NULL, NULL);
    todo_wine ok(status == STATUS_ACCESS_VIOLATION,
        "NtCreateSymbolicLinkObject should have failed with STATUS_ACCESS_VIOLATION got(%08lx)\n", status);
    status = pNtOpenSymbolicLinkObject(&h, SYMBOLIC_LINK_QUERY, NULL);
    todo_wine ok(status == STATUS_INVALID_PARAMETER,
        "NtOpenSymbolicLinkObject should have failed with STATUS_INVALID_PARAMETER got(%08lx)\n", status);

    InitializeObjectAttributes(&attr, NULL, 0, 0, NULL);
    todo_wine{ SYMLNK_TEST_CREATE_FAILURE(&link, STATUS_INVALID_PARAMETER) }
    todo_wine{ SYMLNK_TEST_OPEN_FAILURE(&h, STATUS_OBJECT_PATH_SYNTAX_BAD) }

    /* Bad name */
    pRtlCreateUnicodeStringFromAsciiz(&target, "anywhere");
    InitializeObjectAttributes(&attr, &str, 0, 0, NULL);

    pRtlCreateUnicodeStringFromAsciiz(&str, "");
    SYMLNK_TEST_CREATE_SUCCESS(&link)
    todo_wine{ SYMLNK_TEST_OPEN_FAILURE(&h, STATUS_OBJECT_PATH_SYNTAX_BAD) }
    pNtClose(link);
    pRtlFreeUnicodeString(&str);

    pRtlCreateUnicodeStringFromAsciiz(&str, "\\");
    todo_wine {SYMLNK_TEST_CREATE_FAILURE(&h, STATUS_OBJECT_TYPE_MISMATCH)}
    pRtlFreeUnicodeString(&str);
    pRtlFreeUnicodeString(&target);

    todo_wine{ SYMLNK_TEST_CREATE_OPEN_FAILURE(&h, "BaseNamedObjects", "->Somewhere", STATUS_OBJECT_PATH_SYNTAX_BAD) }
    todo_wine{ SYMLNK_TEST_CREATE_OPEN_FAILURE(&h, "\\BaseNamedObjects\\", "->Somewhere", STATUS_OBJECT_NAME_INVALID) }
    todo_wine{ SYMLNK_TEST_CREATE_OPEN_FAILURE(&h, "\\\\BaseNamedObjects", "->Somewhere", STATUS_OBJECT_NAME_INVALID) }
    todo_wine{ SYMLNK_TEST_CREATE_OPEN_FAILURE(&h, "\\BaseNamedObjects\\\\om.c-test", "->Somewhere", STATUS_OBJECT_NAME_INVALID) }
    todo_wine{ SYMLNK_TEST_CREATE_OPEN_FAILURE(&h, "\\BaseNamedObjects\\om.c-test\\", "->Somewhere", STATUS_OBJECT_NAME_INVALID) }


    /* Compaund test */
    pRtlCreateUnicodeStringFromAsciiz(&str, "\\BaseNamedObjects");
    DIR_TEST_OPEN_SUCCESS(&dir)
    pRtlFreeUnicodeString(&str);

    InitializeObjectAttributes(&attr, &str, 0, dir, NULL);
    pRtlCreateUnicodeStringFromAsciiz(&str, "Local\\test-link");
    pRtlCreateUnicodeStringFromAsciiz(&target, "\\DosDevices");
    SYMLNK_TEST_CREATE_SUCCESS(&link)
    pRtlFreeUnicodeString(&str);
    pRtlFreeUnicodeString(&target);

    pRtlCreateUnicodeStringFromAsciiz(&str, "Local\\test-link\\PIPE");
    status = pNtOpenFile(&h, GENERIC_READ, &attr, &iosb, FILE_SHARE_READ|FILE_SHARE_WRITE, FILE_OPEN);
    todo_wine ok(status == STATUS_SUCCESS, "Failed to open NamedPipe(%08lx)\n", status);
    pRtlFreeUnicodeString(&str);

    pNtClose(h);
    pNtClose(link);
    pNtClose(dir);
}

START_TEST(om)
{
    HMODULE hntdll = GetModuleHandleA("ntdll.dll");
    if (hntdll)
    {
        pRtlCreateUnicodeStringFromAsciiz = (void *)GetProcAddress(hntdll, "RtlCreateUnicodeStringFromAsciiz");
        pRtlFreeUnicodeString   = (void *)GetProcAddress(hntdll, "RtlFreeUnicodeString");
        pNtCreateEvent          = (void *)GetProcAddress(hntdll, "NtCreateEvent");
        pNtCreateMutant         = (void *)GetProcAddress(hntdll, "NtCreateMutant");
        pNtOpenMutant           = (void *)GetProcAddress(hntdll, "NtOpenMutant");
        pNtOpenFile             = (void *)GetProcAddress(hntdll, "NtOpenFile");
        pNtClose                = (void *)GetProcAddress(hntdll, "NtClose");
        pRtlInitUnicodeString   = (void *)GetProcAddress(hntdll, "RtlInitUnicodeString");
        pNtCreateNamedPipeFile  = (void *)GetProcAddress(hntdll, "NtCreateNamedPipeFile");
        pNtOpenDirectoryObject  = (void *)GetProcAddress(hntdll, "NtOpenDirectoryObject");
        pNtCreateDirectoryObject= (void *)GetProcAddress(hntdll, "NtCreateDirectoryObject");
        pNtOpenSymbolicLinkObject = (void *)GetProcAddress(hntdll, "NtOpenSymbolicLinkObject");
        pNtCreateSymbolicLinkObject = (void *)GetProcAddress(hntdll, "NtCreateSymbolicLinkObject");
        pNtCreateSemaphore      =  (void *)GetProcAddress(hntdll, "NtCreateSemaphore");
        pNtCreateTimer          =  (void *)GetProcAddress(hntdll, "NtCreateTimer");
        pNtCreateSection        =  (void *)GetProcAddress(hntdll, "NtCreateSection");

        test_case_sensitive();
        test_namespace_pipe();
        test_name_collisions();
        test_directory();
        test_symboliclink();
    }
}
