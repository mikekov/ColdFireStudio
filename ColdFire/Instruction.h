/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#pragma once
#include "MachineDefs.h"
#include "BasicTypes.h"
#include "DecodedInstr.h"
#include "OutputPointer.h"
#include "Exceptions.h"

// Instruction - base class for all ColdFire instructions
//
// Provides basic information (opcode, addressing modes, sizes, etc.) as well
// as ability to (dis)assemble instruction and simulate its execution


struct IExcludeCodes
{
	IExcludeCodes()
	{
		Zero();
	}

	IExcludeCodes(uint16 mask, uint16 val1)
	{
		Zero();
		mask_ = mask;
		values_[0] = val1;
		count_ = 1;
	}

	IExcludeCodes(uint16 mask, uint16 val1, uint16 val2)
	{
		Zero();
		mask_ = mask;
		values_[0] = val1;
		values_[1] = val2;
		count_ = 2;
	}

	IExcludeCodes(uint16 mask, uint16 val1, uint16 val2, uint16 val3)
	{
		Zero();
		mask_ = mask;
		values_[0] = val1;
		values_[1] = val2;
		values_[2] = val3;
		count_ = 3;
	}

	IExcludeCodes(uint16 mask, uint16 val1, uint16 val2, uint16 val3, uint16 val4)
	{
		Zero();
		mask_ = mask;
		values_[0] = val1;
		values_[1] = val2;
		values_[2] = val3;
		values_[3] = val4;
		count_ = 4;
	}

	IExcludeCodes(uint16 mask, uint16 val1, uint16 val2, uint16 val3, uint16 val4, uint16 val5)
	{
		Zero();
		mask_ = mask;
		values_[0] = val1;
		values_[1] = val2;
		values_[2] = val3;
		values_[3] = val4;
		values_[4] = val5;
		count_ = 5;
	}

	void Zero()
	{
		mask_ = 0;
		std::fill(values_, values_ + 7, 0);
		count_ = 0;
	}

	uint16 mask_;
	uint16 values_[7];
	uint32 count_;
};


struct IDataRange	// immediate data range for an instruction
{
	IDataRange() : min(0), max(0), is_valid(false), exclude(false), exclude_val(0)
	{}
	IDataRange(int32 min, uint32 max) : min(min), max(max), is_valid(true), exclude(false), exclude_val(0)
	{}

	int32 min;
	uint32 max;
	bool is_valid;
	bool exclude;
	uint32 exclude_val;
};


enum class IControlFlow
{
	NONE= 0,	// most instructions
	BRANCH,		// JMP, BRA, Bcc
	SUBROUTINE,	// JSR, BSR, TRAP
	STOP,		// HALT, STOP
	RETURN		// RTS, RTE
};


class IParam	// instruction parameters
{
public:
	IParam() : alt_name_(nullptr), sizes_(IS_UNSIZED), default_size_(IS_NONE), src_modes_(AM_IMPLIED), dst_modes_(AM_NONE), stencil_code_(0), variable_part_(0), defined_in_isa_(ISA::A), exclude_from_isas_(false), immediate_data_size_(S_NA), privileged_instr_(false), control_flow_(IControlFlow::NONE), cycles_(1), second_src_modes_(AM_NONE), second_dst_modes_(AM_NONE)
	{}

	IParam& Opcode(uint16 stencil_code)			{ stencil_code_ = stencil_code; return *this; }
	IParam&	Mask(uint16 variable_part)			{ variable_part_ = variable_part; return *this; }
	IParam& ExcludeCodes(const IExcludeCodes& exclude)	{ exclude_ = exclude; return *this; }

	IParam& ExcludeCodes(uint16 mask, uint16 val1)	{ exclude_ = IExcludeCodes(mask, val1); return *this; }
	IParam& ExcludeCodes(uint16 mask, uint16 val1, uint16 val2)
	{ exclude_ = IExcludeCodes(mask, val1, val2); return *this; }
	IParam& ExcludeCodes(uint16 mask, uint16 val1, uint16 val2, uint16 val3)
	{ exclude_ = IExcludeCodes(mask, val1, val2, val3); return *this; }
	IParam& ExcludeCodes(uint16 mask, uint16 val1, uint16 val2, uint16 val3, uint16 val4)
	{ exclude_ = IExcludeCodes(mask, val1, val2, val3, val4); return *this; }
	IParam& ExcludeCodes(uint16 mask, uint16 val1, uint16 val2, uint16 val3, uint16 val4, uint16 val5)
	{ exclude_ = IExcludeCodes(mask, val1, val2, val3, val4, val5); return *this; }

