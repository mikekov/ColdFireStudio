/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "Register.h"


class Not : public InstructionImpl<Stencil_REG>
{
public:
	Not() : InstructionImpl("NOT", IParam().Sizes(IS_LONG).SrcModes(AM_Dx).Opcode(0x4680).Mask(Stencil::MASK))
	{}

	virtual DecodedInstruction Decode(InstrPointer& ctx) const
	{
		DecodedInstruction out(Mnemonic(), S_LONG, ctx.OpCode());
		out.src_ = EffectiveAddress_DReg(GetRegisterCode(ctx.OpCode(), 0));

		return out;
	}

	virtual void Execute(Context& ctx) const
	{
		Stencil o= OpCode(ctx);

		auto& reg= ctx.Cpu().d_reg[o.reg_index];
		reg = ~reg;

		ctx.SetNZ_ClrCV(reg);
	}
};


static Instruction* instr1= GetInstructions().Register(new Not());



class Neg : public InstructionImpl<Stencil_REG>
{
public:
	Neg(const char* name, uint16 opcode)
		: InstructionImpl(name, IParam().Sizes(IS_LONG).SrcModes(AM_Dx).Opcode(opcode).Mask(Stencil::MASK))
	{}

	virtual DecodedInstruction Decode(InstrPointer& ctx) const
	{
		DecodedInstruction out(Mnemonic(), S_LONG, ctx.OpCode());
		out.src_ = EffectiveAddress_DReg(GetRegisterCode(ctx.OpCode(), 0));

		return out;
	}

	virtual void Execute(Context& ctx) const
	{
		Stencil o= OpCode(ctx);

		auto& reg= ctx.Cpu().d_reg[o.reg_index];
		auto src= reg;
		auto conditional_zero= false;

		if (o.opcode & 0x0400)	// NEG?
		{
			reg = 0 - reg;
		}
		else
		{
			// NEGX
			reg = 0 - reg - (ctx.Extend() ? 1 : 0);
			conditional_zero = true;
		}

		ctx.SetAllFlags(reg, src, 0, S_LONG, conditional_zero, Context::NEG);
	}
};


static Instruction* instr20= GetInstructions().Register(new Neg("NEG", 0x4480));
static Instruction* instr21= GetInstructions().Register(new Neg("NEGX", 0x4080));
