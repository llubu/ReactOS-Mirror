/* $Id: bootvid.c,v 1.1 2003/08/11 18:50:12 chorns Exp $
 *
 * COPYRIGHT:      See COPYING in the top level directory
 * PROJECT:        ReactOS kernel
 * FILE:           ntoskrnl/inbv/bootvid.c
 * PURPOSE:        Boot video support
 * PROGRAMMER:     Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *  12-07-2003 CSH Created
 */

/* INCLUDES ******************************************************************/

#include <roskrnl.h>
#include <reactos/resource.h>
#include <internal/i386/mm.h>
#include <internal/v86m.h>

#define NDEBUG
#include <internal/debug.h>

#define RT_BITMAP   2

typedef struct tagRGBQUAD {
  unsigned char    rgbBlue;
  unsigned char    rgbGreen;
  unsigned char    rgbRed;
  unsigned char    rgbReserved;
} RGBQUAD, *PRGBQUAD;

typedef long FXPT2DOT30;

typedef struct tagCIEXYZ {
  FXPT2DOT30 ciexyzX; 
  FXPT2DOT30 ciexyzY; 
  FXPT2DOT30 ciexyzZ; 
} CIEXYZ;
typedef CIEXYZ * LPCIEXYZ; 

typedef struct tagCIEXYZTRIPLE {
  CIEXYZ  ciexyzRed; 
  CIEXYZ  ciexyzGreen; 
  CIEXYZ  ciexyzBlue; 
} CIEXYZTRIPLE;
typedef CIEXYZTRIPLE *LPCIEXYZTRIPLE;

typedef struct { 
  DWORD        bV5Size; 
  LONG         bV5Width; 
  LONG         bV5Height; 
  WORD         bV5Planes; 
  WORD         bV5BitCount; 
  DWORD        bV5Compression; 
  DWORD        bV5SizeImage; 
  LONG         bV5XPelsPerMeter; 
  LONG         bV5YPelsPerMeter; 
  DWORD        bV5ClrUsed; 
  DWORD        bV5ClrImportant; 
  DWORD        bV5RedMask; 
  DWORD        bV5GreenMask; 
  DWORD        bV5BlueMask; 
  DWORD        bV5AlphaMask; 
  DWORD        bV5CSType; 
  CIEXYZTRIPLE bV5Endpoints; 
  DWORD        bV5GammaRed; 
  DWORD        bV5GammaGreen; 
  DWORD        bV5GammaBlue; 
  DWORD        bV5Intent; 
  DWORD        bV5ProfileData; 
  DWORD        bV5ProfileSize; 
  DWORD        bV5Reserved; 
} BITMAPV5HEADER, *PBITMAPV5HEADER; 


#define MISC     0x3c2
#define SEQ      0x3c4
#define CRTC     0x3d4
#define GRAPHICS 0x3ce
#define FEATURE  0x3da
#define ATTRIB   0x3c0
#define STATUS   0x3da

typedef struct _VideoMode {
  unsigned short VidSeg;
  unsigned char  Misc;
  unsigned char  Feature;
  unsigned short Seq[6];
  unsigned short Crtc[25];
  unsigned short Gfx[9];
  unsigned char  Attrib[21];
} VideoMode;

typedef struct {
  ULONG r;
  ULONG g;
  ULONG b;
} FADER_PALETTE_ENTRY;

/* In pixelsups.S */
extern VOID
InbvPutPixels(int x, int y, unsigned long c);

/* GLOBALS *******************************************************************/

char *vidmem;

/* Must be 4 bytes per entry */
long maskbit[640];
long y80[480];

static HANDLE BitmapThreadHandle;
static CLIENT_ID BitmapThreadId;
static BOOLEAN BitmapIsDrawn;
static BOOLEAN BitmapThreadShouldTerminate;
static PUCHAR BootimageBitmap;
static BOOLEAN InGraphicsMode = FALSE;

/* DATA **********************************************************************/

static VideoMode Mode12 = {
    0xa000, 0xe3, 0x00,

    {0x03, 0x01, 0x0f, 0x00, 0x06 },

    {0x5f, 0x4f, 0x50, 0x82, 0x54, 0x80, 0x0b, 0x3e, 0x00, 0x40, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x59, 0xea, 0x8c, 0xdf, 0x28, 0x00, 0xe7, 0x04, 0xe3,
     0xff},

    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x0f, 0xff},

    {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
     0x0c, 0x0d, 0x0e, 0x0f, 0x81, 0x00, 0x0f, 0x00, 0x00}
};

