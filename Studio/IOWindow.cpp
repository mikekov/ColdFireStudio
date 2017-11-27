/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

// IOWindow.cpp : Simple terminal window

#include "pch.h"
#include "resource.h"
#include "IOWindow.h"
#include "Broadcast.h"
#include "App.h"
#include <algorithm>
#include <utility>
#include "Settings.h"
#include "MemoryDC.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static bool wnd_registered_= false;
static CString wnd_class_;
CFont IOWindow::font_;
LOGFONT IOWindow::log_font_=
{
	13,	// LONG lfHeight;
	0,	// LONG lfWidth;
	0,	// LONG lfEscapement;
	0,	// LONG lfOrientation;
	0,	// LONG lfWeight;
	0,	// BYTE lfItalic;
	0,	// BYTE lfUnderline;
	0,	// BYTE lfStrikeOut;
	0,	// BYTE lfCharSet;
	0,	// BYTE lfOutPrecision;
	0,	// BYTE lfClipPrecision;
	0,	// BYTE lfQuality;
	FIXED_PITCH,	// BYTE lfPitchAndFamily;
	L"Courier"	// CHAR lfFaceName[LF_FACESIZE];
};

CPoint IOWindow::wnd_pos_= CPoint(0, 0);	// window location
int IOWindow::init_w_= 80;
int IOWindow::init_h_= 30;
COLORREF IOWindow::rgb_text_color_= RGB(0,0,0);
COLORREF IOWindow::rgb_backgnd_color_= RGB(255,255,255);

//-----------------------------------------------------------------------------

void IOWindow::RegisterWndClass()
{
	ASSERT(!wnd_registered_);
	if (wnd_registered_)
		return;
	wnd_class_ = AfxRegisterWndClass(0, ::LoadCursor(nullptr, IDC_ARROW), 0, AfxGetApp()->LoadIcon(IDI_IO_WINDOW));
	wnd_registered_ = true;
}

/////////////////////////////////////////////////////////////////////////////
// IOWindow

IMPLEMENT_DYNCREATE(IOWindow, CMiniFrameWnd)

IOWindow::IOWindow() : SettingsClient(L"Terminal window")
{
	m_hWnd = 0;
	width_ = height_ = 0;

	pos_x_ = pos_y_ = 0;	// cursor position
	cursor_count_ = 0;		// cursor hiding counter

	if (!wnd_registered_)
		RegisterWndClass();

	cursor_on_ = false;
	cursor_visible_ = false;

	CallApplySettings();
}

IOWindow::~IOWindow()
{}

//-----------------------------------------------------------------------------
// Create new window

bool IOWindow::Create()
{
	ASSERT(m_hWnd==0);
	CString title;
	VERIFY(title.LoadString(IDS_IO_WINDOW));

	RECT rect= {0,0,100,100};
	rect.left = wnd_pos_.x;
	rect.top = wnd_pos_.y;
	if (!CMiniFrameWnd::CreateEx(WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE, wnd_class_, title,
		WS_POPUP | WS_CAPTION | WS_SYSMENU | MFS_MOVEFRAME /*| MFS_SYNCACTIVE*/, rect, AfxGetMainWnd(), 0))
		return false;

	ModifyStyleEx(0, WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE);
	SetSize(init_w_, init_h_);

	timer_.Start(m_hWnd, 101, 250);

	SetFocus();

	return true;
}


void IOWindow::CalcFontSize()		// calc char dimensions
{
	ASSERT(m_hWnd);
	CClientDC dc(this);
	dc.SelectObject(&font_);
	TEXTMETRIC tm;
	dc.GetTextMetrics(&tm);
	char_h_ = (int)tm.tmHeight + (int)tm.tmExternalLeading;
	char_w_ = (int)tm.tmAveCharWidth;
}

//-----------------------------------------------------------------------------
// Set terminal window size in columns and rows

void IOWindow::SetSize(int w, int h, int resize/* =1*/)
{
	ASSERT(w > 0);
	ASSERT(h > 0);
	init_w_ = w;
	init_h_ = h;
	if (m_hWnd == nullptr)	// no window yet?
		return;
	int new_size= w * h;
	int old_size= width_ * height_;
	bool change=  width_ != w || height_ != h;
	width_ = w;
	height_ = h;
	if (data_.get() == nullptr || new_size != old_size)
	{
		// free it first
		data_ = nullptr;
		// attempt to allocate new buffer
		data_.reset(new UINT8[new_size]);
	}

	if (resize == 0)	// no size change?
		return;
	if (resize == -1 && !change)
		return;
	Resize(true);
}


