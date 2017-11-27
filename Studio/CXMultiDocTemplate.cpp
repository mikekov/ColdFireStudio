/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "CXMultiDocTemplate.h"

bool CXMultiDocTemplate::registration_ext_= true;


BOOL CXMultiDocTemplate::GetDocString(CString& string, enum DocStringIndex i) const
{
	if (!CMultiDocTemplate::GetDocString(string, i))
		return false;

	if (i == filterExt)
	{
		if (registration_ext_)
			if (string.GetLength() > 4)
				string = string.Left(4);
	}

	return true;
}
