/*
 *  CMD.C - command-line interface.
 *
 *
 *  History:
 *
 *    17 Jun 1994 (Tim Norman)
 *        started.
 *
 *    08 Aug 1995 (Matt Rains)
 *        I have cleaned up the source code. changes now bring this source
 *        into guidelines for recommended programming practice.
 *
 *        A added the the standard FreeDOS GNU licence test to the
 *        initialize() function.
 *
 *        Started to replace puts() with printf(). this will help
 *        standardize output. please follow my lead.
 *
 *        I have added some constants to help making changes easier.
 *
 *    15 Dec 1995 (Tim Norman)
 *        major rewrite of the code to make it more efficient and add
 *        redirection support (finally!)
 *
 *    06 Jan 1996 (Tim Norman)
 *        finished adding redirection support!  Changed to use our own
 *        exec code (MUCH thanks to Svante Frey!!)
 *
 *    29 Jan 1996 (Tim Norman)
 *        added support for CHDIR, RMDIR, MKDIR, and ERASE, as per
 *        suggestion of Steffan Kaiser
 *
 *        changed "file not found" error message to "bad command or
 *        filename" thanks to Dustin Norman for noticing that confusing
 *        message!
 *
 *        changed the format to call internal commands (again) so that if
 *        they want to split their commands, they can do it themselves
 *        (none of the internal functions so far need that much power, anyway)
 *
 *    27 Aug 1996 (Tim Norman)
 *        added in support for Oliver Mueller's ALIAS command
 *
 *    14 Jun 1997 (Steffan Kaiser)
 *        added ctrl-break handling and error level
 *
 *    16 Jun 1998 (Rob Lake)
 *        Runs command.com if /P is specified in command line.  Command.com
 *        also stays permanent.  If /C is in the command line, starts the
 *        program next in the line.
 *
 *    21 Jun 1998 (Rob Lake)
 *        Fixed up /C so that arguments for the program
 *
 *    08-Jul-1998 (John P. Price)
 *        Now sets COMSPEC environment variable
 *        misc clean up and optimization
 *        added date and time commands
 *        changed to using spawnl instead of exec.  exec does not copy the
 *        environment to the child process!
 *
 *    14 Jul 1998 (Hans B Pufal)
 *        Reorganised source to be more efficient and to more closely
 *        follow MS-DOS conventions. (eg %..% environment variable
 *        replacement works form command line as well as batch file.
 *
 *        New organisation also properly support nested batch files.
 *
 *        New command table structure is half way towards providing a
 *        system in which COMMAND will find out what internal commands
 *        are loaded
 *
 *    24 Jul 1998 (Hans B Pufal) [HBP_003]
 *        Fixed return value when called with /C option
 *
 *    27 Jul 1998  John P. Price
 *        added config.h include
 *
 *    28 Jul 1998  John P. Price
 *        added showcmds function to show commands and options available
 *
 *    07-Aug-1998 (John P Price <linux-guru@gcfl.net>)
 *        Fixed carrage return output to better match MSDOS with echo
 *        on or off. (marked with "JPP 19980708")
 *
 *    07-Dec-1998 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        First ReactOS release.
 *        Extended length of commandline buffers to 512.
 *
 *    13-Dec-1998 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Added COMSPEC environment variable.
 *        Added "/t" support (color) on cmd command line.
 *
 *    07-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Added help text ("cmd /?").
 *
 *    25-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Unicode and redirection safe!
 *        Fixed redirections and piping.
 *        Piping is based on temporary files, but basic support
 *        for anonymous pipes already exists.
 *
 *    27-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Replaced spawnl() by CreateProcess().
 *
 *    22-Oct-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Added break handler.
 *
 *    15-Dec-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Fixed current directory
 *
 *    28-Dec-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Restore window title after program/batch execution
 *
 *    03-Feb-2001 (Eric Kohl <ekohl@rz-online.de>)
 *        Workaround because argc[0] is NULL under ReactOS
 *
 *    23-Feb-2001 (Carl Nettelblad <cnettel@hem.passagen.se>)
 *        %envvar% replacement conflicted with for.
 *
 *    30-Apr-2004 (Filip Navara <xnavara@volny.cz>)
 *       Make MakeSureDirectoryPathExistsEx unicode safe.
 *
 *    28-Mai-2004 (Hartmut Birr)
 *       Removed MakeSureDirectoryPathExistsEx.
 *       Use the current directory if GetTempPath fails.
 *
 *    12-Jul-2004 (Jens Collin <jens.collin@lakhei.com>)
 *       Added ShellExecute call when all else fails to be able to "launch" any file.
 *
 *    02-Apr-2005 (Magnus Olsen <magnus@greatlord.com>)
 *        Remove all hardcode string to En.rc
 *
 *    06-May-2005 (Klemens Friedl <frik85@gmail.com>)
 *        Add 'help' command (list all commands plus description)
 *
 *    06-jul-2005 (Magnus Olsen <magnus@greatlord.com>)
 *        translate '%errorlevel%' to the internal value.
 *        Add proper memmory alloc ProcessInput, the error 
 *        handling for memmory handling need to be improve
 */

#include <precomp.h>
#include "resource.h"

#ifndef NT_SUCCESS
#define NT_SUCCESS(StatCode)  ((NTSTATUS)(StatCode) >= 0)
#endif

typedef NTSTATUS (STDCALL *NtQueryInformationProcessProc)(HANDLE, PROCESSINFOCLASS,
                                                          PVOID, ULONG, PULONG);
typedef NTSTATUS (STDCALL *NtReadVirtualMemoryProc)(HANDLE, PVOID, PVOID, ULONG, PULONG);

