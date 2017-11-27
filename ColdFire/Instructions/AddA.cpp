/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "Register.h"


class AddA : public InstructionImpl<Stencil_REG_EA>
{
public:
	AddA() : InstructionImpl("ADDA", IParam().AltName("ADD").Sizes(IS_LONG).SrcModes(AM_ALL_SRC).DestModes(AM_Ax).Opcode(0xd1c0).Mask(Stencil::MASK))
	{}

	virtual DecodedInstruction Decode(InstrPointer& ctx) const
	{
		DecodedInstruction out(Mnemonic(), S_LONG, ctx.OpCode());

		out.DecodeSrcEA(3, 0, ctx);
		out.dest_ = EffectiveAddress_AReg(GetRegisterCode(ctx.OpCode(), 9));

		return out;
	}

	virtual void Execute(Context& ctx) const
	{
		Stencil o= OpCode(ctx);

		int ext_words= 0;
		uint32 src= ctx.DecodeSrcLongWordValue(o.ea_mode, S_LONG, ext_words);

		ctx.Cpu().a_reg[o.reg_index] += src;

		ctx.StepPC(ext_words);
	}
};


static Instruction* instr= GetInstructions().Register(new AddA());
