/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2014 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "UIElements.h"
#include "Utilities.h"


void GetHexEditFont(CFont& font)
{
	HFONT hfont= static_cast<HFONT>(::GetStockObject(DEFAULT_GUI_FONT));
	LOGFONT lf;
	::GetObject(hfont, sizeof(lf), &lf);
	lf.lfPitchAndFamily = FIXED_PITCH;
	lf.lfHeight = ToLogicalPixels(CSize(0, -13)).cy;
	_tcscpy(lf.lfFaceName, L"Bitstream Vera Sans Mono");
	font.DeleteObject();
	font.CreateFontIndirect(&lf);
}


void SubclassHexEdit(CWnd* dialog, int ctrl_id, CMFCMaskedEdit& hex_edit, bool long_word)
{
	VERIFY(hex_edit.SubclassDlgItem(ctrl_id, dialog));
	//regs_[i].SetFont(&fixed_font_);
//		bool word= id.ctrl_id == IDC_SR;
	hex_edit.EnableMask(!long_word ? L"AAAA" : L"AAAAAAAA", !long_word ? L"____" : L"________", _T('0'), L"0123456789abcdefABCDEF");
	hex_edit.SetWindowText(L"");
}
