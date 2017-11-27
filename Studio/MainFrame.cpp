/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

// MainFrm.cpp : implementation of the main application window

#include "pch.h"
#include "MainFrame.h"
#include "Options.h"
#include "SaveCode.h"
#include "LoadCode.h"
#include "LoadCodeDlg.h"
#include "afxpriv.h"	// LoadBarState()
#include "AsmSrcView.h"
#include "AsmSrcDoc.h"
#include "Assembler.h"
#include "App.h"
#include "ProtectedCall.h"
#include "Settings.h"
#include "Utf8.h"
#include "TypeTranslators.h"
#include "Utilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static int IDC_TAB= 1111;

//-----------------------------------------------------------------------------

const HWND* /*const*/ MainFrame::windows_[]= {0,0,0,0,0,0,0,0,0,0};

WNDPROC MainFrame::pfn_old_proc_;
CBitmap MainFrame::code_bmp_;		// StatusBar images
CBitmap MainFrame::debug_bmp_;

extern Debugger& GetDebugger()
{
	return theApp.global_.GetDebugger();
}

//-----------------------------------------------------------------------------

const TCHAR REG_ENTRY_CONTROL_BARS[]= _T("CtrlBars\\BarState");
const TCHAR MainFrame::REG_ENTRY_MAINFRM[]= _T("MainFrame");
const TCHAR MainFrame::REG_POSX[]= _T("XPos");
const TCHAR MainFrame::REG_POSY[]= _T("YPos");
const TCHAR MainFrame::REG_SIZX[]= _T("Width");
const TCHAR MainFrame::REG_SIZY[]= _T("Height");
const TCHAR MainFrame::REG_STATE[]= _T("Maximize");


void MainFrame::ConfigSettings(bool load)
{
	//TODO
	// retire registry settings

	static const TCHAR ENTRY_SIM[]= _T("Simulator");
	static const TCHAR SIM_WND_X[]= _T("TerminalXPos");
	static const TCHAR SIM_WND_Y[]= _T("TerminalYPos");
	static const TCHAR ENTRY_VIEW[]= _T("View");

	CWinApp* app = AfxGetApp();
	auto& t= AppSettings().section("simulator.terminal");
	//todo: subscribe to changes

	if (load)		// loading?
	{
		CPoint pos;
		pos.x = app->GetProfileInt(ENTRY_SIM, SIM_WND_X, 200);
		pos.y = app->GetProfileInt(ENTRY_SIM, SIM_WND_Y, 200);
		io_window_.SetWndPos(pos);
	}
	else			// saving
	{
		CPoint pos= io_window_.GetWndPos();
		app->WriteProfileInt(ENTRY_SIM, SIM_WND_X, pos.x);
		app->WriteProfileInt(ENTRY_SIM, SIM_WND_Y, pos.y);

		CSizingControlBar::GlobalSaveState(this, REG_ENTRY_CONTROL_BARS);
		SaveBarState(REG_ENTRY_CONTROL_BARS);
	}
}

/////////////////////////////////////////////////////////////////////////////
// MainFrame

const static int WM_GET_TERMINAL= WM_APP + 10;


IMPLEMENT_DYNAMIC(MainFrame, CMDIFrameWnd)

BEGIN_MESSAGE_MAP(MainFrame, CMDIFrameWnd)
	ON_WM_CREATE()
	ON_WM_CLOSE()
	ON_WM_ERASEBKGND()
	ON_COMMAND(ID_SIM_CHECK_PROG, OnCheckProgram)
	ON_UPDATE_COMMAND_UI(ID_SIM_CHECK_PROG, OnUpdateCheckProgram)
	ON_COMMAND(ID_SIM_ASSEMBLE, OnAssemble)
	ON_UPDATE_COMMAND_UI(ID_SIM_ASSEMBLE, OnUpdateAssemble)
	ON_COMMAND(ID_SIM_STEP_INTO, OnStepInto)
	ON_UPDATE_COMMAND_UI(ID_SIM_STEP_INTO, OnUpdateStepInto)
	ON_COMMAND(ID_SIM_SKIP_INSTR, OnSkipInstr)
	ON_UPDATE_COMMAND_UI(ID_SIM_SKIP_INSTR, OnUpdateSkipInstr)
	ON_COMMAND(ID_SIM_BREAKPOINT, OnSimBreakpoint)
	ON_UPDATE_COMMAND_UI(ID_SIM_BREAKPOINT, OnUpdateSimBreakpoint)
	ON_COMMAND(ID_SIM_BREAK, OnSimBreak)
	ON_UPDATE_COMMAND_UI(ID_SIM_BREAK, OnUpdateSimBreak)
	ON_COMMAND(ID_SIM_GO, OnSimGo)
	ON_UPDATE_COMMAND_UI(ID_SIM_GO, OnUpdateSimGo)
	ON_COMMAND(ID_SIM_OPTIONS, OnOptions)
	ON_UPDATE_COMMAND_UI(ID_SIM_OPTIONS, OnUpdateOptions)
	ON_COMMAND(ID_SIM_GO_TO_CURSOR, OnSimRunToLine)
	ON_UPDATE_COMMAND_UI(ID_SIM_GO_TO_CURSOR, OnUpdateSimRunToLine)
	ON_COMMAND(ID_SIM_SKIP_TO_LINE, OnSimSkipToLine)
	ON_UPDATE_COMMAND_UI(ID_SIM_SKIP_TO_LINE, OnUpdateSimSkipToLine)
	ON_COMMAND(ID_SIM_STEP_OUT, OnSimStepOut)
	ON_UPDATE_COMMAND_UI(ID_SIM_STEP_OUT, OnUpdateSimStepOut)
	ON_COMMAND(ID_SIM_STEP_OVER, OnStepOver)
	ON_UPDATE_COMMAND_UI(ID_SIM_STEP_OVER, OnUpdateStepOver)
	ON_COMMAND(ID_SIM_STEP_INTO_EXCP, OnStepIntoExcp)
	ON_UPDATE_COMMAND_UI(ID_SIM_STEP_INTO_EXCP, OnUpdateStepIntoExcp)
	ON_COMMAND(ID_SIM_EDIT_BREAKPOINT, OnSimEditBreakpoint)
	ON_UPDATE_COMMAND_UI(ID_SIM_EDIT_BREAKPOINT, OnUpdateSimEditBreakpoint)
	ON_COMMAND(ID_SIM_RESTART, OnSimRestart)
	ON_UPDATE_COMMAND_UI(ID_SIM_RESTART, OnUpdateSimRestart)
	ON_COMMAND(ID_FILE_SAVE_CODE, OnFileSaveCode)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE_CODE, OnUpdateFileSaveCode)
	ON_COMMAND(ID_VIEW_DEASM, OnViewDeasm)
	ON_UPDATE_COMMAND_UI(ID_VIEW_DEASM, OnUpdateViewDeasm)
	ON_COMMAND(ID_VIEW_IDENT, OnViewIdents)
	ON_UPDATE_COMMAND_UI(ID_VIEW_IDENT, OnUpdateViewIdents)
	ON_COMMAND(ID_VIEW_IO_WINDOW, OnViewIOWindow)
	ON_UPDATE_COMMAND_UI(ID_VIEW_IO_WINDOW, OnUpdateViewIOWindow)
	ON_WM_DESTROY()
	ON_COMMAND(ID_FILE_OPEN_CODE, OnFileLoadCode)
	ON_UPDATE_COMMAND_UI(ID_FILE_OPEN_CODE, OnUpdateFileLoadCode)
	ON_WM_SYSCOLORCHANGE()
	ON_WM_TIMER()
	ON_COMMAND(ID_VIEW_STACK, OnViewStack)
	ON_UPDATE_COMMAND_UI(ID_VIEW_STACK, OnUpdateViewStack)
	ON_UPDATE_COMMAND_UI(ID_VIEW_CPU_BAR, OnUpdateViewCpuBar)
	ON_COMMAND(ID_VIEW_CPU_BAR, OnViewCpuBar)
	ON_COMMAND(ID_VIEW_LOG, OnViewLog)
	ON_UPDATE_COMMAND_UI(ID_VIEW_LOG, OnUpdateViewLog)
	// Global help commands
	ON_COMMAND(ID_HELP_FINDER, &MainFrame::OnShowHelp)
	//ON_COMMAND(ID_HELP, &MainFrame::OnShowHelp)
	//  ON_COMMAND(ID_CONTEXT_HELP, CMDIFrameWnd::OnContextHelp)
	//ON_COMMAND(ID_DEFAULT_HELP, &MainFrame::OnShowHelp)
	ON_COMMAND_RANGE(ID_VIEW_MEMORY_1, ID_VIEW_MEMORY_4, &MainFrame::OnViewMemory)
	ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_MEMORY_1, ID_VIEW_MEMORY_4, &MainFrame::OnUpdateViewMemory)
	ON_MESSAGE(Broadcast::WM_APP_STATE_CHANGED, &MainFrame::OnExecEvent)
	ON_MESSAGE(WM_APP, &MainFrame::OnMDIRefresh)
	ON_MESSAGE(WM_GET_TERMINAL, &MainFrame::OnGetTerminalWnd)
	ON_NOTIFY(CTCN_SELCHANGE, IDC_TAB, &MainFrame::OnTabSelected)
	ON_MESSAGE(WM_IDLEUPDATECMDUI, &MainFrame::OnIdleUpdateCmdUI)
	ON_COMMAND_RANGE(ID_VIEW_FIRST, ID_VIEW_LAST, OnViewDisplayWindow)
	ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_FIRST, ID_VIEW_LAST, OnUpdateViewDisplayWindow)
