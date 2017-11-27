/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#pragma once


#include "Global.h"

#ifndef __AFXWIN_H__
#error include 'pch.h' before including this file for PCH
#endif


class StudioApp : public CWinApp
{
	static const TCHAR REGISTRY_KEY[];
	static const TCHAR PROFILE_NAME[];
	HINSTANCE rsc_inst_;
	//HMODULE rich_edit_;

public:
	static bool maximize_;	// maximize editor window at start-up
	static bool file_new_;	// open new empty doc at start-up
	Global global_;

	StudioApp();

	// Overrides
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

	// Implementation

	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
};


extern StudioApp theApp;
