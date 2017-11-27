/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "Register.h"
#include <assert.h>


class Ori : public InstructionImpl<Stencil_REG>
{
public:
	Ori() : InstructionImpl("OR", IParam().AltName("ORI").Sizes(IS_LONG).SrcModes(AM_IMMEDIATE).DestModes(AM_Dx).Opcode(0x0080).Mask(Stencil::MASK))
	{}

	virtual DecodedInstruction Decode(InstrPointer& ctx) const
	{
		assert(VerifyCode(ctx.OpCode()));

		DecodedInstruction out(Mnemonic(), S_LONG, ctx.OpCode());

		out.src_ = EffectiveAddress_Imm(ctx.GetNextLongWord(), false);
		out.dest_ = EffectiveAddress_DReg(GetRegisterCode(ctx.OpCode(), 0));

		return out;
	}

	virtual void Execute(Context& ctx) const
	{
		Stencil o= OpCode(ctx);

		int ext_words= 0;
		uint32 mask= ctx.DecodeSrcLongWordValue(EAF_Immediate, S_LONG, ext_words);
		uint32& reg= ctx.Cpu().d_reg[o.reg_index];

		reg |= mask;

		ctx.SetNZ_ClrCV(reg);
		ctx.StepPC(ext_words);
	}
};


static Instruction* instr= GetInstructions().Register(new Ori());


class Eori : public InstructionImpl<Stencil_REG>
{
public:
	Eori() : InstructionImpl("EOR", IParam().AltName("EORI").Sizes(IS_LONG).SrcModes(AM_IMMEDIATE).DestModes(AM_Dx).Opcode(0x0a80).Mask(Stencil::MASK))
	{}

	virtual DecodedInstruction Decode(InstrPointer& ctx) const
	{
		assert(VerifyCode(ctx.OpCode()));

		DecodedInstruction out(Mnemonic(), S_LONG, ctx.OpCode());

		out.src_ = EffectiveAddress_Imm(ctx.GetNextLongWord(), false);
		out.dest_ = EffectiveAddress_DReg(GetRegisterCode(ctx.OpCode(), 0));

		return out;
	}

	virtual void Execute(Context& ctx) const
	{
		Stencil o= OpCode(ctx);

		int ext_words= 0;
		uint32 mask= ctx.DecodeSrcLongWordValue(EAF_Immediate, S_LONG, ext_words);
		uint32& reg= ctx.Cpu().d_reg[o.reg_index];

		reg ^= mask;

		ctx.SetNZ_ClrCV(reg);
		ctx.StepPC(ext_words);
	}
};


static Instruction* instr2= GetInstructions().Register(new Eori());
