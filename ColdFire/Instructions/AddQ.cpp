/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "Register.h"
#include "..\EmitCode.h"


class AddQ : public InstructionImpl<Stencil_REG_EA>
{
public:
	AddQ() : InstructionImpl("ADDQ", IParam().Sizes(IS_LONG).SrcModes(AM_IMMEDIATE).ImmDataRange(1, 8).DestModes(AM_ALL_DST).Opcode(0x5080).Mask(Stencil::MASK))
	{}

	virtual DecodedInstruction Decode(InstrPointer& ctx) const
	{
		DecodedInstruction out(Mnemonic(), S_LONG, ctx.OpCode());
		uint16 val= (ctx.OpCode() >> 9) & 0x7;
		if (val == 0)
			val = 8;
		out.src_ = EffectiveAddress_Imm(val, true);	// not signed data, but signed output is shorter
		out.DecodeDestEA(3, 0, ctx);

		return out;
	}

	virtual void Execute(Context& ctx) const
	{
		Stencil o= OpCode(ctx);

		uint32 src= o.reg_index;
		if (src == 0)
			src = 8;

		InstrSize size= S_LONG;
		int ext_words= 0;
		DecodedAddress da= ctx.DecodeMemoryAddress(o.ea_mode, size, ext_words);
		uint32 dst= ctx.ReadFromAddress(da, size);
		ctx.WriteToAddress(da, src + dst, size);

		auto mode= (o.ea_mode >> 3) & 0x7;
		if (mode != 1)	// conditions not changed for An destination
			ctx.SetAllFlags(src + dst, src, dst, S_LONG, false, Context::ADD);

		ctx.StepPC(ext_words);
	}

	virtual void Encode(InstructionSize size, const EffectiveAddress& ea_src, const EffectiveAddress& ea_dst, OutputPointer& ctx) const
	{
		if (ea_src.mode_ == AM_IMMEDIATE && ea_src.val_.IsNumber())
		{
			// 8 is encoded as 0
			uint16 val= ea_src.val_.value == 8 ? 0 : ea_src.val_.value;
			Emit_Imm_EA(StencilCode(), val, 9, ea_dst, ctx);
		}
		else
			throw std::exception("illegal addressing mode or value in " __FUNCTION__);
	}

	virtual bool CalcSize(InstructionSize size, const EffectiveAddress& ea_src, const EffectiveAddress& ea_dst, uint32& len) const
	{
		return Instruction::CalcSize(size, EffectiveAddress(), ea_dst, len);
	}
};


static Instruction* instr= GetInstructions().Register(new AddQ());
