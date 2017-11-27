/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

// MemoryDC.cpp: implementation of the MemoryDC class.
//
//////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "MemoryDC.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

MemoryDC::MemoryDC(CDC& dc, CWnd* wnd, COLORREF rgb_clr_back/*= -1*/)
{
	CRect rect;
	wnd->GetClientRect(rect);
	Init(dc, rect, rgb_clr_back);
}

MemoryDC::MemoryDC(CDC& dc, const CRect& rect)
{
	Init(dc, rect, -1);
}


void MemoryDC::BitBlt()
{
	POINT org= GetWindowOrg();
	CPoint vp_org= GetViewportOrg();
	vp_org -= pos_;
	SetViewportOrg(0, 0);
	dc_->BitBlt(org.x - vp_org.x, org.y - vp_org.y, size_.cx, size_.cy, this, org.x, org.y, SRCCOPY);
}

MemoryDC::~MemoryDC()
{
	DeleteDC();		// delete in this order!
	screen_bmp_.DeleteObject();
}



void MemoryDC::Init(CDC& dc, const CRect& rect, COLORREF rgb_clr_back)
{
	size_ = rect.Size();
	pos_ = rect.TopLeft();
	dc_ = &dc;

	screen_bmp_.CreateCompatibleBitmap(&dc, size_.cx, size_.cy);
	if (screen_bmp_.m_hObject == 0)
	{
		DWORD dw= ::GetLastError();
		return;
	}

	CreateCompatibleDC(&dc);
	SelectObject(&screen_bmp_);
	if (rgb_clr_back != -1)
		FillSolidRect(0, 0, size_.cx, size_.cy, rgb_clr_back);

	POINT org= dc.GetWindowOrg();
	SetWindowOrg(org);

	POINT vp_org= dc.GetViewportOrg();
	SetViewportOrg(-rect.left + vp_org.x, -rect.top + vp_org.y);

	SetBrushOrg(rect.left % 8, rect.top % 8);
}
