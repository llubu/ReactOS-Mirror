#include "precomp.h"

#define NDEBUG
#include <debug.h>

HGDIOBJ stock_objects[NB_STOCK_OBJECTS]; // temp location.

HDC
FASTCALL
IntCreateDICW ( LPCWSTR   lpwszDriver,
                LPCWSTR   lpwszDevice,
                LPCWSTR   lpwszOutput,
                PDEVMODEW lpInitData,
                ULONG     iType )
{
 UNICODE_STRING Device, Output;
 HDC hDC = NULL;
 BOOL Display = FALSE;
 ULONG UMdhpdev = 0;
 
 HANDLE hspool = NULL;
                
 if ((!lpwszDevice) && (!lpwszDriver)) return hDC;
 else
 {
    if (lpwszDevice) // First
    {
      if (!_wcsnicmp(lpwszDevice, L"\\\\.\\DISPLAY",11)) Display = TRUE;
      RtlInitUnicodeString(&Device, lpwszDevice);
    }
    else
    {
      if (lpwszDriver) // Second
      {
        if ((!_wcsnicmp(lpwszDriver, L"DISPLAY",7)) || 
              (!_wcsnicmp(lpwszDriver, L"\\\\.\\DISPLAY",11))) Display = TRUE;
        RtlInitUnicodeString(&Device, lpwszDriver);
      }
    }
 }
 
 if (lpwszOutput) RtlInitUnicodeString(&Output, lpwszOutput);

 if (!Display)
 {
    //Handle Print device or something else.
    DPRINT1("Not a DISPLAY device! %wZ\n", &Device);
 }
        
 hDC = NtGdiOpenDCW( &Device,
                     (PDEVMODEW) lpInitData,
                     (lpwszOutput ? &Output : NULL),
                      iType,             // DCW 0 and ICW 1.
                      hspool,
                     (PVOID) NULL,       // NULL for now.
                     (PVOID) &UMdhpdev );

// Handle something other than a normal dc object.
 if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
 {
    PDC_ATTR Dc_Attr;
    PLDC pLDC;

    GdiGetHandleUserData((HGDIOBJ) hDC, (PVOID) &Dc_Attr);

    pLDC = LocalAlloc(LMEM_ZEROINIT, sizeof(LDC));

    Dc_Attr->pvLDC = pLDC;
    pLDC->hDC = hDC;
    pLDC->iType = LDC_LDC; // 1 (init) local DC, 2 EMF LDC
 }

 return hDC;     
}


/*
 * @implemented
 */
HDC
STDCALL
CreateDCA (
	LPCSTR		lpszDriver,
	LPCSTR		lpszDevice,
	LPCSTR		lpszOutput,
	CONST DEVMODEA	* lpInitData
	)
{
        ANSI_STRING DriverA, DeviceA, OutputA;
        UNICODE_STRING DriverU, DeviceU, OutputU;
	HDC	hDC;
	DEVMODEW *lpInitDataW;

	/*
	 * If needed, convert to Unicode
	 * any string parameter.
	 */

	if (NULL != lpszDriver)
	{
		RtlInitAnsiString(&DriverA, (LPSTR)lpszDriver);
		RtlAnsiStringToUnicodeString(&DriverU, &DriverA, TRUE);
	} else
		DriverU.Buffer = NULL;
	if (NULL != lpszDevice)
	{
		RtlInitAnsiString(&DeviceA, (LPSTR)lpszDevice);
		RtlAnsiStringToUnicodeString(&DeviceU, &DeviceA, TRUE);
	} else
		DeviceU.Buffer = NULL;
	if (NULL != lpszOutput)
	{
		RtlInitAnsiString(&OutputA, (LPSTR)lpszOutput);
		RtlAnsiStringToUnicodeString(&OutputU, &OutputA, TRUE);
	} else
		OutputU.Buffer = NULL;

	if (NULL != lpInitData)
	{
//		lpInitDataW = HeapAllocMem(
	} else
		lpInitDataW = NULL;

	/*
	 * Call the Unicode version
	 * of CreateDC.
	 */

	hDC = CreateDCW (
		DriverU.Buffer,
		DeviceU.Buffer,
		OutputU.Buffer,
		NULL);
//		lpInitDataW);
	/*
	 * Free Unicode parameters.
	 */
	RtlFreeUnicodeString(&DriverU);
	RtlFreeUnicodeString(&DeviceU);
	RtlFreeUnicodeString(&OutputU);

	/*
	 * Return the possible DC handle.
	 */

	return hDC;
}


