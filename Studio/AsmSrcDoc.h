/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#pragma once
#include "scintilla-mfc/scintilladocview.h"


class AsmSrcDoc : public CScintillaDoc
{
protected:
	AsmSrcDoc();
	DECLARE_DYNCREATE(AsmSrcDoc)

public:
	virtual BOOL OnNewDocument();
	virtual BOOL OnOpenDocument(LPCTSTR path_name);

	CString GetStatusMessage() const;
	void SetStatusMessage(const wchar_t* msg);
	void SetStatusMessage(const std::wstring& msg);

	// Implementation
public:
	virtual ~AsmSrcDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	DECLARE_MESSAGE_MAP()

private:
	CString status_msg_;
};
