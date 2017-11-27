/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#pragma once
#include "Export.h"

// Basic type definitions exposed to clients of simulator

namespace cf {

typedef unsigned __int8		uint8;
typedef unsigned __int16	uint16;
typedef unsigned __int32	uint32;
typedef unsigned __int64	uint64;

typedef __int8				int8;
typedef __int16				int16;
typedef __int32				int32;
typedef __int64				int64;


enum Register
{
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

	//R_CACR,
	//R_ASID,
	//R_ACR0,
	//R_ACR1,
	//R_ACR2,
	//R_ACR3,
	//R_MMUBAR,
	R_MBAR,
	R_VBR,
};


CF_DECL const char* GetRegisterName(Register reg);


enum StatusRegBit : uint16
{
	SR_CARRY=				0x01,
	SR_OVERFLOW=			0x02,
	SR_ZERO=				0x04,
	SR_NEGATIVE=			0x08,
	SR_EXTEND=				0x10,
	SR_BRENCH_PREDICTION=	0x80,
	SR_INTERRUPT_MASK=		7 << 8,
	SR_MASTER=				1 << 12,
	SR_SUPERVISOR=			1 << 13,
	SR_TRACE=				1 << 15,
	SR_INTERRUPT_POS= 8, // position not mask
};


enum Flag
{
	F_CARRY,
	F_OVERFLOW,
	F_ZERO,
	F_NEGATIVE,
	F_EXTENDED,
	F_SUPERVISOR,
	F_TRACE,
};


// events reported by simulator to the client
enum Event
{
	E_RUNNING,		// state has changed, simulator may be running
	E_REGISTER,		// register has been changed (from the outside of a simulator)
	E_MEMORY,		// memory has been changed (from the outside of a simulator)
	E_PROG_SET,		// program has been set
	E_EXEC_STOPPED,	// state has changed, simulation is stopped
	E_DEVICE_IO		// device has been read or written to
};


// type of device operation for E_DEVICE_IO event
enum class DeviceAccess
{
	Read,
	Write,
	Reset
};


// for memory banks:
enum class MemoryAccess
{
	Normal,		// read/write access
	ReadOnly,	// only read (ROM/flash)
	Null		// write sink, null read - no memory, just a reserved area
};


class MemoryBankInfo
{
public:
	MemoryBankInfo(std::string name, MemoryAccess access, uint32 base, uint32 end)
		: name(name), access(access), base(base), end(end)
	{}

	MemoryBankInfo() : base(1), end(0), access(MemoryAccess::Null)
	{}

	uint32 Base() const				{ return base; }
	uint32 End() const				{ return end; }
	uint64 Size() const				{ return static_cast<uint64>(end) - base + 1; }
	cf::MemoryAccess Access() const	{ return access; }
	const std::string& Name() const	{ return name; }
	bool IsValid() const			{ return Size() > 0; }

private:
	std::string name;
	MemoryAccess access;
	uint32 base;
	uint32 end;
};


// Simulator I/O works by writing/reading simulator "ports"; following ports are defined:
//
enum class SimPort : uint32
{
	// general ports
	RAM_BASE= 0,			// RAM bank start address
	RAM_SIZE= 4,			// size of RAM
	TICK_COUNT= 8,			// timer with ms resolution
	DATE_TIME= 0xc,			// date time in 2 sec resolution
	PROG_START_ADDR= 0x10,	// address of code to start or 0
	// terminal ports
	CLEAR= 0x14,			// clear terminal
	IN_OUT= 0x16,			// character in/out
	X_POS= 0x18,			// cursor position
	Y_POS= 0x1a,
	WIDTH= 0x1c,			// terminal size
	HEIGHT= 0x1e,
	// other ports
	RND_NUM= 0x20			// random number
};


} // namespace
