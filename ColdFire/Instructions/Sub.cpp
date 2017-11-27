/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "Register.h"


class Sub : public InstructionImpl<Stencil_REG_OP_EA>
{
public:
	Sub(uint16 opcode_stencil, AddressingMode src, AddressingMode dst)
		: InstructionImpl("SUB", IParam().Sizes(IS_LONG).SrcModes(src).DestModes(dst).Opcode(opcode_stencil).Mask(0x0e3f).ExcludeCodes((dst & AM_Dx) ? IExcludeCodes() : IExcludeCodes(0x38, 0, 0x80)))
	{}

	virtual DecodedInstruction Decode(InstrPointer& ctx) const
	{
		Stencil o= OpCode(ctx);

		int opmode= o.op_mode;

		DecodedInstruction out(Mnemonic(), S_LONG, ctx.OpCode());

		if (opmode == 2)	// <ea>y - Dx -> Dx
		{
			out.DecodeSrcEA(o.ea_mode, ctx);
			out.dest_ = EffectiveAddress_DReg(o.reg_index);
		}
		else if (opmode == 6)	// Dy - <ea>x -> <ea>x
		{
			out.src_ = EffectiveAddress_DReg(o.reg_index);
			out.DecodeDestEA(o.ea_mode, ctx);
		}
		else
		{
			throw LogicError("Sub instr - bad code");
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
		case 2:		// long word, <ea>y - Dx -> Dx
			{
				src = ctx.DecodeSrcLongWordValue(o.ea_mode, size, ext_words);
				uint32& reg= ctx.Cpu().d_reg[o.reg_index];
				dst = reg;
				reg = dst - src;
			}
			break;

		case 6:		// long word, Dy - <ea>x -> <ea>x
			{
				src = ctx.GetDataRegister(o.reg_index);
				DecodedAddress da= ctx.DecodeMemoryAddress(o.ea_mode, size, ext_words);
				dst = ctx.ReadFromAddress(da, size);
				ctx.WriteToAddress(da, dst - src, size);
			}
			break;

		default:
			throw LogicError("unsupported operation code in SUB");
		}

		ctx.SetAllFlags(dst - src, src, dst, S_LONG, false, Context::SUB);
		ctx.StepPC(ext_words);
	}
};

																	// user manual lists immediate mode...
static Instruction* instr1= GetInstructions().Register(new Sub(0x9080, AM_ALL_SRC & ~AM_IMMEDIATE, AM_Dx));
static Instruction* instr2= GetInstructions().Register(new Sub(0x9180, AM_Dx, AM_ALL_DST & ~(AM_Dx | AM_Ax)));


class SubI : public InstructionImpl<Stencil_REG>
{
public:
	SubI() : InstructionImpl("SUBI", IParam().AltName("SUB").Sizes(IS_LONG).SrcModes(AM_IMMEDIATE).DestModes(AM_Dx).Opcode(0x0480).Mask(Stencil::MASK))
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
		uint32 dst= ctx.Cpu().d_reg[o.reg_index];

		ctx.Cpu().d_reg[o.reg_index] -= src;

		ctx.SetAllFlags(dst - src, src, dst, S_LONG, false, Context::SUB);
	}
};



static Instruction* instr3= GetInstructions().Register(new SubI());


class SubA : public InstructionImpl<Stencil_REG_EA>
{
public:
	SubA() : InstructionImpl("SUBA", IParam().AltName("SUB").Sizes(IS_LONG).SrcModes(AM_ALL_SRC).DestModes(AM_Ax).Opcode(0x91c0).Mask(Stencil::MASK))
	{}

	virtual DecodedInstruction Decode(InstrPointer& ctx) const
	{
		DecodedInstruction out(Mnemonic(), S_LONG, ctx.OpCode());

		out.DecodeSrcEA(3, 0, ctx);
		out.dest_ = EffectiveAddress_AReg(GetRegisterCode(ctx.OpCode(), 9));

		return out;
	}

	virtual void Execute(Context& ctx) const
	{
		Stencil o= OpCode(ctx);

		int ext_words= 0;
		uint32 src= ctx.DecodeSrcLongWordValue(o.ea_mode, S_LONG, ext_words);

		ctx.Cpu().a_reg[o.reg_index] -= src;

		ctx.StepPC(ext_words);
	}
};


static Instruction* instr4= GetInstructions().Register(new SubA());


class SubX : public InstructionImpl<Stencil_REG_REG>
{
public:
	SubX() : InstructionImpl("SUBX", IParam().Sizes(IS_LONG).SrcModes(AM_Dx).DestModes(AM_Dx).Opcode(0x9180).Mask(0x0e07))
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

		// SUBX dy, dx
		uint32 reg_dy_index= o.reg2_index;
		uint32 reg_dx_index= o.reg1_index;

		uint32 src= ctx.Cpu().d_reg[reg_dy_index];
		uint32 dst= ctx.Cpu().d_reg[reg_dx_index];

		uint32 extend= ctx.Extend();
		uint32 result= dst - src - extend;

		ctx.Cpu().d_reg[reg_dx_index] = result;

		ctx.SetAllFlags(result, src, dst, S_LONG, true, Context::SUB);
	}
};


static Instruction* instr5= GetInstructions().Register(new SubX());
