/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2013 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "InstructionRepository.h"
#include "InstructionRange.h"


extern InstructionRepository& GetInstructions()
{
	static InstructionRepository* ir= new InstructionRepository();
	return *ir;
}


InstructionRepository::InstructionRepository()
{}


InstructionRepository::~InstructionRepository()
{}


Instruction* InstructionRepository::Register(Instruction* instr)
{
	if (instr == 0)
		throw LogicError("empty instruction pointer cannot be registered " __FUNCTION__);

	// Populate collection of defined instructions
	//
	// collection_[ISA A] contains only ISA A instructions
	// collection_[ISA B] contains ISA A, B instructions
	// collection_[ISA C] contains ISA A, B, C instructions
	//
	// note: instruction from ISA_A can be missing in ISA_B/C maps if ExcludeFromOtherIsas says so

	// in which ISA did instruction first show up?
	ISA isa= instr->DefinedInIsa();
	bool exclude_other_isas= instr->ExcludeFromOtherIsas();

	// add instruction to maps for all ISAs greater than equal to 'isa'
	// unless other architectures are explicitly excluded

	if (exclude_other_isas)
		collection_[isa].push_back(instr);
	else
	{
		// ISA A goes to the maps for A, B, C
		// ISA B goes to the maps for B, C
		// ISA C goes to the C map
		// ISA A+ is not implemented/handled yet

		switch (isa)
		{
		case ISA::A:
			collection_[ISA::A].push_back(instr);
			collection_[ISA::B].push_back(instr);
			collection_[ISA::C].push_back(instr);
			collection_[ISA::A_PLUS].push_back(instr);
			break;

		case ISA::B:
			collection_[ISA::B].push_back(instr);
			collection_[ISA::C].push_back(instr);
			break;

		case ISA::C:
			collection_[ISA::C].push_back(instr);
			break;

		case ISA::A_PLUS:
			//TODO: ISA_A_PLUS
			assert(false);	// logic is not implemented
			break;

		case ISA::MAC:
		case ISA::EMAC:
		case ISA::FPU:
			// put MAC/EMAC/FPU instruction into its own bucket
			collection_[isa].push_back(instr);
			break;
		}
	}

	return instr;
}


InstructionRange InstructionRepository::GetInstructionsHelper(ISA isa) const
{
	auto it= collection_.find(isa);

	Instruction** begin= 0;
	Instruction** end= 0;

	if (it != collection_.end() && !it->second.empty())
	{
		auto& vect= it->second;
		begin = const_cast<Instruction**>(&vect.front());
		end = const_cast<Instruction**>(&vect.back() + 1);
	}

	return InstructionRange(begin, end);
}


std::vector<const Instruction*> InstructionRepository::GetInstructions(ISA isa) const
{
	std::vector<const Instruction*> instructions;

	auto range= GetInstructionsHelper(Architecture(isa));
	instructions.assign(range.begin(), range.end());

	ISA set[]= { ISA::MAC, ISA::EMAC, ISA::EMAC_B, ISA::FPU };

	for (auto test : set)
		if (IsIsaPresent(isa, test))
		{
			auto range= GetInstructionsHelper(test);
			instructions.insert(instructions.end(), range.begin(), range.end());
		}

	return std::move(instructions);
}
