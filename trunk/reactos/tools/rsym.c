/*
 * Usage: rsym input-file output-file
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define IMAGE_DOS_MAGIC 0x5a4d
#define IMAGE_PE_MAGIC 0x00004550

#define IMAGE_SIZEOF_SHORT_NAME 8

#define IMAGE_NUMBEROF_DIRECTORY_ENTRIES 16

typedef void* PVOID;
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef signed long LONG;
typedef unsigned long ULONG;

#pragma pack(push,2)
typedef struct _IMAGE_DOS_HEADER {
	WORD e_magic;
	WORD e_cblp;
	WORD e_cp;
	WORD e_crlc;
	WORD e_cparhdr;
	WORD e_minalloc;
	WORD e_maxalloc;
	WORD e_ss;
	WORD e_sp;
	WORD e_csum;
	WORD e_ip;
	WORD e_cs;
	WORD e_lfarlc;
	WORD e_ovno;
	WORD e_res[4];
	WORD e_oemid;
	WORD e_oeminfo;
	WORD e_res2[10];
	LONG e_lfanew;
} IMAGE_DOS_HEADER,*PIMAGE_DOS_HEADER;
#pragma pack(pop)
#pragma pack(push,4)
typedef struct _IMAGE_DATA_DIRECTORY {
	DWORD VirtualAddress;
	DWORD Size;
} IMAGE_DATA_DIRECTORY,*PIMAGE_DATA_DIRECTORY;
typedef struct _IMAGE_OPTIONAL_HEADER {
	WORD Magic;
	BYTE MajorLinkerVersion;
	BYTE MinorLinkerVersion;
	DWORD SizeOfCode;
	DWORD SizeOfInitializedData;
	DWORD SizeOfUninitializedData;
	DWORD AddressOfEntryPoint;
	DWORD BaseOfCode;
	DWORD BaseOfData;
	DWORD ImageBase;
	DWORD SectionAlignment;
	DWORD FileAlignment;
	WORD MajorOperatingSystemVersion;
	WORD MinorOperatingSystemVersion;
	WORD MajorImageVersion;
	WORD MinorImageVersion;
	WORD MajorSubsystemVersion;
	WORD MinorSubsystemVersion;
	DWORD Reserved1;
	DWORD SizeOfImage;
	DWORD SizeOfHeaders;
	DWORD CheckSum;
	WORD Subsystem;
	WORD DllCharacteristics;
	DWORD SizeOfStackReserve;
	DWORD SizeOfStackCommit;
	DWORD SizeOfHeapReserve;
	DWORD SizeOfHeapCommit;
	DWORD LoaderFlags;
	DWORD NumberOfRvaAndSizes;
	IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER,*PIMAGE_OPTIONAL_HEADER;
typedef struct _IMAGE_FILE_HEADER {
	WORD Machine;
	WORD NumberOfSections;
	DWORD TimeDateStamp;
	DWORD PointerToSymbolTable;
	DWORD NumberOfSymbols;
	WORD SizeOfOptionalHeader;
	WORD Characteristics;
} IMAGE_FILE_HEADER, *PIMAGE_FILE_HEADER;
typedef struct _IMAGE_SECTION_HEADER {
	BYTE Name[IMAGE_SIZEOF_SHORT_NAME];
	union {
		DWORD PhysicalAddress;
		DWORD VirtualSize;
	} Misc;
	DWORD VirtualAddress;
	DWORD SizeOfRawData;
	DWORD PointerToRawData;
	DWORD PointerToRelocations;
	DWORD PointerToLinenumbers;
	WORD NumberOfRelocations;
	WORD NumberOfLinenumbers;
	DWORD Characteristics;
} IMAGE_SECTION_HEADER,*PIMAGE_SECTION_HEADER;
#pragma pack(pop)

typedef struct _SYMBOLFILE_HEADER {
  unsigned long StabsOffset;
  unsigned long StabsLength;
  unsigned long StabstrOffset;
  unsigned long StabstrLength;
} SYMBOLFILE_HEADER, *PSYMBOLFILE_HEADER;

typedef struct _STAB_ENTRY {
  unsigned long n_strx;         /* index into string table of name */
  unsigned char n_type;         /* type of symbol */
  unsigned char n_other;        /* misc info (usually empty) */
  unsigned short n_desc;        /* description field */
  unsigned long n_value;        /* value of symbol */
} STAB_ENTRY, *PSTAB_ENTRY;

