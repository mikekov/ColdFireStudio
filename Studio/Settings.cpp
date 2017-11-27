/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "Settings.h"
#include "ProtectedCall.h"
#include "Utilities.h"
#include "Utf8.h"
#include "TypeTranslators.h"
#include "resource.h"

namespace {
	const wchar_t SETTINGS[]= L"Studio.ini";
}


struct Settings::Impl
{
	boost::property_tree::ptree template_;
	boost::property_tree::ptree tree_;
	std::wstring file_path_;
	boost::signals2::signal<void (Settings&)> change_;
};


void Settings::Load()
{
	// load program settings
	ProtectedCallMsg([&](CString& msg)
	{
		std::ifstream f(impl_->file_path_);
		if (f.good())
			boost::property_tree::read_info(f, impl_->tree_);
		else
			msg = impl_->file_path_.c_str();

		// apply defaults to * missing nodes * only, to make settings complete;
		// if loading fails settings are left intact, with all defaults already in place
		ApplyDefaults(SettingsTemplate(impl_->template_), std::string(), false);

	}, L"Cannot load program settings. Defaults will be used");
}


void Settings::Save()
{
	ProtectedCall([&]()
	{
		std::ofstream f(impl_->file_path_);
		boost::property_tree::write_info(f, impl_->tree_);
	}, L"Cannot write program settings.");
}


void Settings::Save(const wchar_t* path)
{
	std::ofstream f(path);
	boost::property_tree::write_info(f, impl_->tree_);
}


Settings::Settings(int template_id) : impl_(new Impl), SettingsSection(nullptr)
{
	SettingsSection::tree_ = &impl_->tree_;

	std::wstring dir= GetSettingsFolder();
	impl_->file_path_ = dir + SETTINGS;
	ApplyDefaults(SettingsTemplate(impl_->template_, template_id), std::string(), true);
}


Settings::~Settings()
{}


Settings::Settings(const Settings& s) : impl_(new Impl), SettingsSection(nullptr)
{
	SettingsSection::tree_ = &impl_->tree_;

	*this = s;
}


Settings& Settings::operator = (const Settings& s)
{
	impl_->template_ = s.impl_->template_;
	impl_->tree_ = s.impl_->tree_;
	impl_->file_path_ = s.impl_->file_path_;
	//todo: decide whether to copy listeners or not
	//impl_->change_ := s.impl_->change_;

	return *this;
}


boost::signals2::connection Settings::Changed(const std::function<void (SettingsSection& settings)>& fn)
{
	return impl_->change_.connect(fn);
}


void Settings::FireChange()
{
	impl_->change_(*this);
}


namespace {

void LoadTemplate(int settings_attributes_id, boost::property_tree::ptree& attributes)
{
	std::string settings_template;

	auto res_inst= AfxGetResourceHandle();
	if (HRSRC res= ::FindResource(res_inst, MAKEINTRESOURCE(settings_attributes_id), L"CONFIG"))
	{
		HGLOBAL mem= ::LoadResource(res_inst, res);
		void* data= ::LockResource(mem);
		size_t length= ::SizeofResource(res_inst, res);

		settings_template.assign(static_cast<char*>(data), length);
	}
	else
		throw std::runtime_error("missing settings template resource");

	std::istringstream ist(settings_template);
	boost::property_tree::read_info(ist, attributes);
}

}


SettingsTemplate::SettingsTemplate(const boost::property_tree::ptree& subtree)
	: tree_(subtree)
{}


SettingsTemplate::SettingsTemplate(boost::property_tree::ptree& tree, int settings_template_id)
	: tree_(tree)
{
	LoadTemplate(settings_template_id, tree);
}


Settings& AppSettings()
{
	static Settings s(IDR_SETTINGS);
	return s;
}


SettingsTemplate Settings::Template()
{
	return impl_->template_;
}


SettingsSection Settings::Tree(const char* path)
{
	auto& tree= impl_->tree_.get_child(path);	// extract branch in a tree
	return SettingsSection(&tree);
}


