/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "About.h"
#include "ProtectedCall.h"


AboutDlg::AboutDlg() : CDialog(AboutDlg::IDD, AfxGetMainWnd())
{
}

void AboutDlg::DoDataExchange(CDataExchange* dx)
{
	CDialog::DoDataExchange(dx);

//	DDX_Control(dx, IDC_TITLE, title_ctrl_);
	DDX_Text(dx, IDC_ABOUT_VER, version_string_);
}

BEGIN_MESSAGE_MAP(AboutDlg, CDialog)
	ON_WM_ERASEBKGND()
	ON_WM_CTLCOLOR()
	ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()


void AboutDlg::OnLButtonDown(UINT, CPoint)
{
	EndDialog(IDOK);
}


BOOL AboutDlg::OnInitDialog()
{
	bool ok= ProtectedCall([&]
	{
		TCHAR path[MAX_PATH];
		path[0] = 0;
		::GetModuleFileName(AfxGetResourceHandle(), path, MAX_PATH);
		CFileStatus stat;
		CFile::GetStatus(path, stat);

		const int MAX= 200;
		TCHAR date[MAX + 2]= { 0 };
		SYSTEMTIME time;
		if (stat.m_mtime.GetAsSystemTime(time))
			::GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &time, 0, date, MAX);

		const wchar_t* architecture=
#ifdef _WIN64
			L"x64";
#else
			L"x86";
#endif
		if (HRSRC rsrc= ::FindResource(AfxGetResourceHandle(), MAKEINTRESOURCE(VS_VERSION_INFO), RT_VERSION))
			if (HGLOBAL global = ::LoadResource(AfxGetResourceHandle(), rsrc))
			{
				VS_FIXEDFILEINFO* ver= reinterpret_cast<VS_FIXEDFILEINFO*>((char*)::LockResource(global) + 0x28);
				if (ver->dwSignature == 0xfeef04bd)
					version_string_.Format(IDS_ABOUT_VER,
					(int)HIWORD(ver->dwProductVersionMS), (int)LOWORD(ver->dwProductVersionMS),
					(int)HIWORD(ver->dwProductVersionLS), architecture, date);//(int)LOWORD(ver->dwProductVersionLS));

				::FreeResource(global);
			}

		VERIFY(backgnd_.LoadPicture(AfxGetResourceHandle(), IDB_ABOUT));

		CDialog::OnInitDialog();

		BITMAP bm;
		if (backgnd_.GetBitmap().GetBitmap(&bm) > 0)
		{
			//	CSize size= about_.GetBitmapDimension();
			SetWindowPos(0, 0, 0, bm.bmWidth, bm.bmHeight, SWP_NOMOVE | SWP_NOZORDER);
		}

		//	LOGFONT lf;
		//	title_ctrl_.GetFont()->GetLogFont(&lf);

		//GetFont()->GetLogFont(&lf);
		////  CClientDC dc(this);
		////    lf.lfHeight = -MulDiv(9, dc.GetDeviceCaps(LOGPIXELSY), 96);
		//lf.lfWeight = 700;      // bold
		//lf.lfHeight -= 2;		// larger
		//title_font_.CreateFontIndirect(&lf);
		//title_ctrl_.SetFont(&title_font_);

		web_page_link_.SubclassDlgItem(IDC_LINK, this, L"http://www.exifpro.com/utils.html");
		web_page_link_.color_ = 0xffffff;
	}, "Init about dialog");

	if (!ok)
		EndDialog(IDCANCEL);

	return true;
}


BOOL AboutDlg::OnEraseBkgnd(CDC* dc)
{
	CRect rect(0,0,0,0);
	GetClientRect(rect);

	// bitmap (x, y) location in the dialog
	CPoint pos(0, 0);

	DIBSECTION bmp;
	auto& bitmap= backgnd_.GetBitmap();
	if (bitmap.m_hObject && bitmap.GetObject(sizeof bmp, &bmp) && bmp.dsBm.bmBits)
	{
		dc->SetStretchBltMode(COLORONCOLOR);

		::StretchDIBits(*dc, pos.x, pos.y, bmp.dsBm.bmWidth, bmp.dsBm.bmHeight,
			0, 0, bmp.dsBm.bmWidth, bmp.dsBm.bmHeight, bmp.dsBm.bmBits,
			reinterpret_cast<BITMAPINFO*>(&bmp.dsBmih), DIB_RGB_COLORS, SRCCOPY);
	}
	else
		dc->FillSolidRect(rect, RGB(125,125,255));	// something's wrong

	return true;
}


HBRUSH AboutDlg::OnCtlColor(CDC* dc, CWnd* wnd, UINT ctlColor)
{
	HBRUSH br= CDialog::OnCtlColor(dc, wnd, ctlColor);

//	if (wnd != &web_page_link_)
//		return web_page_link_.CtlColor(dc, ctlColor);
//	else
	if (wnd != 0)
	{
		dc->SetTextColor(0xffffff);
		dc->SetBkMode(TRANSPARENT);
		return static_cast<HBRUSH>(::GetStockObject(NULL_BRUSH));
	}
	else
		return br;
}