void IOWindow::Resize(bool cls)
{
	CalcFontSize();
	CRect size(0, 0, char_w_ * width_, char_h_ * height_);
	CalcWindowRect(&size, CWnd::adjustOutside);
	SetWindowPos(nullptr, 0, 0, size.Width(), size.Height(), SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);
	if (cls)
		Cls();
}

// get terminal window size
void IOWindow::GetSize(int& w, int& h)
{
	w = m_hWnd ? width_ : init_w_;
	h = m_hWnd ? height_ : init_h_;
}

// set position of terminal window
void IOWindow::SetWndPos(const POINT& p)
{
	wnd_pos_ = p;
	if (m_hWnd == nullptr)	// no window yet?
		return;
	SetWindowPos(nullptr, p.x, p.y, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
}

// read terminal window position on the screen
CPoint IOWindow::GetWndPos()
{
	if (m_hWnd == nullptr)	// no window yet?
		return wnd_pos_;
	CRect rect;
	GetWindowRect(rect);
	return rect.TopLeft();
}


//-----------------------------------------------------------------------------

int IOWindow::put(char chr, int x, int y)
{
	if (data_.get() == nullptr)
		return -1;
	if (x > width_ || y > height_ || x < 0 || y < 0)
		return -2;
	data_[x + y * width_] = (UINT8)chr;
	return 0;
}


int IOWindow::invalidate(int x, int y)	// invalidate char (x, y) area
{
	RECT rect;
	ASSERT(char_h_ > 0 && char_w_ > 0);
	rect.left = x * char_w_;
	rect.top = y * char_h_;
	rect.right = rect.left + char_w_;
	rect.bottom = rect.top + char_h_;

	PostMessage(WM_USER);

	return 0;
}


int IOWindow::scroll(int dy)		// scroll lines by 'dy' rows
{
	if (data_.get() == nullptr)
		return -1;
	if (dy > height_ || dy < 0)
		return -2;
	// move lines
	memmove(data_.get(), data_.get() + dy * width_, (height_ - dy) * width_);
	// clear exposed space
	memset(data_.get() + (height_ - dy) * width_, 0, dy * width_);
	// invalidate entire window
	PostMessage(WM_USER);
	Invalidate();
	return 0;
}

//-----------------------------------------------------------------------------
int IOWindow::PutH(int chr)			// print hex number (8 bits)
{
	int h1= (chr >> 4) & 0x0f;
	int h2= chr & 0x0f;
	char buf[4];
	buf[0] = h1 > 9 ? h1 + 'A' - 10 : h1 + '0';
	buf[1] = h2 > 9 ? h2 + 'A' - 10 : h2 + '0';
	buf[2] = '\0';
	return PutS(buf);
}


int IOWindow::PutC(int chr)			// print character
{
	HideCursor();

	if (chr == 0x0a) // line feed?
	{
		if (++pos_y_ >= height_)
		{
			ASSERT(pos_y_ == height_);
			pos_y_--;
			scroll(1);		// scroll by one line
		}
		pos_x_ = 0;
	}
	else if (chr == 0x0d) // carriage return?
		pos_x_ = 0;
	else if (chr == 0x08) // backspace?
	{
		if (--pos_x_ < 0)
		{
			pos_x_ = width_ - 1;
			if (--pos_y_ < 0)
			{
				pos_y_ = 0;
				return 0;
			}
		}
		if (put(' ', pos_x_, pos_y_) < 0)
			return -1;
		invalidate(pos_x_, pos_y_);	// invalidate char rect
	}
	else if (chr == '\t') // tab?
	{
		const int tab_step= 8;
		const int spaces= tab_step - pos_x_ % tab_step;
		for (int i= 0; i < spaces; ++i)
			PutChr(' ');
	}
	else
		return PutChr(chr);

	return 0;
}


int IOWindow::PutChr(int chr)			// print character (verbatim, control chars are not interpreted)
{
	HideCursor();
	if (put(chr, pos_x_, pos_y_) < 0)
		return -1;
	invalidate(pos_x_, pos_y_);	// invalidate char rect
	if (++pos_x_ >= width_)
	{
		pos_x_ = 0;
		if (++pos_y_ >= height_)
		{
			ASSERT(pos_y_ == height_);
			pos_y_--;
			scroll(1);		// scroll by one line
		}
	}
	return 0;
}


int IOWindow::PutS(const char* str, int len/*= -1*/)	// print string of chars
{
	for (int i= 0; i < len || len == -1; i++)
	{
		if (str[i] == '\0')
			break;
		if (PutC(str[i]) < 0)
			return -1;
	}
	return 0;
}


bool IOWindow::SetPosition(int x, int y)	// set text position
{
	if (x > width_ || y > height_ || x < 0 || y < 0)
		return FALSE;
	pos_x_ = x;
	pos_y_ = y;
	return TRUE;
}


void IOWindow::GetPosition(int& x, int& y)	// get text position
{
	x = pos_x_;
	y = pos_y_;
}


bool IOWindow::Cls()			// clear window
{
	if (data_.get() == nullptr)
		return FALSE;
	memset(data_.get(), 0, height_ * width_);	// zero out
	pos_x_ = pos_y_ = 0;
	PostMessage(WM_USER);
	return TRUE;
}


int IOWindow::Input()			// input
{
	cursor_on_ = true;

	return input_buffer_.GetChar();		// returns available char or 0 if buffer is empty
}

//-----------------------------------------------------------------------------

BEGIN_MESSAGE_MAP(IOWindow, CMiniFrameWnd)
	//{{AFX_MSG_MAP(IOWindow)
	ON_WM_PAINT()
	ON_WM_GETMINMAXINFO()
	ON_WM_SIZE()
	ON_WM_DESTROY()
	ON_WM_TIMER()
	ON_WM_CHAR()
	ON_WM_CLOSE()
	ON_WM_KEYDOWN()
	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_PASTE, OnPaste)
	//}}AFX_MSG_MAP
	ON_MESSAGE(IOWindow::CMD_CLS, OnCls)
	ON_MESSAGE(IOWindow::CMD_PUTC, OnPutC)
	//ON_MESSAGE(Broadcast::WM_USER_START_DEBUGGER, OnStartDebug)
	//ON_MESSAGE(Broadcast::WM_USER_EXIT_DEBUGGER, OnExitDebug)
	ON_MESSAGE(IOWindow::CMD_IN, OnInput)
	ON_MESSAGE(IOWindow::CMD_POSITION, OnPosition)
	ON_MESSAGE(IOWindow::CMD_SIZE, OnSizeCmd)
	ON_MESSAGE(IOWindow::CMD_GET_PTR, &IOWindow::OnGetChannelPtr)
	ON_MESSAGE(WM_USER, &IOWindow::OnInvalidate)
