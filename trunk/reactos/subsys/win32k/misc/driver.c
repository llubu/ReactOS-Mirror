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
/* $Id: driver.c,v 1.32 2003/11/17 02:12:52 hyperion Exp $
 * 
 * GDI Driver support routines
 * (mostly swiped from Wine)
 * 
 */

#undef WIN32_LEAN_AND_MEAN
#define WIN32_NO_PEHDR

#include <ddk/ntddk.h>
#include <windows.h>
#include <win32k/driver.h>
#include <win32k/misc.h>
#include <wchar.h>
#include <ddk/winddi.h>
#include <ddk/ntddvid.h>
#include <rosrtl/string.h>

#define NDEBUG
#include <debug.h>

#define DRIVER_TAG TAG('G', 'D', 'R', 'V')

typedef struct _GRAPHICS_DRIVER
{
  PWSTR  Name;
  PGD_ENABLEDRIVER  EnableDriver;
  int  ReferenceCount;
  struct _GRAPHICS_DRIVER  *Next;
} GRAPHICS_DRIVER, *PGRAPHICS_DRIVER;

static PGRAPHICS_DRIVER  DriverList;
static PGRAPHICS_DRIVER  GenericDriver = 0;

BOOL DRIVER_RegisterDriver(LPCWSTR  Name, PGD_ENABLEDRIVER  EnableDriver)
{
  PGRAPHICS_DRIVER  Driver = ExAllocatePoolWithTag(NonPagedPool, sizeof(*Driver), DRIVER_TAG);
  DPRINT( "DRIVER_RegisterDriver( Name: %S )\n", Name );
  if (!Driver)  return  FALSE;
  Driver->ReferenceCount = 0;
  Driver->EnableDriver = EnableDriver;
  if (Name)
  {
    Driver->Name = ExAllocatePoolWithTag(PagedPool,
                                         (wcslen(Name) + 1) * sizeof(WCHAR),
                                         DRIVER_TAG);
    wcscpy(Driver->Name, Name);
    Driver->Next  = DriverList;
    DriverList = Driver;
    return  TRUE;
  }
  
  if (GenericDriver != NULL)
  {
    ExFreePool(Driver);
    return  FALSE;
  }
  
  GenericDriver = Driver;
  return  TRUE;
}

PGD_ENABLEDRIVER DRIVER_FindDDIDriver(LPCWSTR Name)
{
  static WCHAR DefaultPath[] = L"\\SystemRoot\\System32\\";
  static WCHAR DefaultExtension[] = L".DLL";
  SYSTEM_LOAD_IMAGE GdiDriverInfo;
  GRAPHICS_DRIVER *Driver = DriverList;
  NTSTATUS Status;
  WCHAR *FullName;
  LPCWSTR p;
  BOOL PathSeparatorFound;
  BOOL DotFound;
  UINT Size;

  DotFound = FALSE;  
  PathSeparatorFound = FALSE;
  p = Name;
  while (L'\0' != *p)
  {
    if (L'\\' == *p || L'/' == *p)
    {
      PathSeparatorFound = TRUE;
      DotFound = FALSE;
    }
    else if (L'.' == *p)
    {
      DotFound = TRUE;
    }
    p++;
  }

  Size = (wcslen(Name) + 1) * sizeof(WCHAR);
  if (! PathSeparatorFound)
  {
    Size += sizeof(DefaultPath) - sizeof(WCHAR);
  }
  if (! DotFound)
  {
    Size += sizeof(DefaultExtension) - sizeof(WCHAR);
  }
  FullName = ExAllocatePoolWithTag(PagedPool, Size, DRIVER_TAG);
  if (NULL == FullName)
  {
    DPRINT1("Out of memory\n");
    return NULL;
  }
  if (PathSeparatorFound)
  {
    FullName[0] = L'\0';
  }
  else
  {
    wcscpy(FullName, DefaultPath);
  }
  wcscat(FullName, Name);
  if (! DotFound)
  {
    wcscat(FullName, DefaultExtension);
  }

  /* First see if the driver hasn't already been loaded */
  while (Driver && FullName)
  {
    if (!_wcsicmp( Driver->Name, FullName)) 
    {
      return Driver->EnableDriver;
    }
    Driver = Driver->Next;
  }

  /* If not, then load it */
  RtlInitUnicodeString (&GdiDriverInfo.ModuleName, (LPWSTR)FullName);
  Status = ZwSetSystemInformation (SystemLoadImage, &GdiDriverInfo, sizeof(SYSTEM_LOAD_IMAGE));
  ExFreePool(FullName);
  if (!NT_SUCCESS(Status)) return NULL;

  DRIVER_RegisterDriver( L"DISPLAY", GdiDriverInfo.EntryPoint);
  return (PGD_ENABLEDRIVER)GdiDriverInfo.EntryPoint;
}

