#include <msvcrti.h>

#define NDEBUG
#include <msvcrtdbg.h>


#define mode_t int

int _chmod(const char *filename, mode_t mode)
{
  DWORD FileAttributes = 0;
  DPRINT("_chmod('%s', %x)\n", filename, mode);

  FileAttributes = GetFileAttributesA(filename);
  if ( FileAttributes == -1 )
    return -1;

  if ( mode == 0 )
    return -1;

  if ((mode & _S_IREAD) == _S_IREAD && (mode & _S_IWRITE) != _S_IWRITE)
    FileAttributes &= FILE_ATTRIBUTE_READONLY;
  else if (((mode & _S_IREAD) != _S_IREAD) && ((mode & _S_IWRITE) == _S_IWRITE))
    FileAttributes &= FILE_ATTRIBUTE_NORMAL;
  else
    FileAttributes &= FILE_ATTRIBUTE_NORMAL;

  if (SetFileAttributesA(filename, FileAttributes) == FALSE)
     return -1;

  return 1;
}

int _wchmod(const wchar_t *filename, mode_t mode)
{
  DWORD FileAttributes = 0;
  DPRINT("_wchmod('%S', %x)\n", filename, mode);

  FileAttributes = GetFileAttributesW(filename);
  if ( FileAttributes == -1 )
    return -1;

  if ( mode == 0 )
    return -1;

  if ((mode & _S_IREAD) == _S_IREAD && (mode & _S_IWRITE) != _S_IWRITE)
    FileAttributes &= FILE_ATTRIBUTE_READONLY;
  else if (((mode & _S_IREAD) != _S_IREAD) && ((mode & _S_IWRITE) == _S_IWRITE))
    FileAttributes &= FILE_ATTRIBUTE_NORMAL;
  else
    FileAttributes &= FILE_ATTRIBUTE_NORMAL;

  if (SetFileAttributesW(filename, FileAttributes) == FALSE)
     return -1;

  return 1;
}
