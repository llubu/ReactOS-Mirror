/* Unit test suite for SHLWAPI string functions
 *
 * Copyright 2003 Jon Griffiths
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

#include <stdio.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "wine/test.h"
#include "winbase.h"
#include "winerror.h"
#include "winnls.h"
#define NO_SHLWAPI_REG
#define NO_SHLWAPI_PATH
#define NO_SHLWAPI_GDI
#define NO_SHLWAPI_STREAM
#include "shlwapi.h"
#include "shtypes.h"

static HMODULE hShlwapi;
static LPSTR   (WINAPI *pStrCpyNXA)(LPSTR,LPCSTR,int);
static LPWSTR  (WINAPI *pStrCpyNXW)(LPWSTR,LPCWSTR,int);
static HRESULT (WINAPI *pStrRetToBSTR)(STRRET*,void*,BSTR*);
static DWORD   (WINAPI *pSHAnsiToAnsi)(LPCSTR,LPSTR,int);
static DWORD   (WINAPI *pSHUnicodeToUnicode)(LPCWSTR,LPWSTR,int);
static BOOL    (WINAPI *pStrIsIntlEqualA)(BOOL,LPCSTR,LPCSTR,int);
static BOOL    (WINAPI *pIntlStrEqWorkerA)(BOOL,LPCSTR,LPCSTR,int);
static BOOL    (WINAPI *pStrIsIntlEqualW)(BOOL,LPCWSTR,LPCWSTR,int);
static BOOL    (WINAPI *pIntlStrEqWorkerW)(BOOL,LPCWSTR,LPCWSTR,int);

static inline int strcmpW(const WCHAR *str1, const WCHAR *str2)
{
    while (*str1 && (*str1 == *str2)) { str1++; str2++; }
    return *str1 - *str2;
}

/* StrToInt/StrToIntEx results */
typedef struct tagStrToIntResult
{
  const char* string;
  int str_to_int;
  int str_to_int_ex;
  int str_to_int_hex;
} StrToIntResult;

static const StrToIntResult StrToInt_results[] = {
     { "1099", 1099, 1099, 1099 },
     { "+88987", 0, 88987, 88987 },
     { "012", 12, 12, 12 },
     { "-55", -55, -55, -55 },
     { "-0", 0, 0, 0 },
     { "0x44ff", 0, 0, 0x44ff },
     { "+0x44f4", 0, 0, 0x44f4 },
     { "-0x44fd", 0, 0, 0x44fd },
     { "+ 88987", 0, 0, 0 },
     { "- 55", 0, 0, 0 },
     { "- 0", 0, 0, 0 },
     { "+ 0x44f4", 0, 0, 0 },
     { "--0x44fd", 0, 0, 0 },
     { " 1999", 0, 1999, 1999 },
     { " +88987", 0, 88987, 88987 },
     { " 012", 0, 12, 12 },
     { " -55", 0, -55, -55 },
     { " 0x44ff", 0, 0, 0x44ff },
     { " +0x44f4", 0, 0, 0x44f4 },
     { " -0x44fd", 0, 0, 0x44fd },
     { NULL, 0, 0, 0 }
};

/* pStrFormatByteSize64/StrFormatKBSize results */
typedef struct tagStrFormatSizeResult
{
  LONGLONG value;
  const char* byte_size_64;
  const char* kb_size;
} StrFormatSizeResult;


static const StrFormatSizeResult StrFormatSize_results[] = {
  { -1023, "-1023 bytes", "0 KB"},
  { -24, "-24 bytes", "0 KB"},
  { 309, "309 bytes", "1 KB"},
  { 10191, "9.95 KB", "10 KB"},
  { 100353, "98.0 KB", "99 KB"},
  { 1022286, "998 KB", "999 KB"},
  { 1046862, "0.99 MB", "1,023 KB"},
  { 1048574619, "999 MB", "1,023,999 KB"},
  { 1073741775, "0.99 GB", "1,048,576 KB"},
  { ((LONGLONG)0x000000f9 << 32) | 0xfffff94e, "999 GB", "1,048,575,999 KB"},
  { ((LONGLONG)0x000000ff << 32) | 0xfffffa9b, "0.99 TB", "1,073,741,823 KB"},
  { ((LONGLONG)0x0003e7ff << 32) | 0xfffffa9b, "999 TB", "1,073,741,823,999 KB"},
  { ((LONGLONG)0x0003ffff << 32) | 0xfffffbe8, "0.99 PB", "1,099,511,627,775 KB"},
  { ((LONGLONG)0x0f9fffff << 32) | 0xfffffd35, "999 PB", "1,099,511,627,776,000 KB"},
  { ((LONGLONG)0x0fffffff << 32) | 0xfffffa9b, "0.99 EB", "1,125,899,906,842,623 KB"},
  { 0, NULL, NULL }
};

