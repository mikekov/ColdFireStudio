/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "Register.h"
#include "../EmitCode.h"


class Link : public InstructionImpl<Stencil_REG>
{
public:
	Link() : InstructionImpl("LINK", IParam().Sizes(IS_WORD).SrcModes(AM_Ax).DestModes(AM_IMMEDIATE).Opcode(0x4e50).Mask(Stencil::MASK))
	{}

	virtual DecodedInstruction Decode(InstrPointer& ctx) const
	{
		DecodedInstruction out(Mnemonic(), S_WORD, ctx.OpCode());
		uint32 ext= SignExtendWord(ctx.GetNextWord());
		out.src_ = EffectiveAddress_AReg(GetRegisterCode(ctx.OpCode(), 0));
		out.dest_ = EffectiveAddress_Imm(ext, true);

		return out;
	}

	virtual void Execute(Context& ctx) const
	{
		Stencil o;
		ctx.OpCode(o);

		uint32 disp= SignExtendWord(ctx.GetNextPCWord());

		uint32& ax= ctx.Cpu().a_reg[o.reg_index];
		uint32& sp= ctx.Cpu().a_reg[7];

		ctx.PushLongWord(ax);
		ax = sp;
		sp += disp;
	}

	virtual void Encode(InstructionSize size, const EffectiveAddress& ea_src, const EffectiveAddress& ea_dst, OutputPointer& ctx) const
	{
		if (ea_src.mode_ == AM_Ax && ea_dst.mode_ == AM_IMMEDIATE)
		{
			uint16 reg= AddressRegToNumber(ea_src.first_reg_);
			ctx << (StencilCode() | reg);
			ctx << static_cast<uint16>(ea_dst.val_.Value() & 0xffff);
		}
		else
			throw LogicError("illegal addressing mode in " __FUNCTION__);
	}

	virtual bool CalcSize(InstructionSize size, const EffectiveAddress& ea_src, const EffectiveAddress& ea_dst, uint32& len) const
	{
		len = 2 + 2;	// LINK occupies 4 bytes
		return true;
	}
};


static Instruction* instr1= GetInstructions().Register(new Link());


class Unlink : public InstructionImpl<Stencil_REG>
{
public:
	Unlink() : InstructionImpl("UNLK", IParam().SrcModes(AM_NONE).DestModes(AM_Ax).Opcode(0x4e58).Mask(Stencil::MASK))
	{}

	virtual DecodedInstruction Decode(InstrPointer& ctx) const
	{
		DecodedInstruction out(Mnemonic(), ctx.OpCode());
		out.dest_ = EffectiveAddress_AReg(GetRegisterCode(ctx.OpCode(), 0));

		return out;
	}

	virtual void Execute(Context& ctx) const
	{
		auto o= OpCode(ctx);

		uint32& ax= ctx.Cpu().a_reg[o.reg_index];
		uint32& sp= ctx.Cpu().a_reg[7];

		sp = ax;
		ax = ctx.PullLongWord();
	}

	virtual void Encode(InstructionSize size, const EffectiveAddress& ea_src, const EffectiveAddress& ea_dst, OutputPointer& ctx) const
	{
		if (ea_src.mode_ == AM_NONE && ea_dst.mode_ == AM_Ax)
		{
			uint16 reg= AddressRegToNumber(ea_dst.first_reg_);
			ctx << (StencilCode() | reg);
		}
		else
			throw LogicError("illegal addressing mode in " __FUNCTION__);
	}
};


static Instruction* instr2= GetInstructions().Register(new Unlink());
