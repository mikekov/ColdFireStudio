/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "MemoryWnd.h"
#include "Settings.h"
#include "Debugger.h"
extern Debugger& GetDebugger();


MemoryWnd::MemoryWnd() : base_addr_(0), bar_(view_), SettingsClient(L"Memory window")
{}

MemoryWnd::~MemoryWnd()
{}

enum { HEX_VIEW_ID= 111 };

BEGIN_MESSAGE_MAP(MemoryWnd, CSizingControlBarCF)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_NOTIFY(HVN_CHANGED, HEX_VIEW_ID, &MemoryWnd::OnMemoryChange)
	ON_NOTIFY(HVN_EDITMODE_CHANGE, HEX_VIEW_ID, &MemoryWnd::OnEditModeChange)
	ON_NOTIFY(HVN_READ_DATA, HEX_VIEW_ID, &MemoryWnd::OnReadMemory)
END_MESSAGE_MAP()


int MemoryWnd::OnCreate(LPCREATESTRUCT create_struct)
{
	Base::OnCreate(create_struct);

	SetSCBStyle(GetSCBStyle() | SCBS_SIZECHILD | SCBS_SHOWEDGES);

	if (!bar_.Create(this))
		return -1;

	auto mem= GetDebugger().GetMemoryBankInfo(0);
	base_addr_ = mem.Base();

//	auto& data= GetDebugger().GetRamRawPointer();

/*
	HFONT hfont= static_cast<HFONT>(::GetStockObject(DEFAULT_GUI_FONT));
	LOGFONT lf;
	::GetObject(hfont, sizeof(lf), &lf);
	lf.lfPitchAndFamily = FIXED_PITCH;
	lf.lfHeight = -13;
	_tcscpy(lf.lfFaceName, L"Bitstream Vera Sans Mono");
	font_.CreateFontIndirect(&lf);
*/
	CRect rect(0,0,0,0);
	GetClientRect(rect);
	if (!view_.Create(this, rect, nullptr, WS_CHILD | WS_VISIBLE))
		return -1;

	CallApplySettings();

	view_.SetDlgCtrlID(HEX_VIEW_ID);
	view_.SetStyle(~0u, HVS_ENDIAN_BIG | HVS_FITTOWINDOW | HVS_FORCE_FIXEDCOLS | HVS_ADDR_MIDCOLON | HVS_REPLACESEL | HVS_ALWAYSVSCROLL | HVS_ASCII_SHOWCTRLS | HVS_ASCII_SHOWEXTD | HVS_DISABLE_UNDO | HVS_FIXED_EDITMODE /*| HVS_ALWAYSDELETE*/);
	view_.SetFont(&font_);
	view_.SetBaseAddress(base_addr_);
	view_.PassData(nullptr /*data.data()*/, 0 /*static_cast<ULONG>(data.size())*/);
	view_.SetGrouping(1);
	view_.SetEditMode(HVMODE_OVERWRITE);
	view_.SetPadding(2, 2);
	auto gray= ::GetSysColor(COLOR_GRAYTEXT);
	view_.SetColor(HVC_ADDRESS, gray);
	view_.SetColor(HVC_ASCII, gray);
	auto text= ::GetSysColor(COLOR_WINDOWTEXT);
	view_.SetColor(HVC_HEXODD, text);
	view_.SetColor(HVC_HEXEVEN, text);

	return 0;
}


void MemoryWnd::OnSize(UINT type, int cx, int cy)
{
	// bypass CSizingControlBar resizing
	CWnd::OnSize(type, cx, cy);

	Resize();
}


void MemoryWnd::Resize()
{
	if (view_.m_hWnd == nullptr || bar_.m_hWnd == nullptr)
		return;

	CRect rect(0,0,0,0);
	GetClientRect(rect);

	CRect bar(0,0,0,0);
	bar_.GetWindowRect(bar);

	auto h= bar.Height();

	bar_.SetWindowPos(nullptr, 0, 0, rect.Width(), h, SWP_NOZORDER | SWP_NOACTIVATE);

	view_.SetWindowPos(nullptr, 0, h, rect.Width(), std::max(0, rect.Height() - h), SWP_NOZORDER | SWP_NOACTIVATE);
}


BOOL MemoryWnd::PreCreateWindow(CREATESTRUCT& cs)
{
	bool pre_created = CSizingControlBarCF::PreCreateWindow(cs);
	cs.dwExStyle |= WS_EX_CONTROLPARENT;
	cs.style |= WS_CLIPCHILDREN;
	return pre_created;
}


void MemoryWnd::OnMemoryChange(NMHDR* hdr, LRESULT* result)
{
	auto chg= reinterpret_cast<NMHVCHANGED*>(hdr);
	*result = 0;
	auto length= static_cast<size_t>(chg->length);
	auto address= memory_.Base() + static_cast<cf::uint32>(chg->offset);

	// commit changes
	try
	{
		if (chg->data)
			memory_.Write(chg->data, static_cast<cf::uint32>(length), address);
		else if (chg->method == HVMETHOD_DELETE || chg->method == HVMETHOD_OVERWRITE)
			memory_.Clear(static_cast<cf::uint32>(length), address);
	}
	catch (MemoryAccessException&)
	{
		// ignore non-memory areas
		MessageBeep(UINT(-1));
	}
}

// reject edit mode changes
void MemoryWnd::OnEditModeChange(NMHDR* hdr, LRESULT* result)
{
	if (view_ && view_.GetEditMode() == HVMODE_INSERT)
		view_.SetEditMode(HVMODE_OVERWRITE);
}


void MemoryWnd::SetSource(MemorySource& mem)
{
	memory_ = mem;

	//TODO
	//view_
}


void MemoryWnd::Notify(int event, UINT data, Debugger& debugger)
{
	//todo
	// set current address

	view_.Invalidate();

	auto read_only= memory_.ReadOnly();

	if (read_only)
		view_.SetEditMode(HVMODE_READONLY);
	else
		view_.SetEditMode(HVMODE_OVERWRITE);

	bar_.SetReadOnly(read_only);
	view_.SetColor(HVC_BACKGROUND, ::GetSysColor(read_only ? COLOR_BTNFACE : COLOR_WINDOW));

	bar_.Notify(event, data, debugger);
}


void MemoryWnd::WatchRegister(cf::Register reg)
{
	bar_.WatchRegister(reg);
}

void MemoryWnd::SetGrouping(int g)
{
	bar_.SetGrouping(g);
}

void MemoryWnd::SetHexOnly()
{
	bar_.SetHexOnly();
}


void MemoryWnd::ApplySettings(SettingsSection& settings)
{
	auto font= settings.get_font("general.default_font");

	font.lfPitchAndFamily = FIXED_PITCH;

	font_.DeleteObject();
	font_.CreateFontIndirect(&font);

	view_.SetFont(&font_);
	view_.Invalidate();
}


void MemoryWnd::OnReadMemory(NMHDR* hdr, LRESULT* result)
{
	NMHREADDATA& read= *static_cast<NMHREADDATA*>(hdr);

	auto& dbg= GetDebugger();

	*result = dbg.ReadMemory(read.dest_buf, static_cast<cf::uint32>(read.offset), static_cast<cf::uint32>(read.length));
}