static BOOLEAN VideoAddressSpaceInitialized = FALSE;
static PVOID NonBiosBaseAddress;

/* FUNCTIONS *****************************************************************/

static BOOLEAN
InbvFindBootimage()
{
  PIMAGE_RESOURCE_DATA_ENTRY ResourceDataEntry;
  LDR_RESOURCE_INFO ResourceInfo;
  NTSTATUS Status;
  PVOID BaseAddress = (PVOID)KERNEL_BASE;
  ULONG Size;

  ResourceInfo.Type = RT_BITMAP;
  ResourceInfo.Name = IDB_BOOTIMAGE;
  ResourceInfo.Language = 0x09;

  Status = LdrFindResource_U(BaseAddress,
    &ResourceInfo,
    RESOURCE_DATA_LEVEL,
    &ResourceDataEntry);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("LdrFindResource_U() failed with status 0x%.08x\n", Status);
      return FALSE;
    }

  Status = LdrAccessResource(BaseAddress,
    ResourceDataEntry,
    (PVOID*)&BootimageBitmap,
    &Size);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("LdrAccessResource() failed with status 0x%.08x\n", Status);
      return FALSE;
    }

  return TRUE;
}


static BOOLEAN
InbvInitializeVideoAddressSpace(VOID)
{
   OBJECT_ATTRIBUTES ObjectAttributes;
   UNICODE_STRING PhysMemName;
   NTSTATUS Status;
   HANDLE PhysMemHandle;
   PVOID BaseAddress;
   LARGE_INTEGER Offset;
   ULONG ViewSize;
   CHAR IVT[1024];
   CHAR BDA[256];
   PVOID start = (PVOID)0x0;

   /*
    * Open the physical memory section
    */
   RtlInitUnicodeStringFromLiteral(&PhysMemName, L"\\Device\\PhysicalMemory");
   InitializeObjectAttributes(&ObjectAttributes,
			      &PhysMemName,
			      0,
			      NULL,
			      NULL);
   Status = NtOpenSection(&PhysMemHandle, SECTION_ALL_ACCESS, 
			  &ObjectAttributes);
   if (!NT_SUCCESS(Status))
     {
	DPRINT("Couldn't open \\Device\\PhysicalMemory\n");
	return FALSE;
     }

   /*
    * Map the BIOS and device registers into the address space
    */
   Offset.QuadPart = 0xa0000;
   ViewSize = 0x100000 - 0xa0000;
   BaseAddress = (PVOID)0xa0000;
   Status = NtMapViewOfSection(PhysMemHandle,
			       NtCurrentProcess(),
			       &BaseAddress,
			       0,
			       8192,
			       &Offset,
			       &ViewSize,
			       ViewUnmap,
			       0,
			       PAGE_EXECUTE_READWRITE);
   if (!NT_SUCCESS(Status))
     {
	DPRINT("Couldn't map physical memory (%x)\n", Status);
	NtClose(PhysMemHandle);
	return FALSE;
     }
   NtClose(PhysMemHandle);
   if (BaseAddress != (PVOID)0xa0000)
     {
       DPRINT("Couldn't map physical memory at the right address "
		"(was %x)\n", BaseAddress);
       return FALSE;
     }

   /*
    * Map some memory to use for the non-BIOS parts of the v86 mode address
    * space
    */
   NonBiosBaseAddress = (PVOID)0x1;
   ViewSize = 0xa0000 - 0x1000;
   Status = NtAllocateVirtualMemory(NtCurrentProcess(),
				    &NonBiosBaseAddress,
				    0,
				    &ViewSize,
				    MEM_COMMIT,
				    PAGE_EXECUTE_READWRITE);
   if (!NT_SUCCESS(Status))
     {
       DPRINT("Failed to allocate virtual memory (Status %x)\n", Status);
       return FALSE;
     }
   if (NonBiosBaseAddress != (PVOID)0x0)
     {
       DPRINT("Failed to allocate virtual memory at right address "
		"(was %x)\n", NonBiosBaseAddress);
       return FALSE;
     }

   /*
    * Get the real mode IVT from the kernel
    */
   Status = NtVdmControl(0, IVT);
   if (!NT_SUCCESS(Status))
     {
       DPRINT("NtVdmControl failed (status %x)\n", Status);
       return FALSE;
     }
   
   /*
    * Copy the real mode IVT into the right place
    */
   memcpy(start, IVT, 1024);
   
   /*
    * Get the BDA from the kernel
    */
   Status = NtVdmControl(1, BDA);
   if (!NT_SUCCESS(Status))
     {
       DPRINT("NtVdmControl failed (status %x)\n", Status);
       return FALSE;
     }
   
   /*
    * Copy the BDA into the right place
    */
   memcpy((PVOID)0x400, BDA, 256);

   return TRUE;
}


