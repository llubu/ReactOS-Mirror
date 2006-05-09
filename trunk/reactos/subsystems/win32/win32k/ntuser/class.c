/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998 - 2006 ReactOS Team
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
/* $Id$
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Window classes
 * FILE:             subsys/win32k/ntuser/class.c
 * PROGRAMER:        Thomas Weidenmueller <w3seek@reactos.com>
 * REVISION HISTORY:
 *       06-06-2001  CSH  Created
 */
/* INCLUDES ******************************************************************/

#include <w32k.h>

#define NDEBUG
#include <debug.h>

/* WINDOWCLASS ***************************************************************/

static VOID
IntFreeClassMenuName(IN OUT PWINDOWCLASS Class)
{
    /* free the menu name, if it was changed and allocated */
    if (Class->MenuName != NULL && !IS_INTRESOURCE(Class->MenuName) &&
        Class->MenuName != (PWSTR)(Class + 1))
    {
        UserHeapFree(Class->MenuName);
        Class->MenuName = NULL;
        Class->AnsiMenuName = NULL;
    }
}

static VOID
IntDestroyClass(IN OUT PWINDOWCLASS Class)
{
    /* there shouldn't be any clones anymore */
    ASSERT(Class->Windows == 0);
    ASSERT(Class->Clone == NULL);

    if (Class->Base == Class)
    {
        /* destruct resources shared with clones */
        if (!Class->System && Class->CallProc != NULL)
        {
            DestroyCallProc(Class->GlobalCallProc ? NULL : Class->Desktop,
                            Class->CallProc);
        }

        if (Class->CallProc2 != NULL)
        {
            DestroyCallProc(Class->GlobalCallProc2 ? NULL : Class->Desktop,
                            Class->CallProc2);
        }

        IntFreeClassMenuName(Class);
    }

    /* free the structure */
    if (Class->Desktop != NULL)
    {
        DesktopHeapFree(Class->Desktop,
                        Class);
    }
    else
    {
        UserHeapFree(Class);
    }
}


/* clean all process classes. all process windows must cleaned first!! */
void FASTCALL DestroyProcessClasses(PW32PROCESS Process )
{
    PWINDOWCLASS Class;
    PW32PROCESSINFO pi = Process->ProcessInfo;

    if (pi != NULL)
    {
        /* free all local classes */
        Class = pi->LocalClassList;
        while (Class != NULL)
        {
            pi->LocalClassList = Class->Next;

            ASSERT(Class->Base == Class);
            IntDestroyClass(Class);

            Class = pi->LocalClassList;
        }

        /* free all global classes */
        Class = pi->GlobalClassList;
        while (Class != NULL)
        {
            pi->GlobalClassList = Class->Next;

            ASSERT(Class->Base == Class);
            IntDestroyClass(Class);

            Class = pi->GlobalClassList;
        }

        /* free all system classes */
        Class = pi->SystemClassList;
        while (Class != NULL)
        {
            pi->SystemClassList = Class->Next;

            ASSERT(Class->Base == Class);
            IntDestroyClass(Class);

            Class = pi->SystemClassList;
        }
    }
}

static BOOL
IntRegisterClassAtom(IN PUNICODE_STRING ClassName,
                     OUT RTL_ATOM *pAtom)
{
    WCHAR szBuf[65];
    PWSTR AtomName;
    NTSTATUS Status;

    if (ClassName->Length != 0)
    {
        /* FIXME - Don't limit to 64 characters! use SEH when allocating memory! */
        if (ClassName->Length / sizeof(WCHAR) >= sizeof(szBuf) / sizeof(szBuf[0]))
        {
            SetLastWin32Error(ERROR_INVALID_PARAMETER);
            return (RTL_ATOM)0;
        }

        RtlCopyMemory(szBuf,
                      ClassName->Buffer,
                      ClassName->Length);
        szBuf[ClassName->Length / sizeof(WCHAR)] = UNICODE_NULL;
        AtomName = szBuf;
    }
    else
        AtomName = ClassName->Buffer;

    Status = RtlAddAtomToAtomTable(gAtomTable,
                                   AtomName,
                                   pAtom);

    if (!NT_SUCCESS(Status))
    {
        SetLastNtError(Status);
        return FALSE;
    }

    return TRUE;
}

static VOID
IntDeregisterClassAtom(IN RTL_ATOM Atom)
{
    RtlDeleteAtomFromAtomTable(gAtomTable,
                               Atom);
}

static BOOL
IntSetClassAtom(IN OUT PWINDOWCLASS Class,
                IN PUNICODE_STRING ClassName)
{
    RTL_ATOM Atom = (RTL_ATOM)0;

    /* update the base class first */
    Class = Class->Base;

    if (!IntRegisterClassAtom(ClassName,
                              &Atom))
    {
        return FALSE;
    }

    IntDeregisterClassAtom(Class->Atom);

    Class->Atom = Atom;

    /* update the clones */
    Class = Class->Clone;
    while (Class != NULL)
    {
        Class->Atom = Atom;

        Class = Class->Next;
    }

    return TRUE;
}

static WNDPROC
IntGetClassWndProc(IN PWINDOWCLASS Class,
                   IN PW32PROCESSINFO pi,
                   IN BOOL Ansi,
                   IN BOOL UseCallProc2)
{
    /* FIXME - assert for exclusive lock! */

    if (Class->System)
    {
        return (Ansi ? Class->WndProcExtra : Class->WndProc);
    }
    else
    {
        if (!Ansi == Class->Unicode)
        {
            return Class->WndProc;
        }
        else
        {
            PCALLPROC *CallProcPtr;
            PWINDOWCLASS BaseClass;

            /* make sure the call procedures are located on the desktop
               of the base class! */
            BaseClass = Class->Base;
            Class = BaseClass;

            CallProcPtr = (UseCallProc2 ? &Class->CallProc2 : &Class->CallProc);

            if (*CallProcPtr != NULL)
            {
                return GetCallProcHandle(*CallProcPtr);
            }
            else
            {
                PCALLPROC NewCallProc;

                if (pi == NULL)
                    return NULL;

                NewCallProc = CreateCallProc(Class->Desktop,
                                             Class->WndProc,
                                             Class->Unicode,
                                             pi);
                if (NewCallProc == NULL)
                {
                    SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
                    return NULL;
                }

                *CallProcPtr = NewCallProc;

                if (Class->Desktop == NULL)
                {
                    if (UseCallProc2)
                        Class->GlobalCallProc2 = TRUE;
                    else
                        Class->GlobalCallProc = TRUE;
                }

                /* update the clones */
                Class = Class->Clone;
                while (Class != NULL)
                {
                    if (UseCallProc2)
                    {
                        Class->CallProc2 = NewCallProc;
                        Class->GlobalCallProc2 = BaseClass->GlobalCallProc2;
                    }
                    else
                    {
                        Class->CallProc = NewCallProc;
                        Class->GlobalCallProc = BaseClass->GlobalCallProc;
                    }

                    Class = Class->Next;
                }

                return GetCallProcHandle(NewCallProc);
            }
        }
    }
}