END_MESSAGE_MAP()

LRESULT IOWindow::OnInvalidate(WPARAM, LPARAM)
{
	Invalidate();
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// IOWindow message handlers

void IOWindow::OnPaint()
{
	CPaintDC paint_dc(this);	// device context for painting

	if (data_.get() == nullptr)
		return;

	MemoryDC dc(paint_dc, this, rgb_backgnd_color_);

	dc.SelectObject(&font_);
	dc.SetBkMode(OPAQUE);
	dc.SetTextColor(rgb_text_color_);
	dc.SetBkColor(rgb_backgnd_color_);
	CPen pen(PS_SOLID, 1, rgb_text_color_);
	auto old= dc.SelectObject(pen);

	CString line;
	UINT8* src= data_.get();
	for (int y= 0, pos_y= 0; y < height_; y++, pos_y += char_h_)
	{
		auto save= src;
		bool special= false;
		TCHAR* dst= line.GetBuffer(width_);
		for (int i= 0; i < width_; ++i)	// copy one row of chars into 'line' buffer
		{
			TCHAR c= *src++;
			if (c == 0)
				*dst++ = ' ';
			else if (c >= 0x80 && c < 0xa0)
			{
				*dst++ = ' ';
				special = true;
			}
			else
				*dst++ = c;
		}
		line.ReleaseBuffer(width_);
		dc.TextOut(0, pos_y, line);		// print text

		if (special)	// special characters
		{
			for (int i= 0, pos_x= 0; i < width_; ++i, pos_x += char_w_)
			{
				switch (*save++)
				{
				case 0x80:	// '\'
					dc.MoveTo(pos_x, pos_y);
					dc.LineTo(pos_x + char_w_, pos_y + char_h_);
					break;

				case 0x81:	// '/'
					dc.MoveTo(pos_x + char_w_, pos_y);
					dc.LineTo(pos_x, pos_y + char_h_);
					break;
				}
			}
		}
	}

	dc.SelectObject(old);

	dc.BitBlt();

	DrawCursor();
}

//=============================================================================

void IOWindow::OnDestroy()
{
	CRect rect;
	GetWindowRect(&rect);
	wnd_pos_ = rect.TopLeft();	// remember window's position

	timer_.Stop();

	CMiniFrameWnd::OnDestroy();
}

//=============================================================================

void IOWindow::PostNcDestroy()
{
	//	CMiniFrameWnd::PostNcDestroy();
	m_hWnd = nullptr;
}

//=============================================================================

void IOWindow::OnGetMinMaxInfo(MINMAXINFO* MMI)
{
	CMiniFrameWnd::OnGetMinMaxInfo(MMI);

	CRect size(0, 0, char_w_ * width_, char_h_ * height_);
	CalcWindowRect(&size, CWnd::adjustOutside);
	int w= size.Width();
	if (GetStyle() & WS_VSCROLL)
		w += ::GetSystemMetrics(SM_CXVSCROLL);
	int h= size.Height();
	if (GetStyle() & WS_HSCROLL)
		h += ::GetSystemMetrics(SM_CYHSCROLL);

	MMI->ptMaxSize.x = w;
	MMI->ptMaxSize.y = h;
	MMI->ptMaxTrackSize.x = w;
	MMI->ptMaxTrackSize.y = h;
}

//=============================================================================

void IOWindow::OnSize(UINT type, int cx, int cy) 
{
	CMiniFrameWnd::OnSize(type, cx, cy);

	if (type == SIZE_RESTORED)
	{
		int w= (GetStyle() & WS_VSCROLL) ? ::GetSystemMetrics(SM_CXVSCROLL) : 0;
		int h= (GetStyle() & WS_HSCROLL) ? ::GetSystemMetrics(SM_CYHSCROLL) : 0;
		CRect rect(0, 0, char_w_ * width_, char_h_ * height_);
		CSize size(rect.Width(), rect.Height());
		bool remove= cx + w >= size.cx && cy + h >= size.cy;
		SCROLLINFO si_horz=
		{
			sizeof si_horz,
			SIF_PAGE | SIF_RANGE,
			0, size.cx - 1,		// min & max
			remove ? size.cx : cx,
			0, 0
		};
		SCROLLINFO si_vert=
		{
			sizeof si_vert,
			SIF_PAGE | SIF_RANGE,
			0, size.cy - 1,		// min & max
			remove ? size.cy : cy,
			0, 0
		};
		SetScrollInfo(SB_HORZ, &si_horz);
		SetScrollInfo(SB_VERT, &si_vert);
	}
	else if (type == SIZE_MAXIMIZED)
		cx = 0;
}

//-----------------------------------------------------------------------------

void IOWindow::SetColors(COLORREF text, COLORREF backgnd)
{
	rgb_backgnd_color_ = backgnd;
	rgb_text_color_ = text;
	if (m_hWnd)
		PostMessage(WM_USER);
}

void IOWindow::GetColors(COLORREF& text, COLORREF& backgnd)
{
	text = rgb_text_color_;
	backgnd = rgb_backgnd_color_;
}

//=============================================================================

//afx_msg LRESULT IOWindow::OnStartDebug(WPARAM /*wParam*/, LPARAM /* lParam */)
//{
//	VERIFY( Cls() );
//	if (!hidden_)		// okno by³o widoczne?
//		if (m_hWnd)
//			ShowWindow(SW_NORMAL);
//		else
//			Create();
//	return 1;
//}
//
//
//afx_msg LRESULT IOWindow::OnExitDebug(WPARAM /*wParam*/, LPARAM /* lParam */)
//{
//	if (m_hWnd && (GetStyle() & WS_VISIBLE))	// okno aktualnie wyœwietlone?
//	{
//		hidden_ = FALSE;				// info - okno by³o wyœwietlane
//		ShowWindow(SW_HIDE);			// ukrycie okna
//	}
//	else
//		hidden_ = TRUE;				// info - okno by³o ukryte
//	return 1;
//}

//=============================================================================

afx_msg LRESULT IOWindow::OnCls(WPARAM /*wParam*/, LPARAM /* lParam */)
{
	VERIFY( Cls() );
	return 1;
}


afx_msg LRESULT IOWindow::OnPutC(WPARAM wParam, LPARAM lParam)
{
	if (lParam == 0)
		VERIFY( PutC(int(UINT8(wParam))) == 0 );
	else if (lParam == 1)
		VERIFY( PutChr(int(UINT8(wParam))) == 0 );
	else if (lParam == 2)
		VERIFY( PutH(int(UINT8(wParam))) == 0 );
	else
	{ ASSERT(false); }
	return 1;
}


afx_msg LRESULT IOWindow::OnInput(WPARAM /*wParam*/, LPARAM /* lParam */)
{
	return Input();
}


afx_msg LRESULT IOWindow::OnSizeCmd(WPARAM wParam, LPARAM lParam)
{
	bool width= !!(wParam & 1);

	if (wParam & 2)	// read?
		return width ? GetTerminalWidth() : GetTerminalHeight();
	else
		width ? SetTerminalWidth(static_cast<int>(lParam)) : SetTerminalHeight(static_cast<int>(lParam));

	return 0;
}


afx_msg LRESULT IOWindow::OnPosition(WPARAM wParam, LPARAM lParam)
{
	bool X_pos= !!(wParam & 1);

	if (wParam & 2)	// get pos?
	{
		return X_pos ? pos_x_ : pos_y_;
	}
	else				// set pos
	{
		int x= pos_x_;
		int y= pos_y_;

		if (X_pos)
			x = static_cast<int>(lParam);
		else
			y = static_cast<int>(lParam);

		if (x >= width_)
			x = width_ - 1;
		if (y >= height_)
			y = height_ - 1;

		if (x != pos_x_ || y != pos_y_)
		{
			if (cursor_visible_ && cursor_on_)
				DrawCursor(pos_x_, pos_y_, false);

			pos_x_ = x;
			pos_y_ = y;

			if (cursor_visible_ && cursor_on_)
				DrawCursor(pos_x_, pos_y_, true);
		}
	}

	return 0;
}


afx_msg LRESULT IOWindow::OnGetChannelPtr(WPARAM, LPARAM)
{
	IOChannel* chnl= this;
	return reinterpret_cast<LRESULT>(chnl);
}

//=============================================================================

void IOWindow::HideCursor()
{
	if (cursor_visible_)
	{
		DrawCursor(pos_x_, pos_y_, false);
		cursor_visible_ = false;
	}
	cursor_on_ = false;
}

// draw cursor
//
void IOWindow::DrawCursor()
{
	if (cursor_on_)
		DrawCursor(pos_x_, pos_y_, cursor_visible_);
}


void IOWindow::DrawCursor(int x, int y, bool visible)
{
	if (data_.get() == nullptr)
		return;
	if (x > width_ || y > height_ || x < 0 || y < 0)
	{
		ASSERT(false);
		return;
	}

	// character under the cursor
	TCHAR buf[2]= { data_[x + y * width_], '\0' };
	if (buf[0] == '\0')
		buf[0] = ' ';

	CClientDC dc(this);

	dc.SelectObject(&font_);
	dc.SetBkMode(OPAQUE);

	if (visible)
	{
		dc.SetTextColor(rgb_backgnd_color_);
		dc.SetBkColor(rgb_text_color_);
	}
	else
	{
		dc.SetTextColor(rgb_text_color_);
		dc.SetBkColor(rgb_backgnd_color_);
	}

	// cursor pos & size
	CPoint pos(x * char_w_, y * char_h_);
	CRect rect(pos, CSize(char_w_, char_h_));

	dc.DrawText(buf, 1, rect, DT_TOP | DT_LEFT | DT_NOPREFIX | DT_SINGLELINE);
}


void IOWindow::OnTimer(UINT_PTR id_event)
{
	if (timer_.Id() == id_event)
	{
		cursor_visible_ = !cursor_visible_;

		DrawCursor();

		if (!cursor_visible_)
			cursor_on_ = false;
	}
	else
		CMiniFrameWnd::OnTimer(id_event);
}


void IOWindow::OnChar(UINT chr, UINT rep_cnt, UINT flags)
{
	char c= char(chr);
	if (c)
		input_buffer_.PutChar(c);
}


BOOL IOWindow::ContinueModal()
{
	if (theApp.global_.GetDebugger().IsStopped())	// execution stopped?
		return false;

	return CMiniFrameWnd::ContinueModal();
}


void IOWindow::OnClose()
{
	ShowWindow(SW_HIDE);
}


void IOWindow::Paste()
{
	if (!::IsClipboardFormatAvailable(CF_TEXT))
		return;

	if (!OpenClipboard())
		return;

	if (HANDLE glb= ::GetClipboardData(CF_TEXT))
	{
		if (VOID* str= ::GlobalLock(glb))
		{
			input_buffer_.Paste(reinterpret_cast<char*>(str));
			GlobalUnlock(glb);
		}
	}

	CloseClipboard();
}

///////////////////////////////////////////////////////////////////////////////

char InputBuffer::GetChar()		// get next available character (returns 0 if there are no chars)
{
	char c= 0;

	if (head_ != tail_)
	{
		c = *tail_++;
		if (tail_ >= buffer_ + BUF_SIZE)
			tail_ = buffer_;
	}

	return c;
}

void InputBuffer::PutChar(char c)	// places char in the buffer (char is ignored if there is no space)
{
	char* next= head_ + 1;

	if (next >= buffer_ + BUF_SIZE)
		next = buffer_;

	if (next != tail_)	// is there a place in buffer?
	{
		*head_ = c;
		head_ = next;
	}
}


void InputBuffer::Paste(const char* text)
{
	int max= std::min<int>(static_cast<int>(strlen(text)), BUF_SIZE);

	for (int i= 0; i < max; ++i)
		PutChar(text[i]);
}

///////////////////////////////////////////////////////////////////////////////

void IOWindow::OnKeyDown(UINT chr, UINT rep_cnt, UINT flags)
{
	if (chr == VK_INSERT)
		Paste();
	else
		CMiniFrameWnd::OnKeyDown(chr, rep_cnt, flags);
}


BOOL IOWindow::PreTranslateMessage(MSG* msg)
{
	if (GetFocus() == this)
	{
		if (msg->message == WM_KEYDOWN || msg->message == WM_KEYUP)
		{
			if (msg->wParam >= VK_SPACE && msg->wParam <= 'Z')
			{
				if (::GetKeyState(VK_CONTROL) < 0 && ::GetKeyState(VK_SHIFT) >= 0)
				{
					// skip the rest of PreTranslateMessage() functions, cause they will
					// eat some of those messages (as accel shortcuts); translate and
					// dispatch them now

					::TranslateMessage(msg);
					::DispatchMessage(msg);
					return true;
				}
			}
		}
	}

	return CMiniFrameWnd::PreTranslateMessage(msg);
}


void IOWindow::OnContextMenu(CWnd* wnd, CPoint point)
{
	CMenu menu;
	if (!menu.LoadMenu(IDR_POPUP_TERMINAL))
		return;
	CMenu* popup= menu.GetSubMenu(0);
	ASSERT(popup != nullptr);
	if (popup == nullptr)
		return;

	if (point.x == -1 && point.y == -1)		// keyboard invoked?
	{
		CRect rect;
		GetClientRect(rect);
		ClientToScreen(rect);
		point = rect.CenterPoint();
	}

	popup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this);
}


