/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "Register.h"


class Trap : public InstructionImpl<Stencil_DATA>
{
public:
	Trap() : InstructionImpl("TRAP", IParam().SrcModes(AM_IMMEDIATE).ImmDataRange(0, 15).Opcode(0x4e40).Mask(0x000f).ControlFlow(IControlFlow::SUBROUTINE))
	{}

	virtual DecodedInstruction Decode(InstrPointer& ctx) const
	{
		Stencil o= OpCode(ctx);

		DecodedInstruction out(Mnemonic(), o.opcode);
		out.src_ = DasmEffectiveAddress(AM_IMMEDIATE, o.data);

		return out;
	}

	virtual void Execute(Context& ctx) const
	{
		Stencil o= OpCode(ctx);

		ctx.EnterException(static_cast<CpuExceptions>(EX_Trap_0 + o.data), ctx.Cpu().pc - 2);
	}

	virtual void Encode(InstructionSize size, const EffectiveAddress& ea_src, const EffectiveAddress& ea_dst, OutputPointer& ctx) const
	{
		ASSERT(ea_src.mode_ == AM_IMMEDIATE && ea_src.val_.inf == Expr::EX_BYTE);
		// vector number:
		uint16 vector_no= ea_src.val_.value & 0xf;
		ctx << (StencilCode() | vector_no);
	}

	virtual bool CalcSize(InstructionSize size, const EffectiveAddress& ea_src, const EffectiveAddress& ea_dst, uint32& len) const
	{
		len = 2;	// TRAP #v occupies 2 bytes
		return true;
	}
};


static Instruction* instr= GetInstructions().Register(new Trap());
