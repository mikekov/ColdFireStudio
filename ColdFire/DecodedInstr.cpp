/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "DecodedInstr.h"
#include "Instruction.h"
#include "OpcodeDefs.h"
#include "Import.h"
#include <sstream>


DasmEffectiveAddress DecodedInstruction::DecodeEA(int mode_bit_pos, int reg_bit_pos, InstrPointer& ctx, InstrSize size)
{
	uint16 code= ctx.OpCode();

	int mode= (code >> mode_bit_pos) & 0x7;
	int reg= (code >> reg_bit_pos) & 0x7;

	return DoDecodeEA(mode, reg, ctx, size);
}


DasmEffectiveAddress DecodedInstruction::DoDecodeEA(int mode, int reg, InstrPointer& ctx, InstrSize size)
{
	DasmEffectiveAddress ea;

	switch (mode)
	{
	case EA_Dx:
		ea.mode_ = AM_Dx;
		ea.register_ = reg;
		break;

	case EA_Ax:
		ea.mode_ = AM_Ax;
		ea.register_ = reg;
		break;

	case EA_Ax_IND:
		ea.mode_ = AM_INDIRECT_Ax;
		ea.register_ = reg;
		break;

	case EA_Ax_INC:
		ea.mode_ = AM_Ax_INC;
		ea.register_ = reg;
		break;

	case EA_DEC_Ax:
		ea.mode_ = AM_DEC_Ax;
		ea.register_ = reg;
		break;

	case EA_DISP_Ax:
		ea.mode_ = AM_DISP_Ax;
		ea.register_ = reg;
		ea.arg_ = ctx.GetNextSWord();
		break;

	case EA_DISP_Ax_Ix:
		ea.mode_ = AM_DISP_Ax_Ix;
		ea.register_ = reg;
		{
			uint16 ext= ctx.GetNextWord();
			ea.index_reg_ = uint8(ext >> 12);
			ea.scale_ = 1 << ((ext >> 9) & 0x3);
			ea.arg_ = int32(int16(int8(ext & 0xff)));
		}
		break;

	case EA_EXT:
		switch (reg)
		{
		case EA_EXT_ABS_W:
			ea.mode_ = AM_ABS_W;
			ea.arg_ = ctx.GetNextSWord();
			break;

		case EA_EXT_ABS_L:
			ea.mode_ = AM_ABS_L;
			ea.arg_ = ctx.GetNextLongWord();
			break;

		case EA_EXT_DISP_PC:
			ea.mode_ = AM_DISP_PC;
			//ea.arg_ = ctx.GetCurAddress();
			ea.arg_ = ctx.GetNextSWord();
			break;

		case EA_EXT_DISP_PC_Ix:
			ea.mode_ = AM_DISP_PC_Ix;
			{
				uint16 ext= ctx.GetNextWord();
				ea.index_reg_ = uint8(ext >> 12);
				ea.scale_ = 1 << ((ext >> 9) & 0x3);
				//ea.arg_ = ctx.GetCurAddress();
				ea.arg_ = int32(int16(int8(ext & 0xff)));
			}
			break;

		case EA_EXT_IMMEDIATE:
			ea.mode_ = AM_IMMEDIATE;
			if (size == S_BYTE)
				ea.arg_ = ctx.GetNextWord() & 0xff;
			else if (size == S_WORD)
				ea.arg_ = ctx.GetNextWord();
			else if (size == S_LONG)
				ea.arg_ = ctx.GetNextLongWord();
			else
				throw RunTimeError("missing instr size " __FUNCTION__);
			break;

		default:
			ea.mode_ = AM_BOGUS;
			//throw "unknown addressing mode";

		}
		break;
	}

	return ea;
}


DasmEffectiveAddress EffectiveAddress_Imm(InstrPointer& ctx, InstrSize size, bool signed_data)
{
	uint32 arg= 0;

	if (size == S_BYTE)
		arg = ctx.GetNextWord() & 0xff;
	else if (size == S_BYTE || size == S_WORD)
		arg = ctx.GetNextWord();
	else if (size == S_LONG)
		arg = ctx.GetNextLongWord();
	else
		throw RunTimeError("wrong instr size " __FUNCTION__);

	return EffectiveAddress_Imm(arg, signed_data);
}


