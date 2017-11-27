/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#pragma once
#include "BlockDevice.h"


class LEDSegments : public BlockDevice, public DisplayDevice
{
public:
	LEDSegments(PParam params, PeripheralConfigData& config);
	virtual ~LEDSegments();

	virtual cf::uint32 GetWidth() const;
	virtual cf::uint32 GetHeight() const;
	virtual cf::uint32 GetBitsPerPixel() const;
	virtual cf::uint32 GetScreenBaseAddress() const;
	virtual NotifyType ChangeNotification() const;

private:
	cf::uint32 width_;
	cf::uint32 height_;
};
