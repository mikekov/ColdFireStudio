#include "pch.h"
#include "TypeTranslators.h"
#include "Utf8.h"
#include "FormatNums.h"


namespace {
	// TODO: find better character that's unlikely to be part of a font name
	const char DELIMITER= '\'';
}

std::string LogFontTranslator::put_value(const LOGFONT& lf) const
{
	std::ostringstream ost;
	ost << DELIMITER << to_utf8(lf.lfFaceName) << DELIMITER << ' ' << lf.lfHeight << ' ' << lf.lfWeight << ' ' << (lf.lfItalic ? 1 : 0);
	return ost.str();
}


LOGFONT LogFontTranslator::get_value(const std::string& str) const
{
	LOGFONT lf;
	memset(&lf, 0, sizeof(lf));

	// some defaults
	HFONT hfont= static_cast<HFONT>(::GetStockObject(DEFAULT_GUI_FONT));
	::GetObject(hfont, sizeof(lf), &lf);

	auto font= from_utf8(str);

	auto pos= font.rfind(DELIMITER);
	if (pos != font.npos && font[0] == DELIMITER)
	{
		wcscpy_s(lf.lfFaceName, font.substr(1, pos - 1).c_str());

		wchar_t* end= nullptr;
		lf.lfHeight = wcstol(font.c_str() + pos + 1, &end, 10);
		lf.lfWidth = 0;
		if (end && *end)
			lf.lfWeight = wcstol(end + 1, &end, 10);
		if (end && *end)
			lf.lfItalic = static_cast<BYTE>(wcstol(end + 1, &end, 10));
	}
	else
	{
		// decide whether to throw here or not
	}

	return lf;
}


std::string ColorTranslator::put_value(COLORREF c) const
{
	std::ostringstream ost;
	// reverse RGB values
	int t= int(GetRValue(c)) << 16 | int(GetGValue(c)) << 8 | int(GetBValue(c)) | (c & 0xff000000);
	ost << std::setw(6) << std::setfill('0') << std::hex << t;
	return ost.str();
}


COLORREF ColorTranslator::get_value(const std::string& str) const
{
	std::istringstream is(str);
	int t= 0;
	is >> std::hex >> t;
	return RGB(t >> 16, t >> 8, t) | (t & 0xff000000);
}
