/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

// DisasmDoc.cpp : implementation file
//

#include "pch.h"
#include "DisasmDoc.h"
#include "DisasmView.h"
#include "Debugger.h"
#include <sstream>
#include "Settings.h"
#include "ProtectedCall.h"


// DisasmDoc

IMPLEMENT_DYNCREATE(DisasmDoc, CDocument)

DisasmDoc::DisasmDoc() : SettingsClient(L"Disassembler document")
{
	debugger_ = nullptr;
	addr_ = 0;
	disasm_flags_ = 0;

	// base SettingsClient is constructed, so virtual call is resolved...
	CallApplySettings();
}


void DisasmDoc::ApplySettings(SettingsSection& settings)
{
	auto& d= settings.section("disasm");

	disasm_flags_ = 0;
	if (!d.get_bool("uppercase"))
		disasm_flags_ |= DecodedInstruction::LOWERCASE_MNEMONICS;
	if (!d.get_bool("uppercase_size"))
		disasm_flags_ |= DecodedInstruction::LOWERCASE_SIZE;

	if (d.get_bool("long_colon"))
		disasm_flags_ |= DecodedInstruction::COLON_IN_LONG_ARGS;
	if (d.get_bool("addr_colon"))
		disasm_flags_ |= DecodedInstruction::COLON_IN_ADDRESS;
}


BOOL DisasmDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;
	return TRUE;
}

DisasmDoc::~DisasmDoc()
{}


BEGIN_MESSAGE_MAP(DisasmDoc, CDocument)
END_MESSAGE_MAP()


// DisasmDoc diagnostics

#ifdef _DEBUG
void DisasmDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void DisasmDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

// DisasmDoc serialization

void DisasmDoc::Serialize(CArchive& ar)
{
	if (debugger_ == nullptr)
		return;

	if (ar.IsStoring())
	{
		// save disassembly

		auto pos= GetFirstViewPosition();
		auto view= static_cast<DisasmView*>(GetNextView(pos));
		if (view == nullptr)
			return;

		const auto& code= debugger_->GetCode();
		cf::BinaryProgramRange it(code);
		bool code_bytes= view->ShowCodeBytes();
		bool ascii= view->ShowCodeAscii();

		while (it)
		{
			auto part= it.Range();

			auto addr= part.first;

			while (addr < part.second && addr >= part.first)
			{
				std::string line= GetDeasmLine(addr, code_bytes, ascii) + "\r\n";
				ar.Write(line.c_str(), static_cast<UINT>(line.size()));
			}

			ar.WriteString(L"\n");

			++it;
		}
	}
	else
	{
		// should not call loading
		assert(false);
	}
}


void DisasmDoc::SetDebugger(Debugger* dbg)
{
	debugger_ = dbg;
	addr_ = dbg->GetCode().GetProgramStart();

	//TODO:  update pointer

	UpdateAllViews(nullptr);
}


Debugger* DisasmDoc::GetDebugger()
{
	return debugger_;
}


std::string DisasmDoc::GetDeasmLine(cf::uint32& addr, bool code_bytes, bool ascii) const
{
	if (debugger_ == nullptr)
		return std::string();

	uint32 address= addr;
	DecodedInstruction d= debugger_->DecodeInstruction(address);
	addr += d.Length();
	if (d.Length() == 0)	// no valid memory? go to the next address
		addr += 2;
	unsigned int flags= DecodedInstruction::SHOW_ADDRESS | disasm_flags_;
	if (code_bytes)
		flags |= DecodedInstruction::SHOW_CODE_BYTES;
	if (ascii)
		flags |= DecodedInstruction::SHOW_CODE_CHARS;

	return d.ToString(address, flags, ' ');
}


cf::uint32 DisasmDoc::GetStartAddr() const
{
	return addr_;
}


void DisasmDoc::SetStartAddr(cf::uint32 addr)
{
	addr_ = addr & ~cf::uint32(1);
}


cf::uint32 DisasmDoc::FindAddress(cf::uint32 addr, int delta_lines) const
{
	auto limit= 4000;	// safety count
	if (delta_lines > 0 && delta_lines < limit)
	{
		for (int i= 0; i < delta_lines; ++i)
		{
			DecodedInstruction d= debugger_->DecodeInstruction(addr);
			addr += std::max<uint32>(d.Length(), 2);
		}
	}
	else if (delta_lines < 0 && delta_lines > -limit)
	{
		int lines= -delta_lines;	// positive value
		// going backwards is tricky
		std::vector<uint32> offsets;
		offsets.reserve(lines);
		int go_back= std::max(2, lines);
		auto top= addr - std::min<uint32>(addr, go_back * 6);	// go back far enough; this is pessimistic estimate
		offsets.push_back(top);
		for (int i= 0; i < 3 * go_back; ++i)
		{
			DecodedInstruction d= debugger_->DecodeInstruction(top);
			top += std::max<uint32>(d.Length(), 2);
			offsets.push_back(top);
			if (top >= addr)
				break;
		}
		addr = offsets.at(uint32(lines) < offsets.size() ? offsets.size() - lines - 1 : 0);
	}
	else
	{
		// just a crude approximation
		addr += 2 * delta_lines;
	}

	return addr;
}


cf::uint32 DisasmDoc::FindAbsAddress(cf::uint32 addr) const
{

	return addr & ~cf::uint32(1);
}

cf::uint32 DisasmDoc::GetPC() const
{
	if (debugger_ == nullptr)
		return ~0;

	return debugger_->GetRegister(cf::R_PC);
}
