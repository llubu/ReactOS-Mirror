/* $Id$
 *
 * PROJECT:     System regression tool for ReactOS
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        tools/sysreg/conf_parser.h
 * PURPOSE:     environment variable lookup
 * PROGRAMMERS: Johannes Anderwald (johannes.anderwald at sbox tugraz at)
 */


#include "env_var.h"
#include <iostream>

namespace System_
{
	using std::cout;
	using std::endl;
	using std::cerr;

//---------------------------------------------------------------------------------------
	EnvironmentVariable::EnvironmentVariable() 
	{

	}

//---------------------------------------------------------------------------------------
	EnvironmentVariable::~EnvironmentVariable()
	{
		m_Map.clear ();
	}

//---------------------------------------------------------------------------------------

	bool EnvironmentVariable::getValue(const System_::string &EnvName, System_::string &EnvValue) 
	{
		EnvironmentMap::const_iterator it = m_Map.find (EnvName);
		if (it != m_Map.end())
		{
			EnvValue = it->second;
			return true;
		}
		
		TCHAR * value = _tgetenv(EnvName.c_str ());
		
		if (!value)
		{
#ifdef NDEBUG
			cerr << "EnvironmentVariable::getValue found no value for " << EnvName << endl;
#endif
			return false;
		}

		if (!_tcslen(value))
		{
#ifdef NDEBUG
			cerr << "EnvironmentVariable::getValue found no value for " << EnvName << endl;
#endif
			return false;
		}

		EnvValue = value;
		m_Map[EnvName] = EnvValue;
		return true;
	}


} // end of namespace System_
