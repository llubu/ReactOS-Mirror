/*
 *  CMD.H - header file for the modules in CMD.EXE
 *
 *
 *  History:
 *
 *    7-15-95 Tim Norman
 *        started
 *
 *    06/29/98 (Rob Lake)
 *        Moved error messages in here
 *
 *    07/12/98 (Rob Lake)
 *        Moved more error messages here.
 *
 *    30-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        Added compile date to version.
 *
 *    26-Feb-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Introduced a new version string.
 *        Thanks to Emanuele Aliberti!
 */

#ifndef _CMD_H_INCLUDED_
#define _CMD_H_INCLUDED_

#include "config.h"

#include <windows.h>
#include <tchar.h>

#include "cmdver.h"

#define BREAK_BATCHFILE 1
#define BREAK_OUTOFBATCH 2
#define BREAK_INPUT 3
#define BREAK_IGNORE 4

/* define some error messages */

#define D_ON         _T("on")
#define D_OFF        _T("off")


/* command line buffer length */
#ifdef __REACTOS__
#define CMDLINE_LENGTH  512
#else
#define CMDLINE_LENGTH  8192
//#define CMDLINE_LENGTH  1024
#endif

/* global variables */
extern HANDLE hOut;
extern HANDLE hIn;
extern HANDLE hConsole;
extern WORD   wColor;
extern WORD   wDefColor;
extern BOOL   bCtrlBreak;
extern BOOL   bIgnoreEcho;
extern BOOL   bExit;
extern INT    nErrorLevel;
extern SHORT  maxx;
extern SHORT  maxy;
extern OSVERSIONINFO osvi;



/* Prototypes for ALIAS.C */
VOID InitializeAlias (VOID);
VOID DestroyAlias (VOID);
VOID ExpandAlias (LPTSTR, INT);
INT CommandAlias (LPTSTR, LPTSTR);


/* Prototypes for ATTRIB.C */
INT CommandAttrib (LPTSTR, LPTSTR);


/* Prototypes for BEEP.C */
INT cmd_beep (LPTSTR, LPTSTR);


/* Prototypes for CALL.C */
INT cmd_call (LPTSTR, LPTSTR);


/* Prototypes for CHCP.C */
INT CommandChcp (LPTSTR, LPTSTR);


/* Prototypes for CHOICE.C */
INT CommandChoice (LPTSTR, LPTSTR);


/* Prototypes for CLS.C */
INT cmd_cls (LPTSTR, LPTSTR);


/* Prototypes for CMD.C */
VOID ParseCommandLine (LPTSTR);
VOID AddBreakHandler (VOID);
VOID RemoveBreakHandler (VOID);

extern HANDLE CMD_ModuleHandle;


/* Prototypes for CMDINPUT.C */
VOID ReadCommand (LPTSTR, INT);


/* Prototypes for CMDTABLE.C */
#define CMD_SPECIAL     1
#define CMD_BATCHONLY   2
#define CMD_HIDE        4

typedef struct tagCOMMAND
{
	LPTSTR name;
	INT    flags;
	INT    (*func) (LPTSTR, LPTSTR);
} COMMAND, *LPCOMMAND;

extern COMMAND cmds[];		/* The internal command table */

VOID PrintCommandList (VOID);
VOID PrintCommandListDetail (VOID);


/* Prototypes for COLOR.C */
VOID SetScreenColor(WORD wArgColor, BOOL bFill);
INT CommandColor (LPTSTR, LPTSTR);


/* Prototypes for CONSOLE.C */
#ifdef _DEBUG
VOID DebugPrintf (LPTSTR, ...);
#endif /* _DEBUG */

VOID ConInDummy (VOID);
VOID ConInDisable (VOID);
VOID ConInEnable (VOID);
VOID ConInFlush (VOID);
VOID ConInKey (PINPUT_RECORD);
VOID ConInString (LPTSTR, DWORD);

VOID ConOutChar (TCHAR);
VOID ConOutPuts (LPTSTR);
VOID ConOutPrintf (LPTSTR, ...);
VOID ConOutPrintfPaging (BOOL NewPage, LPTSTR, ...);
VOID ConErrChar (TCHAR);
VOID ConErrPuts (LPTSTR);
VOID ConErrPrintf (LPTSTR, ...);
VOID ConOutFormatMessage (DWORD MessageId, ...);
VOID ConErrFormatMessage (DWORD MessageId, ...);

