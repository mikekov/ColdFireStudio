/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#pragma once
#include "IOWindow.h"
#include "Broadcast.h"
#include "FlatBar.h"
#include "DynamicHelp.h"
#include "MemoryWnd.h"
#include "CpuBar.h"
#include "LEDSegmentsDlg.h"
#include "LCDDisplayDlg.h"
#include "Defs.h"
#include "CustomTabCtrl.h"
#include "SettingsClient.h"
#include "WndTimer.h"
#include "Debugger.h"

class AsmSrcDoc;
class AsmSrcView;
class Debugger;
class PointerView;
class PeripheralDevice;


class MainFrame : public CMDIFrameWnd, Broadcast, SettingsClient, Debugger::MainWindow
{
public:
	MainFrame();

	AsmSrcView* GetCurrentView();
	AsmSrcDoc* GetCurrentDocument() const;

	void UpdateTextPosition(int row, int col, bool insert_mode);

// Operations
	static const HWND* /*const*/ windows_[];

	void UpdateAll();
	void DelayedUpdateAll();

	void ShowDynamicHelp(const CString& line, int word_start, int word_end);

// Overrides
public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL OnCmdMsg(UINT id, int code, void* extra, AFX_CMDHANDLERINFO* handler_info);
	//virtual void WinHelp(DWORD data, UINT cmd = HELP_CONTEXT);

// Implementation
public:
	virtual ~MainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

private:
	afx_msg int OnCreate(LPCREATESTRUCT create_struct);
	afx_msg void OnClose();
	afx_msg void OnCheckProgram();
	afx_msg void OnUpdateCheckProgram(CCmdUI* cmd_ui);
	afx_msg void OnAssemble();
	afx_msg void OnUpdateAssemble(CCmdUI* cmd_ui);
	afx_msg void OnUpdateSimDebug(CCmdUI* cmd_ui);
	afx_msg void OnSimDebug();
	afx_msg void OnStepInto();
	afx_msg void OnUpdateStepInto(CCmdUI* cmd_ui);
	afx_msg void OnSkipInstr();
	afx_msg void OnUpdateSkipInstr(CCmdUI* cmd_ui);
	afx_msg void OnSimBreakpoint();
	afx_msg void OnUpdateSimBreakpoint(CCmdUI* cmd_ui);
	afx_msg void OnSimBreak();
	afx_msg void OnUpdateSimBreak(CCmdUI* cmd_ui);
	afx_msg void OnSimGo();
	afx_msg void OnUpdateSimGo(CCmdUI* cmd_ui);
	afx_msg void OnOptions();
	afx_msg void OnUpdateOptions(CCmdUI* cmd_ui);
	afx_msg void OnSimRunToLine();
	afx_msg void OnUpdateSimRunToLine(CCmdUI* cmd_ui);
	afx_msg void OnSimSkipToLine();
	afx_msg void OnUpdateSimSkipToLine(CCmdUI* cmd_ui);
	afx_msg void OnSimStepOut();
	afx_msg void OnUpdateSimStepOut(CCmdUI* cmd_ui);
	afx_msg void OnStepOver();
	afx_msg void OnUpdateStepOver(CCmdUI* cmd_ui);
	afx_msg void OnStepIntoExcp();
	afx_msg void OnUpdateStepIntoExcp(CCmdUI* cmd_ui);
	afx_msg void OnSimEditBreakpoint();
	afx_msg void OnUpdateSimEditBreakpoint(CCmdUI* cmd_ui);
	afx_msg void OnSimRestart();
	afx_msg void OnUpdateSimRestart(CCmdUI* cmd_ui);
	afx_msg void OnFileSaveCode();
	afx_msg void OnUpdateFileSaveCode(CCmdUI* cmd_ui);
	afx_msg void OnViewDeasm();
	afx_msg void OnUpdateViewDeasm(CCmdUI* cmd_ui);
	afx_msg void OnViewIdents();
	afx_msg void OnUpdateViewIdents(CCmdUI* cmd_ui);
	afx_msg void OnViewMemory(UINT cmd);
	afx_msg void OnUpdateViewMemory(CCmdUI* cmd_ui);
	afx_msg void OnViewIOWindow();
	afx_msg void OnUpdateViewIOWindow(CCmdUI* cmd_ui);
	afx_msg void OnDestroy();
	afx_msg void OnFileLoadCode();
	afx_msg void OnUpdateFileLoadCode(CCmdUI* cmd_ui);
	afx_msg void OnSysColorChange();
	afx_msg void OnUpdateViewCpuBar(CCmdUI* cmd_ui);
	afx_msg void OnViewCpuBar();
	afx_msg void OnTimer(UINT_PTR id_event);
	afx_msg void OnViewStack();
	afx_msg void OnUpdateViewStack(CCmdUI* cmd_ui);
	afx_msg void OnSimGenIRQ();
	afx_msg void OnUpdateSimGenIRG(CCmdUI* cmd_ui);
	afx_msg void OnSimGenNMI();
	afx_msg void OnUpdateSimGenNMI(CCmdUI* cmd_ui);
	afx_msg void OnSimGenReset();
	afx_msg void OnUpdateSimGenReset(CCmdUI* cmd_ui);
	afx_msg void OnSimGenIntDlg();
	afx_msg void OnUpdateSimGenIntDlg(CCmdUI* cmd_ui);
	afx_msg void OnViewLog();
	afx_msg void OnUpdateViewLog(CCmdUI* cmd_ui);
	afx_msg void OnSimDebugStop();
	afx_msg void OnViewLed7Window();
	afx_msg void OnUpdateViewLed7Window(CCmdUI* cmd_ui);
	afx_msg void OnViewDisplayWindow(UINT id);
	afx_msg void OnUpdateViewLed16Window(CCmdUI* cmd_ui);
	afx_msg void OnUpdateViewDisplayWindow(CCmdUI* cmd_ui);
	void OnUpdateSimDebugStop(CCmdUI* cmd_ui);
	LRESULT OnExecEvent(WPARAM, LPARAM);
	afx_msg void OnShowHelp();
	DECLARE_MESSAGE_MAP()

