/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

// Source code editor based on Scintilla

#include "pch.h"
#include "MainFrame.h"	// todo: remove
#include "AsmSrcDoc.h"
#include "AsmSrcView.h"
#include "SciLexer.h"
#include "resource.h"
#include "Assembler.h"
#include "Settings.h"
#include "ProtectedCall.h"
#include "TypeTranslators.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

const int MARKER_BOOKMARK= 4;
const int MARKER_POINTER= 3;
const int MARKER_ERROR= 2;
const int MARKER_BREAKPOINT= 1;

/////////////////////////////////////////////////////////////////////////////
// AsmSrcView

IMPLEMENT_DYNCREATE(AsmSrcView, CScintillaView)

BEGIN_MESSAGE_MAP(AsmSrcView, BaseView)
	ON_WM_CREATE()
	ON_COMMAND(ID_EDIT_TOGGLE_BOOKMARK, &AsmSrcView::ToggleBookmark)
	ON_COMMAND(ID_EDIT_GOTO_NEXT_BOOKMARK, &AsmSrcView::GoToBookmark)
	ON_COMMAND(ID_EDIT_VIEW_TABS, OnToggleWhiteSpace)
	ON_UPDATE_COMMAND_UI(ID_EDIT_VIEW_TABS, OnUpdateToggleWhiteSpace)
	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_FILE_PRINT, BaseView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, BaseView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, BaseView::OnFilePrintPreview)
	ON_MESSAGE(Broadcast::WM_USER_REMOVE_ERR_MARK, OnRemoveErrMark)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// AsmSrcView construction/destruction

AsmSrcView::AsmSrcView() : SettingsClient(L"Editor window")
{
	pointer_line_ = -1;
	err_mark_line_ = -1;
}

AsmSrcView::~AsmSrcView()
{}

BOOL AsmSrcView::PreCreateWindow(CREATESTRUCT& cs)
{
	bool pre_created = BaseView::PreCreateWindow(cs);
	cs.dwExStyle &= ~WS_EX_CLIENTEDGE;
	return pre_created;
}

/////////////////////////////////////////////////////////////////////////////
// AsmSrcView printing

BOOL AsmSrcView::OnPreparePrinting(CPrintInfo* info)
{
	// default BaseView preparation
	return BaseView::OnPreparePrinting(info);
}

void AsmSrcView::OnBeginPrinting(CDC* dc, CPrintInfo* info)
{
	// Default BaseView begin printing.
	BaseView::OnBeginPrinting(dc, info);
}

void AsmSrcView::OnEndPrinting(CDC* dc, CPrintInfo* info)
{
	// Default BaseView end printing
	BaseView::OnEndPrinting(dc, info);
}

