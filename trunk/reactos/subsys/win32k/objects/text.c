

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <win32k/dc.h>
#include <win32k/text.h>
#include <win32k/kapi.h>
#include <freetype/freetype.h>

#include "../eng/objects.h"

// #define NDEBUG
#include <win32k/debug1.h>

FT_Library  library;

typedef struct _FONTTABLE {
  HFONT hFont;
  LPCWSTR FaceName;
} FONTTABLE, *PFONTTABLE;

FONTTABLE FontTable[256];
INT FontsLoaded = 0;

BOOL InitFontSupport()
{
  ULONG error;

  error = FT_Init_FreeType(&library);
  if(error)
  {
    return FALSE;
  }

  W32kAddFontResource(L"\\SystemRoot\\media\\fonts\\helb____.ttf");
  W32kAddFontResource(L"\\SystemRoot\\media\\fonts\\timr____.ttf");

  DbgPrint("All fonts loaded\n");

  return TRUE;
}

int
STDCALL
W32kAddFontResource(LPCWSTR  Filename)
{
  HFONT NewFont;
  PFONTOBJ FontObj;
  PFONTGDI FontGDI;
  UNICODE_STRING uFileName;
  NTSTATUS Status;
  HANDLE FileHandle;
  OBJECT_ATTRIBUTES ObjectAttributes;
  FILE_STANDARD_INFORMATION FileStdInfo;
  PVOID buffer;
  ULONG size;
  INT error;
  FT_Face face;
  ANSI_STRING StringA;
  UNICODE_STRING StringU;

  FontObj = EngAllocMem(FL_ZERO_MEMORY, sizeof(XLATEOBJ), NULL);
  FontGDI = EngAllocMem(FL_ZERO_MEMORY, sizeof(XLATEGDI), NULL);
  NewFont = CreateGDIHandle(FontGDI, FontObj);

  RtlCreateUnicodeString(&uFileName, Filename);

  //  Open the Module
  InitializeObjectAttributes(&ObjectAttributes, &uFileName, 0, NULL, NULL);

  Status = NtOpenFile(&FileHandle, FILE_ALL_ACCESS, &ObjectAttributes, NULL, 0, 0);

  if (!NT_SUCCESS(Status))
  {
    DbgPrint("Could not open module file: %wZ\n", Filename);
    return 0;
  }

  //  Get the size of the file
  Status = NtQueryInformationFile(FileHandle, NULL, &FileStdInfo, sizeof(FileStdInfo), FileStandardInformation);
  if (!NT_SUCCESS(Status))
  {
    DbgPrint("Could not get file size\n");
    return 0;
  }

  //  Allocate nonpageable memory for driver
  size = FileStdInfo.EndOfFile.u.LowPart;
  buffer = ExAllocatePool(NonPagedPool, size);

  if (buffer == NULL)
  {
    DbgPrint("could not allocate memory for module");
    return 0;
  }
   
  //  Load driver into memory chunk
  Status = NtReadFile(FileHandle, 0, 0, 0, 0, buffer, FileStdInfo.EndOfFile.u.LowPart, 0, 0);
  if (!NT_SUCCESS(Status))
  {
    DbgPrint("could not read module file into memory");
    ExFreePool(buffer);
    return 0;
  }

  NtClose(FileHandle);

  error = FT_New_Memory_Face(library, buffer, size, 0, &face);
  if (error == FT_Err_Unknown_File_Format)
  {
    DbgPrint("Unknown font file format\n");
    return 0;
  }
  else if (error)
  {
    DbgPrint("Error reading font file (error code: %u)\n", error); // 48
    return 0;
  }

  // FontGDI->Filename = Filename; perform strcpy
  FontGDI->face = face;

  // FIXME: Complete text metrics
  FontGDI->TextMetric.tmAscent = face->size->metrics.ascender; // units above baseline
  FontGDI->TextMetric.tmDescent = face->size->metrics.descender; // units below baseline
  FontGDI->TextMetric.tmHeight = FontGDI->TextMetric.tmAscent + FontGDI->TextMetric.tmDescent;

  DbgPrint("Family name: %s\n", face->family_name);
  DbgPrint("Style name: %s\n", face->style_name);
  DbgPrint("Num glyphs: %u\n", face->num_glyphs);

  // Add this font resource to the font table
  FontTable[FontsLoaded].hFont = NewFont;
  FontTable[FontsLoaded].FaceName = ExAllocatePool(NonPagedPool, (StringU.Length + 1) * 2);

  RtlInitAnsiString(&StringA, (LPSTR)face->family_name);
  RtlAnsiStringToUnicodeString(&StringU, &StringA, TRUE);
  wcscpy(FontTable[FontsLoaded].FaceName, StringU.Buffer);
  RtlFreeUnicodeString(&StringU);

  FontsLoaded++;

  return 1;
}

