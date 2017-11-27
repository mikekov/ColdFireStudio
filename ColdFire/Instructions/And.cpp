/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "Register.h"
#include "..\EmitCode.h"


class AndOr : public InstructionImpl<Stencil_REG_OP_EA>
{
public:
	AndOr(const char* name, uint16 opcode_stencil, AddressingMode src, AddressingMode dst)
		: InstructionImpl(name, IParam().Sizes(IS_LONG).SrcModes(src).DestModes(dst).Opcode(opcode_stencil).Mask(0x0e3f))
	{}

	virtual DecodedInstruction Decode(InstrPointer& ctx) const
	{
		int opmode= GetOperandMode(ctx.OpCode(), 6);

		DecodedInstruction out(Mnemonic(), S_LONG, ctx.OpCode());

		if (opmode == 2)	// <ea>y & Dx -> Dx
		{
			out.DecodeSrcEA(3, 0, ctx);
			out.dest_ = EffectiveAddress_DReg(GetRegisterCode(ctx.OpCode(), 9));
		}
		else if (opmode == 6)	// Dy & <ea>x -> <ea>x
		{
			out.src_ = EffectiveAddress_DReg(GetRegisterCode(ctx.OpCode(), 9));
			out.DecodeDestEA(3, 0, ctx);
		}
		else
		{
			throw LogicError("And/Or/Eor instr - bad code");
		}

		return out;
	}

	virtual void Execute(Context& ctx) const
	{
		auto o= OpCode(ctx);

		int oper= o.opcode & 0xf000;

		InstrSize size= S_LONG;
		int ext_words= 0;
		uint32 result= 0;

		switch (o.op_mode)
		{
		case 2:		// long word, <ea>y op Dx -> Dx
			{
				uint32 src= ctx.DecodeSrcLongWordValue(o.ea_mode, size, ext_words);
				uint32& dst= ctx.Cpu().d_reg[o.reg_index];

				switch (oper)
				{
				case 0xc000:	// and
					result = src & dst;
					break;
				case 0x8000:	// or
					result = src | dst;
					break;
				case 0xb000:	// eor
					throw LogicError("unexpected operand in AndOr; EOR from <ea> to Dx not supported");
				default:
					throw LogicError("unexpected operand in AndOr");
				}

				dst = result;
			}
			break;

		case 6:		// long word, Dy op <ea>x -> <ea>x
			{
				uint32 src= ctx.GetDataRegister(o.reg_index);
				DecodedAddress da= ctx.DecodeMemoryAddress(o.ea_mode, size, ext_words);
				uint32 dst= ctx.ReadFromAddress(da, size);

				switch (oper)
				{
				case 0xc000:	// and
					result = src & dst;
					break;
				case 0x8000:	// or
					result = src | dst;
					break;
				case 0xb000:	// eor
					result = src ^ dst;
					break;
				default:
					throw LogicError("unexpected operand in AndOr");
				}

				ctx.WriteToAddress(da, result, size);
			}
			break;
		}

		// set flags based on result
		ctx.SetNZ_ClrCV(result);

		ctx.StepPC(ext_words);
	}
};

static Instruction* instr1= GetInstructions().Register(new AndOr("AND", 0xc080, AM_ALL_SRC & ~(AM_Ax | AM_IMMEDIATE), AM_Dx));
static Instruction* instr2= GetInstructions().Register(new AndOr("AND", 0xc180, AM_Dx, AM_ALL_DST & ~(AM_Dx | AM_Ax)));

static Instruction* instr3= GetInstructions().Register(new AndOr("OR", 0x8080, AM_ALL_SRC & ~(AM_Ax | AM_IMMEDIATE), AM_Dx));
static Instruction* instr4= GetInstructions().Register(new AndOr("OR", 0x8180, AM_Dx, AM_ALL_DST & ~(AM_Dx | AM_Ax)));

static Instruction* instr5= GetInstructions().Register(new AndOr("EOR", 0xB180, AM_Dx, AM_ALL_DST & ~AM_Ax));