SHORT GetCursorX  (VOID);
SHORT GetCursorY  (VOID);
VOID  GetCursorXY (PSHORT, PSHORT);
VOID  SetCursorXY (SHORT, SHORT);

VOID GetScreenSize (PSHORT, PSHORT);
VOID SetCursorType (BOOL, BOOL);

VOID ConOutResPuts (UINT resID);
VOID ConErrResPuts (UINT resID);
VOID ConOutResPaging(BOOL NewPage, UINT resID);

/* Prototypes for COPY.C */
INT cmd_copy (LPTSTR, LPTSTR);


/* Prototypes for DATE.C */
INT cmd_date (LPTSTR, LPTSTR);


/* Prototypes for DEL.C */
INT CommandDelete (LPTSTR, LPTSTR);


/* Prototypes for DELAY.C */
INT CommandDelay (LPTSTR, LPTSTR);


/* Prototypes for DIR.C */
INT CommandDir (LPTSTR, LPTSTR);


/* Prototypes for DIRSTACK.C */
VOID InitDirectoryStack (VOID);
VOID DestroyDirectoryStack (VOID);
INT  GetDirectoryStackDepth (VOID);
INT  CommandPushd (LPTSTR, LPTSTR);
INT  CommandPopd (LPTSTR, LPTSTR);
INT  CommandDirs (LPTSTR, LPTSTR);


/* Prototypes for ECHO.C */
INT  CommandEcho (LPTSTR, LPTSTR);
INT  CommandEchos (LPTSTR, LPTSTR);
INT  CommandEchoerr (LPTSTR, LPTSTR);
INT  CommandEchoserr (LPTSTR, LPTSTR);


/* Prototypes for ERROR.C */
VOID ErrorMessage (DWORD, LPTSTR, ...);

VOID error_no_pipe (VOID);
VOID error_bad_command (VOID);
VOID error_invalid_drive (VOID);
VOID error_req_param_missing (VOID);
VOID error_sfile_not_found (LPTSTR);
VOID error_file_not_found (VOID);
VOID error_path_not_found (VOID);
VOID error_too_many_parameters (LPTSTR);
VOID error_parameter_format(TCHAR);
VOID error_invalid_switch (TCHAR);
VOID error_invalid_parameter_format (LPTSTR);
VOID error_out_of_memory (VOID);
VOID error_syntax (LPTSTR);

VOID msg_pause (VOID);


/* Prototypes for FILECOMP.C */
#ifdef FEATURE_UNIX_FILENAME_COMPLETION
VOID CompleteFilename (LPTSTR, INT);
INT  ShowCompletionMatches (LPTSTR, INT);
#endif
#ifdef FEATURE_4NT_FILENAME_COMPLETION
VOID CompleteFilename (LPTSTR, BOOL, LPTSTR, INT);
#endif


/* Prototypes for FOR.C */
INT cmd_for (LPTSTR, LPTSTR);


/* Prototypes for FREE.C */
INT CommandFree (LPTSTR, LPTSTR);


/* Prototypes for GOTO.C */
INT cmd_goto (LPTSTR, LPTSTR);


/* Prototypes for HISTORY.C */
#ifdef FEATURE_HISTORY
VOID History (INT, LPTSTR);/*add entries browse history*/
VOID History_move_to_bottom(VOID);/*F3*/
VOID InitHistory(VOID);
VOID CleanHistory(VOID);
VOID History_del_current_entry(LPTSTR str);/*CTRL-D*/
INT CommandHistory (LPTSTR cmd, LPTSTR param);
#endif


/* Prototypes for INTERNAL.C */
VOID InitLastPath (VOID);
VOID FreeLastPath (VOID);
INT  cmd_chdir (LPTSTR, LPTSTR);
INT  cmd_mkdir (LPTSTR, LPTSTR);
INT  cmd_rmdir (LPTSTR, LPTSTR);
INT  CommandExit (LPTSTR, LPTSTR);
INT  CommandRem (LPTSTR, LPTSTR);
INT  CommandShowCommands (LPTSTR, LPTSTR);
INT  CommandShowCommandsDetail (LPTSTR, LPTSTR);

/* Prototypes for LABEL.C */
INT cmd_label (LPTSTR, LPTSTR);