static void PrintSignedWord(std::ostream& ost, uint32 arg)
{
	ost << std::hex;
	auto i= static_cast<int32>(arg);
	if (i < 0)
	{
		ost << '-';
		i = -i;
	}
	ost << '$';
	ost << i;
}


static void PrintSignedByte(std::ostream& ost, uint32 arg)
{
	ost << std::hex;
	auto i= static_cast<int32>(arg);
	if (i < 0)
	{
		ost << '-';
		i = -i;
	}
	ost << '$';
	ost << i;
}


static void PrintRegister(std::ostream& ost, int reg)
{
	if (reg > 7)
	{
		// 'A' reg
		ost << 'A' << char('0' + reg - 8);
	}
	else
	{
		// 'D' reg
		ost << 'D' << char('0' + reg);
	}
}


static void PrintScaledReg(std::ostream& ost, int reg, int scale)
{
	PrintRegister(ost, reg);

	if (scale != 1)
		ost << '*' << std::dec << scale;
}


static void PrintRegister(std::ostream& ost, int reg, RegisterWord word)
{
	PrintRegister(ost, reg);

	switch (word)
	{
	case RegisterWord::LOWER:
		ost << ".L";
		break;

	case RegisterWord::UPPER:
		ost << ".U";
		break;

	default:
		break;
	}
}


static void PrintLongWord(std::ostream& ost, uint32 value, bool zero_fill, bool use_colon)
{
	ost << std::hex;
	if (zero_fill)
	{
		ost.fill('0');
		if (use_colon)
		{
			ost.width(4);
			ost << ((value >> 16) & 0xffff);
			ost << ':';
			ost.width(4);
			ost << (value & 0xffff);
		}
		else
		{
			ost.width(8);
			ost << value;
		}
	}
	else
		ost << value;
}


static void PrintLongWordWithColon(std::ostream& ost, uint32 value)
{
	ost << std::hex;
	ost.fill('0');
	ost.width(4);
	ost << ((value >> 16) & 0xffff);
	ost << ':';
	ost.width(4);
	ost << (value & 0xffff);
}


void PrintOffsetAddress(std::ostream& ost, uint32 addr, uint32 arg, bool fill_addr, bool colon_in_long_arg)
{
	ost << "$";
	auto value= addr + static_cast<int32>(arg);
	PrintLongWord(ost, value, fill_addr, colon_in_long_arg);
}


static void PrintReg(std::ostream& ost, int reg)
{
	if (reg < 8)
		ost << 'D' << reg;
	else
		ost << 'A' << reg - 8;
}


void PrintRegList(std::ostream& ost, uint32 list)
{
	uint32 mask= 1;
	bool sep= false;

	list &= 0x0000ffff;

	for (int i= 0; i < 17; ++i, mask <<= 1)
	{
		if (list & mask)
		{
			if (sep)
				ost << '/';

			int first= i;

			do
			{
				++i;
				mask <<= 1;
			} while (list & mask);

			PrintReg(ost, first);
			int last= i - 1;

			if (last > first)
			{
				if (first < 8 && last >= 8)
				{
					if (first < 7)
					{
						ost << '-';
						PrintReg(ost, 7);
					}
					ost << '/';
					PrintReg(ost, 8);
					if (last != 8)
					{
						ost << '-';
						PrintReg(ost, last);
					}
				}
				else
				{
					ost << '-';
					PrintReg(ost, last);
				}
			}

			sep = true;
		}
	}
}


