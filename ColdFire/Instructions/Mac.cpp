/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "Register.h"
#include "../EmitCode.h"

// MAC ISA -------------

// Operation:			ACC + (Ry * Rx){<<|>>} SF -> ACC
// Assembler syntax:	MAC.sz Ry.{U,L}, Rx.{U,L} SF

// Operation:			ACC + (Ry * Rx){<<|>>} SF -> ACC	(<ea>y) -> Rw
// Assembler syntax:	MAC.sz Ry.{U,L}, Rx.{U,L} SF, <ea>y&, Rw	(where & enables the use of the MASK)

// EMAC ISA ------------

// Operation:			ACCx + (Ry * Rx){<<|>>} SF -> ACCx
// Assembler syntax:	MAC.sz Ry.{U,L}, Rx.{U,L} SF, ACCx

// Operation:			ACCx + (Ry * Rx){<<|>>} SF -> ACCx	(<ea>y) -> Rw
// Assembler syntax:	MAC.sz Ry.{U,L}, Rx.{U,L} SF, <ea>y&, Rw, ACCx	(where & enables the use of the MASK)

#if BIT_FIELDS_LSB_TO_MSB

union MAC_ExtWord
{
	struct
	{
		uint16 reg_ry : 4;	// or not used
		uint16 acc_msb : 1;
		uint16 mask : 1;
		uint16 ul_y : 1;
		uint16 ul_x : 1;
		uint16 subtraction : 1;
		uint16 scale_factor : 2;
		uint16 size : 1;
		uint16 reg_rx : 4;	// or not used
	};
	uint16 word;
};

#else

#	error "define MAC_ExtWord"

#endif

static const char MADD[]= "MAC";
static const char MSUB[]= "MSAC";


class Mac : public InstructionImpl<Stencil_MAC_REG_REG>
{
public:
	Mac(bool use_acc, bool add) : InstructionImpl(add ? MADD : MSUB, IParam().Opcode(0xa000).Mask(use_acc ? Stencil::EMAC_MASK : Stencil::MAC_MASK).Sizes(IS_WORD | IS_LONG).SrcModes(AM_Rx_Ry).DestModes(use_acc ? AM_ACCx : AM_NONE).Isa(use_acc ? ISA::EMAC : ISA::MAC).ExcludeCodes(!add ? IExcludeCodes(0xffff, 0xffff) : IExcludeCodes(0, 0)))
	{
		use_acc_ = use_acc;
		add_ = add;		// Multiply Add/Subtract Accumulate
	}

	virtual DecodedInstruction Decode(InstrPointer& ctx) const
	{
		auto s= OpCode(ctx);
		MAC_ExtWord ext;
		ext.word = ctx.GetNextWord();

		InstrSize is= ext.size ? S_LONG : S_WORD;

		DecodedInstruction out(ext.subtraction ? MSUB : MADD, is, s.opcode);

		RegisterWord ul_y= RegisterWord::NONE;
		RegisterWord ul_x= RegisterWord::NONE;

		if (is == S_WORD)
		{
			ul_y = ext.ul_y ? RegisterWord::UPPER : RegisterWord::LOWER;
			ul_x = ext.ul_x ? RegisterWord::UPPER : RegisterWord::LOWER;
		}

		out.src_ = EffectiveAddress_RegY_RegX(s.reg_y, s.reg_x | (s.reg_x_msb ? 0x8 : 0), ul_x, ul_y);

		out.src_.scale_ = ext.scale_factor;

		if (use_acc_)
		{
			int index= s.acc_lsb + 2 * ext.acc_msb;
			SpecialRegister acc= static_cast<SpecialRegister>(REG_ACC0 + index);
			out.dest_ = EffectiveAddress_SpecReg(acc);
		}

		return out;
	}

	virtual void Encode(InstructionSize size, const EffectiveAddress& ea_src, const EffectiveAddress& ea_dst, OutputPointer& ctx) const
	{
		Stencil s;
		s.opcode= StencilCode();

		s.reg_y= RegisterToNumber(ea_src.first_reg_);
		auto reg_x= RegisterToNumber(ea_src.second_reg_);
		s.reg_x = reg_x & 0x7;
		s.reg_x_msb = reg_x & 0x8 ? 1 : 0;

		MAC_ExtWord ext;
		ext.word = 0;

		if (use_acc_)
		{
			//todo
			//
			s.acc_lsb = 0;
			ext.acc_msb = 0;
		}

		if (size == IS_LONG)
			ext.size = 1;

		switch (ea_src.shift_)
		{
		case ShiftFactor::LEFT:
			ext.scale_factor = 1;
			break;
		case ShiftFactor::RIGHT:
			ext.scale_factor = 3;
			break;
		default:
			break;
		}

		if (size == IS_WORD)
		{
			if (ea_src.first_reg_word_ == RegisterWord::UPPER)
				ext.ul_y = 1;

			if (ea_src.second_reg_word_ == RegisterWord::UPPER)
				ext.ul_x = 1;
		}

		if (!add_)
			ext.subtraction = 1;

		ctx << s.opcode;
		ctx << ext.word;
	}

	virtual bool CalcSize(InstructionSize size, const EffectiveAddress& ea_src, const EffectiveAddress& ea_dst, uint32& len) const
	{
		len = 2 + 2;
		return true;
	}

