/* $Id: console.c,v 1.1 2003/03/20 19:19:22 rcampbell Exp $
 *
 *  CONSOLE.C - console input/output functions.
 *
 *
 *  History:
 *
 *    20-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        started
 */

#include "config.h"

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "cmd.h"


#define OUTPUT_BUFFER_SIZE  4096


#ifdef _DEBUG
VOID DebugPrintf (LPTSTR szFormat, ...)
{
	TCHAR szOut[OUTPUT_BUFFER_SIZE];
	va_list arg_ptr;
	DWORD dwWritten;

	va_start (arg_ptr, szFormat);
	_vstprintf (szOut, szFormat, arg_ptr);
	va_end (arg_ptr);

	WriteFile (GetStdHandle (STD_ERROR_HANDLE),
	           szOut,
	           _tcslen(szOut) * sizeof(TCHAR),
	           &dwWritten,
	           NULL);
#if 0
	OutputDebugString (szOut);
#endif
}
#endif /* _DEBUG */


VOID ConInDisable (VOID)
{
	HANDLE hInput = GetStdHandle (STD_INPUT_HANDLE);
	DWORD dwMode;

	GetConsoleMode (hInput, &dwMode);
	dwMode &= ~ENABLE_PROCESSED_INPUT;
	SetConsoleMode (hInput, dwMode);
}


VOID ConInEnable (VOID)
{
	HANDLE hInput = GetStdHandle (STD_INPUT_HANDLE);
	DWORD dwMode;

	GetConsoleMode (hInput, &dwMode);
	dwMode |= ENABLE_PROCESSED_INPUT;
	SetConsoleMode (hInput, dwMode);
}


VOID ConInDummy (VOID)
{
	HANDLE hInput = GetStdHandle (STD_INPUT_HANDLE);
	INPUT_RECORD dummy;
	DWORD  dwRead;

#ifdef _DEBUG
	if (hInput == INVALID_HANDLE_VALUE)
		DebugPrintf ("Invalid input handle!!!\n");
#endif /* _DEBUG */

	ReadConsoleInput (hInput, &dummy, 1, &dwRead);
}

VOID ConInFlush (VOID)
{
	FlushConsoleInputBuffer (GetStdHandle (STD_INPUT_HANDLE));
}


VOID ConInKey (PINPUT_RECORD lpBuffer)
{
	HANDLE hInput = GetStdHandle (STD_INPUT_HANDLE);
	DWORD  dwRead;

#ifdef _DEBUG
	if (hInput == INVALID_HANDLE_VALUE)
		DebugPrintf ("Invalid input handle!!!\n");
#endif /* _DEBUG */

	do
	{
		ReadConsoleInput (hInput, lpBuffer, 1, &dwRead);
		if ((lpBuffer->EventType == KEY_EVENT) &&
			(lpBuffer->Event.KeyEvent.bKeyDown == TRUE))
			break;
	}
	while (TRUE);
}



VOID ConInString (LPTSTR lpInput, DWORD dwLength)
{
	DWORD dwOldMode;
	DWORD dwRead;
	HANDLE hFile;

	LPTSTR p;
	DWORD  i;

	ZeroMemory (lpInput, dwLength * sizeof(TCHAR));
	hFile = GetStdHandle (STD_INPUT_HANDLE);
	GetConsoleMode (hFile, &dwOldMode);

	SetConsoleMode (hFile, ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);

	ReadFile (hFile, lpInput, dwLength, &dwRead, NULL);

	p = lpInput;
	for (i = 0; i < dwRead; i++, p++)
	{
		if (*p == _T('\x0d'))
		{
			*p = _T('\0');
			break;
		}
	}

	SetConsoleMode (hFile, dwOldMode);
}


VOID ConOutChar (TCHAR c)
{
	DWORD dwWritten;

	WriteFile (GetStdHandle (STD_OUTPUT_HANDLE),
	           &c,
	           sizeof(TCHAR),
	           &dwWritten,
	           NULL);
}


