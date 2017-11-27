/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "Register.h"
#include "..\EmitCode.h"


class Tst : public InstructionImpl<Stencil_S_EA>
{
public:
	Tst(uint16 opcode_stencil, InstructionSize size, AddressingMode modes)
		: InstructionImpl("TST", IParam().Sizes(size).SrcModes(modes).Opcode(opcode_stencil).Mask(0x003f).ExcludeCodes(size == IS_BYTE ? 0x38 : 0, 8))
	{}

	virtual DecodedInstruction Decode(InstrPointer& ctx) const
	{
		InstrSize is= GetOperandSize(ctx.OpCode(), 6);

		if (is == S_NA)	//TODO: verify, manual claims this is word too
			throw LogicError("Tst instr - bad code");

		DecodedInstruction out(Mnemonic(), is, ctx.OpCode());
		//out.DecodeDestEA(3, 0, ctx);
		out.DecodeSrcEA(3, 0, ctx);

		return out;
	}

	virtual void Execute(Context& ctx) const
	{
		auto o= OpCode(ctx);

		int ext_words= 0;

		switch (o.size)
		{
		case 0x0:	// byte
			{
				//todo: reject address registers
				uint8 result= ctx.DecodeSrcByteValue(o.ea_mode, S_BYTE, ext_words);
				ctx.SetNZ_ClrCV(result);
				break;
			}
		case 0x1:	// word
		case 0x3:	// word?
			{
				uint16 result= ctx.DecodeSrcWordValue(o.ea_mode, S_WORD, ext_words);
				ctx.SetNZ_ClrCV(result);
				break;
			}
		case 0x2:	// byte
			{
				uint32 result= ctx.DecodeSrcLongWordValue(o.ea_mode, S_LONG, ext_words);
				ctx.SetNZ_ClrCV(result);
				break;
			}
		}

		ctx.StepPC(ext_words);
	}
};


static Instruction* instr1= GetInstructions().Register(new Tst(0x4a00, IS_BYTE, AM_ALL_SRC & ~AM_Ax));
static Instruction* instr2= GetInstructions().Register(new Tst(0x4a40, IS_WORD, AM_ALL_SRC));
static Instruction* instr3= GetInstructions().Register(new Tst(0x4a80, IS_LONG, AM_ALL_SRC));
// manual claims this is word too, but it clashes with HALT opcode:
//static Instruction* instr4= GetInstructions().Register(new Tst(0x4ac0, IS_WORD, AM_ALL_SRC));




class Tas : public InstructionImpl<Stencil_EA>
{
public:
	Tas(uint16 opcode, AddressingMode addr_modes, bool exclude)
		: InstructionImpl("TAS", IParam().Sizes(IS_BYTE).SrcModes(AM_NONE).DestModes(addr_modes).Opcode(opcode).Mask(!exclude ? 0 : Stencil::MASK).Isa(ISA::B).ExcludeCodes(exclude ? 0x38 : 0, 0x00, 0x08, 0x38))
	{}

	virtual DecodedInstruction Decode(InstrPointer& ctx) const
	{
		DecodedInstruction out(Mnemonic(), S_BYTE, ctx.OpCode());
		out.DecodeDestEA(3, 0, ctx);

		return out;
	}

	virtual void Execute(Context& ctx) const
	{
		assert(false);	// TODO
	}

	virtual bool CalcSize(InstructionSize size, const EffectiveAddress& ea_src, const EffectiveAddress& ea_dst, uint32& len) const
	{
		if (VariablePartMask() == 0)
		{
			switch (ea_dst.mode_)
			{
			case AM_ABS_W:
				len = 2 + 2;
				break;
			case AM_ABS_L:
				len = 2 + 4;
				break;
			default:
				throw LogicError("unexpected addressing mode in " __FUNCTION__);
			}
			return true;
		}
		else
			return Instruction::CalcSize(size, ea_src, ea_dst, len);
	}

	void Encode(InstructionSize size, const EffectiveAddress& ea_src, const EffectiveAddress& ea_dst, OutputPointer& ctx) const
	{
		if (VariablePartMask() == 0)
		{
			switch (ea_dst.mode_)
			{
			case AM_ABS_W:
			case AM_ABS_L:
				Emit_EA(StencilCode(), ea_dst, ctx);
				break;
			default:
				throw LogicError("unexpected addressing mode in " __FUNCTION__);
			}
		}
		else
			Instruction::Encode(size, ea_src, ea_dst, ctx);
	}
};


static Instruction* instr40= GetInstructions().Register(new Tas(0x4ac0, AM_ALL_DST & ~(AM_Dx | AM_ABS_W | AM_ABS_L), true));
static Instruction* instr41= GetInstructions().Register(new Tas(0x4af8, AM_ABS_W, false));	// manually add (xxxx).w mode
static Instruction* instr42= GetInstructions().Register(new Tas(0x4af9, AM_ABS_L, false));	// manually add (xxxxxxxx).l mode
