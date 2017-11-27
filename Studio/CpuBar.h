/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#pragma once
#include "scbarcf.h"
#include "CpuDlg.h"
class Debugger;


class CpuBar : public CSizingControlBarCF
{
public:
	CpuBar();
	virtual ~CpuBar();

	void Notify(int event, UINT data, Debugger& debugger);

	void SetCallback(const CpuDlg::Callback& fn);

private:
	DECLARE_MESSAGE_MAP()

private:
	virtual void OnUpdateCmdUI(CFrameWnd* target, BOOL disableIfNoHndler);
	int OnCreate(CREATESTRUCT* cs);

	CpuDlg dlg_;
};
