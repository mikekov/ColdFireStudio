/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "Register.h"
#include <assert.h>

class Pulse : public InstructionImpl<Stencil_UNIQUE>
{
public:
	Pulse() : InstructionImpl("PULSE", IParam().Opcode(0x4acc))
	{}

	virtual DecodedInstruction Decode(InstrPointer& ctx) const
	{
		DecodedInstruction out(Mnemonic(), ctx.OpCode());
		return out;
	}

	virtual void Execute(Context& ctx) const
	{
		// NOP
	}
};

static Instruction* instr1= GetInstructions().Register(new Pulse());
