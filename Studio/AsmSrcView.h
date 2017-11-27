/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

// Source code editor based on Scintilla

#pragma once
#include "scintilla-mfc\ScintillaDocView.h"
#include "PointerView.h"
#include "SettingsClient.h"
class AsmSrcDoc;


class AsmSrcView : public CScintillaView, public PointerView, SettingsClient
{
	typedef CScintillaView BaseView;
protected: // create from serialization only
	AsmSrcView();
	DECLARE_DYNCREATE(AsmSrcView)

	// Attributes
public:
	void RemoveBreakpoint(int line);
	void ClearAllBreakpoints();
	void AddBreakpoint(int line, Defs::Breakpoint bp);
	int GetCurrLineNo();
	void SetErrMark(int line);
	void SetPointer(int line, bool scroll= false);
	void GoToBookmark();
	void OnToggleWhiteSpace();
	void OnUpdateToggleWhiteSpace(CCmdUI* cmdui);
	AsmSrcDoc* GetDocument();

	int GetPointerLine() const      { return pointer_line_; }
	int GetErrorMarkLine() const    { return err_mark_line_; }

	int GetLineCount()			{ return GetCtrl().GetLineCount(); }

	void GetText(CString& text);

	// return all lines with brakpoint markers
	std::set<int> GetAllBreakpoints();

	// Implementation
	virtual ~AsmSrcView();

protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL OnPreparePrinting(CPrintInfo* info);
	virtual void OnBeginPrinting(CDC* dc, CPrintInfo* info);
	virtual void OnEndPrinting(CDC* dc, CPrintInfo* info);
	afx_msg int OnCreate(LPCREATESTRUCT create_struct);
	afx_msg void OnContextMenu(CWnd* wnd, CPoint point);
	afx_msg void ToggleBookmark();
	DECLARE_MESSAGE_MAP()

private:
	virtual void OnModified(SCNotification* notification);
	virtual void OnUpdateUI(SCNotification* notification);
	afx_msg LRESULT OnRemoveErrMark(WPARAM wParam, LPARAM lParam);
	virtual void SetPointer(int line, const std::wstring& doc_path, cf::uint32 pc, bool scroll);
	virtual void ApplySettings(SettingsSection& settings);
	void OnDestroy();
	int ScrollToLine(int line, bool scroll= FALSE);

	int pointer_line_;
	int err_mark_line_;
};

#ifndef _DEBUG
inline AsmSrcDoc* AsmSrcView::GetDocument()
{ return (AsmSrcDoc*)m_pDocument; }
#endif