void PrintEffectiveAddress(std::ostream& ost, const DasmEffectiveAddress& ea, InstrSize size, uint32 addr, bool fill_addr, bool colon_in_long_arg)
{
	char dreg[4]= { 'D', '0' + ea.register_, 0 };
	char areg[4]= { 'A', '0' + ea.register_, 0 };

	switch (ea.mode_)
	{
	case AM_NONE:
		break;

	case AM_IMPLIED:
		break;

	case AM_Dx:
		ost << dreg;
		break;

	case AM_Ax:
		ost << areg;
		break;

	case AM_INDIRECT_Ax:
		ost << '(' << areg << ')';
		break;

	case AM_DISP_Ax:
		ost << '(';
		PrintSignedWord(ost, ea.arg_);
		ost << ", " << areg << ')';
		break;

	case AM_Ax_INC:
		ost << '(' << areg << ")+";
		break;

	case AM_DEC_Ax:
		ost << "-(" << areg << ')';
		break;

	case AM_DISP_Ax_Ix:
		ost << '(';
		PrintSignedByte(ost, ea.arg_);
		ost << ", " << areg << ", ";
		PrintScaledReg(ost, ea.index_reg_, ea.scale_);
		ost << ')';
		break;

	case AM_DISP_PC:
		ost << '(';
		PrintOffsetAddress(ost, addr + 2, ea.arg_, fill_addr, colon_in_long_arg);
		ost << ", PC)";
		break;

	case AM_DISP_PC_Ix:
		ost << '(';
		PrintOffsetAddress(ost, addr + 2, ea.arg_, fill_addr, colon_in_long_arg);
		//PrintSignedByte(ost, ea.arg_);
		ost << ", PC, ";
		PrintScaledReg(ost, ea.index_reg_, ea.scale_);
		ost << ')';
		break;

	case AM_ABS_W:
		ost << "$";// << std::hex;
		PrintLongWord(ost, ea.arg_, fill_addr, colon_in_long_arg);
		ost << ".W";
		break;

	case AM_ABS_L:
		ost << "$";// << std::hex;
		PrintLongWord(ost, ea.arg_, fill_addr, colon_in_long_arg);
		//ost << ".L";
		break;

	case AM_IMMEDIATE:
		ost << '#' << std::hex;
		if (ea.signed_immediate_data_)
		{
			int32 arg= static_cast<int32>(ea.arg_);
			if (arg < 0)
			{
				ost << '-';
				arg = -arg;
			}
			ost << '$' << arg;
		}
		else
		{
			ost << '$';
			ost.fill('0');
			if (size == S_BYTE)
				ost.width(2);
			else if (size == S_WORD)
				ost.width(4);
			else if (size == S_LONG)
			{
				if (colon_in_long_arg)
				{
					PrintLongWord(ost, ea.arg_, true, true);
					break;
				}
				ost.width(8);
			}
			else
				;//throw "missing size param";

			ost << ea.arg_;
		}
		break;

	case AM_RELATIVE:
		PrintOffsetAddress(ost, addr + 2, ea.arg_, fill_addr, colon_in_long_arg);
		break;

	case AM_WORD_DATA:
		ost << '$' << std::hex;
		ost.fill('0');
		ost.width(4);
		ost << ea.arg_;
		break;

	case AM_SPEC_REG:
		ost << GetSpecialRegisterName(static_cast<SpecialRegister>(ea.register_));
		break;

	case AM_REG_LIST:
		PrintRegList(ost, ea.arg_);
		//ost << "#$" << std::hex;
		//ost.fill('0');
		//ost.width(4);
		//ost << ea.arg_;
		break;

	case AM_SPECIAL:
		switch (ea.register_)
		{
		case 1:
			ost << "dc";
			break;
		case 2:
			ost << "ic";
			break;
		case 3:
			ost << "bc";
			break;
		default:
			ost << "??";
			break;
		}
		break;

	case AM_Rx_Ry:
		PrintRegister(ost, ea.register_, ea.reg_1_word_);
		ost << ", ";
		PrintRegister(ost, ea.arg_, ea.reg_2_word_);
		switch (ea.scale_)
		{
		case 1:
			ost << " <<";
			break;
		case 3:
			ost << " >>";
			break;
		}
		break;

	case AM_BOGUS:
		ost << "<Unknown-Addressing-Mode>";
		break;

	default:
		if (ea.mode_ == (AM_Dx | AM_Dx_Dy))	// hack to support DIV/REM
		{
			ost << dreg << ':';
			dreg[1]= '0' + ea.arg_;
			ost << dreg;
			break;
		}

		throw RunTimeError("missing add mode handler " __FUNCTION__);
	}
}


