/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#pragma once
#include "Import.h"
#include "MachineDefs.h"


// ColdFire ISA; (bit field to allow combinations of different ISAs)

enum class ISA : uint32	// Instruction Set Architecture
{
	A= 1,				// ISA_A
	B= 2,				// ISA_B
	C= 4,				// ISA_C
	A_PLUS= 0x100,		// ISA_A+
	ARHITECTURE_MASK= 0xfff,	// mask

	MAC=	0x001000,	// multiply-accumulate unit
	EMAC=	0x002000,	// enhanced MAC
	EMAC_B=	0x004000,	// enhanced MAC, revision B

	FPU=	0x040000,	// floating point unit
	MMU=	0x100000,	// memory management unit

	None=	0
};


inline ISA operator | (ISA e1, ISA e2)
{
	return static_cast<ISA>(static_cast<uint32>(e1) | static_cast<uint32>(e2));
}


inline ISA Architecture(ISA isa)
{
	return static_cast<ISA>(static_cast<uint32>(isa) & static_cast<uint32>(ISA::ARHITECTURE_MASK));
}


inline bool IsIsaPresent(ISA e1, ISA e2)
{
	return (static_cast<uint32>(e1) & static_cast<uint32>(e2)) != 0;
}


CF_DECL ISA StringToISA(const std::wstring& isa);
CF_DECL ISA StringToISA(const std::string& isa);
CF_DECL const wchar_t* ISAToString(ISA isa);
