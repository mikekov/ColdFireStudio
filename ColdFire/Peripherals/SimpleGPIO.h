/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#pragma once
#include "..\Peripheral.h"
#include <array>

// Implementation of a few ports from General Purpose IO in 5475

class SimpleGPIO : public Peripheral
{
public:
	SimpleGPIO(PParam params);
	virtual ~SimpleGPIO();

	// write to device; access_size is 1, 2, or 4
	virtual void Write(Context& ctx, uint32 offset, int access_size, uint32 value);

	// called during simulator run after executing single opcode
	virtual void Update(Context& ctx) {}

	// resetting device
	virtual void Reset();

	// read from device; access_size is 1, 2, or 4
	virtual uint32 Read(uint32 offset, int access_size);

private:
	std::array<uint8, 8> output_data_;
};
