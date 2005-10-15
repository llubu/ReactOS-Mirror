
/* $Id: $
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 
 * PURPOSE:              Directx headers
 * PROGRAMMER:           Magnus Olsen (greatlrd)
 *
 */

#ifndef __DVP_INCLUDED__
#define __DVP_INCLUDED__

DEFINE_GUID( IID_IDDVideoPortContainer,		0x6C142760,0xA733,0x11CE,0xA5,0x21,0x00,0x20,0xAF,0x0B,0xE5,0x60 );
DEFINE_GUID( IID_IDirectDrawVideoPort,		0xB36D93E0,0x2B43,0x11CF,0xA2,0xDE,0x00,0xAA,0x00,0xB9,0x33,0x56 );
DEFINE_GUID( IID_IDirectDrawVideoPortNotify,    0xA655FB94,0x0589,0x4E57,0xB3,0x33,0x56,0x7A,0x89,0x46,0x8C,0x88);

DEFINE_GUID( DDVPTYPE_E_HREFH_VREFH, 0x54F39980L,0xDA60,0x11CF,0x9B,0x06,0x00,0xA0,0xC9,0x03,0xA3,0xB8);
DEFINE_GUID( DDVPTYPE_E_HREFH_VREFL, 0x92783220L,0xDA60,0x11CF,0x9B,0x06,0x00,0xA0,0xC9,0x03,0xA3,0xB8);
DEFINE_GUID( DDVPTYPE_E_HREFL_VREFH, 0xA07A02E0L,0xDA60,0x11CF,0x9B,0x06,0x00,0xA0,0xC9,0x03,0xA3,0xB8);
DEFINE_GUID( DDVPTYPE_E_HREFL_VREFL, 0xE09C77E0L,0xDA60,0x11CF,0x9B,0x06,0x00,0xA0,0xC9,0x03,0xA3,0xB8);
DEFINE_GUID( DDVPTYPE_CCIR656,	     0xFCA326A0L,0xDA60,0x11CF,0x9B,0x06,0x00,0xA0,0xC9,0x03,0xA3,0xB8);
DEFINE_GUID( DDVPTYPE_BROOKTREE,     0x1352A560L,0xDA61,0x11CF,0x9B,0x06,0x00,0xA0,0xC9,0x03,0xA3,0xB8);
DEFINE_GUID( DDVPTYPE_PHILIPS,	     0x332CF160L,0xDA61,0x11CF,0x9B,0x06,0x00,0xA0,0xC9,0x03,0xA3,0xB8);


typedef struct _DDVIDEOPORTCONNECT
{
    DWORD dwSize;
    DWORD dwPortWidth;
    GUID  guidTypeID;
    DWORD dwFlags;
    ULONG_PTR dwReserved1;
} DDVIDEOPORTCONNECT;

typedef struct _DDVIDEOPORTDESC {
  DWORD              dwSize;
  DWORD              dwFieldWidth;
  DWORD              dwVBIWidth;
  DWORD              dwFieldHeight;
  DWORD              dwMicrosecondsPerField;
  DWORD              dwMaxPixelsPerSecond;
  DWORD              dwVideoPortID;
  DWORD              dwReserved1;
  DDVIDEOPORTCONNECT VideoPortType;
  ULONG_PTR          dwReserved2;
  ULONG_PTR          dwReserved3;
} DDVIDEOPORTDESC;

typedef struct _DDVIDEOPORTBANDWIDTH
{
  DWORD dwSize;
  DWORD dwOverlay;    
  DWORD dwColorkey;	
  DWORD dwYInterpolate;
  DWORD dwYInterpAndColorkey;
  ULONG_PTR dwReserved1;	
  ULONG_PTR dwReserved2;	
} DDVIDEOPORTBANDWIDTH;

