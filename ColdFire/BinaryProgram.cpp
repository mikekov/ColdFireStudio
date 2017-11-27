/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "BinaryProgram.h"
#include "Exceptions.h"

namespace cf {
/*
struct Page
{
	enum { SIZE= 0x100 };
public:
	Page()
	{
		Clear();
	}

	void Clear()
	{
		std::fill(values_, values_ + SIZE, 0);
		std::fill(in_use_, in_use_ + SIZE, 0);
	}

	void Store(uint32 offset, uint8 byte)
	{
		if (offset >= SIZE)
			throw "offset out of range";
		values_[offset] = byte;
		in_use_[offset] = 1;
	}

	uint32 FindFirst() const
	{
		return find(0, true);
	}

	uint32 FindLast(uint32 offset) const
	{
		return find(offset + 1, false);
	}

	uint8 Get(uint32 offset)
	{
		if (offset >= SIZE)
			throw "offset out of range";
		if (!in_use_[offset])
			throw "no value available";

		return values_[offset];
	}

	uint32 Limit() const { return SIZE; }

private:
	uint8 values_[SIZE];
	uint8 in_use_[SIZE];

	uint32 find(uint32 offset, bool used) const
	{
		uint32 i= offset;
		if (used)
		{
			for ( ; i < SIZE; ++i)
				if (in_use_[i])
					return i;
		}
		else
		{
			for ( ; i < SIZE; ++i)
				if (!in_use_[i])
					return i;
		}
		return SIZE;
	}
};
*/

class Block
{
	uint32 base_addr_;
	std::vector<uint8> data_;

public:
	Block(uint32 base_addr= 0) { base_addr_ = base_addr; }

	uint32 Begin() const	{ return base_addr_; }
	uint32 End() const		{ return static_cast<uint32>(base_addr_ + data_.size()); }

	void Clear()			{ data_.clear(); }
	size_t Size() const		{ return data_.size(); }
	const uint8* Data() const	{ return data_.data(); }

	void Append(uint8 byte)	{ data_.push_back(byte); }
	void Append(const Block& block)	{ data_.insert(data_.end(), block.data_.begin(), block.data_.end()); }

	void Set(uint32 offset, uint8 byte)	{ data_.at(offset) = byte; }
};


struct BinaryProgram::Impl
{
	Impl()
	{
		start_addr_ = 0;
		isa_ = ISA::A;
	}

//	typedef std::map<uint32, uint8> Map;
//	Map code_;
	uint32 start_addr_;
	ISA isa_;

//	std::map<uint32, Page> mem_;
	//void Put(uint32 addr, uint8 val)
	//{
	//	uint32 page= addr >> 8;
	//	mem_[page].Store(addr - (page << 8), val);
	//}

	typedef std::map<uint32, Block> Memory;
	Memory mem_;

	void Consolidate()
	{
		if (mem_.size() < 2)
			return;

		auto it1= mem_.begin();
		auto it2= mem_.begin();
		++it2;

		for ( ; it2 != mem_.end(); ++it1, ++it2)
		{
			auto& block1= it1->second;
			auto& block2= it2->second;

			if (block1.End() == block2.Begin())
			{
				// merge blocks
				block1.Append(block2);
				block2.Clear();
			}
		}
	}

	void Put(uint32 addr, uint8 byte)
	{
		auto end= mem_.end();
		for (auto it= mem_.begin(); it != end; ++it)
			if (addr >= it->first && addr <= it->second.End())
			{
				if (addr == it->second.End())
				{
					it->second.Append(byte);
					Consolidate();
				}
				else
				{
					it->second.Set(addr - it->second.Begin(), byte);
				}
				return;
			}

		Block b(addr);
		b.Append(byte);
		mem_[addr] = b;
		Consolidate();
	}

	void Put(uint32 addr, const std::vector<uint8>& prg)
	{

	}

