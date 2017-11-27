/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#pragma once
#include "ErrCodes.h"


class Stat
{
public:
	Stat() : code_(OK)
	{}
	Stat(StatCode code) : code_(code)
	{}
	Stat(StatCode code, const char* msg) : code_(code), msg_(msg)
	{}

	operator StatCode () const	{ return code_; }

	const char* message() const	{ return msg_.c_str(); }

private:
	StatCode code_;
	FixedString msg_;
};
