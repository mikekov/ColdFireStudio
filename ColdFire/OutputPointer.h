/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#pragma once
#ifndef _output_ptr_h_
#define _output_ptr_h_

#include "MachineDefs.h"
#include "BasicTypes.h"
#include "BinaryProgram.h"


// helper class to track origin (program counter) during assembly passes
//
class ProgramOrigin
{
public:
	ProgramOrigin();

	void Set(uint32 start);
	uint32 Origin() const;

	operator uint32 () const;

	void Advance(int32 n);
	void Advance(uint32 n);

	bool Missing() const;
	void Reset();

private:
	bool ready_;
	uint32 pc_;	// program counter
};


// helper class to store generated code in 'BinaryProgram'
//
class OutputPointer
{
public:
	OutputPointer(const ProgramOrigin& origin, cf::BinaryProgram& code);

	OutputPointer& operator << (uint16 word);

	uint32 Origin() const;

private:
	ProgramOrigin origin_;
	cf::BinaryProgram& code_;
};


#endif
