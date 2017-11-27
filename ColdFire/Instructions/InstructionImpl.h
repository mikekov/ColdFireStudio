#pragma once

template<class STENCIL>
class InstructionImpl : public Instruction
{
protected:
	typedef STENCIL Stencil;

	InstructionImpl(const char* mnemonic, const IParam& params) : Instruction(mnemonic, params)
	{}

	static Stencil OpCode(Context& ctx)			{ Stencil o; ctx.OpCode(o); return o; }
	static Stencil OpCode(InstrPointer& ctx)	{ Stencil o; ctx.OpCode(o); return o; }
};


// bit fields are convenient, but not portable; still, improvement in readability is nice

#if BIT_FIELDS_LSB_TO_MSB

union Stencil_REG_OP_EA
{
	struct
	{
		uint16 ea_mode : 6;
		uint16 op_mode : 3;
		uint16 reg_index : 3;
		uint16 filler : 4;
	};
	uint16 opcode;

	enum { MASK = 0x0fff };
};

union Stencil_REG_EA
{
	struct
	{
		uint16 ea_mode : 6;
		uint16 filler1 : 3;
		uint16 reg_index : 3;
		uint16 filler2 : 4;
	};
	uint16 opcode;

	enum { MASK = 0x0e3f };
};

union Stencil_OP_REG
{
	struct
	{
		uint16 reg_index : 3;
		uint16 filler1 : 3;
		uint16 op_mode : 3;
		uint16 filler2 : 7;
	};
	uint16 opcode;
};

union Stencil_REG
{
	struct
	{
		uint16 reg_index : 3;
		uint16 filler : 13;
	};
	uint16 opcode;

	enum { MASK = 0x0007 };
};

union Stencil_FLD_REG
{
	struct
	{
		uint16 reg_index : 3;
		uint16 filler1 : 3;
		uint16 code : 2;
		uint16 filler2 : 8;
	};
	uint16 opcode;

	enum { MASK = 0x00c7 };
};

union Stencil_DATA
{
	struct
	{
		uint16 data : 4;
		uint16 filler : 12;
	};
	uint16 opcode;

	enum { MASK = 0x000f };
};

union Stencil_EA
{
	struct
	{
		uint16 ea_mode : 6;
		uint16 filler : 10;
	};
	uint16 opcode;

	enum { MASK = 0x003f };
};

union Stencil_REG_S_EA
{
	struct
	{
		uint16 ea_mode : 6;
		uint16 size : 1;
		uint16 reg_bit : 1;
		uint16 filler2 : 1;
		uint16 reg_index : 3;
		uint16 filler1 : 4;
	};
	uint16 opcode;

	enum { MAC_MASK = 0x0e7f };
	enum { EMAC_MASK = 0x0eff };
};

union Stencil_COND_DISP
{
	struct
	{
		uint16 displacement : 8;
		uint16 condition : 4;
		uint16 filler : 4;
	};
	uint16 opcode;
};

union Stencil_COND_REG
{
	struct
	{
		uint16 reg_index : 3;
		uint16 filler1 : 5;
		uint16 condition : 4;
		uint16 filler2 : 4;
	};
	uint16 opcode;
};

union Stencil_REG_REG
{
	struct
	{
		uint16 reg2_index : 3;
		uint16 filler2 : 6;
		uint16 reg1_index : 3;
		uint16 filler1 : 4;
	};
	uint16 opcode;
};

union Stencil_SHIFTS	// shift instructions
{
	struct
	{
		uint16 reg_index : 3;
		uint16 logical : 1;
		uint16 filler3 : 1;
		uint16 imm_reg : 1;
		uint16 filler2 : 2;
		uint16 direction : 1;
		uint16 count_or_reg : 3;
		uint16 filler1 : 4;
	};
	uint16 opcode;

	enum { MASK = 0x0e27 };
};

union Stencil_S_EA_EA	// move
{
	struct
	{
		uint16 src_ea_mode : 6;
		uint16 dest_ea_mode : 3;
		uint16 dest_ea_reg : 3;
		uint16 size : 2;
		uint16 filler : 2;
	};
	uint16 opcode;
};

