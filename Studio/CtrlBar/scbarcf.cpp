/////////////////////////////////////////////////////////////////////////
//
// CSizingControlBarCF          Version 2.44
// 
// Created: Dec 21, 1998        Last Modified: March 31, 2002
//
// See the official site at www.datamekanix.com for documentation and
// the latest news.
//
/////////////////////////////////////////////////////////////////////////
// Copyright (C) 1998-2002 by Cristi Posea. All rights reserved.
//
// This code is free for personal and commercial use, providing this 
// notice remains intact in the source files and all eventual changes are
// clearly marked with comments.
//
// You must obtain the author's consent before you can include this code
// in a software library.
//
// No warrantee of any kind, express or implied, is included with this
// software; use at your own risk, responsibility for damages (if any) to
// anyone resulting from the use of this software rests entirely with the
// user.
//
// Send bug reports, bug fixes, enhancements, requests, flames, etc. to
// cristi@datamekanix.com or post them at the message board at the site.
/////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "scbarcf.h"

/////////////////////////////////////////////////////////////////////////
// CSizingControlBarCF

IMPLEMENT_DYNAMIC(CSizingControlBarCF, baseCSizingControlBarCF);

CSizingControlBarCF::CSizingControlBarCF()
{
	active_ = false;
}

BEGIN_MESSAGE_MAP(CSizingControlBarCF, baseCSizingControlBarCF)
	//{{AFX_MSG_MAP(CSizingControlBarCF)
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_SETTEXT, OnSetText)
END_MESSAGE_MAP()

void CSizingControlBarCF::OnUpdateCmdUI(CFrameWnd* target, BOOL disableIfNoHndler)
{
	baseCSizingControlBarCF::OnUpdateCmdUI(target, disableIfNoHndler);

	if (!HasGripper())
		return;

	bool need_paint = false;

	CWnd* focus= GetFocus();
	bool active_old= active_;

	active_ = focus->GetSafeHwnd() && IsChild(focus);

	if (active_ != active_old)
		need_paint = true;

	if (need_paint)
		SendMessage(WM_NCPAINT);
}

// gradient defines (if not already defined)
#ifndef COLOR_GRADIENTACTIVECAPTION
#define COLOR_GRADIENTACTIVECAPTION     27
#define COLOR_GRADIENTINACTIVECAPTION   28
#define SPI_GETGRADIENTCAPTIONS         0x1008
#endif

void CSizingControlBarCF::NcPaintGripper(CDC* dc, CRect client_rect)
{
	if (!HasGripper())
		return;

	// compute the caption rectangle
	BOOL horz= false;
	CRect grip= client_rect;
	CRect btn= m_biHide.GetRect();
	if (horz)
	{   // left side gripper
		grip.left -= gripper_height_ + 1;
		grip.right = grip.left + gripper_height_ - 1;
	}
	else
	{   // gripper at top
		grip.top -= gripper_height_ + 1;
		grip.bottom = grip.top + gripper_height_ - 1;
	}

	if (CWnd* focus= GetFocus())
		active_ = focus->GetSafeHwnd() && IsChild(focus);

	dc->FillSolidRect(grip, ::GetSysColor(active_ ? COLOR_HIGHLIGHT : COLOR_3DFACE));
	CString title;
	GetWindowText(title);
	dc->SelectStockObject(DEFAULT_GUI_FONT);
	dc->SetTextColor(::GetSysColor(active_ ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT));
	grip.DeflateRect(2, 0);
	dc->DrawText(title, grip, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

	// draw the button
	m_biHide.Paint(dc);
}

LRESULT CSizingControlBarCF::OnSetText(WPARAM wParam, LPARAM lParam)
{
	LRESULT result= baseCSizingControlBarCF::OnSetText(wParam, lParam);

	SendMessage(WM_NCPAINT);

	return result;
}
