/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 ReactOS Team
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
/* $Id: dc.c,v 1.97 2003/11/08 15:00:36 gvg Exp $
 *
 * DC.C - Device context functions
 *
 */

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <ddk/ntddvid.h>

#include <win32k/bitmaps.h>
#include <win32k/brush.h>
#include <win32k/cliprgn.h>
#include <win32k/coord.h>
#include <win32k/driver.h>
#include <win32k/dc.h>
#include <win32k/misc.h>
#include <win32k/print.h>
#include <win32k/region.h>
#include <win32k/gdiobj.h>
#include <win32k/paint.h>
#include <win32k/pen.h>
#include <win32k/text.h>
#include "../eng/clip.h"
#include "../eng/handle.h"
#include <include/dce.h>
#include <include/error.h>
#include <include/inteng.h>
#include <include/eng.h>
#include <include/palette.h>

#define NDEBUG
#include <win32k/debug1.h>

#define TAG_DC  TAG('D', 'C', 'D', 'C')

static GDIDEVICE PrimarySurface;
static BOOL PrimarySurfaceCreated = FALSE;

/* FIXME: DCs should probably be thread safe  */

/*
 * DC device-independent Get/SetXXX functions
 * (RJJ) swiped from WINE
 */

#define DC_GET_VAL( func_type, func_name, dc_field ) \
func_type STDCALL  func_name( HDC hdc ) \
{                                   \
  func_type  ft;                    \
  PDC  dc = DC_LockDc( hdc );  \
  if (!dc)                          \
  {                                 \
    return 0;                       \
  }                                 \
  ft = dc->dc_field;                \
  DC_UnlockDc( hdc );				\
  return ft;                        \
}

/* DC_GET_VAL_EX is used to define functions returning a POINT or a SIZE. It is
 * important that the function has the right signature, for the implementation
 * we can do whatever we want.
 */
