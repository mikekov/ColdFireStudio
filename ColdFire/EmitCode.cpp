/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "EmitCode.h"
#include "OpcodeDefs.h"
#include "Instruction.h"


uint16 ExprToScale(const Expr& expr)
{
	switch (expr.inf)
	{
	case Expr::EX_UNDEF:
	case Expr::EX_NONE:
		return 0;

	case Expr::EX_BYTE:
	case Expr::EX_WORD:
	case Expr::EX_LONG:
		switch (expr.value)
		{
		case 1:	return 0;
		case 2:	return 1;
		case 4:	return 2;
		case 8:	return 3;
		}
		throw LogicError("expected scale number 1, 2, 4, or 8 " __FUNCTION__);
		break;

	default:
		throw LogicError("expected valid numeric expression " __FUNCTION__);
		break;
	}
}

uint16 ExprToByte(const Expr& expr)
{
	switch (expr.inf)
	{
	case Expr::EX_BYTE:
	case Expr::EX_WORD:
	case Expr::EX_LONG:
		return static_cast<uint16>(expr.value & 0xff);
		break;

	default:
		throw LogicError("expected byte expression " __FUNCTION__);
		break;
	}
}

void ExprToLong(const Expr& expr, uint16& hi, uint16& lo)
{
	switch (expr.inf)
	{
	case Expr::EX_BYTE:
	case Expr::EX_WORD:
	case Expr::EX_LONG:
		hi = static_cast<uint16>(expr.value >> 16);
		lo = static_cast<uint16>(expr.value & 0xffff);
		break;

	default:
		throw LogicError("expected valid numeric expression " __FUNCTION__);
		break;
	}
}

uint16 ExprToWord(const Expr& expr)
{
	switch (expr.inf)
	{
	case Expr::EX_BYTE:
	case Expr::EX_WORD:
	case Expr::EX_LONG:
		return static_cast<uint16>(expr.value & 0xffff);
		break;

	default:
		throw LogicError("expected byte expression " __FUNCTION__);
		break;
	}
}

uint16 DataRegToNumber(CpuRegister data_reg)
{
	if (data_reg < R_D0 || data_reg > R_D7)
		throw LogicError("data reg expected " __FUNCTION__);

	uint16 d_reg= static_cast<uint16>(data_reg - R_D0);
	return d_reg;
}

uint16 AddressRegToNumber(CpuRegister addr_reg)
{
	if (addr_reg < R_A0 || addr_reg > R_SP)
		throw LogicError("address reg expected " __FUNCTION__);

	uint16 a_reg= addr_reg == R_SP ? 7 : static_cast<uint16>(addr_reg - R_A0);
	return a_reg;
}

uint16 RegisterToNumber(CpuRegister reg)
{
	if (reg >= R_D0 && reg <= R_D7)
		return DataRegToNumber(reg);
	else
		return 8 + AddressRegToNumber(reg);
}

uint16 RegisterToCode(CpuRegister reg)
{
	switch (reg)
	{
	case R_PC:		return 0x80f;
	//case R_SR:		return ;	// CFRM doesn't list those as supported by MOVEC
	//case R_CCR:		return ;	// although DBug uses them anyway
	//case R_USP:		return ;
	//case R_ACC0:	return ;
	//case R_ACC1:	return ;
	//case R_ACC2:	return ;
	//case R_ACC3:	return ;

	case R_CACR:	return 2;
	case R_ASID:	return 3;
	case R_ACR0:	return 4;
	case R_ACR1:	return 5;
	case R_ACR2:	return 6;
	case R_ACR3:	return 7;
	case R_MMUBAR:	return 8;

	case R_VBR:		return 0x801;
	case R_MBAR:	return 0xc0f;

	case R_ROMBAR0:	return 0xc00;
	case R_ROMBAR1:	return 0xc01;
	case R_RAMBAR0:	return 0xc04;
	case R_RAMBAR1:	return 0xc05;

	default:
		return 0;
	}
}


