#include <ddk/ntddk.h>
#include <ddk/ntddvid.h>
#include <ddk/winddi.h>
#include <ntos/minmax.h>
#include "vgavideo.h"


INT abs(INT nm)
{
  if(nm<0)
  {
    return nm * -1;
  } else
  {
    return nm;
  }
}

div_t div(int num, int denom)
{
  div_t r;
  if (num > 0 && denom < 0) {
    num = -num;
    denom = -denom;
  }
  r.quot = num / denom;
  r.rem = num % denom;
  if (num < 0 && denom > 0)
  {
    if (r.rem > 0)
    {
      r.quot++;
      r.rem -= denom;
    }
  }
  return r;
}

int mod(int num, int denom)
{
  div_t dvt = div(num, denom);
  return dvt.rem;
}

BYTE bytesPerPixel(ULONG Format)
{
  // This function is taken from /subsys/win32k/eng/surface.c
  // FIXME: GDI bitmaps are supposed to be pixel-packed. Right now if the
  // pixel size if < 1 byte we expand it to 1 byte for simplicities sake

  if(Format==BMF_1BPP)
  {
    return 1;
  } else
  if((Format==BMF_4BPP) || (Format==BMF_4RLE))
  {
    return 1;
  } else
  if((Format==BMF_8BPP) || (Format==BMF_8RLE))
  {
    return 1;
  } else
  if(Format==BMF_16BPP)
  {
    return 2;
  } else
  if(Format==BMF_24BPP)
  {
    return 3;
  } else
  if(Format==BMF_32BPP)
  {
    return 4;
  }

  return 0;
}

VOID vgaPreCalc()
{
  ULONG j;

  startmasks[1] = 127;
  startmasks[2] = 63;
  startmasks[3] = 31;
  startmasks[4] = 15;
  startmasks[5] = 7;
  startmasks[6] = 3;
  startmasks[7] = 1;
  startmasks[8] = 255;

  endmasks[0] = 128;
  endmasks[1] = 192;
  endmasks[2] = 224;
  endmasks[3] = 240;
  endmasks[4] = 248;
  endmasks[5] = 252;
  endmasks[6] = 254;
  endmasks[7] = 255;
  endmasks[8] = 255;

  for(j=0; j<80; j++)
  {
    maskbit[j*8]   = 128;
    maskbit[j*8+1] = 64;
    maskbit[j*8+2] = 32;
    maskbit[j*8+3] = 16;
    maskbit[j*8+4] = 8;
    maskbit[j*8+5] = 4;
    maskbit[j*8+6] = 2;
    maskbit[j*8+7] = 1;

    bit8[j*8]   = 7;
    bit8[j*8+1] = 6;
    bit8[j*8+2] = 5;
    bit8[j*8+3] = 4;
    bit8[j*8+4] = 3;
    bit8[j*8+5] = 2;
    bit8[j*8+6] = 1;
    bit8[j*8+7] = 0;
  }
  for(j=0; j<480; j++)
  {
    y80[j]  = j*80;
  }
  for(j=0; j<640; j++)
  {
    xconv[j] = j >> 3;
  }
}

void
get_masks(int x, int w)
{
  register int tmp;

  leftMask = rightMask = 0;
  byteCounter = w;
  /* right margin */
  tmp = (x+w) & 7;
  if (tmp) {
    byteCounter -= tmp;
    rightMask = (unsigned char)(0xff00 >> tmp);
  }
  /* left margin */
  tmp = x & 7;
  if (tmp) {
    byteCounter -= (8 - tmp);
    leftMask = (0xff >> tmp);
  }
  /* too small ? */
  if (byteCounter < 0) {
    leftMask &= rightMask;
    rightMask = 0;
    byteCounter = 0;
  }
  byteCounter /= 8;
}

VOID vgaPutPixel(INT x, INT y, UCHAR c)
{
  ULONG offset;
  UCHAR a;

  offset = xconv[x]+y80[y];

  WRITE_PORT_UCHAR((PUCHAR)0x3ce,0x08);
  WRITE_PORT_UCHAR((PUCHAR)0x3cf,maskbit[x]);

  a = READ_REGISTER_UCHAR(vidmem + offset);
  WRITE_REGISTER_UCHAR(vidmem + offset, c);
}

VOID vgaPutByte(INT x, INT y, UCHAR c)
{
  ULONG offset;

  offset = xconv[x]+y80[y];

  // Set the write mode
  WRITE_PORT_UCHAR((PUCHAR)0x3ce,0x08);
  WRITE_PORT_UCHAR((PUCHAR)0x3cf,0xff);

  WRITE_REGISTER_UCHAR(vidmem + offset, c);
}

