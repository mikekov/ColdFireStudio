#pragma once
#include "DisasmDoc.h"
#include "DisasmBar.h"
#include "PointerView.h"
#include "SettingsClient.h"


// DisasmView view

class DisasmView : public CView, public PointerView, SettingsClient
{
	DECLARE_DYNCREATE(DisasmView)

public:
	bool ShowCodeBytes() const;
	bool ShowCodeAscii() const;

	void GoToAddress(cf::uint32 address);

protected:
	DisasmView();           // protected constructor used by dynamic creation
	virtual ~DisasmView();
	virtual void OnPrepareDC(CDC* dc, CPrintInfo* info = NULL);
	virtual BOOL OnScroll(UINT scrollCode, UINT pos, BOOL doScroll = TRUE);
	virtual void OnInitialUpdate();
	virtual void OnDraw(CDC* dc);      // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL OnScrollBy(CSize sizeScroll, BOOL doScroll = TRUE);
	virtual void OnUpdate(CView* sender, LPARAM hint, CObject* pHint);

	//afx_msg LRESULT OnExitDebugger(WPARAM /* wParam */, LPARAM /* lParam */);
	afx_msg void OnVScroll(UINT sbcode, UINT pos, CScrollBar* scrollBar);
	afx_msg void OnKeyDown(UINT chr, UINT repCnt, UINT flags);
	afx_msg BOOL OnEraseBkgnd(CDC* dc);
	afx_msg void OnContextMenu(CWnd* wnd, CPoint point);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	void OnMouseMove(UINT flags, CPoint point);
	void OnMouseLeave();
	BOOL OnMouseWheel(UINT flags, short delta, CPoint pos);

	DECLARE_MESSAGE_MAP()

private:
	virtual void SetPointer(int line, const std::wstring& doc_path, cf::uint32 pc, bool scroll);
	virtual void ApplySettings(SettingsSection& settings);

	int font_height_;
	int font_width_;

	int no_of_lines(RECT& prect);
	void scroll(UINT nSBCODE, int nPos, int nRepeat= 1);
	void set_scroll_range();
	CRect get_view_rect();

	DisasmDoc* GetDocument()	{ return static_cast<DisasmDoc*>(CView::GetDocument()); }
	void Resize();
	void OnCmd(UINT cmd);
	void OnLButtonDown(UINT, CPoint);
	int FindLineNumber(CPoint pos);
	int MaxLineLength() const;

	DisasmBar header_;
	CFont font_;
	LOGFONT log_font_;
	cf::uint32 pc_;		// program counter; draw pointer at this location
	cf::uint32 end_;	// end of disassembly in this view
	std::vector<cf::uint32> addresses_;	// one address per disassembly line
	int current_line_;
};
