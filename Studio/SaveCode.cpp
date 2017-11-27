/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

// Save code file dialog + saving binary code

#include "pch.h"
#include "SaveCode.h"
#include "IntelHex.h"
#include "MotorolaSRecord.h"
#include "../ColdFire/Simulator.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


const wchar_t SECTION[]= L"SaveCode";

/////////////////////////////////////////////////////////////////////////////

extern void DDX_HexDec(CDataExchange* dx, int id, unsigned int& num, int size)
{
	HWND wnd= dx->PrepareEditCtrl(id);
	TCHAR buf[64];
	if (dx->m_bSaveAndValidate)
	{
		::GetWindowText(wnd, buf, static_cast<int>(array_count(buf)));
		TCHAR* text= buf;
		bool negative= false;
		if (text[0] == _T('-'))	// liczba ujemna?
		{
			text++;
			negative = true;
		}
		if (text[0] == _T('$'))
		{
			if (swscanf(text + 1, _T("%X"), &num) <= 0)
			{
				AfxMessageBox(IDS_MSG_BAD_DEC_HEX_NUM);
				dx->Fail();		// throws exception
			}
		}
		else if (text[0] == _T('0') && (text[1]==_T('x') || text[1]==_T('X')))
		{
			if (swscanf(text + 2, _T("%X"), &num) <= 0)
			{
				AfxMessageBox(IDS_MSG_BAD_DEC_HEX_NUM);
				dx->Fail();		// throws exception
			}
		}
		else if (swscanf(text, _T("%u"), &num) <= 0)
		{
			AfxMessageBox(IDS_MSG_BAD_DEC_HEX_NUM);
			dx->Fail();		// throws exception
		}
		if (negative)
			num = 0 - num;
	}
	else
	{
		const TCHAR* fmt= _T("$%08X");
		if (size == 1)
			fmt = _T("$%02X");
		else if (size == 2)
			fmt = _T("$%04X");

		wsprintf(buf, fmt, num);
		::SetWindowText(wnd, buf);
	}
}

/////////////////////////////////////////////////////////////////////////////
// SaveCode

IMPLEMENT_DYNAMIC(SaveCode, CFileDialog)

namespace {
	CString GetFilter() { CString filter; VERIFY(filter.LoadString(IDS_SAVE_CODE)); return filter; }
}


SaveCode::SaveCode(const TCHAR* file_name, CWnd* parent, const cf::BinaryProgram& code)
 : CFileDialog(false, L"cfs", file_name, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOREADONLYRETURN, GetFilter(), nullptr, 0),
	code_(code)
{
	VERIFY(title_.LoadString(IDS_SAVE_CODE_DLG));
	m_ofn.lpstrTitle = title_;
	m_ofn.nFilterIndex = AfxGetApp()->GetProfileInt(SECTION, L"Filter", 4);
	format_ = BinaryFormat::CFProgram;
}


BEGIN_MESSAGE_MAP(SaveCode, CFileDialog)
	//{{AFX_MSG_MAP(SaveCode)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

//-----------------------------------------------------------------------------

void SaveCode::Save()
{
	AfxGetApp()->WriteProfileInt(SECTION, L"Filter", m_ofn.nFilterIndex);

	CString file_name= GetPathName();
	if (file_name.GetLength() == 0)
		return;

	Path path(file_name);

	// determine format beased on file extension
	format_ = GuessFormat(path);
	if (format_ == BinaryFormat::AutoDetect) // if extension is not recognized, use format specified in a file save dialog box
	{
		switch (m_ofn.nFilterIndex)
		{
		case 1:
			format_ = BinaryFormat::Hex;
			break;
		case 2:
			format_ = BinaryFormat::SRecord;
			break;
		case 3:
			format_ = BinaryFormat::RawBinary;
			break;
		case 4:
			format_ = BinaryFormat::CFProgram;
			break;
		default:
			ASSERT(false);
			break;
		}
	}

	switch (format_)
	{
	case BinaryFormat::Hex:		// Intel-HEX format (*.hex)
		SaveHexCode(path, code_);
		break;
	case BinaryFormat::SRecord:	// Motorola s-record (*.s9)
		SaveSRecordCode(path, code_);
		break;
	case BinaryFormat::AtariPrg:	// not supported
	case BinaryFormat::RawBinary:	// binary dump
		//todo
		//options
//			SaveBinaryCode(archive,m_uStart,m_uEnd,2, code_);
		break;
	case BinaryFormat::CFProgram:	// binary program in internal CF Studio format
		cf::SaveBinaryCode(file_name, code_);
		break;
	default:
		ASSERT(false);
	}
}
