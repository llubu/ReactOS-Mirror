#ifndef NAMEDPIPE_READER_H__
#define NAMEDPIPE_READER_H__

/* $Id: namedpipe_reader.h 24643 2006-10-24 11:45:21Z janderwald $
 *
 * PROJECT:     System regression tool for ReactOS
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        tools/sysreg/namedpipe_reader.h
 * PURPOSE:     file reading support
 * PROGRAMMERS: Johannes Anderwald (johannes.anderwald at sbox tugraz at)
 *				Christoph von Wittich (Christoph@ApiViewer.de)
 */



#include "user_types.h"
#include <vector>
#include <stdio.h>
#include <stdlib.h>

#ifdef __LINUX__

#elif defined(WIN32)
	#include <windows.h>
#endif

namespace System_
{
//---------------------------------------------------------------------------------------
///
/// class NamedPipeReader
///
/// Description: this class implements a named pipe reader. 

	class NamedPipeReader
	{
	public:

//---------------------------------------------------------------------------------------
///
/// NamedPipeReader
///
/// Description: constructor of class PipeReader

		NamedPipeReader();

//---------------------------------------------------------------------------------------
///
/// virtual ~NamedPipeReader
///
/// Description: destructor of class PipeReader

		virtual ~NamedPipeReader();

//---------------------------------------------------------------------------------------
///
/// openPipe
///
/// Description: this function attempts to open a pipe. If an pipe is already open or
/// it fails to open a pipe, the function returns false
///
/// @param PipeCmd command of the pipe to open
///
/// @return bool

		bool openPipe(const string & PipeCmd);

//---------------------------------------------------------------------------------------
///
/// closePipe
///
/// Description: closes a pipe. Returns true on success
///
/// @return bool

		bool closePipe();

//---------------------------------------------------------------------------------------
///
/// readPipe
///
/// Description: attempts to read from the pipe. Returns true on success. If it returns
/// false, call PipeReader::isEoF() to determine if the pipe should be closed
///
/// @param Buffer to be written to
/// @return size_t

		size_t readPipe(std::vector<string> & vect);

//---------------------------------------------------------------------------------------
///
/// isEof
///
/// Description: returns true if the pipe has reached end of file. The caller should call
/// closePipe if this function returns true

	bool isEof();

protected:
//---------------------------------------------------------------------------------------
///
/// extractLines
///
/// Description: this functions extract lines from a buffer and pushes them into the supplied vector
///
/// @param buffer from which the lines are extracted from
/// @param vect vector storing the extracted lines
/// @param append_line if the line isnt fully read, the line is appended

	void extractLines(TCHAR * buffer, std::vector<string> & vect, bool & append_line, unsigned long cbRead);




	HANDLE h_Pipe;

	}; // end of class NamedPipeReader

} // end of namespace System_



#endif /* end of NAMEDPIPE_READER_H__ */