	virtual void Execute(Context& ctx) const
	{
		auto op= OpCode(ctx);
		MAC_ExtWord ext;
		ext.word = ctx.GetNextPCWord();


		assert(false);	// TODO
	}

private:
	bool use_acc_;
	bool add_;
};


// this is MAC with parallel transfer;
// it has two source addressing modes, and two destination addressing modes

class MacEa : public InstructionImpl<Stencil_REG_S_EA>
{
public:
	MacEa(bool use_acc, bool add) : InstructionImpl(add ? MADD : MSUB, IParam().Opcode(0xa080).Mask(use_acc ? Stencil::EMAC_MASK : Stencil::MAC_MASK).Sizes(IS_WORD | IS_LONG).SrcModes(AM_Rx_Ry).SndSrcModes(AM_INDIRECT_Ax | AM_Ax_INC | AM_DEC_Ax | AM_DISP_Ax | AM_MAC_MASK).DestModes(AM_Dx | AM_Ax).SndDestModes(use_acc ? AM_ACCx : AM_NONE).Isa(use_acc ? ISA::EMAC : ISA::MAC).ExcludeCodes(!add ? IExcludeCodes(0xffff, 0xffff) : IExcludeCodes(0x003f, 0x0000, 0x0008, 0x0030)))
	{
		use_acc_ = use_acc;
		add_ = add;		// Multiply Add/Subtract Accumulate
	}

	virtual bool CalcSize(InstructionSize size, const EffectiveAddress& ea_src, const EffectiveAddress& ea_dst, uint32& len) const
	{
		len = 2 + 2;
		return true;
	}

	virtual DecodedInstruction Decode(InstrPointer& ctx) const
	{
		auto s= OpCode(ctx);
		MAC_ExtWord ext;
		ext.word = ctx.GetNextWord();

		InstrSize is= ext.size ? S_LONG : S_WORD;

		DecodedInstruction out(ext.subtraction ? MSUB : MADD, is, s.opcode);

		RegisterWord ul_y= RegisterWord::NONE;
		RegisterWord ul_x= RegisterWord::NONE;

		if (is == S_WORD)
		{
			ul_y = ext.ul_y ? RegisterWord::UPPER : RegisterWord::LOWER;
			ul_x = ext.ul_x ? RegisterWord::UPPER : RegisterWord::LOWER;
		}

		out.src_ = EffectiveAddress_RegY_RegX(ext.reg_ry, ext.reg_rx, ul_x, ul_y);

		out.src_.scale_ = ext.scale_factor;

		if (use_acc_)
		{
			int index= s.reg_bit + 2 * ext.acc_msb;
			SpecialRegister acc= static_cast<SpecialRegister>(REG_ACC0 + index);
			out.dest_ = EffectiveAddress_SpecReg(acc);
		}

		//TODO
		if (ext.mask)
		{	}

		//TODO
		s.reg_index;

		//TODO
		s.ea_mode;

		return out;
	}

	virtual void Encode(InstructionSize size, const EffectiveAddress& ea_src, const EffectiveAddress& ea_snd_src, const EffectiveAddress& ea_dst, const EffectiveAddress& ea_snd_dst, OutputPointer& ctx) const
	{
		MAC_ExtWord ext;
		ext.word = 0;

		uint16 words[3];
		auto count= Encode_EA(StencilCode(), ea_snd_src, 3, 0, words);
		assert(count == 1 || count == 2);

		Stencil s;
		s.opcode = words[0];

		auto rw= RegisterToNumber(ea_dst.first_reg_);
		s.reg_index = rw & 0x7;
		s.reg_bit = rw >= 8;

		ext.reg_ry= RegisterToNumber(ea_src.first_reg_);
		ext.reg_rx= RegisterToNumber(ea_src.second_reg_);

		// use MAC mask
		ext.mask = ea_snd_src.mode_ & AM_MAC_MASK ? 1 : 0;

		if (use_acc_)
		{
			//todo
			//
			ext.acc_msb = 0;
		}

		if (size == IS_LONG)
			ext.size = 1;

		switch (ea_src.shift_)
		{
		case ShiftFactor::LEFT:
			ext.scale_factor = 1;
			break;
		case ShiftFactor::RIGHT:
			ext.scale_factor = 3;
			break;
		default:
			break;
		}

		if (size == IS_WORD)
		{
			if (ea_src.first_reg_word_ == RegisterWord::UPPER)
				ext.ul_y = 1;

			if (ea_src.second_reg_word_ == RegisterWord::UPPER)
				ext.ul_x = 1;
		}

		if (!add_)
			ext.subtraction = 1;

		ctx << s.opcode;
		ctx << ext.word;
		if (count > 1)
			ctx << words[1];
	}

	virtual void Execute(Context& ctx) const
	{
		auto op= OpCode(ctx);
		MAC_ExtWord ext;
		ext.word = ctx.GetNextPCWord();


		assert(false);	// TODO
	}

private:
	bool use_acc_;
	bool add_;
};


static Instruction* instr10= GetInstructions().Register(new Mac(false, true));	// MAC
static Instruction* instr11= GetInstructions().Register(new Mac(false, false));	// MSAC

static Instruction* instr20= GetInstructions().Register(new MacEa(false, true));	// MAC
static Instruction* instr21= GetInstructions().Register(new MacEa(false, false));	// MSAC
