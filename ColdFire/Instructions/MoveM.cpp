/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "Register.h"
#include "..\EmitCode.h"

extern bool calculate_instr_size(InstructionSize requested_size, const EffectiveAddress& ea, uint32& size);


class MoveM : public InstructionImpl<Stencil_EA>
{
public:
	MoveM(uint16 opcode_stencil, AddressingMode ea, bool ea_first)
		: InstructionImpl("MOVEM", IParam().Sizes(IS_LONG).SrcModes(ea_first ? ea : AM_REG_LIST).DestModes(ea_first ? AM_REG_LIST : ea).Opcode(opcode_stencil | (ea_first ? 0x0400 : 0x0)).Mask(0x0007))
	{}

	virtual DecodedInstruction Decode(InstrPointer& ctx) const
	{
		DecodedInstruction out(Mnemonic(), S_LONG, ctx.OpCode());
		// mask follows
		auto mask= ctx.GetNextWord();

		if (ctx.OpCode() & MEM_TO_REG)
		{
			// mem to reg
			out.DecodeSrcEA(3, 0, ctx);
			out.dest_ = DasmEffectiveAddress(AM_REG_LIST, mask);
		}
		else
		{
			// reg to mem
			out.src_ = DasmEffectiveAddress(AM_REG_LIST, mask);
			out.DecodeDestEA(3, 0, ctx);
		}

		return out;
	}

	enum { MEM_TO_REG= 0x0400 };

	virtual void Execute(Context& ctx) const
	{
		auto o= OpCode(ctx);
		auto mask= ctx.GetNextPCWord();

		int ext_words= 0;
		InstrSize size= S_LONG;
		uint32 access_size= 4;

		DecodedAddress da= ctx.DecodeMemoryAddress(o.ea_mode, size, ext_words);
		if (da.type == DecodedAddress::REGISTER)
			throw LogicError("unexpected address type in " __FUNCTION__);

		if (o.opcode & MEM_TO_REG)
		{
			for (int i= 0; i < 16; ++i, mask >>= 1)
				if (mask & 1)
				{
					ctx.SetRegister(i, ctx.ReadFromAddress(da, size));
					da.cf_addr += access_size;
					da = ctx.GetMemoryAddress(da.cf_addr, access_size);
				}
		}
		else
		{
			for (int i= 0; i < 16; ++i, mask >>= 1)
				if (mask & 1)
				{
					auto value= ctx.GetRegister(i);
					ctx.WriteToAddress(da, value, size);
					da.cf_addr += access_size;
					da = ctx.GetMemoryAddress(da.cf_addr, access_size);
				}
		}

		ctx.StepPC(ext_words);
	}

	virtual void Encode(InstructionSize size, const EffectiveAddress& ea_src, const EffectiveAddress& ea_dst, OutputPointer& ctx) const
	{
		ASSERT(ea_src.mode_ == AM_REG_LIST || ea_dst.mode_ == AM_REG_LIST);

		// register list:
		uint16 reg_list= (ea_src.mode_ == AM_REG_LIST ? ea_src.register_mask : ea_dst.register_mask);

		Emit_Ext_EA(StencilCode(), reg_list, ea_src.mode_ != AM_REG_LIST ? ea_src : ea_dst, ctx);
	}

	virtual bool CalcSize(InstructionSize size, const EffectiveAddress& ea_src, const EffectiveAddress& ea_dst, uint32& len) const
	{
		// MoveM uses one extension word to encode registers being moved

		uint32 src= 0;
		if (ea_src.mode_ == AM_REG_LIST)
			src = 2;
		else if (!calculate_instr_size(size, ea_src, src))
			return false;

		uint32 dst= 0;
		if (ea_dst.mode_ == AM_REG_LIST)
			dst = 2;
		else if (!calculate_instr_size(size, ea_dst, dst))
			return false;

		len = 2 + src + dst;

		return true;
	}
};


static Instruction* instr2= GetInstructions().Register(new MoveM(0x48e8, AM_DISP_Ax, false));
static Instruction* instr1= GetInstructions().Register(new MoveM(0x48d0, AM_INDIRECT_Ax, false));
static Instruction* instr4= GetInstructions().Register(new MoveM(0x48e8, AM_DISP_Ax, true));
static Instruction* instr3= GetInstructions().Register(new MoveM(0x48d0, AM_INDIRECT_Ax, true));