static WNDPROC
IntSetClassWndProc(IN OUT PWINDOWCLASS Class,
                   IN WNDPROC WndProc,
                   IN BOOL Ansi)
{
    WNDPROC Ret;

    if (Class->System)
    {
        DPRINT1("Attempted to change window procedure of system window class 0x%p!\n", Class->Atom);
        SetLastWin32Error(ERROR_ACCESS_DENIED);
        return NULL;
    }

    /* update the base class first */
    Class = Class->Base;

    /* resolve any callproc handle if possible */
    if (IsCallProcHandle(WndProc))
    {
        WNDPROC_INFO wpInfo;

        if (UserGetCallProcInfo((HANDLE)WndProc,
                                &wpInfo))
        {
            WndProc = wpInfo.WindowProc;
            /* FIXME - what if wpInfo.IsUnicode doesn't match Ansi? */
        }
    }

    Ret = IntGetClassWndProc(Class,
                             GetW32ProcessInfo(),
                             Ansi,
                             TRUE);
    if (Ret == NULL)
    {
        return NULL;
    }

    /* update the class info */
    Class->Unicode = !Ansi;
    Class->WndProc = WndProc;
    if (Class->CallProc != NULL)
    {
        Class->CallProc->WndProc = WndProc;
        Class->CallProc->Unicode = !Ansi;
    }

    /* update the clones */
    Class = Class->Clone;
    while (Class != NULL)
    {
        Class->Unicode = !Ansi;
        Class->WndProc = WndProc;

        Class = Class->Next;
    }

    return Ret;
}

static PWINDOWCLASS
IntGetClassForDesktop(IN OUT PWINDOWCLASS BaseClass,
                      IN OUT PWINDOWCLASS *ClassLink,
                      IN PDESKTOP Desktop)
{
    SIZE_T ClassSize;
    PWINDOWCLASS Class;

    ASSERT(Desktop != NULL);
    ASSERT(BaseClass->Base == BaseClass);

    if (BaseClass->Desktop == Desktop)
    {
        /* it is most likely that a window is created on the same
           desktop as the window class. */

        return BaseClass;
    }

    if (BaseClass->Desktop == NULL)
    {
        ASSERT(BaseClass->Windows == 0);
        ASSERT(BaseClass->Clone == NULL);

        /* Classes are also located in the shared heap when the class
           was created before the thread attached to a desktop. As soon
           as a window is created for such a class located on the shared
           heap, the class is cloned into the desktop heap on which the
           window is created. */
        Class = NULL;
    }
    else
    {
        /* The user is asking for a class object on a different desktop,
           try to find one! */
        Class = BaseClass->Clone;
        while (Class != NULL)
        {
            if (Class->Desktop == Desktop)
            {
                ASSERT(Class->Base == BaseClass);
                ASSERT(Class->Clone == NULL);
                break;
            }

            Class = Class->Next;
        }
    }

    if (Class == NULL)
    {
        /* The window is created on a different desktop, we need to
           clone the class object to the desktop heap of the window! */
        ClassSize = (SIZE_T)BaseClass->ClassExtraDataOffset +
                    (SIZE_T)BaseClass->ClsExtra;

        Class = DesktopHeapAlloc(Desktop,
                                 ClassSize);
        if (Class != NULL)
        {
            /* simply clone the class */
            RtlCopyMemory(Class,
                          BaseClass,
                          ClassSize);

            /* update some pointers and link the class */
            Class->Desktop = Desktop;
            Class->Windows = 0;

            if (BaseClass->Desktop == NULL)
            {
                /* we don't really need the base class on the shared
                   heap anymore, delete it so the only class left is
                   the clone we just created, which now serves as the
                   new base class */
                ASSERT(BaseClass->Clone == NULL);
                ASSERT(Class->Clone == NULL);
                Class->Base = Class;
                Class->Next = BaseClass->Next;

                if (!BaseClass->System && BaseClass->CallProc != NULL)
                    Class->GlobalCallProc = TRUE;
                if (BaseClass->CallProc2 != NULL)
                    Class->GlobalCallProc2 = TRUE;

                /* replace the base class */
                (void)InterlockedExchangePointer(ClassLink,
                                                 Class);

                /* destroy the obsolete copy on the shared heap */
                BaseClass->Base = NULL;
                BaseClass->Clone = NULL;
                IntDestroyClass(BaseClass);
            }
            else
            {
                /* link in the clone */
                Class->Clone = NULL;
                Class->Base = BaseClass;
                Class->Next = BaseClass->Clone;
                (void)InterlockedExchangePointer(&BaseClass->Clone,
                                                 Class);
            }
        }
        else
        {
            SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
        }
    }

    return Class;
}

PWINDOWCLASS
IntReferenceClass(IN OUT PWINDOWCLASS BaseClass,
                  IN OUT PWINDOWCLASS *ClassLink,
                  IN PDESKTOP Desktop)
{
    PWINDOWCLASS Class;

    ASSERT(BaseClass->Base == BaseClass);

    Class = IntGetClassForDesktop(BaseClass,
                                  ClassLink,
                                  Desktop);
    if (Class != NULL)
    {
        Class->Windows++;
    }

    return Class;
}

static VOID
IntMakeCloneBaseClass(IN OUT PWINDOWCLASS Class,
                      IN OUT PWINDOWCLASS *BaseClassLink,
                      IN OUT PWINDOWCLASS *CloneLink)
{
    PWINDOWCLASS Clone, BaseClass;
    PCALLPROC CallProc;

    ASSERT(Class->Base != Class);
    ASSERT(Class->Base->Clone != NULL);
    ASSERT(Class->Desktop != NULL);
    ASSERT(Class->Windows != 0);
    ASSERT(Class->Base->Desktop != NULL);
    ASSERT(Class->Base->Windows == 0);

    /* unlink the clone */
    *CloneLink = Class->Next;
    Class->Clone = Class->Base->Clone;

    BaseClass = Class->Base;

    if (!BaseClass->System && BaseClass->CallProc != NULL &&
        !BaseClass->GlobalCallProc)
    {
        /* we need to move the allocated call procedure */
        CallProc = BaseClass->CallProc;
        Class->CallProc = CloneCallProc(Class->Desktop,
                                        CallProc);
        DestroyCallProc(BaseClass->Desktop,
                        CallProc);
    }

    if (BaseClass->CallProc2 != NULL &&
        !BaseClass->GlobalCallProc2)
    {
        /* we need to move the allocated call procedure */
        CallProc = BaseClass->CallProc2;
        Class->CallProc2 = CloneCallProc(Class->Desktop,
                                         CallProc);
        DestroyCallProc(BaseClass->Desktop,
                        CallProc);
    }

    /* update the class information to make it a base class */
    Class->Base = Class;
    Class->Next = (*BaseClassLink)->Next;

    /* update all clones */
    Clone = Class->Clone;
    while (Clone != NULL)
    {
        ASSERT(Clone->Clone == NULL);
        Clone->Base = Class;

        if (!Class->System)
            Clone->CallProc = Class->CallProc;
        Clone->CallProc2 = Class->CallProc2;

        Clone = Clone->Next;
    }

    /* link in the new base class */
    (void)InterlockedExchangePointer(BaseClassLink,
                                     Class);
}