VOID vgaGetByte(ULONG offset,
                UCHAR *b, UCHAR *g,
                UCHAR *r, UCHAR *i)
{
  WRITE_PORT_USHORT((PUSHORT)0x03ce, 0x0304);
  *i = READ_REGISTER_UCHAR(vidmem + offset);
  WRITE_PORT_USHORT((PUSHORT)0x03ce, 0x0204);
  *r = READ_REGISTER_UCHAR(vidmem + offset);
  WRITE_PORT_USHORT((PUSHORT)0x03ce, 0x0104);
  *g = READ_REGISTER_UCHAR(vidmem + offset);
  WRITE_PORT_USHORT((PUSHORT)0x03ce, 0x0004);
  *b = READ_REGISTER_UCHAR(vidmem + offset);
}

INT vgaGetPixel(INT x, INT y)
{
  UCHAR mask, b, g, r, i;
  ULONG offset;

  offset = xconv[x]+y80[y];
  vgaGetByte(offset, &b, &g, &r, &i);

  mask=maskbit[x];
  b=b&mask;
  g=g&mask;
  r=r&mask;
  i=i&mask;

  mask=bit8[x];
  g=g>>mask;
  b=b>>mask;
  r=r>>mask;
  i=i>>mask;

  return(b+2*g+4*r+8*i);
}

BOOL vgaHLine(INT x, INT y, INT len, UCHAR c)
{
  UCHAR a;
  ULONG pre1, i;
  ULONG orgpre1, orgx, midpre1;
  ULONG long leftpixs, midpixs, rightpixs, temp;

  orgx=x;

  if(len<8)
  {
    for (i=x; i<x+len; i++)
      vgaPutPixel(i, y, c);
  } else {

    leftpixs=x;
    while(leftpixs>8) leftpixs-=8;
    temp = len;
    midpixs = 0;

    while(temp>7)
    {
      temp-=8;
      midpixs++;
    }
    if((temp>=0) && (midpixs>0)) midpixs--;

    pre1=xconv[x]+y80[y];
    orgpre1=pre1;

    // Left
    if(leftpixs==8) {
      // Left edge should be an entire middle bar
      x=orgx;
      leftpixs=0;
    }
    else if(leftpixs>0)
    {
      // Write left pixels
      WRITE_PORT_UCHAR((PUCHAR)0x3ce,0x08);     // set the mask
      WRITE_PORT_UCHAR((PUCHAR)0x3cf,startmasks[leftpixs]);

      a = READ_REGISTER_UCHAR(vidmem + pre1);
      WRITE_REGISTER_UCHAR(vidmem + pre1, c);

      // Middle
      x=orgx+(8-leftpixs)+leftpixs;

    } else {
      // leftpixs == 0
      midpixs+=1;
    }

    if(midpixs>0)
    {
      midpre1=xconv[x]+y80[y];

      // Set mask to all pixels in byte
      WRITE_PORT_UCHAR((PUCHAR)0x3ce, 0x08);
      WRITE_PORT_UCHAR((PUCHAR)0x3cf, 0xff);
      memset(vidmem+midpre1, c, midpixs); // write middle pixels, no need to read in latch because of the width
    }

    rightpixs = len - ((midpixs*8) + leftpixs);

    if((rightpixs>0))
    {
      x=(orgx+len)-rightpixs;

      // Go backwards till we reach the 8-byte boundary
      while(mod(x, 8)!=0) { x--; rightpixs++; }

      while(rightpixs>7)
      {
        // This is a BAD case as this should have been a midpixs

        vgaPutByte(x, y, c);
        rightpixs-=8;
        x+=8;
      }

      pre1=xconv[x]+y80[y];

      // Write right pixels
      WRITE_PORT_UCHAR((PUCHAR)0x3ce,0x08);     // set the mask bits
      WRITE_PORT_UCHAR((PUCHAR)0x3cf,endmasks[rightpixs]);

      a = READ_REGISTER_UCHAR(vidmem + pre1);
      WRITE_REGISTER_UCHAR(vidmem + pre1, c);
    }
  }

  return TRUE;
}

BOOL vgaVLine(INT x, INT y, INT len, UCHAR c)
{
  ULONG offset, i;
  UCHAR a;

  offset = xconv[x]+y80[y];

  WRITE_PORT_UCHAR((PUCHAR)0x3ce,0x08);       // set the mask
  WRITE_PORT_UCHAR((PUCHAR)0x3cf,maskbit[x]);

  len++;

  for(i=y; i<y+len; i++)
  {
    a = READ_REGISTER_UCHAR(vidmem + offset);
    WRITE_REGISTER_UCHAR(vidmem + offset, c);
    offset+=80;
  }

  return TRUE;
}

