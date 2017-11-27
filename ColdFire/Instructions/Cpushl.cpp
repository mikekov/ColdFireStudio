/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "Register.h"


class Cpushl : public InstructionImpl<Stencil_FLD_REG>
{
public:
	Cpushl() : InstructionImpl("CPUSHL", IParam().SrcModes(AM_SPECIAL).DestModes(AM_INDIRECT_Ax).Opcode(0xf428).Mask(Stencil::MASK).Isa(ISA::A).Privileged().ExcludeCodes(0xc0, 0))
	{}

	virtual DecodedInstruction Decode(InstrPointer& ctx) const
	{
		auto o= OpCode(ctx);
		DecodedInstruction out(Mnemonic(), o.opcode);

		out.src_ = DasmEffectiveAddress(AM_SPECIAL, o.code, 0, false);

		out.dest_ = DasmEffectiveAddress(AM_INDIRECT_Ax, o.reg_index, 0, false);

		return out;
	}

	virtual void Execute(Context& ctx) const
	{
		//
	}
};


static Instruction* instr= GetInstructions().Register(new Cpushl());
