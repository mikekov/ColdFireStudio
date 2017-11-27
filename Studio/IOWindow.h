/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

// IOWindow.h : Simplistic terminal window

#ifndef _io_window_
#define _io_window_

#include "Defs.h"
#include "Broadcast.h"
#include "IOChannel.h"
#include "SettingsClient.h"
#include "WndTimer.h"

/////////////////////////////////////////////////////////////////////////////
// IOWindow frame

class InputBuffer
{
public:
	InputBuffer() : head_(buffer_), tail_(buffer_)
	{}

	char GetChar();			// get next available character (returns 0 if there are no chars)
	void PutChar(char c);	// places char in the buffer (char is ignored if there is no space)
	void Paste(const char* text);		// paste clipboard text into buffer

private:
	enum { BUF_SIZE= 32 * 1024 };
	char buffer_[BUF_SIZE];
	char* head_;
	char* tail_;
};


class IOWindow : public CMiniFrameWnd, IOChannel, SettingsClient
{
public:
	IOWindow();

	enum Commands						// terminal commands
	{ CMD_CLS = Broadcast::WM_USER_OFFSET + 100, CMD_PUTC, CMD_PUTS, CMD_IN, CMD_POSITION, CMD_GET_PTR, CMD_SIZE };
	bool Create();
	void SetSize(int w, int h, int resize= 1);
	void GetSize(int& w, int& h);
	void Resize(bool cls);
	void SetWndPos(const POINT& p);
	CPoint GetWndPos();
	void Paste();

	void SetColors(COLORREF text, COLORREF backgnd);
	void GetColors(COLORREF& text, COLORREF& backgnd);

	int PutC(int chr);					// print char
	int PutChr(int chr);				// print char (verbatim, no special treatment for new line)
	int PutS(const char *str, int len= -1);	// print string 
	int PutH(int n);					// print 8-bit hex number
	bool SetPosition(int x, int y);		// set cursor position
	void GetPosition(int& x, int& y);	// read cursor position
	bool ShowCursor(bool visible= true);	// turn cursor on/off
	bool ResetCursor();					// turn cursor on, clear hiding counter
	bool Cls();							// clear window
	int  Input();						// return char entered by user or 0 if there isn't any

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(IOWindow)
	virtual BOOL PreTranslateMessage(MSG* msg);
protected:
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	// Implementation
public:
	virtual ~IOWindow();
protected:
	// Generated message map functions
	//{{AFX_MSG(IOWindow)
	afx_msg void OnPaint();
	afx_msg void OnGetMinMaxInfo(MINMAXINFO* MMI);
	afx_msg void OnSize(UINT type, int cx, int cy);
	afx_msg void OnDestroy();
	afx_msg void OnTimer(UINT_PTR id_event);
	afx_msg void OnChar(UINT chr, UINT rep_cnt, UINT flags);
	afx_msg void OnClose();
	afx_msg void OnKeyDown(UINT chr, UINT rep_cnt, UINT flags);
	afx_msg void OnContextMenu(CWnd* wnd, CPoint point);
	afx_msg void OnPaste();
	//}}AFX_MSG
	virtual BOOL ContinueModal();
	LRESULT OnInvalidate(WPARAM, LPARAM);
	DECLARE_MESSAGE_MAP()

private:
	// Operations
	void CalcFontSize();				// calc size of average character
	void DrawCursor();					// draw cursor
	void DrawCursor(int x, int y, bool visible);
	void HideCursor();					// hide cursor if it's on

	// Attributes
	static CFont font_;
	static LOGFONT log_font_;
	static CPoint wnd_pos_;				// window position
	static int init_w_, init_h_;
	static COLORREF rgb_text_color_, rgb_backgnd_color_;

	// implementation
	std::unique_ptr<UINT8[]> data_;		// window buffer
	int width_, height_;				// window size (columns x rows)
	int char_h_, char_w_;				// character size in pixels
	void RegisterWndClass();
	int pos_x_, pos_y_;					// cursor position
	int cursor_count_;					// cursor hiding counter
	bool cursor_on_;					// flag: cursor on/off
	bool cursor_visible_;				// flag: cursor currently visible
	WndTimer timer_;
	InputBuffer input_buffer_;			// keyboard input buffer

	int put(char chr, int x, int y);
	int puts(const char *str, int len, int x, int y);
	int scroll(int dy);					// scroll window content by 'dy' rows
	int invalidate(int x, int y);		// invalidate character (x,y) rectangle

	afx_msg LRESULT OnCls(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnPutC(WPARAM wParam, LPARAM /* lParam */);
	afx_msg LRESULT OnInput(WPARAM /*wParam*/, LPARAM /* lParam */);
	afx_msg LRESULT OnPosition(WPARAM wParam, LPARAM lParam);
	LRESULT OnSizeCmd(WPARAM wParam, LPARAM lParam);

	// IOChannel implementation
	virtual void Clear();
	virtual void PutChar(int chr);
	virtual int GetChar();
	virtual int GetCursorXPos();
	virtual int GetCursorYPos();
	virtual void SetCursorXPos(int x);
	virtual void SetCursorYPos(int y);
	virtual int GetTerminalWidth();
	virtual int GetTerminalHeight();
	virtual void SetTerminalWidth(int width);
	virtual void SetTerminalHeight(int height);

	afx_msg LRESULT OnGetChannelPtr(WPARAM wParam, LPARAM lParam);

	virtual void ApplySettings(SettingsSection& settings);

	DECLARE_DYNCREATE(IOWindow)
};

#endif