#define DC_GET_VAL_EX( NtGdiFuncName, IntFuncName, ret_x, ret_y, type ) \
VOID FASTCALL IntFuncName ( PDC dc, LP##type pt )  \
{                                                  \
  ASSERT ( dc );                                   \
  ASSERT ( pt );                                   \
  ((LPPOINT)pt)->x = dc->ret_x;                    \
  ((LPPOINT)pt)->y = dc->ret_y;                    \
}                                                  \
BOOL STDCALL NtGdiFuncName ( HDC hdc, LP##type pt ) \
{                                                  \
  PDC dc = DC_LockDc ( hdc );                 \
  if ( !dc )                                       \
    return FALSE;                                  \
  IntFuncName ( dc, pt );                          \
  DC_UnlockDc ( hdc );                           \
  return TRUE;                                     \
}

#define DC_SET_MODE( func_name, dc_field, min_val, max_val ) \
INT STDCALL  func_name( HDC hdc, INT mode ) \
{                                           \
  INT  prevMode;                            \
  PDC  dc;                                  \
  if ((mode < min_val) || (mode > max_val)) \
    return 0;                               \
  dc = DC_LockDc ( hdc );              \
  if ( !dc )                                \
    return 0;                               \
  prevMode = dc->dc_field;                  \
  dc->dc_field = mode;                      \
  DC_UnlockDc ( hdc );                    \
  return prevMode;                          \
}


//  ---------------------------------------------------------  File Statics

//  -----------------------------------------------------  Public Functions

BOOL STDCALL
NtGdiCancelDC(HDC  hDC)
{
  UNIMPLEMENTED;
}

HDC STDCALL
NtGdiCreateCompatableDC(HDC  hDC)
{
  PDC  NewDC, OrigDC;
  HBITMAP  hBitmap;
  HDC hNewDC;
  HRGN hVisRgn;
  BITMAPOBJ *pb;
  PSURFGDI SurfGDI;

  if (hDC == NULL)
  {
    if (! PrimarySurfaceCreated)
      {
        DPRINT1("Can't create compatible DC because initialization of primary surface failed\n");
        return NULL;
      }
    OrigDC = NULL;
    hNewDC = DC_AllocDC(L"DISPLAY");
  }
  else
  {
    /*  Allocate a new DC based on the original DC's device  */
    OrigDC = DC_LockDc(hDC);
    if (NULL == OrigDC)
    {
      return NULL;
    }
    hNewDC = DC_AllocDC(OrigDC->DriverName);
  }

  if (NULL == hNewDC)
  {
    return  NULL;
  }
  NewDC = DC_LockDc( hNewDC );

  /* Copy information from original DC to new DC  */
  NewDC->hSelf = NewDC;

  /* FIXME: Should this DC request its own PDEV?  */
  if(OrigDC == NULL)
  {
    NewDC->PDev = PrimarySurface.PDev;
    memcpy(NewDC->FillPatternSurfaces, PrimarySurface.FillPatterns,
           sizeof(NewDC->FillPatternSurfaces));
    NewDC->GDIInfo = &PrimarySurface.GDIInfo;
    NewDC->DevInfo = &PrimarySurface.DevInfo;
    SurfGDI = (PSURFGDI)AccessInternalObject((ULONG) PrimarySurface.Handle);
    NewDC->w.bitsPerPixel = SurfGDI->BitsPerPixel;
  }
  else
  {
    NewDC->PDev = OrigDC->PDev;
    NewDC->DMW = OrigDC->DMW;
    memcpy(NewDC->FillPatternSurfaces,
           OrigDC->FillPatternSurfaces,
           sizeof OrigDC->FillPatternSurfaces);
    NewDC->GDIInfo = OrigDC->GDIInfo;
    NewDC->DevInfo = OrigDC->DevInfo;
    NewDC->w.bitsPerPixel = OrigDC->w.bitsPerPixel;
  }

  /* DriverName is copied in the AllocDC routine  */
  if(OrigDC == NULL)
  {
    NewDC->DeviceDriver = (HANDLE) PrimarySurface.VideoDeviceObject;
  }
  else
  {
    NewDC->DeviceDriver = OrigDC->DeviceDriver;
    NewDC->wndOrgX = OrigDC->wndOrgX;
    NewDC->wndOrgY = OrigDC->wndOrgY;
    NewDC->wndExtX = OrigDC->wndExtX;
    NewDC->wndExtY = OrigDC->wndExtY;
    NewDC->vportOrgX = OrigDC->vportOrgX;
    NewDC->vportOrgY = OrigDC->vportOrgY;
    NewDC->vportExtX = OrigDC->vportExtX;
    NewDC->vportExtY = OrigDC->vportExtY;
  }

  /* Create default bitmap */
  if (!(hBitmap = NtGdiCreateBitmap( 1, 1, 1, NewDC->w.bitsPerPixel, NULL )))
  {
    DC_UnlockDc( hDC );
    DC_UnlockDc( hNewDC );
    DC_FreeDC( hNewDC );
    return NULL;
  }
  NewDC->w.flags        = DC_MEMORY;
  NewDC->w.hBitmap      = hBitmap;
  NewDC->w.hFirstBitmap = hBitmap;
  pb = BITMAPOBJ_LockBitmap(hBitmap);
  NewDC->Surface = BitmapToSurf(pb);
  BITMAPOBJ_UnlockBitmap(hBitmap);

  if(OrigDC == NULL)
  {
    NewDC->w.hPalette = NewDC->DevInfo->hpalDefault;
  }
  else
  {
    NewDC->w.hPalette = OrigDC->w.hPalette;
    NewDC->w.textColor = OrigDC->w.textColor;
    NewDC->w.textAlign = OrigDC->w.textAlign;
    NewDC->w.backgroundColor = OrigDC->w.backgroundColor;
    NewDC->w.backgroundMode = OrigDC->w.backgroundMode;
  }
  if (NULL != hDC)
  {
    DC_UnlockDc( hDC );
  }
  DC_UnlockDc( hNewDC );

  hVisRgn = NtGdiCreateRectRgn(0, 0, 1, 1);
  NtGdiSelectVisRgn(hNewDC, hVisRgn);
  NtGdiDeleteObject(hVisRgn);

  DC_InitDC(hNewDC);

  return  hNewDC;
}

static BOOL FASTCALL
GetRegistryPath(PUNICODE_STRING RegistryPath)
{
  RTL_QUERY_REGISTRY_TABLE QueryTable[2];
  NTSTATUS Status;

  RtlInitUnicodeString(RegistryPath, NULL);
  RtlZeroMemory(QueryTable, sizeof(QueryTable));
  QueryTable[0].Flags = RTL_QUERY_REGISTRY_REQUIRED | RTL_QUERY_REGISTRY_DIRECT;
  QueryTable[0].Name = L"\\Device\\Video0";
  QueryTable[0].EntryContext = RegistryPath;

  Status = RtlQueryRegistryValues(RTL_REGISTRY_DEVICEMAP,
                                  L"VIDEO",
                                  QueryTable,
                                  NULL,
                                  NULL);
  if (! NT_SUCCESS(Status))
    {
      DPRINT1("No \\Device\\Video0 value in DEVICEMAP\\VIDEO found\n");
      return FALSE;
    }

  DPRINT("RegistryPath %S\n", RegistryPath->Buffer);

  return TRUE;
}

static BOOL FASTCALL
FindDriverFileNames(PUNICODE_STRING DriverFileNames)
{
  RTL_QUERY_REGISTRY_TABLE QueryTable[2];
  UNICODE_STRING RegistryPath;
  NTSTATUS Status;

  if (! GetRegistryPath(&RegistryPath))
    {
      DPRINT("GetRegistryPath failed\n");
      return FALSE;
    }

  RtlZeroMemory(QueryTable, sizeof(QueryTable));
  QueryTable[0].Flags = RTL_QUERY_REGISTRY_REQUIRED | RTL_QUERY_REGISTRY_DIRECT;
  QueryTable[0].Name = L"InstalledDisplayDrivers";
  QueryTable[0].EntryContext = DriverFileNames;

  Status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                  RegistryPath.Buffer,
                                  QueryTable,
                                  NULL,
                                  NULL);
  RtlFreeUnicodeString(&RegistryPath);
  if (! NT_SUCCESS(Status))
    {
      DPRINT1("No InstalledDisplayDrivers value in service entry found\n");
      return FALSE;
    }

  DPRINT("DriverFileNames %S\n", DriverFileNames->Buffer);

  return TRUE;
}

static NTSTATUS STDCALL
DevModeCallback(IN PWSTR ValueName,
                IN ULONG ValueType,
                IN PVOID ValueData,
                IN ULONG ValueLength,
                IN PVOID Context,
                IN PVOID EntryContext)
{
  PDEVMODEW DevMode = (PDEVMODEW) Context;

  DPRINT("Found registry value for name %S: type %d, length %d\n",
         ValueName, ValueType, ValueLength);

  if (REG_DWORD == ValueType && sizeof(DWORD) == ValueLength)
    {
      if (0 == _wcsicmp(ValueName, L"DefaultSettings.BitsPerPel"))
        {
          DevMode->dmBitsPerPel = *((DWORD *) ValueData);
        }
      else if (0 == _wcsicmp(ValueName, L"DefaultSettings.Flags"))
        {
          DevMode->dmDisplayFlags = *((DWORD *) ValueData);
        }
      else if (0 == _wcsicmp(ValueName, L"DefaultSettings.VRefresh"))
        {
          DevMode->dmDisplayFrequency = *((DWORD *) ValueData);
        }
      else if (0 == _wcsicmp(ValueName, L"DefaultSettings.XPanning"))
        {
          DevMode->dmPanningWidth = *((DWORD *) ValueData);
        }
      else if (0 == _wcsicmp(ValueName, L"DefaultSettings.XResolution"))
        {
          DevMode->dmPelsWidth = *((DWORD *) ValueData);
        }
      else if (0 == _wcsicmp(ValueName, L"DefaultSettings.YPanning"))
        {
          DevMode->dmPanningHeight = *((DWORD *) ValueData);
        }
      else if (0 == _wcsicmp(ValueName, L"DefaultSettings.YResolution"))
        {
          DevMode->dmPelsHeight = *((DWORD *) ValueData);
        }
    }

  return STATUS_SUCCESS;
}

static BOOL FASTCALL
SetupDevMode(PDEVMODEW DevMode)
{
  static WCHAR RegistryMachineSystem[] = L"\\REGISTRY\\MACHINE\\SYSTEM\\";
  static WCHAR CurrentControlSet[] = L"CURRENTCONTROLSET\\";
  static WCHAR ControlSet[] = L"CONTROLSET";
  static WCHAR Insert[] = L"Hardware Profiles\\Current\\System\\CurrentControlSet\\";
  UNICODE_STRING RegistryPath;
  BOOL Valid;
  PWCHAR AfterControlSet;
  PWCHAR ProfilePath;
  RTL_QUERY_REGISTRY_TABLE QueryTable[2];
  NTSTATUS Status;

  if (! GetRegistryPath(&RegistryPath))
    {
      DPRINT("GetRegistryPath failed\n");
      return FALSE;
    }

  Valid = (0 == _wcsnicmp(RegistryPath.Buffer, RegistryMachineSystem,
                          wcslen(RegistryMachineSystem)));
  if (Valid)
    {
      AfterControlSet = RegistryPath.Buffer + wcslen(RegistryMachineSystem);
      if (0 == _wcsnicmp(AfterControlSet, CurrentControlSet, wcslen(CurrentControlSet)))
        {
          AfterControlSet += wcslen(CurrentControlSet);
        }
      else if (0 == _wcsnicmp(AfterControlSet, ControlSet, wcslen(ControlSet)))
        {
          AfterControlSet += wcslen(ControlSet);
          while (L'0' <= *AfterControlSet && L'9' <= *AfterControlSet)
            {
              AfterControlSet++;
            }
          Valid = (L'\\' == *AfterControlSet);
          AfterControlSet++;
        }
      else
        {
          Valid = FALSE;
        }
    }

  if (Valid)
    {
      ProfilePath = ExAllocatePoolWithTag(PagedPool,
                                          (wcslen(RegistryPath.Buffer) + 
                                           wcslen(Insert) + 1) * sizeof(WCHAR),
                                          TAG_DC);
      if (NULL != ProfilePath)
        {
          wcsncpy(ProfilePath, RegistryPath.Buffer, AfterControlSet - RegistryPath.Buffer);
          wcscpy(ProfilePath + (AfterControlSet - RegistryPath.Buffer), Insert);
          wcscat(ProfilePath, AfterControlSet);

          RtlZeroMemory(QueryTable, sizeof(QueryTable));
          QueryTable[0].QueryRoutine = DevModeCallback;
          QueryTable[0].Flags = 0;
          QueryTable[0].Name = NULL;
          QueryTable[0].EntryContext = NULL;
          QueryTable[0].DefaultType = REG_NONE;
          QueryTable[0].DefaultData = NULL;
          QueryTable[0].DefaultLength = 0;

          Status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                          ProfilePath,
                                          QueryTable,
                                          DevMode,
                                          NULL);
          if (! NT_SUCCESS(Status))
            {
              DPRINT1("RtlQueryRegistryValues for %S failed with status 0x%08x\n",
                      ProfilePath, Status);
              Valid = FALSE;
            }
          else
            {
              DPRINT("dmBitsPerPel %d dmDisplayFrequency %d dmPelsWidth %d dmPelsHeight %d\n",
                     DevMode->dmBitsPerPel, DevMode->dmDisplayFrequency,
                     DevMode->dmPelsWidth, DevMode->dmPelsHeight);
              if (0 == DevMode->dmBitsPerPel || 0 == DevMode->dmDisplayFrequency
                  || 0 == DevMode->dmPelsWidth || 0 == DevMode->dmPelsHeight)
                {
                  DPRINT("Not all required devmode members are set\n");
                  Valid = FALSE;
                }
            }

          ExFreePool(ProfilePath);
        }
      else
        {
          Valid = FALSE;
        }
    }
  else
    {
      DPRINT1("Unparsable registry path %S in DEVICEMAP\\VIDEO0", RegistryPath.Buffer);
    }

  RtlFreeUnicodeString(&RegistryPath);

  if (! Valid)
    {
      RtlZeroMemory(DevMode, sizeof(DEVMODEW));
    }

  return Valid;
}

