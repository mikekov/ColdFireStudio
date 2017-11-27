/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#pragma once

// Exception Vector Assignments
/*
Vector	Vector		Stacked			Assignment
Numbers	Offset		  PC
0		000			—				Initial stack pointer (SSP for cores with dual stack pointers)
1		004			—				Initial program counter
2		008			Fault			Access error
3		00C			Fault			Address error
4		010			Fault			Illegal instruction
5(2		014			Fault			Divide by zero
6–7		018–01C		—				Reserved
8		020			Fault			Privilege violation
9		024			Next			Trace
10		028			Fault			Unimplemented line-a opcode
11		02C			Fault			Unimplemented line-f opcode
12(3	030			Next			Non-PC breakpoint debug interrupt
13(3	034			Next			PC breakpoint debug interrupt
14		038			Fault			Format error
15		03C			Next			Uninitialized interrupt
16–23	040–05C		—				Reserved
24		060			Next			Spurious interrupt
25–31(4	064–07C		Next			Level 1–7 autovectored interrupts
32–47	080–0BC		Next			Trap #0–15 instructions

48(5	0C0			Fault			Floating-point branch on unordered condition
49(5	0C4			NextFP or Fault	Floating-point inexact result
50(5	0C8			NextFP			Floating-point divide-by-zero
51(5	0CC			NextFP or Fault	Floating-point underflow
52(5	0D0			NextFP or Fault	Floating-point operand error
53(5	0D4			NextFP or Fault	Floating-point overflow
54(5	0D8			NextFP or Fault	Floating-point input not-a-number (NAN)
55(5	0DC			NextFP or Fault	Floating-point input denormalized number
56–60	0E0–0F0		—				Reserved
61(6	0F4			Fault			Unsupported instruction
62–63	0F8–0FC		—				Reserved
64–255	100–3FC		Next			User-defined interrupt

(1 ‘Fault’ refers to the PC of the faulting instruction. ‘Next’ refers to the PC of the instruction immediately after the faulting 
instruction. ‘NextFP’ refers to the PC of the next floating-point instruction.
(2 If the divide unit is not present (5202, 5204, 5206), vector 5 is reserved.
(3 On V2 and V3, all debug interrupts use vector 12; vector 13 is reserved.
(4 Support for autovectored interrupts is dependent on the interrupt controller implementation. Consult the specific device 
reference manual for additional details.
(5 If the FPU is not present, vectors 48 - 55 are reserved.
(6 Some devices do not support this exception; refer to CF Manual Table 11-3.
*/


enum CpuExceptions : uint16
{
	EX_InitialSP= 0,
	EX_InitialPC= 1,
	EX_AccessError= 2,
	EX_AddressError= 3,
	EX_IllegalInstruction= 4,
	EX_DivideByZero= 5,
	// 6-7 reserved
	EX_PrivilegeViolation= 8,
	EX_Trace= 9,
	EX_UnimplementedLineAOpcode= 10,
	EX_UnimplementedLineFOpcode= 11,
	// 12(3	030			Next			Non-PC breakpoint debug interrupt
	// 13(3	034			Next			PC breakpoint debug interrupt
	EX_FormatError= 14,				// 14		038			Fault		Format error
	EX_UninitializedInterrupt= 15,	// 15		03C			Next		Uninitialized interrupt
	// 16–23	040–05C		—			Reserved
	EX_SpuriousInterrupt= 24,		// 24		060			Next		Spurious interrupt
									// 25–31	064–07C		Next		Level 1–7 autovectored interrupts
	EX_InterruptLevel_1= 25,
	EX_InterruptLevel_2= 26,
	EX_InterruptLevel_3= 27,
	EX_InterruptLevel_4= 28,
	EX_InterruptLevel_5= 29,
	EX_InterruptLevel_6= 30,
	EX_InterruptLevel_7= 31,

	EX_Trap_0= 32,
	EX_Trap_15= 47,

	// 48–60	0C0–0F0		—			Reserved
	EX_FP_BranchOnUnorderedCond= 48,
	EX_UnsupportedInstruction= 61,	// 61		0x0F4		Fault		Unsupported instruction
	// 62–63	0x0F8–0x0FC	—			Reserved
	// 64–102	0x100–0x198	Next		Device-specific interrupts (range varies in different MCUs)
	EX_DeviceSpecificStart= 64,
	// 103–255	0x19C–0x3FC	—			Reserved
	//
	EX_SIZE= 256
};