VOID
IntDereferenceClass(IN OUT PWINDOWCLASS Class,
                    IN PDESKTOP Desktop,
                    IN PW32PROCESSINFO pi)
{
    PWINDOWCLASS *PrevLink, BaseClass, CurrentClass;

    BaseClass = Class->Base;

    if (--Class->Windows == 0)
    {
        if (BaseClass == Class)
        {
            ASSERT(Class->Base == Class);

            /* check if there are clones of the class on other desktops,
               link the first clone in if possible. If there are no clones
               then leave the class on the desktop heap. It will get moved
               to the shared heap when the thread detaches. */
            if (BaseClass->Clone != NULL)
            {
                if (BaseClass->System)
                    PrevLink = &pi->SystemClassList;
                else if (BaseClass->Global)
                    PrevLink = &pi->GlobalClassList;
                else
                    PrevLink = &pi->LocalClassList;

                while (*PrevLink != BaseClass)
                {
                    ASSERT(BaseClass != NULL);
                    PrevLink = &BaseClass->Next;
                }

                ASSERT(*PrevLink == BaseClass);

                /* make the first clone become the new base class */
                IntMakeCloneBaseClass(BaseClass->Clone,
                                      PrevLink,
                                      &BaseClass->Clone);

                /* destroy the class, there's still another clone of the class
                   that now serves as a base class. Make sure we don't destruct
                   resources shared by all classes (Base = NULL)! */
                BaseClass->Base = NULL;
                BaseClass->Clone = NULL;
                IntDestroyClass(BaseClass);
            }
        }
        else
        {
            /* locate the cloned class and unlink it */
            PrevLink = &BaseClass->Clone;
            CurrentClass = BaseClass->Clone;
            while (CurrentClass != Class)
            {
                ASSERT(CurrentClass != NULL);

                PrevLink = &CurrentClass->Next;
                CurrentClass = CurrentClass->Next;
            }

            ASSERT(CurrentClass == Class);

            (void)InterlockedExchangePointer(PrevLink,
                                             Class->Next);

            ASSERT(Class->Base == BaseClass);
            ASSERT(Class->Clone == NULL);

            /* the class was just a clone, we don't need it anymore */
            IntDestroyClass(Class);
        }
    }
}

static BOOL
IntMoveClassToSharedHeap(IN OUT PWINDOWCLASS Class,
                         IN OUT PWINDOWCLASS **ClassLinkPtr)
{
    PWINDOWCLASS NewClass;
    PCALLPROC CallProc;
    SIZE_T ClassSize;

    ASSERT(Class->Base == Class);
    ASSERT(Class->Desktop != NULL);
    ASSERT(Class->Windows == 0);
    ASSERT(Class->Clone == NULL);

    ClassSize = (SIZE_T)Class->ClassExtraDataOffset +
                (SIZE_T)Class->ClsExtra;

    /* allocate the new base class on the shared heap */
    NewClass = UserHeapAlloc(ClassSize);
    if (NewClass != NULL)
    {
        RtlCopyMemory(NewClass,
                      Class,
                      ClassSize);

        NewClass->Desktop = NULL;
        NewClass->Base = NewClass;

        if (Class->MenuName == (PWSTR)(Class + 1))
        {
            ULONG_PTR AnsiDelta = (ULONG_PTR)Class->AnsiMenuName - (ULONG_PTR)Class->MenuName;

            /* fixup the self-relative MenuName pointers */
            NewClass->MenuName = (PWSTR)(NewClass + 1);
            NewClass->AnsiMenuName = (PSTR)((ULONG_PTR)NewClass->MenuName + AnsiDelta);
        }

        if (!NewClass->System && NewClass->CallProc != NULL &&
            !NewClass->GlobalCallProc)
        {
            /* we need to move the allocated call procedure to the shared heap */
            CallProc = NewClass->CallProc;
            NewClass->CallProc = CloneCallProc(NULL,
                                               CallProc);
            DestroyCallProc(Class->Desktop,
                            CallProc);

            NewClass->GlobalCallProc = TRUE;
        }

        if (NewClass->CallProc2 != NULL &&
            !NewClass->GlobalCallProc2)
        {
            /* we need to move the allocated call procedure to the shared heap */
            CallProc = NewClass->CallProc2;
            NewClass->CallProc2 = CloneCallProc(NULL,
                                                CallProc);
            DestroyCallProc(Class->Desktop,
                            CallProc);

            NewClass->GlobalCallProc2 = TRUE;
        }

        /* replace the class in the list */
        (void)InterlockedExchangePointer(*ClassLinkPtr,
                                         NewClass);
        *ClassLinkPtr = &NewClass->Next;

        /* free the obsolete class on the desktop heap */
        Class->Base = NULL;
        IntDestroyClass(Class);
        return TRUE;
    }

    return FALSE;
}

static VOID
IntCheckDesktopClasses(IN PDESKTOP Desktop,
                       IN OUT PWINDOWCLASS *ClassList,
                       IN BOOL FreeOnFailure,
                       OUT BOOL *Ret)
{
    PWINDOWCLASS Class, NextClass, *Link;

    /* NOTE: We only need to check base classes! When classes are no longer needed
             on a desktop, the clones will be freed automatically as soon as possible.
             However, we need to move base classes to the shared heap, as soon as
             the last desktop heap where a class is allocated on is about to be destroyed.
             If we didn't move the class to the shared heap, the class would become
             inaccessible! */

    ASSERT(Desktop != NULL);

    Link = ClassList;
    Class = *Link;
    while (Class != NULL)
    {
        NextClass = Class->Next;

        ASSERT(Class->Base == Class);

        if (Class->Desktop == Desktop &&
            Class->Windows == 0)
        {
            /* there shouldn't be any clones around anymore! */
            ASSERT(Class->Clone == NULL);

            /* FIXME - If process is terminating, don't move the class but rather destroy it! */
            /* FIXME - We could move the class to another desktop heap if there's still desktops
                       mapped into the process... */

            /* move the class to the shared heap */
            if (IntMoveClassToSharedHeap(Class,
                                         &Link))
            {
                ASSERT(*Link == NextClass);
            }
            else
            {
                ASSERT(NextClass == Class->Next);

                if (FreeOnFailure)
                {
                    /* unlink the base class */
                    (void)InterlockedExchangePointer(Link,
                                                     Class->Next);

                    /* we can free the old base class now */
                    Class->Base = NULL;
                    IntDestroyClass(Class);
                }
                else
                {
                    Link = &Class->Next;
                    *Ret = FALSE;
                }
            }
        }
        else
            Link = &Class->Next;

        Class = NextClass;
    }
}

BOOL
IntCheckProcessDesktopClasses(IN PDESKTOP Desktop,
                              IN BOOL FreeOnFailure)
{
    PW32PROCESSINFO pi;
    BOOL Ret = TRUE;

    pi = GetW32ProcessInfo();
    if (pi == NULL)
        return TRUE;

    /* check all local classes */
    IntCheckDesktopClasses(Desktop,
                           &pi->LocalClassList,
                           FreeOnFailure,
                           &Ret);

    /* check all global classes */
    IntCheckDesktopClasses(Desktop,
                           &pi->GlobalClassList,
                           FreeOnFailure,
                           &Ret);

    /* check all system classes */
    IntCheckDesktopClasses(Desktop,
                           &pi->SystemClassList,
                           FreeOnFailure,
                           &Ret);

    if (!Ret)
    {
        DPRINT1("Failed to move process classes from desktop 0x%p to the shared heap!\n", Desktop);
        SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
    }

    return Ret;
}

