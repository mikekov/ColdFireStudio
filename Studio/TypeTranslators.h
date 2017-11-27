#pragma once
#include "FormatNums.h"
#include "../ColdFire/Isa.h"

// type translators for boost property tree serialization

struct LogFontTranslator
{
	typedef std::string internal_type;
	typedef LOGFONT external_type;

	std::string put_value(const LOGFONT& lf) const;
	LOGFONT get_value(const std::string& str) const;
};


struct ColorTranslator
{
	typedef std::string internal_type;
	typedef COLORREF external_type;

	std::string put_value(COLORREF c) const;
	COLORREF get_value(const std::string& str) const;
};


//ISA StringToISA(const std::wstring& isa);
//ISA StringToISA(const std::string& isa);
