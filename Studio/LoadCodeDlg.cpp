/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

// LoadCodeDlg.cpp : options for loading binary files

#include "pch.h"
#include "resource.h"
#include "LoadCodeDlg.h"

extern void DDX_HexDec(CDataExchange* dx, int id, unsigned int& num, int size);

// LoadCodeDlg dialog

IMPLEMENT_DYNAMIC(LoadCodeDlg, CDialog)

LoadCodeDlg::LoadCodeDlg(CWnd* parent) : CDialog(LoadCodeDlg::IDD, parent)
{
	type_ = BinaryFormat::AutoDetect;
	type_int_ = 0;
	address_ = 0x10000;
	selected_isa_ = 0;
	file_filter_index_ = 0;
}


LoadCodeDlg::~LoadCodeDlg()
{}


void LoadCodeDlg::DoDataExchange(CDataExchange* dx)
{
	CDialog::DoDataExchange(dx);
	DDX_Control(dx, IDC_PATH, path_edit_);
	DDX_Text(dx, IDC_PATH, path_);
	DDX_Radio(dx, IDC_AUTO, type_int_);
	DDX_Control(dx, IDC_ISA, isa_);
	DDX_CBIndex(dx, IDC_ISA, selected_isa_);
	DDX_HexDec(dx, IDC_ADDRESS, address_, 4);
}


BEGIN_MESSAGE_MAP(LoadCodeDlg, CDialog)
	ON_BN_CLICKED(IDC_BUTTON1, &LoadCodeDlg::OnBrowse)
END_MESSAGE_MAP()


// LoadCodeDlg message handlers


void LoadCodeDlg::OnBrowse()
{
	CString filter;
	VERIFY(filter.LoadString(IDS_LOAD_CODE));
	CString path;
	path_edit_.GetWindowText(path);
	CString file_name;

	CFileDialog dlg(true, L"bin", path, OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY, filter, this);

	dlg.GetOFN().nFilterIndex = file_filter_index_;

	if (dlg.DoModal() != IDOK)
		return;

	file_filter_index_ = dlg.GetOFN().nFilterIndex;

	path_edit_.SetWindowText(dlg.GetPathName());
}


const wchar_t SECTION[]= L"LoadCode";


BOOL LoadCodeDlg::OnInitDialog()
{
	path_ = AfxGetApp()->GetProfileString(SECTION, L"Path");
	address_ = AfxGetApp()->GetProfileInt(SECTION, L"Address", address_);
	selected_isa_ = AfxGetApp()->GetProfileInt(SECTION, L"ISA", selected_isa_);
	type_int_ = AfxGetApp()->GetProfileInt(SECTION, L"Type", type_int_);
	file_filter_index_ = AfxGetApp()->GetProfileInt(SECTION, L"Filter", file_filter_index_);

	CDialog::OnInitDialog();

	::SHAutoComplete(path_edit_, SHACF_FILESYSTEM);

	return true;
}


void LoadCodeDlg::OnOK()
{
	CDialog::OnOK();

	switch (type_int_)
	{
	case 0:
		type_ = BinaryFormat::AutoDetect;
		break;
	case 1:
		type_ = BinaryFormat::Hex;
		break;
	case 2:
		type_ = BinaryFormat::SRecord;
		break;
	case 3:
		type_ = BinaryFormat::CFProgram;
		break;
	case 4:
		type_ = BinaryFormat::RawBinary;
		break;
	default:
		ASSERT(false);
		break;
	}

	AfxGetApp()->WriteProfileString(SECTION, L"Path", path_);
	AfxGetApp()->WriteProfileInt(SECTION, L"Address", address_);
	AfxGetApp()->WriteProfileInt(SECTION, L"ISA", selected_isa_);
	AfxGetApp()->WriteProfileInt(SECTION, L"Type", type_int_);
	AfxGetApp()->WriteProfileInt(SECTION, L"Filter", file_filter_index_);
}
