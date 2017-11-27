/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#pragma once
#include "BasicTypes.h"
#include "CpuExceptions.h"


// Interface for interrupt controllers

class Context;


class InterruptController : boost::noncopyable
{
public:
	InterruptController() {}
	virtual ~InterruptController() {}

	virtual void InterruptAssert(uint16 interrupt_source, CpuExceptions vector) = 0;

	virtual void InterruptClear(uint16 interrupt_source) = 0;
};
