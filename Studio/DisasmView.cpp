/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

// DisasmView.cpp : Disassembly view

#include "pch.h"
#include "DisasmView.h"
#include "DisasmDoc.h"
#include "Broadcast.h"
#include "MemoryDC.h"
#include "App.h"
#include "Settings.h"

extern COLORREF CalcColor(COLORREF rgb_color1, COLORREF rgb_color2, float bias);

// DisasmView

IMPLEMENT_DYNCREATE(DisasmView, CView)

DisasmView::DisasmView() : SettingsClient(L"Disassembly window")
{
	current_line_ = -1;
	memset(&log_font_, 0, sizeof(log_font_));
	font_width_ = font_height_ = 1;
	pc_ = ~0;
	end_ = ~0;
}

DisasmView::~DisasmView()
{}

BEGIN_MESSAGE_MAP(DisasmView, CView)
	ON_WM_CONTEXTMENU()
	ON_WM_VSCROLL()
	ON_WM_KEYDOWN()
	ON_WM_ERASEBKGND()
	ON_WM_CONTEXTMENU()
	ON_WM_SIZE()
	//ON_MESSAGE(Broadcast::WM_USER_EXIT_DEBUGGER, OnExitDebugger)
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSELEAVE()
	ON_WM_MOUSEWHEEL()
END_MESSAGE_MAP()


int DisasmView::no_of_lines(RECT& rect)	// calculate number of lines that fit in a window
{
	if (font_height_ == 0)
		return 1;	// not yet ready

	int h= rect.bottom - rect.top;
	if (h >= font_height_)
		return h / font_height_;		// no of rows in window
	else
		return 1;						// always at least one row
}


enum	// sizes in characters
{
	ADDRESS= 10,
	CODE= 15,
	ASCII= 10,
	INSTRUCTION= 48	// est.
};


int DisasmView::MaxLineLength() const
{
	int len= ADDRESS;
	if (header_.show_bytes_)
		len += CODE;
	if (header_.show_ascii_)
		len += ASCII;
	len += INSTRUCTION;
	return len;
}


static void DrawBreakpoint(CDC& dc, int x, int y, int h, bool active, COLORREF breakpoint_color)
{
	CPen pen(PS_SOLID,0,::GetSysColor(COLOR_WINDOWTEXT));
	CPen* oldPen= dc.SelectObject(&pen);
	if (oldPen == nullptr)
		return;
	CBrush brush(active ? breakpoint_color : dc.GetBkColor());
	CBrush* oldBrush= dc.SelectObject(&brush);
	if (oldBrush == nullptr)
	{
		dc.SelectObject(oldPen);
		return;
	}

	dc.Ellipse(x, y, x + h, y + h);

	dc.SelectObject(oldPen);
	dc.SelectObject(oldBrush);
}


static void DrawPointer(CDC& dc, CRect marker, COLORREF color)
{
	int x= marker.left;
	int y= marker.top;
	int h= marker.Height();

	static const POINT shape[]=
	{ {-4,-3}, {0,-3}, {0,-7}, {7,0}, {0,7}, {0,3}, {-4,3}, {-4,-3} };
	const int size= sizeof shape / sizeof(POINT);
	POINT coords[size];

	x += (6 * h) / 15;
	y += (7 * h) / 15;
	for (int i=0; i<size; i++)
	{
		coords[i].x = x + (shape[i].x * h) / 15;	// scale & offset
		coords[i].y = y + (shape[i].y * h) / 15;
	}

	CPen* oldPen, pen(PS_SOLID, 0, ::GetSysColor(COLOR_WINDOWTEXT));
	oldPen = dc.SelectObject(&pen);
	if (oldPen == nullptr)
		return;
	CBrush brush(color);
	CBrush* oldBrush= dc.SelectObject(&brush);
	if (oldBrush == nullptr)
	{
		dc.SelectObject(oldPen);
		return;
	}

	dc.SetPolyFillMode(ALTERNATE);
	dc.Polygon(coords,size);

	dc.SelectObject(oldPen);
	dc.SelectObject(oldBrush);
}


void DisasmView::ApplySettings(SettingsSection& settings)
{
	log_font_ = settings.get_font("disasm.font");
	log_font_.lfPitchAndFamily = FIXED_PITCH;

	font_.DeleteObject();
	font_.CreateFontIndirect(&log_font_);

	Invalidate();
}


