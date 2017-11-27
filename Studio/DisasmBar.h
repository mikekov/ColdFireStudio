#pragma once
#include "resource.h"
#include "../ColdFire/Types.h"
#include "NumberEdit.h"

class DisasmView;


// DisasmBar dialog

class DisasmBar : public CDialog
{
public:
	DisasmBar();
	virtual ~DisasmBar();

	int show_bytes_;
	int show_ascii_;
	DisasmView* view_;

	// Dialog Data
	enum { IDD = IDD_DISASM_BAR };

	void SetAddress(cf::uint32 address);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

private:
	HBRUSH OnCtlColor(CDC* dc, CWnd* wnd, UINT ctlColor);
	virtual void OnOK() {}
	virtual void OnCancel() {}
	virtual BOOL OnInitDialog();
	void OnAddressChange();
	LRESULT OnAddrChanged(WPARAM, LPARAM);
	void OnDeltaposSpin(NMHDR* nmhdr, LRESULT* result);

	afx_msg void OnBytes();
	afx_msg void OnAscii();
	afx_msg void OnGoToBank();

	CBrush backgnd_;
	NumberEdit address_edit_;
	CSpinButtonCtrl spin_;
	CFont mono_;
	bool in_update_;
	CComboBox bank_;
};