typedef struct _DDVIDEOPORTCAPS
{
   DWORD dwSize;		
   DWORD dwFlags;		
   DWORD dwMaxWidth;	
   DWORD dwMaxVBIWidth;
   DWORD dwMaxHeight; 	
   DWORD dwVideoPortID;
   DWORD dwCaps;		
   DWORD dwFX;			
   DWORD dwNumAutoFlipSurfaces;
   DWORD dwAlignVideoPortBoundary;	
   DWORD dwAlignVideoPortPrescaleWidth;
   DWORD dwAlignVideoPortCropBoundary;	
   DWORD dwAlignVideoPortCropWidth;	
   DWORD dwPreshrinkXStep;	
   DWORD dwPreshrinkYStep;	
   DWORD dwNumVBIAutoFlipSurfaces;	
   DWORD dwNumPreferredAutoflip;
   WORD  wNumFilterTapsX;
   WORD  wNumFilterTapsY;
} DDVIDEOPORTCAPS;

typedef struct _DDVIDEOPORTINFO
{
    DWORD           dwSize;
    DWORD           dwOriginX;
    DWORD           dwOriginY;
    DWORD           dwVPFlags;
    RECT            rCrop;
    DWORD           dwPrescaleWidth;
    DWORD           dwPrescaleHeight;
    LPDDPIXELFORMAT lpddpfInputFormat;
    LPDDPIXELFORMAT lpddpfVBIInputFormat;
    LPDDPIXELFORMAT lpddpfVBIOutputFormat;
    DWORD           dwVBIHeight;
    ULONG_PTR       dwReserved1;
    ULONG_PTR       dwReserved2;
} DDVIDEOPORTINFO;

typedef struct _DDVIDEOPORTSTATUS
{
    DWORD              dwSize;
    BOOL               bInUse;
    DWORD              dwFlags;
    DWORD              dwReserved1;
    DDVIDEOPORTCONNECT VideoPortType;
    ULONG_PTR          dwReserved2;
    ULONG_PTR          dwReserved3;
} DDVIDEOPORTSTATUS;

typedef struct _DDVIDEOPORTNOTIFY
{
    LARGE_INTEGER ApproximateTimeStamp;	
    LONG          lField;                        
    UINT          dwSurfaceIndex;                
    LONG          lDone;                         
} DDVIDEOPORTNOTIFY;


