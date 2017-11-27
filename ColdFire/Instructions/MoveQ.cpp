/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "Register.h"
#include "../EmitCode.h"


class MoveQ : public InstructionImpl<Stencil_REG_DATA>
{
public:
	MoveQ() : InstructionImpl("MOVEQ", IParam().Sizes(IS_LONG).SrcModes(AM_IMMEDIATE).ImmDataRange(-0x7f - 1, 0x7f).DestModes(AM_Dx).Opcode(0x7000).Mask(Stencil::MASK))
	{}

	virtual DecodedInstruction Decode(InstrPointer& ctx) const
	{
		DecodedInstruction out(Mnemonic(), S_LONG, ctx.OpCode());
		out.src_ = EffectiveAddress_Imm(SignExtendByte(ctx.OpCode() & 0xff), true);
		out.dest_ = EffectiveAddress_DReg(GetRegisterCode(ctx.OpCode(), 9));

		return out;
	}

	virtual void Execute(Context& ctx) const
	{
		Stencil o;
		ctx.OpCode(o);

		uint32 data= SignExtendByte(o.data);
		ctx.Cpu().d_reg[o.reg_index] = data;
		ctx.SetNZ_ClrCV(data);
	}

	virtual void Encode(InstructionSize size, const EffectiveAddress& ea_src, const EffectiveAddress& ea_dst, OutputPointer& ctx) const
	{
		if (ea_src.mode_ == AM_IMMEDIATE && ea_src.val_.IsNumber() && ea_dst.mode_ == AM_Dx)
		{
			uint16 reg= DataRegToNumber(ea_dst.first_reg_);
			int32 val= ea_src.val_.Value();
			uint16 opcode= StencilCode();
			opcode |= reg << 9;
			opcode |= val & 0xff;
			ctx << opcode;
		}
		else
			throw LogicError("illegal addressing mode or value in " __FUNCTION__);
	}

	virtual bool CalcSize(InstructionSize size, const EffectiveAddress& ea_src, const EffectiveAddress& ea_dst, uint32& len) const
	{
		return Instruction::CalcSize(size, EffectiveAddress(), ea_dst, len);
	}
};


static Instruction* instr1= GetInstructions().Register(new MoveQ());



class Mov3Q : public InstructionImpl<Stencil_REG_EA>
{
public:
	Mov3Q() : InstructionImpl("MOV3Q", IParam().Sizes(IS_LONG).SrcModes(AM_IMMEDIATE).ImmDataRange(-1, 7, 0).DestModes(AM_ALL_DST).Opcode(0xa140).Mask(Stencil::MASK).Isa(ISA::B))
	{}

	virtual DecodedInstruction Decode(InstrPointer& ctx) const
	{
		DecodedInstruction out(Mnemonic(), S_LONG, ctx.OpCode());
		uint8 value= static_cast<uint8>((ctx.OpCode() >> 9) & 0x7);
		if (value == 0)
			value = 0xff;	// -1
		out.src_ = EffectiveAddress_Imm(SignExtendByte(value), true);
		out.DecodeDestEA(3, 0, ctx);
		return out;
	}

	virtual void Execute(Context& ctx) const
	{
		Stencil o;
		ctx.OpCode(o);

		uint32 data= o.reg_index;	// immediate data
		if (data == 0)
			data = ~uint32(0);		// 0 means -1

		int ext_words= 0;
		ctx.DecodeAndSetDestLongWord(o.ea_mode, S_LONG, ext_words, data);
		ctx.SetNZ_ClrCV(data);
		ctx.StepPC(ext_words);
	}

	virtual void Encode(InstructionSize size, const EffectiveAddress& ea_src, const EffectiveAddress& ea_dst, OutputPointer& ctx) const
	{
		if (ea_src.mode_ == AM_IMMEDIATE && ea_src.val_.IsNumber())
		{
			// -1 is encoded as 0
			uint16 val= ea_src.val_.value == -1 ? 0 : static_cast<uint16>(ea_src.val_.value);
			Emit_Imm_EA(StencilCode(), val, 9, ea_dst, ctx);
		}
		else
			throw LogicError("illegal addressing mode or value in " __FUNCTION__);
	}

	virtual bool CalcSize(InstructionSize size, const EffectiveAddress& ea_src, const EffectiveAddress& ea_dst, uint32& len) const
	{
		return Instruction::CalcSize(size, EffectiveAddress(), ea_dst, len);
	}
};

static Instruction* instr2= GetInstructions().Register(new Mov3Q());
