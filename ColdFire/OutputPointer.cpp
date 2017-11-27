/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "OutputPointer.h"
#include "BinaryProgram.h"
#include "Stat.h"


ProgramOrigin::ProgramOrigin()
{
	ready_ = false;
	pc_ = ~0;
}

void ProgramOrigin::Set(uint32 start)
{
	pc_ = start;
	ready_ = true;
}


uint32 ProgramOrigin::Origin() const
{
	if (!ready_)
		throw ERR_UNDEF_ORIGIN;
	return pc_;
}


ProgramOrigin::operator uint32 () const
{
	if (!ready_)
		throw ERR_UNDEF_ORIGIN;
	return pc_;
}


void ProgramOrigin::Advance(uint32 n)
{
	if (!ready_)
		throw ERR_UNDEF_ORIGIN;
	auto pc= pc_ + n;
	if (pc < pc_ || pc < n)
		throw ERR_PC_WRAPED;
	pc_ += n;
}


void ProgramOrigin::Advance(int32 n)
{
	if (!ready_)
		throw ERR_UNDEF_ORIGIN;
	if (n >= 0)
		Advance(static_cast<uint32>(n));
	else
	{
		uint32 neg= -n;
		if (neg > pc_)
			throw ERR_PC_WRAPED;
		pc_ -= neg;
	}
}


//bool ProgramOrigin::OriginDefined() const
//{
//	return ready_;
//}


bool ProgramOrigin::Missing() const
{
	return !ready_;
}


void ProgramOrigin::Reset()
{
	ready_ = false;
	pc_ = ~0;
}

//--------------------------------------------------

OutputPointer::OutputPointer(const ProgramOrigin& origin, cf::BinaryProgram& code)
	: code_(code), origin_(origin)
{}


OutputPointer& OutputPointer::operator << (uint16 word)
{
	code_.PutWord(origin_, word);
	origin_.Advance(2);
	return *this;
}

uint32 OutputPointer::Origin() const
{
	return origin_;
}
