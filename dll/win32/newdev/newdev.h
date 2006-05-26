#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <regstr.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <tchar.h>
#include <wine/debug.h>

#include <stdio.h>

#include "resource.h"

extern HINSTANCE hDllInstance;

typedef struct _DEVINSTDATA
{
	HFONT hTitleFont;
	PBYTE buffer;
	DWORD requiredSize;
	DWORD regDataType;
	HWND hDialog;
	HDEVINFO hDevInfo;
	SP_DEVINFO_DATA devInfoData;
	SP_DRVINFO_DATA drvInfoData;

	LPTSTR CustomSearchPath; /* MULTI_SZ string */
} DEVINSTDATA, *PDEVINSTDATA;

#define WM_SEARCH_FINISHED (WM_USER + 10)

/* newdev.c */
BOOL
SearchDriver(
	IN PDEVINSTDATA DevInstData,
	IN LPCTSTR Directory OPTIONAL,
	IN LPCTSTR InfFile OPTIONAL);

BOOL
SearchDriverRecursive(
	IN PDEVINSTDATA DevInstData,
	IN LPCTSTR Path);

BOOL
InstallCurrentDriver(
	IN PDEVINSTDATA DevInstData);

/* wizard.c */
BOOL
DisplayWizard(
	IN PDEVINSTDATA DevInstData,
	IN HWND hwndParent,
	IN UINT startPage);