BOOL bExit = FALSE;       /* indicates EXIT was typed */
BOOL bCanExit = TRUE;     /* indicates if this shell is exitable */
BOOL bCtrlBreak = FALSE;  /* Ctrl-Break or Ctrl-C hit */
BOOL bIgnoreEcho = FALSE; /* Ignore 'newline' before 'cls' */
INT  nErrorLevel = 0;     /* Errorlevel of last launched external program */
BOOL bChildProcessRunning = FALSE;
DWORD dwChildProcessId = 0;
OSVERSIONINFO osvi;
HANDLE hIn;
HANDLE hOut;
HANDLE hConsole;
HANDLE CMD_ModuleHandle;

static NtQueryInformationProcessProc NtQueryInformationProcessPtr;
static NtReadVirtualMemoryProc       NtReadVirtualMemoryPtr;
static BOOL NtDllChecked = FALSE;

#ifdef INCLUDE_CMD_COLOR
WORD wColor;              /* current color */
WORD wDefColor;           /* default color */
#endif

/*
 *  is character a delimeter when used on first word?
 *
 */

static BOOL IsDelimiter (TCHAR c)
{
	return (c == _T('/') || c == _T('=') || c == _T('\0') || _istspace (c));
}

/*
 * Is a process a console process?
 */
static BOOL IsConsoleProcess(HANDLE Process)
{
	NTSTATUS Status;
	PROCESS_BASIC_INFORMATION Info;
	PEB ProcessPeb;
	ULONG BytesRead;
	HMODULE NtDllModule;

	/* Some people like to run ReactOS cmd.exe on Win98, it helps in the
           build process. So don't link implicitly against ntdll.dll, load it
           dynamically instead */
	if (! NtDllChecked)
	{
		NtDllChecked = TRUE;
		NtDllModule = LoadLibrary(_T("ntdll.dll"));
		if (NULL == NtDllModule)
		{
			/* Probably non-WinNT system. Just wait for the commands
                           to finish. */
			NtQueryInformationProcessPtr = NULL;
			NtReadVirtualMemoryPtr = NULL;
			return TRUE;
		}
		NtQueryInformationProcessPtr = (NtQueryInformationProcessProc)
                                               GetProcAddress(NtDllModule, "NtQueryInformationProcess");
		NtReadVirtualMemoryPtr = (NtReadVirtualMemoryProc)
		                         GetProcAddress(NtDllModule, "NtReadVirtualMemory");
	}

	if (NULL == NtQueryInformationProcessPtr || NULL == NtReadVirtualMemoryPtr)
	{
		return TRUE;
	}

	Status = NtQueryInformationProcessPtr(Process, ProcessBasicInformation,
                                              &Info, sizeof(PROCESS_BASIC_INFORMATION), NULL);
	if (! NT_SUCCESS(Status))
	{
#ifdef _DEBUG
		DebugPrintf (_T("NtQueryInformationProcess failed with status %08x\n"), Status);
#endif
		return TRUE;
	}
	Status = NtReadVirtualMemoryPtr(Process, Info.PebBaseAddress, &ProcessPeb,
                                        sizeof(PEB), &BytesRead);
	if (! NT_SUCCESS(Status) || sizeof(PEB) != BytesRead)
	{
#ifdef _DEBUG
		DebugPrintf (_T("Couldn't read virt mem status %08x bytes read %lu\n"), Status, BytesRead);
#endif
		return TRUE;
	}

	return IMAGE_SUBSYSTEM_WINDOWS_CUI == ProcessPeb.ImageSubSystem;
}



#ifdef _UNICODE
#define SHELLEXECUTETEXT   	"ShellExecuteW"
#else
#define SHELLEXECUTETEXT   	"ShellExecuteA"
#endif

typedef HINSTANCE (WINAPI *MYEX)(
    HWND hwnd,
    LPCTSTR lpOperation,
    LPCTSTR lpFile,
    LPCTSTR lpParameters,
    LPCTSTR lpDirectory,
    INT nShowCmd
);



static BOOL RunFile(LPTSTR filename)
{
	HMODULE 	hShell32;
	MYEX		hShExt;
	HINSTANCE	ret;

#ifdef _DEBUG
		DebugPrintf (_T("RunFile(%s)\n"), filename);
#endif
	hShell32 = LoadLibrary(_T("SHELL32.DLL"));
	if (!hShell32)
	{
#ifdef _DEBUG
		DebugPrintf (_T("RunFile: couldn't load SHELL32.DLL!\n"));
#endif
		return FALSE;
	}

	hShExt = (MYEX)(FARPROC)GetProcAddress(hShell32, SHELLEXECUTETEXT);
	if (!hShExt)
	{
#ifdef _DEBUG
		DebugPrintf (_T("RunFile: couldn't find ShellExecuteA/W in SHELL32.DLL!\n"));
#endif
		FreeLibrary(hShell32);
		return FALSE;
	}

#ifdef _DEBUG
	DebugPrintf (_T("RunFile: ShellExecuteA/W is at %x\n"), hShExt);
#endif

	ret = (hShExt)(NULL, _T("open"), filename, NULL, NULL, SW_SHOWNORMAL);

#ifdef _DEBUG
	DebugPrintf (_T("RunFile: ShellExecuteA/W returned %d\n"), (DWORD)ret);
#endif

	FreeLibrary(hShell32);
	return (((DWORD)ret) > 32);
}



/*
 * This command (in first) was not found in the command table
 *
 * first - first word on command line
 * rest  - rest of command line
 */

