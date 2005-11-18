/* Unit test suite for Ntdll atom API functions
 *
 * Copyright 2003 Gyorgy 'Nog' Jeney
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
 *
 * NOTES
 * We use function pointers here as there is no import library for NTDLL on
 * windows.
 */

#define _WIN32_WINNT 0x0501

#include <stdio.h>
#include <stdarg.h>
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windows.h"
#include "wine/test.h"
#include "wine/unicode.h"
#include "winternl.h"

/* Function pointers for ntdll calls */
static HMODULE hntdll = 0;
static NTSTATUS (WINAPI *pRtlCreateAtomTable)(ULONG,PRTL_ATOM_TABLE);
static NTSTATUS (WINAPI *pRtlDestroyAtomTable)(RTL_ATOM_TABLE);
static NTSTATUS (WINAPI *pRtlEmptyAtomTable)(RTL_ATOM_TABLE,BOOLEAN);
static NTSTATUS (WINAPI *pRtlAddAtomToAtomTable)(RTL_ATOM_TABLE,PCWSTR,PRTL_ATOM);
static NTSTATUS (WINAPI *pRtlDeleteAtomFromAtomTable)(RTL_ATOM_TABLE,RTL_ATOM);
static NTSTATUS (WINAPI *pRtlLookupAtomInAtomTable)(RTL_ATOM_TABLE,PCWSTR,PRTL_ATOM);
static NTSTATUS (WINAPI *pRtlPinAtomInAtomTable)(RTL_ATOM_TABLE,RTL_ATOM);
static NTSTATUS (WINAPI *pRtlQueryAtomInAtomTable)(RTL_ATOM_TABLE,RTL_ATOM,PULONG,PULONG,PWSTR,PULONG);

static const WCHAR EmptyAtom[] = {0};
static const WCHAR testAtom1[] = {'H','e','l','l','o',' ','W','o','r','l','d',0};
static const WCHAR testAtom2[] = {'H','e','l','l','o',' ','W','o','r','l','d','2',0};
static const WCHAR testAtom3[] = {'H','e','l','l','o',' ','W','o','r','l','d','3',0};

static const WCHAR testAtom1Cap[] = {'H','E','L','L','O',' ','W','O','R','L','D',0};
static const WCHAR testAtom1Low[] = {'h','e','l','l','o',' ','w','o','r','l','d',0};

static const WCHAR testAtomInt[] = {'#','1','3','2',0};
static const WCHAR testAtomIntInv[] = {'#','2','3','4','z',0};
static const WCHAR testAtomOTT[] = {'#','1','2','3',0};

static void InitFunctionPtr(void)
{
    hntdll = LoadLibraryA("ntdll.dll");
    ok(hntdll != 0, "Unable to load ntdll.dll\n");

    if (hntdll)
    {
        pRtlCreateAtomTable = (void *)GetProcAddress(hntdll, "RtlCreateAtomTable");
        pRtlDestroyAtomTable = (void *)GetProcAddress(hntdll, "RtlDestroyAtomTable");
        pRtlEmptyAtomTable = (void *)GetProcAddress(hntdll, "RtlEmptyAtomTable");
        pRtlAddAtomToAtomTable = (void *)GetProcAddress(hntdll, "RtlAddAtomToAtomTable");
        pRtlDeleteAtomFromAtomTable = (void *)GetProcAddress(hntdll, "RtlDeleteAtomFromAtomTable");
        pRtlLookupAtomInAtomTable = (void *)GetProcAddress(hntdll, "RtlLookupAtomInAtomTable");
        pRtlPinAtomInAtomTable = (void *)GetProcAddress(hntdll, "RtlPinAtomInAtomTable");
        pRtlQueryAtomInAtomTable = (void *)GetProcAddress(hntdll, "RtlQueryAtomInAtomTable");
    }
}

