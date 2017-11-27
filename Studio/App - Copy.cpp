/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "App.h"
#include "MainFrame.h"
#include "ChildFrm.h"
#include "AsmSrcDoc.h"
#include "AsmSrcView.h"
#include "DisasmDoc.h"
#include "DisasmView.h"
#include "CXMultiDocTemplate.h"
#include "Settings.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

const TCHAR StudioApp::REGISTRY_KEY[]= _T("MiKSoft");
const TCHAR StudioApp::PROFILE_NAME[]= _T("ColdFireStudio\\0.1");

// pray struct layout is the same as in axfimpl.h
struct AUX_DATA
{
	// system metrics
	int cxVScroll, cyHScroll;
	int cxIcon, cyIcon;

	int cxBorder2, cyBorder2;

	// device metrics for screen
	int cxPixelsPerInch, cyPixelsPerInch;

	// convenient system color
	HBRUSH hbrWindowFrame;
	HBRUSH hbrBtnFace;

	// color values of system colors used for CToolBar
	COLORREF clrBtnFace, clrBtnShadow, clrBtnHilite;
	COLORREF clrBtnText, clrWindowFrame;

	// standard cursors
	HCURSOR hcurWait;
	HCURSOR hcurArrow;
	HCURSOR hcurHelp;       // cursor used in Shift+F1 help

	// special GDI objects allocated on demand
	HFONT   hStatusFont;
	HFONT   hToolTipsFont;
	HBITMAP hbmMenuDot;
};

extern AFX_DATA_IMPORT AUX_DATA afxData;

