/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#pragma once
#include "MachineDefs.h"
#include "BasicTypes.h"
class Instruction;


// Instruction map is a simple lookup table from opcode to an 'Instruction' instance
// It's constructed for a given/single ISA

class InstructionMap
{
public:
	InstructionMap(ISA isa);
	~InstructionMap();

	const Instruction* operator [] (uint16 opcode) const	{ return map_[opcode]; }

	// map can be rebuilt for a different ISAs
	void Build(ISA isa);

private:
	std::vector<const Instruction*> map_;
	void BuildMap(const Instruction* instr);
};
