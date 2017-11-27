#pragma once

// UTF8 translator for boost property tree

std::string to_utf8(const wchar_t* str, size_t length);
std::string to_utf8(const std::wstring& str);

std::wstring from_utf8(const char* str, size_t length);
std::wstring from_utf8(const std::string& str);


struct Utf8
{
	typedef std::string  internal_type;
	typedef std::wstring external_type;

	std::string put_value(const std::wstring& s) const;
	std::string put_value(BSTR bstr) const;

	std::wstring get_value(const std::string& str) const;
	std::wstring get_value(const char* str) const;
};
