/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "Peripheral.h"
#include "Context.h"
#include "Exceptions.h"


Peripheral::Peripheral(const PParam& params) : params_(params)
{}


Peripheral::~Peripheral()
{}


IOAreaRange Peripheral::GetIOArea() const
{
	IOAreaRange range(params_.io_area_offset_, params_.io_area_offset_ + params_.io_area_size_);
	return range;
}


int Peripheral::InterruptSource() const
{
	return params_.interrupt_source_;
}


bool Peripheral::NotifyClient() const
{
	return params_.notify_;
}


class Trace
{
public:
	Trace(Context& ctx) : ctx_(ctx)
	{}

	Trace& operator << (int n)	// dec
	{
		char buf[64];
		_itoa_s(n, buf, 10);
		return output(buf);
	}

	Trace& operator << (uint32 n)	// hex
	{
		char buf[64];
		buf[0] = '$';
		_itoa_s(n, buf + 1, 63, 16);
		return output(buf);
	}

	Trace& operator << (char c)
	{
		char buf[]= { c, 0 };
		return output(buf);
	}

	Trace& operator << (const char* s)
	{
		return output(s);
	}

	Trace& operator << (const std::string& str)
	{
		return output(str.c_str());
	}

private:
	Trace& output(const char* s)
	{
		for (size_t i= 0; s[i] != 0; ++i)
			ctx_.SimWrite(cf::SimPort::IN_OUT, s[i]);
		return *this;
	}

	Context& ctx_;
};


void Peripheral::DoUpdate(Context& ctx)
{
	Update(ctx);
}


void Peripheral::DoReset(Context& ctx)
{
	if (params_.trace_)
		Trace(ctx) << params_.category_ << " Reset\n";

	Reset();
}


uint32 Peripheral::DoRead(Context& ctx, uint32 offset, int access_size)
{
	if (params_.trace_)
		Trace(ctx) << params_.category_ << " read." << access_size << " @ " << params_.io_area_offset_ + offset;

	auto value= Read(offset, access_size);

	if (params_.trace_)
		Trace(ctx) << " -> " << value << '\n';

	return value;
}


void Peripheral::DoWrite(Context& ctx, uint32 offset, int access_size, uint32 value)
{
	if (params_.trace_)
		Trace(ctx) << params_.category_ << " write." << access_size << " @ " << params_.io_area_offset_ + offset << ": " << value << '\n';

	Write(ctx, offset, access_size, value);
}


cf::uint8 Peripheral::ReadBufferByte(cf::uint32 index)
{
	throw RunTimeError("ReadBufferByte is not supported by this device");
}


cf::uint32 Peripheral::ReadBufferLongWord(cf::uint32 index)
{
	throw RunTimeError("ReadBufferLongWord is not supported by this device");
}


const std::string& Peripheral::Category() const
{
	return params_.category_;
}


const std::string& Peripheral::Version() const
{
	return params_.version_;
}


cf::uint32 Peripheral::IOAreaStart() const
{
	return params_.io_area_offset_;
}


cf::uint32 Peripheral::IOAreaSize() const
{
	return params_.io_area_size_;
}


void Peripheral::SetIOAreaSize(uint32 size)
{
	if (size > 0xffff)
		throw RunTimeError("SetIOAreaSize: IO area window is limited to 64 KB");

	params_.io_area_size_ = size;
}
