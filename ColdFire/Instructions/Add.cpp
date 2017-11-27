/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "Register.h"


class Add : public InstructionImpl<Stencil_REG_OP_EA>
{
public:
	Add(uint16 opcode_stencil, AddressingMode src, AddressingMode dst)
		: InstructionImpl("ADD", IParam().Sizes(IS_LONG).SrcModes(src).DestModes(dst).Opcode(opcode_stencil).Mask(0x0e3f).ExcludeCodes((dst & AM_Dx) ? IExcludeCodes() : IExcludeCodes(0x38, 0, 0x80)))
	{}

	virtual DecodedInstruction Decode(InstrPointer& ctx) const
	{
		Stencil o= OpCode(ctx);

		int opmode= o.op_mode;

		DecodedInstruction out(Mnemonic(), S_LONG, ctx.OpCode());

		if (opmode == 2)	// <ea>y + Dx -> Dx
		{
			out.DecodeSrcEA(o.ea_mode, ctx);
			out.dest_ = EffectiveAddress_DReg(o.reg_index);
		}
		else if (opmode == 6)	// Dy + <ea>x -> <ea>x
		{
			out.src_ = EffectiveAddress_DReg(o.reg_index);
			out.DecodeDestEA(o.ea_mode, ctx);
		}
		else
		{
			throw LogicError("Add instr - bad code");
		}

		return out;
	}

	virtual void Execute(Context& ctx) const
	{
		Stencil o= OpCode(ctx);

		int ext_words= 0;
		InstrSize size= S_LONG;
		uint32 src, dst;

		switch (o.op_mode)
		{
		case 2:		// long word, <ea>y + Dx -> Dx
			{
				src = ctx.DecodeSrcLongWordValue(o.ea_mode, size, ext_words);
				uint32& reg= ctx.Cpu().d_reg[o.reg_index];
				dst = reg;
				reg = src + dst;
			}
			break;

		case 6:		// long word, Dy + <ea>x -> <ea>x
			{
				src = ctx.GetDataRegister(o.reg_index);
				DecodedAddress da= ctx.DecodeMemoryAddress(o.ea_mode, size, ext_words);
				dst = ctx.ReadFromAddress(da, size);
				ctx.WriteToAddress(da, src + dst, size);
			}
			break;

		default:
			throw LogicError("unsupported operation code in ADD");
		}

		// set flags based on result
		ctx.SetAllFlags(src + dst, src, dst, S_LONG, false, Context::ADD);
		ctx.StepPC(ext_words);
	}
};

static Instruction* instr1= GetInstructions().Register(new Add(0xd080, AM_ALL_SRC & ~AM_IMMEDIATE, AM_Dx));
static Instruction* instr2= GetInstructions().Register(new Add(0xd180, AM_Dx, AM_ALL_DST & ~(AM_Dx | AM_Ax)));


class AddI : public InstructionImpl<Stencil_REG>
{
public:
	AddI() : InstructionImpl("ADDI", IParam().AltName("ADD").Sizes(IS_LONG).SrcModes(AM_IMMEDIATE).DestModes(AM_Dx).Opcode(0x0680).Mask(Stencil::MASK))
	{}

	virtual DecodedInstruction Decode(InstrPointer& ctx) const
	{
		Stencil o= OpCode(ctx);

		DecodedInstruction out(Mnemonic(), S_LONG, ctx.OpCode());
		uint32 ext= ctx.GetNextLongWord();

		out.src_ = EffectiveAddress_Imm(ext, false);
		out.dest_ = EffectiveAddress_DReg(o.reg_index);

		return out;
	}

	virtual void Execute(Context& ctx) const
	{
		Stencil o= OpCode(ctx);

		uint32 src= ctx.GetNextPCLongWord();
		uint32& reg= ctx.Cpu().d_reg[o.reg_index];
		uint32 dst= reg;

		reg += src;

		ctx.SetAllFlags(reg, src, dst, S_LONG, false, Context::ADD);
	}
};


static Instruction* instr3= GetInstructions().Register(new AddI());


class AddX : public InstructionImpl<Stencil_REG_REG>
{
public:
	AddX() : InstructionImpl("ADDX", IParam().Sizes(IS_LONG).SrcModes(AM_Dx).DestModes(AM_Dx).Opcode(0xd180).Mask(0x0e07))
	{}

	virtual DecodedInstruction Decode(InstrPointer& ctx) const
	{
		Stencil o= OpCode(ctx);

		DecodedInstruction out(Mnemonic(), S_LONG, o.opcode);
		out.src_ = EffectiveAddress_DReg(o.reg1_index);
		out.dest_ = EffectiveAddress_DReg(o.reg2_index);

		return out;
	}

	virtual void Execute(Context& ctx) const
	{
		Stencil o= OpCode(ctx);

		// ADDX dy, dx
		uint32 reg_dy_index= o.reg2_index;
		uint32 reg_dx_index= o.reg1_index;

		uint32 src= ctx.Cpu().d_reg[reg_dy_index];
		uint32 dst= ctx.Cpu().d_reg[reg_dx_index];

		uint32 extend= ctx.Extend();
		uint32 result= dst + src + extend;

		ctx.Cpu().d_reg[reg_dx_index] = result;

		ctx.SetAllFlags(result, src, dst, S_LONG, true, Context::ADD);
	}

	//virtual void Encode(InstructionSize size, const EffectiveAddress& ea_src, const EffectiveAddress& ea_dst, OutputPointer& ctx) const
	//{
	//	if (VariablePartMask() == 0x0e07 && ea_src.mode_ == AM_Dx && ea_dst.mode_ == AM_Dx)
	//		Emit_Dx_Dy(StencilCode(), ea_src.first_reg_, 0, ea_dst.first_reg_, 9, ctx);
	//	else
	//		throw "illegal EA combination";
	//}
};


static Instruction* instr4= GetInstructions().Register(new AddX());
