/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "Register.h"
#include "../EmitCode.h"


class SetCC : public InstructionImpl<Stencil_COND_REG>
{
public:
	SetCC(const char* name, uint16 condition_code, const char* alt_name= nullptr)
		: InstructionImpl(name, IParam().Sizes(IS_BYTE).AltName(alt_name).SrcModes(AM_NONE).DestModes(AM_Dx).Opcode(0x50c0 | (condition_code << 8)).Mask(0x0007))
	{}

	virtual DecodedInstruction Decode(InstrPointer& ctx) const
	{
		const char* name= 0;
		switch (ctx.OpCode() & 0x0f00)
		{
		case 0x0000:	name = "ST";	break;
		case 0x0100:	name = "SF";	break;
		case 0x0200:	name = "SHI";	break;
		case 0x0300:	name = "SLS";	break;
		case 0x0400:	name = "SCC";	break;
		case 0x0500:	name = "SCS";	break;
		case 0x0600:	name = "SNE";	break;
		case 0x0700:	name = "SEQ";	break;
		case 0x0800:	name = "SVC";	break;
		case 0x0900:	name = "SVS";	break;
		case 0x0a00:	name = "SPL";	break;
		case 0x0b00:	name = "SMI";	break;
		case 0x0c00:	name = "SGE";	break;
		case 0x0d00:	name = "SLT";	break;
		case 0x0e00:	name = "SGT";	break;
		case 0x0f00:	name = "SLE";	break;
		}

		DecodedInstruction out(name, S_BYTE, ctx.OpCode());

		out.dest_ = EffectiveAddress_DReg(GetRegisterCode(ctx.OpCode(), 0));

		return out;
	}

	virtual void Execute(Context& ctx) const
	{
		auto o= OpCode(ctx);

		uint8 val= 0;

		switch (o.condition)
		{
		case 0x0:	val = 0xff;	break;	// ST
		case 0x1:	val = 0x00;	break;	// SF
		case 0x2:	val = ctx.IsHI() ? 0xff : 0x00;	break;
		case 0x3:	val = ctx.IsLS() ? 0xff : 0x00;	break;
		case 0x4:	val = ctx.IsCC() ? 0xff : 0x00;	break;
		case 0x5:	val = ctx.IsCS() ? 0xff : 0x00;	break;
		case 0x6:	val = ctx.IsNE() ? 0xff : 0x00;	break;
		case 0x7:	val = ctx.IsEQ() ? 0xff : 0x00;	break;
		case 0x8:	val = ctx.IsVC() ? 0xff : 0x00;	break;
		case 0x9:	val = ctx.IsVS() ? 0xff : 0x00;	break;
		case 0xa:	val = ctx.IsPL() ? 0xff : 0x00;	break;
		case 0xb:	val = ctx.IsMI() ? 0xff : 0x00;	break;
		case 0xc:	val = ctx.IsGE() ? 0xff : 0x00;	break;
		case 0xd:	val = ctx.IsLT() ? 0xff : 0x00;	break;
		case 0xe:	val = ctx.IsGT() ? 0xff : 0x00;	break;
		case 0xf:	val = ctx.IsLE() ? 0xff : 0x00;	break;
		}

		auto& reg= ctx.Cpu().d_reg[o.reg_index];
		reg &= ~0xff;
		reg |= val;
	}

	virtual void Encode(InstructionSize size, const EffectiveAddress& ea_src, const EffectiveAddress& ea_dst, OutputPointer& ctx) const
	{
		uint16 opcode= StencilCode() | DataRegToNumber(ea_dst.first_reg_);
		ctx << opcode;
	}
};


static Instruction* instr0= GetInstructions().Register(new SetCC("ST", 0));
static Instruction* instr1= GetInstructions().Register(new SetCC("SF", 1));
static Instruction* instr2= GetInstructions().Register(new SetCC("SHI", 2));
static Instruction* instr3= GetInstructions().Register(new SetCC("SLS", 3));
static Instruction* instr4= GetInstructions().Register(new SetCC("SCC", 4, "SHS"));
static Instruction* instr5= GetInstructions().Register(new SetCC("SCS", 5, "SLO"));
static Instruction* instr6= GetInstructions().Register(new SetCC("SNE", 6));
static Instruction* instr7= GetInstructions().Register(new SetCC("SEQ", 7));
static Instruction* instr8= GetInstructions().Register(new SetCC("SVC", 8));
static Instruction* instr9= GetInstructions().Register(new SetCC("SVS", 9));
static Instruction* instra= GetInstructions().Register(new SetCC("SPL", 10));
static Instruction* instrb= GetInstructions().Register(new SetCC("SMI", 11));
static Instruction* instrc= GetInstructions().Register(new SetCC("SGE", 12));
static Instruction* instrd= GetInstructions().Register(new SetCC("SLT", 13));
static Instruction* instre= GetInstructions().Register(new SetCC("SGT", 14));
static Instruction* instrf= GetInstructions().Register(new SetCC("SLE", 15));
