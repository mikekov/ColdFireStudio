/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "Instruction.h"
#include "EmitCode.h"


// try to deduce default instruction size if it's not given explicitly
void IParam::InitDefaultSize(InstructionSize sizes)
{
	assert(default_size_ == IS_NONE);

	if (sizes & IS_LONG)
		default_size_ = IS_LONG;
	else if (sizes & IS_WORD)
		default_size_ = IS_WORD;
	else if (sizes & IS_BYTE)
		default_size_ = IS_BYTE;
	else
		default_size_ = IS_UNSIZED;
}


//IParam& IParam::ImmDataSize(InstrSize size)
//{
//	switch (size)
//	{
//	case IS_LONG:
//		immediate_data_range_ = IDataRange(-0x7fffffff - 1, 0xffffffff);
//		break;
//
//	case IS_WORD:
//		immediate_data_range_ = IDataRange(-0x7fff - 1, 0xffff);
//		break;
//
//	case IS_BYTE:
//		immediate_data_range_ = IDataRange(-0x7f - 1, 0xff);
//
//	default:
//		immediate_data_range_ = IDataRange(1, 0);
//	}
//
//	return *this;
//}


Instruction::Instruction(const char* mnemonic, const IParam& params) : mnemonic_(mnemonic)
{
	alt_mnemonic_ = params.alt_name_;
	stencil_code_ = params.stencil_code_;
	variable_part_ = params.variable_part_;
	isa_ = params.defined_in_isa_;
	exclude_from_isas_ = params.exclude_from_isas_;
	src_modes_ = params.src_modes_;
	dst_modes_ = params.dst_modes_;
	second_src_modes_ = params.second_src_modes_;
	second_dst_modes_ = params.second_dst_modes_;
	sizes_ = params.sizes_;
	default_size_ = params.default_size_;
	excluded_mask_value_ = params.exclude_;
	immediate_data_range_ = params.immediate_data_range_;
	immediate_data_size_ = params.immediate_data_size_;
	supported_spec_regs_ = params.supported_spec_regs_;
	privileged_instr_ = params.privileged_instr_;
	control_flow_ = params.control_flow_;
	cycles_ = params.cycles_;
}


// try to deduce default instruction size if it's not given explicitly
//void Instruction::Init(InstructionSize sizes)
//{
//	if (sizes & IS_LONG)
//		default_size_ = IS_LONG;
//	else if (sizes & IS_WORD)
//		default_size_ = IS_WORD;
//	else if (sizes & IS_BYTE)
//		default_size_ = IS_BYTE;
//	else
//		default_size_ = IS_UNSIZED;
//}


Instruction::~Instruction()
{}


uint16 Instruction::StencilCode() const
{
	return stencil_code_;
}

uint16 Instruction::VariablePartMask() const
{
	return variable_part_;
}

const IExcludeCodes& Instruction::ExcludedOpcodeValues() const
{
	return excluded_mask_value_;
}

bool Instruction::VerifyCode(uint16 code) const
{
//	return (code & ~variable_part_) == stencil_code_;
	return true;
}

ISA Instruction::DefinedInIsa() const
{
	return isa_;
}

bool Instruction::ExcludeFromOtherIsas() const
{
	return exclude_from_isas_;
}

const char* Instruction::Mnemonic() const
{
	return mnemonic_;
}

const char* Instruction::AlternativeMnemonic() const
{
	return alt_mnemonic_;
}

AddressingMode Instruction::SourceEAModes() const
{
	return src_modes_;
}

AddressingMode Instruction::DestinationEAModes() const
{
	return dst_modes_;
}

AddressingMode Instruction::SecondSourceEAModes() const
{
	return second_src_modes_;
}

AddressingMode Instruction::SecondDestinationEAModes() const
{
	return second_dst_modes_;
}

InstructionSize Instruction::SupportedSizes() const
{
	return sizes_;
}

InstructionSize Instruction::DefaultSize() const
{
	return default_size_;
}

// Encode with two source and two destination params; if either second source of second destination is used derived
// class needs to override this method; here we delegate work to Encode() with single source and destination
//
void Instruction::Encode(InstructionSize size, const EffectiveAddress& ea_src, const EffectiveAddress& ea_snd_src, const EffectiveAddress& ea_dst, const EffectiveAddress& ea_snd_dst, OutputPointer& ctx) const
{
	if (ea_snd_src.mode_ == AM_NONE && ea_snd_src.mode_ == AM_NONE)
		Encode(size, ea_src, ea_dst, ctx);
	else
		throw LogicError("missing Encode implementation" __FUNCTION__);	// derived class needs to override Encode()
}