SpecialRegister DecodeControlRegisterField(uint16 code)
{
	switch (code & 0x0fff)
	{
	case 0x002:	return REG_CACR;
	case 0x003:	return REG_ASID;
	case 0x004:	return REG_ACR0;
	case 0x005:	return REG_ACR1;
	case 0x006:	return REG_ACR2;
	case 0x007:	return REG_ACR3;
	case 0x008:	return REG_MMUBAR;

	case 0x800:	return REG_OTHER_A7;
	case 0x801:	return REG_VBR;
	case 0x804:	return REG_MACSR;
	case 0x805:	return REG_MASK;
	case 0x806:	return REG_ACC0;
	case 0x807:	return REG_ACC1;
	case 0x808:	return REG_ACC2;
	case 0x809:	return REG_ACC3;
	case 0x80a:	return REG_ACCext01;	// verify
	case 0x80b:	return REG_ACCext02;	// ditto
	case 0x80f:	return REG_PC;

	case 0xc00:	return REG_ROMBAR0;
	case 0xc01:	return REG_ROMBAR1;
	case 0xc04:	return REG_RAMBAR0;
	case 0xc05:	return REG_RAMBAR1;

	case 0xc0c:	return REG_MPCR;
	case 0xc0d:	return REG_EDRAMBAR;
	case 0xc0e:	return REG_SECMBAR;
	case 0xc0f:	return REG_MBAR;

	case 0xd02:	return REG_PCR1U0;
	case 0xd03:	return REG_PCR1L0;
	case 0xd04:	return REG_PCR2U0;
	case 0xd05:	return REG_PCR2L0;
	case 0xd06:	return REG_PCR3U0;
	case 0xd07:	return REG_PCR3L0;

	case 0xd0a:	return REG_PCR1U1;
	case 0xd0b:	return REG_PCR1L1;
	case 0xd0c:	return REG_PCR2U1;
	case 0xd0d:	return REG_PCR2L1;
	case 0xd0e:	return REG_PCR3U1;
	case 0xd0f:	return REG_PCR3L1;
	}

	return REG_INVALID;
}


const char* GetSpecialRegisterName(SpecialRegister reg)
{
	switch (reg)
	{
	case REG_INVALID:	return "<unknown-register>";
	case REG_SR:		return "SR";
	case REG_CCR:		return "CCR";
	case REG_USP:		return "USP";
	case REG_CACR:		return "CACR";
	case REG_ASID:		return "ASID";
	case REG_ACR0:		return "ACR0";
	case REG_ACR1:		return "ACR1";
	case REG_ACR2:		return "ACR2";
	case REG_ACR3:		return "ACR3";
	case REG_MMUBAR:	return "MMUBAR";
	case REG_OTHER_A7:	return "OTHER_A7";
	case REG_VBR:		return "VBR";
	case REG_PC:		return "PC";
	case REG_ROMBAR0:	return "ROMBAR0";
	case REG_ROMBAR1:	return "ROMBAR1";
	case REG_RAMBAR0:	return "RAMBAR0";
	case REG_RAMBAR1:	return "RAMBAR1";
	case REG_MPCR:		return "MPCR";
	case REG_EDRAMBAR:	return "EDRAMBAR";
	case REG_SECMBAR:	return "SECMBAR";
	case REG_MBAR:		return "MBAR";
	case REG_PCR1U0:	return "PCR1U0";
	case REG_PCR1L0:	return "PCR1L0";
	case REG_PCR2U0:	return "PCR2U0";
	case REG_PCR2L0:	return "PCR2L0";
	case REG_PCR3U0:	return "PCR3U0";
	case REG_PCR3L0:	return "PCR3L0";
	case REG_PCR1U1:	return "PCR1U1";
	case REG_PCR1L1:	return "PCR1L1";
	case REG_PCR2U1:	return "PCR2U1";
	case REG_PCR2L1:	return "PCR2L1";
	case REG_PCR3U1:	return "PCR3U1";
	case REG_PCR3L1:	return "PCR3L1";
	// MAC
	case REG_MACSR:		return "MACSR";
	case REG_MASK:		return "MASK";
	case REG_ACC0:		return "ACC0";
	case REG_ACC1:		return "ACC1";
	case REG_ACC2:		return "ACC2";
	case REG_ACC3:		return "ACC3";
	case REG_ACCext01:	return "ACCext01";
	case REG_ACCext02:	return "ACCext02";

	default:
		throw LogicError("missing spec reg name " __FUNCTION__);
	}
}


