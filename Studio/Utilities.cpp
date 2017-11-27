/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2014 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "Utilities.h"
#include <boost/filesystem/fstream.hpp>


Path GetApplicationFolder()
{
	TCHAR path[MAX_PATH];
	path[0] = 0;
	::GetModuleFileName(nullptr, path, MAX_PATH);
	wchar_t* p= _tcsrchr(path, L'\\');
	if (p)
		p[0] = 0;
	return Path(path);
}


Path FindFolder(Path app_path, const char* dir)
{
	for (int level= 0; level < 3; ++level)
	{
		Path path= app_path;
		for (int i= 0; i < level; ++i)
			path = path.parent_path();

		if (boost::filesystem::exists(path / dir))
			return path / dir;
	}

	return app_path / dir;
}


Path GetConfigurationPath()
{
	static Path cfg;
	if (cfg.empty())
		cfg = FindFolder(GetApplicationFolder(), "Config");

	return cfg;
}


Path GetMonitorPath()
{
	static Path mon;
	if (mon.empty())
		mon = FindFolder(GetApplicationFolder(), "Monitor");

	return mon;
}


Path GetTemplatesPath()
{
	static Path tem;
	if (tem.empty())
		tem = FindFolder(GetApplicationFolder(), "Templates");

	return tem;
}


std::wstring GetSettingsFolder()
{
	TCHAR path[MAX_PATH];
	*path = 0;
	::SHGetSpecialFolderPath(0, path, CSIDL_LOCAL_APPDATA, false);
	std::wstring dir= path;
	dir += L"\\ColdFireStudio";
	::CreateDirectory(dir.c_str(), nullptr);
	return dir + L'\\';
}


std::string ReadFileIntoString(const Path& file)
{
	auto size= boost::filesystem::file_size(file);
	std::string str;
	str.reserve(static_cast<size_t>(size));
	boost::filesystem::ifstream f(file);
	str.assign((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
	return str;
}


CSize ToLogicalPixels(CSize physical_pixels, CDC* dc/* = nullptr*/)
{
	CDC temp_dc;
	if (dc == nullptr)
	{
		temp_dc.CreateIC(_T("DISPLAY"), nullptr, nullptr, nullptr);
		dc = &temp_dc;
	}

	return CSize(MulDiv(physical_pixels.cx, dc->GetDeviceCaps(LOGPIXELSX), 96), MulDiv(physical_pixels.cy, dc->GetDeviceCaps(LOGPIXELSY), 96));
}