	void AddBreakpoint(AsmSrcView* view, int line, Defs::Breakpoint bp);
	void RemoveBreakpoint(AsmSrcView* view, int line);
	void DockBelow(CControlBar& first, CControlBar& second);
	BOOL OnEraseBkgnd(CDC* dc);
	BOOL VerifyBarState(LPCTSTR profile_name);
	LRESULT OnMDIRefresh(WPARAM active, LPARAM wnd);
	LRESULT OnGetTerminalWnd(WPARAM active, LPARAM wnd);
	void OnTabSelected(NMHDR*, LRESULT*);
	void AdjustDockBarStyles();
	void Assemble(bool enter_debugger);

	bool HaveCodeAtCurrentLine();
	bool HaveCodeAtCurLine(int delta);

	virtual void GetMessageString(UINT id, CString& message) const;
	void RefreshStatusMessage();

	CustomTabCtrl tab_ctrl_;

	virtual void ApplySettings(SettingsSection& settings);
	bool auto_open_source_;
	bool auto_open_disasm_;
	WndTimer timer_;

	IOWindow io_window_;
//	CDynamicHelp help_bar_wnd_;
	MemoryWnd memory_wnd_[4];
	CpuBar cpu_wnd_;
	MemoryWnd call_stack_;
	CStatusBar status_bar_wnd_;
	CFlatToolBar tool_bar_wnd_;
	LEDSegmentsDlg led_7seg_wnd_;
	LEDSegmentsDlg led_16seg_wnd_;
	LCDDisplayDlg lcd_display_wnd_;

	struct DispInfo
	{
		DispInfo(DisplayDlg* wnd= nullptr, DisplayDevice* display= nullptr)
			: wnd(wnd), dirty(Dirty), display(display)
		{}

		DisplayDlg* wnd;
		enum Flag : LONG { None= 0, Dirty, Popup };
		volatile LONG dirty;
		DisplayDevice* display;
	};
	std::map<PeripheralDevice*, DispInfo> display_map_;
	DWORD last_refresh_time_;

	static const TCHAR REG_ENTRY_MAINFRM[];
	static const TCHAR REG_POSX[], REG_POSY[], REG_SIZX[], REG_SIZY[], REG_STATE[];

	static WNDPROC pfn_old_proc_;
	static LRESULT CALLBACK StatusBarWndProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam);
	static CBitmap code_bmp_;
	static CBitmap debug_bmp_;

	CString format_;	// status bar text format

	afx_msg LRESULT OnUpdateState(WPARAM wParam, LPARAM lParam);

	void ForAllViews(std::function<void (PointerView* view)> fn);

	int Options();

	void ConfigSettings(bool load);
	void ExitDebugMode();

	bool InitDisplayWindow(DisplayDlg& wnd, CString title, int id, PeripheralDevice* display_display);
	afx_msg LRESULT OnIdleUpdateCmdUI(WPARAM wParam = 0, LPARAM lParam = 0);

	// debugger interface
	virtual LRESULT SendMsg(int msg, WPARAM, LPARAM);
	virtual LRESULT PostMsg(int msg, WPARAM, LPARAM);
	virtual void DeviceIO(PeripheralDevice* device, cf::DeviceAccess access, cf::uint32 addr);

	DECLARE_DYNAMIC(MainFrame)
};