static const RECTL rclEmpty = { 0, 0, 0, 0 };

BOOL VGADDIIntersectRect(PRECTL prcDst, PRECTL prcSrc1, PRECTL prcSrc2)
{
  prcDst->left  = max(prcSrc1->left, prcSrc2->left);
  prcDst->right = min(prcSrc1->right, prcSrc2->right);

  if (prcDst->left < prcDst->right) {
      prcDst->top = max(prcSrc1->top, prcSrc2->top);
      prcDst->bottom = min(prcSrc1->bottom, prcSrc2->bottom);

     if (prcDst->top < prcDst->bottom)
     {
       return TRUE;
     }
  }

  *prcDst = rclEmpty;

  return FALSE;
}

void DIB_BltFromVGA(int x, int y, int w, int h, void *b, int Dest_lDelta)

//  DIB blt from the VGA.
//  For now we just do slow reads -- pixel by pixel, packing each one into the correct 4BPP format.
{
  PBYTE  pb = b, opb = b;
  BOOLEAN  edgePixel = FALSE;
  ULONG  i, j;
  ULONG  x2 = x + w;
  ULONG  y2 = y + h;
  BYTE  b1, b2;

  // Check if the width is odd
  if(mod(w, 2)>0)
  {
    edgePixel = TRUE;
    x2 -= 1;
  }

  for (j=y; j<y2; j++)
  {
    for (i=x; i<x2; i+=2)
    {
      b1 = vgaGetPixel(i,  j);
      b2 = vgaGetPixel(i+1,  j);
      *pb = b2 | (b1 << 4);
      pb++;
    }

    if(edgePixel == TRUE)
    {
      b1 = vgaGetPixel(x2, j);
      *pb = b1;
      pb++;
    }

    opb += Dest_lDelta; // new test code
    pb = opb; // new test code

  }
}

void DIB_BltToVGA(int x, int y, int w, int h, void *b, int Source_lDelta)

//  DIB blt to the VGA.
//  For now we just do slow writes -- pixel by pixel, packing each one into the correct 4BPP format.
{
  PBYTE  pb = b, opb = b;
  BOOLEAN  edgePixel = FALSE;
  ULONG  i, j;
  ULONG  x2 = x + w;
  ULONG  y2 = y + h;
  BYTE  b1, b2;

  // Check if the width is odd
  if(mod(w, 2)>0)
  {
    edgePixel = TRUE;
    x2 -= 1;
  }

  for (j=y; j<y2; j++)
  {
    for (i=x; i<x2; i+=2)
    {
      b1 = (*pb & 0xf0) >> 4;
      b2 = *pb & 0x0f;
      vgaPutPixel(i,   j, b1);
      vgaPutPixel(i+1, j, b2);
      pb++;
    }

    if(edgePixel == TRUE)
    {
      b1 = *pb;
      vgaPutPixel(x2, j, b1);
      pb++;
    }

    opb += Source_lDelta;
    pb = opb;

  }
}

void DIB_TransparentBltToVGA(int x, int y, int w, int h, void *b, int Source_lDelta, ULONG trans)

//  DIB blt to the VGA.
//  For now we just do slow writes -- pixel by pixel, packing each one into the correct 4BPP format.
{
  PBYTE  pb = b, opb = b;
  BOOLEAN  edgePixel = FALSE;
  ULONG  i, j;
  ULONG  x2 = x + w;
  ULONG  y2 = y + h;
  BYTE  b1, b2;

  // Check if the width is odd
  if(mod(w, 2)>0)
  {
    edgePixel = TRUE;
    x2 -= 1;
  }

  for (j=y; j<y2; j++)
  {
    for (i=x; i<x2; i+=2)
    {
      b1 = (*pb & 0xf0) >> 4;
      b2 = *pb & 0x0f;
      if(b1 != trans) vgaPutPixel(i,   j, b1);
      if(b2 != trans) vgaPutPixel(i+1, j, b2);
      pb++;
    }

    if(edgePixel == TRUE)
    {
      b1 = *pb;
      if(b1 != trans) vgaPutPixel(x2, j, b1);
      pb++;
    }

    opb += Source_lDelta;
    pb = opb; // new test code

  }
}

void DFB_BltFromVGA(int x, int y, int w, int h, void *b, int bw)