static PWINDOWCLASS
IntCreateClass(IN CONST WNDCLASSEXW* lpwcx,
               IN PUNICODE_STRING ClassName,
               IN PUNICODE_STRING MenuName,
               IN WNDPROC wpExtra,
               IN DWORD dwFlags,
               IN PDESKTOP Desktop,
               IN PW32PROCESSINFO pi)
{
    SIZE_T ClassSize;
    PWINDOWCLASS Class = NULL;
    RTL_ATOM Atom;
    NTSTATUS Status = STATUS_SUCCESS;

    if (!IntRegisterClassAtom(ClassName,
                              &Atom))
    {
        DPRINT1("Failed to register class atom!\n");
        return NULL;
    }

    ClassSize = sizeof(WINDOWCLASS) + lpwcx->cbClsExtra;
    ClassSize += ClassName->Length + sizeof(UNICODE_NULL);
    ClassSize += MenuName->Length + sizeof(UNICODE_NULL);
    if (ClassName->Length != 0)
        ClassSize += RtlUnicodeStringToAnsiSize(ClassName);
    if (MenuName->Length != 0)
        ClassSize += RtlUnicodeStringToAnsiSize(MenuName);

    if (Desktop != NULL)
    {
        Class = DesktopHeapAlloc(Desktop,
                                 ClassSize);
    }
    else
    {
        /* FIXME - the class was created before being connected
                   to a desktop. It is possible for the desktop window,
                   but should it be allowed for any other case? */
        Class = UserHeapAlloc(ClassSize);
    }

    if (Class != NULL)
    {
        RtlZeroMemory(Class,
                      sizeof(ClassSize));

        Class->Desktop = Desktop;
        Class->Base = Class;
        Class->Atom = Atom;

        if (dwFlags & REGISTERCLASS_SYSTEM)
        {
            dwFlags &= ~REGISTERCLASS_ANSI;
            Class->WndProcExtra = wpExtra;
            Class->System = TRUE;
        }

        _SEH_TRY
        {
            PWSTR strBuf;
            PSTR strBufA;
            ANSI_STRING AnsiString;

            /* need to protect with SEH since accessing the WNDCLASSEX structure
               and string buffers might raise an exception! We don't want to
               leak memory... */
            Class->WndProc = lpwcx->lpfnWndProc;
            Class->Style = lpwcx->style;
            Class->ClsExtra = lpwcx->cbClsExtra;
            Class->WndExtra = lpwcx->cbWndExtra;
            Class->hInstance = lpwcx->hInstance;
            Class->hIcon = lpwcx->hIcon; /* FIXME */
            Class->hIconSm = lpwcx->hIconSm; /* FIXME */
            Class->hCursor = lpwcx->hCursor; /* FIXME */
            Class->hbrBackground = lpwcx->hbrBackground;

            /* make a copy of the string */
            strBuf = (PWSTR)(Class + 1);
            if (MenuName->Length != 0)
            {
                Class->MenuName = strBuf;
                RtlCopyMemory(Class->MenuName,
                              MenuName->Buffer,
                              MenuName->Length);

                strBuf += (MenuName->Length / sizeof(WCHAR)) + 1;
            }
            else
                Class->MenuName = MenuName->Buffer;

            /* save an ansi copy of the string */
            strBufA = (PSTR)strBuf;
            if (MenuName->Length != 0)
            {
                Class->AnsiMenuName = strBufA;
                AnsiString.MaximumLength = RtlUnicodeStringToAnsiSize(MenuName);
                AnsiString.Buffer = strBufA;
                Status = RtlUnicodeStringToAnsiString(&AnsiString,
                                                      MenuName,
                                                      FALSE);
                if (!NT_SUCCESS(Status))
                {
                    DPRINT1("Failed to convert unicode menu name to ansi!\n");

                    /* life would've been much prettier if ntoskrnl exported RtlRaiseStatus()... */
                    _SEH_LEAVE;
                }

                strBufA += AnsiString.Length + 1;
            }
            else
                Class->AnsiMenuName = (PSTR)MenuName->Buffer;

            /* calculate the offset of the extra data */
            Class->ClassExtraDataOffset = (ULONG_PTR)strBufA - (ULONG_PTR)Class;

            if (!(dwFlags & REGISTERCLASS_ANSI))
                Class->Unicode = TRUE;

            if (Class->Style & CS_GLOBALCLASS)
                Class->Global = TRUE;
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed creating the class: 0x%x\n", Status);

            SetLastNtError(Status);

            DesktopHeapFree(Desktop,
                            Class);
            Class = NULL;

            IntDeregisterClassAtom(Atom);
        }
    }
    else
    {
        DPRINT1("Failed to allocate class on Desktop 0x%p\n", Desktop);

        IntDeregisterClassAtom(Atom);

        SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
    }

    return Class;
}

static PWINDOWCLASS
IntFindClass(IN RTL_ATOM Atom,
             IN HINSTANCE hInstance,
             IN PWINDOWCLASS *ClassList,
             OUT PWINDOWCLASS **Link  OPTIONAL)
{
    PWINDOWCLASS Class, *PrevLink = ClassList;

    Class = *PrevLink;
    while (Class != NULL)
    {
        if (Class->Atom == Atom &&
            (hInstance == NULL || Class->hInstance == hInstance) &&
            !Class->Destroying)
        {
            ASSERT(Class->Base == Class);

            if (Link != NULL)
                *Link = PrevLink;
            break;
        }

        PrevLink = &Class->Next;
        Class = Class->Next;
    }

    return Class;
}

RTL_ATOM
IntGetClassAtom(IN PUNICODE_STRING ClassName,
                IN HINSTANCE hInstance  OPTIONAL,
                IN PW32PROCESSINFO pi  OPTIONAL,
                OUT PWINDOWCLASS *BaseClass  OPTIONAL,
                OUT PWINDOWCLASS **Link  OPTIONAL)
{
    RTL_ATOM Atom = (RTL_ATOM)0;

    if (ClassName->Length != 0)
    {
        WCHAR szBuf[65];
        PWSTR AtomName;
        NTSTATUS Status;

        /* NOTE: Caller has to protect the call with SEH! */

        if (ClassName->Length != 0)
        {
            /* FIXME - Don't limit to 64 characters! use SEH when allocating memory! */
            if (ClassName->Length / sizeof(WCHAR) >= sizeof(szBuf) / sizeof(szBuf[0]))
            {
                SetLastWin32Error(ERROR_INVALID_PARAMETER);
                return (RTL_ATOM)0;
            }

            /* We need to make a local copy of the class name! The caller could
               modify the buffer and we could overflow in RtlLookupAtomInAtomTable.
               We're protected by SEH, but the ranges that might be accessed were
               not probed... */
            RtlCopyMemory(szBuf,
                          ClassName->Buffer,
                          ClassName->Length);
            szBuf[ClassName->Length / sizeof(WCHAR)] = UNICODE_NULL;
            AtomName = szBuf;
        }
        else
            AtomName = ClassName->Buffer;

        /* lookup the atom */
        Status = RtlLookupAtomInAtomTable(gAtomTable,
                                          AtomName,
                                          &Atom);
        if (!NT_SUCCESS(Status))
        {
            if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
            {
                SetLastWin32Error(ERROR_CANNOT_FIND_WND_CLASS);
            }
            else
            {
                SetLastNtError(Status);
            }
        }
    }
    else
    {
        ASSERT(IS_ATOM(ClassName->Buffer));
        Atom = (RTL_ATOM)((ULONG_PTR)ClassName->Buffer);
    }

    if (BaseClass != NULL && Atom != (RTL_ATOM)0)
    {
        PWINDOWCLASS Class;

        /* attempt to locate the class object */

        ASSERT(pi != NULL);

        /* Step 1: try to find an exact match of locally registered classes */
        Class = IntFindClass(Atom,
                             hInstance,
                             &pi->LocalClassList,
                             Link);
        if (Class != NULL)
        {
            goto FoundClass;
        }

        /* Step 2: try to find any globally registered class. The hInstance
                   is not relevant for global classes */
        Class = IntFindClass(Atom,
                             NULL,
                             &pi->GlobalClassList,
                             Link);
        if (Class != NULL)
        {
            goto FoundClass;
        }

        /* Step 3: try to find a system class */
        Class = IntFindClass(Atom,
                             NULL,
                             &pi->SystemClassList,
                             Link);

        if (Class == NULL)
        {
            SetLastWin32Error(ERROR_CLASS_DOES_NOT_EXIST);
            return (RTL_ATOM)0;
        }

FoundClass:
        *BaseClass = Class;
    }

    return Atom;
}

