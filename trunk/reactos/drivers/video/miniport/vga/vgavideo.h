#include <ddk/ntddk.h>
#include <ddk/ntddvid.h>

#define VGA_NORMAL 0
#define VGA_AND    8
#define VGA_OR     16
#define VGA_XOR    24

//This is in mingw standard headers
//typedef struct { int quot, rem; } div_t;

int maskbit[640], y80[480], xconv[640], bit8[640], startmasks[8], endmasks[8];

char* vidmem;

#define MISC     0x3c2
#define SEQ      0x3c4
#define CRTC     0x3d4
#define GRAPHICS 0x3ce
#define FEATURE  0x3da
#define ATTRIB   0x3c0
#define STATUS   0x3da

#define SEQ_I   0x3C4           /* Sequencer Index */
#define SEQ_D   0x3C5           /* Sequencer Data Register */

#define GRA_I   0x3CE           /* Graphics Controller Index */
#define GRA_D   0x3CF           /* Graphics Controller Data Register */


typedef struct _VideoMode {
  unsigned short VidSeg;
  unsigned char  Misc;
  unsigned char  Feature;
  unsigned short Seq[6];
  unsigned short Crtc[25];
  unsigned short Gfx[9];
  unsigned char  Attrib[21];
} VideoMode;

VOID vgaPreCalc();