/* StrFormatByteSize64/StrFormatKBSize results */
typedef struct tagStrFromTimeIntervalResult
{
  DWORD ms;
  int   digits;
  const char* time_interval;
} StrFromTimeIntervalResult;


static const StrFromTimeIntervalResult StrFromTimeInterval_results[] = {
  { 1, 1, " 0 sec" },
  { 1, 2, " 0 sec" },
  { 1, 3, " 0 sec" },
  { 1, 4, " 0 sec" },
  { 1, 5, " 0 sec" },
  { 1, 6, " 0 sec" },
  { 1, 7, " 0 sec" },

  { 1000000, 1, " 10 min" },
  { 1000000, 2, " 16 min" },
  { 1000000, 3, " 16 min 40 sec" },
  { 1000000, 4, " 16 min 40 sec" },
  { 1000000, 5, " 16 min 40 sec" },
  { 1000000, 6, " 16 min 40 sec" },
  { 1000000, 7, " 16 min 40 sec" },

  { 1999999, 1, " 30 min" },
  { 1999999, 2, " 33 min" },
  { 1999999, 3, " 33 min 20 sec" },
  { 1999999, 4, " 33 min 20 sec" },
  { 1999999, 5, " 33 min 20 sec" },
  { 1999999, 6, " 33 min 20 sec" },
  { 1999999, 7, " 33 min 20 sec" },

  { 3999997, 1, " 1 hr" },
  { 3999997, 2, " 1 hr 6 min" },
  { 3999997, 3, " 1 hr 6 min 40 sec" },
  { 3999997, 4, " 1 hr 6 min 40 sec" },
  { 3999997, 5, " 1 hr 6 min 40 sec" },
  { 3999997, 6, " 1 hr 6 min 40 sec" },
  { 3999997, 7, " 1 hr 6 min 40 sec" },

  { 149999851, 7, " 41 hr 40 min 0 sec" },
  { 150999850, 1, " 40 hr" },
  { 150999850, 2, " 41 hr" },
  { 150999850, 3, " 41 hr 50 min" },
  { 150999850, 4, " 41 hr 56 min" },
  { 150999850, 5, " 41 hr 56 min 40 sec" },
  { 150999850, 6, " 41 hr 56 min 40 sec" },
  { 150999850, 7, " 41 hr 56 min 40 sec" },

  { 493999507, 1, " 100 hr" },
  { 493999507, 2, " 130 hr" },
  { 493999507, 3, " 137 hr" },
  { 493999507, 4, " 137 hr 10 min" },
  { 493999507, 5, " 137 hr 13 min" },
  { 493999507, 6, " 137 hr 13 min 20 sec" },
  { 493999507, 7, " 137 hr 13 min 20 sec" },

  { 0, 0, NULL }
};

static void test_StrChrA(void)
{
  char string[129];
  WORD count;

  /* this test crashes on win2k SP4 */
  /*ok(!StrChrA(NULL,'\0'), "found a character in a NULL string!\n");*/

  for (count = 32; count < 128; count++)
    string[count] = (char)count;
  string[128] = '\0';

  for (count = 32; count < 128; count++)
  {
    LPSTR result = StrChrA(string+32, count);
    ok(result - string == count,
        "found char '%c' in wrong place: got %d, expected %d\n",
        count, result - string, count);
  }

  for (count = 32; count < 128; count++)
  {
    LPSTR result = StrChrA(string+count+1, count);
    ok(!result, "found char '%c' not in the string\n", count);
  }
}