RTL_ATOM
UserRegisterClass(IN CONST WNDCLASSEXW* lpwcx,
                  IN PUNICODE_STRING ClassName,
                  IN PUNICODE_STRING MenuName,
                  IN HANDLE hMenu, /* FIXME */
                  IN WNDPROC wpExtra,
                  IN DWORD dwFlags)
{
    PW32THREADINFO ti;
    PW32PROCESSINFO pi;
    PWINDOWCLASS Class;
    RTL_ATOM ClassAtom;
    RTL_ATOM Ret = (RTL_ATOM)0;

    /* NOTE: Accessing the buffers in ClassName and MenuName may raise exceptions! */

    ti = GetW32ThreadInfo();
    if (ti == NULL)
    {
        SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
        return (RTL_ATOM)0;
    }

    pi = ti->kpi;

    /* try to find a previously registered class */
    ClassAtom = IntGetClassAtom(ClassName,
                                NULL,
                                NULL,
                                NULL,
                                NULL);
    if (ClassAtom != (RTL_ATOM)0)
    {
        Class = IntFindClass(ClassAtom,
                             lpwcx->hInstance,
                             &pi->LocalClassList,
                             NULL);
        if (Class != NULL)
        {
            goto ClassAlreadyExists;
        }

        /* if CS_GLOBALCLASS is set, try to find a previously registered global class.
           Re-registering system classes as global classes seems to be allowed,
           so we don't fail */
        if (lpwcx->style & CS_GLOBALCLASS)
        {
            Class = IntFindClass(ClassAtom,
                                 NULL,
                                 ((dwFlags & REGISTERCLASS_SYSTEM) ?
                                     &pi->SystemClassList : &pi->GlobalClassList),
                                 NULL);
            if (Class != NULL)
            {
ClassAlreadyExists:
                DPRINT1("Class 0x%p does already exist!\n", ClassAtom);
                SetLastWin32Error(ERROR_CLASS_ALREADY_EXISTS);
                return (RTL_ATOM)0;
            }
        }
    }

    Class = IntCreateClass(lpwcx,
                           ClassName,
                           MenuName,
                           wpExtra,
                           dwFlags,
                           ti->Desktop,
                           pi);

    if (Class != NULL)
    {
        PWINDOWCLASS *List;

        /* FIXME - pass the PMENU pointer to IntCreateClass instead! */
        Class->hMenu = hMenu;

        /* Register the class */
        if (Class->System)
            List = &pi->SystemClassList;
        else if (Class->Global)
            List = &pi->GlobalClassList;
        else
            List = &pi->LocalClassList;

        Class->Next = *List;
        (void)InterlockedExchangePointer(List,
                                         Class);

        Ret = Class->Atom;
    }

    return Ret;
}

