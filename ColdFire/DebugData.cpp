/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "DebugData.h"
#include "CFAsm.h"

struct DebugData::Impl
{
	std::unique_ptr<masm::DebugInfo> debug_;
};


DebugData::DebugData(std::unique_ptr<masm::DebugInfo> debug) : impl_(new Impl())
{
	impl_->debug_ = std::move(debug);
}


DebugData::~DebugData()
{
	delete impl_;
}


int DebugData::GetLine(cf::uint32 address, std::wstring& out_path)
{
	masm::DebugLine dbg;
	impl_->debug_->GetLine(dbg, address);
	if (dbg.flags == masm::DBG_EMPTY)
		return -1;

	out_path = impl_->debug_->GetFilePath(dbg.line.file).wstring();

	return dbg.line.ln;
}



boost::optional<cf::uint32> DebugData::GetAddress(int line, const std::wstring& file)
{
	masm::DebugLine dl;
	masm::FileUID fid= impl_->debug_->GetFileUID(file);
	boost::optional<cf::uint32> ret;

	if (impl_->debug_->GetAddress(dl, line, fid))
		ret = dl.addr;

	return ret;
}
