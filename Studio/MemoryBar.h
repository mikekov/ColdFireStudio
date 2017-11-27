#pragma once
#include "resource.h"
#include "EditBox.h"
#include "..\ColdFire\Types.h"
class HexViewWnd;
class Debugger;

// MemoryBar dialog

class MemoryBar : public CDialog
{
	DECLARE_DYNAMIC(MemoryBar)

public:
	MemoryBar(HexViewWnd& view, CWnd* parent= nullptr);   // standard constructor
	virtual ~MemoryBar();

	enum { IDD = IDD_MEMORY_BAR };

	bool Create(CWnd* parent);

	void SetReadOnly(bool read_only);

	void Notify(int event, UINT data, Debugger& debugger);

	void WatchRegister(cf::Register reg);
	void SetGrouping(int g);
	void SetHexOnly();

protected:
	virtual void DoDataExchange(CDataExchange* dx);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

private:
	EditBox address_;
	CFont hex_font_;
	CToolBarCtrl tbar_;
	CToolBarCtrl sizes_;
	HexViewWnd& view_;
	CBrush backgnd_;
	CBrush backgnd_read_only_;
	CSpinButtonCtrl spin_;
	bool read_only_;

	void OnClickedBWL(UINT cmd);
	void OnClickedType(UINT cmd);
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	HBRUSH OnCtlColor(CDC* dc, CWnd* wnd, UINT ctlColor);
	void OnAddressChanged(EditBox& edit);
	void Refresh(EditBox& edit, bool force);
};
