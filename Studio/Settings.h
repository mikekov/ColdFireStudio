#pragma once
#include "Color.h"

// Program settings
class SettingsTemplate;
class SettingsSection;

// helper class to get/set settings from a section of a property tree

class SettingsSection
{
public:
	SettingsSection(boost::property_tree::ptree* tree);

	// getters
	int get_int(const std::string& path) const;
	bool get_bool(const std::string& path) const;
	std::wstring get_string(const std::string& path) const;
	COLORREF get_color(const std::string& path) const;
	LOGFONT get_font(const std::string& path) const;

	// setters
	void set_int(const std::string& path, int val);
	void set_bool(const std::string& path, bool val);
	void set_string(const std::string& path, const std::wstring& val);
	void set_font(const std::string& path, const LOGFONT& lf);
	void set_color(const std::string& path, COLORREF c);

	//todo: revise this interface; maybe boost::variant would be appropriate
	// templated get/set didn't work well with COLORREF being just a ulong typedef

	// get another (sub)section
	SettingsSection section(const std::string& path);

	virtual ~SettingsSection();

	boost::property_tree::ptree::const_iterator begin() const;
	boost::property_tree::ptree::const_iterator end() const;

private:
	friend class Settings;
	boost::property_tree::ptree* tree_;	// we don't own the tree
};


// Program settings are remembered in an instance of this singleton class

class Settings : public SettingsSection
{
public:
	Settings(const Settings& s);	// make a copy

	// load/save settings from/to 'file_path_', doesn't throw
	void Load();
	void Save();

	void Save(const wchar_t* path);	// can throw

	// get settings from sub section
	SettingsSection Tree(const char* path);

	// get template for settings
	SettingsTemplate Template();

	// replace all settings withdefault values
	void ApplyDefaults();

	// register for settings changed notification
	boost::signals2::connection Changed(const std::function<void (SettingsSection& settings)>& fn);

	// fire change notification
	void FireChange();

	virtual ~Settings();

	Settings& operator = (const Settings& s);

private:
	Settings(int template_id);

	// apply default values to all nodes (replace == true), or missing nodes only (replace == false)
	// after calling this function all properties are defined and can be read
	void ApplyDefaults(const SettingsTemplate& settings_template, std::string& path, bool replace);

	struct Impl;
	std::unique_ptr<Impl> impl_;

	friend Settings& AppSettings();
};


// helper calls to iterate over settings template; used to generate UI or apply default settings

class SettingsTemplate
{
public:
	SettingsTemplate(boost::property_tree::ptree& tree, int settings_template_id);
	SettingsTemplate(const boost::property_tree::ptree& subtree);

	class iterator;
	class element;

	iterator begin() const;
	iterator end() const;

private:
	const boost::property_tree::ptree& tree_;	// we don't own the tree
};


class SettingsTemplate::element
{
public:
	element(boost::property_tree::ptree::const_iterator it);

	std::string path() const		{ return it->second.get_value<std::string>(); }
	std::string name() const		{ return it->second.get<std::string>("name"); }
	const std::string& type() const	{ return it->first; }
	std::string default_value() const	{ return it->second.get<std::string>("default", ""); }
	std::string description() const	{ return it->second.get<std::string>("description", ""); }
	bool group() const				{ return type() == "group"; }
	bool is_collapsed() const		{ auto c= it->second.get_optional<bool>("collapsed"); return c ? *c : false; }
	std::string enums() const		{ return it->second.get<std::string>("enums"); }
	template<class T>
	boost::optional<T> get(const char* tag) const	{ return it->second.get_optional<T>(tag); }

	SettingsTemplate children() const;

private:
	boost::property_tree::ptree::const_iterator it;
};


class SettingsTemplate::iterator
{
public:
	iterator(boost::property_tree::ptree::const_iterator beg, boost::property_tree::ptree::const_iterator end);
	iterator(const iterator& it);

	bool operator == (const iterator& it) const;
	bool operator != (const iterator& it) const;

	iterator& operator ++ ();

	const element& operator * () const;

private:
	void find_next_item();
	element el;
	boost::property_tree::ptree::const_iterator it;
	boost::property_tree::ptree::const_iterator end;
};


// global app settings

Settings& AppSettings();
