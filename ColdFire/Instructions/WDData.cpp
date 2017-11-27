/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "Register.h"
#include <assert.h>

class WDData : public InstructionImpl<Stencil_S_EA>
{
public:
	WDData(uint16 opcode_stencil, InstructionSize size)
		: InstructionImpl("WDDATA", IParam().Sizes(size).DefaultSize(size == IS_LONG ? size : IS_NONE).SrcModes(AM_INDIRECT_Ax | AM_Ax_INC | AM_DEC_Ax | AM_DISP_Ax | AM_DISP_Ax_Ix | AM_ABS_W | AM_ABS_L).Opcode(opcode_stencil).Mask(0x003f))
		//TODO exclude code (AM_Dx, AM_Ax, and more)
	{}

	virtual DecodedInstruction Decode(InstrPointer& ctx) const
	{
		assert(false); //TODO

		DecodedInstruction out(Mnemonic(), ctx.OpCode());
		return out;
	}

	virtual void Execute(Context& ctx) const
	{
		assert(false); //TODO
	}
};

static Instruction* instr10= GetInstructions().Register(new WDData(0xfb00, IS_BYTE));
static Instruction* instr11= GetInstructions().Register(new WDData(0xfb40, IS_WORD));
static Instruction* instr12= GetInstructions().Register(new WDData(0xfb80, IS_LONG));
