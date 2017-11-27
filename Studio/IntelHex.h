/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#pragma once
#ifndef _intel_hex_
#define _intel_hex_

#include "..\ColdFire\BinaryProgram.h"

void SaveHexCode(const Path& path, const cf::BinaryProgram& prog);
void SaveHexCode(std::ostream& out, const cf::BinaryProgram& prog);

cf::BinaryProgram LoadHexCode(const Path& path);

#endif
