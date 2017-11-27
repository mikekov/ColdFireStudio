/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "Register.h"
#include <assert.h>

class Illegal : public InstructionImpl<Stencil_UNIQUE>
{
public:
	Illegal() : InstructionImpl("ILLEGAL", IParam().Opcode(0x4afc))
	{}

	virtual DecodedInstruction Decode(InstrPointer& ctx) const
	{
		DecodedInstruction out(Mnemonic(), ctx.OpCode());
		return out;
	}

	virtual void Execute(Context& ctx) const
	{
		ctx.EnterException(EX_IllegalInstruction, ctx.Cpu().pc - 2);
	}
};

static Instruction* instr1= GetInstructions().Register(new Illegal());
