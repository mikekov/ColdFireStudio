/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#pragma once
#ifndef _motorola_s_record_
#define _motorola_s_record_


#include "..\ColdFire\BinaryProgram.h"

void SaveSRecordCode(const Path& path, const cf::BinaryProgram& prog);
void SaveSRecordCode(std::ostream& out, const cf::BinaryProgram& prog);

cf::BinaryProgram LoadSRecordCode(const Path& path);


#endif