static void test_StrChrW(void)
{
  WCHAR string[16385];
  WORD count;

  /* this test crashes on win2k SP4 */
  /*ok(!StrChrW(NULL,'\0'), "found a character in a NULL string!\n");*/

  for (count = 32; count < 16384; count++)
    string[count] = count;
  string[16384] = '\0';

  for (count = 32; count < 16384; count++)
  {
    LPWSTR result = StrChrW(string+32, count);
    ok((result - string) == count, "found char %d in wrong place\n", count);
  }

  for (count = 32; count < 16384; count++)
  {
    LPWSTR result = StrChrW(string+count+1, count);
    ok(!result, "found char not in the string\n");
  }
}

static void test_StrChrIA(void)
{
  char string[129];
  WORD count;

  /* this test crashes on win2k SP4 */
  /*ok(!StrChrIA(NULL,'\0'), "found a character in a NULL string!\n");*/

  for (count = 32; count < 128; count++)
    string[count] = (char)count;
  string[128] = '\0';

  for (count = 'A'; count <= 'X'; count++)
  {
    LPSTR result = StrChrIA(string+32, count);

    ok(result - string == count, "found char '%c' in wrong place\n", count);
    ok(StrChrIA(result, count)!=NULL, "didn't find lowercase '%c'\n", count);
  }

  for (count = 'a'; count < 'z'; count++)
  {
    LPSTR result = StrChrIA(string+count+1, count);
    ok(!result, "found char not in the string\n");
  }
}

static void test_StrChrIW(void)
{
  WCHAR string[129];
  WORD count;

  /* this test crashes on win2k SP4 */
  /*ok(!StrChrIA(NULL,'\0'), "found a character in a NULL string!\n");*/

  for (count = 32; count < 128; count++)
    string[count] = count;
  string[128] = '\0';

  for (count = 'A'; count <= 'X'; count++)
  {
    LPWSTR result = StrChrIW(string+32, count);

    ok(result - string == count, "found char '%c' in wrong place\n", count);
    ok(StrChrIW(result, count)!=NULL, "didn't find lowercase '%c'\n", count);
  }

  for (count = 'a'; count < 'z'; count++)
  {
    LPWSTR result = StrChrIW(string+count+1, count);
    ok(!result, "found char not in the string\n");
  }
}

static void test_StrRChrA(void)
{
  char string[129];
  WORD count;

  /* this test crashes on win2k SP4 */
  /*ok(!StrRChrA(NULL, NULL,'\0'), "found a character in a NULL string!\n");*/

  for (count = 32; count < 128; count++)
    string[count] = (char)count;
  string[128] = '\0';

  for (count = 32; count < 128; count++)
  {
    LPSTR result = StrRChrA(string+32, NULL, count);
    ok(result - string == count, "found char %d in wrong place\n", count);
  }

  for (count = 32; count < 128; count++)
  {
    LPSTR result = StrRChrA(string+count+1, NULL, count);
    ok(!result, "found char not in the string\n");
  }

  for (count = 32; count < 128; count++)
  {
    LPSTR result = StrRChrA(string+count+1, string + 127, count);
    ok(!result, "found char not in the string\n");
  }
}

static void test_StrRChrW(void)
{
  WCHAR string[129];
  WORD count;

  /* this test crashes on win2k SP4 */
  /*ok(!StrRChrW(NULL, NULL,'\0'), "found a character in a NULL string!\n");*/

  for (count = 32; count < 128; count++)
    string[count] = count;
  string[128] = '\0';

  for (count = 32; count < 128; count++)
  {
    LPWSTR result = StrRChrW(string+32, NULL, count);
    ok(result - string == count,
        "found char %d in wrong place: got %d, expected %d\n",
        count, result - string, count);
  }

  for (count = 32; count < 128; count++)
  {
    LPWSTR result = StrRChrW(string+count+1, NULL, count);
    ok(!result, "found char %d not in the string\n", count);
  }

  for (count = 32; count < 128; count++)
  {
    LPWSTR result = StrRChrW(string+count+1, string + 127, count);
    ok(!result, "found char %d not in the string\n", count);
  }
}