/*
 * @implemented
 */
HDC
STDCALL
CreateDCW (
	LPCWSTR		lpwszDriver,
	LPCWSTR		lpwszDevice,
	LPCWSTR		lpwszOutput,
	CONST DEVMODEW	* lpInitData
	)
{
	UNICODE_STRING Driver, Device, Output;

	if(lpwszDriver)
		RtlInitUnicodeString(&Driver, lpwszDriver);
	if(lpwszDevice)
		RtlInitUnicodeString(&Driver, lpwszDevice);
	if(lpwszOutput)
		RtlInitUnicodeString(&Driver, lpwszOutput);

	HDC hDC =  NtGdiCreateDC((lpwszDriver ? &Driver : NULL),
						 (lpwszDevice ? &Device : NULL),
						 (lpwszOutput ? &Output : NULL),
						 (PDEVMODEW)lpInitData);
/* DC_ATTR Tests.
 if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_DC)
 {
    PDC_ATTR Dc_Attr;

    GdiGetHandleUserData((HGDIOBJ) hDC, (PVOID) &Dc_Attr);
    DPRINT1("Test DC_ATTR -> ! 0x%x\n", Dc_Attr);

    DPRINT1("Test DC_ATTR access! 0x%x\n", Dc_Attr->pvLDC);
    Dc_Attr->pvLDC = (PVOID)1;
    DPRINT1("Test DC_ATTR access! 0x%x\n", Dc_Attr->pvLDC);
    Dc_Attr->pvLDC = (PVOID)0;
    DPRINT1("Test DC_ATTR access! 0x%x\n", Dc_Attr->pvLDC);

 }
 */
   return hDC;
}


/*
 * @implemented
 */
HDC
STDCALL
CreateICW(
	LPCWSTR			lpszDriver,
	LPCWSTR			lpszDevice,
	LPCWSTR			lpszOutput,
	CONST DEVMODEW *	lpdvmInit
	)
{
  UNICODE_STRING Driver, Device, Output;
  
  if(lpszDriver)
    RtlInitUnicodeString(&Driver, lpszDriver);
  if(lpszDevice)
    RtlInitUnicodeString(&Device, lpszDevice);
  if(lpszOutput)
    RtlInitUnicodeString(&Output, lpszOutput);
  return NtGdiCreateIC ((lpszDriver ? &Driver : NULL),
		      (lpszDevice ? &Device : NULL),
		      (lpszOutput ? &Output : NULL),
		      (CONST PDEVMODEW)lpdvmInit );
}


/*
 * @implemented
 */
HDC
STDCALL
CreateICA(
	LPCSTR			lpszDriver,
	LPCSTR			lpszDevice,
	LPCSTR			lpszOutput,
	CONST DEVMODEA *	lpdvmInit
	)
{
  NTSTATUS Status;
  LPWSTR lpszDriverW, lpszDeviceW, lpszOutputW;
  UNICODE_STRING Driver, Device, Output;
  LPDEVMODEW dvmInitW = NULL;
  HDC rc = 0;

  Status = HEAP_strdupA2W ( &lpszDriverW, lpszDriver );
  if (!NT_SUCCESS (Status))
    SetLastError (RtlNtStatusToDosError(Status));
  else
  {
    Status = HEAP_strdupA2W ( &lpszDeviceW, lpszDevice );
    if (!NT_SUCCESS (Status))
      SetLastError (RtlNtStatusToDosError(Status));
    else
      {
	Status = HEAP_strdupA2W ( &lpszOutputW, lpszOutput );
	if (!NT_SUCCESS (Status))
	  SetLastError (RtlNtStatusToDosError(Status));
	else
	  {
	    if ( lpdvmInit )
          dvmInitW = GdiConvertToDevmodeW((LPDEVMODEA)lpdvmInit);
        
        RtlInitUnicodeString(&Driver, lpszDriverW);
        RtlInitUnicodeString(&Device, lpszDeviceW);
        RtlInitUnicodeString(&Output, lpszOutputW);
	    rc = NtGdiCreateIC ( &Driver,
				&Device,
				&Output,
				lpdvmInit ? dvmInitW : NULL );
        HEAP_free (dvmInitW);
	    HEAP_free ( lpszOutputW );
	  }
	HEAP_free ( lpszDeviceW );
      }
    HEAP_free ( lpszDriverW );
  }
  return rc;
}