static DWORD RtlAtomTestThread(LPVOID Table)
{
    RTL_ATOM_TABLE AtomTable = *(PRTL_ATOM_TABLE)Table;
    RTL_ATOM Atom;
    NTSTATUS res;
    ULONG RefCount = 0, PinCount = 0, Len = 0;
    WCHAR Name[64];

    res = pRtlLookupAtomInAtomTable(AtomTable, testAtom1, &Atom);
    ok(!res, "Unable to find atom from another thread, retval: %lx\n", res);

    res = pRtlLookupAtomInAtomTable(AtomTable, testAtom2, &Atom);
    ok(!res, "Unable to lookup pinned atom in table, retval: %lx\n", res);

    res = pRtlQueryAtomInAtomTable(AtomTable, Atom, &RefCount, &PinCount, Name, &Len);
    ok(res == STATUS_BUFFER_TOO_SMALL, "We got wrong retval: %lx\n", res);

    Len = 64;
    res = pRtlQueryAtomInAtomTable(AtomTable, Atom, &RefCount, &PinCount, Name, &Len);
    ok(!res, "Failed with longenough buffer, retval: %lx\n", res);
    ok(RefCount == 1, "Refcount was not 1 but %lx\n", RefCount);
    ok(PinCount == 1, "Pincount was not 1 but %lx\n", PinCount);
    ok(!strcmpW(Name, testAtom2), "We found wrong atom!!\n");
    ok((strlenW(testAtom2) * sizeof(WCHAR)) == Len, "Returned wrong length %ld\n", Len);

    Len = 64;
    res = pRtlQueryAtomInAtomTable(AtomTable, Atom, NULL, NULL, Name, &Len);
    ok(!res, "RtlQueryAtomInAtomTable with optional args invalid failed, retval: %lx\n", res);
    ok(!strcmpW(Name, testAtom2), "Found Wrong atom!\n");
    ok((strlenW(testAtom2) * sizeof(WCHAR)) == Len, "Returned wrong length %ld\n", Len);

    res = pRtlPinAtomInAtomTable(AtomTable, Atom);
    ok(!res, "Unable to pin atom in atom table, retval: %lx\n", res);

    return 0;
}