static void test_StrCpyW(void)
{
  WCHAR szSrc[256];
  WCHAR szBuff[256];
  const StrFormatSizeResult* result = StrFormatSize_results;


  while(result->value)
  {
    MultiByteToWideChar(0,0,result->byte_size_64,-1,szSrc,sizeof(szSrc)/sizeof(WCHAR));

    StrCpyW(szBuff, szSrc);
    ok(!StrCmpW(szSrc, szBuff), "Copied string %s wrong\n", result->byte_size_64);
    result++;
  }
}


static void test_StrToIntA(void)
{
  const StrToIntResult *result = StrToInt_results;
  int return_val;

  while (result->string)
  {
    return_val = StrToIntA(result->string);
    ok(return_val == result->str_to_int, "converted '%s' wrong (%d)\n",
       result->string, return_val);
    result++;
  }
}

static void test_StrToIntW(void)
{
  WCHAR szBuff[256];
  const StrToIntResult *result = StrToInt_results;
  int return_val;

  while (result->string)
  {
    MultiByteToWideChar(0,0,result->string,-1,szBuff,sizeof(szBuff)/sizeof(WCHAR));
    return_val = StrToIntW(szBuff);
    ok(return_val == result->str_to_int, "converted '%s' wrong (%d)\n",
       result->string, return_val);
    result++;
  }
}

static void test_StrToIntExA(void)
{
  const StrToIntResult *result = StrToInt_results;
  int return_val;
  BOOL bRet;

  while (result->string)
  {
    return_val = -1;
    bRet = StrToIntExA(result->string,0,&return_val);
    ok(!bRet || return_val != -1, "No result returned from '%s'\n",
       result->string);
    if (bRet)
      ok(return_val == result->str_to_int_ex, "converted '%s' wrong (%d)\n",
         result->string, return_val);
    result++;
  }

  result = StrToInt_results;
  while (result->string)
  {
    return_val = -1;
    bRet = StrToIntExA(result->string,STIF_SUPPORT_HEX,&return_val);
    ok(!bRet || return_val != -1, "No result returned from '%s'\n",
       result->string);
    if (bRet)
      ok(return_val == result->str_to_int_hex, "converted '%s' wrong (%d)\n",
         result->string, return_val);
    result++;
  }
}

static void test_StrToIntExW(void)
{
  WCHAR szBuff[256];
  const StrToIntResult *result = StrToInt_results;
  int return_val;
  BOOL bRet;

  while (result->string)
  {
    return_val = -1;
    MultiByteToWideChar(0,0,result->string,-1,szBuff,sizeof(szBuff)/sizeof(WCHAR));
    bRet = StrToIntExW(szBuff, 0, &return_val);
    ok(!bRet || return_val != -1, "No result returned from '%s'\n",
       result->string);
    if (bRet)
      ok(return_val == result->str_to_int_ex, "converted '%s' wrong (%d)\n",
         result->string, return_val);
    result++;
  }

  result = StrToInt_results;
  while (result->string)
  {
    return_val = -1;
    MultiByteToWideChar(0,0,result->string,-1,szBuff,sizeof(szBuff)/sizeof(WCHAR));
    bRet = StrToIntExW(szBuff, STIF_SUPPORT_HEX, &return_val);
    ok(!bRet || return_val != -1, "No result returned from '%s'\n",
       result->string);
    if (bRet)
      ok(return_val == result->str_to_int_hex, "converted '%s' wrong (%d)\n",
         result->string, return_val);
    result++;
  }
}

static void test_StrDupA(void)
{
  LPSTR lpszStr;
  const StrFormatSizeResult* result = StrFormatSize_results;

  while(result->value)
  {
    lpszStr = StrDupA(result->byte_size_64);

    ok(lpszStr != NULL, "Dup failed\n");
    if (lpszStr)
    {
      ok(!strcmp(result->byte_size_64, lpszStr), "Copied string wrong\n");
      LocalFree((HLOCAL)lpszStr);
    }
    result++;
  }

  /* Later versions of shlwapi return NULL for this, but earlier versions
   * returned an empty string (as Wine does).
   */
  lpszStr = StrDupA(NULL);
  ok(lpszStr == NULL || *lpszStr == '\0', "NULL string returned %p\n", lpszStr);
}

