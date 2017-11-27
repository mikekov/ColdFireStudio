/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#pragma once
#include "FixedString.h"
#include "Isa.h"
#include "MachineDefs.h"


enum InstrSize
{
	S_NA= 0,			// not applicable
	S_BYTE,
	S_WORD,
	S_LONG
};


// enums representing addressing mode (effective addressing mode, either source or destination)
// (they have nothing to do with bits encoding effective address in a generated opcode)
enum AddressingMode : uint32
{
	AM_NONE=		0x00000000,	// not a valid mode, but lack of information or no valid AM available/supported

	AM_Dx=			0x00000001,	// Dn
	AM_Ax=			0x00000002,	// An
	AM_INDIRECT_Ax=	0x00000004,	// (An)
	AM_Ax_INC=		0x00000008,	// (An)+
	AM_DEC_Ax=		0x00000010,	// -(An)
	AM_DISP_Ax=		0x00000020,	// (d16, An)
	AM_DISP_Ax_Ix=	0x00000040,	// (d8, An, Xi*SF)
	AM_ABS_W=		0x00000080,	// (xxx).W
	AM_ABS_L=		0x00000100,	// (xxx).L
	AM_IMMEDIATE=	0x00000200,	// #<xxx>
	AM_DISP_PC=		0x00000400,	// (d16, PC)
	AM_DISP_PC_Ix=	0x00000800,	// (d8, PC, Xi*SF)

	AM_SPEC_REG=	0x00001000,	// SR, CCR, ctrl reg
	AM_RELATIVE=	0x00002000,	// branches (BRA, Bcc)
	AM_REG_LIST=	0x00004000,	// MOVEM
	AM_Dx_Dy=		0x00008000,	// Dx:Dy (REMU)
	AM_IMPLIED=		0x00010000,	// (RTS, NOP)
	AM_SPECIAL=		0x00020000,	// special (CPUSHL)
	AM_WORD_DATA=	0x00040000,	// .WORD
	// mac, emac
	AM_Rx_Ry=		0x00080000,	// two registers for MAC/MSAC instructions
	AM_MAC_MASK=	0x00100000,	// MAC uses mask '&' in parallel transfer
	AM_ACCx=		0x00200000,	// accumulator (MAC in EMAC ISA)
//	AM_IMPLIED_Dx=	0x0000000,	// implied data reg (BITREV, ANDI [dest])
//	AM_IMPLIED_Ax=	0x0000000,	// implied address reg (UNLK)

	AM_ALL_SRC=		0x00000fff,	// all source operand modes supported by several instructions
	AM_ALL_DST=		0x000001ff,	// ditto for destination operand

	AM_BOGUS=		0x80000000	// illegal addressing mode encountered during disassembly
};

inline AddressingMode operator | (AddressingMode e1, AddressingMode e2)
{
	return static_cast<AddressingMode>(static_cast<uint32>(e1) | static_cast<uint32>(e2));
}

inline AddressingMode operator |= (AddressingMode& e1, AddressingMode e2)
{
	return e1 = e1 | e2;
}

inline AddressingMode operator & (AddressingMode e1, AddressingMode e2)
{
	return static_cast<AddressingMode>(static_cast<uint32>(e1) & static_cast<uint32>(e2));
}

inline AddressingMode operator &= (AddressingMode& e1, AddressingMode e2)
{
	return e1 = e1 & e2;
}

inline AddressingMode operator ~ (AddressingMode e)
{
	return static_cast<AddressingMode>(~static_cast<uint32>(e));
}


enum InstructionSize
{
	IS_NONE=		0x00,	// not a valid size, lack of information

	IS_UNSIZED=		0x01,	// unsized, it is an error to specify size for unsized instruction
	IS_BYTE=		0x02,
	IS_WORD=		0x04,
	IS_LONG=		0x08,
	IS_SHORT=		0x10,	// Bcc.s (same as Bcc.b)
	IS_ALL=			0x0e,	// byte | word | long
	IS_FLOAT=		0x20,
	IS_DOUBLE=		0x40
};

inline InstructionSize operator | (InstructionSize e1, InstructionSize e2)
{
	return static_cast<InstructionSize>(static_cast<uint32>(e1) | static_cast<uint32>(e2));
}

inline InstructionSize operator |= (InstructionSize& e1, InstructionSize e2)
{
	return e1 = e1 | e2;
}

