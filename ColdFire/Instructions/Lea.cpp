/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "Register.h"


namespace {
	const AddressingMode ea_modes= AM_INDIRECT_Ax | AM_DISP_Ax | AM_DISP_Ax_Ix | AM_ABS_W | AM_ABS_L | AM_DISP_PC | AM_DISP_PC_Ix;
}


class Lea : public InstructionImpl<Stencil_REG_OP_EA>
{
public:
	Lea() : InstructionImpl("LEA", IParam().Sizes(IS_LONG).SrcModes(ea_modes).DestModes(AM_Ax).Opcode(0x41c0).Mask(0x0e3f).ExcludeCodes(0x38, 0x0, 0x08, 0x18, 0x20))
	{}

	virtual DecodedInstruction Decode(InstrPointer& ctx) const
	{
		DecodedInstruction out(Mnemonic(), S_LONG, ctx.OpCode());
		out.DecodeSrcEA(3, 0, ctx);
		// to do: reject illegal addr modes
		out.dest_ = EffectiveAddress_AReg(GetRegisterCode(ctx.OpCode(), 9));

		return out;
	}

	virtual void Execute(Context& ctx) const
	{
		Stencil o= OpCode(ctx);

		int ext_words= 0;
		uint32 addr= ctx.DecodeEffectiveAddress(o.ea_mode, ext_words);

		ctx.Cpu().a_reg[o.reg_index] = addr;

		ctx.StepPC(ext_words);
	}
};


static Instruction* instr1= GetInstructions().Register(new Lea());


class Pea : public InstructionImpl<Stencil_EA>
{
public:
	Pea() : InstructionImpl("PEA", IParam().Sizes(IS_LONG).SrcModes(ea_modes).Opcode(0x4840).Mask(Stencil::MASK).ExcludeCodes(0x38, 0x0, 0x08, 0x18, 0x20))
	{}

	virtual DecodedInstruction Decode(InstrPointer& ctx) const
	{
		DecodedInstruction out(Mnemonic(), S_LONG, ctx.OpCode());
		// to do: reject illegal addr modes
		out.DecodeSrcEA(3, 0, ctx);

		return out;
	}

	virtual void Execute(Context& ctx) const
	{
		Stencil o= OpCode(ctx);

		int ext_words= 0;
		uint32 addr= ctx.DecodeEffectiveAddress(o.ea_mode, ext_words);
		ctx.PushLongWord(addr);

		ctx.StepPC(ext_words);
	}
};


static Instruction* instr2= GetInstructions().Register(new Pea());
