/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#pragma once
#include "DisplayDlg.h"


class LCDDisplayDlg : public DisplayDlg
{
public:
	LCDDisplayDlg();
	virtual ~LCDDisplayDlg();

	// rows and columns
	virtual void SetDimensions(CSize size);

	// refresh display
	virtual void Refresh(PeripheralDevice* device, DisplayDevice* display);

private:
	DECLARE_MESSAGE_MAP()

private:
	void OnPaint();

	virtual CSize BarSize() const;

	void Refresh(const BYTE* display_data, DWORD data_size);

	CSize dimensions_;
	int zoom_;
	CBitmap display_;
	COLORREF backgnd_color_;
};