static void test_NtAtom(void)
{
    RTL_ATOM_TABLE AtomTable = NULL;
    NTSTATUS res;
    RTL_ATOM Atom1, Atom2, Atom3, testEAtom, testAtom;
    HANDLE testThread;
    ULONG RefCount = 0, PinCount = 0, Len = 0;
    WCHAR Name[64];

    /* If we pass a non-null string to create atom table, then it thinks that we
     * have passed it an already allocated atom table */
    res = pRtlCreateAtomTable(0, &AtomTable);
    ok(!res, "RtlCreateAtomTable should succeed with an atom table size of 0\n");

    if (!res)
    {
        res = pRtlDestroyAtomTable(AtomTable);
        ok(!res, "We could create the atom table, but we couldn't destroy it! retval: %lx\n", res);
    }

    AtomTable = NULL;
    res = pRtlCreateAtomTable(37, &AtomTable);
    ok(!res, "We're unable to create an atom table with a valid table size retval: %lx\n", res);
    if (!res)
    {
        res = pRtlAddAtomToAtomTable(AtomTable, testAtom1, &Atom1);
        ok(!res, "We were unable to add a simple atom to the atom table, retval: %lx\n", res);

        res = pRtlLookupAtomInAtomTable(AtomTable, testAtom1Cap, &testAtom);
        ok(!res, "We were unable to find capital version of the atom, retval: %lx\n", res);
        ok(Atom1 == testAtom, "Found wrong atom in table when querying capital atom\n");

        res = pRtlLookupAtomInAtomTable(AtomTable, testAtom1Low, &testAtom);
        ok(!res, "Unable to find lowercase version of the atom, retval: %lx\n", res);
        ok(testAtom == Atom1, "Found wrong atom when querying lowercase atom\n");

        res = pRtlAddAtomToAtomTable(AtomTable, EmptyAtom, &testEAtom);
        ok(res == STATUS_OBJECT_NAME_INVALID, "Got wrong retval, retval: %lx\n", res);

        res = pRtlLookupAtomInAtomTable(AtomTable, testAtom1, &testAtom);
        ok(!res, "Failed to find totally legitimate atom, retval: %lx\n", res);
        ok(testAtom == Atom1, "Found wrong atom!\n");

        res = pRtlAddAtomToAtomTable(AtomTable, testAtom2, &Atom2);
        ok(!res, "Unable to add other legitimate atom to table, retval: %lx\n", res);

        res = pRtlPinAtomInAtomTable(AtomTable, Atom2);
        ok(!res, "Unable to pin atom in atom table, retval: %lx\n", res);

        testThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)RtlAtomTestThread, &AtomTable, 0, NULL);
        WaitForSingleObject(testThread, INFINITE);

        Len = 64;
        res = pRtlQueryAtomInAtomTable(AtomTable, Atom2, &RefCount, &PinCount, Name, &Len);
        ok(!res, "Unable to query atom in atom table, retval: %lx\n", res);
        ok(RefCount == 1, "RefCount is not 1 but %lx\n", RefCount);
        ok(PinCount == 1, "PinCount is not 1 but %lx\n", PinCount);
        ok(!strcmpW(Name, testAtom2), "We found wrong atom\n");
        ok((strlenW(testAtom2) * sizeof(WCHAR)) == Len, "Returned wrong length %ld\n", Len);

        res = pRtlEmptyAtomTable(AtomTable, FALSE);
        ok(!res, "Unable to empty atom table, retval %lx\n", res);

        Len = 64;
        res = pRtlQueryAtomInAtomTable(AtomTable, Atom2, &RefCount, &PinCount, Name, &Len);
        ok(!res, "It seems RtlEmptyAtomTable deleted our pinned atom eaven though we asked it not to, retval: %lx\n", res);
        ok(RefCount == 1, "RefCount is not 1 but %lx\n", RefCount);
        ok(PinCount == 1, "PinCount is not 1 but %lx\n", PinCount);
        ok(!strcmpW(Name, testAtom2), "We found wrong atom\n");
        ok((strlenW(testAtom2) * sizeof(WCHAR)) == Len, "Returned wrong length %ld\n", Len);

        res = pRtlLookupAtomInAtomTable(AtomTable, testAtom2, &testAtom);
        ok(!res, "We can't find our pinned atom!! retval: %lx\n", res);
        ok(testAtom == Atom2, "We found wrong atom!!!\n");

        res = pRtlLookupAtomInAtomTable(AtomTable, testAtom1, &testAtom);
        ok(res == STATUS_OBJECT_NAME_NOT_FOUND, "We found the atom in our table eaven though we asked RtlEmptyAtomTable to remove it, retval: %lx\n", res);

        res = pRtlAddAtomToAtomTable(AtomTable, testAtom3, &Atom3);
        ok(!res, "Unable to add atom to table, retval: %lx\n", res);

        res = pRtlEmptyAtomTable(AtomTable, TRUE);
        ok(!res, "Unable to empty atom table, retval: %lx\n", res);

        res = pRtlLookupAtomInAtomTable(AtomTable, testAtom2, &testAtom);
        ok(res == STATUS_OBJECT_NAME_NOT_FOUND, "The pinned atom should be removed, retval: %lx\n", res);

        res = pRtlLookupAtomInAtomTable(AtomTable, testAtom3, &testAtom);
        ok(res == STATUS_OBJECT_NAME_NOT_FOUND, "Non pinned atom should also be removed, retval: %lx\n", res);

        res = pRtlDestroyAtomTable(AtomTable);
        ok(!res, "Can't destroy atom table, retval: %lx\n", res);
    }

    AtomTable = NULL;
    res = pRtlCreateAtomTable(37, &AtomTable);
    ok(!res, "Unable to create atom table, retval: %lx\n", res);

    if (!res)
    {
        res = pRtlLookupAtomInAtomTable(AtomTable, testAtom1, &testAtom);
        ok(res == STATUS_OBJECT_NAME_NOT_FOUND, "Didn't get expected retval with querying an empty atom table, retval: %lx\n", res);

        res = pRtlAddAtomToAtomTable(AtomTable, testAtom1, &Atom1);
        ok(!res, "Unable to add atom to atom table, retval %lx\n", res);

        res = pRtlLookupAtomInAtomTable(AtomTable, testAtom1, &testAtom);
        ok(!res, "Can't find previously added atom in table, retval: %lx\n", res);
        ok(testAtom == Atom1, "Found wrong atom! retval: %lx\n", res);

        res = pRtlDeleteAtomFromAtomTable(AtomTable, Atom1);
        ok(!res, "Unable to delete atom from table, retval: %lx\n", res);

        res = pRtlLookupAtomInAtomTable(AtomTable, testAtom1, &testAtom);
        ok(res == STATUS_OBJECT_NAME_NOT_FOUND, "Able to find previously deleted atom in table, retval: %lx\n", res);

        res = pRtlAddAtomToAtomTable(AtomTable, testAtom1, &Atom1);
        ok(!res, "Unable to add atom to atom table, retval: %lx\n", res);

        Len = 0;
        res = pRtlQueryAtomInAtomTable(AtomTable, Atom1, NULL, NULL, Name, &Len);
        ok(res == STATUS_BUFFER_TOO_SMALL, "Got wrong retval, retval: %lx\n", res);
        ok((strlenW(testAtom1) * sizeof(WCHAR)) == Len, "Got wrong length %lx\n", Len);

        res = pRtlQueryAtomInAtomTable(AtomTable, Atom1, NULL, NULL, NULL, &Len);
        ok(!res, "Failed to retrieve atom length, retval: %lx\n", res);
        ok(Len == strlenW(testAtom1) * sizeof(WCHAR), "Invalid atom length got %lu expected %u\n",
           Len, strlenW(testAtom1) * sizeof(WCHAR));

        Len = strlenW(testAtom1) * sizeof(WCHAR);
        Name[strlenW(testAtom1)] = '*';
        res = pRtlQueryAtomInAtomTable(AtomTable, Atom1, NULL, NULL, Name, &Len);
        ok(!res, "Failed with exactly long enough buffer, retval: %lx\n", res);
        ok(Name[strlenW(testAtom1)] == '*', "Writing outside buffer\n");
        ok(0 == memcmp(Name, testAtom1, (strlenW(testAtom1) - 1) * sizeof(WCHAR)),
           "We found wrong atom!!\n");

        res = pRtlPinAtomInAtomTable(AtomTable, Atom1);
        ok(!res, "Unable to pin atom in atom table, retval: %lx\n", res);

        res = pRtlLookupAtomInAtomTable(AtomTable, testAtom1, &testAtom);
        ok(!res, "Unable to find atom in atom table, retval: %lx\n", res);
        ok(testAtom == Atom1, "Wrong atom found\n");

        res = pRtlDeleteAtomFromAtomTable(AtomTable, Atom1);
        ok(res == STATUS_WAS_LOCKED, "Unable to delete atom from table, retval: %lx\n", res);

        res = pRtlLookupAtomInAtomTable(AtomTable, testAtom1, &testAtom);
        ok(!res, "Able to find deleted atom in table\n");

        res = pRtlDestroyAtomTable(AtomTable);
        ok(!res, "Unable to destroy atom table\n");
    }
}