int Encode_EA(uint16 opcode, const EffectiveAddress& ea, int mode_bit_pos, int reg_bit_pos, uint16 words[])
{
	uint16 mode= 0;
	uint16 reg= 0;
	uint16 ext1= 0;
	uint16 ext2= 0;
	int ext_words= 0;

	switch (ea.mode_)
	{
	case AM_Dx:
		mode = EA_Dx;
		reg = DataRegToNumber(ea.first_reg_);
		break;

	case AM_Ax:
		mode = EA_Ax;
		reg = AddressRegToNumber(ea.first_reg_);
		break;

	case AM_INDIRECT_Ax:
		mode = EA_Ax_IND;
		reg = AddressRegToNumber(ea.first_reg_);
		break;

	case AM_Ax_INC:
		mode = EA_Ax_INC;
		reg = AddressRegToNumber(ea.first_reg_);
		break;

	case AM_DEC_Ax:
		mode = EA_DEC_Ax;
		reg = AddressRegToNumber(ea.first_reg_);
		break;

	case AM_DISP_Ax:
		mode = EA_DISP_Ax;
		reg = AddressRegToNumber(ea.first_reg_);
		ext1 = ExprToWord(ea.val_);
		ext_words = 1;
		break;

	case AM_DISP_Ax_Ix:
		mode = EA_DISP_Ax_Ix;
		reg = AddressRegToNumber(ea.first_reg_);
		{
			ExtensionWordFmt_DISP_REG_IDX ext;
			ext.word = 0;
			ext.displacement = static_cast<int8>(ExprToByte(ea.val_));
			if (ea.second_reg_ >= R_D0 && ea.second_reg_ <= R_D7)
			{
				ext.reg = DataRegToNumber(ea.second_reg_);
				ext.DA = 0;
			}
			else
			{
				ext.reg = AddressRegToNumber(ea.second_reg_);
				ext.DA = 1;
			}
			ext.WL = 1;		// long index reg (CF doesn't support word here)
			ext.scale = ExprToScale(ea.scale_);
//			ext1 = ExprToByte(ea.val_);
//			ext1 |= RegisterToNumber(ea.second_reg_) << 12;
//			ext1 |= 1 << 11;	// long index reg (CF doesn't support word here)
//			ext1 |= ExprToScale(ea.scale_) << 9;
			ext1 = ext.word;
			ext_words = 1;
		}
		break;

	case AM_ABS_W:
		mode = EA_EXT;
		reg = EA_EXT_ABS_W;
		ext1 = ExprToWord(ea.val_);
		ext_words = 1;
		break;

	case AM_ABS_L:
		mode = EA_EXT;
		reg = EA_EXT_ABS_L;
		ExprToLong(ea.val_, ext1, ext2);
		ext_words = 2;
		break;

	case AM_DISP_PC:
		mode = EA_EXT;
		reg = EA_EXT_DISP_PC;
		ext1 = ExprToWord(ea.val_);
		ext_words = 1;
		break;

	case AM_DISP_PC_Ix:
		mode = EA_EXT;
		reg = EA_EXT_DISP_PC_Ix;
		ext1 = ExprToByte(ea.val_);
		ext1 |= RegisterToNumber(ea.second_reg_) << 12;
		ext1 |= 1 << 11;	// long index reg (CF doesn't support word here)
		ext1 |= ExprToScale(ea.scale_) << 9;
		ext_words = 1;
		break;

	case AM_IMMEDIATE:
		mode = EA_EXT;
		reg = EA_EXT_IMMEDIATE;
		ext_words = 1;
		if (ea.val_.inf == Expr::EX_BYTE)
			ext1 = ExprToByte(ea.val_);
		else if (ea.val_.inf == Expr::EX_WORD)
			ext1 = ExprToWord(ea.val_);
		else if (ea.val_.inf == Expr::EX_LONG)
		{
			ExprToLong(ea.val_, ext1, ext2);
			ext_words = 2;
		}
		else
			throw LogicError("expected valid expression for immediate addressing mode " __FUNCTION__);
		break;

	default:
		throw LogicError("unexpected addressing mode " __FUNCTION__);
	}

	words[0] = opcode | (mode << mode_bit_pos) | (reg << reg_bit_pos);

	if (ext_words == 1)
		words[1] = ext1;
	else if (ext_words == 2)
		words[1] = ext1, words[2] = ext2;

	return 1 + ext_words;
}


void Emit_Ext_EA(uint16 opcode, uint16 ext_word, const EffectiveAddress& ea, OutputPointer& ctx)
{
	uint16 words[3];
	int count= Encode_EA(opcode, ea, 3, 0, words);
	if (count == 0 || count > 2)
		throw LogicError("Illegal addressing mode combination, cannot emit code in " __FUNCTION__);

	ctx << words[0];
	ctx << ext_word;
	for (int i= 1; i < count; ++i)
		ctx << words[i];
}


void Emit_EA(uint16 opcode, const EffectiveAddress& ea, OutputPointer& ctx)
{
	uint16 words[3];
	int count= Encode_EA(opcode, ea, 3, 0, words);
	for (int i= 0; i < count; ++i)
		ctx << words[i];
}


void Emit_Ax_EA(uint16 opcode, CpuRegister addr_reg, int reg_bit_pos, const EffectiveAddress& ea, OutputPointer& ctx)
{
	if (addr_reg < R_A0 || addr_reg > R_SP)
		throw LogicError("address reg expected " __FUNCTION__);

	uint16 a_reg= AddressRegToNumber(addr_reg);
	opcode |= a_reg << reg_bit_pos;
	Emit_EA(opcode, ea, ctx);
}