BOOL STDCALL
NtGdiCreatePrimarySurface(LPCWSTR Driver,
				      LPCWSTR Device)
{
  PGD_ENABLEDRIVER GDEnableDriver;
  DRVENABLEDATA DED;
  PSURFOBJ SurfObj;
  PSURFGDI SurfGDI;
  UNICODE_STRING DriverFileNames;
  PWSTR CurrentName;
  BOOL GotDriver;
  BOOL DoDefault;
  extern void FASTCALL IntInitDesktopWindow(ULONG Width, ULONG Height);

  /*  Open the miniport driver  */
  if ((PrimarySurface.VideoDeviceObject = DRIVER_FindMPDriver(Driver)) == NULL)
  {
    DPRINT1("FindMPDriver failed\n");
    return(FALSE);
  }

  /*  Retrieve DDI driver names from registry */
  RtlInitUnicodeString(&DriverFileNames, NULL);
  if (! FindDriverFileNames(&DriverFileNames))
  {
    DPRINT1("FindDriverFileNames failed\n");
    return(FALSE);
  }

  /* DriverFileNames may be a list of drivers in REG_SZ_MULTI format, scan all of
     them until a good one found */
  CurrentName = DriverFileNames.Buffer;
  GotDriver = FALSE;
  while (! GotDriver && CurrentName < DriverFileNames.Buffer + DriverFileNames.Length)
  {
    /*  Get the DDI driver's entry point  */
    GDEnableDriver = DRIVER_FindDDIDriver(CurrentName);
    if (NULL == GDEnableDriver)
    {
      DPRINT("FindDDIDriver failed for %S\n", CurrentName);
    }
    else
    {
      /*  Call DDI driver's EnableDriver function  */
      RtlZeroMemory(&DED, sizeof(DED));

      if (!GDEnableDriver(DDI_DRIVER_VERSION, sizeof(DED), &DED))
      {
        DPRINT("DrvEnableDriver failed for %S\n", CurrentName);
      }
      else
      {
        GotDriver = TRUE;
      }
    }

    if (! GotDriver)
    {
      /* Skip to the next name but never get past the Unicode string */
      while (L'\0' != *CurrentName &&
             CurrentName < DriverFileNames.Buffer + DriverFileNames.Length)
      {
	CurrentName++;
      }
      if (CurrentName < DriverFileNames.Buffer + DriverFileNames.Length)
      {
        CurrentName++;
      }
    }
  }
  RtlFreeUnicodeString(&DriverFileNames);
  if (! GotDriver)
  {
    ObDereferenceObject(PrimarySurface.VideoDeviceObject);
    DPRINT1("No suitable DDI driver found\n");
    return FALSE;
  }

  DPRINT("Display driver %S loaded\n", CurrentName);

  DPRINT("Building DDI Functions\n");

  /*  Make sure the non existant functions are NULL  */
  RtlZeroMemory(&PrimarySurface.DriverFunctions, sizeof(PrimarySurface.DriverFunctions));

  /*  Construct DDI driver function dispatch table  */
  if (!DRIVER_BuildDDIFunctions(&DED, &PrimarySurface.DriverFunctions))
  {
    ObDereferenceObject(PrimarySurface.VideoDeviceObject);
    DPRINT1("BuildDDIFunctions failed\n");
    return(FALSE);
  }

  /*  Allocate a phyical device handle from the driver  */
  if (SetupDevMode(&PrimarySurface.DMW))
    {
      PrimarySurface.PDev =
        PrimarySurface.DriverFunctions.EnablePDev(&PrimarySurface.DMW,
                                                  L"",
                                                  HS_DDI_MAX,
                                                  PrimarySurface.FillPatterns,
                                                  sizeof(PrimarySurface.GDIInfo),
                                                  (ULONG *) &PrimarySurface.GDIInfo,
                                                  sizeof(PrimarySurface.DevInfo),
                                                  &PrimarySurface.DevInfo,
                                                  NULL,
                                                  L"",
                                                  (HANDLE) (PrimarySurface.VideoDeviceObject));
      DoDefault = (NULL == PrimarySurface.PDev);
      if (DoDefault)
        {
          DPRINT1("DrvEnablePDev with registry parameters failed\n");
        }
    }
  else
    {
      DoDefault = TRUE;
    }

  if (DoDefault)
    {
      RtlZeroMemory(&(PrimarySurface.DMW), sizeof(DEVMODEW));
      PrimarySurface.PDev =
        PrimarySurface.DriverFunctions.EnablePDev(&PrimarySurface.DMW,
                                                  L"",
                                                  HS_DDI_MAX,
                                                  PrimarySurface.FillPatterns,
                                                  sizeof(PrimarySurface.GDIInfo),
                                                  (ULONG *) &PrimarySurface.GDIInfo,
                                                  sizeof(PrimarySurface.DevInfo),
                                                  &PrimarySurface.DevInfo,
                                                  NULL,
                                                  L"",
                                                  (HANDLE) (PrimarySurface.VideoDeviceObject));

      if (NULL == PrimarySurface.PDev)
        {
          ObDereferenceObject(PrimarySurface.VideoDeviceObject);
          DPRINT1("DrvEnablePDEV with default parameters failed\n");
          DPRINT1("Perhaps DDI driver doesn't match miniport driver?\n");
          return FALSE;
        }
    }

  if (0 == PrimarySurface.GDIInfo.ulLogPixelsX)
  {
    DPRINT("Adjusting GDIInfo.ulLogPixelsX\n");
    PrimarySurface.GDIInfo.ulLogPixelsX = 96;
  }
  if (0 == PrimarySurface.GDIInfo.ulLogPixelsY)
  {
    DPRINT("Adjusting GDIInfo.ulLogPixelsY\n");
    PrimarySurface.GDIInfo.ulLogPixelsY = 96;
  }
  GDIOBJ_MarkObjectGlobal(PrimarySurface.DevInfo.hpalDefault);

  DPRINT("calling completePDev\n");

  /*  Complete initialization of the physical device  */
  PrimarySurface.DriverFunctions.CompletePDev(PrimarySurface.PDev,
					      &PrimarySurface);

  DPRINT("calling DRIVER_ReferenceDriver\n");

  DRIVER_ReferenceDriver (Driver);

  DPRINT("calling EnableSurface\n");

  /*  Enable the drawing surface  */
  PrimarySurface.Handle =
    PrimarySurface.DriverFunctions.EnableSurface(PrimarySurface.PDev);
  if (NULL == PrimarySurface.Handle)
    {
      ObDereferenceObject(PrimarySurface.VideoDeviceObject);
      DPRINT1("DrvEnableSurface failed\n");
      return FALSE;
    }

  SurfObj = (PSURFOBJ)AccessUserObject((ULONG) PrimarySurface.Handle);
  SurfObj->dhpdev = PrimarySurface.PDev;
  SurfGDI = (PSURFGDI)AccessInternalObject((ULONG) PrimarySurface.Handle);
  IntInitDesktopWindow(SurfGDI->SurfObj.sizlBitmap.cx, SurfGDI->SurfObj.sizlBitmap.cy);

  return TRUE;
}

HDC STDCALL
NtGdiCreateDC(LPCWSTR  Driver,
             LPCWSTR  Device,
             LPCWSTR  Output,
             CONST PDEVMODEW  InitData)
{
  HDC      hNewDC;
  PDC      NewDC;
  HDC      hDC = NULL;
  PSURFGDI SurfGDI;
  HRGN     hVisRgn;

  /*  Check for existing DC object  */
  if ((hNewDC = DC_FindOpenDC(Driver)) != NULL)
  {
    hDC = hNewDC;
    return  NtGdiCreateCompatableDC(hDC);
  }

  DPRINT("NAME: %S\n", Driver); // FIXME: Should not crash if NULL

  /*  Allocate a DC object  */
  if ((hNewDC = DC_AllocDC(Driver)) == NULL)
  {
    return  NULL;
  }

  NewDC = DC_LockDc( hNewDC );
  ASSERT( NewDC );

  if (! PrimarySurfaceCreated)
    {
      PrimarySurfaceCreated = TRUE;
      if (!NtGdiCreatePrimarySurface(Driver, Device))
	{
	  PrimarySurfaceCreated = FALSE;
	  DC_UnlockDc( hNewDC );
	  DC_FreeDC(hNewDC);
	  return  NULL;
	}
    }
  NewDC->DMW = PrimarySurface.DMW;
  NewDC->DevInfo = &PrimarySurface.DevInfo;
  NewDC->GDIInfo = &PrimarySurface.GDIInfo;
  memcpy(NewDC->FillPatternSurfaces, PrimarySurface.FillPatterns,
	 sizeof(NewDC->FillPatternSurfaces));
  NewDC->PDev = PrimarySurface.PDev;
  NewDC->Surface = PrimarySurface.Handle;
  NewDC->DriverFunctions = PrimarySurface.DriverFunctions;

  NewDC->DMW.dmSize = sizeof(NewDC->DMW);
  NewDC->DMW.dmFields = 0x000fc000;

  /* FIXME: get mode selection information from somewhere  */

  NewDC->DMW.dmLogPixels = 96;
  SurfGDI = (PSURFGDI)AccessInternalObject((ULONG) PrimarySurface.Handle);
  NewDC->DMW.dmBitsPerPel = SurfGDI->BitsPerPixel;
  NewDC->DMW.dmPelsWidth = SurfGDI->SurfObj.sizlBitmap.cx;
  NewDC->DMW.dmPelsHeight = SurfGDI->SurfObj.sizlBitmap.cy;
  NewDC->DMW.dmDisplayFlags = 0;
  NewDC->DMW.dmDisplayFrequency = 0;

  NewDC->w.bitsPerPixel = SurfGDI->BitsPerPixel; // FIXME: set this here??

  NewDC->w.hPalette = NewDC->DevInfo->hpalDefault;

  DPRINT("Bits per pel: %u\n", NewDC->w.bitsPerPixel);
  
  DC_UnlockDc( hNewDC );

  hVisRgn = NtGdiCreateRectRgn(0, 0, SurfGDI->SurfObj.sizlBitmap.cx,
                              SurfGDI->SurfObj.sizlBitmap.cy);
  NtGdiSelectVisRgn(hNewDC, hVisRgn);
  NtGdiDeleteObject(hVisRgn);

  /*  Initialize the DC state  */
  DC_InitDC(hNewDC);
  NtGdiSetTextColor(hNewDC, RGB(0, 0, 0));
  NtGdiSetTextAlign(hNewDC, TA_TOP);
  NtGdiSetBkColor(hNewDC, RGB(255, 255, 255));
  NtGdiSetBkMode(hNewDC, OPAQUE);

  return hNewDC;
}

