/* $Id: driver.c,v 1.15 2000/08/26 16:22:04 ekohl Exp $
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
#include <wchar.h>
#include "../../ntoskrnl/include/internal/module.h"
#include <ddk/winddi.h>
#include <ddk/ntddvid.h>

#define NDEBUG
#include <debug.h>

typedef struct _GRAPHICS_DRIVER
{
  PWSTR  Name;
  PGD_ENABLEDRIVER  EnableDriver;
  int  ReferenceCount;
  struct _GRAPHICS_DRIVER  *Next;
} GRAPHICS_DRIVER, *PGRAPHICS_DRIVER;

static PGRAPHICS_DRIVER  DriverList;
static PGRAPHICS_DRIVER  GenericDriver = 0;

BOOL  DRIVER_RegisterDriver(LPCWSTR  Name, PGD_ENABLEDRIVER  EnableDriver)
{
  PGRAPHICS_DRIVER  Driver = ExAllocatePool(NonPagedPool, sizeof(*Driver));
  DPRINT( "DRIVER_RegisterDriver( Name: %S )\n", Name );
  if (!Driver) 
    {
      return  FALSE;
    }
  Driver->ReferenceCount = 0;
  Driver->EnableDriver = EnableDriver;
  if (Name)
    {
      Driver->Name = ExAllocatePool(PagedPool, 
                                    (wcslen(Name) + 1) * sizeof(WCHAR));
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

PGD_ENABLEDRIVER  DRIVER_FindDDIDriver(LPCWSTR  Name)
{
  SYSTEM_GDI_DRIVER_INFORMATION GdiDriverInfo;
  GRAPHICS_DRIVER *Driver = DriverList;
  NTSTATUS Status;

  /* First see if the driver hasn't already been loaded */
  while (Driver && Name)
    {
      if (!_wcsicmp( Driver->Name, Name)) 
        {
          return Driver->EnableDriver;
        }
      Driver = Driver->Next;
    }

  /* If not, then load it */
  RtlInitUnicodeString (&GdiDriverInfo.DriverName,
			(LPWSTR)Name);
  Status = ZwSetSystemInformation (SystemLoadGdiDriverInformation,
				   &GdiDriverInfo,
				   sizeof(SYSTEM_GDI_DRIVER_INFORMATION));
   if (!NT_SUCCESS(Status))
	return NULL;

  DRIVER_RegisterDriver( L"DISPLAY", GdiDriverInfo.EntryPoint);
  return (PGD_ENABLEDRIVER)GdiDriverInfo.EntryPoint;
}

BOOL  DRIVER_BuildDDIFunctions(PDRVENABLEDATA  DED, 
                               PDRIVER_FUNCTIONS  DF)
{
  int i;

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
#if 0
    if(DED->pdrvfn[i].iFunc == INDEX_DrvGetDirectDrawInfo) DF->GETDIRECTDRAWINFO = (PGD_)DED->pdrvfn[i].pfn;
    if(DED->pdrvfn[i].iFunc == INDEX_DrvEnableDirectDraw)  DF->ENABLEDIRECTDRAW = (PGD_)DED->pdrvfn[i].pfn;
    if(DED->pdrvfn[i].iFunc == INDEX_DrvDisableDirectDraw) DF->DISABLEDIRECTDRAW = (PGD_)DED->pdrvfn[i].pfn;
#endif
    if(DED->pdrvfn[i].iFunc == INDEX_DrvQuerySpoolType)  DF->QuerySpoolType = (PGD_QUERYSPOOLTYPE)DED->pdrvfn[i].pfn;
  }

  return TRUE;
}

typedef VP_STATUS (*PMP_DRIVERENTRY)(PVOID, PVOID);

HANDLE DRIVER_FindMPDriver(LPCWSTR  Name)
{
  SYSTEM_GDI_DRIVER_INFORMATION GdiDriverInfo;
  PWSTR  lName;
  NTSTATUS  Status;
  UNICODE_STRING  DeviceName;
  HANDLE DriverHandle;
  OBJECT_ATTRIBUTES  ObjectAttributes;

  PDRIVER_OBJECT  DriverObject;
  PMP_DRIVERENTRY PMP_DriverEntry;

  /* Phase 1 */
  RtlInitUnicodeString (&GdiDriverInfo.DriverName,
			L"\\SystemRoot\\system32\\drivers\\vgamp.sys");
  Status = ZwSetSystemInformation (SystemLoadGdiDriverInformation,
				   &GdiDriverInfo,
				   sizeof(SYSTEM_GDI_DRIVER_INFORMATION));
  if (!NT_SUCCESS(Status))
     return NULL;

  /* Phase 2 */
  if (Name[0] != '\\')
    {
      lName = ExAllocatePool(NonPagedPool, wcslen(Name) * sizeof(WCHAR) + 
                             10 * sizeof(WCHAR));
      wcscpy(lName, L"\\??\\");
      if (!wcscmp (Name, L"DISPLAY"))
        {
           /* FIXME: Read this information from the registry ??? */
           wcscat(lName, L"DISPLAY1");
        }
      else
        {
           wcscat(lName, Name);
        }
    }
  else
    {
      lName = ExAllocatePool(NonPagedPool, wcslen(Name) * sizeof(WCHAR));
      wcscpy(lName, Name);
    }
  
  /* Phase 3 */
  DriverObject = ExAllocatePool(NonPagedPool,sizeof(DRIVER_OBJECT));
  if (DriverObject == NULL)
  {
    return NULL;
  }
  memset(DriverObject, 0, sizeof(DRIVER_OBJECT));

  // We pass the DriverObject to the Miniport driver, which passes it to the VideoPort driver
  // The VideoPort driver then creates the Device Object

  PMP_DriverEntry = GdiDriverInfo.EntryPoint;
  PMP_DriverEntry(DriverObject, NULL);

  return  DriverObject;
}

BOOL  DRIVER_UnregisterDriver(LPCWSTR  Name)
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

INT  DRIVER_ReferenceDriver (LPCWSTR  Name)
{
  GRAPHICS_DRIVER *Driver = DriverList;
  
  while (Driver && Name)
    {
      DPRINT( "Comparting %S to %S\n", Driver->Name, Name );
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

INT  DRIVER_UnreferenceDriver (LPCWSTR  Name)
{
  GRAPHICS_DRIVER *Driver = DriverList;
  
  while (Driver && Name)
    {
      DPRINT( "Comparting %S to %S\n", Driver->Name, Name );
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