static VOID
Execute (LPTSTR full, LPTSTR first, LPTSTR rest)
{
	TCHAR szFullName[MAX_PATH];
#ifndef __REACTOS__
	TCHAR szWindowTitle[MAX_PATH];
#endif
	DWORD dwExitCode = 0;

#ifdef _DEBUG
	DebugPrintf (_T("Execute: \'%s\' \'%s\'\n"), first, rest);
#endif

	/* check for a drive change */
	if ((_istalpha (first[0])) && (!_tcscmp (first + 1, _T(":"))))
	{
		BOOL working = TRUE;
		if (!SetCurrentDirectory(first))
		/* Guess they changed disc or something, handle that gracefully and get to root */
		{
			TCHAR str[4];
			str[0]=first[0];
			str[1]=_T(':');
			str[2]=_T('\\');
			str[3]=0;
			working = SetCurrentDirectory(str);
		}

		if (!working) ConErrResPuts (STRING_FREE_ERROR1);

		return;
	}

	/* get the PATH environment variable and parse it */
	/* search the PATH environment variable for the binary */
	if (!SearchForExecutable (first, szFullName))
	{
		error_bad_command ();
		return;
	}

#ifndef __REACTOS__
	GetConsoleTitle (szWindowTitle, MAX_PATH);
#endif

	/* check if this is a .BAT or .CMD file */
	if (!_tcsicmp (_tcsrchr (szFullName, _T('.')), _T(".bat")) ||
		!_tcsicmp (_tcsrchr (szFullName, _T('.')), _T(".cmd")))
	{
#ifdef _DEBUG
		DebugPrintf (_T("[BATCH: %s %s]\n"), szFullName, rest);
#endif
		Batch (szFullName, first, rest);
	}
	else
	{
		/* exec the program */
		PROCESS_INFORMATION prci;
		STARTUPINFO stui;

#ifdef _DEBUG
		DebugPrintf (_T("[EXEC: %s %s]\n"), full, rest);
#endif
		/* build command line for CreateProcess() */

		/* fill startup info */
		memset (&stui, 0, sizeof (STARTUPINFO));
		stui.cb = sizeof (STARTUPINFO);
		stui.dwFlags = STARTF_USESHOWWINDOW;
		stui.wShowWindow = SW_SHOWDEFAULT;

		// return console to standard mode
		SetConsoleMode (GetStdHandle(STD_INPUT_HANDLE),
		                ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT | ENABLE_ECHO_INPUT );

		if (CreateProcess (szFullName,
		                   full,
		                   NULL,
		                   NULL,
		                   TRUE,
		                   CREATE_NEW_PROCESS_GROUP,
		                   NULL,
		                   NULL,
		                   &stui,
		                   &prci))
		{
			if (IsConsoleProcess(prci.hProcess))
			{
				/* FIXME: Protect this with critical section */
				bChildProcessRunning = TRUE;
				dwChildProcessId = prci.dwProcessId;

				WaitForSingleObject (prci.hProcess, INFINITE);

				/* FIXME: Protect this with critical section */
				bChildProcessRunning = FALSE;

				GetExitCodeProcess (prci.hProcess, &dwExitCode);
				nErrorLevel = (INT)dwExitCode;
			}
			CloseHandle (prci.hThread);
			CloseHandle (prci.hProcess);
		}
		else
		{
#ifdef _DEBUG
			DebugPrintf (_T("[ShellExecute: %s]\n"), full);
#endif
			// See if we can run this with ShellExecute() ie myfile.xls
			if (!RunFile(full))
			{
#ifdef _DEBUG
				DebugPrintf (_T("[ShellExecute failed!: %s]\n"), full);
#endif
				error_bad_command ();
			}
		}
		// restore console mode
		SetConsoleMode( GetStdHandle( STD_INPUT_HANDLE ),
				ENABLE_PROCESSED_INPUT );
	}

	/* Get code page if it has been change */
	InputCodePage= GetConsoleCP();
    OutputCodePage = GetConsoleOutputCP();
#ifndef __REACTOS__
	SetConsoleTitle (szWindowTitle);
#endif
}


/*
 * look through the internal commands and determine whether or not this
 * command is one of them.  If it is, call the command.  If not, call
 * execute to run it as an external program.
 *
 * line - the command line of the program to run
 *
 */

static VOID
DoCommand (LPTSTR line)
{
	TCHAR com[CMDLINE_LENGTH];  /* the first word in the command */
	LPTSTR cp = com;
	LPTSTR cstart;
	LPTSTR rest;   /* pointer to the rest of the command line */
	INT cl;
	LPCOMMAND cmdptr;

#ifdef _DEBUG
	DebugPrintf (_T("DoCommand: (\'%s\')\n"), line);
#endif /* DEBUG */

	/* Skip over initial white space */
	while (_istspace (*line))
		line++;
	rest = line;

	cstart = rest;

	/* Anything to do ? */
	if (*rest)
	{
		if (*rest == _T('"'))
		{
			/* treat quoted words specially */

			rest++;

			while(*rest != _T('\0') && *rest != _T('"'))
				*cp++ = _totlower (*rest++);
			if (*rest == _T('"'))
				rest++;
		}
		else
		{
			while (!IsDelimiter (*rest))
				*cp++ = _totlower (*rest++);
		}


		/* Terminate first word */
		*cp = _T('\0');

		/* commands are limited to MAX_PATH */
		if(_tcslen(com) > MAX_PATH)
		{
		  error_bad_command();
		  return;
		}

		/* Skip over whitespace to rest of line */
		while (_istspace (*rest))
			rest++;

		/* Scan internal command table */
		for (cmdptr = cmds;; cmdptr++)
		{
			/* If end of table execute ext cmd */
			if (cmdptr->name == NULL)
			{
				Execute (line, com, rest);
				break;
			}

			if (!_tcscmp (com, cmdptr->name))
			{
				cmdptr->func (com, rest);
				break;
			}

			/* The following code handles the case of commands like CD which
			 * are recognised even when the command name and parameter are
			 * not space separated.
			 *
			 * e.g dir..
			 * cd\freda
			 */

			/* Get length of command name */
			cl = _tcslen (cmdptr->name);

			if ((cmdptr->flags & CMD_SPECIAL) &&
			    (!_tcsncmp (cmdptr->name, com, cl)) &&
			    (_tcschr (_T("\\.-"), *(com + cl))))
			{
				/* OK its one of the specials...*/

				/* Terminate first word properly */
				com[cl] = _T('\0');

				/* Call with new rest */
				cmdptr->func (com, cstart + cl);
				break;
			}
		}
	}
}