HDC STDCALL
NtGdiCreateIC(LPCWSTR  Driver,
             LPCWSTR  Device,
             LPCWSTR  Output,
             CONST PDEVMODEW  DevMode)
{
  /* FIXME: this should probably do something else...  */
  return  NtGdiCreateDC(Driver, Device, Output, DevMode);
}

BOOL STDCALL
NtGdiDeleteDC(HDC  DCHandle)
{
  PDC  DCToDelete;

  DCToDelete = DC_LockDc(DCHandle);
  if (DCToDelete == NULL)
    {
      return  FALSE;
    }
  DPRINT( "Deleting DC\n" );
  CHECKPOINT;
  /*  First delete all saved DCs  */
  while (DCToDelete->saveLevel)
  {
    PDC  savedDC;
    HDC  savedHDC;

    savedHDC = DC_GetNextDC (DCToDelete);
    savedDC = DC_LockDc (savedHDC);
    if (savedDC == NULL)
    {
      break;
    }
    DC_SetNextDC (DCToDelete, DC_GetNextDC (savedDC));
    DCToDelete->saveLevel--;
	DC_UnlockDc( savedHDC );
    NtGdiDeleteDC (savedHDC);
  }

  /*  Free GDI resources allocated to this DC  */
  if (!(DCToDelete->w.flags & DC_SAVED))
  {
    /*
    NtGdiSelectObject (DCHandle, STOCK_BLACK_PEN);
    NtGdiSelectObject (DCHandle, STOCK_WHITE_BRUSH);
    NtGdiSelectObject (DCHandle, STOCK_SYSTEM_FONT);
    DC_LockDC (DCHandle); NtGdiSelectObject does not recognize stock objects yet  */
    if (DCToDelete->w.flags & DC_MEMORY)
    {
      EngDeleteSurface (DCToDelete->Surface);
      NtGdiDeleteObject (DCToDelete->w.hFirstBitmap);
    }
  }
  if (DCToDelete->w.hClipRgn)
  {
    NtGdiDeleteObject (DCToDelete->w.hClipRgn);
  }
  if (DCToDelete->w.hVisRgn)
  {
    NtGdiDeleteObject (DCToDelete->w.hVisRgn);
  }
  if (NULL != DCToDelete->CombinedClip)
    {
      IntEngDeleteClipRegion(DCToDelete->CombinedClip);
    }
  if (DCToDelete->w.hGCClipRgn)
  {
    NtGdiDeleteObject (DCToDelete->w.hGCClipRgn);
  }
#if 0 /* FIXME */
  PATH_DestroyGdiPath (&DCToDelete->w.path);
#endif
  DC_UnlockDc( DCHandle );
  DC_FreeDC ( DCHandle );

  return TRUE;
}

INT STDCALL
NtGdiDrawEscape(HDC  hDC,
               INT  nEscape,
               INT  cbInput,
               LPCSTR  lpszInData)
{
  UNIMPLEMENTED;
}

INT STDCALL
NtGdiEnumObjects(HDC  hDC,
                INT  ObjectType,
                GOBJENUMPROC  ObjectFunc,
                LPARAM  lParam)
{
  UNIMPLEMENTED;
}

DC_GET_VAL( COLORREF, NtGdiGetBkColor, w.backgroundColor )
DC_GET_VAL( INT, NtGdiGetBkMode, w.backgroundMode )
DC_GET_VAL_EX( NtGdiGetBrushOrgEx, IntGetBrushOrgEx, w.brushOrgX, w.brushOrgY, POINT )
DC_GET_VAL( HRGN, NtGdiGetClipRgn, w.hClipRgn )

HGDIOBJ STDCALL
NtGdiGetCurrentObject(HDC  hDC, UINT  ObjectType)
{
  UNIMPLEMENTED;
}

DC_GET_VAL_EX ( NtGdiGetCurrentPositionEx, IntGetCurrentPositionEx, w.CursPosX, w.CursPosY, POINT )

BOOL STDCALL
NtGdiGetDCOrgEx(HDC  hDC, LPPOINT  Point)
{
  PDC dc;

  if (!Point)
  {
    return FALSE;
  }
  dc = DC_LockDc(hDC);
  if (dc == NULL)
  {
    return FALSE;
  }

  Point->x = Point->y = 0;

  Point->x += dc->w.DCOrgX;
  Point->y += dc->w.DCOrgY;
  DC_UnlockDc( hDC );
  return  TRUE;
}

COLORREF STDCALL
NtGdiSetBkColor(HDC hDC, COLORREF color)
{
  COLORREF oldColor;
  PDC  dc = DC_LockDc(hDC);

  if ( !dc )
    return 0x80000000;

  oldColor = dc->w.backgroundColor;
  dc->w.backgroundColor = color;
  DC_UnlockDc ( hDC );
  return oldColor;
}

HDC STDCALL
NtGdiGetDCState(HDC  hDC)
{
  PDC  newdc, dc;
  HDC hnewdc;

  dc = DC_LockDc(hDC);
  if (dc == NULL)
  {
    return 0;
  }

  hnewdc = DC_AllocDC(NULL);
  if (hnewdc == NULL)
  {
    DC_UnlockDc( hDC );
    return 0;
  }
  newdc = DC_LockDc( hnewdc );
  ASSERT( newdc );

  newdc->w.flags            = dc->w.flags | DC_SAVED;
  newdc->w.hPen             = dc->w.hPen;
  newdc->w.hBrush           = dc->w.hBrush;
  newdc->w.hFont            = dc->w.hFont;
  newdc->w.hBitmap          = dc->w.hBitmap;
  newdc->w.hFirstBitmap     = dc->w.hFirstBitmap;
#if 0
  newdc->w.hDevice          = dc->w.hDevice;
  newdc->w.hPalette         = dc->w.hPalette;
#endif
  newdc->w.totalExtent      = dc->w.totalExtent;
  newdc->w.bitsPerPixel     = dc->w.bitsPerPixel;
  newdc->w.ROPmode          = dc->w.ROPmode;
  newdc->w.polyFillMode     = dc->w.polyFillMode;
  newdc->w.stretchBltMode   = dc->w.stretchBltMode;
  newdc->w.relAbsMode       = dc->w.relAbsMode;
  newdc->w.backgroundMode   = dc->w.backgroundMode;
  newdc->w.backgroundColor  = dc->w.backgroundColor;
  newdc->w.textColor        = dc->w.textColor;
  newdc->w.brushOrgX        = dc->w.brushOrgX;
  newdc->w.brushOrgY        = dc->w.brushOrgY;
  newdc->w.textAlign        = dc->w.textAlign;
  newdc->w.charExtra        = dc->w.charExtra;
  newdc->w.breakTotalExtra  = dc->w.breakTotalExtra;
  newdc->w.breakCount       = dc->w.breakCount;
  newdc->w.breakExtra       = dc->w.breakExtra;
  newdc->w.breakRem         = dc->w.breakRem;
  newdc->w.MapMode          = dc->w.MapMode;
  newdc->w.GraphicsMode     = dc->w.GraphicsMode;
#if 0
  /* Apparently, the DC origin is not changed by [GS]etDCState */
  newdc->w.DCOrgX           = dc->w.DCOrgX;
  newdc->w.DCOrgY           = dc->w.DCOrgY;
#endif
  newdc->w.CursPosX         = dc->w.CursPosX;
  newdc->w.CursPosY         = dc->w.CursPosY;
  newdc->w.ArcDirection     = dc->w.ArcDirection;
#if 0
  newdc->w.xformWorld2Wnd   = dc->w.xformWorld2Wnd;
  newdc->w.xformWorld2Vport = dc->w.xformWorld2Vport;
  newdc->w.xformVport2World = dc->w.xformVport2World;
  newdc->w.vport2WorldValid = dc->w.vport2WorldValid;
#endif
  newdc->wndOrgX            = dc->wndOrgX;
  newdc->wndOrgY            = dc->wndOrgY;
  newdc->wndExtX            = dc->wndExtX;
  newdc->wndExtY            = dc->wndExtY;
  newdc->vportOrgX          = dc->vportOrgX;
  newdc->vportOrgY          = dc->vportOrgY;
  newdc->vportExtX          = dc->vportExtX;
  newdc->vportExtY          = dc->vportExtY;

  newdc->hSelf = hnewdc;
  newdc->saveLevel = 0;

#if 0
  PATH_InitGdiPath( &newdc->w.path );
#endif

  /* Get/SetDCState() don't change hVisRgn field ("Undoc. Windows" p.559). */

#if 0
  newdc->w.hGCClipRgn = newdc->w.hVisRgn = 0;
#endif
  if (dc->w.hClipRgn)
  {
    newdc->w.hClipRgn = NtGdiCreateRectRgn( 0, 0, 0, 0 );
    NtGdiCombineRgn( newdc->w.hClipRgn, dc->w.hClipRgn, 0, RGN_COPY );
  }
  else
  {
    newdc->w.hClipRgn = 0;
  }
  DC_UnlockDc( hnewdc );
  DC_UnlockDc( hDC );
  return  hnewdc;
}


