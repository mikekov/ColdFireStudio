/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "LEDSegments.h"
#include "../PeripheralRepository.h"
#include "../Exceptions.h"

// register LEDSegments devices
static auto reg_led_7segments= GetPeripherals().Register("display", "led_7_segments", &CreateDevice2<LEDSegments>);
static auto reg_led_16segments= GetPeripherals().Register("display", "led_16_segments", &CreateDevice2<LEDSegments>);

LEDSegments::LEDSegments(PParam params, PeripheralConfigData& config)
	: BlockDevice(params.NotifyClient(true).IOAreaSize(0))
{
	width_ = config.get<int>("width", 10);
	if (width_ > 100)
		throw RunTimeError("LEDSegments: Unsupported width");

	height_ = config.get<int>("height", 1);
	if (height_ > 20)
		throw RunTimeError("LEDSegments: Unsupported height");

	auto size= width_ * height_ * 4;
	Resize(size);
}

LEDSegments::~LEDSegments()
{}

cf::uint32 LEDSegments::GetWidth() const
{
	return width_;
}

cf::uint32 LEDSegments::GetHeight() const
{
	return height_;
}

cf::uint32 LEDSegments::GetBitsPerPixel() const
{
	throw RunTimeError("GetBitsPerPixel is not supported by LED segments");
}

cf::uint32 LEDSegments::GetScreenBaseAddress() const
{
	throw RunTimeError("GetScreenBaseAddress is not supported by LED segments");
}

DisplayDevice::NotifyType LEDSegments::ChangeNotification() const
{ return DisplayDevice::NotifyType::EventBased; } // this display sends notifications when its memory changes
