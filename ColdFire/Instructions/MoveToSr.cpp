/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "Register.h"
#include "..\EmitCode.h"


class MoveToSr : public InstructionImpl<Stencil_EA>
{
public:
	MoveToSr(bool dx) : InstructionImpl("MOVE", IParam().Sizes(IS_WORD).SrcModes(dx ? AM_Dx : AM_IMMEDIATE).DestModes(AM_SPEC_REG).SpecReg(R_SR).Opcode(dx ? 0x46c0 : 0x46fc).Mask(dx ? 0x0007 : 0x0).ImmDataSize(S_WORD).Privileged())
	{}

	virtual DecodedInstruction Decode(InstrPointer& ctx) const
	{
		DecodedInstruction out(Mnemonic(), S_WORD, ctx.OpCode());
		out.DecodeSrcEA(3, 0, ctx);
		out.dest_ = EffectiveAddress_SpecReg(REG_SR);

		return out;
	}

	virtual void Execute(Context& ctx) const
	{
		auto o= OpCode(ctx);
		int words= 0;
		uint16 val= ctx.DecodeSrcWordValue(o.ea_mode, S_WORD, words);
		ctx.Cpu().SetSR(val);
		ctx.StepPC(words);
	}

	virtual bool CalcSize(InstructionSize size, const EffectiveAddress& ea_src, const EffectiveAddress& ea_dst, uint32& len) const
	{
		if (ea_src.mode_ == AM_IMMEDIATE)
		{
			if (ea_src.val_.inf == Expr::EX_WORD || ea_src.val_.inf == Expr::EX_UNDEF)
				return Instruction::CalcSize(IS_WORD, ea_src, ea_dst, len);
			return false;
		}
		else
			return Instruction::CalcSize(size, ea_src, ea_dst, len);
	}

	virtual void Encode(InstructionSize size, const EffectiveAddress& ea_src, const EffectiveAddress& ea_dst, OutputPointer& ctx) const
	{
		Emit_EA(StencilCode(), ea_src, ctx);
	}
};


static Instruction* instr10= GetInstructions().Register(new MoveToSr(true));
static Instruction* instr11= GetInstructions().Register(new MoveToSr(false));



class MoveFromSr : public InstructionImpl<Stencil_REG>
{
public:
	MoveFromSr() : InstructionImpl("MOVE", IParam().Sizes(IS_WORD).SrcModes(AM_SPEC_REG).SpecReg(R_SR).DestModes(AM_Dx).Opcode(0x40c0).Mask(Stencil::MASK).Privileged())
	{}

	virtual DecodedInstruction Decode(InstrPointer& ctx) const
	{
		DecodedInstruction out(Mnemonic(), S_WORD, ctx.OpCode());
		out.src_ = EffectiveAddress_SpecReg(REG_SR);
		out.dest_ = EffectiveAddress_DReg(GetRegisterCode(ctx.OpCode(), 0));

		return out;
	}

	virtual void Execute(Context& ctx) const
	{
		auto o= OpCode(ctx);
		auto& reg= ctx.Cpu().d_reg[o.reg_index];
		reg = (reg & ~0xffff) | ctx.Cpu().GetSR();
	}

	virtual void Encode(InstructionSize size, const EffectiveAddress& ea_src, const EffectiveAddress& ea_dst, OutputPointer& ctx) const
	{
		if (ea_src.mode_ == AM_SPEC_REG && ea_src.first_reg_ == R_SR && ea_dst.mode_ == AM_Dx)
			Emit_Dx(StencilCode(), ea_dst.first_reg_, ctx);
		else
			throw LogicError("unexpected addressing mode in " __FUNCTION__);
	}
};


static Instruction* instr2= GetInstructions().Register(new MoveFromSr());