BOOL DRIVER_BuildDDIFunctions(PDRVENABLEDATA  DED, 
                               PDRIVER_FUNCTIONS  DF)
{
  ULONG i;

  for (i=0; i<DED->c; i++)
  {
    if(DED->pdrvfn[i].iFunc == INDEX_DrvEnablePDEV)      DF->EnablePDev = (PGD_ENABLEPDEV)DED->pdrvfn[i].pfn;
    if(DED->pdrvfn[i].iFunc == INDEX_DrvCompletePDEV)    DF->CompletePDev = (PGD_COMPLETEPDEV)DED->pdrvfn[i].pfn;
    if(DED->pdrvfn[i].iFunc == INDEX_DrvDisablePDEV)     DF->DisablePDev = (PGD_DISABLEPDEV)DED->pdrvfn[i].pfn;
    if(DED->pdrvfn[i].iFunc == INDEX_DrvEnableSurface)   DF->EnableSurface = (PGD_ENABLESURFACE)DED->pdrvfn[i].pfn;
    if(DED->pdrvfn[i].iFunc == INDEX_DrvDisableSurface)  DF->DisableSurface = (PGD_DISABLESURFACE)DED->pdrvfn[i].pfn;
    if(DED->pdrvfn[i].iFunc == INDEX_DrvAssertMode)      DF->AssertMode = (PGD_ASSERTMODE)DED->pdrvfn[i].pfn;
    if(DED->pdrvfn[i].iFunc == INDEX_DrvResetPDEV)       DF->ResetPDev = (PGD_RESETPDEV)DED->pdrvfn[i].pfn;
    if(DED->pdrvfn[i].iFunc == INDEX_DrvCreateDeviceBitmap)
      DF->CreateDeviceBitmap = (PGD_CREATEDEVICEBITMAP)DED->pdrvfn[i].pfn;
    if(DED->pdrvfn[i].iFunc == INDEX_DrvDeleteDeviceBitmap)
      DF->DeleteDeviceBitmap = (PGD_DELETEDEVICEBITMAP)DED->pdrvfn[i].pfn;
    if(DED->pdrvfn[i].iFunc == INDEX_DrvRealizeBrush)    DF->RealizeBrush = (PGD_REALIZEBRUSH)DED->pdrvfn[i].pfn;
    if(DED->pdrvfn[i].iFunc == INDEX_DrvDitherColor)     DF->DitherColor = (PGD_DITHERCOLOR)DED->pdrvfn[i].pfn;
    if(DED->pdrvfn[i].iFunc == INDEX_DrvStrokePath)      DF->StrokePath = (PGD_STROKEPATH)DED->pdrvfn[i].pfn;
    if(DED->pdrvfn[i].iFunc == INDEX_DrvFillPath)        DF->FillPath = (PGD_FILLPATH)DED->pdrvfn[i].pfn;
    if(DED->pdrvfn[i].iFunc == INDEX_DrvStrokeAndFillPath)
      DF->StrokeAndFillPath = (PGD_STROKEANDFILLPATH)DED->pdrvfn[i].pfn;
    if(DED->pdrvfn[i].iFunc == INDEX_DrvPaint)           DF->Paint = (PGD_PAINT)DED->pdrvfn[i].pfn;
    if(DED->pdrvfn[i].iFunc == INDEX_DrvBitBlt)          DF->BitBlt = (PGD_BITBLT)DED->pdrvfn[i].pfn;
    if(DED->pdrvfn[i].iFunc == INDEX_DrvTransparentBlt)  DF->TransparentBlt = (PGD_TRANSPARENTBLT)DED->pdrvfn[i].pfn;
    if(DED->pdrvfn[i].iFunc == INDEX_DrvCopyBits)        DF->CopyBits = (PGD_COPYBITS)DED->pdrvfn[i].pfn;
    if(DED->pdrvfn[i].iFunc == INDEX_DrvStretchBlt)      DF->StretchBlt = (PGD_STRETCHBLT)DED->pdrvfn[i].pfn;
    if(DED->pdrvfn[i].iFunc == INDEX_DrvSetPalette)      DF->SetPalette = (PGD_SETPALETTE)DED->pdrvfn[i].pfn;
    if(DED->pdrvfn[i].iFunc == INDEX_DrvTextOut)         DF->TextOut = (PGD_TEXTOUT)DED->pdrvfn[i].pfn;
    if(DED->pdrvfn[i].iFunc == INDEX_DrvEscape)          DF->Escape = (PGD_ESCAPE)DED->pdrvfn[i].pfn;
    if(DED->pdrvfn[i].iFunc == INDEX_DrvDrawEscape)      DF->DrawEscape = (PGD_DRAWESCAPE)DED->pdrvfn[i].pfn;
    if(DED->pdrvfn[i].iFunc == INDEX_DrvQueryFont)       DF->QueryFont = (PGD_QUERYFONT)DED->pdrvfn[i].pfn;
    if(DED->pdrvfn[i].iFunc == INDEX_DrvQueryFontTree)   DF->QueryFontTree = (PGD_QUERYFONTTREE)DED->pdrvfn[i].pfn;
    if(DED->pdrvfn[i].iFunc == INDEX_DrvQueryFontData)   DF->QueryFontData = (PGD_QUERYFONTDATA)DED->pdrvfn[i].pfn;
    if(DED->pdrvfn[i].iFunc == INDEX_DrvSetPointerShape) DF->SetPointerShape = (PGD_SETPOINTERSHAPE)DED->pdrvfn[i].pfn;
    if(DED->pdrvfn[i].iFunc == INDEX_DrvMovePointer)     DF->MovePointer = (PGD_MOVEPOINTER)DED->pdrvfn[i].pfn;
    if(DED->pdrvfn[i].iFunc == INDEX_DrvLineTo)          DF->LineTo = (PGD_LINETO)DED->pdrvfn[i].pfn;
    if(DED->pdrvfn[i].iFunc == INDEX_DrvSendPage)        DF->SendPage = (PGD_SENDPAGE)DED->pdrvfn[i].pfn;
    if(DED->pdrvfn[i].iFunc == INDEX_DrvStartPage)       DF->StartPage = (PGD_STARTPAGE)DED->pdrvfn[i].pfn;
    if(DED->pdrvfn[i].iFunc == INDEX_DrvEndDoc)          DF->EndDoc = (PGD_ENDDOC)DED->pdrvfn[i].pfn;
    if(DED->pdrvfn[i].iFunc == INDEX_DrvStartDoc)        DF->StartDoc = (PGD_STARTDOC)DED->pdrvfn[i].pfn;
    if(DED->pdrvfn[i].iFunc == INDEX_DrvGetGlyphMode)    DF->GetGlyphMode = (PGD_GETGLYPHMODE)DED->pdrvfn[i].pfn;
    if(DED->pdrvfn[i].iFunc == INDEX_DrvSynchronize)     DF->Synchronize = (PGD_SYNCHRONIZE)DED->pdrvfn[i].pfn;
    if(DED->pdrvfn[i].iFunc == INDEX_DrvSaveScreenBits)  DF->SaveScreenBits = (PGD_SAVESCREENBITS)DED->pdrvfn[i].pfn;
    if(DED->pdrvfn[i].iFunc == INDEX_DrvGetModes)        DF->GetModes = (PGD_GETMODES)DED->pdrvfn[i].pfn;
    if(DED->pdrvfn[i].iFunc == INDEX_DrvFree)            DF->Free = (PGD_FREE)DED->pdrvfn[i].pfn;
    if(DED->pdrvfn[i].iFunc == INDEX_DrvDestroyFont)     DF->DestroyFont = (PGD_DESTROYFONT)DED->pdrvfn[i].pfn;
    if(DED->pdrvfn[i].iFunc == INDEX_DrvQueryFontCaps)   DF->QueryFontCaps = (PGD_QUERYFONTCAPS)DED->pdrvfn[i].pfn;
    if(DED->pdrvfn[i].iFunc == INDEX_DrvLoadFontFile)    DF->LoadFontFile = (PGD_LOADFONTFILE)DED->pdrvfn[i].pfn;
    if(DED->pdrvfn[i].iFunc == INDEX_DrvUnloadFontFile)  DF->UnloadFontFile = (PGD_UNLOADFONTFILE)DED->pdrvfn[i].pfn;
    if(DED->pdrvfn[i].iFunc == INDEX_DrvFontManagement)  DF->FontManagement = (PGD_FONTMANAGEMENT)DED->pdrvfn[i].pfn;
    if(DED->pdrvfn[i].iFunc == INDEX_DrvQueryTrueTypeTable)
      DF->QueryTrueTypeTable = (PGD_QUERYTRUETYPETABLE)DED->pdrvfn[i].pfn;
    if(DED->pdrvfn[i].iFunc == INDEX_DrvQueryTrueTypeOutline)
      DF->QueryTrueTypeOutline = (PGD_QUERYTRUETYPEOUTLINE)DED->pdrvfn[i].pfn;
    if(DED->pdrvfn[i].iFunc == INDEX_DrvGetTrueTypeFile) DF->GetTrueTypeFile = (PGD_GETTRUETYPEFILE)DED->pdrvfn[i].pfn;
    if(DED->pdrvfn[i].iFunc == INDEX_DrvQueryFontFile)   DF->QueryFontFile = (PGD_QUERYFONTFILE)DED->pdrvfn[i].pfn;
    if(DED->pdrvfn[i].iFunc == INDEX_DrvQueryAdvanceWidths)
      DF->QueryAdvanceWidths = (PGD_QUERYADVANCEWIDTHS)DED->pdrvfn[i].pfn;
    if(DED->pdrvfn[i].iFunc == INDEX_DrvSetPixelFormat)  DF->SetPixelFormat = (PGD_SETPIXELFORMAT)DED->pdrvfn[i].pfn;
    if(DED->pdrvfn[i].iFunc == INDEX_DrvDescribePixelFormat)
      DF->DescribePixelFormat = (PGD_DESCRIBEPIXELFORMAT)DED->pdrvfn[i].pfn;
    if(DED->pdrvfn[i].iFunc == INDEX_DrvSwapBuffers)     DF->SwapBuffers = (PGD_SWAPBUFFERS)DED->pdrvfn[i].pfn;
    if(DED->pdrvfn[i].iFunc == INDEX_DrvStartBanding)    DF->StartBanding = (PGD_STARTBANDING)DED->pdrvfn[i].pfn;
    if(DED->pdrvfn[i].iFunc == INDEX_DrvNextBand)        DF->NextBand = (PGD_NEXTBAND)DED->pdrvfn[i].pfn;
    if(DED->pdrvfn[i].iFunc == INDEX_DrvGetDirectDrawInfo) DF->GetDirectDrawInfo = (PGD_GETDIRECTDRAWINFO)DED->pdrvfn[i].pfn;
    if(DED->pdrvfn[i].iFunc == INDEX_DrvEnableDirectDraw)  DF->EnableDirectDraw = (PGD_ENABLEDIRECTDRAW)DED->pdrvfn[i].pfn;
    if(DED->pdrvfn[i].iFunc == INDEX_DrvDisableDirectDraw) DF->DisableDirectDraw = (PGD_DISABLEDIRECTDRAW)DED->pdrvfn[i].pfn;
    if(DED->pdrvfn[i].iFunc == INDEX_DrvQuerySpoolType)  DF->QuerySpoolType = (PGD_QUERYSPOOLTYPE)DED->pdrvfn[i].pfn;
  }

  return TRUE;
}

