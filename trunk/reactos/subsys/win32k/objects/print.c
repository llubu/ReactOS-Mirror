

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <win32k/print.h>

// #define NDEBUG
#include <internal/debug.h>

INT
STDCALL
W32kAbortDoc(HDC  hDC)
{
  UNIMPLEMENTED;
}

INT
STDCALL
W32kEndDoc(HDC  hDC)
{
  UNIMPLEMENTED;
}

INT
STDCALL
W32kEndPage(HDC  hDC)
{
  UNIMPLEMENTED;
}

INT
STDCALL
W32kEscape(HDC  hDC,
                INT  Escape,
                INT  InSize,
                LPCSTR  InData,
                LPVOID  OutData)
{
  UNIMPLEMENTED;
}

INT
STDCALL
W32kExtEscape(HDC  hDC,
                   INT  Escape,
                   INT  InSize,
                   LPCSTR  InData,
                   INT  OutSize,
                   LPSTR  OutData)
{
  UNIMPLEMENTED;
}

INT
STDCALL
W32kSetAbortProc(HDC  hDC,
                      ABORTPROC  AbortProc)
{
  UNIMPLEMENTED;
}

INT
STDCALL
W32kStartDoc(HDC  hDC,
                  CONST PDOCINFO  di)
{
  UNIMPLEMENTED;
}

INT
STDCALL
W32kStartPage(HDC  hDC)
{
  UNIMPLEMENTED;
}

