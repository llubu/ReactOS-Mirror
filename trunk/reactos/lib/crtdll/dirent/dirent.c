/*
 * dirent.c
 *
 * Derived from DIRLIB.C by Matt J. Weinstein 
 * This note appears in the DIRLIB.H
 * DIRLIB.H by M. J. Weinstein   Released to public domain 1-Jan-89
 *
 * Updated by Jeremy Bettis <jeremy@hksys.com>
 * Significantly revised and rewinddir, seekdir and telldir added by Colin
 * Peters <colin@fu.is.saga-u.ac.jp>
 *
 * $Revision: 1.3 $
 * $Author: robd $
 * $Date: 2002/11/24 18:42:13 $
 *
 */

#include <msvcrt/stdlib.h>
/* #include <msvcrt/ctype.h> */
#include <msvcrt/errno.h>
#include <msvcrt/string.h>
#include <msvcrt/dir.h>
#include <msvcrt/direct.h>
#include <msvcrt/sys/stat.h>

#include <msvcrt/dirent.h>

#define SUFFIX	"*"
#define	SLASH	"\\"
#define streq(a,b)	(strcmp(a,b)==0)

/*
 * opendir
 *
 * Returns a pointer to a DIR structure appropriately filled in to begin
 * searching a directory.
 */
DIR*
opendir(const char* szPath)
{
	DIR* nd;
	struct stat statDir;

	errno = 0;

	if (!szPath)
	{
		errno = EFAULT;
		return (DIR*) 0;
	}

	if (szPath[0] == '\0')
	{
		errno = ENOTDIR;
		return (DIR*) 0;
	}

	/* Attempt to determine if the given path really is a directory. */
	if (_stat (szPath, &statDir))
	{
		/* Error, stat should have set an error value. */
		return (DIR*) 0;
	}

	if (!S_ISDIR(statDir.st_mode))
	{
		/* Error, stat reports not a directory. */
		errno = ENOTDIR;
		return (DIR*) 0;
	}

	/* Allocate enough space to store DIR structure and the complete
	 * directory path given. */
	nd = (DIR*) malloc (sizeof(DIR) + strlen(szPath) + strlen(SLASH) +
		strlen(SUFFIX));

	if (!nd)
	{
		/* Error, out of memory. */
		errno = ENOMEM;
		return (DIR*) 0;
	}

	/* Create the search expression. */
	strcpy(nd->dd_name, szPath);

	/* Add on a slash if the path does not end with one. */
	if (nd->dd_name[0] != '\0' &&
	    nd->dd_name[strlen(nd->dd_name)-1] != '/' &&
	    nd->dd_name[strlen(nd->dd_name)-1] != '\\')
	{
		strcat(nd->dd_name, SLASH);
	}

	/* Add on the search pattern */
	strcat(nd->dd_name, SUFFIX);

	/* Initialize handle to -1 so that a premature closedir doesn't try
	 * to call _findclose on it. */
	nd->dd_handle = -1;

	/* Initialize the status. */
	nd->dd_stat = 0;

	/* Initialize the dirent structure. ino and reclen are invalid under
	 * Win32, and name simply points at the appropriate part of the
	 * findfirst_t structure. */
	nd->dd_dir.d_ino = 0;
	nd->dd_dir.d_reclen = 0;
	nd->dd_dir.d_namlen = 0;
	nd->dd_dir.d_name = nd->dd_dta.name;

	return nd;
}


/*
 * readdir
 *
 * Return a pointer to a dirent structure filled with the information on the
 * next entry in the directory.
 */
struct dirent *
readdir( DIR *dirp )
{
	errno = 0;

	/* Check for valid DIR struct. */
	if (!dirp)
	{
		errno = EFAULT;
		return (struct dirent*) 0;
	}

	if (dirp->dd_dir.d_name != dirp->dd_dta.name)
	{
		/* The structure does not seem to be set up correctly. */
		errno = EINVAL;
		return (struct dirent*) 0;
	}

	if (dirp->dd_stat < 0)
	{
		/* We have already returned all files in the directory
		 * (or the structure has an invalid dd_stat). */
		return (struct dirent *) 0;
	}
	else if (dirp->dd_stat == 0)
	{
		/* We haven't started the search yet. */
		/* Start the search */
		dirp->dd_handle = _findfirst(dirp->dd_name, &(dirp->dd_dta));

		if (dirp->dd_handle == -1)
		{
			/* Whoops! Seems there are no files in that
			 * directory. */
			dirp->dd_stat = -1;
		}
		else
		{
			dirp->dd_stat = 1;
		}
	}
	else
	{
		/* Get the next search entry. */
		if (_findnext(dirp->dd_handle, &(dirp->dd_dta)))
		{
			/* We are off the end or otherwise error. */
			_findclose (dirp->dd_handle);
			dirp->dd_handle = -1;
			dirp->dd_stat = -1;
		}
		else
		{
			/* Update the status to indicate the correct
			 * number. */
			dirp->dd_stat++;
		}
	}

	if (dirp->dd_stat > 0)
	{
		/* Successfully got an entry. Everything about the file is
		 * already appropriately filled in except the length of the
		 * file name. */
		dirp->dd_dir.d_namlen = strlen(dirp->dd_dir.d_name);
		return &dirp->dd_dir;
	}

	return (struct dirent*) 0;
}


/*
 * closedir
 *
 * Frees up resources allocated by opendir.
 */
int
closedir (DIR* dirp)
{
	int	rc;

	errno = 0;
	rc = 0;

	if (!dirp)
	{
		errno = EFAULT;
		return -1;
	}

	if (dirp->dd_handle != -1)
	{
		rc = _findclose(dirp->dd_handle);
	}

	/* Delete the dir structure. */
	free (dirp);

	return rc;
}

/*
 * rewinddir
 *
 * Return to the beginning of the directory "stream". We simply call findclose
 * and then reset things like an opendir.
 */
void
rewinddir (DIR* dirp)
{
	errno = 0;

	if (!dirp)
	{
		errno = EFAULT;
		return;
	}

	if (dirp->dd_handle != -1)
	{
		_findclose(dirp->dd_handle);
	}

	dirp->dd_handle = -1;
	dirp->dd_stat = 0;
}

/*
 * telldir
 *
 * Returns the "position" in the "directory stream" which can be used with
 * seekdir to go back to an old entry. We simply return the value in stat.
 */
long
telldir (DIR* dirp)
{
	errno = 0;

	if (!dirp)
	{
		errno = EFAULT;
		return -1;
	}
	return dirp->dd_stat;
}

/*
 * seekdir
 *
 * Seek to an entry previously returned by telldir. We rewind the directory
 * and call readdir repeatedly until either dd_stat is the position number
 * or -1 (off the end). This is not perfect, in that the directory may
 * have changed while we weren't looking. But that is probably the case with
 * any such system.
 */
void
seekdir (DIR* dirp, long lPos)
{
	errno = 0;

	if (!dirp)
	{
		errno = EFAULT;
		return;
	}

	if (lPos < -1)
	{
		/* Seeking to an invalid position. */
		errno = EINVAL;
		return;
	}
	else if (lPos == -1)
	{
		/* Seek past end. */
		if (dirp->dd_handle != -1)
		{
			_findclose (dirp->dd_handle);
		}
		dirp->dd_handle = -1;
		dirp->dd_stat = -1;
	}
	else
	{
		/* Rewind and read forward to the appropriate index. */
		rewinddir (dirp);

		while ((dirp->dd_stat < lPos) && readdir(dirp))
			;
	}
}

