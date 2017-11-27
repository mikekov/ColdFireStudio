/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#pragma once
#include "MachineDefs.h"
#include "BasicTypes.h"
#include "Context.h"
#include <assert.h>
#include <iosfwd>
#include "Import.h"
#include "Utilities.h"
#include "Exceptions.h"


enum SpecialRegister	// special and control registers
{
	REG_INVALID= 0,
	REG_SR,
	REG_CCR,
	REG_USP,
	REG_CACR,
	REG_ASID,
	REG_ACR0,
	REG_ACR1,
	REG_ACR2,
	REG_ACR3,
	REG_MMUBAR,
	REG_OTHER_A7,
	REG_VBR,
	REG_PC,
	REG_ROMBAR0,
	REG_ROMBAR1,
	REG_RAMBAR0,
	REG_RAMBAR1,
	REG_MPCR,
	REG_EDRAMBAR,
	REG_SECMBAR,
	REG_MBAR,
	REG_PCR1U0,
	REG_PCR1L0,
	REG_PCR2U0,
	REG_PCR2L0,
	REG_PCR3U0,
	REG_PCR3L0,
	REG_PCR1U1,
	REG_PCR1L1,
	REG_PCR2U1,
	REG_PCR2L1,
	REG_PCR3U1,
	REG_PCR3L1,
	// MAC
	REG_MACSR,
	REG_MASK,
	REG_ACC0,
	REG_ACC1,
	REG_ACC2,
	REG_ACC3,
	REG_ACCext01,
	REG_ACCext02,
};


SpecialRegister DecodeControlRegisterField(uint16 code);

const char* GetSpecialRegisterName(SpecialRegister reg);

//enum ControlRegister
//{
//};


struct CF_DECL DasmEffectiveAddress
{
	AddressingMode mode_;
	uint8 register_;	// 0..7
	uint8 index_reg_;	// 0..15 (00.7 Dn, 8..15 An)
	uint8 signed_immediate_data_;
	uint8 scale_;
	uint32 arg_;
	RegisterWord reg_1_word_;
	RegisterWord reg_2_word_;

	DasmEffectiveAddress()
	{
		init();
	}

	DasmEffectiveAddress(AddressingMode mode, uint32 arg)
	{
		init();
		mode_ = mode;
		arg_ = arg;
	}

	DasmEffectiveAddress(AddressingMode mode, int reg, uint32 arg, bool signed_data)
	{
		init();
		mode_ = mode;
		arg_ = arg;
		register_ = reg;
		signed_immediate_data_ = signed_data;
#ifdef _DEBUG
		if (mode == AM_Dx || mode == AM_Ax)
			assert(reg >= 0 && reg < 8);
#endif
	}

	DasmEffectiveAddress(SpecialRegister reg)
	{
		init();
		mode_ = AM_SPEC_REG;
		register_ = reg;
	}

	bool IsValid() const		{ return mode_ != AM_NONE; }

private:
	void init()
	{
		mode_ = AM_NONE;
		register_ = -1;
		scale_ = 0;
		arg_ = 0;
		index_reg_ = 0;
		signed_immediate_data_ = 0;
		reg_1_word_	= reg_2_word_ = RegisterWord::NONE;
	}
};


void CF_DECL PrintEffectiveAddress(std::ostream& ost, const DasmEffectiveAddress& ea, InstrSize size, uint32 addr, bool fill_addr, bool colon_in_long_args);


inline DasmEffectiveAddress EffectiveAddress_DReg(int reg)
{
	return DasmEffectiveAddress(AM_Dx, reg, 0, false);
}

inline DasmEffectiveAddress EffectiveAddress_AReg(int reg)
{
	return DasmEffectiveAddress(AM_Ax, reg, 0, false);
}

inline DasmEffectiveAddress EffectiveAddress_Imm(uint32 arg, bool signed_data)
{
	return DasmEffectiveAddress(AM_IMMEDIATE, -1, arg, signed_data);
}

inline DasmEffectiveAddress EffectiveAddress_SpecReg(SpecialRegister reg)
{
	return DasmEffectiveAddress(AM_SPEC_REG, reg, 0, false);
}

DasmEffectiveAddress EffectiveAddress_Imm(InstrPointer& ctx, InstrSize size, bool signed_data);

inline DasmEffectiveAddress EffectiveAddress_RegY_RegX(int reg_y, int reg_x, RegisterWord reg_y_word, RegisterWord reg_x_word)
{
	DasmEffectiveAddress ea(AM_Rx_Ry, reg_y, reg_x, false);
	ea.reg_1_word_ = reg_y_word;
	ea.reg_2_word_ = reg_x_word;
	return ea;
}

