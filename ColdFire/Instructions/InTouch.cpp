/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "Register.h"
#include "..\EmitCode.h"


class InTouch : public InstructionImpl<Stencil_REG>
{
public:
	InTouch() : InstructionImpl("INTOUCH", IParam().SrcModes(AM_NONE).DestModes(AM_INDIRECT_Ax).Opcode(0xf428).Mask(Stencil::MASK).Isa(ISA::B))
	{}

	virtual DecodedInstruction Decode(InstrPointer& ctx) const
	{
		auto o= OpCode(ctx);
		DecodedInstruction out(Mnemonic(), S_NA, o.opcode);
		out.dest_ = DasmEffectiveAddress(AM_INDIRECT_Ax, o.reg_index, 0, false);
		return out;
	}

	virtual void Execute(Context& ctx) const
	{
		//TODO:
		// fetch word
	}

	virtual bool CalcSize(InstructionSize size, const EffectiveAddress& ea_src, const EffectiveAddress& ea_dst, uint32& len) const
	{
		len = 2;	// address register encoded in the opcode
		return true;
	}

	virtual void Encode(InstructionSize size, const EffectiveAddress& ea_src, const EffectiveAddress& ea_dst, OutputPointer& ctx) const
	{
		Stencil s;
		s.opcode= StencilCode();

		if (ea_dst.mode_ == AM_INDIRECT_Ax)
			s.reg_index = ea_dst.first_reg_;
		else
			throw LogicError("unexpected addressing mode in " __FUNCTION__);

		ctx << s.opcode;
	}

};


static Instruction* instr1= GetInstructions().Register(new InTouch());
