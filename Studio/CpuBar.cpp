/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "CpuBar.h"
#include "Debugger.h"

CpuBar::CpuBar()
{}

CpuBar::~CpuBar()
{}


BEGIN_MESSAGE_MAP(CpuBar, CSizingControlBarCF)
	ON_WM_CREATE()
END_MESSAGE_MAP()


void CpuBar::OnUpdateCmdUI(CFrameWnd* target, BOOL disableIfNoHndler)
{
	CSizingControlBarCF::OnUpdateCmdUI(target, false);
}


int CpuBar::OnCreate(CREATESTRUCT* cs)
{
	if (CSizingControlBarCF::OnCreate(cs) == -1)
		return -1;

	SetSCBStyle(GetSCBStyle() | SCBS_SIZECHILD | SCBS_SHOWEDGES);

	if (!dlg_.Create(this))
		return -1;

	return 0;
}


static struct
{
	int id;
	cf::Register reg;
	cf::uint32 old_value;
} g_registers[]=
{
	{ IDC_D0, cf::R_D0, 0 },
	{ IDC_D1, cf::R_D1, 0 },
	{ IDC_D2, cf::R_D2, 0 },
	{ IDC_D3, cf::R_D3, 0 },
	{ IDC_D4, cf::R_D4, 0 },
	{ IDC_D5, cf::R_D5, 0 },
	{ IDC_D6, cf::R_D6, 0 },
	{ IDC_D7, cf::R_D7, 0 },
	{ IDC_A0, cf::R_A0, 0 },
	{ IDC_A1, cf::R_A1, 0 },
	{ IDC_A2, cf::R_A2, 0 },
	{ IDC_A3, cf::R_A3, 0 },
	{ IDC_A4, cf::R_A4, 0 },
	{ IDC_A5, cf::R_A5, 0 },
	{ IDC_A6, cf::R_A6, 0 },
	{ IDC_A7, cf::R_A7, 0 },
	{ IDC_SR, cf::R_SR, 0 },
	{ IDC_PC, cf::R_PC, 0 },
};


void CpuBar::Notify(int event, UINT data, Debugger& debugger)
{
	const SimulatorStatus status= debugger.GetStatus();

	if (event != cf::E_RUNNING)
	{
		for (size_t i= 0; i < array_count(g_registers); ++i)
		{
			auto& r= g_registers[i];
			cf::uint32 val= debugger.GetRegister(r.reg);
			bool modified= status == SIM_FINISHED ? false : r.old_value != val;
			r.old_value = val;
			dlg_.SetRegister(r.id, val, modified);
		}

		cf::uint32 pc= debugger.GetRegister(cf::R_PC);
		DecodedInstruction d= debugger.DecodeInstruction(pc);
		dlg_.SetCurInstr(pc, d);

		dlg_.SetInstructionCount(debugger.ExecutedInstructions());

		bool super= debugger.GetFlag(cf::F_SUPERVISOR);
		dlg_.SetSupervisorMode(super);
	}

	bool enabled= true;
	switch (status)
	{
	case SIM_OK:
	case SIM_STOPPED:
		dlg_.SetStateIcon(CpuDlg::Ready);
		break;

	case SIM_BREAKPOINT_HIT:
		dlg_.SetStateIcon(CpuDlg::Breakpoint);
		break;

	case SIM_EXCEPTION:
		if (debugger.GetLastExceptionVector() == EX_AccessError || debugger.GetLastExceptionVector() == EX_AddressError)
			dlg_.SetStateIcon(CpuDlg::Bomb);
		else
			dlg_.SetStateIcon(CpuDlg::Exception);
		break;

	case SIM_INTERNAL_ERROR:
		dlg_.SetStateIcon(CpuDlg::Bomb);
		break;

	case SIM_FINISHED:
		dlg_.SetStateIcon(CpuDlg::Inactive);
		enabled = false;
		break;

	case SIM_IS_RUNNING:
		dlg_.SetStateIcon(CpuDlg::Running);
		enabled = false;
		break;

	default:
		ASSERT(false);
		break;
	}

	dlg_.SetStatusMsg(debugger.GetStatusMessage(status));

	dlg_.EnableDialog(enabled);
}


void CpuBar::SetCallback(const CpuDlg::Callback& fn)
{
	dlg_.SetCallback(fn);
}