/* Test Adding integer atoms to atom table */
static void test_NtIntAtom(void)
{
    NTSTATUS res;
    RTL_ATOM_TABLE AtomTable;
    RTL_ATOM testAtom;
    ULONG RefCount = 0, PinCount = 0;
    int i;
    WCHAR Name[64];
    ULONG Len;

    AtomTable = NULL;
    res = pRtlCreateAtomTable(37, &AtomTable);
    ok(!res, "Unable to create atom table, %lx\n", res);

    if (!res)
    {
        /* According to the kernel32 functions, integer atoms are only allowd from
         * 0x0001 to 0xbfff and not 0xc000 to 0xffff, which is correct */
        res = pRtlAddAtomToAtomTable(AtomTable, (PWSTR)0, &testAtom);
        ok(res == STATUS_INVALID_PARAMETER, "Didn't get expected result from adding 0 int atom, retval: %lx\n", res);
        for (i = 1; i <= 0xbfff; i++)
        {
            res = pRtlAddAtomToAtomTable(AtomTable, (PWSTR)i, &testAtom);
            ok(!res, "Unable to add valid integer atom %i, retval: %lx\n", i, res);
        }

        for (i = 1; i <= 0xbfff; i++)
        {
            res = pRtlLookupAtomInAtomTable(AtomTable, (PWSTR)i, &testAtom);
            ok(!res, "Unable to find int atom %i, retval: %lx\n", i, res);
            if (!res)
            {
                res = pRtlPinAtomInAtomTable(AtomTable, testAtom);
                ok(!res, "Unable to pin int atom %i, retval: %lx\n", i, res);
            }
        }

        for (i = 0xc000; i <= 0xffff; i++)
        {
            res = pRtlAddAtomToAtomTable(AtomTable, (PWSTR)i, &testAtom);
            ok(res, "Able to illeageal integer atom %i, retval: %lx\n", i, res);
        }

        res = pRtlDestroyAtomTable(AtomTable);
        ok(!res, "Unable to destroy atom table, retval: %lx\n", res);
    }

    AtomTable = NULL;
    res = pRtlCreateAtomTable(37, &AtomTable);
    ok(!res, "Unable to create atom table, %lx\n", res);
    if (!res)
    {
        res = pRtlLookupAtomInAtomTable(AtomTable, (PWSTR)123, &testAtom);
        ok(!res, "Unable to query atom in atom table, retval: %lx\n", res);

        res = pRtlAddAtomToAtomTable(AtomTable, testAtomInt, &testAtom);
        ok(!res, "Unable to add int atom to table, retval: %lx\n", res);

        res = pRtlAddAtomToAtomTable(AtomTable, testAtomIntInv, &testAtom);
        ok(!res, "Unable to add int atom to table, retval: %lx\n", res);

        res = pRtlAddAtomToAtomTable(AtomTable, (PWSTR)123, &testAtom);
        ok(!res, "Unable to add int atom to table, retval: %lx\n", res);

        res = pRtlAddAtomToAtomTable(AtomTable, (PWSTR)123, &testAtom);
        ok(!res, "Unable to re-add int atom to table, retval: %lx\n", res);

        Len = 64;
        res = pRtlQueryAtomInAtomTable(AtomTable, testAtom, &RefCount, &PinCount, Name, &Len);
        ok(!res, "Unable to query atom in atom table, retval: %lx\n", res);
        ok(PinCount == 1, "Expected pincount 1 but got %lx\n", PinCount);
        ok(RefCount == 1, "Expected refcount 1 but got %lx\n", RefCount);
        ok(!strcmpW(testAtomOTT, Name), "Got wrong atom name\n");
        ok((strlenW(testAtomOTT) * sizeof(WCHAR)) == Len, "Got wrong len %ld\n", Len);

        res = pRtlPinAtomInAtomTable(AtomTable, testAtom);
        ok(!res, "Unable to pin int atom, retval: %lx\n", res);

        res = pRtlPinAtomInAtomTable(AtomTable, testAtom);
        ok(!res, "Unable to pin int atom, retval: %lx\n", res);

        res = pRtlQueryAtomInAtomTable(AtomTable, testAtom, &RefCount, &PinCount, NULL, NULL);
        ok(!res, "Unable to query atom in atom table, retval: %lx\n", res);
        ok(PinCount == 1, "Expected pincount 1 but got %lx\n", PinCount);
        ok(RefCount == 1, "Expected refcount 1 but got %lx\n", RefCount);

        res = pRtlDestroyAtomTable(AtomTable);
        ok(!res, "Unable to destroy atom table, retval: %lx\n", res);
    }
}

