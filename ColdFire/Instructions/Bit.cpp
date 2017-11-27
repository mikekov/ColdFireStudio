/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "Register.h"
#include "..\EmitCode.h"


inline InstrSize GetBitOperationSize(InstrPointer& ctx)
{
	return GetOperandMode(ctx.OpCode(), 3) == 0 ? S_LONG : S_BYTE;
}

enum BitOper
{
	TST= 0,
	CHG,
	CLR,
	SET,
	REGISTER_MASK= 0x4
};

void ExecuteBitOperation(Context& ctx, Stencil_REG_OP_EA opcode)
{
	uint32 bit_no= 0;
	int bit_num_in_Dn= ctx.OpCode() & 0x100;
	if (bit_num_in_Dn)
	{
		int Dn= opcode.reg_index;
		bit_no = ctx.Cpu().d_reg[Dn];
	}

	int operation= opcode.op_mode & ~REGISTER_MASK;
	int dest_mode= opcode.ea_mode;	// mode & register

	if ((dest_mode >> 3) == 0)	// data register as destination?
	{
		// destination Dn, operation size is long, bit number modulo 32

		int words= 0;
		uint32& reg= ctx.Cpu().d_reg[dest_mode & 7];

		if (!bit_num_in_Dn)
			bit_no = ctx.GetNextPCWord();
		bit_no &= 0x1f;
		uint32 bit_mask= 1 << bit_no;

		ctx.StepPC(words);

		// first test a bit
		ctx.SetZero((reg & bit_mask) == 0);

		// then set or clear or do nothing; decode operation from the opcode
		switch (operation)
		{
		case TST:
			// nop
			break;
		case CLR:
			reg &= ~bit_mask;
			break;
		case SET:
			reg |= bit_mask;
			break;
		case CHG:
			reg ^= bit_mask;
			break;
		default:
			assert(false);
			break;
		}
	}
	else
	{
		// when destination is anything but Dn, operation size is one byte, and bit number modulo 8

		// bit number extension word may follow, so it's read now;
		// this is going to mess up relative addressing mode, which bit opcodes in CF do not support currently as of ISA_C...
		if (!bit_num_in_Dn)
			bit_no = ctx.GetNextPCWord();	// it has to be read, so other addr modes work by finding right extension words
		bit_no &= 0x7;
		uint32 bit_mask= 1 << bit_no;

		// destination/source operand address
		int words= 0;
		InstrSize size= S_BYTE;
		DecodedAddress da= ctx.DecodeMemoryAddress(dest_mode, size, words);
		uint8 byte= static_cast<uint8>(ctx.ReadFromAddress(da, S_BYTE));

		//if (!bit_num_in_Dn)
		//	++words;

		ctx.StepPC(words);

		// first test a bit
		ctx.SetZero((byte & bit_mask) == 0);

		// then set or clear or do nothing; decode operation from the opcode
		switch (operation)
		{
		case TST:
			// nop
			break;
		case CLR:
			byte &= ~bit_mask;
			ctx.WriteToAddress(da, byte, S_BYTE);
			break;
		case SET:
			byte |= bit_mask;
			ctx.WriteToAddress(da, byte, S_BYTE);
			break;
		case CHG:
			byte ^= bit_mask;
			ctx.WriteToAddress(da, byte, S_BYTE);
			break;
		default:
			assert(false);
			break;
		}
	}
}


// instructions operating on a single bit (set/text/clear/change) appear in four flavors:
// there are two versions that differ by the way bit number is specfied: immadiate or in a data register;
// within those two each has two subsets: one instruction with destination being data reg. (long version),
// and the one with remaining destination addressing modes (byte size version)


class BitImmediate : public InstructionImpl<Stencil_REG_OP_EA>	// immediate bit number versions
{
public:
	// is_dx - destination addr. mode is data register
	BitImmediate(const char* name, uint16 code, bool is_dx)
		: InstructionImpl(name, IParam().Sizes(is_dx ? IS_LONG : IS_BYTE).SrcModes(AM_IMMEDIATE).DestModes(is_dx ? AM_Dx : AM_INDIRECT_Ax | AM_Ax_INC | AM_DEC_Ax | AM_DISP_Ax).Opcode(code).Mask(is_dx ? 0x07 : 0x3f).ExcludeCodes(is_dx ? IExcludeCodes() : IExcludeCodes(0x38, 0)).ImmDataSize(S_BYTE).ImmDataRange(0, is_dx ? 31 : 7))
	{}

	virtual DecodedInstruction Decode(InstrPointer& ctx) const
	{
		InstrSize is= GetBitOperationSize(ctx);

		DecodedInstruction out(Mnemonic(), is, ctx.OpCode());

		uint16 ext= ctx.GetNextWord() & 0xff;
		out.src_ = EffectiveAddress_Imm(ext, true);	// signed format is shorter
		out.DecodeDestEA(3, 0, ctx);

		return out;
	}

