/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#pragma once
#include "Types.h"

// Peripheral device interfaces exposed to the debugger

class PeripheralDevice
{
public:
	PeripheralDevice() {}
	virtual ~PeripheralDevice() {}

	virtual cf::uint8 ReadBufferByte(cf::uint32 index) = 0;
	virtual cf::uint32 ReadBufferLongWord(cf::uint32 index) = 0;

	virtual cf::uint32 IOAreaStart() const = 0;
	virtual cf::uint32 IOAreaSize() const = 0;

	virtual const std::string& Category() const = 0;
	virtual const std::string& Version() const = 0;
};


class DisplayDevice
{
public:
	// width of display (depending on display type this could be either pixels or segments/characters)
	virtual cf::uint32 GetWidth() const = 0;
	// ditto for height
	virtual cf::uint32 GetHeight() const = 0;
	// bits per pixels used or 0 for character-based display
	virtual cf::uint32 GetBitsPerPixel() const = 0;
	// where the display memory starts if display uses external RAM
	virtual cf::uint32 GetScreenBaseAddress() const = 0;

	enum class NotifyType
	{
		EventBased,		// display will fire events when refresh is needed (E_DEVICE_IO)
		Polling			// display needs periodic polling to refresh
	};
	virtual NotifyType ChangeNotification() const = 0;
};