VOID
STDCALL
NtGdiSetDCState ( HDC hDC, HDC hDCSave )
{
  PDC  dc, dcs;

  dc = DC_LockDc ( hDC );
  if ( dc )
  {
    dcs = DC_LockDc ( hDCSave );
    if ( dcs )
    {
      if ( dcs->w.flags & DC_SAVED )
      {
	dc->w.flags            = dcs->w.flags & ~DC_SAVED;

	dc->w.hFirstBitmap     = dcs->w.hFirstBitmap;

#if 0
	dc->w.hDevice          = dcs->w.hDevice;
#endif

	dc->w.totalExtent      = dcs->w.totalExtent;
	dc->w.ROPmode          = dcs->w.ROPmode;
	dc->w.polyFillMode     = dcs->w.polyFillMode;
	dc->w.stretchBltMode   = dcs->w.stretchBltMode;
	dc->w.relAbsMode       = dcs->w.relAbsMode;
	dc->w.backgroundMode   = dcs->w.backgroundMode;
	dc->w.backgroundColor  = dcs->w.backgroundColor;
	dc->w.textColor        = dcs->w.textColor;
	dc->w.brushOrgX        = dcs->w.brushOrgX;
	dc->w.brushOrgY        = dcs->w.brushOrgY;
	dc->w.textAlign        = dcs->w.textAlign;
	dc->w.charExtra        = dcs->w.charExtra;
	dc->w.breakTotalExtra  = dcs->w.breakTotalExtra;
	dc->w.breakCount       = dcs->w.breakCount;
	dc->w.breakExtra       = dcs->w.breakExtra;
	dc->w.breakRem         = dcs->w.breakRem;
	dc->w.MapMode          = dcs->w.MapMode;
	dc->w.GraphicsMode     = dcs->w.GraphicsMode;
#if 0
	/* Apparently, the DC origin is not changed by [GS]etDCState */
	dc->w.DCOrgX           = dcs->w.DCOrgX;
	dc->w.DCOrgY           = dcs->w.DCOrgY;
#endif
	dc->w.CursPosX         = dcs->w.CursPosX;
	dc->w.CursPosY         = dcs->w.CursPosY;
	dc->w.ArcDirection     = dcs->w.ArcDirection;

#if 0
	dc->w.xformWorld2Wnd   = dcs->w.xformWorld2Wnd;
	dc->w.xformWorld2Vport = dcs->w.xformWorld2Vport;
	dc->w.xformVport2World = dcs->w.xformVport2World;
	dc->w.vport2WorldValid = dcs->w.vport2WorldValid;
#endif

	dc->wndOrgX            = dcs->wndOrgX;
	dc->wndOrgY            = dcs->wndOrgY;
	dc->wndExtX            = dcs->wndExtX;
	dc->wndExtY            = dcs->wndExtY;
	dc->vportOrgX          = dcs->vportOrgX;
	dc->vportOrgY          = dcs->vportOrgY;
	dc->vportExtX          = dcs->vportExtX;
	dc->vportExtY          = dcs->vportExtY;

	if (!(dc->w.flags & DC_MEMORY))
	{
	  dc->w.bitsPerPixel = dcs->w.bitsPerPixel;
	}

#if 0
	if (dcs->w.hClipRgn)
	{
	  if (!dc->w.hClipRgn)
	  {
	    dc->w.hClipRgn = NtGdiCreateRectRgn( 0, 0, 0, 0 );
	  }
	  NtGdiCombineRgn( dc->w.hClipRgn, dcs->w.hClipRgn, 0, RGN_COPY );
	}
	else
	{
	  if (dc->w.hClipRgn)
	  {
	    NtGdiDeleteObject( dc->w.hClipRgn );
	  }

	  dc->w.hClipRgn = 0;
	}
	CLIPPING_UpdateGCRegion( dc );
	DC_UnlockDc ( hDC );
#else
	DC_UnlockDc ( hDC );
	NtGdiSelectClipRgn(hDC, dcs->w.hClipRgn);
#endif

	NtGdiSelectObject( hDC, dcs->w.hBitmap );
	NtGdiSelectObject( hDC, dcs->w.hBrush );
	NtGdiSelectObject( hDC, dcs->w.hFont );
	NtGdiSelectObject( hDC, dcs->w.hPen );
	NtGdiSetBkColor( hDC, dcs->w.backgroundColor);
	NtGdiSetTextColor( hDC, dcs->w.textColor);

#if 0
	GDISelectPalette16( hDC, dcs->w.hPalette, FALSE );
#endif
      } else {
	DC_UnlockDc(hDC);      
      }
      DC_UnlockDc ( hDCSave );
    } else {
      DC_UnlockDc ( hDC );
    }
  }
}

INT STDCALL
NtGdiGetDeviceCaps(HDC  hDC,
                  INT  Index)
{
  PDC  dc;
  INT  ret;
  POINT  pt;

  dc = DC_LockDc(hDC);
  if (dc == NULL)
  {
    return 0;
  }

  /* Retrieve capability */
  switch (Index)
  {
    case DRIVERVERSION:
      ret = dc->GDIInfo->ulVersion;
      break;

    case TECHNOLOGY:
      ret = dc->GDIInfo->ulTechnology;
      break;

    case HORZSIZE:
      ret = dc->GDIInfo->ulHorzSize;
      break;

    case VERTSIZE:
      ret = dc->GDIInfo->ulVertSize;
      break;

    case HORZRES:
      ret = dc->GDIInfo->ulHorzRes;
      break;

    case VERTRES:
      ret = dc->GDIInfo->ulVertRes;
      break;

    case LOGPIXELSX:
      ret = dc->GDIInfo->ulLogPixelsX;
      break;

    case LOGPIXELSY:
      ret = dc->GDIInfo->ulLogPixelsY;
      break;

    case BITSPIXEL:
      ret = dc->GDIInfo->cBitsPixel;
      break;

    case PLANES:
      ret = dc->GDIInfo->cPlanes;
      break;

    case NUMBRUSHES:
      UNIMPLEMENTED; /* FIXME */
      break;

    case NUMPENS:
      UNIMPLEMENTED; /* FIXME */
      break;

    case NUMFONTS:
      UNIMPLEMENTED; /* FIXME */
      break;

    case NUMCOLORS:
      ret = dc->GDIInfo->ulNumColors;
      break;

    case ASPECTX:
      ret = dc->GDIInfo->ulAspectX;
      break;

    case ASPECTY:
      ret = dc->GDIInfo->ulAspectY;
      break;

    case ASPECTXY:
      ret = dc->GDIInfo->ulAspectXY;
      break;

    case PDEVICESIZE:
      UNIMPLEMENTED; /* FIXME */
      break;

    case CLIPCAPS:
      UNIMPLEMENTED; /* FIXME */
      break;

    case SIZEPALETTE:
      ret = dc->GDIInfo->ulNumPalReg; /* FIXME not sure */
      break;

    case NUMRESERVED:
      UNIMPLEMENTED; /* FIXME */
      break;

    case COLORRES:
      UNIMPLEMENTED; /* FIXME */
      break;

    case PHYSICALWIDTH:
      if(NtGdiEscape(hDC, GETPHYSPAGESIZE, 0, NULL, (LPVOID)&pt) > 0)
      {
        ret = pt.x;
      }
      else
      {
	ret = 0;
      }
      break;

    case PHYSICALHEIGHT:
      if(NtGdiEscape(hDC, GETPHYSPAGESIZE, 0, NULL, (LPVOID)&pt) > 0)
      {
        ret = pt.y;
      }
      else
      {
	ret = 0;
      }
      break;

    case PHYSICALOFFSETX:
      if(NtGdiEscape(hDC, GETPRINTINGOFFSET, 0, NULL, (LPVOID)&pt) > 0)
      {
        ret = pt.x;
      }
      else
      {
	ret = 0;
      }
      break;

    case PHYSICALOFFSETY:
      if(NtGdiEscape(hDC, GETPRINTINGOFFSET, 0, NULL, (LPVOID)&pt) > 0)
      {
        ret = pt.y;
      }
      else
      {
	ret = 0;
      }
      break;

    case VREFRESH:
      UNIMPLEMENTED; /* FIXME */
      break;

    case SCALINGFACTORX:
      if(NtGdiEscape(hDC, GETSCALINGFACTOR, 0, NULL, (LPVOID)&pt) > 0)
      {
        ret = pt.x;
      }
      else
      {
	ret = 0;
      }
      break;

    case SCALINGFACTORY:
      if(NtGdiEscape(hDC, GETSCALINGFACTOR, 0, NULL, (LPVOID)&pt) > 0)
      {
        ret = pt.y;
      }
      else
      {
	ret = 0;
      }
      break;

    case RASTERCAPS:
      UNIMPLEMENTED; /* FIXME */
      break;

    case CURVECAPS:
      UNIMPLEMENTED; /* FIXME */
      break;

    case LINECAPS:
      UNIMPLEMENTED; /* FIXME */
      break;

    case POLYGONALCAPS:
      UNIMPLEMENTED; /* FIXME */
      break;

    case TEXTCAPS:
      ret = dc->GDIInfo->flTextCaps;
      break;

    default:
      ret = 0;
      break;
  }

  DPRINT("(%04x,%d): returning %d\n", hDC, Index, ret);

  DC_UnlockDc( hDC );
  return ret;
}

