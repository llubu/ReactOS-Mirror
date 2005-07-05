/* $Id$
 *
 * reactos/lib/kernel32/misc/errormsg.c
 *
 */
/*
 * FormatMessage implementation
 *
 * Copyright 1996 Marcus Meissner
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

#include <k32.h>

#define NDEBUG
#include "../include/debug.h"

#define TRACE DPRINT
#define FIXME DPRINT

/* strdup macros */
/* DO NOT USE IT!!  it will go away soon */
inline static LPSTR HEAP_strdupWtoA( HANDLE heap, DWORD flags, LPCWSTR str )
{
    LPSTR ret;
    INT len;

    if (!str) return NULL;
    len = WideCharToMultiByte( CP_ACP, 0, str, -1, NULL, 0, NULL, NULL );
    ret = RtlAllocateHeap(RtlGetProcessHeap(), flags, len );
    if(ret) WideCharToMultiByte( CP_ACP, 0, str, -1, ret, len, NULL, NULL );
    return ret;
}

/* Messages...used by FormatMessage32* (KERNEL32.something)
 *
 * They can be specified either directly or using a message ID and
 * loading them from the resource.
 *
 * The resourcedata has following format:
 * start:
 * 0: DWORD nrofentries
 * nrofentries * subentry:
 *      0: DWORD firstentry
 *      4: DWORD lastentry
 *      8: DWORD offset from start to the stringentries
 *
 * (lastentry-firstentry) * stringentry:
 * 0: WORD len (0 marks end)    [ includes the 4 byte header length ]
 * 2: WORD flags
 * 4: CHAR[len-4]
 *      (stringentry i of a subentry refers to the ID 'firstentry+i')
 *
 * Yes, ANSI strings in win32 resources. Go figure.
 */

/**********************************************************************
 *      load_messageA           (internal)
 */
static INT load_messageA( HMODULE instance, UINT id, WORD lang,
                          LPSTR buffer, INT buflen )
{
    HGLOBAL     hmem;
    HRSRC       hrsrc;
    PMESSAGE_RESOURCE_DATA      mrd;
    PMESSAGE_RESOURCE_BLOCK     mrb;
    PMESSAGE_RESOURCE_ENTRY     mre;
    int         i,slen;

    //TRACE("instance = %08lx, id = %08lx, buffer = %p, length = %ld\n", (DWORD)instance, (DWORD)id, buffer, (DWORD)buflen);

    /*FIXME: I am not sure about the '1' ... But I've only seen those entries*/
    hrsrc = FindResourceExW(instance,(LPWSTR)RT_MESSAGETABLE,(LPWSTR)1,lang);
    if (!hrsrc) return 0;
    hmem = LoadResource( instance, hrsrc );
    if (!hmem) return 0;

    mrd = (PMESSAGE_RESOURCE_DATA)LockResource(hmem);
    mre = NULL;
    mrb = &(mrd->Blocks[0]);
    for (i=mrd->NumberOfBlocks;i--;) {
        if ((id>=mrb->LowId) && (id<=mrb->HighId)) {
            mre = (PMESSAGE_RESOURCE_ENTRY)(((char*)mrd)+mrb->OffsetToEntries);
            id  -= mrb->LowId;
            break;
        }
        mrb++;
    }
    if (!mre)
        return 0;
    for (i=id;i--;) {
        if (!mre->Length)
                return 0;
        mre = (PMESSAGE_RESOURCE_ENTRY)(((char*)mre)+mre->Length);
    }
    slen=mre->Length;
    //TRACE("	- strlen=%d\n",slen);
    i = min(buflen - 1, slen);
    if (buffer == NULL)
        return slen;
    if (i>0) {
        if (mre->Flags & MESSAGE_RESOURCE_UNICODE)
            WideCharToMultiByte( CP_ACP, 0, (LPWSTR)mre->Text, -1, buffer, i, NULL, NULL );
        else
            lstrcpynA(buffer, (LPSTR)mre->Text, i);
        buffer[i]=0;
    } else {
        if (buflen>1) {
            buffer[0]=0;
            return 0;
        }
    }
    if (buffer) {
        //TRACE("'%s' copied !\n", buffer);
        //TRACE("'%s'\n", buffer);
    }
    return i;
}

#if 0  /* FIXME */
/**********************************************************************
 *      load_messageW   (internal)
 */
