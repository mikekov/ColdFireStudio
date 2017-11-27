/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "Register.h"

// all branches: conditional, unconditional, and to the subroutine
//
class Branch : public InstructionImpl<Stencil_COND_DISP>
{
public:
	Branch(const char* name, uint16 condition_code, bool support_long, const char* alt_name= nullptr)
		: InstructionImpl(name, IParam().AltName(alt_name).Sizes(IS_BYTE | IS_SHORT | IS_WORD | (support_long ? IS_LONG : IS_NONE)).DefaultSize(IS_WORD).SrcModes(AM_RELATIVE).Opcode(0x6000 | (condition_code << 8)).Mask(0x00ff).Isa(support_long ? ISA::B : ISA::A).ExcludeFromOtherISAs(!support_long).ControlFlow(condition_code == 1 ? IControlFlow::SUBROUTINE : IControlFlow::BRANCH))
	{}

	virtual DecodedInstruction Decode(InstrPointer& ctx) const
	{
		Stencil o= OpCode(ctx);

		const char* name= 0;
		switch (o.condition)
		{
		case 0x0:	name = "BRA";	break;
		case 0x1:	name = "BSR";	break;
		case 0x2:	name = "BHI";	break;
		case 0x3:	name = "BLS";	break;
		case 0x4:	name = "BCC";	break;
		case 0x5:	name = "BCS";	break;
		case 0x6:	name = "BNE";	break;
		case 0x7:	name = "BEQ";	break;
		case 0x8:	name = "BVC";	break;
		case 0x9:	name = "BVS";	break;
		case 0xa:	name = "BPL";	break;
		case 0xb:	name = "BMI";	break;
		case 0xc:	name = "BGE";	break;
		case 0xd:	name = "BLT";	break;
		case 0xe:	name = "BGT";	break;
		case 0xf:	name = "BLE";	break;
		}

		uint32 displacement= o.displacement;
		InstrSize is= S_NA;

		if (displacement == 0)
		{
			displacement = ctx.GetNextSWord();
			is = S_WORD;
		}
		else if (displacement == 0xff)
		{
			displacement = ctx.GetNextLongWord();
			is = S_LONG;
		}
		else
		{
			displacement = SignExtendByte(uint8(displacement));
			is = S_BYTE;
		}

		DecodedInstruction out(name, is, ctx.OpCode());

		out.src_ = DasmEffectiveAddress(AM_RELATIVE, displacement);

		return out;
	}

	virtual void Execute(Context& ctx) const
	{
		auto o= OpCode(ctx);

		int branch= false;
		bool bsr= false;

		switch (o.condition)
		{
		case 0x0:	branch = true;	break;	// BRA
		case 0x1:	branch = bsr = true;	break;	// BSR
		case 0x2:	branch = ctx.IsHI();	break;
		case 0x3:	branch = ctx.IsLS();	break;
		case 0x4:	branch = ctx.IsCC();	break;
		case 0x5:	branch = ctx.IsCS();	break;
		case 0x6:	branch = ctx.IsNE();	break;
		case 0x7:	branch = ctx.IsEQ();	break;
		case 0x8:	branch = ctx.IsVC();	break;
		case 0x9:	branch = ctx.IsVS();	break;
		case 0xa:	branch = ctx.IsPL();	break;
		case 0xb:	branch = ctx.IsMI();	break;
		case 0xc:	branch = ctx.IsGE();	break;
		case 0xd:	branch = ctx.IsLT();	break;
		case 0xe:	branch = ctx.IsGT();	break;
		case 0xf:	branch = ctx.IsLE();	break;
		}

		uint32 displacement= o.displacement;
		uint32 pc= ctx.Cpu().pc;

		if (displacement == 0)
			displacement = SignExtendWord(ctx.GetNextPCWord());
		else if (displacement == 0xff)
			displacement = ctx.GetNextPCLongWord();
		else
			displacement = SignExtendByte(uint8(displacement));

		if (branch)
		{
			if (bsr)
			{
				// push return address
				ctx.PushLongWord(ctx.Cpu().pc);
			}

			ctx.Cpu().pc = pc + displacement;
		}
	}

	virtual int Condition(Context& ctx) = 0;

	virtual void Encode(InstructionSize size, const EffectiveAddress& ea_src, const EffectiveAddress& ea_dst, OutputPointer& ctx) const
	{
		uint16 temp;

		switch (size)
		{
		case IS_BYTE:
		case IS_SHORT:
			temp = StencilCode() | (ea_src.val_.Value() & 0xff);
			ctx << temp;
			break;

		case IS_NONE:	// by default (if not specified) offset is word
		case IS_WORD:
			ctx << StencilCode();
			ctx << uint16(ea_src.val_.Value() & 0xffff);
			break;

		case IS_LONG:
			temp = StencilCode() | 0xff;
			ctx << temp;
			ctx << uint16((ea_src.val_.Value() >> 16) & 0xffff);	// high word
			ctx << uint16(ea_src.val_.Value() & 0xffff);			// low word
			break;

		default:
			throw LogicError("missing size specification in " __FUNCTION__);
		}
	}

	virtual bool IsRelativeOffsetValid(uint32 offset, InstructionSize requested_size)
	{
		// short branch to the next address is impossible to encode, so reject it
		if (requested_size == IS_SHORT && offset == 0)
			return false;

		// other combinations have been alread validated
		return true;
	}
};


