/* PSDK/NDK Headers */
#define WIN32_NO_STATUS
#include <windows.h>
#define NTOS_MODE_USER
#include <ndk/ntndk.h>

#include <accctrl.h>
#include <psapi.h>

/* Our own BLUE.SYS Driver for Console Output */
#include <blue/ntddblue.h>

/* External Winlogon Header */
#include <winlogon.h>

/* Internal CSRSS Headers */
#include <api.h>
#include <conio.h>
#include <csrplugin.h>
#include <desktopbg.h>
#include "guiconsole.h"
#include "tuiconsole.h"
#include <win32csr.h>

/* Public Win32K Headers */
#include <win32k/ntusrtyp.h>
#include <win32k/ntuser.h>
#include <win32k/callback.h>

#include <tchar.h>
#include <wchar.h>
#include <cpl.h>

#include "resource.h"

/* shared header with console.dll */
#include "console.h"

BOOL
WINAPI
Win32CsrHardError(
    IN PCSRSS_PROCESS_DATA ProcessData,
    IN PHARDERROR_MSG Message);

/* EOF */
