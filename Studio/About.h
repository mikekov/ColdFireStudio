/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

// AboutDlg dialog used for App About

#include "resource.h"
#include "StaticLink.h"
#include "load_jpeg.h"


class AboutDlg : public CDialog
{
public:
	AboutDlg();

	// Dialog Data
	//{{AFX_DATA(AboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	CStatic	title_ctrl_;
	CString	version_string_;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(AboutDlg)
protected:
	virtual void DoDataExchange(CDataExchange* DX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:
	//{{AFX_MSG(AboutDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	CStaticLink web_page_link_;

	CBitmap about_;
	JpegPicture backgnd_;
	//CFont title_font_;

	afx_msg BOOL OnEraseBkgnd(CDC* dc);
	afx_msg HBRUSH OnCtlColor(CDC* dc, CWnd* wnd, UINT ctl_color);
	void OnLButtonDown(UINT, CPoint);
};
