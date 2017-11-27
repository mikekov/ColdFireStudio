/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2014 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

// CpuDlg.cpp : implementation file
//

#include "pch.h"
#include "CpuDlg.h"
#include <sstream>
#include "Block.h"
#include "UIElements.h"
#include "FormatNums.h"

#undef TRACE

// CpuDlg dialog


CpuDlg::CpuDlg()
{
	icon_ = Inactive;
	supervisor_ = false;
	blocked_update_ = true;
	modifying_box_ = 0;
	id_map_.max_load_factor(0.7f);
	modified_regs_.max_load_factor(0.7f);
}

CpuDlg::~CpuDlg()
{}

void CpuDlg::DoDataExchange(CDataExchange* pDX)
{
	CWnd::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CpuDlg, CWnd)
	ON_WM_CTLCOLOR()
	ON_WM_ERASEBKGND()
	ON_COMMAND_RANGE(0, 0xfffe, OnCmd)
//	ON_CONTROL_RANGE(EN_CHANGE, 0, 0xfffe, OnReg)
//	ON_CONTROL_RANGE(EN_KILLFOCUS, 0, 0xfffe, OnReg)
//	ON_MESSAGE(WM_USER, OnRegChanged)
	ON_MESSAGE(NumberEdit::HEX_CHANGED_MSG, OnRegChanged)
	ON_EN_CHANGE(IDC_I, OnInterruptLevelChanged)
END_MESSAGE_MAP()

namespace {
	struct Ids
	{
		Ids(int ctrl_id, cf::Register reg_id) : ctrl_id(ctrl_id), reg_id(reg_id)
		{}
		int ctrl_id;
		cf::Register reg_id;
	};

	static const Ids g_registers[]=
	{
		Ids(IDC_D0, cf::R_D0),
		Ids(IDC_D1, cf::R_D1),
		Ids(IDC_D2, cf::R_D2),
		Ids(IDC_D3, cf::R_D3),
		Ids(IDC_D4, cf::R_D4),
		Ids(IDC_D5, cf::R_D5),
		Ids(IDC_D6, cf::R_D6),
		Ids(IDC_D7, cf::R_D7),
		Ids(IDC_A0, cf::R_A0),
		Ids(IDC_A1, cf::R_A1),
		Ids(IDC_A2, cf::R_A2),
		Ids(IDC_A3, cf::R_A3),
		Ids(IDC_A4, cf::R_A4),
		Ids(IDC_A5, cf::R_A5),
		Ids(IDC_A6, cf::R_A6),
		Ids(IDC_A7, cf::R_A7),
		Ids(IDC_PC, cf::R_PC),
		Ids(IDC_SR, cf::R_SR),
	};

} // namespace


//void CpuDlg::OnReg(UINT ctrl_id)
//{
//	if (modify_register_.empty() || blocked_update_)
//		return;
//
//	auto it= id_map_.find(ctrl_id);
//	if (it == id_map_.end())
//		return;
//
//	// postpone update till masked edit gets changed
////	PostMessage(WM_USER, ctrl_id, 0);
//
//	cf::Register reg= it->second;
//	CString str;
//	GetDlgItemText(it->first, str);
////	TCHAR* dummy;
//	// todo: unsigned int
////	auto value= _tcstol(str, &dummy, 16);
//
////	modify_register_(reg, value, ~uint32(0));
//}


void CpuDlg::OnInterruptLevelChanged()
{
	if (modify_register_ == nullptr || blocked_update_)
		return;

	BOOL translated= false;
	auto level= GetDlgItemInt(IDC_I, &translated, false);

	auto mask= cf::SR_INTERRUPT_MASK << cf::SR_INTERRUPT_POS;

	if (translated && level >= 0 && level <= 7)
		modify_register_(cf::R_SR, level << cf::SR_INTERRUPT_POS, ~mask);
}


LRESULT CpuDlg::OnRegChanged(WPARAM ctrl_id, LPARAM hex_ctrl)
{
	if (modify_register_ == nullptr || blocked_update_)
		return 0;

	auto id= static_cast<int>(ctrl_id);

	auto it= id_map_.find(id);
	if (it == id_map_.end())
		return 0;

	NumberEdit& hex= *reinterpret_cast<NumberEdit*>(hex_ctrl);

	cf::Register reg= it->second;
//	CString str;
//	GetDlgItemText(it->first, str);
//	TCHAR* dummy;
//	auto value= _tcstoul(str, &dummy, 16);
	auto value= hex.GetNumValue();

	modifying_box_ = id;
	modify_register_(reg, value, ~uint32(0));

	return 0;
}


