#include <sys/types.h>
#include <sys/stat.h>

#ifdef WIN32
#include <sys/utime.h>
#else
#include <sys/time.h>
#include <stdlib.h>
#endif

#include <fcntl.h>
#include <stdio.h>

char* convert_path(char* origpath)
{
  char* newpath;
  int i;
   
  newpath = (char *)strdup(origpath);
   
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

int main(int argc, char* argv[])
{
  char* path;
  FILE* file;
#ifdef WIN32
  time_t now;
  struct utimbuf fnow;
#endif

  if (argc != 2)
    {
      fprintf(stderr, "Wrong number of arguments.\n");
      exit(1);
    }

  path = convert_path(argv[1]);
  file = (FILE *)open(path, S_IWRITE);
  if (file == (void*)-1)
    {
      file = (FILE *)open(path, S_IWRITE | O_CREAT);
      if (file == (void*)-1)
        {
          fprintf(stderr, "Cannot create file.\n");
          exit(1);
        }
    }

  close(file);

#ifdef WIN32
  now = time();
  fnow.actime = now;
  fnow.modtime = now;
  (int) utime(path, &fnow);
#else
  (int) utimes(path, NULL);
#endif

  exit(0);
}
