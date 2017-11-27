/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "Register.h"
#include <assert.h>

class Stop : public InstructionImpl<Stencil_UNIQUE>
{
public:
	Stop() : InstructionImpl("STOP", IParam().Opcode(0x4e72).Sizes(IS_WORD).SrcModes(AM_IMMEDIATE).ControlFlow(IControlFlow::STOP))
	{}

	virtual DecodedInstruction Decode(InstrPointer& ctx) const
	{
		DecodedInstruction out(Mnemonic(), ctx.OpCode());
		return out;
	}

	virtual void Execute(Context& ctx) const
	{
		uint16 stat= ctx.GetNextPCWord();

		if ((stat & cf::SR_SUPERVISOR) == 0)
			ctx.EnterException(EX_PrivilegeViolation, ctx.Cpu().pc - 2);
		else
			ctx.Cpu().SetSR(stat);

		// STOP can also cause illegal instruction exception if low power modules are not enabled;
		// this is currently not simulated

		//TODO: execute trace exception if trace is enabled
		//

		//TODO: stop execution and wait for interrupt
		ctx.EnterStopState();
	}

	virtual bool CalcSize(InstructionSize size, const EffectiveAddress& ea_src, const EffectiveAddress& ea_dst, uint32& len) const
	{
		len = 2 + 2;	// opcode + data
		return true;
	}

	virtual void Encode(InstructionSize size, const EffectiveAddress& ea_src, const EffectiveAddress& ea_dst, OutputPointer& ctx) const
	{
		ctx << StencilCode();

		if (ea_src.mode_ == AM_IMMEDIATE)
			ctx << static_cast<uint16>(ea_src.val_.Value());
		else
			throw LogicError("unexpected addressing mode in " __FUNCTION__);
	}
};

static Instruction* instr1= GetInstructions().Register(new Stop());
