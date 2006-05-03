#ifndef _WIN32K_CLASS_H
#define _WIN32K_CLASS_H

#define IS_ATOM(x) \
  (((ULONG_PTR)(x) > 0x0) && ((ULONG_PTR)(x) < 0x10000))

WNDPROC
GetCallProcHandle(IN PCALLPROC CallProc);

VOID
DestroyCallProc(IN PDESKTOP Desktop,
                IN OUT PCALLPROC CallProc);

PCALLPROC
CloneCallProc(IN PDESKTOP Desktop,
              IN PCALLPROC CallProc);

PCALLPROC
CreateCallProc(IN PDESKTOP Desktop,
               IN WNDPROC WndProc,
               IN BOOL Unicode,
               IN PW32PROCESSINFO pi);

BOOL
UserGetCallProcInfo(IN HANDLE hCallProc,
                    OUT PWNDPROC_INFO wpInfo);

void FASTCALL
DestroyProcessClasses(PW32PROCESS Process );

PWINDOWCLASS
IntReferenceClass(IN OUT PWINDOWCLASS BaseClass,
                  IN OUT PWINDOWCLASS *ClassLink,
                  IN PDESKTOP Desktop);

VOID
IntDereferenceClass(IN OUT PWINDOWCLASS Class,
                    IN PDESKTOP Desktop,
                    IN PW32PROCESSINFO pi);

RTL_ATOM
UserRegisterClass(IN CONST WNDCLASSEXW* lpwcx,
                  IN PUNICODE_STRING ClassName,
                  IN PUNICODE_STRING MenuName,
                  IN HANDLE hMenu,
                  IN WNDPROC wpExtra,
                  IN DWORD dwFlags);

BOOL
UserUnregisterClass(IN PUNICODE_STRING ClassName,
                    IN HINSTANCE hInstance);

ULONG_PTR
UserGetClassLongPtr(IN PWINDOWCLASS Class,
                    IN INT Index,
                    IN BOOL Ansi);

RTL_ATOM
IntGetClassAtom(IN PUNICODE_STRING ClassName,
                IN HINSTANCE hInstance  OPTIONAL,
                IN PW32PROCESSINFO pi  OPTIONAL,
                OUT PWINDOWCLASS *BaseClass  OPTIONAL,
                OUT PWINDOWCLASS **Link  OPTIONAL);

BOOL
IntCheckProcessDesktopClasses(IN PDESKTOP Desktop,
                              IN BOOL FreeOnFailure);

#endif /* _WIN32K_CLASS_H */

/* EOF */