#ifdef _DEBUG
AsmSrcDoc* AsmSrcView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(AsmSrcDoc)));
	return (AsmSrcDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////

void AsmSrcView::ApplySettings(SettingsSection& settings)
{
	auto& ed= settings.section("editor");
	auto editor_font= ed.get_font("font");

	auto& ctrl= GetCtrl();

	ctrl.SetLexer(SCLEX_ASM);
	ctrl.StyleSetFont(STYLE_DEFAULT, editor_font.lfFaceName);
	int font_height= abs(editor_font.lfHeight);
	{
		// this is reverse of what Scintilla does to calc font hight for GDI
		CClientDC dc(this);
		int log_y= dc.GetDeviceCaps(LOGPIXELSY);
		if (log_y == 0)
			log_y = 96;
		font_height = MulDiv(72, font_height * 100, log_y);
	}
	ctrl.StyleSetSizeFractional(STYLE_DEFAULT, font_height);

	auto isa= StringToISA(settings.get_string("asm.isa"));

	CString mnemonics(Assembler().GetAllMnemonics(isa).c_str());

	enum KeyWords { Cpu= 0, Fpu, Regs, Directives, DirectivesOp, ExtInstr };
	ctrl.SetKeyWords(Cpu, mnemonics.MakeLower());
	// keep in sync with CpuRegister type
	ctrl.SetKeyWords(Regs, "d0 d1 d2 d3 d4 d5 d6 d7 a0 a1 a2 a3 a4 a5 a6 a7 sp usp ccr sr pc mbar vbr "
		"cacr mmubar rambar0 rambar1 rombar0 rombar1 acc0 acc1 acc2 acc3 acr0 acr1 acr2 acr3 asid");
	ctrl.SetKeyWords(Directives, "dc.l dc.b dc.w dcb.b dcb.w dcb.l org ds.b ds.w ds.l repeat endr macro endm exitm set end align if else endif def ref strlen passdef error");	// todo: more, section, etc.

	auto colors= ed.section("text_colors");
	auto comment= colors.get_color("comments");
	auto string= colors.get_color("strings");

	ctrl.StyleSetFore(SCE_ASM_DEFAULT, colors.get_color("text"));
	ctrl.StyleSetFore(SCE_ASM_COMMENT, comment);
	ctrl.StyleSetItalic(SCE_ASM_COMMENT, true);
	ctrl.StyleSetFore(SCE_ASM_COMMENTBLOCK, comment);
	ctrl.StyleSetFore(SCE_ASM_NUMBER, colors.get_color("numbers"));
	ctrl.StyleSetFore(SCE_ASM_CPUINSTRUCTION, colors.get_color("mnemonics"));
	ctrl.StyleSetFore(SCE_ASM_STRING, string);
	ctrl.StyleSetFore(SCE_ASM_CHARACTER, string);
	auto directives= colors.get_color("directives");
	ctrl.StyleSetFore(SCE_ASM_DIRECTIVE, directives);
	ctrl.StyleSetFore(SCE_ASM_DIRECTIVEOPERAND, directives);
	ctrl.StyleSetFore(SCE_ASM_REGISTER, colors.get_color("registers"));
	ctrl.StyleSetFore(SCE_ASM_OPERATOR, directives);

	ctrl.StyleSetFont(SCE_ASM_CPUINSTRUCTION, editor_font.lfFaceName);
	ctrl.StyleSetSizeFractional(SCE_ASM_CPUINSTRUCTION, font_height);
	ctrl.StyleSetBold(SCE_ASM_CPUINSTRUCTION, ed.get_bool("bold_keywords"));

	ctrl.StyleSetFont(SCE_ASM_DIRECTIVE, editor_font.lfFaceName);
	ctrl.StyleSetSizeFractional(SCE_ASM_DIRECTIVE, font_height);
	ctrl.StyleSetBold(SCE_ASM_DIRECTIVE, ed.get_bool("bold_directives"));

	ctrl.StyleSetFont(SCE_ASM_COMMENT, editor_font.lfFaceName);
	ctrl.StyleSetSizeFractional(SCE_ASM_COMMENT, font_height);
	ctrl.StyleSetItalic(SCE_ASM_COMMENT, ed.get_bool("italic_comments"));

	auto markers= ed.section("marker_colors");

	ctrl.MarkerDefine(MARKER_BOOKMARK, SC_MARK_CIRCLE);
	ctrl.MarkerSetBack(MARKER_BOOKMARK, markers.get_color("bookmark"));

	ctrl.MarkerDefine(MARKER_POINTER, SC_MARK_ARROW);
	ctrl.MarkerSetBack(MARKER_POINTER, markers.get_color("current"));

	ctrl.MarkerDefine(MARKER_BREAKPOINT, SC_MARK_ROUNDRECT);
	ctrl.MarkerSetBack(MARKER_BREAKPOINT, markers.get_color("breakpoint"));

	ctrl.MarkerDefine(MARKER_ERROR, SC_MARK_ARROW);
	ctrl.MarkerSetBack(MARKER_ERROR, markers.get_color("error"));

	ctrl.SetWhitespaceFore(true, colors.get_color("whitespace"));

	ctrl.SetTabWidth(ed.get_int("tab_size"));
}


int AsmSrcView::OnCreate(LPCREATESTRUCT create_struct)
{
	if (BaseView::OnCreate(create_struct) == -1)
		return -1;

	bool ok= ProtectedCall([&]
	{
		auto& ctrl= GetCtrl();

		CallApplySettings();

//#define SCE_ASM_DEFAULT 0
//#define SCE_ASM_COMMENT 1
//#define SCE_ASM_NUMBER 2
//#define SCE_ASM_STRING 3
//#define SCE_ASM_OPERATOR 4
//#define SCE_ASM_IDENTIFIER 5
//#define SCE_ASM_CPUINSTRUCTION 6
//#define SCE_ASM_MATHINSTRUCTION 7
//#define SCE_ASM_REGISTER 8
//#define SCE_ASM_DIRECTIVE 9
//#define SCE_ASM_DIRECTIVEOPERAND 10
//#define SCE_ASM_COMMENTBLOCK 11
//#define SCE_ASM_CHARACTER 12
//#define SCE_ASM_STRINGEOL 13
//#define SCE_ASM_EXTINSTRUCTION 14
//#define SCE_ASM_COMMENTDIRECTIVE 15

		ctrl.Colorize(0, -1);

		ctrl.SetMarginWidthN(1, 18);

		//ctrl.SetMarginWidthN(2, 10);
		//ctrl.SetProperty("fold", "1");
		DWORD pixels= 1;
		::SystemParametersInfo(SPI_GETCARETWIDTH, 0, &pixels, 0);
		ctrl.SetCaretWidth(pixels);

		ctrl.SetWhitespaceSize(3);
	}, "Editor initialization failed");

	return ok ? 0 : -1;
}

//=============================================================================

int AsmSrcView::ScrollToLine(int line, bool scroll)
{
	ASSERT(line >= 0);

	GetCtrl().GotoLine(line);
	return 0;
}


//-----------------------------------------------------------------------------


void AsmSrcView::SetPointer(int line, bool scroll)
{
	if (pointer_line_ != -1)
	{
		int tmp_line= pointer_line_;
		GetCtrl().MarkerDelete(pointer_line_, MARKER_POINTER);
		pointer_line_ = -1;
	}
	pointer_line_ = line;
	if (line != -1)
	{
		ScrollToLine(line, TRUE);
		GetCtrl().MarkerAdd(line, MARKER_POINTER);
	}
}


void AsmSrcView::SetErrMark(int line)
{
	if (err_mark_line_ != -1)
		GetCtrl().MarkerDelete(err_mark_line_, MARKER_ERROR);

	err_mark_line_ = line;

	if (line != -1)
	{
		ScrollToLine(line, TRUE);
		err_mark_line_ = line;
		GetCtrl().MarkerAdd(line, MARKER_ERROR);
	}
}


void AsmSrcView::GoToBookmark()
{
	auto line= GetCurrLineNo();
	line = GetCtrl().MarkerNext(line + 1, 1 << MARKER_BOOKMARK);
	if (line < 0)
		line = GetCtrl().MarkerNext(0, 1 << MARKER_BOOKMARK);
	if (line >= 0)
		GetCtrl().GotoLine(line);
}


void AsmSrcView::ToggleBookmark()
{
	int line= GetCurrLineNo();
	auto m= GetCtrl().MarkerGet(line);
	if (m & (1 << MARKER_BOOKMARK))
		GetCtrl().MarkerDelete(line, MARKER_BOOKMARK);
	else
		GetCtrl().MarkerAdd(line, MARKER_BOOKMARK);
}


int AsmSrcView::GetCurrLineNo()
{
	return GetCtrl().LineFromPosition(GetCtrl().GetCurrentPos());
}


void AsmSrcView::AddBreakpoint(int line, Defs::Breakpoint bp)
{
	GetCtrl().MarkerAdd(line, MARKER_BREAKPOINT);
}


void AsmSrcView::RemoveBreakpoint(int line)
{
	GetCtrl().MarkerDelete(line, MARKER_BREAKPOINT);
}


void AsmSrcView::ClearAllBreakpoints()
{
	GetCtrl().MarkerDeleteAll(MARKER_BREAKPOINT);
}


std::set<int> AsmSrcView::GetAllBreakpoints()
{
	std::set<int> breakpoints;

	for (int line= 0; ; )
	{
		line = GetCtrl().MarkerNext(line, 1 << MARKER_BREAKPOINT);
		if (line == -1)
			break;
		breakpoints.insert(line++);
	}

	return breakpoints;
}


void AsmSrcView::OnContextMenu(CWnd* wnd, CPoint point)
{
	CMenu menu;
	if (!menu.LoadMenu(IDR_POPUP_EDIT))
		return;
	CMenu* popup= menu.GetSubMenu(0);
	ASSERT(popup != nullptr);

	if (point.x == -1 && point.y == -1)
	{
		CRect rect;
		GetClientRect(rect);

		point = rect.TopLeft();
		CPoint top_left(0, 0);
		ClientToScreen(&top_left);
		point.x = top_left.x + rect.Width() / 2;
		point.y = top_left.y + rect.Height() / 2;
	}

	if (popup)
		popup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, AfxGetMainWnd());
}


