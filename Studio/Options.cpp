/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

// Options.cpp : Program options
// This is UI presenting all program options. To show them in a property grid template file is used.
// Settings.ini is inside program resources and it defines all available options, their types, and defaults.
// This approach results in an easy to maintain UI that is not very user friendly unfortunatelly.

#include "pch.h"
#include "resource.h"
#include "Options.h"
#include "ProtectedCall.h"
#include "Settings.h"
#include "Utf8.h"
#include "TypeTranslators.h"
#include "Utilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


OptionsDlg::OptionsDlg(CWnd* parent) : CDialog(IDD, parent)
{}

OptionsDlg::~OptionsDlg()
{}

BEGIN_MESSAGE_MAP(OptionsDlg, CDialog)
	ON_BN_CLICKED(ID_DEFAULTS, &OptionsDlg::OnResetToDefaults)
	ON_BN_CLICKED(ID_APPLY, &OptionsDlg::OnApply)
END_MESSAGE_MAP()

// modify font display
class CMFCPropertyGridFontPropertyX : public CMFCPropertyGridFontProperty
{
public:
	CMFCPropertyGridFontPropertyX(const CString& name, LOGFONT& lf, DWORD flags) : CMFCPropertyGridFontProperty(name, lf, flags)
	{}

	virtual CString FormatProperty()
	{
		CWindowDC dc(m_pWndList);

		int log_y= dc.GetDeviceCaps(LOGPIXELSY);
		if (log_y != 0)
		{
			CString str;
			str.Format(_T("%s, %i pt"), m_lf.lfFaceName, MulDiv(72, -m_lf.lfHeight, log_y));
			return str;
		}
		else
			return m_lf.lfFaceName;
	}
};


void OptionsDlg::DoDataExchange(CDataExchange* dx)
{
	CDialog::DoDataExchange(dx);
	DDX_Control(dx, IDC_GRID, properties_);
}

// parse settings template and build property grid items
static void AddProperties(CMFCPropertyGridCtrl& ctrl, CMFCPropertyGridProperty* group, const SettingsTemplate& attribs, const std::string& path, OptionsDlg::Map& map)
{
	for (auto item : attribs)
	{
		// items have mandatory identifier; it will be used in settings file as a path/key
		auto id= item.path();
		// mandatory name (shown in the options UI)
		CString name(item.name().c_str());
		// nice to have description (also shown in the UI)
		CString descr(item.description().c_str());

		// construct path used by settings file
		auto cur_path= path.empty() ? id : path + '.' + id;

		// group of items
		if (item.group())
		{
			std::unique_ptr<CMFCPropertyGridProperty> subgroup(new CMFCPropertyGridProperty(name));

			subgroup->SetDescription(descr);

			AddProperties(ctrl, subgroup.get(), item.children(), cur_path, map);

			subgroup->Expand(!item.is_collapsed());

			if (group)
				group->AddSubItem(subgroup.get());
			else
				ctrl.AddProperty(subgroup.get());

			subgroup.release();

			continue;
		}

		// here group item expected

		std::unique_ptr<CMFCPropertyGridProperty> subitem;

		if (item.type() == "bool")
		{
			COleVariant value(short(VARIANT_FALSE), VT_BOOL);
			subitem.reset(new CMFCPropertyGridProperty(name, value));
		}
		else if (item.type() == "int")
		{
			COleVariant value(0L, VT_I4);
			subitem.reset(new CMFCPropertyGridProperty(name, value));

			auto min= item.get<int>("min");
			auto max= item.get<int>("max");
			if (min && max)
				subitem->EnableSpinControl(true, *min, *max);
		}
		else if (item.type() == "string")
		{
			COleVariant value(L"");
			subitem.reset(new CMFCPropertyGridProperty(name, value));
		}
		else if (item.type() == "color")
		{
			auto value= RGB(0,0,0);
			auto* color= new CMFCPropertyGridColorProperty(name, value);//, &palette);
			subitem.reset(color);
			color->EnableOtherButton(L"Other...");
//todo:	color->EnableAutomaticButton(_T("Default"), ::GetSysColor(COLOR_3DFACE));
		}
		else if (item.type() == "font")
		{
			LOGFONT lf;
			::GetObject(GetStockObject(DEFAULT_GUI_FONT), sizeof(lf), &lf);
			subitem.reset(new CMFCPropertyGridFontPropertyX(name, lf, CF_SCREENFONTS));
		}
		else if (item.type() == "enum")
		{
			CMFCPropertyGridProperty* prop= new CMFCPropertyGridProperty(name, L"");
			subitem.reset(prop);

			auto e= item.enums();
			if (e == "%config_files%")
			{
				// scan config folder for all *.ini files
				Path cfg= GetConfigurationPath();
				for (auto& d : directory_walk(cfg))
				{
					auto& file= d.path();

					if (boost::filesystem::is_regular_file(file) && file.extension() == ".ini")
						prop->AddOption(file.filename().c_str());
				}
			}
			else
			{
				CString enums(e.c_str());
				for (int i= 0; i < 9999; ++i)
				{
					CString enum_item;
					if (!AfxExtractSubString(enum_item, enums, i, L'|'))
						break;
					prop->AddOption(enum_item);
				}
			}
			subitem->AllowEdit(false);
		}
		else
			throw std::exception("unsupported type in " __FUNCTION__);

		subitem->SetDescription(descr);

		map[subitem.get()] = cur_path;

		if (group)
			group->AddSubItem(subitem.get());
		else
			ctrl.AddProperty(subitem.get());

		subitem.release();
	}

}