/*
 * process the command line and execute the appropriate functions
 * full input/output redirection and piping are supported
 */

VOID ParseCommandLine (LPTSTR cmd)
{
	TCHAR szMsg[RC_STRING_MAX_SIZE];
	TCHAR cmdline[CMDLINE_LENGTH];
	LPTSTR s;
#ifdef FEATURE_REDIRECTION
	TCHAR in[CMDLINE_LENGTH] = _T("");
	TCHAR out[CMDLINE_LENGTH] = _T("");
	TCHAR err[CMDLINE_LENGTH] = _T("");
	TCHAR szTempPath[MAX_PATH] = _T(".\\");
	TCHAR szFileName[2][MAX_PATH] = {_T(""), _T("")};
	HANDLE hFile[2] = {INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE};
	LPTSTR t = NULL;
	INT  num = 0;
	INT  nRedirFlags = 0;
	INT  Length;
	UINT Attributes;

	HANDLE hOldConIn;
	HANDLE hOldConOut;
	HANDLE hOldConErr;
#endif /* FEATURE_REDIRECTION */

	_tcscpy (cmdline, cmd);
	s = &cmdline[0];

#ifdef _DEBUG
	DebugPrintf (_T("ParseCommandLine: (\'%s\')\n"), s);
#endif /* DEBUG */

#ifdef FEATURE_ALIASES
	/* expand all aliases */
	ExpandAlias (s, CMDLINE_LENGTH);
#endif /* FEATURE_ALIAS */

#ifdef FEATURE_REDIRECTION
	/* find the temp path to store temporary files */
	Length = GetTempPath (MAX_PATH, szTempPath);
	if (Length > 0 && Length < MAX_PATH)
	{
		Attributes = GetFileAttributes(szTempPath);
		if (Attributes == 0xffffffff ||
		    !(Attributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			Length = 0;
		}
	}
	if (Length == 0 || Length >= MAX_PATH)
	{
		_tcscpy(szTempPath, _T(".\\"));
	}
	if (szTempPath[_tcslen (szTempPath) - 1] != _T('\\'))
		_tcscat (szTempPath, _T("\\"));

	/* get the redirections from the command line */
	num = GetRedirection (s, in, out, err, &nRedirFlags);

	/* more efficient, but do we really need to do this? */
	for (t = in; _istspace (*t); t++)
		;
	_tcscpy (in, t);

	for (t = out; _istspace (*t); t++)
		;
	_tcscpy (out, t);

	for (t = err; _istspace (*t); t++)
		;
	_tcscpy (err, t);

	/* Set up the initial conditions ... */
	/* preserve STDIN, STDOUT and STDERR handles */
	hOldConIn  = GetStdHandle (STD_INPUT_HANDLE);
	hOldConOut = GetStdHandle (STD_OUTPUT_HANDLE);
	hOldConErr = GetStdHandle (STD_ERROR_HANDLE);

	/* redirect STDIN */
	if (in[0])
	{
		HANDLE hFile;
		SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};

		hFile = CreateFile (in, GENERIC_READ, FILE_SHARE_READ, &sa, OPEN_EXISTING,
		                    FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile == INVALID_HANDLE_VALUE)
		{
			LoadString(CMD_ModuleHandle, STRING_CMD_ERROR1, szMsg, RC_STRING_MAX_SIZE);
			ConErrPrintf(szMsg, in);
			return;
		}

		if (!SetStdHandle (STD_INPUT_HANDLE, hFile))
		{
			LoadString(CMD_ModuleHandle, STRING_CMD_ERROR1, szMsg, RC_STRING_MAX_SIZE);
			ConErrPrintf(szMsg, in);
			return;
		}
#ifdef _DEBUG
		DebugPrintf (_T("Input redirected from: %s\n"), in);
#endif
	}

	/* Now do all but the last pipe command */
	*szFileName[0] = _T('\0');
	hFile[0] = INVALID_HANDLE_VALUE;

	while (num-- > 1)
	{
		SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};

		/* Create unique temporary file name */
		GetTempFileName (szTempPath, _T("CMD"), 0, szFileName[1]);

		/* Set current stdout to temporary file */
		hFile[1] = CreateFile (szFileName[1], GENERIC_WRITE, 0, &sa,
				       TRUNCATE_EXISTING, FILE_ATTRIBUTE_TEMPORARY, NULL);
		if (hFile[1] == INVALID_HANDLE_VALUE)
		{
			LoadString(CMD_ModuleHandle, STRING_CMD_ERROR2, szMsg, RC_STRING_MAX_SIZE);
			ConErrPrintf(szMsg);
			return;
		}

		SetStdHandle (STD_OUTPUT_HANDLE, hFile[1]);

		DoCommand (s);

		/* close stdout file */
		SetStdHandle (STD_OUTPUT_HANDLE, hOldConOut);
		if ((hFile[1] != INVALID_HANDLE_VALUE) && (hFile[1] != hOldConOut))
		{
			CloseHandle (hFile[1]);
			hFile[1] = INVALID_HANDLE_VALUE;
		}

		/* close old stdin file */
		SetStdHandle (STD_INPUT_HANDLE, hOldConIn);
		if ((hFile[0] != INVALID_HANDLE_VALUE) && (hFile[0] != hOldConIn))
		{
			/* delete old stdin file, if it is a real file */
			CloseHandle (hFile[0]);
			hFile[0] = INVALID_HANDLE_VALUE;
			DeleteFile (szFileName[0]);
			*szFileName[0] = _T('\0');
		}

		/* copy stdout file name to stdin file name */
		_tcscpy (szFileName[0], szFileName[1]);
		*szFileName[1] = _T('\0');

		/* open new stdin file */
		hFile[0] = CreateFile (szFileName[0], GENERIC_READ, 0, &sa,
		                       OPEN_EXISTING, FILE_ATTRIBUTE_TEMPORARY, NULL);
		SetStdHandle (STD_INPUT_HANDLE, hFile[0]);

		s = s + _tcslen (s) + 1;
	}

	/* Now set up the end conditions... */
	/* redirect STDOUT */
	if (out[0])
	{
		/* Final output to here */
		HANDLE hFile;
		SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};

		hFile = CreateFile (out, GENERIC_WRITE, FILE_SHARE_READ, &sa,
		                    (nRedirFlags & OUTPUT_APPEND) ? OPEN_ALWAYS : CREATE_ALWAYS,
		                    FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile == INVALID_HANDLE_VALUE)
		{
			LoadString(CMD_ModuleHandle, STRING_CMD_ERROR3, szMsg, RC_STRING_MAX_SIZE);
			ConErrPrintf(szMsg, out);
			return;
		}

		if (!SetStdHandle (STD_OUTPUT_HANDLE, hFile))
		{
			LoadString(CMD_ModuleHandle, STRING_CMD_ERROR3, szMsg, RC_STRING_MAX_SIZE);
			ConErrPrintf(szMsg, out);
			return;
		}

		if (nRedirFlags & OUTPUT_APPEND)
		{
			LONG lHighPos = 0;

			if (GetFileType (hFile) == FILE_TYPE_DISK)
				SetFilePointer (hFile, 0, &lHighPos, FILE_END);
		}
