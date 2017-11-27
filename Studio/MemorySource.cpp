/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "MemorySource.h"
#include "Debugger.h"


struct MemorySource::Impl
{
	Impl(Debugger* dbg) : dbg_(dbg)
	{}

	cf::uint32 base_;
	cf::uint64 size_;
	cf::Register register_;
	bool use_register_;
	Debugger* dbg_;
	cf::uint32 cur_address_;
};


MemorySource::MemorySource() : impl_(new Impl(nullptr))
{
	impl_->cur_address_ = impl_->base_ = 0;
	impl_->size_ = 0;
	impl_->register_ = cf::R_A0;
	impl_->use_register_ = false;
}


MemorySource::MemorySource(Debugger& dbg, cf::Register watch_register)
	: impl_(new Impl(&dbg))
{
	impl_->cur_address_ = impl_->base_ = 0;
	impl_->size_ = cf::uint64(1) << 32;
	impl_->register_ = watch_register;
	impl_->use_register_ = true;
}


MemorySource::MemorySource(Debugger& dbg, int memory_bank)
	: impl_(new Impl(&dbg))
{
	auto range= dbg.GetMemoryBankInfo(memory_bank);
	impl_->cur_address_ = impl_->base_ = range.Base();
	impl_->size_ = range.Size();
	impl_->register_ = cf::R_A0;
	impl_->use_register_ = false;
}


MemorySource::~MemorySource()
{}


cf::uint32 MemorySource::Base() const
{
	return impl_->base_;
}


cf::uint64 MemorySource::Size() const
{
	return impl_->size_;
}


cf::uint32 MemorySource::CurAddress() const
{
	if (impl_->use_register_)
		return impl_->dbg_->GetRegister(impl_->register_);
	else
		return impl_->cur_address_;
}


void MemorySource::SetCurAddress(cf::uint32 addr)
{
	if (addr < impl_->base_)
		addr = impl_->base_;
	else if (addr >= impl_->base_ + impl_->size_)
		addr = static_cast<cf::uint32>(impl_->base_ + impl_->size_ - 1);

	impl_->cur_address_ = addr;
}


void MemorySource::Write(const std::vector<BYTE>& m, cf::uint32 address)
{
	if (impl_->dbg_)
		impl_->dbg_->SetMemory(m, address);
}


void MemorySource::Write(const cf::uint8* m, cf::uint32 size, cf::uint32 address)
{
	if (impl_->dbg_)
		impl_->dbg_->SetMemory(m, size, address);
}


void MemorySource::Clear(cf::uint32 size, cf::uint32 address)
{
	if (impl_->dbg_)
		impl_->dbg_->SetMemory(nullptr, size, address);
}


bool MemorySource::ReadOnly() const
{
	if (impl_->dbg_ == nullptr)
		return true;

	switch (impl_->dbg_->GetStatus())
	{
	case SIM_IS_RUNNING:
	case SIM_FINISHED:
		return true;

	default:
		return false;
	}
}