static INT load_messageW( HMODULE instance, UINT id, WORD lang,
                          LPWSTR buffer, INT buflen )
{
    INT retval;
    LPSTR buffer2 = NULL;
    if (buffer && buflen)
        buffer2 = HeapAlloc( GetProcessHeap(), 0, buflen );
    retval = load_messageA(instance,id,lang,buffer2,buflen);
    if (buffer)
    {
        if (retval) {
            lstrcpynAtoW( buffer, buffer2, buflen );
            retval = lstrlenW( buffer );
        }
        HeapFree( GetProcessHeap(), 0, buffer2 );
    }
    return retval;
}
#endif


/***********************************************************************
 *           FormatMessageA   (KERNEL32.@)
 * FIXME: missing wrap,
 *
 * @implemented
 */
DWORD WINAPI FormatMessageA(
        DWORD   dwFlags,
        LPCVOID lpSource,
        DWORD   dwMessageId,
        DWORD   dwLanguageId,
        LPSTR   lpBuffer,
        DWORD   nSize,
        va_list* _args )
{
    LPDWORD args=(LPDWORD)_args;
#if defined(__i386__) || defined(__sparc__)
/* This implementation is completely dependant on the format of the va_list on x86 CPUs */
    LPSTR       target,t;
    DWORD       talloced;
    LPSTR       from,f;
    DWORD       width = dwFlags & FORMAT_MESSAGE_MAX_WIDTH_MASK;
    BOOL    eos = FALSE;
    INT bufsize;
    HMODULE     hmodule = (HMODULE)lpSource;
    CHAR        ch;

    //TRACE("(0x%lx,%p,%ld,0x%lx,%p,%ld,%p)\n", dwFlags,lpSource,dwMessageId,dwLanguageId,lpBuffer,nSize,args);
    if ((dwFlags & FORMAT_MESSAGE_FROM_STRING)
        &&((dwFlags & FORMAT_MESSAGE_FROM_SYSTEM)
           || (dwFlags & FORMAT_MESSAGE_FROM_HMODULE))) return 0;

    if (width && width != FORMAT_MESSAGE_MAX_WIDTH_MASK)
        //FIXME("line wrapping (%lu) not supported.\n", width);
    from = NULL;
    if (dwFlags & FORMAT_MESSAGE_FROM_STRING)
    {
        from = RtlAllocateHeap(RtlGetProcessHeap(), 0, lstrlenA((LPSTR)lpSource)+1 );
        strcpy( from, (LPSTR)lpSource );
    }
    else {
        bufsize = 0;

        if (dwFlags & FORMAT_MESSAGE_FROM_HMODULE)
        {
           bufsize=load_messageA(hmodule,dwMessageId,dwLanguageId,NULL,100);
           if ((!bufsize) && (!dwLanguageId)) {
                bufsize=load_messageA(hmodule,dwMessageId,
                                      MAKELANGID(LANG_NEUTRAL,SUBLANG_NEUTRAL),NULL,100);
                if (!bufsize) bufsize=load_messageA(hmodule,dwMessageId,
                                                    MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),NULL,100);
                if (!bufsize) bufsize=load_messageA(hmodule,dwMessageId,
                                                    MAKELANGID(LANG_NEUTRAL,SUBLANG_SYS_DEFAULT),NULL,100);
                if (!bufsize) bufsize=load_messageA(hmodule,dwMessageId,
                                                    MAKELANGID(LANG_NEUTRAL,SUBLANG_SYS_DEFAULT),NULL,100);
                if (!bufsize) bufsize=load_messageA(hmodule,dwMessageId,
                                                    MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US),NULL,100);
            }
        }
        if ((dwFlags & FORMAT_MESSAGE_FROM_SYSTEM) && (!bufsize))
        {
           hmodule = GetModuleHandleA("kernel32");
           bufsize=load_messageA(hmodule,dwMessageId,dwLanguageId,NULL,100);
           if ((!bufsize) && (!dwLanguageId)) {
                bufsize=load_messageA(hmodule,dwMessageId,
                                      MAKELANGID(LANG_NEUTRAL,SUBLANG_NEUTRAL),NULL,100);
                if (!bufsize) bufsize=load_messageA(hmodule,dwMessageId,
                                                    MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),NULL,100);
                if (!bufsize) bufsize=load_messageA(hmodule,dwMessageId,
                                                    MAKELANGID(LANG_NEUTRAL,SUBLANG_SYS_DEFAULT),NULL,100);
                if (!bufsize) bufsize=load_messageA(hmodule,dwMessageId,
                                                    MAKELANGID(LANG_NEUTRAL,SUBLANG_SYS_DEFAULT),NULL,100);
                if (!bufsize) bufsize=load_messageA(hmodule,dwMessageId,
                                                    MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US),NULL,100);
            }
        }

        if (!bufsize) {
            TRACE("FormatMessageA: dwFlags=%#x hmodule=%#x dwMessageId=%#x - could not load message\n", dwFlags, hmodule, dwMessageId);
            SetLastError (ERROR_RESOURCE_LANG_NOT_FOUND);
            return 0;
        }

        from = RtlAllocateHeap(RtlGetProcessHeap(), 0, bufsize + 1 );
        load_messageA(hmodule,dwMessageId,dwLanguageId,from,bufsize+1);
    }
    target      = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, 100);
    t   = target;
    talloced= 100;

