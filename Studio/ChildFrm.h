/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#pragma once
#ifndef _ChildFrame_
#define _ChildFrame_


class CChildFrame : public CMDIChildWnd
{
	DECLARE_DYNCREATE(CChildFrame)
public:
	CChildFrame();

	// Attributes
	CSplitterWnd splitter_wnd_;

	// Operations
public:

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CChildFrame)
public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:
	//}}AFX_VIRTUAL

	void ActivateFrame(int cmd_show);

	virtual BOOL IsTabbedMDIChild() { return true; }

	// Implementation
public:
	virtual ~CChildFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CChildFrame)
	// NOTE - the ClassWizard will add and remove member functions here.
	//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	void OnMDIActivate(BOOL activate, CWnd* activateWnd, CWnd*);
	void OnDestroy();
	virtual void OnUpdateFrameTitle(BOOL addToTitle);
};


#endif