static BOOLEAN
InbvDeinitializeVideoAddressSpace(VOID)
{
  ULONG RegionSize;
  PUCHAR ViewBase;

  RegionSize = 0xa0000 - 0x1000;
  NtFreeVirtualMemory(NtCurrentProcess(),
    &NonBiosBaseAddress,
    &RegionSize,
    MEM_RELEASE);

  ViewBase = (PUCHAR) 0xa0000;
  NtUnmapViewOfSection(NtCurrentProcess(), ViewBase);

 return TRUE;
}


static VOID
vgaPreCalc()
{
  ULONG j;

  for(j = 0; j < 80; j++)
  {
    maskbit[j * 8 + 0] = 128;
    maskbit[j * 8 + 1] = 64;
    maskbit[j * 8 + 2] = 32;
    maskbit[j * 8 + 3] = 16;
    maskbit[j * 8 + 4] = 8;
    maskbit[j * 8 + 5] = 4;
    maskbit[j * 8 + 6] = 2;
    maskbit[j * 8 + 7] = 1;
  }
  for(j = 0; j < 480; j++)
  {
    y80[j] = j * 80; /* 80 = 640 / 8 = Number of bytes per scanline */
  }
}

static __inline__ VOID
InbvOutxay(PUSHORT ad, UCHAR x, UCHAR y)
{
  USHORT xy = (x << 8) + y;
  WRITE_PORT_USHORT(ad, xy);
}


static VOID
InbvSetMode(VideoMode mode)
{
  unsigned char x;

  WRITE_PORT_UCHAR((PUCHAR)MISC, mode.Misc);
  WRITE_PORT_UCHAR((PUCHAR)STATUS, 0);
  WRITE_PORT_UCHAR((PUCHAR)FEATURE, mode.Feature);

  for(x=0; x<5; x++)
  {
    InbvOutxay((PUSHORT)SEQ, mode.Seq[x], x);
  }

  WRITE_PORT_USHORT((PUSHORT)CRTC, 0x11);
  WRITE_PORT_USHORT((PUSHORT)CRTC, (mode.Crtc[0x11] & 0x7f));

  for(x=0; x<25; x++)
  {
    InbvOutxay((PUSHORT)CRTC, mode.Crtc[x], x);
  }

  for(x=0; x<9; x++)
  {
    InbvOutxay((PUSHORT)GRAPHICS, mode.Gfx[x], x);
  }

  x=READ_PORT_UCHAR((PUCHAR)FEATURE);

  for(x=0; x<21; x++)
  {
    WRITE_PORT_UCHAR((PUCHAR)ATTRIB, x);
    WRITE_PORT_UCHAR((PUCHAR)ATTRIB, mode.Attrib[x]);
  }

  x=READ_PORT_UCHAR((PUCHAR)STATUS);

  WRITE_PORT_UCHAR((PUCHAR)ATTRIB, 0x20);
}


