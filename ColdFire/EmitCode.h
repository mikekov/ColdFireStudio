/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#pragma once
#include "MachineDefs.h"
#include "BasicTypes.h"
#include "OutputPointer.h"
class Instruction;

// Helper routines to generate code from assembled input

uint16 DataRegToNumber(CpuRegister data_reg);
uint16 AddressRegToNumber(CpuRegister addr_reg);
uint16 RegisterToNumber(CpuRegister reg);
uint16 RegisterToCode(CpuRegister reg);

int Encode_EA(uint16 opcode, const EffectiveAddress& ea, int mode_bit_pos, int reg_bit_pos, uint16 words[]);

void Emit_EA(uint16 opcode, const EffectiveAddress& ea, OutputPointer& ctx);

void Emit_Dx_EA(uint16 opcode, CpuRegister data_reg, int reg_bit_pos, const EffectiveAddress& ea, OutputPointer& ctx);
void Emit_Ax_EA(uint16 opcode, CpuRegister data_reg, int reg_bit_pos, const EffectiveAddress& ea, OutputPointer& ctx);
void Emit_Reg_EA(uint16 opcode, CpuRegister reg, int reg_bit_pos, const EffectiveAddress& ea, OutputPointer& ctx);
void Emit_Imm_EA(uint16 opcode, uint16 imm_value, int val_bit_pos, const EffectiveAddress& ea, OutputPointer& ctx);

void Emit_Dx_Imm(uint16 opcode, CpuRegister data_reg, int reg_bit_pos, const Expr& val, InstructionSize size, OutputPointer& ctx);
void Emit_Dx(uint16 opcode, CpuRegister data_reg, OutputPointer& ctx);
void Emit_Ax(uint16 opcode, CpuRegister data_reg, OutputPointer& ctx);

void Emit_Dx_Dy(uint16 opcode, CpuRegister dx, int reg_bit_pos1, CpuRegister dy, int reg_bit_pos2, OutputPointer& ctx);

void Emit_Opcode_DataReg_EA(const Instruction* i, const EffectiveAddress& ea_src, const EffectiveAddress& ea_dst, OutputPointer& ctx);
void Emit_Opcode_AddrReg_EA(const Instruction* i, const EffectiveAddress& ea_src, const EffectiveAddress& ea_dst, OutputPointer& ctx);

void Emit_Ext_EA(uint16 opcode, uint16 ext_word, const EffectiveAddress& ea, OutputPointer& ctx);