	// alternative mnemonic
	IParam& AltName(const char* alt_name)		{ alt_name_ = alt_name; return *this; }

	IParam& Sizes(InstructionSize sizes)		{ sizes_ = sizes; InitDefaultSize(sizes); return *this; }
	IParam& DefaultSize(InstructionSize size)	{ default_size_ = size; return *this; }

	IParam& ImmDataRange(int32 min, uint32 max)	{ immediate_data_range_ = IDataRange(min, max); return *this; }
	IParam& ImmDataRange(int32 min, uint32 max, int32 excluded)
	{ immediate_data_range_ = IDataRange(min, max); immediate_data_range_.exclude_val = excluded; immediate_data_range_.exclude = true; return *this; }
	IParam& ImmDataSize(InstrSize size)			{ immediate_data_size_ = size; return *this; }

	IParam& AddrModes(AddressingMode modes)		{ src_modes_ = modes; return *this; }
	IParam& SrcModes(AddressingMode modes)		{ src_modes_ = modes; return *this; }
	IParam& DestModes(AddressingMode modes)		{ dst_modes_ = modes; return *this; }

	IParam& SndSrcModes(AddressingMode modes)	{ second_src_modes_ = modes; return *this; }
	IParam& SndDestModes(AddressingMode modes)	{ second_dst_modes_ = modes; return *this; }

	IParam& SpecReg(CpuRegister reg)			{ assert(reg >= R_PC); supported_spec_regs_.insert(reg); return *this; }

	IParam& Privileged()						{ privileged_instr_ = true; return *this; }

	IParam& Isa(ISA isa)						{ defined_in_isa_ = isa; return *this; }
	IParam& ExcludeFromOtherISAs(bool exclude)	{ exclude_from_isas_ = exclude; return *this; }

	IParam& ControlFlow(IControlFlow ctrl_flow)	{ control_flow_ = ctrl_flow; return *this; }

	IParam& Cycles(int cycles)					{ cycles_ = cycles; return *this; }

	uint16 stencil_code_;
	uint16 variable_part_;
	const char* alt_name_;
	InstructionSize sizes_;
	InstructionSize default_size_;
	AddressingMode src_modes_;
	AddressingMode dst_modes_;
	ISA defined_in_isa_;
	bool exclude_from_isas_;
	IExcludeCodes exclude_;
	InstrSize immediate_data_size_;
	IDataRange immediate_data_range_;
	std::set<CpuRegister> supported_spec_regs_;
	bool privileged_instr_;
	IControlFlow control_flow_;
	int cycles_;
	AddressingMode second_src_modes_;
	AddressingMode second_dst_modes_;

	void InitDefaultSize(InstructionSize sizes);
};


class Instruction
{
public:
	// construct instruction
	Instruction(const char* mnemonic, const IParam& params);

	// disassemble, decodes instruction from the input stream; use Simulator's DecodeInstruction for a complete disassembly
	virtual DecodedInstruction Decode(InstrPointer& ctx) const = 0;

	// assemble, generate opcode and extension word(s);
	virtual void Encode(InstructionSize size, const EffectiveAddress& ea_src, const EffectiveAddress& ea_snd_src, const EffectiveAddress& ea_dst, const EffectiveAddress& ea_snd_dst, OutputPointer& ctx) const;
	// default method covers many common cases based on src/dest addressing modes of an instruction,
	// it may need to be overloaded for some unusual ones
	virtual void Encode(InstructionSize size, const EffectiveAddress& ea_src, const EffectiveAddress& ea_dst, OutputPointer& ctx) const;

	// simulation of instruction execution in a CPU; changes state (registers, memory, etc.)
	virtual void Execute(Context& ctx) const = 0;