static VOID
InbvInitVGAMode(VOID)
{
  KV86M_REGISTERS Regs;
  NTSTATUS Status;

  vidmem = (char *)(0xd0000000 + 0xa0000);
  memset(&Regs, 0, sizeof(Regs));
  Regs.Eax = 0x0012;

  Status = Ke386CallBios(0x10, &Regs);
  assert(NT_SUCCESS(Status));

  /* Get VGA registers into the correct state (mainly for setting up the palette registers correctly) */
  InbvSetMode(Mode12);

  /* Get the VGA into the mode we want to work with */
  WRITE_PORT_UCHAR((PUCHAR)0x3ce,0x08);     /* Set */
  WRITE_PORT_UCHAR((PUCHAR)0x3cf,0);        /* the MASK */
  WRITE_PORT_USHORT((PUSHORT)0x3ce,0x0205); /* write mode = 2 (bits 0,1) read mode = 0  (bit 3) */
  (UCHAR) READ_REGISTER_UCHAR(vidmem);      /* Update bit buffer */
  WRITE_REGISTER_UCHAR(vidmem, 0);          /* Write the pixel */
  WRITE_PORT_UCHAR((PUCHAR)0x3ce,0x08);
  WRITE_PORT_UCHAR((PUCHAR)0x3cf,0xff);

  /* Zero out video memory (clear a possibly trashed screen) */
  RtlZeroMemory(vidmem, 64000);

  vgaPreCalc();
}


BOOLEAN
STDCALL
VidResetDisplay(VOID)
{
  KV86M_REGISTERS Regs;
  NTSTATUS Status;

#if 0
  /* FIXME: What if the system has crashed, eg. this function is called from KeBugCheck() ? */

  /* Maybe wait until boot screen bitmap is drawn */
  while (!BitmapIsDrawn)
    {
      NtYieldExecution();
    }
#endif

  /* Zero out video memory (clear the screen) */
  RtlZeroMemory(vidmem, 64000);

  memset(&Regs, 0, sizeof(Regs));
  Regs.Eax = 0x0003;
  Status = Ke386CallBios(0x10, &Regs);
  assert(NT_SUCCESS(Status));

  memset(&Regs, 0, sizeof(Regs));
  Regs.Eax = 0x1112;
  Status = Ke386CallBios(0x10, &Regs);
  assert(NT_SUCCESS(Status));

  memset(&Regs, 0, sizeof(Regs));
  Regs.Eax = 0x0632;    // AH = 0x06 - Scroll active page up, AL = 0x32 - Clear 25 lines
  Regs.Ecx = 0x0000;    // CX = 0x0000 - Upper left of scroll
  Regs.Edx = 0x314F;    // DX = 0x314F - Lower right of scroll
  Regs.Ebx = 0x1100;    // Use normal attribute on blanked line
  Status = Ke386CallBios(0x10, &Regs);
  assert(NT_SUCCESS(Status));

  InGraphicsMode = FALSE;

  return TRUE;
}


VOID
STDCALL
VidCleanUp(VOID)
{
  if (InGraphicsMode)
    {
      VidResetDisplay();
    }

  if (VideoAddressSpaceInitialized)
    {
      InbvDeinitializeVideoAddressSpace();
      VideoAddressSpaceInitialized = FALSE;
    }

  BitmapThreadShouldTerminate = TRUE;
}


static __inline__ VOID
InbvSetColor(int cindex, unsigned char red, unsigned char green, unsigned char blue)
{
  red = red / (256 / 64);
  green = green / (256 / 64);
  blue = blue / (256 / 64);

  WRITE_PORT_UCHAR((PUCHAR)0x03c8, cindex);
  WRITE_PORT_UCHAR((PUCHAR)0x03c9, red);
  WRITE_PORT_UCHAR((PUCHAR)0x03c9, green);
  WRITE_PORT_UCHAR((PUCHAR)0x03c9, blue);
}


static __inline__ VOID
InbvSetBlackPalette()
{
  register ULONG r = 0;

  for (r = 0; r < 16; r++)
    {
      InbvSetColor(r, 0, 0, 0);
    }
}


static VOID
InbvDisplayBitmap(ULONG Width, ULONG Height, PCHAR ImageData)
{
  ULONG j,k,y;
  register ULONG i;
  register ULONG x;
  register ULONG c;

  k = 0;
  for (y = 0; y < Height; y++)
    {
      for (j = 0; j < 8; j++)
        {
          x = j;

          /*
           * Loop through the line and process every 8th pixel.
           * This way we can get a way with using the same bit mask
           * for several pixels and thus not need to do as much I/O
           * communication.
           */
          while (x < 640)
            {
              c = 0;

              if (x < Width)
                {
                  c = ImageData[k + x];
                  for (i = 1; i < 4; i++)
                    {
                      if (x + i*8 < Width)
                        {
                          c |= (ImageData[k + x + i * 8] << i * 8);
                        }
                    }
                }

                InbvPutPixels(x, 479 - y, c);
              x += 8*4;
            }
        }
      k += Width;
    }
}


