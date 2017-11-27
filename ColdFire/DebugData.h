/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#pragma once
#include "Import.h"
#include "Types.h"

namespace masm { class DebugInfo; }


class CF_DECL DebugData
{
public:
	DebugData(std::unique_ptr<masm::DebugInfo> debug);

	~DebugData();

	// find file and line corresponding to the given address (typically program counter)
	// -1 if not found
	int GetLine(cf::uint32 address, std::wstring& out_path);

	// set execution breakpoint at given location, returns false if debug doesn't have this place recorded

	// return address corresponding to a given line in a given source file
	boost::optional<cf::uint32> GetAddress(int line, const std::wstring& file);

private:
	DebugData(const DebugData&);
	DebugData& operator = (const DebugData&);

	struct Impl;
	Impl* impl_;
};
