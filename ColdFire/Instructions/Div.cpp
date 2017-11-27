/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "Register.h"
#include "../EmitCode.h"


class DivWord : public InstructionImpl<Stencil_REG_EA>
{
public:
	DivWord(bool signed_op) : InstructionImpl(signed_op ? "DIVS" : "DIVU", IParam().Sizes(IS_WORD).DefaultSize(IS_NONE).SrcModes(AM_ALL_SRC & ~AM_Ax).DestModes(AM_Dx).Opcode(signed_op ? 0x81c0 : 0x80c0).Mask(Stencil::MASK))
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

		if (src == 0)
		{
			ctx.EnterException(EX_DivideByZero, ctx.Cpu().pc - 2);
			return;
		}

		// destination register
		auto& reg= ctx.Cpu().d_reg[o.reg_index];

		if (signed_op_)
		{
			// signed division
			// note that all 32 bits of destination register are used by 16-bit divide opcode!

			//TODO: check with real hardware what is the remainder sign for negative source/dest combinations
			int32 rem= int32(reg) % int16(src);
			int32 div= int32(reg) / int16(src);

			// check corner cases: quotient overflowing 16-bit integers
			// TODO: what about reminder larger than 16 bits?
			if (div > 0x7fff || div < -0x8000)
			{
				// overflow; leave destination register unaffected
				ctx.SetOverflow(true);
				ctx.SetNegative(false);
				ctx.SetZero(false);
				ctx.SetCarry(false);
			}
			else
			{
				reg = (uint32(rem) << 16) | (uint32(div) & 0xffff);
				ctx.SetNZ_ClrCV(reg, 0x8000);
			}
		}
		else
		{
			// unsigned division

			auto rem= reg % src;
			auto div= reg / src;

			if (div > 0xffff)
			{
				// overflow; leave destination register unaffected
				ctx.SetOverflow(true);
				ctx.SetNegative(false);
				ctx.SetZero(false);
				ctx.SetCarry(false);
			}
			else
			{
				reg = (rem << 16) | (div & 0xffff);
				ctx.SetNZ_ClrCV(reg, 0x8000);
			}
		}

		ctx.StepPC(ext_words);
	}

private:
	bool signed_op_;
};


// long multiplication src modes are more restrictive than word MUL
const AddressingMode modes= AM_Dx | AM_INDIRECT_Ax | AM_Ax_INC | AM_DEC_Ax | AM_DISP_Ax;


class DivLng : public InstructionImpl<Stencil_EA>
{
public:
	DivLng(bool signed_op, bool rem) : InstructionImpl(rem ? (signed_op ? "REMS" : "REMU") : (signed_op ? "DIVS" : "DIVU"), IParam().Sizes(IS_LONG).DefaultSize(IS_NONE).SrcModes(modes).DestModes(rem ? AM_Dx_Dy : AM_Dx).Opcode(0x4c40).Mask(Stencil::MASK).ExcludeCodes(signed_op || rem ? IExcludeCodes(0xffff, 0xffff) : IExcludeCodes(0x0038, 0x0080)).Isa(ISA::A))
	// note - remove all codes form DIVS/REMS/REMU, b/c they are identical with DIVU.L
	{
		signed_op_ = signed_op;
		rem_instr_ = rem;
	}

	virtual DecodedInstruction Decode(InstrPointer& ctx) const
	{
		// same instance of DivLng is used for decoding both MULU/MULS; check sign:
		Stencil_ExtWord_REG_OP ext;
		ext.word = ctx.GetNextWord();
		int is_signed= ext.is_signed;
		bool div= ext.reg_index == ext.reg_index2;
		auto o= OpCode(ctx);
		DecodedInstruction out(div ? (is_signed ? "DIVS" : "DIVU") : (is_signed ? "REMS" : "REMU"), S_LONG, o.opcode);
		out.DecodeSrcEA(o.ea_mode, ctx);
		if (div)
			out.dest_ = EffectiveAddress_DReg(ext.reg_index);
		else	// REM: dx:dy; hack: report mode as AM_Dx_Dy | AM_Dx for disassembler to accept it or else it will be rejected
			out.dest_ = DasmEffectiveAddress(AM_Dx_Dy | AM_Dx, ext.reg_index, ext.reg_index2, false);

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
		Stencil_ExtWord_REG_OP ext;
		ext.word = 0;
		ext.reg_index = ea_dst.first_reg_;

		// both register numbers have to be the same for long division (if they are different REM is performed instead)
		if (ea_dst.mode_ == AM_Dx)
			ext.reg_index2 = ea_dst.first_reg_;
		else if (ea_dst.mode_ == AM_Dx_Dy)
			ext.reg_index2 = ea_dst.second_reg_;
		else
			throw LogicError("illegal addressing mode or value in " __FUNCTION__);

		ext.is_signed = signed_op_ ? 1 : 0;

		Emit_Ext_EA(StencilCode(), ext.word, ea_src, ctx);
	}

	virtual void Execute(Context& ctx) const
	{
		auto o= OpCode(ctx);

		Stencil_ExtWord_REG_OP ext;
		ext.word = ctx.GetNextPCWord();

		int ext_words= 0;
		auto src= ctx.DecodeSrcLongWordValue(o.ea_mode, S_LONG, ext_words);

		if (src == 0)
		{
			ctx.EnterException(EX_DivideByZero, ctx.Cpu().pc - 2);
			return;
		}

		// destination register(s)
		auto& dx= ctx.Cpu().d_reg[ext.reg_index2];
		auto& dw= ctx.Cpu().d_reg[ext.reg_index];
		bool rem_op= &dx != &dw;

		if (signed_op_)
		{
			// signed division

			//TODO: check with real hardware what is the remainder sign for negative sorce/dest combinations
			uint32 rem= int32(dx) % int32(src);
			uint32 div= int32(dx) / int32(src);

			// check corner case
			if (dx == 0x80000000 && src == uint32(-1))
			{
				// overflow
				ctx.SetOverflow(true);
				ctx.SetNegative(false);
				ctx.SetZero(false);
				ctx.SetCarry(false);
			}
			else
			{
				dx = div;
				if (rem_op)
					dw = rem;
				ctx.SetNZ_ClrCV(div, 0x8000);
			}
		}
		else
		{
			// unsigned division

			auto rem= dx % src;
			auto div= dx / src;

			dx = div;
			if (rem_op)
				dw = rem;

			ctx.SetNZ_ClrCV(div, 0x8000);
		}

		ctx.StepPC(ext_words);
	}

private:
	bool signed_op_;
	bool rem_instr_;
	const static uint16 SIGN_MASK= 0x0800;
};


static Instruction* instr10= GetInstructions().Register(new DivWord(true));
static Instruction* instr11= GetInstructions().Register(new DivWord(false));
static Instruction* instrDivSLong= GetInstructions().Register(new DivLng(true, false));
static Instruction* instrDivULong= GetInstructions().Register(new DivLng(false, false));
static Instruction* instr30= GetInstructions().Register(new DivLng(false, true));
static Instruction* instr31= GetInstructions().Register(new DivLng(true, true));