typedef VP_STATUS (*PMP_DRIVERENTRY)(PVOID, PVOID);

PDEVICE_OBJECT DRIVER_FindMPDriver(LPCWSTR Name)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING DeviceName;
  IO_STATUS_BLOCK Iosb;
  HANDLE DisplayHandle;
  NTSTATUS Status;
  PFILE_OBJECT VideoFileObject;
  PDEVICE_OBJECT VideoDeviceObject;

  RtlRosInitUnicodeStringFromLiteral(&DeviceName, L"\\??\\DISPLAY1");
  InitializeObjectAttributes(&ObjectAttributes,
			     &DeviceName,
			     0,
			     NULL,
			     NULL);
  Status = ZwOpenFile(&DisplayHandle,
		      FILE_ALL_ACCESS,
		      &ObjectAttributes,
		      &Iosb,
		      0,
		      FILE_SYNCHRONOUS_IO_ALERT);
  if (NT_SUCCESS(Status))
    {
      Status = ObReferenceObjectByHandle(DisplayHandle,
                                         FILE_READ_DATA | FILE_WRITE_DATA,
                                         IoFileObjectType,
                                         KernelMode,
                                         (PVOID *)&VideoFileObject,
                                         NULL);
      if (NT_SUCCESS(Status))
        {
          VideoDeviceObject = VideoFileObject->DeviceObject;
          ObReferenceObject(VideoDeviceObject);
          ObDereferenceObject(VideoFileObject);
        }
      ZwClose(DisplayHandle);
    }

  if (!NT_SUCCESS(Status))
    {
      DPRINT1("Unable to connect to miniport (Status %lx)\n", Status);
      DPRINT1("Perhaps the miniport wasn't loaded?\n");
      return(NULL);
    }

  return VideoDeviceObject;
}