class Bra : public Branch
{
public:
	Bra(bool support_long) : Branch("BRA", 0, support_long) {}
	int Condition(Context& ctx)	{ return true; }
};

class Bsr : public Branch
{
public:
	Bsr(bool support_long) : Branch("BSR", 1, support_long) {}
	int Condition(Context& ctx)	{ return true; }
};

class Bhi : public Branch
{
public:
	Bhi(bool support_long) : Branch("BHI", 2, support_long) {}
	int Condition(Context& ctx)	{ return ctx.IsHI(); }
};

class Bls : public Branch
{
public:
	Bls(bool support_long) : Branch("BLS", 3, support_long) {}
	int Condition(Context& ctx)	{ return ctx.IsLS(); }
};

class Bcc : public Branch
{
public:
	Bcc(bool support_long) : Branch("BCC", 4, support_long, "BHS") {}
	int Condition(Context& ctx)	{ return !ctx.Carry(); }
};

class Bcs : public Branch
{
public:
	Bcs(bool support_long) : Branch("BCS", 5, support_long, "BLO") {}
	int Condition(Context& ctx)	{ return ctx.Carry(); }
};

class Bne : public Branch
{
public:
	Bne(bool support_long) : Branch("BNE", 6, support_long) {}
	int Condition(Context& ctx)	{ return !ctx.Zero(); }
};

class Beq : public Branch
{
public:
	Beq(bool support_long) : Branch("BEQ", 7, support_long) {}
	int Condition(Context& ctx)	{ return ctx.Zero(); }
};

class Bvc : public Branch
{
public:
	Bvc(bool support_long) : Branch("BVC", 8, support_long) {}
	int Condition(Context& ctx)	{ return !ctx.Overflow(); }
};

class Bvs : public Branch
{
public:
	Bvs(bool support_long) : Branch("BVS", 9, support_long) {}
	int Condition(Context& ctx)	{ return ctx.Overflow(); }
};

class Bpl : public Branch
{
public:
	Bpl(bool support_long) : Branch("BPL", 10, support_long) {}
	int Condition(Context& ctx)	{ return !ctx.Negative(); }
};

class Bmi : public Branch
{
public:
	Bmi(bool support_long) : Branch("BMI", 11, support_long) {}
	int Condition(Context& ctx)	{ return ctx.Negative(); }
};

class Bge : public Branch
{
public:
	Bge(bool support_long) : Branch("BGE", 12, support_long) {}
	int Condition(Context& ctx)	{ return ctx.IsGE(); }
};

class Blt : public Branch
{
public:
	Blt(bool support_long) : Branch("BLT", 13, support_long) {}
	int Condition(Context& ctx)	{ return ctx.IsLT(); }
};

class Bgt : public Branch
{
public:
	Bgt(bool support_long) : Branch("BGT", 14, support_long) {}
	int Condition(Context& ctx)	{ return ctx.IsGT(); }
};

class Ble : public Branch
{
public:
	Ble(bool support_long) : Branch("BLE", 15, support_long) {}
	int Condition(Context& ctx)	{ return ctx.IsLE(); }
};

static Instruction* instr00= GetInstructions().Register(new Bra(true ));
static Instruction* instr01= GetInstructions().Register(new Bra(false));
static Instruction* instr10= GetInstructions().Register(new Bsr(true ));
static Instruction* instr11= GetInstructions().Register(new Bsr(false));
static Instruction* instr20= GetInstructions().Register(new Bhi(true ));
static Instruction* instr21= GetInstructions().Register(new Bhi(false));
static Instruction* instr30= GetInstructions().Register(new Bls(true ));
static Instruction* instr31= GetInstructions().Register(new Bls(false));
static Instruction* instr40= GetInstructions().Register(new Bcc(true ));
static Instruction* instr41= GetInstructions().Register(new Bcc(false));
static Instruction* instr50= GetInstructions().Register(new Bcs(true ));
static Instruction* instr51= GetInstructions().Register(new Bcs(false));
static Instruction* instr60= GetInstructions().Register(new Bne(true ));
static Instruction* instr61= GetInstructions().Register(new Bne(false));
static Instruction* instr70= GetInstructions().Register(new Beq(true ));
static Instruction* instr71= GetInstructions().Register(new Beq(false));
static Instruction* instr80= GetInstructions().Register(new Bvc(true ));
static Instruction* instr81= GetInstructions().Register(new Bvc(false));
static Instruction* instr90= GetInstructions().Register(new Bvs(true ));
static Instruction* instr91= GetInstructions().Register(new Bvs(false));
static Instruction* instra0= GetInstructions().Register(new Bpl(true ));
static Instruction* instra1= GetInstructions().Register(new Bpl(false));
static Instruction* instrb0= GetInstructions().Register(new Bmi(true ));
static Instruction* instrb1= GetInstructions().Register(new Bmi(false));
static Instruction* instrc0= GetInstructions().Register(new Bge(true ));
static Instruction* instrc1= GetInstructions().Register(new Bge(false));
static Instruction* instrd0= GetInstructions().Register(new Blt(true ));
static Instruction* instrd1= GetInstructions().Register(new Blt(false));
static Instruction* instre0= GetInstructions().Register(new Bgt(true ));
static Instruction* instre1= GetInstructions().Register(new Bgt(false));
static Instruction* instrf0= GetInstructions().Register(new Ble(true ));
static Instruction* instrf1= GetInstructions().Register(new Ble(false));