#define ADD_TO_T(c) do { \
        *t++=c;\
        if ((DWORD)((ULONG_PTR)t-(ULONG_PTR)target) == talloced) {\
            target = (char*)RtlReAllocateHeap(RtlGetProcessHeap(),HEAP_ZERO_MEMORY,target,talloced*2);\
            t = target+talloced;\
            talloced*=2;\
      }\
} while (0)

    if (from) {
        f=from;
        if (dwFlags & FORMAT_MESSAGE_IGNORE_INSERTS) {
            while (*f && !eos)
                ADD_TO_T(*f++);
        }
        else {
            while (*f && !eos) {
                if (*f=='%') {
                    int insertnr;
                    char *fmtstr,*x,*lastf;
                    DWORD *argliststart;

                    fmtstr = NULL;
                    lastf = f;
                    f++;
                    if (!*f) {
                        ADD_TO_T('%');
                        continue;
                    }
                    switch (*f) {
                    case '1':case '2':case '3':case '4':case '5':
                    case '6':case '7':case '8':case '9':
                        insertnr=*f-'0';
                        switch (f[1]) {
                        case '0':case '1':case '2':case '3':
                        case '4':case '5':case '6':case '7':
                        case '8':case '9':
                            f++;
                            insertnr=insertnr*10+*f-'0';
                            f++;
                            break;
                        default:
                            f++;
                            break;
                        }
                        if (*f=='!') {
                            f++;
                            if (NULL!=(x=strchr(f,'!'))) {
                                *x='\0';
                                fmtstr=RtlAllocateHeap(RtlGetProcessHeap(),0,lstrlenA(f)+2);
                                sprintf(fmtstr,"%%%s",f);
                                f=x+1;
                            } else {
                                fmtstr=RtlAllocateHeap(RtlGetProcessHeap(),0,lstrlenA(f)+2);
                                sprintf(fmtstr,"%%%s",f);
                                f+=lstrlenA(f); /*at \0*/
                            }
                        } else {
                            if(!args) break;
                            fmtstr = RtlAllocateHeap(RtlGetProcessHeap(),0,3);
                            strcpy( fmtstr, "%s" );
                        }
                        if (args) {
                            int sz;
                            LPSTR b;

                            if (dwFlags & FORMAT_MESSAGE_ARGUMENT_ARRAY)
                                argliststart=args+insertnr-1;
                            else
                                argliststart=(*(DWORD**)args)+insertnr-1;

                                /* FIXME: precision and width components are not handled correctly */
                            if ( (strcmp(fmtstr, "%ls") == 0) || (strcmp(fmtstr,"%S") == 0) ) {
                                sz = WideCharToMultiByte( CP_ACP, 0, *(WCHAR**)argliststart, -1, NULL, 0, NULL, NULL);
                                b = RtlAllocateHeap(RtlGetProcessHeap(), 0, sz);
                                WideCharToMultiByte( CP_ACP, 0, *(WCHAR**)argliststart, -1, b, sz, NULL, NULL);
                            } else {
                                b = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, sz = 1000);
                                /* CMF - This makes a BIG assumption about va_list */
                                //TRACE("A BIG assumption\n");
                                _vsnprintf(b, sz, fmtstr, (va_list) argliststart);
                            }
                            for (x=b; *x; x++) ADD_TO_T(*x);

                            RtlFreeHeap(RtlGetProcessHeap(),0,b);
                        } else {
                                /* NULL args - copy formatstr
                                 * (probably wrong)
                                 */
                            while ((lastf<f)&&(*lastf)) {
                                ADD_TO_T(*lastf++);
                            }
                        }
                        RtlFreeHeap(GetProcessHeap(),0,fmtstr);
                        break;
                    case 'n':
                        ADD_TO_T('\r');
                        ADD_TO_T('\n');
                        f++;
                        break;
                    case '0':
                        eos = TRUE;
                        f++;
                        break;
                    default:
                        ADD_TO_T(*f++);
                        break;
                    }
                } else {
                    ch = *f;
                    f++;
                    if (ch == '\r') {
                        if (*f == '\n')
                            f++;
                        if(width)
                            ADD_TO_T(' ');
                        else
                        {
                            ADD_TO_T('\r');
                            ADD_TO_T('\n');
                        }
                    } else {
                        if (ch == '\n')
                        {
                            if(width)
                                ADD_TO_T(' ');
                            else
                            {
                                ADD_TO_T('\r');
                                ADD_TO_T('\n');
                            }
                        }
                        else
                            ADD_TO_T(ch);
                    }
                }
            }
        }
        *t='\0';
    }
    talloced = lstrlenA(target)+1;
    if (nSize && talloced<nSize) {
        target = (char*)RtlReAllocateHeap(RtlGetProcessHeap(),HEAP_ZERO_MEMORY,target,nSize);
    }
    //TRACE("-- %s\n",debugstr_a(target));
    if (dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) {
        *((LPVOID*)lpBuffer) = (LPVOID)LocalAlloc(GMEM_ZEROINIT,max(nSize, talloced));
        memcpy(*(LPSTR*)lpBuffer,target,talloced);
    } else {
        lstrcpynA(lpBuffer,target,nSize);
    }
    RtlFreeHeap(RtlGetProcessHeap(),0,target);
    if (from) RtlFreeHeap(RtlGetProcessHeap(),0,from);
    //TRACE("-- returning %d\n", (dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) ?  lstrlenA(*(LPSTR*)lpBuffer):lstrlenA(lpBuffer));
    return (dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) ?
        lstrlenA(*(LPSTR*)lpBuffer):
            lstrlenA(lpBuffer);