#ifdef _DEBUG
		DebugPrintf (_T("Output redirected to: %s\n"), out);
#endif
	}
	else if (hOldConOut != INVALID_HANDLE_VALUE)
	{
		/* Restore original stdout */
		HANDLE hOut = GetStdHandle (STD_OUTPUT_HANDLE);
		SetStdHandle (STD_OUTPUT_HANDLE, hOldConOut);
		if (hOldConOut != hOut)
			CloseHandle (hOut);
		hOldConOut = INVALID_HANDLE_VALUE;
	}

	/* redirect STDERR */
	if (err[0])
	{
		/* Final output to here */
		HANDLE hFile;
		SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};

		if (!_tcscmp (err, out))
		{
#ifdef _DEBUG
			DebugPrintf (_T("Stdout and stderr will use the same file!!\n"));
#endif
			DuplicateHandle (GetCurrentProcess (),
			                 GetStdHandle (STD_OUTPUT_HANDLE),
			                 GetCurrentProcess (),
			                 &hFile, 0, TRUE, DUPLICATE_SAME_ACCESS);
		}
		else
		{
			hFile = CreateFile (err,
			                    GENERIC_WRITE,
			                    0,
			                    &sa,
			                    (nRedirFlags & ERROR_APPEND) ? OPEN_ALWAYS : CREATE_ALWAYS,
			                    FILE_ATTRIBUTE_NORMAL,
			                    NULL);
			if (hFile == INVALID_HANDLE_VALUE)
			{
				LoadString(CMD_ModuleHandle, STRING_CMD_ERROR3, szMsg, RC_STRING_MAX_SIZE);
				ConErrPrintf(szMsg, err);
				return;
			}
		}

		if (!SetStdHandle (STD_ERROR_HANDLE, hFile))
		{
			LoadString(CMD_ModuleHandle, STRING_CMD_ERROR3, szMsg, RC_STRING_MAX_SIZE);
			ConErrPrintf(szMsg, err);
			return;
		}

		if (nRedirFlags & ERROR_APPEND)
		{
			LONG lHighPos = 0;

			if (GetFileType (hFile) == FILE_TYPE_DISK)
				SetFilePointer (hFile, 0, &lHighPos, FILE_END);
		}
#ifdef _DEBUG
		DebugPrintf (_T("Error redirected to: %s\n"), err);
#endif
	}
	else if (hOldConErr != INVALID_HANDLE_VALUE)
	{
		/* Restore original stderr */
		HANDLE hErr = GetStdHandle (STD_ERROR_HANDLE);
		SetStdHandle (STD_ERROR_HANDLE, hOldConErr);
		if (hOldConErr != hErr)
			CloseHandle (hErr);
		hOldConErr = INVALID_HANDLE_VALUE;
	}
#endif

	/* process final command */
	DoCommand (s);

#ifdef FEATURE_REDIRECTION
	/* close old stdin file */
#if 0  /* buggy implementation */
	SetStdHandle (STD_INPUT_HANDLE, hOldConIn);
	if ((hFile[0] != INVALID_HANDLE_VALUE) &&
		(hFile[0] != hOldConIn))
	{
		/* delete old stdin file, if it is a real file */
		CloseHandle (hFile[0]);
		hFile[0] = INVALID_HANDLE_VALUE;
		DeleteFile (szFileName[0]);
		*szFileName[0] = _T('\0');
	}

	/* Restore original STDIN */
	if (hOldConIn != INVALID_HANDLE_VALUE)
	{
		HANDLE hIn = GetStdHandle (STD_INPUT_HANDLE);
		SetStdHandle (STD_INPUT_HANDLE, hOldConIn);
		if (hOldConIn != hIn)
			CloseHandle (hIn);
		hOldConIn = INVALID_HANDLE_VALUE;
	}
	else
	{
#ifdef _DEBUG
		DebugPrintf (_T("Can't restore STDIN! Is invalid!!\n"), out);
#endif
	}
