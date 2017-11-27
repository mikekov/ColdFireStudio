/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "Register.h"


class Swap : public InstructionImpl<Stencil_REG>
{
public:
	Swap() : InstructionImpl("SWAP", IParam().Sizes(IS_WORD).SrcModes(AM_NONE).DestModes(AM_Dx).Opcode(0x4840).Mask(Stencil::MASK))
	{}

	virtual DecodedInstruction Decode(InstrPointer& ctx) const
	{
		Stencil o= OpCode(ctx);

		DecodedInstruction out(Mnemonic(), S_WORD, o.opcode);
		out.dest_ = EffectiveAddress_DReg(o.reg_index);

		return out;
	}

	virtual void Execute(Context& ctx) const
	{
		Stencil o= OpCode(ctx);

		auto& dx= ctx.Cpu().d_reg[o.reg_index];
		// swap words in data register
		dx = ((dx >> 16) & 0xffff) | (dx << 16);

		ctx.SetNZ_ClrCV(dx);
	}
};


static Instruction* instr1= GetInstructions().Register(new Swap());