static void test_StrFormatByteSize64A(void)
{
  char szBuff[256];
  const StrFormatSizeResult* result = StrFormatSize_results;

  while(result->value)
  {
    StrFormatByteSize64A(result->value, szBuff, 256);

    ok(!strcmp(result->byte_size_64, szBuff),
        "Formatted %lx%08lx wrong: got %s, expected %s\n",
       (LONG)(result->value >> 32), (LONG)result->value, szBuff, result->byte_size_64);

    result++;
  }
}

static void test_StrFormatKBSizeW(void)
{
  WCHAR szBuffW[256];
  char szBuff[256];
  const StrFormatSizeResult* result = StrFormatSize_results;

  while(result->value)
  {
    StrFormatKBSizeW(result->value, szBuffW, 256);
    WideCharToMultiByte(0,0,szBuffW,-1,szBuff,sizeof(szBuff)/sizeof(WCHAR),0,0);
    ok(!strcmp(result->kb_size, szBuff),
        "Formatted %lx%08lx wrong: got %s, expected %s\n",
       (LONG)(result->value >> 32), (LONG)result->value, szBuff, result->kb_size);
    result++;
  }
}

static void test_StrFormatKBSizeA(void)
{
  char szBuff[256];
  const StrFormatSizeResult* result = StrFormatSize_results;

  while(result->value)
  {
    StrFormatKBSizeA(result->value, szBuff, 256);

    ok(!strcmp(result->kb_size, szBuff),
        "Formatted %lx%08lx wrong: got %s, expected %s\n",
       (LONG)(result->value >> 32), (LONG)result->value, szBuff, result->kb_size);
    result++;
  }
}

static void test_StrFromTimeIntervalA(void)
{
  char szBuff[256];
  const StrFromTimeIntervalResult* result = StrFromTimeInterval_results;

  while(result->ms)
  {
    StrFromTimeIntervalA(szBuff, 256, result->ms, result->digits);

    ok(!strcmp(result->time_interval, szBuff), "Formatted %ld %d wrong\n",
       result->ms, result->digits);
    result++;
  }
}

static void test_StrCmpA(void)
{
  static const char str1[] = {'a','b','c','d','e','f'};
  static const char str2[] = {'a','B','c','d','e','f'};
  ok(0 != StrCmpNA(str1, str2, 6), "StrCmpNA is case-insensitive\n");
  ok(0 == StrCmpNIA(str1, str2, 6), "StrCmpNIA is case-sensitive\n");
  ok(!ChrCmpIA('a', 'a'), "ChrCmpIA doesn't work at all!\n");
  ok(!ChrCmpIA('b', 'B'), "ChrCmpIA is not case-insensitive\n");
  ok(ChrCmpIA('a', 'z'), "ChrCmpIA believes that a == z!\n");

  pStrIsIntlEqualA = (void *)GetProcAddress(hShlwapi, "StrIsIntlEqualA");
  pIntlStrEqWorkerA = (void *)GetProcAddress(hShlwapi, "IntlStrEqWorkerA");

  if (!pStrIsIntlEqualA)
    return;

  ok(pStrIsIntlEqualA(FALSE, str1, str2, 5), "StrIsIntlEqualA(FALSE,...) isn't case-insensitive\n");
  ok(!pStrIsIntlEqualA(TRUE, str1, str2, 5), "StrIsIntlEqualA(TRUE,...) isn't case-sensitive\n");

  if (!pIntlStrEqWorkerA)
    return;

  ok(pIntlStrEqWorkerA(FALSE, str1, str2, 5), "IntlStrEqWorkerA(FALSE,...) isn't case-insensitive\n");
  ok(!pIntlStrEqWorkerA(TRUE, str1, str2, 5), "pIntlStrEqWorkerA(TRUE,...) isn't case-sensitive\n");
}