void Emit_Dx_EA(uint16 opcode, CpuRegister data_reg, int reg_bit_pos, const EffectiveAddress& ea, OutputPointer& ctx)
{
	if (data_reg < R_D0 || data_reg > R_D7)
		throw LogicError("data reg expected " __FUNCTION__);

	uint16 d_reg= DataRegToNumber(data_reg);
	opcode |= d_reg << reg_bit_pos;
	Emit_EA(opcode, ea, ctx);
}


void Emit_Reg_EA(uint16 opcode, CpuRegister reg, int reg_bit_pos, const EffectiveAddress& ea, OutputPointer& ctx)
{
	if (reg >= R_D0 && reg <= R_D7)
		Emit_Dx_EA(opcode, reg, reg_bit_pos, ea, ctx);
	else
		Emit_Ax_EA(opcode, reg, reg_bit_pos, ea, ctx);
}


void Emit_Imm_EA(uint16 opcode, uint16 imm_value, int val_bit_pos, const EffectiveAddress& ea, OutputPointer& ctx)
{
	if (imm_value > 7)
		throw LogicError("value from 0..7 expected " __FUNCTION__);

	opcode |= imm_value << val_bit_pos;
	Emit_EA(opcode, ea, ctx);
}


void Emit_Dx_Imm(uint16 opcode, CpuRegister data_reg, int reg_bit_pos, const Expr& val, InstructionSize size, OutputPointer& ctx)
{
	if (data_reg < R_D0 || data_reg > R_D7)
		throw LogicError("data reg expected " __FUNCTION__);

	uint16 d_reg= DataRegToNumber(data_reg);
	opcode |= d_reg << reg_bit_pos;
	ctx << opcode;

	switch (size)//val.inf)
	{
	case IS_BYTE:
//	case Expr::EX_BYTE:
		{
			uint16 b= ExprToByte(val);
			ctx << b;
		}
		break;

	case IS_WORD:
//	case Expr::EX_WORD:
		{
			uint16 w= ExprToWord(val);
			ctx << w;
		}
		break;

	case IS_LONG:
//	case Expr::EX_LONG:
		{
			uint16 hi, lo;
			ExprToLong(val, hi, lo);
			ctx << hi << lo;
		}
		break;

	default:
		throw LogicError("unexpected expression in " __FUNCTION__);
	}
}


void Emit_Dx(uint16 opcode, CpuRegister data_reg, OutputPointer& ctx)
{
	if (data_reg < R_D0 || data_reg > R_D7)
		throw LogicError("data reg expected " __FUNCTION__);

	uint16 d_reg= DataRegToNumber(data_reg);
	opcode |= d_reg;

	ctx << opcode;
}


void Emit_Ax(uint16 opcode, CpuRegister addr_reg, OutputPointer& ctx)
{
	if (addr_reg < R_A0 || addr_reg > R_SP)
		throw LogicError("address reg expected " __FUNCTION__);

	uint16 a_reg= AddressRegToNumber(addr_reg);
	opcode |= a_reg;

	ctx << opcode;
}


void Emit_Dx_Dy(uint16 opcode, CpuRegister dx, int reg_bit_pos1, CpuRegister dy, int reg_bit_pos2, OutputPointer& ctx)
{
	if (dx < R_D0 || dx > R_D7 || dy < R_D0 || dy > R_D7)
		throw LogicError("data reg expected " __FUNCTION__);

	opcode |= (DataRegToNumber(dx) << reg_bit_pos1) | (DataRegToNumber(dy) << reg_bit_pos2);

	ctx << opcode;
}


static void Emit_Opcode_Reg_EA(AddressingMode mode, const Instruction* i, const EffectiveAddress& ea_src, const EffectiveAddress& ea_dst, OutputPointer& ctx)
{
	const int reg_bits_position= 9;

	if (i->SourceEAModes() == mode)
	{
		if (ea_src.mode_ != mode)
			throw LogicError("src adr mode error " __FUNCTION__);
		Emit_Reg_EA(i->StencilCode(), ea_src.first_reg_, reg_bits_position, ea_dst, ctx);
	}
	else if (i->DestinationEAModes() == mode)
	{
		if (ea_dst.mode_ != mode)
			throw LogicError("dst adr mode error " __FUNCTION__);
		Emit_Reg_EA(i->StencilCode(), ea_dst.first_reg_, reg_bits_position, ea_src, ctx);
	}
	else
		throw LogicError("unexpected src/dst mode " __FUNCTION__);
}

void Emit_Opcode_DataReg_EA(const Instruction* i, const EffectiveAddress& ea_src, const EffectiveAddress& ea_dst, OutputPointer& ctx)
{
	Emit_Opcode_Reg_EA(AM_Dx, i, ea_src, ea_dst, ctx);
}

void Emit_Opcode_AddrReg_EA(const Instruction* i, const EffectiveAddress& ea_src, const EffectiveAddress& ea_dst, OutputPointer& ctx)
{
	Emit_Opcode_Reg_EA(AM_Ax, i, ea_src, ea_dst, ctx);
}
