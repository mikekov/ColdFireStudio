/*-----------------------------------------------------------------------------
	ColdFire Studio

Copyright (c) 1996-2008 Mike Kowalski
-----------------------------------------------------------------------------*/

#include <afxpropertygridctrl.h>
class SettingsSection;


class OptionsDlg : public CDialog
{
public:
	OptionsDlg(CWnd* parent);

	virtual ~OptionsDlg();

	//{{AFX_DATA(OptionsDlg)
	enum { IDD = IDD_OPTIONS };
	//}}AFX_DATA

	typedef std::map<CMFCPropertyGridProperty*, std::string> Map;

	DECLARE_MESSAGE_MAP()

private:
	afx_msg void OnResetToDefaults();
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* dc);
	virtual void OnOK();
	void SettingsToUI(const SettingsSection& settings);
	afx_msg void OnApply();
	void Apply();

	CMFCPropertyGridCtrl properties_;
	Map map_;
};