BOOL DRIVER_UnregisterDriver(LPCWSTR  Name)
{
  PGRAPHICS_DRIVER  Driver = NULL;
  
  if (Name)
  {
    if (DriverList != NULL)
    {
      if (!_wcsicmp(DriverList->Name, Name))
      {
        Driver = DriverList;
        DriverList = DriverList->Next;
      }
      else
      {
        Driver = DriverList;
        while (Driver->Next && _wcsicmp(Driver->Name, Name))
        {
          Driver = Driver->Next;
        }
      }
    }
  }
  else
  {    
    if (GenericDriver != NULL)
    {
      Driver = GenericDriver;
      GenericDriver = NULL;
    }
  }
  
  if (Driver != NULL)
  {
    ExFreePool(Driver->Name);
    ExFreePool(Driver);
      
    return  TRUE;
  }
  else
  {
    return  FALSE;
  }
}

INT DRIVER_ReferenceDriver (LPCWSTR  Name)
{
  GRAPHICS_DRIVER *Driver = DriverList;
  
  while (Driver && Name)
  {
    DPRINT( "Comparing %S to %S\n", Driver->Name, Name );
    if (!_wcsicmp( Driver->Name, Name)) 
    {
      return ++Driver->ReferenceCount;
    }
    Driver = Driver->Next;
  }
  DPRINT( "Driver %S not found to reference, generic count: %d\n", Name, GenericDriver->ReferenceCount );
  assert( GenericDriver != 0 );
  return ++GenericDriver->ReferenceCount;
}

INT DRIVER_UnreferenceDriver (LPCWSTR  Name)
{
  GRAPHICS_DRIVER *Driver = DriverList;
  
  while (Driver && Name)
  {
    DPRINT( "Comparing %S to %S\n", Driver->Name, Name );
    if (!_wcsicmp( Driver->Name, Name)) 
    {
      return --Driver->ReferenceCount;
    }
    Driver = Driver->Next;
  }
  DPRINT( "Driver '%S' not found to dereference, generic count: %d\n", Name, GenericDriver->ReferenceCount );
  assert( GenericDriver != 0 );
  return --GenericDriver->ReferenceCount;
}
/* EOF */