void CpuDlg::OnCmd(UINT cmd)
{
	if (modify_register_ == nullptr || blocked_update_)
		return;

	bool checked= false;

	switch (cmd)
	{
	case IDC_C:
	case IDC_Z:
	case IDC_N:
	case IDC_V:
	case IDC_X:
	case IDC_T:
	case IDC_S:
	case IDC_M:
		checked = !!IsDlgButtonChecked(cmd);
		break;

	default:
		return;
	}

	uint32 bit= 0;

	switch (cmd)
	{
	case IDC_C:
		bit = cf::SR_CARRY; break;
	case IDC_Z:
		bit = cf::SR_ZERO; break;
	case IDC_N:
		bit = cf::SR_NEGATIVE; break;
	case IDC_V:
		bit = cf::SR_OVERFLOW; break;
	case IDC_X:
		bit = cf::SR_EXTEND; break;
	case IDC_T:
		bit = cf::SR_TRACE; break;
	case IDC_S:
		bit = cf::SR_SUPERVISOR; break;
	case IDC_M:
		bit = cf::SR_MASTER; break;
	}

	uint32 add= 0;
	uint32 remove= 0;
	if (checked)
		add = bit;
	else
		remove = bit;

	modify_register_(cf::R_SR, add, remove);
}


bool CpuDlg::Create(CWnd* parent)
{
	if (!CWnd::CreateDlg(MAKEINTRESOURCE(IDD), parent))
		return false;

	InitDialog();

	return true;
}


void CpuDlg::InitDialog()
{
	if (CEdit* edit= static_cast<CEdit*>(GetDlgItem(IDC_I)))
		edit->LimitText(1);

	::GetHexEditFont(fixed_font_);

	for (size_t i= 0; i < array_count(g_registers); ++i)
	{
		auto id= g_registers[i];
		id_map_[id.ctrl_id] = id.reg_id;
		bool long_word= id.ctrl_id != IDC_SR;
		regs_[i].SetSize(long_word);
		VERIFY(regs_[i].SubclassDlgItem(id.ctrl_id, this));
		regs_[i].SetFont(&fixed_font_);
		regs_[i].SetStep(id.ctrl_id == IDC_A7 || id.ctrl_id == IDC_PC ? 2 : 1);

		if (id.ctrl_id == IDC_PC || id.ctrl_id == IDC_SR)
			regs_[i].EnableSwitchButton(false);
	}

	HINSTANCE inst= AfxFindResourceHandle(MAKEINTRESOURCE(IDB_STATE_ICONS), RT_BITMAP);
	int img_width= 28;
	if (HIMAGELIST img_list= ImageList_LoadImage(inst, MAKEINTRESOURCE(IDB_STATE_ICONS), img_width, 0, RGB(255,0,255), IMAGE_BITMAP, LR_CREATEDIBSECTION))
		icons_.Attach(img_list);

	label_.SubclassDlgItem(IDC_STATE_LABEL, this);

	blocked_update_ = false;
}


BOOL CpuDlg::OnEraseBkgnd(CDC* dc)
{
	CRect rect(0,0,0,0);
	GetClientRect(rect);

	COLORREF back= ::GetSysColor(COLOR_3DFACE);
	dc->FillSolidRect(rect, back);

	// bitmap (x, y) location in the dialog
	WINDOWPLACEMENT wp;
	if (label_.GetWindowPlacement(&wp))
	{
		auto h = wp.rcNormalPosition.bottom - wp.rcNormalPosition.top;
		IMAGEINFO ii;
		if (icons_.GetImageInfo(icon_, &ii))
		{
			auto ih = ii.rcImage.bottom - ii.rcImage.top;
			icons_.Draw(dc, icon_, CPoint(wp.rcNormalPosition.right + 2, wp.rcNormalPosition.top - 1 + (h - ih) / 2), ILD_TRANSPARENT);
		}
	}

	return true;
}

// coloring: mark status register with orange background if supervisor bit is set,
// mark registers that have changed since last update with red text
//
const COLORREF SUPER= RGB(255,220,180);
const COLORREF CHANGED= RGB(255,0,0);
CBrush super_brush(SUPER);
CBrush normal_brush(::GetSysColor(COLOR_WINDOW));