BOOL
UserUnregisterClass(IN PUNICODE_STRING ClassName,
                    IN HINSTANCE hInstance)
{
    PWINDOWCLASS *Link;
    PW32PROCESSINFO pi;
    RTL_ATOM ClassAtom;
    PWINDOWCLASS Class;

    pi = GetW32ProcessInfo();
    if (pi == NULL)
    {
        SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    /* NOTE: Accessing the buffer in ClassName may raise an exception! */
    ClassAtom = IntGetClassAtom(ClassName,
                                hInstance,
                                pi,
                                &Class,
                                &Link);
    if (ClassAtom == (RTL_ATOM)0)
    {
        return FALSE;
    }

    ASSERT(Class != NULL);

    if (Class->System)
    {
        DPRINT1("Attempted to unregister system class 0x%p!\n", ClassAtom);
        SetLastWin32Error(ERROR_ACCESS_DENIED);
        return FALSE;
    }

    if (Class->Windows != 0 ||
        Class->Clone != NULL)
    {
        SetLastWin32Error(ERROR_CLASS_HAS_WINDOWS);
        return FALSE;
    }

    /* must be a base class! */
    ASSERT(Class->Base == Class);

    /* unlink the class */
    *Link = Class->Next;

    IntDeregisterClassAtom(Class->Atom);

    /* finally free the resources */
    IntDestroyClass(Class);
    return TRUE;
}

INT
UserGetClassName(IN PWINDOWCLASS Class,
                 IN OUT PUNICODE_STRING ClassName,
                 IN BOOL Ansi)
{
    NTSTATUS Status = STATUS_SUCCESS;
    WCHAR szStaticTemp[32];
    PWSTR szTemp = NULL;
    ULONG BufLen = sizeof(szStaticTemp);
    INT Ret = 0;

    /* Note: Accessing the buffer in ClassName may raise an exception! */

    _SEH_TRY
    {
        if (Ansi)
        {
            PANSI_STRING AnsiClassName = (PANSI_STRING)ClassName;
            UNICODE_STRING UnicodeClassName;

            /* limit the size of the static buffer on the stack to the
               size of the buffer provided by the caller */
            if (BufLen / sizeof(WCHAR) > AnsiClassName->MaximumLength)
            {
                BufLen = AnsiClassName->MaximumLength * sizeof(WCHAR);
            }

            /* find out how big the buffer needs to be */
            Status = RtlQueryAtomInAtomTable(gAtomTable,
                                             Class->Atom,
                                             NULL,
                                             NULL,
                                             szStaticTemp,
                                             &BufLen);
            if (Status == STATUS_BUFFER_TOO_SMALL)
            {
                if (BufLen / sizeof(WCHAR) > AnsiClassName->MaximumLength)
                {
                    /* the buffer required exceeds the ansi buffer provided,
                       pretend like we're using the ansi buffer and limit the
                       size to the buffer size provided */
                    BufLen = AnsiClassName->MaximumLength * sizeof(WCHAR);
                }

                /* allocate a temporary buffer that can hold the unicode class name */
                szTemp = ExAllocatePool(PagedPool,
                                        BufLen);
                if (szTemp == NULL)
                {
                    SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
                    _SEH_LEAVE;
                }

                /* query the class name */
                Status = RtlQueryAtomInAtomTable(gAtomTable,
                                                 Class->Atom,
                                                 NULL,
                                                 NULL,
                                                 szTemp,
                                                 &BufLen);
            }
            else
                szTemp = szStaticTemp;

            if (NT_SUCCESS(Status))
            {
                /* convert the atom name to ansi */

                RtlInitUnicodeString(&UnicodeClassName,
                                     szTemp);

                Status = RtlUnicodeStringToAnsiString(AnsiClassName,
                                                      &UnicodeClassName,
                                                      FALSE);
                if (!NT_SUCCESS(Status))
                {
                    SetLastNtError(Status);
                    _SEH_LEAVE;
                }
            }

            Ret = BufLen / sizeof(WCHAR);
        }
        else /* !Ansi */
        {
            BufLen = ClassName->MaximumLength;

            /* query the atom name */
            Status = RtlQueryAtomInAtomTable(gAtomTable,
                                             Class->Atom,
                                             NULL,
                                             NULL,
                                             ClassName->Buffer,
                                             &BufLen);

            if (NT_SUCCESS(Status))
            {
                SetLastNtError(Status);
                _SEH_LEAVE;
            }

            Ret = BufLen / sizeof(WCHAR);
        }
    }
    _SEH_HANDLE
    {
        SetLastNtError(_SEH_GetExceptionCode());
    }
    _SEH_END;

    if (Ansi && szTemp != NULL && szTemp != szStaticTemp)
    {
        ExFreePool(szTemp);
    }

    return Ret;
}

ULONG_PTR
UserGetClassLongPtr(IN PWINDOWCLASS Class,
                    IN INT Index,
                    IN BOOL Ansi)
{
    ULONG_PTR Ret = 0;

    if (Index > 0)
    {
        PULONG_PTR Data;

        if (Index + sizeof(ULONG_PTR) < Index ||
            Index + sizeof(ULONG_PTR) > Class->ClsExtra)
        {
            SetLastWin32Error(ERROR_INVALID_PARAMETER);
            return 0;
        }

        Data = (PULONG_PTR)((ULONG_PTR)Class + Class->ClassExtraDataOffset);

        /* FIXME - Data might be a unaligned pointer! Might be a problem on
                   certain architectures, maybe using RtlCopyMemory is a
                   better choice for those architectures! */
        return *Data;
    }

    switch (Index)
    {
        case GCL_CBWNDEXTRA:
            Ret = (ULONG_PTR)Class->WndExtra;
            break;

        case GCL_CBCLSEXTRA:
            Ret = (ULONG_PTR)Class->ClsExtra;
            break;

        case GCLP_HBRBACKGROUND:
            Ret = (ULONG_PTR)Class->hbrBackground;
            break;

        case GCLP_HCURSOR:
            /* FIXME - get handle from pointer to CURSOR object */
            Ret = (ULONG_PTR)Class->hCursor;
            break;

        case GCLP_HICON:
            /* FIXME - get handle from pointer to ICON object */
            Ret = (ULONG_PTR)Class->hIcon;
            break;

        case GCLP_HICONSM:
            /* FIXME - get handle from pointer to ICON object */
            Ret = (ULONG_PTR)Class->hIconSm;
            break;

        case GCLP_HMODULE:
            Ret = (ULONG_PTR)Class->hInstance;
            break;

        case GCLP_MENUNAME:
            /* NOTE: Returns pointer in kernel heap! */
            if (Ansi)
                Ret = (ULONG_PTR)Class->AnsiMenuName;
            else
                Ret = (ULONG_PTR)Class->MenuName;
            break;

        case GCL_STYLE:
            Ret = (ULONG_PTR)Class->Style;
            break;

        case GCLP_WNDPROC:
            Ret = (ULONG_PTR)IntGetClassWndProc(Class,
                                                GetW32ProcessInfo(),
                                                Ansi,
                                                FALSE);
            break;

        case GCW_ATOM:
            Ret = (ULONG_PTR)Class->Atom;
            break;

        default:
            SetLastWin32Error(ERROR_INVALID_INDEX);
            break;
    }

    return Ret;
}

static BOOL
IntSetClassMenuName(IN PWINDOWCLASS Class,
                    IN PUNICODE_STRING MenuName)
{
    BOOL Ret = FALSE;

    /* change the base class first */
    Class = Class->Base;

    if (MenuName->Length != 0)
    {
        ANSI_STRING AnsiString;
        PWSTR strBufW;

        AnsiString.MaximumLength = RtlUnicodeStringToAnsiSize(MenuName);

        strBufW = UserHeapAlloc(MenuName->Length + sizeof(UNICODE_NULL) +
                                AnsiString.MaximumLength);
        if (strBufW != NULL)
        {
            _SEH_TRY
            {
                NTSTATUS Status;

                /* copy the unicode string */
                RtlCopyMemory(strBufW,
                              MenuName->Buffer,
                              MenuName->Length);
                strBufW[MenuName->Length / sizeof(WCHAR)] = UNICODE_NULL;

                /* create an ansi copy of the string */
                AnsiString.Buffer = (PSTR)(strBufW + (MenuName->Length / sizeof(WCHAR)) + 1);
                Status = RtlUnicodeStringToAnsiString(&AnsiString,
                                                      MenuName,
                                                      FALSE);
                if (!NT_SUCCESS(Status))
                {
                    SetLastNtError(Status);
                    _SEH_LEAVE;
                }

                Ret = TRUE;
            }
            _SEH_HANDLE
            {
                SetLastNtError(_SEH_GetExceptionCode());
            }
            _SEH_END;

            if (Ret)
            {
                /* update the base class */
                IntFreeClassMenuName(Class);
                Class->MenuName = strBufW;
                Class->AnsiMenuName = AnsiString.Buffer;

                /* update the clones */
                Class = Class->Clone;
                while (Class != NULL)
                {
                    Class->MenuName = strBufW;
                    Class->AnsiMenuName = AnsiString.Buffer;

                    Class = Class->Next;
                }
            }
            else
            {
                DPRINT1("Failed to copy class menu name!\n");
                UserHeapFree(strBufW);
            }
        }
        else
            SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
    }
    else
    {
        ASSERT(IS_INTRESOURCE(MenuName->Buffer));

        /* update the base class */
        IntFreeClassMenuName(Class);
        Class->MenuName = MenuName->Buffer;
        Class->AnsiMenuName = (PSTR)MenuName->Buffer;

        /* update the clones */
        Class = Class->Clone;
        while (Class != NULL)
        {
            Class->MenuName = MenuName->Buffer;
            Class->AnsiMenuName = (PSTR)MenuName->Buffer;

            Class = Class->Next;
        }

        Ret = TRUE;
    }

    return Ret;
}

ULONG_PTR
UserSetClassLongPtr(IN PWINDOWCLASS Class,
                    IN INT Index,
                    IN ULONG_PTR NewLong,
                    IN BOOL Ansi)
{
    ULONG_PTR Ret = 0;

    /* NOTE: For GCLP_MENUNAME and GCW_ATOM this function may raise an exception! */

    /* change the information in the base class first, then update the clones */
    Class = Class->Base;

    if (Index > 0)
    {
        PULONG_PTR Data;

        if (Index + sizeof(ULONG_PTR) < Index ||
            Index + sizeof(ULONG_PTR) > Class->ClsExtra)
        {
            SetLastWin32Error(ERROR_INVALID_PARAMETER);
            return 0;
        }

        Data = (PULONG_PTR)((ULONG_PTR)Class + Class->ClassExtraDataOffset);

        /* FIXME - Data might be a unaligned pointer! Might be a problem on
                   certain architectures, maybe using RtlCopyMemory is a
                   better choice for those architectures! */
        Ret = *Data;
        *Data = NewLong;

        /* update the clones */
        Class = Class->Clone;
        while (Class != NULL)
        {
            *(PULONG_PTR)((ULONG_PTR)Class + Class->ClassExtraDataOffset) = NewLong;
            Class = Class->Next;
        }

        return Ret;
    }

    switch (Index)
    {
        case GCL_CBWNDEXTRA:
            Ret = (ULONG_PTR)Class->WndExtra;
            Class->WndExtra = (INT)NewLong;

            /* update the clones */
            Class = Class->Clone;
            while (Class != NULL)
            {
                Class->WndExtra = (INT)NewLong;
                Class = Class->Next;
            }

            break;

        case GCL_CBCLSEXTRA:
            SetLastWin32Error(ERROR_INVALID_PARAMETER);
            break;

        case GCLP_HBRBACKGROUND:
            Ret = (ULONG_PTR)Class->hbrBackground;
            Class->hbrBackground = (HBRUSH)NewLong;

            /* update the clones */
            Class = Class->Clone;
            while (Class != NULL)
            {
                Class->hbrBackground = (HBRUSH)NewLong;
                Class = Class->Next;
            }
            break;

        case GCLP_HCURSOR:
            /* FIXME - get handle from pointer to CURSOR object */
            Ret = (ULONG_PTR)Class->hCursor;
            Class->hCursor = (HANDLE)NewLong;

            /* update the clones */
            Class = Class->Clone;
            while (Class != NULL)
            {
                Class->hCursor = (HANDLE)NewLong;
                Class = Class->Next;
            }
            break;

        case GCLP_HICON:
            /* FIXME - get handle from pointer to ICON object */
            Ret = (ULONG_PTR)Class->hIcon;
            Class->hIcon = (HANDLE)NewLong;

            /* update the clones */
            Class = Class->Clone;
            while (Class != NULL)
            {
                Class->hIcon = (HANDLE)NewLong;
                Class = Class->Next;
            }
            break;

        case GCLP_HICONSM:
            /* FIXME - get handle from pointer to ICON object */
            Ret = (ULONG_PTR)Class->hIconSm;
            Class->hIconSm = (HANDLE)NewLong;

            /* update the clones */
            Class = Class->Clone;
            while (Class != NULL)
            {
                Class->hIconSm = (HANDLE)NewLong;
                Class = Class->Next;
            }
            break;

        case GCLP_HMODULE:
            Ret = (ULONG_PTR)Class->hInstance;
            Class->hInstance = (HINSTANCE)NewLong;

           /* update the clones */
            Class = Class->Clone;
            while (Class != NULL)
            {
                Class->hInstance = (HINSTANCE)NewLong;
                Class = Class->Next;
            }
            break;

        case GCLP_MENUNAME:
        {
            PUNICODE_STRING Value = (PUNICODE_STRING)NewLong;

            if (!IntSetClassMenuName(Class,
                                     Value))
            {
                DPRINT("Setting the class menu name failed!\n");
            }

            /* FIXME - really return NULL? Wine does so... */
            break;
        }

        case GCL_STYLE:
            Ret = (ULONG_PTR)Class->Style;
            Class->Style = (UINT)NewLong;

            /* FIXME - what if the CS_GLOBALCLASS style is changed? should we
                       move the class to the appropriate list? For now, we save
                       the original value in Class->Global, so we can always
                       locate the appropriate list */

           /* update the clones */
            Class = Class->Clone;
            while (Class != NULL)
            {
                Class->Style = (UINT)NewLong;
                Class = Class->Next;
            }
            break;

        case GCLP_WNDPROC:
            Ret = (ULONG_PTR)IntSetClassWndProc(Class,
                                                (WNDPROC)NewLong,
                                                Ansi);
            break;

        case GCW_ATOM:
        {
            PUNICODE_STRING Value = (PUNICODE_STRING)NewLong;

            Ret = (ULONG_PTR)Class->Atom;
            if (!IntSetClassAtom(Class,
                                 Value))
            {
                Ret = 0;
            }
            break;
        }

        default:
            SetLastWin32Error(ERROR_INVALID_INDEX);
            break;
    }

    return Ret;
}

static BOOL
UserGetClassInfo(IN PWINDOWCLASS Class,
                 OUT PWNDCLASSEXW lpwcx,
                 IN BOOL Ansi)
{
    lpwcx->style = Class->Style;

    lpwcx->lpfnWndProc = IntGetClassWndProc(Class,
                                            GetW32ProcessInfo(),
                                            Ansi,
                                            FALSE);

    lpwcx->cbClsExtra = Class->ClsExtra;
    lpwcx->cbWndExtra = Class->WndExtra;
    lpwcx->hInstance = Class->hInstance;
    lpwcx->hIcon = Class->hIcon; /* FIXME - get handle from pointer */
    lpwcx->hCursor = Class->hCursor; /* FIXME - get handle from pointer */
    lpwcx->hbrBackground = Class->hbrBackground;

    if (Ansi)
        ((PWNDCLASSEXA)lpwcx)->lpszMenuName = Class->AnsiMenuName;
    else
        lpwcx->lpszMenuName = Class->MenuName;

    lpwcx->lpszClassName = (LPCWSTR)((ULONG_PTR)Class->Atom); /* FIXME - return the string? */

    lpwcx->hIconSm = Class->hIconSm; /* FIXME - get handle from pointer */

    return TRUE;
}

/* SYSCALLS *****************************************************************/


RTL_ATOM NTAPI
NtUserRegisterClassEx(IN CONST WNDCLASSEXW* lpwcx,
                      IN PUNICODE_STRING ClassName,
                      IN PUNICODE_STRING MenuName,
                      IN WNDPROC wpExtra,
                      IN DWORD Flags,
                      IN HMENU hMenu)

/*
 * FUNCTION:
 *   Registers a new class with the window manager
 * ARGUMENTS:
 *   lpwcx          = Win32 extended window class structure
 *   bUnicodeClass = Whether to send ANSI or unicode strings
 *                   to window procedures
 *   wpExtra       = Extra window procedure, if this is not null, its used for the second window procedure for standard controls.
 * RETURNS:
 *   Atom identifying the new class
 */
{
    WNDCLASSEXW CapturedClassInfo = {0};
    UNICODE_STRING CapturedName = {0}, CapturedMenuName = {0};
    RTL_ATOM Ret = (RTL_ATOM)0;

    if (Flags & ~REGISTERCLASS_ALL)
    {
        SetLastWin32Error(ERROR_INVALID_FLAGS);
        return Ret;
    }

    UserEnterExclusive();

    _SEH_TRY
    {
        /* Probe the parameters and basic parameter checks */
        if (ProbeForReadUint(&lpwcx->cbSize) != sizeof(WNDCLASSEXW))
        {
            goto InvalidParameter;
        }

        ProbeForRead(lpwcx,
                     sizeof(WNDCLASSEXW),
                     sizeof(ULONG));
        RtlCopyMemory(&CapturedClassInfo,
                      lpwcx,
                      sizeof(WNDCLASSEXW));

        CapturedName = ProbeForReadUnicodeString(ClassName);
        CapturedMenuName = ProbeForReadUnicodeString(MenuName);

        if (CapturedName.Length & 1 || CapturedMenuName.Length & 1 ||
            CapturedClassInfo.cbClsExtra < 0 ||
            CapturedClassInfo.cbClsExtra + CapturedName.Length +
                CapturedMenuName.Length + sizeof(WINDOWCLASS) < CapturedClassInfo.cbClsExtra ||
            CapturedClassInfo.cbWndExtra < 0 ||
            CapturedClassInfo.hInstance == NULL)
        {
            goto InvalidParameter;
        }

        if (CapturedName.Length != 0)
        {
            ProbeForRead(CapturedName.Buffer,
                         CapturedName.Length,
                         sizeof(WCHAR));
        }
        else
        {
            if (!IS_ATOM(CapturedName.Buffer))
            {
                goto InvalidParameter;
            }
        }

        if (CapturedMenuName.Length != 0)
        {
            ProbeForRead(CapturedMenuName.Buffer,
                         CapturedMenuName.Length,
                         sizeof(WCHAR));
        }
        else if (CapturedMenuName.Buffer != NULL &&
                 !IS_INTRESOURCE(CapturedMenuName.Buffer))
        {
InvalidParameter:
            SetLastWin32Error(ERROR_INVALID_PARAMETER);
            _SEH_LEAVE;
        }

        /* Register the class */
        Ret = UserRegisterClass(&CapturedClassInfo,
                                &CapturedName,
                                &CapturedMenuName,
                                hMenu, /* FIXME - pass pointer */
                                wpExtra,
                                Flags);

    }
    _SEH_HANDLE
    {
        SetLastNtError(_SEH_GetExceptionCode());
    }
    _SEH_END;

    UserLeave();

    return Ret;
}



ULONG_PTR NTAPI
NtUserGetClassLong(IN HWND hWnd,
                   IN INT Offset,
                   IN BOOL Ansi)
{
    PWINDOW_OBJECT Window;
    ULONG_PTR Ret = 0;

    if (Offset != GCLP_WNDPROC)
    {
        UserEnterShared();
    }
    else
    {
        UserEnterExclusive();
    }

    Window = UserGetWindowObject(hWnd);
    if (Window != NULL)
    {
        Ret = UserGetClassLongPtr(Window->Class,
                                  Offset,
                                  Ansi);

        if (Ret != 0 && Offset == GCLP_MENUNAME && !IS_INTRESOURCE(Ret))
        {
            Ret = (ULONG_PTR)DesktopHeapAddressToUser(Window->Class->Desktop,
                                                      (PVOID)Ret);
        }
    }

    UserLeave();

    return Ret;
}



ULONG_PTR STDCALL
NtUserSetClassLong(HWND hWnd,
                   INT Offset,
                   ULONG_PTR dwNewLong,
                   BOOL Ansi)
{
    PW32PROCESSINFO pi;
    PWINDOW_OBJECT Window;
    ULONG_PTR Ret = 0;

    UserEnterExclusive();

    pi = GetW32ProcessInfo();
    if (pi == NULL)
        goto Cleanup;

    Window = UserGetWindowObject(hWnd);
    if (Window != NULL)
    {
        if (Window->ti->kpi != pi)
        {
            SetLastWin32Error(ERROR_ACCESS_DENIED);
            goto Cleanup;
        }

        _SEH_TRY
        {
            UNICODE_STRING Value;

            /* probe the parameters */
            if (Offset == GCW_ATOM || Offset == GCLP_MENUNAME)
            {
                Value = ProbeForReadUnicodeString((PUNICODE_STRING)dwNewLong);
                if (Value.Length & 1)
                {
                    goto InvalidParameter;
                }

                if (Value.Length != 0)
                {
                    ProbeForRead(Value.Buffer,
                                 Value.Length,
                                 sizeof(WCHAR));
                }
                else
                {
                    if (Offset == GCW_ATOM && !IS_ATOM(Value.Buffer))
                    {
                        goto InvalidParameter;
                    }
                    else if (Offset == GCLP_MENUNAME && !IS_INTRESOURCE(Value.Buffer))
                    {
InvalidParameter:
                        SetLastWin32Error(ERROR_INVALID_PARAMETER);
                        _SEH_LEAVE;
                    }
                }

                dwNewLong = (ULONG_PTR)&Value;
            }

            Ret = UserSetClassLongPtr(Window->Class,
                                      Offset,
                                      dwNewLong,
                                      Ansi);
        }
        _SEH_HANDLE
        {
            SetLastNtError(_SEH_GetExceptionCode());
        }
        _SEH_END;
    }

Cleanup:
    UserLeave();

    return Ret;
}

DWORD STDCALL
NtUserSetClassWord(DWORD Unknown0,
                   DWORD Unknown1,
                   DWORD Unknown2)
{
   return(0);
}

BOOL NTAPI
NtUserUnregisterClass(IN PUNICODE_STRING ClassNameOrAtom,
                      IN HINSTANCE hInstance)
{
    UNICODE_STRING CapturedClassName;
    BOOL Ret = FALSE;

    UserEnterExclusive();

    _SEH_TRY
    {
        /* probe the paramters */
        CapturedClassName = ProbeForReadUnicodeString(ClassNameOrAtom);
        if (CapturedClassName.Length & 1)
        {
            goto InvalidParameter;
        }

        if (CapturedClassName.Length != 0)
        {
            ProbeForRead(CapturedClassName.Buffer,
                         CapturedClassName.Length,
                         sizeof(WCHAR));
        }
        else
        {
            if (!IS_ATOM(CapturedClassName.Buffer))
            {
InvalidParameter:
                SetLastWin32Error(ERROR_INVALID_PARAMETER);
                _SEH_LEAVE;
            }
        }

        /* unregister the class */
        Ret = UserUnregisterClass(&CapturedClassName,
                                  hInstance);
    }
    _SEH_HANDLE
    {
        SetLastNtError(_SEH_GetExceptionCode());
    }
    _SEH_END;

    UserLeave();

    return Ret;
}

/* NOTE: for system classes hInstance is not NULL here, but User32Instance */
BOOL STDCALL
NtUserGetClassInfo(
   HINSTANCE hInstance,
   PUNICODE_STRING ClassName,
   LPWNDCLASSEXW lpWndClassEx,
   BOOL Ansi)
{
    UNICODE_STRING CapturedClassName;
    PWINDOWCLASS Class;
    RTL_ATOM ClassAtom;
    PW32PROCESSINFO pi;
    BOOL Ret = FALSE;

    /* NOTE: need exclusive lock because getting the wndproc might require the
             creation of a call procedure handle */
    UserEnterExclusive();

    pi = GetW32ProcessInfo();
    if (pi == NULL)
    {
        goto Cleanup;
    }

    _SEH_TRY
    {
        /* probe the paramters */
        CapturedClassName = ProbeForReadUnicodeString(ClassName);
        if (CapturedClassName.Length & 1)
        {
            goto InvalidParameter;
        }

        if (CapturedClassName.Length != 0)
        {
            ProbeForRead(CapturedClassName.Buffer,
                         CapturedClassName.Length,
                         sizeof(WCHAR));
        }
        else
        {
            if (!IS_ATOM(CapturedClassName.Buffer))
            {
                goto InvalidParameter;
            }
        }

        if (ProbeForReadUint(&lpWndClassEx->cbSize) != sizeof(WNDCLASSEXW))
        {
InvalidParameter:
            SetLastWin32Error(ERROR_INVALID_PARAMETER);
            _SEH_LEAVE;
        }

        ProbeForWrite(lpWndClassEx,
                      sizeof(WNDCLASSEXW),
                      sizeof(ULONG));

        ClassAtom = IntGetClassAtom(&CapturedClassName,
                                    hInstance,
                                    pi,
                                    &Class,
                                    NULL);
        if (ClassAtom != (RTL_ATOM)0)
        {
            Ret = UserGetClassInfo(Class,
                                   lpWndClassEx,
                                   Ansi);

            if (Ret)
            {
                lpWndClassEx->lpszClassName = CapturedClassName.Buffer;

                /* FIXME - handle Class->Desktop == NULL!!!!! */

                if (Class->MenuName != NULL &&
                    !IS_INTRESOURCE(Class->MenuName))
                {
                    lpWndClassEx->lpszMenuName = DesktopHeapAddressToUser(Class->Desktop,
                                                                          (Ansi ?
                                                                              (PVOID)Class->AnsiMenuName :
                                                                              (PVOID)Class->MenuName));
                }
            }
        }
    }
    _SEH_HANDLE
    {
        SetLastNtError(_SEH_GetExceptionCode());
    }
    _SEH_END;

Cleanup:
    UserLeave();

    return Ret;
}



INT NTAPI
NtUserGetClassName (IN HWND hWnd,
                    OUT PUNICODE_STRING ClassName,
                    IN BOOL Ansi)
{
    PWINDOW_OBJECT Window;
    UNICODE_STRING CapturedClassName;
    INT Ret = 0;

    UserEnterShared();

    Window = UserGetWindowObject(hWnd);
    if (Window != NULL)
    {
        _SEH_TRY
        {
            ProbeForWriteUnicodeString(ClassName);
            CapturedClassName = *ClassName;

            /* get the class name */
            Ret = UserGetClassName(Window->Class,
                                   &CapturedClassName,
                                   Ansi);

            if (Ret != 0)
            {
                /* update the Length field */
                ClassName->Length = CapturedClassName.Length;
            }
        }
        _SEH_HANDLE
        {
            SetLastNtError(_SEH_GetExceptionCode());
        }
        _SEH_END;
    }

    UserLeave();

    return Ret;
}

DWORD STDCALL
NtUserGetWOWClass(DWORD Unknown0,
                  DWORD Unknown1)
{
   return(0);
}

/* EOF */
