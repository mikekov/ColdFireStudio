/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2013 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "InstructionMap.h"
#include "Instruction.h"
#include <iostream>
#include "InstructionRepository.h"
#include "InstructionRange.h"
#include <boost/format.hpp>


InstructionMap::InstructionMap(ISA isa)
{
	map_.resize(0x10000, nullptr);	// 65536 entries
	Build(isa);
}

InstructionMap::~InstructionMap()
{
	// nothing to do, map doesn't own instructions
}


void InstructionMap::BuildMap(const Instruction* instr)
{
	uint16 stencil= instr->StencilCode();
	uint16 mask= instr->VariablePartMask();

	const IExcludeCodes& exclude= instr->ExcludedOpcodeValues();
	uint32 exclude_mask= exclude.mask_;
	uint32 exclude_value= exclude.values_[0];

	// special case to exclude this instruction from a map
	if (exclude_mask == 0xffff && exclude_value == 0xffff)
		return;

	// if mask is given, then construct range of codes
	// if mask is 0, then range collapses to a single code

	uint32 first= stencil;
	uint32 last= stencil | mask;

	bool filler= last == 0xffff && first == 0;

	for (uint32 i= first; i <= last; ++i)
	{
		uint32 opcode= (i & mask) | stencil;

		if (exclude_mask != 0)
		{
			bool reject= false;
			for (uint32 i= 0; i < exclude.count_; ++i)
				if ((opcode & exclude_mask) == exclude.values_[i])
				{
					reject = true;
					break;
				}

			if (reject)
				continue;
		}

		if (opcode < i)
			continue;

		if (opcode > last)
			break;

		i = opcode;

		if (map_[opcode] == 0)	// empty slot?
			map_[opcode] = instr;
		else
		{
			// instruction opcode already occupied
			auto old= map_[opcode];
			if (!filler)
			{
				//if (old->DefinedInIsa() != instr->DefinedInIsa())
				//{
				//	// replace ISA_A with ISA_B/C
				//	if (instr->DefinedInIsa() > old->DefinedInIsa())
				//		map_[opcode] = instr;
				//}
				//else
				{
//					std::cerr << "existing: " << old->Mnemonic() << "  new one: " << instr->Mnemonic() << " opcode " << std::hex << i << std::endl;
					throw LogicError((boost::format("instruction code is already occupied " __FUNCTION__ "\n"
						"existing: %s  new one: %s opcode %x") % old->Mnemonic() % instr->Mnemonic() % i).str().c_str());
				}
			}
		}
	}
}


void InstructionMap::Build(ISA isa)
{
	std::fill(map_.begin(), map_.end(), nullptr);

	auto range= GetInstructions().GetInstructions(isa);
	for (auto i : range)
		BuildMap(i);
}
