/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "Register.h"
#include "..\EmitCode.h"


class MoveC : public InstructionImpl<Stencil_UNIQUE>
{
public:
	// move Dx or Ax to control register; control registers are write-only (without BDM)
	MoveC() : InstructionImpl("MOVEC", IParam().Sizes(IS_LONG).SrcModes(AM_Dx | AM_Ax).DestModes(AM_SPEC_REG).Opcode(0x4e7b).Privileged().SpecReg(R_CACR).SpecReg(R_ASID).SpecReg(R_ACR0).SpecReg(R_ACR1).SpecReg(R_ACR2).SpecReg(R_ACR3).SpecReg(R_MMUBAR).SpecReg(R_VBR).SpecReg(R_MBAR).SpecReg(R_ROMBAR0).SpecReg(R_ROMBAR1).SpecReg(R_RAMBAR0).SpecReg(R_RAMBAR1).SpecReg(R_PC)) // add more spec regs
	{}

	virtual DecodedInstruction Decode(InstrPointer& ctx) const
	{
		DecodedInstruction out(Mnemonic(), S_LONG, ctx.OpCode());
		uint16 ext= ctx.GetNextWord();
		out.DecodeSrcEAReg(ext >> 12);
		out.dest_ = EffectiveAddress_SpecReg(DecodeControlRegisterField(ext));

		return out;
	}

	virtual void Execute(Context& ctx) const
	{
		Stencil_ExtWord_REG_CREG ext;
		ext.word = ctx.GetNextPCWord();

		auto data= ctx.GetRegister(ext.ad_reg_index);

		// move data to the special register
		switch (DecodeControlRegisterField(ext.ctrl_reg))
		{
		case REG_CACR:
			// TODO
			// emulate bit for disabling user stack pointer
			break;

		case REG_VBR:
			ctx.Cpu().vbr = data;
			break;

		case REG_MBAR:
			ctx.Cpu().mbar = data & CPU::MBAR_ADDR_MASK;
			break;

		case REG_ACR0:	// access control registers
		case REG_ACR1:
			//TODO:
			// memory access control, cache mode, write protect, ...
			break;

		default:
			//TODO
			assert(false);
			break;
		}
	}

	virtual void Encode(InstructionSize size, const EffectiveAddress& ea_src, const EffectiveAddress& ea_dst, OutputPointer& ctx) const
	{
		ctx << StencilCode();
		Stencil_ExtWord_REG_CREG ext;
		ext.word = 0;
 		if (ea_src.mode_ == AM_Dx || ea_src.mode_ == AM_Ax)
			ext.ad_reg_index = RegisterToNumber(ea_src.first_reg_);
		else
			throw LogicError("Wrong src addressing mode in " __FUNCTION__);
		if (ea_dst.mode_ == AM_SPEC_REG)
		{
			auto code= RegisterToCode(ea_dst.first_reg_);
			if (code == 0)
				throw LogicError("Missing special register code in " __FUNCTION__);
			ext.ctrl_reg = code;
		}
		else
			throw LogicError("Wrong dest addressing mode in " __FUNCTION__);
		ctx << ext.word;
	}

	virtual bool CalcSize(InstructionSize size, const EffectiveAddress& ea_src, const EffectiveAddress& ea_dst, uint32& len) const
	{
		len = 2 + 2;
		return true;
	}
};


static Instruction* instr= GetInstructions().Register(new MoveC());
