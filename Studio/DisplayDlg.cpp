/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "DisplayDlg.h"


BEGIN_MESSAGE_MAP(DisplayDlg, CSizingControlBarCF)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()


DisplayDlg::DisplayDlg()
{}


DisplayDlg::~DisplayDlg()
{}


void DisplayDlg::OnUpdateCmdUI(CFrameWnd* target, BOOL disableIfNoHndler)
{
	CSizingControlBarCF::OnUpdateCmdUI(target, false);
}


int DisplayDlg::OnCreate(CREATESTRUCT* cs)
{
	m_szHorz = m_szVert = m_szFloat = BarSize();

	if (CSizingControlBarCF::OnCreate(cs) == -1)
		return -1;

	return 0;
}


CSize DisplayDlg::CalcFixedLayout(BOOL stretch, BOOL horz)
{
	return BarSize();
}


CSize DisplayDlg::CalcDynamicLayout(int length, DWORD mode)
{
	CSize size= CSizingControlBarCF::CalcDynamicLayout(length, mode);

	if (mode & (LM_HORZDOCK | LM_VERTDOCK)) // docked?
		size.cy += 30; // caption TODO: read caption height

	return size;
}


BOOL DisplayDlg::OnEraseBkgnd(CDC* dc)
{
	// for flicker-free redrawing
	return true;
}


void DisplayDlg::OnSize(UINT type, int cx, int cy)
{
	CSizingControlBarCF::OnSize(type, cx, cy);

	// in the absence of window class flags CS_*REDRAW invalidate display window
	Invalidate(false);
}