BOOL OptionsDlg::OnInitDialog()
{
	auto ok= ProtectedCall([&]
	{
		CDialog::OnInitDialog();

		WINDOWPLACEMENT wp;
		properties_.GetWindowPlacement(&wp);
		auto style= properties_.GetStyle();
		properties_.DestroyWindow();
		// recreate; it will use correct window class
		properties_.Create(style, wp.rcNormalPosition, this, IDC_GRID);

		properties_.SetVSDotNetLook(false);
		properties_.SetAlphabeticMode(false);

		// read template file describing all application settings
		auto& settings_template= AppSettings().Template();

		// build properties UI
		AddProperties(properties_, nullptr, settings_template, "", map_);

		auto bgnd= ::GetSysColor(COLOR_WINDOW);
		auto text= ::GetSysColor(COLOR_BTNTEXT);
		auto gray= ::GetSysColor(COLOR_3DFACE);
		properties_.SetCustomColors(bgnd, text, bgnd, text, gray, text, gray);

		auto& font= properties_.GetBoldFont();
		font.DeleteObject();
		LOGFONT lf;
		GetFont()->GetLogFont(&lf);
		font.CreateFontIndirect(&lf);

		properties_.SetBoolLabels(L"Yes", L"No");

		SettingsToUI(AppSettings());

	}, L"Options dialog init failed");

	if (!ok)
		EndDialog(IDCANCEL);

	return true;
}

// transfer data from application settings into property grid items
void OptionsDlg::SettingsToUI(const SettingsSection& settings)
{
	for (auto it : map_)
	{
		auto prop= it.first;
		auto& v= prop->GetValue();

		if (auto p= dynamic_cast<CMFCPropertyGridColorProperty*>(prop))
			p->SetColor(settings.get_color(it.second));
		else if (auto p= dynamic_cast<CMFCPropertyGridFontProperty*>(prop))
		{
			LOGFONT lf= settings.get_font(it.second);
			*p->GetLogFont() = lf;
			p->Redraw();
		}
		else
		{
			COleVariant new_val;

			switch (v.vt)
			{
			case VT_BOOL:
				new_val = COleVariant(short(settings.get_bool(it.second) ? VARIANT_TRUE : VARIANT_FALSE), VT_BOOL);
				break;

			case VT_I4:
				new_val = COleVariant(long(settings.get_int(it.second)));
				break;

			case VT_BSTR:
				new_val = COleVariant(settings.get_string(it.second).c_str());
				break;

			default:
				//todo if needed
				ASSERT(false);
				break;
			}

			if (new_val.vt != VT_EMPTY)
			{
				prop->SetOriginalValue(new_val);
				prop->SetValue(new_val);
			}
		}
	}
}


void OptionsDlg::OnOK()
{
	auto ok= ProtectedCall([&]
	{
		Apply();
	}, "Error updating settings");

	if (ok)
		CDialog::OnOK();
}


void OptionsDlg::OnResetToDefaults()
{
	ProtectedCall([&]
	{
		auto settings= AppSettings();
		settings.ApplyDefaults();
		SettingsToUI(settings);
	}, "Error applying default settings");
}


// read all grid property values from UI and pass them to application settings
void OptionsDlg::Apply()
{
	//todo: consider changing copy first
	auto& app_settings= AppSettings();
	for (auto it : map_)
	{
		auto prop= it.first;

		if (auto p= dynamic_cast<CMFCPropertyGridColorProperty*>(prop))
			app_settings.set_color(it.second, p->GetColor());
		else if (auto* p= dynamic_cast<CMFCPropertyGridFontProperty*>(prop))
			app_settings.set_font(it.second, *p->GetLogFont());
		else
		{
			auto& v= prop->GetValue();
			switch (v.vt)
			{
			case VT_BOOL:
				app_settings.set_bool(it.second, !!v.boolVal);
				break;

			case VT_I4:
				app_settings.set_int(it.second, v.intVal);
				break;

			case VT_BSTR:
				app_settings.set_string(it.second, v.bstrVal);
				break;

			default:
				//todo if needed
				ASSERT(false);
				break;
			}
		}
	}

	app_settings.FireChange();
}


void OptionsDlg::OnApply()
{
	ProtectedCall([&]
	{
		Apply();
	}, "Error applying settings");
}
