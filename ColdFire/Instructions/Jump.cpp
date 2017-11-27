/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "Register.h"
#include <assert.h>


const AddressingMode jump_modes= AM_INDIRECT_Ax | AM_DISP_Ax | AM_DISP_Ax_Ix | AM_ABS_W | AM_ABS_L | AM_DISP_PC | AM_DISP_PC_Ix;


class Jmp : public InstructionImpl<Stencil_EA>
{
public:
	Jmp() : InstructionImpl("JMP", IParam().SrcModes(jump_modes).Opcode(0x4ec0).Mask(0x003f).ControlFlow(IControlFlow::BRANCH))
	{}

	virtual DecodedInstruction Decode(InstrPointer& ctx) const
	{
		Stencil o= OpCode(ctx);

		DecodedInstruction out(Mnemonic(), o.opcode);
		out.DecodeSrcEA(o.ea_mode, ctx);

		return out;
	}

	virtual void Execute(Context& ctx) const
	{
		Stencil o= OpCode(ctx);

		int ext_words= 0;
		uint32 addr= ctx.DecodeEffectiveAddress(o.ea_mode, ext_words);

		ctx.Cpu().pc = addr;
	}
};


class Jsr : public InstructionImpl<Stencil_EA>
{
public:
	Jsr() : InstructionImpl("JSR", IParam().SrcModes(jump_modes).Opcode(0x4e80).Mask(0x003f).ControlFlow(IControlFlow::SUBROUTINE))
	{}

	virtual DecodedInstruction Decode(InstrPointer& ctx) const
	{
		Stencil o= OpCode(ctx);

		DecodedInstruction out(Mnemonic(), o.opcode);
		out.DecodeSrcEA(o.ea_mode, ctx);

		return out;
	}

	virtual void Execute(Context& ctx) const
	{
		Stencil o= OpCode(ctx);

		int ext_words= 0;
		uint32 addr= ctx.DecodeEffectiveAddress(o.ea_mode, ext_words);

		ctx.PushLongWord(ctx.InstructionPointer(ext_words));

		ctx.Cpu().pc = addr;
	}
};


static Instruction* instr1= GetInstructions().Register(new Jmp());
static Instruction* instr2= GetInstructions().Register(new Jsr());
