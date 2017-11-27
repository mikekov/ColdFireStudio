/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#pragma once
#include <exception>


// Low-level exception. Internal program error:
// coding error, wrong instruction addressing modes, or opcodes, etc.

class LogicError : public std::exception
{
public:
	LogicError(const char* msg) : exception(msg)
	{}
};


// low-level exception. Internal error, this time due to run time conditions

class RunTimeError : public std::exception
{
public:
	RunTimeError(const char* msg) : exception(msg)
	{}

	RunTimeError(const std::string& msg) : exception(msg.c_str())
	{}
};