union Stencil_S_EA
{
	struct
	{
		uint16 ea_mode : 6;
		uint16 size : 2;
		uint16 filler : 8;
	};
	uint16 opcode;

	enum { MASK = 0x00ff };
};

union Stencil_REG_DATA
{
	struct
	{
		uint16 data : 8;
		uint16 filler1 : 1;
		uint16 reg_index : 3;
		uint16 filler2 : 4;
	};
	uint16 opcode;

	enum { MASK = 0x0eff };
};

// extension word for long MULU/MULS, DIVS/DIVU/REMS/REMU
union Stencil_ExtWord_REG_OP
{
	struct
	{
		uint16 reg_index2 : 3;
		uint16 filler2 : 8;
		uint16 is_signed : 1;
		uint16 reg_index : 3;
		uint16 filler1 : 1;
	};
	uint16 word;
};

// extension word for MOVEC
union Stencil_ExtWord_REG_CREG
{
	struct
	{
		uint16 ctrl_reg : 12;
		uint16 ad_reg_index : 4;
	};
	uint16 word;
};

// some (E)MAC instructions
union Stencil_MAC_REG_REG
{
	struct
	{
		uint16 reg_y : 4;
		uint16 filler2 : 2;
		uint16 reg_x_msb : 1;
		uint16 acc_lsb : 1;
		uint16 filler1 : 1;
		uint16 reg_x : 3;
		uint16 line_a : 4;
	};
	uint16 opcode;

	enum { MAC_MASK = 0x0e4f };		// MAC ISA - no ACC designation
	enum { EMAC_MASK = 0x0ecf };	// EMAC ISA - using ACC lsb
};


#else

union Stencil_REG_OP_EA
{
	struct
	{
		uint16 filler : 4;
		uint16 reg_index : 3;
		uint16 op_mode : 3;
		uint16 ea_mode : 6;
	};
	uint16 opcode;
};

union Stencil_REG
{
	struct
	{
		uint16 filler : 13;
		uint16 reg_index : 3;
	};
	uint16 opcode;
};

union Stencil_EA
{
	struct
	{
		uint16 filler : 10;
		uint16 ea_mode : 6;
	};
	uint16 opcode;
};

union Stencil_REG_S_EA
{
	struct
	{
		uint16 filler1 : 4;
		uint16 reg_index : 3;
		uint16 filler2 : 2;
		uint16 size : 1;
		uint16 ea_mode : 6;
	};
	uint16 opcode;
};

union Stencil_COND_DISP
{
	struct
	{
		uint16 filler : 4;
		uint16 condition : 4;
		uint16 displacement : 8;
	};
	uint16 opcode;
};

union Stencil_REG_REG
{
	struct
	{
		uint16 filler1 : 4;
		uint16 reg1_index : 3;
		uint16 filler2 : 6;
		uint16 reg2_index : 3;
	};
	uint16 opcode;
};

union Stencil_SHIFTS	// shift instructions
{
	struct
	{
		uint16 filler1 : 4;
		uint16 count_or_reg : 3;
		uint16 direction : 1;
		uint16 filler2 : 2;
		uint16 imm_reg : 1;
		uint16 filler3 : 1;
		uint16 logical : 1;
		uint16 reg_index : 3;
	};
	uint16 opcode;
};

union Stencil_S_EA_EA	// move
{
	struct
	{
		uint16 filler : 2;
		uint16 size : 2;
		uint16 dest_ea_reg : 3;
		uint16 dest_ea_mode : 3;
		uint16 src_ea_mode : 6;
	};
	uint16 opcode;
};

union Stencil_ExtWord_REG_OP
{
	struct
	{
		uint16 filler1 : 1;
		uint16 reg_index : 3;
		uint16 is_signed : 1;
		uint16 filler2 : 8;
		uint16 reg_index2 : 3;
	};
	uint16 word;
};

#endif

union Stencil_UNIQUE
{
	uint16 opcode;
};
