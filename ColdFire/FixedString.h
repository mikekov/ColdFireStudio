/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#pragma once

#if 0	// const string offers small speed improvement in CF macro assembler

#include "../const_string/const_string.hpp"
typedef boost::const_string<char> FixedString;

#include <xstddef>

namespace std {
template<>
struct hash<FixedString>
{
	size_t operator()(const FixedString& _keyval) const
	{
		return std::hash<char*>();
		return _Hash_seq(reinterpret_cast<const unsigned char*>(_keyval.c_str()), _keyval.size());
	}
};

}


#else

#include <string>
typedef std::string FixedString;

#endif