void IOWindow::OnPaste()
{
	Paste();
}

///////////////////////////////////////////////////////////////////////////////

void IOWindow::Clear()
{
	Cls();
}

void IOWindow::PutChar(int chr)
{
	PutC(chr);
}

int IOWindow::GetChar()
{
	return Input();
}

int IOWindow::GetCursorXPos()
{
	return pos_x_;
}

int IOWindow::GetCursorYPos()
{
	return pos_y_;
}

void IOWindow::SetCursorXPos(int x)
{
	OnPosition(1, x);
}

void IOWindow::SetCursorYPos(int y)
{
	OnPosition(0, y);
}

int IOWindow::GetTerminalWidth()
{
	return width_;
}

int IOWindow::GetTerminalHeight()
{
	return height_;
}

void IOWindow::SetTerminalWidth(int width)
{
	if (width > 1 && width <= 1000)
		SetSize(width, height_);
}

void IOWindow::SetTerminalHeight(int height)
{
	if (height > 1 && height <= 1000)
		SetSize(width_, height);
}


void IOWindow::ApplySettings(SettingsSection& settings)
{
	auto t= settings.section("simulator.terminal");

	log_font_ = t.get_font("font");
	log_font_.lfPitchAndFamily = FIXED_PITCH;

	font_.DeleteObject();
	font_.CreateFontIndirect(&log_font_);

	SetColors(t.get_color("text"), t.get_color("backgnd"));

	auto w= t.get_int("width");
	auto h= t.get_int("height");

	if (w != width_ || h != height_)
		SetSize(w, h);
	else
		Resize(false);	// resize window to adjust to a new font
}
