#ifndef _WINGLUE_H
#define _WINGLUE_H

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef unsigned short WCHAR;

#define LOBYTE(w)	((BYTE)(w))
#define HIBYTE(w)	((BYTE)(((WORD)(w)>>8)&0xFF))

#define IMAGE_FILE_DLL	8192
#define IMAGE_SUBSYSTEM_WINDOWS_GUI	2
#define IMAGE_SUBSYSTEM_WINDOWS_CUI	3
#define IMAGE_NUMBEROF_DIRECTORY_ENTRIES 16
#define IMAGE_FILE_MACHINE_I386	332
#define IMAGE_NT_SIGNATURE 0x00004550
#define IMAGE_NT_OPTIONAL_HDR_MAGIC 0x10b

#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif
#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif

#endif /* _WINGLUE_H */