DC_GET_VAL( INT, NtGdiGetMapMode, w.MapMode )
DC_GET_VAL( INT, NtGdiGetPolyFillMode, w.polyFillMode )

INT STDCALL
NtGdiGetObjectA(HANDLE handle, INT count, LPVOID buffer)
{
  PGDIOBJ  gdiObject;
  INT  result = 0;
  DWORD objectType;

  if (!count)
    return  0;
  gdiObject = GDIOBJ_LockObj (handle, GDI_OBJECT_TYPE_DONTCARE);
  if (gdiObject == 0)
    return  0;

  objectType = GDIOBJ_GetObjectType(handle);
  switch(objectType)
  {
/*    case GDI_OBJECT_TYPE_PEN:
      result = PEN_GetObject((PENOBJ *)gdiObject, count, buffer);
      break;
    case GDI_OBJECT_TYPE_BRUSH:
      result = BRUSH_GetObject((BRUSHOBJ *)gdiObject, count, buffer);
      break; */
    case GDI_OBJECT_TYPE_BITMAP:
      result = BITMAP_GetObject((BITMAPOBJ *)gdiObject, count, buffer);
      break;
/*    case GDI_OBJECT_TYPE_FONT:
      result = FONT_GetObjectA((FONTOBJ *)gdiObject, count, buffer);

      // FIXME: Fix the LOGFONT structure for the stock fonts

      if ( (handle >= FIRST_STOCK_HANDLE) && (handle <= LAST_STOCK_HANDLE) )
        FixStockFontSizeA(handle, count, buffer);
      break;
    case GDI_OBJECT_TYPE_PALETTE:
      result = PALETTE_GetObject((PALETTEOBJ *)gdiObject, count, buffer);
      break; */

    case GDI_OBJECT_TYPE_REGION:
    case GDI_OBJECT_TYPE_DC:
    case GDI_OBJECT_TYPE_METADC:
    case GDI_OBJECT_TYPE_METAFILE:
    case GDI_OBJECT_TYPE_ENHMETADC:
    case GDI_OBJECT_TYPE_EMF:
      DPRINT1("GDI object type 0x%08x not implemented\n", objectType);
      break;

    default:
      DPRINT1("Invalid GDI object type 0x%08x\n", objectType);
      break;
  }
  GDIOBJ_UnlockObj(handle, GDI_OBJECT_TYPE_DONTCARE);

  return result;
}

INT STDCALL
NtGdiGetObjectW(HANDLE handle, INT count, LPVOID buffer)
{
  PGDIOBJHDR  gdiObject;
  INT  result = 0;
  DWORD objectType;

  if (!count)
    return 0;
  gdiObject = GDIOBJ_LockObj(handle, GDI_OBJECT_TYPE_DONTCARE);
  if (gdiObject == 0)
    return 0;

  objectType = GDIOBJ_GetObjectType(handle);
  switch(objectType)
  {
/*    case GDI_OBJECT_TYPE_PEN:
      result = PEN_GetObject((PENOBJ *)gdiObject, count, buffer);
      break;
    case GDI_OBJECT_TYPE_BRUSH:
      result = BRUSH_GetObject((BRUSHOBJ *)gdiObject, count, buffer);
       break; */
    case GDI_OBJECT_TYPE_BITMAP:
      result = BITMAP_GetObject((BITMAPOBJ *)gdiObject, count, buffer);
      break;
/*    case GDI_OBJECT_TYPE_FONT:
      result = FONT_GetObjectW((FONTOBJ *)gdiObject, count, buffer);

      // Fix the LOGFONT structure for the stock fonts

      if ( (handle >= FIRST_STOCK_HANDLE) && (handle <= LAST_STOCK_HANDLE) )
      FixStockFontSizeW(handle, count, buffer);
    break;
    case GDI_OBJECT_TYPE_PALETTE:
      result = PALETTE_GetObject((PALETTEOBJ *)gdiObject, count, buffer);
      break; */
    default:
      DPRINT1("GDI object type 0x%08x not implemented\n", objectType);
      break;
  }

  GDIOBJ_UnlockObj(handle, GDI_OBJECT_TYPE_DONTCARE);

  return result;
}

INT STDCALL
NtGdiGetObject(HANDLE handle, INT count, LPVOID buffer)
{
  return NtGdiGetObjectW(handle, count, buffer);
}

DWORD STDCALL
NtGdiGetObjectType(HANDLE handle)
{
  GDIOBJHDR * ptr;
  INT result = 0;
  DWORD objectType;

  ptr = GDIOBJ_LockObj(handle, GDI_OBJECT_TYPE_DONTCARE);
  if (ptr == 0)
    return 0;

  objectType = GDIOBJ_GetObjectType(handle);
  switch(objectType)
  {
    case GDI_OBJECT_TYPE_PEN:
      result = OBJ_PEN;
      break;
    case GDI_OBJECT_TYPE_BRUSH:
      result = OBJ_BRUSH;
      break;
    case GDI_OBJECT_TYPE_BITMAP:
      result = OBJ_BITMAP;
      break;
    case GDI_OBJECT_TYPE_FONT:
      result = OBJ_FONT;
      break;
    case GDI_OBJECT_TYPE_PALETTE:
      result = OBJ_PAL;
      break;
    case GDI_OBJECT_TYPE_REGION:
      result = OBJ_REGION;
      break;
    case GDI_OBJECT_TYPE_DC:
      result = OBJ_DC;
      break;
    case GDI_OBJECT_TYPE_METADC:
      result = OBJ_METADC;
      break;
    case GDI_OBJECT_TYPE_METAFILE:
      result = OBJ_METAFILE;
      break;
    case GDI_OBJECT_TYPE_ENHMETAFILE:
      result = OBJ_ENHMETAFILE;
      break;
    case GDI_OBJECT_TYPE_ENHMETADC:
      result = OBJ_ENHMETADC;
      break;
    case GDI_OBJECT_TYPE_EXTPEN:
      result = OBJ_EXTPEN;
      break;
    case GDI_OBJECT_TYPE_MEMDC:
      result = OBJ_MEMDC;
      break;
    default:
      DPRINT1("Magic 0x%08x not implemented\n", objectType);
      break;
  }
  GDIOBJ_UnlockObj(handle, GDI_OBJECT_TYPE_DONTCARE);
  return result;
}

DC_GET_VAL( INT, NtGdiGetRelAbs, w.relAbsMode )
DC_GET_VAL( INT, NtGdiGetROP2, w.ROPmode )
DC_GET_VAL( INT, NtGdiGetStretchBltMode, w.stretchBltMode )
DC_GET_VAL( UINT, NtGdiGetTextAlign, w.textAlign )
DC_GET_VAL( COLORREF, NtGdiGetTextColor, w.textColor )
DC_GET_VAL_EX( NtGdiGetViewportExtEx, IntGetViewportExtEx, vportExtX, vportExtY, SIZE )
DC_GET_VAL_EX( NtGdiGetViewportOrgEx, IntGetViewportOrgEx, vportOrgX, vportOrgY, POINT )
DC_GET_VAL_EX( NtGdiGetWindowExtEx, IntGetWindowExtEx, wndExtX, wndExtY, SIZE )
DC_GET_VAL_EX( NtGdiGetWindowOrgEx, IntGetWindowOrgEx, wndOrgX, wndOrgY, POINT )

HDC STDCALL
NtGdiResetDC(HDC  hDC, CONST DEVMODEW *InitData)
{
  UNIMPLEMENTED;
}

