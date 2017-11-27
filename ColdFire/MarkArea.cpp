/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "MarkArea.h"

namespace masm {


void MarkArea::SetEnd(int end)
{
	ASSERT(start >= 0);		// przed 'SetEnd' musi byæ wywo³ane 'SetStart'
	ASSERT(end >= start);		// b³êdne krañce przedzia³ów
	if (end < start)		// b³êdne krañce przedzia³ów?
		return;

	Pair pair= { start, end };
	for (size_t i= 0; i < arr.size(); ++i)
	{
		if (arr[i].a > end || arr[i].b < start)
			continue;			// przedzia³y roz³¹czne
		if (arr[i].a <= start)
			if (arr[i].b >= end)
				return;			// nowa para mieœci siê w przedziale
			else
			{
				arr[i].b = end;		// przesuniêcie koñca przedzia³u
				return;
			}
		else if (arr[i].b <= end)
		{
			arr[i].a = start;		// przesuniêcie pocz¹tku przedzia³u
			return;
		}
		else
		{
			arr[i].a = start;		// poszerzenie ca³ego przedzia³u
			arr[i].b = end;
			return;
		}
	}
//	if (arr.size() < n)
//		arr.resize(n + 1);
	arr.push_back(pair);
//	n++;
}


bool MarkArea::GetPartition(size_t no, int &a, int &b)
{
	if (no >= arr.size())
		return false;
	a = arr[no].a;
	b = arr[no].b;
	return true;
}

} // namespace
