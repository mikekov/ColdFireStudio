/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

// Simple interrupt controller implementation

#include "pch.h"
#include "SimpleInterruptController.h"
#include "../Context.h"
#include "../PeripheralRepository.h"
#include <array>
#include "../Exceptions.h"

// register ICM
static auto reg_uart= GetPeripherals().Register("icm", "simple", &CreateDevice<SimpleInterruptController>);


enum Offset : uint16	// offsets from MBAR
{
	ICR1= 0,
	ICR2,
	ICR3,
	ICR4,
	ICR5,
	ICR6,
	ICR7,
	ICR8,
	ICR9,
	ICR10,
	ICR11,
	ICR12,
	ICR13,
	IMR= 0x22,
	IPR= 0x26,
	END= 0x2a
};

enum : uint8
{
	AUTO_VECTOR= 0x80,
	INTERRUPT_LEVEL_POS= 2,
	INTERRUPT_MASK= 0x7,
	PRIORITY_MASK= 0x3
};


struct SimpleInterruptController::icm
{
	icm() : interrupt_pending(0), interrupt_mask(0)
	{
		vectors.fill(EX_SpuriousInterrupt);
		interrupt_counters.fill(0);
		icrs.fill(0);

		uint8 icr= 4;
		for (int i= 1; i <= 7; ++i, icr += 4)
			icrs[i] = icr;
		icrs[8] = icr;
		icrs[9] = 0x80;
		icrs[10] = 0x80;
		icrs[11] = 0x80;
	}

	//uint16 pending_exceptions_mask;
	const static size_t MAX= 32;
	uint32 interrupt_pending;
	uint32 interrupt_mask;
	std::array<CpuExceptions, MAX> vectors;
	std::array<int, 8> interrupt_counters;
	std::array<uint8, MAX> icrs;		// interrupt control registers

	int get_interrupt_level(uint16 interrupt_source) const
	{
		if (interrupt_source < icrs.size())
			return (icrs[interrupt_source] >> INTERRUPT_LEVEL_POS) & INTERRUPT_MASK;

		throw RunTimeError("wrong interrupt_source in " __FUNCTION__);
	}

	CpuExceptions interrupt_acknowledge(int level)
	{
		//todo

		return EX_DeviceSpecificStart;
	}
};


SimpleInterruptController::SimpleInterruptController(PParam params) : Peripheral(params.IOAreaSize(Offset::END))
{
	icm_ = new icm();
}


SimpleInterruptController::~SimpleInterruptController()
{
	delete icm_;
}


// called during simulator run after executing single opcode
void SimpleInterruptController::Update(Context& ctx)
{
	//if (!icm_->pending_exceptions_mask)
//		return;

	if (!icm_->interrupt_pending)
		return;

	if ((icm_->interrupt_pending & ~icm_->interrupt_mask) == 0)
		return;

	TRACE("Interrupt scheduled\n");
 
	auto il_mask= ctx.Cpu().InterruptLevel();
	if (il_mask == 7)
		return;	// how to handle priority 7 non-maskable interrupts?

	// find interrupt source with highest interrupt level and priority

	CpuExceptions vector= EX_SpuriousInterrupt;
	int level= 0;
	int priority= 0;
	int source= -1;

	for (int i= 0; i < icm::MAX; ++i)
	{
		uint64 mask= 1;
		mask <<= i;
		if ((icm_->interrupt_pending & mask) != 0 && (icm_->interrupt_mask & mask) == 0)
		{
			auto il= (icm_->icrs[i] >> INTERRUPT_LEVEL_POS) & INTERRUPT_MASK;
			auto pr= icm_->icrs[i] & PRIORITY_MASK;

			if (il > level || (il == level && pr > priority))
			{
				level = il;
				priority = pr;
				source = i;

				if (icm_->icrs[i] & AUTO_VECTOR)
					vector = static_cast<CpuExceptions>(EX_InterruptLevel_1 - 1 + il);
				else
					vector = icm_->vectors[i];
			}
		}
	}

	// enter interrupt if it's not masked

	if (level > il_mask)
	{
		TRACE("Entering interrupt, vector %d\n", int(vector));

		ctx.EnterException(vector, ctx.Cpu().pc);
		ctx.Cpu().SetInterruptLevel(level);

		if (source >= 0)
		{
			// todo: not sure who's clearing this bit
			uint64 mask= 1;
			mask <<= source;
			icm_->interrupt_pending &= ~mask;
		}
	}

	/*

	for (int level= 7; level >= il_mask; --level)
		if (icm_->interrupt_counters[level])
		{
			// interrupt acknowledge
			auto vector= icm_->interrupt_acknowledge(level);

			ctx.EnterException(vector, ctx.Cpu().pc);

			ctx.Cpu().SetInterruptLevel(level);

			return;
		}
	*/
}


