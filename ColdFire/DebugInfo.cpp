/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "DebugInfo.h"

namespace masm {


template<class K, class T>
bool Lookup(std::unordered_map<K, T>& m, const K& key, T& element)
{
	auto it= m.find(key);
	if (it == m.end())
		return false;
	element = it->second;
	return true;
}


DebugLines::DebugLines()
{
	addr_to_idx_.max_load_factor(0.7f);
	addr_to_idx_.reserve(1000);
	line_to_idx_.max_load_factor(0.7f);
	line_to_idx_.reserve(1000);
	lines_.reserve(1000);
}


void DebugLines::GetLine(DebugLine& ret, uint32 addr)
{
	static const DebugLine empty;	// pusty obiekt - do oznaczenia "nie znaleziony wiersz"
	size_t idx;
	if (Lookup(addr_to_idx_, addr, idx))
		ret = lines_.at(idx);
	else
		ret = empty;
}


bool DebugLines::GetAddress(DebugLine &ret, int ln, FileUID file)
{
	static const DebugLine empty;	// pusty obiekt - do oznaczenia "nie znaleziony adres"
//	size_t idx;
	auto size= line_to_idx_.size();
	auto li= LineInfo(ln, file);
	auto it= line_to_idx_.find(li);
	if (it != line_to_idx_.end())
	//if (Lookup(line_to_idx, li, idx))
	{
		ret = lines_.at(it->second);
		return true;
	}
	else
	{
		ret = empty;
		return false;
	}
}


void DebugLines::AddLine(const DebugLine& dl)
{
	ASSERT(dl.flags != DBG_EMPTY);	// niewype³niony opis wiersza
	size_t idx= lines_.size();
	lines_.push_back(dl);		// dopisanie info o wierszu, zapamiêtanie indeksu
	addr_to_idx_[dl.addr] = idx;	// zapisanie indeksu
//	line_to_idx[dl.line] = idx;	// j.w.
	line_to_idx_[dl.line] = idx;
}


void DebugLines::Empty()
{
	lines_.clear(); //RemoveAll();
	line_to_idx_.clear();
	addr_to_idx_.clear();
	line_to_idx_.clear();
}


//-------------------------------------------------------------------------------


// ustawienie przerwania
masm::Breakpoint DebugInfo::SetBreakpoint(int line, FileUID file, int bp)
{
  ASSERT( (bp & ~BPT_MASK) == 0 );	// niedozwolona kombinacja bitów okreœlaj¹cych przerwanie

  DebugLine dl;
  GetAddress(dl,line,file);	// znalezienie adresu odpowiadaj¹cego wierszowi
  if (dl.flags == DBG_EMPTY || (dl.flags & DBG_MACRO))
    return BPT_NO_CODE;		// nie ma kodu w wierszu 'line'
  if (bp == BPT_NONE)		// rodzaj przerwania nie zosta³ podany?
    bp = dl.flags & DBG_CODE ? BPT_EXECUTE : BPT_READ|BPT_WRITE|BPT_EXECUTE;
  return m_breakpoints.Set(dl.addr,bp);
}


masm::Breakpoint DebugInfo::ModifyBreakpoint(int line, FileUID file, int bp)
{
  ASSERT( (bp & ~(BPT_MASK|BPT_DISABLED)) == 0 );	// niedozwolona kombinacja bitów okreœlaj¹cych przerwanie

  DebugLine dl;
  GetAddress(dl,line,file);	// znalezienie adresu odpowiadaj¹cego wierszowi
  if (dl.flags == DBG_EMPTY || (dl.flags & DBG_MACRO))
    return BPT_NO_CODE;		// nie ma kodu w wierszu 'line'
  if ((bp & BPT_MASK) == BPT_NONE)
  {
    m_breakpoints.ClrBrkp(dl.addr);	// skasowanie przerwania
    return BPT_NONE;
  }
  bp = m_breakpoints.Set(dl.addr,bp & ~BPT_DISABLED);
  m_breakpoints.Enable(dl.addr, !(bp & BPT_DISABLED) );
  return (Breakpoint)bp;
}


masm::Breakpoint DebugInfo::GetBreakpoint(int line, FileUID file)
{
  DebugLine dl;
  GetAddress(dl,line,file);	// znalezienie adresu odpowiadaj¹cego wierszowi
  if (dl.flags == DBG_EMPTY || (dl.flags & DBG_MACRO))
  {
//    ASSERT(FALSE);
    return BPT_NO_CODE;		// nie ma kodu w wierszu 'line'
  }
  return m_breakpoints.Get(dl.addr);
}


masm::Breakpoint DebugInfo::ToggleBreakpoint(int line, FileUID file)
{
  DebugLine dl;
  GetAddress(dl,line,file);	// znalezienie adresu odpowiadaj¹cego wierszowi
  if (dl.flags == DBG_EMPTY || (dl.flags & DBG_MACRO))
    return BPT_NO_CODE;		// nie ma kodu w wierszu 'line'

  if (m_breakpoints.Get(dl.addr) != BPT_NONE)	// jest ju¿ ustawione przerwanie?
    return m_breakpoints.Clr(dl.addr);
  else
    return m_breakpoints.Set(dl.addr, dl.flags & DBG_CODE ? BPT_EXECUTE : BPT_READ|BPT_WRITE|BPT_EXECUTE);
}


void DebugInfo::ClrBreakpoint(int line, FileUID file)
{
  DebugLine dl;
  GetAddress(dl,line,file);	// znalezienie adresu odpowiadaj¹cego wierszowi
  if (dl.flags == DBG_EMPTY || (dl.flags & DBG_MACRO))
  {
    ASSERT(false);		// nie ma kodu w wierszu 'line'
    return;
  }
  m_breakpoints.Clr(dl.addr);
}

} // namespace