class MoveToCcr : public InstructionImpl<Stencil_EA>
{
public:
	MoveToCcr(bool dx) : InstructionImpl("MOVE", IParam().Sizes(IS_BYTE).DefaultSize(IS_BYTE).SrcModes(dx ? AM_Dx : AM_IMMEDIATE).DestModes(AM_SPEC_REG).SpecReg(R_CCR).Opcode(dx ? 0x44c0 : 0x44fc).Mask(dx ? 0x07 : 0x0).ImmDataSize(S_BYTE))
	{}

	virtual DecodedInstruction Decode(InstrPointer& ctx) const
	{
		auto o= OpCode(ctx);
		DecodedInstruction out(Mnemonic(), S_BYTE, o.opcode);
		out.DecodeSrcEA(o.ea_mode, ctx);
		out.dest_ = EffectiveAddress_SpecReg(REG_CCR);

		return out;
	}

	virtual void Execute(Context& ctx) const
	{
		auto o= OpCode(ctx);
		int words= 0;
		uint16 val= ctx.DecodeSrcWordValue(o.ea_mode, S_WORD, words);
		ctx.Cpu().SetSR(val);
		ctx.StepPC(words);
	}

	virtual void Encode(InstructionSize size, const EffectiveAddress& ea_src, const EffectiveAddress& ea_dst, OutputPointer& ctx) const
	{
		Emit_EA(StencilCode(), ea_src, ctx);
	}
};


class MoveFromCcr : public InstructionImpl<Stencil_REG>
{
public:
	MoveFromCcr() : InstructionImpl("MOVE", IParam().Sizes(IS_WORD).SrcModes(AM_SPEC_REG).DestModes(AM_Dx).SpecReg(R_CCR).Opcode(0x42c0).Mask(0x07))
	{}

	virtual DecodedInstruction Decode(InstrPointer& ctx) const
	{
		auto o= OpCode(ctx);
		DecodedInstruction out(Mnemonic(), S_WORD, o.opcode);
		out.src_ = EffectiveAddress_SpecReg(REG_CCR);
		out.dest_ = EffectiveAddress_DReg(o.reg_index);

		return out;
	}

	virtual void Execute(Context& ctx) const
	{
		auto o= OpCode(ctx);
		auto& reg= ctx.Cpu().d_reg[o.reg_index];
		reg &= ~0xffff;
		reg |= ctx.Cpu().GetSR() & 0xff;
	}

	virtual void Encode(InstructionSize size, const EffectiveAddress& ea_src, const EffectiveAddress& ea_dst, OutputPointer& ctx) const
	{
		if (ea_src.mode_ == AM_SPEC_REG && ea_src.first_reg_ == R_CCR && ea_dst.mode_ == AM_Dx)
			Emit_Dx(StencilCode(), ea_dst.first_reg_, ctx);
		else
			throw LogicError("unexpected addressing mode in " __FUNCTION__);
	}
};

static Instruction* instr21= GetInstructions().Register(new MoveToCcr(true));
static Instruction* instr22= GetInstructions().Register(new MoveToCcr(false));
static Instruction* instr23= GetInstructions().Register(new MoveFromCcr());


class MoveFromUsp : public InstructionImpl<Stencil_REG>
{
public:
	MoveFromUsp() : InstructionImpl("MOVE", IParam().Sizes(IS_LONG).SrcModes(AM_SPEC_REG).SpecReg(R_USP).DestModes(AM_Ax).Opcode(0x4e68).Mask(Stencil::MASK).Privileged())
	{}

	virtual DecodedInstruction Decode(InstrPointer& ctx) const
	{
		auto o= OpCode(ctx);
		DecodedInstruction out(Mnemonic(), S_LONG, o.opcode);
		out.src_ = EffectiveAddress_SpecReg(REG_USP);
		out.dest_ = EffectiveAddress_AReg(o.reg_index);

		return out;
	}

	virtual void Execute(Context& ctx) const
	{
		auto o= OpCode(ctx);
		ctx.Cpu().a_reg[o.reg_index] = ctx.Cpu().GetUSP();
	}

