/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "Register.h"
#include <assert.h>
#include "..\Utilities.h"


class Ext : public InstructionImpl<Stencil_OP_REG>
{
public:
	Ext(uint16 opcode_stencil, const char* name, InstructionSize size, InstructionSize default_size)
		: InstructionImpl(name, IParam().Sizes(size).DefaultSize(default_size).SrcModes(AM_Dx).Opcode(opcode_stencil).Mask(0x0007))
	{}

	virtual DecodedInstruction Decode(InstrPointer& ctx) const
	{
		Stencil o= OpCode(ctx);

		int opmode= o.op_mode;// GetOperandMode(ctx.OpCode(), 6);
		InstrSize is= S_NA;
		if (opmode == 2)		// byte -> word
			is = S_WORD;
		else if (opmode == 3)	// word -> long word
			is = S_LONG;
		else if (opmode == 7)	// byte -> long
			is = S_LONG;
		else
			throw LogicError("wrong opmode for Ext");

		DecodedInstruction out(Mnemonic(), is, ctx.OpCode());

		out.src_ = EffectiveAddress_DReg(o.reg_index);

		return out;
	}

	virtual void Execute(Context& ctx) const
	{
		auto o= OpCode(ctx);

		auto& dx= ctx.Cpu().d_reg[o.reg_index];

		switch (o.op_mode)
		{
		case 0x2:	// byte -> word
			{
				uint16 result= SignExtendByte2Word(dx);
				ctx.SetNZ_ClrCV(result);
				dx &= 0xffff0000;
				dx |= result;
			}
			break;
		case 0x3:	// word -> long word
			{
				dx = SignExtendWord(dx);
				ctx.SetNZ_ClrCV(dx);
			}
			break;
		case 0x7:	// byte -> long word
			{
				dx = SignExtendByte(dx);
				ctx.SetNZ_ClrCV(dx);
			}
			break;
		default:
			// illegal, we shouldn't be registered for such codes
			throw LogicError("Ext: unexpected code");
		}
	}
};


static Instruction* instr1= GetInstructions().Register(new Ext(0x4880, "EXT", IS_WORD, IS_NONE));	// byte to word
static Instruction* instr2= GetInstructions().Register(new Ext(0x48c0, "EXT", IS_LONG, IS_LONG));	// word to long
static Instruction* instr3= GetInstructions().Register(new Ext(0x49c0, "EXTB", IS_LONG, IS_LONG));	// byte to long
