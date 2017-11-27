/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#pragma once
#include "BasicTypes.h"
#include "PeripheralDevice.h"
#include <boost/property_tree/ptree.hpp>

// Implementation details for CF peripherals


class Context;


struct IOAreaRange
{
	IOAreaRange(uint32 from, uint32 to) : base(from), end(to)
	{}

	uint32 base;
	uint32 end;	// non inclusive
};


struct PParam	// peripheral parameters
{
	PParam() : io_area_offset_(0), io_area_size_(0), interrupt_source_(0), trace_(false), notify_(false)
	{}

	PParam& IOAreaOffset(uint16 io_area_offset)			{ io_area_offset_ = io_area_offset; return *this; }
	PParam&	IOAreaSize(uint16 io_area_size)				{ io_area_size_ = io_area_size; return *this; }
	PParam& InterruptSource(uint16 interrupt_source)	{ interrupt_source_ = interrupt_source; return *this; }
	PParam& Category(const char* category)				{ category_ = category; return *this; }
	PParam& Version(const char* version)				{ version_ = version; return *this; }
	PParam& Trace(bool enable)							{ trace_ = enable; return *this; }
	PParam& NotifyClient(bool enable)					{ notify_ = enable; return *this; }

	uint16 io_area_offset_;
	uint16 io_area_size_;
	uint16 interrupt_source_;
	bool trace_;
	bool notify_;
	std::string category_;
	std::string version_;
};


typedef boost::property_tree::ptree PeripheralConfigData;


class Peripheral : public PeripheralDevice, boost::noncopyable
{
public:
	Peripheral(const PParam& params);
	virtual ~Peripheral();

	// interface exposed to simulator
	void DoUpdate(Context& ctx);
	void DoReset(Context& ctx);
	uint32 DoRead(Context& ctx, uint32 offset, int access_size);
	void DoWrite(Context& ctx, uint32 offset, int access_size, uint32 value);

	// report where device registers are mapped in a 1 KB peripherals space
	// offsets from 0 are expected, from first to the last valid entry;
	// called by simulator during init to build a map of peripherals
	IOAreaRange GetIOArea() const;

	// interrupt source this device is configured as
	int InterruptSource() const;

	// if true, device reads/writes will be reported to the simulator/client
	bool NotifyClient() const;

	// reporting methods
	const std::string& Category() const;
	const std::string& Version() const;
	cf::uint32 IOAreaStart() const;
	cf::uint32 IOAreaSize() const;

protected:
	void SetIOAreaSize(cf::uint32 size);

	// implementation details
private:
	// called during simulator run after executing single opcode
	virtual void Update(Context& ctx) = 0;

	// resetting device
	virtual void Reset() = 0;

	// read from device; access_size is 1, 2, or 4
	virtual uint32 Read(uint32 offset, int access_size) = 0;

	// write to device; access_size is 1, 2, or 4
	virtual void Write(Context& ctx, uint32 offset, int access_size, uint32 value) = 0;

	// up to the device to implement if needed; simulator may use it to query state of the device
	virtual cf::uint8 ReadBufferByte(cf::uint32 index);
	virtual cf::uint32 ReadBufferLongWord(cf::uint32 index);

	// location of device IO area comes from configuration, and its size is typically device specific;
	// each device has to provide size of this window
	PParam params_;
};