HFONT
STDCALL
W32kCreateFont(int  Height,
                      int  Width,
                      int  Escapement,
                      int  Orientation,
                      int  Weight,
                      DWORD  Italic,
                      DWORD  Underline,
                      DWORD  StrikeOut,
                      DWORD  CharSet,
                      DWORD  OutputPrecision,
                      DWORD  ClipPrecision,
                      DWORD  Quality,
                      DWORD  PitchAndFamily,
                      LPCWSTR  Face)
{
  LOGFONT logfont;

  logfont.lfHeight = Height;
  logfont.lfWidth = Width;
  logfont.lfEscapement = Escapement;
  logfont.lfOrientation = Orientation;
  logfont.lfWeight = Weight;
  logfont.lfItalic = Italic;
  logfont.lfUnderline = Underline;
  logfont.lfStrikeOut = StrikeOut;
  logfont.lfCharSet = CharSet;
  logfont.lfOutPrecision = OutputPrecision;
  logfont.lfClipPrecision = ClipPrecision;
  logfont.lfQuality = Quality;
  logfont.lfPitchAndFamily = PitchAndFamily;
   
  if(Face)
    memcpy(logfont.lfFaceName, Face, sizeof(logfont.lfFaceName));
  else 
    logfont.lfFaceName[0] = '\0';

  return W32kCreateFontIndirect(&logfont);
}

HFONT
STDCALL
W32kCreateFontIndirect(CONST LPLOGFONT  lf)
{
  HFONT hFont = 0;
  PTEXTOBJ fontPtr;

  if (lf)
  {
    if(fontPtr = TEXTOBJ_AllocText())
    {
      memcpy(&fontPtr->logfont, lf, sizeof(LOGFONT));

      if (lf->lfEscapement != lf->lfOrientation) {
        /* this should really depend on whether GM_ADVANCED is set */
        fontPtr->logfont.lfOrientation = fontPtr->logfont.lfEscapement;
      }
      hFont = TEXTOBJ_PtrToHandle(fontPtr);
      TEXTOBJ_UnlockText(hFont);
    }
  }

  return hFont;
}

BOOL
STDCALL
W32kCreateScalableFontResource(DWORD  Hidden,
                                     LPCWSTR  FontRes,
                                     LPCWSTR  FontFile,
                                     LPCWSTR  CurrentPath)
{
  UNIMPLEMENTED;
}

int
STDCALL
W32kEnumFontFamilies(HDC  hDC,
                          LPCWSTR  Family,
                          FONTENUMPROC  EnumFontFamProc,
                          LPARAM  lParam)
{
  UNIMPLEMENTED;
}

int
STDCALL
W32kEnumFontFamiliesEx(HDC  hDC,
                            LPLOGFONT  Logfont,
                            FONTENUMPROC  EnumFontFamExProc,
                            LPARAM  lParam,
                            DWORD  Flags)
{
  UNIMPLEMENTED;
}

int
STDCALL
W32kEnumFonts(HDC  hDC,
                   LPCWSTR FaceName,
                   FONTENUMPROC  FontFunc,
                   LPARAM  lParam)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kExtTextOut(HDC  hDC,
                     int  X,
                     int  Y,
                     UINT  Options,
                     CONST LPRECT  rc,
                     LPCWSTR  String,
                     UINT  Count,
                     CONST LPINT  Dx)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kGetAspectRatioFilterEx(HDC  hDC,
                                 LPSIZE  AspectRatio)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kGetCharABCWidths(HDC  hDC,
                           UINT  FirstChar,
                           UINT  LastChar,
                           LPABC  abc)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kGetCharABCWidthsFloat(HDC  hDC,
                                UINT  FirstChar,
                                UINT  LastChar,
                                LPABCFLOAT  abcF)
{
  UNIMPLEMENTED;
}

