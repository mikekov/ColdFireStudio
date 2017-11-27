/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "Register.h"
#include "..\EmitCode.h"


class BitRev : public InstructionImpl<Stencil_REG>
{
public:
	BitRev() : InstructionImpl("BITREV", IParam().Sizes(IS_LONG).SrcModes(AM_NONE).DestModes(AM_Dx).Opcode(0x00c0).Mask(Stencil::MASK).Isa(ISA::C))
	{}

	virtual DecodedInstruction Decode(InstrPointer& ctx) const
	{
		DecodedInstruction out(Mnemonic(), S_LONG, ctx.OpCode());
		out.dest_ = EffectiveAddress_DReg(GetRegisterCode(ctx.OpCode(), 0));
		return out;
	}

	virtual void Execute(Context& ctx) const
	{
		auto o= OpCode(ctx);

		auto& reg= ctx.Cpu().d_reg[o.reg_index];

		uint32 mask1= 0x80000000;
		uint32 mask2= 0x00000001;
		uint32 reversed= 0;

		for (int i= 0; i < 32; ++i, mask1 >>= 1, mask2 <<= 1)
			if (reg & mask1)
				reversed |= mask2;

		reg = reversed;
	}
};


static Instruction* instr1= GetInstructions().Register(new BitRev());


class ByteRev : public InstructionImpl<Stencil_REG>
{
public:
	ByteRev() : InstructionImpl("BYTEREV", IParam().Sizes(IS_LONG).SrcModes(AM_NONE).DestModes(AM_Dx).Opcode(0x02c0).Mask(Stencil::MASK).Isa(ISA::C))
	{}

	virtual DecodedInstruction Decode(InstrPointer& ctx) const
	{
		DecodedInstruction out(Mnemonic(), S_LONG, ctx.OpCode());
		out.dest_ = EffectiveAddress_DReg(GetRegisterCode(ctx.OpCode(), 0));
		return out;
	}

	virtual void Execute(Context& ctx) const
	{
		auto o= OpCode(ctx);

		auto& reg= ctx.Cpu().d_reg[o.reg_index];

		reg = ByteReverse(reg);
	}
};


static Instruction* instr2= GetInstructions().Register(new ByteRev());