	//void Put(uint32 addr, uint8 val)
	//{
	//	uint32 page= addr >> 8;
	//	mem_[page].Store(addr - (page << 8), val);
	//}
};


BinaryProgram::BinaryProgram() : impl_(new Impl())
{}

BinaryProgram::~BinaryProgram()
{
	delete impl_;
}

BinaryProgram::BinaryProgram(BinaryProgram&& p)
{
	impl_ = p.impl_;
	p.impl_ = nullptr;
}


void BinaryProgram::PutByte(uint32 origin, uint8 byte)
{
//	impl_->code_[origin] = byte;
	impl_->Put(origin, byte);
}


void BinaryProgram::PutWord(uint32 origin, uint16 word)
{
	uint32 pc= origin;
	uint8 hi= word >> 8;
	uint8 lo= word & 0xff;

	//impl_->code_[pc++] = hi;
	//impl_->code_[pc] = lo;

	impl_->Put(origin, hi);
	impl_->Put(origin+1, lo);
}


void BinaryProgram::PutLongWord(uint32 origin, uint32 lword)
{
	PutWord(origin, static_cast<uint16>(lword >> 16));
	PutWord(origin + 2, static_cast<uint16>(lword & 0xffff));
}


uint32 BinaryProgram::GetProgramStart() const
{
	return impl_->start_addr_;
}


void BinaryProgram::SetProgramStart(uint32 start)
{
	impl_->start_addr_ = start;
}


BinaryProgram& BinaryProgram::operator = (const BinaryProgram& src)
{
//	impl_->code_ = src.impl_->code_;
	impl_->start_addr_ = src.impl_->start_addr_;
	impl_->isa_ = src.impl_->isa_;

	impl_->mem_ = src.impl_->mem_;

	return *this;
}


BinaryProgram& BinaryProgram::operator = (BinaryProgram&& p)
{
	std::swap(impl_, p.impl_);
	return *this;
}


bool BinaryProgram::Valid() const
{
//	return !impl_->code_.empty();
	return !impl_->mem_.empty();
}


ISA BinaryProgram::GetIsa() const
{
	return impl_->isa_;
}


void BinaryProgram::SetIsa(ISA isa)
{
	impl_->isa_ = isa;
}


uint32 BinaryProgram::FirstByteAddress() const
{
	if (impl_->mem_.empty())
		return 0;

	return impl_->mem_.begin()->first;
}

uint32 BinaryProgram::LastByteAddress() const
{
	if (impl_->mem_.empty())
		return 0;

	auto it= --impl_->mem_.end();
	return static_cast<uint32>(it->first + it->second.Size());
}

//------------------------------------------------------------------


struct BinaryProgramRange::Impl
{
	Impl(const BinaryProgram& code) : mem_(code.impl_->mem_)
	{
//		first_ = end_ = 0;
//		it_ = code.impl_->code_.begin();
//		mem_ = code.impl_->mem_;
		end_ = mem_.end();
		it_ = mem_.begin();
	}

	// this iterator is built to traverse map in steps;
	// each steps encompases all adjacent values (in address space)
	// that is iterator moves over gaps in code
	void next()
	{
		if (it_ == end_)
			return;

		++it_;
		/*
		if (it_ == code_.impl_->code_.end())
		{
			first_ = end_ = 0;
			return;
		}

		end_ = first_ = it_->first;

		while (it_ != code_.impl_->code_.end())
		{
			if (it_->first != end_)
				break;
			end_ = it_->first + 1;
			++it_;
		} */
	}