#endif  /* buggy implementation */


	if (hOldConIn != INVALID_HANDLE_VALUE)
	{
		HANDLE hIn = GetStdHandle (STD_INPUT_HANDLE);
		SetStdHandle (STD_INPUT_HANDLE, hOldConIn);
		if (hIn == INVALID_HANDLE_VALUE)
		{
#ifdef _DEBUG
			DebugPrintf (_T("Previous STDIN is invalid!!\n"));
#endif
		}
		else
		{
			if (GetFileType (hIn) == FILE_TYPE_DISK)
			{
				if (hFile[0] == hIn)
				{
					CloseHandle (hFile[0]);
					hFile[0] = INVALID_HANDLE_VALUE;
					DeleteFile (szFileName[0]);
					*szFileName[0] = _T('\0');
				}
				else
				{
#ifdef _DEBUG
					DebugPrintf (_T("hFile[0] and hIn dont match!!!\n"));
#endif
				}
			}
		}
	}


	/* Restore original STDOUT */
	if (hOldConOut != INVALID_HANDLE_VALUE)
	{
		HANDLE hOut = GetStdHandle (STD_OUTPUT_HANDLE);
		SetStdHandle (STD_OUTPUT_HANDLE, hOldConOut);
		if (hOldConOut != hOut)
			CloseHandle (hOut);
		hOldConOut = INVALID_HANDLE_VALUE;
	}

	/* Restore original STDERR */
	if (hOldConErr != INVALID_HANDLE_VALUE)
	{
		HANDLE hErr = GetStdHandle (STD_ERROR_HANDLE);
		SetStdHandle (STD_ERROR_HANDLE, hOldConErr);
		if (hOldConErr != hErr)
			CloseHandle (hErr);
		hOldConErr = INVALID_HANDLE_VALUE;
	}
#endif /* FEATURE_REDIRECTION */
}


/*
 * do the prompt/input/process loop
 *
 */

static INT
ProcessInput (BOOL bFlag)
{
	TCHAR commandline[CMDLINE_LENGTH];
	TCHAR readline[CMDLINE_LENGTH];
	LPTSTR tp = NULL;
	LPTSTR ip;
	LPTSTR cp;
	BOOL bEchoThisLine;


	do
	{
		/* if no batch input then... */
		if (!(ip = ReadBatchLine (&bEchoThisLine)))
		{
			if (bFlag)
				return 0;

			ReadCommand (readline, CMDLINE_LENGTH);
			ip = readline;
			bEchoThisLine = FALSE;
		}

		cp = commandline;
		while (*ip)
		{
			if (*ip == _T('%'))
			{
				switch (*++ip)
				{
					case _T('%'):
						*cp++ = *ip++;
						break;

					case _T('0'):
					case _T('1'):
					case _T('2'):
					case _T('3'):
					case _T('4'):
					case _T('5'):
					case _T('6'):
					case _T('7'):
					case _T('8'):
					case _T('9'):
						if ((tp = FindArg (*ip - _T('0'))))
						{
							cp = _stpcpy (cp, tp);
							ip++;
						}
						else
							*cp++ = _T('%');
						break;

					case _T('?'):
						cp += _stprintf (cp, _T("%u"), nErrorLevel);
						ip++;
						break;

					default:
						tp = _tcschr(ip, _T('%'));
						if ((tp != NULL) &&
						    (tp <= _tcschr(ip, _T(' ')) - 1))
						{
              INT size = 512;
							TCHAR *evar;
							*tp = _T('\0');

              /* FIXME: Correct error handling when it can not alloc memmory */
                          	
              /* %CD% */
              if (_tcsicmp(ip,_T("cd")) ==0)
              {
                TCHAR szPath[MAX_PATH];
		            GetCurrentDirectory (MAX_PATH, szPath);
                cp = _stpcpy (cp, szPath);                 
              }

              /* %TIME% */
              else if (_tcsicmp(ip,_T("time")) ==0)
              {
                TCHAR szTime[40];                                                               
                GetTimeFormat(LOCALE_USER_DEFAULT, 0, NULL, NULL, szTime, sizeof(szTime));              
                cp = _stpcpy (cp, szTime);                 	              
              }
              
              /* %DATE% */
              else if (_tcsicmp(ip,_T("date")) ==0)
              {
              TCHAR szDate[40];

	            GetDateFormat(LOCALE_USER_DEFAULT, 0, NULL, _T("ddd"), szDate, sizeof (szDate));
              cp = _stpcpy (cp, szDate);        
              cp = _stpcpy (cp, _T(" "));        
              GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, NULL, NULL, szDate, sizeof (szDate));
              cp = _stpcpy (cp, szDate);        
              }

              /* %RANDOM% */
              else if (_tcsicmp(ip,_T("random")) ==0)
              {
               TCHAR szRand[40];               
               /* Get random number */
               _itot(rand(),szRand,10);
               cp = _stpcpy (cp, szRand); 
              }
               
              /* %CMDCMDLINE% */
              else if (_tcsicmp(ip,_T("cmdcmdline")) ==0)
              {                      
               TCHAR *pargv;               
               /* Get random number */        
               pargv = GetCommandLine();
               cp = _stpcpy (cp, pargv); 
              }

         
              /* %ERRORLEVEL% */
              else if (_tcsicmp(ip,_T("errorlevel")) ==0)
              {
                evar = malloc ( size * sizeof(TCHAR));
                if (evar==NULL) 
                    return 1; 

                memset(evar,0,512 * sizeof(TCHAR));
                _itot(nErrorLevel,evar,10);        
                cp = _stpcpy (cp, evar);

                free(evar);
              }               
							else 
              {
                evar = malloc ( size * sizeof(TCHAR));
                if (evar==NULL) 
                    return 1; 

                size = GetEnvironmentVariable (ip, evar, size);
                if (size!=0)
                {
                    evar = realloc(evar,size * sizeof(TCHAR) );
                    if (evar!=NULL)
                    {
                      size = GetEnvironmentVariable (ip, evar, size);
                    }
                }

                if (size)
                {
								 cp = _stpcpy (cp, evar);
                }

                free(evar);
              }
               
							ip = tp + 1;

						}
						else
						{
							*cp++ = _T('%');
						}              
           
						break;
				}
				continue;
			}


     

			if (_istcntrl (*ip))
				*ip = _T(' ');
			*cp++ = *ip++;
		}

		*cp = _T('\0');

		/* strip trailing spaces */
		while ((--cp >= commandline) && _istspace (*cp));     

		*(cp + 1) = _T('\0');
   
		/* JPP 19980807 */
		/* Echo batch file line */
		if (bEchoThisLine)
		{
			PrintPrompt ();
			ConOutPuts (commandline);
		}

		if (*commandline)
		{
			ParseCommandLine (commandline);
			if (bEcho && !bIgnoreEcho)
				ConOutChar ('\n');
			bIgnoreEcho = FALSE;
		}
	}
	while (!bCanExit || !bExit);

	return 0;
}


