/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

// LoadCode.cpp : helper file to delegate code loading to the right function
//

#include "pch.h"
#include "LoadCode.h"
#include "IntelHex.h"
#include "MotorolaSRecord.h"


BinaryFormat GuessFormat(const Path& file)
{
	auto ext= file.extension();
	BinaryFormat fmt= BinaryFormat::AutoDetect;

	if (boost::iequals(ext.c_str(), L".hex"))
		fmt = BinaryFormat::Hex;
	else if (boost::iequals(ext.c_str(), L".cfp"))
		fmt = BinaryFormat::CFProgram;
	else if (boost::iequals(ext.c_str(), L".bin"))
		fmt = BinaryFormat::RawBinary;
	else if (boost::iequals(ext.c_str(), L".prg"))
		fmt = BinaryFormat::AtariPrg;
	else if (boost::iequals(ext.c_str(), L".srec") || boost::iequals(ext.c_str(), L".s9") || boost::iequals(ext.c_str(), L".s19"))
		fmt = BinaryFormat::SRecord;

	return fmt;
}


cf::BinaryProgram LoadCode(const Path& path, BinaryFormat fmt, ISA isa, cf::uint32 begin)
{
	if (fmt == BinaryFormat::AutoDetect)
	{
		fmt = GuessFormat(path);

		if (fmt == BinaryFormat::AutoDetect)
			fmt = BinaryFormat::RawBinary;
	}

	cf::BinaryProgram code;
	bool set_isa= true;

	switch (fmt)
	{
	case BinaryFormat::Hex:
		code = LoadHexCode(path);
		break;

	case BinaryFormat::SRecord:
		code = LoadSRecordCode(path);
		break;

	case BinaryFormat::CFProgram:
		code = cf::LoadBinaryProgram(path.c_str());
		set_isa = false;
		break;

	case BinaryFormat::RawBinary:
		code = cf::LoadBinaryCode(path.c_str(), isa, begin);
		break;

	case BinaryFormat::AtariPrg:
		code = cf::LoadBinaryCode(path.c_str(), isa, begin);
		break;

	default:
		ASSERT(false);
		break;
	}

	if (set_isa)
		code.SetIsa(isa);

	return code;
}