#else
    FIXME("FormatMessageA: unimplemented\n");
    return 0;
#endif /* __i386__ */
}
#undef ADD_TO_T


/***********************************************************************
 *           FormatMessageW   (KERNEL32.@)
 *
 * @implemented
 */
DWORD WINAPI FormatMessageW(
        DWORD   dwFlags,
        LPCVOID lpSource,
        DWORD   dwMessageId,
        DWORD   dwLanguageId,
        LPWSTR  lpBuffer,
        DWORD   nSize,
        va_list* _args)
{
    LPDWORD args=(LPDWORD)_args;
#if defined(__i386__) || defined(__sparc__)
/* This implementation is completely dependant on the format of the va_list on x86 CPUs */
    LPSTR target,t;
    DWORD talloced;
    LPSTR from,f;
    DWORD width = dwFlags & FORMAT_MESSAGE_MAX_WIDTH_MASK;
    BOOL eos = FALSE;
    INT bufsize;
    HMODULE hmodule = (HMODULE)lpSource;
    CHAR ch;

    //TRACE("(0x%lx,%p,%ld,0x%lx,%p,%ld,%p)\n", dwFlags,lpSource,dwMessageId,dwLanguageId,lpBuffer,nSize,args);
    if ((dwFlags & FORMAT_MESSAGE_FROM_STRING)
        &&((dwFlags & FORMAT_MESSAGE_FROM_SYSTEM)
           || (dwFlags & FORMAT_MESSAGE_FROM_HMODULE))) return 0;

    if (width && width != FORMAT_MESSAGE_MAX_WIDTH_MASK) {
        //FIXME("line wrapping not supported.\n");
    }
    from = NULL;
    if (dwFlags & FORMAT_MESSAGE_FROM_STRING) {
        from = (LPSTR)HEAP_strdupWtoA(GetProcessHeap(),0,(LPWSTR)lpSource);
    }
    else {
        bufsize = 0;

        if (dwFlags & FORMAT_MESSAGE_FROM_HMODULE)
        {
           bufsize=load_messageA(hmodule,dwMessageId,dwLanguageId,NULL,100);
           if ((!bufsize) && (!dwLanguageId)) {
                bufsize=load_messageA(hmodule,dwMessageId,
                                      MAKELANGID(LANG_NEUTRAL,SUBLANG_NEUTRAL),NULL,100);
                if (!bufsize) bufsize=load_messageA(hmodule,dwMessageId,
                                                    MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),NULL,100);
                if (!bufsize) bufsize=load_messageA(hmodule,dwMessageId,
                                                    MAKELANGID(LANG_NEUTRAL,SUBLANG_SYS_DEFAULT),NULL,100);
                if (!bufsize) bufsize=load_messageA(hmodule,dwMessageId,
                                                    MAKELANGID(LANG_NEUTRAL,SUBLANG_SYS_DEFAULT),NULL,100);
                if (!bufsize) bufsize=load_messageA(hmodule,dwMessageId,
                                                    MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US),NULL,100);
            }
        }
        if ((dwFlags & FORMAT_MESSAGE_FROM_SYSTEM) && (!bufsize))
        {
           hmodule = GetModuleHandleA("kernel32");
           bufsize=load_messageA(hmodule,dwMessageId,dwLanguageId,NULL,100);
           if ((!bufsize) && (!dwLanguageId)) {
                bufsize=load_messageA(hmodule,dwMessageId,
                                      MAKELANGID(LANG_NEUTRAL,SUBLANG_NEUTRAL),NULL,100);
                if (!bufsize) bufsize=load_messageA(hmodule,dwMessageId,
                                                    MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),NULL,100);
                if (!bufsize) bufsize=load_messageA(hmodule,dwMessageId,
                                                    MAKELANGID(LANG_NEUTRAL,SUBLANG_SYS_DEFAULT),NULL,100);
                if (!bufsize) bufsize=load_messageA(hmodule,dwMessageId,
                                                    MAKELANGID(LANG_NEUTRAL,SUBLANG_SYS_DEFAULT),NULL,100);
                if (!bufsize) bufsize=load_messageA(hmodule,dwMessageId,
                                                    MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US),NULL,100);
            }
        }

        if (!bufsize) {
            TRACE("FormatMessageW: dwFlags=%#x hmodule=%#x dwMessageId=%#x - could not load message\n", dwFlags, hmodule, dwMessageId);
            SetLastError (ERROR_RESOURCE_LANG_NOT_FOUND);
            return 0;
        }

        from = RtlAllocateHeap(RtlGetProcessHeap(), 0, bufsize + 1 );
        load_messageA(hmodule,dwMessageId,dwLanguageId,from,bufsize+1);
    }
    target = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, 100 );
    t = target;
    talloced= 100;