	// size in bytes of instruction including its arguments; used by assembler
	// overloaded for some corner case instructions
	// true - return valid length in len, false - illegal combination
	virtual bool CalcSize(InstructionSize size, const EffectiveAddress& ea_src, const EffectiveAddress& ea_dst, uint32& len) const;

	// opcode mask
	uint16 StencilCode() const;
	// part of the opcode that can very (a mask, 0 -> fixed opcode)
	uint16 VariablePartMask() const;
	// rarely used mask needed to avoid overlap of instructions while building a map of them;
	// if (during construction of instruction map) range of codes corresponding to a given instruction
	// has all bits in this mask (pair.first) equal to the pair.second value, such opcodes will be skipped;
	// typically some instructions that don't have data register addressing mode (three zero bits in EA)
	// may use this mask to avoid those combination of codes
	const IExcludeCodes& ExcludedOpcodeValues() const;

	// in which ISA was this instruction defined
	ISA DefinedInIsa() const;

	// when building per-ISA map of instructions, exclude this instruction from all ISAs but its own
	bool ExcludeFromOtherIsas() const;

	// primary mnemonic
	const char* Mnemonic() const;
	// secondary mnemonic (if any)
	const char* AlternativeMnemonic() const;

	// supported src/dest effective addressing modes
	AddressingMode SourceEAModes() const;
	AddressingMode DestinationEAModes() const;

	// supported secondary src/dest effective addressing modes (some (E)MAC opcodes with parallel transfer use them)
	AddressingMode SecondSourceEAModes() const;
	AddressingMode SecondDestinationEAModes() const;

	// supported sizes (like .W, .L, if any)
	InstructionSize SupportedSizes() const;
	// default size (if any)
	InstructionSize DefaultSize() const;

	// size limit for immediate data (if any)
	InstrSize ImmediateDataSize() const;

	// some instructions that support immediate addressing mode provide range of legal values
	IDataRange ImmediateDataRange() const;

	// list of special registers supported by this instruction as either source or destination (depending on supported addressing modes)
	const std::set<CpuRegister>& SupportedSpecialRegisters() const;

	// supervisor mode instruction?
	bool Privileged() const;

	// checking relative offset range
	virtual bool IsRelativeOffsetValid(uint32 offset, InstructionSize requested_size) const { return true; }

	// how does instruction impact program flow
	IControlFlow ControlFlow() const;

	// instruction timing
	uint32 Cycles() const;

protected:
	virtual ~Instruction();

	bool VerifyCode(uint16 code) const;

	friend class InstructionRepository;

private:
	const char* mnemonic_;				// primary mnemonic (ADD, CMP)
	const char* alt_mnemonic_;			// alternative mnemonic (ADD for ADDI, CMP for CMPA)
	uint16 stencil_code_;				// instruction code (or some bits of it)
	uint16 variable_part_;				// bit mask: part of the opcode that varies within instruction
	IExcludeCodes excluded_mask_value_;	// opcode and mask may be too loose, some values may need to be excluded
	ISA isa_;		// in which ISA was this instruction defined
	bool exclude_from_isas_;			// implementation detail: instruction only present in one map
	AddressingMode src_modes_;			// what addressing modes are supported for a source (or only) operand
	AddressingMode dst_modes_;			// destination addressing modes
	AddressingMode second_src_modes_;	// what addressing modes are supported for a second source operand
	AddressingMode second_dst_modes_;	// second destination addressing modes
	InstructionSize sizes_;				// supported operand sizes (long, word, byte, none, ...)
	InstructionSize default_size_;		// if not explicitly given, what's the default operand size
	IDataRange immediate_data_range_;	// if instruction supports immediate data mode, this is a limit on size of this data
										//  (if specified, otherwise instruction size dictates limits)
										//  note: this is used by oddballs only, moveq, mov3q, addq, trap, ...
	InstrSize immediate_data_size_;		// type used to encode immediate data (no more, no less)
	std::set<CpuRegister> supported_spec_regs_;	// list of special registers supported as a source or destination
	bool privileged_instr_;				// if true, this instruction requires supervisor mode to execute
	IControlFlow control_flow_;			// impact on program flow (bsr/jsr/trap)
	int cycles_;						// approx how many cycles this instruction takes to execute (excluding addressing mode overhead)
};
