/* 
 * process.h
 *
 * Function calls for spawning child processes.
 *
 * This file is part of the Mingw32 package.
 *
 * Contributors:
 *  Created by Colin Peters <colin@bird.fu.is.saga-u.ac.jp>
 *
 *  THIS SOFTWARE IS NOT COPYRIGHTED
 *
 *  This source code is offered for use in the public domain. You may
 *  use, modify or distribute it freely.
 *
 *  This code is distributed in the hope that it will be useful but
 *  WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 *  DISCLAIMED. This includes but is not limited to warranties of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * $Revision: 1.4 $
 * $Author$
 * $Date$
 *
 */
/* Appropriated for Reactos Crtdll by Ariadne */
/* changed second argument of cwait from nPID to hProc */

#ifndef __STRICT_ANSI__

#ifndef _PROCESS_H_
#define _PROCESS_H_

/*
 * Constants for cwait actions.
 * Obsolete for Win32.
 */
#define _WAIT_CHILD      0
#define _WAIT_GRANDCHILD 1

#ifndef _NO_OLDNAMES
#define WAIT_CHILD      _WAIT_CHILD
#define WAIT_GRANDCHILD _WAIT_GRANDCHILD
#endif  /* Not _NO_OLDNAMES */

/*
 * Mode constants for spawn functions.
 */
#define _P_WAIT     0
#define _P_NOWAIT   1
#define _P_OVERLAY  2
#define _OLD_P_OVERLAY  _P_OVERLAY
#define _P_NOWAITO  3
#define _P_DETACH   4

#ifndef _NO_OLDNAMES
#define P_WAIT      _P_WAIT
#define P_NOWAIT    _P_NOWAIT
#define P_OVERLAY   _P_OVERLAY
#define OLD_P_OVERLAY   _OLD_P_OVERLAY
#define P_NOWAITO   _P_NOWAITO
#define P_DETACH    _P_DETACH
#endif  /* Not _NO_OLDNAMES */


#ifdef  __cplusplus
extern "C" {
#endif

void    _cexit(void);
void    _c_exit(void);

int _cwait (int* pnStatus, int hProc, int nAction);

int _getpid(void);

int _execl      (const char* szPath, const char* szArgv0, ...);
int _execle     (const char* szPath, const char* szArgv0, ...);
int _execlp     (const char* szPath, const char* szArgv0, ...);
int _execlpe    (const char* szPath, const char* szArgv0, ...);
int _execv      (const char* szPath, char* const* szaArgv);
int _execve     (const char* szPath, char* const* szaArgv, char* const* szaEnv);
int _execvp     (const char* szPath, char* const* szaArgv);
int _execvpe    (const char* szPath, char* const* szaArgv, char* const* szaEnv);

int _spawnl     (int nMode, const char* szPath, const char* szArgv0, ...);
int _spawnle    (int nMode, const char* szPath, const char* szArgv0,...);
int _spawnlp    (int nMode, const char* szPath, const char* szArgv0,...);
int _spawnlpe   (int nMode, const char* szPath, const char* szArgv0,...);
int _spawnv     (int nMode, const char* szPath, char* const* szaArgv);
int _spawnve    (int nMode, const char* szPath, char* const* szaArgv, char* const* szaEnv);
int _spawnvp    (int nMode, const char* szPath, char* const* szaArgv);
int _spawnvpe   (int nMode, const char* szPath, char* const* szaArgv, char* const* szaEnv);
/*
 * The functions _beginthreadex and _endthreadex are not provided by CRTDLL.
 * They are provided by MSVCRT.
 *
 * NOTE: Apparently _endthread calls CloseHandle on the handle of the thread,
 * making for race conditions if you are not careful. Basically you have to
 * make sure that no-one is going to do *anything* with the thread handle
 * after the thread calls _endthread or returns from the thread function.
 *
 * NOTE: No old names for these functions. Use the underscore.
 */
unsigned long
    _beginthread(void (__cdecl *pfuncStart)(void*),
             unsigned unStackSize, void* pArgList);
void    _endthread  (void);

#ifdef __MSVCRT__
unsigned long
    _beginthreadex(void* pSecurity, unsigned unStackSize,
             unsigned (__stdcall *pfuncStart)(void*), void* pArgList,
             unsigned unInitFlags, unsigned* pThreadAddr);
void _endthreadex(unsigned unExitCode);
#endif


void* _loaddll(char* name);
int _unloaddll(void* handle);

unsigned long __threadid(void);
#define _threadid  __threadid()
void* __threadhandle(void);


#ifndef _NO_OLDNAMES

#define cwait           _cwait
#define getpid          _getpid
#define execl           _execl
#define execle          _execle
#define execlp          _execlp
#define execlpe         _execlpe

#define execv           _execv
#define execve          _execve
#define execvp          _execvp
#define execvpe         _execvpe

#define spawnl          _spawnl
#define spawnle         _spawnle
#define spawnlp         _spawnlp
#define spawnlpe        _spawnlpe

#define spawnv          _spawnv
#define spawnve         _spawnve
#define spawnvp         _spawnvp
#define spawnvpe        _spawnvpe

#endif  /* Not _NO_OLDNAMES */

#ifdef  __cplusplus
}
#endif

#endif  /* _PROCESS_H_ not defined */

#endif  /* Not __STRICT_ANSI__ */