END_MESSAGE_MAP()


static UINT indicators[]=
{
	ID_SEPARATOR,           // status line indicator
	0,
	0,
	ID_EDIT_INDICATOR_POSITION,
	ID_ISA,
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
};

enum	// status bar part numbers
{
	PART_STATUS,
	PART_SPACER,
	PART_CODE_BMP,
	PART_LINE_COL,
	PART_ISA,
};

/////////////////////////////////////////////////////////////////////////////
// MainFrame construction/destruction

MainFrame::MainFrame() : SettingsClient(L"Debugger"), led_7seg_wnd_(IDB_LED_7, 37), led_16seg_wnd_(IDB_LED_16, 49)
{
	int i= 0;
	windows_[i++] = &m_hWnd;
	windows_[i++] = &io_window_.m_hWnd;
	windows_[i++] = nullptr;

	auto_open_source_ = true;
	auto_open_disasm_ = true;

	last_refresh_time_ = 0;
}

MainFrame::~MainFrame()
{}

//-----------------------------------------------------------------------------

bool MainFrame::InitDisplayWindow(DisplayDlg& wnd, CString title, int id, PeripheralDevice* device)
{
	auto display_device= dynamic_cast<DisplayDevice*>(device);

	if (display_device == nullptr)
		return true;	// not present in a config; that's OK, window will be disabled

	wnd.SetDimensions(CSize(display_device->GetWidth(), display_device->GetHeight()));

	if (!wnd.Create(title, this, id))
		return false;

	wnd.SetBarStyle(cpu_wnd_.GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_FIXED);
	wnd.EnableDocking(CBRS_ALIGN_TOP | CBRS_ALIGN_BOTTOM);

	display_map_[device] = DispInfo(&wnd, display_device);

	return true;
}


//bool MainFrame::InitLEDDisplayWindow(LEDSegmentsDlg& wnd, CString title, int id, PeripheralDevice* led_display)
//{
//	if (led_display == nullptr)
//		return true;
//
//	wnd.SetDimensions(CSize(led_display->IOAreaSize() / sizeof(cf::uint32), 1));
//
//	if (!wnd.Create(title, this, id))
//		return false;
//
//	wnd.SetBarStyle(cpu_wnd_.GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_FIXED);
//	wnd.EnableDocking(CBRS_ALIGN_TOP | CBRS_ALIGN_BOTTOM);
//
//	display_map_[led_display] = DispInfo(&wnd);
//
//	return true;
//}


const DWORD dock_bar_map_ex[4][2] =
{
	{ AFX_IDW_DOCKBAR_TOP,      CBRS_TOP    },
	{ AFX_IDW_DOCKBAR_BOTTOM,   CBRS_BOTTOM },
	{ AFX_IDW_DOCKBAR_LEFT,     CBRS_LEFT   },
	{ AFX_IDW_DOCKBAR_RIGHT,    CBRS_RIGHT  },
};