//  This algorithm goes from goes from left to right, and inside that loop, top to bottom.
//  It also stores each 4BPP pixel in an entire byte.
{
  unsigned char *vp, *vpY, *vpP;
  unsigned char data, mask, maskP;
  unsigned char *bp, *bpY;
  unsigned char plane_mask;
  int byte_per_line = SCREEN_X >> 3;
  int plane, i, j;

  ASSIGNVP4(x, y, vpP)
  ASSIGNMK4(x, y, maskP)
  get_masks(x, w);
  WRITE_PORT_UCHAR((PUCHAR)GRA_I, 0x05);  // read mode 0
  saved_GC_mode = READ_PORT_UCHAR((PUCHAR)GRA_D);
  WRITE_PORT_UCHAR((PUCHAR)GRA_D, 0x00);
  WRITE_PORT_UCHAR((PUCHAR)GRA_I, 0x04);  // read map select
  saved_GC_rmap = READ_PORT_UCHAR((PUCHAR)GRA_D);

  // clear buffer
  bp=b;
  for (j=h; j>0; j--) {
    memset(bp, 0, w);
    bp += bw;
  }

  for (plane=0, plane_mask=1; plane<4; plane++, plane_mask<<=1) {
    WRITE_PORT_UCHAR((PUCHAR)GRA_D, plane);  // read map select
    vpY = vpP;
    bpY = b;
    for (j=h; j>0; j--) {
      vp = vpY;
      bp = bpY;
      if (leftMask) {
        mask = maskP;
        data = *vp++;
        do {
          if (data & mask) *bp |= plane_mask;
          bp++;
          mask >>= 1;
        } while (mask & leftMask);

      }
      if (byteCounter) {
        for (i=byteCounter; i>0; i--) {
          data = *vp++;
          if (data & 0x80) *bp |= plane_mask;
          bp++;
          if (data & 0x40) *bp |= plane_mask;
          bp++;
          if (data & 0x20) *bp |= plane_mask;
          bp++;
          if (data & 0x10) *bp |= plane_mask;
          bp++;
          if (data & 0x08) *bp |= plane_mask;
          bp++;
          if (data & 0x04) *bp |= plane_mask;
          bp++;
          if (data & 0x02) *bp |= plane_mask;
          bp++;
          if (data & 0x01) *bp |= plane_mask;
          bp++;
        }
      }
      if (rightMask) {
        mask = 0x80;
        data = *vp;
        do {
          if (data & mask) *bp |= plane_mask;
          bp++;
          mask >>= 1;
        } while (mask & rightMask);
      }
      bpY += bw;
      vpY += byte_per_line;
    }
  }

  // reset GC register
  WRITE_PORT_UCHAR((PUCHAR)GRA_D, saved_GC_rmap);
  WRITE_PORT_UCHAR((PUCHAR)GRA_I, 0x05);
  WRITE_PORT_UCHAR((PUCHAR)GRA_D, saved_GC_mode);
}


void DFB_BltToVGA(int x, int y, int w, int h, void *b, int bw)

//  This algorithm goes from goes from left to right, and inside that loop, top to bottom.
//  It also stores each 4BPP pixel in an entire byte.
{
  unsigned char *bp, *bpX;
  unsigned char *vp, *vpX;
  unsigned char mask;
  volatile unsigned char dummy;
  int byte_per_line;
  int i, j;

  bpX = b;
  ASSIGNVP4(x, y, vpX)
  ASSIGNMK4(x, y, mask)
  byte_per_line = SCREEN_X >> 3;
  WRITE_PORT_UCHAR((PUCHAR)GRA_I, 0x05);      // write mode 2
  saved_GC_mode = READ_PORT_UCHAR((PUCHAR)GRA_D);
  WRITE_PORT_UCHAR((PUCHAR)GRA_D, 0x02);
  WRITE_PORT_UCHAR((PUCHAR)GRA_I, 0x03);      // replace
  saved_GC_fun = READ_PORT_UCHAR((PUCHAR)GRA_D);
  WRITE_PORT_UCHAR((PUCHAR)GRA_D, 0x00);
  WRITE_PORT_UCHAR((PUCHAR)GRA_I, 0x08);      // bit mask
  saved_GC_mask = READ_PORT_UCHAR((PUCHAR)GRA_D);

  for (i=w; i>0; i--) {
    WRITE_PORT_UCHAR((PUCHAR)GRA_D, mask);
    bp = bpX;
    vp = vpX;
    for (j=h; j>0; j--) {
      dummy = *vp;
      *vp = *bp;
      bp += bw;
      vp += byte_per_line;
    }
    bpX++;
    if ((mask >>= 1) == 0) {
      vpX++;
      mask = 0x80;
    }
  }

  // reset GC register
  WRITE_PORT_UCHAR((PUCHAR)GRA_D, saved_GC_mask);
  WRITE_PORT_UCHAR((PUCHAR)GRA_I, 0x03);
  WRITE_PORT_UCHAR((PUCHAR)GRA_D, saved_GC_fun);
  WRITE_PORT_UCHAR((PUCHAR)GRA_I, 0x05);
  WRITE_PORT_UCHAR((PUCHAR)GRA_D, saved_GC_mode);
}

