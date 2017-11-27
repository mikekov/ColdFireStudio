/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

// Simple timer implementation copied from ColdFire simulator 0.3.1, with changes:

/**********************************/
/*                                */
/*  Copyright 2000, David Grant   */
/*                                */
/*  see LICENSE for more details  */
/*                                */
/**********************************/

#include "pch.h"
#include <assert.h>
#include "SimpleTimer.h"
#include "../Context.h"
#include "../PeripheralRepository.h"

// register timer
static auto reg_timer= GetPeripherals().Register("timer", "5206", &CreateDevice<SimpleTimer>);

/* TMR (timer mode register) :
 *  15  14  13  12  11  10   9   8   7   6   5   4   3   2   1   0
 * +-------------------------------+-------+---+---+---+-------+---+
 * |            Prescalar          | CE1-0 |OM |ORI|FRR|CLK1-0 |RST|
 * +-------------------------------+-------+---+---+---+-------+---+
 * */
struct _TMR {
	unsigned char PS; 	/* PS 7-0 */
	char CE; 		/* CE 1-0 */
	char OM;		/* OM */
	char ORI;		/* ORI */
	char FRR;		/* FRR */
	char CLK;		/* CLK 1-0 */
	char RST;		/* RST */
};

/* TER (timer event register) :
 *   7   6   5   4   3   2   1   0
 * +-----------------------+---+---+
 * |       Reserved        |REF|CAP|
 * +-----------------------+---+---+
 * */

struct _TER {
	char REF;
	char CAP;
};


struct SimpleTimer::_timer_data
{
	_timer_data()
	{
		memset(this, 0, sizeof(*this));
	}

	struct _TMR TMR;
	uint16 TRR;
	uint16 TCR;
	uint16 TCN;
	struct _TER TER;
	char enable;
	uint32 cycles_per_tick;
	uint32 next_tick;
};


SimpleTimer::SimpleTimer(PParam params) : Peripheral(params.IOAreaSize(0x14))
{
	timer = new _timer_data();
}


SimpleTimer::~SimpleTimer()
{
	delete timer;
}


// called during simulator run after executing single opcode
void SimpleTimer::Update(Context& ctx)
{
	if (!timer->enable)
		return;

	bool check_interrupts= false;
/*0	if(board_data.use_timer_hack) {		
		/ * Hack to make things work right, ignore the prescalar and the
		 * CLK setting and the elapsed cycles, just increment 
		 * the TRR every update * /
	} else*/ {
		/* Proper timing for the timer */
		/* If we're not ready to tick, we should at least
		 *  check interrupts, because we may need to withdraw
		 *  an interrupt */
		if (ctx.CyclesTaken() < timer->next_tick) 
			check_interrupts = true;
		else
			timer->next_tick += timer->cycles_per_tick;
	}

	if (!check_interrupts)
	{
		/* From the docs, the reference isn't matched until the TCN==TRR, 
		 * AND the TCN is ready to increment again, so we'll hold off
		 * incrementing the TCN until we compare it to the TRR */

		if (timer->TCN == timer->TRR)
		{
			/* timer reference hit */
			//TRACE("         %s: TCN == TRR = %ld\n", s->name, timer->TRR);
			timer->TER.REF=1;
			if (timer->TMR.FRR) {
				//TRACE("         %s: Restart flag is on, restarting the timer\n", s->name);
				/* Restart the timer */
				timer->TCN=0;
			}
		} else {
			/* Ensure the interupt condition is off */
			timer->TER.REF=0;
		}
	
		/* Now increment the TCN
		 * FIXME: maybe we shouldn't do this if we just reset the TCN? */
		timer->TCN++;
	}
	auto interrupts_on= false;

	/* If the timer is at its reference, and ORI is set, then interrupt */
	if (timer->TER.REF && timer->TMR.ORI)
	{
		//TRACE("          %s: timer reference condition exists\n", s->name);
		interrupts_on = true;
	}

	if (!interrupts_on)
	{
	/* If we get here, we couln't find a condition to turn/leave
		the interrupts on, so turn 'em off */
/*	TRACE("%s: No interrupt conditions, withdrawing any interrupt requests\n", s->name);*/
		ctx.InterruptClear(InterruptSource());
	}
	else
	{
	//TRACE("%s: Posting interrupt request for %s\n", s->name);
		ctx.InterruptAssert(InterruptSource(), EX_SpuriousInterrupt);
	}
}


// resetting device
void SimpleTimer::Reset()
{
	memset(&timer->TMR, 0, sizeof(timer->TMR));
	timer->TRR = 0xffff;
	timer->TCR = 0;
	timer->TCN = 0;
	memset(&timer->TER, 0, sizeof(timer->TER));
	timer->enable = 0;
	timer->cycles_per_tick = 1;
	timer->next_tick = 0;
}