// resetting device
void SimpleInterruptController::Reset()
{
	icm_->interrupt_mask = ~0;
	icm_->interrupt_pending = 0;
}


// read from device; access_size is 1, 2, or 4
uint32 SimpleInterruptController::Read(uint32 offset, int access_size)
{
	//if (access_size != 1)
	//{
	//	ASSERT(false);
	//	return 0;
	//}

	switch (offset)
	{
	case Offset::IMR:
		return icm_->interrupt_mask;

	case Offset::IPR:
		return icm_->interrupt_pending;

	default:
		if (offset >= Offset::ICR1 && offset <= Offset::ICR13)
			return icm_->icrs[1 + offset];

		// not a covered area...
		ASSERT(false);
		break;
	}

	return 0;
}


// write to device; access_size is 1, 2, or 4
void SimpleInterruptController::Write(Context& ctx, uint32 offset, int access_size, uint32 value)
{
	//if (access_size != 1)
	//{
	//	ASSERT(false);
	//	return;
	//}

	switch (offset)
	{
	case Offset::IMR:
		icm_->interrupt_mask = value;
		break;

	case Offset::IPR:
		break;	// read-only

	default:
		if (offset >= Offset::ICR1 && offset <= Offset::ICR13)
			icm_->icrs[1 + offset] = static_cast<uint8>(value);
		else
		{
			// not a covered area...
			ASSERT(false);
		}
		break;
	}
}


void SimpleInterruptController::InterruptAssert(uint16 interrupt_source, CpuExceptions vector)
{
	if (static_cast<size_t>(interrupt_source) >= icm_->vectors.size())
	{
		ASSERT(false);
		throw RunTimeError("Invalid interrupt source in " __FUNCTION__);
	}
	if (vector >= 0x100)
	{
		ASSERT(false);
		throw RunTimeError("Invalid interrupt vector in " __FUNCTION__);
	}

	uint64 mask= 1;
	mask <<= interrupt_source;
	if (icm_->interrupt_pending & mask)
	{
		// this source has already pending interrupt
		return;
	}

	icm_->interrupt_pending |= mask;

	if (icm_->interrupt_mask & mask)
		return;	// interrupt is disabled by a mask

	// get level by reading control register
	auto level= icm_->get_interrupt_level(interrupt_source);

	// determine interrupt vector: this phase happens during IACK cycle, and involves
	// interrupting source returning proper vector or interrupt controller generating auto-vector

	// auto-vectored interrupt?
//	if (icm_->icrs[interrupt_source] & AUTO_VECTOR)
//		vector = CPU::EX_InterruptLevel_1 + level - 1;

	icm_->vectors[interrupt_source] = vector;

//	icm_->interrupt_counters[level]++;

//	icm_->pending_exceptions_mask |= uint16(1 << level);
}


void SimpleInterruptController::InterruptClear(uint16 interrupt_source)
{
	if (static_cast<size_t>(interrupt_source) >= icm_->vectors.size())
	{
		ASSERT(false);
		throw RunTimeError("Invalid interrupt source in " __FUNCTION__);
	}

	// todo: consider icm_->interrupt_mask?

	uint64 mask= 1;
	mask <<= interrupt_source;
	if (icm_->interrupt_pending & mask)
	{
		icm_->interrupt_pending &= ~mask;

		icm_->vectors[interrupt_source] = EX_SpuriousInterrupt;

		auto level= icm_->get_interrupt_level(interrupt_source);

//		if (--icm_->interrupt_counters[level] == 0)
//			icm_->pending_exceptions_mask &= ~uint16(1 << level);
	}
}