DWORD
STDCALL
W32kGetCharacterPlacement(HDC  hDC,
                                 LPCWSTR  String,
                                 int  Count,
                                 int  MaxExtent,
                                 LPGCP_RESULTS  Results,
                                 DWORD  Flags)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kGetCharWidth(HDC  hDC,
                       UINT  FirstChar,
                       UINT  LastChar,
                       LPINT  Buffer)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kGetCharWidth32(HDC  hDC,
                         UINT  FirstChar,
                         UINT  LastChar,
                         LPINT  Buffer)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kGetCharWidthFloat(HDC  hDC,
                            UINT  FirstChar,
                            UINT  LastChar,
                            PFLOAT  Buffer)
{
  UNIMPLEMENTED;
}

DWORD
STDCALL
W32kGetFontLanguageInfo(HDC  hDC)
{
  UNIMPLEMENTED;
}

DWORD
STDCALL
W32kGetGlyphOutline(HDC  hDC,
                           UINT  Char,
                           UINT  Format,
                           LPGLYPHMETRICS  gm,
                           DWORD  Bufsize,
                           LPVOID  Buffer,
                           CONST LPMAT2 mat2)
{
  UNIMPLEMENTED;


}

DWORD
STDCALL
W32kGetKerningPairs(HDC  hDC,
                           DWORD  NumPairs,
                           LPKERNINGPAIR  krnpair)
{
  UNIMPLEMENTED;
}

UINT
STDCALL
W32kGetOutlineTextMetrics(HDC  hDC,
                                UINT  Data,
                                LPOUTLINETEXTMETRIC  otm)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kGetRasterizerCaps(LPRASTERIZER_STATUS  rs,
                            UINT  Size)
{
  UNIMPLEMENTED;
}

UINT
STDCALL
W32kGetTextCharset(HDC  hDC)
{
  UNIMPLEMENTED;
}

UINT
STDCALL
W32kGetTextCharsetInfo(HDC  hDC,
                             LPFONTSIGNATURE  Sig,
                             DWORD  Flags)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kGetTextExtentExPoint(HDC  hDC,
                               LPCWSTR String,
                               int  Count,
                               int  MaxExtent,
                               LPINT  Fit,
                               LPINT  Dx,
                               LPSIZE  Size)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kGetTextExtentPoint(HDC  hDC,
                             LPCWSTR  String,
                             int  Count,
                             LPSIZE  Size)
{
  PDC dc = AccessUserObject(hDC);
  PFONTGDI FontGDI;
  FT_Face face;
  FT_GlyphSlot glyph;
  INT error, pitch, glyph_index, i;
  ULONG TotalWidth = 0, MaxHeight = 0, CurrentChar = 0, SpaceBetweenChars = 5;

  FontGDI = AccessInternalObject(dc->w.hFont);

  for(i=0; i<Count; i++)
  {
    glyph_index = FT_Get_Char_Index(face, *String);
    error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
    if(error) DbgPrint("WARNING: Failed to load and render glyph! [index: %u]\n", glyph_index);
    glyph = face->glyph;

    if (glyph->format == ft_glyph_format_outline)
    {
      error = FT_Render_Glyph(glyph, ft_render_mode_mono);
      if(error) DbgPrint("WARNING: Failed to render glyph!\n");
      pitch = glyph->bitmap.pitch;
    } else {
      pitch = glyph->bitmap.width;
    }

    TotalWidth += pitch-1;
    if((glyph->bitmap.rows-1) > MaxHeight) MaxHeight = glyph->bitmap.rows-1;

    CurrentChar++;

    if(CurrentChar < Size) TotalWidth += SpaceBetweenChars;
    String++;
  }

  Size->cx = TotalWidth;
  Size->cy = MaxHeight;
}

BOOL
STDCALL
W32kGetTextExtentPoint32(HDC  hDC,
                               LPCWSTR  String,
                               int  Count,
                               LPSIZE  Size)
{
  UNIMPLEMENTED;
}