	//const BinaryProgram& code_;
	//BinaryProgram::Impl::Map::const_iterator it_;
	//uint32 first_;
	//uint32 end_;
	BinaryProgram::Impl::Memory& mem_;
	BinaryProgram::Impl::Memory::const_iterator it_;
	BinaryProgram::Impl::Memory::const_iterator end_;
};


BinaryProgramRange::BinaryProgramRange(const BinaryProgram& code) : impl_(new Impl(code))
{
//	operator++();
}


BinaryProgramRange::operator bool () const
{
	return impl_->it_ != impl_->end_;
//	return impl_->end_ != impl_->first_;
}


void BinaryProgramRange::operator ++ ()
{
	impl_->next();
}


std::vector<uint8> BinaryProgramRange::Fragment() const
{
	// read one code fragment and put it in a vector

	if (impl_->it_ == impl_->end_)
		throw RunTimeError("end of code reached in " __FUNCTION__);

	std::vector<uint8> fragment;
	//impl_->it_->second.Size();
	//fragment.reserve(impl_->end_ - impl_->first_);

	fragment.assign(impl_->it_->second.Data(), impl_->it_->second.Data() + impl_->it_->second.Size());

	//for (uint32 i= impl_->first_; i != impl_->end_; ++i)
	//	fragment.push_back(impl_->code_.impl_->code_.find(i)->second);

	return fragment;
}


std::pair<uint32, uint32> BinaryProgramRange::Range() const
{
	if (impl_->it_ == impl_->end_)
		throw RunTimeError("end of code reached in " __FUNCTION__);

	assert(impl_->it_->first == impl_->it_->second.Begin());
	return std::make_pair(impl_->it_->first, impl_->it_->second.End());

//	return std::make_pair(impl_->first_, impl_->end_);
}


uint32 BinaryProgramRange::Address() const
{
//	return impl_->first_;
	if (impl_->it_ == impl_->end_)
		throw RunTimeError("end of code reached in " __FUNCTION__);
	return impl_->it_->first;
}


//------------------------------------------------------------------


namespace
{
	const uint32 Magic= '\xcfprg';
	const uint32 Code= 'code';
	const uint32 Debug= 'dbug';
	const uint32 End= 'end.';

	void Put(std::ostream& stream, uint32 v)
	{
		stream.put(char(v >> 24));
		stream.put(char(v >> 16));
		stream.put(char(v >> 8));
		stream.put(char(v));
	}

	uint32 Get(std::istream& stream)
	{
		uint32 c1= stream.get();
		uint32 c2= stream.get();
		uint32 c3= stream.get();
		uint32 c4= stream.get();
		return (c1 << 24) | (c2 << 16) | (c3 << 8) | c4;
	}
}


CF_DECL BinaryProgram LoadBinaryProgram(const wchar_t* path)
{
	BinaryProgram code;
	std::fstream in(path, std::ios::in | std::ios::binary);
	in.exceptions(std::ios::eofbit | std::ios::failbit | std::ios::badbit);

	if (Get(in) != Magic)
		throw std::exception("Invalid binary program header");

	code.SetProgramStart(Get(in));
	code.SetIsa(static_cast<ISA>(Get(in)));
	Get(in); // reserved

	for (;;)
	{
		auto chunk= Get(in);
		if (chunk == End)
			break;
		auto address= Get(in);
		auto size= Get(in);
		if (chunk == Code)
		{
			for (uint32 i= 0; i != size; ++i)
				code.PutByte(address + i, in.get());
		}
		else
		{
			// other chunks not handled, skip
			in.seekg(size, std::ios::cur);
		}
	}

	return code;
}


// load raw binary file
CF_DECL BinaryProgram LoadBinaryCode(const wchar_t* path, ISA isa, uint32 begin)
{
	BinaryProgram code;
	std::fstream in(path, std::ios::in | std::ios::binary);
	in.exceptions(std::ios::eofbit | std::ios::failbit | std::ios::badbit);

	auto size= boost::filesystem::file_size(path);
	if (size > 0xffffffff)
		throw std::exception("Binary file too big");

	if (size > 0)
	{
		std::vector<char> buf(static_cast<uint32>(size), 0);

		in.read(buf.data(), size);

		for (uint32 i= 0; i != static_cast<uint32>(size); ++i)
			code.PutByte(begin + i, buf[i]);
	}

	code.SetIsa(isa);
	code.SetProgramStart(begin);

	return code;
}


CF_DECL void SaveBinaryCode(const wchar_t* path, const BinaryProgram& code)
{
	std::fstream out(path, std::ios::out | std::ios::binary | std::ios::trunc);
	out.exceptions(std::ios::failbit | std::ios::badbit);

	Put(out, Magic);
	Put(out, code.GetProgramStart());
	Put(out, static_cast<uint32>(code.GetIsa()));
	Put(out, -1);

	BinaryProgramRange it(code);
	while (it)
	{
		Put(out, Code);
		Put(out, it.Address());
		auto chunk= it.Fragment();
		Put(out, static_cast<uint32>(chunk.size()));
		for (auto byte : chunk)
			out.put(byte);

		++it;
	}

	Put(out, End);

	out.close();
}

} // namespace cf
