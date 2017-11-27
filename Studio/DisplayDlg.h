/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#pragma once
#include "scbarcf.h"
class PeripheralDevice;
class DisplayDevice;


class DisplayDlg : public CSizingControlBarCF
{
public:
	DisplayDlg();
	virtual ~DisplayDlg();

	// rows and columns
	virtual void SetDimensions(CSize size) = 0;

	// refresh display
	virtual void Refresh(PeripheralDevice* device, DisplayDevice* display) = 0;

private:
	DECLARE_MESSAGE_MAP()

protected:
	int OnCreate(CREATESTRUCT* cs);

private:
	virtual void OnUpdateCmdUI(CFrameWnd* target, BOOL disableIfNoHndler);
	afx_msg BOOL OnEraseBkgnd(CDC* dc);
	void OnSize(UINT type, int cx, int cy);

	virtual CSize CalcFixedLayout(BOOL stretch, BOOL horz);
	virtual CSize CalcDynamicLayout(int length, DWORD mode);

	virtual CSize BarSize() const = 0;
};
