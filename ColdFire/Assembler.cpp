/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2013 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "Assembler.h"
#include "CFAsm.h"
#include "InstructionRepository.h"

struct Assembler::Impl
{
	std::string last_msg_;
	int last_line_;
	std::wstring path_;
	cf::BinaryProgram code_;
	std::unique_ptr<masm::DebugInfo> debug_;
};


Assembler::Assembler() : impl_(new Impl())
{
	impl_->last_line_ = 0;
	impl_->debug_.reset(new masm::DebugInfo());
}

Assembler::~Assembler()
{
	delete impl_;
}


StatCode Assembler::Assemble(const wchar_t* file, ISA isa, bool case_sensitive)
{
	try
	{
		masm::CFAsm a(file, case_sensitive, impl_->debug_.get(), nullptr, isa);// | ISA::MAC);
		auto stat= a.Assemble();

		impl_->last_msg_ = a.GetErrMsg(stat);
		impl_->last_line_ = a.GetLineNo() + 1;
		impl_->path_ = a.GetFileName().c_str();
		impl_->code_ = a.GetProgram();

		return stat;
	}
	catch (LogicError& ex)
	{
		// internal error; not something user can help, but should report it
		impl_->last_msg_ = "Internal logic program error. ";
		impl_->last_msg_ += ex.what();
	}
	catch (RunTimeError& ex)
	{
		// internal error; perhaps user can help it if it's due to some bogus data (like bad binary code)
		impl_->last_msg_ = "Internal run time program error. ";
		impl_->last_msg_ += ex.what();
	}
	catch (std::exception& ex)
	{
		impl_->last_msg_ = "Unexpected error. ";
		impl_->last_msg_ += ex.what();
	}
	catch (...)
	{
		impl_->last_msg_ = "Fatal error";
	}

	return ERR_INTERNAL;
}


std::string Assembler::LastMessage() const
{
	return impl_->last_msg_;
}


int Assembler::LastLine() const
{
	return impl_->last_line_;
}


const std::wstring& Assembler::GetPath() const
{
	return impl_->path_;
}


const cf::BinaryProgram& Assembler::GetCode() const
{
	return impl_->code_;
}


std::unique_ptr<DebugData> Assembler::TakeOverDebug()
{
	if (impl_->debug_.get() == 0)
		throw RunTimeError("no debug info");

	return std::unique_ptr<DebugData>(new DebugData(std::move(impl_->debug_)));
}


std::string Assembler::GetAllMnemonics(ISA isa) const
{
	std::set<std::string> set;
	static InstructionSize sizes[]= { IS_BYTE, IS_WORD, IS_LONG, IS_SHORT };

	auto instructions= GetInstructions().GetInstructions(isa);
	for (auto i : instructions)
	{
		if (i->SupportedSizes() == IS_UNSIZED || i->DefaultSize() != IS_NONE)
		{
			set.insert(i->Mnemonic());
			if (i->AlternativeMnemonic())
				set.insert(i->AlternativeMnemonic());
		}

		for (auto s : sizes)
			if (s & i->SupportedSizes())
			{
				std::string prefix;
				switch (s)
				{
				case IS_BYTE: prefix = ".b"; break;
				case IS_WORD: prefix = ".w"; break;
				case IS_LONG: prefix = ".l"; break;
				case IS_SHORT: prefix = ".s"; break;
				}

				set.insert(i->Mnemonic() + prefix);

				if (i->AlternativeMnemonic())
					set.insert(i->AlternativeMnemonic() + prefix);
			}
	}

	std::ostringstream out;
	for (auto& s : set)
		out << s << ' ';

	return out.str();
}
