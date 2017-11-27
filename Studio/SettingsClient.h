// SettingsClient - helper class that registers for app settings and fires change notificaiton as needed

#pragma once
class SettingsSection;

class SettingsClient
{
public:
	// client name will be displayed when ApplySettings throws
	SettingsClient(const wchar_t* client_name);

	virtual ~SettingsClient();

	// implement this function to read any view specific settings and apply them as needed
	virtual void ApplySettings(SettingsSection& settings) = 0;

	// convenience function to call above ApplySettings
	void CallApplySettings();

	// calls ApplySettings when settings change; catches exceptions
	virtual void SettingsChanged(SettingsSection& settings);

private:
	// disconnect notification when view goes away
	boost::signals2::scoped_connection settings_chg_;
	CString err_msg_;
};