inline int GetRegisterCode(uint16 opcode, int reg_bit_pos)
{
	return (opcode >> reg_bit_pos) & 0x07;
}

inline int GetOperandMode(uint16 opcode, int mode_bit_pos)
{
	return (opcode >> mode_bit_pos) & 0x07;	// three bits in operand mode
}

inline InstrSize GetOperandSize(uint16 opcode, int size_bit_pos)
{
	switch ((opcode >> size_bit_pos) & 0x03)
	{
	case 0:
		return S_BYTE;
	case 1:
		return S_WORD;
	case 2:
		return S_LONG;
	default:
		return S_NA;
	}
}


struct CF_DECL DecodedInstruction	// decoded instruction info
{
	const char* name_;				// mnemonic
	InstrSize size_;				// its size (B, W, L)
	DasmEffectiveAddress src_;		// source ea
	DasmEffectiveAddress dest_;		// destination ea
	int words_;						// how many words it consists of (1..3)
	uint16 opcode_;
	bool unknown_instr_;
	uint32 ext_;

	DecodedInstruction()
	{
		name_ = "-";
		size_ = S_NA;
		words_ = 0;
		opcode_ = 0;
		ext_ = 0;
		unknown_instr_ = false;
	}

	DecodedInstruction(const char* name, uint16 opcode)
		: name_(name), size_(S_NA), words_(0), opcode_(opcode), unknown_instr_(false), ext_(0)
	{}

	DecodedInstruction(const char* name, InstrSize size, uint16 opcode)
		: name_(name), size_(size), words_(0), opcode_(opcode), unknown_instr_(false), ext_(0)
	{}

	//static uint16 GetDReg(uint16 opcode)
	//{
	//	// typical register Dn encoding in an opcode
	//	return opcode & 0x7;
	//}

	static uint32 GetLongWord(const Context& ctx, uint32 opcode_addr)
	{
		return ctx.GetLongWord(opcode_addr + 1);
	}

	void DecodeSrcEA(int mode_bit_pos, int reg_bit_pos, InstrPointer& ctx)
	{
		src_ = DecodeEA(mode_bit_pos, reg_bit_pos, ctx, size_);
	}

	void DecodeSrcEA(int ea_mode_reg, InstrPointer& ctx)
	{
		src_ = DoDecodeEA((ea_mode_reg >> 3) & 0x7, ea_mode_reg & 0x7, ctx, size_);
	}

	void DecodeDestEA(int mode_bit_pos, int reg_bit_pos, InstrPointer& ctx)
	{
		dest_ = DecodeEA(mode_bit_pos, reg_bit_pos, ctx, size_);
	}

	void DecodeDestEA(int ea_mode_reg, InstrPointer& ctx)
	{
		dest_ = DoDecodeEA((ea_mode_reg >> 3) & 0x7, ea_mode_reg & 0x7, ctx, size_);
	}

	void DecodeSrcEAReg(uint16 reg)
	{
		if (reg < 7)
			src_ = EffectiveAddress_DReg(reg);
		else
			src_ = EffectiveAddress_AReg(reg - 8);
	}

	DasmEffectiveAddress DecodeEA(int mode_bit_pos, int reg_bit_pos, InstrPointer& ctx, InstrSize size);

	DasmEffectiveAddress DoDecodeEA(int mode, int reg, InstrPointer& ctx, InstrSize size);

	const char* InstrSizeName() const
	{
		switch (size_)
		{
		case S_NA:		return "";
		case S_BYTE:	return ".B";
		case S_WORD:	return ".W";
		case S_LONG:	return ".L";
		}
		throw LogicError("missing size name handler in " __FUNCTION__);
	}

	enum Flags { SHOW_NONE= 0, SHOW_ADDRESS= 1, SHOW_CODE_BYTES= 2, SHOW_CODE_CHARS= 4, LOWERCASE_MNEMONICS= 8, COLON_IN_ADDRESS= 0x10, COLON_IN_LONG_ARGS= 0x20, LOWERCASE_SIZE= 0x40 };
	std::string ToString(uint32 address, unsigned int show_flags, char tab) const;

	bool Valid() const;
	uint32 Length() const	{ return Valid() ? words_ * 2 : 2; }
};


DecodedInstruction CF_DECL DecodeInstruction(Context& ctx, uint32 addr);
