/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "Global.h"
#include "Broadcast.h"
#include <io.h>
#include <fcntl.h>
#include "DisasmDoc.h"
#include "Settings.h"
#include "ProtectedCall.h"
#include "Utilities.h"


Global::Global()
{
	deasm_doc_ = nullptr;

	// read resources
	auto rsc_inst= LoadLibrary(L"ResDLL.dll");
	if (rsc_inst == nullptr)
		AfxMessageBox(L"Can't find resource DLL 'ResDLL.dll'.", MB_OK | MB_ICONERROR);
	else
		AfxSetResourceHandle(rsc_inst);

	AppSettings().Load();

	ProtectedCall([&]()
	{
		debugger_.reset(new Debugger());
	}, "Simulator creation failed.");

	// load monitor program
	{
		Path monitor= GetMonitorPath() / "monitor.cfp";

		ProtectedCall([&]()
		{
			debugger_->LoadMonitorCode(monitor.c_str());
		}, L"Cannot load monitor program. Simulation will be impacted.\nFile: " + CString(monitor.c_str()));
	}
}


Global::~Global()
{}


//-----------------------------------------------------------------------------

Debugger& Global::GetDebugger() const
{
	return *debugger_;
}


bool Global::CreateDisasmView()
{
	DisasmDoc* doc= static_cast<DisasmDoc*>(deasm_doc_->OpenDocumentFile(nullptr));

	doc->SetDebugger(debugger_.get());

	return true;
}


void Global::GetOrCreateDisasmView()
{
	auto pos= deasm_doc_->GetFirstDocPosition();
	auto doc= pos ? deasm_doc_->GetNextDoc(pos) : nullptr;
	if (doc == nullptr)
		CreateDisasmView();

	pos = deasm_doc_->GetFirstDocPosition();
	doc = pos ? deasm_doc_->GetNextDoc(pos) : nullptr;
	if (doc == nullptr)
		return;

	if (auto pos2= doc->GetFirstViewPosition())
		if (auto view= doc->GetNextView(pos2))
			if (auto frame= view->GetParentFrame())
				frame->ActivateFrame();
}


void Global::SetDeasmDoc(CDocTemplate* doc)
{
	deasm_doc_ = doc;
}