inline InstructionSize operator & (InstructionSize e1, InstructionSize e2)
{
	return static_cast<InstructionSize>(static_cast<uint32>(e1) & static_cast<uint32>(e2));
}


enum CpuRegister
{
	R_NONE= 0,	// not a valid register

	R_D0,
	R_D1,
	R_D2,
	R_D3,
	R_D4,
	R_D5,
	R_D6,
	R_D7,

	R_A0,
	R_A1,
	R_A2,
	R_A3,
	R_A4,
	R_A5,
	R_A6,
	R_A7,
	R_SP,

	R_PC,
	R_SR,
	R_CCR,
	R_USP,

	R_CACR,
	R_ASID,
	R_ACR0,
	R_ACR1,
	R_ACR2,
	R_ACR3,
	R_MMUBAR,

	R_VBR,
	R_MBAR,

	R_ROMBAR0,
	R_ROMBAR1,
	R_RAMBAR0,
	R_RAMBAR1,

	R_ACC0,
	R_ACC1,
	R_ACC2,
	R_ACC3,
};

// register word designation for MAC instructions
enum class RegisterWord : uint16
{
	NONE, UPPER, LOWER
};

// MAC scale factor
enum class ShiftFactor : uint16
{
	NONE,
	LEFT,	// * 2
	RIGHT	// / 2
};


struct Expr		// constant expression (or its value)
{
	Expr();
	Expr(int32 value);
	Expr(FixedString str);

	static Expr Undefined();

	bool IsNumber() const;
	bool IsRegMask() const;
	int32 Value() const;
	int32 RegMask() const;
	bool IsValid() const;
	bool IsDefined() const;

	int32 value;
	FixedString string;
	enum Type
	{
		EX_NONE= 0,		// 
		EX_UNDEF,		// undefined yet
		EX_BYTE,		// byte, in -255 to 255 range (sic!)
		EX_WORD,		// word, -65535 to 65535
		EX_LONG,		// long
		EX_STRING,		// string, not a number
		EX_REGISTER,	// single register
		EX_REG_LIST		// list of registers (or single register) for MOVEM
	} inf;

	Expr(Type type, int32 value);
};


inline bool operator == (const Expr& e1, const Expr& e2)
{
	return e1.inf == e2.inf && e1.value == e2.value && !!(e1.string == e2.string);
}

inline bool operator != (const Expr& e1, const Expr& e2)
{
	return !(e1 == e2);
}


enum IndexScale
{
	S_NONE= 0,	// none specified
	S_1,
	S_2,
	S_4,
	S_8			// FPU only
};


#if BIT_FIELDS_LSB_TO_MSB

union ExtensionWordFmt_DISP_REG_IDX
{
	struct
	{
		int16 displacement : 8;
		uint16 filler : 1;
		uint16 scale : 2;	// 0 - x1, 1 - x2, 2 - x4, 3 - x8 (FPU)
		uint16 WL : 1;		// word/long (68k only), if zero -> address zero exception
		uint16 reg : 3;		// register index
		uint16 DA : 1;		// data/address register
	};
	uint16 word;
};

#else

union ExtensionWordFmt_DISP_REG_IDX
{
	struct
	{
		uint16 DA : 1;		// data/address register
		uint16 reg : 3;		// register index
		uint16 WL : 1;		// word/long (68k only), if zero -> address zero exception
		uint16 scale : 2;	// 0 - x1, 1 - x2, 2 - x4, 3 - x8 (FPU)
		uint16 filler : 1;
		int16 displacement : 8;
	};
	uint16 word;
};

#endif


struct EffectiveAddress
{
	EffectiveAddress()
	{
		mode_ = AM_NONE;
		first_reg_ = second_reg_ = R_NONE;
		first_reg_word_ = second_reg_word_ = RegisterWord::NONE;
		shift_ = ShiftFactor::NONE;
		accumulator_ = R_NONE;
		register_mask = 0;
	}

	AddressingMode mode_;
	Expr val_;
	CpuRegister first_reg_;
	CpuRegister second_reg_;
	RegisterWord first_reg_word_;
	RegisterWord second_reg_word_;
	ShiftFactor shift_;
	CpuRegister accumulator_;
	Expr scale_;
	uint16 register_mask;	// list of registers (MOVEM)
};
