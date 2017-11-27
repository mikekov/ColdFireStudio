/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#pragma once

#include "Broadcast.h"
#include "Debugger.h"
#include "Defs.h"


class Global
{
public:
	Global();
	~Global();

	Debugger& GetDebugger() const;

	void SetDeasmDoc(CDocTemplate* doc);

	bool CreateDisasmView();

	void GetOrCreateDisasmView();

private:
	bool code_present_;
	std::unique_ptr<Debugger> debugger_;
	CDocTemplate* deasm_doc_;
};