LRESULT AsmSrcView::OnRemoveErrMark(WPARAM wParam, LPARAM lParam)
{
	SetErrMark(-1);
	return 1;
}


void AsmSrcView::GetText(CString& text)
{
	GetCtrl().GetWindowText(text);
}


void AsmSrcView::OnModified(SCNotification* notification)
{
	// clear error mark (if any) when user starts typing
	if (err_mark_line_ != -1 && (notification->modificationType & (SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT)))
	{
		SetErrMark(-1);
		GetDocument()->SetStatusMessage(nullptr);
	}
}


void AsmSrcView::OnUpdateUI(SCNotification* notification)
{
	long pos= GetCtrl().GetCurrentPos();
	int line= GetCtrl().LineFromPosition(pos);
	int col= GetCtrl().GetColumn(pos);

	MainFrame* main = (MainFrame*) AfxGetApp()->m_pMainWnd;
	main->UpdateTextPosition(line + 1, col + 1, !GetCtrl().GetOvertype());
}


void AsmSrcView::OnDestroy()
{}


void AsmSrcView::OnToggleWhiteSpace()
{
	if (GetCtrl().GetViewWS())
		GetCtrl().SetViewWS(SCWS_INVISIBLE);
	else
		GetCtrl().SetViewWS(SCWS_VISIBLEALWAYS);
}


void AsmSrcView::OnUpdateToggleWhiteSpace(CCmdUI* cmdui)
{
	cmdui->Enable(true);
	cmdui->SetCheck(GetCtrl().GetViewWS() ? 1 : 0);
}


void AsmSrcView::SetPointer(int line, const std::wstring& doc_path, cf::uint32 pc, bool scroll)
{
//	AfxComparePath(0, 0);
	if (GetDocument()->GetPathName() == doc_path.c_str())
		SetPointer(line, scroll);
	else
		SetPointer(-1, false);
}
