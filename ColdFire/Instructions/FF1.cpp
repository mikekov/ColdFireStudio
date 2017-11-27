/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "Register.h"
#include "..\EmitCode.h"


class Ff1 : public InstructionImpl<Stencil_REG>
{
public:
	Ff1() : InstructionImpl("FF1", IParam().Sizes(IS_LONG).DefaultSize(IS_LONG).SrcModes(AM_NONE).DestModes(AM_Dx).Opcode(0x04c0).Mask(Stencil::MASK).Isa(ISA::C))
	{}

	virtual DecodedInstruction Decode(InstrPointer& ctx) const
	{
		auto o= OpCode(ctx);
		DecodedInstruction out(Mnemonic(), S_LONG, o.opcode);
		out.dest_ = EffectiveAddress_DReg(o.reg_index);
		return out;
	}

	virtual void Execute(Context& ctx) const
	{
		auto o= OpCode(ctx);

		auto& reg= ctx.Cpu().d_reg[o.reg_index];
		auto d= reg;

		// find first set bit
		uint32 i= 0;
		uint32 mask= 0x80000000;

		for ( ; i < 32; ++i, mask >>= 1)
			if (d & mask)
				break;

		reg = i;

		ctx.SetNZ_ClrCV(d);
	}
};


static Instruction* instr1= GetInstructions().Register(new Ff1());