	virtual void Encode(InstructionSize size, const EffectiveAddress& ea_src, const EffectiveAddress& ea_dst, OutputPointer& ctx) const
	{
		if (ea_src.mode_ == AM_SPEC_REG && ea_src.first_reg_ == R_USP && ea_dst.mode_ == AM_Ax)
			Emit_Ax(StencilCode(), ea_dst.first_reg_, ctx);
		else
			throw LogicError("unexpected addressing mode in " __FUNCTION__);
	}
};


static Instruction* instr3= GetInstructions().Register(new MoveFromUsp());


class MoveToUsp : public InstructionImpl<Stencil_REG>
{
public:
	MoveToUsp() : InstructionImpl("MOVE", IParam().Sizes(IS_LONG).SrcModes(AM_Ax).DestModes(AM_SPEC_REG).SpecReg(R_USP).Opcode(0x4e60).Mask(Stencil::MASK).Privileged())
	{}

	virtual DecodedInstruction Decode(InstrPointer& ctx) const
	{
		auto o= OpCode(ctx);
		DecodedInstruction out(Mnemonic(), S_LONG, o.opcode);
		out.DecodeSrcEAReg(o.reg_index + 8);
		out.dest_ = EffectiveAddress_SpecReg(REG_USP);

		return out;
	}

	virtual void Encode(InstructionSize size, const EffectiveAddress& ea_src, const EffectiveAddress& ea_dst, OutputPointer& ctx) const
	{
		assert(ea_src.mode_ == AM_Ax);
		assert(ea_dst.mode_ == AM_SPEC_REG);

		Emit_Ax(StencilCode(), ea_src.first_reg_, ctx);
	}

	virtual void Execute(Context& ctx) const
	{
		auto o= OpCode(ctx);
		ctx.Cpu().SetUSP(ctx.Cpu().a_reg[o.reg_index]);
	}
};


static Instruction* instr4= GetInstructions().Register(new MoveToUsp());


class StrLdSR : public InstructionImpl<Stencil_UNIQUE>
{
public:
	StrLdSR() : InstructionImpl("STRLDSR", IParam().SrcModes(AM_IMMEDIATE).Opcode(0x40e7).Privileged().Isa(ISA::C))
	{}

	virtual DecodedInstruction Decode(InstrPointer& ctx) const
	{
		auto o= OpCode(ctx);
		DecodedInstruction out(Mnemonic(), S_LONG, o.opcode);
		auto opcode2= ctx.GetNextWord();
		if (opcode2 != MOVE_TO_SR)
		{
			auto data= ctx.GetNextWord();
			return DecodedInstruction();
		}
		out.dest_ = EffectiveAddress_Imm(ctx, S_WORD, false);
		return out;
	}

	virtual bool CalcSize(InstructionSize size, const EffectiveAddress& ea_src, const EffectiveAddress& ea_dst, uint32& len) const
	{
		if (ea_src.mode_ == AM_IMMEDIATE)
		{
			if (ea_src.val_.inf == Expr::EX_WORD || ea_src.val_.inf == Expr::EX_UNDEF)
				return Instruction::CalcSize(IS_WORD, ea_src, ea_dst, len);
		}
		return false;
	}

	virtual void Encode(InstructionSize size, const EffectiveAddress& ea_src, const EffectiveAddress& ea_dst, OutputPointer& ctx) const
	{
		assert(ea_src.mode_ == AM_IMMEDIATE);
		assert(ea_dst.mode_ == AM_NONE);

		ctx << StencilCode();
		ctx << uint16(MOVE_TO_SR);
		ctx << uint16(ea_src.val_.Value());
	}

	virtual void Execute(Context& ctx) const
	{
		uint16 next_opcode= ctx.GetNextPCWord();
		uint16 data= ctx.GetNextPCWord();

		if (next_opcode != MOVE_TO_SR)
		{ } // then what?

		//TODO
		assert(false);
	}

private:
	enum { MOVE_TO_SR = 0x46fc };
};

static Instruction* instr5= GetInstructions().Register(new StrLdSR());