int MainFrame::OnCreate(LPCREATESTRUCT create_struct)
{
	if (CMDIFrameWnd::OnCreate(create_struct) == -1)
		return -1;

	ModifyStyleEx(m_hWndMDIClient, WS_EX_CLIENTEDGE, 0, SWP_FRAMECHANGED);

	GetDebugger().SetMainWnd(this, WM_GET_TERMINAL);

	m_pFloatingFrameClass = RUNTIME_CLASS(CSCBMiniDockFrameWnd);

	if (!tool_bar_wnd_.Create(this) ||
		!tool_bar_wnd_.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}
	CString name;
	VERIFY(name.LoadString(IDS_TOOLBAR));
	tool_bar_wnd_.SetWindowText(name);

	if (!status_bar_wnd_.Create(this) ||
		!status_bar_wnd_.SetIndicators(indicators, sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}

	{	// subclass status bar
		UINT id;
		UINT style;
		int width;

		if (format_.LoadString(IDS_ROW_COLUMN))
		{
			status_bar_wnd_.GetPaneInfo(PART_SPACER, id, style, width);
			status_bar_wnd_.SetPaneInfo(PART_SPACER, id, SBPS_NOBORDERS | SBPS_DISABLED, 1);
		}

		status_bar_wnd_.GetPaneInfo(PART_CODE_BMP, id, style, width);
		status_bar_wnd_.SetPaneInfo(PART_CODE_BMP, id, style, 16);	// make space for status image

		code_bmp_.LoadMappedBitmap(IDB_CODE);
		debug_bmp_.LoadMappedBitmap(IDB_DEBUG);

		pfn_old_proc_ = reinterpret_cast<WNDPROC>(
			::SetWindowLongPtr(status_bar_wnd_.m_hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&MainFrame::StatusBarWndProc)));
	}

	// TODO: Remove this if you don't want tool tips or a resizeable toolbar
	tool_bar_wnd_.SetBarStyle(tool_bar_wnd_.GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);

	EnableDocking(CBRS_ALIGN_ANY);
	AdjustDockBarStyles();
#ifdef _SCB_REPLACE_MINIFRAME
	m_pFloatingFrameClass = RUNTIME_CLASS(CSCBMiniDockFrameWnd);
#endif //_SCB_REPLACE_MINIFRAME

	// TODO: Delete these three lines if you don't want the toolbar to be dockable
	tool_bar_wnd_.EnableDocking(CBRS_ALIGN_ANY);
	//	EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&tool_bar_wnd_);

	{
		if (!tab_ctrl_.Create(WS_CHILD | WS_VISIBLE | CTCS_AUTOHIDEBUTTONS | CTCS_TOOLTIPS | CTCS_DRAGMOVE | CTCS_DRAGCOPY | CTCS_BUTTONSAFTER | CTCS_TOP | CTCS_CLOSEBUTTON, CRect(0,0,0,0), this, IDC_TAB))
			return -1;

		tab_ctrl_.SetBarStyle(CBRS_ALIGN_TOP);
		tab_ctrl_.SetBorders();

		auto font= GetFont();
		CDC dc;
		dc.CreateCompatibleDC(0);
		dc.SelectObject(font);
		TEXTMETRIC tm;
		int height= 13;
		if (dc.GetTextMetrics(&tm))
			height = tm.tmHeight + tm.tmExternalLeading;

		tab_ctrl_.SetFont(font);

		tab_ctrl_.SetIdealHeight(height * 17 / 10);

		//	this->m_rectBorder.top = tab_ctrl_.GetIdealHeight();
		tab_ctrl_.EnableDocking(CBRS_ALIGN_ANY);
		DockControlBar(&tab_ctrl_);
	}

	if (!cpu_wnd_.Create(L"CPU Registers", this, 1030))
		return -1;

	cpu_wnd_.SetBarStyle(cpu_wnd_.GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	cpu_wnd_.EnableDocking(CBRS_ALIGN_ANY);
	cpu_wnd_.SetCallback(std::bind(&Debugger::ModifyRegister, std::ref(GetDebugger()), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	DockControlBar(&cpu_wnd_, AFX_IDW_DOCKBAR_RIGHT);

	CControlBar* above= &cpu_wnd_;
	const int mem_id= 1040;
	int i= 0;
	for (auto& wnd : memory_wnd_)
	{
		CString title;
		title.Format(L"Memory %d", i + 1);
		if (!wnd.Create(title, this, mem_id + i++))
			return -1;

		wnd.SetBarStyle(wnd.GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
		wnd.EnableDocking(CBRS_ALIGN_ANY);
		DockBelow(*above, wnd);
		above = &wnd;
		wnd.SetSource(MemorySource(GetDebugger(), 0));	//todo: banks
	}
	//todo: hide some memory windows initially

	if (!call_stack_.Create(L"Call Stack", this, 1050))
		return -1;

	call_stack_.SetBarStyle(call_stack_.GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	call_stack_.EnableDocking(CBRS_ALIGN_ANY);
	DockBelow(*above, call_stack_);
	call_stack_.SetSource(MemorySource(GetDebugger(), 0)); // todo: banks
	call_stack_.WatchRegister(cf::Register::R_SP);
	call_stack_.SetGrouping(2);
	call_stack_.SetHexOnly();

	if (!InitDisplayWindow(led_7seg_wnd_, L"LED 7-Segments", ID_VIEW_LED7, GetDebugger().FindDevice("display", "led_7_segments")))
		return -1;
	if (led_7seg_wnd_.m_hWnd)
		FloatControlBar(&led_7seg_wnd_, CPoint(100, 100));

	if (!InitDisplayWindow(led_16seg_wnd_, L"LED 16-Segments", ID_VIEW_LED16, GetDebugger().FindDevice("display", "led_16_segments")))
		return -1;
	if (led_16seg_wnd_.m_hWnd)
		FloatControlBar(&led_16seg_wnd_, CPoint(100, 200));

 	if (!InitDisplayWindow(lcd_display_wnd_, L"LCD Display", ID_VIEW_LCD_DISPLAY, GetDebugger().FindDevice("display", "simple_lcd")))
		return -1;
	if (lcd_display_wnd_.m_hWnd)
		FloatControlBar(&lcd_display_wnd_, CPoint(100, 300));

	if (VerifyBarState(REG_ENTRY_CONTROL_BARS))
	{
		CSizingControlBar::GlobalLoadState(this, REG_ENTRY_CONTROL_BARS);
		LoadBarState(REG_ENTRY_CONTROL_BARS);
	}

	// read debugger settings
	CallApplySettings();

	// fake notification to refresh all windows that depend on simulator events
	PostMessage(Broadcast::WM_APP_STATE_CHANGED, cf::E_EXEC_STOPPED, -1);

	return 0;
}


void MainFrame::AdjustDockBarStyles()
{
	for (int i= 0; i < 4; i++)
	{
		CDockBar* dock = dynamic_cast<CDockBar*>(GetControlBar(dock_bar_map_ex[i][0]));
		if (dock)
		{
			dock->m_cxDefaultGap = 0;
			dock->SetBarStyle(dock->GetBarStyle() & ~(CBRS_BORDER_3D | CBRS_BORDER_ANY));
		}
	}
}


void MainFrame::DockBelow(CControlBar& first, CControlBar& second)
{
	RecalcLayout();
	CRect rect(0,0,0,0);
	first.GetWindowRect(rect);
	rect.OffsetRect(0, 1);
	DockControlBar(&second, AFX_IDW_DOCKBAR_RIGHT, rect);
}


// This function is Copyright (c) 2000, Cristi Posea.
// See www.datamekanix.com for more control bars tips&tricks.
BOOL MainFrame::VerifyBarState(LPCTSTR profile_name)
{
	CDockState state;
	state.LoadState(profile_name);

	for (int i = 0; i < state.m_arrBarInfo.GetSize(); i++)
	{
		CControlBarInfo* info = (CControlBarInfo*)state.m_arrBarInfo[i];
		ASSERT(info != nullptr);
		if (info != nullptr)
		{
			int docked_count = static_cast<int>(info->m_arrBarID.GetSize());
			if (docked_count > 0)
			{
				// dockbar
				for (int j = 0; j < docked_count; j++)
				{
					UINT id = (UINT) info->m_arrBarID[j];
					if (id == 0) continue; // row separator
					if (id > 0xFFFF)
						id &= 0xFFFF; // placeholder - get the ID
					if (GetControlBar(id) == nullptr)
						return FALSE;
				}
			}

			if (!info->m_bFloating) // floating dockbars can be created later
				if (GetControlBar(info->m_nBarID) == nullptr)
					return FALSE; // invalid bar ID
		}
	}

	return TRUE;
}

//-----------------------------------------------------------------------------

LRESULT CALLBACK MainFrame::StatusBarWndProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CWnd* cwnd= FromHandlePermanent(wnd);

	switch (msg)
	{
	case WM_PAINT:
		{
			LRESULT ret= (*MainFrame::pfn_old_proc_)(wnd, msg, wParam, lParam);
			if (ret == 0)
			{
				if (!GetDebugger().IsActive())
					return ret;

				// active program available?
				auto active= !GetDebugger().IsFinished();
				if (!active)
					return ret;

				CRect rect;
				(*pfn_old_proc_)(wnd, SB_GETRECT, 2, (LPARAM)(RECT*)rect);	// space for image
				int borders[3];
				(*pfn_old_proc_)(wnd, SB_GETBORDERS, 0, (LPARAM)borders);	// border size
				rect.DeflateRect(borders[0] + 1, borders[1] - 1);
				CClientDC dc(cwnd);
				if (dc)
				{
					CDC memDC;
					memDC.CreateCompatibleDC(&dc);
					if (memDC)
					{
						CBitmap* old_bmp= memDC.SelectObject(active ? &debug_bmp_ : &code_bmp_);
						dc.BitBlt(rect.left + 2, rect.top, rect.Width(), rect.Height(), &memDC, 0, 0, SRCCOPY);
						memDC.SelectObject(old_bmp);
					}
				}
			}
			return ret;
		}

	default:
		return (*MainFrame::pfn_old_proc_)(wnd, msg, wParam, lParam);
	}
}

//-----------------------------------------------------------------------------

static void SetPointer(AsmSrcView* view, int line, bool scroll)
{
	if (view == nullptr)
		return;

	CDocument* doc= view->GetDocument();
	POSITION pos= doc->GetFirstViewPosition();
	while (pos != nullptr)
	{
		if (AsmSrcView* src_view= dynamic_cast<AsmSrcView*>(doc->GetNextView(pos)))
			src_view->SetPointer(line, scroll && src_view == view);
	}
}


LRESULT MainFrame::OnExecEvent(WPARAM wevent, LPARAM lline)
{
	ProtectedCall([&]
	{
		auto event= static_cast<int>(wevent);
		auto line= static_cast<int>(lline);

		auto& dbg= GetDebugger();

		if (event != cf::E_RUNNING)
		{
			auto path= dbg.GetCurrentLinePath();
			auto pc= dbg.GetCurrentProgCounter();

			AsmSrcView* dest= nullptr;
			if (!path.empty() && auto_open_source_)
			{
				// activate/open source document
				if (CDocument* doc= AfxGetApp()->OpenDocumentFile(path.c_str(), false))
				{
					auto pos= doc->GetFirstViewPosition();
					if (pos)
						dest = dynamic_cast<AsmSrcView*>(doc->GetNextView(pos));
				}
			}

			if (dest == nullptr && auto_open_disasm_)	// if there is no source, use disassembly view
				if (GetDebugger().GetStatus() != SIM_FINISHED)
					theApp.global_.GetOrCreateDisasmView();

			ForAllViews([&](PointerView* view)
			{
				view->SetPointer(static_cast<int>(line), path, pc, true);
			});

			status_bar_wnd_.SetPaneText(PART_ISA, ISAToString(dbg.GetCurrentIsa()));
			{
				RECT rect;
				status_bar_wnd_.SendMessage(SB_GETRECT, PART_CODE_BMP, (LPARAM)&rect);
				status_bar_wnd_.InvalidateRect(&rect);
			}
		}
		/*
		auto view= GetCurrentView();

		if (line != ~0)	// valid line number?
		{
		if (dest != view)
		SetPointer(view, -1, false);	// hide pointer

		SetPointer(dest, line, true);
		}
		else
		{
		SetPointer(view, -1, false);		// hide pointer

		if (dest != view)
		SetPointer(view, -1, false);	// hide pointer
		}
		*/

		cpu_wnd_.Notify(event, line, GetDebugger());

		for (auto& wnd : memory_wnd_)
			wnd.Notify(event, line, GetDebugger());

		call_stack_.Notify(event, line, GetDebugger());
	}, "Error processing simulator event.");

	return 0;
}


//-----------------------------------------------------------------------------

BOOL MainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	CWinApp* app= AfxGetApp();

	//TODO: support multimon hardware configurations-------------

	CRect desk;
	::SystemParametersInfo(SPI_GETWORKAREA, 0, desk, 0);

	cs.x = app->GetProfileInt(REG_ENTRY_MAINFRM, REG_POSX, 50);
	cs.y = app->GetProfileInt(REG_ENTRY_MAINFRM, REG_POSY, 50);
	cs.cx = app->GetProfileInt(REG_ENTRY_MAINFRM, REG_SIZX, desk.Width() - 100);
	cs.cy = app->GetProfileInt(REG_ENTRY_MAINFRM, REG_SIZY, desk.Height() - 100);

	// prevent from appearing outside desk area
	if (cs.x < desk.left)
		cs.x = desk.left;
	if (cs.y < desk.top)
		cs.y = desk.top;
	if (cs.x + cs.cx > desk.right)
		cs.x = desk.right - std::min(cs.cx, desk.Width());
	if (cs.y + cs.cy > desk.bottom)
		cs.y = desk.bottom - std::min(cs.cy, desk.Height());

	if (app->GetProfileInt(REG_ENTRY_MAINFRM, REG_STATE, 0))	// maximize?
		StudioApp::maximize_ = true;

	ConfigSettings(true);		// read settings

	return CMDIFrameWnd::PreCreateWindow(cs);
}

// MainFrame diagnostics

#ifdef _DEBUG
void MainFrame::AssertValid() const
{
	CMDIFrameWnd::AssertValid();
}

void MainFrame::Dump(CDumpContext& dc) const
{
	CMDIFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// MainFrame message handlers

void MainFrame::OnClose()
{
	if (GetDebugger().IsRunning())
	{
		if (AfxMessageBox(IDS_MAINFRM_PROG_RUNNING, MB_YESNO) != IDYES)
			return;

		GetDebugger().AbortProg();
	}

	CWinApp* app = AfxGetApp();

	WINDOWPLACEMENT wp;
	if (GetWindowPlacement(&wp))
	{						   // save window location
		CRect wnd(wp.rcNormalPosition);
		app->WriteProfileInt(REG_ENTRY_MAINFRM, REG_POSX, wnd.left);
		app->WriteProfileInt(REG_ENTRY_MAINFRM, REG_POSY, wnd.top);
		app->WriteProfileInt(REG_ENTRY_MAINFRM, REG_SIZX, wnd.Width());
		app->WriteProfileInt(REG_ENTRY_MAINFRM, REG_SIZY, wnd.Height());
		app->WriteProfileInt(REG_ENTRY_MAINFRM, REG_STATE, wp.showCmd == SW_SHOWMAXIMIZED ? 1 : 0);
	}

	ConfigSettings(false);		// save settings

	CMDIFrameWnd::OnClose();
}


void MainFrame::OnDestroy() 
{
	timer_.Stop();

	CMDIFrameWnd::OnDestroy();
}

//-----------------------------------------------------------------------------

void MainFrame::Assemble(bool enter_debugger)
{
	AsmSrcView* view= GetCurrentView();
	if (view == nullptr)
		return;
	AsmSrcDoc* doc= view->GetDocument();
	if (doc == nullptr)
		return;

	SendMessageToViews(WM_USER_REMOVE_ERR_MARK);

	auto& dbg= GetDebugger();
	if (enter_debugger)
		dbg.AbortProg();

	if (doc->IsModified() && !doc->DoFileSave())
		return;

	if (doc->IsModified())
		return;

	// before assembly starts set current dir to the document directory,
	// so include directive will find included files
	const CString& path= doc->GetPathName();
	CFileStatus status;
	if (path.IsEmpty() || !CFile::GetStatus(path, status))
	{
		view->SetErrMark(0);	// mark error line
		doc->SetStatusMessage(L"Save source file before assembling it.");
		return;
	}
	auto dir= Path(path).parent_path();
	current_path(dir);

	// assembler settings
	auto& s= AppSettings().section("asm");
	auto case_sensitive= s.get_bool("case_sens");
	auto isa= StringToISA(s.get_string("isa")) | StringToISA(s.get_string("mac"));

	//TODO: start assembly thread

	CWaitCursor wait;

	std::unique_ptr<Assembler> a(new Assembler());

	auto tm= std::chrono::high_resolution_clock::now();

	StatCode asm_stat= a->Assemble(path, isa, case_sensitive);

	auto duration= std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - tm);

	wait.Restore();

	if (asm_stat == OK)
	{
		doc->SetStatusMessage((boost::wformat(L"OK. Assembly Time: %d ms") % duration.count()).str());

		// check if there's some code generated; assembling empty file is not an error, but it yields no code
		if (enter_debugger && a->GetCode().Valid())
		{
			//TODO: wait for simulator to stop running
			//

			// enter debugger
			SetPointer(view, -1, false);	// hide old pointer

			// remove all breakpoints in a simulator; they are code address based and need to be reset after assembly
			dbg.ClearAllBreakpoints();

			dbg.SetDebugInfo(a->TakeOverDebug());

			// exec monitor code to prepare program for run and stop at the beggining of it
			// this call may fail if destination memory is not available
			try
			{
				dbg.SetProgram(a->GetCode(), true);
			}
			catch (std::exception& ex)
			{
				doc->SetStatusMessage((boost::wformat(L"Sending code to simulator failed: %d") % ex.what()).str());
				return;
			}

			// get all breakpoints set in an editor, they are the ground truth for breakpoints
			auto breakpoints= view->GetAllBreakpoints();

			for (auto line : breakpoints)
				if (!dbg.SetBreakpoint(line, path))	// if breakpoint cannot be set, remove it form the editor (TODO: disable it)
					view->RemoveBreakpoint(line);
		}
	}
	else
	{
		if (int line= a->LastLine())
			if (CDocument* doc= AfxGetApp()->OpenDocumentFile(a->GetPath().c_str(), false))
			{
				if (auto pos= doc->GetFirstViewPosition())
					if (auto view= dynamic_cast<AsmSrcView*>(doc->GetNextView(pos)))
						view->SetErrMark(line - 1);		// mark error line
			}

		doc->SetStatusMessage(from_utf8(a->LastMessage()));
	}

	//RefreshStatusMessage();
}


void MainFrame::OnCheckProgram()
{
	ProtectedCall([&]
	{
		Assemble(false);	// check syntax only
	}, "Error checking program.");
}

void MainFrame::OnUpdateCheckProgram(CCmdUI* cmd_ui)
{
	cmd_ui->Enable(GetCurrentDocument() != nullptr);
}


void MainFrame::OnAssemble()
{
	ProtectedCall([&]
	{
		Assemble(true);		// assemble and start debugger
	}, "Error assembling program.");
}


void MainFrame::OnUpdateAssemble(CCmdUI* cmd_ui) 
{
	cmd_ui->Enable(GetCurrentDocument() != nullptr);
}


void MainFrame::GetMessageString(UINT id, CString& message) const
{
	if (id == AFX_IDS_IDLEMESSAGE)
	{
		// if we have assembly status, show it instead of 'Ready'
		if (AsmSrcDoc* doc= GetCurrentDocument())
		{
			message = doc->GetStatusMessage();
			if (!message.IsEmpty())
				return;
		}
	}

	CMDIFrameWnd::GetMessageString(id, message);
}


void MainFrame::RefreshStatusMessage()
{
	SetMessageText(AFX_IDS_IDLEMESSAGE);
}


AsmSrcDoc* MainFrame::GetCurrentDocument() const
{
	if (CMDIChildWnd* wnd= MDIGetActive())
		if (CDocument* doc= wnd->GetActiveDocument())
			if (doc->IsKindOf(RUNTIME_CLASS(AsmSrcDoc)))
				return static_cast<AsmSrcDoc*>(doc);

	return 0;
}

//-----------------------------------------------------------------------------

void MainFrame::UpdateTextPosition(int row, int col, bool insert_mode)
{
	CString position;
	position.Format(format_, row, col);
	status_bar_wnd_.SetPaneText(PART_LINE_COL, position);
	status_bar_wnd_.UpdateWindow();
	//todo: update insert_mode too
}


//-----------------------------------------------------------------------------

void MainFrame::OnSimBreakpoint() 
{
	AsmSrcView* view= GetCurrentView();

	if (!GetDebugger().IsActive() || view == nullptr)
		return;

	if (view->IsKindOf(RUNTIME_CLASS(AsmSrcView)))
	{
		int line= view->GetCurrLineNo();

		if (GetDebugger().ToggleBreakpoint(line, view->GetDocument()->GetPathName()))
			AddBreakpoint(view, line, Defs::BPT_EXECUTE);
		else
			view->RemoveBreakpoint(line);
	}
	else
	{
		ASSERT(false);	// only editor window expected
		return;
	}
}


void MainFrame::OnUpdateSimBreakpoint(CCmdUI* cmd_ui) 
{
	bool enable= HaveCodeAtCurrentLine();
	cmd_ui->Enable(enable);
}


void MainFrame::AddBreakpoint(AsmSrcView* view, int line, Defs::Breakpoint bp)
{
	if (view == 0)
		return;

	CDocument* doc= view->GetDocument();
	POSITION pos= doc->GetFirstViewPosition();
	while (pos != nullptr)
		if (AsmSrcView* src_view= dynamic_cast<AsmSrcView*>(doc->GetNextView(pos)))
			src_view->AddBreakpoint(line, bp);
}

void MainFrame::RemoveBreakpoint(AsmSrcView* view, int line)
{
	if (view == 0)
		return;

	CDocument* doc= view->GetDocument();
	POSITION pos= doc->GetFirstViewPosition();
	while (pos != nullptr)
		if (AsmSrcView* src_view= dynamic_cast<AsmSrcView*>(doc->GetNextView(pos)))
			src_view->RemoveBreakpoint(line);
}


bool MainFrame::HaveCodeAtCurLine(int delta)
{
	bool present= false;
	AsmSrcView* view= dynamic_cast<AsmSrcView*>(GetActiveFrame()->GetActiveView());

	if (GetDebugger().IsActive() && view != nullptr)
	{
		int line= view->GetCurrLineNo() + delta;
		present = GetDebugger().GetCodeAddress(line, view->GetDocument()->GetPathName()).is_initialized();
	}

	return present;
}

bool MainFrame::HaveCodeAtCurrentLine()
{
	return HaveCodeAtCurLine(0);
}


AsmSrcView* MainFrame::GetCurrentView()
{
	return dynamic_cast<AsmSrcView*>(GetActiveFrame()->GetActiveView());
}


void MainFrame::OnSimEditBreakpoint()
{
	// todo
	// support breakpoint options
}

void MainFrame::OnUpdateSimEditBreakpoint(CCmdUI* cmd_ui)
{
	// todo
	cmd_ui->Enable(false);
}

//-----------------------------------------------------------------------------

void MainFrame::OnSimBreak()		// stop execution
{
	if (!GetDebugger().IsRunning())
		return;	

	GetDebugger().Break();			// stop simulation
	DelayedUpdateAll();

	AfxGetMainWnd()->SetFocus();		// restore focus (so it's not in I/O window)
}

void MainFrame::OnUpdateSimBreak(CCmdUI* cmd_ui) 
{
	cmd_ui->Enable(GetDebugger().IsRunning());
}

//-----------------------------------------------------------------------------

void MainFrame::OnSkipInstr()	// skip current instruction
{
	if (!GetDebugger().IsStopped())
		return;

	if (AsmSrcView* view= GetCurrentView())
	{
		//TODO: improve: any line with code that follows?
		int line= view->GetCurrLineNo() + 1;
		auto addr= GetDebugger().GetCodeAddress(line, view->GetDocument()->GetPathName());
		if (addr)
			GetDebugger().SkipToAddress(*addr);
	}
	else
	{
		//TODO: make it work in a debugger window too
		//
	}
}

void MainFrame::OnUpdateSkipInstr(CCmdUI* cmd_ui) 
{
	cmd_ui->Enable(GetDebugger().IsStopped() && HaveCodeAtCurLine(1));
}

//-----------------------------------------------------------------------------

void MainFrame::OnSimGo()		// run program
{
	if (GetDebugger().IsStopped())
		GetDebugger().Run();
}

void MainFrame::OnUpdateSimGo(CCmdUI* cmd_ui) 
{
	cmd_ui->Enable(GetDebugger().IsStopped());
}

//-----------------------------------------------------------------------------

void MainFrame::OnSimRunToLine()
{
	if (!GetDebugger().IsStopped())
		return;

	if (AsmSrcView* view= GetCurrentView())
	{
		int line= view->GetCurrLineNo();
		auto addr= GetDebugger().GetCodeAddress(line, view->GetDocument()->GetPathName());
		if (addr)
			GetDebugger().RunToAddress(*addr);
	}
}

void MainFrame::OnUpdateSimRunToLine(CCmdUI* cmd_ui) 
{
	cmd_ui->Enable(GetDebugger().IsStopped() && HaveCodeAtCurrentLine());
}

//-----------------------------------------------------------------------------

void MainFrame::OnSimSkipToLine()
{
	if (!GetDebugger().IsStopped())
		return;

	if (AsmSrcView* view= GetCurrentView())
	{
		int line= view->GetCurrLineNo();
		auto addr= GetDebugger().GetCodeAddress(line, view->GetDocument()->GetPathName());
		if (addr)
			GetDebugger().SkipToAddress(*addr);
	}
}

void MainFrame::OnUpdateSimSkipToLine(CCmdUI* cmd_ui) 
{
	cmd_ui->Enable(GetDebugger().IsStopped() && HaveCodeAtCurrentLine());
}

//-----------------------------------------------------------------------------

void MainFrame::OnSimStepOut()
{
	if (!GetDebugger().IsStopped())
		return;
	GetDebugger().StepOut();
}

void MainFrame::OnUpdateSimStepOut(CCmdUI* cmd_ui) 
{
	cmd_ui->Enable(GetDebugger().IsStopped());
}

//-----------------------------------------------------------------------------

void MainFrame::OnStepInto()
{
	if (!GetDebugger().IsStopped())
		return;
	GetDebugger().StepInto();
}

void MainFrame::OnUpdateStepInto(CCmdUI* cmd_ui)
{
	cmd_ui->Enable(GetDebugger().IsStopped());
}


void MainFrame::OnStepIntoExcp()
{
	if (!GetDebugger().IsStopped())
		return;
	GetDebugger().StepIntoException();
}

void MainFrame::OnUpdateStepIntoExcp(CCmdUI* cmd_ui)
{
	cmd_ui->Enable(GetDebugger().IsStoppedAtException());
}

//-----------------------------------------------------------------------------

void MainFrame::OnStepOver()
{
	if (!GetDebugger().IsStopped())
		return;
	GetDebugger().StepOver();
}

void MainFrame::OnUpdateStepOver(CCmdUI* cmd_ui)
{
	cmd_ui->Enable(GetDebugger().IsStopped());
}

//-----------------------------------------------------------------------------

void MainFrame::OnSimRestart()
{
	GetDebugger().Restart();
}

void MainFrame::OnUpdateSimRestart(CCmdUI* cmd_ui) 
{
	cmd_ui->Enable(GetDebugger().IsStopped());
}

//-----------------------------------------------------------------------------

void MainFrame::OnSimDebugStop()
{
	GetDebugger().AbortProg();
}

void MainFrame::OnUpdateSimDebugStop(CCmdUI* cmd_ui)
{
	cmd_ui->Enable(GetDebugger().IsActive());
}


//=============================================================================


int MainFrame::Options()
{
	OptionsDlg dlg(this);
	dlg.DoModal();
	return 0;
}


void MainFrame::OnOptions() 
{
	Options();
}


void MainFrame::OnUpdateOptions(CCmdUI* cmd_ui) 
{
	cmd_ui->Enable(true);
}

//-----------------------------------------------------------------------------

void MainFrame::ForAllViews(std::function<void (PointerView* view)> fn)
{
	auto* app= AfxGetApp();
	auto posTempl= app->GetFirstDocTemplatePosition();
	while (posTempl)
	{
		auto* templ= app->GetNextDocTemplate(posTempl);
		auto posDoc= templ->GetFirstDocPosition();
		while (posDoc)
		{
			auto* doc= templ->GetNextDoc(posDoc);
			auto posView = doc->GetFirstViewPosition();
			while (posView)
			{
				auto* view = doc->GetNextView(posView);
				if (auto* pv= dynamic_cast<PointerView*>(view))
					fn(pv);

			}
		}
	}
}

//-----------------------------------------------------------------------------

void MainFrame::OnFileSaveCode()
{
	SaveCode dlg(_T("Binary Code"), this, GetDebugger().GetCode());
	if (dlg.DoModal() == IDOK)
		dlg.Save();
}

void MainFrame::OnUpdateFileSaveCode(CCmdUI* cmd_ui)
{
	cmd_ui->Enable(GetDebugger().IsActive() && GetDebugger().GetCode().Valid());
}

//-----------------------------------------------------------------------------

void MainFrame::OnViewDeasm() 
{
	theApp.global_.CreateDisasmView();
}

void MainFrame::OnUpdateViewDeasm(CCmdUI* cmd_ui) 
{
	cmd_ui->Enable(GetDebugger().IsActive() && GetDebugger().GetCode().Valid());
}

//-----------------------------------------------------------------------------

void MainFrame::OnViewIdents() 
{
	//TODO: list identifiers found by assembler
}

void MainFrame::OnUpdateViewIdents(CCmdUI* cmd_ui) 
{
}

//-----------------------------------------------------------------------------

void MainFrame::OnViewMemory(UINT cmd)
{
	auto index= cmd - ID_VIEW_MEMORY_1;
	if (index < array_count(memory_wnd_))
	{
		auto& wnd= memory_wnd_[index];
		if (wnd.m_hWnd != 0)
			ShowControlBar(&wnd, !wnd.IsVisible(), false);
	}
}

void MainFrame::OnUpdateViewMemory(CCmdUI* cmd_ui)
{
	cmd_ui->Enable();
	auto index= cmd_ui->m_nID - ID_VIEW_MEMORY_1;
	if (index < array_count(memory_wnd_))
		cmd_ui->SetCheck((memory_wnd_[index].GetStyle() & WS_VISIBLE) != 0);
}

//-----------------------------------------------------------------------------

void MainFrame::OnViewDisplayWindow(UINT id)
{
	auto bar= GetControlBar(id);

	if (bar && bar->m_hWnd)
		ShowControlBar(bar, !bar->IsVisible(), false);
}

void MainFrame::OnUpdateViewDisplayWindow(CCmdUI* cmd_ui)
{
	auto bar= GetControlBar(cmd_ui->m_nID);

	if (bar && bar->m_hWnd)
	{
		cmd_ui->Enable();
		cmd_ui->SetCheck(bar->IsVisible());
	}
	else
		cmd_ui->Enable(false);
}

//-----------------------------------------------------------------------------

void MainFrame::OnViewIOWindow()
{
	//if (!GetDebugger().IsActive())
	//	return;

	if (!io_window_.m_hWnd)	// not yet created?
	{
		io_window_.Create();
		io_window_.ShowWindow(SW_NORMAL);
	}
	else
		io_window_.ShowWindow((io_window_.GetStyle() & WS_VISIBLE) ? SW_HIDE : SW_NORMAL);
}


void MainFrame::OnUpdateViewIOWindow(CCmdUI* cmd_ui) 
{
	cmd_ui->Enable(GetDebugger().IsActive());
	cmd_ui->SetCheck(io_window_.m_hWnd != 0 && (io_window_.GetStyle() & WS_VISIBLE) != 0);
}


LRESULT MainFrame::OnGetTerminalWnd(WPARAM active, LPARAM wnd)
{
	if (!io_window_.m_hWnd)	// not yet created?
		OnViewIOWindow();

	CWnd* terminal= &io_window_;
	return reinterpret_cast<LRESULT>(terminal);
}

//-----------------------------------------------------------------------------

void MainFrame::OnFileLoadCode()
{
	ProtectedCall([&]
	{
		LoadCodeDlg dlg(this);

		if (dlg.DoModal() != IDOK)
			return;

		Path path(dlg.path_);
		auto code= LoadCode(path, dlg.type_, GetDebugger().GetDefaultIsa(), dlg.address_);
		bool run= false; // TODO: set true for CF program only

		if (dlg.selected_isa_ != 0)	// force ISA?
		{
			switch(dlg.selected_isa_)
			{
			case 1:
				code.SetIsa(ISA::A);
				break;
			case 2:
				code.SetIsa(ISA::B);
				break;
			case 3:
				code.SetIsa(ISA::C);
				break;
			default:
				ASSERT(false);
				break;
			}
		}

		GetDebugger().SetDebugInfo(nullptr);
		GetDebugger().SetProgram(code, run);

	}, "Error loading binary code.");
}


void MainFrame::OnUpdateFileLoadCode(CCmdUI* cmd_ui)
{
	cmd_ui->Enable(GetDebugger().IsActive());
}

//-----------------------------------------------------------------------------

void MainFrame::OnSysColorChange()
{
	CMDIFrameWnd::OnSysColorChange();

	code_bmp_.DeleteObject();
	code_bmp_.LoadMappedBitmap(IDB_CODE);
	debug_bmp_.DeleteObject();
	debug_bmp_.LoadMappedBitmap(IDB_DEBUG);
}

//-----------------------------------------------------------------------------

void MainFrame::OnUpdateViewCpuBar(CCmdUI* cmd_ui)
{
	cmd_ui->SetCheck(cpu_wnd_.m_hWnd != 0 && cpu_wnd_.IsVisible());
}

void MainFrame::OnViewCpuBar()
{
	ShowControlBar(&cpu_wnd_, !cpu_wnd_.IsVisible(), false);
}

//-----------------------------------------------------------------------------

void MainFrame::OnViewLog()	// TODO
{
	//if (theApp.global_.IsDebugger())		// is simulator present?
	//{
	//	if (log_wnd_.m_hWnd != 0)
	//		log_wnd_.ShowWindow((log_wnd_.GetStyle() & WS_VISIBLE) ? SW_HIDE : SW_NORMAL);
	//	else
	//	{
	//		log_wnd_.Create();
	//		log_wnd_.ShowWindow(SW_SHOWNA);
	//	}
	//}
	//else		// no program
	//	if (log_wnd_.m_hWnd != 0)
	//		log_wnd_.ShowWindow(SW_HIDE);
}

void MainFrame::OnUpdateViewLog(CCmdUI* cmd_ui)
{
	//	cmd_ui->Enable(theApp.global_.IsDebugger());	// is simulator present?
	//	cmd_ui->SetCheck(log_wnd_.m_hWnd != 0 && (log_wnd_.GetStyle() & WS_VISIBLE) != 0);
}

//-----------------------------------------------------------------------------

void MainFrame::OnViewStack()
{
	ShowControlBar(&call_stack_, !call_stack_.IsVisible(), false);
}

void MainFrame::OnUpdateViewStack(CCmdUI* cmd_ui)
{
	cmd_ui->SetCheck(call_stack_.m_hWnd != 0 && call_stack_.IsVisible());
}

//-----------------------------------------------------------------------------

BOOL MainFrame::OnCmdMsg(UINT id, int code, void* extra, AFX_CMDHANDLERINFO* handler_info) 
{
	if (io_window_.OnCmdMsg(id, code, extra, handler_info))
		return true;

	// If the object(s) in the extended command route don't handle
	// the command, then let the base class OnCmdMsg handle it.
	return CMDIFrameWnd::OnCmdMsg(id, code, extra, handler_info);
}

//-----------------------------------------------------------------------------

void MainFrame::UpdateAll()
{
	if (io_window_.m_hWnd)
		io_window_.InvalidateRect(nullptr, false);
}


void MainFrame::DelayedUpdateAll()
{
	if (!timer_.Start(m_hWnd, 100, 200))
		UpdateAll();
}


void MainFrame::OnTimer(UINT_PTR id_event)
{
	if (id_event == timer_.Id())
	{
		timer_.Stop();
		UpdateAll();
	}
	else
		MainFrame::OnTimer(id_event);
}


void MainFrame::ShowDynamicHelp(const CString& line, int word_start, int word_end)
{
	//TODO
	//	help_bar_wnd_.DisplayHelp(line, word_start, word_end);
}

//-----------------------------------------------------------------------------

BOOL MainFrame::OnEraseBkgnd(CDC* dc)
{
	RECT rect;
	GetClientRect(&rect);
	dc->FillSolidRect(&rect, ::GetSysColor(COLOR_3DFACE));
	return TRUE;
}

// sync tab control with current document window
LRESULT MainFrame::OnMDIRefresh(WPARAM active, LPARAM wnd)
{
	CWnd* w= (CWnd*)wnd;
	if (w == nullptr)
		return 0;

	LPARAM param= LPARAM(w->m_hWnd);

	const int count= tab_ctrl_.GetItemCount();
	int present= -1;

	for (int i= 0; i < count; ++i)
		if (tab_ctrl_.GetItemData(i) == param)
		{
			present = i;
			break;
		}

	if (active == 2)	// wnd deleted?
	{
		if (present >= 0)
			tab_ctrl_.DeleteItem(present);
	}
	else
	{
		CString text;
		w->GetWindowText(text);

		if (present < 0)
			present = tab_ctrl_.InsertItem(-1, text, param);
		else
			tab_ctrl_.SetItemText(present, text);

		if (active)
			tab_ctrl_.SetCurSel(present);
	}

	RefreshStatusMessage();

	return 0;
}


void MainFrame::OnTabSelected(NMHDR* hdr, LRESULT* r)
{
	auto item= tab_ctrl_.GetCurSel();
	auto data= tab_ctrl_.GetItemData(item);

	CWnd* wnd= CWnd::FromHandle(HWND(data));
	MDIActivate(wnd);
}


// Open program help file (compiled HTML format)
extern void OpenHelp(const TCHAR* initial_page)
{
	Path help= GetApplicationFolder() / "Studio.chm";

	if (exists(help))
	{
		CWaitCursor wait;
		CString str= help.c_str();
		if (initial_page && *initial_page)
		{
			str += _T("::/");
			str += initial_page;
		}
		::ShellExecute(0, _T("open"), _T("hh.exe"), str, 0, SW_SHOWNORMAL);
	}
	else
	{
		CString msg= _T("Help file ");
		msg += help.c_str();
		msg += _T(" cannot be found.\nPlease reinstall ColdFire Studio.");
		AfxMessageBox(msg, MB_OK | MB_ICONERROR);
	}
}


void MainFrame::OnShowHelp()
{
	ProtectedCall([&]
	{
		OpenHelp(nullptr);
	}, "Error opening help.");
}


void MainFrame::ApplySettings(SettingsSection& settings)
{
	auto_open_source_ = settings.get_bool("dbg.open_src");
	auto_open_disasm_ = settings.get_bool("dbg.open_disasm");

	StudioApp::file_new_ = settings.get_bool("general.new_file");
}


LRESULT MainFrame::OnIdleUpdateCmdUI(WPARAM wParam, LPARAM lParam)
{
	CMDIFrameWnd::OnIdleUpdateCmdUI();

	// refresh display windows during idle time processing

	for (auto& p : display_map_)
	{
		auto& info= p.second;
		auto* wnd= info.wnd;
		auto* device= p.first;

		if (info.dirty)		// some display windows report when they require refresh
		{
			wnd->Refresh(device, info.display);

			if (info.dirty == DispInfo::Popup && !wnd->IsVisible())
				ShowControlBar(wnd, true, false);

			::InterlockedExchange(&info.dirty, DispInfo::None);
		}
		else if (info.display->ChangeNotification() == DisplayDevice::NotifyType::Polling && wnd->IsVisible())
		{
			auto time= ::GetTickCount();
			if (labs(time - last_refresh_time_) > 100)	// refresh every 10 ms at most
			{
				last_refresh_time_ = time;

				wnd->Refresh(device, info.display);
			}
		}
	}

	return 0;
}


LRESULT MainFrame::SendMsg(int msg, WPARAM wparam, LPARAM lparam)
{
	return SendMessage(msg, wparam, lparam);
}


LRESULT MainFrame::PostMsg(int msg, WPARAM wparam, LPARAM lparam)
{
	return PostMessage(msg, wparam, lparam);
}


void MainFrame::DeviceIO(PeripheralDevice* device, cf::DeviceAccess access, cf::uint32 addr)
{
	if (access == cf::DeviceAccess::Read)
		return;

	auto it= display_map_.find(device);
	if (it != display_map_.end())
		::InterlockedExchange(&it->second.dirty, access == cf::DeviceAccess::Write ? DispInfo::Popup : DispInfo::Dirty);
	else
	{ ASSERT(false); }
}
