#pragma once
#include "LoadCode.h"
#include "afxwin.h"


// LoadCodeDlg dialog

class LoadCodeDlg : public CDialog
{
	DECLARE_DYNAMIC(LoadCodeDlg)

public:
	LoadCodeDlg(CWnd* parent= nullptr);
	virtual ~LoadCodeDlg();

// Dialog Data
	enum { IDD = IDD_LOAD_CODE };

	CString path_;
	BinaryFormat type_;
	unsigned int address_;
	int selected_isa_;

protected:
	virtual void DoDataExchange(CDataExchange* dx);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual void OnOK();

	DECLARE_MESSAGE_MAP()

private:
	afx_msg void OnBrowse();
	CEdit path_edit_;
	int type_int_;
	CComboBox isa_;
	int file_filter_index_;
};
