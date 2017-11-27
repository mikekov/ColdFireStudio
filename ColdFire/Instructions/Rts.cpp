/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "Register.h"
#include <assert.h>


class Rts : public InstructionImpl<Stencil_UNIQUE>
{
public:
	Rts() : InstructionImpl("RTS", IParam().Opcode(0x4e75).ControlFlow(IControlFlow::RETURN))
	{}

	virtual DecodedInstruction Decode(InstrPointer& ctx) const
	{
//		assert(VerifyCode(ctx.OpCode()));

		DecodedInstruction out(Mnemonic(), ctx.OpCode());

		return out;
	}

	virtual void Execute(Context& ctx) const
	{
		uint32 addr= ctx.PullLongWord();
		ctx.Cpu().pc = addr;
	}
};


static Instruction* instr= GetInstructions().Register(new Rts());



class Rte : public InstructionImpl<Stencil_UNIQUE>
{
public:
	Rte() : InstructionImpl("RTE", IParam().Opcode(0x4e73).Privileged().ControlFlow(IControlFlow::RETURN))
	{}

	virtual DecodedInstruction Decode(InstrPointer& ctx) const
	{
		DecodedInstruction out(Mnemonic(), ctx.OpCode());

		return out;
	}

	virtual void Execute(Context& ctx) const
	{
		// ideally this data should be read first, and SP adjusted after that
		auto frame= ctx.PullLongWord();
		auto ret_addr= ctx.PullLongWord();

		// restore original stack alignment
		auto misalignment= (frame >> 28) & 0x03;
		ctx.Cpu().a_reg[7] += misalignment;

		ctx.Cpu().SetSR(static_cast<uint16>(frame & 0xffff));
		ctx.Cpu().pc = ret_addr;
	}
};


static Instruction* instr2= GetInstructions().Register(new Rte());
