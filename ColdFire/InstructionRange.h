/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#pragma once
class Instruction;


// range of instructions
struct InstructionRange
{
	InstructionRange(Instruction** from, Instruction** to) : from(from), to(to)
	{}

	Instruction** begin() { return from; }
	Instruction** end() { return to; }

	size_t size() const { return static_cast<size_t>(to - from); }

private:
	Instruction** from;
	Instruction** to;
};
