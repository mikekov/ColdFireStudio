/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#pragma once
#include "DisplayDlg.h"


class LEDSegmentsDlg : public DisplayDlg
{
public:
	LEDSegmentsDlg(int bitmap_rsrc_id, int segment_height);
	virtual ~LEDSegmentsDlg();

	// rows and columns
	void SetDimensions(CSize size);

	// light up segments; each element's bits control segments in a single alphanumeric LED brick
	void ShowSegments(const unsigned int segments[], size_t count);

	virtual void Refresh(PeripheralDevice* device, DisplayDevice* display);

private:
	DECLARE_MESSAGE_MAP()

private:
	int OnCreate(CREATESTRUCT* cs);
	void OnPaint();
	virtual CSize BarSize() const;

	CSize dimensions_;
	CSize segment_size_;
	std::vector<unsigned int> light_up_segments_;
	int bitmap_rsrc_id_;
	CBitmap led_bmp_;
};