static void test_StrCmpW(void)
{
  static const WCHAR str1[] = {'a','b','c','d','e','f'};
  static const WCHAR str2[] = {'a','B','c','d','e','f'};
  ok(0 != StrCmpNW(str1, str2, 5), "StrCmpNW is case-insensitive\n");
  ok(0 == StrCmpNIW(str1, str2, 5), "StrCmpNIW is case-sensitive\n");
  ok(!ChrCmpIW('a', 'a'), "ChrCmpIW doesn't work at all!\n");
  ok(!ChrCmpIW('b', 'B'), "ChrCmpIW is not case-insensitive\n");
  ok(ChrCmpIW('a', 'z'), "ChrCmpIW believes that a == z!\n");

  pStrIsIntlEqualW = (void *)GetProcAddress(hShlwapi, "StrIsIntlEqualW");
  pIntlStrEqWorkerW = (void *)GetProcAddress(hShlwapi, "IntlStrEqWorkerW");

  if (!pStrIsIntlEqualW)
    return;

  ok(pStrIsIntlEqualW(FALSE, str1, str2, 5), "StrIsIntlEqualW(FALSE,...) isn't case-insensitive\n");
  ok(!pStrIsIntlEqualW(TRUE, str1, str2, 5), "StrIsIntlEqualW(TRUE,...) isn't case-sensitive\n");

  if (!pIntlStrEqWorkerW)
    return;

  ok(pIntlStrEqWorkerW(FALSE, str1, str2, 5), "IntlStrEqWorkerW(FALSE,...) isn't case-insensitive\n");
  ok(!pIntlStrEqWorkerW(TRUE, str1, str2, 5), "IntlStrEqWorkerW(TRUE,...) isn't case-sensitive\n");
}

static WCHAR *CoDupStrW(const char* src)
{
  INT len = MultiByteToWideChar(CP_ACP, 0, src, -1, NULL, 0);
  WCHAR* szTemp = (WCHAR*)CoTaskMemAlloc(len * sizeof(WCHAR));
  MultiByteToWideChar(CP_ACP, 0, src, -1, szTemp, len);
  return szTemp;
}

static void test_StrRetToBSTR(void)
{
    static const WCHAR szTestW[] = { 'T','e','s','t','\0' };
    ITEMIDLIST iidl[10];
    BSTR bstr;
    STRRET strret;
    HRESULT ret;

    pStrRetToBSTR = (void *)GetProcAddress(hShlwapi, "StrRetToBSTR");
    if (!pStrRetToBSTR) return;

    strret.uType = STRRET_WSTR;
    strret.u.pOleStr = CoDupStrW("Test");
    bstr = 0;
    ret = pStrRetToBSTR(&strret, NULL, &bstr);
    ok(ret == S_OK && bstr && !strcmpW(bstr, szTestW),
       "STRRET_WSTR: dup failed, ret=0x%08lx, bstr %p\n", ret, bstr);
    if (bstr)
      SysFreeString(bstr);

    strret.uType = STRRET_CSTR;
    lstrcpyA(strret.u.cStr, "Test");
    ret = pStrRetToBSTR(&strret, NULL, &bstr);
    ok(ret == S_OK && bstr && !strcmpW(bstr, szTestW),
       "STRRET_CSTR: dup failed, ret=0x%08lx, bstr %p\n", ret, bstr);
    if (bstr)
      SysFreeString(bstr);

    strret.uType = STRRET_OFFSET;
    strret.u.uOffset = 1;
    strcpy((char*)&iidl, " Test");
    ret = pStrRetToBSTR(&strret, iidl, &bstr);
    ok(ret == S_OK && bstr && !strcmpW(bstr, szTestW),
       "STRRET_OFFSET: dup failed, ret=0x%08lx, bstr %p\n", ret, bstr);
    if (bstr)
      SysFreeString(bstr);

    /* Native crashes if str is NULL */
}

static void test_StrCpyNXA(void)
{
  LPCSTR lpSrc = "hello";
  LPSTR lpszRes;
  char dest[8];

  pStrCpyNXA = (void *)GetProcAddress(hShlwapi, (LPSTR)399);
  if (!pStrCpyNXA)
    return;

  memset(dest, '\n', sizeof(dest));
  lpszRes = pStrCpyNXA(dest, lpSrc, sizeof(dest)/sizeof(dest[0]));
  ok(lpszRes == dest + 5 && !memcmp(dest, "hello\0\n\n", sizeof(dest)),
       "StrCpyNXA: expected %p, \"hello\\0\\n\\n\", got %p, \"%d,%d,%d,%d,%d,%d,%d,%d\"\n",
       dest + 5, lpszRes, dest[0], dest[1], dest[2], dest[3], dest[4], dest[5], dest[6], dest[7]);
}

