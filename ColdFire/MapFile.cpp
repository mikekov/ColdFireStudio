/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "MapFile.h"
#include "Exceptions.h"


const Path& MapFile::GetPath(masm::FileUID fuid) const
{
	auto it= files_.right.find(fuid);
	if (it == files_.right.end())
	{
		assert(false);	// bogus file id
		throw RunTimeError("Invalid file id in " __FUNCTION__);
	}
	return it->second;
}

namespace {
	template<class M, class T1, class T2> void insert(M& map, T1& t1, T2& t2)
	{
		map.insert(typename M::value_type(t1, t2));
	}
}

masm::FileUID MapFile::GetFileUID(const Path& path)
{
	auto it= files_.left.find(path);

	if (it == files_.left.end())
	{
		masm::FileUID fuid= static_cast<masm::FileUID>(files_.size() + 1);
		insert(files_, path, fuid);
		return fuid;
	}
	else
		return it->second;
}
