/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2014 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "LCDDisplayDlg.h"
#include "MemoryDC.h"
#include "resource.h"
#include "../ColdFire/PeripheralDevice.h"
#include "Debugger.h"

extern Debugger& GetDebugger();


BEGIN_MESSAGE_MAP(LCDDisplayDlg, DisplayDlg)
	ON_WM_PAINT()
END_MESSAGE_MAP()


LCDDisplayDlg::LCDDisplayDlg() : dimensions_(0, 0)
{
	zoom_ = 3;
	backgnd_color_ = RGB(175, 177, 143); // inactive/passive border area
}


LCDDisplayDlg::~LCDDisplayDlg()
{}


CSize LCDDisplayDlg::BarSize() const
{
	return CSize(dimensions_.cx * zoom_, dimensions_.cy * zoom_);
}


void MonoDisplayToDib(const BYTE* display_data, DWORD data_size, int width, int height, int zoom, CBitmap& output)
{
	struct BH : BITMAPINFO
	{
		BH(int width, int height, int bits_per_pixel, int colors)
		{
			memset(this, 0, sizeof(*this));

			bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			bmiHeader.biWidth = width;
			bmiHeader.biHeight = height;
			bmiHeader.biPlanes = 1;
			bmiHeader.biBitCount = bits_per_pixel;
			bmiHeader.biCompression = DIB_RGB_COLORS;
			bmiHeader.biClrUsed = colors;
			bmiHeader.biClrImportant = colors;
		}

		RGBQUAD c[255];
	};

	const static RGBQUAD blk= { 0,0,0,0 };
	const static RGBQUAD wht= { 255,255,255,0 };
	const static RGBQUAD gry= { 63,63,63,0 };
	const static RGBQUAD lgr= { 15,15,15,0 };

	CBitmap mono;
	BH bh1(width, height, 1, 2);
	bh1.bmiColors[0] = blk;
	bh1.bmiColors[1] = wht;
	mono.Attach(::CreateDIBSection(nullptr, &bh1, 0, nullptr, nullptr, 0));
	mono.SetBitmapBits(data_size, display_data);
	::GdiFlush();

	DIBSECTION dib;
	if (!(output.m_hObject && output.GetObject(sizeof(dib), &dib) && dib.dsBm.bmBits && dib.dsBm.bmWidth == width * zoom && dib.dsBm.bmHeight == height * zoom))
	{
		output.DeleteObject();

		BH bh2(width * zoom, height * zoom, 8, 256);

		for (int c= 0; c < 256; ++c)
			bh2.bmiColors[c].rgbRed = 255;

		bh2.bmiColors[0] = blk;
		bh2.bmiColors[1] = wht;
		bh2.bmiColors[2] = gry;
		bh2.bmiColors[3] = lgr;
		output.Attach(::CreateDIBSection(nullptr, &bh2, 0, nullptr, nullptr, 0));
		output.GetObject(sizeof(dib), &dib);
	}

	CDC src, dst;
	src.CreateCompatibleDC(0);
	dst.CreateCompatibleDC(0);
	src.SelectObject(&mono);
	dst.SelectObject(&output);
	dst.SetBkColor(RGB(0,0,0));
	dst.SetTextColor(RGB(255,255,255));
	RGBQUAD table[4];
	table[0] = blk;
	table[1] = wht;
	::SetDIBColorTable(dst, 0, 2, table);
	dst.StretchBlt(0, 0, dib.dsBm.bmWidth, dib.dsBm.bmHeight, &src, 0, 0, width, height, SRCCOPY);

	BYTE gray_level[]= { 0, 2 };
	BYTE lgray_level[]= { 0, 3 };
	if (output.GetObject(sizeof(dib), &dib) && zoom == 3)
	{
		BYTE* buffer= static_cast<BYTE*>(dib.dsBm.bmBits);
		BYTE* line= buffer;
		for (int y= 0; y < dib.dsBm.bmHeight; ++y)
		{
			if (y % zoom == 0)
			{
				BYTE* p= line;

				for (int x= 0; x < width; ++x)
				{
					p[0] = gray_level[p[0]];
					p[1] = gray_level[p[1]];
					p[2] = lgray_level[p[2]];
					p += zoom;
				}
			}
			else
			{
				BYTE* p= line + zoom - 1;

				for (int x= 0; x < width; ++x)
				{
					*p = gray_level[*p];
					p += zoom;
				}
			}

			line += dib.dsBm.bmWidthBytes;
		}
	}
}

static const RGBQUAD BACKGND=	{ 184, 210, 201, 0 };
static const RGBQUAD LIT=		{ 20, 20, 20, 0 };
static const RGBQUAD GRAY=		{ 137,157,150, 0 };
static const RGBQUAD LTGRAY=	{ 169,189,182, 0 };


void LCDDisplayDlg::OnPaint()
{
	CPaintDC paint_dc(this);

	MemoryDC dc(paint_dc, this, backgnd_color_);

	CRect rect(0,0,0,0);
	GetClientRect(rect);

	if (display_.m_hObject)
	{
		CDC src;
		src.CreateCompatibleDC(&paint_dc);
		src.SelectObject(&display_);
		BITMAP bm;
		if (display_.GetBitmap(&bm))
		{
			int x= bm.bmWidth >= rect.Width() ? 0 : (rect.Width() - bm.bmWidth) / 2;
			int y= bm.bmHeight >= rect.Height() ? 0 : (rect.Height() - bm.bmHeight) / 2;
			RGBQUAD table[4];
			table[0] = BACKGND;
			table[1] = LIT;
			table[2] = GRAY;
			table[3] = LTGRAY;
			::SetDIBColorTable(src, 0, 4, table);
			dc.CDC::BitBlt(x, y, bm.bmWidth, bm.bmHeight, &src, 0, 0, SRCCOPY);
		}
	}

	dc.BitBlt();
}


void LCDDisplayDlg::SetDimensions(CSize size)
{
	dimensions_ = size;
}


void LCDDisplayDlg::Refresh(PeripheralDevice* device, DisplayDevice* display)
{
	auto size= display->GetWidth() * display->GetHeight();
	if (display->GetBitsPerPixel() > 8)
		size *= display->GetBitsPerPixel() / 8;
	else
		size /= 8 / display->GetBitsPerPixel();

	if (size == 0)
		return;

	auto base_address= display->GetScreenBaseAddress();
	std::vector<cf::uint8> buffer(size, 0);

	GetDebugger().ReadMemory(buffer.data(), base_address, size);

	Refresh(buffer.data(), size);
}


void LCDDisplayDlg::Refresh(const BYTE* display_data, DWORD data_size)
{
	MonoDisplayToDib(display_data, data_size, dimensions_.cx, dimensions_.cy, zoom_, display_);

	Invalidate();
}