#define ADD_TO_T(c)  do {\
    *t++=c;\
    if ((DWORD)((ULONG_PTR)t-(ULONG_PTR)target) == talloced) {\
        target = (char*)RtlReAllocateHeap(RtlGetProcessHeap(),HEAP_ZERO_MEMORY,target,talloced*2);\
        t = target+talloced;\
        talloced*=2;\
    } \
} while (0)

    if (from) {
        f=from;
        if (dwFlags & FORMAT_MESSAGE_IGNORE_INSERTS) {
            while (*f && !eos)
                ADD_TO_T(*f++);
        }
        else {
            while (*f && !eos) {
                if (*f=='%') {
                    int insertnr;
                    char *fmtstr,*sprintfbuf,*x;
                    DWORD *argliststart;

                    fmtstr = NULL;
                    f++;
                    if (!*f) {
                        ADD_TO_T('%');
                        continue;
                    }

                    switch (*f) {
                    case '1':case '2':case '3':case '4':case '5':
                    case '6':case '7':case '8':case '9':
                        insertnr=*f-'0';
                        switch (f[1]) {
                        case '0':case '1':case '2':case '3':
                        case '4':case '5':case '6':case '7':
                        case '8':case '9':
                            f++;
                            insertnr=insertnr*10+*f-'0';
                            f++;
                            break;
                        default:
                            f++;
                            break;
                        }
                        if (*f=='!') {
                            f++;
                            if (NULL!=(x=strchr(f,'!'))) {
                                *x='\0';
                                fmtstr=RtlAllocateHeap(RtlGetProcessHeap(), 0, lstrlenA(f)+2);
                                sprintf(fmtstr,"%%%s",f);
                                f=x+1;
                            } else {
                                fmtstr=RtlAllocateHeap(RtlGetProcessHeap(),0,lstrlenA(f));
                                sprintf(fmtstr,"%%%s",f);
                                f+=lstrlenA(f); /*at \0*/
                            }
                        } else {
                            if(!args) break;
                            fmtstr = RtlAllocateHeap(RtlGetProcessHeap(),0,3);
                            strcpy( fmtstr, "%s" );
                        }
                        if (dwFlags & FORMAT_MESSAGE_ARGUMENT_ARRAY)
                            argliststart=args+insertnr-1;
                        else
                            argliststart=(*(DWORD**)args)+insertnr-1;

                        if (fmtstr[lstrlenA(fmtstr)-1]=='s' && argliststart[0]) {
                            DWORD xarr[3];

                            xarr[0]=(DWORD)HEAP_strdupWtoA(RtlGetProcessHeap(),0,(LPWSTR)(*(argliststart+0)));
                            /* possible invalid pointers */
                            xarr[1]=*(argliststart+1);
                            xarr[2]=*(argliststart+2);
                            sprintfbuf=RtlAllocateHeap(RtlGetProcessHeap(),0,lstrlenW((LPWSTR)argliststart[0])*2+1);

                            /* CMF - This makes a BIG assumption about va_list */
                            vsprintf(sprintfbuf, fmtstr, (va_list) xarr);
                            RtlFreeHeap(RtlGetProcessHeap(), 0, (LPVOID) xarr[0]);
                        } else {
                            sprintfbuf=RtlAllocateHeap(RtlGetProcessHeap(),0,100);

                            /* CMF - This makes a BIG assumption about va_list */
                            vsprintf(sprintfbuf, fmtstr, (va_list) argliststart);
                        }
                        x=sprintfbuf;
                        while (*x) {
                            ADD_TO_T(*x++);
                        }
                        RtlFreeHeap(RtlGetProcessHeap(),0,sprintfbuf);
                        RtlFreeHeap(RtlGetProcessHeap(),0,fmtstr);
                        break;
                    case 'n':
                        ADD_TO_T('\r');
                        ADD_TO_T('\n');
                        f++;
                        break;
                    case '0':
                        eos = TRUE;
                        f++;
                        break;
                    default:
                        ADD_TO_T(*f++);
                        break;
                    }
                } else {
                    ch = *f;
                    f++;
                    if (ch == '\r') {
                        if (*f == '\n')
                            f++;
                        if(width)
                            ADD_TO_T(' ');
                        else
                        {
                            ADD_TO_T('\r');
                            ADD_TO_T('\n');
                        }
                    } else {
                        if (ch == '\n')
                        {
                            if(width)
                                ADD_TO_T(' ');
                            else
                            {
                                ADD_TO_T('\r');
                                ADD_TO_T('\n');
                            }
                        }
                        else
                            ADD_TO_T(ch);
                    }
                }
            }
        }
        *t='\0';
    }
    talloced = lstrlenA(target)+1;
    if (nSize && talloced<nSize)
        target = (char*)RtlReAllocateHeap(RtlGetProcessHeap(),HEAP_ZERO_MEMORY,target,nSize);
    if (dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) {
        /* nSize is the MINIMUM size */
        DWORD len = MultiByteToWideChar( CP_ACP, 0, target, -1, NULL, 0 );
        *((LPVOID*)lpBuffer) = (LPVOID)LocalAlloc(GMEM_ZEROINIT,len*sizeof(WCHAR));
        MultiByteToWideChar( CP_ACP, 0, target, -1, *(LPWSTR*)lpBuffer, len );
    }
    else
    {
        if (nSize > 0 && !MultiByteToWideChar( CP_ACP, 0, target, -1, lpBuffer, nSize ))
            lpBuffer[nSize-1] = 0;
    }
    RtlFreeHeap(RtlGetProcessHeap(),0,target);
    if (from) RtlFreeHeap(RtlGetProcessHeap(),0,from);
    return (dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) ?
        lstrlenW(*(LPWSTR*)lpBuffer):
            lstrlenW(lpBuffer);
#else
    FIXME("FormatMessageW: unimplemented\n");
    return 0;
#endif /* __i386__ */
}
#undef ADD_TO_T
