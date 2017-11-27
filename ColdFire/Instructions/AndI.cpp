/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "Register.h"


class AndI : public InstructionImpl<Stencil_REG>
{
public:
	AndI() : InstructionImpl("ANDI", IParam().AltName("AND").Sizes(IS_LONG).SrcModes(AM_IMMEDIATE).DestModes(AM_Dx).Opcode(0x0280).Mask(Stencil::MASK))
	{}

	virtual DecodedInstruction Decode(InstrPointer& ctx) const
	{
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

		reg &= mask;

		ctx.SetNZ_ClrCV(reg);
		ctx.StepPC(ext_words);
	}

	//virtual void Encode(const EffectiveAddress& ea_src, const EffectiveAddress& ea_dst, OutputPointer& ctx) const
	//{
	//	if (ea_src.mode_ == AM_IMMEDIATE && ea_dst.mode_ == AM_Dx)
	//		Emit_Dx_Imm(StencilCode(), ea_dst.first_reg_, 0, ea_src.val_, ctx);
	//	else
	//		throw LogicError("invalid mode in ANDI " __FUNCTION__);
	//}
};


static Instruction* instr= GetInstructions().Register(new AndI());
