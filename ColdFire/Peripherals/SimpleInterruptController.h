/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#pragma once
#include "..\Peripheral.h"
#include "..\InterruptController.h"


class SimpleInterruptController : public Peripheral, public InterruptController
{
public:
	SimpleInterruptController(PParam params);
	virtual ~SimpleInterruptController();

	// called during simulator run after executing single opcode
	virtual void Update(Context& ctx);

	// resetting device
	virtual void Reset();

	// read from device; access_size is 1, 2, or 4
	virtual uint32 Read(uint32 offset, int access_size);

	// write to device; access_size is 1, 2, or 4
	virtual void Write(Context& ctx, uint32 offset, int access_size, uint32 value);

	// interrupt controller methods

	virtual void InterruptAssert(uint16 interrupt_source, CpuExceptions vector);
	virtual void InterruptClear(uint16 interrupt_source);

private:
	struct icm;
	icm* icm_;
};
