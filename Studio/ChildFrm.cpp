/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "ChildFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CChildFrame, CMDIChildWnd)

BEGIN_MESSAGE_MAP(CChildFrame, CMDIChildWnd)
	ON_WM_MDIACTIVATE()
	ON_WM_DESTROY()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CChildFrame construction/destruction

CChildFrame::CChildFrame()
{}

CChildFrame::~CChildFrame()
{}


BOOL CChildFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	//cs.style = WS_BORDER; 
	return CMDIChildWnd::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CChildFrame diagnostics

#ifdef _DEBUG
void CChildFrame::AssertValid() const
{
	CMDIChildWnd::AssertValid();
}

void CChildFrame::Dump(CDumpContext& dc) const
{
	CMDIChildWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CChildFrame message handlers

void CChildFrame::ActivateFrame(int cmd_show)
{
	if (cmd_show == -1)
	{
		if (CMDIFrameWnd* frame= GetMDIFrame())
		{
			BOOL maximized;
			CWnd* child= frame->MDIGetActive(&maximized);

			if (child == nullptr)
			{
				// if first MDI window is being activated, maximize it

				ModifyStyle(0, WS_MAXIMIZE);
			}
		}

	}

	CMDIChildWnd::ActivateFrame(cmd_show);
}


void CChildFrame::OnMDIActivate(BOOL activate, CWnd* activate_wnd, CWnd* w)
{
	CMDIChildWnd::OnMDIActivate(activate, activate_wnd, w);

	if (activate_wnd)
		AfxGetMainWnd()->SendMessage(WM_APP, activate, LPARAM(activate_wnd));
}


void CChildFrame::OnDestroy()
{
	AfxGetMainWnd()->SendMessage(WM_APP, 2, LPARAM(this));
}


void CChildFrame::OnUpdateFrameTitle(BOOL addToTitle)
{
	CMDIChildWnd::OnUpdateFrameTitle(addToTitle);

	AfxGetMainWnd()->SendMessage(WM_APP, 0, LPARAM(this));
}
