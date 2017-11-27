/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "Register.h"


class Mv : public InstructionImpl<Stencil_REG_EA>
{
public:
	Mv(uint16 opcode) : InstructionImpl(opcode & UNSIGNED ? "MVZ" : "MVS", IParam().Sizes(opcode & WORD_SIZE ? IS_WORD : IS_BYTE).DefaultSize(IS_NONE).SrcModes(AM_ALL_SRC).DestModes(AM_Dx).Opcode(opcode).Mask(Stencil::MASK).Isa(ISA::B))
	{}

	virtual DecodedInstruction Decode(InstrPointer& ctx) const
	{
		auto o= OpCode(ctx);
		DecodedInstruction out(Mnemonic(), o.opcode & WORD_SIZE ? S_WORD : S_BYTE, o.opcode);
		out.DecodeSrcEA(o.ea_mode, ctx);
		out.dest_ = EffectiveAddress_DReg(o.reg_index);
		return out;
	}

	virtual void Execute(Context& ctx) const
	{
		auto o= OpCode(ctx);

		int ext_words= 0;
		uint32 src;
		if (o.opcode & WORD_SIZE)
		{
			auto v= ctx.DecodeSrcWordValue(o.ea_mode, S_WORD, ext_words);
			if (o.opcode & UNSIGNED)
				src = v;
			else
				src = SignExtendWord(v);
		}
		else
		{
			auto v= ctx.DecodeSrcByteValue(o.ea_mode, S_BYTE, ext_words);
			if (o.opcode & UNSIGNED)
				src = v;
			else
				src = SignExtendByte(v);
		}

		ctx.Cpu().d_reg[o.reg_index] = src;
		ctx.SetNZ_ClrCV(src);	// note that this effectively clears N flag, as needed
	}

	enum { WORD_SIZE= 0x0040, UNSIGNED= 0x0080 };
};

// MVZ
static Instruction* instr10= GetInstructions().Register(new Mv(0x7180));
static Instruction* instr11= GetInstructions().Register(new Mv(0x71c0));

// MVS
static Instruction* instr20= GetInstructions().Register(new Mv(0x7100));
static Instruction* instr21= GetInstructions().Register(new Mv(0x7140));
