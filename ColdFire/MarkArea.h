/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#pragma once
#ifndef _mark_h_
#define _mark_h_

//#include "TypeDefs.h"
namespace masm {


class MarkArea
{
	struct Pair
	{ int a,b; };
	int start;
//	uint32 n;
	std::vector<Pair> arr;

public:

	MarkArea() : start(-1) //, n(0)
	{}

	void SetStart(int s)
	{ ASSERT(s>=0); start = s; }

	void SetEnd(int end);

	bool IsStartSet() const
	{ return start != -1; }

	size_t GetSize() const
	{ return arr.size(); }

	bool GetPartition(size_t no, int &a, int &b);

	void Clear()
	{
		start = -1;
//		n = 0;
		arr.clear();
	}
};

} // namespace

#endif
