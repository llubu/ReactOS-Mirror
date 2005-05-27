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
/* $Id$
 *
 * DC.C - Device context functions
 *
 */
#include <w32k.h>

#ifndef OBJ_COLORSPACE
#define OBJ_COLORSPACE	(14)
#endif

static GDIDEVICE PrimarySurface;

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
    SetLastWin32Error(ERROR_INVALID_HANDLE); \
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
#define DC_GET_VAL_EX( FuncName, ret_x, ret_y, type, ax, ay ) \
VOID FASTCALL Int##FuncName ( PDC dc, LP##type pt) \
{ \
  ASSERT(dc); \
  ASSERT(pt); \
  pt->ax = dc->ret_x; \
  pt->ay = dc->ret_y; \
} \
BOOL STDCALL NtGdi##FuncName ( HDC hdc, LP##type pt ) \
{ \
  NTSTATUS Status; \
  type Safept; \
  PDC dc; \
  if(!pt) \
  { \
    SetLastWin32Error(ERROR_INVALID_PARAMETER); \
    return FALSE; \
  } \
  if(!(dc = DC_LockDc(hdc))) \
  { \
    SetLastWin32Error(ERROR_INVALID_HANDLE); \
    return FALSE; \
  } \
  Int##FuncName( dc, &Safept); \
  DC_UnlockDc(hdc); \
  Status = MmCopyToCaller(pt, &Safept, sizeof( type )); \
  if(!NT_SUCCESS(Status)) \
  { \
    SetLastNtError(Status); \
    return FALSE; \
  } \
  return TRUE; \
}

#define DC_SET_MODE( func_name, dc_field, min_val, max_val ) \
INT STDCALL  func_name( HDC hdc, INT mode ) \
{                                           \
  INT  prevMode;                            \
  PDC  dc;                                  \
  if ((mode < min_val) || (mode > max_val)) \
  { \
    SetLastWin32Error(ERROR_INVALID_PARAMETER); \
    return 0;                               \
  } \
  dc = DC_LockDc ( hdc );              \
  if ( !dc )                                \
  { \
    SetLastWin32Error(ERROR_INVALID_HANDLE); \
    return 0;                               \
  } \
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
  return FALSE;
}

HDC STDCALL
NtGdiCreateCompatableDC(HDC hDC)
{
  PDC  NewDC, OrigDC;
  HBITMAP  hBitmap;
  HDC hNewDC, DisplayDC;
  HRGN hVisRgn;
  UNICODE_STRING DriverName;

  DisplayDC = NULL;
  if (hDC == NULL)
    {
      RtlInitUnicodeString(&DriverName, L"DISPLAY");
      DisplayDC = IntGdiCreateDC(&DriverName, NULL, NULL, NULL, TRUE);
      if (NULL == DisplayDC)
        {
          return NULL;
        }
      hDC = DisplayDC;
    }

  /*  Allocate a new DC based on the original DC's device  */
  OrigDC = DC_LockDc(hDC);
  if (NULL == OrigDC)
    {
      if (NULL != DisplayDC)
        {
          NtGdiDeleteDC(DisplayDC);
        }
      return NULL;
    }
  hNewDC = DC_AllocDC(&OrigDC->DriverName);

  if (NULL == hNewDC)
    {
      DC_UnlockDc(hDC);
      if (NULL != DisplayDC)
        {
          NtGdiDeleteDC(DisplayDC);
        }
      return  NULL;
    }
  NewDC = DC_LockDc( hNewDC );

  /* Copy information from original DC to new DC  */
  NewDC->hSelf = hNewDC;
  NewDC->IsIC = FALSE;

  NewDC->PDev = OrigDC->PDev;
  memcpy(NewDC->FillPatternSurfaces,
         OrigDC->FillPatternSurfaces,
         sizeof OrigDC->FillPatternSurfaces);
  NewDC->GDIInfo = OrigDC->GDIInfo;
  NewDC->DevInfo = OrigDC->DevInfo;
  NewDC->w.bitsPerPixel = OrigDC->w.bitsPerPixel;

  /* DriverName is copied in the AllocDC routine  */
  NewDC->DeviceDriver = OrigDC->DeviceDriver;
  NewDC->wndOrgX = OrigDC->wndOrgX;
  NewDC->wndOrgY = OrigDC->wndOrgY;
  NewDC->wndExtX = OrigDC->wndExtX;
  NewDC->wndExtY = OrigDC->wndExtY;
  NewDC->vportOrgX = OrigDC->vportOrgX;
  NewDC->vportOrgY = OrigDC->vportOrgY;
  NewDC->vportExtX = OrigDC->vportExtX;
  NewDC->vportExtY = OrigDC->vportExtY;

  /* Create default bitmap */
  if (!(hBitmap = NtGdiCreateBitmap( 1, 1, 1, NewDC->w.bitsPerPixel, NULL )))
    {
      DC_UnlockDc( hDC );
      DC_UnlockDc( hNewDC );
      DC_FreeDC( hNewDC );
      if (NULL != DisplayDC)
        {
          NtGdiDeleteDC(DisplayDC);
        }
      return NULL;
    }
  NewDC->w.flags        = DC_MEMORY;
  NewDC->w.hBitmap      = hBitmap;
  NewDC->w.hFirstBitmap = hBitmap;
  NewDC->GDIDevice      = OrigDC->GDIDevice;

  NewDC->w.hPalette = OrigDC->w.hPalette;
  NewDC->w.textColor = OrigDC->w.textColor;
  NewDC->w.textAlign = OrigDC->w.textAlign;
  NewDC->w.backgroundColor = OrigDC->w.backgroundColor;
  NewDC->w.backgroundMode = OrigDC->w.backgroundMode;
  NewDC->w.ROPmode = OrigDC->w.ROPmode;
  DC_UnlockDc( hDC );
  if (NULL != DisplayDC)
    {
      NtGdiDeleteDC(DisplayDC);
    }
  DC_UnlockDc(hNewDC);

  hVisRgn = NtGdiCreateRectRgn(0, 0, 1, 1);
  NtGdiSelectVisRgn(hNewDC, hVisRgn);
  NtGdiDeleteObject(hVisRgn);

  DC_InitDC(hNewDC);

  return hNewDC;
}

static BOOL FASTCALL
GetRegistryPath(PUNICODE_STRING RegistryPath, ULONG DisplayNumber)
{
  RTL_QUERY_REGISTRY_TABLE QueryTable[2];
  WCHAR DeviceNameBuffer[20];
  NTSTATUS Status;

  swprintf(DeviceNameBuffer, L"\\Device\\Video%d", DisplayNumber);
  RtlInitUnicodeString(RegistryPath, NULL);
  RtlZeroMemory(QueryTable, sizeof(QueryTable));
  QueryTable[0].Flags = RTL_QUERY_REGISTRY_REQUIRED | RTL_QUERY_REGISTRY_DIRECT;
  QueryTable[0].Name = DeviceNameBuffer;
  QueryTable[0].EntryContext = RegistryPath;

  Status = RtlQueryRegistryValues(RTL_REGISTRY_DEVICEMAP,
                                  L"VIDEO",
                                  QueryTable,
                                  NULL,
                                  NULL);
  if (! NT_SUCCESS(Status))
    {
      DPRINT1("No \\Device\\Video%d value in DEVICEMAP\\VIDEO found\n", DisplayNumber);
      return FALSE;
    }

  DPRINT("RegistryPath %S\n", RegistryPath->Buffer);

  return TRUE;
}