/*
 * @implemented
 */
BOOL
STDCALL
NEWDeleteDC(HDC hDC)
{
  BOOL Ret = TRUE;
  PDC_ATTR Dc_Attr;
  PLDC pLDC;

  Ret = GdiGetHandleUserData((HGDIOBJ) hDC, (PVOID) &Dc_Attr);

  if ( !Ret ) return FALSE;
  
  if ( Dc_Attr )
    {
      pLDC = Dc_Attr->pvLDC;

      if ( pLDC )
        {
          DPRINT1("Delete the Local DC structure\n");
          LocalFree( pLDC );
        }
    }

  Ret = NtGdiDeleteObjectApp(hDC);
  
  return Ret;
}


/*

 * @implemented
 */
BOOL
STDCALL
DeleteObject(HGDIOBJ hObject)
{
  /* From Wine: DeleteObject does not SetLastError() on a null object */
  if(!hObject) return FALSE;
  
  if (0 != ((DWORD) hObject & GDI_HANDLE_STOCK_MASK))
    {
      DPRINT1("Trying to delete system object 0x%x\n", hObject);
      return TRUE;
    }

  /* deleting a handle that doesn't belong to the caller should be rather rarely
     so for the sake of speed just try to delete it without checking validity */
  return NtGdiDeleteObject(hObject);
}


/*
 * @implemented
 */
DWORD
STDCALL
GetRelAbs(
         HDC  hdc,
         DWORD dwIgnore
           )
{
  return NtGdiGetRelAbs(hdc);
}


/*
 * @implemented
 */
DWORD
STDCALL
GetAndSetDCDWord( HDC hDC, INT u, DWORD dwIn, DWORD Unk1, DWORD Unk2, DWORD Unk3 )
{
  BOOL Ret = TRUE; 
// Handle something other than a normal dc object.
  if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
  {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
       return 0; //call MFDRV
    else
    {
       PLDC pLDC = GdiGetLDC(hDC);
       if ( !pLDC )
       {
           SetLastError(ERROR_INVALID_HANDLE);
           return 0;
       }
       if (pLDC->iType == LDC_EMFLDC)
       {
          Ret = TRUE; //call EMFDRV
          if (Ret)
             return u;
          return 0;
       }
    }
  }
// Ret = NtGdiGetAndSetDCDword( hDC, u, dwIn, (DWORD*) &u );                
  if (Ret) 
     return u;
  else 
     SetLastError(ERROR_INVALID_HANDLE);
  return 0;
}


/*
 * @implemented
 */
DWORD
STDCALL
GetDCDWord( HDC hDC, INT u, DWORD Result )
{
  BOOL Ret = TRUE; //NtGdiGetDCDword( hDC, u, (DWORD*) &u );
  if (!Ret) return Result;
  else return u;
}


/*
 * @implemented
*/
LONG
STDCALL
GetDCOrg(
    HDC hdc
    )
{
  // Officially obsolete by Microsoft
  POINT Pt;
  if (!NtGdiGetDCOrgEx(hdc, &Pt))
    return 0;
  return(MAKELONG(Pt.x, Pt.y));
}


int   
GetNonFontObject(HGDIOBJ hGdiObj, int cbSize, LPVOID lpBuffer)
{
  INT dwType;

  hGdiObj = (HANDLE)GdiFixUpHandle(hGdiObj);
  dwType = GDI_HANDLE_GET_TYPE(hGdiObj);

  if (!lpBuffer) // Should pass it all to Win32k and let god sort it out. ;^)
  {
    switch(dwType)
    {
      case GDI_OBJECT_TYPE_PEN:
        return sizeof(LOGPEN);
      case GDI_OBJECT_TYPE_BRUSH:
        return sizeof(LOGBRUSH);
      case GDI_OBJECT_TYPE_BITMAP:
        return sizeof(BITMAP);
      case GDI_OBJECT_TYPE_PALETTE:
        return sizeof(WORD);
      case GDI_OBJECT_TYPE_EXTPEN: /* we don't know the size, ask win32k */
        break;
    }
  }

  switch(dwType)
  {
    case GDI_OBJECT_TYPE_PEN: //Check the structures and see if A & W are the same.
    case GDI_OBJECT_TYPE_EXTPEN:
    case GDI_OBJECT_TYPE_BRUSH: // Mixing Apples and Oranges?
    case GDI_OBJECT_TYPE_BITMAP:
    case GDI_OBJECT_TYPE_PALETTE:
      return NtGdiExtGetObjectW(hGdiObj, cbSize, lpBuffer);

    case GDI_OBJECT_TYPE_DC:
    case GDI_OBJECT_TYPE_REGION:
    case GDI_OBJECT_TYPE_METAFILE:
    case GDI_OBJECT_TYPE_ENHMETAFILE:
    case GDI_OBJECT_TYPE_EMF:    
      SetLastError(ERROR_INVALID_HANDLE);
  }
  return 0;
}


