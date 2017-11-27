/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

// MemoryBar.cpp : implementation file
//

#include "pch.h"
#include "resource.h"
#include "MemoryBar.h"
#include "UIElements.h"
#include "HexViewWnd.h"
#include "Debugger.h"
extern Debugger& GetDebugger();


// MemoryBar dialog

IMPLEMENT_DYNAMIC(MemoryBar, CDialog)

MemoryBar::MemoryBar(HexViewWnd& view, CWnd* parent) : CDialog(MemoryBar::IDD, parent), view_(view)
{
	backgnd_.CreateSolidBrush(::GetSysColor(COLOR_WINDOW));
	backgnd_read_only_.CreateSolidBrush(::GetSysColor(COLOR_BTNFACE));
	read_only_ = false;
}

MemoryBar::~MemoryBar()
{}

void MemoryBar::DoDataExchange(CDataExchange* dx)
{
	CDialog::DoDataExchange(dx);
	DDX_Control(dx, IDC_TBAR, tbar_);
	DDX_Control(dx, IDC_TYPES, sizes_);
}

enum : int { CMD_HEX= 10, CMD_ASCII, CMD_BOTH, CMD_BYTE, CMD_WORD, CMD_LONG };


BEGIN_MESSAGE_MAP(MemoryBar, CDialog)
	ON_COMMAND_RANGE(CMD_BYTE, CMD_LONG, &MemoryBar::OnClickedBWL)
	ON_COMMAND_RANGE(CMD_HEX, CMD_BOTH, &MemoryBar::OnClickedType)
	ON_WM_CTLCOLOR() // - white backgnd
END_MESSAGE_MAP()


// MemoryBar message handlers

void MemoryBar::OnClickedBWL(UINT cmd)
{
	int group= 1;
	switch (cmd)
	{
	case CMD_WORD:
		group = 2;
		break;
	case CMD_LONG:
		group = 4;
		break;
	}

	view_.SetGrouping(group);
}


void MemoryBar::OnClickedType(UINT cmd)
{
	UINT mask= 0;

	switch (cmd)
	{
	case CMD_HEX:
		mask = HVS_ASCII_INVISIBLE;
		break;
	case CMD_ASCII:
		mask = HVS_HEX_INVISIBLE;
		break;
	case CMD_BOTH:
		mask = 0;
		break;
	}

	view_.SetStyle(HVS_ASCII_INVISIBLE | HVS_HEX_INVISIBLE, mask);
}


bool MemoryBar::Create(CWnd* parent)
{
	return CDialog::Create(IDD, parent);
}