static BOOL FASTCALL
FindDriverFileNames(PUNICODE_STRING DriverFileNames, ULONG DisplayNumber)
{
  RTL_QUERY_REGISTRY_TABLE QueryTable[2];
  UNICODE_STRING RegistryPath;
  NTSTATUS Status;

  if (! GetRegistryPath(&RegistryPath, DisplayNumber))
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
SetupDevMode(PDEVMODEW DevMode, ULONG DisplayNumber)
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

  if (! GetRegistryPath(&RegistryPath, DisplayNumber))
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
              DPRINT("RtlQueryRegistryValues for %S failed with status 0x%08x\n",
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

static BOOL FASTCALL
IntPrepareDriver()
{
   PGD_ENABLEDRIVER GDEnableDriver;
   DRVENABLEDATA DED;
   UNICODE_STRING DriverFileNames;
   PWSTR CurrentName;
   BOOL GotDriver;
   BOOL DoDefault;
   ULONG DisplayNumber;

   for (DisplayNumber = 0; ; DisplayNumber++)
   {
      DPRINT("Trying to load display driver no. %d\n", DisplayNumber);

      RtlZeroMemory(&PrimarySurface, sizeof(PrimarySurface));

      PrimarySurface.VideoFileObject = DRIVER_FindMPDriver(DisplayNumber);

      /* Open the miniport driver  */
      if (PrimarySurface.VideoFileObject == NULL)
      {
         DPRINT1("FindMPDriver failed\n");
         return FALSE;
      }

      /* Retrieve DDI driver names from registry */
      RtlInitUnicodeString(&DriverFileNames, NULL);
      if (!FindDriverFileNames(&DriverFileNames, DisplayNumber))
      {
         DPRINT1("FindDriverFileNames failed\n");
         continue;
      }

      /*
       * DriverFileNames may be a list of drivers in REG_SZ_MULTI format,
       * scan all of them until a good one found.
       */
      CurrentName = DriverFileNames.Buffer;
      GotDriver = FALSE;
      while (!GotDriver &&
             CurrentName < DriverFileNames.Buffer + (DriverFileNames.Length / sizeof (WCHAR)))
      {
         /* Get the DDI driver's entry point */
         GDEnableDriver = DRIVER_FindDDIDriver(CurrentName);
         if (NULL == GDEnableDriver)
         {
            DPRINT("FindDDIDriver failed for %S\n", CurrentName);
         }
         else
         {
            /*  Call DDI driver's EnableDriver function  */
            RtlZeroMemory(&DED, sizeof(DED));

            if (! GDEnableDriver(DDI_DRIVER_VERSION_NT5_01, sizeof(DED), &DED))
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
                   CurrentName < DriverFileNames.Buffer + (DriverFileNames.Length / sizeof (WCHAR)))
            {
               CurrentName++;
            }
            if (CurrentName < DriverFileNames.Buffer + (DriverFileNames.Length / sizeof (WCHAR)))
            {
               CurrentName++;
            }
         }
      }

      RtlFreeUnicodeString(&DriverFileNames);

      if (!GotDriver)
      {
         ObDereferenceObject(PrimarySurface.VideoFileObject);
         DPRINT1("No suitable DDI driver found\n");
         continue;
      }

      DPRINT("Display driver %S loaded\n", CurrentName);

      DPRINT("Building DDI Functions\n");

      /* Construct DDI driver function dispatch table */
      if (!DRIVER_BuildDDIFunctions(&DED, &PrimarySurface.DriverFunctions))
      {
         ObDereferenceObject(PrimarySurface.VideoFileObject);
         DPRINT1("BuildDDIFunctions failed\n");
         return FALSE;
      }

      /* Allocate a phyical device handle from the driver */
      PrimarySurface.DMW.dmSize = sizeof (PrimarySurface.DMW);
      if (SetupDevMode(&PrimarySurface.DMW, DisplayNumber))
      {
         PrimarySurface.PDev = PrimarySurface.DriverFunctions.EnablePDEV(
            &PrimarySurface.DMW,
            L"",
            HS_DDI_MAX,
            PrimarySurface.FillPatterns,
            sizeof(PrimarySurface.GDIInfo),
            (ULONG *) &PrimarySurface.GDIInfo,
            sizeof(PrimarySurface.DevInfo),
            &PrimarySurface.DevInfo,
            NULL,
            L"",
            (HANDLE) (PrimarySurface.VideoFileObject->DeviceObject));
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
         PrimarySurface.DMW.dmSize = sizeof (PrimarySurface.DMW);
         PrimarySurface.PDev = PrimarySurface.DriverFunctions.EnablePDEV(
            &PrimarySurface.DMW,
            L"",
            HS_DDI_MAX,
            PrimarySurface.FillPatterns,
            sizeof(PrimarySurface.GDIInfo),
            (ULONG *) &PrimarySurface.GDIInfo,
            sizeof(PrimarySurface.DevInfo),
            &PrimarySurface.DevInfo,
            NULL,
            L"",
            (HANDLE) (PrimarySurface.VideoFileObject->DeviceObject));

         if (NULL == PrimarySurface.PDev)
         {
            ObDereferenceObject(PrimarySurface.VideoFileObject);
            DPRINT1("DrvEnablePDEV with default parameters failed\n");
            DPRINT1("Perhaps DDI driver doesn't match miniport driver?\n");
            continue;
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

      PrimarySurface.Pointer.Exclude.right = -1;

      DPRINT("calling completePDev\n");

      /* Complete initialization of the physical device */
      PrimarySurface.DriverFunctions.CompletePDEV(
         PrimarySurface.PDev,
	 (HDEV)&PrimarySurface);

      DPRINT("calling DRIVER_ReferenceDriver\n");

      DRIVER_ReferenceDriver(L"DISPLAY");

      PrimarySurface.PreparedDriver = TRUE;
      PrimarySurface.DisplayNumber = DisplayNumber;

      return TRUE;
   }

   return FALSE;
}

static BOOL FASTCALL
IntPrepareDriverIfNeeded()
{
   return (PrimarySurface.PreparedDriver ? TRUE : IntPrepareDriver());
}

BOOL FASTCALL
IntCreatePrimarySurface()
{
   SIZEL SurfSize;
   RECTL SurfaceRect;
   SURFOBJ *SurfObj;

   if (! IntPrepareDriverIfNeeded())
   {
      return FALSE;
   }

   DPRINT("calling EnableSurface\n");
   /* Enable the drawing surface */
   PrimarySurface.Handle =
      PrimarySurface.DriverFunctions.EnableSurface(PrimarySurface.PDev);
   if (NULL == PrimarySurface.Handle)
   {
/*      PrimarySurface.DriverFunctions.AssertMode(PrimarySurface.PDev, FALSE);*/
      PrimarySurface.DriverFunctions.DisablePDEV(PrimarySurface.PDev);
      ObDereferenceObject(PrimarySurface.VideoFileObject);
      DPRINT1("DrvEnableSurface failed\n");
      return FALSE;
   }

   PrimarySurface.DriverFunctions.AssertMode(PrimarySurface.PDev, TRUE);

   /* attach monitor */
   IntAttachMonitor(&PrimarySurface, PrimarySurface.DisplayNumber);

   SurfObj = EngLockSurface((HSURF)PrimarySurface.Handle);
   SurfObj->dhpdev = PrimarySurface.PDev;
   SurfSize = SurfObj->sizlBitmap;
   SurfaceRect.left = SurfaceRect.top = 0;
   SurfaceRect.right = SurfObj->sizlBitmap.cx;
   SurfaceRect.bottom = SurfObj->sizlBitmap.cy;
   /* FIXME - why does EngEraseSurface() sometimes crash?
     EngEraseSurface(SurfObj, &SurfaceRect, 0); */

   /* Put the pointer in the center of the screen */
   GDIDEV(SurfObj)->Pointer.Pos.x = (SurfaceRect.right - SurfaceRect.left) / 2;
   GDIDEV(SurfObj)->Pointer.Pos.y = (SurfaceRect.bottom - SurfaceRect.top) / 2;

   EngUnlockSurface(SurfObj);
   IntShowDesktop(IntGetActiveDesktop(), SurfSize.cx, SurfSize.cy);

   return TRUE;
}

VOID FASTCALL
IntDestroyPrimarySurface()
  {
    DRIVER_UnreferenceDriver(L"DISPLAY");

    /* detach monitor */
    IntDetachMonitor(&PrimarySurface);

    /*
     * FIXME: Hide a mouse pointer there. Also because we have to prevent
     * memory leaks with the Eng* mouse routines.
     */

    DPRINT("Reseting display\n" );
    PrimarySurface.DriverFunctions.AssertMode(PrimarySurface.PDev, FALSE);
    PrimarySurface.DriverFunctions.DisableSurface(PrimarySurface.PDev);
    PrimarySurface.DriverFunctions.DisablePDEV(PrimarySurface.PDev);
    PrimarySurface.PreparedDriver = FALSE;

    DceEmptyCache();

    ObDereferenceObject(PrimarySurface.VideoFileObject);
  }

HDC FASTCALL
IntGdiCreateDC(PUNICODE_STRING Driver,
               PUNICODE_STRING Device,
               PUNICODE_STRING Output,
               CONST PDEVMODEW InitData,
               BOOL CreateAsIC)
{
  HDC      hNewDC;
  PDC      NewDC;
  HDC      hDC = NULL;
  SURFOBJ *SurfObj;
  HRGN     hVisRgn;
  UNICODE_STRING StdDriver;

  RtlInitUnicodeString(&StdDriver, L"DISPLAY");

  if (NULL == Driver || 0 == RtlCompareUnicodeString(Driver, &StdDriver, TRUE))
    {
      if (CreateAsIC)
        {
          if (! IntPrepareDriverIfNeeded())
            {
              DPRINT1("Unable to prepare graphics driver, returning NULL ic\n");
              return NULL;
            }
        }
      else if (! IntGraphicsCheck(TRUE))
        {
          DPRINT1("Unable to initialize graphics, returning NULL dc\n");
          return NULL;
        }
    }

  /*  Check for existing DC object  */
  if ((hNewDC = DC_FindOpenDC(Driver)) != NULL)
  {
    hDC = hNewDC;
    return  NtGdiCreateCompatableDC(hDC);
  }

  if (Driver != NULL && Driver->Buffer != NULL)
  {
    DPRINT("NAME: %wZ\n", Driver); // FIXME: Should not crash if NULL
  }

  /*  Allocate a DC object  */
  if ((hNewDC = DC_AllocDC(Driver)) == NULL)
  {
    return  NULL;
  }

  NewDC = DC_LockDc( hNewDC );
  /* FIXME - NewDC can be NULL!!! Don't assert here! */
  if ( !NewDC )
  {
    DC_FreeDC( hNewDC );
    return NULL;
  }

  NewDC->IsIC = CreateAsIC;
  NewDC->DevInfo = &PrimarySurface.DevInfo;
  NewDC->GDIInfo = &PrimarySurface.GDIInfo;
  memcpy(NewDC->FillPatternSurfaces, PrimarySurface.FillPatterns,
	 sizeof(NewDC->FillPatternSurfaces));
  NewDC->PDev = PrimarySurface.PDev;
  NewDC->GDIDevice = (HDEV)&PrimarySurface;
  NewDC->DriverFunctions = PrimarySurface.DriverFunctions;
  NewDC->w.hBitmap = PrimarySurface.Handle;

  NewDC->w.bitsPerPixel = NewDC->GDIInfo->cBitsPixel * NewDC->GDIInfo->cPlanes;
  DPRINT("Bits per pel: %u\n", NewDC->w.bitsPerPixel);

  if (! CreateAsIC)
  {
    SurfObj = EngLockSurface((HSURF)PrimarySurface.Handle);
    if ( !SurfObj )
    {
      DC_UnlockDc ( hNewDC );
      DC_FreeDC ( hNewDC) ;
      return NULL;
    }
    ASSERT(NewDC->GDIInfo->cBitsPixel * NewDC->GDIInfo->cPlanes == BitsPerFormat(SurfObj->iBitmapFormat));
    ASSERT(NewDC->GDIInfo->ulHorzRes == SurfObj->sizlBitmap.cx);
    ASSERT(NewDC->GDIInfo->ulVertRes == SurfObj->sizlBitmap.cy);

    NewDC->w.hPalette = NewDC->DevInfo->hpalDefault;
    NewDC->w.ROPmode = R2_COPYPEN;

    DC_UnlockDc( hNewDC );

    hVisRgn = NtGdiCreateRectRgn(0, 0, SurfObj->sizlBitmap.cx,
                                 SurfObj->sizlBitmap.cy);
    NtGdiSelectVisRgn(hNewDC, hVisRgn);
    NtGdiDeleteObject(hVisRgn);

    /*  Initialize the DC state  */
    DC_InitDC(hNewDC);
    NtGdiSetTextColor(hNewDC, RGB(0, 0, 0));
    NtGdiSetTextAlign(hNewDC, TA_TOP);
    NtGdiSetBkColor(hNewDC, RGB(255, 255, 255));
    NtGdiSetBkMode(hNewDC, OPAQUE);

    EngUnlockSurface(SurfObj);
  }
  else
  {
    DC_UnlockDc( hNewDC );
  }

  return hNewDC;
}

HDC STDCALL
NtGdiCreateDC(PUNICODE_STRING Driver,
              PUNICODE_STRING Device,
              PUNICODE_STRING Output,
              CONST PDEVMODEW InitData)
{
  UNICODE_STRING SafeDriver, SafeDevice;
  DEVMODEW SafeInitData;
  HDC Ret;
  NTSTATUS Status;

  if(InitData)
  {
    Status = MmCopyFromCaller(&SafeInitData, InitData, sizeof(DEVMODEW));
    if(!NT_SUCCESS(Status))
    {
      SetLastNtError(Status);
      return NULL;
    }
    /* FIXME - InitData can have some more bytes! */
  }

  if(Driver)
  {
    Status = IntSafeCopyUnicodeString(&SafeDriver, Driver);
    if(!NT_SUCCESS(Status))
    {
      SetLastNtError(Status);
      return NULL;
    }
  }

  if(Device)
  {
    Status = IntSafeCopyUnicodeString(&SafeDevice, Device);
    if(!NT_SUCCESS(Status))
    {
      RtlFreeUnicodeString(&SafeDriver);
      SetLastNtError(Status);
      return NULL;
    }
  }

  Ret = IntGdiCreateDC(&SafeDriver, &SafeDevice, NULL, &SafeInitData, FALSE);

  return Ret;
}

HDC STDCALL
NtGdiCreateIC(PUNICODE_STRING Driver,
              PUNICODE_STRING Device,
              PUNICODE_STRING Output,
              CONST PDEVMODEW InitData)
{
  UNICODE_STRING SafeDriver, SafeDevice;
  DEVMODEW SafeInitData;
  HDC Ret;
  NTSTATUS Status;

  if(InitData)
  {
    Status = MmCopyFromCaller(&SafeInitData, InitData, sizeof(DEVMODEW));
    if(!NT_SUCCESS(Status))
    {
      SetLastNtError(Status);
      return NULL;
    }
    /* FIXME - InitData can have some more bytes! */
  }

  if(Driver)
  {
    Status = IntSafeCopyUnicodeString(&SafeDriver, Driver);
    if(!NT_SUCCESS(Status))
    {
      SetLastNtError(Status);
      return NULL;
    }
  }

  if(Device)
  {
    Status = IntSafeCopyUnicodeString(&SafeDevice, Device);
    if(!NT_SUCCESS(Status))
    {
      RtlFreeUnicodeString(&SafeDriver);
      SetLastNtError(Status);
      return NULL;
    }
  }

  Ret = IntGdiCreateDC(NULL == Driver ? NULL : &SafeDriver,
                       NULL == Device ? NULL : &SafeDevice, NULL,
                       NULL == InitData ? NULL : &SafeInitData, TRUE);

  return Ret;
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
      NtGdiDeleteObject (DCToDelete->w.hFirstBitmap);
    }
    if (DCToDelete->XlateBrush != NULL)
      EngDeleteXlate(DCToDelete->XlateBrush);
    if (DCToDelete->XlatePen != NULL)
      EngDeleteXlate(DCToDelete->XlatePen);
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
  return 0;
}

INT STDCALL
NtGdiEnumObjects(HDC  hDC,
                INT  ObjectType,
                GOBJENUMPROC  ObjectFunc,
                LPARAM  lParam)
{
  UNIMPLEMENTED;
  return 0;
}

DC_GET_VAL( COLORREF, NtGdiGetBkColor, w.backgroundColor )
DC_GET_VAL( INT, NtGdiGetBkMode, w.backgroundMode )
DC_GET_VAL_EX( GetBrushOrgEx, w.brushOrgX, w.brushOrgY, POINT, x, y )
DC_GET_VAL( HRGN, NtGdiGetClipRgn, w.hClipRgn )

HGDIOBJ STDCALL
NtGdiGetCurrentObject(HDC  hDC, UINT  ObjectType)
{
  HGDIOBJ SelObject;
  DC *dc;

  if(!(dc = DC_LockDc(hDC)))
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return NULL;
  }

  switch(ObjectType)
  {
    case OBJ_PEN:
      SelObject = dc->w.hPen;
      break;
    case OBJ_BRUSH:
      SelObject = dc->w.hBrush;
      break;
    case OBJ_PAL:
      DPRINT1("FIXME: NtGdiGetCurrentObject() ObjectType OBJ_PAL not supported yet!\n");
      SelObject = NULL;
      break;
    case OBJ_FONT:
      SelObject = dc->w.hFont;
      break;
    case OBJ_BITMAP:
      SelObject = dc->w.hBitmap;
      break;
    case OBJ_COLORSPACE:
      DPRINT1("FIXME: NtGdiGetCurrentObject() ObjectType OBJ_COLORSPACE not supported yet!\n");
      SelObject = NULL;
      break;
    default:
      SelObject = NULL;
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      break;
  }

  DC_UnlockDc(hDC);
  return SelObject;
}

DC_GET_VAL_EX ( GetCurrentPositionEx, w.CursPosX, w.CursPosY, POINT, x, y )

BOOL FASTCALL
IntGdiGetDCOrgEx(DC *dc, LPPOINT  Point)
{
  Point->x = dc->w.DCOrgX;
  Point->y = dc->w.DCOrgY;

  return  TRUE;
}

BOOL STDCALL
NtGdiGetDCOrgEx(HDC  hDC, LPPOINT  Point)
{
  BOOL Ret;
  DC *dc;
  POINT SafePoint;
  NTSTATUS Status;

  if(!Point)
  {
    SetLastWin32Error(ERROR_INVALID_PARAMETER);
    return FALSE;
  }

  dc = DC_LockDc(hDC);
  if(!dc)
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return FALSE;
  }

  Ret = IntGdiGetDCOrgEx(dc, &SafePoint);

  Status = MmCopyToCaller(Point, &SafePoint, sizeof(POINT));
  if(!NT_SUCCESS(Status))
  {
    SetLastNtError(Status);
    DC_UnlockDc(hDC);
    return FALSE;
  }

  DC_UnlockDc(hDC);
  return Ret;
}