/*
 * control-break handler.
 */
BOOL WINAPI BreakHandler (DWORD dwCtrlType)
{

	if ((dwCtrlType != CTRL_C_EVENT) &&
	    (dwCtrlType != CTRL_BREAK_EVENT))
		return FALSE;

	if (bChildProcessRunning == TRUE)
	{
		GenerateConsoleCtrlEvent (CTRL_C_EVENT,
		                          dwChildProcessId);
		return TRUE;
	}

	/* FIXME: Handle batch files */

	/* FIXME: Print "^C" */


	return TRUE;
}


VOID AddBreakHandler (VOID)
{
	SetConsoleCtrlHandler ((PHANDLER_ROUTINE)BreakHandler, TRUE);
}


VOID RemoveBreakHandler (VOID)
{
	SetConsoleCtrlHandler ((PHANDLER_ROUTINE)BreakHandler, FALSE);
}


/*
 * show commands and options that are available.
 *
 */
#if 0
static VOID
ShowCommands (VOID)
{
	/* print command list */
	ConOutResPuts(STRING_CMD_HELP1);
	PrintCommandList();

	/* print feature list */
	ConOutResPuts(STRING_CMD_HELP2);

#ifdef FEATURE_ALIASES
	ConOutResPuts(STRING_CMD_HELP3);
#endif
#ifdef FEATURE_HISTORY
	ConOutResPuts(STRING_CMD_HELP4);
#endif
#ifdef FEATURE_UNIX_FILENAME_COMPLETION
	ConOutResPuts(STRING_CMD_HELP5);
#endif
#ifdef FEATURE_DIRECTORY_STACK
	ConOutResPuts(STRING_CMD_HELP6);
#endif
#ifdef FEATURE_REDIRECTION
	ConOutResPuts(STRING_CMD_HELP7);
#endif
	ConOutChar(_T('\n'));
}
#endif

/*
 * set up global initializations and process parameters
 *
 * argc - number of parameters to command.com
 * argv - command-line parameters
 *
 */
static VOID
Initialize (int argc, TCHAR* argv[])
{
	TCHAR commandline[CMDLINE_LENGTH];
	TCHAR ModuleName[_MAX_PATH + 1];
	INT i;

	//INT len;
	//TCHAR *ptr, *cmdLine;


#ifdef _DEBUG
	INT x;

	DebugPrintf (_T("[command args:\n"));
	for (x = 0; x < argc; x++)
	{
		DebugPrintf (_T("%d. %s\n"), x, argv[x]);
	}
	DebugPrintf (_T("]\n"));
#endif

	/* get version information */
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx (&osvi);

	InitLocale ();

	/* get default input and output console handles */
	hOut = GetStdHandle (STD_OUTPUT_HANDLE);
	hIn  = GetStdHandle (STD_INPUT_HANDLE);


	if (argc >= 2 && !_tcsncmp (argv[1], _T("/?"), 2))
	{
		ConOutResPaging(TRUE,STRING_CMD_HELP8);
		ExitProcess(0);
	}
	SetConsoleMode (hIn, ENABLE_PROCESSED_INPUT);

#ifdef INCLUDE_CMD_CHDIR
	InitLastPath ();
#endif

#ifdef FATURE_ALIASES
	InitializeAlias ();
#endif

	if (argc >= 2)
	{
		for (i = 1; i < argc; i++)
		{
			if (!_tcsicmp (argv[i], _T("/p")))
			{
				if (!IsExistingFile (_T("\\autoexec.bat")))
				{
#ifdef INCLUDE_CMD_DATE
					cmd_date (_T(""), _T(""));
#endif
#ifdef INCLUDE_CMD_TIME
					cmd_time (_T(""), _T(""));
#endif
				}
				else
				{
					ParseCommandLine (_T("\\autoexec.bat"));
				}
				bCanExit = FALSE;
			}
			else if (!_tcsicmp (argv[i], _T("/c")))
			{
				/* This just runs a program and exits */
				++i;
				if (i < argc)
				{
					_tcscpy (commandline, argv[i]);
					while (++i < argc)
					{
						_tcscat (commandline, _T(" "));
						_tcscat (commandline, argv[i]);
					}

					ParseCommandLine(commandline);
					ExitProcess (ProcessInput (TRUE));
				}
				else
				{
					ExitProcess (0);
				}
			}
			else if (!_tcsicmp (argv[i], _T("/k")))
			{
				/* This just runs a program and remains */
				++i;
				if (i < argc)
				{
					_tcscpy (commandline, argv[i]);
					while (++i < argc)
					{
						_tcscat (commandline, _T(" "));
						_tcscat (commandline, argv[i]);
					}

					ParseCommandLine(commandline);
				}
			}
#ifdef INCLUDE_CMD_COLOR
			else if (!_tcsnicmp (argv[i], _T("/t:"), 3))
			{
				/* process /t (color) argument */
				wDefColor = (WORD)_tcstoul (&argv[i][3], NULL, 16);
				wColor = wDefColor;
				SetScreenColor (wColor, TRUE);
			}
#endif
		}
	}

	/* run cmdstart.bat */
	if (IsExistingFile (_T("cmdstart.bat")))
	{
		ParseCommandLine (_T("cmdstart.bat"));
	}
	else if (IsExistingFile (_T("\\cmdstart.bat")))
	{
		ParseCommandLine (_T("\\cmdstart.bat"));
	}
#ifndef __REACTOS__
	else
	{
		/* try to run cmdstart.bat from install dir */
		LPTSTR p;

		_tcscpy (commandline, argv[0]);
		p = _tcsrchr (commandline, _T('\\')) + 1;
		_tcscpy (p, _T("cmdstart.bat"));

		if (IsExistingFile (_T("commandline")))
		{
			LoadString(CMD_ModuleHandle, STRING_CMD_ERROR4, szMsg, RC_STRING_MAX_SIZE);
			ConErrPrintf(szMsg, commandline);
			ParseCommandLine (commandline);
		}
	}
#endif

#ifdef FEATURE_DIR_STACK
	/* initialize directory stack */
	InitDirectoryStack ();
#endif


#ifdef FEATURE_HISTORY
	/*initialize history*/
	InitHistory();
#endif

	/* Set COMSPEC environment variable */
	if (0 != GetModuleFileName (NULL, ModuleName, _MAX_PATH + 1))
	{
		ModuleName[_MAX_PATH] = _T('\0');
		SetEnvironmentVariable (_T("COMSPEC"), ModuleName);
	}

	/* add ctrl break handler */
	AddBreakHandler ();
}