#define N_FUN 0x24
#define N_SLINE 0x44
#define N_SO 0x64

typedef struct 
{
   unsigned long OldOffset;
   unsigned long NewOffset;
   char* Name;
   unsigned long Length;
} STR_ENTRY, *PSTR_ENTRY;

char* convert_path(char* origpath)
{
   char* newpath;
   int i;
   
   newpath = strdup(origpath);
   
   i = 0;
   while (newpath[i] != 0)
     {
#ifdef UNIX_PATHS
	if (newpath[i] == '\\')
	  {
	     newpath[i] = '/';
	  }
#else
#ifdef DOS_PATHS
	if (newpath[i] == '/')
	  {
	     newpath[i] = '\\';
	  }
#endif	
#endif	
	i++;
     }
   return(newpath);
}

#define TRANSFER_SIZE      (65536)

int main(int argc, char* argv[])
{
  SYMBOLFILE_HEADER SymbolFileHeader;
  IMAGE_DOS_HEADER PEDosHeader;
  IMAGE_FILE_HEADER PEFileHeader;
  PIMAGE_SECTION_HEADER PESectionHeaders;
  PVOID SymbolsBase;
  ULONG SymbolsLength;
  PVOID SymbolStringsBase;
  ULONG SymbolStringsLength;
  ULONG Idx;
  char* path1;
  char* path2;
  FILE* in;
  FILE* out;
  int n_in;
  int n_out;
  PSTAB_ENTRY StabEntry;
  ULONG Count;
  ULONG i;
  ULONG SymbolsCount;
  PSTR_ENTRY StrEntry;
  ULONG StrCount;
  ULONG j;
   
   if (argc != 3)
     {
	fprintf(stderr, "Too many arguments\n");
	exit(1);
     }
   
   path1 = convert_path(argv[1]);
   path2 = convert_path(argv[2]);
   
   in = fopen(path1, "rb");
   if (in == NULL)
     {
	perror("Cannot open input file");
	exit(1);
     }

   out = fopen(path2, "wb");
   if (out == NULL)
     {
	perror("Cannot open output file");
	fclose(in);
	exit(1);
     }

  /* Check if MZ header exists  */
  n_in = fread(&PEDosHeader, 1, sizeof(PEDosHeader), in);
  if (PEDosHeader.e_magic != IMAGE_DOS_MAGIC && PEDosHeader.e_lfanew != 0L)
    {
      perror("Input file is not a PE image.\n");
    }

  /* Read PE file header  */
  /* sizeof(ULONG) = sizeof(MAGIC) */
  fseek(in, PEDosHeader.e_lfanew + sizeof(ULONG), SEEK_SET);
  n_in = fread(&PEFileHeader, 1, sizeof(PEFileHeader), in);

  /* Read PE section headers  */
  PESectionHeaders = malloc(PEFileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER));
  fseek(in, PEDosHeader.e_lfanew + sizeof(ULONG) + sizeof(IMAGE_FILE_HEADER)
    + sizeof(IMAGE_OPTIONAL_HEADER), SEEK_SET);
  n_in = fread(PESectionHeaders, 1, PEFileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER), in);

  /* Copy .stab and .stabstr sections if available */
  SymbolsBase = NULL;
  SymbolsLength = 0;
  SymbolStringsBase = NULL;
  SymbolStringsLength = 0;

  for (Idx = 0; Idx < PEFileHeader.NumberOfSections; Idx++)
    {
      //printf("section: '%.08s'\n", PESectionHeaders[Idx].Name);
      if ((strncmp(PESectionHeaders[Idx].Name, ".stab", 5) == 0)
        && (PESectionHeaders[Idx].Name[5] == 0))
        {
	   //printf(".stab section found. Size %d\n", 
           //  PESectionHeaders[Idx].SizeOfRawData);

           SymbolsLength = PESectionHeaders[Idx].SizeOfRawData;
           SymbolsBase = malloc(SymbolsLength);

           fseek(in, PESectionHeaders[Idx].PointerToRawData, SEEK_SET);
           n_in = fread(SymbolsBase, 1, SymbolsLength, in);
        }

      if (strncmp(PESectionHeaders[Idx].Name, ".stabstr", 8) == 0)
        {
	   //printf(".stabstr section found. Size %d\n", 
           //  PESectionHeaders[Idx].SizeOfRawData);

           SymbolStringsLength = PESectionHeaders[Idx].SizeOfRawData;
           SymbolStringsBase = malloc(SymbolStringsLength);

           fseek(in, PESectionHeaders[Idx].PointerToRawData, SEEK_SET);
           n_in = fread(SymbolStringsBase, 1, SymbolStringsLength, in);
        }
    }

  StabEntry = SymbolsBase;
  SymbolsCount = SymbolsLength / sizeof(STAB_ENTRY);
  Count = 0;

  for (i = 0; i < SymbolsCount; i++)
    {
      if (StabEntry[i].n_type == N_FUN ||
	  StabEntry[i].n_type == N_SLINE ||
	  StabEntry[i].n_type == N_SO)
        {
	  memmove(&StabEntry[Count], &StabEntry[i], sizeof(STAB_ENTRY));
	  Count++;
	}
    }

  StrEntry = malloc(sizeof(STR_ENTRY) * Count);
  StrCount = 0;

  for (i = 0; i < Count; i++)
    {
      for (j = 0; j < StrCount; j++)
        {
	  if (StabEntry[i].n_strx == StrEntry[j].OldOffset)
	    {
	      StabEntry[i].n_strx = StrEntry[j].NewOffset;
	      break;
	    }
	}
      if (j >= StrCount)
        {
	  StrEntry[StrCount].OldOffset = StabEntry[i].n_strx;
	  StrEntry[StrCount].Name = (char*)SymbolStringsBase + StrEntry[StrCount].OldOffset;
	  StrEntry[StrCount].Length = strlen(StrEntry[StrCount].Name) + 1;
	  if (StrCount == 0)
	    {
	      StrEntry[StrCount].NewOffset = 0;
	    }
	  else
	    {
	      StrEntry[StrCount].NewOffset = StrEntry[StrCount-1].NewOffset + StrEntry[StrCount-1].Length;
	    }
	  StabEntry[i].n_strx = StrEntry[StrCount].NewOffset;
	  StrCount++;
	}
    }

  SymbolFileHeader.StabsOffset = sizeof(SYMBOLFILE_HEADER);
  SymbolFileHeader.StabsLength = Count * sizeof(STAB_ENTRY);
  SymbolFileHeader.StabstrOffset = SymbolFileHeader.StabsOffset + SymbolFileHeader.StabsLength;
  SymbolFileHeader.StabstrLength = StrEntry[StrCount-1].NewOffset + StrEntry[StrCount-1].Length;

  n_out = fwrite(&SymbolFileHeader, 1, sizeof(SYMBOLFILE_HEADER), out);
  n_out = fwrite(SymbolsBase, 1, SymbolFileHeader.StabsLength, out);
  for (i = 0; i < StrCount; i++)
    {
      fwrite(StrEntry[i].Name, 1, StrEntry[i].Length, out);
    }

  fclose(out);
  exit(0);
}