BOOL STDCALL
NtGdiRestoreDC(HDC  hDC, INT  SaveLevel)
{
  PDC  dc, dcs;
  BOOL  success;

  dc = DC_LockDc(hDC);
  if(!dc)
  {
    return FALSE;
  }

  if (SaveLevel == -1)
  {
    SaveLevel = dc->saveLevel;
  }

  if ((SaveLevel < 1) || (SaveLevel > dc->saveLevel))
  {
    return FALSE;
  }

  success = TRUE;
  while (dc->saveLevel >= SaveLevel)
  {
    HDC hdcs = DC_GetNextDC (dc);

    dcs = DC_LockDc (hdcs);
    if (dcs == NULL)
    {
      return FALSE;
    }
    DC_SetNextDC (dcs, DC_GetNextDC (dcs));
    if (--dc->saveLevel < SaveLevel)
      {
	DC_UnlockDc( hDC );
        DC_UnlockDc( hdcs );
        NtGdiSetDCState(hDC, hdcs);
#if 0
        if (!PATH_AssignGdiPath( &dc->w.path, &dcs->w.path ))
        {
          /* FIXME: This might not be quite right, since we're
           * returning FALSE but still destroying the saved DC state */
          success = FALSE;
        }
#endif
	dc = DC_LockDc(hDC);
	if(!dc)
	{
	  return FALSE;
	}
      }
    else
      {
      DC_UnlockDc( hdcs );
      }
    NtGdiDeleteDC (hdcs);
  }
  DC_UnlockDc( hDC );
  return  success;
}

INT STDCALL
NtGdiSaveDC(HDC  hDC)
{
  HDC  hdcs;
  PDC  dc, dcs;
  INT  ret;

  if (!(hdcs = NtGdiGetDCState(hDC)))
  {
    return 0;
  }

  dcs = DC_LockDc (hdcs);
  if (dcs == NULL)
  {
    return 0;
  }
  dc = DC_LockDc (hDC);
  if (dc == NULL)
  {
    DC_UnlockDc(dc);
    return 0;
  }

#if 0
    /* Copy path. The reason why path saving / restoring is in SaveDC/
     * RestoreDC and not in GetDCState/SetDCState is that the ...DCState
     * functions are only in Win16 (which doesn't have paths) and that
     * SetDCState doesn't allow us to signal an error (which can happen
     * when copying paths).
     */
  if (!PATH_AssignGdiPath (&dcs->w.path, &dc->w.path))
  {
    NtGdiDeleteDC (hdcs);
    return 0;
  }
#endif

  DC_SetNextDC (dcs, DC_GetNextDC (dc));
  DC_SetNextDC (dc, hdcs);
  ret = ++dc->saveLevel;
  DC_UnlockDc( hdcs );
  DC_UnlockDc( hDC );

  return  ret;
}

HGDIOBJ
STDCALL
NtGdiSelectObject(HDC  hDC, HGDIOBJ  hGDIObj)
{
  HGDIOBJ objOrg = NULL; // default to failure
  BITMAPOBJ *pb;
  PDC dc;
  PPENOBJ pen;
  PBRUSHOBJ brush;
  PXLATEOBJ XlateObj;
  PPALGDI PalGDI;
  DWORD objectType;
  COLORREF *ColorMap;
  COLORREF MonoColorMap[2];
  ULONG NumColors, Index;
  HRGN hVisRgn;
  USHORT Mode;

  if(!hDC || !hGDIObj) return NULL;

  dc = DC_LockDc(hDC);
  if (NULL == dc)
    {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return NULL;
    }

  objectType = GDIOBJ_GetObjectType(hGDIObj);

  switch (objectType)
  {
    case GDI_OBJECT_TYPE_PEN:
      objOrg = NULL;
      /* Convert the color of the pen to the format of the DC */
      PalGDI = PALETTE_LockPalette(dc->w.hPalette);
      if (NULL != PalGDI)
        {
          Mode = PalGDI->Mode;
          PALETTE_UnlockPalette(dc->w.hPalette);
          XlateObj = (PXLATEOBJ)IntEngCreateXlate(Mode, PAL_RGB, dc->w.hPalette, NULL);
          if (NULL != XlateObj)
            {
              pen = PENOBJ_LockPen((HPEN) hGDIObj);
              if (NULL != pen)
                {
                  pen->iSolidColor = XLATEOBJ_iXlate(XlateObj, pen->logpen.lopnColor);
                  PENOBJ_UnlockPen((HPEN) hGDIObj);
                  objOrg = (HGDIOBJ)dc->w.hPen;
                  dc->w.hPen = hGDIObj;
                }
              else
                {
                  SetLastWin32Error(ERROR_INVALID_HANDLE);
                }
              EngDeleteXlate(XlateObj);
	    }
          else
            {
              SetLastWin32Error(ERROR_NO_SYSTEM_RESOURCES);
            }
        }
      else
        {
          SetLastWin32Error(ERROR_INVALID_HANDLE);
        }
      break;

    case GDI_OBJECT_TYPE_BRUSH:
      objOrg = NULL;
      /* Convert the color of the brush to the format of the DC */
      PalGDI = PALETTE_LockPalette(dc->w.hPalette);
      if (NULL != PalGDI)
        {
	  Mode = PalGDI->Mode;
          PALETTE_UnlockPalette(dc->w.hPalette);
          XlateObj = (PXLATEOBJ)IntEngCreateXlate(Mode, PAL_RGB, dc->w.hPalette, NULL);
          if (NULL != XlateObj)
            {
              brush = BRUSHOBJ_LockBrush((HBRUSH) hGDIObj);
              if (NULL != brush)
                {
                  brush->iSolidColor = XLATEOBJ_iXlate(XlateObj, brush->logbrush.lbColor);
                  BRUSHOBJ_UnlockBrush((HBRUSH) hGDIObj);
                  objOrg = (HGDIOBJ)dc->w.hBrush;
                  dc->w.hBrush = (HBRUSH) hGDIObj;
                }
              else
                {
                  SetLastWin32Error(ERROR_INVALID_HANDLE);
                }
              EngDeleteXlate(XlateObj);
            }
          else
            {
              SetLastWin32Error(ERROR_NO_SYSTEM_RESOURCES);
            }
        }
      else
        {
          SetLastWin32Error(ERROR_INVALID_HANDLE);
        }
      break;

    case GDI_OBJECT_TYPE_FONT:
      objOrg = (HGDIOBJ)dc->w.hFont;
      dc->w.hFont = (HFONT) hGDIObj;
      TextIntRealizeFont(dc->w.hFont);
      break;

    case GDI_OBJECT_TYPE_BITMAP:
      // must be memory dc to select bitmap
      if (!(dc->w.flags & DC_MEMORY))
        {
          DC_UnlockDc(hDC);
          return NULL;
        }
      pb = BITMAPOBJ_LockBitmap(hGDIObj);
      if (NULL == pb)
	{
	  SetLastWin32Error(ERROR_INVALID_HANDLE);
          DC_UnlockDc(hDC);
	  return NULL;
	}
      objOrg = (HGDIOBJ)dc->w.hBitmap;

      /* Release the old bitmap, lock the new one and convert it to a SURF */
      EngDeleteSurface(dc->Surface);
      dc->w.hBitmap = hGDIObj;
      dc->Surface = BitmapToSurf(pb);

      // if we're working with a DIB, get the palette [fixme: only create if the selected palette is null]
      if(pb->dib)
      {
        dc->w.bitsPerPixel = pb->dib->dsBmih.biBitCount;

        if(pb->dib->dsBmih.biBitCount <= 8)
        {
          if(pb->dib->dsBmih.biBitCount == 1) { NumColors = 2; } else
          if(pb->dib->dsBmih.biBitCount == 4) { NumColors = 16; } else
          if(pb->dib->dsBmih.biBitCount == 8) { NumColors = 256; }

          ColorMap = ExAllocatePoolWithTag(PagedPool, sizeof(COLORREF) * NumColors, TAG_DC);
          ASSERT(ColorMap);
          for (Index = 0; Index < NumColors; Index++)
          {
            ColorMap[Index] = RGB(pb->ColorMap[Index].rgbRed,
                                  pb->ColorMap[Index].rgbGreen,
                                  pb->ColorMap[Index].rgbBlue);
          }
          dc->w.hPalette = PALETTE_AllocPalette(PAL_INDEXED, NumColors, (ULONG *) ColorMap, 0, 0, 0);
          ExFreePool(ColorMap);
        }
        else if ( 16 == pb->dib->dsBmih.biBitCount )
        {
          dc->w.hPalette = PALETTE_AllocPalette(PAL_BITFIELDS, pb->dib->dsBmih.biClrUsed, NULL, 0x7c00, 0x03e0, 0x001f);
        }
        else if(pb->dib->dsBmih.biBitCount >= 24)
        {
          dc->w.hPalette = PALETTE_AllocPalette(PAL_RGB, pb->dib->dsBmih.biClrUsed, NULL, 0, 0, 0);
        }
      }
      else
      {
        dc->w.bitsPerPixel = pb->bitmap.bmBitsPixel;
        if (1 == dc->w.bitsPerPixel)
          {
            MonoColorMap[0] = RGB(0, 0, 0);
            MonoColorMap[1] = RGB(255, 255, 255);
            dc->w.hPalette = PALETTE_AllocPalette(PAL_INDEXED, 2, MonoColorMap, 0, 0, 0);
          }
        else
          {
            dc->w.hPalette = dc->DevInfo->hpalDefault;
          }
      }

      DC_UnlockDc ( hDC );
      hVisRgn = NtGdiCreateRectRgn ( 0, 0, pb->size.cx, pb->size.cy );
      NtGdiSelectVisRgn ( hDC, hVisRgn );
      NtGdiDeleteObject ( hVisRgn );
      BITMAPOBJ_UnlockBitmap(hGDIObj);

      return objOrg;

#if UPDATEREGIONS
    case GDI_OBJECT_TYPE_REGION:
      DC_UnlockDc ( hDC );
      SelectClipRgn(hDC, (HRGN)hGDIObj);
      return NULL;
#endif
    default:
      break;
  }
  DC_UnlockDc( hDC );
  return objOrg;
}