BOOL MemoryBar::OnInitDialog()
{
	address_.Subclass(this, IDC_ADDRESS);

	CDialog::OnInitDialog();

	GetHexEditFont(hex_font_);

	address_.SetFont(hex_font_);
	address_.ChangeCallback(std::bind(&MemoryBar::OnAddressChanged, this, std::placeholders::_1));

	static cf::Register registers[]=
	{
		cf::R_D0, cf::R_D1, cf::R_D2, cf::R_D3, cf::R_D4, cf::R_D5, cf::R_D6, cf::R_D7,
		cf::R_A0, cf::R_A1, cf::R_A2, cf::R_A3, cf::R_A4, cf::R_A5, cf::R_A6, cf::R_A7,
		cf::R_SP, cf::R_PC, cf::R_USP
	};

	for (auto reg : registers)
	{
		auto name= cf::GetRegisterName(reg);
		address_.AddSymbol(name, static_cast<int>(reg));
	}

	CSize padding(3, 11);
	auto font= GetFont();

	TBBUTTON buttons[]=
	{
		{
			-1,
			CMD_HEX,
			TBSTATE_ENABLED,
			BTNS_CHECKGROUP | BTNS_AUTOSIZE | BTNS_SHOWTEXT,
			{ 0 },
			0,
			0
		},
		{
			-1,
			CMD_ASCII,
			TBSTATE_ENABLED,
			BTNS_CHECKGROUP | BTNS_AUTOSIZE | BTNS_SHOWTEXT,
			{ 0 },
			0,
			1
		},
		{
			-1,
			CMD_BOTH,
			TBSTATE_ENABLED | TBSTATE_CHECKED,
			BTNS_CHECKGROUP | BTNS_AUTOSIZE | BTNS_SHOWTEXT,
			{ 0 },
			0,
			2
		}
	};

	tbar_.SetButtonStructSize(sizeof(buttons[0]));
	tbar_.SetFont(font);
	tbar_.SetBitmapSize(CSize(0, 0));
	tbar_.SetPadding(padding.cx, padding.cy);
	tbar_.AddStrings(L"Hex\0ASCII\0Both\0");
	tbar_.AddButtons(static_cast<int>(array_count(buttons)), buttons);

	TBBUTTON types[]=
	{
		{
			-1,
			CMD_BYTE,
			TBSTATE_ENABLED | TBSTATE_CHECKED,
			BTNS_CHECKGROUP | BTNS_AUTOSIZE | BTNS_SHOWTEXT,
			{ 0 },
			0,
			0
		},
		{
			-1,
			CMD_WORD,
			TBSTATE_ENABLED,
			BTNS_CHECKGROUP | BTNS_AUTOSIZE | BTNS_SHOWTEXT,
			{ 0 },
			0,
			1
		},
		{
			-1,
			CMD_LONG,
			TBSTATE_ENABLED,
			BTNS_CHECKGROUP | BTNS_AUTOSIZE | BTNS_SHOWTEXT,
			{ 0 },
			0,
			2
		}
	};

	sizes_.SetButtonStructSize(sizeof(types[0]));
	sizes_.SetFont(font);
	sizes_.SetBitmapSize(CSize(0, 0));
	sizes_.SetPadding(padding.cx, padding.cy);
	sizes_.AddStrings(L"B\0W\0L\0");	// Byte/Word/Long word
	sizes_.AddButtons(static_cast<int>(array_count(types)), types);

	//spin_.SetRange32(0, 1000000);
	address_.SetValue(0);

	return TRUE;  // return TRUE unless you set the focus to a control
}


void MemoryBar::OnOK()
{
	OnAddressChanged(address_);
}

void MemoryBar::OnCancel()
{}


HBRUSH MemoryBar::OnCtlColor(CDC* dc, CWnd* wnd, UINT ctlColor)
{
	HBRUSH br= CDialog::OnCtlColor(dc, wnd, ctlColor);
	dc->SetBkColor(::GetSysColor(read_only_ ? CTLCOLOR_DLG : COLOR_WINDOW));
	return read_only_ ? backgnd_read_only_ : backgnd_;
}


void MemoryBar::OnAddressChanged(EditBox& edit)
{
	Refresh(edit, true);
}


void MemoryBar::Refresh(EditBox& edit, bool force)
{
	if (!view_.m_hWnd)
		return;

	unsigned int number= 0;
	CString text;
	switch (edit.GetEntry(&number, &text))
	{
	case EditBox::DecNum:
	case EditBox::HexNum:
	case EditBox::DollarNum:
		if (force)
			view_.ScrollTop(number);
		break;

	case EditBox::Symbol:	// symbol (a register name)
		{
			auto reg= static_cast<cf::Register>(number);
			auto val= GetDebugger().GetRegister(reg);
			view_.ScrollTop(val);
		}

	case EditBox::Expression:
		//todo: parse expression
		//
		break;
	}
}


void MemoryBar::SetReadOnly(bool read_only)
{
	if (read_only_ != read_only)
	{
		read_only_ = read_only;
		Invalidate();
	}
}


void MemoryBar::Notify(int event, UINT data, Debugger& debugger)
{
	if (event == cf::E_RUNNING)
		return;

	// if using register in an address edit box, refresh the window; register may have changed and points to a different area
	Refresh(address_, false);
}


void MemoryBar::WatchRegister(cf::Register reg)
{
	address_.SetText(CString(cf::GetRegisterName(reg)));
}


void MemoryBar::SetGrouping(int g)
{
	int cmd= 0;
	switch (g)
	{
	case 1:
		cmd = CMD_BYTE;
		break;
	case 2:
		cmd = CMD_WORD;
		break;
	case 4:
		cmd = CMD_LONG;
		break;
	default:
		ASSERT(false);
		break;
	}

	if (cmd)
	{
		sizes_.CheckButton(cmd);
		OnClickedBWL(cmd);
	}
}


void MemoryBar::SetHexOnly()
{
	int cmd= CMD_HEX;
	tbar_.CheckButton(cmd);
	OnClickedType(cmd);
}