static VOID
InbvDisplayCompressedBitmap()
{
  PBITMAPV5HEADER bminfo;
  ULONG i,j,k;
  ULONG x,y;
  ULONG curx,cury;
  ULONG bfOffBits;
  ULONG clen;
  PCHAR ImageData;

  bminfo = (PBITMAPV5HEADER) &BootimageBitmap[0];
  DPRINT("bV5Size = %d\n", bminfo->bV5Size);
  DPRINT("bV5Width = %d\n", bminfo->bV5Width);
  DPRINT("bV5Height = %d\n", bminfo->bV5Height);
  DPRINT("bV5Planes = %d\n", bminfo->bV5Planes);
  DPRINT("bV5BitCount = %d\n", bminfo->bV5BitCount);
  DPRINT("bV5Compression = %d\n", bminfo->bV5Compression);
  DPRINT("bV5SizeImage = %d\n", bminfo->bV5SizeImage);
  DPRINT("bV5XPelsPerMeter = %d\n", bminfo->bV5XPelsPerMeter);
  DPRINT("bV5YPelsPerMeter = %d\n", bminfo->bV5YPelsPerMeter);
  DPRINT("bV5ClrUsed = %d\n", bminfo->bV5ClrUsed);
  DPRINT("bV5ClrImportant = %d\n", bminfo->bV5ClrImportant);

  bfOffBits = bminfo->bV5Size + bminfo->bV5ClrUsed * sizeof(RGBQUAD);
  DPRINT("bfOffBits = %d\n", bfOffBits);
  DPRINT("size of color indices = %d\n", bminfo->bV5ClrUsed * sizeof(RGBQUAD));
  DPRINT("first byte of data = %d\n", BootimageBitmap[bfOffBits]);

  InbvSetBlackPalette();

  ImageData = ExAllocatePool(NonPagedPool, bminfo->bV5Width * bminfo->bV5Height);
  RtlZeroMemory(ImageData, bminfo->bV5Width * bminfo->bV5Height);

  /*
   * ImageData has 1 pixel per byte.
   * bootimage has 2 pixels per byte.
   */

  if (bminfo->bV5Compression == 2)
    {
      k = 0;
      j = 0;
      while ((j < bminfo->bV5SizeImage) && (k < (ULONG) (bminfo->bV5Width * bminfo->bV5Height)))
        {
          unsigned char b;
    
          clen = BootimageBitmap[bfOffBits + j];
          j++;
    
          if (clen > 0)
            {
              /* Encoded mode */
    
              b = BootimageBitmap[bfOffBits + j];
              j++;
    
              for (i = 0; i < (clen / 2); i++)
                {
                  ImageData[k] = (b & 0xf0) >> 4;
                  k++;
                  ImageData[k] = b & 0xf;
                  k++;
                }
              if ((clen & 1) > 0)
              {
                ImageData[k] = (b & 0xf0) >> 4;
                k++;
              }
            }
          else
            {
              /* Absolute mode */
              b = BootimageBitmap[bfOffBits + j];
              j++;
    
              if (b == 0)
                {
                  /* End of line */
                }
              else if (b == 1)
                {
                  /* End of image */
                  break;
                }
              else if (b == 2)
                {
                  x = BootimageBitmap[bfOffBits + j];
                  j++;
                  y = BootimageBitmap[bfOffBits + j];
                  j++;
                  curx = k % bminfo->bV5Width;
                  cury = k / bminfo->bV5Width;
                  k = (cury + y) * bminfo->bV5Width + (curx + x);
                }
              else
                {
                  if ((j & 1) > 0)
                    {
                      DPRINT("Unaligned copy!\n");
                    }
    
                  clen = b;
                  for (i = 0; i < (clen / 2); i++)
                    {
                      b = BootimageBitmap[bfOffBits + j];
                      j++;
        
                      ImageData[k] = (b & 0xf0) >> 4;
                      k++;
                      ImageData[k] = b & 0xf;
                      k++;
                    }
                  if ((clen & 1) > 0)
                  {
                    b = BootimageBitmap[bfOffBits + j];
                    j++;
                    ImageData[k] = (b & 0xf0) >> 4;
                    k++;
                  }
                  /* Word align */
                  j += (j & 1);
                }
            }
        }

      InbvDisplayBitmap(bminfo->bV5Width, bminfo->bV5Height, ImageData);
    }
  else
    {
      DbgPrint("Warning boot image need to be compressed using RLE4\n");
    }

  ExFreePool(ImageData);
}


