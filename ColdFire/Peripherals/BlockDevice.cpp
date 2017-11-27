/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include <assert.h>
#include <mutex>
#include "BlockDevice.h"
#include "../Context.h"
#include "../PeripheralRepository.h"

// do NOT register block device; it is a building block only


struct BlockDevice::_dev_data
{
	_dev_data(uint32 size) : enabled(true), mem_block_(size / sizeof(mem_block_.front()), 0)
	{}

	bool enabled;
	std::vector<cf::uint8> mem_block_;
	mutable std::mutex lock_;
};


BlockDevice::BlockDevice(PParam params) : Peripheral(params)
{
	data = new _dev_data(params.io_area_size_);
}


BlockDevice::~BlockDevice()
{
	delete data;
}


// called during simulator run after executing single opcode
void BlockDevice::Update(Context& ctx)
{
	// no-op
}

// resetting device
void BlockDevice::Reset()
{
	// clear memory block
	std::fill(data->mem_block_.begin(), data->mem_block_.end(), 0);
}


// read from block device; access_size is 1, 2, or 4
uint32 BlockDevice::Read(uint32 offset, int access_size)
{
	uint32 result= 0;

	if (data->enabled && offset + access_size <= data->mem_block_.size() && access_size > 0)
	{
		// reading by simulator's thread; no locking

		switch (access_size)
		{
		case 1:
			result = data->mem_block_[offset];
			break;

		case 2:
			result = (static_cast<uint32>(data->mem_block_[offset]) << 8) | data->mem_block_[offset + 1];
			break;

		case 4:
			result = (static_cast<uint32>(data->mem_block_[offset + 0]) << 24) |
					 (static_cast<uint32>(data->mem_block_[offset + 1]) << 16) |
					 (static_cast<uint32>(data->mem_block_[offset + 2]) << 8) |
					 data->mem_block_[offset + 3];
			break;
		}
	}

	return result;
}


// write to block device; access_size is 1, 2, or 4
void BlockDevice::Write(Context& ctx, uint32 offset, int access_size, uint32 value)
{
	if (data->enabled && offset + access_size <= data->mem_block_.size() && access_size > 0)
	{
		// writing by simulator's thread
		std::lock_guard<std::mutex> l(data->lock_);

		switch (access_size)
		{
		case 1:
			data->mem_block_[offset] = uint8(value);
			break;

		case 2:
			data->mem_block_[offset + 0] = static_cast<uint8>(value >> 8);
			data->mem_block_[offset + 1] = static_cast<uint8>(value);
			break;

		case 4:
			data->mem_block_[offset + 0] = static_cast<uint8>(value >> 24);
			data->mem_block_[offset + 1] = static_cast<uint8>(value >> 16);
			data->mem_block_[offset + 2] = static_cast<uint8>(value >> 8);
			data->mem_block_[offset + 3] = static_cast<uint8>(value);
			break;
		}
	}
}


uint8 BlockDevice::ReadBufferByte(uint32 index)
{
	if (index < data->mem_block_.size())
	{
		// reading from the main thread
		std::lock_guard<std::mutex> l(data->lock_);
		return data->mem_block_[index];
	}

	return 0;
}


uint32 BlockDevice::ReadBufferLongWord(uint32 index)
{
	const auto size= sizeof(uint32);
	index *= size;

	if (index < data->mem_block_.size())
	{
		// reading from the main thread
		std::lock_guard<std::mutex> l(data->lock_);
		return Read(index, size);
	}

	return 0;
}


void BlockDevice::Resize(cf::uint32 size)
{
	data->mem_block_.resize(size, 0);

	SetIOAreaSize(size);
}
