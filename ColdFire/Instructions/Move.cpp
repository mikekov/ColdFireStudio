/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "Register.h"
#include <assert.h>
#include "../EmitCode.h"


class Move : public InstructionImpl<Stencil_S_EA_EA>
{
public:
	//TODO: addr mode combinations stipulations; not all src/dest combinations are valid
	Move(uint16 opcode_stencil, InstructionSize size)
		// note: exclude mask takes care of excluding MOVEA (addr register destination)
		: InstructionImpl("MOVE", IParam().Sizes(size).SrcModes(AM_ALL_SRC).DestModes(AM_ALL_DST & ~AM_Ax).Opcode(opcode_stencil).Mask(0x0fff).ExcludeCodes(0x01c0, 0x0040))
	{}

	// MOVEA only
	Move(uint16 opcode, uint16 mask, InstructionSize size)
		: InstructionImpl("MOVEA", IParam().AltName("MOVE").Sizes(size).SrcModes(AM_ALL_SRC).DestModes(AM_Ax).Opcode(opcode).Mask(mask))
	{}

	virtual DecodedInstruction Decode(InstrPointer& ctx) const
	{
		assert(VerifyCode(ctx.OpCode()));

		Stencil o;
		ctx.OpCode(o);

		//int size= ctx.OpCode() >> 12;
		InstrSize is= S_NA;

		switch (o.size)
		{
		case 1: is = S_BYTE; break;
		case 3: is = S_WORD; break;
		case 2: is = S_LONG; break;

		default:
			return DecodedInstruction();
		}

		DecodedInstruction out(Mnemonic(), is, ctx.OpCode());
		//TODO: order - dst : src or src - dst
		out.DecodeSrcEA(o.src_ea_mode, ctx);

		int dest_mode= (o.dest_ea_mode << 3) | o.dest_ea_reg;
		out.DecodeDestEA(dest_mode, ctx);
		//out.DecodeDestEA(6, 9, ctx);

		return out;
	}

	virtual void Execute(Context& ctx) const
	{
		auto o= OpCode(ctx);

		int dest_mode= (o.dest_ea_mode << 3) | o.dest_ea_reg;

		int ext_words= 0;
		uint32 result= 0, sign_mask= 0;

		switch (o.size)
		{
		case 1:	// is = S_BYTE
			{
				uint8 src= ctx.DecodeSrcByteValue(o.src_ea_mode, S_BYTE, ext_words);
				ctx.DecodeAndSetDestByte(dest_mode, S_BYTE, ext_words, src);
				result = src;
				sign_mask = 1 << 7;
			}
			break;
		case 3:	// is = S_WORD
			{
				uint16 src= ctx.DecodeSrcWordValue(o.src_ea_mode, S_WORD, ext_words);
				ctx.DecodeAndSetDestWord(dest_mode, S_WORD, ext_words, src);
				result = src;
				sign_mask = 1 << 15;
			}
			break;
		case 2:	// is = S_LONG
			{
				uint32 src= ctx.DecodeSrcLongWordValue(o.src_ea_mode, S_LONG, ext_words);
				ctx.DecodeAndSetDestLongWord(dest_mode, S_LONG, ext_words, src);
				result = src;
				sign_mask = 1 << 31;
			}
			break;

		default:
			throw LogicError("invalid size in MOVE instruction opcode");
			break;
		}

		if (o.dest_ea_mode != EAF_Ax)
			ctx.SetNZ_ClrCV(result, sign_mask);

		ctx.StepPC(ext_words);
	}

	virtual void Encode(InstructionSize size, const EffectiveAddress& ea_src, const EffectiveAddress& ea_dst, OutputPointer& ctx) const
	{
		uint16 opcode= StencilCode();

		uint16 words_1[3];
		int count_1= Encode_EA(opcode, ea_src, 3, 0, words_1);

		uint16 words_2[3];
		int count_2= Encode_EA(opcode, ea_dst, 6, 9, words_2);

		int ext_words= count_1 - 1 + count_2 - 1;
		if (ext_words > 2)	// ColdFire supports up to 2 ext. words
			throw LogicError("illegal addressing mode combination in " __FUNCTION__);

		opcode = words_1[0] | words_2[0];
		ctx << opcode;

		//TODO: order - dst : src or src - dst

		// source
		for (int i= 1; i < count_1; ++i)
			ctx << words_1[i];

		// destination
		for (int i= 1; i < count_2; ++i)
			ctx << words_2[i];
	}
};


class Move_L : public Move
{
public:
	Move_L() : Move(0x2000, IS_LONG)
	{}
};


class Move_W : public Move
{
public:
	Move_W() : Move(0x3000, IS_WORD)
	{}
};


class Move_B : public Move
{
public:
	Move_B() : Move(0x1000, IS_BYTE)
	{}
};


class MoveA : public Move
{
public:
	MoveA(uint16 opcode, InstructionSize size) : Move(opcode, 0x0e3f, size)
	{}

	virtual DecodedInstruction Decode(InstrPointer& ctx) const
	{
		assert(VerifyCode(ctx.OpCode()));

		int size= ctx.OpCode() >> 12;
		InstrSize is= S_NA;

		switch (size)
		{
		//illegal here case 1: is = S_BYTE; break;
		case 3: is = S_WORD; break;
		case 2: is = S_LONG; break;

		default:
			return DecodedInstruction();
		}

		DecodedInstruction out(Mnemonic(), is, ctx.OpCode());
		out.DecodeDestEA(6, 9, ctx);
		out.DecodeSrcEA(3, 0, ctx);

		return out;
	}
};


static Instruction* instr1= GetInstructions().Register(new Move_B);
static Instruction* instr2= GetInstructions().Register(new Move_W);
static Instruction* instr3= GetInstructions().Register(new Move_L);
static Instruction* instr40= GetInstructions().Register(new MoveA(0x3040, IS_WORD));
static Instruction* instr41= GetInstructions().Register(new MoveA(0x2040, IS_LONG));