COLORREF STDCALL
NtGdiSetBkColor(HDC hDC, COLORREF color)
{
  COLORREF oldColor;
  PDC dc;
  HBRUSH hBrush;

  if (!(dc = DC_LockDc(hDC)))
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return CLR_INVALID;
  }

  oldColor = dc->w.backgroundColor;
  dc->w.backgroundColor = color;
  hBrush = dc->w.hBrush;
  DC_UnlockDc ( hDC );
  NtGdiSelectObject(hDC, hBrush);
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
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return 0;
  }

  hnewdc = DC_AllocDC(NULL);
  if (hnewdc == NULL)
  {
    DC_UnlockDc( hDC );
    return 0;
  }
  newdc = DC_LockDc( hnewdc );
  /* FIXME - newdc can be NULL!!!! Don't assert here!!! */
  ASSERT( newdc );

  newdc->w.flags            = dc->w.flags | DC_SAVED;
  newdc->w.hPen             = dc->w.hPen;
  newdc->w.hBrush           = dc->w.hBrush;
  newdc->w.hFont            = dc->w.hFont;
  newdc->w.hBitmap          = dc->w.hBitmap;
  newdc->w.hFirstBitmap     = dc->w.hFirstBitmap;
#if 0
  newdc->w.hDevice          = dc->w.hDevice;
