/*
 * COPYRIGHT:      See COPYING in the top level directory
 * PROJECT:        ReactOS kernel
 * FILE:           include/ddk/ntddk.h
 * PURPOSE:        Interface definitions for drivers
 * PROGRAMMER:     David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                 15/05/98: Created
 */

#ifndef __NTDDK_H
#define __NTDDK_H

#ifdef __cplusplus
extern "C"
{
#endif

/* INCLUDES ***************************************************************/

#include <windows.h>

#define QUAD_PART(LI)  (*(LONGLONG *)(&LI))


#define  IO_DISK_INCREMENT  4

#define  FILE_WORD_ALIGNMENT  0x0001

#define  FILE_OPENED          0x0001

#include <ddk/status.h>
#include <ddk/ntdef.h>
#include <ddk/defines.h>
#include <ddk/types.h>
#include <ddk/cfgtypes.h>
#include <ddk/ketypes.h>
#include <ddk/obtypes.h>
#include <ddk/mmtypes.h>
#include <ddk/setypes.h>
#include <ddk/iotypes.h>
#include <ddk/extypes.h>
#include <ddk/pstypes.h>
#include <ddk/ioctrl.h>   
#include <internal/hal/ddk.h>
   
#include <ddk/rtl.h>
#include <ddk/zw.h>
#include <ddk/exfuncs.h>
#include <ddk/mmfuncs.h>
#include <ddk/kefuncs.h>
#include <ddk/iofuncs.h> 
#include <ddk/psfuncs.h>
#include <ddk/obfuncs.h>
#include <ddk/dbgfuncs.h>
      
#ifdef __cplusplus
};
#endif

#endif /* __NTDDK_H */

