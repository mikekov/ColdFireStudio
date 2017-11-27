/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#pragma once
#include <iosfwd>
#include <iomanip>


template<class T>
struct HexNumber
{
	HexNumber<T>() : value(T())
	{}

	HexNumber<T>(T&& t) : value(t)
	{}

	T value;

	operator T () { return value; }
};


template<class charT, class Traits, class T> std::basic_istream<charT, Traits>&
	operator >> (std::basic_istream<charT, Traits>& is, HexNumber<T>& h)
{
	is >> std::hex >> std::showbase >> h.value;
	return is;
}


template<class charT, class Traits, class T> std::basic_ostream<charT, Traits>&
	operator << (std::basic_ostream<charT, Traits >& os, const HexNumber<T>& h)
{
	os << std::setw(8) << std::setfill('0') << std::hex << std::showbase << h.value;
	return os;
}
