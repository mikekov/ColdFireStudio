/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "Register.h"

#if 0
class Filler : public Instruction
{
public:
	Filler() : Instruction(0, 0, 0xffff, ISA_A)
	{}

	virtual DecodedInstruction Decode(InstrPointer& ctx) const
	{
		DecodedInstruction out(".WORD", ctx.OpCode());
		out.src_ = DasmEffectiveAddress(AM_WORD_DATA, -1, ctx.OpCode(), false);
		out.unknown_instr_ = true;
		return out;
	}

	virtual void Execute(Context& ctx) const
	{}
};


//static Instruction* instr= GetInstructions().Register(new Filler());

extern void FillInstrGaps()
{
	static Instruction* instr= GetInstructions().Register(new Filler());
}
#endif
