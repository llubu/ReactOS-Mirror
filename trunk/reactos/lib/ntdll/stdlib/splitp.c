#include <stdlib.h>
#include <string.h>

/*
 * @implemented
 */
void _splitpath(const char* path, char* drive, char* dir, char* fname, char* ext)
{
    char* tmp_drive;
    char* tmp_dir;
    char* tmp_ext;

    tmp_drive = (char*)strchr(path,':');
    if (drive) {
	if (tmp_drive) {
	    strncpy(drive,tmp_drive-1,2);
	    *(drive+2) = 0;
	} else {
	    *drive = 0;
	}
    }
    if (!tmp_drive) {
	tmp_drive = (char*)path - 1;
    }

    tmp_dir = (char*)strrchr(path,'\\');
    if (dir) {
	if (tmp_dir) {
	    strncpy(dir,tmp_drive+1,tmp_dir-tmp_drive);
            *(dir+(tmp_dir-tmp_drive)) = 0;
	} else {
	    *dir =0;
	}
    }

    tmp_ext = (char*)strrchr(path,'.');
    if (!tmp_ext) {
	tmp_ext = (char*)path+strlen(path);
    }
    if (ext) {
        strcpy(ext,tmp_ext);
    }

    if (tmp_dir) {
        strncpy(fname,tmp_dir+1,tmp_ext-tmp_dir-1);
        *(fname+(tmp_ext-tmp_dir-1)) = 0;
    } else {
        strncpy(fname,tmp_drive+1,tmp_ext-tmp_drive-1);
        *(fname+(tmp_ext-path))=0;
    }
}

