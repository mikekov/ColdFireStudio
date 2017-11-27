/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#pragma once
#include "resource.h"
#include "../ColdFire/DecodedInstr.h"
#include "../ColdFire/Types.h"
#include "NumberEdit.h"
#include "load_jpeg.h"

// CpuDlg dialog

class CpuDlg : public CWnd
{
public:
	CpuDlg();
	virtual ~CpuDlg();

	enum { IDD = IDD_CPU_DLG };

	bool Create(CWnd* parent);

	// set edit box
	void SetRegister(int id, UINT val, bool modified);

	void SetCurInstr(uint32 addr, const DecodedInstruction& d);

	enum StateIcon { Bomb, Running, Breakpoint, Ready, Inactive, Exception };
	void SetStateIcon(StateIcon icon);

	void SetStatusMsg(const TCHAR* msg);

	void SetSupervisorMode(bool super);

	void SetInstructionCount(unsigned int count);

	typedef std::function<void (cf::Register id, uint32 add, uint32 remove)> Callback;

	void SetCallback(const Callback& fn);

	void EnableDialog(bool enable);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL PreTranslateMessage(MSG* msg);

	DECLARE_MESSAGE_MAP()

private:
	void InitDialog();
	HBRUSH OnCtlColor(CDC* dc, CWnd* wnd, UINT flags);
	BOOL OnEraseBkgnd(CDC* dc);
	void OnCmd(UINT cmd);
	LRESULT OnRegChanged(WPARAM, LPARAM);
	void OnInterruptLevelChanged();
	NumberEdit* GetHexEdit(int id);

	CFont fixed_font_;
	NumberEdit regs_[8 + 8 + 2];
	CImageList icons_;
	CStatic label_;
	StateIcon icon_;
	bool supervisor_;
	Callback modify_register_;
	bool blocked_update_;
	struct hash_pass_thru
	{
		size_t operator () (int v) const	{ return v; }
	};
	std::unordered_map<int, cf::Register, hash_pass_thru> id_map_;
	std::unordered_map<int, bool, hash_pass_thru> modified_regs_;	// dlg register id to "modified" flag
	int modifying_box_;
};
