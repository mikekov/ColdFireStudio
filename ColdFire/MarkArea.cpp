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
	ASSERT(start >= 0);		// przed 'SetEnd' musi by� wywo�ane 'SetStart'
	ASSERT(end >= start);		// b��dne kra�ce przedzia��w
	if (end < start)		// b��dne kra�ce przedzia��w?
		return;

	Pair pair= { start, end };
	for (size_t i= 0; i < arr.size(); ++i)
	{
		if (arr[i].a > end || arr[i].b < start)
			continue;			// przedzia�y roz��czne
		if (arr[i].a <= start)
			if (arr[i].b >= end)
				return;			// nowa para mie�ci si� w przedziale
			else
			{
				arr[i].b = end;		// przesuni�cie ko�ca przedzia�u
				return;
			}
		else if (arr[i].b <= end)
		{
			arr[i].a = start;		// przesuni�cie pocz�tku przedzia�u
			return;
		}
		else
		{
			arr[i].a = start;		// poszerzenie ca�ego przedzia�u
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