static VOID Cleanup (int argc, TCHAR *argv[])
{
#ifndef __REACTOS__
	TCHAR szMsg[RC_STRING_MAX_SIZE];
#endif

	/* run cmdexit.bat */
	if (IsExistingFile (_T("cmdexit.bat")))
	{
		ConErrResPuts(STRING_CMD_ERROR5);

		ParseCommandLine (_T("cmdexit.bat"));
	}
	else if (IsExistingFile (_T("\\cmdexit.bat")))
	{
		ConErrResPuts (STRING_CMD_ERROR5);
		ParseCommandLine (_T("\\cmdexit.bat"));
	}
#ifndef __REACTOS__
	else
	{
		/* try to run cmdexit.bat from install dir */
		TCHAR commandline[CMDLINE_LENGTH];
		LPTSTR p;

		_tcscpy (commandline, argv[0]);
		p = _tcsrchr (commandline, _T('\\')) + 1;
		_tcscpy (p, _T("cmdexit.bat"));

		if (IsExistingFile (_T("commandline")))
		{
			LoadString(CMD_ModuleHandle, STRING_CMD_ERROR4, szMsg, RC_STRING_MAX_SIZE);
			ConErrPrintf(szMsg, commandline);
			ParseCommandLine (commandline);
		}
	}
#endif

#ifdef FEATURE_ALIASES
	DestroyAlias ();
#endif

#ifdef FEATURE_DIECTORY_STACK
	/* destroy directory stack */
	DestroyDirectoryStack ();
#endif

#ifdef INCLUDE_CMD_CHDIR
	FreeLastPath ();
#endif

#ifdef FEATURE_HISTORY
	CleanHistory();
#endif


	/* remove ctrl break handler */
	RemoveBreakHandler ();
	SetConsoleMode( GetStdHandle( STD_INPUT_HANDLE ),
			ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT | ENABLE_ECHO_INPUT );
}

#ifdef __REACTOS__
#ifdef _UNICODE
PWCHAR * _CommandLineToArgvW(PWCHAR lpCmdLine, int *pNumArgs)
{
   PWCHAR * argvw = NULL;
   PWCHAR ptr = lpCmdLine;
   PWCHAR str;
   int len;
   int NumArgs;

   NumArgs = 0;

   while(lpCmdLine && *lpCmdLine)
   {
       while (iswspace(*lpCmdLine)) lpCmdLine++;
       if (*lpCmdLine)
       {
	   if ((NumArgs % 10)==0)
	   {
               PWCHAR * old_argvw = argvw;
	       argvw = malloc((NumArgs + 10) * sizeof(PWCHAR));
	       memcpy(argvw, old_argvw, NumArgs * sizeof(PWCHAR));
	       free(old_argvw);
	   }
	   ptr = wcschr(lpCmdLine, L' ');
	   if (ptr)
	   {
	       len = ptr - lpCmdLine;
	   }
	   else
	   {
	       len = wcslen(lpCmdLine);
	   }
	   str = malloc((len + 1) * sizeof(WCHAR));
	   memcpy(str, lpCmdLine, len * sizeof(WCHAR));
	   str[len] = 0;
	   argvw[NumArgs]=str;
	   NumArgs++;
	   lpCmdLine = ptr;
       }
   }
   *pNumArgs = NumArgs;
   return argvw;
}
#endif
#endif

/*
 * main function
 */
#ifdef _UNICODE
int main(void)
#else
int main (int argc, char *argv[])
#endif
{
  CONSOLE_SCREEN_BUFFER_INFO Info;
  INT nExitCode;
#ifdef _UNICODE
  PWCHAR * argv;
  int argc=0;
#ifdef __REACTOS__
  argv = _CommandLineToArgvW(GetCommandLineW(), &argc);
#else
  argv = CommandLineToArgvW(GetCommandLineW(), &argc);
#endif
#endif
     

  SetFileApisToOEM();
  InputCodePage= 0;
  OutputCodePage = 0;

  hConsole = CreateFile(_T("CONOUT$"), GENERIC_READ|GENERIC_WRITE,
                        FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
			OPEN_EXISTING, 0, NULL);
  if (GetConsoleScreenBufferInfo(hConsole, &Info) == FALSE)
    {
      ConOutFormatMessage(GetLastError());
      return(1);
    }
  wColor = Info.wAttributes;
  wDefColor = wColor;

  InputCodePage= GetConsoleCP();
  OutputCodePage = GetConsoleOutputCP();
  CMD_ModuleHandle = GetModuleHandle(NULL);
  
  /* check switches on command-line */
  Initialize(argc, argv);

  /* call prompt routine */
  nExitCode = ProcessInput(FALSE);

  
  /* do the cleanup */
  Cleanup(argc, argv);

  
  return(nExitCode);
}

/* EOF */
