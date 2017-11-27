/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "AsmSrcDoc.h"
#include "Utilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// AsmSrcDoc

IMPLEMENT_DYNCREATE(AsmSrcDoc, CScintillaDoc)

BEGIN_MESSAGE_MAP(AsmSrcDoc, CScintillaDoc)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// AsmSrcDoc construction/destruction

AsmSrcDoc::AsmSrcDoc()
{}

AsmSrcDoc::~AsmSrcDoc()
{}

BOOL AsmSrcDoc::OnNewDocument()
{
	if (!CScintillaDoc::OnNewDocument())
		return FALSE;

	static UINT no= 1;
	CString name;
	name.Format(_T("NewFile %u"), no++);
	SetPathName(name, false);

	// read template source file
	Path path= GetTemplatesPath() / "Template.cfs";
	if (boost::filesystem::exists(path))
	{
		if (auto view= GetView())
		{
			auto file= ReadFileIntoString(path);
			view->GetCtrl().SetText(file.c_str());
			view->GetCtrl().EmptyUndoBuffer();
		}
	}

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// AsmSrcDoc diagnostics

#ifdef _DEBUG
void AsmSrcDoc::AssertValid() const
{
	CScintillaDoc::AssertValid();
}

void AsmSrcDoc::Dump(CDumpContext& dc) const
{
	CScintillaDoc::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////

BOOL AsmSrcDoc::OnOpenDocument(LPCTSTR path_name)
{
	if (CDocument::OnOpenDocument(path_name))
	{
		GetView()->GetCtrl().Colorize(0, -1);
		return true;
	}

	return false;
}


CString AsmSrcDoc::GetStatusMessage() const
{
	return status_msg_;
}


void AsmSrcDoc::SetStatusMessage(const wchar_t* msg)
{
	if (msg)
		status_msg_ = msg;
	else
		status_msg_.Empty();

	// ask main frame to refresh status
	if (CFrameWnd* frame= dynamic_cast<CFrameWnd*>(AfxGetMainWnd()))
		frame->SetMessageText(AFX_IDS_IDLEMESSAGE);
}


void AsmSrcDoc::SetStatusMessage(const std::wstring& msg)
{
	SetStatusMessage(msg.c_str());
}