/*
 * @implemented
 */
int   
STDCALL 
GetObjectA(HGDIOBJ hGdiObj, int cbSize, LPVOID lpBuffer)
{
  ENUMLOGFONTEXDVW LogFont;
  DWORD dwType;
  INT Result = 0;

  dwType = GDI_HANDLE_GET_TYPE(hGdiObj);;

  if(dwType == GDI_OBJECT_TYPE_COLORSPACE) //Stays here, processes struct A
  {
     SetLastError(ERROR_NOT_SUPPORTED);
     return 0;
  } 

  if (dwType == GDI_OBJECT_TYPE_FONT)
  {
    if (!lpBuffer)
    {
      return sizeof(LOGFONTA);
    }
    if (cbSize == 0)
    {
      /* Windows does not SetLastError() */
      return 0;
    }
    // ENUMLOGFONTEXDVW is the default size and should be the structure for
    // Entry->KernelData for Font objects.
    Result = NtGdiExtGetObjectW(hGdiObj, sizeof(ENUMLOGFONTEXDVW), &LogFont);

    if (0 == Result)
    {
      return 0;
    }
  
    switch (cbSize)
      {
         case sizeof(ENUMLOGFONTEXDVA):
         // need to move more here.
         case sizeof(ENUMLOGFONTEXA):
            EnumLogFontExW2A( (LPENUMLOGFONTEXA) lpBuffer, &LogFont.elfEnumLogfontEx );
            break;
             
         case sizeof(ENUMLOGFONTA):
         // Same here, maybe? Check the structures.
         case sizeof(EXTLOGFONTA):
         // Same here
         case sizeof(LOGFONTA):
            LogFontW2A((LPLOGFONTA) lpBuffer, &LogFont.elfEnumLogfontEx.elfLogFont);
            break;

         default:
            SetLastError(ERROR_BUFFER_OVERFLOW);
            return 0;         
      }
    return cbSize;
  }

  return GetNonFontObject(hGdiObj, cbSize, lpBuffer);
}


/*
 * @implemented
 */
int   
STDCALL 
GetObjectW(HGDIOBJ hGdiObj, int cbSize, LPVOID lpBuffer)
{
  DWORD dwType = GDI_HANDLE_GET_TYPE(hGdiObj);
  INT Result = 0;

/*
  Check List:
  MSDN, "This can be a handle to one of the following: logical bitmap, a brush,
  a font, a palette, a pen, or a device independent bitmap created by calling
  the CreateDIBSection function."
 */
  if(dwType == GDI_OBJECT_TYPE_COLORSPACE) //Stays here, processes struct W
  {
     SetLastError(ERROR_NOT_SUPPORTED); // Not supported yet.
     return 0;
  }

  if (dwType == GDI_OBJECT_TYPE_FONT)
  {
    if (!lpBuffer)
    {
      return sizeof(LOGFONTW);
    }

    if (cbSize == 0)
    {
      /* Windows does not SetLastError() */
      return 0;
    }
    // Poorly written apps are not ReactOS problem!
    // We fix it here if the size is larger than the default size.
    if( cbSize > sizeof(ENUMLOGFONTEXDVW) ) cbSize = sizeof(ENUMLOGFONTEXDVW);

    Result = NtGdiExtGetObjectW(hGdiObj, cbSize, lpBuffer); // Should handle the copy.

    if (0 == Result)
    {
      return 0;
    }
    return cbSize;
  }

  return GetNonFontObject(hGdiObj, cbSize, lpBuffer);
}


/*
 * @implemented
 */
COLORREF 
STDCALL
GetDCBrushColor(
	HDC hdc
)
{
  return NtUserGetDCBrushColor(hdc);
}

