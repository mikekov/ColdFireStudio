/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#pragma once
#include <iosfwd>
#include <iomanip>

// Struct for describing colors to be persisted in program settings

struct Color
{
	Color() : color(0)
	{}

	Color(BYTE r, BYTE g, BYTE b) : color(RGB(r,g,b))
	{}

	operator COLORREF () { return color; }

	COLORREF color;
};


template<class charT, class Traits> std::basic_istream<charT, Traits>&
	operator >> (std::basic_istream<charT, Traits>& is, Color& c)
{
	is >> std::hex >> c.color;
	return is;
}


template<class charT, class Traits> std::basic_ostream<charT, Traits>&
	operator << (std::basic_ostream<charT, Traits >& os, const Color& c)
{
	os << std::setw(6) << std::setfill('0') << std::hex << c.color;
	return os;
}
