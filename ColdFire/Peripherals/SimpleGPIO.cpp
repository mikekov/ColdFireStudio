#include "pch.h"
#include "SimpleGPIO.h"
#include "..\Context.h"
#include "..\PeripheralRepository.h"


// register SimpleGPIO
static auto reg_uart= GetPeripherals().Register("gpio", "5475", &CreateDevice<SimpleGPIO>);


SimpleGPIO::SimpleGPIO(PParam params) : Peripheral(params.IOAreaSize(0x100))
{
	Reset();
}


SimpleGPIO::~SimpleGPIO()
{}


enum class Offset	// offsets from MBAR
{
	PODR_FEC1L		= 0x07,
	PPDSDR_FEC1L	= 0x27,		// Fast Ethernet
	PPDSDR_PSC3PSC2	= 0x2c,
};


void SimpleGPIO::Write(Context& ctx, uint32 offset, int access_size, uint32 value)
{
	switch (offset)
	{
	case Offset::PODR_FEC1L:
		if (access_size == 1)
			output_data_[0] = uint8(value);
		break;
	}
}


uint32 SimpleGPIO::Read(uint32 offset, int access_size)
{
	switch (offset)
	{
	case Offset::PPDSDR_FEC1L:
	case Offset::PODR_FEC1L:
		if (access_size == 1)
			return (rand() >> 4) & 0xff;
		break;

	}

	return 0;
}


void SimpleGPIO::Reset()
{
	std::fill(output_data_.begin(), output_data_.end(), 0xff);
}
