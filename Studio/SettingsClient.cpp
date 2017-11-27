#include "pch.h"
#include "SettingsClient.h"
#include "Settings.h"
#include "ProtectedCall.h"


SettingsClient::SettingsClient(const wchar_t* client_name)
{
	err_msg_ = "Error applying settings in ";
	err_msg_ += client_name;

	auto& s= AppSettings();
	settings_chg_ = s.Changed(std::bind(&SettingsClient::SettingsChanged, this, std::placeholders::_1));
}


SettingsClient::~SettingsClient()
{}


// calls ApplySettings when settings change; catches exceptions
void SettingsClient::SettingsChanged(SettingsSection& settings)
{
	ProtectedCall([&]
	{
		ApplySettings(settings);
	}, err_msg_);
}


void SettingsClient::CallApplySettings()
{
	ApplySettings(AppSettings());
}
