#define VGA_NORMAL 0
#define VGA_AND    8
#define VGA_OR     16
#define VGA_XOR    24

typedef struct { int quot, rem; } div_t;

int maskbit[640], y80[480], xconv[640], bit8[640], startmasks[8], endmasks[8];

char* vidmem;

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
  unsigned char  Seq[5];
  unsigned char  Crtc[25];
  unsigned char  Gfx[9];
  unsigned char  Attrib[21];
} VideoMode;

VOID vgaPreCalc();
VOID vgaPutPixel(INT x, INT y, UCHAR c);
VOID vgaPutByte(INT x, INT y, UCHAR c);
VOID vgaGetByte(ULONG offset,
                UCHAR *b, UCHAR *g,
                UCHAR *r, UCHAR *i);
INT vgaGetPixel(INT x, INT y);
BOOL vgaHLine(INT x, INT y, INT len, UCHAR c);
BOOL vgaVLine(INT x, INT y, INT len, UCHAR c);
INT abs(INT nm);
BOOL VGADDIIntersectRect(PRECTL prcDst, PRECTL prcSrc1, PRECTL prcSrc2);
