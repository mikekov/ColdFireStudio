/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "LEDSegmentsDlg.h"
#include "MemoryDC.h"
#include "resource.h"
#include "../ColdFire/PeripheralDevice.h"


BEGIN_MESSAGE_MAP(LEDSegmentsDlg, DisplayDlg)
	ON_WM_CREATE()
	ON_WM_PAINT()
END_MESSAGE_MAP()


LEDSegmentsDlg::LEDSegmentsDlg(int bitmap_rsrc_id, int segment_height)
	: dimensions_(10, 1), segment_size_(0, segment_height)
{
	bitmap_rsrc_id_ = bitmap_rsrc_id;
}

LEDSegmentsDlg::~LEDSegmentsDlg()
{}


CSize LEDSegmentsDlg::BarSize() const
{
	return CSize(dimensions_.cx * segment_size_.cx, dimensions_.cy * segment_size_.cy);
}


int LEDSegmentsDlg::OnCreate(CREATESTRUCT* cs)
{
	if (led_bmp_.m_hObject == nullptr)
	{
		auto handle= ::LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(bitmap_rsrc_id_), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
		led_bmp_.Attach(handle);

		BITMAP bm;
		if (led_bmp_.GetBitmap(&bm))
			segment_size_.cx = segment_size_.cy * bm.bmWidth / bm.bmHeight;
	}

	ASSERT(led_bmp_.m_hObject != nullptr);

	if (DisplayDlg::OnCreate(cs) == -1)
		return -1;

	//SetSCBStyle(GetSCBStyle() | SCBS_SIZECHILD | SCBS_SHOWEDGES);

	return 0;
}


// LED segment background color
static const RGBQUAD BLACK= { 0, 0, 0, 0 };
// segments that are not lit
static const RGBQUAD GRAY= { 40, 40, 40, 0 };
// lit segments
static const RGBQUAD RED= { 0, 0, 250, 0 };


void LEDSegmentsDlg::OnPaint()
{
	CPaintDC paint_dc(this);

	COLORREF backgnd= RGB(BLACK.rgbRed, BLACK.rgbGreen, BLACK.rgbBlue);
	MemoryDC dc(paint_dc, this, backgnd);

	CRect rect(0,0,0,0);
	GetClientRect(rect);

	dc.SetStretchBltMode(HALFTONE);

	CDC bmp;
	bmp.CreateCompatibleDC(&dc);

	bmp.SelectObject(&led_bmp_);
	BITMAP bm;
	if (led_bmp_.GetBitmap(&bm) < sizeof(bmp))
		return;

	auto x= rect.left;
	auto y= rect.top;

	auto w= dimensions_.cx * segment_size_.cx;
	auto h= dimensions_.cy * segment_size_.cy;

	if (w < rect.Width())
		x += (rect.Width() - w) / 2;
	if (h < rect.Height())
		y += (rect.Height() - h) / 2;

	auto count= light_up_segments_.size();
	size_t index= 0;

	const int COLORS= 256;

	for (int row= 0; row < dimensions_.cy; ++row)
	{
		for (int column= 0; column < dimensions_.cx; ++column)
		{
			RGBQUAD table[COLORS];

			std::fill_n(table, COLORS, GRAY);

			table[0] = BLACK;

			auto segments= index < count ? light_up_segments_[index++] : 0;

			// turn on LED segments where bits are set; scan 18 most significant bits only
			int bit= 1;
			for (unsigned int mask= 0x80000000; mask > 0x2000; mask >>= 1)
				table[bit++] = mask & segments ? RED : GRAY;

			::SetDIBColorTable(bmp, 0, COLORS, table);

			dc.StretchBlt(x, y, segment_size_.cx, segment_size_.cy, &bmp, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);

			x += segment_size_.cx;
		}

		y += segment_size_.cy;
	}

	dc.BitBlt();
}


void LEDSegmentsDlg::SetDimensions(CSize size)
{
	dimensions_ = size;
}


void LEDSegmentsDlg::ShowSegments(const unsigned int segments[], size_t count)
{
	if (segments)
		light_up_segments_.assign(segments, segments + count);
	else
		light_up_segments_.clear();

	Invalidate();
}


void LEDSegmentsDlg::Refresh(PeripheralDevice* device, DisplayDevice* display)
{
	const auto size= display->GetWidth() * display->GetHeight();
	std::vector<unsigned int> buffer(size, 0);
	for (cf::uint32 i= 0; i < size; ++i)
		buffer[i] = device->ReadBufferLongWord(i);

	ShowSegments(buffer.data(), buffer.size());
}
