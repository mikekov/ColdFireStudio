/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "Register.h"
#include <assert.h>


class Clr : public InstructionImpl<Stencil_S_EA>
{
public:
	Clr(uint16 opcode_stencil, InstructionSize size)
		: InstructionImpl("CLR", IParam().Sizes(size).DefaultSize(size == IS_LONG ? size : IS_NONE).SrcModes(AM_NONE).DestModes(AM_ALL_DST & ~AM_Ax).Opcode(opcode_stencil).Mask(0x003f))
	{}

	virtual DecodedInstruction Decode(InstrPointer& ctx) const
	{
		InstrSize is= GetOperandSize(ctx.OpCode(), 6);

		if (is == S_NA)
			throw LogicError("Clr instr - bad code");

		DecodedInstruction out(Mnemonic(), is, ctx.OpCode());
		out.DecodeDestEA(3, 0, ctx);

		return out;
	}

	virtual void Execute(Context& ctx) const
	{
		Stencil o= OpCode(ctx);

		int dest_mode= o.ea_mode;
		int ext_words= 0;

		switch (o.size)
		{
		case 0:	// BYTE
			ctx.DecodeAndSetDestByte(dest_mode, S_BYTE, ext_words, 0);
			break;

		case 1:	// WORD
			ctx.DecodeAndSetDestWord(dest_mode, S_WORD, ext_words, 0);
			break;

		case 2:	// LONG
			ctx.DecodeAndSetDestLongWord(dest_mode, S_LONG, ext_words, 0);
			break;

		default:	// reserved
			throw LogicError("invalid size in CLR instruction opcode");
			break;
		}

		ctx.SetNZ_ClrCV(uint32(0));
		ctx.StepPC(ext_words);
	}
};


static Instruction* instr1= GetInstructions().Register(new Clr(0x4200, IS_BYTE));
static Instruction* instr2= GetInstructions().Register(new Clr(0x4240, IS_WORD));
static Instruction* instr3= GetInstructions().Register(new Clr(0x4280, IS_LONG));
