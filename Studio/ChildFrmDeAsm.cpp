/*-----------------------------------------------------------------------------
	ColdFire Studio

Copyright (c) 1996-2008 Michal Kowalski
-----------------------------------------------------------------------------*/

// ChildFrm.cpp : implementation of the ChildFrameDeAsm class
//

#include "stdafx.h"
#include "ChildFrmDeAsm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// ChildFrameDeAsm

IMPLEMENT_DYNCREATE(ChildFrameDeAsm, CMDIChildWnd)

BEGIN_MESSAGE_MAP(ChildFrameDeAsm, CMDIChildWnd)
  //{{AFX_MSG_MAP(ChildFrameDeAsm)
    // NOTE - the ClassWizard will add and remove mapping macros here.
    //    DO NOT EDIT what you see in these blocks of generated code !
  //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// ChildFrameDeAsm construction/destruction

ChildFrameDeAsm::ChildFrameDeAsm()
{
  // TODO: add member initialization code here
	
}

ChildFrameDeAsm::~ChildFrameDeAsm()
{
}

BOOL ChildFrameDeAsm::PreCreateWindow(CREATESTRUCT& cs)
{
  // TODO: Modify the Window class or styles here by modifying
  //  the CREATESTRUCT cs

  return CMDIChildWnd::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// ChildFrameDeAsm diagnostics

#ifdef _DEBUG
void ChildFrameDeAsm::AssertValid() const
{
  CMDIChildWnd::AssertValid();
}

void ChildFrameDeAsm::Dump(CDumpContext& dc) const
{
  CMDIChildWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// ChildFrameDeAsm message handlers

BOOL ChildFrameDeAsm::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* context) 
{
	return CMDIChildWnd::OnCreateClient(lpcs, context);
}
