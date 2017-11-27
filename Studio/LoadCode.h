/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#pragma once

#include "../ColdFire/BinaryProgram.h"

enum class BinaryFormat { AutoDetect, RawBinary, AtariPrg, CFProgram, SRecord, Hex };

// Load binary code
cf::BinaryProgram LoadCode(const Path& path, BinaryFormat fmt, ISA isa, cf::uint32 begin);

BinaryFormat GuessFormat(const Path& file);
