/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "Register.h"
#include "..\EmitCode.h"


class Shift : public InstructionImpl<Stencil_SHIFTS>
{
public:
	Shift(uint16 opcode_stencil, const char* name)
		: InstructionImpl(name, IParam().Sizes(IS_LONG).SrcModes(AM_IMMEDIATE | AM_Dx).ImmDataRange(1, 8).DestModes(AM_Dx).Opcode(opcode_stencil).Mask(Stencil::MASK))
	{}

	virtual DecodedInstruction Decode(InstrPointer& ctx) const
	{
		Stencil o= OpCode(ctx);

		DecodedInstruction out(Mnemonic(), S_LONG, o.opcode);

		int count= o.count_or_reg; //GetRegisterCode(ctx.OpCode(), 9);

		if (o.imm_reg) // ctx.OpCode() & 0x0020)
		{
			// shift in register
			out.src_ = EffectiveAddress_DReg(count);
		}
		else
		{
			// immediate shift
			if (count == 0)
				count = 8;
			out.src_ = EffectiveAddress_Imm(count, true);	// data is unsigned, but signed output is shorter
		}

		out.dest_ = EffectiveAddress_DReg(o.reg_index);// GetRegisterCode(ctx.OpCode(), 0));

		return out;
	}

	virtual void Execute(Context& ctx) const
	{
		Stencil o= OpCode(ctx);

		int shift= 0;

		if (o.imm_reg)
		{
			// shift in a register (modulo 64)
			shift = ctx.Cpu().d_reg[o.count_or_reg] & 0x3f;
		}
		else
		{
			shift = o.count_or_reg;
			if (shift == 0)
				shift = 8;
		}

		uint32& dx= ctx.Cpu().d_reg[o.reg_index];
		uint32 last_bit= 0;

		if (o.direction)
		{
			// shift left

			if (shift <= 32)
				last_bit = dx & (0x80000000 >> (shift - 1));

			if (o.logical)
				dx <<= uint32(shift);
			else
				dx = int32(dx) << shift;
		}
		else
		{
			// shift right

			if (shift <= 32)
				last_bit = dx & (1 << (shift - 1));

			if (shift < 32)
			{
				if (o.logical)
					dx >>= uint32(shift);
				else
					dx = int32(dx) >> shift;
			}
			else
				dx = 0;
		}

		ctx.SetNZ(dx);
		ctx.SetOverflow(false);
		ctx.SetCarry(shift ? last_bit : 0);
		if (shift)
			ctx.SetExtend(last_bit);
	}

	virtual void Encode(InstructionSize size, const EffectiveAddress& ea_src, const EffectiveAddress& ea_dst, OutputPointer& ctx) const
	{
		if (ea_src.mode_ == AM_Dx && ea_dst.mode_ == AM_Dx)
		{
			const uint16 reg_shift_count= 0x0020;
			uint16 opcode= StencilCode() | reg_shift_count;
			Emit_Dx_Dy(opcode, ea_src.first_reg_, 9, ea_dst.first_reg_, 0, ctx);
		}
		else if (ea_src.mode_ == AM_IMMEDIATE && ea_dst.mode_ == AM_Dx)
		{
			uint16 shift= ea_src.val_.Value();
			if (shift == 8)
				shift = 0;
			uint16 opcode= StencilCode() | (shift << 9) | DataRegToNumber(ea_dst.first_reg_);
			ctx << opcode;
		}
		else
			throw LogicError("illegal mode in " __FUNCTION__);
	}

	virtual bool CalcSize(InstructionSize size, const EffectiveAddress& ea_src, const EffectiveAddress& ea_dst, uint32& len) const
	{
		// shift instructions have immediate argument and destination register encoded inside the opcode
		//   ASd.L Dy,Dx
		//   ASd.L #<data>,Dx
		// both forms are 2 bytes long

		len = 2;
		return true;
	}
};


static Instruction* instr1= GetInstructions().Register(new Shift(0xe080, "ASR"));
static Instruction* instr2= GetInstructions().Register(new Shift(0xe180, "ASL"));
static Instruction* instr3= GetInstructions().Register(new Shift(0xe088, "LSR"));
static Instruction* instr4= GetInstructions().Register(new Shift(0xe188, "LSL"));