static void test_StrCpyNXW(void)
{
  static const WCHAR lpInit[] = { '\n','\n','\n','\n','\n','\n','\n','\n' };
  static const WCHAR lpSrc[] = { 'h','e','l','l','o','\0' };
  static const WCHAR lpRes[] = { 'h','e','l','l','o','\0','\n','\n' };
  LPWSTR lpszRes;
  WCHAR dest[8];

  pStrCpyNXW = (void *)GetProcAddress(hShlwapi, (LPSTR)400);
  if (!pStrCpyNXW)
    return;

  memcpy(dest, lpInit, sizeof(lpInit));
  lpszRes = pStrCpyNXW(dest, lpSrc, sizeof(dest)/sizeof(dest[0]));
  ok(lpszRes == dest + 5 && !memcmp(dest, lpRes, sizeof(dest)),
       "StrCpyNXA: expected %p, \"hello\\0\\n\\n\", got %p, \"%d,%d,%d,%d,%d,%d,%d,%d\"\n",
       dest + 5, lpszRes, dest[0], dest[1], dest[2], dest[3], dest[4], dest[5], dest[6], dest[7]);
}

static void test_SHAnsiToAnsi(void)
{
  char dest[8];
  DWORD dwRet;

  pSHAnsiToAnsi = (void *)GetProcAddress(hShlwapi, (LPSTR)345);
  if (!pSHAnsiToAnsi)
    return;

  memset(dest, '\n', sizeof(dest));
  dwRet = pSHAnsiToAnsi("hello", dest, sizeof(dest)/sizeof(dest[0]));
  ok(dwRet == 6 && !memcmp(dest, "hello\0\n\n", sizeof(dest)),
     "SHAnsiToAnsi: expected 6, \"hello\\0\\n\\n\", got %ld, \"%d,%d,%d,%d,%d,%d,%d,%d\"\n",
     dwRet, dest[0], dest[1], dest[2], dest[3], dest[4], dest[5], dest[6], dest[7]);
}

static void test_SHUnicodeToUnicode(void)
{
  static const WCHAR lpInit[] = { '\n','\n','\n','\n','\n','\n','\n','\n' };
  static const WCHAR lpSrc[] = { 'h','e','l','l','o','\0' };
  static const WCHAR lpRes[] = { 'h','e','l','l','o','\0','\n','\n' };
  WCHAR dest[8];
  DWORD dwRet;

  pSHUnicodeToUnicode = (void *)GetProcAddress(hShlwapi, (LPSTR)346);
  if (!pSHUnicodeToUnicode)
    return;

  memcpy(dest, lpInit, sizeof(lpInit));
  dwRet = pSHUnicodeToUnicode(lpSrc, dest, sizeof(dest)/sizeof(dest[0]));
  ok(dwRet == 6 && !memcmp(dest, lpRes, sizeof(dest)),
     "SHUnicodeToUnicode: expected 6, \"hello\\0\\n\\n\", got %ld, \"%d,%d,%d,%d,%d,%d,%d,%d\"\n",
     dwRet, dest[0], dest[1], dest[2], dest[3], dest[4], dest[5], dest[6], dest[7]);
}

START_TEST(string)
{
  CoInitialize(0);

  hShlwapi = GetModuleHandleA("shlwapi");
  if (!hShlwapi)
     return;

  test_StrChrA();
  test_StrChrW();
  test_StrChrIA();
  test_StrChrIW();
  test_StrRChrA();
  test_StrRChrW();
  test_StrCpyW();
  test_StrToIntA();
  test_StrToIntW();
  test_StrToIntExA();
  test_StrToIntExW();
  test_StrDupA();
  if (0)
  {
    /* this test fails on locales which do not use '.' as a decimal separator */
    test_StrFormatByteSize64A();

    /* this test fails on locales which do not use '.' as a decimal separator */
    test_StrFormatKBSizeA();

    /* FIXME: Awaiting NLS fixes in kernel before these succeed */
    test_StrFormatKBSizeW();
  }
  test_StrFromTimeIntervalA();
  test_StrCmpA();
  test_StrCmpW();
  test_StrRetToBSTR();
  test_StrCpyNXA();
  test_StrCpyNXW();
  test_SHAnsiToAnsi();
  test_SHUnicodeToUnicode();
}
