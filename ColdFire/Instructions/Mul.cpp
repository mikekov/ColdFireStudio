/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "Register.h"
#include "../EmitCode.h"


class MulWord : public InstructionImpl<Stencil_REG_EA>
{
public:
	MulWord(bool signed_op) : InstructionImpl(signed_op ? "MULS" : "MULU", IParam().Sizes(IS_WORD).DefaultSize(IS_NONE).SrcModes(AM_ALL_SRC & ~AM_Ax).DestModes(AM_Dx).Opcode(signed_op ? 0xc1c0 : 0xc0c0).Mask(Stencil::MASK))
	{
		signed_op_ = signed_op;
	}

	virtual DecodedInstruction Decode(InstrPointer& ctx) const
	{
		DecodedInstruction out(Mnemonic(), S_WORD, ctx.OpCode());

		out.DecodeSrcEA(3, 0, ctx);
		out.dest_ = EffectiveAddress_DReg(GetRegisterCode(ctx.OpCode(), 9));

		return out;
	}

	virtual void Execute(Context& ctx) const
	{
		auto o= OpCode(ctx);

		int ext_words= 0;
		auto src= ctx.DecodeSrcWordValue(o.ea_mode, S_WORD, ext_words);
		// destination register
		auto& reg= ctx.Cpu().d_reg[o.reg_index];

		if (signed_op_)
			reg = SignExtendWord(reg & 0xffff) * SignExtendWord(src);
		else
			reg = (reg & 0xffff) * src;

		ctx.SetNZ_ClrCV(reg);
		ctx.StepPC(ext_words);
	}

private:
	bool signed_op_;
};


// long multiplication src modes are more restrictive than word MUL
const AddressingMode modes= AM_Dx | AM_INDIRECT_Ax | AM_Ax_INC | AM_DEC_Ax | AM_DISP_Ax;


class MulLng : public InstructionImpl<Stencil_EA>
{
public:
	MulLng(bool signed_op) : InstructionImpl(signed_op ? "MULS" : "MULU", IParam().Sizes(IS_LONG).DefaultSize(IS_NONE).SrcModes(modes).DestModes(AM_Dx).Opcode(0x4c00).Mask(Stencil::MASK).ExcludeCodes(signed_op ? IExcludeCodes(0xffff, 0xffff) : IExcludeCodes(0x0038, 0x0080)).Isa(ISA::A))
		// note - artificially remove all codes form MULS, b/c they are identical with MULU
	{
		signed_op_ = signed_op;
	}

	virtual DecodedInstruction Decode(InstrPointer& ctx) const
	{
		// the same instance of MulLng is used for decoding both MULU/MULS; check sign:
		Stencil_ExtWord_REG_OP ext;
		ext.word = ctx.GetNextWord();
		// one instruction (MULU) handles both multiplications (signed and unsigned),
		// b/c they have the same opcodes, only extension word is different
		DecodedInstruction out(ext.is_signed ? "MULS" : "MULU", S_LONG, ctx.OpCode());
		out.DecodeSrcEA(3, 0, ctx);
		out.dest_ = EffectiveAddress_DReg(ext.reg_index);
		return out;
	}

	virtual bool CalcSize(InstructionSize size, const EffectiveAddress& ea_src, const EffectiveAddress& ea_dst, uint32& len) const
	{
		auto ok= Instruction::CalcSize(size, ea_src, ea_dst, len);
		if (ok)
			len += 2;	// 1 extension word (2 bytes)
		return ok;
	}

	virtual void Encode(InstructionSize size, const EffectiveAddress& ea_src, const EffectiveAddress& ea_dst, OutputPointer& ctx) const
	{
		if (ea_dst.mode_ == AM_Dx)
		{
			Stencil_ExtWord_REG_OP ext;
			ext.word = 0;
			// MUL only uses first register in the extension word:
			ext.reg_index = ea_dst.first_reg_;
			ext.is_signed = signed_op_ ? 1 : 0;

			Emit_Ext_EA(StencilCode(), ext.word, ea_src, ctx);
		}
		else
			throw LogicError("illegal addressing mode or value in " __FUNCTION__);
	}

	virtual void Execute(Context& ctx) const
	{
		auto o= OpCode(ctx);

		Stencil_ExtWord_REG_OP ext;
		ext.word = ctx.GetNextPCWord();

		int ext_words= 0;
		auto src= ctx.DecodeSrcLongWordValue(o.ea_mode, S_LONG, ext_words);

		// destination register
		auto& reg= ctx.Cpu().d_reg[ext.reg_index];

		if (ext.is_signed)
			reg = uint32(int32(reg) * int32(src));
		else
			reg = reg * src;

		ctx.SetNZ_ClrCV(reg);
		ctx.StepPC(ext_words);
	}

private:
	bool signed_op_;
	const static uint16 SIGN_MASK= 0x0800;
};


static Instruction* instr10= GetInstructions().Register(new MulWord(false));
static Instruction* instr11= GetInstructions().Register(new MulWord(true));
static Instruction* instr20= GetInstructions().Register(new MulLng(true));
static Instruction* instr21= GetInstructions().Register(new MulLng(false));
