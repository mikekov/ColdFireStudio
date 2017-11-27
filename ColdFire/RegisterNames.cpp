/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "Types.h"

namespace cf {

CF_DECL const char* GetRegisterName(Register reg)
{
	switch (reg)
	{
	case R_D0:	return "D0";
	case R_D1:	return "D1";
	case R_D2:	return "D2";
	case R_D3:	return "D3";
	case R_D4:	return "D4";
	case R_D5:	return "D5";
	case R_D6:	return "D6";
	case R_D7:	return "D7";

	case R_A0:	return "A0";
	case R_A1:	return "A1";
	case R_A2:	return "A2";
	case R_A3:	return "A3";
	case R_A4:	return "A4";
	case R_A5:	return "A5";
	case R_A6:	return "A6";
	case R_A7:	return "A7";

	case R_SP:	return "SP";

	case R_PC:	return "PC";
	case R_SR:	return "SR";
	case R_CCR:	return "CCR";
	case R_USP:	return "USP";

	case R_MBAR:return "MBAR";
	case R_VBR:	return "VBR";
	}

	ASSERT(false);
	return nullptr;
}


} // namespace
