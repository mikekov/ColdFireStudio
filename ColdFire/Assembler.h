/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#pragma once
#include "Import.h"
#include "ErrCodes.h"
#include "Isa.h"
#include "Types.h"
#include "DebugData.h"
#include "BinaryProgram.h"

// CF macro assembler; assembles given file creating binary program

class CF_DECL Assembler
{
public:
	Assembler();
	~Assembler();

	// assemble 'file'
	StatCode Assemble(const wchar_t* file, ISA isa, bool case_sensitive);

	// last error message
	std::string LastMessage() const;

	// line with error (or 0 if none)
	int LastLine() const;
	// path of the last source element (or empty string)
	const std::wstring& GetPath() const;

	// assembled program
	const cf::BinaryProgram& GetCode() const;

	std::unique_ptr<DebugData> TakeOverDebug();

	// get a space separated list of mnemonics in a given ISA
	std::string GetAllMnemonics(ISA isa) const;

private:
	Assembler(const Assembler&);
	Assembler& operator = (const Assembler&);

	struct Impl;
	Impl* impl_;
};