#define PALETTE_FADE_STEPS  20
#define PALETTE_FADE_TIME   20 * 10000 /* 20ms */

static VOID
InbvFadeUpPalette()
{
  PBITMAPV5HEADER bminfo;
  PRGBQUAD Palette;
  ULONG i;
  unsigned char r,g,b;
  register ULONG c;
  LARGE_INTEGER	Interval;
  FADER_PALETTE_ENTRY FaderPalette[16];
  FADER_PALETTE_ENTRY FaderPaletteDelta[16];

  RtlZeroMemory(&FaderPalette, sizeof(FaderPalette));
  RtlZeroMemory(&FaderPaletteDelta, sizeof(FaderPaletteDelta));

  bminfo = (PBITMAPV5HEADER) &BootimageBitmap[0]; //sizeof(BITMAPFILEHEADER)];
  Palette = (PRGBQUAD) &BootimageBitmap[/* sizeof(BITMAPFILEHEADER) + */ bminfo->bV5Size];

  for (i = 0; i < 16; i++)
    {
      if (i < bminfo->bV5ClrUsed)
        {
          FaderPaletteDelta[i].r = ((Palette[i].rgbRed << 8) / PALETTE_FADE_STEPS);
          FaderPaletteDelta[i].g = ((Palette[i].rgbGreen << 8) / PALETTE_FADE_STEPS);
          FaderPaletteDelta[i].b = ((Palette[i].rgbBlue << 8) / PALETTE_FADE_STEPS);
        }
    }

  for (i = 0; i < PALETTE_FADE_STEPS; i++)
    {
      for (c = 0; c < bminfo->bV5ClrUsed; c++)
        {
          /* Add the delta */
          FaderPalette[c].r += FaderPaletteDelta[c].r;
          FaderPalette[c].g += FaderPaletteDelta[c].g;
          FaderPalette[c].b += FaderPaletteDelta[c].b;

          /* Get the integer values */
          r = FaderPalette[c].r >> 8;
          g = FaderPalette[c].g >> 8;
          b = FaderPalette[c].b >> 8;

          /* Don't go too far */
          if (r > Palette[c].rgbRed)
            r = Palette[c].rgbRed;
          if (g > Palette[c].rgbGreen)
            g = Palette[c].rgbGreen;
          if (b > Palette[c].rgbBlue)
            b = Palette[c].rgbBlue;

          /* Update the hardware */
          InbvSetColor(c, r, g, b);
        }
      Interval.QuadPart = -PALETTE_FADE_TIME;
      KeDelayExecutionThread(KernelMode, FALSE, &Interval);
    }
}

static VOID STDCALL
InbvBitmapThreadMain(PVOID Ignored)
{
  if (InbvFindBootimage())
    {
      InbvDisplayCompressedBitmap();
      InbvFadeUpPalette();
    }
  else
    {
      DbgPrint("Warning: Cannot find boot image\n");
    }

  BitmapIsDrawn = TRUE;
  for(;;)
    {
      if (BitmapThreadShouldTerminate)
        {
          DPRINT("Terminating\n");
          return;
        }
      NtYieldExecution();
    }
}


BOOLEAN
STDCALL
VidIsBootDriverInstalled(VOID)
{
  return InGraphicsMode;
}


BOOLEAN
STDCALL
VidInitialize(VOID)
{
  NTSTATUS Status;

  if (!VideoAddressSpaceInitialized)
    {
      InbvInitializeVideoAddressSpace();
    }

  InbvInitVGAMode();

  InGraphicsMode = TRUE;

  BitmapIsDrawn = FALSE;
  BitmapThreadShouldTerminate = FALSE;
  
  Status = PsCreateSystemThread(&BitmapThreadHandle,
    THREAD_ALL_ACCESS,
    NULL,
    NULL,
    &BitmapThreadId,
    InbvBitmapThreadMain,
    NULL);
  if (!NT_SUCCESS(Status))
    {
      return FALSE;
    }

  return TRUE;
}