int
STDCALL
W32kGetTextFace(HDC  hDC,
                     int  Count,
                     LPWSTR  FaceName)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kGetTextMetrics(HDC  hDC,
                         LPTEXTMETRIC  tm)
{
  PDC dc = AccessUserObject(hDC);
  PFONTGDI FontGDI;

  FontGDI = AccessInternalObject(dc->w.hFont);
  memcpy(tm, &FontGDI->TextMetric, sizeof(TEXTMETRIC));

  return TRUE;
}

BOOL
STDCALL
W32kPolyTextOut(HDC  hDC,
                      CONST LPPOLYTEXT  txt,
                      int  Count)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kRemoveFontResource(LPCWSTR  FileName)
{
  UNIMPLEMENTED;
}

DWORD
STDCALL
W32kSetMapperFlags(HDC  hDC,
                          DWORD  Flag)
{
  UNIMPLEMENTED;
}

UINT
STDCALL
W32kSetTextAlign(HDC  hDC,
                       UINT  Mode)
{
  UINT prevAlign;
  DC *dc;
  
  dc = DC_HandleToPtr(hDC);
  if (!dc) 
    {
      return  0;
    }
  prevAlign = dc->w.textAlign;
  dc->w.textAlign = Mode;
  DC_UnlockDC (hDC);
  
  return  prevAlign;
}

COLORREF
STDCALL
W32kSetTextColor(HDC hDC, 
                 COLORREF color)
{
  COLORREF  oldColor;
  PDC  dc = DC_HandleToPtr(hDC);
  
  if (!dc) 
  {
    return 0x80000000;
  }

  oldColor = dc->w.textColor;
  dc->w.textColor = color;

  DC_UnlockDC(hDC);

  return  oldColor;
}

