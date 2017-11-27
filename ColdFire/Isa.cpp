/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "Isa.h"


ISA StringToISA(const std::wstring& isa)
{
	if (isa == L"ISA_A")
		return ISA::A;
	else if (isa == L"ISA_B")
		return ISA::B;
	else if (isa == L"ISA_C")
		return ISA::C;
	else if (isa == L"ISA_A+")
		return ISA::A_PLUS;
	else if (isa == L"MAC")
		return ISA::MAC;
	else if (isa == L"EMAC")
		return ISA::EMAC;
	else if (isa == L"EMAC_B")
		return ISA::EMAC_B;
	else if (isa == L"None")
		return ISA::None;
	else
	{ ASSERT(false); return ISA::A; }
//todo: enhance
}


ISA StringToISA(const std::string& isa)
{
	std::wstring temp;
	temp.assign(isa.begin(), isa.end());
	return StringToISA(temp);
}


const wchar_t* ISAToString(ISA isa)
{
	switch (Architecture(isa))
	{
	case ISA::A:		return L"ISA_A";
	case ISA::A_PLUS:	return L"ISA_A+";
	case ISA::B:		return L"ISA_B";
	case ISA::C:		return L"ISA_C";

	default:
		ASSERT(false);
		return L"?";	// mixed case is not supported
	}
}