BEGIN_MESSAGE_MAP(StudioApp, CWinApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
	ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////

struct B : BITMAPINFO
{
	RGBQUAD c[15];
};

void test()
{
	CDC src, dst;
	int width= 32;
	int height= 8;
	CBitmap mono;
	BITMAPINFO bh= { sizeof(BITMAPINFOHEADER), width, height, 1, 1, DIB_RGB_COLORS, 0, 0, 0, 2, 2 };
	BYTE data[32]= { 0xa, 0xf, 0x0, 0x1, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0xff, 0xaa, 0x55, 0x0f };
	mono.Attach(::CreateDIBSection(nullptr, &bh, 0, nullptr, nullptr, 0));
	mono.SetBitmapBits(32, data);
	::GdiFlush();
	BITMAP bm;
	mono.GetBitmap(&bm);
	DIBSECTION dib0;
	mono.GetObject(sizeof(dib0), &dib0);

	CBitmap gray;
	int zoom= 3;
	BITMAPINFO b= { sizeof(BITMAPINFOHEADER), width * zoom, height * zoom, 1, 4, DIB_RGB_COLORS, 0, 0, 0, 16, 16 };
	B bh2;
	*(BITMAPINFO*)(&bh2) = b;
	RGBQUAD blk= { 0,0,0,0 };
	RGBQUAD wht= { 255,255,255,0 };
	RGBQUAD gry= { 127,127,127,0 };
	RGBQUAD lgr= { 63,63,63,0 };
	bh2.bmiColors[0] = blk;
	bh2.bmiColors[1] = wht;
	bh2.bmiColors[2] = gry;
	bh2.bmiColors[3] = lgr;
	gray.Attach(::CreateDIBSection(nullptr, &b, 0, nullptr, nullptr, 0));
	gray.GetBitmap(&bm);

	src.CreateCompatibleDC(0);
	dst.CreateCompatibleDC(0);
	src.SelectObject(&mono);
	dst.SelectObject(&gray);

	dst.SetBkColor(0xffffff);
	dst.SetTextColor(0x0);//ffffff);
	dst.StretchBlt(0, 0, bm.bmWidth, bm.bmHeight, &src, 0, 0, width, height, SRCCOPY);

	DIBSECTION dib;
	BYTE gray_level[]= { 0, 2 };
	BYTE lgray_level[]= { 0, 3 };
	if (gray.GetObject(sizeof(dib), &dib) && zoom > 1)
	{
		BYTE* buffer= static_cast<BYTE*>(dib.dsBm.bmBits);
		BYTE* line= buffer;
		for (int y= 0; y < dib.dsBm.bmHeight; ++y)
		{
			if (y % zoom == zoom - 1)
			{
				BYTE* p= line;

				for (int x= 0; x < dib.dsBm.bmWidth; ++x)
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

				for (int x= 0; x < dib.dsBm.bmWidth; ++x)
				{
					*p = gray_level[*p];
					p += zoom;
				}
			}

			line += dib.dsBm.bmWidthBytes;
		}
	}

	int a= 0;
}



StudioApp::StudioApp()
{
test();
	rsc_inst_ = nullptr;
	//rich_edit_ = 0;

	// no more messing with position of docked panels, stupid MFC!
	afxData.cyBorder2 = afxData.cxBorder2 = 0;

	EnableHtmlHelp();
}

/////////////////////////////////////////////////////////////////////////////
// The one and only StudioApp object

StudioApp theApp;

/////////////////////////////////////////////////////////////////////////////
// StudioApp initialization

bool StudioApp::maximize_= false;
bool StudioApp::file_new_= true;

BOOL StudioApp::InitInstance()
{
	// read resources
	rsc_inst_ = LoadLibrary(L"ResDLL.dll");
	if (rsc_inst_ == nullptr)
	{
		AfxMessageBox(L"Can't find resource DLL 'ResDLL.dll'.", MB_OK | MB_ICONERROR);
		return false;
	}
	auto rsc_inst = AfxGetResourceHandle();
	AfxSetResourceHandle(rsc_inst_);

	// install monospaced font so we have one known fall back available
	if (HRSRC res= ::FindResource(rsc_inst_, MAKEINTRESOURCE(IDR_MONO_FONT), L"TTF_FONT"))
		if (HGLOBAL mem= ::LoadResource(rsc_inst_, res))
		{
			void* data= ::LockResource(mem);
			auto len= ::SizeofResource(rsc_inst_, res);

			DWORD fonts= 0;
			auto fonthandle= ::AddFontMemResourceEx(
				data,		// ttf font data
				len,		// size of font data
				nullptr,	// reserved, must be 0
				&fonts);	// number of fonts installed

			ASSERT(fonthandle != nullptr);
			// font will be "uninstalled" when app exists, no need to remember the handle
		}

	Scintilla_RegisterClasses(AfxGetInstanceHandle());

	//HMODULE hmod = ::LoadLibrary(L"SciLexer.DLL");
	//if (hmod == NULL)
	//{
	//	AfxMessageBox(L"Can't find Scintilla DLL 'SciLexer.dll'.", MB_OK | MB_ICONERROR);
	//	return false;
	//}

	AfxOleInit();

//	rich_edit_ = ::LoadLibrary(_T("riched20.dll"));
//	AfxInitRichEdit();

	InitHexView();

	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	InitCommonControls();

	ULONG_PTR gdi_plus_token_;
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	// Initialize GDI+hhh
	Gdiplus::GdiplusStartup(&gdi_plus_token_, &gdiplusStartupInput, nullptr);

	SetRegistryKey(REGISTRY_KEY);
	//First free the string allocated by MFC at CWinApp startup.
	//The string is allocated before InitInstance is called.
	free((void*)m_pszProfileName);
	//Change the name of the .INI file.
	//The CWinApp destructor will free the memory.
	m_pszProfileName = _tcsdup(PROFILE_NAME);

	LoadStdProfileSettings(_AFX_MRU_MAX_COUNT);  // Load standard INI file options (including MRU)

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.
	{
		CXMultiDocTemplate* doc_template= new CXMultiDocTemplate(
			IDR_CF_SRC_TYPE,
			RUNTIME_CLASS(AsmSrcDoc),
			RUNTIME_CLASS(CChildFrame), // custom MDI child frame
			RUNTIME_CLASS(AsmSrcView));

		AddDocTemplate(doc_template);
	}

	// disassembly for CF code
	{
		CXMultiDocTemplate* doc_template= new CXMultiDocTemplate(
			IDR_DEASM_TYPE,
			RUNTIME_CLASS(DisasmDoc),
			RUNTIME_CLASS(CChildFrame), // custom MDI child frame
			RUNTIME_CLASS(DisasmView),
			false);

		AddDocTemplate(doc_template);
		global_.SetDeasmDoc(doc_template);
	}

	// create main MDI Frame window
	MainFrame* main_frame = new MainFrame;
	if (!main_frame->LoadFrame(IDR_MAINFRAME))
		return false;
	m_pMainWnd = main_frame;

	// Enable drag/drop open
	m_pMainWnd->DragAcceptFiles();

	// Enable DDE Execute open
	EnableShellOpen();
	RegisterShellFileTypes(true);

	CXMultiDocTemplate::registration_ext_ = false;

	// Parse command line for standard shell commands, DDE, file open
	//  CCommandLineInfo cmdInfo;
	//  ParseCommandLine(cmdInfo);

	if (!file_new_ && cmdInfo.m_nShellCommand == CCommandLineInfo::FileNew)
		cmdInfo.m_nShellCommand = CCommandLineInfo::FileNothing;
	// Dispatch commands specified on the command line
	if (!ProcessShellCommand(cmdInfo))
		return false;

	if (maximize_ && (m_nCmdShow == SW_SHOWNORMAL || m_nCmdShow == SW_SHOWDEFAULT))
		m_nCmdShow = SW_MAXIMIZE;

	// The main window has been initialized, so show and update it.
	main_frame->ShowWindow(m_nCmdShow);
	main_frame->UpdateWindow();
	main_frame->SetFocus();	// move focus to the main window

	return true;
}

/////////////////////////////////////////////////////////////////////////////
// StudioApp commands


#include "About.h"

// App command to run the dialog
void StudioApp::OnAppAbout()
{
	AboutDlg aboutDlg;
	aboutDlg.DoModal();
}


int StudioApp::ExitInstance() 
{
	AppSettings().Save();

	if (rsc_inst_)
		::FreeLibrary(rsc_inst_);

	//if (rich_edit_)
	//	::FreeLibrary(rich_edit_);

	Scintilla_ReleaseResources();

	return CWinApp::ExitInstance();
}
