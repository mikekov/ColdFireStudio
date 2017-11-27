/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2013 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include <assert.h>
//#include <mutex>
#include "SimpleLCDController.h"
#include "../Context.h"
#include "../PeripheralRepository.h"
#include "../Exceptions.h"
#include "../HexNumber.h"

// register SimpleLCDController
static auto reg_uart= GetPeripherals().Register("display", "simple_lcd", &CreateDevice2<SimpleLCDController>);


struct SimpleLCDController::_dev_data
{
	_dev_data(int width, int height) : enabled(false)
	{
		width_ = width;
		height_ = height;
		screen_base_addr_ = 0;
	}

	bool enabled;
	cf::uint32 width_;
	cf::uint32 height_;
	cf::uint32 screen_base_addr_;
};


SimpleLCDController::SimpleLCDController(PParam params, PeripheralConfigData& config) : Peripheral(params)
{
	auto width= config.get<unsigned int>("width", 160);
	if (width > 1024)
		throw RunTimeError("SimpleLCDController: Unsupported width");

	auto height= config.get<unsigned int>("height", 16);
	if (height > 1024)
		throw RunTimeError("SimpleLCDController: Unsupported height");

	data = new _dev_data(width, height);

	data->screen_base_addr_ = config.get<HexNumber<unsigned int>>("screen_base_address", 0) & ~cf::uint32(3);
}


SimpleLCDController::~SimpleLCDController()
{
	delete data;
}


// called during simulator run after executing single opcode
void SimpleLCDController::Update(Context& ctx)
{
	// no-op
}

// resetting device
void SimpleLCDController::Reset()
{
	// clear display?
	//
}


enum Ports { Control= 0, Command= 0x4, ScreenSize= 0x8, ScreenBaseAddr= 0x10, };


// read from block device; access_size is 1, 2, or 4; only 4 is legal
uint32 SimpleLCDController::Read(uint32 offset, int access_size)
{
	uint32 result= 0;

	if (access_size != 4)
		return result;

	switch (offset)
	{
	case Control:
		break;

	case Command:
		break;

	case ScreenSize:
		result = data->width_ << 16 | data->height_;
		break;

	case ScreenBaseAddr:
		result = data->screen_base_addr_;
		break;
	}

	return result;
}


// write to block device; access_size is 1, 2, or 4; only 4 is legal
void SimpleLCDController::Write(Context& ctx, uint32 offset, int access_size, uint32 value)
{
	if (access_size != 4)
		return;

	switch (offset)
	{
	case Control:
		// todo
		break;

	case Command:
		// todo
		switch (value)
		{
		case 0:
			data->enabled = false;
			break;
		case 1:
			data->enabled = true;
			break;

		}
		break;

	case ScreenSize:
		data->width_ = (value >> 16) & 0x3fc;
		data->height_ = value & 0x3ff;
		break;

	case ScreenBaseAddr:
		data->screen_base_addr_ = value & ~3;
		break;

	}
}


// display device parameters
cf::uint32 SimpleLCDController::GetWidth() const				{ return data->width_; }
cf::uint32 SimpleLCDController::GetHeight() const				{ return data->height_; }
cf::uint32 SimpleLCDController::GetBitsPerPixel() const			{ return 1; }
cf::uint32 SimpleLCDController::GetScreenBaseAddress() const	{ return data->screen_base_addr_; }
DisplayDevice::NotifyType SimpleLCDController::ChangeNotification() const
{ return DisplayDevice::NotifyType::Polling; } // this display needs polling, there is no notification sent when screen memory changes