void Instruction::Encode(InstructionSize size, const EffectiveAddress& ea_src, const EffectiveAddress& ea_dst, OutputPointer& ctx) const
{
	if (variable_part_ == 0)
		ctx << stencil_code_;
	else if (variable_part_ == 7 && src_modes_ == AM_Dx && dst_modes_ == AM_NONE)
		Emit_Dx(stencil_code_, ea_src.first_reg_, ctx);
	else if (variable_part_ == 7 && src_modes_ == AM_NONE && dst_modes_ == AM_Dx)
		Emit_Dx(stencil_code_, ea_dst.first_reg_, ctx);
	else if (variable_part_ == 0x3f && src_modes_ != AM_NONE && dst_modes_ == AM_NONE)
		Emit_EA(stencil_code_, ea_src, ctx);
	else if (variable_part_ == 0x3f && src_modes_ == AM_NONE && dst_modes_ != AM_NONE)
		Emit_EA(stencil_code_, ea_dst, ctx);
	else if (variable_part_ == 7 && ea_src.mode_ == AM_IMMEDIATE && ea_dst.mode_ == AM_Dx)
		Emit_Dx_Imm(stencil_code_, ea_dst.first_reg_, 0, ea_src.val_, size, ctx);
	else if (variable_part_ == 0x0e07 && src_modes_ == AM_Dx && dst_modes_ == AM_Dx)
		Emit_Dx_Dy(stencil_code_, ea_src.first_reg_, 0, ea_dst.first_reg_, 9, ctx);
	else if (variable_part_ == 0x0e3f && (src_modes_ == AM_Dx || dst_modes_ == AM_Dx))
		Emit_Opcode_DataReg_EA(this, ea_src, ea_dst, ctx);
	else if (variable_part_ == 0x0e3f && (src_modes_ == AM_Ax || dst_modes_ == AM_Ax))
		Emit_Opcode_AddrReg_EA(this, ea_src, ea_dst, ctx);
	else
		throw LogicError("missing Encode implementation" __FUNCTION__);	// derived class needs to override Encode()
}


IDataRange Instruction::ImmediateDataRange() const
{
	return immediate_data_range_;
}


InstrSize Instruction::ImmediateDataSize() const
{
	return immediate_data_size_;
}


extern bool calculate_instr_size(InstructionSize requested_size, const EffectiveAddress& ea, uint32& size)
{
	size = 0;

	switch (ea.mode_)
	{
	case AM_RELATIVE:	// relative addressing mode for branch instructions only
		switch (requested_size)
		{
		case IS_BYTE:
		case IS_SHORT:
			break;	// offset inside instruction
		case IS_WORD:
			size += 2;
			break;
		case IS_LONG:
			size += 4;
			break;
		case IS_NONE:	// if size of offset is not given assume word
			size += 2;	// this has to be in sync with branch instruction's Encode()
			break;
		default:
			return false;
		}
		break;

		// expected word offset/value (signed)
	case AM_DISP_Ax:
	case AM_ABS_W:
		size += 2;
		break;

	case AM_DISP_PC:
		size += 2;
		break;

		// expected byte offset (signed)
	case AM_DISP_Ax_Ix:
		size += 2;
		break;

	case AM_DISP_PC_Ix:
		size += 2;
		break;

		// expected long value
	case AM_ABS_L:
		size += 4;
		break;

		// expected number at most 'requested_size' in range
	case AM_IMMEDIATE:

		// some instructions have argument "built-in", like AddQ, MoveQ
		// they need to override default CalcSize

		switch (requested_size)
		{
		case IS_BYTE:
			size += 2;	// byte in a word
			break;

		case IS_WORD:
			size += 2;
			break;

		case IS_LONG:
			size += 4;
			break;

		default:
			return false;
		}
		break;
	}

	return true;
}


// default implementation; override if it doesn't work for a particular instruction
//
bool Instruction::CalcSize(InstructionSize size, const EffectiveAddress& ea_src, const EffectiveAddress& ea_dst, uint32& len) const
{
	uint32 src= 0;
	if (!calculate_instr_size(size, ea_src, src))
		return false;

	uint32 dst= 0;
	if (!calculate_instr_size(size, ea_dst, dst))
		return false;

	len = 2 + src + dst;

	return true;
}


const std::set<CpuRegister>& Instruction::SupportedSpecialRegisters() const
{
	return supported_spec_regs_;
}


bool Instruction::Privileged() const
{
	return privileged_instr_;
}


IControlFlow Instruction::ControlFlow() const
{
	return control_flow_;
}


uint32 Instruction::Cycles() const
{
	return static_cast<uint32>(cycles_);
}
