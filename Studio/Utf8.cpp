#include "pch.h"
#include "Utf8.h"


std::string Utf8::put_value(const std::wstring& s) const
{
	return to_utf8(s.c_str());
}

std::string Utf8::put_value(BSTR bstr) const
{
	return to_utf8(bstr);
}


std::wstring Utf8::get_value(const std::string& str) const
{
	return from_utf8(str.c_str());
}

std::wstring Utf8::get_value(const char* str) const
{
	return from_utf8(str);
}


std::wstring from_utf8(const char* str, size_t str_len)
{
	auto length= ::MultiByteToWideChar(CP_UTF8, 0, str, static_cast<int>(str_len), nullptr, 0);

	if (length == 0)
		return std::wstring();

	std::vector<wchar_t> buf(length + 1, 0);

	auto written= ::MultiByteToWideChar(CP_UTF8, 0, str, static_cast<int>(str_len), buf.data(), length);
	if (written > 0 && static_cast<size_t>(written) < buf.size())
		buf[written] = 0;

	return std::wstring(buf.data());
}


std::wstring from_utf8(const std::string& str)
{
	return from_utf8(str.c_str(), str.length());
}


std::string to_utf8(const wchar_t* str, size_t str_len)
{
	auto length= ::WideCharToMultiByte(CP_UTF8, 0, str, static_cast<int>(str_len), nullptr, 0, nullptr, nullptr);

	if (length == 0)
		return std::string();

	std::vector<char> buf(length + 1, 0);

	int written= ::WideCharToMultiByte(CP_UTF8, 0, str, static_cast<int>(str_len), buf.data(), length, nullptr, nullptr);
	if (written > 0 && static_cast<size_t>(written) < buf.size())
		buf[written] = 0;

	return std::string(buf.data());
}


std::string to_utf8(const std::wstring& str)
{
	return to_utf8(str.c_str(), str.length());
}