WORD STDCALL
NtGdiSetHookFlags(HDC hDC, WORD Flags)
{
  WORD wRet;
  DC *dc = DC_LockDc(hDC);

  if (NULL == dc)
    {
      return 0;
    }

  wRet = dc->w.flags & DC_DIRTY;

  /* "Undocumented Windows" info is slightly confusing.
   */

  DPRINT("DC %p, Flags %04x\n", hDC, Flags);

  if (Flags & DCHF_INVALIDATEVISRGN)
    {
      dc->w.flags |= DC_DIRTY;
    }
  else if (Flags & DCHF_VALIDATEVISRGN || 0 == Flags)
    {
      dc->w.flags &= ~DC_DIRTY;
    }

  DC_UnlockDc(hDC);

  return wRet;
}

DC_SET_MODE( NtGdiSetBkMode, w.backgroundMode, TRANSPARENT, OPAQUE )
DC_SET_MODE( NtGdiSetPolyFillMode, w.polyFillMode, ALTERNATE, WINDING )
// DC_SET_MODE( NtGdiSetRelAbs, w.relAbsMode, ABSOLUTE, RELATIVE )
DC_SET_MODE( NtGdiSetROP2, w.ROPmode, R2_BLACK, R2_WHITE )
DC_SET_MODE( NtGdiSetStretchBltMode, w.stretchBltMode, BLACKONWHITE, HALFTONE )

//  ----------------------------------------------------  Private Interface

HDC FASTCALL
DC_AllocDC(LPCWSTR Driver)
{
  PDC  NewDC;
  HDC  hDC;

  hDC = (HDC) GDIOBJ_AllocObj(sizeof(DC), GDI_OBJECT_TYPE_DC, (GDICLEANUPPROC) DC_InternalDeleteDC);
  if (hDC == NULL)
  {
    return  NULL;
  }

  NewDC = DC_LockDc(hDC);

  if (Driver != NULL)
  {
    NewDC->DriverName = ExAllocatePoolWithTag(PagedPool, (wcslen(Driver) + 1) * sizeof(WCHAR), TAG_DC);
    wcscpy(NewDC->DriverName, Driver);
  }

  NewDC->w.xformWorld2Wnd.eM11 = 1.0f;
  NewDC->w.xformWorld2Wnd.eM12 = 0.0f;
  NewDC->w.xformWorld2Wnd.eM21 = 0.0f;
  NewDC->w.xformWorld2Wnd.eM22 = 1.0f;
  NewDC->w.xformWorld2Wnd.eDx = 0.0f;
  NewDC->w.xformWorld2Wnd.eDy = 0.0f;
  NewDC->w.xformWorld2Vport = NewDC->w.xformWorld2Wnd;
  NewDC->w.xformVport2World = NewDC->w.xformWorld2Wnd;
  NewDC->w.vport2WorldValid = TRUE;

  NewDC->w.hFont = NtGdiGetStockObject(SYSTEM_FONT);
  TextIntRealizeFont(NewDC->w.hFont);

  DC_UnlockDc(hDC);

  return  hDC;
}

HDC FASTCALL
DC_FindOpenDC(LPCWSTR  Driver)
{
  return NULL;
}

/*!
 * Initialize some common fields in the Device Context structure.
*/
VOID FASTCALL
DC_InitDC(HDC  DCHandle)
{
//  NtGdiRealizeDefaultPalette(DCHandle);

  NtGdiSelectObject(DCHandle, NtGdiGetStockObject( WHITE_BRUSH ));
  NtGdiSelectObject(DCHandle, NtGdiGetStockObject( BLACK_PEN ));
  //NtGdiSelectObject(DCHandle, hFont);

//  CLIPPING_UpdateGCRegion(DCToInit);

}

VOID FASTCALL
DC_FreeDC(HDC  DCToFree)
{
  if (!GDIOBJ_FreeObj(DCToFree, GDI_OBJECT_TYPE_DC, GDIOBJFLAG_DEFAULT))
  {
    DPRINT("DC_FreeDC failed\n");
  }
}

BOOL FASTCALL
DC_InternalDeleteDC( PDC DCToDelete )
{
  if (! (DCToDelete->w.flags & DC_MEMORY)  /* Don't reset the display if its a memory DC */
      && NULL != DCToDelete->DriverName
      && ! DRIVER_UnreferenceDriver (DCToDelete->DriverName))
    {
      DPRINT( "No more references to driver, reseting display\n" );
      DCToDelete->DriverFunctions.AssertMode( DCToDelete->PDev, FALSE );
      CHECKPOINT;
      DCToDelete->DriverFunctions.DisableSurface(DCToDelete->PDev);
      CHECKPOINT;
      DCToDelete->DriverFunctions.DisablePDev(DCToDelete->PDev);

      ObDereferenceObject(PrimarySurface.VideoDeviceObject);

      PrimarySurfaceCreated = FALSE;
    }

  if (NULL != DCToDelete->DriverName)
    {
      ExFreePool(DCToDelete->DriverName);
    }

  return TRUE;
}

HDC FASTCALL
DC_GetNextDC (PDC pDC)
{
  return pDC->hNext;
}

VOID FASTCALL
DC_SetNextDC (PDC pDC, HDC hNextDC)
{
  pDC->hNext = hNextDC;
}

VOID FASTCALL
DC_UpdateXforms(PDC  dc)
{
  XFORM  xformWnd2Vport;
  FLOAT  scaleX, scaleY;

  /* Construct a transformation to do the window-to-viewport conversion */
  scaleX = (FLOAT)dc->vportExtX / (FLOAT)dc->wndExtX;
  scaleY = (FLOAT)dc->vportExtY / (FLOAT)dc->wndExtY;
  xformWnd2Vport.eM11 = scaleX;
  xformWnd2Vport.eM12 = 0.0;
  xformWnd2Vport.eM21 = 0.0;
  xformWnd2Vport.eM22 = scaleY;
  xformWnd2Vport.eDx  = (FLOAT)dc->vportOrgX - scaleX * (FLOAT)dc->wndOrgX;
  xformWnd2Vport.eDy  = (FLOAT)dc->vportOrgY - scaleY * (FLOAT)dc->wndOrgY;

  /* Combine with the world transformation */
  NtGdiCombineTransform(&dc->w.xformWorld2Vport, &dc->w.xformWorld2Wnd, &xformWnd2Vport);

  /* Create inverse of world-to-viewport transformation */
  dc->w.vport2WorldValid = DC_InvertXform(&dc->w.xformWorld2Vport, &dc->w.xformVport2World);
}

BOOL FASTCALL
DC_InvertXform(const XFORM *xformSrc,
               XFORM *xformDest)
{
  FLOAT  determinant;

  determinant = xformSrc->eM11*xformSrc->eM22 - xformSrc->eM12*xformSrc->eM21;
  if (determinant > -1e-12 && determinant < 1e-12)
  {
    return  FALSE;
  }

  xformDest->eM11 =  xformSrc->eM22 / determinant;
  xformDest->eM12 = -xformSrc->eM12 / determinant;
  xformDest->eM21 = -xformSrc->eM21 / determinant;
  xformDest->eM22 =  xformSrc->eM11 / determinant;
  xformDest->eDx  = -xformSrc->eDx * xformDest->eM11 - xformSrc->eDy * xformDest->eM21;
  xformDest->eDy  = -xformSrc->eDx * xformDest->eM12 - xformSrc->eDy * xformDest->eM22;

  return  TRUE;
}
/* EOF */