void DFB_BltToVGA_Transparent(int x, int y, int w, int h, void *b, int bw)

//  This algorithm goes from goes from left to right, and inside that loop, top to bottom.
//  It also stores each 4BPP pixel in an entire byte.
{
  unsigned char *bp, *bpX;
  unsigned char *vp, *vpX;
  unsigned char mask;
  volatile unsigned char dummy;
  int byte_per_line;
  int i, j;

  bpX = b;
  ASSIGNVP4(x, y, vpX)
  ASSIGNMK4(x, y, mask)
  byte_per_line = SCREEN_X >> 3;
  WRITE_PORT_UCHAR((PUCHAR)GRA_I, 0x05);      // write mode 2
  saved_GC_mode = READ_PORT_UCHAR((PUCHAR)GRA_D);
  WRITE_PORT_UCHAR((PUCHAR)GRA_D, 0x02);
  WRITE_PORT_UCHAR((PUCHAR)GRA_I, 0x03);      // replace
  saved_GC_fun = READ_PORT_UCHAR((PUCHAR)GRA_D);
  WRITE_PORT_UCHAR((PUCHAR)GRA_D, 0x00);
  WRITE_PORT_UCHAR((PUCHAR)GRA_I, 0x08);      // bit mask
  saved_GC_mask = READ_PORT_UCHAR((PUCHAR)GRA_D);

  for (i=w; i>0; i--) {
    WRITE_PORT_UCHAR((PUCHAR)GRA_D, mask);
    bp = bpX;
    vp = vpX;
    for (j=h; j>0; j--) {
      if (*bp != 0)
      {
        dummy = *vp;
        *vp = *bp;
      }
      bp += bw;
      vp += byte_per_line;
    }
    bpX++;
    if ((mask >>= 1) == 0) {
      vpX++;
      mask = 0x80;
    }
  }

  // reset GC register
  WRITE_PORT_UCHAR((PUCHAR)GRA_D, saved_GC_mask);
  WRITE_PORT_UCHAR((PUCHAR)GRA_I, 0x03);
  WRITE_PORT_UCHAR((PUCHAR)GRA_D, saved_GC_fun);
  WRITE_PORT_UCHAR((PUCHAR)GRA_I, 0x05);
  WRITE_PORT_UCHAR((PUCHAR)GRA_D, saved_GC_mode);
}

void DFB_BltToDIB(int x, int y, int w, int h, void *b, int bw, void *bdib, int dibw)

// This algorithm converts a DFB into a DIB
// WARNING: This algorithm is buggy
{
  unsigned char *bp, *bpX, *dib, *dibTmp;
  int i, j, dib_shift;

  bpX = b;
  dib = bdib + y * dibw + (x / 2);

  for (i=w; i>0; i--) {

    // determine the bit shift for the DIB pixel
    dib_shift = mod(w-i, 2);
    if(dib_shift > 0) dib_shift = 4;
    dibTmp = dib;

    bp = bpX;
    for (j=h; j>0; j--) {
      *dibTmp = *bp << dib_shift | *(bp + 1);
      dibTmp += dibw;
      bp += bw;
    }
    bpX++;
    if(dib_shift == 0) dib++;
  }
}

void DIB_BltToDFB(int x, int y, int w, int h, void *b, int bw, void *bdib, int dibw)

// This algorithm converts a DIB into a DFB
{
  unsigned char *bp, *bpX, *dib, *dibTmp;
  int i, j, dib_shift, dib_and;

  bpX = b;
  dib = bdib + y * dibw + (x / 2);

  for (i=w; i>0; i--) {

    // determine the bit shift for the DIB pixel
    dib_shift = mod(w-i, 2);
    if(dib_shift > 0) {
      dib_shift = 0;
      dib_and = 0x0f;
    } else {
      dib_shift = 4;
      dib_and = 0xf0;
    }

    dibTmp = dib;
    bp = bpX;

    for (j=h; j>0; j--) {
      *bp = (*dibTmp & dib_and) >> dib_shift;
      dibTmp += dibw;
      bp += bw;
    }

    bpX++;
    if(dib_shift == 0) dib++;
  }
}
