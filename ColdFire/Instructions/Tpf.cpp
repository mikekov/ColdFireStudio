/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "Register.h"
#include <assert.h>

namespace {
	static const uint16 NO_EXT_WORDS= 4;
	static const uint16 ONE_EXT_WORD= 2;
	static const uint16 TWO_EXT_WORDS= 3;
}

class Tpf : public InstructionImpl<Stencil_REG>
{
public:
	Tpf() : InstructionImpl("TPF", IParam().Opcode(0x51f8).SrcModes(AM_RELATIVE | AM_IMPLIED).Mask(Stencil::MASK).ExcludeCodes(7, 0, 1, 5, 6, 7))
	{}

	virtual DecodedInstruction Decode(InstrPointer& ctx) const
	{
		assert(VerifyCode(ctx.OpCode()));
		auto o= OpCode(ctx);

		DecodedInstruction out(Mnemonic(), o.opcode);

		switch (o.reg_index)
		{
		case NO_EXT_WORDS:
			break;

		case ONE_EXT_WORD:
			out.src_ = DasmEffectiveAddress(AM_RELATIVE, 2);
			break;

		case TWO_EXT_WORDS:
			out.src_ = DasmEffectiveAddress(AM_RELATIVE, 4);
			break;
		}

		return out;
	}

	virtual bool IsRelativeOffsetValid(uint32 offset, InstructionSize requested_size) const
	{
		bool ok= offset == 0 || offset == 2 || offset == 4;
		return ok;
	}

	virtual bool CalcSize(InstructionSize size, const EffectiveAddress& ea_src, const EffectiveAddress& ea_dst, uint32& len) const
	{
		len = 2;	// two bytes; payload (if any) follows as consecutive opcodes of next instruction(s)
		return true;
	}

	virtual void Encode(InstructionSize size, const EffectiveAddress& ea_src, const EffectiveAddress& ea_dst, OutputPointer& ctx) const
	{
		Stencil o;
		o.opcode = StencilCode();
		if (ea_src.val_.inf == Expr::EX_UNDEF || ea_src.val_.inf == Expr::EX_NONE)
			o.reg_index = NO_EXT_WORDS;
		else
			switch (ea_src.val_.Value())
			{
			case 2:
				o.reg_index = ONE_EXT_WORD;
				break;
			case 4:
				o.reg_index = TWO_EXT_WORDS;
				break;
			default:
				throw LogicError("Encoding error; invalid offset in " __FUNCTION__);
			}
		ctx << o.opcode;
	}

	virtual void Execute(Context& ctx) const
	{
		auto opmode= OpCode(ctx).opcode & 7;

		if (opmode == ONE_EXT_WORD)
			ctx.StepPC(1);
		else if (opmode == TWO_EXT_WORDS)
			ctx.StepPC(2);
	}
};


static Instruction* instr= GetInstructions().Register(new Tpf());
