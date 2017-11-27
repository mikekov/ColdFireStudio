/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "ProtectedCall.h"


namespace {
	const TCHAR APP[]= L"ColdFire Studio";
}


bool ProtectedCallMsg(const std::function<void(CString& ret_msg)>& f, CString failure_message)
{
	CString msg;
	auto ok= false;

	try
	{
		f(msg);
		ok = true;
	}
	catch (const char* ex)
	{
		msg = ex;
	}
	catch (std::exception& ex)
	{
		msg = ex.what();
	}
	catch (CException* ex)
	{
		TCHAR buf[MAX_PATH];
		ex->GetErrorMessage(buf, static_cast<UINT>(array_count(buf)));
		msg = buf;
	}
	catch (...)
	{
		msg = "Unknown error";
	}

	if (!msg.IsEmpty())
		AfxGetMainWnd()->MessageBox(failure_message + L"\n\n" + msg, APP, MB_OK | MB_ICONERROR);

	return ok;
}


bool ProtectedCall(const std::function<void()>& f, CString failure_message)
{
	return ProtectedCallMsg(std::bind(f), failure_message);
}
