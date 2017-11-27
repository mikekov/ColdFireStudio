/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "Register.h"
#include "..\EmitCode.h"


class Sats : public InstructionImpl<Stencil_REG>
{
public:
	Sats() : InstructionImpl("SATS", IParam().Sizes(IS_LONG).DefaultSize(IS_LONG).SrcModes(AM_NONE).DestModes(AM_Dx).Opcode(0x4c80).Mask(Stencil::MASK).Isa(ISA::B))
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

		// if overflow, then signed saturate
		if (ctx.IsVS())
		{
			if (int32(reg) < 0)
				reg = 0x80000000;
			else
				reg = 0x7fffffff;
		}

		ctx.SetNZ_ClrCV(reg);
	}
};


static Instruction* instr1= GetInstructions().Register(new Sats());
