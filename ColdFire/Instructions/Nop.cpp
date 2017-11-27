/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "Register.h"
#include <assert.h>


class Nop : public InstructionImpl<Stencil_UNIQUE>
{
public:
	Nop() : InstructionImpl("NOP", IParam().Opcode(0x4e71))
	{}

	virtual DecodedInstruction Decode(InstrPointer& ctx) const
	{
		assert(VerifyCode(ctx.OpCode()));

		DecodedInstruction out(Mnemonic(), ctx.OpCode());

		return out;
	}

	virtual void Execute(Context& ctx) const
	{
		// NOP
	}
};


static Instruction* instr= GetInstructions().Register(new Nop());
