/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#pragma once
#include "MapFile.h"
#include "Asm.h"
#include "Ident.h"

#ifndef _debug_info_h_
#define _debug_info_h_

namespace masm {

struct LineInfo
{
	int ln;			// line number in a source file
	FileUID file;	// identifier of a source file

	LineInfo(int ln, FileUID file) : ln(ln), file(file)
	{}
	LineInfo() : ln(0), file(0)
	{}

	bool operator == (const LineInfo &arg) const
	{ return ln == arg.ln && file == arg.file; }

	bool operator < (const LineInfo& l) const
	{
		return ln < l.ln && file < l.file;
	}
};

struct line_hash
{
	size_t operator () (const masm::LineInfo& line) const
	{
		size_t n= line.ln + (line.file << 16);
		return std::hash<size_t>()(n);
	}
};


struct DebugLine // debug info for a single line
{
	uint8 flags;		// DbgFlag
	uint32 addr;		// address
	LineInfo line;
	DebugLine() : flags(masm::DBG_EMPTY), addr(0)
	{}
	DebugLine(int ln, FileUID uid, uint32 addr, int flg) : flags((uint8)flg), addr(addr), line(ln,uid)
	{}
};


class DebugLines
{
	std::unordered_map<uint32, size_t> addr_to_idx_;
	std::unordered_map<LineInfo, size_t, line_hash> line_to_idx_;
	std::vector<DebugLine> lines_;

public:
	DebugLines();

	// znalezienie wiersza odpowiadaj¹cego adresowi
	void GetLine(DebugLine& ret, uint32 addr);

	// znalezienie adresu odp. wierszowi
	bool GetAddress(DebugLine& ret, int ln, FileUID file);

	void AddLine(const DebugLine& dl);

	void Empty();
};


class CDebugBreakpoints //: CAsm, CByteArray	// informacja o miejscach przerwañ
{
	uint32 temp_bp_index;
public:
	CDebugBreakpoints() : temp_bp_index(0)
	{}

	Breakpoint Set(uint32 addr, int bp= BPT_EXECUTE)	// ustawienie przerwania
	{
		ASSERT( (bp & ~BPT_MASK) == 0 );	// niedozwolona kombinacja bitów okreœlaj¹cych przerwanie
		return BPT_NONE;//Breakpoint( (*this)[addr] |= bp );
	}
	Breakpoint Clr(uint32 addr, int bp= BPT_MASK)		// skasowanie przerwania
	{
		ASSERT( (bp & ~BPT_MASK) == 0 );	// niedozwolona kombinacja bitów okreœlaj¹cych przerwanie
		return BPT_NONE;//Breakpoint( (*this)[addr] &= ~bp );
	}
	Breakpoint Toggle(uint32 addr, int bp)
	{
		ASSERT( (bp & ~BPT_MASK) == 0 );	// niedozwolona kombinacja bitów okreœlaj¹cych przerwanie
		return BPT_NONE;//Breakpoint( (*this)[addr] &= ~bp );
	}
	Breakpoint Get(uint32 addr)
	{ return BPT_NONE; } //Breakpoint( (*this)[addr] ); }

	void Enable(uint32 addr, bool enable= true)
	{
		//ASSERT( (*this)[addr] & BPT_MASK );	// pod danym adresem nie ma przerwania
		//if (enable)
		//	(*this)[addr] &= ~BPT_DISABLED;
		//else
		//	(*this)[addr] |= BPT_DISABLED;
	}
	void ClrBrkp(uint32 addr)		// skasowanie przerwania
	{}// (*this)[addr] = BPT_NONE; }

	void SetTemporaryExec(uint32 addr)
	{
		//temp_bp_index = addr;
		//(*this)[addr] |= BPT_TEMP_EXEC;
	}
	void RemoveTemporaryExec()
	{
		//(*this)[temp_bp_index] &= ~BPT_TEMP_EXEC;
	}

	void ClearAll()		// usuniêcie wszystkich przerwañ
	{
		//memset(m_pData,BPT_NONE,m_nSize*sizeof(BYTE));
	}
};



class DebugIdents				// informacja o identyfikatorach
{
//	CStringArray m_name;
//	CArray<Ident, const Ident&> m_info;

public:
	void SetArrSize(int size)
	{
		//m_name.RemoveAll();
		//m_info.RemoveAll();
		//m_name.SetSize(size);
		//m_info.SetSize(size);
	}
	void SetIdent(int index, const std::string& name, const Ident &info)
	{
		//m_name.SetAt(index,name);
		//m_info.SetAt(index,info);
	}
	void GetIdent(int index, std::string& name, Ident &info)
	{
		//name = m_name[index];	// m_name.ElementAt(index);
		//info = m_info[index];	// m_info.ElementAt(index);
	}
	int GetCount()
	{
		//ASSERT(m_name.GetSize() == m_info.GetSize());
		//return m_name.GetSize();
		return 0;
	}
	void Empty()
	{
		//m_name.RemoveAll();
		//m_info.RemoveAll();
	}
};


class DebugInfo
{
	DebugLines m_lines;			// informacja o wierszach
	DebugIdents m_idents;		// informacja o identyfikatorach
	CDebugBreakpoints m_breakpoints;	// informacja o miejscach przerwañ
	MapFile m_map_file;			// odwzorowania fazwy pliku Ÿród³owego na 'fuid' i odwrotnie

public:

	void Empty()
	{ m_lines.Empty(); m_idents.Empty(); }

	void AddLine(DebugLine &dl)
	{ m_lines.AddLine(dl); }

	void GetLine(DebugLine &ret, uint32 addr)	// znalezienie wiersza odpowiadaj¹cego adresowi
	{ m_lines.GetLine(ret,addr); }

	bool GetAddress(DebugLine &ret, int ln, FileUID file)	// znalezienie adresu odp. wierszowi
	{ return m_lines.GetAddress(ret,ln,file); }

	Breakpoint SetBreakpoint(int line, FileUID file, int bp= BPT_NONE);// ustawienie przerwania
	Breakpoint ToggleBreakpoint(int line, FileUID file);
	Breakpoint GetBreakpoint(int line, FileUID file);
	Breakpoint ModifyBreakpoint(int line, FileUID file, int bp);
	void ClrBreakpoint(int line, FileUID file);
	Breakpoint GetBreakpoint(uint32 addr)
	{ return m_breakpoints.Get(addr); }

	void SetTemporaryExecBreakpoint(uint32 addr)
	{ m_breakpoints.SetTemporaryExec(addr); }

	void RemoveTemporaryExecBreakpoint()
	{ m_breakpoints.RemoveTemporaryExec(); }

	FileUID GetFileUID(const Path& doc_title)
	{ return m_map_file.GetFileUID(doc_title); }		// ID pliku
	const Path& GetFilePath(FileUID fuid)
	{ return m_map_file.GetPath(fuid); }
	//{ return fuid ? m_map_file.GetPath(fuid) : nullptr; }	// nazwa (œcie¿ka do) pliku
	void ResetFileMap()
	{ m_map_file.Reset(); }

	void SetIdentArrSize(int size)
	{ m_idents.SetArrSize(size); }
	void SetIdent(int index, const std::string& name, const Ident &info)
	{ m_idents.SetIdent(index,name,info); }
	void GetIdent(int index, std::string& name, Ident &info)
	{ m_idents.GetIdent(index,name,info); }
	int GetIdentCount()
	{ return m_idents.GetCount(); }
};

} // namespace

#endif