#endif
  newdc->w.hPalette         = dc->w.hPalette;
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
  newdc->w.xformWorld2Wnd   = dc->w.xformWorld2Wnd;
  newdc->w.xformWorld2Vport = dc->w.xformWorld2Vport;
  newdc->w.xformVport2World = dc->w.xformVport2World;
  newdc->w.vport2WorldValid = dc->w.vport2WorldValid;
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
  newdc->IsIC = dc->IsIC;

#if 0
  PATH_InitGdiPath( &newdc->w.path );
#endif

  /* Get/SetDCState() don't change hVisRgn field ("Undoc. Windows" p.559). */

  newdc->w.hGCClipRgn = newdc->w.hVisRgn = 0;
  if (dc->w.hClipRgn)
  {
    newdc->w.hClipRgn = NtGdiCreateRectRgn( 0, 0, 0, 0 );
    NtGdiCombineRgn( newdc->w.hClipRgn, dc->w.hClipRgn, 0, RGN_COPY );
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

	dc->w.xformWorld2Wnd   = dcs->w.xformWorld2Wnd;
	dc->w.xformWorld2Vport = dcs->w.xformWorld2Vport;
	dc->w.xformVport2World = dcs->w.xformVport2World;
	dc->w.vport2WorldValid = dcs->w.vport2WorldValid;

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
	{
		int res;
		res = CLIPPING_UpdateGCRegion( dc );
		ASSERT ( res != ERROR );
	}
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

	NtGdiSelectPalette( hDC, dcs->w.hPalette, FALSE );

#if 0
	GDISelectPalette16( hDC, dcs->w.hPalette, FALSE );
#endif
      } else {
	DC_UnlockDc(hDC);
      }
      DC_UnlockDc ( hDCSave );
    } else {
      DC_UnlockDc ( hDC );
      SetLastWin32Error(ERROR_INVALID_HANDLE);
    }
  }
  else
    SetLastWin32Error(ERROR_INVALID_HANDLE);
}

INT FASTCALL
IntGdiGetDeviceCaps(PDC dc, INT Index)
{
  INT ret;
  POINT  pt;

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
      ret = 0;
      break;

    case COLORRES:
      UNIMPLEMENTED; /* FIXME */
      break;

    case PHYSICALWIDTH:
      if(IntGdiEscape(dc, GETPHYSPAGESIZE, 0, NULL, (LPVOID)&pt) > 0)
      {
        ret = pt.x;
      }
      else
      {
	ret = 0;
      }
      break;

    case PHYSICALHEIGHT:
      if(IntGdiEscape(dc, GETPHYSPAGESIZE, 0, NULL, (LPVOID)&pt) > 0)
      {
        ret = pt.y;
      }
      else
      {
	ret = 0;
      }
      break;

    case PHYSICALOFFSETX:
      if(IntGdiEscape(dc, GETPRINTINGOFFSET, 0, NULL, (LPVOID)&pt) > 0)
      {
        ret = pt.x;
      }
      else
      {
	ret = 0;
      }
      break;

    case PHYSICALOFFSETY:
      if(IntGdiEscape(dc, GETPRINTINGOFFSET, 0, NULL, (LPVOID)&pt) > 0)
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
      if(IntGdiEscape(dc, GETSCALINGFACTOR, 0, NULL, (LPVOID)&pt) > 0)
      {
        ret = pt.x;
      }
      else
      {
	ret = 0;
      }
      break;

    case SCALINGFACTORY:
      if(IntGdiEscape(dc, GETSCALINGFACTOR, 0, NULL, (LPVOID)&pt) > 0)
      {
        ret = pt.y;
      }
      else
      {
	ret = 0;
      }
      break;

    case RASTERCAPS:
      ret = dc->GDIInfo->flRaster;
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

  return ret;
}

INT STDCALL
NtGdiGetDeviceCaps(HDC  hDC,
                  INT  Index)
{
  PDC  dc;
  INT  ret;

  dc = DC_LockDc(hDC);
  if (dc == NULL)
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return 0;
  }

  ret = IntGdiGetDeviceCaps(dc, Index);

  DPRINT("(%04x,%d): returning %d\n", hDC, Index, ret);

  DC_UnlockDc( hDC );
  return ret;
}

DC_GET_VAL( INT, NtGdiGetMapMode, w.MapMode )
DC_GET_VAL( INT, NtGdiGetPolyFillMode, w.polyFillMode )

