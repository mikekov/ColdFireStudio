#pragma once

// program configuration path
Path GetConfigurationPath();

// ColdFire monitor path
Path GetMonitorPath();

// directory where program settings are saved
std::wstring GetSettingsFolder();

// read entire text file into string
std::string ReadFileIntoString(const Path& file);

// source code templates path
Path GetTemplatesPath();

// folder where application resides
Path GetApplicationFolder();

// convert physical pixels to logical
CSize ToLogicalPixels(CSize physical_pixels, CDC* dc = nullptr);

// while waiting for newer boost, let's use this simplistic class to iterate directory with range-base for loop
class directory_walk
{
	Path path_;
public:
	inline directory_walk(Path p) : path_(p)
	{}

	boost::filesystem::directory_iterator begin()	{ return boost::filesystem::directory_iterator(path_); }
	boost::filesystem::directory_iterator end()		{ return boost::filesystem::directory_iterator(); }
}; 