/*
 * @implemented
 */
COLORREF 
STDCALL
GetDCPenColor(
	HDC hdc
)
{
  return NtUserGetDCPenColor(hdc);
}

/*
 * @implemented
 */
COLORREF 
STDCALL
SetDCBrushColor(
	HDC hdc,
	COLORREF crColor
)
{
  return NtUserSetDCBrushColor(hdc, crColor);
}

/*
 * @implemented
 */
COLORREF 
STDCALL
SetDCPenColor(
	HDC hdc,
	COLORREF crColor
)
{
  return NtUserSetDCPenColor(hdc, crColor);
}


/*
 * @implemented
 */
HDC
STDCALL
ResetDCW(
	HDC		hdc,
	CONST DEVMODEW	*lpInitData
	)
{
  NtGdiResetDC ( hdc, (PDEVMODEW)lpInitData, NULL, NULL, NULL);
  return hdc;
}


/*
 * @implemented
 */
HDC
STDCALL
ResetDCA(
	HDC		hdc,
	CONST DEVMODEA	*lpInitData
	)
{
  LPDEVMODEW InitDataW;

  InitDataW = GdiConvertToDevmodeW((LPDEVMODEA)lpInitData);

  NtGdiResetDC ( hdc, InitDataW, NULL, NULL, NULL);
  HEAP_free(InitDataW);
  return hdc;
}


/*
 * @implemented
 */
int 
STDCALL 
StartDocW(
	HDC		hdc,
	CONST DOCINFOW	*a1
	)
{
	return NtGdiStartDoc ( hdc, (DOCINFOW *)a1, NULL, 0);
}


/*
 * @implemented
 */
DWORD
STDCALL
GetObjectType(
	HGDIOBJ h
	)
{
  DWORD Ret = 0;
  
  if(GdiIsHandleValid(h))
  {
    LONG Type = GDI_HANDLE_GET_TYPE(h);
    switch(Type)
    {
      case GDI_OBJECT_TYPE_PEN:
        Ret = OBJ_PEN;
        break;
      case GDI_OBJECT_TYPE_BRUSH:
        Ret = OBJ_BRUSH;
        break;
      case GDI_OBJECT_TYPE_BITMAP:
        Ret = OBJ_BITMAP;
        break;
      case GDI_OBJECT_TYPE_FONT:
        Ret = OBJ_FONT;
        break;
      case GDI_OBJECT_TYPE_PALETTE:
        Ret = OBJ_PAL;
        break;
      case GDI_OBJECT_TYPE_REGION:
        Ret = OBJ_REGION;
        break;
      case GDI_OBJECT_TYPE_DC:
        if ( GetDCDWord( h, GdiGetIsMemDc, 0))
        {
           Ret = OBJ_MEMDC;
        }
        else
           Ret = OBJ_DC;
        break;
      case GDI_OBJECT_TYPE_COLORSPACE:
        Ret = OBJ_COLORSPACE;
        break;
      case GDI_OBJECT_TYPE_METAFILE:
        Ret = OBJ_METAFILE;
        break;
      case GDI_OBJECT_TYPE_ENHMETAFILE:
        Ret = OBJ_ENHMETAFILE;
        break;
      case GDI_OBJECT_TYPE_ENHMETADC:
        Ret = OBJ_ENHMETADC;
        break;
      case GDI_OBJECT_TYPE_EXTPEN:
        Ret = OBJ_EXTPEN;
        break;
      case GDI_OBJECT_TYPE_MEMDC:
        Ret = OBJ_MEMDC;
        break;

      default:
        DPRINT1("GetObjectType: Magic 0x%08x not implemented\n", Type);
        break;
    }
  }
  else
    /* From Wine: GetObjectType does SetLastError() on a null object */
    SetLastError(ERROR_INVALID_HANDLE);
  return Ret;
}


/*
 * @implemented
 */
HGDIOBJ
WINAPI
GetStockObject(
              INT h
              )
{
  HGDIOBJ Ret = NULL;
  if ((h < 0) || (h >= NB_STOCK_OBJECTS)) return Ret;
  Ret = stock_objects[h];
  if (!Ret)
  {
      HGDIOBJ Obj = NtGdiGetStockObject( h );

      if (GdiIsHandleValid(Obj))
      {
         stock_objects[h] = Obj;
         return Obj;
      }// Returns Null anyway.
  }
  return Ret;                    
}

