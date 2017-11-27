/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

// Bar on top of disassembly window

#include "pch.h"
#include "DisasmBar.h"
#include "UIElements.h"
#include "DisasmView.h"
#include "Block.h"
#include "Debugger.h"
extern Debugger& GetDebugger();

// DisasmBar dialog

DisasmBar::DisasmBar() : CDialog(DisasmBar::IDD, nullptr), show_bytes_(true), show_ascii_(true)
{
	view_ = nullptr;
	in_update_ = false;
	backgnd_.CreateSolidBrush(::GetSysColor(COLOR_WINDOW));
}

DisasmBar::~DisasmBar()
{}

void DisasmBar::DoDataExchange(CDataExchange* dx)
{
	CDialog::DoDataExchange(dx);
	DDX_Check(dx, IDC_BYTES, show_bytes_);
	DDX_Check(dx, IDC_ASCII, show_ascii_);

	DDX_Control(dx, IDC_ADDR, address_edit_);
	DDX_Control(dx, IDC_SPIN, spin_);
	DDX_Control(dx, IDC_BANK, bank_);
}

BEGIN_MESSAGE_MAP(DisasmBar, CDialog)
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED(IDC_BYTES, &DisasmBar::OnBytes)
	ON_BN_CLICKED(IDC_ASCII, &DisasmBar::OnAscii)
	ON_EN_CHANGE(IDC_ADDR, OnAddressChange)
	ON_MESSAGE(WM_USER, OnAddrChanged)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN, OnDeltaposSpin)
	ON_CBN_SELCHANGE(IDC_BANK, &DisasmBar::OnGoToBank)
END_MESSAGE_MAP()


BOOL DisasmBar::OnInitDialog()
{
	CDialog::OnInitDialog();

	spin_.SetRange32(0, 1000000);

	GetHexEditFont(mono_);
	address_edit_.SetFont(&mono_);
	address_edit_.EnableSwitchButton(false);

	auto& dbg= GetDebugger();
	for (int i= 0; i < 9999; ++i)
	{
		auto bank= dbg.GetMemoryBankInfo(i);
		if (!bank.IsValid())
			break;

		if (bank.Access() != cf::MemoryAccess::Null)
		{
			auto pos= bank_.AddString(CString(bank.Name().c_str()));
			bank_.SetItemData(pos, i);
		}
	}
	bank_.SetCurSel(0);

	return false;
}


void DisasmBar::OnDeltaposSpin(NMHDR* nmhdr, LRESULT* result)
{
	NM_UPDOWN* updown= reinterpret_cast<NM_UPDOWN*>(nmhdr);

	updown->iPos = 3000;
	if (updown->iDelta)
	{
		auto address= address_edit_.GetNumValue();
		if (updown->iDelta > 0)
			address += 2;
		else
			address -= 2;

		SetAddress(address);
		PostMessage(WM_USER);
	}

	*result = 0;
}

HBRUSH DisasmBar::OnCtlColor(CDC* dc, CWnd* wnd, UINT ctlColor)
{
	HBRUSH br= CDialog::OnCtlColor(dc, wnd, ctlColor);

	dc->SetBkColor(::GetSysColor(COLOR_WINDOW));
	return backgnd_;
}


void DisasmBar::OnBytes()
{
	UpdateData();
	if (view_)
		view_->Invalidate();
}


void DisasmBar::OnAscii()
{
	UpdateData();
	if (view_)
		view_->Invalidate();
}


LRESULT DisasmBar::OnAddrChanged(WPARAM, LPARAM)
{
	auto address= address_edit_.GetNumValue();

	if (view_)
		view_->GoToAddress(address);

	return 0;
}


void DisasmBar::OnAddressChange()
{
	if (address_edit_.m_hWnd == nullptr || in_update_)
		return;

	PostMessage(WM_USER);
}


void DisasmBar::SetAddress(cf::uint32 address)
{
	Block update(in_update_);
	address_edit_.SetNumValue(address);
}


void DisasmBar::OnGoToBank()
{
	if (bank_.m_hWnd)
	{
		auto sel= bank_.GetCurSel();
		if (sel >= 0)
		{
			auto bank_index= static_cast<int>(bank_.GetItemData(sel));
			auto bank= GetDebugger().GetMemoryBankInfo(bank_index);

			SetAddress(bank.Base());
			PostMessage(WM_USER);
		}
	}
}