	virtual void Encode(InstructionSize size, const EffectiveAddress& ea_src, const EffectiveAddress& ea_dst, OutputPointer& ctx) const
	{
		if (ea_src.mode_ == AM_IMMEDIATE)
		{
			auto ext= static_cast<int16>(ea_src.val_.Value() & 0xff);	// bit number
/*
			uint16 words[3];
			int length= Encode_EA(StencilCode(), ea_dst, 3, 0, words);
			ctx << words[0];	// opcode
			ctx << uint16(ea_src.val_.Value() & 0xff);	// bit number
			if (length > 2)
				throw LogicError("unexpected addressing mode in " __FUNCTION__);
			if (length == 2)
				ctx << words[1];
*/
			Emit_Ext_EA(StencilCode(), ext, ea_dst, ctx);
		}
		else
			Instruction::Encode(size, ea_src, ea_dst, ctx);
	}

	virtual void Execute(Context& ctx) const
	{
		ExecuteBitOperation(ctx, OpCode(ctx));
	}

	virtual bool CalcSize(InstructionSize size, const EffectiveAddress& ea_src, const EffectiveAddress& ea_dst, uint32& len) const
	{
		EffectiveAddress src;
		src.mode_ = AM_IMMEDIATE;
		return Instruction::CalcSize(IS_WORD, src, ea_dst, len);
	}
};


class BitDRegister : public InstructionImpl<Stencil_REG_OP_EA>	// bit number in data reg. versions
{
public:
	BitDRegister(const char* name, uint16 code, bool is_dx)
		: InstructionImpl(name, IParam().Sizes(is_dx ? IS_LONG : IS_BYTE).SrcModes(AM_Dx).DestModes(is_dx ? AM_Dx : AM_ALL_DST & ~(AM_Ax | AM_Dx)).Opcode(code).Mask(is_dx ? 0x0e07 : 0x0e3f).ExcludeCodes(is_dx ? IExcludeCodes() : IExcludeCodes(0x38, 0)))
	{}

	virtual DecodedInstruction Decode(InstrPointer& ctx) const
	{
		InstrSize is= GetBitOperationSize(ctx);

		DecodedInstruction out(Mnemonic(), is, ctx.OpCode());

		out.src_ = EffectiveAddress_DReg(GetRegisterCode(ctx.OpCode(), 9));
		out.DecodeDestEA(3, 0, ctx);

		return out;
	}

	virtual void Execute(Context& ctx) const
	{
		ExecuteBitOperation(ctx, OpCode(ctx));
	}
};


class BtstI : public BitImmediate
{
public:
	BtstI(bool is_dx) : BitImmediate("BTST", 0x0800, is_dx)
	{}
};


class BtstD : public BitDRegister
{
public:
	BtstD(bool is_dx) : BitDRegister("BTST", 0x0100, is_dx)
	{}
};



class BchgI : public BitImmediate
{
public:
	BchgI(bool is_dx) : BitImmediate("BCHG", 0x0840, is_dx)
	{}
};

class BchgD : public BitDRegister
{
public:
	BchgD(bool is_dx) : BitDRegister("BCHG", 0x0140, is_dx)
	{}
};



class BclrI : public BitImmediate
{
public:
	BclrI(bool is_dx) : BitImmediate("BCLR", 0x0880, is_dx)
	{}
};

class BclrD : public BitDRegister
{
public:
	BclrD(bool is_dx) : BitDRegister("BCLR", 0x0180, is_dx)
	{}
};



class BsetI : public BitImmediate
{
public:
	BsetI(bool is_dx) : BitImmediate("BSET", 0x08c0, is_dx)
	{}
};

class BsetD : public BitDRegister
{
public:
	BsetD(bool is_dx) : BitDRegister("BSET", 0x01c0, is_dx)
	{}
};


static Instruction* instr10= GetInstructions().Register(new BtstI(false));
static Instruction* instr11= GetInstructions().Register(new BtstI(true));
static Instruction* instr20= GetInstructions().Register(new BtstD(false));
static Instruction* instr21= GetInstructions().Register(new BtstD(true));

static Instruction* instr30= GetInstructions().Register(new BchgI(false));
static Instruction* instr31= GetInstructions().Register(new BchgI(true));
static Instruction* instr40= GetInstructions().Register(new BchgD(false));
static Instruction* instr41= GetInstructions().Register(new BchgD(true));

static Instruction* instr50= GetInstructions().Register(new BclrI(false));
static Instruction* instr51= GetInstructions().Register(new BclrI(true));
static Instruction* instr60= GetInstructions().Register(new BclrD(false));
static Instruction* instr61= GetInstructions().Register(new BclrD(true));

static Instruction* instr70= GetInstructions().Register(new BsetI(false));
static Instruction* instr71= GetInstructions().Register(new BsetI(true));
static Instruction* instr80= GetInstructions().Register(new BsetD(false));
static Instruction* instr81= GetInstructions().Register(new BsetD(true));
