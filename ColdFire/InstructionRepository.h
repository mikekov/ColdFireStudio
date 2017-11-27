/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2013 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#pragma once

#include "Instruction.h"
struct InstructionRange;


// This is repository of all implemented instructions, potentially from many ISAs

class InstructionRepository
{
public:
	Instruction* Register(Instruction* instr);
//	std::size_t Register(Instruction* instructions[], std::size_t count);

	// request all instructions for given ISAs (e.g. ISA_A | MAC, but not ISA_A | ISA_B)
	std::vector<const Instruction*> GetInstructions(ISA isa) const;

private:
	InstructionRange GetInstructionsHelper(ISA isa) const;

	typedef std::map<ISA, std::vector<Instruction*>> Map;
	// collection of known/registered instructions per ISA
	Map collection_;

	InstructionRepository();
	~InstructionRepository();
	InstructionRepository(const InstructionRepository&);
	InstructionRepository& operator = (const InstructionRepository&);

	friend InstructionRepository& GetInstructions();

};


extern InstructionRepository& GetInstructions();