/* Tests to see how the pincount and refcount actually works */
static void test_NtRefPinAtom(void)
{
    RTL_ATOM_TABLE AtomTable;
    RTL_ATOM Atom;
    ULONG PinCount = 0, RefCount = 0;
    NTSTATUS res;

    AtomTable = NULL;
    res = pRtlCreateAtomTable(37, &AtomTable);
    ok(!res, "Unable to create atom table, %lx\n", res);

    if (!res)
    {
        res = pRtlAddAtomToAtomTable(AtomTable, testAtom1, &Atom);
        ok(!res, "Unable to add our atom to the atom table, retval: %lx\n", res);

        res = pRtlAddAtomToAtomTable(AtomTable, testAtom1, &Atom);
        ok(!res, "Unable to add our atom to the atom table, retval: %lx\n", res);

        res = pRtlAddAtomToAtomTable(AtomTable, testAtom1, &Atom);
        ok(!res, "Unable to add our atom to the atom table, retval: %lx\n", res);

        res = pRtlQueryAtomInAtomTable(AtomTable, Atom, &RefCount, &PinCount, NULL, NULL);
        ok(!res, "Unable to query atom in atom table, retval: %lx\n", res);
        ok(PinCount == 0, "Expected pincount 0 but got %lx\n", PinCount);
        ok(RefCount == 3, "Expected refcount 3 but got %lx\n", RefCount); 

        res = pRtlPinAtomInAtomTable(AtomTable, Atom);
        ok(!res, "Unable to pin atom in atom table, retval: %lx\n", res);

        res = pRtlPinAtomInAtomTable(AtomTable, Atom);
        ok(!res, "Unable to pin atom in atom table, retval: %lx\n", res);

        res = pRtlPinAtomInAtomTable(AtomTable, Atom);
        ok(!res, "Unable to pin atom in atom table, retval: %lx\n", res);

        res = pRtlQueryAtomInAtomTable(AtomTable, Atom, &RefCount, &PinCount, NULL, NULL);
        ok(!res, "Unable to query atom in atom table, retval: %lx\n", res);
        ok(PinCount == 1, "Expected pincount 1 but got %lx\n", PinCount);
        ok(RefCount == 3, "Expected refcount 3 but got %lx\n", RefCount);

        res = pRtlDestroyAtomTable(AtomTable);
        ok(!res, "Unable to destroy atom table, retval: %lx\n", res);
    }
}

START_TEST(atom)
{
    InitFunctionPtr();
    if (pRtlCreateAtomTable)
    {
        test_NtAtom();
        test_NtIntAtom();
        test_NtRefPinAtom();
    }
}