VOID ConOutPuts (LPTSTR szText)
{
	DWORD dwWritten;

	WriteFile (GetStdHandle (STD_OUTPUT_HANDLE),
	           szText,
	           _tcslen(szText) * sizeof(TCHAR),
	           &dwWritten,
	           NULL);
	WriteFile (GetStdHandle (STD_OUTPUT_HANDLE),
	           _T("\n"),
	           sizeof(TCHAR),
	           &dwWritten,
	           NULL);
}


VOID ConOutPrintf (LPTSTR szFormat, ...)
{
	TCHAR szOut[OUTPUT_BUFFER_SIZE];
	DWORD dwWritten;
	va_list arg_ptr;

	va_start (arg_ptr, szFormat);
	_vstprintf (szOut, szFormat, arg_ptr);
	va_end (arg_ptr);

	WriteFile (GetStdHandle (STD_OUTPUT_HANDLE),
	           szOut,
	           _tcslen(szOut) * sizeof(TCHAR),
	           &dwWritten,
	           NULL);
}


VOID ConErrChar (TCHAR c)
{
	DWORD dwWritten;

	WriteFile (GetStdHandle (STD_ERROR_HANDLE),
	           &c,
	           sizeof(TCHAR),
	           &dwWritten,
	           NULL);
}


VOID ConErrPuts (LPTSTR szText)
{
	DWORD dwWritten;

	WriteFile (GetStdHandle (STD_ERROR_HANDLE),
	           szText,
	           _tcslen(szText) * sizeof(TCHAR),
	           &dwWritten,
	           NULL);
	WriteFile (GetStdHandle (STD_ERROR_HANDLE),
	           _T ("\n"),
	           sizeof(TCHAR),
	           &dwWritten,
	           NULL);
}


VOID ConErrPrintf (LPTSTR szFormat, ...)
{
	TCHAR szOut[OUTPUT_BUFFER_SIZE];
	DWORD dwWritten;
	va_list arg_ptr;

	va_start (arg_ptr, szFormat);
	_vstprintf (szOut, szFormat, arg_ptr);
	va_end (arg_ptr);

	WriteFile (GetStdHandle (STD_ERROR_HANDLE),
	           szOut,
	           _tcslen(szOut) * sizeof(TCHAR),
	           &dwWritten,
	           NULL);
}


VOID SetCursorXY (SHORT x, SHORT y)
{
	COORD coPos;

	coPos.X = x;
	coPos.Y = y;
	SetConsoleCursorPosition (GetStdHandle (STD_OUTPUT_HANDLE), coPos);
}


VOID GetCursorXY (PSHORT x, PSHORT y)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;

	GetConsoleScreenBufferInfo (GetStdHandle (STD_OUTPUT_HANDLE), &csbi);

	*x = csbi.dwCursorPosition.X;
	*y = csbi.dwCursorPosition.Y;
}


SHORT GetCursorX (VOID)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;

	GetConsoleScreenBufferInfo (GetStdHandle (STD_OUTPUT_HANDLE), &csbi);

	return csbi.dwCursorPosition.X;
}


SHORT GetCursorY (VOID)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;

	GetConsoleScreenBufferInfo (GetStdHandle (STD_OUTPUT_HANDLE), &csbi);

	return csbi.dwCursorPosition.Y;
}


VOID GetScreenSize (PSHORT maxx, PSHORT maxy)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;

	GetConsoleScreenBufferInfo (GetStdHandle (STD_OUTPUT_HANDLE), &csbi);

	if (maxx)
		*maxx = csbi.dwSize.X;
	if (maxy)
		*maxy = csbi.dwSize.Y;
}


VOID SetCursorType (BOOL bInsert, BOOL bVisible)
{
	CONSOLE_CURSOR_INFO cci;

	cci.dwSize = bInsert ? 10 : 99;
	cci.bVisible = bVisible;

	SetConsoleCursorInfo (GetStdHandle (STD_OUTPUT_HANDLE), &cci);
}

/* EOF */