SettingsTemplate::iterator::iterator(boost::property_tree::ptree::const_iterator beg, boost::property_tree::ptree::const_iterator end)
	: it(beg), end(end), el(end)
{
	find_next_item();
}


SettingsTemplate::iterator& SettingsTemplate::iterator::operator ++ ()
{
	++it;
	find_next_item();
	return *this;
}


void SettingsTemplate::iterator::find_next_item()
{
	for (;;)
	{
		if (it == end)
			return;

		auto& type= it->first;
		auto& item= it->second;

		// skip item attributes
		if (type == "collapsed" || type == "name" || type == "description" || type == "default" || type == "min" || type == "max")
		{
			++it;
			continue;
		}

		// we have a real item here, remember it for reference
		el = SettingsTemplate::element(it);

		break;
	}
}


SettingsTemplate::iterator::iterator(const iterator& it)
	: it(it.it), end(it.end), el(it.el)
{}


SettingsTemplate::iterator SettingsTemplate::begin() const
{
	return iterator(tree_.begin(), tree_.end());
}


SettingsTemplate::iterator SettingsTemplate::end() const
{
	return iterator(tree_.end(), tree_.end());
}


bool SettingsTemplate::iterator::operator == (const iterator& it) const
{
	return it.it == this->it;
}


bool SettingsTemplate::iterator::operator != (const iterator& it) const
{
	return it.it != this->it;
}


const SettingsTemplate::element& SettingsTemplate::iterator::operator * () const
{
	return el;
}


SettingsTemplate SettingsTemplate::element::children() const
{
	return SettingsTemplate(it->second);
}

SettingsTemplate::element::element(boost::property_tree::ptree::const_iterator it) : it(it)
{}


void Settings::ApplyDefaults()
{
	ApplyDefaults(SettingsTemplate(impl_->template_), std::string(), true);
}


void Settings::ApplyDefaults(const SettingsTemplate& settings_template, std::string& path, bool replace)
{
	for (auto& item : settings_template)
	{
		auto cur_path= path.empty() ? item.path() : path + '.' + item.path();

		if (item.group())
		{
			ApplyDefaults(item.children(), cur_path, replace);
		}
		else
		{
			auto default_value= item.default_value();

			if (!default_value.empty())
				if (replace || !impl_->tree_.get_child_optional(cur_path))
				{
					void* v= &tree_;
					impl_->tree_.put(cur_path, default_value);
				}
		}
	}
}


SettingsSection::SettingsSection(boost::property_tree::ptree* tree)
	: tree_(tree)
{}


SettingsSection::~SettingsSection()
{}


int SettingsSection::get_int(const std::string& path) const
{
	return tree_->get<int>(path);
}


bool SettingsSection::get_bool(const std::string& path) const
{
	return tree_->get<bool>(path);
}


std::wstring SettingsSection::get_string(const std::string& path) const
{
	return tree_->get<std::wstring>(path, Utf8());
}


COLORREF SettingsSection::get_color(const std::string& path) const
{
	return tree_->get<COLORREF>(path, ColorTranslator());
}


LOGFONT SettingsSection::get_font(const std::string& path) const
{
	return tree_->get<LOGFONT>(path, LogFontTranslator());
}


SettingsSection SettingsSection::section(const std::string& path)
{
	auto& tree= tree_->get_child(path);	// extract branch in a tree
	return SettingsSection(&tree);
}


void SettingsSection::set_int(const std::string& path, int val)
{
	tree_->put(path, val);
}


void SettingsSection::set_bool(const std::string& path, bool val)
{
	tree_->put(path, val);
}


void SettingsSection::set_string(const std::string& path, const std::wstring& val)
{
	tree_->put(path, val, Utf8());
}


void SettingsSection::set_font(const std::string& path, const LOGFONT& lf)
{
	tree_->put(path, lf, LogFontTranslator());
}


void SettingsSection::set_color(const std::string& path, COLORREF c)
{
	tree_->put(path, c, ColorTranslator());
}


boost::property_tree::ptree::const_iterator SettingsSection::begin() const
{
	return tree_->begin();
}

boost::property_tree::ptree::const_iterator SettingsSection::end() const
{
	return tree_->end();
}