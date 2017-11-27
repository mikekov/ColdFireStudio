/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "Register.h"
#include <assert.h>


class Cmp : public InstructionImpl<Stencil_REG_OP_EA>
{
public:
	Cmp(uint16 opcode_stencil, InstructionSize size)
		: InstructionImpl("CMP", IParam().Sizes(size).SrcModes(AM_ALL_SRC & ~AM_IMMEDIATE).DestModes(AM_Dx).Opcode(opcode_stencil).Mask(0x0e3f).Isa(size == IS_LONG ? ISA::A : ISA::B))
	{}

	virtual DecodedInstruction Decode(InstrPointer& ctx) const
	{
		InstrSize is= GetOperandSize(ctx.OpCode(), 6);
		if (is == S_NA)
			throw LogicError("Cmp size decoding error");

		DecodedInstruction out(Mnemonic(), is, ctx.OpCode());

		out.DecodeSrcEA(3, 0, ctx);
		out.dest_ = EffectiveAddress_DReg(GetRegisterCode(ctx.OpCode(), 9));

		return out;
	}

	virtual void Execute(Context& ctx) const
	{
		auto o= OpCode(ctx);

		int ext_words= 0;
		InstrSize size;
		uint32 src= 0;

		switch (o.op_mode)	// operation size
		{
		case 0:	// byte
			size = S_BYTE;
			src = ctx.DecodeSrcByteValue(o.ea_mode, size, ext_words);
			break;

		case 1:	// word
			size = S_WORD;
			src = ctx.DecodeSrcWordValue(o.ea_mode, size, ext_words);
			break;

		case 2:	// long word
			size = S_LONG;
			src = ctx.DecodeSrcLongWordValue(o.ea_mode, size, ext_words);
			break;

		default:
			throw LogicError("CMP execute - wrong opcode");
		}

		uint32 dst= ctx.Cpu().d_reg[o.reg_index];

		// set flags based on result
		ctx.SetAllFlags(dst - src, src, dst, size, false, Context::CMP);

		ctx.StepPC(ext_words);
	}
};


static Instruction* instr1= GetInstructions().Register(new Cmp(0xb000, IS_BYTE));
static Instruction* instr2= GetInstructions().Register(new Cmp(0xb040, IS_WORD));
static Instruction* instr3= GetInstructions().Register(new Cmp(0xb080, IS_LONG));


class CmpA : public InstructionImpl<Stencil_REG_OP_EA>
{
public:
	CmpA(uint16 opcode_stencil, InstructionSize size)
		: InstructionImpl("CMPA", IParam().AltName("CMP").Sizes(size).SrcModes(AM_ALL_SRC).DestModes(AM_Ax).Opcode(opcode_stencil).Mask(0x0e3f).Isa(size == IS_LONG ? ISA::A : ISA::B))
	{}

	virtual DecodedInstruction Decode(InstrPointer& ctx) const
	{
//		auto o= OpCode(ctx);
		int opmode= GetOperandMode(ctx.OpCode(), 6);

		InstrSize is= S_NA;
		if (opmode == 3)
			is = S_WORD;
		else if (opmode == 7)
			is = S_LONG;
		else
			throw LogicError("CmpA size decoding error");

		DecodedInstruction out(Mnemonic(), is, ctx.OpCode());

		out.DecodeSrcEA(3, 0, ctx);
		out.dest_ = EffectiveAddress_AReg(GetRegisterCode(ctx.OpCode(), 9));

		return out;
	}

	virtual void Execute(Context& ctx) const
	{
		auto o= OpCode(ctx);

		int ext_words= 0;
		InstrSize size;
		uint32 src= 0;

		switch (o.op_mode)	// operation size
		{
		case 3:		// word
			size = S_WORD;
			src = SignExtendWord(ctx.DecodeSrcWordValue(o.ea_mode, size, ext_words));
			break;

		case 7:		// long word
			size = S_LONG;
			src = ctx.DecodeSrcLongWordValue(o.ea_mode, size, ext_words);
			break;

		default:
			// this should never happen provided correct CMPA registration
			throw LogicError("CMPA execute - wrong opcode");
		}

		uint32 dst= ctx.Cpu().a_reg[o.reg_index];

		// set flags based on result
		ctx.SetAllFlags(dst - src, src, dst, size, false, Context::CMP);

		ctx.StepPC(ext_words);
	}
};


static Instruction* instr4= GetInstructions().Register(new CmpA(0xb0c0, IS_WORD));
static Instruction* instr5= GetInstructions().Register(new CmpA(0xb1c0, IS_LONG));



class CmpI : public InstructionImpl<Stencil_REG>
{
public:
	CmpI(uint16 opcode_stencil, InstructionSize size)
		: InstructionImpl("CMP", IParam().AltName("CMPI").Sizes(size).SrcModes(AM_IMMEDIATE).DestModes(AM_Dx).Opcode(opcode_stencil).Mask(Stencil::MASK).Isa(size == IS_LONG ? ISA::A : ISA::B))
	{}

	virtual DecodedInstruction Decode(InstrPointer& ctx) const
	{
		InstrSize is= GetOperandSize(ctx.OpCode(), 6);
		if (is == S_NA)
			throw LogicError("CmpI size decoding error");

		DecodedInstruction out(Mnemonic(), is, ctx.OpCode());

		out.src_ = EffectiveAddress_Imm(ctx, is, false);
		out.dest_ = EffectiveAddress_DReg(GetRegisterCode(ctx.OpCode(), 0));

		return out;
	}

	virtual void Execute(Context& ctx) const
	{
		Stencil o= OpCode(ctx);

		uint32 dest= ctx.Cpu().d_reg[o.reg_index];
		uint32 src;
		InstrSize size= S_LONG;

		switch (SupportedSizes())
		{
		case IS_BYTE:
			src = ctx.GetNextPCWord() & 0xff;
			dest &= 0xff;
			size = S_BYTE;
			break;

		case IS_WORD:
			src = ctx.GetNextPCWord();
			dest &= 0xffff;
			size = S_WORD;
			break;

		case IS_LONG:
			src = ctx.GetNextPCLongWord();
			size = S_LONG;
			break;

		default:
			throw LogicError("CmpI size error");
		}

		ctx.SetAllFlags(dest - src, src, dest, size, false, Context::CMP);
	}
};


static Instruction* instr6= GetInstructions().Register(new CmpI(0x0c00, IS_BYTE));
static Instruction* instr7= GetInstructions().Register(new CmpI(0x0c40, IS_WORD));
static Instruction* instr8= GetInstructions().Register(new CmpI(0x0c80, IS_LONG));
