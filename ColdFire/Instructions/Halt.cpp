/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "Register.h"
#include <assert.h>

class Halt : public InstructionImpl<Stencil_UNIQUE>
{
public:
	Halt() : InstructionImpl("HALT", IParam().Opcode(0x4ac8).ControlFlow(IControlFlow::STOP))
	{}

	virtual DecodedInstruction Decode(InstrPointer& ctx) const
	{
		DecodedInstruction out(Mnemonic(), ctx.OpCode());
		return out;
	}

	virtual void Execute(Context& ctx) const
	{
		// HALT instruction is useful in real hardware when BDM module is present; GO signal can then
		// resulme execution after it's been stopped; without BDM HALT causes illegal instruction exception;
		// in a simulator HALT is used to terminate program execution
		ctx.HaltExecution(true);
	}
};

static Instruction* instr1= GetInstructions().Register(new Halt());