INT FASTCALL
IntGdiGetObject(HANDLE Handle, INT Count, LPVOID Buffer)
{
  PVOID GdiObject;
  INT Result = 0;
  DWORD ObjectType;

  GdiObject = GDIOBJ_LockObj(Handle, GDI_OBJECT_TYPE_DONTCARE);
  if (NULL == GdiObject)
    {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return 0;
    }

  ObjectType = GDIOBJ_GetObjectType(Handle);
  switch (ObjectType)
    {
#if 0
      case GDI_OBJECT_TYPE_PEN:
        Result = PEN_GetObject((PENOBJ *) GdiObject, Count, Buffer);
        break;
      case GDI_OBJECT_TYPE_BRUSH:
        Result = BRUSH_GetObject((BRUSHOBJ *) GdiObject, Count, Buffer);
        break;
#endif
      case GDI_OBJECT_TYPE_BITMAP:
        Result = BITMAP_GetObject((BITMAPOBJ *) GdiObject, Count, Buffer);
        break;
      case GDI_OBJECT_TYPE_FONT:
        Result = FontGetObject((PTEXTOBJ) GdiObject, Count, Buffer);
#if 0
        // Fix the LOGFONT structure for the stock fonts
        if (FIRST_STOCK_HANDLE <= Handle && Handle <= LAST_STOCK_HANDLE)
          {
            FixStockFontSizeW(Handle, Count, Buffer);
          }
#endif
        break;
#if 0
      case GDI_OBJECT_TYPE_PALETTE:
        Result = PALETTE_GetObject((PALETTEOBJ *) GdiObject, Count, Buffer);
        break;
#endif
      default:
        DPRINT1("GDI object type 0x%08x not implemented\n", ObjectType);
        break;
    }

  GDIOBJ_UnlockObj(Handle);

  return Result;
}

INT STDCALL
NtGdiGetObject(HANDLE handle, INT count, LPVOID buffer)
{
  INT Ret;
  LPVOID SafeBuf;
  NTSTATUS Status;

  if (count <= 0)
  {
    return 0;
  }

  SafeBuf = ExAllocatePoolWithTag(PagedPool, count, TAG_GDIOBJ);
  if(!SafeBuf)
  {
    SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
    return 0;
  }

  Ret = IntGdiGetObject(handle, count, SafeBuf);

  Status = MmCopyToCaller(buffer, SafeBuf, count);
  ExFreePool(SafeBuf);
  if(!NT_SUCCESS(Status))
  {
    SetLastNtError(Status);
    return 0;
  }

  return Ret;
}

DWORD STDCALL
NtGdiGetObjectType(HANDLE handle)
{
  GDIOBJHDR * ptr;
  INT result;
  DWORD objectType;

  ptr = GDIOBJ_LockObj(handle, GDI_OBJECT_TYPE_DONTCARE);
  if (ptr == 0)
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return 0;
  }

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
      result = 0;
      break;
  }
  GDIOBJ_UnlockObj(handle);
  return result;
}

DC_GET_VAL( INT, NtGdiGetRelAbs, w.relAbsMode )
DC_GET_VAL( INT, NtGdiGetROP2, w.ROPmode )
DC_GET_VAL( INT, NtGdiGetStretchBltMode, w.stretchBltMode )
DC_GET_VAL( UINT, NtGdiGetTextAlign, w.textAlign )
DC_GET_VAL( COLORREF, NtGdiGetTextColor, w.textColor )
DC_GET_VAL_EX( GetViewportExtEx, vportExtX, vportExtY, SIZE, cx, cy )
DC_GET_VAL_EX( GetViewportOrgEx, vportOrgX, vportOrgY, POINT, x, y )
DC_GET_VAL_EX( GetWindowExtEx, wndExtX, wndExtY, SIZE, cx, cy )
DC_GET_VAL_EX( GetWindowOrgEx, wndOrgX, wndOrgY, POINT, x, y )

HDC STDCALL
NtGdiResetDC(HDC  hDC, CONST DEVMODEW *InitData)
{
  UNIMPLEMENTED;
  return 0;
}

