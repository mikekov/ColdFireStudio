#include "pch.h"
#include "SimpleOut.h"
#include "..\Context.h"
#include "..\PeripheralRepository.h"


// register SimpleOut
static auto reg_uart= GetPeripherals().Register("psc", "simple", &CreateDevice<SimpleOut>);


SimpleOut::SimpleOut(PParam params) : Peripheral(params.IOAreaSize(0x100))
{}


SimpleOut::~SimpleOut()
{}


void SimpleOut::Write(Context& ctx, uint32 offset, int access_size, uint32 value)
{
	switch (offset)
	{
	case 0xc:	// PSC Transmit Buffer
		if (access_size == 4)
		{
			// "transmit" bytes from highest to lowest
			for (int i= 3; i >= 0; --i)
			{
				uint32 c= (value >> i * 8) & 0xff;
				ctx.SimWrite(cf::SimPort::IN_OUT, c);
			}
		}
		break;
	}
}