#define DDVPCONNECT_DOUBLECLOCK			 0x00000001
#define DDVPCONNECT_VACT			     0x00000002
#define DDVPCONNECT_INVERTPOLARITY		 0x00000004
#define DDVPCONNECT_DISCARDSVREFDATA	 0x00000008
#define DDVPCONNECT_HALFLINE			 0x00000010
#define DDVPCONNECT_INTERLACED			 0x00000020
#define DDVPCONNECT_SHAREEVEN			 0x00000040
#define DDVPCONNECT_SHAREODD			 0x00000080
#define DDVPCAPS_AUTOFLIP			     0x00000001
#define DDVPCAPS_INTERLACED			     0x00000002
#define DDVPCAPS_NONINTERLACED			 0x00000004
#define DDVPCAPS_READBACKFIELD			 0x00000008
#define DDVPCAPS_READBACKLINE			 0x00000010
#define DDVPCAPS_SHAREABLE			     0x00000020
#define DDVPCAPS_SKIPEVENFIELDS			 0x00000040
#define DDVPCAPS_SKIPODDFIELDS			 0x00000080
#define DDVPCAPS_SYNCMASTER			     0x00000100
#define DDVPCAPS_VBISURFACE	         	 0x00000200
#define DDVPCAPS_COLORCONTROL			 0x00000400
#define DDVPCAPS_OVERSAMPLEDVBI			 0x00000800
#define DDVPCAPS_SYSTEMMEMORY			 0x00001000
#define DDVPCAPS_VBIANDVIDEOINDEPENDENT  0x00002000
#define DDVPCAPS_HARDWAREDEINTERLACE     0x00004000
#define DDVPFX_CROPTOPDATA		         0x00000001
#define DDVPFX_CROPX			         0x00000002
#define DDVPFX_CROPY			         0x00000004
#define DDVPFX_INTERLEAVE		         0x00000008
#define DDVPFX_MIRRORLEFTRIGHT        	 0x00000010
#define DDVPFX_MIRRORUPDOWN	        	 0x00000020
#define DDVPFX_PRESHRINKX        		 0x00000040
#define DDVPFX_PRESHRINKY		         0x00000080
#define DDVPFX_PRESHRINKXB        		 0x00000100
#define DDVPFX_PRESHRINKYB	        	 0x00000200
#define DDVPFX_PRESHRINKXS	        	 0x00000400
#define DDVPFX_PRESHRINKYS	        	 0x00000800
#define DDVPFX_PRESTRETCHX		         0x00001000
#define DDVPFX_PRESTRETCHY	        	 0x00002000
#define DDVPFX_PRESTRETCHXN		         0x00004000
#define DDVPFX_PRESTRETCHYN		         0x00008000
#define DDVPFX_VBICONVERT		         0x00010000
#define DDVPFX_VBINOSCALE		         0x00020000
#define DDVPFX_IGNOREVBIXCROP	         0x00040000
#define DDVPFX_VBINOINTERLEAVE        	 0x00080000
#define DDVP_AUTOFLIP		        	 0x00000001
#define DDVP_CONVERT        			 0x00000002
#define DDVP_CROP			        	 0x00000004
#define DDVP_INTERLEAVE			         0x00000008
#define DDVP_MIRRORLEFTRIGHT        	 0x00000010
#define DDVP_MIRRORUPDOWN	        	 0x00000020
#define DDVP_PRESCALE        			 0x00000040
#define DDVP_SKIPEVENFIELDS        		 0x00000080
#define DDVP_SKIPODDFIELDS		         0x00000100
#define DDVP_SYNCMASTER	        		 0x00000200
#define DDVP_VBICONVERT			         0x00000400
#define DDVP_VBINOSCALE	        		 0x00000800
#define DDVP_OVERRIDEBOBWEAVE	         0x00001000
#define DDVP_IGNOREVBIXCROP	        	 0x00002000
#define DDVP_VBINOINTERLEAVE        	 0x00004000
#define DDVP_HARDWAREDEINTERLACE         0x00008000
#define DDVPFORMAT_VIDEO			     0x00000001
#define DDVPFORMAT_VBI		     		 0x00000002
#define DDVPTARGET_VIDEO				 0x00000001
#define DDVPTARGET_VBI			 		 0x00000002
#define DDVPWAIT_BEGIN				 	 0x00000001
#define DDVPWAIT_END				 	 0x00000002
#define DDVPWAIT_LINE			 		 0x00000003
#define DDVPFLIP_VIDEO				 	 0x00000001
#define DDVPFLIP_VBI			 		 0x00000002
#define DDVPSQ_NOSIGNAL			 		 0x00000001
#define DDVPSQ_SIGNALOK				 	 0x00000002
#define DDVPB_VIDEOPORT				 	 0x00000001
#define DDVPB_OVERLAY			 		 0x00000002
#define DDVPB_TYPE			 		 	 0x00000004
#define DDVPBCAPS_SOURCE		 		 0x00000001
#define DDVPBCAPS_DESTINATION			 0x00000002
#define DDVPCREATE_VBIONLY			 	 0x00000001
#define DDVPCREATE_VIDEOONLY			 0x00000002
#define DDVPSTATUS_VBIONLY		 		 0x00000001
#define DDVPSTATUS_VIDEOONLY			 0x00000002

typedef struct _DDVIDEOPORTCONNECT   *LPDDVIDEOPORTCONNECT;
typedef struct _DDVIDEOPORTCAPS      *LPDDVIDEOPORTCAPS;
typedef struct _DDVIDEOPORTDESC      *LPDDVIDEOPORTDESC;
typedef struct _DDVIDEOPORTINFO      *LPDDVIDEOPORTINFO;
typedef struct _DDVIDEOPORTBANDWIDTH *LPDDVIDEOPORTBANDWIDTH;
typedef struct _DDVIDEOPORTSTATUS    *LPDDVIDEOPORTSTATUS;
typedef struct _DDVIDEOPORTNOTIFY    *LPDDVIDEOPORTNOTIFY;

#endif