void DisasmView::OnDraw(CDC* pdc)	// display disassembled code, line by line
{
	auto doc= GetDocument();
	if (doc == nullptr)
		return;
	auto dbg= doc->GetDebugger();
	if (dbg == nullptr)
		return;

	auto& clr1= AppSettings().section("editor.marker_colors");
	COLORREF bp= clr1.get_color("breakpoint");
	COLORREF cur= clr1.get_color("current");
	auto& clr2= AppSettings().section("disasm.colors");
	COLORREF address_color= clr2.get_color("address");
	COLORREF code_color= clr2.get_color("code");
	COLORREF instructions_color= clr2.get_color("instr");
	COLORREF backgnd_color= clr2.get_color("backgnd");

	MemoryDC dc(*pdc, this, backgnd_color);
	dc.SetBkMode(TRANSPARENT);
	dc.SelectObject(&font_);
	dc.SetBkColor(backgnd_color);

	CRect rect= get_view_rect();
	int lines= no_of_lines(rect);

	rect.bottom = rect.top + font_height_;

	rect.left += font_width_;		// left margin
	if (rect.left >= rect.right)
		rect.right = rect.left;

	cf::uint32 addr= doc->GetStartAddr();
	addresses_.clear();
	addresses_.reserve(lines);

	for (int i= 0; i <= lines; ++i)
	{
		if (i == current_line_)
		{
			COLORREF color= CalcColor(::GetSysColor(COLOR_HIGHLIGHT), backgnd_color, 0.15f);
			auto copy= rect;
			copy.right = copy.left + MaxLineLength() * font_width_;
			dc.FillSolidRect(copy, color);
		}
		else
			dc.SetBkColor(backgnd_color);

		auto pointer= addr == pc_;
		auto breakpoint = dbg->GetBreakpoint(addr) == Defs::BPT_EXECUTE;
		addresses_.push_back(addr);
		std::string str= doc->GetDeasmLine(addr, header_.show_bytes_, header_.show_ascii_);
		const char* p= str.c_str();
		int x= rect.left;

		if (dc.RectVisible(&rect) && !str.empty())
		{
			dc.SetBkColor(backgnd_color);
			dc.SetTextColor(address_color);
			::TextOutA(dc, x, rect.top, p, 9);
			size_t len= ADDRESS;
			if (str.length() > ADDRESS-1 && str[8] != ' ')
				len++;
			x += static_cast<int>(font_width_ * len);

			if (str.length() > len)
			{
				p += len;
				size_t len= 0;
				if (header_.show_bytes_ || header_.show_ascii_)
				{
					dc.SetTextColor(code_color);
					if (header_.show_bytes_)
						len += CODE;
					if (header_.show_ascii_)
						len += ASCII;
					::TextOutA(dc, x, rect.top, p, static_cast<int>(len));
					x += static_cast<int>(font_width_ * len);
					p += len;
				}

				dc.SetTextColor(instructions_color);
				ASSERT(str.length() > len);
				if (str.length() > len)
					::TextOutA(dc, x, rect.top, p, static_cast<int>(strlen(p)));
			}
			else
			{
				x = static_cast<int>(rect.left + font_width_ * len);
				// no memory area
				dc.SetTextColor(code_color);
				::TextOutA(dc, x, rect.top, "-", 1);
			}

			if (breakpoint)
				DrawBreakpoint(dc, x - font_width_, rect.top, font_height_, true, bp);
			if (pointer)
				DrawPointer(dc, CRect(CPoint(x - font_width_, rect.top), CSize(font_width_, font_height_)), cur);
		}

		rect.OffsetRect(0, font_height_);
	}

	end_ = addr;

	dc.BitBlt();
}


void DisasmView::OnPrepareDC(CDC* dc, CPrintInfo* info)
{
	if (info == nullptr && !dc->IsPrinting())
	{
		dc->SetBkMode(OPAQUE);
		dc->SelectObject(&font_);
		TEXTMETRIC tm;
		dc->GetTextMetrics(&tm);
		font_height_ = (int)tm.tmHeight + (int)tm.tmExternalLeading;
		font_width_ = tm.tmAveCharWidth;
	}
	CView::OnPrepareDC(dc, info);
}


BOOL DisasmView::PreCreateWindow(CREATESTRUCT& cs)
{
	bool ret= CView::PreCreateWindow(cs);
	cs.style |= WS_VSCROLL | WS_CLIPCHILDREN;
	cs.dwExStyle &= ~WS_EX_CLIENTEDGE;
	return ret;
}


//=============================================================================


BOOL DisasmView::OnScroll(UINT scrollCode, UINT pos, BOOL doScroll)
{
	if (doScroll)
		return TRUE;

	return CView::OnScroll(scrollCode, pos, doScroll);
}