BOOL
STDCALL
W32kSetTextJustification(HDC  hDC,
                               int  BreakExtra,
                               int  BreakCount)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kTextOut(HDC  hDC,
                  int  XStart,
                  int  YStart,
                  LPCWSTR  String,
                  int  Count)
{
  // Fixme: Call EngTextOut, which does the real work (calling DrvTextOut where appropriate)

  DC *dc = DC_HandleToPtr(hDC);
  SURFOBJ *SurfObj = AccessUserObject(dc->Surface);
  int error, glyph_index, n, load_flags = FT_LOAD_RENDER, i, j, sx, sy, scc;
  FT_Face face;
  FT_GlyphSlot glyph;
  ULONG TextLeft = XStart, TextTop = YStart, SpaceBetweenChars = 2, pitch, previous;
  FT_Bool use_kerning;
  RECTL DestRect, MaskRect;
  POINTL SourcePoint, BrushOrigin;
  HBRUSH hBrush;
  PBRUSHOBJ Brush;
  HBITMAP HSourceGlyph;
  PSURFOBJ SourceGlyphSurf;
  SIZEL bitSize;
  FT_CharMap found = 0, charmap;
  INT yoff;
  HFONT hFont = 0;
  PFONTOBJ FontObj;
  PFONTGDI FontGDI;
  PTEXTOBJ TextObj;
  PPALGDI PalDestGDI;
  PXLATEOBJ XlateObj;

  TextObj = TEXTOBJ_HandleToPtr(dc->w.hFont);

  for(i=0; i<FontsLoaded; i++)
  {
    if(wcscmp(FontTable[i].FaceName, (LPSTR)TextObj->logfont.lfFaceName) == 0)
     hFont = FontTable[i].hFont;
  }

  if(hFont == 0)
  {
    DbgPrint("Specified font %s is not loaded\n", TextObj->logfont.lfFaceName);
    return FALSE;
  }

  FontObj = AccessUserObject(hFont);
  FontGDI = AccessInternalObject(hFont);
  face = FontGDI->face;

  if (face->charmap == NULL)
  {
    DbgPrint("WARNING: No charmap selected!\n");
    DbgPrint("This font face has %d charmaps\n", face->num_charmaps);

    for (n = 0; n < face->num_charmaps; n++)
    {
      charmap = face->charmaps[n];
      DbgPrint("found charmap encoding: %u\n", charmap->encoding);
      if (charmap->encoding != 0)
      {
        found = charmap;
        break;
      }
    }
    if (!found) DbgPrint("WARNING: Could not find desired charmap!\n");
    error = FT_Set_Charmap(face, found);
    if (error) DbgPrint("WARNING: Could not set the charmap!\n");
  }

  error = FT_Set_Pixel_Sizes(face, TextObj->logfont.lfHeight, TextObj->logfont.lfWidth);
  if(error) {
    DbgPrint("Error in setting pixel sizes: %u\n", error);
    return FALSE;
  }

  // Create the brush
  PalDestGDI = AccessInternalObject(dc->w.hPalette);
  XlateObj = EngCreateXlate(PalDestGDI->Mode, PAL_RGB, dc->w.hPalette, NULL);
  hBrush = W32kCreateSolidBrush(XLATEOBJ_iXlate(XlateObj, dc->w.textColor));
  Brush = BRUSHOBJ_HandleToPtr(hBrush);
  EngDeleteXlate(XlateObj);

  SourcePoint.x = 0;
  SourcePoint.y = 0;
  MaskRect.left = 0;
  MaskRect.top = 0;
  BrushOrigin.x = 0;
  BrushOrigin.y = 0;

  // Do we use the current TEXTOBJ's logfont.lfOrientation or the DC's textAlign?
  if (dc->w.textAlign & TA_BASELINE) {
    yoff = 0;
  } else if (dc->w.textAlign & TA_BOTTOM) {
    yoff = -face->size->metrics.descender / 64;
  } else { // TA_TOP
    yoff = face->size->metrics.ascender / 64;
  }

  use_kerning = FT_HAS_KERNING(face);
  previous = 0;

  for(i=0; i<Count; i++)
  {
    glyph_index = FT_Get_Char_Index(face, *String);
    error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
    if(error) {
      DbgPrint("WARNING: Failed to load and render glyph! [index: %u]\n", glyph_index);
      return FALSE;
    }
    glyph = face->glyph;

    // retrieve kerning distance and move pen position
    if (use_kerning && previous && glyph_index)
    {
      FT_Vector delta;
      FT_Get_Kerning(face, previous, glyph_index, 0, &delta);
      TextLeft += delta.x >> 6;
    }

    if (glyph->format == ft_glyph_format_outline)
    {
      error = FT_Render_Glyph(glyph, ft_render_mode_mono);
      if(error) {
        DbgPrint("WARNING: Failed to render glyph!\n");
        return FALSE;
      }
      pitch = glyph->bitmap.pitch;
    } else {
      pitch = glyph->bitmap.width;
    }

    DestRect.left = TextLeft;
    DestRect.top = TextTop + yoff - glyph->bitmap_top;
    DestRect.right = TextLeft + glyph->bitmap.width;
    DestRect.bottom = DestRect.top + glyph->bitmap.rows;
    bitSize.cx = pitch-1;
    bitSize.cy = glyph->bitmap.rows-1;
    MaskRect.right = glyph->bitmap.width;
    MaskRect.bottom = glyph->bitmap.rows;

    // We should create the bitmap out of the loop at the biggest possible glyph size
    // Then use memset with 0 to clear it and sourcerect to limit the work of the transbitblt

    HSourceGlyph = EngCreateBitmap(bitSize, pitch, BMF_1BPP, 0, glyph->bitmap.buffer);
    SourceGlyphSurf = AccessUserObject(HSourceGlyph);

    // Use the font data as a mask to paint onto the DCs surface using a brush
    EngBitBlt(SurfObj, NULL, SourceGlyphSurf, NULL, NULL, &DestRect, &SourcePoint, &MaskRect, Brush, &BrushOrigin, 0xAACC);

    EngDeleteSurface(HSourceGlyph);

    TextLeft += glyph->advance.x >> 6;
    previous = glyph_index;

    String++;
  }
}

UINT
STDCALL
W32kTranslateCharsetInfo(PDWORD  Src,
                               LPCHARSETINFO  CSI,   
                               DWORD  Flags)
{
  UNIMPLEMENTED;
}
