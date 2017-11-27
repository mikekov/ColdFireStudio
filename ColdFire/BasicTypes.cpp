/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "BasicTypes.h"
#include "Exceptions.h"


Expr::Expr() : inf(EX_NONE)
{}

Expr::Expr(int32 value) : inf(EX_LONG), value(value)
{}

Expr::Expr(FixedString str) : inf(EX_STRING), value(0), string(str)
{}

Expr Expr::Undefined()		{ Expr e; e.inf = EX_UNDEF; return e; }

bool Expr::IsNumber() const
{
	return inf == EX_BYTE || inf == EX_WORD || inf == EX_LONG;
}

bool Expr::IsRegMask() const
{
	return inf == EX_REGISTER || inf == EX_REG_LIST;
}

int32 Expr::Value() const
{
	if (IsNumber())
		return value;
	else
		throw RunTimeError("not a number " __FUNCTION__);
}

int32 Expr::RegMask() const
{
	if (IsRegMask())
		return value;
	else
		throw RunTimeError("not a register mask " __FUNCTION__);
}

bool Expr::IsValid() const
{
	return inf != EX_NONE;
}

bool Expr::IsDefined() const
{
	return inf != EX_UNDEF && inf != EX_NONE;
}

Expr::Expr(Type type, int32 value) : inf(type), value(value)
{}