HBRUSH CpuDlg::OnCtlColor(CDC* dc, CWnd* wnd, UINT flags)
{
	if (wnd)
	{
		HBRUSH hbr= nullptr;
		auto id= wnd->GetDlgCtrlID();

		auto it= modified_regs_.find(id);
		if (it != modified_regs_.end() && it->second && wnd->IsWindowEnabled())
		{
			dc->SetTextColor(CHANGED);
			hbr = normal_brush;
		}

		if (supervisor_ && id == IDC_SR)
		{
			dc->SetBkColor(SUPER);
			hbr = super_brush;
		}

		if (hbr)
			return hbr;
	}
	return CWnd::OnCtlColor(dc, wnd, flags);
}


NumberEdit* CpuDlg::GetHexEdit(int id)
{
	return dynamic_cast<NumberEdit*>(GetDlgItem(id));
}


void CpuDlg::SetRegister(int id, UINT val, bool modified)
{
	if (modifying_box_ == id)
	{
		modifying_box_ = 0;
		return;
	}

	NumberEdit* hex= GetHexEdit(id);
	if (hex == nullptr)
		return;

	Block update(blocked_update_);

	if (id != IDC_PC)
		modified_regs_[id] = modified;

	if (id == IDC_SR)
	{
		// block updates

		CheckDlgButton(IDC_X, val & cf::SR_EXTEND ? 1 : 0);
		CheckDlgButton(IDC_N, val & cf::SR_NEGATIVE ? 1 : 0);
		CheckDlgButton(IDC_Z, val & cf::SR_ZERO ? 1 : 0);
		CheckDlgButton(IDC_V, val & cf::SR_OVERFLOW ? 1 : 0);
		CheckDlgButton(IDC_C, val & cf::SR_CARRY ? 1 : 0);

		CheckDlgButton(IDC_S, val & cf::SR_SUPERVISOR ? 1 : 0);
		CheckDlgButton(IDC_T, val & cf::SR_TRACE ? 1 : 0);
		CheckDlgButton(IDC_M, val & cf::SR_MASTER ? 1 : 0);
		SetDlgItemInt(IDC_I, (val & cf::SR_INTERRUPT_MASK) >> cf::SR_INTERRUPT_POS);
	}

	hex->SetNumValue(val);
}


void CpuDlg::SetCurInstr(uint32 addr, const DecodedInstruction& d)
{
	try
	{
		CString temp(d.ToString(addr, DecodedInstruction::SHOW_NONE | DecodedInstruction::COLON_IN_LONG_ARGS, ' ').c_str());
		SetDlgItemText(IDC_INSTR, temp);
	}
	catch (std::exception& ex)
	{
		SetDlgItemTextA(m_hWnd, IDC_INSTR, ex.what());
	}
}


void CpuDlg::SetStateIcon(StateIcon icon)
{
	if (icon_ != icon)
	{
		icon_ = icon;
		Invalidate();
	}
}


void CpuDlg::SetStatusMsg(const TCHAR* msg)
{
	SetDlgItemText(IDC_STATE, msg);
}


void CpuDlg::SetSupervisorMode(bool super)
{
	if (supervisor_ != super)
	{
		supervisor_ = super;
		if (CWnd* wnd= GetDlgItem(IDC_SR))
			wnd->Invalidate();
	}
}


void CpuDlg::SetCallback(const Callback& fn)
{
	modify_register_ = fn;
}


static BOOL CALLBACK EnableProc(HWND hwnd, LPARAM enable)
{
	auto code= ::SendMessage(hwnd, WM_GETDLGCODE, 0, 0);
	if (code & (DLGC_BUTTON | DLGC_DEFPUSHBUTTON | DLGC_RADIOBUTTON | DLGC_UNDEFPUSHBUTTON | DLGC_HASSETSEL))
		::EnableWindow(hwnd, !!enable);
	return true;
}


void CpuDlg::EnableDialog(bool enable)
{
	::EnumChildWindows(*this, EnableProc, enable);
}


BOOL CpuDlg::PreTranslateMessage(MSG* msg)
{
	if (AfxGetMainWnd()->PreTranslateMessage(msg))
		return true;

	if (CWnd::PreTranslateMessage(msg))
		return true;

	return PreTranslateInput(msg);
}


void CpuDlg::SetInstructionCount(unsigned int count)
{
//	SetDlgItemInt(IDC_CYCLES, count, false);
	SetDlgItemText(IDC_INSTRUCTIONS, FormatWithDecimalSep(count).c_str());
}