BOOL STDCALL
NtGdiRestoreDC(HDC  hDC, INT  SaveLevel)
{
  PDC dc, dcs;
  BOOL success;

  DPRINT("NtGdiRestoreDC(%lx, %d)\n", hDC, SaveLevel);

  dc = DC_LockDc(hDC);
  if (!dc)
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return FALSE;
  }

  if (SaveLevel == -1)
    SaveLevel = dc->saveLevel;

  if ((SaveLevel < 1) || (SaveLevel > dc->saveLevel))
  {
    DC_UnlockDc(hDC);
    return FALSE;
  }

  success = TRUE;
  while (dc->saveLevel >= SaveLevel)
  {
    HDC hdcs = DC_GetNextDC (dc);

    dcs = DC_LockDc (hdcs);
    if (dcs == NULL)
    {
      DC_UnlockDc(hDC);
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

  DPRINT("NtGdiSaveDC(%lx)\n", hDC);

  if (!(hdcs = NtGdiGetDCState(hDC)))
  {
    return 0;
  }

  dcs = DC_LockDc (hdcs);
  if (dcs == NULL)
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return 0;
  }
  dc = DC_LockDc (hDC);
  if (dc == NULL)
  {
    DC_UnlockDc(hdcs);
    SetLastWin32Error(ERROR_INVALID_HANDLE);
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
  PGDIBRUSHOBJ pen;
  PGDIBRUSHOBJ brush;
  XLATEOBJ *XlateObj;
  DWORD objectType;
  HRGN hVisRgn;
  BOOLEAN Failed;

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
      pen = PENOBJ_LockPen((HPEN) hGDIObj);
      if (pen == NULL)
      {
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        break;
      }

      XlateObj = IntGdiCreateBrushXlate(dc, pen, &Failed);
      PENOBJ_UnlockPen(hGDIObj);
      if (Failed)
      {
        SetLastWin32Error(ERROR_NO_SYSTEM_RESOURCES);
        break;
      }

      objOrg = (HGDIOBJ)dc->w.hPen;
      dc->w.hPen = hGDIObj;
      if (dc->XlatePen != NULL)
        EngDeleteXlate(dc->XlatePen);
      dc->XlatePen = XlateObj;
      break;

    case GDI_OBJECT_TYPE_BRUSH:
      brush = BRUSHOBJ_LockBrush((HPEN) hGDIObj);
      if (brush == NULL)
      {
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        break;
      }

      XlateObj = IntGdiCreateBrushXlate(dc, brush, &Failed);
      BRUSHOBJ_UnlockBrush(hGDIObj);
      if (Failed)
      {
        SetLastWin32Error(ERROR_NO_SYSTEM_RESOURCES);
        break;
      }

      objOrg = (HGDIOBJ)dc->w.hBrush;
      dc->w.hBrush = hGDIObj;
      if (dc->XlateBrush != NULL)
        EngDeleteXlate(dc->XlateBrush);
      dc->XlateBrush = XlateObj;
      break;

    case GDI_OBJECT_TYPE_FONT:
      if(NT_SUCCESS(TextIntRealizeFont((HFONT)hGDIObj)))
      {
        objOrg = (HGDIOBJ)dc->w.hFont;
        dc->w.hFont = (HFONT) hGDIObj;
      }
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
      dc->w.hBitmap = hGDIObj;

      // if we're working with a DIB, get the palette [fixme: only create if the selected palette is null]
      if(pb->dib)
      {
        dc->w.bitsPerPixel = pb->dib->dsBmih.biBitCount;
        dc->w.hPalette = pb->hDIBPalette;
      }
      else
      {
        dc->w.bitsPerPixel = BitsPerFormat(pb->SurfObj.iBitmapFormat);
        dc->w.hPalette = dc->DevInfo->hpalDefault;
      }

      /* Reselect brush and pen to regenerate the XLATEOBJs. */
      NtGdiSelectObject ( hDC, dc->w.hBrush );
      NtGdiSelectObject ( hDC, dc->w.hPen );

      DC_UnlockDc ( hDC );
      hVisRgn = NtGdiCreateRectRgn ( 0, 0, pb->SurfObj.sizlBitmap.cx, pb->SurfObj.sizlBitmap.cy );
      BITMAPOBJ_UnlockBitmap( hGDIObj );
      NtGdiSelectVisRgn ( hDC, hVisRgn );
      NtGdiDeleteObject ( hVisRgn );

      return objOrg;

    case GDI_OBJECT_TYPE_REGION:
      DC_UnlockDc (hDC);
      /*
       * The return value is one of the following values:
       *  SIMPLEREGION
       *  COMPLEXREGION
       *  NULLREGION
       */
      return (HGDIOBJ) NtGdiSelectClipRgn(hDC, (HRGN) hGDIObj);

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
      SetLastWin32Error(ERROR_INVALID_HANDLE);
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
DC_AllocDC(PUNICODE_STRING Driver)
{
  PDC  NewDC;
  HDC  hDC;
  PWSTR Buf = NULL;

  if (Driver != NULL)
  {
    Buf = ExAllocatePoolWithTag(PagedPool, Driver->MaximumLength, TAG_DC);
    if(!Buf)
    {
      return NULL;
    }
    RtlCopyMemory(Buf, Driver->Buffer, Driver->MaximumLength);
  }

  hDC = (HDC) GDIOBJ_AllocObj(GDI_OBJECT_TYPE_DC);
  if (hDC == NULL)
  {
    if(Buf)
    {
      ExFreePool(Buf);
    }
    return  NULL;
  }

  NewDC = DC_LockDc(hDC);
  /* FIXME - Handle NewDC == NULL! */

  if (Driver != NULL)
  {
    RtlCopyMemory(&NewDC->DriverName, Driver, sizeof(UNICODE_STRING));
    NewDC->DriverName.Buffer = Buf;
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
  NewDC->w.MapMode = MM_TEXT;
  NewDC->wndExtX = 1.0f;
  NewDC->wndExtY = 1.0f;
  NewDC->vportExtX = 1.0f;
  NewDC->vportExtY = 1.0f;
  NewDC->w.textColor = 0;
  NewDC->w.backgroundColor = 0xffffff;

  NewDC->w.hFont = NtGdiGetStockObject(SYSTEM_FONT);
  TextIntRealizeFont(NewDC->w.hFont);

  NewDC->w.hPalette = NtGdiGetStockObject(DEFAULT_PALETTE);

  DC_UnlockDc(hDC);

  return  hDC;
}

HDC FASTCALL
DC_FindOpenDC(PUNICODE_STRING  Driver)
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

/*
  {
    int res;
    res = CLIPPING_UpdateGCRegion(DCToInit);
    ASSERT ( res != ERROR );
  }
*/
}

VOID FASTCALL
DC_FreeDC(HDC  DCToFree)
{
  if (!GDIOBJ_FreeObj(DCToFree, GDI_OBJECT_TYPE_DC))
  {
    DPRINT("DC_FreeDC failed\n");
  }
}

BOOL INTERNAL_CALL
DC_Cleanup(PVOID ObjectBody)
{
  PDC pDC = (PDC)ObjectBody;
  RtlFreeUnicodeString(&pDC->DriverName);
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
  scaleX = (dc->wndExtX ? (FLOAT)dc->vportExtX / (FLOAT)dc->wndExtX : 0.0f);
  scaleY = (dc->wndExtY ? (FLOAT)dc->vportExtY / (FLOAT)dc->wndExtY : 0.0f);
  xformWnd2Vport.eM11 = scaleX;
  xformWnd2Vport.eM12 = 0.0;
  xformWnd2Vport.eM21 = 0.0;
  xformWnd2Vport.eM22 = scaleY;
  xformWnd2Vport.eDx  = (FLOAT)dc->vportOrgX - scaleX * (FLOAT)dc->wndOrgX;
  xformWnd2Vport.eDy  = (FLOAT)dc->vportOrgY - scaleY * (FLOAT)dc->wndOrgY;

  /* Combine with the world transformation */
  IntGdiCombineTransform(&dc->w.xformWorld2Vport, &dc->w.xformWorld2Wnd, &xformWnd2Vport);

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

VOID FASTCALL
DC_SetOwnership(HDC hDC, PEPROCESS Owner)
{
  PDC DC;

  GDIOBJ_SetOwnership(hDC, Owner);
  DC = DC_LockDc(hDC);
  if (NULL != DC)
    {
      if (NULL != DC->w.hClipRgn)
        {
          GDIOBJ_CopyOwnership(hDC, DC->w.hClipRgn);
        }
      if (NULL != DC->w.hVisRgn)
        {
          GDIOBJ_CopyOwnership(hDC, DC->w.hVisRgn);
        }
      if (NULL != DC->w.hGCClipRgn)
        {
          GDIOBJ_CopyOwnership(hDC, DC->w.hGCClipRgn);
        }
      DC_UnlockDc(hDC);
    }
}

BOOL FASTCALL
IntIsPrimarySurface(SURFOBJ *SurfObj)
{
   if (PrimarySurface.Handle == NULL)
     {
       return FALSE;
     }
   return SurfObj->hsurf == PrimarySurface.Handle;
}

/*
 * Returns the color of the brush or pen that is currently selected into the DC.
 * This function is called from GetDCBrushColor() and GetDCPenColor()
 */
COLORREF FASTCALL
IntGetDCColor(HDC hDC, ULONG Object)
{
   /*
    * The previous implementation was completly incorrect. It modified the
    * brush that was currently selected into the device context, but in fact
    * the DC pen/brush color should be stored directly in the device context
    * (at address 0x2C of the user mode DC object memory on Windows 2K/XP).
    * The actual color is then used when DC_BRUSH/DC_PEN object is selected
    * into the device context and BRUSHOBJ for drawing is composed (belongs
    * to IntGdiInitBrushInstance in the current ReactOS implementation). Also
    * the implementation should be moved to user mode GDI32.dll when UM
    * mapped GDI objects will be implemented.
    */

   DPRINT("WIN32K:IntGetDCColor is unimplemented\n");
   return 0xFFFFFF; /* The default DC color. */
}

/*
 * Changes the color of the brush or pen that is currently selected into the DC.
 * This function is called from SetDCBrushColor() and SetDCPenColor()
 */
COLORREF FASTCALL
IntSetDCColor(HDC hDC, ULONG Object, COLORREF Color)
{
   /* See the comment in IntGetDCColor. */

   DPRINT("WIN32K:IntSetDCColor is unimplemented\n");
   return CLR_INVALID;
}

#define SIZEOF_DEVMODEW_300 188
#define SIZEOF_DEVMODEW_400 212
#define SIZEOF_DEVMODEW_500 220

/*! \brief Enumerate possible display settings for the given display...
 *
 * \todo Make thread safe!?
 * \todo Don't ignore pDeviceName
 * \todo Implement non-raw mode (only return settings valid for driver and monitor)
 */
BOOL FASTCALL
IntEnumDisplaySettings(
  IN PUNICODE_STRING pDeviceName  OPTIONAL,
  IN DWORD iModeNum,
  IN OUT LPDEVMODEW pDevMode,
  IN DWORD dwFlags)
{
  static DEVMODEW *CachedDevModes = NULL, *CachedDevModesEnd = NULL;
  static DWORD SizeOfCachedDevModes = 0;
  PDEVMODEW CachedMode = NULL;
  DEVMODEW DevMode;
  INT Size, OldSize;
  ULONG DisplayNumber = 0; /* only default display supported */

  DPRINT1("DevMode->dmSize = %d\n", pDevMode->dmSize);
  DPRINT1("DevMode->dmExtraSize = %d\n", pDevMode->dmDriverExtra);
  if (pDevMode->dmSize != SIZEOF_DEVMODEW_300 &&
      pDevMode->dmSize != SIZEOF_DEVMODEW_400 &&
      pDevMode->dmSize != SIZEOF_DEVMODEW_500)
  {
    SetLastWin32Error(STATUS_INVALID_PARAMETER);
    return FALSE;
  }

  if (iModeNum == ENUM_CURRENT_SETTINGS)
  {
    CachedMode = &PrimarySurface.DMW;
    ASSERT(CachedMode->dmSize > 0);
  }
  else if (iModeNum == ENUM_REGISTRY_SETTINGS)
  {
    RtlZeroMemory(&DevMode, sizeof (DevMode));
    DevMode.dmSize = sizeof (DevMode);
    DevMode.dmDriverExtra = 0;
    if (SetupDevMode(&DevMode, DisplayNumber))
      CachedMode = &DevMode;
    else
    {
      SetLastWin32Error(0); /* FIXME: use error code */
      return FALSE;
    }
    /* FIXME: Maybe look for the matching devmode supplied by the
     *        driver so we can provide driver private/extra data?
     */
  }
  else
  {
    if (iModeNum == 0 || CachedDevModes == NULL) /* query modes from drivers */
    {
      BOOL PrimarySurfaceCreated = FALSE;
      UNICODE_STRING DriverFileNames;
      LPWSTR CurrentName;
      DRVENABLEDATA DrvEnableData;

      /* Retrieve DDI driver names from registry */
      RtlInitUnicodeString(&DriverFileNames, NULL);
      if (!FindDriverFileNames(&DriverFileNames, DisplayNumber))
      {
        DPRINT1("FindDriverFileNames failed\n");
        return FALSE;
      }

      if (!HalQueryDisplayOwnership())
      {
        IntCreatePrimarySurface();
        PrimarySurfaceCreated = TRUE;
      }

      /*
       * DriverFileNames may be a list of drivers in REG_SZ_MULTI format,
       * scan all of them until a good one found.
       */
      CurrentName = DriverFileNames.Buffer;
      for (;CurrentName < DriverFileNames.Buffer + (DriverFileNames.Length / sizeof (WCHAR));
           CurrentName += wcslen(CurrentName) + 1)
      {
        INT i;
        PGD_ENABLEDRIVER GDEnableDriver;

        /* Get the DDI driver's entry point */
        GDEnableDriver = DRIVER_FindDDIDriver(CurrentName);
        if (NULL == GDEnableDriver)
        {
          DPRINT("FindDDIDriver failed for %S\n", CurrentName);
          continue;
        }

        /*  Call DDI driver's EnableDriver function  */
        RtlZeroMemory(&DrvEnableData, sizeof (DrvEnableData));

        if (!GDEnableDriver(DDI_DRIVER_VERSION_NT5_01, sizeof (DrvEnableData), &DrvEnableData))
        {
          DPRINT("DrvEnableDriver failed for %S\n", CurrentName);
          continue;
        }

        CachedDevModesEnd = CachedDevModes;

        /* find DrvGetModes function */
        for (i = 0; i < DrvEnableData.c; i++)
        {
          PDRVFN DrvFn = DrvEnableData.pdrvfn + i;
          PGD_GETMODES GetModes;
          INT SizeNeeded, SizeUsed;

          if (DrvFn->iFunc != INDEX_DrvGetModes)
            continue;

          GetModes = (PGD_GETMODES)DrvFn->pfn;

          /* make sure we have enough memory to hold the modes */
          SizeNeeded = GetModes((HANDLE)(PrimarySurface.VideoFileObject->DeviceObject), 0, NULL);
          if (SizeNeeded <= 0)
          {
            DPRINT("DrvGetModes failed for %S\n", CurrentName);
            break;
          }

          SizeUsed = CachedDevModesEnd - CachedDevModes;
          if (SizeOfCachedDevModes - SizeUsed < SizeNeeded)
          {
            PVOID NewBuffer;

            SizeOfCachedDevModes += SizeNeeded;
            NewBuffer = ExAllocatePool(PagedPool, SizeOfCachedDevModes);
            if (NewBuffer == NULL)
            {
              /* clean up */
              ExFreePool(CachedDevModes);
              SizeOfCachedDevModes = 0;
              CachedDevModes = NULL;
              CachedDevModesEnd = NULL;
              if (PrimarySurfaceCreated)
              {
                IntDestroyPrimarySurface();
              }
              SetLastWin32Error(STATUS_NO_MEMORY);
              return FALSE;
            }
            if (CachedDevModes != NULL)
            {
              RtlCopyMemory(NewBuffer, CachedDevModes, SizeUsed);
              ExFreePool(CachedDevModes);
            }
            CachedDevModes = NewBuffer;
            CachedDevModesEnd = (DEVMODEW *)((PCHAR)NewBuffer + SizeUsed);
          }

          /* query modes */
          SizeNeeded = GetModes((HANDLE)(PrimarySurface.VideoFileObject->DeviceObject),
                                SizeOfCachedDevModes - SizeUsed,
                                CachedDevModesEnd);
          if (SizeNeeded <= 0)
          {
            DPRINT("DrvGetModes failed for %S\n", CurrentName);
          }
          else
          {
            CachedDevModesEnd = (DEVMODEW *)((PCHAR)CachedDevModesEnd + SizeNeeded);
          }
          break;
        }
      }

      if (PrimarySurfaceCreated)
      {
        IntDestroyPrimarySurface();
      }

      RtlFreeUnicodeString(&DriverFileNames);
    }

    /* return cached info */
    CachedMode = CachedDevModes;
    if (CachedMode >= CachedDevModesEnd)
    {
      SetLastWin32Error(STATUS_NO_MORE_ENTRIES);
      return FALSE;
    }
    while (iModeNum-- > 0 && CachedMode < CachedDevModesEnd)
    {
      assert(CachedMode->dmSize > 0);
      CachedMode = (DEVMODEW *)((PCHAR)CachedMode + CachedMode->dmSize + CachedMode->dmDriverExtra);
    }
    if (CachedMode >= CachedDevModesEnd)
    {
      SetLastWin32Error(STATUS_NO_MORE_ENTRIES);
      return FALSE;
    }
  }

  ASSERT(CachedMode != NULL);

  Size = OldSize = pDevMode->dmSize;
  if (Size > CachedMode->dmSize)
    Size = CachedMode->dmSize;
  RtlCopyMemory(pDevMode, CachedMode, Size);
  RtlZeroMemory((PCHAR)pDevMode + Size, OldSize - Size);
  pDevMode->dmSize = OldSize;

  Size = OldSize = pDevMode->dmDriverExtra;
  if (Size > CachedMode->dmDriverExtra)
    Size = CachedMode->dmDriverExtra;
  RtlCopyMemory((PCHAR)pDevMode + pDevMode->dmSize,
                (PCHAR)CachedMode + CachedMode->dmSize, Size);
  RtlZeroMemory((PCHAR)pDevMode + pDevMode->dmSize + Size, OldSize - Size);
  pDevMode->dmDriverExtra = OldSize;

  return TRUE;
}
LONG
FASTCALL
IntChangeDisplaySettings(
  IN PUNICODE_STRING pDeviceName  OPTIONAL,
  IN LPDEVMODEW DevMode,
  IN DWORD dwflags,
  IN PVOID lParam  OPTIONAL)
{
  BOOLEAN Global = FALSE;
  BOOLEAN NoReset = FALSE;
  BOOLEAN Reset = FALSE;
  BOOLEAN SetPrimary = FALSE;
  LONG Ret=0;
  NTSTATUS Status ;

  DPRINT1("display flag : %x\n",dwflags);

  if ((dwflags & CDS_UPDATEREGISTRY) == CDS_UPDATEREGISTRY)
  {
    /* Check global, reset and noreset flags */
    if ((dwflags & CDS_GLOBAL) == CDS_GLOBAL)
      Global = TRUE;
    if ((dwflags & CDS_NORESET) == CDS_NORESET)
      NoReset = TRUE;
    dwflags &= ~(CDS_GLOBAL | CDS_NORESET);
  }
  if ((dwflags & CDS_RESET) == CDS_RESET)
    Reset = TRUE;
  if ((dwflags & CDS_SET_PRIMARY) == CDS_SET_PRIMARY)
    SetPrimary = TRUE;
  dwflags &= ~(CDS_RESET | CDS_SET_PRIMARY);

  if (Reset && NoReset)
    return DISP_CHANGE_BADFLAGS;

  if (dwflags == 0)
  {
   /* Dynamically change graphics mode */
   DPRINT1("flag 0 UNIMPLEMENT \n");
   return DISP_CHANGE_FAILED;
  }

  if ((dwflags & CDS_TEST) == CDS_TEST)
  {
   /* Test reslution */
   dwflags &= ~CDS_TEST;
   DPRINT1("flag CDS_TEST UNIMPLEMENT");
   Ret = DISP_CHANGE_FAILED;
  }
  
  if ((dwflags & CDS_FULLSCREEN) == CDS_FULLSCREEN)
  {
   DEVMODE lpDevMode;
   /* Full Screen */
   dwflags &= ~CDS_FULLSCREEN;
   DPRINT1("flag CDS_FULLSCREEN partially implemented");
   Ret = DISP_CHANGE_FAILED;

   lpDevMode.dmBitsPerPel =0;
   lpDevMode.dmPelsWidth  =0;
   lpDevMode.dmPelsHeight =0;
   lpDevMode.dmDriverExtra =0;

   lpDevMode.dmSize = sizeof(DEVMODE);
   Status = IntEnumDisplaySettings(pDeviceName,  ENUM_CURRENT_SETTINGS, &lpDevMode, 0);
   if (!NT_SUCCESS(Status)) return DISP_CHANGE_FAILED;

   DPRINT1("Req Mode     : %d x %d x %d\n", DevMode->dmPelsWidth,DevMode->dmPelsHeight,DevMode->dmBitsPerPel);
   DPRINT1("Current Mode : %d x %d x %d\n", lpDevMode.dmPelsWidth,lpDevMode.dmPelsHeight, lpDevMode.dmBitsPerPel);


   if ((lpDevMode.dmBitsPerPel == DevMode->dmBitsPerPel) &&
       (lpDevMode.dmPelsWidth  == DevMode->dmPelsWidth) &&
       (lpDevMode.dmPelsHeight == DevMode->dmPelsHeight))
	   Ret = DISP_CHANGE_SUCCESSFUL; 
  }

  if ((dwflags & CDS_VIDEOPARAMETERS) == CDS_VIDEOPARAMETERS)
  {  
    dwflags &= ~CDS_VIDEOPARAMETERS;
    if (lParam == NULL) Ret=DISP_CHANGE_BADPARAM;
	else
	{
		DPRINT1("flag CDS_VIDEOPARAMETERS UNIMPLEMENT");
		Ret = DISP_CHANGE_FAILED;
	}

  }    

  if ((dwflags & CDS_UPDATEREGISTRY) == CDS_UPDATEREGISTRY)
  {  
  
	  	UNICODE_STRING ObjectName;
		UNICODE_STRING KernelModeName;
	  	WCHAR KernelModeNameBuffer[256];
		UNICODE_STRING RegistryKey;
		WCHAR RegistryKeyBuffer[512];
		PDEVICE_OBJECT DeviceObject;		
		ULONG LastSlash;
		OBJECT_ATTRIBUTES ObjectAttributes;
		HANDLE DevInstRegKey;
		ULONG NewValue;
		

		DPRINT1("set CDS_UPDATEREGISTRY \n");
		
		dwflags &= ~CDS_UPDATEREGISTRY;	

		/* Get device name (pDeviceName is "\.\xxx") */
		for (LastSlash = pDeviceName->Length / sizeof(WCHAR); LastSlash > 0; LastSlash--)
		{
			if (pDeviceName->Buffer[LastSlash - 1] == L'\\')
				break;
		}
		
		if (LastSlash == 0) return DISP_CHANGE_RESTART;
		ObjectName = *pDeviceName;
		ObjectName.Length -= LastSlash * sizeof(WCHAR);
		ObjectName.MaximumLength -= LastSlash * sizeof(WCHAR);
		ObjectName.Buffer += LastSlash;

		KernelModeName.Length = 0;
		KernelModeName.MaximumLength = sizeof(KernelModeNameBuffer);
		KernelModeName.Buffer = KernelModeNameBuffer;

		/* Open \??\xxx (ex: "\??\DISPLAY1") */
		Status = RtlAppendUnicodeToString(&KernelModeName, L"\\??\\");
		
		if (!NT_SUCCESS(Status)) return DISP_CHANGE_FAILED;
		Status = RtlAppendUnicodeStringToString(&KernelModeName, &ObjectName);
		
		if (!NT_SUCCESS(Status)) return DISP_CHANGE_FAILED;
		Status = ObReferenceObjectByName(
			&KernelModeName,
			OBJ_CASE_INSENSITIVE,
			NULL,
			0,
			IoDeviceObjectType,
			KernelMode,
			NULL,
			(PVOID*)&DeviceObject);

		if (!NT_SUCCESS(Status)) return DISP_CHANGE_FAILED;
		/* Get associated driver name (ex: "VBE") */
		for (LastSlash = DeviceObject->DriverObject->DriverName.Length / sizeof(WCHAR); LastSlash > 0; LastSlash--)
		{
			if (DeviceObject->DriverObject->DriverName.Buffer[LastSlash - 1] == L'\\')
				break;
		}

		if (LastSlash == 0) { ObDereferenceObject(DeviceObject); return DISP_CHANGE_FAILED; }
		ObjectName = DeviceObject->DriverObject->DriverName;
		ObjectName.Length -= LastSlash * sizeof(WCHAR);
		ObjectName.MaximumLength -= LastSlash * sizeof(WCHAR);
		ObjectName.Buffer += LastSlash;

		RegistryKey.Length = 0;
		RegistryKey.MaximumLength = sizeof(RegistryKeyBuffer);
		RegistryKey.Buffer = RegistryKeyBuffer;

		/* Open registry key */
		Status = RtlAppendUnicodeToString(&RegistryKey,
			L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Hardware Profiles\\Current\\System\\CurrentControlSet\\Services\\");
		
		if (!NT_SUCCESS(Status)) { ObDereferenceObject(DeviceObject); return DISP_CHANGE_FAILED; }
		Status = RtlAppendUnicodeStringToString(&RegistryKey, &ObjectName);

		if (!NT_SUCCESS(Status)) { ObDereferenceObject(DeviceObject); return DISP_CHANGE_FAILED; }
		Status = RtlAppendUnicodeToString(&RegistryKey,
			L"\\Device0");

		if (!NT_SUCCESS(Status)) { ObDereferenceObject(DeviceObject); return DISP_CHANGE_FAILED; }

		InitializeObjectAttributes(&ObjectAttributes, &RegistryKey,
			OBJ_CASE_INSENSITIVE, NULL, NULL);
		Status = ZwOpenKey(&DevInstRegKey, GENERIC_READ | GENERIC_WRITE, &ObjectAttributes);
		ObDereferenceObject(DeviceObject);
		if (!NT_SUCCESS(Status)) return DISP_CHANGE_FAILED;

		/* Update needed fields */
		if (NT_SUCCESS(Status) && DevMode->dmFields & DM_BITSPERPEL)
		{
			RtlInitUnicodeString(&RegistryKey, L"DefaultSettings.BitsPerPel");
			NewValue = DevMode->dmBitsPerPel;
			Status = ZwSetValueKey(DevInstRegKey, &RegistryKey, 0, REG_DWORD, &NewValue, sizeof(NewValue));			
		}

		if (NT_SUCCESS(Status) && DevMode->dmFields & DM_PELSWIDTH)
		{
			RtlInitUnicodeString(&RegistryKey, L"DefaultSettings.XResolution");
			NewValue = DevMode->dmPelsWidth;			
			Status = ZwSetValueKey(DevInstRegKey, &RegistryKey, 0, REG_DWORD, &NewValue, sizeof(NewValue));			
		}

		if (NT_SUCCESS(Status) && DevMode->dmFields & DM_PELSHEIGHT)
		{
			RtlInitUnicodeString(&RegistryKey, L"DefaultSettings.YResolution");
			NewValue = DevMode->dmPelsHeight;
			Status = ZwSetValueKey(DevInstRegKey, &RegistryKey, 0, REG_DWORD, &NewValue, sizeof(NewValue));			
		}

		ZwClose(DevInstRegKey);
		if (NT_SUCCESS(Status))
			Ret = DISP_CHANGE_RESTART;
		else
			/* return DISP_CHANGE_NOTUPDATED when we can save to reg only vaild for NT */ 
			Ret = DISP_CHANGE_NOTUPDATED;
		
    }
 
 if (dwflags != 0)  
    Ret = DISP_CHANGE_BADFLAGS;

  return Ret;
}

/* EOF */