DecodedInstruction DecodeInstruction(Context& ctx, uint32 addr)
{
	DecodedInstruction out;

	InstrPointer p(ctx, addr);
	if (!p.ValidMemory())
		return out;

	if (auto i= ctx.GetInstruction(p.OpCode()))
	{
		out = i->Decode(p);

		if (out.src_.mode_ != AM_NONE)
			if ((i->SourceEAModes() & out.src_.mode_) == AM_NONE)	// unsupported AM?
				out.unknown_instr_ = true;

		if (out.dest_.mode_ != AM_NONE)
			if ((i->DestinationEAModes() & out.dest_.mode_) == AM_NONE)	// unsupported AM?
				out.unknown_instr_ = true;

		out.words_ = p.GetWords();

		if (out.words_ > 3)
			out.unknown_instr_ = true;
		else if (out.words_ > 1)
			out.ext_ = p.GetExt();
	}
	else
		out.unknown_instr_ = true;

	if (out.unknown_instr_)
	{
		out.opcode_ = p.OpCode();
		out.words_ = 1;
	}

	return out;
}


namespace {
	void Output(std::ostringstream& ost, const char* name, int lowercase)
	{
		if (lowercase)
		{
			for (const char* p= name; *p; ++p)
				ost << char(tolower(*p));
		}
		else
			ost << name;
	}
}


std::string DecodedInstruction::ToString(uint32 address, unsigned int show_flags, char tab) const
{
	bool invalid= !Valid();

	std::ostringstream ost;
	const char TAB= tab;
	ost << std::hex;
	ost.fill('0');

	if (show_flags & SHOW_ADDRESS)
	{
		PrintLongWord(ost, address, true, !!(show_flags & COLON_IN_ADDRESS));
		ost << "  ";
	}

	if (words_ == 0)
		return ost.str();

	if (show_flags & SHOW_CODE_BYTES)
	{
		ost.width(4);
		ost << opcode_ << ' ';

		if (words_ == 1 || invalid)
			ost << "          ";
		else if (words_ == 2)
		{
			ost.width(4);
			ost << ext_ << "      ";
		}
		else if (words_ == 3)
		{
			ost.width(4);
			ost << ((ext_ >> 16) & 0xffff) << ' ';
			ost.width(4);
			ost << (ext_ & 0xffff) << ' ';
		}
	}

	if (show_flags & SHOW_CODE_CHARS)
	{
		uint8 text[8];
		int len= 0;
		text[0] = opcode_ >> 8;
		text[1] = opcode_ & 0xff;

		if (words_ == 1 || invalid)
			len = 2;
		else if (words_ == 2)
		{
			len = 4;
			text[2] = ext_ >> 8;
			text[3] = ext_ & 0xff;
		}
		else if (words_ == 3)
		{
			len = 6;
			text[2] = ext_ >> 24;
			text[3] = ext_ >> 16;
			text[4] = ext_ >> 8;
			text[5] = ext_ & 0xff;
		}

		ost << " '";

		for (int i= 0; i < 8; ++i)
		{
			uint8 c= ' ';
			if (i < len)
			{
				c = text[i];

				if (c >= 0 && c < ' ')
					c = '.';
				else if (c >= 0x80)
					c = '.';
			}
			else if (i == len)
				c = '\'';

			ost << char(c);
		}
	}

	if (invalid)
	{
		ost << TAB;
		Output(ost, "DC.W ", show_flags & LOWERCASE_MNEMONICS);
		ost << "$" << std::hex;
		ost.fill('0');
		ost.width(4);
		ost << opcode_;
	}
	else
	{
		ost << TAB;

		Output(ost, name_, show_flags & LOWERCASE_MNEMONICS);

		Output(ost, InstrSizeName(), show_flags & LOWERCASE_SIZE);

		bool colon= !!(show_flags & COLON_IN_LONG_ARGS);

		if (src_.IsValid())
		{
			ost << TAB;
			PrintEffectiveAddress(ost, src_, size_, address, true, colon);
		}

		if (dest_.IsValid())
		{
			if (src_.IsValid())
				ost << ", ";
			else
				ost << TAB;

			PrintEffectiveAddress(ost, dest_, size_, address, true, colon);
		}
	}

	return ost.str();
}


bool DecodedInstruction::Valid() const
{
	bool invalid= unknown_instr_ || src_.mode_ == AM_BOGUS || dest_.mode_ == AM_BOGUS;
	return !invalid;
}
