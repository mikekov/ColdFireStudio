/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#pragma once
#include "HexViewWnd.h"
#include "MemoryBar.h"
#include "MemorySource.h"
#include "SettingsClient.h"
#include "scbarcf.h"


class MemoryWnd : public CSizingControlBarCF, SettingsClient
{
	typedef CSizingControlBarCF Base;
public:
	MemoryWnd();
	virtual ~MemoryWnd();

	void Notify(int event, UINT data, Debugger& debugger);

	void SetSource(MemorySource& mem);

	void WatchRegister(cf::Register reg);
	void SetGrouping(int g);
	void SetHexOnly();

private:
	int OnCreate(LPCREATESTRUCT create_struct);
	void OnSize(UINT nType, int cx, int cy);
	void Resize();
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	void OnMemoryChange(NMHDR* hdr, LRESULT* result);
	void OnEditModeChange(NMHDR* hdr, LRESULT* result);
	void OnReadMemory(NMHDR* hdr, LRESULT* result);
	virtual void ApplySettings(SettingsSection& settings);

	MemoryBar bar_;
	HexViewWnd view_;
	unsigned int base_addr_;
	CFont font_;
	MemorySource memory_;

	DECLARE_MESSAGE_MAP()
};