// read from device; access_size is 1, 2, or 4
uint32 SimpleTimer::Read(uint32 offset, int access_size)
{
	//if (access_size != 1)
	//{
	//	assert(false);
	//	return 0;
	//}

	uint32 result= 0;

	switch (offset)
	{
	case 0x0000: /* timer Mode Register (TMR) */
		result =
			(((uint32)timer->TMR.PS         ) << 8) |
			(((uint32)timer->TMR.CE & 0x0002) << 6) |
			(timer->TMR.OM ? 0x00000020 : 0x0) |
			(timer->TMR.ORI ? 0x00000010 : 0x0) |
			(timer->TMR.FRR ? 0x00000008 : 0x0) |
			(((uint32)timer->TMR.CLK & 0x0002) << 1) |
			(timer->TMR.RST ? 0x00000001 : 0x0);
			
		//TRACE("   Retrieving timer Mode Register (TMR)\n");
		break;
	case 0x0004: /* timer Reference Register  (TRR) */
		result = timer->TRR;
		//TRACE("   Retrieving timer Reference Register (TRR)\n");
		break;
	case 0x0008: /* timer Capture Register (TCR) */
		result = timer->TCR;
		//TRACE("   Retrieving timer Capture Register (TCR)\n");
		break;
	case 0x000C: /* timer Counter (TCN) */
		result = timer->TCN;
		//TRACE("   Retrieving timer Counter (TCN)\n");
		break;
	case 0x0011: /* timer Event Register (TER) */
		result =
			(timer->TER.REF ? 0x00000002 : 0x0) |
			(timer->TER.CAP ? 0x00000001 : 0x0);
		//TRACE("   Retrieving timer Event Register (TER)\n");
		break;
	default:
		break;
	}

	return result;
}


// write to device; access_size is 1, 2, or 4
void SimpleTimer::Write(Context& ctx, uint32 offset, int access_size, uint32 value)
{
	//if (access_size != 1)
	//{
	//	assert(false);
	//	return;
	//}

	switch (offset)
	{
	case 0x0000: /* timer Mode Register (TMR) */
		/* Before writing, check for a 1->0 transition on reset */
		//TRACE("   Setting timer Mode Register (TMR)\n");
		if ((timer->TMR.RST == 1) && (value & 0x0001) == 0)
			Reset();

		timer->TMR.PS = (value & 0xFF00) >> 8;
		timer->TMR.CE = (value & 0x00C0) >> 6;
		timer->TMR.OM = (value & 0x0020) ? 1 : 0;
		timer->TMR.ORI= (value & 0x0010) ? 1 : 0;
		timer->TMR.FRR= (value & 0x0008) ? 1 : 0;
		timer->TMR.CLK= (value & 0x0006) >> 1;
		timer->TMR.RST= (value & 0x0001) ;
		
		//TRACE("      PS=0x%02x, CE=%d, OM=%d, ORI=%d\n", timer->TMR.PS, timer->TMR.CE, timer->TMR.OM, timer->TMR.ORI);
		//TRACE("      FRR=%d, CLK=%d, RST=%d\n", timer->TMR.FRR, timer->TMR.CLK, timer->TMR.RST);

		/* Recompute the cycles_per_tick */
		if (timer->TMR.CLK==1)
			timer->cycles_per_tick = (timer->TMR.PS + 1);
		else if (timer->TMR.CLK==2)
			timer->cycles_per_tick = (timer->TMR.PS + 1) * 16;
		else
			timer->cycles_per_tick = 0;

		//TRACE("   cycles per tick=%ld\n", timer->cycles_per_tick);
		
		/* See if the timer should be on or off */
		/* If RST is 0, values can still be written, but clocking 
		 *  doesn't happen (1->0) transition resets the timer */
		if (timer->TMR.CLK == 0 || timer->TMR.CLK == 3 || timer->TMR.RST == 0)
		{
			/* timer is disabled */
			timer->enable=0;
		}
		else
		{
			timer->enable=1;
			timer->next_tick = ctx.CyclesTaken() + timer->cycles_per_tick;
		}
	
		break;
	case 0x0004: /* timer Reference Register  (TRR) */
		timer->TRR = (uint16)value;
		//TRACE("   Setting timer Reference Register (TRR)\n");
		break;
	case 0x0008: /* timer Capture Register (TCR) */
		timer->TCR = (uint16)value;
		//TRACE("   Setting timer Capture Register (TCR)\n");
		break;
	case 0x000C: /* timer Counter (TCN) */
		timer->TCN = (uint16)value;
		//TRACE("   Setting timer Counter (TCN)\n");
		break;
	case 0x0011: /* timer Event Register (TER) */
		//TRACE("   Setting timer Event Register (TER)\n");
		if (value & 0x2)
		{
			timer->TER.REF = 0;
			//TRACE("      Clearing Output Reference Event\n");
		}
		if (value & 0x1)
		{
			timer->TER.CAP = 0;
			//TRACE("      Clearing Capture Event\n");
		}
		/* INterrupts status will be updated on the next
		 * Tick() call */
		break;
	default:
		break;
	}
}
