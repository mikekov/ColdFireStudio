/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

// Dummy device - a sink for writes, null for reads

#include "pch.h"
#include "Dummy.h"
#include "../Context.h"
#include "../PeripheralRepository.h"

// register dummy
static auto reg_dummy= GetPeripherals().Register("dummy", "", &CreateDevice<Dummy>);


Dummy::Dummy(PParam params) : Peripheral(params)
{}


Dummy::~Dummy()
{}


// called during simulator run after executing single opcode
void Dummy::Update(Context& ctx)
{}


// resetting device
void Dummy::Reset()
{}


// read from device; access_size is 1, 2, or 4
uint32 Dummy::Read(uint32 offset, int access_size)
{
	return 0;
}


// write to device; access_size is 1, 2, or 4
void Dummy::Write(Context& ctx, uint32 offset, int access_size, uint32 value)
{}