BOOL DisasmView::OnScrollBy(CSize sizeScroll, BOOL doScroll)
{
	return CView::OnScrollBy(sizeScroll, doScroll);
}


void DisasmView::OnInitialUpdate()
{
	header_.Create(DisasmBar::IDD, this);
	header_.view_ = this;

	CView::OnInitialUpdate();

	set_scroll_range();

	CallApplySettings();

	Resize();
}


//-----------------------------------------------------------------------------
// scroll window
//
void DisasmView::scroll(UINT sb_code, int pos, int repeat)
{
	DisasmDoc* doc = GetDocument();
	if (doc == nullptr)
		return;

	int delta_lines= 0;
	cf::uint32 addr= 0;
	bool absolute= false;

	switch (sb_code)
	{
	case SB_ENDSCROLL:	// End scroll
		break;

	case SB_LINEDOWN:	// Scroll one line down
		delta_lines = 1;
		break;

	case SB_LINEUP:	// Scroll one line up
		delta_lines = -1;
		break;

	case SB_PAGEDOWN:	// Scroll one page down
		delta_lines = no_of_lines(get_view_rect());
		break;

	case SB_PAGEUP:	// Scroll one page up
		delta_lines = -no_of_lines(get_view_rect());
		break;

	case SB_TOP:	// Scroll to top
		absolute = true;
		addr = 0;
		break;

	case SB_BOTTOM:	// Scroll to bottom
		absolute = true;
		addr = ~0;
		break;

	case SB_THUMBPOSITION:   // Scroll to the absolute position. The current position is provided in pos
	case SB_THUMBTRACK:	// Drag scroll box to specified position. The current position is provided in pos
		absolute = true;
		addr = pos;
		addr <<= 4;
		break;
	default:
		break;
	}

	if (absolute)
		addr = doc->FindAbsAddress(addr);
	else
		addr = doc->FindAddress(doc->GetStartAddr(), delta_lines);

	if (addr != doc->GetStartAddr())
	{
		doc->SetStartAddr(addr);
		header_.SetAddress(addr);
		Invalidate();
	}

	SetScrollPos(SB_VERT, int(addr >> 4));
}


void DisasmView::OnVScroll(UINT sb_code, UINT pos, CScrollBar* scrollBar)
{
	SCROLLINFO si;
	si.cbSize = sizeof si;
	si.fMask = SIF_POS | SIF_TRACKPOS;
	GetScrollInfo(SB_VERT, &si, si.fMask);

	pos = sb_code == SB_THUMBTRACK ? si.nTrackPos : si.nPos;

	scroll(sb_code, pos);
}


void DisasmView::OnKeyDown(UINT chr, UINT rep_cnt, UINT flags)
{
	switch (chr)
	{
	case VK_DOWN:
		scroll(SB_LINEDOWN, 0, rep_cnt);
		break;
	case VK_UP:
		scroll(SB_LINEUP, 0, rep_cnt);
		break;
	case VK_NEXT:
		scroll(SB_PAGEDOWN, 0, rep_cnt);
		break;
	case VK_PRIOR:
		scroll(SB_PAGEUP, 0, rep_cnt);
		break;
	case VK_HOME:
		scroll(SB_TOP, 0, rep_cnt);
		break;
	case VK_END:
		scroll(SB_BOTTOM, 0, rep_cnt);
		break;
	default:
		CView::OnKeyDown(chr, rep_cnt, flags);
	}
}

//=============================================================================

void DisasmView::OnUpdate(CView* sender, LPARAM hint, CObject* pHint)
{
	DisasmDoc* doc = GetDocument();
	if (doc)
	{
		auto addr= doc->GetStartAddr();
		header_.SetAddress(addr);

		SetScrollPos(SB_VERT, addr >> 4);

		pc_ = doc->GetPC();
	}

	Invalidate();
}


//=============================================================================

//afx_msg LRESULT DisasmView::OnExitDebugger(WPARAM /* wParam */, LPARAM /* lParam */)
//{
//	GetDocument()->OnCloseDocument();
//	return 0;
//}

//-----------------------------------------------------------------------------

BOOL DisasmView::OnEraseBkgnd(CDC* dc)
{
	return TRUE;
}

//-----------------------------------------------------------------------------
// Popup menu

