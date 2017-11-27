/*-----------------------------------------------------------------------------
	ColdFire Studio

Copyright (c) 1996-2008 Mike Kowalski
-----------------------------------------------------------------------------*/

// MemoryDC.h: interface for the CMemoryDC class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MEMORYDC_H__1DFA018A_CA70_45E4_87EE_865C0E9982AC__INCLUDED_)
#define AFX_MEMORYDC_H__1DFA018A_CA70_45E4_87EE_865C0E9982AC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class MemoryDC : public CDC
{
public:
	MemoryDC(CDC& dc, CWnd* wnd, COLORREF rgb_clr_back= -1);
	MemoryDC(CDC& dc, const CRect& rect);
	virtual ~MemoryDC();

	void BitBlt();

private:
	CDC* dc_;
	CBitmap screen_bmp_;
	CPoint pos_;
	CSize size_;

	void Init(CDC& dc, const CRect& rect, COLORREF rgb_clr_back);
};

#endif // !defined(AFX_MEMORYDC_H__1DFA018A_CA70_45E4_87EE_865C0E9982AC__INCLUDED_)