/* Prototypes for LOCALE.C */
extern TCHAR cDateSeparator;
extern INT   nDateFormat;
extern TCHAR cTimeSeparator;
extern INT   nTimeFormat;
extern TCHAR cThousandSeparator;
extern TCHAR cDecimalSeparator;
extern INT nNumberGroups;


VOID InitLocale (VOID);
VOID PrintDate (VOID);
VOID PrintTime (VOID);

/* cache codepage */
extern UINT InputCodePage;
extern UINT OutputCodePage;

/* Prototypes for MEMORY.C */
INT CommandMemory (LPTSTR, LPTSTR);


/* Prototypes for MISC.C */
INT GetRootPath(TCHAR *InPath,TCHAR *OutPath,INT size);
BOOL SetRootPath(TCHAR *InPath);
TCHAR  cgetchar (VOID);
BOOL   CheckCtrlBreak (INT);
LPTSTR *split (LPTSTR, LPINT, BOOL);
VOID   freep (LPTSTR *);
LPTSTR _stpcpy (LPTSTR, LPTSTR);
BOOL   IsValidPathName (LPCTSTR);
BOOL   IsExistingFile (LPCTSTR);
BOOL   IsExistingDirectory (LPCTSTR);
BOOL   FileGetString (HANDLE, LPTSTR, INT);
#ifndef __REACTOS__
HWND   GetConsoleWindow(VOID);
#endif

#define PROMPT_NO    0
#define PROMPT_YES   1
#define PROMPT_ALL   2
#define PROMPT_BREAK 3

INT PagePrompt (VOID);
INT FilePromptYN (LPTSTR, ...);
INT FilePromptYNA (LPTSTR, ...);


/* Prototypes for MOVE.C */
INT cmd_move (LPTSTR, LPTSTR);


/* Prototypes for MSGBOX.C */
INT CommandMsgbox (LPTSTR, LPTSTR);


/* Prototypes from PATH.C */
INT cmd_path (LPTSTR, LPTSTR);


/* Prototypes from PROMPT.C */
VOID PrintPrompt (VOID);
INT  cmd_prompt (LPTSTR, LPTSTR);


/* Prototypes for REDIR.C */
#define INPUT_REDIRECTION    1
#define OUTPUT_REDIRECTION   2
#define OUTPUT_APPEND        4
#define ERROR_REDIRECTION    8
#define ERROR_APPEND        16
INT GetRedirection (LPTSTR, LPTSTR, LPTSTR, LPTSTR, LPINT);


/* Prototypes for REN.C */
INT cmd_rename (LPTSTR, LPTSTR);


/* Prototypes for SCREEN.C */
INT CommandScreen (LPTSTR, LPTSTR);


/* Prototypes for SET.C */
INT cmd_set (LPTSTR, LPTSTR);


/* Prototypes for START.C */
INT cmd_start (LPTSTR, LPTSTR);


/* Prototypes for STRTOCLR.C */
BOOL StringToColor (LPWORD, LPTSTR *);


/* Prototypes for TIME.C */
INT cmd_time (LPTSTR, LPTSTR);


/* Prototypes for TIMER.C */
INT CommandTimer (LPTSTR cmd, LPTSTR param);


/* Prototypes for TITLE.C */
INT cmd_title (LPTSTR, LPTSTR);


/* Prototypes for TYPE.C */
INT cmd_type (LPTSTR, LPTSTR);


/* Prototypes for VER.C */
VOID ShortVersion (VOID);
INT  cmd_ver (LPTSTR, LPTSTR);


/* Prototypes for VERIFY.C */
INT cmd_verify (LPTSTR, LPTSTR);


/* Prototypes for VOL.C */
INT cmd_vol (LPTSTR, LPTSTR);


/* Prototypes for WHERE.C */
BOOL SearchForExecutable (LPCTSTR, LPTSTR);

/* Prototypes for WINDOW.C */
INT CommandActivate (LPTSTR, LPTSTR);
INT CommandWindow (LPTSTR, LPTSTR);


/* The MSDOS Batch Commands [MS-DOS 5.0 User's Guide and Reference p359] */
int cmd_if(TCHAR *, TCHAR *);
int cmd_pause(TCHAR *, TCHAR *);
int cmd_shift(TCHAR *, TCHAR *);

#endif /* _CMD_H_INCLUDED_ */