void DisasmView::OnContextMenu(CWnd* wnd, CPoint point)
{
	CMenu menu;
	VERIFY(menu.LoadMenu(IDR_POPUP_DISASM));

	CMenu* popup = menu.GetSubMenu(0);
	if (popup == nullptr)
		return;

	if (point.x == -1 && point.y == -1)     // ctx menu keyboard invoked?
	{
		CRect rect;
		GetClientRect(rect);

		point = rect.TopLeft();
		CPoint pt(0, 0);
		ClientToScreen(&pt);
		point.x = pt.x + rect.Width() / 2;   // window center
		point.y = pt.y + rect.Height() / 2;
	}

	popup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, AfxGetMainWnd());
}


void DisasmView::OnSize(UINT nType, int cx, int cy)
{
	CView::OnSize(nType, cx, cy);

	Resize();

	set_scroll_range();
}


void DisasmView::set_scroll_range()
{
	CRect rect= get_view_rect();
	int lines= no_of_lines(rect);

	SCROLLINFO si;
	si.cbSize = sizeof si;
	si.fMask = SIF_RANGE | SIF_PAGE;
	si.nMin = 0;
	si.nMax = 0x10000000;
	si.nPage = lines; // estimate only
	SetScrollInfo(SB_VERT, &si, FALSE);
}


void DisasmView::Resize()
{
	if (header_.m_hWnd == nullptr)
		return;

	CRect rect(0,0,0,0);
	GetClientRect(rect);

	CRect w;
	header_.GetWindowRect(w);
	header_.SetWindowPos(nullptr, 0, 0, rect.Width(), w.Height(), SWP_NOZORDER | SWP_NOACTIVATE);
}


CRect DisasmView::get_view_rect()
{
	CRect rect(0,0,0,0);
	GetClientRect(rect);

	if (rect.IsRectEmpty() || header_.m_hWnd == nullptr)
		return rect;

	CRect w;
	header_.GetWindowRect(w);
	rect.top += w.Height();
	if (rect.bottom < rect.top)
		rect.bottom = rect.top;

	return rect;
}


bool DisasmView::ShowCodeBytes() const
{
	return header_.show_bytes_;
}

bool DisasmView::ShowCodeAscii() const
{
	return header_.show_ascii_;
}


void DisasmView::GoToAddress(cf::uint32 address)
{
	if (DisasmDoc* doc = GetDocument())
		doc->SetStartAddr(address);
	Invalidate();

	SetScrollPos(SB_VERT, address >> 4);
}


void DisasmView::SetPointer(int line, const std::wstring& doc_path, cf::uint32 pc, bool scroll)
{
	if (pc != pc_)
	{
		pc_ = pc;
		Invalidate();

		if (scroll && pc != ~0)
		{
			auto* doc= GetDocument();
			auto addr= doc->GetStartAddr();
			if (pc < addr || pc >= end_-2)
				doc->SetStartAddr(pc);
		}
	}
}


int DisasmView::FindLineNumber(CPoint pos)
{
	if (font_height_ <= 0)
		return -1;
	int x= pos.x / font_width_;
	auto rect= get_view_rect();
	int lines= no_of_lines(rect);
	if (!rect.PtInRect(pos) || x >= MaxLineLength())
		return -1;
	auto line= (pos.y - rect.top) / font_height_;
	return line;
}


void DisasmView::OnLButtonDown(UINT, CPoint pt)
{
	SetFocus();	// move focus away from the bar above

	auto line= FindLineNumber(pt);

	if (static_cast<size_t>(line) < addresses_.size())
	{
		auto addr= addresses_[line];
		if (auto doc= GetDocument())
			if (auto dbg= doc->GetDebugger())
			{
				dbg->ToggleBreakpoint(addr);
				Invalidate();
			}
	}
}


void DisasmView::OnMouseMove(UINT flags, CPoint point)
{
	TRACKMOUSEEVENT track_mouse;
	track_mouse.cbSize = sizeof(track_mouse);
	track_mouse.dwFlags = TME_LEAVE;
	track_mouse.hwndTrack = GetSafeHwnd();
	track_mouse.dwHoverTime = 0;
	_TrackMouseEvent(&track_mouse);

	auto line= FindLineNumber(point);
	if (line != current_line_)
	{
		current_line_ = line;
		Invalidate();
	}
}


void DisasmView::OnMouseLeave()
{
	if (current_line_ != -1)
	{
		current_line_ = -1;
		Invalidate();
	}
}


BOOL DisasmView::OnMouseWheel(UINT flags, short delta, CPoint pos)
{
	SetFocus();	// move focus away from the bar above

	scroll(delta > 0 ? SB_LINEUP : SB_LINEDOWN, 0, 1);

	return false;
}
