/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#pragma once
#ifndef _ident_h_
#define _ident_h_

#include "BasicTypes.h"


struct Ident
{
	enum IdentInfo	// info o identyfikatorze
	{
		I_INIT,
		I_UNDEF,		// identyfikator niezdefiniowany
		I_ADDRESS,		// identyfikator zawiera adres
		I_VALUE,		// identyfikator zawiera wartoœæ liczbow¹
		I_MACRONAME,	// identyfikator jest nazw¹ makrodefinicji
		I_MACROADDR,	// identyfikator zawiera adres w makrodefinicji
	} info;
	Expr val;
	bool checked;	// definicja identyfikatora potwierdzona w drugim przejœciu asemblacji
	bool variable;	// identyfikator zmiennej

	Ident() : info(I_INIT), checked(false), variable(false)
	{}

	Ident(IdentInfo info, int32 value= 0, bool variable= false)
		: info(info), val(value), checked(false), variable(variable)
	{}
};

#endif
