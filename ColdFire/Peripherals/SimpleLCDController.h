/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2013 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#pragma once
#include "..\Peripheral.h"


class SimpleLCDController : public Peripheral, public DisplayDevice
{
public:
	SimpleLCDController(PParam params, PeripheralConfigData& config);
	virtual ~SimpleLCDController();

	// called during simulator run after executing single opcode
	virtual void Update(Context& ctx);

	// resetting device
	virtual void Reset();

	// read from block device; access_size is 1, 2, or 4; only 4 is legal
	virtual uint32 Read(uint32 offset, int access_size);

	// write to block device; access_size is 1, 2, or 4; only 4 is legal
	virtual void Write(Context& ctx, uint32 offset, int access_size, uint32 value);

	// display device parameters
	virtual cf::uint32 GetWidth() const;
	virtual cf::uint32 GetHeight() const;
	virtual cf::uint32 GetBitsPerPixel() const;
	virtual cf::uint32 GetScreenBaseAddress() const;
	virtual NotifyType ChangeNotification() const;

private:
	struct _dev_data;
	_dev_data* data;
};
